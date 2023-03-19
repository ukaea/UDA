#include "thread_client.hpp"

std::once_flag uda::client::ThreadClient::init_flag_ = {};
uda::client::Client* uda::client::ThreadClient::instance_ = nullptr;

uda::client::Client& uda::client::ThreadClient::instance()
{
    std::call_once(init_flag_, &ThreadClient::init_client);
    return *instance_;
}

void uda::client::ThreadClient::init_client()
{
    instance_ = new Client;
}
