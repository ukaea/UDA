/*
* IDAM XML Parser for EFIT Hierarchical Data Structures 
* 
* Input Arguments:	char *xml 
* 
* Returns:		0 if parse was successful 
* 
*			IDAM_EFIT - Data Structure
*-------------------------------------------------------------------------*/
#include "parseHData.h"

#include <clientserver/stringUtils.h>
#include <logging/logging.h>
#include <clientserver/initXMLStructs.h>
#include <clientserver/errorLog.h>
#include <clientserver/xmlStructs.h>

void parseFloat(xmlDocPtr doc, xmlNodePtr cur, const char* target, float* value)
{
    xmlChar* key;
    *value = 0.0;

    cur = cur->xmlChildrenNode;
    while (cur != NULL) {
        if ((!xmlStrcmp(cur->name, (const xmlChar*)target))) {
            key = xmlNodeListGetString(doc, cur->xmlChildrenNode, 1);
            if (strlen((char*)key) > 0) {
                *value = (float)atof((char*)key);
            }
            IDAM_LOGF(LOG_DEBUG, "%s  %s\n", target, (char*)key);
            xmlFree(key);
            break;
        }
        cur = cur->next;
    }
    return;
}

void parseInt(xmlDocPtr doc, xmlNodePtr cur, const char* target, int* value)
{
    *value = 0;

    cur = cur->xmlChildrenNode;
    while (cur != NULL) {
        if ((!xmlStrcmp(cur->name, (const xmlChar*)target))) {
            xmlChar* key = xmlNodeListGetString(doc, cur->xmlChildrenNode, 1);
            if (strlen((char*)key) > 0) *value = atoi((char*)key);
            IDAM_LOGF(LOG_DEBUG, "%s  %s\n", target, (char*)key);
            xmlFree(key);
            break;
        }
        cur = cur->next;
    }
    return;
}

int* parseIntArray(xmlDocPtr doc, xmlNodePtr cur, const char* target, int* n)
{
    int* value = NULL;
    *n = 0;
    const char* delim = " ";
    char* item;
    int nco = 0;

    cur = cur->xmlChildrenNode;
    while (cur != NULL) {
        if ((!xmlStrcmp(cur->name, (const xmlChar*)target))) {
            xmlChar* key = xmlNodeListGetString(doc, cur->xmlChildrenNode, 1);
            convertNonPrintable((char*)key);
            if (strlen((char*)key) > 0) {
                IDAM_LOGF(LOG_DEBUG, "%s %s \n", target, (char*)key);
                item = strtok((char*)key, delim);
                if (item != NULL) {
                    nco++;
                    IDAM_LOGF(LOG_DEBUG, " [%d] %s \n", nco, item);
                    value = (int*)realloc((void*)value, nco * sizeof(int));
                    value[nco - 1] = atoi(item);
                    IDAM_LOGF(LOG_DEBUG, " [%d] %s %d\n", nco, item, value[nco - 1]);
                    while ((item = strtok(NULL, delim)) != NULL && nco <= XMLMAXLOOP) {
                        nco++;
                        value = (int*)realloc((void*)value, nco * sizeof(int));
                        value[nco - 1] = atoi(item);
                        IDAM_LOGF(LOG_DEBUG, " [%d] %s %d\n", nco, item, value[nco - 1]);
                    }
                }
            }
            *n = nco;
            xmlFree(key);
            break;
        }
        cur = cur->next;
    }
    return value;
}

float* parseFloatAngleArray(xmlDocPtr doc, xmlNodePtr cur, const char* target, int* n)
{
    xmlChar* key, * att;
    float* value = NULL;
    *n = 0;
    char* delim = " ";
    char* item;
    int nco = 0;

    cur = cur->xmlChildrenNode;
    while (cur != NULL) {
        if ((!xmlStrcmp(cur->name, (const xmlChar*)target))) {
            key = xmlNodeListGetString(doc, cur->xmlChildrenNode, 1);
            convertNonPrintable((char*)key);
            if (strlen((char*)key) > 0) {
                IDAM_LOGF(LOG_DEBUG, "%s %s \n", target, (char*)key);
                item = strtok((char*)key, delim);
                if (item != NULL) {
                    nco++;
                    IDAM_LOGF(LOG_DEBUG, " [%d] %s \n", nco, item);
                    value = (float*)realloc((void*)value, nco * sizeof(float));
                    value[nco - 1] = (float)atof(item);
                    IDAM_LOGF(LOG_DEBUG, " [%d] %s %f\n", nco, item, value[nco - 1]);
                    while ((item = strtok(NULL, delim)) != NULL && nco <= XMLMAXLOOP) {
                        nco++;
                        value = (float*)realloc((void*)value, nco * sizeof(float));
                        value[nco - 1] = (float)atof(item);
                        IDAM_LOGF(LOG_DEBUG, " [%d] %s %f\n", nco, item, value[nco - 1]);
                    }
                }
            }
            *n = nco;
            xmlFree(key);

            float factor = 3.1415927f / 180.0f;    // Default is Degrees

            if ((att = xmlGetProp(cur, (xmlChar*)"units")) != NULL) {
                if (strlen((char*)att) > 0) {
                    if (STR_EQUALS((char*)att, "pi")) factor = 3.1415927f;
                    if (STR_EQUALS((char*)att, "radians")) factor = 1.0;
                    xmlFree(att);
                }
            }

            if (value != NULL) {
                int i;
                for (i = 0; i < nco; i++) {
                    value[i] = value[i] * factor;
                }
            }

            break;
        }
        cur = cur->next;
    }
    return value;
}

void parseFloatAngle(xmlDocPtr doc, xmlNodePtr cur, const char* target, float* value)
{
    xmlChar* key, * att;
    *value = 0.0;

    cur = cur->xmlChildrenNode;
    while (cur != NULL) {
        if ((!xmlStrcmp(cur->name, (const xmlChar*)target))) {
            key = xmlNodeListGetString(doc, cur->xmlChildrenNode, 1);
            if (strlen((char*)key) > 0) {
                *value = (float)atof((char*)key);
            }
            IDAM_LOGF(LOG_DEBUG, "%s  %s\n", target, (char*)key);
            xmlFree(key);

            if ((att = xmlGetProp(cur, (xmlChar*)"units")) != NULL) {
                if (strlen((char*)att) > 0) {
                    if (STR_EQUALS((char*)att, "pi")) *value = *value * 3.1415927f;
                    if (STR_EQUALS((char*)att, "degrees")) *value = *value * 3.1415927f / 180.0f;
                    xmlFree(att);
                }
            }

            float factor = 3.1415927f / 180.0f;    // Default is Degrees

            if ((att = xmlGetProp(cur, (xmlChar*)"units")) != NULL) {
                if (strlen((char*)att) > 0) {
                    if (STR_EQUALS((char*)att, "pi")) factor = 3.1415927f;
                    if (STR_EQUALS((char*)att, "radians")) factor = 1.0;
                    xmlFree(att);
                }
            }

            *value = *value * factor;

            break;
        }
        cur = cur->next;
    }
    return;
}

void parseInstance(xmlNodePtr cur, INSTANCE* str)
{
    xmlChar* att;

    if ((att = xmlGetProp(cur, (xmlChar*)"archive")) != NULL) {
        if (xmlStrlen(att) > 0) {
            strcpy(str->archive, (char*)att);
        }
        IDAM_LOGF(LOG_DEBUG, "Archive: %s\n", str->archive);
        xmlFree(att);
    }
    if ((att = xmlGetProp(cur, (xmlChar*)"file")) != NULL) {
        if (xmlStrlen(att) > 0) {
            strcpy(str->file, (char*)att);
        }
        IDAM_LOGF(LOG_DEBUG, "File: %s\n", str->file);
        xmlFree(att);
    }
    if ((att = xmlGetProp(cur, (xmlChar*)"signal")) != NULL) {
        if (xmlStrlen(att) > 0) {
            strcpy(str->signal, (char*)att);
            TrimString(str->signal);
        }
        IDAM_LOGF(LOG_DEBUG, "Signal: %s\n", str->signal);
        xmlFree(att);
    }
    if ((att = xmlGetProp(cur, (xmlChar*)"owner")) != NULL) {
        if (xmlStrlen(att) > 0) {
            strcpy(str->owner, (char*)att);
        }
        IDAM_LOGF(LOG_DEBUG, "Owner: %s\n", str->owner);
        xmlFree(att);
    }
    if ((att = xmlGetProp(cur, (xmlChar*)"format")) != NULL) {
        if (xmlStrlen(att) > 0) {
            strcpy(str->format, (char*)att);
        }
        IDAM_LOGF(LOG_DEBUG, "Format: %s\n", str->format);
        xmlFree(att);
    }

    if ((att = xmlGetProp(cur, (xmlChar*)"seq")) != NULL) {
        if (xmlStrlen(att) > 0) {
            str->seq = atoi((char*)att);
        }
        IDAM_LOGF(LOG_DEBUG, "Seq: %d\n", str->seq);
        xmlFree(att);
    }
    if ((att = xmlGetProp(cur, (xmlChar*)"status")) != NULL) {
        if (xmlStrlen(att) > 0) {
            str->status = atoi((char*)att);
        }
        IDAM_LOGF(LOG_DEBUG, "Status: %d\n", str->status);
        xmlFree(att);
    }
    if ((att = xmlGetProp(cur, (xmlChar*)"factor")) != NULL) {
        if (xmlStrlen(att) > 0) {
            str->factor = (float)atof((char*)att);
        }
        IDAM_LOGF(LOG_DEBUG, "Factor: %f\n", str->factor);
        xmlFree(att);
    }

    return;
}

MAGPROBE* parseMagProbe(xmlDocPtr doc, xmlNodePtr cur, MAGPROBE* str, int* np)
{

    int n = 0;
    xmlChar* att;    // General Input of tag attribute values

    *np = 0;
    cur = cur->xmlChildrenNode;
    while (cur != NULL) {
        IDAM_LOGF(LOG_DEBUG, "%s\n", (char*)cur->name);
        if ((!xmlStrcmp(cur->name, (const xmlChar*)"instance"))) {
            n++;
            str = (MAGPROBE*)realloc((void*)str, n * sizeof(MAGPROBE));

            IDAM_LOGF(LOG_DEBUG, "%d: %p\n", n, str);
            initMagProbe(&str[n - 1]);

// Instance Attributes 

            if ((att = xmlGetProp(cur, (xmlChar*)"id")) != NULL) {
                if (xmlStrlen(att) > 0) strcpy(str[n - 1].id, (char*)att);
                IDAM_LOGF(LOG_DEBUG, "Mag Probe ID: %s\n", str[n - 1].id);
                xmlFree(att);
            }

            parseInstance(cur, &str[n - 1].instance);

// Child Tags	  

            parseFloat(doc, cur, "r", &str[n - 1].r);
            parseFloat(doc, cur, "z", &str[n - 1].z);

            parseFloatAngle(doc, cur, "angle", &str[n - 1].angle);

            parseFloat(doc, cur, "abs_error", &str[n - 1].aerr);
            parseFloat(doc, cur, "rel_error", &str[n - 1].rerr);

            printMagProbe(str[n - 1]);
        }
        cur = cur->next;
    }
    *np = n;    // Number of Tags Found
    return str;
}

float* parseFloatArray(xmlDocPtr doc, xmlNodePtr cur, const char* target, int* n)
{
    xmlChar* key = NULL;
    float* value = NULL;
    *n = 0;
    char* delim = " ";
    char* item;
    int nco = 0;

    cur = cur->xmlChildrenNode;
    while (cur != NULL) {
        if ((!xmlStrcmp(cur->name, (const xmlChar*) target))) {
            key = xmlNodeListGetString(doc, cur->xmlChildrenNode, 1);
            convertNonPrintable((char*) key);
            if (strlen((char*) key) > 0) {
                int lkey = (int) strlen((char*) key);
                IDAM_LOGF(LOG_DEBUG, "[%d] %s %s \n", lkey, target, key);
                item = strtok((char*) key, delim);
                if (item != NULL) {
                    nco++;
                    IDAM_LOGF(LOG_DEBUG, "[%d] %s \n", nco, item);
                    value = (float*) realloc((void*) value, nco * sizeof(float));
                    value[nco - 1] = (float)atof(item);
                    IDAM_LOGF(LOG_DEBUG, "[%d] %s %f\n", nco, item, value[nco - 1]);
                    while ((item = strtok(NULL, delim)) != NULL && nco <= XMLMAXLOOP) {
                        nco++;
                        value = (float*) realloc((void*) value, nco * sizeof(float));
                        value[nco - 1] = (float)atof(item);
                        IDAM_LOGF(LOG_DEBUG, "[%d] %s %f\n", nco, item, value[nco - 1]);
                    }
                }
            }
            *n = nco;
            xmlFree(key);
            break;
        }
        cur = cur->next;
    }
    return value;
}

FLUXLOOP* parseFluxLoop(xmlDocPtr doc, xmlNodePtr cur, FLUXLOOP* str, int* np)
{

    int n = 0;
    int nco;
    xmlChar* att;    // General Input of tag attribute values

    *np = 0;
    cur = cur->xmlChildrenNode;
    while (cur != NULL) {
        IDAM_LOGF(LOG_DEBUG, "%s\n", (char*)cur->name);
        if ((!xmlStrcmp(cur->name, (const xmlChar*)"instance"))) {
            n++;
            str = (FLUXLOOP*)realloc((void*)str, n * sizeof(FLUXLOOP));

            IDAM_LOGF(LOG_DEBUG, "%d: %p\n", n, str);
            initFluxLoop(&str[n - 1]);

// Attributes 

            if ((att = xmlGetProp(cur, (xmlChar*)"id")) != NULL) {
                if (xmlStrlen(att) > 0) strcpy(str[n - 1].id, (char*)att);
                IDAM_LOGF(LOG_DEBUG, "Flux Loop ID: %s\n", str[n - 1].id);
                xmlFree(att);
            }

            parseInstance(cur, &str[n - 1].instance);

// Child Tags	  
            str[n - 1].r = parseFloatArray(doc, cur, "r", &str[n - 1].nco);
            str[n - 1].z = parseFloatArray(doc, cur, "z", &nco);
            str[n - 1].dphi = parseFloatAngleArray(doc, cur, "dphi", &nco);

            parseFloat(doc, cur, "abs_error", &str[n - 1].aerr);
            parseFloat(doc, cur, "rel_error", &str[n - 1].rerr);

            printFluxLoop(str[n - 1]);
        }
        cur = cur->next;
    }
    *np = n;    // Number of Tags Found
    return str;
}

PFCOILS* parsePfCoils(xmlDocPtr doc, xmlNodePtr cur, PFCOILS* str, int* np)
{

    int i, n = 0;
    int nco;
    int* nrnz;
    xmlChar* att;    // General Input of tag attribute values

    *np = 0;
    cur = cur->xmlChildrenNode;
    while (cur != NULL) {
        IDAM_LOGF(LOG_DEBUG, "%s\n", (char*)cur->name);
        if ((!xmlStrcmp(cur->name, (const xmlChar*)"instance"))) {
            n++;
            str = (PFCOILS*)realloc((void*)str, n * sizeof(PFCOILS));

            IDAM_LOGF(LOG_DEBUG, "%d: %p\n", n, str);
            initPfCoils(&str[n - 1]);

// Attributes

            if ((att = xmlGetProp(cur, (xmlChar*)"id")) != NULL) {
                if (xmlStrlen(att) > 0) strcpy(str[n - 1].id, (char*)att);
                IDAM_LOGF(LOG_DEBUG, "PF Coil ID: %s\n", str[n - 1].id);
                xmlFree(att);
            }

            parseInstance(cur, &str[n - 1].instance);

// Child Tags

            str[n - 1].r = parseFloatArray(doc, cur, "r", &str[n - 1].nco);
            str[n - 1].z = parseFloatArray(doc, cur, "z", &nco);
            str[n - 1].dr = parseFloatArray(doc, cur, "dr", &nco);
            str[n - 1].dz = parseFloatArray(doc, cur, "dz", &nco);

            parseInt(doc, cur, "turnsperelement", &str[n - 1].turns);
            parseFloat(doc, cur, "turnsperelement", &str[n - 1].fturns);
            parseFloat(doc, cur, "abs_error", &str[n - 1].aerr);
            parseFloat(doc, cur, "rel_error", &str[n - 1].rerr);

            nrnz = parseIntArray(doc, cur, "modelnrnz", &nco);
            if (nco > 0 && nco <= 2 && nrnz != NULL) {
                for (i = 0; i < nco; i++) str[n - 1].modelnrnz[i] = nrnz[i];
                free(nrnz);
            }

            printPfCoils(str[n - 1]);
        }
        cur = cur->next;
    }
    *np = n;    // Number of Tags Found
    return str;
}

PFPASSIVE* parsePfPassive(xmlDocPtr doc, xmlNodePtr cur, PFPASSIVE* str, int* np)
{

    int i, n = 0;
    int nco;
    int* nrnz;
    xmlChar* att;    // General Input of tag attribute values

    *np = 0;
    cur = cur->xmlChildrenNode;
    while (cur != NULL) {
        IDAM_LOGF(LOG_DEBUG, "%s\n", (char*)cur->name);
        if ((!xmlStrcmp(cur->name, (const xmlChar*)"instance"))) {
            n++;
            str = (PFPASSIVE*)realloc((void*)str, n * sizeof(PFPASSIVE));

            IDAM_LOGF(LOG_DEBUG, "%d: %p\n", n, str);
            initPfPassive(&str[n - 1]);

// Attributes 

            if ((att = xmlGetProp(cur, (xmlChar*)"id")) != NULL) {
                if (xmlStrlen(att) > 0) strcpy(str[n - 1].id, (char*)att);
                IDAM_LOGF(LOG_DEBUG, "Pf Passive ID: %s\n", str[n - 1].id);
                xmlFree(att);
            }

            parseInstance(cur, &str[n - 1].instance);

// Child Tags	  

            str[n - 1].r = parseFloatArray(doc, cur, "r", &str[n - 1].nco);
            str[n - 1].z = parseFloatArray(doc, cur, "z", &nco);
            str[n - 1].dr = parseFloatArray(doc, cur, "dr", &nco);
            str[n - 1].dz = parseFloatArray(doc, cur, "dz", &nco);
            str[n - 1].ang1 = parseFloatAngleArray(doc, cur, "ang1", &nco);
            str[n - 1].ang2 = parseFloatAngleArray(doc, cur, "ang2", &nco);
            str[n - 1].res = parseFloatArray(doc, cur, "resistance", &nco);

//str[n-1].res = str[n-1].r    ;  
// Also fix FREE HEAP as commented out for res 
            parseFloat(doc, cur, "abs_error", &str[n - 1].aerr);
            parseFloat(doc, cur, "rel_error", &str[n - 1].rerr);

            nrnz = parseIntArray(doc, cur, "modelnrnz", &nco);
            if (nco >= 0 && nco <= 2 && nrnz != NULL) {
                for (i = 0; i < nco; i++) str[n - 1].modelnrnz[i] = nrnz[i];
                free(nrnz);
            }

            printPfPassive(str[n - 1]);
        }
        cur = cur->next;
    }
    *np = n;    // Number of Tags Found
    return str;
}

PFSUPPLIES* parsePfSupplies(xmlDocPtr doc, xmlNodePtr cur, PFSUPPLIES* str, int* np)
{

    int n = 0;
    xmlChar* att;    // General Input of tag attribute values

    *np = 0;
    cur = cur->xmlChildrenNode;
    while (cur != NULL) {
        IDAM_LOGF(LOG_DEBUG, "%s\n", (char*)cur->name);
        if ((!xmlStrcmp(cur->name, (const xmlChar*)"instance"))) {
            n++;
            str = (PFSUPPLIES*)realloc((void*)str, n * sizeof(PFSUPPLIES));

            IDAM_LOGF(LOG_DEBUG, "%d: %p\n", n, str);
            initPfSupplies(&str[n - 1]);

// Attributes 

            if ((att = xmlGetProp(cur, (xmlChar*)"id")) != NULL) {
                if (xmlStrlen(att) > 0) strcpy(str[n - 1].id, (char*)att);
                IDAM_LOGF(LOG_DEBUG, "Pf Supplies ID: %s\n", str[n - 1].id);
                xmlFree(att);
            }

            parseInstance(cur, &str[n - 1].instance);

// Child Tags	 

            parseFloat(doc, cur, "abs_error", &str[n - 1].aerr);
            parseFloat(doc, cur, "rel_error", &str[n - 1].rerr);

            printPfSupplies(str[n - 1]);
        }
        cur = cur->next;
    }
    *np = n;    // Number of Tags Found
    return str;
}

PFCIRCUIT* parsePfCircuits(xmlDocPtr doc, xmlNodePtr cur, PFCIRCUIT* str, int* np)
{

    int n = 0;
    xmlChar* att;    // General Input of tag attribute values

    *np = 0;
    cur = cur->xmlChildrenNode;
    while (cur != NULL) {
        IDAM_LOGF(LOG_DEBUG, "%s\n", (char*)cur->name);
        if ((!xmlStrcmp(cur->name, (const xmlChar*)"instance"))) {
            n++;
            str = (PFCIRCUIT*)realloc((void*)str, n * sizeof(PFCIRCUIT));

            IDAM_LOGF(LOG_DEBUG, "%d: %p\n", n, str);
            initPfCircuits(&str[n - 1]);

// Attributes 

            if ((att = xmlGetProp(cur, (xmlChar*)"id")) != NULL) {
                if (xmlStrlen(att) > 0) strcpy(str[n - 1].id, (char*)att);
                IDAM_LOGF(LOG_DEBUG, "Pf Circuits ID: %s\n", str[n - 1].id);
                xmlFree(att);
            }

            parseInstance(cur, &str[n - 1].instance);

// Child Tags	 

            str[n - 1].coil = parseIntArray(doc, cur, "coil_connect", &str[n - 1].nco);
            parseInt(doc, cur, "supply_connect", &str[n - 1].supply);

            printPfCircuits(str[n - 1]);
        }
        cur = cur->next;
    }
    *np = n;    // Number of Tags Found
    return str;
}

PLASMACURRENT* parsePlasmaCurrent(xmlDocPtr doc, xmlNodePtr cur, PLASMACURRENT* str)
{
    cur = cur->xmlChildrenNode;
    while (cur != NULL) {
        IDAM_LOGF(LOG_DEBUG, "%s\n", (char*)cur->name);
        if ((!xmlStrcmp(cur->name, (const xmlChar*)"instance"))) {
            str = (PLASMACURRENT*)realloc((void*)str, sizeof(PLASMACURRENT));

            IDAM_LOGF(LOG_DEBUG, "%p\n", str);
            initPlasmaCurrent(str);

// Attributes  

            parseInstance(cur, &(str->instance));

// Child Tags	  

            parseFloat(doc, cur, "abs_error", &str->aerr);
            parseFloat(doc, cur, "rel_error", &str->rerr);

            printPlasmaCurrent(*str);
        }
        cur = cur->next;
    }
    return str;
}

DIAMAGNETIC* parseDiaMagnetic(xmlDocPtr doc, xmlNodePtr cur, DIAMAGNETIC* str)
{
    cur = cur->xmlChildrenNode;
    while (cur != NULL) {
        IDAM_LOGF(LOG_DEBUG, "%s\n", (char*)cur->name);
        if ((!xmlStrcmp(cur->name, (const xmlChar*)"instance"))) {
            str = (DIAMAGNETIC*)realloc((void*)str, sizeof(DIAMAGNETIC));

            IDAM_LOGF(LOG_DEBUG, "%p\n", str);
            initDiaMagnetic(str);

// Attributes  

            parseInstance(cur, &(str->instance));

// Child Tags	  

            parseFloat(doc, cur, "abs_error", &str->aerr);
            parseFloat(doc, cur, "rel_error", &str->rerr);

            printDiaMagnetic(*str);
        }
        cur = cur->next;
    }
    return str;
}

TOROIDALFIELD* parseToroidalField(xmlDocPtr doc, xmlNodePtr cur, TOROIDALFIELD* str)
{
    cur = cur->xmlChildrenNode;
    while (cur != NULL) {
        IDAM_LOGF(LOG_DEBUG, "%s\n", (char*)cur->name);
        if ((!xmlStrcmp(cur->name, (const xmlChar*)"instance"))) {
            str = (TOROIDALFIELD*)realloc((void*)str, sizeof(TOROIDALFIELD));

            IDAM_LOGF(LOG_DEBUG, "%p\n", str);
            initToroidalField(str);

            // Attributes

            parseInstance(cur, &(str->instance));

            // Child Tags

            parseFloat(doc, cur, "abs_error", &str->aerr);
            parseFloat(doc, cur, "rel_error", &str->rerr);

            printToroidalField(*str);
        }
        cur = cur->next;
    }
    return str;
}

LIMITER* parseLimiter(xmlDocPtr doc, xmlNodePtr cur, LIMITER* str)
{
    int nco = 0;
    xmlChar* att;    // General Input of tag attribute values

    cur = cur->xmlChildrenNode;
    while (cur != NULL) {
        IDAM_LOGF(LOG_DEBUG, "%s\n", (char*)cur->name);
        if ((!xmlStrcmp(cur->name, (const xmlChar*)"instance"))) {
            str = (LIMITER*)realloc((void*)str, sizeof(LIMITER));

            IDAM_LOGF(LOG_DEBUG, "%p\n", str);
            initLimiter(str);

            // Attributes

            if ((att = xmlGetProp(cur, (xmlChar*)"factor")) != NULL) {
                if (xmlStrlen(att) > 0) {
                    str->factor = (float)atof((char*)att);
                }
                IDAM_LOGF(LOG_DEBUG, "Limiter Coordinates Factor: %f\n", str->factor);
                xmlFree(att);
            }

            // Child Tags

            str->r = parseFloatArray(doc, cur, "r", &str->nco);
            str->z = parseFloatArray(doc, cur, "z", &nco);

            printLimiter(*str);
        }
        cur = cur->next;
    }
    return str;
}

int parseEfitXML(const char* xmlfile, EFIT* efit)
{
    int ninst;

    xmlDocPtr doc;
    xmlNodePtr cur;

    xmlChar* att;    // General Input of tag attribute values

    if ((doc = xmlParseFile(xmlfile)) == NULL) {
        THROW_ERROR(999, "XML Not Parsed - Problem with File");
    }

    if ((cur = xmlDocGetRootElement(doc)) == NULL) {
        xmlFreeDoc(doc);
        THROW_ERROR(999, "Empty XML Document");
    }

// Search for the <itm> or <device> tags 

    if (xmlStrcmp(cur->name, (const xmlChar*)"itm")) {
        if (xmlStrcmp(cur->name, (const xmlChar*)"device")) {
            xmlFreeDoc(doc);
            THROW_ERROR(999, "XML Document has neither an ITM nor a DEVICE tag");
        } else {
            if ((att = xmlGetProp(cur, (xmlChar*)"name")) != NULL) {
                if (xmlStrlen(att) > 0) strcpy(efit->device, (char*)att);
                IDAM_LOGF(LOG_DEBUG, "Device Name: %s\n", efit->device);
                xmlFree(att);
            }
            if ((att = xmlGetProp(cur, (xmlChar*)"pulse")) != NULL) {
                if (xmlStrlen(att) > 0) efit->exp_number = atoi((char*)att);
                IDAM_LOGF(LOG_DEBUG, "Pulse Number: %d\n", efit->exp_number);
                xmlFree(att);
            }
        }
    }

    cur = cur->xmlChildrenNode;
    while (cur != NULL) {
        IDAM_LOGF(LOG_DEBUG, "%s\n", (char*)cur->name);

        if ((!xmlStrcmp(cur->name, (const xmlChar*)"magprobes"))) {

            if ((att = xmlGetProp(cur, (xmlChar*)"number")) != NULL) {
                if (xmlStrlen(att) > 0) efit->nmagprobes = atoi((char*)att);
                IDAM_LOGF(LOG_DEBUG, "No. Mag Probes: %d\n", efit->nmagprobes);
                xmlFree(att);
            }

            ninst = 0;
            efit->magprobe = parseMagProbe(doc, cur, efit->magprobe, &ninst);
            if (ninst != efit->nmagprobes) {
                xmlFreeDoc(doc);
                THROW_ERROR(999, "Inconsistent Number of Magnetic Probes");
            }

        }

        if ((!xmlStrcmp(cur->name, (const xmlChar*)"flux"))) {

            if ((att = xmlGetProp(cur, (xmlChar*)"number")) != NULL) {
                if (xmlStrlen(att) > 0) efit->nfluxloops = atoi((char*)att);
                IDAM_LOGF(LOG_DEBUG, "No. Flux Loops: %d\n", efit->nfluxloops);
                xmlFree(att);
            }

            ninst = 0;
            efit->fluxloop = parseFluxLoop(doc, cur, efit->fluxloop, &ninst);
            if (ninst != efit->nfluxloops) {
                xmlFreeDoc(doc);
                THROW_ERROR(999, "Inconsistent Number of Flux Loops");
            }

        }

        if ((!xmlStrcmp(cur->name, (const xmlChar*)"pfpassive"))) {

            if ((att = xmlGetProp(cur, (xmlChar*)"number")) != NULL) {
                if (xmlStrlen(att) > 0) efit->npfpassive = atoi((char*)att);
                IDAM_LOGF(LOG_DEBUG, "No. PF Passive Elements: %d\n", efit->npfpassive);
                xmlFree(att);
            }

            ninst = 0;
            efit->pfpassive = parsePfPassive(doc, cur, efit->pfpassive, &ninst);
            if (ninst != efit->npfpassive) {
                xmlFreeDoc(doc);
                THROW_ERROR(999, "Inconsistent Number of PF Passive Elements");
            }

        }

        if ((!xmlStrcmp(cur->name, (const xmlChar*)"pfsupplies"))) {

            if ((att = xmlGetProp(cur, (xmlChar*)"number")) != NULL) {
                if (xmlStrlen(att) > 0) efit->npfsupplies = atoi((char*)att);
                IDAM_LOGF(LOG_DEBUG, "No. PF Supplies: %d\n", efit->npfsupplies);
                xmlFree(att);
            }

            ninst = 0;
            efit->pfsupplies = parsePfSupplies(doc, cur, efit->pfsupplies, &ninst);
            if (ninst != efit->npfsupplies) {
                xmlFreeDoc(doc);
                THROW_ERROR(999, "Inconsistent Number of PF Supplies");
            }

        }

        if ((!xmlStrcmp(cur->name, (const xmlChar*)"pfcoils"))) {

            if ((att = xmlGetProp(cur, (xmlChar*)"number")) != NULL) {
                if (xmlStrlen(att) > 0) efit->npfcoils = atoi((char*)att);
                IDAM_LOGF(LOG_DEBUG, "No. PF Coils: %d\n", efit->npfcoils);
                xmlFree(att);
            }

            ninst = 0;
            efit->pfcoils = parsePfCoils(doc, cur, efit->pfcoils, &ninst);
            if (ninst != efit->npfcoils) {
                xmlFreeDoc(doc);
                THROW_ERROR(999, "Inconsistent Number of PF Coils");
            }

        }

        if ((!xmlStrcmp(cur->name, (const xmlChar*)"plasmacurrent"))) {
            efit->plasmacurrent = parsePlasmaCurrent(doc, cur, efit->plasmacurrent);
            efit->nplasmacurrent = 1;
        }

        if ((!xmlStrcmp(cur->name, (const xmlChar*)"diamagneticflux"))) {
            efit->diamagnetic = parseDiaMagnetic(doc, cur, efit->diamagnetic);
            efit->ndiamagnetic = 1;
        }

        if ((!xmlStrcmp(cur->name, (const xmlChar*)"toroidalfield"))) {
            efit->toroidalfield = parseToroidalField(doc, cur, efit->toroidalfield);
            efit->ntoroidalfield = 1;
        }

        if ((!xmlStrcmp(cur->name, (const xmlChar*)"pfcircuits"))) {

            if ((att = xmlGetProp(cur, (xmlChar*)"number")) != NULL) {
                if (xmlStrlen(att) > 0) efit->npfcircuits = atoi((char*)att);
                IDAM_LOGF(LOG_DEBUG, "No. PF Circuits: %d\n", efit->npfcircuits);
                xmlFree(att);
            }

            ninst = 0;
            efit->pfcircuit = parsePfCircuits(doc, cur, efit->pfcircuit, &ninst);
            if (ninst != efit->npfcircuits) {
                xmlFreeDoc(doc);
                THROW_ERROR(999, "Inconsistent Number of PF Circuits");
            }
        }

        if ((!xmlStrcmp(cur->name, (const xmlChar*)"limiter"))) {
            efit->limiter = parseLimiter(doc, cur, efit->limiter);
            efit->nlimiter = 1;
        }

        cur = cur->next;
    }

    xmlFreeDoc(doc);

    return 0;
} 

