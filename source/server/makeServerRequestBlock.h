#pragma once

#ifndef UDA_SERVER_MAKESERVERREQUESTBLOCK_H
#define UDA_SERVER_MAKESERVERREQUESTBLOCK_H

#include "udaPlugin.h"
#include "udaStructs.h"
#include "export.h"

#if defined(SERVERBUILD) || defined(FATCLIENT)

int makeServerRequestBlock(REQUEST_BLOCK* request_block, PLUGINLIST pluginList);
int makeServerRequestData(REQUEST_DATA* request, PLUGINLIST pluginList);

#endif

#endif // UDA_SERVER_MAKESERVERREQUESTBLOCK_H
