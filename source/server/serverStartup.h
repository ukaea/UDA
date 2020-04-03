#ifndef UDA_SERVER_SERVERSTARTUP_H
#define UDA_SERVER_SERVERSTARTUP_H

#if defined(_WIN32)
#  define LIBRARY_API __declspec(dllexport)
#else
#  define LIBRARY_API
#endif

#ifdef __cplusplus
extern "C" {
#endif

LIBRARY_API int startup(void);

#ifdef __cplusplus
}
#endif

#endif // UDA_SERVER_SERVERSTARTUP_H

