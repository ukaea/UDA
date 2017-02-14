#ifndef IDAM_PLUGINS_PUTDATA_PUTGROUP_H
#define IDAM_PLUGINS_PUTDATA_PUTGROUP_H

#include <plugins/idamPlugin.h>

int do_group(IDAM_PLUGIN_INTERFACE* idam_plugin_interface);
int testgroup(int ncgrpid, const char* target, int* status, int* targetid);

#endif // IDAM_PLUGINS_PUTDATA_PUTGROUP_H
