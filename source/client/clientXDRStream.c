// Create the Client Side XDR File Stream
//
//----------------------------------------------------------------
#include "clientXDRStream.h"

#include <stdio.h>
#include <rpc/rpc.h>

#include <logging/logging.h>
#include <clientserver/udaDefines.h>

#include "connection.h"

#if defined(SSLAUTHENTICATION) && !defined(FATCLIENT)
#  include <authentication/udaSSL.h>
#endif

static XDR clientXDRinput;
static XDR clientXDRoutput;

XDR* clientInput = &clientXDRinput;
XDR* clientOutput = &clientXDRoutput;

void idamCreateXDRStream()
{
    clientOutput->x_ops = NULL;
    clientInput->x_ops = NULL;

    UDA_LOG(UDA_LOG_DEBUG, "Creating XDR Streams \n");

#if defined(SSLAUTHENTICATION) && !defined(FATCLIENT)

    if(getUdaClientSSLDisabled()){
    
#ifdef __APPLE__
       xdrrec_create(clientOutput, DB_READ_BLOCK_SIZE, DB_WRITE_BLOCK_SIZE, NULL,
                     (int (*)(void*, void*, int))clientReadin,
                     (int (*)(void*, void*, int))clientWriteout);

       xdrrec_create(clientInput, DB_READ_BLOCK_SIZE, DB_WRITE_BLOCK_SIZE, NULL,
                     (int (*)(void*, void*, int))clientReadin,
                     (int (*)(void*, void*, int))clientWriteout);
#else
       xdrrec_create(clientOutput, DB_READ_BLOCK_SIZE, DB_WRITE_BLOCK_SIZE, NULL,
                     (int (*)(char*, char*, int)) clientReadin,
                     (int (*)(char*, char*, int)) clientWriteout);

       xdrrec_create(clientInput, DB_READ_BLOCK_SIZE, DB_WRITE_BLOCK_SIZE, NULL,
                     (int (*)(char*, char*, int)) clientReadin,
                     (int (*)(char*, char*, int)) clientWriteout);
#endif    
    } else {
#ifdef __APPLE__
       xdrrec_create(clientOutput, DB_READ_BLOCK_SIZE, DB_WRITE_BLOCK_SIZE, NULL,
                     (int (*)(void*, void*, int))readUdaClientSSL,
                     (int (*)(void*, void*, int))writeUdaClientSSL);

       xdrrec_create(clientInput, DB_READ_BLOCK_SIZE, DB_WRITE_BLOCK_SIZE, NULL,
                     (int (*)(void*, void*, int))readUdaClientSSL,
                     (int (*)(void*, void*, int))writeUdaClientSSL);
#else
       xdrrec_create(clientOutput, DB_READ_BLOCK_SIZE, DB_WRITE_BLOCK_SIZE, NULL,
                     (int (*)(char*, char*, int)) readUdaClientSSL,
                     (int (*)(char*, char*, int)) writeUdaClientSSL);

       xdrrec_create(clientInput, DB_READ_BLOCK_SIZE, DB_WRITE_BLOCK_SIZE, NULL,
                     (int (*)(char*, char*, int)) readUdaClientSSL,
                     (int (*)(char*, char*, int)) writeUdaClientSSL);
#endif
    }
#else

#ifdef __APPLE__
    xdrrec_create(clientOutput, DB_READ_BLOCK_SIZE, DB_WRITE_BLOCK_SIZE, NULL,
                  (int (*)(void*, void*, int))clientReadin,
                  (int (*)(void*, void*, int))clientWriteout);

    xdrrec_create(clientInput, DB_READ_BLOCK_SIZE, DB_WRITE_BLOCK_SIZE, NULL,
                  (int (*)(void*, void*, int))clientReadin,
                  (int (*)(void*, void*, int))clientWriteout);
#else
    xdrrec_create(clientOutput, DB_READ_BLOCK_SIZE, DB_WRITE_BLOCK_SIZE, NULL,
                  (int (*)(char*, char*, int)) clientReadin,
                  (int (*)(char*, char*, int)) clientWriteout);

    xdrrec_create(clientInput, DB_READ_BLOCK_SIZE, DB_WRITE_BLOCK_SIZE, NULL,
                  (int (*)(char*, char*, int)) clientReadin,
                  (int (*)(char*, char*, int)) clientWriteout);
#endif

#endif   // SSLAUTHENTICATION

    clientInput->x_op = XDR_DECODE;
    clientOutput->x_op = XDR_ENCODE;
}
