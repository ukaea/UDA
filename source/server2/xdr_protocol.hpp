#pragma once

#ifndef UDA_SERVER_XDR_UDA_PROTOCOL_HPP
#define UDA_SERVER_XDR_UDA_PROTOCOL_HPP

#include <rpc/rpc.h>
#include <vector>

#include "clientserver/udaDefines.h"
#include "clientserver/udaStructs.h"
#include "structures/genStructs.h"
#include "cache/memcache.hpp"
#include "server_environment.hpp"

struct IoData
{
    int* server_tot_block_time;
    int* server_timeout;
};

namespace uda {

struct MetadataBlock;

class XdrProtocol
{
public:
    XdrProtocol();
    void create();
    void set_version(int protocol_version);

    int read_client_block(ClientBlock* client_block, LogMallocList* log_malloc_list,
                          UserDefinedTypeList* user_defined_type_list);
    int recv_request_block(REQUEST_BLOCK* request_block, LogMallocList* log_malloc_list,
                           UserDefinedTypeList* user_defined_type_list);
    int send_server_block(SERVER_BLOCK server_block, LogMallocList* log_malloc_list,
                          UserDefinedTypeList* user_defined_type_list);
    int recv_putdata_block_list(PUTDATA_BLOCK_LIST* putdata_block_list, LogMallocList* log_malloc_list,
                                UserDefinedTypeList* user_defined_type_list);

    int flush();
    int eof();

    DATA_BLOCK* read_from_cache(uda::cache::UdaCache* cache, RequestData* request, server::Environment& environment,
                                LogMallocList* log_malloc_list, UserDefinedTypeList* user_defined_type_list);
    void write_to_cache(uda::cache::UdaCache* cache, RequestData* request, uda::server::Environment& environment,
                        DataBlock* data_block, LogMallocList* log_malloc_list,
                        UserDefinedTypeList* user_defined_type_list);

    int send_meta_data(MetadataBlock& metadata_block, LogMallocList* log_malloc_list,
                       UserDefinedTypeList* user_defined_type_list);
    int send_data_blocks(const std::vector<DataBlock>& data_blocks, LogMallocList* log_malloc_list,
                         UserDefinedTypeList* user_defined_type_list);
    int send_hierachical_data(const DataBlock& data_block, LogMallocList* log_malloc_list,
                              UserDefinedTypeList* user_defined_type_list);
    int recv_client_block(SERVER_BLOCK& server_block, CLIENT_BLOCK* client_block, bool* fatal,
                          int server_tot_block_time, const int* server_timeout, LogMallocList* log_malloc_list,
                          UserDefinedTypeList* user_defined_type_list);

private:
    int protocol_version_ = 8;
    XDR server_input_;
    XDR server_output_;
    int server_tot_block_time_;
    int server_timeout_;
    IoData io_data_;
    LogStructList log_struct_list_;
    int malloc_source_;
    int private_flags_;

    void create_streams();
};

} // namespace uda

#endif // UDA_SERVER_XDR_UDA_PROTOCOL_HPP