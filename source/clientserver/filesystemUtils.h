#ifndef UDA_CLIENTSERVER_FILESYSTEMUTILS_H
#define UDA_CLIENTSERVER_FILESYSTEMUTILS_H

#include "export.h"

#ifdef __cplusplus
extern "C" {
#endif

LIBRARY_API int check_allowed_path(const char* expandedPath);

#ifdef __cplusplus
}
#endif

#endif // UDA_CLIENTSERVER_FILESYSTEMUTILS_H