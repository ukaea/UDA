#include <numeric>
#include <algorithm>
#include <catch2/catch_test_macros.hpp>
#include <catch2/matchers/catch_matchers_floating_point.hpp>

#include "clientserver/parse_operation.h"

TEST_CASE( "Parse simple range", "[parse-operation]" )
{
    std::vector<uda::client_server::UdaError> error_stack;
    uda::client_server::Subset subset;

    subset.n_bound = 1;
    strcpy(subset.operation[0], "0:1");

    int rc = parse_operation(error_stack, &subset);
    REQUIRE( rc == 0 );
    REQUIRE( error_stack.empty() );

    REQUIRE( subset.n_bound == 1 );

    REQUIRE( subset.lbindex[0].init == true );
    REQUIRE( subset.lbindex[0].value == 0 );

    REQUIRE( subset.ubindex[0].init == true );
    REQUIRE( subset.ubindex[0].value == 1 );

    REQUIRE( subset.stride[0].init == false );
}

TEST_CASE( "Parse range with stride", "[parse-operation]" )
{
    std::vector<uda::client_server::UdaError> error_stack;
    uda::client_server::Subset subset;

    subset.n_bound = 1;
    strcpy(subset.operation[0], "0:1:2");

    int rc = parse_operation(error_stack, &subset);
    REQUIRE( rc == 0 );
    REQUIRE( error_stack.empty() );

    REQUIRE( subset.n_bound == 1 );

    REQUIRE( subset.lbindex[0].init == true );
    REQUIRE( subset.lbindex[0].value == 0 );

    REQUIRE( subset.ubindex[0].init == true );
    REQUIRE( subset.ubindex[0].value == 1 );

    REQUIRE( subset.stride[0].init == true );
    REQUIRE( subset.stride[0].value == 2 );
}