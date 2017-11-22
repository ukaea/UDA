#include "west_summary.h"

#include <string.h>
#include <stdio.h>
#include <assert.h>
#include <math.h>

#include <logging/logging.h>
#include <clientserver/initStructs.h>
#include <clientserver/errorLog.h>
#include <clientserver/stringUtils.h>
#include <clientserver/udaTypes.h>
#include <plugins/udaPlugin.h>
#include <structures/struct.h>

#include "west_utilities.h"
#include "west_dyn_data_utilities.h"

#include "west_static_data_utilities.h"


void summary_flt1D(const char* mappingValue, int shotNumber, DATA_BLOCK* data_block, int* nodeIndices)
{
	char* diagnostic = NULL;
	char* object_name = NULL;
	int extractionIndex;
	tokenize1DArcadeParameters(mappingValue, &diagnostic, &object_name, &extractionIndex);
	int status = setUDABlockSignalFromArcade(object_name, shotNumber, extractionIndex, data_block, nodeIndices, 1);
	if (status != 0) {
		int err = 901;
		char* errorMsg = "WEST:ERROR: unable to get object ";
		strcat(errorMsg, object_name);
		strcat(errorMsg, " in west_summary:summary_flt1D method.");
		addIdamError(CODEERRORTYPE, errorMsg, err, "");
	}
}

void summary_contrib_flt1D(const char* mappingValue, int shotNumber, DATA_BLOCK* data_block, int* nodeIndices)
{
	char* diagnostic = NULL;
	char* object_name = NULL;
	int extractionIndex;

	char* diagnostic2 = NULL;
	char* object_name2 = NULL;
	int extractionIndex2;

	tokenize1DArcadeParameters2(mappingValue, &diagnostic, &object_name, &extractionIndex, &diagnostic2, &object_name2, &extractionIndex2);
	int status = setUDABlockSignalFromArcade2(shotNumber, object_name, extractionIndex, object_name2, extractionIndex2, data_block, nodeIndices, 1);
	if (status != 0) {
		int err = 901;
		char* errorMsg = "WEST:ERROR: unable to get object ";
		strcat(errorMsg, object_name);
		strcat(errorMsg, " in west_summary:summary_contrib_flt1D method.");
		addIdamError(CODEERRORTYPE, errorMsg, err, "");
	}
}

void summary_global_quantities_v_loop_value(int shotNumber, DATA_BLOCK* data_block, int* nodeIndices)
{
	int extractions [4];
	extractions[0] = 1;
	extractions[1] = 3;
	extractions[2] = 4;
	extractions[3] = 6;
	float **time = NULL;
	float** averaged_data = NULL;
	int len;
	averageArcadeSignal("GMAG_VLOOP", shotNumber, extractions, 4, time, averaged_data, &len);

	float **time2 = NULL;
	float **data2 = NULL;
	int len2;

	int status = getArcadeSignal("GMAG_FV", shotNumber, 2, time2, data2, &len2, 1.0);

	if (status != 0) {
		int err = 901;
		char* errorMsg = "WEST:ERROR: unable to get object ";
		strcat(errorMsg, "GMAG_FV");
		strcat(errorMsg, " in west_summary:summary_global_quantities_v_loop_value method.");
		addIdamError(CODEERRORTYPE, errorMsg, err, "");
	}

	float **ip_time = NULL;
	float **ip_data = NULL;
	int ip_len;

	status = getArcadeSignal("SMAG_IP", shotNumber, 1, ip_time, ip_data, &ip_len, 1.0);

	if (status != 0) {
		int err = 901;
		char* errorMsg = "WEST:ERROR: unable to get object ";
		strcat(errorMsg, "SMAG_IP");
		strcat(errorMsg, " in west_summary:summary_global_quantities_v_loop_value method.");
		addIdamError(CODEERRORTYPE, errorMsg, err, "");
	}

	float treshold = 10000;
	merge2Signals_according_to_ip_treshold_setUDABlock(data_block, len, *time, *averaged_data, *data2, *ip_data, treshold);

}

void summary_global_quantities_r0_value(int shotNumber, DATA_BLOCK* data_block, int* nodeIndices)
{
	setReturnDataFloatScalar(data_block, 2.42, NULL);
}

void summary_global_quantities_beta_tor_value(int shotNumber, DATA_BLOCK* data_block, int* nodeIndices)
{
	float *volume_time;
	float *volume_data;
	int volume_len;
	char* volume_sigName = "GMAG_GEOM";
	int volume_extractionIndex = 13;

	int status = getArcadeSignal(volume_sigName, shotNumber, volume_extractionIndex,
			&volume_time, &volume_data, &volume_len, 1.0);

	if (status != 0) {
		int err = 901;
		char* errorMsg = "WEST:ERROR: unable to get object ";
		strcat(errorMsg, volume_sigName);
		strcat(errorMsg, " in west_summary:summary_global_quantities_beta_tor_value method.");
		addIdamError(CODEERRORTYPE, errorMsg, err, "");
	}

	float *total_energy_time;
	float *total_energy_data;
	int total_energy_len;
	char* total_energy_sigName = "GMAG_BILAN";
	int total_energy_extractionIndex = 10;

	status = getArcadeSignal(total_energy_sigName, shotNumber, total_energy_extractionIndex,
			&total_energy_time, &total_energy_data, &total_energy_len, 1.0);

	if (status != 0) {
		int err = 901;
		char* errorMsg = "WEST:ERROR: unable to get object ";
		strcat(errorMsg, total_energy_sigName);
		strcat(errorMsg, " in west_summary:summary_global_quantities_v_loop_value method.");
		addIdamError(CODEERRORTYPE, errorMsg, err, "");
	}

	float **ratio = NULL;
	status = signalsRatio(ratio, total_energy_data, volume_data, total_energy_len, volume_len);

	if (status != 0) {
		int err = 901;
		char* errorMsg = "WEST:ERROR: unable to get ratio total_energy/volume";
		strcat(errorMsg, " in west_summary:summary_global_quantities_v_loop_value method.");
		addIdamError(CODEERRORTYPE, errorMsg, err, "");
	}

	float *itor_time;
	float *itor_data;
	int itor_len;
	char* itor_sigName = "GMAG_ITOR";
	int itor_extractionIndex = 1;

	float normalizationFactor = 4.*M_PI*1.e-7*18.*2028./(2.*M_PI*2.42);
	status = getArcadeSignal(itor_sigName, shotNumber, itor_extractionIndex, &itor_time, &itor_data, &itor_len, normalizationFactor);

	if (status != 0) {
		int err = 901;
		char* errorMsg = "WEST:ERROR: unable to get object ";
		strcat(errorMsg, itor_sigName);
		strcat(errorMsg, " in west_summary:summary_global_quantities_beta_tor_value method.");
		addIdamError(CODEERRORTYPE, errorMsg, err, "");
	}

	float **b0_square = NULL;
	signalsSquare(b0_square, itor_data, itor_len);

	float **energy_by_volume_by_b0_square = NULL;
	status = signalsRatio(energy_by_volume_by_b0_square, *ratio, *b0_square, total_energy_len, itor_len);

	float cste = (2./3.)*2*M_PI*1e-7;

	multiply(*energy_by_volume_by_b0_square, itor_len, cste);

	SetDynamicData(data_block, itor_len, itor_time, *energy_by_volume_by_b0_square);
}

void summary_global_quantities_beta_tor_norm_value(int shotNumber, DATA_BLOCK* data_block, int* nodeIndices)
{
	float *volume_time;
	float *volume_data;
	int volume_len;
	char* volume_sigName = "GMAG_GEOM";
	int volume_extractionIndex = 13;

	int status = getArcadeSignal(volume_sigName, shotNumber, volume_extractionIndex,
			&volume_time, &volume_data, &volume_len, 1.0);

	if (status != 0) {
		int err = 901;
		char* errorMsg = "WEST:ERROR: unable to get object ";
		strcat(errorMsg, volume_sigName);
		strcat(errorMsg, " in west_summary:summary_global_quantities_beta_tor_value method.");
		addIdamError(CODEERRORTYPE, errorMsg, err, "");
	}

	float *total_energy_time;
	float *total_energy_data;
	int total_energy_len;
	char* total_energy_sigName = "GMAG_BILAN";
	int total_energy_extractionIndex = 10;

	status = getArcadeSignal(total_energy_sigName, shotNumber, total_energy_extractionIndex,
			&total_energy_time, &total_energy_data, &total_energy_len, 1.0);

	if (status != 0) {
		int err = 901;
		char* errorMsg = "WEST:ERROR: unable to get object ";
		strcat(errorMsg, total_energy_sigName);
		strcat(errorMsg, " in west_summary:summary_global_quantities_v_loop_value method.");
		addIdamError(CODEERRORTYPE, errorMsg, err, "");
	}

	float **ratio = NULL;
	status = signalsRatio(ratio, total_energy_data, volume_data, total_energy_len, volume_len);

	if (status != 0) {
		int err = 901;
		char* errorMsg = "WEST:ERROR: unable to get ratio total_energy/volume";
		strcat(errorMsg, " in west_summary:summary_global_quantities_v_loop_value method.");
		addIdamError(CODEERRORTYPE, errorMsg, err, "");
	}

	float *itor_time;
	float *itor_data;
	int itor_len;
	char* itor_sigName = "GMAG_ITOR";
	int itor_extractionIndex = 1;

	float normalizationFactor = 4.*M_PI*1.e-7*18.*2028./(2.*M_PI*2.42);
	status = getArcadeSignal(itor_sigName, shotNumber, itor_extractionIndex, &itor_time, &itor_data, &itor_len, normalizationFactor);

	if (status != 0) {
		int err = 901;
		char* errorMsg = "WEST:ERROR: unable to get object ";
		strcat(errorMsg, itor_sigName);
		strcat(errorMsg, " in west_summary:summary_global_quantities_beta_tor_value method.");
		addIdamError(CODEERRORTYPE, errorMsg, err, "");
	}

	float **b0_square = NULL;
	signalsSquare(b0_square, itor_data, itor_len);

	float **energy_by_volume_by_b0_square = NULL;
	status = signalsRatio(energy_by_volume_by_b0_square, *ratio, *b0_square, total_energy_len, itor_len);

	float cste = (2./3.)*2*M_PI*1e-7;

	multiply(*energy_by_volume_by_b0_square, itor_len, cste);

	SetDynamicData(data_block, itor_len, itor_time, *energy_by_volume_by_b0_square);
}

void summary_global_quantities_b0_value(int shotNumber, DATA_BLOCK* data_block, int* nodeIndices)
{
	float *itor_time;
	float *itor_data;
	int itor_len;
	char* itor_sigName = "GMAG_ITOR";
	int itor_extractionIndex = 1;

	float normalizationFactor = 4*M_PI*1e-7*18*2028/(2*M_PI*2.42);
	int status = getArcadeSignal(itor_sigName, shotNumber, itor_extractionIndex, &itor_time, &itor_data, &itor_len, normalizationFactor);

	if (status != 0) {
		int err = 901;
		char* errorMsg = "WEST:ERROR: unable to get object ";
		strcat(errorMsg, itor_sigName);
		strcat(errorMsg, " in west_summary:summary_global_quantities_v_loop_value method.");
		addIdamError(CODEERRORTYPE, errorMsg, err, "");
	}
}


void summary_global_quantities_tau_resistance_value(int shotNumber, DATA_BLOCK* data_block, int* nodeIndices)
{
	float *voltage_time;
	float *voltage_data;
	int voltage_len;
	char* voltage_sigName = "GMAG_FV";
	int voltage_extractionIndex = 3;

	int status = getArcadeSignal(voltage_sigName, shotNumber, voltage_extractionIndex, &voltage_time, &voltage_data, &voltage_len, 1);

	if (status != 0) {
		int err = 901;
		addIdamError(CODEERRORTYPE, "WEST:ERROR: unable to get GMAG_FV (extraction index 2)", err, "");
	}

	float *current_time;
	float *current_data;
	int current_len;
	char* current_sigName = "SMAG_IP";
	int current_extractionIndex = 1;
	status = getArcadeSignal(current_sigName, shotNumber, current_extractionIndex, &current_time, &current_data, &current_len, 1);

	if (status != 0) {
		int err = 901;
		addIdamError(CODEERRORTYPE, "WEST:ERROR: unable to get SMAG_IP (extraction index 1)", err, "");
	}

	float *resistiveData;
	status = signalsRatio(&resistiveData, voltage_data, current_data, voltage_len, current_len);
	if (status != 0) {
		int err = 901;
		addIdamError(CODEERRORTYPE, "WEST:ERROR: unable to compute the ratio for tau_resistive", err, "");
	}

	SetDynamicData(data_block, voltage_len, voltage_time, resistiveData);
}

/*void summary_global_quantities_ip_value(int shotNumber, DATA_BLOCK* data_block, int* nodeIndices)
{
	char* sigName = "SMAG_IP";
	int extractionIndex = 1;

	int status = setUDABlockSignalFromArcade(sigName, shotNumber, extractionIndex, data_block, nodeIndices, 1);
	if (status != 0) {
		int err = 901;
		addIdamError(CODEERRORTYPE, "WEST:ERROR: unable to get summary_global_quantities_ip_value", err, "");
	}
}

void summary_global_quantities_ohm_value(int shotNumber, DATA_BLOCK* data_block, int* nodeIndices)
{
	char* sigName = "GMAG_BILAN";
	int extractionIndex = 1;

	int status = setUDABlockSignalFromArcade(sigName, shotNumber, extractionIndex, data_block, nodeIndices, 1);
	if (status != 0) {
		int err = 901;
		addIdamError(CODEERRORTYPE, "WEST:ERROR: unable to get summary_global_quantities_ohm_value", err, "");
	}
}

void summary_global_quantities_v_loop_value(int shotNumber, DATA_BLOCK* data_block, int* nodeIndices)
{
	char* sigName = "GMAG_FV";
	int extractionIndex = 2;

	int status = setUDABlockSignalFromArcade(sigName, shotNumber, extractionIndex, data_block, nodeIndices, 1);
	if (status != 0) {
		int err = 901;
		addIdamError(CODEERRORTYPE, "WEST:ERROR: unable to get summary_global_quantities_v_loop_value", err, "");
	}
}

void summary_global_quantities_li_value(int shotNumber, DATA_BLOCK* data_block, int* nodeIndices)
{
	char* sigName = "GMAG_BELI";
	int extractionIndex = 2; //TODO to confirm

	int status = setUDABlockSignalFromArcade(sigName, shotNumber, extractionIndex, data_block, nodeIndices, 1);
	if (status != 0) {
		int err = 901;
		addIdamError(CODEERRORTYPE, "WEST:ERROR: unable to get summary_global_quantities_li_value", err, "");
	}
}

void summary_global_quantities_beta_pol_value(int shotNumber, DATA_BLOCK* data_block, int* nodeIndices)
{
	char* sigName = "GMAG_BELI";
	int extractionIndex = 1; //or 3 ?

	int status = setUDABlockSignalFromArcade(sigName, shotNumber, extractionIndex, data_block, nodeIndices, 1);
	if (status != 0) {
		int err = 901;
		addIdamError(CODEERRORTYPE, "WEST:ERROR: unable to get summary_global_quantities_beta_pol_value", err, "");
	}
}

void summary_global_quantities_energy_diamagnetic_value(int shotNumber, DATA_BLOCK* data_block, int* nodeIndices)
{
	char* sigName = "GMAG_BILAN";
	int extractionIndex = 7;

	int status = setUDABlockSignalFromArcade(sigName, shotNumber, extractionIndex, data_block, nodeIndices, 1);
	if (status != 0) {
		int err = 901;
		addIdamError(CODEERRORTYPE, "WEST:ERROR: unable to get summary_global_quantities_energy_diamagnetic_value", err, "");
	}
}

void summary_global_quantities_energy_total_value(int shotNumber, DATA_BLOCK* data_block, int* nodeIndices)
{
	char* sigName = "GMAG_BILAN";
	int extractionIndex = 10;

	int status = setUDABlockSignalFromArcade(sigName, shotNumber, extractionIndex, data_block, nodeIndices, 1);
	if (status != 0) {
		int err = 901;
		addIdamError(CODEERRORTYPE, "WEST:ERROR: unable to get summary_global_quantities_energy_total_value", err, "");
	}
}

void summary_global_quantities_energy_b_field_pol_value(int shotNumber, DATA_BLOCK* data_block, int* nodeIndices)
{
	char* sigName = "GMAG_BILAN";
	int extractionIndex = 9;

	int status = setUDABlockSignalFromArcade(sigName, shotNumber, extractionIndex, data_block, nodeIndices, 1);
	if (status != 0) {
		int err = 901;
		addIdamError(CODEERRORTYPE, "WEST:ERROR: unable to get summary_global_quantities_energy_b_field_pol_value", err, "");
	}
}

void summary_global_quantities_volume_value(int shotNumber, DATA_BLOCK* data_block, int* nodeIndices)
{
	char* sigName = "GMAG_GEOM";
	int extractionIndex = 13;

	int status = setUDABlockSignalFromArcade(sigName, shotNumber, extractionIndex, data_block, nodeIndices, 1);
	if (status != 0) {
		int err = 901;
		addIdamError(CODEERRORTYPE, "WEST:ERROR: unable to get summary_global_quantities_volume_value", err, "");
	}
}

void summary_global_quantities_r0_value(int shotNumber, DATA_BLOCK* data_block, int* nodeIndices)
{
	char* prod_name = "DMAG";
	char* object_name = "GMAG_GEOM";
	char* param_name = "R";
	int index = 0;

	int status = setUDABlockFromArcadeScalarField(UDA_TYPE_FLOAT, prod_name, object_name, param_name, index,
			shotNumber, data_block, nodeIndices, 1);

	if (status != 0) {
		int err = 901;
		addIdamError(CODEERRORTYPE, "WEST:ERROR: unable to get summary_global_quantities_r0_value", err, "");
	}
}

void summary_global_quantities_b0_value(int shotNumber, DATA_BLOCK* data_block, int* nodeIndices)
{

}

void summary_global_quantities_tau_energy_value(int shotNumber, DATA_BLOCK* data_block, int* nodeIndices)
{
	char* sigName = "GMAG_BILAN";
	int extractionIndex = 8;

	int status = setUDABlockSignalFromArcade(sigName, shotNumber, extractionIndex, data_block, nodeIndices, 1);
	if (status != 0) {
		int err = 901;
		addIdamError(CODEERRORTYPE, "WEST:ERROR: unable to get summary_global_quantities_tau_energy_value", err, "");
	}
}

void summary_global_quantities_tau_resistive_value(int shotNumber, DATA_BLOCK* data_block, int* nodeIndices)
{
	float *voltage_time;
	float *voltage_data;
	int voltage_len;
	char* voltage_sigName = "GMAG_FV";
	int voltage_extractionIndex = 2;

	int status = getArcadeSignal(voltage_sigName, shotNumber, voltage_extractionIndex, &voltage_time, &voltage_data, &voltage_len, 1);

	if (status != 0) {
		int err = 901;
		addIdamError(CODEERRORTYPE, "WEST:ERROR: unable to get GMAG_FV (extraction index 2)", err, "");
	}

	float *current_time;
	float *current_data;
	int current_len;
	char* current_sigName = "SMAG_IP";
	int current_extractionIndex = 1;
	int status = getArcadeSignal(current_sigName, shotNumber, current_extractionIndex, &current_time, &current_data, &current_len, 1);

	if (status != 0) {
		int err = 901;
		addIdamError(CODEERRORTYPE, "WEST:ERROR: unable to get SMAG_IP (extraction index 1)", err, "");
	}

	float *resistiveData;
	status = signalsRatio(&resistiveData, voltage_data, current_data, voltage_len, current_len);
	if (status != 0) {
		int err = 901;
		addIdamError(CODEERRORTYPE, "WEST:ERROR: unable to compute the ratio for tau_resistive", err, "");
	}

	SetDynamicData(data_block, voltage_len, voltage_time, resistiveData);
}

void summary_local_magnetic_axis_position_r(int shotNumber, DATA_BLOCK* data_block, int* nodeIndices)
{
	char* sigName = "GMAG_BARY";
	int extractionIndex = 3;

	int status = setUDABlockSignalFromArcade(sigName, shotNumber, extractionIndex, data_block, nodeIndices, 1);
	if (status != 0) {
		int err = 901;
		addIdamError(CODEERRORTYPE, "WEST:ERROR: unable to get summary_local_magnetic_axis_position_r", err, "");
	}
}

void summary_local_magnetic_axis_position_z(int shotNumber, DATA_BLOCK* data_block, int* nodeIndices)
{
	char* sigName = "GMAG_BARY";
	int extractionIndex = 4;

	int status = setUDABlockSignalFromArcade(sigName, shotNumber, extractionIndex, data_block, nodeIndices, 1);
	if (status != 0) {
		int err = 901;
		addIdamError(CODEERRORTYPE, "WEST:ERROR: unable to get summary_local_magnetic_axis_position_z", err, "");
	}
}*/





