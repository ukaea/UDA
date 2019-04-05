#ifndef UDA_CLIENT_CLOSEDOWN_H
#define UDA_CLIENT_CLOSEDOWN_H

#include <clientserver/socketStructs.h>

#ifdef FATCLIENT
#  define idamClosedown idamClosedownFat
#endif

#ifdef __cplusplus
extern "C" {
#endif

enum ClosedownType {
    CLOSE_SOCKETS = 0,
    CLOSE_ALL = 1,
};

int idamClosedown(int type, SOCKETLIST* socket_list);

#ifdef __cplusplus
}
#endif

#endif // UDA_CLIENT_CLOSEDOWN_H
