#include "teststructs.h"

void init_structure_definitions(UDA_PLUGIN_INTERFACE* plugin_interface)
{
    int offset = 0;
    int shape[2] = {};
    COMPOUNDFIELD* fields[5] = {};

    shape[0] = 56;
    fields[0] = udaNewCompoundArrayField("v1", "string structure element", &offset, ARRAYCHAR, 1, shape);

    shape[0] = 56;
    shape[1] = 3;
    fields[1] = udaNewCompoundArrayField("v2", "string structure element", &offset, ARRAYCHAR, 2, shape);

    fields[2] = udaNewCompoundField("v3", "string structure element", &offset, SCALARSTRING);

    shape[0] = 3;
    fields[3] = udaNewCompoundArrayField("v4", "string structure element", &offset, ARRAYSTRING, 1, shape);

    fields[4] = udaNewCompoundField("v4", "string structure element", &offset, ARRAYSTRING);

    USERDEFINEDTYPE* test9_type = udaNewUserType("TEST9", "Test #9", 0, 0, nullptr, sizeof(TEST9), 5, fields);

    udaAddUserType(plugin_interface, test9_type);

    udaPluginLog(plugin_interface, "Type TEST9 defined\n");

    COMPOUNDFIELD* test9a_fields[6] = {};
    test9a_fields[0] = fields[0];
    test9a_fields[1] = fields[1];
    test9a_fields[2] = fields[2];
    test9a_fields[3] = fields[3];
    test9a_fields[4] = fields[4];

    test9a_fields[5] = udaNewCompoundUserTypeField("v4", "string structure element", &offset, test9_type);

    USERDEFINEDTYPE* test9a_user_type = udaNewUserType("TEST9A", "Test #9A", 0, 0, nullptr, sizeof(TEST9A), 6, test9a_fields);

    udaAddUserType(plugin_interface, test9a_user_type);

    udaPluginLog(plugin_interface, "Type TEST9A defined\n");
}