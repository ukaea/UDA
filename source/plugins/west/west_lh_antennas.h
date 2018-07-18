
#ifndef IDAM_PLUGIN_WEST_LH_ANTENNAS_H
#define IDAM_PLUGIN_WEST_LH_ANTENNAS_H

#include <clientserver/udaStructs.h>

void lh_antennas_power(int shotNumber, DATA_BLOCK* data_block, int* nodeIndices);
void lh_antennas_power_forward(int shotNumber, DATA_BLOCK* data_block, int* nodeIndices);
void lh_antennas_power_reflected(int shotNumber, DATA_BLOCK* data_block, int* nodeIndices);
void lh_antennas_reflection_coefficient(int shotNumber, DATA_BLOCK* data_block, int* nodeIndices);
void lh_antennas_modules_power(int shotNumber, DATA_BLOCK* data_block, int* nodeIndices);
void lh_antennas_modules_power_forward(int shotNumber, DATA_BLOCK* data_block, int* nodeIndices);
void lh_antennas_modules_power_reflected(int shotNumber, DATA_BLOCK* data_block, int* nodeIndices);
void lh_antennas_modules_reflection_coefficient(int shotNumber, DATA_BLOCK* data_block, int* nodeIndices);
void lh_antennas_modules_phase(int shotNumber, DATA_BLOCK* data_block, int* nodeIndices);
void lh_antennas_phase_average(int shotNumber, DATA_BLOCK* data_block, int* nodeIndices);
void lh_antennas_n_parallel_peak(int shotNumber, DATA_BLOCK* data_block, int* nodeIndices);
void lh_antennas_position_r(int shotNumber, DATA_BLOCK* data_block, int* nodeIndices);
void lh_antennas_position_z(int shotNumber, DATA_BLOCK* data_block, int* nodeIndices);
void lh_antennas_position_definition(int shotNumber, DATA_BLOCK* data_block, int* nodeIndices);
void lh_antennas_pressure_tank(int shotNumber, DATA_BLOCK* data_block, int* nodeIndices);

#endif // IDAM_PLUGIN_WEST_LH_ANTENNAS_H
