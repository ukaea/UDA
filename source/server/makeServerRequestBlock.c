//------------------------------------------------------------------------------------------------------------------
/*!
Interprets the API arguments and assembles a Request data structure.

returns An integer Error Code: If non zero, a problem occured.
*/
//------------------------------------------------------------------------------------------------------------------

#include "makeServerRequestBlock.h"

#include <unistd.h>
#include <stdlib.h>
#include <errno.h>
#include <strings.h>

#include <clientserver/udaErrors.h>
#include <clientserver/stringUtils.h>
#include <clientserver/initStructs.h>
#include <clientserver/protocol.h>
#include <server/udaServer.h>

#include "serverPlugin.h"
#include "getServerEnvironment.h"

#if defined(SERVERBUILD) || defined(FATCLIENT)

void extractFunctionName(char* str, REQUEST_BLOCK* request_block)
{
    int i, lstr;
    char* p;
    if (str[0] == '\0') return;
    char* work = (char*) malloc((strlen(str) + 1) * sizeof(char));
    strcpy(work, str);
    if ((p = strchr(work, '(')) == NULL) return;
    p[0] = '\0';
    p = strstr(work, request_block->api_delim);
    if (p != NULL) {
        do {
            lstr = (int) (p - work) + (int) strlen(request_block->api_delim);
            for (i = 0; i < lstr; i++) work[i] = ' ';
            TrimString(work);
            LeftTrimString(work);
        } while ((p = strstr(work, request_block->api_delim)) != NULL);
    }
    strcpy(request_block->function, work);
    free((void*) work);
}

void initServerRequestBlock(REQUEST_BLOCK* str)
{
    str->request = 0;
    str->exp_number = 0;
    str->pass = -1;
    str->tpass[0] = '\0';
    str->path[0] = '\0';
    str->file[0] = '\0';
    str->format[0] = '\0';
    str->archive[0] = '\0';
    str->device_name[0] = '\0';
    str->server[0] = '\0';
    str->function[0] = '\0';
    str->api_delim[0] = '\0';
    str->signal[0] = '\0';
    str->source[0] = '\0';
    str->subset[0] = '\0';
    str->datasubset.subsetCount = 0;
    initNameValueList(&str->nameValueList);
}

int makeServerRequestBlock(REQUEST_BLOCK* request_block, PLUGINLIST pluginList)
{

    int i, rc, ldelim, lstr = 0, err = 0, isFile = 0, isProxy = 0, isFunction = 0, isServer = 0,
            reduceSignal, noSource, isForeign;
    char* test = NULL, * token = NULL, * p, * p0, * p1, * p2;
    char work[MAXMETA];
    char work2[MAXMETA];
    unsigned short strip = 1;        // Remove enclosing quotes from name value pairs

    UDA_LOG(UDA_LOG_DEBUG, "Source Argument\n");

    ENVIRONMENT* environment = getIdamServerEnvironment();

    //------------------------------------------------------------------------------
    // Always use the client's delimiting string if provided, otherwise use the default delimiter

    if ((ldelim = (int) strlen(request_block->api_delim)) == 0) {
        strcpy(request_block->api_delim, environment->api_delim);
        ldelim = (int) strlen(request_block->api_delim);
    }

    //------------------------------------------------------------------------------
    // Start with ignorance about which plugin to use

    request_block->request = REQUEST_READ_UNKNOWN;

    //------------------------------------------------------------------------------
    // Check there is something to work with!

    sprintf(work, "%s%s", environment->api_archive, environment->api_delim);    // default archive
    sprintf(work2, "%s%s", environment->api_device, environment->api_delim);    // default device

    noSource = (request_block->source[0] == '\0' ||                // no source
                STR_IEQUALS(request_block->source, environment->api_device) ||    // default device name
                STR_IEQUALS(request_block->source, work2));            // default device name + delimiting string

    if ((request_block->signal[0] == '\0' || STR_IEQUALS(request_block->signal, work)) && noSource) {
        err = 999;
        addIdamError(CODEERRORTYPE, "makeServerRequestBlock", err,
                     "Neither Data Object nor Source specified!");
        return err;
    }

    //------------------------------------------------------------------------------
    // Strip default device from the source if present and leading

    lstr = (int) strlen(work2);
    if (!noSource && !strncasecmp(request_block->source, work2, lstr)) {
        for (i = 0; i < lstr; i++)request_block->source[i] = ' ';
        LeftTrimString(request_block->source);
    }

    //------------------------------------------------------------------------------
    // Is this server acting as an IDAM Proxy? If all access requests are being re-directed then do nothing to the arguments.
    // They are just passed onwards without interpretation.

    isProxy = environment->server_proxy[0] != '\0';

    if (isProxy) {
        request_block->request = REQUEST_READ_IDAM;
        //return 0;
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

    isFunction = 0;
    isFile = 0;
    isServer = 0;
    isForeign = 0;

    test = strstr(request_block->source, request_block->api_delim);    // Delimiter present?
    if (test != NULL) {
        strncpy(work2, request_block->source, test - request_block->source);
        work2[test - request_block->source] = '\0';
        TrimString(work2);
        strcpy(work, test + ldelim);
    } else {
        work2[0] = '\0';
        strcpy(work, request_block->source);
    }

// Test for DEVICE::LIBRARY::function(argument)	 - More delimiter characters present?

    if (test != NULL && STR_IEQUALS(work2, environment->api_device) &&
        (p = strstr(work, request_block->api_delim)) != NULL) {
        lstr = (int) (p - work);
        strncpy(work2, work, lstr);        // Ignore the default device name - force a pass to Scenario 2
        work2[lstr] = '\0';
        TrimString(work2);
        lstr = lstr + ldelim;
        for (i = 0; i < lstr; i++) work[i] = ' ';
        LeftTrimString(work);
    }

    do {

        if (noSource) {                            // No Source
            strcpy(request_block->device_name, environment->api_device);    // Default Device Name
            break;
        }

        if (test == NULL || STR_IEQUALS(work2, environment->api_device)) {    // No delimiter present or default device?

            UDA_LOG(UDA_LOG_DEBUG, "No device name or format or protocol or library is present\n");

            strcpy(request_block->device_name, environment->api_device);            // Default Device Name

// Regular request: pulse or pulse/pass ==> Generic request

            if (genericRequestTest(work, request_block, pluginList)) break;

// Not a Server Side Function? 		Note: /a/b/fun(aaa) is a (bad!)file path and fun(a/b/c) is a function

            p0 = strchr(work, '/');        // Path separator mixed with parenthesis?
            p1 = strrchr(work, '/');
            p = strchr(work, '(');
            p2 = strchr(work, ')');

            if (p == NULL || p2 == NULL || (p != NULL && p2 == NULL) || (p == NULL && p2 != NULL) ||
                (p0 != NULL && p != NULL && p0 < p) || (p1 != NULL && p2 != NULL && p1 > p2)) {

                if ((p0 != NULL || p1 != NULL) && (p != NULL || p2 != NULL)) {
                    err = 999;
                    addIdamError(CODEERRORTYPE, "makeServerRequestBlock", err,
                                 "Source syntax: path with parenthesis () is incorrect!");
                    return err;
                }

// Request must be a private file format. It cannot be a local/remote server protocol.

                UDA_LOG(UDA_LOG_DEBUG, "No File Format has been specified. Selecting ....\n");

                rc = sourceFileFormatTest(request_block->source, request_block, pluginList);

#ifdef JETSERVER
                if(rc < 0) {
                    strcpy(request_block->format, "PPF");		// Assume the Default Format (PPF?)
                    for(i=0; i<pluginList.count; i++) {
                        if(STR_IEQUALS(request_block->format, pluginList.plugin[i].format)) {
                            request_block->request = pluginList.plugin[i].request;
                            break;
                        }
                    }
                    test = request_block->source;			// No prefix nor delimiter
                    ldelim = 0;					// No offset required
                    rc = 1;
                }
#endif

                if (rc <= 0) {
                    UDA_LOG(UDA_LOG_DEBUG, "File Format NOT identified from name extension!\n");
                    //if(rc < 0) return -rc;
                    err = 999;
                    addIdamError(CODEERRORTYPE, "makeServerRequestBlock", err,
                                 "No File Format identifed: Please specify.");
                    return err;
                }

                expandEnvironmentVariables(request_block);            // Resolve any Serverside environment variables

                UDA_LOG(UDA_LOG_DEBUG, "File Format identified from name extension!\n");
                break;

            } else {

// Internal Server Side Function ?	A file path may contain characters ( and ) !

                if ((p = strchr(work, '(')) != NULL && strchr(p, ')') != NULL) {
                    strcpy(work2, &p[1]);
                    p = strchr(work2, ')');
                    p[0] = '\0';
                    LeftTrimString(work2);
                    TrimString(work2);
                    isFunction = 1;

                    request_block->request = REQUEST_READ_SERVERSIDE;
                    extractFunctionName(work, request_block);

                    UDA_LOG(UDA_LOG_DEBUG, "**** Server Side Function ??? ****\n");

// Extract Name Value pairs

                    if ((rc = nameValuePairs(work2, &request_block->nameValueList, strip)) == -1) {
                        err = 999;
                        addIdamError(CODEERRORTYPE, "makeServerRequestBlock", err,
                                     "Name Value pair syntax is incorrect!");
                        return err;
                    }

// Test for external library functions using the Archive name as the library name identifier

                    reduceSignal = 0;
                    extractArchive(request_block, reduceSignal);

                    for (i = 0; i < pluginList.count; i++) {
                        if (STR_IEQUALS(request_block->archive, pluginList.plugin[i].format)) {
                            request_block->request = pluginList.plugin[i].request;                // Found!
                            strcpy(request_block->format, pluginList.plugin[i].format);
                            isFunction = pluginList.plugin[i].plugin_class == PLUGINFUNCTION;
                            break;
                        }
                    }
                    break;

                } else {

                    err = 999;
                    addIdamError(CODEERRORTYPE, "makeServerRequestBlock", err,
                                 "No Data Access Plugin Identified!");
                    return err;
                }
            }

        } else {

//---------------------------------------------------------------------------------------------------------------------
// Scenario #2: A foreign device name or format or protocol or library is present

            UDA_LOG(UDA_LOG_DEBUG, "A device name or format or protocol or library is present.\n");

// Test for known File formats, Server protocols or Libraries or Devices

            for (i = 0; i < pluginList.count; i++) {
                if (STR_IEQUALS(work2, pluginList.plugin[i].format)) {
                    if (pluginList.plugin[i].plugin_class != PLUGINDEVICE) {
                        request_block->request = pluginList.plugin[i].request;        // Found
                        strcpy(request_block->format, pluginList.plugin[i].format);
                        if (pluginList.plugin[i].plugin_class !=
                            PLUGINFILE) {            // The full file path fully resolved by the client
                            strcpy(request_block->path,
                                   test + ldelim);            // Complete String following :: delimiter
                            strcpy(request_block->file, "");                // Clean the filename
                            if (pluginList.plugin[i].plugin_class == PLUGINFUNCTION) {
                                isFunction = 1;
                                extractFunctionName(work, request_block);
                            }
                        } else {
                            strcpy(request_block->file, (char*) basename(test + ldelim));    // Final token
                        }
                        isFile = pluginList.plugin[i].plugin_class == PLUGINFILE;
                        isServer = pluginList.plugin[i].plugin_class == PLUGINSERVER;
                        break;
                    } else {

// Registered Devices and their access Server Protocols
// Identify the server protocol - it must be a server protocol!
// Substitute the Device name with the protocol and server details

                        int depth = 0;
                        //int id = findPluginRequestByFormat(pluginList.plugin[i].deviceProtocol, &pluginList);
                        int id = findPluginIdByFormat(pluginList.plugin[i].deviceProtocol, &pluginList);
                        if (id >= 0 && pluginList.plugin[id].plugin_class == PLUGINSERVER) {

                            sprintf(work, "%s%s%s", pluginList.plugin[i].deviceProtocol, request_block->api_delim,
                                    pluginList.plugin[i].deviceHost);

                            if (pluginList.plugin[i].devicePort[0] != '\0') {
                                strcat(work, ":");
                                strcat(work, pluginList.plugin[i].devicePort);
                            }

                            if ((test + ldelim)[0] != '/') {
                                if ((test + ldelim)[0] != '\0') {
                                    strcat(work, "/");
                                    strcat(work, test + ldelim);
                                }
                            } else
                                strcat(work, test + ldelim);

                            strcpy(request_block->source, work);
                            if (depth++ > MAXREQDEPTH) {
                                err = 999;
                                addIdamError(CODEERRORTYPE, "makeServerRequestBlock", err,
                                             "Too many chained Device Name to Server Protocol Host subtitutions!");
                            }
                            err = makeServerRequestBlock(request_block, pluginList);
                            depth--;
                            return err;
                        }
                    }
                }
            }

// If no match was found then the prefix must be a foreign Device Name not entered in the server configuration file.
// The request must be a generic lookup of how to access data remotely for the specified device.
// The external server providing access to the foreign device's data will interpret the arguments

            if (request_block->request == REQUEST_READ_UNKNOWN) {
                UDA_LOG(UDA_LOG_DEBUG, "No plugin was identified for the format: %s\n", work2);
                isForeign = 1;
                strcpy(request_block->device_name, work2);                // Copy the DEVICE prefix
                request_block->request = REQUEST_READ_GENERIC;            // The database will identify the target

                /*
                #ifdef JETSERVER

                // Legacy Exceptions: JET Device and PPF or JPF archives

                	    if(STR_IEQUALS(request_block->device_name, "JET")){
                	       reduceSignal = 0;
                               extractArchive(request_block, reduceSignal);
                               if(request_block->archive[0] != '\0'){
                	          for(i=0;i<pluginList.count;i++){
                                     if(STR_IEQUALS(request_block->archive, pluginList.plugin[i].format)){
                	                if(pluginList.plugin[i].request == REQUEST_READ_PPF || pluginList.plugin[i].request == REQUEST_READ_JPF){
                			   isForeign = 0;
                			   isServer  = 1;
                			   genericRequestTest(work, request_block, pluginList);
                			   request_block->request = pluginList.plugin[i].request;	// Legacy exception found
                	                   strcpy(request_block->format, pluginList.plugin[i].format);
                	                   //strcpy(request_block->path, test+ldelim);			// Complete String following :: delimiter
                	                   strcpy(request_block->path, "");				// Clean the path
                	                   strcpy(request_block->file, "");				// Clean the filename
                	                   break;
                	                }
                                     }
                	          }
                	       }
                	       break;
                	    }
                #endif	// JETSERVER
                */
                break;
            }

// A match was found: The Source must be a format or a protocol or a library

            strcpy(request_block->device_name, environment->api_device);            // Default Device Name

            if (isFile) {                                    // Resolve any Serverside environment variables
                UDA_LOG(UDA_LOG_DEBUG, "File Format has been specified.\n");
                expandEnvironmentVariables(request_block);
                break;
            }

            if (!isFile && !isFunction) {        // Server Protocol
                UDA_LOG(UDA_LOG_DEBUG, "Server Protocol\n");
                break;
            }

// Must be a function
// Test syntax

            p0 = strchr(work, '/');        // Path separator mixed with parenthesis?
            p1 = strrchr(work, '/');        // Path separator mixed with parenthesis?
            p = strchr(work, '(');
            p2 = strchr(work, ')');

            if (p == NULL || p2 == NULL || (p != NULL && p2 == NULL) || (p == NULL && p2 != NULL) ||
                (p0 != NULL && p != NULL && p0 < p) || (p1 != NULL && p2 != NULL && p1 > p2)) {
                err = 999;
                addIdamError(CODEERRORTYPE, "makeServerRequestBlock", err,
                             "Not a function when one is expected! - A Library plugin has been specified.");
                return err;
            }

// ToDo: Extract Data subset operations specified within the source argument

// Extract Name Value pairs

            if ((p = strchr(work, '(')) != NULL && strchr(p, ')') != NULL) {
                strcpy(work, &p[1]);
                p = strchr(work, ')');
                p[0] = '\0';
                LeftTrimString(work);
                TrimString(work);

// Extract Name Value pairs

                if ((rc = nameValuePairs(work, &request_block->nameValueList, strip)) == -1) {
                    err = 999;
                    addIdamError(CODEERRORTYPE, "makeServerRequestBlock", err,
                                 "Name Value pair syntax is incorrect!");
                    return err;
                }

// ToDo: Extract Data subset operations specified as a named value pair, tagged 'subset'

            } else {
                err = 999;
                addIdamError(CODEERRORTYPE, "makeServerRequestBlock", err,
                             "Function syntax error - please correct");
                return err;
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

    if ((rc = extractSubset(request_block)) == -1) {
        err = 999;
        addIdamError(CODEERRORTYPE, "makeServerRequestBlock", err, "Subset operation is incorrect!");
        return err;
    }

// as at 19Apr2011 no signals recorded in the IDAM database use either [ or { characters
// there will be no confusion between subset operations and valid signal names

    if (rc == 1) {        // the subset has valid syntax so reduce the signal name by removing the subset instructions
        p = strstr(request_block->signal, request_block->subset);
        if (p != NULL) p[0] = '\0';            // Remove subset operations from variable name
        TrimString(request_block->signal);
    } else {
        request_block->subset[0] = '\0';
        request_block->datasubset.subsetCount = 0;
    }

//------------------------------------------------------------------------------
// Extract the Archive Name and detach from the signal  (detachment is necessary when not passing on to another IDAM server)

    if (request_block->request == REQUEST_READ_IDAM) {
        reduceSignal = 0;
        err = extractArchive(request_block, reduceSignal);
    } else {
        reduceSignal = !isForeign;                // Don't detach if a foreign device
        err = extractArchive(request_block, reduceSignal);
    }
    if (request_block->archive[0] == '\0') strcpy(request_block->archive, environment->api_archive);

//------------------------------------------------------------------------------
// Extract Name Value Pairs from the data object (signal) without modifying

// as at 22Sep2011 261 signals recorded in the IDAM database used parenthesis characters so could be confused
// with function requests. However, for all these cases a source term would be specified. No library
// would be part of this specification so there would be no ambiguity.

    isFunction = 0;

    if (!isServer && (p = strchr(request_block->signal, '(')) != NULL && strchr(p, ')') != NULL &&
        strcasecmp(request_block->archive, environment->api_archive) != 0) {
        strcpy(work, &p[1]);
        if ((p = strrchr(work, ')')) != NULL) {
            p[0] = '\0';
            LeftTrimString(work);
            TrimString(work);
            isFunction = 1;
            if ((rc = nameValuePairs(work, &request_block->nameValueList, strip)) == -1) {
                err = 999;
                addIdamError(CODEERRORTYPE, "makeServerRequestBlock", err,
                             "Name Value pair syntax is incorrect!");
                return err;
            }
            extractFunctionName(request_block->signal, request_block);
        }
    }

//------------------------------------------------------------------------------
// If No Source was Specified: All requirements are contained in the signal string

    if (noSource) {
        UDA_LOG(UDA_LOG_DEBUG, "Signal Argument - No Source\n");
        UDA_LOG(UDA_LOG_DEBUG, "request: %d\n", request_block->request);

// If the signal could be a function call, check the archive name against the function library plugins

        if (isFunction && err == 0) {        // Test for known Function Libraries
            isFunction = 0;
            for (i = 0; i < pluginList.count; i++) {
                if (STR_IEQUALS(request_block->archive, pluginList.plugin[i].format)) {
                    request_block->request = pluginList.plugin[i].request;            // Found
                    strcpy(request_block->format, pluginList.plugin[i].format);
                    isFunction = (pluginList.plugin[i].plugin_class == PLUGINFUNCTION);        // Must be a known Library
                    break;
                }
            }

            UDA_LOG(UDA_LOG_DEBUG, "A request: %d\n", request_block->request);
            UDA_LOG(UDA_LOG_DEBUG, "isFunction: %d\n", isFunction);

            if (!isFunction) {                // Must be a default server-side function

                for (i = 0; i < pluginList.count; i++) {
                    if (STR_IEQUALS(pluginList.plugin[i].symbol, "SERVERSIDE") &&
                        pluginList.plugin[i].library[0] == '\0') {
                        request_block->request = REQUEST_READ_SERVERSIDE;            // Found
                        strcpy(request_block->format, pluginList.plugin[i].format);
                        isFunction = 1;
                        break;
                    }
                }
                if (!isFunction) request_block->function[0] = '\0';
            }

            UDA_LOG(UDA_LOG_DEBUG, "B request: %d\n", request_block->request);

        } else {

// Select the Generic plugin: No source => no format or protocol or library was specified.

            request_block->request = REQUEST_READ_GENERIC;

            UDA_LOG(UDA_LOG_DEBUG, "C request: %d\n", request_block->request);

        }

        // return err;

    } else {

// Does the data object (Signal) have the form: LIBRARY::function?
// Exception is Serverside function

        if (isFunction && strcasecmp(request_block->archive, environment->api_archive) != 0) {
            int id = findPluginIdByFormat(request_block->archive, &pluginList);
            if (id >= 0 && pluginList.plugin[id].plugin_class == PLUGINFUNCTION &&
                strcasecmp(pluginList.plugin[id].symbol, "serverside") != 0) {
                if (request_block->request == REQUEST_READ_GENERIC ||
                    request_block->request == REQUEST_READ_UNKNOWN) {
                    request_block->request = pluginList.plugin[id].request;    // Found
                    strcpy(request_block->format, pluginList.plugin[id].format);
                    UDA_LOG(UDA_LOG_DEBUG, "D request: %d\n", request_block->request);

                } else {
                    if (request_block->request != pluginList.plugin[i].request) {    // Inconsistent
// Let Source have priority over the Signal?
                        UDA_LOG(UDA_LOG_DEBUG, "Inconsistent Plugin Libraries: Source selected over Signal\n");
                    }
                }
            }
        }
    }
    UDA_LOG(UDA_LOG_DEBUG, "E request: %d\n", request_block->request);

//---------------------------------------------------------------------------------------------------------------------
// Legacy Specials ...

#ifdef JETSERVER
    if(request_block->request == REQUEST_READ_PPF) {	// format: PPF::/dda/shot/seq/owner or PPF::/dda/shot/owner
        strcpy(request_block->path, "");			// Clean the path
        strcpy(request_block->file, "");			// Clean the filename
        strcpy(work, test+ldelim);
        if((token = strtok(work, "/")) != NULL) {		// Tokenise the remaining string
            strcpy(request_block->file, token);
            if((token = strtok(NULL, "/")) != NULL) {
                if(IsNumber(token)) request_block->exp_number = atoi(token);
            }
            if((token = strtok(NULL, "/")) != NULL) {
                if(IsNumber(token)) {
                    request_block->pass = atoi(token);
                    if((token = strtok(NULL, "/")) != NULL) strcpy(request_block->path, token);
                } else {
                    strcpy(request_block->path, token);
                }
            }
        }
    }

    if(request_block->request == REQUEST_READ_JPF) {
        strcpy(request_block->path, "");				// Clean the path
        strcpy(request_block->file, "");				// Clean the filename
        strcpy(work, test+ldelim);
        //if((token = strchr(work, '/')) != NULL){		// Tokenise the remaining string
        if(work[0] == '/') {		// Tokenise the remaining string
            if(IsNumber(&work[1])) request_block->exp_number = atoi(&work[1]);
        } else if(IsNumber(work)) {
            request_block->exp_number = atoi(work);
        } else if((token = strrchr(work, '/')) != NULL && IsNumber(token+1)) {
            request_block->exp_number = atoi(token+1);
        } else if((token = strstr(work, environment->api_delim)) != NULL && IsNumber(token+ldelim)) {
            request_block->exp_number = atoi(token+ldelim);
        }
    }
#endif

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

    if (request_block->request == REQUEST_READ_MDS && !isProxy) {

        reverseString(test + ldelim, work);            // Drop the delimiters and Reverse the Source String
        if ((token = strtok(work, "/")) != NULL) {        // Tokenise

            if (IsNumber(token)) {                // This should be the tree Number otherwise only the server is passed
                reverseString(token, work2);            // Un-Reverse the token
                request_block->exp_number = atoi(work2);    // Extract the Data Tree Number
                if ((token = strtok(NULL, "/")) != NULL) {
                    reverseString(token, request_block->file);    // This should be the Tree Name
                    work2[0] = '\0';
                    while ((token = strtok(NULL, "/")) !=
                           NULL) {    // Everything Else is the Server Host and URL Path to the Tree
                        strcat(work2, token);
                        strcat(work2, "/");
                    }
                    if (work2[0] == '/') strcpy(work2, &work2[1]);    // Drop Trailing /
                    reverseString(work2, request_block->server);
                    token = test + ldelim;                // Preserve Leading /
                    if (token[0] != '/' && request_block->server[0] == '/') {
                        request_block->server[0] = ' ';
                        LeftTrimString(request_block->server);
                    }
                } else {
                    err = 3;
                }
            } else {
                strcpy(request_block->server, test + ldelim);    // Server or null string (default server)
            }

        } else {
            strcpy(request_block->server, "");            // Default Server
        }

        if (err != 0) {
            err = NO_SERVER_SPECIFIED;
            addIdamError(CODEERRORTYPE, "makeServerRequestBlock", err,
                         "The MDSPlus Data Source does not comply with the naming models: server/tree/number or server/path/to/data/tree/number");
            return err;
        }
    }

//---------------------------------------------------------------------------------------------------------------------
// IDAM and WEB Servers ...      parse source modelled as: server/source

    if (request_block->request == REQUEST_READ_IDAM || request_block->request == REQUEST_READ_WEB) {
        strcpy(work, test + ldelim);                // Drop the delimiters
        if ((token = strstr(work, "/")) != NULL) {        // Isolate the Server from the source IDAM::server:port/source
            token[0] = '\0';                // Break the String (work)
            strcpy(request_block->server, work);        // Extract the Server Name and Port
            strcpy(request_block->file, token + 1);        // Extract the Source URL Argument
        } else {
            err = NO_SERVER_SPECIFIED;
            addIdamError(CODEERRORTYPE, "makeServerRequestBlock", err,
                         "The Remote Server Data Source specified does not comply with the naming model: serverHost:port/sourceURL");
            return err;
        }
    }

//---------------------------------------------------------------------------------------------------------------------
// SQL Servers ...

    if (request_block->request == REQUEST_READ_SQL) {
        strcpy(request_block->server, request_block->path);
        if ((test = strchr(request_block->server, '/')) != NULL) {
            test[0] = '\0';
            strcpy(request_block->path, &test[1]);
        } else {
            request_block->path[0] = '\0';
        }
    }


    return 0;
}

int sourceFileFormatTest(const char* source, REQUEST_BLOCK* request_block, PLUGINLIST pluginList)
{

// returns 1 (TRUE) if a format was identified, 0 (FALSE) otherwise.
// return negative error code if a problem occured.

    int i, rc = 0;
    char* test;
    char* nc = " nc";
    char* hf = " hf";
    char* ida = " 99";
    char* blank = "   ";

// .99		IDA3 file
// .nc		netCDF4
// .cdf		netCDF3
// .hf		HDF
// .jpg		Binary
// .csv		ASCII

// Note: On JET the default data source is PPF. This is really a server protocol and not a file format.

//------------------------------------------------------------------------------
// Start with ignorance about which plugin to use

    request_block->format[0] = '\0';
    request_block->file[0] = '\0';
    request_block->request = REQUEST_READ_UNKNOWN;

    if (source[0] == '\0') return rc;

// Does the path contain any Illegal (or problem) characters

    if (!IsLegalFilePath((char*) source)) return rc;        // Not compliant with Portable Filename character set

// Does the source have a file extension? If so choose the format using the extension, otherwise investigate the file.

    if ((test = strrchr(source, '.')) == NULL) {

// No extension => test the first line of file, e.g. head -c10 <file>, but both netcdf and hdf5 use the same label HDF!

#ifndef _WIN32
        FILE* ph = NULL;
        int lstr = STRING_LENGTH;
        char* cmd = (char*) malloc(lstr * sizeof(char));
        sprintf(cmd, "head -c10 %s 2>/dev/null", source);
        errno = 0;
        if ((ph = popen(cmd, "r")) == NULL) {
            if (errno != 0) addIdamError(SYSTEMERRORTYPE, "sourceFileFormatTest", errno, "");
            addIdamError(CODEERRORTYPE, "sourceFileFormatTest", 999,
                         "Unable to Identify the File's Format");
            free((void*) cmd);
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
                if (env != NULL) {
                    sprintf(cmd, "%s -h %s 2>/dev/null | head -c10 2>/dev/null", env, source);
                    errno = 0;
                    if ((ph = popen(cmd, "r")) == NULL) {
                        if (errno != 0)
                            addIdamError(SYSTEMERRORTYPE, "sourceFileFormatTest", errno, "");
                        addIdamError(CODEERRORTYPE, "sourceFileFormatTest", 999,
                                     "Unable to Identify the File's Format");
                        free((void*) cmd);
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
                if (env != NULL) {
                    sprintf(cmd, "%s -h %s 2>/dev/null 2>/dev/null", env, source);
                    errno = 0;
                    if ((ph = popen(cmd, "r")) == NULL) {
                        if (errno != 0)
                            addIdamError(SYSTEMERRORTYPE, "sourceFileFormatTest", errno, "");
                        addIdamError(CODEERRORTYPE, "sourceFileFormatTest", 999,
                                     "Unable to Identify the File's Format");
                        free((void*) cmd);
                        return -999;
                    }

                    cmd[0] = '\0';
                    if (!feof(ph))
                        fgets(cmd, lstr - 1, ph);        // IDA3 interface version V3.13 with file structure IDA3.1
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
        free((void*) cmd);

#else
        return rc;
#endif
    }

// Select the format

    do {

// Test against Registered File Extensions
// TO DO: make extensions a list of valid extensions to minimise plugin duplication

        int breakAgain = 0;
        for (i = 0; i < pluginList.count; i++) {
            if (STR_IEQUALS(&test[1], pluginList.plugin[i].extension)) {
                strcpy(request_block->format, pluginList.plugin[i].format);
                breakAgain = 1;
                break;
            }
        }
        if (breakAgain)break;

// Other regular types

        if (strlen(&test[1]) == 2 && IsNumber(&test[1])) {        // an integer number?
            strcpy(request_block->format, "IDA3");
            break;
        }
        if (STR_IEQUALS(&test[1], "nc")) {
            strcpy(request_block->format, "netcdf");
            break;
        }
        if (STR_IEQUALS(&test[1], "cdf")) {
            strcpy(request_block->format, "netcdf");
            break;
        }
        if (STR_IEQUALS(&test[1], "hf")) {
            strcpy(request_block->format, "hdf5");
            break;
        }
        if (STR_IEQUALS(&test[1], "h5")) {
            strcpy(request_block->format, "hdf5");
            break;
        }
        if (STR_IEQUALS(&test[1], "hdf5")) {
            strcpy(request_block->format, "hdf5");
            break;
        }
        if (STR_IEQUALS(&test[1], "xml")) {
            strcpy(request_block->format, "xml");
            break;
        }
        if (STR_IEQUALS(&test[1], "csv")) {
            strcpy(request_block->format, "csv");
            break;
        }

        if (source[0] == '/' && source[1] != '\0' && isdigit(source[1])) {        // Default File Format?
            if (genericRequestTest(&source[1], request_block, pluginList)) {        // Matches 99999/999
                request_block->request = REQUEST_READ_UNKNOWN;
                strcpy(request_block->format, getIdamServerEnvironment()->api_format);        // the default Server File Format
                break;
            }
        }

        return -1;        // No format identified
        /*
              rc = 999;
              addIdamError(CODEERRORTYPE, "sourceFileFormatTest", rc,
                 "Unable to Identify the File's Format: Please specifiy the file's format");
              return -999;
        */

    } while (0);

// Test for known registered plugins for the File's format

    for (i = 0; i < pluginList.count; i++) {
        if (STR_IEQUALS(request_block->format, pluginList.plugin[i].format)) {
            rc = 1;
            UDA_LOG(UDA_LOG_DEBUG, "Format identified, selecting specific plugin for %s\n", request_block->format);
            request_block->request = pluginList.plugin[i].request;            // Found
            if (pluginList.plugin[i].plugin_class !=
                PLUGINFILE) {                // The full file path fully resolved by the client
                strcpy(request_block->file, "");                        // Clean the filename
            } else {
                strcpy(request_block->file, (char*) basename(request_block->source));    // Final token
            }
            break;
        }
    }

    return rc;
}

int genericRequestTest(const char* source, REQUEST_BLOCK* request_block, PLUGINLIST pluginList)
{

// Return 1 (TRUE) if the Generic plugin was selected, 0 (FALSE) otherwise

    int rc = 0;
    char* token = NULL;
    char work[STRING_LENGTH];

//------------------------------------------------------------------------------
// Start with ignorance about which plugin to use

    request_block->format[0] = '\0';
    request_block->file[0] = '\0';
    request_block->request = REQUEST_READ_UNKNOWN;

    if (source[0] == '\0') return rc;
    if (source[0] == '/') return rc;        // Directory based data

//------------------------------------------------------------------------------
// Check if the source has one of these forms:

// pulse		plasma shot number - an integer
// pulse/pass		include a pass or sequence number - this may be a text based component, e.g. LATEST

    if (IsNumber((char*) source)) {                    // Is the source an integer number?
        rc = 1;
        request_block->request = REQUEST_READ_GENERIC;
        strcpy(request_block->path, "");                    // Clean the path
        request_block->exp_number = atoi(source);                // Plasma Shot Number
        UDA_LOG(UDA_LOG_DEBUG, "exp number identified, selecting GENERIC plugin.\n");
    } else {
        strcpy(work, source);
        if ((token = strtok(work, "/")) != NULL) {                // Tokenise the remaining string
            if (IsNumber(token)) {                        // Is the First token an integer number?
                rc = 1;
                request_block->request = REQUEST_READ_GENERIC;
                strcpy(request_block->path, "");                // Clean the path
                request_block->exp_number = atoi(token);
                if ((token = strtok(NULL, "/")) != NULL) {            // Next Token
                    if (IsNumber(token)) {
                        request_block->pass = atoi(token);            // Must be the Pass number
                    } else {
                        strcpy(request_block->tpass, token);            // capture anything else
                    }
                }
                UDA_LOG(UDA_LOG_DEBUG, "exp number and pass id identified, selecting GENERIC plugin.\n");
            }
        }
    }

    return rc;
}


//------------------------------------------------------------------------------
// Strip out the Archive name from the data_object name
// syntax:	ARCHIVE::Data_OBJECT or DATA_OBJECT
// conflict:	ARCHIVE::DATA_OBJECT[::] or DATA_OBJECT[::] subsetting operations
//
// NOTE: Archive Name should not terminate with the character [ or { when a signal begins with the
//       character ] or }. These clash with subsetting syntax.
//
// Input Argument: reduceSignal - If TRUE (1) then extract the archive name and return the data object name
//                                without the prefixed archive name.

int extractArchive(REQUEST_BLOCK* request_block, int reduceSignal)
{

    int err = 0, test1, test2;
    int ldelim = (int) strlen(request_block->api_delim);
    char* test, * token, * work;

    TrimString(request_block->signal);

    if (request_block->signal[0] != '\0' && getIdamServerEnvironment()->server_proxy[0] == '\0') {

        UDA_LOG(UDA_LOG_DEBUG, "Testing for ARCHIVE::Signal\n");

        if ((test = strstr(request_block->signal, request_block->api_delim)) != NULL) {

            if (test - request_block->signal >= STRING_LENGTH - 1 || strlen(test + ldelim) >= MAXMETA - 1) {
                err = ARCHIVE_NAME_TOO_LONG;
                addIdamError(CODEERRORTYPE, "extractArchive", err, "The ARCHIVE Name is too long!");
                return err;
            }
            strncpy(request_block->archive, request_block->signal, test - request_block->signal);
            request_block->archive[test - request_block->signal] = '\0';
            TrimString(request_block->archive);

            if (!IsLegalFilePath(request_block->archive)) {
                request_block->archive[0] = '\0';
                return 0;
            }

// Test the proposed archive name for conflict with subsetting operation

            test1 = 0;
            test2 = 0;

            if ((token = strchr(request_block->archive, '[')) != NULL ||
                (token = strchr(request_block->archive, '{')) != NULL) {
                test1 = (strlen(&token[1]) == 0 || IsNumber(&token[1]));
            }

            if ((token = strchr(test + ldelim, ']')) != NULL ||
                (token = strchr(test + ldelim, '}')) != NULL) {
                work = (char*) malloc((strlen(test + ldelim) + 1) * sizeof(char));
                strcpy(work, test + ldelim);
                work[token - (test + ldelim)] = '\0';
                test2 = (strlen(work) == 0 || IsNumber(work));
                free(work);
            }

            if (!test1 && !test2) {
                if (reduceSignal) {
                    work = (char*) malloc((strlen(test + ldelim) + 1) * sizeof(char));
                    strcpy(work, test + ldelim);
                    strcpy(request_block->signal, work);    // Valid Archive & signal
                    free(work);
                    TrimString(request_block->signal);
                }
            } else {
                request_block->archive[0] = '\0';            // Reset Archive
            }

            UDA_LOG(UDA_LOG_DEBUG, "Archive %s\n", request_block->archive);
            UDA_LOG(UDA_LOG_DEBUG, "Signal  %s\n", request_block->signal);
        }
    }
    return err;
}

#endif


//------------------------------------------------------------------------------
// Does the Path contain with an Environment variable

void expandEnvironmentVariables(REQUEST_BLOCK* request_block)
{

    char* pcwd;
    int err, lcwd = STRING_LENGTH - 1;
    char work[STRING_LENGTH];
    char cwd[STRING_LENGTH];
    char ocwd[STRING_LENGTH];

    if (strchr(request_block->path, '$') == NULL) {
        UDA_LOG(UDA_LOG_DEBUG, "No embedded environment variables detected\n");
        return;
    }

    if ((pcwd = getcwd(ocwd, lcwd)) == NULL) {    // Current Working Directory
        UDA_LOG(UDA_LOG_DEBUG, "Unable to identify PWD!\n");
        return;
    }

    if ((err = chdir(request_block->path)) == 0) {            // Change to path directory
        pcwd = getcwd(cwd, lcwd);                    // The Current Working Directory is now the resolved directory name

        UDA_LOG(UDA_LOG_DEBUG, "Expanding embedded environment variable:\n");
        UDA_LOG(UDA_LOG_DEBUG, "from: %s\n", request_block->path);
        UDA_LOG(UDA_LOG_DEBUG, "to: %s\n", cwd);

        if (pcwd != NULL) strcpy(request_block->path, cwd);    // The expanded path
        chdir(ocwd);                        // Return to the Original WD
    } else {
        UDA_LOG(UDA_LOG_DEBUG, "expandEnvironmentvariables: Direct substitution! \n");

        char* fp = NULL, * env, * fp1;
        char work1[STRING_LENGTH];

        if (request_block->path[0] == '$' ||
            (fp = strchr(&request_block->path[1], '$')) != NULL) {    // Search for a $ character

            if (fp != NULL) {
                strncpy(work, request_block->path, fp - request_block->path);
                work[fp - request_block->path] = '\0';

                if ((fp1 = strchr(fp, '/')) != NULL) {
                    strncpy(work1, fp + 1, fp1 - fp - 1);
                    work1[fp1 - fp - 1] = '\0';
                } else strcpy(work1, fp + 1);

                if ((env = getenv(work1)) != NULL) {
                    if (env[0] == '/') {
                        strcpy(work1, env + 1);
                    } else {
                        strcat(work1, env);
                    }
                }

                strcat(work, work1);
                strcat(work, fp1);
                strcpy(request_block->path, work);
            }

            if (request_block->path[0] == '$') {
                work1[0] = '\0';
                if ((fp = strchr(request_block->path, '/')) != NULL) {
                    strncpy(work, request_block->path + 1, fp - request_block->path - 1);
                    work[fp - request_block->path - 1] = '\0';
                    strcpy(work1, fp);
                } else strcpy(work, request_block->path + 1);

                if ((env = getenv(work)) != NULL) {    // Check for Environment Variable
                    if (env[0] == '/') {
                        strcpy(work, env);
                    } else {
                        strcpy(work, "/");
                        strcat(work, env);
                    }
                }
                strcat(work, work1);
                strcpy(request_block->path, work);
            }
        }

        UDA_LOG(UDA_LOG_DEBUG, "Expanding to: %s\n", request_block->path);
    }

    return;
}

//----------------------------------------------------------------------
// Parse subset instructions- [start:end:stride] or {start:end:stride}
//
// Signal should avoid using subset like components in their name

int extractSubset(REQUEST_BLOCK* request_block)
{

// Return codes:
//
//	1 => Valid subset
//	0 => Not a subset operation - Not compliant with syntax
//     -1 => Error

    int i, j, err, rc = 1, lwork, subsetCount = 1;        // Number of subsetting operations
    char* p, * work, * token = NULL;

    request_block->subset[0] = '\0';
    request_block->datasubset.subsetCount = 0;

    if ((token = strchr(request_block->signal, '[')) == NULL &&
        (token = strchr(request_block->signal, '{')) == NULL)
        return 0;
    if ((work = strrchr(request_block->signal, ']')) == NULL &&
        (work = strrchr(request_block->signal, '}')) == NULL)
        return 0;
    if (work < token) return 0;

// Test the subset operation specified complies with the syntax: [start:end:stride]
// Parse for detailed instructions

    lwork = (int) strlen(token);
    work = (char*) malloc((lwork + 3) * sizeof(char));

    strcpy(work, token);
    copyString(token, request_block->subset, STRING_LENGTH - 1);

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

    while ((token = strstr(work, "][")) != NULL || (token = strstr(work, "}{")) != NULL) {    // Adopt a single syntax
        token[0] = ',';
        token[1] = ' ';
    }
    p = work;
    while ((token = strchr(p, ',')) != NULL) {        // Count the Dimensions
        p = &token[1];
        subsetCount++;
    }
    if (subsetCount > MAXRANK2) subsetCount = MAXRANK2;

// Array of subset instructions for each dimension

    char** work2 = (char**) malloc(subsetCount * sizeof(char*));
    for (i = 0; i < subsetCount; i++) {
        work2[i] = (char*) malloc(lwork * sizeof(char));
        work2[i][0] = '\0';
    }

// 3 subset details

    char** work3 = (char**) malloc(3 * sizeof(char*));
    for (i = 0; i < 3; i++) {
        work3[i] = (char*) malloc(lwork * sizeof(char));
        work3[i][0] = '\0';
    }
    char* work4 = (char*) malloc(lwork * sizeof(char));

    for (i = 0; i < subsetCount; i++) {
        request_block->datasubset.start[i] = 0;
        request_block->datasubset.stop[i] = 0;
        request_block->datasubset.count[i] = 0;
        request_block->datasubset.stride[i] = 0;
    }
    request_block->datasubset.subsetCount = subsetCount;

    subsetCount = 0;
    if ((token = strtok(work, ",")) != NULL) {    // Process each subset instruction separately (work2)
        strcpy(work2[subsetCount++], token);
        while (subsetCount < MAXRANK2 && (token = strtok(NULL, ",")) != NULL) strcpy(work2[subsetCount++], token);

        do {
            for (i = 0; i < subsetCount; i++) {

                request_block->datasubset.subset[i] = 0;

                TrimString(work2[i]);
                LeftTrimString(work2[i]);
                for (j = 0; j < 3; j++)work3[j][0] = '\0';

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
                    if ((p = strstr(work2[i], "::")) != NULL) {
                        p[0] = '\0';
                        strcpy(work4, work2[i]);
                        strcat(work4, ":*");
                        strcat(work4, &p[1]);
                        strcpy(work2[i], work4);
                    } else {
                        strcpy(work4, work2[i]);
                    }
                }

                if (strchr(work2[i], ':') != NULL && (token = strtok(work2[i], ":")) != NULL) {
                    j = 0;
                    strcpy(work3[j++], token);
                    while (j < 3 && (token = strtok(NULL, ":")) != NULL) strcpy(work3[j++], token);
                    for (j = 0; j < 3; j++)TrimString(work3[j]);
                    for (j = 0; j < 3; j++)LeftTrimString(work3[j]);

                    if (work3[0][0] != '\0' && IsNumber(work3[0])) {    // [a:] or [a:*] or [a:b] etc
                        p = NULL;
                        errno = 0;
                        request_block->datasubset.start[i] = (int) strtol(work3[0], &p, 10);
                        if (errno != 0 || *p != 0 || p == work3[0]) {
                            rc = 0;
                            break;
                        }
                        if (request_block->datasubset.start[i] < 0) {
                            err = 999;
                            addIdamError(CODEERRORTYPE, "extractSubset", err,
                                         "Invalid Start Index in subset operation");
                            rc = -1;
                            break;
                        }

                        request_block->datasubset.stop[i] = request_block->datasubset.start[i];
                        request_block->datasubset.count[i] = 1;
                        request_block->datasubset.stride[i] = 1;
                        request_block->datasubset.subset[i] = 1;

                    } else {
                        rc = 0;                    // Not an Error - not a subset operation
                        break;
                    }
                    if (work3[1][0] != '\0' && IsNumber(work3[1])) {    // [a:b]
                        p = NULL;
                        errno = 0;
                        request_block->datasubset.stop[i] = (int) strtol(work3[1], &p, 10);
                        if (errno != 0 || *p != 0 || p == work3[0]) {
                            rc = 0;
                            break;
                        }
                        if (request_block->datasubset.stop[i] < 0) {
                            err = 999;
                            addIdamError(CODEERRORTYPE, "extractSubset", err,
                                         "Invalid sample End Index in subset operation");
                            rc = -1;
                            break;
                        }
                        request_block->datasubset.count[i] =
                                request_block->datasubset.stop[i] - request_block->datasubset.start[i] + 1;
                        request_block->datasubset.subset[i] = 1;

                        if (request_block->datasubset.stop[i] < request_block->datasubset.start[i]) {
                            err = 999;
                            addIdamError(CODEERRORTYPE, "extractSubset", err,
                                         "Invalid Stop Index in subset operation");
                            rc = -1;
                            break;
                        }

                    } else {
                        if (strlen(work3[1]) == 0 || (strlen(work3[1]) == 1 && work3[1][0] == '*')) {    // [a:],[a:*]
                            request_block->datasubset.count[i] = -1;
                            request_block->datasubset.stop[i] = -1;            // To end of dimension
                        } else {
                            rc = 0;
                            break;
                        }
                    }
                    if (work3[2][0] != '\0') {
                        if (IsNumber(work3[2])) {
                            p = NULL;
                            errno = 0;
                            request_block->datasubset.stride[i] = (int) strtol(work3[2], &p, 10);
                            if (errno != 0 || *p != 0 || p == work3[0]) {
                                rc = 0;
                                break;
                            }
                            if (request_block->datasubset.stride[i] <= 0) {
                                err = 999;
                                addIdamError(CODEERRORTYPE, "extractSubset", err,
                                             "Invalid sample stride length in subset operation");
                                rc = -1;
                                break;
                            }
                            if (request_block->datasubset.stride[i] > 1)request_block->datasubset.subset[i] = 1;

                            if (request_block->datasubset.stride[i] > 1 && request_block->datasubset.count[i] > 1) {
                                if ((request_block->datasubset.count[i] % request_block->datasubset.stride[i]) > 0)
                                    request_block->datasubset.count[i] = 1 + request_block->datasubset.count[i] /
                                                                             request_block->datasubset.stride[i];
                                else
                                    request_block->datasubset.count[i] =
                                            request_block->datasubset.count[i] / request_block->datasubset.stride[i];
                            }

                        } else {
                            rc = 0;
                            break;
                        }
                    }
                } else {
                    if (work4[0] == '\0' || work4[0] == '*') {        // [], [*]
                        request_block->datasubset.start[i] = 0;
                        request_block->datasubset.stop[i] = -1;
                        request_block->datasubset.count[i] = -1;
                        request_block->datasubset.stride[i] = 1;
                    } else {
                        if (IsNumber(work4)) {                // [a]
                            p = NULL;
                            errno = 0;
                            request_block->datasubset.start[i] = (int) strtol(work4, &p, 10);
                            if (errno != 0 || *p != 0 || p == work3[0]) {
                                rc = 0;
                                break;
                            }
                            if (request_block->datasubset.start[i] < 0) {
                                err = 999;
                                addIdamError(CODEERRORTYPE, "extractSubset", err,
                                             "Invalid start index in subset operation");
                                rc = -1;
                                break;
                            }

                            request_block->datasubset.stop[i] = request_block->datasubset.start[i];
                            request_block->datasubset.count[i] = 1;
                            request_block->datasubset.stride[i] = 1;
                            request_block->datasubset.subset[i] = 1;

                        } else {
                            rc = 0;
                            break;
                        }
                    }
                }
            }
        } while (0);
    } else
        rc = 0;

    free(work);
    for (i = 0; i < subsetCount; i++)free(work2[i]);
    free(work2);
    for (i = 0; i < 3; i++)free(work3[i]);
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


/*
void initNameValueList(NAMEVALUELIST *nameValueList){
   nameValueList->pairCount = 0;
   nameValueList->listSize  = 0;
   nameValueList->nameValue = NULL;
}
*/
void freeNameValueList(NAMEVALUELIST* nameValueList)
{
    int i;
    if (nameValueList->nameValue != NULL) {
        for (i = 0; i < nameValueList->pairCount; i++) {
            if (nameValueList->nameValue[i].pair != NULL) free((void*) nameValueList->nameValue[i].pair);
            if (nameValueList->nameValue[i].name != NULL) free((void*) nameValueList->nameValue[i].name);
            if (nameValueList->nameValue[i].value != NULL) free((void*) nameValueList->nameValue[i].value);
        }
    }
    free((void*) nameValueList->nameValue);
    nameValueList->pairCount = 0;
    nameValueList->listSize = 0;
    nameValueList->nameValue = NULL;
}

void parseNameValue(char* pair, NAMEVALUE* nameValue, unsigned short strip)
{
    int lstr;
    char* p, * copy;
    lstr = (int) strlen(pair) + 1;
    copy = (char*) malloc(lstr * sizeof(char));
    strcpy(copy, pair);
    LeftTrimString(copy);
    TrimString(copy);
    nameValue->pair = (char*) malloc(lstr * sizeof(char));
    strcpy(nameValue->pair, copy);
    if ((p = strchr(copy, '=')) != NULL) {
        *p = '\0';
        lstr = (int) strlen(copy) + 1;
        nameValue->name = (char*) malloc(lstr * sizeof(char));
        strcpy(nameValue->name, copy);
        lstr = (int) strlen(&p[1]) + 1;
        nameValue->value = (char*) malloc(lstr * sizeof(char));
        strcpy(nameValue->value, &p[1]);
        LeftTrimString(nameValue->name);
        LeftTrimString(nameValue->value);
        TrimString(nameValue->name);
        TrimString(nameValue->value);
        if (strip) {            // remove encosing single or double quotes
            lstr = (int) strlen(nameValue->value);
            if ((nameValue->value[0] == '\'' && nameValue->value[lstr - 1] == '\'') ||
                (nameValue->value[0] == '"' && nameValue->value[lstr - 1] == '"')) {
                nameValue->value[0] = ' ';
                nameValue->value[lstr - 1] = ' ';
                LeftTrimString(nameValue->value);
                TrimString(nameValue->value);
            }
        }
    } else {
        if (copy[0] == '/') {        // Mimic IDL keyword passing
            lstr = (int) strlen(copy);
            nameValue->name = (char*) malloc(lstr * sizeof(char));
            strcpy(nameValue->name, &copy[1]);            // Ignore leader forward slash
            lstr = 5;
            nameValue->value = (char*) malloc(lstr * sizeof(char));
            strcpy(nameValue->value, "true");
            LeftTrimString(nameValue->name);
            TrimString(nameValue->name);
        } else {
            nameValue->name = NULL;
            nameValue->value = NULL;
        }
    }
    free((void*) copy);
}

int nameValuePairs(char* pairList, NAMEVALUELIST* nameValueList, unsigned short strip)
{

// Ignore delimter in anything enclosed in single or double quotes
// Recognise /name as name=TRUE

    int i, lstr, pairCount = 0;
    char proposal, delimiter = ',', substitute = 1;
    char* p, * p2, * p3 = NULL, * buffer, * copy;
    NAMEVALUE nameValue;
    lstr = (int) strlen(pairList);

    if (lstr == 0) return pairCount;                // Nothing to Parse
    if (strchr(pairList, '=') == NULL && pairList[0] != '/')
        return pairCount;        // Not a Name Value list or Keyword
    if (pairList[0] == '=') return -1;                // Syntax error
    if (pairList[lstr - 1] == '=') return -1;            // Syntax error

    lstr = lstr + 1;
    buffer = (char*) malloc(lstr * sizeof(char));
    copy = (char*) malloc(lstr * sizeof(char));

    strcpy(copy, pairList);        // working copy

// Locate the delimiter name value pair if present - use default character ',' if not

    if ((p = strcasestr(copy, "delimiter")) != NULL) {
        strcpy(buffer, &p[9]);
        LeftTrimString(buffer);
        if (buffer[0] == '=' && buffer[1] != '\0') {
            buffer[0] = ' ';
            LeftTrimString(buffer);        // remove whitespace
            if (strlen(buffer) >= 3 && (
                    (buffer[0] == '\'' && buffer[2] == '\'') || (buffer[0] == '"' && buffer[2] == '"'))) {
                proposal = buffer[1];        // proposal delimiter
                lstr = (int) (p - copy);
                if (lstr == 0) {            // delimiter name value pair coincident with start of list
                    delimiter = proposal;        // new delimiter
                    p3 = strchr(&p[9], delimiter);    // change delimiter to avert incorrect parse
                    *p3 = '#';
                } else {
                    strncpy(buffer, copy, lstr);    // check 'delimiter' is not part of another name value pair
                    buffer[lstr] = '\0';
                    TrimString(buffer);
                    lstr = (int) strlen(buffer);
                    if (buffer[lstr - 1] == proposal) {    // must be an immediately preceeding delimiter character
                        delimiter = proposal;        // new delimiter accepted
                        p3 = strchr(&p[9], delimiter);    // change delimiter temporarily to avert incorrect parse
                        *p3 = '#';
                    } else {
                        TrimString(buffer);        // Check for non alpha-numeric character
                        lstr = (int) strlen(buffer);
                        if (!isalpha(buffer[lstr - 1]) && !isdigit(buffer[lstr - 1])) {    // Probable syntax error!
                            free((void*) buffer);
                            free((void*) copy);
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
    for (i = 0; i < lstr; i++) {
        if (copy[i] == '\'' || copy[i] == '"') {
            if (isList)
                isList = 0;        // Switch substitution off
            else
                isList = 1;    // Switch substitution on
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
        if ((p2 = strchr(p, delimiter)) != NULL) {        // Test: subset=[<=3] or not pair just [<=3]
            strncpy(buffer, p, p2 - p);
            buffer[p2 - p] = '\0';
            p = p2 + 1;
        } else {
            strcpy(buffer, p);
        }
        parseNameValue(buffer, &nameValue, strip);

        if (nameValue.name != NULL && nameValue.value != NULL) {
            pairCount++;
            if (pairCount > nameValueList->listSize) {
                nameValueList->nameValue = (NAMEVALUE*) realloc((void*) nameValueList->nameValue,
                                                                (nameValueList->listSize + 10) * sizeof(NAMEVALUE));
                nameValueList->listSize = nameValueList->listSize + 10;
            }
            nameValueList->pairCount = pairCount;
            nameValueList->nameValue[pairCount - 1] = nameValue;
        }
    } while (p2 != NULL);

// housekeeping

    free((void*) buffer);
    free((void*) copy);

    for (i = 0; i < nameValueList->pairCount; i++) {
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
        for (i = 0; i < nameValueList->pairCount; i++) {
            if ((p = strchr(nameValueList->nameValue[i].value, substitute)) != NULL) {
                do {
                    p[0] = delimiter;
                } while ((p = strchr(nameValueList->nameValue[i].value, substitute)) != NULL);
            }
            if ((p = strchr(nameValueList->nameValue[i].pair, substitute)) != NULL) {
                do {
                    p[0] = delimiter;
                } while ((p = strchr(nameValueList->nameValue[i].pair, substitute)) != NULL);
            }
        }
    }

    return pairCount;
}

//------------------------------------------------------------------------------
// Permission to Read a file? Does it exist?

// include <unistd.h>
//static void readPermission(REQUEST_BLOCK *request_block){
// int access (const char *filename, int how)
// The how argument either can be the bitwise OR of the flags R_OK, W_OK, X_OK, or the existence test F_OK.
//}

