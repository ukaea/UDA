#include "putCoordinate.h"

#include <stdlib.h>

#include <clientserver/udaTypes.h>

#include "putVariable.h"
#include "putUnits.h"
#include "putGroup.h"
#include "putOpenClose.h"

static int writeCoordinateArray(PUTDATA_BLOCK* putdata, int grpid, const char* name, int dimid,
                                int* varid, size_t* length, int chunksize, int compression, int ctype, int dctype);

static int writeCoordinateAttribute(PUTDATA_BLOCK* putdata, int grpid, char* attribute, nc_type atype, int varid,
                                    size_t* length, size_t* totlength, int ctype, int dctype);

static int createCoordinateArray(IDAM_PLUGIN_INTERFACE* idam_plugin_interface, int grpid, int varid,
                                 int dimid, int compression, int chunksize, size_t* totlength);

int do_dimension(IDAM_PLUGIN_INTERFACE* idam_plugin_interface)
{
    int fileid = -1;
    FIND_REQUIRED_INT_VALUE(idam_plugin_interface->request_block->nameValueList, fileid);

    int ncfileid = get_file_id(fileid);
    if (ncfileid < 0) {
        RAISE_PLUGIN_ERROR("Invalid fileid given");
    }

    //---------------------------------------------------------------------------
    // Length Parameter

    int length = 0;
    FIND_INT_VALUE(idam_plugin_interface->request_block->nameValueList, length);

    //--------------------------------------------------------------------------
    // Write Dimension Entry

    char* group;
    FIND_REQUIRED_STRING_VALUE(idam_plugin_interface->request_block->nameValueList, group);

    char* name;
    FIND_REQUIRED_STRING_VALUE(idam_plugin_interface->request_block->nameValueList, name);

    IDAM_LOGF(LOG_DEBUG, "Writing Dimension %s = %d\n", name, length);
    IDAM_LOGF(LOG_DEBUG, "to Group %s \n", group);

    int unlimited = findValue(&idam_plugin_interface->request_block->nameValueList, "unlimited");

    int grpid = -1;
    int status = 0;
    if (testgroup(ncfileid, group, &status, &grpid) != NC_NOERR) {
        RAISE_PLUGIN_ERROR("Failed to find or create group");
    }

    int dimid = 0;

    if (unlimited || length == 0) {
        IDAM_LOG(LOG_DEBUG, "Dimension is UNLIMITED");
        if (nc_def_dim(grpid, name, NC_UNLIMITED, &dimid) != NC_NOERR) {
            RAISE_PLUGIN_ERROR("Unable to Write the Group Dimension");
        }
    } else {
        if (nc_def_dim(grpid, name, (size_t) length, &dimid) != NC_NOERR) {
            RAISE_PLUGIN_ERROR("Unable to Write the Group Dimension");
        }
    }

    IDAM_LOGF(LOG_DEBUG, "Dimension #id = %d\n", dimid);

    return 0;
}

int do_coordinate(IDAM_PLUGIN_INTERFACE* idam_plugin_interface)
{
    // Identifying Passed Parameters unambiguously
    //
    // Count:	1	coordinate array
    //		2	start and increment array
    //		3	coordinate, start and increment arrays
    //		3	start, increment and count arrays
    //		4	coordinate, start, increment and count arrays
    //
    // start and increment must be double and be either scalar or arrays with the same length
    // count is always an unsigned IDL long and be either scalar or an array with the same length as start and increment
    //
    // Coordinates and Dimensions must be in the same group.
    //---------------------------------------------------------------------------
    //---------------------------------------------------------------------------
    // Extract Keywords and Parameters

    int fileid = -1;
    FIND_REQUIRED_INT_VALUE(idam_plugin_interface->request_block->nameValueList, fileid);

    char* group;
    FIND_REQUIRED_STRING_VALUE(idam_plugin_interface->request_block->nameValueList, group);

    char* name;
    FIND_REQUIRED_STRING_VALUE(idam_plugin_interface->request_block->nameValueList, name);

    int ncfileid = get_file_id(fileid);
    if (ncfileid < 0) {
        RAISE_PLUGIN_ERROR("Invalid fileid given");
    }

    //--------------------------------------------------------------------------
    // Coordinate Data ?

    if (idam_plugin_interface->request_block->putDataBlockList.blockCount == 0) {
        RAISE_PLUGIN_ERROR("No Coordinate Data have been passed");
    }

    PUTDATA_BLOCK putdata = idam_plugin_interface->request_block->putDataBlockList.putDataBlock[0];

    int grpid = -1;
    int status = 0;
    if (testgroup(ncfileid, group, &status, &grpid) != NC_NOERR) {
        RAISE_PLUGIN_ERROR("Failed to find or create group");
    }

    //--------------------------------------------------------------------------
    // Check the named Coordinate does not already exist in this group

    int err;
    int varid = 0;

    if ((err = nc_inq_varid(grpid, name, &varid)) != NC_NOERR && err != NC_ENOTVAR) {
        RAISE_PLUGIN_ERROR("Unable to Check if the Coordinate variable already exists");
    }

    if (err == NC_NOERR) {
        RAISE_PLUGIN_ERROR("The Coordinate Variable already exists in the Group");
    }

    //--------------------------------------------------------------------------
    // Get the ID of the same named dimension

    int dimid = 0;
    if (nc_inq_dimid(grpid, name, &dimid) != NC_NOERR) {
        RAISE_PLUGIN_ERROR("Unable to Identify Dimensions within Group");
    }

    IDAM_LOGF(LOG_DEBUG, "Coordinate Dimension %s has ID %d\n", name, ncdimid);

    //--------------------------------------------------------------------------
    // Translate IDL to netCDF4 Type (Type is always that of the first parameter)

    int ctype = 0;
    int dctype = 0;
    if (get_complex_types(ncfileid, &ctype, &dctype)) {
        RAISE_PLUGIN_ERROR("Unable to retrieve complex data types from file");
    }

    int nctype;
    if ((nctype = swapType(putdata.data_type, ctype, dctype)) == NC_NAT) {
        RAISE_PLUGIN_ERROR("The Data type for the Coordinate cannot be converted into netCDF4");
    }

    //--------------------------------------------------------------------------
    // Create the Coordinate Variable (always rank 1)

    IDAM_LOGF(LOG_DEBUG, "Creating the Coordinate Variable %s [%d]\n", name, nctype);

    if (nc_def_var(grpid, name, nctype, 1, &dimid, &varid) != NC_NOERR) {
        RAISE_PLUGIN_ERROR("Unable to Create the Coordinate Variable");
    }

    //--------------------------------------------------------------------------
    // Write the Coordinates: Depends on the Number of Optional Parameters passed

    size_t coordcount;

    int chunksize = -1;
    FIND_INT_VALUE(idam_plugin_interface->request_block->nameValueList, chunksize);

    int compression = -1;
    FIND_INT_VALUE(idam_plugin_interface->request_block->nameValueList, compression);

    float* starts = NULL;
    size_t nstarts = 0;
    FIND_FLOAT_ARRAY(idam_plugin_interface->request_block->nameValueList, starts);

    float* increments = NULL;
    size_t nincrements = 0;
    FIND_FLOAT_ARRAY(idam_plugin_interface->request_block->nameValueList, increments);

    int* counts = NULL;
    size_t ncounts = 0;
    FIND_INT_ARRAY(idam_plugin_interface->request_block->nameValueList, counts);

    if (starts == NULL && increments == NULL && counts == NULL) {
        if (writeCoordinateArray(&putdata, grpid, name, dimid, &varid, &coordcount, chunksize, compression, ctype, dctype) != NC_NOERR) {
            RAISE_PLUGIN_ERROR("Failed to write coordinate array");
        }
    } else if (starts != NULL && increments != NULL) {
        size_t totlength = 0;
        if (createCoordinateArray(idam_plugin_interface, grpid, varid, dimid, compression, chunksize, &coordcount) != 0) {
            RAISE_PLUGIN_ERROR("Failed to create coordinate array");
        }

        size_t attlength0 = 0;
        PUTDATA_BLOCK start_putdata = {
            .data_type = TYPE_DOUBLE,
            .rank = 1,
            .count = (unsigned int)nstarts,
            .shape = NULL,
            .data = (char*)starts
        };

        if (writeCoordinateAttribute(&start_putdata, grpid, "start", NC_DOUBLE, varid, &attlength0, &totlength, ctype, dctype) != 0) {
            RAISE_PLUGIN_ERROR("Failed to write start array");
        }

        PUTDATA_BLOCK increment_putdata = {
            .data_type = TYPE_DOUBLE,
            .rank = 1,
            .count = (unsigned int)nincrements,
            .shape = NULL,
            .data = (char*)increments
        };

        size_t attlength1 = 0;
        if (writeCoordinateAttribute(&increment_putdata, grpid, "increment", NC_DOUBLE, varid, &attlength1, &totlength, ctype, dctype) != 0) {
            RAISE_PLUGIN_ERROR("Failed to write increment array");
        }

        if (counts != NULL) {
            PUTDATA_BLOCK count_putdata = {
                .data_type = TYPE_DOUBLE,
                .rank = 1,
                .count = (unsigned int)ncounts,
                .shape = NULL,
                .data = (char*)counts
            };

            size_t attlength2 = 0;
            if (writeCoordinateAttribute(&count_putdata, grpid, "increment", NC_DOUBLE, varid, &attlength2, &totlength, ctype, dctype) != 0) {
                RAISE_PLUGIN_ERROR("Failed to write increment array");
            }
        }
    } else {
        RAISE_PLUGIN_ERROR("Both starts and increments must be provided, or neither");
    }

    // Write Attributes

    char* label = NULL;
    FIND_STRING_VALUE(idam_plugin_interface->request_block->nameValueList, label);

    if (label != NULL) {
        if (nc_put_att_text(grpid, varid, "label", strlen(label), label) != NC_NOERR) {
            RAISE_PLUGIN_ERROR("Unable to Write the Label Attribute");
        }
    }

    char* class = NULL;
    FIND_STRING_VALUE(idam_plugin_interface->request_block->nameValueList, class);

    if (class != NULL) {
        if (nc_put_att_text(grpid, varid, "class", strlen(class), class) != NC_NOERR) {
            RAISE_PLUGIN_ERROR("Unable to Write the Class Attribute");
        }
    }

    char* title = NULL;
    FIND_STRING_VALUE(idam_plugin_interface->request_block->nameValueList, title);

    if (title != NULL) {
        if (nc_put_att_text(grpid, varid, "title", strlen(title), title) != NC_NOERR) {
            RAISE_PLUGIN_ERROR("Unable to Write the Title Attribute");
        }
    }

    char* comment = NULL;
    FIND_STRING_VALUE(idam_plugin_interface->request_block->nameValueList, comment);

    if (comment != NULL) {
        if (nc_put_att_text(grpid, varid, "comment", strlen(comment), comment) != NC_NOERR) {
            RAISE_PLUGIN_ERROR("Unable to Write the Comment Attribute");
        }
    }

    //--------------------------------------------------------------------------
    // If there is a named error variable then check: it exists in this group; it has the same length.

    char* errors = NULL;
    FIND_STRING_VALUE(idam_plugin_interface->request_block->nameValueList, errors);

    if (errors != NULL) {

        int varerrid = 0;

        if (nc_inq_varid(grpid, errors, &varerrid) != NC_NOERR) {
            RAISE_PLUGIN_ERROR("The Coordinate variable Error variable does not exist");
        }

        int ndims = 0;

        if (nc_inq_varndims(grpid, varerrid, &ndims) != NC_NOERR) {
            RAISE_PLUGIN_ERROR("Unable to Query the number of Dimensions of the Error Variable");
        }

        if (ndims != 1) {
            RAISE_PLUGIN_ERROR("The rank of the Error Variable is Inconsistent with a Coordinate");
        }

        if (nc_inq_vardimid(grpid, varerrid, &dimid) != NC_NOERR) {
            RAISE_PLUGIN_ERROR("Unable to Query the Dimension ID of the Error Variable");
        }

        size_t lvarerr = 0;

        if (nc_inq_dimlen(grpid, dimid, &lvarerr) != NC_NOERR) {
            RAISE_PLUGIN_ERROR("Unable to Query the Dimension Length of the Error Variable");
        }

        if (coordcount != lvarerr) {
            RAISE_PLUGIN_ERROR("The Length of the Error Variable is Inconsistent with the Coordinate Length");
        }

        if (nc_put_att_text(grpid, varid, "errors", strlen(errors), errors) != NC_NOERR) {
            RAISE_PLUGIN_ERROR("Unable to Write the Errors Attribute of Coordinate Variable");
        }

    }

    //--------------------------------------------------------------------------
    // Write Units: Last attribute written - in case units not SI compliant

    char* units = NULL;
    FIND_STRING_VALUE(idam_plugin_interface->request_block->nameValueList, units);

    int notstrict = findValue(&idam_plugin_interface->request_block->nameValueList, "notstrict");

    if (units != NULL) {
        if (!notstrict) {
            // Test for SI Compliance
            if (!testUnitsCompliance(units)) {
                RAISE_PLUGIN_ERROR("Unable to Write the Units Attribute");
            }
        }

        if (nc_put_att_text(grpid, varid, "units", strlen(units), units) != NC_NOERR) {
            RAISE_PLUGIN_ERROR("Unable to Write the Units Attribute");
        }
    }

    return 0;
}

int writeCoordinateArray(PUTDATA_BLOCK* putdata, int grpid, const char* name, int dimid,
                         int* varid, size_t* length, int chunksize, int compression, int ctype, int dctype)
{
    //--------------------------------------------------------------------------
    // Check No structures or Strings have been passed

    if (putdata->data_type == TYPE_OPAQUE || putdata->data_type == TYPE_STRING) {
        RAISE_PLUGIN_ERROR("Neither Structured or String Data arrays can be passed for Coordinates");
    }

    //--------------------------------------------------------------------------
    // Translate IDL to netCDF4 Type

    int nctype;
    if ((nctype = swapType(putdata->data_type, ctype, dctype)) == NC_NAT) {
        RAISE_PLUGIN_ERROR("The Data type for the Coordinate cannot be converted into netCDF4");
    }

    //--------------------------------------------------------------------------
    // Check the Rank

    if (putdata->rank > 1) {
        RAISE_PLUGIN_ERROR("Data for the Coordinate must be either a scalar or a rank 1 array");
    }

    *length = putdata->count;

    //--------------------------------------------------------------------------
    // Is the dimension of Unlimited Length?

    int ndims = 0;
    int dimids[NC_MAX_DIMS];

    if (nc_inq_unlimdims(grpid, &ndims, dimids) != NC_NOERR) {
        RAISE_PLUGIN_ERROR("Unable to List Unlimited Dimensions in Group")
    }

    int isUnlimited = 0;
    int i;

    for (i = 0; i < ndims; i++) {
        if (dimid == dimids[i]) {
            isUnlimited = 1;
            break;
        }
    }

    int dimlength = 0;

    if (!isUnlimited) {
        // Get current length of the dimension
        if (nc_inq_dimlen(grpid, dimid, (size_t*) &dimlength) != NC_NOERR) {
            RAISE_PLUGIN_ERROR("Unable to obtain Length of Dimension");
        }

        // Test Length is Consistent with the Dimension
        if (*length != dimlength) {
            RAISE_PLUGIN_ERROR("An Inconsistency with the Length of the Limited Coordinate Variable has been detected:"
                                       "the Data Length is not the same as Dimension length");
        }

        IDAM_LOGF(LOG_DEBUG, "Writing Data to the Limited Length Coordinate Variable %s\n", name);
    }

    //--------------------------------------------------------------------------
    // Chunking is always ON (Only for LIMITED length arrays)

    if (!isUnlimited && *length > 1) {
        size_t chunking = *length;
        if (chunksize > 0) {
            chunking = (size_t) chunksize;
        }

        IDAM_LOGF(LOG_DEBUG, "Using Chunking = %ux\n", chunking);

        if (nc_def_var_chunking(grpid, *varid, NC_CHUNKED, &chunking) != NC_NOERR) {
            RAISE_PLUGIN_ERROR("Unable to set the Chunking Properties of Dimension");
        }
    }

    //--------------------------------------------------------------------------
    // Compression (Only for LIMITED length arrays) (What is the minimum length where a benefit is accrued?)

    if (!isUnlimited && compression > 0 && *length > 1) {
        IDAM_LOGF(LOG_DEBUG, "Using Compression level %d\n", compression);
        if (nc_def_var_deflate(grpid, *varid, NC_NOSHUFFLE, 1, compression) != NC_NOERR) {
            RAISE_PLUGIN_ERROR("Unable to set the Compression Properties of Dimension");
        }
    }

    //--------------------------------------------------------------------------
    // Write the Data

    size_t start[1] = { 0 };    // For Unlimited Dimensions
    size_t count[1];
    count[0] = *length; // For unlimited dimensions, use the length of the data array

    int err = NC_NOERR;

    switch (nctype) {
        case NC_FLOAT:
            if (isUnlimited) {
                err = nc_put_vara_float(grpid, *varid, start, count, (float*) putdata->data);
            } else {
                err = nc_put_var_float(grpid, *varid, (float*) putdata->data);
            }
            break;

        case NC_DOUBLE:
            if (isUnlimited) {
                err = nc_put_vara_double(grpid, *varid, start, count, (double*) putdata->data);
            } else {
                err = nc_put_var_double(grpid, *varid, (double*) putdata->data);
            }
            break;

        case NC_INT64:
            if (isUnlimited) {
                err = nc_put_vara_longlong(grpid, *varid, start, count, (long long*) putdata->data);
            } else {
                err = nc_put_var_longlong(grpid, *varid, (long long*) putdata->data);
            }
            break;

        case NC_INT:
            if (isUnlimited) {
                err = nc_put_vara_int(grpid, *varid, start, count, (int*) putdata->data);
            } else {
                err = nc_put_var_int(grpid, *varid, (int*) putdata->data);
            }
            break;

        case NC_SHORT:
            if (isUnlimited) {
                err = nc_put_vara_short(grpid, *varid, start, count, (short*) putdata->data);
            } else {
                err = nc_put_var_short(grpid, *varid, (short*) putdata->data);
            }
            break;

        case NC_UINT64:
            if (isUnlimited) {
                err = nc_put_vara_ulonglong(grpid, *varid, start, count, (unsigned long long*) putdata->data);
            } else {
                err = nc_put_var_ulonglong(grpid, *varid, (unsigned long long*) putdata->data);
            }
            break;

        case NC_UINT:
            if (isUnlimited) {
                err = nc_put_vara_uint(grpid, *varid, start, count, (unsigned int*) putdata->data);
            } else {
                err = nc_put_var_uint(grpid, *varid, (unsigned int*) putdata->data);
            }
            break;

        case NC_USHORT:
            if (isUnlimited) {
                err = nc_put_vara_ushort(grpid, *varid, start, count, (unsigned short*) putdata->data);
            } else {
                err = nc_put_var_ushort(grpid, *varid, (unsigned short*) putdata->data);
            }
            break;

        case NC_BYTE:
            if (isUnlimited) {
                err = nc_put_vara_schar(grpid, *varid, start, count, (signed char*) putdata->data);
            } else {
                err = nc_put_var_schar(grpid, *varid, (signed char*) putdata->data);
            }
            break;

        default:
            if (nctype == ctype || nctype == dctype) {
                IDAM_LOGF(LOG_DEBUG, "the Coordinate Variable %s is COMPLEX [%d]\n", name, nctype);
                if (isUnlimited) {
                    err = nc_put_vara(grpid, *varid, start, count, (void*) putdata->data);
                } else {
                    err = nc_put_var(grpid, *varid, (void*) putdata->data);
                }
            }
            break;
    }

    //--------------------------------------------------------------------------

    if (err != NC_NOERR) {
        RAISE_PLUGIN_ERROR(nc_strerror(err));
    }

    return 0;
}


int writeCoordinateAttribute(PUTDATA_BLOCK* putdata, int grpid, char* attribute, nc_type atype, int varid,
                             size_t* length, size_t* totlength, int ctype, int dctype)
{
    //--------------------------------------------------------------------------
    // Check No structures or Strings have been passed

    if (putdata->data_type == TYPE_OPAQUE || putdata->data_type == TYPE_STRING) {
        RAISE_PLUGIN_ERROR("Neither Structures or String arrays can be passed for Coordinates Domain Attribute");
    }

    //--------------------------------------------------------------------------
    // Translate IDL to netCDF4 Type

    nc_type nctype = 0;

    if ((nctype = swapType(putdata->data_type, ctype, dctype)) == NC_NAT) {
        RAISE_PLUGIN_ERROR("The Data type for the Coordinate Domain Attribute cannot be converted into netCDF4");
    }

    //--------------------------------------------------------------------------
    // Is the Type Compliant?

    if (nctype != atype) {
        RAISE_PLUGIN_ERROR("The Data type for the Coordinate Domain Attribute must be an scalar or array");
    }

    //--------------------------------------------------------------------------
    // Check the Rank

    if (putdata->rank > 1) {
        RAISE_PLUGIN_ERROR("Data for the Coordinate Domain Attribute must be either a scalar or a rank 1 array");
    }

    //--------------------------------------------------------------------------
    // Write the Attribute

    int err;

    switch (nctype) {

        case NC_DOUBLE:
            err = nc_put_att_double(grpid, varid, attribute, nctype, *length, (double*) putdata->data);
            break;

        case NC_UINT:
            err = nc_put_att_uint(grpid, varid, attribute, nctype, *length, (unsigned int*) putdata->data);

            if (*length > 1) {
                unsigned int* udata = (unsigned int*) putdata->data;
                *totlength = 0;
                int i;
                for (i = 0; i < *length; i++) {
                    *totlength = *totlength + udata[i]; // Total length of the expanded data array
                }
            } else {
                *totlength = *((unsigned int*) putdata->data);
            }

            break;

        default: RAISE_PLUGIN_ERROR(
                "The Data type for the Coordinate Domain Attribute must be a double or unsigned int");

    }

    if (err != NC_NOERR) {
        RAISE_PLUGIN_ERROR(nc_strerror(err));
    }

    return 0;
}


int createCoordinateArray(IDAM_PLUGIN_INTERFACE* idam_plugin_interface, int grpid, int varid,
                          int dimid, int compression, int chunksize, size_t* totlength)
{
    //--------------------------------------------------------------------------
    // Is the dimension of Unlimited Length?

    int ndims = 0;
    int dimids[NC_MAX_DIMS];

    if (nc_inq_unlimdims(grpid, &ndims, dimids) != NC_NOERR) {
        RAISE_PLUGIN_ERROR("Unable to List Unlimited Dimensions");
    }

    int isUnlimited = 0;
    int i;

    for (i = 0; i < ndims; i++) {
        if (dimid == dimids[i]) {
            isUnlimited = 1;
            break;
        }
    }

    int dimlength = 0;

    if (!isUnlimited) {
        // Get current length of the dimension
        if (nc_inq_dimlen(grpid, dimid, (size_t*)&dimlength) != NC_NOERR) {
            RAISE_PLUGIN_ERROR("Unable to obtain Length of Dimension");
        }
    }

    float* starts = NULL;
    size_t nstarts = 0;
    FIND_REQUIRED_FLOAT_ARRAY(idam_plugin_interface->request_block->nameValueList, starts);

    float* increments = NULL;
    size_t nincrements = 0;
    FIND_REQUIRED_FLOAT_ARRAY(idam_plugin_interface->request_block->nameValueList, increments);

    float* counts = NULL;
    size_t ncounts = 0;
    FIND_FLOAT_ARRAY(idam_plugin_interface->request_block->nameValueList, counts);

    if (nstarts != nincrements) {
        RAISE_PLUGIN_ERROR("Length of the starts array must be equal to length of the increments array");
    }

    if (counts != NULL && (ncounts != 1 && ncounts != nstarts)) {
        RAISE_PLUGIN_ERROR("Length of the counts array must be 1 or equal to the length of the starts array");
    }

    //--------------------------------------------------------------------------
    // Build the Array

    if (counts != NULL) {
        *totlength = 0;
        int j;
        for (j = 0; j < ncounts; j++) {
            *totlength += counts[j];
        }
    } else {
        *totlength = nstarts;
    }

    IDAM_LOGF(LOG_DEBUG, "Total Length = %d\n", *totlength);

    // Test the Length is Consistent with the Dimension

    if (*totlength != dimlength) {
        RAISE_PLUGIN_ERROR("An Inconsistency with the Length of the Limited Coordinate Variable has been detected: "
                                   "the calculated Data Length is not the same as Dimension length");
    }

    double* data = (double*) malloc(*totlength * sizeof(double));

    if (counts != NULL) {
        int j;
        for (j = 0; j < ncounts; j++) {
            for (i = 0; i < counts[j]; i++) {
                data[i] = starts[j] + (double) i * increments[j];
            }
        }
    } else {
        int j;
        int n = 0;
        size_t length = *totlength / nstarts;
        for (j = 0; j < nstarts; j++) {
            for (i = 0; i < length; i++) {
                data[n] = starts[j] + (double) i * increments[j];
                n++;
            }
        }
    }

    //--------------------------------------------------------------------------
    // Compression (Only for LIMITED length arrays) (What is the minimum length where a benefit is accrued?)

    if (!isUnlimited && compression > 0 && *totlength > 1) {
        RAISE_PLUGIN_ERROR("Unable to set the Compression Properties");
    }

    //--------------------------------------------------------------------------
    // Chunking is always ON (Only for LIMITED length arrays)

    if (!isUnlimited && *totlength > 1) {
        size_t chunking = *totlength;
        if (chunksize > 0) {
            chunking = (size_t)chunksize;
        }

        if (nc_def_var_chunking(grpid, varid, NC_CHUNKED, &chunking) != NC_NOERR) {
            RAISE_PLUGIN_ERROR("Unable to set the Chunking Properties");
        }
    }

    //--------------------------------------------------------------------------
    // Write the Coordinate Array

    int err;

    if (isUnlimited) {
        size_t start[1] = { 0 };    // For Unlimited Dimensions
        size_t count[1];
        count[0] = *totlength;
        err = nc_put_vara_double(grpid, varid, start, count, data);
    } else {
        err = nc_put_var_double(grpid, varid, data);
    }

    //--------------------------------------------------------------------------
    // Housekeeping

    free((void*) data);

    if (err != NC_NOERR) {
        RAISE_PLUGIN_ERROR(nc_strerror(err));
    }

    return 0;
}

