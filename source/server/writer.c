/*---------------------------------------------------------------
* Write/Read from the XDR Stream
*
* Returns:
*
* Revision 0.4  10-Jul-2005	D.G.Muir
* v1.0 10Jul2006	dgm	Renamed SERVER_TIMEOUT to user_timeout
*				Max Blocking Time decreased from 100ms to 10ms
* v1.1 21Mar2007	dgm	user_timeout renamed server_timeout
*				tot_block_time renamed server_tot_block_time
*				min_block_time changed to #define MIN_BLOCK_TIME
				max_block_time changed to #define MAX_BLOCK_TIME
				DB_Socket renamed serverSocket

* v1.2 09Jul2007	dgm	debugon enabled
// 08Jul2009	dgm	server_tot_block_time moved to idamServer as static
*--------------------------------------------------------------*/

#include "writer.h"

#include <unistd.h>
#include <idamLog.h>

#include "idamServerStartup.h"

int serverSocket = 0;

void setSelectParms(int fd, fd_set* rfds, struct timeval* tv)
{
    FD_ZERO(rfds);    // Initialise the File Descriptor set
    FD_SET(fd, rfds);    // Identify the Socket in the FD set
    tv->tv_sec = 0;
    tv->tv_usec = MIN_BLOCK_TIME;    // minimum wait
    server_tot_block_time = 0;
}

void updateSelectParms(int fd, fd_set* rfds, struct timeval* tv)
{
    FD_ZERO(rfds);
    FD_SET(fd, rfds);
    if (server_tot_block_time < MAXBLOCK) {    // For the First Second have rapid response
        tv->tv_sec = 0;
        tv->tv_usec = MIN_BLOCK_TIME;    // minimum wait
    } else {
        tv->tv_sec = 0;
        tv->tv_usec = MAX_BLOCK_TIME;    // maximum wait
    }
}

int Writeout(void* iohandle, char* buf, int count)
{

// This routine is only called when there is something to write back to the Client

    int rc;
    int BytesSent = 0;

    fd_set wfds;        // File Descriptor Set for Writing to the Socket
    struct timeval tv;

// Block IO until the Socket is ready to write to Client

    setSelectParms(serverSocket, &wfds, &tv);

    while (select(serverSocket + 1, NULL, &wfds, NULL, &tv) <= 0) {
        server_tot_block_time += tv.tv_usec / 1000;
        if (server_tot_block_time / 1000 > server_timeout) {
            IDAM_LOGF(LOG_DEBUG, "Writeout: Total Blocking Time: %d (ms)\n", server_tot_block_time);
        }
        if (server_tot_block_time / 1000 > server_timeout) return (-1);
        updateSelectParms(serverSocket, &wfds, &tv);
    }

// Write to socket, checking for EINTR, as happens if called from IDL

    while (BytesSent < count) {
        while (((rc = (int) write(serverSocket, buf, count)) == -1) && (errno == EINTR)) {}
        BytesSent += rc;
        buf += rc;
    }

    return rc;
}

/*
//-----------------------------------------------------------------------------------------
// This routine is only called when the Server expects to Read something from the Client
//
// There are two time constraints:
//
//	The Maximum Blocking period is 1ms when reading
//	A Maximum number (MAXLOOP) of blocking periods is allowed before this time
//	is modified: It is extended to 100ms to minimise server resource consumption.
//
// When the Server is in a Holding state, it is listening to the Socket for either a
// Closedown or a Data request.
//
// Three Global variables are used to control the Blocking timeout
//
//	min_block_time
//	max_block_time
//	tot_block_time
//
// A Maximum time (MAXBLOCK in seconds) from the last Data Request is permitted before the Server Automatically
// closes down.
//-----------------------------------------------------------------------------------------
*/

int Readin(void* iohandle, char* buf, int count)
{

    int rc;
    fd_set rfds;        // File Descriptor Set for Reading from the Socket
    struct timeval tv, tvc;

// Wait until there are data to be read from the socket

    setSelectParms(serverSocket, &rfds, &tv);
    tvc = tv;

    while (select(serverSocket + 1, &rfds, NULL, NULL, &tvc) <= 0) {
        server_tot_block_time = server_tot_block_time + (int) tv.tv_usec / 1000;
        if (server_tot_block_time > 1000 * server_timeout) {
            IDAM_LOGF(LOG_DEBUG, "Readin: Total Wait Time Exceeds Lifetime Limit = %d (ms)\n", server_timeout * 1000);
        }

        if (server_tot_block_time > 1000 * server_timeout) return (-1);

        updateSelectParms(serverSocket, &rfds, &tv);        // Keep trying ...
        tvc = tv;
    }

// Read from it, checking for EINTR, as happens if called from IDL

    while (((rc = (int) read(serverSocket, buf, count)) == -1) && (errno == EINTR)) {}

// As we have waited to be told that there is data to be read, if nothing
// arrives, then there must be an error

    if (!rc) rc = -1;

    return rc;
}

