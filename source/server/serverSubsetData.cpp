//--------------------------------------------------------------------------------------------------------------------
// Serverside Data Subsetting Operations Data
//
// Return Codes:	0 => OK, otherwise Error
//
//--------------------------------------------------------------------------------------------------------------------

#include "serverSubsetData.h"
#include "getServerEnvironment.h"
#include "clientserver/parseOperation.h"
#include "plugins/udaPlugin.h"
#include "clientserver/makeRequestBlock.h"

#include <cmath>
#include <cfloat>
#include <cerrno>
#if defined(__GNUC__)
#  include <strings.h>
#else
#  define strncasecmp _strnicmp
#endif

#include <logging/logging.h>
#include <clientserver/printStructs.h>
#include <clientserver/errorLog.h>
#include <clientserver/udaTypes.h>
#include <structures/struct.h>
#include <clientserver/initStructs.h>
#include <clientserver/compressDim.h>
#include <clientserver/stringUtils.h>
#include <boost/algorithm/string.hpp>

//----------------------------------------------------------------------------------------------------------------------------
//----------------------------------------------------------------------------------------------------------------------------
// Manage Array Subsetting
//
// recognised subsetting operations: lt, le, eq, =, ==, ~=, ge, gt
// subsets like != can be made by combining two operations: lt and gt
//
// array reshaping operations look like "1:3,*,*" or "3,*" with the operator position identifying the dimension,
// * means ignore (use all dimension elements), n:m means elements n to m (array element numbering begins at 0)
//
// additional operations are:
//
//	reform - reduce the rank by 1 if the dimension length is 1 and the rank > 1


// todo:
//

static int get_subset_indices(const std::string& operation, DIMS* dim, double value, unsigned int* subset_indices);

int apply_sub_setting(DIMS* dims, int rank, int dim_id,
                      char* data, int ndata, int data_type, int not_operation,
                      int start, int end, int start1, int end1, int stride, int* n, void** new_data);

int number_of_subsetting_operations(const ACTION* action) {
    switch (action->actionType) {
        case UDA_COMPOSITE_TYPE:
            // XML Based sub-setting
            if (action->composite.nsubsets == 0) {
                return 0;        // Nothing to Subset
            }
            return action->composite.nsubsets;
        case UDA_SERVER_SIDE_TYPE:
            // Client Requested sub-setting
            if (action->serverside.nsubsets == 0) {
                return 0;    // Nothing to Subset
            }
            return action->serverside.nsubsets;
        case UDA_SUBSET_TYPE:
            // Client Requested sub-setting
            return 1;
        default:
            return 0;
    }
}

int process_subset_operation(int ii, SUBSET subset, DATA_BLOCK* data_block, LOGMALLOCLIST* logmalloclist)
{
    int n_bound = subset.nbound;                        // the Number of operations in the set

    for (int j = 0; j < n_bound; j++) {                        // Process each operation separately

        double value = subset.bound[j];
        std::string operation = subset.operation[j];                // a single operation
        int dim_id = subset.dimid[j];                    // applied to this dimension (if -1 then to data only!)

        UDA_LOG(UDA_LOG_DEBUG, "[%d][%d]Value = %e, Operation = %s, DIM id = %d, Reform = %d\n", ii, j, value, operation.c_str(), dim_id, subset.reform);

        if (dim_id < 0 || dim_id >= (int)data_block->rank) {
            UDA_LOG(UDA_LOG_ERROR, "DIM id = %d,  Rank = %d, Test = %d \n",
                    dim_id, data_block->rank, dim_id >= (int)data_block->rank);
            printDataBlock(*data_block);
            THROW_ERROR(9999, "Data Subsetting is Impossible as the subset Dimension is not Compatible with the Rank of the Signal");
            return 9999;
        }

        //----------------------------------------------------------------------------------------------------------------------------
        // Operations on Simple Data Structures: target must be an Atomic Type, Scalar, Name is Case Sensitive
        //
        // (mapType=1) Array of Structures - member = single scalar value: rank and shape = rank and shape of structure array
        // (mapType=2) Single Structure - member = array of values: rank and shape = increase rank and shape of structure member by 1
        // Array of Structures - member = array of values: Not allowed.
        //
        // (mapType=3) structure[14].array[100] -> newarray[14][100]:

        if (data_block->opaque_type == UDA_OPAQUE_TYPE_STRUCTURES
                && subset.member[0] != '\0'
                && data_block->opaque_block != nullptr) {


            auto udt = (USERDEFINEDTYPE*)data_block->opaque_block;

            // Extract an atomic type data element from the data structure

            for (int i = 0; i < udt->fieldcount; i++) {
                char* extract = nullptr;
                if (STR_EQUALS(udt->compoundfield[i].name, subset.member)) {        // Locate target member by name

                    int data_n = data_block->data_n;

                    int mapType = 0;
                    int udt_count = udt->compoundfield[i].count;

                    if (udt_count == 1 && data_n >= 1) {
                        mapType = 1;
                    } else if (udt_count >= 1 && data_n == 1) {
                        mapType = 2;
                    } else {
                        mapType = 3;
                    }

                    int* shape = nullptr;

                    if (!udt->compoundfield[i].pointer) {            // Regular Array of data

                        switch (udt->compoundfield[i].atomictype) {
                            case UDA_TYPE_DOUBLE: {
                                double* data = nullptr, * dp;

                                if (mapType == 1) {
                                    data = (double*)malloc(data_n * sizeof(double));
                                    for (int k = 0; k < data_n; k++)
                                        data[k] = *(double*)&data_block->data[k * udt->size +
                                                                              udt->compoundfield[i].offset];
                                } else {
                                    if (mapType == 2) {
                                        data_n = udt_count;
                                        data = (double*)malloc(data_n * sizeof(double));
                                        dp = (double*)&data_block->data[udt->compoundfield[i].offset];
                                        for (int k = 0; k < data_n; k++) {
                                            data[k] = dp[k];
                                        }

                                        // Increase rank and shape by 1: Preserve the original dimensional data
                                        // Replace with index using rank and shape from the member

                                        if ((shape = udt->compoundfield[i].shape) == nullptr &&
                                            udt->compoundfield[i].rank > 1) {
                                            THROW_ERROR(999, "The Data Structure member's shape data is missing (rank > 1)");
                                        }

                                    } else {        // mapType == 3
                                        int total_n;
                                        total_n = udt_count * data_n;
                                        data = (double*)malloc(total_n * sizeof(double));
                                        int jjj = 0;
                                        for (int jj = 0; jj < data_n; jj++) {    // Loop over structures
                                            dp = (double*)&data_block->data[jj * udt->size +
                                                                            udt->compoundfield[i].offset];
                                            for (int k = 0; k < udt_count; k++) {
                                                data[jjj++] = dp[k];
                                            }
                                        }
                                        data_n = total_n;
                                    }
                                }
                                data_block->data_type = UDA_TYPE_DOUBLE;
                                extract = (char*)data;
                                break;
                            }
                        }

                    } else {        // Locate the pointer data's properties:

                        int count = 0;
                        int size = 0;
                        const char* type_name = nullptr;
                        int rank = 0;

                        if (data_n > 1) {
                            int count_p = 0;
                            int rank_p = 0;
                            int size_p = 0;
                            int* shape_p = nullptr;
                            const char* type_name_p = nullptr;

                            for (int jj = 0; jj < data_n; jj++) {
                                // Properties Must be identical for all structure array elements
                                extract = *(char**)&data_block->data[jj * udt->size + udt->compoundfield[i].offset];

                                findMalloc2(logmalloclist, (void*)extract, &count, &size, &type_name, &rank,
                                            &shape);
                                if (jj > 0) {
                                    if (count != count_p || size != size_p || rank != rank_p ||
                                        strcmp(type_name, type_name_p) != 0) {
                                        // ERROR
                                    }
                                    if (shape != nullptr) {
                                        for (int k = 0; k < rank; k++) {
                                            if (shape[k] != shape_p[k]) {
                                                //ERROR
                                            }
                                        }
                                    } else {
                                        if (rank > 1) {
                                            THROW_ERROR(999, "The Data Structure member's shape data is missing (rank > 1)");
                                        }
                                    }
                                }
                                count_p = count;
                                size_p = size;
                                type_name_p = type_name;
                                rank_p = rank;
                                shape_p = shape;
                            }

                        } else {
                            extract = *(char**)&data_block->data[udt->compoundfield[i].offset];
                            findMalloc2(logmalloclist, (void*)extract, &count, &size, &type_name, &rank, &shape);
                        }

                        if (mapType == 3) {
                            THROW_ERROR(999, "Unable to subset an array of Data Structures when the target "
                                             "member is also an array. (Functionality has not been implemented!)");
                        }

                        int type = gettypeof(type_name);

                        switch (type) {
                            case UDA_TYPE_DOUBLE: {
                                double* data = nullptr, * dp;
                                if (mapType == 1) {
                                    data = (double*)malloc(data_n * sizeof(double));
                                    for (int k = 0; k < data_n; k++) {
                                        dp = *(double**)&data_block->data[k * udt->size +
                                                                          udt->compoundfield[i].offset];
                                        data[k] = dp[0];
                                    }
                                } else {
                                    if (mapType == 2) {
                                        data_n = count;
                                        data = (double*)malloc(data_n * sizeof(double));
                                        dp = *(double**)&data_block->data[udt->compoundfield[i].offset];
                                        for (int k = 0; k < data_n; k++) {
                                            data[k] = dp[k];
                                        }
                                    }
                                }
                                data_block->data_type = UDA_TYPE_DOUBLE;
                                extract = (char*)data;
                                break;
                            }

                                //default:
                        }
                    }

                    // mapType == 2
                    // Increase rank by 1: Preserve the original dimensional data
                    // Replace with index coordinate using rank and shape from the member

                    if (mapType == 2) {
                        int rank = 0;

                        unsigned int k0 = 0;
                        if (!udt->compoundfield[i].pointer) {
                            if (data_block->rank == 0) {
                                k0 = 0;
                                rank = udt->compoundfield[i].rank;
                            } else {
                                k0 = 1;
                                rank = udt->compoundfield[i].rank + 1;
                            }
                        } else {
                            if (data_block->rank == 0) {
                                k0 = 0;
                            } else {
                                k0 = 1;
                                rank = rank + 1;
                            }
                        }

                        data_block->rank = rank;

                        data_block->dims = (DIMS*)realloc((void*)data_block->dims, rank * sizeof(DIMS));

                        for (int k = k0; k < rank; k++) {
                            initDimBlock(&data_block->dims[k]);
                            if (shape == nullptr) {
                                data_block->dims[k].dim_n = data_n;
                            } else {
                                data_block->dims[k].dim_n = shape[k - 1];
                            }
                            data_block->dims[k].data_type = UDA_TYPE_UNSIGNED_INT;
                            data_block->dims[k].compressed = 1;
                            data_block->dims[k].method = 0;
                            data_block->dims[k].dim0 = 0.0;
                            data_block->dims[k].diff = 1.0;
                        }
                    }

                    if (mapType == 3) {
                        if (!udt->compoundfield[i].pointer) {
                            unsigned int k0 = data_block->rank;
                            data_block->rank = data_block->rank + udt->compoundfield[i].rank;

                            data_block->dims = (DIMS*)realloc((void*)data_block->dims,
                                                              data_block->rank * sizeof(DIMS));

                            for (unsigned int k = k0; k < data_block->rank; k++) {
                                initDimBlock(&data_block->dims[k]);
                                data_block->dims[k].dim_n = udt->compoundfield[i].shape[k - k0];
                                data_block->dims[k].data_type = UDA_TYPE_UNSIGNED_INT;
                                data_block->dims[k].compressed = 1;
                                data_block->dims[k].method = 0;
                                data_block->dims[k].dim0 = 0.0;
                                data_block->dims[k].diff = 1.0;
                            }
                        }
                    }

                    if (logmalloclist != nullptr) {
                        freeMallocLogList(logmalloclist);
                        free(logmalloclist);
                        logmalloclist = nullptr;
                    }

                    // Update the Data Block structure

                    data_block->data = extract;
                    data_block->data_n = data_n;

                    data_block->opaque_type = UDA_OPAQUE_TYPE_UNKNOWN;
                    data_block->opaque_count = 0;
                    data_block->opaque_block = nullptr;
                    break;

                }
            }
        }

        //----------------------------------------------------------------------------------------------------------------------------
        // Subset this Dimension ?

        if (operation == "*") {
            continue;
        }                // This means No subset - Part of an array Reshape Operation

        if (operation[0] == ':'
                && (subset.lbindex[j].init ? subset.lbindex[j].value : 0) == 0
                && (subset.ubindex[j].init ? subset.lbindex[j].value : data_block->dims[dim_id].dim_n) == data_block->dims[dim_id].dim_n
                && (subset.stride[j].init ? subset.stride[j].value : 1) == 1) {
            continue;    // subset spans the complete dimension
        }

        //----------------------------------------------------------------------------------------------------------------------------
        // Decompress the dimensional data if necessary & free Heap Associated with Compression

        DIMS new_dim;
        initDimBlock(&new_dim);                    // Holder for the Sub-setted Dimension (part copy of the original)

        auto dim = &(data_block->dims[dim_id]);                // the original dimension to be subset

        if (dim->compressed) {
            uncompressDim(dim);
            dim->compressed = 0;                    // Can't preserve this status after the subset has been applied
            dim->method = 0;

            free(dim->sams);
            free(dim->offs);
            free(dim->ints);

            dim->udoms = 0;
            dim->sams = nullptr;        // Avoid double freeing of Heap
            dim->offs = nullptr;
            dim->ints = nullptr;
        }

        //----------------------------------------------------------------------------------------------------------------------------
        // Copy all existing Dimension Information to working structure

        new_dim = *dim;
        new_dim.dim_n = 0;
        new_dim.dim = nullptr;

        //----------------------------------------------------------------------------------------------------------------------------
        // Subset Operations: Identify subset indices

        // Sub-setting Indices

//        int start = -1;                 // Starting Index satisfying the operation
//        int end = -1;                   // Ending Index
        int start1 = -1;
        int end1 = -1;

        // Test for Array Reshaping Operations

        int reshape = 0;                // Sub-setting has been defined using array indexing notation

        auto not_operation = (operation[0] == '!');        // a NOT operator => One or Two subsets

        if (operation[0] == '*') {
            continue;        // This means No dimensional subset - Part of an array Reshape Operation
        }

        int dim_n = 0;

        int start = 0;
        int end = dim->dim_n;
        int stride = 1;

        if (operation[0] == ':') {                  // Reshape Operation - Index Range Specified
            auto maybe_start = subset.lbindex[j];         // Number before the first :
            auto maybe_end = subset.ubindex[j];           // Number after the first :
            auto maybe_stride = subset.stride[j];     // Number after the second :

            start = (int)(maybe_start.init ? maybe_start.value : 0);
            if (start < 0) {
                start = dim->dim_n + start;
            }

            end = (int)(maybe_end.init ? maybe_end.value : dim->dim_n);
            if (end < 0) {
                end = dim->dim_n + end;
            }

            stride = (int)(maybe_stride.init ? maybe_stride.value : 1);

            if (start > end) {
                THROW_ERROR(999, "start must be before end")
            }
            reshape = 1;
            dim_n = abs((end - start) / stride);
        }

        if (operation[0] == '#') {            // Reshape Operation - Highest array position (last value)
            start = dim->dim_n - 1;
            end = dim->dim_n;
            reshape = 1;
            dim_n = 1;
        }

        // Create an Array of Indices satisfying the criteria

        if (!reshape) {

            auto subset_indices = (unsigned int*)malloc(dim->dim_n * sizeof(unsigned int));

            if (operation == "!<") {
                operation = ">=";
            }
            if (operation == "!>") {
                operation = "<=";
            }
            if (operation == "!<=") {
                operation = ">";
            }
            if (operation == "!<=") {
                operation = "<";
            }

            if ((dim_n = get_subset_indices(operation, dim, value, subset_indices)) == 0) {
                free(subset_indices);
                THROW_ERROR(9999, "No Data were found that satisfies a subset");
            }

            // Start and End of Subset Ranges

            start = (int)subset_indices[0];
            end = (int)subset_indices[dim_n - 1];

            if (not_operation && dim_n > 1) {        // Double Range	?
                int range2 = 0;
                if (dim_n == dim->dim_n) {
                    not_operation = false;            // No Second Range found so switch OFF NOT Operation
                } else {
                    end1 = (int)subset_indices[dim_n - 1];
                    for (int k = 0; k < dim_n; k++) {
                        if (subset_indices[k] != subset_indices[0] + k) {
                            end = (int)subset_indices[k - 1];
                            start1 = (int)subset_indices[k];
                            break;
                        }
                    }
                    range2 = end1 - start1 + 1;
                }
                if (dim_n != end - start + 1 + range2) {        // Dimension array is Not well ordered!
                    free(subset_indices);
                    THROW_ERROR(9999, "The Dimensional Array is Not Ordered: Unable to Subset");
                }
            } else {
                if (dim_n != end - start + 1) {        // Dimension array is Not well ordered!
                    free(subset_indices);
                    THROW_ERROR(9999, "The Dimensional Array is Not Ordered: Unable to Subset");
                }
            }
            free(subset_indices);
        }

        new_dim.dim_n = dim_n;

        //----------------------------------------------------------------------------------------------------------------------------
        // Build the New Sub-setted Dimension

        printDataBlock(*data_block);
        UDA_LOG(UDA_LOG_DEBUG, "\n\n\n*** dim->data_type: %d\n\n\n", dim->data_type);
        UDA_LOG(UDA_LOG_DEBUG, "\n\n\n*** dim->errhi != nullptr: %d\n\n\n", dim->errhi != nullptr);
        UDA_LOG(UDA_LOG_DEBUG, "\n\n\n*** dim->errlo != nullptr: %d\n\n\n", dim->errlo != nullptr);

        int n;
        int ierr = 0;
        
        if ((ierr = apply_sub_setting(dim, 1, dim_id, dim->dim, dim_n, dim->data_type, not_operation,
                                        start, end, start1, end1, stride, &n, (void**)&new_dim.dim)) != 0) {
            return ierr;
        }

        if (dim->errhi != nullptr && dim->error_type != UDA_TYPE_UNKNOWN) {
            if ((ierr = apply_sub_setting(dim, 1, dim_id, dim->errhi, dim_n, dim->error_type, not_operation,
                                            start, end, start1, end1, stride, &n, (void**)&new_dim.errhi)) != 0)
                return ierr;
        }

        if (dim->errlo != nullptr && dim->error_type != UDA_TYPE_UNKNOWN) {
            if ((ierr = apply_sub_setting(dim, 1, dim_id, dim->errlo, dim_n, dim->error_type, not_operation,
                                            start, end, start1, end1, stride, &n, (void**)&new_dim.errlo)) != 0)
                return ierr;
        }

        //-----------------------------------------------------------------------------------------------------------------------
        // Reshape and Save the Subsetted Data

        printDataBlock(*data_block);

        int n_data;
        char* new_data;
        
        if ((ierr = apply_sub_setting(data_block->dims, data_block->rank, dim_id, data_block->data,
                                        data_block->data_n, data_block->data_type, not_operation,
                                        start, end, start1, end1, stride, &n_data, (void**)&new_data)) != 0) {
            return ierr;
        }

        char* new_errhi;
        
        if (data_block->error_type != UDA_TYPE_UNKNOWN && data_block->errhi != nullptr) {
            if ((ierr = apply_sub_setting(data_block->dims, data_block->rank, dim_id, data_block->errhi,
                                            data_block->data_n, data_block->error_type, not_operation,
                                            start, end, start1, end1, stride, &n, (void**)&new_errhi)) != 0) {
                return ierr;
            }
            free(data_block->errhi);                // Free Original Heap
            data_block->errhi = new_errhi;                // Replace with the Reshaped Array
        }
        
        char* new_errlo;

        if (data_block->error_type != UDA_TYPE_UNKNOWN && dim->errlo != nullptr) {
            if ((ierr = apply_sub_setting(data_block->dims, data_block->rank, dim_id, data_block->errlo,
                                            data_block->data_n, data_block->error_type, not_operation,
                                            start, end, start1, end1, stride, &n, (void**)&new_errlo)) != 0) {
                return ierr;
            }
            free(data_block->errlo);                // Free Original Heap
            data_block->errlo = new_errlo;                // Replace with the Reshaped Array
        }

        data_block->data_n = n_data;

        free(data_block->data);                // Free Original Heap
        data_block->data = new_data;                    // Replace with the Reshaped Array

        // replace the Original Dimensional Structure with the New Subsetted Structure unless a
        // REFORM [Rank Reduction] has been requested and the dimension length is 1 (this has no effect on the Data Array items)

        // Free Heap associated with the original Dimensional Structure Array

        free(dim->dim);
        free(dim->errlo);
        free(dim->errhi);

        dim->dim = nullptr;
        dim->errlo = nullptr;
        dim->errhi = nullptr;

        // Save the reshaped Dimension or Reform the whole

        data_block->dims[dim_id] = new_dim;                            // Replace with the subsetted dimension
    }
    
    return 0;
}

int reform_data(DATA_BLOCK* data_block)
{
    int rank = data_block->rank;
    for (int j = 0; j < rank; j++) {
        if (data_block->dims[j].dim_n <= 1) {
            UDA_LOG(UDA_LOG_DEBUG, "Reforming Dimension %d\n", j);

            data_block->dims[j].compressed = 0;
            data_block->dims[j].method = 0;

            if (data_block->dims[j].dim != nullptr) free(data_block->dims[j].dim);
            if (data_block->dims[j].errlo != nullptr) free(data_block->dims[j].errlo);
            if (data_block->dims[j].errhi != nullptr) free(data_block->dims[j].errhi);
            if (data_block->dims[j].sams != nullptr) free(data_block->dims[j].sams);
            if (data_block->dims[j].offs != nullptr) free(data_block->dims[j].offs);
            if (data_block->dims[j].ints != nullptr) free(data_block->dims[j].ints);

            for (int k = j + 1; k < rank; k++) {
                data_block->dims[k - 1] = data_block->dims[k];            // Shift array contents
            }

            if (data_block->order == j) {
                data_block->order = -1;                                // No Time Dimension if Reformed
            } else {
                if (data_block->order > j) {
                    data_block->order = data_block->order - 1;
                }        // Time Dimension ID reduced by 1
            }

            data_block->rank = data_block->rank - 1;                        // Reduce the Rank
        }
    }
    return 0;
}

int apply_minimum(SUBSET subset, DATA_BLOCK* data_block)
{
    int dim_id = 0;

    if (data_block->rank >= 1) {
        char* p1 = strstr(subset.function, "dim_id");
        if (p1 != nullptr) {
            char* p3, * p2 = strchr(p1, '=');
            p2[0] = ' ';
            p3 = strchr(p2, ')');
            p3[0] = '\0';
            if (IsNumber(p2)) {
                dim_id = atoi(p2);
            } else {
                // ERROR
            }
        } else {
            // ERROR
        }
    }

    if (dim_id < 0 || dim_id >= (int)data_block->rank) {
        UDA_LOG(UDA_LOG_ERROR, "Function Syntax Error -  dim_id = %d,  Rank = %d\n", dim_id,
                data_block->rank);
        THROW_ERROR(999, "The dimension ID identified via the subset function is outside the rank bounds of the array!");
    }

    switch (data_block->data_type) {
        case UDA_TYPE_FLOAT: {
            auto dp = (float*)data_block->data;
            float min = dp[0];
            switch (data_block->rank) {
                case 0: {            // Ignore function dim_id argument
                    for (int j = 1; j < data_block->data_n; j++)
                        if (dp[j] < min) {
                            min = dp[j];
                        }
                    dp[0] = min;
                    dp = (float*)realloc((void*)dp, sizeof(float));        // Reduce array size
                    data_block->data_n = 1;
                    break;
                }
                case 1: {
                    for (int j = 1; j < data_block->dims[0].dim_n; j++) {
                        if (dp[j] < min) {
                            min = dp[j];
                        }
                    }
                    dp[0] = min;
                    dp = (float*)realloc((void*)dp, sizeof(float));        // Reduce array size
                    data_block->rank = 0;
                    data_block->data_n = 1;
                    free(data_block->dims[0].dim);
                    free(data_block->dims);
                    data_block->dims = nullptr;
                    break;
                }
                case 2: {
                    float* ddp;
                    if (dim_id == 0) {
                        ddp = (float*)malloc(data_block->dims[1].dim_n * sizeof(float));
                        for (int i = 0; i < data_block->dims[1].dim_n; i++) {
                            min = dp[i * data_block->dims[0].dim_n];
                            for (int j = 1; j < data_block->dims[0].dim_n; j++) {
                                int k = j + i * data_block->dims[0].dim_n;
                                if (dp[k] < min) {
                                    min = dp[k];
                                }
                            }
                            ddp[i] = min;
                        }
                        free(dp);
                        data_block->rank = 1;
                        data_block->data_n = data_block->dims[1].dim_n;
                        data_block->data = (char*)ddp;
                        free(data_block->dims[0].dim);
                        data_block->dims[0] = data_block->dims[1];
                    } else {
                        ddp = (float*)malloc(data_block->dims[0].dim_n * sizeof(float));
                        for (int j = 0; j < data_block->dims[0].dim_n; j++) {
                            min = dp[j];
                            for (int i = 1; i < data_block->dims[1].dim_n; i++) {
                                int k = j + i * data_block->dims[0].dim_n;
                                if (dp[k] < min) {
                                    min = dp[k];
                                }
                            }
                            ddp[j] = min;
                        }
                        free(dp);
                        data_block->rank = 1;
                        data_block->data_n = data_block->dims[0].dim_n;
                        data_block->data = (char*)ddp;
                        free(data_block->dims[1].dim);
                        break;
                    }
                    break;
                }

                default:
                    break;
            }
            break;
        }

        case UDA_TYPE_DOUBLE: {
            auto dp = (double*)data_block->data;
            double min = dp[0];
            switch (data_block->rank) {
                case 0: {            // Ignore function dim_id argument
                    for (int j = 1; j < data_block->data_n; j++)
                        if (dp[j] < min) {
                            min = dp[j];
                        }
                    dp[0] = min;
                    dp = (double*)realloc((void*)dp, sizeof(double));        // Reduce array size
                    data_block->data_n = 1;
                    break;
                }
                case 1: {
                    for (int j = 1; j < data_block->dims[0].dim_n; j++) {
                        if (dp[j] < min) {
                            min = dp[j];
                        }
                    }
                    dp[0] = min;
                    dp = (double*)realloc((void*)dp, sizeof(double));        // Reduce array size
                    data_block->rank = 0;
                    data_block->data_n = 1;
                    free(data_block->dims[0].dim);
                    free(data_block->dims);
                    data_block->dims = nullptr;
                    break;
                }
                case 2: {
                    double* ddp;
                    if (dim_id == 0) {
                        ddp = (double*)malloc(data_block->dims[1].dim_n * sizeof(double));
                        for (int i = 0; i < data_block->dims[1].dim_n; i++) {
                            min = dp[i * data_block->dims[0].dim_n];
                            for (int j = 1; j < data_block->dims[0].dim_n; j++) {
                                int k = j + i * data_block->dims[0].dim_n;
                                if (dp[k] < min) {
                                    min = dp[k];
                                }
                            }
                            ddp[i] = min;
                        }
                        free(dp);
                        data_block->rank = 1;
                        data_block->data_n = data_block->dims[1].dim_n;
                        data_block->data = (char*)ddp;
                        free(data_block->dims[0].dim);
                        data_block->dims[0] = data_block->dims[1];
                    } else {
                        ddp = (double*)malloc(data_block->dims[0].dim_n * sizeof(double));
                        for (int j = 0; j < data_block->dims[0].dim_n; j++) {
                            min = dp[j];
                            for (int i = 1; i < data_block->dims[1].dim_n; i++) {
                                int k = j + i * data_block->dims[0].dim_n;
                                if (dp[k] < min) {
                                    min = dp[k];
                                }
                            }
                            ddp[j] = min;
                        }
                        free(dp);
                        data_block->rank = 1;
                        data_block->data_n = data_block->dims[0].dim_n;
                        data_block->data = (char*)ddp;
                        free(data_block->dims[1].dim);
                        break;
                    }
                    break;
                }

                default:
                    break;
            }
            break;
        }

        default:
            break;
    }
    return 0;
}

int apply_maximum(SUBSET subset, DATA_BLOCK* data_block)
{
    int dim_id = 0;

    if (data_block->rank >= 1) {
        char* p1 = strstr(subset.function, "dim_id");
        if (p1 != nullptr) {
            char* p3, * p2 = strchr(p1, '=');
            p2[0] = ' ';
            p3 = strchr(p2, ')');
            p3[0] = '\0';
            if (IsNumber(p2)) {
                dim_id = atoi(p2);
            } else {
                // ERROR
            }
        } else {
            // ERROR
        }
    }

    if (dim_id < 0 || dim_id >= (int)data_block->rank) {
        UDA_LOG(UDA_LOG_ERROR, "Function Syntax Error -  dim_id = %d,  Rank = %d\n", dim_id,
                data_block->rank);
        THROW_ERROR(999, "The dimension ID identified via the subset function is outside the rank bounds of the array!");
    }

    switch (data_block->data_type) {
        case UDA_TYPE_FLOAT: {
            auto dp = (float*)data_block->data;
            float max = dp[0];
            switch (data_block->rank) {
                case 0: {            // Ignore function dim_id argument
                    for (int j = 1; j < data_block->data_n; j++) {
                        if (dp[j] > max) {
                            max = dp[j];
                        }
                    }
                    dp[0] = max;
                    dp = (float*)realloc((void*)dp, sizeof(float));        // Reduce array size
                    data_block->data_n = 1;
                    break;
                }
                case 1: {
                    for (int j = 1; j < data_block->dims[0].dim_n; j++) {
                        if (dp[j] > max) {
                            max = dp[j];
                        }
                    }
                    dp[0] = max;
                    dp = (float*)realloc((void*)dp, sizeof(float));        // Reduce array size
                    data_block->rank = 0;
                    data_block->data_n = 1;
                    free(data_block->dims[0].dim);
                    free(data_block->dims);
                    data_block->dims = nullptr;
                    break;
                }
                case 2: {
                    float* ddp;
                    if (dim_id == 0) {
                        ddp = (float*)malloc(data_block->dims[1].dim_n * sizeof(float));
                        for (int i = 0; i < data_block->dims[1].dim_n; i++) {
                            max = dp[i * data_block->dims[0].dim_n];
                            for (int j = 1; j < data_block->dims[0].dim_n; j++) {
                                int k = j + i * data_block->dims[0].dim_n;
                                if (dp[k] > max) {
                                    max = dp[k];
                                }
                            }
                            ddp[i] = max;
                        }
                        free(dp);
                        data_block->rank = 1;
                        data_block->data_n = data_block->dims[1].dim_n;
                        data_block->data = (char*)ddp;
                        free(data_block->dims[0].dim);
                        data_block->dims[0] = data_block->dims[1];
                    } else {
                        ddp = (float*)malloc(data_block->dims[0].dim_n * sizeof(float));
                        for (int j = 0; j < data_block->dims[0].dim_n; j++) {
                            max = dp[j];
                            for (int i = 1; i < data_block->dims[1].dim_n; i++) {
                                int k = j + i * data_block->dims[0].dim_n;
                                if (dp[k] > max) {
                                    max = dp[k];
                                }
                            }
                            ddp[j] = max;
                        }
                        free(dp);
                        data_block->rank = 1;
                        data_block->data_n = data_block->dims[0].dim_n;
                        data_block->data = (char*)ddp;
                        free(data_block->dims[1].dim);
                        break;
                    }
                    break;
                }

                default:
                    break;
            }
            break;
        }

        case UDA_TYPE_DOUBLE: {
            auto dp = (double*)data_block->data;
            double max = dp[0];
            switch (data_block->rank) {
                case 0: {            // Ignore function dim_id argument
                    for (int j = 1; j < data_block->data_n; j++)
                        if (dp[j] > max) {
                            max = dp[j];
                        }
                    dp[0] = max;
                    dp = (double*)realloc((void*)dp, sizeof(double));        // Reduce array size
                    data_block->data_n = 1;
                    break;
                }
                case 1: {
                    for (int j = 1; j < data_block->dims[0].dim_n; j++) {
                        if (dp[j] > max) {
                            max = dp[j];
                        }
                    }
                    dp[0] = max;
                    dp = (double*)realloc((void*)dp, sizeof(double));        // Reduce array size
                    data_block->rank = 0;
                    data_block->data_n = 1;
                    free(data_block->dims[0].dim);
                    free(data_block->dims);
                    data_block->dims = nullptr;
                    break;
                }
                case 2: {
                    double* ddp;
                    if (dim_id == 0) {
                        ddp = (double*)malloc(data_block->dims[1].dim_n * sizeof(double));
                        for (int i = 0; i < data_block->dims[1].dim_n; i++) {
                            max = dp[i * data_block->dims[0].dim_n];
                            for (int j = 1; j < data_block->dims[0].dim_n; j++) {
                                int k = j + i * data_block->dims[0].dim_n;
                                if (dp[k] > max) {
                                    max = dp[k];
                                }
                            }
                            ddp[i] = max;
                        }
                        free(dp);
                        data_block->rank = 1;
                        data_block->data_n = data_block->dims[1].dim_n;
                        data_block->data = (char*)ddp;
                        free(data_block->dims[0].dim);
                        data_block->dims[0] = data_block->dims[1];
                    } else {
                        ddp = (double*)malloc(data_block->dims[0].dim_n * sizeof(double));
                        for (int j = 0; j < data_block->dims[0].dim_n; j++) {
                            max = dp[j];
                            for (int i = 1; i < data_block->dims[1].dim_n; i++) {
                                int k = j + i * data_block->dims[0].dim_n;
                                if (dp[k] > max) {
                                    max = dp[k];
                                }
                            }
                            ddp[j] = max;
                        }
                        free(dp);
                        data_block->rank = 1;
                        data_block->data_n = data_block->dims[0].dim_n;
                        data_block->data = (char*)ddp;
                        free(data_block->dims[1].dim);
                        break;
                    }
                    break;
                }

                default:
                    break;
            }
            break;
        }

        default:
            break;
    }
    return 0;
}

int apply_count(SUBSET subset, DATA_BLOCK* data_block)
{
    char* p1 = strstr(subset.function, "dim_id");
    auto count = (unsigned int*)malloc(sizeof(unsigned int));
    if (p1 == nullptr) {
        count[0] = (unsigned int)data_block->data_n;
        freeDataBlock(data_block);
        initDataBlock(data_block);
        data_block->data_n = 1;
        data_block->data = (char*)count;
        data_block->data_type = UDA_TYPE_UNSIGNED_INT;
    } else {
        int dim_id = 0;
        if (data_block->rank >= 1) {
            char* p3, * p2 = strchr(p1, '=');
            p2[0] = ' ';
            p3 = strchr(p2, ')');
            p3[0] = '\0';
            if (IsNumber(p2)) {
                dim_id = atoi(p2);
            } else {
                // ERROR
            }
        }
        if (dim_id < (int)data_block->rank) {
            count[0] = (unsigned int)data_block->dims[dim_id].dim_n;        // Preserve this value
            DIMS ddim = data_block->dims[dim_id];
            if (ddim.dim != nullptr) free(ddim.dim);
            if (ddim.errhi != nullptr) free(ddim.errhi);
            if (ddim.errlo != nullptr) free(ddim.errlo);
            if (ddim.sams != nullptr) free(ddim.sams);
            if (ddim.offs != nullptr) free(ddim.offs);
            if (ddim.ints != nullptr) free(ddim.ints);
        } else {
            // ERROR
        }
        if (data_block->data != nullptr) free(data_block->data);
        if (data_block->errhi != nullptr) free(data_block->errhi);
        if (data_block->errlo != nullptr) free(data_block->errlo);
        data_block->data = nullptr;
        data_block->errhi = nullptr;
        data_block->errlo = nullptr;
        data_block->error_type = UDA_TYPE_UNKNOWN;
        data_block->error_param_n = 0;

        data_block->data_n = 1;
        for (unsigned int j = 0; j < data_block->rank - 1; j++) {
            if (j >= (unsigned int)dim_id) {
                data_block->dims[j] = data_block->dims[j + 1];        // skip over the target
            }
            data_block->data_n = data_block->data_n * data_block->dims[j].dim_n;
        }
        data_block->rank = data_block->rank - 1;
        data_block->data_type = UDA_TYPE_UNSIGNED_INT;

        count = (unsigned int*)realloc((void*)count, data_block->data_n * sizeof(unsigned int));
        for (int j = 1; j < data_block->data_n; j++) {
            count[j] = count[0];
        }
        data_block->data = (char*)count;
        data_block->data_units[0] = '\0';
        sprintf(data_block->data_label, "count(dim_id=%d)", dim_id);
    }

    return 0;
}

int apply_abs(SUBSET subset, DATA_BLOCK* data_block)
{
    switch (data_block->data_type) {
        case UDA_TYPE_FLOAT: {
            auto dp = (float*)data_block->data;
            for (int j = 0; j < data_block->data_n; j++) {
                dp[j] = fabsf(dp[j]);
            }
            break;
        }
        case UDA_TYPE_DOUBLE: {
            auto dp = (double*)data_block->data;
            for (int j = 0; j < data_block->data_n; j++) {
                dp[j] = fabs(dp[j]);
            }
            break;
        }
        default:
            break;
    }
    return 0;
}

int apply_const(SUBSET subset, DATA_BLOCK* data_block)
{
    double value = 0.0;                    // Zero data default
    char* p1 = strstr(subset.function, "value");
    strcpy(data_block->data_label, subset.function);

    UDA_LOG(UDA_LOG_DEBUG, "%s\n", subset.function);

    if (p1 != nullptr) {
        char* p3, * p2 = strchr(&p1[5], '=');
        p2[0] = ' ';
        p3 = strchr(p2, ')');
        p3[0] = '\0';
        TrimString(p2);
        LeftTrimString(p2);
        UDA_LOG(UDA_LOG_DEBUG, "p2 = [%s]\n", p2);
        if (IsFloat(p2)) {
            value = atof(p2);
        } else {
            UDA_LOG(UDA_LOG_DEBUG, "IsFloat FALSE!\n");
            // ERROR
        }
    }

    UDA_LOG(UDA_LOG_DEBUG, "value = %f\n", value);

    if (data_block->errhi != nullptr) free(data_block->errhi);
    if (data_block->errlo != nullptr) free(data_block->errlo);
    data_block->errhi = nullptr;
    data_block->errlo = nullptr;
    data_block->error_type = UDA_TYPE_UNKNOWN;
    data_block->error_param_n = 0;

    data_block->data_units[0] = '\0';

    switch (data_block->data_type) {
        case UDA_TYPE_FLOAT: {
            auto dp = (float*)data_block->data;
            for (int j = 0; j < data_block->data_n; j++) {
                dp[j] = (float)value;
            }
            break;
        }
        case UDA_TYPE_DOUBLE: {
            auto dp = (double*)data_block->data;
            for (int j = 0; j < data_block->data_n; j++) {
                dp[j] = value;
            }
            break;
        }
        default:
            break;
    }
    return 0;
}

int apply_order(SUBSET subset, DATA_BLOCK* data_block)
{
    char* p1 = strstr(subset.function, "dim_id");
    UDA_LOG(UDA_LOG_DEBUG, "%s\n", subset.function);
    if (p1 != nullptr) {
        char* p3, * p2 = strchr(&p1[5], '=');
        p2[0] = ' ';
        p3 = strchr(p2, ')');
        p3[0] = '\0';
        TrimString(p2);
        LeftTrimString(p2);
        UDA_LOG(UDA_LOG_DEBUG, "p2 = [%s]\n", p2);
        if (IsNumber(p2)) {
            data_block->order = (int)atof(p2);
        } else {
            // ERROR
        }
    }
    UDA_LOG(UDA_LOG_DEBUG, "order = %d\n", data_block->order);
    return 0;
}

int apply_rotate_rz(SUBSET subset, DATA_BLOCK* data_block)
{
    UDA_LOG(UDA_LOG_DEBUG, "%s\n", subset.function);
    if (data_block->rank != 3) {
        THROW_ERROR(999, "The function rotateRZ only operates on rank 3 arrays");
    }
    int order = data_block->order;
    if (order < 0) {
        THROW_ERROR(999, "The function rotateRZ expects a Time coordinate");
    }
    int type = data_block->data_type;
    if (type != UDA_TYPE_DOUBLE) {
        THROW_ERROR(999, "The function rotateRZ is configured for type DOUBLE only");
    }
    int nt, nr, nz, count;
    count = data_block->data_n;
    auto newData = (double*)malloc(count * sizeof(double));
    unsigned int offset = 0;
    auto old = (double*)data_block->data;
    if (order == 0) {        // array[nz][nr][nt] -> [nr][nz][nt]
        nt = data_block->dims[0].dim_n;
        nr = data_block->dims[1].dim_n;
        nz = data_block->dims[2].dim_n;

        auto data = (double***)malloc(nz * sizeof(double**));
        for (int j = 0; j < nz; j++) {
            data[j] = (double**)malloc(nr * sizeof(double*));
            for (int i = 0; i < nr; i++) {
                data[j][i] = (double*)malloc(nt * sizeof(double));
                for (int k = 0; k < nt; k++)data[j][i][k] = old[offset++];
            }
        }
        offset = 0;
        for (int i = 0; i < nr; i++)
            for (int j = 0; j < nz; j++)
                for (int k = 0; k < nt; k++)
                    newData[offset++] = data[j][i][k];
        for (int j = 0; j < nz; j++) {
            for (int i = 0; i < nr; i++) free(data[j][i]);
            free(data[j]);
        }
        free(data);

        DIMS d1 = data_block->dims[1];
        DIMS d2 = data_block->dims[2];
        data_block->dims[1] = d2;
        data_block->dims[2] = d1;
    } else if (order == 1) {        // array[nz][nt][nr]
        THROW_ERROR(999, "The function rotateRZ only operates on arrays with shape [nz][nr][nt] or [nt][nz][nr]");
    } else if (order == 2) {        // array[nt][nz][nr] -> [nt][nr][nz]
        nr = data_block->dims[0].dim_n;
        nz = data_block->dims[1].dim_n;
        nt = data_block->dims[2].dim_n;

        auto data = (double***)malloc(nt * sizeof(double**));
        for (int k = 0; k < nt; k++) {
            data[k] = (double**)malloc(nz * sizeof(double*));
            for (int j = 0; j < nz; j++) {
                data[k][j] = (double*)malloc(nr * sizeof(double));
                for (int i = 0; i < nr; i++)data[k][j][i] = old[offset++];
            }
        }
        offset = 0;
        for (int k = 0; k < nt; k++)
            for (int i = 0; i < nr; i++)
                for (int j = 0; j < nz; j++)
                    newData[offset++] = data[k][j][i];
        for (int k = 0; k < nt; k++) {
            for (int j = 0; j < nz; j++)free(data[k][j]);
            free(data[k]);
        }
        free(data);
        DIMS d0 = data_block->dims[0];
        DIMS d1 = data_block->dims[1];
        data_block->dims[0] = d1;
        data_block->dims[1] = d0;
    } else {
        THROW_ERROR(999, "rotateRZ: Incorrect ORDER value found!");
    }
    free(data_block->data);
    data_block->data = (char*)newData;
    return 0;
}

int apply_functions(SUBSET subset, DATA_BLOCK* data_block)
{
    if (STR_ISTARTSWITH(subset.function, "minimum")) {        // Single scalar result
        return apply_minimum(subset, data_block);
    }

    if (STR_ISTARTSWITH(subset.function, "maximum")) {        // Single scalar result
        return apply_maximum(subset, data_block);
    }

    if (STR_ISTARTSWITH(subset.function, "count")) {        // Single scalar result
        return apply_count(subset, data_block);
    }

    if (STR_ISTARTSWITH(subset.function, "abs")) {            // Absolute value
        return apply_abs(subset, data_block);
    }

    if (STR_ISTARTSWITH(subset.function, "const")) {        // Constant value substitution
        return apply_const(subset, data_block);
    }

    if (STR_ISTARTSWITH(subset.function, "order")) {        // Identify the Time dimension order
        return apply_order(subset, data_block);
    }

    if (STR_ISTARTSWITH(subset.function, "rotateRZ")) {        // Rotate R,Z coordinates in rank 3 array
        return apply_rotate_rz(subset, data_block);
    }

    THROW_ERROR(999, "Unknown function");
}

int serverSubsetData(DATA_BLOCK* data_block, const ACTION& action, LOGMALLOCLIST* logmalloclist)
{
    printAction(action);
    printDataBlock(*data_block);

    //-----------------------------------------------------------------------------------------------------------------------
    // How many sets of sub-setting operations?

    int n_subsets = number_of_subsetting_operations(&action);

    //-----------------------------------------------------------------------------------------------------------------------
    // Check Rank

    if (data_block->rank > 2 &&
        !(action.actionType == UDA_SUBSET_TYPE && !strncasecmp(action.subset.function, "rotateRZ", 8))) {
        THROW_ERROR(9999, "Not Configured to Subset Data with Rank Higher than 2");
    }

    //-----------------------------------------------------------------------------------------------------------------------
    // Process all sets of sub-setting operations

    for (int i = 0; i < n_subsets; i++) {                        // the number of sets of Subset Operations
        SUBSET subset;
        if (action.actionType == UDA_COMPOSITE_TYPE) {
            subset = action.composite.subsets[i];                // the set of Subset Operations
        } else {
            if (action.actionType == UDA_SERVER_SIDE_TYPE) {
                subset = action.serverside.subsets[i];
            } else {
                if (action.actionType == UDA_SUBSET_TYPE) {
                    subset = action.subset;
                }
            }
        }

        int rc = process_subset_operation(i, subset, data_block, logmalloclist);
        if (rc != 0) {
            return rc;
        }

        //-------------------------------------------------------------------------------------------------------------
        // Reform the Data if requested
        if (subset.reform) {
            reform_data(data_block);
        }

        //-------------------------------------------------------------------------------------------------------------
        // Apply simple functions to the subset data: XML syntax function="name(dim_id=9)"
        if (subset.function[0] != '\0') {
            apply_functions(subset, data_block);
        }

        //-------------------------------------------------------------------------------------------------------------
        // Explicitly set the order of the time dimension if not possible via the other options
        if (subset.order >= 0) {
            data_block->order = subset.order;
        }
    }

    return 0;
}




//-------------------------------------------------------------------------------------------------------------
//-------------------------------------------------------------------------------------------------------------
// Build an Action Structure for Serverside Data Operations

// SS::SUBSET(\"xx\", [!=0.15])
// SS::SUBSET(\"xx\", [0:25])
// SS::SUBSET(\"xx\", [=0.15, *], reform)
// SS::SUBSET(\"xx\", [0:5, *])
// SS::SUBSET(\"xx\", [0:0, *], reform)
// SS::SUBSET(\"xx\", [*, 2:8])
// SS::SUBSET(\"xx\", [*, 2:8], member=\"name\")
// SS::SUBSET(\"xx\", [*, 3], member=\"name\", reform)
// SS::SUBSET(\"xx\", [*, 3], member=\"name\", reform, function=\"minimum(dim_id=0)\" )

int serverParseServerSide(REQUEST_DATA* request_block, ACTIONS* actions_serverside, const PLUGINLIST* plugin_list)
{
    ACTION* action = nullptr;
    SUBSET* subsets = nullptr;

    int ierr = 0;

    //-------------------------------------------------------------------------------------------------------------
    // Extract the ARCHIVE::SIGNAL element
    // use the first character after the left parenthesis as the opening quotation character

    std::string signal = request_block->signal;
    char quote_char = signal[7];

    size_t quote_pos = signal.find(quote_char, 8);
    if (quote_pos == std::string::npos) {
        THROW_ERROR(9999, "Syntax Error: The Signal Name has no Terminating Quotation character!");
    }

    std::string remainder = signal.substr(quote_pos + 1);
    signal = signal.substr(8, quote_pos - 8);

    // Check if the data_object(signal) is of the form: ARCHIVE::Signal

    std::string archive;
    bool is_function = false;

    if (boost::contains(signal, "::")) {
        size_t n = signal.find("::");
        archive = signal.substr(0, n);
        signal = signal.substr(n + 2);
        if (boost::contains(signal, "(")) {
            is_function = true;
        }
    }

    //-------------------------------------------------------------------------------------------------------------
    // Extract the Subset Operation

    boost::trim(remainder);

    if (remainder[0] != ',') {
        // Locate the separation character
        THROW_ERROR(9999, "Syntax Error: No Comma after the Signal Name");
    }

    size_t lbracket_pos = remainder.find('[');

    if (lbracket_pos == std::string::npos) {
        // Locate the Operation
        THROW_ERROR(9999, "Syntax Error: No [ enclosing the Operation");
    }

    size_t rbracket_pos = remainder.find(']', lbracket_pos);

    if (rbracket_pos == std::string::npos) {
        THROW_ERROR(9999, "Syntax Error: No ] enclosing the Operation ");
    }

    std::string operation = remainder.substr(lbracket_pos + 1, rbracket_pos - lbracket_pos - 1);

    //-------------------------------------------------------------------------------------------------------------
    // remaining options, e.g., REFORM

    std::string options = remainder.substr(rbracket_pos + 1);

    //-------------------------------------------------------------------------------------------------------------
    // Overwrite the Request Block to enable the correct access to signal data before the subset operations are applied

    strcpy(request_block->archive, archive.c_str());
    if (request_block->archive[0] == '\0') {
        strcpy(request_block->archive, getServerEnvironment()->api_archive);
    }
    if (is_function) {
        strcpy(request_block->format, archive.c_str());
        request_block->request = findPluginRequestByFormat(archive.c_str(), plugin_list);
        size_t l_pos = signal.find('(');
        size_t r_pos = signal.find(')', l_pos);
        std::string func = signal.substr(0, l_pos);
        std::string args = signal.substr(l_pos + 1, r_pos - l_pos - 1);
        strcpy(request_block->function, func.c_str());
        freeNameValueList(&request_block->nameValueList);
        name_value_pairs(args.c_str(), &request_block->nameValueList, true);
    }

    strcpy(request_block->signal, signal.c_str());

    //-------------------------------------------------------------------------------------------------------------
    // Extend the Action Structure and Initialise

    int nactions = actions_serverside->nactions + 1;
    if ((action = (ACTION*)realloc((void*)actions_serverside->action, nactions * sizeof(ACTION))) == nullptr) {
        THROW_ERROR(9999, "Unable to Allocate Heap memory");
    }

    initAction(&action[nactions - 1]);

    action[nactions - 1].actionType = UDA_SERVER_SIDE_TYPE;
    action[nactions - 1].inRange = 1;
    action[nactions - 1].actionId = nactions;

    initServerside(&action[nactions - 1].serverside);

    int nsubsets = 1;
    if ((subsets = (SUBSET*)malloc(sizeof(SUBSET))) == nullptr) {
        THROW_ERROR(9999, "Unable to Allocate Heap memory");
    }

    for (int i = 0; i < nsubsets; i++) {
        initSubset(&subsets[i]);
    }

    action[nactions - 1].serverside.nsubsets = 1;
    strcpy(subsets[nsubsets - 1].data_signal, request_block->signal);

    // Seek specific options

    if (boost::contains(options, "reform")) {
        // Reduce rank
        subsets[nsubsets - 1].reform = 1;
    }

    if (boost::contains(options, "member=")) {
        // Extract a Structure member
        size_t member_pos = options.find("member=");
        strcpy(subsets[nsubsets - 1].member, &options[member_pos + 7]);
        LeftTrimString(subsets[nsubsets - 1].member);
        if (subsets[nsubsets - 1].member[0] == '"') {
            subsets[nsubsets - 1].member[0] = ' ';
            LeftTrimString(subsets[nsubsets - 1].member);
        }
        char* p = nullptr;
        if ((p = strchr(subsets[nsubsets - 1].member, '"')) != nullptr) {
            p[0] = '\0';
        }
        if ((p = strchr(subsets[nsubsets - 1].member, ',')) != nullptr) {
            p[0] = '\0';
        }
    }

    // Simple functions

    if (boost::contains(options, "function=")) {
        // Identify a function
        size_t function_pos = options.find("function=");
        strcpy(subsets[nsubsets - 1].function, &options[function_pos + 9]);
        LeftTrimString(subsets[nsubsets - 1].function);
        if (subsets[nsubsets - 1].function[0] == '"') {
            subsets[nsubsets - 1].function[0] = ' ';
            LeftTrimString(subsets[nsubsets - 1].function);
        }
        char* p = nullptr;
        if ((p = strchr(subsets[nsubsets - 1].function, '"')) != nullptr) {
            p[0] = '\0';
        }
        if ((p = strchr(subsets[nsubsets - 1].function, ',')) != nullptr) {
            p[0] = '\0';
        }
    }

    //-------------------------------------------------------------------------------------------------------------
    // Parse the Operation String for Value and Operation

    int nbound = 0;

    std::string op_string = operation;
    boost::trim(op_string);

    std::vector<std::string> tokens;
    boost::split(tokens, op_string, boost::is_any_of(","), boost::token_compress_off);

    if (tokens.size() > UDA_MAX_DATA_RANK) {
        THROW_ERROR(9999, "The number of Dimensional Operations exceeds the Internal Limit");
    }

    for (auto& token : tokens) {
        boost::trim(token);

        subsets[nsubsets - 1].dimid[nbound] = nbound;
        strcpy(subsets[nsubsets - 1].operation[nbound], token.c_str());

        nbound++;
    }

    subsets[nsubsets - 1].nbound = nbound;

    for (int i = 0; i < nbound; i++) {
        int rc = parseOperation(&subsets[nsubsets - 1]);
        if (rc != 0) {
            return rc;
        }
    }

    action[nactions - 1].serverside.nsubsets = nsubsets;
    action[nactions - 1].serverside.subsets = subsets;

    actions_serverside->action = action;
    actions_serverside->nactions = nactions;

    return ierr;
}


//----------------------------------------------------------------------------------------------------------------------
// Identify the Index Range satisfying a small set of conditional operators

template <typename T>
struct EpsilonSelector
{
    static T Epsilon;
};

template <typename T>
T EpsilonSelector<T>::Epsilon = (T)0;

template <>
double EpsilonSelector<double>::Epsilon = DBL_EPSILON;

template <>
float EpsilonSelector<float>::Epsilon = FLT_EPSILON;


template <typename T>
int get_subset_indices_for_type(const std::string& operation, DIMS* dim, double value, unsigned int* subset_indices)
{
    int count = 0;

    T* p = (T*)dim->dim;
    std::string op_lower = boost::to_lower_copy(operation);

    if (op_lower == "eq" || operation[0] == '=' || operation == "~=") {
        for (int k = 0; k < dim->dim_n; k++) {
            if (p[k] == (T)value) {
                subset_indices[count++] = k;
            }
        }
        if (count == 0 && operation == "~=") {
            for (int k = 0; k < dim->dim_n; k++) {
                if (fabs(p[k] - (T)value) <= EpsilonSelector<T>::Epsilon) {
                    subset_indices[count++] = k;
                }
            }
            if (count == 0) {
                int index = -1;
                double delta;
                double minvalue = fabs((T)value - p[0]);
                for (int k = 0; k < dim->dim_n; k++) {
                    delta = fabs((T)value - p[k]);
                    if (delta < minvalue) {                        // Look for the Single Nearest Value
                        minvalue = delta;
                        index = k;
                    }
                }
                if (index >= 0) {
                    count = 1;
                    subset_indices[0] = index;

                    if (index == 0 ||
                        index == dim->dim_n - 1) {                // Check not an end point by default
                        if (dim->dim_n > 1) {
                            if (index == 0) {
                                delta = fabs(p[1] - p[0]);
                            } else {
                                delta = fabs(p[dim->dim_n - 1] - p[dim->dim_n - 2]);
                            }
                            if (fabs((T)value - p[index]) > delta) count = 0;    // Suspect match!
                        }
                    }
                }
            }
        }
    } else {
        if (op_lower == "lt" || operation == "<") {
            for (int k = 0; k < dim->dim_n; k++) {
                if (p[k] < (T)value) {
                    subset_indices[count++] = k;
                }
            }
        } else if (op_lower == "gt" || operation == ">") {
            for (int k = 0; k < dim->dim_n; k++) {
                if (p[k] > (T)value) {
                    subset_indices[count++] = k;
                }
            }
        } else if (op_lower == "le" || operation == "<=") {
            for (int k = 0; k < dim->dim_n; k++) {
                if (p[k] <= (T)value) {
                    subset_indices[count++] = k;
                }
            }
        } else if (op_lower == "ge" || operation == ">=") {
            for (int k = 0; k < dim->dim_n; k++) {
                if (p[k] >= (T)value) {
                    subset_indices[count++] = k;
                }
            }
        } else if (op_lower == "ne" || operation == "!=" || operation == "!~=") {
            if (boost::starts_with(operation, "!~=")) {
                for (int k = 0; k < dim->dim_n; k++) {
                    if (p[k] != (T)value) {
                        subset_indices[count++] = k;
                    }
                }
            } else {
                int index = -1;
                double delta, minvalue = fabs((T)value - p[0]);
                for (int k = 0; k < dim->dim_n; k++) {
                    delta = fabs((T)value - p[k]);
                    if (delta <
                        minvalue) {                        // Look for the Single Nearest Value
                        minvalue = delta;
                        index = k;
                    }
                }
                if (index >= 0) {
                    for (int k = 0; k < dim->dim_n; k++) {
                        if (k != index) {
                            subset_indices[count++] = k;
                        }            // Drop the single nearest value
                    }
                    if (index == 0 || index == dim->dim_n - 1) {
                        // Check not an end point by default
                        if (dim->dim_n > 1) {
                            if (index == 0) {
                                delta = fabs(p[1] - p[0]);
                            } else {
                                delta = fabs(p[dim->dim_n - 1] - p[dim->dim_n - 2]);
                            }
                            if (fabs((T)value - p[index]) > delta) {
                                count = 0;
                            }    // Suspect match!
                        }
                    }
                }
            }
        }
    }

    return 0;
}


int get_subset_indices(const std::string& operation, DIMS* dim, double value, unsigned int* subset_indices)
{
    int count = 0;

    // Scan the Array applying the Operation

    switch (dim->data_type) {
        case UDA_TYPE_DOUBLE:
            return get_subset_indices_for_type<double>(operation, dim, value, subset_indices);
        case UDA_TYPE_FLOAT:
            return get_subset_indices_for_type<float>(operation, dim, value, subset_indices);
        case UDA_TYPE_INT:
            return get_subset_indices_for_type<int>(operation, dim, value, subset_indices);
        case UDA_TYPE_SHORT:
            return get_subset_indices_for_type<short>(operation, dim, value, subset_indices);
        case UDA_TYPE_LONG:
            return get_subset_indices_for_type<long>(operation, dim, value, subset_indices);
        case UDA_TYPE_UNSIGNED_INT:
            return get_subset_indices_for_type<unsigned int>(operation, dim, value, subset_indices);
        case UDA_TYPE_UNSIGNED_SHORT:
            return get_subset_indices_for_type<unsigned short>(operation, dim, value, subset_indices);
        case UDA_TYPE_UNSIGNED_LONG:
            return get_subset_indices_for_type<unsigned long>(operation, dim, value, subset_indices);
    }

    return count;
}


template <typename T>
int apply_sub_setting_for_type(DIMS* dims, int rank, int dim_id,
                      const char* data, int ndata, int data_type, int not_operation,
                      int start, int end, int start1, int end1, int stride, int* n, void** new_data)
{
    // Allocate heap for the reshaped array

    T* p = nullptr;
    if ((p = (T*)malloc(ndata * sizeof(T))) == nullptr) {
        THROW_ERROR(9999, "Unable to Allocate Heap memory");
    }

    auto dp = (T*)data;        // the Originating Data Array

    // Reshape

    switch (rank) {
        case 1: {
            int k = 0;
            if (stride > 0) {
                for (int i = start; i < end; i += stride) {
                    p[k++] = dp[i];
                }
            } else {
                for (int i = end - 1; i >= start; i += stride) {
                    p[k++] = dp[i];
                }
            }
            *n = k;
            *new_data = (void*)p;
            break;
        }

        case 2: {

            // Original Data

            int rows = dims[1].dim_n;
            int columns = dims[0].dim_n;
            auto dpa = (T**)malloc(rows * sizeof(T*));
            for (int j = 0; j < rows; j++) {
                dpa[j] = &dp[j * columns];
            }

            // Array for Reshaped Data

            int new_rows = 0;
            int new_cols = 0;

            if (dim_id == 0) {
                new_rows = dims[1].dim_n;
                new_cols = end - start + 1;
                if (not_operation) new_cols = new_cols + end1 - start1 + 1;
            } else {
                new_cols = dims[0].dim_n;
                new_rows = end - start + 1;
                if (not_operation) new_rows = new_rows + end1 - start1 + 1;
            }

            auto pa = (T**)malloc(new_rows * sizeof(T*));
            for (int j = 0; j < new_rows; j++) {
                pa[j] = &p[j * new_cols];
            }

            // Reshape the Data

            int count = 0;

            if (dim_id == 0) {
                for (int j = 0; j < rows; j++) {
                    int k = 0;
                    if (stride > 0) {
                        for (int i = start; i < end; i += stride) {
                            pa[j][k++] = dpa[j][i];
                            count++;
                        }
                        if (not_operation) {
                            for (int i = start1; i < end1; i += stride) {
                                pa[j][k++] = dpa[j][i];
                                count++;
                            }
                        }
                    } else {
                        if (not_operation) {
                            for (int i = end1 - 1; i >= start1; i += stride) {
                                pa[j][k++] = dpa[j][i];
                                count++;
                            }
                        }
                        for (int i = end; i >= start; i += stride) {
                            pa[j][k++] = dpa[j][i];
                            count++;
                        }
                    }
                }
            } else {
                int k = 0;
                if (stride > 0) {
                    for (int j = start; j < end; j += stride) {
                        for (int i = 0; i < columns; i++) {
                            pa[k][i] = dpa[j][i];
                            count++;
                        }
                        k++;
                    }
                    if (not_operation) {
                        for (int j = start1; j < end1; j += stride) {
                            for (int i = 0; i < columns; i++) {
                                pa[k][i] = dpa[j][i];
                                count++;
                            }
                            k++;
                        }
                    }
                } else {
                    if (not_operation) {
                        for (int j = end1 - 1; j >= start1; j += stride) {
                            for (int i = 0; i < columns; i++) {
                                pa[k][i] = dpa[j][i];
                                count++;
                            }
                            k++;
                        }
                    }
                    for (int j = end - 1; j >= start; j += stride) {
                        for (int i = 0; i < columns; i++) {
                            pa[k][i] = dpa[j][i];
                            count++;
                        }
                        k++;
                    }
                }
            }

            *new_data = (void*)&pa[0][0];
            *n = count;
            break;
        }
    }

    return 0;
}

int apply_sub_setting(DIMS* dims, int rank, int dim_id,
                        char* data, int ndata, int data_type, int not_operation,
                        int start, int end, int start1, int end1, int stride, int* n, void** new_data)
{
    UDA_LOG(UDA_LOG_DEBUG, "Data Type: %d    Rank: %d\n", data_type, rank);

    *n = 0;

    switch (data_type) {
        case UDA_TYPE_FLOAT:
            return apply_sub_setting_for_type<float>(dims, rank, dim_id, data, ndata, data_type, not_operation, start, end, start1, end1, stride, n, new_data);
            break;
        case UDA_TYPE_DOUBLE:
            return apply_sub_setting_for_type<double>(dims, rank, dim_id, data, ndata, data_type, not_operation, start, end, start1, end1, stride, n, new_data);
            break;
        case UDA_TYPE_INT:
            return apply_sub_setting_for_type<int>(dims, rank, dim_id, data, ndata, data_type, not_operation, start, end, start1, end1, stride, n, new_data);
            break;
        case UDA_TYPE_SHORT:
            return apply_sub_setting_for_type<short>(dims, rank, dim_id, data, ndata, data_type, not_operation, start, end, start1, end1, stride, n, new_data);
            break;
        case UDA_TYPE_LONG:
            return apply_sub_setting_for_type<long>(dims, rank, dim_id, data, ndata, data_type, not_operation, start, end, start1, end1, stride, n, new_data);
            break;
        case UDA_TYPE_UNSIGNED_INT:
            return apply_sub_setting_for_type<unsigned int>(dims, rank, dim_id, data, ndata, data_type, not_operation, start, end, start1, end1, stride, n, new_data);
            break;
        case UDA_TYPE_UNSIGNED_SHORT:
            return apply_sub_setting_for_type<unsigned short>(dims, rank, dim_id, data, ndata, data_type, not_operation, start, end, start1, end1, stride, n, new_data);
            break;
        case UDA_TYPE_UNSIGNED_LONG:
            return apply_sub_setting_for_type<unsigned long>(dims, rank, dim_id, data, ndata, data_type, not_operation, start, end, start1, end1, stride, n, new_data);
            break;
        default:
            UDA_LOG(UDA_LOG_ERROR, "Invalid data type for sub-setting operation!\n");
            UDA_LOG(UDA_LOG_ERROR, "Data Type: %d    Rank: %d\n", data_type, rank);
            THROW_ERROR(9999, "Invalid data type for sub-setting operation!");
    }

    return 0;
}



