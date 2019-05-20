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

#include <stdlib.h>
#ifdef __GNUC__
#  include <strings.h>
#elif defined(_WIN32)
#  define strcasecmp _stricmp
#endif

#include <logging/logging.h>
#include <clientserver/udaErrors.h>
#include <clientserver/errorLog.h>
#include <clientserver/expand_path.h>
#include <clientserver/stringUtils.h>

int makeClientRequestBlock(const char* data_object, const char* data_source, REQUEST_BLOCK* request_block)
{
    int lstr, ldelim, err = 0;
    char* test = nullptr;

    //------------------------------------------------------------------------------------------------------------------
    //! Test Input Arguments comply with string length limits, then copy to the request structure without modification

    if (strlen(data_object) >= MAXMETA) {
        THROW_ERROR(SIGNAL_ARG_TOO_LONG, "The Signal/Data Object Argument string is too long!");
    } else {
        strcpy(request_block->signal, data_object);    // Passed to the server without modification
    }

    if (strlen(data_source) >= STRING_LENGTH) {
        THROW_ERROR(SOURCE_ARG_TOO_LONG, "The Data Source Argument string is too long!");
    } else {
        strcpy(request_block->source, data_source);    // Passed to the server without modification
    }

    //------------------------------------------------------------------------------------------------------------------
    /* Signal and source arguments use a prefix to identify archive or device names, file formats or server protocols.
     * These prefixes are attached to the main signal or source details using a delimiting string, e.g. "::".
     * This delimiting string can be defined by the user via an environment variable "UDA_API_DELIM".
     * This must be passed to the server as it needs to separate the prefix from the main component in order to
     * interpret the data access request.
    */

    ENVIRONMENT* environment = getIdamClientEnvironment();

    strcpy(request_block->api_delim, environment->api_delim);        // Server needs to know how to parse the arguments

    //------------------------------------------------------------------------------------------------------------------
    /* If the default ARCHIVE and/or DEVICE is overridden by local environment variables and the arguments do not contain
     * either an archive or device then prefix
     *
     * These environment variables are legacy and not used by the server
    */

    if (strcasecmp(environment->api_device, API_DEVICE) != 0 &&
        strstr(request_block->source, request_block->api_delim) == nullptr) {
        lstr = (int)strlen(request_block->source) + (int)strlen(environment->api_device) +
               (int)strlen(request_block->api_delim);
        if (lstr >= STRING_LENGTH) {
            THROW_ERROR(SOURCE_ARG_TOO_LONG, "The Data Source Argument, prefixed with the Device Name, is too long!");
        }
        test = (char*)malloc((lstr + 1) * sizeof(char));
        sprintf(test, "%s%s%s", environment->api_device, request_block->api_delim, request_block->source);
        strcpy(request_block->source, test);
        free(test);
    }

    if (strcasecmp(environment->api_archive, API_ARCHIVE) != 0 &&
        strstr(request_block->signal, request_block->api_delim) == nullptr) {
        lstr = (int)strlen(request_block->signal) + (int)strlen(environment->api_archive) +
               (int)strlen(request_block->api_delim);
        if (lstr >= STRING_LENGTH) {
            THROW_ERROR(SIGNAL_ARG_TOO_LONG, "The Signal/Data Object Argument, prefixed with the Archive Name, is too long!");
        }
        test = (char*)malloc((lstr + 1) * sizeof(char));
        sprintf(test, "%s%s%s", environment->api_archive, request_block->api_delim, request_block->signal);
        strcpy(request_block->signal, test);
        free(test);
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
    // XXXX::12345		shot number
    // XXXX::12345/a 	keyword or pass number
    // XXXX::12345/a,b,c	keywords or substitution values
    // XXXX::12345/a=b,c=d	name-value pairs
    // XXXX::a
    // XXXX::a,b,c
    // XXXX::a=b,c=d
    // XXXX::/path/to/data/resource

    if ((test = strstr(request_block->source, request_block->api_delim)) == nullptr) {
        if (strchr(request_block->source, '(') == nullptr &&
            strchr(request_block->source, ')') == nullptr) {
            // source is not a function call
            strcpy(request_block->path, request_block->source);
            expandFilePath(request_block->path, getIdamClientEnvironment());
        }
    } else {
        if (strchr(test, '(') == nullptr && strchr(test, ')') == nullptr) {
            // Prefixed and not a function call
            ldelim = (int)strlen(request_block->api_delim);
            strcpy(request_block->path, &test[ldelim]);
            expandFilePath(request_block->path, getIdamClientEnvironment());
        }
    }

    return err;
}

int shotRequestTest(const char* source)
{
    // Return 1 (TRUE) if the source is shot nuumber based , 0 (FALSE) otherwise

    char* token = nullptr;
    char work[STRING_LENGTH];

    if (source[0] == '\0') return 0;
    if (source[0] == '/') return 0;        // Directory based data

    //------------------------------------------------------------------------------
    // Check if the source has one of these forms:

    // pulse		plasma shot number - an integer
    // pulse/pass		include a pass or sequence number - this may be a text based component, e.g. LATEST

    if (IsNumber((char*) source)) return 1;		// The source an integer number 
        
    strcpy(work, source);
    
    if ((token = strtok(work, "/")) != nullptr) {		// Tokenise the remaining string
       if (IsNumber(token)) return 1;			// Is the First token an integer number?
    }

    return 0;
}

