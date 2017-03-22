#ifndef IDAM_PLUGINS_GEOMETRY_GEOMCONFIG_H
#define IDAM_PLUGINS_GEOMETRY_GEOMCONFIG_H

#include <plugins/udaPlugin.h>
#include <server/sqllib.h>

#define PLUGIN_VERSION 1

int do_geom_get(IDAM_PLUGIN_INTERFACE* idam_plugin_interface);
int do_config_filename(IDAM_PLUGIN_INTERFACE* idam_plugin_interface);

int generateGeom(char* signal, float tor_angle, PGconn* DBConnect, PGresult* DBQuery);

#endif //IDAM_PLUGINS_GEOMETRY_GEOMCONFIG_H
