// Open Data plugin

#ifndef IdamOpenDataInclude
#define IdamOpenDataInclude

// Change History:
//
// 06May2014	dgm	Original Version

#include <idamplugin.h>

#ifdef __cplusplus
extern "C" {
#endif

#define MAXROOT        1024
#define BUFFERSIZE    10*1024

int openData(IDAM_PLUGIN_INTERFACE * idam_plugin_interface);

int copyProvenanceFile(char * oldFile, char * dir, char * newFileName);

#ifdef __cplusplus
}
#endif

#endif
