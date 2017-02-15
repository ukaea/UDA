#ifndef UDA_CLIENT_IDAMAPI_H
#define UDA_CLIENT_IDAMAPI_H

#ifdef __cplusplus
extern "C" {
#endif

#ifdef FATCLIENT
#  define idamGetAPI idamGetAPIFat
#endif

int idamGetAPI(const char *data_object, const char *data_source);

#ifdef __cplusplus
}
#endif

#endif // UDA_CLIENT_IDAMAPI_H

