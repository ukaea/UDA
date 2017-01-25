/*---------------------------------------------------------------
* Close the Socket, XDR Streams, Open File Handles
*
* Argument: 	If 1 then full closedown otherwise only the
*		Socket and XDR Streams.
*
* Returns:
*
*--------------------------------------------------------------*/
#include "closedown.h"

#include <logging/idamLog.h>
#include <include/idamclientprivate.h>

#ifdef FATCLIENT
#  include "idamserver.h"
#  include <libpq-fe.h>
#  include "closeServerSockets.h"
extern PGconn * DBConnect;    // IDAM database Socket Connection
#else
#  include "closeClientSockets.h"
#endif

int idamClosedown(int type)
{
    int rc = 0;

    idamLog(LOG_DEBUG, "IdamAPI: idamCloseDown called (%d)\n", type);
    if (type == 1)
        idamLog(LOG_DEBUG, "IdamAPI: Closing Log Files, Streams and Sockets\n");
    else
        idamLog(LOG_DEBUG, "IdamAPI: Closing Streams and Sockets\n");

    if (type == 1) {
        idamCloseLogging();
        reopen_logs = 1;        // In case the User calls the IDAM API again!
    }

#ifndef FATCLIENT    // <========================== Client Server Code Only

    if (clientInput->x_ops != NULL) xdr_destroy(clientInput);
    if (clientOutput->x_ops != NULL) xdr_destroy(clientOutput);
    clientOutput->x_ops = NULL;
    clientInput->x_ops = NULL;

    if (clientSocket >= 0 && type != 1)
        closeIdamClientSocket(&client_socketlist, clientSocket);
    else
        closeIdamClientSockets(&client_socketlist);

    clientSocket = -1;
    env_host = 1;            // Initialise at Startup
    env_port = 1;

#else			// <========================== Fat Client Code Only

    if (type == 1) {

#ifndef NOTGENERICENABLED
        if (DBConnect != NULL) {
            PQfinish(DBConnect);    // close the IDAM SQL Database connection
            DBConnect = NULL;
        }
#endif
        closeServerSockets(&server_socketlist);    // Close the Socket Connections to Other Data Servers
#ifdef FILELISTTEST
        closeIdamFiles(&idamfilelist);			// Close Open Data Source Files
#endif
    }
#endif

    return rc;
}
