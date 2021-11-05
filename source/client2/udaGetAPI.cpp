#include "udaGetAPI.h"

#include "client.hpp"
#include "thread_client.hpp"
#include "exceptions.hpp"

#include <unordered_map>

int udaGetAPI(const char *data_object, const char *data_source)
{
    auto& client = uda::client::ThreadClient::instance();
    try {
        return client.get(data_object, data_source);
    } catch (uda::exceptions::UDAException& ex) {
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

int udaGetAPIWithHost(const char *data_object, const char *data_source, const char *host, int port)
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