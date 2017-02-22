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

#include "readCDF4.h"

// Read netCDF4 sub-Tree

int idamAtomicType(nc_type type);

static int nameKey = 0;    // Create unique names by appending an incrementing integer

void initVariable(VARIABLE* variable)
{
    variable->varid = 0;
    variable->numatts = 0;
    variable->attribute = NULL;
    variable->varname[0] = '\0';
    variable->vartype = 0;
    variable->rank = 0;
    variable->udtIndex = -1;
    variable->shape = NULL;
    variable->udt = NULL;
}

void initAttribute(ATTRIBUTE* attribute)
{
    attribute->attid = 0;
    attribute->attname[0] = '\0';
    attribute->atttype = 0;
    attribute->attlength = 0;
    attribute->udtIndex = -1;
    attribute->udt = NULL;
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
    group->grpids = NULL;
    group->udt = NULL;
    group->attribute = NULL;
    group->variable = NULL;
}

void initHGroup(HGROUPS* hgroups)
{
    hgroups->numgrps = 0;
    hgroups->group = NULL;
}

static int listHGroupCount = 0;

int addHGroup(HGROUPS* hgroups, GROUP group)
{
    int udtIndex;
    if (hgroups->numgrps + 1 >= listHGroupCount) {
        listHGroupCount = listHGroupCount + GROWMALLOCLIST;
        hgroups->group = (GROUP*)realloc((void*)hgroups->group, (listHGroupCount) * sizeof(GROUP));
    }
    hgroups->group[hgroups->numgrps] = group;
    udtIndex = hgroups->numgrps;
    hgroups->numgrps++;
    return (udtIndex);
}

GROUP* findHGroup(HGROUPS* hgroups, int grpid)
{
    int i;
    for (i = 0; i < hgroups->numgrps; i++) {
        if (hgroups->group[i].grpid == grpid) return (&hgroups->group[i]);
    }
    return NULL;
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
    for (i = 0; i < hgroups->numgrps; i++) freeGroup(&hgroups->group[i]);
    free((void*)hgroups->group);
    listHGroupCount = 0;
    initHGroup(hgroups);
}

char* getUniqueTypeName(char* proposed, int ref_id, USERDEFINEDTYPELIST* userdefinedtypelist)
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
            addMalloc(unique, ndigits, sizeof(char), "char");
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
                addMalloc(unique, ndigits, sizeof(char), "char");
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
        if (hgroups->group[i].udtIndex >= 0) {
            hgroups->group[i].udt = &userdefinedtypelist->userdefinedtype[hgroups->group[i].udtIndex];
        }
        for (j = 0; j < hgroups->group[i].numatts; j++) {
            if (hgroups->group[i].attribute[j].udtIndex >= 0) {
                hgroups->group[i].attribute[j].udt =
                        &userdefinedtypelist->userdefinedtype[hgroups->group[i].attribute[j].udtIndex];
            }
        }
        for (j = 0; j < hgroups->group[i].numvars; j++) {
            if (hgroups->group[i].variable[j].udtIndex >= 0) {
                hgroups->group[i].variable[j].udt =
                        &userdefinedtypelist->userdefinedtype[hgroups->group[i].variable[j].udtIndex];
            }
            for (k = 0; k < hgroups->group[i].variable[j].numatts; k++) {
                if (hgroups->group[i].variable[j].attribute[k].udtIndex >= 0) {
                    hgroups->group[i].variable[j].attribute[k].udt =
                            &userdefinedtypelist->userdefinedtype[hgroups->group[i].variable[j].attribute[k].udtIndex];
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

                    if (test && field->rank > 1 && field->shape != NULL) {    // Check array shape
                        test = proposed_field != NULL;
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
                        if (field->atomictype == 0 && STR_EQUALS(field->type, proposed_name))
                            strcpy(field->type, proposed_name);
                    }
                }
                strcpy(userdefinedtypelist->userdefinedtype[j].name,
                       proposed_name);    // Adopt the type name

            }
        }
    }
}


int findUserDefinedTypeIndexReverse(int ref_id)
{        // Identify via reference type id, search in reverse order
    int i;
    for (i = userdefinedtypelist->listCount - 1; i >= 0; i--) {
        if (userdefinedtypelist->userdefinedtype[i].ref_id == ref_id) return (i);
    }
    return -1;
}

// Target all User Defined Types within the Data Tree starting at the Root Node
// grouplist contains all group ids up to and including the sub-tree node.
// Pass grouplist = NULL to target the current group only

int getCDF4SubTreeUserDefinedTypes(int grpid, GROUPLIST* grouplist, USERDEFINEDTYPELIST* userdefinedtypelist)
{
    int i, err = 0;
    if (grouplist != NULL) {
        for (i = 0; i < grouplist->count; i++) {        // All groups in scope
            if ((err = readCDFTypes(grouplist->grpids[i], userdefinedtypelist)) != 0) return err;
        }
    } else {
        return (readCDFTypes(grpid, userdefinedtypelist));     // All UDTypes in a single group
    }
    return err;
}

void replaceSubTreeEmbeddedStrings(USERDEFINEDTYPE* udt, int ndata, char* dvec)
{

// If the data structure has a User Defined Type with a NC_STRING component,
// or contains other user defined types with string components,
// then intercept and replace with locally allocated strings.
//
// VLEN types also need heap replacement

    int i, j, k, nstr;

    if (udt == NULL) return;

    for (j = 0; j < udt->fieldcount; j++) {
        IDAM_LOGF(LOG_DEBUG, "\nfieldcount %d\n", udt->fieldcount);

        if (udt->compoundfield[j].atomictype == TYPE_UNKNOWN) {    // Child User Defined type?
            IDAM_LOG(LOG_DEBUG, "\nUDT\n");

            char* data;
            USERDEFINEDTYPE* child = findUserDefinedType(udt->compoundfield[j].type, 0);
            nstr = udt->compoundfield[j].count;            // Number of sub-structures in each array element

            if (udt->idamclass == TYPE_VLEN) {
                VLENTYPE* vlen = (VLENTYPE*)dvec;            // If the type is VLEN then read data array for the count
                for (k = 0; k < ndata; k++) {
                    replaceSubTreeEmbeddedStrings(child, vlen[k].len, (void*)vlen[k].data);
                }

// Intercept VLEN netcdf/hdf5 heap and allocate local heap;

                void* vlendata;
                for (k = 0; k < ndata; k++) {
                    vlendata = malloc(vlen[k].len * child->size);
                    addMalloc(vlendata, vlen[k].len, child->size, child->name);
                    memcpy(vlendata, vlen[k].data, vlen[k].len * child->size);
                    free(vlen[k].data);                // Free netcdf/hdf5 allocated heap
                    vlen[k].data = vlendata;
                }

            } else {
                for (k = 0; k < ndata; k++) {                // Loop over each structure array element
                    data = &dvec[k * udt->size] + udt->compoundfield[j].offset * sizeof(char);
                    replaceSubTreeEmbeddedStrings(child, nstr, data);
                }
            }
            continue;
        }
//pfcoilType *y = (pfcoilType *)dvec;
        if (STR_EQUALS(udt->compoundfield[j].type, "STRING *")) {
            IDAM_LOGF(LOG_DEBUG, "\nSTRING *, ndata %d\n", ndata);

// String arrays within data structures are defined (readCDFTypes) as char *str[int] => rank=1, pointer=0 and type=STRING*

            int lstr;
            char** svec;
            char** ssvec;

            nstr = udt->compoundfield[j].count;        // Number of strings to replace

// Allocate new local heap for each string array element
// new heap is freed via the malloc log

            ssvec = (char**)malloc((size_t)nstr * sizeof(char*));

            for (k = 0; k < ndata; k++) {                // Loop over each structure array element

                svec = (char**)(&dvec[k * udt->size] + udt->compoundfield[j].offset * sizeof(char));

                for (i = 0; i < nstr; i++) {
                    lstr = (int)strlen(svec[i]) + 1;
                    ssvec[i] = (char*)malloc(lstr * sizeof(char));
                    addMalloc((void*)ssvec[i], lstr, sizeof(char), "char");
                    strcpy(ssvec[i], svec[i]);
                }
                IDAM_LOGF(LOG_DEBUG, "\nstring count %d\n", nstr);

// *** BUG ? when calling for the same sub-tree from the same file twice !!!

                nc_free_string(nstr, svec);            // Free netCDF/HDF5 heap

                for (i = 0; i < nstr; i++)svec[i] = ssvec[i];    // Replace pointer within local heap pointer
            }
            free((void*)ssvec);
            continue;
        }
    }

    IDAM_LOG(LOG_DEBUG, "\nexiting\n");
}


int getCDF4SubTreeVarMeta(int grpid, int varid, VARIABLE* variable, USERDEFINEDTYPE* udt,
                          USERDEFINEDTYPELIST* userdefinedtypelist)
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

    IDAM_LOG(LOG_DEBUG, "getCDF4SubTreeVarMeta: Properties \n");
    IDAM_LOGF(LOG_DEBUG, "readCDF4Properties: %d\n", (int)readCDF4Properties());
    IDAM_LOGF(LOG_DEBUG, "ignoreHiddenAtts  : %d\n", (int)ignoreHiddenAtts);
    IDAM_LOGF(LOG_DEBUG, "ignoreHiddenVars  : %d\n", (int)ignoreHiddenVars);
    IDAM_LOGF(LOG_DEBUG, "ignoreHiddenGroups: %d\n", (int)ignoreHiddenGroups);
    IDAM_LOGF(LOG_DEBUG, "ignoreHiddenDims  : %d\n", (int)ignoreHiddenDims);
    IDAM_LOGF(LOG_DEBUG, "notPointerType    : %d\n", (int)notPointerType);
    IDAM_LOGF(LOG_DEBUG, "noDimensionData   : %d\n", (int)noDimensionData);
    IDAM_LOGF(LOG_DEBUG, "noAttributeData   : %d\n", (int)noAttributeData);
    IDAM_LOGF(LOG_DEBUG, "noVarAttributeData: %d\n", (int)noVarAttributeData);
    IDAM_LOGF(LOG_DEBUG, "noGrpAttributeData: %d\n", (int)noGrpAttributeData);
    IDAM_LOGF(LOG_DEBUG, "regularVarData    : %d\n", (int)regularVarData);

//----------------------------------------------------------------------
// Initialise

    initUserDefinedType(&usertype);    // New (sub)structure definition
    initVariable(variable);

//----------------------------------------------------------------------
// Variable Name, Type, etc.

    if ((err = nc_inq_var(grpid, varid, name, &variable->vartype, &variable->rank, variable->dimids, &numatts)) !=
        NC_NOERR) {
        addIdamError(&idamerrorstack, CODEERRORTYPE, "readCDF", err, (char*)nc_strerror(err));
        return err;
    }

    if (ignoreHiddenVars && name[0] == NC_HIDDENPREFIX) return err;        // Ignore if hidden variable

    variable->varid = varid;
    strcpy(variable->varname, name);

//----------------------------------------------------------------------
// Create Variable Structure Definition

    usertype.idamclass = TYPE_COMPOUND;
    strcpy(usertype.name, name);

    unique = getUniqueTypeName(usertype.name, nameKey, userdefinedtypelist);
    if (!STR_EQUALS(unique, usertype.name)) {
        // The name of the Variable alone is not guaranteed to be unique
        strcpy(usertype.name, unique);
        nameKey++;
    }
    unique = getUniqueTypeName(usertype.name, nameKey, userdefinedtypelist);        // Check the component types
    if (!STR_EQUALS(unique, usertype.name)) {
        strcpy(usertype.name, unique);
        nameKey++;
    }

    strcpy(usertype.source, "netcdf");
    usertype.ref_id = (int)variable->vartype + 90000;    // Make unique! Why? How is this unique?
    usertype.imagecount = 0;                // No Structure Image data
    usertype.image = NULL;
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
                addIdamError(&idamerrorstack, CODEERRORTYPE, "readCDF", err, (char*)nc_strerror(err));
                free((void*)variable->attribute);
                variable->attribute = NULL;
                freeUserDefinedType(&usertype);
                return err;
            }

            if (ignoreHiddenAtts && name[0] == NC_HIDDENPREFIX) continue; // Skip this attribute

            attCount++;
            strcpy(attribute[i].attname, name);

            if ((err = nc_inq_atttype(grpid, varid, name, &atttype)) != NC_NOERR) {
                addIdamError(&idamerrorstack, CODEERRORTYPE, "readCDF", err, (char*)nc_strerror(err));
                free((void*)variable->attribute);
                variable->attribute = NULL;
                freeUserDefinedType(&usertype);
                return err;
            }
            attribute[i].atttype = atttype;
            if ((err = nc_inq_attlen(grpid, varid, name, (size_t*)&attlength)) != NC_NOERR) {
                addIdamError(&idamerrorstack, CODEERRORTYPE, "readCDF", err, (char*)nc_strerror(err));
                free((void*)variable->attribute);
                variable->attribute = NULL;
                freeUserDefinedType(&usertype);
                return err;
            }
            attribute[i].attlength = (int)attlength;

            initCompoundField(&field);
            strcpy(field.name, name);
            field.desc[0] = '\0';

            if (atttype == NC_CHAR) {                    // Legacy String (Numbers should be NC_BYTE)
                field.atomictype = TYPE_STRING;
                strcpy(field.type, "STRING");                // Single String passed as type: char *
                field.pointer = 1;
                field.rank = 0;                    // Scalar String - Irregular length
                field.count = 1;                    // 1 String
                field.shape = NULL;
                field.size = getsizeof("char *");
                field.offset = newoffset(offset, "char *");
                offset = field.offset +
                         field.size;                                            // use previous offset for alignment
                field.offpad = padding(field.offset, "char *");
                field.alignment = getalignmentof("char *");
            } else if (atttype == NC_STRING) {
                field.atomictype = TYPE_STRING;
                strcpy(field.type, "STRING *");                // String Array
                field.pointer = 1;                    // Passed as array type: char **
                field.rank = 0;
                field.count = 1;                    // Arbitrary Number of Strings of arbitrary length
                field.shape = NULL;
                field.size = field.count * sizeof(char**);
                field.offset = newoffset(offset, field.type);
                offset = field.offset +
                         field.size;                                            // use previous offset for alignment
                field.offpad = padding(field.offset, field.type);
                field.alignment = getalignmentof(field.type);
                /*
                	    field.pointer    = 0;					// Passed as array type: char *[attlength]
                	    field.rank       = 1;
                	    field.count      = attlength;				// Arbitrary Number of Strings
                	    field.shape      = (int *)malloc(field.rank*sizeof(int));	// Needed when rank >= 1
                	    field.shape[0]   = attlength;
                */
            } else {

// Is this Type Atomic?

                if (isAtomicNCType(atttype)) {
                    field.atomictype = convertNCType(atttype);            // convert netCDF base type to IDAM type
                    strcpy(field.type, idamNameType(field.atomictype));        // convert atomic type to a string label
                } else {
                    field.atomictype = TYPE_UNKNOWN;

// Identify the required structure definition

                    attribute[i].udt = NULL;                            // Update when all udts defined (realloc problem!)
                    attribute[i].udtIndex = findUserDefinedTypeIndexReverse(
                            (int)atttype);    // Locate UDT definition using type id

                    if (attribute[i].udtIndex < 0) {
                        err = 999;
                        char* msg = malloc(sizeof(char) * 1024);
                        sprintf(msg,
                                "Variable %s Attribute %s has a User Defined or Atomic Type definition that is either Not within Scope "
                                        "or Not Known or Not currently supported.", variable->varname, name);
                        addIdamError(&idamerrorstack, CODEERRORTYPE, "readCDF4SubTree", err, msg);
                        free((void*)msg);
                        free((void*)variable->attribute);
                        variable->attribute = NULL;
                        freeUserDefinedType(&usertype);
                        break;
                    }
                    strcpy(field.type, userdefinedtypelist->userdefinedtype[attribute[i].udtIndex].name);
                }

// All numeric attribute arrays, including scalars, are passed as Pointer types.

                field.pointer = 1;                    // All data are passed as Pointer types
                field.rank = 1;
                field.count = (int)attlength;
                field.shape = (int*)malloc(field.rank * sizeof(int));
                field.shape[0] = field.count;
                field.size = getsizeof("void *");
                field.offset = newoffset(offset, "void *");
                offset = field.offset +
                         field.size;                                            // use previous offset for alignment
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
            if (attCount == 0 && variable->attribute != NULL) {
                free((void*)variable->attribute);
                variable->attribute = NULL;
            }
        }

    } // End of IF nummatts > 0

//----------------------------------------------------------------------
// Dimension Names, e.g., X=10, Y=20

    if (!noDimensionData) {
        initCompoundField(&field);
        strcpy(field.name, "dimensions");
        field.desc[0] = '\0';

        field.atomictype = TYPE_STRING;
        strcpy(field.type, "STRING");
        field.pointer = 1;
        field.rank = 0;
        field.count = 1;        // Single string of arbitrary length passed as type: char *
        field.shape = NULL;
        field.size = getsizeof("char *");
        field.offset = newoffset(offset, "char *");
        offset = field.offset +
                 field.size;                                            // use previous offset for alignment
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

    if (variable->vartype == NC_CHAR) {                // Numbers should be encoded as NC_BYTE
        field.atomictype = TYPE_STRING;
        strcpy(field.type, "STRING");                // Single String of arbitrary length
        field.pointer = 1;
        field.rank = 0;                    // Scalar string with an irregular length
        field.count = 1;                    // 1 String
        field.shape = NULL;
        field.size = getsizeof("char *");
        field.offset = newoffset(offset, "char *");
        offset = field.offset +
                 field.size;                                            // use previous offset for alignment
        field.offpad = padding(field.offset, "char *");
        field.alignment = getalignmentof("char *");
// ********************************************************************************************
// Test

        // variable->rank = variable->rank-1;		// Rank reduced by 1 for strings

    } else if (variable->vartype == NC_STRING) {                // Passed as array type: char **
        field.atomictype = TYPE_STRING;
        strcpy(field.type, "STRING *");                // String Array
        field.pointer = 1;
        field.rank = 0;                    // Irregular length string
        field.count = 1;                    // Unknown number of Unknown length (at this time!)
        field.shape = NULL;

        field.size = field.count * sizeof(char**);
        field.offset = newoffset(offset, field.type);
        offset = field.offset +
                 field.size;                                            // use previous offset for alignment
        field.offpad = padding(field.offset, field.type);
        field.alignment = getalignmentof(field.type);
    } else {

        if (isAtomicNCType(variable->vartype)) {
            field.atomictype = convertNCType(variable->vartype);        // convert netCDF base type to IDAM type
            strcpy(field.type, idamNameType(field.atomictype));        // convert atomic type to a string label
        } else {
            field.atomictype = TYPE_UNKNOWN;

// Identify the required structure definition

            variable->udt = NULL;                            // Update when all udts defined (realloc problem!)
            variable->udtIndex = findUserDefinedTypeIndexReverse((int)variable->vartype);

            if (variable->udtIndex < 0) {
                err = 999;
                char* msg = malloc(sizeof(char) * 1024);
                sprintf(msg, "Variable %s has a User Defined or Atomic Type definition that is either Not within Scope "
                        "or Not Known or Not currently supported.", variable->varname);
                addIdamError(&idamerrorstack, CODEERRORTYPE, "readCDF4SubTree", err, msg);
                free((void*)msg);
                free((void*)variable->attribute);
                variable->attribute = NULL;
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
        field.size = getsizeof("void *");
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

    unique = getUniqueTypeName(usertype.name, nameKey, userdefinedtypelist);
    if (strcmp(unique, usertype.name) !=
        0) {                    // The name of the Variable alone is not guaranteed to be unique
        strcpy(usertype.name, unique);
        nameKey++;
    }
    unique = getUniqueTypeName(usertype.name, nameKey, userdefinedtypelist);    // Check the component types
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

    IDAM_LOG(LOG_DEBUG, "getCDF4SubTreeVar2Meta: Properties \n");
    IDAM_LOGF(LOG_DEBUG, "ignoreHiddenVars  : %d\n", (int)ignoreHiddenVars);
    IDAM_LOGF(LOG_DEBUG, "notPointerType    : %d\n", (int)notPointerType);

//----------------------------------------------------------------------
// Initialise

    initVariable(variable);

//----------------------------------------------------------------------
// Variable Name, Type, etc.

    if ((err = nc_inq_var(grpid, varid, name, &variable->vartype, &variable->rank, variable->dimids, &numatts)) !=
        NC_NOERR) {
        addIdamError(&idamerrorstack, CODEERRORTYPE, "getCDF4SubTreeVar2Meta", err, (char*)nc_strerror(err));
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
        field->atomictype = TYPE_STRING;
        strcpy(field->type, "STRING");                // Single String of arbitrary length
        field->pointer = 1;
        field->rank = 0;                    // Scalar string with an irregular length
        field->count = 1;                    // 1 String
        field->shape = NULL;
        field->size = getsizeof("char *");
        field->offset = newoffset(*offset, "char *");
        *offset = field->offset + field->size;        // use previous offset for alignment
        field->offpad = padding(field->offset, "char *");
        field->alignment = getalignmentof("char *");
    } else if (variable->vartype == NC_STRING) {                // Passed as array type: char **
        field->atomictype = TYPE_STRING;
        strcpy(field->type, "STRING *");                // String Array
        field->pointer = 1;
        field->rank = 0;                    // Irregular length string
        field->count = 1;                    // Unknown number of Unknown length (at this time!)
        field->shape = NULL;
        field->size = field->count * sizeof(char**);
        field->offset = newoffset(*offset, field->type);
        *offset = field->offset + field->size;        // use previous offset for alignment
        field->offpad = padding(field->offset, field->type);
        field->alignment = getalignmentof(field->type);
    } else {

        if (isAtomicNCType(variable->vartype)) {
            field->atomictype = convertNCType(variable->vartype);        // convert netCDF base type to IDAM type
            strcpy(field->type, idamNameType(field->atomictype));        // convert atomic type to a string label
        } else {
            field->atomictype = TYPE_UNKNOWN;                // Structured Type

// Identify the required structure definition

            variable->udt = NULL;                        // Update when all udts defined (realloc problem!)
            variable->udtIndex = findUserDefinedTypeIndexReverse((int)variable->vartype);

            if (variable->udtIndex < 0) {
                err = 999;
                char* msg = malloc(sizeof(char) * 1024);
                sprintf(msg, "Variable %s has a User Defined or Atomic Type definition that is either Not within Scope "
                        "or Not Known or Not currently supported.", variable->varname);
                addIdamError(&idamerrorstack, CODEERRORTYPE, "getCDF4SubTreeVar2Meta", err, msg);
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
            field->size = getsizeof("void *");
            field->offset = newoffset(*offset, "void *");
            *offset = field->offset + field->size;
            field->offpad = padding(field->offset, "void *");
            field->alignment = getalignmentof("void *");
        } else {

// dgm TODO: check sizes for UDT arrays
            field->pointer = 0;
            field->rank = variable->rank;        // Need to know the dimensions etc for the count, rank and shape
            field->count = 1;
            //if(rank > 1){
            //field->shape      = (int *)malloc(field->rank*sizeof(int));
            //int i;
            //for(i=0;i<field->rank;i++) field->shape[i] = ?;
            //}
            if (field->atomictype == TYPE_UNKNOWN) {
                field->size = userdefinedtypelist->userdefinedtype[variable->udtIndex].size;
            } else {
                field->size = field->count * getsizeof(field->type);
            }
            field->offset = newoffset(*offset,
                                      field->type);        // Need a new or modified function for structures, offpad and alignment?
            *offset = field->offset + field->size;
            field->offpad = padding(field->offset, field->type);
            field->alignment = getalignmentof(field->type);
        }
    }

    return err;
}


int getCDF4SubTreeMeta(int grpid, int parent, USERDEFINEDTYPE* udt, USERDEFINEDTYPELIST* userdefinedtypelist,
                       HGROUPS* hgroups)
{

// Create the Hierarchical Structure Definitions that describes the sub-tree
// Ensure distinct structures (types) with the same name are uniquely identifiable.
// If the structure definition is defined in a hidden attribute and the

    int i, j, err = NC_NOERR, offset = 0;
    int varid;
    int numgrps, numatts, numvars;
    int* grpids, * varids;
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

    if ((err = getCDF4SubTreeUserDefinedTypes(grpid, NULL, userdefinedtypelist)) != NC_NOERR) {
        err = 999;
        addIdamError(&idamerrorstack, CODEERRORTYPE, "readCDF4SubTree", err, "User Defined Type definition Problem!!");
        return err;
    }

//----------------------------------------------------------------------
// Groups within this node

// Ignore 'hidden' groups if requested

    unsigned short ignoreHiddenAtts = readCDF4Properties() & NC_IGNOREHIDDENATTS;
    unsigned short ignoreHiddenVars = readCDF4Properties() & NC_IGNOREHIDDENVARS;
    unsigned short ignoreHiddenGroups = readCDF4Properties() & NC_IGNOREHIDDENGROUPS;
    unsigned short notPointerType =
            readCDF4Properties() & NC_NOTPOINTERTYPE;        // Return data as explicitly sized arrays
    unsigned short noDimensionData = readCDF4Properties() & NC_NODIMENSIONDATA;        // cf: getCDF4SubTreeVarData
    unsigned short noAttributeData = readCDF4Properties() & NC_NOATTRIBUTEDATA;
    unsigned short noVarAttributeData = readCDF4Properties() & NC_NOVARATTRIBUTEDATA;

    unsigned short regularVarData = (noVarAttributeData || noAttributeData) &&
                                    noDimensionData;    // Return variables as regular arrays not structures

    int attCount = 0;

// All Child Groups

    numgrps = 0;

    if ((err = nc_inq_grps(grpid, &numgrps, NULL)) == NC_NOERR) {
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

            IDAM_LOG(LOG_DEBUG, "getCDF4SubTreeMeta: Group List\n");
            for (i = 0; i < numgrps; i++) {
                if ((err = nc_inq_grpname(grpids[i], name)) != NC_NOERR)
                    IDAM_LOGF(LOG_DEBUG, "[%d]: %s\n", i, name);
            }
            err = 0;
        }
        if ((err = nc_inq_grpname(grpid, name)) != NC_NOERR) {
            addIdamError(&idamerrorstack, CODEERRORTYPE, "readCDF", err, (char*)nc_strerror(err));
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

    usertype.idamclass = TYPE_COMPOUND;
    strcpy(usertype.source, "netcdf");
    usertype.ref_id = 0;
    usertype.imagecount = 0;                // No Structure Image data
    usertype.image = NULL;
    usertype.size = 0;                // Structure size

    strcpy(usertype.name, name);

    unique = getUniqueTypeName(usertype.name, nameKey, userdefinedtypelist);
    if (strcmp(unique, usertype.name) !=
        0) {                    // The name of the Group alone is not guaranteed to be unique
        strcpy(usertype.name, unique);
        nameKey++;
    }
    unique = getUniqueTypeName(usertype.name, nameKey, userdefinedtypelist);    // Check the component types
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
                addIdamError(&idamerrorstack, CODEERRORTYPE, "readCDF", err, (char*)nc_strerror(err));
                if (numgrps > 0)free((void*)grpids);
                if (numatts > 0)free((void*)group->attribute);
                freeUserDefinedType(&usertype);
                return err;
            }

            if (ignoreHiddenAtts && name[0] == NC_HIDDENPREFIX) continue;        // Skip this attribute if 'hidden'

            attCount++;
            strcpy(group->attribute[i].attname, name);

            if ((err = nc_inq_atttype(grpid, varid, name, &atttype)) != NC_NOERR) {
                addIdamError(&idamerrorstack, CODEERRORTYPE, "readCDF", err, (char*)nc_strerror(err));
                if (numgrps > 0)free((void*)grpids);
                if (numatts > 0)free((void*)group->attribute);
                freeUserDefinedType(&usertype);
                return err;
            }
            group->attribute[i].atttype = atttype;
            if ((err = nc_inq_attlen(grpid, varid, name, (size_t*)&attlength)) != NC_NOERR) {
                addIdamError(&idamerrorstack, CODEERRORTYPE, "readCDF", err, (char*)nc_strerror(err));
                if (numgrps > 0)free((void*)grpids);
                if (numatts > 0)free((void*)group->attribute);
                freeUserDefinedType(&usertype);
                return err;
            }
            group->attribute[i].attlength = (int)attlength;

            initCompoundField(&field);
            strcpy(field.name, name);
            field.desc[0] = '\0';

            if (atttype == NC_CHAR) {                    // Numbers should be NC_BYTE
                field.atomictype = TYPE_STRING;
                strcpy(field.type, "STRING");                // Single String passed as type: char *
                field.pointer = 1;
                field.rank = 0;                    // Irregular length string
                field.count = 1;                    // 1 String !
                field.shape = NULL;
                field.size = getsizeof("char *");
                field.offset = newoffset(offset, "char *");
                offset = field.offset +
                         field.size;                                            // use previous offset for alignment
                field.offpad = padding(field.offset, "char *");
                field.alignment = getalignmentof("char *");
            } else if (atttype == NC_STRING) {
                field.atomictype = TYPE_STRING;
                strcpy(field.type, "STRING *");                // String Array
                field.pointer = 1;                    // Passed as array type: char **
                field.rank = 0;
                field.count = 1;                    // Arbitrary Number of Strings of arbitrary length
                field.shape = NULL;
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
                    strcpy(field.type, idamNameType(field.atomictype));        // convert atomic type to a string label
                } else {
                    field.atomictype = TYPE_UNKNOWN;

// Identify the required structure definition (must be defined as a User Defined Type)

                    group->attribute[i].udt = NULL;                            // Update when all udts defined (realloc problem!)
                    group->attribute[i].udtIndex = findUserDefinedTypeIndexReverse((int)atttype);    //

                    if (group->attribute[i].udtIndex < 0) {
                        err = 999;
                        char* msg = malloc(sizeof(char) * 1024);
                        sprintf(msg,
                                "Group %s Attribute %s has a User Defined or Atomic Type definition that is either Not within Scope "
                                        "or Not Known or Not currently supported.", group->grpname, name);
                        addIdamError(&idamerrorstack, CODEERRORTYPE, "readCDF4SubTree", err, msg);
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
                    field.size = getsizeof("void *");
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
                    field.size = getsizeof(field.type);
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
            if (attCount == 0 && group->attribute != NULL) {
                free((void*)group->attribute);
                group->attribute = NULL;
            }
        }

    }  // End of IF varnatts


//----------------------------------------------------------------------
// Variables within this node

    numvars = 0;

    if ((err = nc_inq_varids(grpid, &numvars, NULL)) == NC_NOERR) {
        if (numvars > 0) {
            int varCount = 0;
            VARIABLE* variable = (VARIABLE*)malloc(sizeof(VARIABLE) * numvars);
            varids = (int*)malloc(sizeof(int) * numvars);
            group->numvars = numvars;
            group->variable = variable;
            if ((err = nc_inq_varids(grpid, &numvars, varids)) != NC_NOERR) {
                addIdamError(&idamerrorstack, CODEERRORTYPE, "readCDF", err, (char*)nc_strerror(err));
                freeUserDefinedType(&usertype);
                free((void*)variable);
                free((void*)varids);
                return err;
            }

            for (j = 0; j < numvars; j++) {

                if (regularVarData) {        // Just add the variable array ingoring attributes and the dimension doc string

/*
// rank?

   if((err = nc_inq_var(grpid, varids[j], name, &variable->vartype, &variable->rank, variable->dimids, &numatts)) != NC_NOERR){
      addIdamError(&idamerrorstack, CODEERRORTYPE, "getCDF4SubTreeVar2Meta", err, (char *)nc_strerror(err));
      return err;
   }


// Variable Count and Shape

   if(variable->rank > 1 && variable->shape == NULL){
      variable->shape = (int *)malloc(variable->rank*sizeof(int));
      addMalloc((void *)variable->shape, variable->rank, sizeof(int), "int");
      for(i=0;i<variable->rank;i++){
	 if((rc = nc_inq_dim(grpid, variable->dimids[i], dimname, &dnum)) != NC_NOERR){
            err = NETCDF_ERROR_INQUIRING_DIM_3;
            addIdamError(&idamerrorstack, CODEERRORTYPE, "readCDF4SubTreeMeta", err, (char *)nc_strerror(rc));
	    break;
	 }
	 variable->shape[i] = (int)dnum;
      }
   }

   if(variable->rank <= 1){
      if((rc = nc_inq_dim(grpid, variable->dimids[0], dimname, &dnum)) != NC_NOERR){
         err = NETCDF_ERROR_INQUIRING_DIM_3;
         addIdamError(&idamerrorstack, CODEERRORTYPE, "readCDF4SubTreeMeta", err, (char *)nc_strerror(rc));
	 break;
      }
      count = (int)dnum;
   } else {
      count = 1;
      for(i=0;i<variable->rank;i++) count = count * variable->shape[i];
   }

*/

                    if ((err = getCDF4SubTreeVar2Meta(grpid, varids[j], &variable[j], &offset, &field,
                                                      userdefinedtypelist)) != NC_NOERR) {
                        addIdamError(&idamerrorstack, CODEERRORTYPE, "readCDF", err, "getCDF4SubTreeVar2Meta error");
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
                                                     userdefinedtypelist)) != NC_NOERR) {
                        addIdamError(&idamerrorstack, CODEERRORTYPE, "readCDF", err, "getCDF4SubTreeVarMeta error");
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

                    field.atomictype = TYPE_UNKNOWN;
                    strcpy(field.type, gusertype.name);        // Unique data type

                    field.desc[0] = '\0';
                    field.pointer = 1;
                    field.size = getsizeof("void *");
                    field.offset = newoffset(offset, "void *");
                    offset = field.offset +
                             field.size;                                            // use previous offset for alignment
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

    for (i = 0; i < numgrps; i++) {

        getCDF4SubTreeMeta(grpids[i], grpid, &gusertype, userdefinedtypelist, hgroups);

        initCompoundField(&field);

// dgm 13May2011 **** what name should the group have?
// Group Name either from the last entry in hgroups or by direct call for the child group name
//
        //BUG: strcpy(field.name, hgroups->group[hgroups->numgrps-1].grpname);	// This need not be unique!
        //TYPE not Variable Name: strcpy(field.name, gusertype.name);

        GROUP* x = findHGroup(hgroups, grpids[i]);    // Locate the group and it's name
        strcpy(field.name, x->grpname);

        field.atomictype = TYPE_UNKNOWN;
        strcpy(field.type, gusertype.name);        // This should be unique

        field.desc[0] = '\0';
        field.pointer = 1;
        field.size = getsizeof("void *");
        field.offset = newoffset(offset, "void *");
        offset = field.offset +
                 field.size;                                            // use previous offset for alignment
        field.offpad = padding(field.offset, "void *");
        field.alignment = getalignmentof("void *");

        field.rank = 1;
        field.count = 1;
        field.shape = (int*)malloc(field.rank * sizeof(int));
        field.shape[0] = 1;

        addCompoundField(&usertype, field);
    }

//----------------------------------------------------------------------
// If Groups exist without data, add a hidden ignorable item to prevent problems

    if (usertype.fieldcount == 0) {
        initCompoundField(&field);
        sprintf(field.name, "%s", NC_EMPTY_GROUP_VAR_NAME);
        strcpy(field.desc, "Empty Group");
        field.atomictype = TYPE_STRING;
        strcpy(field.type, "STRING");                // Single String passed as type: char *
        field.pointer = 1;
        field.rank = 0;                    // Irregular length string
        field.count = 1;                    // 1 String !
        field.shape = NULL;
        field.size = getsizeof("char *");
        field.offset = newoffset(offset, "char *");
        offset = field.offset +
                 field.size;                                            // use previous offset for alignment
        field.offpad = padding(field.offset, "char *");
        field.alignment = getalignmentof("char *");

        addCompoundField(&usertype, field);

    }

//----------------------------------------------------------------------
// Update Structure Size

    // usertype.size = newoffset(offset, "void *");				// Bug Fix 13Apr2015 - returns a zero sized structure!

    usertype.size = getStructureSize(
            &usertype);                    // This may return a size too small as any terminating packing bytes are ignored!

// Record this type definition

    *udt = usertype;

// Repeat the check that the name is unique

    unique = getUniqueTypeName(usertype.name, nameKey, userdefinedtypelist);
    if (strcmp(unique, usertype.name) !=
        0) {                    // The name of the Group alone is not guaranteed to be unique
        strcpy(usertype.name, unique);
        nameKey++;
    }
    unique = getUniqueTypeName(usertype.name, nameKey, userdefinedtypelist);    // Check the component types
    if (strcmp(unique, usertype.name) != 0) {
        strcpy(usertype.name, unique);
        nameKey++;
    }

// Update Variable structure with type definition Index (Not the pointer as the array base memory changes with realloc)

    addUserDefinedType(userdefinedtypelist, usertype);

    //grp->udtIndex = userdefinedtypelist->listCount-1;
    hgroups->group[grp].udtIndex = userdefinedtypelist->listCount - 1;

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
    int* shape = NULL;
    char name[NC_MAX_NAME + 1];
    char work[2 * NC_MAX_NAME + 1];
    char* dim = NULL;

    *count = 1;
    *shp = NULL;
    if (rank > 1) shape = (int*)malloc(rank * sizeof(int));    // Added externally to MallocLog

    for (i = 0; i < rank; i++) {
        nc_inq_dim(grpid, dimids[i], name, (size_t*)&length);
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

ENUMLIST* getCDF4EnumList(int grpid, nc_type vartype)
{
    int i, rc, err;
    char value[8];
    ENUMLIST* enumlist = (ENUMLIST*)malloc(sizeof(ENUMLIST));
    nc_type base;
    size_t size, members;
    char name[NC_MAX_NAME + 1];

    if ((rc = nc_inq_enum(grpid, vartype, name, &base, &size, &members)) != NC_NOERR) {
        err = 999;
        addIdamError(&idamerrorstack, CODEERRORTYPE, "getCDF4EnumList", err, (char*)nc_strerror(rc));
        return NULL;
    }

    if (members == 0) {
        err = 999;
        addIdamError(&idamerrorstack, CODEERRORTYPE, "getCDF4EnumList", err, "Enumerated Type has Zero Membership?");
        return NULL;
    }

    strcpy(enumlist->name, name);
    enumlist->type = convertNCType(base);
    enumlist->count = members;
    enumlist->enummember = (ENUMMEMBER*)malloc(members * sizeof(ENUMMEMBER));

    for (i = 0; i < members; i++) {
        if ((rc = nc_inq_enum_member(grpid, vartype, i, enumlist->enummember[i].name, (void*)value)) != NC_NOERR) {
            err = 999;
            addIdamError(&idamerrorstack, CODEERRORTYPE, "getCDF4EnumList", err, (char*)nc_strerror(rc));
            return NULL;
        }
        switch (enumlist->type) {
            case (TYPE_CHAR): {
                char* enumvalue = (char*)value;
                enumlist->enummember[i].value = (long long)*enumvalue;
                break;
            }
            case (TYPE_SHORT): {
                short* enumvalue = (short*)value;
                enumlist->enummember[i].value = (long long)*enumvalue;
                break;
            }
            case (TYPE_UNSIGNED_SHORT): {
                unsigned short* enumvalue = (unsigned short*)value;
                enumlist->enummember[i].value = (long long)*enumvalue;
                break;
            }
            case (TYPE_INT): {
                short* enumvalue = (short*)value;
                enumlist->enummember[i].value = (long long)*enumvalue;
                break;
            }
            case (TYPE_UNSIGNED_INT): {
                unsigned int* enumvalue = (unsigned int*)value;
                enumlist->enummember[i].value = (long long)*enumvalue;
                break;
            }
            case (TYPE_LONG64): {
                long long* enumvalue = (long long*)value;
                enumlist->enummember[i].value = (long long)*enumvalue;
                break;
            }
            case (TYPE_UNSIGNED_LONG64): {
                unsigned long long* enumvalue = (unsigned long long*)value;
                enumlist->enummember[i].value = (long long)*enumvalue;
                break;
            }
        }
    }

// Add mallocs to list for freeing when data dispatch complete

    addMalloc((void*)enumlist, 1, sizeof(ENUMLIST), "ENUMLIST");
    addMalloc((void*)enumlist->enummember, members, sizeof(ENUMMEMBER), "ENUMMEMBER");

    return enumlist;
}


int getCDF4SubTreeVarData(int grpid, void** data, VARIABLE* variable)
{

// Read variable data
// Organised as a structure with the variable's attributes and a string documenting the order and size of dimensions
// If there are no attributes and the dimension string is to be 'ignored', the data are organised as they are saved to netCDF4

    int i, rc, err = 0;
    int fieldid, varid;
    int count, size;

    char* p0, * d;
    VOIDTYPE* p;

    size_t startIndex[NC_MAX_DIMS];
    size_t countIndex[NC_MAX_DIMS];

//----------------------------------------------------------------------
// Return a Structure or the Variable?

    //if(variable->numatts == 0 && (readCDF4Properties() & NC_NODIMENSIONDATA)){		// Return just the Variable
    //   return 0;
    //}

//----------------------------------------------------------------------
// Allocate heap for this structure (or atomic array)

    *data = (void*)malloc(variable->udt->size);
    addMalloc(*data, 1, variable->udt->size, variable->udt->name);

// Start of the Structure

    p0 = (char*)*((char**)data);

// Locate the next field (udt compound field must be synchronised to the group elements)

    fieldid = 0;

//----------------------------------------------------------------------
// Start Error Trap

    do {

//----------------------------------------------------------------------
// Read Attributes (arrays are always rank 1 if not NC_STRING when rank 2)

        varid = variable->varid;

        for (i = 0; i < variable->numatts; i++) {

// Locate the next pointer position within the structure

            p = (VOIDTYPE*)&p0[variable->udt->compoundfield[fieldid++].offset];
            *p = 0;

// Regular

            if (variable->attribute[i].udt == NULL) {

                if (variable->attribute[i].atttype == NC_CHAR ||            // Legacy String
                    variable->attribute[i].atttype == NC_STRING) {            // String Atomic Type
                    int attlength = variable->attribute[i].attlength;
                    if (variable->attribute[i].atttype == NC_CHAR) {
                        attlength = attlength + 1;
                        d = (char*)malloc(attlength * sizeof(char));
                        //addMalloc(d, attlength, sizeof(char), "char");
                        addMalloc(d, 1, attlength * sizeof(char), "char");
                        if ((err = nc_get_att_text(grpid, varid, variable->attribute[i].attname, d)) != NC_NOERR) {
                            err = 999;
                            addIdamError(&idamerrorstack, CODEERRORTYPE, "readCDF4SubTree", err,
                                         "Problem Reading Legacy String Attribute");
                            break;
                        }
                        d[variable->attribute[i].attlength] = '\0';        // Ensure NULL terminated
                    } else {
                        int istr, lstr;
                        char** svec = (char**)malloc((size_t)attlength *
                                                     sizeof(char*));    // The attribute length is the number of strings
                        if ((err = nc_get_att(grpid, varid, variable->attribute[i].attname, (void*)svec)) !=
                            NC_NOERR) {
                            err = 999;
                            addIdamError(&idamerrorstack, CODEERRORTYPE, "readCDF4SubTree", err,
                                         "Problem Reading String Attribute");
                            break;
                        }

                        char** ssvec = (char**)malloc((size_t)attlength * sizeof(char*));// Array of strings
                        addMalloc((void*)ssvec, attlength, sizeof(char*), "STRING *");
                        for (istr = 0; istr < attlength; istr++) {                    // Copy into locally managed Heap
                            lstr = (int)strlen(svec[istr]) + 1;
                            d = (char*)malloc(lstr * sizeof(char));
                            addMalloc(d, lstr, sizeof(char), "char");            // Individual strings
                            strcpy(d, svec[istr]);
                            ssvec[istr] = d;
                        }
                        *p = (VOIDTYPE)ssvec;                        // Slot String Array into structure

                        nc_free_string(attlength, svec);
                        free(svec);
                        d = NULL;

// String arrays within data structures are defined (readCDFTypes) as char *str[int] => rank=1, pointer=0 and type=STRING*

// The expectation is a rank 1 char array of length attlength+1 to hold a Legacy string
// The NC_STRING object is a rank 1 array of length attlength of char * pointers to each string.
// The structure definition should be char*[]

                    }

                } else {
                    int idamType = convertNCType(variable->attribute[i].atttype);
                    if (idamType == TYPE_UNKNOWN) {
                        err = 999;
                        addIdamError(&idamerrorstack, CODEERRORTYPE, "readCDF4SubTree", err,
                                     "Variable Attribute has Unknown Atomic Type (Not a User Defined Type)!");
                        break;
                    }
                    size = getsizeof(idamNameType(idamType));
                    d = (char*)malloc(variable->attribute[i].attlength * size);
                    addMalloc(d, variable->attribute[i].attlength, size, idamNameType(idamType));
                    if ((err = nc_get_att(grpid, varid, variable->attribute[i].attname, (void*)d)) != NC_NOERR) {
                        err = 999;
                        addIdamError(&idamerrorstack, CODEERRORTYPE, "readCDF4SubTree", err,
                                     "Problem reading Variable Attribute Value!");
                        break;
                    }
                }

            } else {        // User Defined type

// Modified Functionality 15Feb2013: dgm

                if (variable->attribute[i].udt->idamclass != TYPE_ENUM) {
                    size = variable->attribute[i].udt->size;
                    d = (char*)malloc(variable->attribute[i].attlength * size);
                    addMalloc(d, variable->attribute[i].attlength, size, variable->attribute[i].udt->name);
                    if ((err = nc_get_att(grpid, varid, variable->attribute[i].attname, (void*)d)) != NC_NOERR) {
                        err = 999;
                        addIdamError(&idamerrorstack, CODEERRORTYPE, "readCDF4SubTree", err,
                                     "Problem reading Variable Attribute Value!");
                        break;
                    }
                } else {

                    int j;
                    char value[8];
                    ENUMLIST* enumlist = (ENUMLIST*)malloc(sizeof(ENUMLIST));
                    nc_type base;
                    size_t size, members;
                    char name[NC_MAX_NAME + 1];

// **** Locate the group with the enum definition

                    if ((rc = nc_inq_enum(grpid, variable->attribute[i].atttype, name, &base, &size, &members)) !=
                        NC_NOERR) {
                        err = 999;
                        addIdamError(&idamerrorstack, CODEERRORTYPE, "readCDF4SubTree", err, (char*)nc_strerror(rc));
                        break;
                    }

                    if (members == 0) {
                        err = 999;
                        addIdamError(&idamerrorstack, CODEERRORTYPE, "readCDF4SubTree", err,
                                     "Enumerated Type has Zero Membership!");
                        break;
                    }

                    strcpy(enumlist->name, name);
                    enumlist->type = convertNCType(base);
                    enumlist->count = members;
                    enumlist->enummember = (ENUMMEMBER*)malloc(members * sizeof(ENUMMEMBER));

                    for (j = 0; j < members; j++) {
                        rc = nc_inq_enum_member(grpid, variable->attribute[i].atttype, j, enumlist->enummember[j].name,
                                                (void*)value);
                        switch (enumlist->type) {
                            case (TYPE_CHAR): {
                                char* enumvalue = (char*)value;
                                enumlist->enummember[j].value = (long long)*enumvalue;
                                break;
                            }
                            case (TYPE_SHORT): {
                                short* enumvalue = (short*)value;
                                enumlist->enummember[j].value = (long long)*enumvalue;
                                break;
                            }
                            case (TYPE_UNSIGNED_SHORT): {
                                unsigned short* enumvalue = (unsigned short*)value;
                                enumlist->enummember[j].value = (long long)*enumvalue;
                                break;
                            }
                            case (TYPE_INT): {
                                short* enumvalue = (short*)value;
                                enumlist->enummember[j].value = (long long)*enumvalue;
                                break;
                            }
                            case (TYPE_UNSIGNED_INT): {
                                unsigned int* enumvalue = (unsigned int*)value;
                                enumlist->enummember[j].value = (long long)*enumvalue;
                                break;
                            }
                            case (TYPE_LONG64): {
                                long long* enumvalue = (long long*)value;
                                enumlist->enummember[j].value = (long long)*enumvalue;
                                break;
                            }
                            case (TYPE_UNSIGNED_LONG64): {
                                unsigned long long* enumvalue = (unsigned long long*)value;
                                enumlist->enummember[j].value = (long long)*enumvalue;
                                break;
                            }
                        }
                    }

                    d = (char*)malloc(variable->attribute[i].attlength *
                                      sizeof(long long));    // Sufficient space ignoring integer type
                    if ((err = nc_get_att(grpid, varid, variable->attribute[i].attname, (void*)d)) != NC_NOERR) {
                        err = 999;
                        addIdamError(&idamerrorstack, CODEERRORTYPE, "readCDF4SubTree", err,
                                     "Problem reading Variable Attribute Value!");
                        break;
                    }

                    enumlist->data = (void*)d;
                    d = (char*)enumlist;

// Add mallocs to list for freeing when data dispatch complete

                    addMalloc((void*)enumlist, 1, sizeof(ENUMLIST), "ENUMLIST");
                    addMalloc((void*)enumlist->enummember, members, sizeof(ENUMMEMBER), "ENUMMEMBER");
                    addMalloc((void*)enumlist->data, variable->attribute[i].attlength, size,
                              idamNameType(idamAtomicType(base)));            // Use true integer type
                }

// End of Modified Functionality 15Feb2013

// String replacement

                replaceSubTreeEmbeddedStrings(variable->attribute[i].udt, variable->attribute[i].attlength, d);

            }

            if (d != NULL) *p = (VOIDTYPE)d;        // Slot Data into structure

        }


//----------------------------------------------------------------------
// Dimensions (Skip if requested they be ignored)

        count = 0;

        if (!(readCDF4Properties() & NC_NODIMENSIONDATA)) {
            p = (VOIDTYPE*)&p0[variable->udt->compoundfield[fieldid++].offset];
            *p = 0;
            *p = (VOIDTYPE)dimShapeLabel(grpid, variable->rank, variable->dimids, &count, &variable->shape);
            if ((void*)*p != NULL) {
                addMalloc((void*)*p, 1, (int)(strlen((char*)*p) + 1) * sizeof(char),
                          "char");
            }        // Shape added to Array heap
        }

//----------------------------------------------------------------------
// Read Variable Data and expand enumerated types

        p = (VOIDTYPE*)&p0[variable->udt->compoundfield[fieldid].offset];
        *p = 0;

        if (variable->udt->compoundfield[fieldid].atomictype == TYPE_UNKNOWN) {        // Structure
            size = getsizeof(variable->udt->compoundfield[fieldid].type);
        } else {
            size = getsizeof(idamNameType(variable->udt->compoundfield[fieldid].atomictype));
        }

        if (size == 0) {
            err = 999;
            addIdamError(&idamerrorstack, CODEERRORTYPE, "readCDF4SubTree", err, "Data Type has Zero Size!");
            break;
        }

        if (count != 0) {    // Data are not always written!

            if (variable->rank > 1) {
                if (variable->shape == NULL) {
                    err = 999;
                    addIdamError(&idamerrorstack, CODEERRORTYPE, "readCDF4SubTree", err,
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
                ENUMLIST* enumlist = getCDF4EnumList(grpid, variable->vartype);
                size = getsizeof(idamNameType(enumlist->type));
                d = (char*)malloc(count * size);                            // Enumerated Array
                if (d != NULL) memset(d, 0, count * size);
                addMalloc2((void*)d, count, size, idamNameType(enumlist->type), variable->rank, variable->shape);
                if ((rc = nc_get_vara(grpid, varid, (size_t*)&startIndex, (size_t*)&countIndex, (void*)d)) !=
                    NC_NOERR) {
                    err = 999;
                    addIdamError(&idamerrorstack, CODEERRORTYPE, "readCDF4SubTree", err, (char*)nc_strerror(rc));
                    addIdamError(&idamerrorstack, CODEERRORTYPE, "readCDF4SubTree", err,
                                 "Unable to read Enumerated Array variable data!");
                    break;
                }

                enumlist->data = (void*)d;
                d = (char*)enumlist;
            } else if (variable->vartype == NC_STRING) {

                if (variable->rank > 2) {
                    err = 999;
                    addIdamError(&idamerrorstack, CODEERRORTYPE, "readCDFVar", err,
                                 "String Array rank is too large - must be <= 2");
                    break;
                }

                int istr, lstr;
                char** svec = (char**)malloc(
                        (size_t)count * sizeof(char*));        // Array of pointers to string array elements
                if ((rc = nc_get_var_string(grpid, varid, svec)) != NC_NOERR) {
                    err = 999;
                    addIdamError(&idamerrorstack, CODEERRORTYPE, "readCDF4SubTree", err, (char*)nc_strerror(rc));
                    addIdamError(&idamerrorstack, CODEERRORTYPE, "readCDF4SubTree", err,
                                 "Unable to read String Array variable data!");
                    break;
                }

                char** ssvec = (char**)malloc((size_t)count * sizeof(char*));// Array of strings
                addMalloc((void*)ssvec, count, sizeof(char*), "STRING *");
                for (istr = 0; istr < count; istr++) {                    // Copy into locally managed Heap
                    lstr = (int)strlen(svec[istr]) + 1;
                    d = (char*)malloc(lstr * sizeof(char));
                    addMalloc(d, lstr, sizeof(char), "char");            // Individual strings
                    strcpy(d, svec[istr]);
                    ssvec[istr] = d;
                }
                *p = (VOIDTYPE)ssvec;                        // Slot String Array into structure
                nc_free_string(count, svec);
                free(svec);
                d = NULL;
                if (variable->shape != NULL) addMalloc((void*)variable->shape, variable->rank, sizeof(int), "int");

            } else {

                if (IMAS_HDF_READER && variable->vartype == NC_CHAR) {
                    count = count * 132 +
                            1;
                } // IMAS HDF5 strings are written in blocks of 132 bytes + string terminator

                d = (char*)malloc(count * size);                            // Regular Array
                if (d != NULL) memset(d, 0, count * size);                        // Assign char 0 to all bytes

                if (variable->vartype == NC_CHAR) {        // Legacy Single String => Reduce rank
                    if (variable->rank != 1) {
                        err = 999;
                        addIdamError(&idamerrorstack, CODEERRORTYPE, "readCDF4SubTree", err,
                                     "Unable to read Array of Legacy Strings!");
                        break;
                    }

                    addMalloc2((void*)d, 1, count * size, variable->udt->compoundfield[fieldid].type,
                               variable->rank - 1, variable->shape);
                } else {
                    addMalloc2((void*)d, count, size, variable->udt->compoundfield[fieldid].type, variable->rank,
                               variable->shape);
                }

                if ((rc = nc_get_vara(grpid, varid, (size_t*)&startIndex, (size_t*)&countIndex, (void*)d)) !=
                    NC_NOERR) {
                    err = 999;
                    addIdamError(&idamerrorstack, CODEERRORTYPE, "readCDF4SubTree", err, (char*)nc_strerror(rc));
                    addIdamError(&idamerrorstack, CODEERRORTYPE, "readCDF4SubTree", err,
                                 "Unable to read Regular Array variable data!");
                    break;
                }

// String replacement

                USERDEFINEDTYPE* child = findUserDefinedType(variable->udt->compoundfield[fieldid].type, 0);
                replaceSubTreeEmbeddedStrings(child, count, d);

            }
        } else {
            d = NULL;        // No Data
            if (variable->shape != NULL) addMalloc((void*)variable->shape, variable->rank, sizeof(int), "int");
        }

        if (d != NULL) *p = (VOIDTYPE)d;

        fieldid++;


//----------------------------------------------------------------------
// End of Error Trap

    } while (0);

//----------------------------------------------------------------------
// Housekeeping

    return err;
}


int getCDF4SubTreeVar2Data(int grpid, void** data, VARIABLE* variable, COMPOUNDFIELD* field)
{

// Read variable data as a stand alone element - attributes and the dimension doc string are ignored
// Return a pointer to the heap allocated for the data

// *** STRING arrays are not yet implemented

    int i, rc, err = 0;
    int count = 0, size = 0;

    char dimname[NC_MAX_NAME + 1], typename[NC_MAX_NAME + 1];
    size_t dnum = 0;
    size_t startIndex[NC_MAX_DIMS];
    size_t countIndex[NC_MAX_DIMS];

//----------------------------------------------------------------------
// Error Trap

    do {

//----------------------------------------------------------------------
// Variable Type Size

        if (variable->udt != NULL) {                                // Structure Type
            size = variable->udt->size;
            strcpy(typename, variable->udt->name);
        } else if (isAtomicNCType(variable->vartype)) {                    // Atomic Type
            strcpy(typename, idamNameType(convertNCType(variable->vartype)));
            size = getsizeof(typename);
        }

        if (size == 0) {
            err = 999;
            addIdamError(&idamerrorstack, CODEERRORTYPE, "getCDF4SubTreeVar2Data", err, "Data Type has Zero Size!");
            break;
        }

//----------------------------------------------------------------------
// Variable Count and Shape

        if (variable->rank > 1 && variable->shape == NULL) {
            variable->shape = (int*)malloc(variable->rank * sizeof(int));
            addMalloc((void*)variable->shape, variable->rank, sizeof(int), "int");
            for (i = 0; i < variable->rank; i++) {
                if ((rc = nc_inq_dim(grpid, variable->dimids[i], dimname, &dnum)) != NC_NOERR) {
                    err = NETCDF_ERROR_INQUIRING_DIM_3;
                    addIdamError(&idamerrorstack, CODEERRORTYPE, "readCDF4SubTreeVar2Data", err,
                                 (char*)nc_strerror(rc));
                    break;
                }
                variable->shape[i] = (int)dnum;
            }
        }

        if (variable->rank <= 1) {
            if ((rc = nc_inq_dim(grpid, variable->dimids[0], dimname, &dnum)) != NC_NOERR) {
                err = NETCDF_ERROR_INQUIRING_DIM_3;
                addIdamError(&idamerrorstack, CODEERRORTYPE, "readCDF4SubTreeVar2Data", err, (char*)nc_strerror(rc));
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
                addIdamError(&idamerrorstack, CODEERRORTYPE, "getCDF4SubTreeVar2Data", err,
                             "String Array rank is too large - must be <= 2");
                break;
            }

            int istr, lstr;
            char** svec = (char**)malloc(
                    (size_t)count * sizeof(char*));        // Array of pointers to string array elements
            if ((rc = nc_get_var_string(grpid, variable->varid, svec)) != NC_NOERR) {
                err = 999;
                addIdamError(&idamerrorstack, CODEERRORTYPE, "getCDF4SubTreeVar2Data", err, (char*)nc_strerror(rc));
                addIdamError(&idamerrorstack, CODEERRORTYPE, "getCDF4SubTreeVar2Data", err,
                             "Unable to read String Array variable data!");
                break;
            }

            char* d = NULL;
            char** ssvec = (char**)malloc((size_t)count * sizeof(char*));// Array of strings
            addMalloc((void*)ssvec, count, sizeof(char*), "STRING *");
            for (istr = 0; istr < count; istr++) {                    // Copy into locally managed Heap
                lstr = (int)strlen(svec[istr]) + 1;
                d = (char*)malloc(lstr * sizeof(char));
                addMalloc(d, lstr, sizeof(char), "char");            // Individual strings
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
            if (d != NULL) memset(d, 0, count * size);            // Assign char 0 to all bytes

            if (variable->vartype == NC_CHAR) {                // Legacy Single String => Reduce rank
                if (variable->rank != 1) {
                    err = 999;
                    addIdamError(&idamerrorstack, CODEERRORTYPE, "getCDF4SubTreeVar2Data", err,
                                 "Unable to read Array of Legacy Strings!");
                    free((void*)d);
                    break;
                }
                addMalloc2((void*)d, 1, count * size, typename, variable->rank - 1, variable->shape);
            } else {
                addMalloc2((void*)d, count, size, typename, variable->rank, variable->shape);
            }

            if ((rc = nc_get_vara(grpid, variable->varid, (size_t*)&startIndex, (size_t*)&countIndex,
                                  (void*)d)) != NC_NOERR) {
                err = 999;
                addIdamError(&idamerrorstack, CODEERRORTYPE, "getCDF4SubTreeVar2Data", err, (char*)nc_strerror(rc));
                addIdamError(&idamerrorstack, CODEERRORTYPE, "getCDF4SubTreeVar2Data", err,
                             "Unable to read Regular Array variable data!");
                break;
            }

// If the data has a User Defined Type with a NC_STRING component then intercept and replace with a locally allocated component

            if (variable->udt != NULL) replaceSubTreeEmbeddedStrings(variable->udt, count, d);

            *data = (void*)d;                        // Return the data array


        }

//----------------------------------------------------------------------
// End of Error Trap

    } while (0);

//----------------------------------------------------------------------
// Housekeeping

    return err;
}


int readCDF4SubTreeVar3Data(GROUPLIST grouplist, int varid, int rank, int* dimids, int** shape,
                            int* ndvec, int* data_type, char** data, USERDEFINEDTYPE** udt)
{
    int grpid;

    int i;
    int rc;
    int err = 0;
    int ndata = 1;

    nc_type vartype;                    // NC types
    char dimname[NC_MAX_NAME + 1];
    char* dvec = NULL;
    char** svec = NULL;            // String array

    size_t* start = NULL, * count = NULL;
    size_t* dnums = NULL;

    *udt = NULL;                // Definition of User Defined Data Structures

//----------------------------------------------------------------------
// Default Group ID

    grpid = grouplist.grpid;

//----------------------------------------------------------------------
// Error Trap

    do {

//----------------------------------------------------------------------
// Shape of the Data Array from the dimensions

        if (rank <= 0) rank = 1;

        start = (size_t*)malloc(rank * sizeof(size_t));
        count = (size_t*)malloc(rank * sizeof(size_t));
        dnums = (size_t*)malloc(rank * sizeof(size_t));

        if (start == NULL || count == NULL || dnums == NULL) {
            err = 999;
            addIdamError(&idamerrorstack, CODEERRORTYPE, "readCDF4SubTreeVar3Data", err,
                         "Unable to Allocate Heap for Shape Arrays");
            break;
        }

        for (i = 0; i < rank; i++) {
            if ((rc = nc_inq_dim(grpid, dimids[i], dimname, (size_t*)&dnums[i])) != NC_NOERR) {
                err = NETCDF_ERROR_INQUIRING_DIM_3;
                addIdamError(&idamerrorstack, CODEERRORTYPE, "readCDF4SubTreeVar3Data", err, (char*)nc_strerror(rc));
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
            addIdamError(&idamerrorstack, CODEERRORTYPE, "readCDF4SubTreeVar3Data", err, (char*)nc_strerror(rc));
            break;
        }

//----------------------------------------------------------------------
// Read Data and Identify the corresponding IDAM type

// Allocate heap

        switch (vartype) {

            case (NC_DOUBLE): {
                *data_type = TYPE_DOUBLE;
                dvec = (char*)malloc((size_t)ndata * sizeof(double));
                break;
            }

            case (NC_FLOAT): {
                *data_type = TYPE_FLOAT;
                dvec = (char*)malloc((size_t)ndata * sizeof(float));
                break;
            }

            case (NC_INT64): {
                *data_type = TYPE_LONG64;
                dvec = (char*)malloc((size_t)ndata * sizeof(long long int));
                break;
            }

            case (NC_INT): {
                *data_type = TYPE_INT;
                dvec = (char*)malloc((size_t)ndata * sizeof(int));
                break;
            }

            case (NC_SHORT): {
                *data_type = TYPE_SHORT;
                dvec = (char*)malloc((size_t)ndata * sizeof(short));
                break;
            }

            case (NC_BYTE): {
                *data_type = TYPE_CHAR;
                dvec = (char*)malloc((size_t)ndata * sizeof(char));
                break;
            }

            case (NC_UINT64): {
                *data_type = TYPE_UNSIGNED_LONG64;
                dvec = (char*)malloc((size_t)ndata * sizeof(unsigned long long int));
                break;
            }

            case (NC_UINT): {
                *data_type = TYPE_UNSIGNED_INT;
                dvec = (char*)malloc((size_t)ndata * sizeof(unsigned int));
                break;
            }

            case (NC_USHORT): {
                *data_type = TYPE_UNSIGNED_SHORT;
                dvec = (char*)malloc((size_t)ndata * sizeof(unsigned short));
                break;
            }

            case (NC_UBYTE): {
                *data_type = TYPE_UNSIGNED_CHAR;
                dvec = (char*)malloc((size_t)ndata * sizeof(unsigned char));
                break;
            }

            case (NC_CHAR): {
                *data_type = TYPE_STRING;
                dvec = (char*)malloc((size_t)ndata * sizeof(char));
                break;
            }

            case (NC_STRING): {
                *data_type = TYPE_STRING;                    // Treated as a byte/char array
                svec = (char**)malloc(
                        (size_t)ndata * sizeof(char*));        // Array of pointers to string array elements
                break;
            }

//----------------------------------------------------------------------
// Must be a user defined type: COMPLEX, OPAQUE, VLEN, COMPOUND and ENUMERATED

            default: {

                if (vartype == ctype) {                        // known User Defined types
                    *data_type = TYPE_COMPLEX;
                    dvec = (char*)malloc((size_t)ndata * sizeof(COMPLEX));
                } else {
                    if (vartype == dctype) {
                        *data_type = TYPE_DCOMPLEX;
                        dvec = (char*)malloc((size_t)ndata * sizeof(DCOMPLEX));
                    } else {

// Create User defined data structure definitions: descriptions of the internal composition (Global Structure)

                        if ((err = scopedUserDefinedTypes(grpid)) != 0) break;

// Identify the required structure definition

                        *udt = findUserDefinedType("",
                                                   (int)vartype);        // Identify via type id (assuming local to this group)

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
                            *udt = findUserDefinedType("", (int)vartype);        // Should be found now if within scope
                        }

                        if (*udt == NULL) {
                            err = 999;
                            addIdamError(&idamerrorstack, CODEERRORTYPE, "readCDF4SubTreeVar3Data", err,
                                         "User Defined Type definition Not within Scope!");
                            break;
                        }

// Read the Data - All user defined structures are COMPOUND: No special treatment for VLEN etc.
// Check for consistency with the structure definitions

                        *data_type = TYPE_COMPOUND;

                        if ((*udt)->idamclass == TYPE_ENUM) {
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

        if (err != 0) break;

        if (ndata == 0) {
            if (dvec != NULL) free((void*)dvec);
            dvec = NULL;
        } else {
            if (dvec == NULL && vartype != NC_STRING) {
                err = NETCDF_ERROR_ALLOCATING_HEAP_6;
                addIdamError(&idamerrorstack, CODEERRORTYPE, "readCDF4SubTreeVar3Data", err,
                             "Unable to Allocate Heap Memory for the Data");
                break;
            }
        }

        if (ndata > 0) {
            if (rank > 0) {

                if (vartype == NC_STRING) {
                    if (rank > 1) {
                        err = 999;
                        addIdamError(&idamerrorstack, CODEERRORTYPE, "readCDF4SubTreeVar3Data", err,
                                     "String Array rank is too large - must be 1");
                        break;
                    }

                    rc = nc_get_var_string(grpid, varid, svec);

// Pack the strings for compliance with the legacy middleware

                    replaceStrings(svec, &ndata, &dvec, (int*)dnums);    // Assume int* and size_t* are the same!!!

                } else {

                    rc = nc_get_vara(grpid, varid, start, count, (void*)dvec);

// If the data has a User Defined Type with a NC_STRING component then intercept and replace with a locally allocated component

                    replaceEmbeddedStrings(*udt, ndata, dvec);

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
                addIdamError(&idamerrorstack, CODEERRORTYPE, "readCDF4SubTreeVar3Data", err, (char*)nc_strerror(rc));
                break;
            }

            if (*udt != NULL) {

                if ((*udt)->idamclass == TYPE_COMPOUND) {            // Compound Types
                    addMalloc(dvec, ndata, (*udt)->size, (*udt)->name);    // Free Data via Malloc Log List
                }

                if ((*udt)->idamclass == TYPE_ENUM) {        // Enumerated Types
                    int i;
                    char value[8];
                    ENUMLIST* enumlist = (ENUMLIST*)malloc(sizeof(ENUMLIST));
                    nc_type base;
                    size_t size, members;
                    char name[NC_MAX_NAME + 1];

                    if ((rc = nc_inq_enum(grpid, vartype, name, &base, &size, &members)) != NC_NOERR) {
                        err = 999;
                        addIdamError(&idamerrorstack, CODEERRORTYPE, "readCDF4SubTreeVar3Data", err,
                                     (char*)nc_strerror(rc));
                        break;
                    }

                    if (members == 0) {
                        err = 999;
                        addIdamError(&idamerrorstack, CODEERRORTYPE, "readCDF4SubTreeVar3Data", err,
                                     "Enumerated Type has Zero Membership?");
                        break;
                    }

                    strcpy(enumlist->name, name);
                    enumlist->type = convertNCType(base);
                    enumlist->count = (int)members;
                    enumlist->enummember = (ENUMMEMBER*)malloc(members * sizeof(ENUMMEMBER));

                    for (i = 0; i < members; i++) {
                        rc = nc_inq_enum_member(grpid, vartype, i, enumlist->enummember[i].name, (void*)value);
                        switch (enumlist->type) {
                            case (TYPE_CHAR): {
                                char* enumvalue = (char*)value;
                                enumlist->enummember[i].value = (long long)*enumvalue;
                                break;
                            }
                            case (TYPE_SHORT): {
                                short* enumvalue = (short*)value;
                                enumlist->enummember[i].value = (long long)*enumvalue;
                                break;
                            }
                            case (TYPE_UNSIGNED_SHORT): {
                                unsigned short* enumvalue = (unsigned short*)value;
                                enumlist->enummember[i].value = (long long)*enumvalue;
                                break;
                            }
                            case (TYPE_INT): {
                                short* enumvalue = (short*)value;
                                enumlist->enummember[i].value = (long long)*enumvalue;
                                break;
                            }
                            case (TYPE_UNSIGNED_INT): {
                                unsigned int* enumvalue = (unsigned int*)value;
                                enumlist->enummember[i].value = (long long)*enumvalue;
                                break;
                            }
                            case (TYPE_LONG64): {
                                long long* enumvalue = (long long*)value;
                                enumlist->enummember[i].value = (long long)*enumvalue;
                                break;
                            }
                            case (TYPE_UNSIGNED_LONG64): {
                                unsigned long long* enumvalue = (unsigned long long*)value;
                                enumlist->enummember[i].value = (long long)*enumvalue;
                                break;
                            }
                        }
                    }

                    enumlist->data = (void*)dvec;
                    dvec = (char*)enumlist;

// Add mallocs to list for freeing when data dispatch complete

                    addMalloc((void*)enumlist, 1, sizeof(ENUMLIST), "ENUMLIST");
                    addMalloc((void*)enumlist->enummember, (int)members, sizeof(ENUMMEMBER), "ENUMMEMBER");
                    addMalloc((void*)enumlist->data, ndata, (int)size,
                              idamNameType(idamAtomicType(base)));            // Use true integer type

                }
            }    // End of if(*udt != NULL)
        }        // End of if(ndata > 0)

        *data = dvec;            // Data Array
        *ndvec = ndata;            // Count

        if (rank > 1) {
            int* sh = (int*)malloc(rank * sizeof(int));        // return the shape of the array
            for (i = 0; i < rank; i++)sh[i] = dnums[i];
            *shape = sh;
        } else {
            *shape = NULL;
        }


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


int getCDF4SubTreeData(void** data, GROUP* group, HGROUPS* hgroups)
{

// Now Recursively walk the sub-tree to read all data

    int i, size, err = 0;
    int fieldid, varid;
    int fieldcount = group->udt->fieldcount;

    char* p0, * d;
    VOIDTYPE* p;
    GROUP* grp;

//----------------------------------------------------------------------
// Allocate heap for this structure

    *data = (void*)malloc(group->udt->size);
    addMalloc(*data, 1, group->udt->size, group->udt->name);

// Start address of the Structure

    p0 = (char*)*((char**)data);

// Locate the next field (udt compound field must be synchronised to the group elements)

    fieldid = 0;


/*
struct DIMM{
   char *dimensions;
   float *data;
};
typedef struct DIMM DIMM;
struct GEOMETRY_TYPE{
   char *dimensions;
   int *data;
};
typedef struct GEOMETRY_TYPE GEOMETRY_TYPE;
struct OBLIQUE1A{
   DIMM *r;
   DIMM *z;
   DIMM *length;
   DIMM *thickness;
   DIMM *alpha;
   DIMM *beta;
};
typedef struct OBLIQUE1A OBLIQUE1A;

struct OBLIQUE1{
   float *r;
   float *z;
   float *length;
   float *thickness;
   float *alpha;
   float *beta;
};
typedef struct OBLIQUE1 OBLIQUE1;

struct IDS1{
   int *geometry_type;
   OBLIQUE1 *oblique;
};
typedef struct IDS1 IDS1;
struct IDS1A{
   GEOMETRY_TYPE *geometry_type;
   OBLIQUE1A *oblique;
};
typedef struct IDS1A IDS1A;
struct ROOT1{
   IDS1 *ids1;
};
typedef struct ROOT1 ROOT1;
struct ROOT1A{
   IDS1A *ids1;
};
typedef struct ROOT1A ROOT1A;

ROOT1  *x = (ROOT1  *)p0;
ROOT1A *y = (ROOT1A *)p0;
*/
//----------------------------------------------------------------------
// Start Error Trap

    do {

//----------------------------------------------------------------------
// Read Group Attributes

        varid = NC_GLOBAL;

        for (i = 0; i < group->numatts; i++) {

// Check field synchronisation is bounded

            if (fieldid > fieldcount) {
                err = 999;
                addIdamError(&idamerrorstack, CODEERRORTYPE, "readCDF4SubTree", err,
                             "Structure Field (attribute) Count Exceeded!");
                break;
            }

// Locate the next data position within the structure (a pointer or an opaque block of bytes)

            p = (VOIDTYPE*)&p0[group->udt->compoundfield[fieldid++].offset];
            *p = 0;

            *p = 0;    // Set the pointer to NULL (benign action if opaque block)

// DGM TODO: set all bytes in opaque block to NULL;

// Regular array of data?

            if (group->attribute[i].udt == NULL) {

                if (group->attribute[i].atttype == NC_CHAR ||
                    group->attribute[i].atttype == NC_STRING) {

                    int attlength = group->attribute[i].attlength;

                    if (group->attribute[i].atttype == NC_CHAR) {        // Legacy string

                        attlength = attlength + 1;
                        d = (char*)malloc(attlength * sizeof(char));
                        addMalloc(d, 1, attlength * sizeof(char), "char");

                        if ((err = nc_get_att_text(group->grpid, varid, group->attribute[i].attname, d)) != NC_NOERR) {
                            err = 999;
                            addIdamError(&idamerrorstack, CODEERRORTYPE, "readCDF4SubTree", err,
                                         "Problem Reading Legacy String Group Attribute");
                            break;
                        }

                        d[group->attribute[i].attlength] = '\0';        // Ensure NULL terminated

                    } else {                        // Array of strings

                        int istr, lstr;
                        char** svec = (char**)malloc((size_t)attlength *
                                                     sizeof(char*));    // The attribute length is the number of strings
                        if ((err = nc_get_att(group->grpid, varid, group->attribute[i].attname, (void*)svec)) !=
                            NC_NOERR) {
                            err = 999;
                            addIdamError(&idamerrorstack, CODEERRORTYPE, "readCDF4SubTree", err,
                                         "Problem Reading String Group Attribute");
                            break;
                        }

                        char** ssvec = (char**)malloc((size_t)attlength * sizeof(char*));// Array of strings
                        addMalloc((void*)ssvec, attlength, sizeof(char*), "STRING *");
                        for (istr = 0; istr < attlength; istr++) {                    // Copy into locally managed Heap
                            lstr = (int)strlen(svec[istr]) + 1;
                            d = (char*)malloc(lstr * sizeof(char));
                            addMalloc(d, lstr, sizeof(char), "char");            // Individual strings
                            strcpy(d, svec[istr]);
                            ssvec[istr] = d;
                        }
                        *p = (VOIDTYPE)ssvec;                        // Slot String Array into structure

                        nc_free_string(attlength, svec);
                        free(svec);
                        d = NULL;
                    }
                } else {    // Numeric data

                    int idamType = convertNCType(group->attribute[i].atttype);
                    if (idamType == TYPE_UNKNOWN) {
                        err = 999;
                        addIdamError(&idamerrorstack, CODEERRORTYPE, "readCDF4SubTree", err,
                                     "Variable Attribute has Unknown Atomic Type or User Defined Type!");
                        break;
                    }
                    size = getsizeof(idamNameType(idamType));
                    d = (char*)malloc(group->attribute[i].attlength * size);
                    addMalloc(d, group->attribute[i].attlength, size, idamNameType(idamType));
                    if ((err = nc_get_att(group->grpid, varid, group->attribute[i].attname, (void*)d)) != NC_NOERR) {
                        err = 999;
                        addIdamError(&idamerrorstack, CODEERRORTYPE, "readCDF4SubTree", err,
                                     "Problem reading Variable Attribute Value!");
                        break;
                    }
                }


            } else {        // User Defined Type data array

// Modified Functionality 15Feb2013: dgm

                if (group->attribute[i].udt->idamclass != TYPE_ENUM) {
                    size = group->attribute[i].udt->size;
                    d = (char*)malloc(group->attribute[i].attlength * size);
                    addMalloc(d, group->attribute[i].attlength, size, group->attribute[i].udt->name);
                    if ((err = nc_get_att(group->grpid, varid, group->attribute[i].attname, (void*)d)) != NC_NOERR) {
                        err = 999;
                        addIdamError(&idamerrorstack, CODEERRORTYPE, "readCDF4SubTree", err,
                                     "Problem reading Variable Attribute Value!");
                        break;
                    }
                } else {

                    int j, rc;
                    char value[8];
                    ENUMLIST* enumlist = (ENUMLIST*)malloc(sizeof(ENUMLIST));
                    nc_type base;
                    size_t size, members;
                    char name[NC_MAX_NAME + 1];

// Locate the group with the enum type definition

                    if ((rc = nc_inq_enum(group->grpid, group->attribute[i].atttype, name, &base, &size, &members)) !=
                        NC_NOERR) {
                        err = 999;
                        addIdamError(&idamerrorstack, CODEERRORTYPE, "readCDF4SubTree", err, (char*)nc_strerror(rc));
                        break;
                    }

                    if (members == 0) {
                        err = 999;
                        addIdamError(&idamerrorstack, CODEERRORTYPE, "readCDF4SubTree", err,
                                     "Enumerated Type has Zero Membership!");
                        break;
                    }

                    strcpy(enumlist->name, name);
                    enumlist->type = convertNCType(base);
                    enumlist->count = members;
                    enumlist->enummember = (ENUMMEMBER*)malloc(members * sizeof(ENUMMEMBER));

                    for (j = 0; j < members; j++) {
                        rc = nc_inq_enum_member(group->grpid, group->attribute[i].atttype, j,
                                                enumlist->enummember[j].name, (void*)value);
                        switch (enumlist->type) {
                            case (TYPE_CHAR): {
                                char* enumvalue = (char*)value;
                                enumlist->enummember[j].value = (long long)*enumvalue;
                                break;
                            }
                            case (TYPE_SHORT): {
                                short* enumvalue = (short*)value;
                                enumlist->enummember[j].value = (long long)*enumvalue;
                                break;
                            }
                            case (TYPE_UNSIGNED_SHORT): {
                                unsigned short* enumvalue = (unsigned short*)value;
                                enumlist->enummember[j].value = (long long)*enumvalue;
                                break;
                            }
                            case (TYPE_INT): {
                                short* enumvalue = (short*)value;
                                enumlist->enummember[j].value = (long long)*enumvalue;
                                break;
                            }
                            case (TYPE_UNSIGNED_INT): {
                                unsigned int* enumvalue = (unsigned int*)value;
                                enumlist->enummember[j].value = (long long)*enumvalue;
                                break;
                            }
                            case (TYPE_LONG64): {
                                long long* enumvalue = (long long*)value;
                                enumlist->enummember[j].value = (long long)*enumvalue;
                                break;
                            }
                            case (TYPE_UNSIGNED_LONG64): {
                                unsigned long long* enumvalue = (unsigned long long*)value;
                                enumlist->enummember[j].value = (long long)*enumvalue;
                                break;
                            }
                        }
                    }

                    d = (char*)malloc(group->attribute[i].attlength *
                                      sizeof(long long));    // Sufficient space ignoring integer type
                    if ((rc = nc_get_att(group->grpid, varid, group->attribute[i].attname, (void*)d)) != NC_NOERR) {
                        err = 999;
                        addIdamError(&idamerrorstack, CODEERRORTYPE, "readCDF4SubTree", err,
                                     "Problem reading Variable Attribute Value!");
                        break;
                    }

                    enumlist->data = (void*)d;
                    d = (char*)enumlist;

// Add mallocs to list for freeing when data dispatch complete

                    addMalloc((void*)enumlist, 1, sizeof(ENUMLIST), "ENUMLIST");
                    addMalloc((void*)enumlist->enummember, members, sizeof(ENUMMEMBER), "ENUMMEMBER");
                    addMalloc((void*)enumlist->data, group->attribute[i].attlength, size,
                              idamNameType(idamAtomicType(base)));            // Use true integer type
                }

// End of Modified Functionality 15Feb2013

// String replacement

                replaceSubTreeEmbeddedStrings(group->attribute[i].udt, group->attribute[i].attlength, d);

            }

            if (d != NULL) *p = (VOIDTYPE)d;        // Slot Data into structure

// DGM TODO: All data are assumed pointer types - inconsistent with ITER IMAS IDS
// DGM TODO: When the data is Not a pointer, ensure netCDF API writes directly into the structure to avoid a double copy.

        }

//----------------------------------------------------------------------
// Read Variables

        unsigned short noDimensionData = readCDF4Properties() & NC_NODIMENSIONDATA;
        unsigned short noAttributeData = readCDF4Properties() & NC_NOATTRIBUTEDATA;
        unsigned short noVarAttributeData = readCDF4Properties() & NC_NOVARATTRIBUTEDATA;

        unsigned short regularVarData = (noVarAttributeData || noAttributeData) && noDimensionData;

        for (i = 0; i < group->numvars; i++) {

            if (fieldid > fieldcount) {
                err = 999;
                addIdamError(&idamerrorstack, CODEERRORTYPE, "readCDF4SubTree", err,
                             "Structure Field (variable) Count Exceeded!");
                break;
            }

// DGM TODO: The offsets must match the size of the data objects returned from the netCDF API when the structures are Not pointer based.

            p = (VOIDTYPE*)&p0[group->udt->compoundfield[fieldid++].offset];
            *p = 0;

            if (regularVarData) {
                if ((err = getCDF4SubTreeVar2Data(group->grpid, (void**)&d, &group->variable[i],
                                                  &(group->udt->compoundfield[fieldid - 1]))) != 0) {
                                                      return err;
                }
            } else if ((err = getCDF4SubTreeVarData(group->grpid, (void**)&d, &group->variable[i])) != 0) {
                return err;
            }

            *p = (VOIDTYPE)d;
        }

//----------------------------------------------------------------------
// For each child group Recursively Drill down and repeat

        for (i = 0; i < group->numgrps; i++) {
            if ((grp = findHGroup(hgroups, group->grpids[i])) != NULL) {

                if (fieldid > fieldcount) {
                    err = 999;
                    addIdamError(&idamerrorstack, CODEERRORTYPE, "readCDF4SubTree", err,
                                 "Structure Field (group) Count Exceeded!");
                    break;
                }

                p = (VOIDTYPE*)&p0[group->udt->compoundfield[fieldid++].offset];
                *p = 0;
                if ((err = getCDF4SubTreeData((void**)&d, grp, hgroups)) != 0) return err;
                *p = (VOIDTYPE)d;
            } else {
                err = 999;
                addIdamError(&idamerrorstack, CODEERRORTYPE, "readCDF4SubTree", err,
                             "Problem locating Hierarchical Group Child!");
                break;
            }
        }

//----------------------------------------------------------------------
// Empty Groups

        if (group->numgrps == 0 && group->numvars == 0 && group->numatts == 0 &&
            group->udt->fieldcount == 1 && STR_EQUALS(group->udt->compoundfield[0].name, NC_EMPTY_GROUP_VAR_NAME)) {
            d = (char*)malloc(sizeof(char));
            addMalloc(d, 1, sizeof(char), "char");
            d[0] = '\0';                // Ensure NULL terminated
            p = (VOIDTYPE*)&p0[group->udt->compoundfield[0].offset];
            *p = 0;
            if (d != NULL) *p = (VOIDTYPE)d;        // Slot Empty string into structure
        }

//----------------------------------------------------------------------
// End of Error Trap

    } while (0);

//----------------------------------------------------------------------
// Housekeeping

    return err;
}
