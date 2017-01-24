//! $LastChangedRevision: 353 $
//! $LastChangedDate: 2013-11-18 15:32:28 +0000 (Mon, 18 Nov 2013) $
//! $LastChangedBy: dgm $
//! $HeadURL: https://fussvn.fusion.culham.ukaea.org.uk/svnroot/IDAM/development/source/client/Writeout.c $

// Create the Client Side XDR File Stream Writer Function
//
// Change History
//
// 21Mar2007	dgm	DB_Socket renamed clientSocket
// 24Oct2007	dgm	ERRORSTACK components added
// 16Dec2008	dgm	Test for ECONNRESET, ENETUNREACH and ECONNREFUSED added
// 31Jan2011	dgm	Windows sockets implementation
//----------------------------------------------------------------

#include "Writeout.h"

#include <unistd.h>
#include <idamLog.h>
#include <signal.h>

#include "UpdateSelectParms.h"
#include "idamErrorLog.h"

int idamClientWriteout(void* iohandle, char* buf, int count)
{
    void (* OldSIGPIPEHandler)();
    int rc = 0;
    size_t BytesSent = 0;

    fd_set wfds;
    struct timeval tv;

    idamUpdateSelectParms(clientSocket, &wfds, &tv);

    errno = 0;

    while (select(clientSocket + 1, NULL, &wfds, NULL, &tv) <= 0) {

        if (errno == ECONNRESET || errno == ENETUNREACH || errno == ECONNREFUSED) {
            if (errno == ECONNRESET) {
                idamLog(LOG_DEBUG, "idamClientWriteout: ECONNRESET error!\n");
                addIdamError(&idamerrorstack, CODEERRORTYPE, "idamClientWriteout", -2,
                             "ECONNRESET: The server program has crashed or closed the socket unexpectedly");
                return -2;
            } else {
                if (errno == ENETUNREACH) {
                    idamLog(LOG_DEBUG, "idamClientWriteout: ENETUNREACH error!\n");
                    addIdamError(&idamerrorstack, CODEERRORTYPE, "idamClientWriteout", -3,
                                 "Server Unavailable: ENETUNREACH");
                    return -3;
                } else {
                    idamLog(LOG_DEBUG, "idamClientWriteout: ECONNREFUSED error!\n");
                    addIdamError(&idamerrorstack, CODEERRORTYPE, "idamClientWriteout", -4,
                                 "Server Unavailable: ECONNREFUSED");
                    return -4;
                }
            }
        }

        idamUpdateSelectParms(clientSocket, &wfds, &tv);
    }

    /* UNIX version

     Ignore the SIG_PIPE signal.  Standard behaviour terminates
     the application with an error code of 141.  When the signal
     is ignored, write calls (when there is no server process to
     communicate with) will return with errno set to EPIPE
    */

    if ((OldSIGPIPEHandler = signal(SIGPIPE, SIG_IGN)) == SIG_ERR) {
        addIdamError(&idamerrorstack, CODEERRORTYPE, "idamClientWriteout", -1, "Error attempting to ignore SIG_PIPE");
        return -1;
    }

// Write to socket, checking for EINTR, as happens if called from IDL

    while (BytesSent < count) {
#ifndef _WIN32
        while (((rc = (int) write(clientSocket, buf, count)) == -1) && (errno == EINTR)) { }
#else
        while ( ( (rc=send( clientSocket, buf , count, 0 )) == SOCKET_ERROR) && (errno == EINTR) ) {}
#endif
        BytesSent += rc;
        buf += rc;
    }

// Restore the original SIGPIPE handler set by the application

    if (signal(SIGPIPE, OldSIGPIPEHandler) == SIG_ERR) {
        addIdamError(&idamerrorstack, CODEERRORTYPE, "idamClientWriteout", -1,
                     "Error attempting to restore SIG_PIPE handler");
        return -1;
    }

    return rc;
}
