#ifndef UDA_STRUCTURES_XDRUSERDEFINEDDATA_H
#define UDA_STRUCTURES_XDRUSERDEFINEDDATA_H

#include <rpc/rpc.h>

#include "genStructs.h"

int xdrUserDefinedData(XDR* xdrs, LOGMALLOCLIST* logmalloclist, LOGSTRUCTLIST* log_struct_list,
                       USERDEFINEDTYPELIST* userdefinedtypelist, USERDEFINEDTYPE* userdefinedtype, void** data,
                       int datacount, int structRank, int* structShape, int index, NTREE** NTree, int protocolVersion,
                       int malloc_source);

#endif // UDA_STRUCTURES_XDRUSERDEFINEDDATA_H
