#ifndef UDA_PLUGIN_HDF5PLUGIN_H
#define UDA_PLUGIN_HDF5PLUGIN_H

#include <H5LTpublic.h>
#include <hdf5.h>
#ifdef __GNUC__
#  include <sys/time.h>
#endif

#include "export.h"
#include "udaPlugin.h"
#include <plugins/udaPluginFiles.h>

#ifdef __cplusplus
extern "C" {
#endif

#define THISPLUGIN_VERSION 1
#define THISPLUGIN_MAX_INTERFACE_VERSION 1
#define THISPLUGIN_DEFAULT_METHOD "get"

extern UDA_PLUGIN_FILE_LIST pluginFileList;

LIBRARY_API int hdf5Plugin(IDAM_PLUGIN_INTERFACE* plugin_interface);

#ifdef __cplusplus
}
#endif

#endif // UDA_PLUGIN_HDF5PLUGIN_H
