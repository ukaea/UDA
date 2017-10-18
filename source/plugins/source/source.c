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
#include "source.h"

#include <strings.h>

#include <client/accAPI.h>
#include <structures/struct.h>
#include <clientserver/initStructs.h>
#include <clientserver/stringUtils.h>
#include <clientserver/xmlStructs.h>

static int do_help(IDAM_PLUGIN_INTERFACE* idam_plugin_interface);

static int do_version(IDAM_PLUGIN_INTERFACE* idam_plugin_interface);

static int do_builddate(IDAM_PLUGIN_INTERFACE* idam_plugin_interface);

static int do_defaultmethod(IDAM_PLUGIN_INTERFACE* idam_plugin_interface);

static int do_maxinterfaceversion(IDAM_PLUGIN_INTERFACE* idam_plugin_interface);

static int do_get(IDAM_PLUGIN_INTERFACE* idam_plugin_interface, int *time_count_cache, char **time_cache, char **data_cache);


int source_plugin(IDAM_PLUGIN_INTERFACE* idam_plugin_interface)
{
    idamSetLogLevel(UDA_LOG_DEBUG);

    int err = 0;

    static short init = 0;
    
    static int time_count_cache = 0;
    static char *time_cache = NULL; 
    static char *data_cache = NULL; 

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

        if(time_count_cache != 0){
            time_count_cache = 0;
            if(time_cache != NULL)free((void *)time_cache);
	    if(data_cache != NULL)free((void *)data_cache);
            time_cache = NULL;
            data_cache = NULL;
        } 


        init = 0;

        if (!isReset) return 0;        // Step to Initialisation
    }

    //----------------------------------------------------------------------------------------
    // Initialise

    if (!STR_IEQUALS(request_block->function, "help")  &&
            (!init || STR_IEQUALS(request_block->function, "init") || STR_IEQUALS(request_block->function, "initialise"))) {

        time_count_cache = 0;
        time_cache = NULL;
        data_cache = NULL;

        init = 1;
        if (!strcasecmp(request_block->function, "init") || !strcasecmp(request_block->function, "initialise")) {
            return 0;
        }
    }

    //----------------------------------------------------------------------------------------
    // Plugin Functions
    //----------------------------------------------------------------------------------------

    if (!strcasecmp(request_block->function, "get")) {
        err = do_get(idam_plugin_interface, &time_count_cache, &time_cache, &data_cache);
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
    char* help = "\nsource: Add Functions Names, Syntax, and Descriptions\n\n";
    const char* desc = "source: help = description of this plugin";

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
// Create the Data Source argument for the UDA API
// Use Case: When there are no data_source records in the IDAM metadata catalogue, e.g. JET

// SOURCE::get(signal=signal, format=[ppf|jpf|mast|mds] [,source=source] [,shotNumber=shotNumber] [,pass=pass] [,owner=owner] 
//             [,datascaling=datascaling] [,timescaling=timescaling]
//             [,/data] [,/time] [,/NoCacheData] [,/NoCacheTime]
//             [,host=host] [,port=port])

// keywords:	/data	return the data only
//		/time	return the time only
//		/NoCacheTime	Don't cache the time coordinate data after a request for the "data" using the /data keyword. Cacheing is the default with the cache cleared after time data are returned.
//		/NoCacheData	Don't cache the measurement data after a request for the "data" using the /data keyword. Cacheing is the default with the cache cleared after time data are returned.


static int do_get(IDAM_PLUGIN_INTERFACE* idam_plugin_interface, int *timeCountCache, char **timeCache, char **dataCache)
{
    int i;
    
    DATA_BLOCK* data_block = idam_plugin_interface->data_block;
    initDataBlock(data_block);

    REQUEST_BLOCK* request_block = idam_plugin_interface->request_block;
    
    int time_count_cache = *timeCountCache;
    char *time_cache = *timeCache;
    char *data_cache = *dataCache;

         static char time_units_cache[STRING_LENGTH];
         static char time_label_cache[STRING_LENGTH];
         static char signal_cache[STRING_LENGTH];	
         static char source_cache[STRING_LENGTH];
	 
	 char api_signal[STRING_LENGTH];
	 char api_source[STRING_LENGTH];

         double dataScaling=1.0, timeScaling=1.0;
	 int shotNumber=0, runNumber=-1, port=0;
	 char *signal=NULL, *source=NULL, *owner=NULL, *format=NULL, *host=NULL;
	 	 
	 bool isSignal=0, isShotNumber=0, isRunNumber=0, isSource=0, isFormat=0, isOwner=0, isHost=0, isPort, 
	      isData=0, isTime=0, isNoCacheTime=0, isDataScaling=0, isTimeScaling=0;
	 
	 isData = findValue(&idam_plugin_interface->request_block->nameValueList, "data");   // Only return the data - ignore the coordinates (so do nothing as already in the DATA_BLOCK)
	 isTime = findValue(&idam_plugin_interface->request_block->nameValueList, "time");   // Only return the time - ignore the data
         
	 isNoCacheTime = findValue(&idam_plugin_interface->request_block->nameValueList, "nocacheTime");  // Do Not Cache the time data in preparation for a Time request. Default is to Cache with cache cleared after time data returned.
	 
	 //isNoCacheData = findValue(&idam_plugin_interface->request_block->nameValueList, "nocacheData");  
	 	 
	 
	 for(i=0;i<request_block->nameValueList.pairCount;i++){ 
	    if(!strcasecmp(request_block->nameValueList.nameValue[i].name, "signal")){	 
	       isSignal = 1;
               signal = request_block->nameValueList.nameValue[i].value;
	       continue;
	    }
	    if(!strcasecmp(request_block->nameValueList.nameValue[i].name, "source")){
	       isSource = 1;
	       source = request_block->nameValueList.nameValue[i].value;
	       continue;
	    }	    
	    if(!strcasecmp(request_block->nameValueList.nameValue[i].name, "format") || !strcasecmp(request_block->nameValueList.nameValue[i].name, "pattern")){
	       isFormat = 1;
	       format = request_block->nameValueList.nameValue[i].value;
	       continue;
	    }	    
	    if(!strcasecmp(request_block->nameValueList.nameValue[i].name, "owner")){
	       isOwner = 1;
	       owner = request_block->nameValueList.nameValue[i].value;
	       continue;
	    }	    

	    if(!strcasecmp(request_block->nameValueList.nameValue[i].name, "shotNumber") || !strcasecmp(request_block->nameValueList.nameValue[i].name, "shot") ||
	       !strcasecmp(request_block->nameValueList.nameValue[i].name, "pulse")      || !strcasecmp(request_block->nameValueList.nameValue[i].name, "exp_number")){
	       isShotNumber = 1;
	       shotNumber = atoi(request_block->nameValueList.nameValue[i].value);	
	       continue;
	    }
            if(!strcasecmp(request_block->nameValueList.nameValue[i].name, "runNumber") || !strcasecmp(request_block->nameValueList.nameValue[i].name, "run") ||
	       !strcasecmp(request_block->nameValueList.nameValue[i].name, "pass") || !strcasecmp(request_block->nameValueList.nameValue[i].name, "sequence")){
	       isRunNumber = 1;
	       runNumber = atoi(request_block->nameValueList.nameValue[i].value);	
	       continue;
	    }
 
	    if(!strcasecmp(request_block->nameValueList.nameValue[i].name, "dataScaling")){	// Measurement data need to scaled to comply with standards, e.g. SI Units
	       isDataScaling = 1;
               dataScaling = atof(request_block->nameValueList.nameValue[i].value);
	       continue;
	    } 
	    if(!strcasecmp(request_block->nameValueList.nameValue[i].name, "timeScaling")){	// Measurement times need to scaled to comply with standards, e.g. SI Units
	       isTimeScaling = 1;
               timeScaling = atof(request_block->nameValueList.nameValue[i].value);
	       continue;
	    } 
	    	    
	    if(!strcasecmp(request_block->nameValueList.nameValue[i].name, "host")){
	       isHost = 1;
	       host = request_block->nameValueList.nameValue[i].value;	
	       continue;
	    }
	    if(!strcasecmp(request_block->nameValueList.nameValue[i].name, "port")){
	       isPort = 1;
	       port = atoi(request_block->nameValueList.nameValue[i].value);	
	       continue;
	    }
 
	 }
	      
         if(!isSignal){
            THROW_ERROR(999, "source: No data object name (signal) has been specified!");
         }

// Prepare common code
	 
         char *env = NULL;
	 char work[MAXMETA];
	 
	 IDAM_PLUGIN_INTERFACE next_plugin_interface;
         REQUEST_BLOCK next_request_block;
	    
         PLUGINLIST *pluginList = (PLUGINLIST *)idam_plugin_interface->pluginList;	// List of all data reader plugins (internal and external shared libraries)	 	 	 

         if(pluginList == NULL){
            THROW_ERROR(999, "source: No plugins are available for this data request!");
         }	 	 	 
	 
         next_plugin_interface = *idam_plugin_interface;		// New plugin interface
	    	    
         next_plugin_interface.request_block = &next_request_block;	    
         initServerRequestBlock(&next_request_block);
         strcpy(next_request_block.api_delim, request_block->api_delim);
	    
         strcpy(next_request_block.signal, signal);			// Prepare the API arguments	 
	 if(!isShotNumber) shotNumber = request_block->exp_number;

      
// JET PPF sources: PPF::/$ppfname/$pulseNumber/$sequence/$owner 

         if(isFormat && !strcasecmp(format, "ppf")){			// JET PPF source naming pattern
     
           if(!isSource){
               THROW_ERROR(999, "source: No PPF DDA data source has been specified!");
           }

	    env = getenv("UDA_JET_DEVICE_ALIAS");

	    if(env == NULL)
	       sprintf(next_request_block.source, "PPF%s/%s/%d", request_block->api_delim, source, shotNumber);
	    else   
	       sprintf(next_request_block.source, "%s%sPPF%s/%s/%d", env, request_block->api_delim, request_block->api_delim, source, shotNumber);

	    if(isRunNumber)
	       sprintf(next_request_block.source, "%s/%d", next_request_block.source, runNumber);
	    else
	       sprintf(next_request_block.source, "%s/0", next_request_block.source);   

	    if(isOwner)
	       sprintf(next_request_block.source, "%s/%s", next_request_block.source, owner);

	 } else 
	 
         if(isFormat && !strcasecmp(format, "jpf")){		// JET JPF source naming pattern        	 

	    env = getenv("UDA_JET_DEVICE_ALIAS");

	    if(env == NULL)
	       sprintf(next_request_block.source, "JPF%s%d", request_block->api_delim, shotNumber);
	    else
	       sprintf(next_request_block.source, "%s%sJPF%s%d", env, request_block->api_delim, request_block->api_delim, shotNumber);
         
UDA_LOG(UDA_LOG_DEBUG, "SOURCE:signal #JPF = %s\n", next_request_block.signal);
UDA_LOG(UDA_LOG_DEBUG, "SOURCE:source #JPF = %s\n", next_request_block.source);        
	 	 
	 } else
	  
	 
	 if(isFormat && !strcasecmp(format, "MAST")){		// MAST source naming pattern
	    
            env = getenv("UDA_MAST_DEVICE_ALIAS");
	    
	    if(!isShotNumber && !isRunNumber)                   // Reuse the orignal source
	       if(env == NULL)
	          strcpy(next_request_block.source, request_block->source);
	       else	  		
	          sprintf(next_request_block.source, "%s%s%s", env, request_block->api_delim, request_block->source);       
	    else {
	       if(env == NULL)
	          sprintf(next_request_block.source, "%d", shotNumber);
	       else
	          sprintf(next_request_block.source, "%s%s%d", env, request_block->api_delim, shotNumber);
	       if(isRunNumber) sprintf(next_request_block.source, "%s/%d", next_request_block.source, runNumber);
            }  	    	    

UDA_LOG(UDA_LOG_DEBUG, "SOURCE:signal #MAST = %s\n", next_request_block.signal);
UDA_LOG(UDA_LOG_DEBUG, "SOURCE:source #MAST = %s\n", next_request_block.source);        
	 	 
	 } else  
	  
         if(isFormat && (!strcasecmp(format, "mds") || !strcasecmp(format, "mdsplus") || !strcasecmp(format, "mds+"))){	     // MDS+ source naming pattern
    	    
	    if(!isHost){
               THROW_ERROR(999, "source: No MDSPLUS data server hostname has been specified!");
            }
            
	    env = getenv("UDA_MDSPLUS_ALIAS");
	    
	    if(isSource){	// TDI function or tree?
	       if(env == NULL)
	          sprintf(next_request_block.source, "MDSPLUS%s%s/%s/%d", request_block->api_delim, host, source, shotNumber);
	       else
	          sprintf(next_request_block.source, "%s%s%s/%s/%d", env, request_block->api_delim, host, source, shotNumber);
            } else {	       
	       if(env == NULL)
	          sprintf(next_request_block.source, "MDSPLUS%s%s", request_block->api_delim, host);
	       else
	          sprintf(next_request_block.source, "%s%s%s", env, request_block->api_delim, host);
	       char *p=NULL;
	       if((p=strstr(next_request_block.signal, "$pulseNumber")) != NULL){
	          p[0] = '\0';
		  sprintf(p, "%d%s", shotNumber, &p[12]);   	           
	       } 
	    }
	 } else {	  
            THROW_ERROR(999, "source: the specified data format is not recognised!");
         }

// Create the Request data structure

         env = getenv("UDA_UDA_PLUGIN");

	 if(env != NULL){
	    if(isHost){
	      if(isPort) 
	         sprintf(work, "%s::get(host=%s, port=%d, signal=\"%s\", source=\"%s\")", env, host, port, next_request_block.signal, next_request_block.source);
	      else
	         sprintf(work, "%s::get(host=%s, port=%d, signal=\"%s\", source=\"%s\")", env, host, getIdamServerPort(), next_request_block.signal, next_request_block.source);
	    } else {
	      if(isPort)
	         sprintf(work, "%s::get(host=%s, port=%d, signal=\"%s\", source=\"%s\")", env, getIdamServerHost(), port, next_request_block.signal, next_request_block.source); 
	      else
	         sprintf(work, "%s::get(host=%s, port=%d, signal=\"%s\", source=\"%s\")", env, getIdamServerHost(), getIdamServerPort(), next_request_block.signal, next_request_block.source);
	    }  	      
         } else{
	    if(isHost){
	      if(isPort) 
	         sprintf(work, "UDA::get(host=%s, port=%d, signal=\"%s\", source=\"%s\")", host, port, next_request_block.signal, next_request_block.source);
	      else
	         sprintf(work, "UDA::get(host=%s, port=%d, signal=\"%s\", source=\"%s\")", host, getIdamServerPort(), next_request_block.signal, next_request_block.source);
	    } else {
	      if(isPort)
	         sprintf(work, "UDA::get(host=%s, port=%d, signal=\"%s\", source=\"%s\")", getIdamServerHost(), port, next_request_block.signal, next_request_block.source); 
	      else
	         sprintf(work, "UDA::get(host=%s, port=%d, signal=\"%s\", source=\"%s\")", getIdamServerHost(), getIdamServerPort(), next_request_block.signal, next_request_block.source);
	    }  	      
 	 }

	 next_request_block.source[0] = '\0';
	 strcpy(next_request_block.signal, work);  

UDA_LOG(UDA_LOG_DEBUG, "SOURCE:signal #1 = %s\n", next_request_block.signal);	 
UDA_LOG(UDA_LOG_DEBUG, "SOURCE:source #1 = %s\n", next_request_block.source);	  
	 
	 makeServerRequestBlock(&next_request_block, *pluginList); 
	 
	 strcpy(api_signal, next_request_block.signal);			// These are what are used to access data - retain as cache keys
	 strcpy(api_source, next_request_block.source);	
	 
// Call the IDAM client via the IDAM plugin (ignore the request identified)

	 if(env != NULL)
	    next_request_block.request = findPluginRequestByFormat(env, pluginList);
	 else                
	    next_request_block.request = findPluginRequestByFormat("UDA", pluginList);

 	 if(next_request_block.request < 0){
            THROW_ERROR(999, "source: No UDA server plugin found!");
         }

// is Time requested and is the data cached? Does the IDS time entity name match the cached data entity name
// Caching is the default behaviour
// If data are cached then skip the plugin request for data - use the cached data
         
         int skipPlugin = isTime && !isNoCacheTime && time_count_cache > 0 && !strcasecmp(signal_cache, api_signal) && !strcasecmp(source_cache, api_source);

// Locate and Execute the IDAM plugin  

         if(!skipPlugin){	 	 	 	    
            int id = findPluginIdByRequest(next_request_block.request, pluginList);
            if(id >= 0 && pluginList->plugin[id].idamPlugin != NULL){
               int err = pluginList->plugin[id].idamPlugin(&next_plugin_interface);		// Call the data reader
               if(err != 0){             
                  THROW_ERROR(999, "Source: Data Access is not available!");
               }
            } else {
               THROW_ERROR(999, "Source: Data Access is not available for this data request!");
            }

	 }
	    
         freeNameValueList(&next_request_block.nameValueList);
	 
// Return data is automatic since both next_request_block and request_block point to the same DATA_BLOCK etc.
// IMAS data must be DOUBLE
// Time Data are only cacheable if the data are rank 1 with time data! 

         if(isData){		// Ignore the coordinate data.

             if(data_block->order != 0 || data_block->rank != 1 || !(data_block->data_type == UDA_TYPE_FLOAT || data_block->data_type == UDA_TYPE_DOUBLE)){
	        THROW_ERROR(999, "Source: Data Access is not available for this data request!");
	     }		// Data are not Cacheable           

             if(!isNoCacheTime){		// Save the Time Coordinate data to local cache  (free currently cached data if different)           
                
		if(time_cache != NULL) free((void *)time_cache);
                time_cache = NULL;
                time_count_cache = 0;
                strcpy(signal_cache, api_signal);
                strcpy(source_cache, api_source);
                if(data_block->dims[0].compressed) uncompressDim(&data_block->dims[0]);
                time_count_cache = data_block->dims[0].dim_n;
                strcpy(time_units_cache, data_block->dims[0].dim_units);
                strcpy(time_label_cache, data_block->dims[0].dim_label);
		   
                if(data_block->dims[0].data_type == UDA_TYPE_DOUBLE){
		   time_cache = (char *)malloc(time_count_cache*sizeof(double));
                   if(isTimeScaling){
                      double * work = (double *)data_block->dims[0].dim;
		      for(i=0;i<time_count_cache;i++) work[i] = timeScaling * work[i];
                      data_block->dims[0].dim = (char *)work;
                   }		      
                   memcpy(time_cache, data_block->dims[0].dim, time_count_cache*sizeof(double)); 
		} else {
		   float * data =  (float *) data_block->dims[0].dim;
		   double * work = (double *)malloc(time_count_cache*sizeof(double));
                   if(isTimeScaling)
		      for(i=0;i<time_count_cache;i++) work[i] = timeScaling * (double) data[i];
                   else
                      for(i=0;i<time_count_cache;i++) work[i] = (double) data[i];
		   time_cache = (char *) work;
		}   
             } else {   // End of Time cache
                if(time_cache != NULL) free((void *)time_cache);	// Clear the cache
		if(data_cache != NULL) free((void *)data_cache);
                time_cache = NULL;
		data_cache = NULL;
                time_count_cache = 0;
                signal_cache[0] = '\0';
                source_cache[0] = '\0';	     
	     }   
	     
             if (data_block->rank == 1 && (data_block->data_type == UDA_TYPE_FLOAT || data_block->data_type == UDA_TYPE_DOUBLE)){
 	        
		data_block->rank  = 0;		// No coordinate data to be returned
	        data_block->order = -1;	
		
		if(data_block->data_type == UDA_TYPE_FLOAT){		      
		   float * data =  (float *) data_block->data;
		   double * work = (double *)malloc(data_block->data_n*sizeof(double));
		   if(isDataScaling)
		      for(i=0;i<data_block->data_n;i++) work[i] = dataScaling * (double) data[i];
		   else 
		      for(i=0;i<data_block->data_n;i++) work[i] = (double) data[i];  
		   free((void *) data_block->data);
		   data_block->data = (char *) work;
		   data_block->data_type = UDA_TYPE_DOUBLE;
		} else {
		   if(isDataScaling){
		      double * data =  (double *) data_block->data;
		      for(i=0;i<data_block->data_n;i++) data[i] = dataScaling * data[i];
		   }
		}   		
		 
	        if(data_block->dims[0].dim != NULL) free((void *)data_block->dims[0].dim);
	        data_block->dims[0].dim = NULL;		// prevent a double free
 	        data_block->dims[0].dim_n = 0;
             } else {
	        THROW_ERROR(999, "Source: Data Access is not available for this data request!");
	     }  
         }
	 
// For efficiency, local client cache should also be running
	 
         if(isTime){		// The time data are in the coordinate array indicated by 'order' value. The data must be rank 1. Data may be compressed           
            if(!isNoCacheTime && time_count_cache > 0 &&
               !strcasecmp(signal_cache, api_signal) && !strcasecmp(source_cache, api_source)){		// Retrieve the Time Coordinate data from the local cache after verification of IDS names           
 	        data_block->rank  = 0;
	        data_block->order = -1;	 
		data_block->data  = (char *)malloc(time_count_cache*sizeof(double));
		memcpy(data_block->data, time_cache, time_count_cache*sizeof(double));
		data_block->data_n = time_count_cache;
		data_block->data_type = UDA_TYPE_DOUBLE;
	        data_block->dims = NULL;		
	        strcpy(data_block->data_units, time_units_cache);
	        strcpy(data_block->data_label, time_label_cache); 
            } else
            if (data_block->rank == 1 && data_block->order == 0  && (data_block->data_type == UDA_TYPE_FLOAT || data_block->data_type == UDA_TYPE_DOUBLE)){
	        if(data_block->dims[0].compressed) uncompressDim(&data_block->dims[0]);
 	        data_block->rank  = 0;
	        data_block->order = -1;	 
	        if(data_block->data != NULL) free((void *)data_block->data);
		
		if (data_block->dims[0].data_type == UDA_TYPE_DOUBLE){
		   data_block->data = data_block->dims[0].dim;		   
		   if(isTimeScaling){
		      double * work = (double *)data_block->data;
		      for(i=0;i<data_block->dims[0].dim_n;i++) work[i] = timeScaling * work[i];
		   }   
		} else {
		   float * data = (float *) data_block->dims[0].dim;
		   double * work = (double *)malloc(data_block->dims[0].dim_n*sizeof(double));
		   if(isTimeScaling)
		      for(i=0;i<data_block->dims[0].dim_n;i++) work[i] = timeScaling * (double) data[i];
		   else
		      for(i=0;i<data_block->dims[0].dim_n;i++) work[i] = (double) data[i];
		   data_block->data = (char *) work;
		   free((void *)data_block->dims[0].dim);
		}
		data_block->data_n = data_block->dims[0].dim_n;
		data_block->data_type = UDA_TYPE_DOUBLE;
	        data_block->dims[0].dim = NULL;		// prevent a double free
 	        data_block->dims[0].dim_n = 0;
	        strcpy(data_block->data_units, data_block->dims[0].dim_units);
	        strcpy(data_block->data_label, data_block->dims[0].dim_label); 
             } else {
	        THROW_ERROR(999, "Source: Data Access is not available for this data request!");
	     }  
         } 
	 
	 *timeCache = time_cache;
	 *dataCache = data_cache;
	 *timeCountCache = time_count_cache;  	

    return 0;
}
