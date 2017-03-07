/*---------------------------------------------------------------
* Allocate Memory for the Data and Dim Blocks and Data Error Block
*
* Arguments:	DATA_BLOCK *		Data Block Structure
*
* Returns:	allocData		0 if heap allocation was successful
*		DATA_BLOCK->data	Pointer to Memory Block
*
*--------------------------------------------------------------*/

#include "allocData.h"

#include <logging/logging.h>
#include <clientserver/udaTypes.h>

#include "udaErrors.h"
#include "initStructs.h"

/**
 * Generic function to (Re)Allocate Memory for a typed Array
 * @param data_type
 * @param n_data
 * @param ap
 * @return
 */
int allocArray(int data_type, size_t n_data, char** ap)
{
    if (n_data == 0) return 0; // Insufficient Data to Allocate!

    size_t data_size = getSizeOf(data_type);
    if (data_size > 0) {
        *ap = (char*)realloc((void*)*ap, n_data * data_size);
    } else if (data_type != TYPE_COMPOUND) {
        return UNKNOWN_DATA_TYPE;
    }

    if (*ap == NULL && data_type != TYPE_COMPOUND) {
        return ERROR_ALLOCATING_HEAP;
    }

    return 0;
}

/**
 * Main routine for allocating memory for Data and Errors
 * @param data_block
 * @return
 */
int allocData(DATA_BLOCK* data_block)
{
    unsigned int ndata;
    char* db = NULL;
    char* ebh = NULL;
    char* ebl = NULL;

    //------------------------------------------------------------------------
    // Allocate Memory for data Dimensions

    if (data_block->rank > 0) {
        data_block->dims = (DIMS*)malloc(data_block->rank * sizeof(DIMS));
        if (data_block->dims == NULL) return ERROR_ALLOCATING_HEAP;
        int i;
        for (i = 0; i < data_block->rank; i++) {
            initDimBlock(&data_block->dims[i]);
        }
    }

    //------------------------------------------------------------------------
    // Allocate Memory for data and errors

    if ((ndata = (unsigned int)data_block->data_n) == 0) {
        return 1;
    }   // Insufficient Data to Allocate!

    size_t data_size = getSizeOf(data_block->data_type);

    if (data_size > 0) {
        db = (char*)malloc(ndata * data_size);

        if (data_block->error_type == TYPE_UNKNOWN) {
            ebh = (char*)malloc(ndata * data_size); // Will be same type as Data if Accessed
            if (data_block->errasymmetry) {
                ebl = (char*)malloc(ndata * data_size);
            }
        }
    } else {
        return UNKNOWN_DATA_TYPE;
    }

    IDAM_LOG(LOG_DEBUG, "allocData :\n");
    IDAM_LOGF(LOG_DEBUG, "rank      : %d\n", data_block->rank);
    IDAM_LOGF(LOG_DEBUG, "count     : %d\n", data_block->data_n);
    IDAM_LOGF(LOG_DEBUG, "data_type : %d\n", data_block->data_type);
    IDAM_LOGF(LOG_DEBUG, "error_type: %d\n", data_block->error_type);
    IDAM_LOGF(LOG_DEBUG, "data  != NULL: %d\n", db != NULL);
    IDAM_LOGF(LOG_DEBUG, "errhi != NULL: %d\n", ebh != NULL);
    IDAM_LOGF(LOG_DEBUG, "errlo != NULL: %d\n", ebl != NULL);

    if (db == NULL && data_block->data_type != TYPE_COMPOUND) {
        IDAM_LOG(LOG_DEBUG, "allocData: Unable to Allocate Heap Memory for Data \n");
        return (ERROR_ALLOCATING_HEAP);
    }

    size_t error_size = getSizeOf(data_block->error_type);

    if (error_size > 0) {
        ebh = (char*)malloc(ndata * error_size);
        if (data_block->errasymmetry) {
            ebl = (char*)malloc(ndata * error_size);
        }
    }

    if ((ebh == NULL || (ebl == NULL && data_block->errasymmetry)) &&
        (data_block->error_type != TYPE_COMPOUND && data_block->error_type != TYPE_UNKNOWN)) {
            return ERROR_ALLOCATING_HEAP;
    }

    data_block->data = db;
    data_block->errhi = ebh;
    data_block->errlo = ebl;

    return 0;
}


int allocDim(DATA_BLOCK* data_block)
{
// This routine is only called by the Client if data
// are NOT in a compressed form, when Heap is
// allocated by the Dimension Uncompression function.
//
// It may or may not be called by a Server Plugin.

    unsigned int ndata;
    char* db = NULL;
    char* ebh = NULL;
    char* ebl = NULL;

    int i;
    for (i = 0; i < data_block->rank; i++) {

        ndata = (unsigned int)data_block->dims[i].dim_n;

        if (ndata == 0) return 1;   // Insufficient Data to Allocate!

        size_t data_size = getSizeOf(data_block->dims[i].data_type);

        db = (char*)malloc(ndata * data_size);

        if (data_block->dims[i].error_type == TYPE_UNKNOWN) {
            ebh = (char*)malloc(ndata * data_size);
            if (data_block->dims[i].errasymmetry) {
                ebl = (char*)malloc(ndata * data_size);
            }
        } else {
            size_t error_size = getSizeOf(data_block->dims[i].error_type);

            ebh = (char*)malloc(ndata * error_size);
            if (data_block->dims[i].errasymmetry) {
                ebl = (char*)malloc(ndata * error_size);
            }
        }

        if (db == NULL) return ERROR_ALLOCATING_HEAP;
        if (ebh == NULL || (ebl == NULL && data_block->dims[i].errasymmetry)) return ERROR_ALLOCATING_HEAP;

        data_block->dims[i].dim = db;
        data_block->dims[i].errhi = ebh;
        data_block->dims[i].errlo = ebl;

        // Allocate Heap for Compressed Dimension Domains

        if (data_block->dims[i].compressed && data_block->dims[i].method > 0) {

            data_block->dims[i].sams = NULL;
            data_block->dims[i].offs = NULL;
            data_block->dims[i].ints = NULL;

            switch (data_block->dims[i].method) {
                case 1 :
                    data_block->dims[i].sams = (long*)malloc(data_block->dims[i].udoms * sizeof(long));
                    data_block->dims[i].offs = (char*)malloc(data_block->dims[i].udoms * data_size);
                    data_block->dims[i].ints = (char*)malloc(data_block->dims[i].udoms * data_size);
                    break;
                case 2 :
                    data_block->dims[i].offs = (char*)malloc(data_block->dims[i].udoms * data_size);
                    break;
                case 3 :
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
int allocPutData(PUTDATA_BLOCK* putData)
{
    unsigned int count;
    char* db = NULL;

    //------------------------------------------------------------------------
    // Allocate Memory for data

    if ((count = putData->count) == 0) return 1;   // Insufficient Data to Allocate!

    size_t data_size = getSizeOf(putData->data_type);
    if (data_size > 0) {
        db = (char*)malloc(count * data_size);
    } else {
        return UNKNOWN_DATA_TYPE;
    }

    IDAM_LOG(LOG_DEBUG, "allocPutData :\n");
    IDAM_LOGF(LOG_DEBUG, "rank      : %d\n", putData->rank);
    IDAM_LOGF(LOG_DEBUG, "count     : %d\n", putData->count);
    IDAM_LOGF(LOG_DEBUG, "data_type : %d\n", putData->data_type);
    IDAM_LOGF(LOG_DEBUG, "data  != NULL: %d\n", db != NULL);

    if (db == NULL && putData->data_type != TYPE_COMPOUND) {
        IDAM_LOG(LOG_DEBUG, "allocPutData: Unable to Allocate Heap Memory for Data \n");
        return (ERROR_ALLOCATING_HEAP);
    }

    putData->data = db;

// Shape of data

    if (putData->rank > 1) putData->shape = (int*)malloc(putData->rank * sizeof(int));

// Name of data

    if (putData->blockNameLength > 0) {
        putData->blockName = (char*)malloc((putData->blockNameLength + 1) * sizeof(char));
    } else {
        putData->blockName = NULL;
    }

    return 0;
}

void addIdamPutDataBlockList(PUTDATA_BLOCK* putDataBlock, PUTDATA_BLOCK_LIST* putDataBlockList)
{
    if (putDataBlockList->putDataBlock == NULL ||
        putDataBlockList->blockCount + 1 >= putDataBlockList->blockListSize) {
        putDataBlockList->putDataBlock = (PUTDATA_BLOCK*)realloc((void*)putDataBlockList->putDataBlock,
                                                                 (putDataBlockList->blockListSize +
                                                                  GROWPUTDATABLOCKLIST) * sizeof(PUTDATA_BLOCK));
        putDataBlockList->blockListSize = putDataBlockList->blockListSize + GROWPUTDATABLOCKLIST;
    }
    putDataBlockList->putDataBlock[putDataBlockList->blockCount++] = *putDataBlock;
}

