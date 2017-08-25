
#ifndef IDAM_PLUGIN_WEST_PF_PASSIVE_H
#define IDAM_PLUGIN_WEST_PF_PASSIVE_H

#include <clientserver/udaStructs.h>

void passive_name(int shotNumber, DATA_BLOCK* data_block, int* nodeIndices);
void passive_r(int shotNumber, DATA_BLOCK* data_block, int* nodeIndices);
void passive_z(int shotNumber, DATA_BLOCK* data_block, int* nodeIndices);
void passive_current_shapeOf(int shotNumber, DATA_BLOCK* data_block, int* nodeIndices);
int  passive_current(int shotNumber, DATA_BLOCK* data_block, int* nodeIndices);
int passive_time(int shotNumber, DATA_BLOCK* data_block, int* nodeIndices);


#endif // IDAM_PLUGIN_WEST_PF_PASSIVE_H
