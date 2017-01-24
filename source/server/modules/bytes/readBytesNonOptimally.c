//! $LastChangedRevision: 353 $
//! $LastChangedDate: 2013-11-18 15:32:28 +0000 (Mon, 18 Nov 2013) $
//! $LastChangedBy: dgm $
//! $HeadURL: https://fussvn.fusion.culham.ukaea.org.uk/svnroot/IDAM/development/source/plugins/bytes/readBytesNonOptimally.c $

/*---------------------------------------------------------------
* IDAM Plugin data Reader to Access Files as a Block of Bytes without Interpretation
*
* Input Arguments:	DATA_SOURCE data_source
*			SIGNAL_DESC signal_desc
*
* Returns:		readBytes	0 if read was successful
*					otherwise an Error Code is returned
*			DATA_BLOCK	Structure with Data from the target File
*
* Calls		freeDataBlock	to free Heap memory if an Error Occurs
*
* Notes: 	All memory required to hold data is allocated dynamically
*		in heap storage. Pointers to these areas of memory are held
*		by the passed DATA_BLOCK structure. Local memory allocations
*		are freed on exit. However, the blocks reserved for data are
*		not and MUST BE FREED by the calling routine.
**
* ToDo:
*
* Change History
*
* 1.0	16October2006	D.G.Muir	Original Version
* 23Oct2007	dgm	ERRORSTACK components added
* 24Dec2008	dgm	added b for binary stream in the fopen stream type argument
* 10Oct2009	dgm	Disable this plugin if the user is external
*-----------------------------------------------------------------------------*/

#include <idamLog.h>
#include "readBytesNonOptimally.h"

#include "TrimString.h"
#include "md5Sum.h"
#include "idamErrorLog.h"
#include "freeDataBlock.h"

#ifdef NOBINARYPLUGIN

int readBytes(DATA_SOURCE data_source,
              SIGNAL_DESC signal_desc,
              DATA_BLOCK *data_block) {
    int err = 999;
    addIdamError(&idamerrorstack, CODEERRORTYPE, "readBytes", err, "PLUGIN NOT ENABLED");
    return(err);
}

#else

int readBytes(DATA_SOURCE data_source,
              SIGNAL_DESC signal_desc,
              DATA_BLOCK* data_block)
{

    int err = 0, nchar, serrno = 0;
    int offset, bufsize, nread;

    FILE* fh = NULL;
    char md5file[2 * MD5_SIZE + 1] = "";
    char md5check[2 * MD5_SIZE + 1] = "";

    char* bp = NULL;

#ifdef TIMETEST
    struct timeval tv_start[3];
    struct timeval tv_end[3];
    float testtime ;
#endif

//----------------------------------------------------------------------
// Block Access to External Users

    if (environment.external_user) {
        err = 999;
        addIdamError(&idamerrorstack, CODEERRORTYPE, "readBytes", err, "This Service is Disabled");
        idamLog(LOG_DEBUG, "readByte: Disabled Service - Requested File: %s \n", data_source.path);
        return err;
    }

//----------------------------------------------------------------------
// Test the filepath

    if (!IsLegalFilePath(data_source.path)) {
        err = 999;
        addIdamError(&idamerrorstack, CODEERRORTYPE, "readBytes", err, "The directory path has incorrect syntax");
        idamLog(LOG_DEBUG, "readByte: The directory path has incorrect syntax [%s] \n", data_source.path);
        return err;
    }

//----------------------------------------------------------------------
// Data Source Details

    err = 0;

    idamLog(LOG_DEBUG, "readByte: File Name  : %s \n", data_source.path);

//----------------------------------------------------------------------
// File Attributes

    errno = 0;

// MD5 Checksum

#ifdef TIMETEST
    rc = gettimeofday(&tv_start[0], NULL);
#endif

#ifdef MD5TEST
    strcat(cmd, data_source.path);
    errno = 0;

    ph = popen(cmd, "r")) ;			// This is EXTREMELY SLOW!!!

    serrno = errno;

    if(ph == NULL || serrno != 0) {
    err = BYTEFILEMD5ERROR;
    if(serrno != 0) addIdamError(&idamerrorstack, SYSTEMERRORTYPE, "readBytes", serrno, "");
        addIdamError(&idamerrorstack, CODEERRORTYPE, "readBytes", err, "Unable to Compute the File's MD5 Checksum");
        return err;
    }

    if(!feof(ph)) fgets(md5file,2*MD5_SIZE+1,ph);

        fclose(ph);
        md5file[2*MD5_SIZE] = '\0';
#endif

#ifdef TIMETEST
    rc = gettimeofday(&tv_end[0], NULL);
#endif

#ifdef MD5TEST
    idamLog(LOG_DEBUG, "MD5 Checksum       : %s \n", md5file);
#endif

//----------------------------------------------------------------------
// Open the File as a Binary Stream

    errno = 0;
    fh = fopen(data_source.path, "rb");

    serrno = errno;

    if (fh == NULL || ferror(fh) || serrno != 0) {
        err = BYTEFILEOPENERROR;
        if (serrno != 0) addIdamError(&idamerrorstack, SYSTEMERRORTYPE, "readBytes", serrno, "");
        addIdamError(&idamerrorstack, CODEERRORTYPE, "readBytes", err, "Unable to Open the File for Read Access");
        if (fh != NULL) fclose(fh);
        return err;
    }

//----------------------------------------------------------------------
// Error Trap Loop

    do {

// Read File (Consider using memory mapped I/O & new type to avoid heap free at end if this is too slow!)

#ifdef TIMETEST
        rc = gettimeofday(&tv_start[1], NULL);
#endif

        nchar = 0;
        offset = 0;
        bufsize = 100 * 1024;
        data_block->data_n = bufsize;    // 1 less than no. bytes read: Last Byte is an EOF

        while (!feof(fh)) {
            if ((bp = (char*) realloc(bp, data_block->data_n)) == NULL) {
                err = BYTEFILEHEAPERROR;
                addIdamError(&idamerrorstack, CODEERRORTYPE, "readBytes", err,
                             "Unable to Allocate Heap Memory for the File");
                break;
            }
            nread = (int) fread(bp + offset, sizeof(char), bufsize, fh);
            nchar = nchar + nread;
            offset = nchar;
            data_block->data_n = nchar + bufsize + 1;
        }
        if (err != 0) break;

        //nchar--; 					// Remove EOF Character from end of Byte Block
        data_block->data_n = nchar;
        data_block->data = (char*) bp;

#ifdef TIMETEST
        rc = gettimeofday(&tv_end[1], NULL);
#endif

//----------------------------------------------------------------------
// MD5 Checksum

#ifdef TIMETEST
        rc = gettimeofday(&tv_start[2], NULL);
#endif

        md5Sum((char*) bp, data_block->data_n, md5check);

        strcpy(data_block->data_desc, md5check);    // Pass back the Checksum to the Client

#ifdef TIMETEST
        rc = gettimeofday(&tv_end[2], NULL);
#endif

            idamLog(LOG_DEBUG, "File Size          : %d \n", (int) nchar);
            idamLog(LOG_DEBUG, "File Checksum      : %s \n", md5file);
            idamLog(LOG_DEBUG, "Read Checksum      : %s \n", md5check);
#ifdef MD5TEST
            idamLog(LOG_DEBUG, "Difference?        : %d \n", !strcmp(md5file,md5check));
            idamLog(LOG_DEBUG, "Last Byte == EOF   : %d \n", bp[nchar]==EOF);
#endif

// MD5 Difference?

#ifdef MD5TEST
        if(strcmp(md5file,md5check)) {
            err = BYTEFILEMD5DIFF;
            addIdamError(&idamerrorstack, CODEERRORTYPE, "readBytes", err, "MD5 Checksum Difference Found on Reading File");
            break;
        }
#endif

//----------------------------------------------------------------------
// Fetch Dimensional Data

        data_block->rank = 0;        // Scalar?
        data_block->order = -1;        // No Dimensions
        data_block->data_type = TYPE_CHAR;

    } while (0);

//----------------------------------------------------------------------
// Housekeeping

    if (err != 0) freeDataBlock(data_block);

    fclose(fh);        // Close the File

#ifdef TIMETEST
    testtime = (float)(tv_end[0].tv_sec-tv_start[0].tv_sec)*1.0E6 + (float)(tv_end[0].tv_usec - tv_start[0].tv_usec);
    idamLog(LOG_DEBUG, "Piped MD5 Checksum Timing: %.2f(micros)\n", (float)testtime);
    testtime = (float)(tv_end[1].tv_sec-tv_start[1].tv_sec)*1.0E6 + (float)(tv_end[1].tv_usec - tv_start[1].tv_usec);
    idamLog(LOG_DEBUG, "Reading File Timing: %.2f(micros)\n", (float)testtime);
    testtime = (float)(tv_end[2].tv_sec-tv_start[2].tv_sec)*1.0E6 + (float)(tv_end[2].tv_usec - tv_start[2].tv_usec);
    idamLog(LOG_DEBUG, "Buffer MD5 Checksum Timing: %.2f(micros)\n", (float)testtime);
#endif
    return err;
}

#endif
