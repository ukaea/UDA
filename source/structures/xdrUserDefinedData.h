
#ifndef IDAM_XDRUSERDEFINEDDATA_H
#define IDAM_XDRUSERDEFINEDDATA_H

#include <rpc/rpc.h>
#include <include/idamgenstruct.h>

int xdrUserDefinedData(XDR *xdrs, USERDEFINEDTYPE *userdefinedtype, void **data, int datacount, int structRank, int* structShape, 
                       int index, NTREE **NTree);

#endif // IDAM_XDRUSERDEFINEDDATA_H
