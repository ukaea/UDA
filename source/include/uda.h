#pragma once

#ifndef UDA_H
#define UDA_H

#  include "uda/client.h"
#  include "uda/export.h"
#  include "uda/plugins.h"
#  include "uda/structured.h"
#  include "uda/types.h"

#  ifdef __cplusplus
#    include "uda/uda_plugin_base.hpp"
#  endif

#  ifdef UDA_LEGACY
#    include "uda/legacy.h"
#  endif // UDA_LEGACY

#endif // UDA_H
