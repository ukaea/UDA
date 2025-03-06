#include <catch2/catch_test_macros.hpp>
#include "client2/client.hpp"
#include <string>

//TODO:
// - setting and resetting options
// - ssl stuff
// - get and put functions ?
// - new and free handle
// - idam data blocks
// - server errors

// class MockClient : public uda::client::Client
// {
//
// };

TEST_CASE( "client object can be default constructed without supplying a config file", "[client-instantiation]" )
{
    REQUIRE_NOTHROW(uda::client::Client());
}

TEST_CASE( "client object can be constructed with a config file", "[client-instantiation]" )
{
    std::string file_path = "test_files/uda-client-config.toml";
    REQUIRE_NOTHROW(uda::client::Client(file_path));
}

// TEST_CASE( "client options can be set from a config file", "[option-parsing]" )
// {
//     FAIL("Test not implemented yet");
// }

