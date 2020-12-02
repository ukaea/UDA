#define CATCH_CONFIG_MAIN
#include "catch.hpp"

#include <c++/UDA.hpp>

TEST_CASE( "Test get anu_neutrons signal", "[MAST]" )
{
#ifdef FATCLIENT
#  include "setup.inc"
#endif

    uda::Client client;

    const uda::Result& result = client.get("anu_neutrons", "29957");

    REQUIRE( result.errorCode() == 0 );
    REQUIRE( result.errorMessage().empty() );

    uda::Data* data = result.data();

    REQUIRE( data != nullptr );
    REQUIRE( !data->isNull() );
    REQUIRE( data->type().name() == typeid(float).name() );

    auto arr = dynamic_cast<uda::Array*>(data);

    REQUIRE( arr != nullptr );

    REQUIRE( arr->size() == 51694 );
    REQUIRE( arr->dims().size() == 1 );
    REQUIRE( arr->dims()[0].type().name() == typeid(float).name() );
    REQUIRE( arr->dims()[0].size() == 51694 );

    auto ddata = arr->dims()[0].as<float>();

    REQUIRE( ddata[100] == Approx(0.15301f) );
    REQUIRE( ddata[101] == Approx(0.15302f) );
    REQUIRE( ddata[102] == Approx(0.15303f) );
    REQUIRE( ddata[103] == Approx(0.15304f) );
    REQUIRE( ddata[104] == Approx(0.15305f) );

    REQUIRE( arr->type().name() == typeid(float).name() );

    auto vec = arr->as<float>();
    float* farr = vec.data();

    REQUIRE( farr != nullptr );
    REQUIRE( farr[100] == Approx(2.0580316e+11) );
    REQUIRE( farr[101] == Approx(2.1741017e+11) );
    REQUIRE( farr[102] == Approx(2.2955118e+11) );
    REQUIRE( farr[103] == Approx(2.4211738e+11) );
    REQUIRE( farr[104] == Approx(2.5536466e+11) );
}
