// putOpenClose.c
//
// Control Creating or Opening for Update netCDF4 files
//
// Change Control
//
// 30Jan2009 DGMuir	Original Version
// 19May2009 DGMuir	Added default value for Conventions
//			Changed from 'open' to 'create' 
//-------------------------------------------------------------------------------------------------------

#include "putdata.h"		// IDL API Header

#include <stdlib.h>

int opennetcdf(int nparams, IDL_VPTR argv[], KW_RESULT* kw) {

    int i, lerr, err;
    int exp_number, pass, status, version;
    char* conventions, * code, * date, * filename, * format, * directory, * time, * xml, * path = NULL, * class, * title, * comment;
    char typename[NC_MAX_NAME + 1];

//---------------------------------------------------------------------------      
// Debug Dump of Keywords

    if (kw->debug) printKW(stdout, *kw);

//--------------------------------------------------------------------------      
// Create an Error Trap

    lerr = 0;    // Local Error
    err = 0;    // NC Error

    do {

//-------------------------------------------------------------------------	      
// Check File Format Specified 

        if (kw->is_format) {
            format = IDL_STRING_STR(&kw->format);
            if (strcasecmp(format, "netCDF4") != 0) {
                lerr = -1;
                if (kw->verbose)
                    fprintf(stderr, "Only netCDF4 File Formats are Implemented via this API - please rectify.\n");
                break;
            }
        }

//--------------------------------------------------------------------------      
// Open the File: Create or Update

        if (nparams == 1 && argv[0]->type == IDL_TYP_STRING) {
            filename = IDL_STRING_STR(&(argv[0]->value.str));
        } else {
            if (kw->verbose) fprintf(stderr, "The Parameter must be a String passing the Filename\n");
            lerr = -1;
            break;
        }

        if (kw->debug) fprintf(stdout, "The Filename is %s\n", filename);


        if (kw->is_directory) {
            directory = IDL_STRING_STR(&kw->directory);
            path = malloc(sizeof(char) * (strlen(directory) + strlen(filename) + 5));
            sprintf(path, "%s/%s", directory, filename);
        } else {
            path = malloc(sizeof(char) * (strlen(filename) + 3));

            if (strstr(filename, "/") == NULL)
                sprintf(path, "./%s", filename);        // Local directory
            else
                strcpy(path, filename);            // Contains directory
        }

        if (kw->create) {
            if ((err = nc_create(path, NC_CLOBBER | NC_NETCDF4, &ncfileid)) != NC_NOERR) {
                if (kw->verbose) fprintf(stderr, "Unable to Create the requested netCDF4 File: %s\n", path);
                break;
            }
            if (kw->debug) fprintf(stdout, "Created the requested netCDF4 File: %d\n", ncfileid);
        }

        if (kw->update) {
            if ((err = nc_open(path, NC_WRITE, &ncfileid)) != NC_NOERR) {
                if (kw->verbose) fprintf(stderr, "Unable to Open (for Update) the requested netCDF4 File: %s\n", path);
                break;
            }
            if (kw->debug) fprintf(stdout, "Opened the requested netCDF4 File for Update: %d\n", ncfileid);
        }

        ncfilecount++;        // Save in List next ID issued
        ncfileids = (int*) realloc((void*) ncfileids, ncfilecount * sizeof(int));
        complianceSet = (unsigned int*) realloc((void*) complianceSet, ncfilecount * sizeof(unsigned int));
        ncfileids[ncfilecount - 1] = ncfileid;
        complianceSet[ncfilecount - 1] = 0;
        compliance = 0;

//--------------------------------------------------------------------------     
// Name and Version of File Generator Code

        if (kw->create) {
            char* text = NULL;
            int ltext = 23 + strlen(__DATE__);
            text = (char*) malloc(ltext * sizeof(char));
            sprintf(text, "MAST IDL/DLM putData: %s", __DATE__);
            if ((err = nc_put_att_text(ncfileid, NC_GLOBAL, "generator", ltext, text)) != NC_NOERR) {
                if (kw->verbose) fprintf(stderr, "Warning: Unable to Write the File Generator Attribute: %s\n", text);
            }
            free((void*) text);
        }

//--------------------------------------------------------------------------     
// Current Compliance value

        if (kw->update) {
            if ((err = nc_get_att_uint(ncfileid, NC_GLOBAL, "compliance", &compliance)) != NC_NOERR) {
                compliance = 0;
            } else {
                complianceSet[ncfilecount - 1] = compliance;
            }
        }

        if (kw->debug && ncfileids != NULL)
            for (i = 0; i < ncfilecount; i++)
                fprintf(stdout, "A[%d] %d  %d\n", i, ncfileids[i], complianceSet[i]);

//--------------------------------------------------------------------------     
// Specify user Defined types: COMPLEX and DCOMPLEX 

        if (kw->update) {        // Must have been defined previously when the file was created
            int ntypes = 0;
            int* typeids = NULL;
            if (kw->debug) fprintf(stdout, "Listing Defined Data Types\n");
            if ((err = nc_inq_typeids(ncfileid, &ntypes, NULL)) != NC_NOERR || ntypes == 0) {
                if (kw->verbose) fprintf(stderr, "Unable to Quantify Data types\n");
                break;
            }
            typeids = (int*) malloc(ntypes * sizeof(int));
            if ((err = nc_inq_typeids(ncfileid, &ntypes, typeids)) != NC_NOERR) {
                if (kw->verbose) fprintf(stderr, "Unable to List Data types\n");
                break;
            }
            for (i = 0; i < ntypes; i++) {
                if ((err = nc_inq_compound_name(ncfileid, (nc_type) typeids[i], typename)) != NC_NOERR) {
                    if (kw->verbose) fprintf(stderr, "Unable to List Data types\n");
                    break;
                }
                if (kw->debug) fprintf(stdout, "Data Type %d Name: %s\n", typeids[i], typename);
                if (STR_EQUALS(typename, "complex")) ctype = typeids[i];
                if (STR_EQUALS(typename, "dcomplex")) dctype = typeids[i];
            }
            if (typeids != NULL) free((void*) typeids);
            if (err != NC_NOERR) break;

        } else {

// Single Precision Complex

            if ((err = nc_def_compound(ncfileid, (size_t) sizeof(COMPLEX), "complex", &ctype)) != NC_NOERR) {
                if (kw->verbose) fprintf(stderr, "Unable to Define the User Defined Complex type\n");
                break;
            }
            if ((err = nc_insert_compound(ncfileid, ctype, "real", (size_t) 0, NC_FLOAT)) != NC_NOERR) {
                if (kw->verbose) fprintf(stderr, "Unable to Insert a data type into the User Defined Complex type\n");
                break;
            }
            if ((err = nc_insert_compound(ncfileid, ctype, "imaginary", (size_t) sizeof(float), NC_FLOAT)) !=
                NC_NOERR) {
                if (kw->verbose) fprintf(stderr, "Unable to Insert a data type into the User Defined Complex type\n");
                break;
            }

// Double Precision Complex

            if ((err = nc_def_compound(ncfileid, (size_t) sizeof(DCOMPLEX), "dcomplex", &dctype)) != NC_NOERR) {
                if (kw->verbose) fprintf(stderr, "Unable to Define the User Defined Double Complex type\n");
                break;
            }
            if ((err = nc_insert_compound(ncfileid, dctype, "real", (size_t) 0, NC_DOUBLE)) != NC_NOERR) {
                if (kw->verbose) fprintf(stderr, "Unable to Insert a data type into the User Defined Complex type\n");
                break;
            }
            if ((err = nc_insert_compound(ncfileid, dctype, "imaginary", (size_t) sizeof(double), NC_DOUBLE)) !=
                NC_NOERR) {
                if (kw->verbose)
                    fprintf(stderr, "Unable to Insert a data type into the User Defined Double Complex type\n");
                break;
            }
        }

//--------------------------------------------------------------------------     
// Write Root Group Attributes: Required and Optional  

        if (kw->is_conventions) {
            conventions = IDL_STRING_STR(&kw->conventions);
            if ((err = nc_put_att_text(ncfileid, NC_GLOBAL, "Conventions", strlen(conventions), conventions)) !=
                NC_NOERR) {
                if (kw->verbose)
                    fprintf(stderr, "Unable to Write the Conventions Root Group Attribute: %s\n", conventions);
                break;
            }
            if (!strncasecmp(conventions, CREATE_CONVENTIONS_TEST, strlen(CREATE_CONVENTIONS_TEST)))
                compliance = compliance | CREATE_CONVENTIONS;
        } else {
            if (kw->create) {
                lerr = -1;
                if (kw->verbose) fprintf(stderr, "No Conventions Standard has been specified - please rectify.\n");
                break;
            }
        }

        if (kw->is_class) {
            class = IDL_STRING_STR(&kw->class);
            if (!(STR_IEQUALS(class, "Raw Data") || STR_IEQUALS(class, "Analysed data") ||
                  STR_IEQUALS(class, "Modelled Data"))) {
                lerr = -1;
                if (kw->verbose)
                    fprintf(stderr,
                            "The File Data Class must be one of: Raw, Analysed or Modelled - please rectify.\n");
                break;
            }
            if ((err = nc_put_att_text(ncfileid, NC_GLOBAL, "class", strlen(class), class)) != NC_NOERR) {
                if (kw->verbose) fprintf(stderr, "Unable to Write the Class Root Group Attribute: %s\n", class);
                break;
            }
            compliance = compliance | CREATE_CLASS;
        } else {
            if (kw->create) {
                lerr = -1;
                if (kw->verbose) fprintf(stderr, "No File Data Class has been specified - please rectify.\n");
                break;
            }
        }

        if (kw->is_title) {
            title = IDL_STRING_STR(&kw->title);
            if ((err = nc_put_att_text(ncfileid, NC_GLOBAL, "title", strlen(title), title)) != NC_NOERR) {
                if (kw->verbose) fprintf(stderr, "Unable to Write the Title Root Group Attribute: %s\n", title);
                break;
            }
        } else {
            if (kw->create) {
                lerr = -1;
                if (kw->verbose) fprintf(stderr, "No File Title has been specified - please rectify.\n");
                break;
            }
        }

        if (kw->is_date) {
            date = IDL_STRING_STR(&kw->date);
            if ((err = nc_put_att_text(ncfileid, NC_GLOBAL, "date", strlen(date), date)) != NC_NOERR) {
                if (kw->verbose) fprintf(stderr, "Unable to Write the Date Root Group Attribute: %s\n", date);
                break;
            }
        } else {
            if (kw->create) {
                lerr = -1;
                if (kw->verbose) fprintf(stderr, "No Date has been specified - please rectify.\n");
                break;
            }
        }

        if (kw->is_time) {
            time = IDL_STRING_STR(&kw->time);
            if ((err = nc_put_att_text(ncfileid, NC_GLOBAL, "time", strlen(time), time)) != NC_NOERR) {
                if (kw->verbose) fprintf(stderr, "Unable to Write the Time Root Group Attribute: %s\n", time);
                break;
            }
        } else {
            if (kw->create) {
                lerr = -1;
                if (kw->verbose) fprintf(stderr, "No Time has been specified - please rectify.\n");
                break;
            }
        }

        if (kw->is_exp_number) {
            exp_number = kw->exp_number;
            if ((err = nc_put_att_int(ncfileid, NC_GLOBAL, "shot", NC_INT, 1, &exp_number)) != NC_NOERR) {
                if (kw->verbose)
                    fprintf(stderr, "Unable to Write the Shot Number Root Group Attribute: %d\n", exp_number);
                break;
            }
            compliance = compliance | CREATE_SHOT;
        } else {
            if (kw->create && (STR_IEQUALS(class, "Raw Data") || STR_IEQUALS(class, "Analysed Data"))) {
                lerr = -1;
                if (kw->verbose)
                    fprintf(stderr, "A Shot or Experiment Number must be specified for New Raw or "
                            "Analysed Data Files - please rectify.\n");
                break;
            } else
                compliance = compliance | CREATE_SHOT;
        }

        if (kw->is_pass) {
            pass = kw->pass;
            if ((err = nc_put_att_int(ncfileid, NC_GLOBAL, "pass", NC_INT, 1, &pass)) != NC_NOERR) {
                if (kw->verbose) fprintf(stderr, "Unable to Write the Pass Number Root Group Attribute: %d\n", pass);
                break;
            }
            compliance = compliance | CREATE_PASS;
        } else {
            if (kw->create && STR_IEQUALS(class, "Analysed Data")) {
                lerr = -1;
                if (kw->verbose)
                    fprintf(stderr, "A Pass Number must be specified for New Analysed Data Files - please rectify.\n");
                break;
            } else
                compliance = compliance | CREATE_PASS;
        }

        if (kw->is_status) {
            status = kw->status;
            if ((err = nc_put_att_int(ncfileid, NC_GLOBAL, "status", NC_INT, 1, &status)) != NC_NOERR) {
                if (kw->verbose)
                    fprintf(stderr, "Unable to Write the File Status Number Root Group Attribute: %d\n", status);
                break;
            }
            compliance = compliance | CREATE_STATUS;
        } else {
            if (kw->create && STR_IEQUALS(class, "Analysed Data")) {
                lerr = -1;
                if (kw->verbose)
                    fprintf(stderr,
                            "A File Status Number must be specified for New Analysed Data Files - please rectify.\n");
                break;
            } else
                compliance = compliance | CREATE_STATUS;
        }

// Voluntary Attributes	 

        if (kw->is_comment) {
            comment = IDL_STRING_STR(&kw->comment);
            if ((err = nc_put_att_text(ncfileid, NC_GLOBAL, "comment", strlen(comment), comment)) != NC_NOERR) {
                if (kw->verbose) fprintf(stderr, "Unable to Write the Comment Root Group  Attribute: %s\n", comment);
                break;
            }
        }

// *****************************************
// Replaced by software attribute

        if (kw->is_code) {
            code = IDL_STRING_STR(&kw->code);
            if ((err = nc_put_att_text(ncfileid, NC_GLOBAL, "code", strlen(code), code)) != NC_NOERR) {
                if (kw->verbose) fprintf(stderr, "Unable to Write the Code Name Root Group Attribute: %s\n", code);
                break;
            }
        }

        if (kw->is_version) {
            version = kw->version;
            if ((err = nc_put_att_int(ncfileid, NC_GLOBAL, "version", NC_INT, 1, &version)) != NC_NOERR) {
                if (kw->verbose)
                    fprintf(stderr, "Unable to Write the Code Version Number Root Group Attribute: %d\n", version);
                break;
            }
        }
// ********************************************* 

        if (kw->is_xml) {
            xml = IDL_STRING_STR(&kw->xml);
            if ((err = nc_put_att_text(ncfileid, NC_GLOBAL, "xml", strlen(xml), xml)) != NC_NOERR) {
                if (kw->verbose) fprintf(stderr, "Unable to Write the XML Root Group Attribute: %s\n", xml);
                break;
            }
        }

        if (kw->debug)fprintf(stdout, "Compliance Test Result A: %d\n", compliance);

//--------------------------------------------------------------------------      
// End of Error Trap 

    } while (0);

    if (err != NC_NOERR && kw->verbose) fprintf(stderr, "Error Report: %s\n", nc_strerror(err));

    if (lerr != 0 && err == NC_NOERR) err = lerr;

//--------------------------------------------------------------------------      
// Cleanup Keywords 

    if (path != NULL) {
        free((void*) path);
        path = NULL;
    }

    return err;
}

int closenetcdf(KW_RESULT* kw) {
//
//-------------------------------------------------------------------------
// Change History:
//
// 30Jan2009 DGMuir	Original Version
//-------------------------------------------------------------------------	      

    int err = 0, i, target = -1;

//---------------------------------------------------------------------------      
// Debug Dump of Keywords

    if (kw->debug) {
        fprintf(stdout, "Closing the requested netCDF4 File: %d\n", ncfileid);
        printKW(stdout, *kw);
    }

//---------------------------------------------------------------------------      
// Write the Compliance Status Test Result

    if (!kw->nocompliance) {
        err = nc_put_att_uint(ncfileid, NC_GLOBAL, "compliance", NC_UINT, 1, &compliance);
    }

    if (kw->debug)fprintf(stdout, "Compliance Test Result B: %d\n", compliance);

//---------------------------------------------------------------------------      
// Close the File   

    if (ncfilecount == 0) {
        if (kw->verbose) fprintf(stderr, "Error: There are No Open Files to Close! \n");
        err = -1;
        return err;
    }

    if ((err = nc_close(ncfileid)) != NC_NOERR) {
        if (kw->verbose)
            fprintf(stderr, "Unable to Close the requested netCDF4 File. Error Report: %s\n", nc_strerror(err));
        return err;
    }


//---------------------------------------------------------------------------      
// Update the List of Open Files   

    for (i = 0; i < ncfilecount; i++) {
        if (ncfileids[i] == ncfileid) {                    // Identify target in list
            target = i;
            break;
        }
    }

    if (target > -1) {
        ncfilecount--;
        for (i = target; i < ncfilecount; i++) {
            ncfileids[i] = ncfileids[i + 1];                // Reduce the List
            complianceSet[i] = complianceSet[i + 1];
        }
        if (ncfilecount > 0) {
            ncfileid = ncfileids[ncfilecount - 1];                // pop last value
            compliance = complianceSet[ncfilecount - 1];
        } else {
            ncfileid = INT_MAX;
            compliance = 0;
        }
    }

    if (ncfilecount == 0) {
        if (ncfileids != NULL) {
            free((void*) ncfileids);                // Automatic Free after final close
            free((void*) complianceSet);
            ncfileids = NULL;
            complianceSet = NULL;
        }
        ncfileid = INT_MAX;
        compliance = 0;
    }

    if (kw->debug) {
        fprintf(stdout, "File Closed, %d remain open)\n", ncfilecount);
        if (ncfileids != NULL)
            for (i = 0; i < ncfilecount; i++)
                fprintf(stdout, "B[%d] %d   %d\n", i, ncfileids[i], complianceSet[i]);
    }

//---------------------------------------------------------------------------      
// Free DUNITS Resources

    if (ncfilecount == 0 && unitSystem != NULL) {
        ut_free_system(unitSystem);
        unitSystem = NULL;
    }

    return err;
}
