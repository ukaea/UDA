#define CATCH_CONFIG_MAIN
#include "catch.hpp"

#include <c++/UDA.hpp>

TEST_CASE( "Test HELP::help() function", "[HELP][plugins]" )
{
#include "setup.inc"

    uda::Client client;

    const uda::Result& result = client.get("HELP::help()", "");

    REQUIRE( result.errorCode() == 0 );
    REQUIRE( result.errorMessage().empty() );

    uda::Data* data = result.data();

    REQUIRE( data != nullptr );
    REQUIRE( !data->isNull() );
    REQUIRE( data->type().name() == typeid(char*).name() );

    auto str = dynamic_cast<uda::String*>(data);

    REQUIRE( str != nullptr );

    std::string expected_header = "\nHelp\tList of HELP plugin functions:\n\n";

    const std::string help = str->str();

    // the help text varies with build options (e.g. OIDC authentication), so check the stable parts
    REQUIRE( help.find(expected_header) == 0 );
    for (const char* line : {"services()", "ping()", "servertime()", "servermetadata()"}) {
        REQUIRE( help.find(line) != std::string::npos );
    }
}

TEST_CASE( "Test HELP::services() function", "[HELP][plugins]" )
{
#include "setup.inc"

    uda::Client client;

    const uda::Result& result = client.get("HELP::services()", "");

    REQUIRE( result.errorCode() == 0 );
    REQUIRE( result.errorMessage().empty() );

    uda::Data* data = result.data();

    REQUIRE( data != nullptr );
    REQUIRE( !data->isNull() );
    REQUIRE( data->type().name() == typeid(char*).name() );

    auto str = dynamic_cast<uda::String*>(data);

    REQUIRE( str != nullptr );

    std::string expected = "\nTotal number of registered plugins available";

    REQUIRE( str->str().size() > expected.size() );
    REQUIRE( str->str().substr(0, expected.size()) == expected );
}