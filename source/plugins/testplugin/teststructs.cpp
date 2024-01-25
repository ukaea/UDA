#include "teststructs.h"

#include "struct.h"
#include <include/logging.h>

void init_structure_definitions(IDAM_PLUGIN_INTERFACE* idam_plugin_interface)
{
    USERDEFINEDTYPE* old;

    USERDEFINEDTYPE usertype;
    udaInitUserDefinedType(&usertype); // New structure definition

    strcpy(usertype.name, "TEST9");
    strcpy(usertype.source, "Test #9");
    usertype.ref_id = 0;
    usertype.imagecount = 0; // No Structure Image data
    usertype.image = nullptr;
    usertype.size = sizeof(TEST9); // Structure size
    usertype.idamclass = UDA_TYPE_COMPOUND;

    int offset = 0;

    COMPOUNDFIELD field;
    udaInitCompoundField(&field);
    strcpy(field.name, "v1");
    field.atomictype = UDA_TYPE_STRING;
    strcpy(field.type, "STRING"); // convert atomic type to a string label
    strcpy(field.desc, "string structure element: char [56]");
    field.pointer = 0;
    field.count = 56;
    field.rank = 1;
    field.shape = (int*)malloc(field.rank * sizeof(int)); // Needed when rank >= 1
    field.shape[0] = field.count;
    field.size = field.count * sizeof(char);
    field.offset = newoffset(offset, field.type);
    field.offpad = padding(offset, field.type);
    field.alignment = udaGetalignmentof(field.type);
    offset = field.offset + field.size; // Next Offset
    udaAddCompoundField(&usertype, field); // Single Structure element

    udaInitCompoundField(&field);
    strcpy(field.name, "v2");
    field.atomictype = UDA_TYPE_STRING;
    strcpy(field.type, "STRING"); // convert atomic type to a string label
    strcpy(field.desc, "string structure element: char [3][56]");
    field.pointer = 0;
    field.count = 3 * 56;
    field.rank = 2;
    field.shape = (int*)malloc(field.rank * sizeof(int)); // Needed when rank >= 1
    field.shape[0] = 56;
    field.shape[1] = 3;
    field.size = field.count * sizeof(char);
    field.offset = newoffset(offset, field.type);
    field.offpad = padding(offset, field.type);
    field.alignment = udaGetalignmentof(field.type);
    offset = field.offset + field.size; // Next Offset
    udaAddCompoundField(&usertype, field); // Single Structure element

    udaInitCompoundField(&field);
    strcpy(field.name, "v3");
    field.atomictype = UDA_TYPE_STRING;
    strcpy(field.type, "STRING"); // convert atomic type to a string label
    strcpy(field.desc, "string structure element: char *");
    field.pointer = 1;
    field.count = 1;
    field.rank = 0;
    field.shape = nullptr;
    field.size = field.count * sizeof(char*);
    field.offset = newoffset(offset, field.type);
    field.offpad = padding(offset, field.type);
    field.alignment = udaGetalignmentof(field.type);
    offset = field.offset + field.size; // Next Offset
    udaAddCompoundField(&usertype, field); // Single Structure element

    udaInitCompoundField(&field);
    strcpy(field.name, "v4");
    field.atomictype = UDA_TYPE_STRING;
    strcpy(field.type, "STRING *"); // Array of String pointers
    strcpy(field.desc, "string structure element: char *[3]");
    field.pointer = 0;
    field.count = 3;
    field.rank = 1;
    field.shape = (int*)malloc(field.rank * sizeof(int)); // Needed when rank >= 1
    field.shape[0] = 3;
    field.size = field.count * sizeof(char*);
    field.offset = newoffset(offset, field.type);
    field.offpad = padding(offset, field.type);
    field.alignment = udaGetalignmentof(field.type);
    offset = field.offset + field.size; // Next Offset
    udaAddCompoundField(&usertype, field); // Single Structure element

    udaInitCompoundField(&field);
    strcpy(field.name, "v5");
    field.atomictype = UDA_TYPE_STRING;
    strcpy(field.type, "STRING *"); // Array of String pointers
    strcpy(field.desc, "string structure element: char **");
    field.pointer = 1;
    field.count = 1;
    field.rank = 0;
    field.shape = nullptr;
    field.size = field.count * sizeof(char**);
    field.offset = newoffset(offset, field.type);
    field.offpad = padding(offset, field.type);
    field.alignment = udaGetalignmentof(field.type);
    udaAddCompoundField(&usertype, field); // Single Structure element

    USERDEFINEDTYPELIST* userdefinedtypelist = idam_plugin_interface->userdefinedtypelist;
    udaAddUserDefinedType(userdefinedtypelist, usertype);

    UDA_LOG(UDA_LOG_DEBUG, "Type TEST9 defined\n");

    old = findUserDefinedType(userdefinedtypelist, "TEST9", 0); // Clone existing structure & modify
    udaCopyUserDefinedType(old, &usertype);

    UDA_LOG(UDA_LOG_DEBUG, "Type TEST9 located\n");

    strcpy(usertype.name, "TEST9A");
    strcpy(usertype.source, "Test #9A");
    usertype.size = sizeof(TEST9A); // Structure size

    offset = old->size;

    udaInitCompoundField(&field);
    strcpy(field.name, "v6");
    field.atomictype = UDA_TYPE_UNKNOWN;
    strcpy(field.type, "TEST9"); // Array of String pointers
    strcpy(field.desc, "string structure elements with sub structure");
    field.pointer = 0;
    field.count = 1;
    field.rank = 0;
    field.shape = nullptr;
    field.size = field.count * sizeof(TEST9);
    field.offset = newoffset(offset, field.type);
    field.offpad = padding(offset, field.type);
    field.alignment = udaGetalignmentof(field.type);
    udaAddCompoundField(&usertype, field); // Single Structure element

    udaAddUserDefinedType(userdefinedtypelist, usertype);

    UDA_LOG(UDA_LOG_DEBUG, "Type TEST9A defined\n");
}
