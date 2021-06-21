#include "udaStructs.h"

#include <logging/logging.h>

#include "initStructs.h"
#include "udaTypes.h"
#include "makeRequestBlock.h"

void freePutDataBlockList(PUTDATA_BLOCK_LIST* putDataBlockList)
{
    if (putDataBlockList->putDataBlock != nullptr && putDataBlockList->blockListSize > 0) {
        free(putDataBlockList->putDataBlock);
    }
    initPutDataBlockList(putDataBlockList);
}

void freeRequestData(REQUEST_DATA* request_data)
{
    freeNameValueList(&request_data->nameValueList);
    freePutDataBlockList(&request_data->putDataBlockList);
}

void freeRequestBlock(REQUEST_BLOCK* request_block)
{
    for (int i = 0; i < request_block->num_requests; ++i) {
        freeRequestData(&request_block->requests[0]);
    }
    free(request_block->requests);
    request_block->num_requests = 0;
    request_block->requests = nullptr;
}

void freeClientPutDataBlockList(PUTDATA_BLOCK_LIST* putDataBlockList)
{
    if (putDataBlockList->putDataBlock != nullptr && putDataBlockList->blockListSize > 0) {
        free(putDataBlockList->putDataBlock);
    }
    initPutDataBlockList(putDataBlockList);
}

void freeDataBlock(DATA_BLOCK* data_block)
{
    // Free Heap Memory & Zero all Integer values

    void* cptr;
    DIMS* ddims;
    unsigned int rank;

    UDA_LOG(UDA_LOG_DEBUG, "Enter\n");

    if (data_block != nullptr) {

        UDA_LOG(UDA_LOG_DEBUG, "Opaque Data\n");

        switch (data_block->opaque_type) {
            case UDA_OPAQUE_TYPE_XML_DOCUMENT: {
                if (data_block->opaque_block != nullptr) free(data_block->opaque_block);
                data_block->opaque_count = 0;
                data_block->opaque_block = nullptr;
                break;
            }

            case UDA_OPAQUE_TYPE_STRUCTURES: {
                if (data_block->opaque_block != nullptr) {
//                    if (logmalloclist != nullptr) {
//                        freeMallocLogList(logmalloclist);
//                        free(logmalloclist);
//                        logmalloclist = nullptr;
//                    }

                    data_block->opaque_count = 0;
                    data_block->opaque_block = nullptr;
                    data_block->data_type = UDA_TYPE_UNKNOWN;
                    data_block->opaque_type = UDA_OPAQUE_TYPE_UNKNOWN;

                    data_block->data = nullptr;        // Added to Malloc Log List for freeing
                }
                break;
            }

            case UDA_OPAQUE_TYPE_XDRFILE: {
                if (data_block->opaque_block != nullptr) {
                    free(data_block->opaque_block);
                }
                data_block->opaque_count = 0;
                data_block->opaque_block = nullptr;
                data_block->data_type = UDA_TYPE_UNKNOWN;
                data_block->opaque_type = UDA_OPAQUE_TYPE_UNKNOWN;
                data_block->data = nullptr;
                break;
            }

            case UDA_OPAQUE_TYPE_XDROBJECT: {
                if (data_block->opaque_block != nullptr) {
                    free(data_block->opaque_block);
                }
                data_block->opaque_count = 0;
                data_block->opaque_block = nullptr;
                data_block->data_type = UDA_TYPE_UNKNOWN;
                data_block->opaque_type = UDA_OPAQUE_TYPE_UNKNOWN;
                data_block->data = nullptr;
                break;
            }

            default:
                break;
        }

        UDA_LOG(UDA_LOG_DEBUG, "freeing Data\n");

        rank = data_block->rank;
        ddims = data_block->dims;

        if ((cptr = (void*)data_block->data) != nullptr) free(cptr);
        if ((cptr = (void*)data_block->errhi) != nullptr) free(cptr);
        if ((cptr = (void*)data_block->errlo) != nullptr) free(cptr);

        data_block->data = nullptr;
        data_block->errhi = nullptr;
        data_block->errlo = nullptr;

        UDA_LOG(UDA_LOG_DEBUG, "freeing Dimensions - Rank = %d \n", rank);
        UDA_LOG(UDA_LOG_DEBUG, "Dim Structure Location %p \n", ddims);

        if (ddims != nullptr) {
            for (unsigned int i = 0; i < rank; i++) {

                UDA_LOG(UDA_LOG_DEBUG, "Dimension[%d] \n", i);
                UDA_LOG(UDA_LOG_DEBUG, "Dimension Data \n");

                if ((cptr = (void*)ddims[i].dim) != nullptr) free(cptr);

                UDA_LOG(UDA_LOG_DEBUG, "Dimension Error Hi \n");

                if ((cptr = (void*)ddims[i].errhi) != nullptr) free(cptr);

                UDA_LOG(UDA_LOG_DEBUG, "Dimension Error Lo \n");

                if ((cptr = (void*)ddims[i].errlo) != nullptr) free(cptr);

                UDA_LOG(UDA_LOG_DEBUG, "Dimension Sams \n");

                if ((cptr = (void*)ddims[i].sams) != nullptr) free(cptr);

                UDA_LOG(UDA_LOG_DEBUG, "Dimension offs \n");

                if ((cptr = (void*)ddims[i].offs) != nullptr) free(cptr);

                UDA_LOG(UDA_LOG_DEBUG, "Dimension ints \n");

                if ((cptr = (void*)ddims[i].ints) != nullptr) free(cptr);

                data_block->dims[i].dim = nullptr;
                data_block->dims[i].errhi = nullptr;
                data_block->dims[i].errlo = nullptr;
                data_block->dims[i].sams = nullptr;
                data_block->dims[i].offs = nullptr;
                data_block->dims[i].ints = nullptr;
            }

            UDA_LOG(UDA_LOG_DEBUG, "Dimension Array \n");

            free(ddims);
            data_block->dims = nullptr;
        }

        data_block->handle = 0;
        data_block->errcode = 0;
        data_block->rank = 0;
        data_block->order = 0;
        data_block->data_type = UDA_TYPE_UNKNOWN;
        data_block->error_type = UDA_TYPE_UNKNOWN;
        data_block->data_n = 0;
        data_block->error_param_n = 0;

    }

    UDA_LOG(UDA_LOG_DEBUG, "Exit\n");
}

// Free Heap Memory & Zero all Integer values
void freeReducedDataBlock(DATA_BLOCK* data_block)
{
#ifdef FATCLIENT
    if(data_block == nullptr) return;
    if(data_block->opaque_type != UDA_OPAQUE_TYPE_STRUCTURES) return;
    if(data_block->opaque_block == nullptr) return;

//    if(logmalloclist != nullptr) {
//        freeMallocLogList(logmalloclist);
//        free((void *)logmalloclist);
//        logmalloclist = nullptr;
//    }

    data_block->opaque_count = 0;
    data_block->opaque_block = nullptr;
    data_block->data_type    = UDA_TYPE_UNKNOWN;
    data_block->opaque_type  = UDA_OPAQUE_TYPE_UNKNOWN;

    data_block->data         = nullptr;		// Added to Malloc Log List for freeing
    return;
#endif
}

