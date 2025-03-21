#include <cstdlib>
#if defined(__GNUC__)
#  include <unistd.h>
#else
#  include <windows.h>
#  define sleep Sleep
#endif

#include <iostream>

#include "server.hpp"
#include "config/config.h"
#include "server_exceptions.h"

using namespace uda::server;
using namespace uda::config;

int main()
{
    const char* server_config_path = getenv("UDA_SERVER_CONFIG_PATH");
    if (server_config_path == nullptr) {
        server_config_path = "uda_server.toml";
    }

    Config config;
    try {
        config.load(server_config_path);
    } catch (ConfigError& error) {
        std::cerr << error.what() << std::endl;
        return -1;
    }

    // Optional sleep at startup

    auto startup_sleep = config.get("server.startup_sleep");
    if (startup_sleep) {
        sleep(startup_sleep.as<int>());
    }

    // Run server

    try {
        Server server{std::move(config)};
        server.run();
    } catch (uda::server::Exception& ex) {
        return ex.code();
    }

    return 0;
}
