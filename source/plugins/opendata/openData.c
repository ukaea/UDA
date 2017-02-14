/*---------------------------------------------------------------
* v1 IDAM Open Data plugin: Serverside functionality ...
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
#include "openData.h"

#include <sys/stat.h>
#include <dirent.h>
#include <utime.h>
#include <stdlib.h>
#include <errno.h>
#include <strings.h>

#include <server/idamServer.h>
#include <clientserver/initStructs.h>
#include <clientserver/idamTypes.h>

#ifndef USE_PLUGIN_DIRECTLY
IDAMERRORSTACK* idamErrorStack;    // Pointer to the Server's Error Stack. Global scope within this plugin library
#endif

extern int openData(IDAM_PLUGIN_INTERFACE* idam_plugin_interface)
{
    int i, err = 0;
    char* p;

    static short init = 0;

    static char root[MAXROOT];

//----------------------------------------------------------------------------------------
// Standard v1 Plugin Interface

    DATA_BLOCK* data_block;
    REQUEST_BLOCK* request_block;

#ifndef USE_PLUGIN_DIRECTLY
    idamErrorStack = getIdamServerPluginErrorStack();        // Server's error stack

    initIdamErrorStack(&idamerrorstack);        // Initialise Local Error Stack (defined in idamclientserver.h)
#else
    IDAMERRORSTACK *idamErrorStack = &idamerrorstack;		// local and server are the same!
#endif

    unsigned short housekeeping;

    if (idam_plugin_interface->interfaceVersion >= 1) {

        idam_plugin_interface->pluginVersion = 1;            // This plugin's version number

        data_block = idam_plugin_interface->data_block;
        request_block = idam_plugin_interface->request_block;

        housekeeping = idam_plugin_interface->housekeeping;
    } else {
        err = 999;
        idamLog(LOG_ERROR, "ERROR openData: Plugin Interface Version Unknown\n");

        addIdamError(&idamerrorstack, CODEERRORTYPE, "openData", err,
                     "Plugin Interface Version is Not Known: Unable to execute the request!");
        concatIdamError(idamerrorstack, idamErrorStack);
        return err;
    }

//----------------------------------------------------------------------------------------
// Heap Housekeeping 

// Plugin must maintain a list of open file handles and sockets: loop over and close all files and sockets
// Plugin must maintain a list of plugin functions called: loop over and reset state and free heap.
// Plugin must maintain a list of calls to other plugins: loop over and call each plugin with the housekeeping request
// Plugin must destroy lists at end of housekeeping

// A plugin only has a single instance on a server. For multiple instances, multiple servers are needed.
// Plugins can maintain state so recursive calls (on the same server) must respect this.
// If the housekeeping action is requested, this must be also applied to all plugins called.
// A list must be maintained to register these plugin calls to manage housekeeping.
// Calls to plugins must also respect access policy and user authentication policy

    if (housekeeping || !strcasecmp(request_block->function, "reset")) {

        if (!init) return 0;        // Not previously initialised: Nothing to do!

// Free Heap & reset counters

        init = 0;

        return 0;
    }

//----------------------------------------------------------------------------------------
// Initialise 

    if (!init || !strcasecmp(request_block->function, "init")
        || !strcasecmp(request_block->function, "initialise")) {

        char* env = getenv("UDA_PROVENANCEROOT");
        if (env != NULL) {
            strncpy(root, env, MAXROOT - 1);
            root[MAXROOT - 1] = '\0';
        } else {
            err = 999;
            addIdamError(&idamerrorstack, CODEERRORTYPE, "openData", err, "No Provenance Data Archive Root specified!");
            return err;
        }

        init = 1;
        if (!strcasecmp(request_block->function, "init") || !strcasecmp(request_block->function, "initialise"))
            return 0;
    }

//---------------------------------------------------------------------------------------- 
// name-value Input data:
//
    char* UID = NULL;        // Must be passed
    char* fileLocation = NULL;
    char* inputDataFile = NULL;
    char* outputDataFile = NULL;
    char* modulesListFile = NULL;
    char* lddLibraryFile = NULL;
    char* envFile = NULL;
    char* idamSignalsFile = NULL;
    char* exeBinaryFile = NULL;
    char* runScriptFile = NULL;

    char* fileLocationName = fileLocation;
    char* inputDataFileName = inputDataFile;
    char* outputDataFileName = outputDataFile;
    char* modulesListFileName = modulesListFile;
    char* lddLibraryFileName = lddLibraryFile;
    char* envFileName = envFile;
    char* idamSignalsFileName = idamSignalsFile;
    char* exeBinaryFileName = exeBinaryFile;
    char* runScriptFileName = runScriptFile;

// Keywords

    unsigned short isReplace = 0;    // Replace any previous file within the archive directory

    unsigned short isFileLocation = 0, isInputDataFile = 0, isOutputDataFile = 0,
            isModulesListFile = 0, isLddLibraryFile = 0, isEnvFile = 0, isIdamSignalsFile = 0,
            isExeBinaryFile = 0, isRunScriptFile = 0;

    for (i = 0; i < request_block->nameValueList.pairCount; i++) {
        if (!strcasecmp(request_block->nameValueList.nameValue[i].name, "UID")) {
            UID = request_block->nameValueList.nameValue[i].value;
            continue;
        }

        if (!strcasecmp(request_block->nameValueList.nameValue[i].name, "Replace")) {
            isReplace = 1;
            continue;
        }

        if (!strcasecmp(request_block->nameValueList.nameValue[i].name, "fileLocation")) {
            isFileLocation = 1;
            fileLocation = request_block->nameValueList.nameValue[i].value;
            if ((p = strrchr(fileLocation, '/')) != NULL) fileLocationName = &p[1];
            continue;
        }
        if (!strcasecmp(request_block->nameValueList.nameValue[i].name, "inputDataFile")) {
            isInputDataFile = 1;
            inputDataFile = request_block->nameValueList.nameValue[i].value;
            if ((p = strrchr(inputDataFile, '/')) != NULL) inputDataFileName = &p[1];
            continue;
        }
        if (!strcasecmp(request_block->nameValueList.nameValue[i].name, "outputDataFile")) {
            isOutputDataFile = 1;
            outputDataFile = request_block->nameValueList.nameValue[i].value;
            if ((p = strrchr(outputDataFile, '/')) != NULL) outputDataFileName = &p[1];
            continue;
        }

        if (!strcasecmp(request_block->nameValueList.nameValue[i].name, "modulesListFile")) {
            isModulesListFile = 1;
            modulesListFile = request_block->nameValueList.nameValue[i].value;
            if ((p = strrchr(modulesListFile, '/')) != NULL) modulesListFileName = &p[1];
            continue;
        }
        if (!strcasecmp(request_block->nameValueList.nameValue[i].name, "lddLibraryFile")) {
            isLddLibraryFile = 1;
            lddLibraryFile = request_block->nameValueList.nameValue[i].value;
            if ((p = strrchr(lddLibraryFile, '/')) != NULL) lddLibraryFileName = &p[1];
            continue;
        }
        if (!strcasecmp(request_block->nameValueList.nameValue[i].name, "envFile")) {
            isEnvFile = 1;
            envFile = request_block->nameValueList.nameValue[i].value;
            if ((p = strrchr(envFile, '/')) != NULL) envFileName = &p[1];
            continue;
        }

        if (!strcasecmp(request_block->nameValueList.nameValue[i].name, "idamSignalsFile")) {
            isIdamSignalsFile = 1;
            idamSignalsFile = request_block->nameValueList.nameValue[i].value;
            if ((p = strrchr(idamSignalsFile, '/')) != NULL) idamSignalsFile = &p[1];
            continue;
        }
        if (!strcasecmp(request_block->nameValueList.nameValue[i].name, "exeBinaryFile")) {
            isExeBinaryFile = 1;
            exeBinaryFile = request_block->nameValueList.nameValue[i].value;
            if ((p = strrchr(exeBinaryFile, '/')) != NULL) exeBinaryFileName = &p[1];
            continue;
        }
        if (!strcasecmp(request_block->nameValueList.nameValue[i].name, "runScriptFile")) {
            isRunScriptFile = 1;
            runScriptFile = request_block->nameValueList.nameValue[i].value;
            if ((p = strrchr(runScriptFile, '/')) != NULL) runScriptFileName = &p[1];
            continue;
        }
    }

//----------------------------------------------------------------------------------------
// Plugin Functions 
//----------------------------------------------------------------------------------------

    do {

// Help: A Description of library functionality

        if (!strcasecmp(request_block->function, "help")) {

            p = (char*) malloc(sizeof(char) * 2 * 1024);

            strcpy(p, "\nopenData: Add Functions Names, Syntax, and Descriptions\n\n");

            initDataBlock(data_block);

            data_block->rank = 1;
            data_block->dims = (DIMS*) malloc(data_block->rank * sizeof(DIMS));
            for (i = 0; i < data_block->rank; i++) initDimBlock(&data_block->dims[i]);

            data_block->data_type = TYPE_STRING;
            strcpy(data_block->data_desc, "openData: help = description of this plugin");

            data_block->data = (char*) p;

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
// Required functions
// put()	pass metadata about figures and data used in publications: use information to snapshot code and data

        if (!strcasecmp(request_block->function, "put")) {

            unsigned short priorDir = 0;
            char* newDir = NULL;

// Create a directory using the Unique Identifier
// directory = Provenance Archive Root + UID

            newDir = (char*) malloc((strlen(root) + strlen(UID) + 2) * sizeof(char));
            sprintf(newDir, "%s/%s", root, UID);

// does the archive exist? If not then create it. Ensure correct permissions: read only for non owner.

            errno = 0;
            int rc = mkdir(newDir, S_IRUSR | S_IWUSR | S_IXUSR |
                                   S_IRGRP | S_IWGRP | S_IXGRP |
                                   S_IROTH | S_IWOTH | S_IXOTH);

            if (rc != 0 || errno != 0) {
                if (errno == EEXIST) {
                    priorDir = 1;            // Exists
                } else {
                    err = 999;
                    if (errno != 0) addIdamError(&idamerrorstack, SYSTEMERRORTYPE, "openData", errno, "");
                    addIdamError(&idamerrorstack, CODEERRORTYPE, "openData", err,
                                 "Unable to create a Provenance Archive Directory!");
                    if (newDir != NULL) free(newDir);
                    break;
                }
            }

// copy each identified file preserving date/time stamps (overwrite if requested)

            if (!isReplace && priorDir) {    // Check for duplicate files
                DIR* dp;
                struct dirent* ep;
                char* name;
                dp = opendir(newDir);
                if (dp != NULL) {
                    while ((ep = readdir(dp))) {
                        name = ep->d_name;

                        if ((isFileLocation && !strcasecmp(name, fileLocationName)) ||
                            (isInputDataFile && !strcasecmp(name, inputDataFileName)) ||
                            (isOutputDataFile && !strcasecmp(name, outputDataFileName)) ||
                            (isModulesListFile && !strcasecmp(name, modulesListFileName)) ||
                            (isLddLibraryFile && !strcasecmp(name, lddLibraryFileName)) ||
                            (isEnvFile && !strcasecmp(name, envFileName)) ||
                            (isIdamSignalsFile && !strcasecmp(name, idamSignalsFileName)) ||
                            (isExeBinaryFile && !strcasecmp(name, exeBinaryFileName)) ||
                            (isRunScriptFile && !strcasecmp(name, runScriptFileName))) {
                            err = 999;
                            addIdamError(&idamerrorstack, CODEERRORTYPE, "openData", err,
                                         "A previous file exists in the Provenance Archive!");
                            break;
                        }
                    }
                    if (err != 0 && newDir != NULL) free(newDir);
                    closedir(dp);
                } else {
                    err = 999;
                    addIdamError(&idamerrorstack, CODEERRORTYPE, "openData", err,
                                 "Unable to create a new directory in the Provenance Archive!");
                    if (newDir != NULL) free(newDir);
                    break;
                }
            }

            if (err != 0) break;

            do {
                if (isFileLocation && (err = copyProvenanceFile(fileLocation, newDir, fileLocationName)) != 0) break;
                if (isInputDataFile && (err = copyProvenanceFile(inputDataFile, newDir, inputDataFileName)) != 0) break;
                if (isOutputDataFile &&
                    (err = copyProvenanceFile(outputDataFile, newDir, outputDataFileName)) != 0)
                    break;
                if (isModulesListFile &&
                    (err = copyProvenanceFile(modulesListFile, newDir, modulesListFileName)) != 0)
                    break;
                if (isLddLibraryFile &&
                    (err = copyProvenanceFile(lddLibraryFile, newDir, lddLibraryFileName)) != 0)
                    break;
                if (isEnvFile && (err = copyProvenanceFile(envFile, newDir, envFileName)) != 0) break;
                if (isIdamSignalsFile &&
                    (err = copyProvenanceFile(idamSignalsFile, newDir, idamSignalsFileName)) != 0)
                    break;
                if (isExeBinaryFile && (err = copyProvenanceFile(exeBinaryFile, newDir, exeBinaryFileName)) != 0) break;
                if (isRunScriptFile && (err = copyProvenanceFile(runScriptFile, newDir, runScriptFileName)) != 0) break;
            } while (0);

            if (newDir != NULL) free(newDir);

            if (err != 0)break;

            initDataBlock(data_block);
            break;

        } else

//======================================================================================
// Error ...

            err = 999;
        addIdamError(&idamerrorstack, CODEERRORTYPE, "openData", err, "Unknown function requested!");
        break;

    } while (0);

//--------------------------------------------------------------------------------------
// Housekeeping

    concatIdamError(idamerrorstack, idamErrorStack);    // Combine local errors with the Server's error stack

#ifndef USE_PLUGIN_DIRECTLY
    closeIdamError(&idamerrorstack);            // Free local plugin error stack
#endif

    return err;
}

// Copy files and preserve time stamps

int copyProvenanceFile(char* oldFile, char* dir, char* newFileName)
{
    int err = 0;
    char buffer[BUFFERSIZE];
    size_t rc, count, n;
    FILE* in = NULL, * out = NULL;
    struct stat attributes;
    char* newFile = NULL;

    newFile = (char*) malloc((strlen(dir) + strlen(newFileName) + 2) * sizeof(char));
    sprintf(newFile, "%s/%s", dir, newFileName);

    do {        // Error Trap

// Read file attributes

        errno = 0;
        if ((rc = stat(oldFile, &attributes)) != 0 || errno != 0) {
            err = 999;
            if (errno != 0) addIdamError(&idamerrorstack, SYSTEMERRORTYPE, "openData", errno, "");
            addIdamError(&idamerrorstack, CODEERRORTYPE, "openData", err, "Unknown function requested!");
            break;
        }

// Check the type (regular file only)

        if (!S_ISREG(attributes.st_mode)) {
            err = 999;
            addIdamError(&idamerrorstack, CODEERRORTYPE, "openData", err, "Not a Regular File!");
            break;
        }

// Open for Copying (no native c function!)          

        if ((in = fopen(oldFile, "r")) == NULL || errno != 0) {
            err = 999;
            if (errno != 0) addIdamError(&idamerrorstack, SYSTEMERRORTYPE, "openData", errno, "");
            addIdamError(&idamerrorstack, CODEERRORTYPE, "openData", err, "Unable to Open the file for copying!");
            break;
        }

        if ((out = fopen(newFile, "w")) == NULL || errno != 0) {    // Replace if the file exists
            err = 999;
            if (errno != 0) addIdamError(&idamerrorstack, SYSTEMERRORTYPE, "openData", errno, "");
            addIdamError(&idamerrorstack, CODEERRORTYPE, "openData", err, "Unable to Open the file for writing!");
            break;
        }

        while ((count = fread(buffer, sizeof(char), BUFFERSIZE, in)) > 0 && errno == 0) {
            if ((n = fwrite(buffer, sizeof(char), count, out)) != count || errno != 0) {
                err = 999;
                if (errno != 0) addIdamError(&idamerrorstack, SYSTEMERRORTYPE, "openData", errno, "");
                if (n != count)
                    addIdamError(&idamerrorstack, CODEERRORTYPE, "openData", err, "Inconsistent byte count");
                addIdamError(&idamerrorstack, CODEERRORTYPE, "openData", err, "Unable to Write the file!");
                break;
            }
        }
        if (errno != 0) {
            err = 999;
            if (errno != 0) addIdamError(&idamerrorstack, SYSTEMERRORTYPE, "openData", errno, "");
            addIdamError(&idamerrorstack, CODEERRORTYPE, "openData", err, "Unable to Read and Write the file!");
            break;
        }

    } while (0);        // End of trap

    if (in != NULL) fclose(in);
    if (out != NULL) fclose(out);

    if (err != 0) {
        if (newFile != NULL) free(newFile);
        return err;
    }

// Set the file copy's modification time

    struct utimbuf mtime;

    mtime.actime = attributes.st_atime;    // Access time
    mtime.modtime = attributes.st_mtime;    // Modification time

    if ((rc = utime(newFile, &mtime)) != 0 || errno != 0) {
        err = 999;
        if (errno != 0) addIdamError(&idamerrorstack, SYSTEMERRORTYPE, "openData", errno, "");
        addIdamError(&idamerrorstack, CODEERRORTYPE, "openData", err, "Unable to Set the file copy's time stamp!");
    }

    if (newFile != NULL) free(newFile);

    return err;

}   


