#ifndef UDA_PLUGIN_READBYTESNONOPTIMALLY_H
#define UDA_PLUGIN_READBYTESNONOPTIMALLY_H

#include <clientserver/udaStructs.h>
#include <clientserver/export.h>

#include <string>
#include <plugins/pluginStructs.h>

int readBytes(const std::string& path, IDAM_PLUGIN_INTERFACE* plugin_interface);

#endif // UDA_PLUGIN_READBYTESNONOPTIMALLY_H

