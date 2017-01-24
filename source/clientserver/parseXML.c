//! $LastChangedRevision: 353 $
//! $LastChangedDate: 2013-11-18 15:32:28 +0000 (Mon, 18 Nov 2013) $
//! $LastChangedBy: dgm $
//! $HeadURL: https://fussvn.fusion.culham.ukaea.org.uk/svnroot/IDAM/development/source/server/parseXML.c $

/*---------------------------------------------------------------
* IDAM XML Parser
*
* Input Arguments:	char *xml
*
* Returns:		parseXML	TRUE if parse was successful

*			ACTIONS		Actions Structure
*
* Calls
*
* Notes:
*
* ToDo:
*
* Change History:
*
* 0.1   18Oct2006	D.G.Muir	Significant Changes
* 	06Nov2006	D.G.Muir	Error Model/Error Asymmetry
* 	13Dec2006			errparams now fixed length array rather than heap
* 	22Feb2007 	dgm	TIMEOFFSET structure changed to enable different
*                      		correction methods to be applied.
*       09Mar2007 	dgm	Additional items to COMPOSITE (order, file, format) and DIMCOMPOSITE (file, format) Structures
*	09Jul2007	dgm	debugon enabled
*	29Feb2008	dgm	xmlCleanupParser added to parser housekeeping
// 28Jan2010	dgm	Added invert attribute to the CALIBRATION and DIMCALIBRATION tags + integer invert type to calibration structure
// 05Nov2010	dgm	Added member attribute to SUBSET tag
// 10Nov2010	dgm	Added function attribute to SUBSET tag
// 05May2011	dgm	don't include idamserver.h if these functions are used in the ROIDAM DLM toolset
*-------------------------------------------------------------------------------------------*/

#include "parseXML.h"

#include <logging/idamLog.h>
#include <include/idamtypes.h>
#include <include/idamclientserverprivate.h>
#include "TrimString.h"
#include "parseOperation.h"
#include "idamErrorLog.h"

#ifndef NOXMLPARSER

// Simple Tags with Delimited List of Floating Point Values
// Assume No Attributes

float* parseFloatArray(xmlDocPtr doc, xmlNodePtr cur, char* target, int* n)
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
                IDAM_LOGF(LOG_DEBUG, "parseFloatArray: [%d] %s %s \n", lkey, target, key);
                item = strtok((char*) key, delim);
                if (item != NULL) {
                    nco++;
                    IDAM_LOGF(LOG_DEBUG, "parseFloatArray: [%d] %s \n", nco, item);
                    value = (float*) realloc((void*) value, nco * sizeof(float));
                    value[nco - 1] = atof(item);
                    IDAM_LOGF(LOG_DEBUG, "parseFloatArray: [%d] %s %f\n", nco, item, value[nco - 1]);
                    while ((item = strtok(NULL, delim)) != NULL && nco <= XMLMAXLOOP) {
                        nco++;
                        value = (float*) realloc((void*) value, nco * sizeof(float));
                        value[nco - 1] = atof(item);
                        IDAM_LOGF(LOG_DEBUG, "parseFloatArray: [%d] %s %f\n", nco, item, value[nco - 1]);
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


void parseFixedLengthArray(xmlNodePtr cur, char* target, void* array, int arraytype, int* n)
{
    xmlChar* att = NULL;
    *n = 0;
    char* delim = ",";
    char* item;
    int nco = 0;

    double* dp;
    float* fp;
    int* ip;
    long* lp;

    if ((att = xmlGetProp(cur, (xmlChar*) target)) != NULL) {
        convertNonPrintable((char*) att);
        if (strlen((char*) att) > 0) {
            int l = (int) strlen((char*) att);
            IDAM_LOGF(LOG_DEBUG, "parseFixedLengthArray: [%d] %s %s \n", l, target, att);
            item = strtok((char*) att, delim);
            if (item != NULL) {
                nco++;
                switch (arraytype) {
                    case TYPE_FLOAT:
                        fp = (float*) array;
                        fp[nco - 1] = atof(item);
                        break;
                    case TYPE_DOUBLE:
                        dp = (double*) array;
                        dp[nco - 1] = strtod(item, NULL);
                        break;
                    case TYPE_CHAR: {
                        char* p = (char*) array;
                        p[nco - 1] = (char) atoi(item);
                        break;
                    }
                    case TYPE_SHORT: {
                        short* p = (short*) array;
                        p[nco - 1] = (short) atoi(item);
                        break;
                    }
                    case TYPE_INT:
                        ip = (int*) array;
                        ip[nco - 1] = (int) atoi(item);
                        break;
                    case TYPE_LONG:
                        lp = (long*) array;
                        lp[nco - 1] = (long) atol(item);
                        break;
                    case TYPE_UNSIGNED_CHAR: {
                        unsigned char* p = (unsigned char*) array;
                        p[nco - 1] = (unsigned char) atoi(item);
                        break;
                    }
                    case TYPE_UNSIGNED_SHORT: {
                        unsigned short* p = (unsigned short*) array;
                        p[nco - 1] = (unsigned short) atoi(item);
                        break;
                    }
                    case TYPE_UNSIGNED: {
                        unsigned int* p = (unsigned int*) array;
                        p[nco - 1] = (unsigned int) atoi(item);
                        break;
                    }
                    case TYPE_UNSIGNED_LONG: {
                        unsigned long* p = (unsigned long*) array;
                        p[nco - 1] = (unsigned long) atol(item);
                        break;
                    }
                    default:
                        return;
                }

                while ((item = strtok(NULL, delim)) != NULL && nco <= MAXDATARANK) {
                    nco++;
                    switch (arraytype) {
                        case TYPE_FLOAT:
                            fp = (float*) array;
                            fp[nco - 1] = atof(item);
                            break;
                        case TYPE_DOUBLE:
                            dp = (double*) array;
                            dp[nco - 1] = strtod(item, NULL);
                            break;
                        case TYPE_CHAR: {
                            char* p = (char*) array;
                            p[nco - 1] = (char) atoi(item);
                            break;
                        }
                        case TYPE_SHORT: {
                            short* p = (short*) array;
                            p[nco - 1] = (short) atoi(item);
                            break;
                        }
                        case TYPE_INT:
                            ip = (int*) array;
                            ip[nco - 1] = (int) atoi(item);
                            break;
                        case TYPE_LONG:
                            lp = (long*) array;
                            lp[nco - 1] = (long) atol(item);
                            break;
                        case TYPE_UNSIGNED_CHAR: {
                            unsigned char* p = (unsigned char*) array;
                            p[nco - 1] = (unsigned char) atoi(item);
                            break;
                        }
                        case TYPE_UNSIGNED_SHORT: {
                            unsigned short* p = (unsigned short*) array;
                            p[nco - 1] = (unsigned short) atoi(item);
                            break;
                        }
                        case TYPE_UNSIGNED: {
                            unsigned int* p = (unsigned int*) array;
                            p[nco - 1] = (unsigned int) atoi(item);
                            break;
                        }
                        case TYPE_UNSIGNED_LONG: {
                            unsigned long* p = (unsigned long*) array;
                            p[nco - 1] = (unsigned long) atol(item);
                            break;
                        }
                        default:
                            return;
                    }
                }
            }
        }
        *n = nco;
        xmlFree(att);
    }

    return;
}


void parseFixedLengthStrArray(xmlNodePtr cur, char* target, char array[MAXDATARANK][SXMLMAXSTRING], int* n)
{
    xmlChar* att = NULL;
    *n = 0;
    char* delim = ",";
    char* item;
    int nco = 0;

    if ((att = xmlGetProp(cur, (xmlChar*) target)) != NULL) {
        if (strlen((char*) att) > 0) {
            int l = (int) strlen((char*) att);
            IDAM_LOGF(LOG_DEBUG, "parseFixedLengthStrArray: [%d] %s %s \n", l, target, att);
            item = strtok((char*) att, delim);
            if (item != NULL) {
                nco++;
                if (strlen(item) < SXMLMAXSTRING) {
                    strcpy(array[nco - 1], item);
                } else {
                    strncpy(array[nco - 1], item, SXMLMAXSTRING - 1);
                    array[nco - 1][SXMLMAXSTRING - 1] = '\0';
                }

                while ((item = strtok(NULL, delim)) != NULL && nco <= MAXDATARANK) {
                    nco++;
                    if (strlen(item) < SXMLMAXSTRING) {
                        strcpy(array[nco - 1], item);
                    } else {
                        strncpy(array[nco - 1], item, SXMLMAXSTRING - 1);
                        array[nco - 1][SXMLMAXSTRING - 1] = '\0';
                    }
                }
            }
        }
        *n = nco;
        xmlFree(att);
    }

    return;
}


// Decode Scale Value

double deScale(char* scale)
{
    if (strlen(scale) == 0) return (1.0E0);
    if (!strncmp(scale, "milli", 5)) {
        return (1.0E-3);
    } else {
        if (!strncmp(scale, "micro", 5)) {
            return (1.0E-6);
        } else {
            if (!strncmp(scale, "nano", 4)) {
                return (1.0E-9);
            }
        }
    }
    return (1.0E0);    // Default value
}

// Locate and extract Named Parameter (Numerical and String) Values
// Assume only 1 tag per document

void parseTargetValue(xmlDocPtr doc, xmlNodePtr cur, char* target, double* value)
{
    xmlChar* key = NULL;
    xmlChar* scale = NULL;
    cur = cur->xmlChildrenNode;
    while (cur != NULL) {
        if ((!xmlStrcmp(cur->name, (const xmlChar*) target))) {
            scale = xmlGetProp(cur, (xmlChar*) "scale");
            key = xmlNodeListGetString(doc, cur->xmlChildrenNode, 1);
            if (key != NULL) *value = (double) atof((char*) key);
            if (scale != NULL) *value = *value * deScale((char*) scale);
            xmlFree(key);
            xmlFree(scale);
            break;
        }
        cur = cur->next;
    }
    return;
}

void parseTargetString(xmlDocPtr doc, xmlNodePtr cur, char* target, char* str)
{
    xmlChar* key = NULL;
    cur = cur->xmlChildrenNode;
    while (cur != NULL) {
        if ((!xmlStrcmp(cur->name, (const xmlChar*) target))) {
            key = xmlNodeListGetString(doc, cur->xmlChildrenNode, 1);
            if (key != NULL) strcpy(str, (char*) key);
            xmlFree(key);
            break;
        }
        cur = cur->next;
    }
    return;
}

void parseTimeOffset(xmlDocPtr doc, xmlNodePtr cur, ACTIONS* actions)
{

    xmlChar* att;    // General Input of tag attribute values

    ACTION* str = actions->action;
    int n = actions->nactions;        // Counter

    cur = cur->xmlChildrenNode;
    while (cur != NULL) {
        IDAM_LOGF(LOG_DEBUG, "parseTimeOffset: %s\n", (char*) cur->name);
        if ((!xmlStrcmp(cur->name, (const xmlChar*) "time_offset"))) {
            n++;
            str = (ACTION*) realloc((void*) str, n * sizeof(ACTION));

            initAction(&str[n - 1]);
            str[n - 1].actionType = TIMEOFFSETTYPE;
            initTimeOffset(&str[n - 1].timeoffset);

// Attributes

            if ((att = xmlGetProp(cur, (xmlChar*) "id")) != NULL) {
                if (strlen((char*) att) > 0) str[n - 1].actionId = atoi((char*) att);
                IDAM_LOGF(LOG_DEBUG, "Action ID: %d\n", str[n - 1].actionId);
                xmlFree(att);
            }

            if ((att = xmlGetProp(cur, (xmlChar*) "exp_number_start")) != NULL) {
                if (strlen((char*) att) > 0) str[n - 1].exp_range[0] = atoi((char*) att);
                IDAM_LOGF(LOG_DEBUG, "Exp Number Range Start: %d\n", str[n - 1].exp_range[0]);
                xmlFree(att);
            }

            if ((att = xmlGetProp(cur, (xmlChar*) "exp_number_end")) != NULL) {
                if (strlen((char*) att) > 0) str[n - 1].exp_range[1] = atoi((char*) att);
                IDAM_LOGF(LOG_DEBUG, "Exp Number Range End : %d\n", str[n - 1].exp_range[1]);
                xmlFree(att);
            }

            if ((att = xmlGetProp(cur, (xmlChar*) "pass_start")) != NULL) {
                if (strlen((char*) att) > 0) str[n - 1].pass_range[0] = atoi((char*) att);
                IDAM_LOGF(LOG_DEBUG, "Pass Number Range Start: %d\n", str[n - 1].pass_range[0]);
                xmlFree(att);
            }

            if ((att = xmlGetProp(cur, (xmlChar*) "pass_end")) != NULL) {
                if (strlen((char*) att) > 0) str[n - 1].pass_range[1] = atoi((char*) att);
                IDAM_LOGF(LOG_DEBUG, "Pass Number Range End  : %d\n", str[n - 1].pass_range[1]);
                xmlFree(att);
            }

            if ((att = xmlGetProp(cur, (xmlChar*) "value")) != NULL) {
                if (strlen((char*) att) > 0) {
                    str[n - 1].timeoffset.offset = (double) atof((char*) att);
                    IDAM_LOGF(LOG_DEBUG, "Time Offset  : %f\n", str[n - 1].timeoffset.offset);
                }
                xmlFree(att);
            }

            if ((att = xmlGetProp(cur, (xmlChar*) "method")) != NULL) {
                if (strlen((char*) att) > 0) str[n - 1].timeoffset.method = (int) atoi((char*) att);
                IDAM_LOGF(LOG_DEBUG, "Time Offset Method  : %d\n", str[n - 1].timeoffset.method);
                xmlFree(att);
            }

            if ((att = xmlGetProp(cur, (xmlChar*) "start")) != NULL) {
                if (strlen((char*) att) > 0) {
                    str[n - 1].timeoffset.offset = (double) atof((char*) att);
                    IDAM_LOGF(LOG_DEBUG, "Start Time  : %f\n", str[n - 1].timeoffset.offset);
                }
                xmlFree(att);
            }

            if ((att = xmlGetProp(cur, (xmlChar*) "interval")) != NULL) {
                if (strlen((char*) att) > 0) {
                    str[n - 1].timeoffset.interval = (double) atof((char*) att);
                    IDAM_LOGF(LOG_DEBUG, "Time Interval: %f\n", str[n - 1].timeoffset.interval);
                }
                xmlFree(att);
            }

            if ((att = xmlGetProp(cur, (xmlChar*) "scale")) != NULL) {
                if (strlen((char*) att) > 0)
                    str[n - 1].timeoffset.offset = deScale((char*) att) * str[n - 1].timeoffset.offset;
                IDAM_LOGF(LOG_DEBUG, "Scaled Time Offset  : %f\n", str[n - 1].timeoffset.offset);
                xmlFree(att);
            }
        }
        cur = cur->next;
    }
    actions->nactions = n;    // Number of Tags Found
    actions->action = str;    // Array of Actions bounded by a Ranges
    return;
}


void parseCompositeSubset(xmlDocPtr doc, xmlNodePtr cur, COMPOSITE* comp)
{

    xmlChar* att;    // General Input of tag attribute values

    SUBSET* str = comp->subsets;
    int n = 0;                // Counter
    int i, n0, n1;

// Attributes

    cur = cur->xmlChildrenNode;
    while (cur != NULL) {
        IDAM_LOGF(LOG_DEBUG, "parseCompositeSubset: %s\n", (char*) cur->name);
        if ((!xmlStrcmp(cur->name, (const xmlChar*) "subset"))) {
            n++;
            str = (SUBSET*) realloc((void*) str, n * sizeof(SUBSET));

            initSubset(&str[n - 1]);

// Attributes

            if ((att = xmlGetProp(cur, (xmlChar*) "data")) != NULL) {
                if (strlen((char*) att) > 0) strcpy(str[n - 1].data_signal, (char*) att);
                IDAM_LOGF(LOG_DEBUG, "Subset Signal: %s\n", str[n - 1].data_signal);
                xmlFree(att);
            }

            if ((att = xmlGetProp(cur, (xmlChar*) "reform")) != NULL) {
                if (att[0] == 'Y' || att[0] == 'y') str[n - 1].reform = 1;
                xmlFree(att);
            }

            if ((att = xmlGetProp(cur, (xmlChar*) "member")) != NULL) {
                if (strlen((char*) att) > 0) strcpy(str[n - 1].member, (char*) att);
                IDAM_LOGF(LOG_DEBUG, "Subset member: %s\n", str[n - 1].member);
                xmlFree(att);
            }

            if ((att = xmlGetProp(cur, (xmlChar*) "function")) != NULL) {
                if (strlen((char*) att) > 0) strcpy(str[n - 1].function, (char*) att);
                IDAM_LOGF(LOG_DEBUG, "Subset function: %s\n", str[n - 1].function);
                xmlFree(att);
            }

            if ((att = xmlGetProp(cur, (xmlChar*) "order")) != NULL) {
                if (strlen((char*) att) > 0) str[n - 1].order = atoi((char*) att);
                IDAM_LOGF(LOG_DEBUG, "Subset order: %d\n", str[n - 1].order);
                xmlFree(att);
            }

// Fixed Length Attribute Arrays

            parseFixedLengthStrArray(cur, "operation", str[n - 1].operation, &str[n - 1].nbound);
            for (i = 0; i < str[n - 1].nbound; i++)
                str[n - 1].dimid[i] = i;                    // Ordering is as DATA[4][3][2][1][0]

            parseFixedLengthArray(cur, "bound", (void*) str[n - 1].bound, TYPE_DOUBLE, &n0);
            parseFixedLengthArray(cur, "dimid", (void*) str[n - 1].dimid, TYPE_INT, &n1);

            //if(n0 == 0 && n1 == 0 && str[n-1].nbound > 0){					// Assume Array Reshaping has been requested
            //   for(i=0;i<str[n-1].nbound;i++) str[n-1].dimid[i] = str[n-1].nbound-i-1;	// Ordering is as DATA[4][3][2][1][0]
            //}

            if (idamParseOperation(&str[n - 1]) != 0) return;

            for (i = 0; i < str[n - 1].nbound; i++) {
                IDAM_LOGF(LOG_DEBUG, "Subsetting Bounding Values : %e\n", str[n - 1].bound[i]);
                IDAM_LOGF(LOG_DEBUG, "Subsetting Operation       : %s\n", str[n - 1].operation[i]);
                IDAM_LOGF(LOG_DEBUG, "Dimension ID               : %d\n", str[n - 1].dimid[i]);
                IDAM_LOGF(LOG_DEBUG, "Subsetting Is Index?       : %d\n", str[n - 1].isindex[i]);
                IDAM_LOGF(LOG_DEBUG, "Subsetting Lower Index     : %d\n", (int) str[n - 1].lbindex[i]);
                IDAM_LOGF(LOG_DEBUG, "Subsetting Upper Index     : %d\n", (int) str[n - 1].ubindex[i]);
            }
        }
        cur = cur->next;
    }
    comp->nsubsets = n;        // Number of Subsets
    comp->subsets = str;    // Array of Subset Actions

    return;
}


void parseMaps(xmlDocPtr doc, xmlNodePtr cur, COMPOSITE* comp)
{
    return;
}


void parseDimComposite(xmlDocPtr doc, xmlNodePtr cur, COMPOSITE* comp)
{

    xmlChar* att;    // General Input of tag attribute values

    DIMENSION* str = comp->dimensions;
    int n = 0;                // Counter

    cur = cur->xmlChildrenNode;
    while (cur != NULL) {
        IDAM_LOGF(LOG_DEBUG, "parseDimComposite: %s\n", (char*) cur->name);
        if ((!xmlStrcmp(cur->name, (const xmlChar*) "composite_dim"))) {
            n++;
            str = (DIMENSION*) realloc((void*) str, n * sizeof(DIMENSION));

            initDimension(&str[n - 1]);
            str[n - 1].dimType = DIMCOMPOSITETYPE;
            initDimComposite(&str[n - 1].dimcomposite);

// Attributes

            if ((att = xmlGetProp(cur, (xmlChar*) "to_dim")) != NULL) {            // Target Dimension
                if (strlen((char*) att) > 0) {
                    str[n - 1].dimid = atoi((char*) att);                // Duplicate these tags for convenience
                    str[n - 1].dimcomposite.to_dim = atoi((char*) att);
                }
                IDAM_LOGF(LOG_DEBUG, "To Dimension  : %d\n", str[n - 1].dimid);
                xmlFree(att);
            }

            if ((att = xmlGetProp(cur, (xmlChar*) "from_dim")) !=
                NULL) {        // Swap with this Dimension otherwise swap with Data
                if (strlen((char*) att) > 0) str[n - 1].dimcomposite.from_dim = atoi((char*) att);
                IDAM_LOGF(LOG_DEBUG, "From Dimension  : %d\n", str[n - 1].dimcomposite.from_dim);
                xmlFree(att);
            }

            if ((att = xmlGetProp(cur, (xmlChar*) "dim")) != NULL ||
                (att = xmlGetProp(cur, (xmlChar*) "data")) != NULL) {
                if (strlen((char*) att) > 0) strcpy(str[n - 1].dimcomposite.dim_signal, (char*) att);
                IDAM_LOGF(LOG_DEBUG, "Dimension Signal  : %s\n", str[n - 1].dimcomposite.dim_signal);
                xmlFree(att);
            }

            if ((att = xmlGetProp(cur, (xmlChar*) "error")) != NULL) {
                if (strlen((char*) att) > 0) strcpy(str[n - 1].dimcomposite.dim_error, (char*) att);
                IDAM_LOGF(LOG_DEBUG, "Error Signal  : %s\n", str[n - 1].dimcomposite.dim_error);
                xmlFree(att);
            }
            if ((att = xmlGetProp(cur, (xmlChar*) "aserror")) != NULL) {
                if (strlen((char*) att) > 0) strcpy(str[n - 1].dimcomposite.dim_aserror, (char*) att);
                IDAM_LOGF(LOG_DEBUG, "Error Signal  : %s\n", str[n - 1].dimcomposite.dim_aserror);
                xmlFree(att);
            }
            if ((att = xmlGetProp(cur, (xmlChar*) "file")) != NULL) {
                if (strlen((char*) att) > 0) strcpy(str[n - 1].dimcomposite.file, (char*) att);
                IDAM_LOGF(LOG_DEBUG, "Dimension Source File: %s\n", str[n - 1].dimcomposite.file);
                xmlFree(att);
            }
            if ((att = xmlGetProp(cur, (xmlChar*) "format")) != NULL) {
                if (strlen((char*) att) > 0) strcpy(str[n - 1].dimcomposite.format, (char*) att);
                IDAM_LOGF(LOG_DEBUG, "Dimension Source File Format: %s\n", str[n - 1].dimcomposite.format);
                xmlFree(att);
            }


        }
        cur = cur->next;
    }
    comp->ndimensions = n;    // Number of Tags Found
    comp->dimensions = str;    // Array of Composite Signal Actions on Dimensions
    return;
}

void parseComposite(xmlDocPtr doc, xmlNodePtr cur, ACTIONS* actions)
{

    xmlChar* att;    // General Input of tag attribute values

    ACTION* str = actions->action;
    int n = actions->nactions;        // Counter

    cur = cur->xmlChildrenNode;
    while (cur != NULL) {
        IDAM_LOGF(LOG_DEBUG, "parseComposite: %s\n", (char*) cur->name);
        if ((!xmlStrcmp(cur->name, (const xmlChar*) "composite"))) {
            n++;
            str = (ACTION*) realloc((void*) str, n * sizeof(ACTION));

            initAction(&str[n - 1]);
            str[n - 1].actionType = COMPOSITETYPE;
            initComposite(&str[n - 1].composite);

// Attributes

            if ((att = xmlGetProp(cur, (xmlChar*) "id")) != NULL) {
                if (strlen((char*) att) > 0) str[n - 1].actionId = atoi((char*) att);
                IDAM_LOGF(LOG_DEBUG, "Action ID: %d\n", str[n - 1].actionId);
                xmlFree(att);
            }

            if ((att = xmlGetProp(cur, (xmlChar*) "exp_number_start")) != NULL) {
                if (strlen((char*) att) > 0) str[n - 1].exp_range[0] = atoi((char*) att);
                IDAM_LOGF(LOG_DEBUG, "Exp Number Range Start: %d\n", str[n - 1].exp_range[0]);
                xmlFree(att);
            }

            if ((att = xmlGetProp(cur, (xmlChar*) "exp_number_end")) != NULL) {
                if (strlen((char*) att) > 0) str[n - 1].exp_range[1] = atoi((char*) att);
                IDAM_LOGF(LOG_DEBUG, "Exp Number Range End : %d\n", str[n - 1].exp_range[1]);
                xmlFree(att);
            }

            if ((att = xmlGetProp(cur, (xmlChar*) "pass_start")) != NULL) {
                if (strlen((char*) att) > 0) str[n - 1].pass_range[0] = atoi((char*) att);
                IDAM_LOGF(LOG_DEBUG, "Pass Number Range Start: %d\n", str[n - 1].pass_range[0]);
                xmlFree(att);
            }

            if ((att = xmlGetProp(cur, (xmlChar*) "pass_end")) != NULL) {
                if (strlen((char*) att) > 0) str[n - 1].pass_range[1] = atoi((char*) att);
                IDAM_LOGF(LOG_DEBUG, "Pass Number Range End  : %d\n", str[n - 1].pass_range[1]);
                xmlFree(att);
            }

            if ((att = xmlGetProp(cur, (xmlChar*) "data")) != NULL) {
                if (strlen((char*) att) > 0) strcpy(str[n - 1].composite.data_signal, (char*) att);
                IDAM_LOGF(LOG_DEBUG, "Data Signal  : %s\n", str[n - 1].composite.data_signal);
                xmlFree(att);
            }

            if ((att = xmlGetProp(cur, (xmlChar*) "file")) != NULL) {
                if (strlen((char*) att) > 0) strcpy(str[n - 1].composite.file, (char*) att);
                IDAM_LOGF(LOG_DEBUG, "Data Source File: %s\n", str[n - 1].composite.file);
                xmlFree(att);
            }

            if ((att = xmlGetProp(cur, (xmlChar*) "format")) != NULL) {
                if (strlen((char*) att) > 0) strcpy(str[n - 1].composite.format, (char*) att);
                IDAM_LOGF(LOG_DEBUG, "Source File Format: %s\n", str[n - 1].composite.format);
                xmlFree(att);
            }

            if ((att = xmlGetProp(cur, (xmlChar*) "error")) != NULL) {
                if (strlen((char*) att) > 0) strcpy(str[n - 1].composite.error_signal, (char*) att);
                IDAM_LOGF(LOG_DEBUG, "Error Signal  : %s\n", str[n - 1].composite.error_signal);
                xmlFree(att);
            }
            if ((att = xmlGetProp(cur, (xmlChar*) "aserror")) != NULL) {
                if (strlen((char*) att) > 0) strcpy(str[n - 1].composite.aserror_signal, (char*) att);
                IDAM_LOGF(LOG_DEBUG, "Error Signal  : %s\n", str[n - 1].composite.aserror_signal);
                xmlFree(att);
            }
            if ((att = xmlGetProp(cur, (xmlChar*) "mapto")) != NULL) {
                if (strlen((char*) att) > 0) strcpy(str[n - 1].composite.aserror_signal, (char*) att);
                IDAM_LOGF(LOG_DEBUG, "Map to Signal  : %s\n", str[n - 1].composite.map_to_signal);
                xmlFree(att);
            }


            if ((att = xmlGetProp(cur, (xmlChar*) "order")) != NULL) {
                if (strlen((char*) att) > 0) str[n - 1].composite.order = atoi((char*) att);
                IDAM_LOGF(LOG_DEBUG, "Time Dimension: %d\n", str[n - 1].composite.order);
                xmlFree(att);
            }


// Child Tags

            parseDimComposite(doc, cur, &str[n - 1].composite);
            parseCompositeSubset(doc, cur, &str[n - 1].composite);

// Consolidate Composite Signal name with Subset Signal Name (the Composite record has precedence)

            if (str[n - 1].composite.nsubsets > 0 && strlen(str[n - 1].composite.data_signal) == 0) {
                if (strlen(str[n - 1].composite.subsets[0].data_signal) > 0)
                    strcpy(str[n - 1].composite.data_signal, str[n - 1].composite.subsets[0].data_signal);
            }
        }
        cur = cur->next;
    }

    actions->nactions = n;    // Number of Tags Found
    actions->action = str;    // Array of Actions bounded by a Ranges
    return;
}

void parseDimErrorModel(xmlDocPtr doc, xmlNodePtr cur, ERRORMODEL* mod)
{

    xmlChar* att;    // General Input of tag attribute values
    float* params;
    int i;

    DIMENSION* str = mod->dimensions;
    int n = 0;                // Counter

    cur = cur->xmlChildrenNode;
    while (cur != NULL) {
        IDAM_LOGF(LOG_DEBUG, "parseDimErrorModel: %s\n", (char*) cur->name);
        if ((!xmlStrcmp(cur->name, (const xmlChar*) "dimension"))) {
            n++;
            str = (DIMENSION*) realloc((void*) str, n * sizeof(DIMENSION));

            initDimension(&str[n - 1]);
            str[n - 1].dimType = DIMERRORMODELTYPE;
            initDimErrorModel(&str[n - 1].dimerrormodel);

// Attributes

            if ((att = xmlGetProp(cur, (xmlChar*) "dimid")) != NULL) {            // Target Dimension
                if (strlen((char*) att) > 0) str[n - 1].dimid = atoi((char*) att);
                IDAM_LOGF(LOG_DEBUG, "Dimension : %d\n", str[n - 1].dimid);
                xmlFree(att);
            }
            if ((att = xmlGetProp(cur, (xmlChar*) "model")) != NULL) {            // Error Model
                if (strlen((char*) att) > 0) str[n - 1].dimerrormodel.model = atoi((char*) att);
                IDAM_LOGF(LOG_DEBUG, "Model : %d\n", str[n - 1].dimerrormodel.model);
                xmlFree(att);
            }

// Child Tags

            params = parseFloatArray(doc, cur, "params", &str[n - 1].dimerrormodel.param_n);
            if (params != NULL) {
                if (str[n - 1].dimerrormodel.param_n > MAXERRPARAMS) str[n - 1].dimerrormodel.param_n = MAXERRPARAMS;
                for (i = 0; i < str[n - 1].dimerrormodel.param_n; i++) str[n - 1].dimerrormodel.params[i] = params[i];
                free((void*) params);
            }


        }
        cur = cur->next;
    }
    mod->ndimensions = n;    // Number of Tags Found
    mod->dimensions = str;    // Array of Error Model Actions on Dimensions
    return;
}


void parseErrorModel(xmlDocPtr doc, xmlNodePtr cur, ACTIONS* actions)
{
    xmlChar* att;    // General Input of tag attribute values
    float* params;
    int i;

    ACTION* str = actions->action;
    int n = actions->nactions;        // Counter

    cur = cur->xmlChildrenNode;
    while (cur != NULL) {
        IDAM_LOGF(LOG_DEBUG, "parseErrorModel: %s\n", (char*) cur->name);
        if ((!xmlStrcmp(cur->name, (const xmlChar*) "errormodel"))) {
            n++;
            str = (ACTION*) realloc((void*) str, n * sizeof(ACTION));

            initAction(&str[n - 1]);
            str[n - 1].actionType = ERRORMODELTYPE;
            initErrorModel(&str[n - 1].errormodel);

// Attributes

            if ((att = xmlGetProp(cur, (xmlChar*) "id")) != NULL) {
                if (strlen((char*) att) > 0) str[n - 1].actionId = atoi((char*) att);
                IDAM_LOGF(LOG_DEBUG, "Action ID: %d\n", str[n - 1].actionId);
                xmlFree(att);
            }

            if ((att = xmlGetProp(cur, (xmlChar*) "exp_number_start")) != NULL) {
                if (strlen((char*) att) > 0) str[n - 1].exp_range[0] = atoi((char*) att);
                IDAM_LOGF(LOG_DEBUG, "Exp Number Range Start: %d\n", str[n - 1].exp_range[0]);
                xmlFree(att);
            }

            if ((att = xmlGetProp(cur, (xmlChar*) "exp_number_end")) != NULL) {
                if (strlen((char*) att) > 0) str[n - 1].exp_range[1] = atoi((char*) att);
                IDAM_LOGF(LOG_DEBUG, "Exp Number Range End : %d\n", str[n - 1].exp_range[1]);
                xmlFree(att);
            }

            if ((att = xmlGetProp(cur, (xmlChar*) "pass_start")) != NULL) {
                if (strlen((char*) att) > 0) str[n - 1].pass_range[0] = atoi((char*) att);
                IDAM_LOGF(LOG_DEBUG, "Pass Number Range Start: %d\n", str[n - 1].pass_range[0]);
                xmlFree(att);
            }

            if ((att = xmlGetProp(cur, (xmlChar*) "pass_end")) != NULL) {
                if (strlen((char*) att) > 0) str[n - 1].pass_range[1] = atoi((char*) att);
                IDAM_LOGF(LOG_DEBUG, "Pass Number Range End  : %d\n", str[n - 1].pass_range[1]);
                xmlFree(att);
            }

            if ((att = xmlGetProp(cur, (xmlChar*) "model")) != NULL) {
                if (strlen((char*) att) > 0) str[n - 1].errormodel.model = atoi((char*) att);
                IDAM_LOGF(LOG_DEBUG, "Error Distribution Model: %d\n", str[n - 1].errormodel.model);
                xmlFree(att);
            }

// Child Tags

            params = parseFloatArray(doc, cur, "params", &str[n - 1].errormodel.param_n);
            if (params != NULL) {
                if (str[n - 1].errormodel.param_n > MAXERRPARAMS) str[n - 1].errormodel.param_n = MAXERRPARAMS;
                for (i = 0; i < str[n - 1].errormodel.param_n; i++) str[n - 1].errormodel.params[i] = params[i];
                free((void*) params);
            }

            parseDimErrorModel(doc, cur, &str[n - 1].errormodel);

        }
        cur = cur->next;
    }
    actions->nactions = n;    // Number of Tags Found
    actions->action = str;    // Array of Actions bounded by a Ranges
    return;
}

void parseDimDocumentation(xmlDocPtr doc, xmlNodePtr cur, DOCUMENTATION* document)
{

    xmlChar* att;    // General Input of tag attribute values

    DIMENSION* str = document->dimensions;
    int n = 0;                // Counter

    cur = cur->xmlChildrenNode;
    while (cur != NULL) {
        IDAM_LOGF(LOG_DEBUG, "parseDimDocumentation: %s\n", (char*) cur->name);
        if ((!xmlStrcmp(cur->name, (const xmlChar*) "dimension"))) {
            n++;
            str = (DIMENSION*) realloc((void*) str, n * sizeof(DIMENSION));

            initDimension(&str[n - 1]);
            str[n - 1].dimType = DIMDOCUMENTATIONTYPE;
            initDimDocumentation(&str[n - 1].dimdocumentation);

// Attributes

            if ((att = xmlGetProp(cur, (xmlChar*) "dimid")) != NULL) {            // Target Dimension
                if (strlen((char*) att) > 0) str[n - 1].dimid = atoi((char*) att);
                IDAM_LOGF(LOG_DEBUG, "To Dimension  : %d\n", str[n - 1].dimid);
                xmlFree(att);
            }

// Child Tags

            parseTargetString(doc, cur, "label", str[n - 1].dimdocumentation.label);
            parseTargetString(doc, cur, "units", str[n - 1].dimdocumentation.units);

        }
        cur = cur->next;
    }
    document->ndimensions = n;    // Number of Tags Found
    document->dimensions = str;    // Array of Composite Signal Actions on Dimensions
    return;
}

void parseDocumentation(xmlDocPtr doc, xmlNodePtr cur, ACTIONS* actions)
{

    xmlChar* att;    // General Input of tag attribute values

    ACTION* str = actions->action;
    int n = actions->nactions;        // Counter

    cur = cur->xmlChildrenNode;
    while (cur != NULL) {
        IDAM_LOGF(LOG_DEBUG, "parseDocumentation: %s\n", (char*) cur->name);
        if ((!xmlStrcmp(cur->name, (const xmlChar*) "documentation"))) {
            n++;
            str = (ACTION*) realloc((void*) str, n * sizeof(ACTION));

            initAction(&str[n - 1]);
            str[n - 1].actionType = DOCUMENTATIONTYPE;
            initDocumentation(&str[n - 1].documentation);

// Attributes

            if ((att = xmlGetProp(cur, (xmlChar*) "id")) != NULL) {
                if (strlen((char*) att) > 0) str[n - 1].actionId = atoi((char*) att);
                IDAM_LOGF(LOG_DEBUG, "Action ID: %d\n", str[n - 1].actionId);
                xmlFree(att);
            }

            if ((att = xmlGetProp(cur, (xmlChar*) "exp_number_start")) != NULL) {
                if (strlen((char*) att) > 0) str[n - 1].exp_range[0] = atoi((char*) att);
                IDAM_LOGF(LOG_DEBUG, "Exp Number Range Start: %d\n", str[n - 1].exp_range[0]);
                xmlFree(att);
            }

            if ((att = xmlGetProp(cur, (xmlChar*) "exp_number_end")) != NULL) {
                if (strlen((char*) att) > 0) str[n - 1].exp_range[1] = atoi((char*) att);
                IDAM_LOGF(LOG_DEBUG, "Exp Number Range End : %d\n", str[n - 1].exp_range[1]);
                xmlFree(att);
            }

            if ((att = xmlGetProp(cur, (xmlChar*) "pass_start")) != NULL) {
                if (strlen((char*) att) > 0) str[n - 1].pass_range[0] = atoi((char*) att);
                IDAM_LOGF(LOG_DEBUG, "Pass Number Range Start: %d\n", str[n - 1].pass_range[0]);
                xmlFree(att);
            }

            if ((att = xmlGetProp(cur, (xmlChar*) "pass_end")) != NULL) {
                if (strlen((char*) att) > 0) str[n - 1].pass_range[1] = atoi((char*) att);
                IDAM_LOGF(LOG_DEBUG, "Pass Number Range End  : %d\n", str[n - 1].pass_range[1]);
                xmlFree(att);
            }

// Child Tags

            parseTargetString(doc, cur, "description", str[n - 1].documentation.description);
            parseTargetString(doc, cur, "label", str[n - 1].documentation.label);
            parseTargetString(doc, cur, "units", str[n - 1].documentation.units);

            parseDimDocumentation(doc, cur, &str[n - 1].documentation);

        }
        cur = cur->next;
    }
    actions->nactions = n;    // Number of Tags Found
    actions->action = str;    // Array of Actions bounded by a Ranges
    return;
}

void parseDimCalibration(xmlDocPtr doc, xmlNodePtr cur, CALIBRATION* cal)
{

    xmlChar* att;    // General Input of tag attribute values

    DIMENSION* str = cal->dimensions;
    int n = 0;                // Counter

    cur = cur->xmlChildrenNode;
    while (cur != NULL) {
        IDAM_LOGF(LOG_DEBUG, "parseDimCalibration: %s\n", (char*) cur->name);
        if ((!xmlStrcmp(cur->name, (const xmlChar*) "dimension"))) {
            n++;
            str = (DIMENSION*) realloc((void*) str, n * sizeof(DIMENSION));

            initDimension(&str[n - 1]);
            str[n - 1].dimType = DIMCALIBRATIONTYPE;
            initDimCalibration(&str[n - 1].dimcalibration);

// Attributes

            if ((att = xmlGetProp(cur, (xmlChar*) "dimid")) != NULL) {            // Target Dimension
                if (strlen((char*) att) > 0) str[n - 1].dimid = atoi((char*) att);
                IDAM_LOGF(LOG_DEBUG, "To Dimension  : %d\n", str[n - 1].dimid);
                xmlFree(att);
            }

            if ((att = xmlGetProp(cur, (xmlChar*) "invert")) != NULL) {
                if (att[0] == 'y' || att[0] == 'Y') str[n - 1].dimcalibration.invert = 1;
                IDAM_LOGF(LOG_DEBUG, "Calibration Invert: %d\n", str[n - 1].dimcalibration.invert);
                xmlFree(att);
            }

// Child Tags

            parseTargetString(doc, cur, "units", str[n - 1].dimcalibration.units);
            parseTargetValue(doc, cur, "factor", &str[n - 1].dimcalibration.factor);
            parseTargetValue(doc, cur, "offset", &str[n - 1].dimcalibration.offset);

            IDAM_LOGF(LOG_DEBUG, "Dimension Units               : %s\n", str[n - 1].dimcalibration.units);
            IDAM_LOGF(LOG_DEBUG, "Dimension Calibration Factor  : %f\n", str[n - 1].dimcalibration.factor);
            IDAM_LOGF(LOG_DEBUG, "Dimension Calibration Offset  : %f\n", str[n - 1].dimcalibration.offset);
        }
        cur = cur->next;
    }
    cal->ndimensions = n;    // Number of Tags Found
    cal->dimensions = str;    // Array of Composite Signal Actions on Dimensions
    return;
}


void parseCalibration(xmlDocPtr doc, xmlNodePtr cur, ACTIONS* actions)
{

    xmlChar* att;    // General Input of tag attribute values

    ACTION* str = actions->action;
    int n = actions->nactions;        // Counter

    cur = cur->xmlChildrenNode;
    while (cur != NULL) {
        IDAM_LOGF(LOG_DEBUG, "parseCalibration: %s\n", (char*) cur->name);
        if ((!xmlStrcmp(cur->name, (const xmlChar*) "calibration"))) {
            n++;
            str = (ACTION*) realloc((void*) str, n * sizeof(ACTION));

            initAction(&str[n - 1]);
            str[n - 1].actionType = CALIBRATIONTYPE;
            initCalibration(&str[n - 1].calibration);

// Attributes

            if ((att = xmlGetProp(cur, (xmlChar*) "id")) != NULL) {
                if (strlen((char*) att) > 0) str[n - 1].actionId = atoi((char*) att);
                IDAM_LOGF(LOG_DEBUG, "Action ID: %d\n", str[n - 1].actionId);
                xmlFree(att);
            }

            if ((att = xmlGetProp(cur, (xmlChar*) "exp_number_start")) != NULL) {
                if (strlen((char*) att) > 0) str[n - 1].exp_range[0] = atoi((char*) att);
                IDAM_LOGF(LOG_DEBUG, "Exp Number Range Start: %d\n", str[n - 1].exp_range[0]);
                xmlFree(att);
            }

            if ((att = xmlGetProp(cur, (xmlChar*) "exp_number_end")) != NULL) {
                if (strlen((char*) att) > 0) str[n - 1].exp_range[1] = atoi((char*) att);
                IDAM_LOGF(LOG_DEBUG, "Exp Number Range End : %d\n", str[n - 1].exp_range[1]);
                xmlFree(att);
            }

            if ((att = xmlGetProp(cur, (xmlChar*) "pass_start")) != NULL) {
                if (strlen((char*) att) > 0) str[n - 1].pass_range[0] = atoi((char*) att);
                IDAM_LOGF(LOG_DEBUG, "Pass Number Range Start: %d\n", str[n - 1].pass_range[0]);
                xmlFree(att);
            }

            if ((att = xmlGetProp(cur, (xmlChar*) "pass_end")) != NULL) {
                if (strlen((char*) att) > 0) str[n - 1].pass_range[1] = atoi((char*) att);
                IDAM_LOGF(LOG_DEBUG, "Pass Number Range End  : %d\n", str[n - 1].pass_range[1]);
                xmlFree(att);
            }

            if ((att = xmlGetProp(cur, (xmlChar*) "target")) != NULL) {
                if (strlen((char*) att) > 0) strcpy(str[n - 1].calibration.target, (char*) att);
                IDAM_LOGF(LOG_DEBUG, "Calibration Target: %s\n", str[n - 1].calibration.target);
                xmlFree(att);
            }

            if ((att = xmlGetProp(cur, (xmlChar*) "invert")) != NULL) {
                if (att[0] == 'y' || att[0] == 'Y') str[n - 1].calibration.invert = 1;
                IDAM_LOGF(LOG_DEBUG, "Calibration Invert: %d\n", str[n - 1].calibration.invert);
                xmlFree(att);
            }

// Child Tags

            parseTargetString(doc, cur, "units", str[n - 1].calibration.units);
            parseTargetValue(doc, cur, "factor", &str[n - 1].calibration.factor);
            parseTargetValue(doc, cur, "offset", &str[n - 1].calibration.offset);

            IDAM_LOGF(LOG_DEBUG, "Data Units               : %s\n", str[n - 1].calibration.units);
            IDAM_LOGF(LOG_DEBUG, "Data Calibration Factor  : %f\n", str[n - 1].calibration.factor);
            IDAM_LOGF(LOG_DEBUG, "Data Calibration Offset  : %f\n", str[n - 1].calibration.offset);

            parseDimCalibration(doc, cur, &str[n - 1].calibration);

        }
        cur = cur->next;
    }
    actions->nactions = n;    // Number of Tags Found
    actions->action = str;    // Array of Actions bounded by a Ranges
    return;
}


void parseSubset(xmlDocPtr doc, xmlNodePtr cur, ACTIONS* actions)
{

    xmlChar* att;    // General Input of tag attribute values

    ACTION* str = actions->action;
    //COMPOSITE *comp = NULL;
    SUBSET* sub = NULL;
    int n = actions->nactions;        // Counter
    int i, n0, n1;


    cur = cur->xmlChildrenNode;
    while (cur != NULL) {
        IDAM_LOGF(LOG_DEBUG, "parseSubset: %s\n", (char*) cur->name);
        if ((!xmlStrcmp(cur->name, (const xmlChar*) "subset"))) {
            n++;
            str = (ACTION*) realloc((void*) str, n * sizeof(ACTION));

            initAction(&str[n - 1]);
            str[n - 1].actionType = SUBSETTYPE;
            sub = &str[n - 1].subset;
            initSubset(sub);

// Attributes

            if ((att = xmlGetProp(cur, (xmlChar*) "id")) != NULL) {
                if (strlen((char*) att) > 0) str[n - 1].actionId = atoi((char*) att);
                IDAM_LOGF(LOG_DEBUG, "Action ID: %d\n", str[n - 1].actionId);
                xmlFree(att);
            }

            if ((att = xmlGetProp(cur, (xmlChar*) "data")) != NULL) {
                if (strlen((char*) att) > 0) strcpy(sub->data_signal, (char*) att);
                IDAM_LOGF(LOG_DEBUG, "Data Signal  : %s\n", sub->data_signal);
                xmlFree(att);
            }

// Child Tags

// Attributes

            if ((att = xmlGetProp(cur, (xmlChar*) "reform")) != NULL) {
                if (att[0] == 'Y' || att[0] == 'y') sub->reform = 1;
                xmlFree(att);
            }

            if ((att = xmlGetProp(cur, (xmlChar*) "member")) != NULL) {
                if (strlen((char*) att) > 0) strcpy(sub->member, (char*) att);
                IDAM_LOGF(LOG_DEBUG, "Subset Member: %s\n", sub->member);
                xmlFree(att);
            }

            if ((att = xmlGetProp(cur, (xmlChar*) "function")) != NULL) {
                if (strlen((char*) att) > 0) strcpy(sub->function, (char*) att);
                IDAM_LOGF(LOG_DEBUG, "Subset function: %s\n", sub->function);
                xmlFree(att);
            }

            if ((att = xmlGetProp(cur, (xmlChar*) "order")) != NULL) {
                if (strlen((char*) att) > 0) sub->order = atoi((char*) att);
                IDAM_LOGF(LOG_DEBUG, "Subset order: %d\n", sub->order);
                xmlFree(att);
            }

// Fixed Length Attribute Arrays

            parseFixedLengthStrArray(cur, "operation", &sub->operation[0], &sub->nbound);
            for (i = 0; i < sub->nbound; i++)
                sub->dimid[i] = i;                    // Ordering is as DATA[4][3][2][1][0]

            parseFixedLengthArray(cur, "bound", (void*) sub->bound, TYPE_DOUBLE, &n0);
            parseFixedLengthArray(cur, "dimid", (void*) sub->dimid, TYPE_INT, &n1);

            if (idamParseOperation(sub) != 0) return;

            for (i = 0; i < sub->nbound; i++) {
                IDAM_LOGF(LOG_DEBUG, "Dimension ID               : %d\n", sub->dimid[i]);
                IDAM_LOGF(LOG_DEBUG, "Subsetting Bounding Values : %e\n", sub->bound[i]);
                IDAM_LOGF(LOG_DEBUG, "Subsetting Operation       : %s\n", sub->operation[i]);
                IDAM_LOGF(LOG_DEBUG, "Subsetting Is Index?       : %d\n", sub->isindex[i]);
                IDAM_LOGF(LOG_DEBUG, "Subsetting Lower Index     : %d\n", (int) sub->lbindex[i]);
                IDAM_LOGF(LOG_DEBUG, "Subsetting Upper Index     : %d\n", (int) sub->ubindex[i]);
            }
        }
        cur = cur->next;
    }

    actions->nactions = n;    // Number of Tags Found
    actions->action = str;    // Array of Actions bounded by a Ranges
    return;
}

void parseMap(xmlDocPtr doc, xmlNodePtr cur, ACTIONS* actions)
{
    return;
}


int parseDoc(char* docname, ACTIONS* actions)
{

    xmlDocPtr doc;
    xmlNodePtr cur;

#ifdef TIMETEST
    struct timeval tv_start;
    struct timeval tv_end;
    float testtime ;
    int rc = gettimeofday(&tv_start, NULL);
#endif

    xmlInitParser();

    if ((doc = xmlParseDoc((xmlChar*) docname)) == NULL) {
        xmlFreeDoc(doc);
        xmlCleanupParser();
        addIdamError(&idamerrorstack, CODEERRORTYPE, "parseDoc", 1, "XML Not Parsed");
        return 1;
    }

    if ((cur = xmlDocGetRootElement(doc)) == NULL) {
        addIdamError(&idamerrorstack, CODEERRORTYPE, "parseDoc", 1, "Empty XML Document");
        xmlFreeDoc(doc);
        xmlCleanupParser();
        return 1;
    }

    if (xmlStrcmp(cur->name, (const xmlChar*) "action")) {        //If No Action Tag then Nothing to be done!
        xmlFreeDoc(doc);
        xmlCleanupParser();
        return 1;
    }

    cur = cur->xmlChildrenNode;
    while (cur != NULL) {

        if ((!xmlStrcmp(cur->name, (const xmlChar*) "signal"))) {

            parseComposite(doc, cur, actions);        // Composite can have SUBSET as a child
            parseDocumentation(doc, cur, actions);
            parseCalibration(doc, cur, actions);
            parseTimeOffset(doc, cur, actions);
            parseErrorModel(doc, cur, actions);

            parseSubset(doc, cur, actions);        // Single Subset
            //parseMap(doc, cur, actions);			// Single map

            //parseServerSide(doc, cur, actions);		// Multiple Subsets and mappings

        }
        cur = cur->next;
    }

    xmlFreeDoc(doc);
    xmlCleanupParser();

#ifdef TIMETEST
    rc = gettimeofday(&tv_end, NULL);
    testtime = (float)(tv_end.tv_sec-tv_start.tv_sec)*1.0E6 + (float)(tv_end.tv_usec - tv_start.tv_usec) ;
#ifdef debugon
    IDAM_LOGF(LOG_DEBUG, "XML Parse Timing: %.2f(microsecs)\n", testtime);
#endif
#endif

    return 0;
}


#endif
//==================================================================================================

void printDimensions(int ndim, DIMENSION* dims)
{
    int i, j;
    IDAM_LOGF(LOG_DEBUG, "No. Dimensions     : %d\n", ndim);
    for (i = 0; i < ndim; i++) {

        IDAM_LOGF(LOG_DEBUG, "Dim id     : %d\n", dims[i].dimid);

        switch (dims[i].dimType) {

            case DIMCALIBRATIONTYPE :
                IDAM_LOGF(LOG_DEBUG, "factor     : %.12f\n", dims[i].dimcalibration.factor);
                IDAM_LOGF(LOG_DEBUG, "Offset     : %.12f\n", dims[i].dimcalibration.offset);
                IDAM_LOGF(LOG_DEBUG, "Units      : %s\n", dims[i].dimcalibration.units);
                break;

            case DIMCOMPOSITETYPE :
                IDAM_LOGF(LOG_DEBUG, "to Dim       : %d\n", dims[i].dimcomposite.to_dim);
                IDAM_LOGF(LOG_DEBUG, "from Dim     : %d\n", dims[i].dimcomposite.from_dim);
                IDAM_LOGF(LOG_DEBUG, "Dim signal   : %s\n", dims[i].dimcomposite.dim_signal);
                IDAM_LOGF(LOG_DEBUG, "Dim Error    : %s\n", dims[i].dimcomposite.dim_error);
                IDAM_LOGF(LOG_DEBUG, "Dim ASError  : %s\n", dims[i].dimcomposite.dim_aserror);
                IDAM_LOGF(LOG_DEBUG, "Dim Source File  : %s\n", dims[i].dimcomposite.file);
                IDAM_LOGF(LOG_DEBUG, "Dim Source Format: %s\n", dims[i].dimcomposite.format);

                break;

            case DIMDOCUMENTATIONTYPE :
                IDAM_LOGF(LOG_DEBUG, "Dim Label  : %s\n", dims[i].dimdocumentation.label);
                IDAM_LOGF(LOG_DEBUG, "Dim Units  : %s\n", dims[i].dimdocumentation.units);
                break;

            case DIMERRORMODELTYPE :
                IDAM_LOGF(LOG_DEBUG, "Error Model Id            : %d\n", dims[i].dimerrormodel.model);
                IDAM_LOGF(LOG_DEBUG, "Number of Model Parameters: %d\n", dims[i].dimerrormodel.param_n);
                for (j = 0; j < dims[i].dimerrormodel.param_n; j++)
                    IDAM_LOGF(LOG_DEBUG, "Parameters[%d] = %.12f\n", j, dims[i].dimerrormodel.params[j]);
                break;

            default:
                break;
        }

    }
}

void printAction(ACTION action)
{
    int i, j;
    IDAM_LOGF(LOG_DEBUG, "Action XML Id    : %d\n", action.actionId);
    IDAM_LOGF(LOG_DEBUG, "Action Type      : %d\n", action.actionType);
    IDAM_LOGF(LOG_DEBUG, "In Range?        : %d\n", action.inRange);
    IDAM_LOGF(LOG_DEBUG, "Exp Number Range : %d -> %d\n", action.exp_range[0], action.exp_range[1]);
    IDAM_LOGF(LOG_DEBUG, "Pass Number Range: %d -> %d\n", action.pass_range[0], action.pass_range[1]);

    switch (action.actionType) {
        case (TIMEOFFSETTYPE):
            IDAM_LOG(LOG_DEBUG, "TIMEOFFSET xml\n");
            IDAM_LOGF(LOG_DEBUG, "Method         : %d\n", action.timeoffset.method);
            IDAM_LOGF(LOG_DEBUG, "Time Offset    : %.12f\n", action.timeoffset.offset);
            IDAM_LOGF(LOG_DEBUG, "Time Interval  : %.12f\n", action.timeoffset.interval);
            break;
        case (DOCUMENTATIONTYPE):
            IDAM_LOG(LOG_DEBUG, "DOCUMENTATION xml\n");
            IDAM_LOGF(LOG_DEBUG, "Description: %s\n", action.documentation.description);
            IDAM_LOGF(LOG_DEBUG, "Data Label : %s\n", action.documentation.label);
            IDAM_LOGF(LOG_DEBUG, "Data Units : %s\n", action.documentation.units);
            printDimensions(action.documentation.ndimensions, action.documentation.dimensions);
            break;
        case (CALIBRATIONTYPE):
            IDAM_LOG(LOG_DEBUG, "CALIBRATION xml\n");
            IDAM_LOGF(LOG_DEBUG, "Target     : %s\n", action.calibration.target);
            IDAM_LOGF(LOG_DEBUG, "Factor     : %f\n", action.calibration.factor);
            IDAM_LOGF(LOG_DEBUG, "Offset     : %f\n", action.calibration.offset);
            IDAM_LOGF(LOG_DEBUG, "Invert     : %d\n", action.calibration.invert);
            IDAM_LOGF(LOG_DEBUG, "Data Units : %s\n", action.calibration.units);
            printDimensions(action.calibration.ndimensions, action.calibration.dimensions);
            break;
        case (COMPOSITETYPE):
            IDAM_LOG(LOG_DEBUG, "COMPOSITE xml\n");
            IDAM_LOGF(LOG_DEBUG, "Composite Data Signal    : %s\n", action.composite.data_signal);
            IDAM_LOGF(LOG_DEBUG, "Composite Error Signal   : %s\n", action.composite.error_signal);
            IDAM_LOGF(LOG_DEBUG, "Composite Asymmetric Error Signal   : %s\n", action.composite.aserror_signal);
            IDAM_LOGF(LOG_DEBUG, "Composite Map to Signal  : %s\n", action.composite.map_to_signal);
            IDAM_LOGF(LOG_DEBUG, "Composite Source File    : %s\n", action.composite.file);
            IDAM_LOGF(LOG_DEBUG, "Composite Source Format  : %s\n", action.composite.format);
            IDAM_LOGF(LOG_DEBUG, "Composite Time Dimension : %d\n", action.composite.order);
            printDimensions(action.composite.ndimensions, action.composite.dimensions);
            break;
        case (ERRORMODELTYPE):
            IDAM_LOG(LOG_DEBUG, "ERRORMODEL xml\n");
            IDAM_LOGF(LOG_DEBUG, "Error Model Id            : %d\n", action.errormodel.model);
            IDAM_LOGF(LOG_DEBUG, "Number of Model Parameters: %d\n", action.errormodel.param_n);
            for (i = 0; i < action.errormodel.param_n; i++)
                IDAM_LOGF(LOG_DEBUG, "Parameters[%d] = %.12f\n", i, action.errormodel.params[i]);
            printDimensions(action.errormodel.ndimensions, action.errormodel.dimensions);
            break;

        case (SERVERSIDETYPE):
            IDAM_LOG(LOG_DEBUG, "SERVERSIDE Actions\n");
            IDAM_LOGF(LOG_DEBUG, "Number of Serverside Subsets: %d\n", action.serverside.nsubsets);
            for (i = 0; i < action.serverside.nsubsets; i++) {
                IDAM_LOGF(LOG_DEBUG, "Number of Subsetting Operations: %d\n", action.serverside.subsets[i].nbound);
                IDAM_LOGF(LOG_DEBUG, "Reform?                        : %d\n", action.serverside.subsets[i].reform);
                IDAM_LOGF(LOG_DEBUG, "Member                         : %s\n", action.serverside.subsets[i].member);
                IDAM_LOGF(LOG_DEBUG, "Function                       : %s\n", action.serverside.subsets[i].function);
                IDAM_LOGF(LOG_DEBUG, "Order                          : %d\n", action.serverside.subsets[i].order);
                IDAM_LOGF(LOG_DEBUG, "Signal                         : %s\n", action.serverside.subsets[i].data_signal);
                for (j = 0; j < action.serverside.subsets[i].nbound; j++) {
                    IDAM_LOGF(LOG_DEBUG, "Bounding Value: %e\n", action.serverside.subsets[i].bound[j]);
                    IDAM_LOGF(LOG_DEBUG, "Operation     : %s\n", action.serverside.subsets[i].operation[j]);
                    IDAM_LOGF(LOG_DEBUG, "Dimension ID  : %d\n", action.serverside.subsets[i].dimid[j]);
                }
            }
            IDAM_LOGF(LOG_DEBUG, "Number of Serverside mappings: %d\n", action.serverside.nmaps);
            break;
        case (SUBSETTYPE):
            IDAM_LOG(LOG_DEBUG, "SUBSET Actions\n");
            IDAM_LOG(LOG_DEBUG, "Number of Subsets: 1\n");
            IDAM_LOGF(LOG_DEBUG, "Number of Subsetting Operations: %d\n", action.subset.nbound);
            IDAM_LOGF(LOG_DEBUG, "Reform?                        : %d\n", action.subset.reform);
            IDAM_LOGF(LOG_DEBUG, "Member                         : %s\n", action.subset.member);
            IDAM_LOGF(LOG_DEBUG, "Function                       : %s\n", action.subset.function);
            IDAM_LOGF(LOG_DEBUG, "Order                       : %d\n", action.subset.order);
            IDAM_LOGF(LOG_DEBUG, "Signal                         : %s\n", action.subset.data_signal);
            for (j = 0; j < action.subset.nbound; j++) {
                IDAM_LOGF(LOG_DEBUG, "Bounding Value: %e\n", action.subset.bound[j]);
                IDAM_LOGF(LOG_DEBUG, "Operation     : %s\n", action.subset.operation[j]);
                IDAM_LOGF(LOG_DEBUG, "Dimension ID  : %d\n", action.subset.dimid[j]);
            }
            break;
        default:
            break;
    }

}

void printActions(ACTIONS actions)
{
    int i;
    IDAM_LOGF(LOG_DEBUG, "No. Action Blocks: %d\n", actions.nactions);
    for (i = 0; i < actions.nactions; i++) {
        IDAM_LOGF(LOG_DEBUG, "\n\n# %d\n", i);
        printAction(actions.action[i]);
    }
    IDAM_LOG(LOG_DEBUG, "\n\n");
}


// Initialise an Action Structure and Child Structures

void initDimCalibration(DIMCALIBRATION* act)
{
    act->factor = (double) 1.0E0;    // Data Calibration Correction/Scaling factor
    act->offset = (double) 0.0E0;    // Data Calibration Correction/Scaling offset
    act->invert = 0;            // Don't Invert the data
    act->units[0] = '\0';
}

void initDimComposite(DIMCOMPOSITE* act)
{
    act->to_dim = -1;                // Swap to Dimension ID
    act->from_dim = -1;                // Swap from Dimension ID
    act->file[0] = '\0';            // Data Source File (with Full Path)
    act->format[0] = '\0';            // Data Source File's Format
    act->dim_signal[0] = '\0';            // Source Signal
    act->dim_error[0] = '\0';            // Error Source Signal
    act->dim_aserror[0] = '\0';            // Asymmetric Error Source Signal
}

void initDimDocumentation(DIMDOCUMENTATION* act)
{
    act->label[0] = '\0';
    act->units[0] = '\0';            // Lower in priority than Calibration Units
}

void initDimErrorModel(DIMERRORMODEL* act)
{
    int i;
    act->model = ERROR_MODEL_UNKNOWN;    // No Error Model
    act->param_n = 0;            // No. Model parameters
    for (i = 0; i < MAXERRPARAMS; i++)act->params[i] = 0.0;
}


void initDimension(DIMENSION* act)
{
    act->dimid = -1;        // Dimension Id
    act->dimType = 0;        // Structure Type
}

void initTimeOffset(TIMEOFFSET* act)
{
    act->method = 0;            // Correction Method: Standard offset correction only
    act->offset = (double) 0.0E0;    // Time Dimension offset correction or start time
    act->interval = (double) 0.0E0;    // Time Dimension Interval correction
}

void initCalibration(CALIBRATION* act)
{
    act->factor = (double) 1.0E0;    // Data Calibration Correction/Scaling factor
    act->offset = (double) 0.0E0;    // Data Calibration Correction/Scaling offset
    act->units[0] = '\0';
    act->target[0] = '\0';        // Which data Component to apply calibration? (all, data, error, aserror)
    act->invert = 0;        // No Inversion
    act->ndimensions = 0;
    act->dimensions = NULL;
}

void initDocumentation(DOCUMENTATION* act)
{
    act->label[0] = '\0';
    act->units[0] = '\0';        // Lower in priority than Calibration Units
    act->description[0] = '\0';
    act->ndimensions = 0;
    act->dimensions = NULL;
}

void initComposite(COMPOSITE* act)
{
    act->data_signal[0] = '\0';            // Derived Data using this Data Source
    act->error_signal[0] = '\0';            // Use Errors from this Source
    act->aserror_signal[0] = '\0';            // Use Asymmetric Errors from this Source
    act->map_to_signal[0] = '\0';            // Straight swap of data: map to this signal
    act->file[0] = '\0';            // Data Source File (with Full Path)
    act->format[0] = '\0';            // Data Source File's Format
    act->order = -1;            // Identify the Time Dimension
    act->ndimensions = 0;
    act->nsubsets = 0;
    act->nmaps = 0;
    act->dimensions = NULL;
    act->subsets = NULL;
    act->maps = NULL;
}

void initServerside(SERVERSIDE* act)
{
    act->nsubsets = 0;
    act->nmaps = 0;
    act->subsets = NULL;
    act->maps = NULL;
}

void initErrorModel(ERRORMODEL* act)
{
    int i;
    act->model = ERROR_MODEL_UNKNOWN;    // No Error Model
    act->param_n = 0;            // No. Model parameters
    for (i = 0; i < MAXERRPARAMS; i++)act->params[i] = 0.0;
    act->ndimensions = 0;
    act->dimensions = NULL;
}

void initSubset(SUBSET* act)
{
    int i;
    for (i = 0; i < MAXDATARANK; i++) {
        act->bound[i] = 0.0;                // Subsetting Float Bounds
        act->ubindex[i] = 0;                // Subsetting Integer Bounds (Upper Index)
        act->lbindex[i] = 0;                // Lower Index
        act->isindex[i] = 0;                // Flag the Bound is an Integer Type
        act->dimid[i] = -1;                // Dimension IDs
        act->operation[i][0] = '\0';            // Subsetting Operations
    }
    act->data_signal[0] = '\0';                // Data to Read
    act->member[0] = '\0';                // Structure Member to target
    act->function[0] = '\0';                // Name of simple function to apply
    act->nbound = 0;                // The number of Subsetting Operations
    act->reform = 0;                // reduce the Rank if a subsetted dimension has length 1
    act->order = -1;                // Explicitly set the order of the time dimension if >= 0
}

void initMap(MAP* act)
{
    int i;
    for (i = 0; i < MAXDATARANK; i++) {
        act->value[i] = 0.0;
        act->dimid[i] = -1;                // Dimension IDs
        act->mapping[i][0] = '\0';            // Mapping Operations
    }
    act->data_signal[0] = '\0';                // Data
    act->nmap = 0;                // The number of Mapping Operations
}


// Initialise an Action Structure

void initAction(ACTION* act)
{
    act->actionType = 0;        // Action Range Type
    act->inRange = 0;        // Is this Action Record Applicable to the Current data Request?
    act->actionId = 0;        // Action XML Tag Id
    act->exp_range[0] = 0;        // Applicable over this Exp. Number Range
    act->exp_range[1] = 0;
    act->pass_range[0] = -1;        // Applicable over this Pass Number Range
    act->pass_range[1] = -1;
}


// Initialise an Action Array Structure

void initActions(ACTIONS* act)
{
    act->nactions = 0;        // Number of Action blocks
    act->action = NULL;    // Array of Action blocks
}

void freeActions(ACTIONS* actions)
{

// Free Heap Memory From ACTION Structures

    int i, j;
    void* cptr;

    IDAM_LOG(LOG_DEBUG, "freeActions: Enter\n");
    IDAM_LOGF(LOG_DEBUG, "freeDataBlock: Number of Actions = %d \n", actions->nactions);

    if (actions->nactions == 0) return;

    for (i = 0; i < actions->nactions; i++) {
        IDAM_LOGF(LOG_DEBUG, "freeDataBlock: freeing action Type = %d \n", actions->action[i].actionType);

        switch (actions->action[i].actionType) {

            case COMPOSITETYPE:
                if ((cptr = (void*) actions->action[i].composite.dimensions) != NULL) {
                    free(cptr);
                    actions->action[i].composite.dimensions = NULL;
                    actions->action[i].composite.ndimensions = 0;
                }
                if (actions->action[i].composite.nsubsets > 0) {
                    if ((cptr = (void*) actions->action[i].composite.subsets) != NULL) free(cptr);
                    actions->action[i].composite.subsets = NULL;
                    actions->action[i].composite.nsubsets = 0;
                }
                if (actions->action[i].composite.nmaps > 0) {
                    if ((cptr = (void*) actions->action[i].composite.maps) != NULL) free(cptr);
                    actions->action[i].composite.maps = NULL;
                    actions->action[i].composite.nmaps = 0;
                }
                break;

            case ERRORMODELTYPE:
                actions->action[i].errormodel.param_n = 0;

                for (j = 0; j < actions->action[i].errormodel.ndimensions; j++)
                    if ((cptr = (void*) actions->action[i].errormodel.dimensions) != NULL) {
                        free(cptr);
                        actions->action[i].errormodel.dimensions = NULL;
                        actions->action[i].errormodel.ndimensions = 0;
                    }

                break;

            case CALIBRATIONTYPE:
                if ((cptr = (void*) actions->action[i].calibration.dimensions) != NULL) {
                    free(cptr);
                    actions->action[i].calibration.dimensions = NULL;
                    actions->action[i].calibration.ndimensions = 0;
                }
                break;

            case DOCUMENTATIONTYPE:
                if ((cptr = (void*) actions->action[i].documentation.dimensions) != NULL) {
                    free(cptr);
                    actions->action[i].documentation.dimensions = NULL;
                    actions->action[i].documentation.ndimensions = 0;
                }
                break;

            case SERVERSIDETYPE:
                if (actions->action[i].serverside.nsubsets > 0) {
                    if ((cptr = (void*) actions->action[i].serverside.subsets) != NULL) free(cptr);
                    actions->action[i].serverside.subsets = NULL;
                    actions->action[i].serverside.nsubsets = 0;
                }
                if (actions->action[i].serverside.nmaps > 0) {
                    if ((cptr = (void*) actions->action[i].serverside.maps) != NULL) free(cptr);
                    actions->action[i].serverside.maps = NULL;
                    actions->action[i].serverside.nmaps = 0;
                }
                break;

            default:
                break;
        }
    }

    if ((cptr = (void*) actions->action) != NULL) free(cptr);
    actions->nactions = 0;
    actions->action = NULL;

    IDAM_LOG(LOG_DEBUG, "freeActions: Exit\n");
}

// Copy an Action Structure and Drop Pointers to ACTION & DIMENSION Structures (ensures a single Heap free later)

void copyActions(ACTIONS* actions_out, ACTIONS* actions_in)
{
    *actions_out = *actions_in;
    actions_in->action = NULL;
    actions_in->nactions = 0;
}

