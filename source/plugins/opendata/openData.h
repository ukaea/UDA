#ifndef IDAM_PLUGINS_OPENDATA_OPENDATA_H
#define IDAM_PLUGINS_OPENDATA_OPENDATA_H

#include <include/idamplugin.h>

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

#endif // IDAM_PLUGINS_OPENDATA_OPENDATA_H
