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
using namespace uda::authentication;

int serverSocket = 0;

int server_read(void* iohandle, char* buf, int count)
{
    int rc = 0;
    fd_set rfds; // File Descriptor Set for Reading from the Socket
    timeval tv = {};
    timeval tvc = {};

    auto io_data = reinterpret_cast<uda::server::IoData*>(iohandle);

    // Wait until there are data to be read from the socket

    set_select_params(serverSocket, &rfds, &tv, io_data->server_tot_block_time);
    tvc = tv;

    while (select(serverSocket + 1, &rfds, nullptr, nullptr, &tvc) <= 0) {
        *io_data->server_tot_block_time += (int)tv.tv_usec / 1000;
        if (*io_data->server_tot_block_time > 1000 * *io_data->server_timeout) {
            UDA_LOG(UDA_LOG_DEBUG, "Total Wait Time Exceeds Lifetime Limit = %d (ms)\n",
                    *io_data->server_timeout * 1000);
            return -1;
        }

        update_select_params(serverSocket, &rfds, &tv, *io_data->server_tot_block_time); // Keep trying ...
        tvc = tv;
    }

    // Read from it, checking for EINTR, as happens if called from IDL

    while (((rc = (int)read(serverSocket, buf, count)) == -1) && (errno == EINTR)) {}

    // As we have waited to be told that there is data to be read, if nothing
    // arrives, then there must be an error

    if (!rc) {
        rc = -1;
    }

    return rc;
}

int server_write(void* iohandle, char* buf, int count)
{

    // This routine is only called when there is something to write back to the Client

    int rc = 0;
    int BytesSent = 0;

    fd_set wfds; // File Descriptor Set for Writing to the Socket
    timeval tv = {};

    auto io_data = reinterpret_cast<uda::server::IoData*>(iohandle);

    // Block IO until the Socket is ready to write to Client

    set_select_params(serverSocket, &wfds, &tv, io_data->server_tot_block_time);

    while (select(serverSocket + 1, nullptr, &wfds, nullptr, &tv) <= 0) {
        *io_data->server_tot_block_time += tv.tv_usec / 1000;
        if (*io_data->server_tot_block_time / 1000 > *io_data->server_timeout) {
            UDA_LOG(UDA_LOG_DEBUG, "Total Blocking Time: %d (ms)\n", *io_data->server_tot_block_time);
            return -1;
        }
        update_select_params(serverSocket, &wfds, &tv, *io_data->server_tot_block_time);
    }

    // Write to socket, checking for EINTR, as happens if called from IDL

    while (BytesSent < count) {
        while (((rc = (int)write(serverSocket, buf, count)) == -1) && (errno == EINTR)) {}
        BytesSent += rc;
        buf += rc;
    }

    return rc;
}

void uda::server::XdrProtocol::create()
{
    create_streams();
}

uda::server::XdrProtocol::XdrProtocol()
    : _server_input{}, _server_output{}, _server_tot_block_time{0}, _server_timeout{TIMEOUT}, _io_data{}
{
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
        xdrrec_create(&_server_output, DB_READ_BLOCK_SIZE, DB_WRITE_BLOCK_SIZE, &_io_data,
                      reinterpret_cast<int (*)(void*, void*, int)>(server_read),
                      reinterpret_cast<int (*)(void*, void*, int)>(server_write));

        xdrrec_create(&_server_input, DB_READ_BLOCK_SIZE, DB_WRITE_BLOCK_SIZE, &_io_data,
                      reinterpret_cast<int (*)(void*, void*, int)>(server_read),
                      reinterpret_cast<int (*)(void*, void*, int)>(server_write));
#  else
        xdrrec_create(&server_output_, DB_READ_BLOCK_SIZE, DB_WRITE_BLOCK_SIZE, (char*)&io_data_,
                      reinterpret_cast<int (*)(char*, char*, int)>(server_read),
                      reinterpret_cast<int (*)(char*, char*, int)>(server_write));

        xdrrec_create(&server_input_, DB_READ_BLOCK_SIZE, DB_WRITE_BLOCK_SIZE, (char*)&io_data_,
                      reinterpret_cast<int (*)(char*, char*, int)>(server_read),
                      reinterpret_cast<int (*)(char*, char*, int)>(server_write));
#  endif
    } else {
#  if defined(__APPLE__) || defined(__TIRPC__)
        xdrrec_create(&_server_output, DB_READ_BLOCK_SIZE, DB_WRITE_BLOCK_SIZE, &_io_data,
                      reinterpret_cast<int (*)(void*, void*, int)>(readUdaServerSSL),
                      reinterpret_cast<int (*)(void*, void*, int)>(writeUdaServerSSL));

        xdrrec_create(&_server_input, DB_READ_BLOCK_SIZE, DB_WRITE_BLOCK_SIZE, &_io_data,
                      reinterpret_cast<int (*)(void*, void*, int)>(readUdaServerSSL),
                      reinterpret_cast<int (*)(void*, void*, int)>(writeUdaServerSSL));
#  else
        xdrrec_create(&server_output_, DB_READ_BLOCK_SIZE, DB_WRITE_BLOCK_SIZE, (char*)&io_data_,
                      reinterpret_cast<int (*)(char*, char*, int)>(readUdaServerSSL),
                      reinterpret_cast<int (*)(char*, char*, int)>(writeUdaServerSSL));

        xdrrec_create(&server_input_, DB_READ_BLOCK_SIZE, DB_WRITE_BLOCK_SIZE, (char*)&io_data_,
                      reinterpret_cast<int (*)(char*, char*, int)>(readUdaServerSSL),
                      reinterpret_cast<int (*)(char*, char*, int)>(writeUdaServerSSL));
#  endif
    }
#else // SSLAUTHENTICATION

#  if defined(__APPLE__) || defined(__TIRPC__)
    xdrrec_create(&server_output_, DB_READ_BLOCK_SIZE, DB_WRITE_BLOCK_SIZE, &io_data_,
                  reinterpret_cast<int (*)(void*, void*, int)>(server_read),
                  reinterpret_cast<int (*)(void*, void*, int)>(server_write));

    xdrrec_create(&server_input_, DB_READ_BLOCK_SIZE, DB_WRITE_BLOCK_SIZE, &io_data_,
                  reinterpret_cast<int (*)(void*, void*, int)>(server_read),
                  reinterpret_cast<int (*)(void*, void*, int)>(server_write));
#  else
    xdrrec_create(&server_output_, DB_READ_BLOCK_SIZE, DB_WRITE_BLOCK_SIZE, (char*)&io_data_,
                  reinterpret_cast<int (*)(char*, char*, int)>(server_read),
                  reinterpret_cast<int (*)(char*, char*, int)>(server_write));

    xdrrec_create(&server_input_, DB_READ_BLOCK_SIZE, DB_WRITE_BLOCK_SIZE, (char*)&io_data_,
                  reinterpret_cast<int (*)(char*, char*, int)>(server_read),
                  reinterpret_cast<int (*)(char*, char*, int)>(server_write));
#  endif

#endif // SSLAUTHENTICATION

    _server_input.x_op = XDR_DECODE;
    _server_output.x_op = XDR_ENCODE;

    UDA_LOG(UDA_LOG_DEBUG, "XDR Streams Created\n");
}

int uda::server::XdrProtocol::read_client_block(ClientBlock* client_block, LogMallocList* log_malloc_list,
                                                UserDefinedTypeList* user_defined_type_list)
{
    int err = 0;

    UDA_LOG(UDA_LOG_DEBUG, "Receiving Client Block\n");

    if (!xdrrec_skiprecord(&_server_input)) {
        err = UDA_PROTOCOL_ERROR_5;
        UDA_LOG(UDA_LOG_DEBUG, "xdrrec_skiprecord error!\n");
        add_error(UDA_CODE_ERROR_TYPE, __func__, err, "Protocol 5 Error (Client Block)");
    } else {

        int protocol_id = UDA_PROTOCOL_CLIENT_BLOCK; // Recieve Client Block

        if ((err = protocol2(&_server_input, protocol_id, XDR_RECEIVE, nullptr, log_malloc_list, user_defined_type_list,
                             client_block, _protocol_version, &_log_struct_list, 0, _malloc_source)) != 0) {
            add_error(UDA_CODE_ERROR_TYPE, __func__, err, "Protocol 10 Error (Client Block)");
            UDA_LOG(UDA_LOG_DEBUG, "protocol error! Client Block not received!\n");
        }

        if (err == 0) {
            UDA_LOG(UDA_LOG_DEBUG, "Client Block received\n");
            print_client_block(*client_block);
        }

        // Test for an immediate CLOSEDOWN instruction
    }

    return err;
}

int uda::server::XdrProtocol::send_server_block(ServerBlock server_block, LogMallocList* log_malloc_list,
                                                UserDefinedTypeList* user_defined_type_list)
{
    UDA_LOG(UDA_LOG_DEBUG, "Sending Server Block\n");
    print_server_block(server_block);

    int protocol_id = UDA_PROTOCOL_SERVER_BLOCK; // Receive Server Block: Server Aknowledgement

    int err;
    if ((err = protocol2(&_server_output, protocol_id, XDR_SEND, nullptr, log_malloc_list, user_defined_type_list,
                         (void*)&server_block, _protocol_version, &_log_struct_list, 0, _malloc_source)) != 0) {
        UDA_THROW_ERROR(err, "Protocol 11 Error (Server Block #1)");
    }

    UDA_LOG(UDA_LOG_DEBUG, "Server Block sent without error\n");

    return 0;
}

int uda::server::XdrProtocol::flush()
{
    if (!xdrrec_endofrecord(&_server_output, 1)) { // Send data now
        UDA_THROW_ERROR(UDA_PROTOCOL_ERROR_7, "Protocol 7 Error (Server Block)");
    }

    return 0;
}

int uda::server::XdrProtocol::send_meta_data(MetadataBlock& metadata_block, LogMallocList* log_malloc_list,
                                             UserDefinedTypeList* user_defined_type_list)
{
    //----------------------------------------------------------------------------
    // Send the Data System Structure

    int protocol_id = UDA_PROTOCOL_DATA_SYSTEM;
    int err = 0;

    if ((err = protocol2(&_server_output, protocol_id, XDR_SEND, nullptr, log_malloc_list, user_defined_type_list,
                         &metadata_block.data_system, _protocol_version, &_log_struct_list, 0, _malloc_source)) != 0) {
        UDA_LOG(UDA_LOG_DEBUG, "Problem Sending Data System Structure\n");
        add_error(UDA_CODE_ERROR_TYPE, __func__, err, "Protocol 4 Error");
        return err;
    }

    //----------------------------------------------------------------------------
    // Send the System Configuration Structure

    protocol_id = UDA_PROTOCOL_SYSTEM_CONFIG;

    if ((err = protocol2(&_server_output, protocol_id, XDR_SEND, nullptr, log_malloc_list, user_defined_type_list,
                         &metadata_block.system_config, _protocol_version, &_log_struct_list, 0, _malloc_source)) !=
        0) {
        UDA_LOG(UDA_LOG_DEBUG, "Problem Sending System Configuration Structure\n");
        add_error(UDA_CODE_ERROR_TYPE, __func__, err, "Protocol 5 Error");
        return err;
    }

    //----------------------------------------------------------------------------
    // Send the Data Source Structure

    protocol_id = UDA_PROTOCOL_DATA_SOURCE;

    if ((err = protocol2(&_server_output, protocol_id, XDR_SEND, nullptr, log_malloc_list, user_defined_type_list,
                         &metadata_block.data_source, _protocol_version, &_log_struct_list, 0, _malloc_source)) != 0) {
        UDA_LOG(UDA_LOG_DEBUG, "Problem Sending Data Source Structure\n");
        add_error(UDA_CODE_ERROR_TYPE, __func__, err, "Protocol 6 Error");
        return err;
    }

    //----------------------------------------------------------------------------
    // Send the Signal Structure

    protocol_id = UDA_PROTOCOL_SIGNAL;

    if ((err = protocol2(&_server_output, protocol_id, XDR_SEND, nullptr, log_malloc_list, user_defined_type_list,
                         &metadata_block.signal_rec, _protocol_version, &_log_struct_list, 0, _malloc_source)) != 0) {
        UDA_LOG(UDA_LOG_DEBUG, "Problem Sending Signal Structure\n");
        add_error(UDA_CODE_ERROR_TYPE, __func__, err, "Protocol 7 Error");
        return err;
    }

    //----------------------------------------------------------------------------
    // Send the Signal Description Structure

    protocol_id = UDA_PROTOCOL_SIGNAL_DESC;

    if ((err = protocol2(&_server_output, protocol_id, XDR_SEND, nullptr, log_malloc_list, user_defined_type_list,
                         &metadata_block.signal_desc, _protocol_version, &_log_struct_list, 0, _malloc_source)) != 0) {
        UDA_LOG(UDA_LOG_DEBUG, "Problem Sending Signal Description Structure\n");
        add_error(UDA_CODE_ERROR_TYPE, __func__, err, "Protocol 8 Error");
        return err;
    }

    return err;
}

int uda::server::XdrProtocol::send_data_blocks(const std::vector<DataBlock>& data_blocks,
                                               LogMallocList* log_malloc_list,
                                               UserDefinedTypeList* user_defined_type_list)
{
    UDA_LOG(UDA_LOG_DEBUG, "Sending Data Block Structure to Client\n");

    DataBlockList data_block_list = {};
    data_block_list.count = static_cast<int>(data_blocks.size());
    data_block_list.data = const_cast<DataBlock*>(data_blocks.data());

    int err = 0;
    if ((err = protocol2(&_server_output, UDA_PROTOCOL_DATA_BLOCK_LIST, XDR_SEND, nullptr, log_malloc_list,
                         user_defined_type_list, (void*)&data_block_list, _protocol_version, &_log_struct_list, 0,
                         _malloc_source)) != 0) {
        UDA_LOG(UDA_LOG_DEBUG, "Problem Sending Data Structure\n");
        add_error(UDA_CODE_ERROR_TYPE, __func__, err, "Protocol 2 Error");
        return err;
    }

    UDA_LOG(UDA_LOG_DEBUG, "Data Block Sent to Client\n");

    return err;
}

int uda::server::XdrProtocol::send_hierachical_data(const DataBlock& data_block, LogMallocList* log_malloc_list,
                                                    UserDefinedTypeList* user_defined_type_list)
{
    if (_protocol_version < 9 && data_block.data_type == UDA_TYPE_COMPOUND &&
        data_block.opaque_type != UDA_OPAQUE_TYPE_UNKNOWN) {

        int protocol_id;

        if (data_block.opaque_type == UDA_OPAQUE_TYPE_XML_DOCUMENT) {
            protocol_id = UDA_PROTOCOL_META;
        } else {
            if (data_block.opaque_type == UDA_OPAQUE_TYPE_STRUCTURES ||
                data_block.opaque_type == UDA_OPAQUE_TYPE_XDRFILE) {
                protocol_id = UDA_PROTOCOL_STRUCTURES;
            } else {
                protocol_id = UDA_PROTOCOL_EFIT;
            }
        }

        UDA_LOG(UDA_LOG_DEBUG, "Sending Hierarchical Data Structure to Client\n");

        int err = 0;
        if ((err = protocol2(&_server_output, protocol_id, XDR_SEND, nullptr, log_malloc_list, user_defined_type_list,
                             (void*)&data_block, _protocol_version, &_log_struct_list, 0, _malloc_source)) != 0) {
            add_error(UDA_CODE_ERROR_TYPE, __func__, err, "Server Side Protocol Error (Opaque Structure Type)");
            return err;
        }

        UDA_LOG(UDA_LOG_DEBUG, "Hierarchical Data Structure sent to Client\n");
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
        UDA_THROW_ERROR(UDA_PROTOCOL_ERROR_5, "Protocol 5 Error (Client Block)");
    }

    int protocol_id = UDA_PROTOCOL_CLIENT_BLOCK;

    int err = 0;
    if ((err = protocol2(&_server_input, protocol_id, XDR_RECEIVE, nullptr, log_malloc_list, user_defined_type_list,
                         client_block, _protocol_version, &_log_struct_list, 0, _malloc_source)) != 0) {
        if (server_tot_block_time >= 1000 * *server_timeout) {
            *fatal = true;
            UDA_THROW_ERROR(999, "Server Time Out");
        }

        UDA_LOG(UDA_LOG_DEBUG, "Problem Receiving Client Block\n");

        add_error(UDA_CODE_ERROR_TYPE, __func__, err, "Protocol 10 Error (Receiving Client Block)");
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
    if ((err = protocol2(&_server_input, UDA_PROTOCOL_REQUEST_BLOCK, XDR_RECEIVE, nullptr, log_malloc_list,
                         user_defined_type_list, request_block, _protocol_version, &_log_struct_list, _private_flags,
                         _malloc_source)) != 0) {
        UDA_THROW_ERROR(err, "Protocol 1 Error (Receiving Client Request)");
    }

    UDA_LOG(UDA_LOG_DEBUG, "Request Block Received\n");
    return err;
}

int uda::server::XdrProtocol::recv_putdata_block_list(uda::client_server::PutDataBlockList* putdata_block_list,
                                                      LogMallocList* log_malloc_list,
                                                      UserDefinedTypeList* user_defined_type_list)
{
    int err = 0;
    if ((err = protocol2(&_server_input, UDA_PROTOCOL_PUTDATA_BLOCK_LIST, XDR_RECEIVE, nullptr, log_malloc_list,
                         user_defined_type_list, putdata_block_list, _protocol_version, &_log_struct_list,
                         _private_flags, _malloc_source)) != 0) {
        UDA_THROW_ERROR(err, "Protocol 1 Error (Receiving Client putDataBlockList)");
    }

    UDA_LOG(UDA_LOG_DEBUG, "putData Block List Received\n");
    UDA_LOG(UDA_LOG_DEBUG, "Number of PutData Blocks: %d\n", putdata_block_list->blockCount);
    return err;
}

int uda::server::XdrProtocol::eof()
{
    // Flush (mark as at EOF) the input socket buffer: no more data should be read from this point

    xdrrec_eof(&_server_input);

    return 0;
}

DataBlock* uda::server::XdrProtocol::read_from_cache(uda::cache::UdaCache* cache, RequestData* request,
                                                     server::Environment& environment, LogMallocList* log_malloc_list,
                                                     UserDefinedTypeList* user_defined_type_list)
{
    return cache_read(cache, request, log_malloc_list, user_defined_type_list, *environment.p_env(), _protocol_version,
                      CLIENTFLAG_CACHE, &_log_struct_list, _private_flags, _malloc_source);
}

void uda::server::XdrProtocol::write_to_cache(uda::cache::UdaCache* cache, RequestData* request,
                                              server::Environment& environment, DataBlock* data_block,
                                              LogMallocList* log_malloc_list,
                                              UserDefinedTypeList* user_defined_type_list)
{
    cache_write(cache, request, data_block, log_malloc_list, user_defined_type_list, *environment.p_env(), 8,
                CLIENTFLAG_CACHE, &_log_struct_list, _private_flags, _malloc_source);
}
