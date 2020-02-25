#ifndef UDA_CLIENTSERVER_ALLOCXMLDATA_H
#define UDA_CLIENTSERVER_ALLOCXMLDATA_H

#ifdef HIERARCHICAL_DATA

#include "idamclientserver.h"
#include "idamclientserverxml.h"

#if defined(_WIN32)
#  define LIBRARY_API __declspec(dllexport)
#else
#  define LIBRARY_API
#endif

#ifdef __cplusplus
extern "C" {
#endif

LIBRARY_API int alloc_efit(EFIT *efit);
LIBRARY_API int alloc_pfcircuit(PFCIRCUIT *str);
LIBRARY_API int alloc_pfcoils(PFCOILS *str);
LIBRARY_API int alloc_pfpassive(PFPASSIVE *str);
LIBRARY_API int alloc_fluxloop(FLUXLOOP *str);
LIBRARY_API int alloc_limiter(LIMITER *str);

#ifdef __cplusplus
}
#endif

#endif

#endif // UDA_CLIENTSERVER_ALLOCXMLDATA_H
