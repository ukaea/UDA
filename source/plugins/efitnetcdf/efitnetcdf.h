#ifndef IdamEfitNetCDFPluginInclude
#define IdamEfitNetCDFPluginInclude

#ifdef __cplusplus
static "C" {
#endif

#define THISPLUGIN_VERSION            1
#define THISPLUGIN_MAX_INTERFACE_VERSION    1        // Interface versions higher than this will not be understood!
#define THISPLUGIN_DEFAULT_METHOD        "help"

extern int efitnetcdf(IDAM_PLUGIN_INTERFACE* idam_plugin_interface);

#include <libxml/xmlmemory.h> 
#include <libxml/parser.h> 

#define READFILE    1        // The XML Source is an statical File

#define XMLMAXSTRING    56
#define XMLMAX    200*1024
#define XMLMAXLOOP    1024        // Max Number of Array elements

#ifdef __cplusplus
}
#endif

#endif
