//
// Data Access
//----------------------------------------------------------------

#include "xdata.h"

#include <math.h>
#include <float.h>
#include <stdlib.h>

#include <client/accAPI_C.h>
#include <client/IdamAPI.h>

int xdatamapx(int rGridCount, float* rGrid, int ndata, float* rdata, float* data, float* mapped)
{
//            |..... to this grid .......|..... from this grid .....|  map this | result       |      
// Spatial mapping: data(rdata) -> mapped(rGrid)
//
// Returns non zero if an error occurs

    int i, j;
    float gradient;

    for (i = 0; i < rGridCount; i++) {        // Each point required
        mapped[i] = 0.0;
        if (!isfinite(rGrid[i])) return 1;
        for (j = 0; j < ndata - 1; j++) {
            if (!isfinite(rdata[j]) || !isfinite(rdata[j + 1]) || !isfinite(data[j]) || !isfinite(data[j + 1]))
                return 1;

            if (rGrid[i] >= rdata[j + 1] && rGrid[i] < rdata[j]) {
                gradient = (data[j + 1] - data[j]) / (rdata[j + 1] - rdata[j]);
                mapped[i] = gradient * (rGrid[i] - rdata[j + 1]) + data[j + 1];
                break;
            }
        }
    }

    return 0;
}

int xdatamap(int rGridCount, float* rGrid, int ndata, float* rdata, float* data, float minvalue, float* mapped);

int xdatamapw(int volumePoints, EQUIMAPDATA* equimapdata, EFITDATA* efitdata, float* data, float minvalue,
              float* mapped)
{
    if (volumePoints) {
        switch (equimapdata->rhoType) {
            case (SQRTNORMALISEDTOROIDALFLUX):
                return xdatamap(equimapdata->rhoCount, equimapdata->rho, efitdata->qCount, efitdata->trho, data,
                                minvalue, mapped);
            case (NORMALISEDPOLOIDALFLUX):
                return xdatamap(equimapdata->rhoCount, equimapdata->rho, efitdata->qCount, efitdata->rho, data,
                                minvalue, mapped);
            case (NORMALISEDITMFLUXRADIUS):
                return xdatamap(equimapdata->rhoCount, equimapdata->rho, efitdata->qCount, efitdata->rho_tor, data,
                                minvalue, mapped);
        }
    } else {
        switch (equimapdata->rhoType) {
            case (SQRTNORMALISEDTOROIDALFLUX):
                return xdatamap(equimapdata->rhoBCount, equimapdata->rhoB, efitdata->qCount, efitdata->trho, data,
                                minvalue, mapped);
            case (NORMALISEDPOLOIDALFLUX):
                return xdatamap(equimapdata->rhoBCount, equimapdata->rhoB, efitdata->qCount, efitdata->rho, data,
                                minvalue, mapped);
            case (NORMALISEDITMFLUXRADIUS):
                return xdatamap(equimapdata->rhoBCount, equimapdata->rhoB, efitdata->qCount, efitdata->rho_tor, data,
                                minvalue, mapped);
        }
    }
    return 999;
}

int xdatamap(int rGridCount, float* rGrid, int ndata, float* rdata, float* data, float minvalue,
             float* mapped)
{

// Spatial mapping: data(rdata) -> mapped(rGrid)
//
// Assumptions:		ne(R < Rmin) = minimal value
//			ne(R > Rmax) = minimal value
//			if(rne[0] > Rmin, then linear interpolate using min value at Rmin
//			if(rne[max] < Rmin, then linear interpolate using min value at Rmax 

// Returns non zero if an error occurs

// Lowest value interval

    int istart, istop, i, j, j0, jn;
    float gradient;

    for (i = 0; i < rGridCount; i++) mapped[i] = 0.0;

// Find First Good Point

    j0 = 0;
    for (j = 0; j < ndata; j++) {
        if (isfinite(rdata[j]) && isfinite(data[j])) {
            j0 = j;
            break;
        }
    }

    if (j0 == ndata - 1) {
        idamLog(LOG_ERROR, "All data are either NaN or Infinite!\n");
        return 1;
    }

// Find Last Good Point

    jn = ndata - 1;
    for (j = 0; j < ndata; j++) {
        if (isfinite(rdata[ndata - 1 - j]) && isfinite(data[ndata - 1 - j])) {
            jn = ndata - 1 - j;
            break;
        }
    }

    if (j0 == jn) {
        idamLog(LOG_ERROR, "Only 1 good data point - No points to interpolate between!\n");
        return 2;
    }


// Does the first data point start inside the target grid? If so linearly interpolate for a value.

    istart = 0;
    if (rdata[j0] > rGrid[0]) {
        gradient = (data[j0] - minvalue) / (rdata[j0] - rGrid[0]);    // the difference may be rounding error only!
        for (i = 0; i < rGridCount; i++) {
            if (rGrid[i] >= rdata[j0]) {
                istart = i;
                break;
            }
            mapped[i] = gradient * (rGrid[i] - rGrid[0]) + minvalue;

//idamLog(LOG_ERROR,"A[%2d] %12.4e   %12.4e   %12.4e   %12.4e   %12.4e \n", i, gradient, rGrid[i], rGrid[0], minvalue, mapped[i]);
        }
    }


// Highest value interval
// Is the last data point inside the target grid? If so linearly interpolate for a value.

    istop = rGridCount - 1;
    if (rdata[jn] < rGrid[rGridCount - 1]) {
        gradient = (data[jn] - minvalue) / (rdata[jn] - rGrid[rGridCount - 1]);
        for (i = istart; i < rGridCount; i++) {
            if (rGrid[i] <= rdata[jn]) {
                istop = i;
            } else {
                mapped[i] = gradient * (rGrid[i] - rGrid[rGridCount - 1]) + minvalue;

//idamLog(LOG_ERROR,"B[%2d] %12.4e   %12.4e   %12.4e   %12.4e   %12.4e \n", i, gradient, rGrid[i], rGrid[rGridCount-1], minvalue, mapped[i]);
            }
        }
    }

// Remaining data interval

/*
idamLog(LOG_ERROR,"istart=%d\n", istart);
idamLog(LOG_ERROR,"istop =%d\n", istop);
idamLog(LOG_ERROR,"n     =%d\n", rGridCount-1);
idamLog(LOG_ERROR,"j0    =%d\n", j0);
idamLog(LOG_ERROR,"jn    =%d\n", jn);
idamLog(LOG_ERROR,"n     =%d\n", ndata-1);
*/
    for (i = istart; i <= istop; i++) {

        for (j = j0; j <= jn - 1; j++) {
            if (isfinite(rdata[j]) && isfinite(rdata[j + 1]) && isfinite(data[j]) && isfinite(data[j + 1]) &&
                rdata[j] <= rGrid[i] && rdata[j + 1] > rGrid[i]) {
                gradient = (data[j + 1] - data[j]) / (rdata[j + 1] - rdata[j]);
                mapped[i] = gradient * (rGrid[i] - rdata[j]) + data[j];
//idamLog(LOG_ERROR,"C[%2d] %12.4e   %12.4e   %12.4e   %12.4e   %12.4e \n",
//i, gradient, rGrid[i], rdata[j], data[j], mapped[i]); 
            }
        }

    }

    return 0;
}

int xdataintegrate(int ndata, float* rdata, float* data, float minvalue, int* narea, float* area, float* xarea)
{

// Integrate: area = sum data*dx
//
// Returns non zero if an error occurs

// Lowest value interval

    int j, k, j0, jn;

// Find First Good Point

    j0 = 0;
    for (j = 0; j < ndata; j++) {
        if (isfinite(rdata[j]) && isfinite(data[j])) {
            j0 = j;
            break;
        }
    }

    if (j0 == ndata - 1) {
        idamLog(LOG_ERROR, "All data are either NaN or Infinite!\n");
        return 1;
    }

// Find Last Good Point

    jn = ndata - 1;
    for (j = 0; j < ndata; j++) {
        if (isfinite(rdata[ndata - 1 - j]) && isfinite(data[ndata - 1 - j])) {
            jn = ndata - 1 - j;
            break;
        }
    }

    if (j0 == jn) {
        idamLog(LOG_ERROR, "Only 1 good data point - No points to interpolate between!\n");
        return 2;
    }

// Integrate

    k = 1;

    if (j0 > 0) {
        xarea[0] = rdata[j0];
        area[0] = minvalue * rdata[j0];
    } else {
        xarea[0] = 0.0;
        area[0] = 0.0;
    }

//idamLog(LOG_ERROR,"area[0] %12.4e   %12.4e \n",area[0],xarea[0]);

    for (j = j0; j <= jn - 1; j++) {
        if (isfinite(rdata[j]) && isfinite(rdata[j + 1]) && isfinite(data[j]) && isfinite(data[j + 1])) {
            xarea[k] = rdata[j + 1];
            area[k] = area[k - 1] + 0.5 * (data[j] + data[j + 1]) * (rdata[j + 1] - rdata[j]);

//idamLog(LOG_ERROR,"[%2d] area[%2d] %12.4e   %12.4e \n",j, k, area[k],xarea[k]);

            k++;
        }
    }

    *narea = k;

    return 0;
}


int xdatainterval(int rank, int order, int ndata, int* shape, float* dim,
                  float tslice, float twindow, int* target1, int* target2)
{

    int j;
    float* tvec;
    float epsilon = 2.0 * FLT_EPSILON;

    if (order < 0) {
        idamLog(LOG_ERROR, "Error: Unable to locate a time interval - there is no time vector for this data!\n");
        return (1);
    }

    if (order == 0) {
        tvec = dim;
    } else if (order == 1) {
        tvec = &dim[shape[0]];
    } else if (order == 2) {
        tvec = &dim[shape[0] + shape[1]];
    }

    *target1 = -1;
    *target2 = -1;

    for (j = 0; j < shape[order]; j++) {
        if (*target1 == -1 && isfinite(tvec[j]) && tvec[j] >= (tslice - twindow - epsilon)) {
            *target1 = j;
            //if(twindow == 0.0){
            if (twindow - 0.0 <= FLT_EPSILON) {
                *target2 = j;
                break;
            }
            continue;
        }
        if (*target2 == -1 && isfinite(tvec[j]) && tvec[j] > (tslice + twindow + epsilon)) {
            *target2 = j - 1;
            break;
        }

    }

    if (*target1 == -1 || *target2 == -1) {
        idamLog(LOG_ERROR, "The requested Time %e could not be located in the coordinate data array!\n", tslice);
        return (2);
    }

    return 0;
}

int xdatand(char* signal, char* source, int* hand, int* rank, int* order, int* ndata, int** shape,
            float** data, float** dim)
{

    int i, err = 0, handle, ldim, ndim, offset = 0;
    float* fpd, * fpt;
    int* sp;

    *data = NULL;
    if (shape != NULL) *shape = NULL;
    if (dim != NULL) *dim = NULL;

//--------------------------------------------------------------------------------------------------------------      
// Read Data

    if (*hand < 0) {
        if ((handle = idamGetAPI(signal, source)) < 0 || getIdamErrorCode(handle) != 0) {
            idamLog(LOG_ERROR, "Error: No %s Data from %s!\n %s\n", signal, source, (char*) getIdamErrorMsg(handle));
            err = 1;
            return err;
        }
    } else handle = *hand;

    *ndata = getIdamDataNum(handle);
    *rank = getIdamRank(handle);
    *order = getIdamOrder(handle);

    sp = (int*) malloc(*rank * sizeof(int));

    ldim = 0;
    ndim = 1;
    for (i = 0; i < *rank; i++) {
        sp[i] = getIdamDimNum(handle, i);            // Array Shape: data[2][1][0]

        ldim = ldim + sp[i];                // Total Length of Coordinate Arrays
        ndim = ndim * sp[i];                // Total Size of Measurement Data
    }

    if (*ndata != ndim) {
        idamLog(LOG_ERROR, "Error: %s Coordinate Data from %s has an Inconsistent Shape (%d) %d!\n",
                signal, source, ndim, *ndata);
        free((void*) sp);
        err = 3;
        return err;
    }

    fpd = (float*) malloc(*ndata * sizeof(float));        // Measurement Array

    getIdamFloatData(handle, fpd);            // Extract Measurement Data as floats regardless of original type

    if (dim != NULL) {
        fpt = (float*) malloc(ldim * sizeof(float));    // Concatenated Coordinate Arrays

        getIdamFloatDimData(handle, 0, fpt);        // First Dimension
        offset = sp[0];
        for (i = 1; i < *rank; i++) {
            getIdamFloatDimData(handle, i, &fpt[offset]);
            offset = offset + sp[i];
        }
        *dim = fpt;
    }

    *data = fpd;
    *hand = handle;

    if (shape != NULL) {
        *shape = sp;
    } else {
        free((void*) sp);
    }

    return 0;
}

int xdata1d(char* signal, char* source, int* hand, int* ndata, float** data, float** dim)
{

    int err = 0, handle;
    float* fpd, * fpt;

//--------------------------------------------------------------------------------------------------------------      
// Read Data

    if (*hand < 0) {
        if ((handle = idamGetAPI(signal, source)) < 0 || getIdamErrorCode(handle) != 0) {
            idamLog(LOG_ERROR, "Error: No %s Data from %s!\n %s\n", signal, source, (char*) getIdamErrorMsg(handle));
            err = 1;
            return err;
        }
    } else handle = *hand;

    if (getIdamRank(handle) != 1) {
        idamLog(LOG_ERROR, "Error: %s Data from %s is not Rank 1 [%d]\n", signal, source, getIdamRank(handle));
        err = 2;
        return err;
    }

    *ndata = getIdamDataNum(handle);

    if (*ndata != getIdamDimNum(handle, 0)) {
        idamLog(LOG_ERROR, "Error: %s Coordinate Data from %s has an Inconsistent Length (%d) %d!\n",
                signal, source, getIdamDimNum(handle, 0), *ndata);
        err = 3;
        return err;
    }

    fpd = (float*) malloc(*ndata * sizeof(float));
    fpt = (float*) malloc(*ndata * sizeof(float));

    getIdamFloatData(handle, fpd);
    getIdamFloatDimData(handle, 0, fpt);

    *data = fpd;
    *dim = fpt;
    *hand = handle;

    return 0;
}      	 
