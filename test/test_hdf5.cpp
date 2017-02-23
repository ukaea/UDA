#define CATCH_CONFIG_MAIN
#include "catch.hpp"

#include <c++/UDA.hpp>

TEST_CASE( "Test hdf5::test() function", "[hdf5]" ) {

#ifdef FATCLIENT
#  include "setupEnvironment.inc"
#endif

    Idam::Client client;

    const Idam::Result& result = client.get("NEWHDF5::help()", "");

    REQUIRE( result.errorCode() == 0 );
    REQUIRE( result.error() == "" );

    Idam::Data* data = result.data();

    REQUIRE( data != NULL );
    REQUIRE( !data->isNull() );
    REQUIRE( data->type().name() == typeid(char*).name() );

    Idam::String* str = dynamic_cast<Idam::String*>(data);

    REQUIRE( str != NULL );

    std::string expected = "\nnewHDF5: get - Read data from a HDF5 file\n\n";

    REQUIRE( str->str() == expected );
}
