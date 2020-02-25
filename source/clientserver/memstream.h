#ifndef UDA_CLIENTSERVER_MAC_MEMSTREAM_H
#define UDA_CLIENTSERVER_MAC_MEMSTREAM_H

#include <stdio.h>

#if defined(_WIN32)
#  define LIBRARY_API __declspec(dllexport)
#else
#  define LIBRARY_API
#endif

#ifdef __cplusplus
extern "C"
{
#endif

LIBRARY_API FILE* open_memstream(char** cp, size_t* lenp);

#ifdef __cplusplus
}
#endif

#endif // UDA_CLIENTSERVER_MAC_MEMSTREAM_H
