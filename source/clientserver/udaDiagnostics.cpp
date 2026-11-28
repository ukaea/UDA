#include "udaDiagnostics.h"

#include <cstdarg>
#include <cstdio>
#include <cstring>
#include <string>

#include "udaErrors.h"

// Everything in this file is stateless: it only touches the stack or context it is given, so any number of copies
// of it may be loaded into one process.

namespace {

// Copies at most size - 1 characters of src and always terminates dest. Used instead of strncpy, which GCC's
// -Wstringop-truncation flags here once inlining shows the source may fill the buffer.
void copy_string(char* dest, const char* src, size_t size)
{
    const size_t length = strnlen(src, size - 1);
    memcpy(dest, src, length);
    dest[length] = '\0';
}

} // namespace

UDA_ERROR udaMakeError(int type, const char* location, int code, const char* msg)
{
    UDA_ERROR error = {};

    error.type = type;
    error.code = code;
    copy_string(error.location, location != nullptr ? location : "", STRING_LENGTH);
    copy_string(error.msg, msg != nullptr ? msg : "", STRING_LENGTH);

    size_t lmsg0 = strlen(error.msg);

    if (type == UDA_SYSTEM_ERROR_TYPE) {
        const char* errmsg = strerror(code);
        size_t lmsg1 = strlen(errmsg);
        if (lmsg0 == 0) {
            copy_string(error.msg, errmsg, STRING_LENGTH);
        } else {
            if ((lmsg0 + 2) < STRING_LENGTH) {
                strcat(error.msg, "; ");
                if ((lmsg0 + lmsg1 + 2) < STRING_LENGTH) {
                    strcat(error.msg, errmsg);
                } else {
                    strncat(error.msg, errmsg, ((unsigned int)(STRING_LENGTH - 1 - (lmsg0 + 2))));
                    error.msg[STRING_LENGTH - 1] = '\0';
                }
            }
        }
    }

    return error;
}

void initErrorRecords(UDA_ERROR_STACK* errorstack)
{
    for (unsigned int i = 0; i < UDA_MAX_ERRORS; i++) {
        errorstack->idamerror[i].type = 0;
        errorstack->idamerror[i].code = 0;
        errorstack->idamerror[i].location[0] = '\0';
        errorstack->idamerror[i].msg[0] = '\0';
    }
}

int udaAddErrorToStack(UDA_ERROR_STACK* error_stack, int type, const char* location, int code, const char* msg)
{
    if (error_stack == nullptr) {
        return -1;
    }
    if (error_stack->nerrors >= UDA_MAX_ERRORS) {
        // Keep the earliest records: the first record is the root cause and drives the response header
        return 1;
    }
    error_stack->idamerror[error_stack->nerrors] = udaMakeError(type, location, code, msg);
    error_stack->nerrors++;
    return 0;
}

int udaAppendErrorStack(UDA_ERROR_STACK* error_stack, const UDA_ERROR_STACK* source)
{
    if (error_stack == nullptr || source == nullptr) {
        return -1;
    }
    int rc = 0;
    unsigned int count = source->nerrors < UDA_MAX_ERRORS ? source->nerrors : UDA_MAX_ERRORS;
    for (unsigned int i = 0; i < count; ++i) {
        if (error_stack->nerrors >= UDA_MAX_ERRORS) {
            rc = 1;
            break;
        }
        error_stack->idamerror[error_stack->nerrors++] = source->idamerror[i];
    }
    return rc;
}

int udaEnsureErrorRecorded(UDA_ERROR_STACK* error_stack, int code, const char* location)
{
    if (error_stack == nullptr || code == 0 || error_stack->nerrors > 0) {
        return 0;
    }
    char msg[STRING_LENGTH];
    snprintf(msg, STRING_LENGTH, "Request failed with error code %d but no error details were reported", code);
    udaAddErrorToStack(error_stack, UDA_CODE_ERROR_TYPE, location, code, msg);
    return 1;
}

int udaErrorStackSink(void* error_stack, int type, const char* location, int code, const char* message)
{
    return udaAddErrorToStack(static_cast<UDA_ERROR_STACK*>(error_stack), type, location, code, message);
}

UDA_DIAGNOSTICS udaStackDiagnostics(UDA_ERROR_STACK* error_stack, UDA_LOG_SINK_FUNCTION log_function,
                                    void* log_context, int log_level)
{
    UDA_DIAGNOSTICS diag = {};
    diag.error_function = error_stack != nullptr ? udaErrorStackSink : nullptr;
    diag.error_context = error_stack;
    diag.log_function = log_function;
    diag.log_context = log_context;
    diag.log_level = log_level;
    return diag;
}

int udaDiagAddError(const UDA_DIAGNOSTICS* diag, int type, const char* location, int code, const char* message)
{
    if (diag == nullptr || diag->error_function == nullptr) {
        return -1;
    }
    return diag->error_function(diag->error_context, type, location, code, message);
}

int udaDiagAddErrorF(const UDA_DIAGNOSTICS* diag, int type, const char* location, int code, const char* fmt, ...)
{
    char msg[STRING_LENGTH];
    va_list args;
    va_start(args, fmt);
    vsnprintf(msg, STRING_LENGTH, fmt, args);
    va_end(args);
    return udaDiagAddError(diag, type, location, code, msg);
}

int udaDiagLogEnabled(const UDA_DIAGNOSTICS* diag, int level)
{
    return diag != nullptr && diag->log_function != nullptr && level >= diag->log_level;
}

void udaDiagLog(const UDA_DIAGNOSTICS* diag, int level, const char* file, int line, const char* fmt, ...)
{
    if (!udaDiagLogEnabled(diag, level)) {
        return;
    }

    va_list args;
    va_start(args, fmt);
    va_list args_copy;
    va_copy(args_copy, args);
    int len = vsnprintf(nullptr, 0, fmt, args_copy);
    va_end(args_copy);
    if (len < 0) {
        va_end(args);
        return;
    }
    std::string msg((size_t)len + 1, '\0');
    vsnprintf(&msg[0], msg.size(), fmt, args);
    va_end(args);
    msg.resize((size_t)len);

    if (file != nullptr) {
        // Report the file name only, as UDA_LOG does
        const char* slash = strrchr(file, '/');
        const char* backslash = strrchr(file, '\\');
        const char* base = slash > backslash ? slash : backslash;
        file = base != nullptr ? base + 1 : file;
    }

    diag->log_function(diag->log_context, level, file, line, msg.c_str());
}
