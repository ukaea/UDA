#ifndef UDA_STRUCTURES_XDRUSERDEFINEDDATA_H
#define UDA_STRUCTURES_XDRUSERDEFINEDDATA_H

#include <rpc/rpc.h>

#include "genStructs.h"

int xdrUserDefinedData(XDR* xdrs, LogMallocList* logmalloclist, LogStructList* log_struct_list,
                       UserDefinedTypeList* userdefinedtypelist, UserDefinedType* userdefinedtype, void** data,
                       int datacount, int structRank, int* structShape, int index, NTree** NTree, int protocolVersion,
                       int malloc_source);

#endif // UDA_STRUCTURES_XDRUSERDEFINEDDATA_H
