// Free Heap Memory
//-----------------------------------------------------------------------------

#include "freeDataBlock.h"

#include <stdlib.h>

#include <logging/logging.h>
#include <structures/struct.h>

#include "initStructs.h"

// It is assumed that data pointers within each putDataBlock are private to the client application
// and must be freed by the application.  

void freeIdamClientPutDataBlockList(PUTDATA_BLOCK_LIST* putDataBlockList)
{
    if (putDataBlockList->putDataBlock != NULL && putDataBlockList->blockListSize > 0) {
        free(putDataBlockList->putDataBlock);
    }
    initIdamPutDataBlockList(putDataBlockList);
}

void freeDataBlock(DATA_BLOCK* data_block)
{
    // Free Heap Memory & Zero all Integer values

    void* cptr;
    DIMS* ddims;
    unsigned int i;
    unsigned int rank;

    IDAM_LOG(UDA_LOG_DEBUG, "Enter\n");

    if (data_block != NULL) {

        IDAM_LOG(UDA_LOG_DEBUG, "Opaque Data\n");

        switch (data_block->opaque_type) {
            case UDA_OPAQUE_TYPE_XML_DOCUMENT: {
                if (data_block->opaque_block != NULL) free(data_block->opaque_block);
                data_block->opaque_count = 0;
                data_block->opaque_block = NULL;
                break;
            }

            case UDA_OPAQUE_TYPE_STRUCTURES: {
                if (data_block->opaque_block != NULL) {
//                    if (logmalloclist != NULL) {
//                        freeMallocLogList(logmalloclist);
//                        free(logmalloclist);
//                        logmalloclist = NULL;
//                    }

                    data_block->opaque_count = 0;
                    data_block->opaque_block = NULL;
                    data_block->data_type = UDA_TYPE_UNKNOWN;
                    data_block->opaque_type = UDA_OPAQUE_TYPE_UNKNOWN;

                    data_block->data = NULL;        // Added to Malloc Log List for freeing
                }
                break;
            }

            case UDA_OPAQUE_TYPE_XDRFILE: {
                if (data_block->opaque_block != NULL) {
                    free(data_block->opaque_block);
                }
                data_block->opaque_count = 0;
                data_block->opaque_block = NULL;
                data_block->data_type = UDA_TYPE_UNKNOWN;
                data_block->opaque_type = UDA_OPAQUE_TYPE_UNKNOWN;
                data_block->data = NULL;
                break;
            }

            case UDA_OPAQUE_TYPE_XDROBJECT: {
                if (data_block->opaque_block != NULL) {
                    free(data_block->opaque_block);
                }
                data_block->opaque_count = 0;
                data_block->opaque_block = NULL;
                data_block->data_type = UDA_TYPE_UNKNOWN;
                data_block->opaque_type = UDA_OPAQUE_TYPE_UNKNOWN;
                data_block->data = NULL;
                break;
            }

            default:
                break;
        }

        IDAM_LOG(UDA_LOG_DEBUG, "freeing Data\n");

        rank = data_block->rank;
        ddims = data_block->dims;

        if ((cptr = (void*)data_block->data) != NULL) free(cptr);
        if ((cptr = (void*)data_block->errhi) != NULL) free(cptr);
        if ((cptr = (void*)data_block->errlo) != NULL) free(cptr);

        data_block->data = NULL;
        data_block->errhi = NULL;
        data_block->errlo = NULL;

        IDAM_LOGF(UDA_LOG_DEBUG, "freeing Dimensions - Rank = %d \n", rank);
        IDAM_LOGF(UDA_LOG_DEBUG, "Dim Structure Location %p \n", ddims);

        if (ddims != NULL) {
            for (i = 0; i < rank; i++) {

                IDAM_LOGF(UDA_LOG_DEBUG, "Dimension[%d] \n", i);
                IDAM_LOG(UDA_LOG_DEBUG, "Dimension Data \n");

                if ((cptr = (void*)ddims[i].dim) != NULL) free(cptr);

                IDAM_LOG(UDA_LOG_DEBUG, "Dimension Error Hi \n");

                if ((cptr = (void*)ddims[i].errhi) != NULL) free(cptr);

                IDAM_LOG(UDA_LOG_DEBUG, "Dimension Error Lo \n");

                if ((cptr = (void*)ddims[i].errlo) != NULL) free(cptr);

                IDAM_LOG(UDA_LOG_DEBUG, "Dimension Sams \n");

                if ((cptr = (void*)ddims[i].sams) != NULL) free(cptr);

                IDAM_LOG(UDA_LOG_DEBUG, "Dimension offs \n");

                if ((cptr = (void*)ddims[i].offs) != NULL) free(cptr);

                IDAM_LOG(UDA_LOG_DEBUG, "Dimension ints \n");

                if ((cptr = (void*)ddims[i].ints) != NULL) free(cptr);

                data_block->dims[i].dim = NULL;
                data_block->dims[i].errhi = NULL;
                data_block->dims[i].errlo = NULL;
                data_block->dims[i].sams = NULL;
                data_block->dims[i].offs = NULL;
                data_block->dims[i].ints = NULL;
            }

            IDAM_LOG(UDA_LOG_DEBUG, "Dimension Array \n");

            free(ddims);
            data_block->dims = NULL;
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

    IDAM_LOG(UDA_LOG_DEBUG, "Exit\n");
}

// Free Heap Memory & Zero all Integer values
void freeReducedDataBlock(DATA_BLOCK* data_block)
{
#ifdef FATCLIENT
    if(data_block == NULL) return;
    if(data_block->opaque_type != UDA_OPAQUE_TYPE_STRUCTURES) return;
    if(data_block->opaque_block == NULL) return;

//    if(logmalloclist != NULL) {
//        freeMallocLogList(logmalloclist);
//        free((void *)logmalloclist);
//        logmalloclist = NULL;
//    }

    data_block->opaque_count = 0;
    data_block->opaque_block = NULL;
    data_block->data_type    = UDA_TYPE_UNKNOWN;
    data_block->opaque_type  = UDA_OPAQUE_TYPE_UNKNOWN;

    data_block->data         = NULL;		// Added to Malloc Log List for freeing
    return;
#endif
}

