#ifndef UDA_CLIENT_CLIENTMDS_H
#define UDA_CLIENT_CLIENTMDS_H

#include "export.h"

#ifdef __cplusplus
extern "C" {
#endif

LIBRARY_API int idamClientMDS(const char *server, const char *tree, const char *node, int treenum);

#ifdef __cplusplus
}
#endif

#endif // UDA_CLIENT_CLIENTMDS_H
