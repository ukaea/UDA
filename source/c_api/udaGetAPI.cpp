/*---------------------------------------------------------------
 * Reads the Requested Data
 *
 * Input Arguments:    1) Signal Name (Alias or Generic)
 *            2) Data Source or Experiment Number
 *
 * Returns:
 *
 *--------------------------------------------------------------*/

#include "udaGetAPI.h"

#ifndef _WIN32
#  include <strings.h>
#endif

#include "include/errorLog.h"
#include "clientserver/printStructs.h"
#include "initStructs.h"
#include "include/logging.h"

#include "accAPI.h"
#include "client.h"
#include "client/getEnvironment.h"
#include "client/makeClientRequestBlock.h"
#include "client/startup.h"

#ifdef MEMDEBUG
#  include <mcheck.h>
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
<td><b>FORMAT::/path/to/my/file</b> - a private file with a specified format. The server must have read access
permission.</td>
</tr>
<tr>
<td>row 2, cell 1</td>
<td><b>FORMAT::./path/to/my/file</b> - relative paths are resolved by the client and must be accessible on the network
by the server. FORMAT::../path/to/my/file</b></td>
</tr>
<tr>
<td>row 2, cell 1</td>
<td><b>FORMAT::/scratch/path/to/my/file</b> user file directories local to the client's workstation may have to be
changed. This is done automatically using the environment variables: .... </td>
</tr>
<tr>
<td>row 2, cell 1</td>
<td><b>FORMAT::pulse</b> FORMAT must be the default FORMAT, e.g. IDA3. The IDAM server treats this file as a private
file and has built-in rules to locate the source file within the data archive.</td>
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
<td><b>DEVICE::FORMAT::/path/to/my/file</b> The request is passed to a different data server identified by the DEVICE.
There it is treated as a private file with the default file FORMAT. <b>/pulse/pass</b></td>
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

\b    PROTOCOL::server.host.name:port/U/R/L    server access requests - always requires the delimiter string element in
string\n \n \b    function(arguments or name value pair list)        server side processing of data\n \b
LIBRARY::function(arguments or name value pair list)    function plugin library \n \b    DEVICE::function(arguments or
name value pair list)    Not allowed - use DEVICE::SERVERSIDE::function()\n \n \b    DEVICE::FORMAT:: ...            If
the DEVICE is not the default device, then a server protocol is invoked to pass the request forward (FORMAT:: ...)\n\n
Legacy exception: treat PPF and JPF formats as server protocols => no file path expansion required and ignored\n
\n
\b      PPF::/ddaname/pulse/pass/userid or PPF::ddaname/pulse/pass/userid\n
\b    JPF::pulse or JPF::/pulse\n

*
* @param data_object identifies the data object to be accessed from a source
* @param data_source identifies the location of data.
* @return a reference ID handle used to identify the accessed data in subsequent API accessor function calls.
*/
int udaGetAPI(const char* data_object, const char* data_source)
{
    return udaGetAPIWithHost(data_object, data_source, nullptr, 0);
}

int udaGetAPIWithHost(const char* data_object, const char* data_source, const char* host, int port)
{
    CLIENT_FLAGS* client_flags = udaClientFlags();

    // Lock the thread
    udaLockThread();

    if (host != nullptr) {
        udaPutServerHost(host);
    }

    if (port) {
        udaPutServerPort(port);
    }

    int err = 0;
    static bool startup = true;

    //-------------------------------------------------------------------------
    // Memory Debugger

#ifdef MEMDEBUG
    mtrace();
#endif

    //-------------------------------------------------------------------------
    // Open the Logs

    UDA_LOG(UDA_LOG_DEBUG, "Calling udaStartup\n");

    static bool reopen_logs = true;

    if (udaStartup(0, client_flags, &reopen_logs) != 0) {
        udaUnlockThread();
        return PROBLEM_OPENING_LOGS;
    }

    UDA_LOG(UDA_LOG_DEBUG, "Returned from udaStartup\n");

    //-------------------------------------------------------------------------
    // Log all Arguments passed from Application

#ifdef ARGSTACK
    if (argstack == nullptr) {
        char tempFile[] = "/tmp/idamStackXXXXXX";
        mkstemp(tempFile);
        argstack = fopen(tempFile, environment.logmode);
        if (argstack != nullptr) {
            fprintf(argstack, "udaGetAPI\n");
        }
    }
    if (argstack != nullptr) {
        fprintf(argstack, "[%s][%s]\n", data_object, data_source);
        fflush(argstack);
    }
#endif

    //-------------------------------------------------------------------------
    // Initialise the Client Data Request Structure

    REQUEST_BLOCK request_block;
    initRequestBlock(&request_block);

    //------------------------------------------------------------------------------
    // Build the Request Data Block (Version and API dependent)

    if (startup) {
        udaInitErrorStack();
        startup = false;
    }

    if ((err = makeClientRequestBlock(&data_object, &data_source, 1, &request_block)) != 0) {
        if (udaNumErrors() == 0) {
            UDA_LOG(UDA_LOG_ERROR, "Error identifying the Data Source [%s]\n", data_source);
            udaAddError(UDA_CODE_ERROR_TYPE, __func__, 999, "Error identifying the Data Source");
        }
        udaUnlockThread();
        return -err;
    }

    printRequestBlock(request_block);

    //-------------------------------------------------------------------------
    // Fetch Data

#ifdef TESTSERVERCLIENT
    unudaLockThread();
    return -1;
#endif

    int handle;
    err = idamClient(&request_block, &handle);
    if (err < 0) {
        handle = err;
    }

    //-------------------------------------------------------------------------
    // Memory Debugger Exit

#ifdef MEMDEBUG
    muntrace();
#endif

    freeClientRequestBlock(&request_block);
    // Unlock the thread
    udaUnlockThread();
    return handle;
}

int udaGetBatchAPI(const char** signals, const char** sources, int count, int* handles)
{
    return udaGetBatchAPIWithHost(signals, sources, count, handles, nullptr, 0);
}

int udaGetBatchAPIWithHost(const char** signals, const char** sources, int count, int* handles, const char* host,
                            int port)
{
    CLIENT_FLAGS* client_flags = udaClientFlags();

    // Lock the thread
    udaLockThread();

    if (host != nullptr) {
        udaPutServerHost(host);
    }

    if (port) {
        udaPutServerPort(port);
    }

    static bool startup = true;

    //-------------------------------------------------------------------------
    // Memory Debugger

#ifdef MEMDEBUG
    mtrace();
#endif

    //-------------------------------------------------------------------------
    // Open the Logs

    UDA_LOG(UDA_LOG_DEBUG, "Calling udaStartup\n");

    static bool reopen_logs = true;

    if (udaStartup(0, client_flags, &reopen_logs) != 0) {
        udaUnlockThread();
        return PROBLEM_OPENING_LOGS;
    }

    UDA_LOG(UDA_LOG_DEBUG, "Returned from udaStartup\n");

    //-------------------------------------------------------------------------
    // Log all Arguments passed from Application

#ifdef ARGSTACK
    if (argstack == nullptr) {
        char tempFile[] = "/tmp/idamStackXXXXXX";
        mkstemp(tempFile);
        argstack = fopen(tempFile, environment.logmode);
        if (argstack != nullptr) {
            fprintf(argstack, "udaGetAPI\n");
        }
    }
    if (argstack != nullptr) {
        fprintf(argstack, "[%s][%s]\n", data_object, data_source);
        fflush(argstack);
    }
#endif

    //-------------------------------------------------------------------------
    // Initialise the Client Data Request Structure

    REQUEST_BLOCK request_block;
    initRequestBlock(&request_block);

    //------------------------------------------------------------------------------
    // Build the Request Data Block (Version and API dependent)

    if (startup) {
        udaInitErrorStack();
        startup = false;
    }

    int err = 0;
    if ((err = makeClientRequestBlock(signals, sources, count, &request_block)) != 0) {
        if (udaNumErrors() == 0) {
            udaAddError(UDA_CODE_ERROR_TYPE, __func__, 999, "Error identifying the Data Source");
        }
        udaUnlockThread();
        return -err;
    }

    UDA_LOG(UDA_LOG_DEBUG, "Routine: udaGetBatchAPI\n");
    printRequestBlock(request_block);

    //-------------------------------------------------------------------------
    // Fetch Data

#ifdef TESTSERVERCLIENT
    unudaLockThread();
    return -1;
#endif

    err = idamClient(&request_block, handles);

    //-------------------------------------------------------------------------
    // Memory Debugger Exit

#ifdef MEMDEBUG
    muntrace();
#endif

    // Unlock the thread
    udaUnlockThread();
    return err;
}
