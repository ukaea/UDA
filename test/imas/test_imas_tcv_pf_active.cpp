#define CATCH_CONFIG_MAIN
#include "catch.hpp"
#include "test_helpers.h"

#include <c++/UDA.hpp>

#define QUOTE_(X) #X
#define QUOTE(X) QUOTE_(X)
#define SHOT_NUM "51262"

TEST_CASE( "Test pf_active coil count", "[IMAS][TCV][PF_ACTIVE]" )
{
#ifdef FATCLIENT
#  include "setup.inc"
#endif

    uda::Client client;

    const uda::Result& result = client.get("imas::get(idx=0, group='pf_active', variable='coil/Shape_of', expName='TCV', type=int, rank=0, shot=" SHOT_NUM ", )", "");

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

TEST_CASE( "Test pf_active coil name", "[IMAS][TCV][PF_ACTIVE]" )
{
#ifdef FATCLIENT
#  include "setup.inc"
#endif

    uda::Client client;

    const uda::Result& result = client.get("imas::get(idx=0, group='pf_active', variable='coil/1/name', expName='TCV', type=string, rank=0, shot=" SHOT_NUM ", )", "");

    REQUIRE( result.errorCode() == 0 );
    REQUIRE( result.errorMessage().empty() );

    uda::Data* data = result.data();

    REQUIRE( data != nullptr );
    REQUIRE( !data->isNull() );
    REQUIRE( data->type().name() == typeid(char*).name() );

    auto val = dynamic_cast<uda::String*>(data);

    REQUIRE( val != nullptr );
    REQUIRE( !val->isNull() );

    REQUIRE( val->str() == "A_001" );
}

TEST_CASE( "Test pf_active coil identifier", "[IMAS][TCV][PF_ACTIVE]" )
{
#ifdef FATCLIENT
#  include "setup.inc"
#endif

    uda::Client client;

    const uda::Result& result = client.get("imas::get(idx=0, group='pf_active', variable='coil/1/identifier', expName='TCV', type=int, rank=0, shot=" SHOT_NUM ", )", "");

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
    REQUIRE( val->as<int>() == 1 );
}

TEST_CASE( "Test pf_active coil element count", "[IMAS][TCV][PF_ACTIVE]" )
{
#ifdef FATCLIENT
#  include "setup.inc"
#endif

    uda::Client client;

    const uda::Result& result = client.get("imas::get(idx=0, group='pf_active', variable='coil/1/element/Shape_of', expName='TCV', type=int, rank=0, shot=" SHOT_NUM ", )", "");

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
    REQUIRE( val->as<int>() == 1 );
}

TEST_CASE( "Test pf_active coil element name", "[IMAS][TCV][PF_ACTIVE]" )
{
#ifdef FATCLIENT
#  include "setup.inc"
#endif

    uda::Client client;

    const uda::Result& result = client.get("imas::get(idx=0, group='pf_active', variable='coil/1/element/1/name', expName='TCV', type=string, rank=0, shot=" SHOT_NUM ", )", "");

    REQUIRE( result.errorCode() == 0 );
    REQUIRE( result.errorMessage().empty() );

    uda::Data* data = result.data();

    REQUIRE( data != nullptr );
    REQUIRE( !data->isNull() );
    REQUIRE( data->type().name() == typeid(char*).name() );

    auto val = dynamic_cast<uda::String*>(data);

    REQUIRE( val != nullptr );
    REQUIRE( !val->isNull() );

    REQUIRE( val->str() == "1" );
}

TEST_CASE( "Test pf_active coil element identifier", "[IMAS][TCV][PF_ACTIVE]" )
{
#ifdef FATCLIENT
#  include "setup.inc"
#endif

    uda::Client client;

    const uda::Result& result = client.get("imas::get(idx=0, group='pf_active', variable='coil/1/element/1/identifier', expName='TCV', type=int, rank=0, shot=" SHOT_NUM ", )", "");

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
    REQUIRE( val->as<int>() == 1 );
}

TEST_CASE( "Test pf_active coil element number of turns", "[IMAS][TCV][PF_ACTIVE]" )
{
#ifdef FATCLIENT
#  include "setup.inc"
#endif

    uda::Client client;

    const uda::Result& result = client.get("imas::get(idx=0, group='pf_active', variable='coil/1/element/1/turns_with_sign', expName='TCV', type=double, rank=0, shot=" SHOT_NUM ", )", "");

    REQUIRE( result.errorCode() == 0 );
    REQUIRE( result.errorMessage().empty() );

    uda::Data* data = result.data();

    REQUIRE( data != nullptr );
    REQUIRE( !data->isNull() );
    REQUIRE( data->type().name() == typeid(double).name() );

    auto val = dynamic_cast<uda::Scalar*>(data);

    REQUIRE( val != nullptr );
    REQUIRE( !val->isNull() );

    REQUIRE( val->type().name() == typeid(double).name() );
    REQUIRE( val->as<double>() == Approx(143.0) );
}

TEST_CASE( "Test pf_active coil element geometry type", "[IMAS][TCV][PF_ACTIVE]" )
{
#ifdef FATCLIENT
#  include "setup.inc"
#endif

    uda::Client client;

    const uda::Result& result = client.get("imas::get(idx=0, group='pf_active', variable='coil/1/element/1/geometry/geometry_type', expName='TCV', type=int, rank=0, shot=" SHOT_NUM ", )", "");

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
    REQUIRE( val->as<int>() == 2 );
}

TEST_CASE( "Test pf_active coil element geometry r", "[IMAS][TCV][PF_ACTIVE]" )
{
#ifdef FATCLIENT
#  include "setup.inc"
#endif

    uda::Client client;

    const uda::Result& result = client.get("imas::get(idx=0, group='pf_active', variable='coil/1/element/1/geometry/rectangle/r', expName='TCV', type=double, rank=0, shot=" SHOT_NUM ", )", "");

    REQUIRE( result.errorCode() == 0 );
    REQUIRE( result.errorMessage().empty() );

    uda::Data* data = result.data();

    REQUIRE( data != nullptr );
    REQUIRE( !data->isNull() );
    REQUIRE( data->type().name() == typeid(double).name() );

    auto val = dynamic_cast<uda::Scalar*>(data);

    REQUIRE( val != nullptr );
    REQUIRE( !val->isNull() );

    REQUIRE( val->type().name() == typeid(double).name() );
    REQUIRE( val->as<double>() == Approx(0.4225) );
}

TEST_CASE( "Test pf_active coil element geometry z", "[IMAS][TCV][PF_ACTIVE]" )
{
#ifdef FATCLIENT
#  include "setup.inc"
#endif

    uda::Client client;

    const uda::Result& result = client.get("imas::get(idx=0, group='pf_active', variable='coil/1/element/1/geometry/rectangle/z', expName='TCV', type=double, rank=0, shot=" SHOT_NUM ", )", "");

    REQUIRE( result.errorCode() == 0 );
    REQUIRE( result.errorMessage().empty() );

    uda::Data* data = result.data();

    REQUIRE( data != nullptr );
    REQUIRE( !data->isNull() );
    REQUIRE( data->type().name() == typeid(double).name() );

    auto val = dynamic_cast<uda::Scalar*>(data);

    REQUIRE( val != nullptr );
    REQUIRE( !val->isNull() );

    REQUIRE( val->type().name() == typeid(double).name() );
    REQUIRE( val->as<double>() == Approx(0.0) );
}

TEST_CASE( "Test pf_active coil element geometry width", "[IMAS][TCV][PF_ACTIVE]" )
{
#ifdef FATCLIENT
#  include "setup.inc"
#endif

    uda::Client client;

    {
        const uda::Result& result = client.get(
                "imas::get(idx=0, group='pf_active', variable='coil/1/element/1/geometry/rectangle/width', expName='TCV', type=double, rank=0, shot=" SHOT_NUM ", )",
                "");

        REQUIRE(result.errorCode() == 0);
        REQUIRE(result.errorMessage().empty());

        uda::Data* data = result.data();

        REQUIRE(data != nullptr);
        REQUIRE(!data->isNull());
        REQUIRE(data->type().name() == typeid(double).name());

        auto val = dynamic_cast<uda::Scalar*>(data);

        REQUIRE(val != nullptr);
        REQUIRE(!val->isNull());

        REQUIRE( val->type().name() == typeid(double).name() );
        REQUIRE( val->as<double>() == Approx(0.063) );
    }

    {
        const uda::Result& result = client.get(
                "imas::get(idx=0, group='pf_active', variable='coil/9/element/1/geometry/rectangle/width', expName='TCV', type=double, rank=0, shot=" SHOT_NUM ", )",
                "");

        REQUIRE( result.errorCode() == 0 );
        REQUIRE( result.errorMessage().empty() );

        uda::Data* data = result.data();

        REQUIRE( data != nullptr );
        REQUIRE( !data->isNull() );
        REQUIRE( data->type().name() == typeid(double).name() );

        auto val = dynamic_cast<uda::Scalar*>(data);

        REQUIRE( val != nullptr );
        REQUIRE( !val->isNull() );

        REQUIRE( val->type().name() == typeid(double).name() );
        REQUIRE( val->as<double>() == Approx(0.051) );
    }
}

TEST_CASE( "Test pf_active coil element geometry height", "[IMAS][TCV][PF_ACTIVE]" )
{
#ifdef FATCLIENT
#  include "setup.inc"
#endif

    uda::Client client;

    {
        const uda::Result& result = client.get(
                "imas::get(idx=0, group='pf_active', variable='coil/1/element/1/geometry/rectangle/height', expName='TCV', type=double, rank=0, shot=" SHOT_NUM ", )",
                "");

        REQUIRE(result.errorCode() == 0);
        REQUIRE(result.errorMessage().empty());

        uda::Data* data = result.data();

        REQUIRE(data != nullptr);
        REQUIRE(!data->isNull());
        REQUIRE(data->type().name() == typeid(double).name());

        auto val = dynamic_cast<uda::Scalar*>(data);

        REQUIRE(val != nullptr);
        REQUIRE(!val->isNull());

        REQUIRE(val->type().name() == typeid(double).name());
        REQUIRE(val->as<double>() == Approx(1.584));
    }

    {
        const uda::Result& result = client.get(
                "imas::get(idx=0, group='pf_active', variable='coil/9/element/1/geometry/rectangle/height', expName='TCV', type=double, rank=0, shot=" SHOT_NUM ", )",
                "");

        REQUIRE(result.errorCode() == 0);
        REQUIRE(result.errorMessage().empty());

        uda::Data* data = result.data();

        REQUIRE(data != nullptr);
        REQUIRE(!data->isNull());
        REQUIRE(data->type().name() == typeid(double).name());

        auto val = dynamic_cast<uda::Scalar*>(data);

        REQUIRE( val != nullptr );
        REQUIRE( !val->isNull() );

        REQUIRE( val->type().name() == typeid(double).name() );
        REQUIRE( val->as<double>() == Approx(0.18) );
    }
}

TEST_CASE( "Test pf_active coil current", "[IMAS][TCV][PF_ACTIVE]" )
{
#ifdef FATCLIENT
#  include "setup.inc"
#endif

    uda::Client client;

    const uda::Result& result = client.get("imas::get(idx=0, group='pf_active', variable='coil/1/current/data', expName='TCV', type=double, rank=1, shot=" SHOT_NUM ", )", "");

    REQUIRE( result.errorCode() == 0 );
    REQUIRE( result.errorMessage().empty() );

    uda::Data* data = result.data();

    REQUIRE( data != nullptr );
    REQUIRE( !data->isNull() );
    REQUIRE( data->type().name() == typeid(double).name() );

    auto arr = dynamic_cast<uda::Array*>(data);

    REQUIRE( arr != nullptr );
    REQUIRE( !arr->isNull() );

    REQUIRE( arr->size() == 49820 );
    REQUIRE( arr->type().name() == typeid(double).name() );
    REQUIRE( arr->as<double>()[0] == Approx(6.24609375) );
}

TEST_CASE( "Test pf_active coil current error upper", "[IMAS][TCV][PF_ACTIVE]" )
{
#ifdef FATCLIENT
#  include "setup.inc"
#endif

    uda::Client client;

    const uda::Result& result = client.get("imas::get(idx=0, group='pf_active', variable='coil/1/current/data_error_upper', expName='TCV', type=double, rank=1, shot=" SHOT_NUM ", )", "");

    REQUIRE( result.errorCode() == 0 );
    REQUIRE( result.errorMessage().empty() );

    uda::Data* data = result.data();

    REQUIRE( data != nullptr );
    REQUIRE( !data->isNull() );
    REQUIRE( data->type().name() == typeid(double).name() );

    auto arr = dynamic_cast<uda::Array*>(data);

    REQUIRE( arr != nullptr );
    REQUIRE( !arr->isNull() );

    REQUIRE( arr->size() == 49820 );
    REQUIRE( arr->type().name() == typeid(double).name() );
    REQUIRE( arr->as<double>()[0] == Approx(106.24609375) );
}

TEST_CASE( "Test pf_active coil current error lower", "[IMAS][TCV][PF_ACTIVE]" )
{
#ifdef FATCLIENT
#  include "setup.inc"
#endif

    uda::Client client;

    const uda::Result& result = client.get("imas::get(idx=0, group='pf_active', variable='coil/1/current/data_error_lower', expName='TCV', type=double, rank=1, shot=" SHOT_NUM ", )", "");

    REQUIRE( result.errorCode() == 0 );
    REQUIRE( result.errorMessage().empty() );

    uda::Data* data = result.data();

    REQUIRE( data != nullptr );
    REQUIRE( !data->isNull() );
    REQUIRE( data->type().name() == typeid(double).name() );

    auto arr = dynamic_cast<uda::Array*>(data);

    REQUIRE( arr != nullptr );
    REQUIRE( !arr->isNull() );

    REQUIRE( arr->size() == 49820 );
    REQUIRE( arr->type().name() == typeid(double).name() );
    REQUIRE( arr->as<double>()[0] == Approx(-93.75390625) );
}

TEST_CASE( "Test pf_active coil current time", "[IMAS][TCV][PF_ACTIVE]" )
{
#ifdef FATCLIENT
#  include "setup.inc"
#endif

    uda::Client client;

    const uda::Result& result = client.get("imas::get(idx=0, group='pf_active', variable='coil/1/current/time', expName='TCV', type=double, rank=1, shot=" SHOT_NUM ", )", "");

    REQUIRE( result.errorCode() == 0 );
    REQUIRE( result.errorMessage().empty() );

    uda::Data* data = result.data();

    REQUIRE( data != nullptr );
    REQUIRE( !data->isNull() );
    REQUIRE( data->type().name() == typeid(double).name() );

    auto arr = dynamic_cast<uda::Array*>(data);

    REQUIRE( arr != nullptr );
    REQUIRE( !arr->isNull() );

    REQUIRE( arr->size() == 49820 );
    REQUIRE( arr->type().name() == typeid(double).name() );
    REQUIRE( arr->as<double>()[0] == Approx(-1.3579999208) );
}

TEST_CASE( "Test pf_active coil force count", "[IMAS][TCV][PF_ACTIVE]" )
{
#ifdef FATCLIENT
#  include "setup.inc"
#endif

    uda::Client client;

    const uda::Result& result = client.get("imas::get(idx=0, group='pf_active', variable='vertical_force/Shape_of', expName='TCV', type=int, rank=0, shot=" SHOT_NUM ", )", "");

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

TEST_CASE( "Test pf_active circuit count", "[IMAS][TCV][PF_ACTIVE]" )
{
#ifdef FATCLIENT
#  include "setup.inc"
#endif

    uda::Client client;

    const uda::Result& result = client.get("imas::get(idx=0, group='pf_active', variable='circuit/Shape_of', expName='TCV', type=int, rank=0, shot=" SHOT_NUM ", )", "");

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
    REQUIRE( val->as<int>() == 20 );
}

TEST_CASE( "Test pf_active circuit name", "[IMAS][TCV][PF_ACTIVE]" )
{
#ifdef FATCLIENT
#  include "setup.inc"
#endif

    uda::Client client;

    const uda::Result& result = client.get("imas::get(idx=0, group='pf_active', variable='circuit/1/name', expName='TCV', type=string, rank=0, shot=" SHOT_NUM ", )", "");

    REQUIRE( result.errorCode() == 0 );
    REQUIRE( result.errorMessage().empty() );

    uda::Data* data = result.data();

    REQUIRE( data != nullptr );
    REQUIRE( !data->isNull() );
    REQUIRE( data->type().name() == typeid(char*).name() );

    auto val = dynamic_cast<uda::String*>(data);

    REQUIRE( val != nullptr );
    REQUIRE( !val->isNull() );

    REQUIRE( val->str() == "A_001" );
}

TEST_CASE( "Test pf_active circuit connections", "[IMAS][TCV][PF_ACTIVE]" )
{
#ifdef FATCLIENT
#  include "setup.inc"
#endif

    uda::Client client;

    const uda::Result& result = client.get("imas::get(idx=0, group='pf_active', variable='circuit/1/connections', expName='TCV', type=int, rank=2, shot=" SHOT_NUM ", )", "");

    REQUIRE( result.errorCode() == 0 );
    REQUIRE( result.errorMessage().empty() );

    uda::Data* data = result.data();

    REQUIRE( data != nullptr );
    REQUIRE( !data->isNull() );
    REQUIRE( data->type().name() == typeid(int).name() );

    auto arr = dynamic_cast<uda::Array*>(data);

    REQUIRE( arr != nullptr );
    REQUIRE( !arr->isNull() );

    std::vector<size_t> shape = { 2, 104 };

    REQUIRE( arr->shape() == shape );
    REQUIRE( arr->size() == 208 );
    REQUIRE( arr->type().name() == typeid(int).name() );

    std::vector<int> vals = arr->as<int>();
    vals.resize(5);

    std::vector<int> expected = { 0, 1, 0, 0, 0 };

    REQUIRE( vals == expected );
}

TEST_CASE( "Test pf_active coil supply count", "[IMAS][TCV][PF_ACTIVE]" )
{
#ifdef FATCLIENT
#  include "setup.inc"
#endif

    uda::Client client;

    const uda::Result& result = client.get("imas::get(idx=0, group='pf_active', variable='supply/Shape_of', expName='TCV', type=int, rank=0, shot=" SHOT_NUM ", )", "");

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
    REQUIRE( val->as<int>() == 20 );
}

TEST_CASE( "Test pf_active supply name", "[IMAS][TCV][PF_ACTIVE]" )
{
#ifdef FATCLIENT
#  include "setup.inc"
#endif

    uda::Client client;

    const uda::Result& result = client.get("imas::get(idx=0, group='pf_active', variable='supply/1/name', expName='TCV', type=string, rank=0, shot=" SHOT_NUM ", )", "");

    REQUIRE( result.errorCode() == 0 );
    REQUIRE( result.errorMessage().empty() );

    uda::Data* data = result.data();

    REQUIRE( data != nullptr );
    REQUIRE( !data->isNull() );
    REQUIRE( data->type().name() == typeid(char*).name() );

    auto val = dynamic_cast<uda::String*>(data);

    REQUIRE( val != nullptr );
    REQUIRE( !val->isNull() );

    REQUIRE( val->str() == "A_001" );
}

TEST_CASE( "Test pf_active supply voltage data", "[IMAS][TCV][PF_ACTIVE]" )
{
#ifdef FATCLIENT
#  include "setup.inc"
#endif

    uda::Client client;

    const uda::Result& result = client.get("imas::get(idx=0, group='pf_active', variable='supply/3/voltage/data', expName='TCV', type=double, rank=1, shot=" SHOT_NUM ", )", "");

    REQUIRE( result.errorCode() == 0 );
    REQUIRE( result.errorMessage().empty() );

    uda::Data* data = result.data();

    REQUIRE( data != nullptr );
    REQUIRE( !data->isNull() );
    REQUIRE( data->type().name() == typeid(double).name() );

    auto arr = dynamic_cast<uda::Array*>(data);

    REQUIRE( arr != nullptr );
    REQUIRE( !arr->isNull() );

    REQUIRE( arr->size() == 49820 );
    REQUIRE( arr->type().name() == typeid(double).name() );
    REQUIRE( arr->as<double>()[0] == Approx(-1.3579999208) );
}

TEST_CASE( "Test pf_active supply voltage time", "[IMAS][TCV][PF_ACTIVE]" )
{
#ifdef FATCLIENT
#  include "setup.inc"
#endif

    uda::Client client;

    const uda::Result& result = client.get("imas::get(idx=0, group='pf_active', variable='supply/3/voltage/time', expName='TCV', type=double, rank=1, shot=" SHOT_NUM ", )", "");

    REQUIRE( result.errorCode() == 0 );
    REQUIRE( result.errorMessage().empty() );

    uda::Data* data = result.data();

    REQUIRE( data != nullptr );
    REQUIRE( !data->isNull() );
    REQUIRE( data->type().name() == typeid(double).name() );

    auto arr = dynamic_cast<uda::Array*>(data);

    REQUIRE( arr != nullptr );
    REQUIRE( !arr->isNull() );

    REQUIRE( arr->size() == 49820 );
    REQUIRE( arr->type().name() == typeid(double).name() );

    std::vector<double> vals = arr->as<double>();
    vals.resize(5);

    std::vector<double> expected = { 0.0, 0.0, 0.0, 0.0, 0.0 };

    REQUIRE( vals == ApproxVector(expected) );
}
