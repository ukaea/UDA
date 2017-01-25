#ifndef IDAM_IDAMSERVERHELP_H
#define IDAM_IDAMSERVERHELP_H

#include <include/idamgenstruct.h>
#include <clientserver/idamStructs.h>
#include <include/idamplugin.h>

#define THISPLUGIN_VERSION                  1
#define THISPLUGIN_MAX_INTERFACE_VERSION    1
#define THISPLUGIN_DEFAULT_METHOD           "help"

#ifndef USE_PLUGIN_DIRECTLY

extern USERDEFINEDTYPELIST parseduserdefinedtypelist;
USERDEFINEDTYPELIST parseduserdefinedtypelist;

static IDAMERRORSTACK * idamErrorStack;            // Pointer to the Server's Error Stack.
#endif

int idamServerHelp(IDAM_PLUGIN_INTERFACE * idam_plugin_interface);

#endif //IDAM_IDAMSERVERHELP_H