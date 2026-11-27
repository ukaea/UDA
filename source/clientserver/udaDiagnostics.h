#ifndef UDA_CLIENTSERVER_UDADIAGNOSTICS_H
#define UDA_CLIENTSERVER_UDADIAGNOSTICS_H

// Explicit, stateless error and log reporting for code shared by the client, the server and plugins.
//
// Shared helpers that can fail or trace take a `const UDA_DIAGNOSTICS*` and report only through it, so they hold no
// error or log state of their own. That keeps them correct however many copies of them are loaded into a process
// (each shared library on macOS and Windows binds to its own copy of any library-global state).
//
//   - The client and server pass udaGlobalDiagnostics(), which forwards to their own error stack and logger.
//   - Plugins pass udaPluginDiagnostics(plugin_interface) (plugins/udaPlugin.h), which records onto the plugin
//     interface's error stack and logs through the host's logging callback.
//   - A NULL context, or a NULL sink inside one, discards that output.

#include <clientserver/udaStructs.h>
#include "export.h"

#define UDA_SYSTEM_ERROR_TYPE     1
#define UDA_CODE_ERROR_TYPE       2
#define UDA_PLUGIN_ERROR_TYPE     3

#ifdef __cplusplus
extern "C" {
#endif

//--------------------------------------------------------
// Stateless error-stack helpers: they only touch the stack they are given

LIBRARY_API void initErrorRecords(UDA_ERROR_STACK* errorstack);

/**
 * Append a single error record directly to the given error stack, without touching any library-global state.
 *
 * Strings are copied and terminated, insertion order is preserved and at most UDA_MAX_ERRORS records are kept
 * (earliest first). System errors (UDA_SYSTEM_ERROR_TYPE) have the strerror() text for code appended.
 *
 * @return 0 on success, 1 if the stack was full and the record was dropped, -1 if error_stack is NULL
 */
LIBRARY_API int udaAddErrorToStack(UDA_ERROR_STACK* error_stack, int type, const char* location, int code,
                                   const char* msg);

/**
 * Append all records of source to error_stack, in order, respecting UDA_MAX_ERRORS.
 *
 * @return 0 on success, 1 if some records were dropped, -1 if either argument is NULL
 */
LIBRARY_API int udaAppendErrorStack(UDA_ERROR_STACK* error_stack, const UDA_ERROR_STACK* source);

/**
 * If a request failed (code != 0) but error_stack holds no records, append a fallback record carrying the
 * original code, so that the response header and the error state cannot disagree.
 *
 * @return 1 if a fallback record was added, 0 otherwise
 */
LIBRARY_API int udaEnsureErrorRecorded(UDA_ERROR_STACK* error_stack, int code, const char* location);

//--------------------------------------------------------
// Diagnostics contexts

// Log sink: receives one formatted message. `level` is a LOG_LEVEL value. The message pointer and context are only
// valid for the duration of the call.
typedef void (*UDA_LOG_SINK_FUNCTION)(void* context, int level, const char* file, int line, const char* message);

// Error sink: records one error. Returns 0 if recorded, nonzero if it was dropped (e.g. the stack is full).
typedef int (*UDA_ERROR_SINK_FUNCTION)(void* context, int type, const char* location, int code, const char* message);

typedef struct UdaDiagnostics {
    UDA_ERROR_SINK_FUNCTION error_function;   // NULL => errors are discarded
    void* error_context;
    UDA_LOG_SINK_FUNCTION log_function;       // NULL => logging is a no-op
    void* log_context;
    int log_level;                            // minimum LOG_LEVEL that is delivered to log_function
} UDA_DIAGNOSTICS;

// Build a single error record: strings are copied, truncated and terminated (NULL is treated as empty), and system
// errors have the strerror() text for their code appended.
LIBRARY_API UDA_ERROR udaMakeError(int type, const char* location, int code, const char* msg);

// Error sink that appends to the UDA_ERROR_STACK passed as its context (see udaAddErrorToStack()).
LIBRARY_API int udaErrorStackSink(void* error_stack, int type, const char* location, int code, const char* message);

// A context that records errors onto `error_stack` and logs through `log_function` (either may be NULL).
LIBRARY_API UDA_DIAGNOSTICS udaStackDiagnostics(UDA_ERROR_STACK* error_stack, UDA_LOG_SINK_FUNCTION log_function,
                                                void* log_context, int log_level);

LIBRARY_API int udaDiagAddError(const UDA_DIAGNOSTICS* diag, int type, const char* location, int code,
                                const char* message);
LIBRARY_API int udaDiagAddErrorF(const UDA_DIAGNOSTICS* diag, int type, const char* location, int code,
                                 const char* fmt, ...)
#if defined(__GNUC__)
    __attribute__((format(printf, 5, 6)))
#endif
    ;
LIBRARY_API int udaDiagLogEnabled(const UDA_DIAGNOSTICS* diag, int level);
LIBRARY_API void udaDiagLog(const UDA_DIAGNOSTICS* diag, int level, const char* file, int line, const char* fmt, ...)
#if defined(__GNUC__)
    __attribute__((format(printf, 5, 6)))
#endif
    ;

#ifdef __cplusplus
}
#endif

// Log through a diagnostics context. The level is checked before the message is formatted.
#define UDA_DIAG_LOG(DIAG, LEVEL, FMT, ...) \
    do { \
        if (udaDiagLogEnabled(DIAG, LEVEL)) { \
            udaDiagLog(DIAG, LEVEL, __FILE__, __LINE__, FMT, ##__VA_ARGS__); \
        } \
    } while (0)

#define UDA_DIAG_ADD_ERROR(DIAG, CODE, MSG) udaDiagAddError(DIAG, UDA_CODE_ERROR_TYPE, __func__, CODE, MSG)
#define UDA_DIAG_ADD_SYS_ERROR(DIAG, MSG) udaDiagAddError(DIAG, UDA_SYSTEM_ERROR_TYPE, __func__, errno, MSG)
#define UDA_DIAG_THROW_ERROR(DIAG, CODE, MSG) \
    { \
        udaDiagAddError(DIAG, UDA_CODE_ERROR_TYPE, __func__, CODE, MSG); \
        return CODE; \
    }

#endif // UDA_CLIENTSERVER_UDADIAGNOSTICS_H
