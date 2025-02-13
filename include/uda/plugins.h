#ifndef UDA_PLUGINS_H
#define UDA_PLUGINS_H

#include <uda/export.h>
#include <uda/types.h>

#define UDA_PLUGIN_INFO_FUNCTION_NAME udaPluginInfo

// privacy

#define UDA_PLUGIN_PRIVATE 1 // Only internal users can use the service (access the data!)
#define UDA_PLUGIN_PUBLIC 0  // All users - internal and external - can use the service

#ifdef __cplusplus
extern "C" {
#endif

typedef enum UdaPluginClass {
    UDA_PLUGIN_CLASS_UNKNOWN,
    UDA_PLUGIN_CLASS_FILE,     // File format access
    UDA_PLUGIN_CLASS_SERVER,   // Server protocol access
    UDA_PLUGIN_CLASS_FUNCTION, // Server-side function transformation
    UDA_PLUGIN_CLASS_DEVICE,   // Server to Server chaining, i.e. Pass the request to an external server
    UDA_PLUGIN_CLASS_OTHER
} UDA_PLUGIN_CLASS;

typedef enum UdaPluginCacheMode {
    UDA_PLUGIN_CACHE_MODE_NONE,
    UDA_PLUGIN_CACHE_MODE_OK,
    UDA_PLUGIN_CACHE_MODE_REQUEST,
} UDA_PLUGIN_CACHE_MODE;

typedef struct UdaPluginInfo {
    const char* name;
    const char* version;
    const char* entry_function;
    UDA_PLUGIN_CLASS type;
    const char* extension;
    const char* default_method;
    const char* description;
    UDA_PLUGIN_CACHE_MODE cache_mode;
    bool is_private;
    int interface_version;
} UDA_PLUGIN_INFO;

typedef int (*UDA_PLUGIN_ENTRY_FUNC)(UDA_PLUGIN_INTERFACE*); // Plugin function type

#define UDA_PLUGIN_LOG(INTERFACE, MGS) udaPluginLog(INTERFACE, __FILE__, __LINE__, MGS);
#define UDA_PLUGIN_LOG_S(INTERFACE, FMT, ARG) udaPluginLog_s(INTERFACE, __FILE__, __LINE__, FMT, ARG);
#define UDA_PLUGIN_LOG_I(INTERFACE, FMT, ARG) udaPluginLog_i(INTERFACE, __FILE__, __LINE__, FMT, ARG);

// Prototypes

#define UDA_MAX_PATH 1024 // Same as StringLength

LIBRARY_API void udaSetMetadata(UDA_PLUGIN_INTERFACE* plugin_interface, const char* key, const char* value);
LIBRARY_API const char* udaGetMetadata(UDA_PLUGIN_INTERFACE* plugin_interface, const char* key);
LIBRARY_API bool udaHasMetadata(UDA_PLUGIN_INTERFACE* plugin_interface, const char* key);

LIBRARY_API void udaExpandEnvironmentalVariables(char* path);
LIBRARY_API int udaCallPlugin(UDA_PLUGIN_INTERFACE* plugin_interface, const char* request);
LIBRARY_API int udaCallPlugin2(UDA_PLUGIN_INTERFACE* plugin_interface, const char* request, const char* source);

LIBRARY_API int udaPluginIsExternal(UDA_PLUGIN_INTERFACE* plugin_interface);
LIBRARY_API int udaPluginCheckInterfaceVersion(UDA_PLUGIN_INTERFACE* plugin_interface, int interface_version);
LIBRARY_API int udaPluginGetVersion(UDA_PLUGIN_INTERFACE* plugin_interface);
LIBRARY_API void udaPluginSetVersion(UDA_PLUGIN_INTERFACE* plugin_interface, int plugin_version);
LIBRARY_API const char* udaPluginFunction(UDA_PLUGIN_INTERFACE* plugin_interface);

LIBRARY_API void udaPluginLog(UDA_PLUGIN_INTERFACE* plugin_interface, const char* file, int line, const char* msg);
LIBRARY_API void udaPluginLog_s(UDA_PLUGIN_INTERFACE* plugin_interface, const char* file, int line, const char* fmt, const char* arg);
LIBRARY_API void udaPluginLog_i(UDA_PLUGIN_INTERFACE* plugin_interface, const char* file, int line, const char* fmt, int arg);

LIBRARY_API void udaAddPluginError(UDA_PLUGIN_INTERFACE* plugin_interface, const char* location, int code,
                                   const char* msg);

LIBRARY_API UDA_PLUGIN_INTERFACE* udaCreatePluginInterface(UDA_PLUGIN_INTERFACE* plugin_interface, const char* request);

LIBRARY_API void udaFreePluginInterface(UDA_PLUGIN_INTERFACE* plugin_interface);

LIBRARY_API COMPOUNDFIELD* udaNewCompoundField(const char* name, const char* description, int* offset, int type,
                                               int rank, int* shape);

LIBRARY_API COMPOUNDFIELD* udaNewCompoundPointerField(const char* name, const char* description, int* offset, int type,
                                                      bool is_scalar);

LIBRARY_API COMPOUNDFIELD* udaNewCompoundFixedStringField(const char* name, const char* description, int* offset, int rank, int* shape);

LIBRARY_API COMPOUNDFIELD* udaNewCompoundVarStringField(const char* name, const char* description, int* offset);
LIBRARY_API COMPOUNDFIELD* udaNewCompoundVarStringArrayField(const char* name, const char* description, int* offset, int rank, int* shape);

LIBRARY_API COMPOUNDFIELD* udaNewCompoundUserTypeField(const char* name, const char* description, int* offset,
                                                       USERDEFINEDTYPE* user_type);
LIBRARY_API COMPOUNDFIELD* udaNewCompoundUserTypePointerField(const char* name, const char* description, int* offset,
                                                              USERDEFINEDTYPE* user_type);
LIBRARY_API COMPOUNDFIELD* udaNewCompoundUserTypeArrayField(const char* name, const char* description, int* offset,
                                                            USERDEFINEDTYPE* user_type, int rank, int* shape);

LIBRARY_API USERDEFINEDTYPE* udaNewUserType(const char* name, const char* source, int ref_id, int image_count,
                                            char* image, size_t size, size_t num_fields, COMPOUNDFIELD** fields);

LIBRARY_API int udaAddUserType(UDA_PLUGIN_INTERFACE*, USERDEFINEDTYPE* user_type);
LIBRARY_API int udaRegisterMalloc(UDA_PLUGIN_INTERFACE* plugin_interface, void* data, int, size_t, const char*);
LIBRARY_API int udaRegisterMallocArray(UDA_PLUGIN_INTERFACE* plugin_interface, void* data, int count, size_t size,
                                       const char* type, int rank, int* shape);

LIBRARY_API int udaPluginPluginsCount(UDA_PLUGIN_INTERFACE* plugin_interface);
LIBRARY_API int udaPluginCheckPluginClass(UDA_PLUGIN_INTERFACE* plugin_interface, int plugin_num,
                                          const char* plugin_class);
LIBRARY_API const char* udaPluginPluginFormat(UDA_PLUGIN_INTERFACE* plugin_interface, int plugin_num);
LIBRARY_API const char* udaPluginPluginExtension(UDA_PLUGIN_INTERFACE* plugin_interface, int plugin_num);
LIBRARY_API const char* udaPluginPluginDescription(UDA_PLUGIN_INTERFACE* plugin_interface, int plugin_num);
LIBRARY_API const char* udaPluginPluginExample(UDA_PLUGIN_INTERFACE* plugin_interface, int plugin_num);

LIBRARY_API int udaPluginReturnDataLabel(UDA_PLUGIN_INTERFACE* plugin_interface, const char* label);
LIBRARY_API int udaPluginReturnDataUnits(UDA_PLUGIN_INTERFACE* plugin_interface, const char* units);

#define UDA_DEF_SET_RETURN_FUNCS(NAME, TYPE)                                                                           \
    LIBRARY_API int udaPluginReturnData##NAME##Scalar(UDA_PLUGIN_INTERFACE* plugin_interface, TYPE value,              \
                                                      const char* description);                                        \
    LIBRARY_API int udaPluginReturnData##NAME##Array(UDA_PLUGIN_INTERFACE* plugin_interface, const TYPE* values,       \
                                                     size_t rank, const size_t* shape, const char* description);

UDA_DEF_SET_RETURN_FUNCS(Float, float)
UDA_DEF_SET_RETURN_FUNCS(Double, double)
UDA_DEF_SET_RETURN_FUNCS(Char, char)
UDA_DEF_SET_RETURN_FUNCS(UChar, unsigned char)
UDA_DEF_SET_RETURN_FUNCS(Short, short)
UDA_DEF_SET_RETURN_FUNCS(UShort, unsigned short)
UDA_DEF_SET_RETURN_FUNCS(Int, int)
UDA_DEF_SET_RETURN_FUNCS(UInt, unsigned int)
UDA_DEF_SET_RETURN_FUNCS(Long, long)
UDA_DEF_SET_RETURN_FUNCS(ULong, unsigned long)

LIBRARY_API int udaPluginReturnDataStringScalar(UDA_PLUGIN_INTERFACE* plugin_interface, const char* value,
                                                const char* description);
LIBRARY_API int udaPluginReturnDataStringArray(UDA_PLUGIN_INTERFACE* plugin_interface, const char** values, size_t rank,
                                               const size_t* shape, const char* description);

#undef UDA_DEF_SET_RETURN_FUNCS

LIBRARY_API int udaPluginReturnData(UDA_PLUGIN_INTERFACE* plugin_interface, void* value, size_t size, UDA_TYPE type,
                                    int rank, const int* shape, const char* description);

LIBRARY_API int udaPluginReturnDimensionFloatArray(UDA_PLUGIN_INTERFACE* plugin_interface, int dim_n, float* data,
                                                   size_t size, const char* label, const char* units);

LIBRARY_API int udaPluginReturnErrorAsymmetry(UDA_PLUGIN_INTERFACE* plugin_interface, bool flag);
LIBRARY_API int udaPluginReturnErrorLow(UDA_PLUGIN_INTERFACE* plugin_interface, float* data, size_t size);
LIBRARY_API int udaPluginReturnErrorHigh(UDA_PLUGIN_INTERFACE* plugin_interface, float* data, size_t size);
LIBRARY_API int udaPluginReturnDataOrder(UDA_PLUGIN_INTERFACE* plugin_interface, int order);

LIBRARY_API int udaPluginReturnCompoundData(UDA_PLUGIN_INTERFACE* plugin_interface, char* data, const char* user_type,
                                            const char* description);
LIBRARY_API int udaPluginReturnCompoundArrayData(UDA_PLUGIN_INTERFACE* plugin_interface, char* data,
                                                 const char* user_type, const char* description, int rank, int* shape);

LIBRARY_API int udaPluginArgumentCount(const UDA_PLUGIN_INTERFACE* plugin_interface);
LIBRARY_API const char* udaPluginArgument(const UDA_PLUGIN_INTERFACE* plugin_interface, int num);

LIBRARY_API bool udaPluginFindArg(const UDA_PLUGIN_INTERFACE* plugin_interface, const char* name);

#define UDA_DEF_FIND_FUNCS(NAME, TYPE)                                                                                 \
    LIBRARY_API bool udaPluginFind##NAME##Arg(const UDA_PLUGIN_INTERFACE* plugin_interface, TYPE* value,               \
                                              const char* name);                                                       \
    LIBRARY_API bool udaPluginFind##NAME##ArrayArg(const UDA_PLUGIN_INTERFACE* plugin_interface, TYPE** value,         \
                                                   size_t* nvalues, const char* name);

UDA_DEF_FIND_FUNCS(Float, float)
UDA_DEF_FIND_FUNCS(Double, double)
UDA_DEF_FIND_FUNCS(Char, char)
UDA_DEF_FIND_FUNCS(UChar, unsigned char)
UDA_DEF_FIND_FUNCS(Short, short)
UDA_DEF_FIND_FUNCS(UShort, unsigned short)
UDA_DEF_FIND_FUNCS(Int, int)
UDA_DEF_FIND_FUNCS(UInt, unsigned int)
UDA_DEF_FIND_FUNCS(Long, long)
UDA_DEF_FIND_FUNCS(ULong, unsigned long)
UDA_DEF_FIND_FUNCS(String, const char*)

#undef UDA_DEF_FIND_FUNCS

#define QUOTE_(X) #X
#define QUOTE(X) QUOTE_(X)
#define CONCAT_(X, Y) X##Y
#define CONCAT(X, Y) CONCAT_(X, Y)
#define UNIQUE_VAR(NAME) __func__##NAME##__

#define UDA_RAISE_PLUGIN_ERROR_AND_EXIT(PLUGIN_INTERFACE, MSG)                                                         \
    {                                                                                                                  \
        int UNIQUE_VAR(err) = 999;                                                                                     \
        udaPluginLog(PLUGIN_INTERFACE, "%s\n", MSG);                                                                   \
        udaAddPluginError(__func__, UNIQUE_VAR(err), MSG);                                                             \
        return UNIQUE_VAR(err);                                                                                        \
    }

#define UDA_RAISE_PLUGIN_ERROR(PLUGIN_INTERFACE, MSG)                                                                  \
    {                                                                                                                  \
        int UNIQUE_VAR(err) = 999;                                                                                     \
        UDA_PLUGIN_LOG(PLUGIN_INTERFACE, MSG);                                                                 \
        udaAddPluginError(PLUGIN_INTERFACE, __func__, UNIQUE_VAR(err), MSG);                                           \
        return UNIQUE_VAR(err);                                                                                        \
    }

#define UDA_RAISE_PLUGIN_ERROR_F(PLUGIN_INTERFACE, MSG, FMT, ...)                                                      \
    {                                                                                                                  \
        int UNIQUE_VAR(err) = 999;                                                                                     \
        udaPluginLog(PLUGIN_INTERFACE, "%s\n", FMT, __VA_ARGS__);                                                      \
        udaAddPluginError(PLUGIN_INTERFACE, __func__, UNIQUE_VAR(err), MSG);                                           \
        return UNIQUE_VAR(err);                                                                                        \
    }

#define UDA_RAISE_PLUGIN_ERROR_AND_EXIT_F(PLUGIN_INTERFACE, MSG, FMT, ...)                                             \
    {                                                                                                                  \
        int UNIQUE_VAR(err) = 999;                                                                                     \
        udaPluginLog(PLUGIN_INTERFACE, "%s\n", FMT, __VA_ARGS__);                                                      \
        udaAddPluginError(PLUGIN_INTERFACE, __func__, UNIQUE_VAR(err), MSG);                                           \
        return UNIQUE_VAR(err);                                                                                        \
    }

#define UDA_RAISE_PLUGIN_ERROR_EX(PLUGIN_INTERFACE, MSG, CODE)                                                         \
    int UNIQUE_VAR(err) = 999;                                                                                         \
    udaPluginLog(PLUGIN_INTERFACE, "%s", MSG);                                                                         \
    udaAddPluginError(PLUGIN_INTERFACE, __func__, UNIQUE_VAR(err), MSG);                                               \
    {                                                                                                                  \
        CODE                                                                                                           \
    }                                                                                                                  \
    return UNIQUE_VAR(err);

#define UDA_FIND_REQUIRED_VALUE(PLUGIN_INTERFACE, VARIABLE, TYPE)                                                      \
    if (!udaPluginFind##TYPE##Arg(PLUGIN_INTERFACE, &VARIABLE, QUOTE(VARIABLE))) {                                     \
        UDA_RAISE_PLUGIN_ERROR(PLUGIN_INTERFACE, "Required argument '" QUOTE(VARIABLE) "' not given");                 \
    }

#define UDA_FIND_REQUIRED_ARRAY(PLUGIN_INTERFACE, VARIABLE, TYPE)                                                      \
    if (!udaPluginFind##TYPE##ArrayArg(PLUGIN_INTERFACE, &VARIABLE, CONCAT(&n, VARIABLE), QUOTE(VARIABLE))) {          \
        UDA_RAISE_PLUGIN_ERROR(PLUGIN_INTERFACE, "Required argument '" QUOTE(VARIABLE) "' not given");                 \
    }

#define UDA_FIND_REQUIRED_INT_VALUE(PLUGIN_INTERFACE, VARIABLE) UDA_FIND_REQUIRED_VALUE(PLUGIN_INTERFACE, VARIABLE, Int)
#define UDA_FIND_REQUIRED_SHORT_VALUE(PLUGIN_INTERFACE, VARIABLE)                                                      \
    UDA_FIND_REQUIRED_VALUE(PLUGIN_INTERFACE, VARIABLE, Short)
#define UDA_FIND_REQUIRED_CHAR_VALUE(PLUGIN_INTERFACE, VARIABLE)                                                       \
    UDA_FIND_REQUIRED_VALUE(PLUGIN_INTERFACE, VARIABLE, Char)
#define UDA_FIND_REQUIRED_FLOAT_VALUE(PLUGIN_INTERFACE, VARIABLE)                                                      \
    UDA_FIND_REQUIRED_VALUE(PLUGIN_INTERFACE, VARIABLE, Float)
#define UDA_FIND_REQUIRED_STRING_VALUE(PLUGIN_INTERFACE, VARIABLE)                                                     \
    UDA_FIND_REQUIRED_VALUE(PLUGIN_INTERFACE, VARIABLE, String)

#define UDA_FIND_REQUIRED_INT_ARRAY(PLUGIN_INTERFACE, VARIABLE) UDA_FIND_REQUIRED_ARRAY(PLUGIN_INTERFACE, VARIABLE, Int)
#define UDA_FIND_REQUIRED_FLOAT_ARRAY(PLUGIN_INTERFACE, VARIABLE)                                                      \
    UDA_FIND_REQUIRED_ARRAY(PLUGIN_INTERFACE, VARIABLE, Float)
#define UDA_FIND_REQUIRED_DOUBLE_ARRAY(PLUGIN_INTERFACE, VARIABLE)                                                     \
    UDA_FIND_REQUIRED_ARRAY(PLUGIN_INTERFACE, VARIABLE, Double)

#define UDA_FIND_INT_VALUE(PLUGIN_INTERFACE, VARIABLE) udaPluginFindIntArg(PLUGIN_INTERFACE, &VARIABLE, QUOTE(VARIABLE))
#define UDA_FIND_SHORT_VALUE(PLUGIN_INTERFACE, VARIABLE)                                                               \
    udaPluginFindShortArg(PLUGIN_INTERFACE, &VARIABLE, QUOTE(VARIABLE))
#define UDA_FIND_CHAR_VALUE(PLUGIN_INTERFACE, VARIABLE)                                                                \
    udaPluginFindCharArg(PLUGIN_INTERFACE, &VARIABLE, QUOTE(VARIABLE))
#define UDA_FIND_FLOAT_VALUE(PLUGIN_INTERFACE, VARIABLE)                                                               \
    udaPluginFindFloatArg(PLUGIN_INTERFACE, &VARIABLE, QUOTE(VARIABLE))
#define UDA_FIND_STRING_VALUE(PLUGIN_INTERFACE, VARIABLE)                                                              \
    udaPluginFindStringArg(PLUGIN_INTERFACE, &VARIABLE, QUOTE(VARIABLE))

#define UDA_FIND_INT_ARRAY(PLUGIN_INTERFACE, VARIABLE)                                                                 \
    udaPluginFindIntArrayArg(PLUGIN_INTERFACE, &VARIABLE, CONCAT(&n, VARIABLE), QUOTE(VARIABLE))
#define UDA_FIND_FLOAT_ARRAY(PLUGIN_INTERFACE, VARIABLE)                                                               \
    udaPluginFindFloatArrayArg(PLUGIN_INTERFACE, &VARIABLE, CONCAT(&n, VARIABLE), QUOTE(VARIABLE))
#define UDA_FIND_DOUBLE_ARRAY(PLUGIN_INTERFACE, VARIABLE)                                                              \
    udaPluginFindDoubleArrayArg(PLUGIN_INTERFACE, &VARIABLE, CONCAT(&n, VARIABLE), QUOTE(VARIABLE))

#define UDA_CALL_PLUGIN(PLUGIN_INTERFACE, FMT, ...)                                                                    \
    {                                                                                                                  \
        char UNIQUE_VAR(request)[1024];                                                                                \
        snprintf(UNIQUE_VAR(request), 1024, FMT, __VA_ARGS__);                                                         \
        UNIQUE_VAR(request)[1023] = '\0';                                                                              \
        int UNIQUE_VAR(err) = callPlugin(PLUGIN_INTERFACE, UNIQUE_VAR(request));                                       \
        if (UNIQUE_VAR(err)) {                                                                                         \
            UDA_RAISE_PLUGIN_ERROR("Plugin call failed");                                                              \
        }                                                                                                              \
    }

#ifdef __cplusplus
}
#endif

#endif // UDA_PLUGINS_H