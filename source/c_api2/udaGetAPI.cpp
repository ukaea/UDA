#include <uda/client.h>

#include "client2/client.hpp"
#include "client2/thread_client.hpp"
#include "client2/exceptions.hpp"
#include "logging/logging.h"

#include <iostream>

void udaLoadConfig(const char* config_name) {
    try {
        auto& client = uda::client::ThreadClient::instance();
        client.load_config(config_name);
    } catch (const std::exception& ex) {
        std::cout << "UDAException: " << ex.what() << std::endl;
    }
}

int udaGetAPI(const char* data_object, const char* data_source)
{
    auto& client = uda::client::ThreadClient::instance();
    try {
        return client.get(data_object, data_source);
    } catch (uda::exceptions::UDAException& ex) {
        UDA_LOG(uda::logging::LogLevel::UDA_LOG_ERROR, ex.what());
        std::cout << "UDAException: " << ex.what() << std::endl;
        return -1;
    } catch (std::exception& ex){
        UDA_LOG(uda::logging::LogLevel::UDA_LOG_ERROR, ex.what());
        std::cout << "std::exception: " << ex.what() << std::endl;
        return -1;
    } catch (...)
    {
        std::cout << "unknown error occurred? ¯\\_(ツ)_/¯ " << std::endl;
        return -1;
    }
}

int udaGetBatchAPI(const char** data_signals, const char** data_sources, int count, int* handles)
{
    auto& client = uda::client::ThreadClient::instance();
    try {
        std::vector<std::pair<std::string, std::string>> requests;
        requests.reserve(count);
        for (int i = 0; i < count; ++i) {
            requests.emplace_back(std::make_pair(data_signals[i], data_sources[i]));
        }
        auto handle_vec = client.get(requests);
        for (int i = 0; i < count; ++i) {
            handles[i] = handle_vec[i];
        }
        return 0;
    } catch (uda::exceptions::UDAException& ex) {
        return -1;
    }
}

int udaGetAPIWithHost(const char* data_object, const char* data_source, const char* host, int port)
{
    auto& client = uda::client::ThreadClient::instance();
    client.set_host(host);
    client.set_port(port);
    try {
        return client.get(data_object, data_source);
    } catch (uda::exceptions::UDAException& ex) {
        return -1;
    }
}

int udaGetBatchAPIWithHost(const char** data_signals, const char** data_sources, int count, int* handles, const char* host, int port)
{
    auto& client = uda::client::ThreadClient::instance();
    client.set_host(host);
    client.set_port(port);
    try {
        std::vector<std::pair<std::string, std::string>> requests;
        requests.reserve(count);
        for (int i = 0; i < count; ++i) {
            requests.emplace_back(std::make_pair(data_signals[i], data_sources[i]));
        }
        auto handle_vec = client.get(requests);
        for (int i = 0; i < count; ++i) {
            handles[i] = handle_vec[i];
        }
        return 0;
    } catch (uda::exceptions::UDAException& ex) {
        return -1;
    }
}
