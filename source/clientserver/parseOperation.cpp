#include "parseOperation.h"

#include <boost/algorithm/string.hpp>
#include <string>
#include <vector>

#include "errorLog.h"
#include "stringUtils.h"

using namespace uda::client_server;

int uda::client_server::parse_operation(SUBSET* sub)
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

    for (int i = 0; i < sub->nbound; i++) {

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
                UDA_THROW_ERROR(9999, "Server Side Operation Syntax Error: Too Many :");
            }

            sub->isindex[i] = true;
            sub->ubindex[i] = {.init = false, .value = 0};
            sub->lbindex[i] = {.init = false, .value = 0};
            sub->stride[i] = {.init = false, .value = 0};
            strcpy(sub->operation[i], ":");

            if (!tokens[0].empty() && tokens[0] != "#" && tokens[0] != "*") {
                size_t n = 0;
                long num = std::stol(tokens[0], &n, 10);
                if (n != tokens[0].size()) {
                    UDA_THROW_ERROR(9999, "Server Side Operation Syntax Error: Invalid Lower Index Bound");
                }
                sub->lbindex[i] = {.init = true, .value = num};
            }

            if (!tokens[1].empty() && tokens[1] != "#" && tokens[1] != "*") {
                size_t n = 0;
                long num = std::stol(tokens[1], &n, 10);
                if (n != tokens[1].size()) {
                    UDA_THROW_ERROR(9999, "Server Side Operation Syntax Error: Invalid Upper Index Bound");
                }
                sub->ubindex[i] = {.init = true, .value = num};
            }

            if (tokens.size() == 3 && !tokens[2].empty()) {
                size_t n = 0;
                long num = std::stol(tokens[2], &n, 10);
                if (n != tokens[2].size()) {
                    UDA_THROW_ERROR(9999, "Server Side Operation Syntax Error: Invalid Stride");
                }
                sub->stride[i] = {.init = true, .value = num};
            }
        } else if (operation == "*") {
            // Ignore this Dimension
            sub->isindex[i] = true;
            sub->ubindex[i] = {.init = false, .value = 0};
            sub->lbindex[i] = {.init = true, .value = 0};
            sub->stride[i] = {.init = false, .value = 0};
        } else if (operation == "#") {
            // Last Value in Dimension
            sub->isindex[i] = true;
            sub->ubindex[i] = {.init = false, .value = 0};
            sub->lbindex[i] = {.init = true, .value = -1};
            sub->stride[i] = {.init = false, .value = 0};
        } else {
            size_t n = 0;
            long num = std::stol(operation, &n, 10);

            if (n != operation.size()) {
                UDA_THROW_ERROR(9999, "Server Side Operation Syntax Error: Single Index Bound");
            }

            sub->isindex[i] = true;
            sub->ubindex[i] = {.init = true, .value = num + 1};
            sub->lbindex[i] = {.init = true, .value = num};
            sub->stride[i] = {.init = false, .value = 0};
            strcpy(sub->operation[i], ":");
        }
    }

    return 0;
}
