#include "server_processing.h"

#include <cstdlib>

#include "logging/logging.h"
#include <uda/types.h>

/**
 * UDA Server Side Data Processing
 *
 * @param client_block
 * @param data_block
 * @return 1 if an error occurred, otherwise 0
 */
int uda::serverProcessing(ClientBlock client_block, DataBlock* data_block)
{
    DIMS* ddim = nullptr;
    double *newoffs = nullptr, *newints = nullptr;

    int reduce = 0;
    short ss;
    int si;
    unsigned su;
    long sl;
    float sf;
    double sd;

    //--------------------------------------------------------------------------------------------------
    // If the Rank is 1 and the dimensional data are compressed with zero values, then reduce the
    // rank giving a scalar array

    if (client_block.get_scalar && data_block->rank == 1) {
        ddim = data_block->dims;
        if (ddim->compressed) {
            reduce = 1;
            if (ddim->method == 0) {
                if (ddim->dim0 != (double)0.0 || ddim->diff == (double)0.0) {
                    reduce = 0;
                }
            } else {
                switch (ddim->data_type) {

                    case UDA_TYPE_FLOAT:
                        sf = (float)0.0;
                        switch (ddim->method) {
                            case 1:
                                for (unsigned int i = 0; i < ddim->udoms; i++) {
                                    for (int j = 0; j < *((long*)ddim->sams + i); j++) {
                                        sf = sf + *((float*)ddim->offs + i) + (float)j * *((float*)ddim->ints + i);
                                        if (sf != (float)0.0) {
                                            reduce = 0;
                                            break;
                                        }
                                    }
                                }
                                break;
                            case 2:
                                for (unsigned int i = 0; i < ddim->udoms; i++) {
                                    sf = sf + *((float*)ddim->offs + i);
                                    if (sf != (float)0.0) {
                                        reduce = 0;
                                        break;
                                    }
                                }
                                break;
                            case 3:
                                for (unsigned int i = 0; i < ddim->udoms; i++) {
                                    sf = sf + *((float*)ddim->offs) + (float)i * *((float*)ddim->ints);
                                    if (sf != (float)0.0) {
                                        reduce = 0;
                                        break;
                                    }
                                }
                                break;
                        }
                        break;

                    case UDA_TYPE_DOUBLE:
                        sd = (double)0.0;
                        switch (ddim->method) {
                            case 1:
                                for (unsigned int i = 0; i < ddim->udoms; i++) {
                                    for (int j = 0; j < *((long*)ddim->sams + i); j++) {
                                        sd = sd + *((double*)ddim->offs + i) + (double)j * *((double*)ddim->ints + i);
                                        if (sd != (double)0.0) {
                                            reduce = 0;
                                            break;
                                        }
                                    }
                                }
                                break;
                            case 2:
                                for (unsigned int i = 0; i < ddim->udoms; i++) {
                                    sd = sd + *((double*)ddim->offs + i);
                                    if (sd != (double)0.0) {
                                        reduce = 0;
                                        break;
                                    }
                                }
                                break;
                            case 3:
                                for (unsigned int i = 0; i < ddim->udoms; i++) {
                                    sd = sd + *((double*)ddim->offs) + (double)i * *((double*)ddim->ints);
                                    if (sd != (double)0.0) {
                                        reduce = 0;
                                        break;
                                    }
                                }
                                break;
                        }
                        break;

                    case UDA_TYPE_CHAR: {
                        char sc = (char)0;
                        switch (ddim->method) {
                            case 1:
                                for (unsigned int i = 0; i < ddim->udoms; i++) {
                                    for (int j = 0; j < *((long*)ddim->sams + i); j++) {
                                        sc = sc + *((char*)ddim->offs + i) + (char)j * *((char*)ddim->ints + i);
                                        if (sc != (short)0) {
                                            reduce = 0;
                                            break;
                                        }
                                    }
                                }
                                break;
                            case 2:
                                for (unsigned int i = 0; i < ddim->udoms; i++) {
                                    sc = sc + *((char*)ddim->offs + i);
                                    if (sc != (char)0) {
                                        reduce = 0;
                                        break;
                                    }
                                }
                                break;
                            case 3:
                                for (unsigned int i = 0; i < ddim->udoms; i++) {
                                    sc = sc + *((char*)ddim->offs) + (char)i * *((char*)ddim->ints);
                                    if (sc != (char)0) {
                                        reduce = 0;
                                        break;
                                    }
                                }
                                break;
                        }
                        break;
                    }

                    case UDA_TYPE_SHORT:
                        ss = (short)0;
                        switch (ddim->method) {
                            case 1:
                                for (unsigned int i = 0; i < ddim->udoms; i++) {
                                    for (int j = 0; j < *((long*)ddim->sams + i); j++) {
                                        ss = ss + *((short*)ddim->offs + i) + (short)j * *((short*)ddim->ints + i);
                                        if (ss != (short)0) {
                                            reduce = 0;
                                            break;
                                        }
                                    }
                                }
                                break;
                            case 2:
                                for (unsigned int i = 0; i < ddim->udoms; i++) {
                                    ss = ss + *((short*)ddim->offs + i);
                                    if (ss != (short)0) {
                                        reduce = 0;
                                        break;
                                    }
                                }
                                break;
                            case 3:
                                for (unsigned int i = 0; i < ddim->udoms; i++) {
                                    ss = ss + *((short*)ddim->offs) + (short)i * *((short*)ddim->ints);
                                    if (ss != (short)0) {
                                        reduce = 0;
                                        break;
                                    }
                                }
                                break;
                        }
                        break;

                    case UDA_TYPE_INT:
                        si = (int)0;
                        switch (ddim->method) {
                            case 1:
                                for (unsigned int i = 0; i < ddim->udoms; i++) {
                                    for (int j = 0; j < *((long*)ddim->sams + i); j++) {
                                        si = si + *((int*)ddim->offs + i) + (int)j * *((int*)ddim->ints + i);
                                        if (si != (int)0) {
                                            reduce = 0;
                                            break;
                                        }
                                    }
                                }
                                break;
                            case 2:
                                for (unsigned int i = 0; i < ddim->udoms; i++) {
                                    si = si + *((int*)ddim->offs + i);
                                    if (si != (int)0) {
                                        reduce = 0;
                                        break;
                                    }
                                }
                                break;
                            case 3:
                                for (unsigned int i = 0; i < ddim->udoms; i++) {
                                    si = si + *((int*)ddim->offs) + (int)i * *((int*)ddim->ints);
                                    if (si != (int)0) {
                                        reduce = 0;
                                        break;
                                    }
                                }
                                break;
                        }
                        break;

                    case UDA_TYPE_LONG:
                        sl = (long)0;
                        switch (ddim->method) {
                            case 1:
                                for (unsigned int i = 0; i < ddim->udoms; i++) {
                                    for (int j = 0; j < *((long*)ddim->sams + i); j++) {
                                        sl = sl + *((long*)ddim->offs + i) + (long)j * *((long*)ddim->ints + i);
                                        if (sl != (long)0) {
                                            reduce = 0;
                                            break;
                                        }
                                    }
                                }
                                break;
                            case 2:
                                for (unsigned int i = 0; i < ddim->udoms; i++) {
                                    sl = sl + *((long*)ddim->offs + i);
                                    if (sl != (long)0) {
                                        reduce = 0;
                                        break;
                                    }
                                }
                                break;
                            case 3:
                                for (unsigned int i = 0; i < ddim->udoms; i++) {
                                    sl = sl + *((long*)ddim->offs) + (long)i * *((long*)ddim->ints);
                                    if (sl != (long)0) {
                                        reduce = 0;
                                        break;
                                    }
                                }
                                break;
                        }
                        break;

                    case UDA_TYPE_UNSIGNED_CHAR: {
                        unsigned char sc = (unsigned char)0;
                        switch (ddim->method) {
                            case 1:
                                for (unsigned int i = 0; i < ddim->udoms; i++) {
                                    for (int j = 0; j < *((long*)ddim->sams + i); j++) {
                                        sc = sc + *((unsigned char*)ddim->offs + i) +
                                             (unsigned char)j * *((unsigned char*)ddim->ints + i);
                                        if (sc != (short)0) {
                                            reduce = 0;
                                            break;
                                        }
                                    }
                                }
                                break;
                            case 2:
                                for (unsigned int i = 0; i < ddim->udoms; i++) {
                                    sc = sc + *((unsigned char*)ddim->offs + i);
                                    if (sc != (unsigned char)0) {
                                        reduce = 0;
                                        break;
                                    }
                                }
                                break;
                            case 3:
                                for (unsigned int i = 0; i < ddim->udoms; i++) {
                                    sc = sc + *((unsigned char*)ddim->offs) +
                                         (unsigned char)i * *((unsigned char*)ddim->ints);
                                    if (sc != (unsigned char)0) {
                                        reduce = 0;
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
                                            reduce = 0;
                                            break;
                                        }
                                    }
                                }
                                break;
                            case 2:
                                for (unsigned int i = 0; i < ddim->udoms; i++) {
                                    ss = ss + *((unsigned short*)ddim->offs + i);
                                    if (ss != (unsigned short)0) {
                                        reduce = 0;
                                        break;
                                    }
                                }
                                break;
                            case 3:
                                for (unsigned int i = 0; i < ddim->udoms; i++) {
                                    ss = ss + *((unsigned short*)ddim->offs) +
                                         (unsigned short)i * *((unsigned short*)ddim->ints);
                                    if (ss != (unsigned short)0) {
                                        reduce = 0;
                                        break;
                                    }
                                }
                                break;
                        }
                        break;
                    }

                    case UDA_TYPE_UNSIGNED_INT:
                        su = (unsigned int)0;
                        switch (ddim->method) {
                            case 1:
                                for (unsigned int i = 0; i < ddim->udoms; i++) {
                                    for (int j = 0; j < *((long*)ddim->sams + i); j++) {
                                        su = su + *((unsigned int*)ddim->offs + i) +
                                             (unsigned int)j * *((unsigned int*)ddim->ints + i);
                                        if (su != (unsigned int)0) {
                                            reduce = 0;
                                            break;
                                        }
                                    }
                                }
                                break;
                            case 2:
                                for (unsigned int i = 0; i < ddim->udoms; i++) {
                                    su = su + *((unsigned int*)ddim->offs + i);
                                    if (su != (unsigned int)0) {
                                        reduce = 0;
                                        break;
                                    }
                                }
                                break;
                            case 3:
                                for (unsigned int i = 0; i < ddim->udoms; i++) {
                                    su = su + *((unsigned int*)ddim->offs) +
                                         (unsigned int)i * *((unsigned int*)ddim->ints);
                                    if (su != (unsigned int)0) {
                                        reduce = 0;
                                        break;
                                    }
                                }
                                break;
                        }
                        break;

                    case UDA_TYPE_UNSIGNED_LONG: {
                        unsigned long sl = (unsigned long)0;
                        switch (ddim->method) {
                            case 1:
                                for (unsigned int i = 0; i < ddim->udoms; i++) {
                                    for (int j = 0; j < *((long*)ddim->sams + i); j++) {
                                        sl = sl + *((unsigned long*)ddim->offs + i) +
                                             (unsigned long)j * *((unsigned long*)ddim->ints + i);
                                        if (sl != (unsigned long)0) {
                                            reduce = 0;
                                            break;
                                        }
                                    }
                                }
                                break;
                            case 2:
                                for (unsigned int i = 0; i < ddim->udoms; i++) {
                                    sl = sl + *((unsigned long*)ddim->offs + i);
                                    if (sl != (unsigned long)0) {
                                        reduce = 0;
                                        break;
                                    }
                                }
                                break;
                            case 3:
                                for (unsigned int i = 0; i < ddim->udoms; i++) {
                                    sl = sl + *((unsigned long*)ddim->offs) +
                                         (unsigned long)i * *((unsigned long*)ddim->ints);
                                    if (sl != (unsigned long)0) {
                                        reduce = 0;
                                        break;
                                    }
                                }
                                break;
                        }
                        break;
                    }

                    default:
                        reduce = 0;
                        break;
                }
            }
            if (reduce) { // Reduce the Rank to Scalar and free Dimensional Heap Memory
                data_block->order = -1;
                data_block->rank = 0;
                if (ddim->dim != nullptr) {
                    free(ddim->dim);
                }
                if (ddim->errhi != nullptr) {
                    free(ddim->errhi);
                }
                if (ddim->errlo != nullptr) {
                    free(ddim->errlo);
                }
                if (ddim->sams != nullptr) {
                    free(ddim->sams);
                }
                if (ddim->offs != nullptr) {
                    free(ddim->offs);
                }
                if (ddim->ints != nullptr) {
                    free(ddim->ints);
                }
                free(ddim);
                data_block->dims = nullptr;
            }
        }
    }

    //--------------------------------------------------------------------------------------------------
    // Cast the Time Dimension to Double Precision if the data are in a compressed format
    // or ALL Dimensions if the data are in compressed formats

    UDA_LOG(UDA_LOG_DEBUG, "Server Side Processing\n");

    if (client_block.get_timedble || client_block.get_dimdble) {
        for (unsigned int k = 0; k < data_block->rank; k++) {
            if (client_block.get_timedble && k != (unsigned int)data_block->order) {
                continue; // Only Process the Time Dimension
            }
            UDA_LOG(UDA_LOG_DEBUG, "Processing Dimension %d\n", k);
            ddim = data_block->dims + k;
            if (ddim->compressed) {
                if (ddim->method == 0) {
                    ddim->data_type = UDA_TYPE_DOUBLE;
                } else {
                    switch (ddim->data_type) {
                        case UDA_TYPE_CHAR:
                            switch (ddim->method) {
                                case 1:
                                    newoffs = (double*)malloc(ddim->udoms * sizeof(double));
                                    newints = (double*)malloc(ddim->udoms * sizeof(double));
                                    if (newoffs == nullptr || newints == nullptr) {
                                        if (newoffs != nullptr) {
                                            free(newoffs);
                                        }
                                        if (newints != nullptr) {
                                            free(newints);
                                        }
                                        return 1;
                                    }
                                    for (unsigned int i = 0; i < ddim->udoms; i++) {
                                        *(newoffs + i) = (double)*((char*)ddim->offs + i);
                                        *(newints + i) = (double)*((char*)ddim->ints + i);
                                    }
                                    if (ddim->offs != nullptr) {
                                        free(ddim->offs);
                                    }
                                    if (ddim->ints != nullptr) {
                                        free(ddim->ints);
                                    }
                                    ddim->offs = (char*)newoffs;
                                    ddim->ints = (char*)newints;
                                    ddim->data_type = UDA_TYPE_DOUBLE;
                                    break;
                                case 2:
                                    newoffs = (double*)malloc(ddim->udoms * sizeof(double));
                                    for (unsigned int i = 0; i < ddim->udoms; i++) {
                                        *(newoffs + i) = (double)*((char*)ddim->offs + i);
                                    }
                                    if (ddim->offs != nullptr) {
                                        free(ddim->offs);
                                    }
                                    ddim->offs = (char*)newoffs;
                                    ddim->data_type = UDA_TYPE_DOUBLE;
                                    break;
                                case 3:
                                    newoffs = (double*)malloc(sizeof(double));
                                    newints = (double*)malloc(sizeof(double));
                                    *newoffs = (double)*((char*)ddim->offs);
                                    *newints = (double)*((char*)ddim->ints);
                                    if (ddim->offs != nullptr) {
                                        free(ddim->offs);
                                    }
                                    if (ddim->ints != nullptr) {
                                        free(ddim->ints);
                                    }
                                    ddim->offs = (char*)newoffs;
                                    ddim->ints = (char*)newints;
                                    ddim->data_type = UDA_TYPE_DOUBLE;
                                    break;
                            }
                            break;

                        case UDA_TYPE_SHORT:
                            switch (ddim->method) {
                                case 1:
                                    newoffs = (double*)malloc(ddim->udoms * sizeof(double));
                                    newints = (double*)malloc(ddim->udoms * sizeof(double));
                                    if (newoffs == nullptr || newints == nullptr) {
                                        if (newoffs != nullptr) {
                                            free(newoffs);
                                        }
                                        if (newints != nullptr) {
                                            free(newints);
                                        }
                                        return 1;
                                    }
                                    for (unsigned int i = 0; i < ddim->udoms; i++) {
                                        *(newoffs + i) = (double)*((short*)ddim->offs + i);
                                        *(newints + i) = (double)*((short*)ddim->ints + i);
                                    }
                                    if (ddim->offs != nullptr) {
                                        free(ddim->offs);
                                    }
                                    if (ddim->ints != nullptr) {
                                        free(ddim->ints);
                                    }
                                    ddim->offs = (char*)newoffs;
                                    ddim->ints = (char*)newints;
                                    ddim->data_type = UDA_TYPE_DOUBLE;
                                    break;
                                case 2:
                                    newoffs = (double*)malloc(ddim->udoms * sizeof(double));
                                    for (unsigned int i = 0; i < ddim->udoms; i++) {
                                        *(newoffs + i) = (double)*((short*)ddim->offs + i);
                                    }
                                    if (ddim->offs != nullptr) {
                                        free(ddim->offs);
                                    }
                                    ddim->offs = (char*)newoffs;
                                    ddim->data_type = UDA_TYPE_DOUBLE;
                                    break;
                                case 3:
                                    newoffs = (double*)malloc(sizeof(double));
                                    newints = (double*)malloc(sizeof(double));
                                    *newoffs = (double)*((short*)ddim->offs);
                                    *newints = (double)*((short*)ddim->ints);
                                    if (ddim->offs != nullptr) {
                                        free(ddim->offs);
                                    }
                                    if (ddim->ints != nullptr) {
                                        free(ddim->ints);
                                    }
                                    ddim->offs = (char*)newoffs;
                                    ddim->ints = (char*)newints;
                                    ddim->data_type = UDA_TYPE_DOUBLE;
                                    break;
                            }
                            break;

                        case UDA_TYPE_INT:
                            switch (ddim->method) {
                                case 1:
                                    newoffs = (double*)malloc(ddim->udoms * sizeof(double));
                                    newints = (double*)malloc(ddim->udoms * sizeof(double));
                                    if (newoffs == nullptr || newints == nullptr) {
                                        if (newoffs != nullptr) {
                                            free(newoffs);
                                        }
                                        if (newints != nullptr) {
                                            free(newints);
                                        }
                                        return 1;
                                    }
                                    for (unsigned int i = 0; i < ddim->udoms; i++) {
                                        *(newoffs + i) = (double)*((int*)ddim->offs + i);
                                        *(newints + i) = (double)*((int*)ddim->ints + i);
                                    }
                                    if (ddim->offs != nullptr) {
                                        free(ddim->offs);
                                    }
                                    if (ddim->ints != nullptr) {
                                        free(ddim->ints);
                                    }
                                    ddim->offs = (char*)newoffs;
                                    ddim->ints = (char*)newints;
                                    ddim->data_type = UDA_TYPE_DOUBLE;
                                    break;
                                case 2:
                                    newoffs = (double*)malloc(ddim->udoms * sizeof(double));
                                    for (unsigned int i = 0; i < ddim->udoms; i++) {
                                        *(newoffs + i) = (double)*((int*)ddim->offs + i);
                                    }
                                    if (ddim->offs != nullptr) {
                                        free(ddim->offs);
                                    }
                                    ddim->offs = (char*)newoffs;
                                    ddim->data_type = UDA_TYPE_DOUBLE;
                                    break;
                                case 3:
                                    newoffs = (double*)malloc(sizeof(double));
                                    newints = (double*)malloc(sizeof(double));
                                    *newoffs = (double)*((int*)ddim->offs);
                                    *newints = (double)*((int*)ddim->ints);
                                    if (ddim->offs != nullptr) {
                                        free(ddim->offs);
                                    }
                                    if (ddim->ints != nullptr) {
                                        free(ddim->ints);
                                    }
                                    ddim->offs = (char*)newoffs;
                                    ddim->ints = (char*)newints;
                                    ddim->data_type = UDA_TYPE_DOUBLE;
                                    break;
                            }
                            break;

                        case UDA_TYPE_LONG:
                            switch (ddim->method) {
                                case 1:
                                    newoffs = (double*)malloc(ddim->udoms * sizeof(double));
                                    newints = (double*)malloc(ddim->udoms * sizeof(double));
                                    if (newoffs == nullptr || newints == nullptr) {
                                        if (newoffs != nullptr) {
                                            free(newoffs);
                                        }
                                        if (newints != nullptr) {
                                            free(newints);
                                        }
                                        return 1;
                                    }
                                    for (unsigned int i = 0; i < ddim->udoms; i++) {
                                        *(newoffs + i) = (double)*((long*)ddim->offs + i);
                                        *(newints + i) = (double)*((long*)ddim->ints + i);
                                    }
                                    if (ddim->offs != nullptr) {
                                        free(ddim->offs);
                                    }
                                    if (ddim->ints != nullptr) {
                                        free(ddim->ints);
                                    }
                                    ddim->offs = (char*)newoffs;
                                    ddim->ints = (char*)newints;
                                    ddim->data_type = UDA_TYPE_DOUBLE;
                                    break;
                                case 2:
                                    newoffs = (double*)malloc(ddim->udoms * sizeof(double));
                                    for (unsigned int i = 0; i < ddim->udoms; i++) {
                                        *(newoffs + i) = (double)*((long*)ddim->offs + i);
                                    }
                                    if (ddim->offs != nullptr) {
                                        free(ddim->offs);
                                    }
                                    ddim->offs = (char*)newoffs;
                                    ddim->data_type = UDA_TYPE_DOUBLE;
                                    break;
                                case 3:
                                    newoffs = (double*)malloc(sizeof(double));
                                    newints = (double*)malloc(sizeof(double));
                                    *newoffs = (double)*((long*)ddim->offs);
                                    *newints = (double)*((long*)ddim->ints);
                                    if (ddim->offs != nullptr) {
                                        free(ddim->offs);
                                    }
                                    if (ddim->ints != nullptr) {
                                        free(ddim->ints);
                                    }
                                    ddim->offs = (char*)newoffs;
                                    ddim->ints = (char*)newints;
                                    ddim->data_type = UDA_TYPE_DOUBLE;
                                    break;
                            }
                            break;

                        case UDA_TYPE_UNSIGNED_CHAR:
                            switch (ddim->method) {
                                case 1:
                                    newoffs = (double*)malloc(ddim->udoms * sizeof(double));
                                    newints = (double*)malloc(ddim->udoms * sizeof(double));
                                    if (newoffs == nullptr || newints == nullptr) {
                                        if (newoffs != nullptr) {
                                            free(newoffs);
                                        }
                                        if (newints != nullptr) {
                                            free(newints);
                                        }
                                        return 1;
                                    }
                                    for (unsigned int i = 0; i < ddim->udoms; i++) {
                                        *(newoffs + i) = (double)*((unsigned char*)ddim->offs + i);
                                        *(newints + i) = (double)*((unsigned char*)ddim->ints + i);
                                    }
                                    if (ddim->offs != nullptr) {
                                        free(ddim->offs);
                                    }
                                    if (ddim->ints != nullptr) {
                                        free(ddim->ints);
                                    }
                                    ddim->offs = (char*)newoffs;
                                    ddim->ints = (char*)newints;
                                    ddim->data_type = UDA_TYPE_DOUBLE;
                                    break;
                                case 2:
                                    newoffs = (double*)malloc(ddim->udoms * sizeof(double));
                                    for (unsigned int i = 0; i < ddim->udoms; i++) {
                                        *(newoffs + i) = (double)*((unsigned char*)ddim->offs + i);
                                    }
                                    if (ddim->offs != nullptr) {
                                        free(ddim->offs);
                                    }
                                    ddim->offs = (char*)newoffs;
                                    ddim->data_type = UDA_TYPE_DOUBLE;
                                    break;
                                case 3:
                                    newoffs = (double*)malloc(sizeof(double));
                                    newints = (double*)malloc(sizeof(double));
                                    *newoffs = (double)*((unsigned char*)ddim->offs);
                                    *newints = (double)*((unsigned char*)ddim->ints);
                                    if (ddim->offs != nullptr) {
                                        free(ddim->offs);
                                    }
                                    if (ddim->ints != nullptr) {
                                        free(ddim->ints);
                                    }
                                    ddim->offs = (char*)newoffs;
                                    ddim->ints = (char*)newints;
                                    ddim->data_type = UDA_TYPE_DOUBLE;
                                    break;
                            }
                            break;

                        case UDA_TYPE_UNSIGNED_SHORT:
                            switch (ddim->method) {
                                case 1:
                                    newoffs = (double*)malloc(ddim->udoms * sizeof(double));
                                    newints = (double*)malloc(ddim->udoms * sizeof(double));
                                    if (newoffs == nullptr || newints == nullptr) {
                                        if (newoffs != nullptr) {
                                            free(newoffs);
                                        }
                                        if (newints != nullptr) {
                                            free(newints);
                                        }
                                        return 1;
                                    }
                                    for (unsigned int i = 0; i < ddim->udoms; i++) {
                                        *(newoffs + i) = (double)*((unsigned short*)ddim->offs + i);
                                        *(newints + i) = (double)*((unsigned short*)ddim->ints + i);
                                    }
                                    if (ddim->offs != nullptr) {
                                        free(ddim->offs);
                                    }
                                    if (ddim->ints != nullptr) {
                                        free(ddim->ints);
                                    }
                                    ddim->offs = (char*)newoffs;
                                    ddim->ints = (char*)newints;
                                    ddim->data_type = UDA_TYPE_DOUBLE;
                                    break;
                                case 2:
                                    newoffs = (double*)malloc(ddim->udoms * sizeof(double));
                                    for (unsigned int i = 0; i < ddim->udoms; i++) {
                                        *(newoffs + i) = (double)*((unsigned short*)ddim->offs + i);
                                    }
                                    if (ddim->offs != nullptr) {
                                        free(ddim->offs);
                                    }
                                    ddim->offs = (char*)newoffs;
                                    ddim->data_type = UDA_TYPE_DOUBLE;
                                    break;
                                case 3:
                                    newoffs = (double*)malloc(sizeof(double));
                                    newints = (double*)malloc(sizeof(double));
                                    *newoffs = (double)*((unsigned short*)ddim->offs);
                                    *newints = (double)*((unsigned short*)ddim->ints);
                                    if (ddim->offs != nullptr) {
                                        free(ddim->offs);
                                    }
                                    if (ddim->ints != nullptr) {
                                        free(ddim->ints);
                                    }
                                    ddim->offs = (char*)newoffs;
                                    ddim->ints = (char*)newints;
                                    ddim->data_type = UDA_TYPE_DOUBLE;
                                    break;
                            }
                            break;

                        case UDA_TYPE_UNSIGNED_INT:
                            switch (ddim->method) {
                                case 1:
                                    newoffs = (double*)malloc(ddim->udoms * sizeof(double));
                                    newints = (double*)malloc(ddim->udoms * sizeof(double));
                                    if (newoffs == nullptr || newints == nullptr) {
                                        if (newoffs != nullptr) {
                                            free(newoffs);
                                        }
                                        if (newints != nullptr) {
                                            free(newints);
                                        }
                                        return 1;
                                    }
                                    for (unsigned int i = 0; i < ddim->udoms; i++) {
                                        *(newoffs + i) = (double)*((unsigned int*)ddim->offs + i);
                                        *(newints + i) = (double)*((unsigned int*)ddim->ints + i);
                                    }
                                    if (ddim->offs != nullptr) {
                                        free(ddim->offs);
                                    }
                                    if (ddim->ints != nullptr) {
                                        free(ddim->ints);
                                    }
                                    ddim->offs = (char*)newoffs;
                                    ddim->ints = (char*)newints;
                                    ddim->data_type = UDA_TYPE_DOUBLE;
                                    break;
                                case 2:
                                    newoffs = (double*)malloc(ddim->udoms * sizeof(double));
                                    for (unsigned int i = 0; i < ddim->udoms; i++) {
                                        *(newoffs + i) = (double)*((unsigned int*)ddim->offs + i);
                                    }
                                    if (ddim->offs != nullptr) {
                                        free(ddim->offs);
                                    }
                                    ddim->offs = (char*)newoffs;
                                    ddim->data_type = UDA_TYPE_DOUBLE;
                                    break;
                                case 3:
                                    newoffs = (double*)malloc(sizeof(double));
                                    newints = (double*)malloc(sizeof(double));
                                    *newoffs = (double)*((unsigned int*)ddim->offs);
                                    *newints = (double)*((unsigned int*)ddim->ints);
                                    if (ddim->offs != nullptr) {
                                        free(ddim->offs);
                                    }
                                    if (ddim->ints != nullptr) {
                                        free(ddim->ints);
                                    }
                                    ddim->offs = (char*)newoffs;
                                    ddim->ints = (char*)newints;
                                    ddim->data_type = UDA_TYPE_DOUBLE;
                                    break;
                            }
                            break;

                        case UDA_TYPE_UNSIGNED_LONG:
                            switch (ddim->method) {
                                case 1:
                                    newoffs = (double*)malloc(ddim->udoms * sizeof(double));
                                    newints = (double*)malloc(ddim->udoms * sizeof(double));
                                    if (newoffs == nullptr || newints == nullptr) {
                                        if (newoffs != nullptr) {
                                            free(newoffs);
                                        }
                                        if (newints != nullptr) {
                                            free(newints);
                                        }
                                        return 1;
                                    }
                                    for (unsigned int i = 0; i < ddim->udoms; i++) {
                                        *(newoffs + i) = (double)*((unsigned long*)ddim->offs + i);
                                        *(newints + i) = (double)*((unsigned long*)ddim->ints + i);
                                    }
                                    if (ddim->offs != nullptr) {
                                        free(ddim->offs);
                                    }
                                    if (ddim->ints != nullptr) {
                                        free(ddim->ints);
                                    }
                                    ddim->offs = (char*)newoffs;
                                    ddim->ints = (char*)newints;
                                    ddim->data_type = UDA_TYPE_DOUBLE;
                                    break;
                                case 2:
                                    newoffs = (double*)malloc(ddim->udoms * sizeof(double));
                                    for (unsigned int i = 0; i < ddim->udoms; i++) {
                                        *(newoffs + i) = (double)*((unsigned long*)ddim->offs + i);
                                    }
                                    if (ddim->offs != nullptr) {
                                        free(ddim->offs);
                                    }
                                    ddim->offs = (char*)newoffs;
                                    ddim->data_type = UDA_TYPE_DOUBLE;
                                    break;
                                case 3:
                                    newoffs = (double*)malloc(sizeof(double));
                                    newints = (double*)malloc(sizeof(double));
                                    *newoffs = (double)*((unsigned long*)ddim->offs);
                                    *newints = (double)*((unsigned long*)ddim->ints);
                                    if (ddim->offs != nullptr) {
                                        free(ddim->offs);
                                    }
                                    if (ddim->ints != nullptr) {
                                        free(ddim->ints);
                                    }
                                    ddim->offs = (char*)newoffs;
                                    ddim->ints = (char*)newints;
                                    ddim->data_type = UDA_TYPE_DOUBLE;
                                    break;
                            }
                            break;

                        case UDA_TYPE_FLOAT:
                            switch (ddim->method) {
                                case 1:
                                    newoffs = (double*)malloc(ddim->udoms * sizeof(double));
                                    newints = (double*)malloc(ddim->udoms * sizeof(double));
                                    if (newoffs == nullptr || newints == nullptr) {
                                        if (newoffs != nullptr) {
                                            free(newoffs);
                                        }
                                        if (newints != nullptr) {
                                            free(newints);
                                        }
                                        return 1;
                                    }

                                    for (unsigned int i = 0; i < ddim->udoms; i++) {
                                        UDA_LOG(UDA_LOG_DEBUG, "%i  %f  %f\n", i, *((float*)ddim->offs + i),
                                                *((float*)ddim->ints + i));

                                        *(newoffs + i) = (double)*((float*)ddim->offs + i);
                                        *(newints + i) = (double)*((float*)ddim->ints + i);

                                        UDA_LOG(UDA_LOG_DEBUG, "%i  %f  %f\n", i, *(newoffs + i), *(newints + i));
                                    }
                                    if (ddim->offs != nullptr) {
                                        free(ddim->offs);
                                    }
                                    if (ddim->ints != nullptr) {
                                        free(ddim->ints);
                                    }
                                    ddim->offs = (char*)newoffs;
                                    ddim->ints = (char*)newints;
                                    for (unsigned int i = 0; i < ddim->udoms; i++) {
                                        UDA_LOG(UDA_LOG_DEBUG, "%i  %f  %f\n", i, *((double*)ddim->offs + i),
                                                *((double*)ddim->ints + i));
                                    }
                                    ddim->data_type = UDA_TYPE_DOUBLE;
                                    break;
                                case 2:
                                    UDA_LOG(UDA_LOG_DEBUG, "Processing Float Method 2\n");
                                    UDA_LOG(UDA_LOG_DEBUG, "udoms: %d\n", ddim->udoms);
                                    newoffs = (double*)malloc(ddim->udoms * sizeof(double));
                                    for (unsigned int i = 0; i < ddim->udoms; i++) {
                                        *(newoffs + i) = (double)*((float*)ddim->offs + i);
                                    }
                                    if (ddim->offs != nullptr) {
                                        free(ddim->offs);
                                    }
                                    ddim->offs = (char*)newoffs;
                                    ddim->data_type = UDA_TYPE_DOUBLE;
                                    break;
                                case 3:
                                    UDA_LOG(UDA_LOG_DEBUG, "Processing Float Method 3\n");
                                    newoffs = (double*)malloc(sizeof(double));
                                    newints = (double*)malloc(sizeof(double));
                                    *newoffs = (double)*((float*)ddim->offs);
                                    *newints = (double)*((float*)ddim->ints);
                                    UDA_LOG(UDA_LOG_DEBUG, "%f  %f\n", *((double*)ddim->offs), *((double*)ddim->ints));
                                    if (ddim->offs != nullptr) {
                                        free(ddim->offs);
                                    }
                                    if (ddim->ints != nullptr) {
                                        free(ddim->ints);
                                    }
                                    ddim->offs = (char*)newoffs;
                                    ddim->ints = (char*)newints;
                                    UDA_LOG(UDA_LOG_DEBUG, "%f  %f\n", *((double*)ddim->offs), *((double*)ddim->ints));
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
