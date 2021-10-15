#pragma once

#ifndef UDA_SERVER_SERVERLEGACYPLUGIN_H
#define UDA_SERVER_SERVERLEGACYPLUGIN_H

#include <clientserver/udaStructs.h>
#include <clientserver/export.h>

int udaServerLegacyPlugin(REQUEST_DATA* request, DATA_SOURCE* data_source, SIGNAL_DESC* signal_desc);

#endif // UDA_SERVER_SERVERLEGACYPLUGIN_H
