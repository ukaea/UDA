#include "west_plugin.h"

#include <assert.h>
#include <string.h>

#include <clientserver/errorLog.h>
#include <logging/logging.h>
#include <clientserver/stringUtils.h>
#include <clientserver/initStructs.h>
#include <clientserver/udaTypes.h>
#include <plugins/udaPlugin.h>

#include "west_xml.h"
#include "west_dynamic_data.h"

enum MAPPING_TYPE {
    NONE, STATIC, DYNAMIC
};

static int do_help(IDAM_PLUGIN_INTERFACE* idam_plugin_interface);
static int do_version(IDAM_PLUGIN_INTERFACE* idam_plugin_interface);
static int do_builddate(IDAM_PLUGIN_INTERFACE* idam_plugin_interface);
static int do_defaultmethod(IDAM_PLUGIN_INTERFACE* idam_plugin_interface);
static int do_maxinterfaceversion(IDAM_PLUGIN_INTERFACE* idam_plugin_interface);
static int do_read(IDAM_PLUGIN_INTERFACE* idam_plugin_interface);
static char* getMappingValue(const char* mappingFileName, const char* IDSRequest, int* IDSRequestType);

int westPlugin(IDAM_PLUGIN_INTERFACE* idam_plugin_interface)
{
    int err = 0;

    static short init = 0;

    // ----------------------------------------------------------------------------------------
    // Standard v1 Plugin Interface

    unsigned short housekeeping;

    if (idam_plugin_interface->interfaceVersion >
        THISPLUGIN_MAX_INTERFACE_VERSION) {
        err = 999;
        UDA_LOG(UDA_LOG_ERROR, "Plugin Interface Version Unknown to this plugin: Unable to execute the request!\n");
        addIdamError(CODEERRORTYPE, __func__,
                     err, "Plugin Interface Version Unknown to this plugin: Unable to execute the request!");
        return err;
    }

    idam_plugin_interface->pluginVersion = THISPLUGIN_VERSION;

    REQUEST_BLOCK* request_block = idam_plugin_interface->request_block;

    housekeeping = idam_plugin_interface->housekeeping;

    if (housekeeping || STR_IEQUALS(request_block->function, "reset")) {

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

    if (!init || STR_IEQUALS(request_block->function, "init")
        || STR_IEQUALS(request_block->function, "initialise")) {

        init = 1;
        if (STR_IEQUALS(request_block->function, "init")
            || STR_IEQUALS(request_block->function, "initialise")) {
            return 0;
        }
    }
    // ----------------------------------------------------------------------------------------
    // Plugin Functions
    // ----------------------------------------------------------------------------------------

    // ----------------------------------------------------------------------------------------
    // Standard methods: version, builddate, defaultmethod,
    // maxinterfaceversion

    if (STR_IEQUALS(request_block->function, "help")) {
        err = do_help(idam_plugin_interface);
    } else if (STR_IEQUALS(request_block->function, "version")) {
        err = do_version(idam_plugin_interface);
    } else if (STR_IEQUALS(request_block->function, "builddate")) {
        err = do_builddate(idam_plugin_interface);
    } else if (STR_IEQUALS(request_block->function, "defaultmethod")) {
        err = do_defaultmethod(idam_plugin_interface);
    } else if (STR_IEQUALS(request_block->function, "maxinterfaceversion")) {
        err = do_maxinterfaceversion(idam_plugin_interface);
    } else if (STR_IEQUALS(request_block->function, "read")) {
        err = do_read(idam_plugin_interface);
    } else {
        // ======================================================================================
        // Error ...
        err = 999;
        addIdamError(CODEERRORTYPE, __func__, err, "WEST:ERROR: unknown function requested!");
    }

    // --------------------------------------------------------------------------------------
    // Housekeeping

    return err;
}

// Help: A Description of library functionality
int do_help(IDAM_PLUGIN_INTERFACE* idam_plugin_interface)
{
    DATA_BLOCK* data_block = idam_plugin_interface->data_block;

    char* p = (char*)malloc(sizeof(char) * 2 * 1024);

    strcpy(p, "\nwestPlugin: this plugin maps WEST data to IDSs\n\n");

    initDataBlock(data_block);

    data_block->rank = 1;
    data_block->dims = (DIMS*)malloc(data_block->rank * sizeof(DIMS));

    int i;
    for (i = 0; i < data_block->rank; i++) {
        initDimBlock(&data_block->dims[i]);
    }

    data_block->data_type = UDA_TYPE_STRING;
    strcpy(data_block->data_desc,
           "westPlugin: help = plugin used for mapping WEST experimental data to IDS");

    data_block->data = (char*)p;

    data_block->dims[0].data_type = UDA_TYPE_UNSIGNED_INT;
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
    data_block->data_type = UDA_TYPE_INT;
    data_block->rank = 0;
    data_block->data_n = 1;
    int* data = (int*)malloc(sizeof(int));
    data[0] = THISPLUGIN_VERSION;
    data_block->data = (char*)data;
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
    data_block->data_type = UDA_TYPE_STRING;
    data_block->rank = 0;
    data_block->data_n = strlen(__DATE__) + 1;
    char* data = (char*)malloc(data_block->data_n * sizeof(char));
    strcpy(data, __DATE__);
    data_block->data = (char*)data;
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
    data_block->data_type = UDA_TYPE_STRING;
    data_block->rank = 0;
    data_block->data_n = strlen(THISPLUGIN_DEFAULT_METHOD) + 1;
    char* data = (char*)malloc(data_block->data_n * sizeof(char));
    strcpy(data, THISPLUGIN_DEFAULT_METHOD);
    data_block->data = (char*)data;
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
    data_block->data_type = UDA_TYPE_INT;
    data_block->rank = 0;
    data_block->data_n = 1;
    int* data = (int*)malloc(sizeof(int));
    data[0] = THISPLUGIN_MAX_INTERFACE_VERSION;
    data_block->data = (char*)data;
    strcpy(data_block->data_desc, "Maximum Interface Version");
    strcpy(data_block->data_label, "version");
    strcpy(data_block->data_units, "");

    return 0;
}

// ----------------------------------------------------------------------------------------
// Add functionality here ....
int do_read(IDAM_PLUGIN_INTERFACE* idam_plugin_interface)
{
    UDA_LOG(UDA_LOG_DEBUG, "Calling do_read function from WEST plugin\n");

    int err = 0;

    DATA_BLOCK* data_block = idam_plugin_interface->data_block;

    initDataBlock(data_block);

    data_block->rank = 1;
    data_block->dims = (DIMS*)malloc(data_block->rank * sizeof(DIMS));

    int i;
    for (i = 0; i < data_block->rank; i++) {
        initDimBlock(&data_block->dims[i]);
    }

    REQUEST_BLOCK* request_block = idam_plugin_interface->request_block;

    const char* element;    // will contain the IDAM mapping got from the IDAM request
    int shot;
    int* indices;
    size_t nindices;

    FIND_REQUIRED_STRING_VALUE(request_block->nameValueList, element);
    FIND_REQUIRED_INT_VALUE(request_block->nameValueList, shot);
    FIND_REQUIRED_INT_ARRAY(request_block->nameValueList, indices);

    UDA_LOG(UDA_LOG_INFO, "Calling %s for shot: %d\n", element, shot);

    const char* IDAM_MappingKey = element;

    char* mappingFileName = getenv("UDA_WEST_MAPPING_FILE");

    UDA_LOG(UDA_LOG_DEBUG, "IDAM mapping file: %s\n", mappingFileName);
    UDA_LOG(UDA_LOG_DEBUG, "IDAM mapping key: %s\n", IDAM_MappingKey);

    //Get the mapping function from the value found in the IDAM mapping file for the given IDAM_MappingKey
    //Get also the IDS type ('static' or 'dynamic')
    int IDS_DataType;
    const char* mapfun = getMappingValue(mappingFileName, IDAM_MappingKey, &IDS_DataType);

    //The path requested has not been found
    if (mapfun == NULL) {
        UDA_LOG(UDA_LOG_ERROR, "The requested mapping function has not been found. Check the IDAM mapping file.\n");
        fprintf(stderr, "The requested mapping function has not been found. Check the IDAM mapping file.");
        int err = 901;
        addIdamError(CODEERRORTYPE, "dynamic data empty !", err, "");
        return -1;
    }

    //STATIC DATA CASE
    if (IDS_DataType == STATIC) {

        UDA_LOG(UDA_LOG_DEBUG, "Fetching static data from WEST plugin\n");

        // Executing TSLib for getting static data
        int status = GetStaticData(shot, mapfun, data_block, indices);
        if (status != 0) {
            return status;
        }

        int data_type = data_block->data_type;

        UDA_LOG(UDA_LOG_DEBUG, "Requested data type: %d\n", data_type);

        if (data_type != UDA_TYPE_STRING &&
            data_type != UDA_TYPE_DOUBLE &&
            data_type != UDA_TYPE_FLOAT &&
            data_type != UDA_TYPE_LONG &&
            data_type != UDA_TYPE_INT &&
            data_type != UDA_TYPE_SHORT) {
            err = 999;
            addIdamError(CODEERRORTYPE, __func__, err, "WEST:ERROR: unsupported data type");
        }

        return 0;

    } else if (IDS_DataType == DYNAMIC) {

        // DYNAMIC DATA CASE
        UDA_LOG(UDA_LOG_DEBUG, "Fetching dynamic data from WEST plugin\n");

        int status = GetDynamicData(shot, mapfun, data_block, indices);

        return status;
    }

    return 0;
}



//Get from the IDAM mapping file the IDS XPath for the given key and the data type ('static' or 'dynamic')
//Example : <mapping key="antennas/ec/Shape_of" value="//antennas/ec/@dim" type="static"/>
// where the key is 'antennas/ec/Shape_of' and the IDS XPath is '//antennas/ec/@dim', the type is 'static'

char* getMappingValue(const char* mappingFileName, const char* IDAM_MappingKey,
                      int* IDS_DataType)
{
    xmlDocPtr doc;
    xmlXPathContextPtr xpathCtx;
    xmlXPathObjectPtr xpathObj;

    assert(mappingFileName);
    assert(IDAM_MappingKey);

    /*
     * Load XML document
     */
    doc = xmlParseFile(mappingFileName);
    if (doc == NULL) {
        UDA_LOG(UDA_LOG_ERROR, "WEST:ERROR: unable to parse IDAM mapping file %s\n", mappingFileName);
        int err = 901;
        addIdamError(CODEERRORTYPE, "WEST:ERROR: unable to parse IDAM mapping file !", err, "");
        return NULL;
    }

    /*
     * Create xpath evaluation context
     */
    xpathCtx = xmlXPathNewContext(doc);
    if (xpathCtx == NULL) {
        UDA_LOG(UDA_LOG_ERROR, "WEST:ERROR: unable to create new XPath context\n");
        int err = 901;
        addIdamError(CODEERRORTYPE, "WEST:ERROR: unable to create new XPath context", err, "");
        xmlFreeDoc(doc);
        return NULL;
    }
    // Creating the Xpath request
    UDA_LOG(UDA_LOG_DEBUG, "Creating the Xpath request\n");
    int len = strlen(IDAM_MappingKey) + 26;
    xmlChar* xPathExpr = malloc(len + sizeof(xmlChar));
    const char* c = "//mapping[@key='%s']/@value";
    xmlStrPrintf(xPathExpr, len, c, IDAM_MappingKey);

    /*
     * Evaluate xpath expression for the type
     */
    xpathObj = xmlXPathEvalExpression(xPathExpr, xpathCtx);
    if (xpathObj == NULL) {
        UDA_LOG(UDA_LOG_ERROR, "WEST:ERROR: unable to evaluate xpath expression %s\n", c);
        int err = 901;
        addIdamError(CODEERRORTYPE, "WEST:ERROR: unable to evaluate xpath expression %s\n", err, c);
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
        UDA_LOG(UDA_LOG_DEBUG, "size different of 0\n");
        cur = nodes->nodeTab[0];
        cur = cur->children;
        value = strdup((char*)cur->content);
    } else {
        UDA_LOG(UDA_LOG_DEBUG, "Error : size equals 0\n");
        err = 902;
        addIdamError(CODEERRORTYPE, __func__, err, "no result on XPath request");
    }
    const char* key_type = "//mapping[@key='%s']/@type";
    xmlStrPrintf(xPathExpr, len, key_type,
                 IDAM_MappingKey);

    /*
     * Evaluate xpath expression for the type
     */
    xpathObj = xmlXPathEvalExpression(xPathExpr, xpathCtx);
    if (xpathObj == NULL) {
        UDA_LOG(UDA_LOG_ERROR,
                 "WEST:ERROR: unable to evaluate xpath expression for getting the type (static or dynamic): %s\n", key_type);
        err = 901;
        addIdamError(CODEERRORTYPE, "WEST:ERROR: unable to evaluate following xpath expression for getting the type (static or dynamic): %s\n", err, key_type);
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
        typeStr = strdup((char*)cur->content);
    } else {
        err = 902;
        UDA_LOG(UDA_LOG_ERROR, "no result on XPath request\n");
        addIdamError(CODEERRORTYPE, __func__, err, "no result on XPath request");
    }

    UDA_LOG(UDA_LOG_DEBUG, "Setting IDS_DataType\n");

    if (typeStr == NULL) {
        *IDS_DataType = NONE;
    } else if (STR_IEQUALS(typeStr, "dynamic")) {
        *IDS_DataType = DYNAMIC;
    } else {
        *IDS_DataType = STATIC;
    }

    /*
     * Cleanup
     */
    xmlXPathFreeObject(xpathObj);
    xmlXPathFreeContext(xpathCtx);
    xmlFreeDoc(doc);

    return value;
}

