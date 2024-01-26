#include "udaPlugin.h"

#include "server/initPluginList.h"
#include "server/serverPlugin.h"
#include "server/serverSubsetData.h"
#include "struct.h"

#include "clientserver/initStructs.h"
#include "logging/logging.h"
#include "udaTypes.h"
#include "accessors.h"
#include <clientserver/errorLog.h>
#include <clientserver/makeRequestBlock.h>
#include <clientserver/stringUtils.h>
#include <server/getServerEnvironment.h>

UDA_PLUGIN_INTERFACE* udaCreatePluginInterface(const char* request)
{
    auto plugin_interface = (UDA_PLUGIN_INTERFACE*)calloc(1, sizeof(UDA_PLUGIN_INTERFACE));
    auto environment = getServerEnvironment();
    auto plugin_list = (PLUGINLIST*)calloc(1, sizeof(PLUGINLIST));

    initPluginList(plugin_list, environment);

    auto request_data = (REQUEST_DATA*)calloc(1, sizeof(REQUEST_DATA));
    makeRequestData(request_data, plugin_list, environment);

    auto user_defined_type_list = (USERDEFINEDTYPELIST*)calloc(1, sizeof(USERDEFINEDTYPELIST));
    auto log_malloc_list = (LOGMALLOCLIST*)calloc(1, sizeof(LOGMALLOCLIST));

    plugin_interface->request_data = request_data;
    plugin_interface->pluginList = plugin_list;
    plugin_interface->environment = environment;
    plugin_interface->userdefinedtypelist = user_defined_type_list;
    plugin_interface->logmalloclist = log_malloc_list;

    plugin_interface->interfaceVersion = 1;
    plugin_interface->pluginVersion = 0;
    plugin_interface->housekeeping = 0;
    plugin_interface->changePlugin = 0;
    plugin_interface->error_stack.nerrors = 0;
    plugin_interface->error_stack.idamerror = nullptr;

    return plugin_interface;
}

void udaFreePluginInterface(UDA_PLUGIN_INTERFACE* plugin_interface)
{
    free(plugin_interface->request_data);
    freeUserDefinedTypeList(plugin_interface->userdefinedtypelist);
    free(plugin_interface->userdefinedtypelist);
    freeMallocLogList(plugin_interface->logmalloclist);
    free(plugin_interface->logmalloclist);
    freePluginList((PLUGINLIST*)plugin_interface->pluginList);
    free((PLUGINLIST*)plugin_interface->pluginList);
    free((ENVIRONMENT*)plugin_interface->environment);
    free(plugin_interface);
}

int initPlugin(const UDA_PLUGIN_INTERFACE* plugin_interface)
{
    udaSetLogLevel((LOG_LEVEL)plugin_interface->environment->loglevel);
    return 0;
}

int setReturnDataFloatArray(UDA_PLUGIN_INTERFACE* plugin_interface, float* values, size_t rank, const size_t* shape,
                            const char* description)
{
    DATA_BLOCK* data_block = plugin_interface->data_block;
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

int setReturnDataDoubleArray(UDA_PLUGIN_INTERFACE* plugin_interface, double* values, size_t rank, const size_t* shape,
                             const char* description)
{
    DATA_BLOCK* data_block = plugin_interface->data_block;
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

int setReturnDataIntArray(UDA_PLUGIN_INTERFACE* plugin_interface, int* values, size_t rank, const size_t* shape,
                          const char* description)
{
    DATA_BLOCK* data_block = plugin_interface->data_block;
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

int setReturnDataDoubleScalar(UDA_PLUGIN_INTERFACE* plugin_interface, double value, const char* description)
{
    DATA_BLOCK* data_block = plugin_interface->data_block;
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

int setReturnDataFloatScalar(UDA_PLUGIN_INTERFACE* plugin_interface, float value, const char* description)
{
    DATA_BLOCK* data_block = plugin_interface->data_block;
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

int setReturnDataIntScalar(UDA_PLUGIN_INTERFACE* plugin_interface, int value, const char* description)
{
    DATA_BLOCK* data_block = plugin_interface->data_block;
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

int setReturnDataLongScalar(UDA_PLUGIN_INTERFACE* plugin_interface, long value, const char* description)
{
    DATA_BLOCK* data_block = plugin_interface->data_block;
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

int setReturnDataShortScalar(UDA_PLUGIN_INTERFACE* plugin_interface, short value, const char* description)
{
    DATA_BLOCK* data_block = plugin_interface->data_block;
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

int setReturnDataString(UDA_PLUGIN_INTERFACE* plugin_interface, const char* value, const char* description)
{
    DATA_BLOCK* data_block = plugin_interface->data_block;
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

int setReturnData(UDA_PLUGIN_INTERFACE* plugin_interface, void* value, size_t size, UDA_TYPE type, int rank, const int* shape,
                  const char* description)
{
    DATA_BLOCK* data_block = plugin_interface->data_block;
    initDataBlock(data_block);

    data_block->data_type = type;
    data_block->data = (char*)malloc(size);

    memcpy(data_block->data, value, size);

    data_block->rank = rank;
    data_block->dims = (DIMS*)malloc(data_block->rank * sizeof(DIMS));

    for (unsigned int i = 0; i < data_block->rank; i++) {
        initDimBlock(&data_block->dims[i]);
    }

    if (description != nullptr) {
        strncpy(data_block->data_desc, description, STRING_LENGTH);
        data_block->data_desc[STRING_LENGTH - 1] = '\0';
    }

    for (unsigned int i = 0; i < data_block->rank; i++) {
        data_block->dims[i].data_type = UDA_TYPE_UNSIGNED_INT;
        data_block->dims[i].dim_n = shape[i];
        data_block->dims[i].compressed = 1;
        data_block->dims[i].dim0 = 0.0;
        data_block->dims[i].diff = 1.0;
        data_block->dims[i].method = 0;
    }

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
bool findStringValue(const UDA_PLUGIN_INTERFACE* plugin_interface, const char** value, const char* name)
{
    auto namevaluelist = &plugin_interface->request_data->nameValueList;

    char** names = SplitString(name, "|");
    *value = nullptr;

    bool found = 0;
    for (int i = 0; i < namevaluelist->pairCount; i++) {
        size_t n;
        for (n = 0; names[n] != nullptr; ++n) {
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
bool findIntValue(const UDA_PLUGIN_INTERFACE* plugin_interface, int* value, const char* name)
{
    const char* str;
    bool found = findStringValue(plugin_interface, &str, name);
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
bool findShortValue(const UDA_PLUGIN_INTERFACE* plugin_interface, short* value, const char* name)
{
    const char* str;
    bool found = findStringValue(plugin_interface, &str, name);
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
bool findCharValue(const UDA_PLUGIN_INTERFACE* plugin_interface, char* value, const char* name)
{
    const char* str;
    bool found = findStringValue(plugin_interface, &str, name);
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
bool findFloatValue(const UDA_PLUGIN_INTERFACE* plugin_interface, float* value, const char* name)
{
    const char* str;
    bool found = findStringValue(plugin_interface, &str, name);
    if (found) {
        *value = strtof(str, nullptr);
    }
    return found;
}

bool findIntArray(const UDA_PLUGIN_INTERFACE* plugin_interface, int** values, size_t* nvalues, const char* name)
{
    const char* str;
    bool found = findStringValue(plugin_interface, &str, name);
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

bool findFloatArray(const UDA_PLUGIN_INTERFACE* plugin_interface, float** values, size_t* nvalues, const char* name)
{
    const char* str;
    bool found = findStringValue(plugin_interface, &str, name);
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

bool findDoubleArray(const UDA_PLUGIN_INTERFACE* plugin_interface, double** values, size_t* nvalues, const char* name)
{
    const char* str;
    bool found = findStringValue(plugin_interface, &str, name);
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

bool findValue(const UDA_PLUGIN_INTERFACE* plugin_interface, const char* name)
{
    auto namevaluelist = &plugin_interface->request_data->nameValueList;
    char** names = SplitString(name, "|");

    bool found = false;
    for (int i = 0; i < namevaluelist->pairCount; i++) {
        size_t n = 0;
        while (names[n] != nullptr) {
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

int callPlugin(UDA_PLUGIN_INTERFACE* plugin_interface, const char* request)
{
    UDA_PLUGIN_INTERFACE new_plugin_interface = *plugin_interface;
    REQUEST_DATA request_data = *plugin_interface->request_data;
    new_plugin_interface.request_data = &request_data;

    request_data.source[0] = '\0';
    strcpy(request_data.signal, request);
    makeRequestData(&request_data, plugin_interface->pluginList, plugin_interface->environment);

    request_data.request = findPluginRequestByFormat(request_data.format, plugin_interface->pluginList);
    if (request_data.request == REQUEST_READ_UNKNOWN) {
        RAISE_PLUGIN_ERROR(plugin_interface, "Plugin not found!");
    }

    int err = 0;
    int id = findPluginIdByRequest(request_data.request, plugin_interface->pluginList);
    PLUGIN_DATA* plugin = &(plugin_interface->pluginList->plugin[id]);
    if (id >= 0 && plugin->idamPlugin != nullptr) {
        err = plugin->idamPlugin(&new_plugin_interface); // Call the data reader
    } else {
        RAISE_PLUGIN_ERROR(plugin_interface, "Data Access is not available for this data request!");
    }

    // Apply subsettinng
    if (request_data.datasubset.nbound > 0) {
        ACTION action = {};
        initAction(&action);
        action.actionType = UDA_SUBSET_TYPE;
        action.subset = request_data.datasubset;
        err = serverSubsetData(new_plugin_interface.data_block, action, new_plugin_interface.logmalloclist);
    }

    return err;
}

int setReturnCompoundData(UDA_PLUGIN_INTERFACE *plugin_interface, char* data, const char *user_type) {
    DATA_BLOCK* data_block = plugin_interface->data_block;
    initDataBlock(data_block);

    data_block->data_type = UDA_TYPE_COMPOUND;
    data_block->rank = 0;
    data_block->data_n = 1;
    data_block->data = data;

    strcpy(data_block->data_desc, "Local UDA server time");
    strcpy(data_block->data_label, "servertime");
    strcpy(data_block->data_units, "");

    data_block->opaque_type = UDA_OPAQUE_TYPE_STRUCTURES;
    data_block->opaque_count = 1;
    data_block->opaque_block = (void*)findUserDefinedType(plugin_interface->userdefinedtypelist, user_type, 0);

    return 0;
}

COMPOUNDFIELD* udaNewCompoundField(const char* name, const char* description, int* offset, int type)
{
    COMPOUNDFIELD* field = (COMPOUNDFIELD*)malloc(sizeof(COMPOUNDFIELD));
    defineField(field, name, description, offset, type);
    return field;
}

USERDEFINEDTYPE* udaNewUserType(const char* name, const char* source, int ref_id, int image_count, char* image, size_t size, size_t num_fields, COMPOUNDFIELD** fields)
{
    USERDEFINEDTYPE* user_type = (USERDEFINEDTYPE*)malloc(sizeof(USERDEFINEDTYPE));

    strcpy(user_type->name, name);
    strcpy(user_type->source, source);
    user_type->ref_id = ref_id;
    user_type->imagecount = image_count; // No Structure Image data
    user_type->image = image;
    user_type->size = size; // Structure size
    user_type->idamclass = UDA_TYPE_COMPOUND;

    for (size_t i = 0; i < num_fields; ++i) {
        COMPOUNDFIELD* field = fields[i];
        addCompoundField(user_type, *field);
    }

    return user_type;
}

int udaAddUserType(UDA_PLUGIN_INTERFACE* plugin_interface, USERDEFINEDTYPE* user_type)
{
    USERDEFINEDTYPELIST* userdefinedtypelist = plugin_interface->userdefinedtypelist;
    addUserDefinedType(userdefinedtypelist, *user_type);

    return 0;
}

int udaRegisterMalloc(UDA_PLUGIN_INTERFACE* plugin_interface, void* data, int count, size_t size, const char* type)
{
    addMalloc(plugin_interface->logmalloclist, data, count, size, type);

    return 0;
}

int udaPluginPluginsCount(UDA_PLUGIN_INTERFACE* plugin_interface)
{
    return plugin_interface->pluginList->count;
}

namespace {
int check_plugin_class(UDA_PLUGIN_INTERFACE* plugin_interface, int plugin_num, int plugin_class)
{
    auto plugin_list = plugin_interface->pluginList;
    auto environment = plugin_interface->environment;

    bool is_valid = plugin_list->plugin[plugin_num].plugin_class == plugin_class &&
                    plugin_list->plugin[plugin_num].status == UDA_PLUGIN_OPERATIONAL &&
                    (plugin_list->plugin[plugin_num].is_private == UDA_PLUGIN_PUBLIC ||
                     (plugin_list->plugin[plugin_num].is_private == UDA_PLUGIN_PRIVATE && !environment->external_user));

    if (plugin_class == UDA_PLUGIN_CLASS_FILE) {
        is_valid |= (plugin_list->plugin[plugin_num].format[0] != '\0' && plugin_list->plugin[plugin_num].extension[0] != '\0');
    }

    return is_valid;
}

}

int udaPluginCheckPluginClass(UDA_PLUGIN_INTERFACE* plugin_interface, int plugin_num, const char* plugin_class)
{
    std::string plugin_class_string = plugin_class;

    if (plugin_class_string == "file") {
        return check_plugin_class(plugin_interface, plugin_num, UDA_PLUGIN_CLASS_FILE);
    } else if (plugin_class_string == "function") {
        return check_plugin_class(plugin_interface, plugin_num, UDA_PLUGIN_CLASS_FUNCTION);
    } else if (plugin_class_string == "server") {
        return check_plugin_class(plugin_interface, plugin_num, UDA_PLUGIN_CLASS_SERVER);
    } else if (plugin_class_string == "device") {
        return check_plugin_class(plugin_interface, plugin_num, UDA_PLUGIN_CLASS_DEVICE);
    } else if (plugin_class_string == "other") {
        return check_plugin_class(plugin_interface, plugin_num, UDA_PLUGIN_CLASS_OTHER);
    }

    return 0;
}

const char* udaPluginPluginFormat(UDA_PLUGIN_INTERFACE* plugin_interface, int plugin_num)
{
    return plugin_interface->pluginList->plugin[plugin_num].format;
}

const char* udaPluginPluginExtension(UDA_PLUGIN_INTERFACE* plugin_interface, int plugin_num)
{
    return plugin_interface->pluginList->plugin[plugin_num].extension;
}

const char* udaPluginPluginDescription(UDA_PLUGIN_INTERFACE* plugin_interface, int plugin_num)
{
    return plugin_interface->pluginList->plugin[plugin_num].desc;
}

const char* udaPluginPluginExample(UDA_PLUGIN_INTERFACE* plugin_interface, int plugin_num)
{
    return plugin_interface->pluginList->plugin[plugin_num].example;
}

void udaPluginLog(UDA_PLUGIN_INTERFACE* plugin_interface, const char* fmt, ...)
{
    va_list args;
    va_start(args, fmt);

    udaLog(UDA_LOG_DEBUG, fmt, args);

    va_end(args);
}

void udaAddPluginError(UDA_PLUGIN_INTERFACE* plugin_interface, const char* location, int code, const char* msg)
{
    udaLog(UDA_LOG_ERROR, msg);
    plugin_interface->error_stack.nerrors += 1;
    plugin_interface->error_stack.idamerror = (UDA_ERROR*)realloc(plugin_interface->error_stack.idamerror, plugin_interface->error_stack.nerrors * sizeof(UDA_ERROR));
    plugin_interface->error_stack.idamerror[plugin_interface->error_stack.nerrors - 1] = createIdamError(UDA_CODE_ERROR_TYPE, location, code, msg);
}

int udaPluginIsExternal(UDA_PLUGIN_INTERFACE* plugin_interface)
{
    return plugin_interface->environment->external_user;
}

int udaPluginCheckInterfaceVersion(UDA_PLUGIN_INTERFACE* plugin_interface, int interface_version)
{
    return plugin_interface->interfaceVersion > interface_version;
}

void udaPluginSetVersion(UDA_PLUGIN_INTERFACE* plugin_interface, int plugin_version)
{
    plugin_interface->pluginVersion = plugin_version;
}

const char* udaPluginFunction(UDA_PLUGIN_INTERFACE* plugin_interface)
{
    return plugin_interface->request_data->function;
}

int setReturnDataLabel(UDA_PLUGIN_INTERFACE* plugin_interface, const char* label)
{
    DATA_BLOCK* data_block = plugin_interface->data_block;
    strcpy(data_block->data_label, label);
    return 0;
}

int setReturnDataUnits(UDA_PLUGIN_INTERFACE* plugin_interface, const char* units)
{
    DATA_BLOCK* data_block = plugin_interface->data_block;
    strcpy(data_block->data_units, units);
    return 0;
}

int setReturnErrorAsymmetry(UDA_PLUGIN_INTERFACE* plugin_interface, bool flag)
{
    DATA_BLOCK* data_block = plugin_interface->data_block;
    data_block->errasymmetry = flag;
    return 0;
}

char* memdup(const void* mem, size_t size)
{
    char* out = (char*)malloc(size);

    if (out != nullptr) {
        memcpy(out, mem, size);
    }

    return out;
}

int setReturnErrorLow(UDA_PLUGIN_INTERFACE* plugin_interface, float* data, size_t size)
{
    DATA_BLOCK* data_block = plugin_interface->data_block;
    data_block->errlo = memdup(data, size);
    return 0;
}

int setReturnErrorHigh(UDA_PLUGIN_INTERFACE* plugin_interface, float* data, size_t size)
{
    DATA_BLOCK* data_block = plugin_interface->data_block;
    data_block->errhi = memdup(data, size);
    return 0;
}

int setReturnDataOrder(UDA_PLUGIN_INTERFACE* plugin_interface, int order)
{
    DATA_BLOCK* data_block = plugin_interface->data_block;
    data_block->order = order;
    return 0;
}

int setReturnDimensionFloatArray(UDA_PLUGIN_INTERFACE* plugin_interface, int dim_n, float* data, size_t size, const char* label, const char* units)
{
    DATA_BLOCK* data_block = plugin_interface->data_block;

    data_block->dims[0].data_type = UDA_TYPE_FLOAT;
    data_block->dims[0].dim_n = dim_n;
    data_block->dims[0].compressed = 0;
    strcpy(data_block->dims[0].dim_label, label);
    strcpy(data_block->dims[0].dim_units, units);

    data_block->data = memdup(data, size);
    return 0;
}