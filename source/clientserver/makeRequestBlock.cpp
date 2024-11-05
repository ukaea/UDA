#include "makeRequestBlock.h"

#include <boost/algorithm/string.hpp>
#include <cerrno>
#include <uda/plugins.h>
#include <vector>

#if defined(__GNUC__)
#  include <unistd.h>
#endif

#if defined(__GNUC__)
#  include <libgen.h>
#endif

#include "logging/logging.h"
#include <fmt/format.h>

#include "errorLog.h"
#include "parseXML.h"
#include "common/stringUtils.h"
#include "udaErrors.h"
#include "udaStructs.h"
#include "plugins.h"
#include "config/config.h"

// TODO: remove this!
#include "server/serverPlugin.h"

#if !defined(__GNUC__) && defined(_WIN32)
#  include <direct.h>
#  define strcasecmp _stricmp
#  define strncasecmp _strnicmp
#  define getcwd _getcwd
#  define chdir _chdir
#endif

using namespace uda::client_server;
using namespace uda::logging;

using namespace std::string_literals;

static void extract_function_name(const char* str, RequestData* request);

static int source_file_format_test(const uda::config::Config& config, const char* source, RequestData* request,
                                   const std::vector<PluginData>& pluginList);

static int extract_archive(const uda::config::Config& config, RequestData* request, int reduceSignal);

static int generic_request_test(const char* source, RequestData* request);

static int extract_subset(RequestData* request);

namespace uda
{
struct NameValue {
    std::string pair;
    std::string name;
    std::string value;
};

std::vector<uda::NameValue> name_value_pairs(std::string_view input, bool strip);
} // namespace uda

static int find_plugin_id_by_format(const std::string& name, const std::vector<PluginData>& plugin_list)
{
    auto lower_name = boost::to_lower_copy(name);
    size_t id = 0;
    for (const auto& plugin : plugin_list) {
        if (plugin.name == lower_name) {
            return id;
        }
        ++id;
    }
    return -1;
}

int uda::client_server::make_request_data(const config::Config& config, RequestData* request, const std::vector<PluginData>& pluginList)
{
    int ldelim;
    int err = 0;
    char work[MaxMeta];
    char work2[MaxMeta];
    unsigned short strip = 1; // Remove enclosing quotes from name value pairs

    UDA_LOG(UDA_LOG_DEBUG, "Source Argument")

    //------------------------------------------------------------------------------
    // Always use the client's delimiting string if provided, otherwise use the default delimiter

    auto delim = config.get("request.delim").as_or_default<std::string>({});
    if ((ldelim = (int)strlen(request->api_delim)) == 0) {
        strcpy(request->api_delim, delim.c_str());
        ldelim = (int)strlen(request->api_delim);
    }

    //------------------------------------------------------------------------------
    // Start with ignorance about which plugin to use

    request->request = (int)Request::ReadUnknown;

    //------------------------------------------------------------------------------
    // Check there is something to work with!

    auto archive = config.get("request.default_archive").as_or_default(""s);
    auto device = config.get("request.default_device").as_or_default(""s);

    snprintf(work, MaxMeta, "%s%s", archive.c_str(), delim.c_str()); // default archive
    snprintf(work2, MaxMeta, "%s%s", device.c_str(), delim.c_str()); // default device

    left_trim_string(request->signal);
    trim_string(request->signal);
    left_trim_string(request->source);
    trim_string(request->source);

    bool no_source = (request->source[0] == '\0' ||                            // no source
                     STR_IEQUALS(request->source, device.c_str()) || // default device name
                     STR_IEQUALS(request->source, work2));                    // default device name + delimiting string

    if ((request->signal[0] == '\0' || STR_IEQUALS(request->signal, work)) && no_source) {
        UDA_THROW_ERROR(999, "Neither Data Object nor Source specified!")
    }

    //------------------------------------------------------------------------------
    // Strip default device from the source if present and leading

    size_t lstr = strlen(work2);
    if (!no_source && !strncasecmp(request->source, work2, lstr)) {
        for (size_t i = 0; i < lstr; i++) {
            request->source[i] = ' ';
        }
        left_trim_string(request->source);
    }

    //------------------------------------------------------------------------------
    // Is this server acting as an UDA Proxy? If all access requests are being re-directed then do nothing to the
    // arguments. They are just passed onwards without interpretation.

    bool is_proxy = config.get("server.is_proxy").as_or_default(false);

    if (is_proxy) {
        request->request = (int)Request::ReadUDA;
    }

    //==============================================================================
    // Check if the data_source has one of these forms:
    //
    //    pulse            plasma shot number - an integer
    //    pulse/pass        include a pass or sequence number - this may be a text based component, e.g. LATEST
    //    DEVICE::pulse        prefixed by a device name
    //    DEVICE::pulse/pass
    //
    //    FORMAT::/path/to/my/file
    //      FORMAT::./path/to/my/file        use client side resolution of ./ location contained in path otherwise
    //      ignore FORMAT::../path/to/my/file        use client side resolution of ../ location
    //    FORMAT::/scratch/path/to/my/file    use client side resolution of /scratch location (change name via
    //    environment variable)
    //
    //    FORMAT::pulse        FORMAT is the default FORMAT, e.g. IDA3
    //    FORMAT::/pulse
    //    FORMAT::/pulse/pass
    //    /pulse
    //    /pulse/pass
    //
    //    DEVICE::FORMAT::/path/to/my/file    Passed on to a different UDA server without interpretation
    //    DEVICE::FORMAT::pulse

    //    /path/to/my/file.ext            use file extension 'ext' to identify the correct FORMAT if known
    //      ./path/to/my/file.ext
    //      ../path/to/my/file.ext
    //    /scratch/path/to/my/file.ext
    //
    //    PROTOCOL::server.host.name:port/U/R/L    server access requests - always requires the delimiter string element
    //    in string
    //
    //    function(arguments or name value pair list)        server side processing of data
    //    LIBRARY::function(arguments or name value pair list)    function plugin library
    //    DEVICE::function(arguments or name value pair list)    Not allowed - use DEVICE::ServerSide::function()
    //
    //    DEVICE::FORMAT:: ...            If the DEVICE is not the default device, then a server protocol is invoked to
    //    pass
    //                                              the request forward (FORMAT:: ...)
    //
    // Legacy exception: treat PPF and JPF formats as server protocols => no file path expansion required and ignored
    //
    //      PPF::/ddaname/pulse/pass/userid or PPF::ddaname/pulse/pass/userid
    //    JPF::pulse or JPF::/pulse
    //
    //------------------------------------------------------------------------------
    // Scenario #1: Format or protocol or library is present - there are no delimiters in the source string

    bool is_function = false;
    bool is_file = false;
    bool is_server = false;
    bool is_foreign = false;

    char* test = strstr(request->source, request->api_delim); // Delimiter present?
    if (test != nullptr) {
        copy_string(request->source, work2, test - request->source);
        trim_string(work2);
        strcpy(work, test + ldelim);
    } else {
        work2[0] = '\0';
        strcpy(work, request->source);
    }

    // Test for DEVICE::LIBRARY::function(argument)     - More delimiter characters present?

    char* p;
    if (test != nullptr && STR_IEQUALS(work2, device.c_str()) &&
        (p = strstr(work, request->api_delim)) != nullptr) {
        lstr = (p - work);
        copy_string(work, work2, lstr); // Ignore the default device name - force a pass to Scenario 2
        trim_string(work2);
        lstr = lstr + ldelim;
        for (size_t i = 0; i < lstr; i++) {
            work[i] = ' ';
        }
        left_trim_string(work);
    }

    bool reduce_signal;

    do {

        if (no_source) {
            // No Source
            strcpy(request->device_name, device.c_str()); // Default Device Name
            break;
        }

        if (test == nullptr || STR_IEQUALS(work2, device.c_str())) { // No delimiter present or default device?

            UDA_LOG(UDA_LOG_DEBUG, "No device name or format or protocol or library is present")

            strcpy(request->device_name, device.c_str()); // Default Device Name

            // Regular request: pulse or pulse/pass ==> Generic request

            if (generic_request_test(work, request)) {
                break;
            }

            // Not a Server Side Function?         Note: /a/b/fun(aaa) is a (bad!)file path and fun(a/b/c) is a function

            char* p0 = strchr(work, '/'); // Path separator mixed with parenthesis?
            char* p1 = strrchr(work, '/');
            p = strchr(work, '(');
            char* p2 = strchr(work, ')');

            if (p == nullptr || p2 == nullptr || (p != nullptr && p2 == nullptr) || (p == nullptr && p2 != nullptr) ||
                (p0 != nullptr && p != nullptr && p0 < p) || (p1 != nullptr && p2 != nullptr && p1 > p2)) {

                if ((p0 != nullptr || p1 != nullptr) && (p != nullptr || p2 != nullptr)) {
                    err = 999;
                    add_error(ErrorType::Code, "make_server_request_block", err,
                              "Source syntax: path with parenthesis () is incorrect!");
                    return err;
                }

                // Request must be a private file format. It cannot be a local/remote server protocol.

                UDA_LOG(UDA_LOG_DEBUG, "No File Format has been specified. Selecting ....")

                int rc = source_file_format_test(config, request->source, request, pluginList);

#ifdef JETSERVER
                if (rc < 0) {
                    strcpy(request->format, "PPF"); // Assume the Default Format (PPF?)
                    for (int i = 0; i < pluginList->count; i++) {
                        if (STR_IEQUALS(request->format, pluginList->plugin[i].format)) {
                            request->request = pluginList->plugin[i].request;
                            break;
                        }
                    }
                    test = request->source; // No prefix nor delimiter
                    ldelim = 0;             // No offset required
                    rc = 1;
                }
#endif

                if (rc <= 0) {
                    UDA_LOG(UDA_LOG_DEBUG, "File Format NOT identified from name extension!")
                    UDA_THROW_ERROR(999, "No File Format identified: Please specify.")
                }

                // Resolve any Serverside environment variables
                udaExpandEnvironmentalVariables(request->path);

                UDA_LOG(UDA_LOG_DEBUG, "File Format identified from name extension!")
                break;

            } else {

                // Internal Server Side Function ?    A file path may contain characters ( and ) !

                if ((p = strchr(work, '(')) != nullptr && strchr(p, ')') != nullptr) {
                    strcpy(work2, &p[1]);
                    p = strchr(work2, ')');
                    p[0] = '\0';
                    left_trim_string(work2);
                    trim_string(work2);

                    request->request = (int)Request::ReadServerside;
                    extract_function_name(work, request);

                    UDA_LOG(UDA_LOG_DEBUG, "**** Server Side Function ??? ****")

                    // Extract Name Value pairs

                    if (name_value_pairs(work2, &request->nameValueList, strip) == -1) {
                        UDA_THROW_ERROR(999, "Name Value pair syntax is incorrect!")
                    }

                    // Test for external library functions using the Archive name as the library name identifier

                    reduce_signal = false;
                    extract_archive(config, request, reduce_signal);

                    size_t id = 0;
                    for (const auto& plugin : pluginList) {
                        if (plugin.name == request->archive) {
                            request->request = id;
                            copy_string(plugin.name, request->format, StringLength);
                        }
                        ++id;
                    }
                    break;

                } else {
                    UDA_THROW_ERROR(999, "No Data Access Plugin Identified!")
                }
            }

        } else {

            //---------------------------------------------------------------------------------------------------------------------
            // Scenario #2: A foreign device name or format or protocol or library is present

            UDA_LOG(UDA_LOG_DEBUG, "A device name or format or protocol or library is present.")

            // Test for known File formats, Server protocols or Libraries or Devices

            size_t id = 0;
            for (const auto& plugin : pluginList) {
                if (plugin.name == work2) {
                    if (plugin.type != UDA_PLUGIN_CLASS_DEVICE) {
                        request->request = id; // Found
                        copy_string(plugin.name, request->format, StringLength);
                        if (plugin.type != UDA_PLUGIN_CLASS_FILE) { // The full file path fully resolved by the client
                            strcpy(request->path,
                                   test + ldelim);     // Complete String following :: delimiter
                            strcpy(request->file, ""); // Clean the filename
                            if (plugin.type == UDA_PLUGIN_CLASS_FUNCTION) {
                                is_function = true;
                                extract_function_name(work, request);
                            }
                        } else {
#ifndef __GNUC__
                            char base[1024] = {0};
                            _splitpath(test + ldelim, NULL, base, NULL, NULL);
#else
                            char* base = basename(test + ldelim);
#endif
                            strcpy(request->file, base); // Final token
                        }
                        is_file = plugin.type == UDA_PLUGIN_CLASS_FILE;
                        is_server = plugin.type == UDA_PLUGIN_CLASS_SERVER;
                        break;
                    }
                }
                ++id;
            }

            // If no match was found then the prefix must be a foreign Device Name not entered into the server
            // configuration file. The request must be a generic lookup of how to access data remotely for the specified
            // device. The external server providing access to the foreign device's data will interpret the arguments

            if (request->request == (int)Request::ReadUnknown) {
                UDA_LOG(UDA_LOG_DEBUG, "No plugin was identified for the format: {}", work2)
                is_foreign = true;
                strcpy(request->device_name, work2);     // Copy the DEVICE prefix
                request->request = (int)Request::ReadGeneric; // The database will identify the target

                break;
            }

            // A match was found: The Source must be a format or a protocol or a library

            strcpy(request->device_name, device.c_str()); // Default Device Name

            if (is_file) { // Resolve any Serverside environment variables
                UDA_LOG(UDA_LOG_DEBUG, "File Format has been specified.")
                udaExpandEnvironmentalVariables(request->path);
                break;
            }

            if (!is_file && !is_function) { // Server Protocol
                UDA_LOG(UDA_LOG_DEBUG, "Server Protocol")
                break;
            }

            // Must be a function
            // Test syntax

            char* p0 = strchr(work, '/');  // Path separator mixed with parenthesis?
            char* p1 = strrchr(work, '/'); // Path separator mixed with parenthesis?
            p = strchr(work, '(');
            char* p2 = strchr(work, ')');

            if (p == nullptr || p2 == nullptr || (p != nullptr && p2 == nullptr) || (p == nullptr && p2 != nullptr) ||
                (p0 != nullptr && p != nullptr && p0 < p) || (p1 != nullptr && p2 != nullptr && p1 > p2)) {
                UDA_THROW_ERROR(999, "Not a function when one is expected! - A Library plugin has been specified.")
            }

            // ToDo: Extract Data subset operations specified within the source argument

            // Extract Name Value pairs

            if ((p = strchr(work, '(')) != nullptr && strchr(p, ')') != nullptr) {
                strcpy(work, &p[1]);
                p = strrchr(work, ')');
                p[0] = '\0';
                left_trim_string(work);
                trim_string(work);

                // Extract Name Value pairs

                if (name_value_pairs(work, &request->nameValueList, strip) == -1) {
                    UDA_THROW_ERROR(999, "Name Value pair syntax is incorrect!")
                }

                // ToDo: Extract Data subset operations specified as a named value pair, tagged 'subset'

            } else {
                UDA_THROW_ERROR(999, "Function syntax error - please correct")
            }
        }
    } while (0);

    UDA_LOG(UDA_LOG_DEBUG, "Signal Argument")

    //==============================================================================
    // Check the data object (Signal) has one of these forms:
    //
    // signal
    // signal[subset]
    //
    // ARCHIVE::
    // ARCHIVE::signal
    // ARCHIVE::signal[subset]

    // function(arguments or name value pair list)
    // function(arguments or name value pair list)[subset]

    // LIBRARY::function(arguments or name value pair list)
    // LIBRARY::function(arguments or name value pair list)[subset]

    // A function is defined when the argument contains a pair of parenthesis '()' and
    // is prefixed with a recognised function library identifier - not the default Archive or Device Name.
    // These identifiers are listed in the server configuration file. The only exception to this
    // rule is when the source term is empty or is set to the default device name, then the default server-side
    // function library is assumed.

    // If no library is defined or recognised and the source term is set to a device other than the default device,
    // the selected request option is Unknown.

    // Only functions can be passed via the Signal argument without specifying a source.

    // A Function call via the source argument takes precedence over one passed in the signal argument.

    //------------------------------------------------------------------------------
    // Extract Data subset operations from the data object (signal) string

    int rc = 0;
    if ((rc = extract_subset(request)) == -1) {
        UDA_THROW_ERROR(999, "Subset operation is incorrect!")
    }

    // as at 19Apr2011 no signals recorded in the UDA database use either [ or { characters
    // there will be no confusion between subset operations and valid signal names

    if (rc == 1) { // the subset has valid syntax so reduce the signal name by removing the subset instructions
        p = strstr(request->signal, request->subset);
        if (p != nullptr) {
            p[0] = '\0'; // Remove subset operations from variable name
        }
        trim_string(request->signal);
    } else {
        request->subset[0] = '\0';
        request->datasubset.nbound = 0;
    }

    //------------------------------------------------------------------------------
    // Extract the Archive Name and detach from the signal  (detachment is necessary when not passing on to another UDA
    // server) the Plugin Name is synonymous with the Archive Name and takes priority (The archive name is discarded as
    // unimportant)

    if (request->request == (int)Request::ReadUDA) {
        reduce_signal = false;
        err = extract_archive(config, request, reduce_signal);
    } else {
        reduce_signal = !is_foreign; // Don't detach if a foreign device
        err = extract_archive(config, request, reduce_signal);
    }
    if (request->archive[0] == '\0') {
        strcpy(request->archive, archive.c_str());
    }

    //------------------------------------------------------------------------------
    // Extract Name Value Pairs from the data object (signal) without modifying

    // as at 22Sep2011 261 signals recorded in the UDA database used parenthesis characters so could be confused
    // with function requests. However, for all these cases a source term would be specified. No library
    // would be part of this specification so there would be no ambiguity.

    is_function = false;

    if (!is_server && (p = strchr(request->signal, '(')) != nullptr && strchr(p, ')') != nullptr &&
        strcasecmp(request->archive, archive.c_str()) != 0) {
        strcpy(work, &p[1]);
        if ((p = strrchr(work, ')')) != nullptr) {
            p[0] = '\0';
            left_trim_string(work);
            trim_string(work);
            is_function = true;
            if (name_value_pairs(work, &request->nameValueList, strip) == -1) {
                UDA_THROW_ERROR(999, "Name Value pair syntax is incorrect!")
            }
            extract_function_name(request->signal, request);
        }
    }

    //------------------------------------------------------------------------------
    // If No Source was Specified: All requirements are contained in the signal string

    if (no_source) {
        UDA_LOG(UDA_LOG_DEBUG, "Signal Argument - No Source")
        UDA_LOG(UDA_LOG_DEBUG, "request: {}", request->request)

        // If the signal could be a function call, check the archive name against the function library plugins

        if (is_function && err == 0) { // Test for known Function Libraries
            is_function = false;

            size_t id = 0;
            for (const auto& plugin : pluginList) {
                std::string name = request->archive;
                boost::to_lower(name);
                if (plugin.name == name) {

                    request->request = id;
                    copy_string(plugin.name, request->format, StringLength);
                    is_function = plugin.type == UDA_PLUGIN_CLASS_FUNCTION;
                    break;
                }
                ++id;
            }

            UDA_LOG(UDA_LOG_DEBUG, "A request: {}", request->request);
            UDA_LOG(UDA_LOG_DEBUG, "isFunction: {}", is_function);

            if (!is_function) { // Must be a default server-side function
                for (const auto& plugin : pluginList) {
                    if (plugin.entry_func_name == "ServerSide" && plugin.library_name.empty()) {
                        request->request = (int)Request::ReadServerside; // Found
                        copy_string(plugin.name, request->format, StringLength);
                        is_function = true;
                        break;
                    }
                }
                if (!is_function) {
                    request->function[0] = '\0';
                }
            }

            UDA_LOG(UDA_LOG_DEBUG, "B request: {}", request->request)

        } else {
            // Select the Generic plugin: No source => no format or protocol or library was specified.
            request->request = (int)Request::ReadGeneric;
            UDA_LOG(UDA_LOG_DEBUG, "C request: {}", request->request)
        }

    } else {

        // Does the data object (Signal) have the form: LIBRARY::function?
        // Exception is Serverside function

        if (is_function && strcasecmp(request->archive, archive.c_str()) != 0) {
            int id = find_plugin_id_by_format(request->archive, pluginList);
            if (id >= 0
                    && pluginList[id].type == UDA_PLUGIN_CLASS_FUNCTION
                    && pluginList[id].entry_func_name != "serverside") {
                if (request->request == (int)Request::ReadGeneric || request->request == (int)Request::ReadUnknown) {
                    request->request = id; // Found
                    copy_string(pluginList[id].name, request->format, StringLength);
                    UDA_LOG(UDA_LOG_DEBUG, "D request: {}", request->request);
                } else if (request->request != id) { // Inconsistent
                    // Let Source have priority over the Signal?
                    UDA_LOG(UDA_LOG_DEBUG, "Inconsistent Plugin Libraries: Source selected over Signal")
                }
            }
        }
    }
    UDA_LOG(UDA_LOG_DEBUG, "E request: {}", request->request)

    //---------------------------------------------------------------------------------------------------------------------
    // MDS+ Servers ...

    // MDS+ Source naming models:    MDS+::localhost/tree/number    any source with one or more / must have a trailing
    // number
    //                 MDS+::server/tree/number
    //                MDS+::server/path/to/data/tree/number
    //                MDS+::server/path.to.data/tree/number
    //                MDS+::/path/to/data/tree/number
    //                MDS+::/path.to.data/tree/number
    //                 MDS+::tree/number
    //                MDS+::server
    //                MDS+::

    if (request->request == (int)Request::ReadMDS && !is_proxy) {

        reverse_string(test + ldelim, work); // Drop the delimiters and Reverse the Source String

        char* token;
        if ((token = strtok(work, "/")) != nullptr) { // Tokenise
            if (is_number(token)) {           // This should be the tree Number otherwise only the server is passed
                reverse_string(token, work2); // Un-Reverse the token
                request->exp_number = (int)strtol(work2, nullptr, 10); // Extract the Data Tree Number
                if ((token = strtok(nullptr, "/")) != nullptr) {
                    reverse_string(token, request->file); // This should be the Tree Name
                    work2[0] = '\0';
                    while ((token = strtok(nullptr, "/")) != nullptr) {
                        // Everything Else is the Server Host and URL Path to the Tree
                        strcat(work2, token);
                        strcat(work2, "/");
                    }
                    if (work2[0] == '/') {
                        strcpy(work2, &work2[1]); // Drop Trailing /
                    }
                    reverse_string(work2, request->server);
                    token = test + ldelim; // Preserve Leading /
                    if (token[0] != '/' && request->server[0] == '/') {
                        request->server[0] = ' ';
                        left_trim_string(request->server);
                    }
                } else {
                    err = 3;
                }
            } else {
                strcpy(request->server, test + ldelim); // Server or null string (default server)
            }

        } else {
            strcpy(request->server, ""); // Default Server
        }

        if (err != 0) {
            UDA_THROW_ERROR((int)RequestError::NoServerSpecified, "The MDSPlus Data Source does not comply with the naming models: "
                                                 "server/tree/number or server/path/to/data/tree/number")
        }
    }

    //---------------------------------------------------------------------------------------------------------------------
    // UDA and WEB Servers ...      parse source modelled as: server:port/source

    if (request->request == (int)Request::ReadUDA || request->request == (int)Request::ReadWeb) {
        strcpy(work, test + ldelim); // Drop the delimiters

        // Isolate the Server from the source UDA::server:port/source or SSL://server:port/source

        strcpy(request->server, work);

        char* s = nullptr;
        if ((s = strstr(work, "SSL://")) != nullptr) {
            char* token;
            if ((token = strstr(s + 6, "/")) != nullptr) {
                token[0] = '\0';                  // Break the String (work)
                strcpy(request->server, s);       // Extract the Server Name and Port (with SSL:// prefix)
                strcpy(request->file, token + 1); // Extract the Source URL Argument
            }
        } else {
            char* token;
            if ((token = strstr(work, "/")) != nullptr) {
                token[0] = '\0';                  // Break the String (work)
                strcpy(request->server, work);    // Extract the Server Name and Port
                strcpy(request->file, token + 1); // Extract the Source URL Argument
            }
        }

        UDA_LOG(UDA_LOG_DEBUG, "Server: {}", request->server)
        UDA_LOG(UDA_LOG_DEBUG, "Source: {}", request->file)
    }

    //---------------------------------------------------------------------------------------------------------------------
    // SQL Servers ...

    if (request->request == (int)Request::ReadSQL) {
        strcpy(request->server, request->path);
        if ((test = strchr(request->server, '/')) != nullptr) {
            test[0] = '\0';
            strcpy(request->path, &test[1]);
        } else {
            request->path[0] = '\0';
        }
    }

    return 0;
}

int uda::client_server::make_request_block(const config::Config& config, RequestBlock* request_block,
                                           const std::vector<PluginData>& pluginList)
{
    int rc = 0;

    for (int i = 0; i < request_block->num_requests; ++i) {
        auto request = &request_block->requests[0];
        rc = make_request_data(config, request, pluginList);
        if (rc != 0) {
            break;
        }
    }

    return rc;
}

void extract_function_name(const char* str, RequestData* request)
{
    int lstr;
    char* p;
    if (str[0] == '\0') {
        return;
    }
    char* work = (char*)malloc((strlen(str) + 1) * sizeof(char));
    strcpy(work, str);
    if ((p = strchr(work, '(')) == nullptr) {
        return;
    }
    p[0] = '\0';
    p = strstr(work, request->api_delim);
    if (p != nullptr) {
        do {
            lstr = (int)(p - work) + (int)strlen(request->api_delim);
            for (int i = 0; i < lstr; i++) {
                work[i] = ' ';
            }
            trim_string(work);
            left_trim_string(work);
        } while ((p = strstr(work, request->api_delim)) != nullptr);
    }
    strcpy(request->function, work);
    free(work);
}

/**
 * returns true if a format was identified, false otherwise.
 */
int source_file_format_test(const uda::config::Config& config, const char* source, RequestData* request,
                            const std::vector<PluginData>& pluginList)
{
    int rc = 0;
    const char* test;

    // .99        IDA3 file
    // .nc        netCDF4
    // .cdf        netCDF3
    // .hf        HDF
    // .jpg        Binary
    // .csv        ASCII

    // Note: On JET the default data source is PPF. This is really a server protocol and not a file format.

    //------------------------------------------------------------------------------
    // Start with ignorance about which plugin to use

    request->format[0] = '\0';
    request->file[0] = '\0';
    request->request = (int)Request::ReadUnknown;

    if (source[0] == '\0') {
        return rc;
    }

    // Does the path contain any Illegal (or problem) characters

    if (!is_legal_file_path((char*)source)) {
        return rc; // Not compliant with Portable Filename character set
    }

    // Does the source have a file extension? If so choose the format using the extension, otherwise investigate the
    // file.

    if ((test = strrchr(source, '.')) == nullptr) {

        // No extension => test the first line of file, e.g. head -c10 <file>, but both netcdf and hdf5 use the same
        // label HDF!

#ifndef _WIN32
        const char* nc = " nc";
        const char* hf = " hf";
        const char* ida = " 99";
        const char* blank = "   ";
        FILE* ph = nullptr;
        int lstr = StringLength;
        std::string cmd;
        cmd = fmt::format("head -c10 {} 2>/dev/null", source);
        errno = 0;
        if ((ph = popen(cmd.c_str(), "r")) == nullptr) {
            if (errno != 0) {
                add_error(ErrorType::System, "sourceFileFormatTest", errno, "");
            }
            add_error(ErrorType::Code, "sourceFileFormatTest", 999, "Unable to Identify the File's Format");
            return -999;
        }

        char buffer[StringLength];
        if (!feof(ph)) {
            if (fgets(buffer, lstr - 1, ph) == nullptr) {
                UDA_THROW_ERROR(-999, "failed to read command")
            }
        }
        pclose(ph);

        test = blank;
        convert_non_printable2(buffer);
        left_trim_string(buffer);
        trim_string(buffer);

        if (STR_EQUALS(buffer, "CDF")) { // Legacy netCDF file
            test = nc;
        } else {
            if (STR_EQUALS(buffer, "HDF")) { // Either a netCDF or a HDF5 file: use utility programs to reveal!
                char* env = getenv("UDA_DUMP_NETCDF");
                if (env != nullptr) {
                    cmd = fmt::format("{} -h {} 2>/dev/null | head -c10 2>/dev/null", env, source);
                    errno = 0;
                    if ((ph = popen(cmd.c_str(), "r")) == nullptr) {
                        if (errno != 0) {
                            add_error(ErrorType::System, "sourceFileFormatTest", errno, "");
                        }
                        add_error(ErrorType::Code, "sourceFileFormatTest", 999,
                                  "Unable to Identify the File's Format");
                        return -999;
                    }

                    buffer[0] = '\0';
                    if (!feof(ph)) {
                        if (fgets(buffer, lstr - 1, ph) == nullptr) {
                            UDA_THROW_ERROR(-999, "failed to read command")
                        }
                    }
                    pclose(ph);
                    convert_non_printable2(buffer);
                    left_trim_string(buffer);
                    trim_string(buffer);

                    if (STR_EQUALS(buffer, "netcdf")) { // netCDF file written to an HDF5 file
                        test = nc;
                    } else {
                        if (cmd[0] == '\0') {
                            test = hf; // HDF5 file
                        }
                    }
                }
            } else { // an IDA File?
                char* env = getenv("UDA_DUMP_IDA");
                if (env != nullptr) {
                    cmd = fmt::format("{} -h {} 2>/dev/null 2>/dev/null", env, source);
                    errno = 0;
                    if ((ph = popen(cmd.c_str(), "r")) == nullptr) {
                        if (errno != 0) {
                            UDA_ADD_SYS_ERROR("");
                        }
                        UDA_ADD_ERROR(999, "Unable to Identify the File's Format");
                        return -999;
                    }

                    buffer[0] = '\0';
                    if (!feof(ph)) {
                        // IDA3 interface version V3.13 with file structure IDA3.1
                        if (fgets(buffer, lstr - 1, ph) == nullptr) {
                            UDA_THROW_ERROR(-999, "failed to read command output")
                        }
                    }
                    if (!feof(ph)) {
                        // Build JW Jan 25 2007 09:08:47
                        if (fgets(buffer, lstr - 1, ph) == nullptr) {
                            UDA_THROW_ERROR(-999, "failed to read command output")
                        }
                    }
                    if (!feof(ph)) {
                        // Compiled without high level read/write CUTS
                        if (fgets(buffer, lstr - 1, ph) == nullptr) {
                            UDA_THROW_ERROR(-999, "failed to read command output")
                        }
                    }
                    if (!feof(ph)) {
                        // Opening ida file
                        if (fgets(buffer, lstr - 1, ph) == nullptr) {
                            UDA_THROW_ERROR(-999, "failed to read command output")
                        }
                    }
                    if (!feof(ph)) {
                        // ida_open error ?
                        if (fgets(buffer, lstr - 1, ph) == nullptr) {
                            UDA_THROW_ERROR(-999, "failed to read command output")
                        }
                    }
                    pclose(ph);
                    convert_non_printable2(buffer);
                    left_trim_string(buffer);
                    trim_string(buffer);
                    if (strncmp(buffer, "ida_open error", 14) != 0) {
                        test = ida; // Legacy IDA file
                    }
                }
            }
        }

#else
        return rc;
#endif
    }

    // Select the format

    do {

        // Test against Registered File Extensions
        // TO DO: make extensions a list of valid extensions to minimise plugin duplication

        bool break_again = false;
        for (const auto& plugin : pluginList) {
            if (plugin.extension == &test[1]) {
                copy_string(plugin.name, request->format, StringLength);
                break_again = true;
                break;
            }
        }
        if (break_again) {
            break;
        }

        // Other regular types

        if (strlen(&test[1]) == 2 && is_number(&test[1])) { // an integer number?
            strcpy(request->format, "IDA3");
            break;
        }
        if (STR_IEQUALS(&test[1], "nc")) {
            strcpy(request->format, "netcdf");
            break;
        }
        if (STR_IEQUALS(&test[1], "cdf")) {
            strcpy(request->format, "netcdf");
            break;
        }
        if (STR_IEQUALS(&test[1], "hf")) {
            strcpy(request->format, "hdf5");
            break;
        }
        if (STR_IEQUALS(&test[1], "h5")) {
            strcpy(request->format, "hdf5");
            break;
        }
        if (STR_IEQUALS(&test[1], "hdf5")) {
            strcpy(request->format, "hdf5");
            break;
        }
        if (STR_IEQUALS(&test[1], "xml")) {
            strcpy(request->format, "xml");
            break;
        }
        if (STR_IEQUALS(&test[1], "csv")) {
            strcpy(request->format, "csv");
            break;
        }

        auto format = config.get("server.default_format").as_or_default(""s);
        if (source[0] == '/' && source[1] != '\0' && isdigit(source[1])) { // Default File Format?
            if (generic_request_test(&source[1], request)) {               // Matches 99999/999
                request->request = (int)Request::ReadUnknown;
                strcpy(request->format, format.c_str()); // the default Server File Format
                break;
            }
        }

        return -1; // No format identified

    } while (0);

    // Test for known registered plugins for the File's format

    size_t id = 0;
    for (const auto& plugin : pluginList) {
        if (plugin.name == request->format) {
            rc = 1;
            UDA_LOG(UDA_LOG_DEBUG, "Format identified, selecting specific plugin for {}", request->format)
            request->request = id;
            if (plugin.type != UDA_PLUGIN_CLASS_FILE) {
                // The full file path fully resolved by the client
                strcpy(request->file, "");             // Clean the filename
            } else {
#ifndef __GNUC__
                char base[1024] = {0};
                _splitpath(request->source, NULL, base, NULL, NULL);
#else
                char* base = basename(request->source);
#endif
                copy_string(base, request->file, StringLength);
            }
            break;
        }
        ++id;
    }

    return rc;
}

/**
 * Return true if the Generic plugin was selected, false otherwise
 */
int generic_request_test(const char* source, RequestData* request)
{
    int rc = 0;
    char* token = nullptr;
    char work[StringLength];

    //------------------------------------------------------------------------------
    // Start with ignorance about which plugin to use

    request->format[0] = '\0';
    request->file[0] = '\0';
    request->request = (int)Request::ReadUnknown;

    if (source[0] == '\0') {
        return rc;
    }
    if (source[0] == '/') {
        return rc; // Directory based data
    }

    //------------------------------------------------------------------------------
    // Check if the source has one of these forms:

    // pulse        plasma shot number - an integer
    // pulse/pass        include a pass or sequence number - this may be a text based component, e.g. LATEST

    if (is_number((char*)source)) { // Is the source an integer number?
        rc = 1;
        request->request = (int)Request::ReadGeneric;
        strcpy(request->path, "");                              // Clean the path
        request->exp_number = (int)strtol(source, nullptr, 10); // Plasma Shot Number
        UDA_LOG(UDA_LOG_DEBUG, "exp number identified, selecting GENERIC plugin.");
    } else {
        strcpy(work, source);
        if ((token = strtok(work, "/")) != nullptr) { // Tokenise the remaining string
            if (is_number(token)) {                   // Is the First token an integer number?
                rc = 1;
                request->request = (int)Request::ReadGeneric;
                strcpy(request->path, ""); // Clean the path
                request->exp_number = (int)strtol(token, nullptr, 10);
                if ((token = strtok(nullptr, "/")) != nullptr) { // Next Token
                    if (is_number(token)) {
                        request->pass = (int)strtol(token, nullptr, 10); // Must be the Pass number
                    } else {
                        strcpy(request->tpass, token); // capture anything else
                    }
                }
                UDA_LOG(UDA_LOG_DEBUG, "exp number and pass id identified, selecting GENERIC plugin.");
            }
        }
    }

    return rc;
}

//------------------------------------------------------------------------------
// Strip out the Archive or Plugin name from the data_object name
// syntax:    ARCHIVE::Data_OBJECT or DataObject
//        ARCHIVE::PLUGIN::Function() or PLUGIN::Function()
// conflict:    ARCHIVE::DataObject[::] or DataObject[::] subsetting operations
//
// NOTE: Archive/Plugin Name should not terminate with the character [ or { when a signal begins with the
//       character ] or }. These clash with subsetting syntax.
//
// Input Argument: reduceSignal - If TRUE (1) then extract the archive name and return the data object name
//                                without the prefixed archive name.

int extract_archive(const uda::config::Config& config, RequestData* request, int reduceSignal)
{
    int err = 0, test1, test2;
    int ldelim = (int)strlen(request->api_delim);
    char *test, *token, *work;

    trim_string(request->signal);

    auto proxy_target = config.get("server.proxy_target").as_or_default(""s);

    if (request->signal[0] != '\0' && proxy_target.empty()) {

        UDA_LOG(UDA_LOG_DEBUG, "Testing for ARCHIVE::Signal");

        if ((test = strstr(request->signal, request->api_delim)) != nullptr) {

            if (test - request->signal >= StringLength - 1 || strlen(test + ldelim) >= MaxMeta - 1) {
                UDA_ADD_ERROR((int)RequestError::ArchiveNameTooLong, "The ARCHIVE Name is too long!");
                return err;
            }
            copy_string(request->signal, request->archive, test - request->signal + 1);
            trim_string(request->archive);

            auto archive = config.get("request.default_archive").as_or_default(""s);

            // If a plugin is prefixed by the local archive name then discard the archive name
            if (reduceSignal && !strcasecmp(request->archive, archive.c_str())) {
                request->archive[0] = '\0';
                strcpy(request->signal, &test[ldelim]);
                return extract_archive(config, request, reduceSignal);
            }

            if (!is_legal_file_path(request->archive)) {
                request->archive[0] = '\0';
                return 0;
            }

            // Test the proposed archive name for conflict with subsetting operation

            test1 = 0;
            test2 = 0;

            if ((token = strchr(request->archive, '[')) != nullptr ||
                (token = strchr(request->archive, '{')) != nullptr) {
                test1 = (strlen(&token[1]) == 0 || is_number(&token[1]));
            }

            if ((token = strchr(test + ldelim, ']')) != nullptr || (token = strchr(test + ldelim, '}')) != nullptr) {
                work = (char*)malloc((strlen(test + ldelim) + 1) * sizeof(char));
                strcpy(work, test + ldelim);
                work[token - (test + ldelim)] = '\0';
                test2 = (strlen(work) == 0 || is_number(work));
                free(work);
            }

            if (!test1 && !test2) {
                if (reduceSignal) {
                    work = (char*)malloc((strlen(test + ldelim) + 1) * sizeof(char));
                    strcpy(work, test + ldelim);
                    strcpy(request->signal, work); // Valid Archive & signal
                    free(work);
                    trim_string(request->signal);
                }
            } else {
                request->archive[0] = '\0'; // Reset Archive
            }

            UDA_LOG(UDA_LOG_DEBUG, "Archive {}", request->archive)
            UDA_LOG(UDA_LOG_DEBUG, "Signal  {}", request->signal)
        }
    }
    return err;
}

//------------------------------------------------------------------------------
// Does the Path contain with an Environment variable

void udaExpandEnvironmentalVariables(char* path)
{
    size_t lcwd = StringLength - 1;
    char work[StringLength];
    char cwd[StringLength];
    char ocwd[StringLength];

    if (strchr(path, '$') == nullptr) {
        UDA_LOG(UDA_LOG_DEBUG, "No embedded environment variables detected")
        return;
    }

    if (getcwd(ocwd, lcwd) == nullptr) { // Current Working Directory
        UDA_LOG(UDA_LOG_DEBUG, "Unable to identify PWD!");
        return;
    }

    if (chdir(path) == 0) { // Change to path directory
        // The Current Working Directory is now the resolved directory name
        char* pcwd = getcwd(cwd, lcwd);

        UDA_LOG(UDA_LOG_DEBUG, "Expanding embedded environment variable:")
        UDA_LOG(UDA_LOG_DEBUG, "from: {}", path);
        UDA_LOG(UDA_LOG_DEBUG, "to: {}", cwd);

        if (pcwd != nullptr) {
            strcpy(path, cwd); // The expanded path
        }
        if (chdir(ocwd) != 0) {
            // Return to the Original WD
            UDA_LOG(UDA_LOG_ERROR, "failed to reset working directory")
        }
    } else {
        UDA_LOG(UDA_LOG_DEBUG, "expandEnvironmentvariables: Direct substitution!")

        char *fp = nullptr, *env, *fp1;
        char work1[StringLength];

        if (path[0] == '$' || (fp = strchr(&path[1], '$')) != nullptr) { // Search for a $ character

            if (fp != nullptr) {
                copy_string(path, work, fp - path);

                if ((fp1 = strchr(fp, '/')) != nullptr) {
                    copy_string(fp + 1, work1, fp1 - fp - 1);
                } else {
                    strcpy(work1, fp + 1);
                }

                if ((env = getenv(work1)) != nullptr) {
                    if (env[0] == '/') {
                        strcpy(work1, env + 1);
                    } else {
                        strcat(work1, env);
                    }
                }

                strcat(work, work1);
                strcat(work, fp1);
                strcpy(path, work);
            }

            if (path[0] == '$') {
                work1[0] = '\0';
                if ((fp = strchr(path, '/')) != nullptr) {
                    copy_string(path + 1, work, fp - path - 1);
                    strcpy(work1, fp);
                } else {
                    strcpy(work, path + 1);
                }

                if ((env = getenv(work)) != nullptr) { // Check for Environment Variable
                    if (env[0] == '/') {
                        strcpy(work, env);
                    } else {
                        strcpy(work, "/");
                        strcat(work, env);
                    }
                }
                strcat(work, work1);
                strcpy(path, work);
            }
        }

        UDA_LOG(UDA_LOG_DEBUG, "Expanding to: {}", path);
    }
}

OptionalLong parse_integer(const std::string& value)
{
    if (value.empty()) {
        return {.init = false, .value = 0};
    }
    size_t idx;
    long num = std::stol(value, &idx, 10);
    if (idx != value.size()) {
        throw std::runtime_error("Invalid integer");
    }
    return {.init = true, .value = num};
}

int parse_element(Subset& subset, const std::string& element)
{
    std::vector<std::string> tokens;
    boost::split(tokens, element, boost::is_any_of(":"), boost::token_compress_off);

    int index = subset.nbound;
    strcpy(subset.operation[index], ":");
    subset.dimid[index] = index;

    try {
        switch (tokens.size()) {
            case 0:
                subset.lbindex[index] = {.init = false, .value = 0};
                subset.ubindex[index] = {.init = false, .value = 0};
                subset.stride[index] = {.init = false, .value = 0};
                break;
            case 1:
                // TODO: handle non-slice operations? i.e. [>=4], etc.
                subset.lbindex[index] = parse_integer(tokens[0]);
                subset.ubindex[index] = {.init = true, .value = (subset.lbindex[index].value + 1)};
                subset.stride[index] = {.init = false, .value = 0};
                break;
            case 2:
                subset.lbindex[index] = parse_integer(tokens[0]);
                subset.ubindex[index] = parse_integer(tokens[1]);
                subset.stride[index] = {.init = false, .value = 0};
                break;
            case 3:
                subset.lbindex[index] = parse_integer(tokens[0]);
                subset.ubindex[index] = parse_integer(tokens[1]);
                subset.stride[index] = parse_integer(tokens[2]);
                break;
            default:
                UDA_ADD_ERROR(999, "Invalid number of elements in subset operation");
                return 999;
        }
    } catch (std::runtime_error& ex) {
        UDA_ADD_ERROR(999, ex.what());
        return 999;
    }

    subset.nbound += 1;

    return 0;
}

int parse_operation(Subset& subset, const std::string& operation)
{
    //----------------------------------------------------------------------------------------------------------------------------
    // Split instructions using syntax [a:b:c, d:e:f] where [startIndex:stopIndex:stride]
    //
    // Syntax    [a]        single items at index position a
    //        [*]        all items
    //        []        all items
    //
    //        [:]        all items starting at 0
    //        [a:]        all items starting at a
    //        [a:*]        all items starting at a
    //        [a:b]        all items starting at a and ending at b
    //
    //        [a::c]        all items starting at a with stride c
    //        [a:*:c]        all items starting at a with stride c
    //        [a:b:c]        all items starting at a, ending at b with stride c

    std::vector<std::string> tokens;
    boost::split(tokens, operation, boost::is_any_of(","), boost::token_compress_off);

    for (const auto& token : tokens) {
        int rc = parse_element(subset, token);
        if (rc != 0) {
            return rc;
        }
    }

    return 0;
}

//----------------------------------------------------------------------
// Parse subset instructions- [start:end:stride] or {start:end:stride}
//
// Signal should avoid using subset like components in their name

int extract_subset(RequestData* request)
{
    // Return codes:
    //
    //	1 => Valid subset
    //	0 => Not a subset operation - Not compliant with syntax
    //     -1 => Error

    request->subset[0] = '\0';
    init_subset(&request->datasubset);

    std::string signal = request->signal;

    if (signal.empty() || signal[signal.size() - 1] != ']') {
        return 0;
    }

    size_t rbracket_pos = signal.size() - 1;
    size_t lbracket_pos = signal.rfind('[', rbracket_pos);

    if (lbracket_pos == std::string::npos) {
        return 0;
    }

    std::deque<std::string> subsets = {};
    std::deque<std::string> operations = {};

    while (lbracket_pos != std::string::npos && rbracket_pos != std::string::npos) {
        if (rbracket_pos - lbracket_pos - 1 > 0) {
            std::string operation = signal.substr(lbracket_pos + 1, rbracket_pos - lbracket_pos - 1);
            operations.push_front(operation);

            std::string subset = signal.substr(lbracket_pos, rbracket_pos - lbracket_pos + 1);
            subsets.push_front(subset);
        }

        if (lbracket_pos == 0 || signal[lbracket_pos - 1] != ']') {
            break;
        }

        rbracket_pos = lbracket_pos - 1;
        lbracket_pos = signal.rfind('[', rbracket_pos);
    }

    for (const auto& operation : operations) {
        int rc = parse_operation(request->datasubset, operation);
        if (rc != 0) {
            return rc;
        }
    }

    std::string subset = boost::join(subsets, "");
    strcpy(request->subset, subset.c_str());
    return 1;
}

// name value pairs take the general form: name1=value1, name2=value2, ...
// values can be enclosed in single or double quotes, or none at all.
// All enclosing quotes are optionaly removed/ignored using the 'strip' argument. Usage is always context specific.
//
// name value pair delimiter has the special case insensitive name delimiter=character
// this is searched for first then used to parse all name value pairs. The default is ','
//
// The returned value is the count of the name value pairs. If an error occurs, the returned value of the
// pair count is -1.
void uda::client_server::free_name_value_list(NameValueList* nameValueList)
{
    if (nameValueList->nameValue != nullptr) {
        for (int i = 0; i < nameValueList->pairCount; i++) {
            if (nameValueList->nameValue[i].pair != nullptr) {
                free(nameValueList->nameValue[i].pair);
            }
            if (nameValueList->nameValue[i].name != nullptr) {
                free(nameValueList->nameValue[i].name);
            }
            if (nameValueList->nameValue[i].value != nullptr) {
                free(nameValueList->nameValue[i].value);
            }
        }
    }
    free(nameValueList->nameValue);
    nameValueList->pairCount = 0;
    nameValueList->listSize = 0;
    nameValueList->nameValue = nullptr;
}

void parse_name_value(const char* pair, NameValue* nameValue, unsigned short strip)
{
    int lstr;
    char *p, *copy;
    lstr = (int)strlen(pair) + 1;
    copy = (char*)malloc(lstr * sizeof(char));
    strcpy(copy, pair);
    left_trim_string(copy);
    trim_string(copy);
    nameValue->pair = (char*)malloc(lstr * sizeof(char));
    strcpy(nameValue->pair, copy);
    left_trim_string(nameValue->pair);
    UDA_LOG(UDA_LOG_DEBUG, "Pair: {}", pair)
    if ((p = strchr(copy, '=')) != nullptr) {
        *p = '\0';
        lstr = (int)strlen(copy) + 1;
        nameValue->name = (char*)malloc(lstr * sizeof(char));
        strcpy(nameValue->name, copy);
        lstr = (int)strlen(&p[1]) + 1;
        nameValue->value = (char*)malloc(lstr * sizeof(char));
        strcpy(nameValue->value, &p[1]);
    } else { // Mimic IDL keyword passing or stand alone values for placeholder substitution
        UDA_LOG(UDA_LOG_DEBUG, "Keyword or placeholder value: {}", copy)
        lstr = (int)strlen(copy) + 1;
        nameValue->name = (char*)malloc(lstr * sizeof(char));
        if (copy[0] == '/') {
            strcpy(nameValue->name, &copy[1]); // Ignore leader forward slash
        } else {
            strcpy(nameValue->name, copy);
        }
        lstr = 5;
        nameValue->value = (char*)malloc(lstr * sizeof(char));
        strcpy(nameValue->value, "true");
        UDA_LOG(UDA_LOG_DEBUG, "Placeholder name: {}, value: {}", nameValue->name, nameValue->value)
    }
    left_trim_string(nameValue->name);
    left_trim_string(nameValue->value);
    trim_string(nameValue->name);
    trim_string(nameValue->value);
    UDA_LOG(UDA_LOG_DEBUG, "Name: {}     Value: {}", nameValue->name, nameValue->value)

    // Regardless of whether or not the Value is not enclosed in quotes, strip out a possible closing parenthesis
    // character (seen in placeholder value substitution) This would not be a valid value unless at the end of a string
    // enclosed in quotes!
    lstr = (int)strlen(nameValue->value);
    if (nameValue->value[lstr - 1] == ')' && strchr(nameValue->value, '(') == nullptr) {
        nameValue->value[lstr - 1] = '\0';
    }
    UDA_LOG(UDA_LOG_DEBUG, "Name: {}     Value: {}", nameValue->name, nameValue->value)

    if (strip) { // remove enclosing single or double quotes
        lstr = (int)strlen(nameValue->name);
        if ((nameValue->name[0] == '\'' && nameValue->name[lstr - 1] == '\'') ||
            (nameValue->name[0] == '"' && nameValue->name[lstr - 1] == '"')) {
            nameValue->name[0] = ' ';
            nameValue->name[lstr - 1] = ' ';
            left_trim_string(nameValue->name);
            trim_string(nameValue->name);
        }
        lstr = (int)strlen(nameValue->value);
        if ((nameValue->value[0] == '\'' && nameValue->value[lstr - 1] == '\'') ||
            (nameValue->value[0] == '"' && nameValue->value[lstr - 1] == '"')) {
            nameValue->value[0] = ' ';
            nameValue->value[lstr - 1] = ' ';
            left_trim_string(nameValue->value);
            trim_string(nameValue->value);
        }
        UDA_LOG(UDA_LOG_DEBUG, "Name: {}     Value: {}", nameValue->name, nameValue->value);
    }

    free(copy);
}

uda::NameValue parse_name_value(std::string_view argument, bool strip)
{
    std::vector<std::string> tokens;
    boost::split(tokens, argument, boost::is_any_of("="), boost::token_compress_on);

    for (auto& token : tokens) {
        boost::trim(token);
    }

    uda::NameValue name_value = {};
    name_value.pair = argument;

    if (tokens.size() == 2) {
        // argument is name=value
        name_value.name = tokens[0];
        name_value.value = tokens[1];
    } else if (tokens.size() == 1) {
        // argument is name or /name
        if (boost::starts_with(tokens[0], "/")) {
            name_value.name = tokens[0].substr(1);
        } else {
            name_value.name = tokens[0];
        }
    } else {
        throw std::runtime_error{"invalid token"};
    }

    if (strip) {
        boost::trim_if(name_value.value, boost::is_any_of("'\""));
    }

    return name_value;
}

std::vector<uda::NameValue> uda::name_value_pairs(std::string_view input, bool strip)
{
    std::vector<uda::NameValue> name_values;

    std::vector<std::string> tokens;
    boost::split(tokens, input, boost::is_any_of(","), boost::token_compress_on);

    name_values.reserve(tokens.size());
    for (const auto& token : tokens) {
        name_values.push_back(parse_name_value(token, strip));
    }

    return name_values;
}

int uda::client_server::name_value_pairs(const char* pairList, NameValueList* nameValueList, unsigned short strip)
{
    // Ignore delimiter in anything enclosed in single or double quotes
    // Recognise /name as name=TRUE
    // if strip then remove all enclosing quotes (single or double)

    int lstr, pair_count = 0;
    char proposal, delimiter = ',', substitute = 1;
    char *p, *p2, *p3 = nullptr, *buffer, *copy;
    NameValue name_value;
    lstr = (int)strlen(pairList);

    if (lstr == 0) {
        return pair_count; // Nothing to Parse
    }

    // Placeholder substitution is neither a name-value pair nor a keyword so bypass this test
    // if (strchr(pairList, '=') == nullptr && pairList[0] != '/')
    //    return pairCount;        // Not a Name Value list or Keyword

    if (pairList[0] == '=') {
        return -1; // Syntax error
    }
    if (pairList[lstr - 1] == '=') {
        return -1; // Syntax error
    }

    lstr = lstr + 1;
    buffer = (char*)malloc(lstr * sizeof(char));
    copy = (char*)malloc(lstr * sizeof(char));

    strcpy(copy, pairList); // working copy

    UDA_LOG(UDA_LOG_DEBUG, "Parsing name values from argument: {}", pairList)

    // Locate the delimiter name value pair if present - use default character ',' if not

    if ((p = strcasestr(copy, "delimiter")) != nullptr) {
        strcpy(buffer, &p[9]);
        left_trim_string(buffer);
        if (buffer[0] == '=' && buffer[1] != '\0') {
            buffer[0] = ' ';
            left_trim_string(buffer); // remove whitespace
            if (strlen(buffer) >= 3 &&
                ((buffer[0] == '\'' && buffer[2] == '\'') || (buffer[0] == '"' && buffer[2] == '"'))) {
                proposal = buffer[1]; // proposal delimiter
                lstr = (int)(p - copy);
                if (lstr == 0) {                   // delimiter name value pair coincident with start of list
                    delimiter = proposal;          // new delimiter
                    p3 = strchr(&p[9], delimiter); // change delimiter to avert incorrect parse
                    *p3 = '#';
                } else {
                    copy_string(copy, buffer, lstr); // check 'delimiter' is not part of another name value pair
                    trim_string(buffer);
                    lstr = (int)strlen(buffer);
                    if (buffer[lstr - 1] == proposal) { // must be an immediately preceeding delimiter character
                        delimiter = proposal;           // new delimiter accepted
                        p3 = strchr(&p[9], delimiter);  // change delimiter temporarily to avert incorrect parse
                        *p3 = '#';
                    } else {
                        trim_string(buffer); // Check for non alpha-numeric character
                        lstr = (int)strlen(buffer);
                        if (!isalpha(buffer[lstr - 1]) && !isdigit(buffer[lstr - 1])) { // Probable syntax error!
                            free(buffer);
                            free(copy);
                            return -1; // Flag an Error
                        }
                    }
                }
            }
        }
    }

    // lists are enclosed in either single or double quotes. If the list elements are
    // separated using the delimiter character, replace them with a temporary character

    lstr = lstr - 1;
    int is_list = 0;
    int is_list_delim = 0;
    for (int i = 0; i < lstr; i++) {
        if (copy[i] == '\'' || copy[i] == '"') {
            if (is_list) {
                is_list = 0; // Switch substitution off
            } else {
                is_list = 1;
            } // Switch substitution on
        } else {
            if (is_list && copy[i] == delimiter) {
                is_list_delim = 1;
                copy[i] = substitute;
            }
        }
    }

    // separate each name value pair

    p = copy;
    do {
        if ((p2 = strchr(p, delimiter)) != nullptr) {
            strncpy(buffer, p, p2 - p);
            buffer[p2 - p] = '\0';
            p = p2 + 1;
        } else {
            strcpy(buffer, p);
        }

        UDA_LOG(UDA_LOG_DEBUG, "Parsing name value: {}", buffer)
        parse_name_value(buffer, &name_value, strip);
        UDA_LOG(UDA_LOG_DEBUG, "Name {}, Value: {}", name_value.name, name_value.value)

        // if (nameValue.name != nullptr && nameValue.value != nullptr) {
        if (name_value.name != nullptr) { // Values may be nullptr for use case where placeholder substitution is used
            pair_count++;
            if (pair_count > nameValueList->listSize) {
                nameValueList->nameValue = (NameValue*)realloc((void*)nameValueList->nameValue,
                                                               (nameValueList->listSize + 10) * sizeof(NameValue));
                nameValueList->listSize = nameValueList->listSize + 10;
            }
            nameValueList->pairCount = pair_count;
            nameValueList->nameValue[pair_count - 1] = name_value;
        }
    } while (p2 != nullptr);

    // housekeeping

    free(buffer);
    free(copy);

    for (int i = 0; i < nameValueList->pairCount; i++) {
        if (STR_IEQUALS(nameValueList->nameValue[i].name, "delimiter")) { // replace with correct delimiter value
            p = strchr(nameValueList->nameValue[i].value, '#');
            *p = delimiter;
            p = strrchr(nameValueList->nameValue[i].pair, '#');
            *p = delimiter;
            break;
        }
    }

    // Replace substituted delimiters in lists

    if (is_list_delim) {
        for (int i = 0; i < nameValueList->pairCount; i++) {
            if ((p = strchr(nameValueList->nameValue[i].value, substitute)) != nullptr) {
                do {
                    p[0] = delimiter;
                } while ((p = strchr(nameValueList->nameValue[i].value, substitute)) != nullptr);
            }
            if ((p = strchr(nameValueList->nameValue[i].pair, substitute)) != nullptr) {
                do {
                    p[0] = delimiter;
                } while ((p = strchr(nameValueList->nameValue[i].pair, substitute)) != nullptr);
            }
        }
    }

    return pair_count;
}
