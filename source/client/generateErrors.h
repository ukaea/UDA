#pragma once

namespace uda::client
{

int idamErrorModel(int model, int param_n, float* params, int data_n, float* data, int* asymmetry, float* errhi,
                   float* errlo);

int idamSyntheticModel(int model, int param_n, float* params, int data_n, float* data);

int generateIdamSyntheticData(int handle);

int generateIdamSyntheticDimData(int handle, int ndim);

int generateIdamDataError(int handle);

int generateIdamDimDataError(int handle, int ndim);

} // namespace uda::client
