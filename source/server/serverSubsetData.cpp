//--------------------------------------------------------------------------------------------------------------------
// Serverside Data Subsetting Operations Data
//
// Return Codes:	0 => OK, otherwise Error
//
//--------------------------------------------------------------------------------------------------------------------

#include "serverSubsetData.h"
#include "getServerEnvironment.h"

#include <math.h>
#include <float.h>
#include <errno.h>
#include <strings.h>

#include <logging/logging.h>
#include <clientserver/printStructs.h>
#include <clientserver/errorLog.h>
#include <clientserver/udaTypes.h>
#include <structures/struct.h>
#include <clientserver/initStructs.h>
#include <clientserver/compressDim.h>
#include <clientserver/stringUtils.h>
#include <clientserver/freeDataBlock.h>

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

static int idamserversubsetindices(char* operation, DIMS* dim, double value, unsigned int* subsetindices);

static int idamserverNewDataArray2(DIMS* dims, int rank, int dimid,
                            char* data, int ndata, int data_type, int notoperation, int reverse,
                            int start, int end, int start1, int end1, int* n, void** newdata);

int idamserverSubsetData(DATA_BLOCK* data_block, ACTION action, LOGMALLOCLIST* logmalloclist)
{
    DIMS* dim;
    //DIMS *rdims;
    DIMS newdim;
    SUBSET subset;
    char* operation;

    char* newdata, * newerrhi, * newerrlo;
    int nsubsets, nbound, dimid, start, end, start1, end1, dim_n, ndata, n, reshape, reverse, notoperation, ierr = 0;

    printAction(action);
    printDataBlock(*data_block);

    //-----------------------------------------------------------------------------------------------------------------------
    // How many sets of subsetting operations?

    if (action.actionType == COMPOSITETYPE) {            // XML Based subsetting
        if (action.composite.nsubsets == 0) return 0;        // Nothing to Subset
        nsubsets = action.composite.nsubsets;
    } else {
        if (action.actionType == SERVERSIDETYPE) {        // Client Requested subsetting
            if (action.serverside.nsubsets == 0) return 0;    // Nothing to Subset
            nsubsets = action.serverside.nsubsets;
        } else {
            if (action.actionType == SUBSETTYPE) {            // Client Requested subsetting
                nsubsets = 1;
            } else {
                return 0;
            }
        }
    }

    //-----------------------------------------------------------------------------------------------------------------------
    // Check Rank

    if (data_block->rank > 2 &&
        !(action.actionType == SUBSETTYPE && !strncasecmp(action.subset.function, "rotateRZ", 8))) {
        ierr = 9999;
        addIdamError(CODEERRORTYPE, "idamserverSubsetData", ierr,
                     "Not Configured to Subset Data with Rank Higher than 2");
        return ierr;
    }

    //-----------------------------------------------------------------------------------------------------------------------
    // Process all sets of subsetting operations

    for (int i = 0; i < nsubsets; i++) {                        // the number of sets of Subset Operations

        if (action.actionType == COMPOSITETYPE) {
            subset = action.composite.subsets[i];                // the set of Subset Operations
        } else {
            if (action.actionType == SERVERSIDETYPE) {
                subset = action.serverside.subsets[i];
            } else {
                if (action.actionType == SUBSETTYPE) {
                    subset = action.subset;
                }
            }
        }

        nbound = subset.nbound;                        // the Number of operations in the set

        for (int j = 0; j < nbound; j++) {                        // Process each operation separately

            double value = subset.bound[j];
            operation = subset.operation[j];                // a single operation
            dimid = subset.dimid[j];                    // applied to this dimension (if -1 then to data only!)

            UDA_LOG(UDA_LOG_DEBUG, "[%d][%d]Value = %e, Operation = %s, DIM id = %d, Reform = %d\n",
                    i, j, value, operation, dimid, subset.reform);

            if (dimid < 0 || dimid >= (int)data_block->rank) {
                ierr = 9999;
                UDA_LOG(UDA_LOG_ERROR, "Error ***    DIM id = %d,  Rank = %d, Test = %d \n",
                        dimid, data_block->rank, dimid >= (int)data_block->rank);
                addIdamError(CODEERRORTYPE, "idamserverSubsetData", ierr,
                             "Data Subsetting is Impossible as the subset Dimension is not Compatible with the Rank of the Signal");
                printDataBlock(*data_block);
                return ierr;
            }

            //----------------------------------------------------------------------------------------------------------------------------
            // Operations on Simple Data Structures: target must be an Atomic Type, Scalar, Name is Case Sensitive
            //
            // (mapType=1) Array of Structures - member = single scalar value: rank and shape = rank and shape of structure array
            // (mapType=2) Single Structure - member = array of values: rank and shape = increase rank and shape of structure member by 1
            // Array of Structures - member = array of values: Not allowed.
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
                auto udt = (USERDEFINEDTYPE*)data_block->opaque_block;

                // Extract an atomic type data element from the data structure

                for (i = 0; i < udt->fieldcount; i++) {
                    if (STR_EQUALS(udt->compoundfield[i].name, subset.member)) {        // Locate target member by name

                        data_n = data_block->data_n;

                        if (!udt->compoundfield[i].pointer) {            // Regular Array of data

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
                                    double* data = nullptr, * dp;

                                    if (mapType == 1) {
                                        data = (double*)malloc(data_n * sizeof(double));
                                        for (j = 0; j < data_n; j++)
                                            data[j] = *(double*)&data_block->data[j * udt->size +
                                                                                  udt->compoundfield[i].offset];
                                    } else {
                                        if (mapType == 2) {
                                            data_n = count;
                                            data = (double*)malloc(data_n * sizeof(double));
                                            dp = (double*)&data_block->data[udt->compoundfield[i].offset];
                                            for (j = 0; j < data_n; j++) data[j] = dp[j];

                                            // Increase rank and shape by 1: Preserve the original dimensional data
                                            // Replace with index using rank and shape from the member

                                            if ((shape = udt->compoundfield[i].shape) == nullptr &&
                                                udt->compoundfield[i].rank > 1) {
                                                ierr = 999;
                                                addIdamError(CODEERRORTYPE, "idamserverSubsetData",
                                                             ierr,
                                                             "The Data Structure member's shape data is missing (rank > 1)");
                                                return ierr;
                                            }

                                        } else {        // mapType == 3
                                            int total_n, jjj = 0;
                                            total_n = count * data_n;
                                            data = (double*)malloc(total_n * sizeof(double));
                                            jjj = 0;
                                            int jj;
                                            for (jj = 0; jj < data_n; jj++) {    // Loop over structures
                                                dp = (double*)&data_block->data[jj * udt->size +
                                                                                udt->compoundfield[i].offset];
                                                for (j = 0; j < count; j++) data[jjj++] = dp[j];
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

                            if (data_n > 1) {
                                int count_p = 0;
                                int rank_p = 0;
                                int size_p = 0;
                                int* shape_p = nullptr;
                                const char* type_name_p = nullptr;

                                for (j = 0; j <
                                            data_n; j++) {    // Properties Must be identical for all structure array elements
                                    extract = *(char**)&data_block->data[j * udt->size + udt->compoundfield[i].offset];
                                    findMalloc2(logmalloclist, (void*)extract, &count, &size, &type_name, &rank,
                                                &shape);
                                    if (j > 0) {
                                        if (count != count_p || size != size_p || rank != rank_p ||
                                            strcmp(type_name, type_name_p) != 0) {
                                            // ERROR
                                        }
                                        if (shape != nullptr) {
                                            int k;
                                            for (k = 0; k < rank; k++) {
                                                if (shape[k] != shape_p[k]) {
                                                    //ERROR
                                                }
                                            }
                                        } else {
                                            if (rank > 1) {
                                                ierr = 999;
                                                addIdamError(CODEERRORTYPE, "idamserverSubsetData",
                                                             ierr,
                                                             "The Data Structure member's shape data is missing (rank > 1)");
                                                return ierr;
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

                            if (count == 1 && data_n >= 1) {
                                mapType = 1;
                            } else {
                                if (count >= 1 && data_n == 1) {
                                    mapType = 2;
                                } else {
                                    ierr = 999;
                                    addIdamError(CODEERRORTYPE, "idamserverSubsetData", ierr,
                                                 "Unable to subset an array of Data Structures when the target "
                                                 "member is also an array. Functionality has not been implemented!)");
                                    return ierr;
                                }
                            }

                            type = gettypeof(type_name);

                            switch (type) {
                                case UDA_TYPE_DOUBLE: {
                                    double* data = nullptr, * dp;
                                    if (mapType == 1) {
                                        data = (double*)malloc(data_n * sizeof(double));
                                        for (j = 0; j < data_n; j++) {
                                            dp = *(double**)&data_block->data[j * udt->size +
                                                                              udt->compoundfield[i].offset];
                                            data[j] = dp[0];
                                        }
                                    } else {
                                        if (mapType == 2) {
                                            data_n = count;
                                            data = (double*)malloc(data_n * sizeof(double));
                                            dp = *(double**)&data_block->data[udt->compoundfield[i].offset];
                                            for (j = 0; j < data_n; j++) data[j] = dp[j];
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

                            int k;
                            for (k = k0; k < rank; k++) {
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

                                unsigned int k;
                                for (k = k0; k < data_block->rank; k++) {
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
                            free((void*)logmalloclist);
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

            if (STR_EQUALS(operation, "*")) {
                continue;
            }                // This means No subset - Part of an array Reshape Operation

            if (operation[0] == ':' && subset.lbindex[j] == 0 &&
                (subset.ubindex[j] == -1 || subset.ubindex[j] == data_block->dims[dimid].dim_n - 1)) {
                continue;    // subset spans the complete dimension
            }

            //----------------------------------------------------------------------------------------------------------------------------
            // Decompress the dimensional data if necessary & free Heap Associated with Compression

            initDimBlock(&newdim);                    // Holder for the Subsetted Dimension (part copy of the original)

            dim = &(data_block->dims[dimid]);                // the original dimension to be subset

            if (dim->compressed) {
                uncompressDim(dim);
                dim->compressed = 0;                    // Can't preserve this status after the subset has been applied
                dim->method = 0;

                if (dim->sams != nullptr) free((void*)dim->sams);
                if (dim->offs != nullptr) free((void*)dim->offs);
                if (dim->ints != nullptr) free((void*)dim->ints);

                dim->udoms = 0;
                dim->sams = nullptr;        // Avoid double freeing of Heap
                dim->offs = nullptr;
                dim->ints = nullptr;
            }

            //----------------------------------------------------------------------------------------------------------------------------
            // Copy all existing Dimension Information to working structure

            newdim = *dim;
            newdim.dim_n = 0;
            newdim.dim = nullptr;

            //----------------------------------------------------------------------------------------------------------------------------
            // Subset Operations: Identify subset indicies

            // Subsetting Indices

            start = -1;                    // Starting Index satisfying the operation
            end = -1;                    // Ending Index
            start1 = -1;
            end1 = -1;

            // Test for Array Reshaping Operations

            reshape = 0;                    // Subsetting has been defined using array indexing notation
            reverse = 0;                    // Reverse the ordering of elements (also uses array indexing notation)

            notoperation = (operation[0] == '!');        // a NOT operator => One or Two subsets

            if (operation[0] == '*')
                continue;        // This means No dimensional subset - Part of an array Reshape Operation

            if (operation[0] == ':') {            // Reshape Operation - Index Range Specified
                start = (int)subset.lbindex[j];        // Number before the :
                end = (int)subset.ubindex[j];        // Number after the :
                if (start == -1) start = 0;
                if (start == -2) start = dim->dim_n - 1;    // Final array element requested
                if (end == -1) end = dim->dim_n - 1;

                if (start > end) {                // Check Ordering (Allow for Reversing?)
                    int startcpy = start;
                    reverse = 1;
                    start = end;        // Swap indices
                    end = startcpy;
                }
                reshape = 1;
                dim_n = end - start + 1;
            }

            if (operation[0] == '#') {            // Reshape Operation - Highest array position (last value)
                start = dim->dim_n - 1;
                end = dim->dim_n - 1;
                reshape = 1;
                dim_n = 1;
            }

            // Create an Array of Indices satisfying the criteria

            if (!reshape) {

                auto subsetindices = (unsigned int*)malloc(dim->dim_n * sizeof(unsigned int));

                if (STR_EQUALS(operation, "!<")) strcpy(operation, ">=");
                if (STR_EQUALS(operation, "!>")) strcpy(operation, "<=");
                if (STR_EQUALS(operation, "!<=")) strcpy(operation, ">");
                if (STR_EQUALS(operation, "!>=")) strcpy(operation, "<");

                if ((dim_n = idamserversubsetindices(operation, dim, value, subsetindices)) == 0) {
                    ierr = 9999;
                    addIdamError(CODEERRORTYPE, "idamserverSubsetData", ierr,
                                 "No Data were found that satisfies a subset");
                    free((void*)subsetindices);
                    return ierr;
                }

                // Start and End of Subset Ranges

                start = subsetindices[0];
                end = subsetindices[dim_n - 1];

                if (notoperation && dim_n > 1) {        // Double Range	?
                    int range2 = 0;
                    if (dim_n == dim->dim_n) {
                        notoperation = 0;            // No Second Range found so switch OFF NOT Operation
                    } else {
                        end1 = subsetindices[dim_n - 1];
                        int k;
                        for (k = 0; k < dim_n; k++) {
                            if (subsetindices[k] != subsetindices[0] + k) {
                                end = subsetindices[k - 1];
                                start1 = subsetindices[k];
                                break;
                            }
                        }
                        range2 = end1 - start1 + 1;
                    }
                    if (dim_n != end - start + 1 + range2) {        // Dimension array is Not well ordered!
                        ierr = 9999;
                        addIdamError(CODEERRORTYPE, "idamserverSubsetData", ierr,
                                     "The Dimensional Array is Not Ordered: Unable to Subset");
                        free((void*)subsetindices);
                        return ierr;
                    }
                } else {
                    if (dim_n != end - start + 1) {        // Dimension array is Not well ordered!
                        ierr = 9999;
                        addIdamError(CODEERRORTYPE, "idamserverSubsetData", ierr,
                                     "The Dimensional Array is Not Ordered: Unable to Subset");
                        free((void*)subsetindices);
                        return ierr;
                    }
                }
                free((void*)subsetindices);
            }

            newdim.dim_n = dim_n;

            //----------------------------------------------------------------------------------------------------------------------------
            // Build the New Subsetted Dimension

            printDataBlock(*data_block);
            UDA_LOG(UDA_LOG_DEBUG, "\n\n\n*** dim->data_type: %d\n\n\n", dim->data_type);
            UDA_LOG(UDA_LOG_DEBUG, "\n\n\n*** dim->errhi != nullptr: %d\n\n\n", dim->errhi != nullptr);
            UDA_LOG(UDA_LOG_DEBUG, "\n\n\n*** dim->errlo != nullptr: %d\n\n\n", dim->errlo != nullptr);

            if ((ierr = idamserverNewDataArray2(dim, 1, dimid, dim->dim, dim_n, dim->data_type, notoperation, reverse,
                                                start, end, start1, end1, &n, (void**)&newdim.dim)) != 0) {
                return ierr;
            }

            if (dim->errhi != nullptr && dim->error_type != UDA_TYPE_UNKNOWN) {
                if ((ierr = idamserverNewDataArray2(dim, 1, dimid, dim->errhi, dim_n, dim->error_type, notoperation,
                                                    reverse,
                                                    start, end, start1, end1, &n, (void**)&newdim.errhi)) != 0)
                    return ierr;
            }

            if (dim->errlo != nullptr && dim->error_type != UDA_TYPE_UNKNOWN) {
                if ((ierr = idamserverNewDataArray2(dim, 1, dimid, dim->errlo, dim_n, dim->error_type, notoperation,
                                                    reverse,
                                                    start, end, start1, end1, &n, (void**)&newdim.errlo)) != 0)
                    return ierr;
            }

            //-----------------------------------------------------------------------------------------------------------------------
            // Reshape and Save the Subsetted Data

            printDataBlock(*data_block);

            if ((ierr = idamserverNewDataArray2(data_block->dims, data_block->rank, dimid, data_block->data,
                                                data_block->data_n, data_block->data_type, notoperation, reverse,
                                                start, end, start1, end1, &ndata, (void**)&newdata)) != 0) {
                return ierr;
            }

            if (data_block->error_type != UDA_TYPE_UNKNOWN && data_block->errhi != nullptr) {
                if ((ierr = idamserverNewDataArray2(data_block->dims, data_block->rank, dimid, data_block->errhi,
                                                    data_block->data_n, data_block->error_type, notoperation, reverse,
                                                    start, end, start1, end1, &n, (void**)&newerrhi)) != 0) {
                    return ierr;
                }
                free((void*)data_block->errhi);                // Free Original Heap
                data_block->errhi = newerrhi;                // Replace with the Reshaped Array
            }

            if (data_block->error_type != UDA_TYPE_UNKNOWN && dim->errlo != nullptr) {
                if ((ierr = idamserverNewDataArray2(data_block->dims, data_block->rank, dimid, data_block->errlo,
                                                    data_block->data_n, data_block->error_type, notoperation, reverse,
                                                    start, end, start1, end1, &n, (void**)&newerrlo)) != 0) {
                    return ierr;
                }
                free((void*)data_block->errlo);                // Free Original Heap
                data_block->errlo = newerrlo;                // Replace with the Reshaped Array
            }

            data_block->data_n = ndata;

            free((void*)data_block->data);                // Free Original Heap
            data_block->data = newdata;                    // Replace with the Reshaped Array

            // replace the Original Dimensional Structure with the New Subsetted Structure unless a
            // REFORM [Rank Reduction] has been requested and the dimension length is 1 (this has no effect on the Data Array items)

            // Free Heap associated with the original Dimensional Structure Array

            if (dim->dim != nullptr) free((void*)dim->dim);
            if (dim->errlo != nullptr) free((void*)dim->errlo);
            if (dim->errhi != nullptr) free((void*)dim->errhi);

            dim->dim = nullptr;
            dim->errlo = nullptr;
            dim->errhi = nullptr;

            // Save the reshaped Dimension or Reform the whole

            data_block->dims[dimid] = newdim;                            // Replace with the subsetted dimension
        }
    }

    //-------------------------------------------------------------------------------------------------------------
    // Reform the Data if requested

    if (ierr == 0 && subset.reform) {
        int rank = data_block->rank;
        int j;
        for (j = 0; j < rank; j++) {
            if (data_block->dims[j].dim_n <= 1) {
                UDA_LOG(UDA_LOG_DEBUG, "Reforming Dimension %d\n", j);

                data_block->dims[j].compressed = 0;
                data_block->dims[j].method = 0;

                if (data_block->dims[j].dim != nullptr) free((void*)data_block->dims[j].dim);
                if (data_block->dims[j].errlo != nullptr) free((void*)data_block->dims[j].errlo);
                if (data_block->dims[j].errhi != nullptr) free((void*)data_block->dims[j].errhi);
                if (data_block->dims[j].sams != nullptr) free((void*)data_block->dims[j].sams);
                if (data_block->dims[j].offs != nullptr) free((void*)data_block->dims[j].offs);
                if (data_block->dims[j].ints != nullptr) free((void*)data_block->dims[j].ints);

                int k;
                for (k = j + 1; k < rank; k++) {
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
    }

    //-------------------------------------------------------------------------------------------------------------
    // Apply simple functions to the subset data: XML syntax function="name(dimid=9)"

    if (ierr == 0 && subset.function[0] != '\0') {

        if (!strncasecmp(subset.function, "minimum(", 8)) {        // Single scalar result
            dimid = 0;
            if (data_block->rank >= 1) {
                char* p1 = strstr(subset.function, "dimid");
                if (p1 != nullptr) {
                    char* p3, * p2 = strchr(p1, '=');
                    p2[0] = ' ';
                    p3 = strchr(p2, ')');
                    p3[0] = '\0';
                    if (IsNumber(p2)) {
                        dimid = atoi(p2);
                    } else {
                        // ERROR
                    }
                } else {
                    // ERROR
                }
            }

            if (dimid < 0 || dimid >= (int)data_block->rank) {
                ierr = 999;
                UDA_LOG(UDA_LOG_ERROR, "Function Syntax Error -  dimid = %d,  Rank = %d\n", dimid,
                        data_block->rank);
                addIdamError(CODEERRORTYPE, "idamserverSubsetData", ierr,
                             "The dimension ID identified via the subset function is outside the rank bounds of the array!");
                return ierr;
            }

            switch (data_block->data_type) {
                case UDA_TYPE_FLOAT: {
                    auto dp = (float*)data_block->data;
                    float min = dp[0];
                    switch (data_block->rank) {
                        case 0: {            // Ignore function dimid argument
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
                            newdim = data_block->dims[0];
                            for (int j = 1; j < data_block->dims[0].dim_n; j++) {
                                if (dp[j] < min) {
                                    min = dp[j];
                                }
                            }
                            dp[0] = min;
                            dp = (float*)realloc((void*)dp, sizeof(float));        // Reduce array size
                            data_block->rank = 0;
                            data_block->data_n = 1;
                            free((void*)data_block->dims[0].dim);
                            free((void*)data_block->dims);
                            data_block->dims = nullptr;
                            break;
                        }
                        case 2: {
                            float* ddp;
                            if (dimid == 0) {
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
                                free((void*)dp);
                                data_block->rank = 1;
                                data_block->data_n = data_block->dims[1].dim_n;
                                data_block->data = (char*)ddp;
                                free((void*)data_block->dims[0].dim);
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
                                free((void*)dp);
                                data_block->rank = 1;
                                data_block->data_n = data_block->dims[0].dim_n;
                                data_block->data = (char*)ddp;
                                free((void*)data_block->dims[1].dim);
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
                        case 0: {            // Ignore function dimid argument
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
                            newdim = data_block->dims[0];
                            for (int j = 1; j < data_block->dims[0].dim_n; j++) {
                                if (dp[j] < min) {
                                    min = dp[j];
                                }
                            }
                            dp[0] = min;
                            dp = (double*)realloc((void*)dp, sizeof(double));        // Reduce array size
                            data_block->rank = 0;
                            data_block->data_n = 1;
                            free((void*)data_block->dims[0].dim);
                            free((void*)data_block->dims);
                            data_block->dims = nullptr;
                            break;
                        }
                        case 2: {
                            double* ddp;
                            if (dimid == 0) {
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
                                free((void*)dp);
                                data_block->rank = 1;
                                data_block->data_n = data_block->dims[1].dim_n;
                                data_block->data = (char*)ddp;
                                free((void*)data_block->dims[0].dim);
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
                                free((void*)dp);
                                data_block->rank = 1;
                                data_block->data_n = data_block->dims[0].dim_n;
                                data_block->data = (char*)ddp;
                                free((void*)data_block->dims[1].dim);
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

        if (STR_IEQUALS(subset.function, "maximum")) {        // Single scalar result
        }

        if (!strncasecmp(subset.function, "count", 5)) {        // Single scalar result
            char* p1 = strstr(subset.function, "dimid");
            auto count = (unsigned int*)malloc(sizeof(unsigned int));
            if (p1 == nullptr) {
                count[0] = (unsigned int)data_block->data_n;
                freeDataBlock(data_block);
                initDataBlock(data_block);
                data_block->data_n = 1;
                data_block->data = (char*)count;
                data_block->data_type = UDA_TYPE_UNSIGNED_INT;
            } else {
                dimid = 0;
                if (data_block->rank >= 1) {
                    char* p3, * p2 = strchr(p1, '=');
                    p2[0] = ' ';
                    p3 = strchr(p2, ')');
                    p3[0] = '\0';
                    if (IsNumber(p2)) {
                        dimid = atoi(p2);
                    } else {
                        // ERROR
                    }
                }
                if (dimid < (int)data_block->rank) {
                    count[0] = (unsigned int)data_block->dims[dimid].dim_n;        // Preserve this value
                    DIMS ddim = data_block->dims[dimid];
                    if (ddim.dim != nullptr) free((void*)ddim.dim);
                    if (ddim.errhi != nullptr) free((void*)ddim.errhi);
                    if (ddim.errlo != nullptr) free((void*)ddim.errlo);
                    if (ddim.sams != nullptr) free((void*)ddim.sams);
                    if (ddim.offs != nullptr) free((void*)ddim.offs);
                    if (ddim.ints != nullptr) free((void*)ddim.ints);
                } else {
                    // ERROR
                }
                if (data_block->data != nullptr) free((void*)data_block->data);
                if (data_block->errhi != nullptr) free((void*)data_block->errhi);
                if (data_block->errlo != nullptr) free((void*)data_block->errlo);
                data_block->data = nullptr;
                data_block->errhi = nullptr;
                data_block->errlo = nullptr;
                data_block->error_type = UDA_TYPE_UNKNOWN;
                data_block->error_param_n = 0;

                data_block->data_n = 1;
                unsigned int j;
                for (j = 0; j < data_block->rank - 1; j++) {
                    if (j >= (unsigned int)dimid) {
                        data_block->dims[j] = data_block->dims[j + 1];        // skip over the target
                    }
                    data_block->data_n = data_block->data_n * data_block->dims[j].dim_n;
                }
                data_block->rank = data_block->rank - 1;
                data_block->data_type = UDA_TYPE_UNSIGNED_INT;

                count = (unsigned int*)realloc((void*)count, data_block->data_n * sizeof(unsigned int));
                for (j = 1; j < (unsigned int)data_block->data_n; j++) {
                    count[j] = count[0];
                }
                data_block->data = (char*)count;
                data_block->data_units[0] = '\0';
                sprintf(data_block->data_label, "count(dimid=%d)", dimid);
            }
        }

        if (!strncasecmp(subset.function, "abs()", 5)) {            // Absolute value
            int j;
            switch (data_block->data_type) {
                case UDA_TYPE_FLOAT: {
                    auto dp = (float*)data_block->data;
                    for (j = 0; j < data_block->data_n; j++) {
                        dp[j] = fabsf(dp[j]);
                    }
                    break;
                }
                case UDA_TYPE_DOUBLE: {
                    auto dp = (double*)data_block->data;
                    for (j = 0; j < data_block->data_n; j++) {
                        dp[j] = fabs(dp[j]);
                    }
                    break;
                }
                default:
                    break;
            }
        }

        if (!strncasecmp(subset.function, "const", 5)) {        // Constant value substitution
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

            if (data_block->errhi != nullptr) free((void*)data_block->errhi);
            if (data_block->errlo != nullptr) free((void*)data_block->errlo);
            data_block->errhi = nullptr;
            data_block->errlo = nullptr;
            data_block->error_type = UDA_TYPE_UNKNOWN;
            data_block->error_param_n = 0;

            data_block->data_units[0] = '\0';

            int j;
            switch (data_block->data_type) {
                case UDA_TYPE_FLOAT: {
                    auto dp = (float*)data_block->data;
                    for (j = 0; j < data_block->data_n; j++) {
                        dp[j] = (float)value;
                    }
                    break;
                }
                case UDA_TYPE_DOUBLE: {
                    auto dp = (double*)data_block->data;
                    for (j = 0; j < data_block->data_n; j++) {
                        dp[j] = value;
                    }
                    break;
                }
                default:
                    break;
            }
        }

        if (!strncasecmp(subset.function, "order", 5)) {        // Identify the Time dimension order
            char* p1 = strstr(subset.function, "dimid");
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
                    data_block->order = atof(p2);
                } else {
                    // ERROR
                }
            }
            UDA_LOG(UDA_LOG_DEBUG, "order = %d\n", data_block->order);
        }

        if (!strncasecmp(subset.function, "rotateRZ", 8)) {        // Rotate R,Z coordinates in rank 3 array
            UDA_LOG(UDA_LOG_DEBUG, "%s\n", subset.function);
            if (data_block->rank != 3) {
                ierr = 999;
                addIdamError(CODEERRORTYPE, "idamserverSubsetData", ierr,
                             "The function rotateRZ only operates on rank 3 arrays");
                return ierr;
            }
            int order = data_block->order;
            if (order < 0) {
                ierr = 999;
                addIdamError(CODEERRORTYPE, "idamserverSubsetData", ierr,
                             "The function rotateRZ expects a Time coordinate");
                return ierr;
            }
            int type = data_block->data_type;
            if (type != UDA_TYPE_DOUBLE) {
                ierr = 999;
                addIdamError(CODEERRORTYPE, "idamserverSubsetData", ierr,
                             "The function rotateRZ is configured for type DOUBLE only");
                return ierr;
            }
            int i, j, k, nt, nr, nz, count;
            count = data_block->data_n;
            auto newData = (double*)malloc(count * sizeof(double));
            unsigned int offset = 0;
            auto old = (double*)data_block->data;
            if (order == 0) {        // array[nz][nr][nt] -> [nr][nz][nt]
                nt = data_block->dims[0].dim_n;
                nr = data_block->dims[1].dim_n;
                nz = data_block->dims[2].dim_n;

                auto data = (double***)malloc(nz * sizeof(double**));
                for (j = 0; j < nz; j++) {
                    data[j] = (double**)malloc(nr * sizeof(double*));
                    for (i = 0; i < nr; i++) {
                        data[j][i] = (double*)malloc(nt * sizeof(double));
                        for (k = 0; k < nt; k++)data[j][i][k] = old[offset++];
                    }
                }
                offset = 0;
                for (i = 0; i < nr; i++)
                    for (j = 0; j < nz; j++)
                        for (k = 0; k < nt; k++)
                            newData[offset++] = data[j][i][k];
                for (j = 0; j < nz; j++) {
                    for (i = 0; i < nr; i++) free((void*)data[j][i]);
                    free((void*)data[j]);
                }
                free((void*)data);

                DIMS d1 = data_block->dims[1];
                DIMS d2 = data_block->dims[2];
                data_block->dims[1] = d2;
                data_block->dims[2] = d1;
            } else if (order == 1) {        // array[nz][nt][nr]
                ierr = 999;
                addIdamError(CODEERRORTYPE, "idamserverSubsetData", ierr,
                             "The function rotateRZ only operates on arrays with shape [nz][nr][nt] or [nt][nz][nr]");
                return ierr;
            } else if (order == 2) {        // array[nt][nz][nr] -> [nt][nr][nz]
                nr = data_block->dims[0].dim_n;
                nz = data_block->dims[1].dim_n;
                nt = data_block->dims[2].dim_n;

                auto data = (double***)malloc(nt * sizeof(double**));
                for (k = 0; k < nt; k++) {
                    data[k] = (double**)malloc(nz * sizeof(double*));
                    for (j = 0; j < nz; j++) {
                        data[k][j] = (double*)malloc(nr * sizeof(double));
                        for (i = 0; i < nr; i++)data[k][j][i] = old[offset++];
                    }
                }
                offset = 0;
                for (k = 0; k < nt; k++)
                    for (i = 0; i < nr; i++)
                        for (j = 0; j < nz; j++)
                            newData[offset++] = data[k][j][i];
                for (k = 0; k < nt; k++) {
                    for (j = 0; j < nz; j++)free((void*)data[k][j]);
                    free((void*)data[k]);
                }
                free((void*)data);
                DIMS d0 = data_block->dims[0];
                DIMS d1 = data_block->dims[1];
                data_block->dims[0] = d1;
                data_block->dims[1] = d0;
            } else {
                ierr = 999;
                addIdamError(CODEERRORTYPE, "idamserverSubsetData", ierr,
                             "rotateRZ: Incorrect ORDER value found!");
                return ierr;
            }
            free((void*)data_block->data);
            data_block->data = (char*)newData;
        }

    }

    //-------------------------------------------------------------------------------------------------------------
    // Explicitly set the order of the time dimension if not possible via the other options

    if (subset.order >= 0) data_block->order = subset.order;

    return ierr;
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
// SS::SUBSET(\"xx\", [*, 3], member=\"name\", reform, function=\"minimum(dimid=0)\" )


int idamserverParseServerSide(REQUEST_BLOCK* request_block, ACTIONS* actions_serverside)
{

    char qchar[2];
    char* p = nullptr, * t1 = nullptr, * t2 = nullptr;
    char api_delim[3] = "::";            // ********** TO DO: This should be an Environment Variable compatible with the Client delimiter
    char archive[STRING_LENGTH] = "";
    char signal[STRING_LENGTH] = "";
    char options[STRING_LENGTH] = "";
    char operation[STRING_LENGTH];
    char opcopy[STRING_LENGTH];
    char* endp = nullptr;

    int lsignal, nactions, nsubsets, nbound, ierr, i, j, lop;

    ACTION* action = nullptr;
    SUBSET* subsets = nullptr;

    ierr = 0;

    //-------------------------------------------------------------------------------------------------------------
    // Extract the ARCHIVE::SIGNAL element
    // use the first character after the left parenthesis as the opening quotation character

    strncpy(qchar, request_block->signal + 7, 1);
    qchar[1] = '\0';

    if ((p = strstr(request_block->signal + 8, qchar)) == nullptr) {    // Locate the terminating quotation character
        ierr = 9999;
        addIdamError(CODEERRORTYPE, "idamserverParseServerSide", ierr,
                     "Syntax Error: The Signal Name has no Terminating "
                     "Quotation character! ");
        return ierr;
    }

    lsignal = (int)(p - request_block->signal) - 8;            // Signal name Length
    if (lsignal >= STRING_LENGTH) lsignal = STRING_LENGTH - 1;
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

    if ((t1 = strstr(p + 1, ",")) == nullptr) {    // Locate the separation character
        ierr = 9999;
        addIdamError(CODEERRORTYPE, "idamserverParseServerSide", ierr,
                     "Syntax Error: No Comma after the Signal Name ");
        return ierr;
    }

    if ((t1 = strstr(t1 + 1, "[")) == nullptr) {    // Locate the Operation
        ierr = 9999;
        addIdamError(CODEERRORTYPE, "idamserverParseServerSide", ierr,
                     "Syntax Error: No [ enclosing the Operation ");
        return ierr;
    }

    if ((t2 = strstr(t1 + 1, "]")) == nullptr) {
        ierr = 9999;
        addIdamError(CODEERRORTYPE, "idamserverParseServerSide", ierr,
                     "Syntax Error: No ] enclosing the Operation ");
        return ierr;
    }

    strncpy(operation, t1 + 1, t2 - t1 - 1);    // The Requested Operation including Values
    operation[t2 - t1 - 1] = '\0';

    //-------------------------------------------------------------------------------------------------------------
    // remaining options, e.g., REFORM

    if ((t1 = strstr(t2 + 1, ",")) != nullptr) {
        strcpy(options, t1 + 1);
    }

    //-------------------------------------------------------------------------------------------------------------
    // Overwrite the Request Block to enable the correct access to signal data before the subset operations are applied

    strcpy(request_block->archive, archive);
    if (request_block->archive[0] == '\0') strcpy(request_block->archive, getIdamServerEnvironment()->api_archive);

    strcpy(request_block->signal, signal);

    //-------------------------------------------------------------------------------------------------------------
    // Extend the Action Structure and Initialise

    nactions = actions_serverside->nactions + 1;
    if ((action = (ACTION*)realloc((void*)actions_serverside->action, nactions * sizeof(ACTION))) == nullptr) {
        ierr = 9999;
        addIdamError(CODEERRORTYPE, "idamserverParseServerSide", ierr,
                     "Unable to Allocate Heap memory");
        return ierr;
    }

    initAction(&action[nactions - 1]);

    action[nactions - 1].actionType = SERVERSIDETYPE;
    action[nactions - 1].inRange = 1;
    action[nactions - 1].actionId = nactions;

    initServerside(&action[nactions - 1].serverside);

    nsubsets = 1;
    if ((subsets = (SUBSET*)malloc(sizeof(SUBSET))) == nullptr) {
        ierr = 9999;
        addIdamError(CODEERRORTYPE, "idamserverParseServerSide", ierr,
                     "Unable to Allocate Heap memory");
        return ierr;
    }

    for (i = 0; i < nsubsets; i++) initSubset(&subsets[i]);

    action[nactions - 1].serverside.nsubsets = 1;
    strcpy(subsets[nsubsets - 1].data_signal, request_block->signal);

    // Seek specific options

    if ((p = strstr(options, "reform")) != nullptr) {            // Reduce rank
        subsets[nsubsets - 1].reform = 1;
    }

    if ((p = strstr(options, "member=")) != nullptr) {        // Extract a Structure member
        strcpy(subsets[nsubsets - 1].member, &p[7]);
        LeftTrimString(subsets[nsubsets - 1].member);
        if (subsets[nsubsets - 1].member[0] == '"') {
            subsets[nsubsets - 1].member[0] = ' ';
            LeftTrimString(subsets[nsubsets - 1].member);
        }
        if ((p = strchr(subsets[nsubsets - 1].member, '"')) != nullptr) p[0] = '\0';
        if ((p = strchr(subsets[nsubsets - 1].member, ',')) != nullptr) p[0] = '\0';
    }

    // Simple functions

    if ((p = strstr(options, "function=")) != nullptr) {        // Identify a function
        strcpy(subsets[nsubsets - 1].function, &p[9]);
        LeftTrimString(subsets[nsubsets - 1].function);
        if (subsets[nsubsets - 1].function[0] == '"') {
            subsets[nsubsets - 1].function[0] = ' ';
            LeftTrimString(subsets[nsubsets - 1].function);
        }
        if ((p = strchr(subsets[nsubsets - 1].function, '"')) != nullptr) p[0] = '\0';
        if ((p = strchr(subsets[nsubsets - 1].function, ',')) != nullptr) p[0] = '\0';
    }

    //-------------------------------------------------------------------------------------------------------------
    // Parse the Operation String for Value and Operation

    LeftTrimString(TrimString(operation));    // Remove Leading white space
    strcpy(opcopy, operation);
    nbound = 0;

    if ((p = strtok(opcopy, ",")) != nullptr) {        // Tokenise into Individual Operations on each Dimension
        subsets[nsubsets - 1].dimid[nbound] = nbound;    // Identify the Dimension to apply the operation on
        nbound++;
        if (strlen(p) < SXMLMAXSTRING) {
            strcpy(subsets[nsubsets - 1].operation[nbound - 1], p);
            MidTrimString(subsets[nsubsets - 1].operation[nbound - 1]);    // Remove internal white space
        } else {
            ierr = 9999;
            addIdamError(CODEERRORTYPE, "idamserverParseServerSide", ierr,
                         "Syntax Error: The Signal Operation String is too long ");
            free((void*)subsets);
            return ierr;
        }

        while ((p = strtok(nullptr, ",")) != nullptr) {
            subsets[nsubsets - 1].dimid[nbound] = nbound;
            nbound++;
            if (nbound > MAXDATARANK) {
                ierr = 9999;
                addIdamError(CODEERRORTYPE, "idamserverParseServerSide", ierr,
                             "The number of Dimensional Operations exceeds the Internal Limit ");
                free((void*)subsets);
                return ierr;
            }
            if (strlen(p) < SXMLMAXSTRING) {
                strcpy(subsets[nsubsets - 1].operation[nbound - 1], p);
                MidTrimString(subsets[nsubsets - 1].operation[nbound - 1]);    // Remove white space
            } else {
                ierr = 9999;
                addIdamError(CODEERRORTYPE, "idamserverParseServerSide", ierr,
                             "Syntax Error: The Signal Operation String is too long ");
                free((void*)subsets);
                return ierr;
            }
        }
    }

    subsets[nsubsets - 1].nbound = nbound;

    //-------------------------------------------------------------------------------------------------------------
    // Extract the Value Component from each separate Operation
    // =0.15,!=0.15,<=0.05,>=0.05,!<=0.05,!>=0.05,<0.05,>0.05,0:25,25:0,25,*,25:,:25
    //
    // Identify Three Types of Operations:
    //	A) Contains the characters: =,>, <, !, ~
    //	B) : or Integer Value
    //	C) * or #
    //

    for (i = 0; i < nbound; i++) {

        strcpy(opcopy, subsets[nsubsets - 1].operation[i]);

        if ((p = strstr(opcopy, ":")) != nullptr) {        // Integer Type Array Index Bounds
            t2 = p + 1;
            opcopy[p - opcopy] = '\0';            // Split the Operation String into two components
            t1 = opcopy;

            subsets[nsubsets - 1].isindex[i] = 1;
            subsets[nsubsets - 1].ubindex[i] = -1;
            subsets[nsubsets - 1].lbindex[i] = -1;

            if (t1[0] == '#') {
                subsets[nsubsets - 1].lbindex[i] = -2;
            }        // Reverse the data as # => Final array value

            if (strlen(t1) > 0 && t1[0] != '*' && t1[0] != '#') {
                if (IsNumber(t1)) {
                    subsets[nsubsets - 1].lbindex[i] = strtol(t1, &endp,
                                                              0);        // the Lower Index Value of the Bound
                    if (*endp != '\0' || errno == EINVAL || errno == ERANGE) {
                        ierr = 9999;
                        addIdamError(CODEERRORTYPE, "idamserverParseServerSide", ierr,
                                     "Server Side Operation Syntax Error: Lower Index Bound ");
                        free((void*)subsets);
                        return ierr;
                    }
                } else {
                    ierr = 9999;
                    addIdamError(CODEERRORTYPE, "idamserverParseServerSide", ierr,
                                 "Server Side Operation Syntax Error: Lower Index Bound ");
                    free((void*)subsets);
                    return ierr;
                }
            }
            if (strlen(t2) > 0 && t2[0] != '*' && t2[0] != '#') {
                if (IsNumber(t2)) {
                    subsets[nsubsets - 1].ubindex[i] = strtol(t2, &endp,
                                                              0);        // the Upper Index Value of the Bound
                    if (*endp != '\0' || errno == EINVAL || errno == ERANGE) {
                        ierr = 9999;
                        addIdamError(CODEERRORTYPE, "idamserverParseServerSide", ierr,
                                     "Server Side Operation Syntax Error: Upper Index Bound ");
                        free((void*)subsets);
                        return ierr;
                    }
                } else {
                    ierr = 9999;
                    addIdamError(CODEERRORTYPE, "idamserverParseServerSide", ierr,
                                 "Server Side Operation Syntax Error: Upper Index Bound ");
                    free((void*)subsets);
                    return ierr;
                }
            }
            strcpy(subsets[nsubsets - 1].operation[i], ":");        // Define Simple Operation
            continue;
        }

        if ((p = strstr(opcopy, "*")) != nullptr) {            // Ignore this Dimension
            subsets[nsubsets - 1].isindex[i] = 1;
            subsets[nsubsets - 1].ubindex[i] = -1;
            subsets[nsubsets - 1].lbindex[i] = -1;
            strcpy(subsets[nsubsets - 1].operation[i], "*");        // Define Simple Operation
            continue;
        }

        if ((p = strstr(opcopy, "#")) != nullptr) {            // Last Value in Dimension
            subsets[nsubsets - 1].isindex[i] = 1;
            subsets[nsubsets - 1].ubindex[i] = -1;
            subsets[nsubsets - 1].lbindex[i] = -1;
            strcpy(subsets[nsubsets - 1].operation[i], "#");        // Define Simple Operation
            continue;
        }

        if (IsNumber(opcopy)) {            // Single Index value
            subsets[nsubsets - 1].isindex[i] = 1;
            subsets[nsubsets - 1].ubindex[i] = strtol(opcopy, &endp, 0);        // the Index Value of the Bound
            if (*endp != '\0' || errno == EINVAL || errno == ERANGE) {
                ierr = 9999;
                addIdamError(CODEERRORTYPE, "idamserverParseServerSide", ierr,
                             "Server Side Operation Syntax Error: Single Index Bound ");
                free((void*)subsets);
                return ierr;
            }
            subsets[nsubsets - 1].lbindex[i] = subsets[nsubsets - 1].ubindex[i];
            strcpy(subsets[nsubsets - 1].operation[i], ":");        // Define Simple Operation
            continue;
        }

        // Single value Operation

        p = nullptr;                    // Locate the Start of the Numerical Substring
        lop = (int)strlen(subsets[nsubsets - 1].operation[i]);
        for (j = 0; j < lop; j++) {
            if (subsets[nsubsets - 1].operation[i][j] >= '0' && subsets[nsubsets - 1].operation[i][j] <= '9') {
                p = &subsets[nsubsets - 1].operation[i][j];
                if (j > 0) {                    // Capture sign
                    if (subsets[nsubsets - 1].operation[i][j - 1] == '+' ||
                        subsets[nsubsets - 1].operation[i][j - 1] == '-') {
                        p = &subsets[nsubsets - 1].operation[i][j - 1];
                    }
                } else {
                    ierr = 9999;
                    addIdamError(CODEERRORTYPE, "idamserverParseServerSide", ierr,
                                 "Server Side Operation Syntax Error: No Operator Defined! ");
                    free((void*)subsets);
                    return ierr;
                }
                break;
            }
        }

        if (p == nullptr) {
            ierr = 9999;
            addIdamError(CODEERRORTYPE, "idamserverParseServerSide", ierr,
                         "Server Side Operation Syntax Error: No Numerical Bound ");
            free((void*)subsets);
            return ierr;
        }

        subsets[nsubsets - 1].bound[i] = strtod(p, &endp);            // the Value of the Bound

        if (*endp != '\0' || errno == EINVAL || errno == ERANGE) {
            ierr = 9999;
            addIdamError(CODEERRORTYPE, "idamserverParseServerSide", ierr,
                         "Server Side Operation Syntax Error ");
            free((void*)subsets);
            return ierr;
        }

        subsets[nsubsets - 1].operation[i][p - &subsets[nsubsets -
                                                        1].operation[i][0]] = '\0';    // Isolate the Operation only

    }

    action[nactions - 1].serverside.nsubsets = nsubsets;
    action[nactions - 1].serverside.subsets = subsets;

    actions_serverside->action = action;
    actions_serverside->nactions = nactions;

    return (ierr);

}


//----------------------------------------------------------------------------------------------------------------------
// Identify the Index Range satisfying a small set of conditional operators

int idamserversubsetindices(char* operation, DIMS* dim, double value, unsigned int* subsetindices)
{

    int k, count = 0;

    // Scan the Array applying the Operation

    switch (dim->data_type) {

        case UDA_TYPE_DOUBLE: {
            auto p = (double*)dim->dim;
            if (STR_IEQUALS(operation, "eq") || operation[0] == '=' || STR_EQUALS(operation, "~=")) {
                for (k = 0; k < dim->dim_n; k++) if (p[k] == (double)value) subsetindices[count++] = k;
                if (count == 0 && STR_EQUALS(operation, "~=")) {
                    for (k = 0; k < dim->dim_n; k++)
                        if (fabs(p[k] - (double)value) <= DBL_EPSILON) {
                            subsetindices[count++] = k;
                        }
                    if (count == 0) {
                        int index = -1;
                        double delta, minvalue = fabs((double)value - p[0]);
                        for (k = 0; k < dim->dim_n; k++) {
                            delta = fabs((double)value - p[k]);
                            if (delta < minvalue) {                        // Look for the Single Nearest Value
                                minvalue = delta;
                                index = k;
                            }
                        }
                        if (index >= 0) {
                            count = 1;
                            subsetindices[0] = index;

                            if (index == 0 ||
                                index == dim->dim_n - 1) {                // Check not an end point by default
                                if (dim->dim_n > 1) {
                                    if (index == 0) {
                                        delta = fabs(p[1] - p[0]);
                                    } else {
                                        delta = fabs(p[dim->dim_n - 1] - p[dim->dim_n - 2]);
                                    }
                                    if (fabs((double)value - p[index]) > delta) count = 0;    // Suspect match!
                                }
                            }
                        }
                    }
                }
            } else {
                if (STR_IEQUALS(operation, "lt") || STR_EQUALS(operation, "<")) {
                    for (k = 0; k < dim->dim_n; k++) if (p[k] < (double)value) subsetindices[count++] = k;
                } else {
                    if (STR_IEQUALS(operation, "gt") || STR_EQUALS(operation, ">")) {
                        for (k = 0; k < dim->dim_n; k++) if (p[k] > (double)value) subsetindices[count++] = k;
                    } else {
                        if (STR_IEQUALS(operation, "le") || STR_EQUALS(operation, "<=")) {
                            for (k = 0; k < dim->dim_n; k++) if (p[k] <= (double)value) subsetindices[count++] = k;
                        } else {
                            if (STR_IEQUALS(operation, "ge") || STR_EQUALS(operation, ">=")) {
                                for (k = 0; k < dim->dim_n; k++) if (p[k] >= (double)value) subsetindices[count++] = k;
                            } else {
                                if (STR_IEQUALS(operation, "ne") || STR_EQUALS(operation, "!=") ||
                                    STR_EQUALS(operation, "!~=")) {
                                    if (strncmp(operation, "!~=", 3) != 0) {
                                        for (k = 0; k < dim->dim_n; k++)
                                            if (p[k] != (double)value) {
                                                subsetindices[count++] = k;
                                            }
                                    } else {
                                        int index = -1;
                                        double delta, minvalue = fabs((double)value - p[0]);
                                        for (k = 0; k < dim->dim_n; k++) {
                                            delta = fabs((double)value - p[k]);
                                            if (delta <
                                                minvalue) {                        // Look for the Single Nearest Value
                                                minvalue = delta;
                                                index = k;
                                            }
                                        }
                                        if (index >= 0) {
                                            for (k = 0; k < dim->dim_n; k++) {
                                                if (k != index) {
                                                    subsetindices[count++] = k;
                                                }            // Drop the single nearest value
                                            }
                                            if (index == 0 || index == dim->dim_n -
                                                                       1) {            // Check not an end point by default
                                                if (dim->dim_n > 1) {
                                                    if (index == 0) {
                                                        delta = fabs(p[1] - p[0]);
                                                    } else {
                                                        delta = fabs(p[dim->dim_n - 1] - p[dim->dim_n - 2]);
                                                    }
                                                    if (fabs((double)value - p[index]) > delta) {
                                                        count = 0;
                                                    }    // Suspect match!
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
                for (k = 0; k < dim->dim_n; k++) if (p[k] == (float)value) subsetindices[count++] = k;
                if (count == 0 && STR_EQUALS(operation, "~=")) {
                    for (k = 0; k < dim->dim_n; k++)
                        if (fabsf(p[k] - (float)value) <= FLT_EPSILON) {
                            subsetindices[count++] = k;
                        }
                    if (count == 0) {
                        int index = -1;
                        double delta, minvalue = fabsf((float)value - p[0]);
                        for (k = 0; k < dim->dim_n; k++) {
                            delta = fabsf((float)value - p[k]);
                            if (delta < minvalue) {                        // Look for the Single Nearest Value
                                minvalue = delta;
                                index = k;
                            }
                        }
                        if (index >= 0) {
                            count = 1;
                            subsetindices[0] = index;

                            if (index == 0 ||
                                index == dim->dim_n - 1) {                // Check not an end point by default
                                if (dim->dim_n > 1) {
                                    if (index == 0) {
                                        delta = fabsf(p[1] - p[0]);
                                    } else {
                                        delta = fabsf(p[dim->dim_n - 1] - p[dim->dim_n - 2]);
                                    }
                                    if (fabsf((float)value - p[index]) > delta) count = 0;    // Suspect match!
                                }
                            }
                        }
                    }
                }
            } else {
                if (STR_IEQUALS(operation, "lt") || STR_EQUALS(operation, "<")) {
                    for (k = 0; k < dim->dim_n; k++) if (p[k] < (float)value) subsetindices[count++] = k;
                } else {
                    if (STR_IEQUALS(operation, "gt") || STR_EQUALS(operation, ">")) {
                        for (k = 0; k < dim->dim_n; k++) if (p[k] > (float)value) subsetindices[count++] = k;
                    } else {
                        if (STR_IEQUALS(operation, "le") || STR_EQUALS(operation, "<=")) {
                            for (k = 0; k < dim->dim_n; k++) if (p[k] <= (float)value) subsetindices[count++] = k;
                        } else {
                            if (STR_IEQUALS(operation, "ge") || STR_EQUALS(operation, ">=")) {
                                for (k = 0; k < dim->dim_n; k++) if (p[k] >= (float)value) subsetindices[count++] = k;
                            } else {
                                if (STR_IEQUALS(operation, "ne") || STR_EQUALS(operation, "!=") ||
                                    STR_EQUALS(operation, "!~=")) {
                                    if (strncmp(operation, "!~=", 3) != 0) {
                                        for (k = 0; k < dim->dim_n; k++)
                                            if (p[k] != (float)value) {
                                                subsetindices[count++] = k;
                                            }
                                    } else {
                                        int index = -1;
                                        double delta, minvalue = fabsf((float)value - p[0]);
                                        for (k = 0; k < dim->dim_n; k++) {
                                            delta = fabsf((float)value - p[k]);
                                            if (delta <
                                                minvalue) {                        // Look for the Single Nearest Value
                                                minvalue = delta;
                                                index = k;
                                            }
                                        }
                                        if (index >= 0) {
                                            for (k = 0; k < dim->dim_n; k++) {
                                                if (k != index) {
                                                    subsetindices[count++] = k;
                                                }            // Drop the single nearest value
                                            }
                                            if (index == 0 || index == dim->dim_n -
                                                                       1) {            // Check not an end point by default
                                                if (dim->dim_n > 1) {
                                                    if (index == 0) {
                                                        delta = fabsf(p[1] - p[0]);
                                                    } else {
                                                        delta = fabsf(p[dim->dim_n - 1] - p[dim->dim_n - 2]);
                                                    }
                                                    if (fabsf((float)value - p[index]) > delta) {
                                                        count = 0;
                                                    }    // Suspect match!
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
                for (k = 0; k < dim->dim_n; k++) if (p[k] == (int)value) subsetindices[count++] = k;
            } else {
                if (STR_IEQUALS(operation, "lt") || STR_EQUALS(operation, "<")) {
                    for (k = 0; k < dim->dim_n; k++) if (p[k] < (int)value) subsetindices[count++] = k;
                } else {
                    if (STR_IEQUALS(operation, "gt") || STR_EQUALS(operation, ">")) {
                        for (k = 0; k < dim->dim_n; k++) if (p[k] > (int)value) subsetindices[count++] = k;
                    } else {
                        if (STR_IEQUALS(operation, "le") || STR_EQUALS(operation, "<=")) {
                            for (k = 0; k < dim->dim_n; k++) if (p[k] <= (int)value) subsetindices[count++] = k;
                        } else {
                            if (STR_IEQUALS(operation, "ge") || STR_EQUALS(operation, ">=")) {
                                for (k = 0; k < dim->dim_n; k++) if (p[k] >= (int)value) subsetindices[count++] = k;
                            } else {
                                if (STR_IEQUALS(operation, "ne") || STR_EQUALS(operation, "!=") ||
                                    STR_EQUALS(operation, "!~=")) {
                                    for (k = 0; k < dim->dim_n; k++)
                                        if (p[k] != (int)value) {
                                            subsetindices[count++] = k;
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


int idamserverNewDataArray2(DIMS* dims, int rank, int dimid,
                            char* data, int ndata, int data_type, int notoperation, int reverse,
                            int start, int end, int start1, int end1, int* n, void** newdata)
{

    int i, j, k, ierr = 0, rows, columns, newrows, newcols, count = 0;

    UDA_LOG(UDA_LOG_DEBUG, "Data Type: %d    Rank: %d\n", data_type, rank);

    *n = 0;

    switch (data_type) {

        case UDA_TYPE_FLOAT: {

            float* p, * dp;
            float** pa, ** dpa;

            // Allocate heap for the reshaped array

            if ((p = (float*)malloc(ndata * sizeof(float))) == nullptr) {
                ierr = 9999;
                addIdamError(CODEERRORTYPE, "idamserverNewDataArray", ierr,
                             "Unable to Allocate Heap memory");
                return ierr;
            }

            dp = (float*)data;        // the Originating Data Array

            // Reshape

            switch (rank) {
                case 1:
                    k = 0;
                    if (!reverse) {
                        for (i = start; i <= end; i++) p[k++] = dp[i];
                        if (notoperation) for (i = start1; i <= end1; i++) p[k++] = dp[i];
                    } else {
                        if (notoperation) for (i = end1; i >= start1; i--) p[k++] = dp[i];
                        for (i = end; i >= start; i--) p[k++] = dp[i];
                    }
                    *n = k;
                    *newdata = (void*)p;
                    break;

                case 2:

                    // Original Data

                    rows = dims[1].dim_n;
                    columns = dims[0].dim_n;
                    dpa = (float**)malloc(rows * sizeof(float*));
                    for (j = 0; j < rows; j++) dpa[j] = &dp[j * columns];

                    // Array for Reshaped Data

                    if (dimid == 0) {
                        newrows = dims[1].dim_n;
                        newcols = end - start + 1;
                        if (notoperation) newcols = newcols + end1 - start1 + 1;
                    } else {
                        newcols = dims[0].dim_n;
                        newrows = end - start + 1;
                        if (notoperation) newrows = newrows + end1 - start1 + 1;
                    }
                    pa = (float**)malloc(newrows * sizeof(float*));
                    for (j = 0; j < newrows; j++) pa[j] = &p[j * newcols];

                    // Reshape the Data

                    if (dimid == 0) {
                        for (j = 0; j < rows; j++) {
                            k = 0;
                            if (!reverse) {
                                for (i = start; i <= end; i++) {
                                    pa[j][k++] = dpa[j][i];
                                    count++;
                                }
                                if (notoperation) {
                                    for (i = start1; i <= end1; i++) {
                                        pa[j][k++] = dpa[j][i];
                                        count++;
                                    }
                                }
                            } else {
                                if (notoperation) {
                                    for (i = end1; i <= start1; i--) {
                                        pa[j][k++] = dpa[j][i];
                                        count++;
                                    }
                                }
                                for (i = end; i <= start; i--) {
                                    pa[j][k++] = dpa[j][i];
                                    count++;
                                }
                            }
                        }
                    } else {
                        k = 0;
                        if (!reverse) {
                            for (j = start; j <= end; j++) {
                                for (i = 0; i < columns; i++) {
                                    pa[k][i] = dpa[j][i];
                                    count++;
                                }
                                k++;
                            }
                            if (notoperation) {
                                for (j = start1; j <= end1; j++) {
                                    for (i = 0; i < columns; i++) {
                                        pa[k][i] = dpa[j][i];
                                        count++;
                                    }
                                    k++;
                                }
                            }
                        } else {
                            if (notoperation) {
                                for (j = end1; j <= start1; j--) {
                                    for (i = 0; i < columns; i++) {
                                        pa[k][i] = dpa[j][i];
                                        count++;
                                    }
                                    k++;
                                }
                            }
                            for (j = end; j <= start; j--) {
                                for (i = 0; i < columns; i++) {
                                    pa[k][i] = dpa[j][i];
                                    count++;
                                }
                                k++;
                            }
                        }
                    }

                    *newdata = (void*)&pa[0][0];
                    *n = count;
                    break;
            }
            break;
        }

        case UDA_TYPE_DOUBLE: {

            double* p, * dp;
            double** pa, ** dpa;

            // Allocate heap for the reshaped array

            if ((p = (double*)malloc(ndata * sizeof(double))) == nullptr) {
                ierr = 9999;
                addIdamError(CODEERRORTYPE, "idamserverNewDataArray", ierr,
                             "Unable to Allocate Heap memory");
                return ierr;
            }

            dp = (double*)data;        // the Originating Data Array

            // Reshape

            switch (rank) {
                case 1:
                    k = 0;
                    if (!reverse) {
                        for (i = start; i <= end; i++) p[k++] = dp[i];
                        if (notoperation) for (i = start1; i <= end1; i++) p[k++] = dp[i];
                    } else {
                        if (notoperation) for (i = end1; i >= start1; i--) p[k++] = dp[i];
                        for (i = end; i >= start; i--) p[k++] = dp[i];
                    }
                    *n = k;
                    *newdata = (void*)p;
                    break;

                case 2:

                    // Original Data

                    rows = dims[1].dim_n;
                    columns = dims[0].dim_n;
                    dpa = (double**)malloc(rows * sizeof(double*));
                    for (j = 0; j < rows; j++) dpa[j] = &dp[j * columns];

                    // Array for Reshaped Data

                    if (dimid == 0) {
                        newrows = dims[1].dim_n;
                        newcols = end - start + 1;
                        if (notoperation) newcols = newcols + end1 - start1 + 1;
                    } else {
                        newcols = dims[0].dim_n;
                        newrows = end - start + 1;
                        if (notoperation) newrows = newrows + end1 - start1 + 1;
                    }
                    pa = (double**)malloc(newrows * sizeof(double*));
                    for (j = 0; j < newrows; j++) pa[j] = &p[j * newcols];

                    // Reshape the Data

                    if (dimid == 0) {
                        for (j = 0; j < rows; j++) {
                            k = 0;
                            if (!reverse) {
                                for (i = start; i <= end; i++) {
                                    pa[j][k++] = dpa[j][i];
                                    count++;
                                }
                                if (notoperation) {
                                    for (i = start1; i <= end1; i++) {
                                        pa[j][k++] = dpa[j][i];
                                        count++;
                                    }
                                }
                            } else {
                                if (notoperation) {
                                    for (i = end1; i <= start1; i--) {
                                        pa[j][k++] = dpa[j][i];
                                        count++;
                                    }
                                }
                                for (i = end; i <= start; i--) {
                                    pa[j][k++] = dpa[j][i];
                                    count++;
                                }
                            }
                        }
                    } else {
                        k = 0;
                        if (!reverse) {
                            for (j = start; j <= end; j++) {
                                for (i = 0; i < columns; i++) {
                                    pa[k][i] = dpa[j][i];
                                    count++;
                                }
                                k++;
                            }
                            if (notoperation) {
                                for (j = start1; j <= end1; j++) {
                                    for (i = 0; i < columns; i++) {
                                        pa[k][i] = dpa[j][i];
                                        count++;
                                    }
                                    k++;
                                }
                            }
                        } else {
                            if (notoperation) {
                                for (j = end1; j <= start1; j--) {
                                    for (i = 0; i < columns; i++) {
                                        pa[k][i] = dpa[j][i];
                                        count++;
                                    }
                                    k++;
                                }
                            }
                            for (j = end; j <= start; j--) {
                                for (i = 0; i < columns; i++) {
                                    pa[k][i] = dpa[j][i];
                                    count++;
                                }
                                k++;
                            }
                        }
                    }

                    *newdata = (void*)&pa[0][0];
                    *n = count;
                    break;
            }
            break;
        }

        case UDA_TYPE_INT: {

            int* p, * dp;
            int** pa, ** dpa;

            // Allocate heap for the reshaped array

            if ((p = (int*)malloc(ndata * sizeof(int))) == nullptr) {
                ierr = 9999;
                addIdamError(CODEERRORTYPE, "idamserverNewDataArray", ierr,
                             "Unable to Allocate Heap memory");
                return ierr;
            }

            dp = (int*)data;        // the Originating Data Array

            // Reshape

            switch (rank) {
                case 1:
                    k = 0;
                    if (!reverse) {
                        for (i = start; i <= end; i++) p[k++] = dp[i];
                        if (notoperation) for (i = start1; i <= end1; i++) p[k++] = dp[i];
                    } else {
                        if (notoperation) for (i = end1; i >= start1; i--) p[k++] = dp[i];
                        for (i = end; i >= start; i--) p[k++] = dp[i];
                    }
                    *n = k;
                    *newdata = (void*)p;
                    break;

                case 2:

                    // Original Data

                    rows = dims[1].dim_n;
                    columns = dims[0].dim_n;
                    dpa = (int**)malloc(rows * sizeof(int*));
                    for (j = 0; j < rows; j++) dpa[j] = &dp[j * columns];

                    // Array for Reshaped Data

                    if (dimid == 0) {
                        newrows = dims[1].dim_n;
                        newcols = end - start + 1;
                        if (notoperation) newcols = newcols + end1 - start1 + 1;
                    } else {
                        newcols = dims[0].dim_n;
                        newrows = end - start + 1;
                        if (notoperation) newrows = newrows + end1 - start1 + 1;
                    }
                    pa = (int**)malloc(newrows * sizeof(int*));
                    for (j = 0; j < newrows; j++) pa[j] = &p[j * newcols];

                    // Reshape the Data

                    if (dimid == 0) {
                        for (j = 0; j < rows; j++) {
                            k = 0;
                            if (!reverse) {
                                for (i = start; i <= end; i++) {
                                    pa[j][k++] = dpa[j][i];
                                    count++;
                                }
                                if (notoperation) {
                                    for (i = start1; i <= end1; i++) {
                                        pa[j][k++] = dpa[j][i];
                                        count++;
                                    }
                                }
                            } else {
                                if (notoperation) {
                                    for (i = end1; i <= start1; i--) {
                                        pa[j][k++] = dpa[j][i];
                                        count++;
                                    }
                                }
                                for (i = end; i <= start; i--) {
                                    pa[j][k++] = dpa[j][i];
                                    count++;
                                }
                            }
                        }
                    } else {
                        k = 0;
                        if (!reverse) {
                            for (j = start; j <= end; j++) {
                                for (i = 0; i < columns; i++) {
                                    pa[k][i] = dpa[j][i];
                                    count++;
                                }
                                k++;
                            }
                            if (notoperation) {
                                for (j = start1; j <= end1; j++) {
                                    for (i = 0; i < columns; i++) {
                                        pa[k][i] = dpa[j][i];
                                        count++;
                                    }
                                    k++;
                                }
                            }
                        } else {
                            if (notoperation) {
                                for (j = end1; j <= start1; j--) {
                                    for (i = 0; i < columns; i++) {
                                        pa[k][i] = dpa[j][i];
                                        count++;
                                    }
                                    k++;
                                }
                            }
                            for (j = end; j <= start; j--) {
                                for (i = 0; i < columns; i++) {
                                    pa[k][i] = dpa[j][i];
                                    count++;
                                }
                                k++;
                            }
                        }
                    }

                    *newdata = (void*)&pa[0][0];
                    *n = count;
                    break;
            }
            break;
        }

        default:
            ierr = 9999;
            addIdamError(CODEERRORTYPE, "idamserverNewDataArray", ierr,
                         "Only Float, Double and 32 bit Signed Integer Numerical Types can be Subset at this time!");

            UDA_LOG(UDA_LOG_ERROR,
                    "ERROR - Only Float, Double and 32 bit Signed Integer Numerical Types can be Subset at this time!\n");
            UDA_LOG(UDA_LOG_ERROR, "Data Type: %d    Rank: %d\n", data_type, rank);

            return ierr;
    }

    return ierr;
}



