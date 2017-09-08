#include "west_xml.h"

#include <string.h>
#include <stdio.h>
#include <assert.h>

#include <logging/logging.h>
#include <clientserver/initStructs.h>
#include <clientserver/errorLog.h>
#include <clientserver/stringUtils.h>
#include <clientserver/udaTypes.h>

#include "ts_rqparam.h"

static void printNum(const char* label, int i);
static void RemoveSpaces(char* source);
static void getValueCollect(char* command, char** value, int* nodeIndices);
static void multiplyFloat(float* p, float factor, int val_nb);
static void multiplyInt(int* p, float factor, int val_nb);

static void getFunName(const char* s, char** fun_name);
static void getNormalizationFactor(float* normalizationFactor, char* normalizationAttributes);
static int  convertToInt(char* value);
static void setStaticValue(int data_type, DATA_BLOCK* data_block, char* value, int requestedIndex, float normalizationFactor);
static void setStatic1DValue(int data_type, DATA_BLOCK* data_block, char* value, int val_nb, float normalizationFactor);
static void setStaticINTValue(int data_type, DATA_BLOCK* data_block, int value, float normalizationFactor);
static void getRank(char* attributes, int* rank);
static void tokenizeFunParameters(const char* s, char** argument_name, char** attributes, char** normalizationAttributes);
static void tokenizeFunParametersWithChannels(const char* s, char** unvalid_channels, char** TOP_collections_parameters,
		char** attributes, char** normalizationAttributes);
static void tokenize_set_channels_validity(const char* s, char** unvalid_channels, char** attributes);
static void tokenizeCommand(const char* s, char** prod_name, char** obj_name, char** param_name);
static int  getCommand(int i, char** command, const char* TOP_collections_parameters);
static void getUnvalidChannelsSize(char* unvalid_channels, int* size);
static void getUnvalidChannels(char* unvalid_channels, int* v);
static int  isChannelValid(int channel_number, int* unvalid_channels_list, int unvalid_channels_list_size);

static int  execute(const char* mapfun, int shotNumber, DATA_BLOCK* data_block, int* nodeIndices);
static void execute_tsmat_collect(const char* TOP_collections_parameters, char* attributes,
		int shotNumber, DATA_BLOCK* data_block, int* nodeIndices,
		char* normalizationAttributes);
static void execute_tsmat_without_idam_index(const char* command, char* attributes,
		int shotNumber, DATA_BLOCK* data_block, char* normalizationAttributes);
static void execute_setvalue_collect(const char* TOP_collections_parameters, char* attributes,
		int shotNumber, DATA_BLOCK* data_block, int* nodeIndices, char* normalizationAttributes);
static void execute_setchannels_validity(int* unvalid_channels_list, int unvalid_channels_size, char* attributes,
		int shotNumber, DATA_BLOCK* data_block, int* nodeIndices);

static void getShapeOf(const char* command, int shotNumber, int* nb_val);

static int  getNumIDAMIndex(char* attributes, int* nodeIndices);
static int  getNumIDAMIndex2(char* s, int* nodeIndices);

static void getReturnType(char* attributes, int* dataType);
static void searchIndices(int requestedIndex, int* l, int* searchedArray, int* searchedArrayIndex);
static void addExtractionChars(char* result, char* signalName, int extractionIndex);
static void getTopCollectionsCount(const char* TOP_collections_parameters, int* collectionsCount);
static void getObjectName(char** obj_name, char* command);

int GetStaticData(int shotNumber, const char* mapfun, DATA_BLOCK* data_block, int* nodeIndices)
{
	IDAM_LOG(UDA_LOG_DEBUG, "Calling GetStaticData() from WEST plugin\n");

	//IDAM data block initialization
	initDataBlock(data_block);
	data_block->rank = 1; //we return always a 1D array
	data_block->dims = (DIMS*)malloc(data_block->rank * sizeof(DIMS));
	int i;
	for (i = 0; i < data_block->rank; i++) {
		initDimBlock(&data_block->dims[i]);
	}

	assert(mapfun); //Mandatory function to get WEST data

	IDAM_LOG(UDA_LOG_DEBUG, "Calling execute() from WEST plugin\n");
	int status = execute(mapfun, shotNumber, data_block, nodeIndices);

	if (status != 0) {
		addIdamError(CODEERRORTYPE, __func__, -900, "error while getting static data");
	}

	free(data_block->dims);
	data_block->dims = NULL;

	// Scalar data
	data_block->rank = 0;
	data_block->data_n = 1;

	strcpy(data_block->data_desc, "");
	strcpy(data_block->data_label, "");
	strcpy(data_block->data_units, "");

	return 0;
}

int execute(const char* mapfun, int shotNumber, DATA_BLOCK* data_block, int* nodeIndices)
{

	char* fun_name = NULL; //Shape_of, tsmat_collect, tsbase
	char* TOP_collections_parameters = NULL; //example : TOP_collections_parameters = DMAG:GMAG_BNORM:PosR, DMAG:GMAG_BTANG:PosR, ...
	char* attributes = NULL; //example : attributes = rank:float:#1 (rank=0,1 , type = float, #1 = second IDAM index)
	char* normalizationAttributes = NULL; //example : multiply:cste:3     (multiply value by constant factor equals to 3)

	getFunName(mapfun, &fun_name);
	IDAM_LOGF(UDA_LOG_DEBUG, "fun_name: %s\n", fun_name);

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
	}  else if (strcmp(fun_name, "set_channels_validity") == 0) {
		//returns a static value according to the position of the element in the collection (rank = 0)
		fun = 5;
	}

	printNum("Case : ", fun);

	switch (fun) {
	case 0: {
		IDAM_LOG(UDA_LOG_DEBUG, "Case of tsmat_collect from WEST plugin\n");
		IDAM_LOG(UDA_LOG_DEBUG, "Calling tokenizeFunParameters() from WEST plugin\n");
		tokenizeFunParameters(mapfun, &TOP_collections_parameters, &attributes, &normalizationAttributes);
		execute_tsmat_collect(TOP_collections_parameters, attributes, shotNumber, data_block, nodeIndices,
				normalizationAttributes);

		break;
	}

	case 1: {

		IDAM_LOG(UDA_LOG_DEBUG, "Case of shape_of_tsmat_collect from WEST plugin\n");

		IDAM_LOG(UDA_LOG_DEBUG, "Calling tokenizeFunParameters() from WEST plugin\n");
		tokenizeFunParameters(mapfun, &TOP_collections_parameters, &attributes, &normalizationAttributes);

		//Get the number of parameters collections
		int collectionsCount;

		IDAM_LOG(UDA_LOG_DEBUG, "Calling getTopCollectionsCount() from WEST plugin for shape_of_tsmat_collect case\n");
		getTopCollectionsCount(TOP_collections_parameters, &collectionsCount);

		printNum("Collections count : ", collectionsCount);

		IDAM_LOG(UDA_LOG_DEBUG, "Calling tokenizeFunCollect() from WEST plugin\n");

		//Get the total size by adding all collections lengths
		int i;
		int parametersSize = 0;
		for (i = 0; i < collectionsCount; i++) {
			char* command = NULL;
			IDAM_LOG(UDA_LOG_DEBUG, "Calling getCommand() from WEST plugin for shape_of_tsmat_collect case\n");
			getCommand(i, &command, TOP_collections_parameters); //get one command
			IDAM_LOGF(UDA_LOG_DEBUG, "Command : %s\n", command);
			int nb_val = 0;
			IDAM_LOG(UDA_LOG_DEBUG, "Calling getShapeOf() from WEST plugin for shape_of_tsmat_collect case\n");
			getShapeOf(command, shotNumber, &nb_val);

			printNum("nb_val : ", nb_val);

			IDAM_LOG(UDA_LOG_DEBUG, "after getShapeOf\n");
			parametersSize += nb_val;
		}

		printNum("Parameters size : ", parametersSize);

		data_block->data_type = UDA_TYPE_INT;
		data_block->data = malloc(sizeof(int));
		*((int*)data_block->data) = parametersSize;

		break;
	}

	case 2: {
		IDAM_LOG(UDA_LOG_DEBUG, "Case of set_value from WEST plugin\n");

		IDAM_LOG(UDA_LOG_DEBUG, "Calling tokenizeFunParameters() from WEST plugin\n");
		char* value = NULL;
		tokenizeFunParameters(mapfun, &value, &attributes, &normalizationAttributes);

		int data_type;
		getReturnType(attributes, &data_type);
		char* buffer = NULL;


		if (data_type == UDA_TYPE_FLOAT) {
			IDAM_LOG(UDA_LOG_DEBUG, "UDA_TYPE_FLOAT requested from WEST plugin\n");
			buffer = malloc(sizeof(float));
			float* f_buf = (float*)buffer;
			*f_buf = atof(value);
		} else if (data_type == UDA_TYPE_INT) {
			IDAM_LOG(UDA_LOG_DEBUG, "UDA_TYPE_INT requested from WEST plugin\n");
			buffer = malloc(sizeof(int));
			int* i_buf = (int*)buffer;
			*i_buf = atoi(value);
		} else if (data_type == UDA_TYPE_STRING) {
			IDAM_LOG(UDA_LOG_DEBUG, "UDA_TYPE_STRING requested from WEST plugin\n");
			buffer = strdup(value);
		} else {
			int err = 999;
			IDAM_LOG(UDA_LOG_DEBUG, "Unsupported data type\n");
			addIdamError(CODEERRORTYPE, "Unsupported data type", err, "");
		}

		IDAM_LOG(UDA_LOG_DEBUG, "Calling setStaticValue()\n");
		float normalizationFactor = 1;
		getNormalizationFactor(&normalizationFactor, normalizationAttributes);
		setStaticValue(data_type, data_block, buffer, 0, normalizationFactor); //index is always 0 in this case
		IDAM_LOG(UDA_LOG_DEBUG, "Returning from setStaticValue()\n");

		free(value);
		free(buffer);
		break;
	}

	case 3: {
		IDAM_LOG(UDA_LOG_DEBUG, "Case of tsmat from WEST plugin\n");
		IDAM_LOG(UDA_LOG_DEBUG, "Calling tokenizeFunParameters() from WEST plugin\n");
		tokenizeFunParameters(mapfun, &TOP_collections_parameters, &attributes, &normalizationAttributes);
		execute_tsmat_without_idam_index(TOP_collections_parameters, attributes, shotNumber, data_block,
				normalizationAttributes);
		break;
	}

	case 4: {
		IDAM_LOG(UDA_LOG_DEBUG, "Case of set_value_collect from WEST plugin\n");
		IDAM_LOG(UDA_LOG_DEBUG, "Calling tokenizeFunParameters() from WEST plugin\n");
		tokenizeFunParameters(mapfun, &TOP_collections_parameters, &attributes, &normalizationAttributes);
		execute_setvalue_collect(TOP_collections_parameters, attributes, shotNumber, data_block, nodeIndices,
				normalizationAttributes);
		break;
	}

	case 5: {
		IDAM_LOG(UDA_LOG_DEBUG, "Case of set_channels_validity from WEST plugin\n");
		char* unvalid_channels_request = NULL; //used for interfero_polarimeter IDS, example : invalid_channels:2:1,2
		int unvalid_channels_size;
		int* unvalid_channels_list = NULL;
		IDAM_LOG(UDA_LOG_DEBUG, "Calling tokenize_set_channels_validity() from WEST plugin\n");
		tokenize_set_channels_validity(mapfun, &unvalid_channels_request, &attributes);

		getUnvalidChannelsSize(unvalid_channels_request, &unvalid_channels_size);
		unvalid_channels_list = (int *)malloc(sizeof(int)*unvalid_channels_size);

		getUnvalidChannels(unvalid_channels_request, unvalid_channels_list);
		execute_setchannels_validity(unvalid_channels_list, unvalid_channels_size, attributes, shotNumber, data_block, nodeIndices);

		free(unvalid_channels_request);
		free(unvalid_channels_list);

		break;
	}

	}

	free(fun_name);
	free(TOP_collections_parameters);
	free(attributes);
	free(normalizationAttributes);
	return 0;
}

void execute_setchannels_validity(int* unvalid_channels_list, int unvalid_channels_size, char* attributes,
		int shotNumber, DATA_BLOCK* data_block, int* nodeIndices)
{
	int requestedIndex = getNumIDAMIndex(attributes, nodeIndices);
	int channel_number = requestedIndex + 1; //TODO TO CHECK
	IDAM_LOGF(UDA_LOG_DEBUG, "channel number : %d\n", channel_number);

	if (isChannelValid(channel_number, unvalid_channels_list, unvalid_channels_size)) {
		IDAM_LOGF(UDA_LOG_DEBUG, "valid channel number : %d\n", channel_number);
		setStaticINTValue(UDA_TYPE_INT, data_block, 0, 1.0);
	}
	else
	{
		IDAM_LOGF(UDA_LOG_DEBUG, "unvalid channel number : %d\n", channel_number);
		setStaticINTValue(UDA_TYPE_INT, data_block, -1, 1.0);
	}
}

void execute_setvalue_collect(const char* TOP_collections_parameters, char* attributes,
		int shotNumber, DATA_BLOCK* data_block, int* nodeIndices, char* normalizationAttributes)
{

	int collectionsCount;
	getTopCollectionsCount(TOP_collections_parameters, &collectionsCount);

	int* l;
	l = (int*)calloc(collectionsCount, sizeof(int));

	int i;
	for (i = 0; i < collectionsCount; i++) {
		char* command = NULL;
		getCommand(i, &command, TOP_collections_parameters);
		int nb_val = 0;
		getShapeOf(command, shotNumber, &nb_val);
		l[i] = nb_val;
	}

	int requestedIndex = getNumIDAMIndex(attributes, nodeIndices);

	int searchedArray;
	int searchedArrayIndex;

	searchIndices(requestedIndex, l, &searchedArray, &searchedArrayIndex);
	IDAM_LOGF(UDA_LOG_DEBUG, "searchedArray : %d\n", searchedArray);
	IDAM_LOGF(UDA_LOG_DEBUG, "searchedArrayIndex : %d\n", searchedArrayIndex);

	char* command = NULL;
	getCommand(searchedArray, &command, TOP_collections_parameters);

	char* value;
	getValueCollect(command, &value, nodeIndices);
	IDAM_LOGF(UDA_LOG_DEBUG, "Command : %s\n", command);

	int data_type;
	getReturnType(attributes, &data_type);

	char* buffer = NULL;

	if (data_type == UDA_TYPE_FLOAT) {
		IDAM_LOG(UDA_LOG_DEBUG, "UDA_TYPE_FLOAT requested from WEST plugin\n");
		buffer = malloc(sizeof(float));
		float* f_buf = (float*)buffer;
		*f_buf = atof(value);
	} else if (data_type == UDA_TYPE_DOUBLE) {
		IDAM_LOG(UDA_LOG_DEBUG, "UDA_TYPE_DOUBLE requested from WEST plugin\n");
		buffer = malloc(sizeof(double));
		double* f_buf = (double*)buffer;
		*f_buf = atof(value);
	} else if (data_type == UDA_TYPE_INT) {
		IDAM_LOG(UDA_LOG_DEBUG, "UDA_TYPE_INT requested from WEST plugin\n");
		buffer = malloc(sizeof(int));
		int* i_buf = (int*)buffer;
		*i_buf = atoi(value);
	} else if (data_type == UDA_TYPE_STRING) {
		IDAM_LOG(UDA_LOG_DEBUG, "UDA_TYPE_STRING requested from WEST plugin\n");
		buffer = strdup(value);
	} else {
		int err = 999;
		IDAM_LOG(UDA_LOG_DEBUG, "Unsupported data type in execute_setvalue_collect\n");
		addIdamError(CODEERRORTYPE, "Unsupported data type", err, "");
	}

	IDAM_LOGF(UDA_LOG_DEBUG, "Found value: %s\n", value);

	float normalizationFactor = 1;
	getNormalizationFactor(&normalizationFactor, normalizationAttributes);
	setStaticValue(data_type, data_block, buffer, 0, normalizationFactor);

	free(value);
	free(command);
	free(buffer);


}

void execute_tsmat_collect(const char* TOP_collections_parameters, char* attributes,
		int shotNumber, DATA_BLOCK* data_block, int* nodeIndices, char* normalizationAttributes)
{

	int collectionsCount;
	getTopCollectionsCount(TOP_collections_parameters, &collectionsCount);

	int* l;
	l = (int*)calloc(collectionsCount, sizeof(int));

	int i;
	for (i = 0; i < collectionsCount; i++) {
		char* command = NULL;
		getCommand(i, &command, TOP_collections_parameters);
		int nb_val = 0;
		getShapeOf(command, shotNumber, &nb_val);
		l[i] = nb_val;
	}

	int requestedIndex = getNumIDAMIndex(attributes, nodeIndices);

	int searchedArray;
	int searchedArrayIndex;

	searchIndices(requestedIndex, l, &searchedArray, &searchedArrayIndex);

	char* command = NULL;
	getCommand(searchedArray, &command, TOP_collections_parameters);

	char* prod_name = NULL; //DMAG, ...
	char* object_name = NULL; //GMAG_BNORM, ...
	char* param_name = NULL; //PosR, ...

	int data_type;
	getReturnType(attributes, &data_type);

	//Tokenize mapfun string to get function parameters
	tokenizeCommand(command, &prod_name, &object_name, &param_name);

	char* value = NULL;
	int val_nb = l[searchedArray];
	int nb_val;

	//Reading static parameters using TSLib
	int status = readStaticParameters(&value, &nb_val, shotNumber, prod_name, object_name, param_name, val_nb);
	if (status != 0) {
		int err = 901;
		addIdamError(CODEERRORTYPE, "Unable to read static data from WEST", err, "");
	}

	float normalizationFactor = 1;
	getNormalizationFactor(&normalizationFactor, normalizationAttributes);
	setStaticValue(data_type, data_block, value, searchedArrayIndex, normalizationFactor);

	free(command);
	free(prod_name);
	free(object_name);
	free(param_name);
	free(value);
}

void execute_tsmat_without_idam_index(const char* TOP_collections_parameters, char* attributes,
		int shotNumber, DATA_BLOCK* data_block, char* normalizationAttributes)
{

	char* command;
	getCommand(0, &command, TOP_collections_parameters);

	char* prod_name = NULL; //DMAG, ...
	char* object_name = NULL; //GMAG_BNORM, ...
	char* param_name = NULL; //PosR, ...

	int data_type;
	getReturnType(attributes, &data_type);

	//Tokenize mapfun string to get function parameters
	tokenizeCommand(command, &prod_name, &object_name, &param_name);

	int val_nb = 0;
	getShapeOf(command, shotNumber, &val_nb);

	char* value = NULL;
	int nb_val;

	//Reading static parameters using TSLib
	int status = readStaticParameters(&value, &nb_val, shotNumber, prod_name, object_name, param_name, val_nb);
	if (status != 0) {
		int err = 901;
		addIdamError(CODEERRORTYPE, "Unable to read static data from WEST", err, "");
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
		int err = 999;
		IDAM_LOG(UDA_LOG_DEBUG, "Unsupported rank from execute_tsmat_without_idam_index()\n");
		addIdamError(CODEERRORTYPE, "Unsupported data type", err, "");
	}

	free(command);
	free(prod_name);
	free(object_name);
	free(param_name);
	free(value);

}

int GetDynamicData(int shotNumber, const char* mapfun, DATA_BLOCK* data_block, int* nodeIndices)
{

	IDAM_LOG(UDA_LOG_DEBUG, "Entering GetDynamicData() -- WEST plugin\n");
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

	IDAM_LOG(UDA_LOG_DEBUG, "Evaluating the request type (tsbase_collect, tsbase_time, ...)\n");
	int fun;

	if (strcmp(fun_name, "tsbase_collect") == 0) {
		IDAM_LOG(UDA_LOG_DEBUG, "tsbase_collect request \n");
		fun = 0;
		tokenizeFunParameters(mapfun, &TOP_collections_parameters, &attributes, &normalizationAttributes);
	} else if (strcmp(fun_name, "tsbase_time") == 0) {
		IDAM_LOG(UDA_LOG_DEBUG, "tsbase_time request \n");
		fun = 1;
		tokenizeFunParameters(mapfun, &TOP_collections_parameters, &attributes, &normalizationAttributes);
	} else if (strcmp(fun_name, "tsbase_collect_with_channels") == 0) {
		IDAM_LOG(UDA_LOG_DEBUG, "tsbase_collect_with_channels request \n");
		fun = 2;
		tokenizeFunParametersWithChannels(mapfun, &unvalid_channels, &TOP_collections_parameters, &attributes, &normalizationAttributes);
		getUnvalidChannelsSize(unvalid_channels, &unvalid_channels_size);
		unvalid_channels_list = (int *)malloc(sizeof(int)*unvalid_channels_size);
		getUnvalidChannels(unvalid_channels, unvalid_channels_list);
	} else if (strcmp(fun_name, "tsbase_time_with_channels") == 0) {
		IDAM_LOG(UDA_LOG_DEBUG, "tsbase_time_with_channels request \n");
		fun = 3;
		tokenizeFunParametersWithChannels(mapfun, &unvalid_channels, &TOP_collections_parameters, &attributes, &normalizationAttributes);
		getUnvalidChannelsSize(unvalid_channels, &unvalid_channels_size);
		unvalid_channels_list = (int *)malloc(sizeof(int)*unvalid_channels_size);
		getUnvalidChannels(unvalid_channels, unvalid_channels_list);
	}

	IDAM_LOG(UDA_LOG_DEBUG, "now searching for signals\n");
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

		IDAM_LOG(UDA_LOG_DEBUG, "Group of signals1 ?\n");
		getSignalType(objectName, shotNumber, &signalType);

		if (signalType == 2) {
			getExtractionsCount(objectName, shotNumber, occ, &nb_extractions);
		}

		printNum("Number of extractions : ", nb_extractions);

		extractionsCount[i] = nb_extractions;
	}

	IDAM_LOG(UDA_LOG_DEBUG, "searching for IDAM index\n");
	IDAM_LOGF(UDA_LOG_DEBUG, "attributes : %s\n", attributes);

	int requestedIndex = getNumIDAMIndex(attributes, nodeIndices);

	printNum("Requested index (from IDAM call) : ", requestedIndex);

	IDAM_LOG(UDA_LOG_DEBUG, "searching for the array according to the static index\n");
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

	IDAM_LOG(UDA_LOG_DEBUG, "getting the command\n");
	IDAM_LOGF(UDA_LOG_DEBUG, "TOP_collections_parameters: %s\n", TOP_collections_parameters);

	int status = getCommand(searchedArray, &command, TOP_collections_parameters);

	if (status != 0) {
		int err = 901;
		IDAM_LOG(UDA_LOG_DEBUG, "Unable to get command\n");
		addIdamError(CODEERRORTYPE, "Unable to get command", err, "");
	}

	IDAM_LOGF(UDA_LOG_DEBUG, "command: %s\n", command);
	IDAM_LOG(UDA_LOG_DEBUG, "Getting object name\n");
	getObjectName(&objectName, command);

	IDAM_LOG(UDA_LOG_DEBUG, "Group of signals ?\n");
	getSignalType(objectName, shotNumber, &signalType);

	if (signalType == 2) { //signal is a group of signals, so we append extraction chars to signal name
		IDAM_LOG(UDA_LOG_DEBUG, "Signal belongs to a group of signals\n");
		char result[50];
		addExtractionChars(result, objectName,
				searchedArrayIndex + 1); //Concatenate signalName avec %searchedArrayIndex + 1
		objectName = strdup(result);
	} else {
		IDAM_LOG(UDA_LOG_DEBUG, "Signal does not belong to a group of signals\n");
	}

	IDAM_LOGF(UDA_LOG_DEBUG, "Object name: %s\n", objectName);

	float* time = NULL;
	float* data = NULL;
	int len;

	int rang[2] = { 0, 0 };
	status = readSignal(objectName, shotNumber, 0, rang, &time, &data, &len);

	//float f1 = data[0];
	IDAM_LOGF(UDA_LOG_DEBUG, "%s\n", "First values...");
	int j;
	for (j=0; j <10; j++) {
		IDAM_LOGF(UDA_LOG_DEBUG, "value : %f\n", data[j]);
	}

	//IDAM_LOGF(UDA_LOG_DEBUG, "First value 1: %f\n", data(1));

	IDAM_LOG(UDA_LOG_DEBUG, "End of reading signal\n");

	/*if (status != 0) {
            int err = 901;
            addIdamError(CODEERRORTYPE, "Unable to read dynamic data from WEST !", err, "");
        }*/

	//IDAM_LOG(UDA_LOG_DEBUG, "After error handling\n");

	IDAM_LOG(UDA_LOG_DEBUG, "Getting normalization factor, if any\n");
	float normalizationFactor = 1;
	getNormalizationFactor(&normalizationFactor, normalizationAttributes);
	IDAM_LOG(UDA_LOG_DEBUG, "Starting data normalization\n");
	multiplyFloat(data, normalizationFactor, len);
	IDAM_LOG(UDA_LOG_DEBUG, "end of data normalization, if any\n");

	free(data_block->dims);

	data_block->rank = 1;
	data_block->data_type = UDA_TYPE_FLOAT;
	data_block->data_n = len;

	if (fun == 0) {
		data_block->data = (char*)data;
	} else if (fun == 1) {
		data_block->data = (char*)time;
	} else if (fun == 2) {
		//TODO if (isChannelValid(requestedIndex + 1, unvalid_channels_list, unvalid_channels_size))
		//{
			//IDAM_LOGF(UDA_LOG_DEBUG, "Setting data for valid channel: %d\n", requestedIndex + 1);
			data_block->data = (char*)data;
		//}
	} else if (fun == 3) {
		//TODO if (isChannelValid(requestedIndex + 1, unvalid_channels_list, unvalid_channels_size))
			data_block->data = (char*)time;
	}

	data_block->dims =
			(DIMS*)malloc(data_block->rank * sizeof(DIMS));

	for (i = 0; i < data_block->rank; i++) {
		initDimBlock(&data_block->dims[i]);
	}
	data_block->dims[0].data_type = UDA_TYPE_FLOAT;
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
	IDAM_LOG(UDA_LOG_DEBUG, "TEST_FINAL\n");
	return 0;
}

void getShapeOf(const char* command, int shotNumber, int* nb_val)
{

	char* prod_name = NULL; //DMAG, ...
	char* object_name = NULL;
	char* param_name = NULL;

	IDAM_LOGF(UDA_LOG_DEBUG, "Calling tokenizeCommand with command: %s\n", command);

	//Tokenize mapfun string to get function parameters, return type and arguments (#1, #2,...) to use
	tokenizeCommand(command, &prod_name, &object_name, &param_name);

	char* value = NULL;
	int val_nb = 1;
	//get the size of available data
	IDAM_LOGF(UDA_LOG_DEBUG, "DEBUG : prod_name: %s\n", prod_name);
	IDAM_LOGF(UDA_LOG_DEBUG, "DEBUG : object: %s\n", object_name);
	IDAM_LOGF(UDA_LOG_DEBUG, "DEBUG : param: %s\n", param_name);

	int status = readStaticParameters(&value, nb_val, shotNumber, prod_name, object_name, param_name, val_nb);
	printNum("status : ", status);

	if (status != 0) {
		IDAM_LOG(UDA_LOG_DEBUG, "Error calling readStaticParameters\n");
		int err = 901;
		addIdamError(CODEERRORTYPE, "Unable to get shape of static data from WEST", err, "");
	}

	free(prod_name);
	free(object_name);
	free(param_name);
}

//Cast the results returned by tsmat according to the type of the data and set IDAM data
void setStatic1DValue(int data_type, DATA_BLOCK* data_block, char* value, int val_nb, float normalizationFactor)
{
	if (data_type == UDA_TYPE_FLOAT) {
		IDAM_LOG(UDA_LOG_DEBUG, "handling float 1D in setStatic1DValue()\n");

		char val_nb_str[10];
		sprintf(val_nb_str, "%d", val_nb);
		IDAM_LOGF(UDA_LOG_DEBUG, "val_nb : %s\n", val_nb_str);

		if (val_nb == 0) {
			int err = 901;
			addIdamError(CODEERRORTYPE, __func__, err, "val_nb is equals to 0 !");
		}

		data_block->data_type = UDA_TYPE_FLOAT;
		data_block->data = malloc(val_nb * sizeof(float));
		float* pt_float = (float*)value;
		multiplyFloat(pt_float, normalizationFactor, val_nb);
		IDAM_LOG(UDA_LOG_DEBUG, "handling float 1D in setStatic1DValue21()\n");
		*((float*)data_block->data) = *pt_float;
		IDAM_LOG(UDA_LOG_DEBUG, "handling float 1D in setStatic1DValue22()\n");
	} else if (data_type == UDA_TYPE_INT) {
		IDAM_LOG(UDA_LOG_DEBUG, "handling int 1D in setStatic1DValue()\n");
		data_block->data_type = UDA_TYPE_INT;
		data_block->data = malloc(val_nb * sizeof(int));
		int* pt_int = (int*)value;
		multiplyInt(pt_int, normalizationFactor, val_nb);
		*((int*)data_block->data) = *pt_int;
		IDAM_LOG(UDA_LOG_DEBUG, "handling int 1D in setStatic1DValue2()\n");
	} else {
		int err = 999;
		IDAM_LOG(UDA_LOG_DEBUG, "Unsupported data type from setStatic1DValue()\n");
		addIdamError(CODEERRORTYPE, __func__, err, "Unsupported data type");
	}
}

void setStaticINTValue(int data_type, DATA_BLOCK* data_block, int value, float normalizationFactor)
{
	IDAM_LOGF(UDA_LOG_DEBUG, "handling in setStaticINTValue(): %d\n", value);
	data_block->data_type = UDA_TYPE_INT;
	data_block->data = malloc(1 * sizeof(int));
	((int*)data_block->data)[0] = value * (int) normalizationFactor;
}

//Cast the results returned by tsmat according to the type of the data and set IDAM data
void setStaticValue(int data_type, DATA_BLOCK* data_block, char* value, int requestedIndex, float normalizationFactor)
{
	IDAM_LOG(UDA_LOG_DEBUG, "Entering setStaticValue()\n");
	printNum("requested index : ", requestedIndex);
	if (data_type == UDA_TYPE_DOUBLE) {
		IDAM_LOG(UDA_LOG_DEBUG, "handling double in setStaticValue()\n");
		data_block->data_type = UDA_TYPE_DOUBLE;
		data_block->data = malloc(1 * sizeof(double));
		double* pt_double = (double*)value;
		((double*)data_block->data)[0] = pt_double[requestedIndex] * (double) normalizationFactor;

	} else if (data_type == UDA_TYPE_FLOAT) {
		IDAM_LOGF(UDA_LOG_DEBUG, "handling float in setStaticValue(): %d, %g\n", requestedIndex, normalizationFactor);
		data_block->data_type = UDA_TYPE_FLOAT;
		data_block->data = malloc(1 * sizeof(float));
		float* pt_float = (float*)value;
		((float*)data_block->data)[0] = pt_float[requestedIndex] * normalizationFactor;

	} else if (data_type == UDA_TYPE_LONG) {
		IDAM_LOG(UDA_LOG_DEBUG, "handling long in setStaticValue()\n");
		data_block->data_type = UDA_TYPE_LONG;
		data_block->data = malloc(1 * sizeof(long));
		long* pt_long = (long*)value;
		((long*)data_block->data)[0] = pt_long[requestedIndex] * (long) normalizationFactor;

	} else if (data_type == UDA_TYPE_INT) {
		IDAM_LOG(UDA_LOG_DEBUG, "handling int in setStaticValue()\n");
		data_block->data_type = UDA_TYPE_INT;
		data_block->data = malloc(1 * sizeof(int));
		int* pt_int = (int*)value;
		IDAM_LOGF(UDA_LOG_DEBUG, "handling in setStaticValue(): %d\n", *pt_int);
		((int*)data_block->data)[0] = pt_int[requestedIndex] * (int) normalizationFactor;

	} else if (data_type == UDA_TYPE_SHORT) {
		IDAM_LOG(UDA_LOG_DEBUG, "handling short in setStaticValue()\n");
		data_block->data_type = UDA_TYPE_SHORT;
		data_block->data = malloc(1 * sizeof(short));
		int* pt_short = (int*)value;
		((short*)data_block->data)[0] = pt_short[requestedIndex] * (short) normalizationFactor;

	} else if (data_type == UDA_TYPE_STRING) {
		IDAM_LOG(UDA_LOG_DEBUG, "handling string in setStaticValue()\n");
		data_block->data_type = UDA_TYPE_STRING;
		IDAM_LOG(UDA_LOG_DEBUG, "setting value\n");
		data_block->data = strdup(value);
		IDAM_LOGF(UDA_LOG_DEBUG, "%s\n", data_block->data);

	} else {
		int err = 999;
		IDAM_LOG(UDA_LOG_DEBUG, "Unsupported data type from setStaticValue()\n");
		addIdamError(CODEERRORTYPE, "Unsupported data type", err, "");
	}
}


int convertToInt(char* value)
{
	int i = UDA_TYPE_UNKNOWN;
	int err = 0;

	if (STR_EQUALS(value, "vecstring_type") || STR_EQUALS(value, "string") || STR_EQUALS(value, "STR_0D")) {
		i = UDA_TYPE_STRING;
	} else if (STR_EQUALS(value, "vecflt_type") || STR_EQUALS(value, "float") || STR_EQUALS(value, "FLT_0D")) {
		i = UDA_TYPE_FLOAT;
	} /*else if (STR_EQUALS(value, "double")) {
		i = UDA_TYPE_DOUBLE;
	}*/else if (STR_EQUALS(value, "vecint_type") || STR_EQUALS(value, "integer") || STR_EQUALS(value, "int") ||
			STR_EQUALS(value, "INT_0D")) {
		i = UDA_TYPE_INT;
	} else {
		err = 999;
		addIdamError(CODEERRORTYPE, "west convertToInt() : Unsupported data type", err, "");
	}
	return i;
}

void getFunName(const char* s, char** fun_name)
{
	const char delim[] = ";";
	char* s_copy = strdup(s);
	*fun_name = strdup(strtok(s_copy, delim));
	RemoveSpaces(*fun_name);
	free(s_copy);
}

void tokenizeFunParameters(const char* s, char** TOP_collections_parameters, char** attributes, char** normalizationAttributes)
{
	const char delim[] = ";";
	char* s_copy = strdup(s);
	strdup(strtok(s_copy, delim)); //function name
	*TOP_collections_parameters = strdup(strtok(NULL, delim));
	RemoveSpaces(*TOP_collections_parameters);
	*attributes = strdup(strtok(NULL, delim));
	RemoveSpaces(*attributes);
	char* token = strtok(NULL, delim);
	if (token != NULL) {
		*normalizationAttributes = strdup(token);
		RemoveSpaces(*normalizationAttributes);
	}
	free(s_copy);
}

void tokenizeFunParametersWithChannels(const char* s, char** unvalid_channels, char** TOP_collections_parameters,
		char** attributes, char** normalizationAttributes)
{
	const char delim[] = ";";
	char* s_copy = strdup(s);
	char* s1 = strdup(strtok(s_copy, delim)); //function name
	IDAM_LOGF(UDA_LOG_DEBUG, "fun name : %s\n", s1);
	*unvalid_channels = strdup(strtok(NULL, delim));
	IDAM_LOGF(UDA_LOG_DEBUG, "unvalid channels : %s\n", *unvalid_channels);
	*TOP_collections_parameters = strdup(strtok(NULL, delim));
	RemoveSpaces(*TOP_collections_parameters);
	*attributes = strdup(strtok(NULL, delim));
	RemoveSpaces(*attributes);
	char* token = strtok(NULL, delim);
	if (token != NULL) {
		*normalizationAttributes = strdup(token);
		RemoveSpaces(*normalizationAttributes);
	}
	free(s_copy);
	free(s1);
}

void tokenize_set_channels_validity(const char* s, char** unvalid_channels, char** attributes)
{
	const char delim[] = ";";
	char* s_copy = strdup(s);
	strdup(strtok(s_copy, delim)); //function name
	*unvalid_channels = strdup(strtok(NULL, delim)); //example: unvalid_channels:2:1,2
	*attributes = strdup(strtok(NULL, delim));
	RemoveSpaces(*attributes);
	free(s_copy);
}

void getUnvalidChannelsSize(char* unvalid_channels, int* size)
{
	const char delim[] = ":";
	char* s_copy = strdup(unvalid_channels);
	char* token = strdup(strtok(s_copy, delim)); //should return "unvalid_channels" string
	token = strdup(strtok(NULL, delim)); //return the number of unvalid channels
	*size = atoi(token);
	free(s_copy);
	free(token);
}

void getUnvalidChannels(char* unvalid_channels, int* v)
{
	const char delim[] = ":";
	char* s_copy = strdup(unvalid_channels);
	char* token = strdup(strtok(s_copy, delim)); //should return "unvalid_channels" string

	token = strdup(strtok(NULL, delim)); //return the number of unvalid channels
	int n = atoi(token);

	IDAM_LOGF(UDA_LOG_DEBUG, "unvalid channels count : %d\n", n);

	token = strdup(strtok(NULL, delim)); //return for example "1,2" if channels 1 and 2 are not valid

	IDAM_LOGF(UDA_LOG_DEBUG, "unvalid channels list : %s\n", token);

	const char delim2[] = ",";
	char* s_copy2 = strdup(token);

	int i;
	for (i = 0; i < n; i++)
	{
		if (i == 0) {
			v[i] = atoi(strdup(strtok(s_copy2, delim2)));
		}
		else {
			token = strdup(strtok(NULL, delim2));
			v[i] = atoi(token);
		}
		IDAM_LOGF(UDA_LOG_DEBUG, "unvalid channel : %d\n", v[i]);
	}
	free(s_copy);
	free(s_copy2);
	free(token);
}

int isChannelValid(int channel_number, int* unvalid_channels_list, int unvalid_channels_list_size)
{
	int i;
	IDAM_LOGF(UDA_LOG_DEBUG, "unvalid_channels_list_size : %d\n", unvalid_channels_list_size);
	for (i = 0; i < unvalid_channels_list_size; i++)
	{
		IDAM_LOGF(UDA_LOG_DEBUG, "unvalid_channels_list[i] : %d\n", unvalid_channels_list[i]);
		if (unvalid_channels_list[i] == channel_number)
			return 0;
	}
	return 1;
}

int getCommand(int i, char** command, const char* TOP_collections_parameters)
{

	char* s_copy = strdup(TOP_collections_parameters);
	const char delim[] = ",";
	int j = 0;
	char* token = strdup(strtok(s_copy, delim));
	if (token == NULL) {
		return -1;
	}
	while (token != NULL) {
		if (i == j) {
			*command = strdup(token);
			RemoveSpaces(*command);
			free(s_copy);
			return 0;
		}
		token = strtok(NULL, delim);
		j++;
	}
	free(s_copy);
	return -1;
}

void getTopCollectionsCount(const char* TOP_collections_parameters, int* collectionsCount)
{
	*collectionsCount = 0;
	char* s_copy = strdup(TOP_collections_parameters);
	const char delim[] = ",";
	char* token = strtok(s_copy, delim);
	while (token != NULL) {
		(*collectionsCount)++;
		token = strtok(NULL, delim);
	}
	free(s_copy);
	free(token);
}

void getNormalizationFactor(float* normalizationFactor, char* normalizationAttributes)
{
	char* s_copy = NULL;
	char* operation = NULL;
	char* funname = NULL;
	char* csteStr = NULL;

	if (normalizationAttributes != NULL) {
		IDAM_LOG(UDA_LOG_DEBUG, "normalization attributes found\n");
		const char delim[] = ":";
		s_copy = strdup(normalizationAttributes);
		operation = strdup(strtok(s_copy, delim));
		if (STR_EQUALS("multiply", operation)) {
			IDAM_LOG(UDA_LOG_DEBUG, "Multiply operation\n");
			funname = strdup(strtok(NULL, delim));
			if (STR_EQUALS("cste", funname)) {
				IDAM_LOG(UDA_LOG_DEBUG, "multiply data by constant value\n");
				csteStr = strdup(strtok(NULL, delim));
				IDAM_LOGF(UDA_LOG_DEBUG, "%s\n", csteStr);
				*normalizationFactor = atof(csteStr);
			} else {
				int err = 999;
				IDAM_LOG(UDA_LOG_DEBUG, "Unsupported operand for 'multiply' operation\n");
				addIdamError(CODEERRORTYPE, "Unsupported operand for 'multiply' operation", err,
						"");
			}

		} else {
			int err = 999;
			IDAM_LOG(UDA_LOG_DEBUG, "Unsupported operation to apply\n");
			addIdamError(CODEERRORTYPE, "Unsupported operation to apply", err, "");
		}
	} else {
		IDAM_LOG(UDA_LOG_DEBUG, "no normalization attributes found\n");
	}
	free(s_copy);
	free(operation);
	free(funname);
	free(csteStr);
}

void multiplyFloat(float* p, float factor, int val_nb)
{
	IDAM_LOG(UDA_LOG_DEBUG, "Entering multiplyFloat...\n");
	IDAM_LOGF(UDA_LOG_DEBUG, "val_nb : %d\n", val_nb);
	IDAM_LOGF(UDA_LOG_DEBUG, "factor : %f\n", factor);
	if (factor != 1) {
		int i;
		for (i = 0; i < val_nb; i++) {
			p[i] *= factor;
		}
	}
}

void multiplyInt(int* p, float factor, int val_nb)
{
	if (factor != 1) {
		int i;
		for (i = 0; i < val_nb; i++)
			p[i] *= (int)factor;
	}
}

int getNumIDAMIndex(char *attributes, int* nodeIndices) {

	char* s_copy = strdup(attributes);
	const char delim[] = ":";
	char* charIDAMIndex = NULL;
	strtok(s_copy, delim); //rank
	strtok(NULL, delim); //type
	charIDAMIndex = strdup(strtok(NULL, delim)); //#0
	char firstChar;
	firstChar = charIDAMIndex[0];

	if (firstChar == '#') {
		IDAM_LOG(UDA_LOG_DEBUG, "index specified in IDAM request\n");
		return nodeIndices[atoi(&charIDAMIndex[1])] - 1;
	}
	else {
		IDAM_LOG(UDA_LOG_DEBUG, "idam index hard coded in mapping file\n");
		return atoi(&firstChar) - 1;
	}
	free(s_copy);
	free(charIDAMIndex);
}

int getNumIDAMIndex2(char *s, int* nodeIndices) {

	char firstChar;
	firstChar = s[0];

	if (firstChar == '#') {
		IDAM_LOG(UDA_LOG_DEBUG, "index specified in IDAM request\n");
		return nodeIndices[atoi(&s[1])] - 1;
	}
	else {
		IDAM_LOG(UDA_LOG_DEBUG, "no index specified\n");
		return -1;
	}
}

void getReturnType(char* attributes, int* dataType)
{
	char* s_copy = strdup(attributes);
	const char delim[] = ":";
	strtok(s_copy, delim); //the rank
	*dataType = convertToInt(strdup(strtok(NULL, delim)));
	free(s_copy);
}

void getRank(char* attributes, int* rank)
{
	char* s_copy = strdup(attributes);
	const char delim[] = ":";
	char* rankStr = strdup(strtok(s_copy, delim));
	*rank = atoi(&rankStr[0]);
	free(s_copy);
	free(rankStr);
}

void getValueCollect(char* command, char** value, int* nodeIndices)
{
	char* s_copy = strdup(command);
	const char delim[] = ":";
	strtok(s_copy, delim); //prod_name
	strtok(NULL, delim); //object name
	strtok(NULL, delim); //param name
	*value = strdup(strtok(NULL, delim)); //value for the setvalue_collect function
	char* token = strtok(NULL, delim);
	if (token != NULL) {
		int UDAIndex = getNumIDAMIndex2(token, nodeIndices);
		char str[2];
		sprintf(str, "%s", "_");
		sprintf(str, "%d", UDAIndex);
		*value = strcat(*value, str);
	}
	free(s_copy);
}

void tokenizeCommand(const char* s, char** prod_name, char** obj_name, char** param_name)
{
	char* s_copy = strdup(s);
	const char delim[] = ":";
	IDAM_LOGF(UDA_LOG_DEBUG, "Tokenizing: %s\n", s);
	*prod_name = strdup(strtok(s_copy, delim));
	RemoveSpaces(*prod_name);
	IDAM_LOGF(UDA_LOG_DEBUG, "%s\n", *prod_name);
	*obj_name = strdup(strtok(NULL, delim));
	RemoveSpaces(*obj_name);
	IDAM_LOGF(UDA_LOG_DEBUG, "%s\n", *obj_name);
	*param_name = strdup(strtok(NULL, delim));
	RemoveSpaces(*param_name);
	free(s_copy);
}

void getObjectName(char** obj_name, char* command)
{
	char* s_copy = strdup(command);
	const char delim[] = ":";
	strdup(strtok(s_copy, delim));
	*obj_name = strdup(strtok(NULL, delim));
	RemoveSpaces(*obj_name);
	free(s_copy);
}

void searchIndices(int requestedIndex, int* l, int* searchedArray, int* searchedArrayIndex)
{
	*searchedArray = 0;
	*searchedArrayIndex = 0;

	if (requestedIndex < l[0]) {
		*searchedArrayIndex = requestedIndex;
		*searchedArray = 0;
	} else if (requestedIndex == l[0]) {
		*searchedArrayIndex = 0;
		*searchedArray = 1;
	} else if (requestedIndex > l[0]) {

		int i = 0;

		int d = requestedIndex - l[0];

		while (d >= 0) {
			i++;
			d = d - l[i];
		}
		int k = i - 1;
		int j;
		int s = 0;
		for (j = 0; j <= k; j++) {
			s += l[j];
		}
		*searchedArrayIndex = requestedIndex - s;
		*searchedArray = k + 1;
	}
}

void printNum(const char* label, int i)
{
	IDAM_LOGF(UDA_LOG_DEBUG, "%s -> %d\n", label, i);
}

void addExtractionChars(char* result, char* signalName, int extractionIndex)
{
	strcpy(result, signalName);
	strcat(result, "!");
	char s[5];
	sprintf(s, "%d", extractionIndex);
	strcat(result, s);
}

void RemoveSpaces(char* source)
{
	char* i = source;
	char* j = source;
	while (*j != 0) {
		*i = *j++;
		if (*i != ' ') {
			i++;
		}
	}
	*i = 0;
}

