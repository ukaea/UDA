/*---------------------------------------------------------------
* v1 IDAM Plugin: IDAM Client API
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
#include "uda_plugin.h"

#include <stdlib.h>

#include <client/accAPI.h>
#include <clientserver/initStructs.h>
#include <clientserver/stringUtils.h>
#include <client/udaGetAPI.h>
#include <clientserver/udaTypes.h>
#include <clientserver/errorLog.h>
#include <logging/logging.h>
#include <plugins/udaPlugin.h>

#ifndef USE_PLUGIN_DIRECTLY
IDAMERRORSTACK* idamErrorStack;    // Pointer to the Server's Error Stack. Global scope within this plugin library
#endif

extern int UDAPlugin(IDAM_PLUGIN_INTERFACE* idam_plugin_interface)
{
    int err = 0;

    static short init = 0;
    static char oldServerHost[STRING_LENGTH] = "";
    static int oldPort = 0;

//----------------------------------------------------------------------------------------
// Standard v1 Plugin Interface

    unsigned short housekeeping;

    if (idam_plugin_interface->interfaceVersion > THISPLUGIN_MAX_INTERFACE_VERSION) {
        err = 999;
        idamLog(LOG_ERROR,
                "ERROR idamAPIPlugin: Plugin Interface Version Unknown to this plugin: Unable to execute the request!\n");
        addIdamError(&idamerrorstack, CODEERRORTYPE, "idamAPIPlugin", err,
                     "Plugin Interface Version Unknown to this plugin: Unable to execute the request!");
        return err;
    }

    idam_plugin_interface->pluginVersion = THISPLUGIN_VERSION;

    DATA_BLOCK* data_block = idam_plugin_interface->data_block;
    CLIENT_BLOCK* client_block = idam_plugin_interface->client_block;
    REQUEST_BLOCK* request_block = idam_plugin_interface->request_block;
    DATA_SOURCE* data_source = idam_plugin_interface->data_source;
    SIGNAL_DESC* signal_desc = idam_plugin_interface->signal_desc;

    housekeeping = idam_plugin_interface->housekeeping;

    if (housekeeping || !strcasecmp(request_block->function, "reset")) {

        if (!init) return 0;        // Not previously initialised: Nothing to do!

// Resetting all IDAM client properties

        resetIdamProperties();

        putIdamServerHost(oldServerHost);    // Original Host
        putIdamServerPort(oldPort);    // Original Port

// Free Heap & reset counters

        init = 0;

        return 0;
    }

//----------------------------------------------------------------------------------------
// Initialise 

    if (!init || !strcasecmp(request_block->function, "init")
        || !strcasecmp(request_block->function, "initialise")) {

// Default Server Host and Port 

        strcpy(oldServerHost, getIdamServerHost());    // Current Host
        oldPort = getIdamServerPort();            // Current Port

// Resetting all IDAM client properties

        resetIdamProperties();

// Hand over Server IO File Handles to IDAM Client library 

        idamLog(LOG_DEBUG, "Plugin readIdam: Handing over Server File Handles to IDAM Client\n");

        init = 1;
        if (!strcasecmp(request_block->function, "init") || !strcasecmp(request_block->function, "initialise"))
            return 0;
    }

//----------------------------------------------------------------------------------------
// Plugin Functions 
//----------------------------------------------------------------------------------------

    do {

// Help: A Description of library functionality

        if (!strcasecmp(request_block->function, "help")) {
            char* p = (char*) malloc(sizeof(char) * 2 * 1024);

            strcpy(p, "\nidamAPIPlugin: Add Functions Names, Syntax, and Descriptions\n\n");

            initDataBlock(data_block);

            data_block->rank = 1;
            data_block->dims = (DIMS*) malloc(data_block->rank * sizeof(DIMS));

            int i;
            for (i = 0; i < data_block->rank; i++) {
                initDimBlock(&data_block->dims[i]);
            }

            data_block->data_type = TYPE_STRING;
            strcpy(data_block->data_desc, "idamAPIPlugin: help = description of this plugin");

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
// Standard methods: version, builddate, defaultmethod, maxinterfaceversion 

        if (!strcasecmp(request_block->function, "version")) {
            initDataBlock(data_block);
            data_block->data_type = TYPE_INT;
            data_block->rank = 0;
            data_block->data_n = 1;
            int* data = (int*) malloc(sizeof(int));
            data[0] = THISPLUGIN_VERSION;
            data_block->data = (char*) data;
            strcpy(data_block->data_desc, "Plugin version number");
            strcpy(data_block->data_label, "version");
            strcpy(data_block->data_units, "");
            break;
        } else

// Plugin Build Date

        if (!strcasecmp(request_block->function, "builddate")) {
            initDataBlock(data_block);
            data_block->data_type = TYPE_STRING;
            data_block->rank = 0;
            data_block->data_n = strlen(__DATE__) + 1;
            char* data = (char*) malloc(data_block->data_n * sizeof(char));
            strcpy(data, __DATE__);
            data_block->data = (char*) data;
            strcpy(data_block->data_desc, "Plugin build date");
            strcpy(data_block->data_label, "date");
            strcpy(data_block->data_units, "");
            break;
        } else

// Plugin Default Method

        if (!strcasecmp(request_block->function, "defaultmethod")) {
            initDataBlock(data_block);
            data_block->data_type = TYPE_STRING;
            data_block->rank = 0;
            data_block->data_n = strlen(THISPLUGIN_DEFAULT_METHOD) + 1;
            char* data = (char*) malloc(data_block->data_n * sizeof(char));
            strcpy(data, THISPLUGIN_DEFAULT_METHOD);
            data_block->data = (char*) data;
            strcpy(data_block->data_desc, "Plugin default method");
            strcpy(data_block->data_label, "method");
            strcpy(data_block->data_units, "");
            break;
        } else

// Plugin Maximum Interface Version

        if (!strcasecmp(request_block->function, "maxinterfaceversion")) {
            initDataBlock(data_block);
            data_block->data_type = TYPE_INT;
            data_block->rank = 0;
            data_block->data_n = 1;
            int* data = (int*) malloc(sizeof(int));
            data[0] = THISPLUGIN_MAX_INTERFACE_VERSION;
            data_block->data = (char*) data;
            strcpy(data_block->data_desc, "Maximum Interface Version");
            strcpy(data_block->data_label, "version");
            strcpy(data_block->data_units, "");
            break;
        } else


//---------------------------------------------------------------------------------------- 
//---------------------------------------------------------------------------------------- 
// API

        if (!strcasecmp(request_block->function, "get")) {

// Version 6 structures

            struct OLD_CLIENT_BLOCK {

                int version;
                int pid;            // Client Application process id
                char uid[STRING_LENGTH];    // Who the Client is

                // Server properties set by the client

                int timeout;        // Server Shutdown after this time (minutes) if no data request
                int compressDim;    // Enable Compression of the Dimensional Data?

                unsigned int clientFlags;    // client defined properties passed via bit flags
                int altRank;            // Specify the rank of the alternative signal/source to be used

                int get_nodimdata;    // Don't send Dimensional Data: Send an index only.
                int get_timedble;    // Return Time Dimension Data in Double Precision if originally compressed
                int get_dimdble;    // Return all Dimensional Data in Double Precision
                int get_datadble;    // Return Data in Double Precision

                int get_bad;        // Return Only Data with Bad Status value
                int get_meta;        // Return Meta Data associated with Signal
                int get_asis;        // Return data as Stored in data Archive
                int get_uncal;        // Disable Calibration Correction
                int get_notoff;    // Disable Timing Offset Correction
                int get_scalar;    // Reduce rank from 1 to 0 (Scalar) if dimensional data are all zero
                int get_bytes;        // Return Data as Bytes or Integers without applying the signal's ADC Calibration Data

                unsigned int privateFlags;    // set of private flags used to communicate server to server

            };
            typedef struct OLD_CLIENT_BLOCK OLD_CLIENT_BLOCK;


            struct OLD_DATA_BLOCK {
                int handle;
                int errcode;
                int source_status;
                int signal_status;
                int rank;
                int order;
                int data_type;

                int error_type;
                int error_model;        // Identify the Error Model
                int errasymmetry;        // Flags whether or not error data are asymmetrical
                int error_param_n;        // the Number of Model Parameters

                int data_n;
                char* data;
                char* synthetic;        // Synthetic Data Array used in Client Side Error/Monte-Carlo Modelling

                char* errhi;            // Error Array (Errors above the line: data + error)
                char* errlo;            // Error Array (Errors below the line: data - error)
                float errparams[MAXERRPARAMS];        // the array of model parameters

                char data_units[STRING_LENGTH];
                char data_label[STRING_LENGTH];
                char data_desc[STRING_LENGTH];

                char error_msg[STRING_LENGTH];

                DIMS* dims;
                DATA_SYSTEM* data_system;
                SYSTEM_CONFIG* system_config;
                DATA_SOURCE* data_source;
                SIGNAL* signal_rec;
                SIGNAL_DESC* signal_desc;

                OLD_CLIENT_BLOCK client_block;    // Used to pass properties into legacy data reader plugins - ignore!

                int opaque_type;        // Identifies the Data Structure Type;
                int opaque_count;        // Number of Instances of the Data Structure;
                void* opaque_block;        // Opaque pointer to Hierarchical Data Structures
            };
            typedef struct OLD_DATA_BLOCK OLD_DATA_BLOCK;


            int i, err = 0;
            int handle = 0;
            int newPort = 0;
            char* p;

            char signal[2 * MAXNAME + 2];
            char source[2 * MAXNAME + 2];

/* ----------------------------------------------------------------------
Notes: there are three pathways depending on the request pattern

1> request is the result of an SQL query against the metadata catalog
	if the primary key fields of the data_source and signal_desc structures are non zero
2> request is a device redirect or a server protocol 
	path = server:port/source, server=""
3> request is a function call
	path = "", name-value pair arguments, server=""
4> request is a server call
	server=server:port, path=source
*/

// Identify execution pathway

            int pathway = 0;

            if (data_source->source_id > 0 && signal_desc->signal_desc_id > 0) {
                pathway = 1;
            } else if (request_block->path[0] != '\0' && request_block->server[0] == '\0') {
                pathway = 2;
            } else if (request_block->path[0] == '\0' && request_block->server[0] == '\0') {
                pathway = 3;
            } else if (request_block->server[0] != '\0') {    // Source URL may be an Null string
                pathway = 4;
            } else {
                err = 999;
                idamLog(LOG_ERROR,
                        "ERROR idamAPIPlugin: Execution pathway not recognised: Unable to execute the request!\n");
                addIdamError(&idamerrorstack, CODEERRORTYPE, "idamAPIPlugin", err,
                             "Execution pathway not recognised: Unable to execute the request!");
                break;
            }

//----------------------------------------------------------------------
// Private Flags, User Specified Flags and Properties for the Remote Server

            resetIdamPrivateFlag(PRIVATEFLAG_FULLRESET);
            setIdamPrivateFlag(
                    PRIVATEFLAG_XDRFILE);        // Ensure Hierarchical Data are passed as an opaque object/file

// This fails if the legacy IDAM plugin is called by a server in the forward chain and it set marked a 'private' 
// For IMAS development, this has been disabled  
            //if(environment.external_user) setIdamPrivateFlag(PRIVATEFLAG_EXTERNAL);	// Maintain external user status

// Set Userid

// ... to be implemented   

// Set Properties

            if (client_block->get_nodimdata) setIdamProperty("get_nodimdata");
            if (client_block->get_timedble) setIdamProperty("get_timedble");
            if (client_block->get_dimdble) setIdamProperty("get_dimdble");
            if (client_block->get_datadble) setIdamProperty("get_datadble");

            if (client_block->get_bad) setIdamProperty("get_bad");
            if (client_block->get_meta) setIdamProperty("get_meta");
            if (client_block->get_asis) setIdamProperty("get_asis");
            if (client_block->get_uncal) setIdamProperty("get_uncal");
            if (client_block->get_notoff) setIdamProperty("get_notoff");
            if (client_block->get_scalar) setIdamProperty("get_scalar");
            if (client_block->get_bytes) setIdamProperty("get_bytes");

// Timeout ...

// AltRank ...

// Client Flags ...  

            resetIdamClientFlag(CLIENTFLAG_FULLRESET);
            setIdamClientFlag(client_block->clientFlags);

// Client application provenance

            //putIdamClientDOI(client_block->DOI);
            //putIdamClientOSName(client_block->OSName);

// Client authentication x509 certificate

//----------------------------------------------------------------------
// Signal/Data Object & Data Source Details from the IDAM Database records

// Very primitive: Need to replicate fully the idamGetAPI arguments from the database  

            if (pathway == 1) {    // Request via the Database

                sprintf(signal, "%s::%s", data_source->archive, signal_desc->signal_name);
                sprintf(source, "%s::%d", data_source->device_name, data_source->exp_number);

                if (data_source->server[0] != '\0') {
                    if ((p = strstr(data_source->server, ":")) ==
                        NULL) {        // look for a port number in the server name
                        p = strstr(data_source->server, " ");
                    }
                    if (p != NULL) {
                        p[0] = '\0';
                        if (strcasecmp(oldServerHost, data_source->server) != 0) {
                            strcpy(oldServerHost, data_source->server);
                            putIdamServerHost(data_source->server);
                        }
                        if (IsNumber(&p[1])) {
                            newPort = atoi(&p[1]);
                            if (newPort != oldPort) {
                                putIdamServerPort(newPort);
                                oldPort = newPort;
                            }
                        } else {
                            err = 999;
                            addIdamError(&idamerrorstack, CODEERRORTYPE, "idamAPIPlugin", err,
                                         "The Server Port must be an Integer Number passed "
                                                 "using the formats 'server:port' or 'server port'");
                            return err;
                        }
                    } else {
                        if (strcasecmp(oldServerHost, data_source->server) != 0) {
                            strcpy(oldServerHost, data_source->server);
                            putIdamServerHost(data_source->server);
                        }
                    }
                } else {
                    err = 999;
                    addIdamError(&idamerrorstack, CODEERRORTYPE, "idamAPIPlugin", err, "No Server has been specified!");
                    return err;
                }

                idamLog(LOG_DEBUG, "idamAPIPlugin: Idam Server Host for Idam Plugin %s\n", data_source->server);
                idamLog(LOG_DEBUG, "idamAPIPlugin: Idam Server Port for Idam Plugin %d\n", newPort);
                idamLog(LOG_DEBUG, "idamAPIPlugin: Calling idamGetAPI API (Database based Request)\n");
                idamLog(LOG_DEBUG, "idamAPIPlugin: Signal: %s\n", signal);
                idamLog(LOG_DEBUG, "idamAPIPlugin: Source: %s\n", source);

                handle = idamGetAPI(signal, source);

            } else

//----------------------------------------------------------------------
// Device redirect or server protocol 

            if (pathway == 2) {
                strcpy(request_block->server, request_block->path);        // Extract the Server Name and Port
                if ((p = strchr(request_block->server, '/')) !=
                    NULL) {        // Isolate the Server from the source server:port/source
                    p[0] = '\0';                            // Break the String (work)
                    strcpy(source, p + 1);                // Extract the Source URL Argument
                } else {
                    err = 999;
                    addIdamError(&idamerrorstack, CODEERRORTYPE, "idamAPIPlugin", err,
                                 "The Remote Server Data Source specified does not comply with the naming model: serverHost:port/sourceURL");
                    break;
                }

                if ((p = strstr(request_block->server, ":")) ==
                    NULL) {        // look for a port number in the server name
                    p = strstr(request_block->server, " ");
                }
                if (p != NULL) {
                    p[0] = '\0';
                    if (strcasecmp(oldServerHost, request_block->server) != 0) {
                        strcpy(oldServerHost, request_block->server);
                        putIdamServerHost(request_block->server);    // different host name?
                    }
                    if (IsNumber(&p[1])) {
                        newPort = atoi(&p[1]);
                        if (newPort != oldPort) {
                            putIdamServerPort(newPort);
                            oldPort = newPort;
                        }
                    } else {
                        err = 999;
                        addIdamError(&idamerrorstack, CODEERRORTYPE, "idamAPIPlugin", err,
                                     "The Server Port must be an Integer Number passed "
                                             "using the format 'server:port'  or 'server port'");
                        break;
                    }
                } else {
                    if (strcasecmp(oldServerHost, request_block->server) != 0) {
                        strcpy(oldServerHost, request_block->server);
                        putIdamServerHost(request_block->server);
                    }
                }

                idamLog(LOG_DEBUG, "idamAPIPlugin: Idam Server Host for Idam Plugin %s\n", request_block->server);
                idamLog(LOG_DEBUG, "idamAPIPlugin: Idam Server Port for Idam Plugin %d\n", newPort);
                idamLog(LOG_DEBUG,
                        "idamAPIPlugin: Calling idamGetAPI API (Device redirect or server protocol based Request)\n");
                idamLog(LOG_DEBUG, "idamAPIPlugin: Signal: %s\n", request_block->signal);
                idamLog(LOG_DEBUG, "idamAPIPlugin: Source: %s\n", source);

                handle = idamGetAPI(request_block->signal, source);

            } else

//----------------------------------------------------------------------     
// Function library

            if (pathway == 3) {

                char* host = NULL;
                unsigned short isHost = findStringValue(&request_block->nameValueList, &host, "host");

                char* signal = NULL;
                unsigned short isSignal = findStringValue(&request_block->nameValueList, &signal, "signal");

                char* source = NULL;
                unsigned short isSource = findStringValue(&request_block->nameValueList, &source, "source");

                unsigned short isPort = findIntValue(&request_block->nameValueList, &newPort, "port");

// Set host and port

                if (isHost && strcasecmp(oldServerHost, host) != 0) {
                    strcpy(oldServerHost, host);
                    putIdamServerHost(host);
                }
                if (isPort && oldPort != newPort) {
                    oldPort = newPort;
                    putIdamServerPort(newPort);
                }

                idamLog(LOG_DEBUG, "idamAPIPlugin: Idam Server Host for Idam Plugin %s\n", host);
                idamLog(LOG_DEBUG, "idamAPIPlugin: Idam Server Port for Idam Plugin %d\n", newPort);
                idamLog(LOG_DEBUG, "idamAPIPlugin: Calling idamGetAPI API (plugin library method based Request)\n");

                if (isSignal && isSource) {
                    idamLog(LOG_DEBUG, "idamAPIPlugin: Signal: %s\n", signal);
                    idamLog(LOG_DEBUG, "idamAPIPlugin; Source: %s\n", source);
                    handle = idamGetAPI(signal, source);
                } else if (isSignal && !isSource) {
                    idamLog(LOG_DEBUG, "idamAPIPlugin: Signal: %s\n", signal);
                    idamLog(LOG_DEBUG, "idamAPIPlugin; Source: %s\n", request_block->source);
                    handle = idamGetAPI(signal, request_block->source);
                } else {
                    err = 999;
                    addIdamError(&idamerrorstack, CODEERRORTYPE, "idamAPIPlugin", err,
                                 "A data object (signal) has not been specified!");
                    break;
                }
            } else

//----------------------------------------------------------------------
// Server protocol 

            if (pathway == 4) {

                strcpy(source, request_block->file);                // The Source URL Argument

                if ((p = strstr(request_block->server, ":")) ==
                    NULL) {        // look for a port number in the server name
                    p = strstr(request_block->server, " ");
                }

                if (p != NULL) {                            // look for a port number in the server name
                    p[0] = '\0';                        // Split
                    if (strcasecmp(oldServerHost, request_block->server) != 0) {    // Different Hosts?
                        strcpy(oldServerHost, request_block->server);        // Preserve
                        putIdamServerHost(request_block->server);        // Change to a different host name
                    }
                    if (IsNumber(&p[1])) {
                        newPort = atoi(&p[1]);
                        if (newPort != oldPort) {                    // Different Ports?
                            putIdamServerPort(newPort);
                            oldPort = newPort;
                        }
                    } else {
                        err = 999;
                        addIdamError(&idamerrorstack, CODEERRORTYPE, "idamAPIPlugin", err,
                                     "The Server Port must be an Integer Number passed "
                                             "using the format 'server:port'  or 'server port'");
                        break;
                    }
                } else {                            // No port number passed
                    if (strcasecmp(oldServerHost, request_block->server) != 0) {    // Different Hosts?
                        strcpy(oldServerHost, request_block->server);
                        putIdamServerHost(request_block->server);
                    }
                }

                idamLog(LOG_DEBUG, "idamAPIPlugin: Idam Server Host for Idam Plugin %s\n", request_block->server);
                idamLog(LOG_DEBUG, "idamAPIPlugin: Idam Server Port for Idam Plugin %d\n", newPort);
                idamLog(LOG_DEBUG, "idamAPIPlugin: Calling idamGetAPI API (Server protocol based Request)\n");
                idamLog(LOG_DEBUG, "idamAPIPlugin: Signal: %s\n", request_block->signal);
                idamLog(LOG_DEBUG, "idamAPIPlugin: Source: %s\n", source);

                handle = idamGetAPI(request_block->signal, source);

            }


            resetIdamPrivateFlag(PRIVATEFLAG_FULLRESET);
            resetIdamClientFlag(CLIENTFLAG_FULLRESET);

//---------------------------------------------------------------------- 
// Test for Errors: Close Socket and Free heap

            idamLog(LOG_DEBUG, "idamAPIPlugin:Returned from idamGetAPI API: handle = %d, error code = %d\n", handle,
                    getIdamErrorCode(handle));

            if (handle < 0) {
                concatIdamError(*getIdamServerErrorStack(), idamErrorStack);    // Combine error stacks
                addIdamError(&idamerrorstack, CODEERRORTYPE, "idamAPIPlugin",
                             getIdamServerErrorStackRecordCode(0), getIdamServerErrorStackRecordMsg(0));
                return (abs(handle));
            } else {
                if ((err = getIdamErrorCode(handle)) != 0) {
                    addIdamError(&idamerrorstack, CODEERRORTYPE, "idamAPIPlugin", err,
                                 (char*) getIdamErrorMsg(handle));
                    return err;
                }
            }

//----------------------------------------------------------------------
// Copy the Data Block - method is version dependent
// All xml defined actions on the data will have been executed
// data_source & signal_desc records may will be different to those handed in across the interface
// 1> handed in: records how this plugin was identified 
// 2> handed out: records what data were returned
// In the provenance context 2> is more important (only one record pair can be returned to the application) 

// Check the originals have no XML action definitions before replacement
// Why should a plugin have this concern? 

            int version = getIdamClientVersion();

            if (version == 7) {

                *data_block = *getIdamDataBlock(handle);        // This should contain everything!

            } else {                        // use abstraction functions

// Straight structure mapping causes potential problems when the client library uses different versions 
// of the DATA_BLOCK structure
// or its component structure CLIENT_BLOCK 
// or the initialisation is incomplete!
// Write the structure components element by element! (Ignore the CLIENT_BLOCK component)

                DATA_BLOCK db;
                initDataBlock(&db);

                OLD_DATA_BLOCK* odb = (OLD_DATA_BLOCK*) getIdamDataBlock(handle);

                db.handle = odb->handle;
                db.errcode = odb->errcode;
                db.source_status = odb->source_status;
                db.signal_status = odb->signal_status;
                db.rank = odb->rank;
                db.order = odb->order;
                db.data_type = odb->data_type;

                db.error_type = odb->error_type;
                db.error_model = odb->error_model;
                db.errasymmetry = odb->errasymmetry;
                db.error_param_n = odb->error_param_n;

                db.data_n = odb->data_n;
                db.data = odb->data;
                db.synthetic = odb->synthetic;

                db.errhi = odb->errhi;
                db.errlo = odb->errlo;
                for (i = 0; i < MAXERRPARAMS; i++) db.errparams[i] = odb->errparams[i];

                strcpy(db.data_units, odb->data_units);
                strcpy(db.data_label, odb->data_label);
                strcpy(db.data_desc, odb->data_desc);
                strcpy(db.error_msg, odb->error_msg);

                db.dims = odb->dims;        // These have not changed between versions 6 & 7
                db.data_system = odb->data_system;
                db.system_config = odb->system_config;
                db.data_source = odb->data_source;
                db.signal_rec = odb->signal_rec;
                db.signal_desc = odb->signal_desc;

                db.opaque_type = odb->opaque_type;
                db.opaque_count = odb->opaque_count;
                db.opaque_block = odb->opaque_block;

                *data_block = db;
            }

// Replace provenance metadata records

            if (client_block->get_meta && strlen(data_source->xml) == 0 && strlen(signal_desc->xml) == 0) {
                *data_source = *(data_block->data_source);
                *signal_desc = *(data_block->signal_desc);
            }

            idamLog(LOG_DEBUG, "idamAPIPlugin: Exit\n");

//----------------------------------------------------------------------
// If the Data are Hierarchical, then necessary to forward the xdr file
//
// List of structures from external server may conflict with local definitions
//	Don't use local definitions
//
// No access to malloc log within client
//
// Data received is a Data Tree. This would need to be restructured - i.e., pointer extracted from 
// structure SARRAY (may be different to local SARRAY!)
//	Don't pass a data tree - use an XDR file instead. 
//	Required if http is to be adopted as middleware protocol
// 	Relay everything from the external server back to the client without interpretation. 
//
// Namespace issues: Both the Client and the Server use the same functions to Query. The PRE_LOAD requirement of MDS+
// causes the IDAM client library to be loaded ahead of the server library: Result confusion and seg fault errors.
// Need to add unique name component to IDAM client server to provide namespace separation.
// Prepare a reduced set of external symbols for the client library attached to the server!  

// Structured data are best served as serialised objects - no messy heap logs to free
// Data should be freed with a 'reset' method call after the data have been transmitted
// Add a new 'freeImmediate' plugin property that causes a heap free without other state changes, e.g. socket  


// Problem tests: 115, 118, 124, 141, 142 	

//----------------------------------------------------------------------

            break;
        } else {

//======================================================================================
// Error ...

            err = 999;
            addIdamError(&idamerrorstack, CODEERRORTYPE, "idamAPIPlugin", err, "Unknown function requested!");
            break;
        }

    } while (0);

//--------------------------------------------------------------------------------------
// Housekeeping

    return err;
}
