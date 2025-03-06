#include <catch2/catch_test_macros.hpp>
#include "client2/connection.hpp"
#include "config/config.h"
#include "clientserver/socket_structs.h"
#include "clientserver/uda_structs.h"
#include <string>
#include <rpc/rpc.h>
#include <utility>
#include <ctime>
#include <thread>
#include <chrono>

TEST_CASE( "connection object can be default constructed without supplying a config file", "[client-instantiation]" )
{
    std::vector<uda::client_server::UdaError> error_stack;
    REQUIRE_NOTHROW(uda::client::Connection(error_stack));
    REQUIRE(error_stack.empty());
}

TEST_CASE( "connection object can be constructed with a config file", "[client-instantiation]" )
{
    const std::string file_path = "test_files/uda-client-config.toml";
    std::vector<uda::client_server::UdaError> error_stack;
    uda::config::Config config = {};
    config.load(file_path);
    REQUIRE_NOTHROW(uda::client::Connection(error_stack, config));
}

TEST_CASE( "connection options can be set from a config file", "[option-parsing]" )
{
    std::vector<uda::client_server::UdaError> error_stack;
    std::string file_path = "test_files/client-config-with-hosts.toml";
    uda::config::Config config = {};
    config.load(file_path);
    uda::client::Connection connection(error_stack, config);

    REQUIRE( connection.get_host() == "host2.uda.uk" );
    REQUIRE( connection.get_port() == 56789 );

    const auto options = connection.get_options();
    REQUIRE( options.max_socket_delay == 25 );
    REQUIRE( options.max_socket_attempts == 5 );
    REQUIRE( options.host == "host2.uda.uk" );
    REQUIRE( options.port == 56789 );
}

TEST_CASE( "connection options can be set through set_option methods", "[option-interface]" )
{
    std::vector<uda::client_server::UdaError> error_stack;
    std::string file_path = "test_files/client-config-with-hosts.toml";
    uda::config::Config config = {};
    config.load(file_path);
    uda::client::Connection connection(error_stack, config);

    REQUIRE( connection.get_host() == "host2.uda.uk" );
    REQUIRE( connection.get_port() == 56789 );

    SECTION( "new host from host list" )
    {
        REQUIRE_NOTHROW( connection.set_host("host3") );
        REQUIRE( connection.get_host() == "host3.uda.uk" );
        REQUIRE( connection.get_port() == 12345 );
        REQUIRE( connection.reconnect_required() );
    }

    SECTION( "new host and port not on host list" )
    {
        std::string new_host = "my.new.host";
        int new_port = 9999;
        REQUIRE_NOTHROW( connection.set_host(new_host) );
        REQUIRE_NOTHROW( connection.set_port(new_port) );
        REQUIRE( connection.get_host() == new_host );
        REQUIRE( connection.get_port() == new_port );
        REQUIRE( connection.reconnect_required() );
    }
}

// NOTE! only using method hiding here since functions are not marked virtual in source code
// this means no runtime polymorphism will work. beware (and possibly change later)
class MockConnection : public uda::client::Connection
{
    public:
    int create()
    {
        client_socket_ = socket_list_.size();
        uda::client_server::Socket socket = {};

        // socket.type = uda::client_server::SocketType::UDA;
        socket.open = true;
        socket.fh = client_socket_;
        socket.port = port_;
        socket.host = host_;
        socket.tv_server_start = time(nullptr);
        socket.user_timeout = 600;
        socket.Input = nullptr;
        socket.Output = nullptr;

        socket_list_.push_back(socket);
        startup_state = true;
        server_reconnect_ = false;

        return 0;
    }

    const std::vector<uda::client_server::Socket>& get_socket_list() const
    {
        return socket_list_;
    }

    int get_current_client_socket() const
    {
        return client_socket_;
    }

    // mock the close socket method to just set filehandle to -1
    // do not actually call close...
    void close_socket(int fh)
    {
        for (auto& socket : socket_list_) {
            if (socket.open && socket.fh == fh && socket.fh >= 0) {
                socket.open = false;
                socket.fh = -1;
                socket.Input = nullptr;
                socket.Output = nullptr;
                break;
            }
        }

    }

};

std::pair<XDR*, XDR*> createXDRStream()
{
    static XDR client_input = {};
    static XDR client_output = {};

    client_input.x_op = XDR_DECODE;
    client_output.x_op = XDR_ENCODE;

    return std::make_pair(&client_input, &client_output);
}

TEST_CASE( "Current socket connection details can be queried", "[socket-data]" )
{
    std::vector<uda::client_server::UdaError> error_stack;
    MockConnection connection {error_stack};
    connection.create();

    XDR* client_input;
    XDR* client_output;
    std::tie(client_input, client_output) = createXDRStream();
    connection.register_xdr_streams(client_input, client_output);

    const auto& socket = connection.get_current_connection_data();
    REQUIRE( socket.open == true );
    REQUIRE( socket.fh == 0 );
    REQUIRE( socket.port == 56565 );
    REQUIRE( std::string(socket.host) == "localhost" );
    // don't know exact values of time(now) so just check that values are similar.
    REQUIRE( std::difftime(socket.tv_server_start, time(nullptr)) < 1.0 );
    REQUIRE( socket.user_timeout == 600 );
    REQUIRE( socket.Input == client_input );
    REQUIRE( socket.Output == client_output );
    REQUIRE( connection.error_stack.empty() );
}

TEST_CASE( "Maximum age of the current socket connection can be modified", "[socket-data]" )
{
    int expected_result = 100;

    std::vector<uda::client_server::UdaError> error_stack;
    MockConnection connection {error_stack};
    connection.create();
    connection.set_maximum_socket_age(expected_result);

    const auto& socket = connection.get_current_connection_data();
    REQUIRE( socket.user_timeout == expected_result );
}

TEST_CASE ( "Current socket connection age can be queried", "[socket-data]")
{
    std::vector<uda::client_server::UdaError> error_stack;
    MockConnection connection {error_stack};
    connection.create();

    //NOTE: precision of age is 1s minimum so we have to wait for an entire second here
    std::this_thread::sleep_for(std::chrono::seconds(1));
    auto age = connection.get_current_socket_age();
    REQUIRE( age >= 1.0 );
    // REQUIRE( (age > 0.03 and age < 1.0) );

    // std::this_thread::sleep_for(std::chrono::milliseconds(30));
    // age = connection.get_current_socket_age();
    // REQUIRE( (age > 0.06 and age < 1.0) );
}

TEST_CASE( "Current socket connection timeout status can be queried", "[socket-data]" )
{
    std::vector<uda::client_server::UdaError> error_stack;
    MockConnection connection {error_stack};
    connection.create();

    connection.set_maximum_socket_age(100);
    REQUIRE_FALSE( connection.current_socket_timeout() );

    connection.set_maximum_socket_age(0);
    REQUIRE( connection.current_socket_timeout() );
}

TEST_CASE( "Port can be set which also flags that reconnection is required" "[set-connection-options]")
{
    std::vector<uda::client_server::UdaError> error_stack;
    uda::client::Connection connection {error_stack};

    auto port_number = connection.get_port();
    REQUIRE( port_number == uda::client::DefaultPort );
    int expected_result = 12345;
    connection.set_port(expected_result);
    port_number = connection.get_port();
    REQUIRE( port_number == expected_result );
    REQUIRE( connection.reconnect_required());
    REQUIRE( error_stack.empty() );
}

TEST_CASE( "Current socket connection can be closed", "[close-socket]" )
{
    std::vector<uda::client_server::UdaError> error_stack;
    MockConnection connection {error_stack};
    const auto& socket_list = connection.get_socket_list();
    REQUIRE( socket_list.size() == 0 );
    REQUIRE_FALSE( connection.open() );

    connection.create();
    REQUIRE( socket_list.size() == 1 );
    REQUIRE( connection.open() );

    connection.close_down(uda::client::ClosedownType::CLOSE_SOCKETS);
    REQUIRE( socket_list.size() == 1 );
    REQUIRE_FALSE( connection.open() );
}

TEST_CASE( "Can reconnect to an existing socket" "[reconnect]")
{
    std::vector<uda::client_server::UdaError> error_stack;
    MockConnection connection {error_stack};
    const auto& socket_list = connection.get_socket_list();

    // create function should change to the newly created socket
    // and correctly set or reset the startup state and reconnection_required flags
    connection.set_host("localhost");
    connection.set_port(56789);
    REQUIRE( connection.reconnect_required() );
    connection.create();
    REQUIRE( connection.open() );
    REQUIRE( socket_list.size() == 1 );
    REQUIRE( connection.get_current_client_socket() == socket_list.back().fh );
    REQUIRE( connection.startup_state );
    REQUIRE_FALSE( connection.reconnect_required() );

    connection.set_host("host2.uda.uk");
    connection.set_port(12345);
    REQUIRE( connection.reconnect_required() );
    connection.create();
    REQUIRE( connection.open() );
    REQUIRE( socket_list.size() == 2 );
    REQUIRE( connection.get_current_client_socket() == socket_list.back().fh );
    REQUIRE( connection.startup_state );
    REQUIRE_FALSE( connection.reconnect_required() );

    connection.set_host("host3.uda.uk");
    connection.set_port(54321);
    REQUIRE( connection.reconnect_required() );
    connection.create();
    REQUIRE( connection.open() );
    REQUIRE( socket_list.size() == 3 );
    REQUIRE( connection.get_current_client_socket() == socket_list.back().fh );
    REQUIRE( connection.startup_state );
    REQUIRE_FALSE( connection.reconnect_required() );

    // if the (host,port) details match an existing connection, switch to it
    // and reset the reconnect_required flag
    connection.set_host("localhost");
    connection.set_port(56789);
    REQUIRE( connection.reconnect_required() );
    REQUIRE_NOTHROW( connection.maybe_reuse_existing_socket() );
    REQUIRE( socket_list.size() == 3 );
    auto socket = connection.get_current_connection_data();
    REQUIRE( socket.port == connection.get_port() );
    REQUIRE( socket.host == connection.get_host() );
    REQUIRE( connection.startup_state );
    REQUIRE_FALSE( connection.reconnect_required() );
}

