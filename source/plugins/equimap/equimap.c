/*---------------------------------------------------------------
* IDAM Plugin data Reader to Access DATA mapped to a common time invarient grid

* Input Arguments:	IDAM_PLUGIN_INTERFACE *idam_plugin_interface
*
* Returns:		equimap		0 if read was successful
*					otherwise a Error Code is returned 
*			DATA_BLOCK	Structure with Data from the File 
*
* Calls		freeDataBlock	to free Heap memory if an Error Occurs
*
* Notes: 	All memory required to hold data is allocated dynamically
*		in heap storage. Pointers to these areas of memory are held
*		by the passed DATA_BLOCK structure. Local memory allocations
*		are freed on exit. However, the blocks reserved for data are
*		not and MUST BE FREED by the calling routine.
*
*---------------------------------------------------------------------------------------------------------------*/
#include "equimap.h"

#include <stdlib.h>

#include <include/idamserver.h>
#include <clientserver/initStructs.h>
#include <client/accAPI_C.h>
#include <client/idam_client.h>
#include <clientserver/idamTypes.h>

#include "importdata.h"
#include "smoothpsi.h"

static int handleCount = 0;
static int handles[MAXHANDLES];

static EQUIMAPDATA equimapdata;

extern int equiMap(IDAM_PLUGIN_INTERFACE* idam_plugin_interface)
{
    int i, j, k, err, offset;
    int handle = -1;
    static short init = 0;
    static int prior_exp_number = -1;
    static char prior_file[STRING_LENGTH];
    static int smoothedPsi = 0;
    float* arr;

//----------------------------------------------------------------------------------------
// Standard Plugin Interface

    DATA_BLOCK* data_block;
    REQUEST_BLOCK* request_block;

#ifndef USE_PLUGIN_DIRECTLY
    IDAMERRORSTACK idamerrorstack;
    IDAMERRORSTACK* idamErrorStack = getIdamServerPluginErrorStack();        // Server library functions

    initIdamErrorStack(&idamerrorstack);
#else
    IDAMERRORSTACK *idamErrorStack = &idamerrorstack;   	// For testing only!
#endif

    unsigned short housekeeping;

    if (idam_plugin_interface->interfaceVersion == 1) {

        idam_plugin_interface->pluginVersion = 1;

        data_block = idam_plugin_interface->data_block;
        request_block = idam_plugin_interface->request_block;

        housekeeping = idam_plugin_interface->housekeeping;
    } else {
        err = 999;
        idamLog(LOG_ERROR, "ERROR equimap: Plugin Interface Version Unknown\n");

        addIdamError(&idamerrorstack, CODEERRORTYPE, "equimap", err,
                     "Plugin Interface Version is Not Known: Unable to execute the request!");
        concatIdamError(idamerrorstack, idamErrorStack);
        return err;
    }

//----------------------------------------------------------------------------------------
// Ping - am I here?

    if (!strcasecmp(request_block->function, "ping")) {
        initDataBlock(data_block);
        data_block->rank = 0;
        data_block->data_n = strlen("equimap pinged!") + 1;
        data_block->data_type = TYPE_CHAR;
        data_block->data = malloc(data_block->data_n * sizeof(char));
        strcpy(data_block->data, "equimap pinged!");
        return 0;
    }

//----------------------------------------------------------------------------------------
// Help: A Description of library functionality

    if (!strcasecmp(request_block->function, "help")) {
        initDataBlock(data_block);
        data_block->rank = 0;
        data_block->data_n = strlen("psiRZBox Enabled!") + 1;
        data_block->data_type = TYPE_CHAR;
        data_block->data = malloc(data_block->data_n * sizeof(char));
        strcpy(data_block->data, "psiRZBox Enabled!");
        return 0;
    }

//----------------------------------------------------------------------------------------
// Heap Housekeeping

    if (housekeeping || !strcasecmp(request_block->function, "reset")) {

        if (!init) return 0;        // Not previously initialised: Nothing to do!

        if (prior_exp_number == -1) initEquiMapData();

// Free Heap & reset counters

        freeEquiMapData();

        init = 0;
        prior_exp_number = 0;
        prior_file[0] = '\0';
        smoothedPsi = 0;

        if (equimapdata.efitdata != NULL) free((void*) equimapdata.efitdata);

        return 0;
    }

    if (request_block->exp_number != prior_exp_number || strcmp(request_block->file, prior_file) != 0) {

// Free Heap & reset counters

        freeEquiMapData();

        init = 0;
        smoothedPsi = 0;
    }

//----------------------------------------------------------------------------------------
// Initialise: Define the fixed grid, read the raw data, and set the time vector
//             Read additional data relevant to the ITM

// Set the number of flux surfaces using the name value pair: fluxSurfaceCount = int

// The user has a choice of flux surface label: One must be selected

    if (!init || !strcasecmp(request_block->function, "init")
        || !strcasecmp(request_block->function, "initialise")) {

        initEquiMapData();    // initialise the data structure

// Read the ITM Data set ?

        for (i = 0; i < request_block->nameValueList.pairCount; i++) {
            if (!strcasecmp(request_block->nameValueList.nameValue[i].name, "readITMData")) {
                equimapdata.readITMData = 1;
                equimapdata.rhoType = NORMALISEDITMFLUXRADIUS;    // ITM Default type
                break;
            }
        }

// Number of Flux Surfaces

        for (i = 0; i < request_block->nameValueList.pairCount; i++) {
            if (!strcasecmp(request_block->nameValueList.nameValue[i].name, "fluxSurfaceCount")) {
                equimapdata.rhoBCount = atoi(request_block->nameValueList.nameValue[i].value);
                equimapdata.rhoCount = equimapdata.rhoBCount - 1;
                break;
            }
        }

// Identify Flux Surface label type: Mandatory requirement

        for (i = 0; i < request_block->nameValueList.pairCount; i++) {
            if (!strcasecmp(request_block->nameValueList.nameValue[i].name, "fluxSurfaceLabel")) {
                if (!strcasecmp(request_block->nameValueList.nameValue[i].value, "SQRTNORMALISEDTOROIDALFLUX")) {
                    equimapdata.rhoType = SQRTNORMALISEDTOROIDALFLUX;
                    break;
                } else if (!strcasecmp(request_block->nameValueList.nameValue[i].value, "NORMALISEDPOLOIDALFLUX")) {
                    equimapdata.rhoType = NORMALISEDPOLOIDALFLUX;
                    break;
                } else if (!strcasecmp(request_block->nameValueList.nameValue[i].value, "NORMALISEDITMFLUXRADIUS")) {
                    equimapdata.rhoType = NORMALISEDITMFLUXRADIUS;
                    break;
                }
                break;
            }
        }

// Test a Flux Surface Label has been selected

        if (equimapdata.rhoType == UNKNOWNCOORDINATETYPE) {
            err = 999;
            idamLog(LOG_ERROR, "ERROR equimap: No Flux Surface label type has been selected. "
                    "Use the fluxSurfaceLabel name-value pair argument to set it.\n");
            addIdamError(&idamerrorstack, CODEERRORTYPE, "equimap", err,
                         "No Flux Surface label type has been selected. "
                                 "Use the fluxSurfaceLabel name-value pair argument to set it.");
            concatIdamError(idamerrorstack, idamErrorStack);
            return err;
        }

// Preserve Shot Number// Number of Flux Surfaces

        if (request_block->exp_number == 0) {
            for (i = 0; i < request_block->nameValueList.pairCount; i++) {
                if (!strcasecmp(request_block->nameValueList.nameValue[i].name, "shot")) {
                    request_block->exp_number = atoi(request_block->nameValueList.nameValue[i].value);
                    break;
                }
            }
        }

        equimapdata.exp_number = request_block->exp_number;

// Create a normalised flux surface grid - no particular definition assumed - rhoType is used for Mapping

        equimapdata.rhoB = (float*) malloc(equimapdata.rhoBCount * sizeof(float));
        for (i = 0; i < equimapdata.rhoBCount; i++)
            equimapdata.rhoB[i] = (float) i / ((float) equimapdata.rhoBCount - 1);

        equimapdata.rho = (float*) malloc(equimapdata.rhoCount * sizeof(float));
        for (i = 0; i < equimapdata.rhoCount; i++)
            equimapdata.rho[i] = 0.5 * (equimapdata.rhoB[i] + equimapdata.rhoB[i + 1]);

        if (request_block->exp_number == 0 && request_block->file[0] == '\0') {
            err = 999;
            idamLog(LOG_ERROR, "ERROR equimap: No Shot Number or Private File!\n");
            addIdamError(&idamerrorstack, CODEERRORTYPE, "equimap", err, "No Shot Number or Private File!");
            concatIdamError(idamerrorstack, idamErrorStack);
            return err;
        }

        if ((err = importData(request_block, &equimapdata)) != 0) {
            //err = 999;
            idamLog(LOG_ERROR, "ERROR equimap: Problem importing data\n");
            addIdamError(&idamerrorstack, CODEERRORTYPE, "equimap", err, "Problem importing data");
            concatIdamError(idamerrorstack, idamErrorStack);
            return err;
        }

        if ((err = selectTimes(&equimapdata)) != 0) return err;        // Universal/Default set of times

        equimapdata.efitdata = (EFITDATA*) malloc(equimapdata.timeCount * sizeof(EFITDATA));
        for (i = 0; i < equimapdata.timeCount; i++) {
            initEfitData(&equimapdata.efitdata[i]);
            if ((err = extractData(equimapdata.times[i], &equimapdata.efitdata[i], &equimapdata)) != 0) return err;
        }

        init = 1;
        prior_exp_number = request_block->exp_number;        // Retain previous analysis shot to automatically re-initialise when different
        strcpy(prior_file, request_block->file);

        if (!strcasecmp(request_block->function, "init") || !strcasecmp(request_block->function, "initialise")) {
            initDataBlock(data_block);
            data_block->rank = 0;
            data_block->data_n = strlen("Initialisation Completed") + 1;
            data_block->data_type = TYPE_CHAR;
            data_block->data = malloc(data_block->data_n * sizeof(char));
            strcpy(data_block->data, "Initialisation Completed");
            return 0;
        }
    }

//----------------------------------------------------------------------------------------
// Processing over the Time domain
//----------------------------------------------------------------------------------------

    static float priorLimitRMaj = -1.0;

    for (i = 0; i < request_block->nameValueList.pairCount; i++) {

// Reduce the size of the psi grid to the minimum size enclosing the boundary
// Fixed grid so need to test all time points to establish the spatial range
// Data is processed once only - smoothing is not reversible!

        if (!strcasecmp(request_block->nameValueList.nameValue[i].name, "smoothPsi")) {

            idamLog(LOG_DEBUG, "EQUIMAP: processing time domain option 'smoothPsi'\n");

            int invert = 0;
            int limitPsi = 0;
            float limitRMaj = -1.0;
            for (j = 0; j < request_block->nameValueList.pairCount; j++) {
                if (!strcasecmp(request_block->nameValueList.nameValue[j].name, "invert")) invert = 1;
                if (!strcasecmp(request_block->nameValueList.nameValue[j].name, "limitPsi")) limitPsi = 1;
                if (!strcasecmp(request_block->nameValueList.nameValue[j].name, "limitRMaj"))
                    limitRMaj = (float) atof(request_block->nameValueList.nameValue[j].value);
            }

            idamLog(LOG_DEBUG, "EQUIMAP: smoothPsi(invert=%d, limitPsi=%d, limitRMaj=%f)\n", invert, limitPsi,
                    limitRMaj);

            if (!smoothedPsi) {
                smoothPsi(&equimapdata, invert, limitPsi, -1.0);        // Constrain by LCFS
                smoothedPsi = 1;
            }
            if (limitRMaj != -1.0 && limitRMaj != priorLimitRMaj) {
                smoothPsi(&equimapdata, invert, limitPsi, limitRMaj);        // Constrain by upper RMajor
                priorLimitRMaj = limitRMaj;
            }
            idamLog(LOG_DEBUG, "EQUIMAP: psiRZBox nr=%d, nz=%d)\n", equimapdata.efitdata[0].psiCountRZBox[0],
                    equimapdata.efitdata[0].psiCountRZBox[1]);
        }
    }

//----------------------------------------------------------------------------------------
// Functions
//----------------------------------------------------------------------------------------

    err = 0;

    do {

// Set the required Times via an ASCII Name value pair or subset

        if (!strcasecmp(request_block->function, "setTimes")) {    // User specifies a set of times
            if ((err = subsetTimes(request_block)) != 0) break;    // Subset
        } else

// Return the list of available times

        if (!strcasecmp(request_block->function, "listTimes")) {
            break;
        } else

// Limiter Coordinates (Not time dependent)

        if (!strcasecmp(request_block->function, "Rlim") ||
            !strcasecmp(request_block->function, "Zlim")) {

            initDataBlock(data_block);
            data_block->rank = 1;
            data_block->order = -1;

            data_block->dims = (DIMS*) malloc(data_block->rank * sizeof(DIMS));
            initDimBlock(&data_block->dims[0]);

            data_block->dims[0].dim_n = equimapdata.efitdata[0].nlim;
            data_block->dims[0].dim = NULL;
            data_block->dims[0].compressed = 1;
            data_block->dims[0].data_type = TYPE_FLOAT;
            data_block->dims[0].method = 0;
            data_block->dims[0].dim0 = 0.0;
            data_block->dims[0].diff = 1.0;
            data_block->dims[0].dim_units[0] = '\0';
            data_block->dims[0].dim_label[0] = '\0';

            data_block->data_n = data_block->dims[0].dim_n;
            data_block->data_type = TYPE_FLOAT;
            data_block->data = malloc(data_block->data_n * sizeof(float));
            arr = (float*) data_block->data;

            if (!strcasecmp(request_block->function, "Rlim")) {
                for (i = 0; i < data_block->data_n; i++) arr[i] = equimapdata.efitdata[0].rlim[i];
            } else if (!strcasecmp(request_block->function, "Zlim")) {
                for (i = 0; i < data_block->data_n; i++) arr[i] = equimapdata.efitdata[0].zlim[i];
            }

            if (!strcasecmp(request_block->function, "Rlim")) {
                handle = whichHandle("Rlim");
            } else if (!strcasecmp(request_block->function, "Zlim")) {
                handle = whichHandle("Zlim");
            }
            if (handle >= 0) {
                strcpy(data_block->data_units, getIdamDataUnits(handle));
                strcpy(data_block->data_label, getIdamDataLabel(handle));
                strcpy(data_block->data_desc, getIdamDataDesc(handle));
            }
            break;

        } else

// Rank 1 Time Series Data?

        if (!strcasecmp(request_block->function, "Rmin") ||
            !strcasecmp(request_block->function, "Rmax") ||
            !strcasecmp(request_block->function, "Rmag") ||
            !strcasecmp(request_block->function, "Zmag") ||
            !strcasecmp(request_block->function, "Bphi") ||
            !strcasecmp(request_block->function, "Bvac") ||
            !strcasecmp(request_block->function, "Rvac") ||
            !strcasecmp(request_block->function, "Ip") ||
            !strcasecmp(request_block->function, "psiBoundary") ||
            !strcasecmp(request_block->function, "psiMag") ||
            !strcasecmp(request_block->function, "Nlcfs") ||
            !strcasecmp(request_block->function, "Npsiz0") ||
            !strcasecmp(request_block->function, "rhotorb") ||

            (equimapdata.readITMData && !strcasecmp(request_block->function, "Rgeom")) ||
            (equimapdata.readITMData && !strcasecmp(request_block->function, "Zgeom")) ||
            (equimapdata.readITMData && !strcasecmp(request_block->function, "Aminor")) ||
            (equimapdata.readITMData && !strcasecmp(request_block->function, "TriangL")) ||
            (equimapdata.readITMData && !strcasecmp(request_block->function, "TriangU")) ||
            (equimapdata.readITMData && !strcasecmp(request_block->function, "Elong"))) {

            handle = whichHandle("Rmag");        // Provides Timing Labels only - not data

            initDataBlock(data_block);
            data_block->rank = 1;
            data_block->order = 0;

            data_block->dims = (DIMS*) malloc(data_block->rank * sizeof(DIMS));
            initDimBlock(&data_block->dims[0]);

// Time Dimension

            data_block->dims[0].dim_n = equimapdata.timeCount;
            data_block->dims[0].data_type = TYPE_FLOAT;
            data_block->dims[0].dim = malloc(data_block->dims[0].dim_n * sizeof(float));
            memcpy(data_block->dims[0].dim, equimapdata.times, data_block->dims[0].dim_n * sizeof(float));

            data_block->dims[0].compressed = 0;
            if (handle >= 0) {
                DIMS* tdim = getIdamDimBlock(handle, getIdamOrder(handle));
                strcpy(data_block->dims[0].dim_units, tdim->dim_units);
                strcpy(data_block->dims[0].dim_label, tdim->dim_label);
            } else {
                data_block->dims[0].dim_units[0] = '\0';
                data_block->dims[0].dim_label[0] = '\0';
            }

// Collect data trace labels

            handle = -1;

            data_block->data_n = data_block->dims[0].dim_n;

            data_block->data_type = TYPE_FLOAT;
            data_block->data = malloc(data_block->data_n * sizeof(float));
            arr = (float*) data_block->data;

            if (!strcasecmp(request_block->function, "Rmin")) {
                for (i = 0; i < equimapdata.timeCount; i++) arr[i] = equimapdata.efitdata[i].Rmin;
            } else if (!strcasecmp(request_block->function, "Rmax")) {
                for (i = 0; i < equimapdata.timeCount; i++) arr[i] = equimapdata.efitdata[i].Rmax;
            } else if (!strcasecmp(request_block->function, "Rmag")) {
                for (i = 0; i < equimapdata.timeCount; i++) arr[i] = equimapdata.efitdata[i].rmag;
            } else if (!strcasecmp(request_block->function, "Zmag")) {
                for (i = 0; i < equimapdata.timeCount; i++) arr[i] = equimapdata.efitdata[i].zmag;
            } else if (!strcasecmp(request_block->function, "Bphi")) {
                for (i = 0; i < equimapdata.timeCount; i++) arr[i] = equimapdata.efitdata[i].bphi;
            } else if (!strcasecmp(request_block->function, "Bvac")) {
                for (i = 0; i < equimapdata.timeCount; i++) arr[i] = equimapdata.efitdata[i].bvac;
            } else if (!strcasecmp(request_block->function, "Rvac")) {
                for (i = 0; i < equimapdata.timeCount; i++) arr[i] = equimapdata.efitdata[i].rvac;
            } else if (!strcasecmp(request_block->function, "Ip")) {
                for (i = 0; i < equimapdata.timeCount; i++) arr[i] = equimapdata.efitdata[i].ip;
            } else if (!strcasecmp(request_block->function, "psiBoundary")) {
                for (i = 0; i < equimapdata.timeCount; i++) arr[i] = equimapdata.efitdata[i].psi_bnd;
            } else if (!strcasecmp(request_block->function, "psiMag")) {
                for (i = 0; i < equimapdata.timeCount; i++) arr[i] = equimapdata.efitdata[i].psi_mag;
            } else if (!strcasecmp(request_block->function, "Nlcfs")) {
                data_block->data_type = TYPE_INT;
                int* iarr = (int*) data_block->data;
                for (i = 0; i < equimapdata.timeCount; i++) iarr[i] = equimapdata.efitdata[i].nlcfs;
            } else if (!strcasecmp(request_block->function, "Npsiz0")) {
                for (i = 0; i < equimapdata.timeCount; i++) arr[i] = equimapdata.efitdata[i].rz0Count;
            } else if (!strcasecmp(request_block->function, "rhotorb")) {
                for (i = 0; i < equimapdata.timeCount; i++) arr[i] = equimapdata.efitdata[i].rho_torb;
            } else if (!strcasecmp(request_block->function, "Rgeom")) {
                for (i = 0; i < equimapdata.timeCount; i++) arr[i] = equimapdata.efitdata[i].rgeom;
            } else if (!strcasecmp(request_block->function, "Zgeom")) {
                for (i = 0; i < equimapdata.timeCount; i++) arr[i] = equimapdata.efitdata[i].zgeom;
            } else if (!strcasecmp(request_block->function, "Aminor")) {
                for (i = 0; i < equimapdata.timeCount; i++) arr[i] = equimapdata.efitdata[i].aminor;
            } else if (!strcasecmp(request_block->function, "TriangL")) {
                for (i = 0; i < equimapdata.timeCount; i++) arr[i] = equimapdata.efitdata[i].triangL;
            } else if (!strcasecmp(request_block->function, "TriangU")) {
                for (i = 0; i < equimapdata.timeCount; i++) arr[i] = equimapdata.efitdata[i].triangU;
            } else if (!strcasecmp(request_block->function, "Elong")) {
                for (i = 0; i < equimapdata.timeCount; i++) arr[i] = equimapdata.efitdata[i].elong;
            } else

            if (!strcasecmp(request_block->function, "Rmin")) {
                strcpy(data_block->data_units, "m");
                strcpy(data_block->data_label, "Rmin");
                strcpy(data_block->data_desc, "Inner Boundary Radius");
            } else if (!strcasecmp(request_block->function, "Rmax")) {
                strcpy(data_block->data_units, "m");
                strcpy(data_block->data_label, "Rmax");
                strcpy(data_block->data_desc, "Outer Boundary Radius");
            } else if (!strcasecmp(request_block->function, "Rmag")) {
                strcpy(data_block->data_units, "m");
                strcpy(data_block->data_label, "Rmag");
                strcpy(data_block->data_desc, "Magentic Axis Radius");
            } else if (!strcasecmp(request_block->function, "Zmag")) {
                strcpy(data_block->data_units, "m");
                strcpy(data_block->data_label, "Zmag");
                strcpy(data_block->data_desc, "Magentic Axis Height");
            } else if (!strcasecmp(request_block->function, "Bphi")) {
                strcpy(data_block->data_units, "T");
                strcpy(data_block->data_label, "Bphi");
                strcpy(data_block->data_desc, "Toroidal Magnetic Field");
            } else if (!strcasecmp(request_block->function, "Bvac")) {
                strcpy(data_block->data_units, "T");
                strcpy(data_block->data_label, "Bvac");
                strcpy(data_block->data_desc, "Vacuum Toroidal Magnetic Field at reference radius");
            } else if (!strcasecmp(request_block->function, "Rvac")) {
                strcpy(data_block->data_units, "m");
                strcpy(data_block->data_label, "Rvac");
                strcpy(data_block->data_desc, "Reference Major Radius of Bvac");
            } else if (!strcasecmp(request_block->function, "Ip")) {
                strcpy(data_block->data_units, "A");
                strcpy(data_block->data_label, "Ip");
                strcpy(data_block->data_desc, "Toroidal Plasma Current");
            } else if (!strcasecmp(request_block->function, "psiBoundary")) {
                strcpy(data_block->data_units, "Wb");
                strcpy(data_block->data_label, "psiB");
                strcpy(data_block->data_desc, "Boundary Poloidal Magnetic Flux");
            } else if (!strcasecmp(request_block->function, "psiMag")) {
                strcpy(data_block->data_units, "Wb");
                strcpy(data_block->data_label, "psiMag");
                strcpy(data_block->data_desc, "Axial Poloidal Magnetic Flux");
            } else if (!strcasecmp(request_block->function, "Nlcfs")) {
                strcpy(data_block->data_units, "");
                strcpy(data_block->data_label, "Nlcfs");
                strcpy(data_block->data_desc, "Number of Coordinates in the LCFS Boundary");
            } else if (!strcasecmp(request_block->function, "Npsiz0")) {
                strcpy(data_block->data_units, "");
                strcpy(data_block->data_label, "Npsiz0");
                strcpy(data_block->data_desc, "Number of Coordinates in the Mid-Plane poloidal flux grid");
            } else if (!strcasecmp(request_block->function, "rhotorb")) {
                strcpy(data_block->data_units, "m");
                strcpy(data_block->data_label, "rho_torb");
                strcpy(data_block->data_desc, "ITM Toroidal Flux Radius at Boundary");
            } else

            if (!strcasecmp(request_block->function, "Rgeom")) {
                strcpy(data_block->data_units, "m");
                strcpy(data_block->data_label, "Rgeom");
                strcpy(data_block->data_desc, "Geometrical Axis of boundary (R)");
            } else if (!strcasecmp(request_block->function, "Zgeom")) {
                strcpy(data_block->data_units, "m");
                strcpy(data_block->data_label, "Zgeom");
                strcpy(data_block->data_desc, "Geometrical Axis of boundary (Z)");
            } else if (!strcasecmp(request_block->function, "Aminor")) {
                strcpy(data_block->data_units, "m");
                strcpy(data_block->data_label, "Aminor");
                strcpy(data_block->data_desc, "Minor Radius");
            } else if (!strcasecmp(request_block->function, "TriangL")) {
                strcpy(data_block->data_units, "");
                strcpy(data_block->data_label, "TriangL");
                strcpy(data_block->data_desc, "Lower Triagularity");
            } else if (!strcasecmp(request_block->function, "TriangU")) {
                strcpy(data_block->data_units, "");
                strcpy(data_block->data_label, "TriangU");
                strcpy(data_block->data_desc, "Upper Triagularity");
            } else if (!strcasecmp(request_block->function, "Elong")) {
                strcpy(data_block->data_units, "");
                strcpy(data_block->data_label, "Elong");
                strcpy(data_block->data_desc, "Elongation");
            } else {
                data_block->data_units[0] = '\0';
                data_block->data_label[0] = '\0';
                data_block->data_desc[0] = '\0';
            }

            break;
        }


// Rank 2 Equilibrium Profile Data  [time][rho]

        if (!strcasecmp(request_block->function, "psiCoord") ||
            !strcasecmp(request_block->function, "Phi") ||
            !strcasecmp(request_block->function, "Q") ||
            !strcasecmp(request_block->function, "PRho") ||
            !strcasecmp(request_block->function, "TRho") ||
            !strcasecmp(request_block->function, "RhoTor") ||
            !strcasecmp(request_block->function, "Rlcfs") ||
            !strcasecmp(request_block->function, "Zlcfs") ||
            (!strcasecmp(request_block->function, "P")) ||
            (!strcasecmp(request_block->function, "F")) ||
            (equimapdata.readITMData && !strcasecmp(request_block->function, "PPrime")) ||
            (equimapdata.readITMData && !strcasecmp(request_block->function, "FFPrime")) ||
            (equimapdata.readITMData && !strcasecmp(request_block->function, "ElongPsi")) ||
            (equimapdata.readITMData && !strcasecmp(request_block->function, "TriangLPsi")) ||
            (equimapdata.readITMData && !strcasecmp(request_block->function, "TriangUPsi")) ||
            (equimapdata.readITMData && !strcasecmp(request_block->function, "VolPsi")) ||
            (equimapdata.readITMData && !strcasecmp(request_block->function, "AreaPsi"))) {

            unsigned short lcfsData = 0;

            handle = whichHandle("Rmag");        // Provides Timing Labels only - not data

            initDataBlock(data_block);
            data_block->rank = 2;
            data_block->order = 1;

            data_block->dims = (DIMS*) malloc(data_block->rank * sizeof(DIMS));
            for (i = 0; i < data_block->rank; i++) initDimBlock(&data_block->dims[i]);

// Time Dimension

            data_block->dims[1].dim_n = equimapdata.timeCount;
            data_block->dims[1].data_type = TYPE_FLOAT;
            data_block->dims[1].dim = malloc(data_block->dims[1].dim_n * sizeof(float));
            memcpy(data_block->dims[1].dim, equimapdata.times, data_block->dims[1].dim_n * sizeof(float));

            data_block->dims[1].compressed = 0;
            if (handle >= 0) {
                DIMS* tdim = getIdamDimBlock(handle, getIdamOrder(handle));
                strcpy(data_block->dims[1].dim_units, tdim->dim_units);
                strcpy(data_block->dims[1].dim_label, tdim->dim_label);
            } else {
                data_block->dims[1].dim_units[0] = '\0';
                data_block->dims[1].dim_label[0] = '\0';
            }

// Flux Surface Label: Normalised poloidal flux

            handle = -1;
            if (!strcasecmp(request_block->function, "Q")) {
                handle = whichHandle("Q");
            } else if (!strcasecmp(request_block->function, "P")) {
                handle = whichHandle("P");
            } else if (!strcasecmp(request_block->function, "F")) {
                handle = whichHandle("F");
            } else if (!strcasecmp(request_block->function, "PPrime")) {
                handle = whichHandle("PPrime");
            } else if (!strcasecmp(request_block->function, "FFPrime")) {
                handle = whichHandle("FFPrime");
            } else if (!strcasecmp(request_block->function, "ElongPsi")) {
                handle = whichHandle("ElongPsi");
            } else if (!strcasecmp(request_block->function, "TriangLPsi")) {
                handle = whichHandle("TriangLPsi");
            } else if (!strcasecmp(request_block->function, "TriangUPsi")) {
                handle = whichHandle("TriangUPsi");
            } else if (!strcasecmp(request_block->function, "VolPsi")) {
                handle = whichHandle("VolPsi");
            } else if (!strcasecmp(request_block->function, "AreaPsi")) {
                handle = whichHandle("AreaPsi");
            } else if (!strcasecmp(request_block->function, "psiCoord") ||
                       !strcasecmp(request_block->function, "phi") ||
                       !strcasecmp(request_block->function, "PRho") ||        // poloidal Flux Label
                       !strcasecmp(request_block->function, "TRho") ||        // toroidal Flux Label
                       !strcasecmp(request_block->function, "RhoTor")) {        // ITM Normalised Flux Radius
                handle = whichHandle("Q");                    // Use dimension coordinate labels from Q
            } else if (!strcasecmp(request_block->function, "Rlcfs")) {
                lcfsData = 1;
                handle = whichHandle("Rlcfs");
            } else if (!strcasecmp(request_block->function, "Zlcfs")) {
                lcfsData = 1;
                handle = whichHandle("Zlcfs");
            }

            if (handle >= 0) {
                if (!lcfsData) {
                    DIMS* xdim = getIdamDimBlock(handle, 0);
                    data_block->dims[0].data_type = TYPE_FLOAT;
                    strcpy(data_block->dims[0].dim_units, xdim->dim_units);
                    strcpy(data_block->dims[0].dim_label, xdim->dim_label);
                    data_block->dims[0].dim_n = xdim->dim_n;
                    data_block->dims[0].dim = malloc(data_block->dims[0].dim_n * sizeof(float));
                    getIdamFloatDimData(handle, 0, (float*) data_block->dims[0].dim);

                    //memcpy(data_block->dims[0].dim, xdim->dim, data_block->dims[0].dim_n*sizeof(float));  // *** BUG - assumes FLOAT !

                    data_block->dims[0].compressed = 0;
                } else {
                    int maxn = 0;
                    for (i = 0; i < equimapdata.timeCount; i++)
                        if (maxn < equimapdata.efitdata[i].nlcfs)
                            maxn = equimapdata.efitdata[i].nlcfs;
                    data_block->dims[0].dim_n = maxn;
                    data_block->dims[0].dim = NULL;
                    data_block->dims[0].compressed = 1;
                    data_block->dims[0].data_type = TYPE_FLOAT;
                    data_block->dims[0].method = 0;
                    data_block->dims[0].dim0 = 0.0;
                    data_block->dims[0].diff = 1.0;
                    strcpy(data_block->dims[0].dim_units, "");
                    strcpy(data_block->dims[0].dim_label, "LCFS coordinate id");
                }
            }

// Data

            data_block->data_n = data_block->dims[0].dim_n * data_block->dims[1].dim_n;

            data_block->data_type = TYPE_FLOAT;
            data_block->data = malloc(data_block->data_n * sizeof(float));

            arr = (float*) data_block->data;
            if (!strcasecmp(request_block->function, "Q")) {
                for (i = 0; i < equimapdata.timeCount; i++) {
                    for (j = 0; j < data_block->dims[0].dim_n; j++) {
                        offset = i * data_block->dims[0].dim_n + j;
                        arr[offset] = equimapdata.efitdata[i].q[j];
                    }
                }
            } else if (!strcasecmp(request_block->function, "P")) {
                for (i = 0; i < equimapdata.timeCount; i++) {
                    for (j = 0; j < data_block->dims[0].dim_n; j++) {
                        offset = i * data_block->dims[0].dim_n + j;
                        arr[offset] = equimapdata.efitdata[i].p[j];
                    }
                }
            } else if (!strcasecmp(request_block->function, "F")) {
                for (i = 0; i < equimapdata.timeCount; i++) {
                    for (j = 0; j < data_block->dims[0].dim_n; j++) {
                        offset = i * data_block->dims[0].dim_n + j;
                        arr[offset] = equimapdata.efitdata[i].f[j];
                    }
                }
            } else if (!strcasecmp(request_block->function, "PPrime")) {
                for (i = 0; i < equimapdata.timeCount; i++) {
                    for (j = 0; j < data_block->dims[0].dim_n; j++) {
                        offset = i * data_block->dims[0].dim_n + j;
                        arr[offset] = equimapdata.efitdata[i].pprime[j];
                    }
                }
            } else if (!strcasecmp(request_block->function, "FFPrime")) {
                for (i = 0; i < equimapdata.timeCount; i++) {
                    for (j = 0; j < data_block->dims[0].dim_n; j++) {
                        offset = i * data_block->dims[0].dim_n + j;
                        arr[offset] = equimapdata.efitdata[i].ffprime[j];
                    }
                }
            } else if (!strcasecmp(request_block->function, "ElongPsi")) {
                for (i = 0; i < equimapdata.timeCount; i++) {
                    for (j = 0; j < data_block->dims[0].dim_n; j++) {
                        offset = i * data_block->dims[0].dim_n + j;
                        arr[offset] = equimapdata.efitdata[i].elongp[j];
                    }
                }
            } else if (!strcasecmp(request_block->function, "TriangLPsi")) {
                for (i = 0; i < equimapdata.timeCount; i++) {
                    for (j = 0; j < data_block->dims[0].dim_n; j++) {
                        offset = i * data_block->dims[0].dim_n + j;
                        arr[offset] = equimapdata.efitdata[i].trianglp[j];
                    }
                }
            } else if (!strcasecmp(request_block->function, "TriangUPsi")) {
                for (i = 0; i < equimapdata.timeCount; i++) {
                    for (j = 0; j < data_block->dims[0].dim_n; j++) {
                        offset = i * data_block->dims[0].dim_n + j;
                        arr[offset] = equimapdata.efitdata[i].triangup[j];
                    }
                }
            } else if (!strcasecmp(request_block->function, "VolPsi")) {
                for (i = 0; i < equimapdata.timeCount; i++) {
                    for (j = 0; j < data_block->dims[0].dim_n; j++) {
                        offset = i * data_block->dims[0].dim_n + j;
                        arr[offset] = equimapdata.efitdata[i].volp[j];
                    }
                }
            } else if (!strcasecmp(request_block->function, "AreaPsi")) {
                for (i = 0; i < equimapdata.timeCount; i++) {
                    for (j = 0; j < data_block->dims[0].dim_n; j++) {
                        offset = i * data_block->dims[0].dim_n + j;
                        arr[offset] = equimapdata.efitdata[i].areap[j];
                    }
                }
            } else if (!strcasecmp(request_block->function, "RhoTor")) {
                handle = -1;
                for (i = 0; i < equimapdata.timeCount; i++) {
                    for (j = 0; j < data_block->dims[0].dim_n; j++) {
                        offset = i * data_block->dims[0].dim_n + j;
                        arr[offset] = equimapdata.efitdata[i].rho_tor[j];
                    }
                }
            } else if (!strcasecmp(request_block->function, "PRho")) {
                handle = -1;
                for (i = 0; i < equimapdata.timeCount; i++) {
                    for (j = 0; j < data_block->dims[0].dim_n; j++) {
                        offset = i * data_block->dims[0].dim_n + j;
                        arr[offset] = equimapdata.efitdata[i].rho[j];
                    }
                }
            } else if (!strcasecmp(request_block->function, "TRho")) {
                handle = -1;
                for (i = 0; i < equimapdata.timeCount; i++) {
                    for (j = 0; j < data_block->dims[0].dim_n; j++) {
                        offset = i * data_block->dims[0].dim_n + j;
                        arr[offset] = equimapdata.efitdata[i].trho[j];
                    }
                }
            } else if (!strcasecmp(request_block->function, "PsiCoord")) {
                handle = -1;
                for (i = 0; i < equimapdata.timeCount; i++) {
                    for (j = 0; j < data_block->dims[0].dim_n; j++) {
                        offset = i * data_block->dims[0].dim_n + j;
                        arr[offset] = equimapdata.efitdata[i].psi[j];
                    }
                }
            } else if (!strcasecmp(request_block->function, "Phi")) {
                handle = -1;
                for (i = 0; i < equimapdata.timeCount; i++) {
                    for (j = 0; j < data_block->dims[0].dim_n; j++) {
                        offset = i * data_block->dims[0].dim_n + j;
                        arr[offset] = equimapdata.efitdata[i].phi[j];
                    }
                }
            } else if (!strcasecmp(request_block->function, "Rlcfs")) {
                int maxn = data_block->dims[0].dim_n;
                for (i = 0; i < equimapdata.timeCount; i++) {
                    for (j = 0; j < equimapdata.efitdata[i].nlcfs; j++) {
                        offset = i * maxn + j;
                        arr[offset] = equimapdata.efitdata[i].rlcfs[j];
                    }
                    for (j = equimapdata.efitdata[i].nlcfs; j < maxn; j++) {
                        offset = i * maxn + j;
                        arr[offset] = 0.0;
                    }
                }
            } else if (!strcasecmp(request_block->function, "Zlcfs")) {
                int maxn = data_block->dims[0].dim_n;
                for (i = 0; i < equimapdata.timeCount; i++) {
                    for (j = 0; j < equimapdata.efitdata[i].nlcfs; j++) {
                        offset = i * maxn + j;
                        arr[offset] = equimapdata.efitdata[i].zlcfs[j];
                    }
                    for (j = equimapdata.efitdata[i].nlcfs; j < maxn; j++) {
                        offset = i * maxn + j;
                        arr[offset] = 0.0;
                    }
                }
            }

            if (handle >= 0) {
                strcpy(data_block->data_units, getIdamDataUnits(handle));
                strcpy(data_block->data_label, getIdamDataLabel(handle));
                strcpy(data_block->data_desc, getIdamDataDesc(handle));
            } else {
                if (!strcasecmp(request_block->function, "PsiCoord")) {
                    strcpy(data_block->data_units, "Wb");
                    strcpy(data_block->data_label, "Psi");
                    strcpy(data_block->data_desc, "Poloidal Flux Coordinate");
                } else if (!strcasecmp(request_block->function, "Phi")) {
                    strcpy(data_block->data_units, "Wb");
                    strcpy(data_block->data_label, "Phi");
                    strcpy(data_block->data_desc, "Toroidal Flux Coordinate");
                } else if (!strcasecmp(request_block->function, "PRho")) {
                    strcpy(data_block->data_units, "");
                    strcpy(data_block->data_label, "Rho");
                    strcpy(data_block->data_desc, "Normalised Poloidal Flux");
                } else if (!strcasecmp(request_block->function, "TRho")) {
                    strcpy(data_block->data_units, "");
                    strcpy(data_block->data_label, "TRho");
                    strcpy(data_block->data_desc, "SQRT Normalised Toroidal Flux");
                } else if (!strcasecmp(request_block->function, "RhoTor")) {
                    strcpy(data_block->data_units, "");
                    strcpy(data_block->data_label, "Rho_Tor");
                    strcpy(data_block->data_desc, "Normalised ITM Toroidal Flux Radius");
                } else if (!strcasecmp(request_block->function, "Rlcfs")) {
                    strcpy(data_block->data_units, "m");
                    strcpy(data_block->data_label, "Rlcfs");
                    strcpy(data_block->data_desc, "Major Radius of LCFS Boundary points");
                } else if (!strcasecmp(request_block->function, "Zlcfs")) {
                    strcpy(data_block->data_units, "m");
                    strcpy(data_block->data_label, "Zlcfs");
                    strcpy(data_block->data_desc, "Height above mid-plane of LCFS Boundary points");
                }

            }

            break;
        }


        if (!strcasecmp(request_block->function, "PsiZ0") ||
            !strcasecmp(request_block->function, "RPsiZ0")) {    // Generally ragged arrays !

            handle = whichHandle("psi");                // Provides Timing Labels only - not data

            initDataBlock(data_block);
            data_block->rank = 2;
            data_block->order = 1;

            data_block->dims = (DIMS*) malloc(data_block->rank * sizeof(DIMS));
            for (i = 0; i < data_block->rank; i++) initDimBlock(&data_block->dims[i]);

// Time Dimension

            data_block->dims[1].dim_n = equimapdata.timeCount;
            data_block->dims[1].data_type = TYPE_FLOAT;
            data_block->dims[1].dim = malloc(data_block->dims[1].dim_n * sizeof(float));
            memcpy(data_block->dims[1].dim, equimapdata.times, data_block->dims[1].dim_n * sizeof(float));

            data_block->dims[1].compressed = 0;
            if (handle >= 0) {
                DIMS* tdim = getIdamDimBlock(handle, getIdamOrder(handle));
                strcpy(data_block->dims[1].dim_units, tdim->dim_units);
                strcpy(data_block->dims[1].dim_label, tdim->dim_label);
            } else {
                data_block->dims[1].dim_units[0] = '\0';
                data_block->dims[1].dim_label[0] = '\0';
            }

// Mid-Plane Major Radius - needs regularising to a fixed size - use boundary psi value to pack extra points

            int rz0CountMax = 0;

            for (i = 0; i < equimapdata.timeCount; i++) {
                if (equimapdata.efitdata[i].rz0Count > rz0CountMax) rz0CountMax = equimapdata.efitdata[i].rz0Count;
            }

            data_block->dims[0].dim_n = rz0CountMax;
            data_block->dims[0].data_type = TYPE_FLOAT;
            data_block->dims[0].dim = malloc(rz0CountMax * sizeof(float));

            data_block->dims[0].compressed = 1;
            data_block->dims[0].dim0 = 0.0;
            data_block->dims[0].diff = 1.0;
            data_block->dims[0].method = 0;
            strcpy(data_block->dims[0].dim_units, "");
            strcpy(data_block->dims[0].dim_label, "Ragged Radial Grid Index)");

// Data

            data_block->data_n = data_block->dims[0].dim_n * data_block->dims[1].dim_n;

            data_block->data_type = TYPE_FLOAT;
            data_block->data = malloc(data_block->data_n * sizeof(float));

            arr = (float*) data_block->data;

            if (!strcasecmp(request_block->function, "PsiZ0")) {
                for (i = 0; i < equimapdata.timeCount; i++) {
                    for (j = 0; j < equimapdata.efitdata[i].rz0Count; j++) {
                        offset = i * data_block->dims[0].dim_n + j;
                        arr[offset] = equimapdata.efitdata[i].psiz0[j];
                    }
                    if (rz0CountMax > equimapdata.efitdata[i].rz0Count) {
                        for (j = equimapdata.efitdata[i].rz0Count; j < rz0CountMax; j++) {
                            offset = i * data_block->dims[0].dim_n + j;
                            arr[offset] = equimapdata.efitdata[i].psiz0[equimapdata.efitdata[i].rz0Count - 1];
                        }
                    }
                }
            } else {
                for (i = 0; i < equimapdata.timeCount; i++) {
                    for (j = 0; j < equimapdata.efitdata[i].rz0Count; j++) {
                        offset = i * data_block->dims[0].dim_n + j;
                        arr[offset] = equimapdata.efitdata[i].rz0[j];
                    }
                    if (rz0CountMax > equimapdata.efitdata[i].rz0Count) {
                        for (j = equimapdata.efitdata[i].rz0Count; j < rz0CountMax; j++) {
                            offset = i * data_block->dims[0].dim_n + j;
                            arr[offset] = equimapdata.efitdata[i].rz0[equimapdata.efitdata[i].rz0Count - 1];
                        }
                    }
                }
            }

            if (!strcasecmp(request_block->function, "PsiZ0")) {
                if (handle >= 0) {
                    strcpy(data_block->data_units, getIdamDataUnits(handle));
                } else {
                    strcpy(data_block->data_units, "");
                }
                strcpy(data_block->data_label, "Psi(R,Z=0)");
                strcpy(data_block->data_desc, "Psi Profile (R,Z=0)");
            } else {
                strcpy(data_block->data_units, "m");
                strcpy(data_block->data_label, "R(Z=0)");
                strcpy(data_block->data_desc, "Mid-Plane Major Radii of Poloidal Flux");
            }
            break;
        }



// Rank 3 Equilibrium Profile Data

        if (!strcasecmp(request_block->function, "Psi") ||
            !strcasecmp(request_block->function, "Br") ||
            !strcasecmp(request_block->function, "Bz") ||
            !strcasecmp(request_block->function, "Bt") ||
            !strcasecmp(request_block->function, "Jphi")) {

            handle = whichHandle("Rmag");        // Provides Timing Labels only - not data

            initDataBlock(data_block);
            data_block->rank = 3;
            data_block->order = 2;

            data_block->dims = (DIMS*) malloc(data_block->rank * sizeof(DIMS));
            for (i = 0; i < data_block->rank; i++) initDimBlock(&data_block->dims[i]);

// Time Dimension

            data_block->dims[2].dim_n = equimapdata.timeCount;
            data_block->dims[2].data_type = TYPE_FLOAT;
            data_block->dims[2].dim = malloc(data_block->dims[2].dim_n * sizeof(float));
            memcpy(data_block->dims[2].dim, equimapdata.times, data_block->dims[2].dim_n * sizeof(float));

            data_block->dims[2].compressed = 0;
            if (handle >= 0) {
                DIMS* tdim = getIdamDimBlock(handle, getIdamOrder(handle));
                strcpy(data_block->dims[2].dim_units, tdim->dim_units);
                strcpy(data_block->dims[2].dim_label, tdim->dim_label);
            } else {
                data_block->dims[2].dim_units[0] = '\0';
                data_block->dims[2].dim_label[0] = '\0';
            }

// Spatial Coordinate Grid (R, Z)

            handle = whichHandle("psi");        // array[nt][nz][nr] = [2][1][0]

            if (handle >= 0) {
                DIMS* xdim = getIdamDimBlock(handle, 0);
                data_block->dims[0].dim_n = equimapdata.efitdata[0].psiCount[0];
                data_block->dims[0].data_type = TYPE_FLOAT;
                data_block->dims[0].dim = malloc(data_block->dims[0].dim_n * sizeof(float));
                memcpy(data_block->dims[0].dim, equimapdata.efitdata[0].rgrid,
                       data_block->dims[0].dim_n * sizeof(float));
                data_block->dims[0].compressed = 0;
                strcpy(data_block->dims[0].dim_units, xdim->dim_units);
                strcpy(data_block->dims[0].dim_label, xdim->dim_label);

                xdim = getIdamDimBlock(handle, 1);
                data_block->dims[1].dim_n = equimapdata.efitdata[0].psiCount[1];
                data_block->dims[1].data_type = TYPE_FLOAT;
                data_block->dims[1].dim = malloc(data_block->dims[1].dim_n * sizeof(float));
                memcpy(data_block->dims[1].dim, equimapdata.efitdata[0].zgrid,
                       data_block->dims[1].dim_n * sizeof(float));
                data_block->dims[1].compressed = 0;
                strcpy(data_block->dims[1].dim_units, xdim->dim_units);
                strcpy(data_block->dims[1].dim_label, xdim->dim_label);
            } else {
                err = 999;
                idamLog(LOG_ERROR, "ERROR equimap: Corrupted Psi Data\n");
                addIdamError(&idamerrorstack, CODEERRORTYPE, "equimap", err, "Corrupted Psi Data!");
                concatIdamError(idamerrorstack, idamErrorStack);
                return err;
            }

// Data

            data_block->data_n = data_block->dims[0].dim_n * data_block->dims[1].dim_n * data_block->dims[2].dim_n;

            data_block->data_type = TYPE_FLOAT;
            data_block->data = malloc(data_block->data_n * sizeof(float));

            arr = (float*) data_block->data;

            if (!strcasecmp(request_block->function, "Psi")) {
                for (i = 0; i < equimapdata.timeCount; i++) {
                    for (j = 0; j < data_block->dims[1].dim_n; j++) {
                        for (k = 0; k < data_block->dims[0].dim_n; k++) {
                            offset = j * data_block->dims[0].dim_n + k +
                                     i * data_block->dims[0].dim_n * data_block->dims[1].dim_n;
                            arr[offset] = equimapdata.efitdata[i].psig[j][k];
                        }
                    }
                }
            } else if (!strcasecmp(request_block->function, "Br")) {
                for (i = 0; i < equimapdata.timeCount; i++) {
                    for (j = 0; j < data_block->dims[1].dim_n; j++) {
                        for (k = 0; k < data_block->dims[0].dim_n; k++) {
                            offset = j * data_block->dims[0].dim_n + k +
                                     i * data_block->dims[0].dim_n * data_block->dims[1].dim_n;
                            arr[offset] = equimapdata.efitdata[i].Br[j][k];
                        }
                    }
                }
            } else if (!strcasecmp(request_block->function, "Bz")) {
                for (i = 0; i < equimapdata.timeCount; i++) {
                    for (j = 0; j < data_block->dims[1].dim_n; j++) {
                        for (k = 0; k < data_block->dims[0].dim_n; k++) {
                            offset = j * data_block->dims[0].dim_n + k +
                                     i * data_block->dims[0].dim_n * data_block->dims[1].dim_n;
                            arr[offset] = equimapdata.efitdata[i].Bz[j][k];
                        }
                    }
                }
            } else if (!strcasecmp(request_block->function, "Bt")) {
                for (i = 0; i < equimapdata.timeCount; i++) {
                    for (j = 0; j < data_block->dims[1].dim_n; j++) {
                        for (k = 0; k < data_block->dims[0].dim_n; k++) {
                            offset = j * data_block->dims[0].dim_n + k +
                                     i * data_block->dims[0].dim_n * data_block->dims[1].dim_n;
                            arr[offset] = equimapdata.efitdata[i].Bphi[j][k];
                        }
                    }
                }
            } else if (!strcasecmp(request_block->function, "Jphi")) {
                for (i = 0; i < equimapdata.timeCount; i++) {
                    for (j = 0; j < data_block->dims[1].dim_n; j++) {
                        for (k = 0; k < data_block->dims[0].dim_n; k++) {
                            offset = j * data_block->dims[0].dim_n + k +
                                     i * data_block->dims[0].dim_n * data_block->dims[1].dim_n;
                            arr[offset] = equimapdata.efitdata[i].Jphi[j][k];
                        }
                    }
                }
            }

            if (!strcasecmp(request_block->function, "Psi")) {
                if (handle >= 0) {
                    strcpy(data_block->data_units, getIdamDataUnits(handle));
                    strcpy(data_block->data_label, getIdamDataLabel(handle));
                    strcpy(data_block->data_desc, getIdamDataDesc(handle));
                } else {
                    strcpy(data_block->data_units, "");
                    strcpy(data_block->data_label, "Psi");
                    strcpy(data_block->data_desc, "Psi Surface");
                }
            } else if (!strcasecmp(request_block->function, "Br")) {
                strcpy(data_block->data_units, "T");
                strcpy(data_block->data_label, "Br");
                strcpy(data_block->data_desc, "Radial Magnetic Field");
            } else if (!strcasecmp(request_block->function, "Bz")) {
                strcpy(data_block->data_units, "T");
                strcpy(data_block->data_label, "Bz");
                strcpy(data_block->data_desc, "Vertical Magnetic Field");
            } else if (!strcasecmp(request_block->function, "Bt")) {
                strcpy(data_block->data_units, "T");
                strcpy(data_block->data_label, "Bphi");
                strcpy(data_block->data_desc, "Toroidal Magnetic Field");
            } else if (!strcasecmp(request_block->function, "Jphi")) {
                strcpy(data_block->data_units, "Am-2");
                strcpy(data_block->data_label, "Jphi");
                strcpy(data_block->data_desc, "Toroidal Current Density");
            }

            break;
        }

        if (!strcasecmp(request_block->function, "PsiSR") || !strcasecmp(request_block->function, "PsiRZBox")) {

            handle = whichHandle("Rmag");        // Provides Timing Labels only - not data

            initDataBlock(data_block);
            data_block->rank = 3;
            data_block->order = 2;

            data_block->dims = (DIMS*) malloc(data_block->rank * sizeof(DIMS));
            for (i = 0; i < data_block->rank; i++) initDimBlock(&data_block->dims[i]);

// Time Dimension

            data_block->dims[2].dim_n = equimapdata.timeCount;
            data_block->dims[2].data_type = TYPE_FLOAT;
            data_block->dims[2].dim = malloc(data_block->dims[2].dim_n * sizeof(float));
            memcpy(data_block->dims[2].dim, equimapdata.times, data_block->dims[2].dim_n * sizeof(float));

            data_block->dims[2].compressed = 0;
            if (handle >= 0) {
                DIMS* tdim = getIdamDimBlock(handle, getIdamOrder(handle));
                strcpy(data_block->dims[2].dim_units, tdim->dim_units);
                strcpy(data_block->dims[2].dim_label, tdim->dim_label);
            } else {
                data_block->dims[2].dim_units[0] = '\0';
                data_block->dims[2].dim_label[0] = '\0';
            }

// Spatial Coordinate Grid (R, Z)

            handle = whichHandle("psi");        // array[nt][nz][nr] = [2][1][0]

            if (handle >= 0) {
                DIMS* xdim = getIdamDimBlock(handle, 0);
                if (!strcasecmp(request_block->function, "PsiSR")) {
                    data_block->dims[0].dim_n = equimapdata.efitdata[0].psiCountSR[0];
                } else {
                    data_block->dims[0].dim_n = equimapdata.efitdata[0].psiCountRZBox[0];
                }
                data_block->dims[0].data_type = TYPE_FLOAT;
                data_block->dims[0].dim = malloc(data_block->dims[0].dim_n * sizeof(float));
                if (!strcasecmp(request_block->function, "PsiSR")) {
                    memcpy(data_block->dims[0].dim, equimapdata.efitdata[0].rgridSR,
                           data_block->dims[0].dim_n * sizeof(float));
                } else {
                    memcpy(data_block->dims[0].dim, equimapdata.efitdata[0].rgridRZBox,
                           data_block->dims[0].dim_n * sizeof(float));
                }
                data_block->dims[0].compressed = 0;
                strcpy(data_block->dims[0].dim_units, xdim->dim_units);
                strcpy(data_block->dims[0].dim_label, xdim->dim_label);

                xdim = getIdamDimBlock(handle, 1);
                if (!strcasecmp(request_block->function, "PsiSR")) {
                    data_block->dims[1].dim_n = equimapdata.efitdata[0].psiCountSR[1];
                } else {
                    data_block->dims[1].dim_n = equimapdata.efitdata[0].psiCountRZBox[1];
                }
                data_block->dims[1].data_type = TYPE_FLOAT;
                data_block->dims[1].dim = malloc(data_block->dims[1].dim_n * sizeof(float));
                if (!strcasecmp(request_block->function, "PsiSR")) {
                    memcpy(data_block->dims[1].dim, equimapdata.efitdata[0].zgridSR,
                           data_block->dims[1].dim_n * sizeof(float));
                } else {
                    memcpy(data_block->dims[1].dim, equimapdata.efitdata[0].zgridRZBox,
                           data_block->dims[1].dim_n * sizeof(float));
                }
                data_block->dims[1].compressed = 0;
                strcpy(data_block->dims[1].dim_units, xdim->dim_units);
                strcpy(data_block->dims[1].dim_label, xdim->dim_label);
            } else {
                err = 999;
                idamLog(LOG_ERROR, "ERROR equimap: Corrupted PsiSR Data\n");
                addIdamError(&idamerrorstack, CODEERRORTYPE, "equimap", err, "Corrupted PsiSR Data!");
                concatIdamError(idamerrorstack, idamErrorStack);
                return err;
            }

// Data

            data_block->data_n = data_block->dims[0].dim_n * data_block->dims[1].dim_n * data_block->dims[2].dim_n;

            if (data_block->data_n == 0) {
                err = 999;
                idamLog(LOG_ERROR, "ERROR equimap: No Data Values selected!\n");
                addIdamError(&idamerrorstack, CODEERRORTYPE, "equimap", err, "No Data Values selected!");
                concatIdamError(idamerrorstack, idamErrorStack);
                idamLog(LOG_DEBUG, "equimap: dims[0].dim_n = %d\n", data_block->dims[0].dim_n);
                idamLog(LOG_DEBUG, "equimap: dims[1].dim_n = %d\n", data_block->dims[1].dim_n);
                idamLog(LOG_DEBUG, "equimap: dims[2].dim_n = %d\n", data_block->dims[2].dim_n);
                return err;
            }


            data_block->data_type = TYPE_FLOAT;
            data_block->data = malloc(data_block->data_n * sizeof(float));

            arr = (float*) data_block->data;

            if (!strcasecmp(request_block->function, "PsiSR")) {
                for (i = 0; i < equimapdata.timeCount; i++) {
                    for (j = 0; j < data_block->dims[1].dim_n; j++) {
                        for (k = 0; k < data_block->dims[0].dim_n; k++) {
                            offset = j * data_block->dims[0].dim_n + k +
                                     i * data_block->dims[0].dim_n * data_block->dims[1].dim_n;
                            arr[offset] = equimapdata.efitdata[i].psigSR[j][k];
                        }
                    }
                }
            } else {
                for (i = 0; i < equimapdata.timeCount; i++) {
                    for (j = 0; j < data_block->dims[1].dim_n; j++) {
                        for (k = 0; k < data_block->dims[0].dim_n; k++) {
                            offset = j * data_block->dims[0].dim_n + k +
                                     i * data_block->dims[0].dim_n * data_block->dims[1].dim_n;
                            arr[offset] = equimapdata.efitdata[i].psigRZBox[j][k];
                        }
                    }
                }
            }

            if (handle >= 0) {
                strcpy(data_block->data_units, getIdamDataUnits(handle));
                strcpy(data_block->data_label, getIdamDataLabel(handle));
                strcpy(data_block->data_desc, getIdamDataDesc(handle));
            } else {
                strcpy(data_block->data_units, "");
                strcpy(data_block->data_label, "Psi");
                if (!strcasecmp(request_block->function, "PsiSR")) {
                    strcpy(data_block->data_desc, "Smoothed/Reduced Psi Surface");
                } else {
                    strcpy(data_block->data_desc, "R-Z Box constrained Psi Surface");
                }
            }

            break;
        }

// Experimental Data?

        if (!strcasecmp(request_block->function, "yag_psi") ||
            !strcasecmp(request_block->function, "yag_phi") ||
            !strcasecmp(request_block->function, "yag_prho") ||
            !strcasecmp(request_block->function, "yag_trho") ||
            !strcasecmp(request_block->function, "yag_rhotor") ||
            !strcasecmp(request_block->function, "yag_R") ||
            !strcasecmp(request_block->function, "yag_ne") ||
            !strcasecmp(request_block->function, "yag_Te")) {

            handle = whichHandle("EFM_MAGNETIC_AXIS_R");        // Provides Timing Labels only - not data

            initDataBlock(data_block);
            data_block->rank = 2;
            data_block->order = 1;

            data_block->dims = (DIMS*) malloc(data_block->rank * sizeof(DIMS));
            for (i = 0; i < data_block->rank; i++) initDimBlock(&data_block->dims[i]);

// Time Dimension

            data_block->dims[1].dim_n = equimapdata.timeCount;
            data_block->dims[1].data_type = TYPE_FLOAT;
            data_block->dims[1].dim = malloc(data_block->dims[1].dim_n * sizeof(float));
            memcpy(data_block->dims[1].dim, equimapdata.times, data_block->dims[1].dim_n * sizeof(float));

            data_block->dims[1].compressed = 0;
            if (handle >= 0) {
                DIMS* tdim = getIdamDimBlock(handle, getIdamOrder(handle));
                strcpy(data_block->dims[1].dim_units, tdim->dim_units);
                strcpy(data_block->dims[1].dim_label, tdim->dim_label);
            } else {
                data_block->dims[1].dim_units[0] = '\0';
                data_block->dims[1].dim_label[0] = '\0';
            }

// Flux Surface Label

            data_block->dims[0].dim_n = equimapdata.efitdata[0].nne;
            data_block->dims[0].data_type = TYPE_FLOAT;

            data_block->dims[0].dim = NULL;

            data_block->dims[0].compressed = 1;
            data_block->dims[0].dim0 = 0.0;
            data_block->dims[0].diff = 1.0;
            data_block->dims[0].method = 0;

            strcpy(data_block->dims[0].dim_units, "");
            strcpy(data_block->dims[0].dim_label, "Flux Surface Label)");

// Collect data traces

            handle = -1;

            if (!strcasecmp(request_block->function, "yag_R")) {
                handle = whichHandle("ayc_r");
            } else if (!strcasecmp(request_block->function, "yag_ne")) {
                handle = whichHandle("ayc_ne");
            } else if (!strcasecmp(request_block->function, "yag_Te")) {
                handle = whichHandle("ayc_Te");
            }

            data_block->data_n = data_block->dims[0].dim_n * data_block->dims[1].dim_n;

            data_block->data_type = TYPE_FLOAT;
            data_block->data = malloc(data_block->data_n * sizeof(float));

            for (i = 0; i < equimapdata.timeCount; i++) {
                if (!strcasecmp(request_block->function, "yag_R")) {
                    arr = (float*) data_block->data;
                    for (j = 0; j < data_block->dims[0].dim_n; j++) {
                        offset = i * data_block->dims[0].dim_n + j;
                        arr[offset] = equimapdata.efitdata[i].rne[j];
                    }
                } else if (!strcasecmp(request_block->function, "yag_ne")) {
                    arr = (float*) data_block->data;
                    for (j = 0; j < data_block->dims[0].dim_n; j++) {
                        offset = i * data_block->dims[0].dim_n + j;
                        arr[offset] = equimapdata.efitdata[i].ne[j];
                    }
                } else if (!strcasecmp(request_block->function, "yag_Te")) {
                    arr = (float*) data_block->data;
                    for (j = 0; j < data_block->dims[0].dim_n; j++) {
                        offset = i * data_block->dims[0].dim_n + j;
                        arr[offset] = equimapdata.efitdata[i].te[j];
                    }
                } else if (!strcasecmp(request_block->function, "yag_psi")) {
                    arr = (float*) data_block->data;
                    for (j = 0; j < data_block->dims[0].dim_n; j++) {
                        offset = i * data_block->dims[0].dim_n + j;
                        arr[offset] = equimapdata.efitdata[i].yagpsi[j];
                    }
                } else if (!strcasecmp(request_block->function, "yag_phi")) {
                    arr = (float*) data_block->data;
                    for (j = 0; j < data_block->dims[0].dim_n; j++) {
                        offset = i * data_block->dims[0].dim_n + j;
                        arr[offset] = equimapdata.efitdata[i].yagphi[j];
                    }
                } else if (!strcasecmp(request_block->function, "yag_trho")) {
                    arr = (float*) data_block->data;
                    for (j = 0; j < data_block->dims[0].dim_n; j++) {
                        offset = i * data_block->dims[0].dim_n + j;
                        arr[offset] = equimapdata.efitdata[i].yagtrho[j];
                    }
                } else if (!strcasecmp(request_block->function, "yag_prho")) {
                    arr = (float*) data_block->data;
                    for (j = 0; j < data_block->dims[0].dim_n; j++) {
                        offset = i * data_block->dims[0].dim_n + j;
                        arr[offset] = equimapdata.efitdata[i].yagprho[j];
                    }
                } else if (!strcasecmp(request_block->function, "yag_rhotor")) {
                    arr = (float*) data_block->data;
                    for (j = 0; j < data_block->dims[0].dim_n; j++) {
                        offset = i * data_block->dims[0].dim_n + j;
                        arr[offset] = equimapdata.efitdata[i].yagrhotor[j];
                    }
                }
            }

            if (handle >= 0) {
                strcpy(data_block->data_units, getIdamDataUnits(handle));
                strcpy(data_block->data_label, getIdamDataLabel(handle));
                strcpy(data_block->data_desc, getIdamDataDesc(handle));
            } else {
                if (!strcasecmp(request_block->function, "yag_psi")) {
                    strcpy(data_block->data_units, "Wb");
                    strcpy(data_block->data_label, "psi");
                    strcpy(data_block->data_desc, "Poloidal Flux");
                } else if (!strcasecmp(request_block->function, "yag_phi")) {
                    strcpy(data_block->data_units, "Wb");
                    strcpy(data_block->data_label, "phi");
                    strcpy(data_block->data_desc, "Toroidal Flux");
                } else if (!strcasecmp(request_block->function, "yag_trho")) {
                    strcpy(data_block->data_units, "");
                    strcpy(data_block->data_label, "trho");
                    strcpy(data_block->data_desc, "SQRT Normalised Toroidal Flux");
                } else if (!strcasecmp(request_block->function, "yag_prho")) {
                    strcpy(data_block->data_units, "");
                    strcpy(data_block->data_label, "rho");
                    strcpy(data_block->data_desc, "Normalised Poloidal Flux");
                } else if (!strcasecmp(request_block->function, "yag_rhotor")) {
                    strcpy(data_block->data_units, "");
                    strcpy(data_block->data_label, "rho_tor");
                    strcpy(data_block->data_desc, "Normalised ITM Toroidal Flux Radius");
                }
            }

            break;
        }

// Experimental Data Mapped to Fixed Grid? (Volume or Mid-Points)

        if (!strcasecmp(request_block->function, "MPsi") ||
            !strcasecmp(request_block->function, "MQ") ||
            !strcasecmp(request_block->function, "MYPsi") ||
            !strcasecmp(request_block->function, "MYPsi_inner") ||
            !strcasecmp(request_block->function, "MYPsi_outer") ||
            !strcasecmp(request_block->function, "MYPhi") ||
            !strcasecmp(request_block->function, "MYPhi_inner") ||
            !strcasecmp(request_block->function, "MYPhi_outer") ||
            !strcasecmp(request_block->function, "R_inner") ||
            !strcasecmp(request_block->function, "R_outer") ||
            !strcasecmp(request_block->function, "ne") ||
            !strcasecmp(request_block->function, "ne_inner") ||
            !strcasecmp(request_block->function, "ne_outer") ||
            !strcasecmp(request_block->function, "Te") ||
            !strcasecmp(request_block->function, "Te_inner") ||
            !strcasecmp(request_block->function, "Te_outer") ||
            (!strcasecmp(request_block->function, "MP")) ||
            (!strcasecmp(request_block->function, "MF")) ||
            (equimapdata.readITMData && !strcasecmp(request_block->function, "MPPrime")) ||
            (equimapdata.readITMData && !strcasecmp(request_block->function, "MFFPrime")) ||
            (equimapdata.readITMData && !strcasecmp(request_block->function, "MElong")) ||
            (equimapdata.readITMData && !strcasecmp(request_block->function, "MTriangL")) ||
            (equimapdata.readITMData && !strcasecmp(request_block->function, "MTriangU")) ||
            (equimapdata.readITMData && !strcasecmp(request_block->function, "MVol")) ||
            (equimapdata.readITMData && !strcasecmp(request_block->function, "MArea"))) {

            handle = whichHandle("Rmag");

            initDataBlock(data_block);
            data_block->rank = 2;
            data_block->order = 1;

            data_block->dims = (DIMS*) malloc(data_block->rank * sizeof(DIMS));
            for (i = 0; i < data_block->rank; i++) initDimBlock(&data_block->dims[i]);

// Time Dimension

            data_block->dims[1].dim_n = equimapdata.timeCount;
            data_block->dims[1].data_type = TYPE_FLOAT;
            data_block->dims[1].dim = malloc(data_block->dims[1].dim_n * sizeof(float));
            memcpy(data_block->dims[1].dim, equimapdata.times, data_block->dims[1].dim_n * sizeof(float));

            data_block->dims[1].compressed = 0;
            if (handle >= 0) {
                DIMS* tdim = getIdamDimBlock(handle, getIdamOrder(handle));
                strcpy(data_block->dims[1].dim_units, tdim->dim_units);
                strcpy(data_block->dims[1].dim_label, tdim->dim_label);
            } else {
                data_block->dims[1].dim_units[0] = '\0';
                data_block->dims[1].dim_label[0] = '\0';
            }

// Flux Surface Coordinate

            data_block->dims[0].dim_n = equimapdata.rhoCount;
            data_block->dims[0].data_type = TYPE_FLOAT;

            data_block->dims[0].dim = malloc(data_block->dims[0].dim_n * sizeof(float));
            memcpy(data_block->dims[0].dim, equimapdata.rho, data_block->dims[0].dim_n * sizeof(float));

            data_block->dims[0].compressed = 0;
            strcpy(data_block->dims[0].dim_units, "");

            switch (equimapdata.rhoType) {
                case (SQRTNORMALISEDTOROIDALFLUX): {
                    strcpy(data_block->dims[0].dim_label, "sqrt(Normalised Toroidal Flux)");
                    break;
                }
                case (NORMALISEDPOLOIDALFLUX): {
                    strcpy(data_block->dims[0].dim_label, "Normalised Poloidal Flux");
                    break;
                }
                case (NORMALISEDITMFLUXRADIUS): {
                    strcpy(data_block->dims[0].dim_label, "Normalised ITM Toroidal Flux Radius");
                    break;
                }
            }

// Collect data traces

            if (!strcasecmp(request_block->function, "R_inner") ||
                !strcasecmp(request_block->function, "R_outer")) {
                handle = whichHandle("ayc_r");
            } else if (!strcasecmp(request_block->function, "ne") ||
                       !strcasecmp(request_block->function, "ne_inner") ||
                       !strcasecmp(request_block->function, "ne_outer")) {
                handle = whichHandle("ayc_ne");
            } else if (!strcasecmp(request_block->function, "Te") ||
                       !strcasecmp(request_block->function, "Te_inner") ||
                       !strcasecmp(request_block->function, "Te_outer")) {
                handle = whichHandle("ayc_Te");
            } else if (!strcasecmp(request_block->function, "MPsi")) {
                handle = -1;
            } else if (!strcasecmp(request_block->function, "MQ")) {
                handle = whichHandle("Q");
            } else if (!strcasecmp(request_block->function, "MP")) {
                handle = whichHandle("P");
            } else if (!strcasecmp(request_block->function, "MF")) {
                handle = whichHandle("F");
            } else if (!strcasecmp(request_block->function, "MPPrime")) {
                handle = whichHandle("PPrime");
            } else if (!strcasecmp(request_block->function, "MFFPrime")) {
                handle = whichHandle("FFPrime");
            } else if (!strcasecmp(request_block->function, "MElong")) {
                handle = whichHandle("ElongPsi");
            } else if (!strcasecmp(request_block->function, "MTriangL")) {
                handle = whichHandle("TriangLPsi");
            } else if (!strcasecmp(request_block->function, "MTriangU")) {
                handle = whichHandle("TriangUPsi");
            } else if (!strcasecmp(request_block->function, "MVol")) {
                handle = whichHandle("VolPsi");
            } else if (!strcasecmp(request_block->function, "MArea")) {
                handle = whichHandle("AreaPsi");
            }

            data_block->data_n = data_block->dims[0].dim_n * data_block->dims[1].dim_n;

            data_block->data_type = TYPE_FLOAT;
            data_block->data = malloc(data_block->data_n * sizeof(float));

            arr = (float*) data_block->data;

            for (i = 0; i < equimapdata.timeCount; i++) {
                for (j = 0; j < data_block->dims[0].dim_n; j++) {
                    offset = i * data_block->dims[0].dim_n + j;
                    if (!strcasecmp(request_block->function, "R_inner")) {
                        arr[offset] = equimapdata.efitdata[i].mapyagr1[j];
                    } else if (!strcasecmp(request_block->function, "R_outer")) {
                        arr[offset] = equimapdata.efitdata[i].mapyagr2[j];
                    } else if (!strcasecmp(request_block->function, "ne")) {
                        arr[offset] = equimapdata.efitdata[i].mapyagne[j];
                    } else if (!strcasecmp(request_block->function, "ne_inner")) {
                        arr[offset] = equimapdata.efitdata[i].mapyagne1[j];
                    } else if (!strcasecmp(request_block->function, "ne_outer")) {
                        arr[offset] = equimapdata.efitdata[i].mapyagne2[j];
                    } else if (!strcasecmp(request_block->function, "Te")) {
                        arr[offset] = equimapdata.efitdata[i].mapyagte[j];
                    } else if (!strcasecmp(request_block->function, "Te_inner")) {
                        arr[offset] = equimapdata.efitdata[i].mapyagte1[j];
                    } else if (!strcasecmp(request_block->function, "Te_outer")) {
                        arr[offset] = equimapdata.efitdata[i].mapyagte2[j];
                    } else if (!strcasecmp(request_block->function, "MYPsi")) {
                        arr[offset] = equimapdata.efitdata[i].mapyagpsi[j];
                    } else if (!strcasecmp(request_block->function, "MYPsi_inner")) {
                        arr[offset] = equimapdata.efitdata[i].mapyagpsi1[j];
                    } else if (!strcasecmp(request_block->function, "MYPsi_outer")) {
                        arr[offset] = equimapdata.efitdata[i].mapyagpsi2[j];
                    } else if (!strcasecmp(request_block->function, "MYPhi")) {
                        arr[offset] = equimapdata.efitdata[i].mapyagphi[j];
                    } else if (!strcasecmp(request_block->function, "MYPhi_inner")) {
                        arr[offset] = equimapdata.efitdata[i].mapyagphi1[j];
                    } else if (!strcasecmp(request_block->function, "MYPhi_outer")) {
                        arr[offset] = equimapdata.efitdata[i].mapyagphi2[j];
                    } else if (!strcasecmp(request_block->function, "MPsi")) {
                        arr[offset] = equimapdata.efitdata[i].mappsi[j];
                    } else if (!strcasecmp(request_block->function, "MQ")) {
                        arr[offset] = equimapdata.efitdata[i].mapq[j];
                    } else if (!strcasecmp(request_block->function, "MP")) {
                        arr[offset] = equimapdata.efitdata[i].mapp[j];
                    } else if (!strcasecmp(request_block->function, "MF")) {
                        arr[offset] = equimapdata.efitdata[i].mapf[j];
                    } else if (!strcasecmp(request_block->function, "MPPrime")) {
                        arr[offset] = equimapdata.efitdata[i].mappprime[j];
                    } else if (!strcasecmp(request_block->function, "MFFPrime")) {
                        arr[offset] = equimapdata.efitdata[i].mapffprime[j];
                    } else if (!strcasecmp(request_block->function, "MElong")) {
                        arr[offset] = equimapdata.efitdata[i].mapelongp[j];
                    } else if (!strcasecmp(request_block->function, "MTriangL")) {
                        arr[offset] = equimapdata.efitdata[i].maptrianglp[j];
                    } else if (!strcasecmp(request_block->function, "MTriangU")) {
                        arr[offset] = equimapdata.efitdata[i].maptriangup[j];
                    } else if (!strcasecmp(request_block->function, "MVol")) {
                        arr[offset] = equimapdata.efitdata[i].mapvolp[j];
                    } else if (!strcasecmp(request_block->function, "MArea")) {
                        arr[offset] = equimapdata.efitdata[i].mapareap[j];
                    }
                }
            }

            if (handle >= 0) {
                strcpy(data_block->data_units, getIdamDataUnits(handle));
                strcpy(data_block->data_label, getIdamDataLabel(handle));
                strcpy(data_block->data_desc, getIdamDataDesc(handle));
            } else {
                if (!strcasecmp(request_block->function, "MPsi") ||
                    !strcasecmp(request_block->function, "MYPsi") ||
                    !strcasecmp(request_block->function, "MYPsi_inner") ||
                    !strcasecmp(request_block->function, "MYPsi_outer")) {
                    strcpy(data_block->data_units, "Wb");
                    strcpy(data_block->data_label, "psi");
                    strcpy(data_block->data_desc, "Poloidal Flux");
                } else if (!strcasecmp(request_block->function, "MYPhi") ||
                           !strcasecmp(request_block->function, "MYPhi_inner") ||
                           !strcasecmp(request_block->function, "MYPhi_outer")) {
                    strcpy(data_block->data_units, "Wb");
                    strcpy(data_block->data_label, "phi");
                    strcpy(data_block->data_desc, "Toroidal Flux");
                } else {
                    data_block->data_units[0] = '\0';
                    data_block->data_label[0] = '\0';
                    data_block->data_desc[0] = '\0';
                }
            }

            break;
        }

// Experimental Data Mapped to Fixed Grid? (Surface-Points)

        if (!strcasecmp(request_block->function, "MPsib") ||
            !strcasecmp(request_block->function, "MQb") ||
            !strcasecmp(request_block->function, "MYPsib") ||
            !strcasecmp(request_block->function, "MYPsib_inner") ||
            !strcasecmp(request_block->function, "MYPsib_outer") ||
            !strcasecmp(request_block->function, "MYPhib") ||
            !strcasecmp(request_block->function, "MYPhib_inner") ||
            !strcasecmp(request_block->function, "MYPhib_outer") ||
            !strcasecmp(request_block->function, "Rb_inner") ||
            !strcasecmp(request_block->function, "Rb_outer") ||
            !strcasecmp(request_block->function, "neb") ||
            !strcasecmp(request_block->function, "neb_inner") ||
            !strcasecmp(request_block->function, "neb_outer") ||
            !strcasecmp(request_block->function, "Teb") ||
            !strcasecmp(request_block->function, "Teb_inner") ||
            !strcasecmp(request_block->function, "Teb_outer") ||
            (!strcasecmp(request_block->function, "MPB")) ||
            (!strcasecmp(request_block->function, "MFB")) ||
            (equimapdata.readITMData && !strcasecmp(request_block->function, "MPPrimeB")) ||
            (equimapdata.readITMData && !strcasecmp(request_block->function, "MFFPrimeB")) ||
            (equimapdata.readITMData && !strcasecmp(request_block->function, "MElongB")) ||
            (equimapdata.readITMData && !strcasecmp(request_block->function, "MTriangLB")) ||
            (equimapdata.readITMData && !strcasecmp(request_block->function, "MTriangUB")) ||
            (equimapdata.readITMData && !strcasecmp(request_block->function, "MVolB")) ||
            (equimapdata.readITMData && !strcasecmp(request_block->function, "MAreaB"))) {


            handle = whichHandle("Rmag");

            initDataBlock(data_block);
            data_block->rank = 2;
            data_block->order = 1;

            data_block->dims = (DIMS*) malloc(data_block->rank * sizeof(DIMS));
            for (i = 0; i < data_block->rank; i++) initDimBlock(&data_block->dims[i]);

// Time Dimension

            data_block->dims[1].dim_n = equimapdata.timeCount;
            data_block->dims[1].data_type = TYPE_FLOAT;
            data_block->dims[1].dim = malloc(data_block->dims[1].dim_n * sizeof(float));
            memcpy(data_block->dims[1].dim, equimapdata.times, data_block->dims[1].dim_n * sizeof(float));

            data_block->dims[1].compressed = 0;
            if (handle >= 0) {
                DIMS* tdim = getIdamDimBlock(handle, getIdamOrder(handle));
                strcpy(data_block->dims[1].dim_units, tdim->dim_units);
                strcpy(data_block->dims[1].dim_label, tdim->dim_label);
            } else {
                data_block->dims[1].dim_units[0] = '\0';
                data_block->dims[1].dim_label[0] = '\0';
            }

// Normalised SQRT Toroidal Flux Dimension

            data_block->dims[0].dim_n = equimapdata.rhoBCount;
            data_block->dims[0].data_type = TYPE_FLOAT;

            data_block->dims[0].dim = malloc(data_block->dims[0].dim_n * sizeof(float));
            memcpy(data_block->dims[0].dim, equimapdata.rhoB, data_block->dims[0].dim_n * sizeof(float));

            data_block->dims[0].compressed = 0;
            strcpy(data_block->dims[0].dim_units, "");
            strcpy(data_block->dims[0].dim_label, "sqrt(Normalised Toroidal Flux)");

// Collect data traces

            if (!strcasecmp(request_block->function, "Rb_inner") ||
                !strcasecmp(request_block->function, "Rb_outer")) {
                handle = whichHandle("ayc_r");
            } else if (!strcasecmp(request_block->function, "neb") ||
                       !strcasecmp(request_block->function, "neb_inner") ||
                       !strcasecmp(request_block->function, "neb_outer")) {
                handle = whichHandle("ayc_ne");
            } else if (!strcasecmp(request_block->function, "Teb") ||
                       !strcasecmp(request_block->function, "Teb_inner") ||
                       !strcasecmp(request_block->function, "Teb_outer")) {
                handle = whichHandle("ayc_Te");
            } else if (!strcasecmp(request_block->function, "MPsiB")) {
                handle = -1;
            } else if (!strcasecmp(request_block->function, "MQB")) {
                handle = whichHandle("Q");
            } else if (!strcasecmp(request_block->function, "MPB")) {
                handle = whichHandle("P");
            } else if (!strcasecmp(request_block->function, "MFB")) {
                handle = whichHandle("B");
            } else if (!strcasecmp(request_block->function, "MPPrimeB")) {
                handle = whichHandle("PPrime");
            } else if (!strcasecmp(request_block->function, "MFFPrimeB")) {
                handle = whichHandle("FFPrime");
            } else if (!strcasecmp(request_block->function, "MElongB")) {
                handle = whichHandle("ElongPsi");
            } else if (!strcasecmp(request_block->function, "MTriangLB")) {
                handle = whichHandle("TriangLPsi");
            } else if (!strcasecmp(request_block->function, "MTriangUB")) {
                handle = whichHandle("TriangUPsi");
            } else if (!strcasecmp(request_block->function, "MVolB")) {
                handle = whichHandle("VolPsi");
            } else if (!strcasecmp(request_block->function, "MAreaB")) {
                handle = whichHandle("AreaPsi");
            }

            data_block->data_n = data_block->dims[0].dim_n * data_block->dims[1].dim_n;

            data_block->data_type = TYPE_FLOAT;
            data_block->data = malloc(data_block->data_n * sizeof(float));

            arr = (float*) data_block->data;

            for (i = 0; i < equimapdata.timeCount; i++) {
                for (j = 0; j < data_block->dims[0].dim_n; j++) {
                    offset = i * data_block->dims[0].dim_n + j;
                    if (!strcasecmp(request_block->function, "Rb_inner")) {
                        arr[offset] = equimapdata.efitdata[i].mapyagr1B[j];
                    } else if (!strcasecmp(request_block->function, "Rb_outer")) {
                        arr[offset] = equimapdata.efitdata[i].mapyagr2B[j];
                    } else if (!strcasecmp(request_block->function, "neb")) {
                        arr[offset] = equimapdata.efitdata[i].mapyagneB[j];
                    } else if (!strcasecmp(request_block->function, "neb_inner")) {
                        arr[offset] = equimapdata.efitdata[i].mapyagne1B[j];
                    } else if (!strcasecmp(request_block->function, "neb_outer")) {
                        arr[offset] = equimapdata.efitdata[i].mapyagne2B[j];
                    } else if (!strcasecmp(request_block->function, "Teb")) {
                        arr[offset] = equimapdata.efitdata[i].mapyagteB[j];
                    } else if (!strcasecmp(request_block->function, "Teb_inner")) {
                        arr[offset] = equimapdata.efitdata[i].mapyagte1B[j];
                    } else if (!strcasecmp(request_block->function, "Teb_outer")) {
                        arr[offset] = equimapdata.efitdata[i].mapyagte2B[j];
                    } else if (!strcasecmp(request_block->function, "MYPsib")) {
                        arr[offset] = equimapdata.efitdata[i].mapyagpsiB[j];
                    } else if (!strcasecmp(request_block->function, "MYPsib_inner")) {
                        arr[offset] = equimapdata.efitdata[i].mapyagpsi1B[j];
                    } else if (!strcasecmp(request_block->function, "MYPsib_outer")) {
                        arr[offset] = equimapdata.efitdata[i].mapyagpsi2B[j];
                    } else if (!strcasecmp(request_block->function, "MYPhib")) {
                        arr[offset] = equimapdata.efitdata[i].mapyagphiB[j];
                    } else if (!strcasecmp(request_block->function, "MYPhib_inner")) {
                        arr[offset] = equimapdata.efitdata[i].mapyagphi1B[j];
                    } else if (!strcasecmp(request_block->function, "MYPhib_outer")) {
                        arr[offset] = equimapdata.efitdata[i].mapyagphi2B[j];
                    } else if (!strcasecmp(request_block->function, "MPsib")) {
                        arr[offset] = equimapdata.efitdata[i].mappsiB[j];
                    } else if (!strcasecmp(request_block->function, "MQB")) {
                        arr[offset] = equimapdata.efitdata[i].mapqB[j];
                    } else if (!strcasecmp(request_block->function, "MPB")) {
                        arr[offset] = equimapdata.efitdata[i].mappB[j];
                    } else if (!strcasecmp(request_block->function, "MFB")) {
                        arr[offset] = equimapdata.efitdata[i].mapfB[j];
                    } else if (!strcasecmp(request_block->function, "MPPrimeB")) {
                        arr[offset] = equimapdata.efitdata[i].mappprimeB[j];
                    } else if (!strcasecmp(request_block->function, "MFFPrimeB")) {
                        arr[offset] = equimapdata.efitdata[i].mapffprimeB[j];
                    } else if (!strcasecmp(request_block->function, "MElongB")) {
                        arr[offset] = equimapdata.efitdata[i].mapelongpB[j];
                    } else if (!strcasecmp(request_block->function, "MTriangLB")) {
                        arr[offset] = equimapdata.efitdata[i].maptrianglpB[j];
                    } else if (!strcasecmp(request_block->function, "MTriangUB")) {
                        arr[offset] = equimapdata.efitdata[i].maptriangupB[j];
                    } else if (!strcasecmp(request_block->function, "MVolB")) {
                        arr[offset] = equimapdata.efitdata[i].mapvolpB[j];
                    } else if (!strcasecmp(request_block->function, "MAreaB")) {
                        arr[offset] = equimapdata.efitdata[i].mapareapB[j];
                    }
                }
            }

            if (handle >= 0) {
                strcpy(data_block->data_units, getIdamDataUnits(handle));
                strcpy(data_block->data_label, getIdamDataLabel(handle));
                strcpy(data_block->data_desc, getIdamDataDesc(handle));
            } else {
                if (!strcasecmp(request_block->function, "MPsiB") ||
                    !strcasecmp(request_block->function, "MYPsiB") ||
                    !strcasecmp(request_block->function, "MYPsiB_inner") ||
                    !strcasecmp(request_block->function, "MYPsiB_outer")) {
                    strcpy(data_block->data_units, "Wb");
                    strcpy(data_block->data_label, "psi");
                    strcpy(data_block->data_desc, "Poloidal Flux");
                } else if (!strcasecmp(request_block->function, "MYPhiB") ||
                           !strcasecmp(request_block->function, "MYPhiB_inner") ||
                           !strcasecmp(request_block->function, "MYPhiB_outer")) {
                    strcpy(data_block->data_units, "Wb");
                    strcpy(data_block->data_label, "phi");
                    strcpy(data_block->data_desc, "Toroidal Flux");
                } else {
                    data_block->data_units[0] = '\0';
                    data_block->data_label[0] = '\0';
                    data_block->data_desc[0] = '\0';
                }
            }

            break;
        }

// Fixed Grids

        if (!strcasecmp(request_block->function, "FRho") || !strcasecmp(request_block->function, "FRhoB")) {

            initDataBlock(data_block);
            data_block->rank = 1;
            data_block->order = -1;

            data_block->dims = (DIMS*) malloc(data_block->rank * sizeof(DIMS));
            for (i = 0; i < data_block->rank; i++) initDimBlock(&data_block->dims[i]);

// Array Index

            if (!strcasecmp(request_block->function, "FRho"))
                data_block->dims[0].dim_n = equimapdata.rhoCount;
            else
                data_block->dims[0].dim_n = equimapdata.rhoBCount;
            data_block->dims[0].data_type = TYPE_FLOAT;

            data_block->dims[0].dim = NULL;

            data_block->dims[0].compressed = 1;
            data_block->dims[0].dim0 = 0.0;
            data_block->dims[0].diff = 1.0;
            data_block->dims[0].method = 0;

            strcpy(data_block->dims[0].dim_units, "");
            strcpy(data_block->dims[0].dim_label, "Flux Surface Index");

            data_block->data_n = data_block->dims[0].dim_n;

            data_block->data_type = TYPE_FLOAT;
            data_block->data = malloc(data_block->data_n * sizeof(float));
            if (!strcasecmp(request_block->function, "FRho"))
                memcpy((void*) data_block->data, (void*) equimapdata.rho, equimapdata.rhoCount * sizeof(float));
            else
                memcpy((void*) data_block->data, (void*) equimapdata.rhoB, equimapdata.rhoBCount * sizeof(float));

            strcpy(data_block->data_units, "");

            switch (equimapdata.rhoType) {
                case (SQRTNORMALISEDTOROIDALFLUX): {
                    strcpy(data_block->data_desc, "Sqrt Normalised Toroidal Magnetic Flux at ");
                    break;
                }
                case (NORMALISEDPOLOIDALFLUX): {
                    strcpy(data_block->data_desc, "Normalised Poloidal Magnetic Flux at ");
                    break;
                }
                case (NORMALISEDITMFLUXRADIUS): {
                    strcpy(data_block->data_desc, "Normalised ITM Toroidal Flux Radius at ");
                    break;
                }
            }

            if (!strcasecmp(request_block->function, "FRho")) {
                strcpy(data_block->data_label, "Rho");
                strcat(data_block->data_desc, "Mid-Points");
            } else {
                strcpy(data_block->data_label, "RhoB");
                strcat(data_block->data_desc, "Surface-Points");
            }

            break;
        } else

// Fixed Grids: Create rank 2 array rho[t][x]	// Array Shape: data[2][1][0]

        if (!strcasecmp(request_block->function, "Rho") || !strcasecmp(request_block->function, "RhoB")) {

            initDataBlock(data_block);
            data_block->rank = 2;
            data_block->order = 1;

            data_block->dims = (DIMS*) malloc(data_block->rank * sizeof(DIMS));
            for (i = 0; i < data_block->rank; i++) initDimBlock(&data_block->dims[i]);

// Time Dimension

            handle = whichHandle("EFM_MAGNETIC_AXIS_R");        // Provides Timing Labels only - not data

            data_block->dims[1].dim_n = equimapdata.timeCount;
            data_block->dims[1].data_type = TYPE_FLOAT;
            data_block->dims[1].dim = malloc(data_block->dims[1].dim_n * sizeof(float));
            memcpy(data_block->dims[1].dim, equimapdata.times, data_block->dims[1].dim_n * sizeof(float));

            data_block->dims[1].compressed = 0;
            if (handle >= 1) {
                DIMS* tdim = getIdamDimBlock(handle, getIdamOrder(handle));
                strcpy(data_block->dims[1].dim_units, tdim->dim_units);
                strcpy(data_block->dims[1].dim_label, tdim->dim_label);
            } else {
                data_block->dims[1].dim_units[0] = '\0';
                data_block->dims[1].dim_label[0] = '\0';
            }

// Array Index

            if (!strcasecmp(request_block->function, "Rho"))
                data_block->dims[0].dim_n = equimapdata.rhoCount;
            else
                data_block->dims[0].dim_n = equimapdata.rhoBCount;
            data_block->dims[0].data_type = TYPE_FLOAT;

            data_block->dims[0].dim = NULL;

            data_block->dims[0].compressed = 1;
            data_block->dims[0].dim0 = 0.0;
            data_block->dims[0].diff = 1.0;
            data_block->dims[0].method = 0;

            strcpy(data_block->dims[0].dim_units, "");
            strcpy(data_block->dims[0].dim_label, "Flux Surface Index");

// Data

            data_block->data_n = data_block->dims[0].dim_n * data_block->dims[1].dim_n;

            data_block->data_type = TYPE_FLOAT;
            data_block->data = malloc(data_block->data_n * sizeof(float));
            arr = (float*) data_block->data;

            if (!strcasecmp(request_block->function, "Rho")) {
                for (i = 0; i < equimapdata.timeCount; i++) {
                    for (j = 0; j < equimapdata.rhoCount; j++) {
                        offset = i * equimapdata.rhoCount + j;
                        arr[offset] = equimapdata.rho[j];
                    }
                }
            } else {
                for (i = 0; i < equimapdata.timeCount; i++) {
                    for (j = 0; j < equimapdata.rhoBCount; j++) {
                        offset = i * equimapdata.rhoBCount + j;
                        arr[offset] = equimapdata.rhoB[j];
                    }
                }
            }

            switch (equimapdata.rhoType) {
                case (SQRTNORMALISEDTOROIDALFLUX): {
                    strcpy(data_block->data_desc, "Sqrt Normalised Toroidal Magnetic Flux at ");
                    break;
                }
                case (NORMALISEDPOLOIDALFLUX): {
                    strcpy(data_block->data_desc, "Normalised Poloidal Magnetic Flux at ");
                    break;
                }
                case (NORMALISEDITMFLUXRADIUS): {
                    strcpy(data_block->data_desc, "Normalised ITM Toroidal Flux Radius at ");
                    break;
                }
            }

            strcpy(data_block->data_units, "");
            if (!strcasecmp(request_block->function, "Rho")) {
                strcpy(data_block->data_label, "Rho");
                strcat(data_block->data_desc, "Mid-Points");
            } else {
                strcpy(data_block->data_label, "RhoB");
                strcat(data_block->data_desc, "Surface-Points");
            }

            break;

        } else

// Rank 2 Flux Surface Average Data  [time][rho]

        if (!strcasecmp(request_block->function, "mapgm0") ||
            !strcasecmp(request_block->function, "mapgm1") ||
            !strcasecmp(request_block->function, "mapgm2") ||
            !strcasecmp(request_block->function, "mapgm99") ||
            !strcasecmp(request_block->function, "mapgm3")) {

// ************ //fluxSurfaceAverage();

            handle = whichHandle("Rmag");        // Provides Timing Labels only - not data

            initDataBlock(data_block);
            data_block->rank = 2;
            data_block->order = 1;

            data_block->dims = (DIMS*) malloc(data_block->rank * sizeof(DIMS));
            for (i = 0; i < data_block->rank; i++) initDimBlock(&data_block->dims[i]);

// Time Dimension

            data_block->dims[1].dim_n = equimapdata.timeCount;
            data_block->dims[1].data_type = TYPE_FLOAT;
            data_block->dims[1].dim = malloc(data_block->dims[1].dim_n * sizeof(float));
            memcpy(data_block->dims[1].dim, equimapdata.times, data_block->dims[1].dim_n * sizeof(float));

            data_block->dims[1].compressed = 0;
            if (handle >= 0) {
                DIMS* tdim = getIdamDimBlock(handle, getIdamOrder(handle));
                strcpy(data_block->dims[1].dim_units, tdim->dim_units);
                strcpy(data_block->dims[1].dim_label, tdim->dim_label);
            } else {
                data_block->dims[1].dim_units[0] = '\0';
                data_block->dims[1].dim_label[0] = '\0';
            }

// Flux surface label

            data_block->dims[0].data_type = TYPE_FLOAT;
            strcpy(data_block->dims[0].dim_units, "");
            strcpy(data_block->dims[0].dim_label, "Rho");
            data_block->dims[0].dim_n = equimapdata.rhoCount;
            data_block->dims[0].dim = malloc(data_block->dims[0].dim_n * sizeof(float));
            memcpy(data_block->dims[0].dim, equimapdata.rho, data_block->dims[0].dim_n * sizeof(float));
            data_block->dims[0].compressed = 0;

            switch (equimapdata.rhoType) {
                case (SQRTNORMALISEDTOROIDALFLUX): {
                    strcpy(data_block->dims[0].dim_label, "sqrt(Normalised Toroidal Flux)");
                    break;
                }
                case (NORMALISEDPOLOIDALFLUX): {
                    strcpy(data_block->dims[0].dim_label, "Normalised Poloidal Flux");
                    break;
                }
                case (NORMALISEDITMFLUXRADIUS): {
                    strcpy(data_block->dims[0].dim_label, "Normalised ITM Toroidal Flux Radius");
                    break;
                }
            }

// Data

            data_block->data_n = data_block->dims[0].dim_n * data_block->dims[1].dim_n;

            data_block->data_type = TYPE_FLOAT;
            data_block->data = malloc(data_block->data_n * sizeof(float));
            arr = (float*) data_block->data;

            for (i = 0; i < equimapdata.timeCount; i++) {
                for (j = 0; j < data_block->dims[0].dim_n; j++) {
                    offset = i * data_block->dims[0].dim_n + j;
                    if (!strcasecmp(request_block->function, "mapgm0")) {
                        arr[offset] = equimapdata.fluxAverages[i].metrics.grho[j];
                    } else if (!strcasecmp(request_block->function, "mapgm1")) {
                        arr[offset] = equimapdata.fluxAverages[i].metrics.grho2[j];
                    } else if (!strcasecmp(request_block->function, "mapgm2")) {
                        arr[offset] = equimapdata.fluxAverages[i].metrics.gm2[j];
                    } else if (!strcasecmp(request_block->function, "mapgm3")) {
                        arr[offset] = equimapdata.fluxAverages[i].metrics.gm3[j];
                    }
                }
            }

            if (!strcasecmp(request_block->function, "mapgm0")) {
                strcpy(data_block->data_units, "m^-1");
                strcpy(data_block->data_label, "<|Grad Rho|>");
                strcpy(data_block->data_desc, "Flux Surface Average <|Grad Rho|>");

            } else if (!strcasecmp(request_block->function, "mapgm1")) {
                strcpy(data_block->data_units, "m^-2");
                strcpy(data_block->data_label, "<|Grad Rho|^2>");
                strcpy(data_block->data_desc, "Flux Surface Average <|Grad Rho|^2>");

            } else if (!strcasecmp(request_block->function, "mapgm2")) {
                strcpy(data_block->data_units, "m");
                strcpy(data_block->data_label, "<R>");
                strcpy(data_block->data_desc, "Flux Surface Average <R>");
            }
            else if (!strcasecmp(request_block->function, "mapgm3")) {
                strcpy(data_block->data_units, "m");
                strcpy(data_block->data_label, "<|Grad(Rho/R)|^2>");
                strcpy(data_block->data_desc, "Flux Surface Average <|Grad(Rho/R)|^2>");
            }

            break;
        }







// catch all?
// Experimental Data? Copy the IDAM data structure
// No ... double free unless data and coordinate data are copied.

        //if((handle = whichHandle(request_block->function)) >= 0){
        // *data_block = *getIdamDataBlock(handle);
        // break;
        //}


        err = 999;
        addIdamError(&idamerrorstack, CODEERRORTYPE, "equimap", err, "Unknown function requested!");
        concatIdamError(idamerrorstack, idamErrorStack);
        return err;

    } while (0);

    return err;
}

void initEquiMapData()
{
    int i;
    equimapdata.exp_number = 0;
    equimapdata.timeCount = 0;
    equimapdata.readITMData = 0;
    equimapdata.times = NULL;
    equimapdata.rhoType = SQRTNORMALISEDTOROIDALFLUX;
    equimapdata.rhoBCount = COORDINATECOUNT;
    equimapdata.rhoCount = equimapdata.rhoBCount - 1;
    equimapdata.rho = NULL;
    equimapdata.rhoB = NULL;
    equimapdata.efitdata = NULL;
    equimapdata.fluxAverages = NULL;
    handleCount = 0;
    for (i = 0; i < MAXHANDLES; i++) handles[i] = -1;
}

void initEfitData(EFITDATA* efitdata)
{
    efitdata->psi_bnd = 0.0;
    efitdata->psi_mag = 0.0;
    efitdata->rmag = 0.0;
    efitdata->zmag = 0.0;
    efitdata->ip = 0.0;
    efitdata->bphi = 0.0;
    efitdata->bvac = 0.0;
    efitdata->rvac = 0.0;
    efitdata->Rmin = 0.0;
    efitdata->Rmax = 0.0;

    efitdata->rgeom = 0.0;
    efitdata->zgeom = 0.0;
    efitdata->aminor = 0.0;
    efitdata->triangL = 0.0;
    efitdata->triangU = 0.0;
    efitdata->elong = 0.0;

    efitdata->nlcfs = 0;
    efitdata->rlcfs = NULL;
    efitdata->zlcfs = NULL;

    efitdata->nlim = 0;
    efitdata->rlim = NULL;
    efitdata->zlim = NULL;

    efitdata->psiCount[0] = 0;
    efitdata->psiCount[1] = 0;
    efitdata->psig = NULL;
    efitdata->rgrid = NULL;
    efitdata->zgrid = NULL;

    efitdata->psiCountSR[0] = 0;
    efitdata->psiCountSR[1] = 0;
    efitdata->psigSR = NULL;
    efitdata->rgridSR = NULL;
    efitdata->zgridSR = NULL;

    efitdata->psiCountRZBox[0] = 0;
    efitdata->psiCountRZBox[1] = 0;
    efitdata->psigRZBox = NULL;
    efitdata->rgridRZBox = NULL;
    efitdata->zgridRZBox = NULL;

    efitdata->dpsidr = NULL;
    efitdata->dpsidz = NULL;
    efitdata->Br = NULL;
    efitdata->Bz = NULL;
    efitdata->Bphi = NULL;
    efitdata->Jphi = NULL;

    efitdata->rz0Count = 0;
    efitdata->psiz0 = NULL;
    efitdata->rz0 = NULL;
    efitdata->qCount = 0;
    efitdata->q = NULL;
    efitdata->p = NULL;
    efitdata->f = NULL;
    efitdata->rho = NULL;
    efitdata->psi = NULL;
    efitdata->phi = NULL;
    efitdata->trho = NULL;
    efitdata->rho_torb = 1.0;
    efitdata->rho_tor = NULL;

    efitdata->pprime = NULL;
    efitdata->ffprime = NULL;
    efitdata->elongp = NULL;
    efitdata->trianglp = NULL;
    efitdata->triangup = NULL;
    efitdata->volp = NULL;
    efitdata->areap = NULL;

    efitdata->nne = 0;
    efitdata->ne = NULL;
    efitdata->te = NULL;
    efitdata->rne = NULL;

    efitdata->yagpsi = NULL;
    efitdata->yagtrho = NULL;
    efitdata->yagprho = NULL;
    efitdata->yagrhotor = NULL;

    efitdata->mappsi = NULL;
    efitdata->mappsiB = NULL;
    efitdata->mapq = NULL;
    efitdata->mapqB = NULL;
    efitdata->mapp = NULL;
    efitdata->mappB = NULL;
    efitdata->mapf = NULL;
    efitdata->mapfB = NULL;

    efitdata->mapgm0 = NULL;
    efitdata->mapgm1 = NULL;
    efitdata->mapgm2 = NULL;
    efitdata->mapgm3 = NULL;

    efitdata->mappprime = NULL;
    efitdata->mappprimeB = NULL;
    efitdata->mapffprime = NULL;
    efitdata->mapffprimeB = NULL;
    efitdata->mapelongp = NULL;
    efitdata->mapelongpB = NULL;
    efitdata->maptrianglp = NULL;
    efitdata->maptrianglpB = NULL;

    efitdata->maptriangup = NULL;
    efitdata->maptriangupB = NULL;
    efitdata->mapvolp = NULL;
    efitdata->mapvolpB = NULL;
    efitdata->mapareap = NULL;
    efitdata->mapareapB = NULL;

    efitdata->mapyagne = NULL;
    efitdata->mapyagte = NULL;
    efitdata->mapyagpsi = NULL;
    efitdata->mapyagphi = NULL;
    efitdata->mapyagr1 = NULL;
    efitdata->mapyagne1 = NULL;
    efitdata->mapyagte1 = NULL;
    efitdata->mapyagpsi1 = NULL;
    efitdata->mapyagphi1 = NULL;
    efitdata->mapyagr2 = NULL;
    efitdata->mapyagne2 = NULL;
    efitdata->mapyagte2 = NULL;
    efitdata->mapyagpsi2 = NULL;
    efitdata->mapyagphi2 = NULL;
    efitdata->mapyagneB = NULL;
    efitdata->mapyagteB = NULL;
    efitdata->mapyagpsiB = NULL;
    efitdata->mapyagphiB = NULL;
    efitdata->mapyagr1B = NULL;
    efitdata->mapyagne1B = NULL;
    efitdata->mapyagte1B = NULL;
    efitdata->mapyagpsi1B = NULL;
    efitdata->mapyagphi1B = NULL;
    efitdata->mapyagr2B = NULL;
    efitdata->mapyagne2B = NULL;
    efitdata->mapyagte2B = NULL;
    efitdata->mapyagpsi2B = NULL;
    efitdata->mapyagphi2B = NULL;
}

void freeEquiMapData()
{
    int i, j;

    for (i = 0; i < equimapdata.timeCount; i++) {
        if (equimapdata.efitdata[i].rlim != NULL) free((void*) equimapdata.efitdata[i].rlim);
        if (equimapdata.efitdata[i].zlim != NULL) free((void*) equimapdata.efitdata[i].zlim);
        if (equimapdata.efitdata[i].rlcfs != NULL) free((void*) equimapdata.efitdata[i].rlcfs);
        if (equimapdata.efitdata[i].zlcfs != NULL) free((void*) equimapdata.efitdata[i].zlcfs);
        if (equimapdata.efitdata[i].rgrid != NULL) free((void*) equimapdata.efitdata[i].rgrid);
        if (equimapdata.efitdata[i].zgrid != NULL) free((void*) equimapdata.efitdata[i].zgrid);
        if (equimapdata.efitdata[i].rgridSR != NULL) free((void*) equimapdata.efitdata[i].rgridSR);
        if (equimapdata.efitdata[i].zgridSR != NULL) free((void*) equimapdata.efitdata[i].zgridSR);
        if (equimapdata.efitdata[i].rgridRZBox != NULL) free((void*) equimapdata.efitdata[i].rgridRZBox);
        if (equimapdata.efitdata[i].zgridRZBox != NULL) free((void*) equimapdata.efitdata[i].zgridRZBox);

        if (equimapdata.efitdata[i].psiz0 != NULL) free((void*) equimapdata.efitdata[i].psiz0);
        if (equimapdata.efitdata[i].rz0 != NULL) free((void*) equimapdata.efitdata[i].rz0);
        if (equimapdata.efitdata[i].q != NULL) free((void*) equimapdata.efitdata[i].q);
        if (equimapdata.efitdata[i].p != NULL) free((void*) equimapdata.efitdata[i].p);
        if (equimapdata.efitdata[i].f != NULL) free((void*) equimapdata.efitdata[i].f);
        if (equimapdata.efitdata[i].rho != NULL) free((void*) equimapdata.efitdata[i].rho);
        if (equimapdata.efitdata[i].psi != NULL) free((void*) equimapdata.efitdata[i].psi);
        if (equimapdata.efitdata[i].phi != NULL) free((void*) equimapdata.efitdata[i].phi);
        if (equimapdata.efitdata[i].trho != NULL) free((void*) equimapdata.efitdata[i].trho);
        if (equimapdata.efitdata[i].rho_tor != NULL) free((void*) equimapdata.efitdata[i].rho_tor);

        if (equimapdata.efitdata[i].pprime != NULL) free((void*) equimapdata.efitdata[i].pprime);
        if (equimapdata.efitdata[i].ffprime != NULL) free((void*) equimapdata.efitdata[i].ffprime);
        if (equimapdata.efitdata[i].elongp != NULL) free((void*) equimapdata.efitdata[i].elongp);
        if (equimapdata.efitdata[i].trianglp != NULL) free((void*) equimapdata.efitdata[i].trianglp);
        if (equimapdata.efitdata[i].triangup != NULL) free((void*) equimapdata.efitdata[i].triangup);
        if (equimapdata.efitdata[i].volp != NULL) free((void*) equimapdata.efitdata[i].volp);
        if (equimapdata.efitdata[i].areap != NULL) free((void*) equimapdata.efitdata[i].areap);

        if (equimapdata.efitdata[i].ne != NULL) free((void*) equimapdata.efitdata[i].ne);
        if (equimapdata.efitdata[i].te != NULL) free((void*) equimapdata.efitdata[i].te);
        if (equimapdata.efitdata[i].rne != NULL) free((void*) equimapdata.efitdata[i].rne);
        if (equimapdata.efitdata[i].yagpsi != NULL) free((void*) equimapdata.efitdata[i].yagpsi);
        if (equimapdata.efitdata[i].yagphi != NULL) free((void*) equimapdata.efitdata[i].yagphi);
        if (equimapdata.efitdata[i].yagtrho != NULL) free((void*) equimapdata.efitdata[i].yagtrho);
        if (equimapdata.efitdata[i].yagprho != NULL) free((void*) equimapdata.efitdata[i].yagprho);
        if (equimapdata.efitdata[i].yagrhotor != NULL) free((void*) equimapdata.efitdata[i].yagrhotor);
        if (equimapdata.efitdata[i].mappsi != NULL) free((void*) equimapdata.efitdata[i].mappsi);
        if (equimapdata.efitdata[i].mappsiB != NULL) free((void*) equimapdata.efitdata[i].mappsiB);
        if (equimapdata.efitdata[i].mapq != NULL) free((void*) equimapdata.efitdata[i].mapq);
        if (equimapdata.efitdata[i].mapqB != NULL) free((void*) equimapdata.efitdata[i].mapqB);
        if (equimapdata.efitdata[i].mapp != NULL) free((void*) equimapdata.efitdata[i].mapp);
        if (equimapdata.efitdata[i].mappB != NULL) free((void*) equimapdata.efitdata[i].mappB);
        if (equimapdata.efitdata[i].mapf != NULL) free((void*) equimapdata.efitdata[i].mapf);
        if (equimapdata.efitdata[i].mapfB != NULL) free((void*) equimapdata.efitdata[i].mapfB);
        if (equimapdata.efitdata[i].mappprime != NULL) free((void*) equimapdata.efitdata[i].mappprime);
        if (equimapdata.efitdata[i].mappprimeB != NULL) free((void*) equimapdata.efitdata[i].mappprimeB);
        if (equimapdata.efitdata[i].mapffprime != NULL) free((void*) equimapdata.efitdata[i].mapffprime);
        if (equimapdata.efitdata[i].mapffprimeB != NULL) free((void*) equimapdata.efitdata[i].mapffprimeB);
        if (equimapdata.efitdata[i].mapelongp != NULL) free((void*) equimapdata.efitdata[i].mapelongp);
        if (equimapdata.efitdata[i].mapelongpB != NULL) free((void*) equimapdata.efitdata[i].mapelongpB);
        if (equimapdata.efitdata[i].maptrianglp != NULL) free((void*) equimapdata.efitdata[i].maptrianglp);
        if (equimapdata.efitdata[i].maptrianglpB != NULL) free((void*) equimapdata.efitdata[i].maptrianglpB);
        if (equimapdata.efitdata[i].maptriangup != NULL) free((void*) equimapdata.efitdata[i].maptriangup);
        if (equimapdata.efitdata[i].maptriangupB != NULL) free((void*) equimapdata.efitdata[i].maptriangupB);
        if (equimapdata.efitdata[i].mapvolp != NULL) free((void*) equimapdata.efitdata[i].mapvolp);
        if (equimapdata.efitdata[i].mapvolpB != NULL) free((void*) equimapdata.efitdata[i].mapvolpB);
        if (equimapdata.efitdata[i].mapareap != NULL) free((void*) equimapdata.efitdata[i].mapareap);
        if (equimapdata.efitdata[i].mapareapB != NULL) free((void*) equimapdata.efitdata[i].mapareapB);
        if (equimapdata.efitdata[i].mapgm0 != NULL) free((void*) equimapdata.efitdata[i].mapgm0);
        if (equimapdata.efitdata[i].mapgm1 != NULL) free((void*) equimapdata.efitdata[i].mapgm1);
        if (equimapdata.efitdata[i].mapgm2 != NULL) free((void*) equimapdata.efitdata[i].mapgm2);
        if (equimapdata.efitdata[i].mapgm3 != NULL) free((void*) equimapdata.efitdata[i].mapgm3);

        if (equimapdata.efitdata[i].mapyagne != NULL) free((void*) equimapdata.efitdata[i].mapyagne);
        if (equimapdata.efitdata[i].mapyagte != NULL) free((void*) equimapdata.efitdata[i].mapyagte);
        if (equimapdata.efitdata[i].mapyagpsi != NULL) free((void*) equimapdata.efitdata[i].mapyagpsi);
        if (equimapdata.efitdata[i].mapyagphi != NULL) free((void*) equimapdata.efitdata[i].mapyagphi);
        if (equimapdata.efitdata[i].mapyagr1 != NULL) free((void*) equimapdata.efitdata[i].mapyagr1);
        if (equimapdata.efitdata[i].mapyagne1 != NULL) free((void*) equimapdata.efitdata[i].mapyagne1);
        if (equimapdata.efitdata[i].mapyagte1 != NULL) free((void*) equimapdata.efitdata[i].mapyagte1);
        if (equimapdata.efitdata[i].mapyagpsi1 != NULL) free((void*) equimapdata.efitdata[i].mapyagpsi1);
        if (equimapdata.efitdata[i].mapyagphi1 != NULL) free((void*) equimapdata.efitdata[i].mapyagphi1);
        if (equimapdata.efitdata[i].mapyagr2 != NULL) free((void*) equimapdata.efitdata[i].mapyagr2);
        if (equimapdata.efitdata[i].mapyagne2 != NULL) free((void*) equimapdata.efitdata[i].mapyagne2);
        if (equimapdata.efitdata[i].mapyagte2 != NULL) free((void*) equimapdata.efitdata[i].mapyagte2);
        if (equimapdata.efitdata[i].mapyagpsi2 != NULL) free((void*) equimapdata.efitdata[i].mapyagpsi2);
        if (equimapdata.efitdata[i].mapyagphi2 != NULL) free((void*) equimapdata.efitdata[i].mapyagphi2);
        if (equimapdata.efitdata[i].mapyagneB != NULL) free((void*) equimapdata.efitdata[i].mapyagneB);
        if (equimapdata.efitdata[i].mapyagteB != NULL) free((void*) equimapdata.efitdata[i].mapyagteB);
        if (equimapdata.efitdata[i].mapyagpsiB != NULL) free((void*) equimapdata.efitdata[i].mapyagpsiB);
        if (equimapdata.efitdata[i].mapyagphiB != NULL) free((void*) equimapdata.efitdata[i].mapyagphiB);
        if (equimapdata.efitdata[i].mapyagr1B != NULL) free((void*) equimapdata.efitdata[i].mapyagr1B);
        if (equimapdata.efitdata[i].mapyagne1B != NULL) free((void*) equimapdata.efitdata[i].mapyagne1B);
        if (equimapdata.efitdata[i].mapyagte1B != NULL) free((void*) equimapdata.efitdata[i].mapyagte1B);
        if (equimapdata.efitdata[i].mapyagpsi1B != NULL) free((void*) equimapdata.efitdata[i].mapyagpsi1B);
        if (equimapdata.efitdata[i].mapyagphi1B != NULL) free((void*) equimapdata.efitdata[i].mapyagphi1B);
        if (equimapdata.efitdata[i].mapyagr2B != NULL) free((void*) equimapdata.efitdata[i].mapyagr2B);
        if (equimapdata.efitdata[i].mapyagne2B != NULL) free((void*) equimapdata.efitdata[i].mapyagne2B);
        if (equimapdata.efitdata[i].mapyagte2B != NULL) free((void*) equimapdata.efitdata[i].mapyagte2B);
        if (equimapdata.efitdata[i].mapyagpsi2B != NULL) free((void*) equimapdata.efitdata[i].mapyagpsi2B);
        if (equimapdata.efitdata[i].mapyagphi2B != NULL) free((void*) equimapdata.efitdata[i].mapyagphi2B);

        for (j = 0; j < equimapdata.efitdata[i].psiCount[1]; j++)
            if (equimapdata.efitdata[i].psig[j] != NULL) free((void*) equimapdata.efitdata[i].psig[j]);
        if (equimapdata.efitdata[i].psig != NULL) free((void*) equimapdata.efitdata[i].psig);

        for (j = 0; j < equimapdata.efitdata[i].psiCountSR[1]; j++)
            if (equimapdata.efitdata[i].psigSR[j] != NULL) free((void*) equimapdata.efitdata[i].psigSR[j]);
        if (equimapdata.efitdata[i].psigSR != NULL) free((void*) equimapdata.efitdata[i].psigSR);

        for (j = 0; j < equimapdata.efitdata[i].psiCountRZBox[1]; j++)
            if (equimapdata.efitdata[i].psigRZBox[j] != NULL) free((void*) equimapdata.efitdata[i].psigRZBox[j]);
        if (equimapdata.efitdata[i].psigRZBox != NULL) free((void*) equimapdata.efitdata[i].psigRZBox);

        for (j = 0; j < equimapdata.efitdata[i].psiCount[1]; j++)
            if (equimapdata.efitdata[i].Jphi[j] != NULL) free((void*) equimapdata.efitdata[i].Jphi[j]);
        if (equimapdata.efitdata[i].Jphi != NULL) free((void*) equimapdata.efitdata[i].Jphi);

        if (equimapdata.readITMData) {

            for (j = 0; j < equimapdata.efitdata[i].psiCount[1]; j++) {
                //if(equimapdata.efitdata[i].dpsidr[j] != NULL) free((void *)equimapdata.efitdata[i].dpsidr[j]);
                //if(equimapdata.efitdata[i].dpsidz[j] != NULL) free((void *)equimapdata.efitdata[i].dpsidz[j]);
                if (equimapdata.efitdata[i].Br[j] != NULL) free((void*) equimapdata.efitdata[i].Br[j]);
                if (equimapdata.efitdata[i].Bz[j] != NULL) free((void*) equimapdata.efitdata[i].Bz[j]);
                if (equimapdata.efitdata[i].Bphi[j] != NULL) free((void*) equimapdata.efitdata[i].Bphi[j]);
            }
            //if(equimapdata.efitdata[i].dpsidr != NULL) free((void *)equimapdata.efitdata[i].dpsidr);
            //if(equimapdata.efitdata[i].dpsidz != NULL) free((void *)equimapdata.efitdata[i].dpsidz);
            if (equimapdata.efitdata[i].Br != NULL) free((void*) equimapdata.efitdata[i].Br);
            if (equimapdata.efitdata[i].Bz != NULL) free((void*) equimapdata.efitdata[i].Bz);
            if (equimapdata.efitdata[i].Bphi != NULL) free((void*) equimapdata.efitdata[i].Bphi);
        }


        initEfitData(&equimapdata.efitdata[i]);

        if (equimapdata.readITMData) {

            if (equimapdata.fluxAverages == NULL) continue;

            if (equimapdata.fluxAverages[i].contours != NULL) {
                for (j = 0; j < equimapdata.rhoCount; j++) {
                    if (equimapdata.fluxAverages[i].contours[j].rcontour != NULL)
                        free((void*) equimapdata.fluxAverages[i].contours[j].rcontour);
                    if (equimapdata.fluxAverages[i].contours[j].zcontour != NULL)
                        free((void*) equimapdata.fluxAverages[i].contours[j].zcontour);
                }
                free((void*) equimapdata.fluxAverages[i].contours);
            }

            if (equimapdata.fluxAverages[i].scrunch != NULL) {
                for (j = 0; j < equimapdata.rhoCount; j++) {
                    if (equimapdata.fluxAverages[i].scrunch[j].rcos != NULL)
                        free((void*) equimapdata.fluxAverages[i].scrunch[j].rcos);
                    if (equimapdata.fluxAverages[i].scrunch[j].rsin != NULL)
                        free((void*) equimapdata.fluxAverages[i].scrunch[j].rsin);
                    if (equimapdata.fluxAverages[i].scrunch[j].zcos != NULL)
                        free((void*) equimapdata.fluxAverages[i].scrunch[j].zcos);
                    if (equimapdata.fluxAverages[i].scrunch[j].zsin != NULL)
                        free((void*) equimapdata.fluxAverages[i].scrunch[j].zsin);
                }
                free((void*) equimapdata.fluxAverages[i].scrunch);
            }

            for (j = 0; j < equimapdata.rhoCount; j++) {
                if (equimapdata.fluxAverages[i].metrics.drcosdrho[j] != NULL)
                    free((void*) equimapdata.fluxAverages[i].metrics.drcosdrho[j]);
                if (equimapdata.fluxAverages[i].metrics.dzcosdrho[j] != NULL)
                    free((void*) equimapdata.fluxAverages[i].metrics.dzcosdrho[j]);
                if (equimapdata.fluxAverages[i].metrics.drsindrho[j] != NULL)
                    free((void*) equimapdata.fluxAverages[i].metrics.drsindrho[j]);
                if (equimapdata.fluxAverages[i].metrics.dzsindrho[j] != NULL)
                    free((void*) equimapdata.fluxAverages[i].metrics.dzsindrho[j]);
                if (equimapdata.fluxAverages[i].metrics.r[j] != NULL)
                    free((void*) equimapdata.fluxAverages[i].metrics.r[j]);
                if (equimapdata.fluxAverages[i].metrics.z[j] != NULL)
                    free((void*) equimapdata.fluxAverages[i].metrics.z[j]);
                if (equimapdata.fluxAverages[i].metrics.drdrho[j] != NULL)
                    free((void*) equimapdata.fluxAverages[i].metrics.drdrho[j]);
                if (equimapdata.fluxAverages[i].metrics.dzdrho[j] != NULL)
                    free((void*) equimapdata.fluxAverages[i].metrics.dzdrho[j]);
                if (equimapdata.fluxAverages[i].metrics.drdtheta[j] != NULL)
                    free((void*) equimapdata.fluxAverages[i].metrics.drdtheta[j]);
                if (equimapdata.fluxAverages[i].metrics.dzdtheta[j] != NULL)
                    free((void*) equimapdata.fluxAverages[i].metrics.dzdtheta[j]);
                if (equimapdata.fluxAverages[i].metrics.d2[j] != NULL)
                    free((void*) equimapdata.fluxAverages[i].metrics.d2[j]);
                if (equimapdata.fluxAverages[i].metrics.gradrho[j] != NULL)
                    free((void*) equimapdata.fluxAverages[i].metrics.gradrho[j]);
                if (equimapdata.fluxAverages[i].metrics.gradrhoR2[j] != NULL)
                    free((void*) equimapdata.fluxAverages[i].metrics.gradrhoR2[j]);
                if (equimapdata.fluxAverages[i].metrics.dvdtheta[j] != NULL)
                    free((void*) equimapdata.fluxAverages[i].metrics.dvdtheta[j]);
            }

            if (equimapdata.fluxAverages[i].metrics.drcosdrho != NULL)
                free((void*) equimapdata.fluxAverages[i].metrics.drcosdrho);
            if (equimapdata.fluxAverages[i].metrics.dzcosdrho != NULL)
                free((void*) equimapdata.fluxAverages[i].metrics.dzcosdrho);
            if (equimapdata.fluxAverages[i].metrics.drsindrho != NULL)
                free((void*) equimapdata.fluxAverages[i].metrics.drsindrho);
            if (equimapdata.fluxAverages[i].metrics.dzsindrho != NULL)
                free((void*) equimapdata.fluxAverages[i].metrics.dzsindrho);
            if (equimapdata.fluxAverages[i].metrics.r != NULL)free((void*) equimapdata.fluxAverages[i].metrics.r);
            if (equimapdata.fluxAverages[i].metrics.z != NULL)free((void*) equimapdata.fluxAverages[i].metrics.z);
            if (equimapdata.fluxAverages[i].metrics.drdrho != NULL)
                free((void*) equimapdata.fluxAverages[i].metrics.drdrho);
            if (equimapdata.fluxAverages[i].metrics.dzdrho != NULL)
                free((void*) equimapdata.fluxAverages[i].metrics.dzdrho);
            if (equimapdata.fluxAverages[i].metrics.drdtheta != NULL)
                free((void*) equimapdata.fluxAverages[i].metrics.drdtheta);
            if (equimapdata.fluxAverages[i].metrics.dzdtheta != NULL)
                free((void*) equimapdata.fluxAverages[i].metrics.dzdtheta);
            if (equimapdata.fluxAverages[i].metrics.d2 != NULL)free((void*) equimapdata.fluxAverages[i].metrics.d2);
            if (equimapdata.fluxAverages[i].metrics.gradrho != NULL)
                free((void*) equimapdata.fluxAverages[i].metrics.gradrho);
            if (equimapdata.fluxAverages[i].metrics.gradrhoR2 != NULL)
                free((void*) equimapdata.fluxAverages[i].metrics.gradrhoR2);
            if (equimapdata.fluxAverages[i].metrics.dvdtheta != NULL)
                free((void*) equimapdata.fluxAverages[i].metrics.dvdtheta);

            if (equimapdata.fluxAverages[i].metrics.vprime != NULL)
                free((void*) equimapdata.fluxAverages[i].metrics.vprime);
            if (equimapdata.fluxAverages[i].metrics.xaprime != NULL)
                free((void*) equimapdata.fluxAverages[i].metrics.xaprime);
            if (equimapdata.fluxAverages[i].metrics.len != NULL) free((void*) equimapdata.fluxAverages[i].metrics.len);
            if (equimapdata.fluxAverages[i].metrics.sur != NULL) free((void*) equimapdata.fluxAverages[i].metrics.sur);
            if (equimapdata.fluxAverages[i].metrics.grho != NULL)
                free((void*) equimapdata.fluxAverages[i].metrics.grho);
            if (equimapdata.fluxAverages[i].metrics.grho2 != NULL)
                free((void*) equimapdata.fluxAverages[i].metrics.grho2);
            if (equimapdata.fluxAverages[i].metrics.volume != NULL)
                free((void*) equimapdata.fluxAverages[i].metrics.volume);
            if (equimapdata.fluxAverages[i].metrics.xarea != NULL)
                free((void*) equimapdata.fluxAverages[i].metrics.xarea);
            if (equimapdata.fluxAverages[i].metrics.gm2 != NULL) free((void*) equimapdata.fluxAverages[i].metrics.gm2);
            if (equimapdata.fluxAverages[i].metrics.gm3 != NULL) free((void*) equimapdata.fluxAverages[i].metrics.gm3);
        }
    }

    if (equimapdata.times != NULL) free((void*) equimapdata.times);
    if (equimapdata.rho != NULL) free((void*) equimapdata.rho);
    if (equimapdata.rhoB != NULL) free((void*) equimapdata.rhoB);
    if (equimapdata.efitdata != NULL) free((void*) equimapdata.efitdata);
    if (equimapdata.fluxAverages != NULL) free((void*) equimapdata.fluxAverages);

    for (i = 0; i < handleCount; i++) {
        idamFree(handles[i]);        // Free IDAM Heap
    }

    initEquiMapData();
}
