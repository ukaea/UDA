/*---------------------------------------------------------------
* IDAM Legacy Data Server (protocol versions <= 6)
*
*---------------------------------------------------------------------------------------------------------------------*/

#ifndef IDAM_SERVER_IDAMLEGACYSERVER_H
#define IDAM_SERVER_IDAMLEGACYSERVER_H

#include <clientserver/udaStructs.h>

#include "pluginStructs.h"

int idamLegacyServer(CLIENT_BLOCK client_block, const PLUGINLIST* pluginlist);

#endif // IDAM_SERVER_IDAMLEGACYSERVER_H

