//============================================================================================================================
// Issue log:
//
// 1. limiter data: version returns as null with count 0.
// 2. limiter data: renamed data array to 'data' from 'limiter' as causes search functions to pick up the structure
//    named limiter.
// 3. Check all udts from groups prior to sub-tree root are listed.
// 4. Remove test code from readCDF
// 5. Remove test code from idam dlm
//
//-----------------------------------------------------------------------------------------------------------------

#include <netcdf.h>
#include <stdlib.h>
#include <memory.h>

#include <structures/struct.h>
#include <logging/logging.h>
#include <clientserver/errorLog.h>
#include <clientserver/udaErrors.h>
#include <clientserver/stringUtils.h>

#include "readCDF4.hpp"

// Read netCDF4 sub-Tree

int idamAtomicType(nc_type type);

static int nameKey = 0;    // Create unique names by appending an incrementing integer

void initVariable(VARIABLE* variable)
{
    variable->varid = 0;
    variable->numatts = 0;
    variable->attribute = nullptr;
    variable->varname[0] = '\0';
    variable->vartype = 0;
    variable->rank = 0;
    variable->udtIndex = -1;
    variable->shape = nullptr;
    variable->udt = nullptr;
}

void initAttribute(ATTRIBUTE* attribute)
{
    attribute->attid = 0;
    attribute->attname[0] = '\0';
    attribute->atttype = 0;
    attribute->attlength = 0;
    attribute->udtIndex = -1;
    attribute->udt = nullptr;
}

void initGroup(GROUP* group)
{
    group->grpid = 0;
    group->parent = 0;
    group->grpname[0] = '\0';
    group->numgrps = 0;
    group->numatts = 0;
    group->numvars = 0;
    group->udtIndex = -1;
    group->grpids = nullptr;
    group->udt = nullptr;
    group->attribute = nullptr;
    group->variable = nullptr;
}

void initHGroup(HGROUPS* hgroups)
{
    hgroups->grpcount = 0;
    hgroups->numgrps = 0;
    hgroups->groups = nullptr;
}

int addHGroup(HGROUPS* hgroups, GROUP group)
{
    int udtIndex;
    if (hgroups->numgrps + 1 >= hgroups->grpcount) {
        hgroups->grpcount += GROWMALLOCLIST;
        hgroups->groups = (GROUP*)realloc((void*)hgroups->groups, (hgroups->grpcount) * sizeof(GROUP));
    }
    hgroups->groups[hgroups->numgrps] = group;
    udtIndex = hgroups->numgrps;
    hgroups->numgrps++;
    return (udtIndex);
}

GROUP* findHGroup(HGROUPS* hgroups, int grpid)
{
    int i;
    for (i = 0; i < hgroups->numgrps; i++) {
        if (hgroups->groups[i].grpid == grpid) return (&hgroups->groups[i]);
    }
    return nullptr;
}

void freeGroup(GROUP* group)
{
    int i;
    if (group->numgrps > 0) free((void*)group->grpids);
    if (group->numatts > 0) free((void*)group->attribute);
    for (i = 0; i < group->numvars; i++) {
        if (group->variable[i].numatts > 0) free((void*)group->variable[i].attribute);  // Shape freed via mallocLog
    }
    if (group->numvars > 0) free((void*)group->variable);
}

void freeHGroups(HGROUPS* hgroups)
{
    int i;
    for (i = 0; i < hgroups->numgrps; i++) {
        freeGroup(&hgroups->groups[i]);
    }
    free((void*)hgroups->groups);
    initHGroup(hgroups);
}

char*
getUniqueTypeName(char* proposed, int ref_id, LOGMALLOCLIST* logmalloclist, USERDEFINEDTYPELIST* userdefinedtypelist)
{
    // Sub-trees may have data groups and variables with the same name but with different contents.
    // Normally, the type name is taken from the group or variable name. To ensure uniqueness (searching is
    // done on type name), the reference ID is tagged to the name.

    int i, ndigits;
    char* unique;

    for (i = 0; i < userdefinedtypelist->listCount; i++) {

        // Check the user defined type name
        if (STR_EQUALS(proposed, userdefinedtypelist->userdefinedtype[i].name)) {
            ndigits = 1 + ref_id / 10 + (int)strlen(proposed) + 1 + 1;
            unique = (char*)malloc(ndigits * sizeof(char));
            sprintf(unique, "%s_%d", proposed, ref_id);
            addMalloc(logmalloclist, unique, ndigits, sizeof(char), "char");
            return (unique);
        }
    }

    // Check the structure component types
    int j;
    for (i = 0; i < userdefinedtypelist->listCount; i++) {
        for (j = 0; j < userdefinedtypelist->userdefinedtype[i].fieldcount; j++) {
            if (STR_EQUALS(proposed, userdefinedtypelist->userdefinedtype[i].compoundfield[j].type)) {
                ndigits = 1 + ref_id / 10 + (int)strlen(proposed) + 1 + 1;
                unique = (char*)malloc(ndigits * sizeof(char));
                sprintf(unique, "%s_%d", proposed, ref_id);
                addMalloc(logmalloclist, unique, ndigits, sizeof(char), "char");
                return (unique);
            }
        }
    }
    return (proposed);
}

void updateUdt(HGROUPS* hgroups, USERDEFINEDTYPELIST* userdefinedtypelist)
{
    int i, j, k;
    for (i = 0; i < hgroups->numgrps; i++) {
        if (hgroups->groups[i].udtIndex >= 0) {
            hgroups->groups[i].udt = &userdefinedtypelist->userdefinedtype[hgroups->groups[i].udtIndex];
        }
        for (j = 0; j < hgroups->groups[i].numatts; j++) {
            if (hgroups->groups[i].attribute[j].udtIndex >= 0) {
                hgroups->groups[i].attribute[j].udt =
                        &userdefinedtypelist->userdefinedtype[hgroups->groups[i].attribute[j].udtIndex];
            }
        }
        for (j = 0; j < hgroups->groups[i].numvars; j++) {
            if (hgroups->groups[i].variable[j].udtIndex >= 0) {
                hgroups->groups[i].variable[j].udt =
                        &userdefinedtypelist->userdefinedtype[hgroups->groups[i].variable[j].udtIndex];
            }
            for (k = 0; k < hgroups->groups[i].variable[j].numatts; k++) {
                if (hgroups->groups[i].variable[j].attribute[k].udtIndex >= 0) {
                    hgroups->groups[i].variable[j].attribute[k].udt =
                            &userdefinedtypelist->userdefinedtype[hgroups->groups[i].variable[j].attribute[k].udtIndex];
                }
            }
        }
    }
}

// Identify UDT that are identical in all detail except for name
// Consolidate type names to the first registered name
// 	    STR_EQUALS(userdefinedtypelist->userdefinedtype[j].name, proposed_name)){

void repeatUdt(USERDEFINEDTYPELIST* userdefinedtypelist)
{
    int i, j, k, m, jj, kk;
    for (i = 0; i < userdefinedtypelist->listCount; i++) {
        char* proposed_name = userdefinedtypelist->userdefinedtype[i].name;
        int proposed_size = userdefinedtypelist->userdefinedtype[i].size;
        int proposed_fieldcount = userdefinedtypelist->userdefinedtype[i].fieldcount;
        for (j = i + 1; j < userdefinedtypelist->listCount; j++) {
            if (userdefinedtypelist->userdefinedtype[j].size == proposed_size &&
                userdefinedtypelist->userdefinedtype[j].fieldcount == proposed_fieldcount &&
                strcmp(userdefinedtypelist->userdefinedtype[j].name, proposed_name) != 0) {
                int test = 1;
                for (k = 0; k < proposed_fieldcount; k++) {
                    COMPOUNDFIELD* proposed_field = &userdefinedtypelist->userdefinedtype[i].compoundfield[k];
                    COMPOUNDFIELD* field = &userdefinedtypelist->userdefinedtype[j].compoundfield[k];
                    test = test &&
                           proposed_field->size == field->size &&
                           proposed_field->offset == field->offset &&
                           proposed_field->offpad == field->offpad &&
                           proposed_field->alignment == field->alignment &&
                           proposed_field->atomictype == field->atomictype &&
                           proposed_field->pointer == field->pointer &&
                           proposed_field->rank == field->rank &&
                           proposed_field->count == field->count &&
                           (field->atomictype == 1 ||
                            (field->atomictype > 0 && STR_EQUALS(proposed_field->type, field->type))) &&
                           STR_EQUALS(proposed_field->name, field->name) &&
                           STR_EQUALS(proposed_field->desc, field->desc);

                    if (!test) break;

                    if (test && field->rank > 1 && field->shape != nullptr) {    // Check array shape
                        test = proposed_field != nullptr;
                        for (m = 0; m < field->rank; m++) test = test && (proposed_field->shape[m] == field->shape[m]);
                    }
                    if (!test) break;
                }
                if (!test) break;

// Adopt the proposed UDT type name
// Forward check all UDTs and change all field types that use the original type name to the proposed type name
// If a current field type is also the UDT type then change that also

                for (jj = j;
                     jj < userdefinedtypelist->listCount; jj++) {                // Current and Forward check all UDTs
                    for (kk = 0; kk < userdefinedtypelist->userdefinedtype[jj].fieldcount; kk++) {    // Check all items
                        COMPOUNDFIELD* field = &userdefinedtypelist->userdefinedtype[jj].compoundfield[kk];
                        if (field->atomictype == 0 && STR_EQUALS(field->type, proposed_name)) {
                            strcpy(field->type, proposed_name);
                        }
                    }
                }
                strcpy(userdefinedtypelist->userdefinedtype[j].name,
                       proposed_name);    // Adopt the type name

            }
        }
    }
}

int findUserDefinedTypeIndexReverse(USERDEFINEDTYPELIST* userdefinedtypelist, int ref_id)
{        // Identify via reference type id, search in reverse order
    int i;
    for (i = userdefinedtypelist->listCount - 1; i >= 0; i--) {
        if (userdefinedtypelist->userdefinedtype[i].ref_id == ref_id) return (i);
    }
    return -1;
}

// Target all User Defined Types within the Data Tree starting at the Root Node
// grouplist contains all group ids up to and including the sub-tree node.
// Pass grouplist = nullptr to target the current group only

int getCDF4SubTreeUserDefinedTypes(int grpid, GROUPLIST* grouplist, USERDEFINEDTYPELIST* userdefinedtypelist)
{
    int i, err = 0;
    if (grouplist != nullptr) {
        for (i = 0; i < grouplist->count; i++) {        // All groups in scope
            if ((err = readCDFTypes(grouplist->grpids[i], userdefinedtypelist)) != 0) return err;
        }
    } else {
        return (readCDFTypes(grpid, userdefinedtypelist));     // All UDTypes in a single group
    }
    return err;
}

void replaceSubTreeEmbeddedStrings(LOGMALLOCLIST* logmalloclist, USERDEFINEDTYPELIST* userdefinedtypelist,
                                   USERDEFINEDTYPE* udt, int ndata, char* dvec)
{
    // If the data structure has a User Defined Type with a NC_STRING component,
    // or contains other user defined types with string components,
    // then intercept and replace with locally allocated strings.
    //
    // VLEN types also need heap replacement

    if (udt == nullptr) {
        return;
    }

    int j;
    for (j = 0; j < udt->fieldcount; j++) {
        UDA_LOG(UDA_LOG_DEBUG, "\nfieldcount %d\n", udt->fieldcount);

        if (udt->compoundfield[j].atomictype == UDA_TYPE_UNKNOWN) {    // Child User Defined type?
            UDA_LOG(UDA_LOG_DEBUG, "\nUDT\n");

            char* data;
            USERDEFINEDTYPE* child = findUserDefinedType(userdefinedtypelist, udt->compoundfield[j].type, 0);
            int nstr = udt->compoundfield[j].count;            // Number of sub-structures in each array element

            if (udt->idamclass == UDA_TYPE_VLEN) {
                VLENTYPE* vlen = (VLENTYPE*)dvec;            // If the type is VLEN then read data array for the count
                int k;
                for (k = 0; k < ndata; k++) {
                    replaceSubTreeEmbeddedStrings(logmalloclist, userdefinedtypelist, child, vlen[k].len,
                                                  (char*)vlen[k].data);
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
                int k;
                for (k = 0; k < ndata; k++) {
                    // Loop over each structure array element
                    data = &dvec[k * udt->size] + udt->compoundfield[j].offset * sizeof(char);
                    replaceSubTreeEmbeddedStrings(logmalloclist, userdefinedtypelist, child, nstr, data);
                }
            }
            continue;
        }

        if (STR_EQUALS(udt->compoundfield[j].type, "STRING *")) {
            UDA_LOG(UDA_LOG_DEBUG, "\nSTRING *, ndata %d\n", ndata);

            // String arrays within data structures are defined (readCDFTypes) as char *str[int] => rank=1, pointer=0 and type=STRING*

            int lstr;
            char** svec;
            char** ssvec;

            int nstr = udt->compoundfield[j].count;        // Number of strings to replace

            // Allocate new local heap for each string array element
            // new heap is freed via the malloc log

            ssvec = (char**)malloc((size_t)nstr * sizeof(char*));

            int k;
            for (k = 0; k < ndata; k++) {                // Loop over each structure array element

                svec = (char**)(&dvec[k * udt->size] + udt->compoundfield[j].offset * sizeof(char));

                int i;
                for (i = 0; i < nstr; i++) {
                    lstr = (int)strlen(svec[i]) + 1;
                    ssvec[i] = (char*)malloc(lstr * sizeof(char));
                    addMalloc(logmalloclist, (void*)ssvec[i], lstr, sizeof(char), "char");
                    strcpy(ssvec[i], svec[i]);
                }
                UDA_LOG(UDA_LOG_DEBUG, "\nstring count %d\n", nstr);

                // *** BUG ? when calling for the same sub-tree from the same file twice !!!

                nc_free_string((size_t)nstr, svec);            // Free netCDF/HDF5 heap

                for (i = 0; i < nstr; i++) {
                    // Replace pointer within local heap pointer
                    svec[i] = ssvec[i];
                }
            }
            free((void*)ssvec);
            continue;
        }
    }

    UDA_LOG(UDA_LOG_DEBUG, "\nexiting\n");
}


int getCDF4SubTreeVarMeta(int grpid, int varid, VARIABLE* variable, USERDEFINEDTYPE* udt,
                          LOGMALLOCLIST* logmalloclist, USERDEFINEDTYPELIST* userdefinedtypelist)
{
    // Create the Hierarchical Structure Definition that describes the Variable, its Attributes and Dimensions
    // The new structure type has the same name as the variable (with an additional numeric key for uniqueness)
    // Variables are mapped into structures with attributes and a dimension string
    // Data are always pointer based structure elements

    int i, err = NC_NOERR, offset = 0;
    int numatts;
    char* unique;
    char name[NC_MAX_NAME + 1];
    size_t attlength;

    nc_type atttype;

    ATTRIBUTE* attribute;

    USERDEFINEDTYPE usertype;
    COMPOUNDFIELD field;

    // Ignore 'hidden' objects if requested

    unsigned short ignoreHiddenAtts = readCDF4Properties() & NC_IGNOREHIDDENATTS;
    unsigned short ignoreHiddenVars = readCDF4Properties() & NC_IGNOREHIDDENVARS;
    unsigned short ignoreHiddenGroups = readCDF4Properties() & NC_IGNOREHIDDENGROUPS;
    unsigned short ignoreHiddenDims = readCDF4Properties() & NC_IGNOREHIDDENDIMS;
    unsigned short notPointerType = readCDF4Properties() & NC_NOTPOINTERTYPE;
    unsigned short noDimensionData = readCDF4Properties() & NC_NODIMENSIONDATA;        // cf: getCDF4SubTreeVarData
    unsigned short noAttributeData = readCDF4Properties() & NC_NOATTRIBUTEDATA;
    unsigned short noVarAttributeData = readCDF4Properties() & NC_NOVARATTRIBUTEDATA;
    unsigned short noGrpAttributeData = readCDF4Properties() & NC_NOGROUPATTRIBUTEDATA;

    unsigned short regularVarData = (noVarAttributeData || noAttributeData) && noDimensionData;

    UDA_LOG(UDA_LOG_DEBUG, "getCDF4SubTreeVarMeta: Properties \n");
    UDA_LOG(UDA_LOG_DEBUG, "readCDF4Properties: %d\n", (int)readCDF4Properties());
    UDA_LOG(UDA_LOG_DEBUG, "ignoreHiddenAtts  : %d\n", (int)ignoreHiddenAtts);
    UDA_LOG(UDA_LOG_DEBUG, "ignoreHiddenVars  : %d\n", (int)ignoreHiddenVars);
    UDA_LOG(UDA_LOG_DEBUG, "ignoreHiddenGroups: %d\n", (int)ignoreHiddenGroups);
    UDA_LOG(UDA_LOG_DEBUG, "ignoreHiddenDims  : %d\n", (int)ignoreHiddenDims);
    UDA_LOG(UDA_LOG_DEBUG, "notPointerType    : %d\n", (int)notPointerType);
    UDA_LOG(UDA_LOG_DEBUG, "noDimensionData   : %d\n", (int)noDimensionData);
    UDA_LOG(UDA_LOG_DEBUG, "noAttributeData   : %d\n", (int)noAttributeData);
    UDA_LOG(UDA_LOG_DEBUG, "noVarAttributeData: %d\n", (int)noVarAttributeData);
    UDA_LOG(UDA_LOG_DEBUG, "noGrpAttributeData: %d\n", (int)noGrpAttributeData);
    UDA_LOG(UDA_LOG_DEBUG, "regularVarData    : %d\n", (int)regularVarData);

    //----------------------------------------------------------------------
    // Initialise

    initUserDefinedType(&usertype);    // New (sub)structure definition
    initVariable(variable);

    //----------------------------------------------------------------------
    // Variable Name, Type, etc.

    if ((err = nc_inq_var(grpid, varid, name, &variable->vartype, &variable->rank, variable->dimids, &numatts)) !=
        NC_NOERR) {
        addIdamError(CODEERRORTYPE, "readCDF", err, (char*)nc_strerror(err));
        return err;
    }

    if (ignoreHiddenVars && name[0] == NC_HIDDENPREFIX) return err;        // Ignore if hidden variable

    variable->varid = varid;
    strcpy(variable->varname, name);

    //----------------------------------------------------------------------
    // Create Variable Structure Definition

    usertype.idamclass = UDA_TYPE_COMPOUND;
    strcpy(usertype.name, name);

    unique = getUniqueTypeName(usertype.name, nameKey, logmalloclist, userdefinedtypelist);
    if (!STR_EQUALS(unique, usertype.name)) {
        // The name of the Variable alone is not guaranteed to be unique
        strcpy(usertype.name, unique);
        nameKey++;
    }
    unique = getUniqueTypeName(usertype.name, nameKey, logmalloclist,
                               userdefinedtypelist);        // Check the component types
    if (!STR_EQUALS(unique, usertype.name)) {
        strcpy(usertype.name, unique);
        nameKey++;
    }

    strcpy(usertype.source, "netcdf");
    usertype.ref_id = (int)variable->vartype + 90000;    // Make unique! Why? How is this unique?
    usertype.imagecount = 0;                // No Structure Image data
    usertype.image = nullptr;
    usertype.size = 0;                // Structure size

    //----------------------------------------------------------------------
    // Variable's Attributes

    if (numatts > 0 && (!noAttributeData || !noVarAttributeData)) {
        int attCount = 0;
        variable->numatts = numatts;
        attribute = (ATTRIBUTE*)malloc(numatts * sizeof(ATTRIBUTE));
        variable->attribute = attribute;

        for (i = 0; i < numatts; i++) {
            initAttribute(&attribute[i]);
            if ((err = nc_inq_attname(grpid, varid, i, name)) != NC_NOERR) {
                addIdamError(CODEERRORTYPE, "readCDF", err, (char*)nc_strerror(err));
                free((void*)variable->attribute);
                variable->attribute = nullptr;
                freeUserDefinedType(&usertype);
                return err;
            }

            if (ignoreHiddenAtts && name[0] == NC_HIDDENPREFIX) continue; // Skip this attribute

            attCount++;
            strcpy(attribute[i].attname, name);

            if ((err = nc_inq_atttype(grpid, varid, name, &atttype)) != NC_NOERR) {
                addIdamError(CODEERRORTYPE, "readCDF", err, (char*)nc_strerror(err));
                free((void*)variable->attribute);
                variable->attribute = nullptr;
                freeUserDefinedType(&usertype);
                return err;
            }
            attribute[i].atttype = atttype;
            if ((err = nc_inq_attlen(grpid, varid, name, (size_t*)&attlength)) != NC_NOERR) {
                addIdamError(CODEERRORTYPE, "readCDF", err, (char*)nc_strerror(err));
                free((void*)variable->attribute);
                variable->attribute = nullptr;
                freeUserDefinedType(&usertype);
                return err;
            }
            attribute[i].attlength = (int)attlength;

            initCompoundField(&field);
            strcpy(field.name, name);
            field.desc[0] = '\0';

            if (atttype == NC_CHAR) {                           // Legacy String (Numbers should be NC_BYTE)
                field.atomictype = UDA_TYPE_STRING;
                strcpy(field.type, "STRING");                   // Single String passed as type: char *
                field.pointer = 1;
                field.rank = 0;                                 // Scalar String - Irregular length
                field.count = 1;                                // 1 String
                field.shape = nullptr;
                field.size = getsizeof(userdefinedtypelist, "char *");
                field.offset = newoffset(offset, "char *");
                offset = field.offset + field.size;             // use previous offset for alignment
                field.offpad = padding(field.offset, "char *");
                field.alignment = getalignmentof("char *");
            } else if (atttype == NC_STRING) {
                field.atomictype = UDA_TYPE_STRING;
                strcpy(field.type, "STRING *");                 // String Array
                field.pointer = 1;                              // Passed as array type: char **
                field.rank = 0;
                field.count = 1;                                // Arbitrary Number of Strings of arbitrary length
                field.shape = nullptr;
                field.size = field.count * sizeof(char**);
                field.offset = newoffset(offset, field.type);
                offset = field.offset + field.size;             // use previous offset for alignment
                field.offpad = padding(field.offset, field.type);
                field.alignment = getalignmentof(field.type);
            } else {

                // Is this Type Atomic?

                if (isAtomicNCType(atttype)) {
                    field.atomictype = convertNCType(atttype);            // convert netCDF base type to IDAM type
                    strcpy(field.type,
                           udaNameType((UDA_TYPE)field.atomictype));        // convert atomic type to a string label
                } else {
                    field.atomictype = UDA_TYPE_UNKNOWN;

                    // Identify the required structure definition

                    attribute[i].udt = nullptr;                            // Update when all udts defined (realloc problem!)
                    attribute[i].udtIndex = findUserDefinedTypeIndexReverse(userdefinedtypelist,
                                                                            (int)atttype);    // Locate UDT definition using type id

                    if (attribute[i].udtIndex < 0) {
                        err = 999;
                        char* msg = (char*)malloc(sizeof(char) * 1024);
                        sprintf(msg,
                                "Variable %s Attribute %s has a User Defined or Atomic Type definition that is either Not within Scope "
                                "or Not Known or Not currently supported.", variable->varname, name);
                        addIdamError(CODEERRORTYPE, "readCDF4SubTree", err, msg);
                        free((void*)msg);
                        free((void*)variable->attribute);
                        variable->attribute = nullptr;
                        freeUserDefinedType(&usertype);
                        break;
                    }
                    strcpy(field.type, userdefinedtypelist->userdefinedtype[attribute[i].udtIndex].name);
                }

                // All numeric attribute arrays, including scalars, are passed as Pointer types.

                field.pointer = 1;                                  // All data are passed as Pointer types
                field.rank = 1;
                field.count = (int)attlength;
                field.shape = (int*)malloc(field.rank * sizeof(int));
                field.shape[0] = field.count;
                field.size = getsizeof(userdefinedtypelist, "void *");
                field.offset = newoffset(offset, "void *");
                offset = field.offset + field.size;                 // use previous offset for alignment
                field.offpad = padding(field.offset, "void *");
                field.alignment = getalignmentof("void *");
            }
            addCompoundField(&usertype, field);

        }   // End of FOR loop over i

        if (ignoreHiddenAtts && attCount < numatts) {    // Re-pack the attribute list dropping ignored attributes
            variable->numatts = attCount;
            int j = 0;
            for (i = 0; i < numatts; i++) {
                if (attribute[i].attname[0] != '\0') attribute[j++] = attribute[i];
            }
            if (attCount == 0 && variable->attribute != nullptr) {
                free((void*)variable->attribute);
                variable->attribute = nullptr;
            }
        }

    } // End of IF nummatts > 0

    //----------------------------------------------------------------------
    // Dimension Names, e.g., X=10, Y=20

    if (!noDimensionData) {
        initCompoundField(&field);
        strcpy(field.name, "dimensions");
        field.desc[0] = '\0';

        field.atomictype = UDA_TYPE_STRING;
        strcpy(field.type, "STRING");
        field.pointer = 1;
        field.rank = 0;
        field.count = 1;                                // Single string of arbitrary length passed as type: char *
        field.shape = nullptr;
        field.size = getsizeof(userdefinedtypelist, "char *");
        field.offset = newoffset(offset, "char *");
        offset = field.offset + field.size;             // use previous offset for alignment
        field.offpad = padding(field.offset, "char *");
        field.alignment = getalignmentof("char *");

        addCompoundField(&usertype, field);
    }

    //----------------------------------------------------------------------
    // Variable Data: Either Atomic or Structured or String
    // (Strings have a different field encoding pattern!)

    initCompoundField(&field);
    strcpy(field.name, "data");                // Use the generic
    field.desc[0] = '\0';

    // Is this Type Atomic?

    if (variable->vartype == NC_CHAR) {             // Numbers should be encoded as NC_BYTE
        field.atomictype = UDA_TYPE_STRING;
        strcpy(field.type, "STRING");               // Single String of arbitrary length
        field.pointer = 1;
        field.rank = 0;                             // Scalar string with an irregular length
        field.count = 1;                            // 1 String
        field.shape = nullptr;
        field.size = getsizeof(userdefinedtypelist, "char *");
        field.offset = newoffset(offset, "char *");
        offset = field.offset + field.size;         // use previous offset for alignment
        field.offpad = padding(field.offset, "char *");
        field.alignment = getalignmentof("char *");
    } else if (variable->vartype == NC_STRING) {        // Passed as array type: char **
        field.atomictype = UDA_TYPE_STRING;
        strcpy(field.type, "STRING *");                 // String Array
        field.pointer = 1;
        field.rank = 0;                                 // Irregular length string
        field.count = 1;                                // Unknown number of Unknown length (at this time!)
        field.shape = nullptr;

        field.size = field.count * sizeof(char**);
        field.offset = newoffset(offset, field.type);
        offset = field.offset + field.size;             // use previous offset for alignment
        field.offpad = padding(field.offset, field.type);
        field.alignment = getalignmentof(field.type);
    } else {
        if (isAtomicNCType(variable->vartype)) {
            field.atomictype = convertNCType(variable->vartype);        // convert netCDF base type to IDAM type
            strcpy(field.type, udaNameType((UDA_TYPE)field.atomictype));        // convert atomic type to a string label
        } else {
            field.atomictype = UDA_TYPE_UNKNOWN;

            // Identify the required structure definition

            variable->udt = nullptr;                            // Update when all udts defined (realloc problem!)
            variable->udtIndex = findUserDefinedTypeIndexReverse(userdefinedtypelist, (int)variable->vartype);

            if (variable->udtIndex < 0) {
                err = 999;
                char* msg = (char*)malloc(sizeof(char) * 1024);
                sprintf(msg, "Variable %s has a User Defined or Atomic Type definition that is either Not within Scope "
                             "or Not Known or Not currently supported.", variable->varname);
                addIdamError(CODEERRORTYPE, "readCDF4SubTree", err, msg);
                free((void*)msg);
                free((void*)variable->attribute);
                variable->attribute = nullptr;
                freeUserDefinedType(&usertype);
                return err;
            }
            strcpy(field.type, userdefinedtypelist->userdefinedtype[variable->udtIndex].name);
        }

        // All arrays are passed as Pointer types.

        field.pointer = 1;
        field.rank = 1;
        field.count = 1;
        field.shape = (int*)malloc(field.rank * sizeof(int));
        field.shape[0] = field.count;
        field.size = getsizeof(userdefinedtypelist, "void *");
        field.offset = newoffset(offset, "void *");
        offset = field.offset + field.size;                // use previous offset for alignment
        field.offpad = padding(field.offset, "void *");
        field.alignment = getalignmentof("void *");
    }

    addCompoundField(&usertype, field);

    //----------------------------------------------------------------------
    // Update Structure Size

    usertype.size = newoffset(offset, "void *");

    // Record this type definition

    *udt = usertype;

    // Repeat the check that the name is unique

    unique = getUniqueTypeName(usertype.name, nameKey, logmalloclist, userdefinedtypelist);
    if (strcmp(unique, usertype.name) !=
        0) {                    // The name of the Variable alone is not guaranteed to be unique
        strcpy(usertype.name, unique);
        nameKey++;
    }
    unique = getUniqueTypeName(usertype.name, nameKey, logmalloclist,
                               userdefinedtypelist);    // Check the component types
    if (strcmp(unique, usertype.name) != 0) {
        strcpy(usertype.name, unique);
        nameKey++;
    }

    // Update Variable structure with type definition Index (Not the pointer as the array base memory changes with realloc)

    addUserDefinedType(userdefinedtypelist, usertype);

    variable->udtIndex = userdefinedtypelist->listCount - 1;

    return err;
}

int getCDF4SubTreeVar2Meta(int grpid, int varid, VARIABLE* variable, int* offset, COMPOUNDFIELD* field,
                           USERDEFINEDTYPELIST* userdefinedtypelist)
{
    // Meta data describing the variable on its own
    // The data can be either a pointer or an explicit array within the data structure
    // All variable attributes are ignored

    int err = NC_NOERR;
    int numatts;
    char name[NC_MAX_NAME + 1];

    // Ignore 'hidden' objects if requested

    unsigned short ignoreHiddenVars = readCDF4Properties() & NC_IGNOREHIDDENVARS;
    unsigned short notPointerType = readCDF4Properties() & NC_NOTPOINTERTYPE;

    UDA_LOG(UDA_LOG_DEBUG, "getCDF4SubTreeVar2Meta: Properties \n");
    UDA_LOG(UDA_LOG_DEBUG, "ignoreHiddenVars  : %d\n", (int)ignoreHiddenVars);
    UDA_LOG(UDA_LOG_DEBUG, "notPointerType    : %d\n", (int)notPointerType);

    //----------------------------------------------------------------------
    // Initialise

    initVariable(variable);

    //----------------------------------------------------------------------
    // Variable Name, Type, etc.

    if ((err = nc_inq_var(grpid, varid, name, &variable->vartype, &variable->rank, variable->dimids, &numatts)) !=
        NC_NOERR) {
        addIdamError(CODEERRORTYPE, "getCDF4SubTreeVar2Meta", err, (char*)nc_strerror(err));
        return err;
    }

    if (ignoreHiddenVars && name[0] == NC_HIDDENPREFIX) return err;        // Ignore if hidden variable

    variable->varid = varid;
    strcpy(variable->varname, name);

    //----------------------------------------------------------------------
    // Variable Data: Either Atomic or Structured or String
    // (Strings have a different field encoding pattern!)

    initCompoundField(field);
    strcpy(field->name, variable->varname);
    field->desc[0] = '\0';

    // Is this Type Atomic?

    // DGM TODO: Modify the encoding for Non-Pointer based strings and atomic data
    //           Non-Pointer based data means additional queries to establish lengths
    //           Alternatively, metadata describing the structure to be returned, with fixed array lengths, could be written when the data are written
    //           and used to define the returned structure. This would avoid problems with ragged arrays allowing standardised data structures to be used.
    //           Groups could have a hidden attribute defining how the group's data map into the structure.
    //           Recursion would register child structures ahead of parent structures.
    //           A USERDEFINEDTYPE based attribute is necessary - ITER IMAS IDS structures would be defined by the client application or a server plugin.

    if (variable->vartype == NC_CHAR) {                // Numbers should be encoded as NC_BYTE
        field->atomictype = UDA_TYPE_STRING;
        strcpy(field->type, "STRING");                // Single String of arbitrary length
        field->pointer = 1;
        field->rank = 0;                    // Scalar string with an irregular length
        field->count = 1;                    // 1 String
        field->shape = nullptr;
        field->size = getsizeof(userdefinedtypelist, "char *");
        field->offset = newoffset(*offset, "char *");
        *offset = field->offset + field->size;        // use previous offset for alignment
        field->offpad = padding(field->offset, "char *");
        field->alignment = getalignmentof("char *");
    } else if (variable->vartype == NC_STRING) {                // Passed as array type: char **
        field->atomictype = UDA_TYPE_STRING;
        strcpy(field->type, "STRING *");                // String Array
        field->pointer = 1;
        field->rank = 0;                    // Irregular length string
        field->count = 1;                    // Unknown number of Unknown length (at this time!)
        field->shape = nullptr;
        field->size = field->count * sizeof(char**);
        field->offset = newoffset(*offset, field->type);
        *offset = field->offset + field->size;        // use previous offset for alignment
        field->offpad = padding(field->offset, field->type);
        field->alignment = getalignmentof(field->type);
    } else {

        if (isAtomicNCType(variable->vartype)) {
            field->atomictype = convertNCType(variable->vartype);        // convert netCDF base type to IDAM type
            strcpy(field->type,
                   udaNameType((UDA_TYPE)field->atomictype));        // convert atomic type to a string label
        } else {
            field->atomictype = UDA_TYPE_UNKNOWN;                // Structured Type

            // Identify the required structure definition

            variable->udt = nullptr;                        // Update when all udts defined (realloc problem!)
            variable->udtIndex = findUserDefinedTypeIndexReverse(userdefinedtypelist, (int)variable->vartype);

            if (variable->udtIndex < 0) {
                err = 999;
                char* msg = (char*)malloc(sizeof(char) * 1024);
                sprintf(msg, "Variable %s has a User Defined or Atomic Type definition that is either Not within Scope "
                             "or Not Known or Not currently supported.", variable->varname);
                addIdamError(CODEERRORTYPE, "getCDF4SubTreeVar2Meta", err, msg);
                free((void*)msg);
                return err;
            }
            strcpy(field->type, userdefinedtypelist->userdefinedtype[variable->udtIndex].name);
        }

        // All arrays are passed as Pointer types.

        if (!notPointerType) {
            field->pointer = 1;
            field->rank = 1;
            field->count = 1;
            //field->shape      = (int *)malloc(field->rank*sizeof(int));
            //field->shape[0]   = field->count;
            field->size = getsizeof(userdefinedtypelist, "void *");
            field->offset = newoffset(*offset, "void *");
            *offset = field->offset + field->size;
            field->offpad = padding(field->offset, "void *");
            field->alignment = getalignmentof("void *");
        } else {

            // dgm TODO: check sizes for UDT arrays
            field->pointer = 0;
            field->rank = variable->rank;        // Need to know the dimensions etc for the count, rank and shape
            field->count = 1;
            if (field->atomictype == UDA_TYPE_UNKNOWN) {
                field->size = userdefinedtypelist->userdefinedtype[variable->udtIndex].size;
            } else {
                field->size = field->count * getsizeof(userdefinedtypelist, field->type);
            }
            field->offset = newoffset(*offset,
                                      field->type); // Need a new or modified function for structures, offpad and alignment?
            *offset = field->offset + field->size;
            field->offpad = padding(field->offset, field->type);
            field->alignment = getalignmentof(field->type);
        }
    }

    return err;
}


int getCDF4SubTreeMeta(int grpid, int parent, USERDEFINEDTYPE* udt, LOGMALLOCLIST* logmalloclist,
                       USERDEFINEDTYPELIST* userdefinedtypelist, HGROUPS* hgroups, int* depth, int targetDepth)
{
    // Create the Hierarchical Structure Definitions that describes the sub-tree
    // Ensure distinct structures (types) with the same name are uniquely identifiable.
    // If the structure definition is defined in a hidden attribute and the

    int i, j, err = NC_NOERR, offset = 0;
    int varid;
    int numgrps, numatts, numvars;
    int* grpids = nullptr, * varids;
    size_t attlength;

    char* unique;
    char name[NC_MAX_NAME + 1];

    nc_type atttype;
    GROUP Group;
    GROUP* group = &Group;

    int grp;

    USERDEFINEDTYPE usertype, gusertype;        // usertype is the current group UDT, gusertype are child group UDTs
    COMPOUNDFIELD field;

    //----------------------------------------------------------------------

    initUserDefinedType(&usertype);    // New (sub)structure definition
    initGroup(group);

    //----------------------------------------------------------------------
    // Add local User Defined Types

    if (getCDF4SubTreeUserDefinedTypes(grpid, nullptr, userdefinedtypelist) != NC_NOERR) {
        err = 999;
        addIdamError(CODEERRORTYPE, "readCDF4SubTree", err, "User Defined Type definition Problem!!");
        return err;
    }

    //----------------------------------------------------------------------
    // Groups within this node

    // Ignore 'hidden' groups if requested

    unsigned short ignoreHiddenAtts = readCDF4Properties() & NC_IGNOREHIDDENATTS;
    unsigned short ignoreHiddenVars = readCDF4Properties() & NC_IGNOREHIDDENVARS;
    unsigned short ignoreHiddenGroups = readCDF4Properties() & NC_IGNOREHIDDENGROUPS;
    unsigned short notPointerType = readCDF4Properties() & NC_NOTPOINTERTYPE; // Return data as explicitly sized arrays
    unsigned short noDimensionData = readCDF4Properties() & NC_NODIMENSIONDATA; // cf: getCDF4SubTreeVarData
    unsigned short noAttributeData = readCDF4Properties() & NC_NOATTRIBUTEDATA;
    unsigned short noVarAttributeData = readCDF4Properties() & NC_NOVARATTRIBUTEDATA;
    unsigned short regularVarData = (noVarAttributeData || noAttributeData) &&
                                    noDimensionData;    // Return variables as regular arrays not structures

    int attCount = 0;

    // All Child Groups

    numgrps = 0;

    if (nc_inq_grps(grpid, &numgrps, nullptr) == NC_NOERR) {
        if (numgrps > 0) {
            grpids = (int*)malloc(sizeof(int) * numgrps);
            if ((err = nc_inq_grps(grpid, &numgrps, grpids)) != NC_NOERR) {
                free((void*)grpids);
                return err;
            }

            if (ignoreHiddenGroups) {    // Reduce the list to exclude 'hidden' child groups
                int grpCount = 0;
                for (i = 0; i < numgrps; i++) {
                    if ((err = nc_inq_grpname(grpids[i], name)) == NC_NOERR && name[0] != NC_HIDDENPREFIX) {
                        grpids[grpCount++] = grpids[i];
                    }
                }
                numgrps = grpCount;
            }

            UDA_LOG(UDA_LOG_DEBUG, "getCDF4SubTreeMeta: Group List\n");
            for (i = 0; i < numgrps; i++) {
                if ((err = nc_inq_grpname(grpids[i], name)) != NC_NOERR)
                    UDA_LOG(UDA_LOG_DEBUG, "[%d]: %s\n", i, name);
            }
            err = 0;
        }
        if ((err = nc_inq_grpname(grpid, name)) != NC_NOERR) {
            addIdamError(CODEERRORTYPE, "readCDF", err, (char*)nc_strerror(err));
            if (numgrps > 0)free((void*)grpids);
            return err;
        }
        if (STR_EQUALS(name, "/")) strcpy(name, "root");
    }

    group->grpid = grpid;
    group->parent = parent;
    group->numgrps = numgrps;
    strcpy(group->grpname, name);
    if (numgrps > 0) group->grpids = grpids;

    usertype.idamclass = UDA_TYPE_COMPOUND;
    strcpy(usertype.source, "netcdf");
    usertype.ref_id = 0;
    usertype.imagecount = 0;                // No Structure Image data
    usertype.image = nullptr;
    usertype.size = 0;                // Structure size

    strcpy(usertype.name, name);

    unique = getUniqueTypeName(usertype.name, nameKey, logmalloclist, userdefinedtypelist);
    if (strcmp(unique, usertype.name) !=
        0) {                    // The name of the Group alone is not guaranteed to be unique
        strcpy(usertype.name, unique);
        nameKey++;
    }
    unique = getUniqueTypeName(usertype.name, nameKey, logmalloclist,
                               userdefinedtypelist);    // Check the component types
    if (strcmp(unique, usertype.name) != 0) {
        strcpy(usertype.name, unique);
        nameKey++;
    }

    // *** Note on duplicate name issues:
    // If the group contains a variable or other attribute that is the same as the current group, this will not
    // be trapped at this point because the user type has not yet been added to the list and child groups have
    // not yet been searched. Simple solution is to repeat the check prior to registration.

    //----------------------------------------------------------------------
    // Global Attributes within this node

    varid = NC_GLOBAL;
    numatts = 0;

    if (!noAttributeData && (err = nc_inq_varnatts(grpid, varid, &numatts)) == NC_NOERR) {
        if (numatts > 0) {
            group->numatts = numatts;
            group->attribute = (ATTRIBUTE*)malloc(numatts * sizeof(ATTRIBUTE));
        }
        for (i = 0; i < numatts; i++) {
            initAttribute(&group->attribute[i]);
            if ((err = nc_inq_attname(grpid, varid, i, name)) != NC_NOERR) {
                addIdamError(CODEERRORTYPE, "readCDF", err, (char*)nc_strerror(err));
                if (numgrps > 0) {
                    free((void*)grpids);
                }
                if (numatts > 0) {
                    free((void*)group->attribute);
                }
                freeUserDefinedType(&usertype);
                return err;
            }

            if (ignoreHiddenAtts && name[0] == NC_HIDDENPREFIX) continue;        // Skip this attribute if 'hidden'

            attCount++;
            strcpy(group->attribute[i].attname, name);

            if ((err = nc_inq_atttype(grpid, varid, name, &atttype)) != NC_NOERR) {
                addIdamError(CODEERRORTYPE, "readCDF", err, (char*)nc_strerror(err));
                if (numgrps > 0) {
                    free((void*)grpids);
                }
                if (numatts > 0) {
                    free((void*)group->attribute);
                }
                freeUserDefinedType(&usertype);
                return err;
            }
            group->attribute[i].atttype = atttype;
            if ((err = nc_inq_attlen(grpid, varid, name, (size_t*)&attlength)) != NC_NOERR) {
                addIdamError(CODEERRORTYPE, "readCDF", err, (char*)nc_strerror(err));
                if (numgrps > 0) {
                    free((void*)grpids);
                }
                if (numatts > 0) {
                    free((void*)group->attribute);
                }
                freeUserDefinedType(&usertype);
                return err;
            }
            group->attribute[i].attlength = (int)attlength;

            initCompoundField(&field);
            strcpy(field.name, name);
            field.desc[0] = '\0';

            if (atttype == NC_CHAR) {                    // Numbers should be NC_BYTE
                field.atomictype = UDA_TYPE_STRING;
                strcpy(field.type, "STRING");                // Single String passed as type: char *
                field.pointer = 1;
                field.rank = 0;                    // Irregular length string
                field.count = 1;                    // 1 String !
                field.shape = nullptr;
                field.size = getsizeof(userdefinedtypelist, "char *");
                field.offset = newoffset(offset, "char *");
                offset = field.offset +
                         field.size;                                            // use previous offset for alignment
                field.offpad = padding(field.offset, "char *");
                field.alignment = getalignmentof("char *");
            } else if (atttype == NC_STRING) {
                field.atomictype = UDA_TYPE_STRING;
                strcpy(field.type, "STRING *");                // String Array
                field.pointer = 1;                    // Passed as array type: char **
                field.rank = 0;
                field.count = 1;                    // Arbitrary Number of Strings of arbitrary length
                field.shape = nullptr;
                field.size = field.count * sizeof(char**);
                field.offset = newoffset(offset, field.type);
                offset = field.offset +
                         field.size;                                            // use previous offset for alignment
                field.offpad = padding(field.offset, field.type);
                field.alignment = getalignmentof(field.type);
            } else {

// Is this Type Atomic?

                if (isAtomicNCType(atttype)) {
                    field.atomictype = convertNCType(atttype);            // convert netCDF base type to IDAM type
                    strcpy(field.type,
                           udaNameType((UDA_TYPE)field.atomictype));        // convert atomic type to a string label
                } else {
                    field.atomictype = UDA_TYPE_UNKNOWN;

// Identify the required structure definition (must be defined as a User Defined Type)

                    group->attribute[i].udt = nullptr;                            // Update when all udts defined (realloc problem!)
                    group->attribute[i].udtIndex = findUserDefinedTypeIndexReverse(userdefinedtypelist,
                                                                                   (int)atttype);    //

                    if (group->attribute[i].udtIndex < 0) {
                        err = 999;
                        char* msg = (char*)malloc(sizeof(char) * 1024);
                        sprintf(msg,
                                "Group %s Attribute %s has a User Defined or Atomic Type definition that is either Not within Scope "
                                "or Not Known or Not currently supported.", group->grpname, name);
                        addIdamError(CODEERRORTYPE, "readCDF4SubTree", err, msg);
                        free((void*)msg);
                        if (numgrps > 0)free((void*)grpids);
                        if (numatts > 0)free((void*)group->attribute);
                        freeUserDefinedType(&usertype);
                        return err;
                    }
                    strcpy(field.type, userdefinedtypelist->userdefinedtype[group->attribute[i].udtIndex].name);
                }

                if (!notPointerType) {
                    field.pointer = 1;
                    field.rank = 1;
                    field.count = (int)attlength;
                    field.shape = (int*)malloc(field.rank * sizeof(int));
                    field.shape[0] = field.count;
                    field.size = getsizeof(userdefinedtypelist, "void *");
                    field.offset = newoffset(offset, "void *");
                    offset = field.offset +
                             field.size;                                            // use previous offset for alignment
                    field.offpad = padding(field.offset, "void *");
                    field.alignment = getalignmentof("void *");
                } else {
                    field.pointer = 0;
                    field.rank = 1;
                    field.count = (int)attlength;
                    field.shape = (int*)malloc(field.rank * sizeof(int));
                    field.shape[0] = field.count;
                    field.size = getsizeof(userdefinedtypelist, field.type);
                    field.offset = newoffset(offset, field.type);
                    offset = field.offset + field.size;
                    field.offpad = padding(field.offset, field.type);
                    field.alignment = getalignmentof(field.type);
                }
            }
            addCompoundField(&usertype, field);

        } // End of FOR loop over i

        if (ignoreHiddenAtts && attCount < numatts) {    // Re-pack the attribute list dropping ignored attributes
            group->numatts = attCount;
            int j = 0;
            for (i = 0; i < numatts; i++) {
                if (group->attribute[i].attname[0] != '\0') group->attribute[j++] = group->attribute[i];
            }
            if (attCount == 0 && group->attribute != nullptr) {
                free((void*)group->attribute);
                group->attribute = nullptr;
            }
        }

    }  // End of IF varnatts


//----------------------------------------------------------------------
// Variables within this node

    numvars = 0;

    if ((err = nc_inq_varids(grpid, &numvars, nullptr)) == NC_NOERR) {
        if (numvars > 0) {
            int varCount = 0;
            VARIABLE* variable = (VARIABLE*)malloc(sizeof(VARIABLE) * numvars);
            varids = (int*)malloc(sizeof(int) * numvars);
            group->numvars = numvars;
            group->variable = variable;
            if ((err = nc_inq_varids(grpid, &numvars, varids)) != NC_NOERR) {
                addIdamError(CODEERRORTYPE, "readCDF", err, (char*)nc_strerror(err));
                freeUserDefinedType(&usertype);
                free((void*)variable);
                free((void*)varids);
                return err;
            }

            for (j = 0; j < numvars; j++) {

                if (regularVarData) {        // Just add the variable array ingoring attributes and the dimension doc string

                    if ((err = getCDF4SubTreeVar2Meta(grpid, varids[j], &variable[j], &offset, &field,
                                                      userdefinedtypelist)) != NC_NOERR) {
                        addIdamError(CODEERRORTYPE, "readCDF", err, "getCDF4SubTreeVar2Meta error");
                        freeUserDefinedType(&gusertype);
                        free((void*)variable);
                        free((void*)varids);
                        return err;
                    }

                    if (ignoreHiddenVars && variable[j].varname[0] == NC_HIDDENPREFIX) {
                        continue;
                    }        // Skip this variable if 'hidden'

                    varCount++;
                    addCompoundField(&usertype, field);        // Add the variable to the sub-tree structure

                } else {

                    if ((err = getCDF4SubTreeVarMeta(grpid, varids[j], &variable[j], &gusertype,
                                                     logmalloclist, userdefinedtypelist)) != NC_NOERR) {
                        addIdamError(CODEERRORTYPE, "readCDF", err, "getCDF4SubTreeVarMeta error");
                        freeUserDefinedType(&gusertype);
                        free((void*)variable);
                        free((void*)varids);
                        return err;
                    }

                    if (ignoreHiddenVars && variable[j].varname[0] == NC_HIDDENPREFIX) {
                        continue;
                    }        // Skip this variable if 'hidden'

                    varCount++;

                    initCompoundField(&field);

// dgm 13May2011	Use the variable name instead of the type name
                    //strcpy(field.name, gusertype.name);

                    strcpy(field.name, variable[j].varname);    // Variable name (may not be unique)

                    field.atomictype = UDA_TYPE_UNKNOWN;
                    strcpy(field.type, gusertype.name);        // Unique data type

                    field.desc[0] = '\0';
                    field.pointer = 1;
                    field.size = getsizeof(userdefinedtypelist, "void *");
                    field.offset = newoffset(offset, "void *");
                    offset = field.offset + field.size;         // use previous offset for alignment
                    field.offpad = padding(field.offset, "void *");
                    field.alignment = getalignmentof("void *");

                    field.rank = 1;
                    field.count = 1;
                    field.shape = (int*)malloc(field.rank * sizeof(int));
                    field.shape[0] = 1;

                    addCompoundField(&usertype, field);
                }

            }   // End of FOR loop over j

            if (ignoreHiddenVars && varCount < numvars) {    // Re-pack the variable list dropping ignored variables
                group->numvars = varCount;
                int jj = 0;
                for (j = 0; j < numvars; j++) {
                    if (variable[j].varname[0] != '\0') variable[jj++] = variable[j];
                }
            }

            free((void*)varids);
        }      // End of IF numvars
    }

    //----------------------------------------------------------------------
    // Save this Group Meta Data

    grp = addHGroup(hgroups, Group);

    //----------------------------------------------------------------------
    // For each child group Recursively Drill down and repeat

    if (*depth != targetDepth) {

        UDA_LOG(UDA_LOG_DEBUG, "getCDF4SubTreeMeta: Drill down tree\n");

        *depth = *depth + 1;

        for (i = 0; i < numgrps; i++) {

            getCDF4SubTreeMeta(grpids[i], grpid, &gusertype, logmalloclist, userdefinedtypelist, hgroups, depth,
                               targetDepth);

            initCompoundField(&field);

            GROUP* x = findHGroup(hgroups, grpids[i]);    // Locate the group and it's name
            strcpy(field.name, x->grpname);

            field.atomictype = UDA_TYPE_UNKNOWN;
            strcpy(field.type, gusertype.name);        // This should be unique

            field.desc[0] = '\0';
            field.pointer = 1;
            field.size = getsizeof(userdefinedtypelist, "void *");
            field.offset = newoffset(offset, "void *");
            offset = field.offset + field.size;         // use previous offset for alignment
            field.offpad = padding(field.offset, "void *");
            field.alignment = getalignmentof("void *");

            field.rank = 1;
            field.count = 1;
            field.shape = (int*)malloc(field.rank * sizeof(int));
            field.shape[0] = 1;

            addCompoundField(&usertype, field);
        }

    }
    //----------------------------------------------------------------------
    // If Groups exist without data, add a hidden ignorable item to prevent problems

    if (usertype.fieldcount == 0) {
        initCompoundField(&field);
        sprintf(field.name, "%s", NC_EMPTY_GROUP_VAR_NAME);
        strcpy(field.desc, "Empty Group");
        field.atomictype = UDA_TYPE_STRING;
        strcpy(field.type, "STRING");                // Single String passed as type: char *
        field.pointer = 1;
        field.rank = 0;                    // Irregular length string
        field.count = 1;                    // 1 String !
        field.shape = nullptr;
        field.size = getsizeof(userdefinedtypelist, "char *");
        field.offset = newoffset(offset, "char *");
        offset = field.offset + field.size;         // use previous offset for alignment
        field.offpad = padding(field.offset, "char *");
        field.alignment = getalignmentof("char *");

        addCompoundField(&usertype, field);

    }

    //----------------------------------------------------------------------
    // Update Structure Size

    usertype.size = getStructureSize(userdefinedtypelist,
                                     &usertype); // This may return a size too small as any terminating packing bytes are ignored!

    // Record this type definition

    *udt = usertype;

    // Repeat the check that the name is unique

    unique = getUniqueTypeName(usertype.name, nameKey, logmalloclist, userdefinedtypelist);
    if (strcmp(unique, usertype.name) != 0) {
        // The name of the Group alone is not guaranteed to be unique
        strcpy(usertype.name, unique);
        nameKey++;
    }
    unique = getUniqueTypeName(usertype.name, nameKey, logmalloclist,
                               userdefinedtypelist);    // Check the component types
    if (strcmp(unique, usertype.name) != 0) {
        strcpy(usertype.name, unique);
        nameKey++;
    }

    // Update Variable structure with type definition Index (Not the pointer as the array base memory changes with realloc)

    addUserDefinedType(userdefinedtypelist, usertype);

    hgroups->groups[grp].udtIndex = userdefinedtypelist->listCount - 1;

    //----------------------------------------------------------------------

    return err;
}


//----------------------------------------------------------------------
//----------------------------------------------------------------------
//----------------------------------------------------------------------
//----------------------------------------------------------------------
// Read Data

char* dimShapeLabel(int grpid, int rank, int* dimids, int* count, int** shp)
{
    // Create a descriptive label with the dimension sizes and a shape array

    int i, slen;
    size_t length;
    int* shape = nullptr;
    char name[NC_MAX_NAME + 1];
    char work[2 * NC_MAX_NAME + 1];
    char* dim = nullptr;

    *count = 1;
    *shp = nullptr;
    if (rank > 1) shape = (int*)malloc(rank * sizeof(int));    // Added externally to MallocLog

    for (i = 0; i < rank; i++) {
        nc_inq_dim(grpid, dimids[i], name, &length);
        if (rank > 1) shape[i] = (int)length;                // Shape only required if rank > 1
        if (i == 0) {
            sprintf(work, "%s=%d", name, (int)length);
            slen = (int)strlen(work) + 1;
            dim = (char*)malloc(slen * sizeof(char));        // Added externally to MallocLog
            strcpy(dim, work);
        } else {
            sprintf(work, ", %s=%d", name, (int)length);
            slen = slen + (int)strlen(work) + 1;
            dim = (char*)realloc((void*)dim, slen * sizeof(char));
            strcat(dim, work);
        }
        *count = *count * (int)length;
    }
    *shp = shape;
    return dim;
}

ENUMLIST* getCDF4EnumList(int grpid, nc_type vartype, LOGMALLOCLIST* logmalloclist)
{
    int rc, err;
    char value[8];
    ENUMLIST* enumlist = (ENUMLIST*)malloc(sizeof(ENUMLIST));
    nc_type base;
    size_t size, members;
    char name[NC_MAX_NAME + 1];

    if ((rc = nc_inq_enum(grpid, vartype, name, &base, &size, &members)) != NC_NOERR) {
        err = 999;
        addIdamError(CODEERRORTYPE, "getCDF4EnumList", err, (char*)nc_strerror(rc));
        return nullptr;
    }

    if (members == 0) {
        err = 999;
        addIdamError(CODEERRORTYPE, "getCDF4EnumList", err, "Enumerated Type has Zero Membership?");
        return nullptr;
    }

    strcpy(enumlist->name, name);
    enumlist->type = convertNCType(base);
    enumlist->count = members;
    enumlist->enummember = (ENUMMEMBER*)malloc(members * sizeof(ENUMMEMBER));

    size_t i;
    for (i = 0; i < members; i++) {
        if ((rc = nc_inq_enum_member(grpid, vartype, i, enumlist->enummember[i].name, (void*)value)) != NC_NOERR) {
            err = 999;
            addIdamError(CODEERRORTYPE, "getCDF4EnumList", err, (char*)nc_strerror(rc));
            return nullptr;
        }
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

    // Add mallocs to list for freeing when data dispatch complete

    addMalloc(logmalloclist, (void*)enumlist, 1, sizeof(ENUMLIST), "ENUMLIST");
    addMalloc(logmalloclist, (void*)enumlist->enummember, members, sizeof(ENUMMEMBER), "ENUMMEMBER");

    return enumlist;
}


int getCDF4SubTreeVarData(int grpid, void** data, VARIABLE* variable, LOGMALLOCLIST* logmalloclist,
                          USERDEFINEDTYPELIST* userdefinedtypelist)
{
    // Read variable data
    // Organised as a structure with the variable's attributes and a string documenting the order and size of dimensions
    // If there are no attributes and the dimension string is to be 'ignored', the data are organised as they are saved
    // to netCDF4

    int rc, err = 0;
    int fieldid, varid;
    int count, size;

    char* d = nullptr;
    VOIDTYPE* p = nullptr;

    size_t startIndex[NC_MAX_DIMS];
    size_t countIndex[NC_MAX_DIMS];

    //----------------------------------------------------------------------
    // Allocate heap for this structure (or atomic array)

    *data = malloc(variable->udt->size);
    addMalloc(logmalloclist, *data, 1, variable->udt->size, variable->udt->name);

    // Start of the Structure

    char* p0 = *((char**)data);

    // Locate the next field (udt compound field must be synchronised to the group elements)

    fieldid = 0;

    //----------------------------------------------------------------------
    // Start Error Trap

    do {

        //----------------------------------------------------------------------
        // Read Attributes (arrays are always rank 1 if not NC_STRING when rank 2)

        varid = variable->varid;

        int i;
        for (i = 0; i < variable->numatts; i++) {

            // Locate the next pointer position within the structure

            p = (VOIDTYPE*)&p0[variable->udt->compoundfield[fieldid++].offset];
            *p = 0;

            // Regular

            if (variable->attribute[i].udt == nullptr) {

                if (variable->attribute[i].atttype == NC_CHAR || variable->attribute[i].atttype == NC_STRING) {
                    // String Atomic Type
                    int attlength = variable->attribute[i].attlength;
                    if (variable->attribute[i].atttype == NC_CHAR) {
                        attlength = attlength + 1;
                        d = (char*)malloc(attlength * sizeof(char));
                        addMalloc(logmalloclist, d, 1, attlength * sizeof(char), "char");
                        if ((err = nc_get_att_text(grpid, varid, variable->attribute[i].attname, d)) != NC_NOERR) {
                            err = 999;
                            addIdamError(CODEERRORTYPE, "readCDF4SubTree", err,
                                         "Problem Reading Legacy String Attribute");
                            break;
                        }
                        d[variable->attribute[i].attlength] = '\0';        // Ensure nullptr terminated
                    } else {
                        int istr, lstr;
                        char** svec = (char**)malloc((size_t)attlength *
                                                     sizeof(char*));    // The attribute length is the number of strings
                        if ((err = nc_get_att(grpid, varid, variable->attribute[i].attname, (void*)svec)) !=
                            NC_NOERR) {
                            err = 999;
                            addIdamError(CODEERRORTYPE, "readCDF4SubTree", err, "Problem Reading String Attribute");
                            break;
                        }

                        char** ssvec = (char**)malloc((size_t)attlength * sizeof(char*));// Array of strings
                        addMalloc(logmalloclist, (void*)ssvec, attlength, sizeof(char*), "STRING *");
                        for (istr = 0; istr < attlength; istr++) {                    // Copy into locally managed Heap
                            lstr = (int)strlen(svec[istr]) + 1;
                            d = (char*)malloc(lstr * sizeof(char));
                            addMalloc(logmalloclist, d, lstr, sizeof(char), "char");            // Individual strings
                            strcpy(d, svec[istr]);
                            ssvec[istr] = d;
                        }
                        *p = (VOIDTYPE)ssvec;                        // Slot String Array into structure

                        nc_free_string(attlength, svec);
                        free(svec);
                        d = nullptr;

                        // String arrays within data structures are defined (readCDFTypes) as char *str[int] => rank=1, pointer=0 and type=STRING*

                        // The expectation is a rank 1 char array of length attlength+1 to hold a Legacy string
                        // The NC_STRING object is a rank 1 array of length attlength of char * pointers to each string.
                        // The structure definition should be char*[]
                    }

                } else {
                    int idamType = convertNCType(variable->attribute[i].atttype);
                    if (idamType == UDA_TYPE_UNKNOWN) {
                        err = 999;
                        addIdamError(CODEERRORTYPE, "readCDF4SubTree", err,
                                     "Variable Attribute has Unknown Atomic Type (Not a User Defined Type)!");
                        break;
                    }
                    size = getsizeof(userdefinedtypelist, udaNameType((UDA_TYPE)idamType));
                    d = (char*)malloc(variable->attribute[i].attlength * size);
                    addMalloc(logmalloclist, d, variable->attribute[i].attlength, size,
                              udaNameType((UDA_TYPE)idamType));
                    if ((err = nc_get_att(grpid, varid, variable->attribute[i].attname, (void*)d)) != NC_NOERR) {
                        err = 999;
                        addIdamError(CODEERRORTYPE, "readCDF4SubTree", err,
                                     "Problem reading Variable Attribute Value!");
                        break;
                    }
                }

            } else {        // User Defined type
                if (variable->attribute[i].udt->idamclass != UDA_TYPE_ENUM) {
                    size = variable->attribute[i].udt->size;
                    d = (char*)malloc(variable->attribute[i].attlength * size);
                    addMalloc(logmalloclist, d, variable->attribute[i].attlength, size,
                              variable->attribute[i].udt->name);
                    if ((err = nc_get_att(grpid, varid, variable->attribute[i].attname, (void*)d)) != NC_NOERR) {
                        err = 999;
                        addIdamError(CODEERRORTYPE, "readCDF4SubTree", err,
                                     "Problem reading Variable Attribute Value!");
                        break;
                    }
                } else {
                    char value[8];
                    ENUMLIST* enumlist = (ENUMLIST*)malloc(sizeof(ENUMLIST));
                    nc_type base;
                    size_t size, members;
                    char name[NC_MAX_NAME + 1];

                    // **** Locate the group with the enum definition

                    if ((rc = nc_inq_enum(grpid, variable->attribute[i].atttype, name, &base, &size, &members)) !=
                        NC_NOERR) {
                        err = 999;
                        addIdamError(CODEERRORTYPE, "readCDF4SubTree", err, (char*)nc_strerror(rc));
                        break;
                    }

                    if (members == 0) {
                        err = 999;
                        addIdamError(CODEERRORTYPE, "readCDF4SubTree", err,
                                     "Enumerated Type has Zero Membership!");
                        break;
                    }

                    strcpy(enumlist->name, name);
                    enumlist->type = convertNCType(base);
                    enumlist->count = members;
                    enumlist->enummember = (ENUMMEMBER*)malloc(members * sizeof(ENUMMEMBER));

                    {
                        size_t j;
                        for (j = 0; j < members; j++) {
                            rc = nc_inq_enum_member(grpid, variable->attribute[i].atttype, j,
                                                    enumlist->enummember[j].name,
                                                    (void*)value);
                            switch (enumlist->type) {
                                case UDA_TYPE_CHAR: {
                                    char* enumvalue = (char*)value;
                                    enumlist->enummember[j].value = (long long)*enumvalue;
                                    break;
                                }
                                case UDA_TYPE_SHORT: {
                                    short* enumvalue = (short*)value;
                                    enumlist->enummember[j].value = (long long)*enumvalue;
                                    break;
                                }
                                case UDA_TYPE_UNSIGNED_SHORT: {
                                    unsigned short* enumvalue = (unsigned short*)value;
                                    enumlist->enummember[j].value = (long long)*enumvalue;
                                    break;
                                }
                                case UDA_TYPE_INT: {
                                    int* enumvalue = (int*)value;
                                    enumlist->enummember[j].value = (long long)*enumvalue;
                                    break;
                                }
                                case UDA_TYPE_UNSIGNED_INT: {
                                    unsigned int* enumvalue = (unsigned int*)value;
                                    enumlist->enummember[j].value = (long long)*enumvalue;
                                    break;
                                }
                                case UDA_TYPE_LONG64: {
                                    long long* enumvalue = (long long*)value;
                                    enumlist->enummember[j].value = (long long)*enumvalue;
                                    break;
                                }
                                case UDA_TYPE_UNSIGNED_LONG64: {
                                    unsigned long long* enumvalue = (unsigned long long*)value;
                                    enumlist->enummember[j].value = (long long)*enumvalue;
                                    break;
                                }
                            }
                        }
                    }

                    d = (char*)malloc(variable->attribute[i].attlength *
                                      sizeof(long long));    // Sufficient space ignoring integer type
                    if ((err = nc_get_att(grpid, varid, variable->attribute[i].attname, (void*)d)) != NC_NOERR) {
                        err = 999;
                        addIdamError(CODEERRORTYPE, "readCDF4SubTree", err,
                                     "Problem reading Variable Attribute Value!");
                        break;
                    }

                    // Change to standard unsigned long long integer type		    
                    unsigned long long* enumarray = (unsigned long long*)malloc(
                            variable->attribute[i].attlength * sizeof(unsigned long long));
                    switch (enumlist->type) {
                        case UDA_TYPE_CHAR: {
                            char* data = d;
                            int j;
                            for (j = 0; j < variable->attribute[i].attlength; j++) {
                                enumarray[j] = (unsigned long long)data[j];
                            }
                            break;
                        }
                        case UDA_TYPE_SHORT: {
                            short* data = (short*)d;
                            int j;
                            for (j = 0; j < variable->attribute[i].attlength; j++) {
                                enumarray[j] = (unsigned long long)data[j];
                            }
                            break;
                        }
                        case UDA_TYPE_UNSIGNED_SHORT: {
                            unsigned short* data = (unsigned short*)d;
                            int j;
                            for (j = 0; j < variable->attribute[i].attlength; j++) {
                                enumarray[j] = (unsigned long long)data[j];
                            }
                            break;
                        }
                        case UDA_TYPE_INT: {
                            int* data = (int*)d;
                            int j;
                            for (j = 0; j < variable->attribute[i].attlength; j++) {
                                enumarray[j] = (unsigned long long)data[j];
                            }
                            break;
                        }
                        case UDA_TYPE_UNSIGNED_INT: {
                            unsigned int* data = (unsigned int*)d;
                            int j;
                            for (j = 0; j < variable->attribute[i].attlength; j++) {
                                enumarray[j] = (unsigned long long)data[j];
                            }
                            break;
                        }
                        case UDA_TYPE_LONG64: {
                            long long* data = (long long*)d;
                            int j;
                            for (j = 0; j < variable->attribute[i].attlength; j++) {
                                enumarray[j] = (unsigned long long)data[j];
                            }
                            break;
                        }
                        case UDA_TYPE_UNSIGNED_LONG64: {
                            unsigned long long* data = (unsigned long long*)d;
                            int j;
                            for (j = 0; j < variable->attribute[i].attlength; j++) {
                                enumarray[j] = (unsigned long long)data[j];
                            }
                            break;
                        }
                    }
                    // enumlist->enumarray = (void*)d;
                    enumlist->enumarray = enumarray;
                    free((void*)d);

                    d = (char*)enumlist;

                    // Add mallocs to list for freeing when data dispatch complete

                    addMalloc(logmalloclist, (void*)enumlist, 1, sizeof(ENUMLIST), "ENUMLIST");
                    addMalloc(logmalloclist, (void*)enumlist->enummember, members, sizeof(ENUMMEMBER), "ENUMMEMBER");

                    //addMalloc(logmalloclist, (void*)enumlist->data, variable->attribute[i].attlength, size,
                    //          udaNameType(idamAtomicType(base)));            // Use true integer type
                    addMalloc(logmalloclist, (void*)enumlist->enumarray, variable->attribute[i].attlength,
                              sizeof(unsigned long long), "unsigned long long");

                    enumlist->enumarray_rank = 1;
                    enumlist->enumarray_count = variable->attribute[i].attlength;
                    enumlist->enumarray_shape = (int*)malloc(sizeof(int));
                    enumlist->enumarray_shape[0] = variable->attribute[i].attlength;

                    addMalloc(logmalloclist, (void*)enumlist->enumarray_shape, 1, sizeof(int), "int");

                }

                // String replacement

                replaceSubTreeEmbeddedStrings(logmalloclist, userdefinedtypelist, variable->attribute[i].udt,
                                              variable->attribute[i].attlength, d);

            }

            if (d != nullptr) {
                *p = (VOIDTYPE)d;
            }

        }

        //----------------------------------------------------------------------
        // Dimensions (Skip if requested they be ignored)

        count = 0;

        if (!(readCDF4Properties() & NC_NODIMENSIONDATA)) {
            p = (VOIDTYPE*)&p0[variable->udt->compoundfield[fieldid++].offset];
            *p = 0;
            *p = (VOIDTYPE)dimShapeLabel(grpid, variable->rank, variable->dimids, &count, &variable->shape);
            if ((void*)*p != nullptr) {
                // Shape added to Array heap
                addMalloc(logmalloclist, (void*)*p, 1, (int)(strlen((char*)*p) + 1) * sizeof(char), "char");
            }
        }

        //----------------------------------------------------------------------
        // Read Variable Data and expand enumerated types

        p = (VOIDTYPE*)&p0[variable->udt->compoundfield[fieldid].offset];
        *p = 0;

        if (variable->udt->compoundfield[fieldid].atomictype == UDA_TYPE_UNKNOWN) {        // Structure
            size = getsizeof(userdefinedtypelist, variable->udt->compoundfield[fieldid].type);
        } else {
            size = getsizeof(userdefinedtypelist,
                             udaNameType((UDA_TYPE)variable->udt->compoundfield[fieldid].atomictype));
        }

        if (size == 0) {
            err = 999;
            addIdamError(CODEERRORTYPE, "readCDF4SubTree", err, "Data Type has Zero Size!");
            break;
        }

        if (count != 0) {
            // Data are not always written!

            if (variable->rank > 1) {
                if (variable->shape == nullptr) {
                    err = 999;
                    addIdamError(CODEERRORTYPE, "readCDF4SubTree", err,
                                 "Unable to set Variable array shape!");
                    break;
                }
                for (i = 0; i < variable->rank; i++) {
                    startIndex[i] = 0;
                    countIndex[i] = variable->shape[i];
                }
            } else {
                startIndex[0] = 0;
                countIndex[0] = count;
            }

            if (STR_EQUALS(variable->udt->compoundfield[fieldid].type, "ENUMLIST")) {
                ENUMLIST* enumlist = getCDF4EnumList(grpid, variable->vartype, logmalloclist);
                size = getsizeof(userdefinedtypelist, udaNameType((UDA_TYPE)enumlist->type));
                d = (char*)malloc(count * size);                            // Enumerated Array
                if (d != nullptr) {
                    memset(d, 0, count * size);
                }
                //addMalloc2(logmalloclist, (void*)d, count, size, udaNameType(enumlist->type), variable->rank, variable->shape);
                if ((rc = nc_get_vara(grpid, varid, (size_t*)&startIndex, (size_t*)&countIndex, (void*)d)) !=
                    NC_NOERR) {
                    err = 999;
                    addIdamError(CODEERRORTYPE, "readCDF4SubTree", err, (char*)nc_strerror(rc));
                    addIdamError(CODEERRORTYPE, "readCDF4SubTree", err,
                                 "Unable to read Enumerated Array variable data!");
                    break;
                }

                // Change to standard unsigned long long integer type		    
                int j;
                unsigned long long* enumarray = (unsigned long long*)malloc(count * sizeof(unsigned long long));
                switch (enumlist->type) {
                    case UDA_TYPE_CHAR: {
                        char* data = (char*)d;
                        for (j = 0; j < count; j++) enumarray[j] = (unsigned long long)data[j];
                        break;
                    }
                    case UDA_TYPE_SHORT: {
                        short* data = (short*)d;
                        for (j = 0; j < count; j++) enumarray[j] = (unsigned long long)data[j];
                        break;
                    }
                    case UDA_TYPE_UNSIGNED_SHORT: {
                        unsigned short* data = (unsigned short*)d;
                        for (j = 0; j < count; j++) enumarray[j] = (unsigned long long)data[j];
                        break;
                    }
                    case UDA_TYPE_INT: {
                        int* data = (int*)d;
                        for (j = 0; j < count; j++) enumarray[j] = (unsigned long long)data[j];
                        break;
                    }
                    case UDA_TYPE_UNSIGNED_INT: {
                        unsigned int* data = (unsigned int*)d;
                        for (j = 0; j < count; j++) enumarray[j] = (unsigned long long)data[j];
                        break;
                    }
                    case UDA_TYPE_LONG64: {
                        long long* data = (long long*)d;
                        for (j = 0; j < count; j++) enumarray[j] = (unsigned long long)data[j];
                        break;
                    }
                    case UDA_TYPE_UNSIGNED_LONG64: {
                        unsigned long long* data = (unsigned long long*)d;
                        for (j = 0; j < count; j++) enumarray[j] = (unsigned long long)data[j];
                        break;
                    }
                }

                enumlist->enumarray = enumarray;
                addMalloc2(logmalloclist, (void*)enumlist->enumarray, count, sizeof(unsigned long long),
                           "unsigned long long",
                           variable->rank, variable->shape);

                enumlist->enumarray_rank = variable->rank;
                enumlist->enumarray_count = count;
                enumlist->enumarray_shape = (int*)malloc(sizeof(int));
                for (j = 0; j < variable->rank; j++)enumlist->enumarray_shape[j] = variable->shape[j];

                addMalloc(logmalloclist, (void*)enumlist->enumarray_shape, variable->rank, sizeof(int), "int");

                free((void*)d);

                d = (char*)enumlist;

            } else if (variable->vartype == NC_STRING) {

                if (variable->rank > 2) {
                    err = 999;
                    addIdamError(CODEERRORTYPE, "readCDFVar", err,
                                 "String Array rank is too large - must be <= 2");
                    break;
                }

                int istr, lstr;
                // Array of pointers to string array elements
                char** svec = (char**)malloc((size_t)count * sizeof(char*));

                if ((rc = nc_get_var_string(grpid, varid, svec)) != NC_NOERR) {
                    err = 999;
                    addIdamError(CODEERRORTYPE, "readCDF4SubTree", err, (char*)nc_strerror(rc));
                    addIdamError(CODEERRORTYPE, "readCDF4SubTree", err,
                                 "Unable to read String Array variable data!");
                    break;
                }

                char** ssvec = (char**)malloc((size_t)count * sizeof(char*));// Array of strings
                addMalloc(logmalloclist, (void*)ssvec, count, sizeof(char*), "STRING *");
                for (istr = 0; istr < count; istr++) {                    // Copy into locally managed Heap
                    lstr = (int)strlen(svec[istr]) + 1;
                    d = (char*)malloc(lstr * sizeof(char));
                    addMalloc(logmalloclist, d, lstr, sizeof(char), "char");            // Individual strings
                    strcpy(d, svec[istr]);
                    ssvec[istr] = d;
                }
                *p = (VOIDTYPE)ssvec;                        // Slot String Array into structure
                nc_free_string(count, svec);
                free(svec);
                d = nullptr;
                if (variable->shape != nullptr) {
                    addMalloc(logmalloclist, (void*)variable->shape, variable->rank, sizeof(int), "int");
                }

            } else {

                if (IMAS_HDF_READER && variable->vartype == NC_CHAR) {
                    // IMAS HDF5 strings are written in blocks of 132 bytes + string terminator
                    count = count * 132 + 1;
                }

                d = (char*)malloc(count * size);                            // Regular Array
                if (d != nullptr) {
                    // Assign char 0 to all bytes
                    memset(d, 0, count * size);
                }

                if (variable->vartype == NC_CHAR) {        // Legacy Single String => Reduce rank
                    if (variable->rank != 1) {
                        err = 999;
                        addIdamError(CODEERRORTYPE, "readCDF4SubTree", err,
                                     "Unable to read Array of Legacy Strings!");
                        break;
                    }

                    addMalloc2(logmalloclist, (void*)d, 1, count * size, variable->udt->compoundfield[fieldid].type,
                               variable->rank - 1, variable->shape);
                } else {
                    addMalloc2(logmalloclist, (void*)d, count, size, variable->udt->compoundfield[fieldid].type,
                               variable->rank,
                               variable->shape);
                }

                if ((rc = nc_get_vara(grpid, varid, (size_t*)&startIndex, (size_t*)&countIndex, (void*)d)) !=
                    NC_NOERR) {
                    err = 999;
                    addIdamError(CODEERRORTYPE, "readCDF4SubTree", err, (char*)nc_strerror(rc));
                    addIdamError(CODEERRORTYPE, "readCDF4SubTree", err, "Unable to read Regular Array variable data!");
                    break;
                }

                // String replacement

                USERDEFINEDTYPE* child = findUserDefinedType(userdefinedtypelist,
                                                             variable->udt->compoundfield[fieldid].type, 0);
                replaceSubTreeEmbeddedStrings(logmalloclist, userdefinedtypelist, child, count, d);
            }
        } else {
            d = nullptr;        // No Data
            if (variable->shape != nullptr) {
                addMalloc(logmalloclist, (void*)variable->shape, variable->rank, sizeof(int), "int");
            }
        }

        if (d != nullptr) {
            *p = (VOIDTYPE)d;
        }

        fieldid++;

        //----------------------------------------------------------------------
        // End of Error Trap

    } while (0);

//----------------------------------------------------------------------
// Housekeeping

    return err;
}

int getCDF4SubTreeVar2Data(int grpid, void** data, VARIABLE* variable, LOGMALLOCLIST* logmalloclist,
                           USERDEFINEDTYPELIST* userdefinedtypelist, COMPOUNDFIELD* field)
{

    // Read variable data as a stand alone element - attributes and the dimension doc string are ignored
    // Return a pointer to the heap allocated for the data

    // *** STRING arrays are not yet implemented

    int i, rc, err = 0;
    int count = 0, size = 0;

    char dimname[NC_MAX_NAME + 1], type_name[NC_MAX_NAME + 1];
    size_t dnum = 0;
    size_t startIndex[NC_MAX_DIMS];
    size_t countIndex[NC_MAX_DIMS];

    //----------------------------------------------------------------------
    // Error Trap

    do {

        //----------------------------------------------------------------------
        // Variable Type Size

        if (variable->udt != nullptr) {                                // Structure Type
            size = variable->udt->size;
            strcpy(type_name, variable->udt->name);
        } else if (isAtomicNCType(variable->vartype)) {                    // Atomic Type
            strcpy(type_name, udaNameType((UDA_TYPE)convertNCType(variable->vartype)));
            size = getsizeof(userdefinedtypelist, type_name);
        }

        if (size == 0) {
            err = 999;
            addIdamError(CODEERRORTYPE, "getCDF4SubTreeVar2Data", err, "Data Type has Zero Size!");
            break;
        }

        //----------------------------------------------------------------------
        // Variable Count and Shape

        if (variable->rank > 1 && variable->shape == nullptr) {
            variable->shape = (int*)malloc(variable->rank * sizeof(int));
            addMalloc(logmalloclist, (void*)variable->shape, variable->rank, sizeof(int), "int");
            for (i = 0; i < variable->rank; i++) {
                if ((rc = nc_inq_dim(grpid, variable->dimids[i], dimname, &dnum)) != NC_NOERR) {
                    err = NETCDF_ERROR_INQUIRING_DIM_3;
                    addIdamError(CODEERRORTYPE, "readCDF4SubTreeVar2Data", err,
                                 (char*)nc_strerror(rc));
                    break;
                }
                variable->shape[i] = (int)dnum;
            }
        }

        if (variable->rank <= 1) {
            if ((rc = nc_inq_dim(grpid, variable->dimids[0], dimname, &dnum)) != NC_NOERR) {
                err = NETCDF_ERROR_INQUIRING_DIM_3;
                addIdamError(CODEERRORTYPE, "readCDF4SubTreeVar2Data", err, (char*)nc_strerror(rc));
                break;
            }
            count = (int)dnum;
            startIndex[0] = 0;
            countIndex[0] = count;
        } else {
            count = 1;
            for (i = 0; i < variable->rank; i++) {
                startIndex[i] = 0;
                countIndex[i] = variable->shape[i];
                count = count * variable->shape[i];
            }
        }

        //----------------------------------------------------------------------
        // Read the Variable Data

        if (variable->vartype == NC_STRING) {

            if (variable->rank > 2) {
                err = 999;
                addIdamError(CODEERRORTYPE, "getCDF4SubTreeVar2Data", err,
                             "String Array rank is too large - must be <= 2");
                break;
            }

            int istr, lstr;
            char** svec = (char**)malloc(
                    (size_t)count * sizeof(char*));        // Array of pointers to string array elements
            if ((rc = nc_get_var_string(grpid, variable->varid, svec)) != NC_NOERR) {
                err = 999;
                addIdamError(CODEERRORTYPE, "getCDF4SubTreeVar2Data", err, (char*)nc_strerror(rc));
                addIdamError(CODEERRORTYPE, "getCDF4SubTreeVar2Data", err,
                             "Unable to read String Array variable data!");
                break;
            }

            char* d = nullptr;
            char** ssvec = (char**)malloc((size_t)count * sizeof(char*));// Array of strings
            addMalloc(logmalloclist, (void*)ssvec, count, sizeof(char*), "STRING *");
            for (istr = 0; istr < count; istr++) {                    // Copy into locally managed Heap
                lstr = (int)strlen(svec[istr]) + 1;
                d = (char*)malloc(lstr * sizeof(char));
                addMalloc(logmalloclist, d, lstr, sizeof(char), "char");            // Individual strings
                strcpy(d, svec[istr]);
                ssvec[istr] = d;
            }
            nc_free_string(count, svec);                    // Free internal netCDF4 heap allocation
            free(svec);

            *data = (void**)ssvec;                    // Return the data array

        } else {

            if (IMAS_HDF_READER && variable->vartype == NC_CHAR) {
                count = count * 132 +
                        1;
            } // IMAS HDF5 strings are written in blocks of 132 bytes + string terminator

            char* d = (char*)malloc(count * size);                // Regular Array of Atomic or User Defined Type
            if (d != nullptr) memset(d, 0, count * size);            // Assign char 0 to all bytes

            if (variable->vartype == NC_CHAR) {                // Legacy Single String => Reduce rank
                if (variable->rank != 1) {
                    err = 999;
                    addIdamError(CODEERRORTYPE, "getCDF4SubTreeVar2Data", err,
                                 "Unable to read Array of Legacy Strings!");
                    free((void*)d);
                    break;
                }
                addMalloc2(logmalloclist, (void*)d, 1, count * size, type_name, variable->rank - 1, variable->shape);
            } else {
                addMalloc2(logmalloclist, (void*)d, count, size, type_name, variable->rank, variable->shape);
            }

            if ((rc = nc_get_vara(grpid, variable->varid, (size_t*)&startIndex, (size_t*)&countIndex,
                                  (void*)d)) != NC_NOERR) {
                err = 999;
                addIdamError(CODEERRORTYPE, "getCDF4SubTreeVar2Data", err, (char*)nc_strerror(rc));
                addIdamError(CODEERRORTYPE, "getCDF4SubTreeVar2Data", err,
                             "Unable to read Regular Array variable data!");
                break;
            }

            // If the data has a User Defined Type with a NC_STRING component then intercept and replace with a locally allocated component

            if (variable->udt != nullptr) {
                replaceSubTreeEmbeddedStrings(logmalloclist, userdefinedtypelist, variable->udt, count, d);
            }

            *data = (void*)d;                        // Return the data array


        }

        //----------------------------------------------------------------------
        // End of Error Trap

    } while (0);

    return err;
}


int readCDF4SubTreeVar3Data(GROUPLIST grouplist, int varid, int rank, int* dimids, int** shape,
                            int* ndvec, int* data_type, char** data, LOGMALLOCLIST* logmalloclist,
                            USERDEFINEDTYPELIST* userdefinedtypelist, USERDEFINEDTYPE** udt)
{
    int grpid;

    int i;
    int rc;
    int err = 0;
    int ndata = 1;

    nc_type vartype;                    // NC types
    char dimname[NC_MAX_NAME + 1];
    char* dvec = nullptr;
    char** svec = nullptr;            // String array

    size_t* start = nullptr, * count = nullptr;
    size_t* dnums = nullptr;

    *udt = nullptr;                // Definition of User Defined Data Structures

    //----------------------------------------------------------------------
    // Default Group ID

    grpid = grouplist.grpid;

    //----------------------------------------------------------------------
    // Error Trap

    do {

        //----------------------------------------------------------------------
        // Shape of the Data Array from the dimensions

        if (rank <= 0) {
            rank = 1;
        }

        start = (size_t*)malloc(rank * sizeof(size_t));
        count = (size_t*)malloc(rank * sizeof(size_t));
        dnums = (size_t*)malloc(rank * sizeof(size_t));

        if (start == nullptr || count == nullptr || dnums == nullptr) {
            err = 999;
            addIdamError(CODEERRORTYPE, "readCDF4SubTreeVar3Data", err, "Unable to Allocate Heap for Shape Arrays");
            break;
        }

        for (i = 0; i < rank; i++) {
            if ((rc = nc_inq_dim(grpid, dimids[i], dimname, (size_t*)&dnums[i])) != NC_NOERR) {
                err = NETCDF_ERROR_INQUIRING_DIM_3;
                addIdamError(CODEERRORTYPE, "readCDF4SubTreeVar3Data", err, (char*)nc_strerror(rc));
                break;
            }

            ndata = ndata * (int)dnums[i];                // Total Count of Array Elements
            start[i] = 0;
            count[i] = (int)dnums[i];                    // Shape

        }

        //----------------------------------------------------------------------
        // Data Array Type

        if ((rc = nc_inq_vartype(grpid, varid, &vartype)) != NC_NOERR) {
            err = NETCDF_ERROR_INQUIRING_VARIABLE_3;
            addIdamError(CODEERRORTYPE, "readCDF4SubTreeVar3Data", err, (char*)nc_strerror(rc));
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
                *data_type = UDA_TYPE_STRING;
                dvec = (char*)malloc((size_t)ndata * sizeof(char));
                break;
            }

            case NC_STRING: {
                *data_type = UDA_TYPE_STRING;                    // Treated as a byte/char array
                svec = (char**)malloc(
                        (size_t)ndata * sizeof(char*));        // Array of pointers to string array elements
                break;
            }

                //----------------------------------------------------------------------
                // Must be a user defined type: COMPLEX, OPAQUE, VLEN, COMPOUND and ENUMERATED

            default: {

                if (vartype == ctype) {                        // known User Defined types
                    *data_type = UDA_TYPE_COMPLEX;
                    dvec = (char*)malloc((size_t)ndata * sizeof(COMPLEX));
                } else {
                    if (vartype == dctype) {
                        *data_type = UDA_TYPE_DCOMPLEX;
                        dvec = (char*)malloc((size_t)ndata * sizeof(DCOMPLEX));
                    } else {

                        // Create User defined data structure definitions: descriptions of the internal composition (Global Structure)

                        if ((err = scopedUserDefinedTypes(grpid)) != 0) {
                            break;
                        }

                        // Identify the required structure definition

                        *udt = findUserDefinedType(userdefinedtypelist, "",
                                                   (int)vartype);        // Identify via type id (assuming local to this group)

                        if (*udt == nullptr &&
                            grouplist.grpids !=
                            nullptr) {        // Must be defined elsewhere within scope of this group
                            int igrp;
                            for (igrp = 0; igrp < grouplist.count; igrp++) {
                                // Extend list to include all groups in scope
                                if (grouplist.grpids[igrp] != grpid) {
                                    if ((err = readCDFTypes(grouplist.grpids[igrp], userdefinedtypelist)) != 0) break;
                                }
                            }
                            if (err != 0) {
                                break;
                            }
                            *udt = findUserDefinedType(userdefinedtypelist, "",
                                                       (int)vartype);        // Should be found now if within scope
                        }

                        if (*udt == nullptr) {
                            err = 999;
                            addIdamError(CODEERRORTYPE, "readCDF4SubTreeVar3Data", err,
                                         "User Defined Type definition Not within Scope!");
                            break;
                        }

                        // Read the Data - All user defined structures are COMPOUND: No special treatment for VLEN etc.
                        // Check for consistency with the structure definitions

                        *data_type = UDA_TYPE_COMPOUND;

                        if ((*udt)->idamclass == UDA_TYPE_ENUM) {
                            dvec = (char*)malloc(
                                    ndata * sizeof(long long));    // Sufficient space ignoring integer type
                        } else {
                            dvec = (char*)malloc(ndata * (*udt)->size);        // Create heap for the data
                        }

                        break;
                    }

                }

            }
                break;

        }   // End of SWITCH

        if (err != 0) {
            break;
        }

        if (ndata == 0) {
            free((void*)dvec);
            dvec = nullptr;
        } else {
            if (dvec == nullptr && vartype != NC_STRING) {
                err = NETCDF_ERROR_ALLOCATING_HEAP_6;
                addIdamError(CODEERRORTYPE, "readCDF4SubTreeVar3Data", err,
                             "Unable to Allocate Heap Memory for the Data");
                break;
            }
        }

        if (ndata > 0) {
            if (rank > 0) {
                if (vartype == NC_STRING) {
                    if (rank > 1) {
                        err = 999;
                        addIdamError(CODEERRORTYPE, "readCDF4SubTreeVar3Data", err,
                                     "String Array rank is too large - must be 1");
                        break;
                    }
                    rc = nc_get_var_string(grpid, varid, svec);

                    // Pack the strings for compliance with the legacy middleware

                    replaceStrings(svec, &ndata, &dvec, (int*)dnums);    // Assume int* and size_t* are the same!!!
                } else {
                    rc = nc_get_vara(grpid, varid, start, count, (void*)dvec);

                    // If the data has a User Defined Type with a NC_STRING component then intercept and replace with a locally allocated component

                    replaceEmbeddedStrings(logmalloclist, userdefinedtypelist, *udt, ndata, dvec);
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
                addIdamError(CODEERRORTYPE, "readCDF4SubTreeVar3Data", err, (char*)nc_strerror(rc));
                break;
            }

            if (*udt != nullptr) {

                if ((*udt)->idamclass == UDA_TYPE_COMPOUND) {            // Compound Types
                    addMalloc(logmalloclist, dvec, ndata, (*udt)->size,
                              (*udt)->name);    // Free Data via Malloc Log List
                }

                if ((*udt)->idamclass == UDA_TYPE_ENUM) {        // Enumerated Types
                    char value[8];
                    ENUMLIST* enumlist = (ENUMLIST*)malloc(sizeof(ENUMLIST));
                    nc_type base;
                    size_t size, members;
                    char name[NC_MAX_NAME + 1];

                    if ((rc = nc_inq_enum(grpid, vartype, name, &base, &size, &members)) != NC_NOERR) {
                        err = 999;
                        addIdamError(CODEERRORTYPE, "readCDF4SubTreeVar3Data", err,
                                     (char*)nc_strerror(rc));
                        break;
                    }

                    if (members == 0) {
                        err = 999;
                        addIdamError(CODEERRORTYPE, "readCDF4SubTreeVar3Data", err,
                                     "Enumerated Type has Zero Membership?");
                        break;
                    }

                    strcpy(enumlist->name, name);
                    enumlist->type = convertNCType(base);
                    enumlist->count = (int)members;
                    enumlist->enummember = (ENUMMEMBER*)malloc(members * sizeof(ENUMMEMBER));

                    size_t i;
                    for (i = 0; i < members; i++) {
                        rc = nc_inq_enum_member(grpid, vartype, i, enumlist->enummember[i].name, (void*)value);
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
                    unsigned long long* enumarray = (unsigned long long*)malloc(ndata * sizeof(unsigned long long));
                    switch (enumlist->type) {
                        case UDA_TYPE_CHAR: {
                            char* data = dvec;
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
                    enumlist->enumarray = enumarray;
                    free((void*)dvec);

                    dvec = (char*)enumlist;

                    // Add mallocs to list for freeing when data dispatch complete

                    addMalloc(logmalloclist, (void*)enumlist, 1, sizeof(ENUMLIST), "ENUMLIST");
                    addMalloc(logmalloclist, (void*)enumlist->enummember, (int)members, sizeof(ENUMMEMBER),
                              "ENUMMEMBER");

                    //addMalloc(logmalloclist, (void*)enumlist->data, ndata, (int)size, udaNameType(idamAtomicType(base)));            // Use true integer type
                    addMalloc(logmalloclist, (void*)enumlist->enumarray, ndata, sizeof(unsigned long long),
                              "unsigned long long");

                    enumlist->enumarray_rank = 1;
                    enumlist->enumarray_count = ndata;
                    enumlist->enumarray_shape = (int*)malloc(sizeof(int));
                    enumlist->enumarray_shape[0] = ndata;

                    addMalloc(logmalloclist, (void*)enumlist->enumarray_shape, 1, sizeof(int), "int");

                }
            }    // End of if(*udt != nullptr)
        }        // End of if(ndata > 0)

        *data = dvec;            // Data Array
        *ndvec = ndata;            // Count

        if (rank > 1) {
            int* sh = (int*)malloc(rank * sizeof(int));        // return the shape of the array
            for (i = 0; i < rank; i++)sh[i] = dnums[i];
            *shape = sh;
        } else {
            *shape = nullptr;
        }


//----------------------------------------------------------------------
// End of Error Trap

    } while (0);

//----------------------------------------------------------------------
// Housekeeping

    if (start != nullptr) free((void*)start);
    if (count != nullptr) free((void*)count);
    if (dnums != nullptr) free((void*)dnums);
    if (err != 0 && dvec != nullptr) free((void*)dvec);

    return err;
}


int getCDF4SubTreeData(LOGMALLOCLIST* logmalloclist, USERDEFINEDTYPELIST* userdefinedtypelist, void** data,
                       GROUP* group, HGROUPS* hgroups, int attronly, int* depth, int targetDepth)
{

// Now Recursively walk the sub-tree to read all data

    int i, size, err = 0;
    int fieldid, varid;
    int fieldcount = group->udt->fieldcount;

    VOIDTYPE* p;
    GROUP* grp;

    //----------------------------------------------------------------------
    // Allocate heap for this structure

    *data = malloc(group->udt->size);
    addMalloc(logmalloclist, *data, 1, group->udt->size, group->udt->name);

    // Start address of the Structure

    char* p0 = *((char**)data);

    // Locate the next field (udt compound field must be synchronised to the group elements)

    fieldid = 0;

    //----------------------------------------------------------------------
    // Start Error Trap

    char* d = nullptr;

    do {

        //----------------------------------------------------------------------
        // Read Group Attributes

        varid = NC_GLOBAL;

        for (i = 0; i < group->numatts; i++) {

            // Check field synchronisation is bounded

            if (fieldid > fieldcount) {
                err = 999;
                addIdamError(CODEERRORTYPE, "readCDF4SubTree", err,
                             "Structure Field (attribute) Count Exceeded!");
                break;
            }

            // Locate the next data position within the structure (a pointer or an opaque block of bytes)

            p = (VOIDTYPE*)&p0[group->udt->compoundfield[fieldid++].offset];
            *p = 0;

            *p = 0;    // Set the pointer to nullptr (benign action if opaque block)

            // DGM TODO: set all bytes in opaque block to nullptr;

            // Regular array of data?

            if (group->attribute[i].udt == nullptr) {

                if (group->attribute[i].atttype == NC_CHAR ||
                    group->attribute[i].atttype == NC_STRING) {

                    int attlength = group->attribute[i].attlength;

                    if (group->attribute[i].atttype == NC_CHAR) {        // Legacy string

                        attlength = attlength + 1;
                        d = (char*)malloc(attlength * sizeof(char));
                        addMalloc(logmalloclist, d, 1, attlength * sizeof(char), "char");

                        if ((err = nc_get_att_text(group->grpid, varid, group->attribute[i].attname, d)) != NC_NOERR) {
                            err = 999;
                            addIdamError(CODEERRORTYPE, "readCDF4SubTree", err,
                                         "Problem Reading Legacy String Group Attribute");
                            break;
                        }

                        d[group->attribute[i].attlength] = '\0';        // Ensure nullptr terminated

                    } else {                        // Array of strings

                        int istr, lstr;
                        char** svec = (char**)malloc((size_t)attlength *
                                                     sizeof(char*));    // The attribute length is the number of strings
                        if ((err = nc_get_att(group->grpid, varid, group->attribute[i].attname, (void*)svec)) !=
                            NC_NOERR) {
                            err = 999;
                            addIdamError(CODEERRORTYPE, "readCDF4SubTree", err,
                                         "Problem Reading String Group Attribute");
                            break;
                        }

                        char** ssvec = (char**)malloc((size_t)attlength * sizeof(char*));// Array of strings
                        addMalloc(logmalloclist, (void*)ssvec, attlength, sizeof(char*), "STRING *");
                        for (istr = 0; istr < attlength; istr++) {                    // Copy into locally managed Heap
                            lstr = (int)strlen(svec[istr]) + 1;
                            d = (char*)malloc(lstr * sizeof(char));
                            addMalloc(logmalloclist, d, lstr, sizeof(char), "char");            // Individual strings
                            strcpy(d, svec[istr]);
                            ssvec[istr] = d;
                        }
                        *p = (VOIDTYPE)ssvec;                        // Slot String Array into structure

                        nc_free_string(attlength, svec);
                        free(svec);
                        d = nullptr;
                    }
                } else {    // Numeric data

                    int idamType = convertNCType(group->attribute[i].atttype);
                    if (idamType == UDA_TYPE_UNKNOWN) {
                        err = 999;
                        addIdamError(CODEERRORTYPE, "readCDF4SubTree", err,
                                     "Variable Attribute has Unknown Atomic Type or User Defined Type!");
                        break;
                    }
                    size = getsizeof(userdefinedtypelist, udaNameType((UDA_TYPE)idamType));
                    d = (char*)malloc(group->attribute[i].attlength * size);
                    addMalloc(logmalloclist, d, group->attribute[i].attlength, size, udaNameType((UDA_TYPE)idamType));
                    if ((err = nc_get_att(group->grpid, varid, group->attribute[i].attname, (void*)d)) != NC_NOERR) {
                        err = 999;
                        addIdamError(CODEERRORTYPE, "readCDF4SubTree", err,
                                     "Problem reading Variable Attribute Value!");
                        break;
                    }
                }
            } else {        // User Defined Type data array
                if (group->attribute[i].udt->idamclass != UDA_TYPE_ENUM) {
                    size = group->attribute[i].udt->size;
                    d = (char*)malloc(group->attribute[i].attlength * size);
                    addMalloc(logmalloclist, d, group->attribute[i].attlength, size, group->attribute[i].udt->name);
                    if ((err = nc_get_att(group->grpid, varid, group->attribute[i].attname, (void*)d)) != NC_NOERR) {
                        err = 999;
                        addIdamError(CODEERRORTYPE, "readCDF4SubTree", err,
                                     "Problem reading Variable Attribute Value!");
                        break;
                    }
                } else {

                    int rc;
                    char value[8];
                    ENUMLIST* enumlist = (ENUMLIST*)malloc(sizeof(ENUMLIST));
                    nc_type base;
                    size_t size, members;
                    char name[NC_MAX_NAME + 1];

                    // Locate the group with the enum type definition

                    if ((rc = nc_inq_enum(group->grpid, group->attribute[i].atttype, name, &base, &size, &members)) !=
                        NC_NOERR) {
                        err = 999;
                        addIdamError(CODEERRORTYPE, "readCDF4SubTree", err, (char*)nc_strerror(rc));
                        break;
                    }

                    if (members == 0) {
                        err = 999;
                        addIdamError(CODEERRORTYPE, "readCDF4SubTree", err,
                                     "Enumerated Type has Zero Membership!");
                        break;
                    }

                    strcpy(enumlist->name, name);
                    enumlist->type = convertNCType(base);
                    enumlist->count = members;
                    enumlist->enummember = (ENUMMEMBER*)malloc(members * sizeof(ENUMMEMBER));

                    {
                        size_t j;
                        for (j = 0; j < members; j++) {
                            rc = nc_inq_enum_member(group->grpid, group->attribute[i].atttype, j,
                                                    enumlist->enummember[j].name, (void*)value);
                            switch (enumlist->type) {
                                case UDA_TYPE_CHAR: {
                                    char* enumvalue = (char*)value;
                                    enumlist->enummember[j].value = (long long)*enumvalue;
                                    break;
                                }
                                case UDA_TYPE_SHORT: {
                                    short* enumvalue = (short*)value;
                                    enumlist->enummember[j].value = (long long)*enumvalue;
                                    break;
                                }
                                case UDA_TYPE_UNSIGNED_SHORT: {
                                    unsigned short* enumvalue = (unsigned short*)value;
                                    enumlist->enummember[j].value = (long long)*enumvalue;
                                    break;
                                }
                                case UDA_TYPE_INT: {
                                    int* enumvalue = (int*)value;
                                    enumlist->enummember[j].value = (long long)*enumvalue;
                                    break;
                                }
                                case UDA_TYPE_UNSIGNED_INT: {
                                    unsigned int* enumvalue = (unsigned int*)value;
                                    enumlist->enummember[j].value = (long long)*enumvalue;
                                    break;
                                }
                                case UDA_TYPE_LONG64: {
                                    long long* enumvalue = (long long*)value;
                                    enumlist->enummember[j].value = (long long)*enumvalue;
                                    break;
                                }
                                case UDA_TYPE_UNSIGNED_LONG64: {
                                    unsigned long long* enumvalue = (unsigned long long*)value;
                                    enumlist->enummember[j].value = (long long)*enumvalue;
                                    break;
                                }
                            }
                        }
                    }

                    d = (char*)malloc(group->attribute[i].attlength *
                                      sizeof(long long));    // Sufficient space ignoring integer type
                    if ((rc = nc_get_att(group->grpid, varid, group->attribute[i].attname, (void*)d)) != NC_NOERR) {
                        err = 999;
                        addIdamError(CODEERRORTYPE, "readCDF4SubTree", err,
                                     "Problem reading Variable Attribute Value!");
                        break;
                    }

                    // Change to standard unsigned long long integer type
                    unsigned long long* enumarray = (unsigned long long*)malloc(
                            group->attribute[i].attlength * sizeof(unsigned long long));

                    int j;
                    switch (enumlist->type) {
                        case UDA_TYPE_CHAR: {
                            char* data = d;
                            for (j = 0; j < group->attribute[i].attlength; j++)
                                enumarray[j] = (unsigned long long)data[j];
                            break;
                        }
                        case UDA_TYPE_SHORT: {
                            short* data = (short*)d;
                            for (j = 0; j < group->attribute[i].attlength; j++)
                                enumarray[j] = (unsigned long long)data[j];
                            break;
                        }
                        case UDA_TYPE_UNSIGNED_SHORT: {
                            unsigned short* data = (unsigned short*)d;
                            for (j = 0; j < group->attribute[i].attlength; j++)
                                enumarray[j] = (unsigned long long)data[j];
                            break;
                        }
                        case UDA_TYPE_INT: {
                            int* data = (int*)d;
                            for (j = 0; j < group->attribute[i].attlength; j++)
                                enumarray[j] = (unsigned long long)data[j];
                            break;
                        }
                        case UDA_TYPE_UNSIGNED_INT: {
                            unsigned int* data = (unsigned int*)d;
                            for (j = 0; j < group->attribute[i].attlength; j++)
                                enumarray[j] = (unsigned long long)data[j];
                            break;
                        }
                        case UDA_TYPE_LONG64: {
                            long long* data = (long long*)d;
                            for (j = 0; j < group->attribute[i].attlength; j++)
                                enumarray[j] = (unsigned long long)data[j];
                            break;
                        }
                        case UDA_TYPE_UNSIGNED_LONG64: {
                            unsigned long long* data = (unsigned long long*)d;
                            for (j = 0; j < group->attribute[i].attlength; j++)
                                enumarray[j] = (unsigned long long)data[j];
                            break;
                        }
                    }
                    // enumlist->data = (void*)d;
                    enumlist->enumarray = enumarray;
                    free((void*)d);

                    d = (char*)enumlist;

                    // Add mallocs to list for freeing when data dispatch complete

                    addMalloc(logmalloclist, (void*)enumlist, 1, sizeof(ENUMLIST), "ENUMLIST");
                    addMalloc(logmalloclist, (void*)enumlist->enummember, members, sizeof(ENUMMEMBER), "ENUMMEMBER");
                    //addMalloc(logmalloclist, (void*)enumlist->data, group->attribute[i].attlength, size,
                    //          udaNameType(idamAtomicType(base)));            // Use true integer type
                    addMalloc(logmalloclist, (void*)enumlist->enumarray, group->attribute[i].attlength,
                              sizeof(unsigned long long), "unsigned long long");

                    enumlist->enumarray_rank = 1;
                    enumlist->enumarray_count = group->attribute[i].attlength;
                    enumlist->enumarray_shape = (int*)malloc(sizeof(int));
                    enumlist->enumarray_shape[0] = group->attribute[i].attlength;

                    addMalloc(logmalloclist, (void*)enumlist->enumarray_shape, 1, sizeof(int), "int");

                }

                // String replacement

                replaceSubTreeEmbeddedStrings(logmalloclist, userdefinedtypelist, group->attribute[i].udt,
                                              group->attribute[i].attlength, d);

            }

            if (d != nullptr) *p = (VOIDTYPE)d;        // Slot Data into structure

            // DGM TODO: All data are assumed pointer types - inconsistent with ITER IMAS IDS
            // DGM TODO: When the data is Not a pointer, ensure netCDF API writes directly into the structure to avoid a double copy.

        }

        if (!attronly) {
            //----------------------------------------------------------------------
            // Read Variables

            unsigned short noDimensionData = readCDF4Properties() & NC_NODIMENSIONDATA;
            unsigned short noAttributeData = readCDF4Properties() & NC_NOATTRIBUTEDATA;
            unsigned short noVarAttributeData = readCDF4Properties() & NC_NOVARATTRIBUTEDATA;

            unsigned short regularVarData = (noVarAttributeData || noAttributeData) && noDimensionData;

            for (i = 0; i < group->numvars; i++) {

                if (fieldid > fieldcount) {
                    err = 999;
                    addIdamError(CODEERRORTYPE, "readCDF4SubTree", err,
                                 "Structure Field (variable) Count Exceeded!");
                    break;
                }

                // DGM TODO: The offsets must match the size of the data objects returned from the netCDF API when the structures are Not pointer based.

                p = (VOIDTYPE*)&p0[group->udt->compoundfield[fieldid++].offset];
                *p = 0;

                if (regularVarData) {
                    if ((err = getCDF4SubTreeVar2Data(group->grpid, (void**)&d, &group->variable[i], logmalloclist,
                                                      userdefinedtypelist,
                                                      &(group->udt->compoundfield[fieldid - 1]))) != 0) {
                        return err;
                    }
                } else if ((err = getCDF4SubTreeVarData(group->grpid, (void**)&d, &group->variable[i], logmalloclist,
                                                        userdefinedtypelist)) != 0) {
                    return err;
                }

                *p = (VOIDTYPE)d;
            }
        }
        //----------------------------------------------------------------------
        // For each child group Recursively Drill down and repeat 
        if (*depth != targetDepth) {
            depth = depth + 1;

            for (i = 0; i < group->numgrps; i++) {
                if ((grp = findHGroup(hgroups, group->grpids[i])) != nullptr) {

                    if (fieldid > fieldcount) {
                        err = 999;
                        addIdamError(CODEERRORTYPE, "readCDF4SubTree", err, "Structure Field (group) Count Exceeded!");
                        break;
                    }

                    p = (VOIDTYPE*)&p0[group->udt->compoundfield[fieldid++].offset];
                    *p = 0;
                    if ((err = getCDF4SubTreeData(logmalloclist, userdefinedtypelist, (void**)&d, grp, hgroups,
                                                  attronly, depth, targetDepth)) !=
                        0) {
                        return err;
                    }
                    *p = (VOIDTYPE)d;
                } else {
                    err = 999;
                    addIdamError(CODEERRORTYPE, "readCDF4SubTree", err,
                                 "Problem locating Hierarchical Group Child!");
                    break;
                }
            }
        }

        //----------------------------------------------------------------------
        // Empty Groups

        if (group->numgrps == 0 && group->numvars == 0 && group->numatts == 0 &&
            group->udt->fieldcount == 1 && STR_EQUALS(group->udt->compoundfield[0].name, NC_EMPTY_GROUP_VAR_NAME)) {
            d = (char*)malloc(sizeof(char));
            addMalloc(logmalloclist, d, 1, sizeof(char), "char");
            d[0] = '\0';                // Ensure nullptr terminated
            p = (VOIDTYPE*)&p0[group->udt->compoundfield[0].offset];
            *p = 0;
            if (d != nullptr) *p = (VOIDTYPE)d;        // Slot Empty string into structure
        }

        //----------------------------------------------------------------------
        // End of Error Trap

    } while (0);

//----------------------------------------------------------------------
// Housekeeping

    return err;
}
