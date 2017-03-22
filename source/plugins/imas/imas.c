/*---------------------------------------------------------------
* v1 IDAM Plugin: ITER IMAS put/get 
*
* Input Arguments:	IDAM_PLUGIN_INTERFACE *idam_plugin_interface
*
* Returns:		0 if the plugin functionality was successful
*			otherwise a Error Code is returned 
*
* Standard functionality:
*
*	help	a description of what this plugin does together with a list of functions available
*
*	reset	frees all previously allocated heap, closes file handles and resets all static parameters.
*		This has the same functionality as setting the housekeeping directive in the plugin interface
*		data structure to TRUE (1)
*
*	init	Initialise the plugin: read all required data and process. Retain staticly for
*		future reference.
*
* Issues:
*
*	The imas library export some IDAM functions:
* 0000b6f3 T freeIdamClientPutDataBlockList
* 0000b728 T freeIdamDataBlock
* 0000b0d8 T initDataBlock
* 0000b25d T initDimBlock
* 0000b669 T initIdamPutDataBlock
* 0000b6d1 T initIdamPutDataBlockList	
*
*---------------------------------------------------------------------------------------------------------------*/
#include "imas.h"
#include "imas_mds.h"
#include "imas_hdf5.h"
#include "common.h"

#include <time.h>
#include <string.h>

#include <structures/genStructs.h>
#include <clientserver/errorLog.h>
#include <logging/logging.h>
#include <server/managePluginFiles.h>
#include <clientserver/udaTypes.h>
#include <clientserver/initStructs.h>
#include <server/makeServerRequestBlock.h>
#include <client/udaClient.h>
#include <server/serverPlugin.h>
#include <clientserver/compressDim.h>
#include <clientserver/freeDataBlock.h>

#define MAXOBJECTCOUNT        10000
static void* localObjs[MAXOBJECTCOUNT];
static unsigned int lastObjectId = 0;
static unsigned int initLocalObjs = 1;

static void initLocalObj()
{
// Original IMAS objects may have NULL or (void *)-1 addresses
// Two standard references are created at initialisation for these with refIds of 0 and 1.
// These are not object pointers - both are set to NULL
    int i;
    if (!initLocalObjs) {
        return;
    }
    for (i = 0; i < MAXOBJECTCOUNT; i++) {
        localObjs[i] = 0;
    }
    initLocalObjs = 0;
    localObjs[0] = NULL;    // Original IMAS objects may have NULL or (void *)-1 addresses
    localObjs[1] = NULL;
    lastObjectId = 2;
    return;
}

static int putLocalObj(void* dataObj)
{
    int i;
    if (initLocalObjs)initLocalObj();
    localObjs[lastObjectId] = dataObj;
    return lastObjectId++;
}

static void* findLocalObj(int refId)
{
    if (initLocalObjs)initLocalObj();
    void* obj = NULL;
    if (refId < lastObjectId) return localObjs[refId];
    return obj;
}

static char imasIdsVersion[256] = "";            // The IDS Data Model Version
static char imasIdsDevice[256] = "";            // The IDS Data Model Device name

void putImasIdsVersion(const char* version)
{ strcpy(imasIdsVersion, version); }

const char* getImasIdsVersion()
{ return imasIdsVersion; }

void putImasIdsDevice(const char* device)
{ strcpy(imasIdsDevice, device); }

const char* getImasIdsDevice()
{ return imasIdsDevice; }

static int imasIdsShot = 0;
static int imasIdsRun = 0;

static void putImasIdsShot(int idx, int shot)
{ imasIdsShot = shot; }

static void putImasIdsRun(int idx, int run)
{ imasIdsRun = run; }

static int getImasIdsShot(int idx)
{ return imasIdsShot; }

static int getImasIdsRun(int idx)
{ return imasIdsRun; }

/**
 * Convert name to IMAS type
 * @param typeName
 * @return
 */
int findIMASType(const char* typeName)
{
    if (typeName == NULL) return UNKNOWN_TYPE;
    if (!strcasecmp(typeName, "int")) return INT;
    if (!strcasecmp(typeName, "float")) return FLOAT;
    if (!strcasecmp(typeName, "double")) return DOUBLE;
    if (!strcasecmp(typeName, "string")) return STRING;
    return (UNKNOWN_TYPE);
}

/**
 * Convert IMAS type to IDAM type
 * @param type
 * @return
 */
int findIMASIDAMType(int type)
{
    switch (type) {
        case INT:
            return TYPE_INT;
        case FLOAT:
            return TYPE_FLOAT;
        case DOUBLE:
            return TYPE_DOUBLE;
        case STRING:
            return TYPE_STRING;
        case STRING_VECTOR:
            return TYPE_STRING;
        default:
            return TYPE_UNKNOWN;
    }
    return TYPE_UNKNOWN;
}

extern int imas(IDAM_PLUGIN_INTERFACE* idam_plugin_interface)
{
    int err = 0;

//----------------------------------------------------------------------------------------
// State Variables

    static short init = 0;
    static int idx = 0;                // Last Opened File Index value

    static int time_count_cache = 0;
    static char* time_cache = NULL;
    static char* data_cache = NULL;

    static IDAMPLUGINFILELIST* pluginFileList = NULL;

    USERDEFINEDTYPE usertype;
    COMPOUNDFIELD field;

    unsigned short housekeeping;

    if (idam_plugin_interface->interfaceVersion > THISPLUGIN_MAX_INTERFACE_VERSION) {
        IDAM_LOG(LOG_ERROR, "Plugin Interface Version Unknown to this plugin: Unable to execute the request!\n");
        THROW_ERROR(999, "Plugin Interface Version Unknown to this plugin: Unable to execute the request!");
    }

    idam_plugin_interface->pluginVersion = THISPLUGIN_VERSION;

    DATA_BLOCK* data_block = idam_plugin_interface->data_block;
    REQUEST_BLOCK* request_block = idam_plugin_interface->request_block;
    DATA_SOURCE* data_source = idam_plugin_interface->data_source;
    SIGNAL_DESC* signal_desc = idam_plugin_interface->signal_desc;

    housekeeping = idam_plugin_interface->housekeeping;

    if (housekeeping || !strcasecmp(request_block->function, "reset")) {

        if (!init) return 0;        // Not previously initialised: Nothing to do!

// Free Heap & reset counters

        closeIdamPluginFiles(pluginFileList);    // Close all open files
        initHdf5File();                // Reset the File Index array

        if (time_count_cache != 0) {
            time_count_cache = 0;
            if (time_cache != NULL)free((void*)time_cache);
            if (data_cache != NULL)free((void*)data_cache);
            time_cache = NULL;
            data_cache = NULL;
        }

        init = 0;
        idx = 0;
        putImasIdsVersion("");

        return 0;
    }

//----------------------------------------------------------------------------------------
// Initialise

    if (!init || !strcasecmp(request_block->function, "init")
        || !strcasecmp(request_block->function, "initialise")) {

        pluginFileList = getImasPluginFileList();        // From the libualidamhdf5.so library
        initIdamPluginFileList(pluginFileList);

        char* env = getenv("IMAS_IDS_VERSION");
        if (env != NULL) {
            putImasIdsVersion(env);
        } else {
            putImasIdsVersion("");
        }

        env = getenv("IMAS_IDS_DEVICE");
        if (env != NULL) {
            putImasIdsDevice(env);
        } else {
            putImasIdsDevice("");
        }

        //initHdf5File(); // Initialise the File Index array (done automatically the first time low level IMAS create or open called)

        time_count_cache = 0;
        time_cache = NULL;
        data_cache = NULL;

        init = 1;
        if (!strcasecmp(request_block->function, "init") || !strcasecmp(request_block->function, "initialise")) {
            return 0;
        }
    }


//----------------------------------------------------------------------------------------
// Forced redirection

#ifndef MDSSKIP
    if (getenv("IDAM_NOHDF5_IMAS_PLUGIN") != NULL) return imas_mds(idam_plugin_interface);
#endif

//----------------------------------------------------------------------------------------
// Common Passed name-value pairs and Keywords

    unsigned short int isCPOPath = 0, isPath = 0, isTypeName = 0, isClientIdx = 0, isRank = 0, isIndex = 0, isIndex2 = 0, isCount = 0,
            isShapeString = 0, isDataString = 0, isRelPath = 0, isHdf5Path = 0, isTimedArg = 0, isInterpolMode = 0;
    unsigned short int isFileName = 0, isShotNumber = 0, isRunNumber = 0, isRefShotNumber = 0, isRefRunNumber = 0, isCreateFromModel = 0;
    unsigned short int isPutDataSlice = 0, isReplaceLastDataSlice = 0, isGetDataSlice = 0, isGetDimension = 0;
    unsigned short int isSignal = 0, isSource = 0, isOwner = 0, isFormat = 0, isServer = 0, isImasIdsVersion = 0, isImasIdsDevice = 0;
    unsigned short int isClientObjectId = 0;
    char* CPOPath, * path, * typeName, * shapeString, * dataString, * relPath, * hdf5Path;
    char* filename, * signal, * source, * owner, * format, * server;
    int clientIdx = -1, rank = 0, index = 0, index2 = -1, interpolMode = 0, count = 0, isTimed = 0;        // isTimed is Not a FLAG!
    int shotNumber = 0, refShotNumber = 0;
    int runNumber = 0, refRunNumber = 0;
    int clientObjectId = -1;
    char* imasIdsVersion = NULL, * imasIdsDevice = NULL;

    unsigned short isPutData = 0;        // Has data been passed in a putData block?

    char quote = '"';
    char delimiter = ',';

// Arguments and keywords

    int i;
    for (i = 0; i < request_block->nameValueList.pairCount; i++) {
#ifndef MDSSKIP
        if (!strcasecmp(request_block->nameValueList.nameValue[i].name, "imas_mds")) {
            // *** redirect to the mdsplus imas plugin
            return imas_mds(idam_plugin_interface);
            continue;
        }
#endif
        if (!strcasecmp(request_block->nameValueList.nameValue[i].name, "group") ||
            !strcasecmp(request_block->nameValueList.nameValue[i].name, "cpoPath") ||
            !strcasecmp(request_block->nameValueList.nameValue[i].name, "cpo")) {
            isCPOPath = 1;
            CPOPath = request_block->nameValueList.nameValue[i].value;
            continue;
        }
        if (!strcasecmp(request_block->nameValueList.nameValue[i].name, "variable") ||
            !strcasecmp(request_block->nameValueList.nameValue[i].name, "path")) {
            isPath = 1;
            path = request_block->nameValueList.nameValue[i].value;
            continue;
        }
        if (!strcasecmp(request_block->nameValueList.nameValue[i].name, "type")) {
            isTypeName = 1;
            typeName = request_block->nameValueList.nameValue[i].value;
            continue;
        }
        if (!strcasecmp(request_block->nameValueList.nameValue[i].name, "idx")) {
            isClientIdx = 1;
            clientIdx = atoi(request_block->nameValueList.nameValue[i].value);
            continue;
        }
        if (!strcasecmp(request_block->nameValueList.nameValue[i].name, "clientObjectId") ||
            !strcasecmp(request_block->nameValueList.nameValue[i].name, "ObjectId")) {
            isClientObjectId = 1;
            clientObjectId = atoi(request_block->nameValueList.nameValue[i].value);
            continue;
        }
        if (!strcasecmp(request_block->nameValueList.nameValue[i].name, "rank") ||
            !strcasecmp(request_block->nameValueList.nameValue[i].name, "ndims")) {
            isRank = 1;
            rank = atoi(request_block->nameValueList.nameValue[i].value);
            continue;
        }
        if (!strcasecmp(request_block->nameValueList.nameValue[i].name, "index") ||
            !strcasecmp(request_block->nameValueList.nameValue[i].name, "index1")) {
            isIndex = 1;
            index = atoi(request_block->nameValueList.nameValue[i].value);
            continue;
        }
        if (!strcasecmp(request_block->nameValueList.nameValue[i].name, "index2")) {
            isIndex2 = 1;
            index2 = atoi(request_block->nameValueList.nameValue[i].value);
            continue;
        }
        if (!strcasecmp(request_block->nameValueList.nameValue[i].name, "interpolMode")) {
            isInterpolMode = 1;
            interpolMode = atoi(request_block->nameValueList.nameValue[i].value);
            continue;
        }
        if (!strcasecmp(request_block->nameValueList.nameValue[i].name, "count")) {
            isCount = 1;
            count = atoi(request_block->nameValueList.nameValue[i].value);
            continue;
        }
        if (!strcasecmp(request_block->nameValueList.nameValue[i].name, "shape") ||
            !strcasecmp(request_block->nameValueList.nameValue[i].name, "dims")) {
            isShapeString = 1;
            shapeString = request_block->nameValueList.nameValue[i].value;
            continue;
        }
        if (!strcasecmp(request_block->nameValueList.nameValue[i].name, "data")) {
            isDataString = 1;
            dataString = request_block->nameValueList.nameValue[i].value;
            continue;
        }
        if (!strcasecmp(request_block->nameValueList.nameValue[i].name, "singlequote")) {
            quote = '\'';
            continue;
        }
        if (!strcasecmp(request_block->nameValueList.nameValue[i].name, "doublequote")) {        // default value
            quote = '"';
            continue;
        }
        if (!strcasecmp(request_block->nameValueList.nameValue[i].name, "delimiter")) {        // default value is ','
            delimiter = request_block->nameValueList.nameValue[i].value[0];
            continue;
        }

        if (!strcasecmp(request_block->nameValueList.nameValue[i].name, "filename") ||
            !strcasecmp(request_block->nameValueList.nameValue[i].name, "file") ||
            !strcasecmp(request_block->nameValueList.nameValue[i].name, "name")) {
            isFileName = 1;
            filename = request_block->nameValueList.nameValue[i].value;
            continue;
        }
        if (!strcasecmp(request_block->nameValueList.nameValue[i].name, "shotNumber") ||
            !strcasecmp(request_block->nameValueList.nameValue[i].name, "shot") ||
            !strcasecmp(request_block->nameValueList.nameValue[i].name, "pulse") ||
            !strcasecmp(request_block->nameValueList.nameValue[i].name, "exp_number")) {
            isShotNumber = 1;
            shotNumber = atoi(request_block->nameValueList.nameValue[i].value);
            continue;
        }
        if (!strcasecmp(request_block->nameValueList.nameValue[i].name, "runNumber") ||
            !strcasecmp(request_block->nameValueList.nameValue[i].name, "run") ||
            !strcasecmp(request_block->nameValueList.nameValue[i].name, "pass") ||
            !strcasecmp(request_block->nameValueList.nameValue[i].name, "sequence")) {
            isRunNumber = 1;
            runNumber = atoi(request_block->nameValueList.nameValue[i].value);
            continue;
        }
        if (!strcasecmp(request_block->nameValueList.nameValue[i].name, "refShotNumber") ||
            !strcasecmp(request_block->nameValueList.nameValue[i].name, "refShot")) {
            isRefShotNumber = 1;
            refShotNumber = atoi(request_block->nameValueList.nameValue[i].value);
            continue;
        }
        if (!strcasecmp(request_block->nameValueList.nameValue[i].name, "refRunNumber") ||
            !strcasecmp(request_block->nameValueList.nameValue[i].name, "refRun")) {
            isRefRunNumber = 1;
            refRunNumber = atoi(request_block->nameValueList.nameValue[i].value);
            continue;
        }
        if (!strcasecmp(request_block->nameValueList.nameValue[i].name, "hdf5Path")) {
            isHdf5Path = 1;
            hdf5Path = request_block->nameValueList.nameValue[i].value;
            continue;
        }
        if (!strcasecmp(request_block->nameValueList.nameValue[i].name, "relPath")) {
            isRelPath = 1;
            relPath = request_block->nameValueList.nameValue[i].value;
            continue;
        }
        if (!strcasecmp(request_block->nameValueList.nameValue[i].name, "isTimed")) {
            isTimedArg = 1;
            isTimed = atoi(request_block->nameValueList.nameValue[i].value);
            continue;
        }

        if (!strcasecmp(request_block->nameValueList.nameValue[i].name, "signal")) {
            isSignal = 1;
            signal = request_block->nameValueList.nameValue[i].value;
            continue;
        }
        if (!strcasecmp(request_block->nameValueList.nameValue[i].name, "source")) {
            isSource = 1;
            source = request_block->nameValueList.nameValue[i].value;
            continue;
        }
        if (!strcasecmp(request_block->nameValueList.nameValue[i].name, "format") ||
            !strcasecmp(request_block->nameValueList.nameValue[i].name, "pattern")) {
            isFormat = 1;
            format = request_block->nameValueList.nameValue[i].value;
            continue;
        }
        if (!strcasecmp(request_block->nameValueList.nameValue[i].name, "owner")) {
            isOwner = 1;
            owner = request_block->nameValueList.nameValue[i].value;
            continue;
        }
        if (!strcasecmp(request_block->nameValueList.nameValue[i].name, "server")) {
            isServer = 1;
            server = request_block->nameValueList.nameValue[i].value;
            continue;
        }

        if (!strcasecmp(request_block->nameValueList.nameValue[i].name, "imasIdsVersion") ||
            !strcasecmp(request_block->nameValueList.nameValue[i].name, "idsVersion")) {
            isImasIdsVersion = 1;
            putImasIdsVersion(request_block->nameValueList.nameValue[i].value);
            continue;
        }
        if (!strcasecmp(request_block->nameValueList.nameValue[i].name, "imasIdsDevice") ||
            !strcasecmp(request_block->nameValueList.nameValue[i].name, "idsDevice") ||
            !strcasecmp(request_block->nameValueList.nameValue[i].name, "device")) {
            isImasIdsDevice = 1;
            putImasIdsDevice(request_block->nameValueList.nameValue[i].value);
            continue;
        }
// Keywords
        if (!strcasecmp(request_block->nameValueList.nameValue[i].name, "putSlice")) {
            isPutDataSlice = 1;
            continue;
        }
        if (!strcasecmp(request_block->nameValueList.nameValue[i].name,
                        "replaceLastSlice")) {    // Replace the last written slice (scalar, array)
            isReplaceLastDataSlice = 1;
            continue;
        }
        if (!strcasecmp(request_block->nameValueList.nameValue[i].name, "getSlice")) {
            isGetDataSlice = 1;
            continue;
        }
        if (!strcasecmp(request_block->nameValueList.nameValue[i].name, "getDimension")) {
            isGetDimension = 1;
            continue;
        }
        if (!strcasecmp(request_block->nameValueList.nameValue[i].name, "CreateFromModel")) {
            isCreateFromModel = 1;
            continue;
        }
    }

//----------------------------------------------------------------------------------------
// Data to PUT - either via name-value pairs or the putData data structure

    PUTDATA_BLOCK_LIST* putDataBlockList = &request_block->putDataBlockList;
    PUTDATA_BLOCK* putDataBlock = NULL;
    isPutData = (putDataBlockList != NULL && putDataBlockList->blockCount > 0);
    if (isPutData) putDataBlock = &(putDataBlockList->putDataBlock[0]);

//----------------------------------------------------------------------------------------
// Plugin Functions
//----------------------------------------------------------------------------------------
/*
idx	- reference to the open data file: file handle from an array of open files - hdf5Files[idx]
cpoPath	- the root group where the CPO/IDS is written
path	- the path relative to the root (cpoPath) where the data are written (must include the variable name!)
*/

    do {

//----------------------------------------------------------------------------------------
// IDS Version

        if (!strcasecmp(request_block->function, "putIdsVersion")) {
            int rc = 0;

            if (!isImasIdsVersion && !isImasIdsDevice) {
                IDAM_LOG(LOG_ERROR, "An IDS Version number or a Device name is required!\n");
                THROW_ERROR(999, "An IDS Version number or a Device name is required!");
            }

            if (isImasIdsVersion) putImasIdsVersion(imasIdsVersion);
            if (isImasIdsDevice) putImasIdsDevice(imasIdsDevice);

// Return the Status OK

            int* data = (int*)malloc(sizeof(int));
            data[0] = 1;
            initDataBlock(data_block);
            data_block->rank = 0;
            data_block->dims = NULL;
            data_block->data_type = TYPE_INT;
            data_block->data = (char*)data;
            data_block->data_n = 1;

            break;
        } else

//----------------------------------------------------------------------------------------
// Create the Data Source argument for the IDAM API
// Use Case: When there are no data_source records in the IDAM metadata catalogue, e.g. JET

// IMAS::source(signal=signal, format=[ppf|jpf|mast|mds] [,source=source] [,shotNumber=shotNumber] [,pass=pass] [,owner=owner]
//              [,datascaling=datascaling] [,timescaling=timescaling]
//              [,/data] [,/time] [,/NoCacheTime])
// keywords:	/data	return the data only
//		/time	return the time only
//		/NoCacheTime	Don't cache the time coordinate data after a request for the "data" using the /data keyword. Cacheing is the default with the cache cleared after time data are returned.
//		/NoCacheData	Don't cache the measurement data after a request for the "data" using the /data keyword. Cacheing is the default with the cache cleared after time data are returned.

        if (!strcasecmp(request_block->function, "source")) {

            static char time_units_cache[STRING_LENGTH];
            static char time_label_cache[STRING_LENGTH];
            static char signal_cache[STRING_LENGTH];
            static char source_cache[STRING_LENGTH];

            char api_signal[STRING_LENGTH];
            char api_source[STRING_LENGTH];

            unsigned short int isData = 0, isTime = 0, isNoCacheTime = 0, isNoCacheData = 0, isDataScaling = 0, isTimeScaling = 0;
            double dataScaling = 1.0, timeScaling = 1.0;
            for (i = 0; i < request_block->nameValueList.pairCount; i++) {
                if (!strcasecmp(request_block->nameValueList.nameValue[i].name,
                                "data")) {        // Only return the data - ignore the coordinates (so do nothing as already in the DATA_BLOCK)
                    isData = 1;
                    continue;
                }
                if (!strcasecmp(request_block->nameValueList.nameValue[i].name,
                                "time")) {        // Only return the time - ignore the data
                    isTime = 1;
                    continue;
                }
                if (!strcasecmp(request_block->nameValueList.nameValue[i].name,
                                "nocacheTime")) {    // Do Not Cache the time data in preparation for a Time request. Default is to Cache with cache cleared after time data returned.
                    isNoCacheTime = 1;
                    continue;
                }
                if (!strcasecmp(request_block->nameValueList.nameValue[i].name,
                                "nocacheData")) {    // Do Not Cache the time data in preparation for a Time request. Default is to Cache with cache cleared after time data returned.
                    isNoCacheData = 1;
                    continue;
                }
                if (!strcasecmp(request_block->nameValueList.nameValue[i].name,
                                "dataScaling")) {    // Measurement data need to scaled to comply with standards, e.g. SI Units
                    isDataScaling = 1;
                    dataScaling = atof(request_block->nameValueList.nameValue[i].value);
                    continue;
                }
                if (!strcasecmp(request_block->nameValueList.nameValue[i].name,
                                "timeScaling")) {    // Measurement times need to scaled to comply with standards, e.g. SI Units
                    isTimeScaling = 1;
                    timeScaling = atof(request_block->nameValueList.nameValue[i].value);
                    continue;
                }
            }

            if (!isSignal) {
                IDAM_LOG(LOG_ERROR, "No data object name (signal) has been specified!\n");
                THROW_ERROR(999, "No data object name (signal) has been specified!");
            }

// Prepare common code

            char* env = NULL;
            char work[MAXMETA];

            IDAM_PLUGIN_INTERFACE next_plugin_interface;
            REQUEST_BLOCK next_request_block;

            const PLUGINLIST* pluginList = idam_plugin_interface->pluginList;    // List of all data reader plugins (internal and external shared libraries)

            if (pluginList == NULL) {
                IDAM_LOG(LOG_ERROR, "the specified format is not recognised!\n");
                THROW_ERROR(999, "No plugins are available for this data request");
            }

            next_plugin_interface = *idam_plugin_interface;        // New plugin interface

            next_plugin_interface.request_block = &next_request_block;
            initServerRequestBlock(&next_request_block);
            strcpy(next_request_block.api_delim, request_block->api_delim);

            strcpy(next_request_block.signal, signal);            // Prepare the API arguments
            if (!isShotNumber) shotNumber = request_block->exp_number;

// JET PPF sources: PPF::/$ppfname/$pulseNumber/$sequence/$owner

            if (isFormat && !strcasecmp(format, "ppf")) {            // JET PPF source naming pattern

                if (!isSource) {
                    IDAM_LOG(LOG_ERROR, "No PPF DDA data source has been specified!\n");
                    THROW_ERROR(999, "No PPF DDA data source has been specified!");
                }

                env = getenv("IDAM_JET_DEVICE_ALIAS");

                if (env == NULL) {
                    sprintf(next_request_block.source, "JET%sPPF%s/%s/%d", request_block->api_delim,
                            request_block->api_delim, source, shotNumber);
                } else {
                    sprintf(next_request_block.source, "%s%sPPF%s/%s/%d", env, request_block->api_delim,
                            request_block->api_delim, source, shotNumber);
                }

                if (isRunNumber) {
                    sprintf(next_request_block.source, "%s/%d", next_request_block.source, runNumber);
                } else {
                    sprintf(next_request_block.source, "%s/0", next_request_block.source);
                }

                if (isOwner) {
                    sprintf(next_request_block.source, "%s/%s", next_request_block.source, owner);
                }

            } else if (isFormat && !strcasecmp(format, "jpf")) {        // JET JPF source naming pattern

                env = getenv("IDAM_JET_DEVICE_ALIAS");

                if (env == NULL) {
                    sprintf(next_request_block.source, "JET%sJPF%s%d", request_block->api_delim,
                            request_block->api_delim, shotNumber);
                } else {
                    sprintf(next_request_block.source, "%s%sJPF%s%d", env, request_block->api_delim,
                            request_block->api_delim, shotNumber);
                }
            } else if (isFormat && !strcasecmp(format, "MAST")) {        // MAST source naming pattern

                env = getenv("IDAM_MAST_DEVICE_ALIAS");

                if (!isShotNumber && !isRunNumber) {
                    strcpy(next_request_block.source,
                           request_block->source);        // Re-Use the original source argument
                } else {
                    if (env == NULL) {
                        sprintf(next_request_block.source, "MAST%s%d", request_block->api_delim, shotNumber);
                    } else {
                        sprintf(next_request_block.source, "MAST%s%d", request_block->api_delim, shotNumber);
                    }
                }
                if (isRunNumber) sprintf(next_request_block.source, "%s/%d", next_request_block.source, runNumber);

            } else if (isFormat && (!strcasecmp(format, "mds") || !strcasecmp(format, "mdsplus") ||
                                    !strcasecmp(format, "mds+"))) {    // MDS+ source naming pattern

                if (!isServer) {
                    IDAM_LOG(LOG_ERROR, "No data server has been specified!\n");
                    THROW_ERROR(999, "No data server has been specified!");
                }

                env = getenv("IDAM_MDSPLUS_ALIAS");

                if (isSource) {    // TDI function or tree?
                    if (env == NULL) {
                        sprintf(next_request_block.source, "MDSPLUS%s%s/%s/%d", request_block->api_delim, server,
                                source, shotNumber);
                    } else {
                        sprintf(next_request_block.source, "%s%s%s/%s/%d", env, request_block->api_delim, server,
                                source, shotNumber);
                    }
                } else {
                    if (env == NULL) {
                        sprintf(next_request_block.source, "MDSPLUS%s%s", request_block->api_delim, server);
                    } else {
                        sprintf(next_request_block.source, "%s%s%s", env, request_block->api_delim, server);
                    }
                    char* p = NULL;
                    if ((p = strstr(next_request_block.signal, "$pulseNumber")) != NULL) {
                        p[0] = '\0';
                        sprintf(p, "%d%s", shotNumber, &p[12]);
                    }
                }
            } else {
                IDAM_LOG(LOG_ERROR, "the specified format is not recognised!\n");
                THROW_ERROR(999, "the specified format is not recognised!");
            }

// Create the Request data structure

            env = getenv("IDAM_IDAM_PLUGIN");

            if (env != NULL) {
                sprintf(work, "%s::get(host=%s, port=%d, signal=\"%s\", source=\"%s\")", env, getIdamServerHost(),
                        getIdamServerPort(), next_request_block.signal, next_request_block.source);
            } else {
                sprintf(work, "IDAM::get(host=%s, port=%d, signal=\"%s\", source=\"%s\")", getIdamServerHost(),
                        getIdamServerPort(), next_request_block.signal, next_request_block.source);
            }

            next_request_block.source[0] = '\0';
            strcpy(next_request_block.signal, work);

            makeServerRequestBlock(&next_request_block, *pluginList);

            strcpy(api_signal,
                   next_request_block.signal);            // These are what are used to access data - retain as cache keys
            strcpy(api_source, next_request_block.source);

// Call the IDAM client via the IDAM plugin (ignore the request identified)

            if (env != NULL) {
                next_request_block.request = findPluginRequestByFormat(env, pluginList);
            } else {
                next_request_block.request = findPluginRequestByFormat("IDAM", pluginList);
            }

            if (next_request_block.request < 0) {
                IDAM_LOG(LOG_ERROR, "No IDAM server plugin found!\n");
                THROW_ERROR(999, "No IDAM server plugin found!");
            }

// is Time requested and is the data cached? Does the IDS time entity name match the cached data entity name
// Caching is the default behaviour
// If data are cached then skip the plugin request for data - use the cached data

            int skipPlugin =
                    isTime && !isNoCacheTime && time_count_cache > 0 && !strcasecmp(signal_cache, api_signal) &&
                    !strcasecmp(source_cache, api_source);

// Locate and Execute the IDAM plugin

            if (!skipPlugin) {

                int id = findPluginIdByRequest(next_request_block.request, pluginList);
                if (id >= 0 && pluginList->plugin[id].idamPlugin != NULL) {
                    err = pluginList->plugin[id].idamPlugin(&next_plugin_interface);        // Call the data reader
                    if (err != 0) {
                        addIdamError(&idamerrorstack, CODEERRORTYPE, "Data Access is not available!", err, "");
                        break;
                    }
                } else {
                    err = 999;
                    addIdamError(&idamerrorstack, CODEERRORTYPE, "Data Access is not available for this data request!",
                                 err, "");
                    break;
                }

            }

            freeNameValueList(&next_request_block.nameValueList);

// Return data is automatic since both next_request_block and request_block point to the same DATA_BLOCK etc.
// IMAS data must be DOUBLE
// Time Data are only cacheable if the data are rank 1 with time data!

            if (isData) {        // Ignore the coordinate data.

                if (data_block->order != 0 || data_block->rank != 1 ||
                    !(data_block->data_type == TYPE_FLOAT || data_block->data_type == TYPE_DOUBLE)) {
                    IDAM_LOG(LOG_ERROR, "Data Access is not available for this data request!\n");
                    THROW_ERROR(999, "Data Access is not available for this data request!");
                }        // Data are not Cacheable

                if (!isNoCacheTime) {        // Save the Time Coordinate data to local cache  (free currently cached data if different)

                    if (time_cache != NULL) free((void*)time_cache);
                    time_cache = NULL;
                    time_count_cache = 0;
                    strcpy(signal_cache, api_signal);
                    strcpy(source_cache, api_source);
                    if (data_block->dims[0].compressed) uncompressDim(&data_block->dims[0]);
                    time_count_cache = data_block->dims[0].dim_n;
                    strcpy(time_units_cache, data_block->dims[0].dim_units);
                    strcpy(time_label_cache, data_block->dims[0].dim_label);

                    if (data_block->dims[0].data_type == TYPE_DOUBLE) {
                        time_cache = (char*)malloc(time_count_cache * sizeof(double));
                        if (isTimeScaling) {
                            double* work = (double*)data_block->dims[0].dim;
                            for (i = 0; i < time_count_cache; i++) work[i] = timeScaling * work[i];
                            data_block->dims[0].dim = (char*)work;
                        }
                        memcpy(time_cache, data_block->dims[0].dim, time_count_cache * sizeof(double));
                    } else {
                        float* data = (float*)data_block->dims[0].dim;
                        double* work = (double*)malloc(time_count_cache * sizeof(double));
                        if (isTimeScaling) {
                            for (i = 0; i < time_count_cache; i++) work[i] = timeScaling * (double)data[i];
                        } else {
                            for (i = 0; i < time_count_cache; i++) work[i] = (double)data[i];
                        }
                        time_cache = (char*)work;
                    }
                } else {   // End of Time cache
                    if (time_cache != NULL) free((void*)time_cache);    // Clear the cache
                    if (data_cache != NULL) free((void*)data_cache);
                    time_cache = NULL;
                    data_cache = NULL;
                    time_count_cache = 0;
                    signal_cache[0] = '\0';
                    source_cache[0] = '\0';
                }

                if (data_block->rank == 1 &&
                    (data_block->data_type == TYPE_FLOAT || data_block->data_type == TYPE_DOUBLE)) {

                    data_block->rank = 0;        // No coordinate data to be returned
                    data_block->order = -1;

                    if (data_block->data_type == TYPE_FLOAT) {
                        float* data = (float*)data_block->data;
                        double* work = (double*)malloc(data_block->data_n * sizeof(double));
                        if (isDataScaling) {
                            for (i = 0; i < data_block->data_n; i++) work[i] = dataScaling * (double)data[i];
                        } else {
                            for (i = 0; i < data_block->data_n; i++) work[i] = (double)data[i];
                        }
                        free((void*)data_block->data);
                        data_block->data = (char*)work;
                        data_block->data_type = TYPE_DOUBLE;
                    } else {
                        if (isDataScaling) {
                            double* data = (double*)data_block->data;
                            for (i = 0; i < data_block->data_n; i++) data[i] = dataScaling * data[i];
                        }
                    }

                    if (data_block->dims[0].dim != NULL) free((void*)data_block->dims[0].dim);
                    data_block->dims[0].dim = NULL;        // prevent a double free
                    data_block->dims[0].dim_n = 0;
                } else {
                    IDAM_LOG(LOG_ERROR, "Data Access is not available for this data request!\n");
                    THROW_ERROR(999, "Data Access is not available for this data request!");
                }
            }

// For efficiency, local client cache should also be running

            if (isTime) {        // The time data are in the coordinate array indicated by 'order' value. The data must be rank 1. Data may be compressed
                if (!isNoCacheTime && time_count_cache > 0 &&
                    !strcasecmp(signal_cache, api_signal) && !strcasecmp(source_cache,
                                                                         api_source)) {        // Retrieve the Time Coordinate data from the local cache after verification of IDS names
                    data_block->rank = 0;
                    data_block->order = -1;
                    data_block->data = (char*)malloc(time_count_cache * sizeof(double));
                    memcpy(data_block->data, time_cache, time_count_cache * sizeof(double));
                    data_block->data_n = time_count_cache;
                    data_block->data_type = TYPE_DOUBLE;
                    data_block->dims = NULL;
                    strcpy(data_block->data_units, time_units_cache);
                    strcpy(data_block->data_label, time_label_cache);
                } else if (data_block->rank == 1 && data_block->order == 0 &&
                           (data_block->data_type == TYPE_FLOAT || data_block->data_type == TYPE_DOUBLE)) {
                    if (data_block->dims[0].compressed) uncompressDim(&data_block->dims[0]);
                    data_block->rank = 0;
                    data_block->order = -1;
                    if (data_block->data != NULL) free((void*)data_block->data);

                    if (data_block->dims[0].data_type == TYPE_DOUBLE) {
                        data_block->data = data_block->dims[0].dim;
                        if (isTimeScaling) {
                            double* work = (double*)data_block->data;
                            for (i = 0; i < data_block->dims[0].dim_n; i++) work[i] = timeScaling * work[i];
                        }
                    } else {
                        float* data = (float*)data_block->dims[0].dim;
                        double* work = (double*)malloc(data_block->dims[0].dim_n * sizeof(double));
                        if (isTimeScaling) {
                            for (i = 0; i < data_block->dims[0].dim_n; i++) work[i] = timeScaling * (double)data[i];
                        } else {
                            for (i = 0; i < data_block->dims[0].dim_n; i++) work[i] = (double)data[i];
                        }
                        data_block->data = (char*)work;
                        free((void*)data_block->dims[0].dim);
                    }
                    data_block->data_n = data_block->dims[0].dim_n;
                    data_block->data_type = TYPE_DOUBLE;
                    data_block->dims[0].dim = NULL;        // prevent a double free
                    data_block->dims[0].dim_n = 0;
                    strcpy(data_block->data_units, data_block->dims[0].dim_units);
                    strcpy(data_block->data_label, data_block->dims[0].dim_label);
                } else {
                    IDAM_LOG(LOG_ERROR, "Data Access is not available for this data request!\n");
                    THROW_ERROR(999, "Data Access is not available for this data request!");
                }
            }

            break;

        } else

//----------------------------------------------------------------------------------------
// DELETE Data from an IDS file

        if (!strcasecmp(request_block->function, "delete")) {
            int rc = 0, fid = -1, fileStatus = 0;
            int isOpen = 0;
            char* file = NULL;

// What is the status of the data file

            if (isClientIdx) {
                fid = checkHdf5Idx(clientIdx);
                if (fid >= 0) {
                    if (pluginFileList->files[fid].status) {
                        fileStatus = FILEISOPEN;
                        isOpen = 1;
                    } else {
                        fileStatus = FILEISCLOSED;
                    }
                } else {
                    fileStatus = FILEISNEW;
                    if (!isFileName && !isShotNumber && !isRunNumber) {
                        IDAM_LOG(LOG_ERROR, "No registered data file identified!\n");
                        THROW_ERROR(999, "No registered data file identified!");
                    }
                    isClientIdx = 0;        // Data is not recognised - ignore!
                }
            } else if (isFileName && isShotNumber && isRunNumber) {
                file = getHdf5FileName(filename, shotNumber, runNumber);        // Standard Name design pattern
                fid = findIdamPluginFileByName(pluginFileList, file);        // Check the IDAM log
                if (fid >= 0) {
                    if (pluginFileList->files[fid].status) {
                        fileStatus = FILEISOPEN;
                        if ((clientIdx = findHdf5Idx(fid)) < 0) {
                            IDAM_LOG(LOG_ERROR, "imas delete: Unable to Locate the necessary data file!\n");
                            THROW_ERROR(999, "Unable to Locate the necessary data file!");
                            break;
                        }
                        isClientIdx = 1;                        // Treat as if passed in a name-value pair
                        isOpen = 1;                            // File is Open
                    } else {
                        fileStatus = FILEISCLOSED;
                    }
                } else {
                    fileStatus = FILEISNEW;
                }
            } else {
                IDAM_LOG(LOG_ERROR, "imas delete: No data file identified!\n");
                THROW_ERROR(999, "No data file identified!");
            }

// Open the Data file

            if (!isOpen) {
                IDAM_PLUGIN_INTERFACE private_idam_plugin_interface;
                DATA_BLOCK private_data_block;
                REQUEST_BLOCK private_request_block;
                initDataBlock(&private_data_block);
                initRequestBlock(&private_request_block);

                private_idam_plugin_interface = *idam_plugin_interface;

                private_idam_plugin_interface.data_block = &private_data_block;
                private_idam_plugin_interface.request_block = &private_request_block;

                strcpy(private_request_block.source,
                       request_block->source);            // Use the original Source argument

                if (isClientIdx) {
                    sprintf(private_request_block.signal, "imas::open(idx=%d)",
                            clientIdx);        // Open the file
                } else {
                    sprintf(private_request_block.signal, "imas::open(file=%s, shot=%d, run=%d)", filename, shotNumber,
                            runNumber);
                }

                makeServerRequestBlock(&private_request_block, *idam_plugin_interface->pluginList);

                err = imas(
                        &private_idam_plugin_interface);                        // Recursive call to open/create the file

                if (private_data_block.data == NULL) {
                    clientIdx = -1;
                } else {
                    clientIdx = *(int*)private_data_block.data;
                }

                putImasIdsShot(clientIdx, shotNumber);
                putImasIdsRun(clientIdx, runNumber);

                freeNameValueList(&private_request_block.nameValueList);
                freeDataBlock(&private_data_block);

                if (clientIdx < 0) {
                    IDAM_LOG(LOG_ERROR, "imas delete: Unable to Open the necessary data file!\n");
                    THROW_ERROR(999, "Unable to Open the necessary data file!");
                }
                isClientIdx = 1;                                // Treat as if passed in a name-value pair
                isOpen = 1;                                    // File is Open
            }

// ***TO check passed
            rc = imas_hdf5DeleteData(clientIdx, CPOPath, path);

            if (rc < 0) {
                THROW_ERROR(999, "Data DELETE method failed!");
            }

// Return the data

            int* data = (int*)malloc(sizeof(int));
            data[0] = 1;

            initDataBlock(data_block);
            data_block->rank = 0;
            data_block->data_type = TYPE_INT;
            data_block->data = (char*)data;
            data_block->data_n = 1;

            break;
        } else

//----------------------------------------------------------------------------------------
// IMAS get some data - assumes the file exists

        if (!strcasecmp(request_block->function, "get")) {
            int i, rc = 0, fid = -1, fileStatus = 0;
            int* shape = NULL;
            int isOpen = 0, isType = 0;
            int type, idamType;
            char* file = NULL;
            void* data = NULL;

/*
idx	- reference to the open data file: file handle from an array of open files - hdf5Files[idx]
cpoPath	- the root group where the CPO/IDS is written
path	- the path relative to the root (cpoPath) where the data are written (must include the variable name!)
*/

// What is the status of the data file

            if (isClientIdx) {
                fid = checkHdf5Idx(clientIdx);
                if (fid >= 0) {
                    if (pluginFileList->files[fid].status) {
                        fileStatus = FILEISOPEN;
                        isOpen = 1;
                    } else {
                        fileStatus = FILEISCLOSED;
                    }
                } else {
                    fileStatus = FILEISNEW;
                    if (!isFileName && !isShotNumber && !isRunNumber) {
                        err = 999;
                        IDAM_LOG(LOG_ERROR, "imas put: No registered data file identified!\n");
                        addIdamError(&idamerrorstack, CODEERRORTYPE, "imas get", err,
                                     "No registered data file identified!");
                        break;
                    }
                    isClientIdx = 0;        // Data is not recognised - ignore!
                }
            } else if (isFileName && isShotNumber && isRunNumber) {
                file = getHdf5FileName(filename, shotNumber, runNumber);    // Standard Name design pattern
                fid = findIdamPluginFileByName(pluginFileList, file);        // Check the IDAM log
                if (fid >= 0) {
                    if (pluginFileList->files[fid].status) {
                        fileStatus = FILEISOPEN;
                        if ((clientIdx = findHdf5Idx(fid)) < 0) {
                            err = 999;
                            IDAM_LOG(LOG_ERROR, "imas put: Unable to Locate the necessary data file!\n");
                            addIdamError(&idamerrorstack, CODEERRORTYPE, "imas get", err,
                                         "Unable to Locate the necessary data file!");
                            break;
                        }
                        isClientIdx = 1;                        // Treat as if passed in a name-value pair
                        isOpen = 1;                            // File is Open
                    } else {
                        fileStatus = FILEISCLOSED;
                    }
                } else {
                    fileStatus = FILEISNEW;
                }
            } else {
                err = 999;
                IDAM_LOG(LOG_ERROR, "imas put: No data file identified!\n");
                addIdamError(&idamerrorstack, CODEERRORTYPE, "imas get", err, "No data file identified!");
                break;
            }

// Open the Data file

            if (!isOpen) {
                IDAM_PLUGIN_INTERFACE private_idam_plugin_interface;
                DATA_BLOCK private_data_block;
                REQUEST_BLOCK private_request_block;
                initDataBlock(&private_data_block);
                initRequestBlock(&private_request_block);

                private_idam_plugin_interface = *idam_plugin_interface;

                private_idam_plugin_interface.data_block = &private_data_block;
                private_idam_plugin_interface.request_block = &private_request_block;

                strcpy(private_request_block.source,
                       request_block->source);            // Use the original Source argument

                if (isClientIdx) {
                    sprintf(private_request_block.signal, "imas::open(idx=%d)",
                            clientIdx);                        // Open the file
                } else {
                    sprintf(private_request_block.signal, "imas::open(file=%s, shot=%d, run=%d)", filename, shotNumber,
                            runNumber);
                }

                makeServerRequestBlock(&private_request_block, *idam_plugin_interface->pluginList);

                err = imas(
                        &private_idam_plugin_interface);                        // Recursive call to open/create the file

                if (private_data_block.data == NULL) {
                    clientIdx = -1;
                } else {
                    clientIdx = *(int*)private_data_block.data;
                }

                putImasIdsShot(clientIdx, shotNumber);
                putImasIdsRun(clientIdx, runNumber);

                freeNameValueList(&private_request_block.nameValueList);
                freeDataBlock(&private_data_block);

                if (clientIdx < 0) {
                    IDAM_LOG(LOG_ERROR, "imas put: Unable to Open the necessary data file!\n");
                    THROW_ERROR(999, "Unable to Open the necessary data file!");
                }
                isClientIdx = 1;                                // Treat as if passed in a name-value pair
                isOpen = 1;                                    // File is Open
            }

// Convert type string into IMAS type identifiers

            if (!isGetDimension) {
                if (!isTypeName && !isPutData) {
                    IDAM_LOG(LOG_ERROR, "imas get: The data's Type has not been specified!\n");
                    THROW_ERROR(999, "The data's Type has not been specified!");
                }

                if ((type = findIMASType(typeName)) == 0) {
                    IDAM_LOG(LOG_ERROR, "imas get: The data's Type name cannot be converted!\n");
                    THROW_ERROR(999, "The data's Type name cannot be converted!");
                }

                isType = 1;
            }

            if (!isPutData && !isRank && !isGetDimension) {
                IDAM_LOG(LOG_ERROR, "imas get: The data's Rank has not been specified!\n");
                THROW_ERROR(999, "The data's Rank has not been specified!");
            }

            if (isGetDimension) rank = 7;

            shape = (int*)malloc((rank + 1) * sizeof(int));
            shape[0] = 1;
            for (i = 1; i < rank; i++) shape[i] = 0;

// Which Data Operation?

            int dataOperation = GET_OPERATION;
            if (isGetDataSlice) { dataOperation = GETSLICE_OPERATION; }
            else if (isGetDimension) dataOperation = GETDIMENSION_OPERATION;

// GET and return the data

            char* imasData = NULL;
            double retTime = 0.0;
            initDataBlock(data_block);

            if (dataOperation == GET_OPERATION) {
                rc = imas_getData(clientIdx, CPOPath, path, type, rank, shape, &imasData);
            } else if (dataOperation == GETSLICE_OPERATION) {

                if (!isIndex) {
                    IDAM_LOG(LOG_ERROR, "imas get: No Time Vector Indices have been specified!\n");
                    THROW_ERROR(999, "No Time Vector Indices have been specified!");
                }
                if (!isInterpolMode) {
                    IDAM_LOG(LOG_ERROR, "imas get: No Interpolation Mode has been specified!\n");
                    THROW_ERROR(999, "No Interpolation Mode has been specified!");
                }

                if (!isPutData || putDataBlock->data_type != TYPE_DOUBLE || putDataBlock->count != 3) {
                    IDAM_LOG(LOG_ERROR, "imas get: No Time Values have been specified!\n");
                    THROW_ERROR(999, "No Time Values have been specified!");
                }

                char* currData;
                int nItems = 0;
                double time = ((double*)putDataBlock->data)[0];
                double sliceTime1 = ((double*)putDataBlock->data)[1];
                double sliceTime2 = ((double*)putDataBlock->data)[2];

// from PUTDATA: time, sliceTime1, sliceTime2
// from name-value: sliceIdx1 (aka index), sliceIdx2 (aka index2)

                if (!isIndex2 || index2 == -1) {     //Only a single sample
                    rc = imas_getDataSlices(clientIdx, CPOPath, path, type, rank, shape, index, 1, &currData);
                    retTime = sliceTime1;
                } else {
                    rc = imas_getDataSlices(clientIdx, CPOPath, path, type, rank, shape, index, 2, &currData);

                    if (!rc) {    // No Error

                        double dt = 1.0;
                        unsigned int leftSlicePoint = 0;
                        switch (interpolMode) {
                            case INTERPOLATION:
                                dt = (time - sliceTime1) / (sliceTime2 - sliceTime1);
                                retTime = time;
                                break;
                            case CLOSEST_SAMPLE:
                                leftSlicePoint = time - sliceTime1 < sliceTime2 - time;
                                if (leftSlicePoint) {
                                    retTime = sliceTime1;
                                } else {
                                    retTime = sliceTime2;
                                }
                                break;
                            case PREVIOUS_SAMPLE:
                                retTime = sliceTime1;
                                break;
                        }
                        if (rank == 0) {
                            switch (type) {
                                case (INT): {
                                    int y1 = ((int*)currData)[0];
                                    int y2 = ((int*)currData)[1];
                                    int* retData = (int*)malloc(sizeof(int));
                                    switch (interpolMode) {
                                        case INTERPOLATION:
                                            retData[0] = y1 + (y2 - y1) * dt;
                                            break;
                                        case CLOSEST_SAMPLE:
                                            if (leftSlicePoint) {
                                                retData[0] = y1;
                                            } else {
                                                retData[0] = y2;
                                            }
                                            break;
                                        case PREVIOUS_SAMPLE:
                                            retData[0] = y1;
                                            break;
                                    }
                                    imasData = (char*)retData;
                                    break;
                                }
                                case (FLOAT): {
                                    float y1 = ((float*)currData)[0];
                                    float y2 = ((float*)currData)[1];
                                    float* retData = (float*)malloc(sizeof(float));
                                    switch (interpolMode) {
                                        case INTERPOLATION:
                                            retData[0] = y1 + (y2 - y1) * dt;
                                            break;
                                        case CLOSEST_SAMPLE:
                                            if (leftSlicePoint) {
                                                retData[0] = y1;
                                            } else {
                                                retData[0] = y2;
                                            }
                                            break;
                                        case PREVIOUS_SAMPLE:
                                            retData[0] = y1;
                                            break;
                                    }
                                    imasData = (char*)retData;
                                    break;
                                }
                                case (DOUBLE): {
                                    double y1 = ((double*)currData)[0];
                                    double y2 = ((double*)currData)[1];
                                    double* retData = (double*)malloc(sizeof(double));
                                    switch (interpolMode) {
                                        case INTERPOLATION:
                                            retData[0] = y1 + (y2 - y1) * dt;
                                            break;
                                        case CLOSEST_SAMPLE:
                                            if (leftSlicePoint) {
                                                retData[0] = y1;
                                            } else {
                                                retData[0] = y2;
                                            }
                                            break;
                                        case PREVIOUS_SAMPLE:
                                            retData[0] = y1;
                                            break;
                                    }
                                    imasData = (char*)retData;
                                    break;
                                }
                            }
                        } else if (rank >= 1) {
                            nItems = shape[0];
                            for (i = 1; i < rank; i++)nItems *= shape[i];
                            switch (type) {
                                case (INT): {
                                    int* y1 = (int*)currData;
                                    int* y2 = (int*)&currData[1];
                                    int* retData = (int*)malloc(sizeof(int) * nItems);
                                    for (i = 0; i < nItems; i++) {
                                        switch (interpolMode) {
                                            case INTERPOLATION:
                                                retData[i] = y1[2 * i] + (y2[2 * i] - y1[2 * i]) * dt;
                                                break;
                                            case CLOSEST_SAMPLE:
                                                if (leftSlicePoint) {
                                                    retData[i] = y1[2 * i];
                                                } else {
                                                    retData[i] = y2[2 * i];
                                                }
                                                break;
                                            case PREVIOUS_SAMPLE:
                                                retData[i] = y1[2 * i];
                                                break;
                                        }
                                    }
                                    imasData = (char*)retData;
                                    break;
                                }
                                case (FLOAT): {
                                    float* y1 = (float*)currData;
                                    float* y2 = (float*)&currData[1];
                                    float* retData = (float*)malloc(sizeof(float) * nItems);
                                    for (i = 0; i < nItems; i++) {
                                        switch (interpolMode) {
                                            case INTERPOLATION:
                                                retData[i] = y1[2 * i] + (y2[2 * i] - y1[2 * i]) * dt;
                                                break;
                                            case CLOSEST_SAMPLE:
                                                if (leftSlicePoint) {
                                                    retData[i] = y1[2 * i];
                                                } else {
                                                    retData[i] = y2[2 * i];
                                                }
                                                break;
                                            case PREVIOUS_SAMPLE:
                                                retData[i] = y1[2 * i];
                                                break;
                                        }
                                    }
                                    imasData = (char*)retData;
                                    break;
                                }
                                case (DOUBLE): {
                                    double* y1 = (double*)currData;
                                    double* y2 = (double*)&currData[1];
                                    double* retData = (double*)malloc(sizeof(double) * nItems);
                                    for (i = 0; i < nItems; i++) {
                                        switch (interpolMode) {
                                            case INTERPOLATION:
                                                retData[i] = y1[2 * i] + (y2[2 * i] - y1[2 * i]) * dt;
                                                break;
                                            case CLOSEST_SAMPLE:
                                                if (leftSlicePoint) {
                                                    retData[i] = y1[2 * i];
                                                } else {
                                                    retData[i] = y2[2 * i];
                                                }
                                                break;
                                            case PREVIOUS_SAMPLE:
                                                retData[i] = y1[2 * i];
                                                break;
                                        }
                                    }
                                    imasData = (char*)retData;
                                    break;
                                }
                            }

                        }    // rank >= 1
                    }    // No Error
                }    // 2 indices passed

            } else if (dataOperation == GETDIMENSION_OPERATION) {
                rc = imas_hdf5GetDimension(clientIdx, CPOPath, path, &rank, &shape[0], &shape[1], &shape[2], &shape[3],
                                           &shape[4], &shape[5], &shape[6]);
            }

            if (rc < 0 || (!isGetDimension && imasData == NULL)) {
                free(shape);
                THROW_ERROR(999, "Data GET method failed!");
            }

// Return Data

            switch (dataOperation) {
                case (GETDIMENSION_OPERATION): {
                    data_block->rank = 1;
                    data_block->data_type = TYPE_INT;
                    data_block->data = (char*)shape;
                    data_block->data_n = rank;
                    break;
                }
                case (GET_OPERATION): {
                    data_block->rank = rank;
                    data_block->data_type = findIMASIDAMType(type);
                    data_block->data = (char*)imasData;
                    if (data_block->data_type == TYPE_STRING && rank <= 1) {
                        data_block->data_n = strlen((char*)imasData) + 1;
                        shape[0] = data_block->data_n;
                    } else {
                        data_block->data_n = shape[0];
                        for (i = 1; i < rank; i++)data_block->data_n *= shape[i];
                    }
                    break;
                }
                case (GETSLICE_OPERATION): {        // Need to return Data as well as the time - increase rank by 1 and pass in a
                    // coordinate array of length 1
                    data_block->rank = rank;
                    data_block->data_type = findIMASIDAMType(type);
                    data_block->data = (char*)imasData;
                    data_block->data_n = shape[0];
                    for (i = 1; i < rank; i++)data_block->data_n *= shape[i];
                    break;
                }
            }

// Return dimensions

            if (data_block->rank > 0 || dataOperation == GETSLICE_OPERATION) {
                data_block->dims = (DIMS*)malloc((data_block->rank + 1) * sizeof(DIMS));
                for (i = 0; i < data_block->rank; i++) {
                    initDimBlock(&data_block->dims[i]);
                    data_block->dims[i].dim_n = shape[i];
                    data_block->dims[i].data_type = TYPE_UNSIGNED_INT;
                    data_block->dims[i].compressed = 1;
                    data_block->dims[i].dim0 = 0.0;
                    data_block->dims[i].diff = 1.0;
                    data_block->dims[i].method = 0;
                }
                if (dataOperation == GETSLICE_OPERATION) {
                    data_block->rank = rank + 1;
                    data_block->order = rank;
                    initDimBlock(&data_block->dims[rank]);
                    data_block->dims[rank].dim_n = 1;
                    data_block->dims[rank].data_type = TYPE_DOUBLE;
                    data_block->dims[rank].compressed = 0;
                    double* sliceTime = (double*)malloc(sizeof(double));
                    *sliceTime = retTime;
                    data_block->dims[rank].dim = (char*)sliceTime;
                }
            }

            if (!isGetDimension && shape != NULL) free(shape);

            break;
        } else

//----------------------------------------------------------------------------------------
/* File State Scenarios and Actions

Notes

	Data files are never deleted by the plugin, only overwritten
	The file template (or model) approach should be deprecated - it is too rigid a design.
	The IDAM approach is to create groups where they are missing, including intermediate groups.
	There is no documentation written with the data! Its missing from the IMAS design.

	Files are identified either by an IMAS reference ID or by name, shot and run identifiers.
	Both can be used to identify IMAS and IDAM logged files.

1> New file? A new file has no previous entry in the IMAS or IDAM file log

	exists in the data archive		Open the file
	doesn't exist in the data archive	create the file

2> Existing File? An entry exists in the IMAS and IDAM file logs

	is Open					Reuse file id
	is Closed				Open the file

*/
//----------------------------------------------------------------------------------------
// IMAS put some data - assumes the file exists

        if (!strcasecmp(request_block->function, "put")) {
            int rc = 0, fid = -1, fileStatus = 0;
            int dataRank;
            int* shape = NULL;
            int isOpen = 0, isShape = 0, isData = 0, isType = 0;
            int shapeCount = 0, dataCount = 0;
            int type, idamType;
            char* file = NULL;
            void* data = NULL;

            int isVarData = 0;
            void* varData = NULL;
            short varDataIndex = -1;

            int isTime = 0;
            //double time = 0.0;
            int isPutDataSliceTime = 0;
            double putDataSliceTime = 0.0;

/*
idx	- reference to the open data file: file handle from an array of open files - hdf5Files[idx]
cpoPath	- the root group where the CPO/IDS is written
path	- the path relative to the root (cpoPath) where the data are written (must include the variable name!)
type	- the atomic type of the data (int). The IMAS type enumeration.
rank	- is called nDims in the IMAS code base
shape	- is called dims in the IMAS code base
isTimed	- ?
data	- the data to be written - from a PUTDATA block
time	- the time slice to be written - from a PUTDATA block (putSlice keyword)
*/

// What is the status of the data file

            if (isClientIdx) {
                fid = checkHdf5Idx(clientIdx);
                if (fid >= 0) {
                    if (pluginFileList->files[fid].status) {
                        fileStatus = FILEISOPEN;
                        isOpen = 1;
                    } else {
                        fileStatus = FILEISCLOSED;
                    }
                } else {
                    fileStatus = FILEISNEW;
                    if (!isFileName && !isShotNumber && !isRunNumber) {
                        IDAM_LOG(LOG_ERROR, "imas put: No registered data file identified!\n");
                        THROW_ERROR(999, "No registered data file identified!");
                    }
                    isClientIdx = 0;        // Data is not recognised - ignore!
                }
            } else if (isFileName && isShotNumber && isRunNumber) {
                file = getHdf5FileName(filename, shotNumber, runNumber);        // Standard Name design pattern
                fid = findIdamPluginFileByName(pluginFileList, file);        // Check the IDAM log
                if (fid >= 0) {
                    if (pluginFileList->files[fid].status) {
                        fileStatus = FILEISOPEN;
                        if ((clientIdx = findHdf5Idx(fid)) < 0) {
                            err = 999;
                            IDAM_LOG(LOG_ERROR, "imas put: Unable to Locate the necessary data file!\n");
                            addIdamError(&idamerrorstack, CODEERRORTYPE, "imas put", err,
                                         "Unable to Locate the necessary data file!");
                            break;
                        }
                        isClientIdx = 1;                        // Treat as if passed in a name-value pair
                        isOpen = 1;                            // File is Open
                    } else {
                        fileStatus = FILEISCLOSED;
                    }
                } else {
                    fileStatus = FILEISNEW;
                }
            } else {
                IDAM_LOG(LOG_ERROR, "imas put: No data file identified!\n");
                THROW_ERROR(999, "No data file identified!");
            }

// Open or Create the Data file

            if (!isOpen) {
                IDAM_PLUGIN_INTERFACE private_idam_plugin_interface;
                DATA_BLOCK private_data_block;
                REQUEST_BLOCK private_request_block;
                initDataBlock(&private_data_block);
                initRequestBlock(&private_request_block);

                private_idam_plugin_interface = *idam_plugin_interface;

                private_idam_plugin_interface.data_block = &private_data_block;
                private_idam_plugin_interface.request_block = &private_request_block;

                strcpy(private_request_block.source,
                       request_block->source);            // Use the original Source argument

                if (fileStatus == FILEISNEW) {
                    sprintf(private_request_block.signal, "imas::create(file=%s, shot=%d, run=%d)", filename,
                            shotNumber, runNumber);        // Create the file
                } else {
                    if (isClientIdx) {
                        sprintf(private_request_block.signal, "imas::open(idx=%d)",
                                clientIdx);                        // Open the file
                    } else {
                        sprintf(private_request_block.signal, "imas::open(file=%s, shot=%d, run=%d)", filename,
                                shotNumber, runNumber);
                    }    // Open the file
                }

                makeServerRequestBlock(&private_request_block, *idam_plugin_interface->pluginList);

                err = imas(
                        &private_idam_plugin_interface);                        // Recursive call to open/create the file

                if (private_data_block.data == NULL) {
                    clientIdx = -1;
                } else {
                    clientIdx = *(int*)private_data_block.data;
                }

                putImasIdsShot(clientIdx, shotNumber);
                putImasIdsRun(clientIdx, runNumber);

                freeNameValueList(&private_request_block.nameValueList);
                freeDataBlock(&private_data_block);

                if (clientIdx < 0) {
                    IDAM_LOG(LOG_ERROR, "imas put: Unable to Open/Create the necessary data file!\n");
                    THROW_ERROR(999, "Unable to Open/Create the necessary data file!");
                }
                isClientIdx = 1;                                // Treat as if passed in a name-value pair
                isOpen = 1;                                    // File is Open
            }

// Has a PUTDATA block been passed with a missing or a matching name?

            if (isPutData) {
                for (i = 0; i < putDataBlockList->blockCount; i++) {

                    if (putDataBlockList->putDataBlock[i].blockName == NULL) {
                        if (putDataBlockList->blockCount > 1) {            // Multiple blocks must be named
                            IDAM_LOG(LOG_ERROR, "imas put: Multiple un-named data items - ambiguous!\n");
                            THROW_ERROR(999, "Multiple un-named data items - ambiguous!");
                        }
                        varData = (char*)putDataBlockList->putDataBlock[i].data;    // Only 1 unnamed block
                        varDataIndex = i;
                        break;
                    } else if (!strcasecmp(putDataBlockList->putDataBlock[i].blockName, "variable") ||
                               !strcasecmp(putDataBlockList->putDataBlock[i].blockName, "data") ||
                               (isPath && !strcasecmp(putDataBlockList->putDataBlock[i].blockName, path))) {
                        varData = (char*)putDataBlockList->putDataBlock[i].data;
                        varDataIndex = i;
                        break;
                    }
                }
                for (i = 0; i < putDataBlockList->blockCount; i++) {
                    if (putDataBlockList->putDataBlock[i].blockName != NULL && (
                            !strcasecmp(putDataBlockList->putDataBlock[i].blockName, "putDataTime") ||
                            !strcasecmp(putDataBlockList->putDataBlock[i].blockName, "time"))) {
                        putDataSliceTime = ((double*)putDataBlockList->putDataBlock[i].data)[0];
                        isTime = 1;
                        isPutDataSliceTime = 1;
                        break;
                    }
                }

                if (varDataIndex < 0 && putDataBlockList->blockCount == 1 &&
                    (putDataBlockList->putDataBlock[0].blockName == NULL ||
                     putDataBlockList->putDataBlock[0].blockName[0] == '\0')) {
                    varData = (char*)putDataBlockList->putDataBlock[0].data;
                    varDataIndex = 0;
                }
                if (varDataIndex < 0) {
                    err = 999;
                    IDAM_LOG(LOG_ERROR, "imas put: Unable to Identify the data to PUT!\n");
                    addIdamError(&idamerrorstack, CODEERRORTYPE, "imas put", err,
                                 "Unable to Identify the data to PUT!");
                    break;
                }

                isVarData = 1;
                putDataBlock = &putDataBlockList->putDataBlock[varDataIndex];

                if ((type = findIMASType(convertIdam2StringType(putDataBlock->data_type))) == 0) {
                    // Convert an IDAM type to an IMAS type
                    IDAM_LOG(LOG_ERROR, "imas put: The data's Type cannot be converted!\n");
                    THROW_ERROR(999, "The data's Type cannot be converted!");
                }

            } else {    // if isPutData

// Convert type string into IMAS type identifiers

                if (!isTypeName) {
                    IDAM_LOG(LOG_ERROR, "imas put: The data's Type has not been specified!\n");
                    THROW_ERROR(999, "The data's Type has not been specified!");
                }

                if ((type = findIMASType(typeName)) == 0) {
                    IDAM_LOG(LOG_ERROR, "imas put: The data's Type name cannot be converted!\n");
                    THROW_ERROR(999, "The data's Type name cannot be converted!");
                }
            }

            isType = 1;

// Any Data?

            if (!isDataString && !isVarData) {
                IDAM_LOG(LOG_ERROR, "imas put: No data has been specified!\n");
                THROW_ERROR(999, "No data has been specified!");
            }

// Which Data Operation?

            int dataOperation = PUT_OPERATION;
            if (isPutDataSlice) { dataOperation = PUTSLICE_OPERATION; }
            else if (isReplaceLastDataSlice) dataOperation = REPLACELASTSLICE_OPERATION;

// Convert Name-Value string arrays (shape, data) to numerical arrays of the correct type

// *** use same naming convention as netcdf4 put
// *** re-use netcdf4 put functions: getCDF4VarArray generalised to getIdamNameValuePairVarArray
// *** need new functions: convert from named type to HDF5 type

            if (!isPutData) {
                if (!isRank && !isShapeString) {        // Assume a scalar value
                    isRank = 1;
                    isShape = 1;
                    rank = 0;
                    shape = (int*)malloc(sizeof(int));
                    shape[0] = 1;
                    shapeCount = 1;
                }

                if (isShapeString) {
                    if ((dataRank = getIdamNameValuePairVarArray(shapeString, quote, delimiter, (unsigned short)rank,
                                                                 TYPE_INT, (void**)&shape)) < 0) {
                        IDAM_LOG(LOG_ERROR, "imas put: Unable to convert the passed shape values!\n");
                        THROW_ERROR(-dataRank, "Unable to convert the passed shape value!");
                    }
                    if (isRank && rank != dataRank) {
                        IDAM_LOG(LOG_ERROR, "imas put: The passed rank is inconsistent with the passed shape data!\n");
                        THROW_ERROR(999, "The passed rank is inconsistent with the passed shape data!");
                    }
                    isShape = 1;
                    isRank = 1;
                    rank = dataRank;

                    shapeCount = shape[0];
                    for (i = 1; i < rank; i++) shapeCount = shapeCount * shape[i];
                }    // isShapeString (from name-value pair)

                if (isDataString) {

                    if ((idamType = findIdamType(typeName)) == TYPE_UNDEFINED) {
                        IDAM_LOG(LOG_ERROR, "imas put: The data's Type name cannot be converted!\n");
                        THROW_ERROR(999, "The data's Type name cannot be converted!");
                    }

                    if ((dataCount = getIdamNameValuePairVarArray(dataString, quote, delimiter,
                                                                  (unsigned short)shapeCount, idamType,
                                                                  (void**)&data)) < 0) {
                        err = -dataCount;
                        IDAM_LOG(LOG_ERROR, "imas put: Unable to convert the passed data values!\n");
                        THROW_ERROR(999, "Unable to convert the passed data value!");
                        break;
                    }
                    if (shapeCount != 0 && shapeCount != dataCount) {
                        IDAM_LOG(LOG_ERROR, "imas put: Inconsistent count of Data items!\n");
                        THROW_ERROR(999, "Inconsistent count of Data items!");
                    }
                    isData = 1;
                }    // isDataString (from name-value pair)


// Test all required data is available

//dgm 30Jul2015         if(!isCPOPath || !isPath || !isType || !isRank || !isShape || !isData || (dataOperation == PUT_OPERATION && !isTimedArg))
                if (!isCPOPath || !isPath || !isType || !isRank || !isShape || !isData) {
                    IDAM_LOG(LOG_ERROR, "imas put: Insufficient data parameters passed - put not possible!\n");
                    THROW_ERROR(999, "Insufficient data parameters passed - put not possible!");
                }

// The type passed here is the IMAS type enumeration (imas_putData is the original imas putData function)

                if (dataOperation == PUT_OPERATION) {
                    rc = imas_putData(clientIdx, CPOPath, path, type, rank, shape, isTimed, data);
                } else {
                    if (dataOperation == PUTSLICE_OPERATION) {
                        IDAM_LOG(LOG_ERROR, "imas put: Slice Time not passed!\n");
                        THROW_ERROR(999, "Slice Time not passed!");
                    } else {
                        rc = imas_putDataX(clientIdx, CPOPath, path, type, rank, shape, dataOperation, data, 0.0);
                    }    // Replace Last Slice
                }
                if (rc < 0) err = 999;

// Housekeeping heap

                if (shape != NULL) free((void*)shape);
                if (data != NULL) free(data);

            } else {        // !isPutData

// dgm 30Jul2015         if(!isCPOPath || !isPath || (dataOperation == PUT_OPERATION && !isTimedArg))
                if (!isCPOPath || !isPath) {
                    IDAM_LOG(LOG_ERROR, "imas put: Insufficient data parameters passed - put not possible!\n");
                    THROW_ERROR(999, "Insufficient data parameters passed - put not possible!");
                }

                int freeShape = 0;
                if (putDataBlock->shape == NULL) {
                    if (putDataBlock->rank > 1) {
                        IDAM_LOG(LOG_ERROR, "imas put: No shape information passed!\n");
                        THROW_ERROR(999, "No shape information passed!");
                    }

                    putDataBlock->shape = (int*)malloc(sizeof(int));
                    putDataBlock->shape[0] = putDataBlock->count;
                    freeShape = 1;
                }

                if (dataOperation == PUT_OPERATION) {
                    rc = imas_putData(clientIdx, CPOPath, path, type, putDataBlock->rank, putDataBlock->shape, isTimed,
                                      (void*)putDataBlock->data);
                } else {
                    if (dataOperation == PUTSLICE_OPERATION) {
                        PUTDATA_BLOCK* putTimeBlock = NULL;
                        for (i = 0; i < putDataBlockList->blockCount; i++) {
                            if (!strcasecmp(putDataBlockList->putDataBlock[i].blockName, "time") ||
                                !strcasecmp(putDataBlockList->putDataBlock[i].blockName, "putDataTime")) {
                                putTimeBlock = &putDataBlockList->putDataBlock[i];
                                break;
                            }
                        }
                        if (putTimeBlock == NULL) {
                            IDAM_LOG(LOG_ERROR, "imas put: No Slice Time!\n");
                            THROW_ERROR(999, "No Slice Time!");
                        }
                        if (putTimeBlock->data_type != TYPE_DOUBLE || putTimeBlock->count != 1) {
                            IDAM_LOG(LOG_ERROR, "imas put: Slice Time type and count are incorrect!\n");
                            THROW_ERROR(999, "Slice Time type and count are incorrect!");
                        }
                        rc = imas_putDataX(clientIdx, CPOPath, path, type, putDataBlock->rank, putDataBlock->shape,
                                           dataOperation, (void*)putDataBlock->data, ((double*)putTimeBlock->data)[0]);
                    } else {
                        rc = imas_putDataX(clientIdx, CPOPath, path, type, rank, shape, dataOperation, (void*)data,
                                           0.0);
                    }    // Replace Last Slice
                }

                if (rc < 0) err = 999;

                if (freeShape && putDataBlock->shape) {
                    free((void*)putDataBlock->shape);
                    putDataBlock->shape = NULL;
                    freeShape = 0;
                }

            }

            if (err != 0) {
                THROW_ERROR(999, "Data PUT method failed!");
            }


// Return a status value

            {
                int* data = (int*)malloc(sizeof(int));
                data[0] = 1;
                initDataBlock(data_block);
                data_block->rank = 0;
                data_block->data_type = TYPE_INT;
                data_block->data = (char*)data;
                data_block->data_n = 1;
            }

            break;
        } else

//----------------------------------------------------------------------------------------
// IMAS open an existing file and update the global idx

        if (!strcasecmp(request_block->function, "open")) {
            int rc = 0;
/*
name	- filename
shot	- experiment number
run	- analysis number
refShot - not used
refRun  - not used
retIdx	- returned data file index number
*/

// Passed name-value pairs

            if (!isFileName || !isShotNumber || !isRunNumber) {
                IDAM_LOG(LOG_ERROR, "imas: A Filename, Shot number and Run number are required!\n");
                THROW_ERROR(999, "A Filename, Shot number and Run number are required!");
            }

            if ((rc = imas_hdf5EuitmOpen(filename, shotNumber, runNumber, &idx)) < 0) {
                THROW_ERROR(999, "Data Open method failed!");
            }

// Return the Index Number

            int* data = (int*)malloc(sizeof(int));
            data[0] = idx;

            putImasIdsShot(idx, shotNumber);
            putImasIdsRun(idx, runNumber);

            initDataBlock(data_block);
            data_block->rank = 0;
            data_block->dims = NULL;
            data_block->data_type = TYPE_INT;
            data_block->data = (char*)data;
            data_block->data_n = 1;

            break;
        } else

//----------------------------------------------------------------------------------------
// IMAS create a new file instance - using a Versioned Device specific IDS model file (the version is stored as header meta data)

        if (!strcasecmp(request_block->function, "create")) {
            int rc = 0;
/*
name	- filename
shot	- experiment number
run	- analysis number
refShot - not used
refRun  - not used
retIdx	- returned data file index number
*/

            if (!isFileName || !isShotNumber || !isRunNumber) {
                IDAM_LOG(LOG_ERROR, "imas: A Filename, Shot number and Run number are required!\n");
                THROW_ERROR(999, "A Filename, Shot number and Run number are required!");
            }

            if (isCreateFromModel) {
                if ((rc = imas_hdf5EuitmCreate(filename, shotNumber, runNumber, refShotNumber, refRunNumber, &idx)) <
                    0) {
                    err = 999;
                    addIdamError(&idamerrorstack, CODEERRORTYPE, "imas", err, getImasErrorMsg());
                    addIdamError(&idamerrorstack, CODEERRORTYPE, "imas", err, "File Create method from Model failed!");
                    break;
                }
            } else {
                if (imas_hdf5IMASCreate(filename, shotNumber, runNumber, refShotNumber, refRunNumber, &idx) < 0) {
                    idamLogWithFunc(LOG_DEBUG, (logFunc)H5Eprint1);
                    addIdamError(&idamerrorstack, CODEERRORTYPE, __func__, 999, getImasErrorMsg());
                    THROW_ERROR(999, "File Create method failed!");
                }
            }

            putImasIdsShot(clientIdx, shotNumber);
            putImasIdsRun(clientIdx, runNumber);

// Return the Index Number

            int* data = (int*)malloc(sizeof(int));
            data[0] = idx;

            initDataBlock(data_block);
            data_block->rank = 0;
            data_block->dims = NULL;
            data_block->data_type = TYPE_INT;
            data_block->data = (char*)data;
            data_block->data_n = 1;

            break;
        } else


//----------------------------------------------------------------------------------------
// IMAS close a file

        if (!strcasecmp(request_block->function, "close")) {
            int rc = 0;

            if (!isClientIdx && !(isFileName && isShotNumber && isRunNumber)) {
                IDAM_LOG(LOG_ERROR, "The file IDX or the Filename with Shot number and Run number are required!\n");
                THROW_ERROR(999, "The file IDX or the Filename with Shot number and Run number are required!");
            }

            if ((rc = imas_hdf5EuitmClose(clientIdx, filename, shotNumber, runNumber)) < 0) {
                THROW_ERROR(999, "Data Close Failed!");
            }

// Return Success

            int* data = (int*)malloc(sizeof(int));
            data[0] = 1;

            initDataBlock(data_block);
            data_block->rank = 0;
            data_block->dims = NULL;
            data_block->data_type = TYPE_INT;
            data_block->data = (char*)data;
            data_block->data_n = 1;

            break;
        } else

//----------------------------------------------------------------------------------------
// IMAS create a MODEL file - groups only, no data

        if (!strcasecmp(request_block->function, "createModel")) {
            int rc = 0;
            int version = 0;

            if (!isFileName) {
                IDAM_LOG(LOG_ERROR, "imas put: A Filename is required!\n");
                THROW_ERROR(999, "A Filename is required!");
            }

            if ((rc = imas_hdf5IdsModelCreate(filename, version)) < 0) {
                addIdamError(&idamerrorstack, CODEERRORTYPE, __func__, 999, getImasErrorMsg());
                THROW_ERROR(999, "File Model Create method failed!");
            }

// Return the Status

            int* data = (int*)malloc(sizeof(int));
            data[0] = 1;

            initDataBlock(data_block);
            data_block->rank = 0;
            data_block->dims = NULL;
            data_block->data_type = TYPE_INT;
            data_block->data = (char*)data;
            data_block->data_n = 1;

            break;
        } else

//----------------------------------------------------------------------------------------
//----------------------------------------------------------------------------------------
// Put a Data Slice into an Object

        if (!strcasecmp(request_block->function, "releaseObject") ||
            !strcasecmp(request_block->function, "putObjectGroup") ||
            !strcasecmp(request_block->function, "putObjectSlice") ||
            !strcasecmp(request_block->function, "replaceLastObjectSlice")) {
            int rc = 0;

// void imas_hdf5ReleaseObject(void *obj)

// Test all required data is available

            if (!isClientObjectId) {
                IDAM_LOG(LOG_ERROR, "imas releaseObject: Required data parameter not passed!\n");
                THROW_ERROR(999, "Required data parameter not passed!");
            }

// Identify the local object

            void* obj = findLocalObj(clientObjectId);

// call the original IMAS function

            if (obj != NULL) imas_hdf5ReleaseObject(obj);

// Return a status value

            initDataBlock(data_block);
            data_block->rank = 0;
            data_block->data_n = 1;
            data_block->data_type = TYPE_INT;
            data_block->data = (char*)malloc(sizeof(int));
            *((int*)data_block->data) = 1;

            break;

        } else

//----------------------------------------------------------------------------------------
// Get a Data Slice from an Object

        if (!strcasecmp(request_block->function, "getObjectObject")) {
            int rc = 0, fid = -1, fileStatus = 0;
            int idamType;

// int imas_hdf5GetObjectFromObject(void *obj, char *hdf5Path, int idx, void **dataObj)

// Test all required data is available

            if (!isPutData || !isIndex || !isPath) {
                IDAM_LOG(LOG_ERROR, "Insufficient data parameters passed - begin not possible!\n");
                THROW_ERROR(999, "Insufficient data parameters passed - begin not possible!");
            }

// Identify the local object

// ***TODO change to passed by name-value reference
            void* obj = findLocalObj(*((int*)putDataBlock->data));

// call the original IMAS function

            void* dataObj = NULL;
            rc = imas_hdf5GetObjectFromObject(obj, path, index, &dataObj);

            if (rc < 0) {
                THROW_ERROR(999, "Object Get failed!");
            }

// Save the object reference

            int* refId = (int*)malloc(sizeof(int));
            refId[0] = putLocalObj(dataObj);

// Return the Object reference

            initDataBlock(data_block);
            data_block->rank = 0;
            data_block->data_type = TYPE_INT;
            data_block->data = (char*)refId;
            data_block->data_n = 1;

            break;

        } else

//----------------------------------------------------------------------------------------
// Put a Data Slice into an Object

        if (!strcasecmp(request_block->function, "getObjectSlice")) {
            int rc = 0, fid = -1, fileStatus = 0;
            int idamType;

// int imas_hdf5GetObjectSlice(int expIdx, char *hdf5Path, char *cpoPath, double time, void **obj)

// Test all required data is available

            if (!isPutData || !isClientIdx || !isHdf5Path || !isCPOPath || putDataBlockList->blockCount != 3) {
                IDAM_LOG(LOG_ERROR, "Insufficient data parameters passed - begin not possible!\n");
                THROW_ERROR(999, "Insufficient data parameters passed - begin not possible!");
            }

// Set global variables

// ***TODO check the organisation of the data
            setSliceIdx(((int*)putDataBlock[1].data)[0], ((int*)putDataBlock[1].data)[1]);
            setSliceTime(((double*)putDataBlock[2].data)[0], ((double*)putDataBlock[2].data)[1]);

// Identify the local object
//         void *obj = findLocalObj(*((int *)putDataBlock[0].data));

// call the original IMAS function

            void* dataObj = NULL;
            rc = imas_hdf5GetObjectSlice(clientIdx, hdf5Path, CPOPath, *((double*)putDataBlock[0].data), &dataObj);

            if (rc < 0) {
                THROW_ERROR(999, "Object Get failed!");
            }

// Save the object reference

            int* refId = (int*)malloc(sizeof(int));
            refId[0] = putLocalObj(dataObj);

// Return the Object reference

            initDataBlock(data_block);
            data_block->rank = 0;
            data_block->data_type = TYPE_INT;
            data_block->data = (char*)refId;
            data_block->data_n = 1;

            break;

        } else
//----------------------------------------------------------------------------------------
// Put a Data Slice into an Object

        if (!strcasecmp(request_block->function, "getObjectGroup")) {
            int rc = 0, fid = -1, fileStatus = 0;
            int idamType;

// int imas_hdf5GetObject(int expIdx, char *hdf5Path, char *cpoPath, void **obj, int isTimed)

// Test all required data is available

            if (!isClientIdx || !isPath || !isCPOPath || !isTimedArg) {
                IDAM_LOG(LOG_ERROR, "Insufficient data parameters passed - begin not possible!\n");
                THROW_ERROR(999, "Insufficient data parameters passed - begin not possible!");
                break;
            }

// Identify the local object
//       void *obj = findLocalObj(*((int *)putDataBlock->data));

// call the original IMAS function

            void* dataObj = NULL;
            rc = imas_hdf5GetObject(clientIdx, path, CPOPath, &dataObj, isTimed);

            if (rc < 0) {
                err = 999;
                addIdamError(&idamerrorstack, CODEERRORTYPE, "imas getObjectGroup", err, "Object Get failed!");
                break;
            }

// Save the object reference

            int* refId = (int*)malloc(sizeof(int));
            refId[0] = putLocalObj(dataObj);

// Return the Object reference

            initDataBlock(data_block);
            data_block->rank = 0;
            data_block->data_type = TYPE_INT;
            data_block->data = (char*)refId;
            data_block->data_n = 1;

            break;

        } else


//----------------------------------------------------------------------------------------
// Object Dimension

        if (!strcasecmp(request_block->function, "getObjectDim")) {
            int rc = 0, fid = -1, fileStatus = 0;
            int idamType;

// Object? The data is the local object reference

            if (putDataBlockList->putDataBlock[0].data == NULL) {
                IDAM_LOG(LOG_ERROR, "imas getObjectDim: No data object has been specified!\n");
                THROW_ERROR(999, "No data object has been specified!");
            }

// Identify the local object
// Two standard references are created at initialisation: NULL and -1 with refIds of 0 and 1.

            obj_t* obj = findLocalObj(*((int*)putDataBlock[0].data));

// Save the object reference

            int* dim = (int*)malloc(sizeof(int));

            if (obj != NULL) {
                dim[0] = obj->dim;
            } else {
                dim[0] = 0;
            }

// Return the Object property

            initDataBlock(data_block);
            data_block->rank = 0;
            data_block->data_type = TYPE_INT;
            data_block->data = (char*)dim;
            data_block->data_n = 1;

            break;

        } else


//----------------------------------------------------------------------------------------
// Put a Data Slice into an Object

        if (!strcasecmp(request_block->function, "beginObject")) {
            int rc = 0, fid = -1, fileStatus = 0;
            int idamType;

// void *imas_hdf5BeginObject(int expIdx, void *obj, int index, const char *relPath, int isTimed)

// Test all required data is available

            if (!isClientIdx || !isIndex || !isRelPath || !isPutData || putDataBlockList->blockCount != 2 ||
                !isTimedArg) {
                IDAM_LOG(LOG_ERROR, "Insufficient data parameters passed - begin not possible!\n");
                THROW_ERROR(999, "Insufficient data parameters passed - begin not possible!");
            }

// Object? The data is the local object reference

            if (putDataBlockList->putDataBlock[0].data == NULL) {
                IDAM_LOG(LOG_ERROR, "imas beginObject: No data object has been specified!\n");
                THROW_ERROR(999, "No data object has been specified!");
            }

// Set global variables

            setSliceIdx(((int*)putDataBlock[1].data)[0], ((int*)putDataBlock[1].data)[1]);

// Identify the local object
// Two standard references are created at initialisation: NULL and -1 with refIds of 0 and 1.

            void* obj = findLocalObj(*((int*)putDataBlock[0].data));

// The type passed here is the IMAS type enumeration

            void* data = imas_hdf5BeginObject(clientIdx, obj, index, (const char*)relPath, isTimed);

            if (data == NULL) {
                THROW_ERROR(999, "Object Begin failed!");
            }

// Save the object reference

            int* refId = (int*)malloc(sizeof(int));
            refId[0] = putLocalObj(data);

// Return the Object reference

            initDataBlock(data_block);
            data_block->rank = 0;
            data_block->data_type = TYPE_INT;
            data_block->data = (char*)refId;
            data_block->data_n = 1;

            break;

        } else

//----------------------------------------------------------------------------------------
// Put a Data Slice into an Object

        if (!strcasecmp(request_block->function, "getObject")) {
            int rc = 0, fid = -1, fileStatus = 0;
            int idamType;

//       rc = imas_getDataSliceFromObject(void *obj, char *path, int index, int type, int nDims, int *dims, void **data)

// Test all required data is available

            if (!isPath || !isIndex || !isTypeName || !isRank || !isClientObjectId) {
                IDAM_LOG(LOG_ERROR, "Insufficient data parameters passed - gett not possible!\n");
                THROW_ERROR(999, "Insufficient data parameters passed - get not possible!");
            }

// Identify the local object

            void* obj = findLocalObj(clientObjectId);

// The type passed here is the IMAS type enumeration

            void* data = NULL;
            int type = findIMASType(typeName);

// Object Shape

            int* dims = (int*)malloc((rank + 1) * sizeof(int));

            rc = imas_getDataSliceFromObject(obj, path, index, type, rank, dims, &data);

            if (rc < 0) {
                free(data);
                THROW_ERROR(999, "Data GET method failed!");
            }

// If there is no data, dims is set to 1 and the data are set to EMPTY_<type>

// Return the Object's Data

            initDataBlock(data_block);
            data_block->rank = rank;
            data_block->data_type = findIMASIDAMType(type);
            data_block->data = (char*)data;
            data_block->data_n = 1;

            if (data_block->rank > 0) {
                data_block->dims = (DIMS*)malloc(data_block->rank * sizeof(DIMS));
                for (i = 0; i < data_block->rank; i++) {
                    initDimBlock(&data_block->dims[i]);
                    data_block->dims[i].dim_n = dims[i];
                    data_block->dims[i].data_type = TYPE_UNSIGNED_INT;
                    data_block->dims[i].compressed = 1;
                    data_block->dims[i].dim0 = 0.0;
                    data_block->dims[i].diff = 1.0;
                    data_block->dims[i].method = 0;
                    data_block->data_n *= dims[i];
                }
            } else {
                data_block->data_n = dims[0];
            }

            break;

        } else
//----------------------------------------------------------------------------------------
// Put a Data Slice into an Object

        if (!strcasecmp(request_block->function, "putObject")) {
            int rc = 0, fid = -1, fileStatus = 0;
            int type;

// Test all required data is available

            if (!isPath || !isIndex || !isPutData || putDataBlockList->blockCount != 2) {
                IDAM_LOG(LOG_ERROR, "Insufficient data parameters passed - put not possible!\n");
                THROW_ERROR(999, "Insufficient data parameters passed - put not possible!");
            }

            if ((type = findIMASType(convertIdam2StringType(putDataBlockList->putDataBlock[1].data_type))) ==
                0) {        // Convert an IDAM type to an IMAS type
                IDAM_LOG(LOG_ERROR, "imas putObject: The data's Type cannot be converted!\n");
                THROW_ERROR(999, "The data's Type cannot be converted!");
            }

// Any Data?

            if (putDataBlockList->putDataBlock[1].data == NULL) {
                IDAM_LOG(LOG_ERROR, "imas putObject: No data has been specified!\n");
                THROW_ERROR(999, "No data has been specified!");
            }

// Object?

            if (putDataBlockList->putDataBlock[0].data == NULL) {
                IDAM_LOG(LOG_ERROR, "imas putObject: No data object has been specified!\n");
                THROW_ERROR(999, "No data object has been specified!");
            }

// Identify the local object

            void* obj = findLocalObj(*((int*)putDataBlockList->putDataBlock[0].data));

// Create the shape array if the rank is 1 (not passed by IDAM)

            int shape[1];
            if (putDataBlockList->putDataBlock[1].rank == 1 && putDataBlockList->putDataBlock[1].shape == NULL) {
                putDataBlockList->putDataBlock[1].shape = shape;
                shape[0] = putDataBlockList->putDataBlock[1].count;
            }

// The type passed here is the IMAS type enumeration

            rc = imas_putDataSliceInObject(obj, path, index, type, putDataBlockList->putDataBlock[1].rank,
                                           putDataBlockList->putDataBlock[1].shape,
                                           (void*)putDataBlockList->putDataBlock[1].data);

            if (putDataBlockList->putDataBlock[1].rank == 1 && putDataBlockList->putDataBlock[1].shape == shape) {
                putDataBlockList->putDataBlock[1].shape = NULL;
            }

            if (rc < 0) {
                THROW_ERROR(999, "Data PUT method failed!");
            }

// Return a status value

            int* data = (int*)malloc(sizeof(int));
            data[0] = 1;

            initDataBlock(data_block);
            data_block->rank = 0;
            data_block->dims = NULL;
            data_block->data_type = TYPE_INT;
            data_block->data = (char*)data;
            data_block->data_n = 1;

            break;

        } else

//----------------------------------------------------------------------------------------
// Put an Object into an Object

        if (!strcasecmp(request_block->function, "putObjectInObject")) {

// No HDF5 function in UAL

// Test all required data is available

            if (!isClientObjectId) {
                IDAM_LOG(LOG_ERROR, "Insufficient data parameters passed - put not possible!\n");
                THROW_ERROR(999, "Insufficient data parameters passed - put not possible!");
            }

// Return the Passed Object Reference

            int* data = (int*)malloc(sizeof(int));
            data[0] = clientObjectId;

            initDataBlock(data_block);
            data_block->rank = 0;
            data_block->dims = NULL;
            data_block->data_type = TYPE_INT;
            data_block->data = (char*)data;
            data_block->data_n = 1;

            break;

        } else


//----------------------------------------------------------------------------------------
// Begin/End IDS operations

        if (!strcasecmp(request_block->function, "beginIdsPut")) {

// Return Success

            int* data = (int*)malloc(sizeof(int));
            data[0] = 1;

            initDataBlock(data_block);
            data_block->data_type = TYPE_INT;
            data_block->data = (char*)data;
            data_block->data_n = 1;

            break;

        } else if (!strcasecmp(request_block->function, "endIdsPut")) {

// Return Success

            int* data = (int*)malloc(sizeof(int));
            data[0] = 1;

            initDataBlock(data_block);
            data_block->data_type = TYPE_INT;
            data_block->data = (char*)data;
            data_block->data_n = 1;

            break;

        } else

//----------------------------------------------------------------------------------------
// Standard Plugin Functions
//----------------------------------------------------------------------------------------

// Help: A Description of library functionality

        if (!strcasecmp(request_block->function, "help")) {

            char* p = (char*)malloc(sizeof(char) * 2 * 1024);

            strcpy(p, "\nimas: Add Functions Names, Syntax, and Descriptions\n\n");

            initDataBlock(data_block);

            data_block->rank = 1;
            data_block->dims = (DIMS*)malloc(data_block->rank * sizeof(DIMS));
            for (i = 0; i < data_block->rank; i++) initDimBlock(&data_block->dims[i]);

            data_block->data_type = TYPE_STRING;
            strcpy(data_block->data_desc, "imas: help = description of this plugin");

            data_block->data = p;

            data_block->dims[0].data_type = TYPE_UNSIGNED_INT;
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

//----------------------------------------------------------------------------------------
// Standard methods: version, builddate, defaultmethod, maxinterfaceversion

        if (!strcasecmp(request_block->function, "version")) {
            initDataBlock(data_block);
            data_block->data_type = TYPE_INT;
            data_block->rank = 0;
            data_block->data_n = 1;
            int* data = (int*)malloc(sizeof(int));
            data[0] = THISPLUGIN_VERSION;
            data_block->data = (char*)data;
            strcpy(data_block->data_desc, "Plugin version number");
            strcpy(data_block->data_label, "version");
            strcpy(data_block->data_units, "");
            break;
        } else

// Plugin Build Date

        if (!strcasecmp(request_block->function, "builddate")) {
            initDataBlock(data_block);
            data_block->data_type = TYPE_STRING;
            data_block->rank = 0;
            data_block->data_n = strlen(__DATE__) + 1;
            char* data = (char*)malloc(data_block->data_n * sizeof(char));
            strcpy(data, __DATE__);
            data_block->data = (char*)data;
            strcpy(data_block->data_desc, "Plugin build date");
            strcpy(data_block->data_label, "date");
            strcpy(data_block->data_units, "");
            break;
        } else

// Plugin Default Method

        if (!strcasecmp(request_block->function, "defaultmethod")) {
            initDataBlock(data_block);
            data_block->data_type = TYPE_STRING;
            data_block->rank = 0;
            data_block->data_n = strlen(THISPLUGIN_DEFAULT_METHOD) + 1;
            char* data = (char*)malloc(data_block->data_n * sizeof(char));
            strcpy(data, THISPLUGIN_DEFAULT_METHOD);
            data_block->data = (char*)data;
            strcpy(data_block->data_desc, "Plugin default method");
            strcpy(data_block->data_label, "method");
            strcpy(data_block->data_units, "");
            break;
        } else

// Plugin Maximum Interface Version

        if (!strcasecmp(request_block->function, "maxinterfaceversion")) {
            initDataBlock(data_block);
            data_block->data_type = TYPE_INT;
            data_block->rank = 0;
            data_block->data_n = 1;
            int* data = (int*)malloc(sizeof(int));
            data[0] = THISPLUGIN_MAX_INTERFACE_VERSION;
            data_block->data = (char*)data;
            strcpy(data_block->data_desc, "Maximum Interface Version");
            strcpy(data_block->data_label, "version");
            strcpy(data_block->data_units, "");
            break;
        } else {

//======================================================================================
// Error ...

            THROW_ERROR(999, "Unknown function requested!");
        }

    } while (0);

//--------------------------------------------------------------------------------------
// Housekeeping

    return err;
}

