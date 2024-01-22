#ifndef UDA_PLUGIN_TESTPLUGIN_TESTSTRUCTS_H
#define UDA_PLUGIN_TESTPLUGIN_TESTSTRUCTS_H

#include "pluginStructs.h"

typedef struct Test9 {
    char v1[56];    // single fixed length string
    char v2[3][56]; // 3 fixed length strings
    char* v3;       // single arbitrary length string
    char* v4[3];    // 3 strings of arbitrary length
    char** v5;      // arbitrary number of strings of arbitrary length
} TEST9;

typedef struct Test9A {
    char v1[56];    // single fixed length string
    char v2[3][56]; // 3 fixed length strings
    char* v3;       // single arbitrary length string
    char* v4[3];    // 3 strings of arbitrary length
    char** v5;      // arbitrary number of strings of arbitrary length
    TEST9 v6;       // Sub Structure with String types
} TEST9A;

void init_structure_definitions(IDAM_PLUGIN_INTERFACE* idam_plugin_interface);

#endif // UDA_PLUGIN_TESTPLUGIN_TESTSTRUCTS_H
