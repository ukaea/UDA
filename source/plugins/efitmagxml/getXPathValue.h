#ifndef UDA_PLUGINS_EFITMAGXML_GETXPATHVALUE_H
#define UDA_PLUGINS_EFITMAGXML_GETXPATHVALUE_H

#include <libxml/xpath.h>
#include <clientserver/udaStructs.h>

#define XPATHARRAYMAXLOOP 1024

char* getXPathValue(const char* xmlfile, const char* path, unsigned short cleanup, int *err);

float* xPathFloatArray(const char *value, int* n);
double* xPathDoubleArray(const char *value, int* n);
int* xPathIntArray(const char *value, int* n);

#endif // UDA_PLUGINS_EFITMAGXML_GETXPATHVALUE_H
