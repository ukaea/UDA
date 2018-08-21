#include "west_xml.h"

#include <string.h>
#include <stdio.h>
#include <assert.h>

#include <logging/logging.h>
#include <clientserver/initStructs.h>
#include <clientserver/errorLog.h>
#include <clientserver/stringUtils.h>
#include <clientserver/udaTypes.h>
#include <plugins/udaPlugin.h>
#include <structures/struct.h>

#include "ts_rqparam.h"
#include "west_utilities.h"
#include "west_static_data_utilities.h"

#include "west_ece.h"
#include "west_pf_passive.h"
#include "west_pf_active.h"
#include "west_soft_x_rays.h"
#include "west_summary.h"
#include "west_lh_antennas.h"
#include "west_barometry.h"

char* setBuffer(int data_type, char* value);
void getShapeOf(const char* command, int shotNumber, int* nb_val);
int shape_of_tsmat_collect(int shotNumber, char* TOP_collections_parameters, DATA_BLOCK* data_block);

int execute(const char* mapfun, int shotNumber, DATA_BLOCK* data_block, int* nodeIndices);
int execute_tsmat_collect(const char* TOP_collections_parameters, char* attributes,
		int shotNumber, DATA_BLOCK* data_block, int* nodeIndices,
		char* normalizationAttributes);
int execute_tsmat_without_idam_index(const char* command, char* attributes,
		int shotNumber, DATA_BLOCK* data_block, char* normalizationAttributes);
int execute_setvalue_collect(const char* TOP_collections_parameters, char* attributes,
		int shotNumber, DATA_BLOCK* data_block, int* nodeIndices, char* normalizationAttributes);
void execute_setchannels_validity(int* unvalid_channels_list, int unvalid_channels_size, char* attributes,
		int shotNumber, DATA_BLOCK* data_block, int* nodeIndices);
void setStatic1DValue(int data_type, DATA_BLOCK* data_block, char* value, int val_nb, float normalizationFactor);
void setPatchedStaticValue(DATA_BLOCK* data_block, char* object_name, int data_type, char* value, int searchedArrayIndex, float  normalizationFactor);

int GetStaticData(int shotNumber, const char* mapfun, DATA_BLOCK* data_block, int* nodeIndices)
{
	UDA_LOG(UDA_LOG_DEBUG, "Calling GetStaticData() from WEST plugin\n");

	assert(mapfun); //Mandatory function to get WEST data

	UDA_LOG(UDA_LOG_DEBUG, "Calling execute() from WEST plugin\n");
	int status = execute(mapfun, shotNumber, data_block, nodeIndices);

	if (status != 0) {
		UDA_LOG(UDA_LOG_ERROR, "WEST:ERROR: error while getting static data\n");

		addIdamError(CODEERRORTYPE, __func__, -900, "WEST:ERROR: error while getting static data");
	}

	return 0;
}

int execute(const char* mapfun, int shotNumber, DATA_BLOCK* data_block, int* nodeIndices)
{

	char* fun_name = NULL; //Shape_of, tsmat_collect, tsbase
	char* TOP_collections_parameters = NULL; //example : TOP_collections_parameters = DMAG:GMAG_BNORM:PosR, DMAG:GMAG_BTANG:PosR, ...
	char* attributes = NULL; //example : attributes = rank:float:#1 (rank=0,1 , type = float, #1 = second IDAM index)
	char* normalizationAttributes = NULL; //example : multiply:cste:3     (multiply value by constant factor equals to 3)

	getFunName(mapfun, &fun_name);
	UDA_LOG(UDA_LOG_DEBUG, "C function name found in mapping file: %s\n", fun_name);

	int fun = -1;

	if (strcmp(fun_name, "tsmat_collect") == 0) {
		//returns a static parameter (rank = 0) from a collection of static data (Top objects).
		//Given the list of all static data in the collection, the element returned in the data_block is list(idam index)
		fun = 0;
	} else if (strcmp(fun_name, "shape_of_tsmat_collect") == 0) {
		//Returns the list size of all static data in the collection
		fun = 1;
	} else if (strcmp(fun_name, "set_value") == 0) {
		//returns a static value (rank = 0)
		fun = 2;
	} else if (strcmp(fun_name, "tsmat") == 0) {
		//returns a static parameter (rank = 0 or 1) from a Top object
		fun = 3;
	} else if (strcmp(fun_name, "set_value_collect") == 0) {
		//returns a static value according to the position of the element in the collection (rank = 0)
		fun = 4;
	} else if (strcmp(fun_name, "set_channels_validity") == 0) {
		//returns a static value according to the position of the element in the collection (rank = 0)
		fun = 5;
	}  else if (strcmp(fun_name, "tsmat_collect_poloidal_angle") == 0) {
		fun = 6;
	}

	//--------------------ece----------------------------------
	else if (strcmp(fun_name, "homogeneous_time") == 0) {
		fun = 7;
	} else if (strcmp(fun_name, "ece_t_e_data_shape_of") == 0) {
		fun = 9;
	}

	//------------------pf_passive----------------------------------
	else if (strcmp(fun_name, "pf_passive_loop_name") == 0) {
		fun = 100;
	} else if (strcmp(fun_name, "pf_passive_loop_identifier") == 0) {
		fun = 101;
	} else if (strcmp(fun_name, "pf_passive_element_name") == 0) {
		fun = 102;
	} else if (strcmp(fun_name, "pf_passive_element_identifier") == 0) {
		fun = 103;
	} else if (strcmp(fun_name, "pf_passive_loops_shapeOf") == 0) {
		fun = 104;
	} else if (strcmp(fun_name, "pf_passive_turns") == 0) {
		fun = 105;
	} else if (strcmp(fun_name, "pf_passive_elements_shapeOf") == 0) {
		fun = 106;
	} else if (strcmp(fun_name, "pf_passive_R") == 0) {
		fun = 107;
	} else if (strcmp(fun_name, "pf_passive_Z") == 0) {
		fun = 108;
	} else if (strcmp(fun_name, "pf_passive_current_data") == 0) {
		fun = 109;
	} else if (strcmp(fun_name, "pf_passive_current_time") == 0) {
		fun = 110;
	}

	//------------------pf_active----------------------------------
	else if (strcmp(fun_name, "pf_active_elements_shapeOf") == 0) {
		fun = 140;
	} else if (strcmp(fun_name, "pf_active_coils_shapeOf") == 0) {
		fun = 141;
	} else if (strcmp(fun_name, "pf_active_R") == 0) {
		fun = 142;
	} else if (strcmp(fun_name, "pf_active_Z") == 0) {
		fun = 143;
	} else if (strcmp(fun_name, "pf_active_element_identifier") == 0) {
		fun = 144;
	} else if (strcmp(fun_name, "pf_active_element_name") == 0) {
		fun = 145;
	} else if (strcmp(fun_name, "pf_active_coil_identifier") == 0) {
		fun = 146;
	} else if (strcmp(fun_name, "pf_active_coil_name") == 0) {
		fun = 147;
	} else if (strcmp(fun_name, "pf_active_turns") == 0) {
		fun = 148;
	} else if (strcmp(fun_name, "pf_active_H") == 0) {
		fun = 149;
	} else if (strcmp(fun_name, "pf_active_W") == 0) {
		fun = 150;
	}

	//------------------soft_x_rays----------------------------------
	else if (strcmp(fun_name, "soft_x_rays_idsproperties_comment") == 0) {
		fun = 200;
	}
	else if (strcmp(fun_name, "soft_x_rays_channels_shapeof") == 0) {
		fun = 201;
	}
	else if (strcmp(fun_name, "channel_line_of_sight_first_point_r") == 0) {
		fun = 202;
	}
	else if (strcmp(fun_name, "channel_line_of_sight_first_point_z") == 0) {
		fun = 203;
	}
	else if (strcmp(fun_name, "channel_line_of_sight_first_point_phi") == 0) {
		fun = 204;
	}
	else if (strcmp(fun_name, "channel_line_of_sight_second_point_r") == 0) {
		fun = 205;
	}
	else if (strcmp(fun_name, "channel_line_of_sight_second_point_z") == 0) {
		fun = 206;
	}
	else if (strcmp(fun_name, "channel_line_of_sight_second_point_phi") == 0) {
		fun = 207;
	}
	else if (strcmp(fun_name, "soft_x_rays_channels_energy_band_shapeof") == 0) {
		fun = 208;
	}
	else if (strcmp(fun_name, "soft_x_rays_channels_energy_band_lower_bound") == 0) {
		fun = 209;
	}
	else if (strcmp(fun_name, "soft_x_rays_channels_energy_band_upper_bound") == 0) {
		fun = 210;
	}
	else if (strcmp(fun_name, "summary_global_quantities_r0_value") == 0) {
		fun = 300;
	}
	else if (strcmp(fun_name, "summary_heating_current_drive_ec_power_source") == 0) {
		fun = 301;
	}
	else if (strcmp(fun_name, "lh_antennas_position_definition") == 0) {
		fun = 400;
	}
	else if (strcmp(fun_name, "barometry_gauge_name") == 0) {
		fun = 500;
	}
	else if (strcmp(fun_name, "barometry_gauge_type_name") == 0) {
		fun = 501;
	}
	else if (strcmp(fun_name, "barometry_gauge_type_index") == 0) {
		fun = 502;
	}
	else if (strcmp(fun_name, "barometry_gauge_type_description") == 0) {
		fun = 503;
	}
	else if (strcmp(fun_name, "barometry_gauge_calibration_coefficient") == 0) {
		fun = 504;
	}
	UDA_LOG(UDA_LOG_DEBUG, "Case: %d", fun);

	if (fun == -1) {
		UDA_LOG(UDA_LOG_DEBUG, "WEST:ERROR no function mapped to %s\n", fun_name);
		int err = 901;
		UDA_LOG(UDA_LOG_ERROR, "WEST:ERROR: no C function mapped for %s!", fun_name);
		addIdamError(CODEERRORTYPE, "WEST:ERROR: no C function mapped for %s!", err, fun_name);
	}


	switch (fun) {
	case 0: {
		UDA_LOG(UDA_LOG_DEBUG, "Case of tsmat_collect from WEST plugin\n");
		tokenizeFunParameters(mapfun, &TOP_collections_parameters, &attributes, &normalizationAttributes);
		return execute_tsmat_collect(TOP_collections_parameters, attributes, shotNumber, data_block, nodeIndices,
				normalizationAttributes);

		break;
	}

	case 1: {

		UDA_LOG(UDA_LOG_DEBUG, "Case of shape_of_tsmat_collect from WEST plugin\n");
		tokenizeFunParameters(mapfun, &TOP_collections_parameters, &attributes, &normalizationAttributes);
		return shape_of_tsmat_collect(shotNumber, TOP_collections_parameters, data_block);

		break;
	}

	case 2: {
		UDA_LOG(UDA_LOG_DEBUG, "Case of set_value from WEST plugin\n");
		char* value = NULL;
		tokenizeFunParameters(mapfun, &value, &attributes, &normalizationAttributes);

		int data_type;
		getReturnType(attributes, &data_type);
		char* buffer = setBuffer(data_type, value);

		UDA_LOG(UDA_LOG_DEBUG, "Calling setStaticValue()\n");
		float normalizationFactor = 1;
		getNormalizationFactor(&normalizationFactor, normalizationAttributes);
		setStaticValue(data_type, data_block, buffer, 0, normalizationFactor); //index is always 0 in this case
		UDA_LOG(UDA_LOG_DEBUG, "Returning from setStaticValue()\n");

		free(value);
		free(buffer);
		return 0;
		break;
	}

	case 3: {
		UDA_LOG(UDA_LOG_DEBUG, "Case of tsmat from WEST plugin\n");
		tokenizeFunParameters(mapfun, &TOP_collections_parameters, &attributes, &normalizationAttributes);
		return execute_tsmat_without_idam_index(TOP_collections_parameters, attributes, shotNumber, data_block,
				normalizationAttributes);
		break;
	}

	case 4: {
		UDA_LOG(UDA_LOG_DEBUG, "Case of set_value_collect from WEST plugin\n");
		tokenizeFunParameters(mapfun, &TOP_collections_parameters, &attributes, &normalizationAttributes);
		UDA_LOG(UDA_LOG_DEBUG, "attributes: %s\n", attributes);
		return execute_setvalue_collect(TOP_collections_parameters, attributes, shotNumber, data_block, nodeIndices,
				normalizationAttributes);
		break;
	}

	case 5: {
		UDA_LOG(UDA_LOG_DEBUG, "Case of set_channels_validity from WEST plugin\n");
		char* unvalid_channels_request = NULL; //used for interfero_polarimeter IDS, example : invalid_channels:2:1,2
		int unvalid_channels_size;
		int* unvalid_channels_list = NULL;
		UDA_LOG(UDA_LOG_DEBUG, "Calling tokenize_set_channels_validity() from WEST plugin\n");
		tokenize_set_channels_validity(mapfun, &unvalid_channels_request, &attributes);

		getUnvalidChannelsSize(unvalid_channels_request, &unvalid_channels_size);
		unvalid_channels_list = (int*)malloc(sizeof(int) * unvalid_channels_size);

		getUnvalidChannels(unvalid_channels_request, unvalid_channels_list);
		execute_setchannels_validity(unvalid_channels_list, unvalid_channels_size, attributes, shotNumber,
				data_block, nodeIndices);

		free(unvalid_channels_request);
		free(unvalid_channels_list);
		return 0;
		break;
	}
	case 6: {
		/*UDA_LOG(UDA_LOG_DEBUG, "Case of tsmat_collect_poloidal_angle from WEST plugin\n");
		tokenizeFunParameters(mapfun, &TOP_collections_parameters, &attributes, &normalizationAttributes);
		execute_tsmat_collect_poloidal_angle(TOP_collections_parameters, attributes, shotNumber, data_block, nodeIndices,
				normalizationAttributes);*/
		break;
	}

	case 7: {
		UDA_LOG(UDA_LOG_DEBUG, "Case of homogeneous_time from WEST plugin\n");
		homogeneous_time(shotNumber, data_block, nodeIndices);
		break;
	}

	case 9: {
		UDA_LOG(UDA_LOG_DEBUG, "Case of ece_t_e_data_shape_of from WEST plugin\n");
		char* ece_mapfun = NULL;
		ece_t_e_data_shape_of(shotNumber, &ece_mapfun);
		UDA_LOG(UDA_LOG_DEBUG, "Calling tokenizeFunParameters() from WEST plugin\n");
		tokenizeFunParameters(ece_mapfun, &TOP_collections_parameters, &attributes, &normalizationAttributes);
		shape_of_tsmat_collect(shotNumber, TOP_collections_parameters, data_block);
		free(ece_mapfun);
		return 0;
		break;
	}

	case 100: {
		UDA_LOG(UDA_LOG_DEBUG, "Case of pf_passive_loop_name from WEST plugin\n");
		pf_passive_loop_name(shotNumber, data_block, nodeIndices);
		return 0;
		break;
	}

	case 101: {
		UDA_LOG(UDA_LOG_DEBUG, "Case of pf_passive_loop_identifier from WEST plugin\n");
		pf_passive_loop_identifier(shotNumber, data_block, nodeIndices);
		return 0;
		break;
	}

	case 102: {
		UDA_LOG(UDA_LOG_DEBUG, "Case of pf_passive_element_name from WEST plugin\n");
		pf_passive_element_name(shotNumber, data_block, nodeIndices);
		return 0;
		break;
	}

	case 103: {
		UDA_LOG(UDA_LOG_DEBUG, "Case of pf_passive_element_identifier from WEST plugin\n");
		pf_passive_element_identifier(shotNumber, data_block, nodeIndices);
		break;
	}

	case 104: {
		UDA_LOG(UDA_LOG_DEBUG, "Case of pf_passive_loops_shapeOf from WEST plugin\n");
		pf_passive_loops_shapeOf(shotNumber, data_block, nodeIndices);
		return 0;
		break;
	}

	case 105: {
		UDA_LOG(UDA_LOG_DEBUG, "Case of pf_passive_turns from WEST plugin\n");
		pf_passive_turns(shotNumber, data_block, nodeIndices);
		return 0;
		break;
	}

	case 106: {
		UDA_LOG(UDA_LOG_DEBUG, "Case of pf_passive_elements_shapeOf from WEST plugin\n");
		pf_passive_elements_shapeOf(shotNumber, data_block, nodeIndices);
		return 0;
		break;
	}

	case 107: {
		UDA_LOG(UDA_LOG_DEBUG, "Case of pf_passive_R from WEST plugin\n");
		pf_passive_R(shotNumber, data_block, nodeIndices);
		break;
	}

	case 108: {
		UDA_LOG(UDA_LOG_DEBUG, "Case of pf_passive_Z from WEST plugin\n");
		pf_passive_Z(shotNumber, data_block, nodeIndices);
		return 0;
		break;
	}

	case 109: {
		UDA_LOG(UDA_LOG_DEBUG, "Case of pf_passive_current_data from WEST plugin\n");
		return pf_passive_current_data(shotNumber, data_block, nodeIndices);
	}

	case 110: {
		UDA_LOG(UDA_LOG_DEBUG, "Case of pf_passive_current_time from WEST plugin\n");
		return pf_passive_current_time(shotNumber, data_block, nodeIndices);
	}


	case 140: {
		UDA_LOG(UDA_LOG_DEBUG, "Case of pf_active_elements_shapeOf from WEST plugin\n");
		pf_active_elements_shapeOf(shotNumber, data_block, nodeIndices);
		return 0;
		break;
	}

	case 141: {
		UDA_LOG(UDA_LOG_DEBUG, "Case of pf_active_coils_shapeOf from WEST plugin\n");
		pf_active_coils_shapeOf(shotNumber, data_block, nodeIndices);
		return 0;
		break;
	}

	case 142: {
		UDA_LOG(UDA_LOG_DEBUG, "Case of pf_active_R from WEST plugin\n");
		pf_active_R(shotNumber, data_block, nodeIndices);
		return 0;
		break;
	}

	case 143: {
		UDA_LOG(UDA_LOG_DEBUG, "Case of pf_active_Z from WEST plugin\n");
		pf_active_Z(shotNumber, data_block, nodeIndices);
		return 0;
		break;
	}

	case 144: {
		UDA_LOG(UDA_LOG_DEBUG, "Case of pf_active_element_identifier from WEST plugin\n");
		pf_active_element_identifier(shotNumber, data_block, nodeIndices);
		return 0;
		break;
	}

	case 145: {
		UDA_LOG(UDA_LOG_DEBUG, "Case of pf_active_element_name from WEST plugin\n");
		pf_active_element_name(shotNumber, data_block, nodeIndices);
		return 0;
		break;
	}

	case 146: {
		UDA_LOG(UDA_LOG_DEBUG, "Case of pf_active_coil_identifier from WEST plugin\n");
		pf_active_coil_identifier(shotNumber, data_block, nodeIndices);
		return 0;
		break;
	}

	case 147: {
		UDA_LOG(UDA_LOG_DEBUG, "Case of pf_active_coil_name from WEST plugin\n");
		pf_active_coil_name(shotNumber, data_block, nodeIndices);
		break;
	}

	case 148: {
		UDA_LOG(UDA_LOG_DEBUG, "Case of pf_active_turns from WEST plugin\n");
		pf_active_turns(shotNumber, data_block, nodeIndices);
		return 0;
		break;
	}

	case 149: {
		UDA_LOG(UDA_LOG_DEBUG, "Case of pf_active_H from WEST plugin\n");
		pf_active_H(shotNumber, data_block, nodeIndices);
		break;
	}

	case 150: {
		UDA_LOG(UDA_LOG_DEBUG, "Case of pf_active_W from WEST plugin\n");
		pf_active_W(shotNumber, data_block, nodeIndices);
		return 0;
		break;
	}

	case 200: {
		UDA_LOG(UDA_LOG_DEBUG, "Case of soft_x_rays_idsproperties_comment from WEST plugin\n");
		soft_x_rays_idsproperties_comment(shotNumber, data_block, nodeIndices);
		return 0;
		break;
	}
	case 201: {
		UDA_LOG(UDA_LOG_DEBUG, "Case of soft_x_rays_channels_shapeof from WEST plugin\n");
		soft_x_rays_channels_shapeof(shotNumber, data_block, nodeIndices);
		return 0;
		break;
	}
	case 202: {
		UDA_LOG(UDA_LOG_DEBUG, "Case of channel_line_of_sight_first_point_r from WEST plugin\n");
		channel_line_of_sight_first_point_r(shotNumber, data_block, nodeIndices);
		return 0;
		break;
	}
	case 203: {
		UDA_LOG(UDA_LOG_DEBUG, "Case of channel_line_of_sight_first_point_z from WEST plugin\n");
		channel_line_of_sight_first_point_z(shotNumber, data_block, nodeIndices);
		return 0;
		break;
	}
	case 204: {
		UDA_LOG(UDA_LOG_DEBUG, "Case of channel_line_of_sight_first_point_phi from WEST plugin\n");
		channel_line_of_sight_first_point_phi(shotNumber, data_block, nodeIndices);
		return 0;
		break;
	}
	case 205: {
		UDA_LOG(UDA_LOG_DEBUG, "Case of channel_line_of_sight_second_point_r from WEST plugin\n");
		channel_line_of_sight_second_point_r(shotNumber, data_block, nodeIndices);
		return 0;
		break;
	}
	case 206: {
		UDA_LOG(UDA_LOG_DEBUG, "Case of channel_line_of_sight_second_point_z from WEST plugin\n");
		channel_line_of_sight_second_point_z(shotNumber, data_block, nodeIndices);
		return 0;
		break;
	}
	case 207: {
		UDA_LOG(UDA_LOG_DEBUG, "Case of channel_line_of_sight_second_point_phi from WEST plugin\n");
		channel_line_of_sight_second_point_phi(shotNumber, data_block, nodeIndices);
		return 0;
		break;
	}
	case 208: {
		UDA_LOG(UDA_LOG_DEBUG, "Case of soft_x_rays_channels_energy_band_shapeof from WEST plugin\n");
		soft_x_rays_channels_energy_band_shapeof(shotNumber, data_block, nodeIndices);
		return 0;
		break;
	}
	case 209: {
		UDA_LOG(UDA_LOG_DEBUG, "Case of soft_x_rays_channels_energy_band_lower_bound from WEST plugin\n");
		soft_x_rays_channels_energy_band_lower_bound(shotNumber, data_block, nodeIndices);
		return 0;
		break;
	}
	case 210: {
		UDA_LOG(UDA_LOG_DEBUG, "Case of soft_x_rays_channels_energy_band_upper_bound from WEST plugin\n");
		soft_x_rays_channels_energy_band_upper_bound(shotNumber, data_block, nodeIndices);
		return 0;
		break;
	}
	case 300: {
		UDA_LOG(UDA_LOG_DEBUG, "Case of summary_global_quantities_r0_value from WEST plugin\n");
		return summary_global_quantities_r0_value(shotNumber, data_block, nodeIndices);
		break;
	}
	case 301: {
		UDA_LOG(UDA_LOG_DEBUG, "Case of summary_heating_current_drive_ec_power_source from WEST plugin\n");
		return summary_heating_current_drive_ec_power_source(shotNumber, data_block, nodeIndices);
		break;
	}
	case 400: {
		UDA_LOG(UDA_LOG_DEBUG, "Case of lh_antennas_position_definition from WEST plugin\n");
		lh_antennas_position_definition(shotNumber, data_block, nodeIndices);
		return 0;
		break;
	}

	case 500: {
		UDA_LOG(UDA_LOG_DEBUG, "Case of barometry_gauge_name from WEST plugin\n");
		barometry_gauge_name(shotNumber, data_block, nodeIndices);
		return 0;
		break;
	}
	case 501: {
		UDA_LOG(UDA_LOG_DEBUG, "Case of barometry_gauge_type_name from WEST plugin\n");
		barometry_gauge_type_name(shotNumber, data_block, nodeIndices);
		return 0;
		break;
	}
	case 502: {
		UDA_LOG(UDA_LOG_DEBUG, "Case of barometry_gauge_type_index from WEST plugin\n");
		barometry_gauge_type_index(shotNumber, data_block, nodeIndices);
		return 0;
		break;

	}
	case 503: {
		UDA_LOG(UDA_LOG_DEBUG, "Case of barometry_gauge_type_description from WEST plugin\n");
		barometry_gauge_type_description(shotNumber, data_block, nodeIndices);
		return 0;
		break;

	}
	case 504: {
		UDA_LOG(UDA_LOG_DEBUG, "Case of barometry_gauge_calibration_coefficient from WEST plugin\n");
		barometry_gauge_calibration_coefficient(shotNumber, data_block, nodeIndices);
		return 0;
		break;

	}

	}

	free(fun_name);
	free(TOP_collections_parameters);
	free(attributes);
	free(normalizationAttributes);
	return 0;
}


int shape_of_tsmat_collect(int shotNumber, char* TOP_collections_parameters, DATA_BLOCK* data_block)
{
	//Get the number of parameters collections
	int collectionsCount;

	UDA_LOG(UDA_LOG_DEBUG, "Calling getTopCollectionsCount() from WEST plugin for shape_of_tsmat_collect case\n");
	getTopCollectionsCount(TOP_collections_parameters, &collectionsCount);

	UDA_LOG(UDA_LOG_DEBUG, "Collections count: %d", collectionsCount);

	UDA_LOG(UDA_LOG_DEBUG, "Calling tokenizeFunCollect() from WEST plugin\n");

	//Get the total size by adding all collections lengths
	int i;
	int parametersSize = 0;
	for (i = 0; i < collectionsCount; i++) {
		char* command = NULL;
		UDA_LOG(UDA_LOG_DEBUG, "Calling getCommand() from WEST plugin for shape_of_tsmat_collect case\n");
		getCommand(i, &command, TOP_collections_parameters); //get one command
		UDA_LOG(UDA_LOG_DEBUG, "Command : %s\n", command);
		int nb_val = 0;
		UDA_LOG(UDA_LOG_DEBUG, "Calling getShapeOf() from WEST plugin for shape_of_tsmat_collect case\n");
		getShapeOf(command, shotNumber, &nb_val);
		UDA_LOG(UDA_LOG_DEBUG, "nb_val: %d", nb_val);
		UDA_LOG(UDA_LOG_DEBUG, "after getShapeOf\n");
		parametersSize += nb_val;
	}

	setReturnDataIntScalar(data_block, parametersSize, NULL);
	return 0;
}

void getShapeOf(const char* command, int shotNumber, int* nb_val)
{

	char* prod_name = NULL; //DMAG, ...
	char* object_name = NULL;
	char* param_name = NULL;
	char* flag = NULL;

	UDA_LOG(UDA_LOG_DEBUG, "In getShapeOf, calling tokenizeCommand with command: %s\n", command);

	//Tokenize mapfun string to get function parameters, return type and arguments (#1, #2,...) to use
	tokenizeCommand(command, &prod_name, &object_name, &param_name, &flag);

	char* value = NULL;
	int val_nb = 1;
	//get the size of available data
	UDA_LOG(UDA_LOG_DEBUG, "DEBUG : prod_name: %s\n", prod_name);
	UDA_LOG(UDA_LOG_DEBUG, "DEBUG : object: %s\n", object_name);
	UDA_LOG(UDA_LOG_DEBUG, "DEBUG : param: %s\n", param_name);

	int status = readStaticParameters(&value, nb_val, shotNumber, prod_name, object_name, param_name, val_nb);
	UDA_LOG(UDA_LOG_DEBUG, "readStaticParameters status: %d\n", status);
	UDA_LOG(UDA_LOG_DEBUG, "readStaticParameters nb_val: %d\n", *nb_val);

	if (status != 0) {
		UDA_LOG(UDA_LOG_DEBUG, "Error calling readStaticParameters\n");
		int err = 901;
		UDA_LOG(UDA_LOG_ERROR, "%s", "WEST:ERROR: unable to get shape of static data");
		addIdamError(CODEERRORTYPE, "WEST:ERROR: unable to get shape of static data", err, "");
	}

	free(prod_name);
	free(object_name);
	free(param_name);
}

void execute_setchannels_validity(int* unvalid_channels_list, int unvalid_channels_size, char* attributes,
		int shotNumber, DATA_BLOCK* data_block, int* nodeIndices)
{
	int requestedIndex = getNumIDAMIndex(attributes, nodeIndices);
	int channel_number = requestedIndex + 1; //TODO TO CHECK
	UDA_LOG(UDA_LOG_DEBUG, "channel number : %d\n", channel_number);

	if (isChannelValid(channel_number, unvalid_channels_list, unvalid_channels_size)) {
		UDA_LOG(UDA_LOG_DEBUG, "valid channel number : %d\n", channel_number);
		setReturnDataIntScalar(data_block, 0, NULL);
	} else {
		UDA_LOG(UDA_LOG_DEBUG, "unvalid channel number : %d\n", channel_number);
		setReturnDataIntScalar(data_block, -1, NULL);
	}
}

int execute_setvalue_collect(const char* TOP_collections_parameters, char* attributes,
		int shotNumber, DATA_BLOCK* data_block, int* nodeIndices, char* normalizationAttributes)
{

	int collectionsCount;
	getTopCollectionsCount(TOP_collections_parameters, &collectionsCount);

	int* l;
	l = (int*)calloc(collectionsCount, sizeof(int));

	int i;
	int status = -1;
	for (i = 0; i < collectionsCount; i++) {
		char* command = NULL;
		status = getCommand(i, &command, TOP_collections_parameters);
		if (status == -1) {
			int err = 901;
			UDA_LOG(UDA_LOG_ERROR, "%s", "WEST:ERROR: unable to get the shapeof command");
			addIdamError(CODEERRORTYPE, "WEST:ERROR: unable to get the shapeof command", err, "");
			free(command);
			return status;
		}

		int nb_val = 0;
		getShapeOf(command, shotNumber, &nb_val);
		l[i] = nb_val;
	}

	int requestedIndex = getNumIDAMIndex(attributes, nodeIndices);

	int searchedArray;
	int searchedArrayIndex;

	searchIndices(requestedIndex, l, &searchedArray, &searchedArrayIndex);
	UDA_LOG(UDA_LOG_DEBUG, "searchedArray : %d\n", searchedArray);
	UDA_LOG(UDA_LOG_DEBUG, "searchedArrayIndex : %d\n", searchedArrayIndex);

	char* command = NULL;
	getCommand(searchedArray, &command, TOP_collections_parameters);

	char* value;
	getValueCollect(command, &value, nodeIndices);
	UDA_LOG(UDA_LOG_DEBUG, "Command : %s\n", command);

	int data_type;
	getReturnType(attributes, &data_type);

	char* buffer = setBuffer(data_type, value);

	UDA_LOG(UDA_LOG_DEBUG, "Found value: %s\n", value);

	float normalizationFactor = 1;
	getNormalizationFactor(&normalizationFactor, normalizationAttributes);
	setStaticValue(data_type, data_block, buffer, 0, normalizationFactor);

	free(value);
	free(command);
	free(buffer);
	return 0;
}


char* setBuffer(int data_type, char* value)
{
	char* buffer = NULL;

	if (data_type == UDA_TYPE_FLOAT) {
		UDA_LOG(UDA_LOG_DEBUG, "UDA_TYPE_FLOAT requested from WEST plugin\n");
		buffer = malloc(sizeof(float));
		float* f_buf = (float*)buffer;
		*f_buf = atof(value);
		UDA_LOG(UDA_LOG_DEBUG, "Testing float value : %f\n", *f_buf);
	} else if (data_type == UDA_TYPE_DOUBLE) {
		UDA_LOG(UDA_LOG_DEBUG, "UDA_TYPE_DOUBLE requested from WEST plugin\n");
		buffer = malloc(sizeof(double));
		double* f_buf = (double*)buffer;
		*f_buf = atof(value);
	} else if (data_type == UDA_TYPE_INT) {
		UDA_LOG(UDA_LOG_DEBUG, "UDA_TYPE_INT requested from WEST plugin\n");
		buffer = malloc(sizeof(int));
		int* i_buf = (int*)buffer;
		*i_buf = atoi(value);
	} else if (data_type == UDA_TYPE_STRING) {
		UDA_LOG(UDA_LOG_DEBUG, "UDA_TYPE_STRING requested from WEST plugin\n");
		buffer = strdup(value);
	} else {
		int err = 901;
		UDA_LOG(UDA_LOG_DEBUG, "Unsupported data type in setBuffer(): %d\n", data_type);
		UDA_LOG(UDA_LOG_ERROR, "%s", "WEST:ERROR: unsupported data type");
		addIdamError(CODEERRORTYPE, "WEST:ERROR: unsupported data type", err, "");
	}

	return buffer;
}

int execute_tsmat_collect(const char* TOP_collections_parameters, char* attributes,
		int shotNumber, DATA_BLOCK* data_block, int* nodeIndices, char* normalizationAttributes)
{
	int collectionsCount;
	getTopCollectionsCount(TOP_collections_parameters, &collectionsCount);

	int* l;
	l = (int*)calloc(collectionsCount, sizeof(int));

	int i;
	int status = -1;
	for (i = 0; i < collectionsCount; i++) {
		char* command = NULL;
		status = getCommand(i, &command, TOP_collections_parameters);
		if (status == -1) {
			int err = 901;
			UDA_LOG(UDA_LOG_ERROR, "%s", "WEST:ERROR: unable to get the shapeof command");
			addIdamError(CODEERRORTYPE, "WEST:ERROR: unable to get the shapeof command", err, "");
			free(l);
			return status;
		}

		int nb_val = 0;
		getShapeOf(command, shotNumber, &nb_val);
		l[i] = nb_val;
	}

	UDA_LOG(UDA_LOG_DEBUG, "In execute_tsmat_collect, searching requestedIndex... %s\n", "");
	int requestedIndex = getNumIDAMIndex(attributes, nodeIndices);

	UDA_LOG(UDA_LOG_DEBUG, "In execute_tsmat_collect, after searching requestedIndex --> %d\n", requestedIndex);

	int searchedArray;
	int searchedArrayIndex;

	UDA_LOG(UDA_LOG_DEBUG, "In execute_tsmat_collect, searching index array for requested index: %d\n",
			requestedIndex);
	searchIndices(requestedIndex, l, &searchedArray, &searchedArrayIndex);
	UDA_LOG(UDA_LOG_DEBUG, "In execute_tsmat_collect, searched array:%d\n", searchedArray);
	UDA_LOG(UDA_LOG_DEBUG, "In execute_tsmat_collect, searched array index:%d\n", searchedArrayIndex);

	char* command = NULL;

	UDA_LOG(UDA_LOG_DEBUG, "In execute_tsmat_collect, getting command from TOP_collections_parameters: %s\n",
			TOP_collections_parameters);
	getCommand(searchedArray, &command, TOP_collections_parameters);

	UDA_LOG(UDA_LOG_DEBUG, "In execute_tsmat_collect, after getting command...\n");

	char* prod_name = NULL;     //DMAG, ...
	char* object_name = NULL;   //GMAG_BNORM, ...
	char* param_name = NULL;    //PosR, ...
	char* flag = NULL;          //'Null' or blank

	int data_type;
	getReturnType(attributes, &data_type);

	//Tokenize mapfun string to get function parameters
	UDA_LOG(UDA_LOG_DEBUG, "In execute_tsmat_collect, tokenizing command... %s\n", command);
	tokenizeCommand(command, &prod_name, &object_name, &param_name, &flag);
	UDA_LOG(UDA_LOG_DEBUG, "In execute_tsmat_collect, afetr tokenizing command...\n");

	char* value = NULL;
	int val_nb = l[searchedArray];
	int nb_val;

	UDA_LOG(UDA_LOG_DEBUG, "In execute_tsmat_collect, flag: %s\n", flag);
	UDA_LOG(UDA_LOG_DEBUG, "In execute_tsmat_collect, checking if flag is Null...\n");

	if (flag != NULL && strncmp("Null", flag, 4) == 0) {
		UDA_LOG(UDA_LOG_DEBUG, "In execute_tsmat_collect, setting value for Null flag...\n");
		int data_type;
		getReturnType(attributes, &data_type);
		value = setBuffer(data_type, "0"); //we put zero for 'Null' flag
		searchedArrayIndex = 0;
	} else {
		//Reading static parameters using TSLib
		UDA_LOG(UDA_LOG_DEBUG, "In execute_tsmat_collect, reading static parameters for param. name: %s\n",
				param_name);
		status = readStaticParameters(&value, &nb_val, shotNumber, prod_name, object_name, param_name, val_nb);
		if (status != 0) {
			int err = 901;
			UDA_LOG(UDA_LOG_ERROR, "%s", "WEST:ERROR: unable to read static data");
			addIdamError(CODEERRORTYPE, "WEST:ERROR: unable to read static data", err, "");
			free(command);
			free(prod_name);
			free(object_name);
			free(param_name);
			free(value);
		}
	}

	float normalizationFactor = 1;
	getNormalizationFactor(&normalizationFactor, normalizationAttributes);
	UDA_LOG(UDA_LOG_DEBUG, "In execute_tsmat_collect, setting static value... %s\n", "");

	setStaticValue(data_type, data_block, value, searchedArrayIndex, normalizationFactor);

	free(command);
	free(prod_name);
	free(object_name);
	free(param_name);
	free(value);
	return 0;
}

void setPatchedStaticValue(DATA_BLOCK* data_block, char* object_name, int data_type, char* value, int searchedArrayIndex, float  normalizationFactor) {
	if (strncmp("GMAG_BTANG", object_name, 10) == 0) //TODO Temporary patch, will be removed soon
		set_BTANG_StaticValue(data_type, data_block, value, searchedArrayIndex, normalizationFactor);
	else
		setStaticValue(data_type, data_block, value, searchedArrayIndex, normalizationFactor);
}

int execute_tsmat_without_idam_index(const char* TOP_collections_parameters, char* attributes,
		int shotNumber, DATA_BLOCK* data_block, char* normalizationAttributes)
{
	char* command;
	getCommand(0, &command, TOP_collections_parameters);

	char* prod_name = NULL;     //DMAG, ...
	char* object_name = NULL;   //GMAG_BNORM, ...
	char* param_name = NULL;    //PosR, ..
	char* flag = NULL;

	int data_type;
	getReturnType(attributes, &data_type);

	//Tokenize mapfun string to get function parameters
	tokenizeCommand(command, &prod_name, &object_name, &param_name, &flag);

	int val_nb = 0;
	getShapeOf(command, shotNumber, &val_nb);

	char* value = NULL;
	int nb_val;

	//Reading static parameters using TSLib
	int status = readStaticParameters(&value, &nb_val, shotNumber, prod_name, object_name, param_name, val_nb);
	if (status != 0) {
		int err = 901;
		UDA_LOG(UDA_LOG_ERROR, "%s", "WEST:ERROR: unable to read static data");
		addIdamError(CODEERRORTYPE, "WEST:ERROR: Unable to read static data", err, "");
		free(command);
		free(prod_name);
		free(object_name);
		free(param_name);
		free(value);
		return status;
	}

	int rank;
	getRank(attributes, &rank);

	if (rank == 0) {
		float normalizationFactor = 1;
		getNormalizationFactor(&normalizationFactor, normalizationAttributes);
		setStaticValue(data_type, data_block, value, 0, normalizationFactor);
	} else if (rank == 1) {
		float normalizationFactor = 1;
		getNormalizationFactor(&normalizationFactor, normalizationAttributes);
		setStatic1DValue(data_type, data_block, value, val_nb, normalizationFactor);
	} else {
		int err = 901;
		UDA_LOG(UDA_LOG_DEBUG, "Unsupported rank from execute_tsmat_without_idam_index()\n");
		UDA_LOG(UDA_LOG_ERROR, "%s", "WEST:ERROR: unsupported data type");
		addIdamError(CODEERRORTYPE, "WEST:ERROR: unsupported data type", err, "");
		free(command);
		free(prod_name);
		free(object_name);
		free(param_name);
		free(value);
		return -1;
	}

	free(command);
	free(prod_name);
	free(object_name);
	free(param_name);
	free(value);
	return 0;
}

//Cast the results returned by tsmat according to the type of the data and set IDAM data
void setStatic1DValue(int data_type, DATA_BLOCK* data_block, char* value, int val_nb, float normalizationFactor)
{
	if (data_type == UDA_TYPE_FLOAT) {
		UDA_LOG(UDA_LOG_DEBUG, "handling float 1D in setStatic1DValue()\n");

		char val_nb_str[10];
		sprintf(val_nb_str, "%d", val_nb);
		UDA_LOG(UDA_LOG_DEBUG, "val_nb : %s\n", val_nb_str);

		if (val_nb == 0) {
			int err = 901;
			addIdamError(CODEERRORTYPE, __func__, err, "WEST:ERROR: val_nb is equals to 0 !");
		}

		data_block->data_type = UDA_TYPE_FLOAT;
		data_block->data = malloc(val_nb * sizeof(float));
		float* pt_float = (float*)value;
		multiplyFloat(pt_float, normalizationFactor, val_nb);
		UDA_LOG(UDA_LOG_DEBUG, "handling float 1D in setStatic1DValue21()\n");
		*((float*)data_block->data) = *pt_float;
		UDA_LOG(UDA_LOG_DEBUG, "handling float 1D in setStatic1DValue22()\n");
	} else if (data_type == UDA_TYPE_INT) {
		UDA_LOG(UDA_LOG_DEBUG, "handling int 1D in setStatic1DValue()\n");
		data_block->data_type = UDA_TYPE_INT;
		data_block->data = malloc(val_nb * sizeof(int));
		int* pt_int = (int*)value;
		multiplyInt(pt_int, normalizationFactor, val_nb);
		*((int*)data_block->data) = *pt_int;
		UDA_LOG(UDA_LOG_DEBUG, "handling int 1D in setStatic1DValue2()\n");
	} else {
		int err = 901;
		UDA_LOG(UDA_LOG_DEBUG, "Unsupported data type from setStatic1DValue()\n");
		UDA_LOG(UDA_LOG_ERROR, "%s", "WEST:ERROR: unsupported data type");
		addIdamError(CODEERRORTYPE, __func__, err, "WEST:ERROR: unsupported data type");
	}
}

