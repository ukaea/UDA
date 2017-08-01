#define CATCH_CONFIG_MAIN
#include "catch.hpp"

#include <c++/UDA.hpp>

TEST_CASE( "Test NEWCDF4::help() function", "[NEWCDF4][plugins]" ) {

#ifdef FATCLIENT
#  include "setupEnvironment.inc"
#endif

    uda::Client client;

    const uda::Result& result = client.get("NEWCDF4::help()", "");

    REQUIRE( result.errorCode() == 0 );
    REQUIRE( result.errorMessage() == "" );

    uda::Data* data = result.data();

    REQUIRE( data != NULL );
    REQUIRE( !data->isNull() );
    REQUIRE( data->type().name() == typeid(char*).name() );

    uda::String* str = dynamic_cast<uda::String*>(data);

    REQUIRE( str != NULL );

    std::string expected = "\nnewCDF4: get - Read data from a netCDF4 file\n\n";

    REQUIRE( str->str() == expected );
}
