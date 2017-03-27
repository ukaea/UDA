#ifndef IDAM_CLIENT_CLOSEDOWN_H
#define IDAM_CLIENT_CLOSEDOWN_H

#ifdef FATCLIENT
#  define idamClosedown idamClosedownFat
#endif

enum ClosedownType {
    CLOSE_SOCKETS = 0,
    CLOSE_ALL = 1,
};

int idamClosedown(int type);

#endif // IDAM_CLIENT_CLOSEDOWN_H
