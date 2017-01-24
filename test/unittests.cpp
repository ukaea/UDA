#if 0
#!/bin/bash
g++ unittests.cpp -g -O0 -gdwarf-3 -o test \
    -I$HOME/itmwork/IdamInstall/include/idam -I$HOME/itmwork/IdamInstall/include/idam/c++ \
    -I$BOOST_HOME/include \
    -Wl,-rpath,$HOME/itmwork/IdamInstall/lib -L$HOME/itmwork/IdamInstall/lib \
    -Wl,-rpath,$HOME/lib -L$HOME/lib \
    -lidamcpp -lidam64 \
    -DHOME=$HOME
exit 0
#endif

#include "Idam.hpp"
#include <typeinfo>
#include <iostream>
#include <fstream>

#define CATCH_CONFIG_RUNNER
#include "catch.hpp"

#define QUOTE_(X) #X
#define QUOTE(X) QUOTE_(X)

TEST_CASE( "Fetch size of bpol_probe", "[magnetics]" )
{
    Idam::Client client;

    const Idam::Result& result = client.get("tore::read(element=magnetics/bpol_probe/Shape_of, shot=43970, indices=-1)", "");
    
    Idam::Data * data = result.data();
    REQUIRE( data != NULL );

    Idam::Scalar * scalar = dynamic_cast<Idam::Scalar*>(data);
    REQUIRE( scalar != NULL );

    int value = scalar->as<int>();
    
    REQUIRE( value == 110 );
}

TEST_CASE( "Fetch size of flux_loop position node", "[magnetics]" )
{
    Idam::Client client;

    const Idam::Result& result = client.get("tore::read(element=magnetics/flux_loop/#/position/Shape_of, shot=43970, indices=1)", "");
    
    Idam::Data * data = result.data();
    REQUIRE( data != NULL );

    Idam::Scalar * scalar = dynamic_cast<Idam::Scalar*>(data);
    REQUIRE( scalar != NULL );

    int value = scalar->as<int>();
    
    REQUIRE( value == 1 );
}

TEST_CASE( "Fetch bpol_probe field signal", "[magnetics]" )
{ 
    Idam::Client client;

    const Idam::Result& result = client.get("tore::read(element=magnetics/bpol_probe/#/field, shot=43970, indices=1)", "");

    Idam::Data * data = result.data();
    REQUIRE( data != NULL );

    REQUIRE( data->size() == 6923 );

    Idam::Array * arr = dynamic_cast<Idam::Array*>(data);
    REQUIRE( arr != NULL );
    
    Idam::Dim time = arr->dims().at(0);
    std::vector<float> values = arr->as<float>();
   
    REQUIRE( time.at<float>(0) == Approx(-30.4438) );
    REQUIRE( values.at(0) == Approx(0) );
    
    std::ofstream out("temp.csv");
    for (int i = 0; i < arr->as<float>().size(); ++i) {
    	out << time.at<float>(i) << "," << values.at(i) << "\n"; 
    }
}

TEST_CASE( "Fetch bpol_probe poloidal_angle", "[magnetics]" )
{ 
    Idam::Client client;

    const Idam::Result& result = client.get("tore::read(element=magnetics/bpol_probe/#/poloidal_angle, shot=43970, indices=8)", "");
   
    Idam::Data * data = result.data();
    REQUIRE( data != NULL );

    Idam::Scalar * scalar = dynamic_cast<Idam::Scalar*>(data);
    REQUIRE( scalar != NULL );
    
    float value = scalar->as<float>();

    REQUIRE( value == Approx(-0.75) );
}

TEST_CASE( "Fetch flux_loop position", "[magnetics]" )
{ 
    Idam::Client client;

    const Idam::Result& result = client.get("tore::read(element=magnetics/flux_loop/#/position/#/r, shot=43970, indices=5;1)", "");
   
    Idam::Data * data = result.data();
    REQUIRE( data != NULL );

    Idam::Scalar * scalar = dynamic_cast<Idam::Scalar*>(data);
    REQUIRE( scalar != NULL );
    
    float value = scalar->as<float>();

    REQUIRE( value == Approx(2.937) );
}

TEST_CASE( "Fetch bpol_probe name", "[magnetics]" )
{
    Idam::Client client;

    const Idam::Result& result = client.get("tore::read(element=magnetics/bpol_probe/#/name, shot=43970, indices=110)", "");

    Idam::Data * data = result.data();
    REQUIRE( data != NULL );

    const Idam::String * str = dynamic_cast<const Idam::String*>(data);
    REQUIRE( str != NULL );

    REQUIRE( str->str() == "Btor008" );
}

int main(int argc, const char** argv)
{
    Idam::Client::setProperty(Idam::PROP_DEBUG, true);
    Idam::Client::setProperty(Idam::PROP_VERBOSE, true);

    Idam::Client::setServerHostName("localhost");
    Idam::Client::setServerPort(56565);
   
    int result = Catch::Session().run(argc, argv);

    return result;
}

