#ifndef UDA_CLIENT_STARTUP_H
#define UDA_CLIENT_STARTUP_H

#ifdef FATCLIENT
#  define idamStartup idamStartupFat
#endif

#if defined(_WIN32)
#  define LIBRARY_API __declspec(dllexport)
#else
#  define LIBRARY_API
#endif

#ifdef __cplusplus
extern "C" {
#endif

LIBRARY_API int idamStartup(int reset);

#ifdef __cplusplus
}
#endif

#endif // UDA_CLIENT_STARTUP_H
