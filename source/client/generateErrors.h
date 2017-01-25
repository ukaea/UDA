#ifndef IDAM_CLIENT_GENERATEERRORS_H
#define IDAM_CLIENT_GENERATEERRORS_H

#include "idamclientserver.h"
#include "idamclient.h"
#include "idamErrorLog.h"

int idamErrorModel(int model, int param_n, float *params, int data_n, float *data, int *asymmetry, char *errhi, char *errlo);
int idamSyntheticModel(int model, int param_n, float *params, int data_n, float *data);
int generateIdamSyntheticData(int handle);
int generateIdamSyntheticDimData(int handle, int ndim);
int generateIdamDataError(int handle);
int generateIdamDimDataError(int handle, int ndim);

#endif // IDAM_CLIENT_GENERATEERRORS_H

