#include "xdr_protocol.hpp"
#include "clientserver/error_log.h"
#include "clientserver/print_structs.h"
#include "protocol.h"
#include "clientserver/uda_errors.h"
#include "protocol/xdr_lib.h"
#include <uda/types.h>

#if defined(SSLAUTHENTICATION) && !defined(FATCLIENT)
#  include "authentication/udaServerSSL.h"
#endif

#include <cerrno>
#include <unistd.h>

#include "clientserver/uda_defines.h"
#include "logging/logging.h"

#if !defined(__GNUC__)
#  include <io.h>
#  define read _read
#  define write _write
#endif

#ifndef _WIN32
#  include <sys/select.h>
#else
#  include <winsock.h>
#endif

using namespace uda::client_server;
using namespace uda::logging;
using namespace uda::structures;

#if defined(SSLAUTHENTICATION) && !defined(FATCLIENT)
using namespace uda::authentication;
#endif

namespace uda::protocol {

int read(void* iohandle, char* buf, int count) {
    int rc = 0;
    fd_set rfds; // File Descriptor Set for Reading from the Socket
    timeval tv = {};
    timeval tvc = {};

    auto io_data = static_cast<IoData*>(iohandle);

    // Wait until there are data to be read from the socket

    set_select_params(io_data->server_socket, &rfds, &tv, io_data->server_tot_block_time);
    tvc = tv;

    while (select(io_data->server_socket + 1, &rfds, nullptr, nullptr, &tvc) <= 0) {
        *io_data->server_tot_block_time += (int) tv.tv_usec / 1000;
        if (*io_data->server_tot_block_time > 1000 * *io_data->server_timeout) {
            UDA_LOG(UDA_LOG_DEBUG, "Total Wait Time Exceeds Lifetime Limit = {} (ms)",
                    *io_data->server_timeout * 1000);
            return -1;
        }

        update_select_params(io_data->server_socket, &rfds, &tv, *io_data->server_tot_block_time); // Keep trying ...
        tvc = tv;
    }

    // Read from it, checking for EINTR, as happens if called from IDL

    while (((rc = (int)::read(io_data->server_socket, buf, count)) == -1) && (errno == EINTR)) {}

    // As we have waited to be told that there is data to be read, if nothing
    // arrives, then there must be an error

    if (!rc) {
        rc = -1;
    }

    return rc;
}

int write(void* iohandle, char* buf, int count) {

    // This routine is only called when there is something to write back to the Client

    int rc = 0;
    int bytes_sent = 0;

    fd_set wfds; // File Descriptor Set for Writing to the Socket
    timeval tv = {};

    auto io_data = static_cast<IoData*>(iohandle);

    // Block IO until the Socket is ready to write to Client

    set_select_params(io_data->server_socket, &wfds, &tv, io_data->server_tot_block_time);

    while (select(io_data->server_socket + 1, nullptr, &wfds, nullptr, &tv) <= 0) {
        *io_data->server_tot_block_time += tv.tv_usec / 1000;
        if (*io_data->server_tot_block_time / 1000 > *io_data->server_timeout) {
            UDA_LOG(UDA_LOG_DEBUG, "Total Blocking Time: {} (ms)", *io_data->server_tot_block_time);
            return -1;
        }
        update_select_params(io_data->server_socket, &wfds, &tv, *io_data->server_tot_block_time);
    }

    // Write to socket, checking for EINTR, as happens if called from IDL

    while (bytes_sent < count) {
        while (((rc = (int) ::write(io_data->server_socket, buf, count)) == -1) && (errno == EINTR)) {}
        bytes_sent += rc;
        buf += rc;
    }

    return rc;
}

}

void uda::protocol::XdrProtocol::create()
{
    _io_data.server_socket = 0;
    create_streams();
}

void uda::protocol::XdrProtocol::set_socket(int socket_fd)
{
    _io_data.server_socket = socket_fd;
}

uda::protocol::XdrProtocol::XdrProtocol(std::vector<UdaError>& error_stack)
    : _error_stack{error_stack}
    , _server_input{}
    , _server_output{}
    , _server_tot_block_time{0}
    , _server_timeout{TimeOut}
    , _io_data{}
    , _log_struct_list{}
    , _private_flags{}
{
    _io_data.server_socket = 0;
    _io_data.server_tot_block_time = &_server_tot_block_time;
    _io_data.server_timeout = &_server_timeout;
}

void uda::protocol::XdrProtocol::create_streams()
{
    _server_output.x_ops = nullptr;
    _server_input.x_ops = nullptr;

#if defined(SSLAUTHENTICATION)
    if (getUdaServerSSLDisabled()) {
#  if defined(__APPLE__) || defined(__TIRPC__)
        xdrrec_create(&_server_output, DBReadBlockSize, DBWriteBlockSize, &_io_data,
                      reinterpret_cast<int (*)(void*, void*, int)>(uda::protocol::read),
                      reinterpret_cast<int (*)(void*, void*, int)>(uda::protocol::write));

        xdrrec_create(&_server_input, DBReadBlockSize, DBWriteBlockSize, &_io_data,
                      reinterpret_cast<int (*)(void*, void*, int)>(uda::protocol::read),
                      reinterpret_cast<int (*)(void*, void*, int)>(uda::protocol::write));
#  else
        xdrrec_create(&_server_output, DBReadBlockSize, DBWriteBlockSize, (char*)&_io_data,
                      reinterpret_cast<int (*)(char*, char*, int)>(uda::protocol::read),
                      reinterpret_cast<int (*)(char*, char*, int)>(uda::protocol::write));

        xdrrec_create(&_server_input, DBReadBlockSize, DBWriteBlockSize, (char*)&_io_data,
                      reinterpret_cast<int (*)(char*, char*, int)>(uda::protocol::read),
                      reinterpret_cast<int (*)(char*, char*, int)>(uda::protocol::write));
#  endif
    } else {
#  if defined(__APPLE__) || defined(__TIRPC__)
        xdrrec_create(&_server_output, DBReadBlockSize, DBWriteBlockSize, &_io_data,
                      reinterpret_cast<int (*)(void*, void*, int)>(readUdaServerSSL),
                      reinterpret_cast<int (*)(void*, void*, int)>(writeUdaServerSSL));

        xdrrec_create(&_server_input, DBReadBlockSize, DBWriteBlockSize, &_io_data,
                      reinterpret_cast<int (*)(void*, void*, int)>(readUdaServerSSL),
                      reinterpret_cast<int (*)(void*, void*, int)>(writeUdaServerSSL));
#  else
        xdrrec_create(&_server_output, DBReadBlockSize, DBWriteBlockSize, (char*)&_io_data,
                      reinterpret_cast<int (*)(char*, char*, int)>(readUdaServerSSL),
                      reinterpret_cast<int (*)(char*, char*, int)>(writeUdaServerSSL));

        xdrrec_create(&_server_input, DBReadBlockSize, DBWriteBlockSize, (char*)&_io_data,
                      reinterpret_cast<int (*)(char*, char*, int)>(readUdaServerSSL),
                      reinterpret_cast<int (*)(char*, char*, int)>(writeUdaServerSSL));
#  endif
    }
#else // SSLAUTHENTICATION

#  if defined(__APPLE__) || defined(__TIRPC__)
    xdrrec_create(&_server_output, DBReadBlockSize, DBWriteBlockSize, &_io_data,
                  reinterpret_cast<int (*)(void*, void*, int)>(uda::protocol::read),
                  reinterpret_cast<int (*)(void*, void*, int)>(uda::protocol::write));

    xdrrec_create(&_server_input, DBReadBlockSize, DBWriteBlockSize, &_io_data,
                  reinterpret_cast<int (*)(void*, void*, int)>(uda::protocol::read),
                  reinterpret_cast<int (*)(void*, void*, int)>(uda::protocol::write));
#  else
    xdrrec_create(&server_output_, DBReadBlockSize, DBWriteBlockSize, (char*)&_io_data,
                  reinterpret_cast<int (*)(char*, char*, int)>(uda::protocol::read),
                  reinterpret_cast<int (*)(char*, char*, int)>(uda::protocol::write));

    xdrrec_create(&_server_input, DBReadBlockSize, DBWriteBlockSize, (char*)&_io_data,
                  reinterpret_cast<int (*)(char*, char*, int)>(uda::protocol::read),
                  reinterpret_cast<int (*)(char*, char*, int)>(uda::protocol::write));
#  endif

#endif // SSLAUTHENTICATION

    _server_input.x_op = XDR_DECODE;
    _server_output.x_op = XDR_ENCODE;

    UDA_LOG(UDA_LOG_DEBUG, "XDR Streams Created");
}

int uda::protocol::XdrProtocol::send_server_block(ServerBlock server_block, LogMallocList* log_malloc_list,
                                                UserDefinedTypeList* user_defined_type_list)
{
    UDA_LOG(UDA_LOG_DEBUG, "Sending Server Block");
    print_server_block(server_block);

    ProtocolId protocol_id = ProtocolId::ServerBlock; // Receive Server Block: Server Aknowledgement

    int err;
    if ((err = protocol2(_error_stack, &_server_output, protocol_id, XDRStreamDirection::Send, nullptr, log_malloc_list, user_defined_type_list,
                         (void*)&server_block, _protocol_version, &_log_struct_list, 0, _malloc_source)) != 0) {
        UDA_THROW_ERROR(_error_stack, err, "Protocol 11 Error (Server Block #1)");
    }

    UDA_LOG(UDA_LOG_DEBUG, "Server Block sent without error");

    return 0;
}

int uda::protocol::XdrProtocol::flush()
{
    if (!xdrrec_endofrecord(&_server_output, 1)) { // Send data now
        UDA_THROW_ERROR(_error_stack, (int)ProtocolError::Error7, "Protocol 7 Error (Server Block)");
    }

    return 0;
}

void uda::protocol::XdrProtocol::reset()
{
    _malloc_source = UDA_MALLOC_SOURCE_NONE;
    _log_struct_list = {};
}

int uda::protocol::XdrProtocol::send_meta_data(MetaData& meta_data, LogMallocList* log_malloc_list,
                                             UserDefinedTypeList* user_defined_type_list)
{
    constexpr auto protocol_id = ProtocolId::MetaData;
    int err = 0;

    if ((err = protocol2(_error_stack, &_server_output, protocol_id, XDRStreamDirection::Send, nullptr, log_malloc_list, user_defined_type_list,
                         &meta_data, _protocol_version, &_log_struct_list, 0, _malloc_source)) != 0) {
        UDA_LOG(UDA_LOG_DEBUG, "Problem Sending Data System Structure");
        add_error(_error_stack, ErrorType::Code, __func__, err, "Protocol 4 Error");
        return err;
    }

    return err;
}

int uda::protocol::XdrProtocol::send_data_blocks(const std::vector<DataBlock>& data_blocks,
                                               LogMallocList* log_malloc_list,
                                               UserDefinedTypeList* user_defined_type_list)
{
    UDA_LOG(UDA_LOG_DEBUG, "Sending Data Block Structure to Client");

    int err = 0;
    if ((err = protocol2(_error_stack, &_server_output, ProtocolId::DataBlockList, XDRStreamDirection::Send, nullptr, log_malloc_list,
                         user_defined_type_list, (void*)&data_blocks, _protocol_version, &_log_struct_list, 0,
                         _malloc_source)) != 0) {
        UDA_LOG(UDA_LOG_DEBUG, "Problem Sending Data Structure");
        add_error(_error_stack, ErrorType::Code, __func__, err, "Protocol 2 Error");
        return err;
    }

    UDA_LOG(UDA_LOG_DEBUG, "Data Block Sent to Client");

    return err;
}

int uda::protocol::XdrProtocol::send_hierarchical_data(const DataBlock& data_block, LogMallocList* log_malloc_list,
                                                    UserDefinedTypeList* user_defined_type_list)
{
    if (data_block.data_type == UDA_TYPE_COMPOUND && data_block.opaque_type != UDA_OPAQUE_TYPE_UNKNOWN) {

        auto protocol_id = ProtocolId::Start;

        if (data_block.opaque_type == UDA_OPAQUE_TYPE_XML_DOCUMENT) {
            protocol_id = ProtocolId::Meta;
        } else if (data_block.opaque_type == UDA_OPAQUE_TYPE_STRUCTURES ||
            data_block.opaque_type == UDA_OPAQUE_TYPE_XDRFILE) {
            protocol_id = ProtocolId::Structures;
        }

        UDA_LOG(UDA_LOG_DEBUG, "Sending Hierarchical Data Structure to Client");

        int err = 0;
        if ((err = protocol2(_error_stack, &_server_output, protocol_id, XDRStreamDirection::Send, nullptr, log_malloc_list, user_defined_type_list,
                             (void*)&data_block, _protocol_version, &_log_struct_list, 0, _malloc_source)) != 0) {
            add_error(_error_stack, ErrorType::Code, __func__, err, "Server Side Protocol Error (Opaque Structure Type)");
            return err;
        }

        UDA_LOG(UDA_LOG_DEBUG, "Hierarchical Data Structure sent to Client");
    }

    return 0;
}

int uda::protocol::XdrProtocol::recv_client_block(ClientBlock* client_block, bool* fatal,
                                                int server_tot_block_time, const int* server_timeout,
                                                LogMallocList* log_malloc_list,
                                                UserDefinedTypeList* user_defined_type_list)
{
    // Receive the Client Block, request block and putData block

    if (!xdrrec_skiprecord(&_server_input)) {
        *fatal = true;
        UDA_THROW_ERROR(_error_stack, (int)ProtocolError::Error5, "Protocol 5 Error (Client Block)");
    }

    ProtocolId protocol_id = ProtocolId::ClientBlock;

    int err = 0;
    if ((err = protocol2(_error_stack, &_server_input, protocol_id, XDRStreamDirection::Receive, nullptr, log_malloc_list, user_defined_type_list,
                         client_block, _protocol_version, &_log_struct_list, 0, _malloc_source)) != 0) {
        if (server_tot_block_time >= 1000 * *server_timeout) {
            *fatal = true;
            UDA_THROW_ERROR(_error_stack, 999, "Server Time Out");
        }

        UDA_LOG(UDA_LOG_DEBUG, "Problem Receiving Client Block");

        add_error(_error_stack, ErrorType::Code, __func__, err, "Protocol 10 Error (Receiving Client Block)");

        *fatal = true;
        return err;
    }

    return err;
}

void uda::protocol::XdrProtocol::set_version(int protocol_version)
{
    _protocol_version = protocol_version;
}

int uda::protocol::XdrProtocol::recv_request_block(RequestBlock* request_block, LogMallocList* log_malloc_list,
                                                 UserDefinedTypeList* user_defined_type_list)
{
    int err = 0;
    if ((err = protocol2(_error_stack, &_server_input, ProtocolId::RequestBlock, XDRStreamDirection::Receive, nullptr, log_malloc_list,
                         user_defined_type_list, request_block, _protocol_version, &_log_struct_list, _private_flags,
                         _malloc_source)) != 0) {
        UDA_THROW_ERROR(_error_stack, err, "Protocol 1 Error (Receiving Client Request)");
    }

    UDA_LOG(UDA_LOG_DEBUG, "Request Block Received");
    return err;
}

int uda::protocol::XdrProtocol::recv_putdata_block_list(client_server::PutDataBlockList* putdata_block_list,
                                                      LogMallocList* log_malloc_list,
                                                      UserDefinedTypeList* user_defined_type_list)
{
    int err = 0;
    if ((err = protocol2(_error_stack, &_server_input, ProtocolId::PutdataBlockList, XDRStreamDirection::Receive, nullptr, log_malloc_list,
                         user_defined_type_list, putdata_block_list, _protocol_version, &_log_struct_list,
                         _private_flags, _malloc_source)) != 0) {
        UDA_THROW_ERROR(_error_stack, err, "Protocol 1 Error (Receiving Client putDataBlockList)");
    }

    UDA_LOG(UDA_LOG_DEBUG, "putData Block List Received");
    UDA_LOG(UDA_LOG_DEBUG, "Number of PutData Blocks: {}", putdata_block_list->size());
    return err;
}

int uda::protocol::XdrProtocol::eof()
{
    // Flush (mark as at EOF) the input socket buffer: no more data should be read from this point

    xdrrec_eof(&_server_input);

    return 0;
}

DataBlock* uda::protocol::XdrProtocol::read_from_cache(const config::Config& config, cache::UdaCache* cache,
                                                     const RequestData* request, LogMallocList* log_malloc_list,
                                                     UserDefinedTypeList* user_defined_type_list)
{
    return cache_read(config, cache, request, log_malloc_list, user_defined_type_list, _protocol_version,
                      client_flags::Cache, &_log_struct_list, _private_flags, _malloc_source);
}

void uda::protocol::XdrProtocol::write_to_cache(const config::Config& config, cache::UdaCache* cache,
                                              const RequestData* request, DataBlock* data_block,
                                              LogMallocList* log_malloc_list,
                                              UserDefinedTypeList* user_defined_type_list)
{
    cache_write(config, cache, request, data_block, log_malloc_list, user_defined_type_list, 8,
                client_flags::Cache, &_log_struct_list, _private_flags, _malloc_source);
}
