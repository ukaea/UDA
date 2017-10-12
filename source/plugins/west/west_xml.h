#ifndef IDAM_PLUGIN_WEST_XML_H
#define IDAM_PLUGIN_WEST_XML_H

#include <libxml/xpath.h>
#include <clientserver/udaStructs.h>

int GetStaticData(int shotNumber, const char* mapfun, DATA_BLOCK* data_block, int* nodeIndices);

#endif // IDAM_PLUGIN_WEST_XML_H
