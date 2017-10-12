#ifndef IDAM_PLUGINS_GEOMETRY_GEOMSIGNALMAP_H
#define IDAM_PLUGINS_GEOMETRY_GEOMSIGNALMAP_H

#include <plugins/udaPlugin.h>
#include <server/sqllib.h>

#define PLUGIN_VERSION 1

int do_signal_file(IDAM_PLUGIN_INTERFACE* idam_plugin_interface);
int do_signal_filename(IDAM_PLUGIN_INTERFACE* idam_plugin_interface);

int checkAvailableSignals(int shot, int n_all, int** signal_ids, int** is_available);

#endif //IDAM_PLUGINS_GEOMETRY_GEOMSIGNALMAP_H
