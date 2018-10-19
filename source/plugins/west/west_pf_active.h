
#ifndef IDAM_PLUGIN_WEST_PF_ACTIVE_H
#define IDAM_PLUGIN_WEST_PF_ACTIVE_H

#include <clientserver/udaStructs.h>

int pf_active_elements_shapeOf(int shotNumber, DATA_BLOCK* data_block, int* nodeIndices);
int pf_active_coils_shapeOf(int shotNumber, DATA_BLOCK* data_block, int* nodeIndices);
int pf_active_R(int shotNumber, DATA_BLOCK* data_block, int* nodeIndices);
int pf_active_Z(int shotNumber, DATA_BLOCK* data_block, int* nodeIndices);
int pf_active_H(int shotNumber, DATA_BLOCK* data_block, int* nodeIndices);
int pf_active_W(int shotNumber, DATA_BLOCK* data_block, int* nodeIndices);
int pf_active_element_identifier(int shotNumber, DATA_BLOCK* data_block, int* nodeIndices);
int pf_active_element_name(int shotNumber, DATA_BLOCK* data_block, int* nodeIndices);
int pf_active_coil_identifier(int shotNumber, DATA_BLOCK* data_block, int* nodeIndices);
int pf_active_coil_name(int shotNumber, DATA_BLOCK* data_block, int* nodeIndices);
int pf_active_turns(int shotNumber, DATA_BLOCK* data_block, int* nodeIndices);
int pf_active_current_data(int shotNumber, DATA_BLOCK* data_block, int* nodeIndices);
int pf_active_current_time(int shotNumber, DATA_BLOCK* data_block, int* nodeIndices);
void pf_active_throwsIdamError(int status, char* methodName, char* object_name, int shotNumber);
#endif // IDAM_PLUGIN_WEST_PF_ACTIVE_H
