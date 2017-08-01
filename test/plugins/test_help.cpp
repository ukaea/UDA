#define CATCH_CONFIG_MAIN
#include "catch.hpp"

#include <c++/UDA.hpp>

TEST_CASE( "Test HELP::help() function", "[HELP][plugins]" ) {

#ifdef FATCLIENT
#  include "setupEnvironment.inc"
#endif

    uda::Client client;

    const uda::Result& result = client.get("HELP::help()", "");

    REQUIRE( result.errorCode() == 0 );
    REQUIRE( result.errorMessage() == "" );

    uda::Data* data = result.data();

    REQUIRE( data != NULL );
    REQUIRE( !data->isNull() );
    REQUIRE( data->type().name() == typeid(char*).name() );

    uda::String* str = dynamic_cast<uda::String*>(data);

    REQUIRE( str != NULL );

    std::string expected =
            "\nHelp\tList of HELP plugin functions:\n"
            "\n"
            "services()\tReturns a list of available services with descriptions\n"
            "ping()\t\tReturn the Local Server Time in seconds and microseonds\n"
            "servertime()\tReturn the Local Server Time in seconds and microseonds\n\n";

    REQUIRE( str->str() == expected );
}

TEST_CASE( "Test HELP::services() function", "[HELP][plugins]" ) {

#ifdef FATCLIENT
#  include "setupEnvironment.inc"
#endif

    uda::Client client;

    const uda::Result& result = client.get("HELP::services()", "");

    REQUIRE( result.errorCode() == 0 );
    REQUIRE( result.errorMessage() == "" );

    uda::Data* data = result.data();

    REQUIRE( data != NULL );
    REQUIRE( !data->isNull() );
    REQUIRE( data->type().name() == typeid(char*).name() );

    uda::String* str = dynamic_cast<uda::String*>(data);

    REQUIRE( str != NULL );

    std::string expected = "\nTotal number of registered plugins available";

    REQUIRE( str->str().size() > expected.size() );
    REQUIRE( str->str().substr(0, expected.size()) == expected );
}