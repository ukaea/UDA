#pragma once

#include <string>

#include <clientserver/udaStructs.h>
#include <plugins/pluginStructs.h>

// Errors are recorded on, and diagnostics logged through, plugin_interface
int readBytes(IDAM_PLUGIN_INTERFACE* plugin_interface, FILE* file, DATA_BLOCK* data_block, int offset, int max_bytes,
              const std::string& checksum, bool opaque);

