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
#include "logging/logging.h"

#include "allocData.h"
#include "compressDim.h"
#include "initStructs.h"
#include "printStructs.h"
#include "protocolXML2.h"
#include "xdrlib.h"

#ifdef HIERARCHICAL_DATA
#  include "idamclientserverxml.h" // legacy
#endif

#include "errorLog.h"
#include "protocolXML2Put.h"
#include "udaErrors.h"

static int handle_request_block(XDR* xdrs, int direction, const void* str, int protocolVersion);
static int handle_data_block(XDR* xdrs, int direction, const void* str, int protocolVersion);
static int handle_data_block_list(XDR* xdrs, int direction, const void* str, int protocolVersion);
static int handle_putdata_block_list(XDR* xdrs, int direction, int* token, LOGMALLOCLIST* logmalloclist,
                                     USERDEFINEDTYPELIST* userdefinedtypelist, const void* str, int protocolVersion,
                                     LOGSTRUCTLIST* log_struct_list, unsigned int private_flags, int malloc_source);
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

int protocol2(XDR* xdrs, int protocol_id, int direction, int* token, LOGMALLOCLIST* logmalloclist,
              USERDEFINEDTYPELIST* userdefinedtypelist, void* str, int protocolVersion, LOGSTRUCTLIST* log_struct_list,
              unsigned int private_flags, int malloc_source)
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
                err = protocolXML2(xdrs, protocol_id, direction, token, logmalloclist, userdefinedtypelist, str,
                                   protocolVersion, log_struct_list, private_flags, malloc_source);
            }
    }

    return err;
}

#ifdef SECURITYENABLED
static int handle_security_block(XDR* xdrs, int direction, const void* str)
{
    int err = 0;
    CLIENT_BLOCK* client_block = (CLIENT_BLOCK*)str;
    SECURITY_BLOCK* security_block = &(client_block->securityBlock);

    switch (direction) {
        case XDR_RECEIVE:
            if (!xdr_securityBlock1(xdrs, security_block)) {
                err = UDA_PROTOCOL_ERROR_23;
                break;
            }
            break;

        case XDR_SEND:
            if (!xdr_securityBlock1(xdrs, security_block)) {
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
            if (!xdr_securityBlock2(xdrs, security_block)) {
                err = UDA_PROTOCOL_ERROR_24;
                break;
            }
            break;

        case XDR_SEND:
            if (!xdr_securityBlock2(xdrs, security_block)) {
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
    auto data_object = (DATA_OBJECT*)str;

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
    auto server_block = (SERVER_BLOCK*)str;

    switch (direction) {
        case XDR_RECEIVE:
            udaCloseError(); // Free Heap associated with Previous Data Access

            if (!xdr_server1(xdrs, server_block, protocolVersion)) {
                err = UDA_PROTOCOL_ERROR_22;
                break;
            }

            if (server_block->idamerrorstack.nerrors > 0) { // No Data to Receive?

                server_block->idamerrorstack.idamerror =
                    (UDA_ERROR*)malloc(server_block->idamerrorstack.nerrors * sizeof(UDA_ERROR));
                initErrorRecords(&server_block->idamerrorstack);

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
    auto client_block = (CLIENT_BLOCK*)str;

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
    auto signal_desc = (SIGNAL_DESC*)str;

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
    auto signal = (SIGNAL*)str;

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
    auto data_source = (DATA_SOURCE*)str;

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
    auto system_config = (SYSTEM_CONFIG*)str;

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
    auto data_system = (DATA_SYSTEM*)str;

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

static int handle_putdata_block_list(XDR* xdrs, int direction, int* token, LOGMALLOCLIST* logmalloclist,
                                     USERDEFINEDTYPELIST* userdefinedtypelist, const void* str, int protocolVersion,
                                     LOGSTRUCTLIST* log_struct_list, unsigned int private_flags, int malloc_source)
{
    int err = 0;
    auto putDataBlockList = (PUTDATA_BLOCK_LIST*)str;

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

                PUTDATA_BLOCK putData;
                initPutDataBlock(&putData);

                if (!xdr_putdata_block1(xdrs, &putData)) {
                    err = UDA_PROTOCOL_ERROR_61;
                    UDA_LOG(UDA_LOG_DEBUG, "xdr_putdata_block1 Error (61)\n");
                    break;
                }

                if (protocolVersionTypeTest(protocolVersion, putData.data_type)) {
                    err = UDA_PROTOCOL_ERROR_9999;
                    break;
                }

                if (putData.count > 0 || putData.blockNameLength > 0) { // Some data to receive?

                    if ((err = allocPutData(&putData)) != 0) {
                        break; // Allocate Heap Memory
                    }

                    if (!xdr_putdata_block2(xdrs, &putData)) { // Fetch data
                        err = UDA_PROTOCOL_ERROR_62;
                        break;
                    }
                }

                if (putData.data_type == UDA_TYPE_COMPOUND && putData.opaque_type == UDA_OPAQUE_TYPE_STRUCTURES) {
                    // Structured Data

                    // Create a temporary DATA_BLOCK as the function's argument with structured data

                    // logmalloc list is automatically generated
                    // userdefinedtypelist is passed from the client
                    // NTREE is automatically generated

                    auto data_block = (DATA_BLOCK*)malloc(sizeof(DATA_BLOCK));

                    // *** Add to malloclog and test to ensure it is freed after use ***

                    initDataBlock(data_block);
                    data_block->opaque_type = UDA_OPAQUE_TYPE_STRUCTURES;
                    data_block->data_n = (int)putData.count;         // This number (also rank and shape)
                    data_block->opaque_block = putData.opaque_block; // User Defined Type

                    int protocol_id = UDA_PROTOCOL_STRUCTURES;
                    if ((err = protocolXML2Put(xdrs, protocol_id, direction, token, logmalloclist, userdefinedtypelist,
                                               data_block, protocolVersion, log_struct_list, private_flags,
                                               malloc_source)) != 0) {
                        // Fetch Structured data
                        break;
                    }

                    putData.data = reinterpret_cast<char*>(data_block); // Compact memory block with structures
                    auto general_block = (GENERAL_BLOCK*)data_block->opaque_block;
                    putData.opaque_block = general_block->userdefinedtype;
                }

                addIdamPutDataBlockList(&putData, putDataBlockList); // Add to the growing list
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

                if (protocolVersionTypeTest(protocolVersion, putDataBlockList->putDataBlock[i].data_type)) {
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

                    // Create a temporary DATA_BLOCK as the function's argument with structured data

                    //   *** putdata.opaque_count is not used or needed - count is sufficient

                    DATA_BLOCK data_block;
                    initDataBlock(&data_block);
                    data_block.opaque_type = UDA_OPAQUE_TYPE_STRUCTURES;
                    data_block.data_n =
                        (int)putDataBlockList->putDataBlock[i].count; // This number (also rank and shape)
                    data_block.opaque_block = putDataBlockList->putDataBlock[i].opaque_block; // User Defined Type
                    data_block.data =
                        (char*)putDataBlockList->putDataBlock[i].data; // Compact memory block with structures

                    int protocol_id = UDA_PROTOCOL_STRUCTURES;
                    if ((err = protocolXML2Put(xdrs, protocol_id, direction, token, logmalloclist, userdefinedtypelist,
                                               &data_block, protocolVersion, log_struct_list, private_flags,
                                               malloc_source)) != 0) {
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
    auto data_block = (DATA_BLOCK*)str;

    switch (direction) {
        case XDR_RECEIVE: {
            if (!xdr_data_block1(xdrs, data_block, protocolVersion)) {
                err = UDA_PROTOCOL_ERROR_61;
                break;
            }

            // Check client/server understands new data types
            // direction == XDR_RECEIVE && protocolVersion == 3 Means Client receiving data from a
            // Version >= 3 Server (Type has to be passed first)

            if (protocolVersionTypeTest(protocolVersion, data_block->data_type) ||
                protocolVersionTypeTest(protocolVersion, data_block->error_type)) {
                err = UDA_PROTOCOL_ERROR_9999;
                break;
            }

            if (data_block->data_n == 0) {
                break; // No Data to Receive!
            }

            if ((err = allocData(data_block)) != 0) {
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
                    initDimBlock(&data_block->dims[i]);
                }

                if (!xdr_data_dim1(xdrs, data_block)) {
                    err = UDA_PROTOCOL_ERROR_63;
                    break;
                }

                if (protocolVersion < 3) {
                    for (unsigned int i = 0; i < data_block->rank; i++) {
                        DIMS* dim = &data_block->dims[i];
                        if (protocolVersionTypeTest(protocolVersion, dim->data_type) ||
                            protocolVersionTypeTest(protocolVersion, dim->error_type)) {
                            err = UDA_PROTOCOL_ERROR_9999;
                            break;
                        }
                    }
                    if (err != 0) {
                        break;
                    }
                }

                if ((err = allocDim(data_block)) != 0) {
                    break; // Allocate Heap Memory
                }

                if (!xdr_data_dim2(xdrs, data_block)) { // Collect Only Uncompressed data
                    err = UDA_PROTOCOL_ERROR_64;
                    break;
                }

                for (unsigned int i = 0; i < data_block->rank; i++) { // Expand Compressed Regular Vector
                    err = uncompressDim(&(data_block->dims[i]));      // Allocate Heap as required
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
            printDataBlock(*data_block);

            if (protocolVersionTypeTest(protocolVersion, data_block->data_type) ||
                protocolVersionTypeTest(protocolVersion, data_block->error_type)) {
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
                        DIMS* dim = &data_block->dims[i];
                        if (protocolVersionTypeTest(protocolVersion, dim->data_type) ||
                            protocolVersionTypeTest(protocolVersion, dim->error_type)) {
                            err = UDA_PROTOCOL_ERROR_9999;
                            break;
                        }
                    }
                    if (err != 0) {
                        break;
                    }
                }

                for (unsigned int i = 0; i < data_block->rank; i++) {
                    compressDim(&(data_block->dims[i])); // Minimise Data Transfer if Regular
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
    auto data_block_list = (DATA_BLOCK_LIST*)str;

    switch (direction) {
        case XDR_RECEIVE:
            if (!xdr_data_block_list(xdrs, data_block_list, protocolVersion)) {
                err = UDA_PROTOCOL_ERROR_1;
                break;
            }
            data_block_list->data = (DATA_BLOCK*)malloc(data_block_list->count * sizeof(DATA_BLOCK));
            for (int i = 0; i < data_block_list->count; ++i) {
                DATA_BLOCK* data_block = &data_block_list->data[i];
                initDataBlock(data_block);
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
                DATA_BLOCK* data_block = &data_block_list->data[i];
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
    auto request_block = (REQUEST_BLOCK*)str;

    switch (direction) {
        case XDR_RECEIVE:
            if (!xdr_request(xdrs, request_block, protocolVersion)) {
                err = UDA_PROTOCOL_ERROR_1;
                break;
            }
            request_block->requests = (REQUEST_DATA*)malloc(request_block->num_requests * sizeof(REQUEST_DATA));
            for (int i = 0; i < request_block->num_requests; ++i) {
                initRequestData(&request_block->requests[i]);
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
