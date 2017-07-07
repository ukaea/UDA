#include <assert.h>
#include <string.h>

#include <clientserver/errorLog.h>
#include <logging/logging.h>
#include <clientserver/stringUtils.h>
#include <clientserver/initStructs.h>
#include <clientserver/udaTypes.h>
#include <plugins/udaPlugin.h>

#include "getXPathValue.h"


char* getXPathValue(const char* xmlfile, const char* path, unsigned short cleanup, int *err)
{
    static xmlDocPtr doc;
    static xmlXPathContextPtr xpathCtx;
    xmlXPathObjectPtr xpathObj;

    if(!cleanup) assert(path);
    
    static unsigned short init = 0;
    
    *err = 0;

    
    if(!init){
    /*
     * Load XML document
     */
     
       assert(xmlfile);

       doc = xmlParseFile(xmlfile);
       if (doc == NULL) {
           *err = 999;
	   IDAM_LOG(UDA_LOG_DEBUG, "getXPathValue Error: unable to parse the Machine Description XML file\n");
           return NULL;
       }

    /*
     * Create xpath evaluation context
     */
       xpathCtx = xmlXPathNewContext(doc);
       if (xpathCtx == NULL) {
           *err = 999;
	   IDAM_LOG(UDA_LOG_DEBUG, "getXPathValue Error: unable to create new XPath context\n");
           xmlFreeDoc(doc);
           return NULL;
       }

       init = 1;
       
       IDAM_LOG(UDA_LOG_DEBUG, "getXPathValue: XML File Parsed\n");
       
       return NULL;
    }
    
    // Creating the Xpath request
    IDAM_LOGF(UDA_LOG_DEBUG, "getXPathValue: Creating the Xpath request: %s\n", path);
    
    xmlChar *xPathExpr = xmlCharStrdup(path);		// /Top/pfCoils/pfCoil[@id='9']/@name

    /*
     * Evaluate xpath expression for the type
     */
    xpathObj = xmlXPathEvalExpression(xPathExpr, xpathCtx);
    
    if(xPathExpr) free(xPathExpr);
    
    if (xpathObj == NULL) {
        *err = 999;
	IDAM_LOG(UDA_LOG_DEBUG, "getXPathValue Error: unable to evaluate xpath expression\n");
        return NULL;
    }

    xmlNodeSetPtr nodes = xpathObj->nodesetval;
    int size = (nodes) ? nodes->nodeNr : 0;
    char* value = NULL;

    xmlNodePtr cur;

    if (size != 0) {
        IDAM_LOG(UDA_LOG_DEBUG, "size different of 0\n");
        cur = nodes->nodeTab[0];
        cur = cur->children;
        value = strdup((char*)cur->content);
    } else {
        *err = 998;
	IDAM_LOG(UDA_LOG_DEBUG, "getXPathValue Error : size equals 0\n");
        return NULL;
    }
    
    /*
     * Cleanup
     */
    xmlXPathFreeObject(xpathObj);

    if(cleanup){    
       xmlXPathFreeContext(xpathCtx);
       xmlFreeDoc(doc);
       init = 0;
    }   

    return value;		// Consumer frees this heap
}

float* xPathFloatArray(const char *value, int* n)
{
    float* list = NULL;
    *n = 0;
    char* delim = " ,";
    char* item;
    int nco = 0;
    
    if(!value || value[0] == '\0') return NULL;
    
    char *work = strdup(value);

    item = strtok((char*)work, delim);
    if (item != NULL) {
       nco++;
       list = (float*)realloc((void*)list, nco * sizeof(float));
       list[nco - 1] = (float)atof(item);
       IDAM_LOGF(UDA_LOG_DEBUG, "[%d] %s %f\n", nco, item, list[nco - 1]);       
       while ((item = strtok(NULL, delim)) != NULL && nco <= XPATHARRAYMAXLOOP) {
          nco++;
          list = (float*)realloc((void*)list, nco * sizeof(float));
          list[nco - 1] = (float)atof(item);
          IDAM_LOGF(UDA_LOG_DEBUG, "[%d] %s %f\n", nco, item, list[nco - 1]);
       }
    }
    *n = nco;
    free(work);
    return list;
}

double* xPathDoubleArray(const char *value, int* n)
{
    double* list = NULL;
    *n = 0;
    char* delim = " ,";
    char* item;
    int nco = 0;
    
    if(!value || value[0] == '\0') return NULL;
    
    char *work = strdup(value);

    item = strtok((char*)work, delim);
    if (item != NULL) {
       nco++;
       list = (double*)realloc((void*)list, nco * sizeof(double));
       list[nco - 1] = (double)atof(item);
       IDAM_LOGF(UDA_LOG_DEBUG, "[%d] %s %f\n", nco, item, list[nco - 1]);       
       while ((item = strtok(NULL, delim)) != NULL && nco <= XPATHARRAYMAXLOOP) {
          nco++;
          list = (double*)realloc((void*)list, nco * sizeof(double));
          list[nco - 1] = (double)atof(item);
          IDAM_LOGF(UDA_LOG_DEBUG, "[%d] %s %f\n", nco, item, list[nco - 1]);
       }
    }
    *n = nco;
    free(work);
    return list;
}

int* xPathIntArray(const char *value, int* n)
{
    int* list = NULL;
    *n = 0;
    char* delim = " ,";
    char* item;
    int nco = 0;
    
    if(!value || value[0] == '\0') return NULL;
    
    char *work = strdup(value);

    item = strtok((char*)work, delim);
    if (item != NULL) {
       nco++;
       list = (int*)realloc((void*)list, nco * sizeof(int));
       list[nco - 1] = (int)atoi(item);
       IDAM_LOGF(UDA_LOG_DEBUG, "[%d] %s %d\n", nco, item, list[nco - 1]);       
       while ((item = strtok(NULL, delim)) != NULL && nco <= XPATHARRAYMAXLOOP) {
          nco++;
          list = (int*)realloc((void*)list, nco * sizeof(int));
          list[nco - 1] = (int)atoi(item);
          IDAM_LOGF(UDA_LOG_DEBUG, "[%d] %s %d\n", nco, item, list[nco - 1]);
       }
    }
    *n = nco;
    free(work);
    return list;
}

