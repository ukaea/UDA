#if 0
#!/bin/bash
g++ test_imas4.cpp -g -O0 -gdwarf-3 -o test -DHOME=$HOME -I$HOME/iter/uda/source -I$HOME/iter/uda/source/wrappers \
-L$HOME/iter/uda/lib -Wl,-rpath,$HOME/iter/uda/lib  -luda_cpp -lssl -lcrypto -lxml2
exit 0
#endif

#include <c++/UDA.hpp>
#include <typeinfo>
#include <iostream>
#include <sstream>
#include <fstream>

#define QUOTE_(X) #X
#define QUOTE(X) QUOTE_(X)
#define SHOT_NUM_TORE_SUPRA "43979" // WEST
#define SHOT_NUM "50355" // WEST

int main() {
	setenv("UDA_PLUGIN_DIR", QUOTE(HOME) "/iter/uda/etc/plugins", 1);
	setenv("UDA_PLUGIN_CONFIG", QUOTE(HOME) "/iter/uda/test/idamTest.conf", 1);
	setenv("UDA_SARRAY_CONFIG", QUOTE(HOME) "/iter/uda/bin/plugins/idamgenstruct.conf", 1);
	setenv("UDA_WEST_MAPPING_FILE", QUOTE(HOME) "/iter/uda/source/plugins/west/WEST_mappings/IDAM_mapping.xml", 1);
	setenv("UDA_WEST_MAPPING_FILE_DIRECTORY", QUOTE(HOME) "/iter/uda/source/plugins/west/WEST_mappings", 1);
	setenv("UDA_LOG", QUOTE(HOME) "/iter/uda/", 1);
	setenv("UDA_LOG_MODE", "a", 1);
	setenv("UDA_LOG_LEVEL", "DEBUG", 1);
	setenv("UDA_DEBUG_APPEND", "a", 1);

	uda::Client::setProperty(uda::PROP_DEBUG, true);
	uda::Client::setProperty(uda::PROP_VERBOSE, true);

	uda::Client::setServerHostName("localhost");
	uda::Client::setServerPort(56565);

	uda::Client client;
	
//	const uda::Result& posR = client.get("imas::get(idx=0, group='magnetics', variable='bpol_probe/1/position/r', expName='WEST', type=double, rank=0, shot=" SHOT_NUM ", )", "");
//	const uda::Data * dataR = posR.data();
//	const uda::Scalar* arDataR = dynamic_cast<const uda::Scalar*>(dataR);
//	std::cout << "bpol_probe/1/position/r: " << arDataR->as<double>() << std::endl;
//
//	const uda::Result& posR2 = client.get("imas::get(idx=0, group='magnetics', variable='bpol_probe/2/position/r', expName='WEST', type=double, rank=0, shot=" SHOT_NUM ", )", "");
//	const uda::Data * dataR2 = posR2.data();
//	const uda::Scalar* arDataR2 = dynamic_cast<const uda::Scalar*>(dataR2);
//	std::cout << "bpol_probe/2/position/r: " << arDataR2->as<double>() << std::endl;
//
//	const uda::Result& posR60 = client.get("imas::get(idx=0, group='magnetics', variable='bpol_probe/60/position/r', expName='WEST', type=double, rank=0, shot=" SHOT_NUM ", )", "");
//    const uda::Data * dataR60 = posR60.data();
//	const uda::Scalar* arDataR60 = dynamic_cast<const uda::Scalar*>(dataR60);
//	std::cout << "bpol_probe/60/position/r: " << arDataR60->as<double>() << std::endl;
//
//	const uda::Result& shape_of = client.get("imas::get(idx=0, group='magnetics', variable='bpol_probe/Shape_of', expName='WEST', type=int, rank=0, shot=" SHOT_NUM ", )", "");
//	const uda::Scalar* v_shape_of = dynamic_cast<const uda::Scalar*>(shape_of.data());
//	std::cout << "bpol_probe/Shape_of: " << v_shape_of->as<int>() << std::endl;

//	const uda::Result& tor_angle110 = client.get("imas::get(idx=0, group='magnetics', variable='bpol_probe/110/toroidal_angle', expName='WEST', type=float, rank=0, shot=" SHOT_NUM ", )", "");
//	const uda::Scalar* v_tor_angle110 = dynamic_cast<const uda::Scalar*>(tor_angle110.data());
//	std::cout << "bpol_probe/110/toroidal_angle: " << v_tor_angle110->as<float>() << std::endl;
//
//	const uda::Result& tor_angle = client.get("imas::get(idx=0, group='magnetics', variable='bpol_probe/111/toroidal_angle', expName='WEST', type=float, rank=0, shot=" SHOT_NUM ", )", "");
//	const uda::Scalar* v_tor_angle = dynamic_cast<const uda::Scalar*>(tor_angle.data());
//	std::cout << "bpol_probe/111/toroidal_angle: " << v_tor_angle->as<float>() << std::endl;

//	const uda::Result& tor_angle112 = client.get("imas::get(idx=0, group='magnetics', variable='bpol_probe/112/toroidal_angle', expName='WEST', type=float, rank=0, shot=" SHOT_NUM ", )", "");
//	const uda::Scalar* v_tor_angle112 = dynamic_cast<const uda::Scalar*>(tor_angle112.data());
//	std::cout << "bpol_probe/112/toroidal_angle: " << v_tor_angle112->as<float>() << std::endl;
//
//	const uda::Result& tor_angle129 = client.get("imas::get(idx=0, group='magnetics', variable='bpol_probe/129/toroidal_angle', expName='WEST', type=float, rank=0, shot=" SHOT_NUM ", )", "");
//	const uda::Scalar* v_tor_angle129 = dynamic_cast<const uda::Scalar*>(tor_angle129.data());
//	std::cout << "bpol_probe/129/toroidal_angle: " << v_tor_angle129->as<float>() << std::endl;
//
//	const uda::Result& pol_angle1 = client.get("imas::get(idx=0, group='magnetics', variable='bpol_probe/1/poloidal_angle', expName='WEST', type=float, rank=0, shot=" SHOT_NUM ", )", "");
//	const uda::Scalar* v_pol_angle1 = dynamic_cast<const uda::Scalar*>(pol_angle1.data());
//    std::cout << "bpol_probe/1/pol_angle: " << v_pol_angle1->as<float>() << std::endl;
//
//    const uda::Result& pol_angle100 = client.get("imas::get(idx=0, group='magnetics', variable='bpol_probe/100/poloidal_angle', expName='WEST', type=float, rank=0, shot=" SHOT_NUM ", )", "");
//    const uda::Scalar* v_pol_angle100 = dynamic_cast<const uda::Scalar*>(pol_angle100.data());
//    std::cout << "bpol_probe/100/pol_angle: " << v_pol_angle100->as<float>() << std::endl;

//    const uda::Result& pol_angle110 = client.get("imas::get(idx=0, group='magnetics', variable='bpol_probe/110/poloidal_angle', expName='WEST', type=float, rank=0, shot=" SHOT_NUM ", )", "");
//    const uda::Scalar* v_pol_angle110 = dynamic_cast<const uda::Scalar*>(pol_angle110.data());
//    std::cout << "bpol_probe/110/pol_angle: " << v_pol_angle110->as<float>() << std::endl;
//
//    const uda::Result& pol_angle111 = client.get("imas::get(idx=0, group='magnetics', variable='bpol_probe/111/poloidal_angle', expName='WEST', type=float, rank=0, shot=" SHOT_NUM ", )", "");
//    const uda::Scalar* v_pol_angle111 = dynamic_cast<const uda::Scalar*>(pol_angle111.data());
//    std::cout << "bpol_probe/111/pol_angle: " << v_pol_angle111->as<float>() << std::endl;
//
//    const uda::Result& pol_angle112 = client.get("imas::get(idx=0, group='magnetics', variable='bpol_probe/112/poloidal_angle', expName='WEST', type=float, rank=0, shot=" SHOT_NUM ", )", "");
//    const uda::Scalar* v_pol_angle112 = dynamic_cast<const uda::Scalar*>(pol_angle112.data());
//    std::cout << "bpol_probe/112/pol_angle: " << v_pol_angle112->as<float>() << std::endl;
//
//    const uda::Result& pol_angle115 = client.get("imas::get(idx=0, group='magnetics', variable='bpol_probe/115/poloidal_angle', expName='WEST', type=float, rank=0, shot=" SHOT_NUM ", )", "");
//    const uda::Scalar* v_pol_angle115 = dynamic_cast<const uda::Scalar*>(pol_angle115.data());
//    std::cout << "bpol_probe/115/pol_angle: " << v_pol_angle115->as<float>() << std::endl;

	const uda::Result& pol_angle130 = client.get("imas::get(idx=0, group='magnetics', variable='bpol_probe/130/poloidal_angle', expName='WEST', type=float, rank=0, shot=" SHOT_NUM ", )", "");
    const uda::Scalar* v_pol_angle130 = dynamic_cast<const uda::Scalar*>(pol_angle130.data());
	std::cout << "bpol_probe/130/pol_angle: " << v_pol_angle130->as<float>() << std::endl;

	return 0;
}

