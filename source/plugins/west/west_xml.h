#ifndef IDAM_PLUGIN_WEST_XML_H
#define IDAM_PLUGIN_WEST_XML_H

#include <initStructs.h>
#include <libxml/xpath.h>

int GetStaticData(int shotNumber, const char* mapfun, DATA_BLOCK* data_block, int* nodeIndices);
int GetDynamicData(int shotNumber, const char* mapfun, DATA_BLOCK* data_block, int* nodeIndices);

#endif // IDAM_PLUGIN_WEST_XML_H
