#define CATCH_CONFIG_MAIN
#include "catch.hpp"

#include <c++/UDA.hpp>

TEST_CASE( "Test GEOM::help() function", "[GEOM][plugins]" )
{
#include "setup.inc"

    uda::Client client;

    const uda::Result& result = client.get("GEOM::help()", "");

    REQUIRE( result.errorCode() == 0 );
    REQUIRE( result.errorMessage().empty() );

    uda::Data* data = result.data();

    REQUIRE( data != NULL );
    REQUIRE( !data->isNull() );
    REQUIRE( data->type().name() == typeid(char*).name() );

    auto str = dynamic_cast<uda::String*>(data);

    REQUIRE( str != NULL );

    std::string expected = "\ngeometry: Retrieve geometry data from netcdf files, signal mapping data & "
            "filenames\n\n";

    REQUIRE( str->str() == expected );
}
