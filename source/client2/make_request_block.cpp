#include "make_request_block.hpp"

#include <boost/format.hpp>
#include <string>

#include "clientserver/error_log.h"
#include "clientserver/expand_path.h"
#include "clientserver/init_structs.h"
#include "clientserver/uda_errors.h"
#include "config/config.h"

using namespace uda::client_server;
using namespace uda::config;

namespace
{
int make_request_data(std::vector<UdaError>& error_stack, const Config& config, const char* data_object, const char* data_source, RequestData* request)
{
    //------------------------------------------------------------------------------------------------------------------
    //! Test Input Arguments comply with string length limits, then copy to the request structure without modification

    if (strlen(data_object) >= MaxMeta) {
        UDA_THROW_ERROR(error_stack, (int)RequestError::SignalArgTooLong, "The Signal/Data Object Argument string is too long!");
    } else {
        strcpy(request->signal, data_object); // Passed to the server without modification
    }

    if (strlen(data_source) >= StringLength) {
        UDA_THROW_ERROR(error_stack, (int)RequestError::SourceArgTooLong, "The Data Source Argument string is too long!");
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

    auto api_delim = config.get("request.delim").as_or_default<std::string>({});

    strcpy(request->api_delim, api_delim.c_str()); // Server needs to know how to parse the arguments

    //------------------------------------------------------------------------------------------------------------------
    /* If the default ARCHIVE and/or DEVICE is overridden by local environment variables and the arguments do not
     * contain either an archive or device then prefix
     *
     * These environment variables are legacy and not used by the server
     */

    auto device = config.get("request.default_device").as_or_default<std::string>({});
    auto archive = config.get("request.default_archive").as_or_default<std::string>({});

    if (!device.empty() && strstr(request->source, request->api_delim) == nullptr) {
        auto source = fmt::format("{}{}{}", device, request->api_delim, request->source);
        if (source.length() >= StringLength) {
            UDA_THROW_ERROR(error_stack, (int)RequestError::SourceArgTooLong,
                            "The Data Source Argument, prefixed with the Device Name, is too long!");
        }
        strcpy(request->source, source.c_str());
    }

    if (!archive.empty() && strstr(request->signal, request->api_delim) == nullptr) {
        auto signal = fmt::format("{}{}{}", archive, request->api_delim, request->signal);
        if (signal.length() >= StringLength) {
            UDA_THROW_ERROR(error_stack, (int)RequestError::SignalArgTooLong,
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
            expand_file_path(config, request->path);
        }
    } else {
        if (strchr(test, '(') == nullptr && strchr(test, ')') == nullptr) {
            // Prefixed and not a function call
            int ldelim = (int)strlen(request->api_delim);
            strcpy(request->path, &test[ldelim]);
            expand_file_path(config, request->path);
        }
    }

    return 0;
}
} // namespace

int uda::client::make_request_block(std::vector<UdaError>& error_stack, const Config& config,
                                    const char** signals, const char** sources, int count, RequestBlock* request_block)
{
    request_block->num_requests = count;
    request_block->requests = (RequestData*)malloc(count * sizeof(RequestData));

    int err = 0;
    for (int i = 0; i < count; ++i) {
        RequestData* request = &request_block->requests[i];
        init_request_data(request);
        if ((err = make_request_data(error_stack, config, signals[i], sources[i], request))) {
            return err;
        }
    }

    return err;
}
