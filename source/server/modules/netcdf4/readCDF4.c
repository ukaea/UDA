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
#include <strings.h>

#include <clientserver/errorLog.h>
#include <clientserver/udaErrors.h>

//---------------------------------------------------------------------------------------------------------------
// Stub plugin if disabled

#ifdef NONETCDFPLUGIN

int readCDF(DATA_SOURCE data_source,
            SIGNAL_DESC signal_desc,
            REQUEST_BLOCK request_block,
            DATA_BLOCK *data_block) {
    int err = 999;
    addIdamError(&idamerrorstack, CODEERRORTYPE, "readCDF", err, "Cannot Read netCDF Files - PLUGIN NOT ENABLED");
    return err;
}

#else

#include <stdlib.h>

#include <logging/logging.h>
#include <clientserver/udaTypes.h>
#include <clientserver/initStructs.h>
#include <clientserver/stringUtils.h>
#include <clientserver/printStructs.h>
#include <structures/struct.h>

#include "readCDFMeta.h"
#include "readCDF4SubTree.h"
#include "readCDFAtts.h"

//---------------------------------------------------------------------------------------------------------------

nc_type ctype = NC_NAT;            // User defined Complex types
nc_type dctype = NC_NAT;
static int prior_fd = 0;            // Link the Complex types to the File.

int IMAS_HDF_READER = 0;            // Modify behaviour when reading strings from an HDF5 file

CDFSUBSET cdfsubset;

int readCDF4Var(GROUPLIST grouplist, int varid, int isCoordinate, int rank, int* dimids, unsigned int* extent,
                int* ndvec, int* data_type, int* isIndex, char** data, LOGMALLOCLIST* logmalloclist,
                USERDEFINEDTYPELIST* userdefinedtypelist, USERDEFINEDTYPE** udt);

int readCDF4AVar(GROUPLIST grouplist, int grpid, int varid, nc_type atttype, char* name, int* ndvec, int ndims[2],
                 int* data_type, char** data, LOGMALLOCLIST* logmalloclist, USERDEFINEDTYPELIST* userdefinedtypelist,
                 USERDEFINEDTYPE** udt);

unsigned int readCDF4Properties()
{
    static unsigned int init = 1;
    static unsigned int cdfProperties = 0;
    if (init) {
        char* env = getenv("UDA_CDFPROPERTIES");    // Assign behaviour via the server's environment
        if (env != NULL) cdfProperties = (unsigned int)atoi(env);
        init = 0;
    }
    return cdfProperties;
}

int readCDF(DATA_SOURCE data_source, SIGNAL_DESC signal_desc, REQUEST_BLOCK request_block, DATA_BLOCK* data_block,
            LOGMALLOCLIST* logmalloclist, USERDEFINEDTYPELIST* userdefinedtypelist)
{

    int getMeta = 0;            // ************** temporary

    METAXML metaxml, closexml;
    char xml[STRING_LENGTH];
    char classtxt[STRING_LENGTH];
    char comment[STRING_LENGTH];

    int fd, err = 0, rc;
    int fusion_ver = 0;

    int i, ii, j, lstr, lname, rank, drank, varid, coordid, grpid, cgrpid, attid, error_n;
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
// Initialise the META XML Structure

    metaxml.xml = NULL;
    metaxml.lheap = 0;
    metaxml.nxml = 0;

    closexml.xml = NULL;
    closexml.lheap = 0;
    closexml.nxml = 0;

    if (getMeta) addMetaXML(&metaxml, "<?xml version=\"1.0\"?>\n<netcdf-4>\n<root>\n");    // Top level tags

//----------------------------------------------------------------------
// Error Trap Loop

    err = 0;

    do {

//----------------------------------------------------------------------
// Modify behaviour when reading strings from an HDF5 file

        if (((token = strrchr(data_source.path, '.')) != NULL) && STR_EQUALS(token, ".hd5")) {    // Test File extension
            if ((token = getenv("IMAS_HDF_READER")) != NULL) IMAS_HDF_READER = 1;
        }

//----------------------------------------------------------------------
// Is the netCDF File Already open for Reading? If Not then Open in READ ONLY mode

        errno = 0;

        err = nc_open((const char*)data_source.path, NC_NOWRITE, &fd);

        ctype = NC_NAT;
        dctype = NC_NAT;

        if (err != NC_NOERR) {
            addIdamError(&idamerrorstack, SYSTEMERRORTYPE, "readCDF", err, (char*)nc_strerror(err));
            break;
        }

        IDAM_LOGF(UDA_LOG_DEBUG, "netCDF filename %s\n", data_source.path);

//----------------------------------------------------------------------
// Test the Library Version Number

        if (getMeta) {
            if ((cp = (char*)nc_inq_libvers()) != NULL) {
                addMetaXML(&metaxml, "<library>\"");
                addMetaXML(&metaxml, cp);
                addMetaXML(&metaxml, "\"</library>\n");
            }
        }

//----------------------------------------------------------------------
// Test the File Format Version. Was the file written using hierarchical netCDF4 layout?
// If NC_FORMAT_CLASSIC or NC_FORMAT_64BIT then flat file (non-hierarchical) version assumed.

        hierarchical = 0;

        if ((rc = nc_inq_format(fd, &format)) == NC_NOERR) {
            hierarchical = (format == NC_FORMAT_NETCDF4) || (format == NC_FORMAT_NETCDF4_CLASSIC);
        }

        IDAM_LOGF(UDA_LOG_DEBUG, "netCDF hierarchical organisation ? %d\n", hierarchical);

//----------------------------------------------------------------------
// FUDGE for netcdf-3 TRANSP data (This won't work if the source alias is unknown, e.g. when private file)

        //if(hierarchical && STR_EQUALS(signal_desc.source_alias, "transp")) hierarchical = 0;

//----------------------------------------------------------------------
// Global Meta Data: What convention has been adopted? Data Class? Build XML if Meta data requested

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
                    addIdamError(&idamerrorstack, CODEERRORTYPE, "readCDF", err, (char*)nc_strerror(rc));
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
                    char** conv = (char**)malloc(sizeof(char*));
                    if ((err = nc_get_att_string(fd, NC_GLOBAL, "Conventions", conv)) != NC_NOERR &&
                        (err = nc_get_att_string(fd, NC_GLOBAL, "_Conventions", conv)) != NC_NOERR) {
                        addIdamError(&idamerrorstack, CODEERRORTYPE, "readCDF", err, (char*)nc_strerror(err));
                        break;
                    }
                    attlen = (int)strlen(conv[0]) + 1;
                    conventions = (char*)malloc(attlen * sizeof(char));
                    strcpy(conventions, conv[0]);
                    nc_free_string(1, conv);
                    free((void*)conv);
                } else {
                    conventions = (char*)malloc((attlen + 1) * sizeof(char));
                    conventions[0] = '\0';
                    if ((err = nc_get_att_text(fd, NC_GLOBAL, "Conventions", conventions)) != NC_NOERR &&
                        (err = nc_get_att_text(fd, NC_GLOBAL, "_Conventions", conventions)) != NC_NOERR) {
                        addIdamError(&idamerrorstack, CODEERRORTYPE, "readCDF", err, (char*)nc_strerror(err));
                        free((void*)conventions);
                        break;
                    }
                    conventions[attlen] = '\0';        // Ensure Null terminated
                }
                IDAM_LOGF(UDA_LOG_DEBUG, "netCDF file Conventions?  %s\n", conventions);

                if (conventions[0] != '\0') {

                    if (getMeta) {
                        addMetaXML(&metaxml, "<Conventions>\"");
                        addMetaXML(&metaxml, conventions);
                        addMetaXML(&metaxml, "\"</Conventions>\n");
                    }

                    if ((cp = strstr(conventions, "MAST-")) != NULL) {
                        if ((token = strstr(&cp[5], ".")) != NULL) {
                            token[0] = '\0';
                            //mast_ver = atoi(&cp[5]);	// Need major part only
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

                free((void*)conventions);
            }

// FUDGE for efit++ data

            //if(!compliance && STR_EQUALS(signal_desc.source_alias, "efit")) compliance = 1;


            IDAM_LOGF(UDA_LOG_DEBUG, "netCDF file compliance?  %d\n", compliance);

            if (compliance) {
                attlen = 0;
                char* classification;
                if ((rc = nc_inq_attlen(fd, NC_GLOBAL, "class", &attlen)) == NC_NOERR ||
                    (rc = nc_inq_attlen(fd, NC_GLOBAL, "_class", &attlen)) == NC_NOERR) {
                    if ((err = nc_inq_atttype(fd, NC_GLOBAL, "class", &atype)) != NC_NOERR &&
                        (err = nc_inq_atttype(fd, NC_GLOBAL, "_class", &atype)) != NC_NOERR) {
                        addIdamError(&idamerrorstack, CODEERRORTYPE, "readCDF", err, (char*)nc_strerror(err));
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
                        char** class = (char**)malloc(sizeof(char*));
                        if ((err = nc_get_att_string(fd, NC_GLOBAL, "class", class)) != NC_NOERR &&
                            (err = nc_get_att_string(fd, NC_GLOBAL, "_class", class)) != NC_NOERR) {
                            addIdamError(&idamerrorstack, CODEERRORTYPE, "readCDF", err, (char*)nc_strerror(err));
                            break;
                        }
                        attlen = (int)strlen(class[0]) + 1;
                        classification = (char*)malloc(attlen * sizeof(char));
                        strcpy(classification, class[0]);
                        nc_free_string(1, class);
                        free((void*)class);
                    } else {
                        classification = (char*)malloc((attlen + 1) * sizeof(char));
                        classification[0] = '\0';
                        if ((err = nc_get_att_text(fd, NC_GLOBAL, "class", classification)) != NC_NOERR &&
                            (err = nc_get_att_text(fd, NC_GLOBAL, "_class", classification)) != NC_NOERR) {
                            addIdamError(&idamerrorstack, CODEERRORTYPE, "readCDF", err, (char*)nc_strerror(err));
                            free((void*)classification);
                            break;
                        }
                        classification[attlen] = '\0';        // Ensure Null terminated
                    }
                    if (getMeta) {
                        addMetaXML(&metaxml, "<class>\"");
                        addMetaXML(&metaxml, classification);
                        addMetaXML(&metaxml, "\"</class>\n");
                    }

                    if (STR_IEQUALS(classification, "raw data")) {
                        class = RAW_DATA;
                    } else {
                        if (STR_IEQUALS(classification, "analysed data")) {
                            class = ANALYSED_DATA;
                        } else {
                            if (STR_IEQUALS(classification, "modelled data")) {
                                class = MODELLED_DATA;
                            }
                        }
                    }

                    free((void*)classification);
                }
            }
        }

        IDAM_LOGF(UDA_LOG_DEBUG, "netCDF file class?  %d\n", class);

//----------------------------------------------------------------------
// Complex Data Types (Done once per file if the Conventions are for FUSION and MAST)

        if (compliance && fusion_ver >= 1 && ((ctype == NC_NAT && dctype == NC_NAT) || prior_fd != fd)) {
            int ntypes = 0;
            nc_type* typeids = NULL;
            do {
                if ((err = nc_inq_typeids(fd, &ntypes, NULL)) != NC_NOERR || ntypes == 0) break;
                typeids = (int*)malloc(ntypes * sizeof(int));
                if ((err = nc_inq_typeids(fd, &ntypes, typeids)) != NC_NOERR) break;
                for (i = 0; i < ntypes; i++) {
                    //if ((err = nc_inq_compound_name(fd, (nc_type)typeids[i], typename)) != NC_NOERR) break;
                    typename[0] = '\0';
                    nc_inq_compound_name(fd, typeids[i], typename);        // ignore non-compound types
                    if (STR_EQUALS(typename, "complex")) ctype = typeids[i];
                    if (STR_EQUALS(typename, "dcomplex")) dctype = typeids[i];
                }
            } while (0);
            if (typeids != NULL) free((void*)typeids);
            if (err != NC_NOERR) {
                addIdamError(&idamerrorstack, CODEERRORTYPE, "readCDF", 999, (char*)nc_strerror(err));
                return err;
            }
            prior_fd = fd;
        }

//----------------------------------------------------------------------
// Read all top level attributes and copy to the Meta data XML

        if (getMeta && compliance) {
            if ((err = addIntMetaXML(fd, NC_GLOBAL, &metaxml, "shot")) != 0) break;
            if ((err = addIntMetaXML(fd, NC_GLOBAL, &metaxml, "pass")) != 0) break;
            if ((err = addIntMetaXML(fd, NC_GLOBAL, &metaxml, "status")) != 0) break;

            if ((err = addTextMetaXML(fd, NC_GLOBAL, &metaxml, "generator")) != 0) break;
            if ((err = addTextMetaXML(fd, NC_GLOBAL, &metaxml, "software")) != 0) break;
            if ((err = addTextMetaXML(fd, NC_GLOBAL, &metaxml, "title")) != 0) break;
            if ((err = addTextMetaXML(fd, NC_GLOBAL, &metaxml, "date")) != 0) break;
            if ((err = addTextMetaXML(fd, NC_GLOBAL, &metaxml, "time")) != 0) break;
            if ((err = addTextMetaXML(fd, NC_GLOBAL, &metaxml, "comment")) != 0) break;

            addMetaXML(&metaxml, "\n</root>\n");        // Close the Top level META data tag
        }

//----------------------------------------------------------------------
// Test signal name for non unique compliant name: devices
// signal_alias must be the source alias => no entry in database found
// If found then replace with a truncated form

// ***** This assumes a 3 letter source_alias name prefix !!!!!

        if (compliance) {
            if (STR_EQUALS(&signal_desc.signal_name[4], "/devices/")) {        //   /xyc/devices/...
                strncpy(variable, &signal_desc.signal_name[1], 3);
                variable[3] = '\0';
                IDAM_LOG(UDA_LOG_DEBUG, "devices signal requested\n");
                IDAM_LOGF(UDA_LOG_DEBUG, "source alias: [%s]\n", variable);
                IDAM_LOGF(UDA_LOG_DEBUG, "source alias: [%s]\n", signal_desc.signal_alias);
                if (STR_EQUALS(signal_desc.signal_alias, variable)) {
                    strcpy(variable, &signal_desc.signal_name[4]);
                    strcpy(signal_desc.signal_name, variable);
                    IDAM_LOG(UDA_LOG_DEBUG, "Not recorded in Database: Removing source alias prefix\n");
                    IDAM_LOGF(UDA_LOG_DEBUG, "Target signal: %s\n", signal_desc.signal_name);
                }
            }
        }

//----------------------------------------------------------------------
// Get Group ID List - Group Hierarchy - from the top down to the dataset

        if ((lname = (int)strlen(signal_desc.signal_name) + 2) > lnamemax) {
            err = 999;
            addIdamError(&idamerrorstack, CODEERRORTYPE, "readCDF", err, "the Signal Name is too long for netCDF!");
            break;
        }

        strcpy(variable, signal_desc.signal_name);

        IDAM_LOGF(UDA_LOG_DEBUG, "netCDF signal name?  %s\n", variable);

        if (hierarchical) {
            char* p = NULL;
            group = (char*)malloc(lname * sizeof(char));

            if (signal_desc.signal_name[0] == '/')
                strcpy(group, signal_desc.signal_name);        // Contains the Top Level Group identifier
            else {
                sprintf(group, "/%s", signal_desc.signal_name);    // Insert the Top Level Group identifier
            }

            if ((p = strrchr(group, (int)'/')) != NULL) {
                if (p != &group[0]) {                    // Variable is not attached to top level group
                    *p = '\0';                    // Split the String into group hierarchy and variable name
                    strcpy(variable, &p[1]);
                } else {
                    strcpy(variable, &p[1]);
                    p[1] = '\0';                    // Top Level Group
                }
            }

            grpids = (int*)malloc(sizeof(int) * numgrps);
            grpids[0] = fd;

            if (strcmp(group, "/") != 0) {                // Not required if Top Level Group

                work = (char*)malloc((strlen(group) + 1) * sizeof(char));
                strcpy(work, &group[1]);                // Skip the leading '/' character

                if ((token = strtok(work, "/")) != NULL) {        // Tokenise for 1 or more grouping levels
                    if ((err = getGroupId(grpids[numgrp - 1], token, &grpids[numgrp])) != NC_NOERR) {
                        err = NETCDF_ERROR_INQUIRING_VARIABLE_1;
                        addIdamError(&idamerrorstack, CODEERRORTYPE, "readCDF", err,
                                     "Unable to Locate a Hierarchical Group");
                        free((void*)work);
                        free((void*)group);
                        work = NULL;
                        group = NULL;
                        break;
                    }
                    numgrp++;

                    if (getMeta && compliance) {
                        sprintf(xml, "<%s>\n", token);
                        addMetaXML(&metaxml, xml);
                        sprintf(xml, "\n</%s>\n", token);
                        addMetaXML(&closexml, xml);
                        addTextMetaXML(fd, grpids[numgrp - 1], &metaxml, "title");
                        addTextMetaXML(fd, grpids[numgrp - 1], &metaxml, "comment");
                    }

                    while ((token = strtok(NULL, "/")) != NULL) {
                        if ((err = getGroupId(grpids[numgrp - 1], token, &grpids[numgrp])) != NC_NOERR) {
                            err = NETCDF_ERROR_INQUIRING_VARIABLE_1;
                            addIdamError(&idamerrorstack, CODEERRORTYPE, "readCDF", err,
                                         "Unable to Locate a Hierarchical Group");
                            free((void*)work);
                            free((void*)group);
                            work = NULL;
                            group = NULL;
                            break;
                        }
                        numgrp++;

                        if (getMeta && compliance) {
                            sprintf(xml, "<%s>\n", token);
                            addMetaXML(&metaxml, xml);
                            sprintf(xml, "\n</%s>\n", token);
                            addMetaXML(&closexml, xml);
                            addTextMetaXML(fd, grpids[numgrp - 1], &metaxml, "title");
                            addTextMetaXML(fd, grpids[numgrp - 1], &metaxml, "comment");
                        }

                        if (numgrp == numgrps) {
                            numgrps = numgrps + 10;
                            grpids = (int*)realloc((void*)grpids,
                                                   sizeof(int) * numgrps);    // Extend the Array of IDs
                        }
                    }
                    if (err != NC_NOERR) break;

// Close XML tags (A work in Progress !!!)

                    if (getMeta && compliance) addMetaXML(&metaxml, closexml.xml);

                }
                free((void*)work);
                work = NULL;
            }
            free((void*)group);
            group = NULL;
            grpid = grpids[numgrp - 1];        // Lowest Group in Hierarchy

            grouplist.count = numgrp;
            grouplist.grpid = grpid;
            grouplist.grpids = grpids;

        } else {
            numgrp = 1;
            grpid = fd;                // Always the Top Level group
            grpids = (int*)malloc(sizeof(int));
            grpids[0] = fd;

            grouplist.count = 1;
            grouplist.grpid = fd;
            grouplist.grpids = grpids;

        }

//----------------------------------------------------------------------
// Does the variable name contain sub-setting instructions [start:stop:stride]

        varid = -1;
        cdfsubset.subsetCount = request_block.datasubset.subsetCount;

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
                    cdfsubset.subset[i] = request_block.datasubset.subset[i];
                    cdfsubset.start[i] = (size_t)request_block.datasubset.start[i];
                    cdfsubset.stop[i] = (size_t)request_block.datasubset.stop[i];
                    cdfsubset.count[i] = (size_t)request_block.datasubset.count[i];
                    cdfsubset.stride[i] = (ptrdiff_t)request_block.datasubset.stride[i];
                }
            }

// Does the subset operation remain within the signal name string: extract if so

            char* work = (char*)malloc((strlen(variable) + 1) * sizeof(char));
            strcpy(work, variable);
            char* p = strstr(work, request_block.subset);
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

            IDAM_LOG(UDA_LOG_DEBUG, "variable not found ... trying other options ...\n");

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
                data_block->data_n = (int)data_n;
                IDAM_LOG(UDA_LOG_DEBUG, "unwritten Coordinate dataset found.\n");

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
                    if ((int)cdfsubset.stop[0] == -1) cdfsubset.stop[0] = (size_t)(data_block->data_n - 1);
                    if ((int)cdfsubset.count[0] == -1) {
                        cdfsubset.count[0] = cdfsubset.stop[0] - cdfsubset.start[0] + 1;
                        if ((int)cdfsubset.stride[0] > 1 && (int)cdfsubset.count[0] > 1) {
                            if (((int)cdfsubset.count[0] % (int)cdfsubset.stride[0]) > 0) {
                                cdfsubset.count[0] = 1 + (int)cdfsubset.count[0] / (int)cdfsubset.stride[0];
                            } else {
                                cdfsubset.count[0] = (int)cdfsubset.count[0] / (int)cdfsubset.stride[0];
                            }
                        }
                    }
                    data_block->data_n = (int)cdfsubset.count[0];
                }

                if ((data_block->data = (char*)malloc(data_block->data_n * sizeof(int))) == NULL) {
                    err = NETCDF_ERROR_ALLOCATING_HEAP_1;
                    addIdamError(&idamerrorstack, CODEERRORTYPE, "readCDF", err, "Problem Allocating Data Heap Memory");
                    break;
                }

                readCDF4CreateIndex(data_block->data_n, data_block->data);

                if ((data_block->dims = (DIMS*)malloc(data_block->rank * sizeof(DIMS))) == NULL) {
                    err = NETCDF_ERROR_ALLOCATING_HEAP_1;
                    addIdamError(&idamerrorstack, CODEERRORTYPE, "readCDF", err,
                                 "Problem Allocating Dimension Heap Memory");
                    break;
                }

                initDimBlock(&data_block->dims[0]);
                data_block->dims[0].dim_n = data_block->data_n;
                data_block->dims[0].data_type = TYPE_INT;

                if ((data_block->dims[0].dim = (char*)malloc(data_block->data_n * sizeof(int))) == NULL) {
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
                                        &data_block->data_type, &data_block->data, logmalloclist, userdefinedtypelist, &udt)) != NC_NOERR) {
                    err = 999;
                    addIdamError(&idamerrorstack, CODEERRORTYPE, "readCDF", err,
                                 "Unable to read Group Level Attribute data");
                    break;
                }
                IDAM_LOG(UDA_LOG_DEBUG, "attribute attached to a group found.\n");

                if (udt != NULL) {                // A User Defined Data Structure Type?
                    malloc_source = MALLOCSOURCENETCDF;
                    data_block->opaque_type = OPAQUE_TYPE_STRUCTURES;
                    data_block->opaque_count = 1;
                    data_block->opaque_block = (void*)udt;
                }

                data_block->rank = 1;
                if (data_block->data_type == TYPE_STRING && ndimatt[1] > 0) {
                    data_block->rank = 2;
                }    // Attributes are generally rank 1 except strings

                data_block->order = -1;
                if ((data_block->dims = (DIMS*)malloc(data_block->rank * sizeof(DIMS))) == NULL) {
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
                    if ((data_block->dims[ii].dim = (char*)malloc(data_block->dims[ii].dim_n * sizeof(int))) ==
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
                                                ndimatt, &data_block->data_type, &data_block->data, logmalloclist,
                                                userdefinedtypelist, &udt)) != NC_NOERR) {
                            err = 999;
                            addIdamError(&idamerrorstack, CODEERRORTYPE, "readCDF", err,
                                         "Unable to read Group Level Attribute data");
                            break;
                        }
                        IDAM_LOG(UDA_LOG_DEBUG, "attribute attached to a variable found.\n");

                        if (udt != NULL) {                // A User Defined Data Structure Type?
                            malloc_source = MALLOCSOURCENETCDF;
                            data_block->opaque_type = OPAQUE_TYPE_STRUCTURES;
                            data_block->opaque_count = 1;
                            data_block->opaque_block = (void*)udt;
                        }

                        data_block->rank = 1;
                        if (data_block->data_type == TYPE_STRING && ndimatt[1] > 0) {
                            data_block->rank = 2;
                        }    // Attributes are generally rank 1 except strings

                        data_block->order = -1;
                        if ((data_block->dims = (DIMS*)malloc(data_block->rank * sizeof(DIMS))) == NULL) {
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
                            if ((data_block->dims[ii].dim = (char*)malloc(
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

            int subtree = 0;
            HGROUPS hgroups;

            if (hierarchical && cdfsubset.subsetCount == 0 &&
                ((numgrp == 1 && STR_EQUALS(signal_desc.signal_name, "/")) ||
                 ((rc = getGroupId(grpid, variable, &subtree)) ==
                  NC_NOERR))) {

                USERDEFINEDTYPE usertype;
                LOGMALLOCLIST* logmalloclist = (LOGMALLOCLIST*)malloc(sizeof(LOGMALLOCLIST));
                initLogMallocList(logmalloclist);
                copyUserDefinedTypeList(&userdefinedtypelist); // Allocate and Copy the Master User Defined Type List
                initHGroup(&hgroups);

                IDAM_LOG(UDA_LOG_DEBUG, "Tree or sub-tree found.\n");

                // Target all User Defined types within the scope of this sub-tree Root node (unless root node is also sub-tree node: Prevents duplicate definitions)

                if (subtree == 0 && numgrp == 1 && STR_EQUALS(signal_desc.signal_name, "/")) {
                    subtree = grpid;        // getCDF4SubTreeMeta  will call getCDF4SubTreeUserDefinedTypes for the root group
                } else {
                    err = getCDF4SubTreeUserDefinedTypes(grpid, &grouplist, userdefinedtypelist);
                    if (err != 0) break;
                }

                // Extract all information about groups, variables and attributes within the sub-tree

                if ((err = getCDF4SubTreeMeta(subtree, 0, &usertype, logmalloclist, userdefinedtypelist, &hgroups)) != 0) {
                    freeHGroups(&hgroups);
                    break;
                }

                IDAM_LOG(UDA_LOG_DEBUG, "updating User Defined Type table\n");

                updateUdt(&hgroups, userdefinedtypelist);        // Locate udt pointers using list array index values

                //repeatUdt(userdefinedtypelist);			// Standardise type names - identify repeated types differing only in name.

                IDAM_LOG(UDA_LOG_DEBUG, "printing User Defined Type table\n");
                printUserDefinedTypeListTable(*userdefinedtypelist);

                // Read all Data and Create the Sub-Tree structure

                IDAM_LOG(UDA_LOG_DEBUG, "Creating sub-tree data structure\n");

                err = getCDF4SubTreeData(logmalloclist, userdefinedtypelist, (void**)&data_block->data, &hgroups.group[0], &hgroups);

                if (err == NC_NOERR && hgroups.group[0].udt != NULL) {
                    malloc_source = MALLOCSOURCENETCDF;
                    data_block->data_type = TYPE_COMPOUND;
                    data_block->data_n = 1;
                    data_block->rank = 0;
                    data_block->order = -1;
                    data_block->opaque_type = OPAQUE_TYPE_STRUCTURES;
                    data_block->opaque_count = 1;
                    data_block->opaque_block = (void*)hgroups.group[0].udt;
                }

                IDAM_LOG(UDA_LOG_DEBUG, "Freeing HGroups\n");
                freeHGroups(&hgroups);

                break;
            }

            // Can't identify the data object

            err = NETCDF_ERROR_INQUIRING_VARIABLE_1;
            addIdamError(&idamerrorstack, CODEERRORTYPE, "readCDF", err,
                         "The requested dataset or attribute does not exist: check name and case");
            break;
        }

        //----------------------------------------------------------------------
        // Get Dimension/Coordinate ID List of the variable

        rank = 0;

        if ((rc = nc_inq_varndims(grpid, varid, &rank)) != NC_NOERR) {
            err = NETCDF_ERROR_INQUIRING_DIM_1;
            addIdamError(&idamerrorstack, CODEERRORTYPE, "readCDF", err, (char*)nc_strerror(rc));
            break;
        }

        dimids = NULL;

        if (rank > 0) {
            dimids = (int*)malloc(rank * sizeof(int));
            if ((rc = nc_inq_vardimid(grpid, varid, dimids)) != NC_NOERR) {
                err = NETCDF_ERROR_INQUIRING_DIM_2;
                addIdamError(&idamerrorstack, CODEERRORTYPE, "readCDF", err, (char*)nc_strerror(rc));
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
            addIdamError(&idamerrorstack, CODEERRORTYPE, "readCDF", err, (char*)nc_strerror(rc));
            break;
        }

        //----------------------------------------------------------------------
        // Allocate and Initialise Dimensional/Coordinate Data & Extent data

        data_block->rank = rank;
        data_block->order = -1;        // Don't know the t-vector yet!

        // Allocate & Initialise extents (include an additional element for STRING type)

        if ((extent = (unsigned int*)malloc((data_block->rank + 2) * sizeof(unsigned int))) == NULL) {
            err = NETCDF_ERROR_ALLOCATING_HEAP_1;
            addIdamError(&idamerrorstack, CODEERRORTYPE, "readCDF", err,
                         "Problem Allocating Heap Memory for extent array");
            break;
        }

        if ((dextent = (unsigned int*)malloc((data_block->rank + 2) * sizeof(unsigned int))) == NULL) {
            err = NETCDF_ERROR_ALLOCATING_HEAP_1;
            addIdamError(&idamerrorstack, CODEERRORTYPE, "readCDF", err,
                         "Problem Allocating Heap Memory for dimension extent array");
            break;
        }

        if (data_block->rank > 0) {

            if ((data_block->dims = (DIMS*)malloc(data_block->rank * sizeof(DIMS))) == NULL) {
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
                          &data_block->data_type, &isIndex, &data_block->data, logmalloclist,
                          userdefinedtypelist, &udt);

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
            data_block->opaque_block = (void*)udt;
        }

        //----------------------------------------------------------------------
        // Apply Data Conversion to Raw Data (Disabled with the property: get_bytes)
        // Ignore MAST standard on Rank

        //if(compliance && class == RAW_DATA && rank == 1 && !data_block->client_block.get_bytes)

        if (compliance && class == RAW_DATA && !data_block->client_block.get_bytes) {
            if ((rc = applyCDFCalibration(grpid, varid, data_block->data_n, &data_block->data_type,
                                          &data_block->data)) != NC_NOERR) {
                err = 999;
                addIdamError(&idamerrorstack, CODEERRORTYPE, "readCDF", err, (char*)nc_strerror(rc));
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
            lstr = (int)strlen(signal_desc.signal_name);
            if (lstr < STRING_LENGTH)
                strcpy(data_block->data_label, signal_desc.signal_name);
            else {
                strncpy(data_block->data_label, signal_desc.signal_name, STRING_LENGTH - 1);
                data_block->data_label[STRING_LENGTH - 1] = '\0';
            }
        }

        //----------------------------------------------------------------------
        // Error Data Array: Test for Errors Attribute

        if (compliance) {
            error_n = 0;
            isCoordinate = 0;
            if ((err = readCDF4Err(grpid, varid, isCoordinate, class, rank, dimids, &error_n, &data_block->error_type,
                                   &data_block->errhi, logmalloclist, userdefinedtypelist)) != 0) {
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

        printClientBlock(data_block->client_block);

        if (getMeta && compliance) addMetaXML(&metaxml, "<coordinates>\n");

        cgrouplist.count = 0;
        cgrouplist.grpid = 0;
        cgrouplist.grpids = NULL;

        // If the type is STRING then extend the rank

        if (data_block->rank == 1 && data_block->data_type == TYPE_STRING && extent[1] > 0) {
            data_block->rank = 2;
            data_block->dims = (DIMS*)realloc((void*)data_block->dims, data_block->rank * sizeof(DIMS));
            for (i = 0; i < data_block->rank; i++) initDimBlock(&data_block->dims[i]);
        }

        for (i = 0; i < data_block->rank; i++) {

            ii = data_block->rank - i - 1;        // Reverse the Indexing  (WHY?)

            // Return a Simple Index if the Data are not required

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
                addIdamError(&idamerrorstack, CODEERRORTYPE, "readCDF", err, (char*)nc_strerror(rc));
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
                    if ((data_block->dims[ii].dim = (char*)malloc(data_block->dims[ii].dim_n * sizeof(int))) ==
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
                    addIdamError(&idamerrorstack, CODEERRORTYPE, "readCDF", err, (char*)nc_strerror(rc));
                    break;
                }

                if (drank > 1) {        // Length expectation for this Coordinate
                    dextent[i] = extent[i];
                }

            } else {
                drank = 1;
            }

            if (compliance && drank > 1) {        // Only accept this if non-compliant file
                err = 999;
                addIdamError(&idamerrorstack, CODEERRORTYPE, "readCDF", err, "Coordinate Array has Rank > 1!");
                break;
            }

            isCoordinate = 1;
            cgrouplist.grpid = cgrpid;
            err = readCDF4Var(cgrouplist, coordid, isCoordinate, drank, &dimids[i], &dextent[i],
                              &data_block->dims[ii].dim_n, &data_block->dims[ii].data_type, &isIndex,
                              &data_block->dims[ii].dim, logmalloclist, userdefinedtypelist, &dudt);
            if (err != 0) {
                addIdamError(&idamerrorstack, CODEERRORTYPE, "readCDF", err, "Unable to Read Coordinate Values");
                break;
            }

            // Check values are constant if this is a (Legacy) multi-dimensional coordinate array
            // If not then generate a simple index coordinate for later substitution by the user and modify the label

            if (drank > 1 && !isIndex) {

                err = readCDFCheckCoordinate(cgrpid, coordid, drank, data_block->dims[ii].dim_n,
                                             data_block->dims[ii].dim, logmalloclist, userdefinedtypelist);

                if (err > 0) break;

                if (err < 0) {
                    data_block->dims[ii].dim = (char*)realloc((void*)data_block->dims[ii].dim,
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
                    addIdamError(&idamerrorstack, CODEERRORTYPE, "readCDF", err, (char*)nc_strerror(rc));
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
                        if (rc != NC_NOERR) {
                            addIdamError(&idamerrorstack, CODEERRORTYPE, "readCDF", err,
                                         "Unable to Type Coordinate Domain Count array!");
                        } else {
                            addIdamError(&idamerrorstack, CODEERRORTYPE, "readCDF", err,
                                         "The Coordinate Domain representation Count Attribute's Type is Not Compliant - must be Unsigned Int!");
                        }
                        break;
                    }

                    if ((err = readCDF4AVar(cgrouplist, cgrpid, coordid, NC_UINT, "count", &ncount, ndimatt,
                                            &type, (char**)&count, logmalloclist, userdefinedtypelist, &dudt)) != 0) {
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
                            if (rc != NC_NOERR) {
                                addIdamError(&idamerrorstack, CODEERRORTYPE, "readCDF", err,
                                             "Unable to Type Coordinate Domain Start array");
                            } else {
                                addIdamError(&idamerrorstack, CODEERRORTYPE, "readCDF", err,
                                             "The Coordinate Domain representation Start Attribute's Type is Not Compliant - must be Double!");
                            }
                            break;
                        }

                        if ((err = readCDF4AVar(cgrouplist, cgrpid, coordid, NC_DOUBLE, "start", &nstart, ndimatt,
                                                &type, (char**)&start, logmalloclist, userdefinedtypelist, &dudt)) != 0) {
                            addIdamError(&idamerrorstack, CODEERRORTYPE, "readCDF", err,
                                         "Unable to Read Coordinate Domain Start array");
                            break;
                        }

                        if ((rc = nc_inq_attid(cgrpid, coordid, "increment", &attid)) == NC_NOERR) {

                            // Check the type is compliant and return the Increment array

                            if ((rc = nc_inq_atttype(cgrpid, coordid, "increment", &atype)) != NC_NOERR ||
                                atype != NC_DOUBLE) {
                                err = 999;
                                if (rc != NC_NOERR) {
                                    addIdamError(&idamerrorstack, CODEERRORTYPE, "readCDF", err,
                                                 "Unable to Type Coordinate Domain Increment array");
                                } else {
                                    addIdamError(&idamerrorstack, CODEERRORTYPE, "readCDF", err,
                                                 "The Coordinate Domain representation Increment Attribute's Type is Not Compliant - must be Double!");
                                }
                                break;
                            }

                            if ((err = readCDF4AVar(cgrouplist, cgrpid, coordid, NC_DOUBLE, "increment", &nincrement,
                                                    ndimatt, &type, (char**)&increment, logmalloclist, userdefinedtypelist, &dudt)) != 0) {
                                addIdamError(&idamerrorstack, CODEERRORTYPE, "readCDF", err,
                                             "Unable to Read Coordinate Domain Increment array");
                                break;
                            }

                            // Modify if subsetting required

                            if (cdfsubset.subsetCount > 0 && cdfsubset.subset[ii]) {
                                start[0] = start[0] + (double)cdfsubset.start[ii] * increment[0];
                                count[0] = (int)cdfsubset.count[ii];
                                increment[0] = (double)cdfsubset.stride[ii] * increment[0];
                            }

                            if (ncount == nstart && nstart == nincrement) {
                                data_block->dims[ii].compressed = 1;
                                data_block->dims[ii].method = 1;
                                data_block->dims[ii].data_type = TYPE_DOUBLE;       // Always type DOUBLE
                                data_block->dims[ii].offs = (char*)start;           // Domain Starting Values
                                data_block->dims[ii].ints = (char*)increment;       // Domain Step Increments
                                data_block->dims[ii].udoms = (unsigned int)ncount;  // Number of Domains
                                data_block->dims[ii].sams = (int*)count;            // Domain Lengths

                                if (isUnlimited) {                    // make Consistent with the extent used
                                    unsigned int counter = 0;
                                    data_block->dims[ii].dim_n = extent[i];        // Reduced Array Length
                                    for (j = 0; j < ncount; j++) {
                                        counter = counter + count[j];
                                        if (counter > extent[i]) {
                                            data_block->dims[ii].udoms = (unsigned int)j; // Reduced Number of Domains
                                            count[j] = extent[i] - (counter - count[j]);
                                            data_block->dims[ii].sams = (int*)count; // Reduced Domain Lengths
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
                                                     (char*)nc_strerror(rc));
                                        break;
                                    }
                                    if ((rc = applyCDFCalibration(cgrpid, coordid, data_block->dims[ii].udoms,
                                                                  &data_block->dims[ii].data_type,
                                                                  &data_block->dims[ii].ints)) != NC_NOERR) {
                                        err = 999;
                                        addIdamError(&idamerrorstack, CODEERRORTYPE, "readCDF", err,
                                                     (char*)nc_strerror(rc));
                                        break;
                                    }
                                }

                            } else {
                                if (start != NULL) free((void*)start);
                                if (increment != NULL) free((void*)increment);
                                if (count != NULL) free((void*)count);
                            }
                        }
                    }
                }
            }

            //----------------------------------------------------------------------
            // Read Attribute Values (No Comment attribute: Copy to XML if present)

            comment[0] = '\0';

            err = readCDF4Atts(cgrpid, coordid, data_block->dims[ii].dim_units, data_block->dims[ii].dim_label,
                               classtxt, comment);

            if (err != 0) {
                addIdamError(&idamerrorstack, CODEERRORTYPE, "readCDF", err, "Unable to Read Coordinate Attributes");
                break;
            }

            if (comment[0] != '\0' && getMeta && compliance) {
                sprintf(xml, "<%s>\n<comment>\"", dimname);
                addMetaXML(&metaxml, xml);
                if (strlen(comment) + 2 * strlen(xml) + 2 < STRING_LENGTH) {
                    addMetaXML(&metaxml, comment);
                } else {
                    comment[STRING_LENGTH - 1 - (int)strlen(comment) - 2 * (int)strlen(xml) - 4] = '\0';
                    addMetaXML(&metaxml, comment);
                }
                sprintf(xml, "\"</comment>\n</%s>\n", dimname);
                addMetaXML(&metaxml, xml);
            }

            // Is this the TIME dimension?

            if (compliance) {
                if (STR_EQUALS(classtxt, "time")) data_block->order = ii;    // Yes it is!
            } else {
                if (STR_IEQUALS(data_block->dims[ii].dim_label, "time") ||
                    STR_IEQUALS(data_block->dims[ii].dim_label, "time3")) {
                        data_block->order = ii;
                }    // Simple (& very poor) test for Time Dimension!

                if (!isIndex) {
                    if (strcmp(dimname, data_block->dims[ii].dim_label)) {    // Add Var Name to Label (Should be a Dimension)
                        lstr = (int)strlen(dimname) + (int)strlen(data_block->dims[ii].dim_label) + 3;
                        if (lstr <= STRING_LENGTH) {
                            strcat(data_block->dims[ii].dim_label, " [");
                            strcat(data_block->dims[ii].dim_label, dimname);    // Only if Different to Dimension Name
                            strcat(data_block->dims[ii].dim_label, "]");
                            LeftTrimString(data_block->dims[ii].dim_label);
                        }
                    }
                } else {
                    lstr = (int)strlen(dimname) + (int)strlen(data_block->dims[ii].dim_label) + 51;
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
                                       &data_block->dims[ii].error_type, &data_block->dims[ii].errhi, logmalloclist,
                                       userdefinedtypelist)) != 0) {
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
        //----------------------------------------------------------------------
        // Opaque Structure: XML Text
        //
        if (getMeta) {

            addMetaXML(&metaxml, "\n</coordinates>");
            if (compliance) addMetaXML(&metaxml, "\n</netcdf-4>\n");

            data_block->opaque_block = (void*)metaxml.xml;
            data_block->opaque_count = metaxml.nxml;
            data_block->opaque_type = OPAQUE_TYPE_XML_DOCUMENT;
        }

        //----------------------------------------------------------------------
        // End of Error Trap Loop

    } while (0);

    //----------------------------------------------------------------------
    // Housekeeping

    if (err != 0 && metaxml.xml != NULL) {
        free((void*)metaxml.xml);
        data_block->opaque_block = NULL;
        data_block->opaque_count = 0;
        data_block->opaque_type = OPAQUE_TYPE_UNKNOWN;
    }

    if (closexml.xml != NULL) free((void*)closexml.xml);

    if (grpids != NULL) free((void*)grpids);
    if (dimids != NULL) free((void*)dimids);
    if (extent != NULL) free((void*)extent);
    if (dextent != NULL) free((void*)dextent);

    IDAM_LOG(UDA_LOG_DEBUG, "NC File Closed\n");
    ncclose(fd);        // Close netCDF File

    gettimeofday(&tv_end0, NULL);
    IDAM_LOGF(UDA_LOG_DEBUG, "\n\nTotal Time: %.2f (ms)\n\n", (float)(tv_end0.tv_sec - tv_start0.tv_sec) * 1.0E3 +
                                                          (float)(tv_end0.tv_usec - tv_start0.tv_usec) *
                                                          1.0E-3);

    return err;
}


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

    ncids = (int*)malloc(sizeof(int) * numgrps);

    if ((err = nc_inq_grps(ncgrpid, &numgrps, ncids)) != NC_NOERR) {
        free((void*)ncids);
        return err;
    }

// Test Child Group Names against Target Group Name

    for (i = 0; i < numgrps; i++) {

        if ((err = nc_inq_grpname_len(ncids[i], &namelength)) != NC_NOERR) {
            if (grpname != NULL) free((void*)grpname);
            free((void*)ncids);
            return err;
        }

        grpname = (char*)realloc((void*)grpname, sizeof(char) * (namelength + 1));

        if ((err = nc_inq_grpname(ncids[i], grpname)) != NC_NOERR) {
            free((void*)grpname);
            free((void*)ncids);
            return err;
        }

        if (STR_EQUALS(grpname, target)) {
            *targetid = ncids[i];            // Found - it exists!
            free((void*)grpname);
            free((void*)ncids);
            return (NC_NOERR);
        }

    }

    free((void*)grpname);
    free((void*)ncids);

    return 999;
}

#endif // !defined NONETCDFPLUGIN
