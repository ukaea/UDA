#ifndef UDA_STRUCTURES_XDRUSERDEFINEDDATA_H
#define UDA_STRUCTURES_XDRUSERDEFINEDDATA_H

#include <rpc/rpc.h>

#include "export.h"
#include "genStructs.h"

#ifdef __cplusplus
extern "C" {
#endif

LIBRARY_API int xdrUserDefinedData(XDR* xdrs, LOGMALLOCLIST* logmalloclist, LOGSTRUCTLIST* log_struct_list,
                                   USERDEFINEDTYPELIST* userdefinedtypelist, USERDEFINEDTYPE* userdefinedtype,
                                   void** data, int datacount, int structRank, int* structShape, int index,
                                   NTREE** NTree, int protocolVersion, int malloc_source);

#ifdef __cplusplus
}
#endif

#endif // UDA_STRUCTURES_XDRUSERDEFINEDDATA_H
