#pragma once

namespace uda::client
{

int error_model(int model, int param_n, float* params, int data_n, float* data, int* asymmetry, float* errhi,
                   float* errlo);

int synthetic_model(int model, int param_n, float* params, int data_n, float* data);

int generate_synthetic_data(int handle);

int generate_synthetic_dim_data(int handle, int ndim);

int generate_data_error(int handle);

int generate_dim_data_error(int handle, int ndim);

} // namespace uda::client
