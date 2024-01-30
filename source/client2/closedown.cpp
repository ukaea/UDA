#include "closedown.hpp"

#include "logging/logging.h"

#include "connection.hpp"

#ifdef SSLAUTHENTICATION
#  include <authentication/udaClientSSL.h>
#endif

int uda::client::closedown(ClosedownType type, Connection* connection, XDR* client_input, XDR* client_output,
                           bool* reopen_logs, bool* env_host, bool* env_port)
{
    int rc = 0;

    UDA_LOG(UDA_LOG_DEBUG, "closedown called (%d)\n", type);
    if (type == ClosedownType::CLOSE_ALL) {
        UDA_LOG(UDA_LOG_DEBUG, "Closing Log Files, Streams and Sockets\n");
    } else {
        UDA_LOG(UDA_LOG_DEBUG, "Closing Streams and Sockets\n");
    }

    if (type == ClosedownType::CLOSE_ALL) {
        udaCloseLogging();
        *reopen_logs = true; // In case the User calls the IDAM API again!
    }

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

    if (connection != nullptr) {
        connection->close_down(type);
    }

    *env_host = true; // Initialise at Startup
    *env_port = true;

    // Close the SSL binding and context

#if defined(SSLAUTHENTICATION) && !defined(FATCLIENT)
    closeUdaClientSSL();
#endif

    return rc;
}
