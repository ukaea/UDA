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

#include <logging/logging.h>
#include <clientserver/udaTypes.h>
#include <cstdlib>

#include "allocData.h"
#include "compressDim.h"
#include "xdrlib.h"
#include "initStructs.h"
#include "protocolXML.h"
#include "udaErrors.h"
#include "errorLog.h"

#ifdef SERVERBUILD
#  include <server/serverStartup.h>
#endif

int protocol(XDR* xdrs, int protocol_id, int direction, int* token, LOGMALLOCLIST* logmalloclist,
             USERDEFINEDTYPELIST* userdefinedtypelist, void* str, int protocolVersion, NTREE* full_ntree,
             LOGSTRUCTLIST* log_struct_list, IoData* io_data, unsigned int private_flags, int malloc_source)
{
    int err = 0;

    UDA_LOG(UDA_LOG_DEBUG, "\nPROTOCOL: protocolVersion = %d\n\n", protocolVersion);

    //----------------------------------------------------------------------------
    // Error Management Loop

    do {

        //----------------------------------------------------------------------------
        // Retrieve Client Requests

        if (protocol_id == PROTOCOL_REQUEST_BLOCK) {

            auto request_block = (REQUEST_BLOCK*) str;

            switch (direction) {
                case XDR_RECEIVE:
                    if (!xdrrec_skiprecord(xdrs)) {
                        err = PROTOCOL_ERROR_5;
                        break;
                    }
                    if (!xdr_request(xdrs, request_block, protocolVersion)) {
                        err = PROTOCOL_ERROR_1;
                        break;
                    }
                    break;

                case XDR_SEND:
                    if (!xdr_request(xdrs, request_block, protocolVersion)) {
                        err = PROTOCOL_ERROR_2;
                        break;
                    }
                    if (!xdrrec_endofrecord(xdrs, 1)) {
                        err = PROTOCOL_ERROR_7;
                        break;
                    }
                    break;

                case XDR_FREE_HEAP:
                    if (!xdr_request(xdrs, request_block, protocolVersion)) {
                        err = PROTOCOL_ERROR_3;
                        break;
                    }
                    break;

                default:
                    err = PROTOCOL_ERROR_4;
                    break;
            }

            break;
        }

        //----------------------------------------------------------------------------
        // Data Block

        if (protocol_id == PROTOCOL_DATA_BLOCK_LIST) {

            auto data_block = (DATA_BLOCK*)str;

            switch (direction) {
                case XDR_RECEIVE:
                    if (!xdrrec_skiprecord(xdrs)) {
                        err = PROTOCOL_ERROR_5;
                        break;
                    }
                    if (!xdr_data_block1(xdrs, data_block, protocolVersion)) {
                        err = PROTOCOL_ERROR_61;
                        break;
                    }

                    // Check client/server understands new data types
                    // direction == XDR_RECEIVE && protocolVersion == 3 Means Client receiving data from a
                    // Version >= 3 Server (Type has to be passed first)

                    if (protocolVersionTypeTest(protocolVersion, data_block->data_type) ||
                        protocolVersionTypeTest(protocolVersion, data_block->error_type)) {
                        err = PROTOCOL_ERROR_9999;
                        break;
                    }

                    if (data_block->data_n == 0) break;            // No Data to Receive!

                    if ((err = allocData(data_block)) != 0) break;        // Allocate Heap Memory

                    if (!xdr_data_block2(xdrs, data_block)) {
                        err = PROTOCOL_ERROR_62;
                        break;
                    }

                    if (data_block->error_type != UDA_TYPE_UNKNOWN ||
                        data_block->error_param_n > 0) {    // Receive Only if Error Data are available
                        if (!xdr_data_block3(xdrs, data_block)) {
                            err = PROTOCOL_ERROR_62;
                            break;
                        }

                        if (!xdr_data_block4(xdrs, data_block)) {        // Asymmetric Errors
                            err = PROTOCOL_ERROR_62;
                            break;
                        }
                    }

                    if (data_block->rank > 0) {    // Check if there are Dimensional Data to Receive
                        for (unsigned int i = 0; i < data_block->rank; i++) {
                            initDimBlock(&data_block->dims[i]);
                        }

                        if (!xdr_data_dim1(xdrs, data_block)) {
                            err = PROTOCOL_ERROR_63;
                            break;
                        }

                        if (protocolVersion < 3) {
                            for (unsigned int i = 0; i < data_block->rank; i++) {
                                auto dim = &data_block->dims[i];
                                if (protocolVersionTypeTest(protocolVersion, dim->data_type) ||
                                    protocolVersionTypeTest(protocolVersion, dim->error_type)) {
                                    err = PROTOCOL_ERROR_9999;
                                    break;
                                }
                            }
                        }
                        if (err) break;

                        if ((err = allocDim(data_block)) != 0) break;            // Allocate Heap Memory

                        if (!xdr_data_dim2(xdrs, data_block)) {        // Collect Only Uncompressed data
                            err = PROTOCOL_ERROR_64;
                            break;
                        }

                        for (unsigned int i = 0; i < data_block->rank; i++) {            // Expand Compressed Regular Vector
                            err = uncompressDim(&(data_block->dims[i]));    // Allocate Heap as required
                            err = 0; // Need to Test for Error Condition!
                        }

                        if (!xdr_data_dim3(xdrs, data_block)) {
                            err = PROTOCOL_ERROR_65;
                            break;
                        }

                        if (!xdr_data_dim4(xdrs, data_block)) {        // Asymmetric Errors
                            err = PROTOCOL_ERROR_65;
                            break;
                        }

                    }

                    break;

                case XDR_SEND:

                    // Check client/server understands new data types

                    // direction == XDR_SEND && protocolVersion == 3 Means Server sending data to a Version 3 Client (Type is known)

                    if (protocolVersionTypeTest(protocolVersion, data_block->data_type) ||
                        protocolVersionTypeTest(protocolVersion, data_block->error_type)) {
                        err = PROTOCOL_ERROR_9999;
                        break;
                    }

                    if (!xdr_data_block1(xdrs, data_block, protocolVersion)) {
                        err = PROTOCOL_ERROR_61;
                        break;
                    }

                    if (data_block->data_n == 0) {                // No Data or Dimensions to Send!
                        if (!xdrrec_endofrecord(xdrs, 1)) err = PROTOCOL_ERROR_7;
                        break;
                    }

                    if (!xdr_data_block2(xdrs, data_block)) {
                        err = PROTOCOL_ERROR_62;
                        break;
                    }

                    if (data_block->error_type != UDA_TYPE_UNKNOWN ||
                        data_block->error_param_n > 0) {    // Only Send if Error Data are available
                        if (!xdr_data_block3(xdrs, data_block)) {
                            err = PROTOCOL_ERROR_62;
                            break;
                        }
                        if (!xdr_data_block4(xdrs, data_block)) {
                            err = PROTOCOL_ERROR_62;
                            break;
                        }
                    }


                    if (data_block->rank > 0) {    // Dimensional Data to Send

                        // Check client/server understands new data types

                        if (protocolVersion < 3) {
                            for (unsigned int i = 0; i < data_block->rank; i++) {
                                auto dim = &data_block->dims[i];
                                if (protocolVersionTypeTest(protocolVersion, dim->data_type) ||
                                    protocolVersionTypeTest(protocolVersion, dim->error_type)) {
                                    err = PROTOCOL_ERROR_9999;
                                    break;
                                }
                            }
                        }

                        for (unsigned int i = 0; i < data_block->rank; i++) {
                            compressDim(&(data_block->dims[i]));        // Minimise Data Transfer if Regular
                        }

                        if (!xdr_data_dim1(xdrs, data_block)) {
                            err = PROTOCOL_ERROR_63;
                            break;
                        }

                        if (!xdr_data_dim2(xdrs, data_block)) {
                            err = PROTOCOL_ERROR_64;
                            break;
                        }

                        if (!xdr_data_dim3(xdrs, data_block)) {
                            err = PROTOCOL_ERROR_65;
                        }

                        if (!xdr_data_dim4(xdrs, data_block)) {
                            err = PROTOCOL_ERROR_65;
                        }

                    }

                    if (!xdrrec_endofrecord(xdrs, 1)) {
                        err = PROTOCOL_ERROR_7;
                    }

                    break;

                case XDR_FREE_HEAP:
                    break;

                default:
                    err = PROTOCOL_ERROR_4;
                    break;
            }
            break;
        }

        //----------------------------------------------------------------------------
        // Data Block

        if (protocol_id == PROTOCOL_PUTDATA_BLOCK_LIST) {

            auto putDataBlockList = (PUTDATA_BLOCK_LIST*) str;
            PUTDATA_BLOCK putData;

            switch (direction) {

                case XDR_RECEIVE: {

                    if (!xdrrec_skiprecord(xdrs)) {
                        err = PROTOCOL_ERROR_5;
                        break;
                    }

                    unsigned int blockCount = 0;
                    int rc = xdr_u_int(xdrs, &blockCount);

                    if (!rc) {
                        err = PROTOCOL_ERROR_61;
                        break;
                    }

                    UDA_LOG(UDA_LOG_DEBUG, "receive: putDataBlockList Count: %d\n", blockCount);

                    for (unsigned int i = 0; i < blockCount; i++) {        // Fetch multiple put blocks

                        initIdamPutDataBlock(&putData);

                        if (!xdr_putdata_block1(xdrs, &putData)) {
                            err = PROTOCOL_ERROR_61;
                            UDA_LOG(UDA_LOG_DEBUG, "xdr_putdata_block1 Error (61)\n");
                            break;
                        }

                        if (protocolVersionTypeTest(protocolVersion, putData.data_type)) {
                            err = PROTOCOL_ERROR_9999;
                            break;
                        }

                        if (putData.count > 0 || putData.blockNameLength > 0) {

                            if ((err = allocPutData(&putData)) != 0) break;    // Allocate Heap Memory

                            if (!xdr_putdata_block2(xdrs, &putData)) {    // Fetch data
                                err = PROTOCOL_ERROR_62;
                                break;
                            }

                            addIdamPutDataBlockList(&putData, putDataBlockList);    // Add to the growing list

                        }
                    }

                    break;
                }

                case XDR_SEND: {

                    UDA_LOG(UDA_LOG_DEBUG, "send: putDataBlockList Count: %d\n", putDataBlockList->blockCount);

                    int rc = xdr_u_int(xdrs, &(putDataBlockList->blockCount));

                    if (!rc) {
                        err = PROTOCOL_ERROR_61;
                        break;
                    }

                    for (unsigned int i = 0; i < putDataBlockList->blockCount; i++) {        // Send multiple put blocks

                        if (!xdr_putdata_block1(xdrs, &(putDataBlockList->putDataBlock[i]))) {
                            err = PROTOCOL_ERROR_61;
                            break;
                        }

                        if (protocolVersionTypeTest(protocolVersion, putDataBlockList->putDataBlock[i].data_type)) {
                            err = PROTOCOL_ERROR_9999;
                            break;
                        }

                        if (putDataBlockList->putDataBlock[i].count > 0 ||
                            putDataBlockList->putDataBlock[i].blockNameLength > 0) {                // Data to Send?

                            if (!xdr_putdata_block2(xdrs, &(putDataBlockList->putDataBlock[i]))) {
                                err = PROTOCOL_ERROR_62;
                                break;
                            }

                        }
                    }

                    if (!xdrrec_endofrecord(xdrs, 1)) {
                        err = PROTOCOL_ERROR_7;
                    }

                    break;
                }

                case XDR_FREE_HEAP:
                    break;

                default:
                    err = PROTOCOL_ERROR_4;
                    break;
            }

            break;
        }

        //----------------------------------------------------------------------------
        // Error Status or Next Protocol id or exchange token ....

        if (protocol_id == PROTOCOL_NEXT_PROTOCOL) {

            switch (direction) {

                case XDR_RECEIVE:                    // From Client to Server
                    if (!xdrrec_skiprecord(xdrs)) {
                        err = PROTOCOL_ERROR_5;
                        break;
                    }
                    if (!xdr_int(xdrs, token)) {
                        err = PROTOCOL_ERROR_9;
                        break;
                    }
                    break;

                case XDR_SEND:
                    if (!xdr_int(xdrs, token)) {
                        err = PROTOCOL_ERROR_9;
                        break;
                    }
                    if (!xdrrec_endofrecord(xdrs, 1)) {
                        err = PROTOCOL_ERROR_7;
                        break;
                    }
                    break;

                case XDR_FREE:
                    err = PROTOCOL_ERROR_3;
                    break;

                default:
                    err = PROTOCOL_ERROR_4;
                    break;
            }

            break;
        }

        //----------------------------------------------------------------------------
        // Data System record

        if (protocol_id == PROTOCOL_DATA_SYSTEM) {
            auto data_system = (DATA_SYSTEM*) str;

            switch (direction) {
                case XDR_RECEIVE:                    // From Client to Server
                    if (!xdrrec_skiprecord(xdrs)) {
                        err = PROTOCOL_ERROR_5;
                        break;
                    }
                    if (!xdr_data_system(xdrs, data_system)) {
                        err = PROTOCOL_ERROR_10;
                        break;
                    }
                    break;

                case XDR_SEND:

                    if (!xdr_data_system(xdrs, data_system)) {
                        err = PROTOCOL_ERROR_10;
                        break;
                    }
                    if (!xdrrec_endofrecord(xdrs, 1)) {
                        err = PROTOCOL_ERROR_7;
                        break;
                    }
                    break;

                case XDR_FREE_HEAP:

                    if (!xdr_data_system(xdrs, data_system)) {
                        err = PROTOCOL_ERROR_11;
                        break;
                    }
                    break;

                default:
                    err = PROTOCOL_ERROR_4;
                    break;
            }
            break;
        }

        //----------------------------------------------------------------------------
        // System Configuration record

        if (protocol_id == PROTOCOL_SYSTEM_CONFIG) {

            auto system_config = (SYSTEM_CONFIG*) str;

            switch (direction) {
                case XDR_RECEIVE:
                    // From Client to Server
                    if (!xdrrec_skiprecord(xdrs)) {
                        err = PROTOCOL_ERROR_5;
                        break;
                    }
                    if (!xdr_system_config(xdrs, system_config)) {
                        err = PROTOCOL_ERROR_12;
                        break;
                    }
                    break;

                case XDR_SEND:


                    if (!xdr_system_config(xdrs, system_config)) {
                        err = PROTOCOL_ERROR_12;
                        break;
                    }
                    if (!xdrrec_endofrecord(xdrs, 1)) {
                        err = PROTOCOL_ERROR_7;
                        break;
                    }
                    break;

                case XDR_FREE_HEAP:

                    if (!xdr_system_config(xdrs, system_config)) {
                        err = PROTOCOL_ERROR_13;
                        break;
                    }
                    break;

                default:
                    err = PROTOCOL_ERROR_4;
                    break;
            }
            break;
        }

        //----------------------------------------------------------------------------
        // Data Source record

        if (protocol_id == PROTOCOL_DATA_SOURCE) {

            auto data_source = (DATA_SOURCE*) str;

            switch (direction) {
                case XDR_RECEIVE:
                    // From Client to Server
                    if (!xdrrec_skiprecord(xdrs)) {
                        err = PROTOCOL_ERROR_5;
                        break;
                    }
                    if (!xdr_data_source(xdrs, data_source)) {
                        err = PROTOCOL_ERROR_14;
                        break;
                    }
                    break;

                case XDR_SEND:

                    if (!xdr_data_source(xdrs, data_source)) {
                        err = PROTOCOL_ERROR_14;
                        break;
                    }
                    if (!xdrrec_endofrecord(xdrs, 1)) {
                        err = PROTOCOL_ERROR_7;
                        break;
                    }
                    break;

                case XDR_FREE_HEAP:

                    if (!xdr_data_source(xdrs, data_source)) {
                        err = PROTOCOL_ERROR_15;
                        break;
                    }
                    break;

                default:
                    err = PROTOCOL_ERROR_4;
                    break;
            }
            break;
        }

        //----------------------------------------------------------------------------
        // Signal record

        if (protocol_id == PROTOCOL_SIGNAL) {

            auto signal = (SIGNAL*) str;

            switch (direction) {

                case XDR_RECEIVE:
                    // From Client to Server
                    if (!xdrrec_skiprecord(xdrs)) {
                        err = PROTOCOL_ERROR_5;
                        break;
                    }
                    if (!xdr_signal(xdrs, signal)) {
                        err = PROTOCOL_ERROR_16;
                        break;
                    }
                    break;

                case XDR_SEND:

                    if (!xdr_signal(xdrs, signal)) {
                        err = PROTOCOL_ERROR_16;
                        break;
                    }
                    if (!xdrrec_endofrecord(xdrs, 1)) {
                        err = PROTOCOL_ERROR_7;
                        break;
                    }
                    break;

                case XDR_FREE_HEAP:

                    if (!xdr_signal(xdrs, signal)) {
                        err = PROTOCOL_ERROR_17;
                        break;
                    }
                    break;

                default:
                    err = PROTOCOL_ERROR_4;
                    break;
            }
            break;
        }

        //----------------------------------------------------------------------------
        // Signal Description record

        if (protocol_id == PROTOCOL_SIGNAL_DESC) {

            auto signal_desc = (SIGNAL_DESC*) str;

            switch (direction) {

                case XDR_RECEIVE:
                    // From Client to Server
                    if (!xdrrec_skiprecord(xdrs)) {
                        err = PROTOCOL_ERROR_5;
                        break;
                    }
                    if (!xdr_signal_desc(xdrs, signal_desc)) {
                        err = PROTOCOL_ERROR_18;
                        break;
                    }

                    break;

                case XDR_SEND:

                    if (!xdr_signal_desc(xdrs, signal_desc)) {
                        err = PROTOCOL_ERROR_18;
                        break;
                    }
                    if (!xdrrec_endofrecord(xdrs, 1)) {
                        err = PROTOCOL_ERROR_7;
                        break;
                    }
                    break;

                case XDR_FREE_HEAP:

                    if (!xdr_signal_desc(xdrs, signal_desc)) {
                        err = PROTOCOL_ERROR_19;
                        break;
                    }
                    break;

                default:
                    err = PROTOCOL_ERROR_4;
                    break;
            }
            break;
        }

        //----------------------------------------------------------------------------
        // Client State

        if (protocol_id == PROTOCOL_CLIENT_BLOCK) {

            auto client_block = (CLIENT_BLOCK*) str;

            switch (direction) {

                case XDR_RECEIVE:
                    // From Client to Server
                    if (!xdrrec_skiprecord(xdrs)) {
                        err = PROTOCOL_ERROR_5;
                        break;
                    }
                    if (!xdr_client(xdrs, client_block, protocolVersion)) {
                        err = PROTOCOL_ERROR_20;
                        break;
                    }

                    break;

                case XDR_SEND:

                    if (!xdr_client(xdrs, client_block, protocolVersion)) {
                        err = PROTOCOL_ERROR_20;
                        break;
                    }
                    if (!xdrrec_endofrecord(xdrs, 1)) {
                        err = PROTOCOL_ERROR_7;
                        break;
                    }
                    break;

                case XDR_FREE_HEAP:

                    if (!xdr_client(xdrs, client_block, protocolVersion)) {
                        err = PROTOCOL_ERROR_21;
                        break;
                    }
                    break;

                default:
                    err = PROTOCOL_ERROR_4;
                    break;
            }

            break;
        }

        //----------------------------------------------------------------------------
        // Server State

        if (protocol_id == PROTOCOL_SERVER_BLOCK) {

            auto server_block = (SERVER_BLOCK*) str;

            switch (direction) {

                case XDR_RECEIVE:

                    if (!xdrrec_skiprecord(xdrs)) {
                        err = PROTOCOL_ERROR_5;
                        break;
                    }

                    closeUdaError();    // Free Heap associated with Previous Data Access

                    if (!xdr_server1(xdrs, server_block, protocolVersion)) {
                        err = PROTOCOL_ERROR_22;
                        break;
                    }

                    if (server_block->idamerrorstack.nerrors > 0) {    // No Data to Receive?

                        server_block->idamerrorstack.idamerror = (UDA_ERROR*) malloc(
                                server_block->idamerrorstack.nerrors * sizeof(UDA_ERROR));
                        initErrorRecords(&server_block->idamerrorstack);

                        if (!xdr_server2(xdrs, server_block)) {
                            err = PROTOCOL_ERROR_22;
                            break;
                        }
                    }

                    break;

                case XDR_SEND:
                    if (!xdr_server1(xdrs, server_block, protocolVersion)) {
                        err = PROTOCOL_ERROR_22;
                        break;
                    }

                    if (server_block->idamerrorstack.nerrors > 0) {        // No Data to Send?
                        if (!xdr_server2(xdrs, server_block)) {
                            err = PROTOCOL_ERROR_22;
                            break;
                        }
                    }

                    if (!xdrrec_endofrecord(xdrs, 1)) {
                        err = PROTOCOL_ERROR_7;
                        break;
                    }
                    break;

                case XDR_FREE_HEAP:

                    if (!xdr_server(xdrs, server_block)) {
                        err = PROTOCOL_ERROR_23;
                        break;
                    }
                    break;

                default:
                    err = PROTOCOL_ERROR_4;
                    break;
            }

            break;
        }

        //----------------------------------------------------------------------------
        // Hierarchical or Meta Data Structures

        if (protocol_id > PROTOCOL_OPAQUE_START && protocol_id < PROTOCOL_OPAQUE_STOP) {
            err = protocolXML(xdrs, protocol_id, direction, token, logmalloclist, userdefinedtypelist, str,
                              protocolVersion, full_ntree, log_struct_list, io_data, private_flags, malloc_source);
        }

        //----------------------------------------------------------------------------
        // End of Error Trap Loop

    } while (0);

    return err;
}
