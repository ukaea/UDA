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

#include "logging/logging.h"
#include <uda/types.h>

#include "allocData.h"
#include "compressDim.h"
#include "initStructs.h"
#include "printStructs.h"
#include "protocolXML2.h"
#include "xdrlib.h"

#include "errorLog.h"
#include "protocolXML2Put.h"
#include "udaErrors.h"

using namespace uda::client_server;
using namespace uda::logging;
using namespace uda::structures;

static int handle_request_block(XDR* xdrs, int direction, const void* str, int protocolVersion);
static int handle_data_block(XDR* xdrs, int direction, const void* str, int protocolVersion);
static int handle_data_block_list(XDR* xdrs, int direction, const void* str, int protocolVersion);
static int handle_putdata_block_list(XDR* xdrs, int direction, int* token, LogMallocList* logmalloclist,
                                     UserDefinedTypeList* userdefinedtypelist, const void* str, int protocolVersion,
                                     LogStructList* log_struct_list, unsigned int private_flags, int malloc_source);
static int handle_next_protocol(XDR* xdrs, int direction, int* token);
static int handle_data_system(XDR* xdrs, int direction, const void* str);
static int handle_system_config(XDR* xdrs, int direction, const void* str);
static int handle_data_source(XDR* xdrs, int direction, const void* str);
static int handle_signal(XDR* xdrs, int direction, const void* str);
static int handle_signal_desc(XDR* xdrs, int direction, const void* str);
static int handle_client_block(XDR* xdrs, int direction, const void* str, int protocolVersion);
static int handle_server_block(XDR* xdrs, int direction, const void* str, int protocolVersion);
static int handle_dataobject(XDR* xdrs, int direction, const void* str);
static int handle_dataobject_file(int direction, const void* str);

#ifdef SECURITYENABLED
static int handle_security_block(XDR* xdrs, int direction, const void* str);
#endif

int uda::client_server::protocol2(XDR* xdrs, int protocol_id, int direction, int* token, LogMallocList* logmalloclist,
                                  UserDefinedTypeList* userdefinedtypelist, void* str, int protocolVersion,
                                  LogStructList* log_struct_list, unsigned int private_flags, int malloc_source)
{
    int err = 0;

    switch (protocol_id) {
        case UDA_PROTOCOL_REQUEST_BLOCK:
            err = handle_request_block(xdrs, direction, str, protocolVersion);
            break;
        case UDA_PROTOCOL_DATA_BLOCK_LIST:
            err = handle_data_block_list(xdrs, direction, str, protocolVersion);
            break;
        case UDA_PROTOCOL_PUTDATA_BLOCK_LIST:
            err = handle_putdata_block_list(xdrs, direction, token, logmalloclist, userdefinedtypelist, str,
                                            protocolVersion, log_struct_list, private_flags, malloc_source);
            break;
        case UDA_PROTOCOL_NEXT_PROTOCOL:
            err = handle_next_protocol(xdrs, direction, token);
            break;
        case UDA_PROTOCOL_DATA_SYSTEM:
            err = handle_data_system(xdrs, direction, str);
            break;
        case UDA_PROTOCOL_SYSTEM_CONFIG:
            err = handle_system_config(xdrs, direction, str);
            break;
        case UDA_PROTOCOL_DATA_SOURCE:
            err = handle_data_source(xdrs, direction, str);
            break;
        case UDA_PROTOCOL_SIGNAL:
            err = handle_signal(xdrs, direction, str);
            break;
        case UDA_PROTOCOL_SIGNAL_DESC:
            err = handle_signal_desc(xdrs, direction, str);
            break;
#ifdef SECURITYENABLED
        case UDA_PROTOCOL_SECURITY_BLOCK:
            err = handle_security_block(xdrs, direction, str);
            break;
#endif
        case UDA_PROTOCOL_CLIENT_BLOCK:
            err = handle_client_block(xdrs, direction, str, protocolVersion);
            break;
        case UDA_PROTOCOL_SERVER_BLOCK:
            err = handle_server_block(xdrs, direction, str, protocolVersion);
            break;
        case UDA_PROTOCOL_DATAOBJECT:
            err = handle_dataobject(xdrs, direction, str);
            break;
        case UDA_PROTOCOL_DATAOBJECT_FILE:
            err = handle_dataobject_file(direction, str);

            break;
        default:
            if (protocol_id > UDA_PROTOCOL_OPAQUE_START && protocol_id < UDA_PROTOCOL_OPAQUE_STOP) {
                err = protocol_xml2(xdrs, protocol_id, direction, token, logmalloclist, userdefinedtypelist, str,
                                    protocolVersion, log_struct_list, private_flags, malloc_source);
            }
    }

    return err;
}

#ifdef SECURITYENABLED
static int handle_security_block(XDR* xdrs, int direction, const void* str)
{
    int err = 0;
    ClientBlock* client_block = (ClientBlock*)str;
    SecurityBlock* security_block = &(client_block->securityBlock);

    switch (direction) {
        case XDR_RECEIVE:
            if (!xdr_security_block1(xdrs, security_block)) {
                err = UDA_PROTOCOL_ERROR_23;
                break;
            }
            break;

        case XDR_SEND:
            if (!xdr_security_block1(xdrs, security_block)) {
                err = UDA_PROTOCOL_ERROR_23;
                break;
            }
            break;

        case XDR_FREE_HEAP:
            break;

        default:
            err = UDA_PROTOCOL_ERROR_4;
            break;
    }

    // Allocate heap

    if (security_block->client_ciphertextLength > 0) {
        security_block->client_ciphertext =
            (unsigned char*)malloc(security_block->client_ciphertextLength * sizeof(unsigned char));
    }
    if (security_block->client2_ciphertextLength > 0) {
        security_block->client2_ciphertext =
            (unsigned char*)malloc(security_block->client2_ciphertextLength * sizeof(unsigned char));
    }
    if (security_block->server_ciphertextLength > 0) {
        security_block->server_ciphertext =
            (unsigned char*)malloc(security_block->server_ciphertextLength * sizeof(unsigned char));
    }
    if (security_block->client_X509Length > 0) {
        security_block->client_X509 = (unsigned char*)malloc(security_block->client_X509Length * sizeof(unsigned char));
    }
    if (security_block->client2_X509Length > 0) {
        security_block->client2_X509 =
            (unsigned char*)malloc(security_block->client2_X509Length * sizeof(unsigned char));
    }

    switch (direction) {
        case XDR_RECEIVE:
            if (!xdr_security_block2(xdrs, security_block)) {
                err = UDA_PROTOCOL_ERROR_24;
                break;
            }
            break;

        case XDR_SEND:
            if (!xdr_security_block2(xdrs, security_block)) {
                err = UDA_PROTOCOL_ERROR_24;
                break;
            }
            break;

        case XDR_FREE_HEAP:
            break;

        default:
            err = UDA_PROTOCOL_ERROR_4;
            break;
    }
    return err;
}
#endif // SECURITYENABLED

static int handle_dataobject_file(int direction, const void* str)
{
    int err = 0;

    switch (direction) {
        case XDR_RECEIVE:
            break;

        case XDR_SEND:
            break;

        default:
            err = UDA_PROTOCOL_ERROR_4;
            break;
    }

    return err;
}

static int handle_dataobject(XDR* xdrs, int direction, const void* str)
{
    int err = 0;
    auto data_object = (DataObject*)str;

    switch (direction) {
        case XDR_RECEIVE:
            if (!xdr_data_object1(xdrs, data_object)) { // Storage requirements
                err = UDA_PROTOCOL_ERROR_22;
                break;
            }
            if (data_object->objectSize > 0) {
                data_object->object = (char*)malloc(data_object->objectSize * sizeof(char));
            }
            if (data_object->hashLength > 0) {
                data_object->md = (char*)malloc(data_object->hashLength * sizeof(char));
            }
            if (!xdr_data_object2(xdrs, data_object)) {
                err = UDA_PROTOCOL_ERROR_22;
                break;
            }
            break;

        case XDR_SEND:

            if (!xdr_data_object1(xdrs, data_object)) {
                err = UDA_PROTOCOL_ERROR_22;
                break;
            }
            if (!xdr_data_object2(xdrs, data_object)) {
                err = UDA_PROTOCOL_ERROR_22;
                break;
            }
            break;

        default:
            err = UDA_PROTOCOL_ERROR_4;
            break;
    }
    return err;
}

static int handle_server_block(XDR* xdrs, int direction, const void* str, int protocolVersion)
{
    int err = 0;
    auto server_block = (ServerBlock*)str;

    switch (direction) {
        case XDR_RECEIVE:
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

        case XDR_SEND:
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

            break;

        case XDR_FREE_HEAP:
            break;

        default:
            err = UDA_PROTOCOL_ERROR_4;
            break;
    }
    return err;
}

static int handle_client_block(XDR* xdrs, int direction, const void* str, int protocolVersion)
{
    int err = 0;
    auto client_block = (ClientBlock*)str;

    switch (direction) {
        case XDR_RECEIVE:
            if (!xdr_client(xdrs, client_block, protocolVersion)) {
                err = UDA_PROTOCOL_ERROR_20;
                break;
            }
            break;

        case XDR_SEND:
            if (!xdr_client(xdrs, client_block, protocolVersion)) {
                err = UDA_PROTOCOL_ERROR_20;
                break;
            }
            break;

        case XDR_FREE_HEAP:
            break;

        default:
            err = UDA_PROTOCOL_ERROR_4;
            break;
    }
    return err;
}

static int handle_signal_desc(XDR* xdrs, int direction, const void* str)
{
    int err = 0;
    auto signal_desc = (SignalDesc*)str;

    switch (direction) {
        case XDR_RECEIVE:
            if (!xdr_signal_desc(xdrs, signal_desc)) {
                err = UDA_PROTOCOL_ERROR_18;
                break;
            }
            break;

        case XDR_SEND:
            if (!xdr_signal_desc(xdrs, signal_desc)) {
                err = UDA_PROTOCOL_ERROR_18;
                break;
            }
            break;

        case XDR_FREE_HEAP:
            break;

        default:
            err = UDA_PROTOCOL_ERROR_4;
            break;
    }
    return err;
}

static int handle_signal(XDR* xdrs, int direction, const void* str)
{
    int err = 0;
    auto signal = (Signal*)str;

    switch (direction) {
        case XDR_RECEIVE:
            if (!xdr_signal(xdrs, signal)) {
                err = UDA_PROTOCOL_ERROR_16;
                break;
            }
            break;

        case XDR_SEND:
            if (!xdr_signal(xdrs, signal)) {
                err = UDA_PROTOCOL_ERROR_16;
                break;
            }
            break;

        case XDR_FREE_HEAP:
            break;

        default:
            err = UDA_PROTOCOL_ERROR_4;
            break;
    }
    return err;
}

static int handle_data_source(XDR* xdrs, int direction, const void* str)
{
    int err = 0;
    auto data_source = (DataSource*)str;

    switch (direction) {

        case XDR_RECEIVE:
            if (!xdr_data_source(xdrs, data_source)) {
                err = UDA_PROTOCOL_ERROR_14;
                break;
            }
            break;

        case XDR_SEND:
            if (!xdr_data_source(xdrs, data_source)) {
                err = UDA_PROTOCOL_ERROR_14;
                break;
            }
            break;

        case XDR_FREE_HEAP:
            break;

        default:
            err = UDA_PROTOCOL_ERROR_4;
            break;
    }
    return err;
}

static int handle_system_config(XDR* xdrs, int direction, const void* str)
{
    int err = 0;
    auto system_config = (SystemConfig*)str;

    switch (direction) {
        case XDR_RECEIVE:
            if (!xdr_system_config(xdrs, system_config)) {
                err = UDA_PROTOCOL_ERROR_12;
                break;
            }
            break;

        case XDR_SEND:
            if (!xdr_system_config(xdrs, system_config)) {
                err = UDA_PROTOCOL_ERROR_12;
                break;
            }
            break;

        case XDR_FREE_HEAP:
            break;

        default:
            err = UDA_PROTOCOL_ERROR_4;
            break;
    }
    return err;
}

static int handle_data_system(XDR* xdrs, int direction, const void* str)
{
    int err = 0;
    auto data_system = (DataSystem*)str;

    switch (direction) {
        case XDR_RECEIVE:
            if (!xdr_data_system(xdrs, data_system)) {
                err = UDA_PROTOCOL_ERROR_10;
                break;
            }
            break;

        case XDR_SEND:
            if (!xdr_data_system(xdrs, data_system)) {
                err = UDA_PROTOCOL_ERROR_10;
                break;
            }
            break;

        case XDR_FREE_HEAP:
            break;

        default:
            err = UDA_PROTOCOL_ERROR_4;
            break;
    }
    return err;
}

static int handle_next_protocol(XDR* xdrs, int direction, int* token)
{
    int err = 0;
    switch (direction) {
        case XDR_RECEIVE: // From Client to Server
            if (!xdrrec_skiprecord(xdrs)) {
                err = UDA_PROTOCOL_ERROR_5;
                break;
            }
            if (!xdr_int(xdrs, token)) {
                err = UDA_PROTOCOL_ERROR_9;
                break;
            }
            break;

        case XDR_SEND:
            if (!xdr_int(xdrs, token)) {
                err = UDA_PROTOCOL_ERROR_9;
                break;
            }
            if (!xdrrec_endofrecord(xdrs, 1)) {
                err = UDA_PROTOCOL_ERROR_7;
                break;
            }
            break;

        case XDR_FREE:
            err = UDA_PROTOCOL_ERROR_3;
            break;

        default:
            err = UDA_PROTOCOL_ERROR_4;
            break;
    }
    return err;
}

static int handle_putdata_block_list(XDR* xdrs, int direction, int* token, LogMallocList* logmalloclist,
                                     UserDefinedTypeList* userdefinedtypelist, const void* str, int protocolVersion,
                                     LogStructList* log_struct_list, unsigned int private_flags, int malloc_source)
{
    int err = 0;
    auto putDataBlockList = (PutDataBlockList*)str;

    switch (direction) {

        case XDR_RECEIVE: {
            unsigned int blockCount = 0;

            if (!xdr_u_int(xdrs, &blockCount)) {
                err = UDA_PROTOCOL_ERROR_61;
                break;
            }

            UDA_LOG(UDA_LOG_DEBUG, "receive: putDataBlockList Count: %d\n", blockCount);

            for (unsigned int i = 0; i < blockCount; i++) {
                // Fetch multiple put blocks

                PutDataBlock put_data;
                init_put_data_block(&put_data);

                if (!xdr_putdata_block1(xdrs, &put_data)) {
                    err = UDA_PROTOCOL_ERROR_61;
                    UDA_LOG(UDA_LOG_DEBUG, "xdr_putdata_block1 Error (61)\n");
                    break;
                }

                if (protocol_version_type_test(protocolVersion, put_data.data_type)) {
                    err = UDA_PROTOCOL_ERROR_9999;
                    break;
                }

                if (put_data.count > 0 || put_data.blockNameLength > 0) { // Some data to receive?

                    if ((err = alloc_put_data(&put_data)) != 0) {
                        break; // Allocate Heap Memory
                    }

                    if (!xdr_putdata_block2(xdrs, &put_data)) { // Fetch data
                        err = UDA_PROTOCOL_ERROR_62;
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

                    int protocol_id = UDA_PROTOCOL_STRUCTURES;
                    if ((err = protocol_xml2_put(xdrs, protocol_id, direction, token, logmalloclist,
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

        case XDR_SEND:

            UDA_LOG(UDA_LOG_DEBUG, "send: putDataBlockList Count: %d\n", putDataBlockList->blockCount);

            if (!xdr_u_int(xdrs, &(putDataBlockList->blockCount))) {
                err = UDA_PROTOCOL_ERROR_61;
                break;
            }

            for (unsigned int i = 0; i < putDataBlockList->blockCount; i++) { // Send multiple put blocks

                if (!xdr_putdata_block1(xdrs, &(putDataBlockList->putDataBlock[i]))) {
                    err = UDA_PROTOCOL_ERROR_61;
                    break;
                }

                if (protocol_version_type_test(protocolVersion, putDataBlockList->putDataBlock[i].data_type)) {
                    err = UDA_PROTOCOL_ERROR_9999;
                    break;
                }

                if (putDataBlockList->putDataBlock[i].count > 0 ||
                    putDataBlockList->putDataBlock[i].blockNameLength > 0) { // Data to Send?

                    if (!xdr_putdata_block2(xdrs, &(putDataBlockList->putDataBlock[i]))) {
                        err = UDA_PROTOCOL_ERROR_62;
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

                    int protocol_id = UDA_PROTOCOL_STRUCTURES;
                    if ((err = protocol_xml2_put(xdrs, protocol_id, direction, token, logmalloclist,
                                                 userdefinedtypelist, &data_block, protocolVersion, log_struct_list,
                                                 private_flags, malloc_source)) != 0) {
                        // Send Structured data
                        break;
                    }
                }
            }
            break;

        case XDR_FREE_HEAP:
            break;

        default:
            err = UDA_PROTOCOL_ERROR_4;
            break;
    }
    return err;
}

static int handle_data_block(XDR* xdrs, int direction, const void* str, int protocolVersion)
{
    int err = 0;
    auto data_block = (DataBlock*)str;

    switch (direction) {
        case XDR_RECEIVE: {
            if (!xdr_data_block1(xdrs, data_block, protocolVersion)) {
                err = UDA_PROTOCOL_ERROR_61;
                break;
            }

            // Check client/server understands new data types
            // direction == XDR_RECEIVE && protocolVersion == 3 Means Client receiving data from a
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
                        Dims* dim = &data_block->dims[i];
                        if (protocol_version_type_test(protocolVersion, dim->data_type) ||
                            protocol_version_type_test(protocolVersion, dim->error_type)) {
                            err = UDA_PROTOCOL_ERROR_9999;
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
        }

        case XDR_SEND: {

            // Check client/server understands new data types

            // direction == XDR_SEND && protocolVersion == 3 Means Server sending data to a Version 3 Client (Type is
            // known)

            UDA_LOG(UDA_LOG_DEBUG, "#1 PROTOCOL: Send/Receive Data Block\n");
            print_data_block(*data_block);

            if (protocol_version_type_test(protocolVersion, data_block->data_type) ||
                protocol_version_type_test(protocolVersion, data_block->error_type)) {
                err = UDA_PROTOCOL_ERROR_9999;
                UDA_LOG(UDA_LOG_DEBUG, "PROTOCOL: protocolVersionTypeTest Failed\n");

                break;
            }
            UDA_LOG(UDA_LOG_DEBUG, "#2 PROTOCOL: Send/Receive Data Block\n");
            if (!xdr_data_block1(xdrs, data_block, protocolVersion)) {
                err = UDA_PROTOCOL_ERROR_61;
                break;
            }

            if (data_block->data_n == 0) { // No Data or Dimensions to Send!
                break;
            }

            if (!xdr_data_block2(xdrs, data_block)) {
                err = UDA_PROTOCOL_ERROR_62;
                break;
            }

            if (data_block->error_type != UDA_TYPE_UNKNOWN || data_block->error_param_n > 0) {
                // Only Send if Error Data are available
                if (!xdr_data_block3(xdrs, data_block)) {
                    err = UDA_PROTOCOL_ERROR_62;
                    break;
                }
                if (!xdr_data_block4(xdrs, data_block)) {
                    err = UDA_PROTOCOL_ERROR_62;
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
                            err = UDA_PROTOCOL_ERROR_9999;
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
            break;
        }

        case XDR_FREE_HEAP:
            break;

        default:
            err = UDA_PROTOCOL_ERROR_4;
            break;
    }

    return err;
}

static int handle_data_block_list(XDR* xdrs, int direction, const void* str, int protocolVersion)
{
    int err = 0;
    auto data_block_list = (DataBlockList*)str;

    switch (direction) {
        case XDR_RECEIVE:
            if (!xdr_data_block_list(xdrs, data_block_list, protocolVersion)) {
                err = UDA_PROTOCOL_ERROR_1;
                break;
            }
            data_block_list->data = (DataBlock*)malloc(data_block_list->count * sizeof(DataBlock));
            for (int i = 0; i < data_block_list->count; ++i) {
                DataBlock* data_block = &data_block_list->data[i];
                init_data_block(data_block);
                err = handle_data_block(xdrs, XDR_RECEIVE, data_block, protocolVersion);
                if (err != 0) {
                    err = UDA_PROTOCOL_ERROR_2;
                    break;
                }
            }
            if (err) {
                break;
            }
            break;

        case XDR_SEND: {
            if (!xdr_data_block_list(xdrs, data_block_list, protocolVersion)) {
                err = UDA_PROTOCOL_ERROR_2;
                break;
            }
            for (int i = 0; i < data_block_list->count; ++i) {
                DataBlock* data_block = &data_block_list->data[i];
                int rc = handle_data_block(xdrs, XDR_SEND, data_block, protocolVersion);
                if (rc != 0) {
                    err = UDA_PROTOCOL_ERROR_2;
                    break;
                }
            }
            if (err) {
                break;
            }
            break;
        }

        case XDR_FREE_HEAP:
            break;

        default:
            err = UDA_PROTOCOL_ERROR_4;
            break;
    }
    return err;
}

static int handle_request_block(XDR* xdrs, int direction, const void* str, int protocolVersion)
{
    int err = 0;
    auto request_block = (RequestBlock*)str;

    switch (direction) {
        case XDR_RECEIVE:
            if (!xdr_request(xdrs, request_block, protocolVersion)) {
                err = UDA_PROTOCOL_ERROR_1;
                break;
            }
            request_block->requests = (RequestData*)malloc(request_block->num_requests * sizeof(RequestData));
            for (int i = 0; i < request_block->num_requests; ++i) {
                init_request_data(&request_block->requests[i]);
                if (!xdr_request_data(xdrs, &request_block->requests[i], protocolVersion)) {
                    err = UDA_PROTOCOL_ERROR_2;
                    break;
                }
            }
            if (err) {
                break;
            }
            break;

        case XDR_SEND:
            if (!xdr_request(xdrs, request_block, protocolVersion)) {
                err = UDA_PROTOCOL_ERROR_2;
                break;
            }
            for (int i = 0; i < request_block->num_requests; ++i) {
                if (!xdr_request_data(xdrs, &request_block->requests[i], protocolVersion)) {
                    err = UDA_PROTOCOL_ERROR_2;
                    break;
                }
            }
            if (err) {
                break;
            }
            break;

        case XDR_FREE_HEAP:
            break;

        default:
            err = UDA_PROTOCOL_ERROR_4;
            break;
    }
    return err;
}
