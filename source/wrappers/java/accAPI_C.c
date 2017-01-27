/*---------------------------------------------------------------
* Accessor Functions
*--------------------------------------------------------------*/

#include "idamclientserverpublic.h"
#include "idamclientserverprivate.h"
#include "idamclientpublic.h"
#include "idamclientprivate.h"

#include <malloc.h>

#ifdef FATCLIENT
#include "idamserver.h"
#endif

#include "idamgenstruct.h"
#ifdef FATCLIENT
#include "idamplugin.h"
#endif

// C Accessor Routines

//---------------------------- Static Globals -------------------------

static int clientVersion = 6;

static int debugon = 0;
static int verbose = 0;

#ifdef ARGSTACK
static FILE *argstack = NULL;		// Log all arguments passed
#endif

static FILE *old_errout = NULL;
static FILE *old_dbgout = NULL;
static int erroutpassed = 0;
static int dbgoutpassed = 0;

static int user_timeout = TIMEOUT;	// user specified Server Lifetime

static unsigned int clientFlags = 0;	// Send properties via bit flags
static int altRank	        = 0;	// Rank of alternative Signal/source (name mapping)

static int get_nodimdata = 0;		// Don't send dimensional data: Send a simple Index
static int get_datadble  = 0;		// Cast the Time Dimension to Double Precision
static int get_dimdble   = 0;
static int get_timedble  = 0;
static int get_bad       = 0;
static int get_meta      = 0;
static int get_asis      = 0;
static int get_uncal     = 0;
static int get_notoff    = 0;
static int get_scalar    = 0;		// return scalar (Rank 0) data if the rank is 1 and the dim data has (have) zero value(s) 
static int get_bytes     = 0;
static int get_synthetic = 0;		// return synthetic Data instead of original data

static unsigned int privateFlags = 0;
static unsigned int XDRstdioFlag = 0;

static SOCKETLIST client_socketlist;	// List of open sockets
	
static int Data_Block_Count   = 0;	// Count of Blocks recorded
static DATA_BLOCK *Data_Block = NULL;	// All Data are recorded here!

static int clientSocket = -1;

static time_t tv_server_start = 0;	 
static time_t tv_server_end   = 0; 

static int initEnvironment = 1;		// Flag initilisation
static ENVIRONMENT environment;		// Holds local environment variable values

static int env_host = 1;		// User can change these before startup so flag to the getEnvironment function 
static int env_port = 1;

static CLIENT_BLOCK client_block;  
static SERVER_BLOCK server_block; 

static USERDEFINEDTYPELIST *userdefinedtypelist = NULL;		// List of all known User Defined Structure Types
static LOGMALLOCLIST *logmalloclist = NULL;			// List of all Heap Allocations for Data 

#ifdef FATCLIENT
static PLUGINLIST pluginList;
static USERDEFINEDTYPELIST parseduserdefinedtypelist;		// Input User Defined Structure Types
#endif

//--------------------------------------------------------------  
/* Notes:

Rank Ordering is as follows:

	Order is the Time Dimension from the Left, e.g.,
	
	Rank 2  A[t][x] has Order 0 
		A[x][t] has Order 1 
		
	Rank 3  A[t][x][y] has Order 0 
		A[x][t][y] has Order 1 
		A[x][y][t] has Order 2 

 In IDL and FORTRAN the Time Dimension is counted from the Right, e.g.,  
 
 	Rank 3  A(y,x,t) has Order 0
 
*/
//--------------------------------------------------------------
// Security Certification

//void putIdamSecurityCertificate(const char *cert){ 
//   if(initEnvironment) getIdamClientEnvironment(&environment); 
//   strcpy(environment.security_cert, cert);			// Path to the Client's X.509 Security Certificate
//}

//--------------------------------------------------------------
// Private Flags (Server to Server communication via an IDAM client server plugin)

//! Set a privateFlags property
/** Set a/multiple specific bit/s in the privateFlags property sent between IDAM servers. 
*
* @param flag The bit/s to be set to 1. 
* @return Void.
*/ 
   void setIdamPrivateFlag(unsigned int flag){ 
      privateFlags = privateFlags | flag; 
   }

//! Reset a privateFlags property
/** Reset a/multiple specific bit/s in the privateFlags property sent between IDAM servers. 
*
* @param flag The bit/s to be set to 0. 
* @return Void.
*/ 

   void resetIdamPrivateFlag(unsigned int flag){
      privateFlags = privateFlags & !flag; 
   } 
   
//--------------------------------------------------------------
// Client Flags

//! Set a clientFlags property
/** Set a/multiple specific bit/s in the clientFlags property sent to the IDAM server. 
*
* @param flag The bit/s to be set to 1. 
* @return Void.
*/ 

   void setIdamClientFlag(unsigned int flag){ 
      clientFlags = clientFlags | flag; 
   }

//! Reset a clientFlags property
/** Reset a/multiple specific bit/s in the clientFlags property sent to the IDAM server. 
*
* @param flag The bit/s to be set to 0. 
* @return Void.
*/ 

   void resetIdamClientFlag(unsigned int flag){
      clientFlags = clientFlags & !flag; 
   }      
   
//--------------------------------------------------------------
// Set Server Properties

//! Set a named server property
/** Set a variety of data server properties using their name. These affect the data type returned and any server side processing of data.
* Not all data access plugins respond to these properties.\n
* 
* \eget_datadble	data are returned in double precision.\n
* \eget_dimdble	all coordinate (dimension) data are returned in double precision.\n
* \eget_timedble	the Time coordinate (dimension) data are returned in double precision.\n
* \eget_bytes\n
* \eget_bad\n
* \eget_meta	return all SQL database records used to locate and correct the requested data\n
* \eget_asis	do not apply server side correction to data\n
* \eget_uncal\n
* \eget_notoff	do not apply any timing offset corrections\n
* \eget_synthetic\n
* \eget_scalar\n
* \eget_nodimdata	do not return coordinate (dimension) data\n
* \etimeout=value name value pair to set the number of wait seconds before timing out the server connection\n
* \everbose	\n
* \edebug		create debug output from the client\n
* \ealtData	use efit++ with legacy efm data signal names \n
* \ealtRank	select different efit++ output file as the data source \n  	 
*
* @param property the name of the property to set true or a name value pair. 
* @return Void.
*/ 
void setIdamProperty(const char *property){
 
// User settings for Client and Server behaviour

   char name[56];
   char *value;
   
   if(property[0] == 'g'){
      if(!strcasecmp(property, "get_datadble"))  get_datadble  = 1;
      if(!strcasecmp(property, "get_dimdble"))   get_dimdble   = 1;
      if(!strcasecmp(property, "get_timedble"))  get_timedble  = 1;
      if(!strcasecmp(property, "get_bytes"))     get_bytes     = 1;
      if(!strcasecmp(property, "get_bad"))       get_bad       = 1;
      if(!strcasecmp(property, "get_meta"))      get_meta      = 1;
      if(!strcasecmp(property, "get_asis"))      get_asis      = 1;
      if(!strcasecmp(property, "get_uncal"))     get_uncal     = 1;
      if(!strcasecmp(property, "get_notoff"))    get_notoff    = 1;
      if(!strcasecmp(property, "get_synthetic")) get_synthetic = 1;
      if(!strcasecmp(property, "get_scalar"))    get_scalar    = 1;
      if(!strcasecmp(property, "get_nodimdata")) get_nodimdata = 1;
   } else {
      if(property[0] == 't'){
         strncpy(name,property,55);
	 name[55] = '\0';
	 TrimString(name);
	 LeftTrimString(name);
	 MidTrimString(name);
	 strlwr(name);
	 if((value = strstr(name,"timeout=")) != NULL){
	    value = name+8;
	    if(IsNumber(value)) user_timeout = atoi(value);
	 } 
      } else {
         if(!strcasecmp(property, "verbose"))  verbose  = 1;
         if(!strcasecmp(property, "debug"))    debugon  = 1;
	 if(!strcasecmp(property, "altData"))  clientFlags = clientFlags | CLIENTFLAG_ALTDATA;
	 if(!strncasecmp(property, "altRank",7)){
            strncpy(name,property,55);
	    name[55] = '\0';
	    TrimString(name);
	    LeftTrimString(name);
	    MidTrimString(name);
	    strlwr(name);
	    if((value = strcasestr(name,"altRank=")) != NULL){
	       value = name+8;
	       if(IsNumber(value)) altRank = atoi(value);
	    } 
         }
      }	 
   }  
   return ;
} 

//! Return the value of a named server property
/** 
* @param property the name of the property.
* @return Void.
*/ 
int getIdamProperty(const char *property){
 
// User settings for Client and Server behaviour
   
   if(property[0] == 'g'){
      if(!strcasecmp(property, "get_datadble"))  return(get_datadble);
      if(!strcasecmp(property, "get_dimdble"))   return(get_dimdble);
      if(!strcasecmp(property, "get_timedble"))  return(get_timedble);
      if(!strcasecmp(property, "get_bytes"))     return(get_bytes);
      if(!strcasecmp(property, "get_bad"))       return(get_bad);
      if(!strcasecmp(property, "get_meta"))      return(get_meta);
      if(!strcasecmp(property, "get_asis"))      return(get_asis);
      if(!strcasecmp(property, "get_uncal"))     return(get_uncal);
      if(!strcasecmp(property, "get_notoff"))    return(get_notoff);
      if(!strcasecmp(property, "get_synthetic")) return(get_synthetic);
      if(!strcasecmp(property, "get_scalar"))    return(get_scalar);
      if(!strcasecmp(property, "get_nodimdata")) return(get_nodimdata);      
   } else {
      if(!strcasecmp(property, "timeout"))  return(user_timeout);
      if(!strcasecmp(property, "verbose"))  return(verbose);
      if(!strcasecmp(property, "debug"))    return(debugon);
      if(!strcasecmp(property, "altData"))  return(clientFlags);
      if(!strcasecmp(property, "altRank"))  return(altRank);
   }  
   return(0) ;
} 

//! Reset a specific named data server property to its default value
/**
* @param property the name of the property.
* @return Void.
*/ 

void resetIdamProperty(const char *property){
//
// User settings for Client and Server behaviour

   //char name[56];
   //char *value;
   
   if(property[0] == 'g'){
      if(!strcasecmp(property, "get_datadble"))  get_datadble  = 0;
      if(!strcasecmp(property, "get_dimdble"))   get_dimdble   = 0;
      if(!strcasecmp(property, "get_timedble"))  get_timedble  = 0;
      if(!strcasecmp(property, "get_bytes"))     get_bytes     = 0;
      if(!strcasecmp(property, "get_bad"))       get_bad       = 0;
      if(!strcasecmp(property, "get_meta"))      get_meta      = 0;
      if(!strcasecmp(property, "get_asis"))      get_asis      = 0;
      if(!strcasecmp(property, "get_uncal"))     get_uncal     = 0;
      if(!strcasecmp(property, "get_notoff"))    get_notoff    = 0;
      if(!strcasecmp(property, "get_synthetic")) get_synthetic = 0;
      if(!strcasecmp(property, "get_scalar"))    get_scalar    = 0;
      if(!strcasecmp(property, "get_nodimdata")) get_nodimdata = 0;
   } else {
      if(!strcasecmp(property, "verbose"))       verbose = 0;
      if(!strcasecmp(property, "debug"))         debugon = 0;
      if(!strcasecmp(property, "altData"))       clientFlags = clientFlags & !CLIENTFLAG_ALTDATA;
      if(!strcasecmp(property, "altRank"))       altRank = 0;
   }  
   return ;
} 

//! Reset all data server properties to their default values
/**
* @return Void.
*/ 
void resetIdamProperties(){

// Reset on Both Client and Server

   get_datadble  = 0;
   get_dimdble   = 0;
   get_timedble  = 0;	 
   get_bad       = 0;
   get_meta      = 0;
   get_asis      = 0;
   get_uncal     = 0;
   get_notoff    = 0;
   get_synthetic = 0;
   get_scalar    = 0;
   get_bytes     = 0;
   get_nodimdata = 0;
   verbose       = 0;
   debugon       = 0;
   user_timeout  = TIMEOUT; 
   clientFlags   = clientFlags & !CLIENTFLAG_ALTDATA;
   altRank       = 0; 
   return;
}

CLIENT_BLOCK saveIdamProperties(){		// save current state of properties for future rollback
   CLIENT_BLOCK cb  = client_block; 		// Copy of Global Structure (maybe not initialised! i.e. idam API not called)
   cb.get_datadble  = get_datadble;		// Copy individual properties only
   cb.get_dimdble   = get_dimdble;
   cb.get_timedble  = get_timedble;
   cb.get_bad       = get_bad;
   cb.get_meta      = get_meta;
   cb.get_asis      = get_asis;
   cb.get_uncal     = get_uncal;
   cb.get_notoff    = get_notoff;
   //cb.get_synthetic = get_synthetic;		// Local to Client Only: not a server side property
   cb.get_scalar    = get_scalar;
   cb.get_bytes     = get_bytes;
   cb.get_nodimdata = get_nodimdata;
   cb.clientFlags   = clientFlags;
   cb.altRank       = altRank; 
   return(cb);	
} 

void restoreIdamProperties(CLIENT_BLOCK cb){			// Restore Properties to a prior saved state
   client_block.get_datadble  = cb.get_datadble;		// Overwrite Individual Global Structure Components
   client_block.get_dimdble   = cb.get_dimdble;
   client_block.get_timedble  = cb.get_timedble;
   client_block.get_bad       = cb.get_bad;
   client_block.get_meta      = cb.get_meta;
   client_block.get_asis      = cb.get_asis;
   client_block.get_uncal     = cb.get_uncal;
   client_block.get_notoff    = cb.get_notoff;
   //client_block.get_synthetic = cb.get_synthetic;
   client_block.get_scalar    = cb.get_scalar;
   client_block.get_bytes     = cb.get_bytes;
   client_block.clientFlags   = cb.clientFlags;
   client_block.altRank       = cb.altRank;
   
   get_datadble  = client_block.get_datadble;
   get_dimdble   = client_block.get_dimdble;
   get_timedble  = client_block.get_timedble;	 
   get_bad       = client_block.get_bad;
   get_meta      = client_block.get_meta;
   get_asis      = client_block.get_asis;
   get_uncal     = client_block.get_uncal;
   get_notoff    = client_block.get_notoff;
   //get_synthetic = client_block.get_synthetic;
   get_scalar    = client_block.get_scalar;
   get_bytes     = client_block.get_bytes;
   get_nodimdata = client_block.get_nodimdata;
   clientFlags   = client_block.clientFlags;
   altRank       = client_block.altRank; 

} 

//! Return the client state associated with a specific data item
/** The client state information is at the time the data was accessed.
* @return CLIENT_BLOCK pointer to the data structure.
*/ 
CLIENT_BLOCK *getIdamProperties(int handle){
   if(handle < 0 || handle >= Data_Block_Count) return NULL;
   return(&Data_Block[handle].client_block);	
}      

//! Return the client state associated with a specific data item
/** The client state information is at the time the data was accessed.
* @return CLIENT_BLOCK pointer to the data structure.
*/ 
CLIENT_BLOCK *getIdamDataProperties(int handle){return(getIdamProperties(handle));}


//--------------------------------------------------------------
// Pass Server IO File handles when IDAM client is a Server Plugin (call following a Properties Reset) 

//! Pass the Server's error log file handle to the Server's IDAM client plugin
/** When the IDAM client is a server plugin, set the Client's Error File handle to that of the Server.
* @return void
*/ 
void putIdamErrorFileHandle(FILE *fh){		// Ensures globals have the same value

// Change globals and switch on error output

   old_errout   = errout;	// Preserve the FD for a reset prior to closing the client
   erroutpassed = 1;		// Flag the switch
   errout       = fh;
   verbose      = 1; 
   reopen_logs  = 0; 
    
}

//! Pass the Server's debug output file handle to the Server's IDAM client plugin
/** When the IDAM client is a server plugin, set the Client's Debug File handle to that of the Server.
* @return void
*/ 		 
void putIdamDebugFileHandle(FILE *fh){

// Change globals and switch on debug output

   old_dbgout   = dbgout;	// Preserve the FD for a reset prior to closing the client
   dbgoutpassed = 1;		// Flag the switch
   dbgout       = fh;
   debugon      = 1; 
   reopen_logs  = 0;  
}

 
//--------------------------------------------------------------
//! Test for amount of Free heap memory and current usage
/** When the IDAM client is a server plugin, set the Client's Debug File handle to that of the Server.
* @return void
*/ 		 
int getIdamMemoryFree(){
#ifdef A64
   return 0;
   //struct mstats stats = mstats();
   //return (int) stats.bytes_free;
#else
   struct mallinfo stats = mallinfo();
   return (int) stats.fordblks;
#endif   
}
int getIdamMemoryUsed(){
#ifdef A64 
   return 0;
   //mstats stats = mstats();
   //return (int) stats.bytes_used;
#else
   struct mallinfo stats = mallinfo();
   return (int) stats.uordblks;
#endif
}
 

//--------------------------------------------------------------
// Standard C PUT Accessor Routines

void putIdamErrorModel(int handle, int model, int param_n, float *params){
   int i;
   if(handle < 0 || handle >= Data_Block_Count) return;				// No Data Block available
   if(model <= ERROR_MODEL_UNKNOWN || model >= ERROR_MODEL_UNDEFINED) return; 	// No valid Model 
   
   Data_Block[handle].error_model   = model;					// Model ID
   Data_Block[handle].error_param_n = param_n;					// Number of parameters
   
   if(param_n > MAXERRPARAMS) Data_Block[handle].error_param_n = MAXERRPARAMS;
   for(i=0;i<Data_Block[handle].error_param_n;i++) Data_Block[handle].errparams[i] = params[i];
   
/*   
   if(Data_Block[handle].errparams == NULL){
      Data_Block[handle].errparams = (float *)malloc(Data_Block[handle].error_param_n*sizeof(float));	// Allocate   
   } else {
      realloc(Data_Block[handle].errparams, Data_Block[handle].error_param_n*sizeof(float));		// Change Shape
   }
   memcpy(Data_Block[handle].errparams, params, Data_Block[handle].error_param_n*sizeof(float)); 	// Copy Array of Model Parameters
*/   
}

void putIdamDimErrorModel(int handle, int ndim, int model, int param_n, float *params){
   int i;
   if(handle < 0 || handle >= Data_Block_Count) return;						// No Data Block available
   if(ndim < 0 || ndim >= Data_Block[handle].rank) return;					// No Dim
   if(model <= ERROR_MODEL_UNKNOWN || model >= ERROR_MODEL_UNDEFINED) return; 			// No valid Model 
   
   Data_Block[handle].dims[ndim].error_model   = model;						// Model ID
   Data_Block[handle].dims[ndim].error_param_n = param_n;					// Number of parameters
   
   if(param_n > MAXERRPARAMS) Data_Block[handle].dims[ndim].error_param_n = MAXERRPARAMS;  
   for(i=0;i<Data_Block[handle].dims[ndim].error_param_n;i++) Data_Block[handle].dims[ndim].errparams[i] = params[i];

/*   
   if(Data_Block[handle].dims[ndim].errparams == NULL){
      Data_Block[handle].dims[ndim].errparams = (float *)malloc(Data_Block[handle].dims[ndim].error_param_n*sizeof(float));	// Allocate   
   } else {
      realloc(Data_Block[handle].dims[ndim].errparams, Data_Block[handle].dims[ndim].error_param_n*sizeof(float));		// Change Shape
   }
   memcpy(Data_Block[handle].dims[ndim].errparams, params, Data_Block[handle].dims[ndim].error_param_n*sizeof(float)); 		// Copy Array of Model Parameters
*/   
}

//! Set the IDAM data server host name and port number
/** This takes precedence over the environment variables IDAM_HOST and IDAM_PORT.
* @param host The name of the server host computer.
* @param port The port number the server is connected to.
* @return void
*/
void putIdamServer(const char *host, int port){			 
   if(initEnvironment) getIdamClientEnvironment(&environment);	// Initialise 
   environment.server_port = port;				// IDAM server service port number 
   strcpy(environment.server_host, host);			// IDAM server's host name or IP address
   environment.server_reconnect = 1;				// Create a new Server instance 
   env_host = 0;						// Skip initialsisation at Startup if these are called first
   env_port = 0;     
}

//! Set the IDAM data server host name
/** This takes precedence over the environment variables IDAM_HOST.
* @param host The name of the server host computer.
* @return void
*/
void putIdamServerHost(const char *host){	 
   if(initEnvironment) getIdamClientEnvironment(&environment);	// Initialise 
   strcpy(environment.server_host, host);			// IDAM server's host name or IP address
   environment.server_reconnect = 1;				// Create a new Server instance
   env_host = 0;     
}

//! Set the IDAM data server port number
/** This takes precedence over the environment variables IDAM_PORT.
* @param port The port number the server is connected to.
* @return void
*/
void putIdamServerPort(int port){	 
   if(initEnvironment) getIdamClientEnvironment(&environment);	// Initialise 
   environment.server_port = port;				// IDAM server service port number 
   environment.server_reconnect = 1;				// Create a new Server instance
   env_port = 0;       
}

//! Specify a specific IDAM server socket connection to use
/** The client can be connected to multiple servers, distinguished by their socket id. 
Select the server connection required.
* @param socket The socket ID of the server connection required.
* @return void
*/
void putIdamServerSocket(int socket){	 
   if(initEnvironment) getIdamClientEnvironment(&environment);	// Initialise 
   environment.server_socket        = socket;			// IDAM server service socket number (Must be Open)
   environment.server_change_socket = 1;			// Connect to an Existing Server       
}		

//--------------------------------------------------------------
// Standard C GET Accessor Routines

//! Return the IDAM data server host name, port number and socket connection id
/**
* @param host A preallocated string that will contain the name of the server host computer.
* @param port Returned port number.
* @param socket Returned socket id number.
* @return void
*/
void getIdamServer(char *host, int *port, int *socket){		// Active ...
   if(initEnvironment) getIdamClientEnvironment(&environment);	// Initialise 
   *socket = environment.server_socket;			        // IDAM server service socket number 	 
   *port   = environment.server_port;				// IDAM server service port number 
   host    = environment.server_host;				// IDAM server's host name or IP address
}

//! the IDAM server connection host name
/** 
* @return the Name of the Host
*/
char *getIdamServerHost(){
   if(initEnvironment) getIdamClientEnvironment(&environment);	// Initialise 
   return environment.server_host;				// Active IDAM server's host name or IP address
}

//! the IDAM server connection port number
/**
* @return the Name of the Host
*/
int getIdamServerPort(){
   if(initEnvironment) getIdamClientEnvironment(&environment);	// Initialise 
   return environment.server_port;				// Active IDAM server service port number 
}

//! the IDAM server connection socket ID
/**
* @return the connection socket ID
*/
int getIdamServerSocket(){
   if(initEnvironment) getIdamClientEnvironment(&environment);	// Initialise 
   return environment.server_socket;				// Active IDAM server service socket number 	 
}

//! the IDAM client library verion number
/** 
* @return the verion number
*/
int getIdamClientVersion(){
   return clientVersion;					// Client Library Version				 	 
}

//! the IDAM server verion number
/**
* @return the verion number
*/
int getIdamServerVersion(){			 
   return (server_block.version);				// Server Version			 	 
}

//! the IDAM server error code returned
/**
* @return the error code
*/
int getIdamServerErrorCode(){			 
   return (server_block.error);					// Server Error Code			 	 
}

//! the IDAM server error message returned
/** 
* @return the error message
*/
char *getIdamServerErrorMsg(){			 
   return (server_block.msg);					// Server Error Message			 	 
}

//! the number of IDAM server error message records returned in the error stack
/**
* @return the number of records
*/
int getIdamServerErrorStackSize(){			 
   return (server_block.idamerrorstack.nerrors);		// Server Error Stack Size (No.Records)		 	 
}

//! the Type of server error of a specific server error record
/**
* @param record the error stack record number
* @return the type id
*/
int getIdamServerErrorStackRecordType(int record){
   if(record < 0 || record >= server_block.idamerrorstack.nerrors) return 0;
   return (server_block.idamerrorstack.idamerror[record].type);	// Server Error Stack Record Type		 	 
}

//! the Error code of a specific server error record
/**
* @param record the error stack record number
* @return the error code
*/
int getIdamServerErrorStackRecordCode(int record){
   if(record < 0 || record >= server_block.idamerrorstack.nerrors) return 0;
   return (server_block.idamerrorstack.idamerror[record].code);	// Server Error Stack Record Code		 	 
}

//! the Server error Location name of a specific error record
/** 
* @param record the error stack record number
* @return the location name
*/
char *getIdamServerErrorStackRecordLocation(int record){
   if(record < 0 || record >= server_block.idamerrorstack.nerrors) return 0;
   return (server_block.idamerrorstack.idamerror[record].location);	// Server Error Stack Record Location		 	 
}

//! the Server error message of a specific error record
/**
* @param record the error stack record number
* @return the error message
*/
char *getIdamServerErrorStackRecordMsg(int record){
   if(debugon && dbgout != NULL){ 
      fprintf(dbgout,"getIdamServerErrorStackRecordMsg: record %d\n", record);
      fprintf(dbgout,"getIdamServerErrorStackRecordMsg: count  %d\n", server_block.idamerrorstack.nerrors);
   }
   if(record < 0 || record >= server_block.idamerrorstack.nerrors) return 0;
   return (server_block.idamerrorstack.idamerror[record].msg);	// Server Error Stack Record Message		 	 
}

//! Return the Server error message stack data structure
/** 
@return	the error message stack data structure
*/
IDAMERRORSTACK *getIdamServerErrorStack(){			 
   return (&server_block.idamerrorstack);			// Server Error Stack Structure		 	 
}

//!  returns the data access error code
/**
\param   handle   The data object handle.
\return   Return error code, if non-zero there was a problem: < 0 is client side, > 0 is server side.
*/
int getIdamErrorCode(int handle){					// Error Code Returned from Server
   if(handle < 0 || handle >= Data_Block_Count) 
      return(getIdamServerErrorStackRecordCode(0));			 
   else
      return ((int)Data_Block[handle].errcode);
}

//!  returns the data access error message
/**
\param   handle   The data object handle.
\return   the error message.
*/
char *getIdamErrorMsg(int handle){					// Error Message returned from server
   if(handle < 0 || handle >= Data_Block_Count){
/*
      if(debugon && dbgout != NULL){ 
         char *pp = "#1 This is a TEST MESSAGE!!!";
         char *p = getIdamServerErrorStackRecordMsg(0);
	 fprintf(dbgout,"getIdamErrorMsg: Invalid handle %d\n", handle);
	 if(p == 0){
	    fprintf(dbgout,"getIdamErrorMsg: Null Message in Stack!\n");
	    int stackSize = getIdamServerErrorStackSize();
            for(int i=0;i<stackSize;i++){
               fprintf(dbgout,"[%d] code: %d type: %d location: %s msg: %s\n", i, getIdamServerErrorStackRecordCode(i),
                  getIdamServerErrorStackRecordType(i), getIdamServerErrorStackRecordLocation(i),
	          getIdamServerErrorStackRecordMsg(i)); 
            } 
	 } else {
	    //fprintf(dbgout,"sizeof: %d\n", sizeof(char *));
	    fprintf(dbgout,"*** getIdamErrorMsg: %s\n", p);
	    fprintf(dbgout,"MSG address: %p [%llu]\n", p, (unsigned long long)p);
	    return(p);
	    fprintf(dbgout,"*** getIdamErrorMsg: %s\n", pp);
	    fprintf(dbgout,"MSG address: %p [%llu]\n", pp, (unsigned long long)pp);
	    return(pp);
	 }   
      }
*/            
      return(getIdamServerErrorStackRecordMsg(0));			       			 
   } else {
/*   
      if(debugon && dbgout != NULL){ 
         char *pp = "#2 This is a TEST MESSAGE!!!";
         char *p = Data_Block[handle].error_msg;
	 fprintf(dbgout,"getIdamErrorMsg: Valid handle %d\n", handle);
	 fprintf(dbgout,"*** getIdamErrorMsg: %s\n", p);
	 fprintf(dbgout,"MSG address: %p [%llu]\n", p, (unsigned long long)p);
	 return(p);
	 fprintf(dbgout,"*** getIdamErrorMsg: %s\n", pp);
	 fprintf(dbgout,"MSG address: %p [%llu]\n", pp, (unsigned long long)pp);
	 return(pp);
      }
*/                  
      return ((char *)Data_Block[handle].error_msg);
   }   
}

//!  returns the data source quality status
/**
\param   handle   The data object handle.
\return   Quality status.
*/
int getIdamSourceStatus(int handle){				// Source Status
   if(handle < 0 || handle >= Data_Block_Count) return 0;
   return ((int)Data_Block[handle].source_status);
}

//!  returns the data object quality status
/**
\param   handle   The data object handle.
\return   Quality status.
*/
int getIdamSignalStatus(int handle){				// Signal Status
   if(handle < 0 || handle >= Data_Block_Count) return 0;
   return ((int)Data_Block[handle].signal_status);
}

int getIdamDataStatus(int handle){				// Data Status based on Standard Rule
   if(handle < 0 || handle >= Data_Block_Count) return 0;
   if(getIdamSignalStatus(handle) == DEFAULT_STATUS) 		// Signal Status Not Changed from Default - use Data Source Value
      return ((int)Data_Block[handle].source_status); 
   else   
      return ((int)Data_Block[handle].signal_status);
} 

//!  returns the last data object handle issued
/**
\return   handle.
*/
int getIdamLastHandle(){     
   return(Data_Block_Count-1);
}  
 
//!  returns the number of data items in the data object
/** the number of array elements
\param	handle   The data object handle 
\return	the number of data items
*/
int getIdamDataNum(int handle){					// Data Array Size
   if(handle < 0 || handle >= Data_Block_Count) return 0;
   return ((int)Data_Block[handle].data_n);
} 

//!  returns the rank of the data object
/** the number of array coordinate dimensions
\param	handle   The data object handle 
\return	the rank
*/
int getIdamRank(int handle){					// Array Rank
   if(handle < 0 || handle >= Data_Block_Count) return 0;
   return ((int)Data_Block[handle].rank);
} 
 
//!  Returns the position of the time coordinate dimension in the data object
/** For example, a rank 3 array data[time][x][y] (in Fortran and IDL this is data(y,x,time)) has time order = 0 so order is
counted from left to right in c and from right to left in Fortran and IDL. 
\param	handle   The data object handle 
\return	the time coordinate dimension position
*/
int getIdamOrder(int handle){					// Time Dimension Order in Array
   if(handle < 0 || handle >= Data_Block_Count) return(-1);
   return ((int)Data_Block[handle].order); 
}

//!  returns the atomic or structure type id of the data object
/**
\param	handle   The data object handle 
\return	the type id
*/
int getIdamDataType(int handle){
   if(handle < 0 || handle >= Data_Block_Count) return(TYPE_UNKNOWN);
   return ((int)Data_Block[handle].data_type);
} 

//!  returns the atomic or structure type id of the error data object
/**
\param	handle   The data object handle 
\return	the type id
*/
int getIdamErrorType(int handle){
   if(handle < 0 || handle >= Data_Block_Count) return(TYPE_UNKNOWN);
   return ((int)Data_Block[handle].error_type); 
}

//!  returns the atomic or structure type id of a named type
/**
\param	type   The name of the type
\return	the type id
*/
int getIdamDataTypeId(const char *type){	// Return the Internal Code for Data Types
   if(!strcasecmp(type,"dcomplex"))	return(TYPE_DCOMPLEX);
   if(!strcasecmp(type,"complex"))	return(TYPE_COMPLEX);
   if(!strcasecmp(type,"double"))	return(TYPE_DOUBLE);
   if(!strcasecmp(type,"float"))	return(TYPE_FLOAT);
   if(!strcasecmp(type,"long64"))	return(TYPE_LONG64);
   if(!strcasecmp(type,"long long"))	return(TYPE_LONG64);   
   if(!strcasecmp(type,"ulong64"))            return(TYPE_UNSIGNED_LONG64);
   if(!strcasecmp(type,"unsigned long64"))    return(TYPE_UNSIGNED_LONG64);
   if(!strcasecmp(type,"unsigned long long")) return(TYPE_UNSIGNED_LONG64);
   if(!strcasecmp(type,"long"))            return(TYPE_LONG);
   if(!strcasecmp(type,"unsigned long"))   return(TYPE_UNSIGNED_LONG);
   if(!strcasecmp(type,"int"))             return(TYPE_INT);
   if(!strcasecmp(type,"integer"))         return(TYPE_INT);
   if(!strcasecmp(type,"unsigned"))        return(TYPE_UNSIGNED_INT);
   if(!strcasecmp(type,"unsigned int"))    return(TYPE_UNSIGNED_INT);
   if(!strcasecmp(type,"unsigned integer"))return(TYPE_UNSIGNED_INT);
   if(!strcasecmp(type,"short"))           return(TYPE_SHORT);
   if(!strcasecmp(type,"unsigned short"))  return(TYPE_UNSIGNED_SHORT);
   if(!strcasecmp(type,"char"))            return(TYPE_CHAR);
   if(!strcasecmp(type,"unsigned char"))   return(TYPE_UNSIGNED_CHAR);
   if(!strcasecmp(type,"unknown"))         return(TYPE_UNKNOWN);
   if(!strcasecmp(type,"undefined"))       return(TYPE_UNDEFINED);
   
   if(!strcasecmp(type,"vlen"))		return(TYPE_VLEN);
   if(!strcasecmp(type,"compound"))	return(TYPE_COMPOUND);
   if(!strcasecmp(type,"opaque"))	return(TYPE_OPAQUE);
   if(!strcasecmp(type,"enum"))		return(TYPE_ENUM);
   if(!strcasecmp(type,"string"))	return(TYPE_STRING);
   if(!strcasecmp(type,"void"))		return(TYPE_VOID);

   if(!strcasecmp(type,"string *"))	return(TYPE_STRING);

   return(TYPE_UNKNOWN);      	       
}

void getIdamErrorModel(int handle, int *model, int *param_n, float *params){
   int i;
   if(handle < 0 || handle >= Data_Block_Count){
      *model   = ERROR_MODEL_UNKNOWN;	 
      *param_n = 0;	 
      return;
   }
   *model   = Data_Block[handle].error_model;		// Model ID
   *param_n = Data_Block[handle].error_param_n;		// Number of parameters
   for(i=0;i<Data_Block[handle].error_param_n;i++) params[i] = Data_Block[handle].errparams[i];
}

int getIdamErrorAsymmetry(int handle){
   if(handle < 0 || handle >= Data_Block_Count) return 0;
   return ((int)Data_Block[handle].errasymmetry); 
}

// Return the Internal Code for a named Error Model

int getIdamErrorModelId(const char *model){
   int i;
   for(i=1;i<ERROR_MODEL_UNDEFINED;i++){
      switch(i){
         case 1:
	    if(!strcasecmp(model,"default"))return(ERROR_MODEL_DEFAULT);
	    break;
         case 2:
	    if(!strcasecmp(model,"default_asymmetric"))return(ERROR_MODEL_DEFAULT_ASYMMETRIC);
	    break;
#ifdef NO_GSL_LIB
         case 3:
	    if(!strcasecmp(model,"gaussian"))return(ERROR_MODEL_GAUSSIAN);
	    break;
         case 4:
	    if(!strcasecmp(model,"reseed"))return(ERROR_MODEL_RESEED);
	    break;
         case 5:
	    if(!strcasecmp(model,"gaussian_shift"))return(ERROR_MODEL_GAUSSIAN_SHIFT);
	    break;
	 case 6:
	    if(!strcasecmp(model,"poisson"))return(ERROR_MODEL_POISSON);
	    break;
#endif
	 default: return(ERROR_MODEL_UNKNOWN);
      }
   }
   return 0;
}

char *getIdamSyntheticData(int handle){
   int status = getIdamDataStatus(handle);
   if(handle < 0 || handle >= Data_Block_Count) return NULL;
   if(status == MIN_STATUS && !Data_Block[handle].client_block.get_bad && !get_bad) return NULL;
   if(status != MIN_STATUS && (Data_Block[handle].client_block.get_bad || get_bad)) return NULL;
   if(!get_synthetic || Data_Block[handle].error_model == ERROR_MODEL_UNKNOWN) return((char *)Data_Block[handle].data);
   generateIdamSyntheticData(handle);
   return((char *)Data_Block[handle].synthetic);	     
} 
  
//!  Returns a pointer to the requested data
/** The data may be synthetically generated.
\param	handle   The data object handle 
\return	a pointer to the data - if the status is poor, a NULL pointer is returned unless the \e get_bad property is set.
*/
char *getIdamData(int handle){
   int status = getIdamDataStatus(handle);
   if(handle < 0 || handle >= Data_Block_Count) return NULL;
   if(status == MIN_STATUS && !Data_Block[handle].client_block.get_bad && !get_bad) return NULL;
   if(status != MIN_STATUS && (Data_Block[handle].client_block.get_bad || get_bad)) return NULL;
   if(!get_synthetic) 
      return((char *)Data_Block[handle].data);
   else   
      return(getIdamSyntheticData(handle)); 	     
} 

//! Copy the requested data block to a data buffer for use in MDS+ TDI functions
/**
\param	handle	The data object handle 
\param	char	A preallocated memory block to receive a copy of the data 
\return	void
*/
void getIdamDataTdi(int handle, char *data){
   if(handle < 0 || handle >= Data_Block_Count) return;
   memcpy(data, (void *)Data_Block[handle].data, (int)Data_Block[handle].data_n);
}

char *getIdamAsymmetricError(int handle, int above){
   if(handle < 0 || handle >= Data_Block_Count) return NULL;
   if(Data_Block[handle].error_type != TYPE_UNKNOWN){
      if(above){
         return ((char *)Data_Block[handle].errhi);		// return the default error array
      } else {
         if(!Data_Block[handle].errasymmetry)
	    return ((char *)Data_Block[handle].errhi);		// return the default error array if symmetric errors
	 else
	    return ((char *)Data_Block[handle].errlo);		// otherwise the data array must have been returned by the server or generated 	    	    							
      }	
   } else {
      if(Data_Block[handle].error_model != ERROR_MODEL_UNKNOWN){
         generateIdamDataError(handle);				// Create the errors from a model if the model exits
	 if(above)
	    return ((char *)Data_Block[handle].errhi);
	 else
	    if(!Data_Block[handle].errasymmetry)
	       return ((char *)Data_Block[handle].errhi);		 
	    else
	       return ((char *)Data_Block[handle].errlo);
      } else {

	 char *errhi=NULL;		// Regular Error Component
	 char *errlo=NULL;		// Asymmetric Error Component 
         int i, ndata, rc;
	 
         ndata = Data_Block[handle].data_n;
         Data_Block[handle].error_type = Data_Block[handle].data_type;	// Error Type is Unknown so Assume Data's Data Type 
	 
	 if((rc = allocArray(Data_Block[handle].error_type, ndata, &errhi))!= 0){	// Allocate Heap for Regular Error Data
	    if(verbose && errout != NULL) fprintf(errout,"Heap Allocation Problem with Data Errors\n");
	    Data_Block[handle].errhi = NULL;	  
	 } else 
	    Data_Block[handle].errhi = errhi;   
	 
	 if(Data_Block[handle].errasymmetry){				// Allocate Heap for the Asymmetric Error Data 
	    if((rc = allocArray(Data_Block[handle].error_type, ndata, &errlo))!= 0){
	       if(verbose && errout != NULL){
	          fprintf(errout,"Heap Allocation Problem with Asymmetric Errors\n");
		  fprintf(errout,"Switching Asymmetry Off!\n");
	       }
	       Data_Block[handle].errlo = NULL;	  
	       Data_Block[handle].errasymmetry = 0;
	    } else 
	       Data_Block[handle].errlo = errlo;   
	 }

// Generate and return Zeros if this data is requested unless Error is Modelled
	 
         switch(Data_Block[handle].data_type){			
	    case TYPE_FLOAT: {
	       float *fh, *fl;		
	       fh = (float *)Data_Block[handle].errhi;
	       if(Data_Block[handle].errasymmetry) fl = (float *)Data_Block[handle].errlo;
	       for(i=0;i<ndata;i++){
	          *(fh+i) = (float)0.0;
		  if(Data_Block[handle].errasymmetry) *(fl+i) = (float)0.0;  	       
	       }	  
	       break;   
	    }
	    case TYPE_DOUBLE: {
	       double *dh, *dl;
	       dh = (double *)Data_Block[handle].errhi;
	       if(Data_Block[handle].errasymmetry) dl = (double *)Data_Block[handle].errlo;
	       for(i=0;i<ndata;i++){
	          *(dh+i) = (double)0.0;
		  if(Data_Block[handle].errasymmetry) *(dl+i) = (double)0.0;
	       }	  
	       break;
	    }
	    case TYPE_SHORT:{
	       short *sh, *sl;
	       sh = (short *)Data_Block[handle].errhi;
	       if(Data_Block[handle].errasymmetry) sl = (short *)Data_Block[handle].errlo;
	       for(i=0;i<ndata;i++){
	          *(sh+i) = (short)0;
		  if(Data_Block[handle].errasymmetry) *(sl+i) = (short)0;
	       }	  
	       break;
	    }
	    case TYPE_UNSIGNED_SHORT:{
	       unsigned short *sh, *sl;
	       sh = (unsigned short *)Data_Block[handle].errhi;
	       if(Data_Block[handle].errasymmetry) sl = (unsigned short *)Data_Block[handle].errlo;
	       for(i=0;i<ndata;i++){
	          sh[i] = (unsigned short)0;
		  if(Data_Block[handle].errasymmetry) sl[i] = (unsigned short)0;
	       }	  
	       break;
	    }
	    case TYPE_INT:{
	       int *ih, *il;
	       ih = (int *)Data_Block[handle].errhi;
	       if(Data_Block[handle].errasymmetry) il = (int *)Data_Block[handle].errlo;
	       for(i=0;i<ndata;i++){
	          *(ih+i) = (int)0;
		  if(Data_Block[handle].errasymmetry) *(il+i) = (int)0;
	       }	  
	       break;
	    }
	    case TYPE_UNSIGNED_INT:{
	       unsigned int *uh, *ul;
	       uh = (unsigned int *)Data_Block[handle].errhi;
	       if(Data_Block[handle].errasymmetry) ul = (unsigned int *)Data_Block[handle].errlo;
	       for(i=0;i<ndata;i++){
	          *(uh+i) = (unsigned int)0;
		  if(Data_Block[handle].errasymmetry) *(ul+i) = (unsigned int)0;
	       }
	       break;
	    }	 
	    case TYPE_LONG:{
	       long *lh, *ll;
	       lh = (long *)Data_Block[handle].errhi;
	       if(Data_Block[handle].errasymmetry) ll = (long *)Data_Block[handle].errlo;
	       for(i=0;i<ndata;i++){
	          *(lh+i) = (long)0;
		  if(Data_Block[handle].errasymmetry) *(ll+i) = (long)0;
	       }
	       break;
	    }
	    case TYPE_UNSIGNED_LONG:{
	       unsigned long *lh, *ll;
	       lh = (unsigned long *)Data_Block[handle].errhi;
	       if(Data_Block[handle].errasymmetry) ll = (unsigned long *)Data_Block[handle].errlo;
	       for(i=0;i<ndata;i++){
	          lh[i] = (unsigned long)0;
		  if(Data_Block[handle].errasymmetry) ll[i] = (unsigned long)0;
	       }
	       break;
	    }
	    case TYPE_LONG64:{
	       long long int *lh, *ll;
	       lh = (long long int *)Data_Block[handle].errhi;
	       if(Data_Block[handle].errasymmetry) ll = (long long int *)Data_Block[handle].errlo;
	       for(i=0;i<ndata;i++){
	          *(lh+i) = (long long int)0;
		  if(Data_Block[handle].errasymmetry) *(ll+i) = (long long int)0;
	       }
	       break;
	    }	    
	    case TYPE_UNSIGNED_LONG64:{
	       unsigned long long int *lh, *ll;
	       lh = (unsigned long long int *)Data_Block[handle].errhi;
	       if(Data_Block[handle].errasymmetry) ll = (unsigned long long int *)Data_Block[handle].errlo;
	       for(i=0;i<ndata;i++){
	          lh[i] = (unsigned long long int)0;
		  if(Data_Block[handle].errasymmetry) ll[i] = (unsigned long long int)0;
	       }
	       break;
	    }    	       
	    case TYPE_CHAR:{
	       char *ch, *cl;
	       ch = (char *)Data_Block[handle].errhi;
	       if(Data_Block[handle].errasymmetry) cl = (char *)Data_Block[handle].errlo;
	       for(i=0;i<ndata;i++){
	          ch[i] = (char) 0;
		  if(Data_Block[handle].errasymmetry) cl[i] = (char) 0;
	       }
	       break;        
 	    }
	    case TYPE_UNSIGNED_CHAR:{
	       unsigned char *ch, *cl;
	       ch = (unsigned char *)Data_Block[handle].errhi;
	       if(Data_Block[handle].errasymmetry) cl = (unsigned char *)Data_Block[handle].errlo;
	       for(i=0;i<ndata;i++){
	          ch[i] = (unsigned char) 0;
		  if(Data_Block[handle].errasymmetry) cl[i] = (unsigned char) 0;
	       }
	       break;        
 	    }
	    case TYPE_DCOMPLEX: {
	       DCOMPLEX *ch, *cl;
	       ch = (DCOMPLEX *)Data_Block[handle].errhi;
	       if(Data_Block[handle].errasymmetry) cl = (DCOMPLEX *)Data_Block[handle].errlo;
	       for(i=0;i<ndata;i++){
	          ch[i].real      = (double)0.0;
		  ch[i].imaginary = (double)0.0;
		  if(Data_Block[handle].errasymmetry){
		     cl[i].real      = (double)0.0;
		     cl[i].imaginary = (double)0.0;
	          }
	       }	  
	       break;
	    }
	    case TYPE_COMPLEX: {
	       COMPLEX *ch, *cl;
	       ch = (COMPLEX *)Data_Block[handle].errhi;
	       if(Data_Block[handle].errasymmetry) cl = (COMPLEX *)Data_Block[handle].errlo;
	       for(i=0;i<ndata;i++){
	          ch[i].real      = (float)0.0;
		  ch[i].imaginary = (float)0.0;
		  if(Data_Block[handle].errasymmetry){
		     cl[i].real      = (float)0.0;
		     cl[i].imaginary = (float)0.0;
	          }
	       }	  
	       break;
	    }
	}
	      	       
         if(above)
	    return ((char *)Data_Block[handle].errhi);
	 else
	    if(!Data_Block[handle].errasymmetry)
	       return ((char *)Data_Block[handle].errhi);		 
	    else
	       return ((char *)Data_Block[handle].errlo);	
      }   
   }
}   

//!  Returns a pointer to the memory block containing the requested error data
/** The error data may be synthetically generated.
\param	handle   The data object handle 
\return	a pointer to the data 
*/
char *getIdamError(int handle){
   int above = 1;
   if(handle < 0 || handle >= Data_Block_Count) return NULL;
   return (getIdamAsymmetricError(handle, above));
}
 
//!  Returns data cast to double precision
/** The copy buffer must be preallocated and sized for the data type. The data may be synthetically generated. If the status of the data is poor, no copy to the buffer occurs unless
the property \b get_bad is set.
\param	handle	The data object handle 
\param	fp	A \b double pointer to a preallocated data buffer
\return	void 
*/
void getIdamDoubleData(int handle, double *fp){ 		// Copy Data cast as double to User Provided Array

// **** The double array must be TWICE the size if the type is COMPLEX otherwise a seg fault will occur!
 
   int status = getIdamDataStatus(handle);
   if(handle < 0 || handle >= Data_Block_Count) return;
   if(status == MIN_STATUS && !Data_Block[handle].client_block.get_bad && !get_bad) return;
   if(status != MIN_STATUS && (Data_Block[handle].client_block.get_bad || get_bad)) return;
   
   if(Data_Block[handle].data_type == TYPE_DOUBLE){
      if(!get_synthetic) 
         memcpy((void *)fp, (void *)Data_Block[handle].data, (size_t) Data_Block[handle].data_n*sizeof(double));
      else {
         generateIdamSyntheticData(handle);
         if(Data_Block[handle].synthetic != NULL)
            memcpy((void *)fp, (void *)Data_Block[handle].synthetic, (size_t) Data_Block[handle].data_n*sizeof(double));
         else
            memcpy((void *)fp, (void *)Data_Block[handle].data, (size_t) Data_Block[handle].data_n*sizeof(double));
         return;
      }	 
   } else {
      
      char *array;
      int i, ndata;
      
      ndata = getIdamDataNum(handle);
      
      if(!get_synthetic)
         array = Data_Block[handle].data;
      else {
         generateIdamSyntheticData(handle);
         if(Data_Block[handle].synthetic != NULL)
	    array = Data_Block[handle].synthetic;
	 else 
	    array = Data_Block[handle].data;  	       
      }
      
      switch(Data_Block[handle].data_type){	 
	 case TYPE_FLOAT:{
	    float *dp = (float *)array;
	    for(i=0;i<ndata;i++) fp[i] = (double)dp[i];
	    break;
	 }
	 case TYPE_SHORT:{
	    short *sp = (short *)array;
	    for(i=0;i<ndata;i++) fp[i] = (double)sp[i];
	    break;
	 }
	 case TYPE_UNSIGNED_SHORT:{
	    unsigned short *sp = (unsigned short *)array;
	    for(i=0;i<ndata;i++) fp[i] = (double)sp[i];
	    break;
	 }
	 case TYPE_INT:{
	    int *ip = (int *)array;
	    for(i=0;i<ndata;i++) fp[i] = (double)ip[i];
	    break;
	 }
	 case TYPE_UNSIGNED_INT:{
	    unsigned int *up = (unsigned int *)array;
	    for(i=0;i<ndata;i++) fp[i] = (double)up[i];
	    break;
	 }
	 case TYPE_LONG:{
	    long *lp = (long *)array;
	    for(i=0;i<ndata;i++) fp[i] = (double)lp[i];
	    break;
	 }
	 case TYPE_UNSIGNED_LONG:{
	    unsigned long *lp = (unsigned long *)array;
	    for(i=0;i<ndata;i++) fp[i] = (double)lp[i];
	    break;
	 }
	 case TYPE_LONG64:{
	    long long int *lp = (long long int *)array;
	    for(i=0;i<ndata;i++) fp[i] = (double)lp[i];
	    break;
	 }	 
	 case TYPE_UNSIGNED_LONG64:{
	    unsigned long long int *lp = (unsigned long long int *)array;
	    for(i=0;i<ndata;i++) fp[i] = (double)lp[i];
	    break;
	 }
	 case TYPE_CHAR:{
	    char * cp = (char *)array;
	    for(i=0;i<ndata;i++) fp[i] = (double)cp[i];
	    break;
	 }
	 case TYPE_UNSIGNED_CHAR:{
	    unsigned char *cp = (unsigned char *)array;
	    for(i=0;i<ndata;i++) fp[i] = (double)cp[i];
	    break;
         }
	 case TYPE_UNKNOWN:{
	    for(i=0;i<ndata;i++) fp[i] = (double)0.0;	// No Data !
	    break;	            
         }
	 case TYPE_DCOMPLEX:{
	    int j = 0;
	    DCOMPLEX *dp = (DCOMPLEX *)array;
	    for(i=0;i<ndata;i++){
	       fp[j++] = (double)dp[i].real;
	       fp[j++] = (double)dp[i].imaginary;
	    }
	    break;
	 }
	 case TYPE_COMPLEX:{
	    int j = 0;
	    COMPLEX *dp = (COMPLEX *)array;
	    for(i=0;i<ndata;i++){
	       fp[j++] = (double)dp[i].real;
	       fp[j++] = (double)dp[i].imaginary;
	    }
	    break;
	 }
	 default:   
	    for(i=0;i<ndata;i++) fp[i] = (double)0.0;
	    break;	           
	 
      }
      return ;	
   }   
}

 
//!  Returns data cast to single precision
/** The copy buffer must be preallocated and sized for the data type. The data may be synthetically generated. If the status of the data is poor, no copy to the buffer occurs unless
the property \b get_bad is set.
\param	handle	The data object handle 
\param	fp	A \b float pointer to a preallocated data buffer
\return	void 
*/
void getIdamFloatData(int handle, float *fp){ 		// Copy Data cast as float to User Provided Array

// **** The float array must be TWICE the size if the type is COMPLEX otherwise a seg fault will occur!
 
   int status = getIdamDataStatus(handle);
   if(handle < 0 || handle >= Data_Block_Count) return;
//   if(status == MIN_STATUS && !get_bad) return;
//   if(status != MIN_STATUS && get_bad) return;
   if(status == MIN_STATUS && !Data_Block[handle].client_block.get_bad && !get_bad) return;
   if(status != MIN_STATUS && (Data_Block[handle].client_block.get_bad || get_bad)) return;
   
   if(Data_Block[handle].data_type == TYPE_FLOAT){
      if(!get_synthetic) 
         memcpy((void *)fp, (void *)Data_Block[handle].data, (size_t) Data_Block[handle].data_n*sizeof(float));
      else {
         generateIdamSyntheticData(handle);
         if(Data_Block[handle].synthetic != NULL)
            memcpy((void *)fp, (void *)Data_Block[handle].synthetic, (size_t) Data_Block[handle].data_n*sizeof(float));
         else
            memcpy((void *)fp, (void *)Data_Block[handle].data, (size_t) Data_Block[handle].data_n*sizeof(float));
         return;
      }	 
   } else {
      
      char *array;
      int i, ndata;
      
      ndata = getIdamDataNum(handle);
      
      if(!get_synthetic)
         array = Data_Block[handle].data;
      else {
         generateIdamSyntheticData(handle);
         if(Data_Block[handle].synthetic != NULL)
	    array = Data_Block[handle].synthetic;
	 else 
	    array = Data_Block[handle].data;  	       
      }
      
      switch(Data_Block[handle].data_type){	 
	 case TYPE_DOUBLE:{
	    double *dp = (double *)array;
	    for(i=0;i<ndata;i++) fp[i] = (float)dp[i];
	    break;
	 }
	 case TYPE_SHORT:{
	    short *sp = (short *)array;
	    for(i=0;i<ndata;i++) fp[i] = (float)sp[i];
	    break;
	 }
	 case TYPE_UNSIGNED_SHORT:{
	    unsigned short *sp = (unsigned short *)array;
	    for(i=0;i<ndata;i++) fp[i] = (float)sp[i];
	    break;
	 }
	 case TYPE_INT:{
	    int *ip = (int *)array;
	    for(i=0;i<ndata;i++) fp[i] = (float)ip[i];
	    break;
	 }
	 case TYPE_UNSIGNED_INT:{
	    unsigned int *up = (unsigned int *)array;
	    for(i=0;i<ndata;i++) fp[i] = (float)up[i];
	    break;
	 }
	 case TYPE_LONG:{
	    long *lp = (long *)array;
	    for(i=0;i<ndata;i++) fp[i] = (float)lp[i];
	    break;
	 }
	 case TYPE_UNSIGNED_LONG:{
	    unsigned long *lp = (unsigned long *)array;
	    for(i=0;i<ndata;i++) fp[i] = (float)lp[i];
	    break;
	 }
	 case TYPE_LONG64:{
	    long long int *lp = (long long int *)array;
	    for(i=0;i<ndata;i++) fp[i] = (float)lp[i];
	    break;
	 }	 
	 case TYPE_UNSIGNED_LONG64:{
	    unsigned long long int *lp = (unsigned long long int *)array;
	    for(i=0;i<ndata;i++) fp[i] = (float)lp[i];
	    break;
	 }
	 case TYPE_CHAR:{
	    char * cp = (char *)array;
	    for(i=0;i<ndata;i++) fp[i] = (float)cp[i];
	    break;
	 }
	 case TYPE_UNSIGNED_CHAR:{
	    unsigned char *cp = (unsigned char *)array;
	    for(i=0;i<ndata;i++) fp[i] = (float)cp[i];
	    break;
         }
	 case TYPE_UNKNOWN:{
	    for(i=0;i<ndata;i++) fp[i] = (float)0.0;	// No Data !
	    break;	            
         }
	 case TYPE_DCOMPLEX:{
	    int j = 0;
	    DCOMPLEX *dp = (DCOMPLEX *)array;
	    for(i=0;i<ndata;i++){
	       fp[j++] = (float)dp[i].real;
	       fp[j++] = (float)dp[i].imaginary;
	    }
	    break;
	 }
	 case TYPE_COMPLEX:{
	    int j = 0;
	    COMPLEX *dp = (COMPLEX *)array;
	    for(i=0;i<ndata;i++){
	       fp[j++] = (float)dp[i].real;
	       fp[j++] = (float)dp[i].imaginary;
	    }
	    break;
	 }
	 default:   
	    for(i=0;i<ndata;i++) fp[i] = (float)0.0;
	    break;	           
	 
      }
      return ;	
   }   
}

//!  Returns data as void type
/** The copy buffer must be preallocated and sized for the correct data type. 
\param	handle	The data object handle 
\param	data	A \b void pointer to a preallocated data buffer
\return	void 
*/
void getIdamGenericData(int handle, void *data){
   getidamdatablock_(&handle, data);
}
 
 
void getIdamFloatAsymmetricError(int handle, int above, float *fp){ 	// Copy Error Data cast as float to User Provided Array			

// **** The float array must be TWICE the size if the type is COMPLEX otherwise a seg fault will occur!

   int i, ndata;
   
   if(handle < 0 || handle >= Data_Block_Count) return;
   
   ndata = Data_Block[handle].data_n;
    
   if(Data_Block[handle].error_type == TYPE_UNKNOWN) getIdamAsymmetricError(handle, above);	// Create the Error Data prior to Casting 
      
   switch(Data_Block[handle].error_type){
      case TYPE_UNKNOWN:
	 for(i=0;i<ndata;i++) fp[i] = (float)0.0;	// No Error Data
	 break;	
      case TYPE_FLOAT:
         if(above)
	    memcpy((void *)fp, (void *)Data_Block[handle].errhi, (size_t) Data_Block[handle].data_n*sizeof(float));
	 else 
	    if(!Data_Block[handle].errasymmetry)
	       memcpy((void *)fp, (void *)Data_Block[handle].errhi, (size_t) Data_Block[handle].data_n*sizeof(float));
	    else
	       memcpy((void *)fp, (void *)Data_Block[handle].errlo, (size_t) Data_Block[handle].data_n*sizeof(float));  
	 break;		 
      case TYPE_DOUBLE:{
         double *dp;
	 if(above)
	    dp = (double *)Data_Block[handle].errhi;
	 else
	    if(!Data_Block[handle].errasymmetry)
	       dp = (double *)Data_Block[handle].errhi;
	    else   
	       dp = (double *)Data_Block[handle].errlo;   
	    for(i=0;i<ndata;i++) fp[i] = (float)dp[i];
	 break;
      }
      case TYPE_SHORT:{
         short * sp;
	 if(above)
	    sp = (short *)Data_Block[handle].errhi;
	 else
	    if(!Data_Block[handle].errasymmetry)
	       sp = (short *)Data_Block[handle].errhi;
	    else
	       sp = (short *)Data_Block[handle].errlo;
	 for(i=0;i<ndata;i++) fp[i] = (float)sp[i];
	 break;
      }
      case TYPE_UNSIGNED_SHORT:{
         unsigned short *sp;
	 if(above)
	    sp = (unsigned short *)Data_Block[handle].errhi;
	 else
	    if(!Data_Block[handle].errasymmetry)
	       sp = (unsigned short *)Data_Block[handle].errhi;
	    else
	       sp = (unsigned short *)Data_Block[handle].errlo;
	 for(i=0;i<ndata;i++) fp[i] = (float)sp[i];
	 break;
      }
      case TYPE_INT:{
	 int *ip;
	 if(above)
	    ip = (int *)Data_Block[handle].errhi;
	 else
	    if(!Data_Block[handle].errasymmetry)
	       ip = (int *)Data_Block[handle].errhi;
	    else
	       ip = (int *)Data_Block[handle].errlo;
	 for(i=0;i<ndata;i++) fp[i] = (float)ip[i];
	 break;
      }
      case TYPE_UNSIGNED_INT:{
	 unsigned int *up;
	 if(above)
	    up = (unsigned int *)Data_Block[handle].errhi;
	 else
	    if(!Data_Block[handle].errasymmetry)
	       up = (unsigned int *)Data_Block[handle].errhi;
	    else
	       up = (unsigned int *)Data_Block[handle].errlo;
	 for(i=0;i<ndata;i++) fp[i] = (float)up[i];
	 break;
      }
      case TYPE_LONG:{
	 long *lp;
	 if(above)
	    lp = (long *)Data_Block[handle].errhi;
	 else
	    if(!Data_Block[handle].errasymmetry)
	       lp = (long *)Data_Block[handle].errhi;
	    else
	       lp = (long *)Data_Block[handle].errlo;
	 for(i=0;i<ndata;i++) fp[i] = (float)lp[i];
	 break;
      }
      case TYPE_UNSIGNED_LONG:{
	 unsigned long *lp;
	 if(above)
	    lp = (unsigned long *)Data_Block[handle].errhi;
	 else
	    if(!Data_Block[handle].errasymmetry)
	       lp = (unsigned long *)Data_Block[handle].errhi;
	    else
	       lp = (unsigned long *)Data_Block[handle].errlo;
	 for(i=0;i<ndata;i++) fp[i] = (float)lp[i];
	 break; 
      }
      case TYPE_LONG64:{
	 long long int *lp;
	 if(above)
	    lp = (long long int *)Data_Block[handle].errhi;
	 else
	    if(!Data_Block[handle].errasymmetry)
	       lp = (long long int *)Data_Block[handle].errhi;
	    else
	       lp = (long long int *)Data_Block[handle].errlo;
	 for(i=0;i<ndata;i++) fp[i] = (float)lp[i];
	 break;
      }
      case TYPE_UNSIGNED_LONG64:{
	 unsigned long long int *lp;
	 if(above)
	    lp = (unsigned long long int *)Data_Block[handle].errhi;
	 else
	    if(!Data_Block[handle].errasymmetry)
	       lp = (unsigned long long int *)Data_Block[handle].errhi;
	    else
	       lp = (unsigned long long int *)Data_Block[handle].errlo;
	 for(i=0;i<ndata;i++) fp[i] = (float)lp[i];
	 break;  
      }      
      case TYPE_CHAR:{    
	 char *cp;
	 if(above)
	    cp = (char *)Data_Block[handle].errhi;
	 else
	    if(!Data_Block[handle].errasymmetry)
	       cp = (char *)Data_Block[handle].errhi;
	    else
	       cp = (char *)Data_Block[handle].errlo;
	 for(i=0;i<ndata;i++) fp[i] = (float)cp[i];
	 break;        
      }
      case TYPE_UNSIGNED_CHAR:{
	 unsigned char *cp;
	 if(above)
	    cp = (unsigned char *)Data_Block[handle].errhi;
	 else
	    if(!Data_Block[handle].errasymmetry)
	       cp = (unsigned char *)Data_Block[handle].errhi;
	    else
	       cp = (unsigned char *)Data_Block[handle].errlo;
	 for(i=0;i<ndata;i++) fp[i] = (float)cp[i];
	 break;      
      }
      case TYPE_DCOMPLEX:{
	 int j=0;
	 DCOMPLEX *cp;
	 if(above)
	    cp = (DCOMPLEX *)Data_Block[handle].errhi;
	 else
	    if(!Data_Block[handle].errasymmetry)
	       cp = (DCOMPLEX *)Data_Block[handle].errhi;
	    else
	       cp = (DCOMPLEX *)Data_Block[handle].errlo;
	 for(i=0;i<ndata;i++){
	     fp[j++] = (float)cp[i].real;
	     fp[j++] = (float)cp[i].imaginary;
	 }
	 break;      
      }
      case TYPE_COMPLEX:{
	 int j=0;
	 COMPLEX *cp;
	 if(above)
	    cp = (COMPLEX *)Data_Block[handle].errhi;
	 else
	    if(!Data_Block[handle].errasymmetry)
	       cp = (COMPLEX *)Data_Block[handle].errhi;
	    else
	       cp = (COMPLEX *)Data_Block[handle].errlo;
	 for(i=0;i<ndata;i++){
	     fp[j++] = (float)cp[i].real;
	     fp[j++] = (float)cp[i].imaginary;
	 }
	 break;      
      }
      default:   
	 for(i=0;i<ndata;i++) fp[i] = (float)0.0;
	 break;	           
               
   }
   return ;	
}

//!  Returns error data cast to single precision
/** The copy buffer must be preallocated and sized for the data type. 
\param	handle	The data object handle 
\param	fp	A \b float pointer to a preallocated data buffer
\return	void 
*/
void getIdamFloatError(int handle, float *fp){
   int above = 1;
   getIdamFloatAsymmetricError(handle, above, fp);
} 
  
//!  Returns the DATA_BLOCK data structure - the data, dimension coordinates and associated meta data.
/**  
\param	handle	The data object handle 
\param	db	Returned \b DATA_BLOCK pointer
\return	void 
*/
void getIdamDBlock(int handle, DATA_BLOCK *db){
   if(handle < 0 || handle >= Data_Block_Count) return;  
   *db = Data_Block[handle];
} 

//!  Returns the DATA_BLOCK data structure - the data, dimension coordinates and associated meta data.
/**  
\param	handle	The data object handle 
\return	DATA_BLOCK pointer
*/
DATA_BLOCK *getIdamDataBlock(int handle){
   if(handle < 0 || handle >= Data_Block_Count) return NULL;
   return(&Data_Block[handle]);
}

//!  Returns the data label of a data object
/**  
\param	handle	The data object handle 
\return	pointer to the data label
*/
char *getIdamDataLabel(int handle){
   if(handle < 0 || handle >= Data_Block_Count) return NULL;
   return ((char *)Data_Block[handle].data_label);
}

//!  Returns the data label of a data object for use in MDS+ TDI functions
/**  
\param	handle	The data object handle 
\param	label   preallocated string buffer to receive the copy of the data label
\return	void
*/
void getIdamDataLabelTdi(int handle, char *label){
   if(handle < 0 || handle >= Data_Block_Count) return;
   strcpy(label, (char *)Data_Block[handle].data_label);
}

//!  Returns the data units of a data object
/**  
\param	handle	The data object handle 
\return	pointer to the data units
*/
char *getIdamDataUnits(int handle){
   if(handle < 0 || handle >= Data_Block_Count) return NULL;
   return ((char *)Data_Block[handle].data_units);
}

//!  Returns the data units of a data object for use in MDS+ TDI functions
/**  
\param	handle	The data object handle 
\param	units   preallocated string buffer to receive the copy of the data units
\return	void
*/
void getIdamDataUnitsTdi(int handle, char *units){
   if(handle < 0 || handle >= Data_Block_Count) return;
   strcpy(units, (char *)Data_Block[handle].data_units);
} 

//!  Returns the description of a data object
/**  
\param	handle	The data object handle 
\return	pointer to the data description
*/
char *getIdamDataDesc(int handle){
   if(handle < 0 || handle >= Data_Block_Count) return NULL;
   return ((char *)Data_Block[handle].data_desc);
}

//!  Returns the description of a data object for use in MDS+ TDI functions
/**  
\param	handle	The data object handle 
\param	units   preallocated string buffer to receive the copy of the data description
\return	void
*/
void getIdamDataDescTdi(int handle, char *desc){
   if(handle < 0 || handle >= Data_Block_Count) return;
   strcpy(desc, (char *)Data_Block[handle].data_desc);
} 

// Dimension Coordinates


//! Returns the coordinate dimension size
/** the number of elements in the coordinate array 
\param	handle	The data object handle 
\param	ndim    the position of the dimension in the data array - numbering is as data[0][1][2] 
\return	the dimension size
*/
int getIdamDimNum(int handle, int ndim){
   if(handle < 0 || handle >= Data_Block_Count || ndim < 0 || ndim >= Data_Block[handle].rank) return 0;
   return ((int)Data_Block[handle].dims[ndim].dim_n);
}

//! Returns the coordinate dimension data type
/**
\param	handle	The data object handle 
\param	ndim    the position of the dimension in the data array - numbering is as data[0][1][2] 
\return	the data type id
*/
int getIdamDimType(int handle, int ndim){
   if(handle < 0 || handle >= Data_Block_Count || ndim < 0 || ndim >= Data_Block[handle].rank) return(TYPE_UNKNOWN);
   return ((int)Data_Block[handle].dims[ndim].data_type);
} 

//! Returns the coordinate dimension error data type
/**
\param	handle	The data object handle 
\param	ndim    the position of the dimension in the data array - numbering is as data[0][1][2] 
\return	the data type id
*/
int getIdamDimErrorType(int handle, int ndim){
   if(handle < 0 || handle >= Data_Block_Count || ndim < 0 || ndim >= Data_Block[handle].rank) return(TYPE_UNKNOWN);
   return ((int)Data_Block[handle].dims[ndim].error_type); 
}

//! Returns whether or not coordinate error data are asymmetric.
/**
\param	handle	The data object handle 
\param	ndim    the position of the dimension in the data array - numbering is as data[0][1][2] 
\return	boolean true or false i.e. 1 or 0
*/
int getIdamDimErrorAsymmetry(int handle, int ndim){
   if(handle < 0 || handle >= Data_Block_Count || ndim < 0 || ndim >= Data_Block[handle].rank) return 0;
   return ((int)Data_Block[handle].dims[ndim].errasymmetry); 
}
/*
void getIdamDimErrorModel(int handle, int ndim, int *model, int *param_n, float **params){
   if(handle < 0 || handle >= Data_Block_Count || ndim < 0 || ndim >= Data_Block[handle].rank){
      *model   = ERROR_MODEL_UNKNOWN;
      *param_n = 0;
      *params  = NULL;
       return;
   }
   *model   = Data_Block[handle].dims[ndim].error_model;		// Model ID
   *param_n = Data_Block[handle].dims[ndim].error_param_n;		// Number of parameters
   *params  = Data_Block[handle].dims[ndim].errparams; 			// Array of Model Parameters
}
*/

void getIdamDimErrorModel(int handle, int ndim, int *model, int *param_n, float *params){
   int i;
   if(handle < 0 || handle >= Data_Block_Count || ndim < 0 || ndim >= Data_Block[handle].rank){
  
      *model   = ERROR_MODEL_UNKNOWN;
      *param_n = 0;
      // *params  = NULL;
       return;
   }
   *model   = Data_Block[handle].dims[ndim].error_model;		// Model ID
   *param_n = Data_Block[handle].dims[ndim].error_param_n;		// Number of parameters
   for(i=0;i<Data_Block[handle].dims[ndim].error_param_n;i++)params[i] = Data_Block[handle].dims[ndim].errparams[i]; 
   // *params  = Data_Block[handle].dims[ndim].errparams; 			// Array of Model Parameters
}

char *getIdamSyntheticDimData(int handle, int ndim){
   if(handle < 0 || handle >= Data_Block_Count || ndim < 0 || ndim >= Data_Block[handle].rank) return NULL;
   if(!get_synthetic || Data_Block[handle].dims[ndim].error_model == ERROR_MODEL_UNKNOWN) return((char *)Data_Block[handle].dims[ndim].dim);
   generateIdamSyntheticDimData(handle, ndim);
   return((char *)Data_Block[handle].dims[ndim].synthetic);	     
} 


///!  Returns a pointer to the requested coordinate data
/** The data may be synthetically generated.
\param	handle	The data object handle 
\param	ndim    the position of the dimension in the data array - numbering is as data[0][1][2] 
\return	pointer to the data
*/
char *getIdamDimData(int handle, int ndim){
   if(handle < 0 || handle >= Data_Block_Count || ndim < 0 || ndim >= Data_Block[handle].rank) return NULL;
   if(!get_synthetic) return((char *)Data_Block[handle].dims[ndim].dim);
   return(getIdamSyntheticDimData(handle,ndim)); 	    
}

//! Returns the data label of a coordinate dimension
/**  
\param	handle	The data object handle 
\param	ndim    the position of the dimension in the data array - numbering is as data[0][1][2] 
\return	pointer to the data label
*/
char *getIdamDimLabel(int handle, int ndim){
   if(handle < 0 || handle >= Data_Block_Count || ndim < 0 || ndim >= Data_Block[handle].rank) return NULL;
   return ((char *)Data_Block[handle].dims[ndim].dim_label);
}
//! Returns the data units of a coordinate dimension
/**  
\param	handle	The data object handle 
\param	ndim    the position of the dimension in the data array - numbering is as data[0][1][2] 
\return	pointer to the data units
*/
char *getIdamDimUnits(int handle, int ndim){
   if(handle < 0 || handle >= Data_Block_Count || ndim < 0 || ndim >= Data_Block[handle].rank) return NULL;
   return ((char *)Data_Block[handle].dims[ndim].dim_units);
}
 
//!  Returns the data label of a coordinate dimension for use in MDS+ TDI functions
/**  
\param	handle	The data object handle 
\param	ndim    the position of the dimension in the data array - numbering is as data[0][1][2] 
\param	label   preallocated string buffer to receive the copy of the data label
\return	void
*/
void getIdamDimLabelTdi(int handle, int ndim, char *label){
   if(handle < 0 || handle >= Data_Block_Count || ndim < 0 || ndim >= Data_Block[handle].rank) return;
   strcpy(label, (char *)Data_Block[handle].dims[ndim].dim_label);
}

//!  Returns the data units of a coordinate dimension for use in MDS+ TDI functions
/**  
\param	handle	The data object handle 
\param	ndim    the position of the dimension in the data array - numbering is as data[0][1][2] 
\param	units   preallocated string buffer to receive the copy of the data units
\return	void
*/
void getIdamDimUnitsTdi(int handle, int ndim, char *units){
   if(handle < 0 || handle >= Data_Block_Count || ndim < 0 || ndim >= Data_Block[handle].rank) return;
   strcpy(units, (char *)Data_Block[handle].dims[ndim].dim_units);
} 


//!  Returns coordinate data cast to double precision
/** The copy buffer must be preallocated and sized for the data type. 
\param	handle	The data object handle 
\param	ndim    the position of the dimension in the data array - numbering is as data[0][1][2] 
\param	fp	A \b double pointer to a preallocated data buffer
\return	void 
*/
void getIdamDoubleDimData(int handle, int ndim, double *fp){

// **** The double array must be TWICE the size if the type is COMPLEX otherwise a seg fault will occur!

   if(handle < 0 || handle >= Data_Block_Count || ndim < 0 || ndim >= Data_Block[handle].rank) return;
   if(Data_Block[handle].dims[ndim].data_type == TYPE_DOUBLE){
      if(!get_synthetic)
         memcpy((void *)fp, (void *)Data_Block[handle].dims[ndim].dim, (size_t)Data_Block[handle].dims[ndim].dim_n*sizeof(double));
      else {
         generateIdamSyntheticDimData(handle, ndim);
         if(Data_Block[handle].dims[ndim].synthetic != NULL)
	    memcpy((void *)fp, (void *)Data_Block[handle].dims[ndim].synthetic, (size_t)Data_Block[handle].dims[ndim].dim_n*sizeof(double));
	 else
	    memcpy((void *)fp, (void *)Data_Block[handle].dims[ndim].dim, (size_t)Data_Block[handle].dims[ndim].dim_n*sizeof(double));	 
         return;
      }
   } else {
      char *array;
      int i, ndata;
      
      ndata = Data_Block[handle].dims[ndim].dim_n;
      if(!get_synthetic)
         array = Data_Block[handle].dims[ndim].dim;
      else {
         generateIdamSyntheticDimData(handle, ndim);
         if(Data_Block[handle].dims[ndim].synthetic != NULL)
	    array = Data_Block[handle].dims[ndim].synthetic;
	 else
	    array = Data_Block[handle].dims[ndim].dim;	 
      }	  
      
      switch(Data_Block[handle].dims[ndim].data_type){	 
	 case TYPE_FLOAT:{
	    float *dp = (float *)array;
	    for(i=0;i<ndata;i++) fp[i] = (double)dp[i];
	    break;
	 }
	 case TYPE_SHORT:{
	    short *sp = (short *)array;
	    for(i=0;i<ndata;i++) fp[i] = (double)sp[i];
	    break;
	 }
	 case TYPE_UNSIGNED_SHORT:{
	    unsigned short *sp = (unsigned short *)array;
	    for(i=0;i<ndata;i++) fp[i] = (double)sp[i];
	    break;
	 }
	 case TYPE_INT:{
	    int *ip = (int *)array;
	    for(i=0;i<ndata;i++) fp[i] = (double)ip[i];
	    break;
	 }
	 case TYPE_UNSIGNED_INT:{
	    unsigned int *up = (unsigned int *)array;
	    for(i=0;i<ndata;i++) fp[i] = (double)up[i];
	    break;
	 }
	 case TYPE_LONG:{
	    long *lp = (long *)array;
	    for(i=0;i<ndata;i++) fp[i] = (double)lp[i];
	    break;
	 }
	 case TYPE_UNSIGNED_LONG:{
	    unsigned long *lp = (unsigned long *)array;
	    for(i=0;i<ndata;i++) fp[i] = (double)lp[i];
	    break;
	 }
	 case TYPE_LONG64:{
	    long long int *lp = (long long int *)array;
	    for(i=0;i<ndata;i++) fp[i] = (double)lp[i];
	    break;
	 }
	 case TYPE_UNSIGNED_LONG64:{
	    unsigned long long int *lp = (unsigned long long int *)array;
	    for(i=0;i<ndata;i++) fp[i] = (double)lp[i];
	    break;
	 }	 
	 case TYPE_CHAR:{
	    char *cp = (char *)array;
	    for(i=0;i<ndata;i++) fp[i] = (double)cp[i];
	    break; 
	 }
	 case TYPE_UNSIGNED_CHAR:{
	    unsigned char *cp = (unsigned char *)array;
	    for(i=0;i<ndata;i++) fp[i] = (double)cp[i];
	    break; 
         }
	 case TYPE_DCOMPLEX:{
	    int j=0;
	    DCOMPLEX *cp = (DCOMPLEX *)array;
	    for(i=0;i<ndata;i++){
	       fp[j++] = (double)cp[i].real;
	       fp[j++] = (double)cp[i].imaginary;
	    }
	    break; 
         }
	 case TYPE_COMPLEX:{
	    int j=0;
	    COMPLEX *cp = (COMPLEX *)array;
	    for(i=0;i<ndata;i++){
	       fp[j++] = (double)cp[i].real;
	       fp[j++] = (double)cp[i].imaginary;
	    }
	    break; 
         }	 
	 case TYPE_UNKNOWN:
	    for(i=0;i<ndata;i++) fp[i] = (double)0.0;
	    break;
	 default:   
	    for(i=0;i<ndata;i++) fp[i] = (double)0.0;
	    break;	           
    	           
      }
      return ;	
   }   
}


//!  Returns coordinate data cast to single precision
/** The copy buffer must be preallocated and sized for the data type. 
\param	handle	The data object handle 
\param	ndim    the position of the dimension in the data array - numbering is as data[0][1][2] 
\param	fp	A \b float pointer to a preallocated data buffer
\return	void 
*/
void getIdamFloatDimData(int handle, int ndim, float *fp){

// **** The float array must be TWICE the size if the type is COMPLEX otherwise a seg fault will occur!

   if(handle < 0 || handle >= Data_Block_Count || ndim < 0 || ndim >= Data_Block[handle].rank) return;
   if(Data_Block[handle].dims[ndim].data_type == TYPE_FLOAT){
      if(!get_synthetic)
         memcpy((void *)fp, (void *)Data_Block[handle].dims[ndim].dim, (size_t)Data_Block[handle].dims[ndim].dim_n*sizeof(float));
      else {
         generateIdamSyntheticDimData(handle, ndim);
         if(Data_Block[handle].dims[ndim].synthetic != NULL)
	    memcpy((void *)fp, (void *)Data_Block[handle].dims[ndim].synthetic, (size_t)Data_Block[handle].dims[ndim].dim_n*sizeof(float));
	 else
	    memcpy((void *)fp, (void *)Data_Block[handle].dims[ndim].dim, (size_t)Data_Block[handle].dims[ndim].dim_n*sizeof(float));	 
         return;
      }
   } else {
      char *array;
      int i, ndata;
      
      ndata = Data_Block[handle].dims[ndim].dim_n;
      if(!get_synthetic)
         array = Data_Block[handle].dims[ndim].dim;
      else {
         generateIdamSyntheticDimData(handle, ndim);
         if(Data_Block[handle].dims[ndim].synthetic != NULL)
	    array = Data_Block[handle].dims[ndim].synthetic;
	 else
	    array = Data_Block[handle].dims[ndim].dim;	 
      }	  
      
      switch(Data_Block[handle].dims[ndim].data_type){	 
	 case TYPE_DOUBLE:{
	    double *dp = (double *)array;
	    for(i=0;i<ndata;i++) fp[i] = (float)dp[i];
	    break;
	 }
	 case TYPE_SHORT:{
	    short *sp = (short *)array;
	    for(i=0;i<ndata;i++) fp[i] = (float)sp[i];
	    break;
	 }
	 case TYPE_UNSIGNED_SHORT:{
	    unsigned short *sp = (unsigned short *)array;
	    for(i=0;i<ndata;i++) fp[i] = (float)sp[i];
	    break;
	 }
	 case TYPE_INT:{
	    int *ip = (int *)array;
	    for(i=0;i<ndata;i++) fp[i] = (float)ip[i];
	    break;
	 }
	 case TYPE_UNSIGNED_INT:{
	    unsigned int *up = (unsigned int *)array;
	    for(i=0;i<ndata;i++) fp[i] = (float)up[i];
	    break;
	 }
	 case TYPE_LONG:{
	    long *lp = (long *)array;
	    for(i=0;i<ndata;i++) fp[i] = (float)lp[i];
	    break;
	 }
	 case TYPE_UNSIGNED_LONG:{
	    unsigned long *lp = (unsigned long *)array;
	    for(i=0;i<ndata;i++) fp[i] = (float)lp[i];
	    break;
	 }
	 case TYPE_LONG64:{
	    long long int *lp = (long long int *)array;
	    for(i=0;i<ndata;i++) fp[i] = (float)lp[i];
	    break;
	 }
	 case TYPE_UNSIGNED_LONG64:{
	    unsigned long long int *lp = (unsigned long long int *)array;
	    for(i=0;i<ndata;i++) fp[i] = (float)lp[i];
	    break;
	 }	 
	 case TYPE_CHAR:{
	    char *cp = (char *)array;
	    for(i=0;i<ndata;i++) fp[i] = (float)cp[i];
	    break; 
	 }
	 case TYPE_UNSIGNED_CHAR:{
	    unsigned char *cp = (unsigned char *)array;
	    for(i=0;i<ndata;i++) fp[i] = (float)cp[i];
	    break; 
         }
	 case TYPE_DCOMPLEX:{
	    int j=0;
	    DCOMPLEX *cp = (DCOMPLEX *)array;
	    for(i=0;i<ndata;i++){
	       fp[j++] = (float)cp[i].real;
	       fp[j++] = (float)cp[i].imaginary;
	    }
	    break; 
         }
	 case TYPE_COMPLEX:{
	    int j=0;
	    COMPLEX *cp = (COMPLEX *)array;
	    for(i=0;i<ndata;i++){
	       fp[j++] = (float)cp[i].real;
	       fp[j++] = (float)cp[i].imaginary;
	    }
	    break; 
         }	 
	 case TYPE_UNKNOWN:
	    for(i=0;i<ndata;i++) fp[i] = (float)0.0;
	    break; 
	 default:   
	    for(i=0;i<ndata;i++) fp[i] = (float)0.0;
	    break;	           
      }
      return ;	
   }   
}

//!  Returns coordinate data as void type
/** The copy buffer must be preallocated and sized for the correct data type. 
\param	handle	The data object handle 
\param	ndim    the position of the dimension in the data array - numbering is as data[0][1][2] 
\param	data	A \b void pointer to a preallocated data buffer
\return	void 
*/
void getIdamGenericDimData(int handle, int ndim, void *data){
   getidamdimdata_(&handle, &ndim, data);
}

//!  Returns the coordinate dimension's DIMS data structure - the coordinate data and associated meta data.
/**  
\param	handle	The data object handle 
\param	ndim    the position of the dimension in the data array - numbering is as data[0][1][2]
\return	DIMS pointer
*/
DIMS *getIdamDimBlock(int handle, int ndim){
   if(handle < 0 || handle >= Data_Block_Count || ndim < 0 || ndim >= Data_Block[handle].rank) return NULL;
   return(Data_Block[handle].dims+ndim);
}   


char *getIdamDimAsymmetricError(int handle, int ndim, int above){
   if(handle < 0 || handle >= Data_Block_Count || ndim < 0 || ndim >= Data_Block[handle].rank) return NULL;
   if(Data_Block[handle].dims[ndim].error_type != TYPE_UNKNOWN)
      if(above){
         return ((char *)Data_Block[handle].dims[ndim].errhi);		// return the default error array
      } else {
         if(!Data_Block[handle].dims[ndim].errasymmetry)
	    return ((char *)Data_Block[handle].dims[ndim].errhi);	// return the default error array if symmetric errors
	 else
	    return ((char *)Data_Block[handle].dims[ndim].errlo);	// otherwise the data array must have been returned by the server 	    	    							
      }									// or generated in a previous call	 
   else {
      if(Data_Block[handle].dims[ndim].error_model != ERROR_MODEL_UNKNOWN){
         generateIdamDimDataError(handle, ndim);
	 if(above)
	    return ((char *)Data_Block[handle].dims[ndim].errhi);
	 else
	    if(!Data_Block[handle].dims[ndim].errasymmetry)
	       return ((char *)Data_Block[handle].dims[ndim].errhi);		 
	    else
	       return ((char *)Data_Block[handle].dims[ndim].errlo);
      } else {
	 char *errhi=NULL;
	 char *errlo=NULL;
         int i, rc, ndata;
	 
         ndata = Data_Block[handle].dims[ndim].dim_n;
         Data_Block[handle].dims[ndim].error_type = Data_Block[handle].dims[ndim].data_type;	// Error Type is Unknown so Assume Data's Data Type 


	 if((rc = allocArray(Data_Block[handle].dims[ndim].error_type, ndata, &errhi)) != 0){
	    if(verbose && errout != NULL) fprintf(errout,"Heap Allocation Problem with Dimensional Data Errors\n");
	    Data_Block[handle].dims[ndim].errhi = NULL;	  
	 } else 
	    Data_Block[handle].dims[ndim].errhi = errhi;

	 if(Data_Block[handle].dims[ndim].errasymmetry){					// Allocate Heap for the Asymmetric Error Data 
	    if((rc = allocArray(Data_Block[handle].dims[ndim].error_type, ndata, &errlo)) != 0){
	       if(verbose && errout != NULL){
	          fprintf(errout,"Heap Allocation Problem with Dimensional Asymmetric Errors\n");
		  fprintf(errout,"Switching Asymmetry Off!\n");
	       }
	       Data_Block[handle].dims[ndim].errlo = errlo;	  
	       Data_Block[handle].dims[ndim].errasymmetry = 0;
	    } else 
	       Data_Block[handle].dims[ndim].errlo = errlo;
	 }
	 
         switch(Data_Block[handle].dims[ndim].data_type){			
	    case TYPE_FLOAT:{
	       float *fh, *fl;
	       fh = (float *)Data_Block[handle].dims[ndim].errhi;
	       if(Data_Block[handle].dims[ndim].errasymmetry) fl = (float *)Data_Block[handle].dims[ndim].errlo;
	       for(i=0;i<ndata;i++){
	          fh[i] = (float)0.0;
		  if(Data_Block[handle].dims[ndim].errasymmetry) fl[i] = (float)0.0;  	       
	       }	  
	       break;   
	    }
	    case TYPE_DOUBLE:{
	       double *dh, *dl;
	       dh = (double *)Data_Block[handle].dims[ndim].errhi;
	       if(Data_Block[handle].dims[ndim].errasymmetry) dl = (double *)Data_Block[handle].dims[ndim].errlo;
	       for(i=0;i<ndata;i++){
	          dh[i] = (double)0.0;
		  if(Data_Block[handle].dims[ndim].errasymmetry) dl[i] = (double)0.0;
	       }	  
	       break;
	    }
	    case TYPE_SHORT:{
	       short *sh, *sl;
	       sh = (short *)Data_Block[handle].dims[ndim].errhi;
	       if(Data_Block[handle].dims[ndim].errasymmetry) sl = (short *)Data_Block[handle].dims[ndim].errlo;
	       for(i=0;i<ndata;i++){
	          sh[i] = (short)0;
		  if(Data_Block[handle].dims[ndim].errasymmetry) sl[i] = (short)0;
	       }	  
	       break;
	    }
	    case TYPE_UNSIGNED_SHORT:{
	       unsigned short *sh, *sl;
	       sh = (unsigned short *)Data_Block[handle].dims[ndim].errhi;
	       if(Data_Block[handle].dims[ndim].errasymmetry) sl = (unsigned short *)Data_Block[handle].dims[ndim].errlo;
	       for(i=0;i<ndata;i++){
	          sh[i] = (unsigned short)0;
		  if(Data_Block[handle].dims[ndim].errasymmetry) sl[i] = (unsigned short)0;
	       }	  
	       break;
	    }
	    case TYPE_INT:{
               int *ih, *il;
	       ih = (int *)Data_Block[handle].dims[ndim].errhi;
	       if(Data_Block[handle].dims[ndim].errasymmetry) il = (int *)Data_Block[handle].dims[ndim].errlo;
	       for(i=0;i<ndata;i++){
	          ih[i] = (int)0;
		  if(Data_Block[handle].dims[ndim].errasymmetry) il[i] = (int)0;
	       }	  
	       break;
	    }
	    case TYPE_UNSIGNED_INT:{
               unsigned int *uh, *ul;
	       uh = (unsigned int *)Data_Block[handle].dims[ndim].errhi;
	       if(Data_Block[handle].dims[ndim].errasymmetry) ul = (unsigned int *)Data_Block[handle].dims[ndim].errlo;
	       for(i=0;i<ndata;i++){
	          uh[i] = (unsigned int)0;
		  if(Data_Block[handle].dims[ndim].errasymmetry) ul[i] = (unsigned int)0;
	       }
	       break;
	    }
	    case TYPE_LONG:{
               long *lh, *ll;
	       lh = (long *)Data_Block[handle].dims[ndim].errhi;
	       if(Data_Block[handle].dims[ndim].errasymmetry) ll = (long *)Data_Block[handle].dims[ndim].errlo;
	       for(i=0;i<ndata;i++){
	          lh[i] = (long)0;
		  if(Data_Block[handle].dims[ndim].errasymmetry) ll[i] = (long)0;
	       }
	       break;
	    }
	    case TYPE_UNSIGNED_LONG:{
	       unsigned long *lh, *ll;
	       lh = (unsigned long *)Data_Block[handle].dims[ndim].errhi;
	       if(Data_Block[handle].dims[ndim].errasymmetry) ll = (unsigned long *)Data_Block[handle].dims[ndim].errlo;
	       for(i=0;i<ndata;i++){
	          lh[i] = (unsigned long)0;
		  if(Data_Block[handle].dims[ndim].errasymmetry) ll[i] = (unsigned long)0;
	       }
	       break;
	    }
	    case TYPE_LONG64:{
               long long int *lh, *ll;
	       lh = (long long int *)Data_Block[handle].dims[ndim].errhi;
	       if(Data_Block[handle].dims[ndim].errasymmetry) ll = (long long int *)Data_Block[handle].dims[ndim].errlo;
	       for(i=0;i<ndata;i++){
	          lh[i] = (long long int)0;
		  if(Data_Block[handle].dims[ndim].errasymmetry) ll[i] = (long long int)0;
	       }
	       break;
	    }
	    case TYPE_UNSIGNED_LONG64:{
	       unsigned long long int *lh, *ll;
	       lh = (unsigned long long int *)Data_Block[handle].dims[ndim].errhi;
	       if(Data_Block[handle].dims[ndim].errasymmetry) ll = (unsigned long long int *)Data_Block[handle].dims[ndim].errlo;
	       for(i=0;i<ndata;i++){
	          lh[i] = (unsigned long long int)0;
		  if(Data_Block[handle].dims[ndim].errasymmetry) ll[i] = (unsigned long long int)0;
	       }
	       break;
	    }	    
	    case TYPE_CHAR:{
               char *ch, *cl;
	       ch = (char *)Data_Block[handle].dims[ndim].errhi;
	       if(Data_Block[handle].dims[ndim].errasymmetry) cl = (char *)Data_Block[handle].dims[ndim].errlo;
	       for(i=0;i<ndata;i++){
	          *(ch+i) = ' ';
		  if(Data_Block[handle].dims[ndim].errasymmetry) *(cl+i) = ' ';
	       }
	       break;        
	    }
	    case TYPE_UNSIGNED_CHAR:{
	       unsigned char *ch, *cl;
	       ch = (unsigned char *)Data_Block[handle].dims[ndim].errhi;
	       if(Data_Block[handle].dims[ndim].errasymmetry) cl = (unsigned char *)Data_Block[handle].dims[ndim].errlo;
	       for(i=0;i<ndata;i++){
	          ch[i] = (unsigned char)0;
		  if(Data_Block[handle].dims[ndim].errasymmetry) cl[i] = (unsigned char)0;
	       }
	       break;        
            }
	    case TYPE_DCOMPLEX:{
	       DCOMPLEX *ch, *cl;
	       ch = (DCOMPLEX *)Data_Block[handle].dims[ndim].errhi;
	       if(Data_Block[handle].dims[ndim].errasymmetry) cl = (DCOMPLEX *)Data_Block[handle].dims[ndim].errlo;
	       for(i=0;i<ndata;i++){
	          ch[i].real      = (double)0.0;
		  ch[i].imaginary = (double)0.0;
		  if(Data_Block[handle].dims[ndim].errasymmetry){
		     cl[i].real      = (double)0.0;
		     cl[i].imaginary = (double)0.0;
		  }   
	       }
	       break;        
            }
	    case TYPE_COMPLEX:{
	       COMPLEX *ch, *cl;
	       ch = (COMPLEX *)Data_Block[handle].dims[ndim].errhi;
	       if(Data_Block[handle].dims[ndim].errasymmetry) cl = (COMPLEX *)Data_Block[handle].dims[ndim].errlo;
	       for(i=0;i<ndata;i++){
	          ch[i].real      = (float)0.0;
		  ch[i].imaginary = (float)0.0;
		  if(Data_Block[handle].dims[ndim].errasymmetry){
		     cl[i].real      = (float)0.0;
		     cl[i].imaginary = (float)0.0;
		  }   
	       }
	       break;        
            }		    	    
	 }     	       
         return ((char *)Data_Block[handle].dims[ndim].errhi);		// Errors are Symmetric at this point		
      }   
   }
} 

//!  Returns a pointer to the requested coordinate error data
/**
\param	handle	The data object handle 
\param	ndim	the position of the dimension in the data array - numbering is as data[0][1][2]
\return	a pointer to the data 
*/
char *getIdamDimError(int handle, int ndim){
   int above = 1;
   if(handle < 0 || handle >= Data_Block_Count || ndim < 0 || ndim >= Data_Block[handle].rank) return NULL;
   return(getIdamDimAsymmetricError(handle, ndim, above));
}

void getIdamFloatDimAsymmetricError(int handle, int ndim, int above, float *fp){ 	// Copy Error Data cast as float to User Provided Array
   int i, ndata;
   
   if(handle < 0 || handle >= Data_Block_Count || ndim < 0 || ndim >= Data_Block[handle].rank) return;
   
   ndata = Data_Block[handle].dims[ndim].dim_n;
    
   if(Data_Block[handle].dims[ndim].error_type == TYPE_UNKNOWN) getIdamDimAsymmetricError(handle, ndim, above);		// Create the Error Data prior to Casting 
      
   switch(Data_Block[handle].dims[ndim].error_type){
      case TYPE_UNKNOWN:
	 for(i=0;i<ndata;i++) fp[i] = (float)0.0;	// No Error Data
	 break;	
      case TYPE_FLOAT:
         if(above)
	    memcpy((void *)fp, (void *)Data_Block[handle].dims[ndim].errhi, (size_t) Data_Block[handle].dims[ndim].dim_n*sizeof(float));
	 else
	    if(!Data_Block[handle].dims[ndim].errasymmetry)
	       memcpy((void *)fp, (void *)Data_Block[handle].dims[ndim].errhi, (size_t) Data_Block[handle].dims[ndim].dim_n*sizeof(float));
	    else
	       memcpy((void *)fp, (void *)Data_Block[handle].dims[ndim].errlo, (size_t) Data_Block[handle].dims[ndim].dim_n*sizeof(float));  
	 break;		 
      case TYPE_DOUBLE:{
         double *dp;									// Return Zeros if this data is requested unless Error is Modelled
         if(above)
	    dp = (double *)Data_Block[handle].dims[ndim].errhi;
	 else
	    if(!Data_Block[handle].dims[ndim].errasymmetry)
	       dp = (double *)Data_Block[handle].dims[ndim].errhi;
	    else
	       dp = (double *)Data_Block[handle].dims[ndim].errlo;   
	    for(i=0;i<ndata;i++) fp[i] = (float)dp[i];
	 break;
      }
      case TYPE_SHORT:{
         short *sp;
         if(above)
	    sp = (short *)Data_Block[handle].dims[ndim].errhi;
	 else
	    if(!Data_Block[handle].dims[ndim].errasymmetry)
	       sp = (short *)Data_Block[handle].dims[ndim].errhi;
	    else
	       sp = (short *)Data_Block[handle].dims[ndim].errlo;
	 for(i=0;i<ndata;i++) fp[i] = (float)sp[i];
	 break;
      }
      case TYPE_UNSIGNED_SHORT:{
         unsigned short *sp;
	 if(above)
	    sp = (unsigned short *)Data_Block[handle].dims[ndim].errhi;
	 else
	    if(!Data_Block[handle].dims[ndim].errasymmetry)
	       sp = (unsigned short *)Data_Block[handle].dims[ndim].errhi;
	    else
	       sp = (unsigned short *)Data_Block[handle].dims[ndim].errlo;
	 for(i=0;i<ndata;i++) fp[i] = (float)sp[i];
	 break;
      }
      case TYPE_INT:{
         int *ip;
	 if(above)
	    ip = (int *)Data_Block[handle].dims[ndim].errhi;
	 else
	    if(!Data_Block[handle].dims[ndim].errasymmetry)
	       ip = (int *)Data_Block[handle].dims[ndim].errhi;
	    else
	       ip = (int *)Data_Block[handle].dims[ndim].errlo;
	 for(i=0;i<ndata;i++) fp[i] = (float)ip[i];
	 break;
      }
      case TYPE_UNSIGNED_INT:{
         unsigned int *up;
	 if(above)
	    up = (unsigned int *)Data_Block[handle].dims[ndim].errhi;
	 else
	    if(!Data_Block[handle].dims[ndim].errasymmetry)
	       up = (unsigned int *)Data_Block[handle].dims[ndim].errhi;
	    else
	       up = (unsigned int *)Data_Block[handle].dims[ndim].errlo;
	 for(i=0;i<ndata;i++) fp[i] = (float)up[i];
	 break;
      }
      case TYPE_LONG:{
	 long *lp;
	 if(above)
	    lp = (long *)Data_Block[handle].dims[ndim].errhi;
	 else
	    if(!Data_Block[handle].dims[ndim].errasymmetry)
	       lp = (long *)Data_Block[handle].dims[ndim].errhi;
	    else
	       lp = (long *)Data_Block[handle].dims[ndim].errlo;
	 for(i=0;i<ndata;i++) fp[i] = (float)lp[i];
	 break;
      }
      case TYPE_UNSIGNED_LONG:{
	 unsigned long *lp;
	 if(above)
	    lp = (unsigned long *)Data_Block[handle].dims[ndim].errhi;
	 else
	    if(!Data_Block[handle].dims[ndim].errasymmetry)
	       lp = (unsigned long *)Data_Block[handle].dims[ndim].errhi;
	    else
	       lp = (unsigned long *)Data_Block[handle].dims[ndim].errlo;
	 for(i=0;i<ndata;i++) fp[i] = (float)lp[i];
	 break;
      }
      case TYPE_CHAR:{
	 char * cp;
	 if(above)
	    cp = (char *)Data_Block[handle].dims[ndim].errhi;
	 else
	    if(!Data_Block[handle].dims[ndim].errasymmetry)
	       cp = (char *)Data_Block[handle].dims[ndim].errhi;
	    else
	       cp = (char *)Data_Block[handle].dims[ndim].errlo;
	 for(i=0;i<ndata;i++) fp[i] = (float)cp[i];
	 break;
      }
      case TYPE_UNSIGNED_CHAR:{
	 unsigned char *cp;
	 if(above)
	    cp = (unsigned char *)Data_Block[handle].dims[ndim].errhi;
	 else
	    if(!Data_Block[handle].dims[ndim].errasymmetry)
	       cp = (unsigned char *)Data_Block[handle].dims[ndim].errhi;
	    else
	       cp = (unsigned char *)Data_Block[handle].dims[ndim].errlo;
	 for(i=0;i<ndata;i++) fp[i] = (float)cp[i];
	 break;	         
      }
      case TYPE_DCOMPLEX:{
	 int j=0;
	 DCOMPLEX *cp;
	 if(above)
	    cp = (DCOMPLEX *)Data_Block[handle].dims[ndim].errhi;
	 else
	    if(!Data_Block[handle].dims[ndim].errasymmetry)
	       cp = (DCOMPLEX *)Data_Block[handle].dims[ndim].errhi;
	    else
	       cp = (DCOMPLEX *)Data_Block[handle].dims[ndim].errlo;
	 for(i=0;i<ndata;i++){
	    fp[j++] = (float)cp[i].real;
	    fp[j++] = (float)cp[i].imaginary;
	 }
	 break;	         
      }
      case TYPE_COMPLEX:{
	 int j=0;
	 COMPLEX *cp;
	 if(above)
	    cp = (COMPLEX *)Data_Block[handle].dims[ndim].errhi;
	 else
	    if(!Data_Block[handle].dims[ndim].errasymmetry)
	       cp = (COMPLEX *)Data_Block[handle].dims[ndim].errhi;
	    else
	       cp = (COMPLEX *)Data_Block[handle].dims[ndim].errlo;
	 for(i=0;i<ndata;i++){
	    fp[j++] = (float)cp[i].real;
	    fp[j++] = (float)cp[i].imaginary;
	 }
	 break;	         
      }      
   }
   return ;	
}

//!  Returns coordinate error data cast to single precision
/** The copy buffer must be preallocated and sized for the data type. 
\param	handle	The data object handle 
\param	ndim	the position of the dimension in the data array - numbering is as data[0][1][2]
\param	fp	A \b float pointer to a preallocated data buffer
\return	void 
*/
void getIdamFloatDimError(int handle, int ndim, float *fp){
   int above = 1;
   getIdamFloatDimAsymmetricError(handle, ndim, above, fp);
}

//!  Returns a pointer to the DATA_SYSTEM Meta Data structure
/** A copy of the \b Data_System database table record
\param	handle	The data object handle 
\return	DATA_SYSTEM pointer 
*/
DATA_SYSTEM *getIdamDataSystem(int handle){
   if(handle < 0 || handle >= Data_Block_Count) return NULL;
   return ((DATA_SYSTEM *)Data_Block[handle].data_system);
}
//!  Returns a pointer to the SYSTEM_CONFIG Meta Data structure
/** A copy of the \b system_config database table record
\param	handle	The data object handle 
\return	SYSTEM_CONFIG pointer 
*/
SYSTEM_CONFIG *getIdamSystemConfig(int handle){
   if(handle < 0 || handle >= Data_Block_Count) return NULL;
   return ((SYSTEM_CONFIG *)Data_Block[handle].system_config);
}
//!  Returns a pointer to the DATA_SOURCE Meta Data structure
/** A copy of the \b data_source database table record - the location of data
\param	handle	The data object handle 
\return	DATA_SOURCE pointer 
*/
DATA_SOURCE *getIdamDataSource(int handle){
   if(handle < 0 || handle >= Data_Block_Count) return NULL;
   return ((DATA_SOURCE *)Data_Block[handle].data_source);
}
//!  Returns a pointer to the SIGNAL Meta Data structure
/** A copy of the \b signal database table record  
\param	handle	The data object handle 
\return	SIGNAL pointer 
*/
SIGNAL *getIdamSignal(int handle){
   if(handle < 0 || handle >= Data_Block_Count) return NULL;
   return ((SIGNAL *)Data_Block[handle].signal_rec);
}
//!  Returns a pointer to the SIGNAL_DESC Meta Data structure
/** A copy of the \b signal_desc database table record - a description of the data signal/object  
\param	handle	The data object handle 
\return	SIGNAL_DESC pointer 
*/
SIGNAL_DESC *getIdamSignalDesc(int handle){
   if(handle < 0 || handle >= Data_Block_Count) return NULL;
   return ((SIGNAL_DESC *)Data_Block[handle].signal_desc);
}

//!  Returns a pointer to the File Format string returned in the DATA_SOURCE metadata record
/** Dependent on the server property \b get_meta 
\param	handle	The data object handle 
\return	pointer to the data file format 
*/
char *getIdamFileFormat(int handle){
   if(handle < 0 || handle >= Data_Block_Count) return NULL;
   DATA_SOURCE *data_source = getIdamDataSource(handle);
   if(data_source == NULL) return NULL;
   return (data_source->format);
}    


 
//-----------------------------------------------------------------------------------------------------------
// Various Utilities

void initIdamDataBlock(DATA_BLOCK *str){ initDataBlock(str); return;}
void initIdamRequestBlock(REQUEST_BLOCK *str){ initRequestBlock(str); return;} 
 
int idamDataCheckSum( void *data, int data_n, int type){
   int i, sum=0;   
   switch(type){
      case TYPE_FLOAT:{
         float fsum=0.0;
	 float *dp = (float *)data; 
         for(i=0;i<data_n;i++) if(isfinite(dp[i])) fsum = fsum + dp[i];
	 sum = (int) fsum;
	 if(sum == 0) sum = (int)(1000000.0 * fsum);		// Rescale 
	 break;
      }
      case TYPE_DOUBLE:{
         double fsum=0.0;
	 double *dp = (double *)data; 
         for(i=0;i<data_n;i++) if(isfinite(dp[i])) fsum = fsum + dp[i];
	 sum = (int) fsum;
	 if(sum == 0) sum = (int)(1000000.0 * fsum);		// Rescale 
	 break;
      }
      case TYPE_COMPLEX:{
         float fsum=0.0;
	 COMPLEX *dp = (COMPLEX *)data; 
         for(i=0;i<data_n;i++) if(isfinite(dp[i].real) && isfinite(dp[i].imaginary)) fsum = fsum + dp[i].real + dp[i].imaginary;
	 sum = (int) fsum;
	 if(sum == 0) sum = (int)(1000000.0 * fsum);		// Rescale 
	 break;
      }
      case TYPE_DCOMPLEX:{
         double fsum=0.0;
	 DCOMPLEX *dp = (DCOMPLEX *)data; 
         for(i=0;i<data_n;i++) if(isfinite(dp[i].real) && isfinite(dp[i].imaginary)) fsum = fsum + dp[i].real + dp[i].imaginary;
	 sum = (int) fsum;
	 if(sum == 0) sum = (int)(1000000.0 * fsum);		// Rescale 
	 break;
      }

      case TYPE_CHAR:{
         char *dp = (char *)data; 
         for(i=0;i<data_n;i++) sum = sum + (int) dp[i];
	 break;
      }
      case TYPE_SHORT:{
         short int *dp = (short int *)data; 
         for(i=0;i<data_n;i++) sum = sum + (int) dp[i];
	 break;
      }      
      case TYPE_INT:{
         int *dp = (int *)data; 
         for(i=0;i<data_n;i++) sum = sum + (int) dp[i];
	 break;
      }      
      case TYPE_LONG:{
         long *dp = (long *)data; 
         for(i=0;i<data_n;i++) sum = sum + (int) dp[i];
	 break;
      }      
      case TYPE_LONG64:{
         long long int *dp = (long long int *)data; 
         for(i=0;i<data_n;i++) sum = sum + (int) dp[i];
	 break;
      }      
      case TYPE_UNSIGNED_CHAR:{
         unsigned char *dp = (unsigned char *)data; 
         for(i=0;i<data_n;i++) sum = sum + (int) dp[i];
	 break;
      }
      case TYPE_UNSIGNED_SHORT:{
         unsigned short int *dp = (unsigned short int *)data; 
         for(i=0;i<data_n;i++) sum = sum + (int) dp[i];
	 break;
      }      
      case TYPE_UNSIGNED_INT:{
         unsigned int *dp = (unsigned int *)data; 
         for(i=0;i<data_n;i++) sum = sum + (int) dp[i];
	 break;
      }      
      case TYPE_UNSIGNED_LONG:{
         unsigned long *dp = (unsigned long *)data; 
         for(i=0;i<data_n;i++) sum = sum + (int) dp[i];
	 break;
      }           
      case TYPE_UNSIGNED_LONG64:{
         unsigned long long int *dp = (unsigned long long int *)data; 
         for(i=0;i<data_n;i++) sum = sum + (int) dp[i];
	 break;
      }
      
      default: sum = 0;
   }            
   return sum;
}

int getIdamDataCheckSum(int handle){
   if(handle < 0 || handle >= Data_Block_Count) return 0;
   if(Data_Block[handle].errcode != 0) return 0;
   
   return( idamDataCheckSum((void *)Data_Block[handle].data, Data_Block[handle].data_n, Data_Block[handle].data_type));
} 

int getIdamDimDataCheckSum(int handle, int ndim){
   if(handle < 0 || handle >= Data_Block_Count) return 0;
   if(Data_Block[handle].errcode != 0) return 0;
   if(ndim < 0 || ndim >= Data_Block[handle].rank) return 0;

   return( idamDataCheckSum((void *)Data_Block[handle].dims[ndim].dim, Data_Block[handle].dims[ndim].dim_n, Data_Block[handle].dims[ndim].data_type));
}

