/*---------------------------------------------------------------
* Server Sleeps and Waits for a Wake-up Instruction from the Client
*
* Returns:     1 (True)  if Next Client Request to be Served
*        0 (False) if No Request within the Time Limit
*
*--------------------------------------------------------------*/
#include "sleepServer.h"

#include <logging/logging.h>
#include <clientserver/errorLog.h>
#include <clientserver/protocol.h>
#include <server/udaServer.h>
#include <clientserver/xdrlib.h>

int sleepServer(XDR* server_input, XDR* server_output, LOGMALLOCLIST* logmalloclist,
                USERDEFINEDTYPELIST* userdefinedtypelist, int protocolVersion, NTREE* full_ntree,
                LOGSTRUCTLIST* log_struct_list, int server_tot_block_time, int server_timeout, IoData* io_data,
                int private_flags, int malloc_source)
{
    int protocol_id, next_protocol, err, rc;

    protocol_id = PROTOCOL_NEXT_PROTOCOL;
    next_protocol = 0;

    UDA_LOG(UDA_LOG_DEBUG, "Entering Server Sleep Loop\n");

    UDA_LOG(UDA_LOG_DEBUG, "Protocol 3 Listening for Next Client Request\n");

    if ((err = protocol(server_input, protocol_id, XDR_RECEIVE, &next_protocol, logmalloclist, userdefinedtypelist,
                        nullptr, protocolVersion, full_ntree, log_struct_list, io_data, private_flags,
                        malloc_source)) != 0) {

        UDA_LOG(UDA_LOG_DEBUG, "Protocol 3 Error Listening for Wake-up %d\n", err);

        if (server_tot_block_time <= 1000 * server_timeout) {
            addIdamError(CODEERRORTYPE, "sleepServer", err, "Protocol 3 Error: Listening for Server Wake-up");
        }
        return 0;
    }

    rc = xdrrec_eof(server_input);
    UDA_LOG(UDA_LOG_DEBUG, "Next Client Request Heard\n");
    UDA_LOG(UDA_LOG_DEBUG, "Serve Awakes!\n");
    UDA_LOG(UDA_LOG_DEBUG, "Next Protocol %d Received\n", next_protocol);
    UDA_LOG(UDA_LOG_DEBUG, "XDR #G xdrrec_eof ? %d\n", rc);

#ifndef NOCHAT
    // Echo Next Protocol straight back to Client
    if ((err = protocol(server_output, protocol_id, XDR_SEND, &next_protocol, logmalloclist, userdefinedtypelist,
                        nullptr, protocolVersion, full_ntree, log_struct_list, io_data, private_flags,
                        malloc_source)) != 0) {
        addIdamError(CODEERRORTYPE, "sleepServer", err, "Protocol 3 Error Echoing Next Protocol ID");
        return 0;
    }
#endif

    if (next_protocol == PROTOCOL_CLOSEDOWN) {
        UDA_LOG(UDA_LOG_DEBUG, "Client Requests Server Shutdown\n");
        return 0;
    }

    if (next_protocol != PROTOCOL_WAKE_UP) {
        UDA_LOG(UDA_LOG_DEBUG, "Unknown Wakeup Request -> Server Shutting down\n");
        addIdamError(CODEERRORTYPE, "sleepServer", next_protocol,
                     "Unknown Wakeup Request -> Server Shutdown");
        return 0;
    }

    UDA_LOG(UDA_LOG_DEBUG, "Client Requests Server Wake-Up\n");

    return 1;    // No Time out and Non-Zero Next Protocol id => Next Client Request
}
