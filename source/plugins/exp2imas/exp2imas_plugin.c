#include "exp2imas_plugin.h"

#include <string.h>
#include <libxml/tree.h>
#include <libxml/xpath.h>

#include <clientserver/errorLog.h>
#include <clientserver/initStructs.h>
#include <clientserver/stringUtils.h>
#include <clientserver/udaTypes.h>
#include <logging/logging.h>
#include <plugins/udaPlugin.h>

#include "exp2imas_mds.h"
#include "exp2imas_xml.h"

enum MAPPING_TYPE {
    NONE, CONSTANT, STATIC, DYNAMIC
};

static int do_help(IDAM_PLUGIN_INTERFACE* idam_plugin_interface);
static int do_version(IDAM_PLUGIN_INTERFACE* idam_plugin_interface);
static int do_builddate(IDAM_PLUGIN_INTERFACE* idam_plugin_interface);
static int do_defaultmethod(IDAM_PLUGIN_INTERFACE* idam_plugin_interface);
static int do_maxinterfaceversion(IDAM_PLUGIN_INTERFACE* idam_plugin_interface);
static int do_read(IDAM_PLUGIN_INTERFACE* idam_plugin_interface);

static char* getMappingFileName(const char* element);
static xmlChar* getMappingValue(const char* mapping_file_name, const char* request, int* request_type);
static char* deblank(char* input);
static int getStaticData(DATA_BLOCK* data_block, const int* indices, const char* mapping_file_name, const xmlChar* xPath);
static int getDynamicData(DATA_BLOCK* data_block, int shot, const int* indices, const char* mapping_file_name, const xmlChar* xPath);

#ifndef strndup
char*
strndup(const char* s, size_t n)
{
    char* result;
    size_t len = strlen(s);

    if (n < len) {
        len = n;
    }

    result = (char*)malloc(len + 1);
    if (!result) {
        return 0;
    }

    result[len] = '\0';
    return (char*)memcpy(result, s, len);
}
#endif

int exp2imasPlugin(IDAM_PLUGIN_INTERFACE* idam_plugin_interface)
{
    // ----------------------------------------------------------------------------------------
    // Standard v1 Plugin Interface

    if (idam_plugin_interface->interfaceVersion > THISPLUGIN_MAX_INTERFACE_VERSION) {
        RAISE_PLUGIN_ERROR("Plugin Interface Version Unknown to this plugin: Unable to execute the request!");
    }

    idam_plugin_interface->pluginVersion = THISPLUGIN_VERSION;

    REQUEST_BLOCK* request_block = idam_plugin_interface->request_block;

    static short init = 0;

    // ----------------------------------------------------------------------------------------
    // Heap Housekeeping

    if (idam_plugin_interface->housekeeping || StringIEquals(request_block->function, "reset")) {
        if (!init) {
            return 0;
        }    // Not previously initialised: Nothing to do!

        // Free Heap & reset counters
        init = 0;
        return 0;
    }

    // ----------------------------------------------------------------------------------------
    // Initialise

    if (!init || STR_IEQUALS(request_block->function, "init") || STR_IEQUALS(request_block->function, "initialise")) {
        init = 1;
        if (STR_IEQUALS(request_block->function, "init") || STR_IEQUALS(request_block->function, "initialise")) {
            return 0;
        }
    }

    // ----------------------------------------------------------------------------------------
    // Plugin Functions
    // ----------------------------------------------------------------------------------------

    int err = 0;

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
        RAISE_PLUGIN_ERROR("Unknown function requested!");
    }

    return err;
}

// Help: A Description of library functionality
int do_help(IDAM_PLUGIN_INTERFACE* idam_plugin_interface)
{
    const char* help = "\nexp2imas: this plugin maps machine specific data to IDS\n\n";
    const char* desc = "exp2imas: help = plugin used for mapping machine experimental data to IDS";

    return setReturnDataString(idam_plugin_interface->data_block, help, desc);
}

int do_version(IDAM_PLUGIN_INTERFACE* idam_plugin_interface)
{
    const char* desc = "Plugin version number";

    return setReturnDataIntScalar(idam_plugin_interface->data_block, THISPLUGIN_VERSION, desc);
}

// Plugin Build Date
int do_builddate(IDAM_PLUGIN_INTERFACE* idam_plugin_interface)
{
    const char* desc = "Plugin build date";
    const char* date = ISOdatetime;

    return setReturnDataString(idam_plugin_interface->data_block, date, desc);
}

// Plugin Default Method
int do_defaultmethod(IDAM_PLUGIN_INTERFACE* idam_plugin_interface)
{
    const char* desc = "Plugin default method";

    return setReturnDataString(idam_plugin_interface->data_block, THISPLUGIN_DEFAULT_METHOD, desc);
}

// Plugin Maximum Interface Version
int do_maxinterfaceversion(IDAM_PLUGIN_INTERFACE* idam_plugin_interface)
{
    const char* desc = "Maximum Interface Version";

    return setReturnDataIntScalar(idam_plugin_interface->data_block, THISPLUGIN_MAX_INTERFACE_VERSION, desc);
}

// ----------------------------------------------------------------------------------------
// Add functionality here ....
int do_read(IDAM_PLUGIN_INTERFACE* idam_plugin_interface)
{
    DATA_BLOCK* data_block = idam_plugin_interface->data_block;

    initDataBlock(data_block);

    data_block->rank = 1;
    data_block->dims = (DIMS*)malloc(data_block->rank * sizeof(DIMS));

    int i;
    for (i = 0; i < data_block->rank; i++) {
        initDimBlock(&data_block->dims[i]);
    }

    REQUEST_BLOCK* request_block = idam_plugin_interface->request_block;

    const char* element = NULL;
    FIND_REQUIRED_STRING_VALUE(request_block->nameValueList, element);

    int shot = 0;
    FIND_REQUIRED_INT_VALUE(request_block->nameValueList, shot);

    int* indices = NULL;
    size_t nindices = 0;
    FIND_INT_ARRAY(request_block->nameValueList, indices);

    // Search mapping value and request type (static or dynamic)
    char* imas_mapping_file_name = getMappingFileName(element);
    char* mapping_file_name = getenv("UDA_EXP2IMAS_MAPPING_FILE");

    int request_type;
    const xmlChar* xPath = getMappingValue(imas_mapping_file_name, element, &request_type);

    if (xPath == NULL) {
        return -1;
    }

    switch (request_type) {
        case STATIC:
            return getStaticData(data_block, indices, mapping_file_name, xPath);
        case DYNAMIC:
            return getDynamicData(data_block, shot, indices, mapping_file_name, xPath);
        case CONSTANT:
            return execute_xpath_expression(mapping_file_name, xPath, data_block, indices);
        default:
            THROW_ERROR(999, "Unknown request_type");
    }
}

int getDynamicData(DATA_BLOCK* data_block, int shot, const int* indices, const char* mapping_file_name, const xmlChar* xPath)
{
    int status = execute_xpath_expression(mapping_file_name, xPath, data_block, indices);
    if (status != 0) {
        return status;
    }

    int data_type = data_block->data_type;

    if (data_type == TYPE_STRING) {
        // data_block->data contains MDS+ signal name -- use to get
        // data from MDS+ server
        const char* signalName = data_block->data;

        float* time;
        int len;
        float* data;
        status = exp2imas_mds_get(signalName, shot, &time, &data, &len);
        if (status != 0) {
            return status;
        }

        free(data_block->dims);
        data_block->dims = NULL;

        data_block->rank = 1;
        data_block->data_type = TYPE_FLOAT;
        data_block->data_n = len;
        data_block->data = (char*)data;
        data_block->dims = (DIMS*)malloc(data_block->rank * sizeof(DIMS));

        int i;
        for (i = 0; i < data_block->rank; i++) {
            initDimBlock(&data_block->dims[i]);
        }

        data_block->dims[0].data_type = TYPE_FLOAT;
        data_block->dims[0].dim_n = len;
        data_block->dims[0].compressed = 0;
        data_block->dims[0].dim = (char*)time;

        strcpy(data_block->data_label, "");
        strcpy(data_block->data_units, "");
        strcpy(data_block->data_desc, "");
    } else {
        RAISE_PLUGIN_ERROR("Unsupported data type");
    }

    return 0;
}

int getStaticData(DATA_BLOCK* data_block, const int* indices, const char* mapping_file_name, const xmlChar* xPath)
{
    int status = execute_xpath_expression(mapping_file_name, xPath, data_block, indices);
    if (status != 0) {
        return status;
    }

    int data_type = data_block->data_type;

    if (data_type == TYPE_DOUBLE) {
        double* data = (double*)data_block->data;
        double temp = data[0];
        data_block->data = malloc(sizeof(double));
        *((double*)data_block->data) = temp;
        free(data);
    } else if (data_type == TYPE_FLOAT) {
        float* data = (float*)data_block->data;
        float temp = data[0];
        data_block->data = malloc(sizeof(float));
        *((float*)data_block->data) = temp;
        free(data);
    } else if (data_type == TYPE_LONG) {
        long* data = (long*)data_block->data;
        long temp = data[0];
        data_block->data = malloc(sizeof(long));
        *((long*)data_block->data) = temp;
        free(data);
    } else if (data_type == TYPE_INT) {
        int* data = (int*)data_block->data;
        int temp = data[0];
        data_block->data = malloc(sizeof(int));
        *((int*)data_block->data) = temp;
        free(data);
    } else if (data_type == TYPE_SHORT) {
        short* data = (short*)data_block->data;
        short temp = data[0];
        data_block->data = malloc(sizeof(short));
        *((short*)data_block->data) = temp;
        free(data);
    } else if (data_type == TYPE_STRING) {
        char* data = data_block->data;
        char* temp = deblank(strdup(data));
        data_block->data = temp;
        free(data);
    } else {
        RAISE_PLUGIN_ERROR("Unsupported data type");
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
}

char* getMappingFileName(const char* element)
{
    char** tokens = SplitString(element, "/");

    char* dir = getenv("UDA_EXP2IMAS_MAPPING_FILE_DIRECTORY");

    char* file_name = FormatString("%s/IMAS_mapping_%s.xml", dir, tokens);

    FreeSplitStringTokens(&tokens);

    return file_name;
}

xmlChar* getMappingValue(const char* mapping_file_name, const char* request, int* request_type)
{
    /*
     * Load XML document
     */
    xmlDocPtr doc = xmlParseFile(mapping_file_name);
    if (doc == NULL) {
        addIdamError(&idamerrorstack, CODEERRORTYPE, __func__, 999, "unable to parse file");
        IDAM_LOGF(UDA_LOG_ERROR, "unable to parse file \"%s\"\n", mapping_file_name);
        return NULL;
    }

    /*
     * Create xpath evaluation context
     */
    xmlXPathContextPtr xpath_ctx = xmlXPathNewContext(doc);
    if (xpath_ctx == NULL) {
        addIdamError(&idamerrorstack, CODEERRORTYPE, __func__, 999, "unable to create new XPath context");
        IDAM_LOGF(UDA_LOG_ERROR, "unable to create new XPath context\n", mapping_file_name);
        xmlFreeDoc(doc);
        return NULL;
    }
    // Creating the Xpath request

    XML_FMT_TYPE fmt = "//mapping[@key='%s']/@value";
    size_t len = strlen(request) + strlen(fmt) + 1;
    xmlChar* xpath_expr = malloc(len + sizeof(xmlChar));
    xmlStrPrintf(xpath_expr, (int)len, fmt, request);

    /*
     * Evaluate xpath expression for the type
     */
    xmlXPathObjectPtr xpath_obj = xmlXPathEvalExpression(xpath_expr, xpath_ctx);
    if (xpath_obj == NULL) {
        addIdamError(&idamerrorstack, CODEERRORTYPE, __func__, 999, "unable to evaluate xpath expression");
        IDAM_LOGF(UDA_LOG_ERROR, "unable to evaluate xpath expression \"%s\"\n", xpath_expr);
        xmlXPathFreeContext(xpath_ctx);
        xmlFreeDoc(doc);
        return NULL;
    }

    xmlNodeSetPtr nodes = xpath_obj->nodesetval;
    int size = (nodes) ? nodes->nodeNr : 0;
    xmlChar* value = NULL;

    xmlNodePtr current_node = NULL;
    int err = 0;

    if (size != 0) {
        current_node = nodes->nodeTab[0];
        current_node = current_node->children;
        value = xmlStrdup(current_node->content);
    } else {
        err = 998;
        addIdamError(&idamerrorstack, CODEERRORTYPE, __func__, err, "no result on XPath request, no key attribute defined?");
    }

    fmt = "//mapping[@key='%s']/@type";
    xmlStrPrintf(xpath_expr, (int)len, fmt, request);

    /*
     * Evaluate xpath expression for the type
     */
    xpath_obj = xmlXPathEvalExpression(xpath_expr, xpath_ctx);
    if (xpath_obj == NULL) {
        addIdamError(&idamerrorstack, CODEERRORTYPE, __func__, 999, "unable to evaluate xpath expression");
        IDAM_LOGF(UDA_LOG_ERROR, "unable to evaluate xpath expression \"%s\"\n", xpath_expr);
        xmlXPathFreeContext(xpath_ctx);
        xmlFreeDoc(doc);
        return NULL;
    }

    nodes = xpath_obj->nodesetval;
    size = (nodes) ? nodes->nodeNr : 0;
    char* type_str = NULL;

    if (size != 0) {
        current_node = nodes->nodeTab[0];
        current_node = current_node->children;
        type_str = strdup((char*)current_node->content);
    } else {
        err = 998;
        addIdamError(&idamerrorstack, CODEERRORTYPE, __func__, err, "no result on XPath request, no key attribute defined?");
    }

    if (type_str == NULL) {
        *request_type = NONE;
    } else if (STR_IEQUALS(type_str, "constant")) {
        *request_type = CONSTANT;
    } else if (STR_IEQUALS(type_str, "dynamic")) {
        *request_type = DYNAMIC;
    } else if (STR_IEQUALS(type_str, "static")) {
        *request_type = STATIC;
    } else {
        addIdamError(&idamerrorstack, CODEERRORTYPE, __func__, 999, "unknown mapping type");
        IDAM_LOGF(UDA_LOG_ERROR, "unknown mapping type \"%s\"\n", type_str);
        value = NULL;
    }

    /*
     * Cleanup
     */
    xmlXPathFreeObject(xpath_obj);
    xmlXPathFreeContext(xpath_ctx);
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
