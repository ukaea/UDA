/*---------------------------------------------------------------
* v1 UDA Plugin: Access data objects via Amazon S3 API
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
#ifdef __cplusplus
extern "C" {
#endif

#include "s3.h"

#include <strings.h>
#include <client/accAPI.h>
#include <structures/struct.h>
#include <clientserver/initStructs.h>
#include <clientserver/stringUtils.h>
#include <clientserver/xmlStructs.h>

#include <libs3.h>
#include <sys/stat.h>
#include <unistd.h>

#ifndef SLEEP_UNITS_PER_SECOND
#define SLEEP_UNITS_PER_SECOND 1
#endif


static int do_help(IDAM_PLUGIN_INTERFACE* idam_plugin_interface);

static int do_version(IDAM_PLUGIN_INTERFACE* idam_plugin_interface);

static int do_builddate(IDAM_PLUGIN_INTERFACE* idam_plugin_interface);

static int do_defaultmethod(IDAM_PLUGIN_INTERFACE* idam_plugin_interface);

static int do_maxinterfaceversion(IDAM_PLUGIN_INTERFACE* idam_plugin_interface);

static int do_get(IDAM_PLUGIN_INTERFACE* idam_plugin_interface);

static int do_put(IDAM_PLUGIN_INTERFACE* idam_plugin_interface);

static void S3_init(void);
static S3Status responsePropertiesCallback (const S3ResponseProperties *properties, void *callbackData);
static void responseCompleteCallback(S3Status status, const S3ErrorDetails *error, void *callbackData);
static S3Status getObjectDataCallback(int bufferSize, const char *buffer, void *callbackData);
static int should_retry(void);

static char *hostname=NULL;
static char *accessKeyIdG = NULL;
static char *secretAccessKeyG = NULL;

static int showResponsePropertiesG = 0;
static S3Protocol protocolG = S3ProtocolHTTPS;
static S3UriStyle uriStyleG = S3UriStylePath;
static int statusG = 0;
static int retriesG = 5;
static char errorDetailsG[4096] = { 0 };

int s3(IDAM_PLUGIN_INTERFACE* idam_plugin_interface)
{
    idamSetLogLevel(UDA_LOG_DEBUG);

    int err = 0;

    static short init = 0;
    
    unsigned short housekeeping;
    
    if (idam_plugin_interface->interfaceVersion > THISPLUGIN_MAX_INTERFACE_VERSION) {
        UDA_LOG(UDA_LOG_ERROR, "Plugin Interface Version Unknown to this plugin: Unable to execute the request!\n");
        THROW_ERROR(999, "Plugin Interface Version Unknown to this plugin: Unable to execute the request!");
    }

    idam_plugin_interface->pluginVersion = THISPLUGIN_VERSION;

    REQUEST_BLOCK* request_block = idam_plugin_interface->request_block;

    housekeeping = idam_plugin_interface->housekeeping;

//----------------------------------------------------------------------------------------
// Arguments and keywords 

    unsigned short int isReset = 0;

    int i;
    for (i = 0; i < request_block->nameValueList.pairCount; i++) {
        if (!strcasecmp(request_block->nameValueList.nameValue[i].name, "reset") ||
            !strcasecmp(request_block->nameValueList.nameValue[i].name, "initialise")) {
            isReset = 1;
            break;
        }
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

    if (isReset || housekeeping || !strcasecmp(request_block->function, "reset")) {

        if (!init) return 0;        // Not previously initialised: Nothing to do!

// Free Heap & reset counters

        hostname=NULL;
        accessKeyIdG = NULL;
        secretAccessKeyG = NULL;
	
	S3_deinitialize();
	
        init = 0;

        if (!isReset) return 0;        // Step to Initialisation
    }

    //----------------------------------------------------------------------------------------
    // Initialise

    if (!STR_IEQUALS(request_block->function, "help") &&
        (!init || STR_IEQUALS(request_block->function, "init") || STR_IEQUALS(request_block->function, "initialise"))) {
	
	accessKeyIdG = getenv("S3_ACCESS_KEY_ID");
        if (!accessKeyIdG) THROW_ERROR(999, "Missing: S3_ACCESS_KEY_ID!");
	
        secretAccessKeyG = getenv("S3_SECRET_ACCESS_KEY");
        if (!secretAccessKeyG) THROW_ERROR(999, "Missing: S3_SECRET_ACCESS_KEY!");
	
        hostname = getenv("S3_HOSTNAME");
	
	S3_init();

        init = 1;
        if (!strcasecmp(request_block->function, "init") || !strcasecmp(request_block->function, "initialise")) {
            return 0;
        }
    }

    //----------------------------------------------------------------------------------------
    // Plugin Functions
    //----------------------------------------------------------------------------------------

    if (!strcasecmp(request_block->function, "get")) {
        err = do_get(idam_plugin_interface);
    } else if (!strcasecmp(request_block->function, "help")) {
        do_help(idam_plugin_interface);
    } else if (!strcasecmp(request_block->function, "version")) {
        do_version(idam_plugin_interface);
    } else if (!strcasecmp(request_block->function, "builddate")) {
        do_builddate(idam_plugin_interface);
    } else if (!strcasecmp(request_block->function, "defaultmethod")) {
        do_defaultmethod(idam_plugin_interface);
    } else if (!strcasecmp(request_block->function, "maxinterfaceversion")) {
        do_maxinterfaceversion(idam_plugin_interface);
    } else {
        THROW_ERROR(999, "Unknown function requested!");
    }

//--------------------------------------------------------------------------------------
// Housekeeping

    return err;
}

static int do_help(IDAM_PLUGIN_INTERFACE* idam_plugin_interface)
{
    char* help = "\ns3: Add Functions Names, Syntax, and Descriptions\n\n";
    const char* desc = "s3:get(object=object, bucket=bucket)";
    
    UDA_LOG(UDA_LOG_ERROR, "S3:help() requested\n");

    return setReturnDataString(idam_plugin_interface->data_block, help, desc);
}

static int do_version(IDAM_PLUGIN_INTERFACE* idam_plugin_interface)
{
    return setReturnDataIntScalar(idam_plugin_interface->data_block, THISPLUGIN_VERSION, NULL);
}

static int do_builddate(IDAM_PLUGIN_INTERFACE* idam_plugin_interface)
{
    return setReturnDataString(idam_plugin_interface->data_block, __DATE__, NULL);
}

static int do_defaultmethod(IDAM_PLUGIN_INTERFACE* idam_plugin_interface)
{
    return setReturnDataString(idam_plugin_interface->data_block, THISPLUGIN_DEFAULT_METHOD, NULL);
}

static int do_maxinterfaceversion(IDAM_PLUGIN_INTERFACE* idam_plugin_interface)
{
    return setReturnDataIntScalar(idam_plugin_interface->data_block, THISPLUGIN_MAX_INTERFACE_VERSION, NULL);
}


//----------------------------------------------------------------------------------------
// S3::get(object=object, bucket=bucket)

static int do_get(IDAM_PLUGIN_INTERFACE* idam_plugin_interface)
{
    UDA_LOG(UDA_LOG_ERROR, "S3:get() requested\n");
    
    DATA_BLOCK* data_block = idam_plugin_interface->data_block;
    initDataBlock(data_block);
    
    REQUEST_BLOCK* request_block = idam_plugin_interface->request_block;

    const char* object = NULL, * bucket = NULL;

    bool isObject = 0, isBucket = 0;

    isObject = findStringValue(&request_block->nameValueList, &object, "object");   
    isBucket = findStringValue(&request_block->nameValueList, &bucket, "bucket"); 

    if (!isObject || !isBucket) {
        THROW_ERROR(999, "S3::get() No data object name or S3 bucket has been specified!");
    }
    
    UDA_LOG(UDA_LOG_ERROR, "S3::get() bucket=%s, object=%s\n", bucket, object);
        
    const char *bucketName = (const char *)bucket;
    const char *key        = (const char *)object;
    
    int64_t ifModifiedSince = -1, ifNotModifiedSince = -1;
    const char *ifMatch = 0, *ifNotMatch = 0;
    uint64_t startByte = 0, byteCount = 0;
   
    const char *filename = 0;
    FILE *outfile = 0;
    struct stat fileAttrib;

    if (filename) {
        // Stat the file, and if it doesn't exist, open it in w mode
        if (stat(filename, &fileAttrib) == -1) {
            outfile = fopen(filename, "w");
        }
        else {
            // Open in r+ so that we don't truncate the file, just in case
            // there is an error and we write no bytes, we leave the file
            // unmodified
            outfile = fopen(filename, "r+");
        }
        
        if (!outfile) THROW_ERROR(999, "S3::get() Failed to open temp file buffer!");
    }
    
    S3BucketContext bucketContext =
    {
        0,
        bucketName,
        protocolG,
        uriStyleG,
        accessKeyIdG,
        secretAccessKeyG
    };

    S3GetConditions getConditions =
    {
        ifModifiedSince,
        ifNotModifiedSince,
        ifMatch,
        ifNotMatch
    };

    S3GetObjectHandler getObjectHandler =
    {
        { &responsePropertiesCallback, &responseCompleteCallback },
        &getObjectDataCallback
    };

    do {
        S3_get_object(&bucketContext, key, &getConditions, startByte,
                      byteCount, 0, &getObjectHandler, outfile);
    } while (S3_status_is_retryable(statusG) && should_retry());

    if (statusG != S3StatusOK) {
        THROW_ERROR(999, "S3::get() Failed to access data!");
	//printError();
    }

    fclose(outfile);  
    
    // stat intermediate file for byte count  
    if (stat(filename, &fileAttrib) == -1) THROW_ERROR(999, "S3::get() Failed to query intermediate file!");
        
    int count = (int)fileAttrib.st_size;
    char *buffer = (char *)malloc(count*sizeof(char));
       
    FILE *fd = fopen(filename, "rb");	    	  
    fread(buffer, count, sizeof(char), fd);
    fclose(fd);
    
    UDA_LOG(UDA_LOG_ERROR, "S3::get() Data retrieved!\n");

    if(buffer != NULL && count > 0){  
        
       data_block->rank = 0;
       data_block->order = -1;
       data_block->data_type = UDA_TYPE_UNSIGNED_CHAR;
       data_block->dims = NULL;
       data_block->data_n = count;   
       data_block->data = buffer;            
    }
    
    return 0;
}

//----------------------------------------------------------------------------------------
// S3::put(object=object, bucket=bucket)

static int do_put(IDAM_PLUGIN_INTERFACE* idam_plugin_interface)
{
    UDA_LOG(UDA_LOG_ERROR, "S3:put() requested\n");

    return 0;
}

static void S3_init(void)
{
    S3Status status;
    const char *hostname = getenv("S3_HOSTNAME");
    
    if ((status = S3_initialize("s3", S3_INIT_ALL, hostname))
        != S3StatusOK) {
        fprintf(stderr, "Failed to initialize libs3: %s\n", 
                S3_get_status_name(status));
        exit(-1);
    }
}

// response properties callback ----------------------------------------------

// This callback does the same thing for every request type: prints out the
// properties if the user has requested them to be so
static S3Status responsePropertiesCallback
    (const S3ResponseProperties *properties, void *callbackData)
{
    (void) callbackData;

    if (!showResponsePropertiesG) {
        return S3StatusOK;
    }

#define print_nonnull(name, field)                                 \
    do {                                                           \
        if (properties-> field) {                                  \
            printf("%s: %s\n", name, properties-> field);          \
        }                                                          \
    } while (0)
    
    print_nonnull("Content-Type", contentType);
    print_nonnull("Request-Id", requestId);
    print_nonnull("Request-Id-2", requestId2);
    if (properties->contentLength > 0) {
        printf("Content-Length: %lld\n", 
               (unsigned long long) properties->contentLength);
    }
    print_nonnull("Server", server);
    print_nonnull("ETag", eTag);
    if (properties->lastModified > 0) {
        char timebuf[256];
        time_t t = (time_t) properties->lastModified;
        // gmtime is not thread-safe but we don't care here.
        strftime(timebuf, sizeof(timebuf), "%Y-%m-%dT%H:%M:%SZ", gmtime(&t));
        printf("Last-Modified: %s\n", timebuf);
    }
    int i;
    for (i = 0; i < properties->metaDataCount; i++) {
        printf("x-amz-meta-%s: %s\n", properties->metaData[i].name,
               properties->metaData[i].value);
    }

    return S3StatusOK;
}


// response complete callback ------------------------------------------------

// This callback does the same thing for every request type: saves the status
// and error stuff in global variables
static void responseCompleteCallback(S3Status status,
                                     const S3ErrorDetails *error, 
                                     void *callbackData)
{
    (void) callbackData;

    statusG = status;
    // Compose the error details message now, although we might not use it.
    // Can't just save a pointer to [error] since it's not guaranteed to last
    // beyond this callback
    int len = 0;
    if (error && error->message) {
        len += snprintf(&(errorDetailsG[len]), sizeof(errorDetailsG) - len,
                        "  Message: %s\n", error->message);
    }
    if (error && error->resource) {
        len += snprintf(&(errorDetailsG[len]), sizeof(errorDetailsG) - len,
                        "  Resource: %s\n", error->resource);
    }
    if (error && error->furtherDetails) {
        len += snprintf(&(errorDetailsG[len]), sizeof(errorDetailsG) - len,
                        "  Further Details: %s\n", error->furtherDetails);
    }
    if (error && error->extraDetailsCount) {
        len += snprintf(&(errorDetailsG[len]), sizeof(errorDetailsG) - len,
                        "%s", "  Extra Details:\n");
        int i;
        for (i = 0; i < error->extraDetailsCount; i++) {
            len += snprintf(&(errorDetailsG[len]), 
                            sizeof(errorDetailsG) - len, "    %s: %s\n", 
                            error->extraDetails[i].name,
                            error->extraDetails[i].value);
        }
    }
}

// get object ----------------------------------------------------------------

static S3Status getObjectDataCallback(int bufferSize, const char *buffer,
                                      void *callbackData)
{
    FILE *outfile = (FILE *) callbackData;

    size_t wrote = fwrite(buffer, 1, bufferSize, outfile);
    
    return ((wrote < (size_t) bufferSize) ? 
            S3StatusAbortedByCallback : S3StatusOK);
}

static int should_retry(void)
{
    if (retriesG--) {
        // Sleep before next retry; start out with a 1 second sleep
        static int retrySleepInterval = 1 * SLEEP_UNITS_PER_SECOND;
        sleep(retrySleepInterval);
        // Next sleep 1 second longer
        retrySleepInterval++;
        return 1;
    }

    return 0;
}

#ifdef __cplusplus
}
#endif
