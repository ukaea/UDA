#ifndef UDA_PLUGIN_READBYTESNONOPTIMALLY_H
#define UDA_PLUGIN_READBYTESNONOPTIMALLY_H

#include "udaStructs.h"
#include "export.h"

#include <string>
#include "pluginStructs.h"

int readBytes(const std::string& path, IDAM_PLUGIN_INTERFACE* plugin_interface);

#endif // UDA_PLUGIN_READBYTESNONOPTIMALLY_H

