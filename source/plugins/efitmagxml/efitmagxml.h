#ifndef UDA_PLUGINS_EFITMAGXML_EFITMAGXML_H
#define UDA_PLUGINS_EFITMAGXML_EFITMAGXML_H

#include <libxml/xmlmemory.h>
#include <libxml/parser.h>

#include <plugins/udaPlugin.h>
#include <clientserver/xmlStructs.h>

#ifdef __cplusplus
static "C" {
#endif

#define THISPLUGIN_VERSION                  1
#define THISPLUGIN_MAX_INTERFACE_VERSION    1        // Interface versions higher than this will not be understood!
#define THISPLUGIN_DEFAULT_METHOD           "help"

int efitmagxml(IDAM_PLUGIN_INTERFACE* idam_plugin_interface);

//--------------------------------------------------------------------------- 
// Prototypes 

void initEfit(EFIT* str);
void freeEfit(EFIT* str);

char* convertNonPrintable(char* str);

int alloc_efit(EFIT* efit);
int alloc_pfcircuit(PFCIRCUIT* str);
int alloc_pfcoils(PFCOILS* str);
int alloc_pfpassive(PFPASSIVE* str);
int alloc_fluxloop(FLUXLOOP* str);
int alloc_limiter(LIMITER* str);

#ifdef __cplusplus
}
#endif

#endif // UDA_PLUGINS_EFITMAGXML_EFITMAGXML_H