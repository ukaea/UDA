#include "west_dynamic_data.h"

#include <string.h>
#include <stdio.h>
#include <assert.h>

#include <logging/logging.h>
#include <clientserver/initStructs.h>
#include <clientserver/errorLog.h>
#include <clientserver/stringUtils.h>
#include <clientserver/udaTypes.h>

#include "west_ece.h"
#include "west_pf_passive.h"
#include "west_utilities.h"
#include "west_dyn_data_utilities.h"

int GetDynamicData(int shotNumber, const char* mapfun, DATA_BLOCK* data_block, int* nodeIndices)
{

    UDA_LOG(UDA_LOG_DEBUG, "Entering GetDynamicData() -- WEST plugin\n");

    assert(mapfun); //Mandatory function to get WEST data

    char* fun_name = NULL; //Shape_of, tsmat_collect, tsbase
    char* TOP_collections_parameters = NULL; //example : TOP_collections_parameters = DMAG:GMAG_BNORM:PosR, DMAG:GMAG_BTANG:PosR, ...
    char* attributes = NULL; //example: attributes = 1:float:#1 (rank = 1, type = float, #1 = second IDAM index)
    char* normalizationAttributes = NULL; //example : multiply:cste:3     (multiply value by constant factor equals to 3)

    getFunName(mapfun, &fun_name);

    UDA_LOG(UDA_LOG_DEBUG, "request: %s\n", fun_name);

    if (strcmp(fun_name, "tsbase_collect") == 0) {
        tokenizeFunParameters(mapfun, &TOP_collections_parameters, &attributes, &normalizationAttributes);
        SetNormalizedDynamicData(shotNumber, data_block, nodeIndices, TOP_collections_parameters, attributes,
                                 normalizationAttributes);
    } else if (strcmp(fun_name, "tsbase_time") == 0) {
        tokenizeFunParameters(mapfun, &TOP_collections_parameters, &attributes, &normalizationAttributes);
        SetNormalizedDynamicDataTime(shotNumber, data_block, nodeIndices, TOP_collections_parameters, attributes,
                                     normalizationAttributes);
    } else if (strcmp(fun_name, "tsbase_collect_with_channels") == 0) {
        char* unvalid_channels = NULL; //used for interfero_polarimeter IDS, example : invalid_channels:1,2
        tokenizeFunParametersWithChannels(mapfun, &unvalid_channels, &TOP_collections_parameters, &attributes,
                                          &normalizationAttributes);
        SetNormalizedDynamicData(shotNumber, data_block, nodeIndices, TOP_collections_parameters, attributes,
                                 normalizationAttributes);
    } else if (strcmp(fun_name, "tsbase_time_with_channels") == 0) {
        char* unvalid_channels = NULL; //used for interfero_polarimeter IDS, example : invalid_channels:1,2
        tokenizeFunParametersWithChannels(mapfun, &unvalid_channels, &TOP_collections_parameters, &attributes,
                                          &normalizationAttributes);
        SetNormalizedDynamicDataTime(shotNumber, data_block, nodeIndices, TOP_collections_parameters, attributes,
                                     normalizationAttributes);
    } else if (strcmp(fun_name, "ece_t_e_data") == 0) {
        char* ece_mapfun = NULL;
        ece_t_e_data(shotNumber, &ece_mapfun);
        tokenizeFunParameters(ece_mapfun, &TOP_collections_parameters, &attributes, &normalizationAttributes);
        UDA_LOG(UDA_LOG_DEBUG, "TOP_collections_parameters : %s\n", TOP_collections_parameters);
        SetNormalizedDynamicData(shotNumber, data_block, nodeIndices, TOP_collections_parameters, attributes,
                                 normalizationAttributes);
        free(ece_mapfun);
    } else if (strcmp(fun_name, "ece_t_e_time") == 0) {
        char* ece_mapfun = NULL;
        ece_t_e_time(shotNumber, &ece_mapfun);
        tokenizeFunParameters(ece_mapfun, &TOP_collections_parameters, &attributes, &normalizationAttributes);
        UDA_LOG(UDA_LOG_DEBUG, "TOP_collections_parameters : %s\n", TOP_collections_parameters);
        SetNormalizedDynamicDataTime(shotNumber, data_block, nodeIndices, TOP_collections_parameters, attributes,
                                     normalizationAttributes);
        free(ece_mapfun);
    } else if (strcmp(fun_name, "ece_harmonic_data") == 0) {
        ece_harmonic_data(shotNumber, data_block, nodeIndices);
    } else if (strcmp(fun_name, "ece_harmonic_time") == 0) {
        ece_harmonic_time(shotNumber, data_block, nodeIndices);
    } else if (strcmp(fun_name, "ece_frequencies") == 0) {
        ece_frequencies(shotNumber, data_block, nodeIndices);
    } else if (strcmp(fun_name, "ece_frequencies_time") == 0) {
        ece_harmonic_time(shotNumber, data_block, nodeIndices); //TODO
    } else if (strcmp(fun_name, "passive_current") == 0) {
        passive_current(shotNumber, data_block, nodeIndices);
    } else if (strcmp(fun_name, "passive_time") == 0) {
        passive_time(shotNumber, data_block, nodeIndices);
    } else if (strcmp(fun_name, "test_fun") == 0) {
        test_fun(shotNumber, data_block, nodeIndices); //TODO
    }

    free(fun_name);
    free(TOP_collections_parameters);
    free(attributes);
    free(normalizationAttributes);

    return 0;

}

