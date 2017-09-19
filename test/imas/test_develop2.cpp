#if 0
#!/bin/bash
g++ test_develop2.cpp -g -O0 -gdwarf-3 -o test -DHOME=$HOME -I$HOME/iter/uda/source -I$HOME/iter/uda/source/wrappers \
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
#define SHOT_NUM "51371" // WEST

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

/*
	const uda::Result& sigts = client.get("imas::get(idx=0, group='magnetics', variable='flux_loop/1/flux/data', expName='WEST', type=double, rank=1, shot=" SHOT_NUM ", )", "");
	const uda::Data * datats = sigts.data();
	const uda::Array* arrts = dynamic_cast<const uda::Array*>(datats);

	std::cout << "first values for flux_loop/1/flux/data : ";
	for (int j = 0; j < 10; ++j) {
		std::cout << arrts->as<double>().at(j) << " ";
	}
	std::cout << "..." << std::endl;

	const uda::Result& sigtime = client.get("imas::get(idx=0, group='magnetics', variable='flux_loop/1/flux/time', expName='WEST', type=double, rank=1, shot=" SHOT_NUM ", )", "");
	const uda::Data * datatime = sigtime.data();
	const uda::Array* arrtime = dynamic_cast<const uda::Array*>(datatime);

	std::cout << "first values for flux_loop/1/flux/time : ";
	for (int j = 0; j < 10; ++j) {
		std::cout << arrtime->as<double>().at(j) << " ";
	}
	std::cout << "..." << std::endl;
*/
	/*const uda::Result& current = client.get("imas::get(idx=0, group='pf_passive', variable='loop/1/current', expName='WEST', type=double, rank=1, shot=" SHOT_NUM ", )", "");
	const uda::Data * current_data = current.data();
	const uda::Array* arr_current_data = dynamic_cast<const uda::Array*>(current_data);

	std::cout << "values for loop/1/current from 200 to 210: ";
	for (int j = 199; j < 209; ++j) {
		std::cout << arr_current_data->as<double>().at(j) << " ";
	}
	std::cout << "..." << std::endl;*/

	/*const uda::Result& sigtime2 = client.get("imas::get(idx=0, group='magnetics', variable='flux_loop/1/flux/time', expName='WEST', type=double, rank=1, shot=" SHOT_NUM ", )", "");
	const uda::Data * datatime2 = sigtime2.data();
	const uda::Array* arrtime2 = dynamic_cast<const uda::Array*>(datatime2);

	std::cout << "first values for flux_loop/1/flux/time : ";
	for (int j = 0; j < 10; ++j) {
		std::cout << arrtime2->as<double>().at(j) << " ";
	}
	std::cout << "..." << std::endl;

	const uda::Result& sigtime3 = client.get("imas::get(idx=0, group='magnetics', variable='flux_loop/1/flux/time', expName='WEST', type=double, rank=1, shot=" SHOT_NUM ", )", "");
	const uda::Data * datatime3 = sigtime3.data();
	const uda::Array* arrtime3 = dynamic_cast<const uda::Array*>(datatime3);

	std::cout << "first values for flux_loop/1/flux/time : ";
	for (int j = 0; j < 10; ++j) {
		std::cout << arrtime3->as<double>().at(j) << " ";
	}
	std::cout << "..." << std::endl;*/

	const uda::Result& tor_angle = client.get("imas::get(idx=0, group='magnetics', variable='bpol_probe/1/toroidal_angle', expName='WEST', type=float, rank=0, shot=" SHOT_NUM ", )", "");
	const uda::Scalar* v_tor_angle = dynamic_cast<const uda::Scalar*>(tor_angle.data());
	std::cout << "bpol_probe/1/toroidal_angle: " << v_tor_angle->as<float>() << std::endl;



	return 0;
}

