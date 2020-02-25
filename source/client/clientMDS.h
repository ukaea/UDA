#ifndef UDA_CLIENT_CLIENTMDS_H
#define UDA_CLIENT_CLIENTMDS_H

#if defined(_WIN32)
#  define LIBRARY_API __declspec(dllexport)
#else
#  define LIBRARY_API
#endif

#ifdef __cplusplus
extern "C" {
#endif

LIBRARY_API int idamClientMDS(const char *server, const char *tree, const char *node, int treenum);

#ifdef __cplusplus
}
#endif

#endif // UDA_CLIENT_CLIENTMDS_H
