#define CATCH_CONFIG_MAIN
#include "catch.hpp"

#include <c++/UDA.hpp>

//TEST_CASE( "Test get MAST static data", "[imas]" ) {
//
//#ifdef FATCLIENT
//#  include "setupEnvironment.inc"
//#endif
//
//    uda::Client client;
//
//    const uda::Result& result = client.get("MAGNETICS/FLUX_LOOP/1/NAME", "MAST::");
//
//    REQUIRE( result.errorCode() == 0 );
//    REQUIRE( result.error() == "" );
//
//    uda::Data* data = result.data();
//
//    REQUIRE( data != NULL );
//    REQUIRE( !data->isNull() );
//    REQUIRE( data->type().name() == typeid(char*).name() );
//
//    uda::String* str = dynamic_cast<uda::String*>(data);
//
//    REQUIRE( str != NULL );
//
//    std::string expected = "amb_fl/cc01";
//
//    REQUIRE( str->str() == expected );
//}

TEST_CASE( "Test get MAST experimental data", "[imas]" )
{

#ifdef FATCLIENT

#  include "setupEnvironment.inc"
#endif

    uda::Client client;

    const uda::Result& result = client.get("IMAS::open(filename=MAST, shotNumber=18299, runNumber=0)", "");

    REQUIRE( result.errorCode() == 0 );
    REQUIRE( result.error() == "" );

    uda::Data* data = result.data();

    uda::Scalar* scalar = dynamic_cast<uda::Scalar*>(data);

    REQUIRE( scalar != NULL );

    REQUIRE( scalar->as<int>() == 0 );

//    const uda::Result& result2 = client.get("IMAS::source(signal=\"amb_fl/cc01\", /Data, format=MAST)", "18299/0");
//    const uda::Result& result2 = client.get("UDA::get(host=idam3.mast.ccfe.ac.uk, port=56565, signal=\"amb_fl/cc01\", source=\"18299/0\")", "");
//    const uda::Result& result2 = client.get("MAGNETICS/FLUX_LOOP/1/FLUX/DATA", "18299/0");
    const uda::Result& result2 = client.get("IMAS::get(clientIdx=0, expName='MAST', cpopath='MAGNETICS', path='FLUX_LOOP/1/FLUX/DATA', typeName=double, rank=1, shot=18299)", "");

    REQUIRE( result2.errorCode() == 0 );
    REQUIRE( result2.error() == "" );

    uda::Data* data2 = result2.data();

    REQUIRE( data2 != NULL );
    REQUIRE( !data2->isNull() );
    REQUIRE( data2->type().name() == typeid(double).name() );

    uda::Array* arr = dynamic_cast<uda::Array*>(data2);

    REQUIRE( arr != NULL );

    double expected[] = {
        -0.00022f, 0.00002f, -0.00022f, -0.00022f, -0.00022f, -0.00022f, 0.00003f, -0.00022f, -0.00022f, -0.00022f
    };

//    for (int i = 0; i < 10; ++i) {
//        REQUIRE( arr->at<double>(i) == Approx( expected[i] ) );
//    }

    for (int i = 0; i < 10; ++i) {
        REQUIRE( arr->as<double>()[i] == Approx( expected[i] ) );
    }
}