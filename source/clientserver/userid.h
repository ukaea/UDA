#ifndef UDA_CLIENTSERVER_USERID_H
#define UDA_CLIENTSERVER_USERID_H

#if defined(_WIN32)
#  define LIBRARY_API __declspec(dllexport)
#else
#  define LIBRARY_API
#endif

#ifdef __cplusplus
extern "C" {
#endif

LIBRARY_API void userid(char* uid);

#ifdef __cplusplus
}
#endif

#endif // UDA_CLIENTSERVER_USERID_H
