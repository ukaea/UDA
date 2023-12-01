#pragma once

#ifndef UDA_GETPLUGIN_ADDRESS_H
#define UDA_GETPLUGIN_ADDRESS_H

#include "udaPlugin.h"
#include "export.h"

int getPluginAddress(void **pluginHandle, const char *library, const char *symbol, PLUGINFUNP *pluginfunp);

#endif // UDA_GETPLUGIN_ADDRESS_H
