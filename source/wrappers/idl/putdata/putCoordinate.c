
#include "putdata.h"		// IDL DLM API Header

#include <locale.h>
#include <stdlib.h>

int putDimension(int nparams, IDL_VPTR argv[], KW_RESULT *kw, int ncgrpid) {

    int err, lerr, length;
    int ncdimid;
    char *group, *name;

//--------------------------------------------------------------------------      
// Error Trap  

    err = NC_NOERR;
    lerr = NC_NOERR;

    do {

//---------------------------------------------------------------------------      
// Length Parameter

        if (nparams == 1) {
            IDL_ENSURE_SCALAR(argv[0]);
            length = IDL_LongScalar(argv[0]);
        } else {
            length = 0;
        }

//--------------------------------------------------------------------------      
// Write Dimension Entry  

        if (!kw->is_group) {
            if (kw->verbose) fprintf(stderr, "A Dimension needs a Group!\n");
            lerr = -2;
            break;
        }

        if (!kw->is_name) {
            if (kw->verbose) fprintf(stderr, "A Dimension needs a Name!\n");
            lerr = -2;
            break;
        }

        group = IDL_STRING_STR(&kw->group);
        name = IDL_STRING_STR(&kw->name);

        if (kw->debug) {
            fprintf(stdout, "Writing Dimension %s = %d\n", name, length);
            fprintf(stdout, "to Group %s \n", group);
        }

        if (kw->unlimited || length == 0) {
            if (kw->debug)fprintf(stdout, "Dimension is UNLIMITED\n");
            if ((err = nc_def_dim(ncgrpid, name, NC_UNLIMITED, &ncdimid)) != NC_NOERR) {
                if (kw->verbose) fprintf(stderr, "Unable to Write the Group %s Dimension %s\n", group, name);
                break;
            }
        } else {
            if ((err = nc_def_dim(ncgrpid, name, (size_t) length, &ncdimid)) != NC_NOERR) {
                if (kw->verbose) fprintf(stderr, "Unable to Write the Group %s Dimension %s\n", group, name);
                break;
            }
        }

        if (kw->debug)fprintf(stdout, "Dimension #id = %d\n", (int) ncdimid);

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

// List all dimensions visible from the named group

int testDimension(int ncgrpid, KW_RESULT *kw, char *dimension, int parents, int *ncdimid) {

    int i, err = NC_NOERR, ndims, dimids[NC_MAX_DIMS];
    char dimname[NC_MAX_NAME + 1];

    if ((err = nc_inq_dimids(ncgrpid, &ndims, dimids, parents)) != NC_NOERR) {        // List all Dimensions
        fprintf(stderr, "Unable to List in-Scope Dimensions \n");
        return err;
    }

    if (kw->debug)fprintf(stdout, "Number of Dimensions Visible from Group %d\n", ndims);

    if (ndims == 0) {
        *ncdimid = -1;
        return err;
    }

// Search for the named dimension

    *ncdimid = -1;

    for (i = 0; i < ndims; i++) {

        if ((err = nc_inq_dimname(ncgrpid, dimids[i], dimname)) != NC_NOERR) {
            fprintf(stderr, "Unable to Name an existing Dimension %d\n", dimids[i]);
            return err;
        }

        if (kw->debug)fprintf(stdout, "Comparing Dimension Name [%s] with Target [%s]\n", dimname, dimension);

        if (STR_EQUALS(dimension, dimname)) {        // Dimension Found
            if (kw->debug)fprintf(stdout, "Dimension [%s] Found: ID %d\n", dimension, dimids[i]);

            *ncdimid = dimids[i];
            return err;
        }

    }
    return err;
}


int putCoordinate(int nparams, IDL_VPTR argv[], KW_RESULT *kw, int ncgrpid) {

    int err, lerr, ncdimid, rank = 1;
    int ncvarid;
    nc_type xtype;
    unsigned int coordcount, attlength0, attlength1, attlength2, totlength;

    char *group, *name, *units, *label, *class, *title, *comment;

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

    if (kw->is_group) group = IDL_STRING_STR(&kw->group);
    if (kw->is_name) name = IDL_STRING_STR(&kw->name);

//--------------------------------------------------------------------------      
// Create an Error Trap

    lerr = NC_NOERR;        // Local Error
    err = NC_NOERR;        // NC Error

    do {

//--------------------------------------------------------------------------           
// Coordinate Data ? 

        if (nparams == 0) {
            lerr = -1;
            if (kw->verbose) fprintf(stderr, "No Coordinate Data for %s have been passed - please correct.\n", name);
            break;
        }

//--------------------------------------------------------------------------           
// Check the name 

        if (!kw->is_name) {
            if (kw->verbose) fprintf(stderr, "The Coordinate variable needs a Name - please correct!\n");
            lerr = -1;
            break;
        }
        if (!kw->is_group) {
            if (kw->verbose) fprintf(stderr, "The Coordinate variable needs a Group - please correct!\n");
            lerr = -1;
            break;
        }

//--------------------------------------------------------------------------           
// Check the named Coordinate does not already exist in this group

        if ((err = nc_inq_varid(ncgrpid, name, &ncvarid)) != NC_NOERR && err != NC_ENOTVAR) {
            if (kw->verbose) fprintf(stderr, "Unable to Check if the Coordinate variable already %s exists.\n", name);
            break;
        }

        if (err == NC_NOERR) {        // The variable exists: Edit Mode?
            lerr = -1;
            if (kw->verbose) {
                fprintf(stderr, "The Coordinate Variable %s already exists in the Group: %s\n", name, group);
                //fprintf(stderr,"Edit mode not yet implemented!\n");
            }
            break;
        }

//--------------------------------------------------------------------------           
// Get the ID of the same named dimension

        if ((err = nc_inq_dimid(ncgrpid, name, &ncdimid)) != NC_NOERR) {
            lerr = -1;
            if (kw->verbose) {
                fprintf(stderr, "Unable to Identify Dimensions %s within Group [%d] %s\n", name, ncgrpid, group);
                fprintf(stderr, "Dimensions must be defined within the same group as the Coordinates\n");
            }
            break;
        }

        if (kw->debug)fprintf(stdout, "Coordinate Dimension %s has ID %d\n", name, ncdimid);

//--------------------------------------------------------------------------                  
// Translate IDL to netCDF4 Type (Type is always that of the first parameter) 

        if ((xtype = swapType(argv[0]->type)) == NC_NAT) {
            if (kw->verbose)
                fprintf(stderr, "The IDL Data type for the Coordinate %s cannot be converted into netCDF4\n", name);
            lerr = -1;
            break;
        }

//--------------------------------------------------------------------------                  
// Create the Coordinate Variable (always rank 1)

        if (kw->debug)fprintf(stdout, "Creating the Coordinate Variable %s [%d]\n", name, (int) xtype);

        if ((err = nc_def_var(ncgrpid, name, xtype, rank, &ncdimid, &ncvarid)) != NC_NOERR) {
            if (kw->verbose) fprintf(stderr, "Unable to Create the Coordinate Variable %s\n", name);
            break;
        }

//--------------------------------------------------------------------------           
// Write the Coordinates: Depends on the Number of Optional Parameters passed

        switch (nparams) {

            case 1:    // Must be the Coordinate Array

                if (kw->debug) fprintf(stdout, "Case #1: Single Coordinate Array\n");

                lerr = writeCoordinateArray(argv[0], kw, ncgrpid, group, name, ncdimid, &ncvarid, &coordcount);
                break;

            case 2:    // Must be the Start and Increment Attribute Arrays

                if (kw->debug) fprintf(stdout, "Case #2: Start and Increment Attribute Arrays\n");

                if ((lerr = createCoordinateArray(argv[0], argv[1], NULL, kw, ncgrpid, group, name, ncvarid,
                                                  ncdimid, &totlength)) != 0)
                    break;

                if ((lerr = writeCoordinateAttribute(argv[0], kw, ncgrpid, group, name, "start", NC_DOUBLE,
                                                     ncvarid, &attlength0, &totlength)) != 0)
                    break;

                if ((lerr = writeCoordinateAttribute(argv[1], kw, ncgrpid, group, name, "increment", NC_DOUBLE,
                                                     ncvarid, &attlength1, &totlength)) != 0)
                    break;

                if (attlength0 != attlength1) {
                    lerr = -1;
                    if (kw->verbose)
                        fprintf(stderr,
                                "Coordinate %s Domain Data (start, increment) have incompatible lengths [%d] and "
                                        "[%d]. Please correct.\n", name, attlength0, attlength1);
                    break;
                }

                if (attlength0 != 1) {
                    lerr = -1;
                    if (kw->verbose)
                        fprintf(stderr,
                                "A Domain Count Array is necessary when the Start and Increment Domain data are "
                                        "multi-valued. Please correct the definition of Coordinate Variable %s\n",
                                name);
                    break;
                }

                if ((err = nc_put_att_uint(ncgrpid, ncvarid, "count", NC_UINT, (size_t) attlength0, &totlength)) !=
                    NC_NOERR) {
                    if (kw->verbose)
                        fprintf(stderr, "Unable to Write Data to Coordinate Variable %s Domain Attribute count\n",
                                name);

                    break;
                }

                coordcount = totlength;

                break;

            case 3:

                if (kw->debug) fprintf(stdout, "Case #3: Start, Increment and Count Attribute Arrays\n");

                if (argv[2]->type ==
                    IDL_TYP_ULONG) {    // If the type of arg 3 is unsigned int then standard data trio passed

		  if (kw->debug) fprintf(stdout, "Case #3a: Start, Increment and Count Arrays\n");

                    if ((lerr = createCoordinateArray(argv[0], argv[1], argv[2], kw, ncgrpid, group, name, ncvarid,
                                                      ncdimid, &totlength)) != 0)
                        break;

                    if ((lerr = writeCoordinateAttribute(argv[0], kw, ncgrpid, group, name, "start", NC_DOUBLE,
                                                         ncvarid, &attlength0, &totlength)) != 0)
                        break;

                    if ((lerr = writeCoordinateAttribute(argv[1], kw, ncgrpid, group, name, "increment", NC_DOUBLE,
                                                         ncvarid, &attlength1, &totlength)) != 0)
                        break;

                    if ((lerr = writeCoordinateAttribute(argv[2], kw, ncgrpid, group, name, "count", NC_UINT,
                                                         ncvarid, &attlength2, &totlength)) != 0)
                        break;

                    if (attlength0 != attlength1 || attlength0 != attlength2) {
                        lerr = -1;
                        if (kw->verbose)
                            fprintf(stderr,
                                    "Coordinate %s Domain Data (start, increment, count) have incompatible lengths "
                                            "[%d],[%d] and [%d]. Please correct.\n", name, attlength0, attlength1,
                                    attlength2);
                        break;
                    }

                    coordcount = totlength;

                } else {                    // Data Array + Start and Increment Arrays

		  if (kw->debug) fprintf(stdout, "Case #3b: Data and start & increment attribute arrays\n");

                    if ((lerr = writeCoordinateArray(argv[0], kw, ncgrpid, group, name, ncdimid, &ncvarid,
                                                     &coordcount)) != 0)
                        break;

                    if ((lerr = writeCoordinateAttribute(argv[1], kw, ncgrpid, group, name, "start", NC_DOUBLE,
                                                         ncvarid, &attlength0, &totlength)) != 0)
                        break;
                    if ((lerr = writeCoordinateAttribute(argv[2], kw, ncgrpid, group, name, "increment", NC_DOUBLE,
                                                         ncvarid, &attlength1, &totlength)) != 0)
                        break;

                    if (attlength0 != attlength1) {
                        lerr = -1;
                        if (kw->verbose)
                            fprintf(stderr, "Coordinate %s Data (array, start, increment) have incompatible lengths "
                                    "[%d], [%d] and [%d]. Please correct.\n", name, coordcount, attlength0, attlength1);
                        break;
                    }

                    if (attlength0 != 1) {
                        lerr = -1;
                        if (kw->verbose)
                            fprintf(stderr,
                                    "A Domain Count Array is necessary when the Start and Increment Domain data are "
                                            "multi-valued. Please correct the definition of Coordinate Variable %s\n",
                                    name);
                        break;
                    }

                    if ((err = nc_put_att_uint(ncgrpid, ncvarid, "count", NC_UINT, (size_t) attlength0, &coordcount)) !=
                        NC_NOERR) {
                        if (kw->verbose)
                            fprintf(stderr, "Unable to Write Data to Coordinate Variable %s Domain Attribute count\n",
                                    name);
                    }
                    break;

                }
                break;

            case 4:            // Data Array + Start, Increment and Count Domain Arrays

                if ((lerr = writeCoordinateArray(argv[0], kw, ncgrpid, group, name, ncdimid, &ncvarid, &coordcount)) !=
                    0)
                    break;

                if ((lerr = writeCoordinateAttribute(argv[1], kw, ncgrpid, group, name, "start", NC_DOUBLE,
                                                     ncvarid, &attlength0, &totlength)) != 0)
                    break;
                if ((lerr = writeCoordinateAttribute(argv[2], kw, ncgrpid, group, name, "increment", NC_DOUBLE,
                                                     ncvarid, &attlength1, &totlength)) != 0)
                    break;
                if ((lerr = writeCoordinateAttribute(argv[3], kw, ncgrpid, group, name, "count", NC_UINT,
                                                     ncvarid, &attlength2, &totlength)) != 0)
                    break;

                if (attlength0 != attlength1 || attlength0 != attlength2) {
                    lerr = -1;
                    if (kw->verbose)
                        fprintf(stderr, "Coordinate Domain %s Data (start, increment, count) have incompatible lengths "
                                        "[%d], [%d] and [%d]. Please correct.\n", name, attlength0,
                                attlength1, attlength2);
                    break;
                }
                if (totlength != coordcount) {
                    lerr = -1;
                    if (kw->verbose)
                        fprintf(stderr, "The total count [%d] of values generated from Domain Data (start, increment, "
                                "count) is incompatible with the number of entries in the coordinate %s array "
                                "[%d]. Please correct.\n", totlength, name, coordcount);
                    break;
                }


                break;
        }

        if (err != NC_NOERR || lerr != 0) break;

        if (err != NC_NOERR) {
            if (kw->verbose) fprintf(stderr, "Unable to Write Data to the Coordinate Variable %s\n", name);
            break;
        }

// Write Attributes

        if (kw->is_label) {
            label = IDL_STRING_STR(&kw->label);
            if ((err = nc_put_att_text(ncgrpid, ncvarid, "label", strlen(label), label)) != NC_NOERR) {
                if (kw->verbose) fprintf(stderr, "Unable to Write the Label Attribute of variable: %s\n", name);
                break;
            }
        }

        if (kw->is_class) {
            class = IDL_STRING_STR(&kw->class);
            if ((err = nc_put_att_text(ncgrpid, ncvarid, "class", strlen(class), class)) != NC_NOERR) {
                if (kw->verbose) fprintf(stderr, "Unable to Write the Class Attribute of variable: %s\n", name);
                break;
            }
        }
/*	 
	 if(STR_EQUALS(class, "time")){
            if ((err = nc_put_att_text(ncgrpid, ncvarid, "class", strlen(class), class)) != NC_NOERR){
               if(kw->verbose) fprintf(stderr,"Unable to Write the Class Attribute of variable: %s\n", name);
               break;      
            }	    
         } else {
            if(kw->verbose){
	       fprintf(stderr,"Only a \"time\" Class is acceptable for the Coordinate variable: %s\n", name);
            }
	    break;      
	 }
*/

        if (kw->is_title) {
            title = IDL_STRING_STR(&kw->title);
            if ((err = nc_put_att_text(ncgrpid, ncvarid, "title", strlen(title), title)) != NC_NOERR) {
                if (kw->verbose) fprintf(stderr, "Unable to Write the Title Attribute of variable: %s\n", name);
                break;
            }
        }

        if (kw->is_comment) {
            comment = IDL_STRING_STR(&kw->comment);
            if ((err = nc_put_att_text(ncgrpid, ncvarid, "comment", strlen(comment), comment)) != NC_NOERR) {
                if (kw->verbose) fprintf(stderr, "Unable to Write the Comment Attribute of variable: %s\n", name);
                break;
            }
        }

//--------------------------------------------------------------------------           
// If there is a named error variable then check: it exists in this group; it has the same length.

        if (kw->is_errors) {
            int ncvarerrid, lvarerr, ndims, dimid;
            char *errors = IDL_STRING_STR(&kw->errors);

            if ((err = nc_inq_varid(ncgrpid, errors, &ncvarerrid)) != NC_NOERR) {
                if (kw->verbose)
                    fprintf(stderr, "The Coordinate variable %s Error variable %s does not exist.\n", name, errors);
                break;
            }

            if ((err = nc_inq_varndims(ncgrpid, ncvarerrid, &ndims)) != NC_NOERR) {
                if (kw->verbose)
                    fprintf(stderr, "Unable to Query the number of Dimensions of the Error Variable %s\n", errors);
                break;
            }

            if (ndims != 1) {
                lerr = -1;
                if (kw->verbose)
                    fprintf(stderr, "The rank of the Error Variable %s is Inconsistent with a Coordinate.\n", errors);
                break;
            }

            if ((err = nc_inq_vardimid(ncgrpid, ncvarerrid, &dimid)) != NC_NOERR) {
                if (kw->verbose) fprintf(stderr, "Unable to Query the Dimension ID of the Error Variable %s\n", errors);
                break;
            }

            if ((err = nc_inq_dimlen(ncgrpid, dimid, (size_t *) &lvarerr)) != NC_NOERR) {
                if (kw->verbose)
                    fprintf(stderr, "Unable to Query the Dimension Length of the Error Variable %s\n", errors);
                break;
            }

            if (coordcount != lvarerr) {
                lerr = -1;
                if (kw->verbose)
                    fprintf(stderr,
                            "The Length [%d] of the Error Variable %s is Inconsistent with the Coordinate Length %s [%d].\n",
                            lvarerr, errors, name, coordcount);
                break;
            }

            if ((err = nc_put_att_text(ncgrpid, ncvarid, "errors", strlen(errors), errors)) != NC_NOERR) {
                if (kw->verbose)
                    fprintf(stderr, "Unable to Write the Errors Attribute of Coordinate Variable: %s\n", name);
                break;
            }

        }

//-------------------------------------------------------------------------- 
// Write Units: Last attribute written - in case units not SI compliant

        if (kw->is_units) {
            units = IDL_STRING_STR(&kw->units);

            if (!kw->notstrict) {
                if (!testUnitsCompliance(kw, units)) {        // Test for SI Compliance
                    if (kw->verbose) fprintf(stderr, "Unable to Write the Units Attribute of variable: %s\n", name);
                    lerr = -1;
                    break;
                }
            }

            if ((err = nc_put_att_text(ncgrpid, ncvarid, "units", strlen(units), units)) != NC_NOERR) {
                if (kw->verbose) fprintf(stderr, "Unable to Write the Units Attribute of variable: %s\n", name);
                break;
            }
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


//-------------------------------------------------------------------------- 
//--------------------------------------------------------------------------  

int swapType(int type) {

// Translate IDL to netCDF4 Type

    switch (type) {
        case IDL_TYP_FLOAT:        // 4 byte float
            return NC_FLOAT;

        case IDL_TYP_DOUBLE:        // 8 byte float
            return NC_DOUBLE;

        case IDL_TYP_LONG64:        // 8 byte signed integer
            return NC_INT64;

        case IDL_TYP_LONG:        // 4 byte signed integer
            return NC_INT;

        case IDL_TYP_INT:        // 2 byte signed integer
            return NC_SHORT;

        case IDL_TYP_ULONG64:        // 8 byte unsigned integer
            return NC_UINT64;

        case IDL_TYP_ULONG:        // 4 byte unsigned integer
            return NC_UINT;

        case IDL_TYP_UINT:        // 2 byte unsigned integer
            return NC_USHORT;

        case IDL_TYP_COMPLEX:        // structure with 2 4 byte floats
            return ctype;

        case IDL_TYP_DCOMPLEX:        // structure with 2 8 byte floats
            return dctype;

        case IDL_TYP_BYTE:        // 1 byte signed char (options NC_BYTE, NC_UBYTE, NC_CHAR ?)
            return NC_BYTE;

    }
    return NC_NAT;
}


int writeCoordinateArray(IDL_VPTR argv, KW_RESULT *kw, int ncgrpid, char *group, char *name, int ncdimid,
                         int *ncvarid, unsigned int *length) {

    int i, err, lerr, isArray, rank = 1, isUnlimited;
    void *data = NULL;
    int ndims, dimlength, dimids[NC_MAX_DIMS];
    size_t chunking;
    nc_type xtype;

    float fs;
    double ds;
    long long ls;
    int is;
    short ss;
    signed char bs;
    unsigned short uss;
    size_t uis;
    unsigned long long uls;
    COMPLEX cs;
    DCOMPLEX dcs;

    size_t start[1] = { 0 };    // For Unlimited Dimensions
    size_t count[1];

//--------------------------------------------------------------------------      
// Error Trap  

    err = NC_NOERR;
    lerr = NC_NOERR;

    do {

//--------------------------------------------------------------------------         
// Check No structures or Strings have been passed

        if (argv->type == IDL_TYP_STRUCT || argv->type == IDL_TYP_STRING) {
            if (kw->verbose)
                fprintf(stderr,
                        "Neither Structured or String Data arrays can be passed for Coordinates (%s) - please correct.\n",
                        name);
            lerr = -1;
            break;
        }

//--------------------------------------------------------------------------                  
// Translate IDL to netCDF4 Type

        if ((xtype = swapType(argv->type)) == NC_NAT) {
            if (kw->verbose)
                fprintf(stderr, "The IDL Data type for the Coordinate %s cannot be converted into netCDF4\n", name);
            lerr = -1;
            break;
        }

//--------------------------------------------------------------------------                  
// Scalar or Array Data

        if (argv->flags & IDL_V_ARR) {
            isArray = 1;
            rank = (int) argv->value.arr->n_dim;                // Number of Dimensions
            *length = (unsigned int) argv->value.arr->n_elts;        // Number of Elements
            data = (void *) argv->value.arr->data;            // Array of Values
        } else {
            isArray = 0;
            rank = 0;
            *length = 1;
        }

//--------------------------------------------------------------------------                  
// Check the Rank

        if (rank > 1) {
            if (kw->verbose)
                fprintf(stderr, "Data for the Coordinate %s must be either a scalar "
                        "or a rank 1 array - please correct.\n", name);
            lerr = -1;
            break;
        }

//--------------------------------------------------------------------------                  
// Is the dimension of Unlimited Length?

        if ((err = nc_inq_unlimdims(ncgrpid, &ndims, dimids)) != NC_NOERR) {
            if (kw->verbose) fprintf(stderr, "Unable to List Unlimited Dimensions in Group %s \n", group);
            break;
        }

        isUnlimited = 0;

        for (i = 0; i < ndims; i++) {
            if (ncdimid == dimids[i]) {
                isUnlimited = 1;
                break;
            }
        }


        if (!isUnlimited) {

	  fprintf(stdout, "It is not unlimited\n");
// Get current length of the dimension

            if ((err = nc_inq_dimlen(ncgrpid, ncdimid, (size_t *) &dimlength)) != NC_NOERR) {
                if (kw->verbose) fprintf(stderr, "Unable to obtain Length of Dimension %s \n", name);
                break;
            }

// Test Length is Consistent with the Dimension

            if (*length != dimlength) {
                if (kw->verbose)
                    fprintf(stderr, "An Inconsistency with the Length of the Limited Coordinate Variable %s has been "
                                    "detected: the Data Length [%d] is not the same as Dimension length [%d]\n", name, *length,
                            dimlength);
                lerr = -1;
                break;
            }

            if (kw->debug)fprintf(stdout, "Writing Data to the Limited Length Coordinate Variable %s\n", name);

        }

//--------------------------------------------------------------------------                  
// Chunking is always ON (Only for LIMITED length arrays)

        if (!isUnlimited && *length > 1) {
            chunking = *length;
            if (kw->is_chunksize) chunking = (size_t) kw->chunksize;

            if (kw->debug)fprintf(stdout, "Using Chunking = %d\n", (int) chunking);

            if ((err = nc_def_var_chunking(ncgrpid, *ncvarid, NC_CHUNKED, &chunking)) != NC_NOERR) {
                if (kw->verbose) fprintf(stderr, "Unable to set the Chunking Properties of Dimension %s \n", name);
                break;
            }
        }

//--------------------------------------------------------------------------                  
// Compression (Only for LIMITED length arrays) (What is the minimum length where a benefit is accrued?)

        if (!isUnlimited && kw->is_compression && kw->compression > 0 && *length > 1) {
            if (kw->debug)fprintf(stdout, "Using Compression level %d\n", (int) kw->compression);
            if ((err = nc_def_var_deflate(ncgrpid, *ncvarid, NC_NOSHUFFLE, 1, kw->compression)) != NC_NOERR) {
                if (kw->verbose) fprintf(stderr, "Unable to set the Compression Properties of Dimension %s \n", name);
                break;
            }
        }

//--------------------------------------------------------------------------                  
// Write the Data

        count[0] = *length;        // For unlimited dimensions, use the length of the data array

        switch (xtype) {

            case NC_FLOAT:
                if (!isArray) {
                    fs = (float) IDL_DoubleScalar(argv);
                    data = &fs;
                }

                if (kw->debug) {
                    int i;
                    float *fp = (float *) data;
                    fprintf(stdout, "\n\nWriting Float Data\n");
                    fprintf(stdout, "Group ID %d\n", ncgrpid);
                    fprintf(stdout, "Var ID   %d\n", *ncvarid);
                    fprintf(stdout, "Length   %d\n", *length);
                    for (i = 0; i < *length; i++) fprintf(stdout, "[%d] %f\n", i, fp[i]);
                }

                if (isUnlimited) {
		  fprintf(stdout, "It is unlimited.\n");
                    err = nc_put_vara_float(ncgrpid, *ncvarid, start, count, (float *) data);
                } else {
		  fprintf(stdout, "It is not unlimited.\n");
                    err = nc_put_var_float(ncgrpid, *ncvarid, (float *) data);
                }
		fprintf(stdout, "Written.\n");

                break;

            case NC_DOUBLE:
                if (!isArray) {
                    ds = (double) IDL_DoubleScalar(argv);
                    data = &ds;
                }
                if (isUnlimited) {
                    err = nc_put_vara_double(ncgrpid, *ncvarid, start, count, (double *) data);
                } else {
                    err = nc_put_var_double(ncgrpid, *ncvarid, (double *) data);
                }
                break;

            case NC_INT64:
                if (!isArray) {
                    ls = IDL_Long64Scalar(argv);
                    data = &ls;
                }
                if (isUnlimited) {
                    err = nc_put_vara_longlong(ncgrpid, *ncvarid, start, count,
                                               (long long *) data);
                } else {
                    err = nc_put_var_longlong(ncgrpid, *ncvarid, (long long *) data);
                }
                break;

            case NC_INT:
                if (!isArray) {
                    is = (int) IDL_LongScalar(argv);
                    data = &is;
                }
                if (isUnlimited) {
                    err = nc_put_vara_int(ncgrpid, *ncvarid, start, count, (int *) data);
                } else {
                    err = nc_put_var_int(ncgrpid, *ncvarid, (int *) data);
                }
                break;

            case NC_SHORT:
                if (!isArray) {
                    ss = (short) IDL_LongScalar(argv);
                    data = &ss;
                }
                if (isUnlimited) {
                    err = nc_put_vara_short(ncgrpid, *ncvarid, start, count, (short *) data);
                } else {
                    err = nc_put_var_short(ncgrpid, *ncvarid, (short *) data);
                }
                break;

            case NC_UINT64:
                if (!isArray) {
                    uls = IDL_ULong64Scalar(argv);
                    data = &uls;
                }
                if (isUnlimited) {
                    err = nc_put_vara_ulonglong(ncgrpid, *ncvarid, start, count,
                                                (unsigned long long *) data);
                } else {
                    err = nc_put_var_ulonglong(ncgrpid, *ncvarid, (unsigned long long *) data);
                }
                break;

            case NC_UINT:
                if (!isArray) {
                    uis = IDL_ULongScalar(argv);
                    data = &uis;
                }
                if (isUnlimited) {
                    err = nc_put_vara_uint(ncgrpid, *ncvarid, start, count,
                                           (unsigned int *) data);
                } else {
                    err = nc_put_var_uint(ncgrpid, *ncvarid, (unsigned int *) data);
                }
                break;

            case NC_USHORT:
                if (!isArray) {
                    uss = (unsigned short) IDL_ULongScalar(argv);
                    data = &uss;
                }
                if (isUnlimited) {
                    err = nc_put_vara_ushort(ncgrpid, *ncvarid, start, count,
                                             (unsigned short *) data);
                } else {
                    err = nc_put_var_ushort(ncgrpid, *ncvarid, (unsigned short *) data);
                }
                break;

            case NC_BYTE:
                if (!isArray) {
                    bs = (signed char) IDL_LongScalar(argv);
                    data = &bs;
                }
                if (isUnlimited) {
                    err = nc_put_vara_schar(ncgrpid, *ncvarid, start, count,
                                            (signed char *) data);
                } else {
                    err = nc_put_var_schar(ncgrpid, *ncvarid, (signed char *) data);
                }
                break;

            default:
                if (xtype == ctype || xtype == dctype) {
                    if (kw->debug)fprintf(stdout, "the Coordinate Variable %s is COMPLEX [%d]\n", name, (int) xtype);
                    if (!isArray) {
                        if (xtype == ctype) {
                            cs.real = argv->value.cmp.r;
                            cs.imaginary = argv->value.cmp.i;
                            data = &cs;
                        } else {
                            dcs.real = argv->value.dcmp.r;
                            dcs.imaginary = argv->value.dcmp.i;
                            data = &dcs;
                        }
                    }
                    if (isUnlimited) {
                        err = nc_put_vara(ncgrpid, *ncvarid, start, count, (void *) data);
                    } else {
                        err = nc_put_var(ncgrpid, *ncvarid, (void *) data);
                    }
                }
                break;
        }

	fprintf(stdout, "After switch");

        if (err != NC_NOERR || lerr != 0) break;

//--------------------------------------------------------------------------      
// End of Error Trap  

    } while (0);

//--------------------------------------------------------------------------      

    if (err != NC_NOERR) {
        if (kw->verbose) {
            fprintf(stderr, "Unable to Write Data to Coordinate Variable %s\n", name);
            fprintf(stderr, "Error Report: %s\n", nc_strerror(err));
        }
    } else {
        if (lerr != NC_NOERR) err = lerr;
    }

    return err;
}


int writeCoordinateAttribute(IDL_VPTR argv, KW_RESULT *kw, int ncgrpid, char *group, char *name,
                             char *attribute, nc_type atype, int ncvarid, unsigned int *length,
                             unsigned int *totlength) {

    int i, err = NC_NOERR, isArray, rank = 1;
    void *data = NULL;
    nc_type xtype;

    double ds;
    unsigned int uis;

//--------------------------------------------------------------------------         
// Check No structures or Strings have been passed

    if (argv->type == IDL_TYP_STRUCT || argv->type == IDL_TYP_STRING) {
        if (kw->verbose)
            fprintf(stderr, "Neither Structures or String arrays can be passed for Coordinates "
                    "Domain Attribute (%s) - please correct.\n", name);
        return -1;
    }

//--------------------------------------------------------------------------         
// Translate IDL to netCDF4 Type

    if ((xtype = swapType(argv->type)) == NC_NAT) {
        if (kw->verbose)
            fprintf(stderr, "The IDL Data type for the Coordinate %s Domain Attribute %s cannot be "
                    "converted into netCDF4\n", name, attribute);
        return -1;
    }

//--------------------------------------------------------------------------         
// Is the Type Compliant?

    if (xtype != atype) {
        if (kw->verbose) {
            if (STR_IEQUALS(attribute, "count")) {
                fprintf(stderr, "The IDL Data type for the Coordinate %s Domain Attribute %s must be "
                        "an Unsigned Integer scalar or array\n", name, attribute);
            } else {
                fprintf(stderr, "The IDL Data type for the Coordinate %s Domain Attribute %s must be "
                        "a Double scalar or array\n", name, attribute);
            }
        }
        return -1;
    }

//--------------------------------------------------------------------------         
// Scalar or Array Data

    if (argv->flags & IDL_V_ARR) {
        isArray = 1;
        rank = (int) argv->value.arr->n_dim;                // Number of Dimensions
        *length = (unsigned int) argv->value.arr->n_elts;            // Number of Elements
        data = (void *) argv->value.arr->data;                // Array of Values
    } else {
        isArray = 0;
        rank = 0;
        *length = 1;
    }

//--------------------------------------------------------------------------            
// Check the Rank

    if (rank > 1) {
        if (kw->verbose)
            fprintf(stderr, "Data for the Coordinate %s Domain Attribute %s must be either a scalar "
                    "or a rank 1 array - please correct.\n", name, attribute);
        return -1;
    }

//--------------------------------------------------------------------------         
// Write the Attribute

    switch (xtype) {

        case NC_DOUBLE:
            if (!isArray) {
                ds = (double) IDL_DoubleScalar(argv);
                data = &ds;
            }

            if (kw->debug) {
                int i;
                float *fp = (float *) data;
                fprintf(stdout, "\n\nWriting %s Data\n", attribute);
                fprintf(stdout, "Group ID %d\n", ncgrpid);
                fprintf(stdout, "Var ID   %d\n", ncvarid);
                fprintf(stdout, "Length   %d\n", *length);
                for (i = 0; i < *length; i++) fprintf(stdout, "[%d] %f\n", i, fp[i]);
            }

            err = nc_put_att_double(ncgrpid, ncvarid, attribute, xtype, *length, (double *) data);
            break;

        case NC_UINT:
            if (!isArray) {
                uis = IDL_ULongScalar(argv);
                data = &uis;
            }
            err = nc_put_att_uint(ncgrpid, ncvarid, attribute, xtype, *length, (unsigned int *) data);

            if (*length > 1) {
                unsigned int *udata = (unsigned int *) data;
                *totlength = 0;
                for (i = 0; i < *length; i++)
                    *totlength = *totlength + udata[i];        // Total length of the expanded data array
            } else {
                *totlength = *((unsigned int *) data);
            }

            break;

    }

    if (err != NC_NOERR) {
        if (kw->verbose) {
            fprintf(stderr, "Unable to Write Data to Coordinate Variable %s Domain Attribute %s\n", name, attribute);
            fprintf(stderr, "Error Report: %s\n", nc_strerror(err));
        }
        return err;
    }

    return err;
}


int createCoordinateArray(IDL_VPTR argv0, IDL_VPTR argv1, IDL_VPTR argv2, KW_RESULT *kw, int ncgrpid,
                          char *group, char *name, int ncvarid, int ncdimid, unsigned int *totlength) {

    int i, j, err = NC_NOERR, isArray, rank = 1, length, isUnlimited;
    int ndims, dimlength, dimids[NC_MAX_DIMS];
    size_t chunking;

    nc_type xtype;

    double ds, di;
    double *start, *increment, *data;

    unsigned int uis;
    unsigned int *count;

//--------------------------------------------------------------------------         
// Error trap

    do {

//--------------------------------------------------------------------------                  
// Is the dimension of Unlimited Length?

        if ((err = nc_inq_unlimdims(ncgrpid, &ndims, dimids)) != NC_NOERR) {
            if (kw->verbose) fprintf(stderr, "Unable to List Unlimited Dimensions in Group %s \n", group);
            break;
        }

        isUnlimited = 0;

        for (i = 0; i < ndims; i++) {
            if (ncdimid == dimids[i]) {
                isUnlimited = 1;
                break;
            }
        }

        if (!isUnlimited) {

           // Get current length of the dimension

            if ((err = nc_inq_dimlen(ncgrpid, ncdimid, (size_t *) &dimlength)) != NC_NOERR) {
                if (kw->verbose) fprintf(stderr, "Unable to obtain Length of Dimension %s \n", name);
                break;
            }

        } 

//--------------------------------------------------------------------------         
// Translate IDL to netCDF4 Type

        if (argv0->type != argv1->type) {
            if (kw->verbose)
                fprintf(stderr, "The IDL Domain Data types for the Coordinate %s Array are Different!\n", name);
            err = -1;
            break;
        }

        if (argv2 != NULL) {
            if ((xtype = swapType(argv2->type)) == NC_NAT) {
                if (kw->verbose)
                    fprintf(stderr, "The IDL Count Domain Data type for the Coordinate %s Array cannot be "
                            "converted into netCDF4.\n", name);
                err = -1;
                break;
            }
            if (xtype != NC_UINT) {
                if (kw->verbose)
                    fprintf(stderr, "The IDL Count Domain Data type for the Coordinate %s Array must be "
                            "of IDL type Unsigned Long.\n", name);
                err = -1;
                break;
            }
        }

        if ((xtype = swapType(argv0->type)) == NC_NAT) {
            if (kw->verbose)
                fprintf(stderr, "The IDL Domain Data types for the Coordinate %s Array cannot be "
                        "converted into netCDF4.\n", name);
            err = -1;
            break;
        }

        if (xtype != NC_DOUBLE) {
            if (kw->verbose)
                fprintf(stderr, "The IDL Start and Increment Domain Data types for the Coordinate %s Array must be "
                        "of IDL type Double.\n", name);
            err = -1;
            break;
        }



//--------------------------------------------------------------------------         
// Scalar or Array Data

        if (argv0->flags & IDL_V_ARR) {
            if (!(argv1->flags & IDL_V_ARR)) {
                if (kw->verbose)
                    fprintf(stderr, "The IDL Domain Data containers for the Coordinate %s Array are Different!\n",
                            name);
                err = -1;
                break;
            }
            if (argv2 != NULL) {
                if (!(argv2->flags & IDL_V_ARR)) {
                    if (kw->verbose)
                        fprintf(stderr, "The IDL Domain Data containers for the Coordinate %s Array are Different!\n",
                                name);
                    err = -1;
                    break;
                }
            }
	    if(argv1->flags && argv2 == NULL){
	      // If an array is given for argv1 then we also need argv2 to be an array.
	      // Can only miss argv2 if this is a single domain.
 	      if (kw->verbose) 
		fprintf(stderr, "The dimension has more than one domain, please give a count array for the third argument.\n");
  	      err = -1;
	      break;
	    }

            isArray = 1;

            rank = (int) argv0->value.arr->n_dim;
            if (rank != (int) argv1->value.arr->n_dim) {
                if (kw->verbose)
                    fprintf(stderr, "The IDL Domain Data Array Ranks for the Coordinate %s Array are Different!\n",
                            name);
                err = -1;
                break;
            }
            if (argv2 != NULL) {
                if (rank != (int) argv2->value.arr->n_dim) {
                    if (kw->verbose)
                        fprintf(stderr, "The IDL Domain Data Array Ranks for the Coordinate %s Array are Different!\n",
                                name);
                    err = -1;
                    break;
                }
            }
            if (rank != 1) {
                if (kw->verbose)
                    fprintf(stderr, "The IDL Domain Data Array Ranks for the Coordinate %s Array are Invalid!\n", name);
                err = -1;
                break;
            }

            length = (unsigned int) argv0->value.arr->n_elts;
            if (length != (unsigned int) argv1->value.arr->n_elts) {
                if (kw->verbose)
                    fprintf(stderr, "The IDL Domain Data Array Lengths for the Coordinate %s Array are Different!\n",
                            name);
                err = -1;
                break;
            }
            if (argv2 != NULL) {
                if (length != (unsigned int) argv2->value.arr->n_elts) {
                    if (kw->verbose)
                        fprintf(stderr,
                                "The IDL Domain Data Array Lengths for the Coordinate %s Array are Different!\n", name);
                    err = -1;
                    break;
                }
            }

            start = (double *) argv0->value.arr->data;                // Array of Starting Values
            increment = (double *) argv1->value.arr->data;                // Array of Increment Values
            if (argv2 != NULL) {
	      fprintf(stdout, "Argv2 has %ld elements %ld bytes\n", (long) argv2->value.arr->n_elts, (long) argv2->value.arr->arr_len);
	      fprintf(stdout, "Sizeof int %d sizeof long %d size_t %d", (int)sizeof(int), (int)sizeof(long), (int)sizeof(size_t));
                count = (unsigned int *) argv2->value.arr->data;            // Array of Count Values

		fprintf(stdout, "Length is %d\n", length);
		for (j = 0; j < length; j++) {
		  fprintf(stdout, "j %d count %d\n", j, count[j]);
		}
            } else {
                uis = 1;
                count = &uis;
            }


        } else {
            if ((argv1->flags & IDL_V_ARR)) {
                if (kw->verbose)
                    fprintf(stderr, "The IDL Domain Data containers for the Coordinate %s Array are Different!\n",
                            name);
                return -1;
            }
            isArray = 0;
            rank = 0;
            length = 1;

            ds = (double) IDL_DoubleScalar(argv0);
            start = &ds;
            di = (double) IDL_DoubleScalar(argv1);
            increment = &di;
            if (argv2 != NULL) {
                uis = IDL_ULongScalar(argv2);
                count = &uis;
            } else {
                uis = 1;
                count = &uis;
            }

            if (kw->debug) {
	      fprintf(stdout, "Scalar Domain Data: count = %d, dimension = %d\n", count[0], (int) dimlength);
	      fprintf(stdout, "start = %e, increment = %e, count =%d\n", *start, *increment, count[0]);
            }
        }

// Ensure the Length is Consistent with the Dimension

        if (!isUnlimited && argv2 == NULL) {
            if (!isArray) {
	        fprintf(stdout, "Not an array. Setting length to dimlength\n");
	        length = (unsigned int) dimlength;
	        uis = length;
  	        count = &uis;
            }/*  else { */
	    /*     fprintf(stdout, "Is an array. Length is %d\n", length); */
            /*     if (length > 0) { */
	    /* 	  uis = dimlength / length; */
            /*       count = &uis; */
            /*     } */
            /* } */
        } else if (isUnlimited) {
	  // For unlimited dimensions, take the length of the dimension from the current size of the dimension that is to be written
	  if (!isArray){
	    dimlength = uis;
	    length = 1;
	  } else {
   	    // Need also to do something for arrays
	    dimlength = 0;
	    for (j = 0; j < length; j++) {
	      dimlength = dimlength + count[j];
	    }
	  }
	}


//--------------------------------------------------------------------------            
// Build the Array

        if (argv2 != NULL) {
	  fprintf(stdout, "Constructing length %d\n", length);
            *totlength = 0;
            for (j = 0; j < length; j++) {
	      fprintf(stdout, "j %d count %d\n", j, count[j]);
	      *totlength = *totlength + count[j];
	    }
        } else {
	  fprintf(stdout, "totlength is %d\n", length);
            *totlength = length;
        }

        if (kw->debug) fprintf(stdout, "Total Length = %d\n", *totlength);


// Test the Length is Consistent with the Dimension

        if (*totlength != dimlength) {
            if (kw->verbose)
                fprintf(stderr, "An Inconsistency with the Length of the Limited Coordinate Variable %s has been "
                                "detected: the Data Length [%d] is not the same as Dimension length [%d]\n", name, length,
                        dimlength);
            err = -1;
            break;
        }

        data = (double *) malloc(*totlength * sizeof(double));

        if (argv2 != NULL) {
       	    int data_index = 0;
	    int j;
            for (j = 0; j < length; j++) {
	      for (i = 0; i < count[j]; i++) {
		data[data_index++] = start[j] + (double) i * increment[j];
	      }
            }
        } else {
   	    // No count array : single domain only
            if (!isArray) {
         	// Start and increment were scalars	      
                for (i = 0; i < count[0]; i++) data[i] = start[0] + (double) i * increment[0];
            } 
            /* else { */
	    /*     // Start and increment arrays were given ... does this even make sense ???  */
            /*     for (j = 0; j < length; j++) { */
            /*         for (i = 0; i < count[0]; i++) data[i] = start[j] + (double) i * increment[j]; */
            /*     } */
            /* } */
        }


//--------------------------------------------------------------------------                  
// Compression (Only for LIMITED length arrays) (What is the minimum length where a benefit is accrued?)

        if (!isUnlimited && kw->is_compression && kw->compression > 0 && length > 1) {
            if ((err = nc_def_var_deflate(ncgrpid, ncvarid, NC_SHUFFLE, 1, kw->compression)) != NC_NOERR) {
                if (kw->verbose) fprintf(stderr, "Unable to set the Compression Properties of Dimension %s \n", name);
                break;
            }
        }

//--------------------------------------------------------------------------                  
// Chunking is always ON (Only for LIMITED length arrays)

        if (!isUnlimited && length > 1) {
            chunking = length;
            if (kw->is_chunksize) chunking = (size_t) kw->chunksize;

            if ((err = nc_def_var_chunking(ncgrpid, ncvarid, NC_CHUNKED, &chunking)) != NC_NOERR) {
                if (kw->verbose) fprintf(stderr, "Unable to set the Chunking Properties of Dimension %s \n", name);
                break;
            }
        }

//--------------------------------------------------------------------------         
// Write the Coordinate Array

        if (isUnlimited) {
            size_t start[1] = {0};    // For Unlimited Dimensions
            size_t count[1];
            count[0] = *totlength;
            err = nc_put_vara_double(ncgrpid, ncvarid, start, count, data);
        } else {
            err = nc_put_var_double(ncgrpid, ncvarid, data);
        }

    } while (0);

//--------------------------------------------------------------------------         
// Housekeeping

    free((void *) data);

    if (err != NC_NOERR) {
        if (kw->verbose) {
            fprintf(stderr, "Unable to Write Data to Coordinate Variable %s \n", name);
            fprintf(stderr, "Error Report: %s\n", nc_strerror(err));
        }
        return err;
    }


    return err;
}


int testUnitsCompliance(KW_RESULT *kw, char *units) {

// Return TRUE (1) or FALSE (0)

    int code;
    ut_unit *encoded = NULL;
    unsigned encoding = UT_UTF8; //UT_ASCII;

    errno = 0;

//--------------------------------------------------------------------------      	 
// Check Units Compliance

    if (units == NULL) return 1;
    if (units[0] == '\0') return 1;

    if (unitSystem == NULL) {
        unitSystem = ut_read_xml(NULL);        // Read and Parse Standard Units Definition XML Documents
        //unitSystem = ut_read_xml("/home/dgm/IDAM/source/idl/putdata/udunits/udunits2.xml");
        if (unitSystem == NULL) {
            code = (int) ut_get_status();
            if (kw->verbose) {
                fprintf(stderr, "UDUNITS System Problem!\n");
                fprintf(stderr, "Error Code: %d\n", (int) code);
                perror("");
            }
            return 0;
        }
        if (!setlocale(LC_CTYPE, "")) {
            if (kw->verbose) fprintf(stderr, "Can't set the specified locale! Check LANG, LC_CTYPE, LC_ALL.\n");
            return 0;
        }
    }

    if ((encoded = ut_parse(unitSystem, units, encoding)) == NULL) {
        if (kw->verbose) {
            code = (int) ut_get_status();
            fprintf(stderr, "Units [%s] are Not SI Compliant. Please correct\n", units);
            if (code == UT_SYNTAX) {
                fprintf(stderr, "Units contain a Syntax Error\n");
            } else {
                if (code == UT_UNKNOWN) {
                    fprintf(stderr, "Units contain an Unknown Identifier\n");
                } else fprintf(stderr, "Error Code: %d\n", (int) code);
                perror("");
            }
        }
        return 0;
    }

    ut_free(encoded);        // Free Resources

    return 1;
} 

