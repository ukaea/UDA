#include <uda/client.h>
#include <uda/types.h>

#include "client2/thread_client.hpp"
#include "clientserver/init_structs.h"

using namespace uda::client_server;
using namespace uda::client;

int udaPutListAPI(const char* put_instruction, PUTDATA_BLOCK* put_data_block_list_in, size_t count)
{
    PutDataBlockList put_data_block_list;

    //-------------------------------------------------------------------------
    // Pass an empty structure rather than nullptr (Caller is responsible for freeing)

    if (put_data_block_list_in != nullptr) {
        for (size_t i = 0; i < count; i++) {
            put_data_block_list.push_back(*static_cast<PutDataBlock*>(&put_data_block_list_in[i]));
        }
    }

    auto& client = ThreadClient::instance();
    return client.put(put_instruction, put_data_block_list);
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

    auto& client = ThreadClient::instance();
    return client.put(put_instruction, putdata_block);
}

PUTDATA_BLOCK* udaNewPutDataBlock(UDA_TYPE data_type, int count, int rank, int* shape, const char* data)
{
    PutDataBlock* put_data = (PutDataBlock*)malloc(sizeof(PutDataBlock));
    put_data->data_type = data_type;
    put_data->count = count;
    put_data->rank = rank;
    put_data->shape = (int*)malloc(sizeof(int) * rank);
    for (int i = 0; i < rank; ++i) {
        put_data->shape[i] = shape[i];
    }
    put_data->data = data;
    return put_data;
}

void udaFreePutDataBlock(PUTDATA_BLOCK* opaque_putdata_block)
{
    if (opaque_putdata_block == nullptr) return;

    auto putdata_block = static_cast<PutDataBlock*>(opaque_putdata_block);

    // freeing whole structure later so no need to set to nullptr after freeing fields
    if (putdata_block->shape != nullptr) free(putdata_block->shape);

    // TODO: this field is not freed in the legacy c_api. Memory leak or intentional?
    // if (putdata_block->opaque_block != nullptr) free(putdata_block->opaque_block);

    free(putdata_block);
}
