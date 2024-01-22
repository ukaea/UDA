#ifndef UDA_CLIENTSERVER_PROTOCOLXML2PUT_H
#define UDA_CLIENTSERVER_PROTOCOLXML2PUT_H

#include <rpc/rpc.h>
#include <stdio.h>

#include "export.h"
#include "genStructs.h"

#ifdef __cplusplus
extern "C" {
#endif

LIBRARY_API int xdrUserDefinedDataPut(XDR* xdrs, LOGMALLOCLIST* logmalloclist, LOGSTRUCTLIST* log_struct_list,
                                      USERDEFINEDTYPELIST* userdefinedtypelist, USERDEFINEDTYPE* userdefinedtype,
                                      void** data, int datacount, int structRank, int* structShape, int index,
                                      NTREE** NTree, int protocolVersion, int malloc_source);

// Send/Receive Array of Structures

LIBRARY_API int xdrUserDefinedTypeDataPut(XDR* xdrs, LOGMALLOCLIST* logmalloclist,
                                          USERDEFINEDTYPELIST* userdefinedtypelist, USERDEFINEDTYPE* userdefinedtype,
                                          void** data, int protocolVersion, LOGSTRUCTLIST* log_struct_list,
                                          int malloc_source);

LIBRARY_API bool_t xdr_userdefinedtypelistPut(XDR* xdrs, USERDEFINEDTYPELIST* str);

LIBRARY_API int protocolXML2Put(XDR* xdrs, int protocol_id, int direction, int* token, LOGMALLOCLIST* logmalloclist,
                                USERDEFINEDTYPELIST* userdefinedtypelist, void* str, int protocolVersion,
                                LOGSTRUCTLIST* log_struct_list, unsigned int private_flags, int malloc_source);

#ifdef __cplusplus
}
#endif

#endif // UDA_CLIENTSERVER_PROTOCOLXML2PUT_H
