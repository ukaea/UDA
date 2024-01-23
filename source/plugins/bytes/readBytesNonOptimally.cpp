#include "readBytesNonOptimally.h"

#include <cerrno>
#include <cstdlib>

#include "initStructs.h"
#include "udaTypes.h"
#include <clientserver/errorLog.h>
#include <clientserver/stringUtils.h>
#include <logging/logging.h>
#include <plugins/bytes/md5Sum.h>

#define BYTEFILEDOESNOTEXIST 100001
#define BYTEFILEATTRIBUTEERROR 100002
#define BYTEFILEISNOTREGULAR 100003
#define BYTEFILEOPENERROR 100004
#define BYTEFILEHEAPERROR 100005
#define BYTEFILEMD5ERROR 100006
#define BYTEFILEMD5DIFF 100007

int readBytes(const std::string& path, IDAM_PLUGIN_INTERFACE* plugin_interface)
{
    int err = 0;

    char md5file[2 * MD5_SIZE + 1] = "";
    char md5check[2 * MD5_SIZE + 1] = "";

    const ENVIRONMENT* environment = plugin_interface->environment;
    DATA_BLOCK* data_block = plugin_interface->data_block;

    //----------------------------------------------------------------------
    // Block Access to External Users

    if (environment->external_user) {
        err = 999;
        udaAddError(UDA_CODE_ERROR_TYPE, "readBytes", err, "This Service is Disabled");
        UDA_LOG(UDA_LOG_DEBUG, "Disabled Service - Requested File: %s \n", path.c_str());
        return err;
    }

    //----------------------------------------------------------------------
    // Test the filepath

    if (!IsLegalFilePath(path.c_str())) {
        err = 999;
        udaAddError(UDA_CODE_ERROR_TYPE, "readBytes", err, "The directory path has incorrect syntax");
        UDA_LOG(UDA_LOG_DEBUG, "The directory path has incorrect syntax [%s] \n", path.c_str());
        return err;
    }

    //----------------------------------------------------------------------
    // Data Source Details

    err = 0;

    UDA_LOG(UDA_LOG_DEBUG, "File Name  : %s \n", path.c_str());

    //----------------------------------------------------------------------
    // File Attributes

    errno = 0;

    //----------------------------------------------------------------------
    // Open the File as a Binary Stream

    errno = 0;
    FILE* fh = fopen(path.c_str(), "rb");

    int serrno = errno;

    if (fh == nullptr || ferror(fh) || serrno != 0) {
        err = BYTEFILEOPENERROR;
        if (serrno != 0) {
            udaAddError(UDA_SYSTEM_ERROR_TYPE, "readBytes", serrno, "");
        }
        udaAddError(UDA_CODE_ERROR_TYPE, "readBytes", err, "Unable to Open the File for Read Access");
        if (fh != nullptr) {
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
        data_block->data_n = bufsize; // 1 less than no. bytes read: Last Byte is an EOF

        char* bp = nullptr;
        while (!feof(fh)) {
            if ((bp = (char*)realloc(bp, (size_t)data_block->data_n)) == nullptr) {
                err = BYTEFILEHEAPERROR;
                udaAddError(UDA_CODE_ERROR_TYPE, "readBytes", err, "Unable to Allocate Heap Memory for the File");
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

        // nchar--;                     // Remove EOF Character from end of Byte Block
        data_block->data_n = nchar;
        data_block->data = bp;

        //----------------------------------------------------------------------
        // MD5 Checksum

        md5Sum(bp, data_block->data_n, md5check);

        strcpy(data_block->data_desc, md5check); // Pass back the Checksum to the Client

        UDA_LOG(UDA_LOG_DEBUG, "File Size          : %d \n", nchar);
        UDA_LOG(UDA_LOG_DEBUG, "File Checksum      : %s \n", md5file);
        UDA_LOG(UDA_LOG_DEBUG, "Read Checksum      : %s \n", md5check);

        // MD5 Difference?

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

        data_block->order = -1; // No Dimensions
        data_block->data_type = UDA_TYPE_CHAR;

    } while (0);

    //----------------------------------------------------------------------
    // Housekeeping

    //    if (err != 0) {
    //        freeDataBlock(data_block);
    //    }

    fclose(fh); // Close the File

    return err;
}
