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
#include "west_ece.h"
#include "west_pf_passive.h"
#include "west_pf_active.h"
#include "west_utilities.h"

char* setBuffer(int data_type, char* value);
void getShapeOf(const char* command, int shotNumber, int* nb_val);
void shape_of_tsmat_collect(int shotNumber, char* TOP_collections_parameters, DATA_BLOCK* data_block);

int execute(const char* mapfun, int shotNumber, DATA_BLOCK* data_block, int* nodeIndices);
void execute_tsmat_collect(const char* TOP_collections_parameters, char* attributes,
                           int shotNumber, DATA_BLOCK* data_block, int* nodeIndices,
                           char* normalizationAttributes);
void execute_tsmat_without_idam_index(const char* command, char* attributes,
                                      int shotNumber, DATA_BLOCK* data_block, char* normalizationAttributes);
void execute_setvalue_collect(const char* TOP_collections_parameters, char* attributes,
                              int shotNumber, DATA_BLOCK* data_block, int* nodeIndices, char* normalizationAttributes);
void execute_setchannels_validity(int* unvalid_channels_list, int unvalid_channels_size, char* attributes,
                                  int shotNumber, DATA_BLOCK* data_block, int* nodeIndices);
void setStatic1DValue(int data_type, DATA_BLOCK* data_block, char* value, int val_nb, float normalizationFactor);
void setStaticINTValue(int data_type, DATA_BLOCK* data_block, int value, float normalizationFactor);
void setStaticValue(int data_type, DATA_BLOCK* data_block, char* value, int requestedIndex, float normalizationFactor);

int GetStaticData(int shotNumber, const char* mapfun, DATA_BLOCK* data_block, int* nodeIndices)
{
    UDA_LOG(UDA_LOG_DEBUG, "Calling GetStaticData() from WEST plugin\n");

    //IDAM data block initialization
    initDataBlock(data_block);
    data_block->rank = 1; //we return always a 1D array
    data_block->dims = (DIMS*)malloc(data_block->rank * sizeof(DIMS));
    int i;
    for (i = 0; i < data_block->rank; i++) {
        initDimBlock(&data_block->dims[i]);
    }

    assert(mapfun); //Mandatory function to get WEST data

    UDA_LOG(UDA_LOG_DEBUG, "Calling execute() from WEST plugin\n");
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
    UDA_LOG(UDA_LOG_DEBUG, "fun_name: %s\n", fun_name);

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
    } else if (strcmp(fun_name, "set_channels_validity") == 0) {
        //returns a static value according to the position of the element in the collection (rank = 0)
        fun = 5;
    } /*else if (strcmp(fun_name, "ece_frequencies") == 0) {
		//set frequencies of ece channels: UDA request is ece/channel/#/frequency where # is the channel number
		fun = 6;
	} */
    else if (strcmp(fun_name, "ece_names") == 0) {
        //set names of ece channels: UDA request is ece/channel/#/name where # is the channel number
        fun = 7;
    } else if (strcmp(fun_name, "ece_identifiers") == 0) {
        //set identifiers of ece channels: UDA request is ece/channel/#/identifier where # is the channel number
        fun = 8;
    } else if (strcmp(fun_name, "ece_t_e_data_shape_of") == 0) {
        fun = 9;
    } else if (strcmp(fun_name, "passive_current_shapeOf") == 0) {
        fun = 10;
    } else if (strcmp(fun_name, "passive_r") == 0) {
        fun = 11;
    } else if (strcmp(fun_name, "passive_z") == 0) {
        fun = 12;
    } else if (strcmp(fun_name, "passive_name") == 0) {
        fun = 13;
    } else if (strcmp(fun_name, "pf_active_elements_shapeOf") == 0) {
        fun = 14;
    } else if (strcmp(fun_name, "pf_active_coils_shapeOf") == 0) {
        fun = 15;
    } else if (strcmp(fun_name, "pf_active_R") == 0) {
        fun = 16;
    } else if (strcmp(fun_name, "pf_active_Z") == 0) {
        fun = 17;
    } else if (strcmp(fun_name, "pf_active_element_identifier") == 0) {
        fun = 18;
    } else if (strcmp(fun_name, "pf_active_element_name") == 0) {
        fun = 19;
    } else if (strcmp(fun_name, "pf_active_coil_identifier") == 0) {
        fun = 20;
    } else if (strcmp(fun_name, "pf_active_coil_name") == 0) {
        fun = 21;
    } else if (strcmp(fun_name, "pf_active_turns") == 0) {
        fun = 22;
    } else if (strcmp(fun_name, "pf_active_H") == 0) {
        fun = 23;
    } else if (strcmp(fun_name, "pf_active_W") == 0) {
        fun = 24;
    }

    printNum("Case : ", fun);

    switch (fun) {
        case 0: {
            UDA_LOG(UDA_LOG_DEBUG, "Case of tsmat_collect from WEST plugin\n");
            UDA_LOG(UDA_LOG_DEBUG, "Calling tokenizeFunParameters() from WEST plugin\n");
            tokenizeFunParameters(mapfun, &TOP_collections_parameters, &attributes, &normalizationAttributes);
            execute_tsmat_collect(TOP_collections_parameters, attributes, shotNumber, data_block, nodeIndices,
                                  normalizationAttributes);

            break;
        }

        case 1: {

            UDA_LOG(UDA_LOG_DEBUG, "Case of shape_of_tsmat_collect from WEST plugin\n");

            UDA_LOG(UDA_LOG_DEBUG, "Calling tokenizeFunParameters() from WEST plugin\n");
            tokenizeFunParameters(mapfun, &TOP_collections_parameters, &attributes, &normalizationAttributes);
            shape_of_tsmat_collect(shotNumber, TOP_collections_parameters, data_block);

            break;
        }

        case 2: {
            UDA_LOG(UDA_LOG_DEBUG, "Case of set_value from WEST plugin\n");

            UDA_LOG(UDA_LOG_DEBUG, "Calling tokenizeFunParameters() from WEST plugin\n");
            char* value = NULL;
            tokenizeFunParameters(mapfun, &value, &attributes, &normalizationAttributes);

            int data_type;
            getReturnType(attributes, &data_type);
            char* buffer = setBuffer(data_type, value);

            UDA_LOG(UDA_LOG_DEBUG, "Calling setStaticValue()\n");
            float normalizationFactor = 1;
            getNormalizationFactor(&normalizationFactor, normalizationAttributes);
            setStaticValue(data_type, data_block, buffer, 0, normalizationFactor); //index is always 0 in this case
            UDA_LOG(UDA_LOG_DEBUG, "Returning from setStaticValue()\n");

            free(value);
            free(buffer);
            break;
        }

        case 3: {
            UDA_LOG(UDA_LOG_DEBUG, "Case of tsmat from WEST plugin\n");
            UDA_LOG(UDA_LOG_DEBUG, "Calling tokenizeFunParameters() from WEST plugin\n");
            tokenizeFunParameters(mapfun, &TOP_collections_parameters, &attributes, &normalizationAttributes);
            execute_tsmat_without_idam_index(TOP_collections_parameters, attributes, shotNumber, data_block,
                                             normalizationAttributes);
            break;
        }

        case 4: {
            UDA_LOG(UDA_LOG_DEBUG, "Case of set_value_collect from WEST plugin\n");
            UDA_LOG(UDA_LOG_DEBUG, "Calling tokenizeFunParameters() from WEST plugin\n");
            tokenizeFunParameters(mapfun, &TOP_collections_parameters, &attributes, &normalizationAttributes);
            UDA_LOG(UDA_LOG_DEBUG, "attributes: %s\n", attributes);
            execute_setvalue_collect(TOP_collections_parameters, attributes, shotNumber, data_block, nodeIndices,
                                     normalizationAttributes);
            break;
        }

        case 5: {
            UDA_LOG(UDA_LOG_DEBUG, "Case of set_channels_validity from WEST plugin\n");
            char* unvalid_channels_request = NULL; //used for interfero_polarimeter IDS, example : invalid_channels:2:1,2
            int unvalid_channels_size;
            int* unvalid_channels_list = NULL;
            UDA_LOG(UDA_LOG_DEBUG, "Calling tokenize_set_channels_validity() from WEST plugin\n");
            tokenize_set_channels_validity(mapfun, &unvalid_channels_request, &attributes);

            getUnvalidChannelsSize(unvalid_channels_request, &unvalid_channels_size);
            unvalid_channels_list = (int*)malloc(sizeof(int) * unvalid_channels_size);

            getUnvalidChannels(unvalid_channels_request, unvalid_channels_list);
            execute_setchannels_validity(unvalid_channels_list, unvalid_channels_size, attributes, shotNumber,
                                         data_block, nodeIndices);

            free(unvalid_channels_request);
            free(unvalid_channels_list);

            break;
        }

            /*case 6: {
                UDA_LOG(LOG_DEBUG, "Case of ece_frequencies from WEST plugin\n");
                ece_frequencies(shotNumber, data_block, nodeIndices);

                break;
            }*/

        case 7: {
            UDA_LOG(UDA_LOG_DEBUG, "Case of ece_names from WEST plugin\n");
            ece_names(shotNumber, data_block, nodeIndices);
            break;
        }

        case 8: {
            UDA_LOG(UDA_LOG_DEBUG, "Case of ece_identifiers from WEST plugin\n");
            ece_identifiers(shotNumber, data_block, nodeIndices);
            break;
        }

        case 9: {
            UDA_LOG(UDA_LOG_DEBUG, "Case of ece_t_e_data_shape_of from WEST plugin\n");
            char* ece_mapfun = NULL;
            ece_t_e_data_shape_of(shotNumber, &ece_mapfun);
            UDA_LOG(UDA_LOG_DEBUG, "Calling tokenizeFunParameters() from WEST plugin\n");
            tokenizeFunParameters(ece_mapfun, &TOP_collections_parameters, &attributes, &normalizationAttributes);
            shape_of_tsmat_collect(shotNumber, TOP_collections_parameters, data_block);
            break;
        }

        case 10: {
            UDA_LOG(UDA_LOG_DEBUG, "Case of passive_current_shapeOf from WEST plugin\n");
            passive_current_shapeOf(shotNumber, data_block, nodeIndices);
            break;
        }

        case 11: {
            UDA_LOG(UDA_LOG_DEBUG, "Case of passive_r from WEST plugin\n");
            passive_r(shotNumber, data_block, nodeIndices);
            break;
        }

        case 12: {
            UDA_LOG(UDA_LOG_DEBUG, "Case of passive_z from WEST plugin\n");
            passive_z(shotNumber, data_block, nodeIndices);
            break;
        }

        case 13: {
            UDA_LOG(UDA_LOG_DEBUG, "Case of passive_name from WEST plugin\n");
            passive_name(shotNumber, data_block, nodeIndices);
            break;
        }

        case 14: {
            UDA_LOG(UDA_LOG_DEBUG, "Case of pf_active_elements_shapeOf from WEST plugin\n");
            pf_active_elements_shapeOf(shotNumber, data_block, nodeIndices);
            break;
        }

        case 15: {
            UDA_LOG(UDA_LOG_DEBUG, "Case of pf_active_coils_shapeOf from WEST plugin\n");
            pf_active_coils_shapeOf(shotNumber, data_block, nodeIndices);
            break;
        }

        case 16: {
            UDA_LOG(UDA_LOG_DEBUG, "Case of pf_active_R from WEST plugin\n");
            pf_active_R(shotNumber, data_block, nodeIndices);
            break;
        }

        case 17: {
            UDA_LOG(UDA_LOG_DEBUG, "Case of pf_active_Z from WEST plugin\n");
            pf_active_Z(shotNumber, data_block, nodeIndices);
            break;
        }

        case 18: {
            UDA_LOG(UDA_LOG_DEBUG, "Case of pf_active_element_identifier from WEST plugin\n");
            pf_active_element_identifier(shotNumber, data_block, nodeIndices);
            break;
        }

        case 19: {
            UDA_LOG(UDA_LOG_DEBUG, "Case of pf_active_element_name from WEST plugin\n");
            pf_active_element_name(shotNumber, data_block, nodeIndices);
            break;
        }

        case 20: {
            UDA_LOG(UDA_LOG_DEBUG, "Case of pf_active_coil_identifier from WEST plugin\n");
            pf_active_coil_identifier(shotNumber, data_block, nodeIndices);
            break;
        }

        case 21: {
            UDA_LOG(UDA_LOG_DEBUG, "Case of pf_active_coil_name from WEST plugin\n");
            pf_active_coil_name(shotNumber, data_block, nodeIndices);
            break;
        }

        case 22: {
            UDA_LOG(UDA_LOG_DEBUG, "Case of pf_active_turns from WEST plugin\n");
            pf_active_turns(shotNumber, data_block, nodeIndices);
            break;
        }

        case 23: {
            UDA_LOG(UDA_LOG_DEBUG, "Case of pf_active_H from WEST plugin\n");
            pf_active_H(shotNumber, data_block, nodeIndices);
            break;
        }

        case 24: {
            UDA_LOG(UDA_LOG_DEBUG, "Case of pf_active_W from WEST plugin\n");
            pf_active_W(shotNumber, data_block, nodeIndices);
            break;
        }

    }

    free(fun_name);
    free(TOP_collections_parameters);
    free(attributes);
    free(normalizationAttributes);
    return 0;
}


void shape_of_tsmat_collect(int shotNumber, char* TOP_collections_parameters, DATA_BLOCK* data_block)
{
    //Get the number of parameters collections
    int collectionsCount;

    UDA_LOG(UDA_LOG_DEBUG, "Calling getTopCollectionsCount() from WEST plugin for shape_of_tsmat_collect case\n");
    getTopCollectionsCount(TOP_collections_parameters, &collectionsCount);

    printNum("Collections count : ", collectionsCount);

    UDA_LOG(UDA_LOG_DEBUG, "Calling tokenizeFunCollect() from WEST plugin\n");

    //Get the total size by adding all collections lengths
    int i;
    int parametersSize = 0;
    for (i = 0; i < collectionsCount; i++) {
        char* command = NULL;
        UDA_LOG(UDA_LOG_DEBUG, "Calling getCommand() from WEST plugin for shape_of_tsmat_collect case\n");
        getCommand(i, &command, TOP_collections_parameters); //get one command
        UDA_LOG(UDA_LOG_DEBUG, "Command : %s\n", command);
        int nb_val = 0;
        UDA_LOG(UDA_LOG_DEBUG, "Calling getShapeOf() from WEST plugin for shape_of_tsmat_collect case\n");
        getShapeOf(command, shotNumber, &nb_val);
        //printNum("nb_val : ", nb_val);
        UDA_LOG(UDA_LOG_DEBUG, "after getShapeOf\n");
        parametersSize += nb_val;
    }

    printNum("Parameters size : ", parametersSize);

    data_block->data_type = UDA_TYPE_INT;
    data_block->data = malloc(sizeof(int));
    *((int*)data_block->data) = parametersSize;
}

void getShapeOf(const char* command, int shotNumber, int* nb_val)
{

    char* prod_name = NULL; //DMAG, ...
    char* object_name = NULL;
    char* param_name = NULL;
    char* flag = NULL;

    UDA_LOG(UDA_LOG_DEBUG, "In getShapeOf, calling tokenizeCommand with command: %s\n", command);

    //Tokenize mapfun string to get function parameters, return type and arguments (#1, #2,...) to use
    tokenizeCommand(command, &prod_name, &object_name, &param_name, &flag);

    char* value = NULL;
    int val_nb = 1;
    //get the size of available data
    UDA_LOG(UDA_LOG_DEBUG, "DEBUG : prod_name: %s\n", prod_name);
    UDA_LOG(UDA_LOG_DEBUG, "DEBUG : object: %s\n", object_name);
    UDA_LOG(UDA_LOG_DEBUG, "DEBUG : param: %s\n", param_name);

    int status = readStaticParameters(&value, nb_val, shotNumber, prod_name, object_name, param_name, val_nb);
    UDA_LOG(UDA_LOG_DEBUG, "readStaticParameters status: %d\n", status);
    UDA_LOG(UDA_LOG_DEBUG, "readStaticParameters nb_val: %d\n", *nb_val);

    if (status != 0) {
        UDA_LOG(UDA_LOG_DEBUG, "Error calling readStaticParameters\n");
        int err = 901;
        addIdamError(CODEERRORTYPE, "Unable to get shape of static data from WEST", err, "");
    }

    free(prod_name);
    free(object_name);
    free(param_name);
}

void execute_setchannels_validity(int* unvalid_channels_list, int unvalid_channels_size, char* attributes,
                                  int shotNumber, DATA_BLOCK* data_block, int* nodeIndices)
{
    int requestedIndex = getNumIDAMIndex(attributes, nodeIndices);
    int channel_number = requestedIndex + 1; //TODO TO CHECK
    UDA_LOG(UDA_LOG_DEBUG, "channel number : %d\n", channel_number);

    if (isChannelValid(channel_number, unvalid_channels_list, unvalid_channels_size)) {
        UDA_LOG(UDA_LOG_DEBUG, "valid channel number : %d\n", channel_number);
        setStaticINTValue(UDA_TYPE_INT, data_block, 0, 1.0);
    } else {
        UDA_LOG(UDA_LOG_DEBUG, "unvalid channel number : %d\n", channel_number);
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
    int status = -1;
    for (i = 0; i < collectionsCount; i++) {
        char* command = NULL;
        status = getCommand(i, &command, TOP_collections_parameters);
        if (status == -1) {
            int err = 801;
            addIdamError(CODEERRORTYPE, "Unable to get the shapeof command", err, "");
        }

        int nb_val = 0;
        getShapeOf(command, shotNumber, &nb_val);
        l[i] = nb_val;
    }

    int requestedIndex = getNumIDAMIndex(attributes, nodeIndices);

    int searchedArray;
    int searchedArrayIndex;

    searchIndices(requestedIndex, l, &searchedArray, &searchedArrayIndex);
    UDA_LOG(UDA_LOG_DEBUG, "searchedArray : %d\n", searchedArray);
    UDA_LOG(UDA_LOG_DEBUG, "searchedArrayIndex : %d\n", searchedArrayIndex);

    char* command = NULL;
    getCommand(searchedArray, &command, TOP_collections_parameters);

    char* value;
    getValueCollect(command, &value, nodeIndices);
    UDA_LOG(UDA_LOG_DEBUG, "Command : %s\n", command);

    int data_type;
    getReturnType(attributes, &data_type);

    char* buffer = setBuffer(data_type, value);

    UDA_LOG(UDA_LOG_DEBUG, "Found value: %s\n", value);

    float normalizationFactor = 1;
    getNormalizationFactor(&normalizationFactor, normalizationAttributes);
    setStaticValue(data_type, data_block, buffer, 0, normalizationFactor);

    free(value);
    free(command);
    free(buffer);
}


char* setBuffer(int data_type, char* value)
{
    char* buffer = NULL;

    if (data_type == UDA_TYPE_FLOAT) {
        UDA_LOG(UDA_LOG_DEBUG, "UDA_TYPE_FLOAT requested from WEST plugin\n");
        buffer = malloc(sizeof(float));
        float* f_buf = (float*)buffer;
        *f_buf = atof(value);
        UDA_LOG(UDA_LOG_DEBUG, "Testing float value : %f\n", *f_buf);
    } else if (data_type == UDA_TYPE_DOUBLE) {
        UDA_LOG(UDA_LOG_DEBUG, "UDA_TYPE_DOUBLE requested from WEST plugin\n");
        buffer = malloc(sizeof(double));
        double* f_buf = (double*)buffer;
        *f_buf = atof(value);
    } else if (data_type == UDA_TYPE_INT) {
        UDA_LOG(UDA_LOG_DEBUG, "UDA_TYPE_INT requested from WEST plugin\n");
        buffer = malloc(sizeof(int));
        int* i_buf = (int*)buffer;
        *i_buf = atoi(value);
    } else if (data_type == UDA_TYPE_STRING) {
        UDA_LOG(UDA_LOG_DEBUG, "UDA_TYPE_STRING requested from WEST plugin\n");
        buffer = strdup(value);
    } else {
        int err = 999;
        UDA_LOG(UDA_LOG_DEBUG, "Unsupported data type in setBuffer(): %d\n", data_type);
        addIdamError(CODEERRORTYPE, "Unsupported data type", err, "");
    }

    return buffer;
}

void execute_tsmat_collect(const char* TOP_collections_parameters, char* attributes,
                           int shotNumber, DATA_BLOCK* data_block, int* nodeIndices, char* normalizationAttributes)
{
    int collectionsCount;
    getTopCollectionsCount(TOP_collections_parameters, &collectionsCount);

    int* l;
    l = (int*)calloc(collectionsCount, sizeof(int));

    int i;
    int status = -1;
    for (i = 0; i < collectionsCount; i++) {
        char* command = NULL;
        status = getCommand(i, &command, TOP_collections_parameters);
        if (status == -1) {
            int err = 801;
            addIdamError(CODEERRORTYPE, "Unable to get the shapeof command", err, "");
        }

        int nb_val = 0;
        getShapeOf(command, shotNumber, &nb_val);
        l[i] = nb_val;
    }

    UDA_LOG(UDA_LOG_DEBUG, "In execute_tsmat_collect, searching requestedIndex... %s\n", "");
    int requestedIndex = getNumIDAMIndex(attributes, nodeIndices);

    UDA_LOG(UDA_LOG_DEBUG, "In execute_tsmat_collect, after searching requestedIndex --> %d\n", requestedIndex);

    int searchedArray;
    int searchedArrayIndex;

    UDA_LOG(UDA_LOG_DEBUG, "In execute_tsmat_collect, searching index array for requested index: %d\n",
              requestedIndex);
    searchIndices(requestedIndex, l, &searchedArray, &searchedArrayIndex);
    UDA_LOG(UDA_LOG_DEBUG, "In execute_tsmat_collect, searched array:%d\n", searchedArray);
    UDA_LOG(UDA_LOG_DEBUG, "In execute_tsmat_collect, searched array index:%d\n", searchedArrayIndex);

    char* command = NULL;

    UDA_LOG(UDA_LOG_DEBUG, "In execute_tsmat_collect, getting command from TOP_collections_parameters: %s\n",
              TOP_collections_parameters);
    getCommand(searchedArray, &command, TOP_collections_parameters);

    UDA_LOG(UDA_LOG_DEBUG, "In execute_tsmat_collect, after getting command...\n");

    char* prod_name = NULL;     //DMAG, ...
    char* object_name = NULL;   //GMAG_BNORM, ...
    char* param_name = NULL;    //PosR, ...
    char* flag = NULL;          //'Null' or blank

    int data_type;
    getReturnType(attributes, &data_type);

    //Tokenize mapfun string to get function parameters
    UDA_LOG(UDA_LOG_DEBUG, "In execute_tsmat_collect, tokenizing command... %s\n", command);
    tokenizeCommand(command, &prod_name, &object_name, &param_name, &flag);
    UDA_LOG(UDA_LOG_DEBUG, "In execute_tsmat_collect, afetr tokenizing command...\n");

    char* value = NULL;
    int val_nb = l[searchedArray];
    int nb_val;

    UDA_LOG(UDA_LOG_DEBUG, "In execute_tsmat_collect, flag: %s\n", flag);
    UDA_LOG(UDA_LOG_DEBUG, "In execute_tsmat_collect, checking if flag is Null...\n");

    if (flag != NULL && strncmp("Null", flag, 4) == 0) {
        UDA_LOG(UDA_LOG_DEBUG, "In execute_tsmat_collect, setting value for Null flag...\n");
        int data_type;
        getReturnType(attributes, &data_type);
        value = setBuffer(data_type, "0"); //we put zero for 'Null' flag
        searchedArrayIndex = 0;
    } else {
        //Reading static parameters using TSLib
        UDA_LOG(UDA_LOG_DEBUG, "In execute_tsmat_collect, reading static parameters for param. name: %s\n",
                  param_name);
        status = readStaticParameters(&value, &nb_val, shotNumber, prod_name, object_name, param_name, val_nb);
        if (status != 0) {
            int err = 901;
            addIdamError(CODEERRORTYPE, "Unable to read static data from WEST", err, "");
        }
    }

    float normalizationFactor = 1;
    getNormalizationFactor(&normalizationFactor, normalizationAttributes);
    UDA_LOG(UDA_LOG_DEBUG, "In execute_tsmat_collect, setting static value... %s\n", "");
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

    char* prod_name = NULL;     //DMAG, ...
    char* object_name = NULL;   //GMAG_BNORM, ...
    char* param_name = NULL;    //PosR, ..
    char* flag = NULL;

    int data_type;
    getReturnType(attributes, &data_type);

    //Tokenize mapfun string to get function parameters
    tokenizeCommand(command, &prod_name, &object_name, &param_name, &flag);

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
        UDA_LOG(UDA_LOG_DEBUG, "Unsupported rank from execute_tsmat_without_idam_index()\n");
        addIdamError(CODEERRORTYPE, "Unsupported data type", err, "");
    }

    free(command);
    free(prod_name);
    free(object_name);
    free(param_name);
    free(value);
}

//Cast the results returned by tsmat according to the type of the data and set IDAM data
void setStatic1DValue(int data_type, DATA_BLOCK* data_block, char* value, int val_nb, float normalizationFactor)
{
    if (data_type == UDA_TYPE_FLOAT) {
        UDA_LOG(UDA_LOG_DEBUG, "handling float 1D in setStatic1DValue()\n");

        char val_nb_str[10];
        sprintf(val_nb_str, "%d", val_nb);
        UDA_LOG(UDA_LOG_DEBUG, "val_nb : %s\n", val_nb_str);

        if (val_nb == 0) {
            int err = 901;
            addIdamError(CODEERRORTYPE, __func__, err, "val_nb is equals to 0 !");
        }

        data_block->data_type = UDA_TYPE_FLOAT;
        data_block->data = malloc(val_nb * sizeof(float));
        float* pt_float = (float*)value;
        multiplyFloat(pt_float, normalizationFactor, val_nb);
        UDA_LOG(UDA_LOG_DEBUG, "handling float 1D in setStatic1DValue21()\n");
        *((float*)data_block->data) = *pt_float;
        UDA_LOG(UDA_LOG_DEBUG, "handling float 1D in setStatic1DValue22()\n");
    } else if (data_type == UDA_TYPE_INT) {
        UDA_LOG(UDA_LOG_DEBUG, "handling int 1D in setStatic1DValue()\n");
        data_block->data_type = UDA_TYPE_INT;
        data_block->data = malloc(val_nb * sizeof(int));
        int* pt_int = (int*)value;
        multiplyInt(pt_int, normalizationFactor, val_nb);
        *((int*)data_block->data) = *pt_int;
        UDA_LOG(UDA_LOG_DEBUG, "handling int 1D in setStatic1DValue2()\n");
    } else {
        int err = 999;
        UDA_LOG(UDA_LOG_DEBUG, "Unsupported data type from setStatic1DValue()\n");
        addIdamError(CODEERRORTYPE, __func__, err, "Unsupported data type");
    }
}

void setStaticINTValue(int data_type, DATA_BLOCK* data_block, int value, float normalizationFactor)
{
    UDA_LOG(UDA_LOG_DEBUG, "handling in setStaticINTValue(): %d\n", value);
    data_block->data_type = UDA_TYPE_INT;
    data_block->data = malloc(1 * sizeof(int));
    ((int*)data_block->data)[0] = value * (int)normalizationFactor;
}

//Cast the results returned by tsmat according to the type of the data and set IDAM data
void setStaticValue(int data_type, DATA_BLOCK* data_block, char* value, int requestedIndex, float normalizationFactor)
{
    UDA_LOG(UDA_LOG_DEBUG, "Entering setStaticValue()\n");
    printNum("requested index : ", requestedIndex);
    if (data_type == UDA_TYPE_DOUBLE) {
        UDA_LOG(UDA_LOG_DEBUG, "handling double in setStaticValue()\n");
        data_block->data_type = UDA_TYPE_DOUBLE;
        data_block->data = malloc(1 * sizeof(double));
        double* pt_double = (double*)value;
        ((double*)data_block->data)[0] = pt_double[requestedIndex] * (double)normalizationFactor;

    } else if (data_type == UDA_TYPE_FLOAT) {
        UDA_LOG(UDA_LOG_DEBUG, "handling float in setStaticValue(): %d, %g\n", requestedIndex, normalizationFactor);

        data_block->data_type = UDA_TYPE_FLOAT;
        data_block->data = malloc(1 * sizeof(float));
        float* pt_float = (float*)value;

        UDA_LOG(UDA_LOG_DEBUG, "in setStaticValue(), requestedIndex:  %d\n", requestedIndex);
        UDA_LOG(UDA_LOG_DEBUG, "in setStaticValue(), normalizationFactor:  %f\n", normalizationFactor);
        UDA_LOG(UDA_LOG_DEBUG, "in setStaticValue(), pt_float[requestedIndex]:  %f\n", pt_float[requestedIndex]);

        UDA_LOG(UDA_LOG_DEBUG, "Floating value set to  %f\n", pt_float[requestedIndex] * normalizationFactor);
        ((float*)data_block->data)[0] = pt_float[requestedIndex] * normalizationFactor;

    } else if (data_type == UDA_TYPE_LONG) {
        UDA_LOG(UDA_LOG_DEBUG, "handling long in setStaticValue()\n");
        data_block->data_type = UDA_TYPE_LONG;
        data_block->data = malloc(1 * sizeof(long));
        long* pt_long = (long*)value;
        ((long*)data_block->data)[0] = pt_long[requestedIndex] * (long)normalizationFactor;

    } else if (data_type == UDA_TYPE_INT) {
        UDA_LOG(UDA_LOG_DEBUG, "handling int in setStaticValue()\n");
        data_block->data_type = UDA_TYPE_INT;
        data_block->data = malloc(1 * sizeof(int));
        int* pt_int = (int*)value;
        UDA_LOG(UDA_LOG_DEBUG, "handling in setStaticValue(): %d\n", *pt_int);
        ((int*)data_block->data)[0] = pt_int[requestedIndex] * (int)normalizationFactor;

    } else if (data_type == UDA_TYPE_SHORT) {
        UDA_LOG(UDA_LOG_DEBUG, "handling short in setStaticValue()\n");
        data_block->data_type = UDA_TYPE_SHORT;
        data_block->data = malloc(1 * sizeof(short));
        int* pt_short = (int*)value;
        ((short*)data_block->data)[0] = pt_short[requestedIndex] * (short)normalizationFactor;

    } else if (data_type == UDA_TYPE_STRING) {
        UDA_LOG(UDA_LOG_DEBUG, "handling string in setStaticValue()\n");
        data_block->data_type = UDA_TYPE_STRING;
        UDA_LOG(UDA_LOG_DEBUG, "setting value\n");
        data_block->data = strdup(value);
        UDA_LOG(UDA_LOG_DEBUG, "%s\n", data_block->data);

    } else {
        int err = 999;
        UDA_LOG(UDA_LOG_DEBUG, "Unsupported data type from setStaticValue()\n");
        addIdamError(CODEERRORTYPE, "Unsupported data type", err, "");
    }
}

