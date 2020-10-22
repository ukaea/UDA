#include "cache.h"

#include <clientserver/xdrlib.h>
#include <clientserver/protocol.h>
#include <clientserver/initStructs.h>

void writeCacheData(FILE* fp, LOGMALLOCLIST* logmalloclist, USERDEFINEDTYPELIST* userdefinedtypelist,
                    const DATA_BLOCK* data_block, int protocolVersion)
{
    XDR xdrs;
    xdrstdio_create(&xdrs, fp, XDR_ENCODE);

    int token;
    protocol2(&xdrs, PROTOCOL_DATA_BLOCK, XDR_SEND, &token, logmalloclist, userdefinedtypelist, (void*)data_block,
              protocolVersion);

    xdr_destroy(&xdrs);     // Destroy before the  file otherwise a segmentation error occurs
}

DATA_BLOCK*
readCacheData(FILE* fp, LOGMALLOCLIST* logmalloclist, USERDEFINEDTYPELIST* userdefinedtypelist, int protocolVersion)
{
    XDR xdrs;
    xdrstdio_create(&xdrs, fp, XDR_DECODE);

    auto data_block = (DATA_BLOCK*)malloc(sizeof(DATA_BLOCK));
    initDataBlock(data_block);

    int token;
    protocol2(&xdrs, PROTOCOL_DATA_BLOCK, XDR_RECEIVE, &token, logmalloclist, userdefinedtypelist, (void*)data_block,
              protocolVersion);

    xdr_destroy(&xdrs);     // Destroy before the  file otherwise a segmentation error occurs

    return data_block;
}