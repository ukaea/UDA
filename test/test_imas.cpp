#if 0
#!/bin/bash
g++ test_imas.cpp -g -O0 -gdwarf-3 -o test2 -DHOME=$HOME -I$HOME/iter/uda/source -I$HOME/iter/uda/source/wrappers \
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
#define SHOT_NUM "50080" // WEST

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
	
	/*const uda::Result& shape_of = client.get("imas::get(idx=0, group='magnetics', variable='flux_loop/Shape_of', expName='WEST', type=int, rank=0, shot=" SHOT_NUM ", )", "");
	const uda::Scalar* v_shape_of = dynamic_cast<const uda::Scalar*>(shape_of.data());
	std::cout << "flux_loop/Shape_of: " << v_shape_of->as<int>() << std::endl;
	
	const uda::Result& shape_of1 = client.get("imas::get(idx=0, group='interfero_polarimeter', variable='channel/Shape_of', expName='WEST', type=int, rank=0, shot=" SHOT_NUM ", )", "");
	const uda::Scalar* v_shape_of1 = dynamic_cast<const uda::Scalar*>(shape_of1.data());
	std::cout << "channel/Shape_of: " << v_shape_of1->as<int>() << std::endl;
	
	const uda::Result& n_e_line = client.get("imas::get(idx=0, group='interfero_polarimeter', variable='channel/3/n_e_line/data', expName='WEST', type=double, rank=1, shot=" SHOT_NUM ", )", "");
	const uda::Data * data_n_e_line = n_e_line.data();
	const uda::Array* arr_data_n_e_line = dynamic_cast<const uda::Array*>(data_n_e_line);

	std::cout << "some values for channel/3/n_e_line/data from 27255 to 27265: ";
	for (int j = 27255; j < 27265; ++j) {
		std::cout << arr_data_n_e_line->as<double>().at(j) << " ";
	}
	std::cout << "..." << std::endl;
	
	const uda::Result& n_e_line4 = client.get("imas::get(idx=0, group='interfero_polarimeter', variable='channel/4/n_e_line/data', expName='WEST', type=double, rank=1, shot=" SHOT_NUM ", )", "");
	const uda::Data * data_n_e_line4 = n_e_line4.data();
	const uda::Array* arr_data_n_e_line4 = dynamic_cast<const uda::Array*>(data_n_e_line4);

	std::cout << "some values for channel/4/n_e_line/data from 27255 to 27265: ";
	for (int j = 27255; j < 27265; ++j) {
		std::cout << arr_data_n_e_line4->as<double>().at(j) << " ";
	}
	std::cout << "..." << std::endl;*/
	
	const uda::Result& validity = client.get("imas::get(idx=0, group='interfero_polarimeter', variable='channel/1/validity', expName='WEST', type=int, rank=0, shot=" SHOT_NUM ", )", "");
	const uda::Scalar* v_validity = dynamic_cast<const uda::Scalar*>(validity.data());
	std::cout << "channel/1/validity: " << v_validity->as<int>() << std::endl;
	
	const uda::Result& validity2 = client.get("imas::get(idx=0, group='interfero_polarimeter', variable='channel/2/validity', expName='WEST', type=int, rank=0, shot=" SHOT_NUM ", )", "");
	const uda::Scalar* v_validity2 = dynamic_cast<const uda::Scalar*>(validity2.data());
	std::cout << "channel/2/validity: " << v_validity2->as<int>() << std::endl;
	
	const uda::Result& validity3 = client.get("imas::get(idx=0, group='interfero_polarimeter', variable='channel/3/validity', expName='WEST', type=int, rank=0, shot=" SHOT_NUM ", )", "");
	const uda::Scalar* v_validity3 = dynamic_cast<const uda::Scalar*>(validity3.data());
	std::cout << "channel/3/validity: " << v_validity3->as<int>() << std::endl;
	
	return 0;
}

