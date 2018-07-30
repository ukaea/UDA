// Read netCDF4 variable data (both regular and user defined types)
//--------------------------------------------------------------------------

#include "readCDFVar.h"

#include <netcdf.h>
#include <stdlib.h>

#include <clientserver/errorLog.h>
#include <clientserver/stringUtils.h>
#include <logging/logging.h>
#include <server/udaServer.h>
#include <structures/struct.h>

#include "readCDFTypes.h"
#include "readCDFStructs.h"
#include "readCDF4.h"

int scopedUserDefinedTypes(int grpid)
{
    USERDEFINEDTYPELIST* userdefinedtypelist = (USERDEFINEDTYPELIST*)malloc(sizeof(USERDEFINEDTYPELIST));
    initUserDefinedTypeList(userdefinedtypelist);

    userdefinedtypelist->listCount = parseduserdefinedtypelist.listCount;    // Copy the standard set of structure definitions
    userdefinedtypelist->userdefinedtype = (USERDEFINEDTYPE*)malloc(
            userdefinedtypelist->listCount * sizeof(USERDEFINEDTYPE));

    int i;
    for (i = 0; i < userdefinedtypelist->listCount; i++) {
        USERDEFINEDTYPE usertypeOld = parseduserdefinedtypelist.userdefinedtype[i];
        USERDEFINEDTYPE usertypeNew;
        initUserDefinedType(&usertypeNew);
        usertypeNew = usertypeOld;
        // Copy pointer type (prevents double free)
        usertypeNew.image = (char*)malloc(usertypeOld.imagecount * sizeof(char));
        memcpy(usertypeNew.image, usertypeOld.image, usertypeOld.imagecount);

        usertypeNew.compoundfield = (COMPOUNDFIELD*)malloc(usertypeOld.fieldcount * sizeof(COMPOUNDFIELD));

        int j;
        for (j = 0; j < usertypeOld.fieldcount; j++) {
            initCompoundField(&usertypeNew.compoundfield[j]);
            usertypeNew.compoundfield[j] = usertypeOld.compoundfield[j];
            if (usertypeOld.compoundfield[j].rank > 0) {
                usertypeNew.compoundfield[j].shape = (int*)malloc(usertypeOld.compoundfield[j].rank * sizeof(int));

                int k;
                for (k = 0; k < usertypeOld.compoundfield[j].rank; k++) {
                    usertypeNew.compoundfield[j].shape[k] = usertypeOld.compoundfield[j].shape[k];
                }
            }
        }
        userdefinedtypelist->userdefinedtype[i] = usertypeNew;
    }

    return readCDFTypes(grpid, userdefinedtypelist);
}

void replaceStrings(char** svec, int* ndata, char** dvec, int* ndims)
{
    // String heap is allocated within netcdf/hdf5
    // This needs to be replaced with locally allocated heap and freed
    // Returns a single block with fixed length allocations for individual strings

    int i, lstr, sMax = 0;
    for (i = 0; i < *ndata; i++) if ((lstr = (int)strlen(svec[i])) > sMax) sMax = lstr + 1;
    char* data = (char*)malloc(*ndata * sMax * sizeof(char));
    for (i = 0; i < *ndata; i++)strcpy(&data[i * sMax], svec[i]);
    nc_free_string(*ndata, svec);
    free(svec);

    ndims[1] = sMax;
    ndims[0] = *ndata;

    *ndata = *ndata * sMax;
    *dvec = data;
}

void replaceEmbeddedStrings(LOGMALLOCLIST* logmalloclist, USERDEFINEDTYPELIST* userdefinedtypelist,
                            USERDEFINEDTYPE* udt, int ndata, char* dvec)
{
    // If the data structure has a User Defined Type with a NC_STRING component,
    // or contains other user defined types with string components,
    // then intercept and replace with locally allocated strings.

    // VLEN types also need heap replacement

    int i, j, k, nstr;

    if (udt == NULL) return;

    for (j = 0; j < udt->fieldcount; j++) {

        if (udt->compoundfield[j].atomictype == UDA_TYPE_UNKNOWN) {    // Child User Defined type?

            char* data;
            USERDEFINEDTYPE* child = findUserDefinedType(userdefinedtypelist, udt->compoundfield[j].type, 0);
            nstr = udt->compoundfield[j].count;            // Number of sub-structures

            if (udt->idamclass == UDA_TYPE_VLEN) {
                VLENTYPE* vlen = (VLENTYPE*)dvec;            // If the type is VLEN then read data array for the count
                for (k = 0; k < ndata; k++) {
                    replaceEmbeddedStrings(logmalloclist, userdefinedtypelist, child, vlen[k].len, vlen[k].data);
                }

                // Intercept VLEN netcdf/hdf5 heap and allocate local heap;

                void* vlendata;
                for (k = 0; k < ndata; k++) {
                    vlendata = malloc(vlen[k].len * child->size);
                    addMalloc(logmalloclist, vlendata, vlen[k].len, child->size, child->name);
                    memcpy(vlendata, vlen[k].data, vlen[k].len * child->size);
                    free(vlen[k].data);                // Free netcdf/hdf5 allocated heap
                    vlen[k].data = vlendata;
                }

            } else {
                for (k = 0; k < ndata; k++) {                    // Loop over each structure array element
                    data = &dvec[k * udt->size] + udt->compoundfield[j].offset * sizeof(char);
                    replaceEmbeddedStrings(logmalloclist, userdefinedtypelist, child, nstr, data);
                }
            }
            continue;
        }

        if (STR_EQUALS(udt->compoundfield[j].type, "STRING *")) {

            // String arrays within data structures are defined (readCDFTypes) as char *str[int] => rank=1, pointer=0 and type=STRING*

            int lstr;
            char** svec;
            char** ssvec;

            nstr = udt->compoundfield[j].count;        // Number of strings

            // Allocate new local heap for each structure array element
            // new heap is freed via the malloc log

            ssvec = (char**)malloc((size_t)nstr * sizeof(char*));

            for (k = 0; k < ndata; k++) {                // Loop over each structure array element

                svec = (char**)(&dvec[k * udt->size] + udt->compoundfield[j].offset * sizeof(char));

                for (i = 0; i < nstr; i++) {
                    lstr = (int)strlen(svec[i]) + 1;
                    ssvec[i] = (char*)malloc(lstr * sizeof(char));
                    addMalloc(logmalloclist, (void*)ssvec[i], lstr, sizeof(char), "char");
                    strcpy(ssvec[i], svec[i]);
                }
                nc_free_string(nstr, svec);            // Free netCDF/HDF5 heap
                for (i = 0; i < nstr; i++) {
                    svec[i] = ssvec[i];
                }
            }
            free((void*)ssvec);
            continue;
        }
    }
}

int readCDF4Var(GROUPLIST grouplist, int varid, int isCoordinate, int rank, int* dimids,
                unsigned int* extent, int* ndvec, int* data_type, int* isIndex, char** data,
                LOGMALLOCLIST* logmalloclist, USERDEFINEDTYPELIST* userdefinedtypelist, USERDEFINEDTYPE** udt)
{
    int grpid;

    int i, rc, err = 0, ndata, isextent = 0, createIndex = 0, coordSubsetId = -1;

    int attid;
    nc_type vartype, atttype;                    // NC types
    char dimname[NC_MAX_NAME + 1];
    char* dvec = NULL;
    char** svec = NULL;                    // String array

    size_t* start = NULL, * count = NULL;
    size_t attlen;
    size_t* dnums = NULL;            // dgm changed from int to size_t 16Dec11

    *udt = NULL;                        // Definition of User Defined Data Structures

    //----------------------------------------------------------------------
    // Default Group ID

    grpid = grouplist.grpid;

    //----------------------------------------------------------------------
    // Test for Extent Attribute (which means 1 or more dimensions are unlimited)
    // An unlimited dimension can be ZERO!

    if (!isCoordinate) {

        if (nc_inq_attid(grpid, varid, "extent", &attid) == NC_NOERR) {        // Test Attribute Extent Exists

            if ((rc = nc_inq_att(grpid, varid, "extent", &atttype, &attlen)) != NC_NOERR) {
                err = NETCDF_ERROR_INQUIRING_DIM_3;
                addIdamError(CODEERRORTYPE, "readCDFVar", err, (char*)nc_strerror(rc));
                return err;
            }

            if (atttype != NC_UINT) {
                err = 999;
                addIdamError(CODEERRORTYPE, "readCDFVar", err,
                             "Extent attribute of Variable is Not an Unsigned Integer");
                return err;
            }

            if (attlen != rank && rank > 0) {
                err = 999;
                addIdamError(CODEERRORTYPE, "readCDFVar", err,
                             "Extent attribute of Variable has an Inconsistent Rank");
                return err;
            }

            if ((rc = nc_get_att_uint(grpid, varid, "extent", extent)) != NC_NOERR) {
                err = NETCDF_ERROR_INQUIRING_DIM_3;
                addIdamError(CODEERRORTYPE, "readCDFVar", err, (char*)nc_strerror(rc));
                free((void*)extent);
                return err;
            }

            isextent = 1;

        }
    }

    //----------------------------------------------------------------------
    // Error Trap

    do {

        //----------------------------------------------------------------------
        // If the variable is a coordinate array and the rank == 2 (legacy!) then
        // only read the first dimension: a netcdf error occurs trying to read the second.
        // Test that this dimension is consistent with the length expectation (passed as extent[0]).
        // If the rank is higher than 2 then return an index value (the user needs to substitute coordinate data themselves)
        //
        // Testing the coordinate array is constant along the second dimension.
        // Must be attached to the same group.
        //----------------------------------------------------------------------
        // Shape of the Data and Error Arrays from the dimensions and the used extents

        ndata = 1;                        // Single Scalar Value if Rank == 0

        if (rank > 0) {

            start = (size_t*)malloc(rank * sizeof(size_t));
            count = (size_t*)malloc(rank * sizeof(size_t));
            dnums = (size_t*)malloc(rank * sizeof(size_t));

            if (isCoordinate && rank > 1 && cdfsubset.subsetCount > 0) {
                err = 999;
                addIdamError(CODEERRORTYPE, "readCDFVar", err,
                             "Unable to Subset a Coordinate Variable with a Rank >= 2!");
                break;
            }

            if (start == NULL || count == NULL || dnums == NULL) {
                err = 999;
                addIdamError(CODEERRORTYPE, "readCDFVar", err, "Unable to Allocate Heap for Shape Arrays");
                break;
            }

            for (i = 0; i < rank; i++) {

                if (isextent) {
                    dnums[i] = (size_t)extent[i];                    // Number of Dimension Coordinates
                } else {
                    if ((rc = nc_inq_dim(grpid, dimids[i], dimname, &dnums[i])) != NC_NOERR) {
                        err = NETCDF_ERROR_INQUIRING_DIM_3;
                        addIdamError(CODEERRORTYPE, "readCDFVar", err, (char*)nc_strerror(rc));
                        break;
                    }

                    if (isCoordinate && rank > 1 && cdfsubset.subsetCount == 0) {
                        if ((unsigned int)dnums[0] != extent[0]) {
                            createIndex = 1;
                        }        // Expect this length
                        break;                        // nc_inq_dim will return an error if the next dimension is queried!
                    } else {
                        extent[i] = (unsigned int)dnums[i];                    // retain shape for pass back
                    }
                }

                // Adjust for native sub-setting

                if (cdfsubset.subsetCount > 0) {
                    if (!isCoordinate) {
                        if (!cdfsubset.subset[i]) {
                            cdfsubset.start[i] = 0;            // Fill dimensions if not subset
                            cdfsubset.stop[i] = dnums[i] - 1;
                            cdfsubset.count[i] = dnums[i];
                            cdfsubset.stride[i] = 1;
                        }
                        if (cdfsubset.subset[i]) {
                            if ((int)cdfsubset.stop[i] == -1) {
                                cdfsubset.stop[i] = (size_t)(dnums[i] - 1);
                            } else {
                                if (cdfsubset.stop[i] > (size_t)(dnums[i] - 1)) {
                                    err = 999;
                                    addIdamError(CODEERRORTYPE, "readCDFVar", err,
                                                 "Invalid Stop Index in subset operation");
                                    break;
                                }
                            }
                            if ((int)cdfsubset.count[i] == -1) {
                                cdfsubset.count[i] = cdfsubset.stop[i] - cdfsubset.start[i] + 1;
                                if (cdfsubset.stride[i] > 1 && cdfsubset.count[i] > 1) {
                                    if ((cdfsubset.count[i] % cdfsubset.stride[i]) > 0) {
                                        cdfsubset.count[i] = 1 + cdfsubset.count[i] / cdfsubset.stride[i];
                                    } else {
                                        cdfsubset.count[i] = cdfsubset.count[i] / cdfsubset.stride[i];
                                    }
                                }
                            }
                            dnums[i] = (int)cdfsubset.count[i];
                            extent[i] = (unsigned int)dnums[i];
                        }
                    } else {
                        int j;
                        for (j = 0; j < cdfsubset.rank; j++) {        // Identify the coordinate variable's dimension ID
                            if (cdfsubset.dimids[j] == dimids[0]) {
                                if ((int)cdfsubset.stop[j] == -1) {
                                    cdfsubset.stop[j] = (size_t)(dnums[0] - 1);
                                } else {
                                    if (cdfsubset.stop[j] > (size_t)(dnums[0] - 1)) {
                                        err = 999;
                                        addIdamError(CODEERRORTYPE, "readCDFVar", err,
                                                     "Invalid Stop Index in subset operation");
                                        break;
                                    }
                                }
                                if ((int)cdfsubset.count[j] == -1) {
                                    cdfsubset.count[j] = cdfsubset.stop[j] - cdfsubset.start[j] + 1;
                                    if (cdfsubset.stride[j] > 1 && cdfsubset.count[j] > 1) {
                                        if ((cdfsubset.count[j] % cdfsubset.stride[j]) > 0) {
                                            cdfsubset.count[j] = 1 + cdfsubset.count[j] / cdfsubset.stride[j];
                                        } else {
                                            cdfsubset.count[j] = cdfsubset.count[j] / cdfsubset.stride[j];
                                        }
                                    }
                                }
                                dnums[i] = (int)cdfsubset.count[j];
                                extent[i] = (unsigned int)dnums[i];
                                coordSubsetId = j;
                                break;
                            } else {
                                dnums[i] = 1;
                                extent[i] = 1;
                            }
                        }
                    }
                }

                ndata = ndata * (int)dnums[i];                // Total Count of Array Elements
                start[i] = 0;
                count[i] = (int)dnums[i];
            }

            if (err != 0) break;

            if (isCoordinate && rank > 1 && cdfsubset.subsetCount == 0) {
                for (i = 0; i < rank; i++) {
                    start[i] = 0;
                    count[i] = 1;
                }
                ndata = (int)dnums[0];
                count[1] = (int)dnums[0];            // Slice the coordinate array: make rank 1
            }

        } else {
            if (!isextent) extent[0] = 1;            // Single Scalar value
        }

        //----------------------------------------------------------------------
        // Data Array Type

        if ((rc = nc_inq_vartype(grpid, varid, &vartype)) != NC_NOERR) {
            err = NETCDF_ERROR_INQUIRING_VARIABLE_3;
            addIdamError(CODEERRORTYPE, "readCDFVar", err, (char*)nc_strerror(rc));
            break;
        }

        //----------------------------------------------------------------------
        // Read Data and Identify the corresponding IDAM type

        // Allocate heap

        switch (vartype) {
            case NC_DOUBLE: {
                *data_type = UDA_TYPE_DOUBLE;
                dvec = (char*)malloc((size_t)ndata * sizeof(double));
                break;
            }
            case NC_FLOAT: {
                *data_type = UDA_TYPE_FLOAT;
                dvec = (char*)malloc((size_t)ndata * sizeof(float));
                break;
            }
            case NC_INT64: {
                *data_type = UDA_TYPE_LONG64;
                dvec = (char*)malloc((size_t)ndata * sizeof(long long int));
                break;
            }
            case NC_INT: {
                *data_type = UDA_TYPE_INT;
                dvec = (char*)malloc((size_t)ndata * sizeof(int));
                break;
            }
            case NC_SHORT: {
                *data_type = UDA_TYPE_SHORT;
                dvec = (char*)malloc((size_t)ndata * sizeof(short));
                break;
            }
            case NC_BYTE: {
                *data_type = UDA_TYPE_CHAR;
                dvec = (char*)malloc((size_t)ndata * sizeof(char));
                break;
            }
            case NC_UINT64: {
                *data_type = UDA_TYPE_UNSIGNED_LONG64;
                dvec = (char*)malloc((size_t)ndata * sizeof(unsigned long long int));
                break;
            }
            case NC_UINT: {
                *data_type = UDA_TYPE_UNSIGNED_INT;
                dvec = (char*)malloc((size_t)ndata * sizeof(unsigned int));
                break;
            }
            case NC_USHORT: {
                *data_type = UDA_TYPE_UNSIGNED_SHORT;
                dvec = (char*)malloc((size_t)ndata * sizeof(unsigned short));
                break;
            }
            case NC_UBYTE: {
                *data_type = UDA_TYPE_UNSIGNED_CHAR;
                dvec = (char*)malloc((size_t)ndata * sizeof(unsigned char));
                break;
            }
            case NC_CHAR: {
                // *data_type = UDA_TYPE_CHAR;
                *data_type = UDA_TYPE_STRING;
                if (IMAS_HDF_READER) {
                    dvec = (char*)malloc((size_t)(ndata * 132 + 1) * sizeof(char));
                    // IMAS HDF5 strings are written in blocks of 132 bytes + string terminator
                } else {
                    dvec = (char*)malloc((size_t)ndata * sizeof(char));
                }
                break;
            }
            case NC_STRING: {
                *data_type = UDA_TYPE_STRING;                    // Treated as a byte/char array
                svec = (char**)malloc((size_t)ndata * sizeof(char*)); // Array of pointers to string array elements
                break;
            }

                //----------------------------------------------------------------------
                // Must be a user defined type: COMPLEX, OPAQUE, VLEN, COMPOUND and ENUMERATED

            default: {
                if (isCoordinate) {    // User Defined type coordinates are Not Enabled: swap for a local index
                    *data_type = UDA_TYPE_INT;
                    dvec = (char*)malloc((size_t)ndata * sizeof(int));
                    createIndex = 1;
                    break;
                }

                if (vartype == ctype) {                        // known User Defined types
                    *data_type = UDA_TYPE_COMPLEX;
                    dvec = (char*)malloc((size_t)ndata * sizeof(COMPLEX));
                } else {
                    if (vartype == dctype) {
                        *data_type = UDA_TYPE_DCOMPLEX;
                        dvec = (char*)malloc((size_t)ndata * sizeof(DCOMPLEX));
                    } else {

                        // Create User defined data structure definitions: descriptions of the internal composition (Global Structure)

                        if ((err = readCDFTypes(grpid, userdefinedtypelist)) != 0) break;

                        // Identify the required structure definition

                        // Identify via type id (assuming local to this group)
                        *udt = findUserDefinedType(userdefinedtypelist, "", (int)vartype);

                        if (*udt == NULL && grouplist.grpids != NULL) {
                            // Must be defined elsewhere within scope of this group
                            int igrp;
                            for (igrp = 0; igrp < grouplist.count; igrp++) {
                                // Extend list to include all groups in scope
                                if (grouplist.grpids[igrp] != grpid) {
                                    if ((err = readCDFTypes(grouplist.grpids[igrp], userdefinedtypelist)) != 0) break;
                                }
                            }
                            if (err != 0) break;
                            // Should be found now if within scope
                            *udt = findUserDefinedType(userdefinedtypelist, "", (int)vartype);
                        }

                        if (*udt == NULL) {
                            err = 999;
                            addIdamError(CODEERRORTYPE, "readCDFVar", err,
                                         "User Defined Type definition Not within Scope!");
                            break;
                        }

                        // Read the Data - All user defined structures are COMPOUND: No special treatment for VLEN etc.
                        // Check for consistency with the structure definitions

                        *data_type = UDA_TYPE_COMPOUND;

                        if ((*udt)->idamclass == UDA_TYPE_ENUM) {
                            dvec = (char*)malloc(ndata * sizeof(long long)); // Sufficient space ignoring integer type
                        } else {
                            dvec = (char*)malloc(ndata * (size_t)(*udt)->size);        // Create heap for the data
                        }

                        break;
                    }

                }

            }
                break;

        }

        if (err != 0) {
            break;
        }

        if (ndata == 0) {
            free((void*)dvec);
            dvec = NULL;
        } else {
            if (dvec == NULL && vartype != NC_STRING) {
                err = NETCDF_ERROR_ALLOCATING_HEAP_6;
                addIdamError(CODEERRORTYPE, "readCDFVar", err, "Unable to Allocate Heap Memory for the Data");
                break;
            }
        }

        if (ndata > 0) {
            if (!createIndex) {

                if (rank > 0) {
                    if (cdfsubset.subsetCount == 0) {

                        if (vartype == NC_STRING) {
                            if (rank > 1) {
                                err = 999;
                                addIdamError(CODEERRORTYPE, "readCDFVar", err,
                                             "String Array rank is too large - must be 1");
                                break;
                            }

                            rc = nc_get_var_string(grpid, varid, svec);

                            // Pack the strings for compliance with the legacy middleware
                            //void replaceStrings(char **svec, int *ndata, char **dvec, int *ndims){

                            replaceStrings(svec, &ndata, &dvec, (int*)extent);

                        } else {
                            rc = nc_get_vara(grpid, varid, start, count, (void*)dvec);
                            // If the data has a User Defined Type with a NC_STRING component then intercept and replace with a locally allocated component

                            replaceEmbeddedStrings(logmalloclist, userdefinedtypelist, *udt, ndata, dvec);
                        }

                    } else {
                        if (vartype == NC_STRING) {
                            err = 999;
                            addIdamError(CODEERRORTYPE, "readCDFVar", err,
                                         "Subsetting of String array is Not implemented!");
                            break;
                        }

                        if (!isCoordinate) {
                            rc = nc_get_vars(grpid, varid, cdfsubset.start, cdfsubset.count, cdfsubset.stride,
                                             (void*)dvec);
                        } else {
                            if (cdfsubset.subset[coordSubsetId]) {
                                rc = nc_get_vars(grpid, varid, &cdfsubset.start[coordSubsetId],
                                                 &cdfsubset.count[coordSubsetId],
                                                 &cdfsubset.stride[coordSubsetId], (void*)dvec);
                            } else {
                                rc = nc_get_vara(grpid, varid, start, count, (void*)dvec);
                            }
                        }
                    }
                } else {
                    if (ndata == 1) {
                        size_t index[] = { 0 };
                        rc = nc_get_var1(grpid, varid, index, (void*)dvec);
                    } else {
                        rc = nc_get_var(grpid, varid, (void*)dvec);
                    }
                }

                if (rc != NC_NOERR) {
                    err = NETCDF_ERROR_INQUIRING_VARIABLE_10;
                    addIdamError(CODEERRORTYPE, "readCDFVar", err, (char*)nc_strerror(rc));
                    break;
                }

                if (*udt != NULL) {

                    if ((*udt)->idamclass == UDA_TYPE_COMPOUND) {            // Compound Types
                        addMalloc(logmalloclist, dvec, ndata, (size_t)(*udt)->size,
                                  (*udt)->name);    // Free Data via Malloc Log List
                    }

                    if ((*udt)->idamclass == UDA_TYPE_ENUM) {        // Enumerated Types
                        int i;
                        char value[8];
                        ENUMLIST* enumlist = (ENUMLIST*)malloc(sizeof(ENUMLIST));
                        nc_type base;
                        size_t size, members;
                        char name[NC_MAX_NAME + 1];

                        if ((rc = nc_inq_enum(grpid, vartype, name, &base, &size, &members)) != NC_NOERR) {
                            err = 999;
                            addIdamError(CODEERRORTYPE, "readCDFVar", err, (char*)nc_strerror(rc));
                            break;
                        }

                        if (members == 0) {
                            err = 999;
                            addIdamError(CODEERRORTYPE, "readCDFVar", err,
                                         "Enumerated Type has Zero Membership?");
                            break;
                        }

                        strcpy(enumlist->name, name);
                        enumlist->type = convertNCType(base);
                        enumlist->count = (int)members;
                        enumlist->enummember = (ENUMMEMBER*)malloc(members * sizeof(ENUMMEMBER));

                        for (i = 0; i < members; i++) {
                            nc_inq_enum_member(grpid, vartype, i, enumlist->enummember[i].name, (void*)value);
                            switch (enumlist->type) {
                                case UDA_TYPE_CHAR: {
                                    char* enumvalue = (char*)value;
                                    enumlist->enummember[i].value = (long long)*enumvalue;
                                    break;
                                }
                                case UDA_TYPE_SHORT: {
                                    short* enumvalue = (short*)value;
                                    enumlist->enummember[i].value = (long long)*enumvalue;
                                    break;
                                }
                                case UDA_TYPE_UNSIGNED_SHORT: {
                                    unsigned short* enumvalue = (unsigned short*)value;
                                    enumlist->enummember[i].value = (long long)*enumvalue;
                                    break;
                                }
                                case UDA_TYPE_INT: {
                                    int* enumvalue = (int*)value;
                                    enumlist->enummember[i].value = (long long)*enumvalue;
                                    break;
                                }
                                case UDA_TYPE_UNSIGNED_INT: {
                                    unsigned int* enumvalue = (unsigned int*)value;
                                    enumlist->enummember[i].value = (long long)*enumvalue;
                                    break;
                                }
                                case UDA_TYPE_LONG64: {
                                    long long* enumvalue = (long long*)value;
                                    enumlist->enummember[i].value = (long long)*enumvalue;
                                    break;
                                }
                                case UDA_TYPE_UNSIGNED_LONG64: {
                                    unsigned long long* enumvalue = (unsigned long long*)value;
                                    enumlist->enummember[i].value = (long long)*enumvalue;
                                    break;
                                }
                            }
                        }

                        //enumlist->data = (void*)dvec;
			
		        // Change to standard unsigned long long integer type		    
		        int j;
		        unsigned long long* enumarray = (unsigned long long*)malloc(ndata*sizeof(unsigned long long));
		        switch (enumlist->type) {
                           case UDA_TYPE_CHAR: {
                              char* data = (char*)dvec;
                              for (j = 0; j < ndata; j++) enumarray[j] = (unsigned long long)data[j];
                              break;
                           }
                           case UDA_TYPE_SHORT: {
                              short* data = (short*)dvec;
                              for (j = 0; j < ndata; j++) enumarray[j] = (unsigned long long)data[j];
                              break;
                           }
                           case UDA_TYPE_UNSIGNED_SHORT: {
                              unsigned short* data = (unsigned short*)dvec;
                              for (j = 0; j < ndata; j++) enumarray[j] = (unsigned long long)data[j];
                              break;
                           }
                           case UDA_TYPE_INT: {
                              int* data = (int*)dvec;
                              for (j = 0; j < ndata; j++) enumarray[j] = (unsigned long long)data[j];
                              break;
                           }
                           case UDA_TYPE_UNSIGNED_INT: {
                              unsigned int* data = (unsigned int*)dvec;
                              for (j = 0; j < ndata; j++) enumarray[j] = (unsigned long long)data[j];
                              break;
                           }
                           case UDA_TYPE_LONG64: {
                              long long* data = (long long*)dvec;
                              for (j = 0; j < ndata; j++) enumarray[j] = (unsigned long long)data[j];
                              break;
                           }
                           case UDA_TYPE_UNSIGNED_LONG64: {
                              unsigned long long* data = (unsigned long long*)dvec;
                              for (j = 0; j < ndata; j++) enumarray[j] = (unsigned long long)data[j];
                              break;
                           }
                        }
		        enumlist->enumarray = (void*)enumarray;
		        free((void *)dvec);

                        dvec = (char*)enumlist;

                        // Add mallocs to list for freeing when data dispatch complete

                        addMalloc(logmalloclist, (void*)enumlist, 1, sizeof(ENUMLIST), "ENUMLIST");
                        addMalloc(logmalloclist, (void*)enumlist->enummember, (int)members, sizeof(ENUMMEMBER),
                                  "ENUMMEMBER");
                        //addMalloc(logmalloclist, enumlist->data, ndata, (int)size,
                        //          idamNameType(idamAtomicType(base)));            // Use true integer type
                        addMalloc(logmalloclist, (void*)enumlist->enumarray, ndata, sizeof(unsigned long long), "unsigned long long");  
                		     
                        enumlist->enumarray_rank = 1;
                        enumlist->enumarray_count = ndata;
                        enumlist->enumarray_shape = (int *)malloc(sizeof(int));
                        enumlist->enumarray_shape[0] = ndata;        
                        addMalloc(logmalloclist, (void*)enumlist->enumarray_shape, 1, sizeof(int), "int");

                        // Add mallocs freed through a separate process

                        //addNonMalloc((void *) enumlist, 1, sizeof(ENUMLIST), "ENUMLIST");
                    }
                }

            } else {
                *isIndex = 1;
                dvec = (char*)realloc((void*)dvec, ndata * sizeof(int));
                readCDF4CreateIndex(ndata, dvec);
                *data_type = UDA_TYPE_INT;
            }
        }

        *data = dvec;            // Data Array
        *ndvec = ndata;            // Size

        //----------------------------------------------------------------------
        // End of Error Trap

    } while (0);

    //----------------------------------------------------------------------
    // Housekeeping

    if (start != NULL) free((void*)start);
    if (count != NULL) free((void*)count);
    if (dnums != NULL) free((void*)dnums);
    if (err != 0 && dvec != NULL) free((void*)dvec);

    return err;
}

int readCDF4AVar(GROUPLIST grouplist, int grpid, int varid, nc_type atttype, char* name, int* ndvec, int ndims[2],
                 int* data_type, char** data, LOGMALLOCLIST* logmalloclist, USERDEFINEDTYPELIST* userdefinedtypelist,
                 USERDEFINEDTYPE** udt)
{

    int rc, err = 0, ndata;
    char* dvec = NULL;
    char** svec = NULL;                    // String array

    size_t attlen;

    *udt = NULL;                        // Definition of User Defined Data Structures

    //----------------------------------------------------------------------
    // Error Trap

    do {

        //----------------------------------------------------------------------
        // Length of the Data

        if ((rc = nc_inq_attlen(grpid, varid, name, &attlen)) != NC_NOERR) {
            err = 999;
            addIdamError(CODEERRORTYPE, "readCDFAVar", err, (char*)nc_strerror(rc));
            break;
        }

        ndata = (int)attlen;

        //----------------------------------------------------------------------
        // Read Data and Identify the corresponding IDAM type

        switch (atttype) {

            case NC_DOUBLE: {
                *data_type = UDA_TYPE_DOUBLE;
                dvec = (char*)malloc((size_t)ndata * sizeof(double));
                break;
            }

            case NC_FLOAT: {
                *data_type = UDA_TYPE_FLOAT;
                dvec = (char*)malloc((size_t)ndata * sizeof(float));
                break;
            }

            case NC_INT64: {
                *data_type = UDA_TYPE_LONG64;
                dvec = (char*)malloc((size_t)ndata * sizeof(long long int));
                break;
            }

            case NC_INT: {
                *data_type = UDA_TYPE_INT;
                dvec = (char*)malloc((size_t)ndata * sizeof(int));
                break;
            }

            case NC_SHORT: {
                *data_type = UDA_TYPE_SHORT;
                dvec = (char*)malloc((size_t)ndata * sizeof(short));
                break;
            }

            case NC_BYTE: {
                *data_type = UDA_TYPE_CHAR;
                dvec = (char*)malloc((size_t)ndata * sizeof(char));
                break;
            }

            case NC_UINT64: {
                *data_type = UDA_TYPE_UNSIGNED_LONG64;
                dvec = (char*)malloc((size_t)ndata * sizeof(unsigned long long int));
                break;
            }

            case NC_UINT: {
                *data_type = UDA_TYPE_UNSIGNED_INT;
                dvec = (char*)malloc((size_t)ndata * sizeof(unsigned int));
                break;
            }

            case NC_USHORT: {
                *data_type = UDA_TYPE_UNSIGNED_SHORT;
                dvec = (char*)malloc((size_t)ndata * sizeof(unsigned short));
                break;
            }

            case NC_UBYTE: {
                *data_type = UDA_TYPE_UNSIGNED_CHAR;
                dvec = (char*)malloc((size_t)ndata * sizeof(unsigned char));
                break;
            }

            case NC_CHAR: {            // String Attribute
                *data_type = UDA_TYPE_STRING;
                ndata = ndata + 1;
                dvec = (char*)malloc((size_t)ndata * sizeof(char));
                dvec[ndata - 1] = '\0';            // Add the String Null termination
                break;
            }

            case NC_STRING: {
                *data_type = UDA_TYPE_STRING;                    // Treated as a byte/char array
                svec = (char**)malloc(
                        (size_t)ndata * sizeof(char*));        // Array of pointers to string array elements
                break;
            }

            default: {
                if (atttype == ctype) {
                    *data_type = UDA_TYPE_COMPLEX;
                    dvec = (char*)malloc((size_t)ndata * sizeof(COMPLEX));
                    break;
                } else {
                    if (atttype == dctype) {
                        *data_type = UDA_TYPE_DCOMPLEX;
                        dvec = (char*)malloc((size_t)ndata * sizeof(DCOMPLEX));
                        break;
                    } else {

                        // Attribute is a User Defined type

                        // Create User defined data structure definitions: descriptions of the internal composition (Global Structure)

                        if ((err = readCDFTypes(grpid, userdefinedtypelist)) != 0) break;

                        // Identify the required structure definition

                        *udt = findUserDefinedType(userdefinedtypelist, "",
                                                   (int)atttype);        // Identify via type id (assuming local to this group)

                        if (*udt == NULL &&
                            grouplist.grpids != NULL) {        // Must be defined elsewhere within scope of this group
                            int igrp;
                            for (igrp = 0;
                                 igrp < grouplist.count; igrp++) {        // Extend list to include all groups in scope
                                if (grouplist.grpids[igrp] != grpid) {
                                    if ((err = readCDFTypes(grouplist.grpids[igrp], userdefinedtypelist)) != 0) break;
                                }
                            }
                            if (err != 0) break;
                            *udt = findUserDefinedType(userdefinedtypelist, "",
                                                       (int)atttype);        // Should be found now if within scope
                        }

                        if (*udt == NULL) {
                            err = 999;
                            addIdamError(CODEERRORTYPE, "readCDFAVar", err,
                                         "Unknown Data Type: User Defined Type definition Not within Scope!");
                            break;
                        }

                        // Read the Data - All user defined structures are COMPOUND: No special treatment for VLEN etc.
                        // Check for consistency with the structure definitions

                        *data_type = UDA_TYPE_COMPOUND;

                        if ((*udt)->idamclass == UDA_TYPE_ENUM) {
                            dvec = (char*)malloc(
                                    ndata * sizeof(long long));    // Sufficient space ignoring integer type
                        } else {
                            dvec = (char*)malloc(ndata * (size_t)(*udt)->size);        // Create heap for the data
                        }

                        break;
                    }
                }
                break;
            }
        }

        if (err != 0) {
            break;
        }

        if (dvec == NULL && atttype != NC_STRING) {
            err = NETCDF_ERROR_ALLOCATING_HEAP_6;
            addIdamError(CODEERRORTYPE, "readCDFAVar", err, "Unable to Allocate Heap Memory for the Data");
            break;
        }

        if (atttype == NC_STRING) {
            rc = nc_get_att(grpid, varid, name, (void*)svec);        // The attribute length is the number of strings
            replaceStrings(svec, &ndata, &dvec, ndims);
        } else {
            rc = nc_get_att(grpid, varid, name, (void*)dvec);

            // If the data has a User Defined Type with a NC_STRING component then intercept and replace with a locally allocated component

            replaceEmbeddedStrings(logmalloclist, userdefinedtypelist, *udt, ndata, dvec);

            // If the structure type is an enumeration, then only the value is added to the malloc log, not the structure
            // Create the returned structure

            //if(*udt != NULL) addMalloc(dvec, ndata, (*udt)->size, (*udt)->name);	// Free Data via Malloc Log List

            if (*udt != NULL && (*udt)->idamclass != UDA_TYPE_ENUM) {
                addMalloc(logmalloclist, dvec, ndata, (size_t)(*udt)->size,
                          (*udt)->name);    // Free Data via Malloc Log List
            } else if (*udt != NULL) {
                int i;
                char value[8];
                ENUMLIST* enumlist = (ENUMLIST*)malloc(sizeof(ENUMLIST));
                nc_type base;
                size_t size, members;
                char name[NC_MAX_NAME + 1];

                if ((rc = nc_inq_enum(grpid, atttype, name, &base, &size, &members)) != NC_NOERR) {
                    err = 999;
                    addIdamError(CODEERRORTYPE, "readCDFAVar", err, (char*)nc_strerror(rc));
                    break;
                }

                if (members == 0) {
                    err = 999;
                    addIdamError(CODEERRORTYPE, "readCDFAVar", err,
                                 "Enumerated Type has Zero Membership?");
                    break;
                }

                strcpy(enumlist->name, name);
                enumlist->type = convertNCType(base);
                enumlist->count = (int)members;
                enumlist->enummember = (ENUMMEMBER*)malloc(members * sizeof(ENUMMEMBER));

                for (i = 0; i < members; i++) {
                    rc = nc_inq_enum_member(grpid, atttype, i, enumlist->enummember[i].name, (void*)value);
                    switch (enumlist->type) {
                        case UDA_TYPE_CHAR: {
                            char* enumvalue = (char*)value;
                            enumlist->enummember[i].value = (long long)*enumvalue;
                            break;
                        }
                        case UDA_TYPE_SHORT: {
                            short* enumvalue = (short*)value;
                            enumlist->enummember[i].value = (long long)*enumvalue;
                            break;
                        }
                        case UDA_TYPE_UNSIGNED_SHORT: {
                            unsigned short* enumvalue = (unsigned short*)value;
                            enumlist->enummember[i].value = (long long)*enumvalue;
                            break;
                        }
                        case UDA_TYPE_INT: {
                            int* enumvalue = (int*)value;
                            enumlist->enummember[i].value = (long long)*enumvalue;
                            break;
                        }
                        case UDA_TYPE_UNSIGNED_INT: {
                            unsigned int* enumvalue = (unsigned int*)value;
                            enumlist->enummember[i].value = (long long)*enumvalue;
                            break;
                        }
                        case UDA_TYPE_LONG64: {
                            long long* enumvalue = (long long*)value;
                            enumlist->enummember[i].value = (long long)*enumvalue;
                            break;
                        }
                        case UDA_TYPE_UNSIGNED_LONG64: {
                            unsigned long long* enumvalue = (unsigned long long*)value;
                            enumlist->enummember[i].value = (long long)*enumvalue;
                            break;
                        }
                    }
                }

                //enumlist->data = (void*)dvec;
		
		// Change to standard unsigned long long integer type		    
		    int j;
		    unsigned long long* enumarray = (unsigned long long*)malloc(ndata*sizeof(unsigned long long));
		    switch (enumlist->type) {
                       case UDA_TYPE_CHAR: {
                          char* data = (char*)dvec;
                          for (j = 0; j < ndata; j++) enumarray[j] = (unsigned long long)data[j];
                          break;
                       }
                       case UDA_TYPE_SHORT: {
                          short* data = (short*)dvec;
                          for (j = 0; j < ndata; j++) enumarray[j] = (unsigned long long)data[j];
                          break;
                       }
                       case UDA_TYPE_UNSIGNED_SHORT: {
                           unsigned short* data = (unsigned short*)dvec;
                          for (j = 0; j < ndata; j++) enumarray[j] = (unsigned long long)data[j];
                          break;
                      }
                       case UDA_TYPE_INT: {
                          int* data = (int*)dvec;
                          for (j = 0; j < ndata; j++) enumarray[j] = (unsigned long long)data[j];
                          break;
                      }
                       case UDA_TYPE_UNSIGNED_INT: {
                          unsigned int* data = (unsigned int*)dvec;
                          for (j = 0; j < ndata; j++) enumarray[j] = (unsigned long long)data[j];
                          break;
                       }
                       case UDA_TYPE_LONG64: {
                          long long* data = (long long*)dvec;
                          for (j = 0; j < ndata; j++) enumarray[j] = (unsigned long long)data[j];
                          break;
                       }
                       case UDA_TYPE_UNSIGNED_LONG64: {
                          unsigned long long* data = (unsigned long long*)dvec;
                          for (j = 0; j < ndata; j++) enumarray[j] = (unsigned long long)data[j];
                          break;
                       }
                    }
		    enumlist->enumarray = (void*)enumarray;
		    free((void *)dvec);
		
                dvec = (char*)enumlist;

                // Add mallocs to list for freeing when data dispatch complete

                addMalloc(logmalloclist, (void*)enumlist, 1, sizeof(ENUMLIST), "ENUMLIST");
                addMalloc(logmalloclist, (void*)enumlist->enummember, (int)members, sizeof(ENUMMEMBER), "ENUMMEMBER");
                //addMalloc(logmalloclist, enumlist->data, ndata, (int)size,
                //          idamNameType(idamAtomicType(base)));            // Use true integer type
                addMalloc(logmalloclist, (void*)enumlist->enumarray, ndata, sizeof(unsigned long long), "unsigned long long");  
                		     
                enumlist->enumarray_rank = 1;
                enumlist->enumarray_count = ndata;
                enumlist->enumarray_shape = (int *)malloc(sizeof(int));
                enumlist->enumarray_shape[0] = ndata;        
                addMalloc(logmalloclist, (void*)enumlist->enumarray_shape, 1, sizeof(int), "int");

            }
        }

        if (rc != NC_NOERR) {
            err = NETCDF_ERROR_INQUIRING_VARIABLE_10;
            addIdamError(CODEERRORTYPE, "readCDFAVar", err, (char*)nc_strerror(rc));
            break;
        }

        *data = dvec;        // Data Array
        *ndvec = ndata;        // Size

        if (atttype != NC_STRING) {
            ndims[0] = ndata;
            ndims[1] = 0;
        }

        //----------------------------------------------------------------------
        // End of Error Trap

    } while (0);

    //----------------------------------------------------------------------
    // Housekeeping

    if (err != 0) {
        free((void*)dvec);
    }

    return err;
}

int readCDF4Err(int grpid, int varid, int isCoordinate, int class, int rank, int* dimids, int* nevec,
                int* error_type, char** edata, LOGMALLOCLIST* logmalloclist, USERDEFINEDTYPELIST* userdefinedtypelist)
{

    int i, attid, errid = 0, rc, err = 0, isIndex = 0;
    nc_type atttype;                    // NC types
    size_t attlen;
    char* errors = NULL;
    unsigned int* extent = NULL;
    int ndimatt[2];            // NC_STRING attribute shape
    USERDEFINEDTYPE* udt = NULL;

    //----------------------------------------------------------------------
    // Test Attribute 'errors' Exists

    if (nc_inq_attid(grpid, varid, "errors", &attid) != NC_NOERR) {
        return 0;
    }

    //----------------------------------------------------------------------
    // Length of variable Name

    if ((rc = nc_inq_att(grpid, varid, "errors", &atttype, &attlen)) != NC_NOERR) {
        err = NETCDF_ERROR_INQUIRING_DIM_3;
        addIdamError(CODEERRORTYPE, "readCDF4Err", err, (char*)nc_strerror(rc));
        return err;
    }

    //----------------------------------------------------------------------
    // Check Attribute type: If char then the attribute holds the name of the variable, otherwise the data itself.

    GROUPLIST grouplist;
    grouplist.count = 1;
    grouplist.grpid = grpid;
    grouplist.grpids = NULL;

    if (atttype == NC_CHAR || atttype == NC_STRING) {

        // Allocate Heap

        if (atttype == NC_CHAR) {
            if ((errors = (char*)malloc((attlen + 1) * sizeof(char))) == NULL) {
                err = 999;
                addIdamError(CODEERRORTYPE, "readCDF4Err", err,
                             "Unable to Allocate Heap for Errors Variable Name");
                return err;
            }

            if ((rc = nc_get_att_text(grpid, varid, "errors", errors)) != NC_NOERR) {
                err = NETCDF_ERROR_INQUIRING_DIM_3;
                addIdamError(CODEERRORTYPE, "readCDF4Err", err, (char*)nc_strerror(rc));
                free((void*)errors);
                return err;
            }
            errors[attlen] = '\0';        // Ensure Null terminated
        } else {
            char** errs = (char**)malloc(attlen * sizeof(char*));
            if ((rc = nc_get_att_string(grpid, NC_GLOBAL, "errors", errs)) != NC_NOERR) {
                err = 999;
                addIdamError(CODEERRORTYPE, "readCDF", err, (char*)nc_strerror(rc));
                return err;
            }
            attlen = (int)strlen(errs[0]) + 1;
            errors = (char*)malloc(attlen * sizeof(char));
            strcpy(errors, errs[0]);
            nc_free_string(attlen, errs);
            free((void*)errs);
        }

        // Get Variable ID

        if ((rc = nc_inq_varid(grpid, errors, &errid)) != NC_NOERR) {
            err = NETCDF_ERROR_INQUIRING_VARIABLE_1;
            addIdamError(CODEERRORTYPE, "readCDF4Err", err, (char*)nc_strerror(rc));
            free((void*)errors);
            return err;
        }

        free((void*)errors);

        if ((extent = (unsigned int*)malloc((rank + 1) * sizeof(unsigned int))) == NULL) {
            err = 999;
            addIdamError(CODEERRORTYPE, "readCDF4Err", err,
                         "Unable to Allocate Heap for Extent Array");
            return err;
        }
        for (i = 0; i < rank; i++) extent[i] = 0;
        if ((err = readCDF4Var(grouplist, errid, isCoordinate, rank, dimids, extent, nevec, error_type, &isIndex, edata,
                               logmalloclist, userdefinedtypelist, &udt)) != 0) {
            addIdamError(CODEERRORTYPE, "readCDF4Err", err, "Unable to Read Error Values");
            free((void*)extent);
            return err;
        }

        free((void*)extent);

        // Apply Data Conversion

        if (class == RAW_DATA && (!isCoordinate || rank == 1)) {
            if ((rc = applyCDFCalibration(grpid, errid, *nevec, error_type, edata)) != NC_NOERR) {
                err = 999;
                addIdamError(CODEERRORTYPE, "read4Err", err, (char*)nc_strerror(rc));
                return err;
            }
        }

        //----------------------------------------------------------------------
        // Data held in an Attribute (No Data Conversion required)

    } else {

        if ((err = readCDF4AVar(grouplist, grpid, errid, atttype, "errors", nevec, ndimatt, error_type, edata,
                                logmalloclist, userdefinedtypelist, &udt)) != 0) {
            addIdamError(CODEERRORTYPE, "readCDF4Err", err, "Unable to Read Error Values");
            return err;
        }

    }

    return err;
}

int applyCDFCalibration(int grpid, int varid, int ndata, int* type, char** data)
{

    int i, err, rc, isScale = 0, isOffset = 0;
    float fscale = 0.0, foffset = 0.0;
    double scale = 0.0, offset = 0.0;
    nc_type xtypep;
    size_t lenp;

    //----------------------------------------------------------------------
    // Test for Scale and Offset

    if (nc_inq_att(grpid, varid, "scale", &xtypep, &lenp) == NC_NOERR) {        // Scale Exists
        if (lenp != 1) {
            err = 999;
            addIdamError(CODEERRORTYPE, "readCDF", err, "Scale Factor Attribute is not a Scalar");
            return err;
        }
        if (xtypep == NC_FLOAT) {
            if ((rc = nc_get_att_float(grpid, varid, "scale", &fscale)) == NC_NOERR) {
                isScale = 1;
                scale = (double)fscale;
            }
        } else {
            if (xtypep == NC_DOUBLE) {
                if ((rc = nc_get_att_double(grpid, varid, "scale", &scale)) == NC_NOERR) {
                    isScale = 1;
                }
            } else {
                err = 999;
                addIdamError(CODEERRORTYPE, "readCDF", err,
                             "Scale Factor Type is neither Float nor Double");
                return err;
            }
        }
        if (rc != NC_NOERR) {
            err = 999;
            addIdamError(CODEERRORTYPE, "readCDF", err, (char*)nc_strerror(rc));
            return rc;
        }
    }

    if (nc_inq_att(grpid, varid, "offset", &xtypep, &lenp) == NC_NOERR) {        // Offset Exists
        if (lenp != 1) {
            err = 999;
            addIdamError(CODEERRORTYPE, "readCDF", err, "Offset Factor Attribute is not a Scalar");
            return err;
        }
        if (xtypep == NC_FLOAT) {
            if ((rc = nc_get_att_float(grpid, varid, "offset", &foffset)) == NC_NOERR) {
                isOffset = 1;
                offset = (double)foffset;
            }
        } else {
            if (xtypep == NC_DOUBLE) {
                if ((rc = nc_get_att_double(grpid, varid, "offset", &offset)) == NC_NOERR) {
                    isOffset = 1;
                }
            } else {
                err = 999;
                addIdamError(CODEERRORTYPE, "readCDF", err,
                             "Offset Factor Type is neither Float nor Double");
                return err;
            }
        }
        if (rc != NC_NOERR) {
            err = 999;
            addIdamError(CODEERRORTYPE, "readCDF", err, (char*)nc_strerror(rc));
            return rc;
        }
    }

    if (!isScale && !isOffset) {
        return NC_NOERR;
    }

    UDA_LOG(UDA_LOG_DEBUG, "*** Scale factor  %12.4e\n", scale);
    UDA_LOG(UDA_LOG_DEBUG, "*** Offset        %12.4e\n", offset);

    //----------------------------------------------------------------------
    // Apply and Convert Type to Preserve Precision

    // 8 Byte numbers

    if (*type == UDA_TYPE_DOUBLE || *type == UDA_TYPE_LONG64 || *type == UDA_TYPE_UNSIGNED_LONG64) {
        double* measure = (double*)malloc(ndata * sizeof(double));
        switch (*type) {

            case UDA_TYPE_DOUBLE: {
                double* dvec = (double*)*data;
                if (isScale && isOffset) {
                    for (i = 0; i < ndata; i++) measure[i] = scale * dvec[i] + offset;
                } else {
                    if (isScale && !isOffset) {
                        for (i = 0; i < ndata; i++) measure[i] = scale * dvec[i];
                    } else {
                        if (!isScale && isOffset) {
                            for (i = 0; i < ndata; i++) measure[i] = dvec[i] + offset;
                        }
                    }
                }
                break;
            }

            case UDA_TYPE_LONG64: {
                long long int* dvec = (long long int*)*data;
                if (isScale && isOffset) {
                    for (i = 0; i < ndata; i++) measure[i] = scale * (double)dvec[i] + offset;
                } else {
                    if (isScale && !isOffset) {
                        for (i = 0; i < ndata; i++) measure[i] = scale * (double)dvec[i];
                    } else {
                        if (!isScale && isOffset) {
                            for (i = 0; i < ndata; i++) measure[i] = (double)dvec[i] + offset;
                        }
                    }
                }
                break;
            }

            case UDA_TYPE_UNSIGNED_LONG64: {
                unsigned long long int* dvec = (unsigned long long int*)*data;
                if (isScale && isOffset) {
                    for (i = 0; i < ndata; i++) measure[i] = scale * (double)dvec[i] + offset;
                } else {
                    if (isScale && !isOffset) {
                        for (i = 0; i < ndata; i++) measure[i] = scale * (double)dvec[i];
                    } else {
                        if (!isScale && isOffset) {
                            for (i = 0; i < ndata; i++) measure[i] = (double)dvec[i] + offset;
                        }
                    }
                }
                break;
            }

            default: {
                free((void*)measure);
                measure = NULL;
                break;
            }

        }

        if (measure != NULL) {
            free((void*)*data);
            *data = (char*)measure;
            *type = UDA_TYPE_DOUBLE;
        }

    } else {

        // 4 bytes or less numbers

        if (*type == UDA_TYPE_FLOAT || *type == UDA_TYPE_INT || *type == UDA_TYPE_UNSIGNED_INT ||
            *type == UDA_TYPE_SHORT || *type == UDA_TYPE_UNSIGNED_SHORT || *type == UDA_TYPE_CHAR ||
            *type == UDA_TYPE_UNSIGNED_CHAR) {
            float* measure = (float*)malloc(ndata * sizeof(float));
            switch (*type) {

                case UDA_TYPE_FLOAT: {
                    float* dvec = (float*)*data;
                    if (isScale && isOffset) {
                        for (i = 0; i < ndata; i++) measure[i] = (float)scale * dvec[i] + (float)offset;
                    } else {
                        if (isScale && !isOffset) {
                            for (i = 0; i < ndata; i++) measure[i] = (float)scale * dvec[i];
                        } else {
                            if (!isScale && isOffset) {
                                for (i = 0; i < ndata; i++) measure[i] = dvec[i] + (float)offset;
                            }
                        }
                    }
                    break;
                }

                case UDA_TYPE_INT: {
                    int* dvec = (int*)*data;
                    if (isScale && isOffset) {
                        for (i = 0; i < ndata; i++) measure[i] = (float)scale * (float)dvec[i] + (float)offset;
                    } else {
                        if (isScale && !isOffset) {
                            for (i = 0; i < ndata; i++) measure[i] = (float)scale * (float)dvec[i];
                        } else {
                            if (!isScale && isOffset) {
                                for (i = 0; i < ndata; i++) measure[i] = (float)dvec[i] + (float)offset;
                            }
                        }
                    }
                    break;
                }

                case UDA_TYPE_UNSIGNED_INT: {
                    unsigned int* dvec = (unsigned int*)*data;
                    if (isScale && isOffset) {
                        for (i = 0; i < ndata; i++) measure[i] = (float)scale * (float)dvec[i] + (float)offset;
                    } else {
                        if (isScale && !isOffset) {
                            for (i = 0; i < ndata; i++) measure[i] = (float)scale * (float)dvec[i];
                        } else {
                            if (!isScale && isOffset) {
                                for (i = 0; i < ndata; i++) measure[i] = (float)dvec[i] + (float)offset;
                            }
                        }
                    }
                    break;
                }

                case UDA_TYPE_SHORT: {
                    short* dvec = (short*)*data;
                    if (isScale && isOffset) {
                        for (i = 0; i < ndata; i++) measure[i] = (float)scale * (float)dvec[i] + (float)offset;
                    } else {
                        if (isScale && !isOffset) {
                            for (i = 0; i < ndata; i++) measure[i] = (float)scale * (float)dvec[i];
                        } else {
                            if (!isScale && isOffset) {
                                for (i = 0; i < ndata; i++) measure[i] = (float)dvec[i] + (float)offset;
                            }
                        }
                    }
                    break;
                }

                case UDA_TYPE_UNSIGNED_SHORT: {
                    unsigned short* dvec = (unsigned short*)*data;
                    if (isScale && isOffset) {
                        for (i = 0; i < ndata; i++) measure[i] = (float)scale * (float)dvec[i] + (float)offset;
                    } else {
                        if (isScale && !isOffset) {
                            for (i = 0; i < ndata; i++) measure[i] = (float)scale * (float)dvec[i];
                        } else {
                            if (!isScale && isOffset) {
                                for (i = 0; i < ndata; i++) measure[i] = (float)dvec[i] + (float)offset;
                            }
                        }
                    }
                    break;
                }
                case UDA_TYPE_CHAR: {
                    char* dvec = *data;
                    if (isScale && isOffset) {
                        for (i = 0; i < ndata; i++) measure[i] = (float)scale * (float)dvec[i] + (float)offset;
                    } else {
                        if (isScale && !isOffset) {
                            for (i = 0; i < ndata; i++) measure[i] = (float)scale * (float)dvec[i];
                        } else {
                            if (!isScale && isOffset) {
                                for (i = 0; i < ndata; i++) measure[i] = (float)dvec[i] + (float)offset;
                            }
                        }
                    }
                    break;
                }

                case UDA_TYPE_UNSIGNED_CHAR: {
                    unsigned char* dvec = (unsigned char*)*data;
                    if (isScale && isOffset) {
                        for (i = 0; i < ndata; i++) measure[i] = (float)scale * (float)dvec[i] + (float)offset;
                    } else {
                        if (isScale && !isOffset) {
                            for (i = 0; i < ndata; i++) measure[i] = (float)scale * (float)dvec[i];
                        } else {
                            if (!isScale && isOffset) {
                                for (i = 0; i < ndata; i++) measure[i] = (float)dvec[i] + (float)offset;
                            }
                        }
                    }
                    break;
                }

                default: {
                    free((void*)measure);
                    measure = NULL;
                    break;
                }

            }

            if (measure != NULL) {
                free((void*)*data);
                *data = (char*)measure;
                *type = UDA_TYPE_FLOAT;
            }

        }
    }

    return NC_NOERR;
}

void readCDF4CreateIndex(int ndata, void* dvec)
{
    // Create a substitute index
    int* dp = (int*)dvec;

    int i;
    for (i = 0; i < ndata; i++) {
        dp[i] = i;
    }
}

int readCDFCheckCoordinate(int grpid, int varid, int rank, int ncoords, char* coords, LOGMALLOCLIST* logmalloclist,
                           USERDEFINEDTYPELIST* userdefinedtypelist)
{

    int rc, err = 0, i, j, data_n, data_type, isCoordinate = 0, isIndex = 0;

    int* dimids = NULL;
    unsigned int* extent = NULL;    // Shape of the data array
    char* data = NULL;

    USERDEFINEDTYPE* udt = NULL;
    GROUPLIST grouplist;

    //----------------------------------------------------------------------
    // Error Trap Loop

    do {

        //----------------------------------------------------------------------
        // Get Dimension/Coordinate ID List of the variable

        if ((dimids = (int*)malloc(rank * sizeof(int))) == NULL) {
            err = NETCDF_ERROR_ALLOCATING_HEAP_1;
            addIdamError(CODEERRORTYPE, "readCDF", err,
                         "Problem Allocating Heap Memory for dimids array");
            break;
        }

        if ((rc = nc_inq_vardimid(grpid, varid, dimids)) != NC_NOERR) {
            err = NETCDF_ERROR_INQUIRING_DIM_2;
            addIdamError(CODEERRORTYPE, "readCDFCheckCoordinate", err, (char*)nc_strerror(rc));
            break;
        }

        //----------------------------------------------------------------------
        // Allocate and Initialise Dimensional/Coordinate Data & Extent data

        if ((extent = (unsigned int*)malloc((rank + 1) * sizeof(unsigned int))) == NULL) {
            err = NETCDF_ERROR_ALLOCATING_HEAP_1;
            addIdamError(CODEERRORTYPE, "readCDFCheckCoordinate", err,
                         "Problem Allocating Heap Memory for extent array");
            break;
        }

        //----------------------------------------------------------------------
        // Read the Data Array

        grouplist.count = 1;
        grouplist.grpid = grpid;
        grouplist.grpids = NULL;

        err = readCDF4Var(grouplist, varid, isCoordinate, rank, dimids, extent, &data_n, &data_type, &isIndex, &data,
                          logmalloclist, userdefinedtypelist, &udt);

        if (err != 0) {
            addIdamError(CODEERRORTYPE, "readCDFCheckCoordinate", err, "Unable to Read Data Values");
            break;
        }

        //----------------------------------------------------------------------
        // Check coordinate is constant over the other dimension

        if (extent[1] != ncoords) {
            err = 999;
            addIdamError(CODEERRORTYPE, "readCDFCheckCoordinate", err,
                         "Inconsistent Array Lengths found: Cannot Check Multi-Dimensional Coordinate is Constant");
            break;
        }

        switch (data_type) {

            case UDA_TYPE_DOUBLE: {
                double* dvec = (double*)data;
                double* dcoo = (double*)coords;
                for (i = 0; i < ncoords; i++) {
                    for (j = 0; j < extent[0]; j++) {
                        if (dvec[j * ncoords + i] != dcoo[i]) {
                            err = -1;
                            break;
                        }
                    }
                    if (err != 0) break;
                }
                break;
            }

            case UDA_TYPE_FLOAT: {
                float* dvec = (float*)data;
                float* dcoo = (float*)coords;
                for (i = 0; i < ncoords; i++) {
                    for (j = 0; j < extent[0]; j++) {
                        if (dvec[j * ncoords + i] != dcoo[i]) {
                            err = -1;
                            break;
                        }
                    }
                    if (err != 0) break;
                }
                break;
            }

            case UDA_TYPE_LONG64: {
                long long int* dvec = (long long int*)data;
                long long int* dcoo = (long long int*)coords;
                for (i = 0; i < ncoords; i++) {
                    for (j = 0; j < extent[0]; j++) {
                        if (dvec[j * ncoords + i] != dcoo[i]) {
                            err = -1;
                            break;
                        }
                    }
                    if (err != 0) break;
                }
                break;
            }

            case UDA_TYPE_INT: {
                int* dvec = (int*)data;
                int* dcoo = (int*)coords;
                for (i = 0; i < ncoords; i++) {
                    for (j = 0; j < extent[0]; j++) {
                        if (dvec[j * ncoords + i] != dcoo[i]) {
                            err = -1;
                            break;
                        }
                    }
                    if (err != 0) break;
                }
                break;
            }

            case UDA_TYPE_SHORT: {
                short* dvec = (short*)data;
                short* dcoo = (short*)coords;
                for (i = 0; i < ncoords; i++) {
                    for (j = 0; j < extent[0]; j++) {
                        if (dvec[j * ncoords + i] != dcoo[i]) {
                            err = -1;
                            break;
                        }
                    }
                    if (err != 0) break;
                }
                break;
            }

            case UDA_TYPE_CHAR: {
                char* dvec = data;
                char* dcoo = coords;
                for (i = 0; i < ncoords; i++) {
                    for (j = 0; j < extent[0]; j++) {
                        if (dvec[j * ncoords + i] != dcoo[i]) {
                            err = -1;
                            break;
                        }
                    }
                    if (err != 0) break;
                }
                break;
            }

            case UDA_TYPE_UNSIGNED_LONG64: {
                unsigned long long int* dvec = (unsigned long long int*)data;
                unsigned long long int* dcoo = (unsigned long long int*)coords;
                for (i = 0; i < ncoords; i++) {
                    for (j = 0; j < extent[0]; j++) {
                        if (dvec[j * ncoords + i] != dcoo[i]) {
                            err = -1;
                            break;
                        }
                    }
                    if (err != 0) break;
                }
                break;
            }

            case UDA_TYPE_UNSIGNED_INT: {
                unsigned int* dvec = (unsigned int*)data;
                unsigned int* dcoo = (unsigned int*)coords;
                for (i = 0; i < ncoords; i++) {
                    for (j = 0; j < extent[0]; j++) {
                        if (dvec[j * ncoords + i] != dcoo[i]) {
                            err = -1;
                            break;
                        }
                    }
                    if (err != 0) break;
                }
                break;
            }

            case UDA_TYPE_UNSIGNED_SHORT: {
                unsigned short* dvec = (unsigned short*)data;
                unsigned short* dcoo = (unsigned short*)coords;
                for (i = 0; i < ncoords; i++) {
                    for (j = 0; j < extent[0]; j++) {
                        if (dvec[j * ncoords + i] != dcoo[i]) {
                            err = -1;
                            break;
                        }
                    }
                    if (err != 0) break;
                }
                break;
            }

            case UDA_TYPE_UNSIGNED_CHAR: {
                unsigned char* dvec = (unsigned char*)data;
                unsigned char* dcoo = (unsigned char*)coords;
                for (i = 0; i < ncoords; i++) {
                    for (j = 0; j < extent[0]; j++) {
                        if (dvec[j * ncoords + i] != dcoo[i]) {
                            err = -1;
                            break;
                        }
                    }
                    if (err != 0) break;
                }
                break;
            }

            case UDA_TYPE_COMPLEX: {
                COMPLEX* dvec = (COMPLEX*)data;
                COMPLEX* dcoo = (COMPLEX*)coords;
                for (i = 0; i < ncoords; i++) {
                    for (j = 0; j < extent[0]; j++) {
                        if (dvec[j * ncoords + i].real != dcoo[i].real &&
                            dvec[j * ncoords + i].imaginary != dcoo[i].imaginary) {
                            err = -1;
                            break;
                        }
                    }
                    if (err != 0) break;
                }
                break;
            }

            case UDA_TYPE_DCOMPLEX: {
                DCOMPLEX* dvec = (DCOMPLEX*)data;
                DCOMPLEX* dcoo = (DCOMPLEX*)coords;
                for (i = 0; i < ncoords; i++) {
                    for (j = 0; j < extent[0]; j++) {
                        if (dvec[j * ncoords + i].real != dcoo[i].real &&
                            dvec[j * ncoords + i].imaginary != dcoo[i].imaginary) {
                            err = -1;
                            break;
                        }
                    }
                    if (err != 0) break;
                }
                break;
            }

            default:
                err = NETCDF_ERROR_ALLOCATING_HEAP_6;
                addIdamError(CODEERRORTYPE, "readCDFVar", err, "Unknown Data Type");
                break;
        }


        //----------------------------------------------------------------------
        // House Keeping

    } while (0);

    free((void*)extent);
    free((void*)data);
    free((void*)dimids);

    return err;
}

int isAtomicNCType(nc_type type)
{
    switch (type) {
        case NC_BYTE:
            return 1;
        case NC_CHAR:
            return 1;
        case NC_SHORT:
            return 1;
        case NC_INT:
            return 1;
        case NC_INT64:
            return 1;
        case NC_FLOAT:
            return 1;
        case NC_DOUBLE:
            return 1;
        case NC_UBYTE:
            return 1;
        case NC_USHORT:
            return 1;
        case NC_UINT:
            return 1;
        case NC_UINT64:
            return 1;
        case NC_STRING:
            return 1;    // Treated as a byte/char array
        default:
            return 0;
    }
    return 0;
}

int idamAtomicType(nc_type type)
{
    switch (type) {
        case NC_DOUBLE:
            return UDA_TYPE_DOUBLE;
        case NC_FLOAT:
            return UDA_TYPE_FLOAT;
        case NC_INT64:
            return UDA_TYPE_LONG64;
        case NC_INT:
            return UDA_TYPE_INT;
        case NC_SHORT:
            return UDA_TYPE_SHORT;
        case NC_BYTE:
            return UDA_TYPE_CHAR;
        case NC_CHAR:
            return UDA_TYPE_CHAR;
        case NC_UINT64:
            return UDA_TYPE_UNSIGNED_LONG64;
        case NC_UINT:
            return UDA_TYPE_UNSIGNED_INT;
        case NC_USHORT:
            return UDA_TYPE_UNSIGNED_SHORT;
        case NC_UBYTE:
            return UDA_TYPE_UNSIGNED_CHAR;
        case NC_STRING:
            return UDA_TYPE_STRING;
        default:
            return UDA_TYPE_UNKNOWN;
    }
    return UDA_TYPE_UNKNOWN;
}

int readCDF4ParseSubset(char* op, CDFSUBSET* cdfsubset)
{
    // Return codes:
    //
    //	1 => Valid subset
    //	0 => Not a subset operation - Not compliant with syntax
    //     -1 => Error

    int i, j, err, rc = 1, subsetCount = 1;        // Number of subsetting operations
    char* p, * token = NULL;

    int lwork = (int)strlen(op) + 1;
    char* work = (char*)malloc(lwork * sizeof(char));

    strcpy(work, op);
    work[0] = ' ';
    work[lwork - 2] = ' ';

    UDA_LOG(UDA_LOG_DEBUG, "readCDF4ParseSubset: %s\n", op);

    lwork = lwork + 2;            // expand :: to 0:*:

    //----------------------------------------------------------------------------------------------------------------------------
    // Split instructions using syntax [a:b:c][d:e:f] or [a:b:c, d:e:f] where [startIndex:stopIndex:stride]
    //
    // Syntax	[a]		single items at index position a
    //		[*]		all items
    //		[]		all items
    //
    //		[:]		all items starting at 0
    //		[a:]		all items starting at a
    //		[a:*]		all items starting at a
    //		[a:b]		all items starting at a and ending at b
    //
    //		[a::c]		all items starting at a with stride c
    //		[a:*:c]		all items starting at a with stride c
    //		[a:b:c]		all items starting at a, ending at b with stride c

    while ((token = strstr(work, "][")) != NULL || (token = strstr(work, "}{")) != NULL) {    // Adopt a single syntax
        token[0] = ',';
        token[1] = ' ';
    }
    p = work;
    while ((token = strchr(p, ',')) != NULL) {        // Count the Dimensions
        p = &token[1];
        subsetCount++;
    }
    if (subsetCount > NC_MAX_VAR_DIMS) subsetCount = NC_MAX_VAR_DIMS;

    // Array of subset instructions for each dimension

    char** work2 = (char**)malloc(subsetCount * sizeof(char*));
    for (i = 0; i < subsetCount; i++) {
        work2[i] = (char*)malloc(lwork * sizeof(char));
        work2[i][0] = '\0';
    }

    // 3 subset details

    char** work3 = (char**)malloc(3 * sizeof(char*));
    for (i = 0; i < 3; i++) {
        work3[i] = (char*)malloc(lwork * sizeof(char));
        work3[i][0] = '\0';
    }
    char* work4 = (char*)malloc(lwork * sizeof(char));

    for (i = 0; i < subsetCount; i++) {
        cdfsubset->start[i] = 0;
        cdfsubset->stop[i] = 0;
        cdfsubset->count[i] = 0;
        cdfsubset->stride[i] = 0;
    }
    cdfsubset->subsetCount = subsetCount;

    subsetCount = 0;
    if ((token = strtok(work, ",")) != NULL) {    // Process each subset instruction separately (work2)
        strcpy(work2[subsetCount++], token);
        while (subsetCount < NC_MAX_VAR_DIMS && (token = strtok(NULL, ",")) != NULL)
            strcpy(work2[subsetCount++], token);

        do {
            for (i = 0; i < subsetCount; i++) {

                cdfsubset->subset[i] = 0;

                TrimString(work2[i]);
                LeftTrimString(work2[i]);
                for (j = 0; j < 3; j++)work3[j][0] = '\0';

                if (work2[i][0] == ':') {
                    work4[0] = '0';
                    work4[1] = '\0';
                    if (work2[i][1] != ':') {
                        strcat(work4, work2[i]);
                        strcpy(work2[i], work4);
                    } else {
                        strcat(work4, ":*");
                        strcat(work4, &work2[i][1]);
                        strcpy(work2[i], work4);
                    }
                } else {
                    if ((p = strstr(work2[i], "::")) != NULL) {
                        p[0] = '\0';
                        strcpy(work4, work2[i]);
                        strcat(work4, ":*");
                        strcat(work4, &p[1]);
                        strcpy(work2[i], work4);
                    } else {
                        strcpy(work4, work2[i]);
                    }
                }

                if (strchr(work2[i], ':') != NULL && (token = strtok(work2[i], ":")) != NULL) {
                    j = 0;
                    strcpy(work3[j++], token);
                    while (j < 3 && (token = strtok(NULL, ":")) != NULL) strcpy(work3[j++], token);
                    for (j = 0; j < 3; j++) TrimString(work3[j]);
                    for (j = 0; j < 3; j++) LeftTrimString(work3[j]);

                    if (work3[0][0] != '\0' && IsNumber(work3[0])) {    // [a:] or [a:*] or [a:b] etc
                        p = NULL;
                        errno = 0;
                        cdfsubset->start[i] = (size_t)strtol(work3[0], &p, 10);
                        if (errno != 0 || *p != 0 || p == work3[0]) {
                            rc = 0;
                            break;
                        }
                        if ((int)cdfsubset->start[i] < 0) {
                            err = 999;
                            addIdamError(CODEERRORTYPE, "readCDF", err,
                                         "Invalid Start Index in subset operation");
                            rc = -1;
                            break;
                        }

                        cdfsubset->stop[i] = cdfsubset->start[i];
                        cdfsubset->count[i] = 1;
                        cdfsubset->stride[i] = 1;
                        cdfsubset->subset[i] = 1;

                    } else {
                        rc = 0;                    // Not an Error - not a subset operation
                        break;
                    }
                    if (work3[1][0] != '\0' && IsNumber(work3[1])) {    // [a:b]
                        p = NULL;
                        errno = 0;
                        cdfsubset->stop[i] = (size_t)strtol(work3[1], &p, 10);
                        if (errno != 0 || *p != 0 || p == work3[0]) {
                            rc = 0;
                            break;
                        }
                        if ((int)cdfsubset->stop[i] < 0) {
                            err = 999;
                            addIdamError(CODEERRORTYPE, "readCDF", err,
                                         "Invalid sample End Index in subset operation");
                            rc = -1;
                            break;
                        }
                        cdfsubset->count[i] = cdfsubset->stop[i] - cdfsubset->start[i] + 1;
                        cdfsubset->subset[i] = 1;

                        if ((int)cdfsubset->count[i] < 1) {
                            err = 999;
                            addIdamError(CODEERRORTYPE, "readCDF", err,
                                         "Invalid Stop Index in subset operation");
                            rc = -1;
                            break;
                        }

                    } else {
                        if (strlen(work3[1]) == 0 || (strlen(work3[1]) == 1 && work3[1][0] == '*')) {    // [a:],[a:*]
                            cdfsubset->stop[i] = 0;            // To end of dimension
                            cdfsubset->count[i] = 0;
                        } else {
                            rc = 0;
                            break;
                        }
                    }
                    if (work3[2][0] != '\0') {
                        if (IsNumber(work3[2])) {
                            p = NULL;
                            errno = 0;
                            cdfsubset->stride[i] = (size_t)strtol(work3[2], &p, 10);
                            if (errno != 0 || *p != 0 || p == work3[0]) {
                                rc = 0;
                                break;
                            }
                            if ((int)cdfsubset->stride[i] <= 0) {
                                err = 999;
                                addIdamError(CODEERRORTYPE, "readCDF", err,
                                             "Invalid sample stride length in subset operation");
                                rc = -1;
                                break;
                            }
                            if (cdfsubset->stride[i] > 1)cdfsubset->subset[i] = 1;

                            if (cdfsubset->stride[i] > 1 && cdfsubset->count[i] > 1) {
                                if ((cdfsubset->count[i] % cdfsubset->stride[i]) > 0) {
                                    cdfsubset->count[i] = 1 + cdfsubset->count[i] / cdfsubset->stride[i];
                                } else {
                                    cdfsubset->count[i] = cdfsubset->count[i] / cdfsubset->stride[i];
                                }
                            }


                        } else {
                            rc = 0;
                            break;
                        }
                    }
                } else {
                    if (work4[0] == '\0' || work4[0] == '*') {        // [], [*]
                        cdfsubset->start[i] = 0;
                        cdfsubset->stop[i] = 0;
                        cdfsubset->count[i] = 0;
                        cdfsubset->stride[i] = 1;
                    } else {
                        if (IsNumber(work4)) {                // [a]
                            p = NULL;
                            errno = 0;
                            cdfsubset->start[i] = (size_t)strtol(work4, &p, 10);
                            if (errno != 0 || *p != 0 || p == work3[0]) {
                                rc = 0;
                                break;
                            }
                            if ((int)cdfsubset->start[i] < 0) {
                                err = 999;
                                addIdamError(CODEERRORTYPE, "readCDF", err,
                                             "Invalid start index in subset operation");
                                rc = -1;
                                break;
                            }

                            cdfsubset->stop[i] = cdfsubset->start[i];
                            cdfsubset->count[i] = 1;
                            cdfsubset->stride[i] = 1;
                            cdfsubset->subset[i] = 1;

                        } else {
                            rc = 0;
                            break;
                        }
                    }
                }
            }
        } while (0);
    } else {
        rc = 0;
    }

    free(work);
    for (i = 0; i < subsetCount; i++) {
        free(work2[i]);
    }
    free(work2);
    for (i = 0; i < 3; i++) {
        free(work3[i]);
    }
    free(work3);
    free(work4);

    return rc;
}


