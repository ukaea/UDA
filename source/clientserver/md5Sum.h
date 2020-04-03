#ifndef UDA_CLIENTSERVER_MD5SUM_H
#define UDA_CLIENTSERVER_MD5SUM_H

#if defined(_WIN32)
#  define LIBRARY_API __declspec(dllexport)
#else
#  define LIBRARY_API
#endif

#ifdef __cplusplus
extern "C" {
#endif

LIBRARY_API void md5Sum(char* bp, int size, char* md5check);

#ifdef __cplusplus
}
#endif

#endif // UDA_CLIENTSERVER_MD5SUM_H
