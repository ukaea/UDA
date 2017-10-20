/*
 * geometry.h
 *
 *  Created on: 24 Feb 2016
 *      Author: lkogan
 */

#ifndef IDAM_PLUGINS_GEOMETRY_GEOMETRY_H
#define IDAM_PLUGINS_GEOMETRY_GEOMETRY_H

#include <plugins/udaPlugin.h>

#ifdef __cplusplus
extern "C" {
#endif

#define THISPLUGIN_VERSION                  1
#define THISPLUGIN_MAX_INTERFACE_VERSION    1        // Interface versions higher than this will not be understood!
#define THISPLUGIN_DEFAULT_METHOD           "help"

int idamGeom(IDAM_PLUGIN_INTERFACE *idam_plugin_interface);

int do_help(IDAM_PLUGIN_INTERFACE* idam_plugin_interface);

#ifdef __cplusplus
}
#endif

#endif /* IDAM_PLUGINS_GEOMETRY_GEOMETRY_H */
