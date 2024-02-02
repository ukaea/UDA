#ifndef UDA_PLUGINS_H
#define UDA_PLUGINS_H

#include <uda/export.h>
#include <uda/types.h>

#define UDA_PLUGIN_INTERNAL 0
#define UDA_PLUGIN_EXTERNAL 1 // The plugin resides in an external shared library
#define UDA_PLUGIN_NOT_OPERATIONAL 0
#define UDA_PLUGIN_OPERATIONAL 1

// privacy

#define UDA_PLUGIN_PRIVATE 1 // Only internal users can use the service (access the data!)
#define UDA_PLUGIN_PUBLIC 0  // All users - internal and external - can use the service

#ifdef __cplusplus
extern "C" {
#endif

typedef void (*ADDIDAMERRORFUNP)(UDA_ERROR_STACK*, int, char*, int, char*); // Write to the Error Log

// Prototypes

LIBRARY_API int callPlugin(UDA_PLUGIN_INTERFACE* plugin_interface, const char* request);
LIBRARY_API int callPlugin2(UDA_PLUGIN_INTERFACE* plugin_interface, const char* request, const char* source);

LIBRARY_API int udaPluginIsExternal(UDA_PLUGIN_INTERFACE* plugin_interface);
LIBRARY_API int udaPluginCheckInterfaceVersion(UDA_PLUGIN_INTERFACE* plugin_interface, int interface_version);
LIBRARY_API void udaPluginSetVersion(UDA_PLUGIN_INTERFACE* plugin_interface, int plugin_version);
LIBRARY_API const char* udaPluginFunction(UDA_PLUGIN_INTERFACE* plugin_interface);

LIBRARY_API void udaPluginLog(UDA_PLUGIN_INTERFACE* plugin_interface, const char* fmt, ...);

LIBRARY_API void udaAddPluginError(UDA_PLUGIN_INTERFACE* plugin_interface, const char* location, int code, const char* msg);

LIBRARY_API UDA_PLUGIN_INTERFACE* udaCreatePluginInterface(const char* request);

LIBRARY_API void udaFreePluginInterface(UDA_PLUGIN_INTERFACE* plugin_interface);

LIBRARY_API COMPOUNDFIELD* udaNewCompoundField(const char* name, const char* description, int* offset, int type);
LIBRARY_API COMPOUNDFIELD* udaNewCompoundArrayField(const char* name, const char* description, int* offset, int type, int rank, int* shape);

LIBRARY_API COMPOUNDFIELD* udaNewCompoundUserTypeField(const char* name, const char* description, int* offset, USERDEFINEDTYPE* user_type);
LIBRARY_API COMPOUNDFIELD* udaNewCompoundUserTypePointerField(const char* name, const char* description, int* offset, USERDEFINEDTYPE* user_type);
LIBRARY_API COMPOUNDFIELD* udaNewCompoundUserTypeArrayField(const char* name, const char* description, int* offset, USERDEFINEDTYPE* user_type, int rank, int* shape);

LIBRARY_API USERDEFINEDTYPE* udaNewUserType(const char* name, const char* source, int ref_id, int image_count, char* image, size_t size, size_t num_fields, COMPOUNDFIELD** fields);

LIBRARY_API int udaAddUserType(UDA_PLUGIN_INTERFACE*, USERDEFINEDTYPE* user_type);
LIBRARY_API int udaRegisterMalloc(UDA_PLUGIN_INTERFACE* plugin_interface, void* data, int, size_t, const char*);
LIBRARY_API int udaRegisterMallocArray(UDA_PLUGIN_INTERFACE* plugin_interface, void* data, int count, size_t size, const char* type, int rank, int* shape);

LIBRARY_API int udaPluginPluginsCount(UDA_PLUGIN_INTERFACE* plugin_interface);
LIBRARY_API int udaPluginCheckPluginClass(UDA_PLUGIN_INTERFACE* plugin_interface, int plugin_num, const char* plugin_class);
LIBRARY_API const char* udaPluginPluginFormat(UDA_PLUGIN_INTERFACE* plugin_interface, int plugin_num);
LIBRARY_API const char* udaPluginPluginExtension(UDA_PLUGIN_INTERFACE* plugin_interface, int plugin_num);
LIBRARY_API const char* udaPluginPluginDescription(UDA_PLUGIN_INTERFACE* plugin_interface, int plugin_num);
LIBRARY_API const char* udaPluginPluginExample(UDA_PLUGIN_INTERFACE* plugin_interface, int plugin_num);

LIBRARY_API int setReturnDataLabel(UDA_PLUGIN_INTERFACE* plugin_interface, const char* label);
LIBRARY_API int setReturnDataUnits(UDA_PLUGIN_INTERFACE* plugin_interface, const char* units);

LIBRARY_API int setReturnDataFloatArray(UDA_PLUGIN_INTERFACE* plugin_interface, float* values, size_t rank, const size_t* shape,
                                        const char* description);
LIBRARY_API int setReturnDataDoubleArray(UDA_PLUGIN_INTERFACE* plugin_interface, double* values, size_t rank, const size_t* shape,
                                         const char* description);
LIBRARY_API int setReturnDataCharArray(UDA_PLUGIN_INTERFACE* plugin_interface, const char* values, size_t rank, int *shape,
                                       const char* description);
LIBRARY_API int setReturnDataIntArray(UDA_PLUGIN_INTERFACE* plugin_interface, int* values, size_t rank, const size_t* shape,
                                      const char* description);
LIBRARY_API int setReturnDataDoubleScalar(UDA_PLUGIN_INTERFACE* plugin_interface, double value, const char* description);
LIBRARY_API int setReturnDataFloatScalar(UDA_PLUGIN_INTERFACE* plugin_interface, float value, const char* description);
LIBRARY_API int setReturnDataIntScalar(UDA_PLUGIN_INTERFACE* plugin_interface, int value, const char* description);
LIBRARY_API int setReturnDataLongScalar(UDA_PLUGIN_INTERFACE* plugin_interface, long value, const char* description);
LIBRARY_API int setReturnDataShortScalar(UDA_PLUGIN_INTERFACE* plugin_interface, short value, const char* description);
LIBRARY_API int setReturnDataString(UDA_PLUGIN_INTERFACE* plugin_interface, const char* value, const char* description);

LIBRARY_API int setReturnData(UDA_PLUGIN_INTERFACE* plugin_interface, void* value, size_t size, UDA_TYPE type, int rank,
                              const int* shape, const char* description);

LIBRARY_API int setReturnDimensionFloatArray(UDA_PLUGIN_INTERFACE* plugin_interface, int dim_n, float* data, size_t size, const char* label, const char* units);

LIBRARY_API int setReturnErrorAsymmetry(UDA_PLUGIN_INTERFACE* plugin_interface, bool flag);
LIBRARY_API int setReturnErrorLow(UDA_PLUGIN_INTERFACE* plugin_interface, float* data, size_t size);
LIBRARY_API int setReturnErrorHigh(UDA_PLUGIN_INTERFACE* plugin_interface, float* data, size_t size);
LIBRARY_API int setReturnDataOrder(UDA_PLUGIN_INTERFACE* plugin_interface, int order);

LIBRARY_API int setReturnCompoundData(UDA_PLUGIN_INTERFACE* plugin_interface, char* data, const char* user_type, const char* description);
LIBRARY_API int setReturnCompoundArrayData(UDA_PLUGIN_INTERFACE *plugin_interface, char* data, const char *user_type, const char* description, int rank, int* shape);

LIBRARY_API int udaPluginArgumentCount(const UDA_PLUGIN_INTERFACE* plugin_interface);
LIBRARY_API const char* udaPluginArgument(const UDA_PLUGIN_INTERFACE* plugin_interface, int num);

LIBRARY_API bool findStringValue(const UDA_PLUGIN_INTERFACE* plugin_interface, const char** value, const char* name);
LIBRARY_API bool findValue(const UDA_PLUGIN_INTERFACE* plugin_interface, const char* name);
LIBRARY_API bool findIntValue(const UDA_PLUGIN_INTERFACE* plugin_interface, int* value, const char* name);
LIBRARY_API bool findShortValue(const UDA_PLUGIN_INTERFACE* plugin_interface, short* value, const char* name);
LIBRARY_API bool findCharValue(const UDA_PLUGIN_INTERFACE* plugin_interface, char* value, const char* name);
LIBRARY_API bool findFloatValue(const UDA_PLUGIN_INTERFACE* plugin_interface, float* values, const char* name);
LIBRARY_API bool findIntArray(const UDA_PLUGIN_INTERFACE* plugin_interface, int** values, size_t* nvalues, const char* name);
LIBRARY_API bool findFloatArray(const UDA_PLUGIN_INTERFACE* plugin_interface, float** values, size_t* nvalues, const char* name);
LIBRARY_API bool findDoubleArray(const UDA_PLUGIN_INTERFACE* plugin_interface, double** values, size_t* nvalues,
                                 const char* name);

#define QUOTE_(X) #X
#define QUOTE(X) QUOTE_(X)
#define CONCAT_(X, Y) X##Y
#define CONCAT(X, Y) CONCAT_(X, Y)
#define UNIQUE_VAR(NAME) __func__##NAME##__

#define RAISE_PLUGIN_ERROR_AND_EXIT(PLUGIN_INTERFACE, MSG)                                                             \
    {                                                                                                                  \
        int UNIQUE_VAR(err) = 999;                                                                                     \
        udaPluginLog("%s\n", MSG);                                                                                     \
        udaAddPluginError(__func__, UNIQUE_VAR(err), MSG);                                                             \
        return UNIQUE_VAR(err);                                                                                        \
    }

#define RAISE_PLUGIN_ERROR(PLUGIN_INTERFACE, MSG)                                                                      \
    {                                                                                                                  \
        int UNIQUE_VAR(err) = 999;                                                                                     \
        udaPluginLog(PLUGIN_INTERFACE, "%s\n", MSG);                                                                           \
        udaAddPluginError(PLUGIN_INTERFACE, __func__, UNIQUE_VAR(err), MSG);                                           \
        return UNIQUE_VAR(err);                                                                                        \
    }

#define RAISE_PLUGIN_ERROR_F(PLUGIN_INTERFACE, MSG, FMT, ...)                                                          \
    {                                                                                                                  \
        int UNIQUE_VAR(err) = 999;                                                                                     \
        udaPluginLog("%s\n", FMT, __VA_ARGS__);                                                                        \
        udaAddPluginError(PLUGIN_INTERFACE, __func__, UNIQUE_VAR(err), MSG);                                           \
        return UNIQUE_VAR(err);                                                                                        \
    }

#define RAISE_PLUGIN_ERROR_AND_EXIT_F(PLUGIN_INTERFACE, MSG, FMT, ...)                                                 \
    {                                                                                                                  \
        int UNIQUE_VAR(err) = 999;                                                                                     \
        udaPluginLog("%s\n", FMT, __VA_ARGS__);                                                                        \
        udaAddPluginError(PLUGIN_INTERFACE, __func__, UNIQUE_VAR(err), MSG);                                           \
        return UNIQUE_VAR(err);                                                                                        \
    }

#define RAISE_PLUGIN_ERROR_EX(PLUGIN_INTERFACE, MSG, CODE)                                                             \
    int UNIQUE_VAR(err) = 999;                                                                                         \
    udaPluginLog("%s", MSG);                                                                                           \
    udaAddPluginError(PLUGIN_INTERFACE, __func__, UNIQUE_VAR(err), MSG);                                               \
    { CODE }                                                                                                           \
    return UNIQUE_VAR(err);

#define FIND_REQUIRED_VALUE(PLUGIN_INTERFACE, VARIABLE, TYPE)                                                          \
    if (!find##TYPE##Value(PLUGIN_INTERFACE, &VARIABLE, QUOTE(VARIABLE))) {                                            \
        RAISE_PLUGIN_ERROR(PLUGIN_INTERFACE, "Required argument '" QUOTE(VARIABLE) "' not given");                     \
    }

#define FIND_REQUIRED_ARRAY(PLUGIN_INTERFACE, VARIABLE, TYPE)                                                          \
    if (!find##TYPE##Array(PLUGIN_INTERFACE, &VARIABLE, CONCAT(&n, VARIABLE), QUOTE(VARIABLE))) {                      \
        RAISE_PLUGIN_ERROR(PLUGIN_INTERFACE, "Required argument '" QUOTE(VARIABLE) "' not given");                     \
    }

#define FIND_REQUIRED_INT_VALUE(PLUGIN_INTERFACE, VARIABLE) FIND_REQUIRED_VALUE(PLUGIN_INTERFACE, VARIABLE, Int)
#define FIND_REQUIRED_SHORT_VALUE(PLUGIN_INTERFACE, VARIABLE) FIND_REQUIRED_VALUE(PLUGIN_INTERFACE, VARIABLE, Short)
#define FIND_REQUIRED_CHAR_VALUE(PLUGIN_INTERFACE, VARIABLE) FIND_REQUIRED_VALUE(PLUGIN_INTERFACE, VARIABLE, Char)
#define FIND_REQUIRED_FLOAT_VALUE(PLUGIN_INTERFACE, VARIABLE) FIND_REQUIRED_VALUE(PLUGIN_INTERFACE, VARIABLE, Float)
#define FIND_REQUIRED_STRING_VALUE(PLUGIN_INTERFACE, VARIABLE) FIND_REQUIRED_VALUE(PLUGIN_INTERFACE, VARIABLE, String)

#define FIND_REQUIRED_INT_ARRAY(PLUGIN_INTERFACE, VARIABLE) FIND_REQUIRED_ARRAY(PLUGIN_INTERFACE, VARIABLE, Int)
#define FIND_REQUIRED_FLOAT_ARRAY(PLUGIN_INTERFACE, VARIABLE) FIND_REQUIRED_ARRAY(PLUGIN_INTERFACE, VARIABLE, Float)
#define FIND_REQUIRED_DOUBLE_ARRAY(PLUGIN_INTERFACE, VARIABLE) FIND_REQUIRED_ARRAY(PLUGIN_INTERFACE, VARIABLE, Double)

#define FIND_INT_VALUE(PLUGIN_INTERFACE, VARIABLE) findIntValue(PLUGIN_INTERFACE, &VARIABLE, QUOTE(VARIABLE))
#define FIND_SHORT_VALUE(PLUGIN_INTERFACE, VARIABLE) findShortValue(PLUGIN_INTERFACE, &VARIABLE, QUOTE(VARIABLE))
#define FIND_CHAR_VALUE(PLUGIN_INTERFACE, VARIABLE) findCharValue(PLUGIN_INTERFACE, &VARIABLE, QUOTE(VARIABLE))
#define FIND_FLOAT_VALUE(PLUGIN_INTERFACE, VARIABLE) findFloatValue(PLUGIN_INTERFACE, &VARIABLE, QUOTE(VARIABLE))
#define FIND_STRING_VALUE(PLUGIN_INTERFACE, VARIABLE) findStringValue(PLUGIN_INTERFACE, &VARIABLE, QUOTE(VARIABLE))

#define FIND_INT_ARRAY(PLUGIN_INTERFACE, VARIABLE)                                                                      \
    findIntArray(PLUGIN_INTERFACE, &VARIABLE, CONCAT(&n, VARIABLE), QUOTE(VARIABLE))
#define FIND_FLOAT_ARRAY(PLUGIN_INTERFACE, VARIABLE)                                                                    \
    findFloatArray(PLUGIN_INTERFACE, &VARIABLE, CONCAT(&n, VARIABLE), QUOTE(VARIABLE))
#define FIND_DOUBLE_ARRAY(PLUGIN_INTERFACE, VARIABLE)                                                                   \
    findDoubleArray(PLUGIN_INTERFACE, &VARIABLE, CONCAT(&n, VARIABLE), QUOTE(VARIABLE))

#define CALL_PLUGIN(PLUGIN_INTERFACE, FMT, ...)                                                                        \
    {                                                                                                                  \
        char UNIQUE_VAR(request)[1024];                                                                                \
        snprintf(UNIQUE_VAR(request), 1024, FMT, __VA_ARGS__);                                                         \
        UNIQUE_VAR(request)[1023] = '\0';                                                                              \
        int UNIQUE_VAR(err) = callPlugin(PLUGIN_INTERFACE, UNIQUE_VAR(request));         \
        if (UNIQUE_VAR(err)) {                                                                                         \
            RAISE_PLUGIN_ERROR("Plugin call failed");                                                                  \
        }                                                                                                              \
    }

#ifdef __cplusplus
}
#endif

#endif // UDA_PLUGINS_H