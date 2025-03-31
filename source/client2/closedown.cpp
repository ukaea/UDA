#include "closedown.hpp"

#include "logging/logging.h"

#include "connection.hpp"

#include <rpc/xdr.h>

#if defined(SSLAUTHENTICATION) && !defined(FATCLIENT)
#  include <authentication/client_ssl.h>

using namespace uda::authentication;
#endif

using namespace uda::logging;

void uda::client::close_xdr_stream(XDR* stream)
{
    if (stream != nullptr) {
        if (stream->x_ops != nullptr) {
            xdr_destroy(stream);
        }
        stream->x_ops = nullptr;
    }
}

int uda::client::closedown(ClosedownType type, Connection* connection)
{
    int rc = 0;

    UDA_LOG(UDA_LOG_DEBUG, "closedown called ({})", (int)type);
    if (type == ClosedownType::CLOSE_ALL) {
        UDA_LOG(UDA_LOG_DEBUG, "Closing Log Files, Streams and Sockets");
    } else {
        UDA_LOG(UDA_LOG_DEBUG, "Closing Streams and Sockets");
    }

    // xdr streams owned by socket now
    // close_xdr_stream(client_input);
    // close_xdr_stream(client_output);

    if (connection != nullptr) {
        connection->close_down(type);
    }

    // Close the SSL binding and context

#if defined(SSLAUTHENTICATION) && !defined(FATCLIENT)
    close_client_ssl();
#endif

    return rc;
}
