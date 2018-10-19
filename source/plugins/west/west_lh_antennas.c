#include "west_lh_antennas.h"

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

#include "west_utilities.h"
#include "west_dyn_data_utilities.h"
#include "ts_rqparam.h"
#include "west_static_data_utilities.h"


void lh_antennas_throwsIdamError(int status, char* methodName, char * objectName, int shotNumber) {
	int err = 901;
	char errorMsg[1000];
	sprintf(errorMsg, "%s(%s),object:%s,shot:%d,err:%d\n", "WEST:ERROR", methodName, objectName, shotNumber,status);
	//UDA_LOG(UDA_LOG_ERROR, "%s", errorMsg);
	addIdamError(CODEERRORTYPE, errorMsg, err, "");
}

int lh_antennas_power(int shotNumber, DATA_BLOCK* data_block, int* nodeIndices) {
	UDA_LOG(UDA_LOG_DEBUG, "Calling lh_antennas_power\n");
	float f = 1e6; //conversion factor, power is given in MW in Arcade
	int extractionIndex = 0; //indicates that signal does not belong to a group
	char *object_name = "SHYBPTOT";
	int status = setUDABlockSignalFromArcade(object_name, shotNumber, extractionIndex, data_block, nodeIndices, f);
	if (status != 0) {
		lh_antennas_throwsIdamError(status, "lh_antennas_power", object_name, shotNumber);
	}
	UDA_LOG(UDA_LOG_DEBUG, "Returning from lh_antennas_power\n");
	return status;
}

int lh_antennas_power_forward(int shotNumber, DATA_BLOCK* data_block, int* nodeIndices) {

	float f = 1e6; //conversion factor, power is given in MW in Arcade
	int antennaId = nodeIndices[0]; //=1 or 2
	int status = -1;
	int extractionIndex = 0; //indicates that signal does not belong to a group
	if (antennaId == 1) {
		char *object_name = "SHYBPFORW1";
		status = setUDABlockSignalFromArcade(object_name, shotNumber, extractionIndex, data_block, nodeIndices, f);
	}
	else {
		char *object_name = "SHYBPFORW2";
		status = setUDABlockSignalFromArcade(object_name, shotNumber, extractionIndex, data_block, nodeIndices, f);
	}
	if (status != 0) {
		lh_antennas_throwsIdamError(status, "lh_antennas_power_forward", "SHYBPFORW1/SHYBPFORW2", shotNumber);
	}
	return status;
}

int lh_antennas_power_reflected(int shotNumber, DATA_BLOCK* data_block, int* nodeIndices) {

	float f = 1e6; //conversion factor, power is given in MW in Arcade
	int antennaId = nodeIndices[0]; //=1 or 2
	int extractionIndex = 0; //indicates that signal does not belong to a group
	int status = -1;
	if (antennaId == 1) {
		char *object_name = "SHYBPREFL1";
		status = setUDABlockSignalFromArcade(object_name, shotNumber, extractionIndex, data_block, nodeIndices, f);
	}
	else {
		char *object_name = "SHYBPREFL2";
		status = setUDABlockSignalFromArcade(object_name, shotNumber, extractionIndex, data_block, nodeIndices, f);
	}
	if (status != 0) {
		lh_antennas_throwsIdamError(status, "lh_antennas_power_reflected","SHYBPREFL1/SHYBPREFL2", shotNumber);
	}
	return status;
}

int lh_antennas_reflection_coefficient(int shotNumber, DATA_BLOCK* data_block, int* nodeIndices) {

	float f = 1; //conversion factor, power is given in MW in Arcade
	int antennaId = nodeIndices[0]; //=1 or 2
	int extractionIndex = 0; //indicates that signal does not belong to a group
	int status = -1;
	if (antennaId == 1) {
		char *object_name = "SHYBREF1";
		status = setUDABlockSignalFromArcade(object_name, shotNumber, extractionIndex, data_block, nodeIndices, f);
	}
	else {
		char *object_name = "SHYBREF2";
		status = setUDABlockSignalFromArcade(object_name, shotNumber, extractionIndex, data_block, nodeIndices, f);
	}
	if (status != 0) {
		lh_antennas_throwsIdamError(status, "lh_antennas_reflection_coefficient","SHYBREF1/SHYBREF2", shotNumber);
	}
	return status;
}

int lh_antennas_position_definition(int shotNumber, DATA_BLOCK* data_block, int* nodeIndices) {
	const char* comment = "Closest to machine geometric axis point of the lateral antenna protection at Z = 0";
	setReturnDataString(data_block, comment, NULL);
	return 0;
}

int lh_antennas_pressure_tank(int shotNumber, DATA_BLOCK* data_block, int* nodeIndices) {
	float f = 1e-3; //conversion factor, pressure is given in mPa in Arcade
	int antennaId = nodeIndices[0]; //=1 or 2
	char *object_name = "GPRESHYB";
	int status = setUDABlockSignalFromArcade(object_name, shotNumber, antennaId, data_block, nodeIndices, f);
	if (status != 0) {
		lh_antennas_throwsIdamError(status, "lh_antennas_pressure_tank", object_name, shotNumber);
	}
	return status;
}

int lh_antennas_phase_average(int shotNumber, DATA_BLOCK* data_block, int* nodeIndices) {
	float f = 0.01745329251994329576923690768489; //conversion factor from degrees to radians
	int antennaId = nodeIndices[0]; //=1 or 2
	char *object_name = "GPHASHYB";
	int status = setUDABlockSignalFromArcade(object_name, shotNumber, antennaId, data_block, nodeIndices, f);
	if (status != 0) {
		lh_antennas_throwsIdamError(status, "lh_antennas_phase_average", object_name, shotNumber);
	}
	return status;
}

int lh_antennas_n_parallel_peak(int shotNumber, DATA_BLOCK* data_block, int* nodeIndices) {
	float f = 1;
	int antennaId = nodeIndices[0]; //=1 or 2
	char *object_name = "GNPARHYB";
	int status = setUDABlockSignalFromArcade(object_name, shotNumber, antennaId, data_block, nodeIndices, f);
	if (status != 0) {
		lh_antennas_throwsIdamError(status, "lh_antennas_n_parallel_peak", object_name, shotNumber);
	}
	return status;
}

int lh_antennas_position_r(int shotNumber, DATA_BLOCK* data_block, int* nodeIndices) {
	float f = 1;
	int antennaId = nodeIndices[0]; //=1 or 2
	char *object_name = "GPOSHYB";
	int status = setUDABlockSignalFromArcade(object_name, shotNumber, antennaId, data_block, nodeIndices, f);
	if (status != 0) {
		lh_antennas_throwsIdamError(status, "lh_antennas_position_r", object_name, shotNumber);
	}
	return status;
}

int lh_antennas_position_z(int shotNumber, DATA_BLOCK* data_block, int* nodeIndices) {
	int status = -1;
	lh_antennas_throwsIdamError(status, "lh_antennas_position_z", "NOT IMPLEMENTED !!!", shotNumber);
	return status; //NOT IMPLEMENTED
}

int lh_antennas_modules_power(int shotNumber, DATA_BLOCK* data_block, int* nodeIndices) {
	float f = 1e6; //conversion factor, power is given in MW in Arcade
	int antennaId = nodeIndices[0]; //=1 or 2
	int status = -1;
	if (antennaId == 1) {
		int extractionIndex = nodeIndices[1]; //module id
		char *object_name = "GPINJC1";
		status = setUDABlockSignalFromArcade(object_name, shotNumber, extractionIndex, data_block, nodeIndices, f);
	}
	else {
		int extractionIndex = nodeIndices[1]; //module id
		char *object_name = "GPINJC2";
		status = setUDABlockSignalFromArcade(object_name, shotNumber, extractionIndex, data_block, nodeIndices, f);
	}
	if (status != 0) {
		lh_antennas_throwsIdamError(status, "lh_antennas_modules_power", "GPINJC1/GPINJC2", shotNumber);
	}
	return status;
}

int lh_antennas_modules_power_forward(int shotNumber, DATA_BLOCK* data_block, int* nodeIndices) {
	float f = 1e6; //conversion factor, power is given in MW in Arcade
	int antennaId = nodeIndices[0]; //=1 or 2
	int status = -1;
	if (antennaId == 1) {
		int extractionIndex = nodeIndices[1]; //module id
		char *object_name = "GPFORWC1";
		status = setUDABlockSignalFromArcade(object_name, shotNumber, extractionIndex, data_block, nodeIndices, f);
	}
	else {
		int extractionIndex = nodeIndices[1]; //module id
		char *object_name = "GPFORWC2";
		status = setUDABlockSignalFromArcade(object_name, shotNumber, extractionIndex, data_block, nodeIndices, f);
	}
	if (status != 0) {
		lh_antennas_throwsIdamError(status, "lh_antennas_modules_power_forward", "GPFORWC1/GPFORWC2", shotNumber);
	}
	return status;
}

int lh_antennas_modules_power_reflected(int shotNumber, DATA_BLOCK* data_block, int* nodeIndices) {
	float f = 1e6; //conversion factor, power is given in MW in Arcade
	int antennaId = nodeIndices[0]; //=1 or 2
	int status = -1;
	if (antennaId == 1) {
		int extractionIndex = nodeIndices[1]; //module id
		char *object_name = "GPREFLC1";
		status = setUDABlockSignalFromArcade(object_name, shotNumber, extractionIndex, data_block, nodeIndices, f);
	}
	else {
		int extractionIndex = nodeIndices[1]; //module id
		char *object_name = "GPREFLC2";
		status = setUDABlockSignalFromArcade(object_name, shotNumber, extractionIndex, data_block, nodeIndices, f);
	}
	if (status != 0) {
		lh_antennas_throwsIdamError(status, "lh_antennas_modules_power_reflected", "GPREFLC1/GPREFLC2", shotNumber);
	}
	return status;
}

int lh_antennas_modules_reflection_coefficient(int shotNumber, DATA_BLOCK* data_block, int* nodeIndices) {
	float f = 1; //conversion factor, power is given in MW in Arcade
	int antennaId = nodeIndices[0]; //=1 or 2
	int status = -1;
	if (antennaId == 1) {
		int extractionIndex = nodeIndices[1]; //module id
		char *object_name = "GCREFC1";
		status = setUDABlockSignalFromArcade(object_name, shotNumber, extractionIndex, data_block, nodeIndices, f);
	}
	else {
		int extractionIndex = nodeIndices[1]; //module id
		char *object_name = "GCREFC2";
		status = setUDABlockSignalFromArcade(object_name, shotNumber, extractionIndex, data_block, nodeIndices, f);
	}
	if (status != 0) {
		lh_antennas_throwsIdamError(status, "lh_antennas_modules_reflection_coefficient", "GCREFC1/GCREFC2", shotNumber);
	}
	return status;
}

int lh_antennas_modules_phase(int shotNumber, DATA_BLOCK* data_block, int* nodeIndices) {
	float f = 0.01745329251994329576923690768489; //conversion factor from degrees to radians
	int antennaId = nodeIndices[0]; //=1 or 2
	int status = -1;
	if (antennaId == 1) {
		int extractionIndex = nodeIndices[1]; //module id
		char *object_name = "GPHIC1";
		status = setUDABlockSignalFromArcade(object_name, shotNumber, extractionIndex, data_block, nodeIndices, f);
	}
	else {
		int extractionIndex = nodeIndices[1]; //module id
		char *object_name = "GPHIC2";
		status = setUDABlockSignalFromArcade(object_name, shotNumber, extractionIndex, data_block, nodeIndices, f);
	}
	if (status != 0) {
		lh_antennas_throwsIdamError(status, "lh_antennas_modules_phase", "GPHIC1/GPHIC2", shotNumber);
	}
	return status;
}
