#include "cache.h"

#include "clientserver/init_structs.h"
#include "clientserver/protocol.h"
#include "clientserver/xdrlib.h"

using namespace uda::client_server;
using namespace uda::structures;

void writeCacheData(FILE* fp, LogMallocList* logmalloclist, UserDefinedTypeList* userdefinedtypelist,
                    const DataBlock* data_block, int protocolVersion, LogStructList* log_struct_list,
                    unsigned int private_flags, int malloc_source)
{
    XDR xdrs;
    xdrstdio_create(&xdrs, fp, XDR_ENCODE);

    ProtocolId token;
    DataBlockList data_block_list;
    data_block_list.count = 1;
    data_block_list.data = (DataBlock*)data_block;
    protocol2(&xdrs, ProtocolId::DataBlockList, XDRStreamDirection::Send, &token, logmalloclist, userdefinedtypelist,
              &data_block_list, protocolVersion, log_struct_list, private_flags, malloc_source);

    xdr_destroy(&xdrs); // Destroy before the  file otherwise a segmentation error occurs
}

DataBlock* readCacheData(FILE* fp, LogMallocList* logmalloclist, UserDefinedTypeList* userdefinedtypelist,
                         int protocolVersion, LogStructList* log_struct_list, unsigned int private_flags,
                         int malloc_source)
{
    XDR xdrs;
    xdrstdio_create(&xdrs, fp, XDR_DECODE);

    DataBlockList data_block_list;
    init_data_block_list(&data_block_list);

    ProtocolId token;
    protocol2(&xdrs, ProtocolId::DataBlockList, XDRStreamDirection::Receive, &token, logmalloclist, userdefinedtypelist,
              &data_block_list, protocolVersion, log_struct_list, private_flags, malloc_source);

    xdr_destroy(&xdrs); // Destroy before the  file otherwise a segmentation error occurs

    return data_block_list.data;
}