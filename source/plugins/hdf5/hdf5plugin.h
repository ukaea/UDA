#ifndef UDA_PLUGIN_HDF5PLUGIN_H
#define UDA_PLUGIN_HDF5PLUGIN_H

#include <hdf5.h>
#include <H5LTpublic.h>
#ifdef __GNUC__
#  include <sys/time.h>
#endif

#include <plugins/udaPluginFiles.h>
#include "udaPlugin.h"
#include "export.h"

#ifdef __cplusplus
extern "C" {
#endif

#define THISPLUGIN_VERSION                  1
#define THISPLUGIN_MAX_INTERFACE_VERSION    1
#define THISPLUGIN_DEFAULT_METHOD           "get"

extern UDA_PLUGIN_FILE_LIST pluginFileList;

LIBRARY_API int udaHDF5(IDAM_PLUGIN_INTERFACE* idam_plugin_interface);
LIBRARY_API int readHDF5IdamType(H5T_class_t classtype, int precision, int issigned);
LIBRARY_API int readHDF5Att(hid_t file_id, char* object, hid_t att_id, char* attname, DATA_BLOCK* data_block);

#ifdef __cplusplus
}
#endif

#endif // UDA_PLUGIN_HDF5PLUGIN_H
