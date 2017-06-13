// putVariable.c
//
// 30Jan2009	DGMuir	Original Version
// 19May2009	DGMuir	Added compression and chunksize
//			Added errors attribute and tests the variable exists
// 07May2010	DGMuir	Corrected test for specific /devices device group
// 30Jan2012	DGMuir	Compacted dimension names by removing white space.
//			Reversed dimensions for rank>1
//--------------------------------------------------------------------------- 

#include "putdata.h"		// IDL DLM API Header

#include <stdlib.h>
#include <clientserver/stringUtils.h>

int putVariable(int nparams, IDL_VPTR argv[], KW_RESULT* kw, int ncgrpid) {

    int err, lerr, i, j, ncdimid, isUnlimited;
    size_t dimlength;
    int length, type, rank, ncvarid;
    int mndims, mdimids[NC_MAX_DIMS];
    int nunlimdimids, unlimdimids[NC_MAX_DIMS];

    int unlimitedCount = 0;
    size_t* extents = NULL;
    size_t* chunking = NULL;

    nc_type xtype;

    char* work = NULL;
    char* token;
    char* group;
    char* name;
    char* device;
    char* title;
    char* comment;
    char* units;
    char* label;
    //    char* class;
    char* dimensions;
    size_t* shape = NULL;
    void* data = NULL;

    size_t scalar[1] = {0};

//---------------------------------------------------------------------------      
//---------------------------------------------------------------------------      
// Extract Keywords and Parameters

    if (kw->is_group) group = IDL_STRING_STR(&kw->group);
    if (kw->is_name) name = IDL_STRING_STR(&kw->name);
    //    if (kw->is_class) class = IDL_STRING_STR(&kw->class);

    if (kw->is_dimensions) dimensions = IDL_STRING_STR(&kw->dimensions);

//--------------------------------------------------------------------------      
// Create an Error Trap

    lerr = NC_NOERR;        // Local Error
    err = NC_NOERR;        // NC Error

    do {

//--------------------------------------------------------------------------      
// Check the Variable does not already exist in this group

        if ((err = nc_inq_varid(ncgrpid, name, &ncvarid)) != NC_NOERR && err != NC_ENOTVAR) {
            if (kw->verbose) fprintf(stderr, "Unable to Check if the Group %s Data Variable %s exists!\n", group, name);
            break;
        }

        if (err == NC_NOERR) {        // The variable exists: Edit Mode?
            lerr = -1;
            if (kw->verbose) {
                fprintf(stderr, "The Data Variable %s already exists in the Group: %s\n", name, group);
                //fprintf(stderr,"Edit mode not implemented!\n");
            }
            break;
        } else err = NC_NOERR;

//--------------------------------------------------------------------------      
// Shape and Type

        type = argv[0]->type;

        if (type == IDL_TYP_STRUCT) {
            if (kw->verbose) fprintf(stderr, "Structured Data Variable Types are Not Supported\n");
            lerr = -1;
            break;
        }

        if (type == IDL_TYP_STRING) {
            if (kw->verbose) fprintf(stderr, "String Data Variable Types are Not Supported\n");
            lerr = -1;
            break;
        }

        if (argv[0]->flags & IDL_V_ARR) {
            IDL_ENSURE_ARRAY(argv[0]);
            rank = (int) argv[0]->value.arr->n_dim;        // Number of Dimensions
            length = (int) argv[0]->value.arr->n_elts;        // Number of Elements
            shape = (size_t*) argv[0]->value.arr->dim;        // Shape
            data = (void*) argv[0]->value.arr->data;
        } else {
            IDL_ENSURE_SCALAR(argv[0]);                // Single Scalar value: Must have a dimension of length 1
            rank = 0;
            length = 1;
            scalar[0] = 1;
            shape = scalar;
        }

        if (kw->debug) {
            fprintf(stdout, "Variable %s:\n", name);
            fprintf(stdout, "Rank     %d:\n", rank);
            fprintf(stdout, "Length   %d:\n", length);
            fprintf(stdout, "IDL Shape: ");
            for (i = 0; i < rank; i++)fprintf(stdout, "[%d]=%d ", i, (int)shape[i]);
            fprintf(stdout, "\n");
            fprintf(stdout, "Dimensions: %s\n", dimensions);
        }

//--------------------------------------------------------------------------      
// Check the variable's dimension names are specified and within the scope of this group

        if (!kw->is_dimensions) {
            lerr = -1;
            if (kw->verbose)
                fprintf(stderr, "The Variable's Dimensions must be Named as an ordered comma delimited list.\n");
            break;
        }

// Parse the dimensions string and Scan for the nearest (in scope) defined dimensions up the hierarchy 

        work = (char*) malloc(sizeof(char) * strlen(dimensions) + 1);

        strcpy(work, dimensions);
        TrimString(work);
        LeftTrimString(work);

        mndims = 0;

        //More than one dimension
        if ((token = strtok(work, ",")) != NULL) {        // Tokenise the variable's list of Dimensions

            // Get the ID of the first named dimension (remove white space)
            char* trimtoken = (char*) malloc((strlen(token) + 1) * sizeof(char));
            strcpy(trimtoken, token);
            TrimString(trimtoken);
            LeftTrimString(trimtoken);

            if ((err = nc_inq_dimid(ncgrpid, trimtoken, &ncdimid)) != NC_NOERR) {
                err = testDimension(ncgrpid, kw, trimtoken, 1, &ncdimid); //ncdimid = id of this dimension
                if (err != NC_NOERR || ncdimid < 0) {
                    lerr = -1;
                    if (kw->verbose) {
                        fprintf(stderr, "Unable to Identify Dimensions %s from Group [%d] %s\n", trimtoken, ncgrpid,
                                group);
                        fprintf(stderr, "Dimensions must be in Scope of the Group\n");
                    }
                    free((void*) trimtoken);
                    break;
                }
            }

            if (rank > 0){
               mdimids[rank - 1 - mndims++] = ncdimid;    // *** Reverse dimension IDs
	    } else {
              // Scalar 
	       mdimids[0] = ncdimid;
	       mndims = 1;
	    }

            if (kw->debug) {
                fprintf(stdout, "Rank: %d, mndims: %d\n", rank, mndims);
                fprintf(stdout, "Coordinate Dimension %s has ID %d\n", trimtoken, ncdimid);
            }

            free((void*) trimtoken);

            while ((token = strtok(NULL, ",")) != NULL) {    // Repeat for the next dimension

                // Get the ID of the same named dimension
                char* trimtoken = (char*) malloc((strlen(token) + 1) * sizeof(char));
                strcpy(trimtoken, token);
                TrimString(trimtoken);
                LeftTrimString(trimtoken);

                if ((err = nc_inq_dimid(ncgrpid, trimtoken, &ncdimid)) != NC_NOERR) {
                    err = testDimension(ncgrpid, kw, trimtoken, 1, &ncdimid);
                    if (err != NC_NOERR || ncdimid < 0) {
                        lerr = -1;
                        if (kw->verbose) {
                            fprintf(stderr, "Unable to Identify Dimensions %s from Group [%d] %s\n", trimtoken, ncgrpid,
                                    group);
                            fprintf(stderr, "Dimensions must be in Scope of the Group\n");
                        }
                        break;
                    }
                }

                mdimids[rank - 1 - mndims++] = ncdimid;        // *** Reverse dimension IDs

                if (kw->debug) {
                    fprintf(stdout, "Rank: %d, mndims: %d\n", rank, mndims);
                    fprintf(stdout, "Coordinate Dimension %s has ID %d\n", trimtoken, ncdimid);
                }

                free((void*) trimtoken);
            }

            if (err != NC_NOERR) break;
        }

        if ((rank > 0 && mndims != rank) || (rank == 0 && mndims != 1)) {
            lerr = -1;
            if (kw->verbose)
                fprintf(stderr, "the Rank [%d] of the Data Variable [%s] is not consistent with the declared "
                        "number of dimensions %d\n", rank, name, mndims);
            break;
        }


//--------------------------------------------------------------------------      
// Translate IDL to netCDF4 Type

        xtype = swapType(type);

        if (kw->debug) {
            fprintf(stdout, "\nUser defined Compound Types\n ");
            fprintf(stdout, "COMPLEX       : %d\n", (int) ctype);
            fprintf(stdout, "Double COMPLEX: %d\n\n", (int) dctype);

            fprintf(stdout, "Creating the Data Variable %s \n", name);
            fprintf(stdout, "Type ID %d \n", xtype);
            fprintf(stdout, "Dimension Count %d \n", mndims);
            fprintf(stdout, "Dimension IDs\n");
            for (i = 0; i < mndims; i++)fprintf(stdout, "[%d]", mdimids[i]);
            fprintf(stdout, "\n");
        }

//--------------------------------------------------------------------------      
// Create the Variable
        //                  group id, name, datatype, n_dim, dim_ids, new variable id
        if ((err = nc_def_var(ncgrpid, name, xtype, mndims, mdimids, &ncvarid)) != NC_NOERR) {
            if (kw->verbose) fprintf(stderr, "Unable to Create the Data Variable %s\n", name);
            break;
        }

        if (kw->debug) fprintf(stdout, "Variable ID %d \n", ncvarid);

//--------------------------------------------------------------------------      	 	  
// Get a List of the UNLIMITED Dimensions in scope of this group

        if ((err = nc_inq_unlimdims(ncgrpid, &nunlimdimids, unlimdimids)) != NC_NOERR) {
            if (kw->verbose) fprintf(stderr, "Unable to Inquire on Unlimited Dimension IDs %s\n", name);
            break;
        }

        if (kw->debug) {
            fprintf(stdout, "Number of Unlimited Dimensions %d\n", nunlimdimids);
            fprintf(stdout, "Unlimited Dimension IDs\n");
            for (i = 0; i < nunlimdimids; i++)fprintf(stdout, "[%d]", unlimdimids[i]);
            fprintf(stdout, "\n");
        }


//--------------------------------------------------------------------------      	 	  	 
// Shape (Reversed in C from IDL so data(a,b,c) is data[c][b][a])

        if (kw->debug)fprintf(stdout, "Testing variable %s Dimension Lengths are Consistent \n", name);

// Allocate shape array for use only if one of the dimensions is UNLIMITED

      extents = (size_t *)malloc(mndims*sizeof(size_t));
	 	    
// Check all Array dimension Lengths are consistent (Ignore UNLIMITED dimensions)

        for (i = 0; i < mndims; i++) {

            if ((err = nc_inq_dimlen(ncgrpid, mdimids[i], &dimlength)) != NC_NOERR) {
                if (kw->verbose) fprintf(stderr, "Unable to obtain Length of Dimension %s \n", name);
                break;
            }

            extents[i] = dimlength;        // Current Length

// Is this dimension UNLIMITED?

            isUnlimited = 0;

            for (j = 0; j < nunlimdimids; j++) {
                if (mdimids[i] == unlimdimids[j]) {
                    isUnlimited = 1;
                    break;
                }
            }

            if (kw->debug) {
                fprintf(stdout, "Dimension %d has length %d\n", mdimids[i], (int) dimlength);
            }

            if (isUnlimited) {
                unlimitedCount++;
                // Capture the current size of each UNLIMITED dimension
		if (rank > 0) {
		  extents[i] = shape[rank - 1 -i];    
		} else {
		  extents[i] = shape[0];
		}
                if (kw->debug) {
                    fprintf(stdout, "extents[%d] = %d \n", i, (int) extents[i]);
                }
            }
	    else {
            //Check dimension length matches input data dimension length
	      if (rank > 0) {
                if (dimlength != shape[rank - 1 - i]) {
                   lerr = -1;
                   if (kw->verbose) {
                      fprintf(stderr, "<< Error >> Inconsistent Dimension Lengths for Array Variable %s \n", name);
                      fprintf(stderr, "Dimension %d has length %d \n", mdimids[i], (int) dimlength);
                      fprintf(stderr, "Corresponding Array Dimension %d has length %d \n", i, (int) shape[rank - 1 - i]);
                      fprintf(stderr, "Variable (with name %s) will not be written to file\n", name);
                   }
                   break;

                   extents[i] = dimlength;
                }
	      } else {
	        if (dimlength != shape[0]){
	          lerr = -1;
	          if(kw->verbose){
		    fprintf(stderr,"<< Error >> Inconsistent Dimension Lengths for Array Variable %s \n", name);
		    fprintf(stderr,"Dimension %d has length %d \n", mdimids[i], (int) dimlength);
		    fprintf(stderr,"Corresponding Array Dimension %d has length %d \n", i, (int)shape[rank-1-i]);
		    fprintf(stderr,"Variable (with name %s) will not be written to file\n", name);
	          }	
	          break;      
	     
   	          extents[i] = dimlength;
	        }
             }
	    }
        }

        if (err != NC_NOERR || lerr != 0) break;

//--------------------------------------------------------------------------                  
// Chunking is always ON (Variables with UNLIMITED dimension cannot use Contiguous chunking)

        if (length > 1 && rank > 0) {

            chunking = (size_t*) malloc(rank * sizeof(size_t));
            for (i = 0; i < rank; i++) {
                if (kw->is_chunksize) {
                    chunking[i] = (size_t) kw->chunksize;    // ***** Needs to be an ARRAY!
                } else {
                    chunking[i] = (size_t) shape[rank - 1 - i];
                }
                if (kw->debug)fprintf(stdout, "Using Chunking[%d] = %d\n", i, (int) chunking[i]);
            }

            if (isUnlimited) {
                if ((err = nc_def_var_chunking(ncgrpid, ncvarid, NC_CHUNKED, chunking)) != NC_NOERR) {
                    if (kw->verbose) fprintf(stderr, "Unable to set the Chunking Properties of Variable %s \n", name);
                    break;
                }
            } else {
                if ((err = nc_def_var_chunking(ncgrpid, ncvarid, NC_CHUNKED, chunking)) != NC_NOERR) {
                    if (kw->verbose) fprintf(stderr, "Unable to set the Chunking Properties of Variable %s \n", name);
                    break;
                }
            }

            free((void*) chunking);
        }

//--------------------------------------------------------------------------                  
// Compression 

        if (length > 1 && rank > 0 && kw->is_compression && kw->compression > 0) {
            if (kw->debug)fprintf(stdout, "Using Compression level %d\n", (int) kw->compression);

            //if((err = nc_def_var_deflate(ncgrpid, ncvarid, NC_NOSHUFFLE, 1, (int)kw->compression)) != NC_NOERR){

            if ((err = nc_def_var_deflate(ncgrpid, ncvarid, NC_SHUFFLE, 1, (int) kw->compression)) != NC_NOERR) {
                if (kw->verbose) fprintf(stderr, "Unable to set the Compression Properties of Variable %s \n", name);
                break;
            }
        }

//--------------------------------------------------------------------------      	 	  	 
// Write the Data Variable

        if (kw->debug)fprintf(stdout, "Writing Data to the Data Variable %s \n", name);

        if (rank == 0 && length == 1) {        // Scalar Data Item
            size_t start[1] = { 0 };
            size_t count[1] = { 1 };

            if (kw->debug)fprintf(stdout, "Data Variable %s is a Scalar\n", name);

            switch (xtype) {
                case NC_FLOAT: {
                    float scalar = (float) IDL_DoubleScalar(argv[0]);
                    if (unlimitedCount == 0)
                        err = nc_put_var_float(ncgrpid, ncvarid, &scalar);        // Fixed Length Dimension
                    else
                        err = nc_put_vara_float(ncgrpid, ncvarid, start, count, &scalar);
                    break;
                }
                case NC_DOUBLE: {
                    double scalar = (double) IDL_DoubleScalar(argv[0]);
                    if (unlimitedCount == 0)
                        err = nc_put_var_double(ncgrpid, ncvarid, &scalar);
                    else
                        err = nc_put_vara_double(ncgrpid, ncvarid, start, count, &scalar);
                    break;
                }
                case NC_INT64: {
                    long long scalar = (long long) IDL_Long64Scalar(argv[0]);
                    if (unlimitedCount == 0)
                        err = nc_put_var_longlong(ncgrpid, ncvarid, &scalar);
                    else
                        err = nc_put_vara_longlong(ncgrpid, ncvarid, start, count, &scalar);
                    break;
                }
                case NC_INT: {
                    int scalar = (int) IDL_LongScalar(argv[0]);
                    if (unlimitedCount == 0)
                        err = nc_put_var_int(ncgrpid, ncvarid, &scalar);
                    else
                        err = nc_put_vara_int(ncgrpid, ncvarid, start, count, &scalar);
                    break;
                }
                case NC_SHORT: {
                    short scalar = (short) IDL_LongScalar(argv[0]);
                    if (unlimitedCount == 0)
                        err = nc_put_var_short(ncgrpid, ncvarid, &scalar);
                    else
                        err = nc_put_vara_short(ncgrpid, ncvarid, start, count, &scalar);
                    break;
                }
                case NC_BYTE: {
                    signed char scalar = (signed char) IDL_LongScalar(argv[0]);
                    if (unlimitedCount == 0)
                        err = nc_put_var_schar(ncgrpid, ncvarid, &scalar);
                    else
                        err = nc_put_vara_schar(ncgrpid, ncvarid, start, count, &scalar);
                    break;
                }
                case NC_UINT64: {
                    unsigned long long scalar = (unsigned long long) IDL_ULong64Scalar(argv[0]);
                    if (unlimitedCount == 0)
                        err = nc_put_var_ulonglong(ncgrpid, ncvarid, &scalar);
                    else
                        err = nc_put_vara_ulonglong(ncgrpid, ncvarid, start, count, &scalar);
                    break;
                }
                case NC_UINT: {
                    unsigned int scalar = (unsigned int) IDL_ULongScalar(argv[0]);
                    if (unlimitedCount == 0)
                        err = nc_put_var_uint(ncgrpid, ncvarid, &scalar);
                    else
                        err = nc_put_vara_uint(ncgrpid, ncvarid, start, count, &scalar);
                    break;
                }
                case NC_USHORT: {
                    unsigned short scalar = (unsigned short) IDL_ULongScalar(argv[0]);
                    if (unlimitedCount == 0)
                        err = nc_put_var_ushort(ncgrpid, ncvarid, &scalar);
                    else
                        err = nc_put_vara_ushort(ncgrpid, ncvarid, start, count, &scalar);
                    break;
                }
                default:
                    if (xtype == ctype) {
                        IDL_COMPLEX data = argv[0]->value.cmp;    // See definition of IDL_VPTR & IDL_ALLTYPES
                        COMPLEX scalar;
                        scalar.real = data.r;
                        scalar.imaginary = data.i;
                        if (unlimitedCount == 0)
                            err = nc_put_var(ncgrpid, ncvarid, (void*) &scalar);
                        else
                            err = nc_put_vara(ncgrpid, ncvarid, start, count, &scalar);
                    } else {
                        if (xtype == dctype) {
                            IDL_DCOMPLEX data = argv[0]->value.dcmp;
                            DCOMPLEX scalar;
                            scalar.real = data.r;
                            scalar.imaginary = data.i;
                            if (unlimitedCount == 0)
                                err = nc_put_var(ncgrpid, ncvarid, (void*) &scalar);
                            else
                                err = nc_put_vara(ncgrpid, ncvarid, start, count, &scalar);
                        } else {
                            lerr = -1;
                            if (kw->verbose) fprintf(stdout, "Unknown Variable [%s] type \n", name);
                            break;
                        }
                    }
                    break;
            }

        } else {
            size_t* start = (size_t*) malloc(sizeof(size_t) * rank);
            for (i = 0; i < rank; i++) start[i] = 0;

            if (kw->debug) {
                fprintf(stdout, "Data Variable %s is an Array\n", name);
                fprintf(stdout, "Length %d\n", length);
                fprintf(stdout, "Rank: %d\n", rank);
                fprintf(stdout, "Shape:");
                for (i = 0; i < rank; i++) fprintf(stdout, "[%d]", (int)shape[i]);
                fprintf(stdout, "\n");
                fprintf(stdout, "Starting Indices:");
                for (i = 0; i < rank; i++) fprintf(stdout, "[%d]", (int)start[i]);
                fprintf(stdout, "\n");
            }

            switch (xtype) {
                case NC_FLOAT:
                    if (kw->debug)fprintf(stdout, "Data Variable %s is a Float Array\n", name);
                    err = nc_put_vara_float(ncgrpid, ncvarid, start, extents, (float*) data);
                    break;
                case NC_DOUBLE:
                    if (kw->debug)fprintf(stdout, "Data Variable %s is a Double Array\n", name);
                    err = nc_put_vara_double(ncgrpid, ncvarid, start, extents, (double*) data);
                    break;
                case NC_INT64:
                    if (kw->debug)fprintf(stdout, "Data Variable %s is an long long Array\n", name);
                    err = nc_put_vara_longlong(ncgrpid, ncvarid, start, extents, (long long*) data);
                    break;
                case NC_INT:
                    if (kw->debug)fprintf(stdout, "Data Variable %s is an int Array\n", name);
                    err = nc_put_vara_int(ncgrpid, ncvarid, start, extents, (int*) data);
                    break;
                case NC_SHORT:
                    if (kw->debug)fprintf(stdout, "Data Variable %s is a short Array\n", name);
                    err = nc_put_vara_short(ncgrpid, ncvarid, start, extents, (short*) data);
                    break;
                case NC_BYTE:
                    if (kw->debug)fprintf(stdout, "Data Variable %s is a byte Array\n", name);
                    err = nc_put_vara_schar(ncgrpid, ncvarid, start, extents, (signed char*) data);
                    break;
                case NC_UINT64:
                    if (kw->debug)fprintf(stdout, "Data Variable %s is an unsigned long long Array\n", name);
                    err = nc_put_vara_ulonglong(ncgrpid, ncvarid, start, extents,
                                                (unsigned long long*) data);
                    break;
                case NC_UINT:
                    if (kw->debug)fprintf(stdout, "Data Variable %s is an unsigned int Array\n", name);
                    err = nc_put_vara_uint(ncgrpid, ncvarid, start, extents, (unsigned int*) data);
                    break;
                case NC_USHORT:
                    if (kw->debug)fprintf(stdout, "Data Variable %s is an unsigned short Array\n", name);
                    err = nc_put_vara_ushort(ncgrpid, ncvarid, start, extents,
                                             (unsigned short*) data);
                    break;
                default:
                    if (xtype == ctype || xtype == dctype) {
                        err = nc_put_vara(ncgrpid, ncvarid, start, extents, (void*) data);
                        break;
                    }
            }

            free((void*) start);
        }

        if (err != NC_NOERR) {
            fprintf(stderr, "Unable to Write Data to Data Variable %s. << Error >>  %d\n", name, (int) err);
            break;
        } else {
            fprintf(stdout, "Variable \"%s\" was written to file\n", name);
        }

//--------------------------------------------------------------------------      
// Write extents attribute if any dimension was UNLIMITED  

        if (unlimitedCount > 0) {
            if (kw->debug) {
                fprintf(stdout, "Writing Extent Attribute of variable %s\n", name);
                for (i = 0; i < mndims; i++)fprintf(stdout, "[%d]=%d\n", i, (unsigned int) extents[i]);
            }
	    unsigned int iextents[NC_MAX_DIMS];
	    for (i = 0; i < mndims; i++) {
	      iextents[i] = (unsigned int)extents[i];
	    }
            if ((err = nc_put_att_uint(ncgrpid, ncvarid, "extent", NC_UINT, mndims, iextents)) != NC_NOERR) {
                if (kw->verbose) fprintf(stderr, "Unable to Write the Extent Attribute of variable: %s\n", name);
                break;
            }
        }

//--------------------------------------------------------------------------      
// Write SCALE and OFFSET attributes  

        if (kw->is_scale) {
            float scale = (float) kw->scale;
            if ((err = nc_put_att_float(ncgrpid, ncvarid, "scale", NC_FLOAT, 1, &scale)) != NC_NOERR) {
                if (kw->verbose) fprintf(stderr, "Unable to Write the Scale Attribute of variable: %s\n", name);
                break;
            }
        }

        if (kw->is_offset) {
            float offset = (float) kw->offset;
            if ((err = nc_put_att_float(ncgrpid, ncvarid, "offset", NC_FLOAT, 1, &offset)) != NC_NOERR) {
                if (kw->verbose) fprintf(stderr, "Unable to Write the Offset Attribute of variable: %s\n", name);
                break;
            }
        }

//--------------------------------------------------------------------------      	 
// Write Standard Variable Attributes

        if (kw->is_label) {
            label = IDL_STRING_STR(&kw->label);
            if ((err = nc_put_att_text(ncgrpid, ncvarid, "label", strlen(label), label)) != NC_NOERR) {
                if (kw->verbose) fprintf(stderr, "Unable to Write the Label Attribute of variable: %s\n", name);
                break;
            }
        }
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
// Write Device Association attributes: DEVICE & CHANNEL  

        if (kw->is_device) {
            int devgrp = 0, devgrp0 = 0;

            device = IDL_STRING_STR(&kw->device);

            if ((err = nc_inq_grp_ncid(ncfileid, "devices", &devgrp0)) !=
                NC_NOERR) {            // Check the Devices group exists
                if (kw->verbose)
                    fprintf(stderr, "The /devices group is not defined. Cannot have a sub group %s\n", device);
                break;
            }

            if ((err = nc_inq_grp_ncid(devgrp0, device, &devgrp)) != NC_NOERR) {            // Check the Device exists
                if (kw->verbose) {
                    fprintf(stderr, "The specified Device %s associated with Variable %s is not defined.\n",
                            device, name);
                }
                break;
            }

            if ((err = nc_put_att_text(ncgrpid, ncvarid, "device", strlen(device), device)) != NC_NOERR) {
                if (kw->verbose) fprintf(stderr, "Unable to Write the Device Attribute of variable: %s\n", name);
                break;
            }
        }

        if (kw->is_channels) {                // Ambiguous keyword if channel & channnels are both used
            if (kw->is_device) {
                short channel = (short) kw->channels;
                if ((err = nc_put_att_short(ncgrpid, ncvarid, "channel", NC_SHORT, 1, &channel)) != NC_NOERR) {
                    if (kw->verbose) fprintf(stderr, "Unable to Write the Channel Attribute of variable: %s\n", name);
                    break;
                }
            } else {
                lerr = -1;
                if (kw->verbose)
                    fprintf(stderr, "The Channel Number %d of Variable %s needs the Device to be Identified. "
                            "Please specifiy a defined Device.\n", (int) kw->channels, name);
                break;
            }
        }

//--------------------------------------------------------------------------           
// If there is a named error variable then check it exists in this group and it has the same shape.

        if (kw->is_errors) {
            int ncvarerrid, lvarerr, ndims;
            int* dimids;
            char* errors = IDL_STRING_STR(&kw->errors);

            if ((err = nc_inq_varid(ncgrpid, errors, &ncvarerrid)) != NC_NOERR) {
                if (kw->verbose) fprintf(stderr, "The variable %s Error variable %s does not exist.\n", name, errors);
                break;
            }

            if ((err = nc_inq_varndims(ncgrpid, ncvarerrid, &ndims)) != NC_NOERR) {
                if (kw->verbose)
                    fprintf(stderr, "Unable to Query the number of Dimensions of the Variable %s Error Variable %s\n",
                            name, errors);
                break;
            }

	    if ((ndims != rank && rank > 0) || (rank == 0 && ndims != 1)) {
                  lerr = -1;
                  if (kw->verbose)
                      fprintf(stderr, "The Rank of the Error Variable %s is Inconsistent with the Variable %s.\n", errors,
                              name);
                  break;
          
	    }

            dimids = (int*) malloc(ndims * sizeof(int));

            if ((err = nc_inq_vardimid(ncgrpid, ncvarerrid, dimids)) != NC_NOERR) {
                if (kw->verbose) fprintf(stderr, "Unable to Query the Dimension ID of the Error Variable %s\n", errors);
                break;
            }

	    if (rank > 0){
	      for (i = 0; i < rank; i++) {

                if ((err = nc_inq_dimlen(ncgrpid, dimids[i], (size_t*) &lvarerr)) != NC_NOERR) {
                    if (kw->verbose)
                        fprintf(stderr, "Unable to Query the Dimension Length of the Error Variable %s\n", errors);
                    break;
                }

                if (shape[rank - 1 - i] != lvarerr) {
                    lerr = -1;
                    if (kw->verbose)
                        fprintf(stderr,
                                "The Length [%d] of the Error Variable %s is Inconsistent with the variable Length %s [%d].\n",
                                lvarerr, errors, name, (int) shape[rank - 1 - i]);
                    break;
                }
             }
	    } else {
              if ((err = nc_inq_dimlen(ncgrpid, dimids[0], (size_t*) &lvarerr)) != NC_NOERR) {
		if (kw->verbose)
		  fprintf(stderr, "Unable to Query the Dimension Length of the Error Variable %s\n", errors);
                break;
	      }

              if (shape[0] != lvarerr){
                lerr = -1;
                if (kw->verbose)
		  fprintf(stderr,
			  "The Length [%d] of the Error Variable %s is Inconsistent with the variable Length %s [%d].\n",
			  lvarerr, errors, name, (int) shape[0]);
		break;
	      }
	    }

            free((void*) dimids);

            if (err != NC_NOERR || lerr != 0) break;

            if ((err = nc_put_att_text(ncgrpid, ncvarid, "errors", strlen(errors), errors)) != NC_NOERR) {
                if (kw->verbose) fprintf(stderr, "Unable to Write the Errors Attribute of Variable: %s\n", name);
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

//--------------------------------------------------------------------------      
// Cleanup Keywords 

    if (work != NULL)free((void*) work);

    return err;
} 
