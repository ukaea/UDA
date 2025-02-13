#define CATCH_CONFIG_MAIN
#include "catch.hpp"

#include <filesystem>
#include <boost/algorithm/string.hpp>

#include "server2/server.hpp"
#include "clientserver/makeRequestBlock.h"

int entry_func(UDA_PLUGIN_INTERFACE* plugin_interface)
{
    std::string function = boost::to_lower_copy<std::string>(udaPluginFunction(plugin_interface));

    if (function == "help") {
        udaPluginReturnDataStringScalar(plugin_interface, "test data", "test description");
        return 0;
    }

    return -1;
}

int no_op(void*) { return 0; }

class TestServer : public uda::server::Server {
public:
    TestServer() : Server(uda::config::Config{}) {
        std::filesystem::path path{ __FILE__ };
        auto config_path = path.parent_path() / "test_server.toml";
        config_.load(config_path.string());
        uda::logging::init_logging();
        uda::logging::set_log_level(uda::logging::LogLevel::UDA_LOG_NONE);

        uda::client_server::PluginData plugin;
        plugin.name = "test";
        plugin.entry_func = entry_func;
        plugin.type = UdaPluginClass::UDA_PLUGIN_CLASS_FUNCTION;
        plugin.handle = {this, no_op};

        _plugins.add_plugin(std::move(plugin));
    }

    uda::client_server::RequestData make_request(const std::string& request)
    {
        uda::client_server::RequestData request_data = {};

        strcpy(request_data.signal, request.c_str());

        uda::client_server::make_request_data(config_, &request_data, _plugins.plugin_list());

        return request_data;
    }

    int get(const std::string& request, uda::client_server::DataBlock& data_block)
    {
        int depth = 0;

        uda::client_server::RequestData request_data = make_request(request);
        strcpy(_metadata_block.data_source.format, "test");

        return get_data(&depth, &request_data, &data_block, TestServer::ServerVersion);
    }

};

TEST_CASE( "Test create server", "[server]" )
{
    uda::config::Config config;
    TestServer server;
}

TEST_CASE( "Test make request", "[server]" )
{
    uda::config::Config config;
    TestServer server;

    SECTION( "Test plugin function with no args" ) {
        auto request= server.make_request("TEST::HELP()");
        REQUIRE( std::string{ request.api_delim } == "::" );
        REQUIRE( std::string{ request.signal } == "HELP()" );
        REQUIRE( std::string{ request.archive } == "TEST" );
        REQUIRE( std::string{ request.function } == "HELP" );
        REQUIRE( std::string{ request.format } == "test" );
        REQUIRE( request.request == 0 );
        REQUIRE( request.nameValueList.listSize == 0 );
    }

    SECTION( "Test plugin function with one arg" ) {
        auto request= server.make_request("TEST::HELP(foo=1)");
        REQUIRE( std::string{ request.api_delim } == "::" );
        REQUIRE( std::string{ request.signal } == "HELP(foo=1)" );
        REQUIRE( std::string{ request.archive } == "TEST" );
        REQUIRE( std::string{ request.function } == "HELP" );
        REQUIRE( std::string{ request.format } == "test" );
        REQUIRE( request.request == 0 );
        REQUIRE( request.nameValueList.pairCount == 1 );
        REQUIRE( std::string{ request.nameValueList.nameValue[0].name } == "foo" );
        REQUIRE( std::string{ request.nameValueList.nameValue[0].value } == "1" );
    }

    SECTION( "Test plugin function with two args" ) {
        auto request= server.make_request("TEST::HELP(foo=1, bar=zog)");
        REQUIRE( std::string{ request.api_delim } == "::" );
        REQUIRE( std::string{ request.signal } == "HELP(foo=1, bar=zog)" );
        REQUIRE( std::string{ request.archive } == "TEST" );
        REQUIRE( std::string{ request.function } == "HELP" );
        REQUIRE( std::string{ request.format } == "test" );
        REQUIRE( request.request == 0 );
        REQUIRE( request.nameValueList.pairCount == 2 );
        REQUIRE( std::string{ request.nameValueList.nameValue[0].name } == "foo" );
        REQUIRE( std::string{ request.nameValueList.nameValue[0].value } == "1" );
        REQUIRE( std::string{ request.nameValueList.nameValue[1].name } == "bar" );
        REQUIRE( std::string{ request.nameValueList.nameValue[1].value } == "zog" );
    }

    SECTION( "Test plugin function with flag arg" ) {
        auto request= server.make_request("TEST::HELP(foo)");
        REQUIRE( std::string{ request.api_delim } == "::" );
        REQUIRE( std::string{ request.signal } == "HELP(foo)" );
        REQUIRE( std::string{ request.archive } == "TEST" );
        REQUIRE( std::string{ request.function } == "HELP" );
        REQUIRE( std::string{ request.format } == "test" );
        REQUIRE( request.request == 0 );
        REQUIRE( request.nameValueList.pairCount == 1 );
        REQUIRE( std::string{ request.nameValueList.nameValue[0].name } == "foo" );
        REQUIRE( std::string{ request.nameValueList.nameValue[0].value } == "true" );
    }

    SECTION( "Test plugin function with IDL style flag arg" ) {
        auto request= server.make_request("TEST::HELP(/foo)");
        REQUIRE( std::string{ request.api_delim } == "::" );
        REQUIRE( std::string{ request.signal } == "HELP(/foo)" );
        REQUIRE( std::string{ request.archive } == "TEST" );
        REQUIRE( std::string{ request.function } == "HELP" );
        REQUIRE( std::string{ request.format } == "test" );
        REQUIRE( request.request == 0 );
        REQUIRE( request.nameValueList.pairCount == 1 );
        REQUIRE( std::string{ request.nameValueList.nameValue[0].name } == "foo" );
        REQUIRE( std::string{ request.nameValueList.nameValue[0].value } == "true" );
    }

    SECTION( "Test plugin function with mix of arg types" ) {
        auto request= server.make_request("TEST::HELP(foo=1, /bar)");
        REQUIRE( std::string{ request.api_delim } == "::" );
        REQUIRE( std::string{ request.signal } == "HELP(foo=1, /bar)" );
        REQUIRE( std::string{ request.archive } == "TEST" );
        REQUIRE( std::string{ request.function } == "HELP" );
        REQUIRE( std::string{ request.format } == "test" );
        REQUIRE( request.request == 0 );
        REQUIRE( request.nameValueList.pairCount == 2 );
        REQUIRE( std::string{ request.nameValueList.nameValue[0].name } == "foo" );
        REQUIRE( std::string{ request.nameValueList.nameValue[0].value } == "1" );
        REQUIRE( std::string{ request.nameValueList.nameValue[1].name } == "bar" );
        REQUIRE( std::string{ request.nameValueList.nameValue[1].value } == "true" );
    }
}

TEST_CASE( "Test get data", "[server]" )
{
    uda::config::Config config;
    TestServer server;

    uda::client_server::DataBlock data_block;

    SECTION( "Testing help method" ) {
        int rc = server.get("TEST::HELP()", data_block);
        REQUIRE( rc == 0 );
        REQUIRE( data_block.data_type == UDA_TYPE_STRING );
        REQUIRE( std::string{ data_block.data } == "test data" );
        REQUIRE( std::string{ data_block.data_desc } == "test description" );
    }

}