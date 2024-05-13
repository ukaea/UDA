/*---------------------------------------------------------------
* UDA XML Parser
*
* Input Arguments:    char *xml
*
* Returns:        parseXML    TRUE if parse was successful

*            Actions        Actions Structure
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

#include "logging/logging.h"

using namespace uda::client_server;
using namespace uda::logging;

#ifndef NOXMLPARSER

#  include "clientserver/errorLog.h"
#  include "clientserver/parseOperation.h"
#  include "common/stringUtils.h"

static double deScale(char* scale);
static void parse_target_value(xmlDocPtr doc, xmlNodePtr cur, const char* target, double* value);
static void parse_target_string(xmlDocPtr doc, xmlNodePtr cur, const char* target, char* str);
static void parse_fixed_length_array(xmlNodePtr cur, const char* target, void* array, int arraytype, int* n);
static void parse_documentation(xmlDocPtr doc, xmlNodePtr cur, Actions* actions);
static void parse_calibration(xmlDocPtr doc, xmlNodePtr cur, Actions* actions);
static void parse_time_offset(xmlDocPtr doc, xmlNodePtr cur, Actions* actions);
static void parse_error_model(xmlDocPtr doc, xmlNodePtr cur, Actions* actions);
static void parse_subset(xmlDocPtr doc, xmlNodePtr cur, Actions* actions);
static void print_dimensions(int ndim, Dimension* dims);
static void init_dim_calibration(DimCalibration* act);
static void init_dim_composite(DimComposite* act);
static void init_dim_documentation(DimDocumentation* act);
static void init_dim_error_model(DimErrorModel* act);
static void init_dimension(Dimension* act);
static void init_time_offset(TimeOffset* act);
static void init_calibration(Calibration* act);
static void init_documentation(Documentation* act);
static void init_composite(Composite* act);
static void init_error_model(ErrorModel* act);

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
            convert_non_printable((char*)key);
            if (strlen((char*)key) > 0) {
                int lkey = (int)strlen((char*)key);
                UDA_LOG(UDA_LOG_DEBUG, "parseFloatArray: [{}] {} {} ", lkey, target, (const char*)key);
                item = strtok((char*)key, delim);
                if (item != nullptr) {
                    nco++;
                    UDA_LOG(UDA_LOG_DEBUG, "parseFloatArray: [{}] {} ", nco, item);
                    value = (float*)realloc((void*)value, nco * sizeof(float));
                    value[nco - 1] = atof(item);
                    UDA_LOG(UDA_LOG_DEBUG, "parseFloatArray: [{}] {} {}", nco, item, value[nco - 1]);
                    while ((item = strtok(nullptr, delim)) != nullptr && nco <= UDA_XML_MAX_LOOP) {
                        nco++;
                        value = (float*)realloc((void*)value, nco * sizeof(float));
                        value[nco - 1] = atof(item);
                        UDA_LOG(UDA_LOG_DEBUG, "parseFloatArray: [{}] {} {}", nco, item, value[nco - 1]);
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

template <typename T> void parse_fixed_length_array(xmlNodePtr cur, const char* target, void* array, int* n)
{
    xmlChar* att = nullptr;
    *n = 0;
    const char* delim = ",";
    char* item;
    int nco = 0;

    if ((att = xmlGetProp(cur, (xmlChar*)target)) != nullptr) {
        convert_non_printable((char*)att);
        if (strlen((char*)att) > 0) {
            int l = (int)strlen((char*)att);
            UDA_LOG(UDA_LOG_DEBUG, "parseFixedLengthArray: [{}] {} {} ", l, target, att);
            item = strtok((char*)att, delim);
            if (item != nullptr) {
                nco++;
                auto p = (T*)array;
                std::stringstream ss{item};
                ss >> p[nco - 1];

                while ((item = strtok(nullptr, delim)) != nullptr && nco <= UDA_MAX_DATA_RANK) {
                    nco++;
                    ss = std::stringstream{item};
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
        convert_non_printable((char*)att);
        if (strlen((char*)att) > 0) {
            int l = (int)strlen((char*)att);
            UDA_LOG(UDA_LOG_DEBUG, "parseFixedLengthArray: [{}] {} {} ", l, target, (const char*)att);
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

void parse_fixed_length_str_array(xmlNodePtr cur, const char* target,
                                  char array[UDA_MAX_DATA_RANK][UDA_SXML_MAX_STRING], int* n)
{
    xmlChar* att = nullptr;
    *n = 0;
    const char* delim = ",";
    char* item;
    int nco = 0;

    if ((att = xmlGetProp(cur, (xmlChar*)target)) != nullptr) {
        if (strlen((char*)att) > 0) {
            int l = (int)strlen((char*)att);
            UDA_LOG(UDA_LOG_DEBUG, "parseFixedLengthStrArray: [{}] {} {} ", l, target, (const char*)att);
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
    if (strlen(scale) == 0) {
        return 1.0E0;
    }
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
    return 1.0E0; // Default value
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
            if (key != nullptr) {
                *value = (double)atof((char*)key);
            }
            if (scale != nullptr) {
                *value = *value * deScale((char*)scale);
            }
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
            if (key != nullptr) {
                strcpy(str, (char*)key);
            }
            xmlFree(key);
            break;
        }
        cur = cur->next;
    }
}

void parse_time_offset(xmlDocPtr doc, xmlNodePtr cur, Actions* actions)
{

    xmlChar* att; // General Input of tag attribute values

    Action* str = actions->action;
    int n = actions->nactions; // Counter

    cur = cur->xmlChildrenNode;
    while (cur != nullptr) {
        UDA_LOG(UDA_LOG_DEBUG, "{}", (char*)cur->name);
        if ((!xmlStrcmp(cur->name, (const xmlChar*)"time_offset"))) {
            n++;
            str = (Action*)realloc((void*)str, n * sizeof(Action));

            init_action(&str[n - 1]);
            str[n - 1].actionType = UDA_TIME_OFFSET_TYPE;
            init_time_offset(&str[n - 1].timeoffset);

            // Attributes

            if ((att = xmlGetProp(cur, (xmlChar*)"id")) != nullptr) {
                if (strlen((char*)att) > 0) {
                    str[n - 1].actionId = atoi((char*)att);
                }
                UDA_LOG(UDA_LOG_DEBUG, "Action ID: {}", str[n - 1].actionId);
                xmlFree(att);
            }

            if ((att = xmlGetProp(cur, (xmlChar*)"exp_number_start")) != nullptr) {
                if (strlen((char*)att) > 0) {
                    str[n - 1].exp_range[0] = atoi((char*)att);
                }
                UDA_LOG(UDA_LOG_DEBUG, "Exp Number Range Start: {}", str[n - 1].exp_range[0]);
                xmlFree(att);
            }

            if ((att = xmlGetProp(cur, (xmlChar*)"exp_number_end")) != nullptr) {
                if (strlen((char*)att) > 0) {
                    str[n - 1].exp_range[1] = atoi((char*)att);
                }
                UDA_LOG(UDA_LOG_DEBUG, "Exp Number Range End : {}", str[n - 1].exp_range[1]);
                xmlFree(att);
            }

            if ((att = xmlGetProp(cur, (xmlChar*)"pass_start")) != nullptr) {
                if (strlen((char*)att) > 0) {
                    str[n - 1].pass_range[0] = atoi((char*)att);
                }
                UDA_LOG(UDA_LOG_DEBUG, "Pass Number Range Start: {}", str[n - 1].pass_range[0]);
                xmlFree(att);
            }

            if ((att = xmlGetProp(cur, (xmlChar*)"pass_end")) != nullptr) {
                if (strlen((char*)att) > 0) {
                    str[n - 1].pass_range[1] = atoi((char*)att);
                }
                UDA_LOG(UDA_LOG_DEBUG, "Pass Number Range End  : {}", str[n - 1].pass_range[1]);
                xmlFree(att);
            }

            if ((att = xmlGetProp(cur, (xmlChar*)"value")) != nullptr) {
                if (strlen((char*)att) > 0) {
                    str[n - 1].timeoffset.offset = (double)atof((char*)att);
                    UDA_LOG(UDA_LOG_DEBUG, "Time Offset  : {}", str[n - 1].timeoffset.offset);
                }
                xmlFree(att);
            }

            if ((att = xmlGetProp(cur, (xmlChar*)"method")) != nullptr) {
                if (strlen((char*)att) > 0) {
                    str[n - 1].timeoffset.method = (int)atoi((char*)att);
                }
                UDA_LOG(UDA_LOG_DEBUG, "Time Offset Method  : {}", str[n - 1].timeoffset.method);
                xmlFree(att);
            }

            if ((att = xmlGetProp(cur, (xmlChar*)"start")) != nullptr) {
                if (strlen((char*)att) > 0) {
                    str[n - 1].timeoffset.offset = (double)atof((char*)att);
                    UDA_LOG(UDA_LOG_DEBUG, "Start Time  : {}", str[n - 1].timeoffset.offset);
                }
                xmlFree(att);
            }

            if ((att = xmlGetProp(cur, (xmlChar*)"interval")) != nullptr) {
                if (strlen((char*)att) > 0) {
                    str[n - 1].timeoffset.interval = (double)atof((char*)att);
                    UDA_LOG(UDA_LOG_DEBUG, "Time Interval: {}", str[n - 1].timeoffset.interval);
                }
                xmlFree(att);
            }

            if ((att = xmlGetProp(cur, (xmlChar*)"scale")) != nullptr) {
                if (strlen((char*)att) > 0) {
                    str[n - 1].timeoffset.offset = deScale((char*)att) * str[n - 1].timeoffset.offset;
                }
                UDA_LOG(UDA_LOG_DEBUG, "Scaled Time Offset  : {}", str[n - 1].timeoffset.offset);
                xmlFree(att);
            }
        }
        cur = cur->next;
    }
    actions->nactions = n; // Number of Tags Found
    actions->action = str; // Array of Actions bounded by a Ranges
}

void parseCompositeSubset(xmlDocPtr doc, xmlNodePtr cur, Composite* comp)
{

    xmlChar* att; // General Input of tag attribute values

    Subset* str = comp->subsets;
    int n = 0; // Counter
    int n0, n1;

    // Attributes

    cur = cur->xmlChildrenNode;
    while (cur != nullptr) {
        UDA_LOG(UDA_LOG_DEBUG, "parseCompositeSubset: {}", (char*)cur->name);
        if ((!xmlStrcmp(cur->name, (const xmlChar*)"subset"))) {
            n++;
            str = (Subset*)realloc((void*)str, n * sizeof(Subset));

            init_subset(&str[n - 1]);

            // Attributes

            if ((att = xmlGetProp(cur, (xmlChar*)"data")) != nullptr) {
                if (strlen((char*)att) > 0) {
                    strcpy(str[n - 1].data_signal, (char*)att);
                }
                UDA_LOG(UDA_LOG_DEBUG, "Subset Signal: {}", str[n - 1].data_signal);
                xmlFree(att);
            }

            if ((att = xmlGetProp(cur, (xmlChar*)"reform")) != nullptr) {
                if (att[0] == 'Y' || att[0] == 'y') {
                    str[n - 1].reform = 1;
                }
                xmlFree(att);
            }

            if ((att = xmlGetProp(cur, (xmlChar*)"member")) != nullptr) {
                if (strlen((char*)att) > 0) {
                    strcpy(str[n - 1].member, (char*)att);
                }
                UDA_LOG(UDA_LOG_DEBUG, "Subset member: {}", str[n - 1].member);
                xmlFree(att);
            }

            if ((att = xmlGetProp(cur, (xmlChar*)"function")) != nullptr) {
                if (strlen((char*)att) > 0) {
                    strcpy(str[n - 1].function, (char*)att);
                }
                UDA_LOG(UDA_LOG_DEBUG, "Subset function: {}", str[n - 1].function);
                xmlFree(att);
            }

            if ((att = xmlGetProp(cur, (xmlChar*)"order")) != nullptr) {
                if (strlen((char*)att) > 0) {
                    str[n - 1].order = atoi((char*)att);
                }
                UDA_LOG(UDA_LOG_DEBUG, "Subset order: {}", str[n - 1].order);
                xmlFree(att);
            }

            // Fixed Length Attribute Arrays

            parse_fixed_length_str_array(cur, "operation", str[n - 1].operation, &str[n - 1].nbound);
            for (int i = 0; i < str[n - 1].nbound; i++) {
                str[n - 1].dimid[i] = i; // Ordering is as DATA[4][3][2][1][0]
            }

            parse_fixed_length_array(cur, "bound", (void*)str[n - 1].bound, UDA_TYPE_DOUBLE, &n0);
            parse_fixed_length_array(cur, "dimid", (void*)str[n - 1].dimid, UDA_TYPE_INT, &n1);

            if (parse_operation(&str[n - 1]) != 0) {
                return;
            }

            for (int i = 0; i < str[n - 1].nbound; i++) {
                UDA_LOG(UDA_LOG_DEBUG, "Subsetting Bounding Values : {}", str[n - 1].bound[i]);
                UDA_LOG(UDA_LOG_DEBUG, "Subsetting Operation       : {}", str[n - 1].operation[i]);
                UDA_LOG(UDA_LOG_DEBUG, "Dimension ID               : {}", str[n - 1].dimid[i]);
                UDA_LOG(UDA_LOG_DEBUG, "Subsetting Is Index?       : {}", str[n - 1].isindex[i]);
                UDA_LOG(UDA_LOG_DEBUG, "Subsetting Lower Index     : {}", (int)str[n - 1].lbindex[i].value);
                UDA_LOG(UDA_LOG_DEBUG, "Subsetting Upper Index     : {}", (int)str[n - 1].ubindex[i].value);
            }
        }
        cur = cur->next;
    }
    comp->nsubsets = n;  // Number of Subsets
    comp->subsets = str; // Array of Subset Actions
}

void parseMaps(xmlDocPtr doc, xmlNodePtr cur, Composite* comp) {}

void parseDimComposite(xmlDocPtr doc, xmlNodePtr cur, Composite* comp)
{

    xmlChar* att; // General Input of tag attribute values

    Dimension* str = comp->dimensions;
    int n = 0; // Counter

    cur = cur->xmlChildrenNode;
    while (cur != nullptr) {
        UDA_LOG(UDA_LOG_DEBUG, "parseDimComposite: {}", (char*)cur->name);
        if ((!xmlStrcmp(cur->name, (const xmlChar*)"composite_dim"))) {
            n++;
            str = (Dimension*)realloc((void*)str, n * sizeof(Dimension));

            init_dimension(&str[n - 1]);
            str[n - 1].dimType = UDA_DIM_COMPOSITE_TYPE;
            init_dim_composite(&str[n - 1].dimcomposite);

            // Attributes

            if ((att = xmlGetProp(cur, (xmlChar*)"to_dim")) != nullptr) { // Target Dimension
                if (strlen((char*)att) > 0) {
                    str[n - 1].dimid = atoi((char*)att); // Duplicate these tags for convenience
                    str[n - 1].dimcomposite.to_dim = atoi((char*)att);
                }
                UDA_LOG(UDA_LOG_DEBUG, "To Dimension  : {}", str[n - 1].dimid);
                xmlFree(att);
            }

            if ((att = xmlGetProp(cur, (xmlChar*)"from_dim")) !=
                nullptr) { // Swap with this Dimension otherwise swap with Data
                if (strlen((char*)att) > 0) {
                    str[n - 1].dimcomposite.from_dim = atoi((char*)att);
                }
                UDA_LOG(UDA_LOG_DEBUG, "From Dimension  : {}", str[n - 1].dimcomposite.from_dim);
                xmlFree(att);
            }

            if ((att = xmlGetProp(cur, (xmlChar*)"dim")) != nullptr ||
                (att = xmlGetProp(cur, (xmlChar*)"data")) != nullptr) {
                if (strlen((char*)att) > 0) {
                    strcpy(str[n - 1].dimcomposite.dim_signal, (char*)att);
                }
                UDA_LOG(UDA_LOG_DEBUG, "Dimension Signal  : {}", str[n - 1].dimcomposite.dim_signal);
                xmlFree(att);
            }

            if ((att = xmlGetProp(cur, (xmlChar*)"error")) != nullptr) {
                if (strlen((char*)att) > 0) {
                    strcpy(str[n - 1].dimcomposite.dim_error, (char*)att);
                }
                UDA_LOG(UDA_LOG_DEBUG, "Error Signal  : {}", str[n - 1].dimcomposite.dim_error);
                xmlFree(att);
            }
            if ((att = xmlGetProp(cur, (xmlChar*)"aserror")) != nullptr) {
                if (strlen((char*)att) > 0) {
                    strcpy(str[n - 1].dimcomposite.dim_aserror, (char*)att);
                }
                UDA_LOG(UDA_LOG_DEBUG, "Error Signal  : {}", str[n - 1].dimcomposite.dim_aserror);
                xmlFree(att);
            }
            if ((att = xmlGetProp(cur, (xmlChar*)"file")) != nullptr) {
                if (strlen((char*)att) > 0) {
                    strcpy(str[n - 1].dimcomposite.file, (char*)att);
                }
                UDA_LOG(UDA_LOG_DEBUG, "Dimension Source File: {}", str[n - 1].dimcomposite.file);
                xmlFree(att);
            }
            if ((att = xmlGetProp(cur, (xmlChar*)"format")) != nullptr) {
                if (strlen((char*)att) > 0) {
                    strcpy(str[n - 1].dimcomposite.format, (char*)att);
                }
                UDA_LOG(UDA_LOG_DEBUG, "Dimension Source File Format: {}", str[n - 1].dimcomposite.format);
                xmlFree(att);
            }
        }
        cur = cur->next;
    }
    comp->ndimensions = n;  // Number of Tags Found
    comp->dimensions = str; // Array of Composite Signal Actions on Dimensions
}

void parseComposite(xmlDocPtr doc, xmlNodePtr cur, Actions* actions)
{

    xmlChar* att; // General Input of tag attribute values

    Action* str = actions->action;
    int n = actions->nactions; // Counter

    cur = cur->xmlChildrenNode;
    while (cur != nullptr) {
        UDA_LOG(UDA_LOG_DEBUG, "parseComposite: {}", (char*)cur->name);
        if ((!xmlStrcmp(cur->name, (const xmlChar*)"composite"))) {
            n++;
            str = (Action*)realloc((void*)str, n * sizeof(Action));

            init_action(&str[n - 1]);
            str[n - 1].actionType = UDA_COMPOSITE_TYPE;
            init_composite(&str[n - 1].composite);

            // Attributes

            if ((att = xmlGetProp(cur, (xmlChar*)"id")) != nullptr) {
                if (strlen((char*)att) > 0) {
                    str[n - 1].actionId = atoi((char*)att);
                }
                UDA_LOG(UDA_LOG_DEBUG, "Action ID: {}", str[n - 1].actionId);
                xmlFree(att);
            }

            if ((att = xmlGetProp(cur, (xmlChar*)"exp_number_start")) != nullptr) {
                if (strlen((char*)att) > 0) {
                    str[n - 1].exp_range[0] = atoi((char*)att);
                }
                UDA_LOG(UDA_LOG_DEBUG, "Exp Number Range Start: {}", str[n - 1].exp_range[0]);
                xmlFree(att);
            }

            if ((att = xmlGetProp(cur, (xmlChar*)"exp_number_end")) != nullptr) {
                if (strlen((char*)att) > 0) {
                    str[n - 1].exp_range[1] = atoi((char*)att);
                }
                UDA_LOG(UDA_LOG_DEBUG, "Exp Number Range End : {}", str[n - 1].exp_range[1]);
                xmlFree(att);
            }

            if ((att = xmlGetProp(cur, (xmlChar*)"pass_start")) != nullptr) {
                if (strlen((char*)att) > 0) {
                    str[n - 1].pass_range[0] = atoi((char*)att);
                }
                UDA_LOG(UDA_LOG_DEBUG, "Pass Number Range Start: {}", str[n - 1].pass_range[0]);
                xmlFree(att);
            }

            if ((att = xmlGetProp(cur, (xmlChar*)"pass_end")) != nullptr) {
                if (strlen((char*)att) > 0) {
                    str[n - 1].pass_range[1] = atoi((char*)att);
                }
                UDA_LOG(UDA_LOG_DEBUG, "Pass Number Range End  : {}", str[n - 1].pass_range[1]);
                xmlFree(att);
            }

            if ((att = xmlGetProp(cur, (xmlChar*)"data")) != nullptr) {
                if (strlen((char*)att) > 0) {
                    strcpy(str[n - 1].composite.data_signal, (char*)att);
                }
                UDA_LOG(UDA_LOG_DEBUG, "Data Signal  : {}", str[n - 1].composite.data_signal);
                xmlFree(att);
            }

            if ((att = xmlGetProp(cur, (xmlChar*)"file")) != nullptr) {
                if (strlen((char*)att) > 0) {
                    strcpy(str[n - 1].composite.file, (char*)att);
                }
                UDA_LOG(UDA_LOG_DEBUG, "Data Source File: {}", str[n - 1].composite.file);
                xmlFree(att);
            }

            if ((att = xmlGetProp(cur, (xmlChar*)"format")) != nullptr) {
                if (strlen((char*)att) > 0) {
                    strcpy(str[n - 1].composite.format, (char*)att);
                }
                UDA_LOG(UDA_LOG_DEBUG, "Source File Format: {}", str[n - 1].composite.format);
                xmlFree(att);
            }

            if ((att = xmlGetProp(cur, (xmlChar*)"error")) != nullptr) {
                if (strlen((char*)att) > 0) {
                    strcpy(str[n - 1].composite.error_signal, (char*)att);
                }
                UDA_LOG(UDA_LOG_DEBUG, "Error Signal  : {}", str[n - 1].composite.error_signal);
                xmlFree(att);
            }
            if ((att = xmlGetProp(cur, (xmlChar*)"aserror")) != nullptr) {
                if (strlen((char*)att) > 0) {
                    strcpy(str[n - 1].composite.aserror_signal, (char*)att);
                }
                UDA_LOG(UDA_LOG_DEBUG, "Error Signal  : {}", str[n - 1].composite.aserror_signal);
                xmlFree(att);
            }
            if ((att = xmlGetProp(cur, (xmlChar*)"mapto")) != nullptr) {
                if (strlen((char*)att) > 0) {
                    strcpy(str[n - 1].composite.aserror_signal, (char*)att);
                }
                UDA_LOG(UDA_LOG_DEBUG, "Map to Signal  : {}", str[n - 1].composite.map_to_signal);
                xmlFree(att);
            }

            if ((att = xmlGetProp(cur, (xmlChar*)"order")) != nullptr) {
                if (strlen((char*)att) > 0) {
                    str[n - 1].composite.order = atoi((char*)att);
                }
                UDA_LOG(UDA_LOG_DEBUG, "Time Dimension: {}", str[n - 1].composite.order);
                xmlFree(att);
            }

            // Child Tags

            parseDimComposite(doc, cur, &str[n - 1].composite);
            parseCompositeSubset(doc, cur, &str[n - 1].composite);

            // Consolidate Composite Signal name with Subset Signal Name (the Composite record has precedence)

            if (str[n - 1].composite.nsubsets > 0 && strlen(str[n - 1].composite.data_signal) == 0) {
                if (strlen(str[n - 1].composite.subsets[0].data_signal) > 0) {
                    strcpy(str[n - 1].composite.data_signal, str[n - 1].composite.subsets[0].data_signal);
                }
            }
        }
        cur = cur->next;
    }

    actions->nactions = n; // Number of Tags Found
    actions->action = str; // Array of Actions bounded by a Ranges
}

void parseDimErrorModel(xmlDocPtr doc, xmlNodePtr cur, ErrorModel* mod)
{
    xmlChar* att; // General Input of tag attribute values
    float* params;

    Dimension* str = mod->dimensions;
    int n = 0; // Counter

    cur = cur->xmlChildrenNode;
    while (cur != nullptr) {
        UDA_LOG(UDA_LOG_DEBUG, "parseDimErrorModel: {}", (char*)cur->name);
        if ((!xmlStrcmp(cur->name, (const xmlChar*)"dimension"))) {
            n++;
            str = (Dimension*)realloc((void*)str, n * sizeof(Dimension));

            init_dimension(&str[n - 1]);
            str[n - 1].dimType = UDA_DIM_ERROR_MODEL_TYPE;
            init_dim_error_model(&str[n - 1].dimerrormodel);

            // Attributes

            if ((att = xmlGetProp(cur, (xmlChar*)"dimid")) != nullptr) { // Target Dimension
                if (strlen((char*)att) > 0) {
                    str[n - 1].dimid = atoi((char*)att);
                }
                UDA_LOG(UDA_LOG_DEBUG, "Dimension : {}", str[n - 1].dimid);
                xmlFree(att);
            }
            if ((att = xmlGetProp(cur, (xmlChar*)"model")) != nullptr) { // Error Model
                if (strlen((char*)att) > 0) {
                    str[n - 1].dimerrormodel.model = atoi((char*)att);
                }
                UDA_LOG(UDA_LOG_DEBUG, "Model : {}", str[n - 1].dimerrormodel.model);
                xmlFree(att);
            }

            // Child Tags

            params = parse_float_array(doc, cur, "params", &str[n - 1].dimerrormodel.param_n);
            if (params != nullptr) {
                if (str[n - 1].dimerrormodel.param_n > MAXERRPARAMS) {
                    str[n - 1].dimerrormodel.param_n = MAXERRPARAMS;
                }
                for (int i = 0; i < str[n - 1].dimerrormodel.param_n; i++) {
                    str[n - 1].dimerrormodel.params[i] = params[i];
                }
                free(params);
            }
        }
        cur = cur->next;
    }
    mod->ndimensions = n;  // Number of Tags Found
    mod->dimensions = str; // Array of Error Model Actions on Dimensions
}

void parse_error_model(xmlDocPtr doc, xmlNodePtr cur, Actions* actions)
{
    xmlChar* att; // General Input of tag attribute values
    float* params;

    Action* str = actions->action;
    int n = actions->nactions; // Counter

    cur = cur->xmlChildrenNode;
    while (cur != nullptr) {
        UDA_LOG(UDA_LOG_DEBUG, "parseErrorModel: {}", (char*)cur->name);
        if ((!xmlStrcmp(cur->name, (const xmlChar*)"errormodel"))) {
            n++;
            str = (Action*)realloc((void*)str, n * sizeof(Action));

            init_action(&str[n - 1]);
            str[n - 1].actionType = UDA_ERROR_MODEL_TYPE;
            init_error_model(&str[n - 1].errormodel);

            // Attributes

            if ((att = xmlGetProp(cur, (xmlChar*)"id")) != nullptr) {
                if (strlen((char*)att) > 0) {
                    str[n - 1].actionId = atoi((char*)att);
                }
                UDA_LOG(UDA_LOG_DEBUG, "Action ID: {}", str[n - 1].actionId);
                xmlFree(att);
            }

            if ((att = xmlGetProp(cur, (xmlChar*)"exp_number_start")) != nullptr) {
                if (strlen((char*)att) > 0) {
                    str[n - 1].exp_range[0] = atoi((char*)att);
                }
                UDA_LOG(UDA_LOG_DEBUG, "Exp Number Range Start: {}", str[n - 1].exp_range[0]);
                xmlFree(att);
            }

            if ((att = xmlGetProp(cur, (xmlChar*)"exp_number_end")) != nullptr) {
                if (strlen((char*)att) > 0) {
                    str[n - 1].exp_range[1] = atoi((char*)att);
                }
                UDA_LOG(UDA_LOG_DEBUG, "Exp Number Range End : {}", str[n - 1].exp_range[1]);
                xmlFree(att);
            }

            if ((att = xmlGetProp(cur, (xmlChar*)"pass_start")) != nullptr) {
                if (strlen((char*)att) > 0) {
                    str[n - 1].pass_range[0] = atoi((char*)att);
                }
                UDA_LOG(UDA_LOG_DEBUG, "Pass Number Range Start: {}", str[n - 1].pass_range[0]);
                xmlFree(att);
            }

            if ((att = xmlGetProp(cur, (xmlChar*)"pass_end")) != nullptr) {
                if (strlen((char*)att) > 0) {
                    str[n - 1].pass_range[1] = atoi((char*)att);
                }
                UDA_LOG(UDA_LOG_DEBUG, "Pass Number Range End  : {}", str[n - 1].pass_range[1]);
                xmlFree(att);
            }

            if ((att = xmlGetProp(cur, (xmlChar*)"model")) != nullptr) {
                if (strlen((char*)att) > 0) {
                    str[n - 1].errormodel.model = atoi((char*)att);
                }
                UDA_LOG(UDA_LOG_DEBUG, "Error Distribution Model: {}", str[n - 1].errormodel.model);
                xmlFree(att);
            }

            // Child Tags

            params = parse_float_array(doc, cur, "params", &str[n - 1].errormodel.param_n);
            if (params != nullptr) {
                if (str[n - 1].errormodel.param_n > MAXERRPARAMS) {
                    str[n - 1].errormodel.param_n = MAXERRPARAMS;
                }
                for (int i = 0; i < str[n - 1].errormodel.param_n; i++) {
                    str[n - 1].errormodel.params[i] = params[i];
                }
                free(params);
            }

            parseDimErrorModel(doc, cur, &str[n - 1].errormodel);
        }
        cur = cur->next;
    }
    actions->nactions = n; // Number of Tags Found
    actions->action = str; // Array of Actions bounded by a Ranges
}

void parseDimDocumentation(xmlDocPtr doc, xmlNodePtr cur, Documentation* document)
{
    xmlChar* att; // General Input of tag attribute values

    Dimension* str = document->dimensions;
    int n = 0; // Counter

    cur = cur->xmlChildrenNode;
    while (cur != nullptr) {
        UDA_LOG(UDA_LOG_DEBUG, "parseDimDocumentation: {}", (char*)cur->name);
        if ((!xmlStrcmp(cur->name, (const xmlChar*)"dimension"))) {
            n++;
            str = (Dimension*)realloc((void*)str, n * sizeof(Dimension));

            init_dimension(&str[n - 1]);
            str[n - 1].dimType = UDA_DIM_DOCUMENTATION_TYPE;
            init_dim_documentation(&str[n - 1].dimdocumentation);

            // Attributes

            if ((att = xmlGetProp(cur, (xmlChar*)"dimid")) != nullptr) { // Target Dimension
                if (strlen((char*)att) > 0) {
                    str[n - 1].dimid = atoi((char*)att);
                }
                UDA_LOG(UDA_LOG_DEBUG, "To Dimension  : {}", str[n - 1].dimid);
                xmlFree(att);
            }

            // Child Tags

            parse_target_string(doc, cur, "label", str[n - 1].dimdocumentation.label);
            parse_target_string(doc, cur, "units", str[n - 1].dimdocumentation.units);
        }
        cur = cur->next;
    }
    document->ndimensions = n;  // Number of Tags Found
    document->dimensions = str; // Array of Composite Signal Actions on Dimensions
}

void parse_documentation(xmlDocPtr doc, xmlNodePtr cur, Actions* actions)
{
    xmlChar* att; // General Input of tag attribute values

    Action* str = actions->action;
    int n = actions->nactions; // Counter

    cur = cur->xmlChildrenNode;
    while (cur != nullptr) {
        UDA_LOG(UDA_LOG_DEBUG, "parseDocumentation: {}", (char*)cur->name);
        if ((!xmlStrcmp(cur->name, (const xmlChar*)"documentation"))) {
            n++;
            str = (Action*)realloc((void*)str, n * sizeof(Action));

            init_action(&str[n - 1]);
            str[n - 1].actionType = UDA_DOCUMENTATION_TYPE;
            init_documentation(&str[n - 1].documentation);

            // Attributes

            if ((att = xmlGetProp(cur, (xmlChar*)"id")) != nullptr) {
                if (strlen((char*)att) > 0) {
                    str[n - 1].actionId = atoi((char*)att);
                }
                UDA_LOG(UDA_LOG_DEBUG, "Action ID: {}", str[n - 1].actionId);
                xmlFree(att);
            }

            if ((att = xmlGetProp(cur, (xmlChar*)"exp_number_start")) != nullptr) {
                if (strlen((char*)att) > 0) {
                    str[n - 1].exp_range[0] = atoi((char*)att);
                }
                UDA_LOG(UDA_LOG_DEBUG, "Exp Number Range Start: {}", str[n - 1].exp_range[0]);
                xmlFree(att);
            }

            if ((att = xmlGetProp(cur, (xmlChar*)"exp_number_end")) != nullptr) {
                if (strlen((char*)att) > 0) {
                    str[n - 1].exp_range[1] = atoi((char*)att);
                }
                UDA_LOG(UDA_LOG_DEBUG, "Exp Number Range End : {}", str[n - 1].exp_range[1]);
                xmlFree(att);
            }

            if ((att = xmlGetProp(cur, (xmlChar*)"pass_start")) != nullptr) {
                if (strlen((char*)att) > 0) {
                    str[n - 1].pass_range[0] = atoi((char*)att);
                }
                UDA_LOG(UDA_LOG_DEBUG, "Pass Number Range Start: {}", str[n - 1].pass_range[0]);
                xmlFree(att);
            }

            if ((att = xmlGetProp(cur, (xmlChar*)"pass_end")) != nullptr) {
                if (strlen((char*)att) > 0) {
                    str[n - 1].pass_range[1] = atoi((char*)att);
                }
                UDA_LOG(UDA_LOG_DEBUG, "Pass Number Range End  : {}", str[n - 1].pass_range[1]);
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
    actions->nactions = n; // Number of Tags Found
    actions->action = str; // Array of Actions bounded by a Ranges
}

void parseDimCalibration(xmlDocPtr doc, xmlNodePtr cur, Calibration* cal)
{
    xmlChar* att; // General Input of tag attribute values

    Dimension* str = cal->dimensions;
    int n = 0; // Counter

    cur = cur->xmlChildrenNode;
    while (cur != nullptr) {
        UDA_LOG(UDA_LOG_DEBUG, "parseDimCalibration: {}", (char*)cur->name);
        if ((!xmlStrcmp(cur->name, (const xmlChar*)"dimension"))) {
            n++;
            str = (Dimension*)realloc((void*)str, n * sizeof(Dimension));

            init_dimension(&str[n - 1]);
            str[n - 1].dimType = UDA_DIM_CALIBRATION_TYPE;
            init_dim_calibration(&str[n - 1].dimcalibration);

            // Attributes

            if ((att = xmlGetProp(cur, (xmlChar*)"dimid")) != nullptr) { // Target Dimension
                if (strlen((char*)att) > 0) {
                    str[n - 1].dimid = atoi((char*)att);
                }
                UDA_LOG(UDA_LOG_DEBUG, "To Dimension  : {}", str[n - 1].dimid);
                xmlFree(att);
            }

            if ((att = xmlGetProp(cur, (xmlChar*)"invert")) != nullptr) {
                if (att[0] == 'y' || att[0] == 'Y') {
                    str[n - 1].dimcalibration.invert = 1;
                }
                UDA_LOG(UDA_LOG_DEBUG, "Calibration Invert: {}", str[n - 1].dimcalibration.invert);
                xmlFree(att);
            }

            // Child Tags

            parse_target_string(doc, cur, "units", str[n - 1].dimcalibration.units);
            parse_target_value(doc, cur, "factor", &str[n - 1].dimcalibration.factor);
            parse_target_value(doc, cur, "offset", &str[n - 1].dimcalibration.offset);

            UDA_LOG(UDA_LOG_DEBUG, "Dimension Units               : {}", str[n - 1].dimcalibration.units);
            UDA_LOG(UDA_LOG_DEBUG, "Dimension Calibration Factor  : {}", str[n - 1].dimcalibration.factor);
            UDA_LOG(UDA_LOG_DEBUG, "Dimension Calibration Offset  : {}", str[n - 1].dimcalibration.offset);
        }
        cur = cur->next;
    }
    cal->ndimensions = n;  // Number of Tags Found
    cal->dimensions = str; // Array of Composite Signal Actions on Dimensions
}

void parse_calibration(xmlDocPtr doc, xmlNodePtr cur, Actions* actions)
{
    xmlChar* att; // General Input of tag attribute values

    Action* str = actions->action;
    int n = actions->nactions; // Counter

    cur = cur->xmlChildrenNode;
    while (cur != nullptr) {
        UDA_LOG(UDA_LOG_DEBUG, "parseCalibration: {}", (char*)cur->name);
        if ((!xmlStrcmp(cur->name, (const xmlChar*)"calibration"))) {
            n++;
            str = (Action*)realloc((void*)str, n * sizeof(Action));

            init_action(&str[n - 1]);
            str[n - 1].actionType = UDA_CALIBRATION_TYPE;
            init_calibration(&str[n - 1].calibration);

            // Attributes

            if ((att = xmlGetProp(cur, (xmlChar*)"id")) != nullptr) {
                if (strlen((char*)att) > 0) {
                    str[n - 1].actionId = atoi((char*)att);
                }
                UDA_LOG(UDA_LOG_DEBUG, "Action ID: {}", str[n - 1].actionId);
                xmlFree(att);
            }

            if ((att = xmlGetProp(cur, (xmlChar*)"exp_number_start")) != nullptr) {
                if (strlen((char*)att) > 0) {
                    str[n - 1].exp_range[0] = atoi((char*)att);
                }
                UDA_LOG(UDA_LOG_DEBUG, "Exp Number Range Start: {}", str[n - 1].exp_range[0]);
                xmlFree(att);
            }

            if ((att = xmlGetProp(cur, (xmlChar*)"exp_number_end")) != nullptr) {
                if (strlen((char*)att) > 0) {
                    str[n - 1].exp_range[1] = atoi((char*)att);
                }
                UDA_LOG(UDA_LOG_DEBUG, "Exp Number Range End : {}", str[n - 1].exp_range[1]);
                xmlFree(att);
            }

            if ((att = xmlGetProp(cur, (xmlChar*)"pass_start")) != nullptr) {
                if (strlen((char*)att) > 0) {
                    str[n - 1].pass_range[0] = atoi((char*)att);
                }
                UDA_LOG(UDA_LOG_DEBUG, "Pass Number Range Start: {}", str[n - 1].pass_range[0]);
                xmlFree(att);
            }

            if ((att = xmlGetProp(cur, (xmlChar*)"pass_end")) != nullptr) {
                if (strlen((char*)att) > 0) {
                    str[n - 1].pass_range[1] = atoi((char*)att);
                }
                UDA_LOG(UDA_LOG_DEBUG, "Pass Number Range End  : {}", str[n - 1].pass_range[1]);
                xmlFree(att);
            }

            if ((att = xmlGetProp(cur, (xmlChar*)"target")) != nullptr) {
                if (strlen((char*)att) > 0) {
                    strcpy(str[n - 1].calibration.target, (char*)att);
                }
                UDA_LOG(UDA_LOG_DEBUG, "Calibration Target: {}", str[n - 1].calibration.target);
                xmlFree(att);
            }

            if ((att = xmlGetProp(cur, (xmlChar*)"invert")) != nullptr) {
                if (att[0] == 'y' || att[0] == 'Y') {
                    str[n - 1].calibration.invert = 1;
                }
                UDA_LOG(UDA_LOG_DEBUG, "Calibration Invert: {}", str[n - 1].calibration.invert);
                xmlFree(att);
            }

            // Child Tags

            parse_target_string(doc, cur, "units", str[n - 1].calibration.units);
            parse_target_value(doc, cur, "factor", &str[n - 1].calibration.factor);
            parse_target_value(doc, cur, "offset", &str[n - 1].calibration.offset);

            UDA_LOG(UDA_LOG_DEBUG, "Data Units               : {}", str[n - 1].calibration.units);
            UDA_LOG(UDA_LOG_DEBUG, "Data Calibration Factor  : {}", str[n - 1].calibration.factor);
            UDA_LOG(UDA_LOG_DEBUG, "Data Calibration Offset  : {}", str[n - 1].calibration.offset);

            parseDimCalibration(doc, cur, &str[n - 1].calibration);
        }
        cur = cur->next;
    }
    actions->nactions = n; // Number of Tags Found
    actions->action = str; // Array of Actions bounded by a Ranges
}

void parse_subset(xmlDocPtr doc, xmlNodePtr cur, Actions* actions)
{
    xmlChar* att; // General Input of tag attribute values

    Action* str = actions->action;
    Subset* sub = nullptr;
    int n = actions->nactions; // Counter
    int n0, n1;

    cur = cur->xmlChildrenNode;
    while (cur != nullptr) {
        UDA_LOG(UDA_LOG_DEBUG, "parseSubset: {}", (char*)cur->name);
        if ((!xmlStrcmp(cur->name, (const xmlChar*)"subset"))) {
            n++;
            str = (Action*)realloc((void*)str, n * sizeof(Action));

            init_action(&str[n - 1]);
            str[n - 1].actionType = UDA_SUBSET_TYPE;
            sub = &str[n - 1].subset;
            init_subset(sub);

            // Attributes

            if ((att = xmlGetProp(cur, (xmlChar*)"id")) != nullptr) {
                if (strlen((char*)att) > 0) {
                    str[n - 1].actionId = atoi((char*)att);
                }
                UDA_LOG(UDA_LOG_DEBUG, "Action ID: {}", str[n - 1].actionId);
                xmlFree(att);
            }

            if ((att = xmlGetProp(cur, (xmlChar*)"data")) != nullptr) {
                if (strlen((char*)att) > 0) {
                    strcpy(sub->data_signal, (char*)att);
                }
                UDA_LOG(UDA_LOG_DEBUG, "Data Signal  : {}", sub->data_signal);
                xmlFree(att);
            }

            // Child Tags

            // Attributes

            if ((att = xmlGetProp(cur, (xmlChar*)"reform")) != nullptr) {
                if (att[0] == 'Y' || att[0] == 'y') {
                    sub->reform = 1;
                }
                xmlFree(att);
            }

            if ((att = xmlGetProp(cur, (xmlChar*)"member")) != nullptr) {
                if (strlen((char*)att) > 0) {
                    strcpy(sub->member, (char*)att);
                }
                UDA_LOG(UDA_LOG_DEBUG, "Subset Member: {}", sub->member);
                xmlFree(att);
            }

            if ((att = xmlGetProp(cur, (xmlChar*)"function")) != nullptr) {
                if (strlen((char*)att) > 0) {
                    strcpy(sub->function, (char*)att);
                }
                UDA_LOG(UDA_LOG_DEBUG, "Subset function: {}", sub->function);
                xmlFree(att);
            }

            if ((att = xmlGetProp(cur, (xmlChar*)"order")) != nullptr) {
                if (strlen((char*)att) > 0) {
                    sub->order = atoi((char*)att);
                }
                UDA_LOG(UDA_LOG_DEBUG, "Subset order: {}", sub->order);
                xmlFree(att);
            }

            // Fixed Length Attribute Arrays

            parse_fixed_length_str_array(cur, "operation", &sub->operation[0], &sub->nbound);
            for (int i = 0; i < sub->nbound; i++) {
                sub->dimid[i] = i; // Ordering is as DATA[4][3][2][1][0]
            }

            parse_fixed_length_array(cur, "bound", (void*)sub->bound, UDA_TYPE_DOUBLE, &n0);
            parse_fixed_length_array(cur, "dimid", (void*)sub->dimid, UDA_TYPE_INT, &n1);

            if (parse_operation(sub) != 0) {
                return;
            }

            for (int i = 0; i < sub->nbound; i++) {
                UDA_LOG(UDA_LOG_DEBUG, "Dimension ID               : {}", sub->dimid[i]);
                UDA_LOG(UDA_LOG_DEBUG, "Subsetting Bounding Values : {}", sub->bound[i]);
                UDA_LOG(UDA_LOG_DEBUG, "Subsetting Operation       : {}", sub->operation[i]);
                UDA_LOG(UDA_LOG_DEBUG, "Subsetting Is Index?       : {}", sub->isindex[i]);
                UDA_LOG(UDA_LOG_DEBUG, "Subsetting Lower Index     : {}", (int)sub->lbindex[i].value);
                UDA_LOG(UDA_LOG_DEBUG, "Subsetting Upper Index     : {}", (int)sub->ubindex[i].value);
            }
        }
        cur = cur->next;
    }

    actions->nactions = n; // Number of Tags Found
    actions->action = str; // Array of Actions bounded by a Ranges
}

int uda::client_server::parse_doc(char* docname, Actions* actions)
{
    xmlDocPtr doc;
    xmlNodePtr cur;

#  ifdef TIMETEST
    struct timeval tv_start;
    struct timeval tv_end;
    float testtime;
    int rc = gettimeofday(&tv_start, nullptr);
#  endif

    xmlInitParser();

    if ((doc = xmlParseDoc((xmlChar*)docname)) == nullptr) {
        xmlFreeDoc(doc);
        xmlCleanupParser();
        add_error(UDA_CODE_ERROR_TYPE, "parseDoc", 1, "XML Not Parsed");
        return 1;
    }

    if ((cur = xmlDocGetRootElement(doc)) == nullptr) {
        add_error(UDA_CODE_ERROR_TYPE, "parseDoc", 1, "Empty XML Document");
        xmlFreeDoc(doc);
        xmlCleanupParser();
        return 1;
    }

    if (xmlStrcmp(cur->name, (const xmlChar*)"action")) { // If No Action Tag then Nothing to be done!
        xmlFreeDoc(doc);
        xmlCleanupParser();
        return 1;
    }

    cur = cur->xmlChildrenNode;
    while (cur != nullptr) {

        if ((!xmlStrcmp(cur->name, (const xmlChar*)"signal"))) {

            parseComposite(doc, cur, actions); // Composite can have Subset as a child
            parse_documentation(doc, cur, actions);
            parse_calibration(doc, cur, actions);
            parse_time_offset(doc, cur, actions);
            parse_error_model(doc, cur, actions);

            parse_subset(doc, cur, actions); // Single Subset
        }
        cur = cur->next;
    }

    xmlFreeDoc(doc);
    xmlCleanupParser();

#  ifdef TIMETEST
    rc = gettimeofday(&tv_end, nullptr);
    testtime = (float)(tv_end.tv_sec - tv_start.tv_sec) * 1.0E6 + (float)(tv_end.tv_usec - tv_start.tv_usec);
#  endif

    return 0;
}
#endif
//==================================================================================================

void print_dimensions(int ndim, Dimension* dims)
{
    UDA_LOG(UDA_LOG_DEBUG, "No. Dimensions     : {}", ndim);
    for (int i = 0; i < ndim; i++) {
        UDA_LOG(UDA_LOG_DEBUG, "Dim id     : {}", dims[i].dimid);

        switch (dims[i].dimType) {

            case UDA_DIM_CALIBRATION_TYPE:
                UDA_LOG(UDA_LOG_DEBUG, "factor     : {}", dims[i].dimcalibration.factor);
                UDA_LOG(UDA_LOG_DEBUG, "Offset     : {}", dims[i].dimcalibration.offset);
                UDA_LOG(UDA_LOG_DEBUG, "Units      : {}", dims[i].dimcalibration.units);
                break;

            case UDA_DIM_COMPOSITE_TYPE:
                UDA_LOG(UDA_LOG_DEBUG, "to Dim       : {}", dims[i].dimcomposite.to_dim);
                UDA_LOG(UDA_LOG_DEBUG, "from Dim     : {}", dims[i].dimcomposite.from_dim);
                UDA_LOG(UDA_LOG_DEBUG, "Dim signal   : {}", dims[i].dimcomposite.dim_signal);
                UDA_LOG(UDA_LOG_DEBUG, "Dim Error    : {}", dims[i].dimcomposite.dim_error);
                UDA_LOG(UDA_LOG_DEBUG, "Dim ASError  : {}", dims[i].dimcomposite.dim_aserror);
                UDA_LOG(UDA_LOG_DEBUG, "Dim Source File  : {}", dims[i].dimcomposite.file);
                UDA_LOG(UDA_LOG_DEBUG, "Dim Source Format: {}", dims[i].dimcomposite.format);

                break;

            case UDA_DIM_DOCUMENTATION_TYPE:
                UDA_LOG(UDA_LOG_DEBUG, "Dim Label  : {}", dims[i].dimdocumentation.label);
                UDA_LOG(UDA_LOG_DEBUG, "Dim Units  : {}", dims[i].dimdocumentation.units);
                break;

            case UDA_DIM_ERROR_MODEL_TYPE:
                UDA_LOG(UDA_LOG_DEBUG, "Error Model Id            : {}", dims[i].dimerrormodel.model);
                UDA_LOG(UDA_LOG_DEBUG, "Number of Model Parameters: {}", dims[i].dimerrormodel.param_n);
                for (int j = 0; j < dims[i].dimerrormodel.param_n; j++) {
                    UDA_LOG(UDA_LOG_DEBUG, "Parameters[{}] = {}", j, dims[i].dimerrormodel.params[j]);
                }
                break;

            default:
                break;
        }
    }
}

void uda::client_server::print_action(Action action)
{
    UDA_LOG(UDA_LOG_DEBUG, "Action XML Id    : {}", action.actionId);
    UDA_LOG(UDA_LOG_DEBUG, "Action Type      : {}", action.actionType);
    UDA_LOG(UDA_LOG_DEBUG, "In Range?        : {}", action.inRange);
    UDA_LOG(UDA_LOG_DEBUG, "Exp Number Range : {} -> {}", action.exp_range[0], action.exp_range[1]);
    UDA_LOG(UDA_LOG_DEBUG, "Pass Number Range: {} -> {}", action.pass_range[0], action.pass_range[1]);

    switch (action.actionType) {
        case UDA_TIME_OFFSET_TYPE:
            UDA_LOG(UDA_LOG_DEBUG, "TimeOffset xml");
            UDA_LOG(UDA_LOG_DEBUG, "Method         : {}", action.timeoffset.method);
            UDA_LOG(UDA_LOG_DEBUG, "Time Offset    : {}", action.timeoffset.offset);
            UDA_LOG(UDA_LOG_DEBUG, "Time Interval  : {}", action.timeoffset.interval);
            break;
        case UDA_DOCUMENTATION_TYPE:
            UDA_LOG(UDA_LOG_DEBUG, "Documentation xml");
            UDA_LOG(UDA_LOG_DEBUG, "Description: {}", action.documentation.description);
            UDA_LOG(UDA_LOG_DEBUG, "Data Label : {}", action.documentation.label);
            UDA_LOG(UDA_LOG_DEBUG, "Data Units : {}", action.documentation.units);
            print_dimensions(action.documentation.ndimensions, action.documentation.dimensions);
            break;
        case UDA_CALIBRATION_TYPE:
            UDA_LOG(UDA_LOG_DEBUG, "Calibration xml");
            UDA_LOG(UDA_LOG_DEBUG, "Target     : {}", action.calibration.target);
            UDA_LOG(UDA_LOG_DEBUG, "Factor     : {}", action.calibration.factor);
            UDA_LOG(UDA_LOG_DEBUG, "Offset     : {}", action.calibration.offset);
            UDA_LOG(UDA_LOG_DEBUG, "Invert     : {}", action.calibration.invert);
            UDA_LOG(UDA_LOG_DEBUG, "Data Units : {}", action.calibration.units);
            print_dimensions(action.calibration.ndimensions, action.calibration.dimensions);
            break;
        case UDA_COMPOSITE_TYPE:
            UDA_LOG(UDA_LOG_DEBUG, "Composite xml");
            UDA_LOG(UDA_LOG_DEBUG, "Composite Data Signal    : {}", action.composite.data_signal);
            UDA_LOG(UDA_LOG_DEBUG, "Composite Error Signal   : {}", action.composite.error_signal);
            UDA_LOG(UDA_LOG_DEBUG, "Composite Asymmetric Error Signal   : {}", action.composite.aserror_signal);
            UDA_LOG(UDA_LOG_DEBUG, "Composite Map to Signal  : {}", action.composite.map_to_signal);
            UDA_LOG(UDA_LOG_DEBUG, "Composite Source File    : {}", action.composite.file);
            UDA_LOG(UDA_LOG_DEBUG, "Composite Source Format  : {}", action.composite.format);
            UDA_LOG(UDA_LOG_DEBUG, "Composite Time Dimension : {}", action.composite.order);
            print_dimensions(action.composite.ndimensions, action.composite.dimensions);
            break;
        case UDA_ERROR_MODEL_TYPE:
            UDA_LOG(UDA_LOG_DEBUG, "ErrorModel xml");
            UDA_LOG(UDA_LOG_DEBUG, "Error Model Id            : {}", action.errormodel.model);
            UDA_LOG(UDA_LOG_DEBUG, "Number of Model Parameters: {}", action.errormodel.param_n);
            for (int i = 0; i < action.errormodel.param_n; i++) {
                UDA_LOG(UDA_LOG_DEBUG, "Parameters[{}] = {}", i, action.errormodel.params[i]);
            }
            print_dimensions(action.errormodel.ndimensions, action.errormodel.dimensions);
            break;

        case UDA_SERVER_SIDE_TYPE:
            UDA_LOG(UDA_LOG_DEBUG, "ServerSide Actions");
            UDA_LOG(UDA_LOG_DEBUG, "Number of Serverside Subsets: {}", action.serverside.nsubsets);
            for (int i = 0; i < action.serverside.nsubsets; i++) {
                UDA_LOG(UDA_LOG_DEBUG, "Number of Subsetting Operations: {}", action.serverside.subsets[i].nbound);
                UDA_LOG(UDA_LOG_DEBUG, "Reform?                        : {}", action.serverside.subsets[i].reform);
                UDA_LOG(UDA_LOG_DEBUG, "Member                         : {}", action.serverside.subsets[i].member);
                UDA_LOG(UDA_LOG_DEBUG, "Function                       : {}", action.serverside.subsets[i].function);
                UDA_LOG(UDA_LOG_DEBUG, "Order                          : {}", action.serverside.subsets[i].order);
                UDA_LOG(UDA_LOG_DEBUG, "Signal                         : {}",
                        action.serverside.subsets[i].data_signal);
                for (int j = 0; j < action.serverside.subsets[i].nbound; j++) {
                    UDA_LOG(UDA_LOG_DEBUG, "Bounding Value: {}", action.serverside.subsets[i].bound[j]);
                    UDA_LOG(UDA_LOG_DEBUG, "Operation     : {}", action.serverside.subsets[i].operation[j]);
                    UDA_LOG(UDA_LOG_DEBUG, "Dimension ID  : {}", action.serverside.subsets[i].dimid[j]);
                }
            }
            UDA_LOG(UDA_LOG_DEBUG, "Number of Serverside mappings: {}", action.serverside.nmaps);
            break;
        case UDA_SUBSET_TYPE:
            UDA_LOG(UDA_LOG_DEBUG, "Subset Actions");
            UDA_LOG(UDA_LOG_DEBUG, "Number of Subsets: 1");
            UDA_LOG(UDA_LOG_DEBUG, "Number of Subsetting Operations: {}", action.subset.nbound);
            UDA_LOG(UDA_LOG_DEBUG, "Reform?                        : {}", action.subset.reform);
            UDA_LOG(UDA_LOG_DEBUG, "Member                         : {}", action.subset.member);
            UDA_LOG(UDA_LOG_DEBUG, "Function                       : {}", action.subset.function);
            UDA_LOG(UDA_LOG_DEBUG, "Order                       : {}", action.subset.order);
            UDA_LOG(UDA_LOG_DEBUG, "Signal                         : {}", action.subset.data_signal);
            for (int j = 0; j < action.subset.nbound; j++) {
                UDA_LOG(UDA_LOG_DEBUG, "Bounding Value: {}", action.subset.bound[j]);
                UDA_LOG(UDA_LOG_DEBUG, "Operation     : {}", action.subset.operation[j]);
                UDA_LOG(UDA_LOG_DEBUG, "Dimension ID  : {}", action.subset.dimid[j]);
            }
            break;
        default:
            break;
    }
}

void uda::client_server::print_actions(Actions actions)
{
    UDA_LOG(UDA_LOG_DEBUG, "No. Action Blocks: {}", actions.nactions);
    for (int i = 0; i < actions.nactions; i++) {
        UDA_LOG(UDA_LOG_DEBUG, "\n\n# {}", i);
        print_action(actions.action[i]);
    }
    UDA_LOG(UDA_LOG_DEBUG, "\n");
}

// Initialise an Action Structure and Child Structures

void init_dim_calibration(DimCalibration* act)
{
    act->factor = (double)1.0E0; // Data Calibration Correction/Scaling factor
    act->offset = (double)0.0E0; // Data Calibration Correction/Scaling offset
    act->invert = 0;             // Don't Invert the data
    act->units[0] = '\0';
}

void init_dim_composite(DimComposite* act)
{
    act->to_dim = -1;           // Swap to Dimension ID
    act->from_dim = -1;         // Swap from Dimension ID
    act->file[0] = '\0';        // Data Source File (with Full Path)
    act->format[0] = '\0';      // Data Source File's Format
    act->dim_signal[0] = '\0';  // Source Signal
    act->dim_error[0] = '\0';   // Error Source Signal
    act->dim_aserror[0] = '\0'; // Asymmetric Error Source Signal
}

void init_dim_documentation(DimDocumentation* act)
{
    act->label[0] = '\0';
    act->units[0] = '\0'; // Lower in priority than Calibration Units
}

void init_dim_error_model(DimErrorModel* act)
{
    act->model = ERROR_MODEL_UNKNOWN; // No Error Model
    act->param_n = 0;                 // No. Model parameters
    for (int i = 0; i < MAXERRPARAMS; i++) {
        act->params[i] = 0.0;
    }
}

void init_dimension(Dimension* act)
{
    act->dimid = -1;  // Dimension Id
    act->dimType = 0; // Structure Type
}

void init_time_offset(TimeOffset* act)
{
    act->method = 0;               // Correction Method: Standard offset correction only
    act->offset = (double)0.0E0;   // Time Dimension offset correction or start time
    act->interval = (double)0.0E0; // Time Dimension Interval correction
}

void init_calibration(Calibration* act)
{
    act->factor = (double)1.0E0; // Data Calibration Correction/Scaling factor
    act->offset = (double)0.0E0; // Data Calibration Correction/Scaling offset
    act->units[0] = '\0';
    act->target[0] = '\0'; // Which data Component to apply calibration? (all, data, error, aserror)
    act->invert = 0;       // No Inversion
    act->ndimensions = 0;
    act->dimensions = nullptr;
}

void init_documentation(Documentation* act)
{
    act->label[0] = '\0';
    act->units[0] = '\0'; // Lower in priority than Calibration Units
    act->description[0] = '\0';
    act->ndimensions = 0;
    act->dimensions = nullptr;
}

void init_composite(Composite* act)
{
    act->data_signal[0] = '\0';    // Derived Data using this Data Source
    act->error_signal[0] = '\0';   // Use Errors from this Source
    act->aserror_signal[0] = '\0'; // Use Asymmetric Errors from this Source
    act->map_to_signal[0] = '\0';  // Straight swap of data: map to this signal
    act->file[0] = '\0';           // Data Source File (with Full Path)
    act->format[0] = '\0';         // Data Source File's Format
    act->order = -1;               // Identify the Time Dimension
    act->ndimensions = 0;
    act->nsubsets = 0;
    act->nmaps = 0;
    act->dimensions = nullptr;
    act->subsets = nullptr;
    act->maps = nullptr;
}

void uda::client_server::init_server_side(ServerSide* act)
{
    act->nsubsets = 0;
    act->nmaps = 0;
    act->subsets = nullptr;
    act->maps = nullptr;
}

void init_error_model(ErrorModel* act)
{
    act->model = ERROR_MODEL_UNKNOWN; // No Error Model
    act->param_n = 0;                 // No. Model parameters
    for (int i = 0; i < MAXERRPARAMS; i++) {
        act->params[i] = 0.0;
    }
    act->ndimensions = 0;
    act->dimensions = nullptr;
}

void uda::client_server::init_subset(Subset* act)
{
    for (int i = 0; i < UDA_MAX_DATA_RANK; i++) {
        act->bound[i] = 0.0;                           // Subsetting Float Bounds
        act->ubindex[i] = {.init = false, .value = 0}; // Subsetting Integer Bounds (Upper Index)
        act->lbindex[i] = {.init = false, .value = 0}; // Lower Index
        act->stride[i] = {.init = false, .value = 0};  // Stride
        act->isindex[i] = false;                       // Flag the Bound is an Integer Type
        act->dimid[i] = -1;                            // Dimension IDs
        act->operation[i][0] = '\0';                   // Subsetting Operations
    }
    act->data_signal[0] = '\0'; // Data to Read
    act->member[0] = '\0';      // Structure Member to target
    act->function[0] = '\0';    // Name of simple function to apply
    act->nbound = 0;            // The number of Subsetting Operations
    act->reform = 0;            // reduce the Rank if a subsetted dimension has length 1
    act->order = -1;            // Explicitly set the order of the time dimension if >= 0
}

// Initialise an Action Structure

void uda::client_server::init_action(Action* act)
{
    act->actionType = 0;   // Action Range Type
    act->inRange = 0;      // Is this Action Record Applicable to the Current data Request?
    act->actionId = 0;     // Action XML Tag Id
    act->exp_range[0] = 0; // Applicable over this Exp. Number Range
    act->exp_range[1] = 0;
    act->pass_range[0] = -1; // Applicable over this Pass Number Range
    act->pass_range[1] = -1;
}

// Initialise an Action Array Structure

void uda::client_server::init_actions(Actions* act)
{
    act->nactions = 0;     // Number of Action blocks
    act->action = nullptr; // Array of Action blocks
}

void uda::client_server::free_actions(Actions* actions)
{
    // Free Heap Memory From Action Structures
    void* cptr;

    UDA_LOG(UDA_LOG_DEBUG, "freeActions: Enter");
    UDA_LOG(UDA_LOG_DEBUG, "freeDataBlock: Number of Actions = {} ", actions->nactions);

    if (actions->nactions == 0) {
        return;
    }

    for (int i = 0; i < actions->nactions; i++) {
        UDA_LOG(UDA_LOG_DEBUG, "freeDataBlock: freeing action Type = {} ", actions->action[i].actionType);

        switch (actions->action[i].actionType) {

            case UDA_COMPOSITE_TYPE:
                if ((cptr = (void*)actions->action[i].composite.dimensions) != nullptr) {
                    free(cptr);
                    actions->action[i].composite.dimensions = nullptr;
                    actions->action[i].composite.ndimensions = 0;
                }
                if (actions->action[i].composite.nsubsets > 0) {
                    if ((cptr = (void*)actions->action[i].composite.subsets) != nullptr) {
                        free(cptr);
                    }
                    actions->action[i].composite.subsets = nullptr;
                    actions->action[i].composite.nsubsets = 0;
                }
                if (actions->action[i].composite.nmaps > 0) {
                    if ((cptr = (void*)actions->action[i].composite.maps) != nullptr) {
                        free(cptr);
                    }
                    actions->action[i].composite.maps = nullptr;
                    actions->action[i].composite.nmaps = 0;
                }
                break;

            case UDA_ERROR_MODEL_TYPE:
                actions->action[i].errormodel.param_n = 0;

                for (int j = 0; j < actions->action[i].errormodel.ndimensions; j++) {
                    if ((cptr = (void*)actions->action[i].errormodel.dimensions) != nullptr) {
                        free(cptr);
                        actions->action[i].errormodel.dimensions = nullptr;
                        actions->action[i].errormodel.ndimensions = 0;
                    }
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
                    if ((cptr = (void*)actions->action[i].serverside.subsets) != nullptr) {
                        free(cptr);
                    }
                    actions->action[i].serverside.subsets = nullptr;
                    actions->action[i].serverside.nsubsets = 0;
                }
                if (actions->action[i].serverside.nmaps > 0) {
                    if ((cptr = (void*)actions->action[i].serverside.maps) != nullptr) {
                        free(cptr);
                    }
                    actions->action[i].serverside.maps = nullptr;
                    actions->action[i].serverside.nmaps = 0;
                }
                break;

            default:
                break;
        }
    }

    if ((cptr = (void*)actions->action) != nullptr) {
        free(cptr);
    }
    actions->nactions = 0;
    actions->action = nullptr;

    UDA_LOG(UDA_LOG_DEBUG, "freeActions: Exit");
}

// Copy an Action Structure and Drop Pointers to Action & Dimension Structures (ensures a single Heap free later)

void uda::client_server::copy_actions(Actions* actions_out, Actions* actions_in)
{
    *actions_out = *actions_in;
    actions_in->action = nullptr;
    actions_in->nactions = 0;
}
