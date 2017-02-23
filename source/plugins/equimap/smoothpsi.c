//
//----------------------------------------------------------------
// smooth the psi map external to the LCFS
//----------------------------------------------------------------

#include "smoothpsi.h"

#include <math.h>
#include <stdlib.h>

int smoothPsi(EQUIMAPDATA* equimapdata, int invert, int limitPsi, float limitRMaj)
{

    int i, j, k, jj, kk, nb, nr, nz;
    int wrmin, wrmax, wzmin, wzmax;


    float rrmax, rrmin, zzmax, zzmin;
    float* rgridSR, * zgridSR;
    float** psigSR;

// Reduce the Fixed Grid to the Minimum size enclosing the plasma boundary at all times

    float* rmax = (float*) malloc(equimapdata->timeCount * sizeof(float));
    float* rmin = (float*) malloc(equimapdata->timeCount * sizeof(float));
    float* zmax = (float*) malloc(equimapdata->timeCount * sizeof(float));
    float* zmin = (float*) malloc(equimapdata->timeCount * sizeof(float));

    for (i = 0; i < equimapdata->timeCount; i++) {        // Dimensions of plasma boundary at each time slice
        nb = equimapdata->efitdata[i].nlcfs;
        rmax[i] = -1.0E10;
        rmin[i] = +1.0E10;
        zmax[i] = -1.0E10;
        zmin[i] = +1.0E10;
        for (j = 0; j < nb; j++) {
            if (equimapdata->efitdata[i].rlcfs[j] > rmax[i]) rmax[i] = equimapdata->efitdata[i].rlcfs[j];
            if (equimapdata->efitdata[i].rlcfs[j] < rmin[i]) rmin[i] = equimapdata->efitdata[i].rlcfs[j];
            if (equimapdata->efitdata[i].zlcfs[j] > zmax[i]) zmax[i] = equimapdata->efitdata[i].zlcfs[j];
            if (equimapdata->efitdata[i].zlcfs[j] < zmin[i]) zmin[i] = equimapdata->efitdata[i].zlcfs[j];
        }
    }

    rrmax = -1.0E10;
    rrmin = +1.0E10;
    zzmax = -1.0E10;
    zzmin = +1.0E10;

    for (i = 0; i < equimapdata->timeCount; i++) {            // Dimensions of minimum grid
        if (rmax[i] > rrmax) rrmax = rmax[i];
        if (rmin[i] < rrmin) rrmin = rmin[i];
        if (zmax[i] > zzmax) zzmax = zmax[i];
        if (zmin[i] < zzmin) zzmin = zmin[i];
    }

    if (limitRMaj >= 0.0) {
        rrmax = limitRMaj;            // Over-rule with input value
        for (i = 0; i < equimapdata->timeCount; i++) rmax[i] = limitRMaj;

// Make the Z Grid top-down symmetric      

        if (zzmax > fabs(zzmin)) {
            zzmin = -zzmax;
        } else if (zzmax < fabs(zzmin)) {
            zzmax = fabs(zzmin);
        }
    }

    IDAM_LOG(LOG_DEBUG, "EQUIMAP: smoothPsi\n");
    IDAM_LOGF(LOG_DEBUG, "limitRMaj = %f\n", limitRMaj);
    IDAM_LOGF(LOG_DEBUG, "rmin = %f, rmax = %f\n", rrmin, rrmax);
    IDAM_LOGF(LOG_DEBUG, "zmin = %f, zmax = %f\n", zzmin, zzmax);

    wrmin = -1;
    wrmax = -1;
    wzmin = -1;
    wzmax = -1;

    for (i = 0; i < equimapdata->efitdata[0].psiCount[0]; i++) {        // Scan Grid Major radii
        if (equimapdata->efitdata[0].rgrid[i] >= rrmin) {
            wrmin = i - 1;
            break;
        }
    }
    for (i = 0; i < equimapdata->efitdata[0].psiCount[0]; i++) {
        if (equimapdata->efitdata[0].rgrid[i] >= rrmax) {
            wrmax = i + 1;
            break;
        }
    }

    for (i = 0; i < equimapdata->efitdata[0].psiCount[1]; i++) {        // Scan Grid Z
        if (equimapdata->efitdata[0].zgrid[i] >= zzmin) {
            wzmin = i - 1;
            break;
        }
    }
    for (i = equimapdata->efitdata[0].psiCount[1] - 1; i >= 0; i--) {
        if (equimapdata->efitdata[0].zgrid[i] <= zzmax) {
            wzmax = i + 1;
            break;
        }
    }

// Check grid indices

    if (wrmin < 0) wrmin = 0;
    if (wzmin < 0) wzmin = 0;
    if (wrmax < 0) wrmax = equimapdata->efitdata[0].psiCount[0] - 1;
    if (wzmax < 0) wzmax = equimapdata->efitdata[0].psiCount[1] - 1;

    //if(limitRMaj >= 0.0 &&
    //   equimapdata->efitdata[0].zgrid[wzmax] != abs(equimapdata->efitdata[0].zgrid[wzmin])){	// Ensure top-down symmetry
    //   wzmin = equimapdata->efitdata[0].psiCount[1]-1 - wzmax;
    //}
    if (limitRMaj >= 0.0) {    // No change to the Z grid
        wzmin = 0;
        wzmax = equimapdata->efitdata[0].psiCount[1] - 1;
    }

    IDAM_LOGF(LOG_DEBUG, "wrmin = %d, wrmax = %d\n", wrmin, wrmax);
    IDAM_LOGF(LOG_DEBUG, "wzmin = %d, wzmax = %d\n", wzmin, wzmax);

// Reduce the Grid

    nr = equimapdata->efitdata[0].psiCount[0];
    if (wrmin != 0 || wrmax != equimapdata->efitdata[0].psiCount[0]) {
        nr = wrmax - wrmin + 1;
        rgridSR = (float*) malloc(nr * sizeof(float));
        for (j = 0; j < nr; j++)rgridSR[j] = equimapdata->efitdata[0].rgrid[wrmin + j];
        if (limitRMaj < 0.0) {
            for (i = 0; i < equimapdata->timeCount; i++) {
                equimapdata->efitdata[i].rgridSR = (float*) malloc(nr * sizeof(float));
                for (j = 0; j < nr; j++)equimapdata->efitdata[i].rgridSR[j] = rgridSR[j];
                equimapdata->efitdata[i].psiCountSR[0] = nr;
            }
        } else {
            for (i = 0; i < equimapdata->timeCount; i++) {
                equimapdata->efitdata[i].rgridRZBox = (float*) malloc(nr * sizeof(float));
                for (j = 0; j < nr; j++)equimapdata->efitdata[i].rgridRZBox[j] = rgridSR[j];
                equimapdata->efitdata[i].psiCountRZBox[0] = nr;
            }
        }
        free((void*) rgridSR);
    }
    nz = equimapdata->efitdata[0].psiCount[1];
    if (wzmin != 0 || wzmax != equimapdata->efitdata[0].psiCount[1]) {
        nz = wzmax - wzmin + 1;
        zgridSR = (float*) malloc(nz * sizeof(float));
        for (j = 0; j < nz; j++)zgridSR[j] = equimapdata->efitdata[0].zgrid[wzmin + j];
        if (limitRMaj < 0.0) {
            for (i = 0; i < equimapdata->timeCount; i++) {
                equimapdata->efitdata[i].zgridSR = (float*) malloc(nz * sizeof(float));
                for (j = 0; j < nz; j++)equimapdata->efitdata[i].zgridSR[j] = zgridSR[j];
                equimapdata->efitdata[i].psiCountSR[1] = nz;
            }
        } else {
            for (i = 0; i < equimapdata->timeCount; i++) {
                equimapdata->efitdata[i].zgridRZBox = (float*) malloc(nz * sizeof(float));
                for (j = 0; j < nz; j++)equimapdata->efitdata[i].zgridRZBox[j] = zgridSR[j];
                equimapdata->efitdata[i].psiCountRZBox[1] = nz;
            }
        }
        free((void*) zgridSR);
    }

    IDAM_LOGF(LOG_DEBUG, "nr = %d, nz = %d\n", nr, nz);
    if (limitRMaj < 0.0) {
        IDAM_LOGF(LOG_DEBUG, "rgridSR[0] = %f, rgridSR[nr-1] = %f\n", equimapdata->efitdata[0].rgridSR[0],
                equimapdata->efitdata[0].rgridSR[nr - 1]);
        IDAM_LOGF(LOG_DEBUG, "zgridSR[0] = %f, zgridSR[nz-1] = %f\n", equimapdata->efitdata[0].zgridSR[0],
                equimapdata->efitdata[0].zgridSR[nz - 1]);
    } else {
        IDAM_LOGF(LOG_DEBUG, "rgridRZBox[0] = %f, rgridRZBox[nr-1] = %f\n", equimapdata->efitdata[0].rgridRZBox[0],
                equimapdata->efitdata[0].rgridRZBox[nr - 1]);
        IDAM_LOGF(LOG_DEBUG, "zgridRZBox[0] = %f, zgridRZBox[nz-1] = %f\n", equimapdata->efitdata[0].zgridRZBox[0],
                equimapdata->efitdata[0].zgridRZBox[nz - 1]);
    }

// Reduce the Psi Map

    if (nr != equimapdata->efitdata[0].psiCount[0] || nz != equimapdata->efitdata[0].psiCount[1]) {
        IDAM_LOG(LOG_DEBUG, "Reducing the Psi Map\n");
        for (i = 0; i < equimapdata->timeCount; i++) {
            psigSR = (float**) malloc(nz * sizeof(float*));        // nt * Psi[z][r]
            for (j = 0; j < nz; j++) psigSR[j] = (float*) malloc(nr * sizeof(float));
            jj = 0;
            for (j = 0; j < equimapdata->efitdata[0].psiCount[1]; j++) {
                if (j >= wzmin && j <= wzmax) {
                    kk = 0;
                    for (k = 0; k < equimapdata->efitdata[0].psiCount[0]; k++) {
                        if (k >= wrmin && k <= wrmax) {
                            if (invert)
                                psigSR[jj][kk++] = -equimapdata->efitdata[i].psig[j][k];
                            else
                                psigSR[jj][kk++] = equimapdata->efitdata[i].psig[j][k];
                        }
                    }
                    jj++;
                }
            }
            if (limitRMaj < 0.0) {
                equimapdata->efitdata[i].psigSR = psigSR;                // New Reduced Map
            } else {
                equimapdata->efitdata[i].psigRZBox = psigSR;
            }
        }
    }

// Smooth regions where the flux surface is outside the boundary and the flux is below/above the boundary value   


    if (!limitPsi || limitRMaj >= 0.0) {

        free((void*) rmax);
        free((void*) rmin);
        free((void*) zmax);
        free((void*) zmin);

        return 0;
    }

    IDAM_LOG(LOG_DEBUG, "Limiting the Psi Map\n");


    int** mark = (int**) malloc(nz * sizeof(int*));
    for (j = 0; j < nz; j++) mark[j] = (int*) malloc(nr * sizeof(int));

    for (i = 0; i < equimapdata->timeCount; i++) {        // Dimensions of plasma boundary at each time slice
        nb = equimapdata->efitdata[i].nlcfs;
        rgridSR = equimapdata->efitdata[i].rgridSR;
        zgridSR = equimapdata->efitdata[i].zgridSR;
        psigSR = equimapdata->efitdata[i].psigSR;

// Scan the grid for points outside the boundary

        for (j = 0; j < nz; j++) {
            for (k = 0; k < nr; k++) {
                mark[j][k] = 0;
                if (rgridSR[k] <= rmin[i]) mark[j][k] = 1;
                if (rgridSR[k] >= rmax[i]) mark[j][k] = 1;
                if (zgridSR[j] <= zmin[i]) mark[j][k] = 1;
                if (zgridSR[j] >= zmax[i]) mark[j][k] = 1;
            }
        }

// Locate data lower/higher than boundary psi outside the boundary i.e. where mark == 1

        for (j = 0; j < nz; j++) {
            for (k = 0; k < nr; k++) {
                if (mark[j][k]) {
                    if (invert) {
                        if (psigSR[j][k] < -equimapdata->efitdata[i].psi_bnd) {
                            psigSR[j][k] = -equimapdata->efitdata[i].psi_bnd +
                                           0.1 * fabs(psigSR[j][k] + equimapdata->efitdata[i].psi_bnd);
                        }
                    } else {
                        if (psigSR[j][k] > equimapdata->efitdata[i].psi_bnd) {
                            psigSR[j][k] = equimapdata->efitdata[i].psi_bnd -
                                           0.1 * fabs(psigSR[j][k] - equimapdata->efitdata[i].psi_bnd);
                        }
                    }
                }
            }
        }

        equimapdata->efitdata[i].psigSR = psigSR;
    }

    for (j = 0; j < nz; j++) free((void*) mark[j]);
    free((void*) mark);

    free((void*) rmax);
    free((void*) rmin);
    free((void*) zmax);
    free((void*) zmin);

    return 0;
}
