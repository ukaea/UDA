#define CATCH_CONFIG_MAIN
#include "catch.hpp"
#include "test_helpers.h"

#include <c++/UDA.hpp>

#define QUOTE_(X) #X
#define QUOTE(X) QUOTE_(X)
#define SHOT_NUM "84600"

TEST_CASE( "Test tf coil count", "[IMAS][JET][TF]" )
{
#ifdef FATCLIENT
#  include "setup.inc"
#endif

    uda::Client client;

    const uda::Result& result = client.get("imas::get(idx=0, group='tf', variable='coil/Shape_of', expName='JET', type=int, rank=0, shot=" SHOT_NUM ", )", "");

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
    REQUIRE( val->as<int>() == 32 );
}

TEST_CASE( "Test tf coil conductor count", "[IMAS][JET][TF]" )
{
#ifdef FATCLIENT
#  include "setup.inc"
#endif

    uda::Client client;

    const uda::Result& result = client.get("imas::get(idx=0, group='tf', variable='coil/1/conductor/Shape_of', expName='JET', type=int, rank=0, shot=" SHOT_NUM ", )", "");

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
    REQUIRE( val->as<int>() == 0 );
}

TEST_CASE( "Test tf coil turns", "[IMAS][JET][TF]" )
{
#ifdef FATCLIENT
#  include "setup.inc"
#endif

    uda::Client client;

    const uda::Result& result = client.get("imas::get(idx=0, group='tf', variable='coil/1/turns', expName='JET', type=int, rank=0, shot=" SHOT_NUM ", )", "");

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
    REQUIRE( val->as<int>() == 24 );
}

TEST_CASE( "Test tf vacuum field", "[IMAS][JET][TF]" )
{
#ifdef FATCLIENT
#  include "setup.inc"
#endif

    uda::Client client;

    const uda::Result& result = client.get("imas::get(idx=0, group='tf', variable='b_field_tor_vacuum_r/data', expName='JET', type=double, rank=1, shot=" SHOT_NUM ", )", "");

    REQUIRE( result.errorCode() == 0 );
    REQUIRE( result.errorMessage().empty() );

    uda::Data* data = result.data();

    REQUIRE( data != nullptr );
    REQUIRE( !data->isNull() );
    REQUIRE( data->type().name() == typeid(double).name() );

    auto arr = dynamic_cast<uda::Array*>(data);

    REQUIRE( arr != nullptr );
    REQUIRE( !arr->isNull() );

    std::vector<double> expected = { -0.0006830642, -0.0003118947, -0.0000411735, -0.0001789987, -0.000205223 };

    REQUIRE( arr->type().name() == typeid(double).name() );

    auto vals = arr->as<double>();
    vals.resize(5);

    REQUIRE( vals == ApproxVector(expected) );
}

TEST_CASE( "Test tf vacuum field error upper", "[IMAS][JET][TF]" )
{
#ifdef FATCLIENT
#  include "setup.inc"
#endif

    uda::Client client;

    const uda::Result& result = client.get("imas::get(idx=0, group='tf', variable='b_field_tor_vacuum_r/data_error_upper', expName='JET', type=double, rank=1, shot=" SHOT_NUM ", )", "");

    REQUIRE( result.errorCode() == 0 );
    REQUIRE( result.errorMessage().empty() );

    uda::Data* data = result.data();

    REQUIRE( data != nullptr );
    REQUIRE( !data->isNull() );
    REQUIRE( data->type().name() == typeid(double).name() );

    auto arr = dynamic_cast<uda::Array*>(data);

    REQUIRE( arr != nullptr );
    REQUIRE( !arr->isNull() );

    std::vector<double> expected = { 0.0093169361, 0.0096881054, 0.009958826, 0.0098210014, 0.0097947773 };

    REQUIRE( arr->type().name() == typeid(double).name() );

    auto vals = arr->as<double>();
    vals.resize(5);

    REQUIRE( vals == ApproxVector(expected) );
}

TEST_CASE( "Test tf vacuum field error lower", "[IMAS][JET][TF]" )
{
#ifdef FATCLIENT
#  include "setup.inc"
#endif

    uda::Client client;

    const uda::Result& result = client.get("imas::get(idx=0, group='tf', variable='b_field_tor_vacuum_r/data_error_lower', expName='JET', type=double, rank=1, shot=" SHOT_NUM ", )", "");

    REQUIRE( result.errorCode() == 0 );
    REQUIRE( result.errorMessage().empty() );

    uda::Data* data = result.data();

    REQUIRE( data != nullptr );
    REQUIRE( !data->isNull() );
    REQUIRE( data->type().name() == typeid(double).name() );

    auto arr = dynamic_cast<uda::Array*>(data);

    REQUIRE( arr != nullptr );
    REQUIRE( !arr->isNull() );

    std::vector<double> expected = { -0.0106830643, -0.0103118951, -0.0100411735, -0.010178999, -0.0102052232 };

    REQUIRE( arr->type().name() == typeid(double).name() );

    auto vals = arr->as<double>();
    vals.resize(5);

    REQUIRE( vals == ApproxVector(expected) );
}

TEST_CASE( "Test tf vacuum field time", "[IMAS][JET][TF]" )
{
#ifdef FATCLIENT
#  include "setup.inc"
#endif

    uda::Client client;

    const uda::Result& result = client.get("imas::get(idx=0, group='tf', variable='b_field_tor_vacuum_r/time', expName='JET', type=double, rank=1, shot=" SHOT_NUM ", )", "");

    REQUIRE( result.errorCode() == 0 );
    REQUIRE( result.errorMessage().empty() );

    uda::Data* data = result.data();

    REQUIRE( data != nullptr );
    REQUIRE( !data->isNull() );
    REQUIRE( data->type().name() == typeid(double).name() );

    auto arr = dynamic_cast<uda::Array*>(data);

    REQUIRE( arr != nullptr );
    REQUIRE( !arr->isNull() );

    std::vector<double> expected = { 25.0161991119, 25.0314006805, 25.0466003418, 25.0618000031, 25.0769996643 };

    REQUIRE( arr->type().name() == typeid(double).name() );

    auto vals = arr->as<double>();
    vals.resize(5);

    REQUIRE( vals == ApproxVector(expected) );
}
