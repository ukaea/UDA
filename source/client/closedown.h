#ifndef IDAM_CLOSEDOWN_H
#define IDAM_CLOSEDOWN_H

#ifdef FATCLIENT
#  define idamClosedown idamClosedownFat
#endif

int idamClosedown(int type);

#endif // IDAM_CLOSEDOWN_H
