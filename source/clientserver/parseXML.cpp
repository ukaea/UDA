/*---------------------------------------------------------------
* UDA XML Parser
*
* Input Arguments:    char *xml
*
* Returns:        parseXML    TRUE if parse was successful

*            ACTIONS        Actions Structure
*
* Calls
*
* Notes:
*
* ToDo:
*-------------------------------------------------------------------------------------------*/

#include "parseXML.h"

#include <cstdlib>
#include <sstream>

#include <logging/logging.h>
#ifndef NOXMLPARSER

#include <clientserver/udaTypes.h>
#include <clientserver/stringUtils.h>
#include <clientserver/parseOperation.h>
#include <clientserver/errorLog.h>

static double deScale(char* scale);
static void parse_target_value(xmlDocPtr doc, xmlNodePtr cur, const char* target, double* value);
static void parse_target_string(xmlDocPtr doc, xmlNodePtr cur, const char* target, char* str);
static void parse_fixed_length_array(xmlNodePtr cur, const char* target, void* array, int arraytype, int* n);
static void parse_documentation(xmlDocPtr doc, xmlNodePtr cur, ACTIONS* actions);
static void parse_calibration(xmlDocPtr doc, xmlNodePtr cur, ACTIONS* actions);
static void parse_time_offset(xmlDocPtr doc, xmlNodePtr cur, ACTIONS* actions);
static void parse_error_model(xmlDocPtr doc, xmlNodePtr cur, ACTIONS* actions);
static void parse_subset(xmlDocPtr doc, xmlNodePtr cur, ACTIONS* actions);
static void print_dimensions(int ndim, DIMENSION* dims);
static void init_dim_calibration(DIMCALIBRATION* act);
static void init_dim_composite(DIMCOMPOSITE* act);
static void init_dim_documentation(DIMDOCUMENTATION* act);
static void init_dim_error_model(DIMERRORMODEL* act);
static void init_dimension(DIMENSION* act);
static void init_time_offset(TIMEOFFSET* act);
static void init_calibration(CALIBRATION* act);
static void init_documentation(DOCUMENTATION* act);
static void init_composite(COMPOSITE* act);
static void init_error_model(ERRORMODEL* act);

// Simple Tags with Delimited List of Floating Point Values
// Assume No Attributes

static float* parse_float_array(xmlDocPtr doc, xmlNodePtr cur, const char* target, int* n);

float* parse_float_array(xmlDocPtr doc, xmlNodePtr cur, const char* target, int* n)
{
    xmlChar* key = nullptr;
    float* value = nullptr;
    *n = 0;
    const char* delim = " ";
    char* item;
    int nco = 0;

    cur = cur->xmlChildrenNode;
    while (cur != nullptr) {
        if ((!xmlStrcmp(cur->name, (const xmlChar*)target))) {
            key = xmlNodeListGetString(doc, cur->xmlChildrenNode, 1);
            convertNonPrintable((char*)key);
            if (strlen((char*)key) > 0) {
                int lkey = (int)strlen((char*)key);
                UDA_LOG(UDA_LOG_DEBUG, "parseFloatArray: [%d] %s %s \n", lkey, target, key);
                item = strtok((char*)key, delim);
                if (item != nullptr) {
                    nco++;
                    UDA_LOG(UDA_LOG_DEBUG, "parseFloatArray: [%d] %s \n", nco, item);
                    value = (float*)realloc((void*)value, nco * sizeof(float));
                    value[nco - 1] = atof(item);
                    UDA_LOG(UDA_LOG_DEBUG, "parseFloatArray: [%d] %s %f\n", nco, item, value[nco - 1]);
                    while ((item = strtok(nullptr, delim)) != nullptr && nco <= UDA_XML_MAX_LOOP) {
                        nco++;
                        value = (float*)realloc((void*)value, nco * sizeof(float));
                        value[nco - 1] = atof(item);
                        UDA_LOG(UDA_LOG_DEBUG, "parseFloatArray: [%d] %s %f\n", nco, item, value[nco - 1]);
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

template <typename T>
void parse_fixed_length_array(xmlNodePtr cur, const char* target, void* array, int* n)
{
    xmlChar* att = nullptr;
    *n = 0;
    const char* delim = ",";
    char* item;
    int nco = 0;

    if ((att = xmlGetProp(cur, (xmlChar*)target)) != nullptr) {
        convertNonPrintable((char*)att);
        if (strlen((char*)att) > 0) {
            int l = (int)strlen((char*)att);
            UDA_LOG(UDA_LOG_DEBUG, "parseFixedLengthArray: [%d] %s %s \n", l, target, att);
            item = strtok((char*)att, delim);
            if (item != nullptr) {
                nco++;
                auto p = (T*)array;
                std::stringstream ss{ item };
                ss >> p[nco - 1];

                while ((item = strtok(nullptr, delim)) != nullptr && nco <= MAXDATARANK) {
                    nco++;
                    ss = std::stringstream{ item };
                    ss >> p[nco - 1];
                }
            }
        }
        *n = nco;
        xmlFree(att);
    }
}

void parse_fixed_length_array(xmlNodePtr cur, const char* target, void* array, int arraytype, int* n)
{
    xmlChar* att = nullptr;
    *n = 0;
    const char* delim = ",";
    char* item;
    int nco = 0;

    if ((att = xmlGetProp(cur, (xmlChar*)target)) != nullptr) {
        convertNonPrintable((char*)att);
        if (strlen((char*)att) > 0) {
            int l = (int)strlen((char*)att);
            UDA_LOG(UDA_LOG_DEBUG, "parseFixedLengthArray: [%d] %s %s \n", l, target, att);
            item = strtok((char*)att, delim);
            if (item != nullptr) {
                nco++;
                switch (arraytype) {
                    case UDA_TYPE_FLOAT: {
                        auto fp = (float*)array;
                        fp[nco - 1] = atof(item);
                        break;
                    }
                    case UDA_TYPE_DOUBLE: {
                        auto dp = (double*)array;
                        dp[nco - 1] = strtod(item, nullptr);
                        break;
                    }
                    case UDA_TYPE_CHAR: {
                        auto p = (char*)array;
                        p[nco - 1] = (char)atoi(item);
                        break;
                    }
                    case UDA_TYPE_SHORT: {
                        auto p = (short*)array;
                        p[nco - 1] = (short)atoi(item);
                        break;
                    }
                    case UDA_TYPE_INT: {
                        auto ip = (int*)array;
                        ip[nco - 1] = (int)atoi(item);
                        break;
                    }
                    case UDA_TYPE_LONG: {
                        auto lp = (long*)array;
                        lp[nco - 1] = (long)atol(item);
                        break;
                    }
                    case UDA_TYPE_UNSIGNED_CHAR: {
                        auto p = (unsigned char*)array;
                        p[nco - 1] = (unsigned char)atoi(item);
                        break;
                    }
                    case UDA_TYPE_UNSIGNED_SHORT: {
                        auto p = (unsigned short*)array;
                        p[nco - 1] = (unsigned short)atoi(item);
                        break;
                    }
                    case UDA_TYPE_UNSIGNED_INT: {
                        auto p = (unsigned int*)array;
                        p[nco - 1] = (unsigned int)atoi(item);
                        break;
                    }
                    case UDA_TYPE_UNSIGNED_LONG: {
                        auto p = (unsigned long*)array;
                        p[nco - 1] = (unsigned long)atol(item);
                        break;
                    }
                    default:
                        return;
                }

                while ((item = strtok(nullptr, delim)) != nullptr && nco <= UDA_MAX_DATA_RANK) {
                    nco++;
                    switch (arraytype) {
                        case UDA_TYPE_FLOAT: {
                            auto fp = (float*)array;
                            fp[nco - 1] = atof(item);
                            break;
                        }
                        case UDA_TYPE_DOUBLE: {
                            auto dp = (double*)array;
                            dp[nco - 1] = strtod(item, nullptr);
                            break;
                        }
                        case UDA_TYPE_CHAR: {
                            auto p = (char*)array;
                            p[nco - 1] = (char)atoi(item);
                            break;
                        }
                        case UDA_TYPE_SHORT: {
                            auto p = (short*)array;
                            p[nco - 1] = (short)atoi(item);
                            break;
                        }
                        case UDA_TYPE_INT: {
                            auto ip = (int*)array;
                            ip[nco - 1] = (int)atoi(item);
                            break;
                        }
                        case UDA_TYPE_LONG: {
                            auto lp = (long*)array;
                            lp[nco - 1] = (long)atol(item);
                            break;
                        }
                        case UDA_TYPE_UNSIGNED_CHAR: {
                            auto p = (unsigned char*)array;
                            p[nco - 1] = (unsigned char)atoi(item);
                            break;
                        }
                        case UDA_TYPE_UNSIGNED_SHORT: {
                            auto p = (unsigned short*)array;
                            p[nco - 1] = (unsigned short)atoi(item);
                            break;
                        }
                        case UDA_TYPE_UNSIGNED_INT: {
                            auto p = (unsigned int*)array;
                            p[nco - 1] = (unsigned int)atoi(item);
                            break;
                        }
                        case UDA_TYPE_UNSIGNED_LONG: {
                            auto p = (unsigned long*)array;
                            p[nco - 1] = (unsigned long)atol(item);
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
}

void parse_fixed_length_str_array(xmlNodePtr cur, const char* target, char array[UDA_MAX_DATA_RANK][UDA_SXML_MAX_STRING], int* n)
{
    xmlChar* att = nullptr;
    *n = 0;
    const char* delim = ",";
    char* item;
    int nco = 0;

    if ((att = xmlGetProp(cur, (xmlChar*)target)) != nullptr) {
        if (strlen((char*)att) > 0) {
            int l = (int)strlen((char*)att);
            UDA_LOG(UDA_LOG_DEBUG, "parseFixedLengthStrArray: [%d] %s %s \n", l, target, att);
            item = strtok((char*)att, delim);
            if (item != nullptr) {
                nco++;
                if (strlen(item) < UDA_SXML_MAX_STRING) {
                    strcpy(array[nco - 1], item);
                } else {
                    strncpy(array[nco - 1], item, UDA_SXML_MAX_STRING - 1);
                    array[nco - 1][UDA_SXML_MAX_STRING - 1] = '\0';
                }

                while ((item = strtok(nullptr, delim)) != nullptr && nco <= UDA_MAX_DATA_RANK) {
                    nco++;
                    if (strlen(item) < UDA_SXML_MAX_STRING) {
                        strcpy(array[nco - 1], item);
                    } else {
                        strncpy(array[nco - 1], item, UDA_SXML_MAX_STRING - 1);
                        array[nco - 1][UDA_SXML_MAX_STRING - 1] = '\0';
                    }
                }
            }
        }
        *n = nco;
        xmlFree(att);
    }
}


// Decode Scale Value

double deScale(char* scale)
{
    if (strlen(scale) == 0) return 1.0E0;
    if (STR_EQUALS(scale, "milli")) {
        return 1.0E-3;
    } else {
        if (STR_EQUALS(scale, "micro")) {
            return 1.0E-6;
        } else {
            if (STR_EQUALS(scale, "nano")) {
                return 1.0E-9;
            }
        }
    }
    return 1.0E0;    // Default value
}

// Locate and extract Named Parameter (Numerical and String) Values
// Assume only 1 tag per document

void parse_target_value(xmlDocPtr doc, xmlNodePtr cur, const char* target, double* value)
{
    xmlChar* key = nullptr;
    xmlChar* scale = nullptr;
    cur = cur->xmlChildrenNode;
    while (cur != nullptr) {
        if ((!xmlStrcmp(cur->name, (const xmlChar*)target))) {
            scale = xmlGetProp(cur, (xmlChar*)"scale");
            key = xmlNodeListGetString(doc, cur->xmlChildrenNode, 1);
            if (key != nullptr) *value = (double)atof((char*)key);
            if (scale != nullptr) *value = *value * deScale((char*)scale);
            xmlFree(key);
            xmlFree(scale);
            break;
        }
        cur = cur->next;
    }
}

void parse_target_string(xmlDocPtr doc, xmlNodePtr cur, const char* target, char* str)
{
    xmlChar* key = nullptr;
    cur = cur->xmlChildrenNode;
    while (cur != nullptr) {
        if ((!xmlStrcmp(cur->name, (const xmlChar*)target))) {
            key = xmlNodeListGetString(doc, cur->xmlChildrenNode, 1);
            if (key != nullptr) strcpy(str, (char*)key);
            xmlFree(key);
            break;
        }
        cur = cur->next;
    }
}

void parse_time_offset(xmlDocPtr doc, xmlNodePtr cur, ACTIONS* actions)
{

    xmlChar* att;    // General Input of tag attribute values

    ACTION* str = actions->action;
    int n = actions->nactions;        // Counter

    cur = cur->xmlChildrenNode;
    while (cur != nullptr) {
        UDA_LOG(UDA_LOG_DEBUG, "%s\n", (char*)cur->name);
        if ((!xmlStrcmp(cur->name, (const xmlChar*)"time_offset"))) {
            n++;
            str = (ACTION*)realloc((void*)str, n * sizeof(ACTION));

            initAction(&str[n - 1]);
            str[n - 1].actionType = UDA_TIME_OFFSET_TYPE;
            init_time_offset(&str[n - 1].timeoffset);

            // Attributes

            if ((att = xmlGetProp(cur, (xmlChar*)"id")) != nullptr) {
                if (strlen((char*)att) > 0) str[n - 1].actionId = atoi((char*)att);
                UDA_LOG(UDA_LOG_DEBUG, "Action ID: %d\n", str[n - 1].actionId);
                xmlFree(att);
            }

            if ((att = xmlGetProp(cur, (xmlChar*)"exp_number_start")) != nullptr) {
                if (strlen((char*)att) > 0) str[n - 1].exp_range[0] = atoi((char*)att);
                UDA_LOG(UDA_LOG_DEBUG, "Exp Number Range Start: %d\n", str[n - 1].exp_range[0]);
                xmlFree(att);
            }

            if ((att = xmlGetProp(cur, (xmlChar*)"exp_number_end")) != nullptr) {
                if (strlen((char*)att) > 0) str[n - 1].exp_range[1] = atoi((char*)att);
                UDA_LOG(UDA_LOG_DEBUG, "Exp Number Range End : %d\n", str[n - 1].exp_range[1]);
                xmlFree(att);
            }

            if ((att = xmlGetProp(cur, (xmlChar*)"pass_start")) != nullptr) {
                if (strlen((char*)att) > 0) str[n - 1].pass_range[0] = atoi((char*)att);
                UDA_LOG(UDA_LOG_DEBUG, "Pass Number Range Start: %d\n", str[n - 1].pass_range[0]);
                xmlFree(att);
            }

            if ((att = xmlGetProp(cur, (xmlChar*)"pass_end")) != nullptr) {
                if (strlen((char*)att) > 0) str[n - 1].pass_range[1] = atoi((char*)att);
                UDA_LOG(UDA_LOG_DEBUG, "Pass Number Range End  : %d\n", str[n - 1].pass_range[1]);
                xmlFree(att);
            }

            if ((att = xmlGetProp(cur, (xmlChar*)"value")) != nullptr) {
                if (strlen((char*)att) > 0) {
                    str[n - 1].timeoffset.offset = (double)atof((char*)att);
                    UDA_LOG(UDA_LOG_DEBUG, "Time Offset  : %f\n", str[n - 1].timeoffset.offset);
                }
                xmlFree(att);
            }

            if ((att = xmlGetProp(cur, (xmlChar*)"method")) != nullptr) {
                if (strlen((char*)att) > 0) str[n - 1].timeoffset.method = (int)atoi((char*)att);
                UDA_LOG(UDA_LOG_DEBUG, "Time Offset Method  : %d\n", str[n - 1].timeoffset.method);
                xmlFree(att);
            }

            if ((att = xmlGetProp(cur, (xmlChar*)"start")) != nullptr) {
                if (strlen((char*)att) > 0) {
                    str[n - 1].timeoffset.offset = (double)atof((char*)att);
                    UDA_LOG(UDA_LOG_DEBUG, "Start Time  : %f\n", str[n - 1].timeoffset.offset);
                }
                xmlFree(att);
            }

            if ((att = xmlGetProp(cur, (xmlChar*)"interval")) != nullptr) {
                if (strlen((char*)att) > 0) {
                    str[n - 1].timeoffset.interval = (double)atof((char*)att);
                    UDA_LOG(UDA_LOG_DEBUG, "Time Interval: %f\n", str[n - 1].timeoffset.interval);
                }
                xmlFree(att);
            }

            if ((att = xmlGetProp(cur, (xmlChar*)"scale")) != nullptr) {
                if (strlen((char*)att) > 0)
                    str[n - 1].timeoffset.offset = deScale((char*)att) * str[n - 1].timeoffset.offset;
                UDA_LOG(UDA_LOG_DEBUG, "Scaled Time Offset  : %f\n", str[n - 1].timeoffset.offset);
                xmlFree(att);
            }
        }
        cur = cur->next;
    }
    actions->nactions = n;    // Number of Tags Found
    actions->action = str;    // Array of Actions bounded by a Ranges
}


void parseCompositeSubset(xmlDocPtr doc, xmlNodePtr cur, COMPOSITE* comp)
{

    xmlChar* att;    // General Input of tag attribute values

    SUBSET* str = comp->subsets;
    int n = 0;                // Counter
    int n0, n1;

    // Attributes

    cur = cur->xmlChildrenNode;
    while (cur != nullptr) {
        UDA_LOG(UDA_LOG_DEBUG, "parseCompositeSubset: %s\n", (char*)cur->name);
        if ((!xmlStrcmp(cur->name, (const xmlChar*)"subset"))) {
            n++;
            str = (SUBSET*)realloc((void*)str, n * sizeof(SUBSET));

            initSubset(&str[n - 1]);

            // Attributes

            if ((att = xmlGetProp(cur, (xmlChar*)"data")) != nullptr) {
                if (strlen((char*)att) > 0) strcpy(str[n - 1].data_signal, (char*)att);
                UDA_LOG(UDA_LOG_DEBUG, "Subset Signal: %s\n", str[n - 1].data_signal);
                xmlFree(att);
            }

            if ((att = xmlGetProp(cur, (xmlChar*)"reform")) != nullptr) {
                if (att[0] == 'Y' || att[0] == 'y') str[n - 1].reform = 1;
                xmlFree(att);
            }

            if ((att = xmlGetProp(cur, (xmlChar*)"member")) != nullptr) {
                if (strlen((char*)att) > 0) strcpy(str[n - 1].member, (char*)att);
                UDA_LOG(UDA_LOG_DEBUG, "Subset member: %s\n", str[n - 1].member);
                xmlFree(att);
            }

            if ((att = xmlGetProp(cur, (xmlChar*)"function")) != nullptr) {
                if (strlen((char*)att) > 0) strcpy(str[n - 1].function, (char*)att);
                UDA_LOG(UDA_LOG_DEBUG, "Subset function: %s\n", str[n - 1].function);
                xmlFree(att);
            }

            if ((att = xmlGetProp(cur, (xmlChar*)"order")) != nullptr) {
                if (strlen((char*)att) > 0) str[n - 1].order = atoi((char*)att);
                UDA_LOG(UDA_LOG_DEBUG, "Subset order: %d\n", str[n - 1].order);
                xmlFree(att);
            }

            // Fixed Length Attribute Arrays

            parse_fixed_length_str_array(cur, "operation", str[n - 1].operation, &str[n - 1].nbound);
            for (int i = 0; i < str[n - 1].nbound; i++) {
                str[n - 1].dimid[i] = i;                    // Ordering is as DATA[4][3][2][1][0]
            }

            parse_fixed_length_array(cur, "bound", (void*)str[n - 1].bound, UDA_TYPE_DOUBLE, &n0);
            parse_fixed_length_array(cur, "dimid", (void*)str[n - 1].dimid, UDA_TYPE_INT, &n1);

            if (parseOperation(&str[n - 1]) != 0) {
                return;
            }

            for (int i = 0; i < str[n - 1].nbound; i++) {
                UDA_LOG(UDA_LOG_DEBUG, "Subsetting Bounding Values : %e\n", str[n - 1].bound[i]);
                UDA_LOG(UDA_LOG_DEBUG, "Subsetting Operation       : %s\n", str[n - 1].operation[i]);
                UDA_LOG(UDA_LOG_DEBUG, "Dimension ID               : %d\n", str[n - 1].dimid[i]);
                UDA_LOG(UDA_LOG_DEBUG, "Subsetting Is Index?       : %d\n", str[n - 1].isindex[i]);
                UDA_LOG(UDA_LOG_DEBUG, "Subsetting Lower Index     : %d\n", (int)str[n - 1].lbindex[i].value);
                UDA_LOG(UDA_LOG_DEBUG, "Subsetting Upper Index     : %d\n", (int)str[n - 1].ubindex[i].value);
            }
        }
        cur = cur->next;
    }
    comp->nsubsets = n;        // Number of Subsets
    comp->subsets = str;    // Array of Subset Actions
}


void parseMaps(xmlDocPtr doc, xmlNodePtr cur, COMPOSITE* comp)
{
}


void parseDimComposite(xmlDocPtr doc, xmlNodePtr cur, COMPOSITE* comp)
{

    xmlChar* att;    // General Input of tag attribute values

    DIMENSION* str = comp->dimensions;
    int n = 0;                // Counter

    cur = cur->xmlChildrenNode;
    while (cur != nullptr) {
        UDA_LOG(UDA_LOG_DEBUG, "parseDimComposite: %s\n", (char*)cur->name);
        if ((!xmlStrcmp(cur->name, (const xmlChar*)"composite_dim"))) {
            n++;
            str = (DIMENSION*)realloc((void*)str, n * sizeof(DIMENSION));

            init_dimension(&str[n - 1]);
            str[n - 1].dimType = UDA_DIM_COMPOSITE_TYPE;
            init_dim_composite(&str[n - 1].dimcomposite);

            // Attributes

            if ((att = xmlGetProp(cur, (xmlChar*)"to_dim")) != nullptr) {            // Target Dimension
                if (strlen((char*)att) > 0) {
                    str[n - 1].dimid = atoi((char*)att);                // Duplicate these tags for convenience
                    str[n - 1].dimcomposite.to_dim = atoi((char*)att);
                }
                UDA_LOG(UDA_LOG_DEBUG, "To Dimension  : %d\n", str[n - 1].dimid);
                xmlFree(att);
            }

            if ((att = xmlGetProp(cur, (xmlChar*)"from_dim")) !=
                nullptr) {        // Swap with this Dimension otherwise swap with Data
                if (strlen((char*)att) > 0) str[n - 1].dimcomposite.from_dim = atoi((char*)att);
                UDA_LOG(UDA_LOG_DEBUG, "From Dimension  : %d\n", str[n - 1].dimcomposite.from_dim);
                xmlFree(att);
            }

            if ((att = xmlGetProp(cur, (xmlChar*)"dim")) != nullptr ||
                (att = xmlGetProp(cur, (xmlChar*)"data")) != nullptr) {
                if (strlen((char*)att) > 0) strcpy(str[n - 1].dimcomposite.dim_signal, (char*)att);
                UDA_LOG(UDA_LOG_DEBUG, "Dimension Signal  : %s\n", str[n - 1].dimcomposite.dim_signal);
                xmlFree(att);
            }

            if ((att = xmlGetProp(cur, (xmlChar*)"error")) != nullptr) {
                if (strlen((char*)att) > 0) strcpy(str[n - 1].dimcomposite.dim_error, (char*)att);
                UDA_LOG(UDA_LOG_DEBUG, "Error Signal  : %s\n", str[n - 1].dimcomposite.dim_error);
                xmlFree(att);
            }
            if ((att = xmlGetProp(cur, (xmlChar*)"aserror")) != nullptr) {
                if (strlen((char*)att) > 0) strcpy(str[n - 1].dimcomposite.dim_aserror, (char*)att);
                UDA_LOG(UDA_LOG_DEBUG, "Error Signal  : %s\n", str[n - 1].dimcomposite.dim_aserror);
                xmlFree(att);
            }
            if ((att = xmlGetProp(cur, (xmlChar*)"file")) != nullptr) {
                if (strlen((char*)att) > 0) strcpy(str[n - 1].dimcomposite.file, (char*)att);
                UDA_LOG(UDA_LOG_DEBUG, "Dimension Source File: %s\n", str[n - 1].dimcomposite.file);
                xmlFree(att);
            }
            if ((att = xmlGetProp(cur, (xmlChar*)"format")) != nullptr) {
                if (strlen((char*)att) > 0) strcpy(str[n - 1].dimcomposite.format, (char*)att);
                UDA_LOG(UDA_LOG_DEBUG, "Dimension Source File Format: %s\n", str[n - 1].dimcomposite.format);
                xmlFree(att);
            }


        }
        cur = cur->next;
    }
    comp->ndimensions = n;    // Number of Tags Found
    comp->dimensions = str;    // Array of Composite Signal Actions on Dimensions
}

void parseComposite(xmlDocPtr doc, xmlNodePtr cur, ACTIONS* actions)
{

    xmlChar* att;    // General Input of tag attribute values

    ACTION* str = actions->action;
    int n = actions->nactions;        // Counter

    cur = cur->xmlChildrenNode;
    while (cur != nullptr) {
        UDA_LOG(UDA_LOG_DEBUG, "parseComposite: %s\n", (char*)cur->name);
        if ((!xmlStrcmp(cur->name, (const xmlChar*)"composite"))) {
            n++;
            str = (ACTION*)realloc((void*)str, n * sizeof(ACTION));

            initAction(&str[n - 1]);
            str[n - 1].actionType = UDA_COMPOSITE_TYPE;
            init_composite(&str[n - 1].composite);

            // Attributes

            if ((att = xmlGetProp(cur, (xmlChar*)"id")) != nullptr) {
                if (strlen((char*)att) > 0) str[n - 1].actionId = atoi((char*)att);
                UDA_LOG(UDA_LOG_DEBUG, "Action ID: %d\n", str[n - 1].actionId);
                xmlFree(att);
            }

            if ((att = xmlGetProp(cur, (xmlChar*)"exp_number_start")) != nullptr) {
                if (strlen((char*)att) > 0) str[n - 1].exp_range[0] = atoi((char*)att);
                UDA_LOG(UDA_LOG_DEBUG, "Exp Number Range Start: %d\n", str[n - 1].exp_range[0]);
                xmlFree(att);
            }

            if ((att = xmlGetProp(cur, (xmlChar*)"exp_number_end")) != nullptr) {
                if (strlen((char*)att) > 0) str[n - 1].exp_range[1] = atoi((char*)att);
                UDA_LOG(UDA_LOG_DEBUG, "Exp Number Range End : %d\n", str[n - 1].exp_range[1]);
                xmlFree(att);
            }

            if ((att = xmlGetProp(cur, (xmlChar*)"pass_start")) != nullptr) {
                if (strlen((char*)att) > 0) str[n - 1].pass_range[0] = atoi((char*)att);
                UDA_LOG(UDA_LOG_DEBUG, "Pass Number Range Start: %d\n", str[n - 1].pass_range[0]);
                xmlFree(att);
            }

            if ((att = xmlGetProp(cur, (xmlChar*)"pass_end")) != nullptr) {
                if (strlen((char*)att) > 0) str[n - 1].pass_range[1] = atoi((char*)att);
                UDA_LOG(UDA_LOG_DEBUG, "Pass Number Range End  : %d\n", str[n - 1].pass_range[1]);
                xmlFree(att);
            }

            if ((att = xmlGetProp(cur, (xmlChar*)"data")) != nullptr) {
                if (strlen((char*)att) > 0) strcpy(str[n - 1].composite.data_signal, (char*)att);
                UDA_LOG(UDA_LOG_DEBUG, "Data Signal  : %s\n", str[n - 1].composite.data_signal);
                xmlFree(att);
            }

            if ((att = xmlGetProp(cur, (xmlChar*)"file")) != nullptr) {
                if (strlen((char*)att) > 0) strcpy(str[n - 1].composite.file, (char*)att);
                UDA_LOG(UDA_LOG_DEBUG, "Data Source File: %s\n", str[n - 1].composite.file);
                xmlFree(att);
            }

            if ((att = xmlGetProp(cur, (xmlChar*)"format")) != nullptr) {
                if (strlen((char*)att) > 0) strcpy(str[n - 1].composite.format, (char*)att);
                UDA_LOG(UDA_LOG_DEBUG, "Source File Format: %s\n", str[n - 1].composite.format);
                xmlFree(att);
            }

            if ((att = xmlGetProp(cur, (xmlChar*)"error")) != nullptr) {
                if (strlen((char*)att) > 0) strcpy(str[n - 1].composite.error_signal, (char*)att);
                UDA_LOG(UDA_LOG_DEBUG, "Error Signal  : %s\n", str[n - 1].composite.error_signal);
                xmlFree(att);
            }
            if ((att = xmlGetProp(cur, (xmlChar*)"aserror")) != nullptr) {
                if (strlen((char*)att) > 0) strcpy(str[n - 1].composite.aserror_signal, (char*)att);
                UDA_LOG(UDA_LOG_DEBUG, "Error Signal  : %s\n", str[n - 1].composite.aserror_signal);
                xmlFree(att);
            }
            if ((att = xmlGetProp(cur, (xmlChar*)"mapto")) != nullptr) {
                if (strlen((char*)att) > 0) strcpy(str[n - 1].composite.aserror_signal, (char*)att);
                UDA_LOG(UDA_LOG_DEBUG, "Map to Signal  : %s\n", str[n - 1].composite.map_to_signal);
                xmlFree(att);
            }


            if ((att = xmlGetProp(cur, (xmlChar*)"order")) != nullptr) {
                if (strlen((char*)att) > 0) str[n - 1].composite.order = atoi((char*)att);
                UDA_LOG(UDA_LOG_DEBUG, "Time Dimension: %d\n", str[n - 1].composite.order);
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
}

void parseDimErrorModel(xmlDocPtr doc, xmlNodePtr cur, ERRORMODEL* mod)
{
    xmlChar* att;    // General Input of tag attribute values
    float* params;

    DIMENSION* str = mod->dimensions;
    int n = 0;                // Counter

    cur = cur->xmlChildrenNode;
    while (cur != nullptr) {
        UDA_LOG(UDA_LOG_DEBUG, "parseDimErrorModel: %s\n", (char*)cur->name);
        if ((!xmlStrcmp(cur->name, (const xmlChar*)"dimension"))) {
            n++;
            str = (DIMENSION*)realloc((void*)str, n * sizeof(DIMENSION));

            init_dimension(&str[n - 1]);
            str[n - 1].dimType = UDA_DIM_ERROR_MODEL_TYPE;
            init_dim_error_model(&str[n - 1].dimerrormodel);

            // Attributes

            if ((att = xmlGetProp(cur, (xmlChar*)"dimid")) != nullptr) {            // Target Dimension
                if (strlen((char*)att) > 0) str[n - 1].dimid = atoi((char*)att);
                UDA_LOG(UDA_LOG_DEBUG, "Dimension : %d\n", str[n - 1].dimid);
                xmlFree(att);
            }
            if ((att = xmlGetProp(cur, (xmlChar*)"model")) != nullptr) {            // Error Model
                if (strlen((char*)att) > 0) str[n - 1].dimerrormodel.model = atoi((char*)att);
                UDA_LOG(UDA_LOG_DEBUG, "Model : %d\n", str[n - 1].dimerrormodel.model);
                xmlFree(att);
            }

            // Child Tags

            params = parse_float_array(doc, cur, "params", &str[n - 1].dimerrormodel.param_n);
            if (params != nullptr) {
                if (str[n - 1].dimerrormodel.param_n > MAXERRPARAMS) str[n - 1].dimerrormodel.param_n = MAXERRPARAMS;
                for (int i = 0; i < str[n - 1].dimerrormodel.param_n; i++) {
                    str[n - 1].dimerrormodel.params[i] = params[i];
                }
                free(params);
            }


        }
        cur = cur->next;
    }
    mod->ndimensions = n;    // Number of Tags Found
    mod->dimensions = str;    // Array of Error Model Actions on Dimensions
}


void parse_error_model(xmlDocPtr doc, xmlNodePtr cur, ACTIONS* actions)
{
    xmlChar* att;    // General Input of tag attribute values
    float* params;

    ACTION* str = actions->action;
    int n = actions->nactions;        // Counter

    cur = cur->xmlChildrenNode;
    while (cur != nullptr) {
        UDA_LOG(UDA_LOG_DEBUG, "parseErrorModel: %s\n", (char*)cur->name);
        if ((!xmlStrcmp(cur->name, (const xmlChar*)"errormodel"))) {
            n++;
            str = (ACTION*)realloc((void*)str, n * sizeof(ACTION));

            initAction(&str[n - 1]);
            str[n - 1].actionType = UDA_ERROR_MODEL_TYPE;
            init_error_model(&str[n - 1].errormodel);

            // Attributes

            if ((att = xmlGetProp(cur, (xmlChar*)"id")) != nullptr) {
                if (strlen((char*)att) > 0) str[n - 1].actionId = atoi((char*)att);
                UDA_LOG(UDA_LOG_DEBUG, "Action ID: %d\n", str[n - 1].actionId);
                xmlFree(att);
            }

            if ((att = xmlGetProp(cur, (xmlChar*)"exp_number_start")) != nullptr) {
                if (strlen((char*)att) > 0) str[n - 1].exp_range[0] = atoi((char*)att);
                UDA_LOG(UDA_LOG_DEBUG, "Exp Number Range Start: %d\n", str[n - 1].exp_range[0]);
                xmlFree(att);
            }

            if ((att = xmlGetProp(cur, (xmlChar*)"exp_number_end")) != nullptr) {
                if (strlen((char*)att) > 0) str[n - 1].exp_range[1] = atoi((char*)att);
                UDA_LOG(UDA_LOG_DEBUG, "Exp Number Range End : %d\n", str[n - 1].exp_range[1]);
                xmlFree(att);
            }

            if ((att = xmlGetProp(cur, (xmlChar*)"pass_start")) != nullptr) {
                if (strlen((char*)att) > 0) str[n - 1].pass_range[0] = atoi((char*)att);
                UDA_LOG(UDA_LOG_DEBUG, "Pass Number Range Start: %d\n", str[n - 1].pass_range[0]);
                xmlFree(att);
            }

            if ((att = xmlGetProp(cur, (xmlChar*)"pass_end")) != nullptr) {
                if (strlen((char*)att) > 0) str[n - 1].pass_range[1] = atoi((char*)att);
                UDA_LOG(UDA_LOG_DEBUG, "Pass Number Range End  : %d\n", str[n - 1].pass_range[1]);
                xmlFree(att);
            }

            if ((att = xmlGetProp(cur, (xmlChar*)"model")) != nullptr) {
                if (strlen((char*)att) > 0) str[n - 1].errormodel.model = atoi((char*)att);
                UDA_LOG(UDA_LOG_DEBUG, "Error Distribution Model: %d\n", str[n - 1].errormodel.model);
                xmlFree(att);
            }

            // Child Tags

            params = parse_float_array(doc, cur, "params", &str[n - 1].errormodel.param_n);
            if (params != nullptr) {
                if (str[n - 1].errormodel.param_n > MAXERRPARAMS) str[n - 1].errormodel.param_n = MAXERRPARAMS;
                for (int i = 0; i < str[n - 1].errormodel.param_n; i++) {
                    str[n - 1].errormodel.params[i] = params[i];
                }
                free(params);
            }

            parseDimErrorModel(doc, cur, &str[n - 1].errormodel);

        }
        cur = cur->next;
    }
    actions->nactions = n;    // Number of Tags Found
    actions->action = str;    // Array of Actions bounded by a Ranges
}

void parseDimDocumentation(xmlDocPtr doc, xmlNodePtr cur, DOCUMENTATION* document)
{
    xmlChar* att;    // General Input of tag attribute values

    DIMENSION* str = document->dimensions;
    int n = 0;                // Counter

    cur = cur->xmlChildrenNode;
    while (cur != nullptr) {
        UDA_LOG(UDA_LOG_DEBUG, "parseDimDocumentation: %s\n", (char*)cur->name);
        if ((!xmlStrcmp(cur->name, (const xmlChar*)"dimension"))) {
            n++;
            str = (DIMENSION*)realloc((void*)str, n * sizeof(DIMENSION));

            init_dimension(&str[n - 1]);
            str[n - 1].dimType = UDA_DIM_DOCUMENTATION_TYPE;
            init_dim_documentation(&str[n - 1].dimdocumentation);

            // Attributes

            if ((att = xmlGetProp(cur, (xmlChar*)"dimid")) != nullptr) {            // Target Dimension
                if (strlen((char*)att) > 0) str[n - 1].dimid = atoi((char*)att);
                UDA_LOG(UDA_LOG_DEBUG, "To Dimension  : %d\n", str[n - 1].dimid);
                xmlFree(att);
            }

            // Child Tags

            parse_target_string(doc, cur, "label", str[n - 1].dimdocumentation.label);
            parse_target_string(doc, cur, "units", str[n - 1].dimdocumentation.units);

        }
        cur = cur->next;
    }
    document->ndimensions = n;    // Number of Tags Found
    document->dimensions = str;    // Array of Composite Signal Actions on Dimensions
}

void parse_documentation(xmlDocPtr doc, xmlNodePtr cur, ACTIONS* actions)
{
    xmlChar* att;    // General Input of tag attribute values

    ACTION* str = actions->action;
    int n = actions->nactions;        // Counter

    cur = cur->xmlChildrenNode;
    while (cur != nullptr) {
        UDA_LOG(UDA_LOG_DEBUG, "parseDocumentation: %s\n", (char*)cur->name);
        if ((!xmlStrcmp(cur->name, (const xmlChar*)"documentation"))) {
            n++;
            str = (ACTION*)realloc((void*)str, n * sizeof(ACTION));

            initAction(&str[n - 1]);
            str[n - 1].actionType = UDA_DOCUMENTATION_TYPE;
            init_documentation(&str[n - 1].documentation);

            // Attributes

            if ((att = xmlGetProp(cur, (xmlChar*)"id")) != nullptr) {
                if (strlen((char*)att) > 0) str[n - 1].actionId = atoi((char*)att);
                UDA_LOG(UDA_LOG_DEBUG, "Action ID: %d\n", str[n - 1].actionId);
                xmlFree(att);
            }

            if ((att = xmlGetProp(cur, (xmlChar*)"exp_number_start")) != nullptr) {
                if (strlen((char*)att) > 0) str[n - 1].exp_range[0] = atoi((char*)att);
                UDA_LOG(UDA_LOG_DEBUG, "Exp Number Range Start: %d\n", str[n - 1].exp_range[0]);
                xmlFree(att);
            }

            if ((att = xmlGetProp(cur, (xmlChar*)"exp_number_end")) != nullptr) {
                if (strlen((char*)att) > 0) str[n - 1].exp_range[1] = atoi((char*)att);
                UDA_LOG(UDA_LOG_DEBUG, "Exp Number Range End : %d\n", str[n - 1].exp_range[1]);
                xmlFree(att);
            }

            if ((att = xmlGetProp(cur, (xmlChar*)"pass_start")) != nullptr) {
                if (strlen((char*)att) > 0) str[n - 1].pass_range[0] = atoi((char*)att);
                UDA_LOG(UDA_LOG_DEBUG, "Pass Number Range Start: %d\n", str[n - 1].pass_range[0]);
                xmlFree(att);
            }

            if ((att = xmlGetProp(cur, (xmlChar*)"pass_end")) != nullptr) {
                if (strlen((char*)att) > 0) str[n - 1].pass_range[1] = atoi((char*)att);
                UDA_LOG(UDA_LOG_DEBUG, "Pass Number Range End  : %d\n", str[n - 1].pass_range[1]);
                xmlFree(att);
            }

            // Child Tags

            parse_target_string(doc, cur, "description", str[n - 1].documentation.description);
            parse_target_string(doc, cur, "label", str[n - 1].documentation.label);
            parse_target_string(doc, cur, "units", str[n - 1].documentation.units);

            parseDimDocumentation(doc, cur, &str[n - 1].documentation);

        }
        cur = cur->next;
    }
    actions->nactions = n;    // Number of Tags Found
    actions->action = str;    // Array of Actions bounded by a Ranges
}

void parseDimCalibration(xmlDocPtr doc, xmlNodePtr cur, CALIBRATION* cal)
{
    xmlChar* att;    // General Input of tag attribute values

    DIMENSION* str = cal->dimensions;
    int n = 0;                // Counter

    cur = cur->xmlChildrenNode;
    while (cur != nullptr) {
        UDA_LOG(UDA_LOG_DEBUG, "parseDimCalibration: %s\n", (char*)cur->name);
        if ((!xmlStrcmp(cur->name, (const xmlChar*)"dimension"))) {
            n++;
            str = (DIMENSION*)realloc((void*)str, n * sizeof(DIMENSION));

            init_dimension(&str[n - 1]);
            str[n - 1].dimType = UDA_DIM_CALIBRATION_TYPE;
            init_dim_calibration(&str[n - 1].dimcalibration);

            // Attributes

            if ((att = xmlGetProp(cur, (xmlChar*)"dimid")) != nullptr) {            // Target Dimension
                if (strlen((char*)att) > 0) str[n - 1].dimid = atoi((char*)att);
                UDA_LOG(UDA_LOG_DEBUG, "To Dimension  : %d\n", str[n - 1].dimid);
                xmlFree(att);
            }

            if ((att = xmlGetProp(cur, (xmlChar*)"invert")) != nullptr) {
                if (att[0] == 'y' || att[0] == 'Y') str[n - 1].dimcalibration.invert = 1;
                UDA_LOG(UDA_LOG_DEBUG, "Calibration Invert: %d\n", str[n - 1].dimcalibration.invert);
                xmlFree(att);
            }

            // Child Tags

            parse_target_string(doc, cur, "units", str[n - 1].dimcalibration.units);
            parse_target_value(doc, cur, "factor", &str[n - 1].dimcalibration.factor);
            parse_target_value(doc, cur, "offset", &str[n - 1].dimcalibration.offset);

            UDA_LOG(UDA_LOG_DEBUG, "Dimension Units               : %s\n", str[n - 1].dimcalibration.units);
            UDA_LOG(UDA_LOG_DEBUG, "Dimension Calibration Factor  : %f\n", str[n - 1].dimcalibration.factor);
            UDA_LOG(UDA_LOG_DEBUG, "Dimension Calibration Offset  : %f\n", str[n - 1].dimcalibration.offset);
        }
        cur = cur->next;
    }
    cal->ndimensions = n;    // Number of Tags Found
    cal->dimensions = str;    // Array of Composite Signal Actions on Dimensions
}

void parse_calibration(xmlDocPtr doc, xmlNodePtr cur, ACTIONS* actions)
{
    xmlChar* att;    // General Input of tag attribute values

    ACTION* str = actions->action;
    int n = actions->nactions;        // Counter

    cur = cur->xmlChildrenNode;
    while (cur != nullptr) {
        UDA_LOG(UDA_LOG_DEBUG, "parseCalibration: %s\n", (char*)cur->name);
        if ((!xmlStrcmp(cur->name, (const xmlChar*)"calibration"))) {
            n++;
            str = (ACTION*)realloc((void*)str, n * sizeof(ACTION));

            initAction(&str[n - 1]);
            str[n - 1].actionType = UDA_CALIBRATION_TYPE;
            init_calibration(&str[n - 1].calibration);

            // Attributes

            if ((att = xmlGetProp(cur, (xmlChar*)"id")) != nullptr) {
                if (strlen((char*)att) > 0) str[n - 1].actionId = atoi((char*)att);
                UDA_LOG(UDA_LOG_DEBUG, "Action ID: %d\n", str[n - 1].actionId);
                xmlFree(att);
            }

            if ((att = xmlGetProp(cur, (xmlChar*)"exp_number_start")) != nullptr) {
                if (strlen((char*)att) > 0) str[n - 1].exp_range[0] = atoi((char*)att);
                UDA_LOG(UDA_LOG_DEBUG, "Exp Number Range Start: %d\n", str[n - 1].exp_range[0]);
                xmlFree(att);
            }

            if ((att = xmlGetProp(cur, (xmlChar*)"exp_number_end")) != nullptr) {
                if (strlen((char*)att) > 0) str[n - 1].exp_range[1] = atoi((char*)att);
                UDA_LOG(UDA_LOG_DEBUG, "Exp Number Range End : %d\n", str[n - 1].exp_range[1]);
                xmlFree(att);
            }

            if ((att = xmlGetProp(cur, (xmlChar*)"pass_start")) != nullptr) {
                if (strlen((char*)att) > 0) str[n - 1].pass_range[0] = atoi((char*)att);
                UDA_LOG(UDA_LOG_DEBUG, "Pass Number Range Start: %d\n", str[n - 1].pass_range[0]);
                xmlFree(att);
            }

            if ((att = xmlGetProp(cur, (xmlChar*)"pass_end")) != nullptr) {
                if (strlen((char*)att) > 0) str[n - 1].pass_range[1] = atoi((char*)att);
                UDA_LOG(UDA_LOG_DEBUG, "Pass Number Range End  : %d\n", str[n - 1].pass_range[1]);
                xmlFree(att);
            }

            if ((att = xmlGetProp(cur, (xmlChar*)"target")) != nullptr) {
                if (strlen((char*)att) > 0) strcpy(str[n - 1].calibration.target, (char*)att);
                UDA_LOG(UDA_LOG_DEBUG, "Calibration Target: %s\n", str[n - 1].calibration.target);
                xmlFree(att);
            }

            if ((att = xmlGetProp(cur, (xmlChar*)"invert")) != nullptr) {
                if (att[0] == 'y' || att[0] == 'Y') str[n - 1].calibration.invert = 1;
                UDA_LOG(UDA_LOG_DEBUG, "Calibration Invert: %d\n", str[n - 1].calibration.invert);
                xmlFree(att);
            }

            // Child Tags

            parse_target_string(doc, cur, "units", str[n - 1].calibration.units);
            parse_target_value(doc, cur, "factor", &str[n - 1].calibration.factor);
            parse_target_value(doc, cur, "offset", &str[n - 1].calibration.offset);

            UDA_LOG(UDA_LOG_DEBUG, "Data Units               : %s\n", str[n - 1].calibration.units);
            UDA_LOG(UDA_LOG_DEBUG, "Data Calibration Factor  : %f\n", str[n - 1].calibration.factor);
            UDA_LOG(UDA_LOG_DEBUG, "Data Calibration Offset  : %f\n", str[n - 1].calibration.offset);

            parseDimCalibration(doc, cur, &str[n - 1].calibration);

        }
        cur = cur->next;
    }
    actions->nactions = n;    // Number of Tags Found
    actions->action = str;    // Array of Actions bounded by a Ranges
}


void parse_subset(xmlDocPtr doc, xmlNodePtr cur, ACTIONS* actions)
{
    xmlChar* att;    // General Input of tag attribute values

    ACTION* str = actions->action;
    SUBSET* sub = nullptr;
    int n = actions->nactions;        // Counter
    int n0, n1;

    cur = cur->xmlChildrenNode;
    while (cur != nullptr) {
        UDA_LOG(UDA_LOG_DEBUG, "parseSubset: %s\n", (char*)cur->name);
        if ((!xmlStrcmp(cur->name, (const xmlChar*)"subset"))) {
            n++;
            str = (ACTION*)realloc((void*)str, n * sizeof(ACTION));

            initAction(&str[n - 1]);
            str[n - 1].actionType = UDA_SUBSET_TYPE;
            sub = &str[n - 1].subset;
            initSubset(sub);

            // Attributes

            if ((att = xmlGetProp(cur, (xmlChar*)"id")) != nullptr) {
                if (strlen((char*)att) > 0) str[n - 1].actionId = atoi((char*)att);
                UDA_LOG(UDA_LOG_DEBUG, "Action ID: %d\n", str[n - 1].actionId);
                xmlFree(att);
            }

            if ((att = xmlGetProp(cur, (xmlChar*)"data")) != nullptr) {
                if (strlen((char*)att) > 0) strcpy(sub->data_signal, (char*)att);
                UDA_LOG(UDA_LOG_DEBUG, "Data Signal  : %s\n", sub->data_signal);
                xmlFree(att);
            }

            // Child Tags

            // Attributes

            if ((att = xmlGetProp(cur, (xmlChar*)"reform")) != nullptr) {
                if (att[0] == 'Y' || att[0] == 'y') sub->reform = 1;
                xmlFree(att);
            }

            if ((att = xmlGetProp(cur, (xmlChar*)"member")) != nullptr) {
                if (strlen((char*)att) > 0) strcpy(sub->member, (char*)att);
                UDA_LOG(UDA_LOG_DEBUG, "Subset Member: %s\n", sub->member);
                xmlFree(att);
            }

            if ((att = xmlGetProp(cur, (xmlChar*)"function")) != nullptr) {
                if (strlen((char*)att) > 0) strcpy(sub->function, (char*)att);
                UDA_LOG(UDA_LOG_DEBUG, "Subset function: %s\n", sub->function);
                xmlFree(att);
            }

            if ((att = xmlGetProp(cur, (xmlChar*)"order")) != nullptr) {
                if (strlen((char*)att) > 0) sub->order = atoi((char*)att);
                UDA_LOG(UDA_LOG_DEBUG, "Subset order: %d\n", sub->order);
                xmlFree(att);
            }

            // Fixed Length Attribute Arrays

            parse_fixed_length_str_array(cur, "operation", &sub->operation[0], &sub->nbound);
            for (int i = 0; i < sub->nbound; i++) {
                sub->dimid[i] = i;                    // Ordering is as DATA[4][3][2][1][0]
            }

            parse_fixed_length_array(cur, "bound", (void*)sub->bound, UDA_TYPE_DOUBLE, &n0);
            parse_fixed_length_array(cur, "dimid", (void*)sub->dimid, UDA_TYPE_INT, &n1);

            if (parseOperation(sub) != 0) {
                return;
            }

            for (int i = 0; i < sub->nbound; i++) {
                UDA_LOG(UDA_LOG_DEBUG, "Dimension ID               : %d\n", sub->dimid[i]);
                UDA_LOG(UDA_LOG_DEBUG, "Subsetting Bounding Values : %e\n", sub->bound[i]);
                UDA_LOG(UDA_LOG_DEBUG, "Subsetting Operation       : %s\n", sub->operation[i]);
                UDA_LOG(UDA_LOG_DEBUG, "Subsetting Is Index?       : %d\n", sub->isindex[i]);
                UDA_LOG(UDA_LOG_DEBUG, "Subsetting Lower Index     : %d\n", (int)sub->lbindex[i].value);
                UDA_LOG(UDA_LOG_DEBUG, "Subsetting Upper Index     : %d\n", (int)sub->ubindex[i].value);
            }
        }
        cur = cur->next;
    }

    actions->nactions = n;    // Number of Tags Found
    actions->action = str;    // Array of Actions bounded by a Ranges
}

int parseDoc(char* docname, ACTIONS* actions)
{
    xmlDocPtr doc;
    xmlNodePtr cur;

#ifdef TIMETEST
    struct timeval tv_start;
    struct timeval tv_end;
    float testtime ;
    int rc = gettimeofday(&tv_start, nullptr);
#endif

    xmlInitParser();

    if ((doc = xmlParseDoc((xmlChar*)docname)) == nullptr) {
        xmlFreeDoc(doc);
        xmlCleanupParser();
        addIdamError(UDA_CODE_ERROR_TYPE, "parseDoc", 1, "XML Not Parsed");
        return 1;
    }

    if ((cur = xmlDocGetRootElement(doc)) == nullptr) {
        addIdamError(UDA_CODE_ERROR_TYPE, "parseDoc", 1, "Empty XML Document");
        xmlFreeDoc(doc);
        xmlCleanupParser();
        return 1;
    }

    if (xmlStrcmp(cur->name, (const xmlChar*)"action")) {        //If No Action Tag then Nothing to be done!
        xmlFreeDoc(doc);
        xmlCleanupParser();
        return 1;
    }

    cur = cur->xmlChildrenNode;
    while (cur != nullptr) {

        if ((!xmlStrcmp(cur->name, (const xmlChar*)"signal"))) {

            parseComposite(doc, cur, actions);        // Composite can have SUBSET as a child
            parse_documentation(doc, cur, actions);
            parse_calibration(doc, cur, actions);
            parse_time_offset(doc, cur, actions);
            parse_error_model(doc, cur, actions);

            parse_subset(doc, cur, actions);        // Single Subset
        }
        cur = cur->next;
    }

    xmlFreeDoc(doc);
    xmlCleanupParser();

#ifdef TIMETEST
    rc = gettimeofday(&tv_end, nullptr);
    testtime = (float)(tv_end.tv_sec-tv_start.tv_sec)*1.0E6 + (float)(tv_end.tv_usec - tv_start.tv_usec) ;
#endif

    return 0;
}
#endif
//==================================================================================================

void print_dimensions(int ndim, DIMENSION* dims)
{
    UDA_LOG(UDA_LOG_DEBUG, "No. Dimensions     : %d\n", ndim);
    for (int i = 0; i < ndim; i++) {
        UDA_LOG(UDA_LOG_DEBUG, "Dim id     : %d\n", dims[i].dimid);

        switch (dims[i].dimType) {

            case UDA_DIM_CALIBRATION_TYPE:
                UDA_LOG(UDA_LOG_DEBUG, "factor     : %.12f\n", dims[i].dimcalibration.factor);
                UDA_LOG(UDA_LOG_DEBUG, "Offset     : %.12f\n", dims[i].dimcalibration.offset);
                UDA_LOG(UDA_LOG_DEBUG, "Units      : %s\n", dims[i].dimcalibration.units);
                break;

            case UDA_DIM_COMPOSITE_TYPE:
                UDA_LOG(UDA_LOG_DEBUG, "to Dim       : %d\n", dims[i].dimcomposite.to_dim);
                UDA_LOG(UDA_LOG_DEBUG, "from Dim     : %d\n", dims[i].dimcomposite.from_dim);
                UDA_LOG(UDA_LOG_DEBUG, "Dim signal   : %s\n", dims[i].dimcomposite.dim_signal);
                UDA_LOG(UDA_LOG_DEBUG, "Dim Error    : %s\n", dims[i].dimcomposite.dim_error);
                UDA_LOG(UDA_LOG_DEBUG, "Dim ASError  : %s\n", dims[i].dimcomposite.dim_aserror);
                UDA_LOG(UDA_LOG_DEBUG, "Dim Source File  : %s\n", dims[i].dimcomposite.file);
                UDA_LOG(UDA_LOG_DEBUG, "Dim Source Format: %s\n", dims[i].dimcomposite.format);

                break;

            case UDA_DIM_DOCUMENTATION_TYPE:
                UDA_LOG(UDA_LOG_DEBUG, "Dim Label  : %s\n", dims[i].dimdocumentation.label);
                UDA_LOG(UDA_LOG_DEBUG, "Dim Units  : %s\n", dims[i].dimdocumentation.units);
                break;

            case UDA_DIM_ERROR_MODEL_TYPE:
                UDA_LOG(UDA_LOG_DEBUG, "Error Model Id            : %d\n", dims[i].dimerrormodel.model);
                UDA_LOG(UDA_LOG_DEBUG, "Number of Model Parameters: %d\n", dims[i].dimerrormodel.param_n);
                for (int j = 0; j < dims[i].dimerrormodel.param_n; j++)
                    UDA_LOG(UDA_LOG_DEBUG, "Parameters[%d] = %.12f\n", j, dims[i].dimerrormodel.params[j]);
                break;

            default:
                break;
        }

    }
}

void printAction(ACTION action)
{
    UDA_LOG(UDA_LOG_DEBUG, "Action XML Id    : %d\n", action.actionId);
    UDA_LOG(UDA_LOG_DEBUG, "Action Type      : %d\n", action.actionType);
    UDA_LOG(UDA_LOG_DEBUG, "In Range?        : %d\n", action.inRange);
    UDA_LOG(UDA_LOG_DEBUG, "Exp Number Range : %d -> %d\n", action.exp_range[0], action.exp_range[1]);
    UDA_LOG(UDA_LOG_DEBUG, "Pass Number Range: %d -> %d\n", action.pass_range[0], action.pass_range[1]);

    switch (action.actionType) {
        case UDA_TIME_OFFSET_TYPE:
            UDA_LOG(UDA_LOG_DEBUG, "TIMEOFFSET xml\n");
            UDA_LOG(UDA_LOG_DEBUG, "Method         : %d\n", action.timeoffset.method);
            UDA_LOG(UDA_LOG_DEBUG, "Time Offset    : %.12f\n", action.timeoffset.offset);
            UDA_LOG(UDA_LOG_DEBUG, "Time Interval  : %.12f\n", action.timeoffset.interval);
            break;
        case UDA_DOCUMENTATION_TYPE:
            UDA_LOG(UDA_LOG_DEBUG, "DOCUMENTATION xml\n");
            UDA_LOG(UDA_LOG_DEBUG, "Description: %s\n", action.documentation.description);
            UDA_LOG(UDA_LOG_DEBUG, "Data Label : %s\n", action.documentation.label);
            UDA_LOG(UDA_LOG_DEBUG, "Data Units : %s\n", action.documentation.units);
            print_dimensions(action.documentation.ndimensions, action.documentation.dimensions);
            break;
        case UDA_CALIBRATION_TYPE:
            UDA_LOG(UDA_LOG_DEBUG, "CALIBRATION xml\n");
            UDA_LOG(UDA_LOG_DEBUG, "Target     : %s\n", action.calibration.target);
            UDA_LOG(UDA_LOG_DEBUG, "Factor     : %f\n", action.calibration.factor);
            UDA_LOG(UDA_LOG_DEBUG, "Offset     : %f\n", action.calibration.offset);
            UDA_LOG(UDA_LOG_DEBUG, "Invert     : %d\n", action.calibration.invert);
            UDA_LOG(UDA_LOG_DEBUG, "Data Units : %s\n", action.calibration.units);
            print_dimensions(action.calibration.ndimensions, action.calibration.dimensions);
            break;
        case UDA_COMPOSITE_TYPE:
            UDA_LOG(UDA_LOG_DEBUG, "COMPOSITE xml\n");
            UDA_LOG(UDA_LOG_DEBUG, "Composite Data Signal    : %s\n", action.composite.data_signal);
            UDA_LOG(UDA_LOG_DEBUG, "Composite Error Signal   : %s\n", action.composite.error_signal);
            UDA_LOG(UDA_LOG_DEBUG, "Composite Asymmetric Error Signal   : %s\n", action.composite.aserror_signal);
            UDA_LOG(UDA_LOG_DEBUG, "Composite Map to Signal  : %s\n", action.composite.map_to_signal);
            UDA_LOG(UDA_LOG_DEBUG, "Composite Source File    : %s\n", action.composite.file);
            UDA_LOG(UDA_LOG_DEBUG, "Composite Source Format  : %s\n", action.composite.format);
            UDA_LOG(UDA_LOG_DEBUG, "Composite Time Dimension : %d\n", action.composite.order);
            print_dimensions(action.composite.ndimensions, action.composite.dimensions);
            break;
        case UDA_ERROR_MODEL_TYPE:
            UDA_LOG(UDA_LOG_DEBUG, "ERRORMODEL xml\n");
            UDA_LOG(UDA_LOG_DEBUG, "Error Model Id            : %d\n", action.errormodel.model);
            UDA_LOG(UDA_LOG_DEBUG, "Number of Model Parameters: %d\n", action.errormodel.param_n);
            for (int i = 0; i < action.errormodel.param_n; i++)
                UDA_LOG(UDA_LOG_DEBUG, "Parameters[%d] = %.12f\n", i, action.errormodel.params[i]);
            print_dimensions(action.errormodel.ndimensions, action.errormodel.dimensions);
            break;

        case UDA_SERVER_SIDE_TYPE:
            UDA_LOG(UDA_LOG_DEBUG, "SERVERSIDE Actions\n");
            UDA_LOG(UDA_LOG_DEBUG, "Number of Serverside Subsets: %d\n", action.serverside.nsubsets);
            for (int i = 0; i < action.serverside.nsubsets; i++) {
                UDA_LOG(UDA_LOG_DEBUG, "Number of Subsetting Operations: %d\n", action.serverside.subsets[i].nbound);
                UDA_LOG(UDA_LOG_DEBUG, "Reform?                        : %d\n", action.serverside.subsets[i].reform);
                UDA_LOG(UDA_LOG_DEBUG, "Member                         : %s\n", action.serverside.subsets[i].member);
                UDA_LOG(UDA_LOG_DEBUG, "Function                       : %s\n", action.serverside.subsets[i].function);
                UDA_LOG(UDA_LOG_DEBUG, "Order                          : %d\n", action.serverside.subsets[i].order);
                UDA_LOG(UDA_LOG_DEBUG, "Signal                         : %s\n",
                        action.serverside.subsets[i].data_signal);
                for (int j = 0; j < action.serverside.subsets[i].nbound; j++) {
                    UDA_LOG(UDA_LOG_DEBUG, "Bounding Value: %e\n", action.serverside.subsets[i].bound[j]);
                    UDA_LOG(UDA_LOG_DEBUG, "Operation     : %s\n", action.serverside.subsets[i].operation[j]);
                    UDA_LOG(UDA_LOG_DEBUG, "Dimension ID  : %d\n", action.serverside.subsets[i].dimid[j]);
                }
            }
            UDA_LOG(UDA_LOG_DEBUG, "Number of Serverside mappings: %d\n", action.serverside.nmaps);
            break;
        case UDA_SUBSET_TYPE:
            UDA_LOG(UDA_LOG_DEBUG, "SUBSET Actions\n");
            UDA_LOG(UDA_LOG_DEBUG, "Number of Subsets: 1\n");
            UDA_LOG(UDA_LOG_DEBUG, "Number of Subsetting Operations: %d\n", action.subset.nbound);
            UDA_LOG(UDA_LOG_DEBUG, "Reform?                        : %d\n", action.subset.reform);
            UDA_LOG(UDA_LOG_DEBUG, "Member                         : %s\n", action.subset.member);
            UDA_LOG(UDA_LOG_DEBUG, "Function                       : %s\n", action.subset.function);
            UDA_LOG(UDA_LOG_DEBUG, "Order                       : %d\n", action.subset.order);
            UDA_LOG(UDA_LOG_DEBUG, "Signal                         : %s\n", action.subset.data_signal);
            for (int j = 0; j < action.subset.nbound; j++) {
                UDA_LOG(UDA_LOG_DEBUG, "Bounding Value: %e\n", action.subset.bound[j]);
                UDA_LOG(UDA_LOG_DEBUG, "Operation     : %s\n", action.subset.operation[j]);
                UDA_LOG(UDA_LOG_DEBUG, "Dimension ID  : %d\n", action.subset.dimid[j]);
            }
            break;
        default:
            break;
    }

}

void printActions(ACTIONS actions)
{
    UDA_LOG(UDA_LOG_DEBUG, "No. Action Blocks: %d\n", actions.nactions);
    for (int i = 0; i < actions.nactions; i++) {
        UDA_LOG(UDA_LOG_DEBUG, "\n\n# %d\n", i);
        printAction(actions.action[i]);
    }
    UDA_LOG(UDA_LOG_DEBUG, "\n\n");
}

// Initialise an Action Structure and Child Structures

void init_dim_calibration(DIMCALIBRATION* act)
{
    act->factor = (double)1.0E0;    // Data Calibration Correction/Scaling factor
    act->offset = (double)0.0E0;    // Data Calibration Correction/Scaling offset
    act->invert = 0;            // Don't Invert the data
    act->units[0] = '\0';
}

void init_dim_composite(DIMCOMPOSITE* act)
{
    act->to_dim = -1;                // Swap to Dimension ID
    act->from_dim = -1;                // Swap from Dimension ID
    act->file[0] = '\0';            // Data Source File (with Full Path)
    act->format[0] = '\0';            // Data Source File's Format
    act->dim_signal[0] = '\0';            // Source Signal
    act->dim_error[0] = '\0';            // Error Source Signal
    act->dim_aserror[0] = '\0';            // Asymmetric Error Source Signal
}

void init_dim_documentation(DIMDOCUMENTATION* act)
{
    act->label[0] = '\0';
    act->units[0] = '\0';            // Lower in priority than Calibration Units
}

void init_dim_error_model(DIMERRORMODEL* act)
{
    act->model = ERROR_MODEL_UNKNOWN;    // No Error Model
    act->param_n = 0;            // No. Model parameters
    for (int i = 0; i < MAXERRPARAMS; i++) {
        act->params[i] = 0.0;
    }
}

void init_dimension(DIMENSION* act)
{
    act->dimid = -1;        // Dimension Id
    act->dimType = 0;        // Structure Type
}

void init_time_offset(TIMEOFFSET* act)
{
    act->method = 0;            // Correction Method: Standard offset correction only
    act->offset = (double)0.0E0;    // Time Dimension offset correction or start time
    act->interval = (double)0.0E0;    // Time Dimension Interval correction
}

void init_calibration(CALIBRATION* act)
{
    act->factor = (double)1.0E0;    // Data Calibration Correction/Scaling factor
    act->offset = (double)0.0E0;    // Data Calibration Correction/Scaling offset
    act->units[0] = '\0';
    act->target[0] = '\0';        // Which data Component to apply calibration? (all, data, error, aserror)
    act->invert = 0;        // No Inversion
    act->ndimensions = 0;
    act->dimensions = nullptr;
}

void init_documentation(DOCUMENTATION* act)
{
    act->label[0] = '\0';
    act->units[0] = '\0';        // Lower in priority than Calibration Units
    act->description[0] = '\0';
    act->ndimensions = 0;
    act->dimensions = nullptr;
}

void init_composite(COMPOSITE* act)
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
    act->dimensions = nullptr;
    act->subsets = nullptr;
    act->maps = nullptr;
}

void initServerside(SERVERSIDE* act)
{
    act->nsubsets = 0;
    act->nmaps = 0;
    act->subsets = nullptr;
    act->maps = nullptr;
}

void init_error_model(ERRORMODEL* act)
{
    act->model = ERROR_MODEL_UNKNOWN;    // No Error Model
    act->param_n = 0;            // No. Model parameters
    for (int i = 0; i < MAXERRPARAMS; i++) {
        act->params[i] = 0.0;
    }
    act->ndimensions = 0;
    act->dimensions = nullptr;
}

void initSubset(SUBSET* act)
{
    for (int i = 0; i < UDA_MAX_DATA_RANK; i++) {
        act->bound[i] = 0.0;                // Subsetting Float Bounds
        act->ubindex[i] = { .init = false, .value = 0 };                // Subsetting Integer Bounds (Upper Index)
        act->lbindex[i] = { .init = false, .value = 0 };                // Lower Index
        act->stride[i] = { .init = false, .value = 0 };               // Stride
        act->isindex[i] = false;                // Flag the Bound is an Integer Type
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
    act->action = nullptr;    // Array of Action blocks
}

void freeActions(ACTIONS* actions)
{
    // Free Heap Memory From ACTION Structures
    void* cptr;

    UDA_LOG(UDA_LOG_DEBUG, "freeActions: Enter\n");
    UDA_LOG(UDA_LOG_DEBUG, "freeDataBlock: Number of Actions = %d \n", actions->nactions);

    if (actions->nactions == 0) return;

    for (int i = 0; i < actions->nactions; i++) {
        UDA_LOG(UDA_LOG_DEBUG, "freeDataBlock: freeing action Type = %d \n", actions->action[i].actionType);

        switch (actions->action[i].actionType) {

            case UDA_COMPOSITE_TYPE:
                if ((cptr = (void*)actions->action[i].composite.dimensions) != nullptr) {
                    free(cptr);
                    actions->action[i].composite.dimensions = nullptr;
                    actions->action[i].composite.ndimensions = 0;
                }
                if (actions->action[i].composite.nsubsets > 0) {
                    if ((cptr = (void*)actions->action[i].composite.subsets) != nullptr) free(cptr);
                    actions->action[i].composite.subsets = nullptr;
                    actions->action[i].composite.nsubsets = 0;
                }
                if (actions->action[i].composite.nmaps > 0) {
                    if ((cptr = (void*)actions->action[i].composite.maps) != nullptr) free(cptr);
                    actions->action[i].composite.maps = nullptr;
                    actions->action[i].composite.nmaps = 0;
                }
                break;

            case UDA_ERROR_MODEL_TYPE:
                actions->action[i].errormodel.param_n = 0;

                for (int j = 0; j < actions->action[i].errormodel.ndimensions; j++)
                    if ((cptr = (void*)actions->action[i].errormodel.dimensions) != nullptr) {
                        free(cptr);
                        actions->action[i].errormodel.dimensions = nullptr;
                        actions->action[i].errormodel.ndimensions = 0;
                    }

                break;

            case UDA_CALIBRATION_TYPE:
                if ((cptr = (void*)actions->action[i].calibration.dimensions) != nullptr) {
                    free(cptr);
                    actions->action[i].calibration.dimensions = nullptr;
                    actions->action[i].calibration.ndimensions = 0;
                }
                break;

            case UDA_DOCUMENTATION_TYPE:
                if ((cptr = (void*)actions->action[i].documentation.dimensions) != nullptr) {
                    free(cptr);
                    actions->action[i].documentation.dimensions = nullptr;
                    actions->action[i].documentation.ndimensions = 0;
                }
                break;

            case UDA_SERVER_SIDE_TYPE:
                if (actions->action[i].serverside.nsubsets > 0) {
                    if ((cptr = (void*)actions->action[i].serverside.subsets) != nullptr) free(cptr);
                    actions->action[i].serverside.subsets = nullptr;
                    actions->action[i].serverside.nsubsets = 0;
                }
                if (actions->action[i].serverside.nmaps > 0) {
                    if ((cptr = (void*)actions->action[i].serverside.maps) != nullptr) free(cptr);
                    actions->action[i].serverside.maps = nullptr;
                    actions->action[i].serverside.nmaps = 0;
                }
                break;

            default:
                break;
        }
    }

    if ((cptr = (void*)actions->action) != nullptr) free(cptr);
    actions->nactions = 0;
    actions->action = nullptr;

    UDA_LOG(UDA_LOG_DEBUG, "freeActions: Exit\n");
}

// Copy an Action Structure and Drop Pointers to ACTION & DIMENSION Structures (ensures a single Heap free later)

void copyActions(ACTIONS* actions_out, ACTIONS* actions_in)
{
    *actions_out = *actions_in;
    actions_in->action = nullptr;
    actions_in->nactions = 0;
}

