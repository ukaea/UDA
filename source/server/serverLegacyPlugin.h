#ifndef UDA_SERVER_SERVERLEGACYPLUGIN_H
#define UDA_SERVER_SERVERLEGACYPLUGIN_H

#include <clientserver/udaStructs.h>
#include <clientserver/export.h>

#ifdef __cplusplus
extern "C" {
#endif

LIBRARY_API int udaServerLegacyPlugin(REQUEST_BLOCK *request_block, DATA_SOURCE *data_source, SIGNAL_DESC *signal_desc);

#ifdef __cplusplus
}
#endif

#endif // UDA_SERVER_SERVERLEGACYPLUGIN_H
