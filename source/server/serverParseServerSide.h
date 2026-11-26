#pragma once

#ifndef UDA_SERVER_SERVERPARSESERVERSIDE_H
#define UDA_SERVER_SERVERPARSESERVERSIDE_H

#include <clientserver/parseXML.h>
#include <clientserver/udaStructs.h>
#include <clientserver/export.h>
#include "plugins/pluginStructs.h"

#ifdef __cplusplus
extern "C" {
#endif

LIBRARY_API int serverParseServerSide(REQUEST_DATA *request_block, ACTIONS *actions_serverside, const PLUGINLIST* plugin_list);

#ifdef __cplusplus
}
#endif

#endif // UDA_SERVER_SERVERPARSESERVERSIDE_H
