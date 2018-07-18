#ifndef IDAM_PLUGIN_WEST_UTILITIES_H
#define IDAM_PLUGIN_WEST_UTILITIES_H

#include <libxml/xpath.h>
#include <clientserver/udaStructs.h>

void tokenize1DArcadeParameters(const char* s, char** diagnostic, char** object_name, int* extractionIndex, char** normalizationAttributes);
void tokenize1DArcadeParameters2(const char* s, char** diagnostic, char** object_name, int* extractionIndex, char** diagnostic2, char** object_name2, int* extractionIndex2);
void RemoveSpaces(char* source);
void getValueCollect(char* command, char** value, int* nodeIndices);
void multiplyFloat(float* p, float factor, int val_nb);
void multiplyInt(int* p, float factor, int val_nb);

void getFunName(const char* s, char** fun_name);
void getNormalizationFactor(float* normalizationFactor, char* normalizationAttributes);
void setValue(int data_type, DATA_BLOCK* data_block, char* value, int requestedIndex, float normalizationFactor);
void set1DValue(int data_type, DATA_BLOCK* data_block, char* value, int val_nb, float normalizationFactor);
void setINTValue(int data_type, DATA_BLOCK* data_block, int value, float normalizationFactor);
void getRank(char* attributes, int* rank);
void tokenizeFunParameters(const char* s, char** argument_name, char** attributes, char** normalizationAttributes);
void tokenizeFunParametersWithChannels(const char* s, char** unvalid_channels, char** TOP_collections_parameters,
		char** attributes, char** normalizationAttributes);
void tokenize_set_channels_validity(const char* s, char** unvalid_channels, char** attributes);
void tokenizeCommand(const char* s, char** prod_name, char** obj_name, char** param_name, char **flag);
int  getCommand(int i, char** command, const char* TOP_collections_parameters);
void getUnvalidChannelsSize(char* unvalid_channels, int* size);
void getUnvalidChannels(char* unvalid_channels, int* v);
int  isChannelValid(int channel_number, int* unvalid_channels_list, int unvalid_channels_list_size);

int  getNumIDAMIndex(char* attributes, int* nodeIndices);
int  getNumIDAMIndex2(char* s, int* nodeIndices);

void getReturnType(char* attributes, int* dataType);
void searchIndices(int requestedIndex, int* l, int* searchedArray, int* searchedArrayIndex);
void addExtractionChars(char* result, char* signalName, int extractionIndex);
void getTopCollectionsCount(const char* TOP_collections_parameters, int* collectionsCount);
void getObjectName(char** obj_name, char* command);


#endif // IDAM_PLUGIN_WEST_UTILITIES_H
