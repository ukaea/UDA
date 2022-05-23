#pragma once

#ifndef UDA_SERVER_MAKESERVERREQUESTBLOCK_HPP
#define UDA_SERVER_MAKESERVERREQUESTBLOCK_HPP

#include <plugins/udaPlugin.h>
#include <clientserver/udaStructs.h>
#include <clientserver/export.h>

#include "plugins.hpp"
#include "server_environment.hpp"

namespace uda {

int makeServerRequestBlock(RequestBlock* request_block, const uda::Plugins& plugins, const server::Environment& environment);

int makeServerRequestData(RequestData* request, const uda::Plugins& plugins, const uda::server::Environment& environment);

}

#endif // UDA_SERVER_MAKESERVERREQUESTBLOCK_HPP
