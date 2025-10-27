#include "client.hpp"

#include "client_environment.hpp"
#include "make_request_block.hpp"
#include "error_codes.h"
#include "client_xdr_stream.hpp"
#include "accAPI.h"
#include "exceptions.hpp"

#ifdef _WIN32
#  include "windows_defines.hpp"
#endif

#include <clientserver/udaDefines.h>
#include <logging/logging.h>
#include <clientserver/errorLog.h>
#include <clientserver/initStructs.h>
#include <clientserver/printStructs.h>
#include <clientserver/userid.h>
#include <clientserver/protocol.h>
#include <clientserver/udaErrors.h>
#include <clientserver/xdrlib.h>
#include <cache/fileCache.h>
#include <clientserver/udaTypes.h>
#include <clientserver/stringUtils.h>
#include <clientserver/allocData.h>

namespace {

void copy_data_block(DATA_BLOCK* str, DATA_BLOCK* in)
{
    *str = *in;
    memcpy(str->errparams, in->errparams, MAXERRPARAMS);
    memcpy(str->data_units, in->data_units, STRING_LENGTH);
    memcpy(str->data_label, in->data_label, STRING_LENGTH);
    memcpy(str->data_desc, in->data_desc, STRING_LENGTH);
    memcpy(str->error_msg, in->error_msg, STRING_LENGTH);
    initClientBlock(&str->client_block, 0, "");
}

void copy_client_block(CLIENT_BLOCK* str, const ClientFlags* client_flags)
{
    str->timeout = client_flags->user_timeout;
    str->clientFlags = client_flags->flags;
    str->altRank = client_flags->alt_rank;
    str->get_datadble = client_flags->get_datadble;
    str->get_dimdble = client_flags->get_dimdble;
    str->get_timedble = client_flags->get_timedble;
    str->get_scalar = client_flags->get_scalar;
    str->get_bytes = client_flags->get_bytes;
    str->get_bad = client_flags->get_bad;
    str->get_meta = client_flags->get_meta;
    str->get_asis = client_flags->get_asis;
    str->get_uncal = client_flags->get_uncal;
    str->get_notoff = client_flags->get_notoff;
    str->get_nodimdata = client_flags->get_nodimdata;
}

int alloc_meta(DATA_SYSTEM** data_system, SYSTEM_CONFIG** system_config, DATA_SOURCE** data_source,
               SIGNAL** signal_rec, SIGNAL_DESC** signal_desc)
{
    int err = 0;

    // Allocate memory for the Meta Data
    *data_system = (DATA_SYSTEM*)malloc(sizeof(DATA_SYSTEM));
    *system_config = (SYSTEM_CONFIG*)malloc(sizeof(SYSTEM_CONFIG));
    *data_source = (DATA_SOURCE*)malloc(sizeof(DATA_SOURCE));
    *signal_rec = (SIGNAL*)malloc(sizeof(SIGNAL));
    *signal_desc = (SIGNAL_DESC*)malloc(sizeof(SIGNAL_DESC));

    if (*data_system == nullptr || *system_config == nullptr || *data_source == nullptr ||
            *signal_rec == nullptr || *signal_desc == nullptr) {
        err = ERROR_ALLOCATING_META_DATA_HEAP;
        addIdamError(UDA_CODE_ERROR_TYPE, __func__, err, "Error Allocating Heap for Meta Data");
        return err;
    }

    return err;
}

void update_client_block(CLIENT_BLOCK& client_block, const ClientFlags& client_flags, unsigned int private_flags)
{
    client_block.timeout = client_flags.user_timeout;
    client_block.clientFlags = client_flags.flags;
    client_block.altRank = client_flags.alt_rank;
    client_block.get_datadble = client_flags.get_datadble;
    client_block.get_dimdble = client_flags.get_dimdble;
    client_block.get_timedble = client_flags.get_timedble;
    client_block.get_scalar = client_flags.get_scalar;
    client_block.get_bytes = client_flags.get_bytes;
    client_block.get_bad = client_flags.get_bad;
    client_block.get_meta = client_flags.get_meta;
    client_block.get_asis = client_flags.get_asis;
    client_block.get_uncal = client_flags.get_uncal;
    client_block.get_notoff = client_flags.get_notoff;
    client_block.get_nodimdata = client_flags.get_nodimdata;
    client_block.privateFlags = private_flags;

    // Operating System Name

    char* env;
    if ((env = getenv("OSTYPE")) != nullptr) {
        strcpy(client_block.OSName, env);
    }

    // Client's study DOI

    if ((env = getenv("UDA_CLIENT_DOI")) != nullptr) {
        strcpy(client_block.DOI, env);
    }
}

} // anon namespace

uda::client::Client::Client()
    : connection_{environment_}
    , protocol_version_{ClientVersion}
{
    host_ = DefaultHost;
    port_ = DefaultPort;
    flags_ = 0;

    client_flags_ = {};
    client_flags_.alt_rank = 0;
    client_flags_.user_timeout = TIMEOUT;

    const char* timeout = getenv("UDA_TIMEOUT");
    if (timeout != nullptr) {
        client_flags_.user_timeout = (int)strtol(getenv("UDA_TIMEOUT"), nullptr, 10);
    }

    initUdaErrorStack();

    environment_ = load_environment(&env_host_, &env_port_);
    print_client_environment(environment_);

    //----------------------------------------------------------------
    // Client set Property Flags (can be changed via property accessor functions)
    // Coded user properties changes have priority

    if (environment_.clientFlags != 0) {
        flags_ |= environment_.clientFlags;
    }

    if (environment_.altRank != 0) {
        alt_rank_ = environment_.altRank;
    }

    //----------------------------------------------------------------
    // X.509 Security Certification

    //if((rc = readIdamSecurityCert(environment->security_cert)) != 0){
    //   if(verbose) fprintf(stderr, "Idam: Problem Locating the Security Certificate [%d]\n",  rc);
    //   return(-1);
    //}

    cache_ = uda::cache::open_cache();

    char username[STRING_LENGTH];
    userid(username);
    client_username_ = username;

    initClientBlock(&client_block_, ClientVersion, client_username_.c_str());

    //----------------------------------------------------------------
    // Check if Output Requested

    udaSetLogLevel((LOG_LEVEL)environment_.loglevel);

    if (environment_.loglevel == UDA_LOG_NONE) {
        return;
    }

    //---------------------------------------------------------------
    // Open the Log File

    errno = 0;

    std::string file_name = environment_.logdir;
    file_name += "Debug.dbg";

    FILE* file = fopen(file_name.c_str(), environment_.logmode);
    udaSetLogFile(UDA_LOG_WARN, file);
    udaSetLogFile(UDA_LOG_DEBUG, file);
    udaSetLogFile(UDA_LOG_INFO, file);

    if (errno != 0) {
        addIdamError(UDA_SYSTEM_ERROR_TYPE, __func__, errno, "failed to open debug log");
        udaCloseLogging();
        return;
    }

    if (udaGetLogLevel() <= UDA_LOG_ERROR) {
        file_name = environment_.logdir;
        file_name += "Error.err";

        file = fopen(file_name.c_str(), environment_.logmode);
        udaSetLogFile(UDA_LOG_ERROR, file);
    }

    if (errno != 0) {
        addIdamError(UDA_SYSTEM_ERROR_TYPE, __func__, errno, "failed to open error log");
        udaCloseLogging();
        return;
    }
}

int
uda::client::Client::fetch_meta()
{
    int err = 0;

#ifndef FATCLIENT   // <========================== Client Server Code Only
    DATA_SYSTEM* data_system = &metadata_.data_system;
    SYSTEM_CONFIG* system_config = &metadata_.system_config;
    DATA_SOURCE* data_source = &metadata_.data_source;
    SIGNAL* signal_rec = &metadata_.signal_rec;
    SIGNAL_DESC* signal_desc = &metadata_.signal_desc;

    if ((err = protocol2(client_input_, UDA_PROTOCOL_DATA_SYSTEM, XDR_RECEIVE, nullptr, logmalloclist_,
                         userdefinedtypelist_,
                         data_system, protocol_version_, &log_struct_list_, private_flags_, malloc_source_)) != 0) {
        addIdamError(UDA_CODE_ERROR_TYPE, __func__, err, "Protocol 4 Error (Data System)");
        return err;
    }
    printDataSystem(*data_system);

    if ((err = protocol2(client_input_, UDA_PROTOCOL_SYSTEM_CONFIG, XDR_RECEIVE, nullptr, logmalloclist_,
                         userdefinedtypelist_,
                         system_config, protocol_version_, &log_struct_list_, private_flags_, malloc_source_)) != 0) {
        addIdamError(UDA_CODE_ERROR_TYPE, __func__, err, "Protocol 5 Error (System Config)");
        return err;
    }
    printSystemConfig(*system_config);

    if ((err = protocol2(client_input_, UDA_PROTOCOL_DATA_SOURCE, XDR_RECEIVE, nullptr, logmalloclist_,
                         userdefinedtypelist_,
                         data_source, protocol_version_, &log_struct_list_, private_flags_, malloc_source_)) != 0) {
        addIdamError(UDA_CODE_ERROR_TYPE, __func__, err, "Protocol 6 Error (Data Source)");
        return err;
    }
    printDataSource(*data_source);

    if ((err = protocol2(client_input_, UDA_PROTOCOL_SIGNAL, XDR_RECEIVE, nullptr, logmalloclist_, userdefinedtypelist_,
                         signal_rec, protocol_version_, &log_struct_list_, private_flags_, malloc_source_)) != 0) {
        addIdamError(UDA_CODE_ERROR_TYPE, __func__, err, "Protocol 7 Error (Signal)");
        return err;
    }
    printSignal(*signal_rec);

    if ((err = protocol2(client_input_, UDA_PROTOCOL_SIGNAL_DESC, XDR_RECEIVE, nullptr, logmalloclist_,
                         userdefinedtypelist_,
                         signal_desc, protocol_version_, &log_struct_list_, private_flags_, malloc_source_)) != 0) {
        addIdamError(UDA_CODE_ERROR_TYPE, __func__, err, "Protocol 8 Error (Signal Desc)");
        return err;
    }
    printSignalDesc(*signal_desc);
#endif

    return err;
}

int
uda::client::Client::fetch_hierarchical_data(DATA_BLOCK* data_block)
{
    if (data_block->data_type == UDA_TYPE_COMPOUND &&
        data_block->opaque_type != UDA_OPAQUE_TYPE_UNKNOWN) {

        int protocol_id;
        if (data_block->opaque_type == UDA_OPAQUE_TYPE_XML_DOCUMENT) {
            protocol_id = UDA_PROTOCOL_META;
        } else if (data_block->opaque_type == UDA_OPAQUE_TYPE_STRUCTURES ||
                   data_block->opaque_type == UDA_OPAQUE_TYPE_XDRFILE ||
                   data_block->opaque_type == UDA_OPAQUE_TYPE_XDROBJECT) {
            protocol_id = UDA_PROTOCOL_STRUCTURES;
        } else {
            protocol_id = UDA_PROTOCOL_EFIT;
        }

        UDA_LOG(UDA_LOG_DEBUG, "Receiving Hierarchical Data Structure from Server\n");

        int err = 0;
        if ((err = protocol2(client_input_, protocol_id, XDR_RECEIVE, nullptr, logmalloclist_, userdefinedtypelist_,
                             data_block, protocol_version_, &log_struct_list_, private_flags_, malloc_source_)) != 0) {
            addIdamError(UDA_CODE_ERROR_TYPE, __func__, err, "Client Side Protocol Error (Opaque Structure Type)");
            return err;
        }
    }

    return 0;
}

const char* uda::client::Client::get_server_error_stack_record_msg(int record)
{
    UDA_LOG(UDA_LOG_DEBUG, "record %d\n", record);
    UDA_LOG(UDA_LOG_DEBUG, "count  %d\n", server_block_.idamerrorstack.nerrors);
    if (record < 0 || (unsigned int)record >= server_block_.idamerrorstack.nerrors) {
        return nullptr;
    }
    return server_block_.idamerrorstack.idamerror[record].msg;   // Server Error Stack Record Message
}

int uda::client::Client::get_server_error_stack_record_code(int record)
{
    if (record < 0 || (unsigned int)record >= server_block_.idamerrorstack.nerrors) {
        return 0;
    }
    return server_block_.idamerrorstack.idamerror[record].code;  // Server Error Stack Record Code
}

int uda::client::Client::get_requests(RequestBlock& request_block, int* indices)
{
    initServerBlock(&server_block_, 0);
    initUdaErrorStack();

    time_t tv_server_start = 0;
    time_t tv_server_end = 0;

    if (environment_.server_reconnect || environment_.server_change_socket) {
        int err = connection_.reconnect(&client_input_, &client_output_, &tv_server_start, &client_flags_.user_timeout);
        if (err) {
            return err;
        }
    }

    time(&tv_server_end);
    long age = (long)tv_server_end - (long)tv_server_start;

    UDA_LOG(UDA_LOG_DEBUG, "Start: %ld    End: %ld\n", (long)tv_server_start, (long)tv_server_end);
    UDA_LOG(UDA_LOG_DEBUG, "Server Age: %ld\n", age);

    bool init_server = true;
    if (age >= client_flags_.user_timeout - 2) {
        // Assume the Server has Self-Destructed so Instantiate a New Server
        UDA_LOG(UDA_LOG_DEBUG, "Server Age Limit Reached %ld\n", (long)age);
        UDA_LOG(UDA_LOG_DEBUG, "Server Closed and New Instance Started\n");

        // Close the Existing Socket and XDR Stream: Reopening will Instance a New Server
        closedown(ClosedownType::CLOSE_SOCKETS, &connection_, client_input_, client_output_, &reopen_logs_, &env_host_,
                  &env_port_);
    } else if (connection_.open()) {
        // Assume the Server is Still Alive
        if (client_output_->x_ops == nullptr || client_input_->x_ops == nullptr) {
            addIdamError(UDA_CODE_ERROR_TYPE, __func__, 999, "XDR Streams are Closed!");
            UDA_LOG(UDA_LOG_DEBUG, "XDR Streams are Closed!\n");
            closedown(ClosedownType::CLOSE_SOCKETS, &connection_, client_input_, client_output_, &reopen_logs_, &env_host_,
                      &env_port_);
        } else {
            init_server = false;
            xdrrec_eof(client_input_); // Flush input socket
        }
    }

    //bool authentication_needed = false;
    bool startup_states = false;
    if (init_server) {
        //authentication_needed = true;
        startup_states = true;
        if (connection_.create(client_input_, client_output_, host_list_) != 0) {
            int err = NO_SOCKET_CONNECTION;
            addIdamError(UDA_CODE_ERROR_TYPE, __func__, err, "No Socket Connection to Server");
            return err;
        }

        io_data_ = connection_.io_data();
        std::tie(client_input_, client_output_) = createXDRStream(&io_data_);
        time(&tv_server_start);        // Start the Clock again: Age of Server
    }

    char* env = nullptr;

    if ((env = getenv("UDA_PRIVATEFLAGS")) != nullptr) {
        private_flags_ |= atoi(env);
    }

    update_client_block(client_block_, client_flags_, private_flags_);
    printClientBlock(client_block_);

    //-------------------------------------------------------------------------
    // Client and Server States at Startup only (1 RTT)
    // Will be passed during mutual authentication step

    if (startup_states) {
        perform_handshake();
        startup_states = false;
    } // startup_states

    UDA_LOG(UDA_LOG_DEBUG, "Protocol Version %d\n", protocol_version_);
    UDA_LOG(UDA_LOG_DEBUG, "Client Version   %d\n", client_block_.version);
    UDA_LOG(UDA_LOG_DEBUG, "Server Version   %d\n", server_block_.version);

    int err = test_connection();

    err = send_client_block();
    err = send_request_block(request_block);
    err = send_putdata(request_block);

    err = flush_sockets();

    err = receive_server_block();

    //bool server_side = false;

    if (server_block_.idamerrorstack.nerrors > 0) {
        UDA_LOG(UDA_LOG_DEBUG, "Server Block passed Server Error State %d\n", err);
        err = server_block_.idamerrorstack.idamerror[0].code;      // Problem on the Server Side!
        UDA_LOG(UDA_LOG_DEBUG, "Server Block passed Server Error State %d\n", err);
        //server_side = true;        // Most Server Side errors are benign so don't close the server
        return 0;
    }

    if (client_block_.get_meta) {
        if ((err = fetch_meta()) != 0) {
            return err;
        }
    }

    //------------------------------------------------------------------------------
    // Fetch the data Block

    DATA_BLOCK_LIST recv_data_block_list;

    if ((err = protocol2(client_input_, UDA_PROTOCOL_DATA_BLOCK_LIST, XDR_RECEIVE, nullptr, logmalloclist_,
                         userdefinedtypelist_, &recv_data_block_list, protocol_version_,
                         &log_struct_list_, private_flags_, malloc_source_)) != 0) {
        UDA_LOG(UDA_LOG_DEBUG, "Protocol 2 Error (Failure Receiving Data Block)\n");

        addIdamError(UDA_CODE_ERROR_TYPE, __func__, err, "Protocol 2 Error (Failure Receiving Data Block)");
        throw uda::exceptions::ClientError("Protocol 2 Error (Failure Receiving Data Block)");
    }

    printDataBlockList(recv_data_block_list);

    bool data_received = false;
    std::vector<int> data_block_indices(request_block.num_requests);

    for (int i = 0; i < request_block.num_requests; ++i) {
        data_blocks_.emplace_back(DataBlock{});
        auto data_block_idx = data_blocks_.size() - 1;
        DataBlock* data_block = &data_blocks_.back();

        copy_data_block(data_block, &recv_data_block_list.data[i]);
        copy_client_block(&data_block->client_block, &client_flags_);

        if (client_block_.get_meta) {
            data_block->data_system = (DATA_SYSTEM*)malloc(sizeof(DATA_SYSTEM));
            data_block->system_config = (SYSTEM_CONFIG*)malloc(sizeof(SYSTEM_CONFIG));
            data_block->data_source = (DATA_SOURCE*)malloc(sizeof(DATA_SOURCE));
            data_block->signal_rec = (SIGNAL*)malloc(sizeof(SIGNAL));
            data_block->signal_desc = (SIGNAL_DESC*)malloc(sizeof(SIGNAL_DESC));
            if ((err = alloc_meta(
                    &data_block->data_system,
                    &data_block->system_config,
                    &data_block->data_source,
                    &data_block->signal_rec,
                    &data_block->signal_desc)) != 0) {
                break;
            }
            *data_block->data_system    = metadata_.data_system;
            *data_block->system_config  = metadata_.system_config;
            *data_block->data_source    = metadata_.data_source;
            *data_block->signal_rec     = metadata_.signal_rec;
            *data_block->signal_desc    = metadata_.signal_desc;
        }

        fetch_hierarchical_data(data_block);

        //------------------------------------------------------------------------------
        // Cache the data if the server has passed permission and the application (client) has enabled caching
        if (client_flags_.flags & CLIENTFLAG_FILECACHE) {
            udaFileCacheWrite(data_block, &request_block, logmalloclist_, userdefinedtypelist_, protocol_version_,
                              &log_struct_list_, private_flags_, malloc_source_);
        }

        if (cache_ != nullptr && client_flags_.flags & CLIENTFLAG_CACHE) {
            cache_write(cache_, &request_block.requests[i], data_block, logmalloclist_, userdefinedtypelist_,
                        environment_, protocol_version_, client_flags_.flags, &log_struct_list_,
                        private_flags_, malloc_source_);
        }

        data_block_indices[i] = data_block_idx;
        data_received = true;
    }

    free(recv_data_block_list.data);

    if (data_received) {
        if (err != 0) {
            // Close Socket & XDR Streams but Not Files
            closedown(ClosedownType::CLOSE_SOCKETS, nullptr, client_input_, client_output_, &reopen_logs_, &env_host_,
                      &env_port_);
        }

        for (auto data_block_idx : data_block_indices) {
            DATA_BLOCK* data_block = &data_blocks_[data_block_idx];

            if (err == 0 && (udaGetDataStatus(data_block_idx)) == MIN_STATUS && !client_flags_.get_bad) {
                // If Data are not usable, flag the client
                addIdamError(UDA_CODE_ERROR_TYPE, __func__, DATA_STATUS_BAD, "Data Status is BAD ... Data are Not Usable!");

                if (data_block->errcode == 0) {
                    // Don't over-rule a server side error
                    data_block->errcode = DATA_STATUS_BAD;
                    strcpy(data_block->error_msg, "Data Status is BAD ... Data are Not Usable!");
                }
            }
        }

        //------------------------------------------------------------------------------
        // Concatenate Error Message Stacks & Write to the Error Log

        concat_errors(&server_block_.idamerrorstack);
        closeUdaError();
        udaErrorLog(client_block_, request_block, &server_block_.idamerrorstack);

        //------------------------------------------------------------------------------
        // Copy Most Significant Error Stack Message to the Data Block if a Handle was Issued

        for (auto data_block_idx : data_block_indices) {
            DATA_BLOCK* data_block = &data_blocks_[data_block_idx];

            if (data_block->errcode == 0 && server_block_.idamerrorstack.nerrors > 0) {
                data_block->errcode = get_server_error_stack_record_code(0);
                strcpy(data_block->error_msg, get_server_error_stack_record_msg(0));
            }
        }

        //------------------------------------------------------------------------------
        // Normal Exit: Return to Client

        std::copy(data_block_indices.begin(), data_block_indices.end(), indices);
        return 0;

        //------------------------------------------------------------------------------
        // Abnormal Exit: Return to Client

    } else {
        UDA_LOG(UDA_LOG_DEBUG, "Returning Error %d\n", err);

        if (err != 0) {
            closedown(ClosedownType::CLOSE_SOCKETS, nullptr, client_input_, client_output_, &reopen_logs_, &env_host_,
                      &env_port_);
        }

        concat_errors(&server_block_.idamerrorstack);
        closeUdaError();
        udaErrorLog(client_block_, request_block, &server_block_.idamerrorstack);

        if (err == 0) {
            return ERROR_CONDITION_UNKNOWN;
        }

        return -abs(err);                       // Abnormal Exit
    }

    return 0;
}

int uda::client::Client::send_putdata(const RequestBlock& request_block)
{
    for (int i = 0; i < request_block.num_requests; ++i) {
        REQUEST_DATA* request = &request_block.requests[i];

        if (request->put) {
            int protocol_id = UDA_PROTOCOL_PUTDATA_BLOCK_LIST;
            int err = 0;
            if ((err = protocol2(client_output_, protocol_id, XDR_SEND, nullptr, logmalloclist_, userdefinedtypelist_,
                                 &(request->putDataBlockList), protocol_version_, &log_struct_list_,
                                 private_flags_, malloc_source_)) != 0) {
                addIdamError(UDA_CODE_ERROR_TYPE, __func__, err,
                             "Protocol 1 Error (sending putDataBlockList from Request Block)");
                throw uda::exceptions::ClientError("Protocol 1 Error (sending putDataBlockList from Request Block)");
            }
        }
    }

    return 0;
}

int uda::client::Client::get(std::string_view data_signal, std::string_view data_source)
{
    REQUEST_BLOCK request_block;
    initRequestBlock(&request_block);

    auto signal_ptr = data_signal.data();
    auto source_ptr = data_source.data();

    if (make_request_block(&environment_, &signal_ptr, &source_ptr, 1, &request_block) != 0) {
        if (udaNumErrors() == 0) {
            UDA_LOG(UDA_LOG_ERROR, "Error identifying the Data Source [%s]\n", data_source.data());
            addIdamError(UDA_CODE_ERROR_TYPE, __func__, 999, "Error identifying the Data Source");
        }
        throw uda::exceptions::ClientError("Error identifying the Data Source [%1%]", data_source);
    }

    printRequestBlock(request_block);

    std::vector<int> indices(request_block.num_requests);
    get_requests(request_block, indices.data());

    return indices[0];
}

std::vector<int> uda::client::Client::get(std::vector<std::pair<std::string, std::string>>& requests)
{
    REQUEST_BLOCK request_block;
    initRequestBlock(&request_block);

    std::vector<const char*> signals;
    std::transform(requests.begin(), requests.end(), std::back_inserter(signals),
                   [](const std::pair<std::string, std::string>& p){ return p.first.c_str(); });

    std::vector<const char*> sources;
    std::transform(requests.begin(), requests.end(), std::back_inserter(sources),
                   [](const std::pair<std::string, std::string>& p){ return p.second.c_str(); });

    if (make_request_block(&environment_, signals.data(), sources.data(), requests.size(), &request_block) != 0) {
        if (udaNumErrors() == 0) {
            UDA_LOG(UDA_LOG_ERROR, "Error identifying the Data Source\n");
            addIdamError(UDA_CODE_ERROR_TYPE, __func__, 999, "Error identifying the Data Source");
        }
        throw uda::exceptions::ClientError("Error identifying the Data Source");
    }

    printRequestBlock(request_block);

    std::vector<int> indices(request_block.num_requests);
    get_requests(request_block, indices.data());

    return indices;
}

void uda::client::Client::set_host(std::string_view host)
{
    host_ = host;
}

void uda::client::Client::set_port(int port)
{
    port_ = port;
}

int uda::client::Client::test_connection()
{
    int rc = 0;
    if (!(rc = xdrrec_eof(client_input_))) { // Test for an EOF

        UDA_LOG(UDA_LOG_DEBUG, "xdrrec_eof rc = %d => more input when none expected!\n", rc);

        int count = 0;
        char temp;

        do {
            rc = xdr_char(client_input_, &temp);                  // Flush the input (limit to 64 bytes)

            if (rc) {
                UDA_LOG(UDA_LOG_DEBUG, "[%d] [%c]\n", count++, temp);
            }
        } while (rc && count < 64);

        if (count > 0) {   // Error if data is waiting
            addIdamError(UDA_CODE_ERROR_TYPE, __func__, 999,
                         "Data waiting in the input data buffer when none expected! Please contact the system administrator.");
            UDA_LOG(UDA_LOG_DEBUG, "[%d] excess data bytes waiting in input buffer!\n", count++);
            throw uda::exceptions::ClientError("Data waiting in the input data buffer when none expected! Please contact the system administrator.");
        }

        rc = xdrrec_eof(client_input_);          // Test for an EOF

        if (!rc) {
            rc = xdrrec_skiprecord(client_input_);    // Flush the input buffer (Zero data waiting but not an EOF!)
        }

        rc = xdrrec_eof(client_input_);          // Test for an EOF

        if (!rc) {
            int err = 999;
            addIdamError(UDA_CODE_ERROR_TYPE, __func__, err,
                         "Corrupted input data stream! Please contact the system administrator.");
            UDA_LOG(UDA_LOG_DEBUG, "Unable to flush input buffer!!!\n");
            throw uda::exceptions::ClientError("Corrupted input data stream! Please contact the system administrator.");
        }

        UDA_LOG(UDA_LOG_DEBUG, "xdrrec_eof rc = 1 => no more input, buffer flushed.\n");
    }

    return 0;
}

int uda::client::Client::send_request_block(RequestBlock& request_block)
{
    int protocol_id = UDA_PROTOCOL_REQUEST_BLOCK;     // This is what the Client Wants
    int err = 0;
    if ((err = protocol2(client_output_, protocol_id, XDR_SEND, nullptr, logmalloclist_, userdefinedtypelist_,
                         &request_block, protocol_version_, &log_struct_list_, private_flags_,
                         malloc_source_)) != 0) {
        addIdamError(UDA_CODE_ERROR_TYPE, __func__, err, "Protocol 1 Error (Request Block)");
        throw uda::exceptions::ClientError("Protocol 1 Error (Request Block)");
    }

    return 0;
}

int uda::client::Client::send_client_block()
{
    int protocol_id = UDA_PROTOCOL_CLIENT_BLOCK;      // Send Client Block
    int err = 0;
    if ((err = protocol2(client_output_, protocol_id, XDR_SEND, nullptr, logmalloclist_, userdefinedtypelist_,
                         &client_block_, protocol_version_, &log_struct_list_, private_flags_,
                         malloc_source_)) != 0) {
        addIdamError(UDA_CODE_ERROR_TYPE, __func__, err, "Protocol 10 Error (Client Block)");
        throw uda::exceptions::ClientError("Protocol 10 Error (Client Block)");
    }

    return 0;
}

int uda::client::Client::perform_handshake()
{
    // Flush (mark as at EOF) the input socket buffer (before the exchange begins)

    int protocol_id = UDA_PROTOCOL_CLIENT_BLOCK;      // Send Client Block (proxy for authenticationStep = 6)

    int err = 0;
    if ((err = protocol2(client_output_, protocol_id, XDR_SEND, nullptr, logmalloclist_, userdefinedtypelist_,
                         &client_block_, protocol_version_, &log_struct_list_, private_flags_,
                         malloc_source_)) != 0) {
        addIdamError(UDA_CODE_ERROR_TYPE, __func__, err, "Protocol 10 Error (Client Block)");
        UDA_LOG(UDA_LOG_DEBUG, "Error Sending Client Block\n");
        throw uda::exceptions::ClientError("Protocol 10 Error (Client Block)");
    }

    if (!(xdrrec_endofrecord(client_output_, 1))) { // Send data now
        err = UDA_PROTOCOL_ERROR_7;
        addIdamError(UDA_CODE_ERROR_TYPE, __func__, err, "Protocol 7 Error (Client Block)");
        UDA_LOG(UDA_LOG_DEBUG, "Error xdrrec_endofrecord after Client Block\n");
        throw uda::exceptions::ClientError("Protocol 7 Error (Client Block)");
    }

    // Flush (mark as at EOF) the input socket buffer (start of wait for data)

    // Wait for data, then position buffer reader to the start of a new record
    if (!(xdrrec_skiprecord(client_input_))) {
        err = UDA_PROTOCOL_ERROR_5;
        addIdamError(UDA_CODE_ERROR_TYPE, __func__, err, "Protocol 5 Error (Server Block)");
        UDA_LOG(UDA_LOG_DEBUG, "Error xdrrec_skiprecord prior to Server Block\n");
        throw uda::exceptions::ClientError("Protocol 5 Error (Server Block)");
    }

    protocol_id = UDA_PROTOCOL_SERVER_BLOCK;      // Receive Server Block: Server Aknowledgement (proxy for authenticationStep = 8)

    if ((err = protocol2(client_input_, protocol_id, XDR_RECEIVE, nullptr, logmalloclist_, userdefinedtypelist_,
                         &server_block_, protocol_version_, &log_struct_list_, private_flags_,
                         malloc_source_)) != 0) {
        addIdamError(UDA_CODE_ERROR_TYPE, __func__, err, "Protocol 11 Error (Server Block #1)");
        // Assuming the server_block is corrupted, replace with a clean copy to avoid concatonation problems
        server_block_.idamerrorstack.nerrors = 0;
        UDA_LOG(UDA_LOG_DEBUG, "Error receiving Server Block\n");
        throw uda::exceptions::ClientError("Protocol 11 Error (Server Block #1)");
    }

    // Flush (mark as at EOF) the input socket buffer (not all server state data may have been read - version dependent)

    int rc = xdrrec_eof(client_input_);

    UDA_LOG(UDA_LOG_DEBUG, "Server Block Received\n");
    UDA_LOG(UDA_LOG_DEBUG, "xdrrec_eof rc = %d [1 => no more input]\n", rc);
    printServerBlock(server_block_);

    // Protocol Version: Lower of the client and server version numbers
    // This defines the set of elements within data structures passed between client and server
    // Must be the same on both sides of the socket
    protocol_version_ = std::min(client_block_.version, server_block_.version);

    if (server_block_.idamerrorstack.nerrors > 0) {
        err = server_block_.idamerrorstack.idamerror[0].code;      // Problem on the Server Side!
        throw uda::exceptions::ServerError(server_block_.idamerrorstack.idamerror[0].msg);
    }

    return 0;
}

int uda::client::Client::flush_sockets()
{
    //------------------------------------------------------------------------------
    // Send the Full TCP packet and wait for the returned data

    int rc = 0;
    if (!(rc = xdrrec_endofrecord(client_output_, 1))) {
        int err = UDA_PROTOCOL_ERROR_7;
        addIdamError(UDA_CODE_ERROR_TYPE, __func__, err, "Protocol 7 Error (Request Block & putDataBlockList)");
        return err;
    }

    UDA_LOG(UDA_LOG_DEBUG, "****** Outgoing tcp packet sent without error. Waiting for data.\n");

    if (!xdrrec_skiprecord(client_input_)) {
        int err = UDA_PROTOCOL_ERROR_5;
        addIdamError(UDA_CODE_ERROR_TYPE, __func__, err, " Protocol 5 Error (Server & Data Structures)");
        return err;
    }

    UDA_LOG(UDA_LOG_DEBUG, "****** Incoming tcp packet received without error. Reading...\n");
    return 0;
}

int uda::client::Client::receive_server_block()
{
    //------------------------------------------------------------------------------
    // Receive the Server State/Aknowledgement that the Data has been Accessed
    // Just in case the Server has crashed!

    UDA_LOG(UDA_LOG_DEBUG, "Waiting for Server Status Block\n");

    int err = 0;
    if ((err = protocol2(client_input_, UDA_PROTOCOL_SERVER_BLOCK, XDR_RECEIVE, nullptr, logmalloclist_,
                         userdefinedtypelist_, &server_block_, protocol_version_, &log_struct_list_,
                         private_flags_, malloc_source_)) != 0) {
        UDA_LOG(UDA_LOG_DEBUG, "Protocol 11 Error (Server Block #2) = %d\n", err);
        addIdamError(UDA_CODE_ERROR_TYPE, __func__, err, " Protocol 11 Error (Server Block #2)");
        // Assuming the server_block is corrupted, replace with a clean copy to avoid future concatonation problems
        server_block_.idamerrorstack.nerrors = 0;
        throw uda::exceptions::ClientError("Protocol 11 Error (Server Block #2) = %1%", err);
    }

    UDA_LOG(UDA_LOG_DEBUG, "Server Block Received\n");
    printServerBlock(server_block_);

    return 0;
}

void uda::client::Client::clear()
{
    data_blocks_.clear();
}

int uda::client::Client::new_handle()
{
    data_blocks_.emplace_back(DataBlock{});
    return static_cast<int>(data_blocks_.size() - 1);
}

void uda::client::Client::set_flag(unsigned int flag, bool private_flag)
{
    if (private_flag) {
        private_flags_ |= flag;
    } else {
        client_flags_.flags |= flag;
    }
}

void uda::client::Client::reset_flag(unsigned int flag, bool private_flag)
{
    if (private_flag) {
        private_flags_ &= !flag;
    } else {
        client_flags_.flags &= !flag;
    }
}

void uda::client::Client::set_property(const char* property)
{
    // User settings for Client and Server behaviour

    char name[56];
    char* value;

    if (property[0] == 'g') {
        if (STR_IEQUALS(property, "get_datadble")) client_flags_.get_datadble = 1;
        if (STR_IEQUALS(property, "get_dimdble")) client_flags_.get_dimdble = 1;
        if (STR_IEQUALS(property, "get_timedble")) client_flags_.get_timedble = 1;
        if (STR_IEQUALS(property, "get_bytes")) client_flags_.get_bytes = 1;
        if (STR_IEQUALS(property, "get_bad")) client_flags_.get_bad = 1;
        if (STR_IEQUALS(property, "get_meta")) client_flags_.get_meta = 1;
        if (STR_IEQUALS(property, "get_asis")) client_flags_.get_asis = 1;
        if (STR_IEQUALS(property, "get_uncal")) client_flags_.get_uncal = 1;
        if (STR_IEQUALS(property, "get_notoff")) client_flags_.get_notoff = 1;
        if (STR_IEQUALS(property, "get_synthetic")) client_flags_.get_synthetic = 1;
        if (STR_IEQUALS(property, "get_scalar")) client_flags_.get_scalar = 1;
        if (STR_IEQUALS(property, "get_nodimdata")) client_flags_.get_nodimdata = 1;
    } else {
        if (property[0] == 't') {
            strncpy(name, property, 55);
            name[55] = '\0';
            TrimString(name);
            LeftTrimString(name);
            MidTrimString(name);
            strlwr(name);
            if ((value = strstr(name, "timeout=")) != nullptr) {
                value = name + 8;
                if (IsNumber(value)) client_flags_.user_timeout = atoi(value);
            }
        } else {
            if (STR_IEQUALS(property, "verbose")) udaSetLogLevel(UDA_LOG_INFO);
            if (STR_IEQUALS(property, "debug")) udaSetLogLevel(UDA_LOG_DEBUG);
            if (STR_IEQUALS(property, "altData")) client_flags_.flags = client_flags_.flags | CLIENTFLAG_ALTDATA;
            if (!strncasecmp(property, "altRank", 7)) {
                strncpy(name, property, 55);
                name[55] = '\0';
                TrimString(name);
                LeftTrimString(name);
                MidTrimString(name);
                strlwr(name);
                if ((value = strcasestr(name, "altRank=")) != nullptr) {
                    value = name + 8;
                    if (IsNumber(value)) client_flags_.alt_rank = atoi(value);
                }
            }
        }
        if (STR_IEQUALS(property, "reuseLastHandle")) client_flags_.flags = client_flags_.flags | CLIENTFLAG_REUSELASTHANDLE;
        if (STR_IEQUALS(property, "freeAndReuseLastHandle")) client_flags_.flags = client_flags_.flags | CLIENTFLAG_FREEREUSELASTHANDLE;
        if (STR_IEQUALS(property, "fileCache")) client_flags_.flags = client_flags_.flags | CLIENTFLAG_FILECACHE;
    }
}

int uda::client::Client::get_property(const char* property)
{
    // User settings for Client and Server behaviour

    if (property[0] == 'g') {
        if (STR_IEQUALS(property, "get_datadble")) return client_flags_.get_datadble;
        if (STR_IEQUALS(property, "get_dimdble")) return client_flags_.get_dimdble;
        if (STR_IEQUALS(property, "get_timedble")) return client_flags_.get_timedble;
        if (STR_IEQUALS(property, "get_bytes")) return client_flags_.get_bytes;
        if (STR_IEQUALS(property, "get_bad")) return client_flags_.get_bad;
        if (STR_IEQUALS(property, "get_meta")) return client_flags_.get_meta;
        if (STR_IEQUALS(property, "get_asis")) return client_flags_.get_asis;
        if (STR_IEQUALS(property, "get_uncal")) return client_flags_.get_uncal;
        if (STR_IEQUALS(property, "get_notoff")) return client_flags_.get_notoff;
        if (STR_IEQUALS(property, "get_synthetic")) return client_flags_.get_synthetic;
        if (STR_IEQUALS(property, "get_scalar")) return client_flags_.get_scalar;
        if (STR_IEQUALS(property, "get_nodimdata")) return client_flags_.get_nodimdata;
    } else {
        if (STR_IEQUALS(property, "timeout")) return client_flags_.user_timeout;
        if (STR_IEQUALS(property, "altRank")) return alt_rank_;
        if (STR_IEQUALS(property, "reuseLastHandle")) return (int)(client_flags_.flags & CLIENTFLAG_REUSELASTHANDLE);
        if (STR_IEQUALS(property, "freeAndReuseLastHandle")) return (int)(client_flags_.flags & CLIENTFLAG_FREEREUSELASTHANDLE);
        if (STR_IEQUALS(property, "verbose")) return udaGetLogLevel() == UDA_LOG_INFO;
        if (STR_IEQUALS(property, "debug")) return udaGetLogLevel() == UDA_LOG_DEBUG;
        if (STR_IEQUALS(property, "altData")) return (int)(client_flags_.flags & CLIENTFLAG_ALTDATA);
        if (STR_IEQUALS(property, "fileCache")) return (int)(client_flags_.flags & CLIENTFLAG_FILECACHE);
    }
    return 0;
}

void uda::client::Client::reset_property(const char* property)
{
    // User settings for Client and Server behaviour

    if (property[0] == 'g') {
        if (STR_IEQUALS(property, "get_datadble")) client_flags_.get_datadble = 0;
        if (STR_IEQUALS(property, "get_dimdble")) client_flags_.get_dimdble = 0;
        if (STR_IEQUALS(property, "get_timedble")) client_flags_.get_timedble = 0;
        if (STR_IEQUALS(property, "get_bytes")) client_flags_.get_bytes = 0;
        if (STR_IEQUALS(property, "get_bad")) client_flags_.get_bad = 0;
        if (STR_IEQUALS(property, "get_meta")) client_flags_.get_meta = 0;
        if (STR_IEQUALS(property, "get_asis")) client_flags_.get_asis = 0;
        if (STR_IEQUALS(property, "get_uncal")) client_flags_.get_uncal = 0;
        if (STR_IEQUALS(property, "get_notoff")) client_flags_.get_notoff = 0;
        if (STR_IEQUALS(property, "get_synthetic")) client_flags_.get_synthetic = 0;
        if (STR_IEQUALS(property, "get_scalar")) client_flags_.get_scalar = 0;
        if (STR_IEQUALS(property, "get_nodimdata")) client_flags_.get_nodimdata = 0;
    } else {
        if (STR_IEQUALS(property, "verbose")) udaSetLogLevel(UDA_LOG_NONE);
        if (STR_IEQUALS(property, "debug")) udaSetLogLevel(UDA_LOG_NONE);
        if (STR_IEQUALS(property, "altData")) client_flags_.flags &= !CLIENTFLAG_ALTDATA;
        if (STR_IEQUALS(property, "altRank")) client_flags_.alt_rank = 0;
        if (STR_IEQUALS(property, "reuseLastHandle")) client_flags_.flags &= !CLIENTFLAG_REUSELASTHANDLE;
        if (STR_IEQUALS(property, "freeAndReuseLastHandle")) {
            client_flags_.flags &= !CLIENTFLAG_FREEREUSELASTHANDLE;
        }
        if (STR_IEQUALS(property, "fileCache")) client_flags_.flags &= !CLIENTFLAG_FILECACHE;
    }
}

void uda::client::Client::reset_properties()
{
    // Reset on Both Client and Server

    client_flags_.get_datadble = 0;
    client_flags_.get_dimdble = 0;
    client_flags_.get_timedble = 0;
    client_flags_.get_bad = 0;
    client_flags_.get_meta = 0;
    client_flags_.get_asis = 0;
    client_flags_.get_uncal = 0;
    client_flags_.get_notoff = 0;
    client_flags_.get_synthetic = 0;
    client_flags_.get_scalar = 0;
    client_flags_.get_bytes = 0;
    client_flags_.get_nodimdata = 0;
    udaSetLogLevel(UDA_LOG_NONE);
    client_flags_.user_timeout = TIMEOUT;
    if (getenv("UDA_TIMEOUT")) {
        client_flags_.user_timeout = atoi(getenv("UDA_TIMEOUT"));
    }
    client_flags_.flags = 0;
    client_flags_.alt_rank = 0;
}

DATA_BLOCK* uda::client::Client::data_block(int handle)
{
    auto idx = static_cast<size_t>(handle);
    if (idx < data_blocks_.size()) {
        return &data_blocks_[idx];
    } else {
        return nullptr;
    }
}

const CLIENT_BLOCK* uda::client::Client::client_block(int handle)
{
    return &client_block_;
}

void uda::client::Client::concat_errors(UDA_ERROR_STACK* error_stack)
{
    if (error_stack_.empty()) {
        return;
    }

    unsigned int iold = error_stack->nerrors;
    unsigned int inew = error_stack_.size() + error_stack->nerrors;

    error_stack->idamerror = (UDA_ERROR*)realloc((void*)error_stack->idamerror, (inew * sizeof(UDA_ERROR)));

    for (unsigned int i = iold; i < inew; i++) {
        error_stack->idamerror[i] = error_stack_[i - iold];
    }
    error_stack->nerrors = inew;
}

const CLIENT_FLAGS* uda::client::Client::client_flags()
{
    return &client_flags_;
}

DATA_BLOCK* uda::client::Client::current_data_block()
{
    if (!data_blocks_.empty()) {
        return &data_blocks_.back();
    } else {
        return nullptr;
    }
}

const SERVER_BLOCK* uda::client::Client::server_block()
{
    return &server_block_;
}

ENVIRONMENT* uda::client::Client::environment()
{
    return &environment_;
}

void uda::client::Client::set_user_defined_type_list(USERDEFINEDTYPELIST* userdefinedtypelist)
{
    userdefinedtypelist_ = userdefinedtypelist;
}

void uda::client::Client::set_log_malloc_list(LOGMALLOCLIST* logmalloclist)
{
    logmalloclist_ = logmalloclist;
}

void uda::client::Client::set_full_ntree(NTREE* full_ntree)
{
    full_ntree_ = full_ntree;
}

int uda::client::Client::put(std::string_view put_instruction, PUTDATA_BLOCK* putdata_block)
{
    REQUEST_BLOCK request_block;
    initRequestBlock(&request_block);

    auto signal_ptr = put_instruction.data();
    auto source_ptr = "";

    if (make_request_block(&environment_, &signal_ptr, &source_ptr, 1, &request_block) != 0) {
        if (udaNumErrors() == 0) {
            UDA_LOG(UDA_LOG_ERROR, "Error identifying the Data Source\n");
            addIdamError(UDA_CODE_ERROR_TYPE, __func__, 999, "Error identifying the Data Source");
        }
        throw uda::exceptions::ClientError("Error identifying the Data Source");
    }

    printRequestBlock(request_block);

    request_block.requests[0].put = 1; // flags the direction of data (0 is default => get operation)
    addIdamPutDataBlockList(putdata_block, &request_block.requests[0].putDataBlockList);

    std::vector<int> indices(request_block.num_requests);
    get_requests(request_block, indices.data());

    freeClientPutDataBlockList(&request_block.requests[0].putDataBlockList);

    return indices[0];
}

int uda::client::Client::put(std::string_view put_instruction, PUTDATA_BLOCK_LIST* putdata_block_list)
{
    REQUEST_BLOCK request_block;
    initRequestBlock(&request_block);

    auto signal_ptr = put_instruction.data();
    auto source_ptr = "";

    if (make_request_block(&environment_, &signal_ptr, &source_ptr, 1, &request_block) != 0) {
        if (udaNumErrors() == 0) {
            UDA_LOG(UDA_LOG_ERROR, "Error identifying the Data Source\n");
            addIdamError(UDA_CODE_ERROR_TYPE, __func__, 999, "Error identifying the Data Source");
        }
        throw uda::exceptions::ClientError("Error identifying the Data Source");
    }

    printRequestBlock(request_block);

    request_block.requests[0].put = 1; // flags the direction of data (0 is default => get operation)
    request_block.requests[0].putDataBlockList = *putdata_block_list;

    std::vector<int> indices(request_block.num_requests);
    get_requests(request_block, indices.data());

    return indices[0];
}
