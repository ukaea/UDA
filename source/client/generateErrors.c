/*---------------------------------------------------------------
* IDAM Model Based Symmetric/Asymmetric Error Data Generation
*
* Input Arguments:
* Returns:
*
*
*
* Notes:
*
*--------------------------------------------------------------*/

#include "generateErrors.h"

#include <math.h>
#include <stdlib.h>

#include <clientserver/udaTypes.h>
#include <clientserver/errorLog.h>
#include <clientserver/allocData.h>

#include "accAPI.h"

#ifndef NO_GSL_LIB
#  include <gsl/gsl_randist.h>
#endif

//--------------------------------------------------------------------------------------------------------------
// Generate Error Data

int idamErrorModel(int model, int param_n, float* params, int data_n, float* data, int* asymmetry, float* errhi,
                   float* errlo)
{

    int i;

    *asymmetry = 0;        // No Error Asymmetry for most models

    switch (model) {

        case ERROR_MODEL_DEFAULT:
            if (param_n != 2) return 1;
            for (i = 0; i < data_n; i++) {
                errhi[i] = params[0] + params[1] * fabsf(data[i]);
                errlo[i] = errhi[i];
            }
            break;

        case ERROR_MODEL_DEFAULT_ASYMMETRIC:
            if (param_n != 4) return 1;
            *asymmetry = 1;
            for (i = 0; i < data_n; i++) {
                errhi[i] = params[0] + params[1] * fabsf(data[i]);
                errlo[i] = params[2] + params[3] * fabsf(data[i]);
            }
            break;

        default:
            for (i = 0; i < data_n; i++) {
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
//	GSL_RNG_SEED	12345		for the seed value
//	GSL_RNG_TYPE	mrg		for the name of the random number generator

int idamSyntheticModel(int model, int param_n, float* params, int data_n, float* data)
{

#ifdef NO_GSL_LIB
    int err = 999;
    addIdamError(CODEERRORTYPE, "idamSyntheticModel", err,
                 "Random Number Generators from the GSL library required.");
    return 999;
#else
    int i;
    float shift;
    static gsl_rng *random=NULL;

    if(random == NULL) {		// Initialise the Random Number System
        gsl_rng_env_setup();
        random = gsl_rng_alloc (gsl_rng_default);
        gsl_rng_set(random, (unsigned long int) ERROR_MODEL_SEED);	// Seed the Random Number generator with the library default
    }

    switch (model) {

    case ERROR_MODEL_GAUSSIAN:  	// Standard normal Distribution
        if(param_n < 1 || param_n > 2) return 1;
        if(param_n == 2) gsl_rng_set(random, (unsigned long int) params[1]);		// Change the Seed before Sampling
        for(i=0; i<data_n; i++) data[i] = data[i] + (float) gsl_ran_gaussian (random, (double) params[0]); // Random sample from PDF
        break;

    case ERROR_MODEL_RESEED:  							// Change the Seed
        if(param_n == 1) gsl_rng_set(random, (unsigned long int) params[0]);
        break;

    case ERROR_MODEL_GAUSSIAN_SHIFT:
        if(param_n < 1 || param_n > 2) return 1;
        if(param_n == 2) gsl_rng_set(random, (unsigned long int) params[1]);		// Change the Seed before Sampling
        shift = (float) gsl_ran_gaussian (random, (double) params[0]);
        for(i=0; i<data_n; i++) data[i] = data[i] + shift;				// Random linear shift of data array
        break;

    case ERROR_MODEL_POISSON:
        if(param_n < 0 || param_n > 1) return 1;
        if(param_n == 1) gsl_rng_set(random, (unsigned long int) params[0]);				// Change the Seed before Sampling
        for(i=0; i<data_n; i++) data[i] = data[i] + (float) gsl_ran_poisson(random, (double)data[i]);	// Randomly perturb data array
        break;
    }

    return 0;
#endif
}


int generateIdamSyntheticData(int handle)
{

    int i, err = 0;

    float* fp, * data;
    double* dp;
    short* sp;
    int* ip;
    long* lp;
    unsigned* up;

    char* synthetic = NULL;

    //--------------------------------------------------------------------------------------------------------------
    // Check the handle and model are ok

    if (getIdamData(handle) == NULL) return 0;

    int model;
    int param_n;
    float params[MAXERRPARAMS];

    getIdamErrorModel(handle, &model, &param_n, params);

    if (model <= ERROR_MODEL_UNKNOWN || model >= ERROR_MODEL_UNDEFINED) return 0;    // No valid Model

    if (getIdamDataNum(handle) <= 0) return 0;

    //--------------------------------------------------------------------------------------------------------------
    // Allocate local float work arrays and copy the data array to the work array

    if (getIdamDataType(handle) == UDA_TYPE_DCOMPLEX || getIdamDataType(handle) == UDA_TYPE_COMPLEX) {
        err = 999;
        addIdamError(CODEERRORTYPE, "generateIdamSyntheticData", err,
                     "Not configured to Generate Complex Type Synthetic Data");
        return 999;
    }

    if ((data = (float*)malloc(getIdamDataNum(handle) * sizeof(float))) == NULL) return 1;

    switch (getIdamDataType(handle)) {
        case UDA_TYPE_FLOAT:
            fp = (float*)getIdamData(handle);
            for (i = 0; i < getIdamDataNum(handle); i++) data[i] = fp[i];        // Cast all data to Float
            break;
        case UDA_TYPE_DOUBLE:
            dp = (double*)getIdamData(handle);
            for (i = 0; i < getIdamDataNum(handle); i++) data[i] = (float)dp[i];
            break;
        case UDA_TYPE_SHORT:
            sp = (short*)getIdamData(handle);
            for (i = 0; i < getIdamDataNum(handle); i++) data[i] = (float)sp[i];
            break;
        case UDA_TYPE_INT:
            ip = (int*)getIdamData(handle);
            for (i = 0; i < getIdamDataNum(handle); i++) data[i] = (float)ip[i];
            break;
        case UDA_TYPE_LONG:
            lp = (long*)getIdamData(handle);
            for (i = 0; i < getIdamDataNum(handle); i++) data[i] = (float)lp[i];
            break;
        case UDA_TYPE_LONG64: {
            long long int* lp = (long long int*)getIdamData(handle);
            for (i = 0; i < getIdamDataNum(handle); i++) data[i] = (float)lp[i];
            break;
        }
        case UDA_TYPE_CHAR: {
            char* cp = (char*)getIdamData(handle);
            for (i = 0; i < getIdamDataNum(handle); i++) data[i] = (float)cp[i];
            break;
        }
        case UDA_TYPE_UNSIGNED_SHORT: {
            unsigned short* sp = (unsigned short*)getIdamData(handle);
            for (i = 0; i < getIdamDataNum(handle); i++) data[i] = (float)sp[i];
            break;
        }
        case UDA_TYPE_UNSIGNED_INT:
            up = (unsigned int*)getIdamData(handle);
            for (i = 0; i < getIdamDataNum(handle); i++) data[i] = (float)up[i];
            break;
        case UDA_TYPE_UNSIGNED_LONG: {
            unsigned long* lp = (unsigned long*)getIdamData(handle);
            for (i = 0; i < getIdamDataNum(handle); i++) data[i] = (float)lp[i];
            break;
        }
#ifndef __APPLE__
        case UDA_TYPE_UNSIGNED_LONG64: {
            unsigned long long int * lp = (unsigned long long int *) getIdamData(handle);
            for (i = 0; i < getIdamDataNum(handle); i++) data[i] = (float) lp[i];
            break;
        }
#endif
        case UDA_TYPE_UNSIGNED_CHAR: {
            unsigned char* cp = (unsigned char*)getIdamData(handle);
            for (i = 0; i < getIdamDataNum(handle); i++) data[i] = (float)cp[i];
            break;
        }

        default:
            free((void*)data);
            return 0;
    }

//--------------------------------------------------------------------------------------------------------------
// Generate Synthetic Data

    err = idamSyntheticModel(model, param_n, params, getIdamDataNum(handle), data);

    if (err != 0) {
        addIdamError(CODEERRORTYPE, "generateIdamSyntheticData", err,
                     "Unable to Generate Synthetic Data");
        free((void*)data);
        return err;
    }

//--------------------------------------------------------------------------------------------------------------
// Return the Synthetic Data

    if (acc_getSyntheticData(handle) == NULL) {
        if ((err = allocArray(getIdamDataType(handle), getIdamDataNum(handle), &synthetic))) {
            addIdamError(CODEERRORTYPE, "generateIdamSyntheticData", err,
                         "Problem Allocating Heap Memory for Synthetic Data");
            return err;
        }
        acc_setSyntheticData(handle, synthetic);
    }

    switch (getIdamDataType(handle)) {
        case UDA_TYPE_FLOAT:
            fp = (float*)acc_getSyntheticData(handle);
            for (i = 0; i < getIdamDataNum(handle); i++) fp[i] = data[i];        // Overwrite the Data
            break;
        case UDA_TYPE_DOUBLE:
            dp = (double*)acc_getSyntheticData(handle);
            for (i = 0; i < getIdamDataNum(handle); i++) dp[i] = (double)data[i];
            break;
        case UDA_TYPE_SHORT:
            sp = (short*)acc_getSyntheticData(handle);
            for (i = 0; i < getIdamDataNum(handle); i++) sp[i] = (short)data[i];
            break;
        case UDA_TYPE_INT:
            ip = (int*)acc_getSyntheticData(handle);
            for (i = 0; i < getIdamDataNum(handle); i++) ip[i] = (int)data[i];
            break;
        case UDA_TYPE_LONG:
            lp = (long*)acc_getSyntheticData(handle);
            for (i = 0; i < getIdamDataNum(handle); i++) lp[i] = (long)data[i];
            break;
        case UDA_TYPE_LONG64: {
            long long int* lp = (long long int*)acc_getSyntheticData(handle);
            for (i = 0; i < getIdamDataNum(handle); i++) lp[i] = (long long int)data[i];
            break;
        }
        case UDA_TYPE_UNSIGNED_SHORT: {
            unsigned short* sp;
            sp = (unsigned short*)acc_getSyntheticData(handle);
            for (i = 0; i < getIdamDataNum(handle); i++) sp[i] = (unsigned short)data[i];
            break;
        }
        case UDA_TYPE_UNSIGNED_INT:
            up = (unsigned int*)acc_getSyntheticData(handle);
            for (i = 0; i < getIdamDataNum(handle); i++) up[i] = (unsigned int)data[i];
            break;
        case UDA_TYPE_UNSIGNED_LONG: {
            unsigned long* lp;
            lp = (unsigned long*)acc_getSyntheticData(handle);
            for (i = 0; i < getIdamDataNum(handle); i++) lp[i] = (unsigned long)data[i];
            break;
        }
#ifndef __APPLE__
        case UDA_TYPE_UNSIGNED_LONG64: {
            unsigned long long int * lp;
            lp = (unsigned long long int *) acc_getSyntheticData(handle);
            for (i = 0; i < getIdamDataNum(handle); i++) lp[i] = (unsigned long long int) data[i];
            break;
        }
#endif
        case UDA_TYPE_CHAR: {
            char* cp;
            cp = (char*)acc_getSyntheticData(handle);
            for (i = 0; i < getIdamDataNum(handle); i++) cp[i] = (char)data[i];
            break;
        }
        case UDA_TYPE_UNSIGNED_CHAR: {
            unsigned char* cp;
            cp = (unsigned char*)acc_getSyntheticData(handle);
            for (i = 0; i < getIdamDataNum(handle); i++) cp[i] = (unsigned char)data[i];
            break;
        }

    }

//--------------------------------------------------------------------------------------------------------------
// Housekeeping

    free((void*)data);

    return 0;
}

int generateIdamSyntheticDimData(int handle, int ndim)
{

    int i, err = 0;

    float* fp, * data;
    double* dp;
    short* sp;
    int* ip;
    long* lp;
    unsigned* up;

    char* synthetic = NULL;

//--------------------------------------------------------------------------------------------------------------
// Check the handle and model are ok

    if (getIdamData(handle) == NULL) return 0;            // No Data Block available

    if (ndim < 0 || ndim > getIdamRank(handle)) return 0;

    int model;
    int param_n;
    float params[MAXERRPARAMS];

    getIdamErrorModel(handle, &model, &param_n, params);

    if (model <= ERROR_MODEL_UNKNOWN || model >= ERROR_MODEL_UNDEFINED) return 0; // No valid Model

    if (getIdamDimNum(handle, ndim) <= 0) return 0;

//--------------------------------------------------------------------------------------------------------------
// Allocate local float work arrays and copy the data array to the work array

    if (getIdamDataType(handle) == UDA_TYPE_DCOMPLEX || getIdamDataType(handle) == UDA_TYPE_COMPLEX) {
        err = 999;
        addIdamError(CODEERRORTYPE, "generateIdamSyntheticDimData", err,
                     "Not configured to Generate Complex Type Synthetic Data");
        return 999;
    }

    if ((data = (float*)malloc(getIdamDimNum(handle, ndim) * sizeof(float))) == NULL) {
        addIdamError(CODEERRORTYPE, "generateIdamSyntheticDimData", 1,
                     "Problem Allocating Heap Memory for Synthetic Dimensional Data");
        return 1;
    }

    switch (getIdamDataType(handle)) {
        case UDA_TYPE_FLOAT:
            fp = (float*)getIdamDimData(handle, ndim);
            for (i = 0; i < getIdamDimNum(handle, ndim); i++) data[i] = fp[i];        // Cast all data to Float
            break;
        case UDA_TYPE_DOUBLE:
            dp = (double*)getIdamDimData(handle, ndim);
            for (i = 0; i < getIdamDimNum(handle, ndim); i++) data[i] = (float)dp[i];
            break;
        case UDA_TYPE_INT:
            ip = (int*)getIdamDimData(handle, ndim);
            for (i = 0; i < getIdamDimNum(handle, ndim); i++) data[i] = (float)ip[i];
            break;
        case UDA_TYPE_SHORT:
            sp = (short*)getIdamDimData(handle, ndim);
            for (i = 0; i < getIdamDimNum(handle, ndim); i++) data[i] = (float)sp[i];
            break;
        case UDA_TYPE_LONG:
            lp = (long*)getIdamDimData(handle, ndim);
            for (i = 0; i < getIdamDimNum(handle, ndim); i++) data[i] = (float)lp[i];
            break;
        case UDA_TYPE_LONG64: {
            long long int* lp = (long long int*)getIdamDimData(handle, ndim);
            for (i = 0; i < getIdamDimNum(handle, ndim); i++) data[i] = (float)lp[i];
            break;
        }
        case UDA_TYPE_CHAR: {
            char* cp = (char*)getIdamDimData(handle, ndim);
            for (i = 0; i < getIdamDimNum(handle, ndim); i++) data[i] = (float)cp[i];
            break;
        }
        case UDA_TYPE_UNSIGNED_SHORT: {
            unsigned short* sp = (unsigned short*)getIdamDimData(handle, ndim);
            for (i = 0; i < getIdamDimNum(handle, ndim); i++) data[i] = (float)sp[i];
            break;
        }
        case UDA_TYPE_UNSIGNED_INT:
            up = (unsigned int*)getIdamDimData(handle, ndim);
            for (i = 0; i < getIdamDimNum(handle, ndim); i++) data[i] = (float)up[i];
            break;
        case UDA_TYPE_UNSIGNED_LONG: {
            unsigned long* lp = (unsigned long*)getIdamDimData(handle, ndim);
            for (i = 0; i < getIdamDimNum(handle, ndim); i++) data[i] = (float)lp[i];
            break;
        }
#ifndef __APPLE__
        case UDA_TYPE_UNSIGNED_LONG64: {
            unsigned long long int * lp = (unsigned long long int *) getIdamDimData(handle, ndim);
            for (i = 0; i < getIdamDimNum(handle, ndim); i++) data[i] = (float) lp[i];
            break;
        }
#endif
        case UDA_TYPE_UNSIGNED_CHAR: {
            unsigned char* cp = (unsigned char*)getIdamDimData(handle, ndim);
            for (i = 0; i < getIdamDimNum(handle, ndim); i++) data[i] = (float)cp[i];
            break;
        }
        default:
            free((void*)data);
            return 0;
    }

//--------------------------------------------------------------------------------------------------------------
// Generate Model Data

    err = idamSyntheticModel(model, param_n, params, getIdamDimNum(handle, ndim), data);

    if (err != 0) {
        addIdamError(CODEERRORTYPE, "generateIdamSyntheticDimData", err,
                     "Unable to Generate Synthetic Dimensional Data");
        free((void*)data);
        return err;
    }

//--------------------------------------------------------------------------------------------------------------
// Return the Synthetic Data

    if (acc_getSyntheticDimData(handle, ndim) == NULL) {
        if ((err = allocArray(getIdamDimType(handle, ndim), getIdamDimNum(handle, ndim), &synthetic))) {
            addIdamError(CODEERRORTYPE, "generateIdamSyntheticDimData", err,
                         "Problem Allocating Heap Memory for Synthetic Dimensional Data");
            return err;
        }

        acc_setSyntheticDimData(handle, ndim, synthetic);
    }

    switch (getIdamDimType(handle, ndim)) {
        case UDA_TYPE_FLOAT:
            fp = (float*)acc_getSyntheticDimData(handle, ndim);
            for (i = 0; i < getIdamDimNum(handle, ndim); i++) fp[i] = data[i];
            break;
        case UDA_TYPE_DOUBLE:
            dp = (double*)acc_getSyntheticDimData(handle, ndim);
            for (i = 0; i < getIdamDimNum(handle, ndim); i++) dp[i] = (double)data[i];
            break;
        case UDA_TYPE_SHORT:
            sp = (short*)acc_getSyntheticDimData(handle, ndim);
            for (i = 0; i < getIdamDimNum(handle, ndim); i++) sp[i] = (short)data[i];
            break;
        case UDA_TYPE_INT:
            ip = (int*)acc_getSyntheticDimData(handle, ndim);
            for (i = 0; i < getIdamDimNum(handle, ndim); i++) ip[i] = (int)data[i];
            break;
        case UDA_TYPE_LONG:
            lp = (long*)acc_getSyntheticDimData(handle, ndim);
            for (i = 0; i < getIdamDimNum(handle, ndim); i++) lp[i] = (long)data[i];
            break;
        case UDA_TYPE_LONG64: {
            long long int* lp = (long long int*)acc_getSyntheticDimData(handle, ndim);
            for (i = 0; i < getIdamDimNum(handle, ndim); i++) lp[i] = (long long int)data[i];
            break;
        }
        case UDA_TYPE_CHAR: {
            char* cp = acc_getSyntheticDimData(handle, ndim);
            for (i = 0; i < getIdamDimNum(handle, ndim); i++) cp[i] = (char)data[i];
            break;
        }
        case UDA_TYPE_UNSIGNED_SHORT: {
            unsigned short* sp = (unsigned short*)acc_getSyntheticDimData(handle, ndim);
            for (i = 0; i < getIdamDimNum(handle, ndim); i++) sp[i] = (unsigned short)data[i];
            break;
        }
        case UDA_TYPE_UNSIGNED_INT:
            up = (unsigned int*)acc_getSyntheticDimData(handle, ndim);
            for (i = 0; i < getIdamDimNum(handle, ndim); i++) up[i] = (unsigned int)data[i];
            break;
        case UDA_TYPE_UNSIGNED_LONG: {
            unsigned long* lp = (unsigned long*)acc_getSyntheticDimData(handle, ndim);
            for (i = 0; i < getIdamDimNum(handle, ndim); i++) lp[i] = (unsigned long)data[i];
            break;
        }
#ifndef __APPLE__
        case UDA_TYPE_UNSIGNED_LONG64: {
            unsigned long long int * lp = (unsigned long long int *) acc_getSyntheticDimData(handle, ndim);
            for (i = 0; i < getIdamDimNum(handle, ndim); i++) lp[i] = (unsigned long long int) data[i];
            break;
        }
#endif
        case UDA_TYPE_UNSIGNED_CHAR: {
            unsigned char* cp = (unsigned char*)acc_getSyntheticDimData(handle, ndim);
            for (i = 0; i < getIdamDimNum(handle, ndim); i++) cp[i] = (unsigned char)data[i];
            break;
        }
    }

//--------------------------------------------------------------------------------------------------------------
// Housekeeping

    free((void*)data);

    return 0;
}

int generateIdamDataError(int handle)
{

    int i, err = 0, asymmetry = 0;

    float* fp, * feh, * fel, * data;
    char* errhi, * errlo;
    double* dp, * deh, * del;
    short* sp, * seh, * sel;
    int* ip, * ieh, * iel;
    long* lp, * leh, * lel;
    unsigned* up, * ueh, * uel;

    char* perrlo = NULL;

//--------------------------------------------------------------------------------------------------------------
// Check the handle and model are ok

    if (getIdamData(handle) == NULL) return 0;            // No Data Block available

    int model;
    int param_n;
    float params[MAXERRPARAMS];

    getIdamErrorModel(handle, &model, &param_n, params);

    if (model <= ERROR_MODEL_UNKNOWN || model >= ERROR_MODEL_UNDEFINED) return 0;    // No valid Model

    if (getIdamDataNum(handle) <= 0) return 0;

//--------------------------------------------------------------------------------------------------------------
// Allocate local float work arrays and copy the data array to the work array

    if (getIdamDataType(handle) == UDA_TYPE_DCOMPLEX || getIdamDataType(handle) == UDA_TYPE_COMPLEX) {
        err = 999;
        addIdamError(CODEERRORTYPE, "generateIdamDataError", err,
                     "Not configured to Generate Complex Type Synthetic Data");
        return 999;
    }

    if ((data = (float*)malloc(getIdamDataNum(handle) * sizeof(float))) == NULL) return 1;
    if ((errhi = (char*)malloc(getIdamDataNum(handle) * sizeof(float))) == NULL) return 1;
    if ((errlo = (char*)malloc(getIdamDataNum(handle) * sizeof(float))) == NULL) return 1;

    switch (getIdamDataType(handle)) {
        case UDA_TYPE_FLOAT:
            fp = (float*)getIdamData(handle);
            for (i = 0; i < getIdamDataNum(handle); i++) data[i] = fp[i];        // Cast all data to Float
            break;
        case UDA_TYPE_DOUBLE:
            dp = (double*)getIdamData(handle);
            for (i = 0; i < getIdamDataNum(handle); i++) data[i] = (float)dp[i];
            break;
        case UDA_TYPE_INT:
            ip = (int*)getIdamData(handle);
            for (i = 0; i < getIdamDataNum(handle); i++) data[i] = (float)ip[i];
            break;
        case UDA_TYPE_UNSIGNED_INT:
            up = (unsigned int*)getIdamData(handle);
            for (i = 0; i < getIdamDataNum(handle); i++) data[i] = (float)up[i];
            break;
        case UDA_TYPE_LONG:
            lp = (long*)getIdamData(handle);
            for (i = 0; i < getIdamDataNum(handle); i++) data[i] = (float)lp[i];
            break;
        case UDA_TYPE_LONG64: {
            long long int* lp = (long long int*)getIdamData(handle);
            for (i = 0; i < getIdamDataNum(handle); i++) data[i] = (float)lp[i];
            break;
        }
        case UDA_TYPE_SHORT:
            sp = (short*)getIdamData(handle);
            for (i = 0; i < getIdamDataNum(handle); i++) data[i] = (float)sp[i];
            break;
        case UDA_TYPE_CHAR: {
            char* cp = (char*)getIdamData(handle);
            for (i = 0; i < getIdamDataNum(handle); i++) data[i] = (float)cp[i];
            break;
        }
        case UDA_TYPE_UNSIGNED_LONG: {
            unsigned long* lp = (unsigned long*)getIdamData(handle);
            for (i = 0; i < getIdamDataNum(handle); i++) data[i] = (float)lp[i];
            break;
        }
#ifndef __APPLE__
        case UDA_TYPE_UNSIGNED_LONG64: {
            unsigned long long int * lp = (unsigned long long int *) getIdamData(handle);
            for (i = 0; i < getIdamDataNum(handle); i++) data[i] = (float) lp[i];
            break;
        }
#endif
        case UDA_TYPE_UNSIGNED_SHORT: {
            unsigned short* sp = (unsigned short*)getIdamData(handle);
            for (i = 0; i < getIdamDataNum(handle); i++) data[i] = (float)sp[i];
            break;
        }
        case UDA_TYPE_UNSIGNED_CHAR: {
            unsigned char* cp = (unsigned char*)getIdamData(handle);
            for (i = 0; i < getIdamDataNum(handle); i++) data[i] = (float)cp[i];
            break;
        }
        default:
            free((void*)data);
            free((void*)errhi);
            free((void*)errlo);
            return 0;
    }

//--------------------------------------------------------------------------------------------------------------
// Generate Error Data

    err = idamErrorModel(model, param_n, params, getIdamDataNum(handle), data, &asymmetry, (float*)errhi,
                         (float*)errlo);

    if (err != 0) {
        free((void*)data);
        free((void*)errhi);
        free((void*)errlo);
        return err;
    }

//--------------------------------------------------------------------------------------------------------------
// Return the Error Array

    acc_setIdamDataErrType(handle, getIdamDataType(handle));
    acc_setIdamDataErrAsymmetry(handle, asymmetry);

    if (asymmetry && getIdamDataErrLo(handle) == NULL) {
        if ((err = allocArray(getIdamDataType(handle), getIdamDataNum(handle), &perrlo))) return err;
        acc_setIdamDataErrLo(handle, perrlo);
    }

    switch (getIdamDataType(handle)) {
        case UDA_TYPE_FLOAT:
            feh = (float*)getIdamDataErrHi(handle);
            fel = (float*)getIdamDataErrLo(handle);
            for (i = 0; i < getIdamDataNum(handle); i++) {
                feh[i] = (float)errhi[i];
                if (getIdamDataErrAsymmetry(handle)) fel[i] = (float)errlo[i];
            }
            break;
        case UDA_TYPE_DOUBLE:
            deh = (double*)getIdamDataErrHi(handle);
            del = (double*)getIdamDataErrLo(handle);
            for (i = 0; i < getIdamDataNum(handle); i++) {
                deh[i] = (double)errhi[i];
                if (getIdamDataErrAsymmetry(handle)) del[i] = (double)errlo[i];
            }
            break;
        case UDA_TYPE_INT:
            ieh = (int*)getIdamDataErrHi(handle);
            iel = (int*)getIdamDataErrLo(handle);
            for (i = 0; i < getIdamDataNum(handle); i++) {
                ieh[i] = (int)errhi[i];
                if (getIdamDataErrAsymmetry(handle)) iel[i] = (int)errlo[i];
            }
            break;
        case UDA_TYPE_LONG:
            leh = (long*)getIdamDataErrHi(handle);
            lel = (long*)getIdamDataErrLo(handle);
            for (i = 0; i < getIdamDataNum(handle); i++) {
                leh[i] = (long)errhi[i];
                if (getIdamDataErrAsymmetry(handle)) lel[i] = (long)errlo[i];
            }
            break;
        case UDA_TYPE_LONG64: {
            long long int* leh = (long long int*)getIdamDataErrHi(handle);
            long long int* lel = (long long int*)getIdamDataErrLo(handle);
            for (i = 0; i < getIdamDataNum(handle); i++) {
                leh[i] = (long long int)errhi[i];
                if (getIdamDataErrAsymmetry(handle)) lel[i] = (long long int)errlo[i];
            }
            break;
        }
        case UDA_TYPE_SHORT:
            seh = (short*)getIdamDataErrHi(handle);
            sel = (short*)getIdamDataErrLo(handle);
            for (i = 0; i < getIdamDataNum(handle); i++) {
                seh[i] = (short)errhi[i];
                if (getIdamDataErrAsymmetry(handle)) sel[i] = (short)errlo[i];
            }
            break;
        case UDA_TYPE_CHAR: {
            char* ceh = getIdamDataErrHi(handle);
            char* cel = getIdamDataErrLo(handle);
            for (i = 0; i < getIdamDataNum(handle); i++) {
                ceh[i] = errhi[i];
                if (getIdamDataErrAsymmetry(handle)) cel[i] = errlo[i];
            }
            break;
        }
        case UDA_TYPE_UNSIGNED_INT:
            ueh = (unsigned int*)getIdamDataErrHi(handle);
            uel = (unsigned int*)getIdamDataErrLo(handle);
            for (i = 0; i < getIdamDataNum(handle); i++) {
                ueh[i] = (unsigned int)errhi[i];
                if (getIdamDataErrAsymmetry(handle)) uel[i] = (unsigned int)errlo[i];
            }
            break;
        case UDA_TYPE_UNSIGNED_LONG: {
            unsigned long* leh = (unsigned long*)getIdamDataErrHi(handle);
            unsigned long* lel = (unsigned long*)getIdamDataErrLo(handle);
            for (i = 0; i < getIdamDataNum(handle); i++) {
                leh[i] = (unsigned long)errhi[i];
                if (getIdamDataErrAsymmetry(handle)) lel[i] = (unsigned long)errlo[i];
            }
            break;
        }
#ifndef __APPLE__
        case UDA_TYPE_UNSIGNED_LONG64: {
            unsigned long long int * leh = (unsigned long long int *) getIdamDataErrHi(handle);
            unsigned long long int * lel = (unsigned long long int *) getIdamDataErrLo(handle);
            for (i = 0; i < getIdamDataNum(handle); i++) {
                leh[i] = (unsigned long long int) errhi[i];
                if (getIdamDataErrAsymmetry(handle)) lel[i] = (unsigned long long int) errlo[i];
            }
            break;
        }
#endif
        case UDA_TYPE_UNSIGNED_SHORT: {
            unsigned short* seh = (unsigned short*)getIdamDataErrHi(handle);
            unsigned short* sel = (unsigned short*)getIdamDataErrLo(handle);
            for (i = 0; i < getIdamDataNum(handle); i++) {
                seh[i] = (unsigned short)errhi[i];
                if (getIdamDataErrAsymmetry(handle)) sel[i] = (unsigned short)errlo[i];
            }
            break;
        }
        case UDA_TYPE_UNSIGNED_CHAR: {
            unsigned char* ceh = (unsigned char*)getIdamDataErrHi(handle);
            unsigned char* cel = (unsigned char*)getIdamDataErrLo(handle);
            for (i = 0; i < getIdamDataNum(handle); i++) {
                ceh[i] = (unsigned char)errhi[i];
                if (getIdamDataErrAsymmetry(handle)) cel[i] = (unsigned char)errlo[i];
            }
            break;
        }

    }

//--------------------------------------------------------------------------------------------------------------
// Housekeeping

    free((void*)data);
    free((void*)errhi);
    free((void*)errlo);

    return 0;
}


int generateIdamDimDataError(int handle, int ndim)
{

    int i, err = 0, asymmetry = 0;

    float* fp, * feh, * fel, * data;
    char* errhi, * errlo;
    double* dp, * deh, * del;
    short* sp, * seh, * sel;
    int* ip, * ieh, * iel;
    long* lp, * leh, * lel;
    unsigned* up, * ueh, * uel;

    char* perrlo = NULL;

//--------------------------------------------------------------------------------------------------------------
// Check the handle and model are ok

    if (getIdamData(handle) == NULL) return 0;            // No Data Block available

    if (ndim < 0 || ndim > getIdamRank(handle)) return 0;

    int model;
    int param_n;
    float params[MAXERRPARAMS];

    getIdamErrorModel(handle, &model, &param_n, params);

    if (model <= ERROR_MODEL_UNKNOWN || model >= ERROR_MODEL_UNDEFINED) return 0;    // No valid Model

    if (getIdamDimNum(handle, ndim) <= 0) return 0;

//--------------------------------------------------------------------------------------------------------------
// Allocate local float work arrays and copy the data array to the work array

    if (getIdamDataType(handle) == UDA_TYPE_DCOMPLEX || getIdamDataType(handle) == UDA_TYPE_COMPLEX) {
        err = 999;
        addIdamError(CODEERRORTYPE, "generateIdamDimDataError", err,
                     "Not configured to Generate Complex Type Synthetic Data");
        return 999;
    }

    if ((data = (float*)malloc(getIdamDimNum(handle, ndim) * sizeof(float))) == NULL) return 1;
    if ((errhi = (char*)malloc(getIdamDimNum(handle, ndim) * sizeof(float))) == NULL) return 1;
    if ((errlo = (char*)malloc(getIdamDimNum(handle, ndim) * sizeof(float))) == NULL) return 1;

    switch (getIdamDataType(handle)) {
        case UDA_TYPE_FLOAT:
            fp = (float*)getIdamDimData(handle, ndim);
            for (i = 0; i < getIdamDimNum(handle, ndim); i++) data[i] = fp[i];        // Cast all data to Float
            break;
        case UDA_TYPE_DOUBLE:
            dp = (double*)getIdamDimData(handle, ndim);
            for (i = 0; i < getIdamDimNum(handle, ndim); i++) data[i] = (float)dp[i];
            break;
        case UDA_TYPE_INT:
            ip = (int*)getIdamDimData(handle, ndim);
            for (i = 0; i < getIdamDimNum(handle, ndim); i++) data[i] = (float)ip[i];
            break;
        case UDA_TYPE_LONG:
            lp = (long*)getIdamDimData(handle, ndim);
            for (i = 0; i < getIdamDimNum(handle, ndim); i++) data[i] = (float)lp[i];
            break;
        case UDA_TYPE_LONG64: {
            long long int* lp = (long long int*)getIdamDimData(handle, ndim);
            for (i = 0; i < getIdamDimNum(handle, ndim); i++) data[i] = (float)lp[i];
            break;
        }
        case UDA_TYPE_SHORT:
            sp = (short*)getIdamDimData(handle, ndim);
            for (i = 0; i < getIdamDimNum(handle, ndim); i++) data[i] = (float)sp[i];
            break;
        case UDA_TYPE_CHAR: {
            char* cp = getIdamDimData(handle, ndim);
            for (i = 0; i < getIdamDimNum(handle, ndim); i++) data[i] = (float)cp[i];
            break;
        }
        case UDA_TYPE_UNSIGNED_INT:
            up = (unsigned int*)getIdamDimData(handle, ndim);
            for (i = 0; i < getIdamDimNum(handle, ndim); i++) data[i] = (float)up[i];
            break;
        case UDA_TYPE_UNSIGNED_LONG: {
            unsigned long* lp = (unsigned long*)getIdamDimData(handle, ndim);
            for (i = 0; i < getIdamDimNum(handle, ndim); i++) data[i] = (float)lp[i];
            break;
        }
#ifndef __APPLE__
        case UDA_TYPE_UNSIGNED_LONG64: {
            unsigned long long int * lp = (unsigned long long int *) getIdamDimData(handle, ndim);
            for (i = 0; i < getIdamDimNum(handle, ndim); i++) data[i] = (float) lp[i];
            break;
        }
#endif
        case UDA_TYPE_UNSIGNED_SHORT: {
            unsigned short* sp = (unsigned short*)getIdamDimData(handle, ndim);
            for (i = 0; i < getIdamDimNum(handle, ndim); i++) data[i] = (float)sp[i];
            break;
        }
        case UDA_TYPE_UNSIGNED_CHAR: {
            unsigned char* cp = (unsigned char*)getIdamDimData(handle, ndim);
            for (i = 0; i < getIdamDimNum(handle, ndim); i++) data[i] = (float)cp[i];
            break;
        }
        default:
            free((void*)data);
            free((void*)errhi);
            free((void*)errlo);
            return 0;
    }

//--------------------------------------------------------------------------------------------------------------
// Generate Model Data

    err = idamErrorModel(model, param_n, params, getIdamDimNum(handle, ndim), data, &asymmetry, (float*)errhi,
                         (float*)errlo);

    if (err != 0) {
        free((void*)data);
        free((void*)errhi);
        free((void*)errlo);
        return err;
    }

//--------------------------------------------------------------------------------------------------------------
// Return the Synthetic Data and Error Array

    acc_setIdamDimErrType(handle, ndim, getIdamDimType(handle, ndim));
    acc_setIdamDimErrAsymmetry(handle, ndim, asymmetry);

    if (getIdamDimErrAsymmetry(handle, ndim) && getIdamDimErrLo(handle, ndim) == NULL) {
        if ((err = allocArray(getIdamDimType(handle, ndim), getIdamDimNum(handle, ndim), &perrlo))) return err;
        acc_setIdamDimErrLo(handle, ndim, perrlo);
    }

    switch (getIdamDimType(handle, ndim)) {
        case UDA_TYPE_FLOAT:
            feh = (float*)getIdamDimErrHi(handle, ndim);
            fel = (float*)getIdamDimErrLo(handle, ndim);
            for (i = 0; i < getIdamDimNum(handle, ndim); i++) {
                feh[i] = (float)errhi[i];
                if (getIdamDimErrAsymmetry(handle, ndim))fel[i] = (float)errlo[i];
            }
            break;
        case UDA_TYPE_DOUBLE:
            deh = (double*)getIdamDimErrHi(handle, ndim);
            del = (double*)getIdamDimErrLo(handle, ndim);
            for (i = 0; i < getIdamDimNum(handle, ndim); i++) {
                deh[i] = (double)errhi[i];
                if (getIdamDimErrAsymmetry(handle, ndim))del[i] = (double)errlo[i];
            }
            break;
        case UDA_TYPE_INT:
            ieh = (int*)getIdamDimErrHi(handle, ndim);
            iel = (int*)getIdamDimErrLo(handle, ndim);
            for (i = 0; i < getIdamDimNum(handle, ndim); i++) {
                ieh[i] = (int)errhi[i];
                if (getIdamDimErrAsymmetry(handle, ndim))iel[i] = (int)errlo[i];
            }
            break;
        case UDA_TYPE_UNSIGNED_INT:
            ueh = (unsigned int*)getIdamDimErrHi(handle, ndim);
            uel = (unsigned int*)getIdamDimErrLo(handle, ndim);
            for (i = 0; i < getIdamDimNum(handle, ndim); i++) {
                ueh[i] = (unsigned int)errhi[i];
                if (getIdamDimErrAsymmetry(handle, ndim))uel[i] = (unsigned int)errlo[i];
            }
            break;
        case UDA_TYPE_LONG:
            leh = (long*)getIdamDimErrHi(handle, ndim);
            lel = (long*)getIdamDimErrLo(handle, ndim);
            for (i = 0; i < getIdamDimNum(handle, ndim); i++) {
                leh[i] = (long)errhi[i];
                if (getIdamDimErrAsymmetry(handle, ndim))lel[i] = (long)errlo[i];
            }
            break;
        case UDA_TYPE_LONG64: {
            long long int* leh = (long long int*)getIdamDimErrHi(handle, ndim);
            long long int* lel = (long long int*)getIdamDimErrLo(handle, ndim);
            for (i = 0; i < getIdamDimNum(handle, ndim); i++) {
                leh[i] = (long long int)errhi[i];
                if (getIdamDimErrAsymmetry(handle, ndim))lel[i] = (long long int)errlo[i];
            }
            break;
        }
        case UDA_TYPE_SHORT:
            seh = (short*)getIdamDimErrHi(handle, ndim);
            sel = (short*)getIdamDimErrLo(handle, ndim);
            for (i = 0; i < getIdamDimNum(handle, ndim); i++) {
                seh[i] = (short)errhi[i];
                if (getIdamDimErrAsymmetry(handle, ndim))sel[i] = (short)errlo[i];
            }
            break;
        case UDA_TYPE_CHAR: {
            char* ceh = (char*)getIdamDimErrHi(handle, ndim);
            char* cel = (char*)getIdamDimErrLo(handle, ndim);
            for (i = 0; i < getIdamDimNum(handle, ndim); i++) {
                ceh[i] = (char)errhi[i];
                if (getIdamDimErrAsymmetry(handle, ndim))cel[i] = errlo[i];
            }
            break;
        }
        case UDA_TYPE_UNSIGNED_LONG: {
            unsigned long* leh = (unsigned long*)getIdamDimErrHi(handle, ndim);
            unsigned long* lel = (unsigned long*)getIdamDimErrLo(handle, ndim);
            for (i = 0; i < getIdamDimNum(handle, ndim); i++) {
                leh[i] = (unsigned long)errhi[i];
                if (getIdamDimErrAsymmetry(handle, ndim))lel[i] = (unsigned long)errlo[i];
            }
            break;
        }
#ifndef __APPLE__
        case UDA_TYPE_UNSIGNED_LONG64: {
            unsigned long long int * leh = (unsigned long long int *) getIdamDimErrHi(handle, ndim);
            unsigned long long int * lel = (unsigned long long int *) getIdamDimErrLo(handle, ndim);
            for (i = 0; i < getIdamDimNum(handle, ndim); i++) {
                leh[i] = (unsigned long long int) errhi[i];
                if (getIdamDimErrAsymmetry(handle, ndim))lel[i] = (unsigned long long int) errlo[i];
            }
            break;
        }
#endif
        case UDA_TYPE_UNSIGNED_SHORT: {
            unsigned short* seh = (unsigned short*)getIdamDimErrHi(handle, ndim);
            unsigned short* sel = (unsigned short*)getIdamDimErrLo(handle, ndim);
            for (i = 0; i < getIdamDimNum(handle, ndim); i++) {
                seh[i] = (unsigned short)errhi[i];
                if (getIdamDimErrAsymmetry(handle, ndim))sel[i] = (unsigned short)errlo[i];
            }
            break;
        }
        case UDA_TYPE_UNSIGNED_CHAR: {
            unsigned char* ceh = (unsigned char*)getIdamDimErrHi(handle, ndim);
            unsigned char* cel = (unsigned char*)getIdamDimErrLo(handle, ndim);
            for (i = 0; i < getIdamDimNum(handle, ndim); i++) {
                ceh[i] = (unsigned char)errhi[i];
                if (getIdamDimErrAsymmetry(handle, ndim))cel[i] = (unsigned char)errlo[i];
            }
            break;
        }
    }

//--------------------------------------------------------------------------------------------------------------
// Housekeeping

    free((void*)data);
    free((void*)errhi);
    free((void*)errlo);

    return 0;
}

