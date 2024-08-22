#ifndef UDA_LEGACY_CLIENT_H
#define UDA_LEGACY_CLIENT_H

#include <clientserver/export.h>
#include "udaClient.h"

#ifdef __cplusplus
extern "C" {
#endif

LIBRARY_API inline void idamFree(int handle)
{
    udaFree(handle);
}

LIBRARY_API inline void idamFreeAll()
{
    udaFreeAll();
}

#ifdef __cplusplus
}
#endif


#endif
