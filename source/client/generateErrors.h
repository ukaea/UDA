#ifndef UDA_CLIENT_GENERATEERRORS_H
#define UDA_CLIENT_GENERATEERRORS_H

int idamErrorModel(int model, int param_n, float* params, int data_n, float* data, int* asymmetry, float* errhi,
                   float* errlo);
int idamSyntheticModel(int model, int param_n, float* params, int data_n, float* data);
int generateIdamSyntheticData(int handle);
int generateIdamSyntheticDimData(int handle, int ndim);
int generateIdamDataError(int handle);
int generateIdamDimDataError(int handle, int ndim);

#endif // UDA_CLIENT_GENERATEERRORS_H