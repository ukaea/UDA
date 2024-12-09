/*---------------------------------------------------------------
* UDA Plugin data Reader to Access Files as a Block of Bytes without Interpretation
*
* Input Arguments:    DATA_SOURCE data_source
*            SIGNAL_DESC signal_desc
*
* Returns:        readBytes    0 if read was successful
*                    otherwise an Error Code is returned
*            DATA_BLOCK    Structure with Data from the target File
*
* Calls        freeDataBlock    to free Heap memory if an Error Occurs
*
* Notes:     All memory required to hold data is allocated dynamically
*        in heap storage. Pointers to these areas of memory are held
*        by the passed DATA_BLOCK structure. Local memory allocations
*        are freed on exit. However, the blocks reserved for data are
*        not and MUST BE FREED by the calling routine.
**
* ToDo:
*
*-----------------------------------------------------------------------------*/
#include "readBytesNonOptimally.h"

#include <cerrno>
#include <cstdlib>

#include <logging/logging.h>
#include <clientserver/errorLog.h>
#include <clientserver/stringUtils.h>
#include <plugins/bytes/md5Sum.h>
#include <clientserver/udaTypes.h>
#include <clientserver/initStructs.h>

#define BYTE_FILE_DOES_NOT_EXIST      100001
#define BYTE_FILE_ATTRIBUTE_ERROR     100002
#define BYTE_FILE_IS_NOT_REGULAR      100003
#define BYTE_FILE_OPEN_ERROR          100004
#define BYTE_FILE_HEAP_ERROR          100005
#define BYTE_FILE_MD5_ERROR           100006
#define BYTE_FILE_MD5DIFF             100007
#define BYTE_FILE_READ_ERROR          100008

int readBytes(FILE* fh, DATA_BLOCK* data_block, int offset, int max_bytes, const std::string& checksum)
{
    int err = 0;

    // Read File (Consider using memory mapped I/O & new type to avoid heap free at end if this is too slow!)

    int nchar = 0;
    int buf_offset = 0;
    int buf_size = 1024 * 1024;
    if (max_bytes > 0 && max_bytes < buf_size) {
        buf_size = max_bytes;
    }

    data_block->data_n = buf_size;    // 1 less than no. bytes read: Last Byte is an EOF

    if (offset >= 0) {
        fseek(fh, offset, SEEK_SET);
    }

    char* bp = nullptr;
    while (!feof(fh)) {
        char* newp = (char*)realloc(bp, (size_t)data_block->data_n);
        if (newp == nullptr) {
            free(bp);
            err = BYTE_FILE_HEAP_ERROR;
            addIdamError(UDA_CODE_ERROR_TYPE, "readBytes", err, "Unable to Allocate Heap Memory for the File");
            break;
        }
        bp = newp;

        errno = 0;
        int n_read = (int)fread(bp + buf_offset, sizeof(char), (size_t)buf_size, fh);

        int serrno = errno;
        if (n_read == 0 || serrno != 0) {
            int err = BYTE_FILE_READ_ERROR;
            if (serrno != 0) {
                addIdamError(UDA_SYSTEM_ERROR_TYPE, "readBytes", serrno, "");
            }
            addIdamError(UDA_CODE_ERROR_TYPE, "readBytes", err, "Unable to Open the File for Read Access");
        }

        nchar += n_read;
        if (max_bytes > 0 && nchar >= max_bytes) {
            break;
        }
        buf_offset = nchar;
        data_block->data_n = nchar + buf_size + 1;
    }

    if (err != 0) {
        return err;
    }

    //nchar--;                     // Remove EOF Character from end of Byte Block
    data_block->data_n = nchar;
    data_block->data = bp;

    if (checksum == "md5") {
        //----------------------------------------------------------------------
        // MD5 Checksum

        char md5check[MD5_SIZE];
        md5Sum(bp, data_block->data_n, md5check);
        strcpy(data_block->data_desc, md5check);    // Pass back the Checksum to the Client

        UDA_LOG(UDA_LOG_DEBUG, "File Size          : %d \n", nchar);
        UDA_LOG(UDA_LOG_DEBUG, "Read Checksum      : %s \n", md5check);
    }

    //----------------------------------------------------------------------
    // Fetch Dimensional Data

    data_block->rank = 1;
    data_block->dims = (DIMS*)malloc(sizeof(DIMS));
    initDimBlock(data_block->dims);

    data_block->dims[0].data_type = UDA_TYPE_UNSIGNED_INT;
    data_block->dims[0].dim_n = data_block->data_n;
    data_block->dims[0].compressed = 1;
    data_block->dims[0].dim0 = 0.0;
    data_block->dims[0].diff = 1.0;
    data_block->dims[0].method = 0;

    data_block->order = -1;        // No Dimensions
    data_block->data_type = UDA_TYPE_CHAR;

    return err;
}
