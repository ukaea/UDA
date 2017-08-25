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

#include <logging/logging.h>
#include <client/udaClient.h>

#ifdef FATCLIENT
#  include <server/udaServer.h>
#  include <libpq-fe.h>
#  include <server/closeServerSockets.h>
extern PGconn * DBConnect;    // IDAM database Socket Connection
#else
#  include "getEnvironment.h"
#  include "connection.h"
#endif

int idamClosedown(int type, SOCKETLIST* socket_list)
{
    int rc = 0;

    IDAM_LOGF(UDA_LOG_DEBUG, "IdamAPI: idamCloseDown called (%d)\n", type);
    if (type == CLOSE_ALL) {
        IDAM_LOG(UDA_LOG_DEBUG, "IdamAPI: Closing Log Files, Streams and Sockets\n");
    } else {
        IDAM_LOG(UDA_LOG_DEBUG, "IdamAPI: Closing Streams and Sockets\n");
    }

    if (type == CLOSE_ALL) {
        idamCloseLogging();
        reopen_logs = TRUE;        // In case the User calls the IDAM API again!
    }

#ifndef FATCLIENT    // <========================== Client Server Code Only

    if (clientInput->x_ops != NULL) xdr_destroy(clientInput);
    if (clientOutput->x_ops != NULL) xdr_destroy(clientOutput);
    clientOutput->x_ops = NULL;
    clientInput->x_ops = NULL;

    closeConnection(type);

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
        closeServerSockets(socket_list);    // Close the Socket Connections to Other Data Servers
    }
#endif

    return rc;
}
