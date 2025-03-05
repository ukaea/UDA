#include "teststructs.h"

void init_structure_definitions(UDA_PLUGIN_INTERFACE* plugin_interface)
{
    int offset = 0;
    int shape[2] = {};
    COMPOUNDFIELD* fields[5] = {};

    shape[0] = 56;
    fields[0] = udaNewCompoundFixedStringField("v1", "string structure element", &offset, 1, shape);

    shape[0] = 56;
    shape[1] = 3;
    fields[1] = udaNewCompoundFixedStringField("v2", "string structure element", &offset, 2, shape);

    fields[2] = udaNewCompoundVarStringField("v3", "string structure element", &offset);

    shape[0] = 3;
    fields[3] = udaNewCompoundVarStringArrayField("v4", "string structure element", &offset, 1, shape);

    fields[4] = udaNewCompoundVarStringArrayField("v5", "string structure element", &offset, 0, nullptr);

    USERDEFINEDTYPE* test9_type = udaNewUserType("TEST9", "Test #9", sizeof(TEST9), 5, fields);

    udaAddUserType(plugin_interface, test9_type);

    UDA_PLUGIN_LOG(plugin_interface, "Type TEST9 defined\n");

    COMPOUNDFIELD* test9a_fields[6] = {};
    test9a_fields[0] = fields[0];
    test9a_fields[1] = fields[1];
    test9a_fields[2] = fields[2];
    test9a_fields[3] = fields[3];
    test9a_fields[4] = fields[4];

    test9a_fields[5] = udaNewCompoundUserTypeField("v4", "string structure element", &offset, test9_type);

    USERDEFINEDTYPE* test9a_user_type =
        udaNewUserType("TEST9A", "Test #9A", sizeof(TEST9A), 6, test9a_fields);

    udaAddUserType(plugin_interface, test9a_user_type);

    UDA_PLUGIN_LOG(plugin_interface, "Type TEST9A defined\n");
}