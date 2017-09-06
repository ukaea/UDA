
// POSTGRES Query Plugin

#ifndef UDA_PLUGINS_POSTGRES_PLUGIN_H
#define UDA_PLUGINS_POSTGRES_PLUGIN_H

// Change History:
//
// 26May2017	dgm	Original Version

#ifdef __cplusplus
extern "C" {
#endif

#include <plugins/udaPlugin.h>
#include <libpq-fe.h>

#define THISPLUGIN_VERSION			1
#define THISPLUGIN_MAX_INTERFACE_VERSION	1		// Interface versions higher than this will not be understood!
#define THISPLUGIN_DEFAULT_METHOD		"get"

// Returned database query: Single record

struct POSTGRES_R {
   char *objectName;		// The target data object's (abstracted) (signal) name
   int   expNumber;		// The target experiment number
   char *signal_name;		// The true data object's name		  
   char *type;			// The type of Data Source (e.g. P => Plugin)
   char *signal_alias;		// The alias name of the data object		  
   char *generic_name;		// The generic name for the data object
   char *source_alias;		// The class of data source
   char *signal_class;		// The class of data object (signal)		  
   char *description;		// A description of the data
   int   range_start;		// The starting (inclusive) range value of the expNumber for which this record is valid
   int   range_stop;		// The ending (inclusive) range value of the expNumber for which this record is valid
};
typedef struct POSTGRES_R POSTGRES_R ;

extern int postgres_query(IDAM_PLUGIN_INTERFACE *idam_plugin_interface);

#ifdef __cplusplus
}
#endif

#endif
