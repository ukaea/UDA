#ifndef IDAM_PLUGIN_TORE_SUPRA_TS_XML_H
#define IDAM_PLUGIN_TORE_SUPRA_TS_XML_H

#include <libxml/xpath.h>

#include <clientserver/udaStructs.h>

#if LIBXML_VERSION >= 20904
#  define FMT_TYPE const char*
#else
#  define FMT_TYPE const xmlChar*
#endif

int execute_xpath_expression(const char* filename, const xmlChar* xpathExpr, DATA_BLOCK* data_block, int* nodeIndices);

#endif // IDAM_PLUGIN_TORE_SUPRA_TS_XML_H
