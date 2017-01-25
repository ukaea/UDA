// Create the Server Side XDR File Stream
//
//----------------------------------------------------------------
#include "CreateXDRStream.h"

#include <include/idamserver.h>
#include "writer.h"

void CreateXDRStream() {
    serverOutput->x_ops  = NULL;
    serverInput->x_ops   = NULL;

#ifdef __APPLE__
    xdrrec_create( serverOutput, DB_READ_BLOCK_SIZE, DB_WRITE_BLOCK_SIZE, NULL,
                   (int (*) (void *, void *, int))Readin,
                   (int (*) (void *, void *, int))Writeout);

    xdrrec_create( serverInput, DB_READ_BLOCK_SIZE, DB_WRITE_BLOCK_SIZE, NULL,
                   (int (*) (void *, void *, int))Readin,
                   (int (*) (void *, void *, int))Writeout);
#else
    xdrrec_create( serverOutput, DB_READ_BLOCK_SIZE, DB_WRITE_BLOCK_SIZE, NULL,
                   (int (*) (char *, char *, int))Readin,
                   (int (*) (char *, char *, int))Writeout);

    xdrrec_create( serverInput, DB_READ_BLOCK_SIZE, DB_WRITE_BLOCK_SIZE, NULL,
                   (int (*) (char *, char *, int))Readin,
                   (int (*) (char *, char *, int))Writeout);
#endif

    serverInput->x_op   = XDR_DECODE;
    serverOutput->x_op  = XDR_ENCODE;
}
