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
*-----------------------------------------------------------------------------*/
#include "readBytesNonOptimally.h"

#include <errno.h>
#include <stdlib.h>

#include <logging/logging.h>
#include <clientserver/errorLog.h>
#include <clientserver/stringUtils.h>
#include <clientserver/md5Sum.h>
#include <clientserver/freeDataBlock.h>
#include <clientserver/udaTypes.h>
#include <clientserver/initStructs.h>
#include <server/getServerEnvironment.h>

#ifdef NOBINARYPLUGIN

int readBytes(DATA_SOURCE data_source, SIGNAL_DESC signal_desc, DATA_BLOCK *data_block)
{
    int err = 999;
    addIdamError(CODEERRORTYPE, "readBytes", err, "PLUGIN NOT ENABLED");
    return err;
}

#else

int readBytes(DATA_SOURCE data_source, SIGNAL_DESC signal_desc, DATA_BLOCK* data_block)
{
    int err = 0;

    char md5file[2 * MD5_SIZE + 1] = "";
    char md5check[2 * MD5_SIZE + 1] = "";

    //----------------------------------------------------------------------
    // Block Access to External Users

    const ENVIRONMENT* environment = getIdamServerEnvironment();

    if (environment->external_user) {
        err = 999;
        addIdamError(CODEERRORTYPE, "readBytes", err, "This Service is Disabled");
        IDAM_LOGF(UDA_LOG_DEBUG, "Disabled Service - Requested File: %s \n", data_source.path);
        return err;
    }

    //----------------------------------------------------------------------
    // Test the filepath

    if (!IsLegalFilePath(data_source.path)) {
        err = 999;
        addIdamError(CODEERRORTYPE, "readBytes", err, "The directory path has incorrect syntax");
        IDAM_LOGF(UDA_LOG_DEBUG, "The directory path has incorrect syntax [%s] \n", data_source.path);
        return err;
    }

    //----------------------------------------------------------------------
    // Data Source Details

    err = 0;

    IDAM_LOGF(UDA_LOG_DEBUG, "File Name  : %s \n", data_source.path);

    //----------------------------------------------------------------------
    // File Attributes

    errno = 0;

    //----------------------------------------------------------------------
    // Open the File as a Binary Stream

    errno = 0;
    FILE* fh = fopen(data_source.path, "rb");

    int serrno = errno;

    if (fh == NULL || ferror(fh) || serrno != 0) {
        err = BYTEFILEOPENERROR;
        if (serrno != 0) {
            addIdamError(SYSTEMERRORTYPE, "readBytes", serrno, "");
        }
        addIdamError(CODEERRORTYPE, "readBytes", err, "Unable to Open the File for Read Access");
        if (fh != NULL) {
            fclose(fh);
        }
        return err;
    }

    //----------------------------------------------------------------------
    // Error Trap Loop

    do {

        // Read File (Consider using memory mapped I/O & new type to avoid heap free at end if this is too slow!)

        int nchar = 0;
        int offset = 0;
        int bufsize = 100 * 1024;
        data_block->data_n = bufsize;    // 1 less than no. bytes read: Last Byte is an EOF

        char* bp = NULL;
        while (!feof(fh)) {
            if ((bp = (char*)realloc(bp, (size_t)data_block->data_n)) == NULL) {
                err = BYTEFILEHEAPERROR;
                addIdamError(CODEERRORTYPE, "readBytes", err, "Unable to Allocate Heap Memory for the File");
                break;
            }
            int nread = (int)fread(bp + offset, sizeof(char), (size_t)bufsize, fh);
            nchar = nchar + nread;
            offset = nchar;
            data_block->data_n = nchar + bufsize + 1;
        }

        if (err != 0) {
            break;
        }

        //nchar--; 					// Remove EOF Character from end of Byte Block
        data_block->data_n = nchar;
        data_block->data = bp;

        //----------------------------------------------------------------------
        // MD5 Checksum

        md5Sum(bp, data_block->data_n, md5check);

        strcpy(data_block->data_desc, md5check);    // Pass back the Checksum to the Client

        IDAM_LOGF(UDA_LOG_DEBUG, "File Size          : %d \n", nchar);
        IDAM_LOGF(UDA_LOG_DEBUG, "File Checksum      : %s \n", md5file);
        IDAM_LOGF(UDA_LOG_DEBUG, "Read Checksum      : %s \n", md5check);

        // MD5 Difference?

        //----------------------------------------------------------------------
        // Fetch Dimensional Data

        data_block->rank = 1;
        data_block->dims = malloc(sizeof(DIMS));
        initDimBlock(data_block->dims);

        data_block->dims[0].data_type = UDA_TYPE_UNSIGNED_INT;
        data_block->dims[0].dim_n = data_block->data_n;
        data_block->dims[0].compressed = 1;
        data_block->dims[0].dim0 = 0.0;
        data_block->dims[0].diff = 1.0;
        data_block->dims[0].method = 0;

        data_block->order = -1;        // No Dimensions
        data_block->data_type = UDA_TYPE_CHAR;

    } while (0);

    //----------------------------------------------------------------------
    // Housekeeping

    if (err != 0) {
        freeDataBlock(data_block);
    }

    fclose(fh); // Close the File

    return err;
}

#endif
