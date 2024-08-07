#ifndef LEGACY_CLIENT_H
#define LEGACY_CLIENT_H

#include <clientserver/export.h>
#include "udaClient.h"

#ifdef __cplusplus
extern "C" {
#endif

LIBRARY_API void idamFree(int handle);
LIBRARY_API void idamFreeAll();

#ifdef __cplusplus
}
#endif


#endif
