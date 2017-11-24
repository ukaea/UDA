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

void summary_global_quantities_v_loop_value(int shotNumber, DATA_BLOCK* data_block, int* nodeIndices)
{
	int extractions [4];
	extractions[0] = 1;
	extractions[1] = 3;
	extractions[2] = 4;
	extractions[3] = 6;
	float *averaged_data_time = NULL;
	float* averaged_data = NULL;
	int len;
	averageArcadeSignal("GMAG_VLOOP", shotNumber, extractions, 4, &averaged_data_time, &averaged_data, &len);

	float *time2 = NULL;
	float *data2 = NULL;
	int len2;

	int status = getArcadeSignal("GMAG_FV", shotNumber, 2, &time2, &data2, &len2, 1.0);

	if (status != 0) {
		int err = 901;
		char* errorMsg = "WEST:ERROR: unable to get object ";
		strcat(errorMsg, "GMAG_FV");
		strcat(errorMsg, " in west_summary:summary_global_quantities_v_loop_value method.");
		addIdamError(CODEERRORTYPE, errorMsg, err, "");
	}

	float *ip_time = NULL;
	float *ip_data = NULL;
	int ip_len;

	status = getArcadeSignal("SMAG_IP", shotNumber, 1, &ip_time, &ip_data, &ip_len, 1.0);

	if (status != 0) {
		int err = 901;
		char* errorMsg = "WEST:ERROR: unable to get object ";
		strcat(errorMsg, "SMAG_IP");
		strcat(errorMsg, " in west_summary:summary_global_quantities_v_loop_value method.");
		addIdamError(CODEERRORTYPE, errorMsg, err, "");
	}

	float treshold = 10000;

	float *mergedData = NULL;
	merge2Signals_according_to_ip_treshold(&mergedData, len, averaged_data, data2, ip_data, treshold);

	SetDynamicData(data_block, len, averaged_data_time, mergedData);
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
		strcat(errorMsg, " in west_summary:summary_global_quantities_beta_tor_value method.");
		addIdamError(CODEERRORTYPE, errorMsg, err, "");
	}

	IDAM_LOG(UDA_LOG_DEBUG, "Getting ratio total_energy_data/volume_data...\n");
	float *ratio = NULL;
	status = signalsRatio(&ratio, total_energy_data, volume_data, total_energy_len, volume_len);

	if (status != 0) {
		int err = 901;
		char* errorMsg = "WEST:ERROR: unable to get ratio total_energy/volume";
		strcat(errorMsg, " in west_summary:summary_global_quantities_beta_tor_value method.");
		addIdamError(CODEERRORTYPE, errorMsg, err, "");
	}

	float *itor_time;
	float *itor_data;
	int itor_len;
	char* itor_sigName = "GMAG_ITOR";
	int itor_extractionIndex = 1;

	float normalizationFactor = 4.*M_PI*1.e-7*18.*2028./(2.*M_PI*2.42);
	IDAM_LOG(UDA_LOG_DEBUG, "Getting GMAG_ITOR%1...\n");
	status = getArcadeSignal(itor_sigName, shotNumber, itor_extractionIndex, &itor_time, &itor_data, &itor_len, normalizationFactor);

	if (status != 0) {
		int err = 901;
		char* errorMsg = "WEST:ERROR: unable to get object ";
		strcat(errorMsg, itor_sigName);
		strcat(errorMsg, " in west_summary:summary_global_quantities_beta_tor_value method.");
		addIdamError(CODEERRORTYPE, errorMsg, err, "");
	}

	IDAM_LOG(UDA_LOG_DEBUG, "Getting square of itor_data...\n");
	float *b0_square = NULL;
	signalsSquare(&b0_square, itor_data, itor_len);

	IDAM_LOG(UDA_LOG_DEBUG, "Getting ratio of energy_by_volume by b0_square ...\n");
	float *energy_by_volume_by_b0_square = NULL;
	status = signalsRatio(&energy_by_volume_by_b0_square, ratio, b0_square, total_energy_len, itor_len);

	float cste = (2./3.)*2*M_PI*1e-7;

	IDAM_LOG(UDA_LOG_DEBUG, "Multiply result with constant ...\n");
	multiply(energy_by_volume_by_b0_square, itor_len, cste);

	SetDynamicData(data_block, itor_len, itor_time, energy_by_volume_by_b0_square);
}

void summary_global_quantities_beta_tor_norm_value(int shotNumber, DATA_BLOCK* data_block, int* nodeIndices)
{
	//volume
	//----------------------------------
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
		strcat(errorMsg, " in west_summary:summary_global_quantities_beta_tor_norm_value method.");
		addIdamError(CODEERRORTYPE, errorMsg, err, "");
	}

	//total energy
	//----------------------------------
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
		strcat(errorMsg, " in west_summary:summary_global_quantities_beta_tor_norm_value method.");
		addIdamError(CODEERRORTYPE, errorMsg, err, "");
	}

	//b0_value
	//----------------------------------
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
		strcat(errorMsg, " in west_summary:summary_global_quantities_beta_tor_norm_value method.");
		addIdamError(CODEERRORTYPE, errorMsg, err, "");
	}

	//ip value
	//----------------------------------
	/*float *ifreeb_time;
	float *ifreeb_data;
	int ifreeb_len;
	char* ifreeb_sigName = "GMAG_IFREEB";
	int ifreeb_extractionIndex = 1;

	normalizationFactor = 1.0;
	status = getArcadeSignal(ifreeb_sigName, shotNumber, ifreeb_extractionIndex, &ifreeb_time, &ifreeb_data, &ifreeb_len, normalizationFactor);

	if (status != 0) {
		int err = 901;
		char* errorMsg = "WEST:ERROR: unable to get object ";
		strcat(errorMsg, ifreeb_sigName);
		strcat(errorMsg, " in west_summary:summary_global_quantities_beta_tor_norm_value method.");
		addIdamError(CODEERRORTYPE, errorMsg, err, "");
	}

	float *smagip_time;
	float *smagip_data;
	int smagip_len;
	char* smagip_sigName = "SMAG_IP";
	int smagip_extractionIndex = 1;

	normalizationFactor = 1.0;
	status = getArcadeSignal(smagip_sigName, shotNumber, smagip_extractionIndex, &smagip_time, &smagip_data, &smagip_len, normalizationFactor);

	if (status != 0) {
		int err = 901;
		char* errorMsg = "WEST:ERROR: unable to get object ";
		strcat(errorMsg, smagip_sigName);
		strcat(errorMsg, " in west_summary:summary_global_quantities_beta_tor_norm_value method.");
		addIdamError(CODEERRORTYPE, errorMsg, err, "");
	}

	float treshold = 10000;
	float *ip_data = NULL;
	merge2Signals_according_to_ip_treshold(&ip_data, smagip_len, ifreeb_data, smagip_data, smagip_data, treshold);*/

	const float treshold = 10000;
	float *ip_data = NULL;
	float *ip_time = NULL;
	int ip_len;
	ip_value(&ip_data, ip_time, &ip_len, shotNumber, data_block, treshold);

	//minor radius
	//----------------------------------
	float *minor_radius_time;
	float *minor_radius_data;
	int minor_radius_len;
	char* minor_radius_sigName = "GMAG_GEOM";
	int minor_radius_extractionIndex = 3;

	status = getArcadeSignal(minor_radius_sigName, shotNumber, minor_radius_extractionIndex,
			&minor_radius_time, &minor_radius_data, &minor_radius_len, 1.0);

	if (status != 0) {
		int err = 901;
		char* errorMsg = "WEST:ERROR: unable to get object ";
		strcat(errorMsg, minor_radius_sigName);
		strcat(errorMsg, " in west_summary:summary_global_quantities_beta_tor_norm_value method.");
		addIdamError(CODEERRORTYPE, errorMsg, err, "");
	}

	//Total_energy x minor radius
	//----------------------------------
	float *total_enery_times_minor_radius = NULL;
	multiplySignals(&total_enery_times_minor_radius, total_energy_data, minor_radius_data, total_energy_len);


	//(Total_energy x minor radius)/volume
	//----------------------------------
	float *total_enery_times_minor_radius_by_volume = NULL;
	status = signalsRatio(&total_enery_times_minor_radius_by_volume, total_enery_times_minor_radius, ip_data, total_energy_len, volume_len);

	if (status != 0) {
		int err = 901;
		char* errorMsg = "WEST:ERROR: unable to compute ratio in in west_summary:summary_global_quantities_beta_tor_norm_value method.";
		addIdamError(CODEERRORTYPE, errorMsg, err, "");
	}

	//(Total_energy x minor radius)/volume/b0
	//----------------------------------
	float *total_enery_times_minor_radius_by_volume_by_b0 = NULL;
	status = signalsRatio(&total_enery_times_minor_radius_by_volume_by_b0, total_enery_times_minor_radius_by_volume, itor_data, total_energy_len, itor_len);

	if (status != 0) {
		int err = 901;
		char* errorMsg = "WEST:ERROR: unable to compute second ratio in in west_summary:summary_global_quantities_beta_tor_norm_value method.";
		addIdamError(CODEERRORTYPE, errorMsg, err, "");
	}

	//(Total_energy x minor radius)/volume/b0/ip
	//----------------------------------
	float *total_enery_times_minor_radius_by_volume_by_b0_by_ip = NULL;
	status = signalsRatio(&total_enery_times_minor_radius_by_volume_by_b0_by_ip, total_enery_times_minor_radius_by_volume_by_b0, ip_data, total_energy_len, ip_len);

	if (status != 0) {
		int err = 901;
		char* errorMsg = "WEST:ERROR: unable to compute third ratio in in west_summary:summary_global_quantities_beta_tor_norm_value method.";
		addIdamError(CODEERRORTYPE, errorMsg, err, "");
	}

	float cste = 100*1.6*M_PI/3.;

	multiply(total_enery_times_minor_radius_by_volume_by_b0_by_ip, ip_len, cste);

	SetDynamicData(data_block, total_energy_len, ip_time, total_enery_times_minor_radius_by_volume_by_b0_by_ip);
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
		strcat(errorMsg, " in west_summary:summary_global_quantities_b0_value method.");
		addIdamError(CODEERRORTYPE, errorMsg, err, "");
	}

	SetDynamicData(data_block, itor_len, itor_time, itor_data);
}

void summary_heating_current_drive_ec_power(int shotNumber, DATA_BLOCK* data_block, int* nodeIndices) {

	int k = nodeIndices[0]; //starts from 1
	if (k == 1) {
		char* mappingValue = "flt1D;DMAG:GMAG_BILAN:2";
		flt1D(mappingValue, shotNumber, data_block, nodeIndices);
	}
	else if (k == 2) {
		char* mappingValue = "flt1D;DMAG:GMAG_BILAN:3";
		flt1D(mappingValue, shotNumber, data_block, nodeIndices);
	}
	else if (k == 3) {
		char* mappingValue = "flt1D;DMAG:GMAG_BILAN:4";
		flt1D(mappingValue, shotNumber, data_block, nodeIndices);
	}
}

void summary_heating_current_drive_ec_power_source(int shotNumber, DATA_BLOCK* data_block, int* nodeIndices) {

	int k = nodeIndices[0]; //starts from 1
	if (k == 1) {
		setReturnDataString(data_block, "LHCD", NULL);
	}
	else if (k == 2) {
		setReturnDataString(data_block, "ICRH", NULL);
	}
	else if (k == 3) {
		setReturnDataString(data_block, "ECRH", NULL);
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
		addIdamError(CODEERRORTYPE, "WEST:ERROR: unable to get GMAG_FV%3", err, "");
	}

	float *ip_time = NULL;
	float *ip_data;
	int ip_len;
	/*char* current_sigName = "SMAG_IP";
	int current_extractionIndex = 0; //simple signal
	status = getArcadeSignal(current_sigName, shotNumber, current_extractionIndex, &current_time, &current_data, &current_len, 1);

	if (status != 0) {
		int err = 901;
		addIdamError(CODEERRORTYPE, "WEST:ERROR: unable to get SMAG_IP (extraction index 1)", err, "");
	}*/
	const float treshold = 10000;
	ip_value(&ip_data, ip_time, &ip_len, shotNumber, data_block, treshold);

	float *resistiveData;
	status = signalsRatio(&resistiveData, voltage_data, ip_data, voltage_len, ip_len);
	if (status != 0) {
		int err = 901;
		addIdamError(CODEERRORTYPE, "WEST:ERROR: unable to compute the ratio for tau_resistive", err, "");
	}

	SetDynamicData(data_block, ip_len, ip_time, resistiveData);
}

void ip_value(float **ip_data, float *ip_time, int *ip_len, int shotNumber, DATA_BLOCK* data_block, const float treshold) {
	//ip value
	//----------------------------------
	float *ifreeb_time;
	float *ifreeb_data;
	int ifreeb_len;
	char* ifreeb_sigName = "GMAG_IFREEB";
	int ifreeb_extractionIndex = 1;

	float normalizationFactor = 1.0;
	int status = getArcadeSignal(ifreeb_sigName, shotNumber, ifreeb_extractionIndex, &ifreeb_time, &ifreeb_data, &ifreeb_len, normalizationFactor);

	if (status != 0) {
		int err = 901;
		char* errorMsg = "WEST:ERROR: unable to get object ";
		strcat(errorMsg, ifreeb_sigName);
		strcat(errorMsg, " in west_summary:summary_global_quantities_beta_tor_norm_value method.");
		addIdamError(CODEERRORTYPE, errorMsg, err, "");
	}

	float *smagip_time;
	float *smagip_data;
	int smagip_len;
	char* smagip_sigName = "SMAG_IP";
	int smagip_extractionIndex = 1;

	normalizationFactor = 1.0;
	status = getArcadeSignal(smagip_sigName, shotNumber, smagip_extractionIndex, &smagip_time, &smagip_data, &smagip_len, normalizationFactor);

	if (status != 0) {
		int err = 901;
		char* errorMsg = "WEST:ERROR: unable to get object ";
		strcat(errorMsg, smagip_sigName);
		strcat(errorMsg, " in west_summary:summary_global_quantities_beta_tor_norm_value method.");
		addIdamError(CODEERRORTYPE, errorMsg, err, "");
	}
	ip_time = ifreeb_time;
	*ip_len = ifreeb_len;
	merge2Signals_according_to_ip_treshold(ip_data, smagip_len, ifreeb_data, smagip_data, smagip_data, treshold);
}
