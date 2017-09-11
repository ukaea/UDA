#ifndef IDAM_PLUGIN_WEST_DYN_DATA_UTILITIES_H
#define IDAM_PLUGIN_WEST_DYN_DATA_UTILITIES_H

#include <clientserver/udaStructs.h>

int SetNormalizedDynamicData(int shotNumber, DATA_BLOCK* data_block, int* nodeIndices,
		char* TOP_collections_parameters, char* attributes, char* normalizationAttributes);

int SetNormalizedDynamicDataTime(int shotNumber, DATA_BLOCK* data_block, int* nodeIndices,
		char* TOP_collections_parameters, char* attributes, char* normalizationAttributes);

int GetNormalizedDynamicData(int shotNumber, float** time, float** data, int* len, int* nodeIndices,
		char* TOP_collections_parameters, char* attributes, char* normalizationAttributes);

int GetDynData(int shotNumber, float** time, float** data, int* len, int* nodeIndices,
		char* TOP_collections_parameters, char* attributes);

void SetDynamicData(DATA_BLOCK* data_block, int len, float *time, float *data);

void SetDynamicDataTime(DATA_BLOCK* data_block, int len, float *time, float *data);

#endif // IDAM_PLUGIN_WEST_DYN_DATA_UTILITIES_H
