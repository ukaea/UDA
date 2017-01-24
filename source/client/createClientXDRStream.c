//! $LastChangedRevision: 353 $
//! $LastChangedDate: 2013-11-18 15:32:28 +0000 (Mon, 18 Nov 2013) $
//! $LastChangedBy: dgm $
//! $HeadURL: https://fussvn.fusion.culham.ukaea.org.uk/svnroot/IDAM/development/source/client/createClientXDRStream.c $

// Create the Client Side XDR File Stream
//
// Change History
//
// 21Mar2007	dgm	prefix client added to XDR declarations
// 18Apr2007	dgm	arg 5 of *_create  functions modified: char * changed to void *
// 08Jul2009	dgm	clientInput,clientOutput changed to static class
//----------------------------------------------------------------

#include <idamLog.h>
#include "createClientXDRStream.h"

#include "idamclientserver.h"
#include "idamclient.h"
#include "Readin.h"
#include "Writeout.h"

static XDR clientXDRinput;
static XDR clientXDRoutput;

XDR* clientInput = &clientXDRinput;
XDR* clientOutput = &clientXDRoutput;

void idamCreateXDRStream()
{
    clientOutput->x_ops = NULL;
    clientInput->x_ops = NULL;

    idamLog(LOG_DEBUG, "IdamAPI: Creating XDR Streams \n");

#ifdef __APPLE__
    xdrrec_create( clientOutput, DB_READ_BLOCK_SIZE, DB_WRITE_BLOCK_SIZE, NULL,
                   (int (*) (void *, void *, int))idamClientReadin,
                   (int (*) (void *, void *, int))idamClientWriteout);

    xdrrec_create( clientInput, DB_READ_BLOCK_SIZE, DB_WRITE_BLOCK_SIZE, NULL,
                   (int (*) (void *, void *, int))idamClientReadin,
                   (int (*) (void *, void *, int))idamClientWriteout);
#else
    xdrrec_create(clientOutput, DB_READ_BLOCK_SIZE, DB_WRITE_BLOCK_SIZE, NULL,
                  (int (*)(char*, char*, int)) idamClientReadin,
                  (int (*)(char*, char*, int)) idamClientWriteout);

    xdrrec_create(clientInput, DB_READ_BLOCK_SIZE, DB_WRITE_BLOCK_SIZE, NULL,
                  (int (*)(char*, char*, int)) idamClientReadin,
                  (int (*)(char*, char*, int)) idamClientWriteout);
#endif

    clientInput->x_op = XDR_DECODE;
    clientOutput->x_op = XDR_ENCODE;
}
