#define CATCH_CONFIG_MAIN
#include "catch.hpp"

#include <client/udaPutAPI.h>
#include <c++/UDA.hpp>

TEST_CASE( "Test PUTDATA::help() function", "[PUTDATA][plugins]" )
{
#include "setup.inc"

    uda::Client client;

    const uda::Result& result = client.get("PUTDATA::help()", "");

    REQUIRE( result.errorCode() == 0 );
    REQUIRE( result.error() == "" );

    uda::Data* data = result.data();

    REQUIRE( data != NULL );
    REQUIRE( !data->isNull() );
    REQUIRE( data->type().name() == typeid(char*).name() );

    uda::String* str = dynamic_cast<uda::String*>(data);

    REQUIRE( str != NULL );

    std::string expected = "\nputdataPlugin: Add Functions Names, Syntax, and Descriptions\n\n";

    REQUIRE( str->str() == expected );
}

TEST_CASE( "Test cpp PUTDATA functionality", "[PUTDATA][plugins]" )
{
#include "setup.inc"

    std::stringstream ss;
    ss << "putdata::open(/create"
            << ", filename='test.nc'"
            << ", conventions='Fusion-1.0'"
            << ", class='Analysed'"
            << ", title='Test #1'"
            << ", shot=123456"
            << ", pass=789"
            << ", comment='Comment for test file'"
            << ", code='test_putdata.cpp'"
            << ", version=1"
            << ")";

    idamPutAPI("PUTDATA::open(create, filename=, conventions=, class=, title=, shot=, pass=)", NULL);
}