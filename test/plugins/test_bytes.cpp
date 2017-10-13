#define CATCH_CONFIG_MAIN
#include "catch.hpp"

#include <c++/UDA.hpp>

TEST_CASE( "Test BYTES::help() function", "[BYTES][plugins]" )
{
#include "setup.inc"

    uda::Client client;

    const uda::Result& result = client.get("BYTES::help()", "");

    REQUIRE( result.errorCode() == 0 );
    REQUIRE( result.errorMessage().empty() );

    uda::Data* data = result.data();

    REQUIRE( data != NULL );
    REQUIRE( !data->isNull() );
    REQUIRE( data->type().name() == typeid(char*).name() );

    auto str = dynamic_cast<uda::String*>(data);

    REQUIRE( str != NULL );

    std::string expected = "\nbytes: data reader to access files as a block of bytes without interpretation\n\n";

    REQUIRE( str->str() == expected );
}

TEST_CASE( "Test BYTES::read() function", "[BYTES][plugins]" )
{
#include "setup.inc"

    uda::Client client;

    std::string request = std::string("BYTES::read(path=") + TEST_DATA_DIR + "/bytes.dat" + ")";

    const uda::Result& result = client.get(request, "");

    REQUIRE( result.errorCode() == 0 );
    REQUIRE( result.errorMessage().empty() );

    uda::Data* data = result.data();

    REQUIRE( data != NULL );
    REQUIRE( !data->isNull() );
    REQUIRE( data->type().name() == typeid(char).name() );

    auto arr = dynamic_cast<uda::Array*>(data);

    REQUIRE( arr != NULL );

    std::string expected = "\nbytes: data reader to access files as a block of bytes without interpretation\n\n";

    REQUIRE( arr->type().name() == typeid(char).name() );
    REQUIRE( arr->size() == 5 * sizeof(int) );

    std::vector<char> bytes = arr->as<char>();

    auto numbers = reinterpret_cast<int*>(bytes.data());

    REQUIRE( numbers[0] == 1 );
    REQUIRE( numbers[1] == 2 );
    REQUIRE( numbers[2] == 3 );
    REQUIRE( numbers[3] == 4 );
    REQUIRE( numbers[4] == 5 );
}