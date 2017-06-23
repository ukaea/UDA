#include "west_dynamic_data.h"

#include <string.h>
#include <stdio.h>
#include <assert.h>

#include <logging/logging.h>
#include <clientserver/initStructs.h>
#include <clientserver/errorLog.h>
#include <clientserver/stringUtils.h>
#include <clientserver/udaTypes.h>

#include "ts_rqparam.h"
#include "west_ece.h"
#include "west_utilities.h"


int GetDynamicData(int shotNumber, const char* mapfun, DATA_BLOCK* data_block, int* nodeIndices)
{

	IDAM_LOG(LOG_DEBUG, "Entering GetDynamicData() -- WEST plugin\n");
	//IDAM data block initialization
	initDataBlock(data_block);
	data_block->rank = 1; //we return always a 1D array
	data_block->dims = (DIMS*)malloc(data_block->rank * sizeof(DIMS));
	int i;
	for (i = 0; i < data_block->rank; i++) {
		initDimBlock(&data_block->dims[i]);
	}

	assert(mapfun); //Mandatory function to get WEST data

	char* fun_name = NULL; //Shape_of, tsmat_collect, tsbase
	char* TOP_collections_parameters = NULL; //example : TOP_collections_parameters = DMAG:GMAG_BNORM:PosR, DMAG:GMAG_BTANG:PosR, ...
	char* attributes = NULL; //example: attributes = 1:float:#1 (rank = 1, type = float, #1 = second IDAM index)
	char* normalizationAttributes = NULL; //example : multiply:cste:3     (multiply value by constant factor equals to 3)

	char* unvalid_channels = NULL; //used for interfero_polarimeter IDS, example : invalid_channels:1,2
	int unvalid_channels_size;
	int* unvalid_channels_list = NULL;

	getFunName(mapfun, &fun_name);

	IDAM_LOG(LOG_DEBUG, "Evaluating the request type (tsbase_collect, tsbase_time, ...)\n");
	int fun;

	if (strcmp(fun_name, "tsbase_collect") == 0) {
		IDAM_LOG(LOG_DEBUG, "tsbase_collect request \n");
		fun = 0;
		tokenizeFunParameters(mapfun, &TOP_collections_parameters, &attributes, &normalizationAttributes);
	} else if (strcmp(fun_name, "tsbase_time") == 0) {
		IDAM_LOG(LOG_DEBUG, "tsbase_time request \n");
		fun = 1;
		tokenizeFunParameters(mapfun, &TOP_collections_parameters, &attributes, &normalizationAttributes);
	} else if (strcmp(fun_name, "tsbase_collect_with_channels") == 0) {
		IDAM_LOG(LOG_DEBUG, "tsbase_collect_with_channels request \n");
		fun = 2;
		tokenizeFunParametersWithChannels(mapfun, &unvalid_channels, &TOP_collections_parameters, &attributes, &normalizationAttributes);
		getUnvalidChannelsSize(unvalid_channels, &unvalid_channels_size);
		unvalid_channels_list = (int *)malloc(sizeof(int)*unvalid_channels_size);
		getUnvalidChannels(unvalid_channels, unvalid_channels_list);
	} else if (strcmp(fun_name, "tsbase_time_with_channels") == 0) {
		IDAM_LOG(LOG_DEBUG, "tsbase_time_with_channels request \n");
		fun = 3;
		tokenizeFunParametersWithChannels(mapfun, &unvalid_channels, &TOP_collections_parameters, &attributes, &normalizationAttributes);
		getUnvalidChannelsSize(unvalid_channels, &unvalid_channels_size);
		unvalid_channels_list = (int *)malloc(sizeof(int)*unvalid_channels_size);
		getUnvalidChannels(unvalid_channels, unvalid_channels_list);
	} else if (strcmp(fun_name, "ece_t_e_data") == 0) {
		IDAM_LOG(LOG_DEBUG, "ece_t_e_data request \n");
		fun = 4;
		char * ece_mapfun = NULL;
		ece_t_e_data(shotNumber, &ece_mapfun);
		tokenizeFunParameters(ece_mapfun, &TOP_collections_parameters, &attributes, &normalizationAttributes);
		IDAM_LOGF(LOG_DEBUG, "TOP_collections_parameters : %s\n", TOP_collections_parameters);
	} else if (strcmp(fun_name, "ece_t_e_time") == 0) {
		IDAM_LOG(LOG_DEBUG, "ece_t_e_time request \n");
		fun = 5;
		char * ece_mapfun = NULL;
		ece_t_e_time(shotNumber, &ece_mapfun);
		tokenizeFunParameters(ece_mapfun, &TOP_collections_parameters, &attributes, &normalizationAttributes);
		IDAM_LOGF(LOG_DEBUG, "TOP_collections_parameters : %s\n", TOP_collections_parameters);
	}

	IDAM_LOG(LOG_DEBUG, "now searching for signals\n");
	int collectionsCount;
	getTopCollectionsCount(TOP_collections_parameters, &collectionsCount);

	int* extractionsCount;
	extractionsCount = (int*)calloc(collectionsCount, sizeof(int));

	int signalType = 0;

	for (i = 0; i < collectionsCount; i++) {
		char* command = NULL;
		getCommand(i, &command, TOP_collections_parameters);

		char* objectName = NULL;
		getObjectName(&objectName, command);

		int nb_extractions = 0;
		int occ = 0;

		IDAM_LOG(LOG_DEBUG, "Group of signals1 ?\n");
		getSignalType(objectName, shotNumber, &signalType);

		if (signalType == 2) {
			getExtractionsCount(objectName, shotNumber, occ, &nb_extractions);
		}

		printNum("Number of extractions : ", nb_extractions);

		extractionsCount[i] = nb_extractions;
	}

	IDAM_LOG(LOG_DEBUG, "searching for IDAM index\n");
	IDAM_LOGF(LOG_DEBUG, "attributes : %s\n", attributes);

	int requestedIndex = getNumIDAMIndex(attributes, nodeIndices);

	printNum("Requested index (from IDAM call) : ", requestedIndex);

	IDAM_LOG(LOG_DEBUG, "searching for the array according to the static index\n");
	int searchedArray;
	int searchedArrayIndex;
	searchIndices(requestedIndex, extractionsCount, &searchedArray, &searchedArrayIndex);

	char* objectName = NULL;
	char* command = NULL;

	printNum("searchedArrayIndex : ", searchedArrayIndex);

	printNum("searchedArray : ", searchedArray);

	//This patch means that if the signal does not belong to a group, so it can not be aggregate as signals groups
	//Example of an aggregate : tsbase_collect;DMAG:first_group, DMAG:second_group;float:#0

	if (signalType != 2) {
		searchedArray = 0;
	}

	IDAM_LOG(LOG_DEBUG, "getting the command\n");
	IDAM_LOGF(LOG_DEBUG, "TOP_collections_parameters: %s\n", TOP_collections_parameters);

	int status = getCommand(searchedArray, &command, TOP_collections_parameters);

	if (status != 0) {
		int err = 901;
		IDAM_LOG(LOG_DEBUG, "Unable to get command\n");
		addIdamError(&idamerrorstack, CODEERRORTYPE, "Unable to get command", err, "");
	}

	IDAM_LOGF(LOG_DEBUG, "command: %s\n", command);
	IDAM_LOG(LOG_DEBUG, "Getting object name\n");
	getObjectName(&objectName, command);

	IDAM_LOG(LOG_DEBUG, "Group of signals ?\n");
	getSignalType(objectName, shotNumber, &signalType);

	if (signalType == 2) { //signal is a group of signals, so we append extraction chars to signal name
		IDAM_LOG(LOG_DEBUG, "Signal belongs to a group of signals\n");
		char result[50];
		addExtractionChars(result, objectName,
				searchedArrayIndex + 1); //Concatenate signalName avec %searchedArrayIndex + 1
		objectName = strdup(result);
	} else {
		IDAM_LOG(LOG_DEBUG, "Signal does not belong to a group of signals\n");
	}

	IDAM_LOGF(LOG_DEBUG, "Object name: %s\n", objectName);

	float* time = NULL;
	float* data = NULL;
	int len;

	int rang[2] = { 0, 0 };
	status = readSignal(objectName, shotNumber, 0, rang, &time, &data, &len);

	//float f1 = data[0];
//	IDAM_LOGF(LOG_DEBUG, "%s\n", "First time values...");
//	int j;
//	for (j=0; j <10; j++) {
//		IDAM_LOGF(LOG_DEBUG, "time : %f\n", time[j]);
//	}

	//IDAM_LOGF(LOG_DEBUG, "First value 1: %f\n", data(1));

	IDAM_LOG(LOG_DEBUG, "End of reading signal\n");

	/*if (status != 0) {
            int err = 901;
            addIdamError(&idamerrorstack, CODEERRORTYPE, "Unable to read dynamic data from WEST !", err, "");
        }*/

	//IDAM_LOG(LOG_DEBUG, "After error handling\n");

	IDAM_LOG(LOG_DEBUG, "Getting normalization factor, if any\n");
	float normalizationFactor = 1;
	getNormalizationFactor(&normalizationFactor, normalizationAttributes);
	IDAM_LOG(LOG_DEBUG, "Starting data normalization\n");
	multiplyFloat(data, normalizationFactor, len);
	IDAM_LOG(LOG_DEBUG, "end of data normalization, if any\n");

	free(data_block->dims);

	data_block->rank = 1;
	data_block->data_type = TYPE_FLOAT;
	data_block->data_n = len;

	if ( fun == 0 || fun == 4) {
		IDAM_LOG(LOG_DEBUG, "setting data_block->data\n");
		data_block->data = (char*)data;
	} else if (fun == 1 || fun == 5) {
		IDAM_LOG(LOG_DEBUG, "setting data_block->data\n");
		data_block->data = (char*)time;
	} else if (fun == 2) {
		//TODO if (isChannelValid(requestedIndex + 1, unvalid_channels_list, unvalid_channels_size))
		//{
			//IDAM_LOGF(LOG_DEBUG, "Setting data for valid channel: %d\n", requestedIndex + 1);
		     IDAM_LOG(LOG_DEBUG, "setting data_block->data\n");
			data_block->data = (char*)data;
		//}
	} else if (fun == 3) {
		//TODO if (isChannelValid(requestedIndex + 1, unvalid_channels_list, unvalid_channels_size))
		    IDAM_LOG(LOG_DEBUG, "setting data_block->data\n");
			data_block->data = (char*)time;
	}

	data_block->dims =
			(DIMS*)malloc(data_block->rank * sizeof(DIMS));

	for (i = 0; i < data_block->rank; i++) {
		initDimBlock(&data_block->dims[i]);
	}
	data_block->dims[0].data_type = TYPE_FLOAT;
	data_block->dims[0].dim_n = len;
	data_block->dims[0].compressed = 0;
	data_block->dims[0].dim = (char*)time;

	strcpy(data_block->data_label, "");
	strcpy(data_block->data_units, "");
	strcpy(data_block->data_desc, "");

	free(command);
	free(TOP_collections_parameters);
	free(objectName);
	free(fun_name);
	free(attributes);
	free(normalizationAttributes);
	free(unvalid_channels);
	free(unvalid_channels_list);
	IDAM_LOG(LOG_DEBUG, "TEST_FINAL\n");
	return 0;
}

