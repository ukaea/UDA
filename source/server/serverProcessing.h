#ifndef UDA_SERVER_SERVERPROCESSING_H
#define UDA_SERVER_SERVERPROCESSING_H

#include <clientserver/udaStructs.h>

#if defined(_WIN32)
#  define LIBRARY_API __declspec(dllexport)
#else
#  define LIBRARY_API
#endif

#ifdef __cplusplus
extern "C" {
#endif

LIBRARY_API int serverProcessing(CLIENT_BLOCK client_block, DATA_BLOCK *data_block);

#ifdef __cplusplus
}
#endif

#endif // UDA_SERVER_SERVERPROCESSING_H

