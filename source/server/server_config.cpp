#include "server_config.h"

uda::config::Config* uda::server::server_config() {
    static bool initialised = false;
    static uda::config::Config config = {};

    if (!initialised) {
        config.load("uda_server.toml");
    }

    return &config;
}