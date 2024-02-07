#include "serverProcessing.h"

#include <cstdlib>

#include "logging/logging.h"
#include <uda/types.h>

template <typename T> bool reduce_dim(DIMS* ddim)
{
    T sf = (T)0.0;
    switch (ddim->method) {
        case 1:
            for (unsigned int i = 0; i < ddim->udoms; i++) {
                for (int j = 0; j < *((long*)ddim->sams + i); j++) {
                    sf = sf + *((T*)ddim->offs + i) + (T)j * *((T*)ddim->ints + i);
                    if (sf != (T)0.0) {
                        return false;
                    }
                }
            }
            break;
        case 2:
            for (unsigned int i = 0; i < ddim->udoms; i++) {
                sf = sf + *((T*)ddim->offs + i);
                if (sf != (T)0.0) {
                    return false;
                }
            }
            break;
        case 3:
            for (unsigned int i = 0; i < ddim->udoms; i++) {
                sf = sf + *((T*)ddim->offs) + (T)i * *((T*)ddim->ints);
                if (sf != (T)0.0) {
                    return false;
                }
            }
            break;
    }
    return true;
}

/**
 * If the Rank is 1 and the dimensional data are compressed with zero values, then reduce the rank giving a scalar
 * array.
 *
 * @param data_block
 * @return
 */
int reduce_data(DATA_BLOCK* data_block)
{
    auto ddim = data_block->dims;
    if (ddim->compressed) {
        bool reduce = true;
        if (ddim->method == 0) {
            if (ddim->dim0 != (double)0.0 || ddim->diff == (double)0.0) {
                reduce = false;
            }
        } else {
            switch (ddim->data_type) {
                case UDA_TYPE_FLOAT:
                    reduce = reduce_dim<float>(ddim);
                    break;
                case UDA_TYPE_DOUBLE:
                    reduce = reduce_dim<double>(ddim);
                    break;
                case UDA_TYPE_CHAR:
                    reduce = reduce_dim<char>(ddim);
                    break;
                case UDA_TYPE_SHORT:
                    reduce = reduce_dim<short>(ddim);
                    break;
                case UDA_TYPE_INT:
                    reduce = reduce_dim<int>(ddim);
                    break;
                case UDA_TYPE_LONG:
                    reduce = reduce_dim<long>(ddim);
                    break;
                case UDA_TYPE_UNSIGNED_CHAR:
                    reduce = reduce_dim<unsigned char>(ddim);
                    break;
                case UDA_TYPE_UNSIGNED_SHORT:
                    reduce = reduce_dim<unsigned short>(ddim);
                    break;
                case UDA_TYPE_UNSIGNED_INT:
                    reduce = reduce_dim<unsigned int>(ddim);
                    break;
                case UDA_TYPE_UNSIGNED_LONG:
                    reduce = reduce_dim<unsigned long>(ddim);
                    break;
                default:
                    reduce = false;
                    break;
            }
        }
        if (reduce) {
            // Reduce the Rank to Scalar and free Dimensional Heap Memory
            data_block->order = -1;
            data_block->rank = 0;
            free(ddim->dim);
            free(ddim->errhi);
            free(ddim->errlo);
            free(ddim->sams);
            free(ddim->offs);
            free(ddim->ints);
            free(ddim);
            data_block->dims = nullptr;
        }
    }

    return 0;
}

template <typename T> int cast_dim(DIMS* ddim)
{
    switch (ddim->method) {
        case 1: {
            auto newoffs = (double*)malloc(ddim->udoms * sizeof(double));
            auto newints = (double*)malloc(ddim->udoms * sizeof(double));
            if (newoffs == nullptr || newints == nullptr) {
                free(newoffs);
                free(newints);
                return 1;
            }
            for (unsigned int i = 0; i < ddim->udoms; i++) {
                *(newoffs + i) = (double)*((T*)ddim->offs + i);
                *(newints + i) = (double)*((T*)ddim->ints + i);
            }
            free(ddim->offs);
            free(ddim->ints);
            ddim->offs = (char*)newoffs;
            ddim->ints = (char*)newints;
            ddim->data_type = UDA_TYPE_DOUBLE;
            break;
        }
        case 2: {
            auto newoffs = (double*)malloc(ddim->udoms * sizeof(double));
            for (unsigned int i = 0; i < ddim->udoms; i++) {
                *(newoffs + i) = (double)*((T*)ddim->offs + i);
            }
            free(ddim->offs);
            ddim->offs = (char*)newoffs;
            ddim->data_type = UDA_TYPE_DOUBLE;
            break;
        }
        case 3: {
            auto newoffs = (double*)malloc(sizeof(double));
            auto newints = (double*)malloc(sizeof(double));
            *newoffs = (double)*((T*)ddim->offs);
            *newints = (double)*((T*)ddim->ints);
            free(ddim->offs);
            free(ddim->ints);
            ddim->offs = (char*)newoffs;
            ddim->ints = (char*)newints;
            ddim->data_type = UDA_TYPE_DOUBLE;
            break;
        }
    }
    return 0;
}

/**
 * Cast the Time Dimension to Double Precision if the data are in a compressed format or ALL Dimensions if the data
 * are in compressed formats
 * @param data_block
 * @param client_block
 * @return
 */
int cast_data(DATA_BLOCK* data_block, const CLIENT_BLOCK* client_block)
{
    int rc = 0;
    for (unsigned int k = 0; k < data_block->rank; k++) {
        if (client_block->get_timedble && k != (unsigned int)data_block->order) {
            continue; // Only Process the Time Dimension
        }
        UDA_LOG(UDA_LOG_DEBUG, "Processing Dimension %d\n", k);
        DIMS* ddim = &data_block->dims[k];
        if (ddim->compressed) {
            if (ddim->method == 0) {
                ddim->data_type = UDA_TYPE_DOUBLE;
            } else {
                switch (ddim->data_type) {
                    case UDA_TYPE_CHAR:
                        rc = cast_dim<char>(ddim);
                        break;
                    case UDA_TYPE_SHORT:
                        rc = cast_dim<short>(ddim);
                        break;
                    case UDA_TYPE_INT:
                        rc = cast_dim<int>(ddim);
                        break;
                    case UDA_TYPE_LONG:
                        rc = cast_dim<long>(ddim);
                        break;
                    case UDA_TYPE_UNSIGNED_CHAR:
                        rc = cast_dim<unsigned char>(ddim);
                        break;
                    case UDA_TYPE_UNSIGNED_SHORT:
                        rc = cast_dim<unsigned short>(ddim);
                        break;
                    case UDA_TYPE_UNSIGNED_INT:
                        rc = cast_dim<unsigned int>(ddim);
                        break;
                    case UDA_TYPE_UNSIGNED_LONG:
                        rc = cast_dim<unsigned long>(ddim);
                        break;
                    case UDA_TYPE_FLOAT:
                        rc = cast_dim<float>(ddim);
                        break;
                    default:
                        break;
                }
            }
        }
    }
    return rc;
}

/**
 * UDA Server Side Data Processing
 *
 * @param client_block
 * @param data_block
 * @return 1 if an error occurred, otherwise 0
 */
int serverProcessing(CLIENT_BLOCK client_block, DATA_BLOCK* data_block)
{
    int rc = 0;

    if (client_block.get_scalar && data_block->rank == 1) {
        reduce_data(data_block);
    }

    UDA_LOG(UDA_LOG_DEBUG, "Server Side Processing\n");

    if (client_block.get_timedble || client_block.get_dimdble) {
        rc = cast_data(data_block, &client_block);
    }

    return rc;
}
