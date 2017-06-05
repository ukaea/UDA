#include "putAttribute.h"

#include <netcdf.h>

#include <clientserver/stringUtils.h>
#include <clientserver/udaTypes.h>

#include "putUnits.h"
#include "putGroup.h"
#include "putOpenClose.h"

int do_attribute(IDAM_PLUGIN_INTERFACE* idam_plugin_interface)
{
    //---------------------------------------------------------------------------
    // Extract Keywords and Parameters

    int fileid = -1;
    const char* group = NULL;
    const char* name = NULL;
    const char* varname = NULL;

    FIND_REQUIRED_INT_VALUE(idam_plugin_interface->request_block->nameValueList, fileid);
    FIND_REQUIRED_STRING_VALUE(idam_plugin_interface->request_block->nameValueList, group);
    FIND_REQUIRED_STRING_VALUE(idam_plugin_interface->request_block->nameValueList, name);
    FIND_STRING_VALUE(idam_plugin_interface->request_block->nameValueList, varname);

    int ncfileid = get_file_id(fileid);
    if (ncfileid < 0) {
        RAISE_PLUGIN_ERROR("Invalid fileid given");
    }

    int grpid = -1;
    int status = 0;
    if (testgroup(ncfileid, group, &status, &grpid) != NC_NOERR) {
        RAISE_PLUGIN_ERROR("Failed to find or create group");
    }

    //--------------------------------------------------------------------------
    // Shape and Type

    PUTDATA_BLOCK putdata = idam_plugin_interface->request_block->putDataBlockList.putDataBlock[0];

    if (putdata.data_type == TYPE_OPAQUE) {
        RAISE_PLUGIN_ERROR("Structured Attribute Types are Not Supported");
    }

    if (putdata.data_type == TYPE_STRING && putdata.rank != 0) {
        RAISE_PLUGIN_ERROR("Arrays of Strings Attributes are Not Supported");
    }

    //--------------------------------------------------------------------------
    // Attributes - can be placed within any group level

    // Associated Variable or Group Name
    int varid;

    if (varname != NULL) {
        if (nc_inq_varid(grpid, varname, &varid) != NC_NOERR) {
            RAISE_PLUGIN_ERROR("Unable to Identify the Variable/Coordinate");
        }
    } else {
        varid = NC_GLOBAL;
    }

    int ctype = 0;
    int dctype = 0;
    if (get_complex_types(ncfileid, &ctype, &dctype)) {
        RAISE_PLUGIN_ERROR("Unable to retrieve complex data types from file");
    }

    // Scalar Attributes

    if (putdata.data_type != TYPE_STRING) {
        if (putdata.rank == 0 && putdata.count == 1) {
            IDAM_LOG(UDA_LOG_DEBUG, "Scalar Attribute to be added");

            int err;
            switch (putdata.data_type) {
                case TYPE_FLOAT: {
                    float value = ((float*)putdata.data)[0];
                    err = nc_put_att_float(grpid, varid, name, NC_FLOAT, 1, &value);
                    break;
                }
                case TYPE_DOUBLE: {
                    double value = ((double*)putdata.data)[0];
                    err = nc_put_att_double(grpid, varid, name, NC_DOUBLE, 1, &value);
                    break;
                }
                case TYPE_LONG64: {
                    long long value = ((long long*)putdata.data)[0];
                    err = nc_put_att_longlong(grpid, varid, name, NC_INT64, 1, &value);
                    break;
                }
                case TYPE_LONG: {
                    int value = ((int*)putdata.data)[0];
                    err = nc_put_att_int(grpid, varid, name, NC_INT, 1, &value);
                    break;
                }
                case TYPE_INT: {
                    short value = ((short*)putdata.data)[0];
                    err = nc_put_att_short(grpid, varid, name, NC_SHORT, 1, &value);
                    break;
                }
                case TYPE_UNSIGNED_CHAR: {
                    unsigned char value = ((unsigned char*)putdata.data)[0];
                    err = nc_put_att_ubyte(grpid, varid, name, NC_UBYTE, 1, &value);
                    break;
                }
                case TYPE_UNSIGNED_LONG64: {
                    unsigned long long value = ((unsigned long long*)putdata.data)[0];
                    err = nc_put_att_ulonglong(grpid, varid, name, NC_UINT64, 1, &value);
                    break;
                }
                case TYPE_UNSIGNED_LONG: {
                    unsigned int value = ((unsigned int*)putdata.data)[0];
                    err = nc_put_att_uint(grpid, varid, name, NC_UINT, 1, &value);
                    break;
                }
                case TYPE_UNSIGNED_INT: {
                    unsigned short value = ((unsigned short*)putdata.data)[0];
                    err = nc_put_att_ushort(grpid, varid, name, NC_USHORT, 1, &value);
                    break;
                }

                case TYPE_COMPLEX: {
                    COMPLEX value = ((COMPLEX*)putdata.data)[0];
                    err = nc_put_att(grpid, varid, name, ctype, 1, (void*)&value);
                    break;
                }

                case TYPE_DCOMPLEX: {
                    DCOMPLEX value = ((DCOMPLEX*)putdata.data)[0];
                    err = nc_put_att(grpid, varid, name, dctype, 1, (void*)&value);
                    break;
                }

                default:
                    RAISE_PLUGIN_ERROR("Uknown Attribute Type");
            }

            if (err != NC_NOERR) {
                RAISE_PLUGIN_ERROR(nc_strerror(err));
            }
        } else if (putdata.rank > 0 && putdata.count >= 1 && putdata.data != NULL) {

            IDAM_LOG(UDA_LOG_DEBUG, "Array to be added");

            if (putdata.rank > 1) {
                RAISE_PLUGIN_ERROR("Array Attributes Must be Rank 1");
            }

            int err;
            switch (putdata.data_type) {
                case TYPE_FLOAT:
                    err = nc_put_att_float(grpid, varid, name, NC_FLOAT, putdata.count, (float*)putdata.data);
                    break;
                case TYPE_DOUBLE:
                    err = nc_put_att_double(grpid, varid, name, NC_DOUBLE, putdata.count, (double*)putdata.data);
                    break;
                case TYPE_LONG64:
                    err = nc_put_att_longlong(grpid, varid, name, NC_INT64, putdata.count, (long long*)putdata.data);
                    break;
                case TYPE_LONG:
                    err = nc_put_att_int(grpid, varid, name, NC_INT, putdata.count, (int*)putdata.data);
                    break;
                case TYPE_INT:
                    err = nc_put_att_short(grpid, varid, name, NC_SHORT, putdata.count, (short*)putdata.data);
                    break;
                case TYPE_UNSIGNED_CHAR:
                    err = nc_put_att_ubyte(grpid, varid, name, NC_UBYTE, putdata.count, (unsigned char*)putdata.data);
                    break;
                case TYPE_UNSIGNED_LONG64:
                    err = nc_put_att_ulonglong(grpid, varid, name, NC_UINT64, putdata.count, (unsigned long long*)putdata.data);
                    break;
                case TYPE_UNSIGNED_LONG:
                    err = nc_put_att_uint(grpid, varid, name, NC_UINT, putdata.count, (unsigned int*)putdata.data);
                    break;
                case TYPE_UNSIGNED_INT:
                    err = nc_put_att_ushort(grpid, varid, name, NC_USHORT, putdata.count, (unsigned short*)putdata.data);
                    break;
                case TYPE_COMPLEX:
                    err = nc_put_att(grpid, varid, name, ctype, putdata.count, (void*)putdata.data);
                    break;
                case TYPE_DCOMPLEX:
                    err = nc_put_att(grpid, varid, name, dctype, putdata.count, (void*)putdata.data);
                    break;
                default:
                    RAISE_PLUGIN_ERROR("Uknown Attribute Type");

            }

            if (err != NC_NOERR) {
                RAISE_PLUGIN_ERROR(nc_strerror(err));
            }
        }

    } else {
        // String

        const char* text = putdata.data;

        if (text != NULL) {
            IDAM_LOGF(UDA_LOG_DEBUG, "Text Attribute to be added: %s", text);

            int notstrict = findValue(&idam_plugin_interface->request_block->nameValueList, "nostrict");

            if (!notstrict && varname != NULL && STR_EQUALS(name, "units")) {
                // Test for SI Compliance
                if (!testUnitsCompliance(text)) {
                    RAISE_PLUGIN_ERROR("Unable to Write a Units Attribute for Variable/Coordinate");
                }
            }

            if (nc_put_att_text(grpid, varid, name, strlen(text), text) != NC_NOERR) {
                RAISE_PLUGIN_ERROR("Unable to Write a Text Attribute");
            }
        }
    }

    return 0;
}

