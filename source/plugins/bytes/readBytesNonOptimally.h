#ifndef UDA_PLUGIN_READBYTESNONOPTIMALLY_H
#define UDA_PLUGIN_READBYTESNONOPTIMALLY_H

#include <uda/plugins.h>

#include <string>

#ifdef __cplusplus
extern "C" {
#endif

int readBytes(const std::string &path, UDA_PLUGIN_INTERFACE *plugin_interface);

#ifdef __cplusplus
}
#endif

#endif // UDA_PLUGIN_READBYTESNONOPTIMALLY_H
