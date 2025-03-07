#include <numeric>
#include <catch2/catch_test_macros.hpp>
#include <catch2/matchers/catch_matchers_floating_point.hpp>

#include "clientserver/compress_dim.h"

TEST_CASE( "Compress int dim", "[compress-dim]" )
{
    uda::client_server::Dims dim{};

    std::vector<int> data(100);
    std::iota(data.begin(), data.end(), 0);

    dim.compressed = 0;
    dim.data_type = UDA_TYPE_INT;
    dim.dim = reinterpret_cast<char*>(data.data());
    dim.dim_n = 100;

    auto rc = uda::client_server::compress_dim(&dim);
    REQUIRE( rc == 0 );

    REQUIRE( dim.data_type == UDA_TYPE_INT );
    REQUIRE( dim.compressed == 1 );
    REQUIRE( dim.method == 0 );
    REQUIRE( dim.dim0 == 0 );
    REQUIRE( dim.diff == 1 );
}

TEST_CASE( "Compress float dim", "[compress-dim]" )
{
    uda::client_server::Dims dim{};

    std::vector<float> data(100);
    std::iota(data.begin(), data.end(), 0.0f);
    std::transform(data.begin(), data.end(), data.begin(), [](const float x){return x/10.0f;});

    dim.compressed = 0;
    dim.data_type = UDA_TYPE_FLOAT;
    dim.dim = reinterpret_cast<char*>(data.data());
    dim.dim_n = 100;

    auto rc = uda::client_server::compress_dim(&dim);
    REQUIRE( rc == 0 );

    REQUIRE( dim.data_type == UDA_TYPE_FLOAT );
    REQUIRE( dim.compressed == 1 );
    REQUIRE( dim.method == 0 );
    REQUIRE( dim.dim0 == 0.0f );
    REQUIRE_THAT( dim.diff, Catch::Matchers::WithinRel(0.1f) );
}

TEST_CASE( "Decompress int dim", "[compress-dim]" )
{
    uda::client_server::Dims dim{};
    dim.compressed = 1;
    dim.data_type = UDA_TYPE_INT;
    dim.dim_n = 100;
    dim.method = 0;
    dim.dim0 = 0;
    dim.diff = 1;

    auto rc = uda::client_server::uncompress_dim(&dim);
    REQUIRE( rc == 0 );

    REQUIRE( dim.data_type == UDA_TYPE_INT );
    REQUIRE( dim.dim_n == 100 );
    REQUIRE( dim.compressed == 0 );

    int* data = reinterpret_cast<int*>(dim.dim);
    REQUIRE( data[0] == 0 );
    REQUIRE( data[1] == 1 );
    REQUIRE( data[99] == 99 );
}

TEST_CASE( "Decompress float dim", "[compress-dim]" )
{
    uda::client_server::Dims dim{};
    dim.compressed = 1;
    dim.data_type = UDA_TYPE_FLOAT;
    dim.dim_n = 100;
    dim.method = 0;
    dim.dim0 = 0.0f;
    dim.diff = 0.1f;

    auto rc = uda::client_server::uncompress_dim(&dim);
    REQUIRE( rc == 0 );

    REQUIRE( dim.data_type == UDA_TYPE_FLOAT );
    REQUIRE( dim.dim_n == 100 );
    REQUIRE( dim.compressed == 0 );

    float* data = reinterpret_cast<float*>(dim.dim);
    REQUIRE_THAT( data[0], Catch::Matchers::WithinRel(0.0f) );
    REQUIRE_THAT( data[1], Catch::Matchers::WithinRel(0.1f) );
    REQUIRE_THAT( data[99], Catch::Matchers::WithinRel(9.9f) );
}