#define CATCH_CONFIG_MAIN
#include "catch.hpp"
#include "test_helpers.h"

#include <c++/UDA.hpp>

#define QUOTE_(X) #X
#define QUOTE(X) QUOTE_(X)
#define SHOT_NUM "84600"

TEST_CASE( "Test thomson scattering channel count", "[IMAS][JET][TSCAT]" )
{
#ifdef FATCLIENT
#  include "setup.inc"
#endif

    uda::Client client;

    const uda::Result& result = client.get("imas::get(idx=0, group='thomson_scattering', variable='channel/Shape_of', expName='JET', type=int, rank=0, shot=" SHOT_NUM ", )", "");

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
    REQUIRE( val->as<int>() == 63 );
}

TEST_CASE( "Test thomson scattering channel position r", "[IMAS][JET][TSCAT]" )
{
#ifdef FATCLIENT
#  include "setup.inc"
#endif

    uda::Client client;

    {
        const uda::Result& result = client.get("imas::get(idx=0, group='thomson_scattering', variable='channel/1/position/r', expName='JET', type=double, rank=0, shot=" SHOT_NUM ", )", "");

        REQUIRE( result.errorCode() == 0 );
        REQUIRE( result.errorMessage().empty() );

        uda::Data* data = result.data();

        REQUIRE( data != nullptr );
        REQUIRE( !data->isNull() );
        REQUIRE( data->type().name() == typeid(double).name() );

        auto arr = dynamic_cast<uda::Vector*>(data);

        REQUIRE( arr != nullptr );
        REQUIRE( !arr->isNull() );

        REQUIRE( arr->type().name() == typeid(double).name() );
        REQUIRE( arr->size() == 701 );
        REQUIRE( arr->as<double>()[0] == Approx(2.97450018) );
    }

    {
        const uda::Result& result = client.get("imas::get(idx=0, group='thomson_scattering', variable='channel/2/position/r', expName='JET', type=double, rank=0, shot=" SHOT_NUM ", )", "");

        REQUIRE( result.errorCode() == 0 );
        REQUIRE( result.errorMessage().empty() );

        uda::Data* data = result.data();

        REQUIRE( data != nullptr );
        REQUIRE( !data->isNull() );
        REQUIRE( data->type().name() == typeid(double).name() );

        auto arr = dynamic_cast<uda::Vector*>(data);

        REQUIRE( arr != nullptr );
        REQUIRE( !arr->isNull() );

        REQUIRE( arr->type().name() == typeid(double).name() );
        REQUIRE( arr->size() == 701 );
        REQUIRE( arr->as<double>()[0] == Approx(2.99075007) );
    }
}

TEST_CASE( "Test thomson scattering channel position z", "[IMAS][JET][TSCAT]" )
{
#ifdef FATCLIENT
#  include "setup.inc"
#endif

    uda::Client client;

    {
        // required to set some state in the MDSplus server!
        client.get("imas::get(idx=0, group='thomson_scattering', variable='channel/1/position/r', expName='JET', type=double, rank=0, shot=" SHOT_NUM ", )", "");

        const uda::Result& result = client.get("imas::get(idx=0, group='thomson_scattering', variable='channel/1/position/z', expName='JET', type=double, rank=0, shot=" SHOT_NUM ", )", "");

        REQUIRE( result.errorCode() == 0 );
        REQUIRE( result.errorMessage().empty() );

        uda::Data* data = result.data();

        REQUIRE( data != nullptr );
        REQUIRE( !data->isNull() );
        REQUIRE( data->type().name() == typeid(double).name() );

        auto arr = dynamic_cast<uda::Vector*>(data);

        REQUIRE( arr != nullptr );
        REQUIRE( !arr->isNull() );

        REQUIRE( arr->type().name() == typeid(double).name() );
        REQUIRE( arr->size() == 701 );
        REQUIRE( arr->as<double>()[0] == Approx(0.061622526) );
    }

    {
        // required to set some state in the MDSplus server!
        client.get("imas::get(idx=0, group='thomson_scattering', variable='channel/2/position/r', expName='JET', type=double, rank=0, shot=" SHOT_NUM ", )", "");

        const uda::Result& result = client.get("imas::get(idx=0, group='thomson_scattering', variable='channel/2/position/z', expName='JET', type=double, rank=0, shot=" SHOT_NUM ", )", "");

        REQUIRE( result.errorCode() == 0 );
        REQUIRE( result.errorMessage().empty() );

        uda::Data* data = result.data();

        REQUIRE( data != nullptr );
        REQUIRE( !data->isNull() );
        REQUIRE( data->type().name() == typeid(double).name() );

        auto arr = dynamic_cast<uda::Vector*>(data);

        REQUIRE( arr != nullptr );
        REQUIRE( !arr->isNull() );

        REQUIRE( arr->type().name() == typeid(double).name() );
        REQUIRE( arr->size() == 701 );
        REQUIRE( arr->as<double>()[0] == Approx(0.062425219) );
    }
}

TEST_CASE( "Test thomson scattering channel position phi", "[IMAS][JET][TSCAT]" )
{
#ifdef FATCLIENT
#  include "setup.inc"
#endif

    uda::Client client;

    {
        const uda::Result& result = client.get("imas::get(idx=0, group='thomson_scattering', variable='channel/1/position/phi', expName='JET', type=double, rank=0, shot=" SHOT_NUM ", )", "");

        REQUIRE( result.errorCode() == 0 );
        REQUIRE( result.errorMessage().empty() );

        uda::Data* data = result.data();

        REQUIRE( data != nullptr );
        REQUIRE( !data->isNull() );
        REQUIRE( data->type().name() == typeid(double).name() );

        auto arr = dynamic_cast<uda::Vector*>(data);

        REQUIRE( arr != nullptr );
        REQUIRE( !arr->isNull() );

        REQUIRE( arr->type().name() == typeid(double).name() );
        REQUIRE( arr->size() == 701 );
        REQUIRE( arr->as<double>()[0] == Approx(0.0) );
    }

    {
        const uda::Result& result = client.get("imas::get(idx=0, group='thomson_scattering', variable='channel/2/position/phi', expName='JET', type=double, rank=0, shot=" SHOT_NUM ", )", "");

        REQUIRE( result.errorCode() == 0 );
        REQUIRE( result.errorMessage().empty() );

        uda::Data* data = result.data();

        REQUIRE( data != nullptr );
        REQUIRE( !data->isNull() );
        REQUIRE( data->type().name() == typeid(double).name() );

        auto arr = dynamic_cast<uda::Vector*>(data);

        REQUIRE( arr != nullptr );
        REQUIRE( !arr->isNull() );

        REQUIRE( arr->type().name() == typeid(double).name() );
        REQUIRE( arr->size() == 701 );
        REQUIRE( arr->as<double>()[0] == Approx(0.0) );
    }
}

TEST_CASE( "Test thomson scattering channel t_e data", "[IMAS][JET][TSCAT]" )
{
#ifdef FATCLIENT
#  include "setup.inc"
#endif

    uda::Client client;

    const uda::Result& result = client.get("imas::get(idx=0, group='thomson_scattering', variable='channel/1/t_e/data', expName='JET', type=double, rank=1, shot=" SHOT_NUM ", )", "");

    REQUIRE( result.errorCode() == 0 );
    REQUIRE( result.errorMessage().empty() );

    uda::Data* data = result.data();

    REQUIRE( data != nullptr );
    REQUIRE( !data->isNull() );
    REQUIRE( data->type().name() == typeid(double).name() );

    auto arr = dynamic_cast<uda::Array*>(data);

    REQUIRE( arr != nullptr );
    REQUIRE( !arr->isNull() );

    std::vector<double> expected = { 0.0, 0.0, 0.0, 0.0, 12.56754398, 491.86361694, 300.91174316 };

    REQUIRE( arr->type().name() == typeid(double).name() );

    auto vals = arr->as<double>();
    vals.resize(7);

    REQUIRE( vals == ApproxVector(expected) );
}

TEST_CASE( "Test thomson scattering channel t_e time", "[IMAS][JET][TSCAT]" )
{
#ifdef FATCLIENT
#  include "setup.inc"
#endif

    uda::Client client;

    const uda::Result& result = client.get("imas::get(idx=0, group='thomson_scattering', variable='channel/1/t_e/time', expName='JET', type=double, rank=1, shot=" SHOT_NUM ", )", "");

    REQUIRE( result.errorCode() == 0 );
    REQUIRE( result.errorMessage().empty() );

    uda::Data* data = result.data();

    REQUIRE( data != nullptr );
    REQUIRE( !data->isNull() );
    REQUIRE( data->type().name() == typeid(double).name() );

    auto arr = dynamic_cast<uda::Array*>(data);

    REQUIRE( arr != nullptr );
    REQUIRE( !arr->isNull() );

    std::vector<double> expected = { 40.016254425, 40.066280365, 40.1163063049, 40.1663322449, 40.2163581848 };

    REQUIRE( arr->type().name() == typeid(double).name() );

    auto vals = arr->as<double>();
    vals.resize(5);

    REQUIRE( vals == ApproxVector(expected) );
}

TEST_CASE( "Test thomson scattering channel n_e data", "[IMAS][JET][TSCAT]" )
{
#ifdef FATCLIENT
#  include "setup.inc"
#endif

    uda::Client client;

    const uda::Result& result = client.get("imas::get(idx=0, group='thomson_scattering', variable='channel/1/n_e/data', expName='JET', type=double, rank=1, shot=" SHOT_NUM ", )", "");

    REQUIRE( result.errorCode() == 0 );
    REQUIRE( result.errorMessage().empty() );

    uda::Data* data = result.data();

    REQUIRE( data != nullptr );
    REQUIRE( !data->isNull() );
    REQUIRE( data->type().name() == typeid(double).name() );

    auto arr = dynamic_cast<uda::Array*>(data);

    REQUIRE( arr != nullptr );
    REQUIRE( !arr->isNull() );

    std::vector<double> expected = { 0.0, 0.0, 0.0, 0.0, 15769506927354052608.0, 3531266860311904256.0, 4806206118055378944.0 };

    REQUIRE( arr->type().name() == typeid(double).name() );

    auto vals = arr->as<double>();
    vals.resize(7);

    REQUIRE( vals == ApproxVector(expected) );
}

TEST_CASE( "Test thomson scattering channel n_e time", "[IMAS][JET][TSCAT]" )
{
#ifdef FATCLIENT
#  include "setup.inc"
#endif

    uda::Client client;

    const uda::Result& result = client.get("imas::get(idx=0, group='thomson_scattering', variable='channel/1/n_e/time', expName='JET', type=double, rank=1, shot=" SHOT_NUM ", )", "");

    REQUIRE( result.errorCode() == 0 );
    REQUIRE( result.errorMessage().empty() );

    uda::Data* data = result.data();

    REQUIRE( data != nullptr );
    REQUIRE( !data->isNull() );
    REQUIRE( data->type().name() == typeid(double).name() );

    auto arr = dynamic_cast<uda::Array*>(data);

    REQUIRE( arr != nullptr );
    REQUIRE( !arr->isNull() );

    std::vector<double> expected = { 40.016254425, 40.066280365, 40.1163063049, 40.1663322449, 40.2163581848 };

    REQUIRE( arr->type().name() == typeid(double).name() );

    auto vals = arr->as<double>();
    vals.resize(5);

    REQUIRE( vals == ApproxVector(expected) );
}
