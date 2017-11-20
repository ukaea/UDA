#ifndef IDAM_PLUGINS_GEOMETRY_GEOMSIGNALMAP_H
#define IDAM_PLUGINS_GEOMETRY_GEOMSIGNALMAP_H

#include <libpq-fe.h> // SQL Library Header
#include <plugins/udaPlugin.h>

#define PLUGIN_VERSION 1

int do_signal_file(IDAM_PLUGIN_INTERFACE* idam_plugin_interface, PGconn* DBConnect);
int do_signal_filename(IDAM_PLUGIN_INTERFACE* idam_plugin_interface, PGconn* DBConnect);

int checkAvailableSignals(int shot, int n_all, int** signal_ids, int** is_available);

#endif //IDAM_PLUGINS_GEOMETRY_GEOMSIGNALMAP_H
