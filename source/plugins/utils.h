#ifndef UDA_PLUGINS_UTILS_H
#define UDA_PLUGINS_UTILS_H

#include <clientserver/export.h>

#ifdef __cplusplus
extern "C" {
#endif

LIBRARY_API int check_allowed_path(const char* expanded_path);

#ifdef __cplusplus
}
#endif

#endif // UDA_PLUGINS_UTILS_H
