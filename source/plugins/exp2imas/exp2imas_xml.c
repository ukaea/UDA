#include "exp2imas_xml.h"

#include <assert.h>
#include <regex.h>

#include <clientserver/initStructs.h>
#include <clientserver/udaTypes.h>
#include <clientserver/errorLog.h>
#include <logging/logging.h>
#include <clientserver/stringUtils.h>
#include <plugins/udaPlugin.h>

static int convertTypeStringToUDAType(char* value);
static double* getContent(xmlNode* node, size_t* n_vals);

static char* get_type(const xmlChar* xpathExpr, xmlXPathContextPtr xpathCtx)
{
    char* type = "xs:integer";

    /* First, we get the type of the element which is requested */

    //Creating the Xpath request for the type which does not exist necessarly (it depends on the XML element which is requested)
    char* typeStr = "/@type";
    size_t len = 1 + xmlStrlen(xpathExpr) + strlen(typeStr);
    xmlChar* typeXpathExpr = (xmlChar*)malloc(len * sizeof(xmlChar));
    xmlStrPrintf(typeXpathExpr, (int)len, (XML_FMT_TYPE)"%s%s", xpathExpr, typeStr);

    /* Evaluate xpath expression for the type */
    xmlXPathObjectPtr xpathObj = xmlXPathEvalExpression(typeXpathExpr, xpathCtx);
    if (xpathObj != NULL) {
        xmlNodeSetPtr nodes = xpathObj->nodesetval;

        xmlNodePtr cur;

        if (nodes != NULL && nodes->nodeNr > 0) {
            cur = nodes->nodeTab[0];
            cur = cur->children;
            type = (char*)cur->content;
        }
    }

    free(typeXpathExpr);

    return type;
}

static int* get_dims(const xmlChar* xpathExpr, xmlXPathContextPtr xpathCtx, int* rank)
{
    int* dims = NULL;
    *rank = 0;

    /* First, we get the type of the element which is requested */

    //Creating the Xpath request for the type which does not exist necessarly (it depends on the XML element which is requested)
    char* typeStr = "/@dim";
    size_t len = 1 + xmlStrlen(xpathExpr) + strlen(typeStr);
    xmlChar* typeXpathExpr = (xmlChar*)malloc(len * sizeof(xmlChar));
    xmlStrPrintf(typeXpathExpr, (int)len, (XML_FMT_TYPE)"%s%s", xpathExpr, typeStr);

    /* Evaluate xpath expression for the type */
    xmlXPathObjectPtr xpathObj = xmlXPathEvalExpression(typeXpathExpr, xpathCtx);
    if (xpathObj != NULL) {
        xmlNodeSetPtr nodes = xpathObj->nodesetval;

        xmlNodePtr cur;

        if (nodes != NULL && nodes->nodeNr > 0) {
            cur = nodes->nodeTab[0];
            cur = cur->children;

            char** tokens = SplitString((char*)cur->content, ",");
            int i = 0;
            while (tokens[i] != NULL) {
                dims = realloc(dims, (i + 1) * sizeof(int));
                dims[i] = (int)strtol(tokens[i], NULL, 10);
                ++i;
            }
            FreeSplitStringTokens(&tokens);

            *rank = i;
        }
    }

    free(typeXpathExpr);

    return dims;
}

static int get_time_dim(const xmlChar* xpathExpr, xmlXPathContextPtr xpathCtx)
{
    int time_dim = 0;

    /* First, we get the type of the element which is requested */

    //Creating the Xpath request for the type which does not exist necessarly (it depends on the XML element which is requested)
    char* typeStr = "/../time_dim";
    size_t len = 1 + xmlStrlen(xpathExpr) + strlen(typeStr);
    xmlChar* typeXpathExpr = (xmlChar*)malloc(len * sizeof(xmlChar));
    xmlStrPrintf(typeXpathExpr, (int)len, (XML_FMT_TYPE)"%s%s", xpathExpr, typeStr);

    /* Evaluate xpath expression for the type */
    xmlXPathObjectPtr xpathObj = xmlXPathEvalExpression(typeXpathExpr, xpathCtx);
    if (xpathObj != NULL) {
        xmlNodeSetPtr nodes = xpathObj->nodesetval;

        xmlNodePtr cur;

        if (nodes != NULL && nodes->nodeNr > 0) {
            cur = nodes->nodeTab[0];
            cur = cur->children;
            time_dim = (int)strtol((char*)cur->content, NULL, 10);
        }
    }

    free(typeXpathExpr);

    return time_dim;
}

static char* get_download(const xmlChar* xpathExpr, xmlXPathContextPtr xpathCtx, double** values, size_t* n_values)
{
    *values = NULL;
    *n_values = 0;

    char* download_type = 0;

    /* First, we get the type of the element which is requested */

    const char* typeStr = "/../download/download";
    size_t len = 1 + xmlStrlen(xpathExpr) + strlen(typeStr);
    xmlChar* typeXpathExpr = (xmlChar*)malloc(len * sizeof(xmlChar));
    xmlStrPrintf(typeXpathExpr, (int)len, (XML_FMT_TYPE)"%s%s", xpathExpr, typeStr);

    /* Evaluate xpath expression for the type */
    xmlXPathObjectPtr xpathObj = xmlXPathEvalExpression(typeXpathExpr, xpathCtx);
    if (xpathObj != NULL) {
        xmlNodeSetPtr nodes = xpathObj->nodesetval;

        xmlNodePtr cur;

        if (nodes != NULL && nodes->nodeNr > 0) {
            cur = nodes->nodeTab[0];
            cur = cur->children;
            download_type = (char*)cur->content;
        }
    }

    free(typeXpathExpr);

    if (StringEquals(download_type, "fixed")) {
        char** value_strings = NULL;
        char** repeat_strings = NULL;

        const char* valueStr = "/../download/fixed_value";
        len = 1 + xmlStrlen(xpathExpr) + strlen(valueStr);
        typeXpathExpr = (xmlChar*)malloc(len * sizeof(xmlChar));
        xmlStrPrintf(typeXpathExpr, (int)len, (XML_FMT_TYPE)"%s%s", xpathExpr, valueStr);

        xpathObj = xmlXPathEvalExpression(typeXpathExpr, xpathCtx);
        if (xpathObj != NULL) {
            xmlNodeSetPtr nodes = xpathObj->nodesetval;

            xmlNodePtr cur;

            if (nodes != NULL && nodes->nodeNr > 0) {
                cur = nodes->nodeTab[0];
                cur = cur->children;
                value_strings = SplitString((const char*)cur->content, " ");
            }
        }

        xmlXPathFreeObject(xpathObj);
        free(typeXpathExpr);

        const char* repeatStr = "/../download/fixed_repeat";
        len = 1 + xmlStrlen(xpathExpr) + strlen(repeatStr);
        typeXpathExpr = (xmlChar*)malloc(len * sizeof(xmlChar));
        xmlStrPrintf(typeXpathExpr, (int)len, (XML_FMT_TYPE)"%s%s", xpathExpr, repeatStr);

        xpathObj = xmlXPathEvalExpression(typeXpathExpr, xpathCtx);
        if (xpathObj != NULL) {
            xmlNodeSetPtr nodes = xpathObj->nodesetval;

            xmlNodePtr cur;

            if (nodes != NULL && nodes->nodeNr > 0) {
                cur = nodes->nodeTab[0];
                cur = cur->children;
                repeat_strings = SplitString((const char*)cur->content, " ");
            }
        }

        xmlXPathFreeObject(xpathObj);
        free(typeXpathExpr);

        int idx = 0;
        while (value_strings[idx] != NULL) {
            if (repeat_strings[idx] == NULL) {
                addIdamError(CODEERRORTYPE, __func__, 999, "mis-matching number of values in fixed download");
                return NULL;
            }

            double value = strtod(value_strings[idx], NULL);
            long repeat = strtol(repeat_strings[idx], NULL, 10);

            *values = (double*)realloc(*values, (*n_values + repeat) * sizeof(double));

            int i;
            for (i = 0; i < repeat; ++i) {
                (*values)[*n_values + i] = value;
            }

            *n_values += repeat;

            ++idx;
        }
    }

    return download_type;
}

static int* get_sizes(const xmlChar* xpathExpr, xmlXPathContextPtr xpathCtx)
{
    int* sizes = NULL;

    /* First, we get the type of the element which is requested */

    //Creating the Xpath request for the type which does not exist necessarly (it depends on the XML element which is requested)
    char* typeStr = "/../size";
    size_t len = 1 + xmlStrlen(xpathExpr) + strlen(typeStr);
    xmlChar* typeXpathExpr = (xmlChar*)malloc(len * sizeof(xmlChar));
    xmlStrPrintf(typeXpathExpr, (int)len, (XML_FMT_TYPE)"%s%s", xpathExpr, typeStr);

    /* Evaluate xpath expression for the type */
    xmlXPathObjectPtr xpathObj = xmlXPathEvalExpression(typeXpathExpr, xpathCtx);
    if (xpathObj != NULL) {
        xmlNodeSetPtr nodes = xpathObj->nodesetval;

        xmlNodePtr cur;

        if (nodes != NULL && nodes->nodeNr > 0) {
            sizes = malloc(nodes->nodeNr * sizeof(int));

            int i;
            for (i = 0; i < nodes->nodeNr; ++i) {
                cur = nodes->nodeTab[i];
                cur = cur->children;
                sizes[i] = (int)strtol((char*)cur->content, NULL, 10);
            }
        }
    }

    free(typeXpathExpr);

    return sizes;
}

static void get_coefs(float** coefas, float** coefbs, const xmlChar* xpathExpr, xmlXPathContextPtr xpathCtx)
{
    *coefas = NULL;
    *coefbs = NULL;

    char* typeStr = "/../coefa";
    size_t len = 1 + xmlStrlen(xpathExpr) + strlen(typeStr);
    xmlChar* typeXpathExpr = (xmlChar*)malloc(len * sizeof(xmlChar));
    xmlStrPrintf(typeXpathExpr, (int)len, (XML_FMT_TYPE)"%s%s", xpathExpr, typeStr);

    /* Evaluate xpath expression for the type */
    xmlXPathObjectPtr xpathObj = xmlXPathEvalExpression(typeXpathExpr, xpathCtx);
    if (xpathObj != NULL) {
        xmlNodeSetPtr nodes = xpathObj->nodesetval;

        xmlNodePtr cur;

        if (nodes != NULL && nodes->nodeNr > 0) {
            *coefas = malloc(nodes->nodeNr * sizeof(int));

            int i;
            for (i = 0; i < nodes->nodeNr; ++i) {
                cur = nodes->nodeTab[i];
                cur = cur->children;
                char* endptr = NULL;
                (*coefas)[i] = strtof((char*)cur->content, &endptr);
                if (endptr == (char*)cur->content) {
                    (*coefas)[i] = 1.0f;
                }
            }
        }
    }

    free(typeXpathExpr);

    typeStr = "/../coefb";
    len = 1 + xmlStrlen(xpathExpr) + strlen(typeStr);
    typeXpathExpr = (xmlChar*)malloc(len * sizeof(xmlChar));
    xmlStrPrintf(typeXpathExpr, (int)len, (XML_FMT_TYPE)"%s%s", xpathExpr, typeStr);

    /* Evaluate xpath expression for the type */
    xpathObj = xmlXPathEvalExpression(typeXpathExpr, xpathCtx);
    if (xpathObj != NULL) {
        xmlNodeSetPtr nodes = xpathObj->nodesetval;

        xmlNodePtr cur;

        if (nodes != NULL && nodes->nodeNr > 0) {
            *coefbs = malloc(nodes->nodeNr * sizeof(int));

            int i;
            for (i = 0; i < nodes->nodeNr; ++i) {
                cur = nodes->nodeTab[i];
                cur = cur->children;
                char* endptr = NULL;
                (*coefbs)[i] = strtof((char*)cur->content, &endptr);
                if (endptr == (char*)cur->content) {
                    (*coefbs)[i] = 0.0f;
                }
            }
        }
    }

    free(typeXpathExpr);
}

int execute_xpath_expression(const char* filename, const xmlChar* xpathExpr, int index, XML_DATA* xml_data)
{
    assert(filename);
    assert(xpathExpr);

    static const char* file_name = NULL;
    static xmlDocPtr doc = NULL;
    static xmlXPathContextPtr xpathCtx = NULL;

    if (file_name == NULL) {
        // store the file name of the currently loaded XML file
        file_name = filename;
    }

    if (!StringEquals(file_name, filename)) {
        // clear the file so we reload
        xmlXPathFreeContext(xpathCtx);
        xmlFreeDoc(doc);

        doc = NULL;
        xpathCtx = NULL;

        file_name = filename;
    }

    if (doc == NULL) {
        /* Load XML document */
        doc = xmlParseFile(filename);
        if (doc == NULL) {
            UDA_LOG(UDA_LOG_ERROR, "Error: unable to parse file \"%s\"\n", filename);
            return -1;
        }
    }

    if (xpathCtx == NULL) {
        /* Create xpath evaluation context */
        xpathCtx = xmlXPathNewContext(doc);
        if (xpathCtx == NULL) {
            UDA_LOG(UDA_LOG_ERROR, "Error: unable to create new XPath context\n");
            return -1;
        }
    }

    memset(xml_data, '\0', sizeof(XML_DATA));

    xml_data->dims = get_dims(xpathExpr, xpathCtx, &xml_data->rank);
    char* type = get_type(xpathExpr, xpathCtx);
    xml_data->time_dim = get_time_dim(xpathExpr, xpathCtx);
    xml_data->sizes = get_sizes(xpathExpr, xpathCtx);
    get_coefs(&xml_data->coefas, &xml_data->coefbs, xpathExpr, xpathCtx);
    xml_data->download = get_download(xpathExpr, xpathCtx, &xml_data->values, &xml_data->n_values);

    /* Evaluate xpath expression for requesting the data  */
    xmlXPathObjectPtr xpathObj = xmlXPathEvalExpression(xpathExpr, xpathCtx);

    if (xpathObj == NULL) {
        UDA_LOG(UDA_LOG_ERROR, "Error: unable to evaluate xpath expression \"%s\"\n", xpathExpr);
        return -1;
    }

    xmlNodeSetPtr nodes = xpathObj->nodesetval;

    if (nodes == NULL || nodes->nodeNr == 0) {
        UDA_LOG(UDA_LOG_ERROR, "error in XPath request  \n");
        return -1;
    }

    xml_data->data_type = convertTypeStringToUDAType(type);
    int i;

    if (xml_data->dims == NULL && index == -1) {
        index = 1;
    }

    size_t data_n = 1;
    if (xml_data->dims != NULL) {
        for (i = 0; i < xml_data->rank; ++i) {
            data_n *= (xml_data->dims)[i];
        }
    } else {
        data_n = 1;
    }

    double* content = NULL;
    if (xml_data->data_type != UDA_TYPE_STRING) {
        xmlNodePtr cur = nodes->nodeTab[0];

        if (cur->name == NULL) {
            UDA_LOG(UDA_LOG_ERROR, "Error: null pointer (nodes->nodeTab[nodeindex]->name) \n");
            return -1;
        }

        size_t n_vals = 0;
        if (xml_data->dims == NULL && index > 0) {
            content = getContent(cur, &n_vals);
        } else {
            content = getContent(cur, &n_vals);
            if (n_vals != data_n) {
                UDA_LOG(UDA_LOG_ERROR, "Error: incorrect number of points read from XML file\n");
                return -1;
            }
        }
    }

    if (xml_data->data_type == UDA_TYPE_DOUBLE) {
        xml_data->data = malloc(data_n * sizeof(double));
        if (xml_data->dims == NULL) {
            ((double*)xml_data->data)[0] = content[index-1];
        } else {
            for (i = 0; i < (xml_data->dims)[0]; i++) {
                ((double*)xml_data->data)[i] = content[i];
            }
        }
    } else if (xml_data->data_type == UDA_TYPE_FLOAT) {
        xml_data->data = malloc(data_n * sizeof(float));
        if (xml_data->dims == NULL) {
            ((float*)xml_data->data)[0] = (float)content[index-1];
        } else {
            for (i = 0; i < data_n; i++) {
                ((float*)xml_data->data)[i] = (float)content[i];
            }
        }
    } else if (xml_data->data_type == UDA_TYPE_LONG) {
        xml_data->data = malloc(data_n * sizeof(long));
        if (xml_data->dims == NULL) {
            ((long*)xml_data->data)[0] = (long)content[index-1];
        } else {
            for (i = 0; i < data_n; i++) {
                ((long*)xml_data->data)[i] = (long)content[i];
            }
        }
    } else if (xml_data->data_type == UDA_TYPE_INT) {
        xml_data->data = malloc(data_n * sizeof(int));
        if (xml_data->dims == NULL) {
            ((int*)xml_data->data)[0] = (int)content[index-1];
        } else {
            for (i = 0; i < data_n; i++) {
                ((int*)xml_data->data)[i] = (int)content[i];
            }
        }
    } else if (xml_data->data_type == UDA_TYPE_SHORT) {
        xml_data->data = malloc(data_n * sizeof(short));
        if (xml_data->dims == NULL) {
            ((short*)xml_data->data)[0] = (short)content[index-1];
        } else {
            for (i = 0; i < data_n; i++) {
                ((short*)xml_data->data)[i] = (short)content[i];
            }
        }
    } else if (xml_data->data_type == UDA_TYPE_STRING) {
        char** strings = NULL;
        size_t n_strings = 0;

        if (nodes->nodeNr == 1 && StringIEquals((char*)nodes->nodeTab[0]->children->content, "Put value here")) {
            xmlXPathFreeObject(xpathObj);
            return -1;
        }

        for (i = 0; i < nodes->nodeNr; ++i) {
            xmlNodePtr cur = nodes->nodeTab[i];

            char* string = (char*)cur->children->content;
            if (STR_STARTSWITH(type, "vec")) {
                char** tokens = SplitString(string, ",");
                int j = 0;
                while (tokens[j] != NULL) {
                    strings = realloc(strings, (n_strings + 1) * sizeof(char*));
                    strings[n_strings] = tokens[j];
                    ++n_strings;
                    ++j;
                }
                free(tokens);
            } else {
                strings = realloc(strings, (n_strings + 1) * sizeof(char*));
                strings[n_strings] = strdup(string);
                ++n_strings;
            }
        }

        strings = realloc(strings, (n_strings + 1) * sizeof(char*));
        strings[n_strings] = NULL;

        xml_data->data = (char*)strings;
    } else {
        xmlXPathFreeObject(xpathObj);
        RAISE_PLUGIN_ERROR("Unsupported data type");
    }

    /* Cleanup */
    xmlXPathFreeObject(xpathObj);

    return 0;
}

double* getContent(xmlNode* node, size_t* n_vals)
{
    xmlChar* content = node->children->content;

    const char* chr = (const char*)content;
    *n_vals = 0;

    double* vals = NULL;

    bool in_expand = false;
    bool in_expand_range = false;
    size_t num_expand_vals = 0;
    double* expand_vals = NULL;
    int expand_start = 0;
    int expand_end = 0;

    const char* num_start = chr;

    bool cont = true;

    while (cont) {
        switch (*chr) {
            case ',':
            case '\0':
                if (in_expand) {
                    if (in_expand_range) {
                        expand_start = (int)strtol(num_start, NULL, 10);
                        num_start = chr + 1;
                    } else {
                        expand_vals = realloc(expand_vals, (num_expand_vals + 1) * sizeof(double));
                        expand_vals[num_expand_vals] = strtod(num_start, NULL);
                        ++num_expand_vals;
                        num_start = chr + 1;
                    }
                } else if (*num_start != '\0') {
                    vals = realloc(vals, (*n_vals + 1) * sizeof(double));
                    vals[*n_vals] = strtod(num_start, NULL);
                    ++(*n_vals);
                    num_start = chr + 1;
                }
                if (*chr == '\0') {
                    cont = false;
                }
                break;
            case '=':
                in_expand_range = true;
                num_start = chr + 1;
                break;
            case '(':
                in_expand = true;
                num_start = chr + 1;
                break;
            case ')': {
                expand_end = (int)strtol(num_start, NULL, 10);
                int i = 0;
                for (i = expand_start; i <= expand_end; ++i) {
                    vals = realloc(vals, (*n_vals + num_expand_vals) * sizeof(double));
                    int j;
                    for (j = 0; j < num_expand_vals; ++j) {
                        vals[*n_vals + j] = expand_vals[j];
                    }
                    *n_vals += num_expand_vals;
                }
                if (*(chr + 1) == ',') {
                    ++chr;
                }
                num_start = chr + 1;
                in_expand = false;
                in_expand_range = false;
                free(expand_vals);
                expand_vals = NULL;
                num_expand_vals = 0;
                expand_start = 0;
                expand_end = 0;
                break;
            }
            default:
                break;
        }
        ++chr;
    }

    return vals;
}

int convertTypeStringToUDAType(char* value)
{
    int i = UDA_TYPE_UNKNOWN;
    int err = 0;

    if (StringEquals(value, "matstring_type") || StringEquals(value, "vecstring_type")
        || StringEquals(value, "xs:string") || StringEquals(value, "STR_0D")) {
        i = UDA_TYPE_STRING;
    } else if (StringEquals(value, "matflt_type") || StringEquals(value, "vecflt_type")
               || StringEquals(value, "array3dflt_type") || StringEquals(value, "xs:float")
               || StringEquals(value, "FLT_0D")) {
        i = UDA_TYPE_FLOAT;
    } else if (StringEquals(value, "matint_type") || StringEquals(value, "vecint_type") || StringEquals(value, "array3dint_type")
               || StringEquals(value, "xs:integer") || StringEquals(value, "INT_0D")) {
        i = UDA_TYPE_INT;
    } else {
        err = 999;
        addIdamError(CODEERRORTYPE, __func__, err, "Unsupported data type");
    }
    return i;
}

