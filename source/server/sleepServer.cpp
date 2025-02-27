/*---------------------------------------------------------------
 * Server Sleeps and Waits for a Wake-up Instruction from the Client
 *
 * Returns:     1 (True)  if Next Client Request to be Served
 *        0 (False) if No Request within the Time Limit
 *
 *--------------------------------------------------------------*/
#include "sleepServer.h"

#include "clientserver/errorLog.h"
#include "protocol/protocol.h"
#include "protocol/xdr_lib.h"
#include "logging/logging.h"
#include "server/udaServer.h"

using namespace uda::client_server;
using namespace uda::logging;
using namespace uda::structures;

int uda::server::sleepServer(XDR* server_input, XDR* server_output, LogMallocList* logmalloclist,
                             UserDefinedTypeList* userdefinedtypelist, int protocolVersion,
                             LogStructList* log_struct_list, int server_tot_block_time, int server_timeout,
                             IoData* io_data, int private_flags, int malloc_source)
{
    ProtocolId protocol_id;
    ProtocolId next_protocol;
    int err;
    int rc;

    protocol_id = ProtocolId::NextProtocol;
    next_protocol = ProtocolId::Start;

    UDA_LOG(UDA_LOG_DEBUG, "Entering Server Sleep Loop");

    UDA_LOG(UDA_LOG_DEBUG, "Protocol 3 Listening for Next Client Request");

    if ((err = protocol(server_input, protocol_id, XDRStreamDirection::Receive, &next_protocol, logmalloclist, userdefinedtypelist,
                        nullptr, protocolVersion, log_struct_list, io_data, private_flags, malloc_source)) != 0) {

        UDA_LOG(UDA_LOG_DEBUG, "Protocol 3 Error Listening for Wake-up {}", err);

        if (server_tot_block_time <= 1000 * server_timeout) {
            add_error(ErrorType::Code, "sleepServer", err, "Protocol 3 Error: Listening for Server Wake-up");
        }
        return 0;
    }

    rc = xdrrec_eof(server_input);
    UDA_LOG(UDA_LOG_DEBUG, "Next Client Request Heard");
    UDA_LOG(UDA_LOG_DEBUG, "Serve Awakes!");
    UDA_LOG(UDA_LOG_DEBUG, "Next Protocol {} Received", next_protocol);
    UDA_LOG(UDA_LOG_DEBUG, "XDR #G xdrrec_eof ? {}", rc);

#ifndef NOCHAT
    // Echo Next Protocol straight back to Client
    if ((err = protocol(server_output, protocol_id, XDRStreamDirection::Send, &next_protocol, logmalloclist, userdefinedtypelist,
                        nullptr, protocolVersion, log_struct_list, io_data, private_flags, malloc_source)) != 0) {
        add_error(ErrorType::Code, "sleepServer", err, "Protocol 3 Error Echoing Next Protocol ID");
        return 0;
    }
#endif

    if (next_protocol == ProtocolId::CloseDown) {
        UDA_LOG(UDA_LOG_DEBUG, "Client Requests Server Shutdown");
        return 0;
    }

    if (next_protocol != ProtocolId::WakeUp) {
        UDA_LOG(UDA_LOG_DEBUG, "Unknown Wakeup Request -> Server Shutting down");
        add_error(ErrorType::Code, "sleepServer", (int)next_protocol, "Unknown Wakeup Request -> Server Shutdown");
        return 0;
    }

    UDA_LOG(UDA_LOG_DEBUG, "Client Requests Server Wake-Up");

    return 1; // No Time out and Non-Zero Next Protocol id => Next Client Request
}
