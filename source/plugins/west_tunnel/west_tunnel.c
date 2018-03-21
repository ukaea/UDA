/*---------------------------------------------------------------
* v1 IDAM Plugin Template: Standardised plugin design template, just add ... 
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
#include "west_tunnel.h"

#include <stdlib.h>
#include <strings.h>


#include <clientserver/stringUtils.h>
#include <clientserver/initStructs.h>
#include <clientserver/udaTypes.h>
#include <plugins/udaPlugin.h>
#include <client/makeClientRequestBlock.h>
#include <client/udaClient.h>
#include <client/getEnvironment.h>
#include <client/accAPI.h>

#include "west_tunnel_ssh.h"
#include "west_tunnel_ssh_server.h"

static char* convertIdam2StringType(int type);

static int do_help(IDAM_PLUGIN_INTERFACE* idam_plugin_interface);

static int do_version(IDAM_PLUGIN_INTERFACE* idam_plugin_interface);

static int do_builddate(IDAM_PLUGIN_INTERFACE* idam_plugin_interface);

static int do_defaultmethod(IDAM_PLUGIN_INTERFACE* idam_plugin_interface);

static int do_maxinterfaceversion(IDAM_PLUGIN_INTERFACE* idam_plugin_interface);

static int do_read(IDAM_PLUGIN_INTERFACE* idam_plugin_interface);

typedef struct ServerThreadData {
    const char* experiment;
    const char* ssh_host;
    const char* uda_host;
} SERVER_THREAD_DATA;

static void* server_task(void* ptr)
{
    SERVER_THREAD_DATA* data = (SERVER_THREAD_DATA*)ptr;
    ssh_run_server(data->experiment, data->ssh_host, data->uda_host);
    return NULL;
}

int west_tunnel(IDAM_PLUGIN_INTERFACE* idam_plugin_interface)
{
    static int init = 0;

    //----------------------------------------------------------------------------------------
    // Standard v1 Plugin Interface

    if (idam_plugin_interface->interfaceVersion > THISPLUGIN_MAX_INTERFACE_VERSION) {
        RAISE_PLUGIN_ERROR("Plugin Interface Version Unknown to this plugin: Unable to execute the request!");
    }

    idam_plugin_interface->pluginVersion = THISPLUGIN_VERSION;

    REQUEST_BLOCK* request_block = idam_plugin_interface->request_block;

    if (idam_plugin_interface->housekeeping || STR_IEQUALS(request_block->function, "reset")) {
        if (!init) return 0; // Not previously initialised: Nothing to do!
        // Free Heap & reset counters
        init = 0;
        return 0;
    }

    //----------------------------------------------------------------------------------------
    // Initialise

    if (!init) {
        g_west_tunnel_server_port = 0;
        g_west_tunnel_initialised = false;

        pthread_cond_init(&g_west_tunnel_initialised_cond, NULL);
        pthread_mutex_init(&g_west_tunnel_initialised_mutex, NULL);

        pthread_t server_thread;
        SERVER_THREAD_DATA thread_data = {};
        thread_data.experiment = "WEST";
    	thread_data.ssh_host = "andromede1.partenaires.cea.fr";
    	thread_data.uda_host = "andromede1.partenaires.cea.fr";

        pthread_create(&server_thread, NULL, server_task, &thread_data);

        pthread_mutex_lock(&g_west_tunnel_initialised_mutex);
        while (!g_west_tunnel_initialised) {
            pthread_cond_wait(&g_west_tunnel_initialised_cond, &g_west_tunnel_initialised_mutex);
        }
        pthread_mutex_unlock(&g_west_tunnel_initialised_mutex);

        pthread_mutex_destroy(&g_west_tunnel_initialised_mutex);
        pthread_cond_destroy(&g_west_tunnel_initialised_cond);

        struct timespec sleep_for;
        sleep_for.tv_sec = 0;
        sleep_for.tv_nsec = 100000000;
        nanosleep(&sleep_for, NULL);

        init = 1;
    }

    if (STR_IEQUALS(request_block->function, "help")) {
        return do_help(idam_plugin_interface);
    } else if (STR_IEQUALS(request_block->function, "version")) {
        return do_version(idam_plugin_interface);
    } else if (STR_IEQUALS(request_block->function, "builddate")) {
        return do_builddate(idam_plugin_interface);
    } else if (STR_IEQUALS(request_block->function, "defaultmethod")) {
        return do_defaultmethod(idam_plugin_interface);
    } else if (STR_IEQUALS(request_block->function, "maxinterfaceversion")) {
        return do_maxinterfaceversion(idam_plugin_interface);
    } else if (STR_IEQUALS(request_block->function, "read")) {
        return do_read(idam_plugin_interface);
    } else {
        RAISE_PLUGIN_ERROR("Unknown function requested!");
    }
}

/**
 * Help: A Description of library functionality
 * @param idam_plugin_interface
 * @return
 */
int do_help(IDAM_PLUGIN_INTERFACE* idam_plugin_interface)
{
    const char* help = "\ntemplatePlugin: Add Functions Names, Syntax, and Descriptions\n\n";
    const char* desc = "templatePlugin: help = description of this plugin";

    return setReturnDataString(idam_plugin_interface->data_block, help, desc);
}

/**
 * Plugin version
 * @param idam_plugin_interface
 * @return
 */
int do_version(IDAM_PLUGIN_INTERFACE* idam_plugin_interface)
{
    return setReturnDataIntScalar(idam_plugin_interface->data_block, THISPLUGIN_VERSION, "Plugin version number");
}

/**
 * Plugin Build Date
 * @param idam_plugin_interface
 * @return
 */
int do_builddate(IDAM_PLUGIN_INTERFACE* idam_plugin_interface)
{
    return setReturnDataString(idam_plugin_interface->data_block, __DATE__, "Plugin build date");
}

/**
 * Plugin Default Method
 * @param idam_plugin_interface
 * @return
 */
int do_defaultmethod(IDAM_PLUGIN_INTERFACE* idam_plugin_interface)
{
    return setReturnDataString(idam_plugin_interface->data_block, THISPLUGIN_DEFAULT_METHOD, "Plugin default method");
}

/**
 * Plugin Maximum Interface Version
 * @param idam_plugin_interface
 * @return
 */
int do_maxinterfaceversion(IDAM_PLUGIN_INTERFACE* idam_plugin_interface)
{
    return setReturnDataIntScalar(idam_plugin_interface->data_block, THISPLUGIN_MAX_INTERFACE_VERSION, "Maximum Interface Version");
}

//----------------------------------------------------------------------------------------
// Add functionality here ....
int do_read(IDAM_PLUGIN_INTERFACE* idam_plugin_interface)
{
    //DATA_BLOCK* data_block = idam_plugin_interface->data_block;

    setenv("UDA_HOST", "localhost", 1);

    char port[100];
    sprintf(port, "%d", g_west_tunnel_server_port);
    setenv("UDA_PORT", port, 1);

    REQUEST_BLOCK* request_block = idam_plugin_interface->request_block;

    const char* element;
    FIND_REQUIRED_STRING_VALUE(request_block->nameValueList, element);

    int* indices;
    size_t nindices;
    FIND_REQUIRED_INT_ARRAY(request_block->nameValueList, indices);

    int shot;
    FIND_REQUIRED_INT_VALUE(request_block->nameValueList, shot);

    int rank;
    FIND_REQUIRED_INT_VALUE(request_block->nameValueList, rank);

    int dtype;
    FIND_REQUIRED_INT_VALUE(request_block->nameValueList, dtype);

    const char* expName = request_block->archive;

    char* copy = strdup(element);

    int i;
    for (i = 0; i < nindices; i++) {
        char replace[10];
        sprintf(replace, "%d", indices[i] + 1);
        char* old = copy;
        copy = StringReplace(copy, "#", replace);
        free(old);
    }

    char* found = strchr(copy, '/');

    *found = '\0';
    char* group = strdup(copy);
    char* variable = strdup(found + 1);
    free(copy);

    char* type = convertIdam2StringType(dtype);

    char request[1024];
    sprintf(request, "imas::get(idx=-1, group='%s', variable='%s', expName='%s', type=%s, rank=%d, shot=%d, run=0)", group, variable, expName, type, rank, shot);

    free(group);
    free(variable);

    //fprintf(stderr, "request: %s\n", request);

    REQUEST_BLOCK new_request_block;
    initRequestBlock(&new_request_block);
    int err = 0;

    env_host = 1;
    env_port = 1;

    if ((err = makeClientRequestBlock(request, "", &new_request_block)) != 0) {
        fprintf(stderr, "failed to create request block");
        return err;
    }

    int handle = idamClient(&new_request_block);
    if (handle < 0) {
        //fprintf(stderr, "UDA call failed\n");
        return handle;
    }

    *idam_plugin_interface->data_block = *getIdamDataBlock(handle);

    //fprintf(stderr, "UDA handle %d\n", handle);

    return 0;
}

char* convertIdam2StringType(int type) {
    switch (type) {
        case UDA_TYPE_INT:
            return "int";
        case UDA_TYPE_FLOAT:
            return "double";
        case UDA_TYPE_DOUBLE:
            return "double";
        case UDA_TYPE_STRING:
            return "string";
        default:
            return "unknown";
    }
    return "unknown";
}

