#ifndef UDA_PLUGIN_READBYTESNONOPTIMALLY_H
#define UDA_PLUGIN_READBYTESNONOPTIMALLY_H

#include "export.h"
#include "udaStructs.h"

#include "pluginStructs.h"
#include <string>

int readBytes(const std::string& path, IDAM_PLUGIN_INTERFACE* plugin_interface);

#endif // UDA_PLUGIN_READBYTESNONOPTIMALLY_H
