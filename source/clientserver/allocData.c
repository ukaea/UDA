/*---------------------------------------------------------------
* Allocate Memory for the Data and Dim Blocks and Data Error Block
*
* Arguments:	DATA_BLOCK *		Data Block Structure
*
* Returns:	allocData		0 if heap allocation was successful
*		DATA_BLOCK->data	Pointer to Memory Block
*
*--------------------------------------------------------------*/

#include "allocData.h"

#include <logging/idamLog.h>
#include <include/idamtypes.h>
#include <stdlib.h>
#include "initStructs.h"
#include "idamErrors.h"

#ifdef SERVERBUILD
#  include <server/idamServerStartup.h>
#endif

int allocArray(int data_type, int n_data, char** ap)
{

// Generic function to (Re)Allocate Memory for a typed Array

    if (n_data <= 0) return 0; // Insufficient Data to Allocate!
    size_t ndata = (size_t) n_data;

    switch (data_type) {
        case TYPE_CHAR :
            *ap = (char*) realloc((void*) *ap, ndata * sizeof(char));
            break;
        case TYPE_SHORT :
            *ap = (char*) realloc((void*) *ap, ndata * sizeof(short));
            break;
        case TYPE_INT :
            *ap = (char*) realloc((void*) *ap, ndata * sizeof(int));
            break;
        case TYPE_LONG :
            *ap = (char*) realloc((void*) *ap, ndata * sizeof(long));
            break;
        case TYPE_LONG64 :
            *ap = (char*) realloc((void*) *ap, ndata * sizeof(long long int));
            break;
        case TYPE_FLOAT :
            *ap = (char*) realloc((void*) *ap, ndata * sizeof(float));
            break;
        case TYPE_DOUBLE :
            *ap = (char*) realloc((void*) *ap, ndata * sizeof(double));
            break;
        case TYPE_UNSIGNED_CHAR :
            *ap = (char*) realloc((void*) *ap, ndata * sizeof(unsigned char));
            break;
        case TYPE_UNSIGNED_SHORT :
            *ap = (char*) realloc((void*) *ap, ndata * sizeof(unsigned short));
            break;
        case TYPE_UNSIGNED_INT :
            *ap = (char*) realloc((void*) *ap, ndata * sizeof(unsigned int));
            break;
        case TYPE_UNSIGNED_LONG :
            *ap = (char*) realloc((void*) *ap, ndata * sizeof(unsigned long));
            break;
#ifndef __APPLE__
        case TYPE_UNSIGNED_LONG64 :
            *ap = (char*) realloc((void*) *ap, ndata * sizeof(unsigned long long int));
            break;
#endif
        case TYPE_STRING :
            *ap = (char*) realloc((void*) *ap, ndata * sizeof(char));
            break;
        case TYPE_DCOMPLEX :
            *ap = (char*) realloc((void*) *ap, ndata * sizeof(DCOMPLEX));
            break;
        case TYPE_COMPLEX :
            *ap = (char*) realloc((void*) *ap, ndata * sizeof(COMPLEX));
            break;
        case TYPE_COMPOUND :
            *ap = NULL;
            break;
        default:
            return (UNKNOWN_DATA_TYPE);
    }

    if (*ap == NULL && data_type != TYPE_COMPOUND) return (ERROR_ALLOCATING_HEAP);
    return 0;
}


int allocData(DATA_BLOCK* data_block)
{

// Main routine for allocating memory for Data and Errors

    unsigned int i;
    unsigned int ndata;
    char* db = NULL, * ebh = NULL, * ebl = NULL;

    //unsigned int allocCount = 0;

    //data_block->totalDataBlockSize += sizeof(DATA_BLOCK) + data_block->rank*sizeof(DIMS);

//------------------------------------------------------------------------
// Allocate Memory for data Dimensions

    if (data_block->rank > 0) {
        data_block->dims = (DIMS*) malloc(data_block->rank * sizeof(DIMS));
        if (data_block->dims == NULL) return (ERROR_ALLOCATING_HEAP);
        for (i = 0; i < data_block->rank; i++) initDimBlock(&data_block->dims[i]);
    }

//------------------------------------------------------------------------
// Allocate Memory for data and errors

    if ((ndata = (unsigned int) data_block->data_n) == 0) return 1;   // Insufficient Data to Allocate!

    switch (data_block->data_type) {
        case TYPE_CHAR :
            db = (char*) malloc(ndata * sizeof(char));
            if (data_block->error_type == TYPE_UNKNOWN) {
                ebh = (char*) malloc(ndata * sizeof(char));        // Will be same type as Data if Accessed
                if (data_block->errasymmetry) ebl = (char*) malloc(ndata * sizeof(char));
            }
            break;
        case TYPE_SHORT :
            db = (char*) malloc(ndata * sizeof(short));
            if (data_block->error_type == TYPE_UNKNOWN) {
                ebh = (char*) malloc(ndata * sizeof(short));
                if (data_block->errasymmetry) ebl = (char*) malloc(ndata * sizeof(short));
            }
            break;
        case TYPE_INT :
            db = (char*) malloc(ndata * sizeof(int));
            if (data_block->error_type == TYPE_UNKNOWN) {
                ebh = (char*) malloc(ndata * sizeof(int));
                if (data_block->errasymmetry) ebl = (char*) malloc(ndata * sizeof(int));
            }
            break;
        case TYPE_LONG :
            db = (char*) malloc(ndata * sizeof(int));
            if (data_block->error_type == TYPE_UNKNOWN) {
                ebh = (char*) malloc(ndata * sizeof(long));
                if (data_block->errasymmetry) ebl = (char*) malloc(ndata * sizeof(long));
            }
            break;
        case TYPE_LONG64 :
            db = (char*) malloc(ndata * sizeof(long long int));
            if (data_block->error_type == TYPE_UNKNOWN) {
                ebh = (char*) malloc(ndata * sizeof(long long int));
                if (data_block->errasymmetry) ebl = (char*) malloc(ndata * sizeof(long long int));
            }
            break;
        case TYPE_FLOAT :
            db = (char*) malloc(ndata * sizeof(float));
            if (data_block->error_type == TYPE_UNKNOWN) {
                ebh = (char*) malloc(ndata * sizeof(float));
                if (data_block->errasymmetry) ebl = (char*) malloc(ndata * sizeof(float));
            }
            break;
        case TYPE_DOUBLE :
            db = (char*) malloc(ndata * sizeof(double));
            if (data_block->error_type == TYPE_UNKNOWN) {
                ebh = (char*) malloc(ndata * sizeof(double));
                if (data_block->errasymmetry) ebl = (char*) malloc(ndata * sizeof(double));
            }
            break;
        case TYPE_UNSIGNED_CHAR :
            db = (char*) malloc(ndata * sizeof(unsigned char));
            if (data_block->error_type == TYPE_UNKNOWN) {
                ebh = (char*) malloc(ndata * sizeof(unsigned char));        // Will be same type as Data if Accessed
                if (data_block->errasymmetry) ebl = (char*) malloc(ndata * sizeof(unsigned char));
            }
            break;
        case TYPE_UNSIGNED_SHORT :
            db = (char*) malloc(ndata * sizeof(unsigned short));
            if (data_block->error_type == TYPE_UNKNOWN) {
                ebh = (char*) malloc(ndata * sizeof(unsigned short));
                if (data_block->errasymmetry) ebl = (char*) malloc(ndata * sizeof(unsigned short));
            }
            break;
        case TYPE_UNSIGNED_INT :
            db = (char*) malloc(ndata * sizeof(unsigned int));
            if (data_block->error_type == TYPE_UNKNOWN) {
                ebh = (char*) malloc(ndata * sizeof(unsigned int));
                if (data_block->errasymmetry) ebl = (char*) malloc(ndata * sizeof(unsigned int));
            }
            break;
        case TYPE_UNSIGNED_LONG :
            db = (char*) malloc(ndata * sizeof(unsigned long));
            if (data_block->error_type == TYPE_UNKNOWN) {
                ebh = (char*) malloc(ndata * sizeof(unsigned long));
                if (data_block->errasymmetry) ebl = (char*) malloc(ndata * sizeof(unsigned long));
            }
            break;
#ifndef __APPLE__
        case TYPE_UNSIGNED_LONG64 :
            db = (char*) malloc(ndata * sizeof(unsigned long long int));
            if (data_block->error_type == TYPE_UNKNOWN) {
                ebh = (char*) malloc(ndata * sizeof(unsigned long long int));
                if (data_block->errasymmetry) ebl = (char*) malloc(ndata * sizeof(unsigned long long int));
            }
            break;
#endif
        case TYPE_STRING :
            db = (char*) malloc(ndata * sizeof(char));
            if (data_block->error_type == TYPE_UNKNOWN) {
                ebh = (char*) malloc(ndata * sizeof(char));        // Will be same type as Data if Accessed
                if (data_block->errasymmetry) ebl = (char*) malloc(ndata * sizeof(char));
            }
            break;
        case TYPE_DCOMPLEX :
            db = (char*) malloc(ndata * sizeof(DCOMPLEX));
            if (data_block->error_type == TYPE_UNKNOWN) {
                ebh = (char*) malloc(ndata * sizeof(DCOMPLEX));
                if (data_block->errasymmetry) ebl = (char*) malloc(ndata * sizeof(DCOMPLEX));
            }
            break;
        case TYPE_COMPLEX :
            db = (char*) malloc(ndata * sizeof(COMPLEX));
            if (data_block->error_type == TYPE_UNKNOWN) {
                ebh = (char*) malloc(ndata * sizeof(COMPLEX));
                if (data_block->errasymmetry) ebl = (char*) malloc(ndata * sizeof(COMPLEX));
            }
            break;
        case TYPE_COMPOUND:
            db = NULL;
            ebh = NULL;
            ebl = NULL;
            break;
        default:
            return (UNKNOWN_DATA_TYPE);
    }

    idamLog(LOG_DEBUG, "allocData :\n");
    idamLog(LOG_DEBUG, "rank      : %d\n", data_block->rank);
    idamLog(LOG_DEBUG, "count     : %d\n", data_block->data_n);
    idamLog(LOG_DEBUG, "data_type : %d\n", data_block->data_type);
    idamLog(LOG_DEBUG, "error_type: %d\n", data_block->error_type);
    idamLog(LOG_DEBUG, "data  != NULL: %d\n", db != NULL);
    idamLog(LOG_DEBUG, "errhi != NULL: %d\n", ebh != NULL);
    idamLog(LOG_DEBUG, "errlo != NULL: %d\n", ebl != NULL);

    if (db == NULL && data_block->data_type != TYPE_COMPOUND) {
        idamLog(LOG_DEBUG, "allocData: Unable to Allocate Heap Memory for Data \n");
        return (ERROR_ALLOCATING_HEAP);
    }

    switch (data_block->error_type) {    // maybe of lower precision
        case TYPE_CHAR :
            ebh = (char*) malloc(ndata * sizeof(char));
            if (data_block->errasymmetry) ebl = (char*) malloc(ndata * sizeof(char));
            break;
        case TYPE_SHORT :
            ebh = (char*) malloc(ndata * sizeof(short));
            if (data_block->errasymmetry) ebl = (char*) malloc(ndata * sizeof(short));
            break;
        case TYPE_INT :
            ebh = (char*) malloc(ndata * sizeof(int));
            if (data_block->errasymmetry) ebl = (char*) malloc(ndata * sizeof(int));
            break;
        case TYPE_LONG :
            ebh = (char*) malloc(ndata * sizeof(long));
            if (data_block->errasymmetry) ebl = (char*) malloc(ndata * sizeof(long));
            break;
        case TYPE_LONG64 :
            ebh = (char*) malloc(ndata * sizeof(long long int));
            if (data_block->errasymmetry) ebl = (char*) malloc(ndata * sizeof(long long int));
            break;
        case TYPE_FLOAT :
            ebh = (char*) malloc(ndata * sizeof(float));
            if (data_block->errasymmetry) ebl = (char*) malloc(ndata * sizeof(float));
            break;
        case TYPE_DOUBLE :
            ebh = (char*) malloc(ndata * sizeof(double));
            if (data_block->errasymmetry) ebl = (char*) malloc(ndata * sizeof(double));
            break;
        case TYPE_UNSIGNED_CHAR :
            ebh = (char*) malloc(ndata * sizeof(unsigned char));
            if (data_block->errasymmetry) ebl = (char*) malloc(ndata * sizeof(unsigned char));
            break;
        case TYPE_UNSIGNED_SHORT :
            ebh = (char*) malloc(ndata * sizeof(unsigned short));
            if (data_block->errasymmetry) ebl = (char*) malloc(ndata * sizeof(unsigned short));
            break;
        case TYPE_UNSIGNED_INT :
            ebh = (char*) malloc(ndata * sizeof(unsigned int));
            if (data_block->errasymmetry) ebl = (char*) malloc(ndata * sizeof(unsigned int));
            break;
        case TYPE_UNSIGNED_LONG :
            ebh = (char*) malloc(ndata * sizeof(unsigned long));
            if (data_block->errasymmetry) ebl = (char*) malloc(ndata * sizeof(unsigned long));
            break;
#ifndef __APPLE__
        case TYPE_UNSIGNED_LONG64 :
            ebh = (char*) malloc(ndata * sizeof(unsigned long long int));
            if (data_block->errasymmetry) ebl = (char*) malloc(ndata * sizeof(unsigned long long int));
            break;
#endif
        case TYPE_STRING :
            ebh = (char*) malloc(ndata * sizeof(char));
            if (data_block->errasymmetry) ebl = (char*) malloc(ndata * sizeof(char));
            break;
        case TYPE_DCOMPLEX :
            ebh = (char*) malloc(ndata * sizeof(DCOMPLEX));
            if (data_block->errasymmetry) ebl = (char*) malloc(ndata * sizeof(DCOMPLEX));
            break;
        case TYPE_COMPLEX :
            ebh = (char*) malloc(ndata * sizeof(COMPLEX));
            if (data_block->errasymmetry) ebl = (char*) malloc(ndata * sizeof(COMPLEX));
            break;
        case TYPE_COMPOUND :
            ebh = NULL;
            ebl = NULL;
            break;

        default:
            break;
    }

    if ((ebh == NULL || (ebl == NULL && data_block->errasymmetry)) &&
        (data_block->error_type != TYPE_COMPOUND && data_block->error_type != TYPE_UNKNOWN))
        return (ERROR_ALLOCATING_HEAP);

    data_block->data = db;
    data_block->errhi = ebh;
    data_block->errlo = ebl;

    return 0;
}


int allocDim(DATA_BLOCK* data_block)
{

// This routine is only called by the Client if data
// are NOT in a compressed form, when Heap is
// allocated by the Dimension Uncompression function.
//
// It may or may not be called by a Server Plugin.

    unsigned int i;
    unsigned int ndata;
    char* db = NULL, * ebh = NULL, * ebl = NULL;
    //unsigned int allocCount = 0;

    for (i = 0; i < data_block->rank; i++) {

        ndata = (unsigned int) data_block->dims[i].dim_n;

        if (ndata == 0) return 1;   // Insufficient Data to Allocate!

        switch (data_block->dims[i].data_type) {
            case TYPE_FLOAT :
                //allocCount = ndata * sizeof(float);
                db = (char*) malloc(ndata * sizeof(float));
                if (data_block->dims[i].error_type == TYPE_UNKNOWN) {
                    ebh = (char*) malloc(ndata * sizeof(float));
                    if (data_block->dims[i].errasymmetry) ebl = (char*) malloc(ndata * sizeof(float));
                }
                break;
            case TYPE_DOUBLE :
                //allocCount = ndata * sizeof(double);
                db = (char*) malloc(ndata * sizeof(double));
                if (data_block->dims[i].error_type == TYPE_UNKNOWN) {
                    ebh = (char*) malloc(ndata * sizeof(double));
                    if (data_block->dims[i].errasymmetry) ebl = (char*) malloc(ndata * sizeof(double));
                }
                break;
            case TYPE_INT :
                //allocCount = ndata * sizeof(int);
                db = (char*) malloc(ndata * sizeof(int));
                if (data_block->dims[i].error_type == TYPE_UNKNOWN) {
                    ebh = (char*) malloc(ndata * sizeof(int));
                    if (data_block->dims[i].errasymmetry) ebl = (char*) malloc(ndata * sizeof(int));
                }
                break;
            case TYPE_UNSIGNED_INT :
                //allocCount = ndata * sizeof(unsigned int);
                db = (char*) malloc(ndata * sizeof(unsigned int));
                if (data_block->dims[i].error_type == TYPE_UNKNOWN) {
                    ebh = (char*) malloc(ndata * sizeof(unsigned int));
                    if (data_block->dims[i].errasymmetry) ebl = (char*) malloc(ndata * sizeof(unsigned int));
                }
                break;
            case TYPE_LONG :
                //allocCount = ndata * sizeof(long);
                db = (char*) malloc(ndata * sizeof(long));
                if (data_block->dims[i].error_type == TYPE_UNKNOWN) {
                    ebh = (char*) malloc(ndata * sizeof(long));
                    if (data_block->dims[i].errasymmetry) ebl = (char*) malloc(ndata * sizeof(long));
                }
                break;
            case TYPE_LONG64 :
                //allocCount = ndata * sizeof(long long int);
                db = (char*) malloc(ndata * sizeof(long long int));
                if (data_block->dims[i].error_type == TYPE_UNKNOWN) {
                    ebh = (char*) malloc(ndata * sizeof(long long int));
                    if (data_block->dims[i].errasymmetry) ebl = (char*) malloc(ndata * sizeof(long long int));
                }
                break;
            case TYPE_SHORT :
                //allocCount = ndata * sizeof(short);
                db = (char*) malloc(ndata * sizeof(short));
                if (data_block->dims[i].error_type == TYPE_UNKNOWN) {
                    ebh = (char*) malloc(ndata * sizeof(short));
                    if (data_block->dims[i].errasymmetry) ebl = (char*) malloc(ndata * sizeof(short));
                }
                break;
            case TYPE_CHAR :
                //allocCount = ndata * sizeof(char);
                db = (char*) malloc(ndata * sizeof(char));
                if (data_block->dims[i].error_type == TYPE_UNKNOWN) {
                    ebh = (char*) malloc(ndata * sizeof(char));
                    if (data_block->dims[i].errasymmetry) ebl = (char*) malloc(ndata * sizeof(char));
                }
                break;
            case TYPE_UNSIGNED_LONG :
                //allocCount = ndata * sizeof(unsigned long);
                db = (char*) malloc(ndata * sizeof(unsigned long));
                if (data_block->dims[i].error_type == TYPE_UNKNOWN) {
                    ebh = (char*) malloc(ndata * sizeof(unsigned long));
                    if (data_block->dims[i].errasymmetry) ebl = (char*) malloc(ndata * sizeof(unsigned long));
                }
                break;
#ifndef __APPLE__
            case TYPE_UNSIGNED_LONG64 :
                //allocCount = ndata * sizeof(unsigned long long int);
                db = (char*) malloc(ndata * sizeof(unsigned long long int));
                if (data_block->dims[i].error_type == TYPE_UNKNOWN) {
                    ebh = (char*) malloc(ndata * sizeof(unsigned long long int));
                    if (data_block->dims[i].errasymmetry) ebl = (char*) malloc(ndata * sizeof(unsigned long long int));
                }
                break;
#endif
            case TYPE_UNSIGNED_SHORT :
                //allocCount = ndata * sizeof(unsigned short);
                db = (char*) malloc(ndata * sizeof(unsigned short));
                if (data_block->dims[i].error_type == TYPE_UNKNOWN) {
                    ebh = (char*) malloc(ndata * sizeof(unsigned short));
                    if (data_block->dims[i].errasymmetry) ebl = (char*) malloc(ndata * sizeof(unsigned short));
                }
                break;
            case TYPE_UNSIGNED_CHAR :
                //allocCount = ndata * sizeof(unsigned char);
                db = (char*) malloc(ndata * sizeof(unsigned char));
                if (data_block->dims[i].error_type == TYPE_UNKNOWN) {
                    ebh = (char*) malloc(ndata * sizeof(unsigned char));
                    if (data_block->dims[i].errasymmetry) ebl = (char*) malloc(ndata * sizeof(unsigned char));
                }
                break;
            case TYPE_DCOMPLEX :
                //allocCount = ndata * sizeof(DCOMPLEX);
                db = (char*) malloc(ndata * sizeof(DCOMPLEX));
                if (data_block->dims[i].error_type == TYPE_UNKNOWN) {
                    ebh = (char*) malloc(ndata * sizeof(DCOMPLEX));
                    if (data_block->dims[i].errasymmetry) ebl = (char*) malloc(ndata * sizeof(DCOMPLEX));
                }
                break;
            case TYPE_COMPLEX :
                //allocCount = ndata * sizeof(COMPLEX);
                db = (char*) malloc(ndata * sizeof(COMPLEX));
                if (data_block->dims[i].error_type == TYPE_UNKNOWN) {
                    ebh = (char*) malloc(ndata * sizeof(COMPLEX));
                    if (data_block->dims[i].errasymmetry) ebl = (char*) malloc(ndata * sizeof(COMPLEX));
                }
                break;
            default:
                return (UNKNOWN_DATA_TYPE);
        }
        switch (data_block->dims[i].error_type) {
            case TYPE_FLOAT :
                //allocCount = ndata * sizeof(float);
                ebh = (char*) malloc(ndata * sizeof(float));
                if (data_block->dims[i].errasymmetry) ebl = (char*) malloc(ndata * sizeof(float));
                break;
            case TYPE_DOUBLE :
                //allocCount = ndata * sizeof(double);
                ebh = (char*) malloc(ndata * sizeof(double));
                if (data_block->dims[i].errasymmetry) ebl = (char*) malloc(ndata * sizeof(double));
                break;
            case TYPE_INT :
                //allocCount = ndata * sizeof(int);
                ebh = (char*) malloc(ndata * sizeof(int));
                if (data_block->dims[i].errasymmetry) ebl = (char*) malloc(ndata * sizeof(int));
                break;
            case TYPE_UNSIGNED_INT :
                //allocCount = ndata * sizeof(unsigned int);
                ebh = (char*) malloc(ndata * sizeof(unsigned int));
                if (data_block->dims[i].errasymmetry) ebl = (char*) malloc(ndata * sizeof(unsigned int));
                break;
            case TYPE_LONG :
                //allocCount = ndata * sizeof(long);
                ebh = (char*) malloc(ndata * sizeof(long));
                if (data_block->dims[i].errasymmetry) ebl = (char*) malloc(ndata * sizeof(long));
                break;
            case TYPE_LONG64 :
                //allocCount = ndata * sizeof(long long int);
                ebh = (char*) malloc(ndata * sizeof(long long int));
                if (data_block->dims[i].errasymmetry) ebl = (char*) malloc(ndata * sizeof(long long int));
                break;
            case TYPE_SHORT :
                //allocCount = ndata * sizeof(short);
                ebh = (char*) malloc(ndata * sizeof(short));
                if (data_block->dims[i].errasymmetry) ebl = (char*) malloc(ndata * sizeof(short));
                break;
            case TYPE_CHAR :
                //allocCount = ndata * sizeof(char);
                ebh = (char*) malloc(ndata * sizeof(char));
                if (data_block->dims[i].errasymmetry) ebl = (char*) malloc(ndata * sizeof(char));
                break;
            case TYPE_UNSIGNED_LONG :
                //allocCount = ndata * sizeof(unsigned long);
                ebh = (char*) malloc(ndata * sizeof(unsigned long));
                if (data_block->dims[i].errasymmetry) ebl = (char*) malloc(ndata * sizeof(unsigned long));
                break;
#ifndef __APPLE__
            case TYPE_UNSIGNED_LONG64 :
                //allocCount = ndata * sizeof(unsigned long long int);
                ebh = (char*) malloc(ndata * sizeof(unsigned long long int));
                if (data_block->dims[i].errasymmetry) ebl = (char*) malloc(ndata * sizeof(unsigned long long int));
                break;
#endif
            case TYPE_UNSIGNED_SHORT :
                //allocCount = ndata * sizeof(unsigned short);
                ebh = (char*) malloc(ndata * sizeof(unsigned short));
                if (data_block->dims[i].errasymmetry) ebl = (char*) malloc(ndata * sizeof(unsigned short));
                break;
            case TYPE_UNSIGNED_CHAR :
                //allocCount = ndata * sizeof(unsigned char);
                ebh = (char*) malloc(ndata * sizeof(unsigned char));
                if (data_block->dims[i].errasymmetry) ebl = (char*) malloc(ndata * sizeof(unsigned char));
                break;
            case TYPE_DCOMPLEX :
                //allocCount = ndata * sizeof(DCOMPLEX);
                ebh = (char*) malloc(ndata * sizeof(DCOMPLEX));
                if (data_block->dims[i].errasymmetry) ebl = (char*) malloc(ndata * sizeof(DCOMPLEX));
                break;
            case TYPE_COMPLEX :
                //allocCount = ndata * sizeof(COMPLEX);
                ebh = (char*) malloc(ndata * sizeof(COMPLEX));
                if (data_block->dims[i].errasymmetry) ebl = (char*) malloc(ndata * sizeof(COMPLEX));
                break;
            default:
                break;
        }

        if (db == NULL) return (ERROR_ALLOCATING_HEAP);
        if (ebh == NULL || (ebl == NULL && data_block->dims[i].errasymmetry)) return (ERROR_ALLOCATING_HEAP);

        data_block->dims[i].dim = db;
        data_block->dims[i].errhi = ebh;
        data_block->dims[i].errlo = ebl;

// Allocate Heap for Compressed Dimension Domains

        if (data_block->dims[i].compressed && data_block->dims[i].method > 0) {

            data_block->dims[i].sams = NULL;
            data_block->dims[i].offs = NULL;
            data_block->dims[i].ints = NULL;

            switch (data_block->dims[i].data_type) {
                case TYPE_CHAR :
                    //allocCount = sizeof(char);
                    switch (data_block->dims[i].method) {
                        case 1 :
                            data_block->dims[i].sams = (long*) malloc(data_block->dims[i].udoms * sizeof(long));
                            data_block->dims[i].offs = (char*) malloc(data_block->dims[i].udoms * sizeof(char));
                            data_block->dims[i].ints = (char*) malloc(data_block->dims[i].udoms * sizeof(char));
                            break;
                        case 2 :
                            data_block->dims[i].offs = (char*) malloc(data_block->dims[i].udoms * sizeof(char));
                            break;
                        case 3 :
                            data_block->dims[i].offs = (char*) malloc(sizeof(char));
                            data_block->dims[i].ints = (char*) malloc(sizeof(char));
                            break;
                    }
                    break;
                case TYPE_SHORT :
                    //allocCount = sizeof(short);
                    switch (data_block->dims[i].method) {
                        case 1 :
                            data_block->dims[i].sams = (long*) malloc(data_block->dims[i].udoms * sizeof(long));
                            data_block->dims[i].offs = (char*) malloc(data_block->dims[i].udoms * sizeof(short));
                            data_block->dims[i].ints = (char*) malloc(data_block->dims[i].udoms * sizeof(short));
                            break;
                        case 2 :
                            data_block->dims[i].offs = (char*) malloc(data_block->dims[i].udoms * sizeof(short));
                            break;
                        case 3 :
                            data_block->dims[i].offs = (char*) malloc(sizeof(short));
                            data_block->dims[i].ints = (char*) malloc(sizeof(short));
                            break;
                    }
                    break;
                case TYPE_INT :
                    //allocCount = sizeof(int);
                    switch (data_block->dims[i].method) {
                        case 1 :
                            data_block->dims[i].sams = (long*) malloc(data_block->dims[i].udoms * sizeof(long));
                            data_block->dims[i].offs = (char*) malloc(data_block->dims[i].udoms * sizeof(int));
                            data_block->dims[i].ints = (char*) malloc(data_block->dims[i].udoms * sizeof(int));
                            break;
                        case 2 :
                            data_block->dims[i].offs = (char*) malloc(data_block->dims[i].udoms * sizeof(int));
                            break;
                        case 3 :
                            data_block->dims[i].offs = (char*) malloc(sizeof(int));
                            data_block->dims[i].ints = (char*) malloc(sizeof(int));
                            break;
                    }
                    break;
                case TYPE_LONG :
                    //allocCount = sizeof(long);
                    switch (data_block->dims[i].method) {
                        case 1 :
                            data_block->dims[i].sams = (long*) malloc(data_block->dims[i].udoms * sizeof(long));
                            data_block->dims[i].offs = (char*) malloc(data_block->dims[i].udoms * sizeof(long));
                            data_block->dims[i].ints = (char*) malloc(data_block->dims[i].udoms * sizeof(long));
                            break;
                        case 2 :
                            data_block->dims[i].offs = (char*) malloc(data_block->dims[i].udoms * sizeof(long));
                            break;
                        case 3 :
                            data_block->dims[i].offs = (char*) malloc(sizeof(long));
                            data_block->dims[i].ints = (char*) malloc(sizeof(long));
                            break;
                    }
                    break;
                case TYPE_LONG64 :
                    //allocCount = sizeof(long long int);
                    switch (data_block->dims[i].method) {
                        case 1 :
                            data_block->dims[i].sams = (long*) malloc(
                                    data_block->dims[i].udoms * sizeof(long long int));
                            data_block->dims[i].offs = (char*) malloc(
                                    data_block->dims[i].udoms * sizeof(long long int));
                            data_block->dims[i].ints = (char*) malloc(
                                    data_block->dims[i].udoms * sizeof(long long int));
                            break;
                        case 2 :
                            data_block->dims[i].offs = (char*) malloc(
                                    data_block->dims[i].udoms * sizeof(long long int));
                            break;
                        case 3 :
                            data_block->dims[i].offs = (char*) malloc(sizeof(long long int));
                            data_block->dims[i].ints = (char*) malloc(sizeof(long long int));
                            break;
                    }
                    break;
                case TYPE_FLOAT :
                    //allocCount = sizeof(float);
                    switch (data_block->dims[i].method) {
                        case 1 :
                            data_block->dims[i].sams = (long*) malloc(data_block->dims[i].udoms * sizeof(long));
                            data_block->dims[i].offs = (char*) malloc(data_block->dims[i].udoms * sizeof(float));
                            data_block->dims[i].ints = (char*) malloc(data_block->dims[i].udoms * sizeof(float));
                            break;
                        case 2 :
                            data_block->dims[i].offs = (char*) malloc(data_block->dims[i].udoms * sizeof(float));
                            break;
                        case 3 :
                            data_block->dims[i].offs = (char*) malloc(sizeof(float));
                            data_block->dims[i].ints = (char*) malloc(sizeof(float));
                            break;
                    }
                    break;

                case TYPE_DOUBLE :
                    //allocCount = sizeof(double);
                    switch (data_block->dims[i].method) {
                        case 1 :
                            data_block->dims[i].sams = (long*) malloc(data_block->dims[i].udoms * sizeof(long));
                            data_block->dims[i].offs = (char*) malloc(data_block->dims[i].udoms * sizeof(double));
                            data_block->dims[i].ints = (char*) malloc(data_block->dims[i].udoms * sizeof(double));
                            break;
                        case 2 :
                            data_block->dims[i].offs = (char*) malloc(data_block->dims[i].udoms * sizeof(double));
                            break;
                        case 3 :
                            data_block->dims[i].offs = (char*) malloc(sizeof(double));
                            data_block->dims[i].ints = (char*) malloc(sizeof(double));
                            break;
                    }
                    break;
                case TYPE_UNSIGNED_CHAR :
                    //allocCount = sizeof(unsigned char);
                    switch (data_block->dims[i].method) {
                        case 1 :
                            data_block->dims[i].sams = (long*) malloc(data_block->dims[i].udoms * sizeof(long));
                            data_block->dims[i].offs = (char*) malloc(
                                    data_block->dims[i].udoms * sizeof(unsigned char));
                            data_block->dims[i].ints = (char*) malloc(
                                    data_block->dims[i].udoms * sizeof(unsigned char));
                            break;
                        case 2 :
                            data_block->dims[i].offs = (char*) malloc(
                                    data_block->dims[i].udoms * sizeof(unsigned char));
                            break;
                        case 3 :
                            data_block->dims[i].offs = (char*) malloc(sizeof(unsigned char));
                            data_block->dims[i].ints = (char*) malloc(sizeof(unsigned char));
                            break;
                    }
                    break;
                case TYPE_UNSIGNED_SHORT :
                    //allocCount = sizeof(unsigned short);
                    switch (data_block->dims[i].method) {
                        case 1 :
                            data_block->dims[i].sams = (long*) malloc(data_block->dims[i].udoms * sizeof(long));
                            data_block->dims[i].offs = (char*) malloc(
                                    data_block->dims[i].udoms * sizeof(unsigned short));
                            data_block->dims[i].ints = (char*) malloc(
                                    data_block->dims[i].udoms * sizeof(unsigned short));
                            break;
                        case 2 :
                            data_block->dims[i].offs = (char*) malloc(
                                    data_block->dims[i].udoms * sizeof(unsigned short));
                            break;
                        case 3 :
                            data_block->dims[i].offs = (char*) malloc(sizeof(unsigned short));
                            data_block->dims[i].ints = (char*) malloc(sizeof(unsigned short));
                            break;
                    }
                    break;
                case TYPE_UNSIGNED_INT :
                    //allocCount = sizeof(unsigned int);
                    switch (data_block->dims[i].method) {
                        case 1 :
                            data_block->dims[i].sams = (long*) malloc(data_block->dims[i].udoms * sizeof(long));
                            data_block->dims[i].offs = (char*) malloc(data_block->dims[i].udoms * sizeof(unsigned int));
                            data_block->dims[i].ints = (char*) malloc(data_block->dims[i].udoms * sizeof(unsigned int));
                            break;
                        case 2 :
                            data_block->dims[i].offs = (char*) malloc(data_block->dims[i].udoms * sizeof(unsigned int));
                            break;
                        case 3 :
                            data_block->dims[i].offs = (char*) malloc(sizeof(unsigned int));
                            data_block->dims[i].ints = (char*) malloc(sizeof(unsigned int));
                            break;
                    }
                    break;
                case TYPE_UNSIGNED_LONG :
                    //allocCount = sizeof(unsigned long);
                    switch (data_block->dims[i].method) {
                        case 1 :
                            data_block->dims[i].sams = (long*) malloc(data_block->dims[i].udoms * sizeof(long));
                            data_block->dims[i].offs = (char*) malloc(
                                    data_block->dims[i].udoms * sizeof(unsigned long));
                            data_block->dims[i].ints = (char*) malloc(
                                    data_block->dims[i].udoms * sizeof(unsigned long));
                            break;
                        case 2 :
                            data_block->dims[i].offs = (char*) malloc(
                                    data_block->dims[i].udoms * sizeof(unsigned long));
                            break;
                        case 3 :
                            data_block->dims[i].offs = (char*) malloc(sizeof(unsigned long));
                            data_block->dims[i].ints = (char*) malloc(sizeof(unsigned long));
                            break;
                    }
                    break;
#ifndef __APPLE__
                case TYPE_UNSIGNED_LONG64 :
                    //allocCount = sizeof(unsigned long long int);
                    switch (data_block->dims[i].method) {
                        case 1 :
                            data_block->dims[i].sams = (long*) malloc(data_block->dims[i].udoms * sizeof(long));
                            data_block->dims[i].offs = (char*) malloc(
                                    data_block->dims[i].udoms * sizeof(unsigned long long int));
                            data_block->dims[i].ints = (char*) malloc(
                                    data_block->dims[i].udoms * sizeof(unsigned long long int));
                            break;
                        case 2 :
                            data_block->dims[i].offs = (char*) malloc(
                                    data_block->dims[i].udoms * sizeof(unsigned long long int));
                            break;
                        case 3 :
                            data_block->dims[i].offs = (char*) malloc(sizeof(unsigned long long int));
                            data_block->dims[i].ints = (char*) malloc(sizeof(unsigned long long int));
                            break;
                    }
                    break;
#endif
                case TYPE_DCOMPLEX :
                    //allocCount = sizeof(DCOMPLEX);
                    switch (data_block->dims[i].method) {
                        case 1 :
                            data_block->dims[i].sams = (long*) malloc(data_block->dims[i].udoms * sizeof(long));
                            data_block->dims[i].offs = (char*) malloc(data_block->dims[i].udoms * sizeof(DCOMPLEX));
                            data_block->dims[i].ints = (char*) malloc(data_block->dims[i].udoms * sizeof(DCOMPLEX));
                            break;
                        case 2 :
                            data_block->dims[i].offs = (char*) malloc(data_block->dims[i].udoms * sizeof(DCOMPLEX));
                            break;
                        case 3 :
                            data_block->dims[i].offs = (char*) malloc(sizeof(DCOMPLEX));
                            data_block->dims[i].ints = (char*) malloc(sizeof(DCOMPLEX));
                            break;
                    }
                    break;
                case TYPE_COMPLEX :
                    //allocCount = sizeof(COMPLEX);
                    switch (data_block->dims[i].method) {
                        case 1 :
                            data_block->dims[i].sams = (long*) malloc(data_block->dims[i].udoms * sizeof(long));
                            data_block->dims[i].offs = (char*) malloc(data_block->dims[i].udoms * sizeof(COMPLEX));
                            data_block->dims[i].ints = (char*) malloc(data_block->dims[i].udoms * sizeof(COMPLEX));
                            break;
                        case 2 :
                            data_block->dims[i].offs = (char*) malloc(data_block->dims[i].udoms * sizeof(COMPLEX));
                            break;
                        case 3 :
                            data_block->dims[i].offs = (char*) malloc(sizeof(COMPLEX));
                            data_block->dims[i].ints = (char*) malloc(sizeof(COMPLEX));
                            break;
                    }
                    break;
            }
        }
    }

    return 0;
}


int allocPutData(PUTDATA_BLOCK* putData)
{

// Main routine for allocating memory for 'PUT' Data

    unsigned int count;
    char* db = NULL;

//------------------------------------------------------------------------
// Allocate Memory for data

    if ((count = putData->count) == 0) return 1;   // Insufficient Data to Allocate!

    switch (putData->data_type) {
        case TYPE_CHAR :
            db = (char*) malloc(count * sizeof(char));
            break;
        case TYPE_SHORT :
            db = (char*) malloc(count * sizeof(short));
            break;
        case TYPE_INT :
            db = (char*) malloc(count * sizeof(int));
            break;
        case TYPE_LONG :
            db = (char*) malloc(count * sizeof(long));
            break;
        case TYPE_LONG64 :
            db = (char*) malloc(count * sizeof(long long int));
            break;
        case TYPE_FLOAT :
            db = (char*) malloc(count * sizeof(float));
            break;
        case TYPE_DOUBLE :
            db = (char*) malloc(count * sizeof(double));
            break;
        case TYPE_UNSIGNED_CHAR :
            db = (char*) malloc(count * sizeof(unsigned char));
            break;
        case TYPE_UNSIGNED_SHORT :
            db = (char*) malloc(count * sizeof(unsigned short));
            break;
        case TYPE_UNSIGNED_INT :
            db = (char*) malloc(count * sizeof(unsigned int));
            break;
        case TYPE_UNSIGNED_LONG :
            db = (char*) malloc(count * sizeof(unsigned long));
            break;
#ifndef __APPLE__
        case TYPE_UNSIGNED_LONG64 :
            db = (char*) malloc(count * sizeof(unsigned long long int));
            break;
#endif
        case TYPE_STRING :
            db = (char*) malloc(count * sizeof(char));
            break;
        case TYPE_DCOMPLEX :
            db = (char*) malloc(count * sizeof(DCOMPLEX));
            break;
        case TYPE_COMPLEX :
            db = (char*) malloc(count * sizeof(COMPLEX));
            break;
        case TYPE_COMPOUND:
            db = NULL;
            break;
        default:
            return (UNKNOWN_DATA_TYPE);
    }

    idamLog(LOG_DEBUG, "allocPutData :\n");
    idamLog(LOG_DEBUG, "rank      : %d\n", putData->rank);
    idamLog(LOG_DEBUG, "count     : %d\n", putData->count);
    idamLog(LOG_DEBUG, "data_type : %d\n", putData->data_type);
    idamLog(LOG_DEBUG, "data  != NULL: %d\n", db != NULL);

    if (db == NULL && putData->data_type != TYPE_COMPOUND) {
        idamLog(LOG_DEBUG, "allocPutData: Unable to Allocate Heap Memory for Data \n");
        return (ERROR_ALLOCATING_HEAP);
    }

    putData->data = db;

// Shape of data

    if (putData->rank > 1) putData->shape = (int*) malloc(putData->rank * sizeof(int));

// Name of data

    if (putData->blockNameLength > 0)
        putData->blockName = (char*) malloc((putData->blockNameLength + 1) * sizeof(char));
    else
        putData->blockName = NULL;

    return 0;
}

void addIdamPutDataBlockList(PUTDATA_BLOCK* putDataBlock, PUTDATA_BLOCK_LIST* putDataBlockList)
{
    if (putDataBlockList->putDataBlock == NULL ||
        putDataBlockList->blockCount + 1 >= putDataBlockList->blockListSize) {
        putDataBlockList->putDataBlock = (PUTDATA_BLOCK*) realloc((void*) putDataBlockList->putDataBlock,
                                                                  (putDataBlockList->blockListSize +
                                                                   GROWPUTDATABLOCKLIST) * sizeof(PUTDATA_BLOCK));
        putDataBlockList->blockListSize = putDataBlockList->blockListSize + GROWPUTDATABLOCKLIST;
    }
    putDataBlockList->putDataBlock[putDataBlockList->blockCount++] = *putDataBlock;
}

