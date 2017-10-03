#if 0
#!/bin/bash
g++ test_west_soft_x_rays.cpp -g -std=c++11 -O0 -gdwarf-3 -o test -DHOME=$HOME -I$HOME/iter/uda/source -I$HOME/iter/uda/source/wrappers \
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

	const uda::Result& soft_x_rays_shapeOf = client.get("imas::get(idx=0, group='soft_x_rays', variable='channel/Shape_of', expName='WEST', type=int, rank=0, shot=" SHOT_NUM ", )", "");
	const uda::Scalar* scalar_soft_x_rays_shapeOf = dynamic_cast<const uda::Scalar*>(soft_x_rays_shapeOf.data());
	std::cout << "soft_x_rays/channel/Shape_of: " << scalar_soft_x_rays_shapeOf->as<int>() << std::endl;

	const uda::Result& soft_x_rays_shapeOf2 = client.get("imas::get(idx=0, group='soft_x_rays', variable='channel/1/energy_band/Shape_of', expName='WEST', type=int, rank=0, shot=" SHOT_NUM ", )", "");
	const uda::Scalar* scalar_soft_x_rays_shapeOf2 = dynamic_cast<const uda::Scalar*>(soft_x_rays_shapeOf2.data());
	std::cout << "soft_x_rays/channel/1/energy_band/Shape_of: " << scalar_soft_x_rays_shapeOf2->as<int>() << std::endl;

	const uda::Result& soft_x_rays_R = client.get("imas::get(idx=0, group='soft_x_rays', variable='channel/1/line_of_sight/first_point/r', expName='WEST', type=double, rank=0, shot=" SHOT_NUM ", )", "");
	const uda::Scalar* scalar_soft_x_rays_R = dynamic_cast<const uda::Scalar*>(soft_x_rays_R.data());
	std::cout << "soft_x_rays/channel/1/line_of_sight/first_point/r: " << scalar_soft_x_rays_R->as<double>() << std::endl;

	const uda::Result& soft_x_rays_Z = client.get("imas::get(idx=0, group='soft_x_rays', variable='channel/1/line_of_sight/first_point/z', expName='WEST', type=double, rank=0, shot=" SHOT_NUM ", )", "");
	const uda::Scalar* scalar_soft_x_rays_Z = dynamic_cast<const uda::Scalar*>(soft_x_rays_Z.data());
	std::cout << "soft_x_rays/channel/1/line_of_sight/first_point/z: " << scalar_soft_x_rays_Z->as<double>() << std::endl;

	const uda::Result& lower_bound = client.get("imas::get(idx=0, group='soft_x_rays', variable='channel/1/energy_band/1/lower_bound', expName='WEST', type=double, rank=0, shot=" SHOT_NUM ", )", "");
	const uda::Scalar* scalar_lower_bound = dynamic_cast<const uda::Scalar*>(lower_bound.data());
	std::cout << "soft_x_rays/channel/1/energy_band/1/lower_bound: " << scalar_lower_bound->as<double>() << std::endl;

	const uda::Result& upper_bound = client.get("imas::get(idx=0, group='soft_x_rays', variable='channel/1/energy_band/1/upper_bound', expName='WEST', type=double, rank=0, shot=" SHOT_NUM ", )", "");
	const uda::Scalar* scalar_upper_bound = dynamic_cast<const uda::Scalar*>(upper_bound.data());
	std::cout << "soft_x_rays/channel/1/energy_band/1/upper_bound: " << scalar_upper_bound->as<double>() << std::endl;


	const uda::Result& soft_x_rays = client.get("imas::get(idx=0, group='soft_x_rays', variable='channel/1/power_density/data', expName='WEST', type=double, rank=1, shot=" SHOT_NUM ", )", "");
	const uda::Data * soft_x_rays_data = soft_x_rays.data();
	const uda::Array* soft_x_rays_data_arr = dynamic_cast<const uda::Array*>(soft_x_rays_data);
	std::cout << "first values for soft_x_rays/channel/1/power_density/data: ";
	for (int j = 0; j < 10; ++j) {
		//std::cout << soft_x_rays_data_arr->as<double>().at(j) << " ";
		std::cout << soft_x_rays_data_arr->as<double>().at(j) << " ";
	}
	std::cout << "..." << std::endl;

	const uda::Result& soft_x_rays_time = client.get("imas::get(idx=0, group='soft_x_rays', variable='channel/1/power_density/time', expName='WEST', type=double, rank=1, shot=" SHOT_NUM ", )", "");
	const uda::Data * soft_x_rays_data_time = soft_x_rays_time.data();
	const uda::Array* soft_x_rays_time_arr = dynamic_cast<const uda::Array*>(soft_x_rays_data_time);
	std::cout << "first values for soft_x_rays/channel/1/power_density/time: ";
	for (int j = 0; j < 10; ++j) {
		std::cout << soft_x_rays_time_arr->as<double>().at(j) << " ";
	}
	std::cout << "..." << std::endl;

	return 0;
}

