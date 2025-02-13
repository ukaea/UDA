#pragma once

#include "clientserver/error_log.h"

namespace uda::client {

int error_model(int model, int param_n, float* params, int data_n, float* data, int* asymmetry, float* errhi,
                float* errlo);
int synthetic_model(std::vector<client_server::UdaError>& error_stack, int model, int param_n, float* params, int data_n, float* data);
int generate_synthetic_data(std::vector<client_server::UdaError>& error_stack, int handle);
int generate_synthetic_dim_data(std::vector<client_server::UdaError>& error_stack, int handle, int ndim);
int generate_data_error(std::vector<client_server::UdaError>& error_stack, int handle);
int generate_dim_data_error(std::vector<client_server::UdaError>& error_stack, int handle, int ndim);

} // namespace uda::client
