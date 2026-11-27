#ifndef UDA_PLUGINS_UDAPLUGIN_H
#define UDA_PLUGINS_UDAPLUGIN_H

#include <stdbool.h>

#include <clientserver/udaStructs.h>
#include <logging/logging.h>
#include <clientserver/errorLog.h>
#include <clientserver/udaDiagnostics.h>
#include <plugins/pluginStructs.h>
#include <clientserver/export.h>
#include "clientserver/udaTypes.h"

#ifdef __cplusplus
extern "C" {
#endif

#define MAXFUNCTIONNAME     256

// plugin State

#define UDA_PLUGIN_INTERNAL          0
#define UDA_PLUGIN_EXTERNAL          1           // The plugin resides in an external shared library
#define UDA_PLUGIN_NOT_OPERATIONAL   0
#define UDA_PLUGIN_OPERATIONAL       1

// privacy

#define UDA_PLUGIN_PRIVATE       1           // Only internal users can use the service (access the data!)
#define UDA_PLUGIN_PUBLIC        0           // All users - internal and external - can use the service

typedef void (* ADDIDAMERRORFUNP)(UDA_ERROR_STACK*, int, char*, int, char*);   // Write to the Error Log

#if defined(__GNUC__) || defined(__clang__)
#  define UDA_PLUGIN_PRINTF_FORMAT(FMT_INDEX, ARGS_INDEX) __attribute__((format(printf, FMT_INDEX, ARGS_INDEX)))
#else
#  define UDA_PLUGIN_PRINTF_FORMAT(FMT_INDEX, ARGS_INDEX)
#endif

// Prototypes

/**
 * Call another plugin from within a plugin.
 *
 * The child runs with an empty error stack and the parent's logging context. Child error records are appended,
 * in order, to the parent's error_stack (the parent interface object is the host's and is written to even though
 * the pointer is declared const, as the child already writes to the parent's data_block). A child failure is
 * returned before any subsetting is attempted.
 */
LIBRARY_API int callPlugin(const PLUGINLIST* pluginlist, const char* request, const IDAM_PLUGIN_INTERFACE* old_plugin_interface);

/**
 * Writable form of callPlugin(), preferred for new code.
 */
LIBRARY_API int udaCallPlugin(IDAM_PLUGIN_INTERFACE* plugin_interface, const char* request);

//--------------------------------------------------------------------------------------------------------------------
// Error reporting through the plugin interface
//
// These write directly to plugin_interface->error_stack, which is how errors reach the server. They never touch
// library-global error state, so they work regardless of which UDA library instance a plugin is bound to.

/**
 * Append an error record to the interface's error stack.
 * @return 0 on success, nonzero if the record could not be stored (logging and return codes should not depend on it)
 */
LIBRARY_API int udaPluginAddError(IDAM_PLUGIN_INTERFACE* plugin_interface, int type, const char* location, int code,
                                  const char* msg);
LIBRARY_API int udaPluginAddErrorF(IDAM_PLUGIN_INTERFACE* plugin_interface, int type, const char* location, int code,
                                   const char* fmt, ...) UDA_PLUGIN_PRINTF_FORMAT(5, 6);

//--------------------------------------------------------------------------------------------------------------------
// Logging through the plugin interface
//
// With interface version >= 2 and a host log_function, diagnostics go to the host's configured sinks (never to the
// protocol streams). Under an older host plugin logging is a no-op.

LIBRARY_API bool udaPluginLogEnabled(const IDAM_PLUGIN_INTERFACE* plugin_interface, int level);
LIBRARY_API void udaPluginLog(const IDAM_PLUGIN_INTERFACE* plugin_interface, int level, const char* file, int line,
                              const char* fmt, ...) UDA_PLUGIN_PRINTF_FORMAT(5, 6);

/**
 * A diagnostics context for the shared helpers that take one (udaMakeRequestData(), udaExpandEnvironmentVariables(),
 * udaServerSubsetData(), udaPrintDataBlock(), ...): their errors are recorded on plugin_interface->error_stack and
 * their traces go to the host's log. The context points into plugin_interface, so use it only during this call.
 */
LIBRARY_API UDA_DIAGNOSTICS udaPluginDiagnostics(IDAM_PLUGIN_INTERFACE* plugin_interface);

// udaCreatePluginInterface() / udaFreePluginInterface() create a full host environment (server environment and
// plugin list) and so are host functions: they are declared in server/pluginInterface.h and live in the server.

LIBRARY_API int findPluginIdByRequest(int request, const PLUGINLIST* plugin_list);
LIBRARY_API int findPluginIdByFormat(const char* format, const PLUGINLIST* plugin_list);
LIBRARY_API int findPluginIdByDevice(const char* device, const PLUGINLIST* plugin_list);
LIBRARY_API int findPluginRequestByFormat(const char* format, const PLUGINLIST* plugin_list);
LIBRARY_API int findPluginRequestByExtension(const char* extension, const PLUGINLIST* plugin_list);

LIBRARY_API int setReturnDataFloatArray(DATA_BLOCK* data_block, float* values, size_t rank,
                                        const size_t* shape, const char* description);
LIBRARY_API int setReturnDataDoubleArray(DATA_BLOCK* data_block, double* values, size_t rank,
                                        const size_t* shape, const char* description);
LIBRARY_API int setReturnDataIntArray(DATA_BLOCK* data_block, int* values, size_t rank, 
                                      const size_t* shape, const char* description);
LIBRARY_API int setReturnDataIShortArray(DATA_BLOCK* data_block, short* values, size_t rank, 
                                         const size_t* shape, const char* description);
LIBRARY_API int setReturnDataLongArray(DATA_BLOCK* data_block, long* values, size_t rank, 
                                      const size_t* shape, const char* description);
LIBRARY_API int setReturnDataUnsignedIntArray(DATA_BLOCK* data_block, unsigned int* values, size_t rank, 
                                              const size_t* shape, const char* description);
LIBRARY_API int setReturnDataUnsignedShortArray(DATA_BLOCK* data_block, unsigned short* values, size_t rank, 
                                                const size_t* shape, const char* description);
LIBRARY_API int setReturnDataUnsignedLongArray(DATA_BLOCK* data_block, unsigned long* values, size_t rank, 
                                               const size_t* shape, const char* description);
LIBRARY_API int setReturnDataUnsignedCharArray(DATA_BLOCK* data_block, unsigned char* values, size_t rank, 
                                               const size_t* shape, const char* description);

LIBRARY_API int setReturnDataCharScalar(DATA_BLOCK* data_block, char value, const char* description);
LIBRARY_API int setReturnDataShortScalar(DATA_BLOCK* data_block, short value, const char* description);
LIBRARY_API int setReturnDataIntScalar(DATA_BLOCK* data_block, int value, const char* description);
LIBRARY_API int setReturnDataLongScalar(DATA_BLOCK* data_block, long value, const char* description);
LIBRARY_API int setReturnDataFloatScalar(DATA_BLOCK* data_block, float value, const char* description);
LIBRARY_API int setReturnDataDoubleScalar(DATA_BLOCK* data_block, double value, const char* description);
LIBRARY_API int setReturnDataUnsignedCharScalar(DATA_BLOCK* data_block, unsigned char value,
                                                const char* description);
LIBRARY_API int setReturnDataUnsignedShortScalar(DATA_BLOCK* data_block, unsigned short value,
                                                 const char* description);
LIBRARY_API int setReturnDataUnsignedIntScalar(DATA_BLOCK* data_block, int value,
                                               const char* description);
LIBRARY_API int setReturnDataUnsignedLongScalar(DATA_BLOCK* data_block, unsigned long value,
                                                const char* description);

LIBRARY_API int setReturnDataString(DATA_BLOCK* data_block, const char* value, const char* description);
LIBRARY_API int setReturnData(DATA_BLOCK* data_block, void* value, size_t size, UDA_TYPE type, int rank, const int* shape, const char* description);

LIBRARY_API bool findStringValue(const NAMEVALUELIST* namevaluelist, const char** value, const char* name);
LIBRARY_API bool findValue(const NAMEVALUELIST* namevaluelist, const char* name);
LIBRARY_API bool findIntValue(const NAMEVALUELIST* namevaluelist, int* value, const char* name);
LIBRARY_API bool findLongIntValue(const NAMEVALUELIST* namevaluelist, long* value, const char* name);
LIBRARY_API bool findUnsignedLongValue(const NAMEVALUELIST* namevaluelist, unsigned long* value, const char* name);
LIBRARY_API bool findShortValue(const NAMEVALUELIST* namevaluelist, short* value, const char* name);
LIBRARY_API bool findCharValue(const NAMEVALUELIST* namevaluelist, char* value, const char* name);
LIBRARY_API bool findFloatValue(const NAMEVALUELIST* namevaluelist, float* values, const char* name);
LIBRARY_API bool findIntArray(const NAMEVALUELIST* namevaluelist, int** values, size_t* nvalues, const char* name);
LIBRARY_API bool findFloatArray(const NAMEVALUELIST* namevaluelist, float** values, size_t* nvalues, const char* name);
LIBRARY_API bool findDoubleArray(const NAMEVALUELIST* namevaluelist, double** values, size_t* nvalues, const char* name);

LIBRARY_API unsigned int bitEncodeSemanticVersionNumber(const char* version);

#define QUOTE_(X) #X
#define QUOTE(X) QUOTE_(X)
#define CONCAT_(X, Y) X##Y
#define CONCAT(X, Y) CONCAT_(X, Y)
#define UNIQUE_VAR(NAME) __func__##NAME##__

#ifdef _WIN32
#  define UDA_PLUGIN_FILENAME (strrchr(__FILE__, '\\') ? strrchr(__FILE__, '\\') + 1 : __FILE__)
#else
#  define UDA_PLUGIN_FILENAME (strrchr(__FILE__, '/') ? strrchr(__FILE__, '/') + 1 : __FILE__)
#endif

#define UDA_PLUGIN_LOG(PLUGIN_INTERFACE, LEVEL, FMT, ...) \
do { if (udaPluginLogEnabled(PLUGIN_INTERFACE, LEVEL)) { \
    udaPluginLog(PLUGIN_INTERFACE, LEVEL, UDA_PLUGIN_FILENAME, __LINE__, FMT, ##__VA_ARGS__); \
} } while (0)

#define UDA_PLUGIN_ADD_ERROR(PLUGIN_INTERFACE, CODE, MSG) \
    udaPluginAddError(PLUGIN_INTERFACE, UDA_CODE_ERROR_TYPE, __func__, CODE, MSG)

#define UDA_PLUGIN_ADD_SYS_ERROR(PLUGIN_INTERFACE, ERRNO, MSG) \
    udaPluginAddError(PLUGIN_INTERFACE, UDA_SYSTEM_ERROR_TYPE, __func__, ERRNO, MSG)

// Log, record and return CODE from the current function
#define UDA_PLUGIN_RAISE_ERROR_CODE(PLUGIN_INTERFACE, CODE, MSG) \
do { int uda_plugin_err_ = (CODE); \
    const char* uda_plugin_msg_ = (MSG); \
    UDA_PLUGIN_LOG(PLUGIN_INTERFACE, UDA_LOG_ERROR, "%s\n", uda_plugin_msg_); \
    udaPluginAddError(PLUGIN_INTERFACE, UDA_CODE_ERROR_TYPE, __func__, uda_plugin_err_, uda_plugin_msg_); \
    return uda_plugin_err_; } while (0)

#define UDA_PLUGIN_RAISE_ERROR(PLUGIN_INTERFACE, MSG) UDA_PLUGIN_RAISE_ERROR_CODE(PLUGIN_INTERFACE, 999, MSG)

#define UDA_PLUGIN_RAISE_ERROR_F(PLUGIN_INTERFACE, FMT, ...) \
do { char uda_plugin_fmsg_[STRING_LENGTH]; \
    snprintf(uda_plugin_fmsg_, STRING_LENGTH, FMT, __VA_ARGS__); \
    UDA_PLUGIN_RAISE_ERROR(PLUGIN_INTERFACE, uda_plugin_fmsg_); } while (0)

//--------------------------------------------------------------------------------------------------------------------
// Legacy error macros
//
// By default these record into library-global error state, which may not be the instance the server reads (e.g. on
// macOS or Windows where each library has its own copy); the server then reports a fallback error carrying only the
// return code. Defining UDA_PLUGIN_INTERFACE_VAR (the name of the IDAM_PLUGIN_INTERFACE* in scope) before including
// this header routes them, and the FIND_REQUIRED_* macros, through that interface instead.

#ifdef UDA_PLUGIN_INTERFACE_VAR

#define RAISE_PLUGIN_ERROR_AND_EXIT(MSG, plugin_interface_ptr) UDA_PLUGIN_RAISE_ERROR(plugin_interface_ptr, MSG)
#define RAISE_PLUGIN_ERROR(MSG) UDA_PLUGIN_RAISE_ERROR(UDA_PLUGIN_INTERFACE_VAR, MSG)
#define RAISE_PLUGIN_ERROR_F(MSG, FMT, ...) \
do { UDA_PLUGIN_LOG(UDA_PLUGIN_INTERFACE_VAR, UDA_LOG_ERROR, FMT, __VA_ARGS__); \
    udaPluginAddError(UDA_PLUGIN_INTERFACE_VAR, UDA_CODE_ERROR_TYPE, __func__, 999, MSG); \
    return 999; } while (0)
#define RAISE_PLUGIN_ERROR_AND_EXIT_F(plugin_interface_ptr, MSG, FMT, ...) \
do { UDA_PLUGIN_LOG(plugin_interface_ptr, UDA_LOG_ERROR, FMT, __VA_ARGS__); \
    udaPluginAddError(plugin_interface_ptr, UDA_CODE_ERROR_TYPE, __func__, 999, MSG); \
    return 999; } while (0)
#define RAISE_PLUGIN_ERROR_EX(MSG, CODE) \
UDA_PLUGIN_LOG(UDA_PLUGIN_INTERFACE_VAR, UDA_LOG_ERROR, "%s", MSG); \
udaPluginAddError(UDA_PLUGIN_INTERFACE_VAR, UDA_CODE_ERROR_TYPE, __func__, 999, MSG); \
CODE; \
return 999;

#else

#define RAISE_PLUGIN_ERROR_AND_EXIT(MSG, plugin_interface_ptr) \
{ int UNIQUE_VAR(err) = 999; \
UDA_LOG(UDA_LOG_ERROR, "%s\n", MSG); \
addIdamError(UDA_CODE_ERROR_TYPE, __func__, UNIQUE_VAR(err), MSG); \
concatUdaError(&plugin_interface_ptr->error_stack); \
return UNIQUE_VAR(err); }

#define RAISE_PLUGIN_ERROR(MSG) \
{ int UNIQUE_VAR(err) = 999; \
UDA_LOG(UDA_LOG_ERROR, "%s\n", MSG); \
addIdamError(UDA_CODE_ERROR_TYPE, __func__, UNIQUE_VAR(err), MSG); \
return UNIQUE_VAR(err); }

#define RAISE_PLUGIN_ERROR_F(MSG, FMT, ...) \
{ int UNIQUE_VAR(err) = 999; \
UDA_LOG(UDA_LOG_ERROR, FMT, __VA_ARGS__); \
addIdamError(UDA_CODE_ERROR_TYPE, __func__, UNIQUE_VAR(err), MSG); \
return UNIQUE_VAR(err); }

#define RAISE_PLUGIN_ERROR_AND_EXIT_F(plugin_interface_ptr, MSG, FMT, ...) \
{ int UNIQUE_VAR(err) = 999; \
UDA_LOG(UDA_LOG_ERROR, FMT, __VA_ARGS__); \
addIdamError(UDA_CODE_ERROR_TYPE, __func__, UNIQUE_VAR(err), MSG); \
concatUdaError(&plugin_interface_ptr->error_stack); \
return UNIQUE_VAR(err); }

#define RAISE_PLUGIN_ERROR_EX(MSG, CODE) \
int UNIQUE_VAR(err) = 999; \
UDA_LOG(UDA_LOG_ERROR, "%s", MSG); \
addIdamError(UDA_CODE_ERROR_TYPE, __func__, UNIQUE_VAR(err), MSG); \
CODE; \
return UNIQUE_VAR(err);

#endif // UDA_PLUGIN_INTERFACE_VAR

#define FIND_REQUIRED_VALUE(NAME_VALUE_LIST, VARIABLE, TYPE) \
if (!find##TYPE##Value(&NAME_VALUE_LIST, &VARIABLE, QUOTE(VARIABLE))) { \
    RAISE_PLUGIN_ERROR("Required argument '" QUOTE(VARIABLE) "' not given"); \
}

#define FIND_REQUIRED_ARRAY(NAME_VALUE_LIST, VARIABLE, TYPE) \
if (!find##TYPE##Array(&NAME_VALUE_LIST, &VARIABLE, CONCAT(&n, VARIABLE), QUOTE(VARIABLE))) { \
    RAISE_PLUGIN_ERROR("Required argument '" QUOTE(VARIABLE) "' not given"); \
}

#define FIND_REQUIRED_INT_VALUE(NAME_VALUE_LIST, VARIABLE)      FIND_REQUIRED_VALUE(NAME_VALUE_LIST, VARIABLE, Int)
#define FIND_REQUIRED_LONG_VALUE(NAME_VALUE_LIST, VARIABLE)      FIND_REQUIRED_VALUE(NAME_VALUE_LIST, VARIABLE, LongInt)
#define FIND_REQUIRED_UNSIGNED_LONG_VALUE(NAME_VALUE_LIST, VARIABLE)      FIND_REQUIRED_VALUE(NAME_VALUE_LIST, VARIABLE, UnsignedLong)
#define FIND_REQUIRED_SHORT_VALUE(NAME_VALUE_LIST, VARIABLE)    FIND_REQUIRED_VALUE(NAME_VALUE_LIST, VARIABLE, Short)
#define FIND_REQUIRED_CHAR_VALUE(NAME_VALUE_LIST, VARIABLE)     FIND_REQUIRED_VALUE(NAME_VALUE_LIST, VARIABLE, Char)
#define FIND_REQUIRED_FLOAT_VALUE(NAME_VALUE_LIST, VARIABLE)    FIND_REQUIRED_VALUE(NAME_VALUE_LIST, VARIABLE, Float)
#define FIND_REQUIRED_STRING_VALUE(NAME_VALUE_LIST, VARIABLE)   FIND_REQUIRED_VALUE(NAME_VALUE_LIST, VARIABLE, String)

#define FIND_REQUIRED_INT_ARRAY(NAME_VALUE_LIST, VARIABLE)      FIND_REQUIRED_ARRAY(NAME_VALUE_LIST, VARIABLE, Int)
#define FIND_REQUIRED_FLOAT_ARRAY(NAME_VALUE_LIST, VARIABLE)    FIND_REQUIRED_ARRAY(NAME_VALUE_LIST, VARIABLE, Float)
#define FIND_REQUIRED_DOUBLE_ARRAY(NAME_VALUE_LIST, VARIABLE)    FIND_REQUIRED_ARRAY(NAME_VALUE_LIST, VARIABLE, Double)

#define FIND_INT_VALUE(NAME_VALUE_LIST, VARIABLE)       findIntValue(&NAME_VALUE_LIST, &VARIABLE, QUOTE(VARIABLE))
#define FIND_LONG_VALUE(NAME_VALUE_LIST, VARIABLE)       findLongIntValue(&NAME_VALUE_LIST, &VARIABLE, QUOTE(VARIABLE))
#define FIND_UNSIGNED_LONG_VALUE(NAME_VALUE_LIST, VARIABLE)       findUnsignedLongValue(&NAME_VALUE_LIST, &VARIABLE, QUOTE(VARIABLE))
#define FIND_SHORT_VALUE(NAME_VALUE_LIST, VARIABLE)     findShortValue(&NAME_VALUE_LIST, &VARIABLE, QUOTE(VARIABLE))
#define FIND_CHAR_VALUE(NAME_VALUE_LIST, VARIABLE)      findCharValue(&NAME_VALUE_LIST, &VARIABLE, QUOTE(VARIABLE))
#define FIND_FLOAT_VALUE(NAME_VALUE_LIST, VARIABLE)     findFloatValue(&NAME_VALUE_LIST, &VARIABLE, QUOTE(VARIABLE))
#define FIND_STRING_VALUE(NAME_VALUE_LIST, VARIABLE)    findStringValue(&NAME_VALUE_LIST, &VARIABLE, QUOTE(VARIABLE))

#define FIND_INT_ARRAY(NAME_VALUE_LIST, VARIABLE)       findIntArray(&NAME_VALUE_LIST, &VARIABLE, CONCAT(&n, VARIABLE), QUOTE(VARIABLE))
#define FIND_FLOAT_ARRAY(NAME_VALUE_LIST, VARIABLE)     findFloatArray(&NAME_VALUE_LIST, &VARIABLE, CONCAT(&n, VARIABLE), QUOTE(VARIABLE))
#define FIND_DOUBLE_ARRAY(NAME_VALUE_LIST, VARIABLE)     findDoubleArray(&NAME_VALUE_LIST, &VARIABLE, CONCAT(&n, VARIABLE), QUOTE(VARIABLE))

// Call another plugin; on failure the child's error records are kept and a context record is added after them
#define CALL_PLUGIN(PLUGIN_INTERFACE, FMT, ...) \
{ char UNIQUE_VAR(request)[1024]; \
snprintf(UNIQUE_VAR(request), 1024, FMT, __VA_ARGS__); \
UNIQUE_VAR(request)[1023] = '\0'; \
int UNIQUE_VAR(err) = udaCallPlugin(PLUGIN_INTERFACE, UNIQUE_VAR(request)); \
if (UNIQUE_VAR(err)) { \
    UDA_PLUGIN_LOG(PLUGIN_INTERFACE, UDA_LOG_ERROR, "Plugin call failed: %s\n", UNIQUE_VAR(request)); \
    udaPluginAddErrorF(PLUGIN_INTERFACE, UDA_CODE_ERROR_TYPE, __func__, UNIQUE_VAR(err), \
                       "Plugin call failed: %s", UNIQUE_VAR(request)); \
    return UNIQUE_VAR(err); \
} }

#ifdef __cplusplus
}
#endif

#endif // UDA_PLUGINS_UDAPLUGIN_H
