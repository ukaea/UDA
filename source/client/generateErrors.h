#ifndef UDA_CLIENT_GENERATEERRORS_H
#define UDA_CLIENT_GENERATEERRORS_H

#include "export.h"

#ifdef __cplusplus
extern "C" {
#endif

LIBRARY_API int idamErrorModel(int model, int param_n, float* params, int data_n, float* data, int* asymmetry,
                               float* errhi, float* errlo);
LIBRARY_API int idamSyntheticModel(int model, int param_n, float* params, int data_n, float* data);
LIBRARY_API int generateIdamSyntheticData(int handle);
LIBRARY_API int generateIdamSyntheticDimData(int handle, int ndim);
LIBRARY_API int generateIdamDataError(int handle);
LIBRARY_API int generateIdamDimDataError(int handle, int ndim);

#ifdef __cplusplus
}
#endif

#endif // UDA_CLIENT_GENERATEERRORS_H