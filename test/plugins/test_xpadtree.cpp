#define CATCH_CONFIG_MAIN
#include "catch.hpp"

#include <c++/UDA.hpp>

TEST_CASE( "Test XPADTREE::help() function", "[XPADTREE][plugins]" )
{
#include "setup.inc"

    uda::Client client;

    const uda::Result& result = client.get("XPADTREE::help()", "");

    REQUIRE( result.errorCode() == 0 );
    REQUIRE( result.errorMessage().empty() );

    uda::Data* data = result.data();

    REQUIRE( data != NULL );
    REQUIRE( !data->isNull() );
    REQUIRE( data->type().name() == typeid(char*).name() );

    auto str = dynamic_cast<uda::String*>(data);

    REQUIRE( str != NULL );

    std::string expected = "xpadtree::gettree() - Read in tree.\n";

    REQUIRE( str->str() == expected );
}
