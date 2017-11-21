#include <assert.h>
#include <string.h>

#include <clientserver/errorLog.h>
#include <logging/logging.h>
#include <clientserver/stringUtils.h>
#include <clientserver/initStructs.h>
#include <clientserver/udaTypes.h>
#include <plugins/udaPlugin.h>

#include "getXPathValue.h"

char* getXPathValue(const char* xmlfile, const char* path, unsigned short cleanup, int* err)
{
    static xmlDocPtr doc = NULL;
    static xmlXPathContextPtr xpathCtx = NULL;
    xmlXPathObjectPtr xpathObj = NULL;

    if (!cleanup) assert(path);

    static unsigned short init = 0;

    *err = 0;

    // Cleanup
    if (cleanup) {
        if(xpathCtx != NULL) xmlXPathFreeContext(xpathCtx);
        if(doc != NULL) xmlFreeDoc(doc);
        init = 0;
	xpathCtx = NULL;
	doc = NULL;
	return NULL;
    } 	 

    if (!init) {
        /*
         * Load XML document
         */
	 
	if (xmlfile == NULL || xmlfile[0] == '\0') {
            *err = 999;
	    addIdamError(CODEERRORTYPE, __func__, *err, "No XML Document path provided!");
            return NULL;
        }

	 
	UDA_LOG(UDA_LOG_DEBUG, "Parsing XML File: %s\n", xmlfile);

        assert(xmlfile);

        doc = xmlParseFile(xmlfile);
        if (doc == NULL) {
            *err = 999;
	    addIdamError(CODEERRORTYPE, __func__, *err, "Unable to parse the Machine Description XML file!");
            return NULL;
        }

        /*
         * Create xpath evaluation context
         */
        xpathCtx = xmlXPathNewContext(doc);
        if (xpathCtx == NULL) {
            *err = 999;
            xmlFreeDoc(doc);
	    addIdamError(CODEERRORTYPE, __func__, *err, "Unable to create new XPath context!");
            return NULL;
        }

        init = 1;

        UDA_LOG(UDA_LOG_DEBUG, "XML File Parsed\n");

        return NULL;
    }

    // Creating the Xpath request
    UDA_LOG(UDA_LOG_DEBUG, "Creating the Xpath request: %s\n", path);

    xmlChar* xPathExpr = xmlCharStrdup(path);

    /*
     * Evaluate xpath expression for the type
     */
    xpathObj = xmlXPathEvalExpression(xPathExpr, xpathCtx);

    if (xPathExpr) free(xPathExpr);

    if (xpathObj == NULL) {
        *err = 999;
        UDA_LOG(UDA_LOG_DEBUG, "unable to evaluate xpath expression\n");
        addIdamError(CODEERRORTYPE, __func__, *err, "Unable to evaluate xpath expression!");
	return NULL;
    }

    xmlNodeSetPtr nodes = xpathObj->nodesetval;
    int size = (nodes) ? nodes->nodeNr : 0;
    char* value = NULL;

    xmlNodePtr cur;

    if (size != 0) {
        cur = nodes->nodeTab[0];
        cur = cur->children;
        value = strdup((char*)cur->content);
    } else {
        *err = 998;
        UDA_LOG(UDA_LOG_DEBUG, "size equals 0\n");
        addIdamError(CODEERRORTYPE, __func__, *err, "xmlNodeSetPtr size equals 0!");
	return NULL;
    }

    /*
     * Cleanup
     */
    xmlXPathFreeObject(xpathObj);

    return value;        // Consumer frees this heap
}

float* xPathFloatArray(const char* value, int* n)
{
    float* list = NULL;
    *n = 0;
    char* delim = " ,";
    char* item;
    int nco = 0;

    if (!value || value[0] == '\0') return NULL;

    char* work = strdup(value);

    // clean value string
    if(work[0] == '[' || work[0] == '{' || work[0] == '(') work[0] = ' ';
    int lstr = strlen(work) - 1;
    if(lstr >= 0 && (work[lstr] == ']' || work[lstr] == '}' || work[lstr] == ')')) work[lstr] = ' ';
    TrimString(work);

    item = strtok((char*)work, delim);
    if (item != NULL) {
        nco++;
        list = (float*)realloc((void*)list, nco * sizeof(float));
        list[nco - 1] = (float)atof(item);
        UDA_LOG(UDA_LOG_DEBUG, "[%d] %s %f\n", nco, item, list[nco - 1]);
        while ((item = strtok(NULL, delim)) != NULL && nco <= XPATHARRAYMAXLOOP) {
            nco++;
            list = (float*)realloc((void*)list, nco * sizeof(float));
            list[nco - 1] = (float)atof(item);
            UDA_LOG(UDA_LOG_DEBUG, "[%d] %s %f\n", nco, item, list[nco - 1]);
        }
    }
    *n = nco;
    free(work);
    return list;
}

double* xPathDoubleArray(const char* value, int* n)
{
    double* list = NULL;
    *n = 0;
    char* delim = " ,";
    char* item;
    int nco = 0;

    if (!value || value[0] == '\0') return NULL;

    char* work = strdup(value);
    
    // clean value string
    if(work[0] == '[' || work[0] == '{' || work[0] == '(') work[0] = ' ';
    int lstr = strlen(work) - 1;
    if(lstr >= 0 && (work[lstr] == ']' || work[lstr] == '}' || work[lstr] == ')')) work[lstr] = ' ';
    TrimString(work);

    item = strtok((char*)work, delim);
    if (item != NULL) {
        nco++;
        list = (double*)realloc((void*)list, nco * sizeof(double));
        list[nco - 1] = (double)atof(item);
        UDA_LOG(UDA_LOG_DEBUG, "[%d] %s %f\n", nco, item, list[nco - 1]);
        while ((item = strtok(NULL, delim)) != NULL && nco <= XPATHARRAYMAXLOOP) {
            nco++;
            list = (double*)realloc((void*)list, nco * sizeof(double));
            list[nco - 1] = (double)atof(item);
            UDA_LOG(UDA_LOG_DEBUG, "[%d] %s %f\n", nco, item, list[nco - 1]);
        }
    }
    *n = nco;
    free(work);
    return list;
}

int* xPathIntArray(const char* value, int* n)
{
    int* list = NULL;
    *n = 0;
    char* delim = " ,";
    char* item;
    int nco = 0;

    if (!value || value[0] == '\0') return NULL;

    char* work = strdup(value);

    // clean value string
    if(work[0] == '[' || work[0] == '{' || work[0] == '(') work[0] = ' ';
    int lstr = strlen(work) - 1;
    if(lstr >= 0 && (work[lstr] == ']' || work[lstr] == '}' || work[lstr] == ')')) work[lstr] = ' ';
    TrimString(work);

    item = strtok((char*)work, delim);
    if (item != NULL) {
        nco++;
        list = (int*)realloc((void*)list, nco * sizeof(int));
        list[nco - 1] = (int)atoi(item);
        UDA_LOG(UDA_LOG_DEBUG, "[%d] %s %d\n", nco, item, list[nco - 1]);
        while ((item = strtok(NULL, delim)) != NULL && nco <= XPATHARRAYMAXLOOP) {
            nco++;
            list = (int*)realloc((void*)list, nco * sizeof(int));
            list[nco - 1] = (int)atoi(item);
            UDA_LOG(UDA_LOG_DEBUG, "[%d] %s %d\n", nco, item, list[nco - 1]);
        }
    }
    *n = nco;
    free(work);
    return list;
}

