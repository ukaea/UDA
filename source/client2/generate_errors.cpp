#include "generate_errors.hpp"

#include <cmath>
#include <cstdlib>

#include "udaTypes.h"
#include <clientserver/allocData.h>
#include <clientserver/errorLog.h>

#include "accAPI.h"
#include "thread_client.hpp"

#ifndef NO_GSL_LIB
#  include <gsl/gsl_randist.h>
#endif

//--------------------------------------------------------------------------------------------------------------
// Generate Error Data

int uda::client::error_model(int model, int param_n, float* params, int data_n, float* data, int* asymmetry,
                             float* errhi, float* errlo)
{
    *asymmetry = 0; // No Error Asymmetry for most models

    switch (model) {

        case ERROR_MODEL_DEFAULT:
            if (param_n != 2) {
                return 1;
            }
            for (int i = 0; i < data_n; i++) {
                errhi[i] = params[0] + params[1] * fabsf(data[i]);
                errlo[i] = errhi[i];
            }
            break;

        case ERROR_MODEL_DEFAULT_ASYMMETRIC:
            if (param_n != 4) {
                return 1;
            }
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
    udaAddError(UDA_CODE_ERROR_TYPE, "synthetic_model", err,
                 "Random Number Generators from the GSL library required.");
    return 999;
#else
    float shift;
    static gsl_rng* random = nullptr;

    if (random == nullptr) { // Initialise the Random Number System
        gsl_rng_env_setup();
        random = gsl_rng_alloc(gsl_rng_default);
        gsl_rng_set(random,
                    (unsigned long int)ERROR_MODEL_SEED); // Seed the Random Number generator with the library default
    }

    switch (model) {

        case ERROR_MODEL_GAUSSIAN: // Standard normal Distribution
            if (param_n < 1 || param_n > 2) {
                return 1;
            }
            if (param_n == 2) {
                gsl_rng_set(random, (unsigned long int)params[1]); // Change the Seed before Sampling
            }
            for (i = 0; i < data_n; i++) {
                data[i] = data[i] + (float)gsl_ran_gaussian(random, (double)params[0]); // Random sample from PDF
            }
            break;

        case ERROR_MODEL_RESEED: // Change the Seed
            if (param_n == 1) {
                gsl_rng_set(random, (unsigned long int)params[0]);
            }
            break;

        case ERROR_MODEL_GAUSSIAN_SHIFT:
            if (param_n < 1 || param_n > 2) {
                return 1;
            }
            if (param_n == 2) {
                gsl_rng_set(random, (unsigned long int)params[1]); // Change the Seed before Sampling
            }
            shift = (float)gsl_ran_gaussian(random, (double)params[0]);
            for (i = 0; i < data_n; i++) {
                data[i] = data[i] + shift; // Random linear shift of data array
            }
            break;

        case ERROR_MODEL_POISSON:
            if (param_n < 0 || param_n > 1) {
                return 1;
            }
            if (param_n == 1) {
                gsl_rng_set(random, (unsigned long int)params[0]); // Change the Seed before Sampling
            }
            for (i = 0; i < data_n; i++) {
                data[i] = data[i] + (float)gsl_ran_poisson(random, (double)data[i]); // Randomly perturb data array
            }
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

    if (get_data(handle) == nullptr) {
        return 0;
    }

    int model;
    int param_n;
    float params[MAXERRPARAMS];

    get_error_model(handle, &model, &param_n, params);

    if (model <= ERROR_MODEL_UNKNOWN || model >= ERROR_MODEL_UNDEFINED) {
        return 0; // No valid Model
    }

    if (get_data_num(handle) <= 0) {
        return 0;
    }

    //--------------------------------------------------------------------------------------------------------------
    // Allocate local float work arrays and copy the data array to the work array

    if (get_data_type(handle) == UDA_TYPE_DCOMPLEX || get_data_type(handle) == UDA_TYPE_COMPLEX) {
        err = 999;
        udaAddError(UDA_CODE_ERROR_TYPE, "generate_synthetic_data", err,
                     "Not configured to Generate Complex Type Synthetic Data");
        return 999;
    }

    float* data = nullptr;
    if ((data = (float*)malloc(get_data_num(handle) * sizeof(float))) == nullptr) {
        return 1;
    }

    switch (get_data_type(handle)) {
        case UDA_TYPE_FLOAT: {
            auto fp = (float*)get_data(handle);
            for (int i = 0; i < get_data_num(handle); i++) {
                data[i] = fp[i]; // Cast all data to Float
            }
            break;
        }
        case UDA_TYPE_DOUBLE: {
            auto dp = (double*)get_data(handle);
            for (int i = 0; i < get_data_num(handle); i++) {
                data[i] = (float)dp[i];
            }
            break;
        }
        case UDA_TYPE_SHORT: {
            auto sp = (short*)get_data(handle);
            for (int i = 0; i < get_data_num(handle); i++) {
                data[i] = (float)sp[i];
            }
            break;
        }
        case UDA_TYPE_INT: {
            auto ip = (int*)get_data(handle);
            for (int i = 0; i < get_data_num(handle); i++) {
                data[i] = (float)ip[i];
            }
            break;
        }
        case UDA_TYPE_LONG: {
            auto lp = (long*)get_data(handle);
            for (int i = 0; i < get_data_num(handle); i++) {
                data[i] = (float)lp[i];
            }
            break;
        }
        case UDA_TYPE_LONG64: {
            auto lp = (long long int*)get_data(handle);
            for (int i = 0; i < get_data_num(handle); i++) {
                data[i] = (float)lp[i];
            }
            break;
        }
        case UDA_TYPE_CHAR: {
            auto cp = (char*)get_data(handle);
            for (int i = 0; i < get_data_num(handle); i++) {
                data[i] = (float)cp[i];
            }
            break;
        }
        case UDA_TYPE_UNSIGNED_SHORT: {
            auto sp = (unsigned short*)get_data(handle);
            for (int i = 0; i < get_data_num(handle); i++) {
                data[i] = (float)sp[i];
            }
            break;
        }
        case UDA_TYPE_UNSIGNED_INT: {
            auto up = (unsigned int*)get_data(handle);
            for (int i = 0; i < get_data_num(handle); i++) {
                data[i] = (float)up[i];
            }
            break;
        }
        case UDA_TYPE_UNSIGNED_LONG: {
            auto lp = (unsigned long*)get_data(handle);
            for (int i = 0; i < get_data_num(handle); i++) {
                data[i] = (float)lp[i];
            }
            break;
        }
        case UDA_TYPE_UNSIGNED_LONG64: {
            auto lp = (unsigned long long int*)get_data(handle);
            for (int i = 0; i < get_data_num(handle); i++) {
                data[i] = (float)lp[i];
            }
            break;
        }
        case UDA_TYPE_UNSIGNED_CHAR: {
            auto cp = (unsigned char*)get_data(handle);
            for (int i = 0; i < get_data_num(handle); i++) {
                data[i] = (float)cp[i];
            }
            break;
        }

        default:
            free(data);
            return 0;
    }

    //--------------------------------------------------------------------------------------------------------------
    // Generate Synthetic Data

    err = synthetic_model(model, param_n, params, get_data_num(handle), data);

    if (err != 0) {
        udaAddError(UDA_CODE_ERROR_TYPE, "generate_synthetic_data", err, "Unable to Generate Synthetic Data");
        free(data);
        return err;
    }

    //--------------------------------------------------------------------------------------------------------------
    // Return the Synthetic Data

    if (getSyntheticData(handle) == nullptr) {
        if ((err = allocArray(get_data_type(handle), get_data_num(handle), &synthetic))) {
            udaAddError(UDA_CODE_ERROR_TYPE, "generate_synthetic_data", err,
                         "Problem Allocating Heap Memory for Synthetic Data");
            return err;
        }
        set_synthetic_data(handle, synthetic);
    }

    switch (get_data_type(handle)) {
        case UDA_TYPE_FLOAT: {
            auto fp = (float*)getSyntheticData(handle);
            for (int i = 0; i < get_data_num(handle); i++) {
                fp[i] = data[i]; // Overwrite the Data
            }
            break;
        }
        case UDA_TYPE_DOUBLE: {
            auto dp = (double*)getSyntheticData(handle);
            for (int i = 0; i < get_data_num(handle); i++) {
                dp[i] = (double)data[i];
            }
            break;
        }
        case UDA_TYPE_SHORT: {
            auto sp = (short*)getSyntheticData(handle);
            for (int i = 0; i < get_data_num(handle); i++) {
                sp[i] = (short)data[i];
            }
            break;
        }
        case UDA_TYPE_INT: {
            auto ip = (int*)getSyntheticData(handle);
            for (int i = 0; i < get_data_num(handle); i++) {
                ip[i] = (int)data[i];
            }
            break;
        }
        case UDA_TYPE_LONG: {
            auto lp = (long*)getSyntheticData(handle);
            for (int i = 0; i < get_data_num(handle); i++) {
                lp[i] = (long)data[i];
            }
            break;
        }
        case UDA_TYPE_LONG64: {
            auto lp = (long long int*)getSyntheticData(handle);
            for (int i = 0; i < get_data_num(handle); i++) {
                lp[i] = (long long int)data[i];
            }
            break;
        }
        case UDA_TYPE_UNSIGNED_SHORT: {
            auto sp = (unsigned short*)getSyntheticData(handle);
            for (int i = 0; i < get_data_num(handle); i++) {
                sp[i] = (unsigned short)data[i];
            }
            break;
        }
        case UDA_TYPE_UNSIGNED_INT: {
            auto up = (unsigned int*)getSyntheticData(handle);
            for (int i = 0; i < get_data_num(handle); i++) {
                up[i] = (unsigned int)data[i];
            }
            break;
        }
        case UDA_TYPE_UNSIGNED_LONG: {
            auto lp = (unsigned long*)getSyntheticData(handle);
            for (int i = 0; i < get_data_num(handle); i++) {
                lp[i] = (unsigned long)data[i];
            }
            break;
        }
        case UDA_TYPE_UNSIGNED_LONG64: {
            auto lp = (unsigned long long int*)getSyntheticData(handle);
            for (int i = 0; i < get_data_num(handle); i++) {
                lp[i] = (unsigned long long int)data[i];
            }
            break;
        }
        case UDA_TYPE_CHAR: {
            auto cp = (char*)getSyntheticData(handle);
            for (int i = 0; i < get_data_num(handle); i++) {
                cp[i] = (char)data[i];
            }
            break;
        }
        case UDA_TYPE_UNSIGNED_CHAR: {
            auto cp = (unsigned char*)getSyntheticData(handle);
            for (int i = 0; i < get_data_num(handle); i++) {
                cp[i] = (unsigned char)data[i];
            }
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

    if (get_data(handle) == nullptr) {
        return 0; // No Data Block available
    }

    if (ndim < 0 || ndim > get_rank(handle)) {
        return 0;
    }

    int model;
    int param_n;
    float params[MAXERRPARAMS];

    get_error_model(handle, &model, &param_n, params);

    if (model <= ERROR_MODEL_UNKNOWN || model >= ERROR_MODEL_UNDEFINED) {
        return 0; // No valid Model
    }

    if (get_dim_num(handle, ndim) <= 0) {
        return 0;
    }

    //--------------------------------------------------------------------------------------------------------------
    // Allocate local float work arrays and copy the data array to the work array

    if (get_data_type(handle) == UDA_TYPE_DCOMPLEX || get_data_type(handle) == UDA_TYPE_COMPLEX) {
        err = 999;
        udaAddError(UDA_CODE_ERROR_TYPE, "generate_synthetic_dim_data", err,
                     "Not configured to Generate Complex Type Synthetic Data");
        return 999;
    }

    float* data;
    if ((data = (float*)malloc(get_dim_num(handle, ndim) * sizeof(float))) == nullptr) {
        udaAddError(UDA_CODE_ERROR_TYPE, "generate_synthetic_dim_data", 1,
                     "Problem Allocating Heap Memory for Synthetic Dimensional Data");
        return 1;
    }

    switch (get_data_type(handle)) {
        case UDA_TYPE_FLOAT: {
            auto fp = (float*)get_dim_data(handle, ndim);
            for (int i = 0; i < get_dim_num(handle, ndim); i++) {
                data[i] = fp[i]; // Cast all data to Float
            }
            break;
        }
        case UDA_TYPE_DOUBLE: {
            auto dp = (double*)get_dim_data(handle, ndim);
            for (int i = 0; i < get_dim_num(handle, ndim); i++) {
                data[i] = (float)dp[i];
            }
            break;
        }
        case UDA_TYPE_INT: {
            auto ip = (int*)get_dim_data(handle, ndim);
            for (int i = 0; i < get_dim_num(handle, ndim); i++) {
                data[i] = (float)ip[i];
            }
            break;
        }
        case UDA_TYPE_SHORT: {
            auto sp = (short*)get_dim_data(handle, ndim);
            for (int i = 0; i < get_dim_num(handle, ndim); i++) {
                data[i] = (float)sp[i];
            }
            break;
        }
        case UDA_TYPE_LONG: {
            auto lp = (long*)get_dim_data(handle, ndim);
            for (int i = 0; i < get_dim_num(handle, ndim); i++) {
                data[i] = (float)lp[i];
            }
            break;
        }
        case UDA_TYPE_LONG64: {
            auto lp = (long long int*)get_dim_data(handle, ndim);
            for (int i = 0; i < get_dim_num(handle, ndim); i++) {
                data[i] = (float)lp[i];
            }
            break;
        }
        case UDA_TYPE_CHAR: {
            char* cp = (char*)get_dim_data(handle, ndim);
            for (int i = 0; i < get_dim_num(handle, ndim); i++) {
                data[i] = (float)cp[i];
            }
            break;
        }
        case UDA_TYPE_UNSIGNED_SHORT: {
            auto sp = (unsigned short*)get_dim_data(handle, ndim);
            for (int i = 0; i < get_dim_num(handle, ndim); i++) {
                data[i] = (float)sp[i];
            }
            break;
        }
        case UDA_TYPE_UNSIGNED_INT: {
            auto up = (unsigned int*)get_dim_data(handle, ndim);
            for (int i = 0; i < get_dim_num(handle, ndim); i++) {
                data[i] = (float)up[i];
            }
            break;
        }
        case UDA_TYPE_UNSIGNED_LONG: {
            auto lp = (unsigned long*)get_dim_data(handle, ndim);
            for (int i = 0; i < get_dim_num(handle, ndim); i++) {
                data[i] = (float)lp[i];
            }
            break;
        }
        case UDA_TYPE_UNSIGNED_LONG64: {
            auto lp = (unsigned long long int*)get_dim_data(handle, ndim);
            for (int i = 0; i < get_dim_num(handle, ndim); i++) {
                data[i] = (float)lp[i];
            }
            break;
        }
        case UDA_TYPE_UNSIGNED_CHAR: {
            auto cp = (unsigned char*)get_dim_data(handle, ndim);
            for (int i = 0; i < get_dim_num(handle, ndim); i++) {
                data[i] = (float)cp[i];
            }
            break;
        }
        default:
            free(data);
            return 0;
    }

    //--------------------------------------------------------------------------------------------------------------
    // Generate Model Data

    err = synthetic_model(model, param_n, params, get_dim_num(handle, ndim), data);

    if (err != 0) {
        udaAddError(UDA_CODE_ERROR_TYPE, "generate_synthetic_dim_data", err,
                     "Unable to Generate Synthetic Dimensional Data");
        free(data);
        return err;
    }

    //--------------------------------------------------------------------------------------------------------------
    // Return the Synthetic Data

    if (get_synthetic_dim_data(handle, ndim) == nullptr) {
        if ((err = allocArray(get_dim_type(handle, ndim), get_dim_num(handle, ndim), &synthetic))) {
            udaAddError(UDA_CODE_ERROR_TYPE, "generate_synthetic_dim_data", err,
                         "Problem Allocating Heap Memory for Synthetic Dimensional Data");
            return err;
        }

        set_synthetic_dim_data(handle, ndim, synthetic);
    }

    switch (get_dim_type(handle, ndim)) {
        case UDA_TYPE_FLOAT: {
            auto fp = (float*)get_synthetic_dim_data(handle, ndim);
            for (int i = 0; i < get_dim_num(handle, ndim); i++) {
                fp[i] = data[i];
            }
            break;
        }
        case UDA_TYPE_DOUBLE: {
            auto dp = (double*)get_synthetic_dim_data(handle, ndim);
            for (int i = 0; i < get_dim_num(handle, ndim); i++) {
                dp[i] = (double)data[i];
            }
            break;
        }
        case UDA_TYPE_SHORT: {
            auto sp = (short*)get_synthetic_dim_data(handle, ndim);
            for (int i = 0; i < get_dim_num(handle, ndim); i++) {
                sp[i] = (short)data[i];
            }
            break;
        }
        case UDA_TYPE_INT: {
            auto ip = (int*)get_synthetic_dim_data(handle, ndim);
            for (int i = 0; i < get_dim_num(handle, ndim); i++) {
                ip[i] = (int)data[i];
            }
            break;
        }
        case UDA_TYPE_LONG: {
            auto lp = (long*)get_synthetic_dim_data(handle, ndim);
            for (int i = 0; i < get_dim_num(handle, ndim); i++) {
                lp[i] = (long)data[i];
            }
            break;
        }
        case UDA_TYPE_LONG64: {
            auto lp = (long long int*)get_synthetic_dim_data(handle, ndim);
            for (int i = 0; i < get_dim_num(handle, ndim); i++) {
                lp[i] = (long long int)data[i];
            }
            break;
        }
        case UDA_TYPE_CHAR: {
            auto cp = get_synthetic_dim_data(handle, ndim);
            for (int i = 0; i < get_dim_num(handle, ndim); i++) {
                cp[i] = (char)data[i];
            }
            break;
        }
        case UDA_TYPE_UNSIGNED_SHORT: {
            auto sp = (unsigned short*)get_synthetic_dim_data(handle, ndim);
            for (int i = 0; i < get_dim_num(handle, ndim); i++) {
                sp[i] = (unsigned short)data[i];
            }
            break;
        }
        case UDA_TYPE_UNSIGNED_INT: {
            auto up = (unsigned int*)get_synthetic_dim_data(handle, ndim);
            for (int i = 0; i < get_dim_num(handle, ndim); i++) {
                up[i] = (unsigned int)data[i];
            }
            break;
        }
        case UDA_TYPE_UNSIGNED_LONG: {
            auto lp = (unsigned long*)get_synthetic_dim_data(handle, ndim);
            for (int i = 0; i < get_dim_num(handle, ndim); i++) {
                lp[i] = (unsigned long)data[i];
            }
            break;
        }
        case UDA_TYPE_UNSIGNED_LONG64: {
            auto lp = (unsigned long long int*)get_synthetic_dim_data(handle, ndim);
            for (int i = 0; i < get_dim_num(handle, ndim); i++) {
                lp[i] = (unsigned long long int)data[i];
            }
            break;
        }
        case UDA_TYPE_UNSIGNED_CHAR: {
            auto cp = (unsigned char*)get_synthetic_dim_data(handle, ndim);
            for (int i = 0; i < get_dim_num(handle, ndim); i++) {
                cp[i] = (unsigned char)data[i];
            }
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

    if (get_data(handle) == nullptr) {
        return 0; // No Data Block available
    }

    int model;
    int param_n;
    float params[MAXERRPARAMS];

    get_error_model(handle, &model, &param_n, params);

    if (model <= ERROR_MODEL_UNKNOWN || model >= ERROR_MODEL_UNDEFINED) {
        return 0; // No valid Model
    }

    if (get_data_num(handle) <= 0) {
        return 0;
    }

    //--------------------------------------------------------------------------------------------------------------
    // Allocate local float work arrays and copy the data array to the work array

    if (get_data_type(handle) == UDA_TYPE_DCOMPLEX || get_data_type(handle) == UDA_TYPE_COMPLEX) {
        err = 999;
        udaAddError(UDA_CODE_ERROR_TYPE, "generate_data_error", err,
                     "Not configured to Generate Complex Type Synthetic Data");
        return 999;
    }

    float* data;
    char* errhi;
    char* errlo;

    if ((data = (float*)malloc(get_data_num(handle) * sizeof(float))) == nullptr) {
        return 1;
    }
    if ((errhi = (char*)malloc(get_data_num(handle) * sizeof(float))) == nullptr) {
        return 1;
    }
    if ((errlo = (char*)malloc(get_data_num(handle) * sizeof(float))) == nullptr) {
        return 1;
    }

    switch (get_data_type(handle)) {
        case UDA_TYPE_FLOAT: {
            auto fp = (float*)get_data(handle);
            for (int i = 0; i < get_data_num(handle); i++) {
                data[i] = fp[i]; // Cast all data to Float
            }
            break;
        }
        case UDA_TYPE_DOUBLE: {
            auto dp = (double*)get_data(handle);
            for (int i = 0; i < get_data_num(handle); i++) {
                data[i] = (float)dp[i];
            }
            break;
        }
        case UDA_TYPE_INT: {
            auto ip = (int*)get_data(handle);
            for (int i = 0; i < get_data_num(handle); i++) {
                data[i] = (float)ip[i];
            }
            break;
        }
        case UDA_TYPE_UNSIGNED_INT: {
            auto up = (unsigned int*)get_data(handle);
            for (int i = 0; i < get_data_num(handle); i++) {
                data[i] = (float)up[i];
            }
            break;
        }
        case UDA_TYPE_LONG: {
            auto lp = (long*)get_data(handle);
            for (int i = 0; i < get_data_num(handle); i++) {
                data[i] = (float)lp[i];
            }
            break;
        }
        case UDA_TYPE_LONG64: {
            auto lp = (long long int*)get_data(handle);
            for (int i = 0; i < get_data_num(handle); i++) {
                data[i] = (float)lp[i];
            }
            break;
        }
        case UDA_TYPE_SHORT: {
            auto sp = (short*)get_data(handle);
            for (int i = 0; i < get_data_num(handle); i++) {
                data[i] = (float)sp[i];
            }
            break;
        }
        case UDA_TYPE_CHAR: {
            char* cp = (char*)get_data(handle);
            for (int i = 0; i < get_data_num(handle); i++) {
                data[i] = (float)cp[i];
            }
            break;
        }
        case UDA_TYPE_UNSIGNED_LONG: {
            auto lp = (unsigned long*)get_data(handle);
            for (int i = 0; i < get_data_num(handle); i++) {
                data[i] = (float)lp[i];
            }
            break;
        }
        case UDA_TYPE_UNSIGNED_LONG64: {
            auto lp = (unsigned long long int*)get_data(handle);
            for (int i = 0; i < get_data_num(handle); i++) {
                data[i] = (float)lp[i];
            }
            break;
        }
        case UDA_TYPE_UNSIGNED_SHORT: {
            auto sp = (unsigned short*)get_data(handle);
            for (int i = 0; i < get_data_num(handle); i++) {
                data[i] = (float)sp[i];
            }
            break;
        }
        case UDA_TYPE_UNSIGNED_CHAR: {
            auto cp = (unsigned char*)get_data(handle);
            for (int i = 0; i < get_data_num(handle); i++) {
                data[i] = (float)cp[i];
            }
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

    err = error_model(model, param_n, params, get_data_num(handle), data, &asymmetry, (float*)errhi, (float*)errlo);

    if (err != 0) {
        free(data);
        free(errhi);
        free(errlo);
        return err;
    }

    //--------------------------------------------------------------------------------------------------------------
    // Return the Error Array

    set_data_err_type(handle, get_data_type(handle));
    set_data_err_asymmetry(handle, asymmetry);

    if (asymmetry && get_data_err_lo(handle) == nullptr) {
        if ((err = allocArray(get_data_type(handle), get_data_num(handle), &perrlo))) {
            return err;
        }
        set_data_err_lo(handle, perrlo);
    }

    switch (get_data_type(handle)) {
        case UDA_TYPE_FLOAT: {
            auto feh = (float*)get_data_err_hi(handle);
            auto fel = (float*)get_data_err_lo(handle);
            for (int i = 0; i < get_data_num(handle); i++) {
                feh[i] = (float)errhi[i];
                if (get_data_err_asymmetry(handle)) {
                    fel[i] = (float)errlo[i];
                }
            }
            break;
        }
        case UDA_TYPE_DOUBLE: {
            auto deh = (double*)get_data_err_hi(handle);
            auto del = (double*)get_data_err_lo(handle);
            for (int i = 0; i < get_data_num(handle); i++) {
                deh[i] = (double)errhi[i];
                if (get_data_err_asymmetry(handle)) {
                    del[i] = (double)errlo[i];
                }
            }
            break;
        }
        case UDA_TYPE_INT: {
            auto ieh = (int*)get_data_err_hi(handle);
            auto iel = (int*)get_data_err_lo(handle);
            for (int i = 0; i < get_data_num(handle); i++) {
                ieh[i] = (int)errhi[i];
                if (get_data_err_asymmetry(handle)) {
                    iel[i] = (int)errlo[i];
                }
            }
            break;
        }
        case UDA_TYPE_LONG: {
            auto leh = (long*)get_data_err_hi(handle);
            auto lel = (long*)get_data_err_lo(handle);
            for (int i = 0; i < get_data_num(handle); i++) {
                leh[i] = (long)errhi[i];
                if (get_data_err_asymmetry(handle)) {
                    lel[i] = (long)errlo[i];
                }
            }
            break;
        }
        case UDA_TYPE_LONG64: {
            auto leh = (long long int*)get_data_err_hi(handle);
            auto lel = (long long int*)get_data_err_lo(handle);
            for (int i = 0; i < get_data_num(handle); i++) {
                leh[i] = (long long int)errhi[i];
                if (get_data_err_asymmetry(handle)) {
                    lel[i] = (long long int)errlo[i];
                }
            }
            break;
        }
        case UDA_TYPE_SHORT: {
            auto seh = (short*)get_data_err_hi(handle);
            auto sel = (short*)get_data_err_lo(handle);
            for (int i = 0; i < get_data_num(handle); i++) {
                seh[i] = (short)errhi[i];
                if (get_data_err_asymmetry(handle)) {
                    sel[i] = (short)errlo[i];
                }
            }
            break;
        }
        case UDA_TYPE_CHAR: {
            char* ceh = get_data_err_hi(handle);
            char* cel = get_data_err_lo(handle);
            for (int i = 0; i < get_data_num(handle); i++) {
                ceh[i] = errhi[i];
                if (get_data_err_asymmetry(handle)) {
                    cel[i] = errlo[i];
                }
            }
            break;
        }
        case UDA_TYPE_UNSIGNED_INT: {
            auto ueh = (unsigned int*)get_data_err_hi(handle);
            auto uel = (unsigned int*)get_data_err_lo(handle);
            for (int i = 0; i < get_data_num(handle); i++) {
                ueh[i] = (unsigned int)errhi[i];
                if (get_data_err_asymmetry(handle)) {
                    uel[i] = (unsigned int)errlo[i];
                }
            }
            break;
        }
        case UDA_TYPE_UNSIGNED_LONG: {
            auto leh = (unsigned long*)get_data_err_hi(handle);
            auto lel = (unsigned long*)get_data_err_lo(handle);
            for (int i = 0; i < get_data_num(handle); i++) {
                leh[i] = (unsigned long)errhi[i];
                if (get_data_err_asymmetry(handle)) {
                    lel[i] = (unsigned long)errlo[i];
                }
            }
            break;
        }
        case UDA_TYPE_UNSIGNED_LONG64: {
            auto leh = (unsigned long long int*)get_data_err_hi(handle);
            auto lel = (unsigned long long int*)get_data_err_lo(handle);
            for (int i = 0; i < get_data_num(handle); i++) {
                leh[i] = (unsigned long long int)errhi[i];
                if (get_data_err_asymmetry(handle)) {
                    lel[i] = (unsigned long long int)errlo[i];
                }
            }
            break;
        }
        case UDA_TYPE_UNSIGNED_SHORT: {
            auto seh = (unsigned short*)get_data_err_hi(handle);
            auto sel = (unsigned short*)get_data_err_lo(handle);
            for (int i = 0; i < get_data_num(handle); i++) {
                seh[i] = (unsigned short)errhi[i];
                if (get_data_err_asymmetry(handle)) {
                    sel[i] = (unsigned short)errlo[i];
                }
            }
            break;
        }
        case UDA_TYPE_UNSIGNED_CHAR: {
            auto ceh = (unsigned char*)get_data_err_hi(handle);
            auto cel = (unsigned char*)get_data_err_lo(handle);
            for (int i = 0; i < get_data_num(handle); i++) {
                ceh[i] = (unsigned char)errhi[i];
                if (get_data_err_asymmetry(handle)) {
                    cel[i] = (unsigned char)errlo[i];
                }
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

    if (get_data(handle) == nullptr) {
        return 0; // No Data Block available
    }

    if (ndim < 0 || ndim > get_rank(handle)) {
        return 0;
    }

    int model;
    int param_n;
    float params[MAXERRPARAMS];

    get_error_model(handle, &model, &param_n, params);

    if (model <= ERROR_MODEL_UNKNOWN || model >= ERROR_MODEL_UNDEFINED) {
        return 0; // No valid Model
    }

    if (get_dim_num(handle, ndim) <= 0) {
        return 0;
    }

    //--------------------------------------------------------------------------------------------------------------
    // Allocate local float work arrays and copy the data array to the work array

    if (get_data_type(handle) == UDA_TYPE_DCOMPLEX || get_data_type(handle) == UDA_TYPE_COMPLEX) {
        err = 999;
        udaAddError(UDA_CODE_ERROR_TYPE, "generate_dim_data_error", err,
                     "Not configured to Generate Complex Type Synthetic Data");
        return 999;
    }

    float* data;
    char* errhi;
    char* errlo;

    if ((data = (float*)malloc(get_dim_num(handle, ndim) * sizeof(float))) == nullptr) {
        return 1;
    }
    if ((errhi = (char*)malloc(get_dim_num(handle, ndim) * sizeof(float))) == nullptr) {
        return 1;
    }
    if ((errlo = (char*)malloc(get_dim_num(handle, ndim) * sizeof(float))) == nullptr) {
        return 1;
    }

    switch (get_data_type(handle)) {
        case UDA_TYPE_FLOAT: {
            auto fp = (float*)get_dim_data(handle, ndim);
            for (int i = 0; i < get_dim_num(handle, ndim); i++) {
                data[i] = fp[i]; // Cast all data to Float
            }
            break;
        }
        case UDA_TYPE_DOUBLE: {
            auto dp = (double*)get_dim_data(handle, ndim);
            for (int i = 0; i < get_dim_num(handle, ndim); i++) {
                data[i] = (float)dp[i];
            }
            break;
        }
        case UDA_TYPE_INT: {
            auto ip = (int*)get_dim_data(handle, ndim);
            for (int i = 0; i < get_dim_num(handle, ndim); i++) {
                data[i] = (float)ip[i];
            }
            break;
        }
        case UDA_TYPE_LONG: {
            auto lp = (long*)get_dim_data(handle, ndim);
            for (int i = 0; i < get_dim_num(handle, ndim); i++) {
                data[i] = (float)lp[i];
            }
            break;
        }
        case UDA_TYPE_LONG64: {
            auto lp = (long long int*)get_dim_data(handle, ndim);
            for (int i = 0; i < get_dim_num(handle, ndim); i++) {
                data[i] = (float)lp[i];
            }
            break;
        }
        case UDA_TYPE_SHORT: {
            auto sp = (short*)get_dim_data(handle, ndim);
            for (int i = 0; i < get_dim_num(handle, ndim); i++) {
                data[i] = (float)sp[i];
            }
            break;
        }
        case UDA_TYPE_CHAR: {
            char* cp = get_dim_data(handle, ndim);
            for (int i = 0; i < get_dim_num(handle, ndim); i++) {
                data[i] = (float)cp[i];
            }
            break;
        }
        case UDA_TYPE_UNSIGNED_INT: {
            auto up = (unsigned int*)get_dim_data(handle, ndim);
            for (int i = 0; i < get_dim_num(handle, ndim); i++) {
                data[i] = (float)up[i];
            }
            break;
        }
        case UDA_TYPE_UNSIGNED_LONG: {
            auto lp = (unsigned long*)get_dim_data(handle, ndim);
            for (int i = 0; i < get_dim_num(handle, ndim); i++) {
                data[i] = (float)lp[i];
            }
            break;
        }
        case UDA_TYPE_UNSIGNED_LONG64: {
            auto lp = (unsigned long long int*)get_dim_data(handle, ndim);
            for (int i = 0; i < get_dim_num(handle, ndim); i++) {
                data[i] = (float)lp[i];
            }
            break;
        }
        case UDA_TYPE_UNSIGNED_SHORT: {
            auto sp = (unsigned short*)get_dim_data(handle, ndim);
            for (int i = 0; i < get_dim_num(handle, ndim); i++) {
                data[i] = (float)sp[i];
            }
            break;
        }
        case UDA_TYPE_UNSIGNED_CHAR: {
            auto cp = (unsigned char*)get_dim_data(handle, ndim);
            for (int i = 0; i < get_dim_num(handle, ndim); i++) {
                data[i] = (float)cp[i];
            }
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

    err =
        error_model(model, param_n, params, get_dim_num(handle, ndim), data, &asymmetry, (float*)errhi, (float*)errlo);

    if (err != 0) {
        free(data);
        free(errhi);
        free(errlo);
        return err;
    }

    //--------------------------------------------------------------------------------------------------------------
    // Return the Synthetic Data and Error Array

    set_dim_err_type(handle, ndim, get_dim_type(handle, ndim));
    set_dim_err_asymmetry(handle, ndim, asymmetry);

    if (get_dim_err_asymmetry(handle, ndim) && get_dim_err_lo(handle, ndim) == nullptr) {
        if ((err = allocArray(get_dim_type(handle, ndim), get_dim_num(handle, ndim), &perrlo))) {
            return err;
        }
        set_dim_err_lo(handle, ndim, perrlo);
    }

    switch (get_dim_type(handle, ndim)) {
        case UDA_TYPE_FLOAT: {
            auto feh = (float*)get_dim_err_hi(handle, ndim);
            auto fel = (float*)get_dim_err_lo(handle, ndim);
            for (int i = 0; i < get_dim_num(handle, ndim); i++) {
                feh[i] = (float)errhi[i];
                if (get_dim_err_asymmetry(handle, ndim)) {
                    fel[i] = (float)errlo[i];
                }
            }
            break;
        }
        case UDA_TYPE_DOUBLE: {
            auto deh = (double*)get_dim_err_hi(handle, ndim);
            auto del = (double*)get_dim_err_lo(handle, ndim);
            for (int i = 0; i < get_dim_num(handle, ndim); i++) {
                deh[i] = (double)errhi[i];
                if (get_dim_err_asymmetry(handle, ndim)) {
                    del[i] = (double)errlo[i];
                }
            }
            break;
        }
        case UDA_TYPE_INT: {
            auto ieh = (int*)get_dim_err_hi(handle, ndim);
            auto iel = (int*)get_dim_err_lo(handle, ndim);
            for (int i = 0; i < get_dim_num(handle, ndim); i++) {
                ieh[i] = (int)errhi[i];
                if (get_dim_err_asymmetry(handle, ndim)) {
                    iel[i] = (int)errlo[i];
                }
            }
            break;
        }
        case UDA_TYPE_UNSIGNED_INT: {
            auto ueh = (unsigned int*)get_dim_err_hi(handle, ndim);
            auto uel = (unsigned int*)get_dim_err_lo(handle, ndim);
            for (int i = 0; i < get_dim_num(handle, ndim); i++) {
                ueh[i] = (unsigned int)errhi[i];
                if (get_dim_err_asymmetry(handle, ndim)) {
                    uel[i] = (unsigned int)errlo[i];
                }
            }
            break;
        }
        case UDA_TYPE_LONG: {
            auto leh = (long*)get_dim_err_hi(handle, ndim);
            auto lel = (long*)get_dim_err_lo(handle, ndim);
            for (int i = 0; i < get_dim_num(handle, ndim); i++) {
                leh[i] = (long)errhi[i];
                if (get_dim_err_asymmetry(handle, ndim)) {
                    lel[i] = (long)errlo[i];
                }
            }
            break;
        }
        case UDA_TYPE_LONG64: {
            auto leh = (long long int*)get_dim_err_hi(handle, ndim);
            auto lel = (long long int*)get_dim_err_lo(handle, ndim);
            for (int i = 0; i < get_dim_num(handle, ndim); i++) {
                leh[i] = (long long int)errhi[i];
                if (get_dim_err_asymmetry(handle, ndim)) {
                    lel[i] = (long long int)errlo[i];
                }
            }
            break;
        }
        case UDA_TYPE_SHORT: {
            auto seh = (short*)get_dim_err_hi(handle, ndim);
            auto sel = (short*)get_dim_err_lo(handle, ndim);
            for (int i = 0; i < get_dim_num(handle, ndim); i++) {
                seh[i] = (short)errhi[i];
                if (get_dim_err_asymmetry(handle, ndim)) {
                    sel[i] = (short)errlo[i];
                }
            }
            break;
        }
        case UDA_TYPE_CHAR: {
            char* ceh = (char*)get_dim_err_hi(handle, ndim);
            char* cel = (char*)get_dim_err_lo(handle, ndim);
            for (int i = 0; i < get_dim_num(handle, ndim); i++) {
                ceh[i] = (char)errhi[i];
                if (get_dim_err_asymmetry(handle, ndim)) {
                    cel[i] = errlo[i];
                }
            }
            break;
        }
        case UDA_TYPE_UNSIGNED_LONG: {
            auto leh = (unsigned long*)get_dim_err_hi(handle, ndim);
            auto lel = (unsigned long*)get_dim_err_lo(handle, ndim);
            for (int i = 0; i < get_dim_num(handle, ndim); i++) {
                leh[i] = (unsigned long)errhi[i];
                if (get_dim_err_asymmetry(handle, ndim)) {
                    lel[i] = (unsigned long)errlo[i];
                }
            }
            break;
        }
        case UDA_TYPE_UNSIGNED_LONG64: {
            auto leh = (unsigned long long int*)get_dim_err_hi(handle, ndim);
            auto lel = (unsigned long long int*)get_dim_err_lo(handle, ndim);
            for (int i = 0; i < get_dim_num(handle, ndim); i++) {
                leh[i] = (unsigned long long int)errhi[i];
                if (get_dim_err_asymmetry(handle, ndim)) {
                    lel[i] = (unsigned long long int)errlo[i];
                }
            }
            break;
        }
        case UDA_TYPE_UNSIGNED_SHORT: {
            auto seh = (unsigned short*)get_dim_err_hi(handle, ndim);
            auto sel = (unsigned short*)get_dim_err_lo(handle, ndim);
            for (int i = 0; i < get_dim_num(handle, ndim); i++) {
                seh[i] = (unsigned short)errhi[i];
                if (get_dim_err_asymmetry(handle, ndim)) {
                    sel[i] = (unsigned short)errlo[i];
                }
            }
            break;
        }
        case UDA_TYPE_UNSIGNED_CHAR: {
            auto ceh = (unsigned char*)get_dim_err_hi(handle, ndim);
            auto cel = (unsigned char*)get_dim_err_lo(handle, ndim);
            for (int i = 0; i < get_dim_num(handle, ndim); i++) {
                ceh[i] = (unsigned char)errhi[i];
                if (get_dim_err_asymmetry(handle, ndim)) {
                    cel[i] = (unsigned char)errlo[i];
                }
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
