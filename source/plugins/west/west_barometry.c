#include "west_barometry.h"

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

void barometry_gauge_name(int shotNumber, DATA_BLOCK* data_block, int* nodeIndices) {
	int gaugeId = nodeIndices[0]; //starts from 1
	if (gaugeId == 1) {
		const char* name = "Baratron divertor Haut Q2haut";
		setReturnDataString(data_block, name, NULL);
	} else if (gaugeId == 2) {
		const char* name = "Baratron divertor Q3 Baffle";
		setReturnDataString(data_block, name, NULL);
	} else if (gaugeId == 3) {
		const char* name = "Baratron divertor Q5A Baffle";
		setReturnDataString(data_block, name, NULL);
	}  else if (gaugeId == 4) {
		const char* name = "Jauge Penning Q5B Baffle(CF2P) + optical signal";
		setReturnDataString(data_block, name, NULL);
	}
}

void barometry_gauge_type_name(int shotNumber, DATA_BLOCK* data_block, int* nodeIndices) {
	int gaugeId = nodeIndices[0]; //starts from 1
	if (gaugeId == 1 || gaugeId == 2 || gaugeId == 3) {
		const char* name = "Baratron";
		setReturnDataString(data_block, name, NULL);
	}  else if (gaugeId == 4) {
		const char* name = "Penning";
		setReturnDataString(data_block, name, NULL);
	}
}

void barometry_gauge_type_index(int shotNumber, DATA_BLOCK* data_block, int* nodeIndices) {
	int gaugeId = nodeIndices[0]; //starts from 1
	if (gaugeId == 1 || gaugeId == 2 || gaugeId == 3) {
		int index = 2;
		setReturnDataIntScalar(data_block, index, NULL);
	} else if (gaugeId == 4) {
		int index = 1;
		setReturnDataIntScalar(data_block, index, NULL);
	}
}

void barometry_gauge_type_description(int shotNumber, DATA_BLOCK* data_block, int* nodeIndices) {
	barometry_gauge_name(shotNumber, data_block, nodeIndices);
}

void barometry_gauge_pressure_data(int shotNumber, DATA_BLOCK* data_block, int* nodeIndices) {
	UDA_LOG(UDA_LOG_DEBUG, "Calling barometry_gauge_pressure_data\n");
	float f = 1; //conversion factor
	int gaugeId = nodeIndices[0]; //starts from 1
	if (gaugeId == 1) {
		char *object_name = "GBARDB4";
		int extractionIndex = 2;
		UDA_LOG(UDA_LOG_DEBUG, "Calling setUDABlockSignalFromArcade\n");
		int status = setUDABlockSignalFromArcade(object_name, shotNumber, extractionIndex, data_block, nodeIndices, f);
		UDA_LOG(UDA_LOG_DEBUG, "after Calling setUDABlockSignalFromArcade\n");
		if (status != 0) {
			UDA_LOG(UDA_LOG_DEBUG, "after Calling setUDABlockSignalFromArcade --> error\n");
			int err = 901;
			char* errorMsg = "WEST:ERROR (barometry_gauge_pressure_data) : unable to get object : ";
			strcat(errorMsg, object_name);
			strcat(errorMsg, " for shot : ");
			char shotStr[6];
			sprintf(shotStr, "%d", shotNumber);
			strcat(errorMsg, shotStr);
			UDA_LOG(UDA_LOG_ERROR, "%s", errorMsg);
			addIdamError(CODEERRORTYPE, errorMsg, err, "");
		}
	} else if (gaugeId == 2) {
		char *object_name = "GBARDB8";
		int extractionIndex = 5;
		int status = setUDABlockSignalFromArcade(object_name, shotNumber, extractionIndex, data_block, nodeIndices, f);
		if (status != 0) {
			int err = 901;
			char* errorMsg = "WEST:ERROR (barometry_gauge_pressure_data) : unable to get object : ";
			strcat(errorMsg, object_name);
			strcat(errorMsg, " for shot : ");
			char shotStr[6];
			sprintf(shotStr, "%d", shotNumber);
			strcat(errorMsg, shotStr);
			UDA_LOG(UDA_LOG_ERROR, "%s", errorMsg);
			addIdamError(CODEERRORTYPE, errorMsg, err, "");
		}
	} else if (gaugeId == 3) {
		char *object_name = "GBARDB8";
		int extractionIndex = 12;
		int status = setUDABlockSignalFromArcade(object_name, shotNumber, extractionIndex, data_block, nodeIndices, f);
		if (status != 0) {
			int err = 901;
			char* errorMsg = "WEST:ERROR (barometry_gauge_pressure_data) : unable to get object : ";
			strcat(errorMsg, object_name);
			strcat(errorMsg, " for shot : ");
			char shotStr[6];
			sprintf(shotStr, "%d", shotNumber);
			strcat(errorMsg, shotStr);
			UDA_LOG(UDA_LOG_ERROR, "%s", errorMsg);
			addIdamError(CODEERRORTYPE, errorMsg, err, "");
		}
	} else if (gaugeId == 4) {
		char *object_name = "GBARDP4";
		int extractionIndex = 6;
		int status = setUDABlockSignalFromArcade(object_name, shotNumber, extractionIndex, data_block, nodeIndices, f);
		if (status != 0) {
			int err = 901;
			char* errorMsg = "WEST:ERROR (barometry_gauge_pressure_data) : unable to get object : ";
			strcat(errorMsg, object_name);
			strcat(errorMsg, " for shot : ");
			char shotStr[6];
			sprintf(shotStr, "%d", shotNumber);
			strcat(errorMsg, shotStr);
			UDA_LOG(UDA_LOG_ERROR, "%s", errorMsg);
			addIdamError(CODEERRORTYPE, errorMsg, err, "");
		}
	}

	UDA_LOG(UDA_LOG_DEBUG, "Returning from barometry_gauge_pressure_data\n");
}

void barometry_gauge_pressure_time(int shotNumber, DATA_BLOCK* data_block, int* nodeIndices)
{
	UDA_LOG(UDA_LOG_DEBUG, "calling barometry_gauge_pressure_time...\n");

	int len;
	float *time = NULL;
	float *data = NULL;
	int rang[2] = { 0, 0 };

	char nomsigp_to_extract[50];
	int gaugeId = nodeIndices[0]; //starts from 1
	int status = -1;

	if (gaugeId == 1) {
		char* nomsigp = "GBARDB4";
		int extractionIndex = 2;
		addExtractionChars(nomsigp_to_extract, nomsigp, extractionIndex); //Concatenate nomsigp_to_extract avec !extractionIndex, example: !1, !2, ...
		status = readSignal(nomsigp_to_extract, shotNumber, 0, rang, &time, &data, &len);
	} else if (gaugeId == 2) {
		char* nomsigp = "GBARDB8";
		int extractionIndex = 5;
		addExtractionChars(nomsigp_to_extract, nomsigp, extractionIndex); //Concatenate nomsigp_to_extract avec !extractionIndex, example: !1, !2, ...
		status = readSignal(nomsigp_to_extract, shotNumber, 0, rang, &time, &data, &len);
	} else if (gaugeId == 3) {
		char* nomsigp = "GBARDB8";
		int extractionIndex = 12;
		addExtractionChars(nomsigp_to_extract, nomsigp, extractionIndex); //Concatenate nomsigp_to_extract avec !extractionIndex, example: !1, !2, ...
		status = readSignal(nomsigp_to_extract, shotNumber, 0, rang, &time, &data, &len);
	}  else if (gaugeId == 4) {
		char* nomsigp = "GBARDP4";
		int extractionIndex = 6;
		addExtractionChars(nomsigp_to_extract, nomsigp, extractionIndex); //Concatenate nomsigp_to_extract avec !extractionIndex, example: !1, !2, ...
		status = readSignal(nomsigp_to_extract, shotNumber, 0, rang, &time, &data, &len);
	}

	UDA_LOG(UDA_LOG_DEBUG, "status in barometry_gauge_pressure_time = %d...\n", status);

	if (status != 0) {
		int err = 901;
		char* errorMsg = "WEST:ERROR: error calling barometry_gauge_pressure_time for shot : ";
		char shotStr[6];
		sprintf(shotStr, "%d", shotNumber);
		strcat(errorMsg, shotStr);
		UDA_LOG(UDA_LOG_ERROR, "%s", errorMsg);
		addIdamError(CODEERRORTYPE, errorMsg, err, "");
		if (time != NULL)
			free(time);
		if (data != NULL)
			free(data);
	}
	UDA_LOG(UDA_LOG_DEBUG, "setting time...\n");
	SetDynamicDataTime(data_block, len, time, data);

	UDA_LOG(UDA_LOG_DEBUG, "Returning from barometry_gauge_pressure_time...\n");
}

void barometry_gauge_calibration_coefficient(int shotNumber, DATA_BLOCK* data_block, int* nodeIndices) {
	int gaugeId = nodeIndices[0]; //starts from 1
	if (gaugeId == 1) {
		float calibration_coefficient = 3.9663*1e-5;
		setReturnDataFloatScalar(data_block, calibration_coefficient, NULL);
	} else if (gaugeId == 2) {
		float calibration_coefficient = 3.9663*1e-5;
		setReturnDataFloatScalar(data_block, calibration_coefficient, NULL);
	} else if (gaugeId == 3) {
		float calibration_coefficient = 3.9663*1e-5;
		setReturnDataFloatScalar(data_block, calibration_coefficient, NULL);
	}  else if (gaugeId == 4) {
		float no_calibration_coefficient_defined = -9.0*1e-40;
		setReturnDataFloatScalar(data_block, no_calibration_coefficient_defined, NULL);
	}
}
