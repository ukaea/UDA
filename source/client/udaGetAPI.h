#ifndef UDA_CLIENT_UDAGETAPI_H
#define UDA_CLIENT_UDAGETAPI_H

#include <clientserver/export.h>

#ifdef __cplusplus
extern "C" {
#endif

#ifdef FATCLIENT
#  define idamGetAPI idamGetAPIFat
#endif

LIBRARY_API int idamGetAPI(const char *data_object, const char *data_source);

#ifdef __cplusplus
}
#endif

#endif // UDA_CLIENT_UDAGETAPI_H

