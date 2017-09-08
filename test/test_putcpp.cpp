#define CATCH_CONFIG_MAIN
#include "catch.hpp"

#include <c++/UDA.hpp>

TEST_CASE( "Test cpp putdata functionality", "[putdata]" )
{
#include "setup.inc"

    uda::Client client;

    std::vector<uda::Dim> dims;

    const int size = 10;

    double time[size];
    for (int i = 0; i < size; ++i) {
        time[i] = i * 0.01;
    }

    double axis[size];
    for (int i = 0; i < size; ++i) {
        axis[i] = i;
    }

    dims.push_back(uda::Dim( 0, time, size, "time", "s" ));
    dims.push_back(uda::Dim( 1, axis, size, "axis", "m" ));

    double data[size * size];
    for (int i = 0; i < size * size; ++i) {
        data[i] = sin(time[i]) * cos(axis[i]);
    }

    uda::Array array( data, dims );

    uda::Signal signal( array, uda::RAW, "amc", "AMC_PLASMA CURRENT", 123456, 789 );

    client.put( signal );

//    const uda::Result& result = client.get("NEWHDF5::help()", "");
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
//    std::string expected = "\nnewHDF5: get - Read data from a HDF5 file\n\n";
//
//    REQUIRE( str->str() == expected );
}