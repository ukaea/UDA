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

#include <uda/types.h>

#include "alloc_data.h"
#include "protocolXML2.h"
#include "protocolXML2Put.h"
#include "xdrlib.h"

#include "clientserver/compress_dim.h"
#include "clientserver/error_log.h"
#include "clientserver/init_structs.h"
#include "clientserver/print_structs.h"
#include "clientserver/uda_errors.h"
#include "logging/logging.h"

using namespace uda::protocol;
using namespace uda::logging;
using namespace uda::structures;
using namespace uda::client_server;

static int handle_request_block(XDR* xdrs, XDRStreamDirection direction, const void* str, int protocolVersion);
static int handle_data_block(XDR* xdrs, XDRStreamDirection direction, const void* str, int protocolVersion);
static int handle_data_block_list(XDR* xdrs, XDRStreamDirection direction, const void* str, int protocolVersion);
static int handle_putdata_block_list(std::vector<UdaError>& error_stack, XDR* xdrs, XDRStreamDirection direction,
                                     ProtocolId* token, LogMallocList* logmalloclist,
                                     UserDefinedTypeList* userdefinedtypelist, const void* str, int protocolVersion,
                                     LogStructList* log_struct_list, unsigned int private_flags, int malloc_source);
static int handle_next_protocol(XDR* xdrs, XDRStreamDirection direction, ProtocolId* token);
static int handle_client_block(XDR* xdrs, XDRStreamDirection direction, const void* str, int protocolVersion);
static int handle_server_block(XDR* xdrs, XDRStreamDirection direction, const void* str, int protocolVersion);
static int handle_dataobject(XDR* xdrs, XDRStreamDirection direction, const void* str);
static int handle_dataobject_file(XDRStreamDirection direction, const void* str);
static int handle_meta_data(XDR* xdrs, XDRStreamDirection direction, void* str);

int uda::protocol::protocol2(std::vector<UdaError>& error_stack, XDR* xdrs, ProtocolId protocol_id, XDRStreamDirection direction, ProtocolId* token, LogMallocList* logmalloclist,
                                  UserDefinedTypeList* userdefinedtypelist, void* str, int protocolVersion,
                                  LogStructList* log_struct_list, unsigned int private_flags, int malloc_source)
{
    int err = 0;

    switch (protocol_id) {
        case ProtocolId::RequestBlock:
            err = handle_request_block(xdrs, direction, str, protocolVersion);
            break;
        case ProtocolId::DataBlockList:
            err = handle_data_block_list(xdrs, direction, str, protocolVersion);
            break;
        case ProtocolId::PutdataBlockList:
            err = handle_putdata_block_list(error_stack, xdrs, direction, token, logmalloclist, userdefinedtypelist, str,
                                            protocolVersion, log_struct_list, private_flags, malloc_source);
            break;
        case ProtocolId::NextProtocol:
            err = handle_next_protocol(xdrs, direction, token);
            break;
        case ProtocolId::MetaData:
            err = handle_meta_data(xdrs, direction, str);
            break;
        case ProtocolId::ClientBlock:
            err = handle_client_block(xdrs, direction, str, protocolVersion);
            break;
        case ProtocolId::ServerBlock:
            err = handle_server_block(xdrs, direction, str, protocolVersion);
            break;
        case ProtocolId::DataObject:
            err = handle_dataobject(xdrs, direction, str);
            break;
        case ProtocolId::DataObjectFile:
            err = handle_dataobject_file(direction, str);

            break;
        default:
            if (protocol_id > ProtocolId::OpaqueStart && protocol_id < ProtocolId::OpaqueStop) {
                err = protocol_xml2(error_stack, xdrs, protocol_id, direction, token, logmalloclist, userdefinedtypelist, str,
                                    protocolVersion, log_struct_list, private_flags, malloc_source);
            }
    }

    return err;
}

static int handle_dataobject_file(XDRStreamDirection direction, const void* str)
{
    int err = 0;

    switch (direction) {
        case XDRStreamDirection::Receive:
            break;

        case XDRStreamDirection::Send:
            break;

        default:
            err = (int)ProtocolError::Error4;
            break;
    }

    return err;
}

static int handle_dataobject(XDR* xdrs, XDRStreamDirection direction, const void* str)
{
    int err = 0;
    auto data_object = (DataObject*)str;

    switch (direction) {
        case XDRStreamDirection::Receive:
            if (!xdr_data_object1(xdrs, data_object)) { // Storage requirements
                err = (int)ProtocolError::Error22;
                break;
            }
            if (data_object->objectSize > 0) {
                data_object->object = (char*)malloc(data_object->objectSize * sizeof(char));
            }
            if (data_object->hashLength > 0) {
                data_object->md = (char*)malloc(data_object->hashLength * sizeof(char));
            }
            if (!xdr_data_object2(xdrs, data_object)) {
                err = (int)ProtocolError::Error22;
                break;
            }
            break;

        case XDRStreamDirection::Send:

            if (!xdr_data_object1(xdrs, data_object)) {
                err = (int)ProtocolError::Error22;
                break;
            }
            if (!xdr_data_object2(xdrs, data_object)) {
                err = (int)ProtocolError::Error22;
                break;
            }
            break;

        default:
            err = (int)ProtocolError::Error4;
            break;
    }
    return err;
}

static int handle_server_block(XDR* xdrs, XDRStreamDirection direction, const void* str, int protocolVersion)
{
    int err = 0;
    auto server_block = (ServerBlock*)str;

    switch (direction) {
        case XDRStreamDirection::Receive:
            server_block->error_stack.clear();

            if (!xdr_server1(xdrs, server_block, protocolVersion)) {
                err = (int)ProtocolError::Error22;
                break;
            }

            if (!server_block->error_stack.empty()) { // No Data to Receive?
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

            break;

        case XDRStreamDirection::FreeHeap:
            break;

        default:
            err = (int)ProtocolError::Error4;
            break;
    }
    return err;
}

static int handle_client_block(XDR* xdrs, XDRStreamDirection direction, const void* str, int protocolVersion)
{
    int err = 0;
    auto client_block = (ClientBlock*)str;

    switch (direction) {
        case XDRStreamDirection::Receive:
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
            break;

        case XDRStreamDirection::FreeHeap:
            break;

        default:
            err = (int)ProtocolError::Error4;
            break;
    }
    return err;
}

static int handle_meta_data(XDR* xdrs, XDRStreamDirection direction, void* str) {
    int err = 0;
    auto* meta_data = static_cast<MetaData*>(str);
    switch (direction) {
        case XDRStreamDirection::Receive:
            if (!xdr_metadata(xdrs, meta_data)) {
                err = (int)ProtocolError::Error25;
                break;
            }
            break;
        case XDRStreamDirection::Send:
            if (!xdr_metadata(xdrs, meta_data)) {
                err = (int)ProtocolError::Error25;
                break;
            }
            break;

        case XDRStreamDirection::FreeHeap:
            break;

        default:
            err = static_cast<int>(ProtocolError::Error4);
            break;
    }
    return err;
}

static int handle_next_protocol(XDR* xdrs, XDRStreamDirection direction, ProtocolId* token)
{
    int err = 0;
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
    return err;
}

static int handle_putdata_block_list(std::vector<UdaError>& error_stack, XDR* xdrs, XDRStreamDirection direction,
                                     ProtocolId* token, LogMallocList* logmalloclist,
                                     UserDefinedTypeList* userdefinedtypelist, const void* str, int protocolVersion,
                                     LogStructList* log_struct_list, unsigned int private_flags, int malloc_source)
{
    int err = 0;
    auto putDataBlockList = (PutDataBlockList*)str;

    switch (direction) {

        case XDRStreamDirection::Receive: {
            unsigned int blockCount = 0;

            if (!xdr_u_int(xdrs, &blockCount)) {
                err = (int)ProtocolError::Error61;
                break;
            }

            UDA_LOG(UDA_LOG_DEBUG, "receive: putDataBlockList Count: {}", blockCount);

            for (unsigned int i = 0; i < blockCount; i++) {
                // Fetch multiple put blocks

                PutDataBlock put_data;
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

                if (put_data.count > 0 || put_data.blockNameLength > 0) { // Some data to receive?

                    if ((err = alloc_put_data(&put_data)) != 0) {
                        break; // Allocate Heap Memory
                    }

                    if (!xdr_putdata_block2(xdrs, &put_data)) { // Fetch data
                        err = (int)ProtocolError::Error62;
                        break;
                    }
                }

                if (put_data.data_type == UDA_TYPE_COMPOUND && put_data.opaque_type == UDA_OPAQUE_TYPE_STRUCTURES) {
                    // Structured Data

                    // Create a temporary DataBlock as the function's argument with structured data

                    // logmalloc list is automatically generated
                    // userdefinedtypelist is passed from the client
                    // NTree is automatically generated

                    auto data_block = (DataBlock*)malloc(sizeof(DataBlock));

                    // *** Add to malloclog and test to ensure it is freed after use ***

                    init_data_block(data_block);
                    data_block->opaque_type = UDA_OPAQUE_TYPE_STRUCTURES;
                    data_block->data_n = (int)put_data.count;         // This number (also rank and shape)
                    data_block->opaque_block = put_data.opaque_block; // User Defined Type

                    ProtocolId protocol_id = ProtocolId::Structures;
                    if ((err = protocol_xml2_put(error_stack, xdrs, protocol_id, direction, token, logmalloclist,
                                                 userdefinedtypelist, data_block, protocolVersion, log_struct_list,
                                                 private_flags, malloc_source)) != 0) {
                        // Fetch Structured data
                        break;
                    }

                    put_data.data = reinterpret_cast<char*>(data_block); // Compact memory block with structures
                    auto general_block = (GeneralBlock*)data_block->opaque_block;
                    put_data.opaque_block = general_block->userdefinedtype;
                }

                add_put_data_block_list(&put_data, putDataBlockList); // Add to the growing list
            }
            break;
        }

        case XDRStreamDirection::Send:

            UDA_LOG(UDA_LOG_DEBUG, "send: putDataBlockList Count: {}", putDataBlockList->blockCount);

            if (!xdr_u_int(xdrs, &(putDataBlockList->blockCount))) {
                err = (int)ProtocolError::Error61;
                break;
            }

            for (unsigned int i = 0; i < putDataBlockList->blockCount; i++) { // Send multiple put blocks

                if (!xdr_putdata_block1(xdrs, &(putDataBlockList->putDataBlock[i]))) {
                    err = (int)ProtocolError::Error61;
                    break;
                }

                if (protocol_version_type_test(protocolVersion, putDataBlockList->putDataBlock[i].data_type)) {
                    err = (int)ProtocolError::Error9999;
                    break;
                }

                if (putDataBlockList->putDataBlock[i].count > 0 ||
                    putDataBlockList->putDataBlock[i].blockNameLength > 0) { // Data to Send?

                    if (!xdr_putdata_block2(xdrs, &(putDataBlockList->putDataBlock[i]))) {
                        err = (int)ProtocolError::Error62;
                        break;
                    }
                }

                if (putDataBlockList->putDataBlock[i].data_type == UDA_TYPE_COMPOUND &&
                    putDataBlockList->putDataBlock[i].opaque_type == UDA_OPAQUE_TYPE_STRUCTURES) {
                    // Structured Data

                    // Create a temporary DataBlock as the function's argument with structured data

                    //   *** putdata.opaque_count is not used or needed - count is sufficient

                    DataBlock data_block;
                    init_data_block(&data_block);
                    data_block.opaque_type = UDA_OPAQUE_TYPE_STRUCTURES;
                    data_block.data_n =
                        (int)putDataBlockList->putDataBlock[i].count; // This number (also rank and shape)
                    data_block.opaque_block = putDataBlockList->putDataBlock[i].opaque_block; // User Defined Type
                    data_block.data =
                        (char*)putDataBlockList->putDataBlock[i].data; // Compact memory block with structures

                    ProtocolId protocol_id = ProtocolId::Structures;
                    if ((err = protocol_xml2_put(error_stack, xdrs, protocol_id, direction, token, logmalloclist,
                                                 userdefinedtypelist, &data_block, protocolVersion, log_struct_list,
                                                 private_flags, malloc_source)) != 0) {
                        // Send Structured data
                        break;
                    }
                }
            }
            break;

        case XDRStreamDirection::FreeHeap:
            break;

        default:
            err = (int)ProtocolError::Error4;
            break;
    }
    return err;
}

static int handle_data_block(XDR* xdrs, XDRStreamDirection direction, const void* str, int protocolVersion)
{
    int err = 0;
    auto data_block = (DataBlock*)str;

    switch (direction) {
        case XDRStreamDirection::Receive: {
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
                        Dims* dim = &data_block->dims[i];
                        if (protocol_version_type_test(protocolVersion, dim->data_type) ||
                            protocol_version_type_test(protocolVersion, dim->error_type)) {
                            err = (int)ProtocolError::Error9999;
                            break;
                        }
                    }
                    if (err != 0) {
                        break;
                    }
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
        }

        case XDRStreamDirection::Send: {

            // Check client/server understands new data types

            // direction == XDRStreamDirection::Send && protocolVersion == 3 Means Server sending data to a Version 3 Client (Type is
            // known)

            UDA_LOG(UDA_LOG_DEBUG, "#1 PROTOCOL: Send/Receive Data Block");
            print_data_block(*data_block);

            if (protocol_version_type_test(protocolVersion, data_block->data_type) ||
                protocol_version_type_test(protocolVersion, data_block->error_type)) {
                err = (int)ProtocolError::Error9999;
                UDA_LOG(UDA_LOG_DEBUG, "PROTOCOL: protocolVersionTypeTest Failed");

                break;
            }
            UDA_LOG(UDA_LOG_DEBUG, "#2 PROTOCOL: Send/Receive Data Block");
            if (!xdr_data_block1(xdrs, data_block, protocolVersion)) {
                err = (int)ProtocolError::Error61;
                break;
            }

            if (data_block->data_n == 0) { // No Data or Dimensions to Send!
                break;
            }

            if (!xdr_data_block2(xdrs, data_block)) {
                err = (int)ProtocolError::Error62;
                break;
            }

            if (data_block->error_type != UDA_TYPE_UNKNOWN || data_block->error_param_n > 0) {
                // Only Send if Error Data are available
                if (!xdr_data_block3(xdrs, data_block)) {
                    err = (int)ProtocolError::Error62;
                    break;
                }
                if (!xdr_data_block4(xdrs, data_block)) {
                    err = (int)ProtocolError::Error62;
                    break;
                }
            }

            if (data_block->rank > 0) {
                // Dimensional Data to Send

                // Check client/server understands new data types

                if (protocolVersion < 3) {
                    for (unsigned int i = 0; i < data_block->rank; i++) {
                        Dims* dim = &data_block->dims[i];
                        if (protocol_version_type_test(protocolVersion, dim->data_type) ||
                            protocol_version_type_test(protocolVersion, dim->error_type)) {
                            err = (int)ProtocolError::Error9999;
                            break;
                        }
                    }
                    if (err != 0) {
                        break;
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
            break;
        }

        case XDRStreamDirection::FreeHeap:
            break;

        default:
            err = (int)ProtocolError::Error4;
            break;
    }

    return err;
}

static int handle_data_block_list(XDR* xdrs, XDRStreamDirection direction, const void* str, int protocolVersion)
{
    int err = 0;
    auto data_block_list = (std::vector<DataBlock>*)str;

    switch (direction) {
        case XDRStreamDirection::Receive:
            if (!xdr_data_block_list(xdrs, data_block_list, protocolVersion)) {
                err = static_cast<int>(ProtocolError::Error1);
                break;
            }
            for (size_t i = 0; i < data_block_list->size(); ++i) {
                DataBlock* data_block = &(*data_block_list)[i];
                init_data_block(data_block);
                err = handle_data_block(xdrs, XDRStreamDirection::Receive, data_block, protocolVersion);
                if (err != 0) {
                    err = static_cast<int>(ProtocolError::Error2);
                    break;
                }
            }
            if (err) {
                break;
            }
            break;

        case XDRStreamDirection::Send: {
            if (!xdr_data_block_list(xdrs, data_block_list, protocolVersion)) {
                err = static_cast<int>(ProtocolError::Error2);
                break;
            }
            for (size_t i = 0; i < data_block_list->size(); ++i) {
                const DataBlock* data_block = &(*data_block_list)[i];
                int rc = handle_data_block(xdrs, XDRStreamDirection::Send, data_block, protocolVersion);
                if (rc != 0) {
                    err = static_cast<int>(ProtocolError::Error2);
                    break;
                }
            }
            if (err) {
                break;
            }
            break;
        }

        case XDRStreamDirection::FreeHeap:
            break;

        default:
            err = static_cast<int>(ProtocolError::Error4);
            break;
    }
    return err;
}

static int handle_request_block(XDR* xdrs, XDRStreamDirection direction, const void* str, int protocolVersion)
{
    int err = 0;
    auto request_block = (RequestBlock*)str;

    switch (direction) {
        case XDRStreamDirection::Receive:
            if (!xdr_request(xdrs, request_block, protocolVersion)) {
                err = (int)ProtocolError::Error1;
                break;
            }
            request_block->requests = (RequestData*)malloc(request_block->num_requests * sizeof(RequestData));
            for (int i = 0; i < request_block->num_requests; ++i) {
                init_request_data(&request_block->requests[i]);
                if (!xdr_request_data(xdrs, &request_block->requests[i], protocolVersion)) {
                    err = (int)ProtocolError::Error2;
                    break;
                }
            }
            if (err) {
                break;
            }
            break;

        case XDRStreamDirection::Send:
            if (!xdr_request(xdrs, request_block, protocolVersion)) {
                err = (int)ProtocolError::Error2;
                break;
            }
            for (int i = 0; i < request_block->num_requests; ++i) {
                if (!xdr_request_data(xdrs, &request_block->requests[i], protocolVersion)) {
                    err = (int)ProtocolError::Error2;
                    break;
                }
            }
            if (err) {
                break;
            }
            break;

        case XDRStreamDirection::FreeHeap:
            break;

        default:
            err = (int)ProtocolError::Error4;
            break;
    }
    return err;
}
