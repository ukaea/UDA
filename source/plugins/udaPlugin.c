#include "udaPlugin.h"

#include <errno.h>

#include <clientserver/makeRequestBlock.h>
#include <clientserver/stringUtils.h>
#include <clientserver/initStructs.h>
#include <clientserver/udaTypes.h>
#include <clientserver/protocol.h>

int initPlugin(const IDAM_PLUGIN_INTERFACE* plugin_interface)
{
    idamSetLogLevel((LOG_LEVEL)plugin_interface->environment->loglevel);

    return 0;
}

int setReturnDataFloatArray(DATA_BLOCK* data_block, float* values, size_t rank, const size_t* shape,
                            const char* description)
{
    initDataBlock(data_block);

    if (description != NULL) {
        strncpy(data_block->data_desc, description, STRING_LENGTH);
        data_block->data_desc[STRING_LENGTH - 1] = '\0';
    }

    data_block->rank = (int)rank;
    data_block->dims = malloc(rank * sizeof(DIMS));

    size_t len = 1;

    size_t i;
    for (i = 0; i < rank; ++i) {
        initDimBlock(&data_block->dims[i]);

        data_block->dims[i].data_type = UDA_TYPE_UNSIGNED_INT;
        data_block->dims[i].dim_n = (int)shape[i];
        data_block->dims[i].compressed = 1;
        data_block->dims[i].dim0 = 0.0;
        data_block->dims[i].diff = 1.0;
        data_block->dims[i].method = 0;

        len *= shape[i];
    }

    float* data = malloc(len * sizeof(float));
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

    if (description != NULL) {
        strncpy(data_block->data_desc, description, STRING_LENGTH);
        data_block->data_desc[STRING_LENGTH - 1] = '\0';
    }

    data_block->rank = (int)rank;
    data_block->dims = malloc(rank * sizeof(DIMS));

    size_t len = 1;

    size_t i;
    for (i = 0; i < rank; ++i) {
        initDimBlock(&data_block->dims[i]);

        data_block->dims[i].data_type = UDA_TYPE_UNSIGNED_INT;
        data_block->dims[i].dim_n = (int)shape[i];
        data_block->dims[i].compressed = 1;
        data_block->dims[i].dim0 = 0.0;
        data_block->dims[i].diff = 1.0;
        data_block->dims[i].method = 0;

        len *= shape[i];
    }

    double* data = malloc(len * sizeof(double));
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

    if (description != NULL) {
        strncpy(data_block->data_desc, description, STRING_LENGTH);
        data_block->data_desc[STRING_LENGTH - 1] = '\0';
    }

    data_block->rank = (int)rank;
    data_block->dims = malloc(rank * sizeof(DIMS));

    size_t len = 1;

    size_t i;
    for (i = 0; i < rank; ++i) {
        initDimBlock(&data_block->dims[i]);

        data_block->dims[i].data_type = UDA_TYPE_UNSIGNED_INT;
        data_block->dims[i].dim_n = (int)shape[i];
        data_block->dims[i].compressed = 1;
        data_block->dims[i].dim0 = 0.0;
        data_block->dims[i].diff = 1.0;
        data_block->dims[i].method = 0;

        len *= shape[i];
    }

    int* data = malloc(len * sizeof(int));
    memcpy(data, values, len * sizeof(int));

    data_block->data_type = UDA_TYPE_INT;
    data_block->data = (char*)data;
    data_block->data_n = (int)len;

    return 0;
}

int setReturnDataDoubleScalar(DATA_BLOCK* data_block, double value, const char* description)
{
    initDataBlock(data_block);

    double* data = (double*)malloc(sizeof(double));
    data[0] = value;

    if (description != NULL) {
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

    float* data = (float*)malloc(sizeof(float));
    data[0] = value;

    if (description != NULL) {
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

    if (description != NULL) {
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

    if (description != NULL) {
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

    short* data = (short*)malloc(sizeof(short));
    data[0] = value;

    if (description != NULL) {
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

    unsigned int i;
    for (i = 0; i < data_block->rank; i++) {
        initDimBlock(&data_block->dims[i]);
    }

    if (description != NULL) {
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
 * Find the Plugin identity: return the reference id or -1 if not found.
 * @param request
 * @param plugin_list
 * @return
 */
int findPluginIdByRequest(int request, const PLUGINLIST* plugin_list)
{
    int i;
    for (i = 0; i < plugin_list->count; i++) {
        if (plugin_list->plugin[i].request == request) return i;
    }
    return -1;
}

/**
 * Find the Plugin identity: return the reference id or -1 if not found.
 * @param format
 * @param plugin_list
 * @return
 */
int findPluginIdByFormat(const char* format, const PLUGINLIST* plugin_list)
{
    int i;
    for (i = 0; i < plugin_list->count; i++) {
        if (STR_IEQUALS(plugin_list->plugin[i].format, format)) return i;
    }
    return -1;
}

/**
 * Find the Plugin identity: return the reference id or -1 if not found.
 * @param device
 * @param plugin_list
 * @return
 */
int findPluginIdByDevice(const char* device, const PLUGINLIST* plugin_list)
{
    int i;
    for (i = 0; i < plugin_list->count; i++) {
        if (plugin_list->plugin[i].plugin_class == UDA_PLUGIN_CLASS_DEVICE && STR_IEQUALS(plugin_list->plugin[i].format, device)) {
            return i;
        }
    }
    return -1;
}

/**
 * Find the Plugin Request: return the request or REQUEST_READ_UNKNOWN if not found.
 * @param format
 * @param plugin_list
 * @return
 */
int findPluginRequestByFormat(const char* format, const PLUGINLIST* plugin_list)
{
    int i;
    for (i = 0; i < plugin_list->count; i++) {
        if (STR_IEQUALS(plugin_list->plugin[i].format, format)) return plugin_list->plugin[i].request;
    }
    return REQUEST_READ_UNKNOWN;
}

/**
 * Find the Plugin Request: return the request or REQUEST_READ_UNKNOWN if not found.
 * @param extension
 * @param plugin_list
 * @return
 */
int findPluginRequestByExtension(const char* extension, const PLUGINLIST* plugin_list)
{
    int i;
    for (i = 0; i < plugin_list->count; i++) {
        if (STR_IEQUALS(plugin_list->plugin[i].extension, extension)) return plugin_list->plugin[i].request;
    }
    return REQUEST_READ_UNKNOWN;
}

/**
 * Look for an argument with the given name in the provided NAMEVALUELIST and return it's associated value.
 *
 * If the argument is found the value associated with the argument is provided via the value parameter and the function
 * returns 1. Otherwise value is set to NULL and the function returns 0.
 * @param namevaluelist
 * @param value
 * @param name
 * @return
 */
bool findStringValue(const NAMEVALUELIST* namevaluelist, const char** value, const char* name)
{
    char** names = SplitString(name, "|");
    *value = NULL;

    bool found = 0;
    int i;
    for (i = 0; i < namevaluelist->pairCount; i++) {
        size_t n;
        for (n = 0; names[n] != NULL; ++n) {
            if (STR_IEQUALS(namevaluelist->nameValue[i].name, names[n])) {
                *value = namevaluelist->nameValue[i].value;
                found = 1;
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
        *value = strtof(str, NULL);
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
        for (n = 0; tokens[n] != NULL; ++n) {
            ++num_tokens;
        }
        *values = calloc(num_tokens, sizeof(int));
        for (n = 0; tokens[n] != NULL; ++n) {
            (*values)[n] = (int)strtol(tokens[n], NULL, 10);
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
        for (n = 0; tokens[n] != NULL; ++n) {
            ++num_tokens;
        }
        *values = calloc(num_tokens, sizeof(float));
        for (n = 0; tokens[n] != NULL; ++n) {
            (*values)[n] = strtof(tokens[n], NULL);
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
        for (n = 0; tokens[n] != NULL; ++n) {
            ++num_tokens;
        }
        *values = calloc(num_tokens, sizeof(double));
        for (n = 0; tokens[n] != NULL; ++n) {
            (*values)[n] = strtod(tokens[n], NULL);
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
    int i;
    for (i = 0; i < namevaluelist->pairCount; i++) {
        size_t n = 0;
        while (names[n] != NULL) {
            if (STR_IEQUALS(namevaluelist->nameValue[i].name, names[n])) {
                found = 1;
                break;
            }
            ++n;
        }
    }

    FreeSplitStringTokens(&names);
    return found;
}

int callPlugin(const PLUGINLIST* pluginlist, const char* request, const IDAM_PLUGIN_INTERFACE* old_plugin_interface)
{
    IDAM_PLUGIN_INTERFACE idam_plugin_interface = *old_plugin_interface;
    REQUEST_BLOCK request_block = *old_plugin_interface->request_block;
    idam_plugin_interface.request_block = &request_block;

    request_block.source[0] = '\0';
    strcpy(request_block.signal, request);
    makeRequestBlock(&request_block, *pluginlist, old_plugin_interface->environment);

    request_block.request = findPluginRequestByFormat(request_block.format, pluginlist);

    if (request_block.request < 0) {
        RAISE_PLUGIN_ERROR("Plugin not found!");
    }

    int err = 0;
    int id = findPluginIdByRequest(request_block.request, pluginlist);
    PLUGIN_DATA* plugin = &(pluginlist->plugin[id]);
    if (id >= 0 && plugin->idamPlugin != NULL) {
        err = plugin->idamPlugin(&idam_plugin_interface);    // Call the data reader
    } else {
        RAISE_PLUGIN_ERROR("Data Access is not available for this data request!");
    }

    return err;
}
