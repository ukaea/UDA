#include "putDevice.h"

#include <netcdf.h>

#include "putGroup.h"
#include "putOpenClose.h"

int do_device(IDAM_PLUGIN_INTERFACE* idam_plugin_interface)
{

    //---------------------------------------------------------------------------
    // Extract Keywords

    int fileid = 0;
    const char* device = NULL;

    FIND_REQUIRED_INT_VALUE(idam_plugin_interface->request_block->nameValueList, fileid);
    FIND_REQUIRED_STRING_VALUE(idam_plugin_interface->request_block->nameValueList, device);

    int ncfileid = get_file_id(fileid);
    if (ncfileid < 0) {
        RAISE_PLUGIN_ERROR("Invalid fileid given");
    }

    //---------------------------------------------------------------------------
    // Device Name Parameter

    IDAM_LOGF(UDA_LOG_DEBUG, "The Device is named %s\n", device);

    //--------------------------------------------------------------------------
    // Check the Top Level Group Named 'Devices' exists - Create if not

    int status;
    int grpid;

    if (testgroup(ncfileid, "devices", &status, &grpid) != NC_NOERR) {
        RAISE_PLUGIN_ERROR("Unable to Find or Define the Top Level 'devices' Group");
    }

    //--------------------------------------------------------------------------
    // Create a Child Group named after the device

    if (testgroup(grpid, device, &status, &grpid) != NC_NOERR) {
        RAISE_PLUGIN_ERROR("Unable to Create a Devices Child Group");
    }

    //--------------------------------------------------------------------------
    // Attributes

    const char* type = NULL;
    const char* id = NULL;
    const char* serial = NULL;

    FIND_STRING_VALUE(idam_plugin_interface->request_block->nameValueList, type);
    FIND_STRING_VALUE(idam_plugin_interface->request_block->nameValueList, id);
    FIND_STRING_VALUE(idam_plugin_interface->request_block->nameValueList, serial);

    if (type != NULL && (nc_put_att_text(grpid, NC_GLOBAL, "type", strlen(type), type) != NC_NOERR)) {
        RAISE_PLUGIN_ERROR("Unable to Write the Device Type Attribute");
    }

    if (id != NULL && (nc_put_att_text(grpid, NC_GLOBAL, "id", strlen(id), id) != NC_NOERR)) {
        RAISE_PLUGIN_ERROR("Unable to Write the Device ID Attribute");
    }

    if (serial != NULL && (nc_put_att_text(grpid, NC_GLOBAL, "serial", strlen(serial), serial) != NC_NOERR)) {
        RAISE_PLUGIN_ERROR("Unable to Write the Device Serial Attribute");
    }

    short resolution = -1;
    FIND_SHORT_VALUE(idam_plugin_interface->request_block->nameValueList, resolution);

    if (resolution > 0) {
        if (nc_put_att_short(grpid, NC_GLOBAL, "resolution", NC_SHORT, 1, &resolution) != NC_NOERR) {
            RAISE_PLUGIN_ERROR("Unable to Write the Device Resolution Attribute");
        }
    }

    float* range = NULL;
    size_t nrange = 0;
    FIND_FLOAT_ARRAY(idam_plugin_interface->request_block->nameValueList, range);

    if (range != NULL) {
        if (nrange != 2) {
            RAISE_PLUGIN_ERROR("Range must be given as an array of 2 floats");
        }
        if (nc_put_att_float(grpid, NC_GLOBAL, "range", NC_FLOAT, 2, range) != NC_NOERR) {
            RAISE_PLUGIN_ERROR("Unable to Write the Device Range");
        }
    }

    short channels = -1;
    FIND_SHORT_VALUE(idam_plugin_interface->request_block->nameValueList, channels);

    if (channels > 0) {
        if (nc_put_att_short(grpid, NC_GLOBAL, "channels", NC_SHORT, 1, &channels) != NC_NOERR) {
            RAISE_PLUGIN_ERROR("Unable to Write the Device Channel Count Attribute");
        }
    }

    return 0;
} 

