#include <cstdlib>
#if defined(__GNUC__)
#  include <unistd.h>
#else
#  include <windows.h>
#  define sleep Sleep
#endif

#include "udaServer.h"
#include "config/config.h"

using namespace uda::config;

int main(int argc, char** argv)
{
    // Optional sleep at startup

    char* env = getenv("UDA_SLEEP");
    if (env != nullptr) {
        sleep((unsigned int)atoi(env));
    }

    Config config;
    try {
        config.load("uda_server.toml");
    } catch (ConfigError& error) {
        return -1;
    }

    // Run server

    uda::client_server::ClientBlock client_block = {0};

    int rc = uda::server::uda_server(config, client_block);

    return rc;
}
