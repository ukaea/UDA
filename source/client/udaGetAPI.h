#ifndef UDA_CLIENT_UDAGETAPI_H
#define UDA_CLIENT_UDAGETAPI_H

#include <clientserver/export.h>

#ifdef __cplusplus
extern "C" {
#endif

#ifdef FATCLIENT
#  define idamGetAPI idamGetAPIFat
#  define idamGetBatchAPI idamGetBatchAPIFat
#  define idamGetAPIWithHost idamGetAPIWithHostFat
#  define idamGetBatchAPIWithHost idamGetBatchAPIWithHostFat
#endif

LIBRARY_API int idamGetAPI(const char *data_object, const char *data_source);
LIBRARY_API int idamGetBatchAPI(const char** signals, const char** sources, int count, int* handles);
LIBRARY_API int idamGetAPIWithHost(const char *data_object, const char *data_source, const char *host, int port);
LIBRARY_API int idamGetBatchAPIWithHost(const char** signals, const char** sources, int count, int* handles, const char* host, int port);

#ifdef __cplusplus
}
#endif

#endif // UDA_CLIENT_UDAGETAPI_H

