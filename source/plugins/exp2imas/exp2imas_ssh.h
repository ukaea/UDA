#ifndef UDA_PLUGIN_EXP2IMAS_EXP2IMAS_SSH_H
#define UDA_PLUGIN_EXP2IMAS_EXP2IMAS_SSH_H

#include <pthread.h>
#include <stdbool.h>

extern pthread_mutex_t g_initialised_mutex;
extern pthread_cond_t g_initialised_cond;
extern bool g_initialised;
extern int g_server_port;

#endif // UDA_PLUGIN_EXP2IMAS_EXP2IMAS_SSH_H