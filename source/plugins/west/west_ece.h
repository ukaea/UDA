
#ifndef IDAM_PLUGIN_WEST_ECE_H
#define IDAM_PLUGIN_WEST_ECE_H

#include <clientserver/udaStructs.h>

int test_fun(int shotNumber, DATA_BLOCK* data_block, int* nodeIndices);
int ece_frequencies(int shotNumber, DATA_BLOCK* data_block, int* nodeIndices);
int ece_names(int shotNumber, DATA_BLOCK* data_block, int* nodeIndices);
int ece_identifiers(int shotNumber, DATA_BLOCK* data_block, int* nodeIndices);
void ece_t_e_data(int shotNumber, char** ece_mapfun);
void ece_t_e_time(int shotNumber, char** ece_mapfun);
void ece_t_e_data_shape_of(int shotNumber, char** mapfun);
int ece_harmonic_data(int shotNumber, DATA_BLOCK* data_block, int* nodeIndices);
int ece_harmonic_time(int shotNumber, DATA_BLOCK* data_block, int* nodeIndices);

#endif // IDAM_PLUGIN_WEST_ECE_H
