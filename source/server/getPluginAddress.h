#pragma once

#include "serverPlugin.h"

int getPluginAddress(void** pluginHandle, const char* library, const char* symbol, PLUGINFUNP* pluginfunp);
