
/*---------------------------------------------------------------
* v1 UDA Plugin: Query the UDA POSTGRES Metadata Catalog
*                Designed for use case where each signal class is recorded but not each signal instance
*                Only the first record identified that satisfies the selection criteria is returned - multiple records raises an error
*                Records within the database must be unique 
*
* Input Arguments:	IDAM_PLUGIN_INTERFACE *idam_plugin_interface
*
* Returns:		0 if the plugin functionality was successful
*			otherwise a Error Code is returned 
*
* Public Functions:
*
*       query	return the database metadata record for a data object
*
* Private Functions:
*
*       get	return the name mapping between an alias or generic name for a data object and its true name or
*               method of data access.
*      
*       connection return the private database connection object for other plugins to reuse 
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
* 26May2017	D.G.Muir	Original Version
*---------------------------------------------------------------------------------------------------------------*/

#include "postgresplugin.h"

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <strings.h>

#include <server/udaServer.h>
#include <clientserver/stringUtils.h>
#include <structures/struct.h>
#include <structures/accessors.h>
#include <clientserver/initStructs.h>
#include <server/getServerEnvironment.h>


#ifndef USE_PLUGIN_DIRECTLY
IDAMERRORSTACK* idamErrorStack;    // Pointer to the Server's Error Stack. Global scope within this plugin library
#endif

static int preventSQLInjection(PGconn *DBConnect, char **from){

// Replace the passed string with an Escaped String
// Free the Original string from Heap
      
   int err = 0;
   size_t fromCount = strlen(*from);
   char *to = (char *)malloc((2*fromCount+1)*sizeof(char));
   PQescapeStringConn(DBConnect, to, *from, fromCount, &err);	 
   if(err != 0){
      if(to != NULL) free((void *)to);
      return 1;
   }
   free((void *)*from);
   *from = to;
   return 0;   
}

// Open the Connection with the PostgreSQL IDAM Database

static PGconn *postgresConnect(){  
   
   char *pghost = getenv("UDA_SQLHOST");			
   char *pgport = getenv("UDA_SQLPORT");
   char *dbname = getenv("UDA_SQLDBNAME");
   char *user   = getenv("UDA_SQLUSER");
   
   char pswrd[9] = "readonly";	// Default for user 'readonly'
   
   char *pgoptions = NULL; 	//"connect_timeout=5";
   char *pgtty = NULL;

   PGconn* DBConnect = NULL;
   
//------------------------------------------------------------- 
// Debug Trace Queries

   UDA_LOG(UDA_LOG_DEBUG, "SQL Connection: host %s\n", pghost);
   UDA_LOG(UDA_LOG_DEBUG, "                port %s\n", pgport);
   UDA_LOG(UDA_LOG_DEBUG, "                db   %s\n", dbname);
   UDA_LOG(UDA_LOG_DEBUG, "                user %s\n", user);

//-------------------------------------------------------------
// Connect to the Database Server

   if(strcmp(user, "readonly") != 0) pswrd[0] = '\0';	// No password - set in the server's .pgpass file

   if ((DBConnect = PQsetdbLogin(pghost, pgport, pgoptions, pgtty, dbname, user, pswrd)) == NULL){
      addIdamError(CODEERRORTYPE, "startSQL", 1, "SQL Server Connect Error");
      PQfinish(DBConnect);     
      return(NULL);
   }
      
   if (PQstatus(DBConnect) == CONNECTION_BAD){
      addIdamError(CODEERRORTYPE, "startSQL", 1, "Bad SQL Server Connect Status");
      PQfinish(DBConnect);
      return(NULL);
   }    

   return(DBConnect);
} 

extern int postgres_query(IDAM_PLUGIN_INTERFACE *idam_plugin_interface){
   int i,err=0,offset;
   char *p;
   
   static short init = 0;

//----------------------------------------------------------------------------------------
// Database Objects

   static PGconn* DBConnect = NULL;
   static short sqlPrivate = 0;				// If the Database connection was opened here, it remains local and open but is not passed back.
   static unsigned short DBType = PLUGINSQLNOTKNOWN;	// The database type
  
//----------------------------------------------------------------------------------------
// Standard v1 Plugin Interface

   DATA_BLOCK *data_block;
   REQUEST_BLOCK *request_block;
   DATA_SOURCE *data_source;
   SIGNAL_DESC *signal_desc;
   //ENVIRONMENT *environment;
   
   //PLUGINLIST *pluginList;	// List of all data reader plugins (internal and external shared libraries)
   
   USERDEFINEDTYPE usertype;
   COMPOUNDFIELD field;
   
   unsigned short housekeeping;
   
    if (idam_plugin_interface->interfaceVersion > THISPLUGIN_MAX_INTERFACE_VERSION) {
        err = 999;
        UDA_LOG(UDA_LOG_ERROR,
                "ERROR postgres_plugin: Plugin Interface Version Unknown to this plugin: Unable to execute the request!\n");
        addIdamError(CODEERRORTYPE, "postgres_plugin", err,
                     "Plugin Interface Version Unknown to this plugin: Unable to execute the request!");
        return err;
    }
      
   idam_plugin_interface->pluginVersion = THISPLUGIN_VERSION;
      
   data_block    = idam_plugin_interface->data_block;
   request_block = idam_plugin_interface->request_block;
   data_source   = idam_plugin_interface->data_source;
   signal_desc   = idam_plugin_interface->signal_desc;
 //environment   = idam_plugin_interface->environment;      
 //pluginList    = idam_plugin_interface->pluginList;           

   USERDEFINEDTYPELIST* userdefinedtypelist = idam_plugin_interface->userdefinedtypelist;
   LOGMALLOCLIST* logmalloclist = idam_plugin_interface->logmalloclist;
      
   housekeeping = idam_plugin_interface->housekeeping;

// Database connection passed in from the server (external)

   if(!sqlPrivate){						// Use External database connection
      DBType = idam_plugin_interface->sqlConnectionType;
      if(DBType == PLUGINSQLPOSTGRES) DBConnect = idam_plugin_interface->sqlConnection;
   }  

// Additional interface components (must be defined at the bottom of the standard data structure)
// Versioning must be consistent with the macro THISPLUGIN_MAX_INTERFACE_VERSION and the plugin registration with the server
      
   //if(idam_plugin_interface->interfaceVersion >= 2){
   // NEW COMPONENTS  
   //}      
        
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
   
   if(housekeeping || !strcasecmp(request_block->function, "reset")){ 
      
      if(!init) return 0;		// Not previously initialised: Nothing to do!
      
// Free Heap & reset counters

      if(sqlPrivate && DBConnect != NULL && DBType == PLUGINSQLPOSTGRES){
 	 PQfinish(DBConnect); 
	 sqlPrivate = 0;
	 DBConnect  = NULL;
 	 DBType     = PLUGINSQLNOTKNOWN;
      }

      init = 0;
       
      return 0;
   }
   
//----------------------------------------------------------------------------------------
// Initialise 
   
   if(!init || !strcasecmp(request_block->function, "init") 
            || !strcasecmp(request_block->function, "initialise")){

// Is there an Open Database Connection? If not then open a private (within scope of this plugin only) connection
   
      if(DBConnect == NULL && (DBType == PLUGINSQLPOSTGRES || DBType == PLUGINSQLNOTKNOWN)){
 
         if((DBConnect = postgresConnect()) == NULL) return(999);
 	 
	 DBType = PLUGINSQLPOSTGRES;
 	 sqlPrivate = 1;			// the connection belongs to this plugin
	 UDA_LOG(UDA_LOG_DEBUG, "postgresplugin: Private regular database connection made.\n");
      } 
            
      init = 1;
   } 

//----------------------------------------------------------------------------------------
// Return the private (local) connection Object
// Not intended for client use ...

   if(!strcasecmp(request_block->function, "connection")){ 
           
      if(sqlPrivate && DBConnect != NULL){
         idam_plugin_interface->sqlConnectionType = PLUGINSQLPOSTGRES;
         idam_plugin_interface->sqlConnection = (void *)DBConnect;
      } else {
         idam_plugin_interface->sqlConnectionType = PLUGINSQLNOTKNOWN;
         idam_plugin_interface->sqlConnection = NULL;
      }
     
      return 0;
   }
            
//----------------------------------------------------------------------------------------
// Plugin Functions 
//----------------------------------------------------------------------------------------
/* 
 The query method is controlled by passed name value pairs. This method and plugin is called explicitly for use cases where this is just another regular plugin resource.

 The default method is called when the plugin has the 'special' role of resolving generic signal names using the IDAM database containing name mappings etc. 
 Neither plugin name nor method name is included in the user request for this specific use case. Therefore, the default method must be identified via the plugin registration 
 process and added to the request.  

 query( [signal|objectName]=objectName, [shot|exp_number|pulse|pulno]=exp_number [,source|objectSource]=objectSource [,type=type] [,sourceClass=sourceClass] [,objectClass=objectClass])
 
 get()	requires the shot number to be passed as the second argument

*/
    
   do {
   
      UDA_LOG(UDA_LOG_DEBUG, "postgresplugin: function called: %s\n", request_block->function);

      if(!strcasecmp(request_block->function, "query") || !strcasecmp(request_block->function, THISPLUGIN_DEFAULT_METHOD)){

         unsigned short int isObjectName=0, isExpNumber=0, isObjectSource=0, isType=0, isSourceClass=0, isObjectClass=0;

 	 char *empty = ""; 
         char *objectName=empty, *objectSource=empty, *type=empty, *sourceClass=empty, *objectClass=empty;
         
         int expNumber=0;


      if(!strcasecmp(request_block->function, "query")){		// Name Value pairs => a regular returned DATA_BLOCK 

         for(i=0;i<request_block->nameValueList.pairCount;i++){
            if(!strcasecmp(request_block->nameValueList.nameValue[i].name, "signal") || !strcasecmp(request_block->nameValueList.nameValue[i].name, "objectName")){
	       isObjectName = 1;
	       objectName = request_block->nameValueList.nameValue[i].value;	
	       continue;
	    }
            if(!strcasecmp(request_block->nameValueList.nameValue[i].name, "source") || !strcasecmp(request_block->nameValueList.nameValue[i].name, "objectSource")){
	       isObjectSource = 1;
	       objectSource = request_block->nameValueList.nameValue[i].value;	
	       continue;
	    }
            if(!strcasecmp(request_block->nameValueList.nameValue[i].name, "shot") || !strcasecmp(request_block->nameValueList.nameValue[i].name, "exp_number") ||
               !strcasecmp(request_block->nameValueList.nameValue[i].name, "pulse") || !strcasecmp(request_block->nameValueList.nameValue[i].name, "pulno")){
	       isExpNumber = 1;
	       expNumber = atoi(request_block->nameValueList.nameValue[i].value);	
	       continue;
	    }
            if(!strcasecmp(request_block->nameValueList.nameValue[i].name, "type")){
	       isType = 1;
	       type = request_block->nameValueList.nameValue[i].value;	
	       continue;
	    }
            if(!strcasecmp(request_block->nameValueList.nameValue[i].name, "sourceClass")){
	       isSourceClass = 1;
	       sourceClass = request_block->nameValueList.nameValue[i].value;	
	       continue;
	    }
            if(!strcasecmp(request_block->nameValueList.nameValue[i].name, "objectClass")){
	       isObjectClass = 1;
	       objectClass = request_block->nameValueList.nameValue[i].value;	
	       continue;
	    }
// Keywords

         }

         } else {	// Default Method: Names and shot or source passed via the standard legacy API arguments => returned SIGNAL_DESC and DATA_SOURCE   

 	       isObjectName = 1;
	       objectName = request_block->signal;
	
	       if(request_block->exp_number > 0){
                  isExpNumber = 1;
	          expNumber = request_block->exp_number;
               }
               if(request_block->tpass[0] != '\0'){
                  isObjectSource = 1;
                  objectSource = request_block->tpass;
               } 	
	       
         }

// Mandatory arguments

         if(!isObjectName){
            UDA_LOG(UDA_LOG_DEBUG, "postgresplugin: No Data Object Name specified\n");
            err =  999;
            addIdamError(CODEERRORTYPE, "postgresplugin", err, "No Data Object Name specified");
            break;
         } 

         if(!isExpNumber && !isObjectSource){
            
            if(request_block->exp_number > 0){		// The expNumber has been specified via the client API's second argument
               isExpNumber = 1;
               expNumber = request_block->exp_number;
            }

            if(request_block->tpass[0] != '\0'){
               isObjectSource = 1;
               objectSource = request_block->tpass;	// The object's source has been specified via the client API's second argument
            } 

            if(!isExpNumber && !isObjectSource){
               UDA_LOG(UDA_LOG_DEBUG, "postgresplugin: No Experiment Number or data source specified\n");
               err =  999;
               addIdamError(CODEERRORTYPE, "postgresplugin", err, "No Experiment Number or data source specified");
               break;
            }
            
         } 

// Query for a specific named data object valid for the shot number range

// Measure performance

   struct timeval tv_start, tv_end; 
   gettimeofday(&tv_start, NULL);     

// All classification and abstraction data should be recorded in single case
// Upper case is chosen as the convention. This is mandatory for the object name
// Lower case matches are also made so do not use Mixed Case for data in the database!

   int nrows, ncols, j, rc=0, cost;

   PGresult* DBQuery = NULL;   
         
   char sql[MAXSQL];

//-------------------------------------------------------------
// Escape SIGNAL and TPASS to protect against SQL Injection

   char *signal = (char *)malloc((strlen(objectName)+1)*sizeof(char));
   strcpy(signal, objectName); 
   strupr(signal);
   if(preventSQLInjection(DBConnect, &signal)){
      if(signal != NULL) free((void *)signal);
      int err = 999;
      addIdamError(CODEERRORTYPE, "sqlSignalDescMap", err, "Unable to Escape the signal name!");
      return(0); 
   }
   
   char *tpass = (char *)malloc((strlen(objectSource)+1)*sizeof(char));
   strcpy(tpass, objectSource);
   strupr(tpass); 
   if(preventSQLInjection(DBConnect, &tpass)){
      if(signal != NULL) free((void *)signal);
      if(tpass != NULL)  free((void *)tpass);
      int err = 999;
      addIdamError(CODEERRORTYPE, "sqlSignalDescMap", err, "Unable to Escape the tpass string!");
      return(0); 
   }	
   	
//-------------------------------------------------------------
// Initialise Returned Structures

   initSignalDesc(signal_desc);
   initDataSource(data_source);      

//-------------------------------------------------------------
// Build SQL

// Signal_Desc records have a uniqueness constraint: signal alias + generic name + name mapping shotrange + source alias name
// Signal_alias or generic names are used by users to identify the correct signal for the given shot number
// Generic names can be shared with multiple signal_name records, but only with non overlapping shot ranges.
//

/*

// Add shot range to table: mapping_shot_range of type int4range
// Add default inclusive range values '[0, 10000000]'
// Add time-stamp range to table: mapping_time_range of type tstzrange (specific time-zone with millisecond time resolution)

// Create ranges with inclusive end values [] rather than exclusive end values (). [ and ) can be mixed in defining the range.
// Change uniqueness constraint to UNIQUE(signal_alias, generic_name, mapping_shot_range, source_alias);
// Change index for range: CREATE INDEX mapping_shot_range_idx ON signal_desc USING gist (mapping_shot_range);
   
ToDo:

The 'tpass' string normally contain the data's sequence number but can pass any set of name-value pairs or other types of directives. These are passed into this plugin via the
GET API's second argument - the data source argument. This is currently unused in the query.

Parameters passed to the plugin as name-value pairs (type, source_alias or sourceClass, signal_class or objectClass) are also not used.        
*/
   if(isExpNumber){

      sprintf(sql, "SELECT type, source_alias, signal_alias, generic_name, signal_name, signal_class FROM signal_desc WHERE "   
                   "signal_alias = '%s' OR generic_name = '%s' AND mapping_shot_range @> %d ",
		   signal, signal, expNumber);

/*
      sprintf(sql, "SELECT type, source_alias, signal_alias, generic_name, signal_name, signal_class FROM signal_desc WHERE "   
                   "signal_alias = '%s' OR generic_name = '%s' AND "						
		   "((range_start=0 AND range_stop=0) OR (range_start=0 AND range_stop>=%d) OR "
		   " (range_stop=0  AND range_start<=%d) OR "
		   " (range_start>0 AND range_start<=%d AND range_stop>0 AND range_stop>=%d)) ",
		   signal, signal, expNumber, expNumber, expNumber, expNumber);
*/
   } else {
      sprintf(sql, "SELECT type, source_alias, signal_alias, generic_name, signal_name, signal_class FROM signal_desc WHERE "   
                   "signal_alias = '%s' OR generic_name = '%s' ",
		   signal, signal);
   }  		   
      
   char *work;
   if(isType){
      work = (char *)malloc((13 + strlen(type))*sizeof(char));
      sprintf(work, "AND type='%s' ", strupr(type));
      strcat(sql, work);  
      free((void *)work);		   
   }
   if(isSourceClass){      		
      work = (char *)malloc((21 + strlen(sourceClass))*sizeof(char));
      sprintf(work, "AND source_alias='%s' ", strupr(sourceClass));  
      strcat(sql, work); 
      free((void *)work);		   
   }
   if(isObjectClass){      		
      work = (char *)malloc((21 + strlen(objectClass))*sizeof(char));
      sprintf(work, "AND signal_class='%s' ", strupr(objectClass));  
      strcat(sql, work); 
      free((void *)work);		   
   }

   if(signal != NULL) free((void *)signal);
   if(tpass != NULL)  free((void *)tpass);
		   		 
//-------------------------------------------------------------
// Test Performance

   UDA_LOG(UDA_LOG_DEBUG, "%s\n",sql);
 
   cost = gettimeofday(&tv_start, NULL);	  		          

//-------------------------------------------------------------
// Execute SQL

   if ((DBQuery = PQexec(DBConnect, sql)) == NULL){
      addIdamError(CODEERRORTYPE, "sqlSignalDescMap", 1, PQresultErrorMessage(DBQuery));
      PQclear(DBQuery);
      return(rc); 
   }	
   
   nrows = PQntuples(DBQuery); 		// Number of Rows
   ncols = PQnfields(DBQuery);		// Number of Columns
   
   cost = gettimeofday(&tv_end, NULL); 
   cost = (int)(tv_end.tv_sec-tv_start.tv_sec)*1000 + (int)(tv_end.tv_usec - tv_start.tv_usec) / 1000;
   tv_start = tv_end; 

   UDA_LOG(UDA_LOG_DEBUG, "+++ POSTGRES Plugin +++\n");
   UDA_LOG(UDA_LOG_DEBUG, "SQL Time: %d (ms)\n",cost);
   UDA_LOG(UDA_LOG_DEBUG, "No. Rows: %d\n",nrows);
   UDA_LOG(UDA_LOG_DEBUG, "No. Cols: %d\n",ncols);
   UDA_LOG(UDA_LOG_DEBUG, "SQL Msg : %s\n",PQresultErrorMessage(DBQuery));  
   
   if(nrows == 0){		// Nothing matched!
      PQclear(DBQuery);
      return(rc);		  
   } 
     
//-------------------------------------------------------------
// Multiple records: Process Exception and return error
//
// nrows == 0 => no match
// nrows == 1 => unambiguous match to signal_alias or generic_name. This is the target record
// nrows >= 2 => ambiguous match => Error.
          
   if(nrows > 1){   
      err = 999;
      addIdamError(CODEERRORTYPE, "sqlSignalDescMap", err, "Ambiguous database entries found! "
                   "Please advise the System Administrator.");
      PQclear(DBQuery);
      return(0); 
   }
            
   if(nrows == 0){   
      err = 999;
      addIdamError(CODEERRORTYPE, "sqlSignalDescMap", err, "No generic signal found!");
      PQclear(DBQuery);
      return(0); 
   }
        
//-------------------------------------------------------------
// Extract information
    
   rc = 1;
   j  = 0; 
   int rowId = 0;     
      
// Signal_Desc fields       
              	 	 
      signal_desc->type = PQgetvalue(DBQuery,rowId,j++)[0]; 
      strcpy(signal_desc->source_alias, PQgetvalue(DBQuery,rowId,j++));      
      strcpy(signal_desc->signal_alias, PQgetvalue(DBQuery,rowId,j++));
      strcpy(signal_desc->generic_name, PQgetvalue(DBQuery,rowId,j++));
      strcpy(signal_desc->signal_name,  PQgetvalue(DBQuery,rowId,j++));
      strcpy(signal_desc->signal_class, PQgetvalue(DBQuery,rowId,j++));
      
      PQclear(DBQuery);  

      if(isExpNumber) sprintf(data_source->path,"%d", expNumber); 
      data_source->type = signal_desc->type;
      strcpy(data_source->source_alias,signal_desc->source_alias);    

   if(err > 0) break;
   
// Return Result: Data Block with the Meta Data structures

         if(!strcasecmp(request_block->function, "query")){

// Create the Returned Structure Definition

            initUserDefinedType(&usertype);			// New structure definition
      
	    strcpy(usertype.name, "POSTGRES_R");		 
            usertype.size = sizeof(POSTGRES_R); 		 
	 	 
	    strcpy(usertype.source, "postgresplugin");
            usertype.ref_id     = 0; 		 
            usertype.imagecount = 0;				// No Structure Image data
            usertype.image      = NULL;	 
            usertype.idamclass  = TYPE_COMPOUND;  
	 
            offset = 0;
	 
	    defineField(&field, "objectName", "The target data object's (abstracted) (signal) name", &offset, SCALARSTRING);
            addCompoundField(&usertype, field);
            defineField(&field, "expNumber", "The target experiment number", &offset, SCALARINT);
            addCompoundField(&usertype, field);	
 	    defineField(&field, "signal_name", "the name of the target data object", &offset, SCALARSTRING);
            addCompoundField(&usertype, field);
 	    defineField(&field, "type", "the type of data object", &offset, SCALARSTRING);
            addCompoundField(&usertype, field);
 	    defineField(&field, "signal_alias", "the alias name of the target data object", &offset, SCALARSTRING);
            addCompoundField(&usertype, field);
 	    defineField(&field, "generic_name", "the generic name of the target data object", &offset, SCALARSTRING);
            addCompoundField(&usertype, field);
 	    defineField(&field, "source_alias", "the source class", &offset, SCALARSTRING);
            addCompoundField(&usertype, field);
 	    defineField(&field, "signal_class", "the data class", &offset, SCALARSTRING);
            addCompoundField(&usertype, field);
 	    defineField(&field, "description", "a description of the data object", &offset, SCALARSTRING);
            addCompoundField(&usertype, field);	 	   
            defineField(&field, "range_start", "validity range starting value", &offset, SCALARINT);
	    addCompoundField(&usertype, field);		      
            defineField(&field, "range_stop", "validity range ending value", &offset, SCALARINT);
	    addCompoundField(&usertype, field);		      
              
            addUserDefinedType(userdefinedtypelist, usertype);	 

// Create the data to be returned

            POSTGRES_R * data = (POSTGRES_R *)malloc(sizeof(POSTGRES_R));
            addMalloc(logmalloclist, (void *)data, 1, sizeof(POSTGRES_R), "POSTGRES_R");

            int lstr = strlen(objectName)+1;
            data->objectName = (char *)malloc(lstr*sizeof(char));
            addMalloc(logmalloclist, (void *)data->objectName, 1, lstr*sizeof(char), "char");
            strcpy(data->objectName, objectName);

            lstr = strlen(signal_desc->signal_name)+1;
            data->signal_name = (char *)malloc(lstr*sizeof(char));
            addMalloc(logmalloclist, (void *)data->signal_name, 1, lstr*sizeof(char), "char");
            strcpy(data->signal_name, signal_desc->signal_name);

            lstr = 2;
            data->type = (char *)malloc(lstr*sizeof(char));
            addMalloc(logmalloclist, (void *)data->type, 1, lstr*sizeof(char), "char");
            data->type[0] = signal_desc->type;
            data->type[1] = '\0';

            lstr = strlen(signal_desc->signal_alias)+1;
            data->signal_alias = (char *)malloc(lstr*sizeof(char));
            addMalloc(logmalloclist, (void *)data->signal_alias, 1, lstr*sizeof(char), "char");
            strcpy(data->signal_alias, signal_desc->signal_alias);

            lstr = strlen(signal_desc->generic_name)+1;
            data->generic_name = (char *)malloc(lstr*sizeof(char));
            addMalloc(logmalloclist, (void *)data->generic_name, 1, lstr*sizeof(char), "char");
            strcpy(data->generic_name, signal_desc->generic_name);

            lstr = strlen(signal_desc->source_alias)+1;
            data->source_alias = (char *)malloc(lstr*sizeof(char));
            addMalloc(logmalloclist, (void *)data->source_alias, 1, lstr*sizeof(char), "char");
            strcpy(data->source_alias, signal_desc->source_alias);

            lstr = strlen(signal_desc->signal_class)+1;
            data->signal_class = (char *)malloc(lstr*sizeof(char));
            addMalloc(logmalloclist, (void *)data->signal_class, 1, lstr*sizeof(char), "char");
            strcpy(data->signal_class, signal_desc->signal_class);

            lstr = strlen(signal_desc->description)+1;
            data->description = (char *)malloc(lstr*sizeof(char));
            addMalloc(logmalloclist, (void *)data->description, 1, lstr*sizeof(char), "char");
            strcpy(data->description, signal_desc->description);

            data->expNumber   = expNumber;
            data->range_start = signal_desc->range_start;
            data->range_stop  = signal_desc->range_stop;

// Return the Data	 
	 	 
            data_block->data_type = TYPE_COMPOUND;
            data_block->rank      = 0;
            data_block->data_n    = 1;
            data_block->data      = (char *)data;	
	 	 
            strcpy(data_block->data_desc, "postgresplugin query result");
            strcpy(data_block->data_label,"");
            strcpy(data_block->data_units,""); 
      
            data_block->opaque_type  = OPAQUE_TYPE_STRUCTURES;
            data_block->opaque_count = 1;
 	    data_block->opaque_block = (void *)findUserDefinedType(userdefinedtypelist, "POSTGRES_R", 0);
	 
         }
	 	 	 
         break;
      } else
            	
//----------------------------------------------------------------------------------------    
// Help: A Description of library functionality
 
      if(!strcasecmp(request_block->function, "help")){
      
	 p = (char *)malloc(sizeof(char)*2*1024);
	 
	 strcpy(p, "\npostgresplugin: Function Names, Syntax, and Descriptions\n\n"
                   "Query the POSTGRES IDAM database for specific instances of a data object by alias or generic name and shot number\n\n"
                   "\tquery( [signal|objectName]=objectName, [shot|exp_number|pulse|pulno]=exp_number [,source|objectSource=objectSource])\n"
                   "\t       [objectClass=objectClass] [,sourceClass=sourceClass] [,type=type])\n\n"
                   "\tobjectName: The alias or generic name of the data object.\n"
                   "\texp_number: The experiment shot number. This may be passed via the UDA client API's second argument.\n"
                   "\tobjectSource: the abstract name of the source. This may be passed via the client API's second argument, either alone or with exp_number [exp_number/objectSource]\n"
                   "\tobjectClass: the name of the data's measurement class, e.g. magnetics\n"
                   "\tsourceClass: the name of the data's source class, e.g. imas\n"
                   "\ttype: the data type classsification, e.g. P for Plugin\n\n"
                   "\tobjectName is a mandatory argument. One or both of exp_number and ObjectSource is also mandatory unless passed via the client API's second argument.\n\n\n");
				 
	 initDataBlock(data_block);
	 
         data_block->rank = 1;
	 data_block->dims = (DIMS *)malloc(data_block->rank * sizeof(DIMS));
	 for(i=0;i<data_block->rank;i++) initDimBlock(&data_block->dims[i]);

	 data_block->data_type = TYPE_STRING;
	 strcpy(data_block->data_desc, "postgresplugin: help = description of this plugin");
	 
	 data_block->data = (char *)p;

	 data_block->dims[0].data_type  = TYPE_UNSIGNED_INT;
	 data_block->dims[0].dim_n      = strlen(p)+1;
	 data_block->dims[0].compressed = 1;
	 data_block->dims[0].dim0   = 0.0;
	 data_block->dims[0].diff   = 1.0;
	 data_block->dims[0].method = 0;
	 	
	 data_block->data_n = data_block->dims[0].dim_n;
	 	 
         strcpy(data_block->data_label,"");
         strcpy(data_block->data_units,"");       
      
         break;
      } else
      
//----------------------------------------------------------------------------------------    
// Standard methods: version, builddate, defaultmethod, maxinterfaceversion 
 
      if(!strcasecmp(request_block->function, "version")){ 
         initDataBlock(data_block);	 
         data_block->data_type = TYPE_INT;
         data_block->rank      = 0;
         data_block->data_n    = 1;
	 int *data = (int *)malloc(sizeof(int));
	 data[0] = THISPLUGIN_VERSION;
	 data_block->data      = (char *)data;	 	 
         strcpy(data_block->data_desc, "Plugin version number");
         strcpy(data_block->data_label,"version");
         strcpy(data_block->data_units,""); 
         break;
      } else	

// Plugin Build Date
 
      if(!strcasecmp(request_block->function, "builddate")){ 
         initDataBlock(data_block);	 
         data_block->data_type = TYPE_STRING;
         data_block->rank      = 0;
         data_block->data_n    = strlen(__DATE__)+1;
	 char *data = (char *)malloc(data_block->data_n*sizeof(char));
	 strcpy(data, __DATE__);
	 data_block->data      = (char *)data;	 	 
         strcpy(data_block->data_desc, "Plugin build date");
         strcpy(data_block->data_label,"date");
         strcpy(data_block->data_units,""); 
         break;
      } else
      
// Plugin Default Method
 
      if(!strcasecmp(request_block->function, "defaultmethod")){ 
         initDataBlock(data_block);	 
         data_block->data_type = TYPE_STRING;
         data_block->rank      = 0;
         data_block->data_n    = strlen(THISPLUGIN_DEFAULT_METHOD)+1;
	 char *data = (char *)malloc(data_block->data_n*sizeof(char));
	 strcpy(data, THISPLUGIN_DEFAULT_METHOD);
	 data_block->data      = (char *)data;	 	 
         strcpy(data_block->data_desc, "Plugin default method");
         strcpy(data_block->data_label,"method");
         strcpy(data_block->data_units,""); 
         break;
      } else  
                  
// Plugin Maximum Interface Version
 
      if(!strcasecmp(request_block->function, "maxinterfaceversion")){ 
         initDataBlock(data_block);	 
         data_block->data_type = TYPE_INT;
         data_block->rank      = 0;
         data_block->data_n    = 1;
	 int *data = (int *)malloc(sizeof(int));
	 data[0] = THISPLUGIN_MAX_INTERFACE_VERSION;
	 data_block->data      = (char *)data;	 	 
         strcpy(data_block->data_desc, "Maximum Interface Version");
         strcpy(data_block->data_label,"version");
         strcpy(data_block->data_units,""); 
         break;
      } else {     	                                          
	                                                                                                                                                                                                                                                                                   
//======================================================================================
// Error ...
                   
         err = 999;      
         addIdamError(CODEERRORTYPE, "postgresplugin", err, "Unknown plugin function requested!");
	 UDA_LOG(UDA_LOG_DEBUG, "postgresplugin - Unknown plugin function requested: [%s]\n", request_block->function);
         break;
      }
        
   } while(0); 

//--------------------------------------------------------------------------------------
// Housekeeping
   
   UDA_LOG(UDA_LOG_DEBUG, "Exit\n");
    
   return err;   
}

