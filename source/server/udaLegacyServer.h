/*---------------------------------------------------------------
* UDA Legacy Data Server (protocol versions <= 6)
*
*---------------------------------------------------------------------------------------------------------------------*/

#ifndef UDA_SERVER_UDALEGACYSERVER_H
#define UDA_SERVER_UDALEGACYSERVER_H

#include <clientserver/udaStructs.h>
#include <structures/genStructs.h>
#include <clientserver/socketStructs.h>
#include <plugins/pluginStructs.h>

#if defined(_WIN32)
#  define LIBRARY_API __declspec(dllexport)
#else
#  define LIBRARY_API
#endif

#ifdef __cplusplus
extern "C" {
#endif

LIBRARY_API int idamLegacyServer(CLIENT_BLOCK client_block, const PLUGINLIST* pluginlist, LOGMALLOCLIST* logmalloclist,
                     USERDEFINEDTYPELIST* userdefinedtypelist, SOCKETLIST* socket_list, int protocolVersion);

#ifdef __cplusplus
}
#endif

#endif // UDA_SERVER_UDALEGACYSERVER_H

