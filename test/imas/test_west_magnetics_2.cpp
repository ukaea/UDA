#if 0
#!/bin/bash
g++ test_west_magnetics_2.cpp -g -std=c++11 -O0 -gdwarf-3 -o test -DHOME=$HOME -I$HOME/iter/uda/source -I$HOME/iter/uda/source/wrappers \
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
//#define SHOT_NUM "51827" // WEST
#define SHOT_NUM "51905" // WEST

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

	/*const uda::Result& sigtime = client.get("imas::get(idx=0, group='magnetics', variable='flux_loop/1/flux/time', expName='WEST', type=double, rank=1, shot=" SHOT_NUM ", )", "");
	const uda::Data * datatime = sigtime.data();
	const uda::Array* arrtime = dynamic_cast<const uda::Array*>(datatime);

	std::cout << "first values for flux_loop/1/flux/time : ";
	for (int j = 0; j < 10; ++j) {
		std::cout << arrtime->as<double>().at(j) << " ";
	}
	std::cout << "..." << std::endl;*/

	const uda::Result& sigts = client.get("imas::get(idx=0, group='magnetics', variable='flux_loop/1/flux/data', expName='WEST', type=double, rank=1, shot=" SHOT_NUM ", )", "");
	const uda::Data * datats = sigts.data();
	const uda::Array* arrts = dynamic_cast<const uda::Array*>(datats);

	std::cout << "first values for flux_loop/1/flux/data : ";
	for (int j = 0; j < 10; ++j) {
		std::cout << arrts->as<double>().at(j) << " ";
	}
	std::cout << "..." << std::endl;

	/*const uda::Result& sigtime2 = client.get("imas::get(idx=0, group='magnetics', variable='flux_loop/2/flux/time', expName='WEST', type=double, rank=1, shot=" SHOT_NUM ", )", "");
	const uda::Data * datatime2 = sigtime2.data();
	const uda::Array* arrtime2 = dynamic_cast<const uda::Array*>(datatime2);

	std::cout << "first values for flux_loop/2/flux/time : ";
	for (int j = 0; j < 10; ++j) {
		std::cout << arrtime2->as<double>().at(j) << " ";
	}
	std::cout << "..." << std::endl;*/

	const uda::Result& sigts2 = client.get("imas::get(idx=0, group='magnetics', variable='flux_loop/2/flux/data', expName='WEST', type=double, rank=1, shot=" SHOT_NUM ", )", "");
	const uda::Data * datats2 = sigts2.data();
	const uda::Array* arrts2 = dynamic_cast<const uda::Array*>(datats2);

	std::cout << "first values for flux_loop/2/flux/data : ";
	for (int j = 0; j < 10; ++j) {
		std::cout << arrts2->as<double>().at(j) << " ";
	}
	std::cout << "..." << std::endl;

	/*const uda::Result& sigtime3 = client.get("imas::get(idx=0, group='magnetics', variable='flux_loop/3/flux/time', expName='WEST', type=double, rank=1, shot=" SHOT_NUM ", )", "");
	const uda::Data * datatime3 = sigtime3.data();
	const uda::Array* arrtime3 = dynamic_cast<const uda::Array*>(datatime3);

	std::cout << "first values for flux_loop/3/flux/time : ";
	for (int j = 0; j < 10; ++j) {
		std::cout << arrtime3->as<double>().at(j) << " ";
	}
	std::cout << "..." << std::endl;*/

	const uda::Result& sigts3 = client.get("imas::get(idx=0, group='magnetics', variable='flux_loop/3/flux/data', expName='WEST', type=double, rank=1, shot=" SHOT_NUM ", )", "");
	const uda::Data * datats3 = sigts3.data();
	const uda::Array* arrts3 = dynamic_cast<const uda::Array*>(datats3);

	std::cout << "first values for flux_loop/3/flux/data : ";
	for (int j = 0; j < 10; ++j) {
		std::cout << arrts3->as<double>().at(j) << " ";
	}
	std::cout << "..." << std::endl;

	const uda::Result& tor_angle = client.get("imas::get(idx=0, group='magnetics', variable='bpol_probe/1/toroidal_angle', expName='WEST', type=float, rank=0, shot=" SHOT_NUM ", )", "");
	const uda::Scalar* v_tor_angle = dynamic_cast<const uda::Scalar*>(tor_angle.data());
	std::cout << "bpol_probe/1/toroidal_angle: " << v_tor_angle->as<float>() << std::endl;

	const uda::Result& tor_angle60 = client.get("imas::get(idx=0, group='magnetics', variable='bpol_probe/60/toroidal_angle', expName='WEST', type=float, rank=0, shot=" SHOT_NUM ", )", "");
	const uda::Scalar* v_tor_angle60 = dynamic_cast<const uda::Scalar*>(tor_angle60.data());
	std::cout << "bpol_probe/60/toroidal_angle: " << v_tor_angle60->as<float>() << std::endl;

	const uda::Result& pol_angle60 = client.get("imas::get(idx=0, group='magnetics', variable='bpol_probe/60/poloidal_angle', expName='WEST', type=float, rank=0, shot=" SHOT_NUM ", )", "");
	const uda::Scalar* v_pol_angle60 = dynamic_cast<const uda::Scalar*>(pol_angle60.data());
	std::cout << "bpol_probe/60/poloidal_angle: " << v_pol_angle60->as<float>() << std::endl;

	/*for (int i = 0; i < 110; i++) {
		char* requestPart = "imas::get(idx=0, group='magnetics', variable='";
		char* requestPart = "', expName='WEST', type=float, rank=0, shot=";
		char* requestVaiable = strcat()
		const uda::Result& tor_anglei = client.get("imas::get(idx=0, group='magnetics', variable='bpol_probe/60/toroidal_angle', expName='WEST', type=float, rank=0, shot=" SHOT_NUM ", )", "");
		const uda::Scalar* v_tor_angle60 = dynamic_cast<const uda::Scalar*>(tor_angle60.data());
		std::cout << "bpol_probe/60/toroidal_angle: " << v_tor_angle60->as<float>() << std::endl;
	}*/

	const uda::Result& bpol_probe_posR = client.get("imas::get(idx=0, group='magnetics', variable='bpol_probe/20/position/r', expName='WEST', type=double, rank=0, shot=" SHOT_NUM ", )", "");
	const uda::Scalar* scalar_bpol_probe_posR = dynamic_cast<const uda::Scalar*>(bpol_probe_posR.data());
	std::cout << "bpol_probe/20/position/r: " << scalar_bpol_probe_posR->as<double>() << std::endl;

	const uda::Result& mag = client.get("imas::get(idx=0, group='magnetics', variable='bpol_probe/1/field/data', expName='WEST', type=double, rank=1, shot=" SHOT_NUM ", )", "");
	const uda::Data * data_mag = mag.data();
	const uda::Array* arr_data_mag = dynamic_cast<const uda::Array*>(data_mag);

	std::cout << "first values for bpol_probe/1/field : ";
	for (int j = 0; j < 10; ++j) {
		std::cout << arr_data_mag->as<double>().at(j) << " ";
	}
	std::cout << "..." << std::endl;

	const uda::Result& mag2 = client.get("imas::get(idx=0, group='magnetics', variable='bpol_probe/2/field/data', expName='WEST', type=double, rank=1, shot=" SHOT_NUM ", )", "");
	const uda::Data * data_mag2 = mag2.data();
	const uda::Array* arr_data_mag2 = dynamic_cast<const uda::Array*>(data_mag2);

	std::cout << "first values for bpol_probe/2/field : ";
	for (int j = 0; j < 10; ++j) {
		std::cout << arr_data_mag2->as<double>().at(j) << " ";
	}
	std::cout << "..." << std::endl;

	const uda::Result& mag55 = client.get("imas::get(idx=0, group='magnetics', variable='bpol_probe/55/field/data', expName='WEST', type=double, rank=1, shot=" SHOT_NUM ", )", "");
	const uda::Data * data_mag55 = mag55.data();
	const uda::Array* arr_data_mag55 = dynamic_cast<const uda::Array*>(data_mag55);

	std::cout << "first values for bpol_probe/55/field : ";
	for (int j = 0; j < 10; ++j) {
		std::cout << arr_data_mag55->as<double>().at(j) << " ";
	}
	std::cout << "..." << std::endl;

	const uda::Result& mag56 = client.get("imas::get(idx=0, group='magnetics', variable='bpol_probe/56/field/data', expName='WEST', type=double, rank=1, shot=" SHOT_NUM ", )", "");
	const uda::Data * data_mag56 = mag56.data();
	const uda::Array* arr_data_mag56 = dynamic_cast<const uda::Array*>(data_mag56);

	std::cout << "first values for bpol_probe/56/field : ";
	for (int j = 0; j < 10; ++j) {
		std::cout << arr_data_mag56->as<double>().at(j) << " ";
	}
	std::cout << "..." << std::endl;

	const uda::Result& mag110 = client.get("imas::get(idx=0, group='magnetics', variable='bpol_probe/110/field/data', expName='WEST', type=double, rank=1, shot=" SHOT_NUM ", )", "");
	const uda::Data * data_mag110 = mag110.data();
	const uda::Array* arr_data_mag110 = dynamic_cast<const uda::Array*>(data_mag110);

	std::cout << "first values for bpol_probe/110/field : ";
	for (int j = 0; j < 10; ++j) {
		std::cout << arr_data_mag110->as<double>().at(j) << " ";
	}
	std::cout << "..." << std::endl;

	const uda::Result& mag111 = client.get("imas::get(idx=0, group='magnetics', variable='bpol_probe/111/field/data', expName='WEST', type=double, rank=1, shot=" SHOT_NUM ", )", "");
	const uda::Data * data_mag111 = mag111.data();
	const uda::Array* arr_data_mag111 = dynamic_cast<const uda::Array*>(data_mag111);

	std::cout << "first values for bpol_probe/111/field : ";
	for (int j = 0; j < 10; ++j) {
		std::cout << arr_data_mag111->as<double>().at(j) << " ";
	}
	std::cout << "..." << std::endl;


	return 0;
}

