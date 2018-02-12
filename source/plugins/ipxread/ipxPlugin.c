//
// Created by lkogan on 09/02/18.
//
// Return IPX files from the data archive. Option to specify format so that web-compatible formats (eg. mp4, webm)
// could be produced intershot and retrieved from the archive.
//
// POSSIBLE TO DOS
//
// - On-the-fly conversion from IPX -> other formats if not already available in the archive
// - Decode IPX and return frames as data structure
//

#include "ipxPlugin.h"

#include <clientserver/initStructs.h>
#include <clientserver/stringUtils.h>
#include <structures/struct.h>


int udaIpx(IDAM_PLUGIN_INTERFACE *idam_plugin_interface)
{
    if (idam_plugin_interface->interfaceVersion > THISPLUGIN_MAX_INTERFACE_VERSION) {
        RAISE_PLUGIN_ERROR("Plugin Interface Version Unknown to this plugin: Unable to execute the request!");
    }

    idam_plugin_interface->pluginVersion = THISPLUGIN_VERSION;

    REQUEST_BLOCK* request_block = idam_plugin_interface->request_block;

    //----------------------------------------------------------------------------------------
    // Plugin Functions
    //----------------------------------------------------------------------------------------
    if (STR_IEQUALS(request_block->function, "help")) {
        return do_help(idam_plugin_interface);
    } else if (STR_IEQUALS(request_block->function, "read")) {
        return do_ipx_read(idam_plugin_interface);
    } else {
        RAISE_PLUGIN_ERROR("Unknown function requested!");
    }
}

int do_help(IDAM_PLUGIN_INTERFACE* idam_plugin_interface)
{
    DATA_BLOCK* data_block = idam_plugin_interface->data_block;

    char* p = (char*)malloc(sizeof(char) * 2 * 1024);

    strcpy(p, "\nipx: Retrieve raw ipx files from the data archive\n\n");

    initDataBlock(data_block);

    data_block->rank = 1;
    data_block->dims = (DIMS*)malloc(data_block->rank * sizeof(DIMS));

    int i;
    for (i = 0; i < data_block->rank; i++) {
        initDimBlock(&data_block->dims[i]);
    }

    data_block->data_type = UDA_TYPE_STRING;
    strcpy(data_block->data_desc, "ipx: help = description of this plugin");

    data_block->data = p;

    data_block->dims[0].data_type = UDA_TYPE_UNSIGNED_INT;
    data_block->dims[0].dim_n = strlen(p) + 1;
    data_block->dims[0].compressed = 1;
    data_block->dims[0].dim0 = 0.0;
    data_block->dims[0].diff = 1.0;
    data_block->dims[0].method = 0;

    data_block->data_n = data_block->dims[0].dim_n;

    strcpy(data_block->data_label, "");
    strcpy(data_block->data_units, "");

    return 0;

}

int do_ipx_read(IDAM_PLUGIN_INTERFACE* idam_plugin_interface) {
    ////////////////////////////
    // Get parameters passed by user
    const char* source = NULL;
    FIND_STRING_VALUE(idam_plugin_interface->request_block->nameValueList, source);

    const char* format = NULL;
    FIND_STRING_VALUE(idam_plugin_interface->request_block->nameValueList, format);

    const char* path = NULL;
    FIND_STRING_VALUE(idam_plugin_interface->request_block->nameValueList, path);

    if (path == NULL && (source == NULL || format == NULL)) {
        RAISE_PLUGIN_ERROR("You must either supply a ipx filepath OR a ipx source and format.\n");
    }

    // Construct filepath if it wasn't passed by the user
    char filepath[MAXSQL];

    if (path == NULL) {
        int shot = idam_plugin_interface->request_block->exp_number;

        char *archive_path = getenv("MAST_IMAGES");

        if (archive_path == NULL) {
            RAISE_PLUGIN_ERROR("MAST_IMAGES environment variable must be set\n");
        }

        // The IPX and ipx files are stored in the archive for example like:
        // $MAST_Images/030/30420/rbb030420.ipx
        sprintf(filepath, "%s/0%d/%d/%s%.6d.%s", archive_path, shot / 1000, shot, source, shot, format);
    } else {
        // Is this useful ?! You could just call the bytes plugin directly.....
        sprintf(filepath, "%s", path);
    }

    // Construct plugin request string for the bytes plugin
    char request_string[MAXSQL];
    sprintf(request_string, "BYTES::read(path=%s)", filepath);

    UDA_LOG(UDA_LOG_DEBUG, "Calling bytesPlugin to retrieve file: %s\n", request_string);

    // Call the plugin
    int plugin_rc = callPlugin(idam_plugin_interface->pluginList, request_string, idam_plugin_interface);

    if (plugin_rc != 0) {
        RAISE_PLUGIN_ERROR("Error reading ipx file!\n");
    }

    return plugin_rc;
}
