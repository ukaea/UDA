#include "exp2imas_plugin.h"

#include <string.h>
#include <assert.h>
#include <libxml/tree.h>
#include <libxml/xpath.h>

#include <clientserver/initStructs.h>
#include <clientserver/udaTypes.h>
#include <logging/logging.h>
#include <clientserver/errorLog.h>
#include <clientserver/stringUtils.h>
#include <plugins/udaPlugin.h>

#include "exp2imas_mds.h"
#include "exp2imas_xml.h"

typedef enum MappingType {
    NONE, CONSTANT, STATIC, DYNAMIC
} MAPPING_TYPE;

static int do_help(IDAM_PLUGIN_INTERFACE* idam_plugin_interface);
static int do_version(IDAM_PLUGIN_INTERFACE* idam_plugin_interface);
static int do_builddate(IDAM_PLUGIN_INTERFACE* idam_plugin_interface);
static int do_defaultmethod(IDAM_PLUGIN_INTERFACE* idam_plugin_interface);
static int do_maxinterfaceversion(IDAM_PLUGIN_INTERFACE* idam_plugin_interface);
static int do_read(IDAM_PLUGIN_INTERFACE* idam_plugin_interface);
static char* getMappingFileName(const char* IDSversion);
static char* getMachineMappingFileName(const char* element);
static xmlChar* getMappingValue(const char* mapping_file_name, const char* request, MAPPING_TYPE* request_type, int* index);
static char* deblank(char* token);
static xmlChar* insertNodeIndices(const xmlChar* xpathExpr, int** indices, size_t* n_indices);

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

    if (idam_plugin_interface->housekeeping || STR_IEQUALS(request_block->function, "reset")) {

        if (!init) {
            return 0;
        }    // Not previously initialised: Nothing to do!

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
    const char* help = "\ntsPlugin: this plugin maps Tore Supra data to IDS\n\n";
    const char* desc = "tsPlugin: help = plugin used for mapping Tore Supra experimental data to IDS";

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

    return setReturnDataString(idam_plugin_interface->data_block, __DATE__, desc);
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

xmlChar* insertNodeIndices(const xmlChar* xpathExpr, int** indices, size_t* n_indices)
{
    xmlChar* indexedXpathExpr = xmlStrdup(xpathExpr);

    if ((*indices) == NULL) {
        return indexedXpathExpr;
    }

    const xmlChar* p;
    size_t n = 0;

    while ((p = xmlStrchr(indexedXpathExpr, '#')) != NULL) {
        int len = snprintf(NULL, 0, "%d", (*indices)[n]);
        xmlChar num_str[len + 1];
        xmlStrPrintf(num_str, len + 1, (XML_FMT_TYPE)"%d", (*indices)[n]);
        ++n;

        xmlChar* pre = xmlStrndup(indexedXpathExpr, (int)(p - indexedXpathExpr));

        len = xmlStrlen(pre) + xmlStrlen(num_str) + xmlStrlen(p + 1) + 1;
        xmlChar* temp = malloc((len + 1) * sizeof(xmlChar));
        xmlStrPrintf(temp, len, (XML_FMT_TYPE)"%s%s%s", pre, num_str, p + 1);
        free(indexedXpathExpr);
        indexedXpathExpr = temp;

        free(pre);
    }

    if (n == *n_indices) {
        free(*indices);
        *indices = NULL;
        *n_indices = 0;
    } else {
        int* temp = malloc((*n_indices - n) * sizeof(int));
        size_t i;
        for (i = n; i < *n_indices; ++i) {
            temp[i - n] = (*indices)[i];
        }
        free(*indices);
        *indices = temp;
        *n_indices = (*n_indices - n);
    }

    return indexedXpathExpr;
}

// ----------------------------------------------------------------------------------------
// Add functionality here ....
int do_read(IDAM_PLUGIN_INTERFACE* idam_plugin_interface)
{
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

    const char* element = NULL;
    FIND_REQUIRED_STRING_VALUE(request_block->nameValueList, element);

    int shot = 0;
    FIND_REQUIRED_INT_VALUE(request_block->nameValueList, shot);

    int* indices = NULL;
    size_t nindices = 0;
    FIND_REQUIRED_INT_ARRAY(request_block->nameValueList, indices);

    if (nindices == 1 && indices[0] == -1) {
        nindices = 0;
        free(indices);
        indices = NULL;
    }

    int dtype = 0;
    FIND_REQUIRED_INT_VALUE(request_block->nameValueList, dtype);

    const char* IDS_version = NULL;
    FIND_REQUIRED_STRING_VALUE(request_block->nameValueList, IDS_version);

    // Search mapping value and request type (static or dynamic)
    char* experiment_mapping_file_name = getMachineMappingFileName(element);
    char* mapping_file_name = getMappingFileName(IDS_version);

    MAPPING_TYPE request_type = NONE;
    int index = -1;

    const xmlChar* xPath = getMappingValue(mapping_file_name, element, &request_type, &index);

    xPath = insertNodeIndices(xPath, &indices, &nindices);

    if (xPath == NULL) {
        return -1;
    }

    if (request_type == CONSTANT) {
        switch (dtype) {
            case TYPE_SHORT:
                setReturnDataShortScalar(data_block, (short)strtol((char*)xPath, NULL, 10), NULL);
                break;
            case TYPE_LONG:
                setReturnDataLongScalar(data_block, strtol((char*)xPath, NULL, 10), NULL);
                break;
            case TYPE_FLOAT:
                setReturnDataFloatScalar(data_block, strtof((char*)xPath, NULL), NULL);
                break;
            case TYPE_DOUBLE:
                setReturnDataDoubleScalar(data_block, strtod((char*)xPath, NULL), NULL);
                break;
            case TYPE_INT:
                setReturnDataIntScalar(data_block, (int)strtol((char*)xPath, NULL, 10), NULL);
                break;
            case TYPE_STRING:
                setReturnDataString(data_block, (char*)xPath, NULL);
                break;
            default:
                RAISE_PLUGIN_ERROR("unknown dtype given to plugin");
        }
    } else if (request_type == STATIC) {

        // Executing XPath

        char* data = NULL;
        int* dims = NULL;
        int rank = 0;
        int data_type = TYPE_UNKNOWN;
        int time_dim = 1;
        int* sizes = NULL;
        float* coefas = NULL;
        float* coefbs = NULL;

        int status = execute_xpath_expression(experiment_mapping_file_name, xPath, &data, &data_type, &time_dim, &sizes,
                                              &coefas, &coefbs, index, &dims, &rank);
        if (status != 0) {
            return status;
        }

        if ((rank != 0 || nindices != 1) && (rank != nindices)) {
            THROW_ERROR(999, "incorrect number of indices specified");
        }

        int data_idx = 0;
        int stride = 1;
        for (i = rank - 1; i > 0; --i) {
            data_idx += (indices[i] - 1) * stride;
            stride *= (i > 0) ? dims[i - 1] : 1;
        }

        if (data_type == TYPE_DOUBLE) {
            double* ddata = (double*)data;
            setReturnDataDoubleScalar(data_block, ddata[data_idx], NULL);
            free(data);
        } else if (data_type == TYPE_FLOAT) {
            float* fdata = (float*)data;
            setReturnDataFloatScalar(data_block, fdata[data_idx], NULL);
            free(data);
        } else if (data_type == TYPE_LONG) {
            long* ldata = (long*)data;
            setReturnDataLongScalar(data_block, ldata[data_idx], NULL);
            free(data);
        } else if (data_type == TYPE_INT) {
            int* idata = (int*)data;
            setReturnDataIntScalar(data_block, idata[data_idx], NULL);
            free(data);
        } else if (data_type == TYPE_SHORT) {
            short* sdata = (short*)data;
            setReturnDataShortScalar(data_block, sdata[data_idx], NULL);
            free(data);
        } else if (data_type == TYPE_STRING) {
            char** sdata = (char**)data;
            setReturnDataString(data_block, deblank(sdata[data_idx]), NULL);
            FreeSplitStringTokens((char***)&data);
        } else {
            err = 999;
            addIdamError(&idamerrorstack, CODEERRORTYPE, __func__, err, "Unsupported data type");
        }

        return 0;

    } else {

        // DYNAMIC case

        char* data = NULL;
        int* dims = NULL;
        int rank = 0;
        int data_type = TYPE_UNKNOWN;
        int time_dim = 1;
        int* sizes = NULL;
        float* coefas = NULL;
        float* coefbs = NULL;

        int status = execute_xpath_expression(experiment_mapping_file_name, xPath, &data, &data_type, &time_dim, &sizes,
                                              &coefas, &coefbs, index, &dims, &rank);
        if (status != 0) {
            return status;
        }

        if (data_type == TYPE_STRING) {

            free(data_block->dims);
            data_block->dims = NULL;

            char** signalNames = (char**)data;
            size_t signal_idx = 0;

            int data_n = 0;

            float** data_arrays = NULL;
            size_t n_arrays = 0;

            while (signalNames[signal_idx] != NULL) {
                char* signalName = signalNames[signal_idx];
                float coefa = (coefas != NULL) ? coefas[signal_idx] : 1.0f;
                float coefb = (coefbs != NULL) ? coefbs[signal_idx] : 0.0f;

                float* time;
                float* fdata;
                int len;

                status = mds_get(signalName, shot, &time, &fdata, &len, time_dim);

                if (status != 0) {
                    return status;
                }

                data_n = len / sizes[signal_idx];

                for (i = 0; i < sizes[signal_idx]; ++i) {
                    data_arrays = realloc(data_arrays, (n_arrays + 1) * sizeof(float*));

                    if (StringEndsWith(element, "/time")) {
                        data_arrays[n_arrays] = &time[0];
                    } else {
                        data_arrays[n_arrays] = &fdata[i * data_n];
                        int j;
                        for (j = 0; j < data_n; ++j) {
                            data_arrays[n_arrays][j] = coefa * data_arrays[n_arrays][j] + coefb;
                        }
                    }

                    ++n_arrays;
                }

                if (StringEndsWith(element, "/time")) {
                    free(fdata);
                } else {
                    free(time);
                }
                free(signalName);

                ++signal_idx;
            }

            data_block->rank = 1;
            data_block->data_type = TYPE_FLOAT;
            data_block->data_n = data_n;

            if (indices[0] > 0) {
                if (data_arrays != NULL) {
                    size_t sz = data_n * sizeof(data_block->data_type);
                    data_block->data = malloc(sz);
                    memcpy(data_block->data, (char*)data_arrays[indices[0] - 1], sz);
                    free(data_arrays[0]);
                }
                free(data_arrays);
            } else {
                data_block->data = (data_arrays != NULL) ? (char*)data_arrays[0] : NULL;
            }

            data_block->dims = (DIMS*)malloc(data_block->rank * sizeof(DIMS));
            for (i = 0; i < data_block->rank; i++) {
                initDimBlock(&data_block->dims[i]);
            }

            data_block->dims[0].data_type = TYPE_FLOAT;
            data_block->dims[0].dim_n = data_n;
            data_block->dims[0].compressed = 0;
            data_block->dims[0].dim = (char*)time;

            strcpy(data_block->data_label, "");
            strcpy(data_block->data_units, "");
            strcpy(data_block->data_desc, "");
        } else {
            err = 999;
            addIdamError(&idamerrorstack, CODEERRORTYPE, __func__, err, "Unsupported data type");
        }
    }

    return 0;
}

char* getMappingFileName(const char* IDSversion)
{
    char* dir = getenv("UDA_EXP2IMAS_MAPPING_FILE_DIRECTORY");
    return FormatString("%s/IMAS_mapping.xml", dir);
//    return FormatString("%s/IMAS_mapping_%s.xml", dir, IDSversion);
}

char* getMachineMappingFileName(const char* element)
{
    char* dir = getenv("UDA_EXP2IMAS_MAPPING_FILE_DIRECTORY");

    char* slash = strchr(element, '/');
    char* token = strndup(element, slash - element);

    char* name = FormatString("%s/JET_%s.xml", dir, token);
    free(token);

    return name;
}

xmlChar* getMappingValue(const char* mapping_file_name, const char* request, MAPPING_TYPE* request_type, int* index)
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
        free(xpath_expr);
        xmlXPathFreeContext(xpath_ctx);
        xmlFreeDoc(doc);
        return NULL;
    }

    xmlNodeSetPtr nodes = xpath_obj->nodesetval;
    xmlChar* value = NULL;

    xmlNodePtr current_node = NULL;
    int err = 0;

    if (nodes != NULL && nodes->nodeNr > 0) {
        current_node = nodes->nodeTab[0];
        current_node = current_node->children;
        value = xmlStrdup(current_node->content);
    } else {
        err = 998;
        addIdamError(&idamerrorstack, CODEERRORTYPE, __func__, err, "no result on XPath request, no key attribute defined?");
    }

    fmt = "//mapping[@key='%s']/@type";
    len = strlen(request) + strlen(fmt) + 1;
    free(xpath_expr);
    xpath_expr = malloc(len + sizeof(xmlChar));
    xmlStrPrintf(xpath_expr, (int)len, fmt, request);

    /*
     * Evaluate xpath expression for the type
     */
    xpath_obj = xmlXPathEvalExpression(xpath_expr, xpath_ctx);
    if (xpath_obj == NULL) {
        addIdamError(&idamerrorstack, CODEERRORTYPE, __func__, 999, "unable to evaluate xpath expression");
        IDAM_LOGF(UDA_LOG_ERROR, "unable to evaluate xpath expression \"%s\"\n", xpath_expr);
        free(xpath_expr);
        xmlXPathFreeContext(xpath_ctx);
        xmlFreeDoc(doc);
        return NULL;
    }

    nodes = xpath_obj->nodesetval;
    char* type_str = NULL;

    if (nodes != NULL && nodes->nodeNr > 0) {
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

    fmt = "//mapping[@key='%s']/@index";
    len = strlen(request) + strlen(fmt) + 1;
    free(xpath_expr);
    xpath_expr = malloc(len + sizeof(xmlChar));
    xmlStrPrintf(xpath_expr, (int)len, fmt, request);

    /*
     * Evaluate xpath expression for the type
     */
    xpath_obj = xmlXPathEvalExpression(xpath_expr, xpath_ctx);
    if (xpath_obj == NULL) {
        addIdamError(&idamerrorstack, CODEERRORTYPE, __func__, 999, "unable to evaluate xpath expression");
        IDAM_LOGF(UDA_LOG_ERROR, "unable to evaluate xpath expression \"%s\"\n", xpath_expr);
        free(xpath_expr);
        xmlXPathFreeContext(xpath_ctx);
        xmlFreeDoc(doc);
        return NULL;
    }

    nodes = xpath_obj->nodesetval;

    if (nodes != NULL && nodes->nodeNr > 0) {
        current_node = nodes->nodeTab[0];
        current_node = current_node->children;
        *index = (int)strtol((char*)current_node->content, NULL, 10);
    }

    /*
     * Cleanup
     */
    free(xpath_expr);
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
