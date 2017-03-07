#define CATCH_CONFIG_MAIN
#include "catch.hpp"

#include <client/udaPutAPI.h>

TEST_CASE( "Test cpp putdata functionality", "[putdata]" ) {

#ifdef FATCLIENT
#  include "setupEnvironment.inc"
#endif

    std::stringstream ss;
    ss << "putdata::open(/create"
            << ", filename='test.nc'"
            << ", conventions='Fusion-1.0'"
            << ", class='Analysed'"
            << ", title='Test #1'"
            << ", shot=123456"
            << ", pass=789"
            << ", comment='Comment for test file'"
            << ", code='test_putdata.cpp'"
            << ", version=1"
            << ")";

    idamPutAPI("putdata::open(create, filename=, conventions=, class=, title=, shot=, pass=)", NULL);
}