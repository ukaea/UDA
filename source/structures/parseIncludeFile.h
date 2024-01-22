#ifndef UDA_STRUCTURES_PARSEINCLUDEFILE_H
#define UDA_STRUCTURES_PARSEINCLUDEFILE_H

#include "genStructs.h"

#include "export.h"

#ifdef __cplusplus
extern "C" {
#endif

LIBRARY_API int parseIncludeFile(USERDEFINEDTYPELIST* userdefinedtypelist, const char* header);

#ifdef __cplusplus
}
#endif

#endif // UDA_STRUCTURES_PARSEINCLUDEFILE_H

