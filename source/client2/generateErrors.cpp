#include "generateErrors.h"

#include <cmath>
#include <cstdlib>

#include <clientserver/udaTypes.h>
#include <clientserver/errorLog.h>
#include <clientserver/allocData.h>

#include "accAPI.h"
#include "thread_client.h"

#ifndef NO_GSL_LIB
#  include <gsl/gsl_randist.h>
#endif

//--------------------------------------------------------------------------------------------------------------
// Generate Error Data

int uda::client::error_model(int model, int param_n, float* params, int data_n, float* data, int* asymmetry, float* errhi,
                             float* errlo)
{
    *asymmetry = 0;        // No Error Asymmetry for most models

    switch (model) {

        case ERROR_MODEL_DEFAULT:
            if (param_n != 2) return 1;
            for (int i = 0; i < data_n; i++) {
                errhi[i] = params[0] + params[1] * fabsf(data[i]);
                errlo[i] = errhi[i];
            }
            break;

        case ERROR_MODEL_DEFAULT_ASYMMETRIC:
            if (param_n != 4) return 1;
            *asymmetry = 1;
            for (int i = 0; i < data_n; i++) {
                errhi[i] = params[0] + params[1] * fabsf(data[i]);
                errlo[i] = params[2] + params[3] * fabsf(data[i]);
            }
            break;

        default:
            for (int i = 0; i < data_n; i++) {
                errhi[i] = 0.0;
                errlo[i] = 0.0;
            }
    }

    return 0;
}

//--------------------------------------------------------------------------------------------------------------
// Generate Synthetic Data using Random Number generators * PDFs
//
// NB: to change the default GSL library settings use the following environment variables
//    GSL_RNG_SEED    12345        for the seed value
//    GSL_RNG_TYPE    mrg        for the name of the random number generator

int uda::client::synthetic_model(int model, int param_n, float* params, int data_n, float* data)
{

#ifdef NO_GSL_LIB
    int err = 999;
    addIdamError(CODEERRORTYPE, "synthetic_model", err,
                 "Random Number Generators from the GSL library required.");
    return 999;
#else
    float shift;
    static gsl_rng *random=nullptr;

    if(random == nullptr) {        // Initialise the Random Number System
        gsl_rng_env_setup();
        random = gsl_rng_alloc (gsl_rng_default);
        gsl_rng_set(random, (unsigned long int) ERROR_MODEL_SEED);    // Seed the Random Number generator with the library default
    }

    switch (model) {

    case ERROR_MODEL_GAUSSIAN:      // Standard normal Distribution
        if(param_n < 1 || param_n > 2) return 1;
        if(param_n == 2) gsl_rng_set(random, (unsigned long int) params[1]);        // Change the Seed before Sampling
        for(i=0; i<data_n; i++) data[i] = data[i] + (float) gsl_ran_gaussian (random, (double) params[0]); // Random sample from PDF
        break;

    case ERROR_MODEL_RESEED:                              // Change the Seed
        if(param_n == 1) gsl_rng_set(random, (unsigned long int) params[0]);
        break;

    case ERROR_MODEL_GAUSSIAN_SHIFT:
        if(param_n < 1 || param_n > 2) return 1;
        if(param_n == 2) gsl_rng_set(random, (unsigned long int) params[1]);        // Change the Seed before Sampling
        shift = (float) gsl_ran_gaussian (random, (double) params[0]);
        for(i=0; i<data_n; i++) data[i] = data[i] + shift;                // Random linear shift of data array
        break;

    case ERROR_MODEL_POISSON:
        if(param_n < 0 || param_n > 1) return 1;
        if(param_n == 1) gsl_rng_set(random, (unsigned long int) params[0]);                // Change the Seed before Sampling
        for(i=0; i<data_n; i++) data[i] = data[i] + (float) gsl_ran_poisson(random, (double)data[i]);    // Randomly perturb data array
        break;
    }

    return 0;
#endif
}

char* getSyntheticData(int handle)
{
    auto& instance = uda::client::ThreadClient::instance();
    auto data_block = instance.data_block(handle);

    if (data_block == nullptr) {
        return nullptr;
    }
    return data_block->synthetic;
}

int uda::client::generate_synthetic_data(int handle)
{
    int err = 0;

    char* synthetic = nullptr;

    //--------------------------------------------------------------------------------------------------------------
    // Check the handle and model are ok

    if (udaGetData(handle) == nullptr) return 0;

    int model;
    int param_n;
    float params[MAXERRPARAMS];

    udaGetErrorModel(handle, &model, &param_n, params);

    if (model <= ERROR_MODEL_UNKNOWN || model >= ERROR_MODEL_UNDEFINED) return 0;    // No valid Model

    if (udaGetDataNum(handle) <= 0) return 0;

    //--------------------------------------------------------------------------------------------------------------
    // Allocate local float work arrays and copy the data array to the work array

    if (udaGetDataType(handle) == UDA_TYPE_DCOMPLEX || udaGetDataType(handle) == UDA_TYPE_COMPLEX) {
        err = 999;
        addIdamError(CODEERRORTYPE, "generate_synthetic_data", err,
                     "Not configured to Generate Complex Type Synthetic Data");
        return 999;
    }

    float* data = nullptr;
    if ((data = (float*)malloc(udaGetDataNum(handle) * sizeof(float))) == nullptr) {
        return 1;
    }

    switch (udaGetDataType(handle)) {
        case UDA_TYPE_FLOAT: {
            auto fp = (float*)udaGetData(handle);
            for (int i = 0; i < udaGetDataNum(handle); i++) data[i] = fp[i];        // Cast all data to Float
            break;
        }
        case UDA_TYPE_DOUBLE: {
            auto dp = (double*)udaGetData(handle);
            for (int i = 0; i < udaGetDataNum(handle); i++) data[i] = (float)dp[i];
            break;
        }
        case UDA_TYPE_SHORT: {
            auto sp = (short*)udaGetData(handle);
            for (int i = 0; i < udaGetDataNum(handle); i++) data[i] = (float)sp[i];
            break;
        }
        case UDA_TYPE_INT: {
            auto ip = (int*)udaGetData(handle);
            for (int i = 0; i < udaGetDataNum(handle); i++) data[i] = (float)ip[i];
            break;
        }
        case UDA_TYPE_LONG: {
            auto lp = (long*)udaGetData(handle);
            for (int i = 0; i < udaGetDataNum(handle); i++) data[i] = (float)lp[i];
            break;
        }
        case UDA_TYPE_LONG64: {
            auto lp = (long long int*)udaGetData(handle);
            for (int i = 0; i < udaGetDataNum(handle); i++) data[i] = (float)lp[i];
            break;
        }
        case UDA_TYPE_CHAR: {
            auto cp = (char*)udaGetData(handle);
            for (int i = 0; i < udaGetDataNum(handle); i++) data[i] = (float)cp[i];
            break;
        }
        case UDA_TYPE_UNSIGNED_SHORT: {
            auto sp = (unsigned short*)udaGetData(handle);
            for (int i = 0; i < udaGetDataNum(handle); i++) data[i] = (float)sp[i];
            break;
        }
        case UDA_TYPE_UNSIGNED_INT: {
            auto up = (unsigned int*)udaGetData(handle);
            for (int i = 0; i < udaGetDataNum(handle); i++) data[i] = (float)up[i];
            break;
        }
        case UDA_TYPE_UNSIGNED_LONG: {
            auto lp = (unsigned long*)udaGetData(handle);
            for (int i = 0; i < udaGetDataNum(handle); i++) data[i] = (float)lp[i];
            break;
        }
        case UDA_TYPE_UNSIGNED_LONG64: {
            auto lp = (unsigned long long int *)udaGetData(handle);
            for (int i = 0; i < udaGetDataNum(handle); i++) data[i] = (float) lp[i];
            break;
        }
        case UDA_TYPE_UNSIGNED_CHAR: {
            auto cp = (unsigned char*)udaGetData(handle);
            for (int i = 0; i < udaGetDataNum(handle); i++) data[i] = (float)cp[i];
            break;
        }

        default:
            free(data);
            return 0;
    }

    //--------------------------------------------------------------------------------------------------------------
    // Generate Synthetic Data

    err = synthetic_model(model, param_n, params, udaGetDataNum(handle), data);

    if (err != 0) {
        addIdamError(CODEERRORTYPE, "generate_synthetic_data", err,
                     "Unable to Generate Synthetic Data");
        free(data);
        return err;
    }

    //--------------------------------------------------------------------------------------------------------------
    // Return the Synthetic Data

    if (getSyntheticData(handle) == nullptr) {
        if ((err = allocArray(udaGetDataType(handle), udaGetDataNum(handle), &synthetic))) {
            addIdamError(CODEERRORTYPE, "generate_synthetic_data", err,
                         "Problem Allocating Heap Memory for Synthetic Data");
            return err;
        }
        udaSetSyntheticData(handle, synthetic);
    }

    switch (udaGetDataType(handle)) {
        case UDA_TYPE_FLOAT: {
            auto fp = (float*)getSyntheticData(handle);
            for (int i = 0; i < udaGetDataNum(handle); i++) fp[i] = data[i];        // Overwrite the Data
            break;
        }
        case UDA_TYPE_DOUBLE: {
            auto dp = (double*)getSyntheticData(handle);
            for (int i = 0; i < udaGetDataNum(handle); i++) dp[i] = (double)data[i];
            break;
        }
        case UDA_TYPE_SHORT: {
            auto sp = (short*)getSyntheticData(handle);
            for (int i = 0; i < udaGetDataNum(handle); i++) sp[i] = (short)data[i];
            break;
        }
        case UDA_TYPE_INT: {
            auto ip = (int*)getSyntheticData(handle);
            for (int i = 0; i < udaGetDataNum(handle); i++) ip[i] = (int)data[i];
            break;
        }
        case UDA_TYPE_LONG: {
            auto lp = (long*)getSyntheticData(handle);
            for (int i = 0; i < udaGetDataNum(handle); i++) lp[i] = (long)data[i];
            break;
        }
        case UDA_TYPE_LONG64: {
            auto lp = (long long int*)getSyntheticData(handle);
            for (int i = 0; i < udaGetDataNum(handle); i++) lp[i] = (long long int)data[i];
            break;
        }
        case UDA_TYPE_UNSIGNED_SHORT: {
            auto sp = (unsigned short*)getSyntheticData(handle);
            for (int i = 0; i < udaGetDataNum(handle); i++) sp[i] = (unsigned short)data[i];
            break;
        }
        case UDA_TYPE_UNSIGNED_INT: {
            auto up = (unsigned int*)getSyntheticData(handle);
            for (int i = 0; i < udaGetDataNum(handle); i++) up[i] = (unsigned int)data[i];
            break;
        }
        case UDA_TYPE_UNSIGNED_LONG: {
            auto lp = (unsigned long*)getSyntheticData(handle);
            for (int i = 0; i < udaGetDataNum(handle); i++) lp[i] = (unsigned long)data[i];
            break;
        }
        case UDA_TYPE_UNSIGNED_LONG64: {
            auto lp = (unsigned long long int *)getSyntheticData(handle);
            for (int i = 0; i < udaGetDataNum(handle); i++) lp[i] = (unsigned long long int) data[i];
            break;
        }
        case UDA_TYPE_CHAR: {
            auto cp = (char*)getSyntheticData(handle);
            for (int i = 0; i < udaGetDataNum(handle); i++) cp[i] = (char)data[i];
            break;
        }
        case UDA_TYPE_UNSIGNED_CHAR: {
            auto cp = (unsigned char*)getSyntheticData(handle);
            for (int i = 0; i < udaGetDataNum(handle); i++) cp[i] = (unsigned char)data[i];
            break;
        }

    }

    //--------------------------------------------------------------------------------------------------------------
    // Housekeeping

    free(data);

    return 0;
}

int uda::client::generate_synthetic_dim_data(int handle, int ndim)
{
    int err = 0;

    char* synthetic = nullptr;

    //--------------------------------------------------------------------------------------------------------------
    // Check the handle and model are ok

    if (udaGetData(handle) == nullptr) return 0;            // No Data Block available

    if (ndim < 0 || ndim > udaGetRank(handle)) return 0;

    int model;
    int param_n;
    float params[MAXERRPARAMS];

    udaGetErrorModel(handle, &model, &param_n, params);

    if (model <= ERROR_MODEL_UNKNOWN || model >= ERROR_MODEL_UNDEFINED) return 0; // No valid Model

    if (udaGetDimNum(handle, ndim) <= 0) return 0;

    //--------------------------------------------------------------------------------------------------------------
    // Allocate local float work arrays and copy the data array to the work array

    if (udaGetDataType(handle) == UDA_TYPE_DCOMPLEX || udaGetDataType(handle) == UDA_TYPE_COMPLEX) {
        err = 999;
        addIdamError(CODEERRORTYPE, "generate_synthetic_dim_data", err,
                     "Not configured to Generate Complex Type Synthetic Data");
        return 999;
    }

    float* data;
    if ((data = (float*)malloc(udaGetDimNum(handle, ndim) * sizeof(float))) == nullptr) {
        addIdamError(CODEERRORTYPE, "generate_synthetic_dim_data", 1,
                     "Problem Allocating Heap Memory for Synthetic Dimensional Data");
        return 1;
    }

    switch (udaGetDataType(handle)) {
        case UDA_TYPE_FLOAT: {
            auto fp = (float*)udaGetDimData(handle, ndim);
            for (int i = 0; i < udaGetDimNum(handle, ndim); i++) data[i] = fp[i];        // Cast all data to Float
            break;
        }
        case UDA_TYPE_DOUBLE: {
            auto dp = (double*)udaGetDimData(handle, ndim);
            for (int i = 0; i < udaGetDimNum(handle, ndim); i++) data[i] = (float)dp[i];
            break;
        }
        case UDA_TYPE_INT: {
            auto ip = (int*)udaGetDimData(handle, ndim);
            for (int i = 0; i < udaGetDimNum(handle, ndim); i++) data[i] = (float)ip[i];
            break;
        }
        case UDA_TYPE_SHORT: {
            auto sp = (short*)udaGetDimData(handle, ndim);
            for (int i = 0; i < udaGetDimNum(handle, ndim); i++) data[i] = (float)sp[i];
            break;
        }
        case UDA_TYPE_LONG: {
            auto lp = (long*)udaGetDimData(handle, ndim);
            for (int i = 0; i < udaGetDimNum(handle, ndim); i++) data[i] = (float)lp[i];
            break;
        }
        case UDA_TYPE_LONG64: {
            auto lp = (long long int*)udaGetDimData(handle, ndim);
            for (int i = 0; i < udaGetDimNum(handle, ndim); i++) data[i] = (float)lp[i];
            break;
        }
        case UDA_TYPE_CHAR: {
            char* cp = (char*)udaGetDimData(handle, ndim);
            for (int i = 0; i < udaGetDimNum(handle, ndim); i++) data[i] = (float)cp[i];
            break;
        }
        case UDA_TYPE_UNSIGNED_SHORT: {
            auto sp = (unsigned short*)udaGetDimData(handle, ndim);
            for (int i = 0; i < udaGetDimNum(handle, ndim); i++) data[i] = (float)sp[i];
            break;
        }
        case UDA_TYPE_UNSIGNED_INT: {
            auto up = (unsigned int*)udaGetDimData(handle, ndim);
            for (int i = 0; i < udaGetDimNum(handle, ndim); i++) data[i] = (float)up[i];
            break;
        }
        case UDA_TYPE_UNSIGNED_LONG: {
            auto lp = (unsigned long*)udaGetDimData(handle, ndim);
            for (int i = 0; i < udaGetDimNum(handle, ndim); i++) data[i] = (float)lp[i];
            break;
        }
        case UDA_TYPE_UNSIGNED_LONG64: {
            auto lp = (unsigned long long int *)udaGetDimData(handle, ndim);
            for (int i = 0; i < udaGetDimNum(handle, ndim); i++) data[i] = (float) lp[i];
            break;
        }
        case UDA_TYPE_UNSIGNED_CHAR: {
            auto cp = (unsigned char*)udaGetDimData(handle, ndim);
            for (int i = 0; i < udaGetDimNum(handle, ndim); i++) data[i] = (float)cp[i];
            break;
        }
        default:
            free(data);
            return 0;
    }

    //--------------------------------------------------------------------------------------------------------------
    // Generate Model Data

    err = synthetic_model(model, param_n, params, udaGetDimNum(handle, ndim), data);

    if (err != 0) {
        addIdamError(CODEERRORTYPE, "generate_synthetic_dim_data", err,
                     "Unable to Generate Synthetic Dimensional Data");
        free(data);
        return err;
    }

    //--------------------------------------------------------------------------------------------------------------
    // Return the Synthetic Data

    if (udaGetSyntheticDimData(handle, ndim) == nullptr) {
        if ((err = allocArray(udaGetDimType(handle, ndim), udaGetDimNum(handle, ndim), &synthetic))) {
            addIdamError(CODEERRORTYPE, "generate_synthetic_dim_data", err,
                         "Problem Allocating Heap Memory for Synthetic Dimensional Data");
            return err;
        }

        udaSetSyntheticDimData(handle, ndim, synthetic);
    }

    switch (udaGetDimType(handle, ndim)) {
        case UDA_TYPE_FLOAT: {
            auto fp = (float*)udaGetSyntheticDimData(handle, ndim);
            for (int i = 0; i < udaGetDimNum(handle, ndim); i++) fp[i] = data[i];
            break;
        }
        case UDA_TYPE_DOUBLE: {
            auto dp = (double*)udaGetSyntheticDimData(handle, ndim);
            for (int i = 0; i < udaGetDimNum(handle, ndim); i++) dp[i] = (double)data[i];
            break;
        }
        case UDA_TYPE_SHORT: {
            auto sp = (short*)udaGetSyntheticDimData(handle, ndim);
            for (int i = 0; i < udaGetDimNum(handle, ndim); i++) sp[i] = (short)data[i];
            break;
        }
        case UDA_TYPE_INT: {
            auto ip = (int*)udaGetSyntheticDimData(handle, ndim);
            for (int i = 0; i < udaGetDimNum(handle, ndim); i++) ip[i] = (int)data[i];
            break;
        }
        case UDA_TYPE_LONG: {
            auto lp = (long*)udaGetSyntheticDimData(handle, ndim);
            for (int i = 0; i < udaGetDimNum(handle, ndim); i++) lp[i] = (long)data[i];
            break;
        }
        case UDA_TYPE_LONG64: {
            auto lp = (long long int*)udaGetSyntheticDimData(handle, ndim);
            for (int i = 0; i < udaGetDimNum(handle, ndim); i++) lp[i] = (long long int)data[i];
            break;
        }
        case UDA_TYPE_CHAR: {
            auto cp = udaGetSyntheticDimData(handle, ndim);
            for (int i = 0; i < udaGetDimNum(handle, ndim); i++) cp[i] = (char)data[i];
            break;
        }
        case UDA_TYPE_UNSIGNED_SHORT: {
            auto sp = (unsigned short*)udaGetSyntheticDimData(handle, ndim);
            for (int i = 0; i < udaGetDimNum(handle, ndim); i++) sp[i] = (unsigned short)data[i];
            break;
        }
        case UDA_TYPE_UNSIGNED_INT: {
            auto up = (unsigned int*)udaGetSyntheticDimData(handle, ndim);
            for (int i = 0; i < udaGetDimNum(handle, ndim); i++) up[i] = (unsigned int)data[i];
            break;
        }
        case UDA_TYPE_UNSIGNED_LONG: {
            auto lp = (unsigned long*)udaGetSyntheticDimData(handle, ndim);
            for (int i = 0; i < udaGetDimNum(handle, ndim); i++) lp[i] = (unsigned long)data[i];
            break;
        }
        case UDA_TYPE_UNSIGNED_LONG64: {
            auto lp = (unsigned long long int *)udaGetSyntheticDimData(handle, ndim);
            for (int i = 0; i < udaGetDimNum(handle, ndim); i++) lp[i] = (unsigned long long int) data[i];
            break;
        }
        case UDA_TYPE_UNSIGNED_CHAR: {
            auto cp = (unsigned char*)udaGetSyntheticDimData(handle, ndim);
            for (int i = 0; i < udaGetDimNum(handle, ndim); i++) cp[i] = (unsigned char)data[i];
            break;
        }
    }

    //--------------------------------------------------------------------------------------------------------------
    // Housekeeping

    free(data);

    return 0;
}

int uda::client::generate_data_error(int handle)
{
    int err = 0, asymmetry = 0;

    char* perrlo = nullptr;

    //--------------------------------------------------------------------------------------------------------------
    // Check the handle and model are ok

    if (udaGetData(handle) == nullptr) return 0;            // No Data Block available

    int model;
    int param_n;
    float params[MAXERRPARAMS];

    udaGetErrorModel(handle, &model, &param_n, params);

    if (model <= ERROR_MODEL_UNKNOWN || model >= ERROR_MODEL_UNDEFINED) return 0;    // No valid Model

    if (udaGetDataNum(handle) <= 0) return 0;

    //--------------------------------------------------------------------------------------------------------------
    // Allocate local float work arrays and copy the data array to the work array

    if (udaGetDataType(handle) == UDA_TYPE_DCOMPLEX || udaGetDataType(handle) == UDA_TYPE_COMPLEX) {
        err = 999;
        addIdamError(CODEERRORTYPE, "generate_data_error", err,
                     "Not configured to Generate Complex Type Synthetic Data");
        return 999;
    }

    float* data;
    char* errhi;
    char* errlo;

    if ((data = (float*)malloc(udaGetDataNum(handle) * sizeof(float))) == nullptr) return 1;
    if ((errhi = (char*)malloc(udaGetDataNum(handle) * sizeof(float))) == nullptr) return 1;
    if ((errlo = (char*)malloc(udaGetDataNum(handle) * sizeof(float))) == nullptr) return 1;

    switch (udaGetDataType(handle)) {
        case UDA_TYPE_FLOAT: {
            auto fp = (float*)udaGetData(handle);
            for (int i = 0; i < udaGetDataNum(handle); i++) data[i] = fp[i];        // Cast all data to Float
            break;
        }
        case UDA_TYPE_DOUBLE: {
            auto dp = (double*)udaGetData(handle);
            for (int i = 0; i < udaGetDataNum(handle); i++) data[i] = (float)dp[i];
            break;
        }
        case UDA_TYPE_INT: {
            auto ip = (int*)udaGetData(handle);
            for (int i = 0; i < udaGetDataNum(handle); i++) data[i] = (float)ip[i];
            break;
        }
        case UDA_TYPE_UNSIGNED_INT: {
            auto up = (unsigned int*)udaGetData(handle);
            for (int i = 0; i < udaGetDataNum(handle); i++) data[i] = (float)up[i];
            break;
        }
        case UDA_TYPE_LONG: {
            auto lp = (long*)udaGetData(handle);
            for (int i = 0; i < udaGetDataNum(handle); i++) data[i] = (float)lp[i];
            break;
        }
        case UDA_TYPE_LONG64: {
            auto lp = (long long int*)udaGetData(handle);
            for (int i = 0; i < udaGetDataNum(handle); i++) data[i] = (float)lp[i];
            break;
        }
        case UDA_TYPE_SHORT: {
            auto sp = (short*)udaGetData(handle);
            for (int i = 0; i < udaGetDataNum(handle); i++) data[i] = (float)sp[i];
            break;
        }
        case UDA_TYPE_CHAR: {
            char* cp = (char*)udaGetData(handle);
            for (int i = 0; i < udaGetDataNum(handle); i++) data[i] = (float)cp[i];
            break;
        }
        case UDA_TYPE_UNSIGNED_LONG: {
            auto lp = (unsigned long*)udaGetData(handle);
            for (int i = 0; i < udaGetDataNum(handle); i++) data[i] = (float)lp[i];
            break;
        }
        case UDA_TYPE_UNSIGNED_LONG64: {
            auto lp = (unsigned long long int *)udaGetData(handle);
            for (int i = 0; i < udaGetDataNum(handle); i++) data[i] = (float) lp[i];
            break;
        }
        case UDA_TYPE_UNSIGNED_SHORT: {
            auto sp = (unsigned short*)udaGetData(handle);
            for (int i = 0; i < udaGetDataNum(handle); i++) data[i] = (float)sp[i];
            break;
        }
        case UDA_TYPE_UNSIGNED_CHAR: {
            auto cp = (unsigned char*)udaGetData(handle);
            for (int i = 0; i < udaGetDataNum(handle); i++) data[i] = (float)cp[i];
            break;
        }
        default:
            free(data);
            free(errhi);
            free(errlo);
            return 0;
    }

    //--------------------------------------------------------------------------------------------------------------
    // Generate Error Data

    err = error_model(model, param_n, params, udaGetDataNum(handle), data, &asymmetry, (float*)errhi,
                      (float*)errlo);

    if (err != 0) {
        free(data);
        free(errhi);
        free(errlo);
        return err;
    }

    //--------------------------------------------------------------------------------------------------------------
    // Return the Error Array

    udaSetDataErrType(handle, udaGetDataType(handle));
    udaSetDataErrAsymmetry(handle, asymmetry);

    if (asymmetry && udaGetDataErrLo(handle) == nullptr) {
        if ((err = allocArray(udaGetDataType(handle), udaGetDataNum(handle), &perrlo))) return err;
        udaSetDataErrLo(handle, perrlo);
    }

    switch (udaGetDataType(handle)) {
        case UDA_TYPE_FLOAT: {
            auto feh = (float*)udaGetDataErrHi(handle);
            auto fel = (float*)udaGetDataErrLo(handle);
            for (int i = 0; i < udaGetDataNum(handle); i++) {
                feh[i] = (float)errhi[i];
                if (udaGetDataErrAsymmetry(handle)) fel[i] = (float)errlo[i];
            }
            break;
        }
        case UDA_TYPE_DOUBLE: {
            auto deh = (double*)udaGetDataErrHi(handle);
            auto del = (double*)udaGetDataErrLo(handle);
            for (int i = 0; i < udaGetDataNum(handle); i++) {
                deh[i] = (double)errhi[i];
                if (udaGetDataErrAsymmetry(handle)) del[i] = (double)errlo[i];
            }
            break;
        }
        case UDA_TYPE_INT: {
            auto ieh = (int*)udaGetDataErrHi(handle);
            auto iel = (int*)udaGetDataErrLo(handle);
            for (int i = 0; i < udaGetDataNum(handle); i++) {
                ieh[i] = (int)errhi[i];
                if (udaGetDataErrAsymmetry(handle)) iel[i] = (int)errlo[i];
            }
            break;
        }
        case UDA_TYPE_LONG: {
            auto leh = (long*)udaGetDataErrHi(handle);
            auto lel = (long*)udaGetDataErrLo(handle);
            for (int i = 0; i < udaGetDataNum(handle); i++) {
                leh[i] = (long)errhi[i];
                if (udaGetDataErrAsymmetry(handle)) lel[i] = (long)errlo[i];
            }
            break;
        }
        case UDA_TYPE_LONG64: {
            auto leh = (long long int*)udaGetDataErrHi(handle);
            auto lel = (long long int*)udaGetDataErrLo(handle);
            for (int i = 0; i < udaGetDataNum(handle); i++) {
                leh[i] = (long long int)errhi[i];
                if (udaGetDataErrAsymmetry(handle)) lel[i] = (long long int)errlo[i];
            }
            break;
        }
        case UDA_TYPE_SHORT: {
            auto seh = (short*)udaGetDataErrHi(handle);
            auto sel = (short*)udaGetDataErrLo(handle);
            for (int i = 0; i < udaGetDataNum(handle); i++) {
                seh[i] = (short)errhi[i];
                if (udaGetDataErrAsymmetry(handle)) sel[i] = (short)errlo[i];
            }
            break;
        }
        case UDA_TYPE_CHAR: {
            char* ceh = udaGetDataErrHi(handle);
            char* cel = udaGetDataErrLo(handle);
            for (int i = 0; i < udaGetDataNum(handle); i++) {
                ceh[i] = errhi[i];
                if (udaGetDataErrAsymmetry(handle)) cel[i] = errlo[i];
            }
            break;
        }
        case UDA_TYPE_UNSIGNED_INT: {
            auto ueh = (unsigned int*)udaGetDataErrHi(handle);
            auto uel = (unsigned int*)udaGetDataErrLo(handle);
            for (int i = 0; i < udaGetDataNum(handle); i++) {
                ueh[i] = (unsigned int)errhi[i];
                if (udaGetDataErrAsymmetry(handle)) uel[i] = (unsigned int)errlo[i];
            }
            break;
        }
        case UDA_TYPE_UNSIGNED_LONG: {
            auto leh = (unsigned long*)udaGetDataErrHi(handle);
            auto lel = (unsigned long*)udaGetDataErrLo(handle);
            for (int i = 0; i < udaGetDataNum(handle); i++) {
                leh[i] = (unsigned long)errhi[i];
                if (udaGetDataErrAsymmetry(handle)) lel[i] = (unsigned long)errlo[i];
            }
            break;
        }
        case UDA_TYPE_UNSIGNED_LONG64: {
            auto leh = (unsigned long long int *)udaGetDataErrHi(handle);
            auto lel = (unsigned long long int *)udaGetDataErrLo(handle);
            for (int i = 0; i < udaGetDataNum(handle); i++) {
                leh[i] = (unsigned long long int) errhi[i];
                if (udaGetDataErrAsymmetry(handle)) lel[i] = (unsigned long long int) errlo[i];
            }
            break;
        }
        case UDA_TYPE_UNSIGNED_SHORT: {
            auto seh = (unsigned short*)udaGetDataErrHi(handle);
            auto sel = (unsigned short*)udaGetDataErrLo(handle);
            for (int i = 0; i < udaGetDataNum(handle); i++) {
                seh[i] = (unsigned short)errhi[i];
                if (udaGetDataErrAsymmetry(handle)) sel[i] = (unsigned short)errlo[i];
            }
            break;
        }
        case UDA_TYPE_UNSIGNED_CHAR: {
            auto ceh = (unsigned char*)udaGetDataErrHi(handle);
            auto cel = (unsigned char*)udaGetDataErrLo(handle);
            for (int i = 0; i < udaGetDataNum(handle); i++) {
                ceh[i] = (unsigned char)errhi[i];
                if (udaGetDataErrAsymmetry(handle)) cel[i] = (unsigned char)errlo[i];
            }
            break;
        }

    }

    //--------------------------------------------------------------------------------------------------------------
    // Housekeeping

    free(data);
    free(errhi);
    free(errlo);

    return 0;
}

int uda::client::generate_dim_data_error(int handle, int ndim)
{

    int err = 0, asymmetry = 0;

    char* perrlo = nullptr;

    //--------------------------------------------------------------------------------------------------------------
    // Check the handle and model are ok

    if (udaGetData(handle) == nullptr) return 0;            // No Data Block available

    if (ndim < 0 || ndim > udaGetRank(handle)) return 0;

    int model;
    int param_n;
    float params[MAXERRPARAMS];

    udaGetErrorModel(handle, &model, &param_n, params);

    if (model <= ERROR_MODEL_UNKNOWN || model >= ERROR_MODEL_UNDEFINED) return 0;    // No valid Model

    if (udaGetDimNum(handle, ndim) <= 0) return 0;

    //--------------------------------------------------------------------------------------------------------------
    // Allocate local float work arrays and copy the data array to the work array

    if (udaGetDataType(handle) == UDA_TYPE_DCOMPLEX || udaGetDataType(handle) == UDA_TYPE_COMPLEX) {
        err = 999;
        addIdamError(CODEERRORTYPE, "generate_dim_data_error", err,
                     "Not configured to Generate Complex Type Synthetic Data");
        return 999;
    }

    float* data;
    char* errhi;
    char* errlo;

    if ((data = (float*)malloc(udaGetDimNum(handle, ndim) * sizeof(float))) == nullptr) return 1;
    if ((errhi = (char*)malloc(udaGetDimNum(handle, ndim) * sizeof(float))) == nullptr) return 1;
    if ((errlo = (char*)malloc(udaGetDimNum(handle, ndim) * sizeof(float))) == nullptr) return 1;

    switch (udaGetDataType(handle)) {
        case UDA_TYPE_FLOAT: {
            auto fp = (float*)udaGetDimData(handle, ndim);
            for (int i = 0; i < udaGetDimNum(handle, ndim); i++) data[i] = fp[i];        // Cast all data to Float
            break;
        }
        case UDA_TYPE_DOUBLE: {
            auto dp = (double*)udaGetDimData(handle, ndim);
            for (int i = 0; i < udaGetDimNum(handle, ndim); i++) data[i] = (float)dp[i];
            break;
        }
        case UDA_TYPE_INT: {
            auto ip = (int*)udaGetDimData(handle, ndim);
            for (int i = 0; i < udaGetDimNum(handle, ndim); i++) data[i] = (float)ip[i];
            break;
        }
        case UDA_TYPE_LONG: {
            auto lp = (long*)udaGetDimData(handle, ndim);
            for (int i = 0; i < udaGetDimNum(handle, ndim); i++) data[i] = (float)lp[i];
            break;
        }
        case UDA_TYPE_LONG64: {
            auto lp = (long long int*)udaGetDimData(handle, ndim);
            for (int i = 0; i < udaGetDimNum(handle, ndim); i++) data[i] = (float)lp[i];
            break;
        }
        case UDA_TYPE_SHORT: {
            auto sp = (short*)udaGetDimData(handle, ndim);
            for (int i = 0; i < udaGetDimNum(handle, ndim); i++) data[i] = (float)sp[i];
            break;
        }
        case UDA_TYPE_CHAR: {
            char* cp = udaGetDimData(handle, ndim);
            for (int i = 0; i < udaGetDimNum(handle, ndim); i++) data[i] = (float)cp[i];
            break;
        }
        case UDA_TYPE_UNSIGNED_INT: {
            auto up = (unsigned int*)udaGetDimData(handle, ndim);
            for (int i = 0; i < udaGetDimNum(handle, ndim); i++) data[i] = (float)up[i];
            break;
        }
        case UDA_TYPE_UNSIGNED_LONG: {
            auto lp = (unsigned long*)udaGetDimData(handle, ndim);
            for (int i = 0; i < udaGetDimNum(handle, ndim); i++) data[i] = (float)lp[i];
            break;
        }
        case UDA_TYPE_UNSIGNED_LONG64: {
            auto lp = (unsigned long long int *)udaGetDimData(handle, ndim);
            for (int i = 0; i < udaGetDimNum(handle, ndim); i++) data[i] = (float) lp[i];
            break;
        }
        case UDA_TYPE_UNSIGNED_SHORT: {
            auto sp = (unsigned short*)udaGetDimData(handle, ndim);
            for (int i = 0; i < udaGetDimNum(handle, ndim); i++) data[i] = (float)sp[i];
            break;
        }
        case UDA_TYPE_UNSIGNED_CHAR: {
            auto cp = (unsigned char*)udaGetDimData(handle, ndim);
            for (int i = 0; i < udaGetDimNum(handle, ndim); i++) data[i] = (float)cp[i];
            break;
        }
        default:
            free(data);
            free(errhi);
            free(errlo);
            return 0;
    }

    //--------------------------------------------------------------------------------------------------------------
    // Generate Model Data

    err = error_model(model, param_n, params, udaGetDimNum(handle, ndim), data, &asymmetry, (float*)errhi,
                      (float*)errlo);

    if (err != 0) {
        free(data);
        free(errhi);
        free(errlo);
        return err;
    }

    //--------------------------------------------------------------------------------------------------------------
    // Return the Synthetic Data and Error Array

    udaSetDimErrType(handle, ndim, udaGetDimType(handle, ndim));
    udaSetDimErrAsymmetry(handle, ndim, asymmetry);

    if (udaGetDimErrAsymmetry(handle, ndim) && udaGetDimErrLo(handle, ndim) == nullptr) {
        if ((err = allocArray(udaGetDimType(handle, ndim), udaGetDimNum(handle, ndim), &perrlo))) return err;
        udaSetDimErrLo(handle, ndim, perrlo);
    }

    switch (udaGetDimType(handle, ndim)) {
        case UDA_TYPE_FLOAT: {
            auto feh = (float*)udaGetDimErrHi(handle, ndim);
            auto fel = (float*)udaGetDimErrLo(handle, ndim);
            for (int i = 0; i < udaGetDimNum(handle, ndim); i++) {
                feh[i] = (float)errhi[i];
                if (udaGetDimErrAsymmetry(handle, ndim))fel[i] = (float)errlo[i];
            }
            break;
        }
        case UDA_TYPE_DOUBLE: {
            auto deh = (double*)udaGetDimErrHi(handle, ndim);
            auto del = (double*)udaGetDimErrLo(handle, ndim);
            for (int i = 0; i < udaGetDimNum(handle, ndim); i++) {
                deh[i] = (double)errhi[i];
                if (udaGetDimErrAsymmetry(handle, ndim))del[i] = (double)errlo[i];
            }
            break;
        }
        case UDA_TYPE_INT: {
            auto ieh = (int*)udaGetDimErrHi(handle, ndim);
            auto iel = (int*)udaGetDimErrLo(handle, ndim);
            for (int i = 0; i < udaGetDimNum(handle, ndim); i++) {
                ieh[i] = (int)errhi[i];
                if (udaGetDimErrAsymmetry(handle, ndim))iel[i] = (int)errlo[i];
            }
            break;
        }
        case UDA_TYPE_UNSIGNED_INT: {
            auto ueh = (unsigned int*)udaGetDimErrHi(handle, ndim);
            auto uel = (unsigned int*)udaGetDimErrLo(handle, ndim);
            for (int i = 0; i < udaGetDimNum(handle, ndim); i++) {
                ueh[i] = (unsigned int)errhi[i];
                if (udaGetDimErrAsymmetry(handle, ndim))uel[i] = (unsigned int)errlo[i];
            }
            break;
        }
        case UDA_TYPE_LONG: {
            auto leh = (long*)udaGetDimErrHi(handle, ndim);
            auto lel = (long*)udaGetDimErrLo(handle, ndim);
            for (int i = 0; i < udaGetDimNum(handle, ndim); i++) {
                leh[i] = (long)errhi[i];
                if (udaGetDimErrAsymmetry(handle, ndim))lel[i] = (long)errlo[i];
            }
            break;
        }
        case UDA_TYPE_LONG64: {
            auto leh = (long long int*)udaGetDimErrHi(handle, ndim);
            auto lel = (long long int*)udaGetDimErrLo(handle, ndim);
            for (int i = 0; i < udaGetDimNum(handle, ndim); i++) {
                leh[i] = (long long int)errhi[i];
                if (udaGetDimErrAsymmetry(handle, ndim))lel[i] = (long long int)errlo[i];
            }
            break;
        }
        case UDA_TYPE_SHORT: {
            auto seh = (short*)udaGetDimErrHi(handle, ndim);
            auto sel = (short*)udaGetDimErrLo(handle, ndim);
            for (int i = 0; i < udaGetDimNum(handle, ndim); i++) {
                seh[i] = (short)errhi[i];
                if (udaGetDimErrAsymmetry(handle, ndim))sel[i] = (short)errlo[i];
            }
            break;
        }
        case UDA_TYPE_CHAR: {
            char* ceh = (char*)udaGetDimErrHi(handle, ndim);
            char* cel = (char*)udaGetDimErrLo(handle, ndim);
            for (int i = 0; i < udaGetDimNum(handle, ndim); i++) {
                ceh[i] = (char)errhi[i];
                if (udaGetDimErrAsymmetry(handle, ndim))cel[i] = errlo[i];
            }
            break;
        }
        case UDA_TYPE_UNSIGNED_LONG: {
            auto leh = (unsigned long*)udaGetDimErrHi(handle, ndim);
            auto lel = (unsigned long*)udaGetDimErrLo(handle, ndim);
            for (int i = 0; i < udaGetDimNum(handle, ndim); i++) {
                leh[i] = (unsigned long)errhi[i];
                if (udaGetDimErrAsymmetry(handle, ndim))lel[i] = (unsigned long)errlo[i];
            }
            break;
        }
        case UDA_TYPE_UNSIGNED_LONG64: {
            auto leh = (unsigned long long int *)udaGetDimErrHi(handle, ndim);
            auto lel = (unsigned long long int *)udaGetDimErrLo(handle, ndim);
            for (int i = 0; i < udaGetDimNum(handle, ndim); i++) {
                leh[i] = (unsigned long long int) errhi[i];
                if (udaGetDimErrAsymmetry(handle, ndim))lel[i] = (unsigned long long int) errlo[i];
            }
            break;
        }
        case UDA_TYPE_UNSIGNED_SHORT: {
            auto seh = (unsigned short*)udaGetDimErrHi(handle, ndim);
            auto sel = (unsigned short*)udaGetDimErrLo(handle, ndim);
            for (int i = 0; i < udaGetDimNum(handle, ndim); i++) {
                seh[i] = (unsigned short)errhi[i];
                if (udaGetDimErrAsymmetry(handle, ndim))sel[i] = (unsigned short)errlo[i];
            }
            break;
        }
        case UDA_TYPE_UNSIGNED_CHAR: {
            auto ceh = (unsigned char*)udaGetDimErrHi(handle, ndim);
            auto cel = (unsigned char*)udaGetDimErrLo(handle, ndim);
            for (int i = 0; i < udaGetDimNum(handle, ndim); i++) {
                ceh[i] = (unsigned char)errhi[i];
                if (udaGetDimErrAsymmetry(handle, ndim))cel[i] = (unsigned char)errlo[i];
            }
            break;
        }
    }

    //--------------------------------------------------------------------------------------------------------------
    // Housekeeping

    free(data);
    free(errhi);
    free(errlo);

    return 0;
}

