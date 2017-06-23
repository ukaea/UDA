#ifndef IDAM_PLUGIN_WEST_DYNAMIC_DATA_H
#define IDAM_PLUGIN_WEST_DYNAMIC_DATA_H

#include <libxml/xpath.h>
#include <clientserver/udaStructs.h>

int GetDynamicData(int shotNumber, const char* mapfun, DATA_BLOCK* data_block, int* nodeIndices);


#endif // IDAM_PLUGIN_WEST_DYNAMIC_DATA_H
