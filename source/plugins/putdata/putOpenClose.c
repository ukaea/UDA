#include "putOpenClose.h"

#include <netcdf.h>
#include <stdlib.h>
#include <strings.h>

#include <clientserver/stringUtils.h>
#include <clientserver/udaTypes.h>

typedef struct {
    int count;
    int* ids;
    unsigned int* compliance;
} FileIds;

static FileIds fileIds;

int get_complex_types(int fileid, int* ctype, int* dctype)
{
    static int cache_ctype = -1;
    static int cache_dctype = -1;

    if (cache_ctype >=0 && cache_dctype >= 0) {
        *ctype = cache_ctype;
        *dctype = cache_dctype;
        return 0;
    }

    // Must have been defined previously when the file was created
    IDAM_LOG(LOG_DEBUG, "Listing Defined Data Types\n");

    int ntypes;
    if (nc_inq_typeids(fileid, &ntypes, NULL) != NC_NOERR || ntypes == 0) {
        RAISE_PLUGIN_ERROR("Unable to Quantify Data types");
    }

    int* typeids = (int*) malloc(ntypes * sizeof(int));
    if (nc_inq_typeids(fileid, &ntypes, typeids) != NC_NOERR) {
        RAISE_PLUGIN_ERROR("Unable to List Data types");
    }

    char typename[NC_MAX_NAME + 1];
    int i;

    for (i = 0; i < ntypes; i++) {
        if (nc_inq_compound_name(fileid, (nc_type) typeids[i], typename) != NC_NOERR) {
            RAISE_PLUGIN_ERROR("Unable to List Data types");
        }
        IDAM_LOGF(LOG_DEBUG, "Data Type %d Name: %s\n", typeids[i], typename);
        if (STR_EQUALS(typename, "complex"))   *ctype = typeids[i];
        if (STR_EQUALS(typename, "dcomplex")) *dctype = typeids[i];
    }

    free((void*)typeids);

    cache_ctype = *ctype;
    cache_dctype = *dctype;

    return 0;
}

int get_file_id(int fileidx)
{
    if (fileidx >= 0 && fileidx < fileIds.count) {
        return fileIds.ids[fileidx];
    }
    return 0;
}

int do_open(IDAM_PLUGIN_INTERFACE* idam_plugin_interface)
{
    const char* format = NULL;
    FIND_STRING_VALUE(idam_plugin_interface->request_block->nameValueList, format);

    //-------------------------------------------------------------------------
    // Check File Format Specified

    if (format != NULL) {
        if (!STR_IEQUALS(format, "NETCDF")) {
            RAISE_PLUGIN_ERROR("Only netCDF4 File Formats are Implemented via this API!");
        }
    }

    //--------------------------------------------------------------------------
    // Open the File: Create or Update

    const char* filename = NULL;
    FIND_REQUIRED_STRING_VALUE(idam_plugin_interface->request_block->nameValueList, filename);

    IDAM_LOGF(LOG_DEBUG, "The filename is %s\n", filename);

    const char* directory = NULL;
    FIND_STRING_VALUE(idam_plugin_interface->request_block->nameValueList, directory);

    char* path = NULL;

    if (directory != NULL) {
        path = malloc(sizeof(char) * (strlen(directory) + strlen(filename) + 5));
        sprintf(path, "%s/%s", directory, filename);
    } else {
        path = malloc(sizeof(char) * (strlen(filename) + 3));
        if (strstr(filename, "/") == NULL)
            sprintf(path, "./%s", filename);        // Local directory
        else
            sprintf(path, "%s", filename);          // Contains directory
    }

    int create = findValue(&idam_plugin_interface->request_block->nameValueList, "create");

    int fileid = 0;
    int update = 0;

    int err;

    IDAM_LOGF(LOG_DEBUG, "The path is %s\n", path);

    if ((err = nc_open(path, NC_WRITE, &fileid)) != NC_NOERR) {
        if (create) {
            if ((err = nc_create(path, NC_CLOBBER | NC_NETCDF4, &fileid)) != NC_NOERR) {
                IDAM_LOGF(LOG_ERROR, "error creating netcdf file %s: %s\n", path, nc_strerror(err));
                RAISE_PLUGIN_ERROR("Unable to Create the requested netCDF4 File");
            }
            IDAM_LOGF(LOG_DEBUG, "Created the requested netCDF4 File: %d\n", fileid);
        } else {
            IDAM_LOGF(LOG_ERROR, "error opening netcdf file %s: %s\n", path, nc_strerror(err));
            free((void*)path);
            RAISE_PLUGIN_ERROR("Unable to Open the requested netCDF4 File");
        }
    } else {
        IDAM_LOGF(LOG_DEBUG, "Opened the requested netCDF4 File for Update: %d\n", fileid);
        update = 1;
    }

    free((void*)path);

    fileIds.count++;
    fileIds.ids = (int*)realloc(fileIds.ids, fileIds.count * sizeof(int));
    fileIds.compliance = (unsigned int*)realloc(fileIds.compliance, fileIds.count * sizeof(unsigned int));

    int fileIdx = fileIds.count - 1;
    fileIds.ids[fileIdx] = fileid;
    fileIds.compliance[fileIdx] = 0;

    //--------------------------------------------------------------------------
    // Name and Version of File Generator Code

    if (create) {
        char text[200];
        sprintf(text, "IDAM PutData VERSION %d (%s)", PLUGIN_VERSION, __DATE__);
        if (nc_put_att_text(fileid, NC_GLOBAL, "generator", strlen(text), text) != NC_NOERR) {
            IDAM_LOGF(LOG_WARN, "Unable to Write the File Generator Attribute: %s\n", text);
        }
    }

    //--------------------------------------------------------------------------
    // Current Compliance value

    unsigned int compliance = 0;

    if (update) {
        if (nc_get_att_uint(fileid, NC_GLOBAL, "compliance", &compliance) != NC_NOERR) {
            fileIds.compliance[fileIdx] = 0;
            compliance = 0;
        } else {
            fileIds.compliance[fileIdx] = compliance;
        }
    }

    IDAM_LOG(LOG_DEBUG, "FileIds\n");
    int i;
    for (i = 0; i < fileIds.count; i++) {
        IDAM_LOGF(LOG_DEBUG, "FileIds[%d]: %d %d\n", i, fileIds.ids[i], fileIds.compliance[i]);
    }

    //--------------------------------------------------------------------------
    // Specify user Defined types: COMPLEX and DCOMPLEX

    if (!update) {
        // Single Precision Complex

        int ctype = 0;
        if (nc_def_compound(fileid, sizeof(COMPLEX), "complex", &ctype) != NC_NOERR) {
            RAISE_PLUGIN_ERROR("Unable to Define the User Defined Complex type");
        }
        if (nc_insert_compound(fileid, ctype, "real", (size_t)0, NC_FLOAT) != NC_NOERR) {
            RAISE_PLUGIN_ERROR("Unable to Insert a data type into the User Defined Complex type");
        }
        if (nc_insert_compound(fileid, ctype, "imaginary", sizeof(float), NC_FLOAT) != NC_NOERR) {
            RAISE_PLUGIN_ERROR("Unable to Insert a data type into the User Defined Complex type");
        }

        // Double Precision Complex

        int dctype = 0;
        if (nc_def_compound(fileid,  sizeof(DCOMPLEX), "dcomplex", &dctype) != NC_NOERR) {
            RAISE_PLUGIN_ERROR("Unable to Define the User Defined Double Complex type");
        }
        if (nc_insert_compound(fileid, dctype, "real", (size_t)0, NC_DOUBLE) != NC_NOERR) {
            RAISE_PLUGIN_ERROR("Unable to Insert a data type into the User Defined Complex type");
        }
        if (nc_insert_compound(fileid, dctype, "imaginary", sizeof(double), NC_DOUBLE) != NC_NOERR) {
            RAISE_PLUGIN_ERROR("Unable to Insert a data type into the User Defined Double Complex type");
        }
    }

    //--------------------------------------------------------------------------
    // Write Root Group Attributes: Required and Optional

    const char* conventions = NULL;
    FIND_STRING_VALUE(idam_plugin_interface->request_block->nameValueList, conventions);

    if (conventions != NULL) {
        if (nc_put_att_text(fileid, NC_GLOBAL, "Conventions", strlen(conventions), conventions) != NC_NOERR) {
            RAISE_PLUGIN_ERROR("Unable to Write the Conventions Root Group Attribute");
        }
        if (STR_IEQUALS(conventions, CREATE_CONVENTIONS_TEST)) {
            compliance = compliance | CREATE_CONVENTIONS;
        }
    } else if (create) {
        RAISE_PLUGIN_ERROR("No Conventions Standard has been specified!");
    }

    const char* class = NULL;
    FIND_STRING_VALUE(idam_plugin_interface->request_block->nameValueList, class);

    if (class != NULL) {
        if (!(STR_IEQUALS(class, "Raw") || STR_IEQUALS(class, "Analysed") ||
                STR_IEQUALS(class, "Modelled"))) {
            RAISE_PLUGIN_ERROR("The File Data Class must be one of: Raw, Analysed or Modelled");
        }

        if (nc_put_att_text(fileid, NC_GLOBAL, "class", strlen(class), class) != NC_NOERR) {
            RAISE_PLUGIN_ERROR("Unable to Write the Class Root Group Attribute");
        }
        compliance = compliance | CREATE_CLASS;
    } else if (create) {
        RAISE_PLUGIN_ERROR("No File Data Class has been specified");
    }

    const char* title = NULL;
    FIND_REQUIRED_STRING_VALUE(idam_plugin_interface->request_block->nameValueList, title);

    if (nc_put_att_text(fileid, NC_GLOBAL, "title", strlen(title), title) != NC_NOERR) {
        RAISE_PLUGIN_ERROR("Unable to Write the Title Root Group Attribute");
    }

    time_t timer;
    time(&timer);
    struct tm* tm_info = localtime(&timer);

    char datetime[100] = {};
    strftime(datetime, 100, "%FT%TZ", tm_info);

    if (nc_put_att_text(fileid, NC_GLOBAL, "timestamp", strlen(datetime), datetime) != NC_NOERR) {
        RAISE_PLUGIN_ERROR("Unable to Write the Date Root Group Attribute");
    }

//    const char* date = NULL;
//    FIND_STRING_VALUE(idam_plugin_interface->request_block->nameValueList, date);
//
//    if (date != NULL) {
//        if (nc_put_att_text(fileid, NC_GLOBAL, "date", strlen(date), date) != NC_NOERR) {
//            RAISE_PLUGIN_ERROR("Unable to Write the Date Root Group Attribute");
//        }
//    } else if (create) {
//        RAISE_PLUGIN_ERROR("No Date has been specified");
//    }
//
//    const char* time = NULL;
//    FIND_STRING_VALUE(idam_plugin_interface->request_block->nameValueList, time);
//
//    if (time != NULL) {
//        if (nc_put_att_text(fileid, NC_GLOBAL, "time", strlen(time), time) != NC_NOERR) {
//            RAISE_PLUGIN_ERROR("Unable to Write the Time Root Group Attribute");
//        }
//    } else if (create) {
//        RAISE_PLUGIN_ERROR("No Time has been specified");
//    }

    int shot = -1;
    FIND_INT_VALUE(idam_plugin_interface->request_block->nameValueList, shot);

    if (shot >= 0) {
        if (nc_put_att_int(fileid, NC_GLOBAL, "shot", NC_INT, 1, &shot) != NC_NOERR) {
            RAISE_PLUGIN_ERROR("Unable to Write the Shot Number Root Group Attribute");
        }
        compliance = compliance | CREATE_SHOT;
    } else if (create && (STR_IEQUALS(class, "Raw") || STR_IEQUALS(class, "Analysed"))) {
        RAISE_PLUGIN_ERROR("A Shot or Experiment Number must be specified for New Raw or Analysed Data Files");
    } else {
        compliance = compliance | CREATE_SHOT;
    }

    int pass = -1;
    FIND_INT_VALUE(idam_plugin_interface->request_block->nameValueList, pass);

    if (pass >= 0) {
        if (nc_put_att_int(fileid, NC_GLOBAL, "pass", NC_INT, 1, &pass) != NC_NOERR) {
            RAISE_PLUGIN_ERROR("Unable to Write the Pass Number Root Group Attribute");
        }
        compliance = compliance | CREATE_PASS;
    } else if (create && STR_IEQUALS(class, "Analysed")) {
        RAISE_PLUGIN_ERROR("A Pass Number must be specified for New Analysed Data Files");
    } else {
        compliance = compliance | CREATE_PASS;
    }

    int status = -1;
    FIND_INT_VALUE(idam_plugin_interface->request_block->nameValueList, status);

    if (status >= 0) {
        if (nc_put_att_int(fileid, NC_GLOBAL, "status", NC_INT, 1, &status) != NC_NOERR) {
            RAISE_PLUGIN_ERROR("Unable to Write the File Status Number Root Group Attribute");
        }
        compliance = compliance | CREATE_STATUS;
    } else if (create && STR_IEQUALS(class, "Analysed")) {
        RAISE_PLUGIN_ERROR("A File Status Number must be specified for New Analysed Data Files");
    } else {
        compliance = compliance | CREATE_STATUS;
    }

    // Optional Attributes

    const char* comment = NULL;
    FIND_STRING_VALUE(idam_plugin_interface->request_block->nameValueList, comment);

    if (comment != NULL) {
        if (nc_put_att_text(fileid, NC_GLOBAL, "comment", strlen(comment), comment) != NC_NOERR) {
            RAISE_PLUGIN_ERROR("Unable to Write the Comment Root Group  Attribute");
        }
    }

    // *****************************************
    // Replaced by software attribute

    const char* code = NULL;
    FIND_STRING_VALUE(idam_plugin_interface->request_block->nameValueList, code);

    if (code != NULL) {
        if (nc_put_att_text(fileid, NC_GLOBAL, "code", strlen(code), code) != NC_NOERR) {
            RAISE_PLUGIN_ERROR("Unable to Write the Code Name Root Group Attribute");
        }
    }

    int version = -1;
    FIND_INT_VALUE(idam_plugin_interface->request_block->nameValueList, version);

    if (version > 0) {
        if (nc_put_att_int(fileid, NC_GLOBAL, "version", NC_INT, 1, &version) != NC_NOERR) {
            RAISE_PLUGIN_ERROR("Unable to Write the Code Version Number Root Group Attribute");
        }
    }

    const char* xml = NULL;
    FIND_STRING_VALUE(idam_plugin_interface->request_block->nameValueList, xml);

    if (xml != NULL) {
        if (nc_put_att_text(fileid, NC_GLOBAL, "xml", strlen(xml), xml) != NC_NOERR) {
            RAISE_PLUGIN_ERROR("Unable to Write the XML Root Group Attribute");
        }
    }

    fileIds.compliance[fileIdx] = compliance;

    return setReturnDataIntScalar(idam_plugin_interface->data_block, fileIdx, "file index");
}

int do_close(IDAM_PLUGIN_INTERFACE* idam_plugin_interface)
{
    int fileid = 0;
    FIND_REQUIRED_INT_VALUE(idam_plugin_interface->request_block->nameValueList, fileid);

    IDAM_LOGF(LOG_DEBUG, "Closing the requested netCDF4 File: %d\n", fileid);

    int ncfileid = get_file_id(fileid);
    if (ncfileid < 0) {
        RAISE_PLUGIN_ERROR("Invalid fileid given");
    }

    //---------------------------------------------------------------------------
    // Write the Compliance Status Test Result

    int nocompliance = findValue(&idam_plugin_interface->request_block->nameValueList, "nocompliance");

    unsigned int compliance = 0;
    if (nocompliance) {
        nc_put_att_uint(ncfileid, NC_GLOBAL, "compliance", NC_UINT, 1, &compliance);
    }

    IDAM_LOGF(LOG_DEBUG, "Compliance Test Result B: %d\n", compliance);

    //---------------------------------------------------------------------------
    // Close the File

    if (fileIds.count == 0) {
        RAISE_PLUGIN_ERROR("Error: There are No Open Files to Close!");
    }

    if (nc_close(ncfileid) != NC_NOERR) {
        RAISE_PLUGIN_ERROR("Unable to Close the requested netCDF4 File");
    }

    //---------------------------------------------------------------------------
    // Update the List of Open Files

    int i;

    fileIds.count--;
    for (i = fileid; i < fileIds.count - 1; i++) {
        fileIds.ids[i] = fileIds.ids[i + 1];
        fileIds.compliance[i] = fileIds.compliance[i + 1];
    }

    if (fileIds.count == 0) {
        free(fileIds.ids);
        free(fileIds.compliance);
        fileIds.ids = NULL;
        fileIds.compliance = NULL;
    }

    IDAM_LOGF(LOG_DEBUG, "File Closed, %d files remain open\n", fileIds.count);

    if (fileIds.ids != NULL) {
        for (i = 0; i < fileIds.count; i++) {
            IDAM_LOGF(LOG_DEBUG, "FileIds[%d]: %d %d\n", i, fileIds.ids[i], fileIds.compliance[i]);
        }
    }

    return setReturnDataIntScalar(idam_plugin_interface->data_block, 0, NULL);
}
