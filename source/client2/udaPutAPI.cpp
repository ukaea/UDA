#include "udaPutAPI.h"

#include <clientserver/initStructs.h>

#include "thread_client.hpp"

int udaPutListAPI(const char* put_instruction, PUTDATA_BLOCK_LIST* putdata_block_list_in)
{
    PUTDATA_BLOCK_LIST empty_putdata_block_list;
    PUTDATA_BLOCK_LIST* putdata_block_list = nullptr;

    //-------------------------------------------------------------------------
    // Pass an empty structure rather than nullptr (Caller is responsible for freeing)

    if (putdata_block_list_in != nullptr)
        putdata_block_list = putdata_block_list_in;
    else {
        putdata_block_list = &empty_putdata_block_list;
        initPutDataBlockList(putdata_block_list);
    }

    auto& client = uda::client::ThreadClient::instance();
    return client.put(put_instruction, putdata_block_list);
}

int udaPutAPI(const char* put_instruction, PUTDATA_BLOCK* putdata_block_in)
{
    PUTDATA_BLOCK empty_put_data_block;
    PUTDATA_BLOCK* putdata_block = nullptr;

    //-------------------------------------------------------------------------
    // Pass an empty structure rather than nullptr (Caller is responsible for freeing)

    if (putdata_block_in != nullptr)
        putdata_block = putdata_block_in;
    else {
        putdata_block = &empty_put_data_block;
        initIdamPutDataBlock(putdata_block_in);
    }

    //-------------------------------------------------------------------------
    // Data to Put to the server

    auto& client = uda::client::ThreadClient::instance();
    return client.put(put_instruction, putdata_block);
}


