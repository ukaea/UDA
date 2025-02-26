/*---------------------------------------------------------------
 * Client - Server Conversation Protocol
 *
 * Args:    xdrs        XDR Stream
 *
 *    protocol_id    Client/Server Conversation item: Data Exchange context
 *    direction    Send (0) or Receive (1) or Free (2)
 *    token        current error condition or next protocol or .... exchange token
 *
 *    str        Information Structure depending on the protocol id ....
 *
 *    2    data_block    Data read from the external Source or Data to be written
 *                to an external source
 *    4    data_system    Database Data_Dystem table record
 *    5    system_config    Database System_Config table record
 *    6    data_source    Database Data_Source table record
 *    7    signal        Database Signal table record
 *    8    signal_desc    Database Signal_Desc table record
 *
 * Returns: error code if failure, otherwise 0
 *
 *--------------------------------------------------------------*/

#include "protocol.h"

#include <cstdlib>

#include "alloc_data.h"
#include "protocolXML.h"
#include "xdrlib.h"

#include "clientserver/compress_dim.h"
#include "clientserver/error_log.h"
#include "clientserver/init_structs.h"
#include "clientserver/uda_errors.h"
#include "logging/logging.h"

#ifdef SERVERBUILD
#  include "server/serverStartup.h"
#endif

using namespace uda::client_server;
using namespace uda::logging;
using namespace uda::structures;

namespace {

constexpr int MinBlockTime = 1000;
constexpr int MaxBlockTime = 10000;

} // anon namespace

std::string uda::protocol::format_as(ProtocolId protocol)
{
    switch (protocol) {
        case ProtocolId::Start: return "ProtocolId::Start";
        case ProtocolId::RequestBlock: return "ProtocolId::RequestBlock";
        case ProtocolId::DataBlockList: return "ProtocolId::DataBlockList";
        case ProtocolId::NextProtocol: return "ProtocolId::NextProtocol";
        case ProtocolId::MetaData: return "ProtocolId::MetaData";
        case ProtocolId::Spare1: return "ProtocolId::Spare1";
        case ProtocolId::ClientBlock: return "ProtocolId::ClientBlock";
        case ProtocolId::ServerBlock: return "ProtocolId::ServerBlock";
        case ProtocolId::Spare2: return "ProtocolId::Spare2";
        case ProtocolId::CloseDown: return "ProtocolId::CloseDown";
        case ProtocolId::Sleep: return "ProtocolId::Sleep";
        case ProtocolId::WakeUp: return "ProtocolId::WakeUp";
        case ProtocolId::PutdataBlockList: return "ProtocolId::PutdataBlockList";
        // case ProtocolId::SecurityBlock: return "ProtocolId::SecurityBlock";
        case ProtocolId::Object: return "ProtocolId::Object";
        case ProtocolId::SerialiseObject: return "ProtocolId::SerialiseObject";
        case ProtocolId::SerialiseFile: return "ProtocolId::SerialiseFile";
        case ProtocolId::DataObject: return "ProtocolId::DataObject";
        case ProtocolId::DataObjectFile: return "ProtocolId::DataObjectFile";
        case ProtocolId::RegularStop: return "ProtocolId::RegularStop";
        case ProtocolId::OpaqueStart: return "ProtocolId::OpaqueStart";
        case ProtocolId::Structures: return "ProtocolId::Structures";
        case ProtocolId::Meta: return "ProtocolId::Meta";
        case ProtocolId::OpaqueStop: return "ProtocolId::OpaqueStop";
    }
    return "";
}

void uda::protocol::set_select_params(int fd, fd_set* rfds, struct timeval* tv, int* server_tot_block_time)
{
    FD_ZERO(rfds);    // Initialise the File Descriptor set
    FD_SET(fd, rfds); // Identify the Socket in the FD set
    tv->tv_sec = 0;
    tv->tv_usec = MinBlockTime; // minimum wait microsecs (1ms)
    *server_tot_block_time = 0;
}

void uda::protocol::update_select_params(int fd, fd_set* rfds, struct timeval* tv, int server_tot_block_time)
{
    FD_ZERO(rfds);
    FD_SET(fd, rfds);
    if (server_tot_block_time < MaxBlock) {
        // (ms) For the First blocking period have rapid response (clientserver/uda_defines.h == 1000)
        tv->tv_sec = 0;
        tv->tv_usec = MinBlockTime; // minimum wait (1ms)
    } else {
        tv->tv_sec = 0;
        tv->tv_usec = MaxBlockTime; // maximum wait (10ms)
    }
}

int uda::protocol::protocol(std::vector<UdaError>& error_stack, XDR* xdrs, ProtocolId protocol_id, XDRStreamDirection direction, ProtocolId* token, LogMallocList* logmalloclist,
                                 UserDefinedTypeList* userdefinedtypelist, void* str, int protocolVersion,
                                 LogStructList* log_struct_list, IoData* io_data, unsigned int private_flags,
                                 int malloc_source)
{
    int err = 0;

    UDA_LOG(UDA_LOG_DEBUG, "\nPROTOCOL: protocolVersion = {}\n", protocolVersion);

    //----------------------------------------------------------------------------
    // Error Management Loop

    do {

        //----------------------------------------------------------------------------
        // Retrieve Client Requests

        if (protocol_id == ProtocolId::RequestBlock) {

            auto request_block = (RequestBlock*)str;

            switch (direction) {
                case XDRStreamDirection::Receive:
                    if (!xdrrec_skiprecord(xdrs)) {
                        err = (int)ProtocolError::Error5;
                        break;
                    }
                    if (!xdr_request(xdrs, request_block, protocolVersion)) {
                        err = (int)ProtocolError::Error1;
                        break;
                    }
                    break;

                case XDRStreamDirection::Send:
                    if (!xdr_request(xdrs, request_block, protocolVersion)) {
                        err = (int)ProtocolError::Error2;
                        break;
                    }
                    if (!xdrrec_endofrecord(xdrs, 1)) {
                        err = (int)ProtocolError::Error7;
                        break;
                    }
                    break;

                case XDRStreamDirection::FreeHeap:
                    if (!xdr_request(xdrs, request_block, protocolVersion)) {
                        err = (int)ProtocolError::Error3;
                        break;
                    }
                    break;

                default:
                    err = (int)ProtocolError::Error4;
                    break;
            }

            break;
        }

        //----------------------------------------------------------------------------
        // Data Block

        if (protocol_id == ProtocolId::DataBlockList) {

            auto data_block = (DataBlock*)str;

            switch (direction) {
                case XDRStreamDirection::Receive:
                    if (!xdrrec_skiprecord(xdrs)) {
                        err = (int)ProtocolError::Error5;
                        break;
                    }
                    if (!xdr_data_block1(xdrs, data_block, protocolVersion)) {
                        err = (int)ProtocolError::Error61;
                        break;
                    }

                    // Check client/server understands new data types
                    // direction == XDRStreamDirection::Receive && protocolVersion == 3 Means Client receiving data from a
                    // Version >= 3 Server (Type has to be passed first)

                    if (protocol_version_type_test(protocolVersion, data_block->data_type) ||
                        protocol_version_type_test(protocolVersion, data_block->error_type)) {
                        err = (int)ProtocolError::Error9999;
                        break;
                    }

                    if (data_block->data_n == 0) {
                        break; // No Data to Receive!
                    }

                    if ((err = alloc_data(data_block)) != 0) {
                        break; // Allocate Heap Memory
                    }

                    if (!xdr_data_block2(xdrs, data_block)) {
                        err = (int)ProtocolError::Error62;
                        break;
                    }

                    if (data_block->error_type != UDA_TYPE_UNKNOWN ||
                        data_block->error_param_n > 0) { // Receive Only if Error Data are available
                        if (!xdr_data_block3(xdrs, data_block)) {
                            err = (int)ProtocolError::Error62;
                            break;
                        }

                        if (!xdr_data_block4(xdrs, data_block)) { // Asymmetric Errors
                            err = (int)ProtocolError::Error62;
                            break;
                        }
                    }

                    if (data_block->rank > 0) { // Check if there are Dimensional Data to Receive
                        for (unsigned int i = 0; i < data_block->rank; i++) {
                            init_dim_block(&data_block->dims[i]);
                        }

                        if (!xdr_data_dim1(xdrs, data_block)) {
                            err = (int)ProtocolError::Error63;
                            break;
                        }

                        if (protocolVersion < 3) {
                            for (unsigned int i = 0; i < data_block->rank; i++) {
                                auto dim = &data_block->dims[i];
                                if (protocol_version_type_test(protocolVersion, dim->data_type) ||
                                    protocol_version_type_test(protocolVersion, dim->error_type)) {
                                    err = (int)ProtocolError::Error9999;
                                    break;
                                }
                            }
                        }
                        if (err) {
                            break;
                        }

                        if ((err = alloc_dim(data_block)) != 0) {
                            break; // Allocate Heap Memory
                        }

                        if (!xdr_data_dim2(xdrs, data_block)) { // Collect Only Uncompressed data
                            err = (int)ProtocolError::Error64;
                            break;
                        }

                        for (unsigned int i = 0; i < data_block->rank; i++) { // Expand Compressed Regular Vector
                            err = uncompress_dim(&(data_block->dims[i]));     // Allocate Heap as required
                            err = 0;                                          // Need to Test for Error Condition!
                        }

                        if (!xdr_data_dim3(xdrs, data_block)) {
                            err = (int)ProtocolError::Error65;
                            break;
                        }

                        if (!xdr_data_dim4(xdrs, data_block)) { // Asymmetric Errors
                            err = (int)ProtocolError::Error65;
                            break;
                        }
                    }

                    break;

                case XDRStreamDirection::Send:

                    // Check client/server understands new data types

                    // direction == XDRStreamDirection::Send && protocolVersion == 3 Means Server sending data to a Version 3 Client
                    // (Type is known)

                    if (protocol_version_type_test(protocolVersion, data_block->data_type) ||
                        protocol_version_type_test(protocolVersion, data_block->error_type)) {
                        err = (int)ProtocolError::Error9999;
                        break;
                    }

                    if (!xdr_data_block1(xdrs, data_block, protocolVersion)) {
                        err = (int)ProtocolError::Error61;
                        break;
                    }

                    if (data_block->data_n == 0) { // No Data or Dimensions to Send!
                        if (!xdrrec_endofrecord(xdrs, 1)) {
                            err = (int)ProtocolError::Error7;
                        }
                        break;
                    }

                    if (!xdr_data_block2(xdrs, data_block)) {
                        err = (int)ProtocolError::Error62;
                        break;
                    }

                    if (data_block->error_type != UDA_TYPE_UNKNOWN ||
                        data_block->error_param_n > 0) { // Only Send if Error Data are available
                        if (!xdr_data_block3(xdrs, data_block)) {
                            err = (int)ProtocolError::Error62;
                            break;
                        }
                        if (!xdr_data_block4(xdrs, data_block)) {
                            err = (int)ProtocolError::Error62;
                            break;
                        }
                    }

                    if (data_block->rank > 0) { // Dimensional Data to Send

                        // Check client/server understands new data types

                        if (protocolVersion < 3) {
                            for (unsigned int i = 0; i < data_block->rank; i++) {
                                auto dim = &data_block->dims[i];
                                if (protocol_version_type_test(protocolVersion, dim->data_type) ||
                                    protocol_version_type_test(protocolVersion, dim->error_type)) {
                                    err = (int)ProtocolError::Error9999;
                                    break;
                                }
                            }
                        }

                        for (unsigned int i = 0; i < data_block->rank; i++) {
                            compress_dim(&(data_block->dims[i])); // Minimise Data Transfer if Regular
                        }

                        if (!xdr_data_dim1(xdrs, data_block)) {
                            err = (int)ProtocolError::Error63;
                            break;
                        }

                        if (!xdr_data_dim2(xdrs, data_block)) {
                            err = (int)ProtocolError::Error64;
                            break;
                        }

                        if (!xdr_data_dim3(xdrs, data_block)) {
                            err = (int)ProtocolError::Error65;
                        }

                        if (!xdr_data_dim4(xdrs, data_block)) {
                            err = (int)ProtocolError::Error65;
                        }
                    }

                    if (!xdrrec_endofrecord(xdrs, 1)) {
                        err = (int)ProtocolError::Error7;
                    }

                    break;

                case XDRStreamDirection::FreeHeap:
                    break;

                default:
                    err = (int)ProtocolError::Error4;
                    break;
            }
            break;
        }

        //----------------------------------------------------------------------------
        // Data Block

        if (protocol_id == ProtocolId::PutdataBlockList) {

            auto put_data_block_list = (PutDataBlockList*)str;

            switch (direction) {

                case XDRStreamDirection::Receive: {
                    PutDataBlock put_data;

                    if (!xdrrec_skiprecord(xdrs)) {
                        err = (int)ProtocolError::Error5;
                        break;
                    }

                    unsigned int blockCount = 0;
                    int rc = xdr_u_int(xdrs, &blockCount);

                    if (!rc) {
                        err = (int)ProtocolError::Error61;
                        break;
                    }

                    UDA_LOG(UDA_LOG_DEBUG, "receive: putDataBlockList Count: {}", blockCount);

                    for (unsigned int i = 0; i < blockCount; i++) { // Fetch multiple put blocks

                        init_put_data_block(&put_data);

                        if (!xdr_putdata_block1(xdrs, &put_data)) {
                            err = (int)ProtocolError::Error61;
                            UDA_LOG(UDA_LOG_DEBUG, "xdr_putdata_block1 Error (61)");
                            break;
                        }

                        if (protocol_version_type_test(protocolVersion, put_data.data_type)) {
                            err = (int)ProtocolError::Error9999;
                            break;
                        }

                        if (put_data.count > 0 || put_data.blockNameLength > 0) {

                            if ((err = alloc_put_data(&put_data)) != 0) {
                                break; // Allocate Heap Memory
                            }

                            if (!xdr_putdata_block2(xdrs, &put_data)) { // Fetch data
                                err = (int)ProtocolError::Error62;
                                break;
                            }

                            put_data_block_list->push_back(put_data);
                        }
                    }

                    break;
                }

                case XDRStreamDirection::Send: {

                    unsigned int size = put_data_block_list->size();
                    UDA_LOG(UDA_LOG_DEBUG, "send: putDataBlockList Count: {}", size);

                    int rc = xdr_u_int(xdrs, &size);

                    if (!rc) {
                        err = (int)ProtocolError::Error61;
                        break;
                    }

                    for (unsigned int i = 0; i < size; i++) {
                        // Send multiple put blocks
                        auto* put_data = &(*put_data_block_list)[i];

                        if (!xdr_putdata_block1(xdrs, put_data)) {
                            err = (int)ProtocolError::Error61;
                            break;
                        }

                        if (protocol_version_type_test(protocolVersion, put_data->data_type)) {
                            err = (int)ProtocolError::Error9999;
                            break;
                        }

                        if (put_data->count > 0 || put_data->blockNameLength > 0) {
                            // Data to Send?

                            if (!xdr_putdata_block2(xdrs, &put_data_block_list->at(i))) {
                                err = (int)ProtocolError::Error62;
                                break;
                            }
                        }
                    }

                    if (!xdrrec_endofrecord(xdrs, 1)) {
                        err = (int)ProtocolError::Error7;
                    }

                    break;
                }

                case XDRStreamDirection::FreeHeap:
                    break;

                default:
                    err = (int)ProtocolError::Error4;
                    break;
            }

            break;
        }

        //----------------------------------------------------------------------------
        // Error Status or Next Protocol id or exchange token ....

        if (protocol_id == ProtocolId::NextProtocol) {

            switch (direction) {

                case XDRStreamDirection::Receive: // From Client to Server
                    if (!xdrrec_skiprecord(xdrs)) {
                        err = (int)ProtocolError::Error5;
                        break;
                    }
                    if (!xdr_int(xdrs, (int*)token)) {
                        err = (int)ProtocolError::Error9;
                        break;
                    }
                    break;

                case XDRStreamDirection::Send:
                    if (!xdr_int(xdrs, (int*)token)) {
                        err = (int)ProtocolError::Error9;
                        break;
                    }
                    if (!xdrrec_endofrecord(xdrs, 1)) {
                        err = (int)ProtocolError::Error7;
                        break;
                    }
                    break;

                case XDRStreamDirection::FreeHeap:
                    err = (int)ProtocolError::Error3;
                    break;

                default:
                    err = (int)ProtocolError::Error4;
                    break;
            }

            break;
        }

        //----------------------------------------------------------------------------
        // Meta Data

        if (protocol_id == ProtocolId::MetaData) {
            auto data_system = static_cast<MetaData*>(str);

            switch (direction) {
                case XDRStreamDirection::Receive: // From Client to Server
                    if (!xdrrec_skiprecord(xdrs)) {
                        err = (int)ProtocolError::Error5;
                        break;
                    }
                    if (!xdr_metadata(xdrs, data_system)) {
                        err = (int)ProtocolError::Error10;
                        break;
                    }
                    break;

                case XDRStreamDirection::Send:
                    if (!xdr_metadata(xdrs, data_system)) {
                        err = (int)ProtocolError::Error10;
                        break;
                    }
                    if (!xdrrec_endofrecord(xdrs, 1)) {
                        err = (int)ProtocolError::Error7;
                        break;
                    }
                    break;

                case XDRStreamDirection::FreeHeap:
                    if (!xdr_metadata(xdrs, data_system)) {
                        err = (int)ProtocolError::Error11;
                        break;
                    }
                    break;

                default:
                    err = (int)ProtocolError::Error4;
                    break;
            }
            break;
        }

        //----------------------------------------------------------------------------
        // Client State

        if (protocol_id == ProtocolId::ClientBlock) {

            auto client_block = (ClientBlock*)str;

            switch (direction) {

                case XDRStreamDirection::Receive:
                    // From Client to Server
                    if (!xdrrec_skiprecord(xdrs)) {
                        err = (int)ProtocolError::Error5;
                        break;
                    }
                    if (!xdr_client(xdrs, client_block, protocolVersion)) {
                        err = (int)ProtocolError::Error20;
                        break;
                    }

                    break;

                case XDRStreamDirection::Send:

                    if (!xdr_client(xdrs, client_block, protocolVersion)) {
                        err = (int)ProtocolError::Error20;
                        break;
                    }
                    if (!xdrrec_endofrecord(xdrs, 1)) {
                        err = (int)ProtocolError::Error7;
                        break;
                    }
                    break;

                case XDRStreamDirection::FreeHeap:

                    if (!xdr_client(xdrs, client_block, protocolVersion)) {
                        err = (int)ProtocolError::Error21;
                        break;
                    }
                    break;

                default:
                    err = (int)ProtocolError::Error4;
                    break;
            }

            break;
        }

        //----------------------------------------------------------------------------
        // Server State

        if (protocol_id == ProtocolId::ServerBlock) {

            auto server_block = (ServerBlock*)str;

            switch (direction) {

                case XDRStreamDirection::Receive:

                    if (!xdrrec_skiprecord(xdrs)) {
                        err = (int)ProtocolError::Error5;
                        break;
                    }

                    server_block->error_stack.clear();

                    if (!xdr_server1(xdrs, server_block, protocolVersion)) {
                        err = (int)ProtocolError::Error22;
                        break;
                    }

                    if (!server_block->error_stack.empty()) {
                        if (!xdr_server2(xdrs, server_block)) {
                            err = (int)ProtocolError::Error22;
                            break;
                        }
                    }

                    break;

                case XDRStreamDirection::Send:
                    if (!xdr_server1(xdrs, server_block, protocolVersion)) {
                        err = (int)ProtocolError::Error22;
                        break;
                    }

                    if (!server_block->error_stack.empty()) { // No Data to Send?
                        if (!xdr_server2(xdrs, server_block)) {
                            err = (int)ProtocolError::Error22;
                            break;
                        }
                    }

                    if (!xdrrec_endofrecord(xdrs, 1)) {
                        err = (int)ProtocolError::Error7;
                        break;
                    }
                    break;

                case XDRStreamDirection::FreeHeap:

                    if (!xdr_server(xdrs, server_block)) {
                        err = (int)ProtocolError::Error23;
                        break;
                    }
                    break;

                default:
                    err = (int)ProtocolError::Error4;
                    break;
            }

            break;
        }

        //----------------------------------------------------------------------------
        // Hierarchical or Meta Data Structures

        if (protocol_id > ProtocolId::OpaqueStart && protocol_id < ProtocolId::OpaqueStop) {
            err = protocol_xml(error_stack, xdrs, protocol_id, direction, token, logmalloclist, userdefinedtypelist, str,
                               protocolVersion, log_struct_list, io_data, private_flags, malloc_source, nullptr);
        }

        //----------------------------------------------------------------------------
        // End of Error Trap Loop

    } while (0);

    return err;
}
