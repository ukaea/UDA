#ifndef UDA_CLIENTSERVER_IDAMERRORLOG_H
#define UDA_CLIENTSERVER_IDAMERRORLOG_H

#include <clientserver/udaStructs.h>

#include <time.h>

#define DATELENGTH	27

//--------------------------------------------------------
// Error Management

#define SYSTEMERRORTYPE     1
#define CODEERRORTYPE       2
#define PLUGINERRORTYPE     3

#if defined(_WIN32)
#  define LIBRARY_API __declspec(dllexport)
#else
#  define LIBRARY_API
#endif

#ifdef __cplusplus
extern "C" {
#endif

LIBRARY_API int udaNumErrors(void);
LIBRARY_API void idamErrorLog(CLIENT_BLOCK client_block, REQUEST_BLOCK request, IDAMERRORSTACK* errorstack);
LIBRARY_API void initIdamErrorStack(void);
LIBRARY_API void initIdamErrorRecords(void);
LIBRARY_API void printIdamErrorStack(void);
LIBRARY_API void addIdamError(int type, const char* location, int code, const char* msg);
LIBRARY_API void concatIdamError(IDAMERRORSTACK* errorstackout);
LIBRARY_API void freeIdamErrorStack(IDAMERRORSTACK* errorstack);
LIBRARY_API void closeIdamError(void);

#ifdef __cplusplus
}
#endif

#define THROW_ERROR(ERR, MSG) addIdamError(CODEERRORTYPE, __func__, ERR, MSG); return ERR;

#endif // UDA_CLIENTSERVER_IDAMERRORLOG_H
