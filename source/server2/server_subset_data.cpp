//--------------------------------------------------------------------------------------------------------------------
// Serverside Data Subsetting Operations Data
//
// Return Codes:    0 => OK, otherwise Error
//
//--------------------------------------------------------------------------------------------------------------------

#include "server_subset_data.h"

#include <cerrno>
#include <float.h>
#include <math.h>
#if defined(__GNUC__)
#  include <strings.h>
#else
#  define strncasecmp _strnicmp
#endif

#include "clientserver/compressDim.h"
#include "clientserver/errorLog.h"
#include "clientserver/initStructs.h"
#include "clientserver/print_structs.h"
#include "common/stringUtils.h"
#include "logging/logging.h"
#include "uda/structured.h"
#include <uda/types.h>

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
//    reform - reduce the rank by 1 if the dimension length is 1 and the rank > 1

// todo:
//

using namespace uda::client_server;
using namespace uda::logging;
using namespace uda::structures;
using namespace uda::common;

using namespace std::string_literals;

namespace
{

int server_subset_indices(char* operation, Dims* dim, double value, unsigned int* subset_indices);

int server_new_data_array2(Dims* dims, int rank, int dim_id, char* data, int n_data, int data_type, bool reverse,
                           int start, int end, int stride, int* n, void** new_data);

} // namespace

int uda::server::server_subset_data(client_server::DataBlock* data_block, client_server::Action action,
                                    structures::LogMallocList *log_malloc_list)
{
    Dims* dim;
    Dims new_dim;
    Subset subset;
    char* operation;

    print_action(action);
    print_data_block(*data_block);

    //-----------------------------------------------------------------------------------------------------------------------
    // How many sets of sub-setting operations?

    int n_subsets = 0;

    if (action.actionType == (int)ActionType::Composite) { // XML Based subsetting
        if (action.composite.nsubsets == 0) {
            return 0; // Nothing to Subset
        }
        n_subsets = action.composite.nsubsets;
    } else {
        if (action.actionType == (int)ActionType::ServerSide) { // Client Requested subsetting
            if (action.serverside.nsubsets == 0) {
                return 0; // Nothing to Subset
            }
            n_subsets = action.serverside.nsubsets;
        } else {
            if (action.actionType == (int)ActionType::Subset) { // Client Requested subsetting
                n_subsets = 1;
            } else {
                return 0;
            }
        }
    }

    //-----------------------------------------------------------------------------------------------------------------------
    // Check Rank

    if (data_block->rank > 2 &&
        !(action.actionType == (int)ActionType::Subset && !strncasecmp(action.subset.function, "rotateRZ", 8))) {
        UDA_THROW_ERROR(9999, "Not Configured to Subset Data with Rank Higher than 2");
    }

    // Check for special case of rank 0 data indexed by [0]

    if (data_block->rank == 0 && n_subsets == 1 && action.actionType == (int)ActionType::Subset
            && action.subset.nbound == 1 && action.subset.operation[0][0] == ':'
            && action.subset.lbindex[0].init && action.subset.lbindex[0].value == 0
            && action.subset.ubindex[0].init && action.subset.ubindex[0].value == 1) {
        return 0;
    }

    //-----------------------------------------------------------------------------------------------------------------------
    // Process all sets of sub-setting operations

    int n_bound = 0;

    for (int i = 0; i < n_subsets; i++) { // the number of sets of Subset Operations

        if (action.actionType == (int)ActionType::Composite) {
            subset = action.composite.subsets[i]; // the set of Subset Operations
        } else {
            if (action.actionType == (int)ActionType::ServerSide) {
                subset = action.serverside.subsets[i];
            } else {
                if (action.actionType == (int)ActionType::Subset) {
                    subset = action.subset;
                }
            }
        }

        n_bound = subset.nbound; // the Number of operations in the set

        for (int j = 0; j < n_bound; j++) { // Process each operation separately

            double value = subset.bound[j];
            operation = subset.operation[j]; // a single operation
            int dim_id = subset.dimid[j];         // applied to this dimension (if -1 then to data only!)

            UDA_LOG(UDA_LOG_DEBUG, "[{}][{}]Value = {}, Operation = {}, DIM id = {}, Reform = {}", i, j, value,
                    operation, dim_id, subset.reform);

            if (dim_id < 0 || dim_id >= (int)data_block->rank) {
                UDA_LOG(UDA_LOG_ERROR, "DIM id = {}, Rank = {}, Test = {} ", dim_id, data_block->rank,
                        dim_id >= (int)data_block->rank);
                print_data_block(*data_block);
                UDA_THROW_ERROR(9999, "Data Sub-setting is Impossible as the subset Dimension is not Compatible with "
                                      "the Rank of the Signal");
                return -1;
            }

            //----------------------------------------------------------------------------------------------------------------------------
            // Operations on Simple Data Structures: target must be an Atomic Type, Scalar, Name is Case Sensitive
            //
            // (mapType=1) Array of Structures - member = single scalar value: rank and shape = rank and shape of
            // structure array (mapType=2) Single Structure - member = array of values: rank and shape = increase rank
            // and shape of structure member by 1 Array of Structures - member = array of values: Not allowed.
            //
            // (mapType=3) structure[14].array[100] -> newarray[14][100]:

            if (data_block->opaque_type == UDA_OPAQUE_TYPE_STRUCTURES && subset.member[0] != '\0' &&
                data_block->opaque_block != nullptr) {

                int data_n;
                int count = 0;
                int rank = 0;
                int size;
                int type;
                int mapType = 0;
                int* shape;
                const char* type_name;
                char* extract = nullptr;
                auto udt = (UserDefinedType*)data_block->opaque_block;

                // Extract an atomic type data element from the data structure

                for (i = 0; i < udt->fieldcount; i++) {
                    if (STR_EQUALS(udt->compoundfield[i].name, subset.member)) { // Locate target member by name

                        data_n = data_block->data_n;

                        if (!udt->compoundfield[i].pointer) { // Regular Array of data

                            count = udt->compoundfield[i].count;

                            if (count == 1 && data_n >= 1) {
                                mapType = 1;
                            } else {
                                if (count >= 1 && data_n == 1) {
                                    mapType = 2;
                                } else {
                                    mapType = 3;
                                }
                            }

                            switch (udt->compoundfield[i].atomictype) {
                                case UDA_TYPE_DOUBLE: {
                                    double *data = nullptr, *dp;

                                    if (mapType == 1) {
                                        data = (double*)malloc(data_n * sizeof(double));
                                        for (int k = 0; k < data_n; k++) {
                                            data[k] = *(double*)&data_block
                                                           ->data[k * udt->size + udt->compoundfield[i].offset];
                                        }
                                    } else {
                                        if (mapType == 2) {
                                            data_n = count;
                                            data = (double*)malloc(data_n * sizeof(double));
                                            dp = (double*)&data_block->data[udt->compoundfield[i].offset];
                                            for (int k = 0; k < data_n; k++) {
                                                data[k] = dp[k];
                                            }

                                            // Increase rank and shape by 1: Preserve the original dimensional data
                                            // Replace with index using rank and shape from the member

                                            if ((shape = udt->compoundfield[i].shape) == nullptr &&
                                                udt->compoundfield[i].rank > 1) {
                                                UDA_THROW_ERROR(
                                                    999,
                                                    "The Data Structure member's shape data is missing (rank > 1)");
                                            }

                                        } else { // mapType == 3
                                            int total_n;
                                            total_n = count * data_n;
                                            data = (double*)malloc(total_n * sizeof(double));
                                            int jjj = 0;
                                            for (int jj = 0; jj < data_n; jj++) { // Loop over structures
                                                dp = (double*)&data_block
                                                         ->data[jj * udt->size + udt->compoundfield[i].offset];
                                                for (int k = 0; k < count; k++) {
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

                        } else { // Locate the pointer data's properties:

                            if (data_n > 1) {
                                int count_p = 0;
                                int rank_p = 0;
                                int size_p = 0;
                                int* shape_p = nullptr;
                                const char* type_name_p = nullptr;

                                for (int jj = 0; jj < data_n; jj++) {
                                    // Properties Must be identical for all structure array elements
                                    extract = *(char**)&data_block->data[jj * udt->size + udt->compoundfield[i].offset];
                                    udaFindMalloc2(log_malloc_list, (void*)extract, &count, &size, &type_name, &rank,
                                                   &shape);
                                    if (jj > 0) {
                                        if (count != count_p || size != size_p || rank != rank_p ||
                                            strcmp(type_name, type_name_p) != 0) {
                                            // ERROR
                                        }
                                        if (shape != nullptr) {
                                            for (int k = 0; k < rank; k++) {
                                                if (shape[k] != shape_p[k]) {
                                                    // ERROR
                                                }
                                            }
                                        } else {
                                            if (rank > 1) {
                                                UDA_THROW_ERROR(
                                                    999,
                                                    "The Data Structure member's shape data is missing (rank > 1)");
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
                                udaFindMalloc2(log_malloc_list, (void*)extract, &count, &size, &type_name, &rank, &shape);
                            }

                            if (count == 1 && data_n >= 1) {
                                mapType = 1;
                            } else {
                                if (count >= 1 && data_n == 1) {
                                    mapType = 2;
                                } else {
                                    UDA_THROW_ERROR(999,
                                                    "Unable to subset an array of Data Structures when the target "
                                                    "member is also an array. Functionality has not been implemented!)")
                                }
                            }

                            type = udaGettypeof(type_name);

                            switch (type) {
                                case UDA_TYPE_DOUBLE: {
                                    double *data = nullptr, *dp;
                                    if (mapType == 1) {
                                        data = (double*)malloc(data_n * sizeof(double));
                                        for (int k = 0; k < data_n; k++) {
                                            dp = *(double**)&data_block
                                                      ->data[k * udt->size + udt->compoundfield[i].offset];
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

                                    // default:
                            }
                        }

                        // mapType == 2
                        // Increase rank by 1: Preserve the original dimensional data
                        // Replace with index coordinate using rank and shape from the member

                        if (mapType == 2) {
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

                            data_block->dims = (Dims*)realloc((void*)data_block->dims, rank * sizeof(Dims));

                            for (int k = k0; k < rank; k++) {
                                init_dim_block(&data_block->dims[k]);
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

                                data_block->dims =
                                    (Dims*)realloc((void*)data_block->dims, data_block->rank * sizeof(Dims));

                                for (unsigned int k = k0; k < data_block->rank; k++) {
                                    init_dim_block(&data_block->dims[k]);
                                    data_block->dims[k].dim_n = udt->compoundfield[i].shape[k - k0];
                                    data_block->dims[k].data_type = UDA_TYPE_UNSIGNED_INT;
                                    data_block->dims[k].compressed = 1;
                                    data_block->dims[k].method = 0;
                                    data_block->dims[k].dim0 = 0.0;
                                    data_block->dims[k].diff = 1.0;
                                }
                            }
                        }

                        if (log_malloc_list != nullptr) {
                            udaFreeMallocLogList(log_malloc_list);
                            free(log_malloc_list);
                            log_malloc_list = nullptr;
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

            if (STR_EQUALS(operation, "*")) {
                continue;
            } // This means No subset - Part of an array Reshape Operation

            if (operation[0] == ':' && (subset.lbindex[j].init ? subset.lbindex[j].value : 0) == 0 &&
                (subset.ubindex[j].init ? subset.lbindex[j].value : data_block->dims[dim_id].dim_n) ==
                    data_block->dims[dim_id].dim_n &&
                (subset.stride[j].init ? subset.stride[j].value : 1) == 1) {
                continue; // subset spans the complete dimension
            }

            //----------------------------------------------------------------------------------------------------------------------------
            // Decompress the dimensional data if necessary & free Heap Associated with Compression

            init_dim_block(&new_dim); // Holder for the Subsetted Dimension (part copy of the original)

            dim = &(data_block->dims[dim_id]); // the original dimension to be subset

            if (dim->compressed) {
                uncompress_dim(dim);
                dim->compressed = 0; // Can't preserve this status after the subset has been applied
                dim->method = 0;

                if (dim->sams != nullptr) {
                    free(dim->sams);
                }
                if (dim->offs != nullptr) {
                    free(dim->offs);
                }
                if (dim->ints != nullptr) {
                    free(dim->ints);
                }

                dim->udoms = 0;
                dim->sams = nullptr; // Avoid double freeing of Heap
                dim->offs = nullptr;
                dim->ints = nullptr;
            }

            //----------------------------------------------------------------------------------------------------------------------------
            // Copy all existing Dimension Information to working structure

            new_dim = *dim;
            new_dim.dim_n = 0;
            new_dim.dim = nullptr;

            //----------------------------------------------------------------------------------------------------------------------------
            // Subset Operations: Identify subset indicies

            // Subsetting Indices

            int start = -1; // Starting Index satisfying the operation
            int end = -1;   // Ending Index
            int stride = 1;

            // Test for Array Reshaping Operations

            int reshape = 0; // Sub-setting has been defined using array indexing notation
            bool reverse = false; // Reverse the ordering of elements (also uses array indexing notation)

            if (operation[0] == '*') {
                continue; // This means No dimensional subset - Part of an array Reshape Operation
            }

            int dim_n = 0;

            // Reshape Operation - Index Range Specified
            if (operation[0] == ':') {
                // [start:end:stride]
                start = subset.lbindex[j].init
                        ? (int)subset.lbindex[j].value
                        : 0;
                end = subset.ubindex[j].init
                        ? (int)subset.ubindex[j].value
                        : dim->dim_n;
                stride = subset.stride[j].init
                        ? (int)subset.stride[j].value
                        : 1;

                if (start < 0) {
                    start = dim->dim_n + start;
                }
                if (end < 0) {
                    end = dim->dim_n + end;
                }
                if (stride < 0) {
                    reverse = true;
                    stride = -stride;
                }

                if (start > end) { // Check Ordering (Allow for Reversing?)
                    int start_cpy = start;
                    reverse = true;
                    start = end; // Swap indices
                    end = start_cpy;
                }
                reshape = 1;
                dim_n = end - start + 1;
            }

            if (operation[0] == '#') { // Reshape Operation - Highest array position (last value)
                start = dim->dim_n - 1;
                end = dim->dim_n - 1;
                reshape = 1;
                dim_n = 1;
            }

            // Create an Array of Indices satisfying the criteria

            if (!reshape) {

                auto subset_indices = (unsigned int*)malloc(dim->dim_n * sizeof(unsigned int));

                if (STR_EQUALS(operation, "!<")) {
                    strcpy(operation, ">=");
                }
                if (STR_EQUALS(operation, "!>")) {
                    strcpy(operation, "<=");
                }
                if (STR_EQUALS(operation, "!<=")) {
                    strcpy(operation, ">");
                }
                if (STR_EQUALS(operation, "!>=")) {
                    strcpy(operation, "<");
                }

                if ((dim_n = server_subset_indices(operation, dim, value, subset_indices)) == 0) {
                    free(subset_indices);
                    UDA_THROW_ERROR(9999, "No Data were found that satisfies a subset");
                }

                // Start and End of Subset Ranges

                start = subset_indices[0];
                end = subset_indices[dim_n - 1];

                if (dim_n != end - start + 1) { // Dimension array is Not well ordered!
                    free(subset_indices);
                    UDA_THROW_ERROR(9999, "The Dimensional Array is Not Ordered: Unable to Subset");
                }
                free(subset_indices);
            }

            new_dim.dim_n = dim_n;

            //----------------------------------------------------------------------------------------------------------------------------
            // Build the New Subsetted Dimension

            print_data_block(*data_block);
            UDA_LOG(UDA_LOG_DEBUG, "\n\n\n*** dim->data_type: {}\n\n", dim->data_type);
            UDA_LOG(UDA_LOG_DEBUG, "\n\n\n*** dim->errhi != nullptr: {}\n\n", dim->errhi != nullptr);
            UDA_LOG(UDA_LOG_DEBUG, "\n\n\n*** dim->errlo != nullptr: {}\n\n", dim->errlo != nullptr);

            int ierr = 0;
            int n = 0;

            if ((ierr = server_new_data_array2(dim, 1, dim_id, dim->dim, dim_n, dim->data_type, reverse, start, end,
                                               stride, &n, (void**)&new_dim.dim)) != 0) {
                return ierr;
            }

            if (dim->errhi != nullptr && dim->error_type != UDA_TYPE_UNKNOWN) {
                if ((ierr = server_new_data_array2(dim, 1, dim_id, dim->errhi, dim_n, dim->error_type, reverse, start,
                                                   end, stride, &n, (void**)&new_dim.errhi)) != 0) {
                    return ierr;
                }
            }

            if (dim->errlo != nullptr && dim->error_type != UDA_TYPE_UNKNOWN) {
                if ((ierr = server_new_data_array2(dim, 1, dim_id, dim->errlo, dim_n, dim->error_type, reverse, start,
                                                   end, stride, &n, (void**)&new_dim.errlo)) != 0) {
                    return ierr;
                }
            }

            //-----------------------------------------------------------------------------------------------------------------------
            // Reshape and Save the Subsetted Data

            print_data_block(*data_block);

            char* new_data = nullptr;
            char* new_err_hi = nullptr;
            char* new_err_lo = nullptr;
            int n_data = 0;

            if ((ierr = server_new_data_array2(data_block->dims, data_block->rank, dim_id, data_block->data,
                                            data_block->data_n, data_block->data_type, reverse, start, end, stride, &n_data,
                                            (void**)&new_data)) != 0) {
                return ierr;
            }

            if (data_block->error_type != UDA_TYPE_UNKNOWN && data_block->errhi != nullptr) {
                if ((ierr = server_new_data_array2(data_block->dims, data_block->rank, dim_id, data_block->errhi,
                                                data_block->data_n, data_block->error_type, reverse, start, end, stride, &n,
                                                (void**)&new_err_hi)) != 0) {
                    return ierr;
                }
                free(data_block->errhi);      // Free Original Heap
                data_block->errhi = new_err_hi; // Replace with the Reshaped Array
            }

            if (data_block->error_type != UDA_TYPE_UNKNOWN && dim->errlo != nullptr) {
                if ((ierr = server_new_data_array2(data_block->dims, data_block->rank, dim_id, data_block->errlo,
                                                data_block->data_n, data_block->error_type, reverse,
                                                start, end, stride, &n, (void**)&new_err_lo)) != 0) {
                    return ierr;
                }
                free(data_block->errlo);      // Free Original Heap
                data_block->errlo = new_err_lo; // Replace with the Reshaped Array
            }

            data_block->data_n = n_data;

            free(data_block->data);     // Free Original Heap
            data_block->data = new_data; // Replace with the Reshaped Array

            // replace the Original Dimensional Structure with the New Subsetted Structure unless a
            // REFORM [Rank Reduction] has been requested and the dimension length is 1 (this has no effect on the Data
            // Array items)

            // Free Heap associated with the original Dimensional Structure Array

            free(dim->dim);
            free(dim->errlo);
            free(dim->errhi);

            dim->dim = nullptr;
            dim->errlo = nullptr;
            dim->errhi = nullptr;

            // Save the reshaped Dimension or Reform the whole

            data_block->dims[dim_id] = new_dim; // Replace with the subsetted dimension
        }
    }

    //-------------------------------------------------------------------------------------------------------------
    // Reform the Data if requested

    int ierr = 0;

    if (ierr == 0 && subset.reform) {
        int rank = data_block->rank;
        for (int j = 0; j < rank; j++) {
            if (data_block->dims[j].dim_n <= 1) {
                UDA_LOG(UDA_LOG_DEBUG, "Reforming Dimension {}", j);

                data_block->dims[j].compressed = 0;
                data_block->dims[j].method = 0;

                if (data_block->dims[j].dim != nullptr) {
                    free(data_block->dims[j].dim);
                }
                if (data_block->dims[j].errlo != nullptr) {
                    free(data_block->dims[j].errlo);
                }
                if (data_block->dims[j].errhi != nullptr) {
                    free(data_block->dims[j].errhi);
                }
                if (data_block->dims[j].sams != nullptr) {
                    free(data_block->dims[j].sams);
                }
                if (data_block->dims[j].offs != nullptr) {
                    free(data_block->dims[j].offs);
                }
                if (data_block->dims[j].ints != nullptr) {
                    free(data_block->dims[j].ints);
                }

                for (int k = j + 1; k < rank; k++) {
                    data_block->dims[k - 1] = data_block->dims[k]; // Shift array contents
                }

                if (data_block->order == j) {
                    data_block->order = -1; // No Time Dimension if Reformed
                } else {
                    if (data_block->order > j) {
                        data_block->order = data_block->order - 1;
                    } // Time Dimension ID reduced by 1
                }

                data_block->rank = data_block->rank - 1; // Reduce the Rank
            }
        }
    }

    //-------------------------------------------------------------------------------------------------------------
    // Apply simple functions to the subset data: XML syntax function="name(dim_id=9)"

    if (ierr == 0 && subset.function[0] != '\0') {

        if (!strncasecmp(subset.function, "minimum(", 8)) { // Single scalar result
            int dim_id = 0;
            if (data_block->rank >= 1) {
                char* p1 = strstr(subset.function, "dim_id");
                if (p1 != nullptr) {
                    char *p3, *p2 = strchr(p1, '=');
                    p2[0] = ' ';
                    p3 = strchr(p2, ')');
                    p3[0] = '\0';
                    if (is_number(p2)) {
                        dim_id = atoi(p2);
                    } else {
                        // ERROR
                    }
                } else {
                    // ERROR
                }
            }

            if (dim_id < 0 || dim_id >= (int)data_block->rank) {
                UDA_LOG(UDA_LOG_ERROR, "Function Syntax Error -  dim_id = {},  Rank = {}", dim_id, data_block->rank);
                UDA_THROW_ERROR(
                    999,
                    "The dimension ID identified via the subset function is outside the rank bounds of the array!");
            }

            switch (data_block->data_type) {
                case UDA_TYPE_FLOAT: {
                    auto dp = (float*)data_block->data;
                    float min = dp[0];
                    switch (data_block->rank) {
                        case 0: { // Ignore function dim_id argument
                            for (int j = 1; j < data_block->data_n; j++) {
                                if (dp[j] < min) {
                                    min = dp[j];
                                }
                            }
                            dp[0] = min;
                            dp = (float*)realloc((void*)dp, sizeof(float)); // Reduce array size
                            data_block->data_n = 1;
                            break;
                        }
                        case 1: {
                            new_dim = data_block->dims[0];
                            for (int j = 1; j < data_block->dims[0].dim_n; j++) {
                                if (dp[j] < min) {
                                    min = dp[j];
                                }
                            }
                            dp[0] = min;
                            dp = (float*)realloc((void*)dp, sizeof(float)); // Reduce array size
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
                        case 0: { // Ignore function dim_id argument
                            for (int j = 1; j < data_block->data_n; j++) {
                                if (dp[j] < min) {
                                    min = dp[j];
                                }
                            }
                            dp[0] = min;
                            dp = (double*)realloc((void*)dp, sizeof(double)); // Reduce array size
                            data_block->data_n = 1;
                            break;
                        }
                        case 1: {
                            new_dim = data_block->dims[0];
                            for (int j = 1; j < data_block->dims[0].dim_n; j++) {
                                if (dp[j] < min) {
                                    min = dp[j];
                                }
                            }
                            dp[0] = min;
                            dp = (double*)realloc((void*)dp, sizeof(double)); // Reduce array size
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
        }

        if (STR_IEQUALS(subset.function, "maximum")) { // Single scalar result
        }

        if (!strncasecmp(subset.function, "count", 5)) { // Single scalar result
            char* p1 = strstr(subset.function, "dim_id");
            auto count = (unsigned int*)malloc(sizeof(unsigned int));
            if (p1 == nullptr) {
                count[0] = (unsigned int)data_block->data_n;
                free_data_block(data_block);
                init_data_block(data_block);
                data_block->data_n = 1;
                data_block->data = (char*)count;
                data_block->data_type = UDA_TYPE_UNSIGNED_INT;
            } else {
                int dim_id = 0;
                if (data_block->rank >= 1) {
                    char *p3, *p2 = strchr(p1, '=');
                    p2[0] = ' ';
                    p3 = strchr(p2, ')');
                    p3[0] = '\0';
                    if (is_number(p2)) {
                        dim_id = atoi(p2);
                    } else {
                        // ERROR
                    }
                }
                if (dim_id < (int)data_block->rank) {
                    count[0] = (unsigned int)data_block->dims[dim_id].dim_n; // Preserve this value
                    Dims ddim = data_block->dims[dim_id];
                    if (ddim.dim != nullptr) {
                        free(ddim.dim);
                    }
                    if (ddim.errhi != nullptr) {
                        free(ddim.errhi);
                    }
                    if (ddim.errlo != nullptr) {
                        free(ddim.errlo);
                    }
                    if (ddim.sams != nullptr) {
                        free(ddim.sams);
                    }
                    if (ddim.offs != nullptr) {
                        free(ddim.offs);
                    }
                    if (ddim.ints != nullptr) {
                        free(ddim.ints);
                    }
                } else {
                    // ERROR
                }
                if (data_block->data != nullptr) {
                    free(data_block->data);
                }
                if (data_block->errhi != nullptr) {
                    free(data_block->errhi);
                }
                if (data_block->errlo != nullptr) {
                    free(data_block->errlo);
                }
                data_block->data = nullptr;
                data_block->errhi = nullptr;
                data_block->errlo = nullptr;
                data_block->error_type = UDA_TYPE_UNKNOWN;
                data_block->error_param_n = 0;

                data_block->data_n = 1;
                for (unsigned int j = 0; j < data_block->rank - 1; j++) {
                    if (j >= (unsigned int)dim_id) {
                        data_block->dims[j] = data_block->dims[j + 1]; // skip over the target
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
                snprintf(data_block->data_label, StringLength, "count(dim_id=%d)", dim_id);
            }
        }

        if (!strncasecmp(subset.function, "abs()", 5)) { // Absolute value
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
        }

        if (!strncasecmp(subset.function, "const", 5)) { // Constant value substitution
            double value = 0.0;                          // Zero data default
            char* p1 = strstr(subset.function, "value");
            strcpy(data_block->data_label, subset.function);

            UDA_LOG(UDA_LOG_DEBUG, "{}", subset.function);

            if (p1 != nullptr) {
                char *p3, *p2 = strchr(&p1[5], '=');
                p2[0] = ' ';
                p3 = strchr(p2, ')');
                p3[0] = '\0';
                trim_string(p2);
                left_trim_string(p2);
                UDA_LOG(UDA_LOG_DEBUG, "p2 = [{}]", p2);
                if (is_float(p2)) {
                    value = atof(p2);
                } else {
                    UDA_LOG(UDA_LOG_DEBUG, "IsFloat FALSE!");
                    // ERROR
                }
            }

            UDA_LOG(UDA_LOG_DEBUG, "value = {}", value);

            if (data_block->errhi != nullptr) {
                free(data_block->errhi);
            }
            if (data_block->errlo != nullptr) {
                free(data_block->errlo);
            }
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
        }

        if (!strncasecmp(subset.function, "order", 5)) { // Identify the Time dimension order
            char* p1 = strstr(subset.function, "dim_id");
            UDA_LOG(UDA_LOG_DEBUG, "{}", subset.function);
            if (p1 != nullptr) {
                char *p3, *p2 = strchr(&p1[5], '=');
                p2[0] = ' ';
                p3 = strchr(p2, ')');
                p3[0] = '\0';
                trim_string(p2);
                left_trim_string(p2);
                UDA_LOG(UDA_LOG_DEBUG, "p2 = [{}]", p2);
                if (is_number(p2)) {
                    data_block->order = (int)atof(p2);
                } else {
                    // ERROR
                }
            }
            UDA_LOG(UDA_LOG_DEBUG, "order = {}", data_block->order);
        }

        if (!strncasecmp(subset.function, "rotateRZ", 8)) { // Rotate R,Z coordinates in rank 3 array
            UDA_LOG(UDA_LOG_DEBUG, "{}", subset.function);
            if (data_block->rank != 3) {
                UDA_THROW_ERROR(999, "The function rotateRZ only operates on rank 3 arrays");
            }
            int order = data_block->order;
            if (order < 0) {
                UDA_THROW_ERROR(999, "The function rotateRZ expects a Time coordinate");
            }
            int type = data_block->data_type;
            if (type != UDA_TYPE_DOUBLE) {
                UDA_THROW_ERROR(999, "The function rotateRZ is configured for type DOUBLE only");
            }
            int nt, nr, nz, count;
            count = data_block->data_n;
            auto newData = (double*)malloc(count * sizeof(double));
            unsigned int offset = 0;
            auto old = (double*)data_block->data;
            if (order == 0) { // array[nz][nr][nt] -> [nr][nz][nt]
                nt = data_block->dims[0].dim_n;
                nr = data_block->dims[1].dim_n;
                nz = data_block->dims[2].dim_n;

                auto data = (double***)malloc(nz * sizeof(double**));
                for (int j = 0; j < nz; j++) {
                    data[j] = (double**)malloc(nr * sizeof(double*));
                    for (int i = 0; i < nr; i++) {
                        data[j][i] = (double*)malloc(nt * sizeof(double));
                        for (int k = 0; k < nt; k++) {
                            data[j][i][k] = old[offset++];
                        }
                    }
                }
                offset = 0;
                for (int i = 0; i < nr; i++) {
                    for (int j = 0; j < nz; j++) {
                        for (int k = 0; k < nt; k++) {
                            newData[offset++] = data[j][i][k];
                        }
                    }
                }
                for (int j = 0; j < nz; j++) {
                    for (int i = 0; i < nr; i++) {
                        free(data[j][i]);
                    }
                    free(data[j]);
                }
                free(data);

                Dims d1 = data_block->dims[1];
                Dims d2 = data_block->dims[2];
                data_block->dims[1] = d2;
                data_block->dims[2] = d1;
            } else if (order == 1) { // array[nz][nt][nr]
                UDA_THROW_ERROR(
                    999, "The function rotateRZ only operates on arrays with shape [nz][nr][nt] or [nt][nz][nr]");
            } else if (order == 2) { // array[nt][nz][nr] -> [nt][nr][nz]
                nr = data_block->dims[0].dim_n;
                nz = data_block->dims[1].dim_n;
                nt = data_block->dims[2].dim_n;

                auto data = (double***)malloc(nt * sizeof(double**));
                for (int k = 0; k < nt; k++) {
                    data[k] = (double**)malloc(nz * sizeof(double*));
                    for (int j = 0; j < nz; j++) {
                        data[k][j] = (double*)malloc(nr * sizeof(double));
                        for (int i = 0; i < nr; i++) {
                            data[k][j][i] = old[offset++];
                        }
                    }
                }
                offset = 0;
                for (int k = 0; k < nt; k++) {
                    for (int i = 0; i < nr; i++) {
                        for (int j = 0; j < nz; j++) {
                            newData[offset++] = data[k][j][i];
                        }
                    }
                }
                for (int k = 0; k < nt; k++) {
                    for (int j = 0; j < nz; j++) {
                        free(data[k][j]);
                    }
                    free(data[k]);
                }
                free(data);
                Dims d0 = data_block->dims[0];
                Dims d1 = data_block->dims[1];
                data_block->dims[0] = d1;
                data_block->dims[1] = d0;
            } else {
                UDA_THROW_ERROR(999, "rotateRZ: Incorrect ORDER value found!");
            }
            free(data_block->data);
            data_block->data = (char*)newData;
        }
    }

    //-------------------------------------------------------------------------------------------------------------
    // Explicitly set the order of the time dimension if not possible via the other options

    if (subset.order >= 0) {
        data_block->order = subset.order;
    }

    return ierr;
}

//-------------------------------------------------------------------------------------------------------------
//-------------------------------------------------------------------------------------------------------------
// Build an Action Structure for Serverside Data Operations

// SS::Subset(\"xx\", [!=0.15])
// SS::Subset(\"xx\", [0:25])
// SS::Subset(\"xx\", [=0.15, *], reform)
// SS::Subset(\"xx\", [0:5, *])
// SS::Subset(\"xx\", [0:0, *], reform)
// SS::Subset(\"xx\", [*, 2:8])
// SS::Subset(\"xx\", [*, 2:8], member=\"name\")
// SS::Subset(\"xx\", [*, 3], member=\"name\", reform)
// SS::Subset(\"xx\", [*, 3], member=\"name\", reform, function=\"minimum(dim_id=0)\" )

int uda::server::server_parse_server_side(config::Config& config, client_server::RequestData* request_block, client_server::Actions* actions_serverside)
{

    char qchar[2];
    char *p = nullptr, *t1 = nullptr, *t2 = nullptr;
    char api_delim[3] =
        "::"; // ********** TO DO: This should be an Environment Variable compatible with the Client delimiter
    char archive[StringLength] = "";
    char signal[StringLength] = "";
    char options[StringLength] = "";
    char operation[StringLength];
    char opcopy[StringLength];
    char* endp = nullptr;

    int nactions, n_subsets, n_bound, ierr, lop;

    Action* action = nullptr;
    Subset* subsets = nullptr;

    ierr = 0;

    //-------------------------------------------------------------------------------------------------------------
    // Extract the ARCHIVE::Signal element
    // use the first character after the left parenthesis as the opening quotation character

    strncpy(qchar, request_block->signal + 7, 1);
    qchar[1] = '\0';

    if ((p = strstr(request_block->signal + 8, qchar)) == nullptr) {
        // Locate the terminating quotation character
        UDA_THROW_ERROR(9999, "Syntax Error: The Signal Name has no Terminating Quotation character!");
    }

    size_t lsignal = (size_t)(p - request_block->signal) - 8; // Signal name Length
    if (lsignal >= StringLength) {
        lsignal = StringLength - 1;
    }
    strncpy(signal, request_block->signal + 8, lsignal);
    signal[lsignal] = '\0';

    // Check if the data_object(signal) is of the form: ARCHIVE::Signal

    if ((t1 = strstr(signal, api_delim)) != nullptr) {
        strncpy(archive, signal, t1 - signal);
        archive[t1 - signal] = '\0';
        strcpy(signal, t1 + strlen(api_delim));
    }

    //-------------------------------------------------------------------------------------------------------------
    // Extract the Subset Operation

    if ((t1 = strstr(p + 1, ",")) == nullptr) {
        // Locate the separation character
        UDA_THROW_ERROR(9999, "Syntax Error: No Comma after the Signal Name");
    }

    if ((t1 = strstr(t1 + 1, "[")) == nullptr) {
        // Locate the Operation
        UDA_THROW_ERROR(9999, "Syntax Error: No [ enclosing the Operation");
    }

    if ((t2 = strstr(t1 + 1, "]")) == nullptr) {
        UDA_THROW_ERROR(9999, "Syntax Error: No ] enclosing the Operation ");
    }

    strncpy(operation, t1 + 1, t2 - t1 - 1); // The Requested Operation including Values
    operation[t2 - t1 - 1] = '\0';

    //-------------------------------------------------------------------------------------------------------------
    // remaining options, e.g., REFORM

    if ((t1 = strstr(t2 + 1, ",")) != nullptr) {
        strcpy(options, t1 + 1);
    }

    //-------------------------------------------------------------------------------------------------------------
    // Overwrite the Request Block to enable the correct access to signal data before the subset operations are applied

    auto default_archive = config.get("request.default_archive").as_or_default(""s);

    strcpy(request_block->archive, archive);
    if (request_block->archive[0] == '\0') {
        strcpy(request_block->archive, default_archive.c_str());
    }

    strcpy(request_block->signal, signal);

    //-------------------------------------------------------------------------------------------------------------
    // Extend the Action Structure and Initialise

    nactions = actions_serverside->nactions + 1;
    if ((action = (Action*)realloc((void*)actions_serverside->action, nactions * sizeof(Action))) == nullptr) {
        UDA_THROW_ERROR(9999, "Unable to Allocate Heap memory");
    }

    init_action(&action[nactions - 1]);

    action[nactions - 1].actionType = (int)ActionType::ServerSide;
    action[nactions - 1].inRange = 1;
    action[nactions - 1].actionId = nactions;

    init_server_side(&action[nactions - 1].serverside);

    n_subsets = 1;
    if ((subsets = (Subset*)malloc(sizeof(Subset))) == nullptr) {
        UDA_THROW_ERROR(9999, "Unable to Allocate Heap memory");
    }

    for (int i = 0; i < n_subsets; i++) {
        init_subset(&subsets[i]);
    }

    action[nactions - 1].serverside.nsubsets = 1;
    strcpy(subsets[n_subsets - 1].data_signal, request_block->signal);

    // Seek specific options

    if ((p = strstr(options, "reform")) != nullptr) { // Reduce rank
        subsets[n_subsets - 1].reform = 1;
    }

    if ((p = strstr(options, "member=")) != nullptr) { // Extract a Structure member
        strcpy(subsets[n_subsets - 1].member, &p[7]);
        left_trim_string(subsets[n_subsets - 1].member);
        if (subsets[n_subsets - 1].member[0] == '"') {
            subsets[n_subsets - 1].member[0] = ' ';
            left_trim_string(subsets[n_subsets - 1].member);
        }
        if ((p = strchr(subsets[n_subsets - 1].member, '"')) != nullptr) {
            p[0] = '\0';
        }
        if ((p = strchr(subsets[n_subsets - 1].member, ',')) != nullptr) {
            p[0] = '\0';
        }
    }

    // Simple functions

    if ((p = strstr(options, "function=")) != nullptr) { // Identify a function
        strcpy(subsets[n_subsets - 1].function, &p[9]);
        left_trim_string(subsets[n_subsets - 1].function);
        if (subsets[n_subsets - 1].function[0] == '"') {
            subsets[n_subsets - 1].function[0] = ' ';
            left_trim_string(subsets[n_subsets - 1].function);
        }
        if ((p = strchr(subsets[n_subsets - 1].function, '"')) != nullptr) {
            p[0] = '\0';
        }
        if ((p = strchr(subsets[n_subsets - 1].function, ',')) != nullptr) {
            p[0] = '\0';
        }
    }

    //-------------------------------------------------------------------------------------------------------------
    // Parse the Operation String for Value and Operation

    left_trim_string(trim_string(operation)); // Remove Leading white space
    strcpy(opcopy, operation);
    n_bound = 0;

    if ((p = strtok(opcopy, ",")) != nullptr) {       // Tokenise into Individual Operations on each Dimension
        subsets[n_subsets - 1].dimid[n_bound] = n_bound; // Identify the Dimension to apply the operation on
        n_bound++;
        if (strlen(p) < SxmlMaxString) {
            strcpy(subsets[n_subsets - 1].operation[n_bound - 1], p);
            mid_trim_string(subsets[n_subsets - 1].operation[n_bound - 1]); // Remove internal white space
        } else {
            free(subsets);
            UDA_THROW_ERROR(9999, "Syntax Error: The Signal Operation String is too long");
        }

        while ((p = strtok(nullptr, ",")) != nullptr) {
            subsets[n_subsets - 1].dimid[n_bound] = n_bound;
            n_bound++;
            if (n_bound > MaxDataRank) {
                free(subsets);
                UDA_THROW_ERROR(9999, "The number of Dimensional Operations exceeds the Internal Limit");
            }
            if (strlen(p) < SxmlMaxString) {
                strcpy(subsets[n_subsets - 1].operation[n_bound - 1], p);
                mid_trim_string(subsets[n_subsets - 1].operation[n_bound - 1]); // Remove white space
            } else {
                free(subsets);
                UDA_THROW_ERROR(9999, "Syntax Error: The Signal Operation String is too long");
            }
        }
    }

    subsets[n_subsets - 1].nbound = n_bound;

    //-------------------------------------------------------------------------------------------------------------
    // Extract the Value Component from each separate Operation
    // =0.15,!=0.15,<=0.05,>=0.05,!<=0.05,!>=0.05,<0.05,>0.05,0:25,25:0,25,*,25:,:25
    //
    // Identify Three Types of Operations:
    //    A) Contains the characters: =,>, <, !, ~
    //    B) : or Integer Value
    //    C) * or #
    //

    for (int i = 0; i < n_bound; i++) {

        strcpy(opcopy, subsets[n_subsets - 1].operation[i]);

        if ((p = strstr(opcopy, ":")) != nullptr) { // Integer Type Array Index Bounds
            t2 = p + 1;
            opcopy[p - opcopy] = '\0'; // Split the Operation String into two components
            t1 = opcopy;

            subsets[n_subsets - 1].isindex[i] = true;
            subsets[n_subsets - 1].ubindex[i] = {.init = true, .value = -1};
            subsets[n_subsets - 1].lbindex[i] = {.init = true, .value = -1};

            if (t1[0] == '#') {
                subsets[n_subsets - 1].lbindex[i] = {.init = true, .value = -1};
            } // Reverse the data as # => Final array value

            if (strlen(t1) > 0 && t1[0] != '*' && t1[0] != '#') {
                if (is_number(t1)) {
                    // the Lower Index Value of the Bound
                    subsets[n_subsets - 1].lbindex[i] = {.init = true, .value = strtol(t1, &endp, 0)};
                    if (*endp != '\0' || errno == EINVAL || errno == ERANGE) {
                        free(subsets);
                        UDA_THROW_ERROR(9999, "Server Side Operation Syntax Error: Lower Index Bound");
                    }
                } else {
                    free(subsets);
                    UDA_THROW_ERROR(9999, "Server Side Operation Syntax Error: Lower Index Bound");
                }
            }
            if (strlen(t2) > 0 && t2[0] != '*' && t2[0] != '#') {
                if (is_number(t2)) {
                    // the Upper Index Value of the Bound
                    subsets[n_subsets - 1].ubindex[i] = {.init = true, .value = strtol(t2, &endp, 0)};
                    if (*endp != '\0' || errno == EINVAL || errno == ERANGE) {
                        free(subsets);
                        UDA_THROW_ERROR(9999, "Server Side Operation Syntax Error: Upper Index Bound");
                    }
                } else {
                    free(subsets);
                    UDA_THROW_ERROR(9999, "Server Side Operation Syntax Error: Upper Index Bound");
                }
            }
            strcpy(subsets[n_subsets - 1].operation[i], ":"); // Define Simple Operation
            continue;
        }

        if ((p = strstr(opcopy, "*")) != nullptr) { // Ignore this Dimension
            subsets[n_subsets - 1].isindex[i] = true;
            subsets[n_subsets - 1].ubindex[i] = {.init = true, .value = -1};
            subsets[n_subsets - 1].lbindex[i] = {.init = true, .value = -1};
            strcpy(subsets[n_subsets - 1].operation[i], "*"); // Define Simple Operation
            continue;
        }

        if ((p = strstr(opcopy, "#")) != nullptr) { // Last Value in Dimension
            subsets[n_subsets - 1].isindex[i] = true;
            subsets[n_subsets - 1].ubindex[i] = {.init = true, .value = -1};
            subsets[n_subsets - 1].lbindex[i] = {.init = true, .value = -1};
            strcpy(subsets[n_subsets - 1].operation[i], "#"); // Define Simple Operation
            continue;
        }

        if (is_number(opcopy)) { // Single Index value
            subsets[n_subsets - 1].isindex[i] = true;
            // the Index Value of the Bound
            subsets[n_subsets - 1].ubindex[i] = {.init = true, .value = strtol(opcopy, &endp, 0)};
            if (*endp != '\0' || errno == EINVAL || errno == ERANGE) {
                free(subsets);
                UDA_THROW_ERROR(9999, "Server Side Operation Syntax Error: Single Index Bound");
            }
            subsets[n_subsets - 1].lbindex[i] = subsets[n_subsets - 1].ubindex[i];
            strcpy(subsets[n_subsets - 1].operation[i], ":"); // Define Simple Operation
            continue;
        }

        // Single value Operation

        p = nullptr; // Locate the Start of the Numerical Substring
        lop = (int)strlen(subsets[n_subsets - 1].operation[i]);
        for (int j = 0; j < lop; j++) {
            if (subsets[n_subsets - 1].operation[i][j] >= '0' && subsets[n_subsets - 1].operation[i][j] <= '9') {
                p = &subsets[n_subsets - 1].operation[i][j];
                if (j > 0) { // Capture sign
                    if (subsets[n_subsets - 1].operation[i][j - 1] == '+' ||
                        subsets[n_subsets - 1].operation[i][j - 1] == '-') {
                        p = &subsets[n_subsets - 1].operation[i][j - 1];
                    }
                } else {
                    free(subsets);
                    UDA_THROW_ERROR(9999, "Server Side Operation Syntax Error: No Operator Defined!");
                }
                break;
            }
        }

        if (p == nullptr) {
            free(subsets);
            UDA_THROW_ERROR(9999, "Server Side Operation Syntax Error: No Numerical Bound");
        }

        subsets[n_subsets - 1].bound[i] = strtod(p, &endp); // the Value of the Bound

        if (*endp != '\0' || errno == EINVAL || errno == ERANGE) {
            free(subsets);
            UDA_THROW_ERROR(9999, "Server Side Operation Syntax Error");
        }

        // Isolate the Operation only
        subsets[n_subsets - 1].operation[i][p - &subsets[n_subsets - 1].operation[i][0]] = '\0';
    }

    action[nactions - 1].serverside.nsubsets = n_subsets;
    action[nactions - 1].serverside.subsets = subsets;

    actions_serverside->action = action;
    actions_serverside->nactions = nactions;

    return ierr;
}

//----------------------------------------------------------------------------------------------------------------------
// Identify the Index Range satisfying a small set of conditional operators

namespace
{

int server_subset_indices(char* operation, Dims* dim, double value, unsigned int* subset_indices)
{
    int count = 0;

    // Scan the Array applying the Operation

    switch (dim->data_type) {

        case UDA_TYPE_DOUBLE: {
            auto p = (double*)dim->dim;
            if (STR_IEQUALS(operation, "eq") || operation[0] == '=' || STR_EQUALS(operation, "~=")) {
                for (int k = 0; k < dim->dim_n; k++) {
                    if (p[k] == (double)value) {
                        subset_indices[count++] = k;
                    }
                }
                if (count == 0 && STR_EQUALS(operation, "~=")) {
                    for (int k = 0; k < dim->dim_n; k++) {
                        if (fabs(p[k] - (double)value) <= DBL_EPSILON) {
                            subset_indices[count++] = k;
                        }
                    }
                    if (count == 0) {
                        int index = -1;
                        double delta, minvalue = fabs((double)value - p[0]);
                        for (int k = 0; k < dim->dim_n; k++) {
                            delta = fabs((double)value - p[k]);
                            if (delta < minvalue) { // Look for the Single Nearest Value
                                minvalue = delta;
                                index = k;
                            }
                        }
                        if (index >= 0) {
                            count = 1;
                            subset_indices[0] = index;

                            if (index == 0 || index == dim->dim_n - 1) { // Check not an end point by default
                                if (dim->dim_n > 1) {
                                    if (index == 0) {
                                        delta = fabs(p[1] - p[0]);
                                    } else {
                                        delta = fabs(p[dim->dim_n - 1] - p[dim->dim_n - 2]);
                                    }
                                    if (fabs((double)value - p[index]) > delta) {
                                        count = 0; // Suspect match!
                                    }
                                }
                            }
                        }
                    }
                }
            } else {
                if (STR_IEQUALS(operation, "lt") || STR_EQUALS(operation, "<")) {
                    for (int k = 0; k < dim->dim_n; k++) {
                        if (p[k] < (double)value) {
                            subset_indices[count++] = k;
                        }
                    }
                } else {
                    if (STR_IEQUALS(operation, "gt") || STR_EQUALS(operation, ">")) {
                        for (int k = 0; k < dim->dim_n; k++) {
                            if (p[k] > (double)value) {
                                subset_indices[count++] = k;
                            }
                        }
                    } else {
                        if (STR_IEQUALS(operation, "le") || STR_EQUALS(operation, "<=")) {
                            for (int k = 0; k < dim->dim_n; k++) {
                                if (p[k] <= (double)value) {
                                    subset_indices[count++] = k;
                                }
                            }
                        } else {
                            if (STR_IEQUALS(operation, "ge") || STR_EQUALS(operation, ">=")) {
                                for (int k = 0; k < dim->dim_n; k++) {
                                    if (p[k] >= (double)value) {
                                        subset_indices[count++] = k;
                                    }
                                }
                            } else {
                                if (STR_IEQUALS(operation, "ne") || STR_EQUALS(operation, "!=") ||
                                    STR_EQUALS(operation, "!~=")) {
                                    if (strncmp(operation, "!~=", 3) != 0) {
                                        for (int k = 0; k < dim->dim_n; k++) {
                                            if (p[k] != (double)value) {
                                                subset_indices[count++] = k;
                                            }
                                        }
                                    } else {
                                        int index = -1;
                                        double delta, minvalue = fabs((double)value - p[0]);
                                        for (int k = 0; k < dim->dim_n; k++) {
                                            delta = fabs((double)value - p[k]);
                                            if (delta < minvalue) { // Look for the Single Nearest Value
                                                minvalue = delta;
                                                index = k;
                                            }
                                        }
                                        if (index >= 0) {
                                            for (int k = 0; k < dim->dim_n; k++) {
                                                if (k != index) {
                                                    subset_indices[count++] = k;
                                                } // Drop the single nearest value
                                            }
                                            if (index == 0 ||
                                                index == dim->dim_n - 1) { // Check not an end point by default
                                                if (dim->dim_n > 1) {
                                                    if (index == 0) {
                                                        delta = fabs(p[1] - p[0]);
                                                    } else {
                                                        delta = fabs(p[dim->dim_n - 1] - p[dim->dim_n - 2]);
                                                    }
                                                    if (fabs((double)value - p[index]) > delta) {
                                                        count = 0;
                                                    } // Suspect match!
                                                }
                                            }
                                        }
                                    }
                                }
                            }
                        }
                    }
                }
            }
            break;
        }

        case UDA_TYPE_FLOAT: {
            auto p = (float*)dim->dim;
            if (STR_IEQUALS(operation, "eq") || operation[0] == '=' || STR_EQUALS(operation, "~=")) {
                for (int k = 0; k < dim->dim_n; k++) {
                    if (p[k] == (float)value) {
                        subset_indices[count++] = k;
                    }
                }
                if (count == 0 && STR_EQUALS(operation, "~=")) {
                    for (int k = 0; k < dim->dim_n; k++) {
                        if (fabsf(p[k] - (float)value) <= FLT_EPSILON) {
                            subset_indices[count++] = k;
                        }
                    }
                    if (count == 0) {
                        int index = -1;
                        double delta, minvalue = fabsf((float)value - p[0]);
                        for (int k = 0; k < dim->dim_n; k++) {
                            delta = fabsf((float)value - p[k]);
                            if (delta < minvalue) { // Look for the Single Nearest Value
                                minvalue = delta;
                                index = k;
                            }
                        }
                        if (index >= 0) {
                            count = 1;
                            subset_indices[0] = index;

                            if (index == 0 || index == dim->dim_n - 1) { // Check not an end point by default
                                if (dim->dim_n > 1) {
                                    if (index == 0) {
                                        delta = fabsf(p[1] - p[0]);
                                    } else {
                                        delta = fabsf(p[dim->dim_n - 1] - p[dim->dim_n - 2]);
                                    }
                                    if (fabsf((float)value - p[index]) > delta) {
                                        count = 0; // Suspect match!
                                    }
                                }
                            }
                        }
                    }
                }
            } else {
                if (STR_IEQUALS(operation, "lt") || STR_EQUALS(operation, "<")) {
                    for (int k = 0; k < dim->dim_n; k++) {
                        if (p[k] < (float)value) {
                            subset_indices[count++] = k;
                        }
                    }
                } else {
                    if (STR_IEQUALS(operation, "gt") || STR_EQUALS(operation, ">")) {
                        for (int k = 0; k < dim->dim_n; k++) {
                            if (p[k] > (float)value) {
                                subset_indices[count++] = k;
                            }
                        }
                    } else {
                        if (STR_IEQUALS(operation, "le") || STR_EQUALS(operation, "<=")) {
                            for (int k = 0; k < dim->dim_n; k++) {
                                if (p[k] <= (float)value) {
                                    subset_indices[count++] = k;
                                }
                            }
                        } else {
                            if (STR_IEQUALS(operation, "ge") || STR_EQUALS(operation, ">=")) {
                                for (int k = 0; k < dim->dim_n; k++) {
                                    if (p[k] >= (float)value) {
                                        subset_indices[count++] = k;
                                    }
                                }
                            } else {
                                if (STR_IEQUALS(operation, "ne") || STR_EQUALS(operation, "!=") ||
                                    STR_EQUALS(operation, "!~=")) {
                                    if (strncmp(operation, "!~=", 3) != 0) {
                                        for (int k = 0; k < dim->dim_n; k++) {
                                            if (p[k] != (float)value) {
                                                subset_indices[count++] = k;
                                            }
                                        }
                                    } else {
                                        int index = -1;
                                        double delta, minvalue = fabsf((float)value - p[0]);
                                        for (int k = 0; k < dim->dim_n; k++) {
                                            delta = fabsf((float)value - p[k]);
                                            if (delta < minvalue) { // Look for the Single Nearest Value
                                                minvalue = delta;
                                                index = k;
                                            }
                                        }
                                        if (index >= 0) {
                                            for (int k = 0; k < dim->dim_n; k++) {
                                                if (k != index) {
                                                    subset_indices[count++] = k;
                                                } // Drop the single nearest value
                                            }
                                            if (index == 0 ||
                                                index == dim->dim_n - 1) { // Check not an end point by default
                                                if (dim->dim_n > 1) {
                                                    if (index == 0) {
                                                        delta = fabsf(p[1] - p[0]);
                                                    } else {
                                                        delta = fabsf(p[dim->dim_n - 1] - p[dim->dim_n - 2]);
                                                    }
                                                    if (fabsf((float)value - p[index]) > delta) {
                                                        count = 0;
                                                    } // Suspect match!
                                                }
                                            }
                                        }
                                    }
                                }
                            }
                        }
                    }
                }
            }
            break;
        }

        case UDA_TYPE_INT: {
            int* p = (int*)dim->dim;
            if (STR_IEQUALS(operation, "eq") || operation[0] == '=' || STR_EQUALS(operation, "~=")) {
                for (int k = 0; k < dim->dim_n; k++) {
                    if (p[k] == (int)value) {
                        subset_indices[count++] = k;
                    }
                }
            } else {
                if (STR_IEQUALS(operation, "lt") || STR_EQUALS(operation, "<")) {
                    for (int k = 0; k < dim->dim_n; k++) {
                        if (p[k] < (int)value) {
                            subset_indices[count++] = k;
                        }
                    }
                } else {
                    if (STR_IEQUALS(operation, "gt") || STR_EQUALS(operation, ">")) {
                        for (int k = 0; k < dim->dim_n; k++) {
                            if (p[k] > (int)value) {
                                subset_indices[count++] = k;
                            }
                        }
                    } else {
                        if (STR_IEQUALS(operation, "le") || STR_EQUALS(operation, "<=")) {
                            for (int k = 0; k < dim->dim_n; k++) {
                                if (p[k] <= (int)value) {
                                    subset_indices[count++] = k;
                                }
                            }
                        } else {
                            if (STR_IEQUALS(operation, "ge") || STR_EQUALS(operation, ">=")) {
                                for (int k = 0; k < dim->dim_n; k++) {
                                    if (p[k] >= (int)value) {
                                        subset_indices[count++] = k;
                                    }
                                }
                            } else {
                                if (STR_IEQUALS(operation, "ne") || STR_EQUALS(operation, "!=") ||
                                    STR_EQUALS(operation, "!~=")) {
                                    for (int k = 0; k < dim->dim_n; k++) {
                                        if (p[k] != (int)value) {
                                            subset_indices[count++] = k;
                                        }
                                    }
                                }
                            }
                        }
                    }
                }
            }
            break;
        }
    }

    return count;
}

template <typename T>
int apply_subsetting(Dims* dims, int rank, int dim_id, char* data, int n_data, int data_type, bool reverse, int start,
                     int end, int stride, int* n, void** new_data) {

    T *p, *dp;
    T **pa, **dpa;

    // Allocate heap for the reshaped array

    if ((p = (T*)malloc(n_data * sizeof(T))) == nullptr) {
        UDA_THROW_ERROR(9999, "Unable to Allocate Heap memory");
    }

    dp = (T*)data; // the Originating Data Array

    // Reshape
    
    int rows = 0;
    int columns = 0;
    int new_rows = 0;
    int new_cols = 0;
    int count = 0;

    switch (rank) {
        case 1: {
            int k = 0;
            if (!reverse) {
                for (int i = start; i < end; i += stride) {
                    p[k++] = dp[i];
                }
            } else {
                for (int i = end - 1; i >= start; i -= stride) {
                    p[k++] = dp[i];
                }
            }
            *n = k;
            *new_data = (void*)p;
            break;
        }

        case 2:

            // Original Data

            rows = dims[1].dim_n;
            columns = dims[0].dim_n;
            dpa = (T**)malloc(rows * sizeof(T*));
            for (int j = 0; j < rows; j++) {
                dpa[j] = &dp[j * columns];
            }

            // Array for Reshaped Data

            if (dim_id == 0) {
                new_rows = dims[1].dim_n;
                new_cols = end - start + 1;
            } else {
                new_cols = dims[0].dim_n;
                new_rows = end - start + 1;
            }
            pa = (T**)malloc(new_rows * sizeof(T*));
            for (int j = 0; j < new_rows; j++) {
                pa[j] = &p[j * new_cols];
            }

            // Reshape the Data

            if (dim_id == 0) {
                for (int j = 0; j < rows; j++) {
                    int k = 0;
                    if (!reverse) {
                        for (int i = start; i < end; i += stride) {
                            pa[j][k++] = dpa[j][i];
                            count++;
                        }
                    } else {
                        for (int i = end - 1; i >= start; i -= stride) {
                            pa[j][k++] = dpa[j][i];
                            count++;
                        }
                    }
                }
            } else {
                int k = 0;
                if (!reverse) {
                    for (int j = start; j < end; j += stride) {
                        for (int i = 0; i < columns; i++) {
                            pa[k][i] = dpa[j][i];
                            count++;
                        }
                        k++;
                    }
                } else {
                    for (int j = end - 1; j >= start; j -= stride) {
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

    return 0;
}

int server_new_data_array2(Dims* dims, int rank, int dim_id, char* data, int n_data, int data_type, bool reverse,
                           int start, int end, int stride, int* n, void** new_data)
{
    UDA_LOG(UDA_LOG_DEBUG, "Data Type: {}    Rank: {}", data_type, rank);

    *n = 0;

    int ierr = 0;

    switch (data_type) {

        case UDA_TYPE_FLOAT:
            ierr = apply_subsetting<float>(dims, rank, dim_id, data, n_data, data_type, reverse, start, end, stride, n, new_data);
            break;

        case UDA_TYPE_DOUBLE:
            ierr = apply_subsetting<double>(dims, rank, dim_id, data, n_data, data_type, reverse, start, end, stride, n, new_data);
            break;

        case UDA_TYPE_INT:
            ierr = apply_subsetting<int>(dims, rank, dim_id, data, n_data, data_type, reverse, start, end, stride, n, new_data);
            break;

        case UDA_TYPE_UNSIGNED_INT:
            ierr = apply_subsetting<unsigned int>(dims, rank, dim_id, data, n_data, data_type, reverse, start, end, stride, n, new_data);
            break;

        default:
            UDA_LOG(UDA_LOG_ERROR,
                    "Only Float, Double and 32 bit Integer Numerical Types can be Subset at this time!\n");
            UDA_LOG(UDA_LOG_ERROR, "Data Type: {}    Rank: {}", data_type, rank);
            UDA_THROW_ERROR(9999,
                            "Only Float, Double and 32 bit Signed Integer Numerical Types can be Subset at this time!");
    }

    return ierr;
}

} // namespace
