#include "exp2imas_xml.h"

#include <assert.h>
#include <regex.h>

#include <clientserver/initStructs.h>
#include <clientserver/udaTypes.h>
#include <clientserver/errorLog.h>
#include <logging/logging.h>
#include <clientserver/stringUtils.h>
#include <plugins/udaPlugin.h>

static int convertToInt(char* value);
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

int execute_xpath_expression(const char* filename, const xmlChar* xpathExpr, char** data, int* data_type, int* time_dim,
                             int** sizes, float** coefas, float** coefbs, int index, int** dims, int* rank)
{
    assert(filename);
    assert(xpathExpr);

    static xmlDocPtr doc = NULL;

    if (doc == NULL) {
        /* Load XML document */
        doc = xmlParseFile(filename);
        if (doc == NULL) {
            IDAM_LOGF(UDA_LOG_ERROR, "Error: unable to parse file \"%s\"\n", filename);
            return -1;
        }
    }

    static xmlXPathContextPtr xpathCtx = NULL;

    if (xpathCtx == NULL) {
        /* Create xpath evaluation context */
        xpathCtx = xmlXPathNewContext(doc);
        if (xpathCtx == NULL) {
            IDAM_LOG(UDA_LOG_ERROR, "Error: unable to create new XPath context\n");
//            xmlFreeDoc(doc);
            return -1;
        }

    }

    *dims = get_dims(xpathExpr, xpathCtx, rank);
    char* type = get_type(xpathExpr, xpathCtx);
    *time_dim = get_time_dim(xpathExpr, xpathCtx);
    *sizes = get_sizes(xpathExpr, xpathCtx);
    get_coefs(coefas, coefbs, xpathExpr, xpathCtx);

    /* Evaluate xpath expression for requesting the data  */
    xmlXPathObjectPtr xpathObj = xmlXPathEvalExpression(xpathExpr, xpathCtx);

    if (xpathObj == NULL) {
        IDAM_LOGF(UDA_LOG_ERROR, "Error: unable to evaluate xpath expression \"%s\"\n", xpathExpr);
//        xmlXPathFreeContext(xpathCtx);
//        xmlFreeDoc(doc);
        return -1;
    }

    xmlNodeSetPtr nodes = xpathObj->nodesetval;

    if (nodes == NULL || nodes->nodeNr == 0) {
        IDAM_LOG(UDA_LOG_ERROR, "error in XPath request  \n");
//        xmlXPathFreeContext(xpathCtx);
//        xmlFreeDoc(doc);
        return -1;
    }

    *data_type = convertToInt(type);
    int i;

    if (*dims == NULL && index == -1) {
        index = 1;
    }

    size_t data_n = 1;
    if (*dims != NULL) {
        for (i = 0; i < *rank; ++i) {
            data_n *= (*dims)[i];
        }
    } else {
        data_n = 1;
    }

    double* content = NULL;
    if (*data_type != UDA_TYPE_STRING) {
        xmlNodePtr cur = nodes->nodeTab[0];

        if (cur->name == NULL) {
            IDAM_LOG(UDA_LOG_ERROR, "Error: null pointer (nodes->nodeTab[nodeindex]->name) \n");
//            xmlXPathFreeContext(xpathCtx);
//            xmlFreeDoc(doc);
            return -1;
        }

        size_t n_vals = 0;
        if (*dims == NULL && index > 0) {
            content = getContent(cur, &n_vals);
        } else {
            content = getContent(cur, &n_vals);
            if (n_vals != data_n) {
                IDAM_LOG(UDA_LOG_ERROR, "Error: incorrect number of points read from XML file\n");
                return -1;
            }
        }
    }

    if (*data_type == UDA_TYPE_DOUBLE) {
        *data = malloc(data_n * sizeof(double));
        if (*dims == NULL) {
            ((double*)*data)[0] = content[index-1];
        } else {
            for (i = 0; i < (*dims)[0]; i++) {
                ((double*)*data)[i] = content[i];
            }
        }
    } else if (*data_type == UDA_TYPE_FLOAT) {
        *data = malloc(data_n * sizeof(float));
        if (*dims == NULL) {
            ((float*)*data)[0] = (float)content[index-1];
        } else {
            for (i = 0; i < data_n; i++) {
                ((float*)*data)[i] = (float)content[i];
            }
        }
    } else if (*data_type == UDA_TYPE_LONG) {
        *data = malloc(data_n * sizeof(long));
        if (*dims == NULL) {
            ((long*)*data)[0] = (long)content[index-1];
        } else {
            for (i = 0; i < data_n; i++) {
                ((long*)*data)[i] = (long)content[i];
            }
        }
    } else if (*data_type == UDA_TYPE_INT) {
        *data = malloc(data_n * sizeof(int));
        if (*dims == NULL) {
            ((int*)*data)[0] = (int)content[index-1];
        } else {
            for (i = 0; i < data_n; i++) {
                ((int*)*data)[i] = (int)content[i];
            }
        }
    } else if (*data_type == UDA_TYPE_SHORT) {
        *data = malloc(data_n * sizeof(short));
        if (*dims == NULL) {
            ((short*)*data)[0] = (short)content[index-1];
        } else {
            for (i = 0; i < data_n; i++) {
                ((short*)*data)[i] = (short)content[i];
            }
        }
    } else if (*data_type == UDA_TYPE_STRING) {
        char** strings = NULL;
        size_t n_strings = 0;

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

        *data = (char*)strings;
    } else {
        xmlXPathFreeObject(xpathObj);
        RAISE_PLUGIN_ERROR("Unsupported data type");
    }

    /* Cleanup */
    xmlXPathFreeObject(xpathObj);
//    xmlXPathFreeContext(xpathCtx);
//    xmlFreeDoc(doc);

    return 0;
}

double* getContent(xmlNode* node, size_t* n_vals)
{
    xmlChar* content = node->children->content;

    const char* chr = (const char*)content;
    *n_vals = 0;

    double* vals = NULL;

    bool in_expand = false;
    bool have_expand_val = false;
    double expand_val = 0;
    int expand_start = 0;
    int expand_end = 0;

    const char* num_start = chr;

    bool cont = true;

    while (cont) {
        switch (*chr) {
            case ',':
            case '\0':
                if (in_expand) {
                    if (have_expand_val) {
                        expand_start = (int)strtol(num_start, NULL, 10);
                        num_start = chr + 1;
                    } else {
                        expand_val = strtod(num_start, NULL);
                        have_expand_val = true;
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
                    vals = realloc(vals, (*n_vals + 1) * sizeof(double));
                    vals[*n_vals] = expand_val;
                    ++(*n_vals);
                }
                if (*(chr + 1) == ',') {
                    ++chr;
                }
                num_start = chr + 1;
                in_expand = false;
                have_expand_val = false;
                expand_val = 0;
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


int convertToInt(char* value)
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
    } else if (StringEquals(value, "matint_type") || StringEquals(value, "vecint_type")
               || StringEquals(value, "xs:integer") || StringEquals(value, "INT_0D")) {
        i = UDA_TYPE_INT;
    } else {
        err = 999;
        addIdamError(CODEERRORTYPE, __func__, err, "Unsupported data type");
    }
    return i;
}
