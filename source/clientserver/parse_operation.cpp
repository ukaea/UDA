#include "parse_operation.h"

#include <boost/algorithm/string.hpp>
#include <string>
#include <vector>

#include "error_log.h"
#include "logging/logging.h"

using namespace uda::client_server;
using namespace uda::logging;

int uda::client_server::parse_operation(std::vector<UdaError>& error_stack, Subset* sub)
{
    //-------------------------------------------------------------------------------------------------------------
    // Extract the Value Component from each separate Operation
    // =0.15,!=0.15,<=0.05,>=0.05,!<=0.05,!>=0.05,<0.05,>0.05,0:25,25:0,25,*,25:,:25, 25:#
    //
    // Identify Three Types of Operations:
    //    A) Contains the characters: =,>, <, !, ~
    //    B) : or Integer Value
    //    C) * or #  (* => ignore this dimension; # => Last Value in array)
    //
    // If the operation string is enclosed in [ ] then ignore these

    for (int i = 0; i < sub->n_bound; i++) {

        std::string operation = sub->operation[i];

        boost::trim_left_if(operation, [](char c) { return c == '['; });
        boost::trim_right_if(operation, [](char c) { return c == ']'; });

        if (boost::starts_with(operation, "[")) {
            operation = operation.substr(1);
        }

        if (boost::ends_with(operation, "]")) {
            operation = operation.substr(0, operation.size() - 1);
        }

        boost::trim(operation);

        if (boost::contains(operation, ":")) {
            std::vector<std::string> tokens;
            boost::split(tokens, operation, boost::is_any_of(":"), boost::token_compress_off);

            if (tokens.size() < 2 || tokens.size() > 3) {
                UDA_THROW_ERROR(error_stack, 9999, "Server Side Operation Syntax Error: Too Many :");
            }

            sub->is_index[i] = true;
            sub->ubindex[i] = {.init = false, .value = 0};
            sub->lbindex[i] = {.init = false, .value = 0};
            sub->stride[i] = {.init = false, .value = 0};
            strcpy(sub->operation[i], ":");

            if (!tokens[0].empty() && tokens[0] != "#" && tokens[0] != "*") {
                size_t n = 0;
                long num = std::stol(tokens[0], &n, 10);
                if (n != tokens[0].size()) {
                    UDA_THROW_ERROR(error_stack, 9999, "Server Side Operation Syntax Error: Invalid Lower Index Bound");
                }
                sub->lbindex[i] = {.init = true, .value = num};
            }

            if (!tokens[1].empty() && tokens[1] != "#" && tokens[1] != "*") {
                size_t n = 0;
                long num = std::stol(tokens[1], &n, 10);
                if (n != tokens[1].size()) {
                    UDA_THROW_ERROR(error_stack, 9999, "Server Side Operation Syntax Error: Invalid Upper Index Bound");
                }
                sub->ubindex[i] = {.init = true, .value = num};
            }

            if (tokens.size() == 3 && !tokens[2].empty()) {
                size_t n = 0;
                long num = std::stol(tokens[2], &n, 10);
                if (n != tokens[2].size()) {
                    UDA_THROW_ERROR(error_stack, 9999, "Server Side Operation Syntax Error: Invalid Stride");
                }
                sub->stride[i] = {.init = true, .value = num};
            }
        } else if (operation == "*") {
            // Ignore this Dimension
            sub->is_index[i] = true;
            sub->ubindex[i] = {.init = false, .value = 0};
            sub->lbindex[i] = {.init = true, .value = 0};
            sub->stride[i] = {.init = false, .value = 0};
        } else if (operation == "#") {
            // Last Value in Dimension
            sub->is_index[i] = true;
            sub->ubindex[i] = {.init = false, .value = 0};
            sub->lbindex[i] = {.init = true, .value = -1};
            sub->stride[i] = {.init = false, .value = 0};
        } else {
            size_t n = 0;
            long num = std::stol(operation, &n, 10);

            if (n != operation.size()) {
                UDA_THROW_ERROR(error_stack, 9999, "Server Side Operation Syntax Error: Single Index Bound");
            }

            sub->is_index[i] = true;
            sub->ubindex[i] = {.init = true, .value = num + 1};
            sub->lbindex[i] = {.init = true, .value = num};
            sub->stride[i] = {.init = false, .value = 0};
            strcpy(sub->operation[i], ":");
        }
    }

    return 0;
}

void uda::client_server::init_subset(Subset* sub) {
    for (int i = 0; i < MaxDataRank; i++) {
        sub->bound[i] = 0.0;                // Subsetting Float Bounds
        sub->ubindex[i] = { .init = false, .value = 0 };                // Subsetting Integer Bounds (Upper Index)
        sub->lbindex[i] = { .init = false, .value = 0 };                // Lower Index
        sub->stride[i] = { .init = false, .value = 0 };               // Stride
        sub->is_index[i] = false;                // Flag the Bound is an Integer Type
        sub->dim_id[i] = -1;                // Dimension IDs
        sub->operation[i][0] = '\0';            // Subsetting Operations
    }
    sub->data_signal[0] = '\0';                // Data to Read
    sub->member[0] = '\0';                // Structure Member to target
    sub->function[0] = '\0';                // Name of simple function to apply
    sub->n_bound = 0;                // The number of Subsetting Operations
    sub->reform = 0;                // reduce the Rank if a subsetted dimension has length 1
    sub->order = -1;                // Explicitly set the order of the time dimension if >= 0
}

void uda::client_server::print_subset(const Subset& subset) {
    UDA_LOG(UDA_LOG_DEBUG, "SUBSET\n");
    UDA_LOG(UDA_LOG_DEBUG, "Number of Sub-setting Operations: %d\n", subset.n_bound);
    UDA_LOG(UDA_LOG_DEBUG, "Reform?                         : %d\n", subset.reform);
    UDA_LOG(UDA_LOG_DEBUG, "Member                          : %s\n", subset.member);
    UDA_LOG(UDA_LOG_DEBUG, "Function                        : %s\n", subset.function);
    UDA_LOG(UDA_LOG_DEBUG, "Order                           : %d\n", subset.order);
    UDA_LOG(UDA_LOG_DEBUG, "Signal                          : %s\n", subset.data_signal);
    for (int j = 0; j < subset.n_bound; j++) {
        UDA_LOG(UDA_LOG_DEBUG, "Bounding Value : %e\n", subset.bound[j]);
        UDA_LOG(UDA_LOG_DEBUG, "Operation      : %s\n", subset.operation[j]);
        UDA_LOG(UDA_LOG_DEBUG, "Dimension ID   : %d\n", subset.dim_id[j]);
    }
}
