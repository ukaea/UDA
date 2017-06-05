#define CATCH_CONFIG_MAIN
#include "catch.hpp"

#include <c++/UDA.hpp>

TEST_CASE( "Test EFITMAGXML::help() function", "[EFITMAGXML][plugins]" )
{
#ifdef FATCLIENT
#  include "setupEnvironment.inc"
#endif

    uda::Client client;

    const uda::Result& result = client.get("EFITMAGXML::help()", "");

    REQUIRE( result.errorCode() == 0 );
    REQUIRE( result.error() == "" );

    uda::Data* data = result.data();

    REQUIRE( data != NULL );
    REQUIRE( !data->isNull() );
    REQUIRE( data->type().name() == typeid(char*).name() );

    uda::String* str = dynamic_cast<uda::String*>(data);

    REQUIRE( str != NULL );

    std::string expected = "\nefitmagxml: Add Functions Names, Syntax, and Descriptions\n\n";

    REQUIRE( str->str() == expected );
}
