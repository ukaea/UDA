#ifndef UDA_CLIENT_UDAGETAPI_H
#define UDA_CLIENT_UDAGETAPI_H

#include "export.h"

#ifdef __cplusplus
extern "C" {
#endif

#ifdef FATCLIENT
#  define udaGetAPI udaGetAPIFat
#  define udaGetBatchAPI udaGetBatchAPIFat
#  define udaGetAPIWithHost udaGetAPIWithHostFat
#  define udaGetBatchAPIWithHost udaGetBatchAPIWithHostFat
#endif

LIBRARY_API int udaGetAPI(const char* data_object, const char* data_source);
LIBRARY_API int udaGetBatchAPI(const char** uda_signals, const char** sources, int count, int* handles);
LIBRARY_API int udaGetAPIWithHost(const char* data_object, const char* data_source, const char* host, int port);
LIBRARY_API int udaGetBatchAPIWithHost(const char** uda_signals, const char** sources, int count, int* handles,
                                       const char* host, int port);

#ifdef __cplusplus
}
#endif

#endif // UDA_CLIENT_UDAGETAPI_H
