#pragma once

#include <string>
#include <uda/plugins.h>
#include <hdf5.h>
#include <H5Tpublic.h>

#include "include/uda/uda_plugin_base.hpp"

class HDF5Plugin : public UDAPluginBase
{
public:
    HDF5Plugin();
    int read(UDA_PLUGIN_INTERFACE* plugin_interface);
    int hello(UDA_PLUGIN_INTERFACE* plugin_interface)
    {
        return udaPluginReturnDataStringScalar(plugin_interface, "hello!", nullptr);
    }
    static int foo(UDA_PLUGIN_INTERFACE* plugin_interface)
    {
        return udaPluginReturnDataStringScalar(plugin_interface, "foo!", nullptr);
    }
    void init(UDA_PLUGIN_INTERFACE* plugin_interface) override {}
    void reset() override {}

private:
    int read_hdf5(UDA_PLUGIN_INTERFACE* plugin_interface, const std::string& file_path, const std::string& data_path);
    int read_hdf5_att(UDA_PLUGIN_INTERFACE* plugin_interface, hid_t file_id, const std::string& grp_name, hid_t att_id, const std::string& att_name);
};

//#ifdef NOHDF5PLUGIN
//
//void H5Fclose(int fh);
//
//#else
//
//#  include <hdf5.h>
//
//#  define HDF5_ERROR_OPENING_FILE 200
//#  define HDF5_ERROR_IDENTIFYING_DATA_ITEM 201
//#  define HDF5_ERROR_OPENING_DATASPACE 202
//#  define HDF5_ERROR_ALLOCATING_DIM_HEAP 203
//#  define HDF5_ERROR_ALLOCATING_DATA_HEAP 204
//#  define HDF5_ERROR_READING_DATA 205
//#  define HDF5_ERROR_OPENING_ATTRIBUTE 206
//#  define HDF5_ERROR_NO_STORAGE_SIZE 207
//#  define HDF5_ERROR_UNKNOWN_TYPE 208
//#  define HDF5_ERROR_OPENING_DATASET 209
//
//int readHDF5IdamType(H5T_class_t classtype, int precision, int issigned);
//int readHDF5Att(hid_t file_id, char* object, hid_t att_id, char* attname, uda::client_server::DataBlock* data_block);
//
//#endif
