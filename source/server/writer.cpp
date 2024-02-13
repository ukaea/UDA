#include "writer.h"

#include <cerrno>

#include "clientserver/udaDefines.h"
#include "logging/logging.h"

#include "createXDRStream.h"

#if !defined(__GNUC__)
#  include <io.h>
#  define read _read
#  define write _write
#endif

int serverSocket = 0;

/*
//-----------------------------------------------------------------------------------------
// This routine is only called when the Server expects to Read something from the Client
//
// There are two time constraints:
//
//    The Maximum Blocking period is 1ms when reading
//    A Maximum number (MAXLOOP) of blocking periods is allowed before this time
//    is modified: It is extended to 100ms to minimise server resource consumption.
//
// When the Server is in a Holding state, it is listening to the Socket for either a
// Closedown or a Data request.
//
// Three Global variables are used to control the Blocking timeout
//
//    min_block_time
//    max_block_time
//    tot_block_time
//
// A Maximum time (MAXBLOCK) from the last Data Request is permitted before the Server Automatically
// closes down.
//-----------------------------------------------------------------------------------------
*/

using namespace uda::client_server;

int uda::server::server_read(void* iohandle, char* buf, int count)
{
    int rc = 0;
    fd_set rfds; // File Descriptor Set for Reading from the Socket
    timeval tv = {};
    timeval tvc = {};

    auto io_data = reinterpret_cast<uda::server::IoData*>(iohandle);

    // Wait until there are data to be read from the socket

    setSelectParms(serverSocket, &rfds, &tv, io_data->server_tot_block_time);
    tvc = tv;

    while (select(serverSocket + 1, &rfds, nullptr, nullptr, &tvc) <= 0) {
        *io_data->server_tot_block_time += (int)tv.tv_usec / 1000;
        if (*io_data->server_tot_block_time > 1000 * *io_data->server_timeout) {
            UDA_LOG(UDA_LOG_DEBUG, "Total Wait Time Exceeds Lifetime Limit = %d (ms)\n",
                    *io_data->server_timeout * 1000);
            return -1;
        }

        updateSelectParms(serverSocket, &rfds, &tv, *io_data->server_tot_block_time); // Keep trying ...
        tvc = tv;
    }

    // Read from it, checking for EINTR, as happens if called from IDL

    while (((rc = (int)read(serverSocket, buf, count)) == -1) && (errno == EINTR)) {}

    // As we have waited to be told that there is data to be read, if nothing
    // arrives, then there must be an error

    if (!rc) {
        rc = -1;
    }

    return rc;
}

int uda::server::server_write(void* iohandle, char* buf, int count)
{

    // This routine is only called when there is something to write back to the Client

    int rc = 0;
    int BytesSent = 0;

    fd_set wfds; // File Descriptor Set for Writing to the Socket
    timeval tv = {};

    auto io_data = reinterpret_cast<uda::server::IoData*>(iohandle);

    // Block IO until the Socket is ready to write to Client

    setSelectParms(serverSocket, &wfds, &tv, io_data->server_tot_block_time);

    while (select(serverSocket + 1, nullptr, &wfds, nullptr, &tv) <= 0) {
        *io_data->server_tot_block_time += tv.tv_usec / 1000;
        if (*io_data->server_tot_block_time / 1000 > *io_data->server_timeout) {
            UDA_LOG(UDA_LOG_DEBUG, "Total Blocking Time: %d (ms)\n", *io_data->server_tot_block_time);
            return -1;
        }
        updateSelectParms(serverSocket, &wfds, &tv, *io_data->server_tot_block_time);
    }

    // Write to socket, checking for EINTR, as happens if called from IDL

    while (BytesSent < count) {
        while (((rc = (int)write(serverSocket, buf, count)) == -1) && (errno == EINTR)) {}
        BytesSent += rc;
        buf += rc;
    }

    return rc;
}
