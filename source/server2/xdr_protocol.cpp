#include "xdr_protocol.hpp"
#include "clientserver/errorLog.h"
#include "clientserver/printStructs.h"
#include "clientserver/protocol.h"
#include "clientserver/udaErrors.h"
#include "clientserver/xdrlib.h"
#include <uda/types.h>

#if defined(SSLAUTHENTICATION) && !defined(FATCLIENT)
#  include "authentication/udaServerSSL.h"
#endif

#include <cerrno>
#include <unistd.h>

#include "clientserver/udaDefines.h"
#include "logging/logging.h"

#include "server.hpp"

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

namespace uda::server {

int read(void* iohandle, char* buf, int count) {
    int rc = 0;
    fd_set rfds; // File Descriptor Set for Reading from the Socket
    timeval tv = {};
    timeval tvc = {};

    auto io_data = reinterpret_cast<uda::server::IoData*>(iohandle);

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

    auto io_data = reinterpret_cast<uda::server::IoData*>(iohandle);

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

void uda::server::XdrProtocol::create()
{
    _io_data.server_socket = 0;
    create_streams();
}

void uda::server::XdrProtocol::set_socket(int socket_fd)
{
    _io_data.server_socket = socket_fd;
}

uda::server::XdrProtocol::XdrProtocol()
    : _server_input{}
    , _server_output{}
    , _server_tot_block_time{0}
    , _server_timeout{TimeOut}
    , _io_data{}
{
    _io_data.server_socket = 0;
    _io_data.server_tot_block_time = &_server_tot_block_time;
    _io_data.server_timeout = &_server_timeout;
}

void uda::server::XdrProtocol::create_streams()
{
    _server_output.x_ops = nullptr;
    _server_input.x_ops = nullptr;

#if defined(SSLAUTHENTICATION)
    if (getUdaServerSSLDisabled()) {
#  if defined(__APPLE__) || defined(__TIRPC__)
        xdrrec_create(&_server_output, DBReadBlockSize, DBWriteBlockSize, &_io_data,
                      reinterpret_cast<int (*)(void*, void*, int)>(uda::server::read),
                      reinterpret_cast<int (*)(void*, void*, int)>(uda::server::write));

        xdrrec_create(&_server_input, DBReadBlockSize, DBWriteBlockSize, &_io_data,
                      reinterpret_cast<int (*)(void*, void*, int)>(uda::server::read),
                      reinterpret_cast<int (*)(void*, void*, int)>(uda::server::write));
#  else
        xdrrec_create(&_server_output, DBReadBlockSize, DBWriteBlockSize, (char*)&_io_data,
                      reinterpret_cast<int (*)(char*, char*, int)>(uda::server::read),
                      reinterpret_cast<int (*)(char*, char*, int)>(uda::server::write));

        xdrrec_create(&_server_input, DBReadBlockSize, DBWriteBlockSize, (char*)&_io_data,
                      reinterpret_cast<int (*)(char*, char*, int)>(uda::server::read),
                      reinterpret_cast<int (*)(char*, char*, int)>(uda::server::write));
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
                  reinterpret_cast<int (*)(void*, void*, int)>(uda::server::read),
                  reinterpret_cast<int (*)(void*, void*, int)>(uda::server::write));

    xdrrec_create(&_server_input, DBReadBlockSize, DBWriteBlockSize, &_io_data,
                  reinterpret_cast<int (*)(void*, void*, int)>(uda::server::read),
                  reinterpret_cast<int (*)(void*, void*, int)>(uda::server::write));
#  else
    xdrrec_create(&server_output_, DBReadBlockSize, DBWriteBlockSize, (char*)&_io_data,
                  reinterpret_cast<int (*)(char*, char*, int)>(uda::server::read),
                  reinterpret_cast<int (*)(char*, char*, int)>(uda::server::write));

    xdrrec_create(&_server_input, DBReadBlockSize, DBWriteBlockSize, (char*)&_io_data,
                  reinterpret_cast<int (*)(char*, char*, int)>(uda::server::read),
                  reinterpret_cast<int (*)(char*, char*, int)>(uda::server::write));
#  endif

#endif // SSLAUTHENTICATION

    _server_input.x_op = XDR_DECODE;
    _server_output.x_op = XDR_ENCODE;

    UDA_LOG(UDA_LOG_DEBUG, "XDR Streams Created");
}

int uda::server::XdrProtocol::read_client_block(ClientBlock* client_block, LogMallocList* log_malloc_list,
                                                UserDefinedTypeList* user_defined_type_list)
{
    int err = 0;

    UDA_LOG(UDA_LOG_DEBUG, "Receiving Client Block");

    if (!xdrrec_skiprecord(&_server_input)) {
        err = (int)ProtocolError::Error5;
        UDA_LOG(UDA_LOG_DEBUG, "xdrrec_skiprecord error!");
        add_error(ErrorType::Code, __func__, err, "Protocol 5 Error (Client Block)");
    } else {

        ProtocolId protocol_id = ProtocolId::ClientBlock; // Recieve Client Block

        if ((err = protocol2(&_server_input, protocol_id, XDRStreamDirection::Receive, nullptr, log_malloc_list, user_defined_type_list,
                             client_block, _protocol_version, &_log_struct_list, 0, _malloc_source)) != 0) {
            add_error(ErrorType::Code, __func__, err, "Protocol 10 Error (Client Block)");
            UDA_LOG(UDA_LOG_DEBUG, "protocol error! Client Block not received!");
        }

        if (err == 0) {
            UDA_LOG(UDA_LOG_DEBUG, "Client Block received");
            print_client_block(*client_block);
        }

        // Test for an immediate CLOSEDOWN instruction
    }

    return err;
}

int uda::server::XdrProtocol::send_server_block(ServerBlock server_block, LogMallocList* log_malloc_list,
                                                UserDefinedTypeList* user_defined_type_list)
{
    UDA_LOG(UDA_LOG_DEBUG, "Sending Server Block");
    print_server_block(server_block);

    ProtocolId protocol_id = ProtocolId::ServerBlock; // Receive Server Block: Server Aknowledgement

    int err;
    if ((err = protocol2(&_server_output, protocol_id, XDRStreamDirection::Send, nullptr, log_malloc_list, user_defined_type_list,
                         (void*)&server_block, _protocol_version, &_log_struct_list, 0, _malloc_source)) != 0) {
        UDA_THROW_ERROR(err, "Protocol 11 Error (Server Block #1)");
    }

    UDA_LOG(UDA_LOG_DEBUG, "Server Block sent without error");

    return 0;
}

int uda::server::XdrProtocol::flush()
{
    if (!xdrrec_endofrecord(&_server_output, 1)) { // Send data now
        UDA_THROW_ERROR((int)ProtocolError::Error7, "Protocol 7 Error (Server Block)");
    }

    return 0;
}

void uda::server::XdrProtocol::reset()
{
    _malloc_source = UDA_MALLOC_SOURCE_NONE;
    _log_struct_list = {};
}

int uda::server::XdrProtocol::send_meta_data(MetadataBlock& metadata_block, LogMallocList* log_malloc_list,
                                             UserDefinedTypeList* user_defined_type_list)
{
    //----------------------------------------------------------------------------
    // Send the Data System Structure

    ProtocolId protocol_id = ProtocolId::DataSystem;
    int err = 0;

    if ((err = protocol2(&_server_output, protocol_id, XDRStreamDirection::Send, nullptr, log_malloc_list, user_defined_type_list,
                         &metadata_block.data_system, _protocol_version, &_log_struct_list, 0, _malloc_source)) != 0) {
        UDA_LOG(UDA_LOG_DEBUG, "Problem Sending Data System Structure");
        add_error(ErrorType::Code, __func__, err, "Protocol 4 Error");
        return err;
    }

    //----------------------------------------------------------------------------
    // Send the System Configuration Structure

    protocol_id = ProtocolId::SystemConfig;

    if ((err = protocol2(&_server_output, protocol_id, XDRStreamDirection::Send, nullptr, log_malloc_list, user_defined_type_list,
                         &metadata_block.system_config, _protocol_version, &_log_struct_list, 0, _malloc_source)) !=
        0) {
        UDA_LOG(UDA_LOG_DEBUG, "Problem Sending System Configuration Structure");
        add_error(ErrorType::Code, __func__, err, "Protocol 5 Error");
        return err;
    }

    //----------------------------------------------------------------------------
    // Send the Data Source Structure

    protocol_id = ProtocolId::DataSource;

    if ((err = protocol2(&_server_output, protocol_id, XDRStreamDirection::Send, nullptr, log_malloc_list, user_defined_type_list,
                         &metadata_block.data_source, _protocol_version, &_log_struct_list, 0, _malloc_source)) != 0) {
        UDA_LOG(UDA_LOG_DEBUG, "Problem Sending Data Source Structure");
        add_error(ErrorType::Code, __func__, err, "Protocol 6 Error");
        return err;
    }

    //----------------------------------------------------------------------------
    // Send the Signal Structure

    protocol_id = ProtocolId::Signal;

    if ((err = protocol2(&_server_output, protocol_id, XDRStreamDirection::Send, nullptr, log_malloc_list, user_defined_type_list,
                         &metadata_block.signal_rec, _protocol_version, &_log_struct_list, 0, _malloc_source)) != 0) {
        UDA_LOG(UDA_LOG_DEBUG, "Problem Sending Signal Structure");
        add_error(ErrorType::Code, __func__, err, "Protocol 7 Error");
        return err;
    }

    //----------------------------------------------------------------------------
    // Send the Signal Description Structure

    protocol_id = ProtocolId::SignalDesc;

    if ((err = protocol2(&_server_output, protocol_id, XDRStreamDirection::Send, nullptr, log_malloc_list, user_defined_type_list,
                         &metadata_block.signal_desc, _protocol_version, &_log_struct_list, 0, _malloc_source)) != 0) {
        UDA_LOG(UDA_LOG_DEBUG, "Problem Sending Signal Description Structure");
        add_error(ErrorType::Code, __func__, err, "Protocol 8 Error");
        return err;
    }

    return err;
}

int uda::server::XdrProtocol::send_data_blocks(const std::vector<DataBlock>& data_blocks,
                                               LogMallocList* log_malloc_list,
                                               UserDefinedTypeList* user_defined_type_list)
{
    UDA_LOG(UDA_LOG_DEBUG, "Sending Data Block Structure to Client");

    DataBlockList data_block_list = {};
    data_block_list.count = static_cast<int>(data_blocks.size());
    data_block_list.data = const_cast<DataBlock*>(data_blocks.data());

    int err = 0;
    if ((err = protocol2(&_server_output, ProtocolId::DataBlockList, XDRStreamDirection::Send, nullptr, log_malloc_list,
                         user_defined_type_list, (void*)&data_block_list, _protocol_version, &_log_struct_list, 0,
                         _malloc_source)) != 0) {
        UDA_LOG(UDA_LOG_DEBUG, "Problem Sending Data Structure");
        add_error(ErrorType::Code, __func__, err, "Protocol 2 Error");
        return err;
    }

    UDA_LOG(UDA_LOG_DEBUG, "Data Block Sent to Client");

    return err;
}

int uda::server::XdrProtocol::send_hierachical_data(const DataBlock& data_block, LogMallocList* log_malloc_list,
                                                    UserDefinedTypeList* user_defined_type_list)
{
    if (data_block.data_type == UDA_TYPE_COMPOUND && data_block.opaque_type != UDA_OPAQUE_TYPE_UNKNOWN) {

        ProtocolId protocol_id = ProtocolId::Start;

        if (data_block.opaque_type == UDA_OPAQUE_TYPE_XML_DOCUMENT) {
            protocol_id = ProtocolId::Meta;
        } else if (data_block.opaque_type == UDA_OPAQUE_TYPE_STRUCTURES ||
            data_block.opaque_type == UDA_OPAQUE_TYPE_XDRFILE) {
            protocol_id = ProtocolId::Structures;
        }

        UDA_LOG(UDA_LOG_DEBUG, "Sending Hierarchical Data Structure to Client");

        int err = 0;
        if ((err = protocol2(&_server_output, protocol_id, XDRStreamDirection::Send, nullptr, log_malloc_list, user_defined_type_list,
                             (void*)&data_block, _protocol_version, &_log_struct_list, 0, _malloc_source)) != 0) {
            add_error(ErrorType::Code, __func__, err, "Server Side Protocol Error (Opaque Structure Type)");
            return err;
        }

        UDA_LOG(UDA_LOG_DEBUG, "Hierarchical Data Structure sent to Client");
    }

    return 0;
}

int uda::server::XdrProtocol::recv_client_block(ServerBlock& server_block, ClientBlock* client_block, bool* fatal,
                                                int server_tot_block_time, const int* server_timeout,
                                                LogMallocList* log_malloc_list,
                                                UserDefinedTypeList* user_defined_type_list)
{
    // Receive the Client Block, request block and putData block

    if (!xdrrec_skiprecord(&_server_input)) {
        *fatal = true;
        UDA_THROW_ERROR((int)ProtocolError::Error5, "Protocol 5 Error (Client Block)");
    }

    ProtocolId protocol_id = ProtocolId::ClientBlock;

    int err = 0;
    if ((err = protocol2(&_server_input, protocol_id, XDRStreamDirection::Receive, nullptr, log_malloc_list, user_defined_type_list,
                         client_block, _protocol_version, &_log_struct_list, 0, _malloc_source)) != 0) {
        if (server_tot_block_time >= 1000 * *server_timeout) {
            *fatal = true;
            UDA_THROW_ERROR(999, "Server Time Out");
        }

        UDA_LOG(UDA_LOG_DEBUG, "Problem Receiving Client Block");

        add_error(ErrorType::Code, __func__, err, "Protocol 10 Error (Receiving Client Block)");
        concat_error(&server_block.idamerrorstack); // Update Server State with Error Stack
        close_error();

        *fatal = true;
        return err;
    }

    return err;
}

void uda::server::XdrProtocol::set_version(int protocol_version)
{
    _protocol_version = protocol_version;
}

int uda::server::XdrProtocol::recv_request_block(RequestBlock* request_block, LogMallocList* log_malloc_list,
                                                 UserDefinedTypeList* user_defined_type_list)
{
    int err = 0;
    if ((err = protocol2(&_server_input, ProtocolId::RequestBlock, XDRStreamDirection::Receive, nullptr, log_malloc_list,
                         user_defined_type_list, request_block, _protocol_version, &_log_struct_list, _private_flags,
                         _malloc_source)) != 0) {
        UDA_THROW_ERROR(err, "Protocol 1 Error (Receiving Client Request)");
    }

    UDA_LOG(UDA_LOG_DEBUG, "Request Block Received");
    return err;
}

int uda::server::XdrProtocol::recv_putdata_block_list(client_server::PutDataBlockList* putdata_block_list,
                                                      LogMallocList* log_malloc_list,
                                                      UserDefinedTypeList* user_defined_type_list)
{
    int err = 0;
    if ((err = protocol2(&_server_input, ProtocolId::PutdataBlockList, XDRStreamDirection::Receive, nullptr, log_malloc_list,
                         user_defined_type_list, putdata_block_list, _protocol_version, &_log_struct_list,
                         _private_flags, _malloc_source)) != 0) {
        UDA_THROW_ERROR(err, "Protocol 1 Error (Receiving Client putDataBlockList)");
    }

    UDA_LOG(UDA_LOG_DEBUG, "putData Block List Received");
    UDA_LOG(UDA_LOG_DEBUG, "Number of PutData Blocks: {}", putdata_block_list->blockCount);
    return err;
}

int uda::server::XdrProtocol::eof()
{
    // Flush (mark as at EOF) the input socket buffer: no more data should be read from this point

    xdrrec_eof(&_server_input);

    return 0;
}

DataBlock* uda::server::XdrProtocol::read_from_cache(config::Config& config, cache::UdaCache* cache, RequestData* request,
                                                     LogMallocList* log_malloc_list,
                                                     UserDefinedTypeList* user_defined_type_list)
{
    return cache_read(config, cache, request, log_malloc_list, user_defined_type_list, _protocol_version,
                      client_flags::Cache, &_log_struct_list, _private_flags, _malloc_source);
}

void uda::server::XdrProtocol::write_to_cache(config::Config& config, cache::UdaCache* cache, RequestData* request,
                                              DataBlock* data_block,
                                              LogMallocList* log_malloc_list,
                                              UserDefinedTypeList* user_defined_type_list)
{
    cache_write(config, cache, request, data_block, log_malloc_list, user_defined_type_list, 8,
                client_flags::Cache, &_log_struct_list, _private_flags, _malloc_source);
}
