#include "west_dynamic_data.h"

#include <string.h>
#include <stdio.h>
#include <assert.h>

#include <logging/logging.h>
#include <clientserver/initStructs.h>
#include <clientserver/errorLog.h>
#include <clientserver/stringUtils.h>
#include <clientserver/udaTypes.h>

#include "west_utilities.h"
#include "west_dyn_data_utilities.h"

#include "west_ece.h"
#include "west_pf_passive.h"
#include "west_pf_active.h"
#include "west_soft_x_rays.h"
#include "west_summary.h"
#include "west_lh_antennas.h"
#include "west_barometry.h"

int GetDynamicData(int shotNumber, const char* mapfun, DATA_BLOCK* data_block, int* nodeIndices)
{

	UDA_LOG(UDA_LOG_DEBUG, "Entering GetDynamicData() -- WEST plugin\n");

	assert(mapfun); //Mandatory function to get WEST data

	char* fun_name = NULL; //Shape_of, tsmat_collect, tsbase
	char* TOP_collections_parameters = NULL; //example : TOP_collections_parameters = DMAG:GMAG_BNORM:PosR, DMAG:GMAG_BTANG:PosR, ...
	char* attributes = NULL; //example: attributes = 1:float:#1 (rank = 1, type = float, #1 = second IDAM index)
	char* normalizationAttributes = NULL; //example : multiply:cste:3     (multiply value by constant factor equals to 3)

	getFunName(mapfun, &fun_name);

	UDA_LOG(UDA_LOG_DEBUG, "UDA request: %s for shot: %d\n", fun_name, shotNumber);

	if (strcmp(fun_name, "tsbase_collect") == 0) {
		tokenizeFunParameters(mapfun, &TOP_collections_parameters, &attributes, &normalizationAttributes);
		return SetNormalizedDynamicData(shotNumber, data_block, nodeIndices, TOP_collections_parameters, attributes,
				normalizationAttributes);
	} else if (strcmp(fun_name, "tsbase_time") == 0) {
		tokenizeFunParameters(mapfun, &TOP_collections_parameters, &attributes, &normalizationAttributes);
		return SetNormalizedDynamicDataTime(shotNumber, data_block, nodeIndices, TOP_collections_parameters, attributes,
				normalizationAttributes);
	} else if (strcmp(fun_name, "tsbase_collect_with_channels") == 0) {
		char* unvalid_channels = NULL; //used for interfero_polarimeter IDS, example : invalid_channels:1,2
		tokenizeFunParametersWithChannels(mapfun, &unvalid_channels, &TOP_collections_parameters, &attributes,
				&normalizationAttributes);
		return SetNormalizedDynamicData(shotNumber, data_block, nodeIndices, TOP_collections_parameters, attributes,
				normalizationAttributes);
	} else if (strcmp(fun_name, "tsbase_time_with_channels") == 0) {
		char* unvalid_channels = NULL; //used for interfero_polarimeter IDS, example : invalid_channels:1,2
		tokenizeFunParametersWithChannels(mapfun, &unvalid_channels, &TOP_collections_parameters, &attributes,
				&normalizationAttributes);
		return SetNormalizedDynamicDataTime(shotNumber, data_block, nodeIndices, TOP_collections_parameters, attributes,
				normalizationAttributes);
	} else if (strcmp(fun_name, "ece_t_e_data") == 0) {
		char* ece_mapfun = NULL;
		ece_t_e_data(shotNumber, &ece_mapfun);
		tokenizeFunParameters(ece_mapfun, &TOP_collections_parameters, &attributes, &normalizationAttributes);
		UDA_LOG(UDA_LOG_DEBUG, "TOP_collections_parameters : %s\n", TOP_collections_parameters);
		int status = SetNormalizedDynamicData(shotNumber, data_block, nodeIndices, TOP_collections_parameters, attributes,
				normalizationAttributes);
		free(ece_mapfun);
		return status;

	} else if (strcmp(fun_name, "ece_t_e_time") == 0) {
		return ece_t_e_time(shotNumber, data_block, nodeIndices);
	} else if (strcmp(fun_name, "ece_time") == 0) {
		return ece_time(shotNumber, data_block, nodeIndices);
	} else if (strcmp(fun_name, "ece_frequencies") == 0) {
		return ece_frequencies(shotNumber, data_block, nodeIndices);
	} else if (strcmp(fun_name, "ece_frequencies_time") == 0) {
		return ece_harmonic_time(shotNumber, data_block, nodeIndices); //TODO
	} else if (strcmp(fun_name, "pf_passive_current_data") == 0) {
		return pf_passive_current_data(shotNumber, data_block, nodeIndices);
	} else if (strcmp(fun_name, "pf_passive_current_time") == 0) {
		return pf_passive_current_time(shotNumber, data_block, nodeIndices);
	} else if (strcmp(fun_name, "pf_active_current_data") == 0) {
		return pf_active_current_data(shotNumber, data_block, nodeIndices);
	} else if (strcmp(fun_name, "pf_active_current_time") == 0) {
		return pf_active_current_time(shotNumber, data_block, nodeIndices);
	} else if (strcmp(fun_name, "soft_x_rays_channels_power_density_data") == 0) {
		return soft_x_rays_channels_power_density_data(shotNumber, data_block, nodeIndices); //TODO
	} else if (strcmp(fun_name, "soft_x_rays_channels_power_density_time") == 0) {
		return soft_x_rays_channels_power_density_time(shotNumber, data_block, nodeIndices); //TODO
	} else if (strcmp(fun_name, "flt1D") == 0) {
		return flt1D(mapfun, shotNumber, data_block, nodeIndices); //TODO
	} else if (strcmp(fun_name, "summary_global_quantities_v_loop_value") == 0) {
		return summary_global_quantities_v_loop_value(shotNumber, data_block, nodeIndices); //TODO
	} else if (strcmp(fun_name, "summary_global_quantities_b0_value") == 0) {
		return summary_global_quantities_b0_value(shotNumber, data_block, nodeIndices); //TODO
	} else if (strcmp(fun_name, "summary_global_quantities_beta_tor_value") == 0) {
		return summary_global_quantities_beta_tor_value(shotNumber, data_block, nodeIndices); //TODO
	} else if (strcmp(fun_name, "summary_heating_current_drive_ec_power") == 0) {
		return summary_heating_current_drive_ec_power(shotNumber, data_block, nodeIndices); //TODO
	} else if (strcmp(fun_name, "summary_time") == 0) {
		return summary_time(shotNumber, data_block, nodeIndices); //TODO
	}else if (strcmp(fun_name, "lh_antennas_power") == 0) {
		return lh_antennas_power(shotNumber, data_block, nodeIndices);
	}else if (strcmp(fun_name, "lh_antennas_power_forward") == 0) {
		return lh_antennas_power_forward(shotNumber, data_block, nodeIndices);
	}else if (strcmp(fun_name, "lh_antennas_power_reflected") == 0) {
		return lh_antennas_power_reflected(shotNumber, data_block, nodeIndices);
	}else if (strcmp(fun_name, "lh_antennas_reflection_coefficient") == 0) {
		return lh_antennas_reflection_coefficient(shotNumber, data_block, nodeIndices);
	}else if (strcmp(fun_name, "lh_antennas_modules_power") == 0) {
		return lh_antennas_modules_power(shotNumber, data_block, nodeIndices);
	}else if (strcmp(fun_name, "lh_antennas_modules_power_forward") == 0) {
		return lh_antennas_modules_power_forward(shotNumber, data_block, nodeIndices);
	}else if (strcmp(fun_name, "lh_antennas_modules_power_reflected") == 0) {
		return lh_antennas_modules_power_reflected(shotNumber, data_block, nodeIndices);
	}else if (strcmp(fun_name, "lh_antennas_modules_reflection_coefficient") == 0) {
		return lh_antennas_modules_reflection_coefficient(shotNumber, data_block, nodeIndices);
	}else if (strcmp(fun_name, "lh_antennas_modules_phase") == 0) {
		return lh_antennas_modules_phase(shotNumber, data_block, nodeIndices);
	}else if (strcmp(fun_name, "lh_antennas_phase_average") == 0) {
		return lh_antennas_phase_average(shotNumber, data_block, nodeIndices);
	}else if (strcmp(fun_name, "lh_antennas_n_parallel_peak") == 0) {
		return lh_antennas_n_parallel_peak(shotNumber, data_block, nodeIndices);
	}else if (strcmp(fun_name, "lh_antennas_position_r") == 0) {
		return lh_antennas_position_r(shotNumber, data_block, nodeIndices);
	}else if (strcmp(fun_name, "lh_antennas_position_z") == 0) {
		return lh_antennas_position_z(shotNumber, data_block, nodeIndices);
	}else if (strcmp(fun_name, "lh_antennas_pressure_tank") == 0) {
		return lh_antennas_pressure_tank(shotNumber, data_block, nodeIndices); //TODO
	}else if (strcmp(fun_name, "barometry_gauge_pressure_data") == 0) {
		return barometry_gauge_pressure_data(shotNumber, data_block, nodeIndices); //TODO
	}else if (strcmp(fun_name, "barometry_gauge_pressure_time") == 0) {
		return barometry_gauge_pressure_time(shotNumber, data_block, nodeIndices); //TODO
	}/*else if (strcmp(fun_name, "test_fun") == 0) {
		return test_fun(shotNumber, data_block, nodeIndices); //TODO
	}*/
	else {
		const char* errorMsg = "WEST:ERROR: mapped C function not found in west_dynamic_data.c !\n";
		char msg[1000];
		strcpy(msg, errorMsg);
		sprintf(msg, ",function:%s,shot:%d\n", fun_name, shotNumber);
		UDA_LOG(UDA_LOG_ERROR,"%s\n", msg);
		UDA_LOG(UDA_LOG_DEBUG,"%s\n", msg);
	}
	free(fun_name);
	free(TOP_collections_parameters);
	free(attributes);
	free(normalizationAttributes);
	return 0;
}

int flt1D(const char* mappingValue, int shotNumber, DATA_BLOCK* data_block, int* nodeIndices)
{
	UDA_LOG(UDA_LOG_DEBUG, "Calling flt1D\n");
	char* diagnostic = NULL;
	char* object_name = NULL;
	int extractionIndex;
	char* normalizationAttributes = NULL;
	tokenize1DArcadeParameters(mappingValue, &diagnostic, &object_name, &extractionIndex, &normalizationAttributes);
	float normalizationFactor = 1.0;
	getNormalizationFactor(&normalizationFactor, normalizationAttributes);
	int status = setUDABlockSignalFromArcade(object_name, shotNumber, extractionIndex, data_block, nodeIndices, normalizationFactor);
	if (status != 0) {
		int err = 901;
		char errorMsg[1000];
		strcpy(errorMsg, "WEST:ERROR (flt1D): unable to get object:");
		sprintf(errorMsg, "%s,shot:%d\n", object_name, shotNumber);
		//UDA_LOG(UDA_LOG_ERROR, "%s", errorMsg);
		addIdamError(CODEERRORTYPE, errorMsg, err, "");
	}
	free(diagnostic);
	free(object_name);
	return status;
}
