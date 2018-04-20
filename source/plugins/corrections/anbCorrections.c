/*---------------------------------------------------------------
* v1 IDAM Plugin: Standardised wrapper code around plugin functionality
*
* Input Arguments:	IDAM_PLUGIN_INTERFACE *idam_plugin_interface
*
* Returns:		0 if read was successful
*			otherwise a Error Code is returned 
*
* Calls			freeDataBlock	to free Heap memory if an Error Occurs
*
* Standard functionality:
*
*	help	    a description of what this plugin does together with a list of functions available
*	reset	    frees all previously allocated heap, closes file handles and resets all static parameters.
*		        This has the same functionality as setting the housekeeping directive in the plugin interface
*		        data structure to TRUE (1)
*	init	    Initialise the plugin: read all required data and process. Retain staticly for
*		        future reference.
*---------------------------------------------------------------------------------------------------------------*/
#include "anbCorrections.h"

#include <math.h>
#include <stdlib.h>
#include <string.h>

#include <clientserver/initStructs.h>
#include <clientserver/udaTypes.h>
#include <logging/logging.h>
#include <clientserver/errorLog.h>
#include <clientserver/freeDataBlock.h>
#include <clientserver/stringUtils.h>
#include <client/udaClient.h>
#include <client/accAPI.h>
#include <client/udaGetAPI.h>
#include <plugins/udaPlugin.h>

void makeDataBlock(DATA_BLOCK* out, int dataCount)
{
// Data are type FLOAT
// Coordinates are type DOUBLE
    int i;
    initDataBlock(out);
    float* data = (float*)malloc(dataCount * sizeof(float));
    for (i = 0; i < dataCount; i++) data[i] = (float)i;
    out->data = (char*)data;
    out->data_type = UDA_TYPE_FLOAT;
    out->rank = 1;
    out->order = 0;
    out->data_n = dataCount;
    strcpy(out->data_units, "Test Units");
    strcpy(out->data_label, "Test Label");
    strcpy(out->data_desc, "Test Description");

    float* errhi = (float*)malloc(dataCount * sizeof(float));
    for (i = 0; i < dataCount; i++) {
        errhi[i] = 0.1f * data[i];
    }
    out->errhi = (char*)errhi;
    out->error_type = UDA_TYPE_FLOAT;

    out->dims = (DIMS*)malloc(sizeof(DIMS));
    initDimBlock(out->dims);
    double* coords = (double*)malloc(dataCount * sizeof(double));
    for (i = 0; i < dataCount; i++) coords[i] = (double)i;
    out->dims[0].dim = (char*)coords;
    out->dims[0].data_type = UDA_TYPE_DOUBLE;
    out->dims[0].dim_n = dataCount;
    strcpy(out->dims[0].dim_units, "Test Units");
    strcpy(out->dims[0].dim_label, "Test Label");
}

void makeLegacyDataBlock(DATA_BLOCK* out)
{
// Data are type FLOAT
// Coordinates are type DOUBLE
    int dataCount = 2;
    int i;
    initDataBlock(out);
    float* data = (float*)malloc(dataCount * sizeof(float));
    for (i = 0; i < dataCount; i++) data[i] = 0.0;
    out->data = (char*)data;
    out->data_type = UDA_TYPE_FLOAT;
    out->rank = 1;
    out->order = 0;
    out->data_n = dataCount;
    strcpy(out->data_units, "MW");
    strcpy(out->data_label, "");
    strcpy(out->data_desc, "");

    out->dims = (DIMS*)malloc(sizeof(DIMS));
    initDimBlock(out->dims);
    double* coords = (double*)malloc(dataCount * sizeof(double));
    for (i = 0; i < dataCount; i++) coords[i] = (double)i;
    out->dims[0].dim = (char*)coords;
    out->dims[0].data_type = UDA_TYPE_DOUBLE;
    out->dims[0].dim_n = dataCount;
    strcpy(out->dims[0].dim_units, "s");
    strcpy(out->dims[0].dim_label, "Time");
}

void copyANBDataBlock(DATA_BLOCK* out, DATA_BLOCK* in, int dataCount)
{
// Data are type FLOAT
// Coordinates are type DOUBLE or FLOAT

    *out = *in;
    out->data = (char*)malloc(dataCount * sizeof(float));
    memcpy((void*)out->data, (void*)in->data, (size_t)dataCount * sizeof(float));

    out->errhi = (char*)malloc(dataCount * sizeof(float));
    memcpy((void*)out->errhi, (void*)in->errhi, (size_t)dataCount * sizeof(float));

    out->dims = (DIMS*)malloc(sizeof(DIMS));
    *(out->dims) = *(in->dims);
    out->dims[0].synthetic = NULL;
    out->dims[0].errhi = NULL;
    out->dims[0].errlo = NULL;
    if (in->dims[0].udoms > 0) {
        out->dims[0].sams = (int*)malloc(in->dims[0].udoms * sizeof(long));
        out->dims[0].offs = (char*)malloc(in->dims[0].udoms * sizeof(long));
        out->dims[0].ints = (char*)malloc(in->dims[0].udoms * sizeof(long));
        memcpy((void*)out->dims[0].sams, (void*)in->dims[0].sams, (size_t)in->dims[0].udoms * sizeof(long));
        memcpy((void*)out->dims[0].offs, (void*)in->dims[0].offs, (size_t)in->dims[0].udoms * sizeof(long));
        memcpy((void*)out->dims[0].ints, (void*)in->dims[0].ints, (size_t)in->dims[0].udoms * sizeof(long));
    }
    if (out->dims[0].data_type == UDA_TYPE_FLOAT) {
        out->dims[0].dim = (char*)malloc(dataCount * sizeof(float));
        memcpy((void*)out->dims[0].dim, (void*)in->dims[0].dim, (size_t)dataCount * sizeof(float));
    } else {
        out->dims[0].dim = (char*)malloc(dataCount * sizeof(double));
        memcpy((void*)out->dims[0].dim, (void*)in->dims[0].dim, (size_t)dataCount * sizeof(double));
    }
}

extern int anbCorrections(IDAM_PLUGIN_INTERFACE* idam_plugin_interface)
{
    int i, err;
    static short init = 0;
    char* p;
    int h1 = -1, h2 = -1, h3 = -1;
    static DATA_BLOCK data_block_e;
    static DATA_BLOCK data_block_e2;
    static DATA_BLOCK data_block_e3;
    static DATA_BLOCK data_block_t;
    static DATA_BLOCK data_block_tt;
    static int old_exp_number = 0;
    static int freeOldData = 0;        // Flag that data have been previously prepared and state preserved
    static int dataCountSW = 0;
    static int dataCountSS = 0;

//----------------------------------------------------------------------------------------
// Standard v1 Plugin Interface

    DATA_BLOCK* data_block;
    REQUEST_BLOCK* request_block;

    unsigned short housekeeping;

    if (idam_plugin_interface->interfaceVersion >= 1) {

        idam_plugin_interface->pluginVersion = 1;

        data_block = idam_plugin_interface->data_block;
        request_block = idam_plugin_interface->request_block;

        housekeeping = idam_plugin_interface->housekeeping;

    } else {
        RAISE_PLUGIN_ERROR("Plugin Interface Version is Not Known: Unable to execute the request!");
    }

//----------------------------------------------------------------------------------------
// Heap Housekeeping 

// Plugin must maintain a list of open file handles and sockets: loop over and close all files and sockets
// Plugin must maintain a list of plugin functions called: loop over and reset state and free heap.
// Plugin must maintain a list of calls to other plugins: loop over and call each plugin with the housekeeping request
// Plugin must destroy lists at end of housekeeping

    if (housekeeping || !strcasecmp(request_block->function, "reset")) {

        if (!init) return 0;        // Not previously initialised: Nothing to do!

// Free Heap & reset counters

        if (freeOldData) {
            freeDataBlock(&data_block_e);
            freeDataBlock(&data_block_e2);
            freeDataBlock(&data_block_e3);
            freeDataBlock(&data_block_t);
            freeDataBlock(&data_block_tt);        // type float - all others are type double

            old_exp_number = 0;
            freeOldData = 0;
            dataCountSW = 0;
            dataCountSS = 0;
        }

        init = 0;

        return 0;
    }

//----------------------------------------------------------------------------------------
// Initialise 

    if (!init || !strcasecmp(request_block->function, "init")
        || !strcasecmp(request_block->function, "initialise")) {

        init = 1;
        if (!strcasecmp(request_block->function, "init") || !strcasecmp(request_block->function, "initialise")) {
            return 0;
        }
    }

//----------------------------------------------------------------------------------------
// name value pairs and keywords 
//----------------------------------------------------------------------------------------

    char* signal = NULL;
    char* source = NULL;
    for (i = 0; i < request_block->nameValueList.pairCount; i++) {
        if (!strcasecmp(request_block->nameValueList.nameValue[i].name, "signal")) {
            signal = request_block->nameValueList.nameValue[i].value;
        }
        if (!strcasecmp(request_block->nameValueList.nameValue[i].name, "exp_number")) {
            source = request_block->nameValueList.nameValue[i].value;
        }
    }

//----------------------------------------------------------------------------------------
// Data Source
//----------------------------------------------------------------------------------------

    err = 0;
    do {        // Error Trap

        //----------------------------------------------------------------------------------------
        // Plugin Functions
        //----------------------------------------------------------------------------------------

        // Help: A Description of library functionality

        if (!strcasecmp(request_block->function, "help")) {

            p = (char*)malloc(sizeof(char) * 2 * 1024);

            strcpy(p, "\nanbCorrections: Functions Names and Test Descriptions\n\n");

            initDataBlock(data_block);

            data_block->rank = 1;
            data_block->dims = (DIMS*)malloc(data_block->rank * sizeof(DIMS));
            for (i = 0; i < data_block->rank; i++) initDimBlock(&data_block->dims[i]);

            data_block->data_type = UDA_TYPE_STRING;
            strcpy(data_block->data_desc, "anbCorrections help = description of this plugin");

            data_block->data = p;

            data_block->dims[0].data_type = UDA_TYPE_UNSIGNED_INT;
            data_block->dims[0].dim_n = strlen(p) + 1;
            data_block->dims[0].compressed = 1;
            data_block->dims[0].dim0 = 0.0;
            data_block->dims[0].diff = 1.0;
            data_block->dims[0].method = 0;

            data_block->data_n = data_block->dims[0].dim_n;

            strcpy(data_block->data_label, "");
            strcpy(data_block->data_units, "");

            break;
        } else

        //======================================================================================
        // SW NBI Power Correction

        if (!strcasecmp(request_block->function, "chequerBoardPower")) {

            // correction coefficients

            int coeffCount = 5;
            float chequerBoardCoeffs[3][5] = {{ -3.3909E-3f, 1.8803E-2f,  -3.7006E-4f, 3.3039E-6f,  -1.2755E-8f },
                                              { 4.2319E-1f,  -2.6726E-2f, 8.1757E-4f,  -1.0053E-5f, 4.3258E-8f },
                                              { 7.3109E-1f,  -2.1436E-2f, 3.1952E-4f,  -3.2529E-6f, 1.5714E-8f }};

            int exp_number;

            if (request_block->source[0] != '\0') {
                source = request_block->source;
            } else if (source == NULL) {
                err = 999;
                addIdamError(CODEERRORTYPE, "anbCorrections", err, "Please specify a shot number");
                break;
            }

            // Check source is a shot number

            if (!IsNumber(source)) {
                err = 999;
                addIdamError(CODEERRORTYPE, "anbCorrections", err,
                             "The data source is Not a shot number!");
                break;
            }

            exp_number = atoi(source);

            if (exp_number < 28411 || exp_number > 30473) {
                err = 999;
                idamLog(UDA_LOG_ERROR, "ERROR anbCorrections: Invalid Shot number - outside range 28411-30473\n");

                addIdamError(CODEERRORTYPE, "anbCorrections", err,
                             "Invalid Shot number - outside range 28411-30473");
                break;
            }

            // New or previous shot with static data ?

            if (exp_number == old_exp_number && !freeOldData) {    // use saved power fractions from previous request

                // Use previously prepared data - no need to recalculate
                // All typed as float data, double time

                if (!strcasecmp(signal, "ANB_SW_FULL_POWER")) {
                    copyANBDataBlock(data_block, &data_block_e, dataCountSW);    // MW
                } else if (!strcasecmp(signal, "ANB_SW_HALF_POWER")) {
                    copyANBDataBlock(data_block, &data_block_e2, dataCountSW);    // MW
                } else if (!strcasecmp(signal, "ANB_SW_THIRD_POWER")) {
                    copyANBDataBlock(data_block, &data_block_e3, dataCountSW);    // MW
                } else if (!strcasecmp(signal, "ANB_SW_SUM_POWER")) {
                    copyANBDataBlock(data_block, &data_block_t, dataCountSW);    // MW
                } else if (!strcasecmp(signal, "ANB_TOT_SUM_POWER")) {
                    copyANBDataBlock(data_block, &data_block_tt, dataCountSS);
                }    // MW

            } else {                        // calculate new power fractions

                // free old data

                if (freeOldData) {
                    freeDataBlock(&data_block_e);
                    freeDataBlock(&data_block_e2);
                    freeDataBlock(&data_block_e3);
                    freeDataBlock(&data_block_t);
                    freeDataBlock(&data_block_tt);

                    old_exp_number = 0;
                    freeOldData = 0;
                    dataCountSW = 0;
                    dataCountSS = 0;
                }

                initDataBlock(&data_block_e);
                initDataBlock(&data_block_e2);
                initDataBlock(&data_block_e3);
                initDataBlock(&data_block_t);
                initDataBlock(&data_block_tt);

                // Read SW beam current (xnb_sw_beam_current), beam voltage (xnb_sw_beam_voltage)
                // If either is missing, then assume No Data. Copy legacy pattern and return arrays of length 2. data=[0,0], time=[0,1]

                unsigned short noSWBeam = 0;
                unsigned short noSSBeam = 0;

                h1 = -1;
                h2 = -1;
                h3 = -1;

                const char* host = getenv("CORRECTIONS_UDA_HOST");
                const char* port = getenv("CORRECTIONS_UDA_PORT");

                if (host) {
                    // TODO: If this needs to happen then we need to changed how the plugin_interface struct
                    // sprintf(idam_plugin_interface->environment->server_host, "%s", host);
                }

                if (port) {
                    // TODO: If this needs to happen then we need to changed how the plugin_interface struct
                    //idam_plugin_interface->environment->server_port = atoi(port);
                }

                h1 = idamGetAPI("xnb_sw_beam_current", source);        // current (Amps assumed)

                if (h1 < 0 || getIdamErrorCode(h1) != 0) {
                    noSWBeam = 1;
                    idamFree(h1);
                    h1 = -1;
                }

                if (!noSWBeam) {
                    h2 = idamGetAPI("xnb_sw_beam_voltage", source);        // voltage (kV assumed)
                    if (h2 < 0 || getIdamErrorCode(h2) != 0) {
                        noSWBeam = 1;
                        idamFree(h1);
                        idamFree(h2);
                        h2 = -1;
                    }
                }

                h3 = idamGetAPI("ANB_SS_SUM_POWER", source);            // Total SS Beam Power (MW)

                if (h3 < 0 || getIdamErrorCode(h3) != 0) {
                    noSSBeam = 1;
                    idamFree(h3);
                    h3 = -1;
                }

                if (noSWBeam && noSSBeam) {
                    dataCountSW = 2;
                    dataCountSS = 2;
                    makeLegacyDataBlock(&data_block_e);
                    makeLegacyDataBlock(&data_block_e2);
                    makeLegacyDataBlock(&data_block_e3);
                    makeLegacyDataBlock(&data_block_t);
                    makeLegacyDataBlock(&data_block_tt);
                } else if (noSWBeam && !noSSBeam) {        // ANB_TOT_SUM_POWER needs no correction so just return

                    makeLegacyDataBlock(&data_block_e);
                    makeLegacyDataBlock(&data_block_e2);
                    makeLegacyDataBlock(&data_block_e3);
                    makeLegacyDataBlock(&data_block_t);

                    dataCountSW = 2;
                    dataCountSS = getIdamDataNum(h3);
                    data_block_tt = *getIdamDataBlock(h3);
                    initDataBlock(getIdamDataBlock(h3));        // All heap transferred

                } else {

// Check consistency: count, rank and type

                    dataCountSW = getIdamDataNum(h1);    // Always a SW beam
                    int rank = getIdamRank(h1);

                    if (getIdamDimType(h1, 0) != getIdamDimType(h2, 0)) {
                        err = 999;
                        addIdamError(CODEERRORTYPE, "anbCorrections", err,
                                     "Inconsistent SW beam current and voltage Data types");
                        break;
                    }

                    if (dataCountSW != getIdamDataNum(h2)) {
                        err = 999;
                        addIdamError(CODEERRORTYPE, "anbCorrections", err,
                                     "Inconsistent SW beam current and voltage Data Counts");
                        break;
                    }

                    noSSBeam = dataCountSW != getIdamDataNum(h3);

                    if (!noSSBeam) {
                        dataCountSS = getIdamDataNum(h3);
                    } else {
                        dataCountSS = dataCountSW;
                    }

                    if (rank != 1 || rank != getIdamRank(h2) || (!noSSBeam && rank != getIdamRank(h3))) {
                        err = 999;
                        addIdamError(CODEERRORTYPE, "anbCorrections", err,
                                     "Inconsistent SW beam current and voltage Data Rank");
                        break;
                    }

                    char* values_e = NULL;
                    char* values_e2 = NULL;
                    char* values_e3 = NULL;
                    char* values_t = NULL;
                    char* values_tt = NULL;

                    int j;

// Preserve legacy data types: all float

                    float* p_e = (float*)malloc(dataCountSW * sizeof(float));
                    float* p_e2 = (float*)malloc(dataCountSW * sizeof(float));
                    float* p_e3 = (float*)malloc(dataCountSW * sizeof(float));
                    float* p_t = (float*)malloc(dataCountSW * sizeof(float));
                    float* p_tt = (float*)malloc(dataCountSW * sizeof(float));

                    float* current = (float*)malloc(dataCountSW * sizeof(float));
                    float* voltage = (float*)malloc(dataCountSW * sizeof(float));
                    float* ssPower = (float*)malloc(dataCountSW * sizeof(float));

                    getIdamFloatData(h1, current);
                    getIdamFloatData(h2, voltage);

                    if (!noSSBeam) {
                        getIdamFloatData(h3, ssPower);
                    } else {
                        for (i = 0; i < dataCountSW; i++) ssPower[i] = 0.0;
                    }

                    float normFactor;

// Correction formulae assumes voltage is kV, current is A
// Calculated power is MW

                    for (i = 0; i < dataCountSW; i++) {
                        p_e[i] = 0.0;
                        p_e2[i] = 0.0;
                        p_e3[i] = 0.0;
                        for (j = 0; j < coeffCount; j++) {
                            float vpowj = (float)pow((double)voltage[i], (double)j);
                            p_e[i] = p_e[i] + chequerBoardCoeffs[0][j] * vpowj;
                            p_e2[i] = p_e2[i] + chequerBoardCoeffs[1][j] * vpowj;
                            p_e3[i] = p_e3[i] + chequerBoardCoeffs[2][j] * vpowj;
                        }
                    }
                    for (i = 0; i < dataCountSW; i++) {
                        normFactor = current[i] * voltage[i] / 1000.0f;
                        p_e[i] = normFactor * p_e[i];
                        p_e2[i] = normFactor * p_e2[i];
                        p_e3[i] = normFactor * p_e3[i];
                        p_t[i] = p_e[i] + p_e2[i] + p_e3[i];
                        p_tt[i] = p_t[i] + ssPower[i];
                    }

                    values_e = (char*)p_e;
                    values_e2 = (char*)p_e2;
                    values_e3 = (char*)p_e3;
                    values_t = (char*)p_t;
                    values_tt = (char*)p_tt;

// Error Data

                    float* ep_e = (float*)malloc(dataCountSW * sizeof(float));
                    float* ep_e2 = (float*)malloc(dataCountSW * sizeof(float));
                    float* ep_e3 = (float*)malloc(dataCountSW * sizeof(float));
                    float* ep_t = (float*)malloc(dataCountSW * sizeof(float));
                    float* ep_tt = (float*)malloc(dataCountSW * sizeof(float));

                    float errFactor = 0.15;
                    for (i = 0; i < dataCountSW; i++) {
                        ep_e[i] = errFactor * (float)fabs(p_e[i]);
                        ep_e2[i] = errFactor * (float)fabs(p_e2[i]);
                        ep_e3[i] = errFactor * (float)fabs(p_e3[i]);
                        ep_t[i] = errFactor * (float)fabs(p_t[i]);
                        ep_tt[i] = errFactor * (float)fabs(p_tt[i]);
                    }

                    free((void*)current);
                    free((void*)voltage);
                    free((void*)ssPower);

// Coordinate Variables	 

                    DIMS* dim_e = (DIMS*)malloc(sizeof(DIMS));
                    DIMS* dim_e2 = (DIMS*)malloc(sizeof(DIMS));
                    DIMS* dim_e3 = (DIMS*)malloc(sizeof(DIMS));
                    DIMS* dim_t = (DIMS*)malloc(sizeof(DIMS));
                    DIMS* dim_tt = (DIMS*)malloc(sizeof(DIMS));

                    initDimBlock(dim_e);
                    initDimBlock(dim_e2);
                    initDimBlock(dim_e3);
                    initDimBlock(dim_t);
                    initDimBlock(dim_tt);

                    strcpy(dim_e->dim_units, "s");
                    strcpy(dim_e->dim_label, "time");

// Preserve legacy coordinate data types: all double

                    dim_e->data_type = UDA_TYPE_DOUBLE;

                    dim_e->dim_n = dataCountSW;
                    dim_e->compressed = getIdamDimBlock(h1, 0)->compressed;
                    dim_e->dim0 = getIdamDimBlock(h1, 0)->dim0;
                    dim_e->diff = getIdamDimBlock(h1, 0)->diff;
                    dim_e->method = getIdamDimBlock(h1, 0)->method;
                    dim_e->udoms = getIdamDimBlock(h1, 0)->udoms;

                    *dim_e2 = *dim_e;
                    *dim_e3 = *dim_e;
                    *dim_t = *dim_e;
                    *dim_tt = *dim_e;

// Assume time series is identical for all signals	 

                    int* sams = getIdamDimBlock(h1, 0)->sams;
                    char* offs = getIdamDimBlock(h1, 0)->offs;
                    char* ints = getIdamDimBlock(h1, 0)->ints;

                    if (sams != NULL) {
                        dim_e->sams = (int*)malloc(dim_e->udoms * sizeof(int));
                        dim_e2->sams = (int*)malloc(dim_e->udoms * sizeof(int));
                        dim_e3->sams = (int*)malloc(dim_e->udoms * sizeof(int));
                        dim_t->sams = (int*)malloc(dim_e->udoms * sizeof(int));
                        dim_tt->sams = (int*)malloc(dim_e->udoms * sizeof(int));
                        memcpy((void*)dim_e->sams, (void*)sams, (size_t)dim_e->udoms * sizeof(long));
                        memcpy((void*)dim_e2->sams, (void*)sams, (size_t)dim_e->udoms * sizeof(long));
                        memcpy((void*)dim_e3->sams, (void*)sams, (size_t)dim_e->udoms * sizeof(long));
                        memcpy((void*)dim_t->sams, (void*)sams, (size_t)dim_e->udoms * sizeof(long));
                        memcpy((void*)dim_tt->sams, (void*)sams, (size_t)dim_e->udoms * sizeof(long));
                    }

                    size_t dataSize = dataCountSW * sizeof(double);

                    if (offs != NULL) {
                        double* e = (double*)malloc(dataSize);
                        double* e2 = (double*)malloc(dataSize);
                        double* e3 = (double*)malloc(dataSize);
                        double* t = (double*)malloc(dataSize);
                        double* tt = (double*)malloc(dataSize);
                        if (getIdamDimType(h1, 0) == UDA_TYPE_DOUBLE) {
                            memcpy((void*)e, (void*)offs, dataSize);
                            memcpy((void*)e2, (void*)offs, dataSize);
                            memcpy((void*)e3, (void*)offs, dataSize);
                            memcpy((void*)t, (void*)offs, dataSize);
                        } else {
                            int i;
                            float* foffs = (float*)offs;
                            for (i = 0; i < dataCountSW; i++) {
                                e[i] = (double)foffs[i];
                                e2[i] = (double)foffs[i];
                                e3[i] = (double)foffs[i];
                                t[i] = (double)foffs[i];
                            }
                        }
                        if (!noSSBeam && getIdamDimType(h3, 0) == UDA_TYPE_DOUBLE) {
                            memcpy((void*)tt, (void*)offs, dataSize);
                        } else {
                            int i;
                            float* foffs = (float*)offs;
                            for (i = 0; i < dataCountSW; i++) {
                                tt[i] = (double)foffs[i];
                            }
                        }
                        dim_e->offs = (char*)e;
                        dim_e2->offs = (char*)e2;
                        dim_e3->offs = (char*)e3;
                        dim_t->offs = (char*)t;
                        dim_tt->offs = (char*)tt;
                    }

                    if (ints != NULL) {
                        double* e = (double*)malloc(dataSize);
                        double* e2 = (double*)malloc(dataSize);
                        double* e3 = (double*)malloc(dataSize);
                        double* t = (double*)malloc(dataSize);
                        double* tt = (double*)malloc(dataSize);
                        if (getIdamDimType(h1, 0) == UDA_TYPE_DOUBLE) {
                            memcpy((void*)e, (void*)ints, dataSize);
                            memcpy((void*)e2, (void*)ints, dataSize);
                            memcpy((void*)e3, (void*)ints, dataSize);
                            memcpy((void*)t, (void*)ints, dataSize);
                        } else {
                            int i;
                            float* fints = (float*)ints;
                            for (i = 0; i < dataCountSW; i++) {
                                e[i] = (double)fints[i];
                                e2[i] = (double)fints[i];
                                e3[i] = (double)fints[i];
                                t[i] = (double)fints[i];
                            }
                        }
                        if (!noSSBeam && getIdamDimType(h3, 0) == UDA_TYPE_DOUBLE) {
                            memcpy((void*)tt, (void*)ints, dataSize);
                        } else {
                            int i;
                            float* fints = (float*)ints;
                            for (i = 0; i < dataCountSW; i++) {
                                tt[i] = (double)fints[i];
                            }
                        }
                        dim_e->ints = (char*)e;
                        dim_e2->ints = (char*)e2;
                        dim_e3->ints = (char*)e3;
                        dim_t->ints = (char*)t;
                        dim_tt->ints = (char*)tt;
                    }

                    if (getIdamDimData(h1, 0) != NULL) {
                        dim_e->dim = (char*)malloc(dataSize);
                        dim_e2->dim = (char*)malloc(dataSize);
                        dim_e3->dim = (char*)malloc(dataSize);
                        dim_t->dim = (char*)malloc(dataSize);
                        getIdamDoubleDimData(h1, 0, (double*)dim_e->dim);
                        getIdamDoubleDimData(h1, 0, (double*)dim_e2->dim);
                        getIdamDoubleDimData(h1, 0, (double*)dim_e3->dim);
                        getIdamDoubleDimData(h1, 0, (double*)dim_t->dim);
                    }

                    dim_tt->dim = (char*)malloc(dataSize);
                    if (!noSSBeam && getIdamDimData(h3, 0) != NULL) {
                        getIdamDoubleDimData(h3, 0, (double*)dim_tt->dim);
                    } else
                        memcpy((void*)dim_tt->dim, (void*)dim_e->dim, dataSize);

                    data_block_e.data_type = UDA_TYPE_FLOAT;
                    data_block_e.data_n = dataCountSW;
                    data_block_e.rank = getIdamRank(h1);
                    data_block_e.order = getIdamOrder(h1);

// Error Model should work but doesn't - compute and pass instead!

                    data_block_e.error_type = UDA_TYPE_FLOAT;

                    //data_block_e.error_param_n = 2;
                    //data_block_e.error_model   = ERROR_MODEL_DEFAULT;
                    //data_block_e.errparams[0]  = 0.0;	// Zero Offset systematic error
                    //data_block_e.errparams[1]  = 0.15;	// Standard deviation

                    idamFree(h1);
                    idamFree(h2);
                    idamFree(h3);
                    h1 = -1;
                    h2 = -1;
                    h3 = -1;

                    strcpy(data_block_e.data_units, "MW");

                    data_block_e2 = data_block_e;
                    data_block_e3 = data_block_e;
                    data_block_t = data_block_e;
                    data_block_tt = data_block_e;

                    data_block_e.data = values_e;
                    data_block_e.errhi = (char*)ep_e;
                    data_block_e.dims = dim_e;
                    strcpy(data_block_e.data_label, "P(E)");
                    strcpy(data_block_e.data_desc, "Power Fraction at full energy");

                    data_block_e2.data = values_e2;
                    data_block_e2.errhi = (char*)ep_e2;
                    data_block_e2.dims = dim_e2;
                    strcpy(data_block_e2.data_label, "P(E/2)");
                    strcpy(data_block_e2.data_desc, "Power Fraction at half energy");

                    data_block_e3.data = values_e3;
                    data_block_e3.errhi = (char*)ep_e3;
                    data_block_e3.dims = dim_e3;
                    strcpy(data_block_e3.data_label, "P(E/3)");
                    strcpy(data_block_e3.data_desc, "Power Fraction at third energy");

                    data_block_t.data = values_t;
                    data_block_t.errhi = (char*)ep_t;
                    data_block_t.dims = dim_t;
                    strcpy(data_block_t.data_label, "P(Total)");
                    strcpy(data_block_t.data_desc, "Total of Power Fractions");

                    data_block_tt.data = values_tt;
                    data_block_tt.errhi = (char*)ep_tt;
                    data_block_tt.dims = dim_tt;
                    strcpy(data_block_tt.data_label, "P(SS+SW)");
                    strcpy(data_block_tt.data_desc, "Total of SS and SW Powers");

                }

// Preserve state: dataCount already set

                old_exp_number = exp_number;
                freeOldData = 1;

// Return the requested data

                if (!strcasecmp(signal, "ANB_SW_FULL_POWER")) {
                    copyANBDataBlock(data_block, &data_block_e, dataCountSW);    // MW
                } else if (!strcasecmp(signal, "ANB_SW_HALF_POWER")) {
                    copyANBDataBlock(data_block, &data_block_e2, dataCountSW);    // MW
                } else if (!strcasecmp(signal, "ANB_SW_THIRD_POWER")) {
                    copyANBDataBlock(data_block, &data_block_e3, dataCountSW);    // MW
                } else if (!strcasecmp(signal, "ANB_SW_SUM_POWER")) {
                    copyANBDataBlock(data_block, &data_block_t, dataCountSW);    // MW
                } else if (!strcasecmp(signal, "ANB_TOT_SUM_POWER")) {
                    copyANBDataBlock(data_block, &data_block_tt, dataCountSS);
                }    // MW

                break;
            }
        } else {
            RAISE_PLUGIN_ERROR("Unknown function requested!");
        }

    } while (0);

//--------------------------------------------------------------------------------------
// Housekeeping

    if (err != 0) {
        if (h1 >= 0) idamFree(h1);
        if (h2 >= 0) idamFree(h2);
        if (h3 >= 0) idamFree(h3);
        freeDataBlock(&data_block_e);
        freeDataBlock(&data_block_e2);
        freeDataBlock(&data_block_e3);
        freeDataBlock(&data_block_t);
        freeDataBlock(&data_block_tt);
        old_exp_number = 0;
        freeOldData = 0;
        dataCountSW = 0;
        dataCountSS = 0;
    }

    return err;
}
