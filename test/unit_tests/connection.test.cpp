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

// TEST_CASE( "connection options can be set from a config file", "[option-parsing]" )
// {
//     FAIL("Test not implemented yet");
// }

// NOTE! only using method hiding here since functions are not marked virtual in source code
// this means no runtime polymorphism will work. beware (and possibly change later)
class MockConnection : public uda::client::Connection
{
    public:
    MockConnection() : Connection{error_stack} {}

    int create(XDR* client_input, XDR* client_output)
    {
        client_socket_ = socket_list_.size();
        uda::client_server::Socket socket = {};

        // socket.type = uda::client_server::SocketType::UDA;
        socket.open = true;
        socket.fh = client_socket_;
        socket.port = port_;
        socket.host = host_;
        socket.tv_server_start = time(nullptr);
        socket.user_timeout = 0;
        socket.Input = client_input;
        socket.Output = client_output;

        socket_list_.push_back(socket);
        return 0;
    }

    const std::vector<uda::client_server::Socket>& get_socket_list() const
    {
        return socket_list_;
    }

    std::vector<uda::client_server::UdaError> error_stack;

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
    XDR* client_input;
    XDR* client_output;
    std::tie(client_input, client_output) = createXDRStream();

    std::vector<uda::client_server::UdaError> error_stack;
    MockConnection connection{};
    connection.create(client_input, client_output);

    const auto& socket = connection.get_current_connection_data();
    REQUIRE( socket.open == true );
    // TODO: should we use an actual filehandle from a call to open(whatever)
    // here to avoid any other issues when close(fh) is called in close_down()?
    // fh = 0 technically corresponds to stdin...
    REQUIRE( socket.fh == 0 );
    REQUIRE( socket.port == 56565 );
    REQUIRE( std::string(socket.host) == "localhost" );
    // don't know exact values of time(now) so just check that values are similar.
    REQUIRE( std::difftime(socket.tv_server_start, time(nullptr)) < 1.0 );
    REQUIRE( socket.user_timeout == 0 );
    REQUIRE( socket.Input == client_input );
    REQUIRE( socket.Output == client_output );
    REQUIRE( connection.error_stack.empty() );
}

TEST_CASE( "Maximum age of the current socket connection can be modified", "[socket-data]" )
{
    int expected_result = 100;

    XDR* client_input;
    XDR* client_output;
    std::tie(client_input, client_output) = createXDRStream();

    MockConnection connection{};
    connection.create(client_input, client_output);
    connection.set_maximum_socket_age(expected_result);

    const auto& socket = connection.get_current_connection_data();
    REQUIRE( socket.user_timeout == expected_result );
}

TEST_CASE ( "Current socket connection age can be queried", "[socket-data]")
{
    XDR* client_input;
    XDR* client_output;
    std::tie(client_input, client_output) = createXDRStream();

    MockConnection connection{};
    connection.create(client_input, client_output);

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
    XDR* client_input;
    XDR* client_output;
    std::tie(client_input, client_output) = createXDRStream();

    std::vector<uda::client_server::UdaError> error_stack;
    MockConnection connection{};
    connection.create(client_input, client_output);

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

// TEST_CASE( "host and port can be set from a host-list file", "[set-connection-options]" )
// {
//     FAIL("Test not implemented yet");
// }

TEST_CASE( "Current socket connection can be closed", "[close-socket]" )
{
    XDR* client_input;
    XDR* client_output;
    std::tie(client_input, client_output) = createXDRStream();

    MockConnection connection{};
    const auto& socket_list = connection.get_socket_list();
    REQUIRE( socket_list.size() == 0 );
    REQUIRE_FALSE( connection.open() );

    connection.create(client_input, client_output);
    REQUIRE( socket_list.size() == 1 );
    REQUIRE( connection.open() );

    connection.close_down(uda::client::ClosedownType::CLOSE_SOCKETS);
    // NOTE: socket details not deleted from socket list? 
    // currently filehandle is only closed
    REQUIRE( socket_list.size() == 1 );
    REQUIRE_FALSE( connection.open() );
}

//TODO: it's not clear what this function should do. 
// currently just flags the connection as closed which later signals to the marshalling
// class to call the create function again
// can consider moving all marshalling logic into connection class here
// TEST_CASE( "Reconnect function closes all connections and creates new ones when required" "[reconnect]")
// {
//     FAIL("Test not implemented yet");
// }

