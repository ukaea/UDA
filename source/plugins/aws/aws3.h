#ifndef UDA_PLUGINS_AMAZONS3_H
#define UDA_PLUGINS_AMAZONS3_H

#ifdef __cplusplus
extern "C" {
#endif

#include <plugins/udaPlugin.h>
#include <plugins/serverPlugin.h>
#include <client/udaClient.h>
#include <clientserver/compressDim.h>

#define THISPLUGIN_VERSION                  1
#define THISPLUGIN_MAX_INTERFACE_VERSION    1        // Interface versions higher than this will not be understood!
#define THISPLUGIN_DEFAULT_METHOD           "get"

typedef struct S3_OBJECT {
    unsigned short struct_version;	// How to interpret this structure 
    unsigned short uda_version;		// defines the serialisation
    unsigned int object_size;		// The object size
    char *bucketName;
    char *objectName;    
    void *object;
} S3_OBJECT;
   

int s3(IDAM_PLUGIN_INTERFACE* idam_plugin_interface);

#ifdef __cplusplus
}
#endif

#endif // UDA_PLUGINS_AMAZONS3_H
