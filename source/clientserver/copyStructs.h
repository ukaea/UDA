#ifndef UDA_CLIENTSERVER_COPYSTRUCTS_H
#define UDA_CLIENTSERVER_COPYSTRUCTS_H

#include <plugins/udaPlugin.h>
#include "udaStructs.h"
#include "export.h"

#ifdef __cplusplus
extern "C" {
#endif

LIBRARY_API void copyRequestBlock(REQUEST_BLOCK* out, REQUEST_BLOCK in);
LIBRARY_API void copyDataSource(DATA_SOURCE* out, DATA_SOURCE in);
LIBRARY_API void copyPluginInterface(IDAM_PLUGIN_INTERFACE* out, IDAM_PLUGIN_INTERFACE* in);

#ifdef __cplusplus
}
#endif

#endif // UDA_CLIENTSERVER_COPYSTRUCTS_H
