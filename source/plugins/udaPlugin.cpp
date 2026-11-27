#include "udaPlugin.h"
#include "structures/struct.h"
#include "server/serverSubsetData.h"

#include <clientserver/makeRequestBlock.h>
#include <clientserver/stringUtils.h>
#include <clientserver/initStructs.h>
#include <clientserver/udaTypes.h>
#include <version.h>

#include <algorithm>
#include <cstdarg>
#include <memory>
#include <type_traits>
#include <regex>
#include <string>
#include <cstring>

int initPlugin(const IDAM_PLUGIN_INTERFACE* plugin_interface)
{
    // Historically set the log level of whichever logger this library was bound to. The host now owns logging
    // (interface version 2), so there is nothing to initialise; kept for source and binary compatibility.
    (void)plugin_interface;
    return 0;
}

template<typename T>
constexpr int get_uda_type()
{
    if (std::is_same<T, int>::value)
    {
        return UDA_TYPE_INT;
    }
    if (std::is_same<T, char>::value)
	{
		return UDA_TYPE_CHAR;
	}
    if (std::is_same<T, short>::value)
	{
		return UDA_TYPE_SHORT;
	}
    if (std::is_same<T, long>::value)
	{
		return UDA_TYPE_LONG;
	}
    if (std::is_same<T, float>::value)
    {
        return UDA_TYPE_FLOAT;
    }
    if (std::is_same<T, double>::value)
    {
        return UDA_TYPE_DOUBLE;
    }
    if (std::is_same<T, unsigned char>::value)
	{
		return UDA_TYPE_UNSIGNED_CHAR;
	}
    if (std::is_same<T, unsigned short>::value)
	{
		return UDA_TYPE_UNSIGNED_SHORT;
	}
    if (std::is_same<T, unsigned int>::value)
	{
		return UDA_TYPE_UNSIGNED_INT;
	}
    if (std::is_same<T, unsigned long>::value)
	{
		return UDA_TYPE_UNSIGNED_LONG;
    }

    return UDA_TYPE_UNDEFINED; 
}

template <typename T>
int setReturnDataArray(DATA_BLOCK* data_block, T* values, size_t rank, const size_t* shape,
                            const char* description)
{
    initDataBlock(data_block);

    if (description != nullptr) {
        strncpy(data_block->data_desc, description, STRING_LENGTH);
        data_block->data_desc[STRING_LENGTH - 1] = '\0';
    }

    data_block->rank = static_cast<int>(rank);
    data_block->dims = reinterpret_cast<DIMS*>(malloc(rank * sizeof(DIMS)));

    size_t len = 1;

    for (size_t i = 0; i < rank; ++i) {
        initDimBlock(&data_block->dims[i]);

        data_block->dims[i].data_type = UDA_TYPE_UNSIGNED_INT;
        data_block->dims[i].dim_n = static_cast<int>(shape[i]);
        data_block->dims[i].compressed = 1;
        data_block->dims[i].dim0 = 0.0;
        data_block->dims[i].diff = 1.0;
        data_block->dims[i].method = 0;

        len *= shape[i];
    }

    auto data = reinterpret_cast<T*>(malloc(len * sizeof(T)));
    memcpy(data, values, len * sizeof(T));

    data_block->data_type = get_uda_type<T>();
    data_block->data = reinterpret_cast<char *>(data);
    data_block->data_n = static_cast<int>(len);

    return 0;
}

int setReturnDataFloatArray(DATA_BLOCK* data_block, float* values, size_t rank, const size_t* shape,
                            const char* description)
{
    return setReturnDataArray(data_block, values, rank, shape, description);
}

int setReturnDataDoubleArray(DATA_BLOCK* data_block, double* values, size_t rank, const size_t* shape,
                             const char* description)
{
    return setReturnDataArray(data_block, values, rank, shape, description);
}

int setReturnDataIntArray(DATA_BLOCK* data_block, int* values, size_t rank, 
                          const size_t* shape, const char* description)
{
    return setReturnDataArray(data_block, values, rank, shape, description);
}

int setReturnDataIShortArray(DATA_BLOCK* data_block, short* values, size_t rank, 
                          const size_t* shape, const char* description)
{
    return setReturnDataArray(data_block, values, rank, shape, description);
}

int setReturnDataLongArray(DATA_BLOCK* data_block, long* values, size_t rank, 
                          const size_t* shape, const char* description)
{
    return setReturnDataArray(data_block, values, rank, shape, description);
}

int setReturnDataUnsignedIntArray(DATA_BLOCK* data_block, unsigned int* values, size_t rank, 
                          const size_t* shape, const char* description)
{
    return setReturnDataArray(data_block, values, rank, shape, description);
}

int setReturnDataUnsignedShortArray(DATA_BLOCK* data_block, unsigned short* values, size_t rank, 
                          const size_t* shape, const char* description)
{
    return setReturnDataArray(data_block, values, rank, shape, description);
}

int setReturnDataUnsignedLongArray(DATA_BLOCK* data_block, unsigned long* values, size_t rank, 
                          const size_t* shape, const char* description)
{
    return setReturnDataArray(data_block, values, rank, shape, description);
}

int setReturnDataUnsignedCharArray(DATA_BLOCK* data_block, unsigned char* values, size_t rank, 
                          const size_t* shape, const char* description)
{
    return setReturnDataArray(data_block, values, rank, shape, description);
}

template<typename T>
int setReturnDataScalar(DATA_BLOCK* data_block, T value, const char* description)
{
    initDataBlock(data_block);

    auto data = reinterpret_cast<T*>(malloc(sizeof(T)));
    data[0] = value;

    if (description != nullptr) {
        strncpy(data_block->data_desc, description, STRING_LENGTH);
        data_block->data_desc[STRING_LENGTH - 1] = '\0';
    }

    data_block->rank = 0;
    data_block->data_type = get_uda_type<T>();
    data_block->data = reinterpret_cast<char *>(data);
    data_block->data_n = 1;

    return 0;
}

int setReturnDataCharScalar(DATA_BLOCK* data_block, char value, const char* description)
{
    return setReturnDataScalar(data_block, value, description);
}

int setReturnDataUnsignedCharScalar(DATA_BLOCK* data_block, unsigned char value, const char* description)
{
    return setReturnDataScalar(data_block, value, description);
}

int setReturnDataDoubleScalar(DATA_BLOCK* data_block, double value, const char* description)
{
    return setReturnDataScalar(data_block, value, description);
}

int setReturnDataFloatScalar(DATA_BLOCK* data_block, float value, const char* description)
{
    return setReturnDataScalar(data_block, value, description);
}

int setReturnDataIntScalar(DATA_BLOCK* data_block, int value, const char* description)
{
    return setReturnDataScalar(data_block, value, description);
}

int setReturnDataUnsignedIntScalar(DATA_BLOCK* data_block, unsigned int value, const char* description)
{
    return setReturnDataScalar(data_block, value, description);
}

int setReturnDataLongScalar(DATA_BLOCK* data_block, long value, const char* description)
{
    return setReturnDataScalar(data_block, value, description);
}

int setReturnDataUnsignedLongScalar(DATA_BLOCK* data_block, unsigned long value, const char* description)
{
    return setReturnDataScalar(data_block, value, description);
}

int setReturnDataShortScalar(DATA_BLOCK* data_block, short value, const char* description)
{
    return setReturnDataScalar(data_block, value, description);
}

int setReturnDataUnsignedShortScalar(DATA_BLOCK* data_block, unsigned short value, const char* description)
{
    return setReturnDataScalar(data_block, value, description);
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

int setReturnData(DATA_BLOCK* data_block, void* value, size_t size, UDA_TYPE type, int rank, const int* shape, const char* description)
{
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
 * Find the Plugin identity: return the reference id or -1 if not found.
 * @param request
 * @param plugin_list
 * @return
 */
int findPluginIdByRequest(int request, const PLUGINLIST* plugin_list)
{
    for (int i = 0; i < plugin_list->count; i++) {
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
    for (int i = 0; i < plugin_list->count; i++) {
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
    for (int i = 0; i < plugin_list->count; i++) {
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
    for (int i = 0; i < plugin_list->count; i++) {
        if (STR_IEQUALS(plugin_list->plugin[i].format, format)) {
            return plugin_list->plugin[i].request;
        }
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
    for (int i = 0; i < plugin_list->count; i++) {
        if (STR_IEQUALS(plugin_list->plugin[i].extension, extension)) {
            return plugin_list->plugin[i].request;
        }
    }
    return REQUEST_READ_UNKNOWN;
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
 * Look for an argument with the given name in the provided NAMEVALUELIST and return it's associate value as a long integer.
 *
 * If the argument is found the value associated with the argument is provided via the value parameter and the function
 * returns 1. Otherwise value is not set and the function returns 0.
 * @param namevaluelist
 * @param value
 * @param name
 * @return
 */
bool findLongIntValue(const NAMEVALUELIST* namevaluelist, long* value, const char* name)
{
    const char* str;
    bool found = findStringValue(namevaluelist, &str, name);
    if (found) {
        *value = atol(str);
    }
    return found;
}

/**
 * Look for an argument with the given name in the provided NAMEVALUELIST and return it's associate value as an
 *  unsigned long integer.
 *
 * If the argument is found the value associated with the argument is provided via the value parameter and the function
 * returns 1. Otherwise value is not set and the function returns 0.
 * @param namevaluelist
 * @param value
 * @param name
 * @return
 */
bool findUnsignedLongValue(const NAMEVALUELIST* namevaluelist, unsigned long* value, const char* name)
{
    const char* str;
    bool found = findStringValue(namevaluelist, &str, name);
    if (found) {
        if (std::strchr(str, '-') != nullptr) {
            // No plugin interface here to report through, so a negative value is treated as not supplied (the
            // legacy RAISE_PLUGIN_ERROR used here returned 999, i.e. "found", without setting the value)
            return false;
        }
        *value = std::strtoul(str, nullptr, 10);
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
                found = 1;
                break;
            }
            ++n;
        }
    }

    FreeSplitStringTokens(&names);
    return found;
}

int udaPluginAddError(IDAM_PLUGIN_INTERFACE* plugin_interface, int type, const char* location, int code,
                      const char* msg)
{
    if (plugin_interface == nullptr) {
        return -1;
    }
    return udaAddErrorToStack(&plugin_interface->error_stack, type, location, code, msg);
}

int udaPluginAddErrorF(IDAM_PLUGIN_INTERFACE* plugin_interface, int type, const char* location, int code,
                       const char* fmt, ...)
{
    char msg[STRING_LENGTH];
    va_list args;
    va_start(args, fmt);
    vsnprintf(msg, STRING_LENGTH, fmt, args);
    va_end(args);
    return udaPluginAddError(plugin_interface, type, location, code, msg);
}

bool udaPluginLogEnabled(const IDAM_PLUGIN_INTERFACE* plugin_interface, int level)
{
    // Only a version 2 host provides a logging sink; with an older host plugin logging is a no-op
    return plugin_interface != nullptr && plugin_interface->interfaceVersion >= 2
           && plugin_interface->log_function != nullptr && level >= plugin_interface->log_level;
}

void udaPluginLog(const IDAM_PLUGIN_INTERFACE* plugin_interface, int level, const char* file, int line,
                  const char* fmt, ...)
{
    char msg[STRING_LENGTH];
    va_list args;
    va_start(args, fmt);
    vsnprintf(msg, STRING_LENGTH, fmt, args);
    va_end(args);

    if (udaPluginLogEnabled(plugin_interface, level)) {
        plugin_interface->log_function(plugin_interface->log_context, level, file, line, msg);
    }
}

UDA_DIAGNOSTICS udaPluginDiagnostics(IDAM_PLUGIN_INTERFACE* plugin_interface)
{
    if (plugin_interface == nullptr) {
        return UDA_DIAGNOSTICS{};
    }
    UDA_DIAGNOSTICS diag = udaStackDiagnostics(&plugin_interface->error_stack, nullptr, nullptr, UDA_LOG_NONE);
    if (plugin_interface->interfaceVersion >= 2) {
        diag.log_function = plugin_interface->log_function;
        diag.log_context = plugin_interface->log_context;
        diag.log_level = plugin_interface->log_level;
    }
    return diag;
}

namespace {

// Logging context for a nested call: forwards to the parent's sink, identifying the child plugin.
// Lives on udaCallPlugin()'s stack, so is valid for exactly the duration of the child call.
struct NestedLogContext {
    UDA_PLUGIN_LOG_FUNCTION parent_function;
    void* parent_context;
    const char* child_name;
};

void nested_log(void* context, int level, const char* file, int line, const char* message)
{
    auto nested = static_cast<NestedLogContext*>(context);
    if (nested == nullptr || nested->parent_function == nullptr) {
        return;
    }
    char msg[STRING_LENGTH];
    snprintf(msg, STRING_LENGTH, "[%s] %s", nested->child_name, message != nullptr ? message : "");
    nested->parent_function(nested->parent_context, level, file, line, msg);
}

void add_parent_error(IDAM_PLUGIN_INTERFACE* plugin_interface, int code, const char* msg)
{
    UDA_PLUGIN_LOG(plugin_interface, UDA_LOG_ERROR, "%s\n", msg);
    udaPluginAddError(plugin_interface, UDA_CODE_ERROR_TYPE, "callPlugin", code, msg);
}

} // namespace

int udaCallPlugin(IDAM_PLUGIN_INTERFACE* plugin_interface, const char* signal)
{
    if (plugin_interface == nullptr) {
        return 999;
    }

    const PLUGINLIST* pluginlist = plugin_interface->pluginList;
    if (pluginlist == nullptr || signal == nullptr) {
        add_parent_error(plugin_interface, 999, "No plugin list or request available for the plugin call");
        return 999;
    }

    if (strlen(signal) >= MAXMETA) {
        add_parent_error(plugin_interface, 999, "Plugin call request is too long");
        return 999;
    }

    // The child interface shares the parent's data block, environment and logging context, but starts with an
    // empty error stack so that only its own records are merged back below
    auto child_interface = std::make_unique<IDAM_PLUGIN_INTERFACE>(*plugin_interface);
    child_interface->error_stack.nerrors = 0;

    REQUEST_DATA request = *plugin_interface->request_data;
    child_interface->request_data = &request;

    request.source[0] = '\0';
    strcpy(request.signal, signal);

    // Helper errors are collected on a scratch stack and only passed to the parent if the step fails; traces go
    // straight to the parent's log
    auto scratch = std::make_unique<UDA_ERROR_STACK>();
    scratch->nerrors = 0;
    UDA_DIAGNOSTICS diag = udaPluginDiagnostics(plugin_interface);
    diag.error_context = scratch.get();

    int err = udaMakeRequestData(&diag, &request, *pluginlist, plugin_interface->environment);
    if (err != 0) {
        udaAppendErrorStack(&plugin_interface->error_stack, scratch.get());
        udaPluginAddErrorF(plugin_interface, UDA_CODE_ERROR_TYPE, "callPlugin", err,
                           "Unable to parse plugin call request: %s", signal);
        return err;
    }

    request.request = findPluginRequestByFormat(request.format, pluginlist);
    if (request.request == REQUEST_READ_UNKNOWN) {
        char msg[STRING_LENGTH];
        snprintf(msg, STRING_LENGTH, "Plugin not found for request: %s", signal);
        add_parent_error(plugin_interface, 999, msg);
        return 999;
    }

    int id = findPluginIdByRequest(request.request, pluginlist);
    const PLUGIN_DATA* plugin = id >= 0 ? &pluginlist->plugin[id] : nullptr;
    if (plugin == nullptr || plugin->idamPlugin == nullptr) {
        add_parent_error(plugin_interface, 999, "Data Access is not available for this data request!");
        return 999;
    }

    int registered_version = plugin->interfaceVersion > 0 ? plugin->interfaceVersion : 1;
    if (registered_version > UDA_PLUGIN_INTERFACE_VERSION) {
        char msg[STRING_LENGTH];
        snprintf(msg, STRING_LENGTH, "Plugin %s requires interface version %d but this host supports up to %d",
                 plugin->format, registered_version, UDA_PLUGIN_INTERFACE_VERSION);
        add_parent_error(plugin_interface, 999, msg);
        return 999;
    }
    // The appended (version >= 2) fields are only known to be valid when the parent was given them
    int host_version = plugin_interface->interfaceVersion >= 2 ? UDA_PLUGIN_INTERFACE_VERSION : 1;
    child_interface->interfaceVersion = (unsigned short)std::min(registered_version, host_version);

    NestedLogContext nested_log_context = {};
    if (host_version >= 2 && plugin_interface->log_function != nullptr) {
        nested_log_context.parent_function = plugin_interface->log_function;
        nested_log_context.parent_context = plugin_interface->log_context;
        nested_log_context.child_name = plugin->format;
        child_interface->log_function = nested_log;
        child_interface->log_context = &nested_log_context;
    }

    UDA_PLUGIN_LOG(plugin_interface, UDA_LOG_DEBUG, "Calling plugin %s: %s\n", plugin->format, signal);

    err = plugin->idamPlugin(child_interface.get());    // Call the data reader

    if (err != 0) {
        UDA_PLUGIN_LOG(plugin_interface, UDA_LOG_ERROR, "Plugin %s returned error %d for request: %s\n",
                       plugin->format, err, signal);
        // Guarantee the child's failure carries at least one record of its own
        udaEnsureErrorRecorded(&child_interface->error_stack, err, plugin->format);
    }

    // Child records first (root cause), then any context the parent adds
    udaAppendErrorStack(&plugin_interface->error_stack, &child_interface->error_stack);

    if (err != 0) {
        return err;
    }

    // Apply subsetting
    if (request.datasubset.nbound > 0) {
        ACTION action = {};
        initAction(&action);
        action.actionType = UDA_SUBSET_TYPE;
        action.subset = request.datasubset;
        scratch->nerrors = 0;
        err = udaServerSubsetData(&diag, child_interface->data_block, action, child_interface->logmalloclist);
        if (err != 0) {
            udaAppendErrorStack(&plugin_interface->error_stack, scratch.get());
            char msg[STRING_LENGTH];
            snprintf(msg, STRING_LENGTH, "Subsetting failed for plugin call request: %s", signal);
            add_parent_error(plugin_interface, err, msg);
            return err;
        }
    }

    return 0;
}

int callPlugin(const PLUGINLIST* pluginlist, const char* signal, const IDAM_PLUGIN_INTERFACE* old_plugin_interface)
{
    if (old_plugin_interface == nullptr) {
        return 999;
    }
    // The parent interface is always a writable host object (the child already writes to its data_block); the
    // const qualifier is historical. Errors from the child are merged into the parent's error_stack.
    auto plugin_interface = const_cast<IDAM_PLUGIN_INTERFACE*>(old_plugin_interface);
    if (pluginlist == old_plugin_interface->pluginList) {
        return udaCallPlugin(plugin_interface, signal);
    }
    // Caller supplied a different plugin list: honour it for this call only
    const PLUGINLIST* saved = plugin_interface->pluginList;
    plugin_interface->pluginList = pluginlist;
    int err = udaCallPlugin(plugin_interface, signal);
    plugin_interface->pluginList = saved;
    return err;
}
