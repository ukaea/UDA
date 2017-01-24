//! $LastChangedRevision: 353 $
//! $LastChangedDate: 2013-11-18 15:32:28 +0000 (Mon, 18 Nov 2013) $
//! $LastChangedBy: dgm $
//! $HeadURL: https://fussvn.fusion.culham.ukaea.org.uk/svnroot/IDAM/development/source/clientserver/compressDim.c $

/*---------------------------------------------------------------
* IDAM Naive Dimensional Data Compressor
*
* Input Arguments:	DIMS *		Dimensional Data
*
* Returns:		compressDim	1 if no Problems Found
*			DATA_BLOCK	Compressed Dimensional Data
*
*
* Notes: 	If the dimensional data is regular it can be compressed into
*		three numbers: A starting value, a step value and the
*		number of data points. The first two of these are cast as
*		doubles to preserve the highest level of accuracy.
*
* Change History
* 1.0  	06Jul2005	D.G.Muir
* 1.1	28Nov2005	D.G.Muir	Correction of Bug relating to returned value
*					Removal of Heap Free - Done elsewhere
* 29Oct2008	dgm	Added unsigned types: short, long and char
* 09Jun2009	dgm	All data types now added: No compression for COMPLEX types
* 07Jul2009	dgm	Added compiler option ULONG64_OK to disable unsigned long long int: There is a gcc/ld bug with message
*			hidden symbol `__fixunssfdi' in /usr/lib/gcc/i386-redhat-linux/4.1.1/libgcc.a(_fixunssfdi.o) is referenced by DSO
*			/usr/bin/ld: final link failed: Nonrepresentable section on output
*			This problem is fixed by removing references to unsigned long long int
//			Also added option LONG64_OK for long long int
// 08Feb2010	dgm	Added compiler option CASTLONG64_OK to switch out __fixunssfdi bug (float cast from long long)
// 09May2011	dgm	Removed compiler options LONG64_OK & ULONG64_OK
// 12Dec2012	dgm	Tidied up precision check
*--------------------------------------------------------------*/

#include "compressDim.h"

#include <float.h>
#include <math.h>
#include <stdlib.h>
#include <include/idamtypes.h>
#include "idamErrors.h"

int compressDim(DIMS* ddim)
{

    double d0, ddif, pddif = 0, precision, mddif;
    int ndata;
    int i, ndif = 1;

    float* fp;
    double* dp;
    int* ip;
    long* lp;
    long long int* llp;
    short* sp;
    char* cp;
    unsigned int* uip;
    unsigned long* ulp;
#ifndef __APPLE__
    unsigned long long int* ullp;
#endif
    unsigned short* usp;
    unsigned char* ucp;

#ifdef COMPRESSOFF
    return(1);
#endif

    if (!ddim || !ddim->dim || ddim->compressed)
        return (1);   // No Data or Already Compressed or Functionality disabled!

    ndata = (int) ddim->dim_n;

    if (ndata == 1) return (1);   // Insufficient Data to Compress!

    switch (ddim->data_type) {
        case TYPE_CHAR :
            cp = ddim->dim;
            d0 = (double) cp[0];
            pddif = (double) cp[1] - d0;
            precision = FLT_EPSILON;
            mddif = ((double) cp[ndata - 1] - d0) / (double) (ndata - 1);
            break;
        case TYPE_SHORT :
            sp = (short*) ddim->dim;
            d0 = (double) sp[0];
            pddif = (double) sp[1] - d0;
            precision = FLT_EPSILON;
            mddif = ((double) sp[ndata - 1] - d0) / (double) (ndata - 1);
            break;
        case TYPE_INT :
            ip = (int*) ddim->dim;
            d0 = (double) ip[0];
            pddif = (double) ip[1] - d0;
            precision = FLT_EPSILON;
            mddif = ((double) ip[ndata - 1] - d0) / (double) (ndata - 1);
            break;
        case TYPE_LONG :
            lp = (long*) ddim->dim;
            d0 = (double) lp[0];
            pddif = (double) lp[1] - d0;
            precision = FLT_EPSILON;
            mddif = ((double) lp[ndata - 1] - d0) / (double) (ndata - 1);
            break;
        case TYPE_LONG64 :
            llp = (long long int*) ddim->dim;
            d0 = (double) llp[0];
            pddif = (double) llp[1] - d0;
            precision = FLT_EPSILON;
            mddif = ((double) llp[ndata - 1] - d0) / (double) (ndata - 1);            // Use the System Precision MACRO
            break;
        case TYPE_FLOAT :
            fp = (float*) ddim->dim;
            d0 = (double) fp[0];
            pddif = (double) fp[1] - d0;
            precision = FLT_EPSILON;
            mddif = ((double) fp[ndata - 1] - d0) / (double) (ndata - 1);
            break;
        case TYPE_DOUBLE :
            dp = (double*) ddim->dim;
            d0 = dp[0];
            pddif = dp[1] - d0;
            precision = DBL_EPSILON;
            mddif = (dp[ndata - 1] - d0) / (double) (ndata - 1);
            break;

        case TYPE_UNSIGNED_CHAR :
            ucp = (unsigned char*) ddim->dim;
            d0 = (double) ucp[0];
            pddif = (double) ucp[1] - d0;
            precision = FLT_EPSILON;
            mddif = ((double) ucp[ndata - 1] - d0) / (double) (ndata - 1);
            break;
        case TYPE_UNSIGNED_SHORT :
            usp = (unsigned short*) ddim->dim;
            d0 = (double) usp[0];
            pddif = (double) usp[1] - d0;
            precision = FLT_EPSILON;
            mddif = ((double) usp[ndata - 1] - d0) / (double) (ndata - 1);
            break;
        case TYPE_UNSIGNED_INT :
            uip = (unsigned int*) ddim->dim;
            d0 = (double) uip[0];
            pddif = (double) uip[1] - d0;
            precision = FLT_EPSILON;
            mddif = ((double) uip[ndata - 1] - d0) / (double) (ndata - 1);
            break;
        case TYPE_UNSIGNED_LONG :
            ulp = (unsigned long*) ddim->dim;
            d0 = (double) ulp[0];
            pddif = (double) ulp[1] - d0;
            precision = FLT_EPSILON;
            mddif = ((double) ulp[ndata - 1] - d0) / (double) (ndata - 1);
            break;
#ifndef __APPLE__
        case TYPE_UNSIGNED_LONG64 :
            ullp = (unsigned long long int*) ddim->dim;
            d0 = (double) ullp[0];
            pddif = (double) ullp[1] - d0;
            precision = FLT_EPSILON;
            mddif = ((double) ullp[ndata - 1] - d0) / (double) (ndata - 1);
            break;
#endif
        default:
            ddim->compressed = 0;
            return (1);
    }

    for (i = 1; i < ndata; i++) {
        switch (ddim->data_type) {
            case TYPE_CHAR :
                ddif = (double) cp[i] - (double) cp[i - 1];
                break;
            case TYPE_SHORT :
                ddif = (double) sp[i] - (double) sp[i - 1];
                break;
            case TYPE_INT :
                ddif = (double) ip[i] - (double) ip[i - 1];
                break;
            case TYPE_LONG :
                ddif = (double) lp[i] - (double) lp[i - 1];
                break;
            case TYPE_LONG64 :
                ddif = (double) llp[i] - (double) llp[i - 1];
                break;
            case TYPE_FLOAT :
                ddif = (double) fp[i] - (double) fp[i - 1];
                break;
            case TYPE_DOUBLE :
                ddif = (double) dp[i] - (double) dp[i - 1];
                break;
            case TYPE_UNSIGNED_CHAR :
                ddif = (double) ucp[i] - (double) ucp[i - 1];
                break;
            case TYPE_UNSIGNED_SHORT :
                ddif = (double) usp[i] - (double) usp[i - 1];
                break;
            case TYPE_UNSIGNED_INT :
                ddif = (double) uip[i] - (double) uip[i - 1];
                break;
            case TYPE_UNSIGNED_LONG :
                ddif = (double) ulp[i] - (double) ulp[i - 1];
                break;
#ifndef __APPLE__
            case TYPE_UNSIGNED_LONG64 :
                ddif = (double) ullp[i] - (double) ullp[i - 1];
                break;
#endif
        }

        if (fabs(ddif - pddif) > precision) break;        // Differences are Not Constant!
        pddif = ddif;
        ndif++;
    }

    if (ndif != ndata) {
        ddim->compressed = 0;
        return (1);        // Data not regular
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
* Input Arguments:	DIMS *		Dimensional Data
*
* Returns:		uncompressDim	0 if no Problems Found
*					Error Code if a Problem Occured
*
*			DIMS* ->dim	Un-Compressed Dimensional Data
*			DIMS* ->compressed	Unchanged (necessary)
*
* Note: XML based data correction also uses the compression models: New models
* must also have corrections applied.
*
* Revision 1.0  06Jul2005	D.G.Muir
* 13May2011 dgm		return if zero length dimension
*--------------------------------------------------------------*/

int uncompressDim(DIMS* ddim)
{

    double d0, diff;
    int ndata;
    int i, j, count;

    float* fp;
    double* dp;
    int* ip;
    long* lp;
    short* sp;
    char* cp;
    unsigned int* uip;
    unsigned long* ulp;
    unsigned short* usp;
    unsigned char* ucp;

    if (!ddim || ddim->compressed == 0) return (0);    // Nothing to Uncompress!
    if (ddim->dim_n == 0) return (0);    // Nothing to Uncompress!

    ndata = ddim->dim_n;
    d0 = ddim->dim0;        // Default Compression Method
    diff = ddim->diff;

    switch (ddim->data_type) {

        case TYPE_CHAR :
            if (ddim->dim == NULL) {
                if ((ddim->dim = (char*) malloc(ndata * sizeof(char))) == NULL) {
                    return UNCOMPRESS_ALLOCATING_HEAP;
                }
            }
            cp = ddim->dim;

            switch (ddim->method) {
                case 0:
                    cp[0] = (char) d0;
                    for (i = 1; i < ndata; i++)cp[i] = cp[i - 1] + (char) diff;
                    break;
                case 1:
                    count = 0;
                    for (i = 0; i < ddim->udoms; i++) {
                        for (j = 0; j < *((long*) ddim->sams + i); j++) {
                            cp[count++] = *((char*) ddim->offs + i) + (char) j * *((char*) ddim->ints + i);
                        }
                    }
                    break;
                case 2:
                    for (i = 0; i < ddim->udoms; i++)cp[i] = *((char*) ddim->offs + i);
                    break;
                case 3:
                    for (i = 0; i < ddim->udoms; i++)cp[i] = *((char*) ddim->offs) + (char) i * *((char*) ddim->ints);
                    break;
            }
            break;

        case TYPE_SHORT :
            if (ddim->dim == NULL) {
                if ((ddim->dim = (char*) malloc(ndata * sizeof(short))) == NULL) {
                    return UNCOMPRESS_ALLOCATING_HEAP;
                }
            }
            sp = (short*) ddim->dim;

            switch (ddim->method) {
                case 0:
                    sp[0] = (short) d0;
                    for (i = 1; i < ndata; i++)sp[i] = sp[i - 1] + (short) diff;
                    break;
                case 1:
                    count = 0;
                    for (i = 0; i < ddim->udoms; i++) {
                        for (j = 0; j < *((long*) ddim->sams + i); j++) {
                            sp[count++] = *((short*) ddim->offs + i) + (short) j * *((short*) ddim->ints + i);
                        }
                    }
                    break;
                case 2:
                    for (i = 0; i < ddim->udoms; i++)sp[i] = *((short*) ddim->offs + i);
                    break;
                case 3:
                    for (i = 0; i < ddim->udoms; i++)
                        sp[i] = *((short*) ddim->offs) + (short) i * *((short*) ddim->ints);
                    break;
            }
            break;

        case TYPE_INT :
            if (ddim->dim == NULL) {
                if ((ddim->dim = (char*) malloc(ndata * sizeof(int))) == NULL) {
                    return UNCOMPRESS_ALLOCATING_HEAP;
                }
            }
            ip = (int*) ddim->dim;

            switch (ddim->method) {
                case 0:
                    ip[0] = (int) d0;
                    for (i = 1; i < ndata; i++)ip[i] = ip[i - 1] + (int) diff;
                    break;
                case 1:
                    count = 0;
                    for (i = 0; i < ddim->udoms; i++) {
                        for (j = 0; j < *((long*) ddim->sams + i); j++) {
                            ip[count++] = *((int*) ddim->offs + i) + (int) j * *((int*) ddim->ints + i);
                        }
                    }
                    break;
                case 2:
                    for (i = 0; i < ddim->udoms; i++)ip[i] = *((int*) ddim->offs + i);
                    break;
                case 3:
                    for (i = 0; i < ddim->udoms; i++)ip[i] = *((int*) ddim->offs) + (int) i * *((int*) ddim->ints);
                    break;
            }
            break;

        case TYPE_LONG :
            if (ddim->dim == NULL) {
                if ((ddim->dim = (char*) malloc(ndata * sizeof(long))) == NULL) {
                    return UNCOMPRESS_ALLOCATING_HEAP;
                }
            }
            lp = (long*) ddim->dim;
            switch (ddim->method) {
                case 0:
                    lp[0] = (long) d0;
                    for (i = 1; i < ndata; i++)lp[i] = lp[i - 1] + (long) diff;
                    break;
                case 1:
                    count = 0;
                    for (i = 0; i < ddim->udoms; i++) {
                        for (j = 0; j < *((long*) ddim->sams + i); j++) {
                            lp[count++] = *((long*) ddim->offs + i) + (long) j * *((long*) ddim->ints + i);
                        }
                    }
                    break;
                case 2:
                    for (i = 0; i < ddim->udoms; i++)lp[i] = *((long*) ddim->offs + i);
                    break;
                case 3:
                    for (i = 0; i < ddim->udoms; i++)lp[i] = *((long*) ddim->offs) + (long) i * *((long*) ddim->ints);
                    break;
            }
            break;
        case TYPE_LONG64 : {
            long long int* lp;
            if (ddim->dim == NULL) {
                if ((ddim->dim = (char*) malloc(ndata * sizeof(long long int))) == NULL) {
                    return UNCOMPRESS_ALLOCATING_HEAP;
                }
            }
            lp = (long long int*) ddim->dim;
            switch (ddim->method) {
                case 0:
                    lp[0] = (long long int) d0;
                    for (i = 1; i < ndata; i++)lp[i] = lp[i - 1] + (long long int) diff;
                    break;
                case 1:
                    count = 0;
                    for (i = 0; i < ddim->udoms; i++) {
                        for (j = 0; j < *((long long int*) ddim->sams + i); j++) {
                            lp[count++] = *((long long int*) ddim->offs + i) +
                                          (long long int) j * *((long long int*) ddim->ints + i);
                        }
                    }
                    break;
                case 2:
                    for (i = 0; i < ddim->udoms; i++)lp[i] = *((long long int*) ddim->offs + i);
                    break;
                case 3:
                    for (i = 0; i < ddim->udoms; i++)
                        lp[i] = *((long long int*) ddim->offs) + (long long int) i * *((long long int*) ddim->ints);
                    break;
            }
            break;
        }
        case TYPE_FLOAT :
            if ((ddim->dim = (char*) malloc(ndata * sizeof(float))) == NULL) {
                return UNCOMPRESS_ALLOCATING_HEAP;
            }
            fp = (float*) ddim->dim;

            switch (ddim->method) {
                case 0:
                    fp[0] = (float) d0;
                    for (i = 1; i < ndata; i++)fp[i] = fp[i - 1] + (float) diff;
                    break;
                case 1:
                    count = 0;
                    for (i = 0; i < ddim->udoms; i++) {
                        for (j = 0; j < *((long*) ddim->sams + i); j++) {
                            fp[count++] = *((float*) ddim->offs + i) + (float) j * *((float*) ddim->ints + i);
                        }
                    }
                    break;
                case 2:
                    for (i = 0; i < ddim->udoms; i++)fp[i] = *((float*) ddim->offs + i);
                    break;
                case 3:
                    for (i = 0; i < ddim->udoms; i++)
                        fp[i] = *((float*) ddim->offs) + (float) i * *((float*) ddim->ints);
                    break;
            }
            break;

        case TYPE_DOUBLE :
            if (ddim->dim == NULL) {
                if ((ddim->dim = (char*) malloc(ndata * sizeof(double))) == NULL) {
                    return UNCOMPRESS_ALLOCATING_HEAP;
                }
            }
            dp = (double*) ddim->dim;

            switch (ddim->method) {
                case 0:
                    dp[0] = (double) d0;
                    for (i = 1; i < ndata; i++)dp[i] = dp[i - 1] + (double) diff;
                    break;
                case 1:
                    count = 0;
                    for (i = 0; i < ddim->udoms; i++) {
                        for (j = 0; j < *((long*) ddim->sams + i); j++) {
                            dp[count++] = *((double*) ddim->offs + i) + (double) j * *((double*) ddim->ints + i);
                        }
                    }
                    break;
                case 2:
                    for (i = 0; i < ddim->udoms; i++)dp[i] = *((double*) ddim->offs + i);
                    break;
                case 3:
                    for (i = 0; i < ddim->udoms; i++)
                        dp[i] = *((double*) ddim->offs) + (double) i * *((double*) ddim->ints);
                    break;
            }
            break;

        case TYPE_UNSIGNED_CHAR :
            if (ddim->dim == NULL) {
                if ((ddim->dim = (char*) malloc(ndata * sizeof(unsigned char))) == NULL) {
                    return UNCOMPRESS_ALLOCATING_HEAP;
                }
            }
            ucp = (unsigned char*) ddim->dim;

            switch (ddim->method) {
                case 0:
                    ucp[0] = (unsigned char) d0;
                    for (i = 1; i < ndata; i++)ucp[i] = ucp[i - 1] + (char) diff;
                    break;
                case 1:
                    count = 0;
                    for (i = 0; i < ddim->udoms; i++) {
                        for (j = 0; j < *((long*) ddim->sams + i); j++) {
                            ucp[count++] = *((unsigned char*) ddim->offs + i) +
                                           (unsigned char) j * *((unsigned char*) ddim->ints + i);
                        }
                    }
                    break;
                case 2:
                    for (i = 0; i < ddim->udoms; i++)ucp[i] = *((unsigned char*) ddim->offs + i);
                    break;
                case 3:
                    for (i = 0; i < ddim->udoms; i++)
                        ucp[i] = *((unsigned char*) ddim->offs) + (unsigned char) i * *((unsigned char*) ddim->ints);
                    break;
            }
            break;

        case TYPE_UNSIGNED_SHORT :
            if (ddim->dim == NULL) {
                if ((ddim->dim = (char*) malloc(ndata * sizeof(unsigned short))) == NULL) {
                    return UNCOMPRESS_ALLOCATING_HEAP;
                }
            }
            usp = (unsigned short*) ddim->dim;

            switch (ddim->method) {
                case 0:
                    usp[0] = (unsigned short) d0;
                    for (i = 1; i < ndata; i++)usp[i] = usp[i - 1] + (unsigned short) diff;
                    break;
                case 1:
                    count = 0;
                    for (i = 0; i < ddim->udoms; i++) {
                        for (j = 0; j < *((long*) ddim->sams + i); j++) {
                            usp[count++] = *((unsigned short*) ddim->offs + i) +
                                           (unsigned short) j * *((unsigned short*) ddim->ints + i);
                        }
                    }
                    break;
                case 2:
                    for (i = 0; i < ddim->udoms; i++)usp[i] = *((unsigned short*) ddim->offs + i);
                    break;
                case 3:
                    for (i = 0; i < ddim->udoms; i++)
                        usp[i] = *((unsigned short*) ddim->offs) + (unsigned short) i * *((unsigned short*) ddim->ints);
                    break;
            }
            break;

        case TYPE_UNSIGNED_INT :
            if (ddim->dim == NULL) {
                if ((ddim->dim = (char*) malloc(ndata * sizeof(unsigned int))) == NULL) {
                    return UNCOMPRESS_ALLOCATING_HEAP;
                }
            }
            uip = (unsigned int*) ddim->dim;

            switch (ddim->method) {
                case 0:
                    uip[0] = (unsigned int) d0;
                    for (i = 1; i < ndata; i++)uip[i] = uip[i - 1] + (unsigned int) diff;
                    break;
                case 1:
                    count = 0;
                    for (i = 0; i < ddim->udoms; i++) {
                        for (j = 0; j < *((long*) ddim->sams + i); j++) {
                            uip[count++] = *((unsigned int*) ddim->offs + i) +
                                           (unsigned int) j * *((unsigned int*) ddim->ints + i);
                        }
                    }
                    break;
                case 2:
                    for (i = 0; i < ddim->udoms; i++)uip[i] = *((unsigned int*) ddim->offs + i);
                    break;
                case 3:
                    for (i = 0; i < ddim->udoms; i++)
                        uip[i] = *((unsigned int*) ddim->offs) + (unsigned int) i * *((unsigned int*) ddim->ints);
                    break;
            }
            break;

        case TYPE_UNSIGNED_LONG :
            if (ddim->dim == NULL) {
                if ((ddim->dim = (char*) malloc(ndata * sizeof(unsigned long))) == NULL) {
                    return UNCOMPRESS_ALLOCATING_HEAP;
                }
            }
            ulp = (unsigned long*) ddim->dim;
            switch (ddim->method) {
                case 0:
                    ulp[0] = (unsigned long) d0;
                    for (i = 1; i < ndata; i++)ulp[i] = ulp[i - 1] + (unsigned long) diff;
                    break;
                case 1:
                    count = 0;
                    for (i = 0; i < ddim->udoms; i++) {
                        for (j = 0; j < *((unsigned long*) ddim->sams + i); j++) {
                            ulp[count++] = *((unsigned long*) ddim->offs + i) +
                                           (unsigned long) j * *((unsigned long*) ddim->ints + i);
                        }
                    }
                    break;
                case 2:
                    for (i = 0; i < ddim->udoms; i++)ulp[i] = *((unsigned long*) ddim->offs + i);
                    break;
                case 3:
                    for (i = 0; i < ddim->udoms; i++)
                        ulp[i] = *((unsigned long*) ddim->offs) + (unsigned long) i * *((unsigned long*) ddim->ints);
                    break;
            }
            break;
#ifndef __APPLE__
        case TYPE_UNSIGNED_LONG64 : {
            unsigned long long int* ulp;
            if (ddim->dim == NULL) {
                if ((ddim->dim = (char*) malloc(ndata * sizeof(unsigned long long int))) == NULL) {
                    return UNCOMPRESS_ALLOCATING_HEAP;
                }
            }
            ulp = (unsigned long long int*) ddim->dim;
            switch (ddim->method) {
                case 0:
                    ulp[0] = (unsigned long long int) d0;
                    for (i = 1; i < ndata; i++)ulp[i] = ulp[i - 1] + (unsigned long long int) diff;
                    break;
                case 1:
                    count = 0;
                    for (i = 0; i < ddim->udoms; i++) {
                        for (j = 0; j < *((unsigned long long int*) ddim->sams + i); j++) {
                            ulp[count++] = *((unsigned long long int*) ddim->offs + i) +
                                           (unsigned long long int) j * *((unsigned long long int*) ddim->ints + i);
                        }
                    }
                    break;
                case 2:
                    for (i = 0; i < ddim->udoms; i++)ulp[i] = *((unsigned long long int*) ddim->offs + i);
                    break;
                case 3:
                    for (i = 0; i < ddim->udoms; i++)
                        ulp[i] = *((unsigned long long int*) ddim->offs) + (unsigned long long int) i *
                                                                           *((unsigned long long int*) ddim->ints);
                    break;
            }
            break;
        }
#endif

        default:
            return UNKNOWN_DATA_TYPE;
    }

// Retain this compression status as required by protocol manager routine
// ddim->compressed = 0; becaue this Indicates that data is available in a block

    return 0;
}
