#include "udaPlugin.h"

#include <clientserver/makeRequestBlock.h>
#include <clientserver/stringUtils.h>
#include <clientserver/initStructs.h>
#include <clientserver/udaTypes.h>
#include <server/serverPlugin.h>

int initPlugin(const IDAM_PLUGIN_INTERFACE* plugin_interface)
{
    udaSetLogLevel((LOG_LEVEL)plugin_interface->environment->loglevel);
    return 0;
}

int setReturnDataFloatArray(DATA_BLOCK* data_block, float* values, size_t rank, const size_t* shape,
                            const char* description)
{
    initDataBlock(data_block);

    if (description != nullptr) {
        strncpy(data_block->data_desc, description, STRING_LENGTH);
        data_block->data_desc[STRING_LENGTH - 1] = '\0';
    }

    data_block->rank = (int)rank;
    data_block->dims = (DIMS*)malloc(rank * sizeof(DIMS));

    size_t len = 1;

    for (size_t i = 0; i < rank; ++i) {
        initDimBlock(&data_block->dims[i]);

        data_block->dims[i].data_type = UDA_TYPE_UNSIGNED_INT;
        data_block->dims[i].dim_n = (int)shape[i];
        data_block->dims[i].compressed = 1;
        data_block->dims[i].dim0 = 0.0;
        data_block->dims[i].diff = 1.0;
        data_block->dims[i].method = 0;

        len *= shape[i];
    }

    auto data = (float*)malloc(len * sizeof(float));
    memcpy(data, values, len * sizeof(float));

    data_block->data_type = UDA_TYPE_FLOAT;
    data_block->data = (char*)data;
    data_block->data_n = (int)len;

    return 0;
}

int setReturnDataDoubleArray(DATA_BLOCK* data_block, double* values, size_t rank, const size_t* shape,
                             const char* description)
{
    initDataBlock(data_block);

    if (description != nullptr) {
        strncpy(data_block->data_desc, description, STRING_LENGTH);
        data_block->data_desc[STRING_LENGTH - 1] = '\0';
    }

    data_block->rank = (int)rank;
    data_block->dims = (DIMS*)malloc(rank * sizeof(DIMS));

    size_t len = 1;

    for (size_t i = 0; i < rank; ++i) {
        initDimBlock(&data_block->dims[i]);

        data_block->dims[i].data_type = UDA_TYPE_UNSIGNED_INT;
        data_block->dims[i].dim_n = (int)shape[i];
        data_block->dims[i].compressed = 1;
        data_block->dims[i].dim0 = 0.0;
        data_block->dims[i].diff = 1.0;
        data_block->dims[i].method = 0;

        len *= shape[i];
    }

    auto data = (double*)malloc(len * sizeof(double));
    memcpy(data, values, len * sizeof(double));

    data_block->data_type = UDA_TYPE_DOUBLE;
    data_block->data = (char*)data;
    data_block->data_n = (int)len;

    return 0;
}

int
setReturnDataIntArray(DATA_BLOCK* data_block, int* values, size_t rank, const size_t* shape, const char* description)
{
    initDataBlock(data_block);

    if (description != nullptr) {
        strncpy(data_block->data_desc, description, STRING_LENGTH);
        data_block->data_desc[STRING_LENGTH - 1] = '\0';
    }

    data_block->rank = (int)rank;
    data_block->dims = (DIMS*)malloc(rank * sizeof(DIMS));

    size_t len = 1;

    for (size_t i = 0; i < rank; ++i) {
        initDimBlock(&data_block->dims[i]);

        data_block->dims[i].data_type = UDA_TYPE_UNSIGNED_INT;
        data_block->dims[i].dim_n = (int)shape[i];
        data_block->dims[i].compressed = 1;
        data_block->dims[i].dim0 = 0.0;
        data_block->dims[i].diff = 1.0;
        data_block->dims[i].method = 0;

        len *= shape[i];
    }

    int* data = (int*)malloc(len * sizeof(int));
    memcpy(data, values, len * sizeof(int));

    data_block->data_type = UDA_TYPE_INT;
    data_block->data = (char*)data;
    data_block->data_n = (int)len;

    return 0;
}

int setReturnDataDoubleScalar(DATA_BLOCK* data_block, double value, const char* description)
{
    initDataBlock(data_block);

    auto data = (double*)malloc(sizeof(double));
    data[0] = value;

    if (description != nullptr) {
        strncpy(data_block->data_desc, description, STRING_LENGTH);
        data_block->data_desc[STRING_LENGTH - 1] = '\0';
    }

    data_block->rank = 0;
    data_block->data_type = UDA_TYPE_DOUBLE;
    data_block->data = (char*)data;
    data_block->data_n = 1;

    return 0;
}

int setReturnDataFloatScalar(DATA_BLOCK* data_block, float value, const char* description)
{
    initDataBlock(data_block);

    auto data = (float*)malloc(sizeof(float));
    data[0] = value;

    if (description != nullptr) {
        strncpy(data_block->data_desc, description, STRING_LENGTH);
        data_block->data_desc[STRING_LENGTH - 1] = '\0';
    }

    data_block->rank = 0;
    data_block->data_type = UDA_TYPE_FLOAT;
    data_block->data = (char*)data;
    data_block->data_n = 1;

    return 0;
}

int setReturnDataIntScalar(DATA_BLOCK* data_block, int value, const char* description)
{
    initDataBlock(data_block);

    int* data = (int*)malloc(sizeof(int));
    data[0] = value;

    if (description != nullptr) {
        strncpy(data_block->data_desc, description, STRING_LENGTH);
        data_block->data_desc[STRING_LENGTH - 1] = '\0';
    }

    data_block->rank = 0;
    data_block->data_type = UDA_TYPE_INT;
    data_block->data = (char*)data;
    data_block->data_n = 1;

    return 0;
}

int setReturnDataLongScalar(DATA_BLOCK* data_block, long value, const char* description)
{
    initDataBlock(data_block);

    long* data = (long*)malloc(sizeof(long));
    data[0] = value;

    if (description != nullptr) {
        strncpy(data_block->data_desc, description, STRING_LENGTH);
        data_block->data_desc[STRING_LENGTH - 1] = '\0';
    }

    data_block->rank = 0;
    data_block->data_type = UDA_TYPE_LONG;
    data_block->data = (char*)data;
    data_block->data_n = 1;

    return 0;
}

int setReturnDataShortScalar(DATA_BLOCK* data_block, short value, const char* description)
{
    initDataBlock(data_block);

    auto data = (short*)malloc(sizeof(short));
    data[0] = value;

    if (description != nullptr) {
        strncpy(data_block->data_desc, description, STRING_LENGTH);
        data_block->data_desc[STRING_LENGTH - 1] = '\0';
    }

    data_block->rank = 0;
    data_block->data_type = UDA_TYPE_SHORT;
    data_block->data = (char*)data;
    data_block->data_n = 1;

    return 0;
}

int setReturnDataString(DATA_BLOCK* data_block, const char* value, const char* description)
{
    initDataBlock(data_block);

    data_block->data_type = UDA_TYPE_STRING;
    data_block->data = strdup(value);

    data_block->rank = 1;
    data_block->dims = (DIMS*)malloc(data_block->rank * sizeof(DIMS));

    for (unsigned int i = 0; i < data_block->rank; i++) {
        initDimBlock(&data_block->dims[i]);
    }

    if (description != nullptr) {
        strncpy(data_block->data_desc, description, STRING_LENGTH);
        data_block->data_desc[STRING_LENGTH - 1] = '\0';
    }

    data_block->dims[0].data_type = UDA_TYPE_UNSIGNED_INT;
    data_block->dims[0].dim_n = (int)strlen(value) + 1;
    data_block->dims[0].compressed = 1;
    data_block->dims[0].dim0 = 0.0;
    data_block->dims[0].diff = 1.0;
    data_block->dims[0].method = 0;

    data_block->data_n = data_block->dims[0].dim_n;

    return 0;
}

/**
 * Look for an argument with the given name in the provided NAMEVALUELIST and return it's associated value.
 *
 * If the argument is found the value associated with the argument is provided via the value parameter and the function
 * returns 1. Otherwise value is set to nullptr and the function returns 0.
 * @param namevaluelist
 * @param value
 * @param name
 * @return
 */
bool findStringValue(const NAMEVALUELIST* namevaluelist, const char** value, const char* name)
{
    char** names = SplitString(name, "|");
    *value = nullptr;

    bool found = false;
    for (int i = 0; i < namevaluelist->pairCount; i++) {
        size_t n;
        for (n = 0; names[n] != nullptr; ++n) {
            if (STR_IEQUALS(namevaluelist->nameValue[i].name, names[n])) {
                *value = namevaluelist->nameValue[i].value;
                found = true;
                break;
            }
        }
    }

    FreeSplitStringTokens(&names);
    return found;
}

/**
 * Look for an argument with the given name in the provided NAMEVALUELIST and return it's associate value as an integer.
 *
 * If the argument is found the value associated with the argument is provided via the value parameter and the function
 * returns 1. Otherwise value is not set and the function returns 0.
 * @param namevaluelist
 * @param value
 * @param name
 * @return
 */
bool findIntValue(const NAMEVALUELIST* namevaluelist, int* value, const char* name)
{
    const char* str;
    bool found = findStringValue(namevaluelist, &str, name);
    if (found) {
        *value = atoi(str);
    }
    return found;
}

/**
 * Look for an argument with the given name in the provided NAMEVALUELIST and return it's associate value as a short.
 *
 * If the argument is found the value associated with the argument is provided via the value parameter and the function
 * returns 1. Otherwise value is not set and the function returns 0.
 * @param namevaluelist
 * @param value
 * @param name
 * @return
 */
bool findShortValue(const NAMEVALUELIST* namevaluelist, short* value, const char* name)
{
    const char* str;
    bool found = findStringValue(namevaluelist, &str, name);
    if (found) {
        *value = (short)atoi(str);
    }
    return found;
}

/**
 * Look for an argument with the given name in the provided NAMEVALUELIST and return it's associate value as a short.
 *
 * If the argument is found the value associated with the argument is provided via the value parameter and the function
 * returns 1. Otherwise value is not set and the function returns 0.
 * @param namevaluelist
 * @param value
 * @param name
 * @return
 */
bool findCharValue(const NAMEVALUELIST* namevaluelist, char* value, const char* name)
{
    const char* str;
    bool found = findStringValue(namevaluelist, &str, name);
    if (found) {
        *value = (char)atoi(str);
    }
    return found;
}

/**
 * Look for an argument with the given name in the provided NAMEVALUELIST and return it's associate value as a float.
 *
 * If the argument is found the value associated with the argument is provided via the value parameter and the function
 * returns 1. Otherwise value is not set and the function returns 0.
 * @param namevaluelist
 * @param value
 * @param name
 * @return
 */
bool findFloatValue(const NAMEVALUELIST* namevaluelist, float* value, const char* name)
{
    const char* str;
    bool found = findStringValue(namevaluelist, &str, name);
    if (found) {
        *value = strtof(str, nullptr);
    }
    return found;
}

bool findIntArray(const NAMEVALUELIST* namevaluelist, int** values, size_t* nvalues, const char* name)
{
    const char* str;
    bool found = findStringValue(namevaluelist, &str, name);
    if (found) {
        char** tokens = SplitString(str, ";");
        size_t n;
        size_t num_tokens = 0;
        for (n = 0; tokens[n] != nullptr; ++n) {
            ++num_tokens;
        }
        *values = (int*)calloc(num_tokens, sizeof(int));
        for (n = 0; tokens[n] != nullptr; ++n) {
            (*values)[n] = (int)strtol(tokens[n], nullptr, 10);
        }
        FreeSplitStringTokens(&tokens);
        *nvalues = num_tokens;
    }
    return found;
}

bool findFloatArray(const NAMEVALUELIST* namevaluelist, float** values, size_t* nvalues, const char* name)
{
    const char* str;
    bool found = findStringValue(namevaluelist, &str, name);
    if (found) {
        char** tokens = SplitString(str, ";");
        size_t n;
        size_t num_tokens = 0;
        for (n = 0; tokens[n] != nullptr; ++n) {
            ++num_tokens;
        }
        *values = (float*)calloc(num_tokens, sizeof(float));
        for (n = 0; tokens[n] != nullptr; ++n) {
            (*values)[n] = strtof(tokens[n], nullptr);
        }
        FreeSplitStringTokens(&tokens);
        *nvalues = num_tokens;
    }
    return found;
}

bool findDoubleArray(const NAMEVALUELIST* namevaluelist, double** values, size_t* nvalues, const char* name)
{
    const char* str;
    bool found = findStringValue(namevaluelist, &str, name);
    if (found) {
        char** tokens = SplitString(str, ";");
        size_t n;
        size_t num_tokens = 0;
        for (n = 0; tokens[n] != nullptr; ++n) {
            ++num_tokens;
        }
        *values = (double*)calloc(num_tokens, sizeof(double));
        for (n = 0; tokens[n] != nullptr; ++n) {
            (*values)[n] = strtod(tokens[n], nullptr);
        }
        FreeSplitStringTokens(&tokens);
        *nvalues = num_tokens;
    }
    return found;
}

bool findValue(const NAMEVALUELIST* namevaluelist, const char* name)
{
    char** names = SplitString(name, "|");

    bool found = false;
    for (int i = 0; i < namevaluelist->pairCount; i++) {
        size_t n = 0;
        while (names[n] != nullptr) {
            if (STR_IEQUALS(namevaluelist->nameValue[i].name, names[n])) {
                found = true;
                break;
            }
            ++n;
        }
    }

    FreeSplitStringTokens(&names);
    return found;
}

int callPlugin(const PLUGINLIST* pluginlist, const char* singal, const IDAM_PLUGIN_INTERFACE* old_plugin_interface)
{
    IDAM_PLUGIN_INTERFACE idam_plugin_interface = *old_plugin_interface;
    REQUEST_DATA request = *old_plugin_interface->request_data;
    idam_plugin_interface.request_data = &request;

    request.source[0] = '\0';
    strcpy(request.signal, singal);
    makeRequestData(&request, *pluginlist, old_plugin_interface->environment);

    request.request = findPluginRequestByFormat(request.format, pluginlist);
    if (request.request == REQUEST_READ_UNKNOWN) {
        RAISE_PLUGIN_ERROR("Plugin not found!")
    }

    int err;
    int id = findPluginIdByRequest(request.request, pluginlist);
    PLUGIN_DATA* plugin = &(pluginlist->plugin[id]);
    if (id >= 0 && plugin->idamPlugin != nullptr) {
        err = plugin->idamPlugin(&idam_plugin_interface);    // Call the data reader
    } else {
        RAISE_PLUGIN_ERROR("Data Access is not available for this data request!")
    }

    return err;
}
