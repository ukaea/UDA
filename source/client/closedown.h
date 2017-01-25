#ifndef IDAM_CLIENT_CLOSEDOWN_H
#define IDAM_CLIENT_CLOSEDOWN_H

#ifdef FATCLIENT
#  define idamClosedown idamClosedownFat
#endif

int idamClosedown(int type);

#endif // IDAM_CLIENT_CLOSEDOWN_H
