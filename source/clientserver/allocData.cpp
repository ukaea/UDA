/*---------------------------------------------------------------
 * Allocate Memory for the Data and Dim Blocks and Data Error Block
 *
 * Arguments:    DataBlock *        Data Block Structure
 *
 * Returns:    allocData        0 if heap allocation was successful
 *        DataBlock->data    Pointer to Memory Block
 *
 *--------------------------------------------------------------*/

#include "allocData.h"

#include "logging/logging.h"
#include <uda/types.h>

#include "initStructs.h"
#include "udaErrors.h"

using namespace uda::logging;

/**
 * Generic function to (Re)Allocate Memory for a typed Array
 * @param data_type
 * @param n_data
 * @param ap
 * @return
 */
int uda::client_server::alloc_array(int data_type, size_t n_data, char** ap)
{
    if (n_data == 0) {
        return 0; // Insufficient Data to Allocate!
    }

    *ap = nullptr;

    size_t data_size = getSizeOf((UDA_TYPE)data_type);
    if (data_size > 0) {
        *ap = (char*)realloc((void*)*ap, n_data * data_size);
    } else if (data_type != UDA_TYPE_COMPOUND) {
        return UNKNOWN_DATA_TYPE;
    }

    if (*ap == nullptr && data_type != UDA_TYPE_COMPOUND) {
        return ERROR_ALLOCATING_HEAP;
    }

    return 0;
}

/**
 * Main routine for allocating memory for Data and Errors
 * @param data_block
 * @return
 */
int uda::client_server::alloc_data(DataBlock* data_block)
{
    //------------------------------------------------------------------------
    // Allocate Memory for data Dimensions

    if (data_block->rank > 0) {
        data_block->dims = (Dims*)malloc(data_block->rank * sizeof(Dims));
        if (data_block->dims == nullptr) {
            return ERROR_ALLOCATING_HEAP;
        }
        for (unsigned int i = 0; i < data_block->rank; i++) {
            init_dim_block(&data_block->dims[i]);
        }
    }

    //------------------------------------------------------------------------
    // Allocate Memory for data and errors

    unsigned int ndata;
    if ((ndata = (unsigned int)data_block->data_n) == 0) {
        // Insufficient Data to Allocate!
        return 1;
    }

    char* db = nullptr;
    char* ebh = nullptr;
    char* ebl = nullptr;

    size_t data_size = getSizeOf((UDA_TYPE)data_block->data_type);
    if (data_size > 0) {
        db = (char*)malloc(ndata * data_size);
        if (data_block->error_type == UDA_TYPE_UNKNOWN) {
            ebh = (char*)malloc(ndata * data_size);
            if (data_block->errasymmetry) {
                ebl = (char*)malloc(ndata * data_size);
            }
        }
    } else if (data_block->data_type != UDA_TYPE_COMPOUND) {
        return UNKNOWN_DATA_TYPE;
    }

    UDA_LOG(UDA_LOG_DEBUG, "allocData :");
    UDA_LOG(UDA_LOG_DEBUG, "rank      : {}", data_block->rank);
    UDA_LOG(UDA_LOG_DEBUG, "count     : {}", data_block->data_n);
    UDA_LOG(UDA_LOG_DEBUG, "data_type : {}", data_block->data_type);
    UDA_LOG(UDA_LOG_DEBUG, "error_type: {}", data_block->error_type);
    UDA_LOG(UDA_LOG_DEBUG, "data  != nullptr: {}", db != nullptr);
    UDA_LOG(UDA_LOG_DEBUG, "errhi != nullptr: {}", ebh != nullptr);
    UDA_LOG(UDA_LOG_DEBUG, "errlo != nullptr: {}", ebl != nullptr);

    if (db == nullptr && data_block->data_type != UDA_TYPE_COMPOUND) {
        UDA_LOG(UDA_LOG_DEBUG, "allocData: Unable to Allocate Heap Memory for Data");
        return ERROR_ALLOCATING_HEAP;
    }

    size_t error_size = getSizeOf((UDA_TYPE)data_block->error_type);
    if (error_size > 0) {
        ebh = (char*)malloc(ndata * error_size);
        if (data_block->errasymmetry) {
            ebl = (char*)malloc(ndata * error_size);
        }
    }

    if ((ebh == nullptr || (ebl == nullptr && data_block->errasymmetry)) &&
        (data_block->error_type != UDA_TYPE_COMPOUND && data_block->error_type != UDA_TYPE_UNKNOWN)) {
        return ERROR_ALLOCATING_HEAP;
    }

    data_block->data = db;
    data_block->errhi = ebh;
    data_block->errlo = ebl;

    return 0;
}

int uda::client_server::alloc_dim(DataBlock* data_block)
{
    // This routine is only called by the Client if data
    // are NOT in a compressed form, when Heap is
    // allocated by the Dimension Uncompression function.
    //
    // It may or may not be called by a Server Plugin.

    unsigned int ndata;
    char* db = nullptr;
    char* ebh = nullptr;
    char* ebl = nullptr;

    for (unsigned int i = 0; i < data_block->rank; i++) {

        ndata = (unsigned int)data_block->dims[i].dim_n;

        if (ndata == 0) {
            return 1; // Insufficient Data to Allocate!
        }

        size_t data_size = getSizeOf((UDA_TYPE)data_block->dims[i].data_type);

        db = (char*)malloc(ndata * data_size);

        if (data_block->dims[i].error_type == UDA_TYPE_UNKNOWN) {
            ebh = (char*)malloc(ndata * data_size);
            if (data_block->dims[i].errasymmetry) {
                ebl = (char*)malloc(ndata * data_size);
            }
        } else {
            size_t error_size = getSizeOf((UDA_TYPE)data_block->dims[i].error_type);

            ebh = (char*)malloc(ndata * error_size);
            if (data_block->dims[i].errasymmetry) {
                ebl = (char*)malloc(ndata * error_size);
            }
        }

        if (db == nullptr) {
            return ERROR_ALLOCATING_HEAP;
        }
        if (ebh == nullptr || (ebl == nullptr && data_block->dims[i].errasymmetry)) {
            return ERROR_ALLOCATING_HEAP;
        }

        data_block->dims[i].dim = db;
        data_block->dims[i].errhi = ebh;
        data_block->dims[i].errlo = ebl;

        // Allocate Heap for Compressed Dimension Domains

        if (data_block->dims[i].compressed && data_block->dims[i].method > 0) {

            data_block->dims[i].sams = nullptr;
            data_block->dims[i].offs = nullptr;
            data_block->dims[i].ints = nullptr;

            switch (data_block->dims[i].method) {
                case 1:
                    data_block->dims[i].sams = (int*)malloc(data_block->dims[i].udoms * sizeof(int));
                    data_block->dims[i].offs = (char*)malloc(data_block->dims[i].udoms * data_size);
                    data_block->dims[i].ints = (char*)malloc(data_block->dims[i].udoms * data_size);
                    break;
                case 2:
                    data_block->dims[i].offs = (char*)malloc(data_block->dims[i].udoms * data_size);
                    break;
                case 3:
                    data_block->dims[i].offs = (char*)malloc(data_size);
                    data_block->dims[i].ints = (char*)malloc(data_size);
                    break;
            }
        }
    }

    return 0;
}

/**
 * Main routine for allocating memory for 'PUT' Data
 * @param putData
 * @return
 */
int uda::client_server::alloc_put_data(PutDataBlock* putData)
{
    unsigned int count;
    char* db = nullptr;

    //------------------------------------------------------------------------
    // Allocate Memory for data

    if ((count = putData->count) == 0) {
        return 1; // Insufficient Data to Allocate!
    }

    size_t data_size = getSizeOf((UDA_TYPE)putData->data_type);
    if (data_size > 0) {
        db = (char*)malloc(count * data_size);
    } else {
        return UNKNOWN_DATA_TYPE;
    }

    UDA_LOG(UDA_LOG_DEBUG, "allocPutData :");
    UDA_LOG(UDA_LOG_DEBUG, "rank      : {}", putData->rank);
    UDA_LOG(UDA_LOG_DEBUG, "count     : {}", putData->count);
    UDA_LOG(UDA_LOG_DEBUG, "data_type : {}", putData->data_type);
    UDA_LOG(UDA_LOG_DEBUG, "data  != nullptr: {}", db != nullptr);

    if (db == nullptr && putData->data_type != UDA_TYPE_COMPOUND) {
        UDA_LOG(UDA_LOG_DEBUG, "allocPutData: Unable to Allocate Heap Memory for Data");
        return ERROR_ALLOCATING_HEAP;
    }

    putData->data = db;

    // Shape of data

    if (putData->rank > 0) {
        putData->shape = (int*)malloc(putData->rank * sizeof(int));
    }

    // Name of data

    if (putData->blockNameLength > 0) {
        putData->blockName = (char*)malloc((putData->blockNameLength + 1) * sizeof(char));
    } else {
        putData->blockName = nullptr;
    }

    return 0;
}

void uda::client_server::add_put_data_block_list(PutDataBlock* putDataBlock, PutDataBlockList* putDataBlockList)
{
    if (putDataBlockList->putDataBlock == nullptr ||
        putDataBlockList->blockCount + 1 >= putDataBlockList->blockListSize) {
        putDataBlockList->putDataBlock =
            (PutDataBlock*)realloc((void*)putDataBlockList->putDataBlock,
                                   (putDataBlockList->blockListSize + GrowPutdataBlockList) * sizeof(PutDataBlock));
        putDataBlockList->blockListSize = putDataBlockList->blockListSize + GrowPutdataBlockList;
    }
    putDataBlockList->putDataBlock[putDataBlockList->blockCount++] = *putDataBlock;
}
