#ifndef UDA_PLUGIN_WEST_TUNNEL_WEST_TUNNEL_SSH_H
#define UDA_PLUGIN_WEST_TUNNEL_WEST_TUNNEL_SSH_H

#include <pthread.h>
#include <stdbool.h>

extern pthread_mutex_t g_west_tunnel_initialised_mutex;
extern pthread_cond_t g_west_tunnel_initialised_cond;
extern bool g_west_tunnel_initialised;
extern int g_west_tunnel_server_port;

#endif // UDA_PLUGIN_WEST_TUNNEL_WEST_TUNNEL_SSH_H
