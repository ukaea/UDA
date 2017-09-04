#ifndef IDAM_CLIENT_CLOSEDOWN_H
#define IDAM_CLIENT_CLOSEDOWN_H

#include <clientserver/socketStructs.h>

#ifdef FATCLIENT
#  define idamClosedown idamClosedownFat
#endif

enum ClosedownType {
    CLOSE_SOCKETS = 0,
    CLOSE_ALL = 1,
};

int idamClosedown(int type, SOCKETLIST* socket_list);

#endif // IDAM_CLIENT_CLOSEDOWN_H
