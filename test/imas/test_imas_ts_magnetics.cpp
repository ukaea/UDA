#define CATCH_CONFIG_MAIN
#include <catch2/catch_all.hpp>
#include "test_helpers.h"

#include <c++/UDA.hpp>

#define QUOTE_(X) #X
#define QUOTE(X) QUOTE_(X)
#define SHOT_NUM "43970"


TEST_CASE( "Test flux_loop count", "[IMAS][TS][FLUX_LOOP]" )
{
#ifdef FATCLIENT
#  include "setup.inc"
#endif

    uda::Client client;

   const uda::Result& result = client.get("TORE::read(element='magnetics/flux_loop/Shape_of', indices='', experiment='TORE', dtype=3, shot=" SHOT_NUM ", IDS_version='')", "");

    REQUIRE( result.errorCode() == 0 );
    REQUIRE( result.errorMessage().empty() );

    uda::Data* data = result.data();

    REQUIRE( data != nullptr );
    REQUIRE( !data->isNull() );
    REQUIRE( data->type().name() == typeid(int).name() );

    auto val = dynamic_cast<uda::Scalar*>(data);

    REQUIRE( val != nullptr );
    REQUIRE( !val->isNull() );

    REQUIRE( val->type().name() == typeid(int).name() );
    REQUIRE( val->as<int>() == 6 );
}


TEST_CASE( "Test flux data field", "[IMAS][TS][FLUX]" )
{
#ifdef FATCLIENT
#  include "setup.inc"
#endif

    uda::Client client;

    const uda::Result& result = client.get("TORE::read(element='magnetics/flux_loop/#/flux/data', indices='1', experiment='TORE', dtype=7, shot=" SHOT_NUM ", IDS_version='')", "");

    REQUIRE( result.errorCode() == 0 );
    REQUIRE( result.errorMessage().empty() );

    uda::Data* data = result.data();

    REQUIRE( data != nullptr );
    REQUIRE( !data->isNull() );

    REQUIRE( data->type().name() == typeid(float).name() );

    auto arr = dynamic_cast<uda::Array*>(data);

    REQUIRE( arr != nullptr );
    REQUIRE( !arr->isNull() );

    std::vector<float> expected = { -0.0f, 0.00504f, -0.0f, -0.00504f, 0.00504f };

    REQUIRE( arr->type().name() == typeid(float).name() );

    auto vals = arr->as<float>();
    vals.resize(5);


    REQUIRE( vals == ApproxVector(expected) );
}


