/*
 * xpadtree.h
 *
 *  Created on: 24 Feb 2016
 *      Author: lkogan
 */

#ifndef XPADTREE_H_
#define XPADTREE_H_

#include <libpq-fe.h>
#include <server/pluginStructs.h>

#include <plugins/udaPlugin.h>

#ifdef __cplusplus
extern "C" {
#endif

#define THISPLUGIN_VERSION                  1
#define THISPLUGIN_MAX_INTERFACE_VERSION    1        // Interface versions higher than this will not be understood!
#define THISPLUGIN_DEFAULT_METHOD           "help"

int idamXpadTree(IDAM_PLUGIN_INTERFACE *idam_plugin_interface);
int do_xpad_signals(IDAM_PLUGIN_INTERFACE *idam_plugin_interface, PGconn* DBConnect);
int do_xpad_signal_tags(IDAM_PLUGIN_INTERFACE *idam_plugin_interface, PGconn* DBConnect);
int do_xpad_signal_tree(IDAM_PLUGIN_INTERFACE *idam_plugin_interface, PGconn* DBConnect);
int do_help(IDAM_PLUGIN_INTERFACE* idam_plugin_interface);

#ifdef __cplusplus
}
#endif

#endif /* XPADTREE_H_ */
