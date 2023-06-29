#include "compressDim.h"

#include <cfloat>
#include <cstdlib>
#include <cstdint>

#include <clientserver/udaTypes.h>

#include "udaErrors.h"

namespace {

template <typename T>
struct Precision {
    static T precision;
};

template <typename T> T Precision<T>::precision = 0;
template <> float Precision<float>::precision = FLT_EPSILON;
template <> double Precision<double>::precision = DBL_EPSILON;

template <typename T>
int compress(DIMS* ddim)
{
    T* dim_data = (T*)ddim->dim;
    if (dim_data == nullptr) {
        return 0;
    }
    
    int ndata = ddim->dim_n;

    //no need to compress if the data is already compressed or if there are less or equal to 2 elements
    if (ndata <3 || ddim->compressed==1) {
	    return 1;
    }
    T prev_diff = dim_data[1] - dim_data[0];
    T mean_diff = (dim_data[ndata - 1] - dim_data[0]) / (ndata - 1);
    T precision = Precision<T>::precision;

    bool constant = true;
    for (int i = 1; i < ndata; i++) {
        T diff = dim_data[i] - dim_data[i - 1];
        T abs_diff = diff < prev_diff ? (prev_diff - diff) : (diff - prev_diff);
        if (abs_diff > precision) {
            constant = false;
            break;
        }
        prev_diff = diff;
    }

    if (!constant) {
        ddim->compressed = 0;
        return 1;        // Data not regular
    }

    ddim->compressed = 1;
    ddim->dim0 = dim_data[0];
    ddim->diff = mean_diff;     // Average difference
    ddim->method = 0;           // Default Decompression Method

    return 0;
}

template <typename T>
int decompress(DIMS* ddim)
{
    int ndata = ddim->dim_n;

    if (ddim->dim == nullptr) {
        if ((ddim->dim = (char*)malloc(ndata * sizeof(char))) == nullptr) {
            return UNCOMPRESS_ALLOCATING_HEAP;
        }
    }
    T* dim_data = (T*)ddim->dim;

    T d0 = (T)ddim->dim0;        // Default Compression Method
    T diff = (T)ddim->diff;
    int count = 0;

    switch (ddim->method) {
        case 0:
            dim_data[0] = d0;
            for (int i = 1; i < ndata; i++) {
                dim_data[i] = dim_data[i - 1] + diff;
            }
            break;
        case 1:
            for (unsigned int i = 0; i < ddim->udoms; i++) {
                for (int j = 0; j < *(ddim->sams + i); j++) {
                    dim_data[count++] = *((T*)ddim->offs + i) + (T)j * *((T*)ddim->ints + i);
                }
            }
            break;
        case 2:
            for (unsigned int i = 0; i < ddim->udoms; i++) {
                dim_data[i] = *((T*)ddim->offs + i);
            }
            break;
        case 3:
            for (unsigned int i = 0; i < ddim->udoms; i++) {
                dim_data[i] = *((T*)ddim->offs) + (T)i * *((T*)ddim->ints);
            }
            break;
    }

    return 0;
}

} // anon namespace

/**
 * UDA Naive Dimensional Data Compressor
 *
 * @param ddim      Dimensional Data
 * @return          0 if no problems found
 *                  1 if no data compression performed
 *
 * Out:             DIMS* ->dim0            Starting value
 *                  DIMS* ->diff            Value increment
 *                  DIMS* ->method          0 for naive compression
 *                  DIMS* ->compressed      1 if compression performed
 *
 * Notes: If the dimensional data is regular it can be compressed into three numbers: A starting value, a step value
 * and the number of data points. The first two of these are cast as doubles to preserve the highest level of accuracy.
 */
int compressDim(DIMS* ddim)
{
    switch (ddim->data_type) {
        case UDA_TYPE_CHAR:
            return compress<char>(ddim);
        case UDA_TYPE_SHORT:
            return compress<short>(ddim);
        case UDA_TYPE_INT:
            return compress<int>(ddim);
        case UDA_TYPE_LONG:
            return compress<long>(ddim);
//        case UDA_TYPE_LONG64:
//            return compress<int64_t>(ddim);
        case UDA_TYPE_FLOAT:
            return compress<float>(ddim);
        case UDA_TYPE_DOUBLE:
            return compress<double>(ddim);
        case UDA_TYPE_UNSIGNED_CHAR:
            return compress<unsigned char>(ddim);
        case UDA_TYPE_UNSIGNED_SHORT:
            return compress<unsigned short>(ddim);
        case UDA_TYPE_UNSIGNED_INT:
            return compress<unsigned int>(ddim);
        case UDA_TYPE_UNSIGNED_LONG:
            return compress<unsigned long>(ddim);
//        case UDA_TYPE_UNSIGNED_LONG64:
//            return compress<uint64_t>(ddim);
        default:
            ddim->compressed = 0;
            return 1;
    }
}

/**
 * UDA Dimensional Data Decompressor
 *
 * @param ddim      Dimensional Data
 * @return          0 if no Problems Found
 *                  Error Code if a Problem Occurred
 *
 * Out:             DIMS* ->dim             Un-Compressed Dimensional Data
 *                  DIMS* ->compressed      Unchanged (necessary)
 *
 * Note: XML based data correction also uses the compression models: New models
 * must also have corrections applied.
 */
int uncompressDim(DIMS* ddim)
{
    if (!ddim || ddim->compressed == 0) {
        return 0;    // Nothing to Uncompress!
    }
    if (ddim->dim_n == 0) {
        return 0;    // Nothing to Uncompress!
    }

    switch (ddim->data_type) {
        case UDA_TYPE_CHAR:
            return decompress<char>(ddim);
        case UDA_TYPE_SHORT:
            return decompress<short>(ddim);
        case UDA_TYPE_INT:
            return decompress<int>(ddim);
        case UDA_TYPE_LONG:
            return decompress<long>(ddim);
//        case UDA_TYPE_LONG64:
//            return decompress<int64_t>(ddim);
        case UDA_TYPE_FLOAT:
            return decompress<float>(ddim);
        case UDA_TYPE_DOUBLE:
            return decompress<double>(ddim);
        case UDA_TYPE_UNSIGNED_CHAR:
            return decompress<unsigned char>(ddim);
        case UDA_TYPE_UNSIGNED_SHORT:
            return decompress<unsigned short>(ddim);
        case UDA_TYPE_UNSIGNED_INT:
            return decompress<unsigned int>(ddim);
        case UDA_TYPE_UNSIGNED_LONG:
            return decompress<unsigned long>(ddim);
//        case UDA_TYPE_UNSIGNED_LONG64:
//            return decompress<uint64_t>(ddim);
        default:
            return UNKNOWN_DATA_TYPE;
    }
}
