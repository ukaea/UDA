#include <catch2/catch_test_macros.hpp>
#include <catch2/catch_approx.hpp>
#include "config/config.h"
using namespace std::string_literals;

TEST_CASE( "config object can be default constructed", "[config-instantiation]" )
{
    REQUIRE_NOTHROW(uda::config::Config());
}

TEST_CASE( "A valid TOML config file can be loaded", "[config-instantiation]" )
{
    uda::config::Config config {};
    std::string file_path = "test_files/uda-client-config.toml";
    REQUIRE_NOTHROW( config.load(file_path) );
}

TEST_CASE( "Loading an invalid file path fails", "[config-instantiation]" )
{
    uda::config::Config config {};
    std::string file_path = "test_files/non-existant-file.toml";
    REQUIRE_THROWS_AS( config.load(file_path), uda::config::ConfigError );
}

TEST_CASE( "Loading an empty file succeeds", "[config-instantiation]" )
{
    uda::config::Config config {};
    std::string file_path = "test_files/empty.toml";
    REQUIRE_NOTHROW( config.load(file_path) );
}

TEST_CASE( "Validation fails when loading a file with an unrecognised section", "[config-instantiation]")
{
    uda::config::Config config {};
    std::string file_path = "test_files/unrecognised-section.toml";
    REQUIRE_THROWS_AS( config.load(file_path), uda::config::ConfigError );
}

TEST_CASE( "Validation fails when loading a file with an unrecognised field", "[config-instantiation]")
{
    uda::config::Config config {};
    std::string file_path = "test_files/unrecognised-field.toml";
    REQUIRE_THROWS_AS( config.load(file_path), uda::config::ConfigError );
}

TEST_CASE( "Validation fails when loading a file with an incorrectly typed field value", "[config-instantiation]")
{
    uda::config::Config config {};
    std::string file_path = "test_files/incorrect-field-type.toml";
    REQUIRE_THROWS_AS( config.load(file_path), uda::config::ConfigError );
}

// TEST_CASE( "Validation fails if toml nesting isn't exactly 1 layer deep (values sit in sections)", "[config-instantiation]")
// {
//     // uda::config::Config config {};
//     // std::string file_path = "test_files/unrecognised-section.toml";
//     // REQUIRE_THROWS_AS( config.load(file_path), uda::config::ConfigError );
//     FAIL("Test not implemented yet");
// }


TEST_CASE( "An initialised config object is truthy, and uninitialised is falsy", "[config-instantiation]" )
{
    uda::config::Config config {};
    REQUIRE_FALSE(config);
    std::string file_path = "test_files/uda-client-config.toml";
    config.load(file_path);
    REQUIRE(config);
}

TEST_CASE( "Config object can be initialised in memory for testing with no underlying TOML file on disk", "[config-instantiation]")
{
    uda::config::Config config {};
    REQUIRE_NOTHROW( config.load_in_memory() );
    REQUIRE(config);
}

TEST_CASE( "values of all required types are retrievable from a toml config file", "[config-get-data]")
{
    uda::config::Config config {};
    std::string file_path = "test_files/test-types.toml";
    config.load(file_path);
    REQUIRE(config);

    const auto boolean_value = config.get("test.boolean").as<bool>();
    REQUIRE( boolean_value == true );

    const auto integer_value = config.get("test.integer").as<int64_t>();
    REQUIRE( integer_value == 1);

    const auto float_value = config.get("test.float").as<double>();
    REQUIRE( float_value == Catch::Approx(5.2) );

    const auto string_value = config.get("test.string").as<std::string>();
    REQUIRE( string_value == "some text" );

    const auto char_value = config.get("test.char").as<std::string>();
    REQUIRE( char_value == "a" );
}

TEST_CASE( "whole sections of a toml config file can be returned as a std::unorderd_map", "[config-get-data]")
{
    uda::config::Config config {};
    std::string file_path = "test_files/uda-client-config.toml";
    config.load(file_path);
    REQUIRE(config);
    const auto result = config.get_section_as_map("connection");
    std::unordered_map<std::string, uda::config::Option> expected_result = 
    {
        {"host", {"host", "uda2.mast.l"}},
        {"port", {"port", 56565}},
        {"max_socket_delay", {"max_socket_delay", 10}},
        {"max_socket_attempts", {"max_socket_attempts", 3}}
    };
    REQUIRE(result == expected_result);
}

TEST_CASE( "values of all required types can be set in the in-memory representation of a toml config file", "[config-set-data]")
{
    uda::config::Config config {};
    config.load_in_memory();
    REQUIRE(config);
    REQUIRE_NOTHROW( config.set("test.boolean", true) );
    int64_t integer_value {};
    REQUIRE_NOTHROW( config.set("test.integer", integer_value) );
    double float_value {};
    REQUIRE_NOTHROW( config.set("test.float", float_value) );
    REQUIRE_NOTHROW( config.set("test.string", "some text"s) );
    REQUIRE_NOTHROW( config.set("test.char", "a"s) );
}

TEST_CASE( "validation fails when trying to set data in an unrecognised section", "[config-set-data]")
{
    uda::config::Config config {};
    config.load_in_memory();
    REQUIRE(config);
    REQUIRE_THROWS_AS( config.set("fake_section.field_1", false),  uda::config::ConfigError );
}

TEST_CASE( "validation fails when trying to set data in an unrecognised field", "[config-set-data]")
{
    uda::config::Config config {};
    config.load_in_memory();
    REQUIRE(config);
    REQUIRE_THROWS_AS( config.set("connection.fake_field", false),  uda::config::ConfigError );
}

TEST_CASE( "validation fails when trying to set incorrectly typed data for a given field", "[config-set-data]")
{
    uda::config::Config config {};
    config.load_in_memory();
    REQUIRE(config);
    REQUIRE_THROWS_AS( config.set("test.char", "not a char"),  uda::config::ConfigError );
    int64_t int_not_string {};
    REQUIRE_THROWS_AS( config.set("test.string", int_not_string),  uda::config::ConfigError );
    bool bool_not_int {};
    REQUIRE_THROWS_AS( config.set("test.integer", bool_not_int),  uda::config::ConfigError );
    REQUIRE_THROWS_AS( config.set("test.float", "not a float"),  uda::config::ConfigError );
    REQUIRE_THROWS_AS( config.set("test.boolean", "not a bool"),  uda::config::ConfigError );
}


TEST_CASE( "can parse arrays of structures", "[config-arraystruct]")
{
    uda::config::Config config {};
    std::string file_path = "test_files/host_list.toml";
    REQUIRE_NOTHROW( config.load(file_path) );
}


TEST_CASE( "can retreive data from arrays of structures", "[config-arraystruct]")
{
    uda::config::Config config {};
    std::string file_path = "test_files/host_list.toml";
    config.load(file_path);

    auto host_list = config.get_array("host_list");
    REQUIRE( host_list.size() == 5 ); 

    std::unordered_map<std::string, uda::config::Option> expected_result = 
    {
        {"host_name", {"host_name", "localhost"}},
        {"host_alias", {"host_alias", "localhost"}},
        {"port", {"port", 56565}},
    };
    auto localhost = host_list[0];
    REQUIRE( localhost == expected_result );
}
