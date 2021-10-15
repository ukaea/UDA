/*---------------------------------------------------------------
* UDA Naive Dimensional Data Compressor
*
* Input Arguments:    DIMS *        Dimensional Data
*
* Returns:        compressDim    1 if no Problems Found
*            DATA_BLOCK    Compressed Dimensional Data
*
*
* Notes:     If the dimensional data is regular it can be compressed into
*        three numbers: A starting value, a step value and the
*        number of data points. The first two of these are cast as
*        doubles to preserve the highest level of accuracy.
*--------------------------------------------------------------*/

#include "compressDim.h"

#include <cfloat>
#include <cmath>
#include <cstdlib>
#include <clientserver/udaTypes.h>

#include "udaErrors.h"

int compressDim(DIMS* ddim)
{
    double d0, ddif = 0, pddif = 0, precision, mddif;
    int ndata;
    int ndif = 1;

    float* fp = nullptr;
    double* dp = nullptr;
    int* ip = nullptr;
    long* lp = nullptr;
    long long int* llp = nullptr;
    short* sp = nullptr;
    char* cp = nullptr;
    unsigned int* uip = nullptr;
    unsigned long* ulp = nullptr;
    unsigned long long int* ullp = nullptr;
    unsigned short* usp = nullptr;
    unsigned char* ucp = nullptr;

    if (!ddim || !ddim->dim || ddim->compressed) {
        // No Data or Already Compressed or Functionality disabled!
        return 1;
    }

    ndata = ddim->dim_n;

    if (ndata == 1) return 1;   // Insufficient Data to Compress!

    switch (ddim->data_type) {
        case UDA_TYPE_CHAR:
            cp = ddim->dim;
            d0 = (double)cp[0];
            pddif = (double)cp[1] - d0;
            precision = FLT_EPSILON;
            mddif = ((double)cp[ndata - 1] - d0) / (double)(ndata - 1);
            break;
        case UDA_TYPE_SHORT:
            sp = (short*)ddim->dim;
            d0 = (double)sp[0];
            pddif = (double)sp[1] - d0;
            precision = FLT_EPSILON;
            mddif = ((double)sp[ndata - 1] - d0) / (double)(ndata - 1);
            break;
        case UDA_TYPE_INT:
            ip = (int*)ddim->dim;
            d0 = (double)ip[0];
            pddif = (double)ip[1] - d0;
            precision = FLT_EPSILON;
            mddif = ((double)ip[ndata - 1] - d0) / (double)(ndata - 1);
            break;
        case UDA_TYPE_LONG:
            lp = (long*)ddim->dim;
            d0 = (double)lp[0];
            pddif = (double)lp[1] - d0;
            precision = FLT_EPSILON;
            mddif = ((double)lp[ndata - 1] - d0) / (double)(ndata - 1);
            break;
        case UDA_TYPE_LONG64:
            llp = (long long int*)ddim->dim;
            d0 = (double)llp[0];
            pddif = (double)llp[1] - d0;
            precision = FLT_EPSILON;
            mddif = ((double)llp[ndata - 1] - d0) / (double)(ndata - 1);            // Use the System Precision MACRO
            break;
        case UDA_TYPE_FLOAT:
            fp = (float*)ddim->dim;
            d0 = (double)fp[0];
            pddif = (double)fp[1] - d0;
            precision = FLT_EPSILON;
            mddif = ((double)fp[ndata - 1] - d0) / (double)(ndata - 1);
            break;
        case UDA_TYPE_DOUBLE:
            dp = (double*)ddim->dim;
            d0 = dp[0];
            pddif = dp[1] - d0;
            precision = DBL_EPSILON;
            mddif = (dp[ndata - 1] - d0) / (double)(ndata - 1);
            break;

        case UDA_TYPE_UNSIGNED_CHAR:
            ucp = (unsigned char*)ddim->dim;
            d0 = (double)ucp[0];
            pddif = (double)ucp[1] - d0;
            precision = FLT_EPSILON;
            mddif = ((double)ucp[ndata - 1] - d0) / (double)(ndata - 1);
            break;
        case UDA_TYPE_UNSIGNED_SHORT:
            usp = (unsigned short*)ddim->dim;
            d0 = (double)usp[0];
            pddif = (double)usp[1] - d0;
            precision = FLT_EPSILON;
            mddif = ((double)usp[ndata - 1] - d0) / (double)(ndata - 1);
            break;
        case UDA_TYPE_UNSIGNED_INT:
            uip = (unsigned int*)ddim->dim;
            d0 = (double)uip[0];
            pddif = (double)uip[1] - d0;
            precision = FLT_EPSILON;
            mddif = ((double)uip[ndata - 1] - d0) / (double)(ndata - 1);
            break;
        case UDA_TYPE_UNSIGNED_LONG:
            ulp = (unsigned long*)ddim->dim;
            d0 = (double)ulp[0];
            pddif = (double)ulp[1] - d0;
            precision = FLT_EPSILON;
            mddif = ((double)ulp[ndata - 1] - d0) / (double)(ndata - 1);
            break;
        case UDA_TYPE_UNSIGNED_LONG64:
            ullp = (unsigned long long int*)ddim->dim;
            d0 = (double)ullp[0];
            pddif = (double)ullp[1] - d0;
            precision = FLT_EPSILON;
            mddif = ((double)ullp[ndata - 1] - d0) / (double)(ndata - 1);
            break;
        default:
            ddim->compressed = 0;
            return 1;
    }

    for (int i = 1; i < ndata; i++) {
        switch (ddim->data_type) {
            case UDA_TYPE_CHAR:
                ddif = (double)cp[i] - (double)cp[i - 1];
                break;
            case UDA_TYPE_SHORT:
                ddif = (double)sp[i] - (double)sp[i - 1];
                break;
            case UDA_TYPE_INT:
                ddif = (double)ip[i] - (double)ip[i - 1];
                break;
            case UDA_TYPE_LONG:
                ddif = (double)lp[i] - (double)lp[i - 1];
                break;
            case UDA_TYPE_LONG64:
                ddif = (double)llp[i] - (double)llp[i - 1];
                break;
            case UDA_TYPE_FLOAT:
                ddif = (double)fp[i] - (double)fp[i - 1];
                break;
            case UDA_TYPE_DOUBLE:
                ddif = (double)dp[i] - (double)dp[i - 1];
                break;
            case UDA_TYPE_UNSIGNED_CHAR:
                ddif = (double)ucp[i] - (double)ucp[i - 1];
                break;
            case UDA_TYPE_UNSIGNED_SHORT:
                ddif = (double)usp[i] - (double)usp[i - 1];
                break;
            case UDA_TYPE_UNSIGNED_INT:
                ddif = (double)uip[i] - (double)uip[i - 1];
                break;
            case UDA_TYPE_UNSIGNED_LONG:
                ddif = (double)ulp[i] - (double)ulp[i - 1];
                break;
            case UDA_TYPE_UNSIGNED_LONG64:
                ddif = (double)ullp[i] - (double)ullp[i - 1];
                break;
        }

        if (fabs(ddif - pddif) > precision) break;        // Differences are Not Constant!
        pddif = ddif;
        ndif++;
    }

    if (ndif != ndata) {
        ddim->compressed = 0;
        return 1;        // Data not regular
    }

    ddim->compressed = 1;
    ddim->dim0 = d0;
    ddim->diff = mddif;    // Average difference
    ddim->method = 0;    // Default Decompression Method

    return 0;
}


/*---------------------------------------------------------------
* IDAM Dimensional Data Uncompressor
*
* Input Arguments:    DIMS *        Dimensional Data
*
* Returns:        uncompressDim    0 if no Problems Found
*                    Error Code if a Problem Occured
*
*            DIMS* ->dim    Un-Compressed Dimensional Data
*            DIMS* ->compressed    Unchanged (necessary)
*
* Note: XML based data correction also uses the compression models: New models
* must also have corrections applied.
*--------------------------------------------------------------*/

int uncompressDim(DIMS* ddim)
{
    if (!ddim || ddim->compressed == 0) {
        return 0;    // Nothing to Uncompress!
    }
    if (ddim->dim_n == 0) {
        return 0;    // Nothing to Uncompress!
    }

    auto ndata = (size_t)ddim->dim_n;
    double d0 = ddim->dim0;        // Default Compression Method
    double diff = ddim->diff;

    int count = 0;

    switch (ddim->data_type) {

        case UDA_TYPE_CHAR: {
            if (ddim->dim == nullptr) {
                if ((ddim->dim = (char*)malloc(ndata * sizeof(char))) == nullptr) {
                    return UNCOMPRESS_ALLOCATING_HEAP;
                }
            }
            char* cp = ddim->dim;

            switch (ddim->method) {
                case 0:
                    cp[0] = (char)d0;
                    for (unsigned int i = 1; i < ndata; i++) {
                        cp[i] = cp[i - 1] + (char)diff;
                    }
                    break;
                case 1:
                    count = 0;
                    for (unsigned int i = 0; i < ddim->udoms; i++) {
                        for (int j = 0; j < *(ddim->sams + i); j++) {
                            cp[count++] = *(ddim->offs + i) + (char)j * *(ddim->ints + i);
                        }
                    }
                    break;
                case 2:
                    for (unsigned int i = 0; i < ddim->udoms; i++)cp[i] = *(ddim->offs + i);
                    break;
                case 3:
                    for (unsigned int i = 0; i < ddim->udoms; i++)cp[i] = *(ddim->offs) + (char)i * *(ddim->ints);
                    break;
            }
            break;
        }

        case UDA_TYPE_SHORT: {
            if (ddim->dim == nullptr) {
                if ((ddim->dim = (char*)malloc(ndata * sizeof(short))) == nullptr) {
                    return UNCOMPRESS_ALLOCATING_HEAP;
                }
            }
            auto sp = (short*)ddim->dim;

            switch (ddim->method) {
                case 0:
                    sp[0] = (short)d0;
                    for (unsigned int i = 1; i < ndata; i++) {
                        sp[i] = sp[i - 1] + (short)diff;
                    }
                    break;
                case 1:
                    count = 0;
                    for (unsigned int i = 0; i < ddim->udoms; i++) {
                        for (int j = 0; j < *(ddim->sams + i); j++) {
                            sp[count++] = *((short*)ddim->offs + i) + (short)j * *((short*)ddim->ints + i);
                        }
                    }
                    break;
                case 2:
                    for (unsigned int i = 0; i < ddim->udoms; i++)sp[i] = *((short*)ddim->offs + i);
                    break;
                case 3:
                    for (unsigned int i = 0; i < ddim->udoms; i++)
                        sp[i] = *((short*)ddim->offs) + (short)i * *((short*)ddim->ints);
                    break;
            }
            break;
        }

        case UDA_TYPE_INT: {
            if (ddim->dim == nullptr) {
                if ((ddim->dim = (char*)malloc(ndata * sizeof(int))) == nullptr) {
                    return UNCOMPRESS_ALLOCATING_HEAP;
                }
            }
            auto ip = (int*)ddim->dim;

            switch (ddim->method) {
                case 0:
                    ip[0] = (int)d0;
                    for (unsigned int i = 1; i < ndata; i++) {
                        ip[i] = ip[i - 1] + (int)diff;
                    }
                    break;
                case 1:
                    count = 0;
                    for (unsigned int i = 0; i < ddim->udoms; i++) {
                        for (int j = 0; j < *(ddim->sams + i); j++) {
                            ip[count++] = *((int*)ddim->offs + i) + j * *((int*)ddim->ints + i);
                        }
                    }
                    break;
                case 2:
                    for (unsigned int i = 0; i < ddim->udoms; i++)ip[i] = *((int*)ddim->offs + i);
                    break;
                case 3:
                    for (unsigned int i = 0; i < ddim->udoms; i++)ip[i] = *((int*)ddim->offs) + i * *((int*)ddim->ints);
                    break;
            }
            break;
        }

        case UDA_TYPE_LONG: {
            if (ddim->dim == nullptr) {
                if ((ddim->dim = (char*)malloc(ndata * sizeof(long))) == nullptr) {
                    return UNCOMPRESS_ALLOCATING_HEAP;
                }
            }
            auto lp = (long*)ddim->dim;
            switch (ddim->method) {
                case 0:
                    lp[0] = (long)d0;
                    for (unsigned int i = 1; i < ndata; i++) {
                        lp[i] = lp[i - 1] + (long)diff;
                    }
                    break;
                case 1:
                    count = 0;
                    for (unsigned int i = 0; i < ddim->udoms; i++) {
                        for (int j = 0; j < *(ddim->sams + i); j++) {
                            lp[count++] = *((long*)ddim->offs + i) + (long)j * *((long*)ddim->ints + i);
                        }
                    }
                    break;
                case 2:
                    for (unsigned int i = 0; i < ddim->udoms; i++)lp[i] = *((long*)ddim->offs + i);
                    break;
                case 3:
                    for (unsigned int i = 0; i < ddim->udoms; i++)
                        lp[i] = *((long*)ddim->offs) + (long)i * *((long*)ddim->ints);
                    break;
            }
            break;
        }

        case UDA_TYPE_LONG64: {
            if (ddim->dim == nullptr) {
                if ((ddim->dim = (char*)malloc(ndata * sizeof(long long int))) == nullptr) {
                    return UNCOMPRESS_ALLOCATING_HEAP;
                }
            }
            auto lp = (long long int*)ddim->dim;
            switch (ddim->method) {
                case 0:
                    lp[0] = (long long int)d0;
                    for (unsigned int i = 1; i < ndata; i++) {
                        lp[i] = lp[i - 1] + (long long int)diff;
                    }
                    break;
                case 1:
                    count = 0;
                    for (unsigned int i = 0; i < ddim->udoms; i++) {
                        for (int j = 0; j < *(ddim->sams + i); j++) {
                            lp[count++] = *((long long int*)ddim->offs + i) +
                                          (long long int)j * *((long long int*)ddim->ints + i);
                        }
                    }
                    break;
                case 2:
                    for (unsigned int i = 0; i < ddim->udoms; i++)lp[i] = *((long long int*)ddim->offs + i);
                    break;
                case 3:
                    for (unsigned int i = 0; i < ddim->udoms; i++)
                        lp[i] = *((long long int*)ddim->offs) + (long long int)i * *((long long int*)ddim->ints);
                    break;
            }
            break;
        }

        case UDA_TYPE_FLOAT: {
            if ((ddim->dim = (char*)malloc(ndata * sizeof(float))) == nullptr) {
                return UNCOMPRESS_ALLOCATING_HEAP;
            }
            auto fp = (float*)ddim->dim;

            switch (ddim->method) {
                case 0:
                    fp[0] = (float)d0;
                    for (unsigned int i = 1; i < ndata; i++) {
                        fp[i] = fp[i - 1] + (float)diff;
                    }
                    break;
                case 1:
                    count = 0;
                    for (unsigned int i = 0; i < ddim->udoms; i++) {
                        for (int j = 0; j < *(ddim->sams + i); j++) {
                            fp[count++] = *((float*)ddim->offs + i) + (float)j * *((float*)ddim->ints + i);
                        }
                    }
                    break;
                case 2:
                    for (unsigned int i = 0; i < ddim->udoms; i++)fp[i] = *((float*)ddim->offs + i);
                    break;
                case 3:
                    for (unsigned int i = 0; i < ddim->udoms; i++)
                        fp[i] = *((float*)ddim->offs) + (float)i * *((float*)ddim->ints);
                    break;
            }
            break;
        }

        case UDA_TYPE_DOUBLE: {
            if (ddim->dim == nullptr) {
                if ((ddim->dim = (char*)malloc(ndata * sizeof(double))) == nullptr) {
                    return UNCOMPRESS_ALLOCATING_HEAP;
                }
            }
            auto dp = (double*)ddim->dim;

            switch (ddim->method) {
                case 0:
                    dp[0] = (double)d0;
                    for (unsigned int i = 1; i < ndata; i++) {
                        dp[i] = dp[i - 1] + diff;
                    }
                    break;
                case 1:
                    count = 0;
                    for (unsigned int i = 0; i < ddim->udoms; i++) {
                        for (int j = 0; j < *(ddim->sams + i); j++) {
                            dp[count++] = *((double*)ddim->offs + i) + (double)j * *((double*)ddim->ints + i);
                        }
                    }
                    break;
                case 2:
                    for (unsigned int i = 0; i < ddim->udoms; i++)dp[i] = *((double*)ddim->offs + i);
                    break;
                case 3:
                    for (unsigned int i = 0; i < ddim->udoms; i++)
                        dp[i] = *((double*)ddim->offs) + (double)i * *((double*)ddim->ints);
                    break;
            }
            break;
        }

        case UDA_TYPE_UNSIGNED_CHAR: {
            if (ddim->dim == nullptr) {
                if ((ddim->dim = (char*)malloc(ndata * sizeof(unsigned char))) == nullptr) {
                    return UNCOMPRESS_ALLOCATING_HEAP;
                }
            }
            auto ucp = (unsigned char*)ddim->dim;

            switch (ddim->method) {
                case 0:
                    ucp[0] = (unsigned char)d0;
                    for (unsigned int i = 1; i < ndata; i++) {
                        ucp[i] = ucp[i - 1] + (char)diff;
                    }
                    break;
                case 1:
                    count = 0;
                    for (unsigned int i = 0; i < ddim->udoms; i++) {
                        for (int j = 0; j < *(ddim->sams + i); j++) {
                            ucp[count++] = *((unsigned char*)ddim->offs + i) +
                                           (unsigned char)j * *((unsigned char*)ddim->ints + i);
                        }
                    }
                    break;
                case 2:
                    for (unsigned int i = 0; i < ddim->udoms; i++)ucp[i] = *((unsigned char*)ddim->offs + i);
                    break;
                case 3:
                    for (unsigned int i = 0; i < ddim->udoms; i++)
                        ucp[i] = *((unsigned char*)ddim->offs) + (unsigned char)i * *((unsigned char*)ddim->ints);
                    break;
            }
            break;
        }

        case UDA_TYPE_UNSIGNED_SHORT: {
            if (ddim->dim == nullptr) {
                if ((ddim->dim = (char*)malloc(ndata * sizeof(unsigned short))) == nullptr) {
                    return UNCOMPRESS_ALLOCATING_HEAP;
                }
            }
            auto usp = (unsigned short*)ddim->dim;

            switch (ddim->method) {
                case 0:
                    usp[0] = (unsigned short)d0;
                    for (unsigned int i = 1; i < ndata; i++) {
                        usp[i] = usp[i - 1] + (unsigned short)diff;
                    }
                    break;
                case 1:
                    count = 0;
                    for (unsigned int i = 0; i < ddim->udoms; i++) {
                        for (int j = 0; j < *(ddim->sams + i); j++) {
                            usp[count++] = *((unsigned short*)ddim->offs + i) +
                                           (unsigned short)j * *((unsigned short*)ddim->ints + i);
                        }
                    }
                    break;
                case 2:
                    for (unsigned int i = 0; i < ddim->udoms; i++)usp[i] = *((unsigned short*)ddim->offs + i);
                    break;
                case 3:
                    for (unsigned int i = 0; i < ddim->udoms; i++)
                        usp[i] = *((unsigned short*)ddim->offs) + (unsigned short)i * *((unsigned short*)ddim->ints);
                    break;
            }
            break;
        }

        case UDA_TYPE_UNSIGNED_INT: {
            if (ddim->dim == nullptr) {
                if ((ddim->dim = (char*)malloc(ndata * sizeof(unsigned int))) == nullptr) {
                    return UNCOMPRESS_ALLOCATING_HEAP;
                }
            }
            auto uip = (unsigned int*)ddim->dim;

            switch (ddim->method) {
                case 0:
                    uip[0] = (unsigned int)d0;
                    for (unsigned int i = 1; i < ndata; i++) {
                        uip[i] = uip[i - 1] + (unsigned int)diff;
                    }
                    break;
                case 1:
                    count = 0;
                    for (unsigned int i = 0; i < ddim->udoms; i++) {
                        for (int j = 0; j < *(ddim->sams + i); j++) {
                            uip[count++] = *((unsigned int*)ddim->offs + i) +
                                           (unsigned int)j * *((unsigned int*)ddim->ints + i);
                        }
                    }
                    break;
                case 2:
                    for (unsigned int i = 0; i < ddim->udoms; i++)uip[i] = *((unsigned int*)ddim->offs + i);
                    break;
                case 3:
                    for (unsigned int i = 0; i < ddim->udoms; i++)
                        uip[i] = *((unsigned int*)ddim->offs) + (unsigned int)i * *((unsigned int*)ddim->ints);
                    break;
            }
            break;
        }

        case UDA_TYPE_UNSIGNED_LONG: {
            if (ddim->dim == nullptr) {
                if ((ddim->dim = (char*)malloc(ndata * sizeof(unsigned long))) == nullptr) {
                    return UNCOMPRESS_ALLOCATING_HEAP;
                }
            }
            auto ulp = (unsigned long*)ddim->dim;
            switch (ddim->method) {
                case 0:
                    ulp[0] = (unsigned long)d0;
                    for (unsigned int i = 1; i < ndata; i++) {
                        ulp[i] = ulp[i - 1] + (unsigned long)diff;
                    }
                    break;
                case 1:
                    count = 0;
                    for (unsigned int i = 0; i < ddim->udoms; i++) {
                        for (int j = 0; j < *(ddim->sams + i); j++) {
                            ulp[count++] = *((unsigned long*)ddim->offs + i) +
                                           (unsigned long)j * *((unsigned long*)ddim->ints + i);
                        }
                    }
                    break;
                case 2:
                    for (unsigned int i = 0; i < ddim->udoms; i++)ulp[i] = *((unsigned long*)ddim->offs + i);
                    break;
                case 3:
                    for (unsigned int i = 0; i < ddim->udoms; i++)
                        ulp[i] = *((unsigned long*)ddim->offs) + (unsigned long)i * *((unsigned long*)ddim->ints);
                    break;
            }
            break;
        }

        case UDA_TYPE_UNSIGNED_LONG64: {
            if (ddim->dim == nullptr) {
                if ((ddim->dim = (char*)malloc(ndata * sizeof(unsigned long long int))) == nullptr) {
                    return UNCOMPRESS_ALLOCATING_HEAP;
                }
            }
            auto ulp = (unsigned long long int*)ddim->dim;
            switch (ddim->method) {
                case 0:
                    ulp[0] = (unsigned long long int)d0;
                    for (unsigned int i = 1; i < ndata; i++)ulp[i] = ulp[i - 1] + (unsigned long long int)diff;
                    break;
                case 1:
                    count = 0;
                    for (unsigned int i = 0; i < ddim->udoms; i++) {
                        for (int j = 0; j < *(ddim->sams + i); j++) {
                            ulp[count++] = *((unsigned long long int*)ddim->offs + i) +
                                           (unsigned long long int)j * *((unsigned long long int*)ddim->ints + i);
                        }
                    }
                    break;
                case 2:
                    for (unsigned int i = 0; i < ddim->udoms; i++)ulp[i] = *((unsigned long long int*)ddim->offs + i);
                    break;
                case 3:
                    for (unsigned int i = 0; i < ddim->udoms; i++)
                        ulp[i] = *((unsigned long long int*)ddim->offs) + (unsigned long long int)i *
                                                                          *((unsigned long long int*)ddim->ints);
                    break;
            }
            break;
        }

        default:
            return UNKNOWN_DATA_TYPE;
    }

    return 0;
}
