// **** Special Version for Otto with data from efit++
//----------------------------------------------------------------
// Select 'Good' times from equilibrium and YAG data
//----------------------------------------------------------------

#include "importdata.h"
#include "xdata.h"

#include <math.h>
#include <float.h>

#include <client/accAPI_C.h>
#include <client/IdamAPI.h>
#include <clientserver/TrimString.h>
#include <clientserver/initStructs.h>

#ifndef M_PI
# define M_PI           3.14159265358979323846
#endif

static int isAltData = 0;
static int handleCount = 0;
static int handles[MAXHANDLES];
static char signals[MAXHANDLES][MAXSIGNALNAME];
static char alias[MAXHANDLES][MAXSIGNALNAME];

int whichHandle(char* name)
{
    int i;
    for (i = 0; i < handleCount; i++) {
        if (!strcasecmp(signals[i], name)) return handles[i];
    }
    for (i = 0; i < handleCount; i++) {
        if (!strcasecmp(alias[i], name)) return handles[i];
    }
    return -1;        // Not found
}

char* whichName(char* name)
{
    int i;
    for (i = 0; i < handleCount; i++) {
        if (!strcasecmp(signals[i], name)) return alias[i];
    }
    for (i = 0; i < handleCount; i++) {
        if (!strcasecmp(alias[i], name)) return signals[i];
    }
    return NULL; // Not found
}

int whichIndex(char* name)
{
    int i;
    for (i = 0; i < handleCount; i++) {
        if (!strcasecmp(signals[i], name))return i;
    }
    for (i = 0; i < handleCount; i++) {
        if (!strcasecmp(alias[i], name))return i;
    }
    return -1;        // Not found
}


int selectTimes(EQUIMAPDATA* equimapdata)
{        // *** check EFM_STATUS !!!

    int err, i, handle, rank;

// Pass 1: Equilibrium Data Rmag - Times have already been selected with non converging solutions discarded.


// *** Assume all times are identical


    //if((handle = whichHandle("EFM_MAGNETIC_AXIS_R")) < 0) // complications when extracted from a data structure!

    if ((handle = whichHandle("EFM_R(PSI100)_IN")) < 0) {
        err = 999;
        addIdamError(&idamerrorstack, CODEERRORTYPE, "equimap", err, "No Equilibrium data times found when expected!");
        return err;
    }

    rank = getIdamRank(handle);
    if (rank <= 1) {
        equimapdata->timeCount = getIdamDataNum(handle);
        equimapdata->times = (float*) malloc(equimapdata->timeCount * sizeof(float));
        getIdamFloatDimData(handle, 0, equimapdata->times);
    } else {
        equimapdata->timeCount = getIdamDimNum(handle, getIdamOrder(handle));
        equimapdata->times = (float*) malloc(equimapdata->timeCount * sizeof(float));
        getIdamFloatDimData(handle, getIdamOrder(handle), equimapdata->times);
    }

    for (i = 0; i < equimapdata->timeCount; i++) idamLog(LOG_DEBUG, "[%d] %f\n", i, equimapdata->times[i]);

// Pass 2: YAG Data

    if ((handle = whichHandle("ayc_ne")) < 0) {
        err = 999;
        addIdamError(&idamerrorstack, CODEERRORTYPE, "equimap", err, "No YAG data times found when expected!");
        return err;
    }

    int timeCount = 0;
    float* times = NULL;

    rank = getIdamRank(handle);
    if (rank <= 1) {
        timeCount = getIdamDimNum(handle, 0);
        times = (float*) malloc(timeCount * sizeof(float));
        getIdamFloatDimData(handle, 0, times);
    } else {
        timeCount = getIdamDimNum(handle, getIdamOrder(handle));
        times = (float*) malloc(timeCount * sizeof(float));
        getIdamFloatDimData(handle, getIdamOrder(handle), times);
    }

// Check First and Last times

    if (times[0] > equimapdata->times[0]) {
        int start = 0;
        for (i = 0; i < equimapdata->timeCount; i++) if (times[0] > equimapdata->times[i])equimapdata->times[i] = -1.0;
        for (i = 0; i < equimapdata->timeCount; i++) {
            if (equimapdata->times[i] >= 0.0) {
                start = i;
                break;
            }
        }
        for (i = 0; i < equimapdata->timeCount - start - 1; i++)
            equimapdata->times[i] = equimapdata->times[i + start + 1];
        equimapdata->timeCount = equimapdata->timeCount - start - 1;
    }

    if (times[timeCount - 1] < equimapdata->times[equimapdata->timeCount - 1]) {
        for (i = 0; i < equimapdata->timeCount; i++) {
            if (times[timeCount - 1] < equimapdata->times[i]) {
                equimapdata->timeCount = i;
                break;
            }
        }
    }
    free((void*) times);

//equimapdata->times[0]  = equimapdata->times[18];
//equimapdata->timeCount = 1;
    return 0;
}

int subsetTimes(REQUEST_BLOCK* request_block)
{
    return 0;
}

int imputeData(char* signal)
{

// Remove NaNs by imputing from good neighbouring points

    int err, i, j, k, handle, dataCount, timeCount, rCount, order, offset, start, last, end;
    float* dataR, * dataT, * work, * smooth, * radii, * times;
    int* good;
    float gradient;

    if ((handle = whichHandle(signal)) < 0) {
        err = 999;
        addIdamError(&idamerrorstack, CODEERRORTYPE, "equimap imputeData", err, "No YAG data found when expected!");
        return err;
    }

    order = getIdamOrder(handle);
    dataCount = getIdamDataNum(handle);
    dataR = (float*) malloc(dataCount * sizeof(float));
    dataT = (float*) malloc(dataCount * sizeof(float));
    getIdamFloatData(handle, dataR);
    memcpy(dataT, dataR, dataCount * sizeof(float));
    timeCount = getIdamDimNum(handle, order);
    rCount = dataCount / timeCount;

    radii = (float*) malloc(rCount * sizeof(float));
    times = (float*) malloc(timeCount * sizeof(float));
    if (order == 0) {
        getIdamFloatDimData(handle, 0, times);
        getIdamFloatDimData(handle, 1, radii);
    } else {
        getIdamFloatDimData(handle, 1, times);
        getIdamFloatDimData(handle, 0, radii);
    }

    idamLog(LOG_DEBUG, "imputeData : %s\n", signal);
    idamLog(LOG_DEBUG, "time Count : %d\n", timeCount);
    idamLog(LOG_DEBUG, "radii Count: %d\n", rCount);

// Scan Across Major Radius detecting NaNs

    work = (float*) malloc(rCount * sizeof(float));
    smooth = (float*) malloc(rCount * sizeof(float));
    good = (int*) malloc(rCount * sizeof(int));

    for (i = 0; i < timeCount; i++) {
        idamLog(LOG_DEBUG, "time[%d] : %f\n", i, times[i]);
        for (j = 0; j < rCount; j++) {
            if (order == 0)
                offset = j * timeCount + i;
            else
                offset = i * rCount + j;
            work[j] = dataR[offset];        // Extract Slice of Profile
            good[j] = isfinite(dataR[offset]);
        }
        for (j = 0; j < rCount; j++) {
            if (good[j]) {
                start = j;            // First good point
                break;
            }
        }
        for (j = 0; j < rCount; j++) {
            if (good[rCount - j - 1]) {
                last = rCount - j - 1;        // Last good point
                break;
            }
        }
        for (j = 0; j < rCount; j++) {        // All good points
            if (good[j]) {
                start = j;
                end = j;
            } else {
                end = -1;
                for (k = start + 1; k < rCount; k++) {
                    if (good[k]) {
                        end = k;
                        break;
                    }
                }
            }
            if (start != end) {
                if (j < start) {
                    work[j] = work[start];
                } else if (j > last) {
                    work[j] = work[last];
                } else if (end < start) {
                    work[j] = work[start];
                } else {
                    gradient = (work[end] - work[start]) / (radii[end] - radii[start]);
                    work[j] = gradient * (radii[j] - radii[start]) + work[start];
                }
            }
        }
        for (j = 1; j < rCount - 1; j++) smooth[j] = (work[j - 1] + work[j] + work[j + 1]) / 3.0;
        smooth[0] = 0.5 * (work[0] + work[1]);
        smooth[rCount - 1] = 0.5 * (work[rCount - 2] + work[rCount - 1]);
        for (j = 0; j < rCount; j++) {
            if (order == 0)
                offset = j * timeCount + i;
            else
                offset = i * rCount + j;
            dataR[offset] = smooth[j];        // Replace Slice of Profile
        }
    }
    free((void*) work);
    free((void*) good);
    free((void*) smooth);

// Replace Data

    DATA_BLOCK* data_block = getIdamDataBlock(handle);
    data_block->data_type = TYPE_FLOAT;
    free((void*) data_block->data);
    data_block->data = (char*) dataR;

    free((void*) dataT);
    free((void*) times);
    free((void*) radii);

    return 0;
}


//----------------------------------------------------------------
// Import Data
//----------------------------------------------------------------

float lineInt(float* arr, float* x, int narr)
{

// Calculates the line integral of [arr] calculated at (x), a single dimensional coordinate
// using D.Taylor algorithm from tor_flux_dt.pro

    int i;
    double ans = 0.0;    // Initialise the integral variable (Returned Value if only one element in abscissa)

    if (narr > 1) {

        float* difference = (float*) malloc((narr - 1) * sizeof(float));
        float* weight = (float*) malloc((narr - 1) * sizeof(float));

        for (i = 0; i < narr - 1; i++) {
            difference[i] = x[i + 1] - x[i];
            if (difference[i] >= 0)        // Note where the abscissa decreases
                weight[i] = 1.0;
            else
                weight[i] = -1.0;
        }

// For each point on the line except the last

        for (i = 0; i < narr - 1; i++)
            ans = ans + 0.5 * weight[i] * (arr[i] + arr[i + 1]) * fabs(x[i] - x[i +
                                                                                1]);    // Increment integral by (mean of function at points) *      										// (distance between one point and the next)

        free((void*) difference);
        free((void*) weight);
    }

    return ans;
}

// Read all required data

int importData(REQUEST_BLOCK* request_block, EQUIMAPDATA* equimapdata)
{

    int i, err, set = 1, handleCount2 = 0;

// Shot Number String

    char source[STRING_LENGTH];

// Two possible sources: shot number (resolved by the IDAM metadata catalog) or a private file

    if (request_block->exp_number == 0 && request_block->file[0] != '\0')
        strcpy(source, request_block->source);
    else
        sprintf(source, "%d", request_block->exp_number);

//--------------------------------------------------------------------------------------------------------------
// Read Equilibrium and Experimental Data
//--------------------------------------------------------------------------------------------------------------

// Data Sets

    if (set == 1) {

        strcpy(alias[handleCount], "Rmin");            // efitdata->Rmin
        strcpy(signals[handleCount++], "EFM_R(PSI100)_IN");    // Inner Major Radius of LCFS Data
        strcpy(alias[handleCount], "Rmax");            // efitdata->Rmax
        strcpy(signals[handleCount++], "EFM_R(PSI100)_OUT");    // Outer Major Radius of LCFS Data

        strcpy(alias[handleCount], "Rmag");
        strcpy(signals[handleCount++], "EFM_MAGNETIC_AXIS_R");    // Magnetic Axis R
        strcpy(alias[handleCount], "Zmag");
        strcpy(signals[handleCount++], "EFM_MAGNETIC_AXIS_Z");    // Magnetic Axis Z
        strcpy(alias[handleCount], "Bphi");
        strcpy(signals[handleCount++], "EFM_BPHI_RMAG");        // Toroidal Magnetic Field
        strcpy(alias[handleCount], "Ip");
        strcpy(signals[handleCount++], "EFM_PLASMA_CURR(C)");    // Toroidal Plasma Current

        strcpy(alias[handleCount], "Q");            // efitdata->q
        strcpy(signals[handleCount++], "efm_q(psi)_(c)");        // Q Profile
        strcpy(alias[handleCount], "psi");
        strcpy(signals[handleCount++], "efm_psi(r,z)");        // Poloidal Flux Map
        strcpy(alias[handleCount], "jphi");
        strcpy(signals[handleCount++], "efm_plasma_curr(r,z)");    // Toroidal plasma current

        strcpy(alias[handleCount], "psiBoundary");        // efitdata->psi_bnd
        strcpy(signals[handleCount++], "efm_psi_boundary");    // poloidal flux at LCFS
        strcpy(alias[handleCount], "psiMag");
        strcpy(signals[handleCount++], "efm_psi_axis");        // poloidal flux at Magnetic Axis

        strcpy(alias[handleCount], "BVacuum");
        strcpy(signals[handleCount++], "EFM_BVAC_VAL");        // Vacuum Toroidal Magnetic Field
        strcpy(alias[handleCount], "RBVacuum");
        strcpy(signals[handleCount++], "EFM_BVAC_R");        // Reference Major Radius

        strcpy(alias[handleCount], "Nlcfs");
        strcpy(signals[handleCount++], "EFM_LCFS(N)_(C)");        // Number of points in the LCFS
        strcpy(alias[handleCount], "Rlcfs");
        strcpy(signals[handleCount++], "efm_lcfs(r)_(c)");        // LCFS Radius coordinate
        strcpy(alias[handleCount], "Zlcfs");
        strcpy(signals[handleCount++], "efm_lcfs(z)_(c)");        // LCFS Z coordinate

        strcpy(alias[handleCount], "Rlim");
        strcpy(signals[handleCount++], "efm_limiter(r)");        // Limiter major radii
        strcpy(alias[handleCount], "Zlim");
        strcpy(signals[handleCount++], "efm_limiter(z)");        // Limiter Z coordinates

// Include ITM specific data

        if (equimapdata->readITMData) {

// Scalar quantities

            strcpy(alias[handleCount], "Rgeom");            // Geometrical Axis of boundary (R)
            strcpy(signals[handleCount++], "EFM_GEOM_AXIS_R(C)");
            strcpy(alias[handleCount], "Zgeom");            // Geometrical Axis of boundary (Z)
            strcpy(signals[handleCount++], "EFM_GEOM_AXIS_Z(C)");
            strcpy(alias[handleCount], "Aminor");            // Minor radius
            strcpy(signals[handleCount++], "EFM_MINOR_RADIUS");
            strcpy(alias[handleCount], "TriangL");
            strcpy(signals[handleCount++], "EFM_TRIANG_LOWER");        // Lower Triagularity
            strcpy(alias[handleCount], "TriangU");
            strcpy(signals[handleCount++], "EFM_TRIANG_UPPER");        // Upper Triagularity
            strcpy(alias[handleCount], "Elong");
            strcpy(signals[handleCount++], "EFM_ELONGATION");        // Elongation

// 1D Psi Profiles

            strcpy(alias[handleCount], "P");
            strcpy(signals[handleCount++], "efm_p(psi)_(c)");        // Pressure Profile
            strcpy(alias[handleCount], "F");
            strcpy(signals[handleCount++], "efm_f(psi)_(c)");        // F Diamagnetic Profile
            strcpy(alias[handleCount], "PPrime");
            strcpy(signals[handleCount++], "EFM_PPRIME");        // p-prime
            strcpy(alias[handleCount], "FFprime");
            strcpy(signals[handleCount++], "EFM_FFPRIME");        // ff-prime
// ? Jphi
// ? Jparallel
// ? Rinboard
// ? Routboard
            strcpy(alias[handleCount], "ElongPsi");
            strcpy(signals[handleCount++], "EFM_ELONG(PSI)_(C)");    // Elongation
            strcpy(alias[handleCount], "TriangLPsi");
            strcpy(signals[handleCount++], "EFM_TRIANG_L(PSI)_(C)");    // Lower Triagularity profile
            strcpy(alias[handleCount], "TriangUPsi");
            strcpy(signals[handleCount++], "EFM_TRIANG_U(PSI)_(C)");    // Upper Triagularity profile
            strcpy(alias[handleCount], "VolPsi");
            strcpy(signals[handleCount++], "EFM_VOLP_(C)");            // Volume
            strcpy(alias[handleCount], "AreaPsi");
            strcpy(signals[handleCount++], "EFM_AREAP_(C)");        // Area

// ? gm1
// ? gm2
// ? gm3

// 2D (R,Z) Profiles

// ? Br
// ? Bz
// ? Bphi

// 1D Core Profiles (Rho dependent)

// ? Jtotal
// ? ni
// ? Ti
// ? Vtor
// ? Zeff

// Scalar

// ? Atomic Mass
// ? Nuclear Charge
// ? Dominant ionisation state

        }
    }

    if (set == 2) {
        strcpy(signals[handleCount++], "EFM_XPOINT1_R(C)");
        strcpy(signals[handleCount++], "EFM_XPOINT1_Z(C)");
        strcpy(signals[handleCount++], "EFM_XPOINT2_R(C)");
        strcpy(signals[handleCount++], "EFM_XPOINT2_Z(C)");
    }

//--------------------------------------------------------------------------------------------------------------
// Mapping from aPrivate Equilibrium file

// Set Server Properties & additional mapping data

    for (i = 0; i < request_block->nameValueList.pairCount; i++) {
        if (!strcasecmp(request_block->nameValueList.nameValue[i].name, "altData")) {
            setIdamProperty("altData");                        // Use efit++ data with legacy name mappings

// Additional signals needed to map the flux profile

            isAltData = 1;
            strcpy(alias[handleCount], "profile-R");
            strcpy(signals[handleCount++], "/output/profiles2D/r");
            strcpy(alias[handleCount], "profile-Z");
            strcpy(signals[handleCount++], "/output/profiles2D/z");

            continue;
        }
        if (!strcasecmp(request_block->nameValueList.nameValue[i].name, "altRank")) {
            setIdamProperty(request_block->nameValueList.nameValue[i].pair);    // Rank of Equilibrium mapping
            continue;
        }
        if (!strcasecmp(request_block->nameValueList.nameValue[i].name, "source")) {
            copyString(request_block->nameValueList.nameValue[i].value, source,
                       STRING_LENGTH - 1);        // Private File or other source
            continue;
        }
    }

//--------------------------------------------------------------------------------------------------------------
// Measurement Data - from the MAST Archive only

    if (set == 1) {

        handleCount2 = handleCount;                // Keep track of two groups from same/different sources

        if (request_block->exp_number > ATMAYCSHOT) {
            strcpy(signals[handleCount++], "ayc_r");        // major radii
            strcpy(signals[handleCount++], "ayc_ne");        // electron density
            strcpy(signals[handleCount++], "ayc_te");        // electron temperature
        } else {
            strcpy(signals[handleCount++], "atm_r");
            strcpy(signals[handleCount++], "atm_ne");
            strcpy(signals[handleCount++], "atm_te");
        }
    }

//--------------------------------------------------------------------------------------------------------------
// Read Data from Equilibrium Group

    for (i = 0; i < handleCount2; i++) {

        if ((handles[i] = idamGetAPI(signals[i], source)) < 0 || getIdamErrorCode(handles[i]) != 0) {

            idamLog(LOG_ERROR, "ERROR equimap importdata: No Data for %s", signals[i]);
            idamLog(LOG_ERROR, "ERROR: %s", getIdamErrorMsg(handles[i]));

            idamLog(LOG_DEBUG, "ERROR equimap importdata: No Data for %s, %s", signals[i], source);
            idamLog(LOG_DEBUG, "handle %d", handles[i]);
            idamLog(LOG_DEBUG, "error code %d", getIdamErrorCode(handles[i]));
            idamLog(LOG_DEBUG, "error msg: %s", getIdamErrorMsg(handles[i]));

            err = 999;
            return err;
        }
    }

// Read Data from Measurement Group

    if (isAltData) {
        sprintf(source, "%d", request_block->exp_number);
        resetIdamClientFlag(CLIENTFLAG_ALTDATA);
        resetIdamProperty("altRank=0");
    }

    for (i = handleCount2; i < handleCount; i++) {

        if ((handles[i] = idamGetAPI(signals[i], source)) < 0 || getIdamErrorCode(handles[i]) != 0) {

            idamLog(LOG_ERROR, "ERROR equimap importdata: No Data for %s\n", signals[i]);
            idamLog(LOG_ERROR, "ERROR: %s\n", getIdamErrorMsg(handles[i]));
            idamLog(LOG_DEBUG, "ERROR equimap importdata: No Data for %s, %s", signals[i], source);
            idamLog(LOG_DEBUG, "handle %d", handles[i]);
            idamLog(LOG_DEBUG, "error code %d", getIdamErrorCode(handles[i]));
            idamLog(LOG_DEBUG, "error msg: %s", getIdamErrorMsg(handles[i]));

            err = 999;
            return err;
        }
    }

//--------------------------------------------------------------------------------------------------------------
// Remove Shot dependency for YAG data - Use a generic name

    if (request_block->exp_number <= ATMAYCSHOT) {
        if ((i = whichIndex("atm_r")) >= 0) {
            strcpy(signals[i], "ayc_r");        // major radii
            strcpy(alias[i], "ayc_r");
        }
        if ((i = whichIndex("atm_ne")) >= 0) {
            strcpy(signals[i], "ayc_ne");        // electron density
            strcpy(alias[i], "ayc_ne");
        }
        if ((i = whichIndex("atm_te")) >= 0) {
            strcpy(signals[i], "ayc_te");        // electron temperature
            strcpy(alias[i], "ayc_te");
        }
    }

//--------------------------------------------------------------------------------------------------------------
// Remove NaNs and Smooth

    imputeData("ayc_ne");
    imputeData("ayc_te");

//--------------------------------------------------------------------------------------------------------------
// BUG with EFIT++ mapped data: Change Scalars to Arrays

    if (isAltData) {
        int handle;
        if ((handle = whichHandle("EFM_R(PSI100)_IN")) < 0) {
            err = 999;
            addIdamError(&idamerrorstack, CODEERRORTYPE, "equimap", err,
                         "No Equilibrium data times found when expected!");
            return err;
        }
        int timeCount = getIdamDimNum(handle, getIdamOrder(handle));
        if (timeCount < 1) {
            err = 999;
            addIdamError(&idamerrorstack, CODEERRORTYPE, "equimap", err,
                         "No Equilibrium data times found when expected!");
            return err;
        }

        float* times = (float*) malloc(timeCount * sizeof(float));
        getIdamFloatDimData(handle, getIdamOrder(handle), times);

        for (i = 0; i < handleCount; i++) {
            DATA_BLOCK* db = getIdamDataBlock(whichHandle(signals[i]));
            if (db->rank == 0 && db->order == -1 && db->data_n == 1 && db->dims == NULL) {
                db->rank = 1;
                db->order = 0;
                db->dims = (DIMS*) malloc(sizeof(DIMS));
                initDimBlock(db->dims);
                db->dims->dim_n = 1;
                db->dims->dim = (char*) malloc(sizeof(float));
                float* temp = (float*) db->dims->dim;
                temp[0] = times[0];
                db->data_type = TYPE_FLOAT;
            }
        }
        free((void*) times);
    }

    return (0);
}

int extractData(float tslice, EFITDATA* efitdata, EQUIMAPDATA* equimapdata)
{

    int i, j, k, target1, target2, err, err1, err2,
            nslices, order, handle, set = 1;

    int* shape = NULL;
    float window = 0.0;
    float* data, * dim, * datar, * datan, * datat;

    float sum;
    int nt, nr, nz, offset;

    int rank, ndata;
    char* shot_str = "";

//--------------------------------------------------------------------------------------------------------------
// Error Trap

    err = 0;

    do {

//--------------------------------------------------------------------------------------------------------------
// Set 1 Data

        if (set == 1) {

// Read Inner Major Radius of LCFS Data

            if ((handle = whichHandle("EFM_R(PSI100)_IN")) < 0) {
                err = 999;
                addIdamError(&idamerrorstack, CODEERRORTYPE, "equimap extractData", err,
                             "IDAM data handle not found when expected!");
                return err;
            }

            if ((err = xdatand("EFM_R(PSI100)_IN", shot_str, &handle, &rank, &order, &ndata, &shape, &data, &dim)) !=
                0)
                break;

            if ((err = xdatainterval(rank, order, ndata, shape, dim, tslice, window, &target1, &target2)) != 0) {
                idamLog(LOG_ERROR, "The requested Time %e could not be located in the Rmin coordinate data array!\n",
                        tslice);
                break;
            }

            nslices = target2 - target1 + 1;
            efitdata->Rmin = 0.0;
            for (j = target1; j <= target2; j++)
                efitdata->Rmin = efitdata->Rmin + data[j];        // Inner edge of plasma
            efitdata->Rmin = efitdata->Rmin / (float) nslices;

            free((void*) shape);
            free((void*) data);
            free((void*) dim);

            idamLog(LOG_DEBUG, "[%d] Inner Major Radius of LCFS Data %f\n", handle, efitdata->Rmin);

// Read Outer Major Radius of LCFS Data

            if ((handle = whichHandle("EFM_R(PSI100)_OUT")) < 0) {
                err = 999;
                addIdamError(&idamerrorstack, CODEERRORTYPE, "equimap extractData", err,
                             "IDAM data handle not found when expected!");
                return err;
            }

            if ((err = xdatand("EFM_R(PSI100)_OUT", shot_str, &handle, &rank, &order, &ndata, &shape, &data, &dim)) !=
                0)
                break;

            if ((err = xdatainterval(rank, order, ndata, shape, dim, tslice, window, &target1, &target2)) != 0) {
                idamLog(LOG_ERROR, "The requested Time %e could not be located in the Rmax coordinate data array!\n",
                        tslice);
                break;
            }

            nslices = target2 - target1 + 1;
            efitdata->Rmax = 0.0;
            for (j = target1; j <= target2; j++)
                efitdata->Rmax = efitdata->Rmax + data[j];        // Outer edge of plasma
            efitdata->Rmax = efitdata->Rmax / (float) nslices;

            free((void*) shape);
            free((void*) data);
            free((void*) dim);

            idamLog(LOG_DEBUG, "[%d] Outer Major Radius of LCFS Data %f\n", handle, efitdata->Rmax);

// Magnetic Axis Position - 1D time dependent array

            if ((handle = whichHandle("EFM_MAGNETIC_AXIS_R")) < 0) {
                err = 999;
                addIdamError(&idamerrorstack, CODEERRORTYPE, "equimap extractData", err,
                             "IDAM data handle not found when expected!");
                return err;
            }

            if ((err = xdatand("EFM_MAGNETIC_AXIS_R", shot_str, &handle, &rank, &order, &ndata, &shape, &data, &dim)) !=
                0)
                break;

            if ((err = xdatainterval(rank, order, ndata, shape, dim, tslice, window, &target1, &target2)) != 0) {
                idamLog(LOG_ERROR, "The requested Time %e could not be located in the data array!\n", tslice);
                break;
            }

            nslices = target2 - target1 + 1;

            sum = 0.0;
            for (j = target1; j <= target2; j++) sum = sum + data[j];

            efitdata->rmag = sum / (float) nslices;

            free((void*) shape);
            free((void*) data);
            free((void*) dim);

            idamLog(LOG_DEBUG, "[%d] Magnetic Axis Position %f\n", handle, efitdata->rmag);

// Magnetic Axis Height - 1D time dependent array

            if ((handle = whichHandle("EFM_MAGNETIC_AXIS_Z")) < 0) {
                err = 999;
                addIdamError(&idamerrorstack, CODEERRORTYPE, "equimap extractData", err,
                             "IDAM data handle not found when expected!");
                return err;
            }

            if ((err = xdatand("EFM_MAGNETIC_AXIS_Z", shot_str, &handle, &rank, &order, &ndata, &shape, &data, &dim)) !=
                0)
                break;

            if ((err = xdatainterval(rank, order, ndata, shape, dim, tslice, window, &target1, &target2)) != 0) {
                idamLog(LOG_ERROR, "The requested Time %e could not be located in the data array!\n", tslice);
                break;
            }

            nslices = target2 - target1 + 1;

            sum = 0.0;
            for (j = target1; j <= target2; j++) sum = sum + data[j];

            efitdata->zmag = sum / (float) nslices;

            free((void*) shape);
            free((void*) data);
            free((void*) dim);

            idamLog(LOG_DEBUG, "[%d] Magnetic Axis Height %f\n", handle, efitdata->zmag);

// Toroidal Magnetic Field

            if ((handle = whichHandle("EFM_BPHI_RMAG")) < 0) {
                err = 999;
                addIdamError(&idamerrorstack, CODEERRORTYPE, "equimap extractData", err,
                             "IDAM data handle not found when expected!");
                return err;
            }

            if ((err = xdatand("EFM_BPHI_RMAG", shot_str, &handle, &rank, &order, &ndata, &shape, &data, &dim)) !=
                0)
                break;

            if ((err = xdatainterval(rank, order, ndata, shape, dim, tslice, window, &target1, &target2)) != 0) {
                idamLog(LOG_ERROR, "The requested Time %e could not be located in the data array!\n", tslice);
                break;
            }

            nslices = target2 - target1 + 1;

            sum = 0.0;
            for (j = target1; j <= target2; j++) sum = sum + data[j];

            efitdata->bphi = sum / (float) nslices;

            free((void*) shape);
            free((void*) data);
            free((void*) dim);

            idamLog(LOG_DEBUG, "[%d] Toroidal Magnetic Field  %f\n", handle, efitdata->bphi);

// Plasma Current

            if ((handle = whichHandle("EFM_PLASMA_CURR(C)")) < 0) {
                err = 999;
                addIdamError(&idamerrorstack, CODEERRORTYPE, "equimap extractData", err,
                             "IDAM data handle not found when expected!");
                return err;
            }

            if ((err = xdatand("EFM_PLASMA_CURR(C)", shot_str, &handle, &rank, &order, &ndata, &shape, &data, &dim)) !=
                0)
                break;

            if ((err = xdatainterval(rank, order, ndata, shape, dim, tslice, window, &target1, &target2)) != 0) {
                idamLog(LOG_ERROR, "The requested Time %e could not be located in the data array!\n", tslice);
                break;
            }

            nslices = target2 - target1 + 1;

            sum = 0.0;
            for (j = target1; j <= target2; j++) sum = sum + data[j];

            efitdata->ip = sum / (float) nslices;

            free((void*) shape);
            free((void*) data);
            free((void*) dim);

            idamLog(LOG_DEBUG, "[%d] Toroidal Plasma Current %f\n", handle, efitdata->ip);

// PSI at the Boundary - 1D time dependent array

            if ((handle = whichHandle("efm_psi_boundary")) < 0) {
                err = 999;
                addIdamError(&idamerrorstack, CODEERRORTYPE, "equimap extractData", err,
                             "IDAM data handle not found when expected!");
                return err;
            }

            if ((err = xdatand("efm_psi_boundary", shot_str, &handle, &rank, &order, &ndata, &shape, &data, &dim)) !=
                0)
                break;

            if ((err = xdatainterval(rank, order, ndata, shape, dim, tslice, window, &target1, &target2)) != 0) {
                idamLog(LOG_ERROR, "The requested Time %e could not be located in the data array!\n", tslice);
                break;
            }

            nslices = target2 - target1 + 1;

            sum = 0.0;
            for (j = target1; j <= target2; j++) sum = sum + data[j];

            efitdata->psi_bnd = sum / (float) nslices;

            free((void*) shape);
            free((void*) data);
            free((void*) dim);

            idamLog(LOG_DEBUG, "[%d] [%d, %d] PSI at the Boundary %f\n", handle, target1, target2, efitdata->psi_bnd);

// PSI at the Magnetic Axis - 1D time dependent array

            if ((handle = whichHandle("efm_psi_axis")) < 0) {
                err = 999;
                addIdamError(&idamerrorstack, CODEERRORTYPE, "equimap extractData", err,
                             "IDAM data handle not found when expected!");
                return err;
            }

            if ((err = xdatand("efm_psi_axis", shot_str, &handle, &rank, &order, &ndata, &shape, &data, &dim)) !=
                0)
                break;

            if ((err = xdatainterval(rank, order, ndata, shape, dim, tslice, window, &target1, &target2)) != 0) {
                idamLog(LOG_ERROR, "The requested Time %e could not be located in the data array!\n", tslice);
                break;
            }

            nslices = target2 - target1 + 1;

            sum = 0.0;
            for (j = target1; j <= target2; j++) sum = sum + data[j];

            efitdata->psi_mag = sum / (float) nslices;

            free((void*) shape);
            free((void*) data);
            free((void*) dim);

            idamLog(LOG_DEBUG, "[%d] [%d, %d] PSI at the Magnetic Axis %f\n", handle, target1, target2,
                    efitdata->psi_mag);

// Number of Boundary Points in the LCFS locus - 1D time dependent array
// Cannot average over time so take the first value

            if ((handle = whichHandle("EFM_LCFS(N)_(C)")) < 0) {
                err = 999;
                addIdamError(&idamerrorstack, CODEERRORTYPE, "equimap extractData", err,
                             "IDAM data handle not found when expected!");
                return err;
            }

            if ((err = xdatand("EFM_LCFS(N)_(C)", shot_str, &handle, &rank, &order, &ndata, &shape, &data, &dim)) !=
                0)
                break;

            if ((err = xdatainterval(rank, order, ndata, shape, dim, tslice, window, &target1, &target2)) != 0) {
                idamLog(LOG_ERROR, "The requested Time %e could not be located in the data array!\n", tslice);
                break;
            }

            efitdata->nlcfs = (int) data[target1];

            free((void*) shape);
            free((void*) data);
            free((void*) dim);

            idamLog(LOG_DEBUG, "[%d] [%d, %d] Number of Boundary Points in the LCFS locus %d\n", handle, target1,
                    target2, efitdata->nlcfs);

// Major Radii of the LCFS locus - 2D Time dependent:	Rlcfs[Nlcfs[t]][t]

            if ((handle = whichHandle("efm_lcfs(r)_(c)")) < 0) {
                err = 999;
                addIdamError(&idamerrorstack, CODEERRORTYPE, "equimap extractData", err,
                             "IDAM data handle not found when expected!");
                return err;
            }

            if ((err = xdatand("efm_lcfs(r)_(c)", shot_str, &handle, &rank, &order, &ndata, &shape, &data, &dim)) !=
                0)
                break;

            if ((err = xdatainterval(rank, order, ndata, shape, dim, tslice, window, &target1, &target2)) != 0) {
                idamLog(LOG_ERROR, "The requested Time %e could not be located in the data array!\n", tslice);
                break;
            }

            idamLog(LOG_DEBUG, "[%d] Rlcfs\n", handle);
            idamLog(LOG_DEBUG, "Rlcfs: target = %d, shape[0] = %d, shape[1] = %d, order = %d\n", target1, shape[0],
                    shape[1], order);

            if (order == 0) {        // array[nr][nt]
                nt = shape[0];
                nr = shape[1];
            } else {
                nt = shape[1];        // array[nt][nr]
                nr = shape[0];
            }

            float* rlcfs = (float*) malloc(
                    nr * sizeof(float));    // nr is the maximum possible, not the number of points

            if (order == 0) {        // array[nr][nt]
                for (i = 0; i < nr; i++) {
                    offset = i * nt + target1;
                    rlcfs[i] = data[offset];
                }
            } else {            // array[nt][nr]
                for (i = 0; i < nr; i++) {
                    offset = target1 * nr + i;
                    rlcfs[i] = data[offset];
                }
            }

            for (i = efitdata->nlcfs; i < nr; i++)
                rlcfs[i] = rlcfs[0];    // Set these superfluous point to the first value

            efitdata->rlcfs = rlcfs;

            free((void*) shape);
            free((void*) data);
            free((void*) dim);

// Z-Coordinate of the LCFS locus - 2D Time dependent:	Zlcfs[Nlcfs[t]][t]

            if ((handle = whichHandle("efm_lcfs(z)_(c)")) < 0) {
                err = 999;
                addIdamError(&idamerrorstack, CODEERRORTYPE, "equimap extractData", err,
                             "IDAM data handle not found when expected!");
                return err;
            }

            if ((err = xdatand("efm_lcfs(z)_(c)", shot_str, &handle, &rank, &order, &ndata, &shape, &data, &dim)) !=
                0)
                break;

            if ((err = xdatainterval(rank, order, ndata, shape, dim, tslice, window, &target1, &target2)) != 0) {
                idamLog(LOG_ERROR, "The requested Time %e could not be located in the data array!\n", tslice);
                break;
            }

            idamLog(LOG_DEBUG, "[%d] Zlcfs\n", handle);
            idamLog(LOG_DEBUG, "Zlcfs: target = %d, shape[0] = %d, shape[1] = %d, order = %d\n", target1, shape[0],
                    shape[1], order);

            if (order == 0) {        // array[nz][nt]
                nt = shape[0];
                nz = shape[1];
            } else {
                nt = shape[1];        // array[nz][nr]
                nz = shape[0];
            }

            float* zlcfs = (float*) malloc(
                    nz * sizeof(float));    // nr is the maximum possible, not the number of points

            if (order == 0) {        // array[nz][nt]
                for (i = 0; i < nz; i++) {
                    offset = i * nt + target1;
                    zlcfs[i] = data[offset];
                }
            } else {            // array[nt][nz]
                for (i = 0; i < nz; i++) {
                    offset = target1 * nz + i;
                    zlcfs[i] = data[offset];
                }
            }

            for (i = efitdata->nlcfs; i < nz; i++)
                zlcfs[i] = zlcfs[0];    // Set these superfluous points to the first value (closed)

            efitdata->zlcfs = zlcfs;

            free((void*) shape);
            free((void*) data);
            free((void*) dim);


// Major Radii of the Limiter - 1D Non-Time dependent:	Rlim[]

            if ((handle = whichHandle("efm_limiter(r)")) < 0) {
                err = 999;
                addIdamError(&idamerrorstack, CODEERRORTYPE, "equimap extractData", err,
                             "IDAM data handle not found when expected!");
                return err;
            }

            if ((err = xdatand("efm_limiter(r)", shot_str, &handle, &rank, &order, &ndata, &shape, &data, &dim)) !=
                0)
                break;

            idamLog(LOG_DEBUG, "[%d] Rlim\n", handle);

            efitdata->nlim = ndata;
            efitdata->rlim = data;

            free((void*) shape);
            free((void*) dim);

// Z-Coordinate of the Limiter - 1D Non-Time dependent:	Zlim[]

            if ((handle = whichHandle("efm_limiter(z)")) < 0) {
                err = 999;
                addIdamError(&idamerrorstack, CODEERRORTYPE, "equimap extractData", err,
                             "IDAM data handle not found when expected!");
                return err;
            }

            if ((err = xdatand("efm_limiter(z)", shot_str, &handle, &rank, &order, &ndata, &shape, &data, &dim)) !=
                0)
                break;

            idamLog(LOG_DEBUG, "[%d] Zlim\n", handle);

            efitdata->zlim = data;

            free((void*) shape);
            free((void*) dim);


// Vacuum Toroidal magnetic Field at the Reference major radius

            if ((handle = whichHandle("EFM_BVAC_VAL")) < 0) {
                err = 999;
                addIdamError(&idamerrorstack, CODEERRORTYPE, "equimap extractData", err,
                             "IDAM data handle not found when expected!");
                return err;
            }

            if ((err = xdatand("EFM_BVAC_VAL", shot_str, &handle, &rank, &order, &ndata, &shape, &data, &dim)) !=
                0)
                break;

            if ((err = xdatainterval(rank, order, ndata, shape, dim, tslice, window, &target1, &target2)) != 0) {
                idamLog(LOG_ERROR, "The requested Time %e could not be located in the data array!\n", tslice);
                break;
            }

            nslices = target2 - target1 + 1;

            sum = 0.0;
            for (j = target1; j <= target2; j++) sum = sum + data[j];

            efitdata->bvac = sum / (float) nslices;

            free((void*) shape);
            free((void*) data);
            free((void*) dim);

            idamLog(LOG_DEBUG, "[%d] [%d, %d] Vacuum Magnetic Field %f\n", handle, target1, target2, efitdata->bvac);

// Vacuum Toroidal magnetic Field Reference major radius

            if ((handle = whichHandle("EFM_BVAC_R")) < 0) {
                err = 999;
                addIdamError(&idamerrorstack, CODEERRORTYPE, "equimap extractData", err,
                             "IDAM data handle not found when expected!");
                return err;
            }

            if ((err = xdatand("EFM_BVAC_R", shot_str, &handle, &rank, &order, &ndata, &shape, &data, &dim)) != 0)
                break;

            if ((err = xdatainterval(rank, order, ndata, shape, dim, tslice, window, &target1, &target2)) != 0) {
                idamLog(LOG_ERROR, "The requested Time %e could not be located in the data array!\n", tslice);
                break;
            }

            nslices = target2 - target1 + 1;

            sum = 0.0;
            for (j = target1; j <= target2; j++) sum = sum + data[j];

            efitdata->rvac = sum / (float) nslices;

            free((void*) shape);
            free((void*) data);
            free((void*) dim);

            idamLog(LOG_DEBUG, "[%d] [%d, %d] Vacuum Magnetic Field Radius %f\n", handle, target1, target2,
                    efitdata->rvac);

//-------------------------------------------------------------------------------------------------------------
// ITM time dependent Scalar quantities: Assumed defined on the same time coordinate

            if (equimapdata->readITMData) {

// Geometrical Axis of boundary (R)

                if ((handle = whichHandle("Rgeom")) < 0) {
                    err = 999;
                    addIdamError(&idamerrorstack, CODEERRORTYPE, "equimap extractData", err,
                                 "IDAM data handle not found when expected!");
                    return err;
                }

                if ((err = xdatand(whichName("Rgeom"), shot_str, &handle, &rank, &order, &ndata, &shape, &data,
                                   &dim)) != 0)
                    break;

                if ((err = xdatainterval(rank, order, ndata, shape, dim, tslice, window, &target1, &target2)) != 0) {
                    idamLog(LOG_ERROR, "The requested Time %e could not be located in the data array!\n", tslice);
                    break;
                }

                nslices = target2 - target1 + 1;

                sum = 0.0;
                for (j = target1; j <= target2; j++) sum = sum + data[j];

                efitdata->rgeom = sum / (float) nslices;

                free((void*) shape);
                free((void*) data);
                free((void*) dim);

// Geometrical Axis of boundary (Z)

                if ((handle = whichHandle("Zgeom")) < 0) {
                    err = 999;
                    addIdamError(&idamerrorstack, CODEERRORTYPE, "equimap extractData", err,
                                 "IDAM data handle not found when expected!");
                    return err;
                }
                if ((err = xdatand(whichName("Zgeom"), shot_str, &handle, &rank, &order, &ndata, &shape, &data,
                                   &dim)) != 0)
                    break;
                sum = 0.0;
                for (j = target1; j <= target2; j++) sum = sum + data[j];
                efitdata->zgeom = sum / (float) nslices;
                free((void*) shape);
                free((void*) data);
                free((void*) dim);

// Minor radius

                if ((handle = whichHandle("Aminor")) < 0) {
                    err = 999;
                    addIdamError(&idamerrorstack, CODEERRORTYPE, "equimap extractData", err,
                                 "IDAM data handle not found when expected!");
                    return err;
                }
                if ((err = xdatand(whichName("Aminor"), shot_str, &handle, &rank, &order, &ndata, &shape, &data,
                                   &dim)) != 0)
                    break;
                sum = 0.0;
                for (j = target1; j <= target2; j++) sum = sum + data[j];
                efitdata->aminor = sum / (float) nslices;
                free((void*) shape);
                free((void*) data);
                free((void*) dim);

// Lower Triagularity

                if ((handle = whichHandle("TriangL")) < 0) {
                    err = 999;
                    addIdamError(&idamerrorstack, CODEERRORTYPE, "equimap extractData", err,
                                 "IDAM data handle not found when expected!");
                    return err;
                }
                if ((err = xdatand(whichName("TriangL"), shot_str, &handle, &rank, &order, &ndata, &shape, &data,
                                   &dim)) != 0)
                    break;
                sum = 0.0;
                for (j = target1; j <= target2; j++) sum = sum + data[j];
                efitdata->triangL = sum / (float) nslices;
                free((void*) shape);
                free((void*) data);
                free((void*) dim);

// Upper Triagularity

                if ((handle = whichHandle("TriangU")) < 0) {
                    err = 999;
                    addIdamError(&idamerrorstack, CODEERRORTYPE, "equimap extractData", err,
                                 "IDAM data handle not found when expected!");
                    return err;
                }
                if ((err = xdatand(whichName("TriangU"), shot_str, &handle, &rank, &order, &ndata, &shape, &data,
                                   &dim)) != 0)
                    break;
                sum = 0.0;
                for (j = target1; j <= target2; j++) sum = sum + data[j];
                efitdata->triangU = sum / (float) nslices;
                free((void*) shape);
                free((void*) data);
                free((void*) dim);

// Elongation

                if ((handle = whichHandle("Elong")) < 0) {
                    err = 999;
                    addIdamError(&idamerrorstack, CODEERRORTYPE, "equimap extractData", err,
                                 "IDAM data handle not found when expected!");
                    return err;
                }
                if ((err = xdatand(whichName("Elong"), shot_str, &handle, &rank, &order, &ndata, &shape, &data,
                                   &dim)) != 0)
                    break;
                sum = 0.0;
                for (j = target1; j <= target2; j++) sum = sum + data[j];
                efitdata->elong = sum / (float) nslices;
                free((void*) shape);
                free((void*) data);
                free((void*) dim);

                idamLog(LOG_DEBUG, "[%d] Geometrical Axis of boundary (R) %f\n", handle, efitdata->rgeom);
                idamLog(LOG_DEBUG, "[%d] Geometrical Axis of boundary (Z) %f\n", handle, efitdata->zgeom);
                idamLog(LOG_DEBUG, "[%d] Minor radius                     %f\n", handle, efitdata->aminor);
                idamLog(LOG_DEBUG, "[%d] Lower Triagularity               %f\n", handle, efitdata->triangL);
                idamLog(LOG_DEBUG, "[%d] Upper Triagularity               %f\n", handle, efitdata->triangU);
                idamLog(LOG_DEBUG, "[%d] Elongation                       %f\n", handle, efitdata->elong);
            }

//---------------------------------------------------------------------------------------------------------
// Psi Profiles

// Q Profile	q[normalised poloidal flux][t]

            if ((handle = whichHandle("efm_q(psi)_(c)")) < 0) {
                err = 999;
                addIdamError(&idamerrorstack, CODEERRORTYPE, "equimap extractData", err,
                             "IDAM data handle not found when expected!");
                return err;
            }

            if ((err = xdatand("efm_q(psi)_(c)", shot_str, &handle, &rank, &order, &ndata, &shape, &data, &dim)) !=
                0)
                break;

            if ((err = xdatainterval(rank, order, ndata, shape, dim, tslice, window, &target1, &target2)) != 0) {
                idamLog(LOG_ERROR, "The requested Time %e could not be located in the data array!\n", tslice);
                break;
            }

            idamLog(LOG_DEBUG, "[%d] Q Profile\n", handle);
            idamLog(LOG_DEBUG, "Q: target = %d, shape[0] = %d, shape[1] = %d, order = %d\n", target1, shape[0],
                    shape[1], order);

            if (order == 0) {        // array[nr][nt]
                nt = shape[0];
                nr = shape[1];
            } else {
                nt = shape[1];        // array[nt][nr]
                nr = shape[0];
            }

            float* q = (float*) malloc(nr * sizeof(float));

            if (order == 0) {        // array[nr][nt]
                for (i = 0; i < nr; i++) {
                    offset = i * nt + target1;
                    q[i] = data[offset];
                }
            } else {            // array[nt][nr]
                for (i = 0; i < nr; i++) {
                    offset = target1 * nr + i;
                    q[i] = data[offset];
                }
            }

            efitdata->qCount = nr;
            efitdata->q = q;

            free((void*) shape);
            free((void*) data);
            free((void*) dim);

// Include ITM specific data

            if (equimapdata->readITMData) {

// P Profile	p[normalised poloidal flux][t]

                if ((handle = whichHandle("P")) < 0) {
                    err = 999;
                    addIdamError(&idamerrorstack, CODEERRORTYPE, "equimap extractData", err,
                                 "IDAM data handle not found when expected!");
                    return err;
                }

                if ((err = xdatand(whichName("P"), shot_str, &handle, &rank, &order, &ndata, &shape, &data, &dim)) !=
                    0)
                    break;

                if ((err = xdatainterval(rank, order, ndata, shape, dim, tslice, window, &target1, &target2)) != 0) {
                    idamLog(LOG_ERROR, "The requested Time %e could not be located in the data array!\n", tslice);
                    break;
                }

                idamLog(LOG_DEBUG, "[%d] P Profile\n", handle);
                idamLog(LOG_DEBUG, "P: target = %d, shape[0] = %d, shape[1] = %d, order = %d\n", target1, shape[0],
                        shape[1], order);

                if (order == 0) {        // array[nr][nt]
                    nt = shape[0];
                    nr = shape[1];
                } else {
                    nt = shape[1];        // array[nt][nr]
                    nr = shape[0];
                }

                efitdata->p = (float*) malloc(nr * sizeof(float));

                if (order == 0) {        // array[nr][nt]
                    for (i = 0; i < nr; i++) {
                        offset = i * nt + target1;
                        efitdata->p[i] = data[offset];
                    }
                } else {            // array[nt][nr]
                    for (i = 0; i < nr; i++) {
                        offset = target1 * nr + i;
                        efitdata->p[i] = data[offset];
                    }
                }

                free((void*) shape);
                free((void*) data);
                free((void*) dim);

// F Profile	f[normalised poloidal flux][t]

                if ((handle = whichHandle("F")) < 0) {
                    err = 999;
                    addIdamError(&idamerrorstack, CODEERRORTYPE, "equimap extractData", err,
                                 "IDAM data handle not found when expected!");
                    return err;
                }

                if ((err = xdatand(whichName("F"), shot_str, &handle, &rank, &order, &ndata, &shape, &data, &dim)) !=
                    0)
                    break;

                idamLog(LOG_DEBUG, "[%d] F Profile\n", handle);
                idamLog(LOG_DEBUG, "F: target = %d, shape[0] = %d, shape[1] = %d, order = %d\n", target1, shape[0],
                        shape[1], order);

                if (order == 0) {        // array[nr][nt]
                    nt = shape[0];
                    nr = shape[1];
                } else {
                    nt = shape[1];        // array[nt][nr]
                    nr = shape[0];
                }

                efitdata->f = (float*) malloc(nr * sizeof(float));

                if (order == 0) {        // array[nr][nt]
                    for (i = 0; i < nr; i++) {
                        offset = i * nt + target1;
                        efitdata->f[i] = data[offset];
                    }
                } else {            // array[nt][nr]
                    for (i = 0; i < nr; i++) {
                        offset = target1 * nr + i;
                        efitdata->f[i] = data[offset];
                    }
                }

                free((void*) shape);
                free((void*) data);
                free((void*) dim);

// P_Prime Profile	[normalised poloidal flux][t]

                if ((handle = whichHandle("PPrime")) < 0) {
                    err = 999;
                    addIdamError(&idamerrorstack, CODEERRORTYPE, "equimap extractData", err,
                                 "IDAM data handle not found when expected!");
                    return err;
                }

                if ((err = xdatand(whichName("PPrime"), shot_str, &handle, &rank, &order, &ndata, &shape, &data,
                                   &dim)) != 0)
                    break;

                idamLog(LOG_DEBUG, "[%d] PPrime Profile\n", handle);
                idamLog(LOG_DEBUG, "PPrime: target = %d, shape[0] = %d, shape[1] = %d, order = %d\n", target1,
                        shape[0], shape[1], order);

                if (order == 0) {        // array[nr][nt]
                    nt = shape[0];
                    nr = shape[1];
                } else {
                    nt = shape[1];        // array[nt][nr]
                    nr = shape[0];
                }

                efitdata->pprime = (float*) malloc(nr * sizeof(float));

                if (order == 0) {        // array[nr][nt]
                    for (i = 0; i < nr; i++) {
                        offset = i * nt + target1;
                        efitdata->pprime[i] = data[offset];
                    }
                } else {            // array[nt][nr]
                    for (i = 0; i < nr; i++) {
                        offset = target1 * nr + i;
                        efitdata->pprime[i] = data[offset];
                    }
                }

                free((void*) shape);
                free((void*) data);
                free((void*) dim);

// FF_Prime		[normalised poloidal flux][t]

                if ((handle = whichHandle("FFPrime")) < 0) {
                    err = 999;
                    addIdamError(&idamerrorstack, CODEERRORTYPE, "equimap extractData", err,
                                 "IDAM data handle not found when expected!");
                    return err;
                }

                if ((err = xdatand(whichName("FFPrime"), shot_str, &handle, &rank, &order, &ndata, &shape, &data,
                                   &dim)) != 0)
                    break;

                idamLog(LOG_DEBUG, "[%d] FFPrime Profile\n", handle);
                idamLog(LOG_DEBUG, "FFPrime: target = %d, shape[0] = %d, shape[1] = %d, order = %d\n", target1,
                        shape[0], shape[1], order);

                if (order == 0) {        // array[nr][nt]
                    nt = shape[0];
                    nr = shape[1];
                } else {
                    nt = shape[1];        // array[nt][nr]
                    nr = shape[0];
                }

                efitdata->ffprime = (float*) malloc(nr * sizeof(float));

                if (order == 0) {        // array[nr][nt]
                    for (i = 0; i < nr; i++) {
                        offset = i * nt + target1;
                        efitdata->ffprime[i] = data[offset];
                    }
                } else {            // array[nt][nr]
                    for (i = 0; i < nr; i++) {
                        offset = target1 * nr + i;
                        efitdata->ffprime[i] = data[offset];
                    }
                }

                free((void*) shape);
                free((void*) data);
                free((void*) dim);

// Elongation		[normalised poloidal flux][t]

                if ((handle = whichHandle("ElongPsi")) < 0) {
                    err = 999;
                    addIdamError(&idamerrorstack, CODEERRORTYPE, "equimap extractData", err,
                                 "IDAM data handle not found when expected!");
                    return err;
                }

                if ((err = xdatand(whichName("ElongPsi"), shot_str, &handle, &rank, &order, &ndata, &shape, &data,
                                   &dim)) != 0)
                    break;

                idamLog(LOG_DEBUG, "[%d] ElongPsi Profile\n", handle);
                idamLog(LOG_DEBUG, "ElongPsi: target = %d, shape[0] = %d, shape[1] = %d, order = %d\n", target1,
                        shape[0], shape[1], order);

                if (order == 0) {        // array[nr][nt]
                    nt = shape[0];
                    nr = shape[1];
                } else {
                    nt = shape[1];        // array[nt][nr]
                    nr = shape[0];
                }

                efitdata->elongp = (float*) malloc(nr * sizeof(float));

                if (order == 0) {        // array[nr][nt]
                    for (i = 0; i < nr; i++) {
                        offset = i * nt + target1;
                        efitdata->elongp[i] = data[offset];
                    }
                } else {            // array[nt][nr]
                    for (i = 0; i < nr; i++) {
                        offset = target1 * nr + i;
                        efitdata->elongp[i] = data[offset];
                    }
                }

                free((void*) shape);
                free((void*) data);
                free((void*) dim);

// Lower Triagularity		[normalised poloidal flux][t]

                if ((handle = whichHandle("TriangLPsi")) < 0) {
                    err = 999;
                    addIdamError(&idamerrorstack, CODEERRORTYPE, "equimap extractData", err,
                                 "IDAM data handle not found when expected!");
                    return err;
                }

                if ((err = xdatand(whichName("TriangLPsi"), shot_str, &handle, &rank, &order, &ndata, &shape, &data,
                                   &dim)) != 0)
                    break;

                idamLog(LOG_DEBUG, "[%d] TriangLPsi Profile\n", handle);
                idamLog(LOG_DEBUG, "TriangLPsi: target = %d, shape[0] = %d, shape[1] = %d, order = %d\n", target1,
                        shape[0], shape[1], order);

                if (order == 0) {        // array[nr][nt]
                    nt = shape[0];
                    nr = shape[1];
                } else {
                    nt = shape[1];        // array[nt][nr]
                    nr = shape[0];
                }

                efitdata->trianglp = (float*) malloc(nr * sizeof(float));

                if (order == 0) {
                    for (i = 0; i < nr; i++) {
                        offset = i * nt + target1;
                        efitdata->trianglp[i] = data[offset];
                    }
                } else {
                    for (i = 0; i < nr; i++) {
                        offset = target1 * nr + i;
                        efitdata->trianglp[i] = data[offset];
                    }
                }

                free((void*) shape);
                free((void*) data);
                free((void*) dim);

// Upper Triagularity		[normalised poloidal flux][t]

                if ((handle = whichHandle("TriangUPsi")) < 0) {
                    err = 999;
                    addIdamError(&idamerrorstack, CODEERRORTYPE, "equimap extractData", err,
                                 "IDAM data handle not found when expected!");
                    return err;
                }

                if ((err = xdatand(whichName("TriangUPsi"), shot_str, &handle, &rank, &order, &ndata, &shape, &data,
                                   &dim)) != 0)
                    break;

                idamLog(LOG_DEBUG, "[%d] TriangUPsi Profile\n", handle);
                idamLog(LOG_DEBUG, "TriangUPsi: target = %d, shape[0] = %d, shape[1] = %d, order = %d\n", target1,
                        shape[0], shape[1], order);

                if (order == 0) {        // array[nr][nt]
                    nt = shape[0];
                    nr = shape[1];
                } else {
                    nt = shape[1];        // array[nt][nr]
                    nr = shape[0];
                }

                efitdata->triangup = (float*) malloc(nr * sizeof(float));

                if (order == 0) {
                    for (i = 0; i < nr; i++) {
                        offset = i * nt + target1;
                        efitdata->triangup[i] = data[offset];
                    }
                } else {
                    for (i = 0; i < nr; i++) {
                        offset = target1 * nr + i;
                        efitdata->triangup[i] = data[offset];
                    }
                }

                free((void*) shape);
                free((void*) data);
                free((void*) dim);

// Volume		[normalised poloidal flux][t]

                if ((handle = whichHandle("VolPsi")) < 0) {
                    err = 999;
                    addIdamError(&idamerrorstack, CODEERRORTYPE, "equimap extractData", err,
                                 "IDAM data handle not found when expected!");
                    return err;
                }

                if ((err = xdatand(whichName("VolPsi"), shot_str, &handle, &rank, &order, &ndata, &shape, &data,
                                   &dim)) != 0)
                    break;

                idamLog(LOG_DEBUG, "[%d] VolPsi Profile\n", handle);
                idamLog(LOG_DEBUG, "VolPsi: target = %d, shape[0] = %d, shape[1] = %d, order = %d\n", target1,
                        shape[0], shape[1], order);

                if (order == 0) {        // array[nr][nt]
                    nt = shape[0];
                    nr = shape[1];
                } else {
                    nt = shape[1];        // array[nt][nr]
                    nr = shape[0];
                }

                efitdata->volp = (float*) malloc(nr * sizeof(float));

                if (order == 0) {
                    for (i = 0; i < nr; i++) {
                        offset = i * nt + target1;
                        efitdata->volp[i] = data[offset];
                    }
                } else {
                    for (i = 0; i < nr; i++) {
                        offset = target1 * nr + i;
                        efitdata->volp[i] = data[offset];
                    }
                }

                free((void*) shape);
                free((void*) data);
                free((void*) dim);

// Area [normalised poloidal flux][t]

                if ((handle = whichHandle("AreaPsi")) < 0) {
                    err = 999;
                    addIdamError(&idamerrorstack, CODEERRORTYPE, "equimap extractData", err,
                                 "IDAM data handle not found when expected!");
                    return err;
                }

                if ((err = xdatand(whichName("AreaPsi"), shot_str, &handle, &rank, &order, &ndata, &shape, &data,
                                   &dim)) != 0)
                    break;

                idamLog(LOG_DEBUG, "[%d] AreaPsi Profile\n", handle);
                idamLog(LOG_DEBUG, "AreaPsi: target = %d, shape[0] = %d, shape[1] = %d, order = %d\n", target1,
                        shape[0], shape[1], order);

                if (order == 0) {        // array[nr][nt]
                    nt = shape[0];
                    nr = shape[1];
                } else {
                    nt = shape[1];        // array[nt][nr]
                    nr = shape[0];
                }

                efitdata->areap = (float*) malloc(nr * sizeof(float));

                if (order == 0) {
                    for (i = 0; i < nr; i++) {
                        offset = i * nt + target1;
                        efitdata->areap[i] = data[offset];
                    }
                } else {
                    for (i = 0; i < nr; i++) {
                        offset = target1 * nr + i;
                        efitdata->areap[i] = data[offset];
                    }
                }

                free((void*) shape);
                free((void*) data);
                free((void*) dim);

            }

// Poloidal Flux Surface - 3D time dependent array 		psi[R][Z][t]

            if ((handle = whichHandle("efm_psi(r,z)")) < 0) {
                err = 999;
                addIdamError(&idamerrorstack, CODEERRORTYPE, "equimap extractData", err,
                             "IDAM data handle not found when expected!");
                return err;
            }

            if ((err = xdatand("efm_psi(r,z)", shot_str, &handle, &rank, &order, &ndata, &shape, &data, &dim)) !=
                0)
                break;

            if ((err = xdatainterval(rank, order, ndata, shape, dim, tslice, window, &target1, &target2)) != 0) {
                idamLog(LOG_ERROR, "The requested Time %e could not be located in the data array!\n", tslice);
                break;
            }

            if (order == 0) {        // array[nz][nr][nt]
                nt = shape[0];
                nr = shape[1];
                nz = shape[2];
            } else {
                if (order == 1) {        // array[nz][nt][nr]
                    nr = shape[0];
                    nt = shape[1];
                    nz = shape[2];
                } else {
                    nr = shape[0];        // array[nt][nz][nr]
                    nz = shape[1];
                    nt = shape[2];
                }
            }

            float** psig = (float**) malloc(nz * sizeof(float*));        // Psi[z][r]
            for (i = 0; i < nz; i++) psig[i] = (float*) malloc(nr * sizeof(float));

            float* rgrid = (float*) malloc(nr * sizeof(float));        // Psi Fixed Grid
            float* zgrid = (float*) malloc(nz * sizeof(float));

            float* psiz0 = (float*) malloc((nr + 3) * sizeof(float));        // Mid-plane psi[r]
            float* rz0 = (float*) malloc(
                    (nr + 3) * sizeof(float));        // + 3 to Add magnetic axis and 2 Boundary points

            int altHandleR, altHandleZ;

            if (isAltData) {
                if (nt > 1) {
                    err = 999;
                    addIdamError(&idamerrorstack, CODEERRORTYPE, "equimap extractData", err,
                                 "Can only work with single time slices when ALT data selected! -- time dependent psi map coordinates");
                    return err;
                }
                if (nr != nz) {
                    err = 999;
                    addIdamError(&idamerrorstack, CODEERRORTYPE, "equimap extractData", err,
                                 "Not a Square profile grid!");
                    return err;
                }
                if ((altHandleR = whichHandle("profile-R")) < 0 || (altHandleZ = whichHandle("profile-Z")) < 0) {
                    err = 999;
                    addIdamError(&idamerrorstack, CODEERRORTYPE, "equimap extractData", err,
                                 "IDAM data handle not found when expected!");
                    return err;
                }
                getIdamFloatData(altHandleR, rgrid);
                getIdamFloatData(altHandleZ, zgrid);
            }

            if (order == 0) {        // array[nz][nr][nt]
                for (j = 0; j < nz; j++) {
                    for (i = 0; i < nr; i++) {
                        offset = i * nr + target1 + j * nt * nr;
                        psig[j][i] = data[offset];
                    }
                }
                if (!isAltData) {
                    getIdamFloatDimData(handle, 1, rgrid);
                    getIdamFloatDimData(handle, 2, zgrid);
                }
                getIdamFloatDimData(handle, 1, rz0);
            } else {
                if (order == 1) {        // array[nz][nt][nr]
                    for (j = 0; j < nz; j++) {
                        for (i = 0; i < nr; i++) {
                            offset = target1 * nt + i + j * nt * nr;
                            psig[j][i] = data[offset];
                        }
                    }
                    if (!isAltData) {
                        getIdamFloatDimData(handle, 0, rgrid);
                        getIdamFloatDimData(handle, 2, zgrid);
                    }
                    getIdamFloatDimData(handle, 0, rz0);
                } else {            // array[nt][nz][nr]
                    for (j = 0; j < nz; j++) {
                        for (i = 0; i < nr; i++) {
                            offset = j * nr + i + target1 * nr * nz;
                            psig[j][i] = data[offset];
                        }
                    }
                    if (!isAltData) {
                        getIdamFloatDimData(handle, 0, rgrid);
                        getIdamFloatDimData(handle, 1, zgrid);
                    }
                    getIdamFloatDimData(handle, 0, rz0);
                }
            }

            for (i = 0; i < nr; i++)
                psiz0[i] = psig[nz / 2][i];        // Flux on Mid-Plane (not passing through magnetic axis)

            efitdata->psig = psig;                // Flux Map
            efitdata->rgrid = rgrid;
            efitdata->zgrid = zgrid;
            efitdata->psiCount[0] = nr;
            efitdata->psiCount[1] = nz;

// Remove most of the redundant data outside the plasma boundary

            int rz0Count = 0, rz00Count = 0;
            float* rz00 = (float*) malloc((nr + 3) * sizeof(float));
            float* psiz00 = (float*) malloc((nr + 3) * sizeof(float));

            for (i = 0; i < nr; i++) {
                if (rz0[i] >= 0.9 * efitdata->Rmin) {            // *** Creates Ragged arrays in time !!!
                    psiz00[rz00Count] = psiz0[i];
                    rz00[rz00Count++] = rz0[i];
                }
            }

            for (i = 0; i < rz00Count; i++) {
                if (rz00[i] <= 1.1 * efitdata->Rmax) {
                    psiz0[rz0Count] = psiz00[i];            // Mid-Plane poloidal flux
                    rz0[rz0Count++] = rz00[i];            // Reduced radial grid
                }
            }

            free((void*) rz00);
            free((void*) psiz00);

// Linearly Interpolate across the mid-plane for the Magnetic Axis flux value

            j = -1;
            for (i = 0; i < rz0Count - 1; i++) {
                if (rz0[i] < efitdata->rmag && efitdata->rmag < rz0[i + 1]) {
                    j = i;
                    break;
                }
            }
            if (j >= 0) {

                float psimagnetic = (psiz0[j + 1] - psiz0[j]) / (rz0[j + 1] - rz0[j]) * (rz0[j + 1] - efitdata->rmag) +
                                    psiz0[j + 1];

                k = rz0Count;
                for (i = j + 2; i <= rz0Count; i++) {
                    rz0[k] = rz0[k - 1];
                    psiz0[k] = psiz0[k - 1];
                    k--;
                }
                rz0[j + 1] = efitdata->rmag;        // Not a fixed grid position
                psiz0[j + 1] = psimagnetic;
                rz0Count++;
            }

// Add the Inner Edge flux point

            j = -1;
            for (i = 0; i < rz0Count - 1; i++) {
                if (rz0[i] < efitdata->Rmin && efitdata->Rmin < rz0[i + 1]) {
                    j = i;
                    break;
                }
            }
            if (j >= 0) {
                k = rz0Count;
                for (i = j + 2; i <= rz0Count; i++) {
                    rz0[k] = rz0[k - 1];
                    psiz0[k] = psiz0[k - 1];
                    k--;
                }
                rz0[j + 1] = efitdata->Rmin;
                psiz0[j + 1] = efitdata->psi_bnd;
                rz0Count++;
            }
            if (j == -1) {
                k = rz0Count;
                for (i = 1; i <= rz0Count; i++) {
                    rz0[k] = rz0[k - 1];
                    psiz0[k] = psiz0[k - 1];
                    k--;
                }
                rz0[0] = efitdata->Rmin;            // Not a fixed grid position
                psiz0[0] = efitdata->psi_bnd;
                rz0Count++;
            }

// Add the Outer Edge flux point

            j = -1;
            for (i = 0; i < rz0Count - 1; i++) {
                if (rz0[i] < efitdata->Rmax && efitdata->Rmax < rz0[i + 1]) {
                    j = i;
                    break;
                }
            }
            if (j >= 0) {
                k = rz0Count;
                for (i = j + 2; i <= rz0Count; i++) {
                    rz0[k] = rz0[k - 1];
                    psiz0[k] = psiz0[k - 1];
                    k--;
                }
                rz0[j + 1] = efitdata->Rmax;
                psiz0[j + 1] = efitdata->psi_bnd;
                rz0Count++;
            }
            if (j == -1) {
                rz0[rz0Count] = efitdata->Rmax;        // Not a fixed grid position
                psiz0[rz0Count] = efitdata->psi_bnd;
                rz0Count++;
            }

            efitdata->psiz0 = psiz0;
            efitdata->rz0 = rz0;        // this is a ragged (and non regularly gridded) array in time !
            efitdata->rz0Count = rz0Count;

            free((void*) shape);
            free((void*) data);
            free((void*) dim);

            idamLog(LOG_DEBUG, "[%d] Poloidal Flux Surface\n", handle);
            idamLog(LOG_DEBUG, "Mid-Plane Poloidal Flux: Count = %d, Major Radius, Poloidal Flux\n", rz0Count);
            for (i = 0; i < rz0Count; i++)
                idamLog(LOG_DEBUG, "[%d] %f   %f \n", i, efitdata->rz0[i], efitdata->psiz0[i]);

// Toroidal current density - 3D time dependent array 		Jphi[R][Z][t]

            if ((handle = whichHandle("efm_plasma_curr(r,z)")) < 0) {
                err = 999;
                addIdamError(&idamerrorstack, CODEERRORTYPE, "equimap extractData", err,
                             "IDAM data handle not found when expected!");
                return err;
            }

            if ((err = xdatand("efm_plasma_curr(r,z)", shot_str, &handle, &rank, &order, &ndata, &shape, &data,
                               &dim)) != 0)
                break;

            if ((err = xdatainterval(rank, order, ndata, shape, dim, tslice, window, &target1, &target2)) != 0) {
                idamLog(LOG_ERROR, "The requested Time %e could not be located in the data array!\n", tslice);
                break;
            }

            if (order == 0) {        // array[nz][nr][nt]
                nt = shape[0];
                nr = shape[1];
                nz = shape[2];
            } else {
                if (order == 1) {        // array[nz][nt][nr]
                    nr = shape[0];
                    nt = shape[1];
                    nz = shape[2];
                } else {
                    nr = shape[0];        // array[nt][nz][nr]
                    nz = shape[1];
                    nt = shape[2];
                }
            }

            float** jphi = (float**) malloc(nz * sizeof(float*));        // Jphi[z][r]
            for (i = 0; i < nz; i++) jphi[i] = (float*) malloc(nr * sizeof(float));

            if (order == 0) {        // array[nz][nr][nt]
                for (j = 0; j < nz; j++) {
                    for (i = 0; i < nr; i++) {
                        offset = i * nr + target1 + j * nt * nr;
                        jphi[j][i] = data[offset];
                    }
                }
            } else {
                if (order == 1) {        // array[nz][nt][nr]
                    for (j = 0; j < nz; j++) {
                        for (i = 0; i < nr; i++) {
                            offset = target1 * nt + i + j * nt * nr;
                            jphi[j][i] = data[offset];
                        }
                    }
                } else {            // array[nt][nz][nr]
                    for (j = 0; j < nz; j++) {
                        for (i = 0; i < nr; i++) {
                            offset = j * nr + i + target1 * nr * nz;
                            jphi[j][i] = data[offset];
                        }
                    }
                }
            }

            free((void*) shape);
            free((void*) data);
            free((void*) dim);

            efitdata->Jphi = jphi;

//----------------------------------------------------------------------------------------------------------------
// Calculate (Normalised) Toroidal Flux Profile + All equivalent flux labels associated with efit data

            float* psi = (float*) malloc(efitdata->qCount * sizeof(float));        // poloidal flux
            float* phi = (float*) malloc(efitdata->qCount * sizeof(float));        // Toroidal flux

            float* rho = (float*) malloc(
                    efitdata->qCount * sizeof(float));    // EFIT Flux Label: Normalised poloidal Flux
            float* trho = (float*) malloc(
                    efitdata->qCount * sizeof(float));    // TRANSP Flux Label: Normalised toroidal Flux
            float* rho_tor = (float*) malloc(efitdata->qCount * sizeof(float));    // ITM Flux label (Not Normalised)

            for (i = 0; i < efitdata->qCount; i++) {
                rho[i] = (float) i / (float) (efitdata->qCount - 1);                    // Efit flux label
                psi[i] = efitdata->psi_mag + rho[i] * (efitdata->psi_bnd - efitdata->psi_mag);    // Poloidal Flux
                phi[i] = fabs(lineInt(efitdata->q, psi, i + 1));                    // Toroidal Flux
            }

            trho[0] = 0.0;
            for (i = 1; i < efitdata->qCount; i++)
                trho[i] = sqrt(phi[i] / phi[efitdata->qCount - 1]);    // TRANSP Flux Label

            float rho_torb = sqrt(
                    phi[efitdata->qCount - 1] / M_PI / fabsf(efitdata->bvac));            // ITM Flux Radius at boundary
            for (i = 0; i < efitdata->qCount; i++)
                rho_tor[i] = sqrt(phi[i] / M_PI / fabsf(efitdata->bvac)) / rho_torb;    // Normalised ITM Flux Label

            efitdata->rho = rho;
            efitdata->psi = psi;
            efitdata->phi = phi;
            efitdata->trho = trho;
            efitdata->rho_torb = rho_torb;
            efitdata->rho_tor = rho_tor;

            idamLog(LOG_DEBUG, "Flux Coordinates of the Q Profile: rho, psi, phi, trho, rho_tor\n");
            for (i = 0; i < efitdata->qCount; i++)
                idamLog(LOG_DEBUG, "[%3d] %f   %f   %f   %f   %f\n", i, rho[i], psi[i], phi[i], trho[i], rho_tor[i]);

//----------------------------------------------------------------------------------------------------------------
// Magnetic field components: Bz = 1/R dpsi/dR, Br = -1/R dpsi/dZ, Bphi = F/R

            if (equimapdata->readITMData) {

                psig = efitdata->psig;                // Flux Map
                rgrid = efitdata->rgrid;
                zgrid = efitdata->zgrid;
                nr = efitdata->psiCount[0];
                nz = efitdata->psiCount[1];

                float** dpsidr = (float**) malloc(nz * sizeof(float*));
                float** dpsidz = (float**) malloc(nz * sizeof(float*));

                for (i = 0; i < nz; i++) {
                    dpsidr[i] = (float*) malloc(nr * sizeof(float));
                    dpsidz[i] = (float*) malloc(nr * sizeof(float));
                }

// Centred derivatives

                float g1, g2;

                for (i = 0; i < nz; i++) {
                    dpsidr[i][0] = (psig[i][1] - psig[i][0]) / (rgrid[1] - rgrid[0]);
                    for (j = 1; j < nr - 1; j++) {
                        g1 = (psig[i][j] - psig[i][j - 1]) / (rgrid[j] - rgrid[j - 1]);
                        g2 = (psig[i][j + 1] - psig[i][j]) / (rgrid[j + 1] - rgrid[j]);
                        dpsidr[i][j] = 0.5 * (g1 + g2);
                    }
                    dpsidr[i][nr - 1] = (psig[i][nr - 1] - psig[i][nr - 2]) / (rgrid[nr - 1] - rgrid[nr - 2]);
                }

                for (j = 0; j < nr; j++) {
                    for (i = 1; i < nz - 1; i++) {
                        g1 = (psig[i][j] - psig[i - 1][j]) / (zgrid[i] - zgrid[i - 1]);
                        g2 = (psig[i + 1][j] - psig[i][j]) / (zgrid[i + 1] - zgrid[i]);
                        dpsidz[i][j] = 0.5 * (g1 + g2);
                    }
                }

// R,Z Field Components

                float** Br = (float**) malloc(nz * sizeof(float*));
                float** Bz = (float**) malloc(nz * sizeof(float*));

                float fluxCorrection = 1.0; // 2.0*M_PI;	// Flux per radian

                for (i = 0; i < nz; i++) {
                    Br[i] = (float*) malloc(nr * sizeof(float));
                    Bz[i] = (float*) malloc(nr * sizeof(float));
                    for (j = 0; j < nr; j++) {
                        Br[i][j] = -fluxCorrection * dpsidz[i][j] / rgrid[j];
                        Bz[i][j] = fluxCorrection * dpsidr[i][j] / rgrid[j];
                    }
                }

                for (i = 0; i < nz; i++) {
                    free((void*) dpsidr[i]);
                    free((void*) dpsidz[i]);
                }
                free((void*) dpsidr);
                free((void*) dpsidz);

                efitdata->Br = Br;
                efitdata->Bz = Bz;

// Toroidal Field Component

                float** Bphi = (float**) malloc(nz * sizeof(float*));
                for (i = 0; i < nz; i++) {
                    Bphi[i] = (float*) malloc(nr * sizeof(float));
                    for (j = 0; j < nr; j++)
                        Bphi[i][j] = efitdata->bvac * efitdata->rvac / rgrid[j];    // vacuum Field
                }

                float Dmag, dmag, dmin, psirz, grad, frz;
                int kmin;
                float* dlcfs = (float*) malloc(efitdata->nlcfs * sizeof(float));
                for (i = 0; i < nz; i++) {
                    for (j = 0; j < nr; j++) {
                        Dmag = sqrt(pow((efitdata->rmag - rgrid[j]), 2.0) + pow((efitdata->zmag - zgrid[i]), 2.0));
                        for (k = 0; k < efitdata->nlcfs; k++)
                            dlcfs[k] = sqrt(pow((efitdata->rlcfs[k] - rgrid[j]), 2.0) +
                                            pow((efitdata->zlcfs[k] - zgrid[i]), 2.0));
                        kmin = -1;
                        dmin = 1.0E20;
                        for (k = 0; k < efitdata->nlcfs; k++) {
                            if (dlcfs[k] < dmin) {
                                dmin = dlcfs[k];        // Minimum distance from the LCFS to the grid point
                                kmin = k;
                            }
                        }
                        dmag = sqrt(pow((efitdata->rmag - efitdata->rlcfs[kmin]), 2.0)
                                    + pow((efitdata->zmag - efitdata->zlcfs[kmin]),
                                          2.0));        // distance from magnetic axis
                        if (dmag < Dmag) continue;    // Outside plasma

                        psirz = psig[i][j];
                        kmin = -1;
                        for (k = 1; k < efitdata->qCount; k++) {
                            if (psirz <= efitdata->psi[k - 1] && psirz > efitdata->psi[k]) {
                                kmin = k - 1;
                                break;
                            }
                        }
                        if (kmin < 0) continue;        // Outside plasma

                        grad = (efitdata->f[kmin + 1] - efitdata->f[kmin]) /
                               (efitdata->psi[kmin + 1] - efitdata->psi[kmin]);
                        frz = grad * (psirz - efitdata->psi[kmin]) + efitdata->f[kmin];    // Linear interpolate for F

                        Bphi[i][j] = frz / rgrid[j];

                    }
                }

                free((void*) dlcfs);

                efitdata->Bphi = Bphi;
            }

//--------------------------------------------------------------------------------------------------------------
// Map Psi Profile onto target fixed grid (Linear interpolation)

            float* mappsi = (float*) malloc(equimapdata->rhoCount * sizeof(float));
            float* mappsiB = (float*) malloc(equimapdata->rhoBCount * sizeof(float));

            err1 = 0;
            err2 = 0;

            switch (equimapdata->rhoType) {
                case (SQRTNORMALISEDTOROIDALFLUX): {

                    //              TO                                       FROM                              Input               Output

                    err1 = xdatamap(equimapdata->rhoCount, equimapdata->rho, efitdata->qCount, efitdata->trho,
                                    efitdata->psi, 0.0, mappsi);
                    err2 = xdatamap(equimapdata->rhoBCount, equimapdata->rhoB, efitdata->qCount, efitdata->trho,
                                    efitdata->psi, 0.0, mappsiB);
                    break;
                }
                case (NORMALISEDPOLOIDALFLUX): {
                    err1 = xdatamap(equimapdata->rhoCount, equimapdata->rho, efitdata->qCount, efitdata->rho,
                                    efitdata->psi, 0.0, mappsi);
                    err2 = xdatamap(equimapdata->rhoBCount, equimapdata->rhoB, efitdata->qCount, efitdata->rho,
                                    efitdata->psi, 0.0, mappsiB);
                    break;
                }
                case (NORMALISEDITMFLUXRADIUS): {
                    err1 = xdatamap(equimapdata->rhoCount, equimapdata->rho, efitdata->qCount, efitdata->rho_tor,
                                    efitdata->psi, 0.0, mappsi);
                    err2 = xdatamap(equimapdata->rhoBCount, equimapdata->rhoB, efitdata->qCount, efitdata->rho_tor,
                                    efitdata->psi, 0.0, mappsiB);
                    break;
                }
                default: {
                    err1 = 1;
                    err2 = 1;
                    break;
                }
            }

            if (err1 != 0 || err2 != 0) {
                err = 1;
                addIdamError(&idamerrorstack, CODEERRORTYPE, "equimap extractData", err,
                             "Unable to Map the Poloidal Flux!");
                return err;
            }

            mappsiB[0] = efitdata->psi_mag;
            mappsiB[equimapdata->rhoBCount - 1] = efitdata->psi_bnd;

            efitdata->mappsi = mappsi;
            efitdata->mappsiB = mappsiB;

// Test the mapped psi is within the known range of values

            if (efitdata->psi_mag <= efitdata->psi_bnd) {
                for (k = 0; k < equimapdata->rhoCount; k++) {
                    if ((efitdata->mappsi[k] - efitdata->psi_mag) < FLT_EPSILON ||
                        (efitdata->mappsi[k] - efitdata->psi_bnd) > FLT_EPSILON) {
                        err = 999;
                        addIdamError(&idamerrorstack, CODEERRORTYPE, "equimap extractData", err,
                                     "Poloidal Flux mapping error A!");
                        return err;
                    }
                }
                for (k = 1; k < equimapdata->rhoBCount - 1; k++) {
                    if ((efitdata->mappsiB[k] - efitdata->psi_mag) < FLT_EPSILON ||
                        (efitdata->mappsiB[k] - efitdata->psi_bnd) > FLT_EPSILON) {
                        err = 999;
                        addIdamError(&idamerrorstack, CODEERRORTYPE, "equimap extractData", err,
                                     "Poloidal Flux mapping error A!");
                        return err;
                    }
                }
            } else {
                for (k = 0; k < equimapdata->rhoCount; k++) {
                    if ((efitdata->mappsi[k] - efitdata->psi_mag) > FLT_EPSILON ||
                        (efitdata->mappsi[k] - efitdata->psi_bnd) < FLT_EPSILON) {
                        err = 999;
                        addIdamError(&idamerrorstack, CODEERRORTYPE, "equimap extractData", err,
                                     "Poloidal Flux mapping error B!");
                        return err;
                    }
                }
                for (k = 1; k < equimapdata->rhoBCount - 1; k++) {
                    if ((efitdata->mappsiB[k] - efitdata->psi_mag) > FLT_EPSILON ||
                        (efitdata->mappsiB[k] - efitdata->psi_bnd) < FLT_EPSILON) {
                        err = 999;
                        addIdamError(&idamerrorstack, CODEERRORTYPE, "equimap extractData", err,
                                     "Poloidal Flux mapping error B!");
                        return err;
                    }
                }
            }

//--------------------------------------------------------------------------------------------------------------
// Map Q Profile onto target fixed grid

            float* mapq = (float*) malloc(equimapdata->rhoCount * sizeof(float));
            float* mapqB = (float*) malloc(equimapdata->rhoBCount * sizeof(float));

            err1 = 0;
            err2 = 0;

            switch (equimapdata->rhoType) {
                case (SQRTNORMALISEDTOROIDALFLUX): {
                    err1 = xdatamap(equimapdata->rhoCount, equimapdata->rho, efitdata->qCount, efitdata->trho,
                                    efitdata->q, 0.0, mapq);
                    err2 = xdatamap(equimapdata->rhoBCount, equimapdata->rhoB, efitdata->qCount, efitdata->trho,
                                    efitdata->q, 0.0, mapqB);
                    break;
                }
                case (NORMALISEDPOLOIDALFLUX): {
                    err1 = xdatamap(equimapdata->rhoCount, equimapdata->rho, efitdata->qCount, efitdata->rho,
                                    efitdata->q,
                                    0.0, mapq);
                    err2 = xdatamap(equimapdata->rhoBCount, equimapdata->rhoB, efitdata->qCount, efitdata->rho,
                                    efitdata->q, 0.0, mapqB);
                    break;
                }
                case (NORMALISEDITMFLUXRADIUS): {
                    err1 = xdatamap(equimapdata->rhoCount, equimapdata->rho, efitdata->qCount, efitdata->rho_tor,
                                    efitdata->q, 0.0, mapq);
                    err2 = xdatamap(equimapdata->rhoBCount, equimapdata->rhoB, efitdata->qCount, efitdata->rho_tor,
                                    efitdata->q, 0.0, mapqB);
                    break;
                }
                default: {
                    err1 = 1;
                    err2 = 1;
                    break;
                }
            }

            if (err1 != 0 || err2 != 0) {
                err = 1;
                addIdamError(&idamerrorstack, CODEERRORTYPE, "equimap extractData", err,
                             "Unable to Map the Q Profile!");
                return err;
            }

            mapqB[0] = efitdata->q[0];
            mapqB[equimapdata->rhoBCount - 1] = efitdata->q[efitdata->qCount - 1];

            efitdata->mapq = mapq;
            efitdata->mapqB = mapqB;

//--------------------------------------------------------------------------------------------------------------
// Map P Profile onto target fixed grid

            if (equimapdata->readITMData) {

                float* mapp = (float*) malloc(equimapdata->rhoCount * sizeof(float));
                float* mappB = (float*) malloc(equimapdata->rhoBCount * sizeof(float));

                err1 = 0;
                err2 = 0;

                switch (equimapdata->rhoType) {
                    case (SQRTNORMALISEDTOROIDALFLUX): {
                        err1 = xdatamap(equimapdata->rhoCount, equimapdata->rho, efitdata->qCount, efitdata->trho,
                                        efitdata->p, 0.0, mapp);
                        err2 = xdatamap(equimapdata->rhoBCount, equimapdata->rhoB, efitdata->qCount, efitdata->trho,
                                        efitdata->p, 0.0, mappB);
                        break;
                    }
                    case (NORMALISEDPOLOIDALFLUX): {
                        err1 = xdatamap(equimapdata->rhoCount, equimapdata->rho, efitdata->qCount, efitdata->rho,
                                        efitdata->p, 0.0, mapp);
                        err2 = xdatamap(equimapdata->rhoBCount, equimapdata->rhoB, efitdata->qCount, efitdata->rho,
                                        efitdata->p, 0.0, mappB);
                        break;
                    }
                    case (NORMALISEDITMFLUXRADIUS): {
                        err1 = xdatamap(equimapdata->rhoCount, equimapdata->rho, efitdata->qCount, efitdata->rho_tor,
                                        efitdata->p, 0.0, mapp);
                        err2 = xdatamap(equimapdata->rhoBCount, equimapdata->rhoB, efitdata->qCount, efitdata->rho_tor,
                                        efitdata->p, 0.0, mappB);
                        break;
                    }
                    default: {
                        err1 = 1;
                        err2 = 1;
                        break;
                    }
                }

                if (err1 != 0 || err2 != 0) {
                    err = 1;
                    addIdamError(&idamerrorstack, CODEERRORTYPE, "equimap extractData", err,
                                 "Unable to Map the P Profile!");
                    return err;
                }

                mappB[0] = efitdata->p[0];
                mappB[equimapdata->rhoBCount - 1] = efitdata->p[efitdata->qCount - 1];

                efitdata->mapp = mapp;
                efitdata->mappB = mappB;

//--------------------------------------------------------------------------------------------------------------
// Map F Profile onto target fixed grid

                float* mapf = (float*) malloc(equimapdata->rhoCount * sizeof(float));
                float* mapfB = (float*) malloc(equimapdata->rhoBCount * sizeof(float));

                err1 = 0;
                err2 = 0;

                switch (equimapdata->rhoType) {
                    case (SQRTNORMALISEDTOROIDALFLUX): {
                        err1 = xdatamap(equimapdata->rhoCount, equimapdata->rho, efitdata->qCount, efitdata->trho,
                                        efitdata->f, 0.0, mapf);
                        err2 = xdatamap(equimapdata->rhoBCount, equimapdata->rhoB, efitdata->qCount, efitdata->trho,
                                        efitdata->f, 0.0, mapfB);
                        break;
                    }
                    case (NORMALISEDPOLOIDALFLUX): {
                        err1 = xdatamap(equimapdata->rhoCount, equimapdata->rho, efitdata->qCount, efitdata->rho,
                                        efitdata->f, 0.0, mapf);
                        err2 = xdatamap(equimapdata->rhoBCount, equimapdata->rhoB, efitdata->qCount, efitdata->rho,
                                        efitdata->f, 0.0, mapfB);
                        break;
                    }
                    case (NORMALISEDITMFLUXRADIUS): {
                        err1 = xdatamap(equimapdata->rhoCount, equimapdata->rho, efitdata->qCount, efitdata->rho_tor,
                                        efitdata->f, 0.0, mapf);
                        err2 = xdatamap(equimapdata->rhoBCount, equimapdata->rhoB, efitdata->qCount, efitdata->rho_tor,
                                        efitdata->f, 0.0, mapfB);
                        break;
                    }
                    default: {
                        err1 = 1;
                        err2 = 1;
                        break;
                    }
                }

                if (err1 != 0 || err2 != 0) {
                    err = 1;
                    addIdamError(&idamerrorstack, CODEERRORTYPE, "equimap extractData", err,
                                 "Unable to Map the F Profile!");
                    return err;
                }

                mapfB[0] = efitdata->f[0];
                mapfB[equimapdata->rhoBCount - 1] = efitdata->f[efitdata->qCount - 1];

                efitdata->mapf = mapf;
                efitdata->mapfB = mapfB;

//--------------------------------------------------------------------------------------------------------------
// Map P_Prime Profile onto target fixed grid

                efitdata->mappprime = (float*) malloc(equimapdata->rhoCount * sizeof(float));
                efitdata->mappprimeB = (float*) malloc(equimapdata->rhoBCount * sizeof(float));

                int volumePoints = 1;        // map to equimapdata->rho grid
                err1 = xdatamapw(volumePoints, equimapdata, efitdata, efitdata->pprime, 0.0, efitdata->mappprime);

                volumePoints = 0;        // map to equimapdata->rhoB grid
                err2 = xdatamapw(volumePoints, equimapdata, efitdata, efitdata->pprime, 0.0, efitdata->mappprimeB);

                if (err1 != 0 || err2 != 0) {
                    err = 999;
                    addIdamError(&idamerrorstack, CODEERRORTYPE, "equimap extractData", err,
                                 "Unable to Map the PPrime Profile!");
                    return err;
                }

                efitdata->mappprimeB[0] = efitdata->pprime[0];
                efitdata->mappprimeB[equimapdata->rhoBCount - 1] = efitdata->pprime[efitdata->qCount - 1];

//--------------------------------------------------------------------------------------------------------------
// Map FF_Prime Profile onto target fixed grid

                efitdata->mapffprime = (float*) malloc(equimapdata->rhoCount * sizeof(float));
                efitdata->mapffprimeB = (float*) malloc(equimapdata->rhoBCount * sizeof(float));

                volumePoints = 1;
                err1 = xdatamapw(volumePoints, equimapdata, efitdata, efitdata->ffprime, 0.0, efitdata->mapffprime);

                volumePoints = 0;
                err2 = xdatamapw(volumePoints, equimapdata, efitdata, efitdata->ffprime, 0.0, efitdata->mapffprimeB);

                if (err1 != 0 || err2 != 0) {
                    err = 999;
                    addIdamError(&idamerrorstack, CODEERRORTYPE, "equimap extractData", err,
                                 "Unable to Map the FFPrime Profile!");
                    return err;
                }

                efitdata->mapffprimeB[0] = efitdata->ffprime[0];
                efitdata->mapffprimeB[equimapdata->rhoBCount - 1] = efitdata->ffprime[efitdata->qCount - 1];

//--------------------------------------------------------------------------------------------------------------
// Map Elongation Profile onto target fixed grid

                efitdata->mapelongp = (float*) malloc(equimapdata->rhoCount * sizeof(float));
                efitdata->mapelongpB = (float*) malloc(equimapdata->rhoBCount * sizeof(float));

                volumePoints = 1;
                err1 = xdatamapw(volumePoints, equimapdata, efitdata, efitdata->elongp, 1.0, efitdata->mapelongp);

                volumePoints = 0;
                err2 = xdatamapw(volumePoints, equimapdata, efitdata, efitdata->elongp, 1.0, efitdata->mapelongpB);

                if (err1 != 0 || err2 != 0) {
                    err = 999;
                    addIdamError(&idamerrorstack, CODEERRORTYPE, "equimap extractData", err,
                                 "Unable to Map the Elongation Profile!");
                    return err;
                }

                efitdata->mapelongpB[0] = efitdata->elongp[0];
                efitdata->mapelongpB[equimapdata->rhoBCount - 1] = efitdata->elongp[efitdata->qCount - 1];

//--------------------------------------------------------------------------------------------------------------
// Map Lower Triangularity Profile onto target fixed grid

                efitdata->maptrianglp = (float*) malloc(equimapdata->rhoCount * sizeof(float));
                efitdata->maptrianglpB = (float*) malloc(equimapdata->rhoBCount * sizeof(float));

                volumePoints = 1;
                err1 = xdatamapw(volumePoints, equimapdata, efitdata, efitdata->trianglp, 1.0, efitdata->maptrianglp);

                volumePoints = 0;
                err2 = xdatamapw(volumePoints, equimapdata, efitdata, efitdata->trianglp, 1.0, efitdata->maptrianglpB);

                if (err1 != 0 || err2 != 0) {
                    err = 999;
                    addIdamError(&idamerrorstack, CODEERRORTYPE, "equimap extractData", err,
                                 "Unable to Map the Lower Triangularity Profile!");
                    return err;
                }

                efitdata->maptrianglp[0] = efitdata->trianglp[0];
                efitdata->maptrianglpB[equimapdata->rhoBCount - 1] = efitdata->trianglp[efitdata->qCount - 1];

//--------------------------------------------------------------------------------------------------------------
// Map Upper Triangularity Profile onto target fixed grid

                efitdata->maptriangup = (float*) malloc(equimapdata->rhoCount * sizeof(float));
                efitdata->maptriangupB = (float*) malloc(equimapdata->rhoBCount * sizeof(float));

                volumePoints = 1;
                err1 = xdatamapw(volumePoints, equimapdata, efitdata, efitdata->triangup, 1.0, efitdata->maptriangup);

                volumePoints = 0;
                err2 = xdatamapw(volumePoints, equimapdata, efitdata, efitdata->triangup, 1.0, efitdata->maptriangupB);

                if (err1 != 0 || err2 != 0) {
                    err = 999;
                    addIdamError(&idamerrorstack, CODEERRORTYPE, "equimap extractData", err,
                                 "Unable to Map the Upper Triangularity Profile!");
                    return err;
                }

                efitdata->maptriangup[0] = efitdata->triangup[0];
                efitdata->maptriangupB[equimapdata->rhoBCount - 1] = efitdata->triangup[efitdata->qCount - 1];

//--------------------------------------------------------------------------------------------------------------
// Map Volume Profile onto target fixed grid

                efitdata->mapvolp = (float*) malloc(equimapdata->rhoCount * sizeof(float));
                efitdata->mapvolpB = (float*) malloc(equimapdata->rhoBCount * sizeof(float));

                volumePoints = 1;
                err1 = xdatamapw(volumePoints, equimapdata, efitdata, efitdata->volp, 1.0, efitdata->mapvolp);

                volumePoints = 0;
                err2 = xdatamapw(volumePoints, equimapdata, efitdata, efitdata->volp, 1.0, efitdata->mapvolpB);

                if (err1 != 0 || err2 != 0) {
                    err = 999;
                    addIdamError(&idamerrorstack, CODEERRORTYPE, "equimap extractData", err,
                                 "Unable to Map the Volume Profile!");
                    return err;
                }

                efitdata->mapvolp[0] = efitdata->volp[0];
                efitdata->mapvolpB[equimapdata->rhoBCount - 1] = efitdata->volp[efitdata->qCount - 1];

//--------------------------------------------------------------------------------------------------------------
// Map Area Profile onto target fixed grid

                efitdata->mapareap = (float*) malloc(equimapdata->rhoCount * sizeof(float));
                efitdata->mapareapB = (float*) malloc(equimapdata->rhoBCount * sizeof(float));

                volumePoints = 1;
                err1 = xdatamapw(volumePoints, equimapdata, efitdata, efitdata->areap, 1.0, efitdata->mapareap);

                volumePoints = 0;
                err2 = xdatamapw(volumePoints, equimapdata, efitdata, efitdata->areap, 1.0, efitdata->mapareapB);

                if (err1 != 0 || err2 != 0) {
                    err = 999;
                    addIdamError(&idamerrorstack, CODEERRORTYPE, "equimap extractData", err,
                                 "Unable to Map the Area Profile!");
                    return err;
                }

                efitdata->mapareap[0] = efitdata->areap[0];
                efitdata->mapareapB[equimapdata->rhoBCount - 1] = efitdata->areap[efitdata->qCount - 1];

            }

//--------------------------------------------------------------------------------------------------------------
//--------------------------------------------------------------------------------------------------------------
// Nd/YAG Plasma Density and Temperature data

            if ((handle = whichHandle("ayc_ne")) < 0) {
                err = 999;
                addIdamError(&idamerrorstack, CODEERRORTYPE, "equimap extractData", err,
                             "IDAM data handle not found when expected!");
                return err;
            }
            if ((err = xdatand("ayc_ne", shot_str, &handle, &rank, &order, &ndata, &shape, &datan, &dim)) != 0) break;

            if ((handle = whichHandle("ayc_te")) < 0) {
                err = 999;
                addIdamError(&idamerrorstack, CODEERRORTYPE, "equimap extractData", err,
                             "IDAM data handle not found when expected!");
                return err;
            }
            if ((err = xdatand("ayc_te", shot_str, &handle, &rank, &order, &ndata, NULL, &datat, NULL)) != 0) break;

            if ((handle = whichHandle("ayc_r")) < 0) {
                err = 999;
                addIdamError(&idamerrorstack, CODEERRORTYPE, "equimap extractData", err,
                             "IDAM data handle not found when expected!");
                return err;
            }
            if (equimapdata->exp_number > ATMAYCSHOT) {
                if ((err = xdatand("ayc_r", shot_str, &handle, &rank, &order, &ndata, NULL, &datar, NULL)) != 0) break;
                if ((err = xdatainterval(rank, order, ndata, shape, dim, tslice, window, &target1, &target2)) != 0) {
                    idamLog(LOG_ERROR, "The requested Time %e could not be located in the Nd/YAG Density data!\n",
                            tslice);
                    break;
                }
            } else {    // radii are time independent
                datar = (float*) getIdamData(handle);
            }

            nslices = 1;    // No Averaging

            efitdata->nne = 0;

            if (order == 0) {                    // data[r][time]: Add 3 elements for boundary and magnetic axis data
                efitdata->ne = (float*) malloc((shape[1] + 3) * sizeof(float));
                efitdata->te = (float*) malloc((shape[1] + 3) * sizeof(float));
                efitdata->rne = (float*) malloc((shape[1] + 3) * sizeof(float));
                for (i = 0; i < shape[1]; i++) {
                    offset = target1 * shape[1] + i;
                    efitdata->ne[efitdata->nne] = datan[offset];    // For time Invariance - Ignore location wrt Rmin & Rmax & ignore NaN etc.
                    efitdata->te[efitdata->nne] = datat[offset];
                    if (equimapdata->exp_number > ATMAYCSHOT) {
                        efitdata->rne[efitdata->nne] = datar[offset];
                    } else {
                        efitdata->rne[efitdata->nne] = datar[i];
                    }
                    efitdata->nne++;
                }
            } else {                            // data[time][r]
                efitdata->ne = (float*) malloc((shape[0] + 3) * sizeof(float));
                efitdata->te = (float*) malloc((shape[0] + 3) * sizeof(float));
                efitdata->rne = (float*) malloc((shape[0] + 3) * sizeof(float));
                for (i = 0; i < shape[0]; i++) {
                    offset = target1 * shape[0] + i;
                    efitdata->ne[efitdata->nne] = datan[offset];
                    efitdata->te[efitdata->nne] = datat[offset];
                    if (equimapdata->exp_number > ATMAYCSHOT) {
                        efitdata->rne[efitdata->nne] = datar[offset];
                    } else {
                        efitdata->rne[efitdata->nne] = datar[i];
                    }
                    efitdata->nne++;
                }

                free((void*) shape);
                free((void*) dim);
                if (equimapdata->exp_number > ATMAYCSHOT) free((void*) datar);
                free((void*) datan);
                free((void*) datat);

            }

            idamLog(LOG_DEBUG, "Number of Nd/YAG ne and Te Points: %d\n", efitdata->nne);
            idamLog(LOG_DEBUG, "Time index selected: %d\n", target1);
            for (i = 0; i < efitdata->nne; i++)
                idamLog(LOG_DEBUG, "[%3d]  %10.4e %10.4e %10.4e\n", i, efitdata->rne[i], efitdata->ne[i],
                        efitdata->te[i]);

// Map data from Major Radius to Normalised Poloidal or Toroidal Flux within the Plasma

// 1) ne(R) = ne(psi(R)) - Identify psi values at each measurement R location using psiz0 and rz0 data
//    No Poloidal Flux data are available for points outside the rz0 array
//    rz0 is an increasing valued array
//    Measurements are assumed to be on the mid-plane only *** this needs correction !!!

            float* yagpsi = (float*) malloc(
                    (efitdata->nne + 3) * sizeof(float));    // Measurement location psi values
            for (i = 0; i < efitdata->nne; i++)yagpsi[i] = 0.0;

            err = xdatamap(efitdata->nne, efitdata->rne, efitdata->rz0Count, efitdata->rz0, efitdata->psiz0,
                           efitdata->psi_bnd, yagpsi);

            for (i = 0; i < efitdata->nne; i++) {
                if (efitdata->rne[i] < efitdata->rz0[0]) yagpsi[i] = efitdata->psiz0[0];
                if (efitdata->rne[i] > efitdata->rz0[efitdata->rz0Count - 1]) yagpsi[i] = psiz0[efitdata->rz0Count - 1];
            }

// 2) Interpolate psi(R) to phi(R) or trho(R) - Inner and Outer set of mappings


            float* yagphi = (float*) malloc((efitdata->nne + 3) * sizeof(float));
            for (i = 0; i < efitdata->nne; i++)yagphi[i] = 0.0;

            err = xdatamapx(efitdata->nne, yagpsi, efitdata->qCount, efitdata->psi, efitdata->phi, yagphi);

            for (i = 0; i < efitdata->nne; i++) {
                if (efitdata->rne[i] <= efitdata->Rmin) yagphi[i] = efitdata->phi[efitdata->qCount - 1];
                if (efitdata->rne[i] >= efitdata->Rmax) yagphi[i] = efitdata->phi[efitdata->qCount - 1];
            }

            float* yagprho = (float*) malloc((efitdata->nne + 3) * sizeof(float));
            for (i = 0; i < efitdata->nne; i++)yagprho[i] = 0.0;

            err = xdatamapx(efitdata->nne, yagpsi, efitdata->qCount, efitdata->psi, efitdata->rho, yagprho);

            for (i = 0; i < efitdata->nne; i++) {
                if (efitdata->rne[i] <= efitdata->Rmin) yagprho[i] = 1.0;
                if (efitdata->rne[i] >= efitdata->Rmax) yagprho[i] = 1.0;
            }

// 3) Interpolate ne(trho(R)) to required trho grid - Inner and Outer sets

            float* yagtrho = (float*) malloc((efitdata->nne + 3) * sizeof(float));
            for (i = 0; i < efitdata->nne; i++)yagtrho[i] = 0.0;

            err = xdatamapx(efitdata->nne, yagpsi, efitdata->qCount, efitdata->psi, efitdata->trho, yagtrho);

            for (i = 0; i < efitdata->nne; i++) {
                if (efitdata->rne[i] <= efitdata->Rmin) yagtrho[i] = 1.0;
                if (efitdata->rne[i] >= efitdata->Rmax) yagtrho[i] = 1.0;
            }

// 4) Interpolate ne(rho_tor(R)) to required rho_tor grid - Inner and Outer sets

            float* yagrhotor = (float*) malloc((efitdata->nne + 3) * sizeof(float));
            for (i = 0; i < efitdata->nne; i++)yagrhotor[i] = 0.0;

            err = xdatamapx(efitdata->nne, yagpsi, efitdata->qCount, efitdata->psi, efitdata->rho_tor, yagrhotor);

            for (i = 0; i < efitdata->nne; i++) {
                if (efitdata->rne[i] <= efitdata->Rmin) yagrhotor[i] = 1.0;
                if (efitdata->rne[i] >= efitdata->Rmax) yagrhotor[i] = 1.0;
            }

            idamLog(LOG_DEBUG, "YAG Coordinates: R, psi, phi, rho, trho, rho_tor\n");
            for (i = 0; i < efitdata->nne; i++)
                idamLog(LOG_DEBUG, "[%3d] %f   %f   %f   %f   %f   %f\n", i, efitdata->rne[i], yagpsi[i],
                        yagphi[i], yagprho[i], yagtrho[i], yagrhotor[i]);

            efitdata->yagpsi = yagpsi;
            efitdata->yagphi = yagphi;
            efitdata->yagprho = yagprho;
            efitdata->yagtrho = yagtrho;
            efitdata->yagrhotor = yagrhotor;

// Magnetic Axis and Boundary values

            float neRmag, TeRmag, neRmin, TeRmin, neRmax, TeRmax;

            err = xdatamap(1, &efitdata->rmag, efitdata->nne, efitdata->rne, efitdata->ne, 0.0, &neRmag);
            err = xdatamap(1, &efitdata->rmag, efitdata->nne, efitdata->rne, efitdata->te, 0.0, &TeRmag);
            err = xdatamap(1, &efitdata->Rmin, efitdata->nne, efitdata->rne, efitdata->ne, 0.0, &neRmin);
            err = xdatamap(1, &efitdata->Rmin, efitdata->nne, efitdata->rne, efitdata->te, 0.0, &TeRmin);
            err = xdatamap(1, &efitdata->Rmax, efitdata->nne, efitdata->rne, efitdata->ne, 0.0, &neRmax);
            err = xdatamap(1, &efitdata->Rmax, efitdata->nne, efitdata->rne, efitdata->te, 0.0, &TeRmax);

            if (neRmin == 0.0 && neRmax != 0.0) neRmin = neRmax;
            if (TeRmin == 0.0 && TeRmax != 0.0) TeRmin = TeRmax;
            if (neRmin != 0.0 && neRmax == 0.0) neRmax = neRmin;
            if (TeRmin != 0.0 && TeRmax == 0.0) TeRmax = TeRmin;

            idamLog(LOG_DEBUG, "YAG Data at Rmin, Rmag, Rmax\n");
            idamLog(LOG_DEBUG, "ne: [%f] %e   [%f] %e   [%f] %e\n", efitdata->Rmin, neRmin, efitdata->rmag, neRmag,
                    efitdata->Rmax, neRmax);
            idamLog(LOG_DEBUG, "Te: [%f] %e   [%f] %e   [%f] %e\n", efitdata->Rmin, TeRmin, efitdata->rmag, TeRmag,
                    efitdata->Rmax, TeRmax);

// Locate the Magnetic Axis

            int iRmag = -1;
            for (i = 0; i < efitdata->nne; i++) if (efitdata->rne[i] <= efitdata->rmag) iRmag = i;
            idamLog(LOG_DEBUG, "YAG Psi coordinate: Major Radius, Poloidal Flux, Magnetic Axis %d\n", iRmag);

// Add points at each boundary and magnetic axis if missing to rne, ne, te, yagpsi and yagphi

            if (efitdata->rne[iRmag] < efitdata->rmag && efitdata->rne[iRmag + 1] > efitdata->rmag) {
                k = efitdata->nne;
                for (i = iRmag + 1; i < efitdata->nne; i++) {
                    efitdata->rne[k] = efitdata->rne[k - 1];
                    efitdata->ne[k] = efitdata->ne[k - 1];
                    efitdata->te[k] = efitdata->te[k - 1];
                    efitdata->yagpsi[k] = efitdata->yagpsi[k - 1];
                    efitdata->yagphi[k] = efitdata->yagphi[k - 1];
                    efitdata->yagprho[k] = efitdata->yagprho[k - 1];
                    efitdata->yagtrho[k] = efitdata->yagtrho[k - 1];
                    efitdata->yagrhotor[k] = efitdata->yagrhotor[k - 1];
                    k--;
                }
                efitdata->rne[iRmag + 1] = efitdata->rmag;
                efitdata->ne[iRmag + 1] = neRmag;
                efitdata->te[iRmag + 1] = TeRmag;
                efitdata->yagpsi[iRmag + 1] = efitdata->psi[0];
                efitdata->yagphi[iRmag + 1] = efitdata->phi[0];
                efitdata->yagprho[iRmag + 1] = 0.0;
                efitdata->yagtrho[iRmag + 1] = 0.0;
                efitdata->yagrhotor[iRmag + 1] = 0.0;
                efitdata->nne++;
                iRmag++;
            }

            if (efitdata->rne[0] > efitdata->Rmin) {
                k = efitdata->nne;
                for (i = 0; i < efitdata->nne; i++) {
                    efitdata->rne[k] = efitdata->rne[k - 1];
                    efitdata->ne[k] = efitdata->ne[k - 1];
                    efitdata->te[k] = efitdata->te[k - 1];
                    efitdata->yagpsi[k] = efitdata->yagpsi[k - 1];
                    efitdata->yagphi[k] = efitdata->yagphi[k - 1];
                    efitdata->yagprho[k] = efitdata->yagprho[k - 1];
                    efitdata->yagtrho[k] = efitdata->yagtrho[k - 1];
                    efitdata->yagrhotor[k] = efitdata->yagrhotor[k - 1];
                    k--;
                }
                efitdata->rne[0] = efitdata->Rmin;
                efitdata->ne[0] = neRmin;
                efitdata->te[0] = TeRmin;
                efitdata->yagpsi[0] = efitdata->psi_bnd;
                efitdata->yagphi[0] = efitdata->phi[efitdata->qCount - 1];
                efitdata->yagprho[0] = 1.0;
                efitdata->yagtrho[0] = 1.0;
                efitdata->yagrhotor[0] = 1.0;
                efitdata->nne++;
            }

            if (efitdata->rne[efitdata->nne - 1] < efitdata->Rmax) {
                efitdata->nne++;
                efitdata->rne[efitdata->nne - 1] = efitdata->Rmax;
                efitdata->ne[efitdata->nne - 1] = neRmax;
                efitdata->te[efitdata->nne - 1] = TeRmax;
                efitdata->yagpsi[efitdata->nne - 1] = efitdata->psi_bnd;
                efitdata->yagphi[efitdata->nne - 1] = efitdata->phi[efitdata->qCount - 1];
                efitdata->yagprho[efitdata->nne - 1] = 1.0;
                efitdata->yagtrho[efitdata->nne - 1] = 1.0;
                efitdata->yagrhotor[efitdata->nne - 1] = 1.0;
            }

// Locate the Magnetic Axis

            iRmag = -1;
            for (i = 0; i < efitdata->nne; i++) if (efitdata->rne[i] <= efitdata->rmag) iRmag = i;

            idamLog(LOG_DEBUG, "YAG Psi coordinate: Major Radius, Poloidal Flux, Magnetic Axis %d\n", iRmag);
            for (i = 0; i < efitdata->nne; i++) idamLog(LOG_DEBUG, "[%3d] %f   %f\n", i, efitdata->rne[i], yagpsi[i]);

// Map data to Flux Surface Centers Grid

            float* mapyagne = (float*) malloc(equimapdata->rhoCount * sizeof(float));
            float* mapyagte = (float*) malloc(equimapdata->rhoCount * sizeof(float));
            float* mapyagpsi = (float*) malloc(equimapdata->rhoCount * sizeof(float));
            float* mapyagphi = (float*) malloc(equimapdata->rhoCount * sizeof(float));

            float* mapyagr1 = (float*) malloc(equimapdata->rhoCount * sizeof(float));
            float* mapyagne1 = (float*) malloc(equimapdata->rhoCount * sizeof(float));
            float* mapyagte1 = (float*) malloc(equimapdata->rhoCount * sizeof(float));
            float* mapyagpsi1 = (float*) malloc(equimapdata->rhoCount * sizeof(float));
            float* mapyagphi1 = (float*) malloc(equimapdata->rhoCount * sizeof(float));

            float* mapyagr2 = (float*) malloc(equimapdata->rhoCount * sizeof(float));
            float* mapyagne2 = (float*) malloc(equimapdata->rhoCount * sizeof(float));
            float* mapyagte2 = (float*) malloc(equimapdata->rhoCount * sizeof(float));
            float* mapyagpsi2 = (float*) malloc(equimapdata->rhoCount * sizeof(float));
            float* mapyagphi2 = (float*) malloc(equimapdata->rhoCount * sizeof(float));

            switch (equimapdata->rhoType) {

                case (SQRTNORMALISEDTOROIDALFLUX): {

// Inner measurements

                    err = xdatamapx(equimapdata->rhoCount, equimapdata->rho, iRmag + 1, efitdata->yagtrho,
                                    efitdata->rne,
                                    mapyagr1);

                    err = xdatamapx(equimapdata->rhoCount, equimapdata->rho, iRmag + 1, efitdata->yagtrho, efitdata->ne,
                                    mapyagne1);

                    err = xdatamapx(equimapdata->rhoCount, equimapdata->rho, iRmag + 1, efitdata->yagtrho, efitdata->te,
                                    mapyagte1);

                    err = xdatamapx(equimapdata->rhoCount, equimapdata->rho, iRmag + 1, efitdata->yagtrho,
                                    efitdata->yagpsi, mapyagpsi1);

                    err = xdatamapx(equimapdata->rhoCount, equimapdata->rho, iRmag + 1, efitdata->yagtrho,
                                    efitdata->yagphi, mapyagphi1);
// Outer measurements

                    err = xdatamap(equimapdata->rhoCount, equimapdata->rho, efitdata->nne - iRmag,
                                   &efitdata->yagtrho[iRmag], &efitdata->rne[iRmag], 0.0, mapyagr2);

                    err = xdatamap(equimapdata->rhoCount, equimapdata->rho, efitdata->nne - iRmag,
                                   &efitdata->yagtrho[iRmag], &efitdata->ne[iRmag], 0.0, mapyagne2);

                    err = xdatamap(equimapdata->rhoCount, equimapdata->rho, efitdata->nne - iRmag,
                                   &efitdata->yagtrho[iRmag], &efitdata->te[iRmag], 0.0, mapyagte2);

                    err = xdatamap(equimapdata->rhoCount, equimapdata->rho, efitdata->nne - iRmag,
                                   &efitdata->yagtrho[iRmag], &efitdata->yagpsi[iRmag], 0.0, mapyagpsi2);

                    err = xdatamap(equimapdata->rhoCount, equimapdata->rho, efitdata->nne - iRmag,
                                   &efitdata->yagtrho[iRmag], &efitdata->yagphi[iRmag], 0.0, mapyagphi2);

                    break;
                }

                case (NORMALISEDPOLOIDALFLUX): {

// Inner measurements

                    err = xdatamapx(equimapdata->rhoCount, equimapdata->rho, iRmag + 1, efitdata->yagprho,
                                    efitdata->rne,
                                    mapyagr1);
                    err = xdatamapx(equimapdata->rhoCount, equimapdata->rho, iRmag + 1, efitdata->yagprho, efitdata->ne,
                                    mapyagne1);
                    err = xdatamapx(equimapdata->rhoCount, equimapdata->rho, iRmag + 1, efitdata->yagprho, efitdata->te,
                                    mapyagte1);
                    err = xdatamapx(equimapdata->rhoCount, equimapdata->rho, iRmag + 1, efitdata->yagprho,
                                    efitdata->yagpsi, mapyagpsi1);
                    err = xdatamapx(equimapdata->rhoCount, equimapdata->rho, iRmag + 1, efitdata->yagprho,
                                    efitdata->yagphi, mapyagphi1);

// Outer measurements

                    err = xdatamap(equimapdata->rhoCount, equimapdata->rho, efitdata->nne - iRmag,
                                   &efitdata->yagprho[iRmag], &efitdata->rne[iRmag], 0.0, mapyagr2);
                    err = xdatamap(equimapdata->rhoCount, equimapdata->rho, efitdata->nne - iRmag,
                                   &efitdata->yagprho[iRmag], &efitdata->ne[iRmag], 0.0, mapyagne2);
                    err = xdatamap(equimapdata->rhoCount, equimapdata->rho, efitdata->nne - iRmag,
                                   &efitdata->yagprho[iRmag], &efitdata->te[iRmag], 0.0, mapyagte2);
                    err = xdatamap(equimapdata->rhoCount, equimapdata->rho, efitdata->nne - iRmag,
                                   &efitdata->yagprho[iRmag], &efitdata->yagpsi[iRmag], 0.0, mapyagpsi2);
                    err = xdatamap(equimapdata->rhoCount, equimapdata->rho, efitdata->nne - iRmag,
                                   &efitdata->yagprho[iRmag], &efitdata->yagphi[iRmag], 0.0, mapyagphi2);
                    break;
                }

                case (NORMALISEDITMFLUXRADIUS): {

// Inner measurements

                    err = xdatamapx(equimapdata->rhoCount, equimapdata->rho, iRmag + 1, efitdata->yagrhotor,
                                    efitdata->rne, mapyagr1);
                    err = xdatamapx(equimapdata->rhoCount, equimapdata->rho, iRmag + 1, efitdata->yagrhotor,
                                    efitdata->ne,
                                    mapyagne1);
                    err = xdatamapx(equimapdata->rhoCount, equimapdata->rho, iRmag + 1, efitdata->yagrhotor,
                                    efitdata->te,
                                    mapyagte1);
                    err = xdatamapx(equimapdata->rhoCount, equimapdata->rho, iRmag + 1, efitdata->yagrhotor,
                                    efitdata->yagpsi, mapyagpsi1);
                    err = xdatamapx(equimapdata->rhoCount, equimapdata->rho, iRmag + 1, efitdata->yagrhotor,
                                    efitdata->yagphi, mapyagphi1);

// Outer measurements

                    err = xdatamap(equimapdata->rhoCount, equimapdata->rho, efitdata->nne - iRmag,
                                   &efitdata->yagrhotor[iRmag], &efitdata->rne[iRmag], 0.0, mapyagr2);
                    err = xdatamap(equimapdata->rhoCount, equimapdata->rho, efitdata->nne - iRmag,
                                   &efitdata->yagrhotor[iRmag], &efitdata->ne[iRmag], 0.0, mapyagne2);
                    err = xdatamap(equimapdata->rhoCount, equimapdata->rho, efitdata->nne - iRmag,
                                   &efitdata->yagrhotor[iRmag], &efitdata->te[iRmag], 0.0, mapyagte2);
                    err = xdatamap(equimapdata->rhoCount, equimapdata->rho, efitdata->nne - iRmag,
                                   &efitdata->yagrhotor[iRmag], &efitdata->yagpsi[iRmag], 0.0, mapyagpsi2);
                    err = xdatamap(equimapdata->rhoCount, equimapdata->rho, efitdata->nne - iRmag,
                                   &efitdata->yagrhotor[iRmag], &efitdata->yagphi[iRmag], 0.0, mapyagphi2);
                    break;
                }

            }

// Average Inner and Outer measurement profiles (Not Major Radii locations!)

            for (i = 0; i < equimapdata->rhoCount; i++) {
                mapyagne[i] = 0.5 * (mapyagne1[i] + mapyagne2[i]);
                mapyagte[i] = 0.5 * (mapyagte1[i] + mapyagte2[i]);
                mapyagpsi[i] = 0.5 * (mapyagpsi1[i] + mapyagpsi2[i]);
                mapyagphi[i] = 0.5 * (mapyagphi1[i] + mapyagphi2[i]);
            }

            efitdata->mapyagne = mapyagne;
            efitdata->mapyagte = mapyagte;
            efitdata->mapyagpsi = mapyagpsi;
            efitdata->mapyagphi = mapyagphi;

            efitdata->mapyagr1 = mapyagr1;
            efitdata->mapyagne1 = mapyagne1;
            efitdata->mapyagte1 = mapyagte1;
            efitdata->mapyagpsi1 = mapyagpsi1;
            efitdata->mapyagphi1 = mapyagphi1;

            efitdata->mapyagr2 = mapyagr2;
            efitdata->mapyagne2 = mapyagne2;
            efitdata->mapyagte2 = mapyagte2;
            efitdata->mapyagpsi2 = mapyagpsi2;
            efitdata->mapyagphi2 = mapyagphi2;


// Map data to Surface Boundaries Grid

            float* mapyagneB = (float*) malloc(equimapdata->rhoBCount * sizeof(float));
            float* mapyagteB = (float*) malloc(equimapdata->rhoBCount * sizeof(float));
            float* mapyagpsiB = (float*) malloc(equimapdata->rhoBCount * sizeof(float));
            float* mapyagphiB = (float*) malloc(equimapdata->rhoBCount * sizeof(float));

            float* mapyagr1B = (float*) malloc(equimapdata->rhoBCount * sizeof(float));
            float* mapyagne1B = (float*) malloc(equimapdata->rhoBCount * sizeof(float));
            float* mapyagte1B = (float*) malloc(equimapdata->rhoBCount * sizeof(float));
            float* mapyagpsi1B = (float*) malloc(equimapdata->rhoBCount * sizeof(float));
            float* mapyagphi1B = (float*) malloc(equimapdata->rhoBCount * sizeof(float));

            float* mapyagr2B = (float*) malloc(equimapdata->rhoBCount * sizeof(float));
            float* mapyagne2B = (float*) malloc(equimapdata->rhoBCount * sizeof(float));
            float* mapyagte2B = (float*) malloc(equimapdata->rhoBCount * sizeof(float));
            float* mapyagpsi2B = (float*) malloc(equimapdata->rhoBCount * sizeof(float));
            float* mapyagphi2B = (float*) malloc(equimapdata->rhoBCount * sizeof(float));

            switch (equimapdata->rhoType) {

                case (SQRTNORMALISEDTOROIDALFLUX): {

// Inner measurements

                    err = xdatamapx(equimapdata->rhoBCount, equimapdata->rhoB, iRmag + 1, efitdata->yagtrho,
                                    efitdata->rne, mapyagr1B);
                    mapyagr1B[0] = efitdata->rmag;
                    mapyagr1B[equimapdata->rhoBCount - 1] = efitdata->Rmin;

                    err = xdatamapx(equimapdata->rhoBCount, equimapdata->rhoB, iRmag + 1, efitdata->yagtrho,
                                    efitdata->ne,
                                    mapyagne1B);
                    mapyagne1B[0] = neRmag;
                    mapyagne1B[equimapdata->rhoBCount - 1] = neRmin;

                    err = xdatamapx(equimapdata->rhoBCount, equimapdata->rhoB, iRmag + 1, efitdata->yagtrho,
                                    efitdata->te,
                                    mapyagte1B);
                    mapyagte1B[0] = TeRmag;
                    mapyagte1B[equimapdata->rhoBCount - 1] = TeRmin;

                    err = xdatamapx(equimapdata->rhoBCount, equimapdata->rhoB, iRmag + 1, efitdata->yagtrho,
                                    efitdata->yagpsi, mapyagpsi1B);
                    mapyagpsi1B[0] = efitdata->psi_mag;
                    mapyagpsi1B[equimapdata->rhoBCount - 1] = efitdata->psi_bnd;

                    err = xdatamapx(equimapdata->rhoBCount, equimapdata->rhoB, iRmag + 1, efitdata->yagtrho,
                                    efitdata->yagphi, mapyagphi1B);
                    mapyagphi1B[0] = efitdata->phi[0];
                    mapyagphi1B[equimapdata->rhoBCount - 1] = efitdata->phi[efitdata->qCount - 1];

// Outer measurements

                    err = xdatamap(equimapdata->rhoBCount, equimapdata->rhoB, efitdata->nne - iRmag,
                                   &efitdata->yagtrho[iRmag], &efitdata->rne[iRmag], 0.0, mapyagr2B);
                    mapyagr2B[0] = efitdata->rmag;
                    mapyagr2B[equimapdata->rhoBCount - 1] = efitdata->Rmax;

                    err = xdatamap(equimapdata->rhoBCount, equimapdata->rhoB, efitdata->nne - iRmag,
                                   &efitdata->yagtrho[iRmag], &efitdata->ne[iRmag], 0.0, mapyagne2B);
                    mapyagne2B[0] = neRmag;
                    mapyagne2B[equimapdata->rhoBCount - 1] = neRmax;

                    err = xdatamap(equimapdata->rhoBCount, equimapdata->rhoB, efitdata->nne - iRmag,
                                   &efitdata->yagtrho[iRmag], &efitdata->te[iRmag], 0.0, mapyagte2B);
                    mapyagte2B[0] = TeRmag;
                    mapyagte2B[equimapdata->rhoBCount - 1] = TeRmax;

                    err = xdatamap(equimapdata->rhoBCount, equimapdata->rhoB, efitdata->nne - iRmag,
                                   &efitdata->yagtrho[iRmag], &efitdata->yagpsi[iRmag], 0.0, mapyagpsi2B);
                    mapyagpsi2B[0] = efitdata->psi_mag;
                    mapyagpsi2B[equimapdata->rhoBCount - 1] = efitdata->psi_bnd;

                    err = xdatamap(equimapdata->rhoBCount, equimapdata->rhoB, efitdata->nne - iRmag,
                                   &efitdata->yagtrho[iRmag], &efitdata->yagphi[iRmag], 0.0, mapyagphi2B);
                    mapyagphi2B[0] = efitdata->phi[0];
                    mapyagphi2B[equimapdata->rhoBCount - 1] = efitdata->phi[efitdata->qCount - 1];

                    break;
                }

                case (NORMALISEDPOLOIDALFLUX): {

// Inner measurements

                    err = xdatamapx(equimapdata->rhoBCount, equimapdata->rhoB, iRmag + 1, efitdata->yagprho,
                                    efitdata->rne, mapyagr1B);
                    mapyagr1B[0] = efitdata->rmag;
                    mapyagr1B[equimapdata->rhoBCount - 1] = efitdata->Rmin;

                    err = xdatamapx(equimapdata->rhoBCount, equimapdata->rhoB, iRmag + 1, efitdata->yagprho,
                                    efitdata->ne,
                                    mapyagne1B);
                    mapyagne1B[0] = neRmag;
                    mapyagne1B[equimapdata->rhoBCount - 1] = neRmin;

                    err = xdatamapx(equimapdata->rhoBCount, equimapdata->rhoB, iRmag + 1, efitdata->yagprho,
                                    efitdata->te,
                                    mapyagte1B);
                    mapyagte1B[0] = TeRmag;
                    mapyagte1B[equimapdata->rhoBCount - 1] = TeRmin;

                    err = xdatamapx(equimapdata->rhoBCount, equimapdata->rhoB, iRmag + 1, efitdata->yagprho,
                                    efitdata->yagpsi, mapyagpsi1B);
                    mapyagpsi1B[0] = efitdata->psi_mag;
                    mapyagpsi1B[equimapdata->rhoBCount - 1] = efitdata->psi_bnd;

                    err = xdatamapx(equimapdata->rhoBCount, equimapdata->rhoB, iRmag + 1, efitdata->yagprho,
                                    efitdata->yagphi, mapyagphi1B);
                    mapyagphi1B[0] = efitdata->phi[0];
                    mapyagphi1B[equimapdata->rhoBCount - 1] = efitdata->phi[efitdata->qCount - 1];

// Outer measurements

                    err = xdatamap(equimapdata->rhoBCount, equimapdata->rhoB, efitdata->nne - iRmag,
                                   &efitdata->yagprho[iRmag], &efitdata->rne[iRmag], 0.0, mapyagr2B);
                    mapyagr2B[0] = efitdata->rmag;
                    mapyagr2B[equimapdata->rhoBCount - 1] = efitdata->Rmax;

                    err = xdatamap(equimapdata->rhoBCount, equimapdata->rhoB, efitdata->nne - iRmag,
                                   &efitdata->yagprho[iRmag], &efitdata->ne[iRmag], 0.0, mapyagne2B);
                    mapyagne2B[0] = neRmag;
                    mapyagne2B[equimapdata->rhoBCount - 1] = neRmax;

                    err = xdatamap(equimapdata->rhoBCount, equimapdata->rhoB, efitdata->nne - iRmag,
                                   &efitdata->yagprho[iRmag], &efitdata->te[iRmag], 0.0, mapyagte2B);
                    mapyagte2B[0] = TeRmag;
                    mapyagte2B[equimapdata->rhoBCount - 1] = TeRmax;

                    err = xdatamap(equimapdata->rhoBCount, equimapdata->rhoB, efitdata->nne - iRmag,
                                   &efitdata->yagprho[iRmag], &efitdata->yagpsi[iRmag], 0.0, mapyagpsi2B);
                    mapyagpsi2B[0] = efitdata->psi_mag;
                    mapyagpsi2B[equimapdata->rhoBCount - 1] = efitdata->psi_bnd;

                    err = xdatamap(equimapdata->rhoBCount, equimapdata->rhoB, efitdata->nne - iRmag,
                                   &efitdata->yagprho[iRmag], &efitdata->yagphi[iRmag], 0.0, mapyagphi2B);
                    mapyagphi2B[0] = efitdata->phi[0];
                    mapyagphi2B[equimapdata->rhoBCount - 1] = efitdata->phi[efitdata->qCount - 1];

                    break;
                }

                case (NORMALISEDITMFLUXRADIUS): {

// Inner measurements

                    err = xdatamapx(equimapdata->rhoBCount, equimapdata->rhoB, iRmag + 1, efitdata->yagrhotor,
                                    efitdata->rne, mapyagr1B);
                    mapyagr1B[0] = efitdata->rmag;
                    mapyagr1B[equimapdata->rhoBCount - 1] = efitdata->Rmin;

                    err = xdatamapx(equimapdata->rhoBCount, equimapdata->rhoB, iRmag + 1, efitdata->yagrhotor,
                                    efitdata->ne, mapyagne1B);
                    mapyagne1B[0] = neRmag;
                    mapyagne1B[equimapdata->rhoBCount - 1] = neRmin;

                    err = xdatamapx(equimapdata->rhoBCount, equimapdata->rhoB, iRmag + 1, efitdata->yagrhotor,
                                    efitdata->te, mapyagte1B);
                    mapyagte1B[0] = TeRmag;
                    mapyagte1B[equimapdata->rhoBCount - 1] = TeRmin;

                    err = xdatamapx(equimapdata->rhoBCount, equimapdata->rhoB, iRmag + 1, efitdata->yagrhotor,
                                    efitdata->yagpsi, mapyagpsi1B);
                    mapyagpsi1B[0] = efitdata->psi_mag;
                    mapyagpsi1B[equimapdata->rhoBCount - 1] = efitdata->psi_bnd;

                    err = xdatamapx(equimapdata->rhoBCount, equimapdata->rhoB, iRmag + 1, efitdata->yagrhotor,
                                    efitdata->yagphi, mapyagphi1B);
                    mapyagphi1B[0] = efitdata->phi[0];
                    mapyagphi1B[equimapdata->rhoBCount - 1] = efitdata->phi[efitdata->qCount - 1];

// Outer measurements

                    err = xdatamap(equimapdata->rhoBCount, equimapdata->rhoB, efitdata->nne - iRmag,
                                   &efitdata->yagrhotor[iRmag], &efitdata->rne[iRmag], 0.0, mapyagr2B);
                    mapyagr2B[0] = efitdata->rmag;
                    mapyagr2B[equimapdata->rhoBCount - 1] = efitdata->Rmax;

                    err = xdatamap(equimapdata->rhoBCount, equimapdata->rhoB, efitdata->nne - iRmag,
                                   &efitdata->yagrhotor[iRmag], &efitdata->ne[iRmag], 0.0, mapyagne2B);
                    mapyagne2B[0] = neRmag;
                    mapyagne2B[equimapdata->rhoBCount - 1] = neRmax;

                    err = xdatamap(equimapdata->rhoBCount, equimapdata->rhoB, efitdata->nne - iRmag,
                                   &efitdata->yagrhotor[iRmag], &efitdata->te[iRmag], 0.0, mapyagte2B);
                    mapyagte2B[0] = TeRmag;
                    mapyagte2B[equimapdata->rhoBCount - 1] = TeRmax;

                    err = xdatamap(equimapdata->rhoBCount, equimapdata->rhoB, efitdata->nne - iRmag,
                                   &efitdata->yagrhotor[iRmag], &efitdata->yagpsi[iRmag], 0.0, mapyagpsi2B);
                    mapyagpsi2B[0] = efitdata->psi_mag;
                    mapyagpsi2B[equimapdata->rhoBCount - 1] = efitdata->psi_bnd;

                    err = xdatamap(equimapdata->rhoBCount, equimapdata->rhoB, efitdata->nne - iRmag,
                                   &efitdata->yagrhotor[iRmag], &efitdata->yagphi[iRmag], 0.0, mapyagphi2B);
                    mapyagphi2B[0] = efitdata->phi[0];
                    mapyagphi2B[equimapdata->rhoBCount - 1] = efitdata->phi[efitdata->qCount - 1];

                    break;
                }
            }

// Average Inner and Outer measurement profiles (Not Major Radii locations!)

            for (i = 0; i < equimapdata->rhoBCount; i++) {
                mapyagneB[i] = 0.5 * (mapyagne1B[i] + mapyagne2B[i]);
                mapyagteB[i] = 0.5 * (mapyagte1B[i] + mapyagte2B[i]);
                mapyagpsiB[i] = 0.5 * (mapyagpsi1B[i] + mapyagpsi2B[i]);
                mapyagphiB[i] = 0.5 * (mapyagphi1B[i] + mapyagphi2B[i]);
            }

            efitdata->mapyagneB = mapyagneB;
            efitdata->mapyagteB = mapyagteB;
            efitdata->mapyagpsiB = mapyagpsiB;
            efitdata->mapyagphiB = mapyagphiB;

            efitdata->mapyagr1B = mapyagr1B;
            efitdata->mapyagne1B = mapyagne1B;
            efitdata->mapyagte1B = mapyagte1B;
            efitdata->mapyagpsi1B = mapyagpsi1B;
            efitdata->mapyagphi1B = mapyagphi1B;

            efitdata->mapyagr2B = mapyagr2B;
            efitdata->mapyagne2B = mapyagne2B;
            efitdata->mapyagte2B = mapyagte2B;
            efitdata->mapyagpsi2B = mapyagpsi2B;
            efitdata->mapyagphi2B = mapyagphi2B;
        }

//--------------------------------------------------------------------------------------------------------------
//--------------------------------------------------------------------------------------------------------------
//--------------------------------------------------------------------------------------------------------------
// Read Inner Limiter Data for Center Column Major Radius

        if (set == 2) {

// Coordinates of First X-Point on Separatrix

            if ((handle = whichHandle("EFM_XPOINT1_R(C)")) < 0) {
                err = 999;
                addIdamError(&idamerrorstack, CODEERRORTYPE, "equimap extractData", err,
                             "IDAM data handle not found when expected!");
                return err;
            }

            if ((err = xdatand("EFM_XPOINT1_R(C)", shot_str, &handle, &rank, &order, &ndata, &shape, &data, &dim)) !=
                0)
                break;

            if ((err = xdatainterval(rank, order, ndata, shape, dim, tslice, window, &target1, &target2)) != 0) {
                idamLog(LOG_ERROR, "The requested Time %e could not be located in the data array!\n", tslice);
                break;
            }

            nslices = target2 - target1 + 1;

            sum = 0.0;
            for (j = target1; j <= target2; j++) sum = sum + data[j];

            efitdata->rxpoint1 = sum / (float) nslices;

            free((void*) shape);
            free((void*) data);
            free((void*) dim);

            if ((handle = whichHandle("EFM_XPOINT1_Z(C)")) < 0) {
                err = 999;
                addIdamError(&idamerrorstack, CODEERRORTYPE, "equimap extractData", err,
                             "IDAM data handle not found when expected!");
                return err;
            }

            if ((err = xdatand("EFM_XPOINT1_Z(C)", shot_str, &handle, &rank, &order, &ndata, &shape, &data, &dim)) !=
                0)
                break;

            if ((err = xdatainterval(rank, order, ndata, shape, dim, tslice, window, &target1, &target2)) != 0) {
                idamLog(LOG_ERROR, "The requested Time %e could not be located in the data array!\n", tslice);
                break;
            }

            nslices = target2 - target1 + 1;

            sum = 0.0;
            for (j = target1; j <= target2; j++) sum = sum + data[j];

            efitdata->zxpoint1 = sum / (float) nslices;

            free((void*) shape);
            free((void*) data);
            free((void*) dim);

// Coordinates of Second X-Point on Separatrix

            if ((handle = whichHandle("EFM_XPOINT2_R(C)")) < 0) {
                err = 999;
                addIdamError(&idamerrorstack, CODEERRORTYPE, "equimap extractData", err,
                             "IDAM data handle not found when expected!");
                return err;
            }

            if ((err = xdatand("EFM_XPOINT2_R(C)", shot_str, &handle, &rank, &order, &ndata, &shape, &data, &dim)) !=
                0)
                break;

            if ((err = xdatainterval(rank, order, ndata, shape, dim, tslice, window, &target1, &target2)) != 0) {
                idamLog(LOG_ERROR, "The requested Time %e could not be located in the data array!\n", tslice);
                break;
            }

            nslices = target2 - target1 + 1;

            sum = 0.0;
            for (j = target1; j <= target2; j++) sum = sum + data[j];

            efitdata->rxpoint2 = sum / (float) nslices;

            free((void*) shape);
            free((void*) data);
            free((void*) dim);


            if ((handle = whichHandle("EFM_XPOINT2_Z(C)")) < 0) {
                err = 999;
                addIdamError(&idamerrorstack, CODEERRORTYPE, "equimap extractData", err,
                             "IDAM data handle not found when expected!");
                return err;
            }

            if ((err = xdatand("EFM_XPOINT2_Z(C)", shot_str, &handle, &rank, &order, &ndata, &shape, &data, &dim)) !=
                0)
                break;

            if ((err = xdatainterval(rank, order, ndata, shape, dim, tslice, window, &target1, &target2)) != 0) {
                idamLog(LOG_ERROR, "The requested Time %e could not be located in the data array!\n", tslice);
                break;
            }

            nslices = target2 - target1 + 1;

            sum = 0.0;
            for (j = target1; j <= target2; j++) sum = sum + data[j];

            efitdata->zxpoint2 = sum / (float) nslices;

            free((void*) shape);
            free((void*) data);
            free((void*) dim);

        }

//--------------------------------------------------------------------------------------------------------------
// Housekeeping

    } while (0);

    return (0);
}
