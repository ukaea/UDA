#ifndef IDAM_PLUGIN_WEST_STATIC_DATA_UTILITIES_H
#define IDAM_PLUGIN_WEST_STATIC_DATA_UTILITIES_H

#include <clientserver/udaStructs.h>

int setUDABlockFromArcadeScalarField(int data_type, char* prod_name, char* object_name, char* param_name, int index,
		int shotNumber, DATA_BLOCK* data_block, int* nodeIndices, float normalizationFactor);
void setStaticValue(int data_type, DATA_BLOCK* data_block, char* value, int requestedIndex, float normalizationFactor);
void set_BTANG_StaticValue(int data_type, DATA_BLOCK* data_block, char* value, int requestedIndex, float normalizationFactor);
void SetStatic1DData(DATA_BLOCK* data_block, int len, float *data);
void SetStatic1DINTData(DATA_BLOCK* data_block, int len, int *data);

#endif // IDAM_PLUGIN_WEST_DYN_DATA_UTILITIES_H
