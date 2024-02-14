#pragma once

#include <rpc/rpc.h>

#include "genStructs.h"

namespace uda::structures
{

int xdrUserDefinedData(XDR* xdrs, LogMallocList* logmalloclist, LogStructList* log_struct_list,
                       UserDefinedTypeList* userdefinedtypelist, UserDefinedType* userdefinedtype, void** data,
                       int datacount, int structRank, int* structShape, int index, NTree** NTree, int protocolVersion,
                       int malloc_source);

}
