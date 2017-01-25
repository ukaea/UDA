#ifndef IDAM_PLUGINS_PUTDATA_PUTOPENCLOSE_H
#define IDAM_PLUGINS_PUTDATA_PUTOPENCLOSE_H

#include <include/idamplugin.h>

#define PLUGIN_VERSION 1

#define CREATE_CONVENTIONS          1
#define CREATE_CONVENTIONS_TEST     "FUSION"
#define CREATE_CLASS                2
#define CREATE_SHOT                 4
#define CREATE_PASS                 8
#define CREATE_STATUS               16

int do_open(IDAM_PLUGIN_INTERFACE* idam_plugin_interface);
int do_close(IDAM_PLUGIN_INTERFACE* idam_plugin_interface);

int get_complex_types(int fileid, int* ctype, int* dctype);
int get_file_id(int fileidx);

#endif //IDAM_PLUGINS_PUTDATA_PUTOPENCLOSE_H
