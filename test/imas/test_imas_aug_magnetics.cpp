#define CATCH_CONFIG_MAIN
#include "catch.hpp"

#include <c++/UDA.hpp>

#include "test_helpers.h"

#define QUOTE_(X) #X
#define QUOTE(X) QUOTE_(X)
#define SHOT_NUM "33173"

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
 * imas::get(expName='AUG', idx=0, group='magnetics', variable='bpol_probe/Shape_of', type=int, rank=0, shot=84600, )
 */
TEST_CASE( "Test bpol_probe count", "[IMAS][AUG][BPOL]" )
{
#ifdef FATCLIENT
#  include "setup.inc"
#endif

    uda::Client client;

    const uda::Result& result = client.get("imas::get(idx=0, group='magnetics', variable='bpol_probe/Shape_of', expName='AUG', type=int, rank=0, shot=" SHOT_NUM ", )", "");

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
    REQUIRE( val->as<int>() == 62 );
}

/*
 * imas::get(expName='AUG', idx=0, group='magnetics', variable='bpol_probe/#/name', type=string, rank=0, shot=84600, )
 */
TEST_CASE( "Test bpol_probe name", "[IMAS][AUG][BPOL]" )
{
#ifdef FATCLIENT
#  include "setup.inc"
#endif

    uda::Client client;

    const uda::Result& result = client.get("imas::get(idx=0, group='magnetics', variable='bpol_probe/1/name', expName='AUG', type=string, rank=0, shot=" SHOT_NUM ", )", "");

    REQUIRE( result.errorCode() == 0 );
    REQUIRE( result.errorMessage().empty() );

    uda::Data* data = result.data();

    REQUIRE( data != nullptr );
    REQUIRE( !data->isNull() );
    REQUIRE( data->type().name() == typeid(char*).name() );

    auto val = dynamic_cast<uda::String*>(data);

    REQUIRE( val != nullptr );
    REQUIRE( !val->isNull() );

    REQUIRE( val->str() == "Bthe01" );
}

/*
 * imas::get(expName='AUG', idx=0, group='magnetics', variable='bpol_probe/#/identifier', type=string, rank=0, shot=84600, )
 */
TEST_CASE( "Test bpol_probe identifier", "[IMAS][AUG][BPOL]" )
{
#ifdef FATCLIENT
#  include "setup.inc"
#endif

    uda::Client client;

    const uda::Result& result = client.get("imas::get(idx=0, group='magnetics', variable='bpol_probe/1/identifier', expName='AUG', type=int, rank=0, shot=" SHOT_NUM ", )", "");

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
 * imas::get(expName='AUG', idx=0, group='magnetics', variable='bpol_probe/#/toroidal_angle_error_index', type=int, rank=0, shot=84600, )
 * imas::get(expName='AUG', idx=0, group='magnetics', variable='bpol_probe/#/toroidal_angle_error_lower', type=double, rank=0, shot=84600, )
 * imas::get(expName='AUG', idx=0, group='magnetics', variable='bpol_probe/#/toroidal_angle_error_upper', type=double, rank=0, shot=84600, )
 * imas::get(expName='AUG', idx=0, group='magnetics', variable='bpol_probe/#/toroidal_angle', type=double, rank=0, shot=84600, )
 */
TEST_CASE( "Test bpol_probe toroidal_angle", "[IMAS][AUG][BPOL]" )
{
#ifdef FATCLIENT
#  include "setup.inc"
#endif

	uda::Client client;

	const uda::Result& result = client.get("imas::get(idx=0, group='magnetics', variable='bpol_probe/1/toroidal_angle', expName='AUG', type=double, rank=0, shot=" SHOT_NUM ", )", "");

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
 * imas::get(expName='AUG', idx=0, group='magnetics', variable='bpol_probe/#/poloidal_angle_error_index', type=int, rank=0, shot=84600, )
 * imas::get(expName='AUG', idx=0, group='magnetics', variable='bpol_probe/#/poloidal_angle_error_lower', type=double, rank=0, shot=84600, )
 * imas::get(expName='AUG', idx=0, group='magnetics', variable='bpol_probe/#/poloidal_angle_error_upper', type=double, rank=0, shot=84600, )
 * imas::get(expName='AUG', idx=0, group='magnetics', variable='bpol_probe/#/poloidal_angle', type=double, rank=0, shot=84600, )
 */
TEST_CASE( "Test bpol_probe poloidal_angle", "[IMAS][AUG][BPOL]" )
{
#ifdef FATCLIENT
#  include "setup.inc"
#endif

    uda::Client client;

    double expected_vals[] = { -1.51257, -1.39984, -1.28770, -1.17506, -1.06257 };

    int index = 1;
    for (auto expected_val : expected_vals) {

        std::string signal = "imas::get(idx=0, group='magnetics', variable='bpol_probe/"
                             + std::to_string(index)
                             + "/poloidal_angle', expName='AUG', type=double, rank=0, shot=" + SHOT_NUM + ", )";

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
 * imas::get(expName='AUG', idx=0, group='magnetics', variable='bpol_probe/#/area_error_index', type=int, rank=0, shot=84600, )
 * imas::get(expName='AUG', idx=0, group='magnetics', variable='bpol_probe/#/area_error_lower', type=double, rank=0, shot=84600, )
 * imas::get(expName='AUG', idx=0, group='magnetics', variable='bpol_probe/#/area_error_upper', type=double, rank=0, shot=84600, )
 * imas::get(expName='AUG', idx=0, group='magnetics', variable='bpol_probe/#/area', type=double, rank=0, shot=84600, )
 */
TEST_CASE( "Test bpol_probe area", "[IMAS][AUG][BPOL]" )
{
#ifdef FATCLIENT
#  include "setup.inc"
#endif

    uda::Client client;

    const uda::Result& result = client.get("imas::get(idx=0, group='magnetics', variable='bpol_probe/1/area', expName='AUG', type=double, rank=0, shot=" SHOT_NUM ", )", "");

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
    REQUIRE( val->as<double>() == Approx(0.002774) );
}

/*
 * imas::get(expName='AUG', idx=0, group='magnetics', variable='bpol_probe/#/position/r_error_index', type=int, rank=0, shot=84600, )
 * imas::get(expName='AUG', idx=0, group='magnetics', variable='bpol_probe/#/position/r_error_lower', type=double, rank=0, shot=84600, )
 * imas::get(expName='AUG', idx=0, group='magnetics', variable='bpol_probe/#/position/r_error_upper', type=double, rank=0, shot=84600, )
 * imas::get(expName='AUG', idx=0, group='magnetics', variable='bpol_probe/#/position/r', type=double, rank=0, shot=84600, )
 */
TEST_CASE( "Test bpol_probe position r", "[IMAS][AUG][BPOL]" )
{
#ifdef FATCLIENT
#  include "setup.inc"
#endif

    uda::Client client;

    const uda::Result& result = client.get("imas::get(idx=0, group='magnetics', variable='bpol_probe/1/position/r', expName='AUG', type=double, rank=0, shot=" SHOT_NUM ", )", "");

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
    REQUIRE( val->as<double>() == Approx(2.5050) );
}

/*
 * imas::get(expName='AUG', idx=0, group='magnetics', variable='bpol_probe/#/position/z_error_index', type=int, rank=0, shot=84600, )
 * imas::get(expName='AUG', idx=0, group='magnetics', variable='bpol_probe/#/position/z_error_lower', type=double, rank=0, shot=84600, )
 * imas::get(expName='AUG', idx=0, group='magnetics', variable='bpol_probe/#/position/z_error_upper', type=double, rank=0, shot=84600, )
 * imas::get(expName='AUG', idx=0, group='magnetics', variable='bpol_probe/#/position/z', type=double, rank=0, shot=84600, )
 */
TEST_CASE( "Test bpol_probe position z", "[IMAS][AUG][BPOL]" )
{
#ifdef FATCLIENT
#  include "setup.inc"
#endif

    uda::Client client;

    const uda::Result& result = client.get("imas::get(idx=0, group='magnetics', variable='bpol_probe/1/position/z', expName='AUG', type=double, rank=0, shot=" SHOT_NUM ", )", "");

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
    REQUIRE( val->as<double>() == Approx(0.0829) );
}

/*
 * imas::get(expName='AUG', idx=0, group='magnetics', variable='bpol_probe/#/position/phi_error_index', type=int, rank=0, shot=84600, )
 * imas::get(expName='AUG', idx=0, group='magnetics', variable='bpol_probe/#/position/phi_error_lower', type=double, rank=0, shot=84600, )
 * imas::get(expName='AUG', idx=0, group='magnetics', variable='bpol_probe/#/position/phi_error_upper', type=double, rank=0, shot=84600, )
 * imas::get(expName='AUG', idx=0, group='magnetics', variable='bpol_probe/#/position/phi', type=double, rank=0, shot=84600, )
 */
//TEST_CASE( "Test bpol_probe position phi", "[IMAS][AUG][BPOL]" )
//{
//#ifdef FATCLIENT
//#  include "setup.inc"
//#endif
//
//    uda::Client client;
//
//    const uda::Result& result = client.get("imas::get(idx=0, group='magnetics', variable='bpol_probe/1/position/phi', expName='AUG', type=double, rank=0, shot=" SHOT_NUM ", )", "");
//
//    REQUIRE( result.errorCode() == 0 );
//    REQUIRE( result.errorMessage().empty() );
//
//    uda::Data* data = result.data();
//
//    REQUIRE( data != nullptr );
//    REQUIRE( !data->isNull() );
//    REQUIRE( data->type().name() == typeid(double).name() );
//
//    auto val = dynamic_cast<uda::Scalar*>(data);
//
//    REQUIRE( val != nullptr );
//    REQUIRE( !val->isNull() );
//
//    REQUIRE( val->type().name() == typeid(double).name() );
//    REQUIRE( val->as<double>() == Approx(0.0) );
//}

/*
 * imas::get(expName='AUG', idx=0, group='magnetics', variable='bpol_probe/#/length_error_index', type=int, rank=0, shot=84600, )
 * imas::get(expName='AUG', idx=0, group='magnetics', variable='bpol_probe/#/length_error_lower', type=double, rank=0, shot=84600, )
 * imas::get(expName='AUG', idx=0, group='magnetics', variable='bpol_probe/#/length_error_upper', type=double, rank=0, shot=84600, )
 * imas::get(expName='AUG', idx=0, group='magnetics', variable='bpol_probe/#/length', type=double, rank=0, shot=84600, )
 */
TEST_CASE( "Test bpol_probe length", "[IMAS][AUG][BPOL]" )
{
#ifdef FATCLIENT
#  include "setup.inc"
#endif

    uda::Client client;

    const uda::Result& result = client.get("imas::get(idx=0, group='magnetics', variable='bpol_probe/1/length', expName='AUG', type=double, rank=0, shot=" SHOT_NUM ", )", "");

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
    REQUIRE( val->as<double>() == Approx(0.131) );
}

/*
 * imas::get(expName='AUG', idx=0, group='magnetics', variable='bpol_probe/#/turns', type=int, rank=0, shot=84600, )
 */
TEST_CASE( "Test bpol_probe turns", "[IMAS][AUG][BPOL]" )
{
#ifdef FATCLIENT
#  include "setup.inc"
#endif

    uda::Client client;

    const uda::Result& result = client.get("imas::get(idx=0, group='magnetics', variable='bpol_probe/1/turns', expName='AUG', type=int, rank=0, shot=" SHOT_NUM ", )", "");

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
    REQUIRE( val->as<int>() == 131 );
}

/*
 * imas::get(expName='AUG', idx=0, group='magnetics', variable='bpol_probe/#/field/data_error_index', type=int, rank=0, shot=84600, )
 * imas::get(expName='AUG', idx=0, group='magnetics', variable='bpol_probe/#/field/data_error_lower', type=double, rank=1, shot=84600, )
 * imas::get(expName='AUG', idx=0, group='magnetics', variable='bpol_probe/#/field/data_error_upper', type=double, rank=1, shot=84600, )
 * imas::get(expName='AUG', idx=0, group='magnetics', variable='bpol_probe/#/field/data', type=double, rank=1, shot=84600, )
 */
TEST_CASE( "Test bpol_probe field", "[IMAS][AUG][BPOL]" )
{
#ifdef FATCLIENT
#  include "setup.inc"
#endif

    uda::Client client;

    const uda::Result& result = client.get("imas::get(idx=0, group='magnetics', variable='bpol_probe/1/field/data', expName='AUG', type=double, rank=1, shot=" SHOT_NUM ", )", "");

    REQUIRE( result.errorCode() == 0 );
    REQUIRE( result.errorMessage().empty() );

    uda::Data* data = result.data();

    REQUIRE( data != nullptr );
    REQUIRE( !data->isNull() );
    REQUIRE( data->type().name() == typeid(double).name() );

    auto arr = dynamic_cast<uda::Array*>(data);

    REQUIRE( arr != nullptr );
    REQUIRE( !arr->isNull() );

    std::vector<double> expected{ 0.0215063058, 0.0215953439, 0.0216131546, 0.0215597302, 0.0214172639 };

    REQUIRE( arr->size() == 100001 );
    REQUIRE( arr->type().name() == typeid(double).name() );

    auto vals = arr->as<double>();
    vals.resize(5);

    REQUIRE( vals == ApproxVector(expected) );
}

TEST_CASE( "Test bpol_probe field error upper", "[IMAS][AUG][BPOL]" )
{
#ifdef FATCLIENT
#  include "setup.inc"
#endif

    uda::Client client;

    const uda::Result& result = client.get("imas::get(idx=0, group='magnetics', variable='bpol_probe/1/field/data_error_upper', expName='AUG', type=double, rank=1, shot=" SHOT_NUM ", )", "");

    REQUIRE( result.errorCode() == 0 );
    REQUIRE( result.errorMessage().empty() );

    uda::Data* data = result.data();

    REQUIRE( data != nullptr );
    REQUIRE( !data->isNull() );
    REQUIRE( data->type().name() == typeid(double).name() );

    auto arr = dynamic_cast<uda::Array*>(data);

    REQUIRE( arr != nullptr );
    REQUIRE( !arr->isNull() );

    std::vector<double> expected{ 0.0222563054, 0.0223453436, 0.0223631542, 0.0223097298, 0.0221672636 };

    REQUIRE( arr->size() == 100001 );
    REQUIRE( arr->type().name() == typeid(double).name() );

    auto vals = arr->as<double>();
    vals.resize(5);

    REQUIRE( vals == ApproxVector(expected) );
}

TEST_CASE( "Test bpol_probe field error lower", "[IMAS][AUG][BPOL]" )
{
#ifdef FATCLIENT
#  include "setup.inc"
#endif

    uda::Client client;

    const uda::Result& result = client.get("imas::get(idx=0, group='magnetics', variable='bpol_probe/1/field/data_error_lower', expName='AUG', type=double, rank=1, shot=" SHOT_NUM ", )", "");

    REQUIRE( result.errorCode() == 0 );
    REQUIRE( result.errorMessage().empty() );

    uda::Data* data = result.data();

    REQUIRE( data != nullptr );
    REQUIRE( !data->isNull() );
    REQUIRE( data->type().name() == typeid(double).name() );

    auto arr = dynamic_cast<uda::Array*>(data);

    REQUIRE( arr != nullptr );
    REQUIRE( !arr->isNull() );

    std::vector<double> expected{ 0.0207563061, 0.0208453443, 0.0208631549, 0.0208097305, 0.0206672642 };

    REQUIRE( arr->size() == 100001 );
    REQUIRE( arr->type().name() == typeid(double).name() );

    auto vals = arr->as<double>();
    vals.resize(5);

    REQUIRE( vals == ApproxVector(expected) );
}

/*
 * imas::get(expName='AUG', idx=0, group='magnetics', variable='bpol_probe/#/field/time', type=double, rank=1, shot=84600, )
 */
TEST_CASE( "Test bpol_probe time", "[IMAS][AUG][BPOL]" )
{
#ifdef FATCLIENT
#  include "setup.inc"
#endif

    uda::Client client;

    const uda::Result& result = client.get("imas::get(idx=0, group='magnetics', variable='bpol_probe/1/field/time', expName='AUG', type=double, rank=1, shot=" SHOT_NUM ", )", "");

    REQUIRE( result.errorCode() == 0 );
    REQUIRE( result.errorMessage().empty() );

    uda::Data* data = result.data();

    REQUIRE( data != nullptr );
    REQUIRE( !data->isNull() );
    REQUIRE( data->type().name() == typeid(double).name() );

    auto arr = dynamic_cast<uda::Array*>(data);

    REQUIRE( arr != nullptr );
    REQUIRE( !arr->isNull() );

    std::vector<double> expected{ -0.0000305176, 0.0000696182, 0.000169754, 0.0002698898, 0.0003700256 };

    REQUIRE( arr->size() == 100001 );
    REQUIRE( arr->type().name() == typeid(double).name() );

    auto vals = arr->as<double>();
    vals.resize(5);

    REQUIRE( vals == ApproxVector(expected) );
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

TEST_CASE( "Test flux_loop count", "[IMAS][AUG][FLUX]" )
{
#ifdef FATCLIENT
#  include "setup.inc"
#endif

    uda::Client client;

    const uda::Result& result = client.get("imas::get(idx=0, group='magnetics', variable='flux_loop/Shape_of', expName='AUG', type=int, rank=0, shot=" SHOT_NUM ", )", "");

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
    REQUIRE( val->as<int>() == 18 );
}

TEST_CASE( "Test flux_loop name", "[IMAS][AUG][FLUX]" )
{
#ifdef FATCLIENT
#  include "setup.inc"
#endif

    uda::Client client;

    const uda::Result& result = client.get("imas::get(idx=0, group='magnetics', variable='flux_loop/1/name', expName='AUG', type=string, rank=0, shot=" SHOT_NUM ", )", "");

    REQUIRE( result.errorCode() == 0 );
    REQUIRE( result.errorMessage().empty() );

    uda::Data* data = result.data();

    REQUIRE( data != nullptr );
    REQUIRE( !data->isNull() );
    REQUIRE( data->type().name() == typeid(char*).name() );

    auto str = dynamic_cast<uda::String*>(data);

    REQUIRE( str != nullptr );
    REQUIRE( !str->isNull() );

    REQUIRE( str->str() == "Dpsi01D" );
}

TEST_CASE( "Test flux_loop identifier", "[IMAS][AUG][FLUX]" )
{
#ifdef FATCLIENT
#  include "setup.inc"
#endif

    uda::Client client;

    const uda::Result& result = client.get("imas::get(idx=0, group='magnetics', variable='flux_loop/1/identifier', expName='AUG', type=int, rank=0, shot=" SHOT_NUM ", )", "");

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

TEST_CASE( "Test flux_loop position count", "[IMAS][AUG][FLUX]" )
{
#ifdef FATCLIENT
#  include "setup.inc"
#endif

    uda::Client client;

    const uda::Result& result = client.get(
            "imas::get(idx=0, group='magnetics', variable='flux_loop/1/position/Shape_of', expName='AUG', type=int, rank=0, shot=" SHOT_NUM ", )",
            "");

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
    REQUIRE( val->as<int>() == 8 );
}

TEST_CASE( "Test flux_loop position r", "[IMAS][AUG][FLUX]" )
{
#ifdef FATCLIENT
#  include "setup.inc"
#endif

    uda::Client client;

    const uda::Result& result_x = client.get("imas::get(idx=0, group='magnetics', variable='flux_loop/1/position/1/r', expName='AUG', type=float, rank=0, shot=" SHOT_NUM ", )", "");

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
    REQUIRE( val->as<float>() == Approx(2.2888f) );

    const uda::Result& result_y = client.get("imas::get(idx=0, group='magnetics', variable='flux_loop/2/position/1/r', expName='AUG', type=float, rank=0, shot=" SHOT_NUM ", )", "");

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
    REQUIRE( val->as<float>() == Approx(1.6315f) );
}

TEST_CASE( "Test flux_loop position z", "[IMAS][AUG][FLUX]" )
{
#ifdef FATCLIENT
#  include "setup.inc"
#endif

    uda::Client client;

    const uda::Result& result_x = client.get("imas::get(idx=0, group='magnetics', variable='flux_loop/1/position/1/z', expName='AUG', type=float, rank=0, shot=" SHOT_NUM ", )", "");

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
    REQUIRE( val->as<float>() == Approx(0.685f) );

    const uda::Result& result_y = client.get("imas::get(idx=0, group='magnetics', variable='flux_loop/2/position/1/z', expName='AUG', type=float, rank=0, shot=" SHOT_NUM ", )", "");

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
    REQUIRE( val->as<float>() == Approx(1.219f) );
}

TEST_CASE( "Test flux_loop position phi", "[IMAS][AUG][FLUX]" )
{
#ifdef FATCLIENT
#  include "setup.inc"
#endif

    uda::Client client;

    const uda::Result& result_x = client.get("imas::get(idx=0, group='magnetics', variable='flux_loop/1/position/1/phi', expName='AUG', type=float, rank=0, shot=" SHOT_NUM ", )", "");

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

    const uda::Result& result_y = client.get("imas::get(idx=0, group='magnetics', variable='flux_loop/2/position/1/phi', expName='AUG', type=float, rank=0, shot=" SHOT_NUM ", )", "");

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

TEST_CASE( "Test flux_loop flux", "[IMAS][AUG][FLUX]" )
{
#ifdef FATCLIENT
#  include "setup.inc"
#endif

    uda::Client client;

    const uda::Result& result_1 = client.get("imas::get(idx=0, group='magnetics', variable='flux_loop/1/flux/data', expName='AUG', type=float, rank=1, shot=" SHOT_NUM ", )", "");

    REQUIRE( result_1.errorCode() == 0 );
    REQUIRE( result_1.errorMessage().empty() );

    uda::Data* data = result_1.data();

    REQUIRE( data != nullptr );
    REQUIRE( !data->isNull() );
    REQUIRE( data->type().name() == typeid(float).name() );

    auto arr = dynamic_cast<uda::Array*>(data);

    REQUIRE( arr != nullptr );
    REQUIRE( !arr->isNull() );

    std::vector<float> expected{ 0.12588f, 0.12575f, 0.12588f, 0.12569f, 0.12594f };

    REQUIRE( arr->size() == 100001 );
    REQUIRE( arr->type().name() == typeid(float).name() );

    auto vals = arr->as<float>();
    vals.resize(5);

    REQUIRE( vals == ApproxVector(expected) );

    const uda::Result& result_36 = client.get("imas::get(idx=0, group='magnetics', variable='flux_loop/18/flux/data', expName='AUG', type=float, rank=1, shot=" SHOT_NUM ", )", "");

    REQUIRE( result_36.errorCode() == 0 );
    REQUIRE( result_36.errorMessage().empty() );

    data = result_36.data();

    REQUIRE( data != nullptr );
    REQUIRE( !data->isNull() );
    REQUIRE( data->type().name() == typeid(float).name() );

    arr = dynamic_cast<uda::Array*>(data);

    REQUIRE( arr != nullptr );
    REQUIRE( !arr->isNull() );

    expected = std::vector<float>{ -0.02919f, -0.02928f, -0.02932f, -0.02928f, -0.02917f };

    REQUIRE( arr->size() == 100001 );
    REQUIRE( arr->type().name() == typeid(float).name() );

    vals = arr->as<float>();
    vals.resize(5);

    REQUIRE( vals == ApproxVector(expected) );
}

TEST_CASE( "Test flux_loop flux error upper", "[IMAS][AUG][FLUX]" )
{
#ifdef FATCLIENT
#  include "setup.inc"
#endif

    uda::Client client;

    const uda::Result& result_1 = client.get("imas::get(idx=0, group='magnetics', variable='flux_loop/1/flux/data_error_upper', expName='AUG', type=float, rank=1, shot=" SHOT_NUM ", )", "");

    REQUIRE( result_1.errorCode() == 0 );
    REQUIRE( result_1.errorMessage().empty() );

    uda::Data* data = result_1.data();

    REQUIRE( data != nullptr );
    REQUIRE( !data->isNull() );
    REQUIRE( data->type().name() == typeid(float).name() );

    auto arr = dynamic_cast<uda::Array*>(data);

    REQUIRE( arr != nullptr );
    REQUIRE( !arr->isNull() );

    std::vector<float> expected{ 8.31355f, 8.31342f, 8.31355f, 8.31336f, 8.31361f };

    REQUIRE( arr->size() == 100001 );
    REQUIRE( arr->type().name() == typeid(float).name() );

    auto vals = arr->as<float>();
    vals.resize(5);

    REQUIRE( vals == ApproxVector(expected) );

    const uda::Result& result_36 = client.get("imas::get(idx=0, group='magnetics', variable='flux_loop/18/flux/data_error_upper', expName='AUG', type=float, rank=1, shot=" SHOT_NUM ", )", "");

    REQUIRE( result_36.errorCode() == 0 );
    REQUIRE( result_36.errorMessage().empty() );

    data = result_36.data();

    REQUIRE( data != nullptr );
    REQUIRE( !data->isNull() );
    REQUIRE( data->type().name() == typeid(float).name() );

    arr = dynamic_cast<uda::Array*>(data);

    REQUIRE( arr != nullptr );
    REQUIRE( !arr->isNull() );

    expected = std::vector<float>{ 8.15848f, 8.15839f, 8.15835f, 8.1584f, 8.1585f };

    REQUIRE( arr->size() == 100001 );
    REQUIRE( arr->type().name() == typeid(float).name() );

    vals = arr->as<float>();
    vals.resize(5);

    REQUIRE( vals == ApproxVector(expected) );
}

TEST_CASE( "Test flux_loop flux error lower", "[IMAS][AUG][FLUX]" )
{
#ifdef FATCLIENT
#  include "setup.inc"
#endif

    uda::Client client;

    const uda::Result& result_1 = client.get("imas::get(idx=0, group='magnetics', variable='flux_loop/1/flux/data_error_lower', expName='AUG', type=float, rank=1, shot=" SHOT_NUM ", )", "");

    REQUIRE( result_1.errorCode() == 0 );
    REQUIRE( result_1.errorMessage().empty() );

    uda::Data* data = result_1.data();

    REQUIRE( data != nullptr );
    REQUIRE( !data->isNull() );
    REQUIRE( data->type().name() == typeid(float).name() );

    auto arr = dynamic_cast<uda::Array*>(data);

    REQUIRE( arr != nullptr );
    REQUIRE( !arr->isNull() );

    std::vector<float> expected{ -8.0618f, -8.06192f, -8.0618f, -8.06199f, -8.06173f };

    REQUIRE( arr->size() == 100001 );
    REQUIRE( arr->type().name() == typeid(float).name() );

    auto vals = arr->as<float>();
    vals.resize(5);

    REQUIRE( vals == ApproxVector(expected) );

    const uda::Result& result_36 = client.get("imas::get(idx=0, group='magnetics', variable='flux_loop/18/flux/data_error_lower', expName='AUG', type=float, rank=1, shot=" SHOT_NUM ", )", "");

    REQUIRE( result_36.errorCode() == 0 );
    REQUIRE( result_36.errorMessage().empty() );

    data = result_36.data();

    REQUIRE( data != nullptr );
    REQUIRE( !data->isNull() );
    REQUIRE( data->type().name() == typeid(float).name() );

    arr = dynamic_cast<uda::Array*>(data);

    REQUIRE( arr != nullptr );
    REQUIRE( !arr->isNull() );

    expected = std::vector<float>{ -8.21686f, -8.21696f, -8.217f, -8.21695f, -8.21684f };

    REQUIRE( arr->size() == 100001 );
    REQUIRE( arr->type().name() == typeid(float).name() );

    vals = arr->as<float>();
    vals.resize(5);

    REQUIRE( vals == ApproxVector(expected) );
}

//TEST_CASE( "Test flux_loop flux errors", "[IMAS][AUG][BPOL]" )
//{
//#ifdef FATCLIENT
//#  include "setup.inc"
//#endif
//
//    setenv("UDA_EXP2IMAS_MAPPING_FILE_DIRECTORY", MAPPINGS_DIR, 1);
//
//    uda::Client client;
//
////    const uda::Result& error_index = client.get("imas::get(idx=0, group='magnetics', variable='flux_loop/1/flux/data_error_index', expName='AUG', type=int, rank=0, shot=" SHOT_NUM ", )", "");
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
//    const uda::Result& error_upper = client.get("imas::get(idx=0, group='magnetics', variable='flux_loop/1/flux/data_error_upper', expName='AUG', type=double, rank=0, shot=" SHOT_NUM ", )", "");
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

TEST_CASE( "Test flux_loop time", "[IMAS][AUG][FLUX]" )
{
#ifdef FATCLIENT
#  include "setup.inc"
#endif

    uda::Client client;

    const uda::Result& result_1 = client.get("imas::get(idx=0, group='magnetics', variable='flux_loop/1/flux/time', expName='AUG', type=float, rank=1, shot=" SHOT_NUM ", )", "");

    REQUIRE( result_1.errorCode() == 0 );
    REQUIRE( result_1.errorMessage().empty() );

    uda::Data* data = result_1.data();

    REQUIRE( data != nullptr );
    REQUIRE( !data->isNull() );
    REQUIRE( data->type().name() == typeid(float).name() );

    auto arr = dynamic_cast<uda::Array*>(data);

    REQUIRE( arr != nullptr );
    REQUIRE( !arr->isNull() );

    std::vector<float> expected{ -0.00003f, 0.00007f, 0.00017f, 0.00027f, 0.00037f };

    REQUIRE( arr->size() == 100001 );
    REQUIRE( arr->type().name() == typeid(float).name() );

    auto vals = arr->as<float>();
    vals.resize(5);

    REQUIRE( vals == ApproxVector(expected) );

    const uda::Result& result_36 = client.get("imas::get(idx=0, group='magnetics', variable='flux_loop/18/flux/time', expName='AUG', type=float, rank=1, shot=" SHOT_NUM ", )", "");

    REQUIRE( result_36.errorCode() == 0 );
    REQUIRE( result_36.errorMessage().empty() );

    data = result_36.data();

    REQUIRE( data != nullptr );
    REQUIRE( !data->isNull() );
    REQUIRE( data->type().name() == typeid(float).name() );

    arr = dynamic_cast<uda::Array*>(data);

    REQUIRE( arr != nullptr );
    REQUIRE( !arr->isNull() );

    expected = std::vector<float>{ -0.00003f, 0.00007f, 0.00017f, 0.00027f, 0.00037f };

    REQUIRE( arr->size() == 100001 );
    REQUIRE( arr->type().name() == typeid(float).name() );

    vals = arr->as<float>();
    vals.resize(5);

    REQUIRE( vals == ApproxVector(expected) );
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
