#include "west_tunnel_ssh.h"

pthread_mutex_t g_initialised_mutex;
pthread_cond_t g_initialised_cond;
bool g_initialised;
int g_server_port;