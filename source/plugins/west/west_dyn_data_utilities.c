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

    if (status != 0) {
        int err = 901;
        addIdamError(CODEERRORTYPE, "Unable to get dynamic data", err, "");
        free(time);
        free(data);
    } else {
        UDA_LOG(UDA_LOG_DEBUG, "Getting normalization factor, if any\n");
        float normalizationFactor = 1;
        getNormalizationFactor(&normalizationFactor, normalizationAttributes);
        UDA_LOG(UDA_LOG_DEBUG, "Starting data normalization\n");
        multiplyFloat(data, normalizationFactor, len);
        UDA_LOG(UDA_LOG_DEBUG, "end of data normalization, if any\n");

        SetDynData(data_block, len, time, data, setTime);
    }
    return status;
}

int GetNormalizedDynamicData(int shotNumber, float** time, float** data, int* len, int* nodeIndices,
                             char* TOP_collections_parameters, char* attributes, char* normalizationAttributes)
{
    int status = GetDynData(shotNumber, time, data, len, nodeIndices,
                            TOP_collections_parameters, attributes);

    if (status != 0) {
        int err = 901;
        addIdamError(CODEERRORTYPE, "Unable to get dynamic data", err, "");
    } else {

        UDA_LOG(UDA_LOG_DEBUG, "Getting normalization factor, if any\n");
        float normalizationFactor = 1;
        getNormalizationFactor(&normalizationFactor, normalizationAttributes);
        UDA_LOG(UDA_LOG_DEBUG, "Starting data normalization\n");
        multiplyFloat(*data, normalizationFactor, *len);
        UDA_LOG(UDA_LOG_DEBUG, "end of data normalization, if any\n");
    }

    return status;
}

int GetDynData(int shotNumber, float** time, float** data, int* len, int* nodeIndices,
               char* TOP_collections_parameters, char* attributes)
{
    UDA_LOG(UDA_LOG_DEBUG, "now searching for signals\n");
    int collectionsCount;
    getTopCollectionsCount(TOP_collections_parameters, &collectionsCount);

    int* extractionsCount;
    extractionsCount = (int*)calloc(collectionsCount, sizeof(int));

    int signalType = 0;
    int i;
    char* command = NULL;
    int totalExtractions = 0;

    for (i = 0; i < collectionsCount; i++) {

        UDA_LOG(UDA_LOG_DEBUG, "In GetDynData, i: %d\n", i);

        getCommand(i, &command, TOP_collections_parameters);

        UDA_LOG(UDA_LOG_DEBUG, "In GetDynData, command: %s\n", command);

        char* objectName = NULL;
        getObjectName(&objectName, command);

        int nb_extractions = 0;
        int occ = 0;

        UDA_LOG(UDA_LOG_DEBUG, "Group of signals ?\n");
        getSignalType(objectName, shotNumber, &signalType);

        if (signalType == 2) {
            getExtractionsCount(objectName, shotNumber, occ, &nb_extractions);
        }

        printNum("Number of extractions : ", nb_extractions);

        extractionsCount[i] = nb_extractions;
        totalExtractions += extractionsCount[i];
        command = NULL;
    }

    UDA_LOG(UDA_LOG_DEBUG, "searching for IDAM index\n");
    UDA_LOG(UDA_LOG_DEBUG, "attributes : %s\n", attributes);

    int requestedIndex = getNumIDAMIndex(attributes, nodeIndices);

    printNum("Requested index (from IDAM call) : ", requestedIndex);

    UDA_LOG(UDA_LOG_DEBUG, "searching for the array according to the UDA index\n");
    int searchedArray;
    int searchedArrayIndex;
    searchIndices(requestedIndex, extractionsCount, &searchedArray, &searchedArrayIndex);

    char* objectName = NULL;

    printNum("searchedArrayIndex : ", searchedArrayIndex);
    printNum("searchedArray : ", searchedArray);

    //This patch means that if the signal does not belong to a group, so it can not be aggregate as signals groups
    //Example of an aggregate : tsbase_collect;DMAG:first_group, DMAG:second_group;float:#0

    if (signalType != 2) {
        searchedArray = 0;
    }

    UDA_LOG(UDA_LOG_DEBUG, "getting the command\n");
    UDA_LOG(UDA_LOG_DEBUG, "TOP_collections_parameters: %s\n", TOP_collections_parameters);

    int status = getCommand(searchedArray, &command, TOP_collections_parameters);

    if (status != 0) {
        int err = 901;
        UDA_LOG(UDA_LOG_DEBUG, "Unable to get command\n");
        addIdamError(CODEERRORTYPE, "Unable to get command", err, "");
    }

    UDA_LOG(UDA_LOG_DEBUG, "command: %s\n", command);
    UDA_LOG(UDA_LOG_DEBUG, "Getting object name\n");
    getObjectName(&objectName, command);

    UDA_LOG(UDA_LOG_DEBUG, "Group of signals ?\n");
    getSignalType(objectName, shotNumber, &signalType);

    if (signalType == 2) { //signal is a group of signals, so we append extraction chars to signal name
        UDA_LOG(UDA_LOG_DEBUG, "Signal belongs to a group of signals\n");
        char result[50];
        addExtractionChars(result, objectName,
                           searchedArrayIndex +
                           1); //Concatenate signalName avec %(searchedArrayIndex + 1), example: %1, %2, ...
        objectName = strdup(result);
    } else {
        UDA_LOG(UDA_LOG_DEBUG, "Signal does not belong to a group of signals\n");
    }

    UDA_LOG(UDA_LOG_DEBUG, "Object name: %s\n", objectName);

    int rang[2] = { 0, 0 };
    status = readSignal(objectName, shotNumber, 0, rang, time, data, len);

    UDA_LOG(UDA_LOG_DEBUG, "End of reading signal\n");

//	UDA_LOG(UDA_LOG_DEBUG, "%s\n", "First time values...");
//	int j;
//	for (j=0; j <10; j++) {
//		UDA_LOG(UDA_LOG_DEBUG, "time : %f\n", *time[j]);
//	}

    if (len == 0) {
        int err = 901;
        addIdamError(CODEERRORTYPE, "Dynamic data empty from WEST !", err, "");
        status = -1;
    } else {
        status = 0;
    }

    free(command);
    free(objectName);
    free(extractionsCount);

    return status;
}

void SetDynamicData(DATA_BLOCK* data_block, int len, float* time, float* data)
{
    SetDynData(data_block, len, time, data, 0);
}

void SetDynamicDataTime(DATA_BLOCK* data_block, int len, float* time, float* data)
{
    SetDynData(data_block, len, time, data, 1);
}

void SetDynData(DATA_BLOCK* data_block, int len, float* time, float* data, int setTime)
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
        data_block->data = (char*)time;
    }

    data_block->dims = (DIMS*)malloc(data_block->rank * sizeof(DIMS));

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
}

