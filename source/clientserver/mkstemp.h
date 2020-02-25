#ifndef UDA_CLIENTSERVER_MKSTEMP_H
#define UDA_CLIENTSERVER_MKSTEMP_H

#if defined(_WIN32)
#  define LIBRARY_API __declspec(dllexport)
#else
#  define LIBRARY_API
#endif

#ifdef __cplusplus
extern "C" {
#endif

#ifndef __GNUC__
LIBRARY_API int mkstemp (char *tmpl);
#endif

#ifdef __cplusplus
}
#endif

#endif // UDA_CLIENTSERVER_MKSTEMP_H
