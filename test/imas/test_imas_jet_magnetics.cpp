#define CATCH_CONFIG_MAIN
#include "catch.hpp"

#include <c++/UDA.hpp>

#define QUOTE_(X) #X
#define QUOTE(X) QUOTE_(X)
#define SHOT_NUM "84600"

#define MAPPINGS_DIR "/Users/jhollocombe/Projects/uda/source/plugins/exp2imas/mappings"

/*
  ✓ bpol_probe/Shape_of
  ✓ bpol_probe/#/name
  ✓ bpol_probe/#/identifier
  ✓ bpol_probe/#/position/r
  ✓ bpol_probe/#/position/z
  ✓ bpol_probe/#/position/phi
  ✓ bpol_probe/#/poloidal_angle
  ✓ bpol_probe/#/toroidal_angle
  ✓ bpol_probe/#/area
  ✓ bpol_probe/#/length
  ✓ bpol_probe/#/turns
  ✓ bpol_probe/#/field
  ✓ bpol_probe/#/field/time
 */

/*
 * imas::get(expName='JET', idx=0, group='magnetics', variable='bpol_probe/Shape_of', type=int, rank=0, shot=84600, )
 */
TEST_CASE( "Test bpol_probe count", "[IMAS][JET][BPOL]" )
{
#ifdef FATCLIENT
#  include "setup.inc"
#endif

    setenv("UDA_EXP2IMAS_MAPPING_FILE_DIRECTORY", MAPPINGS_DIR, 1);

    uda::Client client;

    const uda::Result& result = client.get("imas::get(idx=0, group='magnetics', variable='bpol_probe/Shape_of', expName='JET', type=int, rank=0, shot=" SHOT_NUM ", )", "");

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
    REQUIRE( val->as<int>() == 71 );
}

/*
 * imas::get(expName='JET', idx=0, group='magnetics', variable='bpol_probe/#/name', type=string, rank=0, shot=84600, )
 */
TEST_CASE( "Test bpol_probe name", "[IMAS][JET][BPOL]" )
{
#ifdef FATCLIENT
#  include "setup.inc"
#endif

    setenv("UDA_EXP2IMAS_MAPPING_FILE_DIRECTORY", MAPPINGS_DIR, 1);

    uda::Client client;

    const uda::Result& result = client.get("imas::get(idx=0, group='magnetics', variable='bpol_probe/1/name', expName='JET', type=string, rank=0, shot=" SHOT_NUM ", )", "");

    REQUIRE( result.errorCode() == 0 );
    REQUIRE( result.errorMessage().empty() );

    uda::Data* data = result.data();

    REQUIRE( data != nullptr );
    REQUIRE( !data->isNull() );
    REQUIRE( data->type().name() == typeid(char*).name() );

    auto val = dynamic_cast<uda::String*>(data);

    REQUIRE( val != nullptr );
    REQUIRE( !val->isNull() );

    REQUIRE( val->str() == "C2-CX01" );
}

/*
 * imas::get(expName='JET', idx=0, group='magnetics', variable='bpol_probe/#/identifier', type=string, rank=0, shot=84600, )
 */
TEST_CASE( "Test bpol_probe identifier", "[IMAS][JET][BPOL]" )
{
#ifdef FATCLIENT
#  include "setup.inc"
#endif

    setenv("UDA_EXP2IMAS_MAPPING_FILE_DIRECTORY", MAPPINGS_DIR, 1);

    uda::Client client;

    const uda::Result& result = client.get("imas::get(idx=0, group='magnetics', variable='bpol_probe/1/identifier', expName='JET', type=int, rank=0, shot=" SHOT_NUM ", )", "");

    REQUIRE( result.errorCode() == 0 );
    REQUIRE( result.errorMessage().empty() );

    uda::Data* data = result.data();

    REQUIRE( data != nullptr );
    REQUIRE( !data->isNull() );
    REQUIRE( data->type().name() == typeid(int).name() );

    auto val = dynamic_cast<uda::Scalar*>(data);

    REQUIRE( val != nullptr );
    REQUIRE( !val->isNull() );

    REQUIRE( val->as<int>() == 1 );
}

/*
 * imas::get(expName='JET', idx=0, group='magnetics', variable='bpol_probe/#/toroidal_angle_error_index', type=int, rank=0, shot=84600, )
 * imas::get(expName='JET', idx=0, group='magnetics', variable='bpol_probe/#/toroidal_angle_error_lower', type=double, rank=0, shot=84600, )
 * imas::get(expName='JET', idx=0, group='magnetics', variable='bpol_probe/#/toroidal_angle_error_upper', type=double, rank=0, shot=84600, )
 * imas::get(expName='JET', idx=0, group='magnetics', variable='bpol_probe/#/toroidal_angle', type=double, rank=0, shot=84600, )
 */
TEST_CASE( "Test bpol_probe toroidal_angle", "[IMAS][JET][BPOL]" )
{
#ifdef FATCLIENT
#  include "setup.inc"
#endif

    setenv("UDA_EXP2IMAS_MAPPING_FILE_DIRECTORY", MAPPINGS_DIR, 1);

	uda::Client client;

	const uda::Result& result = client.get("imas::get(idx=0, group='magnetics', variable='bpol_probe/1/toroidal_angle', expName='JET', type=double, rank=0, shot=" SHOT_NUM ", )", "");

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

/*
 * imas::get(expName='JET', idx=0, group='magnetics', variable='bpol_probe/#/poloidal_angle_error_index', type=int, rank=0, shot=84600, )
 * imas::get(expName='JET', idx=0, group='magnetics', variable='bpol_probe/#/poloidal_angle_error_lower', type=double, rank=0, shot=84600, )
 * imas::get(expName='JET', idx=0, group='magnetics', variable='bpol_probe/#/poloidal_angle_error_upper', type=double, rank=0, shot=84600, )
 * imas::get(expName='JET', idx=0, group='magnetics', variable='bpol_probe/#/poloidal_angle', type=double, rank=0, shot=84600, )
 */
TEST_CASE( "Test bpol_probe poloidal_angle", "[IMAS][JET][BPOL]" )
{
#ifdef FATCLIENT
#  include "setup.inc"
#endif

	setenv("UDA_EXP2IMAS_MAPPING_FILE_DIRECTORY", MAPPINGS_DIR, 1);
	setenv("UDA_EXP2IMAS_MAPPING_FILE", MAPPINGS_DIR "/JET_Mapping.xml", 1);

    uda::Client client;

    double expected_vals[] = { -1.2933, -1.0559, -0.8203, -0.5847, -0.2304 };

    int index = 1;
    for (auto expected_val : expected_vals) {

        std::string signal = "imas::get(idx=0, group='magnetics', variable='bpol_probe/"
                             + std::to_string(index)
                             + "/poloidal_angle', expName='JET', type=double, rank=0, shot=" + SHOT_NUM + ", )";

        const uda::Result& result = client.get(signal, "");
        ++index;

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
        REQUIRE(val->as<double>() == Approx(expected_val));
    }
}

/*
 * imas::get(expName='JET', idx=0, group='magnetics', variable='bpol_probe/#/area_error_index', type=int, rank=0, shot=84600, )
 * imas::get(expName='JET', idx=0, group='magnetics', variable='bpol_probe/#/area_error_lower', type=double, rank=0, shot=84600, )
 * imas::get(expName='JET', idx=0, group='magnetics', variable='bpol_probe/#/area_error_upper', type=double, rank=0, shot=84600, )
 * imas::get(expName='JET', idx=0, group='magnetics', variable='bpol_probe/#/area', type=double, rank=0, shot=84600, )
 */
TEST_CASE( "Test bpol_probe area", "[IMAS][JET][BPOL]" )
{
#ifdef FATCLIENT
#  include "setup.inc"
#endif

    setenv("UDA_EXP2IMAS_MAPPING_FILE_DIRECTORY", MAPPINGS_DIR, 1);

    uda::Client client;

    const uda::Result& result = client.get("imas::get(idx=0, group='magnetics', variable='bpol_probe/1/area', expName='JET', type=double, rank=0, shot=" SHOT_NUM ", )", "");

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
    REQUIRE( val->as<double>() == Approx(0.001) );
}

/*
 * imas::get(expName='JET', idx=0, group='magnetics', variable='bpol_probe/#/position/r_error_index', type=int, rank=0, shot=84600, )
 * imas::get(expName='JET', idx=0, group='magnetics', variable='bpol_probe/#/position/r_error_lower', type=double, rank=0, shot=84600, )
 * imas::get(expName='JET', idx=0, group='magnetics', variable='bpol_probe/#/position/r_error_upper', type=double, rank=0, shot=84600, )
 * imas::get(expName='JET', idx=0, group='magnetics', variable='bpol_probe/#/position/r', type=double, rank=0, shot=84600, )
 */
TEST_CASE( "Test bpol_probe position r", "[IMAS][JET][BPOL]" )
{
#ifdef FATCLIENT
#  include "setup.inc"
#endif

    setenv("UDA_EXP2IMAS_MAPPING_FILE_DIRECTORY", MAPPINGS_DIR, 1);

    uda::Client client;

    const uda::Result& result = client.get("imas::get(idx=0, group='magnetics', variable='bpol_probe/1/position/r', expName='JET', type=double, rank=0, shot=" SHOT_NUM ", )", "");

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
    REQUIRE( val->as<double>() == Approx(4.2920) );
}

/*
 * imas::get(expName='JET', idx=0, group='magnetics', variable='bpol_probe/#/position/z_error_index', type=int, rank=0, shot=84600, )
 * imas::get(expName='JET', idx=0, group='magnetics', variable='bpol_probe/#/position/z_error_lower', type=double, rank=0, shot=84600, )
 * imas::get(expName='JET', idx=0, group='magnetics', variable='bpol_probe/#/position/z_error_upper', type=double, rank=0, shot=84600, )
 * imas::get(expName='JET', idx=0, group='magnetics', variable='bpol_probe/#/position/z', type=double, rank=0, shot=84600, )
 */
TEST_CASE( "Test bpol_probe position z", "[IMAS][JET][BPOL]" )
{
#ifdef FATCLIENT
#  include "setup.inc"
#endif

    setenv("UDA_EXP2IMAS_MAPPING_FILE_DIRECTORY", MAPPINGS_DIR, 1);

    uda::Client client;

    const uda::Result& result = client.get("imas::get(idx=0, group='magnetics', variable='bpol_probe/1/position/z', expName='JET', type=double, rank=0, shot=" SHOT_NUM ", )", "");

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
    REQUIRE( val->as<double>() == Approx(0.6040) );
}

/*
 * imas::get(expName='JET', idx=0, group='magnetics', variable='bpol_probe/#/position/phi_error_index', type=int, rank=0, shot=84600, )
 * imas::get(expName='JET', idx=0, group='magnetics', variable='bpol_probe/#/position/phi_error_lower', type=double, rank=0, shot=84600, )
 * imas::get(expName='JET', idx=0, group='magnetics', variable='bpol_probe/#/position/phi_error_upper', type=double, rank=0, shot=84600, )
 * imas::get(expName='JET', idx=0, group='magnetics', variable='bpol_probe/#/position/phi', type=double, rank=0, shot=84600, )
 */
TEST_CASE( "Test bpol_probe position phi", "[IMAS][JET][BPOL]" )
{
#ifdef FATCLIENT
#  include "setup.inc"
#endif

    setenv("UDA_EXP2IMAS_MAPPING_FILE_DIRECTORY", MAPPINGS_DIR, 1);

    uda::Client client;

    const uda::Result& result = client.get("imas::get(idx=0, group='magnetics', variable='bpol_probe/1/position/phi', expName='JET', type=double, rank=0, shot=" SHOT_NUM ", )", "");

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

/*
 * imas::get(expName='JET', idx=0, group='magnetics', variable='bpol_probe/#/length_error_index', type=int, rank=0, shot=84600, )
 * imas::get(expName='JET', idx=0, group='magnetics', variable='bpol_probe/#/length_error_lower', type=double, rank=0, shot=84600, )
 * imas::get(expName='JET', idx=0, group='magnetics', variable='bpol_probe/#/length_error_upper', type=double, rank=0, shot=84600, )
 * imas::get(expName='JET', idx=0, group='magnetics', variable='bpol_probe/#/length', type=double, rank=0, shot=84600, )
 */
TEST_CASE( "Test bpol_probe length", "[IMAS][JET][BPOL]" )
{
#ifdef FATCLIENT
#  include "setup.inc"
#endif

    setenv("UDA_EXP2IMAS_MAPPING_FILE_DIRECTORY", MAPPINGS_DIR, 1);

    uda::Client client;

    const uda::Result& result = client.get("imas::get(idx=0, group='magnetics', variable='bpol_probe/1/length', expName='JET', type=double, rank=0, shot=" SHOT_NUM ", )", "");

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
    REQUIRE( val->as<double>() == Approx(0.15) );
}

/*
 * imas::get(expName='JET', idx=0, group='magnetics', variable='bpol_probe/#/turns', type=int, rank=0, shot=84600, )
 */
TEST_CASE( "Test bpol_probe turns", "[IMAS][JET][BPOL]" )
{
#ifdef FATCLIENT
#  include "setup.inc"
#endif

    setenv("UDA_EXP2IMAS_MAPPING_FILE_DIRECTORY", MAPPINGS_DIR, 1);

    uda::Client client;

    const uda::Result& result = client.get("imas::get(idx=0, group='magnetics', variable='bpol_probe/1/turns', expName='JET', type=int, rank=0, shot=" SHOT_NUM ", )", "");

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

/*
 * imas::get(expName='JET', idx=0, group='magnetics', variable='bpol_probe/#/field/data_error_index', type=int, rank=0, shot=84600, )
 * imas::get(expName='JET', idx=0, group='magnetics', variable='bpol_probe/#/field/data_error_lower', type=double, rank=1, shot=84600, )
 * imas::get(expName='JET', idx=0, group='magnetics', variable='bpol_probe/#/field/data_error_upper', type=double, rank=1, shot=84600, )
 * imas::get(expName='JET', idx=0, group='magnetics', variable='bpol_probe/#/field/data', type=double, rank=1, shot=84600, )
 */
TEST_CASE( "Test bpol_probe field", "[IMAS][JET][BPOL]" )
{
#ifdef FATCLIENT
#  include "setup.inc"
#endif

    setenv("UDA_EXP2IMAS_MAPPING_FILE_DIRECTORY", MAPPINGS_DIR, 1);

    uda::Client client;

    const uda::Result& result = client.get("imas::get(idx=0, group='magnetics', variable='bpol_probe/1/field/data', expName='JET', type=double, rank=1, shot=" SHOT_NUM ", )", "");

    REQUIRE( result.errorCode() == 0 );
    REQUIRE( result.errorMessage().empty() );

    uda::Data* data = result.data();

    REQUIRE( data != nullptr );
    REQUIRE( !data->isNull() );
    REQUIRE( data->type().name() == typeid(double).name() );

    auto arr = dynamic_cast<uda::Array*>(data);

    REQUIRE( arr != nullptr );
    REQUIRE( !arr->isNull() );

    REQUIRE( arr->size() == 1024 );
    REQUIRE( arr->type().name() == typeid(double).name() );
    REQUIRE( arr->as<double>()[0] == Approx(-0.0000273885871) );
}

/*
 * imas::get(expName='JET', idx=0, group='magnetics', variable='bpol_probe/#/field/time', type=double, rank=1, shot=84600, )
 */
TEST_CASE( "Test bpol_probe time", "[IMAS][JET][BPOL]" )
{
#ifdef FATCLIENT
#  include "setup.inc"
#endif

    setenv("UDA_EXP2IMAS_MAPPING_FILE_DIRECTORY", MAPPINGS_DIR, 1);

    uda::Client client;

    const uda::Result& result = client.get("imas::get(idx=0, group='magnetics', variable='bpol_probe/1/field/time', expName='JET', type=double, rank=1, shot=" SHOT_NUM ", )", "");

    REQUIRE( result.errorCode() == 0 );
    REQUIRE( result.errorMessage().empty() );

    uda::Data* data = result.data();

    REQUIRE( data != nullptr );
    REQUIRE( !data->isNull() );
    REQUIRE( data->type().name() == typeid(double).name() );

    auto arr = dynamic_cast<uda::Array*>(data);

    REQUIRE( arr != nullptr );
    REQUIRE( !arr->isNull() );

    REQUIRE( arr->size() == 1024 );
    REQUIRE( arr->type().name() == typeid(double).name() );
    REQUIRE( arr->as<double>()[0] == Approx(25.0161991) );
}

/*
  ✓ flux_loop/Shape_of
  ✓ flux_loop/#/name
  ✓ flux_loop/#/identifier
  ✓ flux_loop/#/position/Shape_of
  ✓ flux_loop/#/position/#/r
  ✓ flux_loop/#/position/#/z
  ✓ flux_loop/#/position/#/phi
  ✓ flux_loop/#/flux
  ✓ flux_loop/#/flux/time
 */

TEST_CASE( "Test flux_loop count", "[IMAS][JET][FLUX]" )
{
#ifdef FATCLIENT
#  include "setup.inc"
#endif

    setenv("UDA_EXP2IMAS_MAPPING_FILE_DIRECTORY", MAPPINGS_DIR, 1);

    uda::Client client;

    const uda::Result& result = client.get("imas::get(idx=0, group='magnetics', variable='flux_loop/Shape_of', expName='JET', type=int, rank=0, shot=" SHOT_NUM ", )", "");

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
    REQUIRE( val->as<int>() == 36 );
}

TEST_CASE( "Test flux_loop name", "[IMAS][JET][FLUX]" )
{
#ifdef FATCLIENT
#  include "setup.inc"
#endif

    setenv("UDA_EXP2IMAS_MAPPING_FILE_DIRECTORY", MAPPINGS_DIR, 1);

    uda::Client client;

    const uda::Result& result = client.get("imas::get(idx=0, group='magnetics', variable='flux_loop/1/name', expName='JET', type=string, rank=0, shot=" SHOT_NUM ", )", "");

    REQUIRE( result.errorCode() == 0 );
    REQUIRE( result.errorMessage().empty() );

    uda::Data* data = result.data();

    REQUIRE( data != nullptr );
    REQUIRE( !data->isNull() );
    REQUIRE( data->type().name() == typeid(char*).name() );

    auto str = dynamic_cast<uda::String*>(data);

    REQUIRE( str != nullptr );
    REQUIRE( !str->isNull() );

    REQUIRE( str->str().empty() );
}

TEST_CASE( "Test flux_loop identifier", "[IMAS][JET][FLUX]" )
{
#ifdef FATCLIENT
#  include "setup.inc"
#endif

    setenv("UDA_EXP2IMAS_MAPPING_FILE_DIRECTORY", MAPPINGS_DIR, 1);

    uda::Client client;

    const uda::Result& result = client.get("imas::get(idx=0, group='magnetics', variable='flux_loop/1/identifier', expName='JET', type=int, rank=0, shot=" SHOT_NUM ", )", "");

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

TEST_CASE( "Test flux_loop position count", "[IMAS][JET][FLUX]" )
{
#ifdef FATCLIENT
#  include "setup.inc"
#endif

    setenv("UDA_EXP2IMAS_MAPPING_FILE_DIRECTORY", MAPPINGS_DIR, 1);

    uda::Client client;

    const uda::Result& result = client.get("imas::get(idx=0, group='magnetics', variable='flux_loop/1/position/Shape_of', expName='JET', type=int, rank=0, shot=" SHOT_NUM ", )", "");

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

TEST_CASE( "Test flux_loop position r", "[IMAS][JET][FLUX]" )
{
#ifdef FATCLIENT
#  include "setup.inc"
#endif

    setenv("UDA_EXP2IMAS_MAPPING_FILE_DIRECTORY", MAPPINGS_DIR, 1);

    uda::Client client;

    const uda::Result& result_x = client.get("imas::get(idx=0, group='magnetics', variable='flux_loop/1/position/1/r', expName='JET', type=float, rank=0, shot=" SHOT_NUM ", )", "");

    REQUIRE( result_x.errorCode() == 0 );
    REQUIRE( result_x.errorMessage().empty() );

    uda::Data* data = result_x.data();

    REQUIRE( data != nullptr );
    REQUIRE( !data->isNull() );
    REQUIRE( data->type().name() == typeid(float).name() );

    auto val = dynamic_cast<uda::Scalar*>(data);

    REQUIRE( val != nullptr );
    REQUIRE( !val->isNull() );

    REQUIRE( val->type().name() == typeid(float).name() );
    REQUIRE( val->as<float>() == Approx(2.6080f) );

    const uda::Result& result_y = client.get("imas::get(idx=0, group='magnetics', variable='flux_loop/1/position/2/r', expName='JET', type=float, rank=0, shot=" SHOT_NUM ", )", "");

    REQUIRE( result_y.errorCode() == 0 );
    REQUIRE( result_y.errorMessage().empty() );

    data = result_y.data();

    REQUIRE( data != nullptr );
    REQUIRE( !data->isNull() );
    REQUIRE( data->type().name() == typeid(float).name() );

    val = dynamic_cast<uda::Scalar*>(data);

    REQUIRE( val != nullptr );
    REQUIRE( !val->isNull() );

    REQUIRE( val->type().name() == typeid(float).name() );
    REQUIRE( val->as<float>() == Approx(0.0f) );
}

TEST_CASE( "Test flux_loop position z", "[IMAS][JET][FLUX]" )
{
#ifdef FATCLIENT
#  include "setup.inc"
#endif

    setenv("UDA_EXP2IMAS_MAPPING_FILE_DIRECTORY", MAPPINGS_DIR, 1);

    uda::Client client;

    const uda::Result& result_x = client.get("imas::get(idx=0, group='magnetics', variable='flux_loop/1/position/1/z', expName='JET', type=float, rank=0, shot=" SHOT_NUM ", )", "");

    REQUIRE( result_x.errorCode() == 0 );
    REQUIRE( result_x.errorMessage().empty() );

    uda::Data* data = result_x.data();

    REQUIRE( data != nullptr );
    REQUIRE( !data->isNull() );
    REQUIRE( data->type().name() == typeid(float).name() );

    auto val = dynamic_cast<uda::Scalar*>(data);

    REQUIRE( val != nullptr );
    REQUIRE( !val->isNull() );

    REQUIRE( val->type().name() == typeid(float).name() );
    REQUIRE( val->as<float>() == Approx(2.3220f) );

    const uda::Result& result_y = client.get("imas::get(idx=0, group='magnetics', variable='flux_loop/1/position/2/z', expName='JET', type=float, rank=0, shot=" SHOT_NUM ", )", "");

    REQUIRE( result_y.errorCode() == 0 );
    REQUIRE( result_y.errorMessage().empty() );

    data = result_y.data();

    REQUIRE( data != nullptr );
    REQUIRE( !data->isNull() );
    REQUIRE( data->type().name() == typeid(float).name() );

    val = dynamic_cast<uda::Scalar*>(data);

    REQUIRE( val != nullptr );
    REQUIRE( !val->isNull() );

    REQUIRE( val->type().name() == typeid(float).name() );
    REQUIRE( val->as<float>() == Approx(0.0f) );
}

TEST_CASE( "Test flux_loop position phi", "[IMAS][JET][FLUX]" )
{
#ifdef FATCLIENT
#  include "setup.inc"
#endif

    setenv("UDA_EXP2IMAS_MAPPING_FILE_DIRECTORY", MAPPINGS_DIR, 1);

    uda::Client client;

    const uda::Result& result_x = client.get("imas::get(idx=0, group='magnetics', variable='flux_loop/1/position/1/phi', expName='JET', type=float, rank=0, shot=" SHOT_NUM ", )", "");

    REQUIRE( result_x.errorCode() == 0 );
    REQUIRE( result_x.errorMessage().empty() );

    uda::Data* data = result_x.data();

    REQUIRE( data != nullptr );
    REQUIRE( !data->isNull() );
    REQUIRE( data->type().name() == typeid(float).name() );

    auto val = dynamic_cast<uda::Scalar*>(data);

    REQUIRE( val != nullptr );
    REQUIRE( !val->isNull() );

    REQUIRE( val->type().name() == typeid(float).name() );
    REQUIRE( val->as<float>() == Approx(0.0f) );

    const uda::Result& result_y = client.get("imas::get(idx=0, group='magnetics', variable='flux_loop/1/position/2/phi', expName='JET', type=float, rank=0, shot=" SHOT_NUM ", )", "");

    REQUIRE( result_y.errorCode() == 0 );
    REQUIRE( result_y.errorMessage().empty() );

    data = result_y.data();

    REQUIRE( data != nullptr );
    REQUIRE( !data->isNull() );
    REQUIRE( data->type().name() == typeid(float).name() );

    val = dynamic_cast<uda::Scalar*>(data);

    REQUIRE( val != nullptr );
    REQUIRE( !val->isNull() );

    REQUIRE( val->type().name() == typeid(float).name() );
    REQUIRE( val->as<float>() == Approx(0.0f) );
}

TEST_CASE( "Test flux_loop flux", "[IMAS][JET][FLUX]" )
{
#ifdef FATCLIENT
#  include "setup.inc"
#endif

    setenv("UDA_EXP2IMAS_MAPPING_FILE_DIRECTORY", MAPPINGS_DIR, 1);

    uda::Client client;

    const uda::Result& result_1 = client.get("imas::get(idx=0, group='magnetics', variable='flux_loop/1/flux/data', expName='JET', type=float, rank=1, shot=" SHOT_NUM ", )", "");

    REQUIRE( result_1.errorCode() == 0 );
    REQUIRE( result_1.errorMessage().empty() );

    uda::Data* data = result_1.data();

    REQUIRE( data != nullptr );
    REQUIRE( !data->isNull() );
    REQUIRE( data->type().name() == typeid(float).name() );

    auto arr = dynamic_cast<uda::Array*>(data);

    REQUIRE( arr != nullptr );
    REQUIRE( !arr->isNull() );

    REQUIRE( arr->size() == 1024 );
    REQUIRE( arr->type().name() == typeid(float).name() );
    REQUIRE( arr->as<float>()[0] == Approx(0.00029f) );

    const uda::Result& result_36 = client.get("imas::get(idx=0, group='magnetics', variable='flux_loop/36/flux/data', expName='JET', type=float, rank=1, shot=" SHOT_NUM ", )", "");

    REQUIRE( result_36.errorCode() == 0 );
    REQUIRE( result_36.errorMessage().empty() );

    data = result_36.data();

    REQUIRE( data != nullptr );
    REQUIRE( !data->isNull() );
    REQUIRE( data->type().name() == typeid(float).name() );

    arr = dynamic_cast<uda::Array*>(data);

    REQUIRE( arr != nullptr );
    REQUIRE( !arr->isNull() );

    REQUIRE( arr->size() == 1024 );
    REQUIRE( arr->type().name() == typeid(float).name() );
    REQUIRE( arr->as<float>()[0] == Approx(0.2953f) );
}

//TEST_CASE( "Test flux_loop flux errors", "[IMAS][JET][BPOL]" )
//{
//#ifdef FATCLIENT
//#  include "setup.inc"
//#endif
//
//    setenv("UDA_EXP2IMAS_MAPPING_FILE_DIRECTORY", MAPPINGS_DIR, 1);
//    setenv("UDA_EXP2IMAS_MAPPING_FILE", MAPPINGS_DIR "/JET_Mapping.xml", 1);
//
//    uda::Client client;
//
////    const uda::Result& error_index = client.get("imas::get(idx=0, group='magnetics', variable='flux_loop/1/flux/data_error_index', expName='JET', type=int, rank=0, shot=" SHOT_NUM ", )", "");
////
////    REQUIRE( error_index.errorCode() == 0 );
////    REQUIRE( error_index.errorMessage().empty() );
////
////    uda::Data* data = error_index.data();
////
////    REQUIRE( data != nullptr );
////    REQUIRE( !data->isNull() );
////    REQUIRE( data->type().name() == typeid(int).name() );
////
////    auto val = dynamic_cast<uda::Scalar*>(data);
////
////    REQUIRE( val != nullptr );
////    REQUIRE( !val->isNull() );
////
////    REQUIRE( val->type().name() == typeid(int).name() );
////    REQUIRE( val->as<int>() == 0 );
//
//    const uda::Result& error_upper = client.get("imas::get(idx=0, group='magnetics', variable='flux_loop/1/flux/data_error_upper', expName='JET', type=double, rank=0, shot=" SHOT_NUM ", )", "");
//
//    REQUIRE( error_upper.errorCode() == 0 );
//    REQUIRE( error_upper.errorMessage().empty() );
//
//    uda::Data* data = error_upper.data();
//
//    REQUIRE( data != nullptr );
//    REQUIRE( !data->isNull() );
//    REQUIRE( data->type().name() == typeid(double).name() );
//
//    auto arr = dynamic_cast<uda::Array*>(data);
//
//    REQUIRE( arr != nullptr );
//    REQUIRE( !arr->isNull() );
//
//    REQUIRE( arr->size() == 1024 );
//    REQUIRE( arr->type().name() == typeid(double).name() );
//    REQUIRE( arr->as<float>()[0] == Approx(0.2953f) );
//}

TEST_CASE( "Test flux_loop time", "[IMAS][JET][FLUX]" )
{
#ifdef FATCLIENT
#  include "setup.inc"
#endif

    setenv("UDA_EXP2IMAS_MAPPING_FILE_DIRECTORY", MAPPINGS_DIR, 1);

    uda::Client client;

    const uda::Result& result_1 = client.get("imas::get(idx=0, group='magnetics', variable='flux_loop/1/flux/time', expName='JET', type=float, rank=1, shot=" SHOT_NUM ", )", "");

    REQUIRE( result_1.errorCode() == 0 );
    REQUIRE( result_1.errorMessage().empty() );

    uda::Data* data = result_1.data();

    REQUIRE( data != nullptr );
    REQUIRE( !data->isNull() );
    REQUIRE( data->type().name() == typeid(float).name() );

    auto arr = dynamic_cast<uda::Array*>(data);

    REQUIRE( arr != nullptr );
    REQUIRE( !arr->isNull() );

    REQUIRE( arr->size() == 1024 );
    REQUIRE( arr->type().name() == typeid(float).name() );
    REQUIRE( arr->as<float>()[0] == Approx(25.0162f) );
    REQUIRE( arr->as<float>()[1023] == Approx(55.1882f) );

    const uda::Result& result_36 = client.get("imas::get(idx=0, group='magnetics', variable='flux_loop/36/flux/time', expName='JET', type=float, rank=1, shot=" SHOT_NUM ", )", "");

    REQUIRE( result_36.errorCode() == 0 );
    REQUIRE( result_36.errorMessage().empty() );

    data = result_36.data();

    REQUIRE( data != nullptr );
    REQUIRE( !data->isNull() );
    REQUIRE( data->type().name() == typeid(float).name() );

    arr = dynamic_cast<uda::Array*>(data);

    REQUIRE( arr != nullptr );
    REQUIRE( !arr->isNull() );

    REQUIRE( arr->size() == 1024 );
    REQUIRE( arr->type().name() == typeid(float).name() );
    REQUIRE( arr->as<float>()[0] == Approx(25.0162f) );
    REQUIRE( arr->as<float>()[1023] == Approx(55.1882f) );
}

/*
  pf_active/coil/Shape_of
  pf_active/coil/#/element/Shape_of
  pf_active/vertical_force/Shape_of
  pf_active/circuit/Shape_of
  pf_active/supply/Shape_of
 */

/*
  tf/coil/Shape_of
  tf/coil/#/conductor/Shape_of
  tf/field_map/Shape_of
  tf/field_map/grid/space/Shape_of
  tf/field_map/#/grid/space/objects_per_dimension(:)/Shape_of
  tf/field_map/#/grid/space/objects_per_dimension(:)/object(:)/Shape_of
  tf/field_map/#/grid/space/objects_per_dimension(:)/object(:)/boundary(:)/Shape_of
  tf/field_map/grid/grid_subset/Shape_of
  tf/field_map/#/grid/grid_subset/element(:)/Shape_of
  tf/field_map/#/grid/grid_subset/element(:)/object(:)/Shape_of
  tf/field_map/#/grid/grid_subset/base(:)/Shape_of
  tf/field_map/#/b_r/Shape_of
  tf/field_map/#/b_z/Shape_of
  tf/field_map/#/b_tor/Shape_of
  tf/field_map/#/a_r/Shape_of
  tf/field_map/#/a_z/Shape_of
  tf/field_map/#/a_tor/Shape_of
 */

/*
  wall/global_quantities/neutral/Shape_of
  wall/global_quantities/neutral/#/element/Shape_of
  wall/description_2d/Shape_of
  wall/description_2d/limiter/unit/Shape_of
  wall/description_2d/vessel/unit/Shape_of
  wall/description_2d/#/vessel/unit/element(:)/Shape_of
 */
