#ifndef UDA_PLUGIN_EXP2IMAS_EXP2IMAS_XML_H
#define UDA_PLUGIN_EXP2IMAS_EXP2IMAS_XML_H

#include <libxml/xpath.h>

#include <clientserver/udaStructs.h>

#ifdef LIBXML2_PRINTF_CHAR_ARG
#  define XML_FMT_TYPE const char*
#else
#  define XML_FMT_TYPE const xmlChar*
#endif

int execute_xpath_expression(const char* filename, const xmlChar* xpathExpr, char** data, int* data_type, int* time_dim,
                             int** sizes, float** coefas, float** coefbs, int index, int** dims, int* rank);

#endif // UDA_PLUGIN_EXP2IMAS_EXP2IMAS_XML_H
