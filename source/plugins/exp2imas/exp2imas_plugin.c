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
#include <structures/struct.h>

#include "exp2imas_mds.h"
#include "exp2imas_xml.h"

typedef enum MappingType {
    NONE,
    CONSTANT,
    STATIC,
    DYNAMIC,
    ERROR
} MAPPING_TYPE;

static int do_help(IDAM_PLUGIN_INTERFACE* idam_plugin_interface);
static int do_version(IDAM_PLUGIN_INTERFACE* idam_plugin_interface);
static int do_builddate(IDAM_PLUGIN_INTERFACE* idam_plugin_interface);
static int do_defaultmethod(IDAM_PLUGIN_INTERFACE* idam_plugin_interface);
static int do_maxinterfaceversion(IDAM_PLUGIN_INTERFACE* idam_plugin_interface);
static int do_read(IDAM_PLUGIN_INTERFACE* idam_plugin_interface);
static char* getMappingFileName(const char* IDSversion, const char* element);
static char* getMachineMappingFileName(const char* element);
static xmlChar* getMappingValue(const char* mapping_file_name, const char* request, MAPPING_TYPE* request_type, int* index, int* adjust);
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
            // Not previously initialised: Nothing to do!
            return 0;
        }

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

static int handle_constant(DATA_BLOCK* data_block, int dtype, const xmlChar* xPath)
{
    switch (dtype) {
        case UDA_TYPE_SHORT:
            setReturnDataShortScalar(data_block, (short)strtol((char*)xPath, NULL, 10), NULL);
            break;
        case UDA_TYPE_LONG:
            setReturnDataLongScalar(data_block, strtol((char*)xPath, NULL, 10), NULL);
            break;
        case UDA_TYPE_FLOAT:
            setReturnDataFloatScalar(data_block, strtof((char*)xPath, NULL), NULL);
            break;
        case UDA_TYPE_DOUBLE:
            setReturnDataDoubleScalar(data_block, strtod((char*)xPath, NULL), NULL);
            break;
        case UDA_TYPE_INT:
            setReturnDataIntScalar(data_block, (int)strtol((char*)xPath, NULL, 10), NULL);
            break;
        case UDA_TYPE_STRING:
            setReturnDataString(data_block, (char*)xPath, NULL);
            break;
        default:
            RAISE_PLUGIN_ERROR("unknown dtype given to plugin");
    }
    return 0;
}

static int handle_static(DATA_BLOCK* data_block, const char* experiment_mapping_file_name, const xmlChar* xPath,
                         int index, int* indices, size_t nindices, int adjust)
{
    // Executing XPath

    XML_DATA xml_data;

    int status = execute_xpath_expression(experiment_mapping_file_name, xPath, index, &xml_data);
    if (status != 0) {
        return status;
    }

    if (xml_data.rank > 1 && index != -1) {
        indices = realloc(indices, (nindices + 1) * sizeof(int));
        indices[nindices] = index;
        ++nindices;
    }

    if ((xml_data.rank != 0 || nindices != 1) && (xml_data.rank != nindices)) {
        if (xml_data.rank == 1 && nindices == 0) {
            size_t shape[] = { (size_t)xml_data.dims[0] };

            if (xml_data.data_type == UDA_TYPE_DOUBLE) {
                setReturnDataDoubleArray(data_block, (double*)xml_data.data, 1, shape, NULL);
            } else if (xml_data.data_type == UDA_TYPE_FLOAT) {
                setReturnDataFloatArray(data_block, (float*)xml_data.data, 1, shape, NULL);
            } else if (xml_data.data_type == UDA_TYPE_INT) {
                setReturnDataIntArray(data_block, (int*)xml_data.data, 1, shape, NULL);
            } else {
                RAISE_PLUGIN_ERROR("Unsupported data type");
            }
        } else {
            THROW_ERROR(999, "incorrect number of indices specified");
        }
    } else {
        int data_idx = 0;
        int stride = 1;
        int i;
        for (i = xml_data.rank - 1; i >= 0; --i) {
            data_idx += (indices[i] - 1) * stride;
            stride *= xml_data.dims[i];
        }

        if (xml_data.data_type == UDA_TYPE_DOUBLE) {
            double* ddata = (double*)xml_data.data;
            setReturnDataDoubleScalar(data_block, ddata[data_idx] + adjust, NULL);
            free(xml_data.data);
        } else if (xml_data.data_type == UDA_TYPE_FLOAT) {
            float* fdata = (float*)xml_data.data;
            setReturnDataFloatScalar(data_block, fdata[data_idx] + adjust, NULL);
            free(xml_data.data);
        } else if (xml_data.data_type == UDA_TYPE_LONG) {
            long* ldata = (long*)xml_data.data;
            setReturnDataLongScalar(data_block, ldata[data_idx] + adjust, NULL);
            free(xml_data.data);
        } else if (xml_data.data_type == UDA_TYPE_INT) {
            int* idata = (int*)xml_data.data;
            setReturnDataIntScalar(data_block, idata[data_idx] + adjust, NULL);
            free(xml_data.data);
        } else if (xml_data.data_type == UDA_TYPE_SHORT) {
            short* sdata = (short*)xml_data.data;
            setReturnDataShortScalar(data_block, sdata[data_idx] + (short)adjust, NULL);
            free(xml_data.data);
        } else if (xml_data.data_type == UDA_TYPE_STRING) {
            char** sdata = (char**)xml_data.data;
            setReturnDataString(data_block, deblank(sdata[data_idx]), NULL);
            FreeSplitStringTokens((char***)&xml_data.data);
        } else {
            RAISE_PLUGIN_ERROR("Unsupported data type");
        }
    }

    return 0;
}

static int handle_dynamic(DATA_BLOCK* data_block, const char* experiment_mapping_file_name, const xmlChar* xPath,
                          const char* element, int index, int shot, const int* indices, size_t nindices)
{
    // DYNAMIC case

    XML_DATA xml_data;

    int status = execute_xpath_expression(experiment_mapping_file_name, xPath, index, &xml_data);
    if (status != 0) {
        return status;
    }

    if (xml_data.data_type == UDA_TYPE_STRING) {

        free(data_block->dims);
        data_block->dims = NULL;

        char** signalNames = (char**)xml_data.data;
        size_t signal_idx = 0;

        int data_n = 0;

        float** data_arrays = NULL;
        size_t n_arrays = 0;

        while (signalNames[signal_idx] != NULL) {
            char* signalName = signalNames[signal_idx];
            float coefa = (xml_data.coefas != NULL) ? xml_data.coefas[signal_idx] : 1.0f;
            float coefb = (xml_data.coefbs != NULL) ? xml_data.coefbs[signal_idx] : 0.0f;

            float* time;
            float* fdata;
            int len;

            status = mds_get(signalName, shot, &time, &fdata, &len, xml_data.time_dim);

            if (status != 0) {
                return status;
            }

            int size = xml_data.sizes[signal_idx] == 0 ? 1 : xml_data.sizes[signal_idx];
            data_n = len / size;

            int i;
            for (i = 0; i < size; ++i) {
                data_arrays = realloc(data_arrays, (n_arrays + 1) * sizeof(float*));

                data_arrays[n_arrays] = malloc(data_n * sizeof(float));

                if (StringEndsWith(element, "/time")) {
                    memcpy(data_arrays[n_arrays], &time[0], data_n * sizeof(float));
                } else {
                    int j;
                    for (j = 0; j < data_n; ++j) {
                        data_arrays[n_arrays][j] = coefa * fdata[i + j * size] + coefb;
                    }
                }

                ++n_arrays;
            }

            free(fdata);
            free(time);
            free(signalName);

            ++signal_idx;
        }

        data_block->rank = 1;
        data_block->data_type = UDA_TYPE_FLOAT;
        data_block->data_n = data_n;

        size_t sz = data_n * sizeof(float);
        data_block->data = malloc(sz);

        if (data_arrays != NULL) {
            if (nindices > 0 && indices[0] > 0) {
                memcpy(data_block->data, (char*)data_arrays[indices[0] - 1], sz);
            } else {
                memcpy(data_block->data, (char*)data_arrays[0], sz);
            }

            int i;
            for (i = 0; i < n_arrays; ++i) {
                free(data_arrays[i]);
            }
            free(data_arrays);
        }

        data_block->dims = (DIMS*)malloc(data_block->rank * sizeof(DIMS));

        int i;
        for (i = 0; i < data_block->rank; i++) {
            initDimBlock(&data_block->dims[i]);
        }

        data_block->dims[0].data_type = UDA_TYPE_FLOAT;
        data_block->dims[0].dim_n = data_n;
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

#ifndef MAX
#  define MAX(x, y) (((x) > (y)) ? (x) : (y))
#endif

static int handle_error(DATA_BLOCK* data_block, const char* experiment_mapping_file_name, const xmlChar* xPath,
                          const char* element, int index, int shot, const int* indices, size_t nindices)
{
    // ERROR case

    char* abserror = StringReplace((const char*)xPath, "/value/", "/abserror/");

    XML_DATA xml_abserror;

    int status = execute_xpath_expression(experiment_mapping_file_name, (const xmlChar*)abserror, index, &xml_abserror);
    if (status != 0) {
        return status;
    }

    char* relerror = StringReplace((const char*)xPath, "/value/", "/relerror/");

    XML_DATA xml_relerror;

    status = execute_xpath_expression(experiment_mapping_file_name, (const xmlChar*)relerror, index, &xml_relerror);
    if (status != 0) {
        return status;
    }

    XML_DATA xml_data;

    status = execute_xpath_expression(experiment_mapping_file_name, xPath, index, &xml_data);
    if (status != 0) {
        return status;
    }

    if (xml_data.data_type != UDA_TYPE_STRING) {
        RAISE_PLUGIN_ERROR("Unsupported data type");
    }

    free(data_block->dims);
    data_block->dims = NULL;

    char** signalNames = (char**)xml_data.data;
    size_t signal_idx = 0;

    int data_n = 0;

    float** error_arrays = NULL;
    size_t n_arrays = 0;

    while (signalNames[signal_idx] != NULL) {
        char* signalName = signalNames[signal_idx];
        float coefa = (xml_data.coefas != NULL) ? xml_data.coefas[signal_idx] : 1.0f;
        float coefb = (xml_data.coefbs != NULL) ? xml_data.coefbs[signal_idx] : 0.0f;

        float* time;
        float* fdata;
        int len;

        status = mds_get(signalName, shot, &time, &fdata, &len, xml_data.time_dim);
        free(time);

        if (status != 0) {
            return status;
        }

        int size = xml_data.sizes[signal_idx] == 0 ? 1 : xml_data.sizes[signal_idx];
        data_n = len / size;

        int i;
        for (i = 0; i < size; ++i) {
            error_arrays = realloc(error_arrays, (n_arrays + 1) * sizeof(float*));

            double abs = xml_abserror.values[i];
            double rel = xml_relerror.values[i];

            error_arrays[n_arrays] = malloc(data_n * sizeof(float));

            int j;
            for (j = 0; j < data_n; ++j) {
                error_arrays[n_arrays][j] = coefa * fdata[i + j * size] + coefb;

                double error = MAX(abs, rel * error_arrays[n_arrays][j]);
                if (StringEndsWith(element, "lower")) {
                    error_arrays[n_arrays][j] -= error / 2;
                } else {
                    error_arrays[n_arrays][j] += error / 2;
                }
            }

            ++n_arrays;
        }

        free(fdata);
        free(signalName);

        ++signal_idx;
    }

    data_block->rank = 1;
    data_block->data_type = UDA_TYPE_FLOAT;
    data_block->data_n = data_n;

    if (nindices > 0 && indices[0] > 0) {
        if (error_arrays != NULL) {
            size_t sz = data_n * sizeof(data_block->data_type);
            data_block->data = malloc(sz);
            memcpy(data_block->data, (char*)error_arrays[indices[0] - 1], sz);
            free(error_arrays[0]);
        }
        free(error_arrays);
    } else {
        data_block->data = (error_arrays != NULL) ? (char*)error_arrays[0] : NULL;
    }

    data_block->dims = (DIMS*)malloc(data_block->rank * sizeof(DIMS));

    int i;
    for (i = 0; i < data_block->rank; i++) {
        initDimBlock(&data_block->dims[i]);
    }

    data_block->dims[0].data_type = UDA_TYPE_FLOAT;
    data_block->dims[0].dim_n = data_n;
    data_block->dims[0].compressed = 0;
    data_block->dims[0].dim = (char*)time;

    strcpy(data_block->data_label, "");
    strcpy(data_block->data_units, "");
    strcpy(data_block->data_desc, "");

    return 0;
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
    char* mapping_file_name = getMappingFileName(IDS_version, element);

    MAPPING_TYPE request_type = NONE;
    int index = -1;
    int adjust = 0;

    const xmlChar* xPath = getMappingValue(mapping_file_name, element, &request_type, &index, &adjust);

    xPath = insertNodeIndices(xPath, &indices, &nindices);

    if (xPath == NULL) {
        return -1;
    }

    int err = 0;

    switch (request_type) {
        case CONSTANT:
            err = handle_constant(data_block, dtype, xPath);
            break;
        case STATIC:
            err = handle_static(data_block, experiment_mapping_file_name, xPath, index, indices, nindices, adjust);
            break;
        case DYNAMIC:
            err = handle_dynamic(data_block, experiment_mapping_file_name, xPath, element, index, shot, indices, nindices);
            break;
        case ERROR:
            err = handle_error(data_block, experiment_mapping_file_name, xPath, element, index, shot, indices, nindices);
            break;
        default:
            RAISE_PLUGIN_ERROR("unknown request type");
    }

    return err;
}

char* getMappingFileName(const char* IDSversion, const char* element)
{
    static char* dir = NULL;

    if (dir == NULL) {
        dir = getenv("UDA_EXP2IMAS_MAPPING_FILE_DIRECTORY");
    }

    char* slash = strchr(element, '/');
    char* token = strndup(element, slash - element);

    char* name = FormatString("%s/IMAS_mapping_%s.xml", dir, token);
    free(token);

    return name;
}

char* getMachineMappingFileName(const char* element)
{
    static char* dir = NULL;

    if (dir == NULL) {
        dir = getenv("UDA_EXP2IMAS_MAPPING_FILE_DIRECTORY");
    }

    char* slash = strchr(element, '/');
    char* token = strndup(element, slash - element);

    char* name = FormatString("%s/JET_%s.xml", dir, token);
    free(token);

    return name;
}

xmlChar* getMappingValue(const char* mapping_file_name, const char* request, MAPPING_TYPE* request_type, int* index, int* adjust)
{
    static const char* file_name = NULL;
    static xmlDocPtr doc = NULL;
    static xmlXPathContextPtr xpath_ctx = NULL;

    if (file_name == NULL) {
        // store the file name of the currently loaded XML file
        file_name = mapping_file_name;
    }

    if (!StringEquals(file_name, mapping_file_name)) {
        // clear the file so we reload
        xmlXPathFreeContext(xpath_ctx);
        xmlFreeDoc(doc);

        doc = NULL;
        xpath_ctx = NULL;

        file_name = mapping_file_name;
    }

    if (doc == NULL) {
        /*
         * Load XML document
         */
        doc = xmlParseFile(mapping_file_name);
        if (doc == NULL) {
            addIdamError(CODEERRORTYPE, __func__, 999, "unable to parse file");
            UDA_LOG(UDA_LOG_ERROR, "unable to parse file \"%s\"\n", mapping_file_name);
            return NULL;
        }
    }

    if (xpath_ctx == NULL) {
        /*
         * Create xpath evaluation context
         */
        xpath_ctx = xmlXPathNewContext(doc);
        if (xpath_ctx == NULL) {
            addIdamError(CODEERRORTYPE, __func__, 999, "unable to create new XPath context");
            UDA_LOG(UDA_LOG_ERROR, "unable to create new XPath context\n", mapping_file_name);
            return NULL;
        }
    }

    // Creating the Xpath request
    const char* fmt = "//mapping[@key='%s']/@value";
    size_t len = strlen(request) + strlen(fmt) + 1;
    xmlChar* xpath_expr = malloc(len + sizeof(xmlChar));
    xmlStrPrintf(xpath_expr, (int)len, (XML_FMT_TYPE)fmt, request);

    /*
     * Evaluate xpath expression for the type
     */
    xmlXPathObjectPtr xpath_obj = xmlXPathEvalExpression(xpath_expr, xpath_ctx);
    if (xpath_obj == NULL) {
        addIdamError(CODEERRORTYPE, __func__, 999, "unable to evaluate xpath expression");
        UDA_LOG(UDA_LOG_ERROR, "unable to evaluate xpath expression \"%s\"\n", xpath_expr);
        free(xpath_expr);
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
        addIdamError(CODEERRORTYPE, __func__, err, "no result on XPath request, no key attribute defined?");
    }

    fmt = "//mapping[@key='%s']/@type";
    len = strlen(request) + strlen(fmt) + 1;
    free(xpath_expr);
    xpath_expr = malloc(len + sizeof(xmlChar));
    xmlStrPrintf(xpath_expr, (int)len, (XML_FMT_TYPE)fmt, request);

    /*
     * Evaluate xpath expression for the type
     */
    xmlXPathFreeObject(xpath_obj);
    xpath_obj = xmlXPathEvalExpression(xpath_expr, xpath_ctx);
    if (xpath_obj == NULL) {
        addIdamError(CODEERRORTYPE, __func__, 999, "unable to evaluate xpath expression");
        UDA_LOG(UDA_LOG_ERROR, "unable to evaluate xpath expression \"%s\"\n", xpath_expr);
        free(xpath_expr);
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
        addIdamError(CODEERRORTYPE, __func__, err, "no result on XPath request, no key attribute defined?");
        return NULL;
    }

    if (type_str == NULL) {
        *request_type = NONE;
    } else if (STR_IEQUALS(type_str, "constant")) {
        *request_type = CONSTANT;
    } else if (STR_IEQUALS(type_str, "dynamic")) {
        *request_type = DYNAMIC;
    } else if (STR_IEQUALS(type_str, "static")) {
        *request_type = STATIC;
    } else if (STR_IEQUALS(type_str, "error")) {
        *request_type = ERROR;
    } else {
        addIdamError(CODEERRORTYPE, __func__, 999, "unknown mapping type");
        UDA_LOG(UDA_LOG_ERROR, "unknown mapping type \"%s\"\n", type_str);
        value = NULL;
    }

    fmt = "//mapping[@key='%s']/@index";
    len = strlen(request) + strlen(fmt) + 1;
    free(xpath_expr);
    xpath_expr = malloc(len + sizeof(xmlChar));
    xmlStrPrintf(xpath_expr, (int)len, (XML_FMT_TYPE)fmt, request);

    xmlXPathFreeObject(xpath_obj);
    xpath_obj = xmlXPathEvalExpression(xpath_expr, xpath_ctx);
    if (xpath_obj == NULL) {
        addIdamError(CODEERRORTYPE, __func__, 999, "unable to evaluate xpath expression");
        UDA_LOG(UDA_LOG_ERROR, "unable to evaluate xpath expression \"%s\"\n", xpath_expr);
        free(xpath_expr);
        return NULL;
    }

    nodes = xpath_obj->nodesetval;

    if (nodes != NULL && nodes->nodeNr > 0) {
        current_node = nodes->nodeTab[0];
        current_node = current_node->children;
        *index = (int)strtol((char*)current_node->content, NULL, 10);
    }

    fmt = "//mapping[@key='%s']/@adjust";
    len = strlen(request) + strlen(fmt) + 1;
    free(xpath_expr);
    xpath_expr = malloc(len + sizeof(xmlChar));
    xmlStrPrintf(xpath_expr, (int)len, (XML_FMT_TYPE)fmt, request);

    xmlXPathFreeObject(xpath_obj);
    xpath_obj = xmlXPathEvalExpression(xpath_expr, xpath_ctx);
    if (xpath_obj != NULL) {
        nodes = xpath_obj->nodesetval;

        if (nodes != NULL && nodes->nodeNr > 0) {
            current_node = nodes->nodeTab[0];
            current_node = current_node->children;
            *adjust = (int)strtol((char*)current_node->content, NULL, 10);
        }
    }

    free(xpath_expr);
    xmlXPathFreeObject(xpath_obj);

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
