#ifndef UDA_CLIENTSERVER_PROTOCOLXML2PUT_H
#define UDA_CLIENTSERVER_PROTOCOLXML2PUT_H

#include <stdio.h>
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

LIBRARY_API int xdrUserDefinedDataPut(XDR* xdrs, LOGMALLOCLIST* logmalloclist, USERDEFINEDTYPELIST* userdefinedtypelist,
                          USERDEFINEDTYPE* userdefinedtype, void** data, int datacount, int structRank,
                          int* structShape, int index, NTREE** NTree, int protocolVersion);

// Send/Receive Array of Structures

LIBRARY_API int xdrUserDefinedTypeDataPut(XDR* xdrs, LOGMALLOCLIST* logmalloclist, USERDEFINEDTYPELIST* userdefinedtypelist,
                              USERDEFINEDTYPE* userdefinedtype, void** data, int protocolVersion);

LIBRARY_API bool_t xdr_userdefinedtypelistPut(XDR* xdrs, USERDEFINEDTYPELIST* str);

LIBRARY_API int protocolXML2Put(XDR* xdrs, int protocol_id, int direction, int* token, LOGMALLOCLIST* logmalloclist,
                    USERDEFINEDTYPELIST* userdefinedtypelist, void* str, int protocolVersion);

#ifdef __cplusplus
}
#endif

#endif // UDA_CLIENTSERVER_PROTOCOLXML2PUT_H
