#if 0
#!/bin/bash
g++ test_west_summary.cpp -g -std=c++11 -O0 -gdwarf-3 -o test -DHOME=$HOME -I$HOME/iter/uda/source -I$HOME/iter/uda/source/wrappers \
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
#define SHOT_NUM "51827" // WEST

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


	const uda::Result& current = client.get("imas::get(idx=0, group='summary', variable='global_quantities/ip/value', expName='WEST', type=double, rank=1, shot=" SHOT_NUM ", )", "");
	const uda::Data * current_data = current.data();
	const uda::Array* arr_current_data = dynamic_cast<const uda::Array*>(current_data);

	std::cout << "values for global_quantities/ip/value from 0 to 50: ";
	for (int j = 0; j < 51; ++j) {
		std::cout << arr_current_data->as<double>().at(j) << " ";
	}
	std::cout << "..." << std::endl;

	const uda::Result& ohm_value = client.get("imas::get(idx=0, group='summary', variable='global_quantities/ohm/value', expName='WEST', type=double, rank=1, shot=" SHOT_NUM ", )", "");
	const uda::Data * ohm_value_data = ohm_value.data();
	const uda::Array* arr_ohm_value_data = dynamic_cast<const uda::Array*>(ohm_value_data);

	std::cout << "values for global_quantities/ohm/value from 0 to 50: ";
	for (int j = 0; j < 51; ++j) {
		std::cout << arr_ohm_value_data->as<double>().at(j) << " ";
	}
	std::cout << "..." << std::endl;

	const uda::Result& vloop_value = client.get("imas::get(idx=0, group='summary', variable='global_quantities/v_loop/value', expName='WEST', type=double, rank=1, shot=" SHOT_NUM ", )", "");
	const uda::Data * vloop_value_data = vloop_value.data();
	const uda::Array* arr_vloop_value_data = dynamic_cast<const uda::Array*>(vloop_value_data);

	std::cout << "values for global_quantities/v_loop/value from 0 to 50: ";
	for (int j = 0; j < 51; ++j) {
		std::cout << arr_vloop_value_data->as<double>().at(j) << " ";
	}
	std::cout << "..." << std::endl;

	const uda::Result& li_value = client.get("imas::get(idx=0, group='summary', variable='global_quantities/li/value', expName='WEST', type=double, rank=1, shot=" SHOT_NUM ", )", "");
	const uda::Data * li_value_data = li_value.data();
	const uda::Array* arr_li_value_data = dynamic_cast<const uda::Array*>(li_value_data);

	std::cout << "values for global_quantities/li/value from 0 to 50: ";
	for (int j = 0; j < 51; ++j) {
		std::cout << arr_li_value_data->as<double>().at(j) << " ";
	}
	std::cout << "..." << std::endl;

	const uda::Result& beta_pol_value = client.get("imas::get(idx=0, group='summary', variable='global_quantities/beta_pol/value', expName='WEST', type=double, rank=1, shot=" SHOT_NUM ", )", "");
	const uda::Data * beta_pol_value_data = beta_pol_value.data();
	const uda::Array* arr_beta_pol_value_data = dynamic_cast<const uda::Array*>(beta_pol_value_data);

	std::cout << "values for global_quantities/beta_pol/value from 0 to 50: ";
	for (int j = 0; j < 51; ++j) {
		std::cout << arr_beta_pol_value_data->as<double>().at(j) << " ";
	}
	std::cout << "..." << std::endl;

	/*const uda::Result& beta_tor_value = client.get("imas::get(idx=0, group='summary', variable='global_quantities/beta_tor/value', expName='WEST', type=double, rank=1, shot=" SHOT_NUM ", )", "");
	const uda::Data * beta_tor_value_data = beta_tor_value.data();
	const uda::Array* arr_tor_value_data = dynamic_cast<const uda::Array*>(beta_tor_value_data);

	std::cout << "values for global_quantities/beta_tor/value from 0 to 50: ";
	for (int j = 0; j < 51; ++j) {
		std::cout << arr_tor_value_data->as<double>().at(j) << " ";
	}
	std::cout << "..." << std::endl;*/

	const uda::Result& global_quantities_r0 = client.get("imas::get(idx=0, group='summary', variable='global_quantities/r0/value', expName='WEST', type=double, rank=0, shot=" SHOT_NUM ", )", "");
	const uda::Scalar* scalar_global_quantities_r0 = dynamic_cast<const uda::Scalar*>(global_quantities_r0.data());
	std::cout << "global_quantities/r0/value: " << scalar_global_quantities_r0->as<double>() << std::endl;

	const uda::Result& b0_value = client.get("imas::get(idx=0, group='summary', variable='global_quantities/b0/value', expName='WEST', type=double, rank=1, shot=" SHOT_NUM ", )", "");
	const uda::Data * b0_value_data = b0_value.data();
	const uda::Array* arr_b0_value_data = dynamic_cast<const uda::Array*>(b0_value_data);

	std::cout << "values for global_quantities/r0/value from 0 to 50: ";
	for (int j = 0; j < 51; ++j) {
		std::cout << arr_b0_value_data->as<double>().at(j) << " ";
	}
	std::cout << "..." << std::endl;

	/*const uda::Result& tau_resistance_value = client.get("imas::get(idx=0, group='summary', variable='global_quantities/tau_resistance/value', expName='WEST', type=double, rank=1, shot=" SHOT_NUM ", )", "");
	const uda::Data * tau_resistance_value_data = tau_resistance_value.data();
	const uda::Array* arr_tau_resistance_value_data = dynamic_cast<const uda::Array*>(tau_resistance_value_data);

	std::cout << "values for global_quantities/tau_resistance/value from 0 to 50: ";
	for (int j = 0; j < 51; ++j) {
		std::cout << arr_tau_resistance_value_data->as<double>().at(j) << " ";
	}
	std::cout << "..." << std::endl;*/

	const uda::Result& local_edge_q__value = client.get("imas::get(idx=0, group='summary', variable='local/edge/q/value', expName='WEST', type=double, rank=1, shot=" SHOT_NUM ", )", "");
	const uda::Data * local_edge_q__value_data = local_edge_q__value.data();
	const uda::Array* arr_local_edge_q__value_data = dynamic_cast<const uda::Array*>(local_edge_q__value_data);

	std::cout << "values for local/edge/q/value from 0 to 50: ";
	for (int j = 0; j < 51; ++j) {
		std::cout << arr_local_edge_q__value_data->as<double>().at(j) << " ";
	}
	std::cout << "..." << std::endl;

	const uda::Result& volume_average_n_e_value = client.get("imas::get(idx=0, group='summary', variable='volume_average/n_e/value', expName='WEST', type=double, rank=1, shot=" SHOT_NUM ", )", "");
	const uda::Data * volume_average_n_e_value_data = volume_average_n_e_value.data();
	const uda::Array* arr_volume_average_n_e_value_data = dynamic_cast<const uda::Array*>(volume_average_n_e_value_data);

	std::cout << "values for volume_average/n_e/value from 0 to 50: ";
	for (int j = 0; j < 51; ++j) {
		std::cout << arr_volume_average_n_e_value_data->as<double>().at(j) << " ";
	}
	std::cout << "..." << std::endl;

	const uda::Result& heating_current_drive_ec_1_source = client.get("imas::get(idx=0, group='summary', variable='heating_current_drive/ec/1/power/source', expName='WEST', type=string, rank=0, shot=" SHOT_NUM ", )", "");
	const uda::Data * heating_current_drive_ec_1_source_data = heating_current_drive_ec_1_source.data();
	const uda::String* heating_current_drive_ec_1_source_str = dynamic_cast<const uda::String*>(heating_current_drive_ec_1_source_data);
	std::cout << "heating_current_drive/ec/1/power/source: " << heating_current_drive_ec_1_source_str->str() << "\n";

	const uda::Result& heating_current_drive_ec_2_source = client.get("imas::get(idx=0, group='summary', variable='heating_current_drive/ec/2/power/source', expName='WEST', type=string, rank=0, shot=" SHOT_NUM ", )", "");
	const uda::Data * heating_current_drive_ec_2_source_data = heating_current_drive_ec_2_source.data();
	const uda::String* heating_current_drive_ec_2_source_str = dynamic_cast<const uda::String*>(heating_current_drive_ec_2_source_data);
	std::cout << "heating_current_drive/ec/2/power/source: " << heating_current_drive_ec_2_source_str->str() << "\n";

	const uda::Result& heating_current_drive_ec_3_source = client.get("imas::get(idx=0, group='summary', variable='heating_current_drive/ec/3/power/source', expName='WEST', type=string, rank=0, shot=" SHOT_NUM ", )", "");
	const uda::Data * heating_current_drive_ec_3_source_data = heating_current_drive_ec_3_source.data();
	const uda::String* heating_current_drive_ec_3_source_str = dynamic_cast<const uda::String*>(heating_current_drive_ec_3_source_data);
	std::cout << "heating_current_drive/ec/3/power/source: " << heating_current_drive_ec_3_source_str->str() << "\n";

	const uda::Result& heating_current_drive_ec_1_value = client.get("imas::get(idx=0, group='summary', variable='heating_current_drive/ec/1/power/value', expName='WEST', type=double, rank=1, shot=" SHOT_NUM ", )", "");
	const uda::Data * heating_current_drive_ec_1_value_data = heating_current_drive_ec_1_value.data();
	const uda::Array* arr_heating_current_drive_ec_1_value_data = dynamic_cast<const uda::Array*>(heating_current_drive_ec_1_value_data);

	std::cout << "values for heating_current_drive/ec/1/power/value from 0 to 50: ";
	for (int j = 0; j < 51; ++j) {
		std::cout << arr_heating_current_drive_ec_1_value_data->as<double>().at(j) << " ";
	}
	std::cout << "..." << std::endl;

	const uda::Result& magnetic_axis_position_r_value = client.get("imas::get(idx=0, group='summary', variable='local/magnetic_axis/position/r', expName='WEST', type=double, rank=1, shot=" SHOT_NUM ", )", "");
	const uda::Data * magnetic_axis_position_r_value_data = magnetic_axis_position_r_value.data();
	const uda::Array* arr_magnetic_axis_position_r_value_data = dynamic_cast<const uda::Array*>(magnetic_axis_position_r_value_data);

	std::cout << "values for magnetic_axis/position/r/value from 0 to 50: ";
	for (int j = 0; j < 51; ++j) {
		std::cout << arr_magnetic_axis_position_r_value_data->as<double>().at(j) << " ";
	}
	std::cout << "..." << std::endl;

	const uda::Result& magnetics_time = client.get("imas::get(idx=0, group='summary', variable='time', expName='WEST', type=double, rank=1, shot=" SHOT_NUM ", )", "");
	const uda::Data * magnetics_time_data = magnetics_time.data();
	const uda::Array* arr_magnetics_time_data = dynamic_cast<const uda::Array*>(magnetics_time_data);

	std::cout << "values for /time from 0 to 50: ";
	for (int j = 0; j < 51; ++j) {
		std::cout << arr_magnetics_time_data->as<double>().at(j) << " ";
	}
	std::cout << "..." << std::endl;



	return 0;
}

