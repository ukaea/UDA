#include "client_config.h"

uda::config::Config* uda::client::client_config() {
    static bool initialised = false;
    static uda::config::Config config = {};

    if (!initialised) {
        config.load("uda_client.toml");
    }

    return &config;
}