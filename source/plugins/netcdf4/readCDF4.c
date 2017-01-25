/*---------------------------------------------------------------
* IDAM Plugin data Reader to Access DATA from netCDF4 Files
*
* Input Arguments:	DATA_SOURCE data_source
*			SIGNAL_DESC signal_desc
*
* Returns:		readCDF		0 if read was successful
*					otherwise a Error Code is returned
*			DATA_BLOCK	Structure with Data from the IDA File
*
* Calls		freeDataBlock	to free Heap memory if an Error Occurs
*
* Notes: 	All memory required to hold data is allocated dynamically
*		in heap storage. Pointers to these areas of memory are held
*		by the passed DATA_BLOCK structure. Local memory allocations
*		are freed on exit. However, the blocks reserved for data are
*		not and MUST BE FREED by the calling routine.
*
*		Data can either be a variable or an attribute attached to a group or variable
*		For the latter, a dot operator is assumed.
*
*		scale and offset apply only to Raw Data class data of Rank 1
*
*		TRANSP data has coordinate dimensions that are of rank > 1: They are time dependent!
*
* ToDo:
*		1> Coordinates and Attributes can also be User Defined Data Structure Type
*		2> If the UDType is not local to the group, then enlarge the scope back up the tree to locate it.
*		3> If the variable or attribute is not found, then return the whole sub-tree.
*			3.1> List all user defined type within scope
*			3.2> List all dimensions and coordinates within scope
*			3.3> Walk the sub-tree and build the full compound structure.
*			3.4> Complex types: only enquire if required.
*---------------------------------------------------------------------------------------------------------------*/
#include "readCDF4.h"

#include <server/managePluginFiles.h>
#include <structures/struct.h>
#include <clientserver/initStructs.h>
#include <clientserver/TrimString.h>

#include "netCDF4.h"
#include "readCDFAtts.h"
#include "readCDF4SubTree.h"

#define NETCDF_ERROR_OPENING_FILE           200
#define NETCDF_ERROR_ALLOCATING_HEAP_1      201
#define NETCDF_ERROR_ALLOCATING_HEAP_2      202
#define NETCDF_ERROR_ALLOCATING_HEAP_3      203
#define NETCDF_ERROR_ALLOCATING_HEAP_4      204
#define NETCDF_ERROR_ALLOCATING_HEAP_5      205
#define NETCDF_ERROR_ALLOCATING_HEAP_6      206
#define NETCDF_ERROR_ALLOCATING_HEAP_7      207
#define NETCDF_ERROR_ALLOCATING_HEAP_8      208
#define NETCDF_ERROR_ALLOCATING_HEAP_9      209
#define NETCDF_ERROR_ALLOCATING_HEAP_10     210
#define NETCDF_ERROR_INQUIRING_VARIABLE_1   220
#define NETCDF_ERROR_INQUIRING_VARIABLE_2   221
#define NETCDF_ERROR_INQUIRING_VARIABLE_3   222
#define NETCDF_ERROR_INQUIRING_VARIABLE_4   223
#define NETCDF_ERROR_INQUIRING_VARIABLE_5   224
#define NETCDF_ERROR_INQUIRING_VARIABLE_6   225
#define NETCDF_ERROR_INQUIRING_VARIABLE_7   226
#define NETCDF_ERROR_INQUIRING_VARIABLE_8   227
#define NETCDF_ERROR_INQUIRING_VARIABLE_9   228
#define NETCDF_ERROR_INQUIRING_VARIABLE_10  229
#define NETCDF_ERROR_INQUIRING_VARIABLE_11  230
#define NETCDF_ERROR_INQUIRING_VARIABLE_12  231
#define NETCDF_ERROR_INQUIRING_VARIABLE_13  232
#define NETCDF_ERROR_INQUIRING_VARIABLE_14  233
#define NETCDF_ERROR_INQUIRING_DIM_1        240
#define NETCDF_ERROR_INQUIRING_DIM_2        241
#define NETCDF_ERROR_INQUIRING_DIM_3        242
#define NETCDF_ERROR_INQUIRING_DIM_4        243
#define NETCDF_ERROR_INQUIRING_DIM_5        244
#define NETCDF_ERROR_INQUIRING_ATT_1        250
#define NETCDF_ERROR_INQUIRING_ATT_2        251
#define NETCDF_ERROR_INQUIRING_ATT_3        252
#define NETCDF_ERROR_INQUIRING_ATT_4        253
#define NETCDF_ERROR_INQUIRING_ATT_5        254
#define NETCDF_ERROR_INQUIRING_ATT_6        255
#define NETCDF_ERROR_INQUIRING_ATT_7        256
#define NETCDF_ERROR_INQUIRING_ATT_8        257
#define NETCDF_ERROR_INQUIRING_ATT_9        258
#define NETCDF_ERROR_INQUIRING_ATT_10       259

#define TEST            0

#define NOCLASS_DATA    0
#define RAW_DATA        1
#define ANALYSED_DATA   2
#define MODELLED_DATA   3

#define COMPLIANCE_PASS 31

//nc_type ctype = NC_NAT;        // User defined Complex types
//nc_type dctype = NC_NAT;
int prior_fd = 0;            // Link the Complex types to the File.

/*
unsigned int readCDF4Properties()
{
    static unsigned int readProperty = 1, cdfProperties = 0;
    if (readProperty) {
        char* env = getenv("IDAM_CDFPROPERTIES");    // Assign behaviour via the server's environment
        if (env != NULL) cdfProperties = (unsigned int) atoi(env);
        readProperty = 0;
    }
    return cdfProperties;
}
*/

int readerCDF4(DATA_SOURCE* data_source, SIGNAL_DESC* signal_desc, REQUEST_BLOCK* request_block,
               DATA_BLOCK* data_block)
{
    char classtxt[STRING_LENGTH];
    char comment[STRING_LENGTH];

    int fd, err = 0, rc;
    int fusion_ver = 0;

    int i, ii, j, lstr, lname, rank, drank, varid, coordid, grpid, cgrpid, subtree, attid, error_n;
    int hierarchical;
    unsigned int fdcompliance = 0;

    char* cp = NULL;
    char* group, * work, * token;
    char* conventions = NULL;

    int format, compliance = 0, class, isCoordinate = 0, isIndex = 0;
    size_t attlen, dimlen;
    nc_type atype;

    int lnamemax = NC_MAX_NAME + 1;
    char variable[NC_MAX_NAME + 1];
    char dimname[NC_MAX_NAME + 1];
    char typename[NC_MAX_NAME + 1];

    int numgrps = 10;        // Initial assumed number of groups along hierarchy: grow as required
    int numgrp = 1;        // Always the Top level group
    int* grpids = NULL;
    GROUPLIST grouplist;
    GROUPLIST cgrouplist;
    HGROUPS hgroups;

    int* dimids = NULL;
    unsigned int* extent = NULL;    // Shape of the data array
    unsigned int* dextent = NULL;    // Dimension Lengths

    int unlimdimids[NC_MAX_DIMS];
    int nunlimdims, isUnlimited;

    int ndimatt[2];            // NC_STRING attribute shape

    USERDEFINEDTYPE* udt = NULL, * dudt = NULL;        // User defined structure type definition

    struct timeval tv_start0;    // Start time
    struct timeval tv_end0;    // Stop time

    gettimeofday(&tv_start0, NULL);

//----------------------------------------------------------------------
// Error Trap Loop

    err = 0;

    do {

//----------------------------------------------------------------------
// Is the netCDF File Already open for Reading? If Not then Open in READ ONLY mode
// The file handle fd is an integer - use the Integer specific API

        errno = 0;

        if ((fd = getOpenIdamPluginFileInt(&pluginFileList, data_source->path)) < 0) {
            if ((rc = nc_open((const char*) data_source->path, NC_NOWRITE, &fd)) != NC_NOERR) {
                err = NETCDF_ERROR_OPENING_FILE;
                if (errno != 0) addIdamError(&idamerrorstack, SYSTEMERRORTYPE, "readCDF", errno, "");
                addIdamError(&idamerrorstack, CODEERRORTYPE, "readCDF", err, (char*) nc_strerror(rc));
                idamLog(LOG_DEBUG, "netCDF: Error opening file - %s\n", nc_strerror(rc));
                break;
            }
            addIdamPluginFileInt(&pluginFileList, data_source->path, fd);        // Register the File Handle
        }

        idamLog(LOG_DEBUG, "netCDF filename %s\n", data_source->path);

//----------------------------------------------------------------------
// Register the API function needed to close the file: int nc_close(int ncid)

        static unsigned short isRegistered = 0;

        if (!isRegistered) {
            int (* close)(int);    // Function pointer
            close = &nc_close;
            registerIdamPluginFileClose(&pluginFileList, (void*) close);
            isRegistered = 1;
        }

//----------------------------------------------------------------------
// Test the File Format Version. Was the file written using hierarchical netCDF4 layout?
// If NC_FORMAT_CLASSIC or NC_FORMAT_64BIT then flat file (non-hierarchical) version assumed.

        hierarchical = 0;

        if ((rc = nc_inq_format(fd, &format)) == NC_NOERR) {
            hierarchical = (format == NC_FORMAT_NETCDF4) || (format == NC_FORMAT_NETCDF4_CLASSIC);
        }

        idamLog(LOG_DEBUG, "netCDF hierarchical organisation ? %d\n", hierarchical);

//----------------------------------------------------------------------
// FUDGE for netcdf-3 TRANSP data (This won't work if the source alias is unknown, e.g. when private file)

        //if(hierarchical && !strcmp(signal_desc->source_alias, "transp")) hierarchical = 0;

//----------------------------------------------------------------------
// Global Meta Data: What convention has been adopted? Data Class?

        compliance = 0;            // Compliance also means hierarchical file format
        class = NOCLASS_DATA;
        attlen = 0;

        if (hierarchical) {

// Check the compliance attribute is set (ignore for now as a work in progress)

            if (0 && (rc = nc_get_att_uint(fd, NC_GLOBAL, "compliance", &fdcompliance)) == NC_NOERR) {
                compliance = (fdcompliance == COMPLIANCE_PASS);
            }

// Conventions (Always with an upper case C)

            attlen = 0;
            if ((rc = nc_inq_attlen(fd, NC_GLOBAL, "Conventions", &attlen)) == NC_NOERR ||
                (rc = nc_inq_attlen(fd, NC_GLOBAL, "_Conventions", &attlen)) == NC_NOERR) {

                if ((err = nc_inq_atttype(fd, NC_GLOBAL, "Conventions", &atype)) != NC_NOERR &&
                    (err = nc_inq_atttype(fd, NC_GLOBAL, "_Conventions", &atype)) != NC_NOERR) {
                    addIdamError(&idamerrorstack, CODEERRORTYPE, "readCDF", err, (char*) nc_strerror(rc));
                    addIdamError(&idamerrorstack, CODEERRORTYPE, "readCDF", err,
                                 "Conventions attribute type not known!");
                    break;
                }

                if (atype == NC_STRING) {
                    if (attlen != 1) {        // Single string expected
                        err = 999;
                        addIdamError(&idamerrorstack, CODEERRORTYPE, "readCDF", err,
                                     "Multiple Conventions found when only one expected!");
                        break;
                    }
                    char** conv = (char**) malloc(sizeof(char*));
                    if ((err = nc_get_att_string(fd, NC_GLOBAL, "Conventions", conv)) != NC_NOERR &&
                        (err = nc_get_att_string(fd, NC_GLOBAL, "_Conventions", conv)) != NC_NOERR) {
                        addIdamError(&idamerrorstack, CODEERRORTYPE, "readCDF", err, (char*) nc_strerror(err));
                        break;
                    }
                    attlen = (int) strlen(conv[0]) + 1;
                    conventions = (char*) malloc(attlen * sizeof(char));
                    strcpy(conventions, conv[0]);
                    nc_free_string(1, conv);
                    free((void*) conv);
                } else {
                    conventions = (char*) malloc((attlen + 1) * sizeof(char));
                    conventions[0] = '\0';
                    if ((err = nc_get_att_text(fd, NC_GLOBAL, "Conventions", conventions)) != NC_NOERR &&
                        (err = nc_get_att_text(fd, NC_GLOBAL, "_Conventions", conventions)) != NC_NOERR) {
                        addIdamError(&idamerrorstack, CODEERRORTYPE, "readCDF", err, (char*) nc_strerror(err));
                        free((void*) conventions);
                        break;
                    }
                    conventions[attlen] = '\0';        // Ensure Null terminated
                }
                idamLog(LOG_DEBUG, "netCDF file Conventions?  %s\n", conventions);

                if (conventions[0] != '\0') {

                    if ((cp = strstr(conventions, "MAST-")) != NULL) {
                        if ((token = strstr(&cp[5], ".")) != NULL) {
                            token[0] = '\0';
                        }
                    }

                    if ((cp = strstr(conventions, "Fusion-")) != NULL) {
                        compliance = 1;
                        if ((token = strstr(&cp[7], ".")) != NULL) {
                            token[0] = '\0';
                            fusion_ver = atoi(&cp[7]);    // Need major part only
                        }
                    }
                }

                free((void*) conventions);
            }

            idamLog(LOG_DEBUG, "netCDF file compliance?  %d\n", compliance);

            if (compliance) {
                attlen = 0;
                char* classification;
                if ((rc = nc_inq_attlen(fd, NC_GLOBAL, "class", &attlen)) == NC_NOERR ||
                    (rc = nc_inq_attlen(fd, NC_GLOBAL, "_class", &attlen)) == NC_NOERR) {
                    if ((err = nc_inq_atttype(fd, NC_GLOBAL, "class", &atype)) != NC_NOERR &&
                        (err = nc_inq_atttype(fd, NC_GLOBAL, "_class", &atype)) != NC_NOERR) {
                        addIdamError(&idamerrorstack, CODEERRORTYPE, "readCDF", err, (char*) nc_strerror(err));
                        addIdamError(&idamerrorstack, CODEERRORTYPE, "readCDF", err, "class attribute type not known!");
                        break;
                    }

                    if (atype == NC_STRING) {
                        if (attlen != 1) {        // Single string expected
                            err = 999;
                            addIdamError(&idamerrorstack, CODEERRORTYPE, "readCDF", err,
                                         "Multiple classes found when only one expected!");
                            break;
                        }
                        char** class = (char**) malloc(sizeof(char*));
                        if ((err = nc_get_att_string(fd, NC_GLOBAL, "class", class)) != NC_NOERR &&
                            (err = nc_get_att_string(fd, NC_GLOBAL, "_class", class)) != NC_NOERR) {
                            addIdamError(&idamerrorstack, CODEERRORTYPE, "readCDF", err, (char*) nc_strerror(err));
                            break;
                        }
                        attlen = (int) strlen(class[0]) + 1;
                        classification = (char*) malloc(attlen * sizeof(char));
                        strcpy(classification, class[0]);
                        nc_free_string(1, class);
                        free((void*) class);
                    } else {
                        classification = (char*) malloc((attlen + 1) * sizeof(char));
                        classification[0] = '\0';
                        if ((err = nc_get_att_text(fd, NC_GLOBAL, "class", classification)) != NC_NOERR &&
                            (err = nc_get_att_text(fd, NC_GLOBAL, "_class", classification)) != NC_NOERR) {
                            addIdamError(&idamerrorstack, CODEERRORTYPE, "readCDF", err, (char*) nc_strerror(err));
                            free((void*) classification);
                            break;
                        }
                        classification[attlen] = '\0';        // Ensure Null terminated
                    }

                    if (!strcasecmp(classification, "raw data")) {
                        class = RAW_DATA;
                    } else {
                        if (!strcasecmp(classification, "analysed data")) {
                            class = ANALYSED_DATA;
                        } else {
                            if (!strcasecmp(classification, "modelled data")) {
                                class = MODELLED_DATA;
                            }
                        }
                    }

                    free((void*) classification);
                }
            }
        }

        idamLog(LOG_DEBUG, "netCDF file class?  %d\n", class);

//----------------------------------------------------------------------
// Complex Data Types (Done once per file if the Conventions are for FUSION and MAST)

        if (compliance && fusion_ver >= 1 && ((ctype == NC_NAT && dctype == NC_NAT) || prior_fd != fd)) {
            int ntypes = 0;
            nc_type* typeids = NULL;
            do {
                if ((err = nc_inq_typeids(fd, &ntypes, NULL)) != NC_NOERR || ntypes == 0) break;
                typeids = (int*) malloc(ntypes * sizeof(int));
                if ((err = nc_inq_typeids(fd, &ntypes, typeids)) != NC_NOERR) break;
                for (i = 0; i < ntypes; i++) {
                    //if ((err = nc_inq_compound_name(fd, (nc_type)typeids[i], typename)) != NC_NOERR) break;
                    typename[0] = '\0';
                    nc_inq_compound_name(fd, typeids[i], typename);        // ignore non-compound types
                    if (!strcmp(typename, "complex")) ctype = typeids[i];
                    if (!strcmp(typename, "dcomplex")) dctype = typeids[i];
                }
            } while (0);
            if (typeids != NULL) free((void*) typeids);
            if (err != NC_NOERR) {
                addIdamError(&idamerrorstack, CODEERRORTYPE, "readCDF", 999, (char*) nc_strerror(err));
                return err;
            }
            prior_fd = fd;
        }

//----------------------------------------------------------------------
// Test signal name for non unique compliant name: devices
// signal_alias must be the source alias => no entry in database found
// If found then replace with a truncated form

// ***** This assumes a 3 letter source_alias name prefix !!!!!

        if (compliance) {
            if (!strncmp(&signal_desc->signal_name[4], "/devices/", 9)) {        //   /xyc/devices/...
                strncpy(variable, &signal_desc->signal_name[1], 3);
                variable[3] = '\0';
                idamLog(LOG_DEBUG, "devices signal requested\n");
                idamLog(LOG_DEBUG, "source alias: [%s]\n", variable);
                idamLog(LOG_DEBUG, "source alias: [%s]\n", signal_desc->signal_alias);
                if (!strcmp(signal_desc->signal_alias, variable)) {
                    strcpy(variable, &signal_desc->signal_name[4]);
                    strcpy(signal_desc->signal_name, variable);
                    idamLog(LOG_DEBUG, "Not recorded in Database: Removing source alias prefix\n");
                    idamLog(LOG_DEBUG, "Target signal: %s\n", signal_desc->signal_name);
                }
            }
        }

//----------------------------------------------------------------------
// Get Group ID List - Group Hierarchy - from the top down to the dataset

        if ((lname = (int) strlen(signal_desc->signal_name) + 2) > lnamemax) {
            err = 999;
            addIdamError(&idamerrorstack, CODEERRORTYPE, "readCDF", err, "the Signal Name is too long for netCDF!");
            break;
        }

        strcpy(variable, signal_desc->signal_name);

        idamLog(LOG_DEBUG, "netCDF signal name?  %s\n", variable);

        if (hierarchical) {
            char* p = NULL;
            group = (char*) malloc(lname * sizeof(char));

            if (signal_desc->signal_name[0] == '/')
                strcpy(group, signal_desc->signal_name);        // Contains the Top Level Group identifier
            else {
                sprintf(group, "/%s", signal_desc->signal_name);    // Insert the Top Level Group identifier
            }

            if ((p = strrchr(group, (int) '/')) != NULL) {
                if (p != &group[0]) {                    // Variable is not attached to top level group
                    *p = '\0';                    // Split the String into group hierarchy and variable name
                    strcpy(variable, &p[1]);
                } else {
                    strcpy(variable, &p[1]);
                    p[1] = '\0';                    // Top Level Group
                }
            }

            grpids = (int*) malloc(sizeof(int) * numgrps);
            grpids[0] = fd;

            if (strcmp(group, "/") != 0) {                // Not required if Top Level Group

                work = (char*) malloc((strlen(group) + 1) * sizeof(char));
                strcpy(work, &group[1]);                // Skip the leading '/' character

                if ((token = strtok(work, "/")) != NULL) {        // Tokenise for 1 or more grouping levels
                    if ((err = getGroupId(grpids[numgrp - 1], token, &grpids[numgrp])) != NC_NOERR) {
                        err = NETCDF_ERROR_INQUIRING_VARIABLE_1;
                        addIdamError(&idamerrorstack, CODEERRORTYPE, "readCDF", err,
                                     "Unable to Locate a Hierarchical Group");
                        free((void*) work);
                        free((void*) group);
                        work = NULL;
                        group = NULL;
                        break;
                    }
                    numgrp++;

                    while ((token = strtok(NULL, "/")) != NULL) {
                        if ((err = getGroupId(grpids[numgrp - 1], token, &grpids[numgrp])) != NC_NOERR) {
                            err = NETCDF_ERROR_INQUIRING_VARIABLE_1;
                            addIdamError(&idamerrorstack, CODEERRORTYPE, "readCDF", err,
                                         "Unable to Locate a Hierarchical Group");
                            free((void*) work);
                            free((void*) group);
                            work = NULL;
                            group = NULL;
                            break;
                        }
                        numgrp++;

                        if (numgrp == numgrps) {
                            numgrps = numgrps + 10;
                            grpids = (int*) realloc((void*) grpids,
                                                    sizeof(int) * numgrps);    // Extend the Array of IDs
                        }
                    }
                    if (err != NC_NOERR) break;

                }
                free((void*) work);
                work = NULL;
            }
            free((void*) group);
            group = NULL;
            grpid = grpids[numgrp - 1];        // Lowest Group in Hierarchy

            grouplist.count = numgrp;
            grouplist.grpid = grpid;
            grouplist.grpids = grpids;

        } else {
            numgrp = 1;
            grpid = fd;                // Always the Top Level group
            grpids = (int*) malloc(sizeof(int));
            grpids[0] = fd;

            grouplist.count = 1;
            grouplist.grpid = fd;
            grouplist.grpids = grpids;

        }

//----------------------------------------------------------------------
// Does the variable name contain sub-setting instructions [start:stop:stride]

        varid = -1;
        cdfsubset.subsetCount = request_block->datasubset.subsetCount;

        if (cdfsubset.subsetCount > 0) {

            if (cdfsubset.subsetCount > NC_MAX_VAR_DIMS) {
                err = 999;
                addIdamError(&idamerrorstack, CODEERRORTYPE, "readCDF", err,
                             "Too many subset dimensions for netCDF: limit exceeded.");
                break;
            }

// Copy subset details to local structure

            {
                int i;
                for (i = 0; i < cdfsubset.subsetCount; i++) {
                    cdfsubset.subset[i] = request_block->datasubset.subset[i];
                    cdfsubset.start[i] = (size_t) request_block->datasubset.start[i];
                    cdfsubset.stop[i] = (size_t) request_block->datasubset.stop[i];
                    cdfsubset.count[i] = (size_t) request_block->datasubset.count[i];
                    cdfsubset.stride[i] = (ptrdiff_t) request_block->datasubset.stride[i];
                }
            }

// Does the subset operation remain within the signal name string: extract if so

            char* work = (char*) malloc((strlen(variable) + 1) * sizeof(char));
            strcpy(work, variable);
            char* p = strstr(work, request_block->subset);
            if (p != NULL) p[0] = '\0';            // Remove subset operations from variable name
            TrimString(work);

// Test the reduced Variable name matches a Group variable. If it does use the reduced name.

            if ((rc = nc_inq_varid(grpid, work, &varid)) == NC_NOERR) {
                strcpy(variable, work);
            } else {
                rc = NC_NOERR;        // Carry on with the original signal name ignoring any subset operations
                varid = -1;
                cdfsubset.subsetCount = 0;
            }
            free(work);
        }

//----------------------------------------------------------------------
// Get Variable ID attached to the final group (or Return the Attribute values)

        if (varid == -1 &&
            (rc = nc_inq_varid(grpid, variable, &varid)) != NC_NOERR) {    // If not found then irregular data item
            int rc2;
            int dimid;
            nc_type atttype;
            char* attname = NULL;

            idamLog(LOG_DEBUG, "netCDF: variable not found ... trying other options ...\n");

// Check it's not an unwritten Coordinate dataset (with the same name as the variable). If so then create an index array

            if ((rc = nc_inq_dimid(grpid, variable, &dimid)) == NC_NOERR) {                // Found!
// dgm size_t 64 bit bug 16Dec2011
                size_t data_n;
                if ((rc = nc_inq_dimlen(grpid, dimid, &data_n)) != NC_NOERR) {
                    err = 999;
                    addIdamError(&idamerrorstack, CODEERRORTYPE, "readCDF", err,
                                 "Unable to identify the length of a Dimension");
                    break;
                }
                data_block->data_n = (int) data_n;
                idamLog(LOG_DEBUG, "netCDF: unwritten Coordinate dataset found.\n");

                data_block->rank = 1;
                data_block->order = -1;
                data_block->data_type = TYPE_INT;

// Subset operation?

                if (cdfsubset.subsetCount > 1) {
                    err = 999;
                    addIdamError(&idamerrorstack, CODEERRORTYPE, "readCDF", err,
                                 "Cannot multi-dimension subset a Dimension Variable!");
                    break;
                }

// Adjust data array length and fill out missing data

                if (cdfsubset.subsetCount == 1 && cdfsubset.subset[0]) {    // Dimension variables are always rank 1
                    cdfsubset.rank = 1;
                    cdfsubset.dimids[0] = dimid;
                    if ((int) cdfsubset.stop[0] == -1) cdfsubset.stop[0] = (size_t) (data_block->data_n - 1);
                    if ((int) cdfsubset.count[0] == -1) {
                        cdfsubset.count[0] = cdfsubset.stop[0] - cdfsubset.start[0] + 1;
                        if ((int) cdfsubset.stride[0] > 1 && (int) cdfsubset.count[0] > 1) {
                            if (((int) cdfsubset.count[0] % (int) cdfsubset.stride[0]) > 0)
                                cdfsubset.count[0] = 1 + (int) cdfsubset.count[0] / (int) cdfsubset.stride[0];
                            else
                                cdfsubset.count[0] = (int) cdfsubset.count[0] / (int) cdfsubset.stride[0];
                        }
                    }
                    data_block->data_n = (int) cdfsubset.count[0];
                }

                if ((data_block->data = (char*) malloc(data_block->data_n * sizeof(int))) == NULL) {
                    err = NETCDF_ERROR_ALLOCATING_HEAP_1;
                    addIdamError(&idamerrorstack, CODEERRORTYPE, "readCDF", err, "Problem Allocating Data Heap Memory");
                    break;
                }

                readCDF4CreateIndex(data_block->data_n, data_block->data);

                if ((data_block->dims = (DIMS*) malloc(data_block->rank * sizeof(DIMS))) == NULL) {
                    err = NETCDF_ERROR_ALLOCATING_HEAP_1;
                    addIdamError(&idamerrorstack, CODEERRORTYPE, "readCDF", err,
                                 "Problem Allocating Dimension Heap Memory");
                    break;
                }

                initDimBlock(&data_block->dims[0]);
                data_block->dims[0].dim_n = data_block->data_n;
                data_block->dims[0].data_type = TYPE_INT;

                if ((data_block->dims[0].dim = (char*) malloc(data_block->data_n * sizeof(int))) == NULL) {
                    err = NETCDF_ERROR_ALLOCATING_HEAP_1;
                    addIdamError(&idamerrorstack, CODEERRORTYPE, "readCDF", err,
                                 "Problem Allocating Dimension Heap Memory");
                    break;
                }

                readCDF4CreateIndex(data_block->dims[0].dim_n, data_block->dims[0].dim);

                break;

            }

// Check it's not an attribute attached to a group (No native subsetting of attribute array data)

            if (cdfsubset.subsetCount == 0 &&
                (rc2 = nc_inq_atttype(grpid, NC_GLOBAL, variable, &atttype)) == NC_NOERR) {
                if ((rc2 = readCDF4AVar(grouplist, grpid, NC_GLOBAL, atttype, variable, &data_block->data_n, ndimatt,
                                        &data_block->data_type, &data_block->data, &udt)) != NC_NOERR) {
                    err = 999;
                    addIdamError(&idamerrorstack, CODEERRORTYPE, "readCDF", err,
                                 "Unable to read Group Level Attribute data");
                    break;
                }
                idamLog(LOG_DEBUG, "netCDF: attribute attached to a group found.\n");

                if (udt != NULL) {                // A User Defined Data Structure Type?
                    malloc_source = MALLOCSOURCENETCDF;
                    data_block->opaque_type = OPAQUE_TYPE_STRUCTURES;
                    data_block->opaque_count = 1;
                    data_block->opaque_block = (void*) udt;
                }

                data_block->rank = 1;
                if (data_block->data_type == TYPE_STRING && ndimatt[1] > 0)
                    data_block->rank = 2;    // Attributes are generally rank 1 except strings

                data_block->order = -1;
                if ((data_block->dims = (DIMS*) malloc(data_block->rank * sizeof(DIMS))) == NULL) {
                    err = NETCDF_ERROR_ALLOCATING_HEAP_1;
                    addIdamError(&idamerrorstack, CODEERRORTYPE, "readCDF", err,
                                 "Problem Allocating Dimension Heap Memory");
                    break;
                }
                for (i = 0; i < data_block->rank; i++) {
                    ii = data_block->rank - i - 1;        // Reverse the Indexing
                    initDimBlock(&data_block->dims[ii]);
                    data_block->dims[ii].dim_n = ndimatt[i];
                    data_block->dims[ii].data_type = TYPE_INT;
                    if ((data_block->dims[ii].dim = (char*) malloc(data_block->dims[ii].dim_n * sizeof(int))) ==
                        NULL) {
                        err = NETCDF_ERROR_ALLOCATING_HEAP_1;
                        addIdamError(&idamerrorstack, CODEERRORTYPE, "readCDF", err,
                                     "Problem Allocating Dimension Heap Memory");
                        break;
                    }
                    readCDF4CreateIndex(data_block->dims[ii].dim_n, data_block->dims[ii].dim);
                }
                break;
            }

// Check it's an attribute attached to a variable (assuming a DOT operator)

            if (cdfsubset.subsetCount == 0 &&
                (attname = strstr(variable, ".")) != NULL) {                        // Maybe a variable attribute
                attname[0] = '\0';
                if ((rc2 = nc_inq_varid(grpid, variable, &varid)) == NC_NOERR) {
                    if ((rc2 = nc_inq_atttype(grpid, varid, &attname[1], &atttype)) == NC_NOERR) {
                        if ((rc2 = readCDF4AVar(grouplist, grpid, varid, atttype, &attname[1], &data_block->data_n,
                                                ndimatt,
                                                &data_block->data_type, &data_block->data, &udt)) != NC_NOERR) {
                            err = 999;
                            addIdamError(&idamerrorstack, CODEERRORTYPE, "readCDF", err,
                                         "Unable to read Group Level Attribute data");
                            break;
                        }
                        idamLog(LOG_DEBUG, "netCDF: attribute attached to a variable found.\n");

                        if (udt != NULL) {                // A User Defined Data Structure Type?
                            malloc_source = MALLOCSOURCENETCDF;
                            data_block->opaque_type = OPAQUE_TYPE_STRUCTURES;
                            data_block->opaque_count = 1;
                            data_block->opaque_block = (void*) udt;
                        }

                        data_block->rank = 1;
                        if (data_block->data_type == TYPE_STRING && ndimatt[1] > 0)
                            data_block->rank = 2;    // Attributes are generally rank 1 except strings

                        data_block->order = -1;
                        if ((data_block->dims = (DIMS*) malloc(data_block->rank * sizeof(DIMS))) == NULL) {
                            err = NETCDF_ERROR_ALLOCATING_HEAP_1;
                            addIdamError(&idamerrorstack, CODEERRORTYPE, "readCDF", err,
                                         "Problem Allocating Dimension Heap Memory");
                            break;
                        }
                        for (i = 0; i < data_block->rank; i++) {
                            ii = data_block->rank - i - 1;        // Reverse the Indexing
                            initDimBlock(&data_block->dims[ii]);
                            data_block->dims[ii].dim_n = ndimatt[i];
                            data_block->dims[ii].data_type = TYPE_INT;
                            if ((data_block->dims[ii].dim = (char*) malloc(
                                    data_block->dims[ii].dim_n * sizeof(int))) == NULL) {
                                err = NETCDF_ERROR_ALLOCATING_HEAP_1;
                                addIdamError(&idamerrorstack, CODEERRORTYPE, "readCDF", err,
                                             "Problem Allocating Dimension Heap Memory");
                                break;
                            }
                            readCDF4CreateIndex(data_block->dims[ii].dim_n, data_block->dims[ii].dim);
                        }
                        break;
                    }
                }
            }


// If it's a group name or the Root then return the whole sub-tree (without modification)
// No subsetting operation

            subtree = 0;

            if (hierarchical && cdfsubset.subsetCount == 0 &&
                ((numgrp == 1 && !strcmp(signal_desc->signal_name, "/")) ||
                 ((rc = getGroupId(grpid, variable, &subtree)) == NC_NOERR))) {

                USERDEFINEDTYPE usertype;
                logmalloclist = (LOGMALLOCLIST*) malloc(sizeof(LOGMALLOCLIST));
                initLogMallocList(logmalloclist);
                copyUserDefinedTypeList(
                        &userdefinedtypelist);                // Allocate and Copy the Master User Defined Type List
                initHGroup(&hgroups);

                idamLog(LOG_DEBUG, "netCDF: Tree or sub-tree found.\n");

// Target all User Defined types within the scope of this sub-tree Root node (unless root node is also sub-tree node: Prevents duplicate definitions)

                if (subtree == 0 && numgrp == 1 && !strcmp(signal_desc->signal_name, "/")) {
                    subtree = grpid;        // getCDF4SubTreeMeta  will call getCDF4SubTreeUserDefinedTypes for the root group
                } else {
                    err = getCDF4SubTreeUserDefinedTypes(grpid, &grouplist, userdefinedtypelist);
                    if (err != 0) break;
                }

// Extract all information about groups, variables and attributes within the sub-tree

                if ((err = getCDF4SubTreeMeta(subtree, 0, &usertype, userdefinedtypelist, &hgroups)) != 0) {
                    freeHGroups(&hgroups);
                    break;
                }

                idamLog(LOG_DEBUG, "\nreadCDF4: updating User Defined Type table\n");

                updateUdt(&hgroups, userdefinedtypelist);        // Locate udt pointers using list array index values

                idamLog(LOG_DEBUG, "\nreadCDF4: printing User Defined Type table\n");
                printUserDefinedTypeListTable(*userdefinedtypelist);

// Read all Data and Create the Sub-Tree structure

                idamLog(LOG_DEBUG, "\nreadCDF4: Creating sub-tree data structure\n");

                err = getCDF4SubTreeData((void**) &data_block->data, &hgroups.group[0], &hgroups);

                if (err == NC_NOERR && hgroups.group[0].udt != NULL) {
                    malloc_source = MALLOCSOURCENETCDF;
                    data_block->data_type = TYPE_COMPOUND;
                    data_block->data_n = 1;
                    data_block->rank = 0;
                    data_block->order = -1;
                    data_block->opaque_type = OPAQUE_TYPE_STRUCTURES;
                    data_block->opaque_count = 1;
                    data_block->opaque_block = (void*) hgroups.group[0].udt;
                }

                idamLog(LOG_DEBUG, "\nreadCDF4: Freeing HGroups\n");

                freeHGroups(&hgroups);

                break;
            }

// Can't identify the data object

            err = NETCDF_ERROR_INQUIRING_VARIABLE_1;
            addIdamError(&idamerrorstack, CODEERRORTYPE, "readCDF", err,
                         "The requested dataset or attribute does not exist: check name and case");
            //addIdamError(&idamerrorstack, CODEERRORTYPE, "readCDF", err, (char *)nc_strerror(rc));
            break;
        }

//----------------------------------------------------------------------
// Get Dimension/Coordinate ID List of the variable

        rank = 0;

        if ((rc = nc_inq_varndims(grpid, varid, &rank)) != NC_NOERR) {
            err = NETCDF_ERROR_INQUIRING_DIM_1;
            addIdamError(&idamerrorstack, CODEERRORTYPE, "readCDF", err, (char*) nc_strerror(rc));
            break;
        }

        dimids = NULL;

        if (rank > 0) {
            dimids = (int*) malloc(rank * sizeof(int));
            if ((rc = nc_inq_vardimid(grpid, varid, dimids)) != NC_NOERR) {
                err = NETCDF_ERROR_INQUIRING_DIM_2;
                addIdamError(&idamerrorstack, CODEERRORTYPE, "readCDF", err, (char*) nc_strerror(rc));
                break;
            }
        }

        if (cdfsubset.subsetCount > 0) {
            int i, count = 0;
            cdfsubset.rank = rank;
            for (i = 0; i < rank; i++) cdfsubset.dimids[i] = dimids[i];
            if (cdfsubset.subsetCount > rank) {
                err = 999;
                addIdamError(&idamerrorstack, CODEERRORTYPE, "readCDF", err, "Too many Subset operations specified!");
                break;
            }
            if (cdfsubset.subsetCount < rank) {
                for (i = cdfsubset.subsetCount; i < rank; i++) {
                    cdfsubset.subset[i] = 0;
                    cdfsubset.start[i] = 0;
                    cdfsubset.stop[i] = -1;
                    cdfsubset.count[i] = -1;
                    cdfsubset.stride[i] = 1;
                }
            }

// Check there is at least one dimension to subset!

            for (i = 0; i < rank; i++) if (cdfsubset.subset[i]) count++;
            if (count == 0) cdfsubset.subsetCount = 0;    // Disable all subsetting
        }


//----------------------------------------------------------------------
// Get a list of the Unlimited Dimensions visible from this group

        if ((rc = nc_inq_unlimdims(grpid, &nunlimdims, unlimdimids)) != NC_NOERR) {
            err = NETCDF_ERROR_INQUIRING_DIM_1;
            addIdamError(&idamerrorstack, CODEERRORTYPE, "readCDF", err, (char*) nc_strerror(rc));
            break;
        }

//----------------------------------------------------------------------
// Allocate and Initialise Dimensional/Coordinate Data & Extent data

        data_block->rank = rank;
        data_block->order = -1;        // Don't know the t-vector yet!

// Allocate & Initialise extents (include an additional element for STRING type)

        if ((extent = (unsigned int*) malloc((data_block->rank + 2) * sizeof(unsigned int))) == NULL) {
            err = NETCDF_ERROR_ALLOCATING_HEAP_1;
            addIdamError(&idamerrorstack, CODEERRORTYPE, "readCDF", err,
                         "Problem Allocating Heap Memory for extent array");
            break;
        }

        if ((dextent = (unsigned int*) malloc((data_block->rank + 2) * sizeof(unsigned int))) == NULL) {
            err = NETCDF_ERROR_ALLOCATING_HEAP_1;
            addIdamError(&idamerrorstack, CODEERRORTYPE, "readCDF", err,
                         "Problem Allocating Heap Memory for dimension extent array");
            break;
        }

        if (data_block->rank > 0) {

            if ((data_block->dims = (DIMS*) malloc(data_block->rank * sizeof(DIMS))) == NULL) {
                err = NETCDF_ERROR_ALLOCATING_HEAP_1;
                addIdamError(&idamerrorstack, CODEERRORTYPE, "readCDF", err,
                             "Problem Allocating Dimension Heap Memory");
                break;
            }

            for (i = 0; i < data_block->rank; i++) {
                initDimBlock(&data_block->dims[i]);
                extent[i] = 0;
                dextent[i] = 0;
            }

        } else {
            extent[0] = 0;
            dextent[0] = 0;
        }

        extent[data_block->rank] = 0;
        extent[data_block->rank + 1] = 0;
        dextent[data_block->rank] = 0;
        dextent[data_block->rank + 1] = 0;


//----------------------------------------------------------------------
//----------------------------------------------------------------------
// Read the Data Array first

        isCoordinate = 0;
        err = readCDF4Var(grouplist, varid, isCoordinate, rank, dimids, extent, &data_block->data_n,
                          &data_block->data_type,
                          &isIndex, &data_block->data, &udt);

        if (err != 0) {
            addIdamError(&idamerrorstack, CODEERRORTYPE, "readCDF", err, "Unable to Read Data Values");
            break;
        }

//----------------------------------------------------------------------
// A User Defined Data Structure Type?

        if (udt != NULL) {
            malloc_source = MALLOCSOURCENETCDF;

            data_block->opaque_type = OPAQUE_TYPE_STRUCTURES;
            data_block->opaque_count = 1;
            data_block->opaque_block = (void*) udt;
        }

//----------------------------------------------------------------------
// Apply Data Conversion to Raw Data (Disabled with the property: get_bytes)
// Ignore MAST standard on Rank

        //if(compliance && class == RAW_DATA && rank == 1 && !data_block->client_block.get_bytes)

        if (compliance && class == RAW_DATA && !data_block->client_block.get_bytes) {
            if ((rc = applyCDFCalibration(grpid, varid, data_block->data_n, &data_block->data_type,
                                          &data_block->data)) != NC_NOERR) {
                err = 999;
                addIdamError(&idamerrorstack, CODEERRORTYPE, "readCDF", err, (char*) nc_strerror(rc));
                break;
            }
        }

//----------------------------------------------------------------------
// Data Attributes

        classtxt[0] = '\0';

        err = readCDF4Atts(grpid, varid, data_block->data_units, data_block->data_label, classtxt,
                           data_block->data_desc);

        if (err != 0) {
            addIdamError(&idamerrorstack, CODEERRORTYPE, "readCDF", err, "Unable to Read Data Variable Attributes");
            break;
        }

        if (data_block->data_label[0] == '\0') {
            lstr = (int) strlen(signal_desc->signal_name);
            if (lstr < STRING_LENGTH)
                strcpy(data_block->data_label, signal_desc->signal_name);
            else {
                strncpy(data_block->data_label, signal_desc->signal_name, STRING_LENGTH - 1);
                data_block->data_label[STRING_LENGTH - 1] = '\0';
            }
        }

//----------------------------------------------------------------------
// Error Data Array: Test for Errors Attribute

        if (compliance) {
            error_n = 0;
            isCoordinate = 0;
            if ((err = readCDF4Err(grpid, varid, isCoordinate, class, rank, dimids, &error_n, &data_block->error_type,
                                   &data_block->errhi)) != 0) {
                addIdamError(&idamerrorstack, CODEERRORTYPE, "readCDF", err, "Unable to Read Data Error Values");
                break;
            }

// Check Size is consistent

            if (error_n > 0 && error_n != data_block->data_n) {
                err = 999;
                addIdamError(&idamerrorstack, CODEERRORTYPE, "readCDF", err, "The Shape of the Error Array is Not"
                        "consistent with the Shape of the Data Array!");
                break;
            }
        }

//----------------------------------------------------------------------
//----------------------------------------------------------------------
// Read Dimensional/Coordinate Data

        cgrouplist.count = 0;
        cgrouplist.grpid = 0;
        cgrouplist.grpids = NULL;

// If the type is STRING then extend the rank

        if (data_block->rank == 1 && data_block->data_type == TYPE_STRING && extent[1] > 0) {
            data_block->rank = 2;
            data_block->dims = (DIMS*) realloc((void*) data_block->dims, data_block->rank * sizeof(DIMS));
            for (i = 0; i < data_block->rank; i++) initDimBlock(&data_block->dims[i]);
        }

        for (i = 0; i < data_block->rank; i++) {

            ii = data_block->rank - i - 1;        // Reverse the Indexing  (WHY?)

// Return a Simple Index if the Data are not required


            //if(data_block->client_block.get_nodimdata || (data_block->data_type == TYPE_STRING && i > 0)){
            if (data_block->client_block.get_nodimdata || data_block->data_type == TYPE_STRING) {
                data_block->dims[ii].compressed = 1;
                data_block->dims[ii].data_type = TYPE_UNSIGNED_INT;
                data_block->dims[ii].method = 0;
                data_block->dims[ii].dim0 = 0.0;
                data_block->dims[ii].diff = 1.0;
                data_block->dims[ii].dim_n = extent[i];        // Shape passed from reading the Data Variable array
                continue;
            }

            data_block->dims[ii].compressed = 0;

// Get Dimension Name and Size

            if ((rc = nc_inq_dim(grpid, dimids[i], dimname, &dimlen)) != NC_NOERR) {
                err = NETCDF_ERROR_INQUIRING_DIM_3;
                addIdamError(&idamerrorstack, CODEERRORTYPE, "readCDF", err, (char*) nc_strerror(rc));
                break;
            }

            // Get Coordinate Variable ID (Must be in the Scope of the data variable, i.e., anywhere within the group hierarchy)

            for (j = 0; j < numgrp; j++) {
                if ((rc = nc_inq_varid(grpids[numgrp - j - 1], dimname, &coordid)) == NC_NOERR) {
                    cgrpid = grpids[numgrp - j - 1];
                    break;
                }
            }

            if (rc != NC_NOERR) {        // Coordinate Variable must be missing so use an index array
                data_block->dims[ii].compressed = 1;
                data_block->dims[ii].data_type = TYPE_INT;
                data_block->dims[ii].method = 0;
                data_block->dims[ii].dim = NULL;
                data_block->dims[ii].dim0 = 0.0;
                data_block->dims[ii].diff = 1.0;
                data_block->dims[ii].dim_n = extent[i];        // Shape passed from reading the Data Variable array

                if (extent[i] > 0) {
                    if ((data_block->dims[ii].dim = (char*) malloc(data_block->dims[ii].dim_n * sizeof(int))) ==
                        NULL) {
                        err = NETCDF_ERROR_ALLOCATING_HEAP_1;
                        addIdamError(&idamerrorstack, CODEERRORTYPE, "readCDF", err,
                                     "Problem Allocating Dimension Heap Memory");
                        break;
                    }

                    readCDF4CreateIndex(data_block->dims[ii].dim_n, data_block->dims[ii].dim);
                }
                continue;
            }

// Is this dimension ULIMITED?

            isUnlimited = 0;

            for (j = 0; j < nunlimdims; j++) {
                if (dimids[i] == unlimdimids[j]) {
                    isUnlimited = 1;
                    break;
                }
            }

// Check the Coordinate variable's Rank if not Unlimited (always 1) as may be > 1 for legacy files (e.g. TRANSP)

            if (!isUnlimited) {
                if ((rc = nc_inq_varndims(cgrpid, coordid, &drank)) != NC_NOERR) {
                    err = NETCDF_ERROR_INQUIRING_DIM_1;
                    addIdamError(&idamerrorstack, CODEERRORTYPE, "readCDF", err, (char*) nc_strerror(rc));
                    break;
                }

                if (drank > 1) {        // Length expectation for this Coordinate
                    dextent[i] = extent[i];
                }

            } else drank = 1;



// Disable until efit++ compliant
#ifndef ISCOMPLIANT
            if (compliance && drank > 1) {        // Only accept this if non-compliant file
                err = 999;
                addIdamError(&idamerrorstack, CODEERRORTYPE, "readCDF", err, "Coordinate Array has Rank > 1!");
                break;
            }
#endif
            isCoordinate = 1;

            cgrouplist.grpid = cgrpid;
            err = readCDF4Var(cgrouplist, coordid, isCoordinate, drank, &dimids[i], &dextent[i],
                              &data_block->dims[ii].dim_n,
                              &data_block->dims[ii].data_type, &isIndex, &data_block->dims[ii].dim, &dudt);

            if (err != 0) {
                addIdamError(&idamerrorstack, CODEERRORTYPE, "readCDF", err, "Unable to Read Coordinate Values");
                break;
            }

// Check values are constant if this is a (Legacy) multi-dimensional coordinate array
// If not then generate a simple index coordinate for later substitution by the user and modify the label

            if (drank > 1 && !isIndex) {

                err = readCDFCheckCoordinate(cgrpid, coordid, drank, data_block->dims[ii].dim_n,
                                             data_block->dims[ii].dim);

                if (err > 0) break;

                if (err < 0) {
                    data_block->dims[ii].dim = (char*) realloc((void*) data_block->dims[ii].dim,
                                                               data_block->dims[ii].dim_n * sizeof(int));
                    readCDF4CreateIndex(data_block->dims[ii].dim_n, data_block->dims[ii].dim);
                    data_block->dims[ii].data_type = TYPE_INT;
                    err = 0;
                    isIndex = 1;    // modify the label: Flag the coordinate as Multi-Dimensional
                }
            }

// Apply Data Conversion to Raw Data: Enforce MAST convention on Rank as Coordinate variables must be Rank 1

            if (compliance && class == RAW_DATA && drank == 1) {
                if ((rc = applyCDFCalibration(cgrpid, coordid, data_block->dims[ii].dim_n,
                                              &data_block->dims[ii].data_type, &data_block->dims[ii].dim)) !=
                    NC_NOERR) {
                    err = 999;
                    addIdamError(&idamerrorstack, CODEERRORTYPE, "readCDF", err, (char*) nc_strerror(rc));
                    break;
                }
            }

//----------------------------------------------------------------------
// Read Domain Representation (MAST Convention) of the Coordinate Array

            if (compliance) {

                int type, ncount = 0, nstart = 0, nincrement = 0;
                unsigned int* count = NULL;
                double* start = NULL, * increment = NULL;
                nc_type atype;

                if ((rc = nc_inq_attid(cgrpid, coordid, "count", &attid)) == NC_NOERR) {

// Check the type is compliant and return the Count array

                    if ((rc = nc_inq_atttype(cgrpid, coordid, "count", &atype)) != NC_NOERR || atype != NC_UINT) {
                        err = 999;
                        if (rc != NC_NOERR)
                            addIdamError(&idamerrorstack, CODEERRORTYPE, "readCDF", err,
                                         "Unable to Type Coordinate Domain Count array!");
                        else
                            addIdamError(&idamerrorstack, CODEERRORTYPE, "readCDF", err,
                                         "The Coordinate Domain representation Count Attribute's Type is Not Compliant - must be Unsigned Int!");
                        break;
                    }

                    if ((err = readCDF4AVar(cgrouplist, cgrpid, coordid, NC_UINT, "count", &ncount, ndimatt,
                                            &type, (char**) &count, &dudt)) != 0) {
                        addIdamError(&idamerrorstack, CODEERRORTYPE, "readCDF", err,
                                     "Unable to Read Coordinate Domain Count array");
                        break;
                    }

// Subsetting only applicable to single domain coordinate data

                    if (cdfsubset.subsetCount > 0 && cdfsubset.subset[ii] && ncount > 1) {
                        err = 999;
                        addIdamError(&idamerrorstack, CODEERRORTYPE, "readCDF", err,
                                     "Subset operations are not currently enabled for Multi-Domain Representation of Coordinate variable data!");
                        break;
                    }

                    if ((rc = nc_inq_attid(cgrpid, coordid, "start", &attid)) == NC_NOERR) {

// Check the type is compliant and return the Start array

                        if ((rc = nc_inq_atttype(cgrpid, coordid, "start", &atype)) != NC_NOERR || atype != NC_DOUBLE) {
                            err = 999;
                            if (rc != NC_NOERR)
                                addIdamError(&idamerrorstack, CODEERRORTYPE, "readCDF", err,
                                             "Unable to Type Coordinate Domain Start array");
                            else
                                addIdamError(&idamerrorstack, CODEERRORTYPE, "readCDF", err,
                                             "The Coordinate Domain representation Start Attribute's Type is Not Compliant - must be Double!");
                            break;
                        }

                        if ((err = readCDF4AVar(cgrouplist, cgrpid, coordid, NC_DOUBLE, "start", &nstart, ndimatt,
                                                &type, (char**) &start, &dudt)) != 0) {
                            addIdamError(&idamerrorstack, CODEERRORTYPE, "readCDF", err,
                                         "Unable to Read Coordinate Domain Start array");
                            break;
                        }

                        if ((rc = nc_inq_attid(cgrpid, coordid, "increment", &attid)) == NC_NOERR) {

// Check the type is compliant and return the Increment array

                            if ((rc = nc_inq_atttype(cgrpid, coordid, "increment", &atype)) != NC_NOERR ||
                                atype != NC_DOUBLE) {
                                err = 999;
                                if (rc != NC_NOERR)
                                    addIdamError(&idamerrorstack, CODEERRORTYPE, "readCDF", err,
                                                 "Unable to Type Coordinate Domain Increment array");
                                else
                                    addIdamError(&idamerrorstack, CODEERRORTYPE, "readCDF", err,
                                                 "The Coordinate Domain representation Increment Attribute's Type is Not Compliant - must be Double!");
                                break;
                            }

                            if ((err = readCDF4AVar(cgrouplist, cgrpid, coordid, NC_DOUBLE, "increment", &nincrement,
                                                    ndimatt,
                                                    &type, (char**) &increment, &dudt)) != 0) {
                                addIdamError(&idamerrorstack, CODEERRORTYPE, "readCDF", err,
                                             "Unable to Read Coordinate Domain Increment array");
                                break;
                            }

// Modify if subsetting required

                            if (cdfsubset.subsetCount > 0 && cdfsubset.subset[ii]) {
                                start[0] = start[0] + (double) cdfsubset.start[ii] * increment[0];
                                count[0] = (int) cdfsubset.count[ii];
                                increment[0] = (double) (cdfsubset.stride[ii] * increment[0]);
                            }

                            if (ncount == nstart && nstart == nincrement) {
                                data_block->dims[ii].compressed = 1;
                                data_block->dims[ii].method = 1;
                                data_block->dims[ii].data_type = TYPE_DOUBLE;        // Always type DOUBLE
                                data_block->dims[ii].offs = (char*) start;        // Domain Starting Values
                                data_block->dims[ii].ints = (char*) increment;    // Domain Step Increments
                                data_block->dims[ii].udoms = (int) ncount;        // Number of Domains
                                data_block->dims[ii].sams = (long*) count;        // Domain Lengths

                                if (isUnlimited) {                    // make Consistent with the extent used
                                    unsigned int counter = 0;
                                    data_block->dims[ii].dim_n = extent[i];        // Reduced Array Length
                                    for (j = 0; j < ncount; j++) {
                                        counter = counter + count[j];
                                        if (counter > extent[i]) {
                                            data_block->dims[ii].udoms = (int) j;        // Reduced Number of Domains
                                            count[j] = extent[i] - (counter - count[j]);
                                            data_block->dims[ii].sams = (long*) count;    // Reduced Domain Lengths
                                            break;
                                        }
                                    }
                                }

// Apply Data Conversion to Raw Domain Data

                                if (class == RAW_DATA) {
                                    if ((rc = applyCDFCalibration(cgrpid, coordid, data_block->dims[ii].udoms,
                                                                  &data_block->dims[ii].data_type,
                                                                  &data_block->dims[ii].offs)) != NC_NOERR) {
                                        err = 999;
                                        addIdamError(&idamerrorstack, CODEERRORTYPE, "readCDF", err,
                                                     (char*) nc_strerror(rc));
                                        break;
                                    }
                                    if ((rc = applyCDFCalibration(cgrpid, coordid, data_block->dims[ii].udoms,
                                                                  &data_block->dims[ii].data_type,
                                                                  &data_block->dims[ii].ints)) != NC_NOERR) {
                                        err = 999;
                                        addIdamError(&idamerrorstack, CODEERRORTYPE, "readCDF", err,
                                                     (char*) nc_strerror(rc));
                                        break;
                                    }
                                }

                            } else {
                                if (start != NULL) free((void*) start);
                                if (increment != NULL) free((void*) increment);
                                if (count != NULL) free((void*) count);
                            }
                        }
                    }
                }
            }

//----------------------------------------------------------------------
// Read Attribute Values

            comment[0] = '\0';

            err = readCDF4Atts(cgrpid, coordid, data_block->dims[ii].dim_units, data_block->dims[ii].dim_label,
                               classtxt, comment);

            if (err != 0) {
                addIdamError(&idamerrorstack, CODEERRORTYPE, "readCDF", err, "Unable to Read Coordinate Attributes");
                break;
            }

// Is this the TIME dimension?

            if (compliance) {
                if (!strcmp(classtxt, "time")) data_block->order = ii;    // Yes it is!
            } else {
                if (!strcasecmp(data_block->dims[ii].dim_label, "time") ||
                    !strcasecmp(data_block->dims[ii].dim_label, "time3"))
                    data_block->order = ii;    // Simple (& very poor) test for Time Dimension!

                if (!isIndex) {
                    if (strcmp(dimname,
                               data_block->dims[ii].dim_label)) {    // Add Var Name to Label (Should be a Dimension)
                        lstr = (int) strlen(dimname) + (int) strlen(data_block->dims[ii].dim_label) + 3;
                        if (lstr <= STRING_LENGTH) {
                            strcat(data_block->dims[ii].dim_label, " [");
                            strcat(data_block->dims[ii].dim_label, dimname);    // Only if Different to Dimension Name
                            strcat(data_block->dims[ii].dim_label, "]");
                            LeftTrimString(data_block->dims[ii].dim_label);
                        }
                    }
                } else {
                    lstr = (int) strlen(dimname) + (int) strlen(data_block->dims[ii].dim_label) + 51;
                    if (lstr <= STRING_LENGTH) {
                        strcat(data_block->dims[ii].dim_label,
                               " [Substitute Index into Multi-Dimensional Coordinate Array: ");
                        strcat(data_block->dims[ii].dim_label, dimname);
                        strcat(data_block->dims[ii].dim_label, "]");
                        LeftTrimString(data_block->dims[ii].dim_label);
                    }
                }
            }

//----------------------------------------------------------------------
// Coordinate Error Array

            if (compliance) {
                error_n = 0;
                isCoordinate = 1;
                if ((err = readCDF4Err(cgrpid, coordid, isCoordinate, class, rank, dimids, &error_n,
                                       &data_block->dims[ii].error_type, &data_block->dims[ii].errhi)) != 0) {
                    addIdamError(&idamerrorstack, CODEERRORTYPE, "readCDF", err,
                                 "Unable to Read Coordinate Error Values");
                    break;
                }

// Check Size is consistent

                if (error_n > 0 && error_n != data_block->dims[ii].dim_n) {
                    err = 999;
                    addIdamError(&idamerrorstack, CODEERRORTYPE, "readCDF", err, "The Shape of the Error Array is Not"
                            "consistent with the Shape of the Data Array!");
                    break;
                }
            }

//----------------------------------------------------------------------
// End of Dimension Do Loop

        }

        if (err != 0) break;

//----------------------------------------------------------------------
// End of Error Trap Loop

    } while (0);

//----------------------------------------------------------------------
// Housekeeping

    if (grpids != NULL) free((void*) grpids);
    if (dimids != NULL) free((void*) dimids);
    if (extent != NULL) free((void*) extent);
    if (dextent != NULL) free((void*) dextent);

    rc = gettimeofday(&tv_end0, NULL);
    idamLog(LOG_DEBUG, "\n\nreadCDF4 Total Time: %.2f (ms)\n\n",
            (float) (tv_end0.tv_sec - tv_start0.tv_sec) * 1.0E3 +
            (float) (tv_end0.tv_usec - tv_start0.tv_usec) *
            1.0E-3);

    return err;
}

/*
//-------------------------------------------------------------------------------------------------------
// Locate a specific named group

int getGroupId(int ncgrpid, char* target, int* targetid)
{
    int i, err = 0, numgrps = 0;
    size_t namelength = 0;
    int* ncids = NULL;
    char* grpname = NULL;

// List All Child Groups

    if ((err = nc_inq_grps(ncgrpid, &numgrps, NULL)) != NC_NOERR) return err;

    if (numgrps == 0) return 999;

    ncids = (int*) malloc(sizeof(int) * numgrps);

    if ((err = nc_inq_grps(ncgrpid, &numgrps, ncids)) != NC_NOERR) {
        free((void*) ncids);
        return err;
    }

// Test Child Group Names against Target Group Name

    for (i = 0; i < numgrps; i++) {

        if ((err = nc_inq_grpname_len(ncids[i], (size_t*) &namelength)) != NC_NOERR) {
            if (grpname != NULL) free((void*) grpname);
            free((void*) ncids);
            return err;
        }

        grpname = (char*) realloc((void*) grpname, sizeof(char) * (namelength + 1));

        if ((err = nc_inq_grpname(ncids[i], grpname)) != NC_NOERR) {
            free((void*) grpname);
            free((void*) ncids);
            return err;
        }

        if (!strcmp(grpname, target)) {
            *targetid = ncids[i];            // Found - it exists!
            free((void*) grpname);
            free((void*) ncids);
            return (NC_NOERR);
        }

    }

    free((void*) grpname);
    free((void*) ncids);

    return 999;
}
*/