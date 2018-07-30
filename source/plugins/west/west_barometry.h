
#ifndef IDAM_PLUGIN_WEST_BAROMETRY_H
#define IDAM_PLUGIN_WEST_BAROMETRY_H

#include <clientserver/udaStructs.h>

void barometry_gauge_name(int shotNumber, DATA_BLOCK* data_block, int* nodeIndices);
void barometry_gauge_type_name(int shotNumber, DATA_BLOCK* data_block, int* nodeIndices);
void barometry_gauge_type_index(int shotNumber, DATA_BLOCK* data_block, int* nodeIndices);
void barometry_gauge_type_description(int shotNumber, DATA_BLOCK* data_block, int* nodeIndices);
void barometry_gauge_pressure_data(int shotNumber, DATA_BLOCK* data_block, int* nodeIndices);
void barometry_gauge_pressure_time(int shotNumber, DATA_BLOCK* data_block, int* nodeIndices);
void barometry_gauge_calibration_coefficient(int shotNumber, DATA_BLOCK* data_block, int* nodeIndices);

#endif // IDAM_PLUGIN_WEST_BAROMETRY_H
