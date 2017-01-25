/*---------------------------------------------------------------
* Server Sleeps and Waits for a Wake-up Instruction from the Client
*
* Returns: 	1 (True)  if Next Client Request to be Served
*		0 (False) if No Request within the Time Limit
*
*--------------------------------------------------------------*/
#include "sleepServer.h"

#include <idamLog.h>
#include <clientserver/idamErrorLog.h>
#include <include/idamclientserverprivate.h>
#include <clientserver/protocol.h>
#include <include/idamserver.h>

int sleepServer(void)
{

    int protocol_id, next_protocol, err, rc;

    protocol_id = PROTOCOL_NEXT_PROTOCOL;
    next_protocol = 0;
    err = 0;

    idamLog(LOG_DEBUG, "IdamServer: Entering Server Sleep Loop\n");

    idamLog(LOG_DEBUG, "IdamServer: Protocol 3 Listening for Next Client Request\n");

    if ((err = protocol(serverInput, protocol_id, XDR_RECEIVE, &next_protocol, NULL)) != 0) {

        idamLog(LOG_DEBUG, "IdamServer: Protocol 3 Error Listening for Wake-up %d\n", err);

        if (server_tot_block_time <= 1000 * server_timeout)
            addIdamError(&idamerrorstack, CODEERRORTYPE, "sleepServer", err,
                         "Protocol 3 Error: Listening for Server Wake-up");
        return 0;
    }

    rc = (int) xdrrec_eof(serverInput);
    idamLog(LOG_DEBUG, "IdamServer: Next Client Request Heard\n");
    idamLog(LOG_DEBUG, "IdamServer: Serve Awakes!\n");
    idamLog(LOG_DEBUG, "IdamServer: Next Protocol %d Received\n", next_protocol);
    idamLog(LOG_DEBUG, "IdamServer: XDR #G xdrrec_eof ? %d\n", rc);

#ifndef NOCHAT

// Echo Next Protocol straight back to Client

    if ((err = protocol(serverOutput, protocol_id, XDR_SEND, &next_protocol, NULL)) != 0) {
        addIdamError(&idamerrorstack, CODEERRORTYPE, "sleepServer", err, "Protocol 3 Error Echoing Next Protocol ID");
        return 0;
    }
#endif

    if (next_protocol == PROTOCOL_CLOSEDOWN) {
            idamLog(LOG_DEBUG, "IdamServer: Client Requests Server Shutdown\n");
        return 0;
    }

    if (next_protocol != PROTOCOL_WAKE_UP) {
            idamLog(LOG_DEBUG, "IdamServer: Unknown Wakeup Request -> Server Shutting down\n");
        addIdamError(&idamerrorstack, CODEERRORTYPE, "sleepServer", next_protocol,
                     "Unknown Wakeup Request -> Server Shutdown");
        return 0;
    }

        idamLog(LOG_DEBUG, "IdamServer: Client Requests Server Wake-Up\n");

    return 1;    // No Time out and Non-Zero Next Protocol id => Next Client Request
}
