#ifndef UDA_SERVER_SERVERLEGACYPLUGIN_H
#define UDA_SERVER_SERVERLEGACYPLUGIN_H

#include <clientserver/udaStructs.h>

#if defined(_WIN32)
#  define LIBRARY_API __declspec(dllexport)
#else
#  define LIBRARY_API
#endif

#ifdef __cplusplus
extern "C" {
#endif

LIBRARY_API int idamServerLegacyPlugin(REQUEST_BLOCK *request_block, DATA_SOURCE *data_source, SIGNAL_DESC *signal_desc);

#ifdef __cplusplus
}
#endif

#endif // UDA_SERVER_SERVERLEGACYPLUGIN_H

