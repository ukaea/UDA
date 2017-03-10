/*---------------------------------------------------------------
* Client - Server Conversation Protocol
*
* Args:	xdrs		XDR Stream
*
*	protocol_id	Client/Server Conversation item: Data Exchange context
*	direction	Send (0) or Receive (1) or Free (2)
*	token		current error condition or next protocol or .... exchange token
*
*	str		Information Structure depending on the protocol id ....
*
*	2	data_block	Data read from the external Source or Data to be written
*				to an external source
*	4	data_system	Database Data_Dystem table record
*	5	system_config	Database System_Config table record
*	6	data_source	Database Data_Source table record
*	7	signal		Database Signal table record
*	8	signal_desc	Database Signal_Desc table record
*
* Returns: error code if failure, otherwise 0
*
*--------------------------------------------------------------*/

#include "protocol.h"

#include <stdlib.h>

#include <logging/logging.h>
#include <clientserver/udaTypes.h>

#include "allocData.h"
#include "compressDim.h"
#include "printStructs.h"
#include "xdrlib.h"
#include "initStructs.h"
#include "protocolXML2.h"

#ifdef HIERARCHICAL_DATA
#  include "idamclientserverxml.h"	// legacy
#endif

#include "protocolXML2Put.h"
#include "udaErrors.h"
#include "errorLog.h"

int protocol2(XDR* xdrs, int protocol_id, int direction, int* token, void* str)
{

    DATA_BLOCK* data_block;
    DIMS* dim;
    DATA_SYSTEM* data_system;
    SYSTEM_CONFIG* system_config;
    DATA_SOURCE* data_source;
    SIGNAL* signal;
    SIGNAL_DESC* signal_desc;

    REQUEST_BLOCK* request_block;
    CLIENT_BLOCK* client_block;
    SERVER_BLOCK* server_block;

    int err = 0;

//----------------------------------------------------------------------------
// Error Management Loop

    do {

//----------------------------------------------------------------------------
// Retrieve Client Requests

        if (protocol_id == PROTOCOL_REQUEST_BLOCK) {

            request_block = (REQUEST_BLOCK*) str;

            switch (direction) {

                case XDR_RECEIVE :
                    if (!xdr_request(xdrs, request_block)) {
                        err = PROTOCOL_ERROR_1;
                        break;
                    }
                    break;

                case XDR_SEND:
                    if (!xdr_request(xdrs, request_block)) {
                        err = PROTOCOL_ERROR_2;
                        break;
                    }
                    break;

                case XDR_FREE_HEAP :
                    break;


                default:
                    err = PROTOCOL_ERROR_4;
                    break;
            }

            break;
        }


//----------------------------------------------------------------------------
// Data Block

        if (protocol_id == PROTOCOL_DATA_BLOCK) {

            data_block = (DATA_BLOCK*) str;

#ifndef SKIPSEND
            switch (direction) {

                case XDR_RECEIVE :
                    if (!xdr_data_block1(xdrs, data_block)) {
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

                    if (data_block->error_type != TYPE_UNKNOWN ||
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

                        int i;
                        for (i = 0; i < data_block->rank; i++) initDimBlock(&data_block->dims[i]);

                        if (!xdr_data_dim1(xdrs, data_block)) {
                            err = PROTOCOL_ERROR_63;
                            break;
                        }

                        if (protocolVersion < 3) {
                            for (i = 0; i < data_block->rank; i++) {
                                dim = &data_block->dims[i];
                                if (protocolVersionTypeTest(protocolVersion, dim->data_type) ||
                                    protocolVersionTypeTest(protocolVersion, dim->error_type)) {
                                    err = PROTOCOL_ERROR_9999;
                                    break;
                                }
                            }
                        }

                        if ((err = allocDim(data_block)) != 0) break;            // Allocate Heap Memory

                        if (!xdr_data_dim2(xdrs, data_block)) {        // Collect Only Uncompressed data
                            err = PROTOCOL_ERROR_64;
                            break;
                        }

                        for (i = 0; i < data_block->rank; i++) {            // Expand Compressed Regular Vector
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

                    IDAM_LOG(LOG_DEBUG, "#1 Send/Receive Data Block\n");
                    printDataBlock(*data_block);

                    if (protocolVersionTypeTest(protocolVersion, data_block->data_type) ||
                        protocolVersionTypeTest(protocolVersion, data_block->error_type)) {
                        err = PROTOCOL_ERROR_9999;
                        IDAM_LOG(LOG_DEBUG, "protocolVersionTypeTest Failed\n");

                        break;
                    }
                    IDAM_LOG(LOG_DEBUG, "#2 Send/Receive Data Block\n");
                    if (!xdr_data_block1(xdrs, data_block)) {
                        err = PROTOCOL_ERROR_61;
                        break;
                    }

                    if (data_block->data_n == 0) {                // No Data or Dimensions to Send!
                        break;
                    }

                    if (!xdr_data_block2(xdrs, data_block)) {
                        err = PROTOCOL_ERROR_62;
                        break;
                    }

                    if (data_block->error_type != TYPE_UNKNOWN ||
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

                        int i;
                        if (protocolVersion < 3) {
                            for (i = 0; i < data_block->rank; i++) {
                                dim = &data_block->dims[i];
                                if (protocolVersionTypeTest(protocolVersion, dim->data_type) ||
                                    protocolVersionTypeTest(protocolVersion, dim->error_type)) {
                                    err = PROTOCOL_ERROR_9999;
                                    break;
                                }
                            }
                        }

                        for (i = 0; i < data_block->rank; i++) {
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
                    break;

                case XDR_FREE_HEAP :
                    break;

                default:
                    err = PROTOCOL_ERROR_4;
                    break;
            }
#endif
            break;
        }

//----------------------------------------------------------------------------
// Put Data Block (Atomic and Structured types)

        if (protocol_id == PROTOCOL_PUTDATA_BLOCK_LIST) {

            PUTDATA_BLOCK_LIST* putDataBlockList = (PUTDATA_BLOCK_LIST*) str;
            PUTDATA_BLOCK putData;

            switch (direction) {

                case XDR_RECEIVE : {
                    unsigned int blockCount = 0;

                    if (!xdr_u_int(xdrs, &blockCount)) {
                        err = PROTOCOL_ERROR_61;
                        break;
                    }

                    IDAM_LOGF(LOG_DEBUG, "receive: putDataBlockList Count: %d\n", blockCount);

                    int i;
                    for (i = 0; i < blockCount; i++) {        // Fetch multiple put blocks

                        initIdamPutDataBlock(&putData);

                        if (!xdr_putdata_block1(xdrs, &putData)) {
                            err = PROTOCOL_ERROR_61;
                            IDAM_LOG(LOG_DEBUG, "xdr_putdata_block1 Error (61)\n");
                            break;
                        }

                        if (protocolVersionTypeTest(protocolVersion, putData.data_type)) {
                            err = PROTOCOL_ERROR_9999;
                            break;
                        }

                        if (putData.count > 0 || putData.blockNameLength > 0) {    // Some data to receive?

                            if ((err = allocPutData(&putData)) != 0) break;    // Allocate Heap Memory

                            if (!xdr_putdata_block2(xdrs, &putData)) {    // Fetch data
                                err = PROTOCOL_ERROR_62;
                                break;
                            }
                        }

                        if (putData.data_type == TYPE_COMPOUND &&
                            putData.opaque_type == OPAQUE_TYPE_STRUCTURES) {    // Structured Data

// Create a temporary DATA_BLOCK as the function's argument with structured data

// logmalloc list is automatically generated
// userdefinedtypelist is passed from the client
// NTREE is automatically generated

                            DATA_BLOCK* data_block = (DATA_BLOCK*) malloc(sizeof(DATA_BLOCK));

// *** Add to malloclog and test to ensure it is freed after use ***

                            initDataBlock(data_block);
                            data_block->opaque_type = OPAQUE_TYPE_STRUCTURES;
                            data_block->data_n = putData.count;        // This number (also rank and shape)
                            data_block->opaque_block = putData.opaque_block;    // User Defined Type

                            protocol_id = PROTOCOL_STRUCTURES;
                            if ((err = protocolXML2Put(xdrs, protocol_id, direction, token, data_block)) != 0)
                                break;    // Fetch Structured data

                            putData.data = (void*) data_block;        // Compact memory block with structures
                            GENERAL_BLOCK* general_block = (GENERAL_BLOCK*) data_block->opaque_block;
                            putData.opaque_block = general_block->userdefinedtype;

                        }

                        addIdamPutDataBlockList(&putData, putDataBlockList);        // Add to the growing list

                    }
                    break;
                }

                case XDR_SEND:

                    IDAM_LOGF(LOG_DEBUG, "send: putDataBlockList Count: %d\n", putDataBlockList->blockCount);

                    if (!xdr_u_int(xdrs, &(putDataBlockList->blockCount))) {
                        err = PROTOCOL_ERROR_61;
                        break;
                    }

                    int i;
                    for (i = 0; i < putDataBlockList->blockCount; i++) {        // Send multiple put blocks

                        if (!xdr_putdata_block1(xdrs, &(putDataBlockList->putDataBlock[i]))) {
                            err = PROTOCOL_ERROR_61;
                            break;
                        }

                        if (protocolVersionTypeTest(protocolVersion, putDataBlockList->putDataBlock[i].data_type)) {
                            err = PROTOCOL_ERROR_9999;
                            break;
                        }

                        if (putDataBlockList->putDataBlock[i].count > 0 ||
                            putDataBlockList->putDataBlock[i].blockNameLength > 0) {    // Data to Send?

                            if (!xdr_putdata_block2(xdrs, &(putDataBlockList->putDataBlock[i]))) {
                                err = PROTOCOL_ERROR_62;
                                break;
                            }
                        }

                        if (putDataBlockList->putDataBlock[i].data_type == TYPE_COMPOUND &&
                            putDataBlockList->putDataBlock[i].opaque_type ==
                            OPAQUE_TYPE_STRUCTURES) {        // Structured Data

// Create a temporary DATA_BLOCK as the function's argument with structured data

//   *** putdata.opaque_count is not used or needed - count is sufficient

                            DATA_BLOCK data_block;
                            initDataBlock(&data_block);
                            data_block.opaque_type = OPAQUE_TYPE_STRUCTURES;
                            data_block.data_n = putDataBlockList->putDataBlock[i].count;        // This number (also rank and shape)
                            data_block.opaque_block = putDataBlockList->putDataBlock[i].opaque_block;    // User Defined Type
                            data_block.data = (char*) putDataBlockList->putDataBlock[i].data;    // Compact memory block with structures

                            protocol_id = PROTOCOL_STRUCTURES;
                            if ((err = protocolXML2Put(xdrs, protocol_id, direction, token, &data_block)) != 0)
                                break;    // Send Structured data

                        }
                    }
                    break;

                case XDR_FREE_HEAP :
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

                case XDR_RECEIVE :                    // From Client to Server
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

                case XDR_FREE :
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

            data_system = (DATA_SYSTEM*) str;
#ifndef SKIPSEND
            switch (direction) {

                case XDR_RECEIVE :                    // From Client to Server
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
                    break;

                case XDR_FREE_HEAP :
                    break;

                default:
                    err = PROTOCOL_ERROR_4;
                    break;
            }
#endif
            break;
        }


//----------------------------------------------------------------------------
// System Configuration record

        if (protocol_id == PROTOCOL_SYSTEM_CONFIG) {

            system_config = (SYSTEM_CONFIG*) str;
#ifndef SKIPSEND
            switch (direction) {

                case XDR_RECEIVE :
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
                    break;

                case XDR_FREE_HEAP :
                    break;

                default:
                    err = PROTOCOL_ERROR_4;
                    break;
            }
#endif
            break;
        }


//----------------------------------------------------------------------------
// Data Source record

        if (protocol_id == PROTOCOL_DATA_SOURCE) {

            data_source = (DATA_SOURCE*) str;
#ifndef SKIPSEND
            switch (direction) {

                case XDR_RECEIVE :
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
                    break;

                case XDR_FREE_HEAP :
                    break;

                default:
                    err = PROTOCOL_ERROR_4;
                    break;
            }
#endif
            break;
        }



//----------------------------------------------------------------------------
// Signal record

        if (protocol_id == PROTOCOL_SIGNAL) {

            signal = (SIGNAL*) str;
#ifndef SKIPSEND
            switch (direction) {

                case XDR_RECEIVE :
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
                    break;

                case XDR_FREE_HEAP :
                    break;

                default:
                    err = PROTOCOL_ERROR_4;
                    break;
            }
#endif
            break;
        }



//----------------------------------------------------------------------------
// Signal Description record

        if (protocol_id == PROTOCOL_SIGNAL_DESC) {

            signal_desc = (SIGNAL_DESC*) str;
#ifndef SKIPSEND
            switch (direction) {

                case XDR_RECEIVE :
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
                    break;

                case XDR_FREE_HEAP :
                    break;

                default:
                    err = PROTOCOL_ERROR_4;
                    break;
            }
#endif
            break;
        }

//----------------------------------------------------------------------------
// SECURITY

#ifdef SECURITYENABLED
        if(protocol_id == PROTOCOL_SECURITY_BLOCK) {

            client_block = (CLIENT_BLOCK *)str;
            SECURITY_BLOCK* security_block = &(client_block->securityBlock);

            switch (direction) {
                case XDR_RECEIVE:
                    if(!xdr_securityBlock1(xdrs, security_block)) {
                        err = PROTOCOL_ERROR_23;
                        break;
                    }
                    break;

                case XDR_SEND:
                    if(!xdr_securityBlock1(xdrs, security_block)) {
                        err = PROTOCOL_ERROR_23;
                        break;
                    }
                    break;

                case XDR_FREE_HEAP:
                    break;

                default:
                    err = PROTOCOL_ERROR_4;
                    break;
            }

// Allocate heap

            if(security_block->client_ciphertextLength > 0)
                security_block->client_ciphertext  = (unsigned char *)malloc(security_block->client_ciphertextLength*sizeof(unsigned char));
            if(security_block->client2_ciphertextLength > 0)
                security_block->client2_ciphertext = (unsigned char *)malloc(security_block->client2_ciphertextLength*sizeof(unsigned char));
            if(security_block->server_ciphertextLength > 0)
                security_block->server_ciphertext  = (unsigned char *)malloc(security_block->server_ciphertextLength*sizeof(unsigned char));
            if(security_block->client_X509Length > 0)
                security_block->client_X509        = (unsigned char *)malloc(security_block->client_X509Length*sizeof(unsigned char));
            if(security_block->client2_X509Length > 0)
                security_block->client2_X509       = (unsigned char *)malloc(security_block->client2_X509Length*sizeof(unsigned char));

            switch (direction) {
                case XDR_RECEIVE :
                    if(!xdr_securityBlock2(xdrs, security_block)) {
                        err = PROTOCOL_ERROR_24;
                        break;
                    }
                    break;

                case XDR_SEND:
                    if(!xdr_securityBlock2(xdrs, security_block)) {
                        err = PROTOCOL_ERROR_24;
                        break;
                    }
                    break;

                case XDR_FREE_HEAP :
                    break;

                default:
                    err = PROTOCOL_ERROR_4;
                    break;
            }

            break;
        }
#endif

//----------------------------------------------------------------------------
// Client State

        if (protocol_id == PROTOCOL_CLIENT_BLOCK) {

            client_block = (CLIENT_BLOCK*) str;

            switch (direction) {

                case XDR_RECEIVE :
                    if (!xdr_client(xdrs, client_block)) {
                        err = PROTOCOL_ERROR_20;
                        break;
                    }
                    break;

                case XDR_SEND:
                    if (!xdr_client(xdrs, client_block)) {
                        err = PROTOCOL_ERROR_20;
                        break;
                    }
                    break;

                case XDR_FREE_HEAP :
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

            server_block = (SERVER_BLOCK*) str;

            switch (direction) {

                case XDR_RECEIVE:
                    closeIdamError(&server_block->idamerrorstack);    // Free Heap associated with Previous Data Access

                    if (!xdr_server1(xdrs, server_block)) {
                        err = PROTOCOL_ERROR_22;
                        break;
                    }

                    if (server_block->idamerrorstack.nerrors > 0) {    // No Data to Receive?

                        server_block->idamerrorstack.idamerror = (IDAMERROR*) malloc(
                                server_block->idamerrorstack.nerrors * sizeof(IDAMERROR));
                        initIdamErrorRecords(&server_block->idamerrorstack);

                        if (!xdr_server2(xdrs, server_block)) {
                            err = PROTOCOL_ERROR_22;
                            break;
                        }
                    }

                    break;

                case XDR_SEND:
                    if (!xdr_server1(xdrs, server_block)) {
                        err = PROTOCOL_ERROR_22;
                        break;
                    }

                    if (server_block->idamerrorstack.nerrors > 0) {        // No Data to Send?
                        if (!xdr_server2(xdrs, server_block)) {
                            err = PROTOCOL_ERROR_22;
                            break;
                        }
                    }

                    break;

                case XDR_FREE_HEAP :
                    break;

                default:
                    err = PROTOCOL_ERROR_4;
                    break;
            }

            break;
        }


//----------------------------------------------------------------------------
// Send/Receive data objects (opaque structures with serialised data saved to volatile memory)

        if (protocol_id == PROTOCOL_DATAOBJECT) {

            DATA_OBJECT* data_object = (DATA_OBJECT*) str;

            switch (direction) {

                case XDR_RECEIVE :

                    if (!xdr_data_object1(xdrs, data_object)) {    // Storage requirements
                        err = PROTOCOL_ERROR_22;
                        break;
                    }
                    if (data_object->objectSize > 0)
                        data_object->object = (char*) malloc(data_object->objectSize * sizeof(char));
                    if (data_object->hashLength > 0)
                        data_object->md = (char*) malloc(data_object->hashLength * sizeof(char));
                    if (!xdr_data_object2(xdrs, data_object)) {
                        err = PROTOCOL_ERROR_22;
                        break;
                    }
                    break;

                case XDR_SEND:

                    if (!xdr_data_object1(xdrs, data_object)) {
                        err = PROTOCOL_ERROR_22;
                        break;
                    }
                    if (!xdr_data_object2(xdrs, data_object)) {
                        err = PROTOCOL_ERROR_22;
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
// Send/Receive data object files (opaque structures with serialised data saved to permanent file)

        if (protocol_id == PROTOCOL_DATAOBJECT_FILE) {

            data_block = (DATA_BLOCK*) str;

            switch (direction) {

                case XDR_RECEIVE :
                    break;

                case XDR_SEND:
                    break;

                default:
                    err = PROTOCOL_ERROR_4;
                    break;
            }

            break;
        }


//----------------------------------------------------------------------------
// Legacy: Hierarchical or Meta Data Structures

        if (protocol_id > PROTOCOL_OPAQUE_START && protocol_id < PROTOCOL_OPAQUE_STOP) {
            err = protocolXML2(xdrs, protocol_id, direction, token, str);
        }

//----------------------------------------------------------------------------
// End of Error Trap Loop

    } while (0);

    return err;
}