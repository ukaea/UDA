#define CATCH_CONFIG_MAIN
#include "catch.hpp"

#include <client/udaPutAPI.h>
#include <c++/UDA.hpp>

TEST_CASE( "Test PUTDATA::help() function", "[PUTDATA][plugins]" ) {

#ifdef FATCLIENT
#  include "setupEnvironment.inc"
#endif

    uda::Client client;

    const uda::Result& result = client.get("PUTDATA::help()", "");

    REQUIRE( result.errorCode() == 0 );
    REQUIRE( result.errorMessage().empty() );

    uda::Data* data = result.data();

    REQUIRE( data != NULL );
    REQUIRE( !data->isNull() );
    REQUIRE( data->type().name() == typeid(char*).name() );

    auto str = dynamic_cast<uda::String*>(data);

    REQUIRE( str != NULL );

    std::string expected = "\nputdataPlugin: Add Functions Names, Syntax, and Descriptions\n\n";

    REQUIRE( str->str() == expected );
}

TEST_CASE( "Test cpp PUTDATA functionality", "[PUTDATA][plugins]" ) {

#ifdef FATCLIENT
#  include "setupEnvironment.inc"
#endif

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