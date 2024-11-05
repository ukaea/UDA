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

#include "logging/logging.h"
#include <cstdlib>

#include "allocData.h"
#include "compressDim.h"
#include "errorLog.h"
#include "initStructs.h"
#include "protocolXML.h"
#include "udaErrors.h"
#include "xdrlib.h"

#ifdef SERVERBUILD
#  include "server/serverStartup.h"
#endif

using namespace uda::client_server;
using namespace uda::logging;
using namespace uda::structures;

void uda::client_server::set_select_params(int fd, fd_set* rfds, struct timeval* tv, int* server_tot_block_time)
{
    FD_ZERO(rfds);    // Initialise the File Descriptor set
    FD_SET(fd, rfds); // Identify the Socket in the FD set
    tv->tv_sec = 0;
    tv->tv_usec = MIN_BLOCK_TIME; // minimum wait microsecs (1ms)
    *server_tot_block_time = 0;
}

void uda::client_server::update_select_params(int fd, fd_set* rfds, struct timeval* tv, int server_tot_block_time)
{
    FD_ZERO(rfds);
    FD_SET(fd, rfds);
    if (server_tot_block_time < MaxBlock) {
        // (ms) For the First blocking period have rapid response (clientserver/udaDefines.h == 1000)
        tv->tv_sec = 0;
        tv->tv_usec = MIN_BLOCK_TIME; // minimum wait (1ms)
    } else {
        tv->tv_sec = 0;
        tv->tv_usec = MAX_BLOCK_TIME; // maximum wait (10ms)
    }
}

int uda::client_server::protocol(XDR* xdrs, ProtocolId protocol_id, XDRStreamDirection direction, ProtocolId* token, LogMallocList* logmalloclist,
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
                        err = UDA_PROTOCOL_ERROR_5;
                        break;
                    }
                    if (!xdr_request(xdrs, request_block, protocolVersion)) {
                        err = UDA_PROTOCOL_ERROR_1;
                        break;
                    }
                    break;

                case XDRStreamDirection::Send:
                    if (!xdr_request(xdrs, request_block, protocolVersion)) {
                        err = UDA_PROTOCOL_ERROR_2;
                        break;
                    }
                    if (!xdrrec_endofrecord(xdrs, 1)) {
                        err = UDA_PROTOCOL_ERROR_7;
                        break;
                    }
                    break;

                case XDRStreamDirection::FreeHeap:
                    if (!xdr_request(xdrs, request_block, protocolVersion)) {
                        err = UDA_PROTOCOL_ERROR_3;
                        break;
                    }
                    break;

                default:
                    err = UDA_PROTOCOL_ERROR_4;
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
                        err = UDA_PROTOCOL_ERROR_5;
                        break;
                    }
                    if (!xdr_data_block1(xdrs, data_block, protocolVersion)) {
                        err = UDA_PROTOCOL_ERROR_61;
                        break;
                    }

                    // Check client/server understands new data types
                    // direction == XDRStreamDirection::Receive && protocolVersion == 3 Means Client receiving data from a
                    // Version >= 3 Server (Type has to be passed first)

                    if (protocol_version_type_test(protocolVersion, data_block->data_type) ||
                        protocol_version_type_test(protocolVersion, data_block->error_type)) {
                        err = UDA_PROTOCOL_ERROR_9999;
                        break;
                    }

                    if (data_block->data_n == 0) {
                        break; // No Data to Receive!
                    }

                    if ((err = alloc_data(data_block)) != 0) {
                        break; // Allocate Heap Memory
                    }

                    if (!xdr_data_block2(xdrs, data_block)) {
                        err = UDA_PROTOCOL_ERROR_62;
                        break;
                    }

                    if (data_block->error_type != UDA_TYPE_UNKNOWN ||
                        data_block->error_param_n > 0) { // Receive Only if Error Data are available
                        if (!xdr_data_block3(xdrs, data_block)) {
                            err = UDA_PROTOCOL_ERROR_62;
                            break;
                        }

                        if (!xdr_data_block4(xdrs, data_block)) { // Asymmetric Errors
                            err = UDA_PROTOCOL_ERROR_62;
                            break;
                        }
                    }

                    if (data_block->rank > 0) { // Check if there are Dimensional Data to Receive
                        for (unsigned int i = 0; i < data_block->rank; i++) {
                            init_dim_block(&data_block->dims[i]);
                        }

                        if (!xdr_data_dim1(xdrs, data_block)) {
                            err = UDA_PROTOCOL_ERROR_63;
                            break;
                        }

                        if (protocolVersion < 3) {
                            for (unsigned int i = 0; i < data_block->rank; i++) {
                                auto dim = &data_block->dims[i];
                                if (protocol_version_type_test(protocolVersion, dim->data_type) ||
                                    protocol_version_type_test(protocolVersion, dim->error_type)) {
                                    err = UDA_PROTOCOL_ERROR_9999;
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
                            err = UDA_PROTOCOL_ERROR_64;
                            break;
                        }

                        for (unsigned int i = 0; i < data_block->rank; i++) { // Expand Compressed Regular Vector
                            err = uncompress_dim(&(data_block->dims[i]));     // Allocate Heap as required
                            err = 0;                                          // Need to Test for Error Condition!
                        }

                        if (!xdr_data_dim3(xdrs, data_block)) {
                            err = UDA_PROTOCOL_ERROR_65;
                            break;
                        }

                        if (!xdr_data_dim4(xdrs, data_block)) { // Asymmetric Errors
                            err = UDA_PROTOCOL_ERROR_65;
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
                        err = UDA_PROTOCOL_ERROR_9999;
                        break;
                    }

                    if (!xdr_data_block1(xdrs, data_block, protocolVersion)) {
                        err = UDA_PROTOCOL_ERROR_61;
                        break;
                    }

                    if (data_block->data_n == 0) { // No Data or Dimensions to Send!
                        if (!xdrrec_endofrecord(xdrs, 1)) {
                            err = UDA_PROTOCOL_ERROR_7;
                        }
                        break;
                    }

                    if (!xdr_data_block2(xdrs, data_block)) {
                        err = UDA_PROTOCOL_ERROR_62;
                        break;
                    }

                    if (data_block->error_type != UDA_TYPE_UNKNOWN ||
                        data_block->error_param_n > 0) { // Only Send if Error Data are available
                        if (!xdr_data_block3(xdrs, data_block)) {
                            err = UDA_PROTOCOL_ERROR_62;
                            break;
                        }
                        if (!xdr_data_block4(xdrs, data_block)) {
                            err = UDA_PROTOCOL_ERROR_62;
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
                                    err = UDA_PROTOCOL_ERROR_9999;
                                    break;
                                }
                            }
                        }

                        for (unsigned int i = 0; i < data_block->rank; i++) {
                            compress_dim(&(data_block->dims[i])); // Minimise Data Transfer if Regular
                        }

                        if (!xdr_data_dim1(xdrs, data_block)) {
                            err = UDA_PROTOCOL_ERROR_63;
                            break;
                        }

                        if (!xdr_data_dim2(xdrs, data_block)) {
                            err = UDA_PROTOCOL_ERROR_64;
                            break;
                        }

                        if (!xdr_data_dim3(xdrs, data_block)) {
                            err = UDA_PROTOCOL_ERROR_65;
                        }

                        if (!xdr_data_dim4(xdrs, data_block)) {
                            err = UDA_PROTOCOL_ERROR_65;
                        }
                    }

                    if (!xdrrec_endofrecord(xdrs, 1)) {
                        err = UDA_PROTOCOL_ERROR_7;
                    }

                    break;

                case XDRStreamDirection::FreeHeap:
                    break;

                default:
                    err = UDA_PROTOCOL_ERROR_4;
                    break;
            }
            break;
        }

        //----------------------------------------------------------------------------
        // Data Block

        if (protocol_id == ProtocolId::PutdataBlockList) {

            auto put_data_block_list = (PutDataBlockList*)str;
            PutDataBlock put_data;

            switch (direction) {

                case XDRStreamDirection::Receive: {

                    if (!xdrrec_skiprecord(xdrs)) {
                        err = UDA_PROTOCOL_ERROR_5;
                        break;
                    }

                    unsigned int blockCount = 0;
                    int rc = xdr_u_int(xdrs, &blockCount);

                    if (!rc) {
                        err = UDA_PROTOCOL_ERROR_61;
                        break;
                    }

                    UDA_LOG(UDA_LOG_DEBUG, "receive: putDataBlockList Count: {}", blockCount);

                    for (unsigned int i = 0; i < blockCount; i++) { // Fetch multiple put blocks

                        init_put_data_block(&put_data);

                        if (!xdr_putdata_block1(xdrs, &put_data)) {
                            err = UDA_PROTOCOL_ERROR_61;
                            UDA_LOG(UDA_LOG_DEBUG, "xdr_putdata_block1 Error (61)");
                            break;
                        }

                        if (protocol_version_type_test(protocolVersion, put_data.data_type)) {
                            err = UDA_PROTOCOL_ERROR_9999;
                            break;
                        }

                        if (put_data.count > 0 || put_data.blockNameLength > 0) {

                            if ((err = alloc_put_data(&put_data)) != 0) {
                                break; // Allocate Heap Memory
                            }

                            if (!xdr_putdata_block2(xdrs, &put_data)) { // Fetch data
                                err = UDA_PROTOCOL_ERROR_62;
                                break;
                            }

                            add_put_data_block_list(&put_data, put_data_block_list); // Add to the growing list
                        }
                    }

                    break;
                }

                case XDRStreamDirection::Send: {

                    UDA_LOG(UDA_LOG_DEBUG, "send: putDataBlockList Count: {}", put_data_block_list->blockCount);

                    int rc = xdr_u_int(xdrs, &(put_data_block_list->blockCount));

                    if (!rc) {
                        err = UDA_PROTOCOL_ERROR_61;
                        break;
                    }

                    for (unsigned int i = 0; i < put_data_block_list->blockCount; i++) { // Send multiple put blocks

                        if (!xdr_putdata_block1(xdrs, &(put_data_block_list->putDataBlock[i]))) {
                            err = UDA_PROTOCOL_ERROR_61;
                            break;
                        }

                        if (protocol_version_type_test(protocolVersion,
                                                       put_data_block_list->putDataBlock[i].data_type)) {
                            err = UDA_PROTOCOL_ERROR_9999;
                            break;
                        }

                        if (put_data_block_list->putDataBlock[i].count > 0 ||
                            put_data_block_list->putDataBlock[i].blockNameLength > 0) { // Data to Send?

                            if (!xdr_putdata_block2(xdrs, &(put_data_block_list->putDataBlock[i]))) {
                                err = UDA_PROTOCOL_ERROR_62;
                                break;
                            }
                        }
                    }

                    if (!xdrrec_endofrecord(xdrs, 1)) {
                        err = UDA_PROTOCOL_ERROR_7;
                    }

                    break;
                }

                case XDRStreamDirection::FreeHeap:
                    break;

                default:
                    err = UDA_PROTOCOL_ERROR_4;
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
                        err = UDA_PROTOCOL_ERROR_5;
                        break;
                    }
                    if (!xdr_int(xdrs, (int*)token)) {
                        err = UDA_PROTOCOL_ERROR_9;
                        break;
                    }
                    break;

                case XDRStreamDirection::Send:
                    if (!xdr_int(xdrs, (int*)token)) {
                        err = UDA_PROTOCOL_ERROR_9;
                        break;
                    }
                    if (!xdrrec_endofrecord(xdrs, 1)) {
                        err = UDA_PROTOCOL_ERROR_7;
                        break;
                    }
                    break;

                case XDRStreamDirection::FreeHeap:
                    err = UDA_PROTOCOL_ERROR_3;
                    break;

                default:
                    err = UDA_PROTOCOL_ERROR_4;
                    break;
            }

            break;
        }

        //----------------------------------------------------------------------------
        // Data System record

        if (protocol_id == ProtocolId::DataSystem) {
            auto data_system = (DataSystem*)str;

            switch (direction) {
                case XDRStreamDirection::Receive: // From Client to Server
                    if (!xdrrec_skiprecord(xdrs)) {
                        err = UDA_PROTOCOL_ERROR_5;
                        break;
                    }
                    if (!xdr_data_system(xdrs, data_system)) {
                        err = UDA_PROTOCOL_ERROR_10;
                        break;
                    }
                    break;

                case XDRStreamDirection::Send:

                    if (!xdr_data_system(xdrs, data_system)) {
                        err = UDA_PROTOCOL_ERROR_10;
                        break;
                    }
                    if (!xdrrec_endofrecord(xdrs, 1)) {
                        err = UDA_PROTOCOL_ERROR_7;
                        break;
                    }
                    break;

                case XDRStreamDirection::FreeHeap:

                    if (!xdr_data_system(xdrs, data_system)) {
                        err = UDA_PROTOCOL_ERROR_11;
                        break;
                    }
                    break;

                default:
                    err = UDA_PROTOCOL_ERROR_4;
                    break;
            }
            break;
        }

        //----------------------------------------------------------------------------
        // System Configuration record

        if (protocol_id == ProtocolId::SystemConfig) {

            auto system_config = (SystemConfig*)str;

            switch (direction) {
                case XDRStreamDirection::Receive:
                    // From Client to Server
                    if (!xdrrec_skiprecord(xdrs)) {
                        err = UDA_PROTOCOL_ERROR_5;
                        break;
                    }
                    if (!xdr_system_config(xdrs, system_config)) {
                        err = UDA_PROTOCOL_ERROR_12;
                        break;
                    }
                    break;

                case XDRStreamDirection::Send:

                    if (!xdr_system_config(xdrs, system_config)) {
                        err = UDA_PROTOCOL_ERROR_12;
                        break;
                    }
                    if (!xdrrec_endofrecord(xdrs, 1)) {
                        err = UDA_PROTOCOL_ERROR_7;
                        break;
                    }
                    break;

                case XDRStreamDirection::FreeHeap:

                    if (!xdr_system_config(xdrs, system_config)) {
                        err = UDA_PROTOCOL_ERROR_13;
                        break;
                    }
                    break;

                default:
                    err = UDA_PROTOCOL_ERROR_4;
                    break;
            }
            break;
        }

        //----------------------------------------------------------------------------
        // Data Source record

        if (protocol_id == ProtocolId::DataSource) {

            auto data_source = (DataSource*)str;

            switch (direction) {
                case XDRStreamDirection::Receive:
                    // From Client to Server
                    if (!xdrrec_skiprecord(xdrs)) {
                        err = UDA_PROTOCOL_ERROR_5;
                        break;
                    }
                    if (!xdr_data_source(xdrs, data_source)) {
                        err = UDA_PROTOCOL_ERROR_14;
                        break;
                    }
                    break;

                case XDRStreamDirection::Send:

                    if (!xdr_data_source(xdrs, data_source)) {
                        err = UDA_PROTOCOL_ERROR_14;
                        break;
                    }
                    if (!xdrrec_endofrecord(xdrs, 1)) {
                        err = UDA_PROTOCOL_ERROR_7;
                        break;
                    }
                    break;

                case XDRStreamDirection::FreeHeap:

                    if (!xdr_data_source(xdrs, data_source)) {
                        err = UDA_PROTOCOL_ERROR_15;
                        break;
                    }
                    break;

                default:
                    err = UDA_PROTOCOL_ERROR_4;
                    break;
            }
            break;
        }

        //----------------------------------------------------------------------------
        // Signal record

        if (protocol_id == ProtocolId::Signal) {

            auto signal = (Signal*)str;

            switch (direction) {

                case XDRStreamDirection::Receive:
                    // From Client to Server
                    if (!xdrrec_skiprecord(xdrs)) {
                        err = UDA_PROTOCOL_ERROR_5;
                        break;
                    }
                    if (!xdr_signal(xdrs, signal)) {
                        err = UDA_PROTOCOL_ERROR_16;
                        break;
                    }
                    break;

                case XDRStreamDirection::Send:

                    if (!xdr_signal(xdrs, signal)) {
                        err = UDA_PROTOCOL_ERROR_16;
                        break;
                    }
                    if (!xdrrec_endofrecord(xdrs, 1)) {
                        err = UDA_PROTOCOL_ERROR_7;
                        break;
                    }
                    break;

                case XDRStreamDirection::FreeHeap:

                    if (!xdr_signal(xdrs, signal)) {
                        err = UDA_PROTOCOL_ERROR_17;
                        break;
                    }
                    break;

                default:
                    err = UDA_PROTOCOL_ERROR_4;
                    break;
            }
            break;
        }

        //----------------------------------------------------------------------------
        // Signal Description record

        if (protocol_id == ProtocolId::SignalDesc) {

            auto signal_desc = (SignalDesc*)str;

            switch (direction) {

                case XDRStreamDirection::Receive:
                    // From Client to Server
                    if (!xdrrec_skiprecord(xdrs)) {
                        err = UDA_PROTOCOL_ERROR_5;
                        break;
                    }
                    if (!xdr_signal_desc(xdrs, signal_desc)) {
                        err = UDA_PROTOCOL_ERROR_18;
                        break;
                    }

                    break;

                case XDRStreamDirection::Send:

                    if (!xdr_signal_desc(xdrs, signal_desc)) {
                        err = UDA_PROTOCOL_ERROR_18;
                        break;
                    }
                    if (!xdrrec_endofrecord(xdrs, 1)) {
                        err = UDA_PROTOCOL_ERROR_7;
                        break;
                    }
                    break;

                case XDRStreamDirection::FreeHeap:

                    if (!xdr_signal_desc(xdrs, signal_desc)) {
                        err = UDA_PROTOCOL_ERROR_19;
                        break;
                    }
                    break;

                default:
                    err = UDA_PROTOCOL_ERROR_4;
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
                        err = UDA_PROTOCOL_ERROR_5;
                        break;
                    }
                    if (!xdr_client(xdrs, client_block, protocolVersion)) {
                        err = UDA_PROTOCOL_ERROR_20;
                        break;
                    }

                    break;

                case XDRStreamDirection::Send:

                    if (!xdr_client(xdrs, client_block, protocolVersion)) {
                        err = UDA_PROTOCOL_ERROR_20;
                        break;
                    }
                    if (!xdrrec_endofrecord(xdrs, 1)) {
                        err = UDA_PROTOCOL_ERROR_7;
                        break;
                    }
                    break;

                case XDRStreamDirection::FreeHeap:

                    if (!xdr_client(xdrs, client_block, protocolVersion)) {
                        err = UDA_PROTOCOL_ERROR_21;
                        break;
                    }
                    break;

                default:
                    err = UDA_PROTOCOL_ERROR_4;
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
                        err = UDA_PROTOCOL_ERROR_5;
                        break;
                    }

                    close_error(); // Free Heap associated with Previous Data Access

                    if (!xdr_server1(xdrs, server_block, protocolVersion)) {
                        err = UDA_PROTOCOL_ERROR_22;
                        break;
                    }

                    if (server_block->idamerrorstack.nerrors > 0) { // No Data to Receive?

                        server_block->idamerrorstack.idamerror =
                            (UdaError*)malloc(server_block->idamerrorstack.nerrors * sizeof(UdaError));
                        init_error_records(&server_block->idamerrorstack);

                        if (!xdr_server2(xdrs, server_block)) {
                            err = UDA_PROTOCOL_ERROR_22;
                            break;
                        }
                    }

                    break;

                case XDRStreamDirection::Send:
                    if (!xdr_server1(xdrs, server_block, protocolVersion)) {
                        err = UDA_PROTOCOL_ERROR_22;
                        break;
                    }

                    if (server_block->idamerrorstack.nerrors > 0) { // No Data to Send?
                        if (!xdr_server2(xdrs, server_block)) {
                            err = UDA_PROTOCOL_ERROR_22;
                            break;
                        }
                    }

                    if (!xdrrec_endofrecord(xdrs, 1)) {
                        err = UDA_PROTOCOL_ERROR_7;
                        break;
                    }
                    break;

                case XDRStreamDirection::FreeHeap:

                    if (!xdr_server(xdrs, server_block)) {
                        err = UDA_PROTOCOL_ERROR_23;
                        break;
                    }
                    break;

                default:
                    err = UDA_PROTOCOL_ERROR_4;
                    break;
            }

            break;
        }

        //----------------------------------------------------------------------------
        // Hierarchical or Meta Data Structures

        if (protocol_id > ProtocolId::OpaqueStart && protocol_id < ProtocolId::OpaqueStop) {
            err = protocol_xml(xdrs, protocol_id, direction, token, logmalloclist, userdefinedtypelist, str,
                               protocolVersion, log_struct_list, io_data, private_flags, malloc_source, nullptr);
        }

        //----------------------------------------------------------------------------
        // End of Error Trap Loop

    } while (0);

    return err;
}
