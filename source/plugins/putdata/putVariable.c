#include "putVariable.h"

#include <stdlib.h>

#include <clientserver/stringUtils.h>
#include <clientserver/udaTypes.h>

#include "putUnits.h"
#include "putGroup.h"
#include "putOpenClose.h"

static int testDimension(int grpid, char* dimension, int parents, int* ncdimid);

int do_variable(IDAM_PLUGIN_INTERFACE* idam_plugin_interface)
{
    //---------------------------------------------------------------------------
    //---------------------------------------------------------------------------
    // Extract Keywords and Parameters

    int fileid = 0;
    FIND_REQUIRED_INT_VALUE(idam_plugin_interface->request_block->nameValueList, fileid);

    const char* group = NULL;
    FIND_REQUIRED_STRING_VALUE(idam_plugin_interface->request_block->nameValueList, group);

    const char* name = NULL;
    FIND_REQUIRED_STRING_VALUE(idam_plugin_interface->request_block->nameValueList, name);

    const char* dimensions = NULL;
    FIND_REQUIRED_STRING_VALUE(idam_plugin_interface->request_block->nameValueList, dimensions);

    int ncfileid = get_file_id(fileid);
    if (ncfileid < 0) {
        RAISE_PLUGIN_ERROR("Invalid fileid given");
    }

    //--------------------------------------------------------------------------
    // Find or create the group

    int grpid = -1;
    if (testmakegroup(ncfileid, group, &grpid) != NC_NOERR) {
        RAISE_PLUGIN_ERROR("Failed to find or create group");
    }

    //--------------------------------------------------------------------------
    // Check the Variable does not already exist in this group
    int err;
    int varid;
    if ((err = nc_inq_varid(grpid, name, &varid)) != NC_NOERR && err != NC_ENOTVAR) {
        RAISE_PLUGIN_ERROR("Unable to Check if the Data Variable exists!");
    }

    if (err == NC_NOERR) {
        // The variable exists: Edit Mode?
        RAISE_PLUGIN_ERROR("The Data Variable already exists in the Group");
    }

    //--------------------------------------------------------------------------
    // Shape and Type

    PUTDATA_BLOCK_LIST putdata_block_list = idam_plugin_interface->request_block->putDataBlockList;
    if (putdata_block_list.blockCount != 1) {
        RAISE_PLUGIN_ERROR("Exactly one putdata block should be provided");
    }

    PUTDATA_BLOCK putdata = putdata_block_list.putDataBlock[0];

    if (putdata.data_type == UDA_TYPE_OPAQUE) {
        RAISE_PLUGIN_ERROR("Structured Data Variable Types are Not Supported");
    }

    if (putdata.data_type == UDA_TYPE_STRING) {
        RAISE_PLUGIN_ERROR("String Data Variable Types are Not Supported");
    }

    int length = putdata.count;
    int rank = putdata.rank;
    int* shape = putdata.shape;
    const char* data = putdata.data;

    IDAM_LOGF(UDA_LOG_DEBUG, "Variable %s:\n", name);
    IDAM_LOGF(UDA_LOG_DEBUG, "Rank     %d:\n", rank);
    IDAM_LOGF(UDA_LOG_DEBUG, "Length   %d:\n", length);
    IDAM_LOG(UDA_LOG_DEBUG, "Shape:\n");

    if (putdata.shape == NULL) {
      IDAM_LOG(UDA_LOG_DEBUG, "Shape is NULL\n");
    }

    int i;
    for (i = 0; i < rank; i++) {
        IDAM_LOGF(UDA_LOG_DEBUG, "    [%d]=%d\n", i, shape[i]);
    }
    IDAM_LOGF(UDA_LOG_DEBUG, "Dimensions: %s\n", dimensions);

    //--------------------------------------------------------------------------
    // Check the variable's dimension names are specified and within the scope of this group

    if (dimensions == NULL) {
        RAISE_PLUGIN_ERROR("The Variable's Dimensions must be Named as an ordered comma delimited list");
    }

    // Parse the dimensions string and Scan for the nearest (in scope) defined dimensions up the hierarchy

    char* work = (char*) malloc(sizeof(char) * strlen(dimensions) + 1);

    strcpy(work, dimensions);
    TrimString(work);
    LeftTrimString(work);

    size_t mndims = 0;
    int mdimids[100];

    char* token;
    char* tokwork = work;

    while ((token = strtok(tokwork, ";")) != NULL) {
        char* trimtoken = (char*)malloc((strlen(token) + 1) * sizeof(char));
        strcpy(trimtoken, token);
        TrimString(trimtoken);
        LeftTrimString(trimtoken);

        int dimid = -1;
        if ((nc_inq_dimid(grpid, trimtoken, &dimid)) != NC_NOERR) {
            err = testDimension(grpid, trimtoken, 1, &dimid);
            if (err != NC_NOERR || dimid < 0) {
                RAISE_PLUGIN_ERROR("Unable to Identify Dimensions from Group");
            }
        }

        mdimids[mndims] = dimid;
        ++mndims;

        IDAM_LOGF(UDA_LOG_DEBUG, "Rank: %d, mndims: %d\n", rank, mndims);
        IDAM_LOGF(UDA_LOG_DEBUG, "Coordinate Dimension %s has ID %d\n", trimtoken, ncdimid);

        free((void*) trimtoken);

        // Repeat with NULL to take next token
        tokwork = NULL;
    }

    if ((rank > 0 && mndims != rank) || (rank == 0 && mndims != 1)) {
        RAISE_PLUGIN_ERROR("The Rank of the Data Variable is not consistent with the declared number of dimensions");
    }

    //--------------------------------------------------------------------------
    // Translate IDL to netCDF4 Type

    int ctype = 0;
    int dctype = 0;
    if (get_complex_types(ncfileid, &ctype, &dctype)) {
        RAISE_PLUGIN_ERROR("Unable to retrieve complex data types from file");
    }

    int nctype = swapType(putdata.data_type, ctype, dctype);

    IDAM_LOG(UDA_LOG_DEBUG, "\nUser defined Compound Types\n ");
    IDAM_LOGF(UDA_LOG_DEBUG, "Creating the Data Variable %s \n", name);
    IDAM_LOGF(UDA_LOG_DEBUG, "Type ID %d \n", nctype);
    IDAM_LOGF(UDA_LOG_DEBUG, "Dimension Count %d \n", mndims);
    IDAM_LOG(UDA_LOG_DEBUG, "Dimension IDs\n");
    for (i = 0; i < mndims; i++) {
        IDAM_LOGF(UDA_LOG_DEBUG, "[%d]", mdimids[i]);
    }

    //--------------------------------------------------------------------------
    // Create the Variable

    if (nc_def_var(grpid, name, nctype, (int)mndims, mdimids, &varid) != NC_NOERR) {
        RAISE_PLUGIN_ERROR("Unable to Create the Data Variable");
    }

    IDAM_LOGF(UDA_LOG_DEBUG, "Variable ID %d \n", varid);

    //--------------------------------------------------------------------------
    // Get a List of the UNLIMITED Dimensions in scope of this group

    int nunlimdimids;
    int unlimdimids[NC_MAX_DIMS];

    if ((err = nc_inq_unlimdims(grpid, &nunlimdimids, unlimdimids)) != NC_NOERR) {
        RAISE_PLUGIN_ERROR("Unable to Inquire on Unlimited Dimension IDs");
    }

    IDAM_LOGF(UDA_LOG_DEBUG, "Number of Unlimited Dimensions %d", nunlimdimids);
    IDAM_LOG(UDA_LOG_DEBUG, "Unlimited Dimension IDs");
    for (i = 0; i < nunlimdimids; i++) {
        IDAM_LOGF(UDA_LOG_DEBUG, "[%d]", unlimdimids[i]);
    }

    //--------------------------------------------------------------------------
    // Shape (Reversed in C from IDL so data(a,b,c) is data[c][b][a])

    IDAM_LOGF(UDA_LOG_DEBUG, "Testing variable %s Dimension Lengths are Consistent \n", name);

    // Allocate shape array for use only if one of the dimensions is UNLIMITED

    size_t* extents = (size_t*) malloc(mndims * sizeof(size_t));

    // Check all Array dimension Lengths are consistent (Ignore UNLIMITED dimensions)

    // Is this dimension UNLIMITED?
    int isUnlimited = 0;
    int unlimitedCount = 0;

    for (i = 0; i < mndims; i++) {
        size_t dimlength;
        if ((err = nc_inq_dimlen(grpid, mdimids[i], &dimlength)) != NC_NOERR) {
            RAISE_PLUGIN_ERROR("Unable to obtain Length of Dimension");
        }

        extents[i] = dimlength;        // Current Length

        int j;
        for (j = 0; j < nunlimdimids; j++) {
            if (mdimids[i] == unlimdimids[j]) {
                isUnlimited = 1;
                break;
            }
        }

        IDAM_LOGF(UDA_LOG_DEBUG, "Dimension %d has length %d\n", mdimids[i], dimlength);

	if (rank > 0) {
	  if (isUnlimited) {
            unlimitedCount++;
            extents[i] = (size_t) shape[i]; // Capture the current size of each UNLIMITED dimension
            IDAM_LOGF(UDA_LOG_DEBUG, "extents[%d] = %d \n", i, (int) extents[i]);
	  } else { 
	    //Check dimension length matches input data dimension length
	    if (dimlength != shape[i]) {
	      RAISE_PLUGIN_ERROR("Inconsistent Dimension Lengths for Array Variable");
	    }
	  }
	} else if (isUnlimited) {
	  unlimitedCount++;
	}
    }

    //--------------------------------------------------------------------------
    // Chunking is always ON (Variables with UNLIMITED dimension cannot use Contiguous chunking)

    int chunksize = -1;
    FIND_INT_VALUE(idam_plugin_interface->request_block->nameValueList, chunksize);

    if (length > 1 && rank > 0) {
        size_t* chunking = (size_t*)malloc(rank * sizeof(size_t));
        for (i = 0; i < rank; i++) {
            if (chunksize > 0) {
                chunking[i] = (size_t) chunksize;
            } else {
                chunking[i] = (size_t) shape[i];
            }
            IDAM_LOGF(UDA_LOG_DEBUG, "Using Chunking[%d] = %d\n", i, (int) chunking[i]);
        }

        if (isUnlimited) {
            if ((err = nc_def_var_chunking(grpid, varid, NC_CHUNKED, chunking)) != NC_NOERR) {
                RAISE_PLUGIN_ERROR("Unable to set the Chunking Properties of Variable");
            }
        } else if ((err = nc_def_var_chunking(grpid, varid, NC_CHUNKED, chunking)) != NC_NOERR) {
            RAISE_PLUGIN_ERROR("Unable to set the Chunking Properties of Variable");
        }

        free((void*) chunking);
    }

    //--------------------------------------------------------------------------
    // Compression

    int compression = -1;
    FIND_INT_VALUE(idam_plugin_interface->request_block->nameValueList, compression);

    if (length > 1 && rank > 0 && compression > 0) {
        IDAM_LOGF(UDA_LOG_DEBUG, "Using Compression level %d\n", compression);

        if ((err = nc_def_var_deflate(grpid, varid, NC_SHUFFLE, 1, compression)) != NC_NOERR) {
            RAISE_PLUGIN_ERROR("Unable to set the Compression Properties of Variable");
        }
    }

    //--------------------------------------------------------------------------
    // Write the Data Variable

    IDAM_LOGF(UDA_LOG_DEBUG, "Writing Data to the Data Variable %s \n", name);

    if (rank == 0 && length == 1) {
        // Scalar Data Item
        size_t start[1] = { 0 };
        size_t count[1] = { 1 };

        IDAM_LOGF(UDA_LOG_DEBUG, "Data Variable %s is a Scalar\n", name);

        switch (nctype) {
            case NC_FLOAT: {
                float scalar = ((float*)data)[0];
                if (unlimitedCount == 0) {
                    err = nc_put_var_float(grpid, varid, &scalar);
                } else {
                    err = nc_put_vara_float(grpid, varid, start, count, &scalar);
                }
                break;
            }
            case NC_DOUBLE: {
                double scalar = ((double*)data)[0];
                if (unlimitedCount == 0) {
                    err = nc_put_var_double(grpid, varid, &scalar);
                } else {
                    err = nc_put_vara_double(grpid, varid, start, count, &scalar);
                }
                break;
            }
            case NC_INT64: {
                long long scalar = ((long long*)data)[0];
                if (unlimitedCount == 0) {
                    err = nc_put_var_longlong(grpid, varid, &scalar);
                } else {
                    err = nc_put_vara_longlong(grpid, varid, start, count, &scalar);
                }
                break;
            }
            case NC_INT: {
                int scalar = ((int*)data)[0];
                if (unlimitedCount == 0) {
                    err = nc_put_var_int(grpid, varid, &scalar);
                } else {
                    err = nc_put_vara_int(grpid, varid, start, count, &scalar);
                }
                break;
            }
            case NC_SHORT: {
                short scalar = ((short*)data)[0];
                if (unlimitedCount == 0) {
                    err = nc_put_var_short(grpid, varid, &scalar);
                } else {
                    err = nc_put_vara_short(grpid, varid, start, count, &scalar);
                }
                break;
            }
            case NC_BYTE: {
                signed char scalar = ((signed char*)data)[0];
                if (unlimitedCount == 0) {
                    err = nc_put_var_schar(grpid, varid, &scalar);
                } else {
                    err = nc_put_vara_schar(grpid, varid, start, count, &scalar);
                }
                break;
            }
            case NC_UINT64: {
                unsigned long long scalar = ((unsigned long long*)data)[0];
                if (unlimitedCount == 0) {
                    err = nc_put_var_ulonglong(grpid, varid, &scalar);
                } else {
                    err = nc_put_vara_ulonglong(grpid, varid, start, count, &scalar);
                }
                break;
            }
            case NC_UINT: {
                unsigned int scalar = ((unsigned int*)data)[0];
                if (unlimitedCount == 0) {
                    err = nc_put_var_uint(grpid, varid, &scalar);
                } else {
                    err = nc_put_vara_uint(grpid, varid, start, count, &scalar);
                }
                break;
            }
            case NC_USHORT: {
                unsigned short scalar = ((unsigned short*)data)[0];
                if (unlimitedCount == 0) {
                    err = nc_put_var_ushort(grpid, varid, &scalar);
                } else {
                    err = nc_put_vara_ushort(grpid, varid, start, count, &scalar);
                }
                break;
            }
            default:
                if (nctype == ctype) {
                    COMPLEX scalar = ((COMPLEX*)data)[0];
                    if (unlimitedCount == 0) {
                        err = nc_put_var(grpid, varid, (void*) &scalar);
                    } else {
                        err = nc_put_vara(grpid, varid, start, count, &scalar);
                    }
                } else if (nctype == dctype) {
                    DCOMPLEX scalar = ((DCOMPLEX*)data)[0];
                    if (unlimitedCount == 0) {
                        err = nc_put_var(grpid, varid, (void*) &scalar);
                    } else {
                        err = nc_put_vara(grpid, varid, start, count, &scalar);
                    }
                } else {
                    RAISE_PLUGIN_ERROR("Unknown Variable type");
                }
        }

    } else {
        size_t* start = (size_t*) malloc(sizeof(size_t) * rank);
        for (i = 0; i < rank; i++) {
            start[i] = 0;
        }

        IDAM_LOGF(UDA_LOG_DEBUG, "Data Variable %s is an Array\n", name);
        IDAM_LOGF(UDA_LOG_DEBUG, "Length %d\n", length);
        IDAM_LOGF(UDA_LOG_DEBUG, "Rank: %d\n", rank);
        IDAM_LOG(UDA_LOG_DEBUG, "Shape:");
        for (i = 0; i < rank; i++) {
            IDAM_LOGF(UDA_LOG_DEBUG, "[%d]", shape[i]);
        }
        IDAM_LOG(UDA_LOG_DEBUG, "Starting Indices:");
        for (i = 0; i < rank; i++) {
            IDAM_LOGF(UDA_LOG_DEBUG, "[%d]", start[i]);
        }
        IDAM_LOG(UDA_LOG_DEBUG, "Extents::\n");
        for (i = 0; i < rank; i++) {
            IDAM_LOGF(UDA_LOG_DEBUG, "[%d]\n", extents[i]);
        }

        switch (nctype) {
            case NC_FLOAT:
                IDAM_LOGF(UDA_LOG_DEBUG, "Data Variable %s is a Float Array\n", name);
                err = nc_put_vara_float(grpid, varid, start, extents, (float*) data);
                break;
            case NC_DOUBLE:
                IDAM_LOGF(UDA_LOG_DEBUG, "Data Variable %s is a Double Array\n", name);
                err = nc_put_vara_double(grpid, varid, start, extents, (double*) data);
                break;
            case NC_INT64:
                IDAM_LOGF(UDA_LOG_DEBUG, "Data Variable %s is an long long Array\n", name);
                err = nc_put_vara_longlong(grpid, varid, start, extents, (long long*) data);
                break;
            case NC_INT:
                IDAM_LOGF(UDA_LOG_DEBUG, "Data Variable %s is an int Array\n", name);
                err = nc_put_vara_int(grpid, varid, start, extents, (int*) data);
                break;
            case NC_SHORT:
                IDAM_LOGF(UDA_LOG_DEBUG, "Data Variable %s is a short Array\n", name);
                err = nc_put_vara_short(grpid, varid, start, extents, (short*) data);
                break;
            case NC_BYTE:
                IDAM_LOGF(UDA_LOG_DEBUG, "Data Variable %s is a byte Array\n", name);
                err = nc_put_vara_schar(grpid, varid, start, extents, (signed char*) data);
                break;
            case NC_UINT64:
                IDAM_LOGF(UDA_LOG_DEBUG, "Data Variable %s is an unsigned long long Array\n", name);
                err = nc_put_vara_ulonglong(grpid, varid, start, extents,
                                            (unsigned long long*) data);
                break;
            case NC_UINT:
                IDAM_LOGF(UDA_LOG_DEBUG, "Data Variable %s is an unsigned int Array\n", name);
                err = nc_put_vara_uint(grpid, varid, start, extents, (unsigned int*) data);
                break;
            case NC_USHORT:
                IDAM_LOGF(UDA_LOG_DEBUG, "Data Variable %s is an unsigned short Array\n", name);
                err = nc_put_vara_ushort(grpid, varid, start, extents,
                                         (unsigned short*) data);
                break;
            default:
                if (nctype == ctype || nctype == dctype) {
		  IDAM_LOGF(UDA_LOG_DEBUG, "Data Variable %s is a complex or double complex Array\n", name);
                    err = nc_put_vara(grpid, varid, start, extents, (void*) data);
                }
        }

        free((void*) start);
    }

    if (err != NC_NOERR) {
        RAISE_PLUGIN_ERROR("Unable to Write Data to Data Variable");
    } else {
        IDAM_LOGF(UDA_LOG_DEBUG, "Variable \"%s\" was written to file\n", name);
    }

    //--------------------------------------------------------------------------
    // Write extents attribute if any dimension was UNLIMITED

    if (unlimitedCount > 0) {
        IDAM_LOGF(UDA_LOG_DEBUG, "Writing Extent Attribute of variable %s\n", name);
        unsigned int iextents[NC_MAX_DIMS];
        for (i = 0; i < mndims; i++) {
            iextents[i] = (unsigned int)extents[i];
            IDAM_LOGF(UDA_LOG_DEBUG, "[%d]=%d\n", i, extents[i]);
        }
        if ((err = nc_put_att_uint(grpid, varid, "extent", NC_UINT64, mndims, iextents)) != NC_NOERR) {
            RAISE_PLUGIN_ERROR("Unable to Write the Extent Attribute of variable");
        }
    }

    //--------------------------------------------------------------------------
    // Write SCALE and OFFSET attributes

    float scale = -1;
    FIND_FLOAT_VALUE(idam_plugin_interface->request_block->nameValueList, scale);

    if (scale > 0) {
        if ((err = nc_put_att_float(grpid, varid, "scale", NC_FLOAT, 1, &scale)) != NC_NOERR) {
            RAISE_PLUGIN_ERROR("Unable to Write the Scale Attribute");
        }
    }

    float offset = -1;
    FIND_FLOAT_VALUE(idam_plugin_interface->request_block->nameValueList, offset);

    if (offset > 0) {
        if ((err = nc_put_att_float(grpid, varid, "offset", NC_FLOAT, 1, &offset)) != NC_NOERR) {
            RAISE_PLUGIN_ERROR("Unable to Write the Offset Attribute");
        }
    }

    //--------------------------------------------------------------------------
    // Write Standard Variable Attributes

    const char* label = NULL;
    FIND_STRING_VALUE(idam_plugin_interface->request_block->nameValueList, label);

    if (label != NULL) {
        if ((err = nc_put_att_text(grpid, varid, "label", strlen(label), label)) != NC_NOERR) {
            RAISE_PLUGIN_ERROR("Unable to Write the Label Attribute");
        }
    }

    const char* title = NULL;
    FIND_STRING_VALUE(idam_plugin_interface->request_block->nameValueList, title);

    if (title != NULL) {
        if ((err = nc_put_att_text(grpid, varid, "title", strlen(title), title)) != NC_NOERR) {
            RAISE_PLUGIN_ERROR("Unable to Write the Title Attribute");
        }
    }

    const char* comment = NULL;
    FIND_STRING_VALUE(idam_plugin_interface->request_block->nameValueList, comment);

    if (comment != NULL) {
        if ((err = nc_put_att_text(grpid, varid, "comment", strlen(comment), comment)) != NC_NOERR) {
            RAISE_PLUGIN_ERROR("Unable to Write the Comment Attribute");
        }
    }

    const char* units = NULL;
    FIND_STRING_VALUE(idam_plugin_interface->request_block->nameValueList, units);

    int notstrict = findValue(&idam_plugin_interface->request_block->nameValueList, "notstrict");

    if (units != NULL) {
        if (!notstrict) {
            if (!testUnitsCompliance(units)) {
                // Test for SI Compliance
                RAISE_PLUGIN_ERROR("Unable to Write the Units Attribute");
            }
        }
        if ((err = nc_put_att_text(grpid, varid, "units", strlen(units), units)) != NC_NOERR) {
            RAISE_PLUGIN_ERROR("Unable to Write the Units Attribute");
        }
    }

    //--------------------------------------------------------------------------
    // Write Device Association attributes: DEVICE & CHANNEL

    const char* device = NULL;
    FIND_STRING_VALUE(idam_plugin_interface->request_block->nameValueList, device);

    if (device != NULL) {
        int devgrp = 0;
        int devgrp0 = 0;

        if ((err = nc_inq_grp_ncid(ncfileid, "devices", &devgrp0)) != NC_NOERR) {
            // Check the Devices group exists
            RAISE_PLUGIN_ERROR("The /devices group is not defined");
        }

        if ((err = nc_inq_grp_ncid(devgrp0, device, &devgrp)) != NC_NOERR) {
            // Check the Device exists
            RAISE_PLUGIN_ERROR("The Device not defined");
        }

        if ((err = nc_put_att_text(grpid, varid, "device", strlen(device), device)) != NC_NOERR) {
            RAISE_PLUGIN_ERROR("Unable to Write the Device Attribute");
        }
    }

    int channels = -1;
    FIND_INT_VALUE(idam_plugin_interface->request_block->nameValueList, channels);

    if (channels > 0) {
        // Ambiguous keyword if channel & channnels are both used
        if (device != NULL) {
            short channel = (short) channels;
            if ((err = nc_put_att_short(grpid, varid, "channel", NC_SHORT, 1, &channel)) != NC_NOERR) {
                RAISE_PLUGIN_ERROR("Unable to Write the Channel Attribute");
            }
        } else {
            RAISE_PLUGIN_ERROR("The Channel Number needs the Device to be Identified");
        }
    }

    //--------------------------------------------------------------------------
    // If there is a named error variable then check it exists in this group and it has the same shape.

    const char* errors = NULL;
    FIND_STRING_VALUE(idam_plugin_interface->request_block->nameValueList, errors);

    if (errors != NULL) {
        int ncvarerrid;
        if ((err = nc_inq_varid(grpid, errors, &ncvarerrid)) != NC_NOERR) {
            RAISE_PLUGIN_ERROR("The Error variable does not exist");
        }

        int ndims;
        if ((err = nc_inq_varndims(grpid, ncvarerrid, &ndims)) != NC_NOERR) {
            RAISE_PLUGIN_ERROR("Unable to Query the number of Dimensions");
        }

        if ((rank > 0 && ndims != rank) || (rank == 0 && ndims != 1)) {
            RAISE_PLUGIN_ERROR("The Rank of the Error Variable is Inconsistent with the Variable");
        }

        int* dimids = (int*) malloc(ndims * sizeof(int));

        if ((err = nc_inq_vardimid(grpid, ncvarerrid, dimids)) != NC_NOERR) {
            RAISE_PLUGIN_ERROR("Unable to Query the Dimension ID of the Error Variable");
        }

	if (rank > 0) {
	  for (i = 0; i < rank; i++) {
            int lvarerr;
            if ((err = nc_inq_dimlen(grpid, dimids[i], (size_t*) &lvarerr)) != NC_NOERR) {
                RAISE_PLUGIN_ERROR("Unable to Query the Dimension Length of the Error Variable");
            }

            if (shape[i] != lvarerr) {
                RAISE_PLUGIN_ERROR("The Length of the Error Variable is Inconsistent with the variable Length");
            }
	  }
	} else {
	  int lvarerr;
	  if ((err = nc_inq_dimlen(grpid, dimids[0], (size_t*) &lvarerr)) != NC_NOERR) {
	    RAISE_PLUGIN_ERROR("Unable to Query the Dimension Length of the Error Variable");
	  }

	  if (lvarerr != 1) {
	    RAISE_PLUGIN_ERROR("The Length of the Error Variable is Inconsistent with the variable Length");
	  }
	}

        free((void*) dimids);

        if ((err = nc_put_att_text(grpid, varid, "errors", strlen(errors), errors)) != NC_NOERR) {
            RAISE_PLUGIN_ERROR("Unable to Write the Errors Attribute of Variable");
        }

    }

    //--------------------------------------------------------------------------
    // Cleanup Keywords

    if (work != NULL)free((void*) work);

    return err;
}


int testDimension(int grpid, char* dimension, int parents, int* ncdimid)
{
    char dimname[NC_MAX_NAME + 1];

    int err = 0;
    int ndims;
    int dimids[NC_MAX_DIMS];

    if ((err = nc_inq_dimids(grpid, &ndims, dimids, parents)) != NC_NOERR) {        // List all Dimensions
        fprintf(stderr, "Unable to List in-Scope Dimensions \n");
        return err;
    }

    IDAM_LOGF(UDA_LOG_DEBUG, "Number of Dimensions Visible from Group %d\n", ndims);

    if (ndims == 0) {
        *ncdimid = -1;
        return err;
    }

    // Search for the named dimension

    *ncdimid = -1;

    int i;
    for (i = 0; i < ndims; i++) {
        if ((err = nc_inq_dimname(grpid, dimids[i], dimname)) != NC_NOERR) {
            IDAM_LOGF(UDA_LOG_DEBUG, "Unable to Name an existing Dimension %d\n", dimids[i]);
            return err;
        }

        IDAM_LOGF(UDA_LOG_DEBUG, "Comparing Dimension Name [%s] with Target [%s]\n", dimname, dimension);

        if (STR_EQUALS(dimension, dimname)) {
            // Dimension Found
            IDAM_LOGF(UDA_LOG_DEBUG, "Dimension [%s] Found: ID %d\n", dimension, dimids[i]);
            *ncdimid = dimids[i];
            return err;
        }

    }

    return err;
}

nc_type swapType(int type, int ctype, int dctype)
{
    // Translate IDAM to netCDF4 Type
    switch (type) {
        case UDA_TYPE_FLOAT:            // 4 byte float
            return NC_FLOAT;

        case UDA_TYPE_DOUBLE:           // 8 byte float
            return NC_DOUBLE;

        case UDA_TYPE_LONG64:           // 8 byte signed integer
            return NC_INT64;

        case UDA_TYPE_LONG:             // 8 byte signed integer
            return NC_INT64;

        case UDA_TYPE_INT:              // 4 byte signed integer
            return NC_INT;

        case UDA_TYPE_SHORT:            // 2 byte signed integer
            return NC_SHORT;

        case UDA_TYPE_UNSIGNED_LONG64:  // 8 byte unsigned integer
            return NC_UINT64;

        case UDA_TYPE_UNSIGNED_LONG:    // 8 byte unsigned integer
            return NC_UINT64;

        case UDA_TYPE_UNSIGNED_INT:     // 4 byte unsigned integer
            return NC_UINT;

        case UDA_TYPE_UNSIGNED_SHORT:     // 2 byte unsigned integer
            return NC_USHORT;

        case UDA_TYPE_COMPLEX:          // structure with 2 4 byte floats
            return ctype;

        case UDA_TYPE_DCOMPLEX:         // structure with 2 8 byte floats
            return dctype;

        case UDA_TYPE_CHAR:             // 1 byte signed char (options NC_BYTE, NC_UBYTE, NC_CHAR ?)
            return NC_BYTE;

        case UDA_TYPE_UNSIGNED_CHAR:             // 1 byte unsigned char (options NC_BYTE, NC_UBYTE, NC_CHAR ?)
            return NC_UBYTE;

        default:
            return NC_NAT;

    }
}
