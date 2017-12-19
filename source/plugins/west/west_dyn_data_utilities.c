#include "west_dyn_data_utilities.h"

#include <string.h>
#include <stdio.h>
#include <assert.h>

#include <logging/logging.h>
#include <clientserver/initStructs.h>
#include <clientserver/errorLog.h>
#include <clientserver/stringUtils.h>
#include <clientserver/udaTypes.h>

#include "ts_rqparam.h"
#include "west_utilities.h"

void SetDynData(DATA_BLOCK* data_block, int len, float* time, float* data, int setTime);

int SetNormalizedDynData(int shotNumber, DATA_BLOCK* data_block, int* nodeIndices,
		char* TOP_collections_parameters, char* attributes, char* normalizationAttributes,
		int setTime);


int SetNormalizedDynamicData(int shotNumber, DATA_BLOCK* data_block, int* nodeIndices,
		char* TOP_collections_parameters, char* attributes, char* normalizationAttributes)
{
	return SetNormalizedDynData(shotNumber, data_block, nodeIndices,
			TOP_collections_parameters, attributes, normalizationAttributes, 0);
}

int SetNormalizedDynamicDataTime(int shotNumber, DATA_BLOCK* data_block, int* nodeIndices,
		char* TOP_collections_parameters, char* attributes, char* normalizationAttributes)
{
	return SetNormalizedDynData(shotNumber, data_block, nodeIndices,
			TOP_collections_parameters, attributes, normalizationAttributes, 1);
}

int SetNormalizedDynData(int shotNumber, DATA_BLOCK* data_block, int* nodeIndices,
		char* TOP_collections_parameters, char* attributes, char* normalizationAttributes, int setTime)
{
	int len;
	float* time;
	float* data;

	int status = GetDynData(shotNumber, &time, &data, &len, nodeIndices,
			TOP_collections_parameters, attributes);

	IDAM_LOG(UDA_LOG_DEBUG, "after calling GetDynData\n");

	if (status != 0) {
		int err = 901;
		IDAM_LOG(UDA_LOG_DEBUG, "after calling GetDynData1\n");
		addIdamError(CODEERRORTYPE, "WEST:ERROR: unable to get dynamic data", err, "");
		/*if (time !=NULL)
			free(time);
		if (data != NULL)
			free(data);*/
		IDAM_LOG(UDA_LOG_DEBUG, "after calling GetDynData2\n");
	} else {
		IDAM_LOG(UDA_LOG_DEBUG, "Getting normalization factor, if any\n");
		float normalizationFactor = 1;
		getNormalizationFactor(&normalizationFactor, normalizationAttributes);
		IDAM_LOG(UDA_LOG_DEBUG, "Starting data normalization\n");
		multiplyFloat(data, normalizationFactor, len);
		IDAM_LOG(UDA_LOG_DEBUG, "End of data normalization, if any\n");

		SetDynData(data_block, len, time, data, setTime);
	}
	IDAM_LOG(UDA_LOG_DEBUG, "End of function SetNormalizedDynData()\n");
	return status;
}

int GetNormalizedDynamicData(int shotNumber, float** data_time, float** data, int* len, int* nodeIndices,
		char* TOP_collections_parameters, char* attributes, char* normalizationAttributes)
{
	int status = GetDynData(shotNumber, data_time, data, len, nodeIndices,
			TOP_collections_parameters, attributes);

	if (status != 0) {
		int err = 901;
		addIdamError(CODEERRORTYPE, "WEST:ERROR: unable to get dynamic data", err, "");
	} else {

		IDAM_LOG(UDA_LOG_DEBUG, "Getting normalization factor, if any\n");
		float normalizationFactor = 1;
		getNormalizationFactor(&normalizationFactor, normalizationAttributes);
		IDAM_LOG(UDA_LOG_DEBUG, "Starting data normalization\n");
		multiplyFloat(*data, normalizationFactor, *len);
		IDAM_LOG(UDA_LOG_DEBUG, "end of data normalization, if any\n");
	}

	return status;
}

int GetDynData(int shotNumber, float** data_time, float** data, int* len, int* nodeIndices,
		char* TOP_collections_parameters, char* attributes)
{
	IDAM_LOG(UDA_LOG_DEBUG, "Entering in GetDynData()...\n");
	int collectionsCount;
	getTopCollectionsCount(TOP_collections_parameters, &collectionsCount);

	int* extractionsCount;
	extractionsCount = (int*)calloc(collectionsCount, sizeof(int));

	int signalType = 0;
	int i;
	char* command = NULL;
	int totalExtractions = 0;

	for (i = 0; i < collectionsCount; i++) {

		IDAM_LOGF(UDA_LOG_DEBUG, "In GetDynData, i: %d\n", i);

		getCommand(i, &command, TOP_collections_parameters);

		IDAM_LOGF(UDA_LOG_DEBUG, "Command: %s\n", command);

		char* objectName = NULL;
		getObjectName(&objectName, command);

		int nb_extractions = 0;
		int occ = 0;

		IDAM_LOG(UDA_LOG_DEBUG, "Group of signals ?\n");
		getSignalType(objectName, shotNumber, &signalType);

		if (signalType == 2) {
			getExtractionsCount(objectName, shotNumber, occ, &nb_extractions);
		}

		IDAM_LOGF(UDA_LOG_DEBUG, "Number of extractions: %d", nb_extractions);

		extractionsCount[i] = nb_extractions;
		totalExtractions += extractionsCount[i];
		command = NULL;
	}

	IDAM_LOG(UDA_LOG_DEBUG, "searching for IDAM index...\n");
	IDAM_LOGF(UDA_LOG_DEBUG, "attributes : %s\n", attributes);

	int requestedIndex = getNumIDAMIndex(attributes, nodeIndices);

	IDAM_LOGF(UDA_LOG_DEBUG, "Requested index (from UDA call): %d\n", requestedIndex);

	int searchedArray;
	int searchedArrayIndex;
	IDAM_LOG(UDA_LOG_DEBUG, "searching for the array according to the UDA index\n");
	searchIndices(requestedIndex, extractionsCount, &searchedArray, &searchedArrayIndex);

	char* objectName = NULL;

	IDAM_LOGF(UDA_LOG_DEBUG, "searchedArrayIndex: %d\n", searchedArrayIndex);
	IDAM_LOGF(UDA_LOG_DEBUG, "searchedArray: %d\n", searchedArray);

	//This patch means that if the signal does not belong to a group, so it can not be aggregate as signals groups
	//Example of an aggregate : tsbase_collect;DMAG:first_group, DMAG:second_group;float:#0

	if (signalType != 2) {
		searchedArray = 0;
	}


	IDAM_LOGF(UDA_LOG_DEBUG, "TOP_collections_parameters: %s\n", TOP_collections_parameters);

	int status = getCommand(searchedArray, &command, TOP_collections_parameters);

	if (status != 0) {
		int err = 901;
		IDAM_LOGF(UDA_LOG_DEBUG, "WEST:ERROR: unable to get command for TOP: %s\n", TOP_collections_parameters);
		addIdamError(CODEERRORTYPE, "WEST:ERROR: unable to get command", err, "");
		return status;
	}

	IDAM_LOGF(UDA_LOG_DEBUG, "Command: %s\n", command);
	getObjectName(&objectName, command);
	IDAM_LOGF(UDA_LOG_DEBUG, "Getting object name: %s\n", objectName);

	IDAM_LOG(UDA_LOG_DEBUG, "Group of signals ?\n");
	getSignalType(objectName, shotNumber, &signalType);

	if (signalType == 2) { //signal is a group of signals, so we append extraction chars to signal name
		IDAM_LOG(UDA_LOG_DEBUG, "Signal belongs to a group of signals\n");
		char result[50];
		addExtractionChars(result, objectName,
				searchedArrayIndex +
				1); //Concatenate signalName avec %(searchedArrayIndex + 1), example: %1, %2, ...
		objectName = strdup(result);
	} else {
		IDAM_LOG(UDA_LOG_DEBUG, "Signal does not belong to a group of signals\n");
	}

	int rang[2] = { 0, 0 };
	status = readSignal(objectName, shotNumber, 0, rang, data_time, data, len);

	if (status != 0) {
		int err = 901;
		IDAM_LOGF(UDA_LOG_DEBUG, "WEST:ERROR: error reading signal\n", objectName);
		addIdamError(CODEERRORTYPE, "WEST:ERROR: error reading signal", err, "");
		if (command != NULL)
			free(command);
		if (objectName != NULL)
			free(objectName);
		if (extractionsCount != NULL)
			free(extractionsCount);
		IDAM_LOG(UDA_LOG_DEBUG, "Returning from GetDynData\n");
		return status;
	}

	IDAM_LOG(UDA_LOG_DEBUG, "End of reading signal\n");

	//	IDAM_LOGF(UDA_LOG_DEBUG, "%s\n", "First time values...");
	//	int j;
	//	for (j=0; j <10; j++) {
	//		IDAM_LOGF(UDA_LOG_DEBUG, "time : %f\n", *time[j]);
	//	}

	if (len == 0) {
		int err = 901;
		addIdamError(CODEERRORTYPE, "WEST:ERROR: dynamic data empty !", err, "");
		status = -1;
	} else {
		status = 0;
	}

	free(command);
	free(objectName);
	free(extractionsCount);

	return status;
}

void SetDynamicData(DATA_BLOCK* data_block, int len, float* data_time, float* data)
{
	SetDynData(data_block, len, data_time, data, 0);
}

void SetDynamicDataTime(DATA_BLOCK* data_block, int len, float* data_time, float* data)
{
	SetDynData(data_block, len, data_time, data, 1);
}

void SetDynData(DATA_BLOCK* data_block, int len, float* data_time, float* data, int setTime)
{

	//IDAM data block initialization
	initDataBlock(data_block);
	int i;
	data_block->rank = 1;
	data_block->data_type = UDA_TYPE_FLOAT;
	data_block->data_n = len;

	if (setTime == 0) {
		data_block->data = (char*)data;
	} else {
		data_block->data = (char*)data_time;
	}

	data_block->dims = (DIMS*)malloc(data_block->rank * sizeof(DIMS));

	for (i = 0; i < data_block->rank; i++) {
		initDimBlock(&data_block->dims[i]);
	}

	data_block->dims[0].data_type = UDA_TYPE_FLOAT;
	data_block->dims[0].dim_n = len;
	data_block->dims[0].compressed = 0;
	data_block->dims[0].dim = (char*)data_time;

	strcpy(data_block->data_label, "");
	strcpy(data_block->data_units, "");
	strcpy(data_block->data_desc, "");
}

int getArcadeSignal(char* nomsigp, int shotNumber, int extractionIndex, float** data_time, float** data, int* len, float normalizationFactor)
{
	IDAM_LOG(UDA_LOG_DEBUG, "Group of signals ?\n");
	char nomsigp_to_extract[50];

	if (extractionIndex > 0) {
		addExtractionChars(nomsigp_to_extract, nomsigp,
				extractionIndex); //Concatenate nomsigp_to_extract avec !extractionIndex, example: !1, !2, ...
	}
	else {
		strcpy(nomsigp_to_extract, nomsigp);
		RemoveSpaces(nomsigp_to_extract);
	}
	IDAM_LOGF(UDA_LOG_DEBUG, "signal: %s\n", nomsigp_to_extract);
	int rang[2] = { 0, 0 };
	int status = readSignal(nomsigp_to_extract, shotNumber, 0, rang, data_time, data, len);
	multiplyFloat(*data, normalizationFactor, *len);
	return status;
}

int setUDABlockSignalFromArcade(char* sigName, int shotNumber, int extractionIndex, DATA_BLOCK* data_block, int* nodeIndices, float normalizationFactor)
{
	float *data_time = NULL;
	float *data = NULL;
	int len;

	int status = getArcadeSignal(sigName, shotNumber, extractionIndex, &data_time, &data, &len, normalizationFactor);

	if (status != 0) {
		int err = 901;
		addIdamError(CODEERRORTYPE, "WEST:ERROR: unable to get arcade signal", err, "");
	}
	else {
		SetDynamicData(data_block, len, data_time, data);
	}
	return 0;
}

int setUDABlockSignalFromArcade2(int shotNumber, char* sigName, int extractionIndex, char* sigName2,
		int extractionIndex2, DATA_BLOCK* data_block, int* nodeIndices, float treshold)
{
	float *time1 = NULL;
	float *data1 = NULL;
	int len1;

	int status = getArcadeSignal(sigName, shotNumber, extractionIndex, &time1, &data1, &len1, 1.);

	if (status != 0) {
		int err = 901;
		addIdamError(CODEERRORTYPE, "WEST:ERROR: unable to get arcade signal1", err, "");
	}

	float *time2 = NULL;
	float *data2 = NULL;
	int len2;

	status = getArcadeSignal(sigName, shotNumber, extractionIndex, &time2, &data2, &len2, 1.);

	if (status != 0) {
		int err = 901;
		addIdamError(CODEERRORTYPE, "WEST:ERROR: unable to get arcade signal2", err, "");
	}

	float *ip_time = NULL;
	float *ip_data = NULL;
	int ip_len;

	IDAM_LOG(UDA_LOG_DEBUG, "test in setUDABlockSignalFromArcade2\n");

	status = getArcadeSignal("SMAG_IP", shotNumber, 1, &ip_time, &ip_data, &ip_len, 1.);
	if (status != 0) {
		int err = 901;
		addIdamError(CODEERRORTYPE, "WEST:ERROR: unable to get SMAG_IP signal", err, "");
	}

	float *data = NULL;
	merge2Signals_according_to_ip_treshold(&data, len1, data1, data2, ip_data, treshold);
	SetDynamicData(data_block, len1, time1, data);
	return 0;
}

void merge2Signals_according_to_ip_treshold(float **data, int len, float *data1, float *data2, float *ip, float treshold) {

	int i;
	*data = (float*) malloc(sizeof(float)*len);
	for (i = 0; i < len; i++) {
		if (ip[i] >= treshold) {
			*(*data + i) = data2[i];
		}
		else {
			*(*data + i) = data1[i];
		}
	}
}

void averageArcadeSignal(char* sigName, int shotNumber, int extractions[], int extractions_length, float** data_time, float** averaged_data, int* len) {

	int i;

	for (i=0; i<extractions_length; i++) {

		int status = -1;
		if (i == 0) {
			status = getArcadeSignal(sigName, shotNumber, extractions[i], data_time, averaged_data, len, 1.);
		}
		else {
			float *data = NULL;
			status = getArcadeSignal(sigName, shotNumber, extractions[i], data_time, &data, len, 1.);
			sum(*averaged_data, data, *len);
		}

		if (status != 0) {
			int err = 901;
			addIdamError(CODEERRORTYPE, "WEST:ERROR: unable to get arcade signal in averageArcadeSignal method", err, "");
		}
	}
	normalize(*averaged_data, *len, extractions_length);
}


void sum(float* sum_data, float* data, int len) {
	int i;
	for (i = 0; i < len; i++) {
		sum_data[i] = data[i] + sum_data[i];
	}
}

void normalize(float* sum_data, int len, int normalizationFactor) {
	int i;
	for (i = 0; i < len; i++) {
		sum_data[i] = sum_data[i]/normalizationFactor;
	}
}

void multiply(float* data, int len, float factor) {
	int i;
	for (i = 0; i < len; i++) {
		data[i] = data[i]*factor;
	}
}

int signalsRatio(float **result_q_by_r, float *q, float *r, int lenq, int lenr) {
	IDAM_LOGF(UDA_LOG_DEBUG, "length of q:%d\n", lenq);
	IDAM_LOGF(UDA_LOG_DEBUG, "length of r:%d\n", lenr);
	if (lenq != lenr)
		return -1;
	*result_q_by_r = (float*) malloc(sizeof(float)*lenq);
	int i;
	for (i = 0; i < lenq; i++) {
		//IDAM_LOGF(UDA_LOG_DEBUG, "r[i]=%f\n", r[i]);
		if (r[i] < 1e-30)
			*(*result_q_by_r + i) = 0.;
		else
			*(*result_q_by_r + i) = q[i]/r[i];
	}
	return 0;
}

int signalsSquare(float **square_s, float *s, int len) {
	*square_s = (float*) malloc(sizeof(float)*len);
	int i;
	for (i = 0; i < len; i++)
		*(*square_s + i) = s[i]*s[i];
	return 0;
}

int multiplySignals(float **result, float *p, float *q, int len) {
	*result = (float*) malloc(sizeof(float)*len);
	int i;
	for (i = 0; i < len; i++)
		*(*result + i) = p[i]*q[i];
	return 0;
}

