#include <uda/client.h>

#include "client2/thread_client.hpp"
#include "clientserver/allocData.h"
#include "clientserver/errorLog.h"
#include "clientserver/initStructs.h"
#include "clientserver/printStructs.h"
#include "logging/logging.h"

using namespace uda::client_server;
using namespace uda::client;
using namespace uda::logging;

int udaPutListAPI(const char* put_instruction, PUTDATA_BLOCK_LIST* putdata_block_list_in)
{
    PutDataBlockList empty_putdata_block_list;
    PutDataBlockList* putdata_block_list = nullptr;

    //-------------------------------------------------------------------------
    // Pass an empty structure rather than nullptr (Caller is responsible for freeing)

    if (putdata_block_list_in != nullptr)
        putdata_block_list = static_cast<PutDataBlockList*>(putdata_block_list_in);
    else {
        putdata_block_list = &empty_putdata_block_list;
        init_put_data_block_list(putdata_block_list);
    }

    auto& client = uda::client::ThreadClient::instance();
    return client.put(put_instruction, putdata_block_list);
}

int udaPutAPI(const char* put_instruction, PUTDATA_BLOCK* putdata_block_in)
{
    PutDataBlock empty_put_data_block;
    PutDataBlock* putdata_block = nullptr;

    //-------------------------------------------------------------------------
    // Pass an empty structure rather than nullptr (Caller is responsible for freeing)

    if (putdata_block_in != nullptr)
        putdata_block = static_cast<PutDataBlock*>(putdata_block_in);
    else {
        putdata_block = &empty_put_data_block;
        init_put_data_block(putdata_block);
    }

    //-------------------------------------------------------------------------
    // Data to Put to the server

    auto& client = uda::client::ThreadClient::instance();
    return client.put(put_instruction, putdata_block);
}


