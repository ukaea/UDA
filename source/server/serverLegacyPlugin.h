#pragma once

#ifndef UDA_SERVER_SERVERLEGACYPLUGIN_H
#  define UDA_SERVER_SERVERLEGACYPLUGIN_H

#  include "clientserver/udaStructs.h"

int udaServerLegacyPlugin(uda::client_server::REQUEST_DATA* request, uda::client_server::DATA_SOURCE* data_source,
                          uda::client_server::SIGNAL_DESC* signal_desc);

#endif // UDA_SERVER_SERVERLEGACYPLUGIN_H
