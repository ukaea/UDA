#ifndef IDAM_SERVER_IDAMSERVERLEGACYPLUGIN_H
#define IDAM_SERVER_IDAMSERVERLEGACYPLUGIN_H

#include <clientserver/udaStructs.h>

#ifdef __cplusplus
extern "C" {
#endif

int idamServerLegacyPlugin(REQUEST_BLOCK *request_block, DATA_SOURCE *data_source, SIGNAL_DESC *signal_desc);

#ifdef __cplusplus
}
#endif

#endif // IDAM_SERVER_IDAMSERVERLEGACYPLUGIN_H

