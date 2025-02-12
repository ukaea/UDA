#include "cache.h"

#include "clientserver/init_structs.h"
#include "protocol/xdrlib.h"
#include "protocol/protocol.h"

using namespace uda::client_server;
using namespace uda::structures;

void writeCacheData(FILE* fp, LogMallocList* logmalloclist, UserDefinedTypeList* userdefinedtypelist,
                    const DataBlock* data_block, int protocolVersion, LogStructList* log_struct_list,
                    unsigned int private_flags, int malloc_source)
{
    XDR xdrs;
    xdrstdio_create(&xdrs, fp, XDR_ENCODE);

    ProtocolId token;
    std::vector<DataBlock> data_blocks;
    data_blocks.push_back(*data_block);
    protocol2(&xdrs, ProtocolId::DataBlockList, XDRStreamDirection::Send, &token, logmalloclist, userdefinedtypelist,
              &data_blocks, protocolVersion, log_struct_list, private_flags, malloc_source);

    xdr_destroy(&xdrs); // Destroy before the  file otherwise a segmentation error occurs
}

DataBlock* readCacheData(FILE* fp, LogMallocList* logmalloclist, UserDefinedTypeList* userdefinedtypelist,
                         int protocolVersion, LogStructList* log_struct_list, unsigned int private_flags,
                         int malloc_source)
{
    XDR xdrs;
    xdrstdio_create(&xdrs, fp, XDR_DECODE);

    std::vector<DataBlock> data_blocks;

    ProtocolId token;
    protocol2(&xdrs, ProtocolId::DataBlockList, XDRStreamDirection::Receive, &token, logmalloclist, userdefinedtypelist,
              &data_blocks, protocolVersion, log_struct_list, private_flags, malloc_source);

    xdr_destroy(&xdrs); // Destroy before the  file otherwise a segmentation error occurs

    DataBlock* data_block = new DataBlock;
    *data_block = data_blocks.front();

    return data_block;
}