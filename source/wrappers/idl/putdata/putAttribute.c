#include "putdata.h"		// IDL DLM API Header

#include <clientserver/udaTypes.h>

int putAttribute(int nparams, IDL_VPTR argv[], KW_RESULT* kw, int ncgrpid) {
//
//-------------------------------------------------------------------------
// Change History:
//
// 30Jan2009 DGMuir	Original Version
//-------------------------------------------------------------------------	      

    int length, type, rank, err, lerr;
    int ncvarid;
    //    char* group;
    char* name;
    char* varname;
    char* text;
    void* data = NULL;

//---------------------------------------------------------------------------      
// Extract Keywords and Parameters

    type = 0;

    //    group = NULL;
    name = NULL;
    varname = NULL;
    text = NULL;

    //    if (kw->is_group) group = IDL_STRING_STR(&kw->group);
    if (kw->is_name) name = IDL_STRING_STR(&kw->name);
    if (kw->is_varname) varname = IDL_STRING_STR(&kw->varname);

//--------------------------------------------------------------------------      
// Create an Error Trap

    lerr = 0;        // Local Error
    err = 0;        // NC Error

    do {

//--------------------------------------------------------------------------      
// Shape and Type

        type = argv[0]->type;

        if (type == IDL_TYP_STRUCT) {
            if (kw->verbose) fprintf(stderr, "Structured Attribute Types are Not Supported\n");
            lerr = -1;
            break;
        }

        if (argv[0]->flags & IDL_V_ARR && type == IDL_TYP_STRING) {
            if (kw->verbose) fprintf(stderr, "Arrays of Strings Attributes are Not Supported\n");
            lerr = -1;
            break;
        }

        if (argv[0]->flags & IDL_V_ARR) {
            IDL_ENSURE_ARRAY(argv[0]);
            rank = (int) argv[0]->value.arr->n_dim;            // Number of Dimensions
            length = (int) argv[0]->value.arr->n_elts;            // Number of Elements
            data = (void*) argv[0]->value.arr->data;
        } else {
            IDL_ENSURE_SCALAR(argv[0]);                    // Single Scalar value
            rank = 0;
            length = 1;
        }


//--------------------------------------------------------------------------      
// Attributes - can be placed within any group level

// Associated Variable or Group Name

        if (kw->is_varname) {
            if ((err = nc_inq_varid(ncgrpid, varname, &ncvarid)) != NC_NOERR) {
                if (kw->verbose) fprintf(stderr, "Unable to Identify the Variable/Coordinate: %s\n", varname);
                break;
            }
        } else {
            ncvarid = NC_GLOBAL;
        }

// Scalar Attributes

        if (type != IDL_TYP_STRING) {

            if (rank == 0 && length == 1) {

                if (kw->debug) fprintf(stdout, "Scalar Attribute to be added ... \n");

                switch (type) {
                    case IDL_TYP_FLOAT: {
                        float value = (float) IDL_DoubleScalar(argv[0]);
                        err = nc_put_att_float(ncgrpid, ncvarid, name, NC_FLOAT, 1, &value);
                        break;
                    }
                    case IDL_TYP_DOUBLE: {
                        double value = (double) IDL_DoubleScalar(argv[0]);
                        err = nc_put_att_double(ncgrpid, ncvarid, name, NC_DOUBLE, 1, &value);
                        break;
                    }
                    case IDL_TYP_LONG64: {
                        long long value = (long long) IDL_Long64Scalar(argv[0]);
                        err = nc_put_att_longlong(ncgrpid, ncvarid, name, NC_INT64, 1, &value);
                        break;
                    }
                    case IDL_TYP_LONG: {
                        int value = (int) IDL_LongScalar(argv[0]);
                        err = nc_put_att_int(ncgrpid, ncvarid, name, NC_INT, 1, &value);
                        break;
                    }
                    case IDL_TYP_INT: {
                        short value = (short) IDL_LongScalar(argv[0]);
                        err = nc_put_att_short(ncgrpid, ncvarid, name, NC_SHORT, 1, &value);
                        break;
                    }
                    case IDL_TYP_BYTE: {
                        unsigned char value = (unsigned char) IDL_ULongScalar(argv[0]);
                        err = nc_put_att_ubyte(ncgrpid, ncvarid, name, NC_UBYTE, 1, &value);
                        break;
                    }
                    case IDL_TYP_ULONG64: {
                        unsigned long long value = (unsigned long long) IDL_ULong64Scalar(argv[0]);
                        err = nc_put_att_ulonglong(ncgrpid, ncvarid, name, NC_UINT64, 1, &value);
                        break;
                    }
                    case IDL_TYP_ULONG: {
                        unsigned int value = (unsigned int) IDL_ULongScalar(argv[0]);
                        err = nc_put_att_uint(ncgrpid, ncvarid, name, NC_UINT, 1, &value);
                        break;
                    }
                    case IDL_TYP_UINT: {
                        unsigned short value = (unsigned short) IDL_ULongScalar(argv[0]);
                        err = nc_put_att_ushort(ncgrpid, ncvarid, name, NC_USHORT, 1, &value);
                        break;
                    }

                    case IDL_TYP_COMPLEX: {
                        IDL_COMPLEX data = argv[0]->value.cmp;    // See definition of IDL_VPTR & IDL_ALLTYPES
                        COMPLEX value;
                        value.real = data.r;
                        value.imaginary = data.i;
                        if (kw->debug) {
                            fprintf(stdout, "Complex Scalar: Real      = %f\n", value.real);
                            fprintf(stdout, "Complex Scalar: imaginary = %f\n", value.imaginary);
                        }
                        err = nc_put_att(ncgrpid, ncvarid, name, ctype, 1, (void*) &value);
                        break;
                    }

                    case IDL_TYP_DCOMPLEX: {
                        IDL_DCOMPLEX data = argv[0]->value.dcmp;
                        DCOMPLEX value;
                        value.real = data.r;
                        value.imaginary = data.i;
                        if (kw->debug) {
                            fprintf(stdout, "Complex Scalar: Real      = %f\n", (float) value.real);
                            fprintf(stdout, "Complex Scalar: imaginary = %f\n", (float) value.imaginary);
                        }
                        err = nc_put_att(ncgrpid, ncvarid, name, dctype, 1, (void*) &value);
                        break;
                    }

                    default:
                        if (kw->verbose) fprintf(stderr, "Uknown Attribute Type: %d\n", type);
                        lerr = -1;
                        break;

                }

                if (err != NC_NOERR || lerr != 0) {
                    if (kw->verbose) fprintf(stderr, "Unable to Write the Scalar Attribute: %s\n", name);
                    break;
                }

                break;

            }

// Array

            if (rank > 0 && length >= 1 && data != NULL) {

                if (kw->debug) fprintf(stdout, "Array to be added ... \n");

                if (rank > 1) {
                    if (kw->verbose) {
                        fprintf(stderr, "Array Attributes Must be Rank 1 not %d\n", rank);
                        fprintf(stderr, "Unable to Write the Array Attribute: %s\n", name);
                    }
                    lerr = -1;
                    break;
                }

                switch (type) {
                    case IDL_TYP_FLOAT:
                        err = nc_put_att_float(ncgrpid, ncvarid, name, NC_FLOAT, length, (float*) data);
                        break;
                    case IDL_TYP_DOUBLE:
                        err = nc_put_att_double(ncgrpid, ncvarid, name, NC_DOUBLE, length, (double*) data);
                        break;
                    case IDL_TYP_LONG64:
                        err = nc_put_att_longlong(ncgrpid, ncvarid, name, NC_INT64, length, (long long*) data);
                        break;
                    case IDL_TYP_LONG:
                        err = nc_put_att_int(ncgrpid, ncvarid, name, NC_INT, length, (int*) data);
                        break;
                    case IDL_TYP_INT:
                        err = nc_put_att_short(ncgrpid, ncvarid, name, NC_SHORT, length, (short*) data);
                        break;
                    case IDL_TYP_BYTE:
                        err = nc_put_att_ubyte(ncgrpid, ncvarid, name, NC_UBYTE, length, (unsigned char*) data);
                        break;
                    case IDL_TYP_ULONG64:
                        err = nc_put_att_ulonglong(ncgrpid, ncvarid, name, NC_UINT64, length,
                                                   (unsigned long long*) data);
                        break;
                    case IDL_TYP_ULONG:
                        err = nc_put_att_uint(ncgrpid, ncvarid, name, NC_UINT, length, (unsigned int*) data);
                        break;
                    case IDL_TYP_UINT:
                        err = nc_put_att_ushort(ncgrpid, ncvarid, name, NC_USHORT, length, (unsigned short*) data);
                        break;
                    case IDL_TYP_COMPLEX:
                        err = nc_put_att(ncgrpid, ncvarid, name, ctype, length, (void*) data);
                        break;
                    case IDL_TYP_DCOMPLEX:
                        err = nc_put_att(ncgrpid, ncvarid, name, dctype, length, (void*) data);
                        break;
                    default:
                        if (kw->verbose) fprintf(stderr, "Uknown Attribute Type: %d\n", type);
                        lerr = -1;
                        break;

                }

                if (err != NC_NOERR) {
                    if (kw->verbose) fprintf(stderr, "Unable to Write the Array Attribute: %s\n", name);
                    break;
                }

                break;


            }

        } else {

// String

            IDL_ENSURE_STRING(argv[0]);

            if ((text = IDL_STRING_STR(&(argv[0]->value.str))) != NULL) {

                if (kw->debug) fprintf(stdout, "Text Attribute to be added ... %s\n", text);


                if (!kw->notstrict && kw->is_varname && STR_EQUALS(name, "units")) {    // Test for SI Compliance
                    if (!testUnitsCompliance(kw, text)) {
                        if (kw->verbose)
                            fprintf(stderr, "Unable to Write a Units Attribute for Variable/Coordinate %s\n", varname);
                        lerr = -1;
                        break;
                    }
                }

                if ((err = nc_put_att_text(ncgrpid, ncvarid, name, strlen(text), text)) != NC_NOERR) {
                    if (kw->verbose) fprintf(stderr, "Unable to Write a Text Attribute: %s\n", name);
                    break;
                }

                break;
            }

            break;
        }

//--------------------------------------------------------------------------      
// End of Error Trap 

    } while (0);

    if (err != NC_NOERR) {
        if (kw->verbose) fprintf(stderr, "Error Report: %s\n", nc_strerror(err));
    } else {
        err = lerr;
    }


    return err;
}

