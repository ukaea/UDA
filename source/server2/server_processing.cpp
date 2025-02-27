#include "server_processing.h"

#include <cstdlib>

#include "logging/logging.h"
#include <uda/types.h>

using namespace uda::logging;

namespace {
template <typename T>
bool reduce_dimension(uda::client_server::Dims* dim) {
    bool reduce = true;

    T sum = static_cast<T>(0.0);
    switch (dim->method) {
        case 0:
            if (dim->dim0 != 0.0 || dim->diff == 0.0) {
                reduce = false;
            }
        break;

        case 1:
            for (unsigned int i = 0; i < dim->udoms; i++) {
                for (int j = 0; j < dim->sams[i]; j++) {
                    sum += *(reinterpret_cast<T*>(dim->offs) + i) + static_cast<T>(j) * *(reinterpret_cast<T*>(dim->ints) + i);
                    if (sum != static_cast<T>(0.0)) {
                        reduce = false;
                        break;
                    }
                }
            }
        break;

        case 2:
            for (unsigned int i = 0; i < dim->udoms; i++) {
                sum += *(reinterpret_cast<T*>(dim->offs) + i);
                if (sum != static_cast<T>(0.0)) {
                    reduce = false;
                    break;
                }
            }
        break;

        case 3:
            for (unsigned int i = 0; i < dim->udoms; i++) {
                sum += *reinterpret_cast<T*>(dim->offs) + static_cast<T>(i) * *reinterpret_cast<T*>(dim->ints);
                if (sum != static_cast<T>(0.0)) {
                    reduce = false;
                    break;
                }
            }
        break;

        default:
            throw std::invalid_argument("Invalid compression method");
    }

    return reduce;
}

template <typename T>
int convert_dim_to_double(uda::client_server::Dims* dim) {
    double* new_offs = nullptr;
    double* new_ints = nullptr;

    switch (dim->method) {
        case 0:
            dim->data_type = UDA_TYPE_DOUBLE;
            return 0;

        case 1:
            new_offs = (double*)malloc(dim->udoms * sizeof(double));
            new_ints = (double*)malloc(dim->udoms * sizeof(double));
            if (new_offs == nullptr || new_ints == nullptr) {
                free(new_offs);
                free(new_ints);
                return 999;
            }
            for (unsigned int i = 0; i < dim->udoms; i++) {
                new_offs[i] = reinterpret_cast<T*>(dim->offs)[i];
                new_ints[i] = reinterpret_cast<T*>(dim->ints)[i];
            }
            free(dim->offs);
            free(dim->ints);
            dim->offs = reinterpret_cast<char*>(new_offs);
            dim->ints = reinterpret_cast<char*>(new_ints);
            dim->data_type = UDA_TYPE_DOUBLE;
        break;

        case 2:
            new_offs = (double*)malloc(dim->udoms * sizeof(double));
            for (unsigned int i = 0; i < dim->udoms; i++) {
                new_offs[i] = reinterpret_cast<T*>(dim->offs)[i];
            }
            free(dim->offs);
            dim->offs = reinterpret_cast<char*>(new_offs);
            dim->data_type = UDA_TYPE_DOUBLE;
        break;

        case 3:
            new_offs = (double*)malloc(sizeof(double));
            new_ints = (double*)malloc(sizeof(double));
            *new_offs = *reinterpret_cast<T*>(dim->offs);
            *new_ints = *reinterpret_cast<T*>(dim->ints);
            free(dim->offs);
            free(dim->ints);
            dim->offs = reinterpret_cast<char*>(new_offs);
            dim->ints = reinterpret_cast<char*>(new_ints);
            dim->data_type = UDA_TYPE_DOUBLE;
        break;
    }

    return 0;
}

} // anon namespace

/**
 * UDA Server Side Data Processing
 *
 * @param client_block
 * @param data_block
 * @return 1 if an error occurred, otherwise 0
 */
int uda::server::server_processing(client_server::ClientBlock client_block, client_server::DataBlock* data_block)
{
    if (client_block.get_scalar && data_block->rank == 1) {
        //--------------------------------------------------------------------------------------------------
        // If the Rank is 1 and the dimensional data are compressed with zero values, then reduce the
        // rank giving a scalar array

        client_server::Dims* dim = data_block->dims;
        if (dim->compressed) {
            bool reduce = true;
            switch (dim->data_type) {
                case UDA_TYPE_FLOAT: reduce = reduce_dimension<float>(dim); break;
                case UDA_TYPE_DOUBLE: reduce = reduce_dimension<double>(dim); break;
                case UDA_TYPE_CHAR: reduce = reduce_dimension<char>(dim); break;
                case UDA_TYPE_SHORT: reduce = reduce_dimension<short>(dim); break;
                case UDA_TYPE_INT: reduce = reduce_dimension<int>(dim); break;
                case UDA_TYPE_LONG: reduce = reduce_dimension<long>(dim); break;
                case UDA_TYPE_UNSIGNED_CHAR: reduce = reduce_dimension<unsigned char>(dim); break;
                case UDA_TYPE_UNSIGNED_SHORT: reduce = reduce_dimension<unsigned short>(dim); break;
                case UDA_TYPE_UNSIGNED_INT: reduce = reduce_dimension<unsigned int>(dim); break;
                case UDA_TYPE_UNSIGNED_LONG: reduce = reduce_dimension<unsigned long>(dim); break;
                default:
                    reduce = false;
                    break;
            }
            if (reduce) {
                // Reduce the Rank to Scalar and free Dimensional Heap Memory
                data_block->order = -1;
                data_block->rank = 0;
                free(dim->dim);
                free(dim->errhi);
                free(dim->errlo);
                free(dim->sams);
                free(dim->offs);
                free(dim->ints);
                free(dim);
                data_block->dims = nullptr;
            }
        }
    }

    //--------------------------------------------------------------------------------------------------
    // Cast the Time Dimension to Double Precision if the data are in a compressed format
    // or ALL Dimensions if the data are in compressed formats

    UDA_LOG(UDA_LOG_DEBUG, "Server Side Processing");

    if (client_block.get_timedble || client_block.get_dimdble) {
        for (unsigned int k = 0; k < data_block->rank; k++) {
            if (client_block.get_timedble && k != static_cast<unsigned int>(data_block->order)) {
                continue; // Only Process the Time Dimension
            }
            UDA_LOG(UDA_LOG_DEBUG, "Processing Dimension {}", k);
            client_server::Dims* dim = &data_block->dims[k];
            if (dim->compressed) {
                int rc = 0;

                switch (dim->data_type) {
                    case UDA_TYPE_CHAR: rc = convert_dim_to_double<char>(dim); break;
                    case UDA_TYPE_SHORT: rc = convert_dim_to_double<short>(dim); break;
                    case UDA_TYPE_INT: rc = convert_dim_to_double<int>(dim); break;
                    case UDA_TYPE_LONG: rc = convert_dim_to_double<long>(dim); break;
                    case UDA_TYPE_UNSIGNED_CHAR: rc = convert_dim_to_double<unsigned char>(dim); break;
                    case UDA_TYPE_UNSIGNED_SHORT: rc = convert_dim_to_double<unsigned short>(dim); break;
                    case UDA_TYPE_UNSIGNED_INT: rc = convert_dim_to_double<unsigned int>(dim); break;
                    case UDA_TYPE_UNSIGNED_LONG: rc = convert_dim_to_double<unsigned long>(dim); break;
                    case UDA_TYPE_FLOAT: rc = convert_dim_to_double<float>(dim); break;
                    default: break;
                }

                if (rc != 0) {
                    return rc;
                }
            }
        }
    }

    return 0;
}
