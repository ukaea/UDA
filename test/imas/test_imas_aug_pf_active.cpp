#define CATCH_CONFIG_MAIN
#include "catch.hpp"

#include <c++/UDA.hpp>

#define QUOTE_(X) #X
#define QUOTE(X) QUOTE_(X)
#define SHOT_NUM "33173"

TEST_CASE( "Test pf_active coil count", "[IMAS][AUG][PF_ACTIVE]" )
{
#ifdef FATCLIENT
#  include "setup.inc"
#endif

    uda::Client client;

    const uda::Result& result = client.get("imas::get(idx=0, group='pf_active', variable='coil/Shape_of', expName='AUG', type=int, rank=0, shot=" SHOT_NUM ", )", "");

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
    REQUIRE( val->as<int>() == 14 );
}

TEST_CASE( "Test pf_active coil name", "[IMAS][AUG][PF_ACTIVE]" )
{
#ifdef FATCLIENT
#  include "setup.inc"
#endif

    uda::Client client;

    const uda::Result& result = client.get("imas::get(idx=0, group='pf_active', variable='coil/1/name', expName='AUG', type=string, rank=0, shot=" SHOT_NUM ", )", "");

    REQUIRE( result.errorCode() == 0 );
    REQUIRE( result.errorMessage().empty() );

    uda::Data* data = result.data();

    REQUIRE( data != nullptr );
    REQUIRE( !data->isNull() );
    REQUIRE( data->type().name() == typeid(char*).name() );

    auto val = dynamic_cast<uda::String*>(data);

    REQUIRE( val != nullptr );
    REQUIRE( !val->isNull() );

    REQUIRE( val->str() == "V1o" );
}

TEST_CASE( "Test pf_active coil identifier", "[IMAS][AUG][PF_ACTIVE]" )
{
#ifdef FATCLIENT
#  include "setup.inc"
#endif

    uda::Client client;

    const uda::Result& result = client.get("imas::get(idx=0, group='pf_active', variable='coil/1/identifier', expName='AUG', type=int, rank=0, shot=" SHOT_NUM ", )", "");

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

TEST_CASE( "Test pf_active coil element count", "[IMAS][AUG][PF_ACTIVE]" )
{
#ifdef FATCLIENT
#  include "setup.inc"
#endif

    uda::Client client;

    const uda::Result& result = client.get("imas::get(idx=0, group='pf_active', variable='coil/1/element/Shape_of', expName='AUG', type=int, rank=0, shot=" SHOT_NUM ", )", "");

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
    REQUIRE( val->as<int>() == 3 );
}

TEST_CASE( "Test pf_active coil element name", "[IMAS][AUG][PF_ACTIVE]" )
{
#ifdef FATCLIENT
#  include "setup.inc"
#endif

    uda::Client client;

    const uda::Result& result = client.get("imas::get(idx=0, group='pf_active', variable='coil/1/element/1/name', expName='AUG', type=string, rank=0, shot=" SHOT_NUM ", )", "");

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

TEST_CASE( "Test pf_active coil element identifier", "[IMAS][AUG][PF_ACTIVE]" )
{
#ifdef FATCLIENT
#  include "setup.inc"
#endif

    uda::Client client;

    const uda::Result& result = client.get("imas::get(idx=0, group='pf_active', variable='coil/1/element/1/identifier', expName='AUG', type=int, rank=0, shot=" SHOT_NUM ", )", "");

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

TEST_CASE( "Test pf_active coil element number of turns", "[IMAS][AUG][PF_ACTIVE]" )
{
#ifdef FATCLIENT
#  include "setup.inc"
#endif

    uda::Client client;

    const uda::Result& result = client.get("imas::get(idx=0, group='pf_active', variable='coil/1/element/1/turns_with_sign', expName='AUG', type=double, rank=0, shot=" SHOT_NUM ", )", "");

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
    REQUIRE( val->as<double>() == Approx(35.0) );
}

TEST_CASE( "Test pf_active coil element geometry type", "[IMAS][AUG][PF_ACTIVE]" )
{
#ifdef FATCLIENT
#  include "setup.inc"
#endif

    uda::Client client;

    const uda::Result& result = client.get("imas::get(idx=0, group='pf_active', variable='coil/1/element/1/geometry/geometry_type', expName='AUG', type=int, rank=0, shot=" SHOT_NUM ", )", "");

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

TEST_CASE( "Test pf_active coil element geometry r", "[IMAS][AUG][PF_ACTIVE]" )
{
#ifdef FATCLIENT
#  include "setup.inc"
#endif

    uda::Client client;

    const uda::Result& result = client.get("imas::get(idx=0, group='pf_active', variable='coil/1/element/1/geometry/rectangle/r', expName='AUG', type=double, rank=0, shot=" SHOT_NUM ", )", "");

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
    REQUIRE( val->as<double>() == Approx(1.3836) );
}

TEST_CASE( "Test pf_active coil element geometry z", "[IMAS][AUG][PF_ACTIVE]" )
{
#ifdef FATCLIENT
#  include "setup.inc"
#endif

    uda::Client client;

    const uda::Result& result = client.get("imas::get(idx=0, group='pf_active', variable='coil/1/element/1/geometry/rectangle/z', expName='AUG', type=double, rank=0, shot=" SHOT_NUM ", )", "");

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
    REQUIRE( val->as<double>() == Approx(2.3582) );
}

TEST_CASE( "Test pf_active coil element geometry width", "[IMAS][AUG][PF_ACTIVE]" )
{
#ifdef FATCLIENT
#  include "setup.inc"
#endif

    uda::Client client;

    {
        const uda::Result& result = client.get(
                "imas::get(idx=0, group='pf_active', variable='coil/1/element/1/geometry/rectangle/width', expName='AUG', type=double, rank=0, shot=" SHOT_NUM ", )",
                "");

        REQUIRE( result.errorCode() == 0 );
        REQUIRE( result.errorMessage().empty()) ;

        uda::Data* data = result.data();

        REQUIRE( data != nullptr );
        REQUIRE( !data->isNull() );
        REQUIRE( data->type().name() == typeid(double).name() );

        auto val = dynamic_cast<uda::Scalar*>(data);

        REQUIRE( val != nullptr );
        REQUIRE( !val->isNull() );

        REQUIRE( val->type().name() == typeid(double).name() );
        REQUIRE( val->as<double>() == Approx(0.2182) );
    }

    {
        const uda::Result& result = client.get(
                "imas::get(idx=0, group='pf_active', variable='coil/9/element/1/geometry/rectangle/width', expName='AUG', type=double, rank=0, shot=" SHOT_NUM ", )",
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
        REQUIRE( val->as<double>() == Approx(0.075) );
    }
}

TEST_CASE( "Test pf_active coil element geometry height", "[IMAS][AUG][PF_ACTIVE]" )
{
#ifdef FATCLIENT
#  include "setup.inc"
#endif

    uda::Client client;

    {
        const uda::Result& result = client.get(
                "imas::get(idx=0, group='pf_active', variable='coil/1/element/1/geometry/rectangle/height', expName='AUG', type=double, rank=0, shot=" SHOT_NUM ", )",
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
        REQUIRE( val->as<double>() == Approx(0.3463) );
    }

    {
        const uda::Result& result = client.get(
                "imas::get(idx=0, group='pf_active', variable='coil/9/element/1/geometry/rectangle/height', expName='AUG', type=double, rank=0, shot=" SHOT_NUM ", )",
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
        REQUIRE( val->as<double>() == Approx(0.032) );
    }
}

TEST_CASE( "Test pf_active coil current", "[IMAS][AUG][PF_ACTIVE]" )
{
#ifdef FATCLIENT
#  include "setup.inc"
#endif

    uda::Client client;

    {
        const uda::Result& result = client.get(
                "imas::get(idx=0, group='pf_active', variable='coil/1/current/data', expName='AUG', type=double, rank=1, shot=" SHOT_NUM ", )",
                "");

        REQUIRE( result.errorCode() == 0 );
        REQUIRE( result.errorMessage().empty() );

        uda::Data* data = result.data();

        REQUIRE( data != nullptr );
        REQUIRE( !data->isNull() );
        REQUIRE( data->type().name() == typeid(double).name() );

        auto arr = dynamic_cast<uda::Array*>(data);

        REQUIRE( arr != nullptr );
        REQUIRE( !arr->isNull() );

        REQUIRE( arr->size() == 225000 );
        REQUIRE( arr->type().name() == typeid(double).name() );
        REQUIRE( arr->as<double>()[0] == Approx(0.0) );
    }

    {
        const uda::Result& result = client.get(
                "imas::get(idx=0, group='pf_active', variable='coil/14/current/data', expName='AUG', type=double, rank=1, shot=" SHOT_NUM ", )",
                "");

        REQUIRE( result.errorCode() == 0 );
        REQUIRE( result.errorMessage().empty() );

        uda::Data* data = result.data();

        REQUIRE( data != nullptr );
        REQUIRE( !data->isNull() );
        REQUIRE( data->type().name() == typeid(double).name() );

        auto arr = dynamic_cast<uda::Array*>(data);

        REQUIRE( arr != nullptr );
        REQUIRE( !arr->isNull() );

        REQUIRE( arr->size() == 225000 );
        REQUIRE( arr->type().name() == typeid(double).name() );
        REQUIRE( arr->as<double>()[0] == Approx(95.3396987915) );
    }
}

TEST_CASE( "Test pf_active coil current error upper", "[IMAS][AUG][PF_ACTIVE]" )
{
#ifdef FATCLIENT
#  include "setup.inc"
#endif

    uda::Client client;

    REQUIRE_THROWS( client.get("imas::get(idx=0, group='pf_active', variable='coil/1/current/data_error_upper', expName='AUG', type=double, rank=1, shot=" SHOT_NUM ", )", "") );
}

TEST_CASE( "Test pf_active coil current error lower", "[IMAS][AUG][PF_ACTIVE]" )
{
#ifdef FATCLIENT
#  include "setup.inc"
#endif

    uda::Client client;

    REQUIRE_THROWS( client.get("imas::get(idx=0, group='pf_active', variable='coil/1/current/data_error_lower', expName='AUG', type=double, rank=1, shot=" SHOT_NUM ", )", "") );
}

TEST_CASE( "Test pf_active coil current time", "[IMAS][AUG][PF_ACTIVE]" )
{
#ifdef FATCLIENT
#  include "setup.inc"
#endif

    uda::Client client;

    const uda::Result& result = client.get("imas::get(idx=0, group='pf_active', variable='coil/1/current/time', expName='AUG', type=double, rank=1, shot=" SHOT_NUM ", )", "");

    REQUIRE( result.errorCode() == 0 );
    REQUIRE( result.errorMessage().empty() );

    uda::Data* data = result.data();

    REQUIRE( data != nullptr );
    REQUIRE( !data->isNull() );
    REQUIRE( data->type().name() == typeid(double).name() );

    auto arr = dynamic_cast<uda::Array*>(data);

    REQUIRE( arr != nullptr );
    REQUIRE( !arr->isNull() );

    REQUIRE( arr->size() == 225000 );
    REQUIRE( arr->type().name() == typeid(double).name() );
    REQUIRE( arr->as<double>()[0] == Approx(-10.15133) );
}

TEST_CASE( "Test pf_active coil force count", "[IMAS][AUG][PF_ACTIVE]" )
{
#ifdef FATCLIENT
#  include "setup.inc"
#endif

    uda::Client client;

    const uda::Result& result = client.get("imas::get(idx=0, group='pf_active', variable='vertical_force/Shape_of', expName='AUG', type=int, rank=0, shot=" SHOT_NUM ", )", "");

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

TEST_CASE( "Test pf_active coil circuit count", "[IMAS][AUG][PF_ACTIVE]" )
{
#ifdef FATCLIENT
#  include "setup.inc"
#endif

    uda::Client client;

    const uda::Result& result = client.get("imas::get(idx=0, group='pf_active', variable='circuit/Shape_of', expName='AUG', type=int, rank=0, shot=" SHOT_NUM ", )", "");

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

TEST_CASE( "Test pf_active coil supply count", "[IMAS][AUG][PF_ACTIVE]" )
{
#ifdef FATCLIENT
#  include "setup.inc"
#endif

    uda::Client client;

    const uda::Result& result = client.get("imas::get(idx=0, group='pf_active', variable='supply/Shape_of', expName='AUG', type=int, rank=0, shot=" SHOT_NUM ", )", "");

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
