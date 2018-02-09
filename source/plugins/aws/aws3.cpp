/*---------------------------------------------------------------
* v1 UDA Plugin: Create a source argument for UDA data access requests
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
#include "aws3.h"

extern "C" {
#include <strings.h>
#include <client/accAPI.h>
#include <structures/struct.h>
#include <clientserver/initStructs.h>
#include <clientserver/stringUtils.h>
#include <clientserver/xmlStructs.h>
}

#include <aws/core/Aws.h>
#include <aws/s3/S3Client.h>
#include <aws/s3/model/GetObjectRequest.h>
#include <aws/s3/model/PutObjectRequest.h>
#include <iostream>
#include <fstream>

static int do_help(IDAM_PLUGIN_INTERFACE* idam_plugin_interface);

static int do_version(IDAM_PLUGIN_INTERFACE* idam_plugin_interface);

static int do_builddate(IDAM_PLUGIN_INTERFACE* idam_plugin_interface);

static int do_defaultmethod(IDAM_PLUGIN_INTERFACE* idam_plugin_interface);

static int do_maxinterfaceversion(IDAM_PLUGIN_INTERFACE* idam_plugin_interface);

static int do_get(IDAM_PLUGIN_INTERFACE* idam_plugin_interface);

static int do_put(IDAM_PLUGIN_INTERFACE* idam_plugin_interface);


int s3(IDAM_PLUGIN_INTERFACE* idam_plugin_interface)
{
    idamSetLogLevel(UDA_LOG_DEBUG);

    int err = 0;

    static short init = 0;
    
    unsigned short housekeeping;
    
    Aws::SDKOptions options;

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

        Aws::ShutdownAPI(options);
	
        init = 0;

        if (!isReset) return 0;        // Step to Initialisation
    }

    //----------------------------------------------------------------------------------------
    // Initialise

    if (!STR_IEQUALS(request_block->function, "help") &&
        (!init || STR_IEQUALS(request_block->function, "init") || STR_IEQUALS(request_block->function, "initialise"))) {

        Aws::InitAPI(options);
	options.loggingOptions.logLevel = Aws::Utils::Logging::LogLevel::Info;

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
        
    const Aws::String bucket_name = bucket;
    const Aws::String key_name    = object;

    Aws::S3::S3Client s3_client;
    
    //S3BucketContext bucketContext

    Aws::S3::Model::GetObjectRequest object_request;
    object_request.WithBucket(bucket_name).WithKey(key_name);

    auto get_object_outcome = s3_client.GetObject(object_request);
    
    char *buffer = NULL;
    int count = 0;

    if (get_object_outcome.IsSuccess())
    {
       Aws::OFStream tmp_file;
       char * filename = (char *)malloc((strlen(bucket)+strlen(object)+15)*sizeof(char));
       sprintf(filename, "/tmp/uda/s3/%s_%s", bucket, object);
       tmp_file.open(filename, std::ios::out | std::ios::binary);
       tmp_file << get_object_outcome.GetResult().GetBody().rdbuf();
       count = tmp_file.tellp();
       tmp_file.close();
       
       buffer = (char *)malloc(count*sizeof(char));
       
       FILE *fd = fopen(filename, "rb");	    	  
       fread(buffer, count, sizeof(char), fd);
       fclose(fd);
    }
    else
    {
       UDA_LOG(UDA_LOG_ERROR, "S3::get() No data object found!\n");
       /*UDA_LOG(UDA_LOG_ERROR, "S3: No data object found! [%s] %s\n", 
               get_object_outcome.GetError().GetExceptionName().c_str(), 
               get_object_outcome.GetError().GetMessage().c_str());
       */
       THROW_ERROR(999, "S3::get() No data object found!");
    }    

//data_block->data=buffer;
//sprintf(data_block->data,"S3:get() requested and plugin entered\ncount=%d\n", count);
//data_block->data_type=UDA_TYPE_STRING;
//data_block->data_n=strlen(data_block->data)+1;
//return 0;

    if(buffer != NULL && count > 0){  
    
/*
    char* buffer;
    size_t bufsize = 0;

    FILE* memfile = open_memstream(&buffer, &bufsize);

    fwrite(value, sizeof(char), len, memfile);
    fseek(memfile, 0L, SEEK_SET);

    XDR xdrs;
    xdrstdio_create(&xdrs, memfile, XDR_DECODE);

    DATA_BLOCK* data_block = (DATA_BLOCK*)malloc(sizeof(DATA_BLOCK));
    initDataBlock(data_block);

    int token;
    protocol2(&xdrs, PROTOCOL_DATA_BLOCK, XDR_RECEIVE, &token, logmalloclist, userdefinedtypelist, (void*)data_block);

    xdr_destroy(&xdrs);     // Destroy before the  file otherwise a segmentation error occurs
    fclose(memfile);
    free(key);

*/      
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
/*    
    REQUEST_BLOCK* request_block = idam_plugin_interface->request_block;

    DATA_BLOCK* data_block = idam_plugin_interface->data_block;
    initDataBlock(data_block);
    
    const char* object = NULL, * bucket = NULL;

    bool isObject = 0, isBucket = 0;

    isObject = findStringValue(&request_block->nameValueList, &object, "object");   
    isBucket = findStringValue(&request_block->nameValueList, &bucket, "bucket"); 

    if (!isObject || !isBucket) {
        THROW_ERROR(999, "S3::put() No data object name or S3 bucket has been specified!");
    }
    
    // Serialise the data_block    
    S3_OBJECT s3_object;
    
    char* buffer = NULL;
    size_t bufsize = 0;

    FILE* memfile = open_memstream(&buffer, &bufsize);

    XDR xdrs;
    xdrstdio_create(&xdrs, memfile, XDR_ENCODE);

    int token;

    //protocol2(&xdrs, PROTOCOL_DATA_BLOCK, XDR_SEND, &token, idam_plugin_interface->logmalloclist, 
    //          idam_plugin_interface->userdefinedtypelist, (void*)idam_plugin_interface->data_block);    
    
    protocol2(&xdrs, PROTOCOL_PUTDATA_BLOCK_LIST, XDR_SEND, &token, idam_plugin_interface->logmalloclist, 
              idam_plugin_interface->userdefinedtypelist, (void*)idam_plugin_interface->request_block->putDataBlockList);
    
    
    xdr_destroy(&xdrs);     // Destroy before the file otherwise a segmentation error occurs
    fclose(memfile);

    // Write the serialised data to tmp 
    char * filename = (char *)malloc((strlen(bucket)+strlen(object)+15)*sizeof(char));
    sprintf(filename, "/tmp/uda/s3/%s_%s", bucket, object);
    FILE *fd = fopen(filename, "wb");	    	  
    fwrite(buffer, bufsize, sizeof(char), fd);
    fclose(fd);
    
    // Write serialised data to AWS S3	    
            
    const Aws::String bucket_name = bucket;
    const Aws::String key_name    = object;
    const Aws::String region      = "";

    Aws::Client::ClientConfiguration clientConfig;
    if (!region.empty())
        clientConfig.region = region;
    Aws::S3::S3Client s3_client(clientConfig);

    Aws::S3::Model::PutObjectRequest object_request;
    object_request.WithBucket(bucket).WithKey(object);

    // Binary files must also have the std::ios_base::bin flag or'ed in
    auto input_data = Aws::MakeShared<Aws::FStream>("PutObjectInputStream",
         file_name, std::ios_base::in | std::ios_base::binary);

    object_request.SetBody(input_data);
    
    auto put_object_outcome = s3_client.PutObject(object_request);

    if (put_object_outcome.IsSuccess())
    {
        UDA_LOG(UDA_LOG_ERROR, "S3::put() Object put successfully!\n");
    }
    else
    {
        UDA_LOG(UDA_LOG_ERROR, "S3::put() Object put failed!\n");
	UDA_LOG(UDA_LOG_ERROR, "S3::put() %s\n", put_object_outcome.GetError().GetExceptionName().c_str());
	UDA_LOG(UDA_LOG_ERROR, "S3::put() %s\n", put_object_outcome.GetError().GetMessage().c_str());
        THROW_ERROR(999, "S3::put() Object put failed!!!");
    }    

    data_block->rank = 0;
    data_block->order = -1;
    data_block->data_type = UDA_TYPE_INT;
    data_block->dims = NULL;  
    data_block->data_n = 1;
    int *data = (int *)malloc(sizeof(int));
    data[0] = bufsize;    
    data_block->data = (char *)data;          
*/    
    return 0;
}
