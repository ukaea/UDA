#define CATCH_CONFIG_MAIN
#include <catch2/catch_all.hpp>

#include <c++/UDA.hpp>

using Catch::Approx;

TEST_CASE( "Test UDA::help() function", "[UDA][plugins]" )
{
#include "setup.inc"

    uda::Client client;

    const uda::Result& result = client.get("UDA::help()", "");

    REQUIRE( result.errorCode() == 0 );
    REQUIRE( result.errorMessage().empty() );

    uda::Data* data = result.data();

    REQUIRE( data != nullptr );
    REQUIRE( !data->isNull() );
    REQUIRE( data->type().name() == typeid(char*).name() );

    auto str = dynamic_cast<uda::String*>(data);

    REQUIRE( str != nullptr );

    std::string expected = "\nUDA: Add Functions Names, Syntax, and Descriptions\n\n";

    REQUIRE( str->str() == expected );
}

TEST_CASE( "Test UDA::get() function", "[UDA][plugins]" )
{
#include "setup.inc"

    uda::Client client;

    const uda::Result& result = client.get("UDA::get(host=idam3.mast.ccfe.ac.uk, port=56565, signal=\"amb_fl/cc01\", source=\"18299\")", "");

    REQUIRE( result.errorCode() == 0 );
    REQUIRE( result.errorMessage().empty() );

    uda::Data* data = result.data();

    REQUIRE( data != nullptr );
    REQUIRE( !data->isNull() );
    REQUIRE( data->type().name() == typeid(float).name() );

    uda::Array* arr = dynamic_cast<uda::Array*>(data);

    REQUIRE( arr != nullptr );

    REQUIRE( arr->size() == 7500 );
    REQUIRE( arr->type().name() == typeid(float).name() );

    std::vector<uda::Dim> dims = arr->dims();
    REQUIRE( dims.size() == 1 );

    uda::Dim dim1 = dims[0];
    REQUIRE( dim1.size() == 7500 );

    std::vector<float> dim = dim1.as<float>();

    REQUIRE( dim[0] == Approx(-0.15) );
    REQUIRE( dim[1] == Approx(-0.1498) );
    REQUIRE( dim[2] == Approx(-0.1496) );
    REQUIRE( dim[7498] == Approx(1.3496) );
    REQUIRE( dim[7499] == Approx(1.3498) );

    std::vector<float> vec = arr->as<float>();
    REQUIRE( vec.size() == 7500 );

    REQUIRE( vec[0] == Approx(-0.00022) );
    REQUIRE( vec[1] == Approx(0.00002) );
    REQUIRE( vec[2] == Approx(-0.00022) );
    REQUIRE( vec[7498] == Approx(0.00011) );
    REQUIRE( vec[7499] == Approx(0.00011) );
}