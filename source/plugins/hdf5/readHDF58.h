#ifndef UDA_PLUGIN_READHDF58_H
#define UDA_PLUGIN_READHDF58_H

#include "clientserver/udaStructs.h"

#ifdef __cplusplus
extern "C" {
#endif

int readHDF5(DATA_SOURCE data_source, SIGNAL_DESC signal_desc, DATA_BLOCK* data_block);

#ifdef NOHDF5PLUGIN

void H5Fclose(int fh);

#else

#  include <hdf5.h>

#  define HDF5_ERROR_OPENING_FILE 200
#  define HDF5_ERROR_IDENTIFYING_DATA_ITEM 201
#  define HDF5_ERROR_OPENING_DATASPACE 202
#  define HDF5_ERROR_ALLOCATING_DIM_HEAP 203
#  define HDF5_ERROR_ALLOCATING_DATA_HEAP 204
#  define HDF5_ERROR_READING_DATA 205
#  define HDF5_ERROR_OPENING_ATTRIBUTE 206
#  define HDF5_ERROR_NO_STORAGE_SIZE 207
#  define HDF5_ERROR_UNKNOWN_TYPE 208
#  define HDF5_ERROR_OPENING_DATASET 209

int readHDF5IdamType(H5T_class_t classtype, int precision, int issigned);
int readHDF5Att(hid_t file_id, char* object, hid_t att_id, char* attname, DATA_BLOCK* data_block);

#endif

#ifdef __cplusplus
}
#endif

#endif // UDA_PLUGIN_READHDF58_H
