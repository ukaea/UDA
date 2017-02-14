/*---------------------------------------------------------------
* v1 IDAM Plugin: Standardised wrapper code around plugin functionality
*
* Input Arguments:	IDAM_PLUGIN_INTERFACE *idam_plugin_interface
*
* Returns:		0 if read was successful
*			otherwise a Error Code is returned 
*
* Calls			freeDataBlock	to free Heap memory if an Error Occurs
*
* Standard functionality:
*
*	help	a description of what this plugin does together with a list of functions available
*	reset	frees all previously allocated heap, closes file handles and resets all static parameters.
*		This has the same functionality as setting the housekeeping directive in the plugin interface
*		data structure to TRUE (1)
*	init	Initialise the plugin: read all required data and process. Retain staticly for
*		future reference.
*---------------------------------------------------------------------------------------------------------------*/
#include "idamServerHelp.h"

#include <stdlib.h>
#include <strings.h>

#include <server/idamServer.h>
#include <clientserver/initStructs.h>
#include <structures/struct.h>
#include <structures/accessors.h>
#include <clientserver/idamErrorLog.h>
#include <logging/idamLog.h>
#include <plugins/idamPlugin.h>

int idamServerHelp(IDAM_PLUGIN_INTERFACE* idam_plugin_interface)
{
    int i, err, offset;
    static short init = 0;
    char* p;

//----------------------------------------------------------------------------------------
// Standard v1 Plugin Interface

    DATA_BLOCK* data_block;
    REQUEST_BLOCK* request_block;

    PLUGINLIST* pluginList;    // List of all data reader plugins (internal and external shared libraries)

    USERDEFINEDTYPE usertype;
    COMPOUNDFIELD field;

    unsigned short housekeeping;

    if (idam_plugin_interface->interfaceVersion > THISPLUGIN_MAX_INTERFACE_VERSION) {
        err = 999;
        idamLog(LOG_ERROR,
                "ERROR Help: Plugin Interface Version Unknown to this plugin: Unable to execute the request!\n");
        addIdamError(&idamerrorstack, CODEERRORTYPE, "Help", err,
                     "Plugin Interface Version Unknown to this plugin: Unable to execute the request!");
        return err;
    }

    idam_plugin_interface->pluginVersion = THISPLUGIN_VERSION;

    data_block = idam_plugin_interface->data_block;
    request_block = idam_plugin_interface->request_block;

    pluginList = idam_plugin_interface->pluginList;

    housekeeping = idam_plugin_interface->housekeeping;

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

    if (housekeeping || !strcasecmp(request_block->function, "reset")) {

        if (!init) return 0;        // Not previously initialised: Nothing to do!
        init = 0;
        return 0;
    }

//----------------------------------------------------------------------------------------
// Initialise 

    if (!init || !strcasecmp(request_block->function, "init")
        || !strcasecmp(request_block->function, "initialise")) {

        init = 1;
        if (!strcasecmp(request_block->function, "init") || !strcasecmp(request_block->function, "initialise"))
            return 0;
    }

//----------------------------------------------------------------------------------------
// Name value pairs and keywords 
//----------------------------------------------------------------------------------------


//----------------------------------------------------------------------------------------
// Data Source
//----------------------------------------------------------------------------------------

    err = 0;

    do {        // Error Trap

//----------------------------------------------------------------------------------------
// Plugin Functions 
//----------------------------------------------------------------------------------------

// Help: A Description of library functionality

        if (!strcasecmp(request_block->function, "help") || request_block->function[0] == '\0') {

            p = (char*) malloc(sizeof(char) * 2 * 1024);

            strcpy(p, "\nHelp\tList of HELP plugin functions:\n\n"
                    "services()\tReturns a list of available services with descriptions\n"
                    "ping()\t\tReturn the Local Server Time in seconds and microseonds\n"
                    "servertime()\tReturn the Local Server Time in seconds and microseonds\n\n");

            initDataBlock(data_block);

            data_block->rank = 1;
            data_block->dims = (DIMS*) malloc(data_block->rank * sizeof(DIMS));
            for (i = 0; i < data_block->rank; i++) initDimBlock(&data_block->dims[i]);

            data_block->data_type = TYPE_STRING;
            strcpy(data_block->data_desc, "Help help = description of this plugin");

            data_block->data = (char*) p;

            data_block->dims[0].data_type = TYPE_UNSIGNED;
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

// Ping: Timing

        if (!strcasecmp(request_block->function, "ping") ||
            !strcasecmp(request_block->function, "servertime")) {

            struct timeval serverTime;        // Local time in microseconds
            gettimeofday(&serverTime, NULL);

// define the returned data structure

            struct HELP_PING {
                unsigned int seconds;    // Server time in seconds
                unsigned int microseconds;    // Server time in microseconds
            };
            typedef struct HELP_PING HELP_PING;

            initUserDefinedType(&usertype);            // New structure definition
            initCompoundField(&field);

            strcpy(usertype.name, "HELP_PING");
            strcpy(usertype.source, "idamServerHelp");
            usertype.ref_id = 0;
            usertype.imagecount = 0;                // No Structure Image data
            usertype.image = NULL;
            usertype.size = sizeof(HELP_PING);        // Structure size
            usertype.idamclass = TYPE_COMPOUND;

            offset = 0;
            defineField(&field, "seconds", "Server time in seconds from the epoch start", &offset, SCALARUINT);
            addCompoundField(&usertype, field);
            defineField(&field, "microseconds", "Server inter-second time in microseconds", &offset, SCALARUINT);
            addCompoundField(&usertype, field);

            addUserDefinedType(userdefinedtypelist, usertype);

// assign the returned data structure

            HELP_PING* data = (HELP_PING*) malloc(sizeof(HELP_PING));
            addMalloc((void*) data, 1, sizeof(HELP_PING), "HELP_PING");        // Register

            data->seconds = (unsigned int) serverTime.tv_sec;
            data->microseconds = (unsigned int) serverTime.tv_usec;

// return to the client	 

            initDataBlock(data_block);

            data_block->data_type = TYPE_COMPOUND;
            data_block->rank = 0;
            data_block->data_n = 1;
            data_block->data = (char*) data;

            strcpy(data_block->data_desc, "Local IDAM server time");
            strcpy(data_block->data_label, "servertime");
            strcpy(data_block->data_units, "");

            data_block->opaque_type = OPAQUE_TYPE_STRUCTURES;
            data_block->opaque_count = 1;
            data_block->opaque_block = (void*) findUserDefinedType("HELP_PING", 0);

            break;
        } else

//====================================================================================== 
// Plugin functionality 

        if (!strcasecmp(request_block->function, "services")) {

            int i, j, count;
            unsigned short target;

            char* line = "\n------------------------------------------------------\n";

// Document is a single block of chars

            char* rec = (char*) malloc(STRING_LENGTH * sizeof(char));
            char* doc = (char*) malloc(20 * STRING_LENGTH * sizeof(char));
            doc[0] = '\0';

// Total Number of registered plugins available

            count = 0;
            for (i = 0; i < pluginList->count; i++)
                if (pluginList->plugin[i].status == PLUGINOPERATIONAL &&
                    (pluginList->plugin[i].private == PLUGINPUBLIC ||
                     (pluginList->plugin[i].private == PLUGINPRIVATE && !environment.external_user)))
                    count++;

            sprintf(rec, "\nTotal number of registered plugins available: %d\n", count);
            strcat(doc, rec);

            for (j = 0; j < 5; j++) {
                count = 0;
                strcat(doc, line);
                switch (j) {
                    case 0: {
                        target = PLUGINFILE;

                        for (i = 0; i < pluginList->count; i++)
                            if (pluginList->plugin[i].class == target &&
                                pluginList->plugin[i].status == PLUGINOPERATIONAL &&
                                (pluginList->plugin[i].private == PLUGINPUBLIC ||
                                 (pluginList->plugin[i].private == PLUGINPRIVATE && !environment.external_user)) &&
                                pluginList->plugin[i].format[0] != '\0' && pluginList->plugin[i].extension[0] != '\0')
                                count++;

                        sprintf(rec, "\nNumber of plugins for data file formats: %d\n\n", count);
                        strcat(doc, rec);

                        for (i = 0; i < pluginList->count; i++)
                            if (pluginList->plugin[i].class == target &&
                                pluginList->plugin[i].status == PLUGINOPERATIONAL &&
                                (pluginList->plugin[i].private == PLUGINPUBLIC ||
                                 (pluginList->plugin[i].private == PLUGINPRIVATE && !environment.external_user)) &&
                                pluginList->plugin[i].format[0] != '\0' && pluginList->plugin[i].extension[0] != '\0') {
                                sprintf(rec, "File format:\t\t%s\n", pluginList->plugin[i].format);
                                strcat(doc, rec);
                                sprintf(rec, "Filename extension:\t%s\n", pluginList->plugin[i].extension);
                                strcat(doc, rec);
                                sprintf(rec, "Description:\t\t%s\n", pluginList->plugin[i].desc);
                                strcat(doc, rec);
                                sprintf(rec, "Example API call:\t%s\n\n", pluginList->plugin[i].example);
                                strcat(doc, rec);
                            }
                        break;
                    }
                    case 1: {
                        target = PLUGINFUNCTION;
                        for (i = 0; i < pluginList->count; i++)
                            if (pluginList->plugin[i].class == target &&
                                pluginList->plugin[i].status == PLUGINOPERATIONAL &&
                                (pluginList->plugin[i].private == PLUGINPUBLIC ||
                                 (pluginList->plugin[i].private == PLUGINPRIVATE && !environment.external_user)))
                                count++;
                        sprintf(rec, "\nNumber of plugins for Libraries: %d\n\n", count);
                        strcat(doc, rec);
                        for (i = 0; i < pluginList->count; i++)
                            if (pluginList->plugin[i].class == target &&
                                pluginList->plugin[i].status == PLUGINOPERATIONAL &&
                                (pluginList->plugin[i].private == PLUGINPUBLIC ||
                                 (pluginList->plugin[i].private == PLUGINPRIVATE && !environment.external_user))) {
                                sprintf(rec, "Library name:\t\t%s\n", pluginList->plugin[i].format);
                                strcat(doc, rec);
                                sprintf(rec, "Description:\t\t%s\n", pluginList->plugin[i].desc);
                                strcat(doc, rec);
                                sprintf(rec, "Example API call:\t%s\n\n", pluginList->plugin[i].example);
                                strcat(doc, rec);
                            }
                        break;
                    }
                    case 2: {
                        target = PLUGINSERVER;
                        for (i = 0; i < pluginList->count; i++)
                            if (pluginList->plugin[i].class == target &&
                                pluginList->plugin[i].status == PLUGINOPERATIONAL &&
                                (pluginList->plugin[i].private == PLUGINPUBLIC ||
                                 (pluginList->plugin[i].private == PLUGINPRIVATE && !environment.external_user)))
                                count++;
                        sprintf(rec, "\nNumber of plugins for Data Servers: %d\n\n", count);
                        strcat(doc, rec);
                        for (i = 0; i < pluginList->count; i++)
                            if (pluginList->plugin[i].class == target &&
                                pluginList->plugin[i].status == PLUGINOPERATIONAL &&
                                (pluginList->plugin[i].private == PLUGINPUBLIC ||
                                 (pluginList->plugin[i].private == PLUGINPRIVATE && !environment.external_user))) {
                                sprintf(rec, "Server name:\t\t%s\n", pluginList->plugin[i].format);
                                strcat(doc, rec);
                                sprintf(rec, "Description:\t\t%s\n", pluginList->plugin[i].desc);
                                strcat(doc, rec);
                                sprintf(rec, "Example API call:\t%s\n\n", pluginList->plugin[i].example);
                                strcat(doc, rec);
                            }
                        break;
                    }
                    case 3: {
                        target = PLUGINDEVICE;
                        for (i = 0; i < pluginList->count; i++)
                            if (pluginList->plugin[i].class == target &&
                                pluginList->plugin[i].status == PLUGINOPERATIONAL &&
                                (pluginList->plugin[i].private == PLUGINPUBLIC ||
                                 (pluginList->plugin[i].private == PLUGINPRIVATE && !environment.external_user)))
                                count++;
                        sprintf(rec, "\nNumber of plugins for External Devices: %d\n\n", count);
                        strcat(doc, rec);
                        for (i = 0; i < pluginList->count; i++)
                            if (pluginList->plugin[i].class == target &&
                                pluginList->plugin[i].status == PLUGINOPERATIONAL &&
                                (pluginList->plugin[i].private == PLUGINPUBLIC ||
                                 (pluginList->plugin[i].private == PLUGINPRIVATE && !environment.external_user))) {
                                sprintf(rec, "External device name:\t%s\n", pluginList->plugin[i].format);
                                strcat(doc, rec);
                                sprintf(rec, "Description:\t\t%s\n", pluginList->plugin[i].desc);
                                strcat(doc, rec);
                                sprintf(rec, "Example API call:\t%s\n\n", pluginList->plugin[i].example);
                                strcat(doc, rec);
                            }
                        break;
                    }
                    case 4: {
                        target = PLUGINOTHER;
                        for (i = 0; i < pluginList->count; i++)
                            if (pluginList->plugin[i].class == target &&
                                pluginList->plugin[i].status == PLUGINOPERATIONAL &&
                                (pluginList->plugin[i].private == PLUGINPUBLIC ||
                                 (pluginList->plugin[i].private == PLUGINPRIVATE && !environment.external_user)))
                                count++;
                        sprintf(rec, "\nNumber of plugins for Other data services: %d\n\n", count);
                        strcat(doc, rec);
                        for (i = 0; i < pluginList->count; i++)
                            if (pluginList->plugin[i].class == target &&
                                pluginList->plugin[i].status == PLUGINOPERATIONAL &&
                                (pluginList->plugin[i].private == PLUGINPUBLIC ||
                                 (pluginList->plugin[i].private == PLUGINPRIVATE && !environment.external_user))) {
                                sprintf(rec, "Data service name:\t%s\n", pluginList->plugin[i].format);
                                strcat(doc, rec);
                                sprintf(rec, "Description:\t\t%s\n", pluginList->plugin[i].desc);
                                strcat(doc, rec);
                                sprintf(rec, "Example API call:\t%s\n\n", pluginList->plugin[i].example);
                                strcat(doc, rec);
                            }
                        break;
                    }
                }
            }

            strcat(doc, "\n\n");

// Return the document

            data_block->data_type = TYPE_STRING;
            data_block->rank = 0;
            data_block->data_n = strlen(doc) + 1;
            data_block->data = (char*) doc;

            strcpy(data_block->data_desc, "Description of IDAM data access services");
            strcpy(data_block->data_label, "");
            strcpy(data_block->data_units, "");

            break;

        } else

//======================================================================================
// Error ...

            err = 999;
        addIdamError(&idamerrorstack, CODEERRORTYPE, "Help", err, "Unknown function requested!");
        break;

    } while (0);

//--------------------------------------------------------------------------------------
// Housekeeping

    return err;
}
