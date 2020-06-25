// Create the Client Side XDR File Stream
//
//----------------------------------------------------------------
#include "createClientXDRStream.h"

#include <stdio.h>
#include <rpc/rpc.h>

#include <logging/logging.h>
#include <clientserver/udaDefines.h>

#include "readin.h"
#include "writeout.h"

static XDR clientXDRinput;
static XDR clientXDRoutput;

XDR* clientInput = &clientXDRinput;
XDR* clientOutput = &clientXDRoutput;

void idamCreateXDRStream()
{
    clientOutput->x_ops = nullptr;
    clientInput->x_ops = nullptr;

    IDAM_LOG(UDA_LOG_DEBUG, "IdamAPI: Creating XDR Streams \n");

#if defined (__APPLE__) || defined(__TIRPC__)
    xdrrec_create( clientOutput, DB_READ_BLOCK_SIZE, DB_WRITE_BLOCK_SIZE, nullptr,
                   (int (*) (void *, void *, int))idamClientReadin,
                   (int (*) (void *, void *, int))idamClientWriteout);

    xdrrec_create( clientInput, DB_READ_BLOCK_SIZE, DB_WRITE_BLOCK_SIZE, nullptr,
                   (int (*) (void *, void *, int))idamClientReadin,
                   (int (*) (void *, void *, int))idamClientWriteout);
#else
    xdrrec_create(clientOutput, DB_READ_BLOCK_SIZE, DB_WRITE_BLOCK_SIZE, nullptr,
                  (int (*)(char*, char*, int)) idamClientReadin,
                  (int (*)(char*, char*, int)) idamClientWriteout);

    xdrrec_create(clientInput, DB_READ_BLOCK_SIZE, DB_WRITE_BLOCK_SIZE, nullptr,
                  (int (*)(char*, char*, int)) idamClientReadin,
                  (int (*)(char*, char*, int)) idamClientWriteout);
#endif

    clientInput->x_op = XDR_DECODE;
    clientOutput->x_op = XDR_ENCODE;
}
