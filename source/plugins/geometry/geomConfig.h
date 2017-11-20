#ifndef IDAM_PLUGINS_GEOMETRY_GEOMCONFIG_H
#define IDAM_PLUGINS_GEOMETRY_GEOMCONFIG_H

//#include "geomDatabase.h"
#include <libpq-fe.h> // SQL Library Header
#include <plugins/udaPlugin.h>

#define PLUGIN_VERSION 1

int do_geom_get(IDAM_PLUGIN_INTERFACE* idam_plugin_interface, PGconn* DBConnect);
int do_config_filename(IDAM_PLUGIN_INTERFACE* idam_plugin_interface, PGconn* DBConnect);

#endif //IDAM_PLUGINS_GEOMETRY_GEOMCONFIG_H
