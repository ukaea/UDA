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

void printNum(const char* label, int i);
void RemoveSpaces(char* source);
static void getValueCollect(char* command, char** value);
static void multiplyFloat(float* p, float factor, int val_nb);
static void multiplyInt(int* p, float factor, int val_nb);

static void getFunName(const char* s, char** fun_name);
static void getNormalizationFactor(float* normalizationFactor, char* normalizationAttributes);
static int convertToInt(char* value);
static void
setStaticValue(int data_type, DATA_BLOCK* data_block, char* value, int requestedIndex, float normalizationFactor);
static void setStatic1DValue(int data_type, DATA_BLOCK* data_block, char* value, int val_nb, float normalizationFactor);
static void getRank(char* attributes, int* rank);
static void
tokenizeFunParameters(const char* s, char** argument_name, char** attributes, char** normalizationAttributes);

static void tokenizeCommand(const char* s, char** prod_name, char** obj_name, char** param_name);
static int getCommand(int i, char** command, const char* TOP_collections_parameters);
static int execute(const char* mapfun, int shotNumber, DATA_BLOCK* data_block, int* nodeIndices);
static void execute_tsmat_collect(const char* TOP_collections_parameters, char* attributes,
                                  int shotNumber, DATA_BLOCK* data_block, int* nodeIndices,
                                  char* normalizationAttributes);
static void execute_tsmat_without_idam_index(const char* command, char* attributes,
                                             int shotNumber, DATA_BLOCK* data_block, char* normalizationAttributes);
void execute_setvalue_collect(const char* TOP_collections_parameters, char* attributes,
                              int shotNumber, DATA_BLOCK* data_block, int* nodeIndices, char* normalizationAttributes);
static void getShapeOf(const char* command, int shotNumber, int* nb_val);

static int getNumIDAMIndex(char* attributes, int* nodeIndices);
static void getReturnType(char* attributes, int* dataType);
static void searchIndices(int requestedIndex, int* l, int* searchedArray, int* searchedArrayIndex);
static void addExtractionChars(char* result, char* signalName, int extractionIndex);
static void getTopCollectionsCount(const char* TOP_collections_parameters, int* collectionsCount);
static void getObjectName(char** obj_name, char* command);

int GetStaticData(int shotNumber, const char* mapfun, DATA_BLOCK* data_block, int* nodeIndices)
{
    idamLog(LOG_DEBUG, "Calling GetStaticData() from WEST plugin\n");

    //IDAM data block initialization
    initDataBlock(data_block);
    data_block->rank = 1; //we return always a 1D array
    data_block->dims = (DIMS*)malloc(data_block->rank * sizeof(DIMS));
    int i;
    for (i = 0; i < data_block->rank; i++) {
        initDimBlock(&data_block->dims[i]);
    }

    assert(mapfun); //Mandatory function to get WEST data

    idamLog(LOG_DEBUG, "Calling execute() from WEST plugin\n");
    int status = execute(mapfun, shotNumber, data_block, nodeIndices);

    if (status != 0) {
        addIdamError(&idamerrorstack, CODEERRORTYPE, "west : error while getting static data", -900, "");
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

    int fun = -1;

    if (STR_EQUALS(fun_name, "tsmat_collect")) {
        //returns a static parameter (rank = 0) from a collection of static data (Top objects).
        //Given the list of all static data in the collection, the element returned in the data_block is list(idam index)
        fun = 0;
    } else if (STR_EQUALS(fun_name, "shape_of_tsmat_collect")) {
        //Returns the list size of all static data in the collection

        fun = 1;
    } else if (STR_EQUALS(fun_name, "set_value")) {
        //returns a static value (rank = 0)
        fun = 2;
    } else if (STR_EQUALS(fun_name, "tsmat")) {
        //returns a static parameter (rank = 0 or 1) from a Top object
        fun = 3;
    } else if (STR_EQUALS(fun_name, "set_value_collect")) {
        //returns a static value according to the position of the element in the collection (rank = 0)
        fun = 4;
    }

    printNum("Case : ", fun);

    switch (fun) {
        case 0: {
            idamLog(LOG_DEBUG, "Case of tsmat_collect from WEST plugin\n");
            idamLog(LOG_DEBUG, "Calling tokenizeFunParameters() from WEST plugin\n");
            tokenizeFunParameters(mapfun, &TOP_collections_parameters, &attributes, &normalizationAttributes);
            execute_tsmat_collect(TOP_collections_parameters, attributes, shotNumber, data_block, nodeIndices,
                                  normalizationAttributes);

            break;
        }

        case 1: {

            idamLog(LOG_DEBUG, "Case of shape_of_tsmat_collect from WEST plugin\n");

            idamLog(LOG_DEBUG, "Calling tokenizeFunParameters() from WEST plugin\n");
            tokenizeFunParameters(mapfun, &TOP_collections_parameters, &attributes, &normalizationAttributes);

            //Get the number of parameters collections
            int collectionsCount;

            idamLog(LOG_DEBUG, "Calling getTopCollectionsCount() from WEST plugin for shape_of_tsmat_collect case\n");
            getTopCollectionsCount(TOP_collections_parameters, &collectionsCount);

            printNum("Collections count : ", collectionsCount);

            idamLog(LOG_DEBUG, "Calling tokenizeFunCollect() from WEST plugin\n");

            //Get the total size by adding all collections lengths
            int i;
            int parametersSize = 0;
            for (i = 0; i < collectionsCount; i++) {
                char* command = NULL;
                idamLog(LOG_DEBUG, "Calling getCommand() from WEST plugin for shape_of_tsmat_collect case\n");
                getCommand(i, &command, TOP_collections_parameters); //get one command
                idamLog(LOG_DEBUG, "Command : ");
                idamLog(LOG_DEBUG, command);
                idamLog(LOG_DEBUG, "\n");
                int nb_val = 0;
                idamLog(LOG_DEBUG, "Calling getShapeOf() from WEST plugin for shape_of_tsmat_collect case\n");
                getShapeOf(command, shotNumber, &nb_val);

                printNum("nb_val : ", nb_val);

                idamLog(LOG_DEBUG, "after getShapeOf\n");
                parametersSize += nb_val;
            }

            printNum("Parameters size : ", parametersSize);

            data_block->data_type = TYPE_INT;
            data_block->data = malloc(sizeof(int));
            *((int*)data_block->data) = parametersSize;

            break;
        }

        case 2: {
            idamLog(LOG_DEBUG, "Case of set_value from WEST plugin\n");

            idamLog(LOG_DEBUG, "Calling tokenizeFunParameters() from WEST plugin\n");
            char* value = NULL;
            tokenizeFunParameters(mapfun, &value, &attributes, &normalizationAttributes);

            int data_type;
            getReturnType(attributes, &data_type);
            char* buffer = malloc(1 * sizeof(char));

            if (data_type == TYPE_FLOAT) {
                idamLog(LOG_DEBUG, "TYPE_FLOAT requested from WEST plugin\n");
                float* f_buf = (float*)buffer;
                *f_buf = atof(value);
            } else if (data_type == TYPE_INT) {
                idamLog(LOG_DEBUG, "TYPE_INT requested from WEST plugin\n");
                int* i_buf = (int*)buffer;
                *i_buf = atoi(value);
            } else if (data_type == TYPE_STRING) {
                idamLog(LOG_DEBUG, "TYPE_STRING requested from WEST plugin\n");
                buffer = strdup(value);
            } else {
                int err = 999;
                idamLog(LOG_DEBUG, "WEST : Unsupported data type\n");
                addIdamError(&idamerrorstack, CODEERRORTYPE, "west : Unsupported data type", err, "");
            }

            idamLog(LOG_DEBUG, "Calling setStaticValue()\n");
            float normalizationFactor = 1;
            getNormalizationFactor(&normalizationFactor, normalizationAttributes);
            setStaticValue(data_type, data_block, buffer, 0, normalizationFactor); //index is always 0 in this case
            idamLog(LOG_DEBUG, "Returning from setStaticValue()\n");
            break;
        }

        case 3: {
            idamLog(LOG_DEBUG, "Case of tsmat from WEST plugin\n");
            idamLog(LOG_DEBUG, "Calling tokenizeFunParameters() from WEST plugin\n");
            tokenizeFunParameters(mapfun, &TOP_collections_parameters, &attributes, &normalizationAttributes);
            execute_tsmat_without_idam_index(TOP_collections_parameters, attributes, shotNumber, data_block,
                                             normalizationAttributes);
            break;
        }

        case 4: {
            idamLog(LOG_DEBUG, "Case of set_value_collect from WEST plugin\n");
            idamLog(LOG_DEBUG, "Calling tokenizeFunParameters() from WEST plugin\n");
            tokenizeFunParameters(mapfun, &TOP_collections_parameters, &attributes, &normalizationAttributes);
            execute_setvalue_collect(TOP_collections_parameters, attributes, shotNumber, data_block, nodeIndices,
                                     normalizationAttributes);
            break;
        }
    }
    return 0;
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

    char* command = NULL;
    getCommand(searchedArray, &command, TOP_collections_parameters);

    char* value;
    getValueCollect(command, &value);

    int data_type;
    getReturnType(attributes, &data_type);

    char* buffer = NULL;

    if (data_type == TYPE_FLOAT) {
        idamLog(LOG_DEBUG, "TYPE_FLOAT requested from WEST plugin\n");
        buffer = malloc(sizeof(float));
        float* f_buf = (float*)buffer;
        *f_buf = atof(value);
    } else if (data_type == TYPE_DOUBLE) {
        idamLog(LOG_DEBUG, "TYPE_DOUBLE requested from WEST plugin\n");
        buffer = malloc(sizeof(double));
        double* f_buf = (double*)buffer;
        *f_buf = atof(value);
    } else if (data_type == TYPE_INT) {
        idamLog(LOG_DEBUG, "TYPE_INT requested from WEST plugin\n");
        buffer = malloc(sizeof(int));
        int* i_buf = (int*)buffer;
        *i_buf = atoi(value);
    } else if (data_type == TYPE_STRING) {
        idamLog(LOG_DEBUG, "TYPE_STRING requested from WEST plugin\n");
        buffer = strdup(value);
    } else {
        int err = 999;
        idamLog(LOG_DEBUG, "WEST : Unsupported data type in execute_setvalue_collect\n");
        addIdamError(&idamerrorstack, CODEERRORTYPE, "west : Unsupported data type", err, "");
    }

    idamLog(LOG_DEBUG, "Found value: ");
    idamLog(LOG_DEBUG, value);
    idamLog(LOG_DEBUG, "\n");

    float normalizationFactor = 1;
    getNormalizationFactor(&normalizationFactor, normalizationAttributes);
    setStaticValue(data_type, data_block, buffer, 0, normalizationFactor);

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
        addIdamError(&idamerrorstack, CODEERRORTYPE, "west : Unable to read static data from WEST", err, "");
    }

    float normalizationFactor = 1;
    getNormalizationFactor(&normalizationFactor, normalizationAttributes);
    setStaticValue(data_type, data_block, value, searchedArrayIndex, normalizationFactor);
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
        addIdamError(&idamerrorstack, CODEERRORTYPE, "west : Unable to read static data from WEST", err, "");
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
        idamLog(LOG_DEBUG, "WEST : Unsupported rank from execute_tsmat_without_idam_index()\n");
        addIdamError(&idamerrorstack, CODEERRORTYPE, "west : Unsupported data type", err, "");
    }


}

int GetDynamicData(int shotNumber, const char* mapfun, DATA_BLOCK* data_block, int* nodeIndices)
{

    idamLog(LOG_DEBUG, "Entering GetDynamicData() -- WEST plugin\n");
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

    getFunName(mapfun, &fun_name);
    tokenizeFunParameters(mapfun, &TOP_collections_parameters, &attributes, &normalizationAttributes);

    idamLog(LOG_DEBUG, "Evaluating the request type (tsbase_collect or tsbase_time, ...)  -- WEST plugin\n");
    int fun;

    if (STR_EQUALS(fun_name, "tsbase_collect")) {
        idamLog(LOG_DEBUG, "tsbase_collect request \n");
        fun = 0;
    } else if (STR_EQUALS(fun_name, "tsbase_time")) {
        idamLog(LOG_DEBUG, "tsbase_time request \n");
        fun = 1;
    }

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

        idamLog(LOG_DEBUG, "Group of signals1 ?\n");
        getSignalType(objectName, shotNumber, &signalType);

        if (signalType == 2) {
            getExtractionsCount(objectName, shotNumber, occ, &nb_extractions);
        }

        printNum("Number of extractions : ", nb_extractions);

        extractionsCount[i] = nb_extractions;
    }

    idamLog(LOG_DEBUG, "searching for IDAM index\n");
    idamLog(LOG_DEBUG, "attributes : ");
    idamLog(LOG_DEBUG, attributes);
    idamLog(LOG_DEBUG, "\n");

    int requestedIndex = getNumIDAMIndex(attributes, nodeIndices);

    printNum("Requested index (from IDAM call) : ", requestedIndex);

    idamLog(LOG_DEBUG, "searching for the array according to the static index\n");
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

    idamLog(LOG_DEBUG, "getting the command\n");
    idamLog(LOG_DEBUG, "TOP_collections_parameters: ");
    idamLog(LOG_DEBUG, TOP_collections_parameters);
    idamLog(LOG_DEBUG, "\n");

    int status = getCommand(searchedArray, &command, TOP_collections_parameters);

    if (status != 0) {
        int err = 901;
        idamLog(LOG_DEBUG, "west : Unable to get command\n");
        addIdamError(&idamerrorstack, CODEERRORTYPE, "west : Unable to get command", err, "");
    }

    idamLog(LOG_DEBUG, "command: \n");
    idamLog(LOG_DEBUG, command);
    idamLog(LOG_DEBUG, "\n");
    idamLog(LOG_DEBUG, "Getting object name\n");
    getObjectName(&objectName, command);

    idamLog(LOG_DEBUG, "Group of signals ?\n");
    getSignalType(objectName, shotNumber, &signalType);

    if (signalType == 2) { //signal is a group of signals, so we append extraction chars to signal name
        idamLog(LOG_DEBUG, "Signal belongs to a group of signals\n");
        char result[50];
        addExtractionChars(result, objectName,
                           searchedArrayIndex + 1); //Concatenate signalName avec %searchedArrayIndex + 1
        objectName = strdup(result);
    } else {
        idamLog(LOG_DEBUG, "Signal does not belong to a group of signals\n");
    }

    idamLog(LOG_DEBUG, "Object name: \n");
    idamLog(LOG_DEBUG, objectName);
    idamLog(LOG_DEBUG, "\n");

    float* time = NULL;
    float* data = NULL;
    int len;

    int rang[2] = { 0, 0 };
    status = readSignal(objectName, shotNumber, 0, rang, &time, &data, &len);

    idamLog(LOG_DEBUG, "End of reading signal\n");

    /*if (status != 0) {
            int err = 901;
            addIdamError(&idamerrorstack, CODEERRORTYPE, "west : Unable to read dynamic data from WEST !", err, "");
        }*/

    idamLog(LOG_DEBUG, "After error handling\n");

    idamLog(LOG_DEBUG, "Getting normalization factor, if any\n");
    float normalizationFactor = 1;
    getNormalizationFactor(&normalizationFactor, normalizationAttributes);
    idamLog(LOG_DEBUG, "Starting data normalization\n");
    multiplyFloat(data, normalizationFactor, len);
    idamLog(LOG_DEBUG, "end of data normalization, if any\n");

    free(data_block->dims);

    data_block->rank = 1;
    data_block->data_type = TYPE_FLOAT;
    data_block->data_n = len;
    if (fun == 0) {
        data_block->data = (char*)data;
    } else {
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

    return 0;
}

void getShapeOf(const char* command, int shotNumber, int* nb_val)
{

    char* prod_name = NULL; //DMAG, ...
    char* object_name = NULL;
    char* param_name = NULL;

    idamLog(LOG_DEBUG, "Calling tokenizeCommand with command : \n");
    idamLog(LOG_DEBUG, command);
    idamLog(LOG_DEBUG, "\n");
    //Tokenize mapfun string to get function parameters, return type and arguments (#1, #2,...) to use
    tokenizeCommand(command, &prod_name, &object_name, &param_name);

    char* value = NULL;
    int val_nb = 1;
    //get the size of available data
    int status = readStaticParameters(&value, nb_val, shotNumber, prod_name, object_name, param_name, val_nb);
    printNum("status : ", status);

    if (status != 0) {
        idamLog(LOG_DEBUG, "Error calling readStaticParameters\n");
        int err = 901;
        addIdamError(&idamerrorstack, CODEERRORTYPE, "west : Unable to get shape of static data from WEST", err, "");
    }
}

//Cast the results returned by tsmat according to the type of the data and set IDAM data
void setStatic1DValue(int data_type, DATA_BLOCK* data_block, char* value, int val_nb, float normalizationFactor)
{
    if (data_type == TYPE_FLOAT) {
        idamLog(LOG_DEBUG, "handling float 1D in setStatic1DValue()\n");

        char val_nb_str[10];
        sprintf(val_nb_str, "%d", val_nb);
        idamLog(LOG_DEBUG, "val_nb : ");
        idamLog(LOG_DEBUG, val_nb_str);
        idamLog(LOG_DEBUG, "\n");

        if (val_nb == 0) {
            int err = 901;
            addIdamError(&idamerrorstack, CODEERRORTYPE, "west : val_nb is equals to 0 !", err, "");
        }

        data_block->data_type = TYPE_FLOAT;
        data_block->data = malloc(val_nb * sizeof(float));
        float* pt_float = (float*)value;
        multiplyFloat(pt_float, normalizationFactor, val_nb);
        idamLog(LOG_DEBUG, "handling float 1D in setStatic1DValue21()\n");
        *((float*)data_block->data) = *pt_float;
        idamLog(LOG_DEBUG, "handling float 1D in setStatic1DValue22()\n");
    } else if (data_type == TYPE_INT) {
        idamLog(LOG_DEBUG, "handling int 1D in setStatic1DValue()\n");
        data_block->data_type = TYPE_INT;
        data_block->data = malloc(val_nb * sizeof(int));
        int* pt_int = (int*)value;
        multiplyInt(pt_int, normalizationFactor, val_nb);
        *((int*)data_block->data) = *pt_int;
        idamLog(LOG_DEBUG, "handling int 1D in setStatic1DValue2()\n");
    } else {
        int err = 999;
        idamLog(LOG_DEBUG, "WEST : Unsupported data type from setStatic1DValue()\n");
        addIdamError(&idamerrorstack, CODEERRORTYPE, "west : Unsupported data type", err, "");
    }
}

//Cast the results returned by tsmat according to the type of the data and set IDAM data
void setStaticValue(int data_type, DATA_BLOCK* data_block, char* value, int requestedIndex, float normalizationFactor)
{
    idamLog(LOG_DEBUG, "Entering setStaticValue()\n");
    printNum("requested index : ", requestedIndex);
    if (data_type == TYPE_DOUBLE) {
        idamLog(LOG_DEBUG, "handling double in setStaticValue()\n");
        data_block->data_type = TYPE_DOUBLE;
        data_block->data = malloc(1 * sizeof(double));
        double* pt_double = (double*)value;
        ((double*)data_block->data)[0] = pt_double[requestedIndex] * normalizationFactor;

    } else if (data_type == TYPE_FLOAT) {
        idamLog(LOG_DEBUG, "handling float in setStaticValue(): %d, %g\n", requestedIndex, normalizationFactor);
        data_block->data_type = TYPE_FLOAT;
        data_block->data = malloc(1 * sizeof(float));
        float* pt_float = (float*)value;
        ((float*)data_block->data)[0] = pt_float[requestedIndex] * normalizationFactor;

    } else if (data_type == TYPE_LONG) {
        idamLog(LOG_DEBUG, "handling long in setStaticValue()\n");
        data_block->data_type = TYPE_LONG;
        data_block->data = malloc(1 * sizeof(long));
        long* pt_long = (long*)value;
        ((long*)data_block->data)[0] = pt_long[requestedIndex] * normalizationFactor;

    } else if (data_type == TYPE_INT) {
        idamLog(LOG_DEBUG, "handling int in setStaticValue()\n");
        data_block->data_type = TYPE_INT;
        data_block->data = malloc(1 * sizeof(int));
        int* pt_int = (int*)value;
        ((int*)data_block->data)[0] = pt_int[requestedIndex] * normalizationFactor;

    } else if (data_type == TYPE_SHORT) {
        idamLog(LOG_DEBUG, "handling short in setStaticValue()\n");
        data_block->data_type = TYPE_SHORT;
        data_block->data = malloc(1 * sizeof(short));
        int* pt_short = (int*)value;
        ((short*)data_block->data)[0] = pt_short[requestedIndex] * normalizationFactor;

    } else if (data_type == TYPE_STRING) {
        idamLog(LOG_DEBUG, "handling string in setStaticValue()\n");
        data_block->data_type = TYPE_STRING;
        idamLog(LOG_DEBUG, "setting value\n");
        data_block->data = strdup(value);
        idamLog(LOG_DEBUG, data_block->data);
        idamLog(LOG_DEBUG, "\n");

    } else {
        int err = 999;
        idamLog(LOG_DEBUG, "WEST : Unsupported data type from setStaticValue()\n");
        addIdamError(&idamerrorstack, CODEERRORTYPE, "west : Unsupported data type", err, "");
    }
}


int convertToInt(char* value)
{
    int i = TYPE_UNKNOWN;
    int err = 0;

    if (STR_EQUALS(value, "vecstring_type") || STR_EQUALS(value, "string") || STR_EQUALS(value, "STR_0D")) {
        i = TYPE_STRING;
    } else if (STR_EQUALS(value, "vecflt_type") || STR_EQUALS(value, "float") || STR_EQUALS(value, "FLT_0D")) {
        i = TYPE_FLOAT;
    } /*else if (STR_EQUALS(value, "double")) {
		i = TYPE_DOUBLE;
	}*/else if (STR_EQUALS(value, "vecint_type") || STR_EQUALS(value, "integer") || STR_EQUALS(value, "int") ||
                STR_EQUALS(value, "INT_0D")) {
        i = TYPE_INT;
    } else {
        err = 999;
        addIdamError(&idamerrorstack, CODEERRORTYPE, "west convertToInt() : Unsupported data type", err, "");
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

void tokenizeFunParameters(const char* s, char** value, char** attributes, char** normalizationAttributes)
{
    const char delim[] = ";";
    char* s_copy = strdup(s);
    strdup(strtok(s_copy, delim)); //function name
    *value = strdup(strtok(NULL, delim));
    RemoveSpaces(*value);
    *attributes = strdup(strtok(NULL, delim));
    RemoveSpaces(*attributes);
    char* token = strtok(NULL, delim);
    if (token != NULL) {
        *normalizationAttributes = strdup(token);
        RemoveSpaces(*normalizationAttributes);
    }
    free(s_copy);
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
            return 0;
        }
        token = strtok(NULL, delim);
        j++;
    }
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
}

void getNormalizationFactor(float* normalizationFactor, char* normalizationAttributes)
{
    if (normalizationAttributes != NULL) {
        idamLog(LOG_DEBUG, "normalization attributes found\n");
        const char delim[] = ":";
        char* s_copy = strdup(normalizationAttributes);
        char* operation = NULL;
        operation = strdup(strtok(s_copy, delim));
        if (STR_EQUALS("multiply", operation)) {
            idamLog(LOG_DEBUG, "Multiply operation\n");
            char* funname = NULL;
            funname = strdup(strtok(NULL, delim));
            if (STR_EQUALS("cste", funname)) {
                idamLog(LOG_DEBUG, "WEST : multiply data by constant value\n");
                char* csteStr = NULL;
                csteStr = strdup(strtok(NULL, delim));
                idamLog(LOG_DEBUG, csteStr);
                idamLog(LOG_DEBUG, "\n");
                *normalizationFactor = atof(csteStr);
            } else {
                int err = 999;
                idamLog(LOG_DEBUG, "WEST : Unsupported operand for 'multiply' operation\n");
                addIdamError(&idamerrorstack, CODEERRORTYPE, "west : Unsupported operand for 'multiply' operation", err,
                             "");
            }

        } else {
            int err = 999;
            idamLog(LOG_DEBUG, "WEST : Unsupported operation to apply\n");
            addIdamError(&idamerrorstack, CODEERRORTYPE, "west : Unsupported operation to apply", err, "");
        }
    } else {
        idamLog(LOG_DEBUG, "no normalization attributes found\n");
    }
}

void multiplyFloat(float* p, float factor, int val_nb)
{
    idamLog(LOG_DEBUG, "Entering multiplyFloat...\n");
    if (factor != 1) {
        int i;
        idamLog(LOG_DEBUG, "\n");
        idamLog(LOG_DEBUG, "Inside multiplyFloat...\n");
        for (i = 0; i < val_nb; i++) {
            idamLog(LOG_DEBUG, "OK");
            p[i] *= factor;
            idamLog(LOG_DEBUG, "\n");
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

int getNumIDAMIndex(char* attributes, int* nodeIndices)
{

    char* s_copy = strdup(attributes);
    const char delim[] = ":";
    char* charIDAMIndex = NULL;
    char* symbol;
    symbol = (char*)malloc(2 * sizeof(char));
    strtok(s_copy, delim); //rank
    strtok(NULL, delim); //type
    charIDAMIndex = strdup(strtok(NULL, delim));
    symbol[0] = charIDAMIndex[0];
    symbol[1] = '\0';

    char* firstChar = &charIDAMIndex[0];

    if (STR_EQUALS(symbol, "#")) {
        idamLog(LOG_DEBUG, "index specified in IDAM request\n");
        return nodeIndices[atoi(&charIDAMIndex[1])] - 1;
    } else {
        idamLog(LOG_DEBUG, "idam index hard coded in mapping file\n");
        return atoi(firstChar) - 1;
    }

}

void getReturnType(char* attributes, int* dataType)
{
    char* s_copy = strdup(attributes);
    const char delim[] = ":";
    strtok(s_copy, delim); //the rank
    *dataType = convertToInt(strdup(strtok(NULL, delim)));
}

void getRank(char* attributes, int* rank)
{
    char* s_copy = strdup(attributes);
    const char delim[] = ":";
    char* rankStr = strdup(strtok(s_copy, delim));
    *rank = atoi(&rankStr[0]);
}

void getValueCollect(char* command, char** value)
{
    char* s_copy = strdup(command);
    const char delim[] = ":";
    strtok(s_copy, delim); //prod_name
    strtok(NULL, delim); //object name
    strtok(NULL, delim); //param name
    *value = strdup(strtok(NULL, delim)); //value for the setvalue_collect function
}

void tokenizeCommand(const char* s, char** prod_name, char** obj_name, char** param_name)
{
    char* s_copy = strdup(s);
    const char delim[] = ":";
    idamLog(LOG_DEBUG, "Tokenizing : ");
    idamLog(LOG_DEBUG, s);
    idamLog(LOG_DEBUG, "\n");
    idamLog(LOG_DEBUG, s_copy);
    idamLog(LOG_DEBUG, "\n");
    *prod_name = strdup(strtok(s_copy, delim));
    RemoveSpaces(*prod_name);
    idamLog(LOG_DEBUG, *prod_name);
    idamLog(LOG_DEBUG, "\n");
    *obj_name = strdup(strtok(NULL, delim));
    RemoveSpaces(*obj_name);
    idamLog(LOG_DEBUG, *obj_name);
    idamLog(LOG_DEBUG, "\n");
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
    char str[10];
    sprintf(str, "%d", i);
    idamLog(LOG_DEBUG, label);
    idamLog(LOG_DEBUG, str);
    idamLog(LOG_DEBUG, "\n");
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

