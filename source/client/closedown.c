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
#include <client/udaClientHostList.h>

#ifdef FATCLIENT
#  include <server/udaServer.h>
#  include <server/closeServerSockets.h>
#  include <server/sqllib.h>
#else
#  include "getEnvironment.h"
#  include "connection.h"
#endif

#if defined(SSLAUTHENTICATION) && !defined(FATCLIENT)
#  include <authentication/udaSSL.h>
#endif

int idamClosedown(int type, SOCKETLIST* socket_list)
{
    int rc = 0;

    UDA_LOG(UDA_LOG_DEBUG, "IdamAPI: idamCloseDown called (%d)\n", type);
    if (type == CLOSE_ALL) {
        UDA_LOG(UDA_LOG_DEBUG, "IdamAPI: Closing Log Files, Streams and Sockets\n");
    } else {
        UDA_LOG(UDA_LOG_DEBUG, "IdamAPI: Closing Streams and Sockets\n");
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
        if (gDBConnect != NULL) {
            PQfinish(gDBConnect);    // close the IDAM SQL Database connection
            gDBConnect = NULL;
        }
#endif
        closeServerSockets(socket_list);    // Close the Socket Connections to Other Data Servers
    }
#endif

    // Close the host list
    
    udaClientFreeHostList();

    // Close the SSL binding and context

#if defined(SSLAUTHENTICATION) && !defined(FATCLIENT)
    closeUdaClientSSL();
#endif

    return rc;
}
