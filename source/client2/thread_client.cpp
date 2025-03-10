#include "thread_client.hpp"

#include "generate_errors.hpp"
#include "uda/client.h"

using namespace uda::client_server;

thread_local bool uda::client::ThreadClient::init_flag_ = false;
thread_local uda::client::Client* uda::client::ThreadClient::instance_ = nullptr;

uda::client::Client& uda::client::ThreadClient::instance()
{
    // std::call_once(init_flag_, &ThreadClient::init_client);
    if (!init_flag_)
    {
        init_client();
        init_flag_ = true;
    }
    return *instance_;
}

void uda::client::ThreadClient::init_client()
{
    instance_ = new Client;
}

DataBlock* uda::client::get_data_block(const int handle)
{
    auto& instance = ThreadClient::instance();
    auto* data_block = instance.data_block(handle);

    if (data_block == nullptr) {
        return nullptr;
    }
    return data_block;
}

char* uda::client::get_data(const int handle)
{
    auto& instance = ThreadClient::instance();
    const auto* data_block = instance.data_block(handle);
    const auto* client_flags = instance.client_flags();

    int status = get_data_status(handle);
    if (data_block == nullptr) {
        return nullptr;
    }
    if (status == MIN_STATUS && !data_block->client_block.get_bad && !client_flags->get_bad) {
        return nullptr;
    }
    if (status != MIN_STATUS && (data_block->client_block.get_bad || client_flags->get_bad)) {
        return nullptr;
    }
    if (!client_flags->get_synthetic) {
        return data_block->data;
    } else {
        return get_synthetic_data(handle);
    }
}

char* uda::client::get_synthetic_data(const int handle)
{
    auto& instance = ThreadClient::instance();
    const auto* data_block = instance.data_block(handle);
    const auto* client_flags = instance.client_flags();

    int status = get_data_status(handle);
    if (data_block == nullptr) {
        return nullptr;
    }
    if (status == MIN_STATUS && !data_block->client_block.get_bad && !client_flags->get_bad) {
        return nullptr;
    }
    if (status != MIN_STATUS && (data_block->client_block.get_bad || client_flags->get_bad)) {
        return nullptr;
    }
    if (!client_flags->get_synthetic || data_block->error_model == (int)ErrorModelType::Unknown) {
        return data_block->data;
    }
    generate_synthetic_data(instance.error_stack(), handle);
    return data_block->synthetic;
}

void uda::client::set_synthetic_data(int handle, char* data)
{
    auto& instance = ThreadClient::instance();
    auto* data_block = instance.data_block(handle);

    if (data_block == nullptr) {
        return;
    }
    data_block->synthetic = data;
}

int uda::client::get_data_status(const int handle)
{
    auto& instance = ThreadClient::instance();
    const auto* data_block = instance.data_block(handle);

    // Data Status based on Standard Rule
    if (data_block == nullptr) {
        return 0;
    }
    if (get_signal_status(handle) == DefaultStatus) {
        // Signal Status Not Changed from Default - use Data Source Value
        return data_block->source_status;
    } else {
        return data_block->signal_status;
    }
}

int uda::client::get_signal_status(const int handle)
{
    auto& instance = ThreadClient::instance();
    const auto* data_block = instance.data_block(handle);

    // Signal Status
    if (data_block == nullptr) {
        return 0;
    }
    return data_block->signal_status;
}

int uda::client::get_data_num(const int handle)
{
    auto& instance = ThreadClient::instance();
    const auto* data_block = instance.data_block(handle);

    // Data Array Size
    if (data_block == nullptr) {
        return 0;
    }
    return data_block->data_n;
}

int uda::client::get_data_type(const int handle)
{
    auto& instance = ThreadClient::instance();
    const auto* data_block = instance.data_block(handle);

    if (data_block == nullptr) {
        return UDA_TYPE_UNKNOWN;
    }
    return data_block->data_type;
}

void uda::client::get_error_model(int handle, int* model, int* param_n, float* params)
{
    auto& instance = ThreadClient::instance();
    const auto* data_block = instance.data_block(handle);

    if (data_block == nullptr) {
        *model = (int)ErrorModelType::Unknown;
        *param_n = 0;
        return;
    }
    *model = data_block->error_model;     // Model ID
    *param_n = data_block->error_param_n; // Number of parameters
    for (int i = 0; i < data_block->error_param_n; i++) {
        params[i] = data_block->errparams[i];
    }
}

int uda::client::get_dim_num(int handle, int n_dim)
{
    auto& instance = ThreadClient::instance();
    const auto* data_block = instance.data_block(handle);

    if (data_block == nullptr || n_dim < 0 || (unsigned int)n_dim >= data_block->rank) {
        return 0;
    }
    return data_block->dims[n_dim].dim_n;
}

char* uda::client::get_dim_data(int handle, int n_dim)
{
    auto& instance = ThreadClient::instance();
    const auto* data_block = instance.data_block(handle);

    if (data_block == nullptr || n_dim < 0 || (unsigned int)n_dim >= data_block->rank) {
        return nullptr;
    }

    const auto* client_flags = instance.client_flags();
    if (!client_flags->get_synthetic) {
        return data_block->dims[n_dim].dim;
    }
    return get_synthetic_dim_data(handle, n_dim);
}

int uda::client::get_dim_type(int handle, int n_dim)
{
    auto& instance = ThreadClient::instance();
    const auto* data_block = instance.data_block(handle);

    if (data_block == nullptr || n_dim < 0 || (unsigned int)n_dim >= data_block->rank) {
        return UDA_TYPE_UNKNOWN;
    }
    return data_block->dims[n_dim].data_type;
}

char* uda::client::get_synthetic_dim_data(int handle, int n_dim)
{
    auto& instance = ThreadClient::instance();
    const auto* data_block = instance.data_block(handle);

    if (data_block == nullptr) {
        return nullptr;
    }
    return data_block->dims[n_dim].synthetic;
}

void uda::client::set_synthetic_dim_data(int handle, int n_dim, char* data)
{
    auto& instance = ThreadClient::instance();
    const auto* data_block = instance.data_block(handle);

    if (data_block == nullptr) {
        return;
    }
    data_block->dims[n_dim].synthetic = data;
}

char* uda::client::get_data_err_hi(const int handle)
{
    auto& instance = ThreadClient::instance();
    const auto* data_block = instance.data_block(handle);

    if (data_block == nullptr) {
        return nullptr;
    }
    return data_block->errhi;
}

char* uda::client::get_data_err_lo(const int handle)
{
    auto& instance = ThreadClient::instance();
    const auto* data_block = instance.data_block(handle);

    if (data_block == nullptr) {
        return nullptr;
    }
    return data_block->errlo;
}

int uda::client::get_data_err_asymmetry(const int handle)
{
    auto& instance = ThreadClient::instance();
    const auto* data_block = instance.data_block(handle);

    if (data_block == nullptr) {
        return 0;
    }
    return data_block->errasymmetry;
}

char* uda::client::get_dim_err_hi(int handle, int n_dim)
{
    auto& instance = ThreadClient::instance();
    const auto* data_block = instance.data_block(handle);

    if (data_block == nullptr) {
        return nullptr;
    }
    return data_block->dims[n_dim].errhi;
}

char* uda::client::get_dim_err_lo(int handle, int n_dim)
{
    auto& instance = ThreadClient::instance();
    const auto* data_block = instance.data_block(handle);

    if (data_block == nullptr) {
        return nullptr;
    }
    return data_block->dims[n_dim].errlo;
}

int uda::client::get_dim_err_asymmetry(int handle, int n_dim)
{
    auto& instance = ThreadClient::instance();
    const auto* data_block = instance.data_block(handle);

    if (data_block == nullptr || n_dim < 0 || (unsigned int)n_dim >= data_block->rank) {
        return 0;
    }
    return data_block->dims[n_dim].errasymmetry;
}

void uda::client::set_dim_err_lo(int handle, int n_dim, char* err_lo)
{
    auto& instance = ThreadClient::instance();
    const auto* data_block = instance.data_block(handle);

    if (data_block == nullptr) {
        return;
    }
    data_block->dims[n_dim].errlo = err_lo;
}

void uda::client::set_dim_err_type(int handle, int n_dim, int type)
{
    auto& instance = ThreadClient::instance();
    const auto* data_block = instance.data_block(handle);

    if (data_block == nullptr) {
        return;
    }
    data_block->dims[n_dim].error_type = type;
}

void uda::client::set_dim_err_asymmetry(int handle, int n_dim, int asymmetry)
{
    auto& instance = ThreadClient::instance();
    const auto* data_block = instance.data_block(handle);

    if (data_block == nullptr) {
        return;
    }
    data_block->dims[n_dim].errasymmetry = asymmetry;
}

int uda::client::get_rank(const int handle)
{
    auto& instance = ThreadClient::instance();
    const auto* data_block = instance.data_block(handle);

    // Array Rank
    if (data_block == nullptr) {
        return 0;
    }
    return (int)data_block->rank;
}

void uda::client::set_data_err_lo(int handle, char* err_lo)
{
    auto& instance = ThreadClient::instance();
    auto* data_block = instance.data_block(handle);

    if (data_block == nullptr) {
        return;
    }
    data_block->errlo = err_lo;
}

void uda::client::set_data_err_type(int handle, int type)
{
    auto& instance = ThreadClient::instance();
    auto* data_block = instance.data_block(handle);

    if (data_block == nullptr) {
        return;
    }
    data_block->error_type = type;
}

void uda::client::set_data_err_asymmetry(int handle, int asymmetry)
{
    auto& instance = ThreadClient::instance();
    auto* data_block = instance.data_block(handle);

    if (data_block == nullptr) {
        return;
    }
    data_block->errasymmetry = asymmetry;
}
