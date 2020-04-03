#ifndef UDA_CLIENT_CLOSEDOWN_H
#define UDA_CLIENT_CLOSEDOWN_H

#include <clientserver/socketStructs.h>

#ifdef FATCLIENT
#  define idamClosedown idamClosedownFat
#endif

#if defined(_WIN32)
#  define LIBRARY_API __declspec(dllexport)
#else
#  define LIBRARY_API
#endif

#ifdef __cplusplus
extern "C" {
#endif

enum ClosedownType {
    CLOSE_SOCKETS = 0,
    CLOSE_ALL = 1,
};

LIBRARY_API int idamClosedown(int type, SOCKETLIST* socket_list);

#ifdef __cplusplus
}
#endif

#endif // UDA_CLIENT_CLOSEDOWN_H
