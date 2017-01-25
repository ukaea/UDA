#ifndef IDAM_CLIENT_STARTUP_H
#define IDAM_CLIENT_STARTUP_H

#ifdef FATCLIENT
#  define idamStartup idamStartupFat
#endif

int idamStartup(int reset);

#endif // IDAM_CLIENT_STARTUP_H
