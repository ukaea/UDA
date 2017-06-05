/*---------------------------------------------------------------
* Reads the Requested Data
*
* Input Arguments:	1) Signal Name (Alias or Generic)
*			2) Data Source or Experiment Number
*
* Returns:
*
*--------------------------------------------------------------*/

#include "udaGetAPI.h"

#include <stdarg.h>
#include <strings.h>

#include <logging/logging.h>
#include <clientserver/initStructs.h>
#include <clientserver/errorLog.h>
#include <clientserver/printStructs.h>

#include "makeClientRequestBlock.h"
#include "startup.h"
#include "udaClient.h"
#include "accAPI.h"

#ifdef MEMDEBUG
#include <mcheck.h>
#endif

//! the principal IDAM API
/** All requests are passed via two string arguments. These take multiple forms or patterns.\n\n
Data source patterns:\n\n

<table border="1">
<tr>
<th>object</th>
<th>source</th>
</tr>
<tr>
<td>row 1, cell 1</td>
<td><b>pulse</b> - an integer plasma shot number</td>
</tr>
<tr>
<td>row 2, cell 1</td>
<td><b>pulse/pass</b> - include a pass or sequence number which may be text based e.g. LATEST</td>
</tr>
<tr>
<td>row 1, cell 1</td>
<td><b>DEVICE::pulse</b> - prefix the shot number with a Device Name.</td>
</tr>
<tr>
<td>row 2, cell 1</td>
<td><b>DEVICE::pulse/pass</b></td>
</tr>
<tr>
<td>row 2, cell 1</td>
<td><b>FORMAT::/path/to/my/file</b> - a private file with a specified format. The server must have read access permission.</td>
</tr>
<tr>
<td>row 2, cell 1</td>
<td><b>FORMAT::./path/to/my/file</b> - relative paths are resolved by the client and must be accessible on the network by the server.
FORMAT::../path/to/my/file</b></td>
</tr>
<tr>
<td>row 2, cell 1</td>
<td><b>FORMAT::/scratch/path/to/my/file</b> user file directories local to the client's workstation may have to be changed. This is
done automatically using the environment variables: .... </td>
</tr>
<tr>
<td>row 2, cell 1</td>
<td><b>FORMAT::pulse</b> FORMAT must be the default FORMAT, e.g. IDA3. The IDAM server treats this file as a private file and has
built-in rules to locate the source file within the data archive.</td>
</tr>
<tr>
<td>row 2, cell 1</td>
<td><b>FORMAT::/pulse</b> Treated as a private file.
<b>FORMAT::/pulse/pass</b></td>
</tr>
<tr>
<td>row 2, cell 1</td>
<td><b>/pulse</b> Treated as a private file with the default file FORMAT.
<b>/pulse/pass</b></td>
</tr>

<tr>
<td>row 2, cell 1</td>
<td><b>DEVICE::FORMAT::/path/to/my/file</b> The request is passed to a different data server identified by the DEVICE. There it is
treated as a private file with the default file FORMAT.
<b>/pulse/pass</b></td>
</tr>
<tr>
<td>row 2, cell 1</td>
<td><b>DEVICE::FORMAT::pulse</b> The request is passed to a different data server identified by the DEVICE. There it is
treated as a private file within the data archive if FORMAT is the default file FORMAT.
<b>DEVICE::FORMAT::pulse/pass</b></td>
</tr>

<tr>
<td>row 2, cell 1</td>
<td><b>DEVICE::/pulse</b> The request is passed to a different data server identified by the DEVICE. There it is
treated as a private file within the data archive if FORMAT is the default file FORMAT.
<b>DEVICE::/pulse/pass</b></td>
</tr>

<tr>
<td>row 2, cell 1</td>
<td><b>/path/to/my/file.ext</b> The format is identified using server registered file extensions.</td>
</tr>
<tr>
<td>row 2, cell 1</td>
<td><b>./path/to/my/file.ext</b> Relative file paths are resolved by the client.
<b>../path/to/my/file.ext</b></td>
</tr>
<tr>
<td>row 2, cell 1</td>
<td><b>/scratch/path/to/my/file.ext</b> Relative file paths are resolved by the client.</td>
</tr>


</table>

\b	PROTOCOL::server.host.name:port/U/R/L	server access requests - always requires the delimiter string element in string\n
\n
\b	function(arguments or name value pair list)		server side processing of data\n
\b	LIBRARY::function(arguments or name value pair list)	function plugin library \n
\b	DEVICE::function(arguments or name value pair list)	Not allowed - use DEVICE::SERVERSIDE::function()\n
\n
\b	DEVICE::FORMAT:: ...			If the DEVICE is not the default device, then a server protocol is invoked to pass
                                                the request forward (FORMAT:: ...)\n\n
Legacy exception: treat PPF and JPF formats as server protocols => no file path expansion required and ignored\n
\n
\b      PPF::/ddaname/pulse/pass/userid or PPF::ddaname/pulse/pass/userid\n
\b	JPF::pulse or JPF::/pulse\n

*
* @param data_object identifies the data object to be accessed from a source
* @param data_source identifies the location of data.
* @return a reference ID handle used to identify the accessed data in subsequent API accessor function calls.
*/
int idamGetAPI(const char* data_object, const char* data_source)
{

// Lock the thread
    lockIdamThread();

    int err = 0;
    REQUEST_BLOCK request_block;
    static short startup = 1;

//-------------------------------------------------------------------------
// Memory Debugger

#ifdef MEMDEBUG
    mtrace();
#endif

//-------------------------------------------------------------------------
// Open the Logs

    IDAM_LOG(UDA_LOG_DEBUG, "Calling idamStartup\n");

    if (idamStartup(0) != 0) {
        unlockIdamThread();
        return PROBLEM_OPENING_LOGS;
    }

    IDAM_LOG(UDA_LOG_DEBUG, "Returned from idamStartup\n");

//-------------------------------------------------------------------------
// Log all Arguments passed from Application

#ifdef ARGSTACK
    if(argstack == NULL) {
        char tempFile[] = "/tmp/idamStackXXXXXX";
        mkstemp(tempFile);
        argstack = fopen(tempFile, environment.logmode);
        if(argstack != NULL) fprintf(argstack, "idamGetAPI\n");
    }
    if(argstack != NULL) {
        fprintf(argstack,"[%s][%s]\n", data_object, data_source);
        fflush(argstack);
    }
#endif

//-------------------------------------------------------------------------
// Initialise the Client Data Request Structure

    initRequestBlock(&request_block);

//------------------------------------------------------------------------------
// Build the Request Data Block (Version and API dependent)

    if (startup) {
        initIdamErrorStack(&idamerrorstack);
        startup = 0;
    }

    if ((err = makeClientRequestBlock(data_object, data_source, &request_block)) != 0) {
        if (idamerrorstack.nerrors == 0) {
            IDAM_LOGF(UDA_LOG_ERROR, "Error identifying the Data Source [%s]\n", data_source);
            addIdamError(&idamerrorstack, CODEERRORTYPE, __func__, 999, "Error identifying the Data Source");
        }
        unlockIdamThread();
        return -err;
    }

    IDAM_LOG(UDA_LOG_DEBUG, "Routine: idamGetAPI\n");
    printRequestBlock(request_block);

//-------------------------------------------------------------------------
// Fetch Data

#ifdef TESTSERVERCLIENT
    unlockIdamThread();    
    return -1;
#endif

    err = idamClient(&request_block);

//-------------------------------------------------------------------------
// Memory Debugger Exit

#ifdef MEMDEBUG
    muntrace();
#endif

// Unlock the thread
    unlockIdamThread();
    return err;
}