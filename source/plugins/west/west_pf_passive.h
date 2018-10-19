
#ifndef IDAM_PLUGIN_WEST_PF_PASSIVE_H
#define IDAM_PLUGIN_WEST_PF_PASSIVE_H

#include <clientserver/udaStructs.h>

int pf_passive_loop_name(int shotNumber, DATA_BLOCK* data_block, int* nodeIndices);
int pf_passive_loop_identifier(int shotNumber, DATA_BLOCK* data_block, int* nodeIndices);
int pf_passive_element_name(int shotNumber, DATA_BLOCK* data_block, int* nodeIndices);
int pf_passive_element_identifier(int shotNumber, DATA_BLOCK* data_block, int* nodeIndices);
int pf_passive_elements_shapeOf(int shotNumber, DATA_BLOCK* data_block, int* nodeIndices);
int pf_passive_loops_shapeOf(int shotNumber, DATA_BLOCK* data_block, int* nodeIndices);
int pf_passive_turns(int shotNumber, DATA_BLOCK* data_block, int* nodeIndices);
int pf_passive_R(int shotNumber, DATA_BLOCK* data_block, int* nodeIndices);
int pf_passive_Z(int shotNumber, DATA_BLOCK* data_block, int* nodeIndices);
int pf_passive_current_data(int shotNumber, DATA_BLOCK* data_block, int* nodeIndices);
int pf_passive_current_time(int shotNumber, DATA_BLOCK* data_block, int* nodeIndices);
void pf_passive_throwsIdamError(int status, char* methodName, char* object_name, int shotNumber);

#endif // IDAM_PLUGIN_WEST_PF_PASSIVE_H
