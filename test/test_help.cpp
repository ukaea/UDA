#define CATCH_CONFIG_MAIN
#include "catch.hpp"

#include <c++/Idam.hpp>

TEST_CASE( "Test help::help() function", "[help]" ) {

#ifdef FATCLIENT
#  include "setupEnvironment.inc"
#endif

    Idam::Client client;

    const Idam::Result& result = client.get("help::help()", "");

    REQUIRE( result.errorCode() == 0 );
    REQUIRE( result.error() == "" );

    Idam::Data* data = result.data();

    REQUIRE( data != NULL );
    REQUIRE( !data->isNull() );
    REQUIRE( data->type().name() == typeid(char*).name() );

    Idam::String* str = dynamic_cast<Idam::String*>(data);

    REQUIRE( str != NULL );

    std::string expected =
            "\nHelp\tList of HELP plugin functions:\n"
            "\n"
            "services()\tReturns a list of available services with descriptions\n"
            "ping()\t\tReturn the Local Server Time in seconds and microseonds\n"
            "servertime()\tReturn the Local Server Time in seconds and microseonds\n\n";

    REQUIRE( str->str() == expected );
}