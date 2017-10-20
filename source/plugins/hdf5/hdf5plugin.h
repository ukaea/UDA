#ifndef IdamNewHDF5PluginInclude
#define IdamNewHDF5PluginInclude

#ifdef __cplusplus
extern "C" {
#endif

#include <hdf5.h>
#include <H5LTpublic.h>
#include <sys/time.h>

#include <plugins/udaPluginFiles.h>
#include <plugins/udaPlugin.h>

#define THISPLUGIN_VERSION                  1
#define THISPLUGIN_MAX_INTERFACE_VERSION    1
#define THISPLUGIN_DEFAULT_METHOD           "get"

extern IDAMPLUGINFILELIST pluginFileList;

int udaHDF5(IDAM_PLUGIN_INTERFACE * idam_plugin_interface);

int readHDF5IdamType(H5T_class_t classtype, int precision, int issigned);

int readHDF5Att(hid_t file_id, char * object, hid_t att_id, char * attname, DATA_BLOCK * data_block);

#ifdef __cplusplus
}
#endif

#endif
