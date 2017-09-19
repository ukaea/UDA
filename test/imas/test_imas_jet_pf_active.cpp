#define CATCH_CONFIG_MAIN
#include "catch.hpp"

#include <c++/UDA.hpp>

#define QUOTE_(X) #X
#define QUOTE(X) QUOTE_(X)
#define SHOT_NUM "84600"

#define MAPPINGS_DIR "/Users/jhollocombe/Projects/uda/source/plugins/exp2imas/mappings"

TEST_CASE( "Test pf_active coil count", "[IMAS][JET][PF_ACTIVE]" )
{
#ifdef FATCLIENT
#  include "setup.inc"
#endif

    setenv("UDA_EXP2IMAS_MAPPING_FILE_DIRECTORY", MAPPINGS_DIR, 1);

    uda::Client client;

    const uda::Result& result = client.get("imas::get(idx=0, group='pf_active', variable='coil/Shape_of', expName='JET', type=int, rank=0, shot=" SHOT_NUM ", )", "");

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
    REQUIRE( val->as<int>() == 16 );
}

TEST_CASE( "Test pf_active coil name", "[IMAS][JET][PF_ACTIVE]" )
{
#ifdef FATCLIENT
#  include "setup.inc"
#endif

    setenv("UDA_EXP2IMAS_MAPPING_FILE_DIRECTORY", MAPPINGS_DIR, 1);

    uda::Client client;

    const uda::Result& result = client.get("imas::get(idx=0, group='pf_active', variable='coil/1/name', expName='JET', type=string, rank=0, shot=" SHOT_NUM ", )", "");

    REQUIRE( result.errorCode() == 0 );
    REQUIRE( result.errorMessage().empty() );

    uda::Data* data = result.data();

    REQUIRE( data != nullptr );
    REQUIRE( !data->isNull() );
    REQUIRE( data->type().name() == typeid(char*).name() );

    auto val = dynamic_cast<uda::String*>(data);

    REQUIRE( val != nullptr );
    REQUIRE( !val->isNull() );

    REQUIRE( val->str() == "P1Eu_P1El" );
}

TEST_CASE( "Test pf_active coil identifier", "[IMAS][JET][PF_ACTIVE]" )
{
#ifdef FATCLIENT
#  include "setup.inc"
#endif

    setenv("UDA_EXP2IMAS_MAPPING_FILE_DIRECTORY", MAPPINGS_DIR, 1);

    uda::Client client;

    const uda::Result& result = client.get("imas::get(idx=0, group='pf_active', variable='coil/1/identifier', expName='JET', type=int, rank=0, shot=" SHOT_NUM ", )", "");

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

TEST_CASE( "Test pf_active coil element count", "[IMAS][JET][PF_ACTIVE]" )
{
#ifdef FATCLIENT
#  include "setup.inc"
#endif

    setenv("UDA_EXP2IMAS_MAPPING_FILE_DIRECTORY", MAPPINGS_DIR, 1);

    uda::Client client;

    const uda::Result& result = client.get("imas::get(idx=0, group='pf_active', variable='coil/1/element/Shape_of', expName='JET', type=int, rank=0, shot=" SHOT_NUM ", )", "");

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
    REQUIRE( val->as<int>() == 4 );
}

TEST_CASE( "Test pf_active coil element name", "[IMAS][JET][PF_ACTIVE]" )
{
#ifdef FATCLIENT
#  include "setup.inc"
#endif

    setenv("UDA_EXP2IMAS_MAPPING_FILE_DIRECTORY", MAPPINGS_DIR, 1);

    uda::Client client;

    const uda::Result& result = client.get("imas::get(idx=0, group='pf_active', variable='coil/1/element/1/name', expName='JET', type=string, rank=0, shot=" SHOT_NUM ", )", "");

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

TEST_CASE( "Test pf_active coil element identifier", "[IMAS][JET][PF_ACTIVE]" )
{
#ifdef FATCLIENT
#  include "setup.inc"
#endif

    setenv("UDA_EXP2IMAS_MAPPING_FILE_DIRECTORY", MAPPINGS_DIR, 1);

    uda::Client client;

    const uda::Result& result = client.get("imas::get(idx=0, group='pf_active', variable='coil/1/element/1/identifier', expName='JET', type=int, rank=0, shot=" SHOT_NUM ", )", "");

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

TEST_CASE( "Test pf_active coil element number of turns", "[IMAS][JET][PF_ACTIVE]" )
{
#ifdef FATCLIENT
#  include "setup.inc"
#endif

    setenv("UDA_EXP2IMAS_MAPPING_FILE_DIRECTORY", MAPPINGS_DIR, 1);

    uda::Client client;

    const uda::Result& result = client.get("imas::get(idx=0, group='pf_active', variable='coil/1/element/1/turns_with_sign', expName='JET', type=double, rank=0, shot=" SHOT_NUM ", )", "");

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
    REQUIRE( val->as<double>() == Approx(71.0) );
}

TEST_CASE( "Test pf_active coil element geometry type", "[IMAS][JET][PF_ACTIVE]" )
{
#ifdef FATCLIENT
#  include "setup.inc"
#endif

    setenv("UDA_EXP2IMAS_MAPPING_FILE_DIRECTORY", MAPPINGS_DIR, 1);

    uda::Client client;

    const uda::Result& result = client.get("imas::get(idx=0, group='pf_active', variable='coil/1/element/1/geometry/geometry_type', expName='JET', type=int, rank=0, shot=" SHOT_NUM ", )", "");

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

TEST_CASE( "Test pf_active coil element geometry r", "[IMAS][JET][PF_ACTIVE]" )
{
#ifdef FATCLIENT
#  include "setup.inc"
#endif

    setenv("UDA_EXP2IMAS_MAPPING_FILE_DIRECTORY", MAPPINGS_DIR, 1);

    uda::Client client;

    const uda::Result& result = client.get("imas::get(idx=0, group='pf_active', variable='coil/1/element/1/geometry/outline/r', expName='JET', type=double, rank=0, shot=" SHOT_NUM ", )", "");

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
    REQUIRE( val->as<double>() == Approx(0.897) );
}

TEST_CASE( "Test pf_active coil element geometry z", "[IMAS][JET][PF_ACTIVE]" )
{
#ifdef FATCLIENT
#  include "setup.inc"
#endif

    setenv("UDA_EXP2IMAS_MAPPING_FILE_DIRECTORY", MAPPINGS_DIR, 1);

    uda::Client client;

    const uda::Result& result = client.get("imas::get(idx=0, group='pf_active', variable='coil/1/element/1/geometry/outline/z', expName='JET', type=double, rank=0, shot=" SHOT_NUM ", )", "");

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
    REQUIRE( val->as<double>() == Approx(2.461) );
}

TEST_CASE( "Test pf_active coil force count", "[IMAS][JET][PF_ACTIVE]" )
{
#ifdef FATCLIENT
#  include "setup.inc"
#endif

    setenv("UDA_EXP2IMAS_MAPPING_FILE_DIRECTORY", MAPPINGS_DIR, 1);

    uda::Client client;

    const uda::Result& result = client.get("imas::get(idx=0, group='pf_active', variable='vertical_force/Shape_of', expName='JET', type=int, rank=0, shot=" SHOT_NUM ", )", "");

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

TEST_CASE( "Test pf_active coil circuit count", "[IMAS][JET][PF_ACTIVE]" )
{
#ifdef FATCLIENT
#  include "setup.inc"
#endif

    setenv("UDA_EXP2IMAS_MAPPING_FILE_DIRECTORY", MAPPINGS_DIR, 1);

    uda::Client client;

    const uda::Result& result = client.get("imas::get(idx=0, group='pf_active', variable='circuit/Shape_of', expName='JET', type=int, rank=0, shot=" SHOT_NUM ", )", "");

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

TEST_CASE( "Test pf_active coil supply count", "[IMAS][JET][PF_ACTIVE]" )
{
#ifdef FATCLIENT
#  include "setup.inc"
#endif

    setenv("UDA_EXP2IMAS_MAPPING_FILE_DIRECTORY", MAPPINGS_DIR, 1);

    uda::Client client;

    const uda::Result& result = client.get("imas::get(idx=0, group='pf_active', variable='supply/Shape_of', expName='JET', type=int, rank=0, shot=" SHOT_NUM ", )", "");

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