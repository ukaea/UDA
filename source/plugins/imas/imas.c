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

#include <client/udaClient.h>
#include <clientserver/compressDim.h>
#include <clientserver/errorLog.h>
#include <clientserver/freeDataBlock.h>
#include <clientserver/initStructs.h>
#include <clientserver/udaTypes.h>
#include <logging/logging.h>
#include <server/makeServerRequestBlock.h>
#include <server/managePluginFiles.h>
#include <server/serverPlugin.h>
#include <structures/genStructs.h>
#include <clientserver/stringUtils.h>
#include <clientserver/printStructs.h>
#include <client/makeClientRequestBlock.h>
#include <uda.h>

static int do_beginIdsPut(IDAM_PLUGIN_INTERFACE* idam_plugin_interface);
static int do_beginObject(IDAM_PLUGIN_INTERFACE* idam_plugin_interface);
static int do_builddate(IDAM_PLUGIN_INTERFACE* idam_plugin_interface);
static int do_close(IDAM_PLUGIN_INTERFACE* idam_plugin_interface);
static int do_create(IDAM_PLUGIN_INTERFACE* idam_plugin_interface, int* idx);
static int do_createModel(IDAM_PLUGIN_INTERFACE* idam_plugin_interface);
static int do_defaultmethod(IDAM_PLUGIN_INTERFACE* idam_plugin_interface);
static int do_delete(IDAM_PLUGIN_INTERFACE* idam_plugin_interface, int* idx);
static int do_endIdsPut(IDAM_PLUGIN_INTERFACE* idam_plugin_interface);
static int do_get(IDAM_PLUGIN_INTERFACE* idam_plugin_interface, int* idx);
static int do_getObject(IDAM_PLUGIN_INTERFACE* idam_plugin_interface);
static int do_getObjectDim(IDAM_PLUGIN_INTERFACE* idam_plugin_interface);
static int do_getObjectGroup(IDAM_PLUGIN_INTERFACE* idam_plugin_interface);
static int do_getObjectObject(IDAM_PLUGIN_INTERFACE* idam_plugin_interface);
static int do_getObjectSlice(IDAM_PLUGIN_INTERFACE* idam_plugin_interface);
static int do_help(IDAM_PLUGIN_INTERFACE* idam_plugin_interface);
static int do_maxinterfaceversion(IDAM_PLUGIN_INTERFACE* idam_plugin_interface);
static int do_open(IDAM_PLUGIN_INTERFACE* idam_plugin_interface, int* idx);
static int do_put(IDAM_PLUGIN_INTERFACE* idam_plugin_interface, int* idx);
static int do_putIdsVersion(IDAM_PLUGIN_INTERFACE* idam_plugin_interface);
static int do_putObject(IDAM_PLUGIN_INTERFACE* idam_plugin_interface);
static int do_putObjectInObject(IDAM_PLUGIN_INTERFACE* idam_plugin_interface);
static int do_releaseObject(IDAM_PLUGIN_INTERFACE* idam_plugin_interface);
static int do_source(IDAM_PLUGIN_INTERFACE* idam_plugin_interface, int* time_count_cache, char* time_cache, char* data_cache);
static int do_version(IDAM_PLUGIN_INTERFACE* idam_plugin_interface);

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
    if (initLocalObjs) {
        initLocalObj();
    }
    localObjs[lastObjectId] = dataObj;
    return lastObjectId++;
}

static void* findLocalObj(int refId)
{
    if (initLocalObjs)initLocalObj();
    void* obj = NULL;
    if (refId < lastObjectId) {
        return localObjs[refId];
    }
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

static int sliceIdx1;
static int sliceIdx2;
static double sliceTime1;
static double sliceTime2;

void setSliceIdx(int idx1, int idx2)
{
    sliceIdx1 = idx1;
    sliceIdx2 = idx2;
}

int getSliceIdx1()
{
    return sliceIdx1;
}

int getSliceIdx2()
{
    return sliceIdx2;
}

void setSliceTime(double time1, double time2)
{
    sliceTime1 = time1;
    sliceTime2 = time1;
}

double getSliceTime1()
{
    return sliceTime1;
}

double getSliceTime2()
{
    return sliceTime2;
}

/**
 * Convert name to IMAS type
 * @param typeName
 * @return
 */
int findIMASType(const char* typeName)
{
    if (typeName == NULL)                       return UNKNOWN_TYPE;
    else if (!strcasecmp(typeName, "int"))      return INT;
    else if (!strcasecmp(typeName, "float"))    return FLOAT;
    else if (!strcasecmp(typeName, "double"))   return DOUBLE;
    else if (!strcasecmp(typeName, "string"))   return STRING;
    else return UNKNOWN_TYPE;
}

/**
 * Convert IMAS type to IDAM type
 * @param type
 * @return
 */
int findIMASIDAMType(int type)
{
    switch (type) {
        case INT:           return TYPE_INT;
        case FLOAT:         return TYPE_FLOAT;
        case DOUBLE:        return TYPE_DOUBLE;
        case STRING:        return TYPE_STRING;
        case STRING_VECTOR: return TYPE_STRING;
        default:            return TYPE_UNKNOWN;
    }
    return TYPE_UNKNOWN;
}

extern int imas(IDAM_PLUGIN_INTERFACE* idam_plugin_interface)
{
    idamSetLogLevel(LOG_DEBUG);

    int err = 0;

    //----------------------------------------------------------------------------------------
    // State Variables

    static short init = 0;
    static int idx = 0;                // Last Opened File Index value

    static int time_count_cache = 0;
    static char* time_cache = NULL;
    static char* data_cache = NULL;

    static IDAMPLUGINFILELIST* pluginFileList = NULL;

    unsigned short housekeeping;

    if (idam_plugin_interface->interfaceVersion > THISPLUGIN_MAX_INTERFACE_VERSION) {
        IDAM_LOG(LOG_ERROR, "Plugin Interface Version Unknown to this plugin: Unable to execute the request!\n");
        THROW_ERROR(999, "Plugin Interface Version Unknown to this plugin: Unable to execute the request!");
    }

    idam_plugin_interface->pluginVersion = THISPLUGIN_VERSION;

    REQUEST_BLOCK* request_block = idam_plugin_interface->request_block;

    housekeeping = idam_plugin_interface->housekeeping;

    if (housekeeping || !strcasecmp(request_block->function, "reset")) {

        if (!init) return 0;        // Not previously initialised: Nothing to do!

        // Free Heap & reset counters

        closeIdamPluginFiles(pluginFileList);   // Close all open files
        initHdf5File();                         // Reset the File Index array

        if (time_count_cache != 0) {
            time_count_cache = 0;
            free((void*)time_cache);
            free((void*)data_cache);
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

        time_count_cache = 0;
        time_cache = NULL;
        data_cache = NULL;

        init = 1;
        if (!strcasecmp(request_block->function, "init") || !strcasecmp(request_block->function, "initialise")) {
            return 0;
        }
    }

#ifndef MDSSKIP
    //----------------------------------------------------------------------------------------
    // Forced redirection

    if (getenv("UDA_NOHDF5_IMAS_PLUGIN") != NULL || findValue(&request_block->nameValueList, "imas_mds")) {
        return imas_mds(idam_plugin_interface);
    }
#endif

    //----------------------------------------------------------------------------------------
    // Plugin Functions
    //----------------------------------------------------------------------------------------

    if (!strcasecmp(request_block->function, "putIdsVersion")) {
        err = do_putIdsVersion(idam_plugin_interface);
    } else if (!strcasecmp(request_block->function, "source")) {
        err = do_source(idam_plugin_interface, &time_count_cache, time_cache, data_cache);
    } else if (!strcasecmp(request_block->function, "delete")) {
        err = do_delete(idam_plugin_interface, &idx);
    } else if (!strcasecmp(request_block->function, "get")) {
        err = do_get(idam_plugin_interface, &idx);
    } else if (!strcasecmp(request_block->function, "put")) {
        err = do_put(idam_plugin_interface, &idx);
    } else if (!strcasecmp(request_block->function, "open")) {
        err = do_open(idam_plugin_interface, &idx);
    } else if (!strcasecmp(request_block->function, "create")) {
        err = do_create(idam_plugin_interface, &idx);
    } else if (!strcasecmp(request_block->function, "close")) {
        err = do_close(idam_plugin_interface);
    } else if (!strcasecmp(request_block->function, "createModel")) {
        err = do_createModel(idam_plugin_interface);
    } else if (!strcasecmp(request_block->function, "releaseObject") ||
        !strcasecmp(request_block->function, "putObjectGroup") ||
        !strcasecmp(request_block->function, "putObjectSlice") ||
        !strcasecmp(request_block->function, "replaceLastObjectSlice")) {
        err = do_releaseObject(idam_plugin_interface);
    } else if (!strcasecmp(request_block->function, "getObjectObject")) {
        err = do_getObjectObject(idam_plugin_interface);
    } else if (!strcasecmp(request_block->function, "getObjectSlice")) {
        err = do_getObjectSlice(idam_plugin_interface);
    } else if (!strcasecmp(request_block->function, "getObjectGroup")) {
        err = do_getObjectGroup(idam_plugin_interface);
    } else if (!strcasecmp(request_block->function, "getObjectDim")) {
        err = do_getObjectDim(idam_plugin_interface);
    } else if (!strcasecmp(request_block->function, "beginObject")) {
        err = do_beginObject(idam_plugin_interface);
    } else if (!strcasecmp(request_block->function, "getObject")) {
        err = do_getObject(idam_plugin_interface);
    } else if (!strcasecmp(request_block->function, "putObject")) {
        err = do_putObject(idam_plugin_interface);
    } else if (!strcasecmp(request_block->function, "putObjectInObject")) {
        err = do_putObjectInObject(idam_plugin_interface);
    } else if (!strcasecmp(request_block->function, "beginIdsPut")) {
        err = do_beginIdsPut(idam_plugin_interface);
    } else if (!strcasecmp(request_block->function, "endIdsPut")) {
        err = do_endIdsPut(idam_plugin_interface);
    } else if (!strcasecmp(request_block->function, "help")) {
        err = do_help(idam_plugin_interface);
    } else if (!strcasecmp(request_block->function, "version")) {
        err = do_version(idam_plugin_interface);
    } else if (!strcasecmp(request_block->function, "builddate")) {
        err = do_builddate(idam_plugin_interface);
    } else if (!strcasecmp(request_block->function, "defaultmethod")) {
        err = do_defaultmethod(idam_plugin_interface);
    } else if (!strcasecmp(request_block->function, "maxinterfaceversion")) {
        err = do_maxinterfaceversion(idam_plugin_interface);
    } else {
        THROW_ERROR(999, "Unknown function requested!");
    }

//--------------------------------------------------------------------------------------
// Housekeeping

    return err;
}

static int do_putIdsVersion(IDAM_PLUGIN_INTERFACE* idam_plugin_interface)
{
    int err = 0;
    DATA_BLOCK* data_block = idam_plugin_interface->data_block;

    char* imasIdsDevice;
    FIND_REQUIRED_STRING_VALUE(idam_plugin_interface->request_block->nameValueList, imasIdsDevice);

    char* imasIdsVersion;
    FIND_REQUIRED_STRING_VALUE(idam_plugin_interface->request_block->nameValueList, imasIdsVersion);

    putImasIdsVersion(imasIdsVersion);
    putImasIdsDevice(imasIdsDevice);

    // Return the Status OK

    int* data = (int*)malloc(sizeof(int));
    data[0] = 1;
    initDataBlock(data_block);
    data_block->rank = 0;
    data_block->dims = NULL;
    data_block->data_type = TYPE_INT;
    data_block->data = (char*)data;
    data_block->data_n = 1;

    return err;
}

static int do_delete(IDAM_PLUGIN_INTERFACE* idam_plugin_interface, int* idx)
{
    int err = 0;
    DATA_BLOCK* data_block = idam_plugin_interface->data_block;

    int rc = 0;
    int isOpen = 0;

    // What is the status of the data file

    IDAMPLUGINFILELIST* pluginFileList = getImasPluginFileList();

    int clientIdx;
    bool isClientIdx = FIND_INT_VALUE(idam_plugin_interface->request_block->nameValueList, clientIdx);

    char* filename;
    bool isFileName = FIND_STRING_VALUE(idam_plugin_interface->request_block->nameValueList, filename);

    int shotNumber;
    bool isShotNumber = FIND_INT_VALUE(idam_plugin_interface->request_block->nameValueList, shotNumber);

    int runNumber;
    bool isRunNumber = FIND_INT_VALUE(idam_plugin_interface->request_block->nameValueList, runNumber);

    if (isClientIdx) {
        int fid = checkHdf5Idx(clientIdx);
        if (fid >= 0) {
            if (pluginFileList->files[fid].status) {
                isOpen = 1;
            }
        } else {
            if (!isFileName && !isShotNumber && !isRunNumber) {
                IDAM_LOG(LOG_ERROR, "No registered data file identified!\n");
                THROW_ERROR(999, "No registered data file identified!");
            }
        }
    } else if (isFileName && isShotNumber && isRunNumber) {
        char* file = getHdf5FileName(filename, shotNumber, runNumber);      // Standard Name design pattern
        int fid = findIdamPluginFileByName(pluginFileList, file);           // Check the IDAM log
        if (fid >= 0) {
            if (pluginFileList->files[fid].status) {
                if ((clientIdx = findHdf5Idx(fid)) < 0) {
                    IDAM_LOG(LOG_ERROR, "Unable to Locate the necessary data file!\n");
                    THROW_ERROR(999, "Unable to Locate the necessary data file!");
                }
                // File is Open
                isOpen = 1;
            }
        }
    } else {
        IDAM_LOG(LOG_ERROR, "No data file identified!\n");
        THROW_ERROR(999, "No data file identified!");
    }

    // Open the Data file

    if (!isOpen) {
        do_open(idam_plugin_interface, idx);

        if (idam_plugin_interface->data_block->data == NULL) {
            clientIdx = -1;
        } else {
            clientIdx = *(int*)idam_plugin_interface->data_block->data;
        }

        freeDataBlock(idam_plugin_interface->data_block);

        if (clientIdx < 0) {
            IDAM_LOG(LOG_ERROR, "imas delete: Unable to Open the necessary data file!\n");
            THROW_ERROR(999, "Unable to Open the necessary data file!");
        }
    }

    char* path;
    FIND_STRING_VALUE(idam_plugin_interface->request_block->nameValueList, path);

    char* CPOPath;
    FIND_STRING_VALUE(idam_plugin_interface->request_block->nameValueList, CPOPath);

    rc = imas_hdf5_DeleteData(clientIdx, CPOPath, path);

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

    return err;
}

static int do_get(IDAM_PLUGIN_INTERFACE* idam_plugin_interface, int* idx)
{
    int err = 0;
    DATA_BLOCK* data_block = idam_plugin_interface->data_block;

/*
idx	- reference to the open data file: file handle from an array of open files - hdf5Files[idx]
cpoPath	- the root group where the CPO/IDS is written
path	- the path relative to the root (cpoPath) where the data are written (must include the variable name!)
*/

    IDAMPLUGINFILELIST* pluginFileList = getImasPluginFileList();

    int clientIdx;
    bool isClientIdx = FIND_INT_VALUE(idam_plugin_interface->request_block->nameValueList, clientIdx);

    char* filename;
    bool isFileName = FIND_STRING_VALUE(idam_plugin_interface->request_block->nameValueList, filename);

    int shotNumber;
    bool isShotNumber = FIND_INT_VALUE(idam_plugin_interface->request_block->nameValueList, shotNumber);

    int runNumber;
    bool isRunNumber = FIND_INT_VALUE(idam_plugin_interface->request_block->nameValueList, runNumber);

    // What is the status of the data file

    bool isOpen = FALSE;

    if (isClientIdx) {
        int fid = checkHdf5Idx(clientIdx);
        if (fid >= 0) {
            if (pluginFileList->files[fid].status) {
                isOpen = TRUE;
            }
        } else {
            if (!(isFileName && isShotNumber && isRunNumber)) {
                IDAM_LOG(LOG_ERROR, "No registered data file identified!\n");
                THROW_ERROR(999, "No registered data file identified!");
            }
        }
    } else if (isFileName && isShotNumber && isRunNumber) {
        char* file = getHdf5FileName(filename, shotNumber, runNumber);    // Standard Name design pattern
        int fid = findIdamPluginFileByName(pluginFileList, file);       // Check the IDAM log
        if (fid >= 0) {
            if (pluginFileList->files[fid].status) {
                if ((clientIdx = findHdf5Idx(fid)) < 0) {
                    IDAM_LOG(LOG_ERROR, "Unable to Locate the necessary data file!\n");
                    THROW_ERROR(999, "Unable to Locate the necessary data file!");
                }
                // File is Open
                isOpen = TRUE;
            }
        }
    } else {
        IDAM_LOG(LOG_ERROR, "No data file identified!\n");
        THROW_ERROR(999, "No data file identified!");
    }

    // Open the Data file

    if (!isOpen) {
        // Recursive call to open/create the file
        err = do_open(idam_plugin_interface, idx);

        if (idam_plugin_interface->data_block->data == NULL) {
            clientIdx = -1;
        } else {
            clientIdx = *(int*)idam_plugin_interface->data_block->data;
        }

        freeDataBlock(idam_plugin_interface->data_block);

        if (clientIdx < 0) {
            IDAM_LOG(LOG_ERROR, "Unable to Open the necessary data file!\n");
            THROW_ERROR(999, "Unable to Open the necessary data file!");
        }
    }

    // Convert type string into IMAS type identifiers

    bool isPutData = (bool)(idam_plugin_interface->request_block->putDataBlockList.blockCount > 0);

    bool isGetDimension = findValue(&idam_plugin_interface->request_block->nameValueList, "getDimension");
    bool isGetDataSlice = findValue(&idam_plugin_interface->request_block->nameValueList, "getDataSlice");

    char* typeName;
    bool isTypeName = FIND_STRING_VALUE(idam_plugin_interface->request_block->nameValueList, typeName);

    int type = TYPE_UNKNOWN;

    if (!isGetDimension) {
        if (!isTypeName && !isPutData) {
            IDAM_LOG(LOG_ERROR, "imas get: The data's Type has not been specified!\n");
            THROW_ERROR(999, "The data's Type has not been specified!");
        }

        if ((type = findIMASType(typeName)) == UNKNOWN_TYPE) {
            IDAM_LOG(LOG_ERROR, "imas get: The data's Type name cannot be converted!\n");
            THROW_ERROR(999, "The data's Type name cannot be converted!");
        }
    }

    int rank;
    bool isRank = FIND_INT_VALUE(idam_plugin_interface->request_block->nameValueList, rank);

    if (!isPutData && !isRank && !isGetDimension) {
        IDAM_LOG(LOG_ERROR, "imas get: The data's Rank has not been specified!\n");
        THROW_ERROR(999, "The data's Rank has not been specified!");
    }

    if (isGetDimension) rank = 7;

    int* shape = (int*)malloc((rank + 1) * sizeof(int));
    shape[0] = 1;

    int i;
    for (i = 1; i < rank; i++) {
        shape[i] = 0;
    }

    // Which Data Operation?

    int dataOperation = GET_OPERATION;
    if (isGetDataSlice) { dataOperation = GETSLICE_OPERATION; }
    else if (isGetDimension) dataOperation = GETDIMENSION_OPERATION;

    // GET and return the data

    char* imasData = NULL;
    double retTime = 0.0;
    initDataBlock(data_block);

    char* CPOPath;
    char* path;
    FIND_REQUIRED_STRING_VALUE(idam_plugin_interface->request_block->nameValueList, CPOPath);
    FIND_REQUIRED_STRING_VALUE(idam_plugin_interface->request_block->nameValueList, path);

    int rc = 0;

    if (dataOperation == GET_OPERATION) {
        rc = imas_hdf5_getData(clientIdx, CPOPath, path, type, rank, shape, &imasData);

        if (rc != 0) {
            // data not in IDS - go to other plugins to try and get it

            char* expName = NULL;
            FIND_REQUIRED_STRING_VALUE(idam_plugin_interface->request_block->nameValueList, expName);

            int shot;
            FIND_REQUIRED_INT_VALUE(idam_plugin_interface->request_block->nameValueList, shot);

            char* signal = FormatString("%s/%s", CPOPath, path);
            char* source = FormatString("%d", shot);

            IDAM_LOGF(LOG_DEBUG, "Signal: %s\n", signal);
            IDAM_LOGF(LOG_DEBUG, "Source: %s\n", source);

            rc = idamGetAPI(signal, source);

            IDAM_LOGF(LOG_DEBUG, "Handle: %d\n", rc);

            if (rc >= 0) {
                imasData = getIdamData(rc);
                rank = 1;
                shape[0] = getIdamDataNum(rc);
                IDAM_LOGF(LOG_DEBUG, "Shape: %d\n", shape[0]);
            }
        }
    } else if (dataOperation == GETSLICE_OPERATION) {

        PUTDATA_BLOCK* putDataBlock = isPutData
                                      ? &idam_plugin_interface->request_block->putDataBlockList.putDataBlock[0]
                                      : NULL;

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

        int index;
        FIND_REQUIRED_INT_VALUE(idam_plugin_interface->request_block->nameValueList, index);

        int index2;
        bool isIndex2 = FIND_INT_VALUE(idam_plugin_interface->request_block->nameValueList, index2);

        if (!isIndex2 || index2 == -1) {
            //Only a single sample
            rc = imas_hdf5_getDataSlices(clientIdx, CPOPath, path, type, rank, shape, index, 1, &currData);
            retTime = sliceTime1;
        } else {
            rc = imas_hdf5_getDataSlices(clientIdx, CPOPath, path, type, rank, shape, index, 2, &currData);

            short interpolMode;
            FIND_SHORT_VALUE(idam_plugin_interface->request_block->nameValueList, interpolMode);

            if (!rc) {    // No Error
                double dt = 1.0;
                unsigned int leftSlicePoint = 0;
                switch (interpolMode) {
                    case INTERPOLATION:
                        dt = (time - sliceTime1) / (sliceTime2 - sliceTime1);
                        retTime = time;
                        break;
                    case CLOSEST_SAMPLE:
                        leftSlicePoint = (unsigned int)(time - sliceTime1 < sliceTime2 - time);
                        if (leftSlicePoint) {
                            retTime = sliceTime1;
                        } else {
                            retTime = sliceTime2;
                        }
                        break;
                    case PREVIOUS_SAMPLE:
                        retTime = sliceTime1;
                        break;
                    default:
                        THROW_ERROR(999, "unknown interpolation type");
                }
                if (rank == 0) {
                    switch (type) {
                        case INT: {
                            int y1 = ((int*)currData)[0];
                            int y2 = ((int*)currData)[1];
                            int* retData = (int*)malloc(sizeof(int));
                            switch (interpolMode) {
                                case INTERPOLATION:
                                    retData[0] = (int)(y1 + (y2 - y1) * dt);
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
                                default:
                                    THROW_ERROR(999, "unknown interpolation type");
                            }
                            imasData = (char*)retData;
                            break;
                        }
                        case FLOAT: {
                            float y1 = ((float*)currData)[0];
                            float y2 = ((float*)currData)[1];
                            float* retData = (float*)malloc(sizeof(float));
                            switch (interpolMode) {
                                case INTERPOLATION:
                                    retData[0] = (float)(y1 + (y2 - y1) * dt);
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
                                default:
                                    THROW_ERROR(999, "unknown interpolation type");
                            }
                            imasData = (char*)retData;
                            break;
                        }
                        case DOUBLE: {
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
                                default:
                                    THROW_ERROR(999, "unknown interpolation type");
                            }
                            imasData = (char*)retData;
                            break;
                        }
                        default:
                            THROW_ERROR(999, "unknown data type");
                    }
                } else if (rank >= 1) {
                    nItems = shape[0];
                    for (i = 1; i < rank; i++)nItems *= shape[i];
                    switch (type) {
                        case INT: {
                            int* y1 = (int*)currData;
                            int* y2 = (int*)&currData[1];
                            int* retData = (int*)malloc(sizeof(int) * nItems);
                            for (i = 0; i < nItems; i++) {
                                switch (interpolMode) {
                                    case INTERPOLATION:
                                        retData[i] = (int)(y1[2 * i] + (y2[2 * i] - y1[2 * i]) * dt);
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
                                    default:
                                        THROW_ERROR(999, "unknown interpolation type");
                                }
                            }
                            imasData = (char*)retData;
                            break;
                        }
                        case FLOAT: {
                            float* y1 = (float*)currData;
                            float* y2 = (float*)&currData[1];
                            float* retData = (float*)malloc(sizeof(float) * nItems);
                            for (i = 0; i < nItems; i++) {
                                switch (interpolMode) {
                                    case INTERPOLATION:
                                        retData[i] = (float)(y1[2 * i] + (y2[2 * i] - y1[2 * i]) * dt);
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
                                    default:
                                        THROW_ERROR(999, "unknown interpolation type");
                                }
                            }
                            imasData = (char*)retData;
                            break;
                        }
                        case DOUBLE: {
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
                                    default:
                                        THROW_ERROR(999, "unknown interpolation type");
                                }
                            }
                            imasData = (char*)retData;
                            break;
                        }
                        default:
                            THROW_ERROR(999, "unknown data type");
                    }

                }    // rank >= 1
            }    // No Error
        }    // 2 indices passed

    } else if (dataOperation == GETDIMENSION_OPERATION) {
        rc = imas_hdf5_GetDimension(clientIdx, CPOPath, path, &rank, &shape[0], &shape[1], &shape[2], &shape[3],
                                   &shape[4], &shape[5], &shape[6]);
    }

    if (rc < 0 || (!isGetDimension && imasData == NULL)) {
        free(shape);
        THROW_ERROR(999, "Data GET method failed!");
    }

    // Return Data

    switch (dataOperation) {
        case GETDIMENSION_OPERATION: {
            data_block->rank = 1;
            data_block->data_type = TYPE_INT;
            data_block->data = (char*)shape;
            data_block->data_n = rank;
            break;
        }
        case GET_OPERATION: {
            data_block->rank = (unsigned int)rank;
            data_block->data_type = findIMASIDAMType(type);
            data_block->data = imasData;
            if (data_block->data_type == TYPE_STRING && rank <= 1) {
                data_block->data_n = (int)strlen(imasData) + 1;
                shape[0] = data_block->data_n;
            } else {
                data_block->data_n = shape[0];
                for (i = 1; i < rank; i++) {
                    data_block->data_n *= shape[i];
                }
            }
            break;
        }
        case GETSLICE_OPERATION: {        // Need to return Data as well as the time - increase rank by 1 and pass in a
            // coordinate array of length 1
            data_block->rank = (unsigned int)rank;
            data_block->data_type = findIMASIDAMType(type);
            data_block->data = imasData;
            data_block->data_n = shape[0];
            for (i = 1; i < rank; i++) {
                data_block->data_n *= shape[i];
            }
            break;
        }
        default:
            THROW_ERROR(999, "unknown data operation");
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
            data_block->rank = (unsigned int)rank + 1;
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

    return err;
}

static int do_put(IDAM_PLUGIN_INTERFACE* idam_plugin_interface, int* idx)
{
    int err = 0;
    DATA_BLOCK* data_block = idam_plugin_interface->data_block;

    int rc = 0;
    int fid;
    int fileStatus = 0;
    int dataRank;
    int* shape = NULL;
    int isOpen = 0, isShape = 0, isData = 0, isType = 0;
    int shapeCount = 0, dataCount = 0;
    int type, idamType;
    char* file = NULL;

    int isVarData = 0;
    short varDataIndex = -1;

    IDAMPLUGINFILELIST* pluginFileList = getImasPluginFileList();

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

    int clientIdx;
    bool isClientIdx = FIND_INT_VALUE(idam_plugin_interface->request_block->nameValueList, clientIdx);

    char* filename;
    bool isFileName = FIND_STRING_VALUE(idam_plugin_interface->request_block->nameValueList, filename);

    int shotNumber;
    bool isShotNumber = FIND_INT_VALUE(idam_plugin_interface->request_block->nameValueList, shotNumber);

    int runNumber;
    bool isRunNumber = FIND_INT_VALUE(idam_plugin_interface->request_block->nameValueList, runNumber);

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
        }
    } else if (isFileName && isShotNumber && isRunNumber) {
        file = getHdf5FileName(filename, shotNumber, runNumber);    // Standard Name design pattern
        fid = findIdamPluginFileByName(pluginFileList, file);       // Check the IDAM log
        if (fid >= 0) {
            if (pluginFileList->files[fid].status) {
                fileStatus = FILEISOPEN;
                if ((clientIdx = findHdf5Idx(fid)) < 0) {
                    IDAM_LOG(LOG_ERROR, "Unable to Locate the necessary data file!\n");
                    THROW_ERROR(999, "Unable to Locate the necessary data file!");
                }
                // File is Open
                isOpen = 1;
            } else {
                fileStatus = FILEISCLOSED;
            }
        } else {
            fileStatus = FILEISNEW;
        }
    } else {
        IDAM_LOG(LOG_ERROR, "No data file identified!\n");
        THROW_ERROR(999, "No data file identified!");
    }

// Open or Create the Data file

    if (!isOpen) {
        if (fileStatus == FILEISNEW) {
            err = do_create(idam_plugin_interface, idx);
        } else {
            err = do_open(idam_plugin_interface, idx);
        }

        if (idam_plugin_interface->data_block->data == NULL) {
            clientIdx = -1;
        } else {
            clientIdx = *(int*)idam_plugin_interface->data_block->data;
        }

        freeDataBlock(idam_plugin_interface->data_block);

        if (clientIdx < 0) {
            IDAM_LOG(LOG_ERROR, "Unable to Open/Create the necessary data file!\n");
            THROW_ERROR(999, "Unable to Open/Create the necessary data file!");
        }
    }

    // Has a PUTDATA block been passed with a missing or a matching name?

    bool isPutData = (bool)(idam_plugin_interface->request_block->putDataBlockList.blockCount > 0);
    void* putdata = NULL;

    if (isPutData) {
        PUTDATA_BLOCK_LIST* putDataBlockList = &idam_plugin_interface->request_block->putDataBlockList;

        char* path;
        bool isPath = FIND_STRING_VALUE(idam_plugin_interface->request_block->nameValueList, path);

        int i;
        for (i = 0; i < putDataBlockList->blockCount; i++) {

            if (putDataBlockList->putDataBlock[i].blockName == NULL) {
                if (putDataBlockList->blockCount > 1) {            // Multiple blocks must be named
                    IDAM_LOG(LOG_ERROR, "Multiple un-named data items - ambiguous!\n");
                    THROW_ERROR(999, "Multiple un-named data items - ambiguous!");
                }
                putdata = (void*)putDataBlockList->putDataBlock[i].data;    // Only 1 unnamed block
                varDataIndex = (short)i;
                break;
            } else if (!strcasecmp(putDataBlockList->putDataBlock[i].blockName, "variable") ||
                       !strcasecmp(putDataBlockList->putDataBlock[i].blockName, "data") ||
                       (isPath && !strcasecmp(putDataBlockList->putDataBlock[i].blockName, path))) {
                putdata = (void*)putDataBlockList->putDataBlock[i].data;
                varDataIndex = (short)i;
                break;
            }
        }
        for (i = 0; i < putDataBlockList->blockCount; i++) {
            if (putDataBlockList->putDataBlock[i].blockName != NULL && (
                    !strcasecmp(putDataBlockList->putDataBlock[i].blockName, "putDataTime") ||
                    !strcasecmp(putDataBlockList->putDataBlock[i].blockName, "time"))) {
//                putDataSliceTime = ((double*)putDataBlockList->putDataBlock[i].data)[0];
//                isTime = 1;
//                isPutDataSliceTime = 1;
                break;
            }
        }

        if (varDataIndex < 0 && putDataBlockList->blockCount == 1 &&
            (putDataBlockList->putDataBlock[0].blockName == NULL ||
             putDataBlockList->putDataBlock[0].blockName[0] == '\0')) {
            putdata = (void*)putDataBlockList->putDataBlock[0].data;
            varDataIndex = 0;
        }
        if (varDataIndex < 0) {
            IDAM_LOG(LOG_ERROR, "Unable to Identify the data to PUT!\n");
            THROW_ERROR(999, "Unable to Identify the data to PUT!");
        }

        isVarData = 1;
        PUTDATA_BLOCK* putDataBlock = &putDataBlockList->putDataBlock[varDataIndex];

        if ((type = findIMASType(convertIdam2StringType(putDataBlock->data_type))) == 0) {
            // Convert an IDAM type to an IMAS type
            IDAM_LOG(LOG_ERROR, "The data's Type cannot be converted!\n");
            THROW_ERROR(999, "The data's Type cannot be converted!");
        }

    } else {
        // Convert type string into IMAS type identifiers
        char* typeName;
        FIND_REQUIRED_STRING_VALUE(idam_plugin_interface->request_block->nameValueList, typeName);

        if ((type = findIMASType(typeName)) == UNKNOWN_TYPE) {
            IDAM_LOG(LOG_ERROR, "The data's Type name cannot be converted!\n");
            THROW_ERROR(999, "The data's Type name cannot be converted!");
        }
    }

    isType = 1;

// Any Data?

    char* dataString;
    bool isDataString = FIND_STRING_VALUE(idam_plugin_interface->request_block->nameValueList, dataString);

    if (!isDataString && !isVarData) {
        IDAM_LOG(LOG_ERROR, "No data has been specified!\n");
        THROW_ERROR(999, "No data has been specified!");
    }

// Which Data Operation?
    bool isPutDataSlice = findValue(&idam_plugin_interface->request_block->nameValueList, "putDataSlice");
    bool isReplaceLastDataSlice = findValue(&idam_plugin_interface->request_block->nameValueList, "replaceLastDataSlice");

    int dataOperation = PUT_OPERATION;
    if (isPutDataSlice) {
        dataOperation = PUTSLICE_OPERATION;
    } else if (isReplaceLastDataSlice) {
        dataOperation = REPLACELASTSLICE_OPERATION;
    }

// Convert Name-Value string arrays (shape, data) to numerical arrays of the correct type

// *** use same naming convention as netcdf4 put
// *** re-use netcdf4 put functions: getCDF4VarArray generalised to getIdamNameValuePairVarArray
// *** need new functions: convert from named type to HDF5 type

    int rank;
    bool isRank = FIND_INT_VALUE(idam_plugin_interface->request_block->nameValueList, rank);

    char* shapeString;
    bool isShapeString = FIND_STRING_VALUE(idam_plugin_interface->request_block->nameValueList, shapeString);

    if (!isPutData) {
        if (!isRank && !isShapeString) {        // Assume a scalar value
            isRank = 1;
            isShape = 1;
            rank = 0;
            shape = (int*)malloc(sizeof(int));
            shape[0] = 1;
            shapeCount = 1;
        }

        char quote = '"';
        if (findValue(&idam_plugin_interface->request_block->nameValueList, "singleQuote")) {
            quote = '\'';
        }

        char delimiter = ',';
        FIND_CHAR_VALUE(idam_plugin_interface->request_block->nameValueList, delimiter);

        if (isShapeString) {
            if ((dataRank = getIdamNameValuePairVarArray(shapeString, quote, delimiter, (unsigned short)rank,
                                                         TYPE_INT, (void**)&shape)) < 0) {
                IDAM_LOG(LOG_ERROR, "Unable to convert the passed shape values!\n");
                THROW_ERROR(-dataRank, "Unable to convert the passed shape value!");
            }
            if (isRank && rank != dataRank) {
                IDAM_LOG(LOG_ERROR, "The passed rank is inconsistent with the passed shape data!\n");
                THROW_ERROR(999, "The passed rank is inconsistent with the passed shape data!");
            }
            isShape = 1;
            isRank = 1;
            rank = dataRank;

            shapeCount = shape[0];

            int i;
            for (i = 1; i < rank; i++) {
                shapeCount = shapeCount * shape[i];
            }
        }    // isShapeString (from name-value pair)

        if (isDataString) {
            char* typeName;
            FIND_REQUIRED_STRING_VALUE(idam_plugin_interface->request_block->nameValueList, typeName);

            if ((idamType = findIdamType(typeName)) == TYPE_UNDEFINED) {
                IDAM_LOG(LOG_ERROR, "The data's Type name cannot be converted!\n");
                THROW_ERROR(999, "The data's Type name cannot be converted!");
            }

            if ((dataCount = getIdamNameValuePairVarArray(dataString, quote, delimiter, (unsigned short)shapeCount,
                                                          idamType, &putdata)) < 0) {
                IDAM_LOG(LOG_ERROR, "Unable to convert the passed data values!\n");
                THROW_ERROR(-dataCount, "Unable to convert the passed data value!");
            }
            if (shapeCount != 0 && shapeCount != dataCount) {
                IDAM_LOG(LOG_ERROR, "Inconsistent count of Data items!\n");
                THROW_ERROR(999, "Inconsistent count of Data items!");
            }
            isData = 1;
        }    // isDataString (from name-value pair)


// Test all required data is available

        if (!isType || !isRank || !isShape || !isData) {
            IDAM_LOG(LOG_ERROR, "Insufficient data parameters passed - put not possible!\n");
            THROW_ERROR(999, "Insufficient data parameters passed - put not possible!");
        }

        char* path;
        FIND_REQUIRED_STRING_VALUE(idam_plugin_interface->request_block->nameValueList, path);

        char* CPOPath;
        FIND_REQUIRED_STRING_VALUE(idam_plugin_interface->request_block->nameValueList, CPOPath);

// The type passed here is the IMAS type enumeration (imas_putData is the original imas putData function)

        if (dataOperation == PUT_OPERATION) {
            bool isTimed = findValue(&idam_plugin_interface->request_block->nameValueList, "timed");

            rc = imas_hdf5_putData(clientIdx, CPOPath, path, type, rank, shape, isTimed, putdata);
        } else {
            if (dataOperation == PUTSLICE_OPERATION) {
                IDAM_LOG(LOG_ERROR, "Slice Time not passed!\n");
                THROW_ERROR(999, "Slice Time not passed!");
            } else {
                rc = imas_hdf5_putDataX(clientIdx, CPOPath, path, type, rank, shape, dataOperation, putdata, 0.0);
            }    // Replace Last Slice
        }
        if (rc < 0) err = 999;

// Housekeeping heap

        free((void*)shape);
        free(putdata);

    } else {        // !isPutData
        char* path;
        FIND_REQUIRED_STRING_VALUE(idam_plugin_interface->request_block->nameValueList, path);

        char* CPOPath;
        FIND_REQUIRED_STRING_VALUE(idam_plugin_interface->request_block->nameValueList, CPOPath);

        PUTDATA_BLOCK_LIST* putDataBlockList = &idam_plugin_interface->request_block->putDataBlockList;
        PUTDATA_BLOCK* putDataBlock = &putDataBlockList->putDataBlock[0];

        int freeShape = 0;
        if (putDataBlock->shape == NULL) {
            if (putDataBlock->rank > 1) {
                IDAM_LOG(LOG_ERROR, "No shape information passed!\n");
                THROW_ERROR(999, "No shape information passed!");
            }

            putDataBlock->shape = (int*)malloc(sizeof(int));
            putDataBlock->shape[0] = putDataBlock->count;
            freeShape = 1;
        }

        if (dataOperation == PUT_OPERATION) {
            bool isTimed = findValue(&idam_plugin_interface->request_block->nameValueList, "timed");

            rc = imas_hdf5_putData(clientIdx, CPOPath, path, type, putDataBlock->rank, putDataBlock->shape, isTimed,
                              (void*)putDataBlock->data);
        } else {
            if (dataOperation == PUTSLICE_OPERATION) {
                PUTDATA_BLOCK* putTimeBlock = NULL;
                int i;
                for (i = 0; i < putDataBlockList->blockCount; i++) {
                    if (!strcasecmp(putDataBlockList->putDataBlock[i].blockName, "time") ||
                        !strcasecmp(putDataBlockList->putDataBlock[i].blockName, "putDataTime")) {
                        putTimeBlock = &putDataBlockList->putDataBlock[i];
                        break;
                    }
                }
                if (putTimeBlock == NULL) {
                    IDAM_LOG(LOG_ERROR, "No Slice Time!\n");
                    THROW_ERROR(999, "No Slice Time!");
                }
                if (putTimeBlock->data_type != TYPE_DOUBLE || putTimeBlock->count != 1) {
                    IDAM_LOG(LOG_ERROR, "Slice Time type and count are incorrect!\n");
                    THROW_ERROR(999, "Slice Time type and count are incorrect!");
                }
                rc = imas_hdf5_putDataX(clientIdx, CPOPath, path, type, putDataBlock->rank, putDataBlock->shape,
                                   dataOperation, (void*)putDataBlock->data, ((double*)putTimeBlock->data)[0]);
            } else {
                rc = imas_hdf5_putDataX(clientIdx, CPOPath, path, type, rank, shape, dataOperation, putdata, 0.0);
            }    // Replace Last Slice
        }

        if (rc < 0) err = 999;

        if (freeShape && putDataBlock->shape) {
            free((void*)putDataBlock->shape);
            putDataBlock->shape = NULL;
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

    return err;
}

static int do_open(IDAM_PLUGIN_INTERFACE* idam_plugin_interface, int* idx)
{
    int err = 0;
    DATA_BLOCK* data_block = idam_plugin_interface->data_block;

/*
name	- filename
shot	- experiment number
run	- analysis number
refShot - not used
refRun  - not used
retIdx	- returned data file index number
*/

    // Passed name-value pairs

    char* filename;
    FIND_REQUIRED_STRING_VALUE(idam_plugin_interface->request_block->nameValueList, filename);

    int shotNumber;
    FIND_REQUIRED_INT_VALUE(idam_plugin_interface->request_block->nameValueList, shotNumber);

    int runNumber;
    FIND_REQUIRED_INT_VALUE(idam_plugin_interface->request_block->nameValueList, runNumber);

    if (imas_hdf5_EuitmOpen(filename, shotNumber, runNumber, idx) != 0) {
        idamLogWithFunc(LOG_DEBUG, (logFunc)H5Eprint1);
        THROW_ERROR(999, "Data Open method failed!");
    }

    // Return the Index Number

    int* data = (int*)malloc(sizeof(int));
    data[0] = *idx;

    putImasIdsShot(*idx, shotNumber);
    putImasIdsRun(*idx, runNumber);

    initDataBlock(data_block);
    data_block->rank = 0;
    data_block->dims = NULL;
    data_block->data_type = TYPE_INT;
    data_block->data = (char*)data;
    data_block->data_n = 1;

    return err;
}

static int do_create(IDAM_PLUGIN_INTERFACE* idam_plugin_interface, int* idx)
{
    int err = 0;
    DATA_BLOCK* data_block = idam_plugin_interface->data_block;

/*
name	- filename
shot	- experiment number
run	- analysis number
refShot - not used
refRun  - not used
retIdx	- returned data file index number
*/

    char* filename;
    FIND_REQUIRED_STRING_VALUE(idam_plugin_interface->request_block->nameValueList, filename);

    int shotNumber;
    FIND_REQUIRED_INT_VALUE(idam_plugin_interface->request_block->nameValueList, shotNumber);

    int runNumber;
    FIND_REQUIRED_INT_VALUE(idam_plugin_interface->request_block->nameValueList, runNumber);

    int refShotNumber = shotNumber;
    FIND_INT_VALUE(idam_plugin_interface->request_block->nameValueList, refShotNumber);

    int refRunNumber = runNumber;
    FIND_INT_VALUE(idam_plugin_interface->request_block->nameValueList, refRunNumber);

    int clientIdx;
    FIND_REQUIRED_INT_VALUE(idam_plugin_interface->request_block->nameValueList, clientIdx);

    bool isCreateFromModel = findValue(&idam_plugin_interface->request_block->nameValueList, "createFromModel");

    if (isCreateFromModel) {
        if (imas_hdf5_EuitmCreate(filename, shotNumber, runNumber, refShotNumber, refRunNumber, idx) < 0) {
            addIdamError(&idamerrorstack, CODEERRORTYPE, __func__, 999, getImasErrorMsg());
            THROW_ERROR(999, "File Create method from Model failed!");
        }
    } else {
        if (imas_hdf5_IMASCreate(filename, shotNumber, runNumber, refShotNumber, refRunNumber, idx) < 0) {
            idamLogWithFunc(LOG_DEBUG, (logFunc)H5Eprint1);
            addIdamError(&idamerrorstack, CODEERRORTYPE, __func__, 999, getImasErrorMsg());
            THROW_ERROR(999, "File Create method failed!");
        }
    }

    putImasIdsShot(clientIdx, shotNumber);
    putImasIdsRun(clientIdx, runNumber);

    // Return the Index Number

    int* data = (int*)malloc(sizeof(int));
    data[0] = *idx;

    initDataBlock(data_block);
    data_block->rank = 0;
    data_block->dims = NULL;
    data_block->data_type = TYPE_INT;
    data_block->data = (char*)data;
    data_block->data_n = 1;

    return err;
}

static int do_close(IDAM_PLUGIN_INTERFACE* idam_plugin_interface)
{
    int err = 0;
    DATA_BLOCK* data_block = idam_plugin_interface->data_block;

    char* filename;
    FIND_REQUIRED_STRING_VALUE(idam_plugin_interface->request_block->nameValueList, filename);

    int shotNumber;
    FIND_REQUIRED_INT_VALUE(idam_plugin_interface->request_block->nameValueList, shotNumber);

    int runNumber;
    FIND_REQUIRED_INT_VALUE(idam_plugin_interface->request_block->nameValueList, runNumber);

    int clientIdx;
    FIND_REQUIRED_INT_VALUE(idam_plugin_interface->request_block->nameValueList, clientIdx);

    if (imas_hdf5_EuitmClose(clientIdx, filename, shotNumber, runNumber) < 0) {
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

    return err;
}

static int do_createModel(IDAM_PLUGIN_INTERFACE* idam_plugin_interface)
{
    int err = 0;
    DATA_BLOCK* data_block = idam_plugin_interface->data_block;

    int version = 0;

    char* filename;
    FIND_REQUIRED_STRING_VALUE(idam_plugin_interface->request_block->nameValueList, filename);

    if (imas_hdf5_IdsModelCreate(filename, version) < 0) {
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

    return err;
}

static int do_releaseObject(IDAM_PLUGIN_INTERFACE* idam_plugin_interface)
{
    int err = 0;
    DATA_BLOCK* data_block = idam_plugin_interface->data_block;

    // Test all required data is available

    int clientObjectId;
    FIND_REQUIRED_INT_VALUE(idam_plugin_interface->request_block->nameValueList, clientObjectId);

    // Identify the local object

    void* obj = findLocalObj(clientObjectId);

    // call the original IMAS function

    if (obj != NULL) {
        imas_hdf5_ReleaseObject(obj);
    }

    // Return a status value

    initDataBlock(data_block);
    data_block->rank = 0;
    data_block->data_n = 1;
    data_block->data_type = TYPE_INT;
    data_block->data = (char*)malloc(sizeof(int));
    *((int*)data_block->data) = 1;

    return err;
}

static int do_getObjectObject(IDAM_PLUGIN_INTERFACE* idam_plugin_interface)
{
    int err = 0;
    DATA_BLOCK* data_block = idam_plugin_interface->data_block;

    int rc = 0;

// Test all required data is available

    int index;
    FIND_REQUIRED_INT_VALUE(idam_plugin_interface->request_block->nameValueList, index);

    char* path;
    FIND_REQUIRED_STRING_VALUE(idam_plugin_interface->request_block->nameValueList, path);

    bool isPutData = (bool)(idam_plugin_interface->request_block->putDataBlockList.blockCount > 0);

    if (!isPutData) {
        IDAM_LOG(LOG_ERROR, "Insufficient data parameters passed - begin not possible!\n");
        THROW_ERROR(999, "Insufficient data parameters passed - begin not possible!");
    }

    PUTDATA_BLOCK* putDataBlock = &idam_plugin_interface->request_block->putDataBlockList.putDataBlock[0];

// Identify the local object

// ***TODO change to passed by name-value reference
    void* obj = findLocalObj(*((int*)putDataBlock->data));

// call the original IMAS function

    void* dataObj = NULL;
    rc = imas_hdf5_GetObjectFromObject(obj, path, index, &dataObj);

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

    return err;
}

static int do_getObjectSlice(IDAM_PLUGIN_INTERFACE* idam_plugin_interface)
{
    int err = 0;
    DATA_BLOCK* data_block = idam_plugin_interface->data_block;

    int rc = 0;

// Test all required data is available

    int clientIdx;
    FIND_REQUIRED_INT_VALUE(idam_plugin_interface->request_block->nameValueList, clientIdx);

    char* CPOPath;
    FIND_REQUIRED_STRING_VALUE(idam_plugin_interface->request_block->nameValueList, CPOPath);

    char* hdf5Path;
    FIND_REQUIRED_STRING_VALUE(idam_plugin_interface->request_block->nameValueList, hdf5Path);

    unsigned int blockCount = idam_plugin_interface->request_block->putDataBlockList.blockCount;

    if (blockCount != 3) {
        IDAM_LOG(LOG_ERROR, "Insufficient data parameters passed - begin not possible!\n");
        THROW_ERROR(999, "Insufficient data parameters passed - begin not possible!");
    }

    PUTDATA_BLOCK* putDataBlock = &idam_plugin_interface->request_block->putDataBlockList.putDataBlock[0];

// Set global variables

// ***TODO check the organisation of the data
    setSliceIdx(((int*)putDataBlock[1].data)[0], ((int*)putDataBlock[1].data)[1]);
    setSliceTime(((double*)putDataBlock[2].data)[0], ((double*)putDataBlock[2].data)[1]);

// call the original IMAS function

    void* dataObj = NULL;
    rc = imas_hdf5_GetObjectSlice(clientIdx, hdf5Path, CPOPath, *((double*)putDataBlock[0].data), &dataObj);

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

    return err;
}

static int do_getObjectGroup(IDAM_PLUGIN_INTERFACE* idam_plugin_interface)
{
    int err = 0;
    DATA_BLOCK* data_block = idam_plugin_interface->data_block;

    int rc = 0;

// Test all required data is available

    int clientIdx;
    FIND_REQUIRED_INT_VALUE(idam_plugin_interface->request_block->nameValueList, clientIdx);

    char* path;
    FIND_REQUIRED_STRING_VALUE(idam_plugin_interface->request_block->nameValueList, path);

    char* CPOPath;
    FIND_REQUIRED_STRING_VALUE(idam_plugin_interface->request_block->nameValueList, CPOPath);

    short isTimed;
    FIND_REQUIRED_SHORT_VALUE(idam_plugin_interface->request_block->nameValueList, isTimed);

// Identify the local object
//       void *obj = findLocalObj(*((int *)putDataBlock->data));

// call the original IMAS function

    void* dataObj = NULL;
    rc = imas_hdf5_GetObject(clientIdx, path, CPOPath, &dataObj, isTimed);

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

    return err;
}

static int do_getObjectDim(IDAM_PLUGIN_INTERFACE* idam_plugin_interface)
{
    int err = 0;
    DATA_BLOCK* data_block = idam_plugin_interface->data_block;

// Object? The data is the local object reference

    PUTDATA_BLOCK_LIST* putDataBlockList = &idam_plugin_interface->request_block->putDataBlockList;

    if (putDataBlockList->putDataBlock[0].data == NULL) {
        IDAM_LOG(LOG_ERROR, "No data object has been specified!\n");
        THROW_ERROR(999, "No data object has been specified!");
    }

    PUTDATA_BLOCK* putDataBlock = idam_plugin_interface->request_block->putDataBlockList.putDataBlock;

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

    return err;
}

static int do_beginObject(IDAM_PLUGIN_INTERFACE* idam_plugin_interface)
{
    int err = 0;
    DATA_BLOCK* data_block = idam_plugin_interface->data_block;

// Test all required data is available

    int clientIdx;
    FIND_REQUIRED_INT_VALUE(idam_plugin_interface->request_block->nameValueList, clientIdx);

    int index;
    FIND_REQUIRED_INT_VALUE(idam_plugin_interface->request_block->nameValueList, index);

    char* relPath;
    FIND_REQUIRED_STRING_VALUE(idam_plugin_interface->request_block->nameValueList, relPath);

    int isTimed;
    FIND_REQUIRED_INT_VALUE(idam_plugin_interface->request_block->nameValueList, isTimed);

    unsigned int blockCount = idam_plugin_interface->request_block->putDataBlockList.blockCount;

    if (blockCount != 2) {
        IDAM_LOG(LOG_ERROR, "Insufficient data parameters passed - begin not possible!\n");
        THROW_ERROR(999, "Insufficient data parameters passed - begin not possible!");
    }

// Object? The data is the local object reference

    PUTDATA_BLOCK_LIST* putDataBlockList = &idam_plugin_interface->request_block->putDataBlockList;

    if (putDataBlockList->putDataBlock[0].data == NULL) {
        IDAM_LOG(LOG_ERROR, "imas beginObject: No data object has been specified!\n");
        THROW_ERROR(999, "No data object has been specified!");
    }

// Set global variables

    PUTDATA_BLOCK* putDataBlock = idam_plugin_interface->request_block->putDataBlockList.putDataBlock;

    setSliceIdx(((int*)putDataBlock[1].data)[0], ((int*)putDataBlock[1].data)[1]);

// Identify the local object
// Two standard references are created at initialisation: NULL and -1 with refIds of 0 and 1.

    void* obj = findLocalObj(*((int*)putDataBlock[0].data));

// The type passed here is the IMAS type enumeration

    void* data = imas_hdf5_BeginObject(clientIdx, obj, index, relPath, isTimed);

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

    return err;
}

static int do_getObject(IDAM_PLUGIN_INTERFACE* idam_plugin_interface)
{
    int err = 0;
    DATA_BLOCK* data_block = idam_plugin_interface->data_block;

    int rc = 0;

// Test all required data is available

    int index;
    FIND_REQUIRED_INT_VALUE(idam_plugin_interface->request_block->nameValueList, index);

    char* path;
    FIND_REQUIRED_STRING_VALUE(idam_plugin_interface->request_block->nameValueList, path);

    char* typeName;
    FIND_REQUIRED_STRING_VALUE(idam_plugin_interface->request_block->nameValueList, typeName);

    int rank;
    FIND_REQUIRED_INT_VALUE(idam_plugin_interface->request_block->nameValueList, rank);

    int clientObjectId;
    FIND_REQUIRED_INT_VALUE(idam_plugin_interface->request_block->nameValueList, clientObjectId);


// Identify the local object

    void* obj = findLocalObj(clientObjectId);

// The type passed here is the IMAS type enumeration

    void* data = NULL;
    int type = findIMASType(typeName);

// Object Shape

    int* dims = (int*)malloc((rank + 1) * sizeof(int));

    rc = imas_hdf5_getDataSliceFromObject(obj, path, index, type, rank, dims, &data);

    if (rc < 0) {
        free(data);
        THROW_ERROR(999, "Data GET method failed!");
    }

// If there is no data, dims is set to 1 and the data are set to EMPTY_<type>

// Return the Object's Data

    initDataBlock(data_block);
    data_block->rank = (unsigned int)rank;
    data_block->data_type = findIMASIDAMType(type);
    data_block->data = (char*)data;
    data_block->data_n = 1;

    if (data_block->rank > 0) {
        data_block->dims = (DIMS*)malloc(data_block->rank * sizeof(DIMS));

        int i;
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

    return err;
}

static int do_putObject(IDAM_PLUGIN_INTERFACE* idam_plugin_interface)
{
    int err = 0;
    DATA_BLOCK* data_block = idam_plugin_interface->data_block;

    int rc = 0;
    int type;

// Test all required data is available

    int index;
    FIND_REQUIRED_INT_VALUE(idam_plugin_interface->request_block->nameValueList, index);

    char* path;
    FIND_REQUIRED_STRING_VALUE(idam_plugin_interface->request_block->nameValueList, path);

    unsigned int blockCount = idam_plugin_interface->request_block->putDataBlockList.blockCount;

    if (blockCount != 2) {
        IDAM_LOG(LOG_ERROR, "Insufficient data parameters passed - put not possible!\n");
        THROW_ERROR(999, "Insufficient data parameters passed - put not possible!");
    }

    PUTDATA_BLOCK_LIST* putDataBlockList = &idam_plugin_interface->request_block->putDataBlockList;

    if ((type = findIMASType(convertIdam2StringType(putDataBlockList->putDataBlock[1].data_type))) == 0) {
        // Convert an IDAM type to an IMAS type
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

    int* shape = (int*) malloc(sizeof(int));
    if (putDataBlockList->putDataBlock[1].rank == 1 && putDataBlockList->putDataBlock[1].shape == NULL) {
        putDataBlockList->putDataBlock[1].shape = shape;
        shape[0] = putDataBlockList->putDataBlock[1].count;
    }

// The type passed here is the IMAS type enumeration

    rc = imas_hdf5_putDataSliceInObject(obj, path, index, type, putDataBlockList->putDataBlock[1].rank,
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

    return err;
}

static int do_putObjectInObject(IDAM_PLUGIN_INTERFACE* idam_plugin_interface)
{
    int err = 0;
    DATA_BLOCK* data_block = idam_plugin_interface->data_block;

    // No HDF5 function in UAL

    // Test all required data is available

    int clientObjectId;
    FIND_REQUIRED_INT_VALUE(idam_plugin_interface->request_block->nameValueList, clientObjectId);

    // Return the Passed Object Reference

    int* data = (int*)malloc(sizeof(int));
    data[0] = clientObjectId;

    initDataBlock(data_block);
    data_block->rank = 0;
    data_block->dims = NULL;
    data_block->data_type = TYPE_INT;
    data_block->data = (char*)data;
    data_block->data_n = 1;

    return err;
}

static int do_source(IDAM_PLUGIN_INTERFACE* idam_plugin_interface, int* time_count_cache, char* time_cache, char* data_cache)
{
    int err = 0;
    DATA_BLOCK* data_block = idam_plugin_interface->data_block;

    static char time_units_cache[STRING_LENGTH];
    static char time_label_cache[STRING_LENGTH];
    static char signal_cache[STRING_LENGTH];
    static char source_cache[STRING_LENGTH];

    char api_signal[STRING_LENGTH];
    char api_source[STRING_LENGTH];

    REQUEST_BLOCK* request_block = idam_plugin_interface->request_block;

    bool isData = findValue(&request_block->nameValueList, "data");
    bool isTime = findValue(&request_block->nameValueList, "time");
    bool isNoCacheTime = findValue(&request_block->nameValueList, "nocacheTime");
    bool isNoCacheData = findValue(&request_block->nameValueList, "nocacheData");

    float dataScaling;
    bool isDataScaling = FIND_FLOAT_VALUE(request_block->nameValueList, dataScaling);

    float timeScaling = 0.0;
    bool isTimeScaling = FIND_FLOAT_VALUE(request_block->nameValueList, dataScaling);

    char* signal;
    FIND_REQUIRED_STRING_VALUE(request_block->nameValueList, signal);

    // Prepare common code

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

    int shotNumber;
    bool isShotNumber = FIND_INT_VALUE(request_block->nameValueList, shotNumber);
    if (!isShotNumber) {
        shotNumber = request_block->exp_number;
    }

    char* host = NULL;
    int port;

// JET PPF sources: PPF::/$ppfname/$pulseNumber/$sequence/$owner

    char* format;
    bool isFormat = FIND_STRING_VALUE(request_block->nameValueList, format);

    if (isFormat && !strcasecmp(format, "ppf")) {            // JET PPF source naming pattern
        int source;
        FIND_REQUIRED_INT_VALUE(request_block->nameValueList, source);

        char* env = getenv("UDA_JET_DEVICE_ALIAS");
        host = getenv("UDA_JET_HOST");
        port = atoi(getenv("UDA_JET_PORT"));

        if (env == NULL) {
            sprintf(next_request_block.source, "JET%sPPF%s/%s/%d", request_block->api_delim,
                    request_block->api_delim, source, shotNumber);
        } else {
            sprintf(next_request_block.source, "%s%sPPF%s/%s/%d", env, request_block->api_delim,
                    request_block->api_delim, source, shotNumber);
        }

        int runNumber;
        bool isRunNumber = FIND_INT_VALUE(request_block->nameValueList, runNumber);

        if (isRunNumber) {
            sprintf(next_request_block.source, "%s/%d", next_request_block.source, runNumber);
        } else {
            sprintf(next_request_block.source, "%s/0", next_request_block.source);
        }

        char* owner;
        bool isOwner = FIND_STRING_VALUE(request_block->nameValueList, owner);

        if (isOwner) {
            sprintf(next_request_block.source, "%s/%s", next_request_block.source, owner);
        }

    } else if (isFormat && !strcasecmp(format, "jpf")) {        // JET JPF source naming pattern

        char* env = getenv("UDA_JET_DEVICE_ALIAS");
        host = getenv("UDA_JET_HOST");
        port = atoi(getenv("UDA_JET_PORT"));

        if (env == NULL) {
            sprintf(next_request_block.source, "JET%sJPF%s%d", request_block->api_delim,
                    request_block->api_delim, shotNumber);
        } else {
            sprintf(next_request_block.source, "%s%sJPF%s%d", env, request_block->api_delim,
                    request_block->api_delim, shotNumber);
        }
    } else if (isFormat && !strcasecmp(format, "MAST")) {        // MAST source naming pattern

        char* env = getenv("UDA_MAST_DEVICE_ALIAS");
        host = getenv("UDA_MAST_HOST");
        port = atoi(getenv("UDA_MAST_PORT"));

        int runNumber;
        bool isRunNumber = FIND_INT_VALUE(request_block->nameValueList, runNumber);

        if (!isShotNumber && !isRunNumber) {
            // Re-Use the original source argument
            strcpy(next_request_block.source, request_block->source);
        } else {
            if (env == NULL) {
                sprintf(next_request_block.source, "MAST%s%d", request_block->api_delim, shotNumber);
            } else {
                sprintf(next_request_block.source, "%s%s%d", env, request_block->api_delim, shotNumber);
            }
        }
        if (isRunNumber) sprintf(next_request_block.source, "%s/%d", next_request_block.source, runNumber);

    } else if (isFormat && (!strcasecmp(format, "mds") || !strcasecmp(format, "mdsplus") ||
                            !strcasecmp(format, "mds+"))) {    // MDS+ source naming pattern

        char* server;
        FIND_REQUIRED_STRING_VALUE(request_block->nameValueList, server);

        char* source;
        bool isSource = FIND_STRING_VALUE(request_block->nameValueList, source);

        char* env = getenv("UDA_MDSPLUS_ALIAS");
        host = getenv("UDA_MDSPLUS_HOST");
        port = atoi(getenv("UDA_MDSPLUS_PORT"));

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

    char work[MAXMETA];
    char* env = getenv("UDA_CLIENT_PLUGIN");

    if (env != NULL) {
        sprintf(work, "%s::get(host=%s, port=%d, signal=\"%s\", source=\"%s\")", env, host, port,
                next_request_block.signal, next_request_block.source);
    } else {
        sprintf(work, "UDA::get(host=%s, port=%d, signal=\"%s\", source=\"%s\")", host, port,
                next_request_block.signal, next_request_block.source);
    }

    next_request_block.source[0] = '\0';
    strcpy(next_request_block.signal, work);

    makeServerRequestBlock(&next_request_block, *pluginList);

    // These are what are used to access data - retain as cache keys
    strcpy(api_signal, next_request_block.signal);
    strcpy(api_source, next_request_block.source);

// Call the IDAM client via the IDAM plugin (ignore the request identified)

    if (env != NULL) {
        next_request_block.request = findPluginRequestByFormat(env, pluginList);
    } else {
        next_request_block.request = findPluginRequestByFormat("UDA", pluginList);
    }

    if (next_request_block.request < 0) {
        IDAM_LOG(LOG_ERROR, "No UDA server plugin found!\n");
        THROW_ERROR(999, "No UDA server plugin found!");
    }

    // is Time requested and is the data cached? Does the IDS time entity name match the cached data entity name
    // Caching is the default behaviour
    // If data are cached then skip the plugin request for data - use the cached data

    int skipPlugin =
            isTime && !isNoCacheTime && *time_count_cache > 0 && !strcasecmp(signal_cache, api_signal) &&
            !strcasecmp(source_cache, api_source);

    // Locate and Execute the IDAM plugin

    if (!skipPlugin) {
        int id = findPluginIdByRequest(next_request_block.request, pluginList);
        if (id >= 0 && pluginList->plugin[id].idamPlugin != NULL) {
            err = pluginList->plugin[id].idamPlugin(&next_plugin_interface);        // Call the data reader
            if (err != 0) {
                THROW_ERROR(999, "Data Access is not available!");
            }
        } else {
            THROW_ERROR(999, "Data Access is not available for this data request!");
        }

    }

    freeNameValueList(&next_request_block.nameValueList);

    // Return data is automatic since both next_request_block and request_block point to the same DATA_BLOCK etc.
    // IMAS data must be DOUBLE
    // Time Data are only cacheable if the data are rank 1 with time data!

    if (isData) {
        // Ignore the coordinate data.

        if (data_block->order != 0 || data_block->rank != 1 ||
            !(data_block->data_type == TYPE_FLOAT || data_block->data_type == TYPE_DOUBLE)) {
            // Data are not Cacheable
            IDAM_LOG(LOG_ERROR, "Data Access is not available for this data request!\n");
            THROW_ERROR(999, "Data Access is not available for this data request!");
        }

        if (!isNoCacheTime) {
            // Save the Time Coordinate data to local cache  (free currently cached data if different)
            free((void*)time_cache);
            time_cache = NULL;
            *time_count_cache = 0;
            strcpy(signal_cache, api_signal);
            strcpy(source_cache, api_source);
            if (data_block->dims[0].compressed) {
                uncompressDim(&data_block->dims[0]);
            }
            *time_count_cache = data_block->dims[0].dim_n;
            strcpy(time_units_cache, data_block->dims[0].dim_units);
            strcpy(time_label_cache, data_block->dims[0].dim_label);

            if (data_block->dims[0].data_type == TYPE_DOUBLE) {
                time_cache = (char*)malloc(*time_count_cache * sizeof(double));
                if (isTimeScaling) {
                    double* dimdata = (double*)data_block->dims[0].dim;
                    int i;
                    for (i = 0; i < *time_count_cache; i++) {
                        dimdata[i] = timeScaling * dimdata[i];
                    }
                    data_block->dims[0].dim = (char*)dimdata;
                }
                memcpy(time_cache, data_block->dims[0].dim, *time_count_cache * sizeof(double));
            } else {
                float* data = (float*)data_block->dims[0].dim;
                double* dimdata = (double*)malloc(*time_count_cache * sizeof(double));
                if (isTimeScaling) {
                    int i;
                    for (i = 0; i < *time_count_cache; i++) {
                        dimdata[i] = timeScaling * (double)data[i];
                    }
                } else {
                    int i;
                    for (i = 0; i < *time_count_cache; i++) {
                        dimdata[i] = (double)data[i];
                    }
                }
                time_cache = (char*)dimdata;
            }
        } else {
            // End of Time cache
            free((void*)time_cache);    // Clear the cache
            free((void*)data_cache);
            time_cache = NULL;
            data_cache = NULL;
            *time_count_cache = 0;
            signal_cache[0] = '\0';
            source_cache[0] = '\0';
        }

        if (data_block->rank == 1 &&
            (data_block->data_type == TYPE_FLOAT || data_block->data_type == TYPE_DOUBLE)) {

            data_block->rank = 0;        // No coordinate data to be returned
            data_block->order = -1;

            if (data_block->data_type == TYPE_FLOAT) {
                float* data = (float*)data_block->data;
                double* dimdata = (double*)malloc(data_block->data_n * sizeof(double));
                if (isDataScaling) {
                    int i;
                    for (i = 0; i < data_block->data_n; i++) {
                        dimdata[i] = dataScaling * (double)data[i];
                    }
                } else {
                    int i;
                    for (i = 0; i < data_block->data_n; i++) {
                        dimdata[i] = (double)data[i];
                    }
                }
                free((void*)data_block->data);
                data_block->data = (char*)dimdata;
                data_block->data_type = TYPE_DOUBLE;
            } else {
                if (isDataScaling) {
                    double* data = (double*)data_block->data;
                    int i;
                    for (i = 0; i < data_block->data_n; i++) {
                        data[i] = dataScaling * data[i];
                    }
                }
            }

            if (data_block->dims[0].dim != NULL) {
                free((void*)data_block->dims[0].dim);
            }
            data_block->dims[0].dim = NULL;        // prevent a double free
            data_block->dims[0].dim_n = 0;
        } else {
            IDAM_LOG(LOG_ERROR, "Data Access is not available for this data request!\n");
            THROW_ERROR(999, "Data Access is not available for this data request!");
        }
    }

// For efficiency, local client cache should also be running

    if (isTime) {
        // The time data are in the coordinate array indicated by 'order' value. The data must be rank 1. Data may be compressed
        if (!isNoCacheTime && *time_count_cache > 0 &&
            !strcasecmp(signal_cache, api_signal) && !strcasecmp(source_cache, api_source)) {
            // Retrieve the Time Coordinate data from the local cache after verification of IDS names
            data_block->rank = 0;
            data_block->order = -1;
            data_block->data = (char*)malloc(*time_count_cache * sizeof(double));
            memcpy(data_block->data, time_cache, *time_count_cache * sizeof(double));
            data_block->data_n = *time_count_cache;
            data_block->data_type = TYPE_DOUBLE;
            data_block->dims = NULL;
            strcpy(data_block->data_units, time_units_cache);
            strcpy(data_block->data_label, time_label_cache);
        } else if (data_block->rank == 1 && data_block->order == 0 &&
                   (data_block->data_type == TYPE_FLOAT || data_block->data_type == TYPE_DOUBLE)) {

            if (data_block->dims[0].compressed) {
                uncompressDim(&data_block->dims[0]);
            }
            data_block->rank = 0;
            data_block->order = -1;
            if (data_block->data != NULL) {
                free((void*)data_block->data);
            }

            if (data_block->dims[0].data_type == TYPE_DOUBLE) {
                data_block->data = data_block->dims[0].dim;
                if (isTimeScaling) {
                    double* dimdata = (double*)data_block->data;
                    int i;
                    for (i = 0; i < data_block->dims[0].dim_n; i++) {
                        dimdata[i] = timeScaling * dimdata[i];
                    }
                }
            } else {
                float* data = (float*)data_block->dims[0].dim;
                double* dimdata = (double*)malloc(data_block->dims[0].dim_n * sizeof(double));
                if (isTimeScaling) {
                    int i;
                    for (i = 0; i < data_block->dims[0].dim_n; i++) {
                        dimdata[i] = timeScaling * (double)data[i];
                    }
                } else {
                    int i;
                    for (i = 0; i < data_block->dims[0].dim_n; i++) {
                        dimdata[i] = (double)data[i];
                    }
                }
                data_block->data = (char*)dimdata;
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

    return err;
}

static int do_beginIdsPut(IDAM_PLUGIN_INTERFACE* idam_plugin_interface)
{
    int err = 0;
    DATA_BLOCK* data_block = idam_plugin_interface->data_block;

    // Return Success

    int* data = (int*)malloc(sizeof(int));
    data[0] = 1;

    initDataBlock(data_block);
    data_block->data_type = TYPE_INT;
    data_block->data = (char*)data;
    data_block->data_n = 1;

    return err;
}

static int do_endIdsPut(IDAM_PLUGIN_INTERFACE* idam_plugin_interface)
{
    int err = 0;
    DATA_BLOCK* data_block = idam_plugin_interface->data_block;

    // Return Success

    int* data = (int*)malloc(sizeof(int));
    data[0] = 1;

    initDataBlock(data_block);
    data_block->data_type = TYPE_INT;
    data_block->data = (char*)data;
    data_block->data_n = 1;

    return err;
}

static int do_help(IDAM_PLUGIN_INTERFACE* idam_plugin_interface)
{
    int err = 0;
    DATA_BLOCK* data_block = idam_plugin_interface->data_block;

    char* p = (char*)malloc(sizeof(char) * 2 * 1024);

    strcpy(p, "\nimas: Add Functions Names, Syntax, and Descriptions\n\n");

    initDataBlock(data_block);

    data_block->rank = 1;
    data_block->dims = (DIMS*)malloc(data_block->rank * sizeof(DIMS));

    int i;
    for (i = 0; i < data_block->rank; i++) {
        initDimBlock(&data_block->dims[i]);
    }

    data_block->data_type = TYPE_STRING;
    strcpy(data_block->data_desc, "imas: help = description of this plugin");

    data_block->data = p;

    data_block->dims[0].data_type = TYPE_UNSIGNED_INT;
    data_block->dims[0].dim_n = (int)strlen(p) + 1;
    data_block->dims[0].compressed = 1;
    data_block->dims[0].dim0 = 0.0;
    data_block->dims[0].diff = 1.0;
    data_block->dims[0].method = 0;

    data_block->data_n = data_block->dims[0].dim_n;

    strcpy(data_block->data_label, "");
    strcpy(data_block->data_units, "");

    return err;
}

static int do_version(IDAM_PLUGIN_INTERFACE* idam_plugin_interface)
{
    int err = 0;
    DATA_BLOCK* data_block = idam_plugin_interface->data_block;

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

    return err;
}

static int do_builddate(IDAM_PLUGIN_INTERFACE* idam_plugin_interface)
{
    int err = 0;
    DATA_BLOCK* data_block = idam_plugin_interface->data_block;

    initDataBlock(data_block);
    data_block->data_type = TYPE_STRING;
    data_block->rank = 0;
    data_block->data_n = (int)strlen(__DATE__) + 1;
    char* data = (char*)malloc(data_block->data_n * sizeof(char));
    strcpy(data, __DATE__);
    data_block->data = data;
    strcpy(data_block->data_desc, "Plugin build date");
    strcpy(data_block->data_label, "date");
    strcpy(data_block->data_units, "");

    return err;
}

static int do_defaultmethod(IDAM_PLUGIN_INTERFACE* idam_plugin_interface)
{
    int err = 0;
    DATA_BLOCK* data_block = idam_plugin_interface->data_block;

    initDataBlock(data_block);
    data_block->data_type = TYPE_STRING;
    data_block->rank = 0;
    data_block->data_n = (int)strlen(THISPLUGIN_DEFAULT_METHOD) + 1;
    char* data = (char*)malloc(data_block->data_n * sizeof(char));
    strcpy(data, THISPLUGIN_DEFAULT_METHOD);
    data_block->data = data;
    strcpy(data_block->data_desc, "Plugin default method");
    strcpy(data_block->data_label, "method");
    strcpy(data_block->data_units, "");

    return err;
}

static int do_maxinterfaceversion(IDAM_PLUGIN_INTERFACE* idam_plugin_interface)
{
    int err = 0;
    DATA_BLOCK* data_block = idam_plugin_interface->data_block;

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

    return err;
}
