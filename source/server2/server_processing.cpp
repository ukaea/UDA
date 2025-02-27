#include "server_processing.h"

#include <cstdlib>

#include "logging/logging.h"
#include <uda/types.h>

using namespace uda::logging;

/**
 * UDA Server Side Data Processing
 *
 * @param client_block
 * @param data_block
 * @return 1 if an error occurred, otherwise 0
 */
int uda::server::server_processing(client_server::ClientBlock client_block, client_server::DataBlock* data_block)
{
    uda::client_server::Dims* ddim = nullptr;
    double *new_offs = nullptr, *new_ints = nullptr;

    bool reduce = false;

    //--------------------------------------------------------------------------------------------------
    // If the Rank is 1 and the dimensional data are compressed with zero values, then reduce the
    // rank giving a scalar array

    if (client_block.get_scalar && data_block->rank == 1) {
        ddim = data_block->dims;
        if (ddim->compressed) {
            reduce = true;
            if (ddim->method == 0) {
                if (ddim->dim0 != (double)0.0 || ddim->diff == (double)0.0) {
                    reduce = false;
                }
            } else {
                switch (ddim->data_type) {

                    case UDA_TYPE_FLOAT: {
                        float sf = (float)0.0;
                        switch (ddim->method) {
                            case 1:
                                for (unsigned int i = 0; i < ddim->udoms; i++) {
                                    for (int j = 0; j < *((long*)ddim->sams + i); j++) {
                                        sf = sf + *((float*)ddim->offs + i) + (float)j * *((float*)ddim->ints + i);
                                        if (sf != (float)0.0) {
                                            reduce = false;
                                            break;
                                        }
                                    }
                                }
                                break;
                            case 2:
                                for (unsigned int i = 0; i < ddim->udoms; i++) {
                                    sf = sf + *((float*)ddim->offs + i);
                                    if (sf != (float)0.0) {
                                        reduce = false;
                                        break;
                                    }
                                }
                                break;
                            case 3:
                                for (unsigned int i = 0; i < ddim->udoms; i++) {
                                    sf = sf + *((float*)ddim->offs) + (float)i * *((float*)ddim->ints);
                                    if (sf != (float)0.0) {
                                        reduce = false;
                                        break;
                                    }
                                }
                                break;
                        }
                        break;
                    }

                    case UDA_TYPE_DOUBLE: {
                        double sd = (double)0.0;
                        switch (ddim->method) {
                            case 1:
                                for (unsigned int i = 0; i < ddim->udoms; i++) {
                                    for (int j = 0; j < *((long*)ddim->sams + i); j++) {
                                        sd = sd + *((double*)ddim->offs + i) + (double)j * *((double*)ddim->ints + i);
                                        if (sd != (double)0.0) {
                                            reduce = false;
                                            break;
                                        }
                                    }
                                }
                                break;
                            case 2:
                                for (unsigned int i = 0; i < ddim->udoms; i++) {
                                    sd = sd + *((double*)ddim->offs + i);
                                    if (sd != (double)0.0) {
                                        reduce = false;
                                        break;
                                    }
                                }
                                break;
                            case 3:
                                for (unsigned int i = 0; i < ddim->udoms; i++) {
                                    sd = sd + *((double*)ddim->offs) + (double)i * *((double*)ddim->ints);
                                    if (sd != (double)0.0) {
                                        reduce = false;
                                        break;
                                    }
                                }
                                break;
                        }
                        break;
                    }

                    case UDA_TYPE_CHAR: {
                        char sc = (char)0;
                        switch (ddim->method) {
                            case 1:
                                for (unsigned int i = 0; i < ddim->udoms; i++) {
                                    for (int j = 0; j < *((long*)ddim->sams + i); j++) {
                                        sc = sc + *((char*)ddim->offs + i) + (char)j * *((char*)ddim->ints + i);
                                        if (sc != (short)0) {
                                            reduce = false;
                                            break;
                                        }
                                    }
                                }
                                break;
                            case 2:
                                for (unsigned int i = 0; i < ddim->udoms; i++) {
                                    sc = sc + *((char*)ddim->offs + i);
                                    if (sc != (char)0) {
                                        reduce = false;
                                        break;
                                    }
                                }
                                break;
                            case 3:
                                for (unsigned int i = 0; i < ddim->udoms; i++) {
                                    sc = sc + *((char*)ddim->offs) + (char)i * *((char*)ddim->ints);
                                    if (sc != (char)0) {
                                        reduce = false;
                                        break;
                                    }
                                }
                                break;
                        }
                        break;
                    }

                    case UDA_TYPE_SHORT: {
                        short ss = (short)0;
                        switch (ddim->method) {
                            case 1:
                                for (unsigned int i = 0; i < ddim->udoms; i++) {
                                    for (int j = 0; j < *((long*)ddim->sams + i); j++) {
                                        ss = ss + *((short*)ddim->offs + i) + (short)j * *((short*)ddim->ints + i);
                                        if (ss != (short)0) {
                                            reduce = false;
                                            break;
                                        }
                                    }
                                }
                                break;
                            case 2:
                                for (unsigned int i = 0; i < ddim->udoms; i++) {
                                    ss = ss + *((short*)ddim->offs + i);
                                    if (ss != (short)0) {
                                        reduce = false;
                                        break;
                                    }
                                }
                                break;
                            case 3:
                                for (unsigned int i = 0; i < ddim->udoms; i++) {
                                    ss = ss + *((short*)ddim->offs) + (short)i * *((short*)ddim->ints);
                                    if (ss != (short)0) {
                                        reduce = false;
                                        break;
                                    }
                                }
                                break;
                        }
                        break;
                    }

                    case UDA_TYPE_INT: {
                        int si = (int)0;
                        switch (ddim->method) {
                            case 1:
                                for (unsigned int i = 0; i < ddim->udoms; i++) {
                                    for (int j = 0; j < *((long*)ddim->sams + i); j++) {
                                        si = si + *((int*)ddim->offs + i) + (int)j * *((int*)ddim->ints + i);
                                        if (si != (int)0) {
                                            reduce = false;
                                            break;
                                        }
                                    }
                                }
                                break;
                            case 2:
                                for (unsigned int i = 0; i < ddim->udoms; i++) {
                                    si = si + *((int*)ddim->offs + i);
                                    if (si != (int)0) {
                                        reduce = false;
                                        break;
                                    }
                                }
                                break;
                            case 3:
                                for (unsigned int i = 0; i < ddim->udoms; i++) {
                                    si = si + *((int*)ddim->offs) + (int)i * *((int*)ddim->ints);
                                    if (si != (int)0) {
                                        reduce = false;
                                        break;
                                    }
                                }
                                break;
                        }
                        break;
                    }

                    case UDA_TYPE_LONG: {
                        long sl = (long)0;
                        switch (ddim->method) {
                            case 1:
                                for (unsigned int i = 0; i < ddim->udoms; i++) {
                                    for (int j = 0; j < *((long*)ddim->sams + i); j++) {
                                        sl = sl + *((long*)ddim->offs + i) + (long)j * *((long*)ddim->ints + i);
                                        if (sl != (long)0) {
                                            reduce = false;
                                            break;
                                        }
                                    }
                                }
                                break;
                            case 2:
                                for (unsigned int i = 0; i < ddim->udoms; i++) {
                                    sl = sl + *((long*)ddim->offs + i);
                                    if (sl != (long)0) {
                                        reduce = false;
                                        break;
                                    }
                                }
                                break;
                            case 3:
                                for (unsigned int i = 0; i < ddim->udoms; i++) {
                                    sl = sl + *((long*)ddim->offs) + (long)i * *((long*)ddim->ints);
                                    if (sl != (long)0) {
                                        reduce = false;
                                        break;
                                    }
                                }
                                break;
                        }
                        break;
                    }

                    case UDA_TYPE_UNSIGNED_CHAR: {
                        unsigned char sc = (unsigned char)0;
                        switch (ddim->method) {
                            case 1:
                                for (unsigned int i = 0; i < ddim->udoms; i++) {
                                    for (int j = 0; j < *((long*)ddim->sams + i); j++) {
                                        sc = sc + *((unsigned char*)ddim->offs + i) +
                                             (unsigned char)j * *((unsigned char*)ddim->ints + i);
                                        if (sc != (short)0) {
                                            reduce = false;
                                            break;
                                        }
                                    }
                                }
                                break;
                            case 2:
                                for (unsigned int i = 0; i < ddim->udoms; i++) {
                                    sc = sc + *((unsigned char*)ddim->offs + i);
                                    if (sc != (unsigned char)0) {
                                        reduce = false;
                                        break;
                                    }
                                }
                                break;
                            case 3:
                                for (unsigned int i = 0; i < ddim->udoms; i++) {
                                    sc = sc + *((unsigned char*)ddim->offs) +
                                         (unsigned char)i * *((unsigned char*)ddim->ints);
                                    if (sc != (unsigned char)0) {
                                        reduce = false;
                                        break;
                                    }
                                }
                                break;
                        }
                        break;
                    }

                    case UDA_TYPE_UNSIGNED_SHORT: {
                        unsigned short ss = (unsigned short)0;
                        switch (ddim->method) {
                            case 1:
                                for (unsigned int i = 0; i < ddim->udoms; i++) {
                                    for (int j = 0; j < *((long*)ddim->sams + i); j++) {
                                        ss = ss + *((unsigned short*)ddim->offs + i) +
                                             (unsigned short)j * *((unsigned short*)ddim->ints + i);
                                        if (ss != (unsigned short)0) {
                                            reduce = false;
                                            break;
                                        }
                                    }
                                }
                                break;
                            case 2:
                                for (unsigned int i = 0; i < ddim->udoms; i++) {
                                    ss = ss + *((unsigned short*)ddim->offs + i);
                                    if (ss != (unsigned short)0) {
                                        reduce = false;
                                        break;
                                    }
                                }
                                break;
                            case 3:
                                for (unsigned int i = 0; i < ddim->udoms; i++) {
                                    ss = ss + *((unsigned short*)ddim->offs) +
                                         (unsigned short)i * *((unsigned short*)ddim->ints);
                                    if (ss != (unsigned short)0) {
                                        reduce = false;
                                        break;
                                    }
                                }
                                break;
                        }
                        break;
                    }

                    case UDA_TYPE_UNSIGNED_INT: {
                        unsigned int su = (unsigned int)0;
                        switch (ddim->method) {
                            case 1:
                                for (unsigned int i = 0; i < ddim->udoms; i++) {
                                    for (int j = 0; j < *((long*)ddim->sams + i); j++) {
                                        su = su + *((unsigned int*)ddim->offs + i) +
                                             (unsigned int)j * *((unsigned int*)ddim->ints + i);
                                        if (su != (unsigned int)0) {
                                            reduce = false;
                                            break;
                                        }
                                    }
                                }
                                break;
                            case 2:
                                for (unsigned int i = 0; i < ddim->udoms; i++) {
                                    su = su + *((unsigned int*)ddim->offs + i);
                                    if (su != (unsigned int)0) {
                                        reduce = false;
                                        break;
                                    }
                                }
                                break;
                            case 3:
                                for (unsigned int i = 0; i < ddim->udoms; i++) {
                                    su = su + *((unsigned int*)ddim->offs) +
                                         (unsigned int)i * *((unsigned int*)ddim->ints);
                                    if (su != (unsigned int)0) {
                                        reduce = false;
                                        break;
                                    }
                                }
                                break;
                        }
                        break;
                    }

                    case UDA_TYPE_UNSIGNED_LONG: {
                        unsigned long sl = (unsigned long)0;
                        switch (ddim->method) {
                            case 1:
                                for (unsigned int i = 0; i < ddim->udoms; i++) {
                                    for (int j = 0; j < *((long*)ddim->sams + i); j++) {
                                        sl = sl + *((unsigned long*)ddim->offs + i) +
                                             (unsigned long)j * *((unsigned long*)ddim->ints + i);
                                        if (sl != (unsigned long)0) {
                                            reduce = false;
                                            break;
                                        }
                                    }
                                }
                                break;
                            case 2:
                                for (unsigned int i = 0; i < ddim->udoms; i++) {
                                    sl = sl + *((unsigned long*)ddim->offs + i);
                                    if (sl != (unsigned long)0) {
                                        reduce = false;
                                        break;
                                    }
                                }
                                break;
                            case 3:
                                for (unsigned int i = 0; i < ddim->udoms; i++) {
                                    sl = sl + *((unsigned long*)ddim->offs) +
                                         (unsigned long)i * *((unsigned long*)ddim->ints);
                                    if (sl != (unsigned long)0) {
                                        reduce = false;
                                        break;
                                    }
                                }
                                break;
                        }
                        break;
                    }

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
    }

    //--------------------------------------------------------------------------------------------------
    // Cast the Time Dimension to Double Precision if the data are in a compressed format
    // or ALL Dimensions if the data are in compressed formats

    UDA_LOG(UDA_LOG_DEBUG, "Server Side Processing");

    if (client_block.get_timedble || client_block.get_dimdble) {
        for (unsigned int k = 0; k < data_block->rank; k++) {
            if (client_block.get_timedble && k != (unsigned int)data_block->order) {
                continue; // Only Process the Time Dimension
            }
            UDA_LOG(UDA_LOG_DEBUG, "Processing Dimension {}", k);
            ddim = data_block->dims + k;
            if (ddim->compressed) {
                if (ddim->method == 0) {
                    ddim->data_type = UDA_TYPE_DOUBLE;
                } else {
                    switch (ddim->data_type) {
                        case UDA_TYPE_CHAR:
                            switch (ddim->method) {
                                case 1:
                                    new_offs = (double*)malloc(ddim->udoms * sizeof(double));
                                    new_ints = (double*)malloc(ddim->udoms * sizeof(double));
                                    if (new_offs == nullptr || new_ints == nullptr) {
                                        if (new_offs != nullptr) {
                                            free(new_offs);
                                        }
                                        if (new_ints != nullptr) {
                                            free(new_ints);
                                        }
                                        return 1;
                                    }
                                    for (unsigned int i = 0; i < ddim->udoms; i++) {
                                        *(new_offs + i) = (double)*((char*)ddim->offs + i);
                                        *(new_ints + i) = (double)*((char*)ddim->ints + i);
                                    }
                                    if (ddim->offs != nullptr) {
                                        free(ddim->offs);
                                    }
                                    if (ddim->ints != nullptr) {
                                        free(ddim->ints);
                                    }
                                    ddim->offs = (char*)new_offs;
                                    ddim->ints = (char*)new_ints;
                                    ddim->data_type = UDA_TYPE_DOUBLE;
                                    break;
                                case 2:
                                    new_offs = (double*)malloc(ddim->udoms * sizeof(double));
                                    for (unsigned int i = 0; i < ddim->udoms; i++) {
                                        *(new_offs + i) = (double)*((char*)ddim->offs + i);
                                    }
                                    if (ddim->offs != nullptr) {
                                        free(ddim->offs);
                                    }
                                    ddim->offs = (char*)new_offs;
                                    ddim->data_type = UDA_TYPE_DOUBLE;
                                    break;
                                case 3:
                                    new_offs = (double*)malloc(sizeof(double));
                                    new_ints = (double*)malloc(sizeof(double));
                                    *new_offs = (double)*((char*)ddim->offs);
                                    *new_ints = (double)*((char*)ddim->ints);
                                    if (ddim->offs != nullptr) {
                                        free(ddim->offs);
                                    }
                                    if (ddim->ints != nullptr) {
                                        free(ddim->ints);
                                    }
                                    ddim->offs = (char*)new_offs;
                                    ddim->ints = (char*)new_ints;
                                    ddim->data_type = UDA_TYPE_DOUBLE;
                                    break;
                            }
                            break;

                        case UDA_TYPE_SHORT:
                            switch (ddim->method) {
                                case 1:
                                    new_offs = (double*)malloc(ddim->udoms * sizeof(double));
                                    new_ints = (double*)malloc(ddim->udoms * sizeof(double));
                                    if (new_offs == nullptr || new_ints == nullptr) {
                                        if (new_offs != nullptr) {
                                            free(new_offs);
                                        }
                                        if (new_ints != nullptr) {
                                            free(new_ints);
                                        }
                                        return 1;
                                    }
                                    for (unsigned int i = 0; i < ddim->udoms; i++) {
                                        *(new_offs + i) = (double)*((short*)ddim->offs + i);
                                        *(new_ints + i) = (double)*((short*)ddim->ints + i);
                                    }
                                    if (ddim->offs != nullptr) {
                                        free(ddim->offs);
                                    }
                                    if (ddim->ints != nullptr) {
                                        free(ddim->ints);
                                    }
                                    ddim->offs = (char*)new_offs;
                                    ddim->ints = (char*)new_ints;
                                    ddim->data_type = UDA_TYPE_DOUBLE;
                                    break;
                                case 2:
                                    new_offs = (double*)malloc(ddim->udoms * sizeof(double));
                                    for (unsigned int i = 0; i < ddim->udoms; i++) {
                                        *(new_offs + i) = (double)*((short*)ddim->offs + i);
                                    }
                                    if (ddim->offs != nullptr) {
                                        free(ddim->offs);
                                    }
                                    ddim->offs = (char*)new_offs;
                                    ddim->data_type = UDA_TYPE_DOUBLE;
                                    break;
                                case 3:
                                    new_offs = (double*)malloc(sizeof(double));
                                    new_ints = (double*)malloc(sizeof(double));
                                    *new_offs = (double)*((short*)ddim->offs);
                                    *new_ints = (double)*((short*)ddim->ints);
                                    if (ddim->offs != nullptr) {
                                        free(ddim->offs);
                                    }
                                    if (ddim->ints != nullptr) {
                                        free(ddim->ints);
                                    }
                                    ddim->offs = (char*)new_offs;
                                    ddim->ints = (char*)new_ints;
                                    ddim->data_type = UDA_TYPE_DOUBLE;
                                    break;
                            }
                            break;

                        case UDA_TYPE_INT:
                            switch (ddim->method) {
                                case 1:
                                    new_offs = (double*)malloc(ddim->udoms * sizeof(double));
                                    new_ints = (double*)malloc(ddim->udoms * sizeof(double));
                                    if (new_offs == nullptr || new_ints == nullptr) {
                                        if (new_offs != nullptr) {
                                            free(new_offs);
                                        }
                                        if (new_ints != nullptr) {
                                            free(new_ints);
                                        }
                                        return 1;
                                    }
                                    for (unsigned int i = 0; i < ddim->udoms; i++) {
                                        *(new_offs + i) = (double)*((int*)ddim->offs + i);
                                        *(new_ints + i) = (double)*((int*)ddim->ints + i);
                                    }
                                    if (ddim->offs != nullptr) {
                                        free(ddim->offs);
                                    }
                                    if (ddim->ints != nullptr) {
                                        free(ddim->ints);
                                    }
                                    ddim->offs = (char*)new_offs;
                                    ddim->ints = (char*)new_ints;
                                    ddim->data_type = UDA_TYPE_DOUBLE;
                                    break;
                                case 2:
                                    new_offs = (double*)malloc(ddim->udoms * sizeof(double));
                                    for (unsigned int i = 0; i < ddim->udoms; i++) {
                                        *(new_offs + i) = (double)*((int*)ddim->offs + i);
                                    }
                                    if (ddim->offs != nullptr) {
                                        free(ddim->offs);
                                    }
                                    ddim->offs = (char*)new_offs;
                                    ddim->data_type = UDA_TYPE_DOUBLE;
                                    break;
                                case 3:
                                    new_offs = (double*)malloc(sizeof(double));
                                    new_ints = (double*)malloc(sizeof(double));
                                    *new_offs = (double)*((int*)ddim->offs);
                                    *new_ints = (double)*((int*)ddim->ints);
                                    if (ddim->offs != nullptr) {
                                        free(ddim->offs);
                                    }
                                    if (ddim->ints != nullptr) {
                                        free(ddim->ints);
                                    }
                                    ddim->offs = (char*)new_offs;
                                    ddim->ints = (char*)new_ints;
                                    ddim->data_type = UDA_TYPE_DOUBLE;
                                    break;
                            }
                            break;

                        case UDA_TYPE_LONG:
                            switch (ddim->method) {
                                case 1:
                                    new_offs = (double*)malloc(ddim->udoms * sizeof(double));
                                    new_ints = (double*)malloc(ddim->udoms * sizeof(double));
                                    if (new_offs == nullptr || new_ints == nullptr) {
                                        if (new_offs != nullptr) {
                                            free(new_offs);
                                        }
                                        if (new_ints != nullptr) {
                                            free(new_ints);
                                        }
                                        return 1;
                                    }
                                    for (unsigned int i = 0; i < ddim->udoms; i++) {
                                        *(new_offs + i) = (double)*((long*)ddim->offs + i);
                                        *(new_ints + i) = (double)*((long*)ddim->ints + i);
                                    }
                                    if (ddim->offs != nullptr) {
                                        free(ddim->offs);
                                    }
                                    if (ddim->ints != nullptr) {
                                        free(ddim->ints);
                                    }
                                    ddim->offs = (char*)new_offs;
                                    ddim->ints = (char*)new_ints;
                                    ddim->data_type = UDA_TYPE_DOUBLE;
                                    break;
                                case 2:
                                    new_offs = (double*)malloc(ddim->udoms * sizeof(double));
                                    for (unsigned int i = 0; i < ddim->udoms; i++) {
                                        *(new_offs + i) = (double)*((long*)ddim->offs + i);
                                    }
                                    if (ddim->offs != nullptr) {
                                        free(ddim->offs);
                                    }
                                    ddim->offs = (char*)new_offs;
                                    ddim->data_type = UDA_TYPE_DOUBLE;
                                    break;
                                case 3:
                                    new_offs = (double*)malloc(sizeof(double));
                                    new_ints = (double*)malloc(sizeof(double));
                                    *new_offs = (double)*((long*)ddim->offs);
                                    *new_ints = (double)*((long*)ddim->ints);
                                    if (ddim->offs != nullptr) {
                                        free(ddim->offs);
                                    }
                                    if (ddim->ints != nullptr) {
                                        free(ddim->ints);
                                    }
                                    ddim->offs = (char*)new_offs;
                                    ddim->ints = (char*)new_ints;
                                    ddim->data_type = UDA_TYPE_DOUBLE;
                                    break;
                            }
                            break;

                        case UDA_TYPE_UNSIGNED_CHAR:
                            switch (ddim->method) {
                                case 1:
                                    new_offs = (double*)malloc(ddim->udoms * sizeof(double));
                                    new_ints = (double*)malloc(ddim->udoms * sizeof(double));
                                    if (new_offs == nullptr || new_ints == nullptr) {
                                        if (new_offs != nullptr) {
                                            free(new_offs);
                                        }
                                        if (new_ints != nullptr) {
                                            free(new_ints);
                                        }
                                        return 1;
                                    }
                                    for (unsigned int i = 0; i < ddim->udoms; i++) {
                                        *(new_offs + i) = (double)*((unsigned char*)ddim->offs + i);
                                        *(new_ints + i) = (double)*((unsigned char*)ddim->ints + i);
                                    }
                                    if (ddim->offs != nullptr) {
                                        free(ddim->offs);
                                    }
                                    if (ddim->ints != nullptr) {
                                        free(ddim->ints);
                                    }
                                    ddim->offs = (char*)new_offs;
                                    ddim->ints = (char*)new_ints;
                                    ddim->data_type = UDA_TYPE_DOUBLE;
                                    break;
                                case 2:
                                    new_offs = (double*)malloc(ddim->udoms * sizeof(double));
                                    for (unsigned int i = 0; i < ddim->udoms; i++) {
                                        *(new_offs + i) = (double)*((unsigned char*)ddim->offs + i);
                                    }
                                    if (ddim->offs != nullptr) {
                                        free(ddim->offs);
                                    }
                                    ddim->offs = (char*)new_offs;
                                    ddim->data_type = UDA_TYPE_DOUBLE;
                                    break;
                                case 3:
                                    new_offs = (double*)malloc(sizeof(double));
                                    new_ints = (double*)malloc(sizeof(double));
                                    *new_offs = (double)*((unsigned char*)ddim->offs);
                                    *new_ints = (double)*((unsigned char*)ddim->ints);
                                    if (ddim->offs != nullptr) {
                                        free(ddim->offs);
                                    }
                                    if (ddim->ints != nullptr) {
                                        free(ddim->ints);
                                    }
                                    ddim->offs = (char*)new_offs;
                                    ddim->ints = (char*)new_ints;
                                    ddim->data_type = UDA_TYPE_DOUBLE;
                                    break;
                            }
                            break;

                        case UDA_TYPE_UNSIGNED_SHORT:
                            switch (ddim->method) {
                                case 1:
                                    new_offs = (double*)malloc(ddim->udoms * sizeof(double));
                                    new_ints = (double*)malloc(ddim->udoms * sizeof(double));
                                    if (new_offs == nullptr || new_ints == nullptr) {
                                        if (new_offs != nullptr) {
                                            free(new_offs);
                                        }
                                        if (new_ints != nullptr) {
                                            free(new_ints);
                                        }
                                        return 1;
                                    }
                                    for (unsigned int i = 0; i < ddim->udoms; i++) {
                                        *(new_offs + i) = (double)*((unsigned short*)ddim->offs + i);
                                        *(new_ints + i) = (double)*((unsigned short*)ddim->ints + i);
                                    }
                                    if (ddim->offs != nullptr) {
                                        free(ddim->offs);
                                    }
                                    if (ddim->ints != nullptr) {
                                        free(ddim->ints);
                                    }
                                    ddim->offs = (char*)new_offs;
                                    ddim->ints = (char*)new_ints;
                                    ddim->data_type = UDA_TYPE_DOUBLE;
                                    break;
                                case 2:
                                    new_offs = (double*)malloc(ddim->udoms * sizeof(double));
                                    for (unsigned int i = 0; i < ddim->udoms; i++) {
                                        *(new_offs + i) = (double)*((unsigned short*)ddim->offs + i);
                                    }
                                    if (ddim->offs != nullptr) {
                                        free(ddim->offs);
                                    }
                                    ddim->offs = (char*)new_offs;
                                    ddim->data_type = UDA_TYPE_DOUBLE;
                                    break;
                                case 3:
                                    new_offs = (double*)malloc(sizeof(double));
                                    new_ints = (double*)malloc(sizeof(double));
                                    *new_offs = (double)*((unsigned short*)ddim->offs);
                                    *new_ints = (double)*((unsigned short*)ddim->ints);
                                    if (ddim->offs != nullptr) {
                                        free(ddim->offs);
                                    }
                                    if (ddim->ints != nullptr) {
                                        free(ddim->ints);
                                    }
                                    ddim->offs = (char*)new_offs;
                                    ddim->ints = (char*)new_ints;
                                    ddim->data_type = UDA_TYPE_DOUBLE;
                                    break;
                            }
                            break;

                        case UDA_TYPE_UNSIGNED_INT:
                            switch (ddim->method) {
                                case 1:
                                    new_offs = (double*)malloc(ddim->udoms * sizeof(double));
                                    new_ints = (double*)malloc(ddim->udoms * sizeof(double));
                                    if (new_offs == nullptr || new_ints == nullptr) {
                                        if (new_offs != nullptr) {
                                            free(new_offs);
                                        }
                                        if (new_ints != nullptr) {
                                            free(new_ints);
                                        }
                                        return 1;
                                    }
                                    for (unsigned int i = 0; i < ddim->udoms; i++) {
                                        *(new_offs + i) = (double)*((unsigned int*)ddim->offs + i);
                                        *(new_ints + i) = (double)*((unsigned int*)ddim->ints + i);
                                    }
                                    if (ddim->offs != nullptr) {
                                        free(ddim->offs);
                                    }
                                    if (ddim->ints != nullptr) {
                                        free(ddim->ints);
                                    }
                                    ddim->offs = (char*)new_offs;
                                    ddim->ints = (char*)new_ints;
                                    ddim->data_type = UDA_TYPE_DOUBLE;
                                    break;
                                case 2:
                                    new_offs = (double*)malloc(ddim->udoms * sizeof(double));
                                    for (unsigned int i = 0; i < ddim->udoms; i++) {
                                        *(new_offs + i) = (double)*((unsigned int*)ddim->offs + i);
                                    }
                                    if (ddim->offs != nullptr) {
                                        free(ddim->offs);
                                    }
                                    ddim->offs = (char*)new_offs;
                                    ddim->data_type = UDA_TYPE_DOUBLE;
                                    break;
                                case 3:
                                    new_offs = (double*)malloc(sizeof(double));
                                    new_ints = (double*)malloc(sizeof(double));
                                    *new_offs = (double)*((unsigned int*)ddim->offs);
                                    *new_ints = (double)*((unsigned int*)ddim->ints);
                                    if (ddim->offs != nullptr) {
                                        free(ddim->offs);
                                    }
                                    if (ddim->ints != nullptr) {
                                        free(ddim->ints);
                                    }
                                    ddim->offs = (char*)new_offs;
                                    ddim->ints = (char*)new_ints;
                                    ddim->data_type = UDA_TYPE_DOUBLE;
                                    break;
                            }
                            break;

                        case UDA_TYPE_UNSIGNED_LONG:
                            switch (ddim->method) {
                                case 1:
                                    new_offs = (double*)malloc(ddim->udoms * sizeof(double));
                                    new_ints = (double*)malloc(ddim->udoms * sizeof(double));
                                    if (new_offs == nullptr || new_ints == nullptr) {
                                        if (new_offs != nullptr) {
                                            free(new_offs);
                                        }
                                        if (new_ints != nullptr) {
                                            free(new_ints);
                                        }
                                        return 1;
                                    }
                                    for (unsigned int i = 0; i < ddim->udoms; i++) {
                                        *(new_offs + i) = (double)*((unsigned long*)ddim->offs + i);
                                        *(new_ints + i) = (double)*((unsigned long*)ddim->ints + i);
                                    }
                                    if (ddim->offs != nullptr) {
                                        free(ddim->offs);
                                    }
                                    if (ddim->ints != nullptr) {
                                        free(ddim->ints);
                                    }
                                    ddim->offs = (char*)new_offs;
                                    ddim->ints = (char*)new_ints;
                                    ddim->data_type = UDA_TYPE_DOUBLE;
                                    break;
                                case 2:
                                    new_offs = (double*)malloc(ddim->udoms * sizeof(double));
                                    for (unsigned int i = 0; i < ddim->udoms; i++) {
                                        *(new_offs + i) = (double)*((unsigned long*)ddim->offs + i);
                                    }
                                    if (ddim->offs != nullptr) {
                                        free(ddim->offs);
                                    }
                                    ddim->offs = (char*)new_offs;
                                    ddim->data_type = UDA_TYPE_DOUBLE;
                                    break;
                                case 3:
                                    new_offs = (double*)malloc(sizeof(double));
                                    new_ints = (double*)malloc(sizeof(double));
                                    *new_offs = (double)*((unsigned long*)ddim->offs);
                                    *new_ints = (double)*((unsigned long*)ddim->ints);
                                    if (ddim->offs != nullptr) {
                                        free(ddim->offs);
                                    }
                                    if (ddim->ints != nullptr) {
                                        free(ddim->ints);
                                    }
                                    ddim->offs = (char*)new_offs;
                                    ddim->ints = (char*)new_ints;
                                    ddim->data_type = UDA_TYPE_DOUBLE;
                                    break;
                            }
                            break;

                        case UDA_TYPE_FLOAT:
                            switch (ddim->method) {
                                case 1:
                                    new_offs = (double*)malloc(ddim->udoms * sizeof(double));
                                    new_ints = (double*)malloc(ddim->udoms * sizeof(double));
                                    if (new_offs == nullptr || new_ints == nullptr) {
                                        if (new_offs != nullptr) {
                                            free(new_offs);
                                        }
                                        if (new_ints != nullptr) {
                                            free(new_ints);
                                        }
                                        return 1;
                                    }

                                    for (unsigned int i = 0; i < ddim->udoms; i++) {
                                        UDA_LOG(UDA_LOG_DEBUG, "{}  {}  {}", i, *((float*)ddim->offs + i),
                                                *((float*)ddim->ints + i));

                                        *(new_offs + i) = (double)*((float*)ddim->offs + i);
                                        *(new_ints + i) = (double)*((float*)ddim->ints + i);

                                        UDA_LOG(UDA_LOG_DEBUG, "{}  {}  {}", i, *(new_offs + i), *(new_ints + i));
                                    }
                                    if (ddim->offs != nullptr) {
                                        free(ddim->offs);
                                    }
                                    if (ddim->ints != nullptr) {
                                        free(ddim->ints);
                                    }
                                    ddim->offs = (char*)new_offs;
                                    ddim->ints = (char*)new_ints;
                                    for (unsigned int i = 0; i < ddim->udoms; i++) {
                                        UDA_LOG(UDA_LOG_DEBUG, "{}  {}  {}", i, *((double*)ddim->offs + i),
                                                *((double*)ddim->ints + i));
                                    }
                                    ddim->data_type = UDA_TYPE_DOUBLE;
                                    break;
                                case 2:
                                    UDA_LOG(UDA_LOG_DEBUG, "Processing Float Method 2");
                                    UDA_LOG(UDA_LOG_DEBUG, "udoms: {}", ddim->udoms);
                                    new_offs = (double*)malloc(ddim->udoms * sizeof(double));
                                    for (unsigned int i = 0; i < ddim->udoms; i++) {
                                        *(new_offs + i) = (double)*((float*)ddim->offs + i);
                                    }
                                    if (ddim->offs != nullptr) {
                                        free(ddim->offs);
                                    }
                                    ddim->offs = (char*)new_offs;
                                    ddim->data_type = UDA_TYPE_DOUBLE;
                                    break;
                                case 3:
                                    UDA_LOG(UDA_LOG_DEBUG, "Processing Float Method 3");
                                    new_offs = (double*)malloc(sizeof(double));
                                    new_ints = (double*)malloc(sizeof(double));
                                    *new_offs = (double)*((float*)ddim->offs);
                                    *new_ints = (double)*((float*)ddim->ints);
                                    UDA_LOG(UDA_LOG_DEBUG, "{}  {}", *((double*)ddim->offs), *((double*)ddim->ints));
                                    if (ddim->offs != nullptr) {
                                        free(ddim->offs);
                                    }
                                    if (ddim->ints != nullptr) {
                                        free(ddim->ints);
                                    }
                                    ddim->offs = (char*)new_offs;
                                    ddim->ints = (char*)new_ints;
                                    UDA_LOG(UDA_LOG_DEBUG, "{}  {}", *((double*)ddim->offs), *((double*)ddim->ints));
                                    ddim->data_type = UDA_TYPE_DOUBLE;
                                    break;
                            }
                            break;

                        default:
                            break;
                    }
                }
            }
        }
    }
    return 0;
}
