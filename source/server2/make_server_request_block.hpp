#pragma once

#ifndef UDA_SERVER_MAKESERVERREQUESTBLOCK_HPP
#  define UDA_SERVER_MAKESERVERREQUESTBLOCK_HPP

#  include "export.h"
#  include "udaPlugin.h"
#  include "udaStructs.h"

#  include "plugins.hpp"
#  include "server_environment.hpp"

namespace uda
{

int makeServerRequestBlock(RequestBlock* request_block, const uda::Plugins& plugins,
                           const server::Environment& environment);

int makeServerRequestData(RequestData* request, const uda::Plugins& plugins,
                          const uda::server::Environment& environment);

} // namespace uda

#endif // UDA_SERVER_MAKESERVERREQUESTBLOCK_HPP
