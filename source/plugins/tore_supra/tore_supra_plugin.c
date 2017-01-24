#include "tore_supra_plugin.h"

#include <sys/types.h>
#include <regex.h>
#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include <assert.h>
#include <libxml/tree.h>
#include <libxml/parser.h>
#include <libxml/xpath.h>
#include <libxml/xpathInternals.h>

#include <idamserver.h>
#include <idamErrorLog.h>
#include <initStructs.h>
#include <idamtypes.h>
#include <makeServerRequestBlock.h>
#include <idamServerPlugin.h>
#include <idamLog.h>

#include "ts_mds.h"
#include "ts_xml.h"

enum MAPPING_TYPE {
    NONE, STATIC, DYNAMIC
};

static int do_help(IDAM_PLUGIN_INTERFACE* idam_plugin_interface);
static int do_version(IDAM_PLUGIN_INTERFACE* idam_plugin_interface);
static int do_builddate(IDAM_PLUGIN_INTERFACE* idam_plugin_interface);
static int do_defaultmethod(IDAM_PLUGIN_INTERFACE* idam_plugin_interface);
static int do_maxinterfaceversion(IDAM_PLUGIN_INTERFACE*
idam_plugin_interface);
static int do_read(IDAM_PLUGIN_INTERFACE* idam_plugin_interface);
static char* getTSToIDSMappingFileName(char* IDSRequest, int shotNumber);
static char* getMappingValue(const char* mappingFileName,
                             const char* IDSRequest, int* IDSRequestType);
static char* deblank(char* token);

int tsPlugin(IDAM_PLUGIN_INTERFACE* idam_plugin_interface)
{
    int err = 0;

    static short init = 0;

    // ----------------------------------------------------------------------------------------
    // Standard v1 Plugin Interface

    unsigned short housekeeping;

    if (idam_plugin_interface->interfaceVersion >
        THISPLUGIN_MAX_INTERFACE_VERSION) {
        err = 999;
        idamLog(LOG_ERROR,
                "ERROR templatePlugin: Plugin Interface Version Unknown to this plugin: Unable to execute the request!\n");
        addIdamError(&idamerrorstack, CODEERRORTYPE, "templatePlugin",
                     err,
                     "Plugin Interface Version Unknown to this plugin: Unable to execute the request!");
        return err;
    }

    idam_plugin_interface->pluginVersion = THISPLUGIN_VERSION;

    REQUEST_BLOCK* request_block = idam_plugin_interface->request_block;

    housekeeping = idam_plugin_interface->housekeeping;

#ifndef USE_PLUGIN_DIRECTLY
    // Don't copy the structure if housekeeping is requested - may
    // dereference a NULL or freed pointer!
    if (!housekeeping && idam_plugin_interface->environment != NULL) {
        environment = *idam_plugin_interface->environment;
    }
#endif

    // Additional interface components (must be defined at the bottom of
    // the standard data structure)
    // Versioning must be consistent with the macro
    // THISPLUGIN_MAX_INTERFACE_VERSION and the plugin registration with
    // the server

    // if(idam_plugin_interface->interfaceVersion >= 2){
    // NEW COMPONENTS
    // }

    // ----------------------------------------------------------------------------------------
    // Heap Housekeeping

    // Plugin must maintain a list of open file handles and sockets: loop
    // over and close all files and sockets
    // Plugin must maintain a list of plugin functions called: loop over
    // and reset state and free heap.
    // Plugin must maintain a list of calls to other plugins: loop over
    // and call each plugin with the housekeeping request
    // Plugin must destroy lists at end of housekeeping

    // A plugin only has a single instance on a server. For multiple
    // instances, multiple servers are needed.
    // Plugins can maintain state so recursive calls (on the same server)
    // must respect this.
    // If the housekeeping action is requested, this must be also applied
    // to all plugins called.
    // A list must be maintained to register these plugin calls to manage
    // housekeeping.
    // Calls to plugins must also respect access policy and user
    // authentication policy

    if (housekeeping || !strcasecmp(request_block->function, "reset")) {

        if (!init) {
            return 0;
        }    // Not previously initialised: Nothing to
        // do!

        // Free Heap & reset counters

        init = 0;

        return 0;
    }
    // ----------------------------------------------------------------------------------------
    // Initialise

    if (!init || !strcasecmp(request_block->function, "init")
        || !strcasecmp(request_block->function, "initialise")) {

        init = 1;
        if (!strcasecmp(request_block->function, "init")
            || !strcasecmp(request_block->function, "initialise")) {
                return 0;
        }
    }
    // ----------------------------------------------------------------------------------------
    // Plugin Functions
    // ----------------------------------------------------------------------------------------

    // ----------------------------------------------------------------------------------------
    // Standard methods: version, builddate, defaultmethod,
    // maxinterfaceversion

    if (!strcasecmp(request_block->function, "help")) {
        err = do_help(idam_plugin_interface);
    } else if (!strcasecmp(request_block->function, "version")) {
        err = do_version(idam_plugin_interface);
    } else if (!strcasecmp(request_block->function, "builddate")) {
        err = do_builddate(idam_plugin_interface);
    } else if (!strcasecmp(request_block->function, "defaultmethod")) {
        err = do_defaultmethod(idam_plugin_interface);
    } else if (!strcasecmp(request_block->function, "maxinterfaceversion")) {
        err = do_maxinterfaceversion(idam_plugin_interface);
    } else if (!strcasecmp(request_block->function, "read")) {
        err = do_read(idam_plugin_interface);
    } else {
        // ======================================================================================
        // Error ...
        err = 999;
        addIdamError(&idamerrorstack, CODEERRORTYPE, "templatePlugin",
                     err, "Unknown function requested!");
    }

    // --------------------------------------------------------------------------------------
    // Housekeeping

    return err;
}

// Help: A Description of library functionality
int do_help(IDAM_PLUGIN_INTERFACE* idam_plugin_interface)
{
    DATA_BLOCK* data_block = idam_plugin_interface->data_block;

    char* p = (char*) malloc(sizeof(char) * 2 * 1024);

    strcpy(p, "\ntsPlugin: this plugin maps Tore Supra data to IDS\n\n");

    initDataBlock(data_block);

    data_block->rank = 1;
    data_block->dims = (DIMS*) malloc(data_block->rank * sizeof(DIMS));

    int i;
    for (i = 0; i < data_block->rank; i++) {
        initDimBlock(&data_block->dims[i]);
    }

    data_block->data_type = TYPE_STRING;
    strcpy(data_block->data_desc,
           "tsPlugin: help = plugin used for mapping Tore Supra experimental data to IDS");

    data_block->data = (char*) p;

    data_block->dims[0].data_type = TYPE_UNSIGNED_INT;
    data_block->dims[0].dim_n = strlen(p) + 1;
    data_block->dims[0].compressed = 1;
    data_block->dims[0].dim0 = 0.0;
    data_block->dims[0].diff = 1.0;
    data_block->dims[0].method = 0;

    data_block->dims[0].compressed = 0;

    data_block->data_n = data_block->dims[0].dim_n;

    strcpy(data_block->data_label, "");
    strcpy(data_block->data_units, "");

    return 0;
}

int do_version(IDAM_PLUGIN_INTERFACE* idam_plugin_interface)
{
    DATA_BLOCK* data_block = idam_plugin_interface->data_block;

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

    return 0;
}

// Plugin Build Date
int do_builddate(IDAM_PLUGIN_INTERFACE* idam_plugin_interface)
{
    DATA_BLOCK* data_block = idam_plugin_interface->data_block;

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

    return 0;
}

// Plugin Default Method
int do_defaultmethod(IDAM_PLUGIN_INTERFACE* idam_plugin_interface)
{
    DATA_BLOCK* data_block = idam_plugin_interface->data_block;

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

    return 0;
}

// Plugin Maximum Interface Version
int do_maxinterfaceversion(IDAM_PLUGIN_INTERFACE* idam_plugin_interface)
{
    DATA_BLOCK* data_block = idam_plugin_interface->data_block;

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

    return 0;
}

// ----------------------------------------------------------------------------------------
// Add functionality here ....
int do_read(IDAM_PLUGIN_INTERFACE* idam_plugin_interface)
{
    int err = 0;

    DATA_BLOCK* data_block = idam_plugin_interface->data_block;

    initDataBlock(data_block);

    data_block->rank = 1;
    data_block->dims = (DIMS*) malloc(data_block->rank * sizeof(DIMS));

    int i;
    for (i = 0; i < data_block->rank; i++) {
        initDimBlock(&data_block->dims[i]);
    }

    REQUEST_BLOCK* request_block = idam_plugin_interface->request_block;

    char* element;        // will contain the modified IDSRequest
    // which will be one key of the IDS
    // requests mapping file
    int shot;
    int* indices;
    size_t nindices;

    FIND_REQUIRED_STRING_VALUE(request_block->nameValueList, element);
    FIND_REQUIRED_INT_VALUE(request_block->nameValueList, shot);
    FIND_REQUIRED_INT_ARRAY(request_block->nameValueList, indices);

    char* IDSRequest = element;

    // Search mapping value and request type (static or dynamic)
    char* fullTSMappingFileName =
            getTSToIDSMappingFileName(IDSRequest, shot);
    char* mappingFileName = getenv("IDAM_TS_MAPPING_FILE");

    int IDSRequestType;
    const xmlChar* xPath =
            (xmlChar*) getMappingValue(mappingFileName, IDSRequest,
                                       &IDSRequestType);

    if (xPath == NULL) {
        return -1;
    }

    if (IDSRequestType == STATIC) {

        // Executing XPath

        int status =
                execute_xpath_expression(fullTSMappingFileName, xPath, data_block, indices);
        if (status != 0) {
            return status;
        }

        int data_type = data_block->data_type;

        if (data_type == TYPE_DOUBLE) {
            double* data = (double*) data_block->data;
            double temp = data[0];
            //if (indices[0] > 0)
            //    temp = data[indices[0]-1];
            data_block->data = malloc(sizeof(double));
            *((double*) data_block->data) = temp;
            free(data);
        } else if (data_type == TYPE_FLOAT) {
            float* data = (float*) data_block->data;
            float temp = data[0];
            //if (indices[0] > 0)
            //    temp = data[indices[0]-1];
            data_block->data = malloc(sizeof(float));
            *((float*) data_block->data) = temp;
            free(data);
        } else if (data_type == TYPE_LONG) {
            long* data = (long*) data_block->data;
            long temp = data[0];
            //if (indices[0] > 0)
            //    temp = data[indices[0]-1];
            data_block->data = malloc(sizeof(long));
            *((long*) data_block->data) = temp;
            free(data);
        } else if (data_type == TYPE_INT) {
            int* data = (int*) data_block->data;
            int temp = data[0];
            //if (indices[0] > 0)
            //    temp = data[indices[0]-1];
            data_block->data = malloc(sizeof(int));
            *((int*) data_block->data) = temp;
            free(data);
        } else if (data_type == TYPE_SHORT) {
            short* data = (short*) data_block->data;
            short temp = data[0];
            //if (indices[0] > 0)
            //    temp = data[indices[0]-1];
            data_block->data = malloc(sizeof(short));
            *((short*) data_block->data) = temp;
            free(data);
        } else if (data_type == TYPE_STRING) {
            char* data = (char*) data_block->data;
            char* temp = deblank(strdup(data));
            data_block->data = temp;
            free(data);
        } else {
            err = 999;
            addIdamError(&idamerrorstack, CODEERRORTYPE,
                         "tore_supra : Unsupported data type", err,
                         "");
        }

        free(data_block->dims);
        data_block->dims = NULL;

        // Scalar data
        data_block->rank = 0;
        data_block->data_n = 1;

        strcpy(data_block->data_label, "");
        strcpy(data_block->data_units, "");
        strcpy(data_block->data_desc, "");

        return 0;

    } else {

        // DYNAMIC case

        int status =
                execute_xpath_expression(fullTSMappingFileName, xPath, data_block, indices);
        if (status != 0) {
            return status;
        }

        int data_type = data_block->data_type;

        if (data_type == TYPE_STRING) {
            // data_block->data contains MDS+ signal name -- use to get
            // data from MDS+ server
            char* signalName = data_block->data;

            float* time;
            int len;
            float* data;
            int status =
                    ts_mds_get(signalName, shot, &time, &data, &len);
            if (status != 0) {
                return status;
            }

            free(data_block->dims);
            data_block->dims = NULL;

            data_block->rank = 1;
            data_block->data_type = TYPE_FLOAT;
            data_block->data_n = len;
            data_block->data = (char*) data;
            data_block->dims =
                    (DIMS*) malloc(data_block->rank * sizeof(DIMS));

            for (i = 0; i < data_block->rank; i++) {
                initDimBlock(&data_block->dims[i]);
            }

            data_block->dims[0].data_type = TYPE_FLOAT;
            data_block->dims[0].dim_n = len;
            data_block->dims[0].compressed = 0;
            data_block->dims[0].dim = (char*) time;

            strcpy(data_block->data_label, "");
            strcpy(data_block->data_units, "");
            strcpy(data_block->data_desc, "");
        } else {
            err = 999;
            addIdamError(&idamerrorstack, CODEERRORTYPE,
                         "tore_supra : Unsupported data type", err,
                         "");
        }
    }

    return 0;
}

char* getTSToIDSMappingFileName(char* IDSRequest, int shot)
{
    char* p = strchr(IDSRequest, '/');
    char* IDSName = strndup(IDSRequest, p - IDSRequest);

    int mappingVersion = 0;

    // TODO : change htis code by calling a TS service (using TSLib ?) to
    // get the mapping version for the requested shot

    if (shot >= 28764 && shot <= 100000) {
        mappingVersion = 1;
    } else {
        int err = -100;
        addIdamError(&idamerrorstack, CODEERRORTYPE,
                     "tore_supra : no TS/IDS mapping file available for the requested shot number",
                     err, "");
    }

    char* dir = getenv("IDAM_TS_MAPPING_FILE_DIRECTORY");

    const char* fmt = "%s/%s_v%d.xml";
    size_t len = snprintf(NULL, 0, fmt, dir, IDSName, mappingVersion);
    char* mappingFileName = malloc(len + 1);
    snprintf(mappingFileName, len + 1, fmt, dir, IDSName, mappingVersion);

    return mappingFileName;
}

char* getMappingValue(const char* mappingFileName, const char* IDSRequest,
                      int* IDSRequestType)
{
    xmlDocPtr doc;
    xmlXPathContextPtr xpathCtx;
    xmlXPathObjectPtr xpathObj;

    assert(mappingFileName);
    assert(IDSRequest);

    /*
     * Load XML document
     */
    doc = xmlParseFile(mappingFileName);
    if (doc == NULL) {
        fprintf(stderr, "Error: unable to parse file \"%s\"\n",
                mappingFileName);
        return NULL;
    }

    /*
     * Create xpath evaluation context
     */
    xpathCtx = xmlXPathNewContext(doc);
    if (xpathCtx == NULL) {
        fprintf(stderr, "Error: unable to create new XPath context\n");
        xmlFreeDoc(doc);
        return NULL;
    }
    // Creating the Xpath request

    int len = strlen(IDSRequest) + 26;
    xmlChar* xPathExpr = malloc(len + sizeof(xmlChar));
    xmlStrPrintf(xPathExpr, len, (xmlChar*) "//mapping[@key='%s']/@value",
                 IDSRequest);

    /*
     * Evaluate xpath expression for the type
     */
    xpathObj = xmlXPathEvalExpression(xPathExpr, xpathCtx);
    if (xpathObj == NULL) {
        fprintf(stderr,
                "Error: unable to evaluate xpath expression \"%s\"\n",
                xPathExpr);
        xmlXPathFreeContext(xpathCtx);
        xmlFreeDoc(doc);
        return NULL;
    }

    xmlNodeSetPtr nodes = xpathObj->nodesetval;
    int size = (nodes) ? nodes->nodeNr : 0;
    char* value = NULL;

    xmlNodePtr cur;
    int err = 0;

    if (size != 0) {
        cur = nodes->nodeTab[0];
        cur = cur->children;
        value = strdup((char*) cur->content);
    } else {
        err = 998;
        addIdamError(&idamerrorstack, CODEERRORTYPE,
                     "tore_supra plugin", err,
                     "no result on XPath request, no key attribute defined?");
    }

    xmlStrPrintf(xPathExpr, len, (xmlChar*) "//mapping[@key='%s']/@type",
                 IDSRequest);

    /*
     * Evaluate xpath expression for the type
     */
    xpathObj = xmlXPathEvalExpression(xPathExpr, xpathCtx);
    if (xpathObj == NULL) {
        fprintf(stderr,
                "Error: unable to evaluate xpath expression \"%s\"\n",
                xPathExpr);
        xmlXPathFreeContext(xpathCtx);
        xmlFreeDoc(doc);
        return NULL;
    }

    nodes = xpathObj->nodesetval;
    size = (nodes) ? nodes->nodeNr : 0;
    char* typeStr = NULL;

    err = 0;

    if (size != 0) {
        cur = nodes->nodeTab[0];
        cur = cur->children;
        typeStr = strdup((char*) cur->content);
    } else {
        err = 998;
        addIdamError(&idamerrorstack, CODEERRORTYPE,
                     "tore_supra plugin : no result on XPath request, no key attribute defined ?",
                     err, "");
    }

    if (typeStr == NULL) {
        *IDSRequestType = NONE;
    } else if (!strcasecmp(typeStr, "dynamic")) {
        *IDSRequestType = DYNAMIC;
    } else {
        *IDSRequestType = STATIC;
    }

    /*
     * Cleanup
     */
    xmlXPathFreeObject(xpathObj);
    xmlXPathFreeContext(xpathCtx);
    xmlFreeDoc(doc);

    return value;
}

char* deblank(char* input)
{
    int i, j;
    char* output = input;
    for (i = 0, j = 0; i < strlen(input); i++, j++) {
        if (input[i] != ' ' && input[i] != '\'') {
            output[j] = input[i];
        } else {
            j--;
        }
    }
    output[j] = 0;
    return output;
}
