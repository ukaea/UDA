#include "ts_xml.h"

#include <assert.h>

#include <clientserver/initStructs.h>
#include <clientserver/udaTypes.h>
#include <clientserver/errorLog.h>
#include <logging/logging.h>
#include <clientserver/stringUtils.h>

static int convertToInt(char* value);
static char** getContent(xmlNode* node, int dim);
static xmlChar* insertNodeIndices(const xmlChar* xpathExpr, int* nodeIndices);

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
int execute_xpath_expression(const char* filename, const xmlChar* xpathExpr, DATA_BLOCK* data_block, int* nodeIndices)
{
    xmlDocPtr doc;
    xmlXPathContextPtr xpathCtx;
    xmlXPathObjectPtr xpathObj;

    initDataBlock(data_block);

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

    // Defaults
    int dim = 1; //we always return an array with dimension >= 1
    char* type = "xs:integer";

    xpathExpr = insertNodeIndices(xpathExpr, nodeIndices);

    /* First, we get the type of the element which is requested */

    //Creating the Xpath request for the type which does not exist necessarly (it depends on the XML element which is requested)
    char* typeStr = "/@data_type";
    size_t len = 1 + xmlStrlen(xpathExpr) + strlen(typeStr);
    xmlChar* typeXpathExpr = (xmlChar*)malloc(len * sizeof(xmlChar));
    xmlStrPrintf(typeXpathExpr, (int)len, (XML_FMT_TYPE)"%s%s", xpathExpr, typeStr);

    /* Evaluate xpath expression for the type */
    xpathObj = xmlXPathEvalExpression(typeXpathExpr, xpathCtx);
    if (xpathObj != NULL) {
        xmlNodeSetPtr nodes = xpathObj->nodesetval;
        int size = (nodes) ? nodes->nodeNr : 0;

        xmlNodePtr cur;
        //int err = 0;

        if (size != 0) {
            cur = nodes->nodeTab[0];
            cur = cur->children;
            type = (char*)cur->content;
        }
    }

    /* Second, we get the size of the element which is requested */
    //Creating the Xpath request for the array dimension (the request returns nothing if the XML element is not an array)
    typeStr = "/@dim";

    len = 1 + xmlStrlen(xpathExpr) + strlen(typeStr);
    typeXpathExpr = (xmlChar*)malloc(len * sizeof(xmlChar));
    xmlStrPrintf(typeXpathExpr, (int)len, (XML_FMT_TYPE)"%s%s", xpathExpr, typeStr);

    /* Evaluate xpath expression for the type */
    xpathObj = xmlXPathEvalExpression(typeXpathExpr, xpathCtx);
    if (xpathObj != NULL) {
        xmlNodeSetPtr nodes = xpathObj->nodesetval;
        int size = (nodes) ? nodes->nodeNr : 0;    //if size==0, there is no 'dim' attribute

        xmlNodePtr cur;

        if (size != 0) {
            cur = nodes->nodeTab[0];    //Getting the dim of the data to request
            cur = cur->children;
            dim = atoi((char*)cur->content);
        }
    }

    /* Evaluate xpath expression for requesting the data  */
    xpathObj = xmlXPathEvalExpression(xpathExpr, xpathCtx);

    if (xpathObj == NULL) {
        fprintf(stderr, "Error: unable to evaluate xpath expression \"%s\"\n", xpathExpr);
        xmlXPathFreeContext(xpathCtx);
        xmlFreeDoc(doc);
        return -1;
    }

    xmlNodeSetPtr nodes = xpathObj->nodesetval;

    int size = (nodes) ? nodes->nodeNr : 0;
    xmlNodePtr cur;
    int err = 0;

    if (size == 0) {
        fprintf(stderr, "ts_xml plugin : error in XPath request  \n");
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

    data_block->rank = 1;
    data_block->dims = (DIMS*)malloc(data_block->rank * sizeof(DIMS));

    int i;
    for (i = 0; i < data_block->rank; i++) {
        initDimBlock(&data_block->dims[i]);
    }

    int data_type = convertToInt(type);

    if (data_type == UDA_TYPE_DOUBLE) {
        data_block->data_type = UDA_TYPE_DOUBLE;
        data_block->data = malloc(dim * sizeof(double));
        char** data = getContent(cur, dim);
        for (i = 0; i < dim; i++) {
            ((double*)data_block->data)[i] = atof(data[i]);
        }
    } else if (data_type == UDA_TYPE_FLOAT) {
        data_block->data_type = UDA_TYPE_FLOAT;
        data_block->data = malloc(dim * sizeof(float));
        char** data = getContent(cur, dim);
        for (i = 0; i < dim; i++) {
            ((float*)data_block->data)[i] = (float)atof(data[i]);
        }
    } else if (data_type == UDA_TYPE_LONG) {
        data_block->data_type = UDA_TYPE_LONG;
        data_block->data = malloc(dim * sizeof(long));
        char** data = getContent(cur, dim);
        for (i = 0; i < dim; i++) {
            ((long*)data_block->data)[i] = atol(data[i]);
        }
    } else if (data_type == UDA_TYPE_INT) {
        data_block->data_type = UDA_TYPE_INT;
        data_block->data = malloc(dim * sizeof(int));
        char** data = getContent(cur, dim);
        for (i = 0; i < dim; i++) {
            ((int*)data_block->data)[i] = atoi(data[i]);
        }
    } else if (data_type == UDA_TYPE_SHORT) {
        data_block->data_type = UDA_TYPE_SHORT;
        data_block->data = malloc(dim * sizeof(short));
        char** data = getContent(cur, dim);
        for (i = 0; i < dim; i++) {
            ((short*)data_block->data)[i] = (short)atoi(data[i]);
        }
    } else if (data_type == UDA_TYPE_STRING) {
        data_block->data_type = UDA_TYPE_STRING;
        data_block->data = strdup((char*)cur->children->content);

    } else {
        err = 999;
        addIdamError(CODEERRORTYPE, "tore_supra : Unsupported data type", err, "");
    }

    data_block->dims[0].data_type = UDA_TYPE_UNSIGNED_INT;
    data_block->dims[0].dim_n = dim;
    data_block->dims[0].compressed = 1;
    data_block->dims[0].dim0 = 0.0;
    data_block->dims[0].diff = 1.0;
    data_block->dims[0].method = 0;

    data_block->data_n = data_block->dims[0].dim_n;

    strcpy(data_block->data_desc, "");
    strcpy(data_block->data_label, "");
    strcpy(data_block->data_units, "");

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
            if (c == '\0') {
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
    int i = UDA_TYPE_UNKNOWN;
    int err = 0;

    if (STR_EQUALS(value, "vecstring_type") || STR_EQUALS(value, "xs:string") || STR_EQUALS(value, "STR_0D")) {
        i = UDA_TYPE_STRING;
    } else if (STR_EQUALS(value, "vecflt_type") || STR_EQUALS(value, "xs:float") || STR_EQUALS(value, "FLT_0D")) {
        i = UDA_TYPE_FLOAT;
    } else if (STR_EQUALS(value, "vecint_type") || STR_EQUALS(value, "xs:integer") || STR_EQUALS(value, "INT_0D")) {
        i = UDA_TYPE_INT;
    } else {
        err = 999;
        addIdamError(CODEERRORTYPE, "tore_supra convertToInt() : Unsupported data type", err, "");
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
