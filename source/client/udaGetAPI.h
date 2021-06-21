#ifndef UDA_CLIENT_UDAGETAPI_H
#define UDA_CLIENT_UDAGETAPI_H

#include <clientserver/export.h>
#include <cstdlib>

#ifdef __cplusplus
extern "C" {
#endif

#ifdef FATCLIENT
#  define idamGetAPI idamGetAPIFat
#  define idamGetBatchAPI idamGetBatchAPIFat
#endif

LIBRARY_API int idamGetAPI(const char *data_object, const char *data_source);
LIBRARY_API int idamGetBatchAPI(const char** signals, const char** sources, size_t count);

#ifdef __cplusplus
}
#endif

#endif // UDA_CLIENT_UDAGETAPI_H

