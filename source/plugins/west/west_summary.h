
#ifndef IDAM_PLUGIN_WEST_SUMMARY_H
#define IDAM_PLUGIN_WEST_SUMMARY_H

#include <clientserver/udaStructs.h>

void summary_global_quantities_beta_tor_value(int shotNumber, DATA_BLOCK* data_block, int* nodeIndices);
void summary_flt1D(const char* mappingValue, int shotNumber, DATA_BLOCK* data_block, int* nodeIndices);
void summary_contrib_flt1D(const char* mappingValue, int shotNumber, DATA_BLOCK* data_block, int* nodeIndices);
void summary_global_quantities_tau_resistance_value(int shotNumber, DATA_BLOCK* data_block, int* nodeIndices);
void summary_global_quantities_v_loop_value(int shotNumber, DATA_BLOCK* data_block, int* nodeIndices);
void summary_global_quantities_r0_value(int shotNumber, DATA_BLOCK* data_block, int* nodeIndices);
void summary_global_quantities_b0_value(int shotNumber, DATA_BLOCK* data_block, int* nodeIndices);

/*
void summary_global_quantities_ip_value(int shotNumber, DATA_BLOCK* data_block, int* nodeIndices);
void summary_global_quantities_ohm_value(int shotNumber, DATA_BLOCK* data_block, int* nodeIndices);
void summary_global_quantities_v_loop_value(int shotNumber, DATA_BLOCK* data_block, int* nodeIndices);
void summary_global_quantities_li_value(int shotNumber, DATA_BLOCK* data_block, int* nodeIndices);
void summary_global_quantities_beta_pol_value(int shotNumber, DATA_BLOCK* data_block, int* nodeIndices);
void summary_global_quantities_energy_diamagnetic_value(int shotNumber, DATA_BLOCK* data_block, int* nodeIndices);
void summary_global_quantities_energy_total_value(int shotNumber, DATA_BLOCK* data_block, int* nodeIndices);
void summary_global_quantities_energy_b_field_pol_value(int shotNumber, DATA_BLOCK* data_block, int* nodeIndices);
void summary_global_quantities_volume_value(int shotNumber, DATA_BLOCK* data_block, int* nodeIndices);
void summary_global_quantities_r0_value(int shotNumber, DATA_BLOCK* data_block, int* nodeIndices);
void summary_global_quantities_b0_value(int shotNumber, DATA_BLOCK* data_block, int* nodeIndices);
void summary_global_quantities_tau_energy_value(int shotNumber, DATA_BLOCK* data_block, int* nodeIndices);
void summary_global_quantities_tau_resistive_value(int shotNumber, DATA_BLOCK* data_block, int* nodeIndices);
*/



#endif // IDAM_PLUGIN_WEST_SUMMARY_H
