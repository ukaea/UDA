#ifndef UDA_STRUCTURES_XDRUSERDEFINEDDATA_H
#define UDA_STRUCTURES_XDRUSERDEFINEDDATA_H

#include <rpc/rpc.h>

#include <structures/genStructs.h>

#if defined(_WIN32)
#  define LIBRARY_API __declspec(dllexport)
#else
#  define LIBRARY_API
#endif

#ifdef __cplusplus
extern "C" {
#endif

LIBRARY_API int xdrUserDefinedData(XDR* xdrs, LOGMALLOCLIST* logmalloclist, USERDEFINEDTYPELIST* userdefinedtypelist,
                       USERDEFINEDTYPE* userdefinedtype, void** data, int datacount, int structRank, int* structShape,
                       int index, NTREE** NTree, int protocolVersion);

#ifdef __cplusplus
}
#endif

#endif // UDA_STRUCTURES_XDRUSERDEFINEDDATA_H
