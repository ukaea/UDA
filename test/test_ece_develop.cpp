#if 0
#!/bin/bash
g++ test_ece_develop.cpp -g -O0 -gdwarf-3 -o test -DHOME=$HOME -I$HOME/iter/uda/source -I$HOME/iter/uda/source/wrappers \
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
#define SHOT_NUM "50355" // WEST

#define SHOT_NUM_10000 "10000" // WEST
#define SHOT_NUM_33456 "33456" // WEST

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


	const uda::Result& freq = client.get("imas::get(idx=0, group='ece', variable='channel/1/frequency', expName='WEST', type=float, rank=0, shot=" SHOT_NUM ", )", "");
	const uda::Scalar* scal_freq = dynamic_cast<const uda::Scalar*>(freq.data());
	std::cout << "ece/channel/1/frequency for shot 50355: " << scal_freq->as<float>() << std::endl;

	const uda::Result& freq2 = client.get("imas::get(idx=0, group='ece', variable='channel/1/frequency', expName='WEST', type=float, rank=0, shot=" SHOT_NUM_10000 ", )", "");
	scal_freq = dynamic_cast<const uda::Scalar*>(freq2.data());
	std::cout << "ece/channel/1/frequency for shot 10000: " << scal_freq->as<float>() << std::endl;

	const uda::Result& freq3 = client.get("imas::get(idx=0, group='ece', variable='channel/2/frequency', expName='WEST', type=float, rank=0, shot=" SHOT_NUM ", )", "");
	scal_freq = dynamic_cast<const uda::Scalar*>(freq3.data());
	std::cout << "ece/channel/2/frequency for shot 50355: " << scal_freq->as<float>() << std::endl;

	const uda::Result& freq4 = client.get("imas::get(idx=0, group='ece', variable='channel/2/frequency', expName='WEST', type=float, rank=0, shot=" SHOT_NUM_10000 ", )", "");
	scal_freq = dynamic_cast<const uda::Scalar*>(freq4.data());
	std::cout << "ece/channel/2/frequency for shot 10000: " << scal_freq->as<float>() << std::endl;

	const uda::Result& name = client.get("imas::get(idx=0, group='ece', variable='channel/1/name', expName='WEST', type=string, rank=0, shot=" SHOT_NUM ", )", "");
	const uda::Data * data = name.data();
	const uda::String* s = dynamic_cast<const uda::String*>(data);

	std::cout << "channel/1/name : ";
	std::cout << s->str();
	std::cout << "\n" << std::endl;

	const uda::Result& name2 = client.get("imas::get(idx=0, group='ece', variable='channel/10/name', expName='WEST', type=string, rank=0, shot=" SHOT_NUM ", )", "");
	const uda::Data * data2 = name2.data();
	const uda::String* s2 = dynamic_cast<const uda::String*>(data2);

	std::cout << "channel/10/name : ";
	std::cout << s2->str();
	std::cout << "\n" << std::endl;

	const uda::Result& sig = client.get("imas::get(idx=0, group='ece', variable='channel/1/t_e/data', expName='WEST', type=double, rank=1, shot=" SHOT_NUM_33456 ", )", "");
	data = sig.data();
	const uda::Array* arr = dynamic_cast<const uda::Array*>(data);

	std::cout << "first values for channel/1/t_e/data : ";
	for (int j = 0; j < 10; ++j) {
		std::cout << arr->as<double>().at(j) << " ";
	}
	std::cout << "..." << std::endl;
	
	const uda::Result& sigtime = client.get("imas::get(idx=0, group='ece', variable='channel/1/t_e/time', expName='WEST', type=double, rank=1, shot=" SHOT_NUM_33456 ", )", "");
	data = sigtime.data();
	const uda::Array* arrtime = dynamic_cast<const uda::Array*>(data);

	std::cout << "first values for channel/1/t_e/time : ";
	for (int j = 0; j < 10; ++j) {
		std::cout << arrtime->as<double>().at(j) << " ";
	}
	std::cout << "..." << std::endl;



	return 0;
}

