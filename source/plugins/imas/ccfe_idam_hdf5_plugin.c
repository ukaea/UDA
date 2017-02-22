// IDAM interface library
// Wrappers to ual_low_level_hdf5

/*
Observations:

expIdx or idx	- reference to the open data file: index into an array of open files - hdf5Files[idx]  
cpoPath	- the root group where the CPO/IDS is written
path	- the path relative to the root (cpoPath) where the data are written (must include the variable name!)
data	- the data to be written

HDF5_BASE - an environment variable defining the root directory of the HDF5 data archive
HDF5_MODEL_BASE - an environment variable defining a model HDF5 file

hdf5EuitmCreate - copies the HDF5 model file to a new file and opens it
hdf5EuitmOpen - opens a file

Concerns:

What does the HDF5 model file contain. Probably all group hierarchies for each IDS.
Is this a further limitation to versioning structures
Why should the user be concerned about what type of file to open!
There are no model files in the git repo

hdf5EuitmCreate may create a new file and overwrite a previous file!
TODO: Check for existing files

hdf5EuitmCreate calls the system() function
This is a security risk
Additional system commands may be included in the passed name strings
There are 2048 bytes to fill with code!
TODO: Need to use IDAM function that check filenames from getHdf5FileName and getHdf5ModelName are standards compliant
      use IsLegalFilePath(char *str) from IDAM TrimString utilities
      Done!

There is no mechanism for opening a previously existing file that is already open!
TODO: Add name details to the file handle log and check for open files before opening a new file
      Use the IDAM managePluginFiles utilities
      Done!
      
TODO: The source argument for idamGetAPI is empty (default server) - this should be a targeted source!  

**** THERE ARE NO UNIT TESTS FOR THE LOW LEVEL ROUTINES!!!!
**** All putSlice functions have a 'double time' argument that is not used! 

For Scalar putDataSlice, rank=0, shape=NULL
For array 		 rank=n, shape=same shape as the original
   

Planned actions:

Change over file management to the IDAM plugin system
Add additional types
Review how strings are handled
Review default values for missing data: NaN rather than the arbitrary extreme values chosen in IMAS, e.g. EMPTY_INT 
Locate all Object specific code to the IDAM plugin
Investigate the object system - is this just a local cache?


Change History

21Apr2015 dgmuir	Original version based on ual_low_level_hdf5 from ITER git master repo
			All original source code reused where possible
			HDF5 specific code relocated to an IDAM put plugin
			Minimal code changes made: No renaming; No change to arguments
			
			Added initHdf5File to initialise array 
			Modified hdf5EuitmCreate to call initHdf5File on startup 
			Modified hdf5EuitmOpen to call initHdf5File on startup
			Added hdf5IMASCreate to create a new file without a model
			
			Commented out TranslateLogical as external dependence
			Modified getHdf5FileName to use getenv instead of TranslateLogical 
			Modified getHdf5ModelName to use getenv instead of TranslateLogical 
			
			bug fix: function hdf5GetDimension has no type - adding type int 
			
			Pointer errmsg was undefined - create locate string and assign errmsg to it.
			declared extern - changed to static
			
			Added IDAM error log to pass back errors and messages
			
			Added IDAM state variable: static unsigned short isCloseRegistered 
			
			Added function static int findHdf5Idx(int idam_id)
			
			Added functionality to create missing groups to putData
			
			Added function findIMASType and an UNKNOWN_TYPE
			
			Added function createGroup with #define MAX_TOKENS
			
			Added function findIMASIDAMType
			
			putData renamed imas_putData 
			New function putData created that calls IDAM to replace original
			
			putDataSlice renamed imas_putDataSlice 
			New function putDataSlice created that calls IDAM to replace original
			
			Added functionality to create missing groups to imas_putDataSlice, imas_replaceLastDataSlice

			getData renamed imas_getData 
			New function getData created that calls IDAM to replace original

			getDataSlices renamed imas_getDataSlices 
			New function getDataSlices created that calls IDAM to replace original

			hdf5GetDimension renamed imas_hdf5GetDimension 
			New function hdf5GetDimension created that calls IDAM to replace original
			
			hdf5DeleteData renamed imas_hdf5DeleteData
			New function hdf5DeleteData created that calls IDAM to replace original

			new function hdf5imasOpen wrapper to hdf5EuitmOpen
			hdf5EuitmOpen renamed imas_hdf5EuitmOpen
			New function hdf5EuitmOpen created that calls IDAM to replace original
			
			new function hdf5imasCreate wrapper to hdf5EuitmCreate
			hdf5EuitmCreate renamed imas_hdf5EuitmCreate
			New function hdf5EuitmCreate created that calls IDAM to replace original
			IsLegalFilePath used to test passed file name before system level api called
			
			hdf5IMASCreate renamed imas_hdf5IMASCreate
			New function hdf5IMASCreate created that calls IDAM to replace original

	imas_hdf5BeginObject:sliceIdx1 
	imas_hdf5GetObjectSlice: sliceIdx1, sliceIdx2, sliceTime1, sliceTime2
	replicate these static variables in the plugin environment
	
30Jun2015 dgm	Added IDS version to the file name	

30Jul2015 dgm	Added H5Eset_auto2 to function initHdf5File to stop warning messages

*/

#include <hdf5.h>
#include <stdlib.h>
#include <string.h>

#include "idamclientpublic.h"

#include <time.h>
#include <openssl/sha.h>
#include <stringUtils.h>
#include <managePluginFiles.h>
#include <unistd.h>

#include "imas.h"
#include "ccfe_idam_hdf5_plugin.h"

#define MAX_FILES   10000
#define MAX_STRINGS 20000
#define SLICE_BLOCK 1

#define MAX_TOKENS  64

#define STRING          1
#define STRING_VECTOR   5

#define UNKNOWN_TYPE 0

IDAMPLUGINFILELIST * pluginFileList;

const int EMPTY_INT = -999999999;
const float EMPTY_FLOAT = -9.0E35;
const double EMPTY_DOUBLE = -9.0E40;

static hid_t hdf5Files[MAX_FILES];
static char ErrMsg[MAX_STRINGS];
static char * errmsg = &ErrMsg[0];

static unsigned short hdf5Startup = 1;            // IMAS file register state
static unsigned short isCloseRegistered = 0;        // IDAM state variable - close function is registered

static int sliceIdx1, sliceIdx2;
static double sliceTime1, sliceTime2;

// SHA1 Hash =====================================================================================================================

#define PARTBLOCKINIT   1
#define PARTBLOCKUPDATE 2
#define PARTBLOCKOUTPUT 3
#define FILEBUFFERSIZE  1024

// include libcrypto.so in link step
// The hash output is 20 bytes -> 40 bytes hex
// http://linux.die.net/man/3/sha1_update

// Hash a complete data block 
void sha1Block(unsigned char * block, size_t blockSize, unsigned char * md)
{
    SHA1(block, blockSize, md);
}

// Hash a set of data blocks
void sha1PartBlock(unsigned char * partBlock, size_t partBlockSize, unsigned char * md, unsigned int state)
{
    static SHA_CTX sc;
    if (state == PARTBLOCKINIT)
        SHA1_Init(&sc);
    else if (state == PARTBLOCKUPDATE)
        SHA1_Update(&sc, partBlock, partBlockSize);
    else if (state == PARTBLOCKOUTPUT)
        SHA1_Final(md, &sc);
}

// Hash a file
int sha1File(char * name, FILE * fh, unsigned char * md)
{
    FILE * f;
    unsigned char buf[FILEBUFFERSIZE];
    SHA_CTX sc;
    int err;
    if (fh == NULL) {        // Is the file already open? If not Open
        f = fopen(name, "rb");
        if (f == NULL) {
            return -1;
        }
    }
    SHA1_Init(&sc);
    size_t len;
    for (; ;) {
        len = fread(buf, 1, FILEBUFFERSIZE, f);
        if (len == 0) break;
        SHA1_Update(&sc, buf, len);
    }
    err = ferror(f);
    fclose(f);
    if (err) {
        return -1;
    }
    SHA1_Final(md, &sc);
    return 0;
}
// SHA1 Hash =====================================================================================================================

// File Copy =====================================================================================================================
/*
#include <fcntl.h>
#include <unistd.h>
#if defined(__APPLE__) || defined(__FreeBSD__)
   #include <copyfile.h>
#else
   #include <sys/sendfile.h>
#endif

int OSCopyFile(const char* source, const char* destination)
{    
    int input, output;    
    if ((input = open(source, O_RDONLY)) == -1)
    {
        return -1;
    }    
    if ((output = open(destination, O_RDWR | O_CREAT)) == -1)
    {
        close(input);
        return -1;
    }

    //Here we use kernel-space copying for performance reasons
#if defined(__APPLE__) || defined(__FreeBSD__)
    //fcopyfile works on FreeBSD and OS X 10.5+ 
    int result = fcopyfile(input, output, 0, COPYFILE_ALL);
#else
    //sendfile will work with non-socket output (i.e. regular file) on Linux 2.6.33+
    off_t bytesCopied = 0;
    struct stat fileinfo = {0};
    fstat(input, &fileinfo);
    int result = sendfile(output, input, &bytesCopied, fileinfo.st_size);
#endif

    close(input);
    close(output);

    return result;
}
*/
//================================================================================================================================

IDAMPLUGINFILELIST * getImasPluginFileList()
{
    return pluginFileList;
}

char * getImasErrorMsg()
{
    return errmsg;
}

void openDebug()
{
    errout = 0;
    if (dbgout == NULL) dbgout = fopen("/scratch/imas/Debug.txt", "w");
    if (errout == 0 && dbgout != NULL) debugon = 1;
}

// dgm  Assign static variable values

#ifndef DGMSKIP

void setSliceIdx(int idx1, int idx2)
{
    sliceIdx1 = idx1;
    sliceIdx2 = idx2;
}

void setSliceTime(double time1, double time2)
{
    sliceTime1 = time1;
    sliceTime2 = time1;
}

#endif
// dgm  Convert name to IMAS type

int findIMASType(char * typeName)
{
    if (typeName == NULL) return (UNKNOWN_TYPE);
    //if(STR_IEQUALS(typeName, "byte"))     return TYPE_CHAR;
    //if(STR_IEQUALS(typeName, "char"))     return TYPE_CHAR;
    //if(STR_IEQUALS(typeName, "short"))    return TYPE_SHORT;
    if (STR_IEQUALS(typeName, "int")) return INT;
    //if(STR_IEQUALS(typeName, "int64"))    return TYPE_LONG64;
    if (STR_IEQUALS(typeName, "float")) return FLOAT;
    if (STR_IEQUALS(typeName, "double")) return DOUBLE;
    //if(STR_IEQUALS(typeName, "ubyte"))    return TYPE_UNSIGNED_CHAR;
    //if(STR_IEQUALS(typeName, "ushort"))   return TYPE_UNSIGNED_SHORT;
    //if(STR_IEQUALS(typeName, "uint"))     return TYPE_UNSIGNED_INT;
    //if(STR_IEQUALS(typeName, "uint64"))   return TYPE_UNSIGNED_LONG64;
    //if(STR_IEQUALS(typeName, "text"))     return TYPE_STRING;
    if (STR_IEQUALS(typeName, "string")) return STRING;
    //if(STR_IEQUALS(typeName, "vlen"))     return TYPE_VLEN;
    //if(STR_IEQUALS(typeName, "compound")) return TYPE_COMPOUND;
    //if(STR_IEQUALS(typeName, "opaque"))   return TYPE_OPAQUE;
    //if(STR_IEQUALS(typeName, "enum"))     return TYPE_ENUM;
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
        case STRING_VECTOR:
            return TYPE_STRING;
    }
    return TYPE_UNKNOWN;
}

// Create missing and Intermediate Groups

hid_t createGroup(hid_t rootId, char * pathName)
{
    char fullName[MAX_STRING_LENGTH], groupPath[MAX_STRING_LENGTH];
    char * tokens[MAX_TOKENS], * groupName;
    int i, tokenCount;
    hid_t parentId, groupId;

// parse path name (eg /group1/group2/group3) into tokens delimited by '/'

    strcpy(fullName, pathName);
    i = 0;
    tokens[i] = strtok(fullName, "/");
    while (tokens[i]) {
        i++;
        if (i > MAX_TOKENS) return -1;
        tokens[i] = strtok(NULL, "/");
    }
    tokenCount = i;

// find or create a group for each token 

    groupId = rootId;
    groupPath[0] = '\0';

    for (i = 0; i < tokenCount; i++) {
        groupName = tokens[i];
        sprintf(groupPath, "%s/%s", groupPath, groupName);
        parentId = groupId;
        groupId = H5Gcreate(parentId, (const char *) groupName, H5P_DEFAULT, H5P_DEFAULT, H5P_DEFAULT);
        if (groupId < 0) {    // dgm Assume group exists
            groupId = H5Gopen(parentId, (const char *) groupName, H5P_DEFAULT);    // Open if already existing
            if (groupId < 0) return groupId;                    // Fail if not open
        }
    }
    return groupId;
}

// dgm	Find the file's IDX from the IDAM entry

int findHdf5Idx(int idam_id)
{
    int i;
    for (i = 0; i < MAX_FILES && hdf5Files[i]; i++) {
        if (i < pluginFileList->count && hdf5Files[i] == pluginFileList->files[idam_id].handleInt) return i;
    }
    return -1;
}

// dgm	Check the file's IDX is logged in IMAS and IDAM

int checkHdf5Idx(int idx)
{

//openDebug();

    int i;
//idamLog(LOG_DEBUG, "checkHdf5Idx: hdf5Files[%d]=%d/n", idx, hdf5Files[idx]);
    if (hdf5Files[idx] >= 0) {
        for (i = 0; i < pluginFileList->count; i++) {
//idamLog(LOG_DEBUG, "checkHdf5Idx: %d   %d/n", hdf5Files[idx], pluginFileList->files[i].handleInt);
            if (hdf5Files[idx] == pluginFileList->files[i].handleInt) return i;
        }
    }
//if(debugon)fflush(dbgout);    
    return -1;
}

int findFirstHdf5Idx()
{
    int i;

    for (i = 0; i < MAX_FILES && hdf5Files[i]; i++);
    if (i == MAX_FILES) {
        sprintf(errmsg, "No more HDF5 files available");
        return -1;
    }
    return i;
}

// dgm extern char *TranslateLogical(char *);   external dependency
// Separate location to the Model files

char * getHdf5FileName(char * filename, int shot, int run)
{
    static char outName[2048];

// dgm    char *base = TranslateLogical("HDF5_BASE");
    char * base = getenv("HDF5_BASE");

    if (base && *base)
        sprintf(outName, "%s/%s_%d_%d.hd5", base, filename, shot, run);
    else
        sprintf(outName, "%s_%d_%d.hd5", filename, shot, run);        // Local directory
// dgm    free(base);

// dgm TODO check outName is standards compliant containing no hidden system commands (not a comprehensive test!)
    if (IsLegalFilePath(outName)) return outName;

    return NULL;
}

// dgm path to a Model File
// dgm Add an extra level to the model location directory path - the IDS version
// dgm Assume each model file is named after a device - contains static device specific data (not shot range specific) 

char * getHdf5ModelName(char * filename)
{
    static char outName[2048];

// dgm    char *base = TranslateLogical("HDF5_MODEL_BASE");
    char * base = getenv("HDF5_MODEL_BASE");

// dgm Get the IDS version and Device 
    char * imasIdsVersion = getImasIdsVersion();
    char * imasIdsDevice = getImasIdsDevice();

    if (base && *base) {
        if (imasIdsVersion != NULL && imasIdsDevice != NULL)
            sprintf(outName, "%s/%s/%s/%s_model.hd5", base, imasIdsVersion, imasIdsDevice, filename);
        else
            sprintf(outName, "%s/%s_model.hd5", base, filename);    // default version with no device specific data
    } else
        sprintf(outName, "%s_model.hd5", filename);
// dgm    free(base);
// dgm TODO check outName is standards compliant containing no hidden system commands
    if (IsLegalFilePath(outName)) return outName;
    return NULL;
}

void releaseHdf5File(int idx)
{
    hdf5Files[idx] = 0;
}

void initHdf5File()
{
    int i;
    for (i = 0; i < MAX_FILES; i++) hdf5Files[i] = 0;

// Swtich off error/warning messages
    H5Eset_auto2(H5E_DEFAULT, NULL, NULL);
}

// dgm Create an HDF5 Model file

int imas_hdf5IdsModelCreate(char * filename, int version)
{

    hid_t rootId;

    if (!IsLegalFilePath(filename)) {
        sprintf(errmsg, "Error creating HDF5 Model file: illegal filename");
        return -1;
    }

// Open 

    char * model = getHdf5ModelName(filename);

    if ((rootId = H5Fcreate((const char *) model, H5F_ACC_EXCL, H5P_DEFAULT, H5P_DEFAULT)) < 0) {
        sprintf(errmsg, "Error creating HDF5 Model file %s", model);
        return -1;
    }

// Write Model: ids groups

    createGroup(rootId, "pf_active");
    createGroup(rootId, "pf_passive");
    createGroup(rootId, "core_profiles");

// Code to build the Model from missing groups using Debug.txt
// Dedupe without losing the order with.... 
// awk '!seen[$0]++' /scratch/imas/Debug.txt > rep.c
#include "./rep.inc"

// Close the file

    H5Fclose(rootId);
    return 0;
}

// dgm Original HDF5 hdf5EuitmCreate renamed imas_hdf5EuitmCreate - called by the IDAM IMAS plugin
// *** security concern: uses a system call to copy a file
// dgm Create from a Model file
// dgm Select the model based on IDS version and Device name
// dgm Add meta data to the file

int imas_hdf5EuitmCreate(char * name, int shot, int run, int refShot, int refRun, int * retIdx)
{
    if (hdf5Startup) {
        initHdf5File();
        hdf5Startup = 0;
    }
// Register the API function needed to close HDF5 files with the IDAM file manager
// herr_t H5Fclose(hid_t files_id);
// herr_t and hid_t are both integers
// The return code herr_t in an integer (ref: H5public.h) - use the Integer specific API

    if (!isCloseRegistered) {
        int (* close)(int);    // Function pointer
        close = &H5Fclose;
        registerIdamPluginFileClose(pluginFileList, (void *) close);
        isCloseRegistered = 1;
    }

// Is the HDF5 file already open for reading? If not then open it. 
// The handle hid_t is an integer (ref: H5Ipublic.h) - use the Integer specific API 

    int rc, idx;
    char * file = getHdf5FileName(name, shot, run);

    if ((idx = getOpenIdamPluginFileInt(pluginFileList, file)) >= 0) {
        *retIdx = findIdamPluginFileByInt(pluginFileList, idx);
        return 0;
    }

    idx = findFirstHdf5Idx();

// dgm TODO check name is standards compliant containing no hidden system commands (not a comprehensive test!)

    if (!IsLegalFilePath(name)) {
        sprintf(errmsg, "Error creating HDF5 file: illegal filename");
        return -1;
    }

    static char cpCommand[2048];
    char * model = getHdf5ModelName(name);

    //sprintf(cpCommand, "ls -l %s > /scratch/filecopy 2>&1; cp %s %s -v >> /scratch/filecopy 2>&1",  model, model, file);

    sprintf(cpCommand, "cp %s %s > /dev/null 2>&1", model, file);
    //system(cpCommand);

    errno = 0;
    FILE * ph = popen(cpCommand, "r");
    if (ph != NULL) fclose(ph);

// Check the file exists with the correct permissions

    rc = (access(file, F_OK) == 0) && (access(file, R_OK) == 0) && (access(file, W_OK) == 0);

    if (!rc) {
        sprintf(errmsg, "HDF5 file %s has Incorrect Permissions", file);
        return -1;
    }

    hdf5Files[idx] = H5Fopen(file, H5F_ACC_RDWR, H5P_DEFAULT);
    if (hdf5Files[idx] < 0) {
        sprintf(errmsg, "Error creating HDF5 file %s", file);
        return -1;
    }
    *retIdx = idx;
    addIdamPluginFileInt(pluginFileList, file, hdf5Files[idx]);        // Register the File Handle

// Add meta data
    int j, lstr = 0;
    char * p = NULL;
// 1> Create a header group for the information:	/metadata
// 2> Date
    time_t current_time = time(NULL);        // seconds elapsed since the Epoch
    p = ctime(&current_time);    // Convert to local time format
    lstr = strlen(p);
    imas_putData(idx, "metadata", "created", findIMASType("string"), 1, &lstr, 0, (void *) p);
// 3> Device name	
    lstr = 0;
    p = getImasIdsDevice();
    if (p && p[0] != '\0') {
        lstr = strlen(p);
        imas_putData(idx, "metadata", "device", findIMASType("string"), 1, &lstr, 0, (void *) p);
    }
// 4> Shot number	
    int nDims = 1;
    int dims[1] = { 1 };
    imas_putData(idx, "metadata", "shot", findIMASType("int"), nDims, dims, 0, (void *) &shot);
// 5> Reference number	
    imas_putData(idx, "metadata", "run", findIMASType("int"), nDims, dims, 0, (void *) &run);
// 6> IDS version
    p = getImasIdsVersion();
    if (p && p[0] != '\0') {
        lstr = strlen(p);
        imas_putData(idx, "metadata", "idsVersion", findIMASType("string"), 1, &lstr, 0, (void *) p);
    }
// 7> Model file name
    lstr = strlen(model);
    imas_putData(idx, "metadata", "modelFileName", findIMASType("string"), 1, &lstr, 0, (void *) model
    );
// 8> Model file SHA1 hash
    unsigned char md[20];
    char hash[41];
    sha1File(model, NULL, md);
    for (j = 0; j < 20; j++) sprintf(&hash[2 * j], "%2.2x", md[j]);
    hash[40] = '\0';
    lstr = 40;
    imas_putData(idx, "metadata", "modelSHA1", findIMASType("string"), 1, &lstr, 0, (void *) hash);

    return 0;
}

// dgm Original HDF5 hdf5IMASCreate renamed imas_hdf5IMASCreate - called by the IDAM IMAS plugin

// * Create a New HDF5 File and Fail if it already exists

int imas_hdf5IMASCreate(char * name, int shot, int run, int refShot, int refRun, int * retIdx)
{
    if (hdf5Startup) {
        initHdf5File();
        hdf5Startup = 0;
    }

// Register the API function needed to close HDF5 files with the IDAM file manager
// herr_t H5Fclose(hid_t files_id);
// herr_t and hid_t are both integers
// The return code herr_t in an integer (ref: H5public.h) - use the Integer specific API

    if (!isCloseRegistered) {
        int (* close)(int);    // Function pointer
        close = &H5Fclose;
        registerIdamPluginFileClose(pluginFileList, (void *) close);
        isCloseRegistered = 1;
    }

// Is the HDF5 file already open for reading? If not then open it. 
// The handle hid_t is an integer (ref: H5Ipublic.h) - use the Integer specific API 

    int idx;
    if ((idx = getOpenIdamPluginFileInt(pluginFileList, getHdf5FileName(name, shot, run))) >= 0) {
        *retIdx = findIdamPluginFileByInt(pluginFileList, idx);
        return 0;
    }

    idx = findFirstHdf5Idx();

// H5F_ACC_TRUNC to overwrite
// H5F_ACC_DEBUG to request debug output (use OR bit combination)

    hdf5Files[idx] = H5Fcreate(getHdf5FileName(name, shot, run), H5F_ACC_EXCL, H5P_DEFAULT, H5P_DEFAULT);
    if (hdf5Files[idx] < 0) {
        sprintf(errmsg, "Error creating HDF5 file %s", getHdf5FileName(name, shot, run));
        return -1;
    }
    *retIdx = idx;
    addIdamPluginFileInt(pluginFileList, getHdf5FileName(name, shot, run),
                         hdf5Files[idx]);        // Register the File Handle

    return 0;
}


// dgm Original HDF5 hdf5EuitmOpen renamed imas_hdf5EuitmOpen - called by the IDAM IMAS plugin

int imas_hdf5EuitmOpen(char * name, int shot, int run, int * retIdx)
{
    if (hdf5Startup) {
        initHdf5File();
        hdf5Startup = 0;
    }

    if (!isCloseRegistered) {
        int (* close)(int);    // Function pointer
        close = &H5Fclose;
        registerIdamPluginFileClose(pluginFileList, (void *) close);
        isCloseRegistered = 1;
    }

// Is the HDF5 file already open for reading? If not then open it. 
// The handle hid_t is an integer (ref: H5Ipublic.h) - use the Integer specific API 

    int idx;
    if ((idx = getOpenIdamPluginFileInt(pluginFileList, getHdf5FileName(name, shot, run))) >= 0) {
        *retIdx = findIdamPluginFileByInt(pluginFileList, idx);
        return 0;
    }

    idx = findFirstHdf5Idx();

    errno = 0;

    hdf5Files[idx] = H5Fopen(getHdf5FileName(name, shot, run), H5F_ACC_RDWR, H5P_DEFAULT);

    if (errno != 0 || hdf5Files[idx] < 0) {
        //if(errno != 0) addIdamError(&idamerrorstack, SYSTEMERRORTYPE, "imas", errno, "Unable to Open the file! Does the file exist?");
        sprintf(errmsg, "Error opening HDF5 file %s", getHdf5FileName(name, shot, run));
        //addIdamError(&idamerrorstack, CODEERRORTYPE, "imas", err, errmsg);
        return -1;
    }
    *retIdx = idx;
    addIdamPluginFileInt(pluginFileList, getHdf5FileName(name, shot, run),
                         hdf5Files[idx]);        // Register the File Handle
    return 0;
}


// dgm Original HDF5 hdf5EuitmClose renamed imas_hdf5EuitmClose - called by the IDAM IMAS plugin

int imas_hdf5EuitmClose(int idx, char * name, int shot, int run)
{
    char * filename = NULL;
    if (idx < 0 && name && name[0] != '\0' && shot > 0 &&
        run > 0) {                // dgm: Assume file idx is not passed
        filename = getHdf5FileName(name, shot, run);
        closeIdamPluginFile(pluginFileList, filename);
        return 0;
    }

    if (H5Fclose(hdf5Files[idx]) < 0) {
        sprintf(errmsg, "Error closing HDF5 file %s", name);        // dgm: This should be an ignorable error condition
        return -1;
    }

    hdf5Files[idx] = 0;

// dgm: Change state in the IDAM file log

    filename = getHdf5FileName(name, shot, run);
    int record = findIdamPluginFileByName(pluginFileList, filename);
    if (record < 0) record = findIdamPluginFileByInt(pluginFileList, hdf5Files[idx]);
    if (record >= 0) setIdamPluginFileClosed(pluginFileList, record);
// dgm    

    return 0;

}

void splitVarGrp(char * cpoPath, char * path, char ** groupName, char ** dataName)
{
    int i;
    char currPath[1024];
    *groupName = malloc(strlen(cpoPath) + strlen(path) + 2);
    *dataName = malloc(strlen(path) + 1);

    for (i = strlen(path) - 1; i >= 0 && path[i] != '/'; i--);
    if (i < 0) //no slashes in path
    {
        strcpy(*groupName, cpoPath);                // IDS' name
        strcpy(*dataName, path);                // Variable's name
    }
    else {
        strcpy(currPath, path);                    // Split path at last /
        currPath[i] = 0;                    // Variable's group relative to cpoPath
        sprintf(*groupName, "%s/%s", cpoPath, currPath);        // Full group path
        strcpy(*dataName, &path[i + 1]);                // Variable's name
    }

    return;
}

// dgm Original HDF5 putData renamed imas_putData - called by the IDAM IMAS plugin
// Modify the passed bool isTimed to an enumerated type - the data operation
// Redirect to appropriate function depending on enumerted value

int imas_putData(int idx, char * cpoPath, char * path, int type, int nDims, int * dims, int isTimed, void * data)
{

// dgm: IMAS strings are an exception - they are always scalar!
// dgm: split strings into blocks of 132 characters 

    int rc = 0;
    static int recursion = 1;
    if (type == STRING && recursion) {
        int strDims = ((dims[0] - 1) / 132) + 1;
        recursion = 0;

// Pack the remainder of the string with nulls
        int i, lstr = 132 * strDims * sizeof(char);
        char * cleaned = (char *) malloc(lstr);
        strncpy(cleaned, (char *) data, dims[0]);
        for (i = dims[0]; i < lstr; i++)cleaned[i] = '\0';
        rc = imas_putData(idx, cpoPath, path, type, nDims, &strDims, isTimed, cleaned);
        free((void *) cleaned);
        recursion = 1;
        return rc;
    }
/*
    if(isTimed == PUTSLICE_OPERATION)
       return imas_putDataSlice(idx, cpoPath, path, type, nDims, dims, data);
    else   
    if(isTimed == REPLACELASTSLICE_OPERATION)
       return imas_replaceLastDataSlice(idx, cpoPath, path, type, nDims, dims, data);
*/
    int i;
    char * groupName = NULL;
    char * dataName = NULL;

    hid_t group, dataset, datatype, inDatatype, dataspace;
    int exists;
    hsize_t hdims[32], maxDims[32];
    hid_t cParms;
    hsize_t chunkDims[32];
    int totSize = 0;

    splitVarGrp(cpoPath, path, &groupName, &dataName);

/*
    for(i = strlen(path) - 1; i >= 0 && path[i] != '/'; i--);
    if(i < 0) //no slashes in path
    {
        strcpy(groupName, cpoPath);		// IDS name
        strcpy(dataName, path);			// Variable name
    }
    else
    {
        strcpy(currPath, path);					// Split path at last /
        currPath[i] = 0;					// Variable's group relative to cpoPath
        sprintf(groupName, "%s/%s", cpoPath, currPath);		// Full group path
        strcpy(dataName, &path[i+1]);				// Variable's name
    }
*/

// dgm Create the group and any intermediates: returns invalid code if the group already exists

    if ((group = createGroup(hdf5Files[idx], groupName)) < 0) {
        group = H5Gopen(hdf5Files[idx], (const char *) groupName, H5P_DEFAULT);    // Open if already existing
    }

    if (group < 0) {
        sprintf(errmsg, "Error opening HDF5 Group %s", groupName);
        free(groupName);
        free(dataName);
        return -1;
    }
    exists = H5Lexists(group, dataName, H5P_DEFAULT);
    if (exists < 0) {
        sprintf(errmsg, "Error checking dataset existence for %s", dataName);
        free(groupName);
        free(dataName);
        return -1;
    }
    if (exists) {
        if (H5Ldelete(group, dataName, H5P_DEFAULT) < 0) {
            sprintf(errmsg, "Error deleting dataset  %s", dataName);
            free(groupName);
            free(dataName);
            return -1;
        }
    }

    for (i = 0, totSize = 1; i < nDims; i++)
        totSize *= dims[i];
    if (totSize == 0) {
        free(groupName);
        free(dataName);
        return 0;
    }


    //Ready to create a new dataset: first define the datatype
    switch (type) {
        case INT:
            datatype = H5Tcopy(H5T_STD_I32BE);
            inDatatype = H5Tcopy(H5T_NATIVE_INT);
            break;
        case FLOAT:
            datatype = H5Tcopy(H5T_IEEE_F32BE);
            inDatatype = H5Tcopy(H5T_NATIVE_FLOAT);
            break;
        case DOUBLE:
            datatype = H5Tcopy(H5T_IEEE_F64BE);
            inDatatype = H5Tcopy(H5T_NATIVE_DOUBLE);
            break;
        case STRING:
            datatype = H5Tcopy(H5T_C_S1);
            H5Tset_size(datatype, 132);
            inDatatype = H5Tcopy(H5T_C_S1);
            H5Tset_size(inDatatype, 132);

            //Ricordarsi alla fine H5Dvlen_reclaim!!!!
            break;
    }

    //then the dataspace
    for (i = 0; i < nDims; i++)
        hdims[i] = dims[i];
    if (!isTimed) {
        dataspace = H5Screate_simple(nDims, hdims, NULL);
    }
    else {
        for (i = 0; i < nDims - 1; i++) {
            maxDims[i] = dims[i];
        }
        //if(type == STRING)
        //    maxDims[nDims - 1] = dims[nDims - 1];
        //else
        maxDims[nDims - 1] = H5S_UNLIMITED;
        dataspace = H5Screate_simple(nDims, hdims, maxDims);
    }

    if (isTimed) {
        cParms = H5Pcreate(H5P_DATASET_CREATE);
        for (i = 0; i < nDims; i++)
            chunkDims[i] = dims[i];
        H5Pset_chunk(cParms, nDims, chunkDims);
        dataset = H5Dcreate(group, dataName, datatype, dataspace, H5P_DEFAULT, cParms, H5P_DEFAULT);
    }
    else
        dataset = H5Dcreate(group, dataName, datatype, dataspace, H5P_DEFAULT, H5P_DEFAULT, H5P_DEFAULT);
    if (dataset < 0) {
        sprintf(errmsg, "Error creating dataset %s in group %s", dataName, groupName);
        free(groupName);
        free(dataName);
        return -1;
    }

    //Write in the dataset
    if (H5Dwrite(dataset, inDatatype, H5S_ALL, H5S_ALL, H5P_DEFAULT, data) < 0) {
        sprintf(errmsg, "Error writing dataset %s in group %s", dataName, groupName);
        free(groupName);
        free(dataName);
        return -1;
    }
    free(groupName);
    free(dataName);
    H5Gclose(group);
    H5Tclose(datatype);
    H5Tclose(inDatatype);
    H5Dclose(dataset);
    H5Sclose(dataspace);
    return 0;
}

// dgm Original HDF5 putDataSlice renamed imas_putDataSlice - called by the IDAM IMAS plugin

int imas_putDataX(int idx, char * cpoPath, char * path, int type, int nDims, int * dims, int dataOperation, void * data,
                  double time)
{

// dgm: IMAS strings are an exception - they are always scalar!
// dgm: split strings into blocks of 132 characters 

    int rc = 0;
    static int recursion = 1;
    if (type == STRING && recursion) {
        int strDims = ((dims[0] - 1) / 132) + 1;
        recursion = 0;

// Pack the remainder of the string with nulls
        int i, lstr = 132 * strDims * sizeof(char);
        char * cleaned = (char *) malloc(lstr);
        strncpy(cleaned, (char *) data, dims[0]);
        for (i = dims[0]; i < lstr; i++)cleaned[i] = '\0';
        rc = imas_putDataX(idx, cpoPath, path, type, nDims, &strDims, dataOperation, cleaned, time);
        free((void *) cleaned);
        recursion = 1;
        return rc;
    }

    if (dataOperation == PUTSLICE_OPERATION)
        return imas_putDataSlice(idx, cpoPath, path, type, nDims, dims, data, time);
    else if (dataOperation == REPLACELASTSLICE_OPERATION)
        return imas_replaceLastDataSlice(idx, cpoPath, path, type, nDims, dims, data);

    return -1;
}

int imas_putDataSlice(int idx, char * cpoPath, char * path, int type, int nDims, int * dims, void * data, double time)
{
    int i, currNDims, totSize;
    char * groupName = malloc(strlen(cpoPath) + strlen(path) + 2), currPath[1024];
    char * dataName = malloc(strlen(path) + 1);
    hid_t group, dataset, datatype, inDatatype, dataspace, memDataspace;
    int exists;
    hsize_t hdims[32], maxDims[32], currHDims[32], startOut[32],
            strideOut[32], blockOut[32], countOut[32];
    hid_t cParms;
    hsize_t chunkDims[32], extSize[32];

    for (i = 0, totSize = 1; i < nDims; i++)
        totSize *= dims[i];
    if (totSize == 0) {
        free(groupName);
        free(dataName);
        return 0;
    }

    for (i = strlen(path) - 1; i >= 0 && path[i] != '/'; i--);
    if (i < 0) //no slashes in path
    {
        strcpy(groupName, cpoPath);
        strcpy(dataName, path);
    }
    else {
        strcpy(currPath, path);
        currPath[i] = 0;
        sprintf(groupName, "%s/%s", cpoPath, currPath);
        strcpy(dataName, &path[i + 1]);
    }

// dgm Create the group and any intermediates: returns invalid code if the group already exists

    if ((group = createGroup(hdf5Files[idx], groupName)) < 0) {
        group = H5Gopen(hdf5Files[idx], (const char *) groupName, H5P_DEFAULT);    // Open if already existing
    }

// dgm group = H5Gopen( hdf5Files[idx], (const char *)groupName, H5P_DEFAULT);

    if (group < 0) {
        sprintf(errmsg, "Error opening HDF5 Group %s", groupName);
        free(groupName);
        free(dataName);
        return -1;
    }
    exists = H5Lexists(group, dataName, H5P_DEFAULT);
    if (exists < 0) {
        sprintf(errmsg, "Error checking dataset existence for %s", dataName);
        free(groupName);
        free(dataName);
        return -1;
    }
    //Ready to create a new dataset: first define the datatype
    switch (type) {
        case INT:
            datatype = H5Tcopy(H5T_STD_I32BE);
            inDatatype = H5Tcopy(H5T_NATIVE_INT);
            break;
        case FLOAT:
            datatype = H5Tcopy(H5T_IEEE_F32BE);
            inDatatype = H5Tcopy(H5T_NATIVE_FLOAT);
            break;
        case DOUBLE:
            datatype = H5Tcopy(H5T_IEEE_F64BE);
            inDatatype = H5Tcopy(H5T_NATIVE_DOUBLE);
            break;
        case STRING:
            datatype = H5Tcopy(H5T_C_S1);
            H5Tset_size(datatype, H5T_VARIABLE);
            inDatatype = H5Tcopy(H5T_C_S1);
            H5Tset_size(inDatatype, H5T_VARIABLE);

            //Ricordarsi alla fine H5Dvlen_reclaim!!!!
            break;
    }

    if (!exists) //The first time we need to create the dataset
    {
        for (i = 0; i < nDims; i++)
            hdims[i] = dims[i];
        for (i = 0; i < nDims; i++) {
            maxDims[i] = dims[i];
        }
        hdims[nDims] = 1;
        maxDims[nDims] = H5S_UNLIMITED;
        dataspace = H5Screate_simple(nDims + 1, hdims, maxDims);

        cParms = H5Pcreate(H5P_DATASET_CREATE);
        for (i = 0; i < nDims; i++)
            chunkDims[i] = dims[i];
        chunkDims[nDims] = SLICE_BLOCK;
        H5Pset_chunk(cParms, nDims + 1, chunkDims);
        dataset = H5Dcreate(group, dataName, datatype, dataspace, H5P_DEFAULT, cParms, H5P_DEFAULT);
        if (dataset < 0) {
            sprintf(errmsg, "Error creating dataset %s in group %s", dataName, groupName);
            free(groupName);
            free(dataName);
            return -1;
        }
        H5Pclose(cParms);
    }
    else //get the dataset
    {
        dataset = H5Dopen(group, dataName, H5P_DEFAULT);
        if (dataset < 0) {
            sprintf(errmsg, "Error opening dataset %s in group %s", dataName, groupName);
            free(groupName);
            free(dataName);
            return -1;
        }
        dataspace = H5Dget_space(dataset);
        if (dataspace < 0) {
            sprintf(errmsg, "Error getting dataset %s in group %s", dataName, groupName);
            free(groupName);
            free(dataName);
            return -1;
        }
        //Get last dimension
    }
    currNDims = H5Sget_simple_extent_ndims(dataspace);
    if (currNDims != nDims + 1) {
        sprintf(errmsg, "Internal error: wrong number of dimensions in %s, group %s", dataName, groupName);
        free(groupName);
        free(dataName);
        return -1;
    }
    if (H5Sget_simple_extent_dims(dataspace, currHDims, maxDims) < 0) {
        sprintf(errmsg, "Error in H5Sget_simple_extent_dims in %s, group %s", dataName, groupName);
        free(groupName);
        free(dataName);
        return -1;
    }
    for (i = 0; i < nDims; i++) {
        if (dims[i] != currHDims[i]) {
            sprintf(errmsg, "Internal error: wrong dimension in %s, group %s", dataName, groupName);
            free(groupName);
            free(dataName);
            return -1;
        }
    }
    memDataspace = H5Screate(H5S_SIMPLE);
    if (nDims == 0) {
        hdims[0] = 1;
        H5Sset_extent_simple(memDataspace, 1, hdims, hdims);
    }
    else {
        for (i = 0; i < nDims; i++)
            hdims[i] = dims[i];
        H5Sset_extent_simple(memDataspace, nDims, hdims, hdims);
    }
    //Select out hyperslab

    for (i = 0; i < nDims; i++) {
        startOut[i] = 0;
        blockOut[i] = dims[i];
        strideOut[i] = 1;
        countOut[i] = 1;
    }
    if (exists) {
        startOut[nDims] = currHDims[nDims];
        blockOut[nDims] = 1;
        strideOut[nDims] = 1;
        countOut[nDims] = 1;
        for (i = 0; i < nDims; i++) {
            extSize[i] = dims[i];
        }
        extSize[nDims] = currHDims[nDims] + 1;
        if (H5Dset_extent(dataset, extSize) < 0) {
            sprintf(errmsg, "Internal error: extend failed in %s, group %s", dataName, groupName);
            free(groupName);
            free(dataName);
            return -1;
        }
        H5Sclose(dataspace);
        dataspace = H5Dget_space(dataset);
    }
    else //The first time the dataset is created
    {
        startOut[nDims] = 0;
        blockOut[nDims] = 1;
        strideOut[nDims] = 1;
        countOut[nDims] = 1;
    }


    if (H5Sselect_hyperslab(dataspace, H5S_SELECT_SET, startOut, strideOut, countOut, blockOut) < 0) {
        sprintf(errmsg, "Error in H5Sselect_hyperslab in %s, group %s", dataName, groupName);
        free(groupName);
        free(dataName);
        return -1;
    }

    //Write in the dataset
    if (H5Dwrite(dataset, inDatatype, memDataspace, dataspace, H5P_DEFAULT, data) < 0) {
        sprintf(errmsg, "Error writing dataset  %s in group %s", dataName, groupName);
        free(groupName);
        free(dataName);
        return -1;
    }
    free(groupName);
    free(dataName);
    H5Gclose(group);
    H5Tclose(datatype);
    H5Tclose(inDatatype);
    H5Dclose(dataset);
    H5Sclose(dataspace);
    H5Sclose(memDataspace);
    return 0;
}

// dgm Original HDF5 replaceLastDataSlice renamed imas_replaceLastDataSlice - called by the IDAM IMAS plugin

int imas_replaceLastDataSlice(int idx, char * cpoPath, char * path, int type, int nDims, int * dims, void * data)
{
    int i, currNDims;
    char * groupName = malloc(strlen(cpoPath) + strlen(path) + 2), currPath[1024];
    char * dataName = malloc(strlen(path) + 1);
    hid_t group, dataset, datatype, inDatatype, dataspace, memDataspace;
    int exists;
    hsize_t hdims[32], maxDims[32], currHDims[32], startOut[32],
            strideOut[32], blockOut[32], countOut[32];

    for (i = strlen(path) - 1; i >= 0 && path[i] != '/'; i--);
    if (i < 0) //no slashes in path
    {
        strcpy(groupName, cpoPath);
        strcpy(dataName, path);
    }
    else {
        strcpy(currPath, path);
        currPath[i] = 0;
        sprintf(groupName, "%s/%s", cpoPath, currPath);
        strcpy(dataName, &path[i + 1]);
    }

// dgm Create the group and any intermediates: returns invalid code if the group already exists

    if ((group = createGroup(hdf5Files[idx], groupName)) < 0) {
        group = H5Gopen(hdf5Files[idx], (const char *) groupName, H5P_DEFAULT);    // Open if already existing
    }

// dgm    group = H5Gopen( hdf5Files[idx], (const char *)groupName, H5P_DEFAULT);
    if (group < 0) {
        sprintf(errmsg, "Error opening HDF5 Group %s", groupName);
        free(groupName);
        free(dataName);
        return -1;
    }
    exists = H5Lexists(group, dataName, H5P_DEFAULT);
    if (exists <= 0) {
        sprintf(errmsg, "Error checking dataset existence for %s", dataName);
        free(groupName);
        free(dataName);
        return -1;
    }
    //Ready to create a new dataset: first define the datatype
    switch (type) {
        case INT:
            datatype = H5Tcopy(H5T_STD_I32BE);
            inDatatype = H5Tcopy(H5T_NATIVE_INT);
            break;
        case FLOAT:
            datatype = H5Tcopy(H5T_IEEE_F32BE);
            inDatatype = H5Tcopy(H5T_NATIVE_FLOAT);
            break;
        case DOUBLE:
            datatype = H5Tcopy(H5T_IEEE_F64BE);
            inDatatype = H5Tcopy(H5T_NATIVE_DOUBLE);
            break;
        case STRING:
            datatype = H5Tcopy(H5T_C_S1);
            H5Tset_size(datatype, H5T_VARIABLE);
            inDatatype = H5Tcopy(H5T_C_S1);
            H5Tset_size(inDatatype, H5T_VARIABLE);

            //Ricordarsi alla fine H5Dvlen_reclaim!!!!
            break;
    }

//get the dataset
    dataset = H5Dopen(group, dataName, H5P_DEFAULT);
    if (dataset < 0) {
        sprintf(errmsg, "Error opening dataset %s in group %s", dataName, groupName);
        free(groupName);
        free(dataName);
        return -1;
    }
    dataspace = H5Dget_space(dataset);
    if (dataspace < 0) {
        sprintf(errmsg, "Error getting dataset %s in group %s", dataName, groupName);
        free(groupName);
        free(dataName);
        return -1;
    }
    //Get last dimension
    currNDims = H5Sget_simple_extent_ndims(dataspace);
    if (currNDims != nDims + 1) {
        sprintf(errmsg, "Internal error: wrong number of dimensions in %s, group %s", dataName, groupName);
        free(groupName);
        free(dataName);
        return -1;
    }
    if (H5Sget_simple_extent_dims(dataspace, currHDims, maxDims) < 0) {
        sprintf(errmsg, "Error in H5Sget_simple_extent_dims in %s, group %s", dataName, groupName);
        free(groupName);
        free(dataName);
        return -1;
    }
    for (i = 0; i < nDims; i++) {
        if (dims[i] != currHDims[i]) {
            sprintf(errmsg, "Internal error: wrong dimension in %s, group %s", dataName, groupName);
            free(groupName);
            free(dataName);
            return -1;
        }
    }
    memDataspace = H5Screate(H5S_SIMPLE);
    if (nDims == 0) {
        hdims[0] = 1;
        H5Sset_extent_simple(memDataspace, 1, hdims, hdims);
    }
    else {
        for (i = 0; i < nDims; i++)
            hdims[i] = dims[i];
        H5Sset_extent_simple(memDataspace, nDims, hdims, hdims);
    }
    //Select out hyperslab

    for (i = 0; i < nDims; i++) {
        startOut[i] = 0;
        blockOut[i] = dims[i];
        strideOut[i] = 1;
        countOut[i] = 1;
    }
    startOut[nDims] = currHDims[nDims] - 1;
    blockOut[nDims] = 1;
    strideOut[nDims] = 1;
    countOut[nDims] = 1;

    if (H5Sselect_hyperslab(dataspace, H5S_SELECT_SET, startOut, strideOut, countOut, blockOut) < 0) {
        sprintf(errmsg, "Error in H5Sselect_hyperslab in %s, group %s", dataName, groupName);
        free(groupName);
        free(dataName);
        return -1;
    }

    //Write in the dataset
    if (H5Dwrite(dataset, inDatatype, memDataspace, dataspace, H5P_DEFAULT, data) < 0) {
        sprintf(errmsg, "Error writing dataset  %s in group %s", dataName, groupName);
        free(groupName);
        free(dataName);
        return -1;
    }
    free(groupName);
    free(dataName);
    H5Gclose(group);
    H5Tclose(datatype);
    H5Tclose(inDatatype);
    H5Dclose(dataset);
    H5Sclose(dataspace);
    H5Sclose(memDataspace);
    return 0;
}

// dgm Original HDF5 getData renamed imas_getData - called by the IDAM IMAS plugin

int imas_getData(int idx, char * cpoPath, char * path, int type, int nDims, int * dims, char ** data)
{
    int i, savedNDims, dataSize, strLen;
    char * groupName = malloc(strlen(cpoPath) + strlen(path) + 2), currPath[1024];
    char * outData;
    char * dataName = malloc(strlen(path) + 1);
    hid_t group, dataset, datatype, inDatatype, savedDataspace;
    int exists;
    hsize_t savedDims[32], savedMaxdims[32];

    for (i = strlen(path) - 1; i >= 0 && path[i] != '/'; i--);
    if (i < 0) //no slashes in path
    {
        strcpy(groupName, cpoPath);
        strcpy(dataName, path);
    }
    else {
        strcpy(currPath, path);
        currPath[i] = 0;
        sprintf(groupName, "%s/%s", cpoPath, currPath);
        strcpy(dataName, &path[i + 1]);
    }

    group = H5Gopen(hdf5Files[idx], (const char *) groupName, H5P_DEFAULT);
    if (group < 0) {
        sprintf(errmsg, "Error opening HDF5 Group %s", groupName);
        free(groupName);
        free(dataName);
        return -1;
    }
    exists = H5Lexists(group, dataName, H5P_DEFAULT);
    if (exists <= 0) {
        sprintf(errmsg, "Error: dataset %s not found", dataName);
        free(groupName);
        free(dataName);
        return -1;
    }

    //Ready to create a new dataset: first define the datatype
    switch (type) {
        case INT:
            datatype = H5Tcopy(H5T_STD_I32BE);
            inDatatype = H5Tcopy(H5T_NATIVE_INT);
            break;
        case FLOAT:
            datatype = H5Tcopy(H5T_IEEE_F32BE);
            inDatatype = H5Tcopy(H5T_NATIVE_FLOAT);
            break;
        case DOUBLE:
            datatype = H5Tcopy(H5T_IEEE_F64BE);
            inDatatype = H5Tcopy(H5T_NATIVE_DOUBLE);
            break;
        case STRING:
            inDatatype = H5Tcopy(H5T_C_S1);
            H5Tset_size(inDatatype, 132);
            datatype = H5Tcopy(H5T_C_S1);
            H5Tset_size(inDatatype, 132);
            break;
        case STRING_VECTOR:
            datatype = H5Tcopy(H5T_C_S1);
            H5Tset_size(datatype, H5T_VARIABLE);
            inDatatype = H5Tcopy(H5T_C_S1);
            H5Tset_size(inDatatype, H5T_VARIABLE);

            //Ricordarsi alla fine H5Dvlen_reclaim!!!!
            break;
    }
    dataset = H5Dopen(group, dataName, H5P_DEFAULT);
    if (dataset < 0) {
        sprintf(errmsg, "Error opening dataset %s in group %s", dataName, groupName);
        free(groupName);
        free(dataName);
        return -1;
    }

    /*    savedDatatype = H5Dget_type(dataset);
    if(! H5Tequal(savedDatatype, datatype))   
    {
        sprintf(errmsg, "Error: invalid datatype for %s, using saved datatype ", dataName);
	//free(groupName);
        //free(dataName);
	return -1;
    }
    */
    //Check dataspace
    savedDataspace = H5Dget_space(dataset);
    savedNDims = H5Sget_simple_extent_ndims(savedDataspace);
    if (nDims != savedNDims) {
        sprintf(errmsg, "Error: wrong number of dimensions for %s ", dataName);
        free(groupName);
        free(dataName);
        return -1;
    }

    if (H5Sget_simple_extent_dims(savedDataspace, savedDims, savedMaxdims) < 0) {
        sprintf(errmsg, "Error in H5Sget_simple_extent_dims for %s ", dataName);
        free(groupName);
        free(dataName);
        return -1;
    }
    for (i = 0; i < nDims; i++) {
        dims[i] = savedDims[i];
    }

    //Allocate space for data
    dataSize = H5Dget_storage_size(dataset);
    outData = malloc(dataSize);


    //Check passed, read data
    if (H5Dread(dataset, inDatatype, H5S_ALL, H5S_ALL, H5P_DEFAULT, outData) < 0) {
        sprintf(errmsg, "Error reading dataset %s ", dataName);
        free(groupName);
        free(dataName);
        free(outData);
        return -1;
    }

    if (type == STRING) {

        strLen = strlen(outData);
        *data = malloc(strLen + 1);
        strcpy(*data, outData);

        free(outData);

    }
    else if (type == STRING_VECTOR) {
        *data = malloc(dims[0] * sizeof(char *));
        for (i = 0; i < dims[0]; i++) {
            strLen = strlen(((char **) outData)[i]);
            ((char **) *data)[i] = malloc(strLen + 1);
            strcpy(((char **) *data)[i], ((char **) outData)[i]);
        }
        H5Dvlen_reclaim(inDatatype, savedDataspace, H5P_DEFAULT, outData);
        free(outData);
    }
    else {
        *data = outData;
    }


    free(groupName);
    free(dataName);
    H5Gclose(group);
    H5Tclose(datatype);
    H5Tclose(inDatatype);
    H5Dclose(dataset);
    H5Sclose(savedDataspace);
    return 0;
}

// dgm Original HDF5 getDataSlices renamed imas_getDataSlices - called by the IDAM IMAS plugin

//Get the two consecutive data samples starting from dataIdx
int imas_getDataSlices(int idx, char * cpoPath, char * path, int type, int nDims, int * dims, int dataIdx,
                       int numSlices, char ** data)
{
    int i, currNDims, dataSize;
    char * groupName = malloc(strlen(cpoPath) + strlen(path) + 2), currPath[1024];
    char * dataName = malloc(strlen(path) + 1);
    hid_t group, dataset, datatype, inDatatype, dataspace, memDataspace;

    hsize_t hdims[32], maxDims[32], currHDims[32], startOut[32],
            strideOut[32], blockOut[32], countOut[32];

    for (i = strlen(path) - 1; i >= 0 && path[i] != '/'; i--);
    if (i < 0) //no slashes in path
    {
        strcpy(groupName, cpoPath);
        strcpy(dataName, path);
    }
    else {
        strcpy(currPath, path);
        currPath[i] = 0;
        sprintf(groupName, "%s/%s", cpoPath, currPath);
        strcpy(dataName, &path[i + 1]);
    }

    group = H5Gopen(hdf5Files[idx], (const char *) groupName, H5P_DEFAULT);
    if (group < 0) {
        sprintf(errmsg, "Error opening HDF5 Group %s", groupName);
        free(groupName);
        free(dataName);
        return -1;
    }
    if (H5Lexists(group, dataName, H5P_DEFAULT) <= 0) {
        sprintf(errmsg, "Error checking dataset existence for %s", dataName);
        free(groupName);
        free(dataName);
        return -1;
    }
    //Ready to create a new dataset: first define the datatype
    switch (type) {
        case INT:
            datatype = H5Tcopy(H5T_STD_I32BE);
            inDatatype = H5Tcopy(H5T_NATIVE_INT);
            dataSize = sizeof(int);
            break;
        case FLOAT:
            datatype = H5Tcopy(H5T_IEEE_F32BE);
            inDatatype = H5Tcopy(H5T_NATIVE_FLOAT);
            dataSize = sizeof(float);
            break;
        case DOUBLE:
            datatype = H5Tcopy(H5T_IEEE_F64BE);
            inDatatype = H5Tcopy(H5T_NATIVE_DOUBLE);
            dataSize = sizeof(double);
            break;
        case STRING:
            sprintf(errmsg, "String slices not supported");
            return -1;
            break;
    }

    dataset = H5Dopen(group, dataName, H5P_DEFAULT);
    if (dataset < 0) {
        sprintf(errmsg, "Error opening dataset %s in group %s", dataName, groupName);
        free(groupName);
        free(dataName);
        return -1;
    }
    /*    savedDatatype = H5Dget_type(dataset);
    if(! H5Tequal(savedDatatype, datatype))   
    {
        sprintf(errmsg, "Error: invalid datatype for %s, using saved datatype ", dataName);
	printf("Datatype does not match...\n");
        datatype = savedDatatype;
	free(groupName);
        free(dataName);
	return -1;
    }
    */
    dataspace = H5Dget_space(dataset);
    if (dataspace < 0) {
        sprintf(errmsg, "Error getting dataset  %s in group %s", dataName, groupName);
        free(groupName);
        free(dataName);
        return -1;
    }

    //Get last dimension
    currNDims = H5Sget_simple_extent_ndims(dataspace);
    if (currNDims != nDims + 1) {
        sprintf(errmsg, "Internal error: wrong number of dimensions in %s, group %s", dataName, groupName);
        free(groupName);
        free(dataName);
        return -1;
    }
    if (H5Sget_simple_extent_dims(dataspace, currHDims, maxDims) < 0) {
        sprintf(errmsg, "Error in H5Sget_simple_extent_dims in %s, group %s", dataName, groupName);
        free(groupName);
        free(dataName);
        return -1;
    }

    for (i = 0; i < nDims; i++) {
        dims[i] = currHDims[i];
    }
    if (dataIdx < 0 || dataIdx + numSlices > currHDims[nDims]) {
        sprintf(errmsg, "Internal error: slice Idx outsize last dimension range in %s, group %s", dataName, groupName);
        free(groupName);
        free(dataName);
        return -1;
    }

    memDataspace = H5Scopy(dataspace);

    //Select out hyperslab

    for (i = 0; i < nDims; i++) {
        startOut[i] = 0;
        blockOut[i] = dims[i];
        strideOut[i] = 1;
        countOut[i] = 1;
        dataSize *= dims[i];
        hdims[i] = dims[i];
    }
    startOut[nDims] = dataIdx;
    blockOut[nDims] = numSlices;
    strideOut[nDims] = 1;
    countOut[nDims] = 1;
    hdims[nDims] = numSlices;
    dataSize *= numSlices;

    if (H5Sselect_hyperslab(dataspace, H5S_SELECT_SET, startOut, strideOut, countOut, blockOut) < 0) {
        sprintf(errmsg, "Error in H5Sselect_hyperslab in %s, group %s", dataName, groupName);
        free(groupName);
        free(dataName);
        return -1;
    }

    memDataspace = H5Screate_simple(nDims + 1, hdims, NULL);


    //Write in the dataset
    *data = malloc(dataSize);
    if (H5Dread(dataset, inDatatype, memDataspace, dataspace, H5P_DEFAULT, *data) < 0) {
        sprintf(errmsg, "Error reading dataset  %s in group %s", dataName, groupName);
        free(groupName);
        free(dataName);
        free(*data);
        return -1;
    }
    free(groupName);
    free(dataName);
    H5Gclose(group);
    H5Tclose(datatype);
    H5Tclose(inDatatype);
    H5Dclose(dataset);
    H5Sclose(dataspace);
    H5Sclose(memDataspace);
    return 0;
}

// dgm Replacement for Original IMAS HDF5 hdf5GetDimension (renamed imas_hdf5GetDimension) that calls IDAM 

int imas_hdf5GetDimension(int expIdx, char * cpoPath, char * path, int * numDims, int * dim1, int * dim2, int * dim3,
                          int * dim4, int * dim5, int * dim6, int * dim7)
{
    int i, savedNDims;
    char * groupName = malloc(strlen(cpoPath) + strlen(path) + 2);
    char currPath[1024];
    char * dataName = malloc(strlen(path) + 1);
    hid_t group, dataset, savedDataspace;
    int exists;
    hsize_t savedDims[32], savedMaxdims[32];

    for (i = strlen(path) - 1; i >= 0 && path[i] != '/'; i--);
    if (i < 0) //no slashes in path
    {
        strcpy(groupName, cpoPath);
        strcpy(dataName, path);
    }
    else {
        strcpy(currPath, path);
        currPath[i] = 0;
        sprintf(groupName, "%s/%s", cpoPath, currPath);
        strcpy(dataName, &path[i + 1]);
    }

    group = H5Gopen(hdf5Files[expIdx], (const char *) groupName, H5P_DEFAULT);
    if (group < 0) {
        sprintf(errmsg, "Error opening HDF5 Group %s", groupName);
        free(groupName);
        free(dataName);
        return -1;
    }
    exists = H5Lexists(group, dataName, H5P_DEFAULT);
    if (exists <= 0) {
        sprintf(errmsg, "Error: dataset %s not found", dataName);
        free(groupName);
        free(dataName);
        return -1;
    }
    dataset = H5Dopen(group, dataName, H5P_DEFAULT);
    if (dataset < 0) {
        sprintf(errmsg, "Error opening dataset %s in group %s", dataName, groupName);
        free(groupName);
        free(dataName);
        return -1;
    }

    //Check dataspace
    savedDataspace = H5Dget_space(dataset);
    savedNDims = H5Sget_simple_extent_ndims(savedDataspace);
    if (H5Sget_simple_extent_dims(savedDataspace, savedDims, savedMaxdims) < 0) {
        sprintf(errmsg, "Error in H5Sget_simple_extent_dims for %s ", dataName);
        free(groupName);
        free(dataName);
        return -1;
    }
    *numDims = savedNDims;
    if (savedNDims > 0)
        *dim1 = savedDims[0];
    if (savedNDims > 1)
        *dim2 = savedDims[1];
    if (savedNDims > 2)
        *dim3 = savedDims[2];
    if (savedNDims > 3)
        *dim4 = savedDims[3];
    if (savedNDims > 4)
        *dim5 = savedDims[4];
    if (savedNDims > 5)
        *dim6 = savedDims[5];
    if (savedNDims > 6)
        *dim7 = savedDims[6];


    free(groupName);
    free(dataName);
    H5Gclose(group);
    H5Dclose(dataset);
    H5Sclose(savedDataspace);
    return 0;
}


int imas_hdf5DeleteData(int expIdx, char * cpoPath, char * path)
{
    int i;
    char * groupName = malloc(strlen(cpoPath) + strlen(path) + 2);
    char * dataName = malloc(strlen(path) + 1);
    char currPath[1024];
    hid_t group;
    int exists;

    for (i = strlen(path) - 1; i >= 0 && path[i] != '/'; i--);
    if (i < 0) //no slashes in path
    {
        strcpy(groupName, cpoPath);
        strcpy(dataName, path);
    }
    else {
        strcpy(currPath, path);
        currPath[i] = 0;
        sprintf(groupName, "%s/%s", cpoPath, currPath);
        strcpy(dataName, &path[i + 1]);
    }

    group = H5Gopen(hdf5Files[expIdx], (const char *) groupName, H5P_DEFAULT);
    if (group < 0) {

// Missing group from MODEL? Create commands for use to repair the Model

//openDebug();
        if (debugon) {
            idamLog(LOG_DEBUG, "createGroup(rootId, \"/%s\");\n", groupName);
            fflush(dbgout);
        }
        sprintf(errmsg, "Error opening HDF5 Group %s", groupName);
        free(groupName);
        free(dataName);
        return -1;
    }
    exists = H5Lexists(group, dataName, H5P_DEFAULT);
    if (exists < 0) {
        sprintf(errmsg, "Error checking dataset existence for %s", dataName);
        free(groupName);
        free(dataName);
        return -1;
    }
    if (exists) {
        if (H5Ldelete(group, dataName, H5P_DEFAULT) < 0) {
            sprintf(errmsg, "Error deleting dataset  %s", dataName);
            free(groupName);
            free(dataName);
            return -1;
        }
    }
    free(groupName);
    free(dataName);
    H5Gclose(group);
    return 0;

}

/* ---------------------- *
 *  Arrays of structures  *
 * ---------------------- */

/*
For true data storage agnosticism, all object handling is pushed to the server
Any namespace references to hdf5 etc is unfortunate
Objects are treated as opaque entities
Each time slice is an independent object
No object processing occurs on the client - objects are not passed back from the server

There is no explicit serialisation
Modifying the contents of objects assumes the bit representaion, byte packing etc. are consistent
Object caching is best done by an application side client-server system to ensure consistent data representation 

All objects are local to the server
Replace the object passed back to the application with an scalar 64 bit integer corresponding to the local object address.
On initialisation this is set to -1 by the application
Objects may be a NULL pointer

putDataSliceInObject 
getDataSliceFromObject
imas_putDataSliceInObject
imas_getDataSliceFromObject
hdf5BeginObject
imas_hdf5BeginObject
hdf5GetObject
imas_hdf5GetObject
hdf5GetObjectSlice
imas_hdf5GetObjectSlice
hdf5GetObjectFromObject
imas_hdf5GetObjectFromObject
hdf5ReleaseObject
imas_hdf5ReleaseObject
*/

/* ---------------------- *
 *  Arrays of structures  *
 * ---------------------- */

#define CREATE 1
#define NO_CREATE 0
#define CLEAR 1
#define NO_CLEAR 0

/** Open a group.
 * If set, create_flag forces creation of the group and all necessary intermediate groups.
 * If set, clear_flag forces to erase the current content of the group */
hid_t openGroup(hid_t root, const char * relPath, int create_flag, int clear_flag)
{
    char field[1024];
    const char * src;
    char * dst;
    hid_t group = root, newGroup;

    for (src = relPath; ; src++) {

        // field next field name
        for (dst = field; *src != 0 && *src != '/'; dst++, src++)
            *dst = *src;
        *dst = 0;

        if (H5Lexists(group, field, H5P_DEFAULT)) {
            if (*src == 0 && clear_flag) {
                // if it was the last field and we are asked to clear it
                H5Ldelete(group, field, H5P_DEFAULT);
                newGroup = H5Gcreate(group, (const char *) field, H5P_DEFAULT, H5P_DEFAULT, H5P_DEFAULT);
            } else {
                // open the group
                newGroup = H5Gopen(group, (const char *) field, H5P_DEFAULT);
            }
        } else {
            if (create_flag) {
                // if it doesn't exist, then create it
                newGroup = H5Gcreate(group, (const char *) field, H5P_DEFAULT, H5P_DEFAULT, H5P_DEFAULT);
            } else {
                newGroup = -1;
            }
        }
        if (newGroup < 0) {
            if (group != root) H5Gclose(group);
            sprintf(errmsg, "Error opening HDF5 Group %s", relPath);
            return -1;
        }

        // release old group handle
        if (group != root) H5Gclose(group);
        group = newGroup;

        // if it was last field then return
        if (*src == 0)
            return group;
    }
}

/** Append data to a field
 * If we are dealing with non timed data, the field is assumed to have
 * been cleared previously
 * The size of the field can vary between slices.
 * The size of the field is therefore stored in an associated "_size" field
 * nDims should be 0 for scalars
 * If the specified slice is not the next slice to be filled
 * then we insert empty slices */

// dgm Original HDF5 putDataSliceInObject renamed imas_putDataSliceInObject - called by the IDAM IMAS plugin
// The obj is a True Object

int imas_putDataSliceInObject(void * obj, char * path, int index, int type, int nDims, int * dims, void * data)
{
    obj_t * object = (obj_t *) obj;
    hid_t hObj = object->handle;

    int i, totSize;
    int exists;
    char tmpstr[1024], dataName[1024], groupName[1024], sizeName[1024];
    hid_t group, dataset, datatype, memDatatype, dataspace, memDataspace;
    hid_t sizeset, sizetype, memSizetype, sizespace, memSizespace;
    hid_t stringType;
    hid_t cParms;
    hsize_t hdims[1], extDims[1], startOut[1];
    hsize_t countOut[1];
    hsize_t maxDims[1];
    hsize_t chunkDims[1];
    hvl_t * vl_data, * vl_size;
    int zero = 0;
    long lastIdx;

    // compute total size of data
    for (i = 0, totSize = 1; i < nDims; i++)
        totSize *= dims[i];
/*    if(totSize == 0)
        return 0;*/

    // split path into group name and field name
    for (i = strlen(path) - 1; i >= 0 && path[i] != '/'; i--);
    strcpy(groupName, path);
    groupName[i] = 0;
    strcpy(dataName, &path[i + 1]);

    // Generate correct group path by substituting leading field by index number
    for (i = 0; groupName[i] != '/' && groupName[i] != 0; i++);
    if (groupName[i] == 0)
        sprintf(tmpstr, "%d", index);
    else
        sprintf(tmpstr, "%d/%s", index, &groupName[i + 1]);
    strcpy(groupName, tmpstr);

    // open container group
    group = openGroup(hObj, groupName, CREATE, NO_CLEAR);
    if (group < 0) {
        sprintf(errmsg, "Error opening HDF5 Group %s", groupName);
        return -1;
    }

    //====================================
    // Write dimensions (for arrays only)
    //====================================

    if (nDims != 0) {

        // create field name to store the dimensions
        sprintf(sizeName, "%s_size", dataName);

        // define datatypes
        sizetype = H5Tvlen_create(H5T_STD_I32BE);
        memSizetype = H5Tvlen_create(H5T_NATIVE_INT);

        // open/create the dataset
        exists = H5Lexists(group, sizeName, H5P_DEFAULT);
        if (!exists) //The first time we need to create the dataset
        {
            // create and empty but extendible dataspace
            hdims[0] = 0;
            maxDims[0] = H5S_UNLIMITED;
            sizespace = H5Screate_simple(1, hdims, maxDims);

            // define chunk size for the dataspace
            chunkDims[0] = SLICE_BLOCK;
            cParms = H5Pcreate(H5P_DATASET_CREATE);
            H5Pset_chunk(cParms, 1, chunkDims);

            // create dataset
            sizeset = H5Dcreate(group, sizeName, sizetype, sizespace, H5P_DEFAULT, cParms, H5P_DEFAULT);
            if (sizeset < 0) {
                sprintf(errmsg, "Error creating dataset %s in group %s", sizeName, groupName);
                return -1;
            }
            H5Pclose(cParms);
        }
        else //open the dataset
        {
            sizeset = H5Dopen(group, sizeName, H5P_DEFAULT);
            if (sizeset < 0) {
                sprintf(errmsg, "Error opening dataset %s in group %s", sizeName, groupName);
                return -1;
            }

            // get dataspace
            sizespace = H5Dget_space(sizeset);
            if (sizespace < 0) {
                sprintf(errmsg, "Error getting dataset %s in group %s", sizeName, groupName);
                return -1;
            }
        }

        // get current size
        if (H5Sget_simple_extent_dims(sizespace, hdims, maxDims) < 0) {
            sprintf(errmsg, "Error in H5Sget_simple_extent_dims in %s, group %s", dataName, groupName);
            return -1;
        }
        lastIdx = (long) hdims[0] - 1;

        if (object->timeIdx < lastIdx) {
            sprintf(errmsg, "Internal error: trying to overwrite a slice in %s, group %s", sizeName, groupName);
            return -1;
        }

        if (object->timeIdx == lastIdx) {   // we are replacing the last slice
            startOut[0] = lastIdx;
            countOut[0] = 1;
        }
        else {   // we are writing a new slice

            // increase size
            extDims[0] = object->timeIdx + 1;

            if (H5Dset_extent(sizeset, extDims) < 0) {
                sprintf(errmsg, "Internal error: extend failed in %s, group %s", sizeName, groupName);
                return -1;
            }

            // get new dataspace
            H5Sclose(sizespace);
            sizespace = H5Dget_space(sizeset);

            startOut[0] = hdims[0];
            countOut[0] = object->timeIdx + 1 - hdims[0];
        }

        // select only last slices for writing
        // including those that are going to be created empty
        if (H5Sselect_hyperslab(sizespace, H5S_SELECT_SET, startOut, NULL, countOut, NULL) < 0) {
            sprintf(errmsg, "Error in H5Sselect_hyperslab in %s, group %s", sizeName, groupName);
            return -1;
        }

        // the memory dataspace will contain all the new slices
        memSizespace = H5Screate(H5S_SIMPLE);
        H5Sset_extent_simple(memSizespace, 1, countOut, countOut);

        // prepare buffer for writing
        vl_size = (hvl_t *) malloc(countOut[0] * sizeof(hvl_t));

        // if the current index is not the next one in the dataset
        // then we need to fill with empty rows
        for (i = 0; i < countOut[0] - 1; i++) {
            vl_size[i].len = 1;
            vl_size[i].p = &zero;
        }
        vl_size[i].len = nDims;
        vl_size[i].p = dims;

        //Write in the sizeset
        if (H5Dwrite(sizeset, memSizetype, memSizespace, sizespace, H5P_DEFAULT, vl_size) < 0) {
            sprintf(errmsg, "Error writing dataset %s in group %s", dataName, groupName);
            free(vl_size);
            return -1;
        }

        // clean up
        free(vl_size);
        H5Tclose(sizetype);
        H5Tclose(memSizetype);
        H5Dclose(sizeset);
        H5Sclose(sizespace);
        H5Sclose(memSizespace);
    }

    //====================================
    //            Write data
    //====================================

    // define datatypes
    switch (type) {
        case INT:
            datatype = H5Tvlen_create(H5T_STD_I32BE);
            memDatatype = H5Tvlen_create(H5T_NATIVE_INT);
            break;
        case FLOAT:
            datatype = H5Tvlen_create(H5T_IEEE_F32BE);
            memDatatype = H5Tvlen_create(H5T_NATIVE_FLOAT);
            break;
        case DOUBLE:
            datatype = H5Tvlen_create(H5T_IEEE_F64BE);
            memDatatype = H5Tvlen_create(H5T_NATIVE_DOUBLE);
            break;
        case STRING:
            stringType = H5Tcopy(H5T_C_S1);
            H5Tset_size(stringType, H5T_VARIABLE);
            datatype = H5Tvlen_create(stringType);
            memDatatype = H5Tcopy(datatype);
            H5Tclose(stringType);
            break;
    }

    // open/create the dataset
    exists = H5Lexists(group, dataName, H5P_DEFAULT);
    if (!exists) //The first time we need to create the dataset
    {
        // create and empty but extendible dataspace
        hdims[0] = 0;
        maxDims[0] = H5S_UNLIMITED;
        dataspace = H5Screate_simple(1, hdims, maxDims);

        // define chunk size for the dataspace
        chunkDims[0] = SLICE_BLOCK;
        cParms = H5Pcreate(H5P_DATASET_CREATE);
        H5Pset_chunk(cParms, 1, chunkDims);

        // create dataset
        dataset = H5Dcreate(group, dataName, datatype, dataspace, H5P_DEFAULT, cParms, H5P_DEFAULT);
        if (dataset < 0) {
            sprintf(errmsg, "Error creating dataset %s in group %s", dataName, groupName);
            return -1;
        }
        H5Pclose(cParms);

    }
    else //open the dataset
    {
        dataset = H5Dopen(group, dataName, H5P_DEFAULT);
        if (dataset < 0) {
            sprintf(errmsg, "Error opening dataset %s in group %s", dataName, groupName);
            return -1;
        }

        // get dataspace
        dataspace = H5Dget_space(dataset);
        if (dataspace < 0) {
            sprintf(errmsg, "Error getting dataset %s in group %s", dataName, groupName);
            return -1;
        }

    }

    // get current size
    if (H5Sget_simple_extent_dims(dataspace, hdims, maxDims) < 0) {
        sprintf(errmsg, "Error in H5Sget_simple_extent_dims in %s, group %s", dataName, groupName);
        return -1;
    }
    lastIdx = (long) hdims[0] - 1;

    if (object->timeIdx < lastIdx) {
        sprintf(errmsg, "Internal error: trying to overwrite a slice in %s, group %s", dataName, groupName);
        return -1;
    }

    if (object->timeIdx == lastIdx) {   // we are replacing the last slice
        startOut[0] = lastIdx;
        countOut[0] = 1;
    }
    else {   // we are writing a new slice

        // increase size
        extDims[0] = object->timeIdx + 1;
        if (H5Dset_extent(dataset, extDims) < 0) {
            sprintf(errmsg, "Internal error: extend failed in %s, group %s", dataName, groupName);
            return -1;
        }

        // get new dataspace
        H5Sclose(dataspace);
        dataspace = H5Dget_space(dataset);

        startOut[0] = hdims[0];
        countOut[0] = object->timeIdx + 1 - hdims[0];
    }

    // select only last slices for writing
    // including those that are going to be created empty
    if (H5Sselect_hyperslab(dataspace, H5S_SELECT_SET, startOut, NULL, countOut, NULL) < 0) {
        sprintf(errmsg, "Error in H5Sselect_hyperslab in %s, group %s", dataName, groupName);
        return -1;
    }

    // the memory dataspace will contain all the new slices
    memDataspace = H5Screate(H5S_SIMPLE);
    H5Sset_extent_simple(memDataspace, 1, countOut, countOut);

    // prepare buffer for writing
    vl_data = (hvl_t *) malloc(countOut[0] * sizeof(hvl_t));
    // if the current index is not the next one in the dataset
    // then we need to fill with empty rows
    for (i = 0; i < countOut[0] - 1; i++) {
        vl_data[i].len = 0;
        vl_data[i].p = NULL;
    }
    vl_data[i].len = totSize;
    vl_data[i].p = data;

    //Write in the dataset
    if (H5Dwrite(dataset, memDatatype, memDataspace, dataspace, H5P_DEFAULT, vl_data) < 0) {
        sprintf(errmsg, "Error writing dataset %s in group %s", dataName, groupName);
        free(vl_data);
        return -1;
    }

    // clean up
    free(vl_data);
    H5Gclose(group);
    H5Tclose(datatype);
    H5Tclose(memDatatype);
    H5Dclose(dataset);
    H5Sclose(dataspace);
    H5Sclose(memDataspace);
    return 0;
}


/** Read a single slice from a field
 *  The size of the field is read in the associated "_size" field
 *  nDims should be 0 for scalars
 *  If the required slice does not exist (is higher than the last slice)
 *  then we return an empty result ("empty" value for scalars, size 0 for arrays)
 *  It is possible to read only the size (not the data) by setting type to DIMENSION */

// dgm Original HDF5 getDataSliceFromObject renamed imas_getDataSliceFromObject - called by the IDAM IMAS plugin

int imas_getDataSliceFromObject(void * obj, char * path, int index, int type, int nDims, int * dims, void ** data)
{
    obj_t * object = (obj_t *) obj;
    hid_t hObj = object->handle;
    int timeIdx = object->timeIdx;

    int i, totSize;
    char tmpstr[1024], dataName[1024], groupName[1024], sizeName[1024];
    hid_t group, dataset, datatype, memDatatype, dataspace, memDataspace;
    hid_t sizeset, sizetype, memSizetype, sizespace, memSizespace;
    hid_t stringType;
    hsize_t hdims[1], startOut[1];
    hsize_t countOut[1];
    hsize_t maxDims[1];
    hvl_t vl_data, vl_size;
    int nDimsRead;

    // split path into group name and field name
    for (i = strlen(path) - 1; i >= 0 && path[i] != '/'; i--);
    strcpy(groupName, path);
    groupName[i] = 0;
    strcpy(dataName, &path[i + 1]);

    // Generate correct group path by substituting leading field by index number
    for (i = 0; groupName[i] != '/' && groupName[i] != 0; i++);
    if (groupName[i] == 0)
        sprintf(tmpstr, "%d", index);
    else
        sprintf(tmpstr, "%d/%s", index, &groupName[i + 1]);
    strcpy(groupName, tmpstr);

    // open container group
    group = openGroup(hObj, groupName, NO_CREATE, NO_CLEAR);

    // if the field cannot be opened then consider that it is empty
    if (group < 0 || !H5Lexists(group, dataName, H5P_DEFAULT)) {
        for (i = 0; i < nDims; i++) {
            dims[i] = 0;
        }
        *data = NULL;
    }
    else {
        //===================================
        // Read dimensions (for arrays only)
        //===================================

        if (nDims > 0) {

            // create field name to read dimensions from
            sprintf(sizeName, "%s_size", dataName);

            // define datatypes
            sizetype = H5Tvlen_create(H5T_STD_I32BE);
            memSizetype = H5Tvlen_create(H5T_NATIVE_INT);

            // memory dataspace is a single set of dimensions (single slice)
            hdims[0] = 1;
            memSizespace = H5Screate(H5S_SIMPLE);
            H5Sset_extent_simple(memSizespace, 1, hdims, hdims);

            // open the dataset
            sizeset = H5Dopen(group, sizeName, H5P_DEFAULT);
            if (sizeset < 0) {
                sprintf(errmsg, "Error opening dataset %s in group %s", sizeName, groupName);
                return -1;
            }

            // get dataspace
            sizespace = H5Dget_space(sizeset);
            if (sizespace < 0) {
                sprintf(errmsg, "Error getting dataset %s in group %s", sizeName, groupName);
                return -1;
            }

            //Get last dimension
            if (H5Sget_simple_extent_dims(sizespace, hdims, maxDims) < 0) {
                sprintf(errmsg, "Error in H5Sget_simple_extent_dims in %s, group %s", dataName, groupName);
                return -1;
            }

            if (timeIdx >= hdims[0]) // we are trying to read a non existing slice
            {
                for (i = 0; i < nDims; i++) {
                    dims[i] = 0;
                }
            } else {

                //Select only one slice for reading
                startOut[0] = timeIdx;
                countOut[0] = 1;
                if (H5Sselect_hyperslab(sizespace, H5S_SELECT_SET, startOut, NULL, countOut, NULL) < 0) {
                    sprintf(errmsg, "Error in H5Sselect_hyperslab in %s, group %s", sizeName, groupName);
                    return -1;
                }

                // Read from the sizeset
                if (H5Dread(sizeset, memSizetype, memSizespace, sizespace, H5P_DEFAULT, &vl_size) < 0) {
                    sprintf(errmsg, "Error reading dataset %s in group %s", dataName, groupName);
                    return -1;
                }

                // extract dimensions
                nDimsRead = vl_size.len;
                for (i = 0, totSize = 1; i < nDimsRead; i++) {
                    dims[i] = ((int *) (vl_size.p))[i];
                    totSize *= dims[i];
                }
                H5Dvlen_reclaim(memSizetype, memSizespace, H5P_DEFAULT, &vl_size);
            }

            // clean up
            H5Tclose(sizetype);
            H5Tclose(memSizetype);
            H5Dclose(sizeset);
            H5Sclose(sizespace);
            H5Sclose(memSizespace);
        }

        // if we only want the dimensions of the array, then stop here
        if (type == DIMENSION) {
            H5Gclose(group);
            *data = malloc(sizeof(int));
            **(int **) data = nDimsRead; // return the number of dimensions as data
            return 0;
        }

        //===================================
        //            Read data
        //===================================

        // define datatypes
        switch (type) {
            case INT:
                datatype = H5Tvlen_create(H5T_STD_I32BE);
                memDatatype = H5Tvlen_create(H5T_NATIVE_INT);
                break;
            case FLOAT:
                datatype = H5Tvlen_create(H5T_IEEE_F32BE);
                memDatatype = H5Tvlen_create(H5T_NATIVE_FLOAT);
                break;
            case DOUBLE:
                datatype = H5Tvlen_create(H5T_IEEE_F64BE);
                memDatatype = H5Tvlen_create(H5T_NATIVE_DOUBLE);
                break;
            case STRING:
                stringType = H5Tcopy(H5T_C_S1);
                H5Tset_size(stringType, H5T_VARIABLE);
                datatype = H5Tvlen_create(stringType);
                memDatatype = H5Tcopy(datatype);
                H5Tclose(stringType);
                break;
        }

        // memory dataspace is a single set of dimensions (single slice)
        hdims[0] = 1;
        memDataspace = H5Screate(H5S_SIMPLE);
        H5Sset_extent_simple(memDataspace, 1, hdims, hdims);

        // open the dataset
        dataset = H5Dopen(group, dataName, H5P_DEFAULT);
        if (dataset < 0) {
            sprintf(errmsg, "Error opening dataset %s in group %s", dataName, groupName);
            return -1;
        }

        // get dataspace
        dataspace = H5Dget_space(dataset);
        if (dataspace < 0) {
            sprintf(errmsg, "Error getting dataset %s in group %s", dataName, groupName);
            return -1;
        }

        //Get last dimension
        if (H5Sget_simple_extent_dims(dataspace, hdims, maxDims) < 0) {
            sprintf(errmsg, "Error in H5Sget_simple_extent_dims in %s, group %s", dataName, groupName);
            return -1;
        }

        if (timeIdx >= hdims[0]) // we are trying to read a non existing slice
        {
            *data = NULL;
        } else {

            //Select only one slice for reading
            startOut[0] = timeIdx;
            countOut[0] = 1;
            if (H5Sselect_hyperslab(dataspace, H5S_SELECT_SET, startOut, NULL, countOut, NULL) < 0) {
                sprintf(errmsg, "Error in H5Sselect_hyperslab in %s, group %s", dataName, groupName);
                return -1;
            }

            // Read from the dataset
            if (H5Dread(dataset, memDatatype, memDataspace, dataspace, H5P_DEFAULT, &vl_data) < 0) {
                sprintf(errmsg, "Error writing dataset %s in group %s", dataName, groupName);
                return -1;
            }
            *data = vl_data.p;
        }

        // clean up
        H5Gclose(group);
        H5Tclose(datatype);
        H5Tclose(memDatatype);
        H5Dclose(dataset);
        H5Sclose(dataspace);
        H5Sclose(memDataspace);
    }

    // if it is a scalar and we could not read it then return
    // the default "empty" value
    if (nDims == 0) {
        dims[0] = 1;        // Single Scalar Values
        if (*data == NULL) {
            switch (type) {
                case INT:
                    *data = malloc(sizeof(int));
                    **(int **) data = EMPTY_INT;
                    break;
                case FLOAT:
                    *data = malloc(sizeof(float));
                    **(float **) data = EMPTY_FLOAT;
                    break;
                case DOUBLE:
                    *data = malloc(sizeof(double));
                    **(double **) data = EMPTY_DOUBLE;
                    break;
                case STRING:
/*
                *data = malloc(sizeof(char*));
                **(char ***)data = (char *)malloc(sizeof(char));
                ***(char ***)data = 0;
*/
                    *data = malloc(sizeof(char));
                    **(char **) data = '\0';
                    break;
            }
        }
    }

    return 0;
}

/** Find the last element in the list of objects
 *  that are contained in the root object */
obj_t * lastDescendant(obj_t * root)
{
    obj_t * obj;
    for (obj = root; obj->nextObj != NULL; obj = obj->nextObj);
    return obj;
}

// dgm Original HDF5 hdf5BeginObject renamed imas_hdf5BeginObject - called by the IDAM IMAS plugin

/** Open or create a group corresponding to an object (for writing).
 *  Stores the group handle in an object structure.
 *  If necessary, delete current content of the group. */

void * imas_hdf5BeginObject(int expIdx, void * obj, int index, const char * relPath, int isTimed)
{
    obj_t * object = (obj_t *) obj;
    obj_t * output;
    hid_t group;
    char * groupName, * tmpstr;
    char * subName;
    hid_t root;
    int clearContent;

    groupName = (char *) malloc(strlen(relPath) + 20);
    output = (obj_t *) malloc(sizeof(obj_t));
    if (output == NULL) {
        sprintf(errmsg, "Error in hdf5BeginObject: out of memory");
        return NULL;
    }

    if (obj == NULL) {
        //-------------- Object is at top level --------------
        if (isTimed == NON_TIMED)
            sprintf(groupName, "%s/non_timed", relPath);
        else
            sprintf(groupName, "%s/timed", relPath);
        root = hdf5Files[expIdx];
    } else {
        //-------------- Object is inside another object --------------

        // if this object is a slice of a time-dependent object, then the group
        // was already opened and initialized by the containing object (that contains all the times).
        // We recognize this case because the field name is, by convention, "ALLTIMES"

        if (STR_EQUALS(relPath, "ALLTIMES")) {
            output->handle = object->handle;
            output->dim = -99;       // not used for output objects
            output->nextObj = NULL;  // this is the last object for now
            if (object->timeIdx < 0)  // we are doing a put
                output->timeIdx = index;
            else                            // we are doing a putSlice
                output->timeIdx = object->timeIdx;
            lastDescendant(object)->nextObj = output;
            free(groupName);
            return output;
        }

        // other cases correspond to real nesting of arrays of structures,
        // so we can proceed
        sprintf(groupName, "%s", relPath);
        root = object->handle;
        // Generate correct path by substituting leading field by index number
        for (subName = groupName; *subName != '/' && *subName != 0; subName++);
        if (*subName == 0) {
            sprintf(errmsg, "Error in hdf5BeginObject: malformed path: %s", groupName);
            free(groupName);
            free(output);
            return NULL;
        }
        subName++;
        tmpstr = (char *) malloc(strlen(relPath) + 20);
        sprintf(tmpstr, "%d/%s", index, subName);
        strcpy(groupName, tmpstr);
        free(tmpstr);
    }

    // we are asked to delete all the data for this timed object
    // or it is a non-timed object and we are going to overwrite it
    if (isTimed == TIMED_CLEAR || isTimed == NON_TIMED) {
        clearContent = CLEAR;
    } else {
        clearContent = NO_CLEAR;
    }

    // open group (create if it doesn't exist)
    group = openGroup(root, (const char *) groupName, CREATE, clearContent);
    free(groupName);
    if (group < 0) {
        sprintf(errmsg, "Error in hdf5BeginObject: could not open/create group %s", groupName);
        free(output);
        return NULL;
    }

    // store handle and return it
    output->handle = group;
    output->dim = -99;       // not used for output objects
    output->nextObj = NULL;  // this is the last object for now

    // index of the slice we need to write to
    if (obj == NULL) {        // object is a top level
        if (isTimed == TIMED) // we are doing a putSlice
            output->timeIdx = sliceIdx1;
        else if (isTimed == TIMED_CLEAR) // we are doing a put
            output->timeIdx = -1;      // the index is going to be defined by the next hdf5BeginObject
        else   // non timed object
            output->timeIdx = 0;
    } else { // for nested objects, inherit the slice index
        output->timeIdx = object->timeIdx;
    }

    // if this object is not at the top level, then add it to the list
    // of descendants of the containing object
    if (obj != NULL) {
        lastDescendant(object)->nextObj = output;
    }

    return (void *) output;
}

// dgm Original HDF5 hdf5GetObject renamed imas_hdf5GetObject - called by the IDAM IMAS plugin

/** Open the group corresponding to an object
 *  and store its handle */
int imas_hdf5GetObject(int expIdx, char * hdf5Path, char * cpoPath, void ** obj, int isTimed)
{
    char fullPath[1024];
    H5G_info_t group_info;
    obj_t * newObj = (obj_t *) malloc(sizeof(obj_t));
    int nDims, dim1;

    // timed and non-timed objects are kept in different fields
    if (isTimed)
        sprintf(fullPath, "%s/%s/timed", hdf5Path, cpoPath);
    else
        sprintf(fullPath, "%s/%s/non_timed", hdf5Path, cpoPath);

    // try to open the group corresponding to the object
    newObj->handle = openGroup(hdf5Files[expIdx], fullPath, NO_CREATE, NO_CLEAR);

    if (newObj->handle < 0) {
        // if it could not be read, then return an empty object
        newObj->dim = 0;
    } else if (!isTimed) {
        // if the object is not timed, then the size is the current number of elements
        if (H5Gget_info(newObj->handle, &group_info) < 0) {
            free(newObj);
            *obj = NULL;
            sprintf(errmsg, "Error in hdf5GetObject: cannot get information for group %s", fullPath);
            return -1;
        }
        newObj->dim = group_info.nlinks;
    } else {
        // if the object is timed, then the size is the number of time slices
// dgm changed to plugin function
        if (imas_hdf5GetDimension(expIdx, hdf5Path, "time", &nDims, &dim1, NULL, NULL, NULL, NULL, NULL, NULL) < 0) {
            free(newObj);
            *obj = NULL;
            sprintf(errmsg, "Error in hdf5GetObject: cannot get time for group %s", fullPath);
            return -1;
        }
        newObj->dim = dim1;
    }

    // If the object is not time-dependent then we are going to read the first (and only) slice.
    // Otherwise the slice to be read is going to be defined by the next hdf5GetObjectFromObject.
    // For now, setting timeIdx to -1 means, by convention, that we want to read all time slices.
    if (!isTimed)
        newObj->timeIdx = 0;
    else
        newObj->timeIdx = -1;

    // This is necessarily a top-level object,
    // we yet have to read the contained objects
    newObj->nextObj = NULL;

    *obj = newObj;
    return 0;
}

// dgm Original HDF5 hdf5GetObjectSlice renamed imas_hdf5GetObjectSlice - called by the IDAM IMAS plugin

/** Open the group corresponding to an object slice
 *  and store its handle */
int imas_hdf5GetObjectSlice(int expIdx, char * hdf5Path, char * cpoPath, double time, void ** obj)
{
    char fullPath[1024];
    obj_t * newObj = (obj_t *) malloc(sizeof(obj_t));

    // timed and non-timed objects are kept in different fields
    // here we are dealing with timed fields
    sprintf(fullPath, "%s/%s/timed", hdf5Path, cpoPath);

    // try to open the group corresponding to the object
    newObj->handle = openGroup(hdf5Files[expIdx], fullPath, NO_CREATE, NO_CLEAR);

    if (newObj->handle < 0) {
        // if it could not be read, then return an empty object
        newObj->dim = 0;
    } else {
        // otherwise this object contains a single slice
        newObj->dim = 1;
    }

    // The slice to be read by the next hdf5GetObjectFromObject is the closest slice in time
    if (sliceIdx2 < 0 || time - sliceTime1 < sliceTime2 - time) {
        newObj->timeIdx = sliceIdx1;
    }
    else {
        newObj->timeIdx = sliceIdx2;
    }

    // This is necessarily a top-level object,
    // we yet have to read the contained objects
    newObj->nextObj = NULL;

    *obj = newObj;
    return 0;
}

// dgm Original HDF5 hdf5GetObjectFromObject renamed imas_hdf5GetObjectFromObject - called by the IDAM IMAS plugin

/** Open an object inside another object
*/
int imas_hdf5GetObjectFromObject(void * obj, char * hdf5Path, int idx, void ** dataObj)
{
    char groupName[1024];
    char * subName;
    H5G_info_t group_info;
    obj_t * newObj = (obj_t *) malloc(sizeof(obj_t));

    //------------------------------------------------------------------------------
    // if we are starting to read a slice of a time-dependent object
    // (in this case it is contained in a bigger object that contains all the times).
    // We recognize this case because the field name is, by convention, "ALLTIMES"
    //------------------------------------------------------------------------------

    if (STR_EQUALS(hdf5Path, "ALLTIMES")) {

        // we have already opened the HDF5 group when opening the containing object,
        // so just copy the handle
        newObj->handle = ((obj_t *) obj)->handle;

        // get the current number of elements
        if (H5Gget_info(newObj->handle, &group_info) < 0) {
            free(newObj);
            sprintf(errmsg, "Error in hdf5GetObjectFromObject: cannot get information for group %s", hdf5Path);
            *dataObj = NULL;
            return -1;
        }
        newObj->dim = group_info.nlinks;

        // by convention, timeIdx has been set to -1 if we are dealing with a get rather than a getSlice.
        // In that case, use the given index.
        // Otherwise, timeIdx has been set to the index of the slice to read
        if (((obj_t *) obj)->timeIdx < 0) {
            newObj->timeIdx = idx;
        } else {
            newObj->timeIdx = ((obj_t *) obj)->timeIdx;
        }

        //------------------------------------------------------------------------------
        // other cases correspond to real nesting of arrays of structures
        //------------------------------------------------------------------------------

    } else {
        // Generate correct path by substituting leading field by index number
        for (subName = hdf5Path; *subName != '/' && *subName != 0; subName++);
        if (*subName == 0) {
            sprintf(errmsg, "Error in hdf5GetObjectFromObject: malformed path: %s", hdf5Path);
            free(newObj);
            *dataObj = NULL;
            return -1;
        }
        subName++;
        sprintf(groupName, "%d/%s", idx, subName);

        // try to open the group corresponding to the object
        newObj->handle = openGroup(((obj_t *) obj)->handle, groupName, NO_CREATE, NO_CLEAR);

        if (newObj->handle < 0) {
            // if it could not be read, then return an empty object
            newObj->dim = 0;
        } else {
            // otherwise get the current number of elements
            if (H5Gget_info(newObj->handle, &group_info) < 0) {
                free(newObj);
                sprintf(errmsg, "Error in hdf5GetObjectFromObject: cannot get information for group %s", hdf5Path);
                *dataObj = NULL;
                return -1;
            }
            newObj->dim = group_info.nlinks;
        }

        // Inherit the time to be read from the containing object
        newObj->timeIdx = ((obj_t *) obj)->timeIdx;
    }

    newObj->nextObj = NULL;  // this is the last object for now

    // add this object to the list of descendants of the containing object
    lastDescendant((obj_t *) obj)->nextObj = newObj;

    *dataObj = (void *) newObj;

    return 0;
}

// dgm Original HDF5 hdf5ReleaseObject renamed imas_hdf5ReleaseObject - called by the IDAM IMAS plugin

/** Free memory and release HDF5 handles
 *  for the specified object and all its descendants */
void imas_hdf5ReleaseObject(void * obj)
{
    obj_t * currentObj, * nextObj;
    hid_t handle = -1;
    hid_t topHandle = ((obj_t *) obj)->handle;

    for (currentObj = (obj_t *) obj; currentObj != NULL;) {
        nextObj = currentObj->nextObj;
        handle = currentObj->handle;
        if (handle != topHandle || currentObj == obj) {  // do not close the group if it was already closed!
            H5Gclose(
                    handle);                        // this can only happen for slice objects that repeat the handle of the top level object
        }
        free(currentObj);
        currentObj = nextObj;
    }
}

