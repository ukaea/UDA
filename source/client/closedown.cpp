/*---------------------------------------------------------------
 * Close the Socket, XDR Streams, Open File Handles
 *
 * Argument:     If 1 then full closedown otherwise only the
 *        Socket and XDR Streams.
 *
 * Returns:
 *
 *--------------------------------------------------------------*/
#include "closedown.h"

#include "client/udaClientHostList.h"
#include "logging/logging.h"

#ifdef FATCLIENT
#  include "server/closeServerSockets.h"
#  include "server/udaServer.h"
using namespace uda::server;
#else
#  include "connection.h"
#  include "getEnvironment.h"
#endif

#if defined(SSLAUTHENTICATION) && !defined(FATCLIENT)
#  include <authentication/udaClientSSL.h>
#endif

using namespace uda::client;

int uda::client::closedown(ClosedownType type, uda::client_server::SOCKETLIST* socket_list, XDR* client_input,
                           XDR* client_output, bool* reopen_logs)
{
    int rc = 0;

    UDA_LOG(UDA_LOG_DEBUG, "idamCloseDown called (%d)\n", type);
    if (type == ClosedownType::CLOSE_ALL) {
        UDA_LOG(UDA_LOG_DEBUG, "Closing Log Files, Streams and Sockets\n");
    } else {
        UDA_LOG(UDA_LOG_DEBUG, "Closing Streams and Sockets\n");
    }

    if (type == ClosedownType::CLOSE_ALL) {
        udaCloseLogging();
        *reopen_logs = true; // In case the User calls the IDAM API again!
    }

#ifndef FATCLIENT // <========================== Client Server Code Only
    if (client_input != nullptr) {
        if (client_input->x_ops != nullptr) {
            xdr_destroy(client_input);
        }
        client_input->x_ops = nullptr;
    }

    if (client_output != nullptr) {
        if (client_output->x_ops != nullptr) {
            xdr_destroy(client_output);
        }
        client_output->x_ops = nullptr;
    }

    closeConnection(type);

    // Initialise at Startup
    udaSetEnvHost(true);
    udaSetEnvPort(true);

#else // <========================== Fat Client Code Only
    if (type == ClosedownType::CLOSE_ALL) {
        closeServerSockets(socket_list); // Close the Socket Connections to Other Data Servers
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
