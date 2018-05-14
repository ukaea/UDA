#ifndef UDA_PLUGIN_EXP2IMAS_EXP2IMAS_XML_H
#define UDA_PLUGIN_EXP2IMAS_EXP2IMAS_XML_H

#include <libxml/xpath.h>

#include <clientserver/udaStructs.h>

#ifdef LIBXML2_PRINTF_CHAR_ARG
#  define XML_FMT_TYPE const char*
#else
#  define XML_FMT_TYPE const xmlChar*
#endif

typedef struct XMLData {
    char* data;
    int data_type;
    int time_dim;
    int* sizes;
    float* coefas;
    float* coefbs;
    int* dims;
    int rank;
    char* download;
    double* values;
    size_t n_values;
    int resize;
} XML_DATA;

int execute_xpath_expression(const char* filename, const xmlChar* xpathExpr, int index, XML_DATA* xml_data);

#endif // UDA_PLUGIN_EXP2IMAS_EXP2IMAS_XML_H
