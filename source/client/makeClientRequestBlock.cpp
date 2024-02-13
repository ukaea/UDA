//------------------------------------------------------------------------------------------------------------------
/*!
Interprets the API arguments and assembles a Request data structure.

@param data_object Signal or data object name
@param data_source Data Source or Experiment Number
@param request_block  Returned data access Request Data Structure

@returns An integer Error Code: If non zero, a problem occured.
*/
//------------------------------------------------------------------------------------------------------------------

#include "makeClientRequestBlock.h"
#include "getEnvironment.h"

#include <cstdlib>
#ifdef __GNUC__
#  include <strings.h>
#elif defined(_WIN32)
#  define strcasecmp _stricmp
#endif

#include <string>

#include "clientserver/errorLog.h"
#include "clientserver/expand_path.h"
#include "clientserver/initStructs.h"
#include "clientserver/makeRequestBlock.h"
#include "clientserver/stringUtils.h"
#include "clientserver/udaErrors.h"
#include "logging/logging.h"
#include <fmt/format.h>

using namespace uda::client_server;
using namespace uda::client;

int make_request_data(const char* data_object, const char* data_source, REQUEST_DATA* request)
{
    //------------------------------------------------------------------------------------------------------------------
    //! Test Input Arguments comply with string length limits, then copy to the request structure without modification

    if (strlen(data_object) >= MAXMETA) {
        UDA_THROW_ERROR(SIGNAL_ARG_TOO_LONG, "The Signal/Data Object Argument string is too long!");
    } else {
        strcpy(request->signal, data_object); // Passed to the server without modification
    }

    if (strlen(data_source) >= STRING_LENGTH) {
        UDA_THROW_ERROR(SOURCE_ARG_TOO_LONG, "The Data Source Argument string is too long!");
    } else {
        strcpy(request->source, data_source); // Passed to the server without modification
    }

    //------------------------------------------------------------------------------------------------------------------
    /* Signal and source arguments use a prefix to identify archive or device names, file formats or server protocols.
     * These prefixes are attached to the main signal or source details using a delimiting string, e.g. "::".
     * This delimiting string can be defined by the user via an environment variable "UDA_API_DELIM".
     * This must be passed to the server as it needs to separate the prefix from the main component in order to
     * interpret the data access request.
     */

    ENVIRONMENT* environment = getIdamClientEnvironment();

    strcpy(request->api_delim, environment->api_delim); // Server needs to know how to parse the arguments

    //------------------------------------------------------------------------------------------------------------------
    /* If the default ARCHIVE and/or DEVICE is overridden by local environment variables and the arguments do not
     * contain either an archive or device then prefix
     *
     * These environment variables are legacy and not used by the server
     */

    if (environment->api_device[0] != '\0' && strstr(request->source, request->api_delim) == nullptr) {
        int lstr =
            (int)strlen(request->source) + (int)strlen(environment->api_device) + (int)strlen(request->api_delim);
        if (lstr >= STRING_LENGTH) {
            UDA_THROW_ERROR(SOURCE_ARG_TOO_LONG,
                            "The Data Source Argument, prefixed with the Device Name, is too long!");
        }
        std::string test = fmt::format("{}{}{}", environment->api_device, request->api_delim, request->source);
        strcpy(request->source, test.c_str());
    }

    if (environment->api_archive[0] != '\0' && strstr(request->signal, request->api_delim) == nullptr) {
        int lstr =
            (int)strlen(request->signal) + (int)strlen(environment->api_archive) + (int)strlen(request->api_delim);
        if (lstr >= STRING_LENGTH) {
            UDA_THROW_ERROR(SIGNAL_ARG_TOO_LONG,
                            "The Signal/Data Object Argument, prefixed with the Archive Name, is too long!");
        }
        std::string test = fmt::format("{}{}{}", environment->api_archive, request->api_delim, request->signal);
        strcpy(request->signal, test.c_str());
    }

    //------------------------------------------------------------------------------------------------------------------
    /* The source argument can contain Directory paths to private files. These paths may use environment variables or
     * ./ or ../ or beginning /scratch (local workstation directory). Consequently, these paths must be expanded before
     * dispatch to the server.
     *
     * These private files must be seen by the server - the file directory must be accessible with read permission.
     *
     * Expanded paths are passed to the server via the path component of the Request data structure.
     *
     * Server side function requests, passed via the source argument, must contain the full path name and only use
     * environment variables local and known to the server. These paths are Not expanded before dispatch to the server.
     *
     * Server side function requests are identified by testing for a pair of parenthesis characters. This is very
     * primitive and needs improvement. (Server side function requests is an under-developed component of UDA.)
     *
     * Any attempted expansion of a server URL will result in a meaningless path. These are ignored by the server.
     * *** the original source is always retained !
     */

    // Path expansion disabled - applications must provide the full path to data resources.
    // XXXX::12345        shot number
    // XXXX::12345/a     keyword or pass number
    // XXXX::12345/a,b,c    keywords or substitution values
    // XXXX::12345/a=b,c=d    name-value pairs
    // XXXX::a
    // XXXX::a,b,c
    // XXXX::a=b,c=d
    // XXXX::/path/to/data/resource

    char* test = nullptr;
    if ((test = strstr(request->source, request->api_delim)) == nullptr) {
        if (strchr(request->source, '(') == nullptr && strchr(request->source, ')') == nullptr) {
            // source is not a function call
            strcpy(request->path, request->source);
            expand_file_path(request->path, getIdamClientEnvironment());
        }
    } else {
        if (strchr(test, '(') == nullptr && strchr(test, ')') == nullptr) {
            // Prefixed and not a function call
            int ldelim = (int)strlen(request->api_delim);
            strcpy(request->path, &test[ldelim]);
            expand_file_path(request->path, getIdamClientEnvironment());
        }
    }

    return 0;
}

int uda::client::makeClientRequestBlock(const char** signals, const char** sources, int count,
                                        REQUEST_BLOCK* request_block)
{
    request_block->num_requests = (int)count;
    request_block->requests = (REQUEST_DATA*)malloc(count * sizeof(REQUEST_DATA));

    int err = 0;
    for (int i = 0; i < count; ++i) {
        REQUEST_DATA* request = &request_block->requests[i];
        init_request_data(request);
        if ((err = make_request_data(signals[i], sources[i], request))) {
            return err;
        }
    }

    return err;
}

void uda::client::freeClientRequestBlock(REQUEST_BLOCK* request_block)
{
    if (request_block != nullptr && request_block->requests != nullptr) {
        for (int i = 0; i < request_block->num_requests; i++) {
            free_name_value_list(&request_block->requests[i].nameValueList);
            freeClientPutDataBlockList(&request_block->requests[i].putDataBlockList);
        }
        free(request_block->requests);
        request_block->requests = nullptr;
    }
}

int shotRequestTest(const char* source)
{
    // Return 1 (TRUE) if the source is shot nuumber based , 0 (FALSE) otherwise

    char* token = nullptr;
    char work[STRING_LENGTH];

    if (source[0] == '\0') {
        return 0;
    }
    if (source[0] == '/') {
        return 0; // Directory based data
    }

    //------------------------------------------------------------------------------
    // Check if the source has one of these forms:

    // pulse        plasma shot number - an integer
    // pulse/pass        include a pass or sequence number - this may be a text based component, e.g. LATEST

    if (is_number((char*)source)) {
        return 1; // The source an integer number
    }

    strcpy(work, source);

    if ((token = strtok(work, "/")) != nullptr) { // Tokenise the remaining string
        if (is_number(token)) {
            return 1; // Is the First token an integer number?
        }
    }

    return 0;
}
