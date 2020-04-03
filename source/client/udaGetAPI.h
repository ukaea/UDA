#ifndef UDA_CLIENT_UDAGETAPI_H
#define UDA_CLIENT_UDAGETAPI_H

#if defined(_WIN32)
#  define LIBRARY_API __declspec(dllexport)
#else
#  define LIBRARY_API
#endif

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

