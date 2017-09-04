#ifndef IDAM_STRUCTURES_XDRUSERDEFINEDDATA_H
#define IDAM_STRUCTURES_XDRUSERDEFINEDDATA_H

#include <rpc/rpc.h>

#include <structures/genStructs.h>

int xdrUserDefinedData(XDR *xdrs, LOGMALLOCLIST* logmalloclist, USERDEFINEDTYPELIST* userdefinedtypelist,
                       USERDEFINEDTYPE *userdefinedtype, void **data, int datacount, int structRank, int* structShape,
                       int index, NTREE **NTree);

#endif // IDAM_STRUCTURES_XDRUSERDEFINEDDATA_H
