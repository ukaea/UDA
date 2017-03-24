#ifndef IDAM_PLUGINS_IMAS_COMMON_H
#define IDAM_PLUGINS_IMAS_COMMON_H

#ifdef __cplusplus
extern "C" {
#endif

void getIdamNameValuePairItemList(const char * list, char *** itemList, unsigned short * count, char quote,
                                  char delimiter);

void freeIdamNameValuePairItemList(char *** list, unsigned short count);

int getIdamNameValuePairVarArray(const char * values, char quote, char delimiter, unsigned short varSize, int varType,
                                 void ** varData);

int findIdamType(const char * typeName);

char * convertIdam2StringType(int type);

int findHDF5Type(char * typeName);

#ifndef IdamIMASMDSPluginInclude

int convertIdam2HDF5Type(int type);

#endif

int sizeIdamType(int type);

#ifdef __cplusplus
}
#endif

#endif // IDAM_PLUGINS_IMAS_COMMON_H