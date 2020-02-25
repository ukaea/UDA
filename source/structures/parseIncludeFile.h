#ifndef IDAM_STRUCTURES_PARSEINCLUDEFILE_H
#define IDAM_STRUCTURES_PARSEINCLUDEFILE_H

#include "genStructs.h"

#if defined(_WIN32)
#  define LIBRARY_API __declspec(dllexport)
#else
#  define LIBRARY_API
#endif

#ifdef __cplusplus
extern "C" {
#endif

LIBRARY_API int parseIncludeFile(USERDEFINEDTYPELIST* userdefinedtypelist, const char* header);

#ifdef __cplusplus
}
#endif

#endif // IDAM_STRUCTURES_PARSEINCLUDEFILE_H

