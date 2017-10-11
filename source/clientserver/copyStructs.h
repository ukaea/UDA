#ifndef UDA_CLIENTSERVER_COPYSTRUCTS_H
#define UDA_CLIENTSERVER_COPYSTRUCTS_H

#include <plugins/udaPlugin.h>
#include "udaStructs.h"

void copyServerBlock(SERVER_BLOCK * out, SERVER_BLOCK in);

void copyRequestBlock(REQUEST_BLOCK * out, REQUEST_BLOCK in);

void copyDataBlock(DATA_BLOCK * out, DATA_BLOCK in);

void copyDataSystem(DATA_SYSTEM * out, DATA_SYSTEM in);

void copySystemConfig(SYSTEM_CONFIG * out, SYSTEM_CONFIG in);

void copyDataSource(DATA_SOURCE * out, DATA_SOURCE in);

void copySignal(SIGNAL * out, SIGNAL in);

void copySignalDesc(SIGNAL_DESC * out, SIGNAL_DESC in);

void copyPluginInterface(IDAM_PLUGIN_INTERFACE* out, IDAM_PLUGIN_INTERFACE* in);

#endif // UDA_CLIENTSERVER_COPYSTRUCTS_H
