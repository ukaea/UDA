/*---------------------------------------------------------------
* netCDF4 User Defined Data Types: Create definition data structure
*
* Input Arguments:
*
* Returns:
*
* Calls
*
* Possible Issue: If there are multiple user defined types with the same name but defined in different locations, does netcdf use a
* scoping rule to decide which definition is valid? IDAM does not use a scoping rule - each type must be unique.
*
*-----------------------------------------------------------------------------*/

#include <netcdf.h>
#include <stdlib.h>
#include <memory.h>

#include <structures/struct.h>
#include <clientserver/errorLog.h>

#include "readCDF4.h"

int readCDFTypes(int grpid, USERDEFINEDTYPELIST* userdefinedtypelist)
{
    int rc, err = 0, ntypes = 0, class, foff;

    int* typeids = NULL;
    char name[NC_MAX_NAME + 1];

    size_t size;
    nc_type base, type;
    size_t fieldcount;
    size_t offset;
    int rank, shape[NC_MAX_DIMS];

    USERDEFINEDTYPE usertype;
    COMPOUNDFIELD field;

    //----------------------------------------------------------------------
    // Error Trap Loop

    do {

        //----------------------------------------------------------------------
        // List all User Defined data types within this group (What is the scoping rule?)
        // Should Add all others within scope if types are Not Local

        if ((rc = nc_inq_typeids(grpid, &ntypes, NULL)) != NC_NOERR || ntypes == 0) break;

        typeids = (int*)malloc(ntypes * sizeof(int));        // Type List

        if ((rc = nc_inq_typeids(grpid, &ntypes, typeids)) != NC_NOERR) break;

        //----------------------------------------------------------------------
        // Build Definitions of User Defined data types in this group

        int i;
        for (i = 0; i < ntypes; i++) {

            initUserDefinedType(&usertype);    // New structure definition

            if ((rc = nc_inq_user_type(grpid, (nc_type)typeids[i], name, &size, &base, &fieldcount, &class)) !=
                NC_NOERR) {
                err = 999;
                addIdamError(CODEERRORTYPE, __FILE__, err, (char*)nc_strerror(rc));
                break;
            }

            strcpy(usertype.name, name);
            strcpy(usertype.source, "netcdf");
            usertype.ref_id = typeids[i];        // Defined within this group only (Scope?)
            usertype.imagecount = 0;                // No Structure Image data
            usertype.image = NULL;
            usertype.size = (int)size;            // Structure size

            switch (class) {

                case NC_COMPOUND: {                // Compound Types

                    usertype.idamclass = UDA_TYPE_COMPOUND;

                    foff = 0;

                    size_t j;
                    for (j = 0; j < fieldcount; j++) {

                        initCompoundField(&field);

                        if ((rc = nc_inq_compound_field(grpid, (nc_type)typeids[i], j, name, &offset, &type, &rank,
                                                        shape)) == NC_NOERR) {

                            strcpy(field.name, name);

                            // STRING Convention: 1 byte numerical values are always signed and unsigned bytes. Character strings (rank > 0, length > 1) are always char.

                            if (type == NC_CHAR && rank > 0 && shape[0] > 1) {
                                // Otherwise treat as a single character
                                field.atomictype = UDA_TYPE_STRING;
                                strcpy(field.type, "STRING");
                            } else {
                                if (type == NC_STRING) {
                                    field.atomictype = UDA_TYPE_STRING;
                                    strcpy(field.type,
                                           "STRING *");                // Array of strings of arbitrary length
                                } else {
                                    field.atomictype = convertNCType(
                                            type);            // convert netCDF base type to IDAM type
                                    strcpy(field.type, idamNameType(
                                            field.atomictype));        // convert atomic type to a string label
                                }
                            }

                            if (field.atomictype == UDA_TYPE_UNKNOWN) {
                                // must be another user defined structure
                                // Identify via type id
                                USERDEFINEDTYPE* udt = findUserDefinedType(userdefinedtypelist, "", (int)type);

                                if (udt != NULL) {
                                    // **** ENUM types within compound structures cannot be passed in the expanded form: collapse back to the original integer type.
                                    // **** Unable to substitute a pointer as integer type may only be 1 or 2 bytes
                                    // **** NEED: Return the enumerated type set of values and names to the client

                                    if (udt->idamclass == UDA_TYPE_ENUM) {
                                        size_t base_size, memberCount;        // dgm changed from int to size_t 16Dec11
                                        nc_type base;
                                        rc = nc_inq_enum(grpid, type, name, &base, (size_t*)&base_size,
                                                         (size_t*)&memberCount);
                                        field.atomictype = convertNCType(base);
                                        strcpy(field.type, idamNameType(field.atomictype));
                                    } else {
                                        strcpy(field.type, udt->name);
                                    }
                                } else {
                                    err = 999;
                                    addIdamError(CODEERRORTYPE, "readCDFTypes", err,
                                                 "User defined type not registered!");
                                    break;
                                }
                            }

                            field.pointer = 0;                        // all fields are fixed size arrays
                            field.desc[0] = '\0';
                            field.size = getsizeof(userdefinedtypelist, field.type);

                            field.rank = rank;
                            field.count = 1;
                            if (rank > 0) {
                                field.shape = (int*)malloc(rank * sizeof(int));
                                int k;
                                for (k = 0; k < rank; k++) {
                                    field.shape[k] = shape[k];
                                    field.count = field.count * shape[k];
                                }
                            } else {
                                field.shape = NULL;
                            }

                            if (type == NC_STRING) {

                                if (rank > 1) {
                                    err = 999;
                                    addIdamError(CODEERRORTYPE, "readCDFTypes", err,
                                                 "String Arrays with Rank > 1 have not been implemented");
                                    break;
                                }
                                field.size = field.count * getsizeof(userdefinedtypelist, "char *");
                            }

                            field.offset = (int)offset;                                            // use previous offset for alignment
                            field.offpad = padding(field.offset, field.type);
                            field.alignment = getalignmentof(field.type);

                            addCompoundField(&usertype, field);
                        }
                    }

                    break;
                }

                case NC_VLEN: {                    // variable Length Array Types

                    usertype.idamclass = UDA_TYPE_VLEN;

                    // Ensure the Type name is unique as the data item has a named type (not void *)

                    static unsigned int counter = 0;
                    if (counter > 0) {
                        char* unique = (char*)malloc((strlen(usertype.name) + 10) * sizeof(char));
                        sprintf(unique, "%s_%d", usertype.name, counter);
                        strcpy(usertype.name, unique);
                        free(unique);
                    } else {
                        counter++;
                    }

                    if ((rc = nc_inq_vlen(grpid, (nc_type)typeids[i], name, &size, &base)) == NC_NOERR) {

                        initCompoundField(&field);

                        strcpy(field.name, "len");
                        strcpy(field.type, "unsigned int");
                        strcpy(field.desc, "Length of the data array");
                        field.size = getsizeof(userdefinedtypelist, field.type);
                        field.offset = newoffset(0, field.type);
                        foff = field.offset + field.size;
                        field.offpad = padding(field.offset, field.type);
                        field.alignment = getalignmentof(field.type);
                        field.pointer = 0;
                        field.atomictype = UDA_TYPE_UNSIGNED_INT;
                        field.rank = 0;
                        field.count = 1;
                        field.shape = NULL;

                        addCompoundField(&usertype, field);

                        initCompoundField(&field);

                        strcpy(field.name, "data");
                        strcpy(field.desc, "the variable length data array");
                        field.atomictype = convertNCType(base);                // convert netCDF base type to IDAM type

                        if (field.atomictype == UDA_TYPE_UNKNOWN) {                // must be a User Defined Type
                            // Identify via type id
                            USERDEFINEDTYPE* udt = findUserDefinedType(userdefinedtypelist, "", (int)base);
                            if (udt == NULL) {
                                err = 999;
                                addIdamError(CODEERRORTYPE, __FILE__, err, "User defined type not registered!");
                                break;
                            }
                            strcpy(field.type, udt->name);
                        } else {
                            // convert atomic type to a string label
                            strcpy(field.type, idamNameType(field.atomictype));
                        }

                        field.pointer = 1;

                        field.size = getsizeof(userdefinedtypelist, "void *");                // pointer size
                        field.offset = newoffset(foff, "void *");
                        field.offpad = padding(field.offset, "void *");
                        field.alignment = getalignmentof("void *");

                        field.rank = 0;
                        field.count = 1;                        // Single pointer to array of length 'len'
                        field.shape = NULL;

                        addCompoundField(&usertype, field);
                    }
                    break;
                }

                case NC_OPAQUE: {                                // Opaque Types
                    err = 999;
                    addIdamError(CODEERRORTYPE, __FILE__, err, "Not configured for OPAQUE Types!");
                    break;
                }

                case NC_ENUM: {                // enum Types use an existing structure: Copy and personalise

                    // Standard ENUMLIST type
                    USERDEFINEDTYPE* udt = findUserDefinedType(userdefinedtypelist, "ENUMLIST", 0);

                    usertype = *udt;

                    usertype.idamclass = UDA_TYPE_ENUM;
                    usertype.ref_id = typeids[i];        // Type ID is used to locate this definition (shares a type name)

                    usertype.image = (char*)malloc(udt->imagecount * sizeof(char));        // Copy pointer type (prevents double free)
                    memcpy(usertype.image, udt->image, udt->imagecount);

                    usertype.compoundfield = (COMPOUNDFIELD*)malloc(udt->fieldcount * sizeof(COMPOUNDFIELD));
                    int j;
                    for (j = 0; j < udt->fieldcount; j++) {
                        initCompoundField(&usertype.compoundfield[j]);
                        usertype.compoundfield[j] = udt->compoundfield[j];
                        if (udt->compoundfield[j].rank > 0) {
                            usertype.compoundfield[j].shape = (int*)malloc(udt->compoundfield[j].rank * sizeof(int));
                            int k;
                            for (k = 0; k < udt->compoundfield[j].rank; k++) {
                                usertype.compoundfield[j].shape[k] = udt->compoundfield[j].shape[k];
                            }
                        }
                    }

                    break;
                }

            }

            if (err == NC_NOERR) {
                addUserDefinedType(userdefinedtypelist, usertype);
            }
        }

        //----------------------------------------------------------------------

    } while (0);

    free((void*)typeids);

    if (err != NC_NOERR) {
        addIdamError(CODEERRORTYPE, "readCDF", err, "Unable to Query User Defined Types");
        addIdamError(CODEERRORTYPE, "readCDF", err, (char*)nc_strerror(rc));
        err = 999;
        return err;
    }

    return err;
}

int convertNCType(nc_type type)
{
    switch (type) {
        case NC_BYTE:
            return UDA_TYPE_CHAR;
        case NC_CHAR:
            return UDA_TYPE_CHAR;
        case NC_SHORT:
            return UDA_TYPE_SHORT;
        case NC_INT:
            return UDA_TYPE_INT;
        case NC_INT64:
            return UDA_TYPE_LONG64;
        case NC_FLOAT:
            return UDA_TYPE_FLOAT;
        case NC_DOUBLE:
            return UDA_TYPE_DOUBLE;
        case NC_UBYTE:
            return UDA_TYPE_UNSIGNED_CHAR;
        case NC_USHORT:
            return UDA_TYPE_UNSIGNED_SHORT;
        case NC_UINT:
            return UDA_TYPE_UNSIGNED_INT;
        case NC_UINT64:
            return UDA_TYPE_UNSIGNED_LONG64;
        case NC_VLEN:
            return UDA_TYPE_VLEN;
        case NC_COMPOUND:
            return UDA_TYPE_COMPOUND;
        case NC_OPAQUE:
            return UDA_TYPE_OPAQUE;
        case NC_ENUM:
            return UDA_TYPE_ENUM;
        case NC_STRING:
            return UDA_TYPE_STRING;
        default:
            return UDA_TYPE_UNKNOWN;
    }
}

void printNCType(FILE* fd, nc_type type)
{
    switch (type) {
        case NC_BYTE: {
            fprintf(fd, "%d NC_BYTE\n", type);
            return;
        }
        case NC_SHORT: {
            fprintf(fd, "%d NC_SHORT\n", type);
            return;
        }
        case NC_INT: {
            fprintf(fd, "%d NC_INT\n", type);
            return;
        }
        case NC_INT64: {
            fprintf(fd, "%d NC_INT64\n", type);
            return;
        }
        case NC_FLOAT: {
            fprintf(fd, "%d NC_FLOAT\n", type);
            return;
        }
        case NC_DOUBLE: {
            fprintf(fd, "%d NC_DOUBLE\n", type);
            return;
        }
        case NC_UBYTE: {
            fprintf(fd, "%d NC_UBYTE\n", type);
            return;
        }
        case NC_USHORT: {
            fprintf(fd, "%d NC_USHORT\n", type);
            return;
        }
        case NC_UINT: {
            fprintf(fd, "%d NC_UINT\n", type);
            return;
        }
        case NC_UINT64: {
            fprintf(fd, "%d NC_UINT64\n", type);
            return;
        }
        case NC_VLEN: {
            fprintf(fd, "%d NC_VLEN\n", type);
            return;
        }
        case NC_COMPOUND: {
            fprintf(fd, "%d NC_COMPOUND\n", type);
            return;
        }
        case NC_OPAQUE: {
            fprintf(fd, "%d NC_OPAQUE\n", type);
            return;
        }
        case NC_ENUM: {
            fprintf(fd, "%d NC_ENUM\n", type);
            return;
        }
        case NC_STRING: {
            fprintf(fd, "%d NC_STRING\n", type);
            return;
        }
        default:
            fprintf(fd, "%d NOT KNOWN: Not an Atomic Type\n", type);
    }
}
