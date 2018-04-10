#ifndef UDA_CLIENT_STARTUP_H
#define UDA_CLIENT_STARTUP_H

#ifdef FATCLIENT
#  define idamStartup idamStartupFat
#endif

#ifdef __cplusplus
extern "C" {
#endif

int idamStartup(int reset);

#ifdef __cplusplus
}
#endif

#endif // UDA_CLIENT_STARTUP_H
