#include "handle.hpp"

#include "clientserver/initStructs.h"
#include "clientserver/udaStructs.h"
#include "uda/structured.h"

#include "thread_client.hpp"

void uda::client::free_handle(int handle)
{
    // Free Heap Memory (Not the Data Blocks themselves: These will be re-used.)

    char* cptr;
    DIMS* ddims;
    int rank;

    DATA_BLOCK* data_block = get_data_block(handle);

    if (data_block == nullptr) {
        return;
    }

    // Free Hierarchical structured data first

    switch (data_block->opaque_type) {
        case UDA_OPAQUE_TYPE_XML_DOCUMENT: {
            if (data_block->opaque_block != nullptr) {
                free(data_block->opaque_block);
            }

            data_block->opaque_count = 0;
            data_block->opaque_block = nullptr;
            data_block->opaque_type = UDA_OPAQUE_TYPE_UNKNOWN;
            break;
        }

        case UDA_OPAQUE_TYPE_STRUCTURES: {
            if (data_block->opaque_block != nullptr) {
                auto general_block = (GENERAL_BLOCK*)data_block->opaque_block;

                if (general_block->userdefinedtypelist != nullptr) {
                    freeUserDefinedTypeList(general_block->userdefinedtypelist);
                    free(general_block->userdefinedtypelist);
                }

                if (general_block->logmalloclist != nullptr) {
                    freeMallocLogList(general_block->logmalloclist);
                    free(general_block->logmalloclist);
                }
            }

            data_block->opaque_block = nullptr;
            data_block->data_type = UDA_TYPE_UNKNOWN;
            data_block->opaque_count = 0;
            data_block->opaque_type = UDA_OPAQUE_TYPE_UNKNOWN;
            data_block->data = nullptr;

            break;
        }

        case UDA_OPAQUE_TYPE_XDRFILE:
        case UDA_OPAQUE_TYPE_XDROBJECT: {
            if (data_block->opaque_block != nullptr) {
                free(data_block->opaque_block);
            }

            data_block->opaque_block = nullptr;
            data_block->data_type = UDA_TYPE_UNKNOWN;
            data_block->opaque_count = 0;
            data_block->opaque_type = UDA_OPAQUE_TYPE_UNKNOWN;
            data_block->data = nullptr;

            break;
        }

        default:
            break;
    }

    rank = data_block->rank;
    ddims = data_block->dims;

    if ((cptr = data_block->data) != nullptr) {
        free(cptr);
        data_block->data = nullptr; // Prevent another Free
    }

    if ((cptr = data_block->errhi) != nullptr) {
        free(cptr);
        data_block->errhi = nullptr;
    }

    if ((cptr = data_block->errlo) != nullptr) {
        free(cptr);
        data_block->errlo = nullptr;
    }

    if ((cptr = data_block->synthetic) != nullptr) {
        free(cptr);
        data_block->synthetic = nullptr;
    }

    if (data_block->data_system != nullptr) {
        free(data_block->data_system);
        data_block->data_system = nullptr;
    }

    if (data_block->system_config != nullptr) {
        free(data_block->system_config);
        data_block->system_config = nullptr;
    }

    if (data_block->data_source != nullptr) {
        free(data_block->data_source);
        data_block->data_source = nullptr;
    }

    if (data_block->signal_rec != nullptr) {
        free(data_block->signal_rec);
        data_block->signal_rec = nullptr;
    }

    if (data_block->signal_desc != nullptr) {
        free(data_block->signal_desc);
        data_block->signal_desc = nullptr;
    }

    if (ddims != nullptr && rank > 0) {
        for (int i = 0; i < rank; i++) {
            if ((cptr = data_block->dims[i].dim) != nullptr) {
                free(cptr);
            }

            if ((cptr = data_block->dims[i].synthetic) != nullptr) {
                free(cptr);
            }

            if ((cptr = data_block->dims[i].errhi) != nullptr) {
                free(cptr);
            }

            if ((cptr = data_block->dims[i].errlo) != nullptr) {
                free(cptr);
            }

            data_block->dims[i].dim = nullptr; // Prevent another Free
            data_block->dims[i].synthetic = nullptr;
            data_block->dims[i].errhi = nullptr;
            data_block->dims[i].errlo = nullptr;

            if ((cptr = (char*)data_block->dims[i].sams) != nullptr) {
                free(cptr);
            }

            if ((cptr = data_block->dims[i].offs) != nullptr) {
                free(cptr);
            }

            if ((cptr = data_block->dims[i].ints) != nullptr) {
                free(cptr);
            }

            data_block->dims[i].sams = nullptr;
            data_block->dims[i].offs = nullptr;
            data_block->dims[i].ints = nullptr;
        }

        free(ddims);
        data_block->dims = nullptr; // Prevent another Free
    }

    // closeIdamError(&server_block.idamerrorstack);

    initDataBlock(data_block);
    data_block->handle = -1; // Flag this as ready for re-use
}
