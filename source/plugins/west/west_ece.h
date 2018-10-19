
#ifndef IDAM_PLUGIN_WEST_ECE_H
#define IDAM_PLUGIN_WEST_ECE_H

#include <clientserver/udaStructs.h>

int homogeneous_time(int shotNumber, DATA_BLOCK* data_block, int* nodeIndices);
int ece_frequencies(int shotNumber, DATA_BLOCK* data_block, int* nodeIndices);
int ece_t_e_time(int shotNumber, DATA_BLOCK* data_block, int* nodeIndices);
int ece_time(int shotNumber, DATA_BLOCK* data_block, int* nodeIndices);
int ece_harmonic_time(int shotNumber, DATA_BLOCK* data_block, int* nodeIndices);
int ece_t_e_data_shape_of(int shotNumber, char** mapfun);
int ece_t_e_data(int shotNumber, char** mapfun);
void ece_throwsIdamError(int status, char* methodName, char* object_name, int shotNumber);
void ece_throwsIdamError2(int status, char* methodName, char* object_name, int channel, int shotNumber) ;
#endif // IDAM_PLUGIN_WEST_ECE_H
