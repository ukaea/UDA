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

int idamLegacyServer(CLIENT_BLOCK client_block, const PLUGINLIST* pluginlist, LOGMALLOCLIST* logmalloclist,
                     USERDEFINEDTYPELIST* userdefinedtypelist, SOCKETLIST* socket_list, int protocolVersion);

#endif // UDA_SERVER_UDALEGACYSERVER_H

