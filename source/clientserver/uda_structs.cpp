#include "uda_structs.h"

#include "logging/logging.h"

#include <uda/types.h>

using namespace uda::logging;

bool uda::client_server::MetaData::contains(const std::string_view name) const {
    for (const auto& field : fields) {
        if (name == field.name.data()) {
            return true;
        }
    }
    return false;
}

std::string_view uda::client_server::MetaData::find(const std::string_view name) const {
    for (const auto& field : fields) {
        if (name == field.name.data()) {
            return {field.name.data(), strnlen(field.name.data(), field.name.size() - 1)};
        }
    }
    return {nullptr, 0};
}

void uda::client_server::free_put_data_block_list(PutDataBlockList* putDataBlockList)
{
    if (putDataBlockList->putDataBlock != nullptr && putDataBlockList->blockListSize > 0) {
        free(putDataBlockList->putDataBlock);
    }
    //    init_put_data_block_list(putDataBlockList);
}

// void freeRequestData(RequestData* request_data)
//{
//     free_name_value_list(&request_data->nameValueList);
//     free_put_data_block_list(&request_data->putDataBlockList);
// }

void uda::client_server::free_request_block(RequestBlock* request_block)
{
    //    for (int i = 0; i < request_block->num_requests; ++i) {
    //        freeRequestData(&request_block->requests[0]);
    //    }
    //    free(request_block->requests);
    //    request_block->num_requests = 0;
    //    request_block->requests = nullptr;
}

void uda::client_server::free_client_put_data_block_list(PutDataBlockList* putDataBlockList)
{
    if (putDataBlockList->putDataBlock != nullptr && putDataBlockList->blockListSize > 0) {
        free(putDataBlockList->putDataBlock);
    }
    //    init_put_data_block_list(putDataBlockList);
}

void uda::client_server::free_data_block(DataBlock* data_block)
{
    // Free Heap Memory & Zero all Integer values

    void* cptr;
    Dims* ddims;
    unsigned int rank;

    UDA_LOG(UDA_LOG_DEBUG, "Enter");

    if (data_block != nullptr) {

        UDA_LOG(UDA_LOG_DEBUG, "Opaque Data");

        switch (data_block->opaque_type) {
            case UDA_OPAQUE_TYPE_XML_DOCUMENT: {
                if (data_block->opaque_block != nullptr) {
                    free(data_block->opaque_block);
                }
                data_block->opaque_count = 0;
                data_block->opaque_block = nullptr;
                break;
            }

            case UDA_OPAQUE_TYPE_STRUCTURES: {
                if (data_block->opaque_block != nullptr) {
                    //                    if (logmalloclist != nullptr) {
                    //                        udaFreeMallocLogList(logmalloclist);
                    //                        free(logmalloclist);
                    //                        logmalloclist = nullptr;
                    //                    }

                    data_block->opaque_count = 0;
                    data_block->opaque_block = nullptr;
                    data_block->data_type = UDA_TYPE_UNKNOWN;
                    data_block->opaque_type = UDA_OPAQUE_TYPE_UNKNOWN;

                    data_block->data = nullptr; // Added to Malloc Log List for freeing
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

        UDA_LOG(UDA_LOG_DEBUG, "freeing Data");

        rank = data_block->rank;
        ddims = data_block->dims;

        if ((cptr = (void*)data_block->data) != nullptr) {
            free(cptr);
        }
        if ((cptr = (void*)data_block->errhi) != nullptr) {
            free(cptr);
        }
        if ((cptr = (void*)data_block->errlo) != nullptr) {
            free(cptr);
        }

        data_block->data = nullptr;
        data_block->errhi = nullptr;
        data_block->errlo = nullptr;

        UDA_LOG(UDA_LOG_DEBUG, "freeing Dimensions - Rank = {} ", rank);
        UDA_LOG(UDA_LOG_DEBUG, "Dim Structure Location {} ", (void*)ddims);

        if (ddims != nullptr) {
            for (unsigned int i = 0; i < rank; i++) {

                UDA_LOG(UDA_LOG_DEBUG, "Dimension[{}] ", i);
                UDA_LOG(UDA_LOG_DEBUG, "Dimension Data");

                if ((cptr = (void*)ddims[i].dim) != nullptr) {
                    free(cptr);
                }

                UDA_LOG(UDA_LOG_DEBUG, "Dimension Error Hi");

                if ((cptr = (void*)ddims[i].errhi) != nullptr) {
                    free(cptr);
                }

                UDA_LOG(UDA_LOG_DEBUG, "Dimension Error Lo");

                if ((cptr = (void*)ddims[i].errlo) != nullptr) {
                    free(cptr);
                }

                UDA_LOG(UDA_LOG_DEBUG, "Dimension Sams");

                if ((cptr = (void*)ddims[i].sams) != nullptr) {
                    free(cptr);
                }

                UDA_LOG(UDA_LOG_DEBUG, "Dimension offs");

                if ((cptr = (void*)ddims[i].offs) != nullptr) {
                    free(cptr);
                }

                UDA_LOG(UDA_LOG_DEBUG, "Dimension ints");

                if ((cptr = (void*)ddims[i].ints) != nullptr) {
                    free(cptr);
                }

                data_block->dims[i].dim = nullptr;
                data_block->dims[i].errhi = nullptr;
                data_block->dims[i].errlo = nullptr;
                data_block->dims[i].sams = nullptr;
                data_block->dims[i].offs = nullptr;
                data_block->dims[i].ints = nullptr;
            }

            UDA_LOG(UDA_LOG_DEBUG, "Dimension Array");

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

    UDA_LOG(UDA_LOG_DEBUG, "Exit");
}

void uda::client_server::free_data_block_list(std::vector<DataBlock>& data_block_list)
{
    for (size_t i = 0; i < data_block_list.size(); ++i) {
        free_data_block(&data_block_list[i]);
    }
    data_block_list.clear();
}

// Free Heap Memory & Zero all Integer values
void uda::client_server::free_reduced_data_block(DataBlock* data_block)
{
#ifdef FATCLIENT
    if (data_block == nullptr) {
        return;
    }
    if (data_block->opaque_type != UDA_OPAQUE_TYPE_STRUCTURES) {
        return;
    }
    if (data_block->opaque_block == nullptr) {
        return;
    }

    //    if(logmalloclist != nullptr) {
    //        udaFreeMallocLogList(logmalloclist);
    //        free((void *)logmalloclist);
    //        logmalloclist = nullptr;
    //    }

    data_block->opaque_count = 0;
    data_block->opaque_block = nullptr;
    data_block->data_type = UDA_TYPE_UNKNOWN;
    data_block->opaque_type = UDA_OPAQUE_TYPE_UNKNOWN;

    data_block->data = nullptr; // Added to Malloc Log List for freeing
    return;
#endif
}
