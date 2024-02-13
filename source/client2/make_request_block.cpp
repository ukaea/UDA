#include "make_request_block.hpp"
#include "client_environment.hpp"

#include <boost/format.hpp>
#include <string>

#include "clientserver/errorLog.h"
#include "clientserver/expand_path.h"
#include "clientserver/initStructs.h"
#include "clientserver/udaErrors.h"
#include "logging/logging.h"

using namespace uda::client_server;

namespace
{
int make_request_data(const Environment* environment, const char* data_object, const char* data_source,
                      RequestData* request)
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

    strcpy(request->api_delim, environment->api_delim); // Server needs to know how to parse the arguments

    //------------------------------------------------------------------------------------------------------------------
    /* If the default ARCHIVE and/or DEVICE is overridden by local environment variables and the arguments do not
     * contain either an archive or device then prefix
     *
     * These environment variables are legacy and not used by the server
     */

    if (environment->api_device[0] != '\0' && strstr(request->source, request->api_delim) == nullptr) {
        auto source =
            (boost::format("%1%%2%%3%") % environment->api_device % request->api_delim % request->source).str();
        if (source.length() >= STRING_LENGTH) {
            UDA_THROW_ERROR(SOURCE_ARG_TOO_LONG,
                            "The Data Source Argument, prefixed with the Device Name, is too long!");
        }
        strcpy(request->source, source.c_str());
    }

    if (environment->api_archive[0] != '\0' && strstr(request->signal, request->api_delim) == nullptr) {
        auto signal =
            (boost::format("%1%%2%%3%") % environment->api_archive % request->api_delim % request->signal).str();
        if (signal.length() >= STRING_LENGTH) {
            UDA_THROW_ERROR(SIGNAL_ARG_TOO_LONG,
                            "The Signal/Data Object Argument, prefixed with the Archive Name, is too long!");
        }
        strcpy(request->signal, signal.c_str());
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
            expand_file_path(request->path, environment);
        }
    } else {
        if (strchr(test, '(') == nullptr && strchr(test, ')') == nullptr) {
            // Prefixed and not a function call
            int ldelim = (int)strlen(request->api_delim);
            strcpy(request->path, &test[ldelim]);
            expand_file_path(request->path, environment);
        }
    }

    return 0;
}
} // namespace

int uda::client::make_request_block(const Environment* environment, const char** signals, const char** sources,
                                    int count, RequestBlock* request_block)
{
    request_block->num_requests = (int)count;
    request_block->requests = (RequestData*)malloc(count * sizeof(RequestData));

    int err = 0;
    for (int i = 0; i < count; ++i) {
        RequestData* request = &request_block->requests[i];
        init_request_data(request);
        if ((err = make_request_data(environment, signals[i], sources[i], request))) {
            return err;
        }
    }

    return err;
}
