//! $LastChangedRevision: 121 $
//! $LastChangedDate: 2009-11-09 10:21:09 +0000 (Mon, 09 Nov 2009) $
//! $LastChangedBy: dgm $
//! $HeadURL: https://fussvn.fusion.culham.ukaea.org.uk/svnroot/IDAM/development/source/plugins/idam/readXDRFile.c $

/*---------------------------------------------------------------
* IDAM XDR File Read/Writer
*
* Input Arguments:
*
*
* Returns:		 0 if read was successful
*			 otherwise an Error Code is returned
*
*
* Calls
*
* Notes:
*
* ToDo:
*
* Change History
*
* 26Apr2010	D.G.Muir	Original Version

*-----------------------------------------------------------------------------*/

#include "readXDRFile.h"

#include <stdlib.h>
#include <errno.h>
#include <logging/idamLog.h>
#include <include/idamclientserverprivate.h>

#include "idamErrorLog.h"

#ifdef SERVERBUILD
#  include "idamServerStartup.h"
#endif

#define MAXDOLOOPLIMIT 500            // ~50MB file

int sendXDRFile(XDR* xdrs, char* xdrfile)
{

    int err = 0, rc = 1, nchar, bufsize, count;
    FILE* fh;
    char* bp = NULL;

//----------------------------------------------------------------------
// Open the File as a Binary Stream

    errno = 0;
    fh = fopen(xdrfile, "rb");

    if (fh == NULL || errno != 0 || ferror(fh)) {
        err = 999;
        if (errno != 0) addIdamError(&idamerrorstack, SYSTEMERRORTYPE, "sendXDRFile", errno, "");
        addIdamError(&idamerrorstack, CODEERRORTYPE, "sendXDRFile", err, "Unable to Open the XDR File for Read Access");
        if (fh != NULL) fclose(fh);
        return err;
    }

    idamLog(LOG_DEBUG, "sendXDRFile: reading temporary XDR file %s\n", xdrfile);

//----------------------------------------------------------------------
// Error Trap Loop

    do {

//----------------------------------------------------------------------
// Read File and write to xdr data stream

        nchar = 0;
        bufsize = 100 * 1024;
        rc = 1;
        count = 0;

        if ((bp = (char*) malloc(bufsize * sizeof(char))) == NULL) {
            err = 999;
            addIdamError(&idamerrorstack, CODEERRORTYPE, "sendXDRFile", err,
                         "Unable to Allocate Heap Memory for the XDR File");
            bufsize = 0;
            rc = xdr_int(xdrs, &bufsize);
            break;
        }

        rc = xdr_int(xdrs, &bufsize);    // Send Server buffer size, e.g., 100k bytes

        idamLog(LOG_DEBUG, "sendXDRFile: Buffer size %d\n", bufsize);

        while (!feof(fh)) {
            nchar = (int) fread(bp, sizeof(char), bufsize, fh);
            rc = rc && xdr_int(xdrs, &nchar);                // Number of Bytes to send

            idamLog(LOG_DEBUG, "sendXDRFile: File block size %d\n", nchar);

            if (nchar > 0) {        // Send the bytes
                rc = rc && xdr_vector(xdrs, (char*) bp, nchar, sizeof(char), (xdrproc_t) xdr_char);
            }

            rc = rc && xdrrec_endofrecord(xdrs, 1);

            count = count + nchar;
        }

        idamLog(LOG_DEBUG, "sendXDRFile: Total File size %d\n", count);

    } while (0);

    nchar = 0;                    // No more bytes to send
    rc = rc && xdr_int(xdrs, &nchar);
    rc = rc && xdrrec_endofrecord(xdrs, 1);

// *** Send count to client as a check all data received
// *** Send hash sum to client as a test data is accurate - another reason to use files and cache rather than a data stream


//----------------------------------------------------------------------
// Housekeeping

    fclose(fh);        // Close the File
    free((void*) bp);

    return err;
}


int receiveXDRFile(XDR* xdrs, char* xdrfile)
{

    int err = 0, rc = 1, nchar, bufsize, count, doLoopLimit = 0;
    FILE* fh;
    char* bp = NULL;

//----------------------------------------------------------------------
// Open the File as a Binary Stream

    errno = 0;
    fh = fopen(xdrfile, "wb");

    if (fh == NULL || errno != 0 || ferror(fh)) {
        err = 999;
        if (errno != 0) addIdamError(&idamerrorstack, SYSTEMERRORTYPE, "receiveXDRFile", errno, "");
        addIdamError(&idamerrorstack, CODEERRORTYPE, "receiveXDRFile", err,
                     "Unable to Open the XDR File for Write Access");
        if (fh != NULL) fclose(fh);
        return err;
    }

    idamLog(LOG_DEBUG, "receiveXDRFile: writing temporary XDR file %s\n", xdrfile);

//----------------------------------------------------------------------
// Error Trap Loop

    do {

//----------------------------------------------------------------------
// Read xdr data stream and write file

        nchar = 0;

        rc = xdrrec_skiprecord(xdrs);
        rc = xdr_int(xdrs, &bufsize);    // Server buffer size, e.g., 100k bytes

        idamLog(LOG_DEBUG, "receiveXDRFile: Buffer size %d\n", bufsize);

        if (bufsize <= 0 || bufsize > 100 * 1024) {
            err = 999;
            addIdamError(&idamerrorstack, CODEERRORTYPE, "receiveXDRFile", err, "Zero buffer size: Server failure");
            break;
        }

        if ((bp = (char*) malloc(bufsize * sizeof(char))) == NULL) {
            err = 999;
            addIdamError(&idamerrorstack, CODEERRORTYPE, "receiveXDRFile", err,
                         "Unable to Allocate Heap Memory for the XDR File");
            break;
        }

        count = 0;

        do {
            errno = 0;

            if (doLoopLimit > 0) rc = rc && xdrrec_skiprecord(xdrs);

            rc = rc && xdr_int(xdrs, &nchar);                // How many bytes to receive?

            idamLog(LOG_DEBUG, "receiveXDRFile: [%d] File block size %d\n", doLoopLimit, nchar);

            if (nchar > bufsize) {
                err = 999;
                addIdamError(&idamerrorstack, CODEERRORTYPE, "receiveXDRFile", err,
                             "File block size inconsistent with buffer size");
                break;
            }

            if (nchar > 0) {
                rc = rc && xdr_vector(xdrs, (char*) bp, nchar, sizeof(char), (xdrproc_t) xdr_char); //Bytes
                count = count + (int) fwrite(bp, sizeof(char), nchar, fh);
            }
        } while (nchar > 0 && errno == 0 && doLoopLimit++ < MAXDOLOOPLIMIT);

        if (doLoopLimit >= MAXDOLOOPLIMIT) {
            err = 999;
            addIdamError(&idamerrorstack, CODEERRORTYPE, "receiveXDRFile", err,
                         "Maximum XDR file size reached: ~50MBytes");
            break;
        }

// *** Read count from server to check all data received
// *** Read hash sum from server to test data is accurate - another reason to use files and cache rather than a data stream

        idamLog(LOG_DEBUG, "receiveXDRFile: Total File size %d\n", count);

        if (errno != 0) {
            err = 999;
            addIdamError(&idamerrorstack, SYSTEMERRORTYPE, "receiveXDRFile", errno, "Problem receiving XDR File");
            break;
        }

    } while (0);

//----------------------------------------------------------------------
// Housekeeping

    fclose(fh);        // Close the File
    free((void*) bp);

    return err;
}

