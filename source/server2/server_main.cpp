#include <cstdlib>
#if defined(__GNUC__)
#  include <unistd.h>
#else
#  include <windows.h>
#  define sleep Sleep
#endif

#include "server.hpp"
#include "server_config.h"
#include "server_exceptions.h"

using namespace uda::server;

int main()
{
    Config config;
    try {
        config.load("uda_server.toml");
    } catch (uda::server::ConfigError& error) {
        return -1;
    }

    // Optional sleep at startup

    auto startup_sleep = config.get("server.startup_sleep");
    if (startup_sleep) {
        sleep(startup_sleep.as<int>());
    }

    // Run server

    try {
        Server server{config};
        server.run();
    } catch (uda::server::Exception& ex) {
        return ex.code();
    }

    return 0;
}
