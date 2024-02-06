/*---------------------------------------------------------------
 * Puts the passed Data to the serverside plugin
 *
 * Input Arguments:  1) plugin library call
 *                   2) the data passed to the plugin
 *
 * Returns:
 *--------------------------------------------------------------*/

#include <uda/client.h>

#include "clientserver/allocData.h"
#include "clientserver/errorLog.h"
#include "clientserver/printStructs.h"
#include "clientserver/initStructs.h"
#include "logging/logging.h"

#include "client/makeClientRequestBlock.h"
#include "client/udaClient.h"

// Create a list of data blocks to be sent to the server
// Each block has a private name
// Put data are available to server plugins as a set of named arguments in arbitrary order
// It is more efficient to send multiple blocks simultaneously when a Long Fat Pipe is involved
// Server side functions may require multiple array arguments, e.g. NAG routines

// Send multiple data blocks to the server

/* #ifndef FATCLIENT */
/* static unsigned short udaGetAPICalledOnce = 0; */
/* #endif */

int udaPutListAPI(const char* putInstruction, PUTDATA_BLOCK_LIST* inPutDataBlockList)
{

    int err = 0;
    REQUEST_BLOCK request_block;
    PUTDATA_BLOCK_LIST emptyPutDataBlockList;
    PUTDATA_BLOCK_LIST* putDataBlockList = nullptr;

    //-------------------------------------------------------------------------
    // Pass an empty structure rather than nullptr (Caller is responsible for freeing)

    if (inPutDataBlockList != nullptr) {
        putDataBlockList = inPutDataBlockList;
    } else {
        putDataBlockList = &emptyPutDataBlockList;
        udaInitPutDataBlockList(putDataBlockList);
    }

    //-------------------------------------------------------------------------
    // All client/server initialisation is controlled by the main API: udaGetAPI
    // This needs to have been called at least once before a put! - *** temporary fix!!!
    // This problem also causes the application malloclog and the userdefinedtypelist heaps to be overwritten.
    // Copy and replace to preserve the application heap

    /* #ifndef FATCLIENT */
    /*         if (!udaGetAPICalledOnce) { */
    /*       LOGMALLOCLIST* oldlogmalloclist = logmalloclist; */
    /*         USERDEFINEDTYPELIST* olduserdefinedtypelist = userdefinedtypelist; */
    /*         logmalloclist = nullptr; */
    /*         userdefinedtypelist = nullptr; */
    /*         int h = udaGetAPI("help::ping()", ""); */
    /*         udaFree(h); */
    /*         udaGetAPICalledOnce = 1; */
    /*         lastMallocIndex = 0; */
    /*         logmalloclist = oldlogmalloclist; */
    /*         userdefinedtypelist = olduserdefinedtypelist; */
    /*     } */
    /* #endif */

    //-------------------------------------------------------------------------
    // Initialise the Client Data Request Structure

    udaInitRequestBlock(&request_block);

    //------------------------------------------------------------------------------
    // Build the Request Data Block (Version and API dependent)

    const char* source = "";
    if ((err = makeClientRequestBlock(&putInstruction, &source, 1, &request_block)) != 0) {
        udaCloseError();
        if (udaNumErrors() == 0) {
            UDA_LOG(UDA_LOG_ERROR, "Error processing the put instruction [%s]\n", putInstruction);
            udaAddError(UDA_CODE_ERROR_TYPE, __func__, 999, "Error processing the put instruction");
        }
        return -err;
    }

    printRequestBlock(request_block);

    //-------------------------------------------------------------------------
    // Pass an empty structure rather than nullptr

    //-------------------------------------------------------------------------
    // Data to Put to the server

    request_block.requests[0].put = 1; // flags the direction of data (0 is default => get operation)
    request_block.requests[0].putDataBlockList = *putDataBlockList;

    int handle;
    err = idamClient(&request_block, &handle);
    if (err < 0) {
        handle = err;
    }

    return handle;
}

// Send a single data block to the server

int udaPutAPI(const char* putInstruction, PUTDATA_BLOCK* inPutData)
{
    int err = 0;
    REQUEST_BLOCK request_block;
    PUTDATA_BLOCK emptyPutDataBlock;
    PUTDATA_BLOCK* putData = nullptr;

    //-------------------------------------------------------------------------
    // Pass an empty structure rather than nullptr (Caller is responsible for freeing)

    if (inPutData != nullptr) {
        putData = inPutData;
    } else {
        putData = &emptyPutDataBlock;
        udaInitPutDataBlock(putData);
    }

    //-------------------------------------------------------------------------
    // All client/server initialisation is controlled by the main API: udaGetAPI
    // This needs to have been called at least once before a put! - *** temporary fix!!!
    // This problem also causes the application malloclog and the userdefinedtypelist heaps to be overwritten.
    // Copy and replace to preserve the application heap

    /* #ifndef FATCLIENT */
    /*     if (!udaGetAPICalledOnce) { */
    /*         UDA_LOG(LOG_DEBUG, "!udaGetAPICalledOnce\n"); */
    /*         LOGMALLOCLIST* oldlogmalloclist = logmalloclist; */
    /*         USERDEFINEDTYPELIST* olduserdefinedtypelist = userdefinedtypelist; */
    /*         logmalloclist = nullptr; */
    /*         userdefinedtypelist = nullptr; */
    /*         int h = udaGetAPI("help::ping()", ""); */
    /*         udaFree(h); */
    /*         udaGetAPICalledOnce = 1; */
    /*         lastMallocIndex = 0; */
    /*         logmalloclist = oldlogmalloclist; */
    /*         userdefinedtypelist = olduserdefinedtypelist; */
    /*     } */
    /* #endif */

    //-------------------------------------------------------------------------
    // Initialise the Client Data Request Structure

    udaInitRequestBlock(&request_block);

    //------------------------------------------------------------------------------
    // Build the Request Data Block (Version and API dependent)

    const char* source = "";
    if ((err = makeClientRequestBlock(&putInstruction, &source, 1, &request_block)) != 0) {
        udaCloseError();
        if (udaNumErrors() == 0) {
            UDA_LOG(UDA_LOG_ERROR, "Error processing the put instruction [%s]\n", putInstruction);
            udaAddError(UDA_CODE_ERROR_TYPE, __func__, 999, "Error processing the put instruction");
        }
        return -err;
    }

    printRequestBlock(request_block);

    //-------------------------------------------------------------------------
    // Data to Put to the server

    request_block.requests[0].put = 1; // flags the direction of data (0 is default => get operation)

    addIdamPutDataBlockList(putData, &request_block.requests[0].putDataBlockList);

    int handle;
    err = idamClient(&request_block, &handle);
    if (err < 0) {
        handle = err;
    }

    //-------------------------------------------------------------------------
    // Free Heap

    freeClientPutDataBlockList(&request_block.requests[0].putDataBlockList);

    return handle;
}
