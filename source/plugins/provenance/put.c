/*-----------------------------------------------------------------------------------------------
  PHP Extension for Provenance Web App (journal publication atomic entities)
  
  Issues:
  
  Overwritting previous files
  Files with the same name but from different directories
  Avoid punctuation within comments - may break the primitive parse (need to use regular expressions etc)
  
  To Do:
  sha1 hash of files
  
  uuid
  slave
  class
  origin
  ccfeowned
  description
  filelocation
  format
  os
  desktoputility
  processdescription
  inputdatafile
  duid
  outputdatafile
  codename
  version
  repository 
  host
  user
  modules
  libraries
  environment
  idamsignals
  exebinary
  runscript
  clargs
  tags
  status
*---------------------------------------------------------------------------------------------------------------*/
#include "put.h"

#include <sys/stat.h>
#include <openssl/sha.h>
#include <utime.h>
#include <stdlib.h>
#include <errno.h>
#include <strings.h>

#include <clientserver/stringUtils.h>
#include <clientserver/initStructs.h>
#include <clientserver/udaTypes.h>

#include "provenance.h"

int put(IDAM_PLUGIN_INTERFACE* idam_plugin_interface)
{

    int err = 0;
    int i;
    char* p;


    static char root[MAXROOT] = "";
    FILE* log = NULL;

    IDAM_LOG(UDA_LOG_DEBUG, "Provenance: entering function 'put'\n");

//----------------------------------------------------------------------------------------
// Standard v1 Plugin Interface

    DATA_BLOCK* data_block;
    REQUEST_BLOCK* request_block;

    if (idam_plugin_interface->interfaceVersion == 1) {

        idam_plugin_interface->pluginVersion = 1;

        data_block = idam_plugin_interface->data_block;
        request_block = idam_plugin_interface->request_block;

    } else {
        RAISE_PLUGIN_ERROR("Plugin Interface Version is Not Known: Unable to execute the request!");
    }

    IDAM_LOG(UDA_LOG_DEBUG, "Provenance: Plugin Interface transferred\n");

//----------------------------------------------------------------------------------------
// Provenance Data Archive location

    if (root[0] == '\0') {
        char* env = getenv("UDA_PROVENANCEROOT");
        if (env != NULL) {
            strncpy(root, env, MAXROOT - 1);
            root[MAXROOT - 1] = '\0';
            TrimString(root);
            IDAM_LOGF(UDA_LOG_DEBUG, "Provenance: Archive root directory [%s]\n", root);
        } else {
            RAISE_PLUGIN_ERROR("No Provenance Data Archive Root specified!");
        }
    }

//----------------------------------------------------------------------------------------
// Common Name Value pairs

// Keywords have higher priority

    char emptyString[1] = "";

    char* UID = NULL;                // Must be passed
    char* fileLocation = emptyString;
    char* inputDataFile = emptyString;
    char* outputDataFile = emptyString;
    char* modulesListFile = emptyString;
    char* lddLibraryFile = emptyString;
    char* envFile = emptyString;
    char* idamSignalsFile = emptyString;
    char* exeBinaryFile = emptyString;
    char* runScriptFile = emptyString;

    char* fileLocationName = fileLocation;
    char* inputDataFileName = inputDataFile;
    char* outputDataFileName = outputDataFile;
    char* modulesListFileName = modulesListFile;
    char* lddLibraryFileName = lddLibraryFile;
    char* envFileName = envFile;
    char* idamSignalsFileName = idamSignalsFile;
    char* exeBinaryFileName = exeBinaryFile;
    char* runScriptFileName = runScriptFile;

    unsigned short isReplace = 0;    // Replace any previous file within the archive directory
    unsigned short isReturnPath = 0;    // Return the path to the snapshot archive directory

    unsigned short isUID = 0, isFileLocation = 0, isInputDataFile = 0, isOutputDataFile = 0,
            isModulesListFile = 0, isLddLibraryFile = 0, isEnvFile = 0, isIdamSignalsFile = 0,
            isExeBinaryFile = 0, isRunScriptFile = 0;

    for (i = 0; i < request_block->nameValueList.pairCount; i++) {

// Pairs

        if (STR_IEQUALS(request_block->nameValueList.nameValue[i].name, "UUID") ||
            STR_IEQUALS(request_block->nameValueList.nameValue[i].name, "UID") ||
            STR_IEQUALS(request_block->nameValueList.nameValue[i].name, "DOI")) {
            isUID = 1;
            UID = request_block->nameValueList.nameValue[i].value;
            continue;
        }

        if (STR_IEQUALS(request_block->nameValueList.nameValue[i].name, "fileLocation")) {
            fileLocation = request_block->nameValueList.nameValue[i].value;
            isFileLocation = (strlen(fileLocation) > 0);
            if (isFileLocation && (p = strrchr(fileLocation, '/')) != NULL) fileLocationName = &p[1];
            continue;
        }
        if (STR_IEQUALS(request_block->nameValueList.nameValue[i].name, "inputDataFile")) {
            inputDataFile = request_block->nameValueList.nameValue[i].value;
            isInputDataFile = (strlen(inputDataFile) > 0);
            if (isInputDataFile && (p = strrchr(inputDataFile, '/')) != NULL) inputDataFileName = &p[1];
            continue;
        }
        if (STR_IEQUALS(request_block->nameValueList.nameValue[i].name, "outputDataFile")) {
            outputDataFile = request_block->nameValueList.nameValue[i].value;
            isOutputDataFile = (strlen(outputDataFile) > 0);
            if (isOutputDataFile && (p = strrchr(outputDataFile, '/')) != NULL) outputDataFileName = &p[1];
            continue;
        }

        if (STR_IEQUALS(request_block->nameValueList.nameValue[i].name, "modules")) {
            modulesListFile = request_block->nameValueList.nameValue[i].value;
            isModulesListFile = (strlen(modulesListFile) > 0);
            if (isModulesListFile && (p = strrchr(modulesListFile, '/')) != NULL) modulesListFileName = &p[1];
            continue;
        }
        if (STR_IEQUALS(request_block->nameValueList.nameValue[i].name, "libraries")) {
            lddLibraryFile = request_block->nameValueList.nameValue[i].value;
            isLddLibraryFile = (strlen(lddLibraryFile) > 0);
            if (isLddLibraryFile && (p = strrchr(lddLibraryFile, '/')) != NULL) lddLibraryFileName = &p[1];
            continue;
        }
        if (STR_IEQUALS(request_block->nameValueList.nameValue[i].name, "environment")) {
            envFile = request_block->nameValueList.nameValue[i].value;
            isEnvFile = (strlen(envFile) > 0);
            if (isEnvFile && (p = strrchr(envFile, '/')) != NULL) envFileName = &p[1];
            continue;
        }

        if (STR_IEQUALS(request_block->nameValueList.nameValue[i].name, "idamSignals")) {
            idamSignalsFile = request_block->nameValueList.nameValue[i].value;
            isIdamSignalsFile = (strlen(idamSignalsFile) > 0);
            if (isIdamSignalsFile && (p = strrchr(idamSignalsFile, '/')) != NULL) idamSignalsFile = &p[1];
            continue;
        }
        if (STR_IEQUALS(request_block->nameValueList.nameValue[i].name, "exeBinary")) {
            exeBinaryFile = request_block->nameValueList.nameValue[i].value;
            isExeBinaryFile = (strlen(exeBinaryFile) > 0);
            if (isExeBinaryFile && (p = strrchr(exeBinaryFile, '/')) != NULL) exeBinaryFileName = &p[1];
            continue;
        }
        if (STR_IEQUALS(request_block->nameValueList.nameValue[i].name, "runScript")) {
            runScriptFile = request_block->nameValueList.nameValue[i].value;
            isRunScriptFile = (strlen(runScriptFile) > 0);
            if (isRunScriptFile && (p = strrchr(runScriptFile, '/')) != NULL) runScriptFileName = &p[1];
            continue;
        }
        if (STR_IEQUALS(request_block->nameValueList.nameValue[i].name, "user")) {
            user = request_block->nameValueList.nameValue[i].value;
            continue;
        }
// keywords

        if (STR_IEQUALS(request_block->nameValueList.nameValue[i].name, "returnPath")) {
            isReturnPath = 1;
            continue;
        }

        if (STR_IEQUALS(request_block->nameValueList.nameValue[i].name, "Replace")) {
            isReplace = 1;
            continue;
        }

    }

    if (!isUID) {
        RAISE_PLUGIN_ERROR("No unique identifier passed!");
    }

//----------------------------------------------------------------------------------------
// Deconstruct lists into string arrays
/*      
   char **inputDataFileList  = NULL, **outputDataFileList  = NULL;
   char **inputDataFileNames = NULL, **outputDataFileNames = NULL;
   unsigned short inputDataFileListCount = 0;
   unsigned short outputDataFileListCount = 0;

   if(isInputDataFile)  getFileList(inputDataFile,  &inputDataFileList,  &inputDataFileNames,  &inputDataFileListCount);
   if(isOutputDataFile) getFileList(outputDataFile, &outputDataFileList, &outputDataFileNames, &outputDataFileListCount);
*/
//----------------------------------------------------------------------------------------
// Error trap

    err = 0;

    do {

//----------------------------------------------------------------------------------------
// Create a directory within the provenance archive to receive files and a log

        unsigned short priorDir = 0;    // Does the directory pre-exist?
        char* newDir = NULL;        // the archive directory

        if ((err = makeProvenanceDir(&newDir, root, UID, &priorDir)) != 0) break;

        if (err == 0) IDAM_LOGF(UDA_LOG_DEBUG, "Provenance Archive Directory: [%s]\n", newDir);

// Open a log file to record all activities and errors

        char* logName = (char*)malloc((strlen(newDir) + strlen(UID) + strlen(LOGEXT) + 3) * sizeof(char));
        sprintf(logName, "%s/%s.%s", newDir, UID, LOGEXT);

        unsigned short newDirLength = strlen(newDir) + 1;
        unsigned short uidLength = strlen(UID);
        for (i = newDirLength; i < newDirLength + uidLength; i++)
            if (logName[i] == '/') logName[i] = '-';

        if (isReplace) {
            log = fopen(logName, "w");    // Rewrite the log
        } else {
            log = fopen(logName, "a");
        }    // Append all entries to the log


        if (log == NULL || errno != 0) {
            err = 999;
            if (errno != 0) addIdamError(SYSTEMERRORTYPE, "Provenance", errno, "");
            addIdamError(CODEERRORTYPE, "Provenance", err, "Unable to Open the Provenance Log file!");
            IDAM_LOGF(UDA_LOG_DEBUG, "Provenance Error: Unable to Open the Provenance Log file! [%s]\n", logName);
            free((void*)logName);
            break;
        }
        if (logName != NULL) free((void*)logName);

//----------------------------------------------------------------------------------------
// Copy all files to the provenance archive
// Write all directives and snapshot copy outcomes to the log

        for (i = 0; i < request_block->nameValueList.pairCount; i++) {
            fprintf(log, "\n[%2d]: %s\n", i, request_block->nameValueList.nameValue[i].pair);
        }

        do {
            if (isFileLocation && (err = copyProvenanceWebFile(fileLocation, newDir, fileLocationName, log)) != 0)
                break;
            if (isModulesListFile &&
                (err = copyProvenanceWebFile(modulesListFile, newDir, modulesListFileName, log)) != 0)
                break;
            if (isLddLibraryFile &&
                (err = copyProvenanceWebFile(lddLibraryFile, newDir, lddLibraryFileName, log)) != 0)
                break;
            if (isEnvFile && (err = copyProvenanceWebFile(envFile, newDir, envFileName, log)) != 0) break;
            if (isIdamSignalsFile &&
                (err = copyProvenanceWebFile(idamSignalsFile, newDir, idamSignalsFileName, log)) != 0)
                break;
            if (isExeBinaryFile &&
                (err = copyProvenanceWebFile(exeBinaryFile, newDir, exeBinaryFileName, log)) != 0)
                break;
            if (isRunScriptFile &&
                (err = copyProvenanceWebFile(runScriptFile, newDir, runScriptFileName, log)) != 0)
                break;
            if (isInputDataFile &&
                (err = copyProvenanceWebFile(inputDataFile, newDir, inputDataFileName, log)) != 0)
                break;
            if (isOutputDataFile &&
                (err = copyProvenanceWebFile(outputDataFile, newDir, outputDataFileName, log)) != 0)
                break;
        } while (0);

        fflush(log);

        if (err != 0) {
            if (newDir != NULL) free(newDir);
            break;
        }

        initDataBlock(data_block);

        if (isReturnPath && newDir != NULL) {
            data_block->data = newDir;
            data_block->data_type = UDA_TYPE_STRING;
            data_block->rank = 0;
            data_block->data_n = strlen(newDir) + 1;
            strcpy(data_block->data_desc, "path to provenance records");
        } else {
            if (newDir != NULL) free(newDir);
            data_block->data = (char*)malloc(sizeof(char));
            data_block->data[0] = '\0';
            data_block->data_type = UDA_TYPE_CHAR;
            data_block->rank = 0;
            data_block->data_n = 1;
            strcpy(data_block->data_desc, "Nothing to return!");
        }

    } while (0);

    return err;
}

// Make a new Directory

int makeProvenanceDir(char** newDir, char* root, char* UID, unsigned short* priorDir)
{
    int rc, err = 0;
    char* p1 = NULL;

// Create a directory using the Unique Identifier
// directory = Provenance Archive Root + UID

    *priorDir = 0;
    *newDir = (char*)malloc((strlen(root) + strlen(UID) + 2) * sizeof(char));
    sprintf(*newDir, "%s/%s", root, UID);

    char* p = *newDir;        // root must already exist!
    unsigned short offset = strlen(root) + 1;

    while ((p1 = strchr(&p[offset], '/')) != NULL) {
        p1[0] = '\0';

// does the archive directory exist? If not then create it. Ensure correct permissions: read only for non owner.

        errno = 0;
        rc = mkdir(p, S_IRUSR | S_IWUSR | S_IXUSR |
                      S_IRGRP | S_IWGRP | S_IXGRP |
                      S_IROTH | S_IWOTH | S_IXOTH);
        if (errno == EEXIST) {
            errno = 0;            // Exists so ignore
            rc = 0;
        }
        p1[0] = '/';
        offset = &p1[1] - p;

        if (rc != 0 || errno != 0)break;
    }

    if (rc == 0 && errno == 0) {
        rc = mkdir(p, S_IRUSR | S_IWUSR | S_IXUSR |
                      S_IRGRP | S_IWGRP | S_IXGRP |
                      S_IROTH | S_IWOTH | S_IXOTH);
        if (errno == EEXIST) {
            *priorDir = 1;            // Exists so ignore, but flag as pre-existing
            errno = 0;
            rc = 0;
        }
    }

    if (rc != 0 || errno != 0) {
        err = 999;
        if (errno != 0) addIdamError(SYSTEMERRORTYPE, "Provenance", errno, "");
        addIdamError(CODEERRORTYPE, "Provenance", err,
                     "Unable to create a Provenance Archive Directory!");
        if (*newDir != NULL) free(*newDir);
        *newDir = NULL;
    } else
        sprintf(*newDir, "%s/%s", root, UID);

    return err;
}

// Extract file list
// Expand the path using prior full entry

void getFileList(char* list, char*** fileList, char*** fileNames, unsigned short* fileListCount)
{

    unsigned int i, count = 0, pathLength = 0;
    *fileListCount = 0;
    *fileList = NULL;
    *fileNames = NULL;
    if (list == NULL || list[0] == '\0') return;

    char* path = NULL;

// How many files?

    char* p = NULL, * p0 = NULL, * p1 = NULL;
    count = 1;
    p0 = list;
    while ((p = strchr(p0, ',')) != NULL) {
        count++;                // Comma separated list
        p0 = &p[1];
    }

// Allocate array

    *fileList = (char**)malloc(count * sizeof(char*));
    *fileNames = (char**)malloc(count * sizeof(char*));

// Deconstruct the list, checking for a full path - adding if missing 

    i = 0;
    p0 = list;
    p1 = list;
    while ((p = strchr(p0, ',')) != NULL) {
        p[0] = '\0';

        if (p0[0] == '/' && (p1 = strrchr(p0, '/')) != p0) {
            pathLength = p1 - p0;
            if (path != NULL) free((void*)path);
            path = (char*)malloc((pathLength + 1) * sizeof(char));
            strncpy(path, p0, pathLength);                        // Reset the latest path
            path[pathLength] = '\0';
        }

        if ((p1 = strrchr(p0, '/')) != NULL || path == NULL) {            // contains the path
            (*fileList)[i] = (char*)malloc((strlen(p0) + 1) * sizeof(char));
            strcpy((*fileList)[i], p0);
            (*fileNames)[i] = (char*)malloc(
                    (strlen(&p1[1]) + 1) * sizeof(char));        // The file's name (needed for dup test)
            strcpy((*fileNames)[i], &p1[1]);
        } else {                                    // Add the previous path
            (*fileList)[i] = (char*)malloc((strlen(p0) + pathLength + 2) * sizeof(char));
            sprintf((*fileList)[i], "%s/%s", path, p0);
            (*fileNames)[i] = (char*)malloc((strlen(p0) + 1) * sizeof(char));
            strcpy((*fileNames)[i], p0);
        }
        p0 = &p[1];
        i++;
    }

    if ((p1 = strrchr(p0, '/')) != NULL || path == NULL) {
        (*fileList)[i] = (char*)malloc((strlen(p0) + 1) * sizeof(char));
        strcpy((*fileList)[i], p0);
        (*fileNames)[i] = (char*)malloc((strlen(&p1[1]) + 1) * sizeof(char));
        strcpy((*fileNames)[i], &p1[1]);
    } else {
        (*fileList)[i] = (char*)malloc((strlen(p0) + pathLength + 2) * sizeof(char));
        sprintf((*fileList)[i], "%s/%s", path, p0);
        (*fileNames)[i] = (char*)malloc((strlen(p0) + 1) * sizeof(char));
        strcpy((*fileNames)[i], p0);
    }
    *fileListCount = i + 1;
    if (path != NULL) free((void*)path);

    return;
}

// Free file list

void freeFileNameList(char*** list, unsigned short fileCount)
{
    unsigned int i;
    for (i = 0; i < fileCount; i++) if ((*list)[i] != NULL) free((void*)(*list)[i]);
    free((void*)*list);
    *list = NULL;
}

int fileSHA1(char* file, unsigned char* out)
{
    unsigned char buf[8192];
    SHA_CTX sc;
    FILE* f = fopen(file, "rb");
    if (f == NULL) return -1;
    SHA1_Init(&sc);
    for (;;) {
        size_t len;
        len = fread(buf, 1, sizeof buf, f);
        if (len == 0) break;
        SHA1_Update(&sc, buf, len);
    }
    int err = ferror(f);
    fclose(f);
    if (err) return -1;
    SHA1_Final(out, &sc);
    return 0;
}

// Copy files from a remote workstation (web server unable to preserve time stamps of the original?)
// oldFile	- the remote temporary location of the file to be copied (not mounted!)
// dir		- the local mounted archive directory
// newFileName	- the name of the copy

// Files are on different storage - need to scp across 

// scp -p -B user@remote:oldFile newFile > /dev/null 2>&1		[Batch mode, password-less login enabled, all messages ignored]

/* notes on password-less scp

#1 Create public and private keys using ssh-key-gen on local-host
	on idam0 as root
	ssh-keygen
	save to /home/dgm/.ssh/id_rsa
#2 Copy the public key to remote-host using ssh-copy-id
	ssh-copy-id -i ~/.ssh/id_rsa.pub tm-icat1.fusion.ccfe.ac.uk
#3 Login to remote-host without entering the password
	ssh -Y dgm@tm-icat1.fusion.ccfe.ac.uk
	** success !!!
#4 su root
#5 copy /home/dgm/.ssh/id_rsa* to idam0 in root/.ssh
#6 Login to remote-host without entering the password
	ssh -Y dgm@tm-icat1.fusion.ccfe.ac.uk
	** success !!!	
	
idam on idam0 & idam1 executes as user "idamuser"

create accounts on tm-icat1: idamuser, dgm

root> useradd --home /home/idamuser idamuser
root> passwd --stdin idamuser
root> qwerty

idamuser needs to have permission to write to /projects/provenance	

*/

int copyProvenanceWebFile(char* oldFile, char* dir, char* newFileName, FILE* log)
{
    int err = 0;
    char* p = NULL;
    FILE* ph = NULL;
    char cmd[CMDSIZE];
    char* copyFile = NULL;
    char* newFile = NULL;
    static struct timeval tv_start, tv_stop;    // Performance
    int msecs, usecs;

// Has the file been copied to the web server?

    char* webSCP = getenv("UDA_PROVWEBSCP");
    char* webURL = getenv("UDA_PROVWEBURL");
    char* webDir = getenv("UDA_PROVWEBDIR");
    unsigned int isWebCopy = (webURL != NULL && webDir != NULL) && !strncmp(webURL, oldFile, strlen(webURL));

    do {        // Error Trap

// Target and Copy files

        if (isWebCopy) {
            unsigned int lstr = strlen(webURL);
            p = &oldFile[lstr];
            if (p[0] == '/') p = &oldFile[lstr + 1];        // drop leading '/' character
            copyFile = (char*)malloc((strlen(webDir) + strlen(p) + 2) * sizeof(char));
            sprintf(copyFile, "%s/%s", webDir, p);
        } else {
            copyFile = oldFile;
        }

        newFile = (char*)malloc((strlen(dir) + strlen(newFileName) + 2) * sizeof(char));
        sprintf(newFile, "%s/%s", dir, newFileName);

        gettimeofday(&tv_start, NULL);
        IDAM_LOGF(UDA_LOG_DEBUG, "\nProvenance Put\nTarget file to copy - %s\nCopy location - %s\n\n", copyFile,
                  newFile);

        if (log != NULL) fprintf(log, "Snapshot copy of [%s] to [%s]\n", copyFile, newFile);

// Copy the file

        if (webSCP != NULL)
            sprintf(cmd, "scp -p -B %s:%s %s > /dev/null 2>&1", webSCP, copyFile, newFile);
        else
            sprintf(cmd, "scp -p -B %s:%s %s > /dev/null 2>&1", WEBSCP, copyFile, newFile);

        errno = 0;
        ph = popen(cmd, "r");

        if (ph == NULL || errno != 0) {
            err = 999;
            if (errno != 0) addIdamError(SYSTEMERRORTYPE, "Provenance", errno, "");
            addIdamError(CODEERRORTYPE, "Provenance", err, "Cannot copy file to data archive");
            IDAM_LOGF(UDA_LOG_DEBUG, "Error: Unable to copy the file! [%s]\n", copyFile);
            if (log != NULL) fprintf(log, "Error: Unable to copy the file!\n");
            break;
        }

        gettimeofday(&tv_stop, NULL);
        msecs = (int)(tv_stop.tv_sec - tv_start.tv_sec) * 1000 + (int)(tv_stop.tv_usec - tv_start.tv_usec) / 1000;
        usecs = (int)(tv_stop.tv_sec - tv_start.tv_sec) * 1000000 + (int)(tv_stop.tv_usec - tv_start.tv_usec);
        IDAM_LOGF(UDA_LOG_DEBUG, "Provenance: scp Cost %d (ms), %d (microsecs)\n", msecs, usecs);
        tv_start = tv_stop;
        if (log != NULL) fprintf(log, "Copy cost %d (ms), %d (microsecs)\n", msecs, usecs);

        pclose(ph);        // wait for pipe cost ~ 20ms !

// Hash the file

    } while (0);        // End of trap


// Housekeeping

    if (newFile != NULL) free(newFile);
    if (isWebCopy && copyFile != NULL) free(copyFile);

    return err;

}


// Copy files (shared file system, e.g. web server files mounted)
// sha1 hash sum before and after copy
//	if hashes agree and if copied from the web server, delete the oldFile 
// oldFile	- the temporary location of the file to be copied
// dir		- the archive directory
// newFileName	- the name of the copy

int copyProvenanceFile(char* oldFile, char* dir, char* newFileName)
{
    int i, err = 0;
    char* p = NULL;
    char buffer[BUFFERSIZE];
    size_t rc, count, n;
    FILE* in = NULL, * out = NULL;
    struct stat attributes;
    char* copyFile = NULL;
    char* newFile = NULL;
    unsigned char oldHash[HASHLENGTH];
    unsigned char newHash[HASHLENGTH];

// Has the file been copied to the web server?

    char* webURL = getenv("UDA_PROVWEBURL");
    char* webDir = getenv("UDA_PROVWEBDIR");
    unsigned int isWebCopy = (webURL != NULL && webDir != NULL) && !strncmp(webURL, oldFile, strlen(webURL));

    do {        // Error Trap

// Target and Copy files

        if (isWebCopy) {
            unsigned int lstr = strlen(webURL);
            p = &oldFile[lstr];
            if (p[0] == '/') p = &oldFile[lstr + 1];        // drop leading '/' character
            copyFile = (char*)malloc((strlen(webDir) + strlen(p) + 2) * sizeof(char));
            sprintf(copyFile, "%s/%s", webDir, p);
        } else {
            copyFile = oldFile;
        }

        newFile = (char*)malloc((strlen(dir) + strlen(newFileName) + 2) * sizeof(char));
        sprintf(newFile, "%s/%s", dir, newFileName);

        IDAM_LOGF(UDA_LOG_DEBUG, "\nProvenance Put\nTarget file to copy - %s\nCopy location - %s\n\n", copyFile,
                  newFile);

// Read file attributes

        errno = 0;
        if ((rc = stat(copyFile, &attributes)) != 0 || errno != 0) {
            err = 999;
            if (errno != 0) addIdamError(SYSTEMERRORTYPE, "Provenance", errno, "");
            addIdamError(CODEERRORTYPE, "Provenance", err, "Unable to read the file's attributes!");
            IDAM_LOGF(UDA_LOG_DEBUG, "Error: Unable to read the file's attributes! [%s] %s\n", copyFile,
                      strerror(errno));
            break;
        }

// Check the type (regular file only)

        if (!S_ISREG(attributes.st_mode)) {
            err = 999;
            addIdamError(CODEERRORTYPE, "Provenance", err, "Not a Regular File!");
            IDAM_LOGF(UDA_LOG_DEBUG, "Error: Not a Regular File! [%s]\n", copyFile);
            break;
        }

// Open for Copying (no native c function!)          

        if ((in = fopen(copyFile, "rb")) == NULL || errno != 0) {
            err = 999;
            if (errno != 0) addIdamError(SYSTEMERRORTYPE, "Provenance", errno, "");
            addIdamError(CODEERRORTYPE, "Provenance", err, "Unable to Open the file for copying!");
            IDAM_LOGF(UDA_LOG_DEBUG, "Error: Unable to Open the file for copying! [%s]\n", copyFile);
            break;
        }

        if ((out = fopen(newFile, "w")) == NULL || errno != 0) {    // Replace if the file exists
            err = 999;
            if (errno != 0) addIdamError(SYSTEMERRORTYPE, "Provenance", errno, "");
            addIdamError(CODEERRORTYPE, "Provenance", err, "Unable to Open the file for writing!");
            IDAM_LOGF(UDA_LOG_DEBUG, "Error: Unable to Open the file for writing! [%s]\n", newFile);
            break;
        }

        while ((count = fread(buffer, sizeof(char), BUFFERSIZE, in)) > 0 && errno == 0) {
            if ((n = fwrite(buffer, sizeof(char), count, out)) != count || errno != 0) {
                err = 999;
                if (errno != 0) addIdamError(SYSTEMERRORTYPE, "Provenance", errno, "");
                if (n != count) {
                    addIdamError(CODEERRORTYPE, "Provenance", err, "Inconsistent byte count");
                }
                addIdamError(CODEERRORTYPE, "Provenance", err, "Unable to Write the file!");
                IDAM_LOGF(UDA_LOG_DEBUG, "Error: Unable to Write the file! [%s]\n", newFile);
                break;
            }
        }
        if (errno != 0) {
            err = 999;
            if (errno != 0) addIdamError(SYSTEMERRORTYPE, "Provenance", errno, "");
            addIdamError(CODEERRORTYPE, "Provenance", err, "Unable to Read and Write the file!");
            IDAM_LOGF(UDA_LOG_DEBUG, "Error: Unable to Read and Write the file! [%s, %s]\n", copyFile, newFile);
            break;
        }

        if (in != NULL) fclose(in);
        if (out != NULL) fclose(out);
        in = NULL;
        out = NULL;

// Set the file copy's modification time

        struct utimbuf mtime;

        mtime.actime = attributes.st_atime;    // Access time
        mtime.modtime = attributes.st_mtime;    // Modification time

        if ((rc = utime(newFile, &mtime)) != 0 || errno != 0) {
            err = 999;
            if (errno != 0) addIdamError(SYSTEMERRORTYPE, "Provenance", errno, "");
            addIdamError(CODEERRORTYPE, "Provenance", err,
                         "Unable to Set the file copy's time stamp!");
            IDAM_LOGF(UDA_LOG_DEBUG, "Error: Unable to Set the file copy's time stamp! [%s]\n", newFile);
            break;
        }

// Hash the files and compare

        if ((rc = fileSHA1(copyFile, oldHash)) != 0) {
            err = 999;
            if (errno != 0) addIdamError(SYSTEMERRORTYPE, "Provenance", errno, "");
            addIdamError(CODEERRORTYPE, "Provenance", err, "Unable to hash the original file!");
            IDAM_LOGF(UDA_LOG_DEBUG, "Error: Unable to hash the original file! [%s]\n", copyFile);
            break;
        }
        if ((rc = fileSHA1(newFile, newHash)) != 0) {
            err = 999;
            if (errno != 0) addIdamError(SYSTEMERRORTYPE, "Provenance", errno, "");
            addIdamError(CODEERRORTYPE, "Provenance", err, "Unable to hash the file copy!");
            IDAM_LOGF(UDA_LOG_DEBUG, "Error: Unable to hash the file copy! [%s]\n", newFile);
            break;
        }

        rc = 0;
        for (i = 0; i < HASHLENGTH; i++) rc += oldHash[i] != newHash[i];
        if (rc != 0) {
            err = 999;
            if (errno != 0) addIdamError(SYSTEMERRORTYPE, "Provenance", errno, "");
            addIdamError(CODEERRORTYPE, "Provenance", err,
                         "The original file and the file copy have different hash sums!");
            IDAM_LOG(UDA_LOG_DEBUG, "Error: The original file and the file copy have different hash sums!\n");
            break;
        }

        IDAM_LOG(UDA_LOG_DEBUG, "\nProvenance Put\n: Hash sums checked OK\n");

    } while (0);        // End of trap

// Housekeeping

    if (in != NULL) fclose(in);
    if (out != NULL) fclose(out);

    if (newFile != NULL) free(newFile);
    if (isWebCopy && copyFile != NULL) free(copyFile);

    return err;

}   
