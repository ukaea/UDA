#ifndef UDA_PLUGINS_PROVENANCE_PUT_H
#define UDA_PLUGINS_PROVENANCE_PUT_H

#include <plugins/udaPlugin.h>

#ifdef __cplusplus
extern "C" {
#endif

int put(IDAM_PLUGIN_INTERFACE * idam_plugin_interface);

int makeProvenanceDir(char ** newDir, char * root, char * UID, unsigned short * priorDir);

int copyProvenanceWebFile(char * oldFile, char * dir, char * newFileName, FILE * log);

void getFileList(char * list, char *** fileList, char *** fileNames, unsigned short * fileListCount);

void freeFileNameList(char *** list, unsigned short listCount);

#ifdef __cplusplus
}
#endif

#endif // UDA_PLUGINS_PROVENANCE_PUT_H
