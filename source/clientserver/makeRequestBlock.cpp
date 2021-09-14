#include "makeRequestBlock.h"

#include <cerrno>

#if defined(__GNUC__)
#  include <unistd.h>
#endif

#if defined(__GNUC__)
#  include <libgen.h>
#endif

#include <logging/logging.h>
#include <plugins/pluginStructs.h>

#include "errorLog.h"
#include "stringUtils.h"
#include "udaErrors.h"
#include "udaStructs.h"

#if !defined(__GNUC__) && defined(_WIN32)
#  include <direct.h>
#  define strcasecmp _stricmp
#  define strncasecmp _strnicmp
#  define getcwd _getcwd
#  define chdir _chdir
#endif

#define MAXMAPDEPTH     10  // Maximum number of chained signal name mappings (Recursive depth)
#define MAXREQDEPTH     4   // Maximum number of Device Name to Server Protocol and Host substitution

static void extract_function_name(const char* str, REQUEST_DATA* request);

static int source_file_format_test(const char* source, REQUEST_DATA* request, PLUGINLIST pluginList,
                                   const ENVIRONMENT* environment);

static int extract_archive(REQUEST_DATA* request, int reduceSignal, const ENVIRONMENT* environment);

static int generic_request_test(const char* source, REQUEST_DATA* request);

static int extract_subset(REQUEST_DATA* request);

static int find_plugin_id_by_format(const char* format, const PLUGINLIST* plugin_list)
{
    for (int i = 0; i < plugin_list->count; i++) {
        if (STR_IEQUALS(plugin_list->plugin[i].format, format)) {
            return i;
        }
    }
    return -1;
}

int makeRequestData(REQUEST_DATA* request, PLUGINLIST pluginList, const ENVIRONMENT* environment)
{
    int ldelim;
    int err = 0;
    char work[MAXMETA];
    char work2[MAXMETA];
    unsigned short strip = 1;        // Remove enclosing quotes from name value pairs

    UDA_LOG(UDA_LOG_DEBUG, "Source Argument\n");

    //------------------------------------------------------------------------------
    // Always use the client's delimiting string if provided, otherwise use the default delimiter

    if ((ldelim = (int)strlen(request->api_delim)) == 0) {
        strcpy(request->api_delim, environment->api_delim);
        ldelim = (int)strlen(request->api_delim);
    }

    //------------------------------------------------------------------------------
    // Start with ignorance about which plugin to use

    request->request = REQUEST_READ_UNKNOWN;

    //------------------------------------------------------------------------------
    // Check there is something to work with!

    sprintf(work, "%s%s", environment->api_archive, environment->api_delim);    // default archive
    sprintf(work2, "%s%s", environment->api_device, environment->api_delim);    // default device

    LeftTrimString(request->signal);
    TrimString(request->signal);
    LeftTrimString(request->source);
    TrimString(request->source);

    bool noSource = (request->source[0] == '\0' ||                // no source
                     STR_IEQUALS(request->source, environment->api_device) ||    // default device name
                     STR_IEQUALS(request->source, work2));            // default device name + delimiting string

    if ((request->signal[0] == '\0' || STR_IEQUALS(request->signal, work)) && noSource) {
        THROW_ERROR(999, "Neither Data Object nor Source specified!");
    }

    //------------------------------------------------------------------------------
    // Strip default device from the source if present and leading

    size_t lstr = strlen(work2);
    if (!noSource && !strncasecmp(request->source, work2, lstr)) {
        for (size_t i = 0; i < lstr; i++) {
            request->source[i] = ' ';
        }
        LeftTrimString(request->source);
    }

    //------------------------------------------------------------------------------
    // Is this server acting as an IDAM Proxy? If all access requests are being re-directed then do nothing to the arguments.
    // They are just passed onwards without interpretation.

    bool isProxy = environment->server_proxy[0] != '\0';

    if (isProxy) {
        request->request = REQUEST_READ_IDAM;
    }

    //==============================================================================
    // Check if the data_source has one of these forms:
    //
    //	pulse			plasma shot number - an integer
    //	pulse/pass		include a pass or sequence number - this may be a text based component, e.g. LATEST
    //	DEVICE::pulse		prefixed by a device name
    //	DEVICE::pulse/pass
    //
    //	FORMAT::/path/to/my/file
    //      FORMAT::./path/to/my/file		use client side resolution of ./ location contained in path otherwise ignore
    //      FORMAT::../path/to/my/file		use client side resolution of ../ location
    //	FORMAT::/scratch/path/to/my/file	use client side resolution of /scratch location (change name via environment variable)
    //
    //	FORMAT::pulse		FORMAT is the default FORMAT, e.g. IDA3
    //	FORMAT::/pulse
    //	FORMAT::/pulse/pass
    //	/pulse
    //	/pulse/pass
    //
    //	DEVICE::FORMAT::/path/to/my/file	Passed on to a different IDAM server without interpretation
    //	DEVICE::FORMAT::pulse

    //	/path/to/my/file.ext			use file extension 'ext' to identify the correct FORMAT if known
    //      ./path/to/my/file.ext
    //      ../path/to/my/file.ext
    //	/scratch/path/to/my/file.ext
    //
    //	PROTOCOL::server.host.name:port/U/R/L	server access requests - always requires the delimiter string element in string
    //
    //	function(arguments or name value pair list)		server side processing of data
    //	LIBRARY::function(arguments or name value pair list)	function plugin library
    //	DEVICE::function(arguments or name value pair list)	Not allowed - use DEVICE::SERVERSIDE::function()
    //
    //	DEVICE::FORMAT:: ...			If the DEVICE is not the default device, then a server protocol is invoked to pass
    //                                              the request forward (FORMAT:: ...)
    //
    // Legacy exception: treat PPF and JPF formats as server protocols => no file path expansion required and ignored
    //
    //      PPF::/ddaname/pulse/pass/userid or PPF::ddaname/pulse/pass/userid
    //	JPF::pulse or JPF::/pulse
    //
    //------------------------------------------------------------------------------
    // Scenario #1: Format or protocol or library is present - there are no delimiters in the source string

    bool isFunction = false;
    bool isFile = false;
    bool isServer = false;
    bool isForeign = false;

    char* test = strstr(request->source, request->api_delim);    // Delimiter present?
    if (test != nullptr) {
        strncpy(work2, request->source, test - request->source);
        work2[test - request->source] = '\0';
        TrimString(work2);
        strcpy(work, test + ldelim);
    } else {
        work2[0] = '\0';
        strcpy(work, request->source);
    }

    // Test for DEVICE::LIBRARY::function(argument)	 - More delimiter characters present?

    char* p;
    if (test != nullptr && STR_IEQUALS(work2, environment->api_device) &&
        (p = strstr(work, request->api_delim)) != nullptr) {
        lstr = (p - work);
        strncpy(work2, work, lstr);        // Ignore the default device name - force a pass to Scenario 2
        work2[lstr] = '\0';
        TrimString(work2);
        lstr = lstr + ldelim;
        for (size_t i = 0; i < lstr; i++) {
            work[i] = ' ';
        }
        LeftTrimString(work);
    }

    bool reduceSignal;

    do {

        if (noSource) {
            // No Source
            strcpy(request->device_name, environment->api_device);    // Default Device Name
            break;
        }

        if (test == nullptr ||
            STR_IEQUALS(work2, environment->api_device)) {    // No delimiter present or default device?

            UDA_LOG(UDA_LOG_DEBUG, "No device name or format or protocol or library is present\n");

            strcpy(request->device_name, environment->api_device);            // Default Device Name

            // Regular request: pulse or pulse/pass ==> Generic request

            if (generic_request_test(work, request)) break;

            // Not a Server Side Function? 		Note: /a/b/fun(aaa) is a (bad!)file path and fun(a/b/c) is a function

            char* p0 = strchr(work, '/');        // Path separator mixed with parenthesis?
            char* p1 = strrchr(work, '/');
            p = strchr(work, '(');
            char* p2 = strchr(work, ')');

            if (p == nullptr || p2 == nullptr || (p != nullptr && p2 == nullptr) || (p == nullptr && p2 != nullptr) ||
                (p0 != nullptr && p != nullptr && p0 < p) || (p1 != nullptr && p2 != nullptr && p1 > p2)) {

                if ((p0 != nullptr || p1 != nullptr) && (p != nullptr || p2 != nullptr)) {
                    err = 999;
                    addIdamError(CODEERRORTYPE, "makeServerRequestBlock", err,
                                 "Source syntax: path with parenthesis () is incorrect!");
                    return err;
                }

                // Request must be a private file format. It cannot be a local/remote server protocol.

                UDA_LOG(UDA_LOG_DEBUG, "No File Format has been specified. Selecting ....\n");

                int rc = source_file_format_test(request->source, request, pluginList, environment);

#ifdef JETSERVER
                if (rc < 0) {
                    strcpy(request->format, "PPF");       // Assume the Default Format (PPF?)
                    for (int i = 0; i < pluginList.count; i++) {
                        if (STR_IEQUALS(request->format, pluginList.plugin[i].format)) {
                            request->request = pluginList.plugin[i].request;
                            break;
                        }
                    }
                    test = request->source;   // No prefix nor delimiter
                    ldelim = 0;                     // No offset required
                    rc = 1;
                }
#endif

                if (rc <= 0) {
                    UDA_LOG(UDA_LOG_DEBUG, "File Format NOT identified from name extension!\n");
                    THROW_ERROR(999, "No File Format identified: Please specify.");
                }

                // Resolve any Serverside environment variables
                expand_environment_variables(request->path);

                UDA_LOG(UDA_LOG_DEBUG, "File Format identified from name extension!\n");
                break;

            } else {

                // Internal Server Side Function ?	A file path may contain characters ( and ) !

                if ((p = strchr(work, '(')) != nullptr && strchr(p, ')') != nullptr) {
                    strcpy(work2, &p[1]);
                    p = strchr(work2, ')');
                    p[0] = '\0';
                    LeftTrimString(work2);
                    TrimString(work2);

                    request->request = REQUEST_READ_SERVERSIDE;
                    extract_function_name(work, request);

                    UDA_LOG(UDA_LOG_DEBUG, "**** Server Side Function ??? ****\n");

                    // Extract Name Value pairs

                    if (name_value_pairs(work2, &request->nameValueList, strip) == -1) {
                        THROW_ERROR(999, "Name Value pair syntax is incorrect!");
                    }

                    // Test for external library functions using the Archive name as the library name identifier

                    reduceSignal = false;
                    extract_archive(request, reduceSignal, environment);

                    for (int i = 0; i < pluginList.count; i++) {
                        if (STR_IEQUALS(request->archive, pluginList.plugin[i].format)) {
                            request->request = pluginList.plugin[i].request;                // Found!
                            strcpy(request->format, pluginList.plugin[i].format);
                            break;
                        }
                    }
                    break;

                } else {
                    THROW_ERROR(999, "No Data Access Plugin Identified!");
                }
            }

        } else {

            //---------------------------------------------------------------------------------------------------------------------
            // Scenario #2: A foreign device name or format or protocol or library is present

            UDA_LOG(UDA_LOG_DEBUG, "A device name or format or protocol or library is present.\n");

            // Test for known File formats, Server protocols or Libraries or Devices

            for (int i = 0; i < pluginList.count; i++) {
                if (STR_IEQUALS(work2, pluginList.plugin[i].format)) {
                    if (pluginList.plugin[i].plugin_class != UDA_PLUGIN_CLASS_DEVICE) {
                        request->request = pluginList.plugin[i].request;        // Found
                        strcpy(request->format, pluginList.plugin[i].format);
                        if (pluginList.plugin[i].plugin_class !=
                            UDA_PLUGIN_CLASS_FILE) {            // The full file path fully resolved by the client
                            strcpy(request->path,
                                   test + ldelim);            // Complete String following :: delimiter
                            strcpy(request->file, "");                // Clean the filename
                            if (pluginList.plugin[i].plugin_class == UDA_PLUGIN_CLASS_FUNCTION) {
                                isFunction = true;
                                extract_function_name(work, request);
                            }
                        } else {
#ifndef __GNUC__
                            char base[1024] = { 0 };
                            _splitpath(test + ldelim, NULL, base, NULL, NULL);
#else
                            char* base = basename(test + ldelim);
#endif
                            strcpy(request->file, base);    // Final token
                        }
                        isFile = pluginList.plugin[i].plugin_class == UDA_PLUGIN_CLASS_FILE;
                        isServer = pluginList.plugin[i].plugin_class == UDA_PLUGIN_CLASS_SERVER;
                        break;
                    } else {

                        // Registered Devices and their access Server Protocols
                        // Identify the server protocol - it must be a server protocol!
                        // Substitute the Device name with the protocol and server details

                        static int depth = 0;
                        //int id = findPluginRequestByFormat(pluginList.plugin[i].deviceProtocol, &pluginList);
                        int id = find_plugin_id_by_format(pluginList.plugin[i].deviceProtocol, &pluginList);
                        if (id >= 0 && pluginList.plugin[id].plugin_class == UDA_PLUGIN_CLASS_SERVER) {

                            sprintf(work, "%s%s%s", pluginList.plugin[i].deviceProtocol, request->api_delim,
                                    pluginList.plugin[i].deviceHost);
                            UDA_LOG(UDA_LOG_DEBUG, "work#1: %s\n", work);

                            if (pluginList.plugin[i].devicePort[0] != '\0') {
                                strcat(work, ":");
                                strcat(work, pluginList.plugin[i].devicePort);
                            }
                            UDA_LOG(UDA_LOG_DEBUG, "work#2: %s\n", work);
                            UDA_LOG(UDA_LOG_DEBUG, "test: %s\n", test);
                            UDA_LOG(UDA_LOG_DEBUG, "test+ldelim: %s\n", test + ldelim);

                            if ((test + ldelim)[0] != '/') {
                                if ((test + ldelim)[0] != '\0') {
                                    strcat(work, "/");
                                    strcat(work, test + ldelim);
                                }
                                UDA_LOG(UDA_LOG_DEBUG, "work#3: %s\n", work);
                            } else {
                                strcat(work, test + ldelim);
                            }
                            UDA_LOG(UDA_LOG_DEBUG, "work#4: %s\n", work);

                            strcpy(request->source, work);
                            if (depth++ > MAXREQDEPTH) {
                                THROW_ERROR(999, "Too many chained Device Name to Server Protocol Host subtitutions!");
                            }
                            err = makeRequestData(request, pluginList, environment);
                            depth--;
                            return err;
                        }
                    }
                }
            }

            // If no match was found then the prefix must be a foreign Device Name not entered in the server configuration file.
            // The request must be a generic lookup of how to access data remotely for the specified device.
            // The external server providing access to the foreign device's data will interpret the arguments

            if (request->request == REQUEST_READ_UNKNOWN) {
                UDA_LOG(UDA_LOG_DEBUG, "No plugin was identified for the format: %s\n", work2);
                isForeign = true;
                strcpy(request->device_name, work2);                // Copy the DEVICE prefix
                request->request = REQUEST_READ_GENERIC;            // The database will identify the target

                break;
            }

            // A match was found: The Source must be a format or a protocol or a library

            strcpy(request->device_name, environment->api_device);            // Default Device Name

            if (isFile) {                                    // Resolve any Serverside environment variables
                UDA_LOG(UDA_LOG_DEBUG, "File Format has been specified.\n");
                expand_environment_variables(request->path);
                break;
            }

            if (!isFile && !isFunction) {        // Server Protocol
                UDA_LOG(UDA_LOG_DEBUG, "Server Protocol\n");
                break;
            }

            // Must be a function
            // Test syntax

            char* p0 = strchr(work, '/');        // Path separator mixed with parenthesis?
            char* p1 = strrchr(work, '/');        // Path separator mixed with parenthesis?
            p = strchr(work, '(');
            char* p2 = strchr(work, ')');

            if (p == nullptr || p2 == nullptr || (p != nullptr && p2 == nullptr) || (p == nullptr && p2 != nullptr) ||
                (p0 != nullptr && p != nullptr && p0 < p) || (p1 != nullptr && p2 != nullptr && p1 > p2)) {
                THROW_ERROR(999, "Not a function when one is expected! - A Library plugin has been specified.");
            }

            // ToDo: Extract Data subset operations specified within the source argument

            // Extract Name Value pairs

            if ((p = strchr(work, '(')) != nullptr && strchr(p, ')') != nullptr) {
                strcpy(work, &p[1]);
                p = strchr(work, ')');
                p[0] = '\0';
                LeftTrimString(work);
                TrimString(work);

                // Extract Name Value pairs

                if (name_value_pairs(work, &request->nameValueList, strip) == -1) {
                    THROW_ERROR(999, "Name Value pair syntax is incorrect!");
                }

                // ToDo: Extract Data subset operations specified as a named value pair, tagged 'subset'

            } else {
                THROW_ERROR(999, "Function syntax error - please correct");
            }
        }
    } while (0);

    UDA_LOG(UDA_LOG_DEBUG, "Signal Argument\n");

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
        THROW_ERROR(999, "Subset operation is incorrect!");
    }

    // as at 19Apr2011 no signals recorded in the IDAM database use either [ or { characters
    // there will be no confusion between subset operations and valid signal names

    if (rc == 1) {        // the subset has valid syntax so reduce the signal name by removing the subset instructions
        p = strstr(request->signal, request->subset);
        if (p != nullptr) p[0] = '\0';            // Remove subset operations from variable name
        TrimString(request->signal);
    } else {
        request->subset[0] = '\0';
        request->datasubset.subsetCount = 0;
    }

    //------------------------------------------------------------------------------
    // Extract the Archive Name and detach from the signal  (detachment is necessary when not passing on to another IDAM server)
    // the Plugin Name is synonymous with the Archive Name and takes priority (The archive name is discarded as unimportant)

    if (request->request == REQUEST_READ_IDAM) {
        reduceSignal = false;
        err = extract_archive(request, reduceSignal, environment);
    } else {
        reduceSignal = !isForeign;                // Don't detach if a foreign device
        err = extract_archive(request, reduceSignal, environment);
    }
    if (request->archive[0] == '\0') {
        strcpy(request->archive, environment->api_archive);
    }

    //------------------------------------------------------------------------------
    // Extract Name Value Pairs from the data object (signal) without modifying

    // as at 22Sep2011 261 signals recorded in the IDAM database used parenthesis characters so could be confused
    // with function requests. However, for all these cases a source term would be specified. No library
    // would be part of this specification so there would be no ambiguity.

    isFunction = false;

    if (!isServer && (p = strchr(request->signal, '(')) != nullptr && strchr(p, ')') != nullptr &&
        strcasecmp(request->archive, environment->api_archive) != 0) {
        strcpy(work, &p[1]);
        if ((p = strrchr(work, ')')) != nullptr) {
            p[0] = '\0';
            LeftTrimString(work);
            TrimString(work);
            isFunction = true;
            if (name_value_pairs(work, &request->nameValueList, strip) == -1) {
                THROW_ERROR(999, "Name Value pair syntax is incorrect!");
            }
            extract_function_name(request->signal, request);
        }
    }

    //------------------------------------------------------------------------------
    // If No Source was Specified: All requirements are contained in the signal string

    if (noSource) {
        UDA_LOG(UDA_LOG_DEBUG, "Signal Argument - No Source\n");
        UDA_LOG(UDA_LOG_DEBUG, "request: %d\n", request->request);

        // If the signal could be a function call, check the archive name against the function library plugins

        if (isFunction && err == 0) {        // Test for known Function Libraries
            isFunction = false;
            for (int i = 0; i < pluginList.count; i++) {
                if (STR_IEQUALS(request->archive, pluginList.plugin[i].format)) {
                    request->request = pluginList.plugin[i].request;            // Found
                    strcpy(request->format, pluginList.plugin[i].format);
                    isFunction = (pluginList.plugin[i].plugin_class ==
                                  UDA_PLUGIN_CLASS_FUNCTION);        // Must be a known Library
                    break;
                }
            }

            UDA_LOG(UDA_LOG_DEBUG, "A request: %d\n", request->request);
            UDA_LOG(UDA_LOG_DEBUG, "isFunction: %d\n", isFunction);

            if (!isFunction) {                // Must be a default server-side function
                for (int i = 0; i < pluginList.count; i++) {
                    if (STR_IEQUALS(pluginList.plugin[i].symbol, "SERVERSIDE") &&
                        pluginList.plugin[i].library[0] == '\0') {
                        request->request = REQUEST_READ_SERVERSIDE;            // Found
                        strcpy(request->format, pluginList.plugin[i].format);
                        isFunction = true;
                        break;
                    }
                }
                if (!isFunction) request->function[0] = '\0';
            }

            UDA_LOG(UDA_LOG_DEBUG, "B request: %d\n", request->request);

        } else {
            // Select the Generic plugin: No source => no format or protocol or library was specified.
            request->request = REQUEST_READ_GENERIC;
            UDA_LOG(UDA_LOG_DEBUG, "C request: %d\n", request->request);
        }

    } else {

        // Does the data object (Signal) have the form: LIBRARY::function?
        // Exception is Serverside function

        if (isFunction && strcasecmp(request->archive, environment->api_archive) != 0) {
            int id = find_plugin_id_by_format(request->archive, &pluginList);
            if (id >= 0 && pluginList.plugin[id].plugin_class == UDA_PLUGIN_CLASS_FUNCTION &&
                strcasecmp(pluginList.plugin[id].symbol, "serverside") != 0) {
                if (request->request == REQUEST_READ_GENERIC ||
                    request->request == REQUEST_READ_UNKNOWN) {
                    request->request = pluginList.plugin[id].request;    // Found
                    strcpy(request->format, pluginList.plugin[id].format);
                    UDA_LOG(UDA_LOG_DEBUG, "D request: %d\n", request->request);
                } else {
                    if (request->request != pluginList.plugin[id].request) {    // Inconsistent
                        // Let Source have priority over the Signal?
                        UDA_LOG(UDA_LOG_DEBUG, "Inconsistent Plugin Libraries: Source selected over Signal\n");
                    }
                }
            }
        }
    }
    UDA_LOG(UDA_LOG_DEBUG, "E request: %d\n", request->request);

    //---------------------------------------------------------------------------------------------------------------------
    // MDS+ Servers ...

    // MDS+ Source naming models:	MDS+::localhost/tree/number	any source with one or more / must have a trailing number
    // 				MDS+::server/tree/number
    //				MDS+::server/path/to/data/tree/number
    //				MDS+::server/path.to.data/tree/number
    //				MDS+::/path/to/data/tree/number
    //				MDS+::/path.to.data/tree/number
    // 				MDS+::tree/number
    //				MDS+::server
    //				MDS+::

    if (request->request == REQUEST_READ_MDS && !isProxy) {

        reverseString(test + ldelim, work);            // Drop the delimiters and Reverse the Source String

        char* token;
        if ((token = strtok(work, "/")) != nullptr) {        // Tokenise
            if (IsNumber(token)) {                // This should be the tree Number otherwise only the server is passed
                reverseString(token, work2);            // Un-Reverse the token
                request->exp_number = (int)strtol(work2, nullptr, 10);    // Extract the Data Tree Number
                if ((token = strtok(nullptr, "/")) != nullptr) {
                    reverseString(token, request->file);    // This should be the Tree Name
                    work2[0] = '\0';
                    while ((token = strtok(nullptr, "/")) != nullptr) {
                        // Everything Else is the Server Host and URL Path to the Tree
                        strcat(work2, token);
                        strcat(work2, "/");
                    }
                    if (work2[0] == '/') strcpy(work2, &work2[1]);    // Drop Trailing /
                    reverseString(work2, request->server);
                    token = test + ldelim;                // Preserve Leading /
                    if (token[0] != '/' && request->server[0] == '/') {
                        request->server[0] = ' ';
                        LeftTrimString(request->server);
                    }
                } else {
                    err = 3;
                }
            } else {
                strcpy(request->server, test + ldelim);    // Server or null string (default server)
            }

        } else {
            strcpy(request->server, "");            // Default Server
        }

        if (err != 0) {
            THROW_ERROR(NO_SERVER_SPECIFIED,
                        "The MDSPlus Data Source does not comply with the naming models: server/tree/number or server/path/to/data/tree/number");
        }
    }

    //---------------------------------------------------------------------------------------------------------------------
    // IDAM and WEB Servers ...      parse source modelled as: server:port/source

    if (request->request == REQUEST_READ_IDAM || request->request == REQUEST_READ_WEB) {
        strcpy(work, test + ldelim);                // Drop the delimiters

        // Isolate the Server from the source IDAM::server:port/source or SSL://server:port/source

        strcpy(request->server, work);

        char* s = nullptr;
        if ((s = strstr(work, "SSL://")) != nullptr) {
            char* token;
            if ((token = strstr(s + 6, "/")) != nullptr) {
                token[0] = '\0';                // Break the String (work)
                strcpy(request->server, s);        // Extract the Server Name and Port (with SSL:// prefix)
                strcpy(request->file, token + 1);        // Extract the Source URL Argument
            }
        } else {
            char* token;
            if ((token = strstr(work, "/")) != nullptr) {
                token[0] = '\0';                        // Break the String (work)
                strcpy(request->server, work);        // Extract the Server Name and Port
                strcpy(request->file, token + 1);        // Extract the Source URL Argument
            }
        }

        UDA_LOG(UDA_LOG_DEBUG, "Server: %s\n", request->server);
        UDA_LOG(UDA_LOG_DEBUG, "Source: %s\n", request->file);
    }

    //---------------------------------------------------------------------------------------------------------------------
    // SQL Servers ...

    if (request->request == REQUEST_READ_SQL) {
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

int make_request_block(REQUEST_BLOCK* request_block, PLUGINLIST pluginList, const ENVIRONMENT* environment)
{
    int rc = 0;

    for (int i = 0; i < request_block->num_requests; ++i) {
        auto request = &request_block->requests[0];
        rc = makeRequestData(request, pluginList, environment);
        if (rc != 0) {
            break;
        }
    }

    return rc;
}

void extract_function_name(const char* str, REQUEST_DATA* request)
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
            TrimString(work);
            LeftTrimString(work);
        } while ((p = strstr(work, request->api_delim)) != nullptr);
    }
    strcpy(request->function, work);
    free(work);
}

/**
 * returns true if a format was identified, false otherwise.
 */
int source_file_format_test(const char* source, REQUEST_DATA* request, PLUGINLIST pluginList,
                            const ENVIRONMENT* environment)
{
    int rc = 0;
    const char* test;

    // .99		IDA3 file
    // .nc		netCDF4
    // .cdf		netCDF3
    // .hf		HDF
    // .jpg		Binary
    // .csv		ASCII

    // Note: On JET the default data source is PPF. This is really a server protocol and not a file format.

    //------------------------------------------------------------------------------
    // Start with ignorance about which plugin to use

    request->format[0] = '\0';
    request->file[0] = '\0';
    request->request = REQUEST_READ_UNKNOWN;

    if (source[0] == '\0') return rc;

    // Does the path contain any Illegal (or problem) characters

    if (!IsLegalFilePath((char*)source)) {
        return rc;        // Not compliant with Portable Filename character set
    }

    // Does the source have a file extension? If so choose the format using the extension, otherwise investigate the file.

    if ((test = strrchr(source, '.')) == nullptr) {

        // No extension => test the first line of file, e.g. head -c10 <file>, but both netcdf and hdf5 use the same label HDF!

#ifndef _WIN32
        const char* nc = " nc";
        const char* hf = " hf";
        const char* ida = " 99";
        const char* blank = "   ";
        FILE* ph = nullptr;
        int lstr = STRING_LENGTH;
        char* cmd = (char*)malloc(lstr * sizeof(char));
        sprintf(cmd, "head -c10 %s 2>/dev/null", source);
        errno = 0;
        if ((ph = popen(cmd, "r")) == nullptr) {
            if (errno != 0) addIdamError(SYSTEMERRORTYPE, "sourceFileFormatTest", errno, "");
            addIdamError(CODEERRORTYPE, "sourceFileFormatTest", 999,
                         "Unable to Identify the File's Format");
            free(cmd);
            return -999;
        }

        cmd[0] = '\0';
        if (!feof(ph)) fgets(cmd, lstr - 1, ph);
        fclose(ph);

        test = blank;
        convertNonPrintable2(cmd);
        LeftTrimString(cmd);
        TrimString(cmd);

        if (STR_EQUALS(cmd, "CDF")) {    // Legacy netCDF file
            test = nc;
        } else {
            if (STR_EQUALS(cmd, "HDF")) {    // Either a netCDF or a HDF5 file: use utility programs to reveal!
                char* env = getenv("UDA_DUMP_NETCDF");
                if (env != nullptr) {
                    sprintf(cmd, "%s -h %s 2>/dev/null | head -c10 2>/dev/null", env, source);
                    errno = 0;
                    if ((ph = popen(cmd, "r")) == nullptr) {
                        if (errno != 0) {
                            addIdamError(SYSTEMERRORTYPE, "sourceFileFormatTest", errno, "");
                        }
                        addIdamError(CODEERRORTYPE, "sourceFileFormatTest", 999,
                                     "Unable to Identify the File's Format");
                        free(cmd);
                        return -999;
                    }

                    cmd[0] = '\0';
                    if (!feof(ph)) fgets(cmd, lstr - 1, ph);
                    fclose(ph);
                    convertNonPrintable2(cmd);
                    LeftTrimString(cmd);
                    TrimString(cmd);

                    if (STR_EQUALS(cmd, "netcdf")) {        // netCDF file written to an HDF5 file
                        test = nc;
                    } else {
                        if (cmd[0] == '\0') test = hf;        // HDF5 file
                    }
                }
            } else {                    // an IDA File?
                char* env = getenv("UDA_DUMP_IDA");
                if (env != nullptr) {
                    sprintf(cmd, "%s -h %s 2>/dev/null 2>/dev/null", env, source);
                    errno = 0;
                    if ((ph = popen(cmd, "r")) == nullptr) {
                        if (errno != 0) {
                            ADD_SYS_ERROR("");
                        }
                        ADD_ERROR(999, "Unable to Identify the File's Format");
                        free(cmd);
                        return -999;
                    }

                    cmd[0] = '\0';
                    if (!feof(ph)) {
                        fgets(cmd, lstr - 1, ph);
                    }        // IDA3 interface version V3.13 with file structure IDA3.1
                    if (!feof(ph)) fgets(cmd, lstr - 1, ph);        // Build JW Jan 25 2007 09:08:47
                    if (!feof(ph)) fgets(cmd, lstr - 1, ph);        // Compiled without high level read/write CUTS
                    if (!feof(ph)) fgets(cmd, lstr - 1, ph);        // Opening ida file
                    if (!feof(ph)) fgets(cmd, lstr - 1, ph);        // ida_open error ?
                    fclose(ph);
                    convertNonPrintable2(cmd);
                    LeftTrimString(cmd);
                    TrimString(cmd);
                    if (strncmp(cmd, "ida_open error", 14) != 0)test = ida;    // Legacy IDA file

                }
            }
        }
        free(cmd);

#else
        return rc;
#endif
    }

    // Select the format

    do {

        // Test against Registered File Extensions
        // TO DO: make extensions a list of valid extensions to minimise plugin duplication

        int breakAgain = 0;
        for (int i = 0; i < pluginList.count; i++) {
            if (STR_IEQUALS(&test[1], pluginList.plugin[i].extension)) {
                strcpy(request->format, pluginList.plugin[i].format);
                breakAgain = 1;
                break;
            }
        }
        if (breakAgain)break;

        // Other regular types

        if (strlen(&test[1]) == 2 && IsNumber(&test[1])) {        // an integer number?
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

        if (source[0] == '/' && source[1] != '\0' && isdigit(source[1])) {        // Default File Format?
            if (generic_request_test(&source[1], request)) {        // Matches 99999/999
                request->request = REQUEST_READ_UNKNOWN;
                strcpy(request->format, environment->api_format);        // the default Server File Format
                break;
            }
        }

        return -1;        // No format identified

    } while (0);

    // Test for known registered plugins for the File's format

    for (int i = 0; i < pluginList.count; i++) {
        if (STR_IEQUALS(request->format, pluginList.plugin[i].format)) {
            rc = 1;
            UDA_LOG(UDA_LOG_DEBUG, "Format identified, selecting specific plugin for %s\n", request->format);
            request->request = pluginList.plugin[i].request;            // Found
            if (pluginList.plugin[i].plugin_class !=
                UDA_PLUGIN_CLASS_FILE) {                // The full file path fully resolved by the client
                strcpy(request->file, "");                        // Clean the filename
            } else {
#ifndef __GNUC__
                char base[1024] = { 0 };
                _splitpath(request->source, NULL, base, NULL, NULL);
#else
                char* base = basename(request->source);
#endif
                strcpy(request->file, base);    // Final token
            }
            break;
        }
    }

    return rc;
}

/**
 * Return true if the Generic plugin was selected, false otherwise
 */
int generic_request_test(const char* source, REQUEST_DATA* request)
{
    int rc = 0;
    char* token = nullptr;
    char work[STRING_LENGTH];

    //------------------------------------------------------------------------------
    // Start with ignorance about which plugin to use

    request->format[0] = '\0';
    request->file[0] = '\0';
    request->request = REQUEST_READ_UNKNOWN;

    if (source[0] == '\0') return rc;
    if (source[0] == '/') return rc;        // Directory based data

    //------------------------------------------------------------------------------
    // Check if the source has one of these forms:

    // pulse		plasma shot number - an integer
    // pulse/pass		include a pass or sequence number - this may be a text based component, e.g. LATEST

    if (IsNumber((char*)source)) {                    // Is the source an integer number?
        rc = 1;
        request->request = REQUEST_READ_GENERIC;
        strcpy(request->path, "");                    // Clean the path
        request->exp_number = (int)strtol(source, nullptr, 10);                // Plasma Shot Number
        UDA_LOG(UDA_LOG_DEBUG, "exp number identified, selecting GENERIC plugin.\n");
    } else {
        strcpy(work, source);
        if ((token = strtok(work, "/")) != nullptr) {                // Tokenise the remaining string
            if (IsNumber(token)) {                        // Is the First token an integer number?
                rc = 1;
                request->request = REQUEST_READ_GENERIC;
                strcpy(request->path, "");                // Clean the path
                request->exp_number = (int)strtol(token, nullptr, 10);
                if ((token = strtok(nullptr, "/")) != nullptr) {            // Next Token
                    if (IsNumber(token)) {
                        request->pass = (int)strtol(token, nullptr, 10);            // Must be the Pass number
                    } else {
                        strcpy(request->tpass, token);            // capture anything else
                    }
                }
                UDA_LOG(UDA_LOG_DEBUG, "exp number and pass id identified, selecting GENERIC plugin.\n");
            }
        }
    }

    return rc;
}

//------------------------------------------------------------------------------
// Strip out the Archive or Plugin name from the data_object name
// syntax:	ARCHIVE::Data_OBJECT or DATA_OBJECT
//		ARCHIVE::PLUGIN::Function() or PLUGIN::Function()
// conflict:	ARCHIVE::DATA_OBJECT[::] or DATA_OBJECT[::] subsetting operations
//
// NOTE: Archive/Plugin Name should not terminate with the character [ or { when a signal begins with the
//       character ] or }. These clash with subsetting syntax.
//
// Input Argument: reduceSignal - If TRUE (1) then extract the archive name and return the data object name
//                                without the prefixed archive name.

int extract_archive(REQUEST_DATA* request, int reduceSignal, const ENVIRONMENT* environment)
{
    int err = 0, test1, test2;
    int ldelim = (int)strlen(request->api_delim);
    char* test, * token, * work;

    TrimString(request->signal);

    if (request->signal[0] != '\0' && environment->server_proxy[0] == '\0') {

        UDA_LOG(UDA_LOG_DEBUG, "Testing for ARCHIVE::Signal\n");

        if ((test = strstr(request->signal, request->api_delim)) != nullptr) {

            if (test - request->signal >= STRING_LENGTH - 1 || strlen(test + ldelim) >= MAXMETA - 1) {
                ADD_ERROR(ARCHIVE_NAME_TOO_LONG, "The ARCHIVE Name is too long!");
                return err;
            }
            strncpy(request->archive, request->signal, test - request->signal);
            request->archive[test - request->signal] = '\0';
            TrimString(request->archive);

            // If a plugin is prefixed by the local archive name then discard the archive name
            if (reduceSignal && !strcasecmp(request->archive, environment->api_archive)) {
                request->archive[0] = '\0';
                strcpy(request->signal, &test[ldelim]);
                return extract_archive(request, reduceSignal, environment);
            }

            if (!IsLegalFilePath(request->archive)) {
                request->archive[0] = '\0';
                return 0;
            }

            // Test the proposed archive name for conflict with subsetting operation

            test1 = 0;
            test2 = 0;

            if ((token = strchr(request->archive, '[')) != nullptr ||
                (token = strchr(request->archive, '{')) != nullptr) {
                test1 = (strlen(&token[1]) == 0 || IsNumber(&token[1]));
            }

            if ((token = strchr(test + ldelim, ']')) != nullptr ||
                (token = strchr(test + ldelim, '}')) != nullptr) {
                work = (char*)malloc((strlen(test + ldelim) + 1) * sizeof(char));
                strcpy(work, test + ldelim);
                work[token - (test + ldelim)] = '\0';
                test2 = (strlen(work) == 0 || IsNumber(work));
                free(work);
            }

            if (!test1 && !test2) {
                if (reduceSignal) {
                    work = (char*)malloc((strlen(test + ldelim) + 1) * sizeof(char));
                    strcpy(work, test + ldelim);
                    strcpy(request->signal, work);    // Valid Archive & signal
                    free(work);
                    TrimString(request->signal);
                }
            } else {
                request->archive[0] = '\0';            // Reset Archive
            }

            UDA_LOG(UDA_LOG_DEBUG, "Archive %s\n", request->archive);
            UDA_LOG(UDA_LOG_DEBUG, "Signal  %s\n", request->signal);
        }
    }
    return err;
}

//------------------------------------------------------------------------------
// Does the Path contain with an Environment variable

void expand_environment_variables(char* path)
{
    size_t lcwd = STRING_LENGTH - 1;
    char work[STRING_LENGTH];
    char cwd[STRING_LENGTH];
    char ocwd[STRING_LENGTH];

    if (strchr(path, '$') == nullptr) {
        UDA_LOG(UDA_LOG_DEBUG, "No embedded environment variables detected\n");
        return;
    }

    if (getcwd(ocwd, lcwd) == nullptr) {    // Current Working Directory
        UDA_LOG(UDA_LOG_DEBUG, "Unable to identify PWD!\n");
        return;
    }

    if (chdir(path) == 0) {            // Change to path directory
        // The Current Working Directory is now the resolved directory name
        char* pcwd = getcwd(cwd, lcwd);

        UDA_LOG(UDA_LOG_DEBUG, "Expanding embedded environment variable:\n");
        UDA_LOG(UDA_LOG_DEBUG, "from: %s\n", path);
        UDA_LOG(UDA_LOG_DEBUG, "to: %s\n", cwd);

        if (pcwd != nullptr) {
            strcpy(path, cwd);    // The expanded path
        }
        chdir(ocwd);                        // Return to the Original WD
    } else {
        UDA_LOG(UDA_LOG_DEBUG, "expandEnvironmentvariables: Direct substitution! \n");

        char* fp = nullptr, * env, * fp1;
        char work1[STRING_LENGTH];

        if (path[0] == '$' ||
            (fp = strchr(&path[1], '$')) != nullptr) {    // Search for a $ character

            if (fp != nullptr) {
                strncpy(work, path, fp - path);
                work[fp - path] = '\0';

                if ((fp1 = strchr(fp, '/')) != nullptr) {
                    strncpy(work1, fp + 1, fp1 - fp - 1);
                    work1[fp1 - fp - 1] = '\0';
                } else { strcpy(work1, fp + 1); }

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
                    strncpy(work, path + 1, fp - path - 1);
                    work[fp - path - 1] = '\0';
                    strcpy(work1, fp);
                } else { strcpy(work, path + 1); }

                if ((env = getenv(work)) != nullptr) {    // Check for Environment Variable
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

        UDA_LOG(UDA_LOG_DEBUG, "Expanding to: %s\n", path);
    }
}

//----------------------------------------------------------------------
// Parse subset instructions- [start:end:stride] or {start:end:stride}
//
// Signal should avoid using subset like components in their name

int extract_subset(REQUEST_DATA* request)
{
    // Return codes:
    //
    //	1 => Valid subset
    //	0 => Not a subset operation - Not compliant with syntax
    //     -1 => Error

    int rc = 1, lwork, subsetCount = 1;        // Number of subsetting operations
    char* p, * work, * token = nullptr;

    request->subset[0] = '\0';
    request->datasubset.subsetCount = 0;

    if ((token = strchr(request->signal, '[')) == nullptr &&
        (token = strchr(request->signal, '{')) == nullptr) {
        return 0;
    }
    if ((work = strrchr(request->signal, ']')) == nullptr &&
        (work = strrchr(request->signal, '}')) == nullptr) {
        return 0;
    }
    if (work < token) return 0;

    // Test the subset operation specified complies with the syntax: [start:end:stride]
    // Parse for detailed instructions

    lwork = (int)strlen(token);
    work = (char*)malloc((lwork + 3) * sizeof(char));

    strcpy(work, token);
    copyString(token, request->subset, STRING_LENGTH - 1);

    work[0] = ' ';        // Remove lead/trailing brackets
    work[lwork - 1] = ' ';

    lwork = lwork + 2;        // expand "::" to "0:*:"

    //----------------------------------------------------------------------------------------------------------------------------
    // Split instructions using syntax [a:b:c][d:e:f] or [a:b:c, d:e:f] where [startIndex:stopIndex:stride]
    //
    // Syntax	[a]		single items at index position a
    //		[*]		all items
    //		[]		all items
    //
    //		[:]		all items starting at 0
    //		[a:]		all items starting at a
    //		[a:*]		all items starting at a
    //		[a:b]		all items starting at a and ending at b
    //
    //		[a::c]		all items starting at a with stride c
    //		[a:*:c]		all items starting at a with stride c
    //		[a:b:c]		all items starting at a, ending at b with stride c

    while ((token = strstr(work, "][")) != nullptr ||
           (token = strstr(work, "}{")) != nullptr) {    // Adopt a single syntax
        token[0] = ',';
        token[1] = ' ';
    }
    p = work;
    while ((token = strchr(p, ',')) != nullptr) {        // Count the Dimensions
        p = &token[1];
        subsetCount++;
    }
    if (subsetCount > MAXRANK2) subsetCount = MAXRANK2;

    // Array of subset instructions for each dimension

    char** work2 = (char**)malloc(subsetCount * sizeof(char*));
    for (int i = 0; i < subsetCount; i++) {
        work2[i] = (char*)malloc(lwork * sizeof(char));
        work2[i][0] = '\0';
    }

    // 3 subset details

    char** work3 = (char**)malloc(3 * sizeof(char*));
    for (int i = 0; i < 3; i++) {
        work3[i] = (char*)malloc(lwork * sizeof(char));
        work3[i][0] = '\0';
    }
    char* work4 = (char*)malloc(lwork * sizeof(char));

    for (int i = 0; i < subsetCount; i++) {
        request->datasubset.start[i] = 0;
        request->datasubset.stop[i] = 0;
        request->datasubset.count[i] = 0;
        request->datasubset.stride[i] = 0;
    }
    request->datasubset.subsetCount = subsetCount;

    subsetCount = 0;
    if ((token = strtok(work, ",")) != nullptr) {    // Process each subset instruction separately (work2)
        strcpy(work2[subsetCount++], token);
        while (subsetCount < MAXRANK2 && (token = strtok(nullptr, ",")) != nullptr) {
            strcpy(work2[subsetCount++], token);
        }

        do {
            for (int i = 0; i < subsetCount; i++) {

                request->datasubset.subset[i] = 0;

                TrimString(work2[i]);
                LeftTrimString(work2[i]);
                for (int j = 0; j < 3; j++)work3[j][0] = '\0';

                if (work2[i][0] == ':') {
                    work4[0] = '0';
                    work4[1] = '\0';
                    if (work2[i][1] != ':') {
                        strcat(work4, work2[i]);
                        strcpy(work2[i], work4);
                    } else {
                        strcat(work4, ":*");
                        strcat(work4, &work2[i][1]);
                        strcpy(work2[i], work4);
                    }
                } else {
                    if ((p = strstr(work2[i], "::")) != nullptr) {
                        p[0] = '\0';
                        strcpy(work4, work2[i]);
                        strcat(work4, ":*");
                        strcat(work4, &p[1]);
                        strcpy(work2[i], work4);
                    } else {
                        strcpy(work4, work2[i]);
                    }
                }

                if (strchr(work2[i], ':') != nullptr && (token = strtok(work2[i], ":")) != nullptr) {
                    int j = 0;
                    strcpy(work3[j++], token);
                    while (j < 3 && (token = strtok(nullptr, ":")) != nullptr) {
                        strcpy(work3[j++], token);
                    }
                    for (int jj = 0; jj < 3; jj++) {
                        TrimString(work3[jj]);
                    }
                    for (int jj = 0; jj < 3; jj++) {
                        LeftTrimString(work3[jj]);
                    }

                    if (work3[0][0] != '\0' && IsNumber(work3[0])) {    // [a:] or [a:*] or [a:b] etc
                        p = nullptr;
                        errno = 0;
                        request->datasubset.start[i] = (int)strtol(work3[0], &p, 10);
                        if (errno != 0 || *p != 0 || p == work3[0]) {
                            rc = 0;
                            break;
                        }
                        if (request->datasubset.start[i] < 0) {
                            ADD_ERROR(999, "Invalid Start Index in subset operation");
                            rc = -1;
                            break;
                        }

                        request->datasubset.stop[i] = request->datasubset.start[i];
                        request->datasubset.count[i] = 1;
                        request->datasubset.stride[i] = 1;
                        request->datasubset.subset[i] = 1;

                    } else {
                        rc = 0;                    // Not an Error - not a subset operation
                        break;
                    }
                    if (work3[1][0] != '\0' && IsNumber(work3[1])) {    // [a:b]
                        p = nullptr;
                        errno = 0;
                        request->datasubset.stop[i] = (int)strtol(work3[1], &p, 10);
                        if (errno != 0 || *p != 0 || p == work3[0]) {
                            rc = 0;
                            break;
                        }
                        if (request->datasubset.stop[i] < 0) {
                            ADD_ERROR(999, "Invalid sample End Index in subset operation");
                            rc = -1;
                            break;
                        }
                        request->datasubset.count[i] =
                                request->datasubset.stop[i] - request->datasubset.start[i] + 1;
                        request->datasubset.subset[i] = 1;

                        if (request->datasubset.stop[i] < request->datasubset.start[i]) {
                            ADD_ERROR(999, "Invalid Stop Index in subset operation");
                            rc = -1;
                            break;
                        }

                    } else {
                        if (strlen(work3[1]) == 0 || (strlen(work3[1]) == 1 && work3[1][0] == '*')) {    // [a:],[a:*]
                            request->datasubset.count[i] = -1;
                            request->datasubset.stop[i] = -1;            // To end of dimension
                        } else {
                            rc = 0;
                            break;
                        }
                    }
                    if (work3[2][0] != '\0') {
                        if (IsNumber(work3[2])) {
                            p = nullptr;
                            errno = 0;
                            request->datasubset.stride[i] = (int)strtol(work3[2], &p, 10);
                            if (errno != 0 || *p != 0 || p == work3[0]) {
                                rc = 0;
                                break;
                            }
                            if (request->datasubset.stride[i] <= 0) {
                                ADD_ERROR(999, "Invalid sample stride length in subset operation");
                                rc = -1;
                                break;
                            }
                            if (request->datasubset.stride[i] > 1)request->datasubset.subset[i] = 1;

                            if (request->datasubset.stride[i] > 1 && request->datasubset.count[i] > 1) {
                                if ((request->datasubset.count[i] % request->datasubset.stride[i]) > 0) {
                                    request->datasubset.count[i] = 1 + request->datasubset.count[i] /
                                                                       request->datasubset.stride[i];
                                } else {
                                    request->datasubset.count[i] =
                                            request->datasubset.count[i] / request->datasubset.stride[i];
                                }
                            }

                        } else {
                            rc = 0;
                            break;
                        }
                    }
                } else {
                    if (work4[0] == '\0' || work4[0] == '*') {        // [], [*]
                        request->datasubset.start[i] = 0;
                        request->datasubset.stop[i] = -1;
                        request->datasubset.count[i] = -1;
                        request->datasubset.stride[i] = 1;
                    } else {
                        if (IsNumber(work4)) {                // [a]
                            p = nullptr;
                            errno = 0;
                            request->datasubset.start[i] = (int)strtol(work4, &p, 10);
                            if (errno != 0 || *p != 0 || p == work3[0]) {
                                rc = 0;
                                break;
                            }
                            if (request->datasubset.start[i] < 0) {
                                ADD_ERROR(999, "Invalid start index in subset operation");
                                rc = -1;
                                break;
                            }

                            request->datasubset.stop[i] = request->datasubset.start[i];
                            request->datasubset.count[i] = 1;
                            request->datasubset.stride[i] = 1;
                            request->datasubset.subset[i] = 1;

                        } else {
                            rc = 0;
                            break;
                        }
                    }
                }
            }
        } while (0);
    } else {
        rc = 0;
    }

    free(work);
    for (int i = 0; i < subsetCount; i++)free(work2[i]);
    free(work2);
    for (int i = 0; i < 3; i++)free(work3[i]);
    free(work3);
    free(work4);

    return rc;
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
void freeNameValueList(NAMEVALUELIST* nameValueList)
{
    if (nameValueList->nameValue != nullptr) {
        for (int i = 0; i < nameValueList->pairCount; i++) {
            if (nameValueList->nameValue[i].pair != nullptr) free(nameValueList->nameValue[i].pair);
            if (nameValueList->nameValue[i].name != nullptr) free(nameValueList->nameValue[i].name);
            if (nameValueList->nameValue[i].value != nullptr) free(nameValueList->nameValue[i].value);
        }
    }
    free(nameValueList->nameValue);
    nameValueList->pairCount = 0;
    nameValueList->listSize = 0;
    nameValueList->nameValue = nullptr;
}

void parse_name_value(const char* pair, NAMEVALUE* nameValue, unsigned short strip)
{
    int lstr;
    char* p, * copy;
    lstr = (int)strlen(pair) + 1;
    copy = (char*)malloc(lstr * sizeof(char));
    strcpy(copy, pair);
    LeftTrimString(copy);
    TrimString(copy);
    nameValue->pair = (char*)malloc(lstr * sizeof(char));
    strcpy(nameValue->pair, copy);
    LeftTrimString(nameValue->pair);
    UDA_LOG(UDA_LOG_DEBUG, "Pair: %s\n", pair);
    if ((p = strchr(copy, '=')) != nullptr) {
        *p = '\0';
        lstr = (int)strlen(copy) + 1;
        nameValue->name = (char*)malloc(lstr * sizeof(char));
        strcpy(nameValue->name, copy);
        lstr = (int)strlen(&p[1]) + 1;
        nameValue->value = (char*)malloc(lstr * sizeof(char));
        strcpy(nameValue->value, &p[1]);
    } else {                    // Mimic IDL keyword passing or stand alone values for placeholder substitution
        UDA_LOG(UDA_LOG_DEBUG, "Keyword or placeholder value: %s\n", copy);
        lstr = (int)strlen(copy) + 1;
        nameValue->name = (char*)malloc(lstr * sizeof(char));
        if (copy[0] == '/') {
            strcpy(nameValue->name, &copy[1]);            // Ignore leader forward slash
            lstr = 5;
            nameValue->value = (char*)malloc(lstr * sizeof(char));
            strcpy(nameValue->value, "true");
            UDA_LOG(UDA_LOG_DEBUG, "Placeholder name: %s, value: %s\n", nameValue->name, nameValue->value);
        } else {
            strcpy(nameValue->name, copy);
            nameValue->value = (char*)malloc(lstr * sizeof(char));
            strcpy(nameValue->value, copy);
            UDA_LOG(UDA_LOG_DEBUG, "Placeholder value: %s\n", nameValue->name);
        }
    }
    LeftTrimString(nameValue->name);
    LeftTrimString(nameValue->value);
    TrimString(nameValue->name);
    TrimString(nameValue->value);
    UDA_LOG(UDA_LOG_DEBUG, "Name: %s     Value: %s\n", nameValue->name, nameValue->value);

    // Regardless of whether or not the Value is not enclosed in quotes, strip out a possible closing parenthesis character (seen in placeholder value substitution)
    // This would not be a valid value unless at the end of a string enclosed in quotes!
    lstr = (int)strlen(nameValue->value);
    if (nameValue->value[lstr - 1] == ')') nameValue->value[lstr - 1] = '\0';
    UDA_LOG(UDA_LOG_DEBUG, "Name: %s     Value: %s\n", nameValue->name, nameValue->value);

    if (strip) {            // remove enclosing single or double quotes
        lstr = (int)strlen(nameValue->name);
        if ((nameValue->name[0] == '\'' && nameValue->name[lstr - 1] == '\'') ||
            (nameValue->name[0] == '"' && nameValue->name[lstr - 1] == '"')) {
            nameValue->name[0] = ' ';
            nameValue->name[lstr - 1] = ' ';
            LeftTrimString(nameValue->name);
            TrimString(nameValue->name);
        }
        lstr = (int)strlen(nameValue->value);
        if ((nameValue->value[0] == '\'' && nameValue->value[lstr - 1] == '\'') ||
            (nameValue->value[0] == '"' && nameValue->value[lstr - 1] == '"')) {
            nameValue->value[0] = ' ';
            nameValue->value[lstr - 1] = ' ';
            LeftTrimString(nameValue->value);
            TrimString(nameValue->value);
        }
        UDA_LOG(UDA_LOG_DEBUG, "Name: %s     Value: %s\n", nameValue->name, nameValue->value);
    }

    free(copy);
}

int name_value_pairs(const char* pairList, NAMEVALUELIST* nameValueList, unsigned short strip)
{
    // Ignore delimiter in anything enclosed in single or double quotes
    // Recognise /name as name=TRUE
    // if strip then remove all enclosing quotes (single or double)

    int lstr, pairCount = 0;
    char proposal, delimiter = ',', substitute = 1;
    char* p, * p2, * p3 = nullptr, * buffer, * copy;
    NAMEVALUE nameValue;
    lstr = (int)strlen(pairList);

    if (lstr == 0) return pairCount;            // Nothing to Parse

    // Placeholder substitution is neither a name-value pair nor a keyword so bypass this test
    //if (strchr(pairList, '=') == nullptr && pairList[0] != '/')
    //    return pairCount;        // Not a Name Value list or Keyword

    if (pairList[0] == '=') return -1;            // Syntax error
    if (pairList[lstr - 1] == '=') return -1;        // Syntax error

    lstr = lstr + 1;
    buffer = (char*)malloc(lstr * sizeof(char));
    copy = (char*)malloc(lstr * sizeof(char));

    strcpy(copy, pairList);        // working copy

    UDA_LOG(UDA_LOG_DEBUG, "Parsing name values from argument: %s\n", pairList);

    // Locate the delimiter name value pair if present - use default character ',' if not

    if ((p = strcasestr(copy, "delimiter")) != nullptr) {
        strcpy(buffer, &p[9]);
        LeftTrimString(buffer);
        if (buffer[0] == '=' && buffer[1] != '\0') {
            buffer[0] = ' ';
            LeftTrimString(buffer);        // remove whitespace
            if (strlen(buffer) >= 3 && (
                    (buffer[0] == '\'' && buffer[2] == '\'') || (buffer[0] == '"' && buffer[2] == '"'))) {
                proposal = buffer[1];        // proposal delimiter
                lstr = (int)(p - copy);
                if (lstr == 0) {            // delimiter name value pair coincident with start of list
                    delimiter = proposal;        // new delimiter
                    p3 = strchr(&p[9], delimiter);    // change delimiter to avert incorrect parse
                    *p3 = '#';
                } else {
                    strncpy(buffer, copy, lstr);    // check 'delimiter' is not part of another name value pair
                    buffer[lstr] = '\0';
                    TrimString(buffer);
                    lstr = (int)strlen(buffer);
                    if (buffer[lstr - 1] == proposal) {        // must be an immediately preceeding delimiter character
                        delimiter = proposal;            // new delimiter accepted
                        p3 = strchr(&p[9], delimiter);        // change delimiter temporarily to avert incorrect parse
                        *p3 = '#';
                    } else {
                        TrimString(buffer);            // Check for non alpha-numeric character
                        lstr = (int)strlen(buffer);
                        if (!isalpha(buffer[lstr - 1]) && !isdigit(buffer[lstr - 1])) {    // Probable syntax error!
                            free(buffer);
                            free(copy);
                            return -1;                // Flag an Error
                        }
                    }
                }
            }
        }
    }

    // lists are enclosed in either single or double quotes. If the list elements are
    // separated using the delimiter character, replace them with a temporary character

    lstr = lstr - 1;
    int isList = 0;
    int isListDelim = 0;
    for (int i = 0; i < lstr; i++) {
        if (copy[i] == '\'' || copy[i] == '"') {
            if (isList) {
                isList = 0;        // Switch substitution off
            } else {
                isList = 1;
            }    // Switch substitution on
        } else {
            if (isList && copy[i] == delimiter) {
                isListDelim = 1;
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

        UDA_LOG(UDA_LOG_DEBUG, "Parsing name value: %s\n", buffer);
        parse_name_value(buffer, &nameValue, strip);
        UDA_LOG(UDA_LOG_DEBUG, "Name %s, Value: %s\n", nameValue.name, nameValue.value);

        //if (nameValue.name != nullptr && nameValue.value != nullptr) {
        if (nameValue.name !=
            nullptr) {        // Values may be nullptr for use case where placeholder substitution is used
            pairCount++;
            if (pairCount > nameValueList->listSize) {
                nameValueList->nameValue = (NAMEVALUE*)realloc((void*)nameValueList->nameValue,
                                                               (nameValueList->listSize + 10) * sizeof(NAMEVALUE));
                nameValueList->listSize = nameValueList->listSize + 10;
            }
            nameValueList->pairCount = pairCount;
            nameValueList->nameValue[pairCount - 1] = nameValue;
        }
    } while (p2 != nullptr);

    // housekeeping

    free(buffer);
    free(copy);

    for (int i = 0; i < nameValueList->pairCount; i++) {
        if (STR_IEQUALS(nameValueList->nameValue[i].name, "delimiter")) {        // replace with correct delimiter value
            p = strchr(nameValueList->nameValue[i].value, '#');
            *p = delimiter;
            p = strrchr(nameValueList->nameValue[i].pair, '#');
            *p = delimiter;
            break;
        }
    }

    // Replace substituted delimiters in lists

    if (isListDelim) {
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

    return pairCount;
}
