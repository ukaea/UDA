//--------------------------------------------------------------------------------------------------------------------
// Parse server-side subsetting requests (SS::SUBSET(...)) into actions
//
// Server only: uses the server environment and plugin list. The subset operation itself is in serverSubsetData.cpp.
//--------------------------------------------------------------------------------------------------------------------

#include "serverParseServerSide.h"
#include "getServerEnvironment.h"
#include "clientserver/parseOperation.h"
#include "plugins/udaPlugin.h"
#include "clientserver/makeRequestBlock.h"

#include <string>
#if defined(__GNUC__)
#  include <strings.h>
#else
#  define strncasecmp _strnicmp
#endif

#include <logging/logging.h>
#include <clientserver/errorLog.h>
#include <clientserver/initStructs.h>
#include <clientserver/stringUtils.h>
#include <boost/algorithm/string.hpp>

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
        UDA_THROW_ERROR(9999, "Syntax Error: The Signal Name has no Terminating Quotation character!");
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
        UDA_THROW_ERROR(9999, "Syntax Error: No Comma after the Signal Name");
    }

    size_t lbracket_pos = remainder.find('[');

    if (lbracket_pos == std::string::npos) {
        // Locate the Operation
        UDA_THROW_ERROR(9999, "Syntax Error: No [ enclosing the Operation");
    }

    size_t rbracket_pos = remainder.find(']', lbracket_pos);

    if (rbracket_pos == std::string::npos) {
        UDA_THROW_ERROR(9999, "Syntax Error: No ] enclosing the Operation ");
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
        UDA_THROW_ERROR(9999, "Unable to Allocate Heap memory");
    }

    initAction(&action[nactions - 1]);

    action[nactions - 1].actionType = UDA_SERVER_SIDE_TYPE;
    action[nactions - 1].inRange = 1;
    action[nactions - 1].actionId = nactions;

    initServerside(&action[nactions - 1].serverside);

    int nsubsets = 1;
    if ((subsets = (SUBSET*)malloc(sizeof(SUBSET))) == nullptr) {
        UDA_THROW_ERROR(9999, "Unable to Allocate Heap memory");
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
        UDA_THROW_ERROR(9999, "The number of Dimensional Operations exceeds the Internal Limit");
    }

    for (auto& token : tokens) {
        boost::trim(token);

        subsets[nsubsets - 1].dimid[nbound] = nbound;
        strcpy(subsets[nsubsets - 1].operation[nbound], token.c_str());

        nbound++;
    }

    subsets[nsubsets - 1].nbound = nbound;

    //-------------------------------------------------------------------------------------------------------------
    // Extract the Value Component from each separate Operation
    // =0.15,!=0.15,<=0.05,>=0.05,!<=0.05,!>=0.05,<0.05,>0.05,0:25,25:0,25,*,25:,:25
    //
    // Identify Three Types of Operations:
    //    A) Contains the characters: =,>, <, !, ~
    //    B) : or Integer Value
    //    C) * or #
    //

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
