#ifndef IDAM_PLUGINS_CORRECTIONS_ANBCORRECTIONS_H
#define IDAM_PLUGINS_CORRECTIONS_ANBCORRECTIONS_H

#include <clientserver/udaStructs.h>
#include <server/pluginStructs.h>

void makeDataBlock(DATA_BLOCK *out, int dataCount);

void makeLegacyDataBlock(DATA_BLOCK *out);

void copyANBDataBlock(DATA_BLOCK *out, DATA_BLOCK *in, int dataCount);

int anbCorrections(IDAM_PLUGIN_INTERFACE *idam_plugin_interface);

#endif // IDAM_PLUGINS_CORRECTIONS_ANBCORRECTIONS_H