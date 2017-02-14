#ifndef IDAM_CLIENT_ACCAPI_GEN_H
#define IDAM_CLIENT_ACCAPI_GEN_H

#ifdef __cplusplus
extern "C" {
#endif

int setIdamDataTree(int handle);

// Return a specific data tree

NTREE* getIdamDataTree(int handle);

// **** name typo - elliminate ASAP
NTREE* getIdamTreeData(int handle);

// Return a user defined data structure definition

USERDEFINEDTYPE* getIdamUserDefinedType(int handle);

NTREE* findIdamNTreeStructureDefinition(NTREE* node, const char* target);

#ifdef __cplusplus
}
#endif

#endif //IDAM_CLIENT_ACCAPI_GEN_H
