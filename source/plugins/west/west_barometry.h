
#ifndef IDAM_PLUGIN_WEST_BAROMETRY_H
#define IDAM_PLUGIN_WEST_BAROMETRY_H

#include <clientserver/udaStructs.h>

int barometry_gauge_name(int shotNumber, DATA_BLOCK* data_block, int* nodeIndices);
int barometry_gauge_type_name(int shotNumber, DATA_BLOCK* data_block, int* nodeIndices);
int barometry_gauge_type_index(int shotNumber, DATA_BLOCK* data_block, int* nodeIndices);
int barometry_gauge_type_description(int shotNumber, DATA_BLOCK* data_block, int* nodeIndices);
int barometry_gauge_pressure_data(int shotNumber, DATA_BLOCK* data_block, int* nodeIndices);
int barometry_gauge_pressure_time(int shotNumber, DATA_BLOCK* data_block, int* nodeIndices);
int barometry_gauge_calibration_coefficient(int shotNumber, DATA_BLOCK* data_block, int* nodeIndices);
void barometry_throwsIdamError(int status, char* methodName, char* object_name, int shotNumber);
#endif // IDAM_PLUGIN_WEST_BAROMETRY_H
