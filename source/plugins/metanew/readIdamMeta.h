#ifndef IDAM_PLUGINS_READMETANEW_READIDAMMETA_H
#define IDAM_PLUGINS_READMETANEW_READIDAMMETA_H

#include <plugins/udaPlugin.h>
#include <server/sqllib.h>

struct DATALASTSHOT
{
    unsigned int lastshot;         // Scalar integer
};
typedef struct DATALASTSHOT DATALASTSHOT;

struct DATALASTPASS
{
    unsigned int lastpass;         // Scalar integer
};
typedef struct DATALASTPASS DATALASTPASS;

struct DATASHOTDATETIME
{
    unsigned int shot;         // Scalar integer
    char * date;
    char * time;
};
typedef struct DATASHOTDATETIME DATASHOTDATETIME;

struct DATALISTSIGNALS_C
{
    unsigned int count;
    unsigned int shot;        // Scalar integer
    int pass;            // the default is -1
    char ** signal_name;        // Array of Strings each with arbitrary length
    char ** generic_name;
    char ** source_alias;
    char ** type;
    char ** description;
    int * signal_status;
};
typedef struct DATALISTSIGNALS_C DATALISTSIGNALS_C;

struct DATALISTSIGNALSNOSTATUS_C
{
    unsigned int count;
    unsigned int shot;        // Scalar integer
    int pass;            // the default is -1
    char ** signal_name;        // Array of Strings each with arbitrary length
    char ** generic_name;
    char ** source_alias;
    char ** type;
    char ** description;
};
typedef struct DATALISTSIGNALSNOSTATUS_C DATALISTSIGNALSNOSTATUS_C;

PGconn* open_connection();

char* get_escaped_string(PGconn* DBConnect, const char* instring);

int get_lastshot(IDAM_PLUGIN_INTERFACE* idam_plugin_interface);
int get_lastpass(IDAM_PLUGIN_INTERFACE* idam_plugin_interface);
int get_shotdatetime(IDAM_PLUGIN_INTERFACE* idam_plugin_interface);
int get_listsignals(IDAM_PLUGIN_INTERFACE* idam_plugin_interface);
//int get_listsources(IDAM_PLUGIN_INTERFACE* idam_plugin_interface);

#endif // IDAM_PLUGINS_READMETANEW_READIDAMMETA_H
