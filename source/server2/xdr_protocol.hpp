#pragma once

#ifndef UDA_SERVER_XDR_UDA_PROTOCOL_HPP
#  define UDA_SERVER_XDR_UDA_PROTOCOL_HPP

#  include <rpc/rpc.h>
#  include <vector>

#  include "cache/memcache.hpp"
#  include "clientserver/protocol.h"
#  include "clientserver/udaDefines.h"
#  include "clientserver/udaStructs.h"
#  include "server_environment.hpp"
#  include "structures/genStructs.h"

namespace uda::server
{

struct IoData : uda::client_server::IoData {
    int* server_tot_block_time;
    int* server_timeout;
};

struct MetadataBlock;

class XdrProtocol
{
  public:
    XdrProtocol();
    void create();
    void set_version(int protocol_version);

    int read_client_block(uda::client_server::ClientBlock* client_block,
                          uda::structures::LogMallocList* log_malloc_list,
                          uda::structures::UserDefinedTypeList* user_defined_type_list);
    int recv_request_block(uda::client_server::RequestBlock* request_block,
                           uda::structures::LogMallocList* log_malloc_list,
                           uda::structures::UserDefinedTypeList* user_defined_type_list);
    int send_server_block(uda::client_server::ServerBlock server_block, uda::structures::LogMallocList* log_malloc_list,
                          uda::structures::UserDefinedTypeList* user_defined_type_list);
    int recv_putdata_block_list(uda::client_server::PutDataBlockList* putdata_block_list,
                                uda::structures::LogMallocList* log_malloc_list,
                                uda::structures::UserDefinedTypeList* user_defined_type_list);

    int flush();
    int eof();

    uda::client_server::DataBlock* read_from_cache(uda::cache::UdaCache* cache,
                                                   uda::client_server::RequestData* request,
                                                   server::Environment& environment,
                                                   uda::structures::LogMallocList* log_malloc_list,
                                                   uda::structures::UserDefinedTypeList* user_defined_type_list);
    void write_to_cache(uda::cache::UdaCache* cache, uda::client_server::RequestData* request,
                        uda::server::Environment& environment, uda::client_server::DataBlock* data_block,
                        uda::structures::LogMallocList* log_malloc_list,
                        uda::structures::UserDefinedTypeList* user_defined_type_list);

    int send_meta_data(MetadataBlock& metadata_block, uda::structures::LogMallocList* log_malloc_list,
                       uda::structures::UserDefinedTypeList* user_defined_type_list);
    int send_data_blocks(const std::vector<uda::client_server::DataBlock>& data_blocks,
                         uda::structures::LogMallocList* log_malloc_list,
                         uda::structures::UserDefinedTypeList* user_defined_type_list);
    int send_hierachical_data(const uda::client_server::DataBlock& data_block,
                              uda::structures::LogMallocList* log_malloc_list,
                              uda::structures::UserDefinedTypeList* user_defined_type_list);
    int recv_client_block(uda::client_server::ServerBlock& server_block, uda::client_server::ClientBlock* client_block,
                          bool* fatal, int server_tot_block_time, const int* server_timeout,
                          uda::structures::LogMallocList* log_malloc_list,
                          uda::structures::UserDefinedTypeList* user_defined_type_list);

  private:
    int protocol_version_ = 8;
    XDR server_input_;
    XDR server_output_;
    int server_tot_block_time_;
    int server_timeout_;
    uda::server::IoData io_data_;
    uda::structures::LogStructList log_struct_list_;
    int malloc_source_;
    int private_flags_;

    void create_streams();
};

} // namespace uda::server

#endif // UDA_SERVER_XDR_UDA_PROTOCOL_HPP