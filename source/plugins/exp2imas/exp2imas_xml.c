#include "exp2imas_xml.h"

#include <assert.h>
#include <regex.h>

#include <clientserver/initStructs.h>
#include <clientserver/udaTypes.h>
#include <clientserver/errorLog.h>
#include <logging/logging.h>
#include <clientserver/stringUtils.h>

static int convertToInt(char* value);
static char** getContent(xmlNode* node, int dim);
static xmlChar* insertNodeIndices(const xmlChar* xpathExpr, int* nodeIndices);

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

static int get_dim(const xmlChar* xpathExpr, xmlXPathContextPtr xpathCtx)
{
    int dim = 1;

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
            dim = atoi((char*)cur->content);
        }
    }

    free(typeXpathExpr);

    return dim;
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
            time_dim = atoi((char*)cur->content);
        }
    }

    free(typeXpathExpr);

    return time_dim;
}

static int get_size(const xmlChar* xpathExpr, xmlXPathContextPtr xpathCtx)
{
    int size = 0;

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
            cur = nodes->nodeTab[0];
            cur = cur->children;
            size = atoi((char*)cur->content);
        }
    }

    free(typeXpathExpr);

    return size;
}

/**
 * execute_xpath_expression:
 * @filename:		the input XML filename.
 * @xpathExpr:		the xpath expression for evaluation.
 * @nsList:		the optional list of known namespaces in 
 *			"<prefix1>=<href1> <prefix2>=href2> ..." format.
 *
 * Parses input XML file, evaluates XPath expression and prints results.
 *
 * Returns 0 on success and a negative value otherwise.
 */
int execute_xpath_expression(const char* filename, const xmlChar* xpathExpr, int* nodeIndices, char** data, int* data_type, int* time_dim, int* size)
{
    xmlDocPtr doc;
    xmlXPathContextPtr xpathCtx;
    xmlXPathObjectPtr xpathObj;

    assert(filename);
    assert(xpathExpr);

    /* Load XML document */
    doc = xmlParseFile(filename);
    if (doc == NULL) {
        fprintf(stderr, "Error: unable to parse file \"%s\"\n", filename);
        return -1;
    }

    /* Create xpath evaluation context */
    xpathCtx = xmlXPathNewContext(doc);
    if (xpathCtx == NULL) {
        fprintf(stderr, "Error: unable to create new XPath context\n");
        xmlFreeDoc(doc);
        return -1;
    }

    xpathExpr = insertNodeIndices(xpathExpr, nodeIndices);

    int dim = get_dim(xpathExpr, xpathCtx);
    char* type = get_type(xpathExpr, xpathCtx);
    *time_dim = get_time_dim(xpathExpr, xpathCtx);
    *size = get_size(xpathExpr, xpathCtx);

    /* Evaluate xpath expression for requesting the data  */
    xpathObj = xmlXPathEvalExpression(xpathExpr, xpathCtx);

    if (xpathObj == NULL) {
        fprintf(stderr, "Error: unable to evaluate xpath expression \"%s\"\n", xpathExpr);
        xmlXPathFreeContext(xpathCtx);
        xmlFreeDoc(doc);
        return -1;
    }

    xmlNodeSetPtr nodes = xpathObj->nodesetval;

    xmlNodePtr cur;
    int err = 0;

    if (nodes == NULL || nodes->nodeNr == 0) {
        fprintf(stderr, "error in XPath request  \n");
        xmlXPathFreeContext(xpathCtx);
        xmlFreeDoc(doc);
        return -1;
    }

    nodes = xpathObj->nodesetval;

    cur = nodes->nodeTab[0];

    if (cur->name == NULL) {
        fprintf(stderr, "Error: null pointer (nodes->nodeTab[nodeindex]->name) \n");
        xmlXPathFreeContext(xpathCtx);
        xmlFreeDoc(doc);
        return -1;
    }

    *data_type = convertToInt(type);
    int i;

    if (*data_type == TYPE_DOUBLE) {
        *data = malloc(dim * sizeof(double));
        char** content = getContent(cur, dim);
        for (i = 0; i < dim; i++) {
            ((double*)*data)[i] = atof(content[i]);
        }
    } else if (*data_type == TYPE_FLOAT) {
        *data = malloc(dim * sizeof(float));
        char** content = getContent(cur, dim);
        for (i = 0; i < dim; i++) {
            ((float*)*data)[i] = (float)atof(content[i]);
        }
    } else if (*data_type == TYPE_LONG) {
        *data = malloc(dim * sizeof(long));
        char** content = getContent(cur, dim);
        for (i = 0; i < dim; i++) {
            ((long*)*data)[i] = atol(content[i]);
        }
    } else if (*data_type == TYPE_INT) {
        *data = malloc(dim * sizeof(int));
        char** content = getContent(cur, dim);
        for (i = 0; i < dim; i++) {
            ((int*)*data)[i] = atoi(content[i]);
        }
    } else if (*data_type == TYPE_SHORT) {
        *data = malloc(dim * sizeof(short));
        char** content = getContent(cur, dim);
        for (i = 0; i < dim; i++) {
            ((short*)*data)[i] = (short)atoi(content[i]);
        }
    } else if (*data_type == TYPE_STRING) {
        char* content = (char*)cur->children->content;
        if (STR_STARTSWITH(type, "vec")) {
            char** tokens = SplitString(content, ",");
            *data = (char*)tokens;
        } else {
            char** temp = (char**)malloc(2 * sizeof(char*));
            temp[0] = strdup(content);
            temp[1] = NULL;
            *data = (char*)temp;
        }
    } else {
        err = 999;
        addIdamError(&idamerrorstack, CODEERRORTYPE, __func__, err, "Unsupported data type");
    }

    /* Cleanup */
    xmlXPathFreeObject(xpathObj);
    xmlXPathFreeContext(xpathCtx);
    xmlFreeDoc(doc);

    return 0;
}

char** getContent(xmlNode* node, int dim)
{
    char** data = malloc(dim * sizeof(char*));
    xmlChar* content = node->children->content;

    const char* pattern = "\\(([0-9.]+), i=([0-9]+),([0-9]+)\\)";
    regex_t preg;

    int rc = regcomp(&preg, pattern, REG_EXTENDED);
    if (rc != 0) {
        printf("regcomp() failed\n");
        return 0;
    }

    const char* work = (const char*)content;
    size_t nmatch = 4;
    regmatch_t pmatch[4];

    rc = regexec(&preg, work, nmatch, pmatch, 0);
    if (rc == 0) {
        while (rc == 0) {
            double num = strtod(&work[pmatch[1].rm_so], NULL);
            long start_index = strtol(&work[pmatch[2].rm_so], NULL, 10);
            long end_index = strtol(&work[pmatch[3].rm_so], NULL, 10);

            long i;
            for (i = start_index; i <= end_index; ++i) {
                data[i-1] = FormatString("%g", num);
            }

            work = &work[pmatch[0].rm_eo];
            rc = regexec(&preg, work, nmatch, pmatch, 0);
        }

        return data;
    }

    char temp[128];
    int i = 0;
    int dim_i = 0;
    int n = 0;
    for (;; ++i) {
        char c = content[i];

        if (c == ' ') {
            continue;
        }
        if (c == ',' || c == '\0') {
            temp[n] = '\0';
            assert(dim_i < dim);
            data[dim_i++] = strdup(temp);
            n = 0;
            if (c == '\0' || dim_i == dim) {
                break;
            }
            continue;
        }
        assert(n < 128);
        temp[n++] = c;
    }

    assert(dim_i == dim);
    return data;
}


int convertToInt(char* value)
{
    int i = TYPE_UNKNOWN;
    int err = 0;

    if (STR_EQUALS(value, "vecstring_type") || STR_EQUALS(value, "xs:string") || STR_EQUALS(value, "STR_0D")) {
        i = TYPE_STRING;
    } else if (STR_EQUALS(value, "vecflt_type") || STR_EQUALS(value, "xs:float") || STR_EQUALS(value, "FLT_0D")) {
        i = TYPE_FLOAT;
    } else if (STR_EQUALS(value, "vecint_type") || STR_EQUALS(value, "xs:integer") || STR_EQUALS(value, "INT_0D")) {
        i = TYPE_INT;
    } else {
        err = 999;
        addIdamError(&idamerrorstack, CODEERRORTYPE, __func__, err, "Unsupported data type");
    }
    return i;
}

xmlChar* insertNodeIndices(const xmlChar* xpathExpr, int* nodeIndices)
{
    xmlChar* indexedXpathExpr = xmlStrdup(xpathExpr);

    if (nodeIndices == NULL) {
        return indexedXpathExpr;
    }

    const xmlChar* p;
    size_t n = 0;

    while ((p = xmlStrchr(indexedXpathExpr, '#')) != NULL) {
        int len = snprintf(NULL, 0, "%d", nodeIndices[n]);
        xmlChar num_str[len + 1];
        xmlStrPrintf(num_str, len + 1, (XML_FMT_TYPE)"%d", nodeIndices[n]);
        ++n;

        xmlChar* pre = xmlStrndup(indexedXpathExpr, p - indexedXpathExpr);

        len = xmlStrlen(pre) + xmlStrlen(num_str) + xmlStrlen(p + 1) + 1;
        xmlChar* temp = malloc((len + 1) * sizeof(xmlChar));
        xmlStrPrintf(temp, len, (XML_FMT_TYPE)"%s%s%s", pre, num_str, p + 1);
        free(indexedXpathExpr);
        indexedXpathExpr = temp;

        free(pre);
    }

    return indexedXpathExpr;
}
