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
*---------------------------------------------------------------------------------------------------------------*/
#include "imas_mds.h"

#include "common.h"
#include "extract_indices.h"
#include "imas.h"
#include "ual_low_level.h"
#include "ual_low_level_mdsplus.h"
#include "imas_hdf5.h"

#include <mdslib.h>
#include <regex.h>

#include <client/accAPI.h>
#include <client/udaClient.h>
#include <clientserver/copyStructs.h>
#include <clientserver/initStructs.h>
#include <clientserver/printStructs.h>
#include <clientserver/stringUtils.h>
#include <clientserver/udaTypes.h>
#include <logging/logging.h>
#include <server/makeServerRequestBlock.h>
#include <server/managePluginFiles.h>
#include <plugins/serverPlugin.h>

IDAMPLUGINFILELIST pluginFileList_mds;

extern char* errmsg;

char* spawnCommand(char* command, char* ipAddress);

#define MAXOBJECTCOUNT 100000
static void* localObjs[MAXOBJECTCOUNT];
static unsigned int lastObjectId = 0;
static unsigned int initLocalObjs = 1;

static int process_arguments(IDAM_PLUGIN_INTERFACE* idam_plugin_interface, PLUGIN_ARGS* plugin_args);
static int do_putIdsVersion(IDAM_PLUGIN_INTERFACE* idam_plugin_interface, PLUGIN_ARGS plugin_args);
static int do_delete(IDAM_PLUGIN_INTERFACE* idam_plugin_interface, PLUGIN_ARGS plugin_args);
static int do_get(IDAM_PLUGIN_INTERFACE* idam_plugin_interface, PLUGIN_ARGS plugin_args, int idx);
static int do_put(IDAM_PLUGIN_INTERFACE* idam_plugin_interface, PLUGIN_ARGS plugin_args, int idx);
static int do_open(IDAM_PLUGIN_INTERFACE* idam_plugin_interface, PLUGIN_ARGS plugin_args, int* idx);
static int do_create(IDAM_PLUGIN_INTERFACE* idam_plugin_interface, PLUGIN_ARGS plugin_args, int* idx);
static int do_close(IDAM_PLUGIN_INTERFACE* idam_plugin_interface, PLUGIN_ARGS plugin_args);
static int do_createModel(IDAM_PLUGIN_INTERFACE* idam_plugin_interface, PLUGIN_ARGS plugin_args);
static int do_setTimeBasePath(IDAM_PLUGIN_INTERFACE* idam_plugin_interface, PLUGIN_ARGS plugin_args);
static int do_releaseObject(IDAM_PLUGIN_INTERFACE* idam_plugin_interface, PLUGIN_ARGS plugin_args);
static int do_getObjectObject(IDAM_PLUGIN_INTERFACE* idam_plugin_interface, PLUGIN_ARGS plugin_args);
static int do_getObjectSlice(IDAM_PLUGIN_INTERFACE* idam_plugin_interface, PLUGIN_ARGS plugin_args);
static int do_getObjectGroup(IDAM_PLUGIN_INTERFACE* idam_plugin_interface, PLUGIN_ARGS plugin_args);
static int do_getObjectDim(IDAM_PLUGIN_INTERFACE* idam_plugin_interface, PLUGIN_ARGS plugin_args);
static int do_beginObject(IDAM_PLUGIN_INTERFACE* idam_plugin_interface, PLUGIN_ARGS plugin_args);
static int do_getObject(IDAM_PLUGIN_INTERFACE* idam_plugin_interface, PLUGIN_ARGS plugin_args);
static int do_putObject(IDAM_PLUGIN_INTERFACE* idam_plugin_interface, PLUGIN_ARGS plugin_args);
static int do_putObjectInObject(IDAM_PLUGIN_INTERFACE* idam_plugin_interface, PLUGIN_ARGS plugin_args);
static int do_putObjectGroup(IDAM_PLUGIN_INTERFACE* idam_plugin_interface, PLUGIN_ARGS plugin_args);
static int do_putObjectSlice(IDAM_PLUGIN_INTERFACE* idam_plugin_interface, PLUGIN_ARGS plugin_args);
static int do_source(IDAM_PLUGIN_INTERFACE* idam_plugin_interface, PLUGIN_ARGS plugin_args);
static int do_replaceLastObjectSlice(IDAM_PLUGIN_INTERFACE* idam_plugin_interface, PLUGIN_ARGS plugin_args);
static int do_cache(IDAM_PLUGIN_INTERFACE* idam_plugin_interface, PLUGIN_ARGS plugin_args);
static int do_getUniqueRun(IDAM_PLUGIN_INTERFACE* idam_plugin_interface, PLUGIN_ARGS plugin_args);
static int do_spawnCommand(IDAM_PLUGIN_INTERFACE* idam_plugin_interface, PLUGIN_ARGS plugin_args);
static int do_putIDS(IDAM_PLUGIN_INTERFACE* idam_plugin_interface, PLUGIN_ARGS plugin_args);
static int do_beginIdsPut(IDAM_PLUGIN_INTERFACE* idam_plugin_interface, PLUGIN_ARGS plugin_args);
static int do_endIdsPut(IDAM_PLUGIN_INTERFACE* idam_plugin_interface, PLUGIN_ARGS plugin_args);
static int do_beginIdsGet(IDAM_PLUGIN_INTERFACE* idam_plugin_interface, PLUGIN_ARGS plugin_args);
static int do_endIdsGet(IDAM_PLUGIN_INTERFACE* idam_plugin_interface, PLUGIN_ARGS plugin_args);
static int do_beginIdsGetSlice(IDAM_PLUGIN_INTERFACE* idam_plugin_interface, PLUGIN_ARGS plugin_args);
static int do_endIdsGetSlice(IDAM_PLUGIN_INTERFACE* idam_plugin_interface, PLUGIN_ARGS plugin_args);
static int do_beginIdsPutSlice(IDAM_PLUGIN_INTERFACE* idam_plugin_interface, PLUGIN_ARGS plugin_args);
static int do_endIdsPutSlice(IDAM_PLUGIN_INTERFACE* idam_plugin_interface, PLUGIN_ARGS plugin_args);
static int do_beginIdsPutNonTimed(IDAM_PLUGIN_INTERFACE* idam_plugin_interface, PLUGIN_ARGS plugin_args);
static int do_endIdsPutNonTimed(IDAM_PLUGIN_INTERFACE* idam_plugin_interface, PLUGIN_ARGS plugin_args);
static int do_beginIdsReplaceLastSlice(IDAM_PLUGIN_INTERFACE* idam_plugin_interface, PLUGIN_ARGS plugin_args);
static int do_endIdsReplaceLastSlice(IDAM_PLUGIN_INTERFACE* idam_plugin_interface, PLUGIN_ARGS plugin_args);
static int do_beginIdsPutTimed(IDAM_PLUGIN_INTERFACE* idam_plugin_interface, PLUGIN_ARGS plugin_args);
static int do_endIdsPutTimed(IDAM_PLUGIN_INTERFACE* idam_plugin_interface, PLUGIN_ARGS plugin_args);
static int do_help(IDAM_PLUGIN_INTERFACE* idam_plugin_interface, PLUGIN_ARGS plugin_args);
static int do_version(IDAM_PLUGIN_INTERFACE* idam_plugin_interface, PLUGIN_ARGS plugin_args);
static int do_builddate(IDAM_PLUGIN_INTERFACE* idam_plugin_interface, PLUGIN_ARGS plugin_args);
static int do_defaultmethod(IDAM_PLUGIN_INTERFACE* idam_plugin_interface, PLUGIN_ARGS plugin_args);
static int do_maxinterfaceversion(IDAM_PLUGIN_INTERFACE* idam_plugin_interface, PLUGIN_ARGS plugin_args);

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

static char TimeBasePath[TIMEBASEPATHLENGTH];

void putTimeBasePath(char* timeBasePath)
{
    if (strlen(timeBasePath) < TIMEBASEPATHLENGTH) {
        strcpy(TimeBasePath, timeBasePath);
    } else {
        TimeBasePath[0] = '\0';
    }
}

char* getTimeBasePath()
{
    return TimeBasePath;
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

    if (idam_plugin_interface->interfaceVersion > THISPLUGIN_MAX_INTERFACE_VERSION) {
        RAISE_PLUGIN_ERROR("Plugin Interface Version Unknown to this plugin: Unable to execute the request!");
    }

    idam_plugin_interface->pluginVersion = THISPLUGIN_VERSION;

    REQUEST_BLOCK* request_block = idam_plugin_interface->request_block;

    if (idam_plugin_interface->housekeeping || STR_IEQUALS(request_block->function, "reset")) {

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

    if (!init || STR_IEQUALS(request_block->function, "init") || STR_IEQUALS(request_block->function, "initialise")) {
        initIdamPluginFileList(&pluginFileList_mds);
        initLocalObj();
        putTimeBasePath("");

        init = 1;
        if (STR_IEQUALS(request_block->function, "init") || STR_IEQUALS(request_block->function, "initialise")) {
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

    imas_reset_errmsg();     // Clear previous error message

    if (STR_IEQUALS(request_block->function, "putIdsVersion")) {
        err = do_putIdsVersion(idam_plugin_interface, plugin_args);
    } else if (STR_IEQUALS(request_block->function, "source")) {
        err = do_source(idam_plugin_interface, plugin_args);
    } else if (STR_IEQUALS(request_block->function, "delete")) {
        err = do_delete(idam_plugin_interface, plugin_args);
    } else if (STR_IEQUALS(request_block->function, "get")) {
        err = do_get(idam_plugin_interface, plugin_args, idx);
    } else if (STR_IEQUALS(request_block->function, "put")) {
        err = do_put(idam_plugin_interface, plugin_args, idx);
    } else if (STR_IEQUALS(request_block->function, "open")) {
        err = do_open(idam_plugin_interface, plugin_args, &idx);
    } else if (STR_IEQUALS(request_block->function, "create")) {
        err = do_create(idam_plugin_interface, plugin_args, &idx);
    } else if (STR_IEQUALS(request_block->function, "close")) {
        err = do_close(idam_plugin_interface, plugin_args);
    } else if (STR_IEQUALS(request_block->function, "createModel")) {
        err = do_createModel(idam_plugin_interface, plugin_args);
    } else if (STR_IEQUALS(request_block->function, "setTimeBasePath")
               || STR_IEQUALS(request_block->function, "putTimeBasePath")) {
        err = do_setTimeBasePath(idam_plugin_interface, plugin_args);
    } else if (STR_IEQUALS(request_block->function, "releaseObject")) {
        err = do_releaseObject(idam_plugin_interface, plugin_args);
    } else if (STR_IEQUALS(request_block->function, "getObjectObject")) {
        err = do_getObjectObject(idam_plugin_interface, plugin_args);
    } else if (STR_IEQUALS(request_block->function, "getObjectSlice")) {
        err = do_getObjectSlice(idam_plugin_interface, plugin_args);
    } else if (STR_IEQUALS(request_block->function, "getObjectGroup")) {
        err = do_getObjectGroup(idam_plugin_interface, plugin_args);
    } else if (STR_IEQUALS(request_block->function, "getObjectDim")) {
        err = do_getObjectDim(idam_plugin_interface, plugin_args);
    } else if (STR_IEQUALS(request_block->function, "beginObject")) {
        err = do_beginObject(idam_plugin_interface, plugin_args);
    } else if (STR_IEQUALS(request_block->function, "getObject")) {
        err = do_getObject(idam_plugin_interface, plugin_args);
    } else if (STR_IEQUALS(request_block->function, "putObject")) {
        err = do_putObject(idam_plugin_interface, plugin_args);
    } else if (STR_IEQUALS(request_block->function, "putObjectInObject")) {
        err = do_putObjectInObject(idam_plugin_interface, plugin_args);
    } else if (STR_IEQUALS(request_block->function, "putObjectGroup")) {
        err = do_putObjectGroup(idam_plugin_interface, plugin_args);
    } else if (STR_IEQUALS(request_block->function, "putObjectSlice")) {
        err = do_putObjectSlice(idam_plugin_interface, plugin_args);
    } else if (STR_IEQUALS(request_block->function, "replaceLastObjectSlice")) {
        err = do_replaceLastObjectSlice(idam_plugin_interface, plugin_args);
    } else if (STR_IEQUALS(request_block->function, "cache")) {
        err = do_cache(idam_plugin_interface, plugin_args);
    } else if (STR_IEQUALS(request_block->function, "getUniqueRun")) {
        err = do_getUniqueRun(idam_plugin_interface, plugin_args);
    } else if (STR_IEQUALS(request_block->function, "spawnCommand")) {
        err = do_spawnCommand(idam_plugin_interface, plugin_args);
    } else if (STR_IEQUALS(request_block->function, "putIDS")) {
        err = do_putIDS(idam_plugin_interface, plugin_args);
    } else if (STR_IEQUALS(request_block->function, "beginIdsPut")) {
        err = do_beginIdsPut(idam_plugin_interface, plugin_args);
    } else if (STR_IEQUALS(request_block->function, "endIdsPut")) {
        err = do_endIdsPut(idam_plugin_interface, plugin_args);
    } else if (STR_IEQUALS(request_block->function, "beginIdsGet")) {
        err = do_beginIdsGet(idam_plugin_interface, plugin_args);
    } else if (STR_IEQUALS(request_block->function, "endIdsGet")) {
        err = do_endIdsGet(idam_plugin_interface, plugin_args);
    } else if (STR_IEQUALS(request_block->function, "beginIdsGetSlice")) {
        err = do_beginIdsGetSlice(idam_plugin_interface, plugin_args);
    } else if (STR_IEQUALS(request_block->function, "endIdsGetSlice")) {
        err = do_endIdsGetSlice(idam_plugin_interface, plugin_args);
    } else if (STR_IEQUALS(request_block->function, "beginIdsPutSlice")) {
        err = do_beginIdsPutSlice(idam_plugin_interface, plugin_args);
    } else if (STR_IEQUALS(request_block->function, "endIdsPutSlice")) {
        err = do_endIdsPutSlice(idam_plugin_interface, plugin_args);
    } else if (STR_IEQUALS(request_block->function, "beginIdsPutNonTimed")) {
        err = do_beginIdsPutNonTimed(idam_plugin_interface, plugin_args);
    } else if (STR_IEQUALS(request_block->function, "endIdsPutNonTimed")) {
        err = do_endIdsPutNonTimed(idam_plugin_interface, plugin_args);
    } else if (STR_IEQUALS(request_block->function, "beginIdsReplaceLastSlice")) {
        err = do_beginIdsReplaceLastSlice(idam_plugin_interface, plugin_args);
    } else if (STR_IEQUALS(request_block->function, "endIdsReplaceLastSlice")) {
        err = do_endIdsReplaceLastSlice(idam_plugin_interface, plugin_args);
    } else if (STR_IEQUALS(request_block->function, "beginIdsPutTimed")) {
        err = do_beginIdsPutTimed(idam_plugin_interface, plugin_args);
    } else if (STR_IEQUALS(request_block->function, "endIdsPutTimed")) {
        err = do_endIdsPutTimed(idam_plugin_interface, plugin_args);
    } else if (STR_IEQUALS(request_block->function, "help")) {
        err = do_help(idam_plugin_interface, plugin_args);
    } else if (STR_IEQUALS(request_block->function, "version")) {
        err = do_version(idam_plugin_interface, plugin_args);
    } else if (STR_IEQUALS(request_block->function, "builddate")) {
        err = do_builddate(idam_plugin_interface, plugin_args);
    } else if (STR_IEQUALS(request_block->function, "defaultmethod")) {
        err = do_defaultmethod(idam_plugin_interface, plugin_args);
    } else if (STR_IEQUALS(request_block->function, "maxinterfaceversion")) {
        err = do_maxinterfaceversion(idam_plugin_interface, plugin_args);
    } else {
        err = 999;
        addIdamError(CODEERRORTYPE, "imas", err, "Unknown function requested!");
    }

//--------------------------------------------------------------------------------------
// Housekeeping

    char* p;
    if (err != 0 && (p = imas_last_errmsg()) != 0) {
        TrimString(p);
        if (strlen(p) > 0) {
            err = 999;
            addIdamError(CODEERRORTYPE, "imas", err, p);
        }
        imas_reset_errmsg();
    }

    return err;
}

int imas_mds_putDataSlice(int idx, char* cpoPath, char* path, char* timeBasePath, int type, int nDims, int* dims,
                          void* data, double time)
{
    switch (nDims) {
        case 0: {
            switch (type) {
                case INT:
                    return mdsPutIntSlice(idx, cpoPath, path, timeBasePath, ((int*)data)[0], time);
                case FLOAT:
                    return mdsPutFloatSlice(idx, cpoPath, path, timeBasePath, ((float*)data)[0], time);
                case DOUBLE:
                    return mdsPutDoubleSlice(idx, cpoPath, path, timeBasePath, ((double*)data)[0], time);
                case STRING:
                    return mdsPutStringSlice(idx, cpoPath, path, timeBasePath, (char*)data, time);
            }
            break;
        }
        case 1: {
            switch (type) {
                case INT:
                    return mdsPutVect1DIntSlice(idx, cpoPath, path, timeBasePath, (int*)data, dims[0], time);
                case FLOAT:
                    return mdsPutVect1DFloatSlice(idx, cpoPath, path, timeBasePath, (float*)data, dims[0], time);
                case DOUBLE:
                    return mdsPutVect1DDoubleSlice(idx, cpoPath, path, timeBasePath, (double*)data, dims[0], time);
            }
            break;
        }
        case 2: {
            switch (type) {
                case INT:
                    return mdsPutVect2DIntSlice(idx, cpoPath, path, timeBasePath, (int*)data, dims[0], dims[1], time);
                case FLOAT:
                    return mdsPutVect2DFloatSlice(idx, cpoPath, path, timeBasePath, (float*)data, dims[0], dims[1],
                                                  time);
                case DOUBLE:
                    return mdsPutVect2DDoubleSlice(idx, cpoPath, path, timeBasePath, (double*)data, dims[0], dims[1],
                                                   time);
            }
            break;
        }
        case 3: {
            switch (type) {
                case INT:
                    return mdsPutVect3DIntSlice(idx, cpoPath, path, timeBasePath, (int*)data, dims[0], dims[1],
                                                dims[2], time);
                case FLOAT:
                    return mdsPutVect3DFloatSlice(idx, cpoPath, path, timeBasePath, (float*)data, dims[0], dims[1],
                                                  dims[2], time);
                case DOUBLE:
                    return mdsPutVect3DDoubleSlice(idx, cpoPath, path, timeBasePath, (double*)data, dims[0], dims[1],
                                                   dims[2], time);
            }
            break;
        }
        case 4: {
            switch (type) {
                case INT:
                    return mdsPutVect4DIntSlice(idx, cpoPath, path, timeBasePath, (int*)data, dims[0], dims[1],
                                                dims[2], dims[3], time);
                case FLOAT:
                    return mdsPutVect4DFloatSlice(idx, cpoPath, path, timeBasePath, (float*)data, dims[0], dims[1],
                                                  dims[2], dims[3], time);
                case DOUBLE:
                    return mdsPutVect4DDoubleSlice(idx, cpoPath, path, timeBasePath, (double*)data, dims[0], dims[1],
                                                   dims[2], dims[3], time);
            }
            break;
        }
        case 5: {
            switch (type) {
                case INT:
                    return mdsPutVect5DIntSlice(idx, cpoPath, path, timeBasePath, (int*)data, dims[0], dims[1],
                                                dims[2], dims[3], dims[4], time);
                case FLOAT:
                    return mdsPutVect5DFloatSlice(idx, cpoPath, path, timeBasePath, (float*)data, dims[0], dims[1],
                                                  dims[2], dims[3], dims[4], time);
                case DOUBLE:
                    return mdsPutVect5DDoubleSlice(idx, cpoPath, path, timeBasePath, (double*)data, dims[0], dims[1],
                                                   dims[2], dims[3], dims[4], time);
            }
            break;
        }
        case 6: {
            switch (type) {
                case INT:
                    return mdsPutVect6DIntSlice(idx, cpoPath, path, timeBasePath, (int*)data, dims[0], dims[1],
                                                dims[2], dims[3], dims[4], dims[5], time);
                case FLOAT:
                    return mdsPutVect6DFloatSlice(idx, cpoPath, path, timeBasePath, (float*)data, dims[0], dims[1],
                                                  dims[2], dims[3], dims[4], dims[5], time);
                case DOUBLE:
                    return mdsPutVect6DDoubleSlice(idx, cpoPath, path, timeBasePath, (double*)data, dims[0], dims[1],
                                                   dims[2], dims[3], dims[4], dims[5], time);
            }
            break;
        }
    }
    return 0;
}

int imas_mds_replaceLastDataSlice(int idx, char* cpoPath, char* path, int type, int nDims, int* dims, void* data)
{

    switch (nDims) {
        case 0: {
            switch (type) {
                case INT:
                    return mdsReplaceLastIntSlice(idx, cpoPath, path, ((int*)data)[0]);
                case FLOAT:
                    return mdsReplaceLastFloatSlice(idx, cpoPath, path, ((float*)data)[0]);
                case DOUBLE:
                    return mdsReplaceLastDoubleSlice(idx, cpoPath, path, ((double*)data)[0]);
                case STRING:
                    return mdsReplaceLastStringSlice(idx, cpoPath, path, (char*)data);
            }
            break;
        }
        case 1: {
            switch (type) {
                case INT:
                    return mdsReplaceLastVect1DIntSlice(idx, cpoPath, path, (int*)data, dims[0]);
                case FLOAT:
                    return mdsReplaceLastVect1DFloatSlice(idx, cpoPath, path, (float*)data, dims[0]);
                case DOUBLE:
                    return mdsReplaceLastVect1DDoubleSlice(idx, cpoPath, path, (double*)data, dims[0]);
            }
            break;
        }
        case 2: {
            switch (type) {
                case INT:
                    return mdsReplaceLastVect2DIntSlice(idx, cpoPath, path, (int*)data, dims[0], dims[1]);
                case FLOAT:
                    return mdsReplaceLastVect2DFloatSlice(idx, cpoPath, path, (float*)data, dims[0], dims[1]);
                case DOUBLE:
                    return mdsReplaceLastVect2DDoubleSlice(idx, cpoPath, path, (double*)data, dims[0], dims[1]);
            }
            break;
        }
        case 3: {
            switch (type) {
                case INT:
                    return mdsReplaceLastVect3DIntSlice(idx, cpoPath, path, (int*)data, dims[0], dims[1], dims[2]);
                case FLOAT:
                    return mdsReplaceLastVect3DFloatSlice(idx, cpoPath, path, (float*)data, dims[0], dims[1],
                                                          dims[2]);
                case DOUBLE:
                    return mdsReplaceLastVect3DDoubleSlice(idx, cpoPath, path, (double*)data, dims[0], dims[1],
                                                           dims[2]);
            }
            break;
        }
        case 4: {
            switch (type) {
                case INT:
                    return mdsReplaceLastVect4DIntSlice(idx, cpoPath, path, (int*)data, dims[0], dims[1], dims[2],
                                                        dims[3]);
                case FLOAT:
                    return mdsReplaceLastVect4DFloatSlice(idx, cpoPath, path, (float*)data, dims[0], dims[1], dims[2],
                                                          dims[3]);
                case DOUBLE:
                    return mdsReplaceLastVect4DDoubleSlice(idx, cpoPath, path, (double*)data, dims[0], dims[1],
                                                           dims[2], dims[3]);
            }
            break;
        }
        case 5: {
            switch (type) {
                case INT:
                    return mdsReplaceLastVect5DIntSlice(idx, cpoPath, path, (int*)data, dims[0], dims[1], dims[2],
                                                        dims[3], dims[4]);
                case FLOAT:
                    return mdsReplaceLastVect5DFloatSlice(idx, cpoPath, path, (float*)data, dims[0], dims[1], dims[2],
                                                          dims[3], dims[4]);
                case DOUBLE:
                    return mdsReplaceLastVect5DDoubleSlice(idx, cpoPath, path, (double*)data, dims[0], dims[1],
                                                           dims[2], dims[3], dims[4]);
            }
            break;
        }
        case 6: {
            switch (type) {
                case INT:
                    return mdsReplaceLastVect6DIntSlice(idx, cpoPath, path, (int*)data, dims[0], dims[1], dims[2],
                                                        dims[3], dims[4], dims[5]);
                case FLOAT:
                    return mdsReplaceLastVect6DFloatSlice(idx, cpoPath, path, (float*)data, dims[0], dims[1], dims[2],
                                                          dims[3], dims[4], dims[5]);
                case DOUBLE:
                    return mdsReplaceLastVect6DDoubleSlice(idx, cpoPath, path, (double*)data, dims[0], dims[1],
                                                           dims[2], dims[3], dims[4], dims[5]);
            }
            break;
        }
    }
    return 0;
}


int imas_mds_putData(int idx, char* cpoPath, char* path, int type, int nDims, int* dims, int isTimed, void* data,
                     double time)
{

//TODO *** timeBasePath, time
//TODO putTimes, nPutTimes must be global - nPutTimes == dims[0], used when isTimed is != 0

    if (isTimed == PUTSLICE_OPERATION) {
        return imas_mds_putDataSlice(idx, cpoPath, path, getTimeBasePath(), type, nDims, dims, data, time);
    } else if (isTimed == REPLACELASTSLICE_OPERATION) {
        return imas_mds_replaceLastDataSlice(idx, cpoPath, path, type, nDims, dims, data);
    }

    char* timeBasePath = getTimeBasePath();

    switch (nDims) {
        case 0: {
            switch (type) {
                case INT:
                    return mdsPutInt(idx, cpoPath, path, ((int*)data)[0]);
                case FLOAT:
                    return mdsPutFloat(idx, cpoPath, path, ((float*)data)[0]);
                case DOUBLE:
                    return mdsPutDouble(idx, cpoPath, path, ((double*)data)[0]);
                case STRING:
                    return mdsPutString(idx, cpoPath, path, (char*)data);
            }
            break;
        }
        case 1: {
            switch (type) {
                case INT:
                    return mdsPutVect1DInt(idx, cpoPath, path, timeBasePath, (int*)data, dims[0], isTimed);
                case FLOAT:
                    return mdsPutVect1DFloat(idx, cpoPath, path, timeBasePath, (float*)data, dims[0], isTimed);
                case DOUBLE:
                    return mdsPutVect1DDouble(idx, cpoPath, path, timeBasePath, (double*)data, dims[0], isTimed);
                case STRING:
                    return mdsPutVect1DString(idx, cpoPath, path, timeBasePath, (char**)data, dims[0], isTimed);
            }
            break;
        }
        case 2: {
            switch (type) {
                case INT:
                    return mdsPutVect2DInt(idx, cpoPath, path, timeBasePath, (int*)data, dims[0], dims[1], isTimed);
                case FLOAT:
                    return mdsPutVect2DFloat(idx, cpoPath, path, timeBasePath, (float*)data, dims[0], dims[1],
                                             isTimed);
                case DOUBLE:
                    return mdsPutVect2DDouble(idx, cpoPath, path, timeBasePath, (double*)data, dims[0], dims[1],
                                              isTimed);
            }
            break;
        }
        case 3: {
            switch (type) {
                case INT:
                    return mdsPutVect3DInt(idx, cpoPath, path, timeBasePath, (int*)data, dims[0], dims[1], dims[2],
                                           isTimed);
                case FLOAT:
                    return mdsPutVect3DFloat(idx, cpoPath, path, timeBasePath, (float*)data, dims[0], dims[1],
                                             dims[2], isTimed);
                case DOUBLE:
                    return mdsPutVect3DDouble(idx, cpoPath, path, timeBasePath, (double*)data, dims[0], dims[1],
                                              dims[2], isTimed);
            }
            break;
        }
        case 4: {
            switch (type) {
                case INT:
                    return mdsPutVect4DInt(idx, cpoPath, path, timeBasePath, (int*)data, dims[0], dims[1], dims[2],
                                           dims[3], isTimed);
                case FLOAT:
                    return mdsPutVect4DFloat(idx, cpoPath, path, timeBasePath, (float*)data, dims[0], dims[1],
                                             dims[2], dims[3], isTimed);
                case DOUBLE:
                    return mdsPutVect4DDouble(idx, cpoPath, path, timeBasePath, (double*)data, dims[0], dims[1],
                                              dims[2], dims[3], isTimed);
            }
            break;
        }
        case 5: {
            switch (type) {
                case INT:
                    return mdsPutVect5DInt(idx, cpoPath, path, timeBasePath, (int*)data, dims[0], dims[1], dims[2],
                                           dims[3], dims[4], isTimed);
                case FLOAT:
                    return mdsPutVect5DFloat(idx, cpoPath, path, timeBasePath, (float*)data, dims[0], dims[1],
                                             dims[2], dims[3], dims[4], isTimed);
                case DOUBLE:
                    return mdsPutVect5DDouble(idx, cpoPath, path, timeBasePath, (double*)data, dims[0], dims[1],
                                              dims[2], dims[3], dims[4], isTimed);
            }
            break;
        }
        case 6: {
            switch (type) {
                case INT:
                    return mdsPutVect6DInt(idx, cpoPath, path, timeBasePath, (int*)data, dims[0], dims[1], dims[2],
                                           dims[3], dims[4], dims[5], isTimed);
                case FLOAT:
                    return mdsPutVect6DFloat(idx, cpoPath, path, timeBasePath, (float*)data, dims[0], dims[1],
                                             dims[2], dims[3], dims[4], dims[5], isTimed);
                case DOUBLE:
                    return mdsPutVect6DDouble(idx, cpoPath, path, timeBasePath, (double*)data, dims[0], dims[1],
                                              dims[2], dims[3], dims[4], dims[5], isTimed);
            }
            break;
        }
        case 7: {
            switch (type) {
                case INT:
                    return mdsPutVect7DInt(idx, cpoPath, path, timeBasePath, (int*)data, dims[0], dims[1], dims[2],
                                           dims[3], dims[4], dims[5], dims[6], isTimed);
                case FLOAT:
                    return mdsPutVect7DFloat(idx, cpoPath, path, timeBasePath, (float*)data, dims[0], dims[1],
                                             dims[2], dims[3], dims[4], dims[5], dims[6], isTimed);
                case DOUBLE:
                    return mdsPutVect7DDouble(idx, cpoPath, path, timeBasePath, (double*)data, dims[0], dims[1],
                                              dims[2], dims[3], dims[4], dims[5], dims[6], isTimed);
            }
            break;
        }
    }
    return 0;
}

int imas_mds_putDataX(int idx, char* cpoPath, char* path, int type, int nDims, int* dims, int dataOperation,
                      void* data, double time)
{
    if (dataOperation == PUTSLICE_OPERATION) {
        return imas_mds_putDataSlice(idx, cpoPath, path, getTimeBasePath(), type, nDims, dims, data, time);
    } else if (dataOperation == REPLACELASTSLICE_OPERATION) {
        return imas_mds_replaceLastDataSlice(idx, cpoPath, path, type, nDims, dims, data);
    }

    return -1;
}

int imas_mds_getData(int idx, char* cpoPath, char* path, int type, int nDims, int* dims, void** dataOut)
{

    switch (nDims) {
        case 0: {
            switch (type) {
                case INT: {
                    int* data = (int*)malloc(sizeof(int));
                    data[0] = 0;
                    *dataOut = data;
                    return mdsGetInt(idx, cpoPath, path, data);
                }
                case FLOAT: {
                    float* data = (float*)malloc(sizeof(float));
                    data[0] = 0.0;
                    *dataOut = data;
                    return mdsGetFloat(idx, cpoPath, path, data);
                }
                case DOUBLE: {
                    double* data = (double*)malloc(sizeof(double));
                    data[0] = 0.0;
                    *dataOut = data;
                    return mdsGetDouble(idx, cpoPath, path, data);
                }
                case STRING:
                    return mdsGetString(idx, cpoPath, path, (char**)dataOut);
            }
            break;
        }
        case 1: {
            switch (type) {
                case INT:
                    return mdsGetVect1DInt(idx, cpoPath, path, (int**)dataOut, &dims[0]);
                case FLOAT:
                    return mdsGetVect1DFloat(idx, cpoPath, path, (float**)dataOut, &dims[0]);
                case DOUBLE:
                    return mdsGetVect1DDouble(idx, cpoPath, path, (double**)dataOut, &dims[0]);
                case STRING:
                    return mdsGetVect1DString(idx, cpoPath, path, (char***)dataOut, &dims[0]);
            }
            break;
        }
        case 2: {
            switch (type) {
                case INT:
                    return mdsGetVect2DInt(idx, cpoPath, path, (int**)dataOut, &dims[0], &dims[1]);
                case FLOAT:
                    return mdsGetVect2DFloat(idx, cpoPath, path, (float**)dataOut, &dims[0], &dims[1]);
                case DOUBLE:
                    return mdsGetVect2DDouble(idx, cpoPath, path, (double**)dataOut, &dims[0], &dims[1]);
            }
            break;
        }
        case 3: {
            switch (type) {
                case INT:
                    return mdsGetVect3DInt(idx, cpoPath, path, (int**)dataOut, &dims[0], &dims[1], &dims[2]);
                case FLOAT:
                    return mdsGetVect3DFloat(idx, cpoPath, path, (float**)dataOut, &dims[0], &dims[1], &dims[2]);
                case DOUBLE:
                    return mdsGetVect3DDouble(idx, cpoPath, path, (double**)dataOut, &dims[0], &dims[1], &dims[2]);
            }
            break;
        }
        case 4: {
            switch (type) {
                case INT:
                    return mdsGetVect4DInt(idx, cpoPath, path, (int**)dataOut, &dims[0], &dims[1], &dims[2],
                                           &dims[3]);
                case FLOAT:
                    return mdsGetVect4DFloat(idx, cpoPath, path, (float**)dataOut, &dims[0], &dims[1], &dims[2],
                                             &dims[3]);
                case DOUBLE:
                    return mdsGetVect4DDouble(idx, cpoPath, path, (double**)dataOut, &dims[0], &dims[1], &dims[2],
                                              &dims[3]);
            }
            break;
        }
        case 5: {
            switch (type) {
                case INT:
                    return mdsGetVect5DInt(idx, cpoPath, path, (int**)dataOut, &dims[0], &dims[1], &dims[2], &dims[3],
                                           &dims[4]);
                case FLOAT:
                    return mdsGetVect5DFloat(idx, cpoPath, path, (float**)dataOut, &dims[0], &dims[1], &dims[2],
                                             &dims[3], &dims[4]);
                case DOUBLE:
                    return mdsGetVect5DDouble(idx, cpoPath, path, (double**)dataOut, &dims[0], &dims[1], &dims[2],
                                              &dims[3], &dims[4]);
            }
            break;
        }
        case 6: {
            switch (type) {
                case INT:
                    return mdsGetVect6DInt(idx, cpoPath, path, (int**)dataOut, &dims[0], &dims[1], &dims[2], &dims[3],
                                           &dims[4], &dims[5]);
                case FLOAT:
                    return mdsGetVect6DFloat(idx, cpoPath, path, (float**)dataOut, &dims[0], &dims[1], &dims[2],
                                             &dims[3], &dims[4], &dims[5]);
                case DOUBLE:
                    return mdsGetVect6DDouble(idx, cpoPath, path, (double**)dataOut, &dims[0], &dims[1], &dims[2],
                                              &dims[3], &dims[4], &dims[5]);
            }
            break;
        }
        case 7: {
            switch (type) {
                case INT:
                    return mdsGetVect7DInt(idx, cpoPath, path, (int**)dataOut, &dims[0], &dims[1], &dims[2], &dims[3],
                                           &dims[4], &dims[5], &dims[6]);
                case FLOAT:
                    return mdsGetVect7DFloat(idx, cpoPath, path, (float**)dataOut, &dims[0], &dims[1], &dims[2],
                                             &dims[3], &dims[4], &dims[5], &dims[6]);
                case DOUBLE:
                    return mdsGetVect7DDouble(idx, cpoPath, path, (double**)dataOut, &dims[0], &dims[1], &dims[2],
                                              &dims[3], &dims[4], &dims[5], &dims[6]);
            }
            break;
        }
    }
    return 0;
}

int imas_mds_getDataSlices(int idx, char* cpoPath, char* path, int type, int rank, int* shape, void** data,
                           double time, double* retTime, int interpolMode)
{
    char* timeBasePath = getTimeBasePath();
    switch (rank) {
        case 0: {
            switch (type) {
                case INT: {
                    int* dataSlice = (int*)malloc(sizeof(int));
                    dataSlice[0] = 0;
                    *data = dataSlice;
                    return mdsGetIntSlice(idx, cpoPath, path, timeBasePath, dataSlice, time, retTime, interpolMode);
                }
                case FLOAT: {
                    float* dataSlice = (float*)malloc(sizeof(float));
                    dataSlice[0] = 0.0;
                    *data = dataSlice;
                    return mdsGetFloatSlice(idx, cpoPath, path, timeBasePath, dataSlice, time, retTime, interpolMode);
                }
                case DOUBLE: {
                    double* dataSlice = (double*)malloc(sizeof(double));
                    dataSlice[0] = 0;
                    *data = dataSlice;
                    return mdsGetDoubleSlice(idx, cpoPath, path, timeBasePath, dataSlice, time, retTime, interpolMode);
                }
                case STRING: {
                    return mdsGetStringSlice(idx, cpoPath, path, timeBasePath, (char**)data, time, retTime,
                                             interpolMode);
                }
            }
            break;
        }
        case 1: {
            switch (type) {
                case INT:
                    return mdsGetVect1DIntSlice(idx, cpoPath, path, timeBasePath, (int**)data, &shape[0], time,
                                                retTime, interpolMode);
                case FLOAT:
                    return mdsGetVect1DFloatSlice(idx, cpoPath, path, timeBasePath, (float**)data, &shape[0], time,
                                                  retTime, interpolMode);
                case DOUBLE:
                    return mdsGetVect1DDoubleSlice(idx, cpoPath, path, timeBasePath, (double**)data, &shape[0], time,
                                                   retTime, interpolMode);
            }
            break;
        }
        case 2: {
            switch (type) {
                case INT:
                    return mdsGetVect2DIntSlice(idx, cpoPath, path, timeBasePath, (int**)data, &shape[0], &shape[1],
                                                time, retTime, interpolMode);
                case FLOAT:
                    return mdsGetVect2DFloatSlice(idx, cpoPath, path, timeBasePath, (float**)data, &shape[0],
                                                  &shape[1], time, retTime, interpolMode);
                case DOUBLE:
                    return mdsGetVect2DDoubleSlice(idx, cpoPath, path, timeBasePath, (double**)data, &shape[0],
                                                   &shape[1], time, retTime, interpolMode);
            }
            break;
        }
        case 3: {
            switch (type) {
                case INT:
                    return mdsGetVect3DIntSlice(idx, cpoPath, path, timeBasePath, (int**)data, &shape[0], &shape[1],
                                                &shape[2], time, retTime, interpolMode);
                case FLOAT:
                    return mdsGetVect3DFloatSlice(idx, cpoPath, path, timeBasePath, (float**)data, &shape[0],
                                                  &shape[1], &shape[2], time, retTime, interpolMode);
                case DOUBLE:
                    return mdsGetVect3DDoubleSlice(idx, cpoPath, path, timeBasePath, (double**)data, &shape[0],
                                                   &shape[1], &shape[2], time, retTime, interpolMode);
            }
            break;
        }
        case 4: {
            switch (type) {
                case INT:
                    return mdsGetVect4DIntSlice(idx, cpoPath, path, timeBasePath, (int**)data, &shape[0], &shape[1],
                                                &shape[2], &shape[3], time, retTime, interpolMode);
                case FLOAT:
                    return mdsGetVect4DFloatSlice(idx, cpoPath, path, timeBasePath, (float**)data, &shape[0],
                                                  &shape[1], &shape[2], &shape[3], time, retTime, interpolMode);
                case DOUBLE:
                    return mdsGetVect4DDoubleSlice(idx, cpoPath, path, timeBasePath, (double**)data, &shape[0],
                                                   &shape[1], &shape[2], &shape[3], time, retTime, interpolMode);
            }
            break;
        }
        case 5: {
            switch (type) {
                case INT:
                    return mdsGetVect5DIntSlice(idx, cpoPath, path, timeBasePath, (int**)data, &shape[0], &shape[1],
                                                &shape[2], &shape[3], &shape[4], time, retTime, interpolMode);
                case FLOAT:
                    return mdsGetVect5DFloatSlice(idx, cpoPath, path, timeBasePath, (float**)data, &shape[0],
                                                  &shape[1], &shape[2], &shape[3], &shape[4], time, retTime,
                                                  interpolMode);
                case DOUBLE:
                    return mdsGetVect5DDoubleSlice(idx, cpoPath, path, timeBasePath, (double**)data, &shape[0],
                                                   &shape[1], &shape[2], &shape[3], &shape[4], time, retTime,
                                                   interpolMode);
            }
            break;
        }
        case 6: {
            switch (type) {
                case INT:
                    return mdsGetVect6DIntSlice(idx, cpoPath, path, timeBasePath, (int**)data, &shape[0], &shape[1],
                                                &shape[2], &shape[3], &shape[4], &shape[5], time, retTime,
                                                interpolMode);
                case FLOAT:
                    return mdsGetVect6DFloatSlice(idx, cpoPath, path, timeBasePath, (float**)data, &shape[0],
                                                  &shape[1], &shape[2], &shape[3], &shape[4], &shape[5], time, retTime,
                                                  interpolMode);
                case DOUBLE:
                    return mdsGetVect6DDoubleSlice(idx, cpoPath, path, timeBasePath, (double**)data, &shape[0],
                                                   &shape[1], &shape[2], &shape[3], &shape[4], &shape[5], time, retTime,
                                                   interpolMode);
            }
            break;
        }
    }

    return 0;
}

//===========================================================================================================================================
// Objects

// The obj is a True Object

void* imas_mds_putDataSliceInObject(void* obj, char* path, int index, int type, int nDims, int* dims, void* data)
{

    switch (nDims) {
        case 0: {
            switch (type) {
                case INT:
                    return mdsPutIntInObject(obj, path, index, ((int*)data)[0]);
                case FLOAT:
                    return mdsPutFloatInObject(obj, path, index, ((float*)data)[0]);
                case DOUBLE:
                    return mdsPutDoubleInObject(obj, path, index, ((double*)data)[0]);
                case STRING:
                    return mdsPutStringInObject(obj, path, index, (char*)data);
            }
            break;
        }
        case 1: {
            switch (type) {
                case INT:
                    return mdsPutVect1DIntInObject(obj, path, index, (int*)data, dims[0]);
                case FLOAT:
                    return mdsPutVect1DFloatInObject(obj, path, index, (float*)data, dims[0]);
                case DOUBLE:
                    return mdsPutVect1DDoubleInObject(obj, path, index, (double*)data, dims[0]);
                case STRING:
                    return mdsPutVect1DStringInObject(obj, path, index, (char**)data, dims[0]);
            }
            break;
        }
        case 2: {
            switch (type) {
                case INT:
                    return mdsPutVect2DIntInObject(obj, path, index, (int*)data, dims[0], dims[1]);
                case FLOAT:
                    return mdsPutVect2DFloatInObject(obj, path, index, (float*)data, dims[0], dims[1]);
                case DOUBLE:
                    return mdsPutVect2DDoubleInObject(obj, path, index, (double*)data, dims[0], dims[1]);
            }
            break;
        }
        case 3: {
            switch (type) {
                case INT:
                    return mdsPutVect3DIntInObject(obj, path, index, (int*)data, dims[0], dims[1], dims[2]);
                case FLOAT:
                    return mdsPutVect3DFloatInObject(obj, path, index, (float*)data, dims[0], dims[1], dims[2]);
                case DOUBLE:
                    return mdsPutVect3DDoubleInObject(obj, path, index, (double*)data, dims[0], dims[1], dims[2]);
            }
            break;
        }
        case 4: {
            switch (type) {
                case INT:
                    return mdsPutVect4DIntInObject(obj, path, index, (int*)data, dims[0], dims[1], dims[2], dims[3]);
                case FLOAT:
                    return mdsPutVect4DFloatInObject(obj, path, index, (float*)data, dims[0], dims[1], dims[2],
                                                     dims[3]);
                case DOUBLE:
                    return mdsPutVect4DDoubleInObject(obj, path, index, (double*)data, dims[0], dims[1], dims[2],
                                                      dims[3]);
            }
            break;
        }
        case 5: {
            switch (type) {
                case INT:
                    return mdsPutVect5DIntInObject(obj, path, index, (int*)data, dims[0], dims[1], dims[2], dims[3],
                                                   dims[4]);
                case FLOAT:
                    return mdsPutVect5DFloatInObject(obj, path, index, (float*)data, dims[0], dims[1], dims[2],
                                                     dims[3], dims[4]);
                case DOUBLE:
                    return mdsPutVect5DDoubleInObject(obj, path, index, (double*)data, dims[0], dims[1], dims[2],
                                                      dims[3], dims[4]);
            }
            break;
        }
        case 6: {
            switch (type) {
                case INT:
                    return mdsPutVect6DIntInObject(obj, path, index, (int*)data, dims[0], dims[1], dims[2], dims[3],
                                                   dims[4], dims[5]);
                case FLOAT:
                    return mdsPutVect6DFloatInObject(obj, path, index, (float*)data, dims[0], dims[1], dims[2],
                                                     dims[3], dims[4], dims[5]);
                case DOUBLE:
                    return mdsPutVect6DDoubleInObject(obj, path, index, (double*)data, dims[0], dims[1], dims[2],
                                                      dims[3], dims[4], dims[5]);
            }
            break;
        }
        case 7: {
            switch (type) {
                case INT:
                    return mdsPutVect7DIntInObject(obj, path, index, (int*)data, dims[0], dims[1], dims[2], dims[3],
                                                   dims[4], dims[5], dims[6]);
                case FLOAT:
                    return mdsPutVect7DFloatInObject(obj, path, index, (float*)data, dims[0], dims[1], dims[2],
                                                     dims[3], dims[4], dims[5], dims[6]);
                case DOUBLE:
                    return mdsPutVect7DDoubleInObject(obj, path, index, (double*)data, dims[0], dims[1], dims[2],
                                                      dims[3], dims[4], dims[5], dims[6]);
            }
            break;
        }
    }

    return 0;
}

int imas_mds_getDataSliceInObject(void* obj, char* path, int index, int type, int nDims, int* dims, void** data)
{

    switch (nDims) {
        case 0: {
            switch (type) {
                case INT: {
                    int* dataSlice = (int*)malloc(sizeof(int));
                    dataSlice[0] = 0;
                    *data = dataSlice;
                    return mdsGetIntFromObject(obj, path, index, dataSlice);
                }
                case FLOAT: {
                    float* dataSlice = (float*)malloc(sizeof(float));
                    dataSlice[0] = 0.0;
                    *data = dataSlice;
                    return mdsGetFloatFromObject(obj, path, index, dataSlice);
                }
                case DOUBLE: {
                    double* dataSlice = (double*)malloc(sizeof(double));
                    dataSlice[0] = 0;
                    *data = dataSlice;
                    return mdsGetDoubleFromObject(obj, path, index, dataSlice);
                }
                case STRING:
                    return mdsGetStringFromObject(obj, path, index, (char**)data);
            }
            break;
        }
        case 1: {
            switch (type) {
                case INT:
                    return mdsGetVect1DIntFromObject(obj, path, index, (int**)data, &dims[0]);
                case FLOAT:
                    return mdsGetVect1DFloatFromObject(obj, path, index, (float**)data, &dims[0]);
                case DOUBLE:
                    return mdsGetVect1DDoubleFromObject(obj, path, index, (double**)data, &dims[0]);
                case STRING:
                    return mdsGetVect1DStringFromObject(obj, path, index, (char***)data, &dims[0]);
            }
            break;
        }
        case 2: {
            switch (type) {
                case INT:
                    return mdsGetVect2DIntFromObject(obj, path, index, (int**)data, &dims[0], &dims[1]);
                case FLOAT:
                    return mdsGetVect2DFloatFromObject(obj, path, index, (float**)data, &dims[0], &dims[1]);
                case DOUBLE:
                    return mdsGetVect2DDoubleFromObject(obj, path, index, (double**)data, &dims[0], &dims[1]);
            }
            break;
        }
        case 3: {
            switch (type) {
                case INT:
                    return mdsGetVect3DIntFromObject(obj, path, index, (int**)data, &dims[0], &dims[1], &dims[2]);
                case FLOAT:
                    return mdsGetVect3DFloatFromObject(obj, path, index, (float**)data, &dims[0], &dims[1], &dims[2]);
                case DOUBLE:
                    return mdsGetVect3DDoubleFromObject(obj, path, index, (double**)data, &dims[0], &dims[1],
                                                        &dims[2]);
            }
            break;
        }
        case 4: {
            switch (type) {
                case INT:
                    return mdsGetVect4DIntFromObject(obj, path, index, (int**)data, &dims[0], &dims[1], &dims[2],
                                                     &dims[3]);
                case FLOAT:
                    return mdsGetVect4DFloatFromObject(obj, path, index, (float**)data, &dims[0], &dims[1], &dims[2],
                                                       &dims[3]);
                case DOUBLE:
                    return mdsGetVect4DDoubleFromObject(obj, path, index, (double**)data, &dims[0], &dims[1],
                                                        &dims[2], &dims[3]);
            }
            break;
        }
        case 5: {
            switch (type) {
                case INT:
                    return mdsGetVect5DIntFromObject(obj, path, index, (int**)data, &dims[0], &dims[1], &dims[2],
                                                     &dims[3], &dims[4]);
                case FLOAT:
                    return mdsGetVect5DFloatFromObject(obj, path, index, (float**)data, &dims[0], &dims[1], &dims[2],
                                                       &dims[3], &dims[4]);
                case DOUBLE:
                    return mdsGetVect5DDoubleFromObject(obj, path, index, (double**)data, &dims[0], &dims[1],
                                                        &dims[2], &dims[3], &dims[4]);
            }
            break;
        }
        case 6: {
            switch (type) {
                case INT:
                    return mdsGetVect6DIntFromObject(obj, path, index, (int**)data, &dims[0], &dims[1], &dims[2],
                                                     &dims[3], &dims[4], &dims[5]);
                case FLOAT:
                    return mdsGetVect6DFloatFromObject(obj, path, index, (float**)data, &dims[0], &dims[1], &dims[2],
                                                       &dims[3], &dims[4], &dims[5]);
                case DOUBLE:
                    return mdsGetVect6DDoubleFromObject(obj, path, index, (double**)data, &dims[0], &dims[1],
                                                        &dims[2], &dims[3], &dims[4], &dims[5]);
            }
            break;
        }
        case 7: {
            switch (type) {
                case INT:
                    return mdsGetVect7DIntFromObject(obj, path, index, (int**)data, &dims[0], &dims[1], &dims[2],
                                                     &dims[3], &dims[4], &dims[5], &dims[6]);
                case FLOAT:
                    return mdsGetVect7DFloatFromObject(obj, path, index, (float**)data, &dims[0], &dims[1], &dims[2],
                                                       &dims[3], &dims[4], &dims[5], &dims[6]);
                case DOUBLE:
                    return mdsGetVect7DDoubleFromObject(obj, path, index, (double**)data, &dims[0], &dims[1],
                                                        &dims[2], &dims[3], &dims[4], &dims[5], &dims[6]);
            }
            break;
        }
    }

    return 0;
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
    plugin_args->isClientObjectId = findIntValue(&request_block->nameValueList, &plugin_args->clientObjectId,
                                                 "clientObjectId|ObjectId");
    plugin_args->isRank = findIntValue(&request_block->nameValueList, &plugin_args->rank, "rank|ndims");
    plugin_args->isIndex = findIntValue(&request_block->nameValueList, &plugin_args->index, "index");
    plugin_args->isCount = findIntValue(&request_block->nameValueList, &plugin_args->count, "count");
    plugin_args->isShapeString = findStringValue(&request_block->nameValueList, &plugin_args->shapeString,
                                                 "shape|dims");
    plugin_args->isDataString = findStringValue(&request_block->nameValueList, &plugin_args->dataString, "data");
    plugin_args->quote = findValue(&request_block->nameValueList, "singlequote") ? (char)'\'' : plugin_args->quote;
    plugin_args->quote = findValue(&request_block->nameValueList, "doublequote") ? (char)'\"' : plugin_args->quote;
    plugin_args->delimiter = findValue(&request_block->nameValueList, "delimiter") ? (char)'\'' : plugin_args->delimiter;
    plugin_args->isFileName = findStringValue(&request_block->nameValueList, &plugin_args->filename,
                                              "filename|file|name");
    plugin_args->isShotNumber = findIntValue(&request_block->nameValueList, &plugin_args->shotNumber,
                                             "shotNumber|shot|pulse|exp_number");
    plugin_args->isRunNumber = findIntValue(&request_block->nameValueList, &plugin_args->runNumber,
                                            "runNumber|run|pass|sequence");
    plugin_args->isRefShotNumber = findIntValue(&request_block->nameValueList, &plugin_args->refShotNumber,
                                                "refShotNumber|refShot");
    plugin_args->isRefRunNumber = findIntValue(&request_block->nameValueList, &plugin_args->refRunNumber,
                                               "refRunNumber|refRun");
    plugin_args->isTimedArg = findIntValue(&request_block->nameValueList, &plugin_args->isTimed, "isTimed");
    plugin_args->isInterpolMode = findIntValue(&request_block->nameValueList, &plugin_args->interpolMode,
                                               "interpolMode");
    plugin_args->isSignal = findStringValue(&request_block->nameValueList, &plugin_args->signal, "signal");
    plugin_args->isSource = findStringValue(&request_block->nameValueList, &plugin_args->source, "source");
    plugin_args->isFormat = findStringValue(&request_block->nameValueList, &plugin_args->format, "format|pattern");
    plugin_args->isOwner = findStringValue(&request_block->nameValueList, &plugin_args->owner, "owner");
    plugin_args->isServer = findStringValue(&request_block->nameValueList, &plugin_args->server, "server");
    plugin_args->isImasIdsVersion = findStringValue(&request_block->nameValueList, &plugin_args->imasIdsVersion,
                                                    "imasIdsVersion|idsVersion");
    plugin_args->isImasIdsDevice = findStringValue(&request_block->nameValueList, &plugin_args->imasIdsDevice,
                                                   "imasIdsDevice|idsDevice|device");
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

    return 0;
}

//----------------------------------------------------------------------------------------
// Create the Data Source argument for the IDAM API
// Use Case: When there are no data_source records in the IDAM metadata catalogue, e.g. JET

// IMAS::source(signal=signal, format=[ppf|jpf|mast|mds] [,source=source] [,shotNumber=shotNumber] [,pass=pass] [,owner=owner])

static int do_source(IDAM_PLUGIN_INTERFACE* idam_plugin_interface, PLUGIN_ARGS plugin_args)
{
    int err = 0;

    if (!plugin_args.isSignal) {
        RAISE_PLUGIN_ERROR("No data object name (signal) has been specified!");
    }

// Prepare common code

    char* env = NULL;
    char work[MAXMETA];

    const PLUGINLIST* plugin_list = idam_plugin_interface->pluginList;    // List of all data reader plugins (internal and external shared libraries)

    if (plugin_list == NULL) {
        RAISE_PLUGIN_ERROR("No plugins are available for this data request!");
    }

    IDAM_PLUGIN_INTERFACE next_plugin_interface = *idam_plugin_interface;        // New plugin interface
    REQUEST_BLOCK next_request_block;
    REQUEST_BLOCK* request_block = idam_plugin_interface->request_block;

    next_plugin_interface.request_block = &next_request_block;
    initServerRequestBlock(&next_request_block);
    strcpy(next_request_block.api_delim, request_block->api_delim);

    strcpy(next_request_block.signal, plugin_args.signal);            // Prepare the API arguments
    if (!plugin_args.isShotNumber) {
        plugin_args.shotNumber = request_block->exp_number;
    }

// JET PPF sources: PPF::/$ppfname/$pulseNumber/$sequence/$owner

    if (plugin_args.isFormat && STR_IEQUALS(plugin_args.format, "ppf")) {            // JET PPF source naming pattern

        if (!plugin_args.isSource) {
            RAISE_PLUGIN_ERROR("No data source has been specified!");
        }

        env = getenv("UDA_JET_DEVICE_ALIAS");

        if (!plugin_args.isShotNumber && !plugin_args.isRunNumber && !plugin_args.isOwner) {
            if (env == NULL) {
                sprintf(next_request_block.source, "JET%sPPF%s/%s/%s", request_block->api_delim,
                        request_block->api_delim, plugin_args.source, request_block->source);
            } else {
                sprintf(next_request_block.source, "%s%sPPF%s/%s/%s", env, request_block->api_delim,
                        request_block->api_delim, plugin_args.source, request_block->source);
            }
        } else {
            if (plugin_args.isShotNumber) {
                if (env == NULL) {
                    sprintf(next_request_block.source, "JET%sPPF%s/%s/%d", request_block->api_delim,
                            request_block->api_delim, plugin_args.source, plugin_args.shotNumber);
                } else {
                    sprintf(next_request_block.source, "%s%sPPF%s/%s/%d", env, request_block->api_delim,
                            request_block->api_delim, plugin_args.source, plugin_args.shotNumber);
                }
                if (plugin_args.isRunNumber) {
                    sprintf(next_request_block.source, "%s/%d", next_request_block.source, plugin_args.runNumber);
                }
            }
            if (plugin_args.isOwner) {
                sprintf(next_request_block.source, "%s/%s", next_request_block.source, plugin_args.owner);
            }
        }
    } else if (plugin_args.isFormat && STR_IEQUALS(plugin_args.format, "jpf")) {        // JET JPF source naming pattern

        env = getenv("UDA_JET_DEVICE_ALIAS");

        if (env == NULL) {
            sprintf(next_request_block.source, "JET%sJPF%s%d", request_block->api_delim,
                    request_block->api_delim, plugin_args.shotNumber);
        } else {
            sprintf(next_request_block.source, "%s%sJPF%s%d", env, request_block->api_delim,
                    request_block->api_delim, plugin_args.shotNumber);
        }
    } else if (plugin_args.isFormat && STR_IEQUALS(plugin_args.format, "MAST")) {        // MAST source naming pattern

        env = getenv("UDA_MAST_DEVICE_ALIAS");

        if (!plugin_args.isShotNumber && !plugin_args.isRunNumber) {
            strcpy(next_request_block.source, request_block->source);        // Re-Use the original source argument
        } else {
            if (env == NULL) {
                sprintf(next_request_block.source, "MAST%s%d", request_block->api_delim, plugin_args.shotNumber);
            } else {
                sprintf(next_request_block.source, "%s%s%d", env, request_block->api_delim, plugin_args.shotNumber);
            }
        }
        if (plugin_args.isRunNumber) {
            sprintf(next_request_block.source, "%s/%d", next_request_block.source, plugin_args.runNumber);
        }

    } else if (plugin_args.isFormat &&
               (STR_IEQUALS(plugin_args.format, "mds") || STR_IEQUALS(plugin_args.format, "mdsplus") ||
                STR_IEQUALS(plugin_args.format, "mds+"))) {    // MDS+ source naming pattern

        if (!plugin_args.isServer) {
            RAISE_PLUGIN_ERROR("No data server has been specified!");
        }

        env = getenv("UDA_MDSPLUS_ALIAS");

        if (plugin_args.isSource) {    // TDI function or tree?
            if (env == NULL) {
                sprintf(next_request_block.source, "MDSPLUS%s%s/%s/%d", request_block->api_delim, plugin_args.server,
                        plugin_args.source, plugin_args.shotNumber);
            } else {
                sprintf(next_request_block.source, "%s%s%s/%s/%d", env, request_block->api_delim, plugin_args.server,
                        plugin_args.source, plugin_args.shotNumber);
            }
        } else {
            if (env == NULL) {
                sprintf(next_request_block.source, "MDSPLUS%s%s", request_block->api_delim, plugin_args.server);
            } else {
                sprintf(next_request_block.source, "%s%s%s", env, request_block->api_delim, plugin_args.server);
            }
            char* p = NULL;
            if ((p = strstr(next_request_block.signal, "$pulseNumber")) != NULL) {
                p[0] = '\0';
                sprintf(p, "%d%s", plugin_args.shotNumber, &p[12]);
            }
        }
    } else {
        RAISE_PLUGIN_ERROR("the specified format is not recognised!");
    }

// Create the Request data structure

    env = getenv("UDA_IDAM_PLUGIN");

    if (env != NULL) {
        sprintf(work, "%s::get(host=%s, port=%d, signal=\"%s\", source=\"%s\")", env, getIdamServerHost(),
                getIdamServerPort(), next_request_block.signal, next_request_block.source);
    } else {
        sprintf(work, "IDAM::get(host=%s, port=%d, signal=\"%s\", source=\"%s\")", getIdamServerHost(),
                getIdamServerPort(), next_request_block.signal, next_request_block.source);
    }

    next_request_block.source[0] = '\0';
    strcpy(next_request_block.signal, work);

    makeServerRequestBlock(&next_request_block, *plugin_list);

// Call the IDAM client via the IDAM plugin (ignore the request identified)

    if (env != NULL) {
        next_request_block.request = findPluginRequestByFormat(env, plugin_list);
    } else {
        next_request_block.request = findPluginRequestByFormat("IDAM", plugin_list);
    }

    if (next_request_block.request < 0) {
        RAISE_PLUGIN_ERROR("No UDA server plugin found!");
    }

// Locate and Execute the UDA plugin

    int id = findPluginIdByRequest(next_request_block.request, plugin_list);
    if (id >= 0 && plugin_list->plugin[id].idamPlugin != NULL) {
        err = plugin_list->plugin[id].idamPlugin(&next_plugin_interface);        // Call the data reader
    } else {
        RAISE_PLUGIN_ERROR("Data Access is not available for this data request!");
    }

    freeNameValueList(&next_request_block.nameValueList);

// Return data is automatic since both next_request_block and request_block point to the same DATA_BLOCK etc.

    return err;
}

//----------------------------------------------------------------------------------------
// IDS Version
static int do_putIdsVersion(IDAM_PLUGIN_INTERFACE* idam_plugin_interface, PLUGIN_ARGS plugin_args)
{
    if (!plugin_args.isImasIdsVersion && !plugin_args.isImasIdsDevice) {
        UDA_LOG(UDA_LOG_ERROR, "imas version: An IDS Version number or a Device name is required!\n");
        THROW_ERROR(999, "An IDS Version number or a Device name is required!");
    }

    if (plugin_args.isImasIdsVersion) {
        putImasIdsVersion(plugin_args.imasIdsVersion);
    }
    if (plugin_args.isImasIdsDevice) {
        putImasIdsDevice(plugin_args.imasIdsDevice);
    }

    // Return the Status OK

    return setReturnDataIntScalar(idam_plugin_interface->data_block, OK_RETURN_VALUE, NULL);
}

//----------------------------------------------------------------------------------------
// DELETE Data from an IDS file

static int do_delete(IDAM_PLUGIN_INTERFACE* idam_plugin_interface, PLUGIN_ARGS plugin_args)
{
    int rc;

    if (plugin_args.isClientIdx && plugin_args.isCPOPath && plugin_args.isPath) {
        rc = mdsDeleteData(plugin_args.clientIdx, (char*)plugin_args.CPOPath, (char*)plugin_args.path);
    } else {
        THROW_ERROR(999, "Incomplete set of arguments!");
    }

    if (rc < 0) {
        THROW_ERROR(999, "Data DELETE method failed!");
    }

    // Return the data

    return setReturnDataIntScalar(idam_plugin_interface->data_block, rc, NULL);
}

//----------------------------------------------------------------------------------------
// IMAS get some data - assumes the file exists

static int do_get(IDAM_PLUGIN_INTERFACE* idam_plugin_interface, PLUGIN_ARGS plugin_args, int idx)
{
    int err = 0;
    int rc = 0;
    int* shape = NULL;
    int type = UDA_TYPE_UNKNOWN;

/*
idx	- reference to the open data file: file handle from an array of open files - hdf5Files[idx]
cpoPath	- the root group where the CPO/IDS is written
path	- the path relative to the root (cpoPath) where the data are written (must include the variable name!)
*/

// Convert type string into IMAS type identifiers

    if (!plugin_args.isGetDimension) {
        if (!plugin_args.isTypeName && !plugin_args.isPutData) {
            UDA_LOG(UDA_LOG_ERROR, "imas get: The data's Type has not been specified!\n");
            THROW_ERROR(999, "The data's Type has not been specified!");
        }

        if ((type = findIMASType(plugin_args.typeName)) == 0) {
            UDA_LOG(UDA_LOG_ERROR, "imas get: The data's Type name cannot be converted!\n");
            THROW_ERROR(999, "The data's Type name cannot be converted!");
        }
    }

    if (!plugin_args.isPutData && !plugin_args.isRank && !plugin_args.isGetDimension) {
        UDA_LOG(UDA_LOG_ERROR, "imas get: The data's Rank has not been specified!\n");
        THROW_ERROR(999, "The data's Rank has not been specified!");
    }

    if (plugin_args.isGetDimension) plugin_args.rank = 7;

    shape = (int*)malloc((plugin_args.rank + 1) * sizeof(int));
    shape[0] = 1;

    int i;
    for (i = 1; i < plugin_args.rank; i++) shape[i] = 0;

    if (plugin_args.isGetDimension) plugin_args.rank = 7;

// Which Data Operation?

    int dataOperation = GET_OPERATION;
    if (plugin_args.isGetDataSlice) {
        dataOperation = GETSLICE_OPERATION;
    } else if (plugin_args.isGetDimension) {
        dataOperation = GETDIMENSION_OPERATION;
    }

    REQUEST_BLOCK* request_block = idam_plugin_interface->request_block;
    PUTDATA_BLOCK_LIST* putDataBlockList = &request_block->putDataBlockList;
    PUTDATA_BLOCK* putDataBlock = NULL;
    plugin_args.isPutData = (putDataBlockList != NULL && putDataBlockList->blockCount > 0);
    if (plugin_args.isPutData) {
        putDataBlock = &(putDataBlockList->putDataBlock[0]);
    }

// GET and return the data

    char* imasData = NULL;
    double retTime = 0.0;

    DATA_BLOCK* data_block = idam_plugin_interface->data_block;

    initDataBlock(data_block);

    if (dataOperation == GET_OPERATION) {

        // Replacing // in path with / to standardise paths for mapping
        char* tmp_path = calloc(strlen((char*)plugin_args.path) + 1, sizeof(char));
        i = 0;
        size_t ii = 0;
        while (i < strlen((char*)plugin_args.path)) {
            if (plugin_args.path[i] == '/' && plugin_args.path[i + 1] == '/') {
                ++i;
            }
            tmp_path[ii++] = plugin_args.path[i++];
        }
        strcpy((char*)plugin_args.path, tmp_path);
        free(tmp_path);

        if (plugin_args.clientIdx < 0) {
            static int cache_shot = 0;
            static int cache_run = 0;
            static int cache_idx = 0;

	    if (cache_shot == plugin_args.shotNumber && cache_run == plugin_args.runNumber) {
                idx = cache_idx;
            } else {
                if (mdsimasOpen((char*)"ids", plugin_args.shotNumber, plugin_args.runNumber, &idx) < 0) {
                    THROW_ERROR(999, "Data OPEN method failed!");
                }
                cache_shot = plugin_args.shotNumber;
                cache_run = plugin_args.runNumber;
                cache_idx = idx;
            }
        }

        UDA_LOG(UDA_LOG_ERROR, "CPOPath: %s, path: %s, type: %d, rank: %d, shape[0]: %d\n", (char*)plugin_args.CPOPath,
                (char*)plugin_args.path, type,
                plugin_args.rank,
                plugin_args.rank > 0 ? shape[0] : 0);
        rc = imas_mds_getData(idx, (char*)plugin_args.CPOPath, (char*)plugin_args.path, type, plugin_args.rank, shape,
                              (void**)&imasData);

        if (rc != 0) {
            // data not in IDS - go to other plugins to try and get it

            const char* expName = NULL;
            FIND_REQUIRED_STRING_VALUE(idam_plugin_interface->request_block->nameValueList, expName);

            const char* pluginName = NULL;
            if (StringEquals(expName, "WEST")) {
                pluginName = "WEST_TUNNEL";
            } else {
                pluginName = "EXP2IMAS";
            }

            int id = findPluginIdByFormat(pluginName, idam_plugin_interface->pluginList);
            if (id < 0) {
                UDA_LOG(UDA_LOG_ERROR, "Specified IDAM data plugin not found\n");
            } else {
                PLUGIN_DATA* plugin = &idam_plugin_interface->pluginList->plugin[id];

                char* path = NULL;
                int* indices = NULL;
                int num_indices = extract_array_indices((char*)plugin_args.path, &path, &indices);
                char* indices_string = indices_to_string(indices, num_indices);

                int get_shape = 0;

//                size_t len = strlen(path);
//                if (len > 5 && (STR_EQUALS(path + (len - 5), "/time") || STR_EQUALS(path + (len - 5), "/data"))) {
//                    path[len - 5] = '\0';
//                }

                REQUEST_BLOCK new_request;
                copyRequestBlock(&new_request, *idam_plugin_interface->request_block);

                const char* fmt = path[0] == '/'
                    ? "%s%sread(element=%s%s, indices=%s, shot=%d, dtype=%d, rank=%d%s, IDS_version=3.15.1)"
                    : "%s%sread(element=%s/%s, indices=%s, shot=%d, dtype=%d, rank=%d%s, IDS_version=3.15.1)";

                int shot = plugin_args.isShotNumber
                           ? plugin_args.shotNumber
                           : ual_get_shot(idx);

                int uda_type = findIMASIDAMType(type);

                sprintf(new_request.signal, fmt, expName, new_request.api_delim, (char*)plugin_args.CPOPath, path,
                        indices_string, shot, uda_type, plugin_args.rank, get_shape ? ", get_shape" : "");

                UDA_LOG(UDA_LOG_DEBUG, "imas: %s", new_request.signal);

                makeServerRequestBlock(&new_request, *idam_plugin_interface->pluginList);
                printRequestBlock(new_request);

                idam_plugin_interface->request_block = &new_request;

                rc = plugin->idamPlugin(idam_plugin_interface);

                if (rc == 0) {
                    for (i = 0; i < data_block->rank; ++i) {
                        shape[i] = data_block->dims[i].dim_n;
                    }
                    if (uda_type != data_block->data_type) {
                        if (uda_type == UDA_TYPE_DOUBLE && data_block->data_type == UDA_TYPE_FLOAT) {
                            imasData = malloc(data_block->data_n * sizeof(double));
                            for (i = 0; i < data_block->data_n; ++i) {
                                ((double*)imasData)[i] = ((float*)data_block->data)[i];
                            }
                            free(data_block->data);
                            data_block->data = imasData;
                        } else {
                            UDA_LOG(UDA_LOG_ERROR, "imas get: wrong data type returned\n");
                            THROW_ERROR(999, "wrong data type returned");
                        }
                    } else {
                        imasData = data_block->data;
                    }
                    imas_mds_putData(idx, (char*)plugin_args.CPOPath, (char*)plugin_args.path, type, plugin_args.rank,
                                     shape, PUT_OPERATION, (void*)imasData, 0.0);
                }
            }
        }
    } else if (dataOperation == GETSLICE_OPERATION) {
        if (!plugin_args.isInterpolMode) {
            UDA_LOG(UDA_LOG_ERROR, "imas get: No Interpolation Mode has been specified!\n");
            THROW_ERROR(999, "No Interpolation Mode has been specified!");
        }

        if (!plugin_args.isPutData || putDataBlock->data_type != UDA_TYPE_DOUBLE || putDataBlock->count != 3) {
            UDA_LOG(UDA_LOG_ERROR, "imas get: No Time Values have been specified!\n");
            THROW_ERROR(999, "No Time Values have been specified!");
        }

        double time = ((double*)putDataBlock->data)[0];

        rc = imas_mds_getDataSlices(idx, (char*)plugin_args.CPOPath, (char*)plugin_args.path, type, plugin_args.rank, shape,
                                    (void**)&imasData, time, &retTime,
                                    plugin_args.interpolMode);

    } else if (dataOperation == GETDIMENSION_OPERATION) {
        rc = mdsGetDimension(idx, (char*)plugin_args.CPOPath, (char*)plugin_args.path, &plugin_args.rank, &shape[0], &shape[1],
                             &shape[2], &shape[3], &shape[4],
                             &shape[5], &shape[6]);
    }

    if (rc < 0 || (!plugin_args.isGetDimension && imasData == NULL)) {
        free(shape);
        THROW_ERROR(999, "Data GET method failed!");
    }

// Return Data

    switch (dataOperation) {
        case GETDIMENSION_OPERATION: {
            data_block->rank = 1;
            data_block->data_type = UDA_TYPE_INT;
            data_block->data = (char*)shape;
            data_block->data_n = plugin_args.rank;
            break;
        }
        case GET_OPERATION: {
            data_block->data_type = findIMASIDAMType(type);
            data_block->data = imasData;
            data_block->rank = (unsigned int)plugin_args.rank;
            if (plugin_args.rank == 0 && type == STRING) {
                data_block->data_n = (int)strlen(imasData) + 1;
            } else {
                data_block->data_n = shape[0];
            }
            for (i = 1; i < plugin_args.rank; i++) {
                data_block->data_n *= shape[i];
            }
            break;
        }
        case GETSLICE_OPERATION: {
            // Need to return Data as well as the time - increase rank by 1 and pass in a
            // coordinate array of length 1
            data_block->rank = plugin_args.rank;
            data_block->data_type = findIMASIDAMType(type);
            data_block->data = imasData;
            data_block->data_n = shape[0];
            for (i = 1; i < plugin_args.rank; i++)data_block->data_n *= shape[i];
            break;
        }
    }

    // Return dimensions

    if (data_block->rank > 0 || dataOperation == GETSLICE_OPERATION) {
        data_block->dims = (DIMS*)malloc((data_block->rank + 1) * sizeof(DIMS));
        for (i = 0; i < data_block->rank; i++) {
            initDimBlock(&data_block->dims[i]);
            data_block->dims[i].dim_n = shape[i];
            data_block->dims[i].data_type = UDA_TYPE_UNSIGNED_INT;
            data_block->dims[i].compressed = 1;
            data_block->dims[i].dim0 = 0.0;
            data_block->dims[i].diff = 1.0;
            data_block->dims[i].method = 0;
        }
        if (dataOperation == GETSLICE_OPERATION) {
            data_block->rank = plugin_args.rank + 1;
            data_block->order = plugin_args.rank;
            initDimBlock(&data_block->dims[plugin_args.rank]);
            data_block->dims[plugin_args.rank].dim_n = 1;
            data_block->dims[plugin_args.rank].data_type = UDA_TYPE_DOUBLE;
            data_block->dims[plugin_args.rank].compressed = 0;
            double* sliceTime = (double*)malloc(sizeof(double));
            *sliceTime = retTime;
            data_block->dims[plugin_args.rank].dim = (char*)sliceTime;
        }
    }

    if (!plugin_args.isGetDimension && shape != NULL) {
        free(shape);
    }

    return err;
}


//----------------------------------------------------------------------------------------
// IMAS put some data - assumes the file exists

static int do_put(IDAM_PLUGIN_INTERFACE* idam_plugin_interface, PLUGIN_ARGS plugin_args, int idx)
{
    int err = 0;
    int rc = 0;
    int dataRank;
    int* shape = NULL;
    int isShape = 0, isData = 0, isType = 0;
    int shapeCount = 0, dataCount = 0;
    int type, idamType;
    void* data = NULL;

    int isVarData = 0;
    short varDataIndex = -1;

    int isTime = 0;
    double putDataSliceTime = 0.0;

    PUTDATA_BLOCK localPutDataBlock;

    REQUEST_BLOCK* request_block = idam_plugin_interface->request_block;
    PUTDATA_BLOCK_LIST* putDataBlockList = &request_block->putDataBlockList;
    PUTDATA_BLOCK* putDataBlock = NULL;
    plugin_args.isPutData = (putDataBlockList != NULL && putDataBlockList->blockCount > 0);
    if (plugin_args.isPutData) {
        putDataBlock = &(putDataBlockList->putDataBlock[0]);
    }

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

// Has a PUTDATA block been passed with a missing or matching name?

    if (plugin_args.isPutData) {
        int i;
        for (i = 0; i < putDataBlockList->blockCount; i++) {

            if (putDataBlockList->putDataBlock[i].blockName == NULL) {
                if (putDataBlockList->blockCount > 1) {
                    UDA_LOG(UDA_LOG_ERROR, "imas: Multiple un-named data items - ambiguous!\n");
                    THROW_ERROR(999, "Multiple un-named data items - ambiguous!");
                }
                varDataIndex = i;
                break;
            } else if (STR_IEQUALS(putDataBlockList->putDataBlock[i].blockName, "variable") ||
                       STR_IEQUALS(putDataBlockList->putDataBlock[i].blockName, "data") ||
                       (plugin_args.isPath &&
                        STR_IEQUALS(putDataBlockList->putDataBlock[i].blockName, (char*)plugin_args.path))) {
                varDataIndex = i;
                break;
            }
        }
        for (i = 0; i < putDataBlockList->blockCount; i++) {
            if (putDataBlockList->putDataBlock[i].blockName != NULL && (
                    STR_IEQUALS(putDataBlockList->putDataBlock[i].blockName, "putDataTime") ||
                    STR_IEQUALS(putDataBlockList->putDataBlock[i].blockName, "time"))) {
                putDataSliceTime = ((double*)putDataBlockList->putDataBlock[i].data)[0];
                isTime = 1;
                break;
            }
        }
        if (varDataIndex < 0 && putDataBlockList->blockCount == 1 &&
            (putDataBlockList->putDataBlock[0].blockName == NULL ||
             putDataBlockList->putDataBlock[0].blockName[0] == '\0')) {
            varDataIndex = 0;
        }

        if (varDataIndex < 0 && plugin_args.isShapeString) {
            if ((dataRank = getIdamNameValuePairVarArray(plugin_args.shapeString, plugin_args.quote,
                                                         plugin_args.delimiter, (unsigned short)plugin_args.rank,
                                                         UDA_TYPE_INT, (void**)&shape)) < 0) {
                UDA_LOG(UDA_LOG_ERROR, "imas put: Unable to convert the passed shape values!\n");
                THROW_ERROR(-dataRank, "Unable to convert the passed shape value!");
            }
            if (plugin_args.isRank && plugin_args.rank != dataRank) {
                UDA_LOG(UDA_LOG_ERROR, "imas put: The passed rank is inconsistent with the passed shape data!\n");
                THROW_ERROR(999, "The passed rank is inconsistent with the passed shape data!");
            }
            isShape = 1;
            plugin_args.isRank = 1;
            plugin_args.rank = dataRank;

            shapeCount = shape[0];
            for (i = 1; i < plugin_args.rank; i++) {
                shapeCount = shapeCount * shape[i];
            }
        }

        if (varDataIndex < 0 && plugin_args.isDataString) {

            if ((idamType = findIdamType(plugin_args.typeName)) == UDA_TYPE_UNDEFINED) {
                UDA_LOG(UDA_LOG_ERROR, "imas put: The data's Type name cannot be converted!\n");
                THROW_ERROR(999, "The data's Type name cannot be converted!");
            }

            if ((dataCount = getIdamNameValuePairVarArray(plugin_args.dataString, plugin_args.quote,
                                                          plugin_args.delimiter,
                                                          (unsigned short)shapeCount, idamType,
                                                          &data)) < 0) {
                UDA_LOG(UDA_LOG_ERROR, "imas put: Unable to convert the passed data values!\n");
                THROW_ERROR(-dataCount, "Unable to convert the passed data value!");
            }
            if (plugin_args.isShapeString && shapeCount != dataCount) {
                UDA_LOG(UDA_LOG_ERROR, "imas put: Inconsistent count of Data items!\n");
                THROW_ERROR(999, "Inconsistent count of Data items!");
            }
            isData = 1;
            isVarData = 1;

            putDataBlock = &localPutDataBlock;
            putDataBlock->data_type = idamType;
            putDataBlock->data = data;
            putDataBlock->count = dataCount;
            putDataBlock->shape = NULL;
            if (plugin_args.isRank) {
                putDataBlock->rank = plugin_args.rank;
            } else {
                if (dataCount == 1) {
                    putDataBlock->rank = 0;
                } else {
                    putDataBlock->rank = 1;
                }
            }
            type = findIMASType(convertIdam2StringType(putDataBlock->data_type));
        }

        if (varDataIndex < 0 && !plugin_args.isDataString) {
            UDA_LOG(UDA_LOG_ERROR, "imas put: Unable to Identify the data to PUT!\n");
            THROW_ERROR(999, "Unable to Identify the data to PUT!");
        }

        if (varDataIndex >= 0) {
            isVarData = 1;
            putDataBlock = &putDataBlockList->putDataBlock[varDataIndex];
            if ((type = findIMASType(convertIdam2StringType(putDataBlock->data_type))) == 0) {
                // Convert an IDAM type to an IMAS type
                UDA_LOG(UDA_LOG_ERROR, "imas put: The data's Type cannot be converted!\n");
                THROW_ERROR(999, "The data's Type cannot be converted!");
            }
        }

    } else {    // isPutData

// Convert type string into IMAS type identifiers

        if (!plugin_args.isTypeName) {
            UDA_LOG(UDA_LOG_ERROR, "imas put: The data's Type has not been specified!\n");
            THROW_ERROR(999, "The data's Type has not been specified!");
        }

        if ((type = findIMASType(plugin_args.typeName)) == 0) {
            UDA_LOG(UDA_LOG_ERROR, "imas put: The data's Type name cannot be converted!\n");
            THROW_ERROR(999, "The data's Type name cannot be converted!");
        }
    }

    isType = 1;

// Any Data?

    if (!plugin_args.isDataString && !isVarData) {
        UDA_LOG(UDA_LOG_ERROR, "imas put: No data has been specified!\n");
        THROW_ERROR(999, "No data has been specified!");
    }

    if (plugin_args.isPutDataSlice && !isTime) {
        UDA_LOG(UDA_LOG_ERROR, "imas put: No specific time has been specified!\n");
        THROW_ERROR(999, "No specific time has been specified!");
    }

// Which Data Operation?

    int dataOperation = PUT_OPERATION;
    if (plugin_args.isPutDataSlice) {
        dataOperation = PUTSLICE_OPERATION;
    } else if (plugin_args.isReplaceLastDataSlice) {
        dataOperation = REPLACELASTSLICE_OPERATION;
    }

// Convert Name-Value string arrays (shape, data) to numerical arrays of the correct type

    if (!plugin_args.isPutData) {
        if (!plugin_args.isRank && !plugin_args.isShapeString) {        // Assume a scalar value
            plugin_args.isRank = 1;
            isShape = 1;
            plugin_args.rank = 0;
            shape = (int*)malloc(sizeof(int));
            shape[0] = 1;
            shapeCount = 1;
        }

        if (plugin_args.isShapeString) {
            if ((dataRank = getIdamNameValuePairVarArray(plugin_args.shapeString, plugin_args.quote,
                                                         plugin_args.delimiter, (unsigned short)plugin_args.rank,
                                                         UDA_TYPE_INT, (void**)&shape)) < 0) {
                UDA_LOG(UDA_LOG_ERROR, "Unable to convert the passed shape values!\n");
                THROW_ERROR(-dataRank, "Unable to convert the passed shape value!");
            }
            if (plugin_args.isRank && plugin_args.rank != dataRank) {
                UDA_LOG(UDA_LOG_ERROR, "The passed rank is inconsistent with the passed shape data!\n");
                THROW_ERROR(999, "The passed rank is inconsistent with the passed shape data!");
            }
            isShape = 1;
            plugin_args.isRank = 1;
            plugin_args.rank = dataRank;

            shapeCount = shape[0];
            int i;
            for (i = 1; i < plugin_args.rank; i++) {
                shapeCount = shapeCount * shape[i];
            }
        }

        if (plugin_args.isDataString) {

            if ((idamType = findIdamType(plugin_args.typeName)) == UDA_TYPE_UNDEFINED) {
                UDA_LOG(UDA_LOG_ERROR, "The data's Type name cannot be converted!\n");
                THROW_ERROR(999, "The data's Type name cannot be converted!");
            }

            if ((dataCount = getIdamNameValuePairVarArray(plugin_args.dataString, plugin_args.quote,
                                                          plugin_args.delimiter,
                                                          (unsigned short)shapeCount, idamType,
                                                          &data)) < 0) {
                UDA_LOG(UDA_LOG_ERROR, "Unable to convert the passed data values!\n");
                THROW_ERROR(-dataCount, "Unable to convert the passed data value!");
            }
            if (shapeCount != dataCount) {
                UDA_LOG(UDA_LOG_ERROR, "Inconsistent count of Data items!\n");
                THROW_ERROR(999, "Inconsistent count of Data items!");
            }
            isData = 1;
        }

// Test all required data is available

        if (!plugin_args.isCPOPath || !plugin_args.isPath || !isType || !plugin_args.isRank || !isShape || !isData) {
            UDA_LOG(UDA_LOG_ERROR, "Insufficient data parameters passed - put not possible!\n");
            THROW_ERROR(999, "Insufficient data parameters passed - put not possible!");
        }

// The type passed here is the IMAS type enumeration (imas_putData is the original imas putData function)

        if (dataOperation == PUT_OPERATION) {
            rc = imas_mds_putData(idx, (char*)plugin_args.CPOPath, (char*)plugin_args.path, type, plugin_args.rank, shape,
                                  plugin_args.isTimed, (void*)data,
                                  putDataSliceTime);
        } else {
            if (dataOperation == PUTSLICE_OPERATION) {
                UDA_LOG(UDA_LOG_ERROR, "Slice Time not passed!\n");
                THROW_ERROR(999, "Slice Time not passed!");
            } else {
                rc = imas_mds_putDataX(idx, (char*)plugin_args.CPOPath, (char*)plugin_args.path, type, plugin_args.rank, shape,
                                       dataOperation, data, 0.0);    // Replace Last Slice
            }
        }

        if (rc < 0) err = 999;

// Housekeeping heap

        free((void*)shape);
        free(data);

    } else {        // !isPutData

        if (!plugin_args.isCPOPath || !plugin_args.isPath) {
            UDA_LOG(UDA_LOG_ERROR, "imas put: Insufficient data parameters passed - put not possible!\n");
            THROW_ERROR(999, "Insufficient data parameters passed - put not possible!");
        }

        int freeShape = 0;
        if (putDataBlock->shape == NULL) {
            if (putDataBlock->rank > 1) {
                UDA_LOG(UDA_LOG_ERROR, "imas put: No shape information passed!\n");
                THROW_ERROR(999, "No shape information passed!");
            }

            putDataBlock->shape = (int*)malloc(sizeof(int));
            putDataBlock->shape[0] = putDataBlock->count;
            freeShape = 1;
        }

// TODO....time
        if (dataOperation == PUT_OPERATION) {
            rc = imas_mds_putData(idx, (char*)plugin_args.CPOPath, (char*)plugin_args.path, type, putDataBlock->rank,
                                  putDataBlock->shape, plugin_args.isTimed,
                                  (void*)putDataBlock->data, putDataSliceTime);
        } else {
            if (dataOperation == PUTSLICE_OPERATION) {
                PUTDATA_BLOCK* putTimeBlock = NULL;
                int i;
                for (i = 0; i < putDataBlockList->blockCount; i++) {
                    if (STR_IEQUALS(putDataBlockList->putDataBlock[i].blockName, "time") ||
                        STR_IEQUALS(putDataBlockList->putDataBlock[i].blockName, "putDataTime")) {
                        putTimeBlock = &putDataBlockList->putDataBlock[i];
                        break;
                    }
                }
                if (putTimeBlock == NULL) {
                    UDA_LOG(UDA_LOG_ERROR, "imas put: No Slice Time!\n");
                    THROW_ERROR(999, "No Slice Time!");
                }
                if (putTimeBlock->data_type != UDA_TYPE_DOUBLE || putTimeBlock->count != 1) {
                    UDA_LOG(UDA_LOG_ERROR, "imas put: Slice Time type and count are incorrect!\n");
                    THROW_ERROR(999, "Slice Time type and count are incorrect!");
                }
                rc = imas_mds_putDataX(idx, (char*)plugin_args.CPOPath, (char*)plugin_args.path, type, putDataBlock->rank,
                                       putDataBlock->shape,
                                       dataOperation, (void*)putDataBlock->data, putDataSliceTime);
            } else {
                rc = imas_mds_putDataX(idx, (char*)plugin_args.CPOPath, (char*)plugin_args.path, type, putDataBlock->rank,
                                       putDataBlock->shape, dataOperation, (void*)putDataBlock->data, 0.0);
            }
        }

        if (rc < 0) err = 999;

        if (freeShape && putDataBlock->shape) {
            free((void*)putDataBlock->shape);
            putDataBlock->shape = NULL;
            freeShape = 0;
        }

    }

    if (err != 0) {
        THROW_ERROR(err, "Data PUT method failed!");
    }

    // Return a status value

    return setReturnDataIntScalar(idam_plugin_interface->data_block, OK_RETURN_VALUE, NULL);
}


//----------------------------------------------------------------------------------------
// IMAS open an existing file

static int do_open(IDAM_PLUGIN_INTERFACE* idam_plugin_interface, PLUGIN_ARGS plugin_args, int* idx)
{
/*
name	- filename
shot	- experiment number
run	- analysis number
retIdx	- returned data file index number
*/

    if (!plugin_args.isFileName || !plugin_args.isShotNumber || !plugin_args.isRunNumber) {
        UDA_LOG(UDA_LOG_ERROR, "A Filename, Shot number and Run number are required!\n");
        THROW_ERROR(999, "A Filename, Shot number and Run number are required!");
    }

    if (mdsimasOpen((char*)plugin_args.filename, plugin_args.shotNumber, plugin_args.runNumber, idx) < 0) {
        THROW_ERROR(999, "Data OPEN method failed!");
    }

// Register the file

    char work[512];
    // Use comma separated list (non compliant with filenaming conventions)
    sprintf(work, "%s,%d,%d", (char*)plugin_args.filename, plugin_args.shotNumber, plugin_args.runNumber);
    addIdamPluginFileLong(&pluginFileList_mds, work, *idx);

    // Return the Index Number

    return setReturnDataIntScalar(idam_plugin_interface->data_block, *idx, NULL);
}


//----------------------------------------------------------------------------------------
// IMAS create a new file instance - using a Versioned Device specific IDS model file (the version is stored as header meta data)
/*
mdsimasCreate   calls getMdsShot, passes MDSPLUS_TREE_BASE_$ via environment variable $tree_path - may be overriden externally)
		calls TreeOpen($tree, -1, 0) : int TreeOpen(char *tree, int shot, int read_only) { return _TreeOpen(&DBID,tree,shot,read_only);}
		model file is named $tree_model.[characteristics, datafile, tree]
*/

static int do_create(IDAM_PLUGIN_INTERFACE* idam_plugin_interface, PLUGIN_ARGS plugin_args, int* idx)
{
/*
name	- filename
shot	- experiment number
run	- analysis number
refShot - not used
refRun  - not used
retIdx	- returned data file index number
*/
	char* env = getenv("UDA_IMAS_CAN_CREATE");

	if (env == NULL || StringEquals(env, "0")) {
        UDA_LOG(UDA_LOG_ERROR, "imas: create not allowed\n");
        THROW_ERROR(999, "create not allowed");
	}

    char* env = getenv("UDA_IMAS_CAN_CREATE");
    if (env == NULL || StringEquals(env, "0")) {
        UDA_LOG(UDA_LOG_ERROR, "imas: create not allowed on this server\n");
        THROW_ERROR(999, "create not allowed on this server");
    }

    if (!plugin_args.isFileName || !plugin_args.isShotNumber || !plugin_args.isRunNumber) {
        UDA_LOG(UDA_LOG_ERROR, "imas: A Filename, Shot number and Run number are required!\n");
        THROW_ERROR(999, "A Filename, Shot number and Run number are required!");
    }

    if (plugin_args.isCreateFromModel) {
        if (mdsimasCreate((char*)plugin_args.filename, plugin_args.shotNumber, plugin_args.runNumber,
                                plugin_args.refShotNumber, plugin_args.refRunNumber, idx) < 0) {
// BUG: createMdsImasFromModel is missing from the set of mdsplus functions.
            THROW_ERROR(999, "File Create method from Model failed!");
        }
    } else {
        if (mdsimasCreate((char*)plugin_args.filename, plugin_args.shotNumber, plugin_args.runNumber,
                                plugin_args.refShotNumber, plugin_args.refRunNumber, idx) < 0) {
            THROW_ERROR(999, "File Create method failed!");
        }
    }

// Register the file

    char work[512];
    // Use comma separated list (non compliant with filenaming conventions)
    sprintf(work, "%s,%d,%d", (char*)plugin_args.filename, plugin_args.shotNumber, plugin_args.runNumber);
    addIdamPluginFileLong(&pluginFileList_mds, work, *idx);

    // Return the Index Number

    return setReturnDataIntScalar(idam_plugin_interface->data_block, *idx, NULL);
}


//----------------------------------------------------------------------------------------
// IMAS close a file

static int do_close(IDAM_PLUGIN_INTERFACE* idam_plugin_interface, PLUGIN_ARGS plugin_args)
{
    if (!plugin_args.isClientIdx && !(plugin_args.isFileName && plugin_args.isShotNumber && plugin_args.isRunNumber)) {
        UDA_LOG(UDA_LOG_ERROR, "The file IDX or the Filename with Shot number and Run number are required!\n");
        THROW_ERROR(999, "The file IDX or the Filename with Shot number and Run number are required!");
    }

    if (!plugin_args.isClientIdx) {
        char work[512];
        sprintf(work, "%s,%d,%d", (char*)plugin_args.filename, plugin_args.shotNumber, plugin_args.runNumber);
        plugin_args.clientIdx = (int)getOpenIdamPluginFileLong(&pluginFileList_mds, work);
    }

    if (plugin_args.isFileName && plugin_args.isShotNumber && plugin_args.isRunNumber) {
        char work[512];
        sprintf(work, "%s,%d,%d", (char*)plugin_args.filename, plugin_args.shotNumber, plugin_args.runNumber);
        closeIdamPluginFile(&pluginFileList_mds, work);
    } else {
        int id = findIdamPluginFileByLong(&pluginFileList_mds, plugin_args.clientIdx);
        if (id >= 0) {
            char work[512];
            strcpy(work, pluginFileList_mds.files[id].filename);
            closeIdamPluginFile(&pluginFileList_mds, work);

            char* p = strrchr(work, ',');
            if (p != NULL) {
                plugin_args.runNumber = atoi(&p[1]);
                p[0] = '\0';
                p = strrchr(work, ',');
                plugin_args.shotNumber = atoi(&p[1]);
                p[0] = '\0';
                plugin_args.filename = work;
            }
        }
    }

    if (mdsimasClose(plugin_args.clientIdx, (char*)plugin_args.filename, plugin_args.shotNumber, plugin_args.runNumber) < 0) {
        THROW_ERROR(999, "Data Close Failed!");
    }

    // Return Success

    return setReturnDataIntScalar(idam_plugin_interface->data_block, OK_RETURN_VALUE, NULL);
}


//----------------------------------------------------------------------------------------
// IMAS create a MODEL file - groups only, no data

static int do_createModel(IDAM_PLUGIN_INTERFACE* idam_plugin_interface, PLUGIN_ARGS plugin_args)
{
    UDA_LOG(UDA_LOG_ERROR, "Not Implemented for mdsplus!\n");
    THROW_ERROR(999, "Not Implemented for mdsplus!");

/*
         if(!isFileName){
            err = 999;
            UDA_LOG(UDA_LOG_ERROR, "imas createModel: A Filename is required!\n");
            addIdamError(CODEERRORTYPE, "imas", err, "A Filename is required!");
	    break;
	 }

	 if((rc = imas_hdf5IdsModelCreate(filename, version)) < 0){
            err = 999;
            addIdamError(CODEERRORTYPE, "imas", err, getImasErrorMsg());
            addIdamError(CODEERRORTYPE, "imas", err, "File Model Create method failed!");
            break;
	 }

// Return the Status

	 int *data  = (int *)malloc(sizeof(int));
         data[0] = 1;

	 initDataBlock(data_block);
         data_block->rank = 0;
	 data_block->dims = NULL;
	 data_block->data_type = UDA_TYPE_INT;
	 data_block->data = (char *)data;
	 data_block->data_n = 1;

         break;
*/
}


//----------------------------------------------------------------------------------------
// Time Base

static int do_setTimeBasePath(IDAM_PLUGIN_INTERFACE* idam_plugin_interface, PLUGIN_ARGS plugin_args)
{
    if (!plugin_args.isPath) {
        UDA_LOG(UDA_LOG_ERROR, "imas putTimeBasePath: No path has been specified!\n");
        THROW_ERROR(999, "No path has been specified!");
    }

    putTimeBasePath((char*)plugin_args.path);

    // Return the Status OK

    return setReturnDataIntScalar(idam_plugin_interface->data_block, OK_RETURN_VALUE, NULL);
}


//----------------------------------------------------------------------------------------
//----------------------------------------------------------------------------------------
// Put a Data Slice into an Object

static int do_releaseObject(IDAM_PLUGIN_INTERFACE* idam_plugin_interface, PLUGIN_ARGS plugin_args)
{
// Test all required data is available

    if (!plugin_args.isPutData) {
        UDA_LOG(UDA_LOG_ERROR, "imas releaseObject: Insufficient data parameters passed - begin not possible!\n");
        THROW_ERROR(999, "Insufficient data parameters passed - begin not possible!");
    }

    PUTDATA_BLOCK_LIST* putDataBlockList = &idam_plugin_interface->request_block->putDataBlockList;
    PUTDATA_BLOCK* putDataBlock = NULL;
    plugin_args.isPutData = (putDataBlockList != NULL && putDataBlockList->blockCount > 0);
    if (plugin_args.isPutData) {
        putDataBlock = &(putDataBlockList->putDataBlock[0]);
    }

// Identify the local object

    void* obj = findLocalObj(*((int*)putDataBlock->data));

    if (obj == NULL) obj = mdsBeginObject();

// call the original IMAS function

    if (obj != NULL) mdsReleaseObject(obj);

    return setReturnDataIntScalar(idam_plugin_interface->data_block, OK_RETURN_VALUE, NULL);
}


//----------------------------------------------------------------------------------------
// Get a Data Slice from an Object

static int do_getObjectObject(IDAM_PLUGIN_INTERFACE* idam_plugin_interface, PLUGIN_ARGS plugin_args)
{
// Test all required data is available

    if (!plugin_args.isPutData || !plugin_args.isIndex || !plugin_args.isPath) {
        UDA_LOG(UDA_LOG_ERROR, "Insufficient data parameters passed - begin not possible!\n");
        THROW_ERROR(999, "Insufficient data parameters passed - begin not possible!");
    }

    PUTDATA_BLOCK_LIST* putDataBlockList = &idam_plugin_interface->request_block->putDataBlockList;
    PUTDATA_BLOCK* putDataBlock = NULL;
    plugin_args.isPutData = (putDataBlockList != NULL && putDataBlockList->blockCount > 0);
    if (plugin_args.isPutData) {
        putDataBlock = &(putDataBlockList->putDataBlock[0]);
    }

// Identify the local object

    void* obj = findLocalObj(*((int*)putDataBlock->data));

    if (obj == NULL) obj = mdsBeginObject();

// call the original IMAS function

    void* dataObj = NULL;
    int rc = mdsGetObjectFromObject(obj, (char*)plugin_args.path, plugin_args.index, &dataObj);

    if (rc < 0) {
        THROW_ERROR(999, "Object Get failed!");
    }

    // Save the object reference

    int refId = putLocalObj(dataObj);

    return setReturnDataIntScalar(idam_plugin_interface->data_block, refId, NULL);
}


//----------------------------------------------------------------------------------------
// Get a Data Slice from an Object

static int do_getObjectSlice(IDAM_PLUGIN_INTERFACE* idam_plugin_interface, PLUGIN_ARGS plugin_args)
{
    PUTDATA_BLOCK_LIST* putDataBlockList = &idam_plugin_interface->request_block->putDataBlockList;
    PUTDATA_BLOCK* putDataBlock = NULL;
    plugin_args.isPutData = (putDataBlockList != NULL && putDataBlockList->blockCount > 0);
    if (plugin_args.isPutData) {
        putDataBlock = &(putDataBlockList->putDataBlock[0]);
    }

// Test all required data is available

    if (!plugin_args.isPutData || !plugin_args.isClientIdx || !plugin_args.isPath || !plugin_args.isCPOPath ||
        putDataBlockList->blockCount != 3) {
        UDA_LOG(UDA_LOG_ERROR, "imas getObjectSlice: Insufficient data parameters passed - begin not possible!\n");
        THROW_ERROR(999, "Insufficient data parameters passed - begin not possible!");
    }

// Set global variables

    setSliceIdx(((int*)putDataBlock[1].data)[0], ((int*)putDataBlock[1].data)[1]);
    setSliceTime(((double*)putDataBlock[2].data)[0], ((double*)putDataBlock[2].data)[1]);

// Identify the local object

    //void *obj = findLocalObj(*((int *)putDataBlock[0].data));

    //if(obj == NULL) obj = mdsBeginObject();

// call the original IMAS function

    void* dataObj = NULL;
    int rc = mdsGetObjectSlice(plugin_args.clientIdx, (char*)plugin_args.CPOPath, (char*)plugin_args.path,
                           *((double*)putDataBlock[0].data), &dataObj);

    if (rc < 0) {
        THROW_ERROR(999, "Object Get failed!");
    }

    // Save the object reference

    int refId = putLocalObj(dataObj);

    // Return the Object reference

    return setReturnDataIntScalar(idam_plugin_interface->data_block, refId, NULL);
}


//----------------------------------------------------------------------------------------
// Get a Data Group from an Object

// TODO Check this !!!!

static int do_getObjectGroup(IDAM_PLUGIN_INTERFACE* idam_plugin_interface, PLUGIN_ARGS plugin_args)
{
// Test all required data is available

    if (!plugin_args.isClientIdx || !plugin_args.isCPOPath || !plugin_args.isPath || !plugin_args.isTimedArg) {
        UDA_LOG(UDA_LOG_ERROR, "Insufficient data parameters passed - begin not possible!\n");
        THROW_ERROR(999, "Insufficient data parameters passed - begin not possible!");
    }

// call the original IMAS function

    void* dataObj = NULL;
    int rc = mdsGetObject(plugin_args.clientIdx, (char*)plugin_args.CPOPath, (char*)plugin_args.path, &dataObj, plugin_args.isTimed);

    if (rc != 0) {
        THROW_ERROR(999, "Object Get failed!");
    }

// Save the object reference

    int refId = putLocalObj(dataObj);

    return setReturnDataIntScalar(idam_plugin_interface->data_block, refId, NULL);
}


//----------------------------------------------------------------------------------------
// Object Dimension: int mdsGetObjectDim(void *obj)

static int do_getObjectDim(IDAM_PLUGIN_INTERFACE* idam_plugin_interface, PLUGIN_ARGS plugin_args)
{
    PUTDATA_BLOCK_LIST* putDataBlockList = &idam_plugin_interface->request_block->putDataBlockList;
    PUTDATA_BLOCK* putDataBlock = NULL;
    plugin_args.isPutData = (putDataBlockList != NULL && putDataBlockList->blockCount > 0);
    if (plugin_args.isPutData) {
        putDataBlock = &(putDataBlockList->putDataBlock[0]);
    }

// Object? The data is the local object reference

    if (!plugin_args.isPutData || putDataBlockList->putDataBlock[0].data == NULL) {
        UDA_LOG(UDA_LOG_ERROR, "imas getObjectDim: No data object has been specified!\n");
        THROW_ERROR(999, "No data object has been specified!");
    }

// Identify the local object

    void* obj = findLocalObj(*((int*)putDataBlock[0].data));

    if (obj == NULL) obj = mdsBeginObject();

    if (obj == NULL) {
        UDA_LOG(UDA_LOG_ERROR, "imas getObjectDim: No data object has been found!\n");
        THROW_ERROR(999, "No data object has been found!");
    }

    int dim = mdsGetObjectDim(obj);

    // Return the Object property

    return setReturnDataIntScalar(idam_plugin_interface->data_block, dim, NULL);
}


//----------------------------------------------------------------------------------------
// Initialise an Object

static int do_beginObject(IDAM_PLUGIN_INTERFACE* idam_plugin_interface, PLUGIN_ARGS plugin_args)
{
    void* data = mdsBeginObject();

    if (data == NULL) {
        THROW_ERROR(999, "Object Begin failed!");
    }

// Save the object reference

    int refId = putLocalObj(data);

    // Return the Object reference

    return setReturnDataIntScalar(idam_plugin_interface->data_block, refId, NULL);
}


//----------------------------------------------------------------------------------------
// Get a Data Slice from an Object

static int do_getObject(IDAM_PLUGIN_INTERFACE* idam_plugin_interface, PLUGIN_ARGS plugin_args)
{
    int err = 0;
    int rc = 0;

// Test all required data is available

    if (!plugin_args.isGetDimension) {
        if (!plugin_args.isPath || !plugin_args.isIndex || !plugin_args.isTypeName || !plugin_args.isClientObjectId ||
            !plugin_args.isRank) {
            UDA_LOG(UDA_LOG_ERROR, "Insufficient data parameters passed - put not possible!\n");
            THROW_ERROR(999, "Insufficient data parameters passed - put not possible!");
        }

// Identify the local object

        void* obj = findLocalObj(plugin_args.clientObjectId);

        if (obj == NULL) obj = mdsBeginObject();

// The type passed here is the IMAS type enumeration

        void* data = NULL;
        int type = findIMASType(plugin_args.typeName);

        int shape[7] = { 0, 0, 0, 0, 0, 0, 0 };

        rc = imas_mds_getDataSliceInObject(obj, (char*)plugin_args.path, plugin_args.index, type, plugin_args.rank, shape,
                                           &data);

        if (rc < 0) {
            free(data);
            THROW_ERROR(999, "Data GET method failed!");
        }

        DATA_BLOCK* data_block = idam_plugin_interface->data_block;

// Return the Data Slice

        initDataBlock(data_block);
        data_block->rank = plugin_args.rank;
        data_block->data_type = findIMASIDAMType(type);
        data_block->data = (char*)data;
        data_block->data_n = shape[0];
        if (data_block->rank > 0) {
            data_block->dims = (DIMS*)malloc(data_block->rank * sizeof(DIMS));
            int i;
            for (i = 0; i < data_block->rank; i++) {
                initDimBlock(&data_block->dims[i]);
                data_block->dims[i].dim_n = shape[i];
                data_block->dims[i].data_type = UDA_TYPE_UNSIGNED_INT;
                data_block->dims[i].compressed = 1;
                data_block->dims[i].dim0 = 0.0;
                data_block->dims[i].diff = 1.0;
                data_block->dims[i].method = 0;
                data_block->data_n *= data_block->dims[i].dim_n;
            }
        }

        return err;
    }

// Dimensional Data Only

    if (!plugin_args.isPath || !plugin_args.isIndex || !plugin_args.isClientObjectId) {
        UDA_LOG(UDA_LOG_ERROR, "Insufficient data parameters passed - put not possible!\n");
        THROW_ERROR(999, "Insufficient data parameters passed - put not possible!");
    }

// Identify the local object

    void* obj = findLocalObj(plugin_args.clientObjectId);

    if (obj == NULL) obj = mdsBeginObject();

// The type passed here is the IMAS type enumeration

    int* shape = (int*)malloc(7 * sizeof(int));
    int numDims = 0;

    rc = mdsGetDimensionFromObject(-1, obj, (char*)plugin_args.path, plugin_args.index, &numDims, &(shape[0]), &(shape[1]),
                                   &(shape[2]),
                                   &(shape[3]), &(shape[4]), &(shape[5]), &(shape[6]));

    if (rc < 0) {
        THROW_ERROR(999, "Data GET method failed!");
    }

    DATA_BLOCK* data_block = idam_plugin_interface->data_block;

// Return the Data Slice

    initDataBlock(data_block);
    data_block->rank = 1;
    data_block->data_type = UDA_TYPE_INT;
    data_block->data = (char*)shape;
    data_block->data_n = numDims;

    data_block->dims = (DIMS*)malloc(data_block->rank * sizeof(DIMS));
    initDimBlock(&data_block->dims[0]);
    data_block->dims[0].dim_n = numDims;
    data_block->dims[0].data_type = UDA_TYPE_UNSIGNED_INT;
    data_block->dims[0].compressed = 1;
    data_block->dims[0].dim0 = 0.0;
    data_block->dims[0].diff = 1.0;
    data_block->dims[0].method = 0;

    return err;
}


//----------------------------------------------------------------------------------------
// Put a Data Slice into an Object

static int do_putObject(IDAM_PLUGIN_INTERFACE* idam_plugin_interface, PLUGIN_ARGS plugin_args)
{
    int type;

    PUTDATA_BLOCK_LIST* putDataBlockList = &idam_plugin_interface->request_block->putDataBlockList;

// Test all required data is available

    if (!plugin_args.isPath || !plugin_args.isIndex || !plugin_args.isPutData || putDataBlockList->blockCount != 2) {
        UDA_LOG(UDA_LOG_ERROR, "Insufficient data parameters passed - put not possible!\n");
        THROW_ERROR(999, "Insufficient data parameters passed - put not possible!");
    }

    // Convert an IDAM type to an IMAS type
    if ((type = findIMASType(convertIdam2StringType(putDataBlockList->putDataBlock[1].data_type))) == 0) {
        UDA_LOG(UDA_LOG_ERROR, "The data's Type cannot be converted!\n");
        THROW_ERROR(999, "The data's Type cannot be converted!");
    }

// Any Data?

    if (putDataBlockList->putDataBlock[1].data == NULL) {
        UDA_LOG(UDA_LOG_ERROR, "No data has been specified!\n");
        THROW_ERROR(999, "No data has been specified!");
    }

// Object?

    if (putDataBlockList->putDataBlock[0].data == NULL) {
        UDA_LOG(UDA_LOG_ERROR, "No data object has been specified!\n");
        THROW_ERROR(999, "No data object has been specified!");
    }

// Identify the local object

    void* obj = findLocalObj(((int*)putDataBlockList->putDataBlock[0].data)[0]);

    if (obj == NULL) obj = mdsBeginObject();

// Create the shape array if the rank is 1 (not passed by IDAM)

    int shape[1];
    if (putDataBlockList->putDataBlock[1].rank == 1 && putDataBlockList->putDataBlock[1].shape == NULL) {
        putDataBlockList->putDataBlock[1].shape = shape;
        shape[0] = putDataBlockList->putDataBlock[1].count;
    }

// The type passed here is the IMAS type enumeration

    void* newObj = imas_mds_putDataSliceInObject(obj, (char*)plugin_args.path, plugin_args.index, type,
                                                 putDataBlockList->putDataBlock[1].rank,
                                                 putDataBlockList->putDataBlock[1].shape,
                                                 (void*)putDataBlockList->putDataBlock[1].data);

    if (putDataBlockList->putDataBlock[1].rank == 1 && putDataBlockList->putDataBlock[1].shape == shape) {
        putDataBlockList->putDataBlock[1].shape = NULL;
    }

    if (!newObj) {
        THROW_ERROR(999, "Data PUT method failed!");
    }

    int refId = putLocalObj(newObj);

    return setReturnDataIntScalar(idam_plugin_interface->data_block, refId, NULL);
}


//----------------------------------------------------------------------------------------
// Put an Object into an Object

static int do_putObjectInObject(IDAM_PLUGIN_INTERFACE* idam_plugin_interface, PLUGIN_ARGS plugin_args)
{
    PUTDATA_BLOCK_LIST* putDataBlockList = &idam_plugin_interface->request_block->putDataBlockList;
    PUTDATA_BLOCK* putDataBlock = NULL;
    plugin_args.isPutData = (putDataBlockList != NULL && putDataBlockList->blockCount > 0);
    if (plugin_args.isPutData) {
        putDataBlock = &(putDataBlockList->putDataBlock[0]);
    }

// Test all required data is available

    if (!plugin_args.isPath || !plugin_args.isIndex || !plugin_args.isPutData || putDataBlock->count != 2) {
        UDA_LOG(UDA_LOG_ERROR, "Insufficient data parameters passed - put not possible!\n");
        THROW_ERROR(999, "Insufficient data parameters passed - put not possible!");
    }

// Any Data?

    if (putDataBlock->data == NULL || putDataBlock->data_type != UDA_TYPE_INT) {
        UDA_LOG(UDA_LOG_ERROR, "No data has been specified!\n");
        THROW_ERROR(999, "No data has been specified!");
    }

// Identify the local object and object to be inserted

    void* obj = findLocalObj(((int*)putDataBlock->data)[0]);
    void* dataObj = findLocalObj(((int*)putDataBlock->data)[1]);

    if (obj == NULL) obj = mdsBeginObject();
    if (dataObj == NULL) dataObj = mdsBeginObject();

// The type passed here is the IMAS type enumeration

    void* newObj = mdsPutObjectInObject(obj, (char*)plugin_args.path, plugin_args.index, dataObj);

    if (!newObj) {
        THROW_ERROR(999, "Data PUT method failed!");
    }

    // Register the object

    int refId = putLocalObj(newObj);

    return setReturnDataIntScalar(idam_plugin_interface->data_block, refId, NULL);
}


//----------------------------------------------------------------------------------------
// Put a Data Group into an Object

static int do_putObjectGroup(IDAM_PLUGIN_INTERFACE* idam_plugin_interface, PLUGIN_ARGS plugin_args)
{
    int err = 0;
    int rc = 0;

// Test all required data is available

    if (!plugin_args.isClientIdx || !plugin_args.isCPOPath || !plugin_args.isPath || !plugin_args.isClientObjectId ||
        !plugin_args.isTimedArg) {
        UDA_LOG(UDA_LOG_ERROR, "Insufficient data parameters passed - putObject not possible!\n");
        THROW_ERROR(999, "Insufficient data parameters passed - putObject not possible!");
    }

// Identify the local object

    void* obj = findLocalObj(plugin_args.clientObjectId);

    if (obj == NULL) obj = mdsBeginObject();

//            struct descriptor * object = (struct descriptor *) obj;    // For debugging

// call the original IMAS function

    rc = mdsPutObject(plugin_args.clientIdx, (char*)plugin_args.CPOPath, (char*)plugin_args.path, obj, plugin_args.isTimed);

    if (rc < 0) {
        THROW_ERROR(999, "Object Put failed!");
    }

    int* data = (int*)malloc(sizeof(int));
    data[0] = rc;

    DATA_BLOCK* data_block = idam_plugin_interface->data_block;

    initDataBlock(data_block);
    data_block->rank = 0;
    data_block->dims = NULL;
    data_block->data_type = UDA_TYPE_INT;
    data_block->data = (char*)data;
    data_block->data_n = 1;

    return err;
}


//----------------------------------------------------------------------------------------
// Put a Data Group into an Object

static int do_putObjectSlice(IDAM_PLUGIN_INTERFACE* idam_plugin_interface, PLUGIN_ARGS plugin_args)
{
    PUTDATA_BLOCK_LIST* putDataBlockList = &idam_plugin_interface->request_block->putDataBlockList;
    PUTDATA_BLOCK* putDataBlock = NULL;
    plugin_args.isPutData = (putDataBlockList != NULL && putDataBlockList->blockCount > 0);
    if (plugin_args.isPutData) {
        putDataBlock = &(putDataBlockList->putDataBlock[0]);
    }

// Test all required data is available

    if (!plugin_args.isClientIdx || !plugin_args.isCPOPath || !plugin_args.isPath || !plugin_args.isPutData ||
        !plugin_args.isClientObjectId) {
        UDA_LOG(UDA_LOG_ERROR, "Insufficient data parameters passed - putObjectSlice not possible!\n");
        THROW_ERROR(999, "Insufficient data parameters passed - putObjectSlice not possible!");
    }

// Required Time

    double time = ((double*)putDataBlock->data)[0];

// Identify the local object

    void* obj = findLocalObj(plugin_args.clientObjectId);

    if (obj == NULL) obj = mdsBeginObject();

// call the original IMAS function

    int rc = mdsPutObjectSlice(plugin_args.clientIdx, (char*)plugin_args.CPOPath, (char*)plugin_args.path, time, obj);

    if (rc < 0) {
        THROW_ERROR(999, "Object Put failed!");
    }

    return setReturnDataIntScalar(idam_plugin_interface->data_block, rc, NULL);
}


//----------------------------------------------------------------------------------------
// Replace Last Object Slice

static int do_replaceLastObjectSlice(IDAM_PLUGIN_INTERFACE* idam_plugin_interface, PLUGIN_ARGS plugin_args)
{

// Test all required data is available

    if (!plugin_args.isClientIdx || !plugin_args.isCPOPath || !plugin_args.isPath ||
        !plugin_args.isClientObjectId) {
        UDA_LOG(UDA_LOG_ERROR, "Insufficient data parameters passed - replaceLastObjectSlice not possible!\n");
        THROW_ERROR(999, "Insufficient data parameters passed - replaceLastObjectSlice not possible!");
    }

// Identify the local object

    void* obj = findLocalObj(plugin_args.clientObjectId);

    if (obj == NULL) obj = mdsBeginObject();

// call the original IMAS function

    int rc = mdsReplaceLastObjectSlice(plugin_args.clientIdx, (char*)plugin_args.CPOPath, (char*)plugin_args.path, obj);

    if (rc < 0) {
        THROW_ERROR(999, "Object Replace failed");
    }

    initDataBlock(idam_plugin_interface->data_block);

    return setReturnDataIntScalar(idam_plugin_interface->data_block, rc, NULL);
}


//----------------------------------------------------------------------------------------
//----------------------------------------------------------------------------------------
// mdsplus: Cache functions
// keywords: flush, discard, getLevel, setLevel, flushCPO, disable, enable
// values: setLevel

static int do_cache(IDAM_PLUGIN_INTERFACE* idam_plugin_interface, PLUGIN_ARGS plugin_args)
{
    int rc;

    if (plugin_args.isFlush && plugin_args.isClientIdx) {
        imas_flush_mem_cache(plugin_args.clientIdx);
        rc = OK_RETURN_VALUE;
    } else if (plugin_args.isDiscard && plugin_args.isClientIdx && plugin_args.isCPOPath) {
        imas_discard_cpo_mem_cache(plugin_args.clientIdx, (char*)plugin_args.CPOPath);
        rc = OK_RETURN_VALUE;
    } else if (plugin_args.isGetLevel && plugin_args.isClientIdx) {
        rc = imas_get_cache_level(plugin_args.clientIdx);
    } else if (plugin_args.isSetLevel && plugin_args.isClientIdx) {
        imas_set_cache_level(plugin_args.clientIdx, plugin_args.setLevel);
        rc = OK_RETURN_VALUE;
    } else if (plugin_args.isFlushCPO && plugin_args.isClientIdx && plugin_args.isCPOPath) {
        imas_flush_cpo_mem_cache(plugin_args.clientIdx, (char*)plugin_args.CPOPath);
        rc = OK_RETURN_VALUE;
    } else if (plugin_args.isDisable && plugin_args.isClientIdx) {
        imas_disable_mem_cache(plugin_args.clientIdx);
        rc = OK_RETURN_VALUE;
    } else if (plugin_args.isDiscard && plugin_args.isClientIdx) {
        imas_discard_mem_cache(plugin_args.clientIdx);
        rc = OK_RETURN_VALUE;
    } else if (plugin_args.isEnable && plugin_args.isClientIdx) {
        imas_enable_mem_cache(plugin_args.clientIdx);
        rc = OK_RETURN_VALUE;
    } else {
        UDA_LOG(UDA_LOG_ERROR, "Insufficient parameters passed!\n");
        THROW_ERROR(999, "Insufficient parameters passed!");
    }

    return setReturnDataIntScalar(idam_plugin_interface->data_block, rc, NULL);
}


// mdsplus: various functions
// keywords: None
// values: None

static int do_getUniqueRun(IDAM_PLUGIN_INTERFACE* idam_plugin_interface, PLUGIN_ARGS plugin_args)
{
    int rc;

    if (plugin_args.isShotNumber) {
        rc = getUniqueRun(plugin_args.shotNumber);
    } else {
        UDA_LOG(UDA_LOG_ERROR, "Insufficient parameters passed!\n");
        THROW_ERROR(999, "Insufficient parameters passed!");
    }

    return setReturnDataIntScalar(idam_plugin_interface->data_block, rc, NULL);
}


// keywords: None
// values: command, ipAddress

static int do_spawnCommand(IDAM_PLUGIN_INTERFACE* idam_plugin_interface, PLUGIN_ARGS plugin_args)
{
    char* result = NULL;

    if (plugin_args.isCommand && plugin_args.isIPAddress) {
        result = spawnCommand((char*)plugin_args.command, (char*)plugin_args.IPAddress);        // from mdsObject library
    } else {
        UDA_LOG(UDA_LOG_ERROR, "Insufficient parameters passed!\n");
        THROW_ERROR(999, "Insufficient parameters passed!");
    }

    return setReturnDataString(idam_plugin_interface->data_block, result, NULL);
}


// keywords: beginIDSSlice, endIDSSlice, replaceIDSSlice, beginIDS, endIDS, beginIDSTimed, endIDSTimed, beginIDSNonTimed, endIDSNonTimed
// values: times

static int do_putIDS(IDAM_PLUGIN_INTERFACE* idam_plugin_interface, PLUGIN_ARGS plugin_args)
{
    int rc;

    if (plugin_args.isClientIdx && plugin_args.isPath) {
        if (plugin_args.isBeginIDSSlice) {
            rc = mdsbeginIdsPutSlice(plugin_args.clientIdx, (char*)plugin_args.path);
        } else if (plugin_args.isEndIDSSlice) {
            rc = mdsendIdsPutSlice(plugin_args.clientIdx, (char*)plugin_args.path);
        } else if (plugin_args.isReplaceIDSSlice) {
            rc = mdsendIdsReplaceLastSlice(plugin_args.clientIdx, (char*)plugin_args.path);
        } else if (plugin_args.isBeginIDS) {
            rc = mdsbeginIdsPut(plugin_args.clientIdx, (char*)plugin_args.path);
        } else if (plugin_args.isEndIDS) {
            rc = mdsendIdsPut(plugin_args.clientIdx, (char*)plugin_args.path);
        } else if (plugin_args.isEndIDSTimed) {
            rc = mdsendIdsPutTimed(plugin_args.clientIdx, (char*)plugin_args.path);
        } else if (plugin_args.isBeginIDSTimed) {
            int count = 0;
            double* times = NULL;
            //TODO ***** fetch data from PUTDATA or name-value list
            rc = mdsbeginIdsPutTimed(plugin_args.clientIdx, (char*)plugin_args.path, count, times);
        } else if (plugin_args.isBeginIDSNonTimed) {
            rc = mdsbeginIdsPutNonTimed(plugin_args.clientIdx, (char*)plugin_args.path);
        } else if (plugin_args.isEndIDSNonTimed) {
            rc = mdsendIdsPutNonTimed(plugin_args.clientIdx, (char*)plugin_args.path);
        } else {
            UDA_LOG(UDA_LOG_ERROR, "Insufficient parameters passed!\n");
            THROW_ERROR(999, "Insufficient parameters passed!");
        }
    } else {
        UDA_LOG(UDA_LOG_ERROR, "Insufficient parameters passed!\n");
        THROW_ERROR(999, "Insufficient parameters passed!");
    }

    // Return value
    return setReturnDataIntScalar(idam_plugin_interface->data_block, rc, NULL);
}


//----------------------------------------------------------------------------------------
// Begin/End IDS operations

static int do_beginIdsPut(IDAM_PLUGIN_INTERFACE* idam_plugin_interface, PLUGIN_ARGS plugin_args)
{
    if (!plugin_args.isClientIdx || !plugin_args.isPath) {
        UDA_LOG(UDA_LOG_ERROR, "The function parameters have not been specified!\n");
        THROW_ERROR(999, "The function parameters have not been specified!");
    }

    if (mdsbeginIdsPut(plugin_args.clientIdx, (char*)plugin_args.path) < 0) {
        UDA_LOG(UDA_LOG_ERROR, "%s\n", errmsg);
        THROW_ERROR(999, errmsg);
    }

    // Return Success
    return setReturnDataIntScalar(idam_plugin_interface->data_block, OK_RETURN_VALUE, NULL);
}

static int do_endIdsPut(IDAM_PLUGIN_INTERFACE* idam_plugin_interface, PLUGIN_ARGS plugin_args)
{
    if (!plugin_args.isClientIdx || !plugin_args.isPath) {
        UDA_LOG(UDA_LOG_ERROR, "The function parameters have not been specified!\n");
        THROW_ERROR(999, "The function parameters have not been specified!");
    }

    if (mdsendIdsPut(plugin_args.clientIdx, (char*)plugin_args.path) < 0) {
        UDA_LOG(UDA_LOG_ERROR, "%s\n", errmsg);
        THROW_ERROR(999, errmsg);
    }

    // Return Success
    return setReturnDataIntScalar(idam_plugin_interface->data_block, OK_RETURN_VALUE, NULL);
}

static int do_beginIdsGet(IDAM_PLUGIN_INTERFACE* idam_plugin_interface, PLUGIN_ARGS plugin_args)
{
    int retSamples = 0;

    if (!plugin_args.isClientIdx || !plugin_args.isPath || !plugin_args.isTimedArg) {
        UDA_LOG(UDA_LOG_ERROR, "The function parameters have not been specified!\n");
        THROW_ERROR(999, "The function parameters have not been specified!");
    }

    if (mdsbeginIdsGet(plugin_args.clientIdx, (char*)plugin_args.path, plugin_args.isTimed, &retSamples) < 0) {
        UDA_LOG(UDA_LOG_ERROR, "%s\n", errmsg);
        THROW_ERROR(999, errmsg);
    }

    // Return Success
    return setReturnDataIntScalar(idam_plugin_interface->data_block, retSamples, NULL);
}

static int do_endIdsGet(IDAM_PLUGIN_INTERFACE* idam_plugin_interface, PLUGIN_ARGS plugin_args)
{
    if (!plugin_args.isClientIdx || !plugin_args.isPath) {
        UDA_LOG(UDA_LOG_ERROR, "The function parameters have not been specified!\n");
        THROW_ERROR(999, "The function parameters have not been specified!");
    }

    if (mdsendIdsGet(plugin_args.clientIdx, (char*)plugin_args.path) < 0) {
        UDA_LOG(UDA_LOG_ERROR, "%s\n", errmsg);
        THROW_ERROR(999, errmsg);
    }

    // Return Success
    return setReturnDataIntScalar(idam_plugin_interface->data_block, OK_RETURN_VALUE, NULL);
}

static int do_beginIdsGetSlice(IDAM_PLUGIN_INTERFACE* idam_plugin_interface, PLUGIN_ARGS plugin_args)
{
    int err = 0;

    if (!plugin_args.isClientIdx || !plugin_args.isPath || !plugin_args.isPutData) {
        UDA_LOG(UDA_LOG_ERROR, "The function parameters have not been specified!\n");
        THROW_ERROR(999, "The function parameters have not been specified!");
    }

    PUTDATA_BLOCK_LIST* putDataBlockList = &idam_plugin_interface->request_block->putDataBlockList;
    PUTDATA_BLOCK* putDataBlock = NULL;
    plugin_args.isPutData = (putDataBlockList != NULL && putDataBlockList->blockCount > 0);
    if (plugin_args.isPutData) {
        putDataBlock = &(putDataBlockList->putDataBlock[0]);
    }

// Time is passed in a single PUTDATA structure

    if (putDataBlock->data_type != UDA_TYPE_DOUBLE || putDataBlock->count != 1) {
        UDA_LOG(UDA_LOG_ERROR, "imas beginIdsGetSlice: No Valid Time Value have been specified!\n");
        THROW_ERROR(999, "No Valid Time Value have been specified!");
    }

    double time = ((double*)putDataBlock->data)[0];

    if (mdsbeginIdsGetSlice(plugin_args.clientIdx, (char*)plugin_args.path, time) < 0) {
        UDA_LOG(UDA_LOG_ERROR, "%s\n", errmsg);
        THROW_ERROR(999, errmsg);
    }

    // Return Success
    return setReturnDataIntScalar(idam_plugin_interface->data_block, OK_RETURN_VALUE, NULL);
}

static int do_endIdsGetSlice(IDAM_PLUGIN_INTERFACE* idam_plugin_interface, PLUGIN_ARGS plugin_args)
{
    if (!plugin_args.isClientIdx || !plugin_args.isPath) {
        UDA_LOG(UDA_LOG_ERROR, "The function parameters have not been specified!\n");
        THROW_ERROR(999, "The function parameters have not been specified!");
    }

    if (mdsendIdsGetSlice(plugin_args.clientIdx, (char*)plugin_args.path) < 0) {
        UDA_LOG(UDA_LOG_ERROR, "%s\n", errmsg);
        THROW_ERROR(999, errmsg);
    }

    // Return Success
    return setReturnDataIntScalar(idam_plugin_interface->data_block, OK_RETURN_VALUE, NULL);
}

static int do_beginIdsPutSlice(IDAM_PLUGIN_INTERFACE* idam_plugin_interface, PLUGIN_ARGS plugin_args)
{
    if (!plugin_args.isClientIdx || !plugin_args.isPath) {
        UDA_LOG(UDA_LOG_ERROR, "The function parameters have not been specified!\n");
        THROW_ERROR(999, "The function parameters have not been specified!");
    }

    if (mdsbeginIdsPutSlice(plugin_args.clientIdx, (char*)plugin_args.path) < 0) {
        UDA_LOG(UDA_LOG_ERROR, "%s\n", errmsg);
        THROW_ERROR(999, errmsg);
    }

    // Return Success
    return setReturnDataIntScalar(idam_plugin_interface->data_block, OK_RETURN_VALUE, NULL);
}

static int do_endIdsPutSlice(IDAM_PLUGIN_INTERFACE* idam_plugin_interface, PLUGIN_ARGS plugin_args)
{
    if (!plugin_args.isClientIdx || !plugin_args.isPath) {
        UDA_LOG(UDA_LOG_ERROR, "The function parameters have not been specified!\n");
        THROW_ERROR(999, "The function parameters have not been specified!");
    }

    if (mdsendIdsPutSlice(plugin_args.clientIdx, (char*)plugin_args.path) < 0) {
        UDA_LOG(UDA_LOG_ERROR, "%s\n", errmsg);
        THROW_ERROR(999, errmsg);
    }

    // Return Success
    return setReturnDataIntScalar(idam_plugin_interface->data_block, OK_RETURN_VALUE, NULL);
}

static int do_beginIdsPutNonTimed(IDAM_PLUGIN_INTERFACE* idam_plugin_interface, PLUGIN_ARGS plugin_args)
{
    if (!plugin_args.isClientIdx || !plugin_args.isPath) {
        UDA_LOG(UDA_LOG_ERROR, "The function parameters have not been specified!\n");
        THROW_ERROR(999, "The function parameters have not been specified!");
    }

    if (mdsbeginIdsPutNonTimed(plugin_args.clientIdx, (char*)plugin_args.path) < 0) {
        UDA_LOG(UDA_LOG_ERROR, "%s\n", errmsg);
        THROW_ERROR(999, errmsg);
    }

    // Return Success
    return setReturnDataIntScalar(idam_plugin_interface->data_block, OK_RETURN_VALUE, NULL);
}

static int do_endIdsPutNonTimed(IDAM_PLUGIN_INTERFACE* idam_plugin_interface, PLUGIN_ARGS plugin_args)
{
    if (!plugin_args.isClientIdx || !plugin_args.isPath) {
        UDA_LOG(UDA_LOG_ERROR, "The function parameters have not been specified!\n");
        THROW_ERROR(999, "");
    }

    if (mdsendIdsPutNonTimed(plugin_args.clientIdx, (char*)plugin_args.path) < 0) {
        UDA_LOG(UDA_LOG_ERROR, "%s\n", errmsg);
        THROW_ERROR(999, errmsg);
    }

    // Return Success
    return setReturnDataIntScalar(idam_plugin_interface->data_block, OK_RETURN_VALUE, NULL);
}

static int do_beginIdsReplaceLastSlice(IDAM_PLUGIN_INTERFACE* idam_plugin_interface, PLUGIN_ARGS plugin_args)
{
    if (!plugin_args.isClientIdx || !plugin_args.isPath) {
        UDA_LOG(UDA_LOG_ERROR, "The function parameters have not been specified!\n");
        THROW_ERROR(999, "");
    }

    if (mdsbeginIdsReplaceLastSlice(plugin_args.clientIdx, (char*)plugin_args.path) < 0) {
        UDA_LOG(UDA_LOG_ERROR, "%s\n", errmsg);
        THROW_ERROR(999, errmsg);
    }

    // Return Success
    return setReturnDataIntScalar(idam_plugin_interface->data_block, OK_RETURN_VALUE, NULL);
}

static int do_endIdsReplaceLastSlice(IDAM_PLUGIN_INTERFACE* idam_plugin_interface, PLUGIN_ARGS plugin_args)
{
    if (!plugin_args.isClientIdx || !plugin_args.isPath) {
        UDA_LOG(UDA_LOG_ERROR, "The function parameters have not been specified!\n");
        THROW_ERROR(999, "The function parameters have not been specified!");
    }

    if (mdsendIdsReplaceLastSlice(plugin_args.clientIdx, (char*)plugin_args.path) < 0) {
        UDA_LOG(UDA_LOG_ERROR, "imas endIdsReplaceLastSlice: %s\n", errmsg);
        THROW_ERROR(999, errmsg);
    }

    // Return Success
    return setReturnDataIntScalar(idam_plugin_interface->data_block, OK_RETURN_VALUE, NULL);
}

static int do_beginIdsPutTimed(IDAM_PLUGIN_INTERFACE* idam_plugin_interface, PLUGIN_ARGS plugin_args)
{
    if (!plugin_args.isClientIdx || !plugin_args.isPath || !plugin_args.isPutData) {
        UDA_LOG(UDA_LOG_ERROR, "The function parameters have not been specified!\n");
        THROW_ERROR(999, "The function parameters have not been specified!");
    }

    PUTDATA_BLOCK_LIST* putDataBlockList = &idam_plugin_interface->request_block->putDataBlockList;
    PUTDATA_BLOCK* putDataBlock = NULL;
    plugin_args.isPutData = (putDataBlockList != NULL && putDataBlockList->blockCount > 0);
    if (plugin_args.isPutData) {
        putDataBlock = &(putDataBlockList->putDataBlock[0]);
    }

// Time is passed in a single PUTDATA structure

    if (putDataBlock->data_type != UDA_TYPE_DOUBLE || putDataBlock->data == NULL) {
        UDA_LOG(UDA_LOG_ERROR, "No Valid Time Value have been specified!\n");
        THROW_ERROR(999, "No Valid Time Value have been specified!");
    }

    double* inTimes = (double*)putDataBlock->data;

    if (mdsbeginIdsPutTimed(plugin_args.clientIdx, (char*)plugin_args.path, putDataBlock->count, inTimes) < 0) {
        UDA_LOG(UDA_LOG_ERROR, "%s\n", errmsg);
        THROW_ERROR(999, errmsg);
    }

    // Return Success
    return setReturnDataIntScalar(idam_plugin_interface->data_block, OK_RETURN_VALUE, NULL);
}

static int do_endIdsPutTimed(IDAM_PLUGIN_INTERFACE* idam_plugin_interface, PLUGIN_ARGS plugin_args)
{
    if (!plugin_args.isClientIdx || !plugin_args.isPath) {
        UDA_LOG(UDA_LOG_ERROR, "imas endIdsPutTimed: The function parameters have not been specified!\n");
        THROW_ERROR(999, "The function parameters have not been specified!");
    }

    if (mdsendIdsPutTimed(plugin_args.clientIdx, (char*)plugin_args.path) < 0) {
        UDA_LOG(UDA_LOG_ERROR, "imas endIdsPutTimed: %s\n", errmsg);
        THROW_ERROR(999, errmsg);
    }

    // Return Success
    return setReturnDataIntScalar(idam_plugin_interface->data_block, OK_RETURN_VALUE, NULL);
}

//----------------------------------------------------------------------------------------
// Help: A Description of library functionality

static int do_help(IDAM_PLUGIN_INTERFACE* idam_plugin_interface, PLUGIN_ARGS plugin_args)
{
    const char* help = "\nimas_mds: Add Functions Names, Syntax, and Descriptions\n\n";
    const char* desc = "imas_mds: help = description of this plugin";

    return setReturnDataString(idam_plugin_interface->data_block, help, desc);
}

//----------------------------------------------------------------------------------------
// Standard methods: version, builddate, defaultmethod, maxinterfaceversion

static int do_version(IDAM_PLUGIN_INTERFACE* idam_plugin_interface, PLUGIN_ARGS plugin_args)
{
    return setReturnDataIntScalar(idam_plugin_interface->data_block, THISPLUGIN_VERSION, "Plugin version number");
}

//----------------------------------------------------------------------------------------
// Plugin Build Date

static int do_builddate(IDAM_PLUGIN_INTERFACE* idam_plugin_interface, PLUGIN_ARGS plugin_args)
{
    return setReturnDataString(idam_plugin_interface->data_block, __DATE__, "Plugin build date");
}

//----------------------------------------------------------------------------------------
// Plugin Default Method

static int do_defaultmethod(IDAM_PLUGIN_INTERFACE* idam_plugin_interface, PLUGIN_ARGS plugin_args)
{
    return setReturnDataString(idam_plugin_interface->data_block, THISPLUGIN_DEFAULT_METHOD, "Plugin default method");
}

//----------------------------------------------------------------------------------------
// Plugin Maximum Interface Version

static int do_maxinterfaceversion(IDAM_PLUGIN_INTERFACE* idam_plugin_interface, PLUGIN_ARGS plugin_args)
{
    return setReturnDataIntScalar(idam_plugin_interface->data_block, THISPLUGIN_MAX_INTERFACE_VERSION, "Maximum Interface Version");
}
