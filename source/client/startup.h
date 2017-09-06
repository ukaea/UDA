#ifndef UDA_CLIENT_STARTUP_H
#define UDA_CLIENT_STARTUP_H

#ifdef FATCLIENT
#  define idamStartup idamStartupFat
#endif

int idamStartup(int reset);

#endif // UDA_CLIENT_STARTUP_H
