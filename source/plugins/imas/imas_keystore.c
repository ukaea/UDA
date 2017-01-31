/*---------------------------------------------------------------
* v1 IDAM Plugin: ITER IMAS mdsplus put/get
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
* Change History
*
* 20Aug2015	D.G.Muir	Original Version based on imas.c
				pluginFileList disabled as all tree management is within the mdsplus system
				getImasIdsVersion & getImasIdsDevice commented out
				All object related code commented out
*---------------------------------------------------------------------------------------------------------------*/

#include "imas_mds.h"

#include "ual_low_level.h"
#include "extract_indices.h"

#include <idamErrorLog.h>
#include <initStructs.h>
#include <stringUtils.h>
#include <managePluginFiles.h>
#include <makeServerRequestBlock.h>
#include <accAPI_C.h>
#include <idamServerPlugin.h>
#include <idamLog.h>
#include <regex.h>

IDAMPLUGINFILELIST pluginFileList_mds;

char* spawnCommand(char* command, char* ipAddress);

#define MAXOBJECTCOUNT 100000
static void* localObjs[MAXOBJECTCOUNT];
static unsigned int lastObjectId = 0;
static unsigned int initLocalObjs = 1;

static int process_arguments(IDAM_PLUGIN_INTERFACE* idam_plugin_interface, PLUGIN_ARGS* plugin_args);
static int do_putIdsVersion(IDAM_PLUGIN_INTERFACE* idam_plugin_interface, PLUGIN_ARGS plugin_args);
static int do_get(IDAM_PLUGIN_INTERFACE* idam_plugin_interface, PLUGIN_ARGS plugin_args, int idx);
static int do_put(IDAM_PLUGIN_INTERFACE* idam_plugin_interface, PLUGIN_ARGS plugin_args, int idx);
static int do_open(IDAM_PLUGIN_INTERFACE* idam_plugin_interface, PLUGIN_ARGS plugin_args, int idx);
static int do_create(IDAM_PLUGIN_INTERFACE* idam_plugin_interface, PLUGIN_ARGS plugin_args, int idx);
static int do_close(IDAM_PLUGIN_INTERFACE* idam_plugin_interface, PLUGIN_ARGS plugin_args, int idx);

void initLocalObj()
{
// Original IMAS objects may have NULL or (void *)-1 addresses
// Two standard references are created at initialisation for these with refIds of 0 and 1.
// These are not object pointers - both are set to NULL
    int i;
    if (!initLocalObjs) return;
    for (i = 0; i < MAXOBJECTCOUNT; i++) {
        localObjs[i] = NULL;
    }
    initLocalObjs = 0;
    localObjs[0] = NULL;    // Original IMAS objects may have NULL or (void *)-1 addresses
    localObjs[1] = NULL;
    lastObjectId = 2;
    return;
}

int putLocalObj(void* dataObj)
{
    int i;

// if this is the same as a previous object it may contain changes
// Check and return the saved object ID

    for (i = 0; i < lastObjectId; i++) if (localObjs[i] == dataObj) return i;

// Save new object

    localObjs[lastObjectId] = dataObj;
    return lastObjectId++;
}

void* findLocalObj(int refId)
{
    if (refId < lastObjectId) return localObjs[refId];
    return NULL;
}

static char imasIdsVersion[256] = "";   // The IDS Data Model Version
static char imasIdsDevice[256] = "";    // The IDS Data Model Device name

void putImasIdsVersion(const char* version)
{
    strcpy(imasIdsVersion, version);
}

char* getImasIdsVersion()
{
    return imasIdsVersion;
}

void putImasIdsDevice(const char* device)
{
    strcpy(imasIdsDevice, device);
}

char* getImasIdsDevice()
{
    return imasIdsDevice;
}

static char TimeBasePath[TIMEBASEPATHLENGTH];

void putTimeBasePath(char* timeBasePath)
{
    if (strlen(timeBasePath) < TIMEBASEPATHLENGTH)
        strcpy(TimeBasePath, timeBasePath);
    else
        TimeBasePath[0] = '\0';
}

char* getTimeBasePath()
{
    return TimeBasePath;
}

// dgm  Convert name to IMAS type
int findIMASType(char* typeName)
{
    if (typeName == NULL) return (UNKNOWN_TYPE);
    if (!strcasecmp(typeName, "int")) return INT;
    if (!strcasecmp(typeName, "float")) return FLOAT;
    if (!strcasecmp(typeName, "double")) return DOUBLE;
    if (!strcasecmp(typeName, "string")) return STRING;
    return (UNKNOWN_TYPE);
}

// dgm  Convert IMAS type to IDAM type

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
    }
    return TYPE_UNKNOWN;
}

static int sliceIdx1, sliceIdx2;
static double sliceTime1, sliceTime2;

void setSliceIdx(int index1, int index2)
{
    sliceIdx1 = index1;
    sliceIdx2 = index2;
}

void setSliceTime(double time1, double time2)
{
    sliceTime1 = time1;
    sliceTime2 = time2;
}

extern int imas_mds(IDAM_PLUGIN_INTERFACE* idam_plugin_interface)
{
    int err = 0;

//----------------------------------------------------------------------------------------
// State Variables

    static short init = 0;
    static int idx = 0;                // Last Opened File Index value

//----------------------------------------------------------------------------------------
// Standard v1 Plugin Interface

    unsigned short housekeeping;

    if (idam_plugin_interface->interfaceVersion > THISPLUGIN_MAX_INTERFACE_VERSION) {
        err = 999;
        idamLog(LOG_ERROR,
                "ERRO imas: Plugin Interface Version Unknown to this plugin: Unable to execute the request!\n");
        addIdamError(&idamerrorstack, CODEERRORTYPE, "imas", err,
                     "Plugin Interface Version Unknown to this plugin: Unable to execute the request!");
        return err;
    }

    idam_plugin_interface->pluginVersion = THISPLUGIN_VERSION;

    REQUEST_BLOCK* request_block = idam_plugin_interface->request_block;

    if (idam_plugin_interface->housekeeping || !strcasecmp(request_block->function, "reset")) {

        if (!init) return 0;        // Not previously initialised: Nothing to do!

// Free Heap & reset counters

        init = 0;
        idx = 0;
        putImasIdsVersion("");

        initLocalObj();

        putTimeBasePath("");

        return 0;
    }

//----------------------------------------------------------------------------------------
// Initialise

    if (!init || !strcasecmp(request_block->function, "init") || !strcasecmp(request_block->function, "initialise")) {
        initIdamPluginFileList(&pluginFileList_mds);
        initLocalObj();
        putTimeBasePath("");

        init = 1;
        if (!strcasecmp(request_block->function, "init") || !strcasecmp(request_block->function, "initialise")) {
            return 0;
        }
    }

//----------------------------------------------------------------------------------------
// Common Passed name-value pairs and Keywords

    PLUGIN_ARGS plugin_args;
    process_arguments(idam_plugin_interface, &plugin_args);

//----------------------------------------------------------------------------------------
// Plugin Functions
//----------------------------------------------------------------------------------------
/*
idx	- reference to the open data file: file handle from an array of open files - hdf5Files[idx]
cpoPath	- the root group where the CPO/IDS is written
path	- the path relative to the root (cpoPath) where the data are written (must include the variable name!)
*/

    if (!strcasecmp(request_block->function, "putIdsVersion")) {
        err = do_putIdsVersion(idam_plugin_interface, plugin_args);
    } else if (!strcasecmp(request_block->function, "delete")) {
        err = do_delete(idam_plugin_interface, plugin_args);
    } else if (!strcasecmp(request_block->function, "get")) {
        err = do_get(idam_plugin_interface, plugin_args, idx);
    } else if (!strcasecmp(request_block->function, "put")) {
        err = do_put(idam_plugin_interface, plugin_args, idx);
    } else if (!strcasecmp(request_block->function, "open")) {
        err = do_open(idam_plugin_interface, plugin_args, idx);
    } else if (!strcasecmp(request_block->function, "create")) {
        err = do_create(idam_plugin_interface, plugin_args, idx);
    } else if (!strcasecmp(request_block->function, "close")) {
        err = do_close(idam_plugin_interface, plugin_args, int);
    } else {
        err = 999;
        addIdamError(&idamerrorstack, CODEERRORTYPE, "imas", err, "Unknown function requested!");
    }

//--------------------------------------------------------------------------------------
// Housekeeping

    char* p;
    if (err != 0 && (p = imas_last_errmsg()) != 0) {
        TrimString(p);
        if (strlen(p) > 0) {
            err = 999;
            addIdamError(&idamerrorstack, CODEERRORTYPE, "imas", err, p);
        }
    }

    return err;
}

static int process_arguments(IDAM_PLUGIN_INTERFACE* idam_plugin_interface, PLUGIN_ARGS* plugin_args)
{
    memset(plugin_args, '\0', sizeof(PLUGIN_ARGS));

    // Default values

    plugin_args->quote = '\"';
    plugin_args->delimiter = ',';

    // Arguments and keywords

    REQUEST_BLOCK* request_block = idam_plugin_interface->request_block;

    plugin_args->isCPOPath = findStringValue(&request_block->nameValueList, &plugin_args->CPOPath, "group|cpoPath|cpo");
    plugin_args->isPath = findStringValue(&request_block->nameValueList, &plugin_args->path, "variable|path");
    plugin_args->isTypeName = findStringValue(&request_block->nameValueList, &plugin_args->typeName, "type");
    plugin_args->isClientIdx = findIntValue(&request_block->nameValueList, &plugin_args->clientIdx, "idx");
    plugin_args->isClientObjectId = findIntValue(&request_block->nameValueList, &plugin_args->clientObjectId, "clientObjectId|ObjectId");
    plugin_args->isRank = findIntValue(&request_block->nameValueList, &plugin_args->rank, "rank|ndims");
    plugin_args->isIndex = findIntValue(&request_block->nameValueList, &plugin_args->index, "index");
    plugin_args->isCount = findIntValue(&request_block->nameValueList, &plugin_args->count, "count");
    plugin_args->isShapeString = findStringValue(&request_block->nameValueList, &plugin_args->shapeString, "shape|dims");
    plugin_args->isDataString = findStringValue(&request_block->nameValueList, &plugin_args->dataString, "data");
    plugin_args->quote = findValue(&request_block->nameValueList, "singlequote") ? '\'' : plugin_args->quote;
    plugin_args->quote = findValue(&request_block->nameValueList, "doublequote") ? '\"' : plugin_args->quote;
    plugin_args->delimiter = findValue(&request_block->nameValueList, "delimiter") ? '\'' : plugin_args->delimiter;
    plugin_args->isFileName = findStringValue(&request_block->nameValueList, &plugin_args->filename, "filename|file|name");
    plugin_args->isShotNumber = findIntValue(&request_block->nameValueList, &plugin_args->shotNumber, "shotNumber|shot|pulse|exp_number");
    plugin_args->isRunNumber = findIntValue(&request_block->nameValueList, &plugin_args->runNumber, "runNumber|run|pass|sequence");
    plugin_args->isRefShotNumber = findIntValue(&request_block->nameValueList, &plugin_args->refShotNumber, "refShotNumber|refShot");
    plugin_args->isRefRunNumber = findIntValue(&request_block->nameValueList, &plugin_args->refRunNumber, "refRunNumber|refRun");
    plugin_args->isTimedArg = findIntValue(&request_block->nameValueList, &plugin_args->isTimed, "isTimed");
    plugin_args->isInterpolMode = findIntValue(&request_block->nameValueList, &plugin_args->interpolMode, "interpolMode");
    plugin_args->isSignal = findStringValue(&request_block->nameValueList, &plugin_args->signal, "signal");
    plugin_args->isSource = findStringValue(&request_block->nameValueList, &plugin_args->source, "source");
    plugin_args->isFormat = findStringValue(&request_block->nameValueList, &plugin_args->format, "format|pattern");
    plugin_args->isOwner = findStringValue(&request_block->nameValueList, &plugin_args->owner, "owner");
    plugin_args->isServer = findStringValue(&request_block->nameValueList, &plugin_args->server, "server");
    plugin_args->isImasIdsVersion = findStringValue(&request_block->nameValueList, &plugin_args->imasIdsVersion, "imasIdsVersion|idsVersion");
    plugin_args->isImasIdsDevice = findStringValue(&request_block->nameValueList, &plugin_args->imasIdsDevice, "imasIdsDevice|idsDevice|device");
    plugin_args->isSetLevel = findIntValue(&request_block->nameValueList, &plugin_args->setLevel, "setLevel");
    plugin_args->isCommand = findStringValue(&request_block->nameValueList, &plugin_args->command, "command");
    plugin_args->isIPAddress = findStringValue(&request_block->nameValueList, &plugin_args->IPAddress, "IPAddress");
    plugin_args->isTimes = findStringValue(&request_block->nameValueList, &plugin_args->timesString, "times");
    plugin_args->isPutDataSlice = findValue(&request_block->nameValueList, "putSlice");
    plugin_args->isReplaceLastDataSlice = findValue(&request_block->nameValueList, "replaceSlice");
    plugin_args->isGetDataSlice = findValue(&request_block->nameValueList, "getSlice");
    plugin_args->isGetDimension = findValue(&request_block->nameValueList, "getDimension");
    plugin_args->isCreateFromModel = findValue(&request_block->nameValueList, "CreateFromModel");
    plugin_args->isFlush = findValue(&request_block->nameValueList, "flush");
    plugin_args->isDiscard = findValue(&request_block->nameValueList, "discard");
    plugin_args->isGetLevel = findValue(&request_block->nameValueList, "getLevel");
    plugin_args->isFlushCPO = findValue(&request_block->nameValueList, "flushcpo");
    plugin_args->isDisable = findValue(&request_block->nameValueList, "disable");
    plugin_args->isEnable = findValue(&request_block->nameValueList, "enable");
    plugin_args->isBeginIDSSlice = findValue(&request_block->nameValueList, "beginIDSSlice");
    plugin_args->isEndIDSSlice = findValue(&request_block->nameValueList, "endIDSSlice");
    plugin_args->isReplaceIDSSlice = findValue(&request_block->nameValueList, "replaceIDSSlice");
    plugin_args->isBeginIDS = findValue(&request_block->nameValueList, "beginIDS");
    plugin_args->isEndIDS = findValue(&request_block->nameValueList, "endIDS");
    plugin_args->isBeginIDSTimed = findValue(&request_block->nameValueList, "beginIDSTimed");
    plugin_args->isEndIDSTimed = findValue(&request_block->nameValueList, "endIDSTimed");
    plugin_args->isBeginIDSNonTimed = findValue(&request_block->nameValueList, "beginIDSNonTimed");
    plugin_args->isEndIDSNonTimed = findValue(&request_block->nameValueList, "endIDSNonTimed");
}

//----------------------------------------------------------------------------------------
// IDS Version
static int do_putIdsVersion(IDAM_PLUGIN_INTERFACE* idam_plugin_interface, PLUGIN_ARGS plugin_args)
{
    if (!plugin_args.isImasIdsVersion && !plugin_args.isImasIdsDevice) {
        RAISE_PLUGIN_ERROR("An IDS Version number or a Device name is required!");
    }

    if (plugin_args.isImasIdsVersion) putImasIdsVersion(plugin_args.imasIdsVersion);
    if (plugin_args.isImasIdsDevice) putImasIdsDevice(plugin_args.imasIdsDevice);

// Return the Status OK

    int* data = (int*) malloc(sizeof(int));
    data[0] = 1;

    DATA_BLOCK* data_block = idam_plugin_interface->data_block;

    initDataBlock(data_block);
    data_block->rank = 0;
    data_block->dims = NULL;
    data_block->data_type = TYPE_INT;
    data_block->data = (char*) data;
    data_block->data_n = 1;

    return 0;
}

//----------------------------------------------------------------------------------------
// IMAS get some data - assumes the file exists

static int do_get(IDAM_PLUGIN_INTERFACE* idam_plugin_interface, PLUGIN_ARGS plugin_args, int idx)
{

/*
idx	- reference to the open data file: file handle from an array of open files - hdf5Files[idx]
cpoPath	- the root group where the CPO/IDS is written
path	- the path relative to the root (cpoPath) where the data are written (must include the variable name!)
*/

// Convert type string into IMAS type identifiers

    int type;
    if (!plugin_args.isGetDimension) {
        if (!plugin_args.isTypeName && !plugin_args.isPutData) {
            RAISE_PLUGIN_ERROR("The data's Type has not been specified!");
        }
        if ((type = findIMASType(plugin_args.typeName)) == 0) {
            RAISE_PLUGIN_ERROR("The data's Type name cannot be converted!");
        }
    }

    if (!plugin_args.isPutData && !plugin_args.isRank && !plugin_args.isGetDimension) {
        RAISE_PLUGIN_ERROR("The data's Rank has not been specified!");
    }

    if (plugin_args.isGetDimension) plugin_args.rank = 7;

    int* shape = (int*) malloc((plugin_args.rank + 1) * sizeof(int));
    shape[0] = 1;

    int i;
    for (i = 1; i < plugin_args.rank; i++) shape[i] = 0;

// GET and return the data

    DATA_BLOCK* data_block = idam_plugin_interface->data_block;

    initDataBlock(data_block);

    IDAM_LOGF(LOG_ERROR, "%s, path: %s, type: %d, rank: %d, shape[0]: %d\n", plugin_args.CPOPath,
            plugin_args.path, type,
            plugin_args.rank,
            plugin_args.rank > 0 ? shape[0] : 0);

    CALL_PLUGIN(idam_plugin_interface, "KEYVALUE::read(key=/IDS/%d/%s/%s/DATA)", idx, plugin_args.CPOPath, plugin_args.path);
    char* imasData = idam_plugin_interface->data_block->data;

    if (imasData == NULL) {
        // data not in IDS - go to other plugins to try and get it

        char* expName = NULL;
        FIND_REQUIRED_STRING_VALUE(idam_plugin_interface->request_block->nameValueList, expName);

        if (expName == NULL) {
            RAISE_PLUGIN_ERROR("No IDAM data plugin specified");
        } else {
            char* path = NULL;
            int* indices = NULL;
            int num_indices = extract_array_indices(plugin_args.path, &path, &indices);
            char* indices_string = indices_to_string(indices, num_indices);

            int get_shape = 0;

            size_t len = strlen(path);
            if (len > 5 && (!strcmp(path + (len - 5), "/time") || !strcmp(path + (len - 5), "/data"))) {
                path[len - 5] = '\0';
            }

            const char* fmt = path[0] == '/'
                              ? "%s%sread(element=%s%s, indices=%s, shot=%d%s)"
                              : "%s%sread(element=%s/%s, indices=%s, shot=%d%s)";

            int shot = plugin_args.isShotNumber
                       ? plugin_args.shotNumber
                       : ual_get_shot(idx);

            CALL_PLUGIN(idam_plugin_interface, "%s::read(element=%s%s, indices=%s, shot=%d%s)", expName,
                        plugin_args.CPOPath, path,
                        indices_string, shot, get_shape ? ", get_shape" : "");

            DATA_BLOCK* data_block = idam_plugin_interface->data_block;
            for (i = 0; i < data_block->rank; ++i) {
                shape[i] = data_block->dims[i].dim_n;
            }

            int idam_type = findIMASIDAMType(type);
            if (idam_type != data_block->data_type) {
                if (idam_type == TYPE_DOUBLE && data_block->data_type == TYPE_FLOAT) {
                    imasData = malloc(data_block->data_n * sizeof(double));
                    for (i = 0; i < data_block->data_n; ++i) {
                        ((double*)imasData)[i] = ((float*)data_block->data)[i];
                    }
                    free(data_block->data);
                    data_block->data = imasData;
                } else {
                    RAISE_PLUGIN_ERROR("wrong data type returned");
                }
            } else {
                imasData = data_block->data;
            }

            CALL_PLUGIN(idam_plugin_interface, "KEYVALUE::write(key=/IDS/%d/%s/%s/TYPE, value=%d)", idx, plugin_args.CPOPath, plugin_args.path, plugin_args.typeName);
            CALL_PLUGIN(idam_plugin_interface, "KEYVALUE::write(key=/IDS/%d/%s/%s/RANK, value=%d)", idx, plugin_args.CPOPath, plugin_args.path, plugin_args.rank);
            CALL_PLUGIN(idam_plugin_interface, "KEYVALUE::write(key=/IDS/%d/%s/%s/SHAPE, value=%d)", idx, plugin_args.CPOPath, plugin_args.path, plugin_args.shapeString);
            CALL_PLUGIN(idam_plugin_interface, "KEYVALUE::write(key=/IDS/%d/%s/%s/DATA, value=%d)", idx, plugin_args.CPOPath, plugin_args.path, imasData);
        }
    }

    data_block->rank = plugin_args.rank;
    data_block->data_type = findIMASIDAMType(type);
    data_block->data = imasData;
    if (data_block->data_type == TYPE_STRING && plugin_args.rank <= 1) {
        data_block->data_n = strlen(imasData) + 1;
        shape[0] = data_block->data_n;
    } else {
        data_block->data_n = shape[0];
        for (i = 1; i < plugin_args.rank; i++) {
            data_block->data_n *= shape[i];
        }
    }

    return 0;
}

//----------------------------------------------------------------------------------------
// IMAS put some data - assumes the file exists

static int do_put(IDAM_PLUGIN_INTERFACE* idam_plugin_interface, PLUGIN_ARGS plugin_args, int idx)
{
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

    if (!plugin_args.isTypeName) {
        RAISE_PLUGIN_ERROR("The data's Type has not been specified!");
    }

    int type;
    if ((type = findIMASType(plugin_args.typeName)) == 0) {
        RAISE_PLUGIN_ERROR("The data's Type name cannot be converted!");
    }

    if (!plugin_args.isCPOPath || !plugin_args.isPath) {
        RAISE_PLUGIN_ERROR("Insufficient data parameters passed - put not possible!");
    }

    CALL_PLUGIN(idam_plugin_interface, "KEYVALUE::write(key=/IDS/%d/%s/%s/TYPE, value=%d)", idx, plugin_args.CPOPath, plugin_args.path, plugin_args.typeName);
    CALL_PLUGIN(idam_plugin_interface, "KEYVALUE::write(key=/IDS/%d/%s/%s/RANK, value=%d)", idx, plugin_args.CPOPath, plugin_args.path, plugin_args.rank);
    CALL_PLUGIN(idam_plugin_interface, "KEYVALUE::write(key=/IDS/%d/%s/%s/SHAPE, value=%d)", idx, plugin_args.CPOPath, plugin_args.path, plugin_args.shapeString);
    CALL_PLUGIN(idam_plugin_interface, "KEYVALUE::write(key=/IDS/%d/%s/%s/DATA, value=%d)", idx, plugin_args.CPOPath, plugin_args.path, plugin_args.dataString);

// Return a status value

    {
        DATA_BLOCK* data_block = idam_plugin_interface->data_block;

        int* data = (int*) malloc(sizeof(int));
        data[0] = OK_RETURN_VALUE;
        initDataBlock(data_block);
        data_block->rank = 0;
        data_block->data_type = TYPE_INT;
        data_block->data = (char*) data;
        data_block->data_n = 1;
    }

    return 0;
}

static int do_open(IDAM_PLUGIN_INTERFACE* idam_plugin_interface, PLUGIN_ARGS plugin_args, int idx)
{
    CALL_PLUGIN(idam_plugin_interface, "KEYVALUE::write(key=/IDS/%d/SHOT, value=%d)", idx, plugin_args.shotNumber);
    CALL_PLUGIN(idam_plugin_interface, "KEYVALUE::write(key=/IDS/%d/RUN, value=%d)", idx, plugin_args.runNumber);

    // Return the Index Number
    int* data = (int*) malloc(sizeof(int));
    data[0] = idx;

    DATA_BLOCK* data_block = idam_plugin_interface->data_block;

    initDataBlock(data_block);
    data_block->rank = 0;
    data_block->dims = NULL;
    data_block->data_type = TYPE_INT;
    data_block->data = (char*) data;
    data_block->data_n = 1;

    return 0;
}

static int do_create(IDAM_PLUGIN_INTERFACE* idam_plugin_interface, PLUGIN_ARGS plugin_args, int idx)
{
    return do_open(idam_plugin_interface, plugin_args, idx);
}

//----------------------------------------------------------------------------------------
// IMAS close a file

static int do_close(IDAM_PLUGIN_INTERFACE* idam_plugin_interface, PLUGIN_ARGS plugin_args, int idx)
{
    CALL_PLUGIN(idam_plugin_interface, "KEYVALUE::delete(key=/IDS/%d/SHOT)", idx, plugin_args.shotNumber);
    CALL_PLUGIN(idam_plugin_interface, "KEYVALUE::delete(key=/IDS/%d/RUN)", idx, plugin_args.runNumber);

    // Return the Index Number
    int* data = (int*) malloc(sizeof(int));
    data[0] = idx;

    DATA_BLOCK* data_block = idam_plugin_interface->data_block;

    initDataBlock(data_block);
    data_block->rank = 0;
    data_block->dims = NULL;
    data_block->data_type = TYPE_INT;
    data_block->data = (char*) data;
    data_block->data_n = 1;
}
