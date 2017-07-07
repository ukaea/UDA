/*---------------------------------------------------------------
* IDAM Plugin data Reader to Access DATA from IDA Files
*
* Input Arguments:	DATA_SOURCE data_source
*			SIGNAL_DESC signal_desc
*
* Returns:		readIDA		0 if read was successful
*					otherwise a Error Code is returned 
*			DATA_BLOCK	Structure with Data from the IDA File 
*
* Calls		freeDataBlock	to free Heap memory if an Error Occurs
*
* Notes: 	All memory required to hold data is allocated dynamically
*		in heap storage. Pointers to these areas of memory are held
*		by the passed DATA_BLOCK structure. Local memory allocations
*		are freed on exit. However, the blocks reserved for data are
*		not and MUST BE FREED by the calling routine.	
*
* 		A VALGRIND Test on an old IDA file generated some errors in low level IDA library components.
*		These may be due to legacy problems. The test was for shot 8323 and signals XNB_SS_BEAM_CURRENT
*		and XNB_SS_BEAM_VOLTAGE.	 
*
* ToDo:		                     
*
*-----------------------------------------------------------------------------*/
#include "readIda.h"

#include <ida3.h>

#include <strings.h>

#include <clientserver/udaErrors.h>
#include <clientserver/stringUtils.h>
#include <clientserver/errorLog.h>
#include <clientserver/printStructs.h>
#include <server/mastArchiveFilePath.h>
#include <logging/logging.h>

#include "nameIda.h"
#include "readIdaItem.h"

int readIda3(DATA_BLOCK* data_block, int exp_number, int pass, const char* source_alias, const char* signal_name,
                   const char* filename, const char* path, char type)
{
    int err = 0;

    //----------------------------------------------------------------------
    // Data Source Details

    char ida_file[IDA_FSIZE + 1] = "";
    char ida_path[STRING_LENGTH] = "";

    if (exp_number > 0) {

        char* alias;

        if (strlen(source_alias) == 0) {
            alias = strdup(signal_name);
        } else {
            alias = strdup(source_alias);
        }

        TrimString(alias);
        strlwr(alias);

        char* file = NULL;

        if (filename != NULL) {
            file = strdup(filename);
            TrimString(file);
            strlwr(file);
        }

        IDAM_LOGF(UDA_LOG_DEBUG, "alias          : %s \n", alias);
        IDAM_LOGF(UDA_LOG_DEBUG, "filename       : %s \n", file);
        IDAM_LOGF(UDA_LOG_DEBUG, "length         : %d \n", strlen(alias));
        IDAM_LOGF(UDA_LOG_DEBUG, "alias == file? : %d \n", strcasecmp(file, alias));

        // Check whether or not the filename is the alias name
        // If is it then form the correct filename

        if (file == NULL || STR_IEQUALS(file, alias)) {
            nameIDA(alias, exp_number, ida_file);
        } else {
            strncpy(ida_file, filename, IDA_FSIZE + 1);
            ida_file[IDA_FSIZE] = '\0';
        }

        // Check whether or not a Path has been specified

        if (path == NULL || strlen(path) == 0) {
            if (type == 'R') {
                mastArchiveFilePath(exp_number, -1, ida_file, ida_path);    // Always Latest
            } else {
                mastArchiveFilePath(exp_number, pass, ida_file, ida_path);
            }
        } else {                        // User Specified
            strncpy(ida_path, path, STRING_LENGTH);
            ida_path[STRING_LENGTH - 1] = '\0';
        }

        IDAM_LOGF(UDA_LOG_DEBUG, "Signal Name  : %s \n", signal_name);
        IDAM_LOGF(UDA_LOG_DEBUG, "File Alias   : %s \n", source_alias);
        IDAM_LOGF(UDA_LOG_DEBUG, "File Name    : %s \n", ida_file);
        IDAM_LOGF(UDA_LOG_DEBUG, "File Path    : %s \n", ida_path);
        IDAM_LOGF(UDA_LOG_DEBUG, "Pulse Number : %d \n", exp_number);
        IDAM_LOGF(UDA_LOG_DEBUG, "Pass Number  : %d \n", pass);

    } else {
        strcpy(ida_path, path);        //Fully Specified

        IDAM_LOGF(UDA_LOG_DEBUG, "Signal Name  : %s \n", signal_name);
        IDAM_LOGF(UDA_LOG_DEBUG, "File Name    : %s \n", ida_path);
    }

    //----------------------------------------------------------------------
    // Test String lengths are Compliant

    if (strlen(filename) <= IDA_FSIZE + 1 || exp_number < 0) {
        strcpy(ida_file, filename);
    } else {
        err = IDA_CLIENT_FILE_NAME_TOO_LONG;
        addIdamError(&idamerrorstack, CODEERRORTYPE, __func__, err, "IDA Filename Length is too Long");
        return err;
    }

    char ida_signal[IDA_LSIZE + 1] = "";

    if (strlen(signal_name) <= IDA_LSIZE + 1) {
        strcpy(ida_signal, signal_name);
    } else {
        err = IDA_CLIENT_SIGNAL_NAME_TOO_LONG;
        addIdamError(&idamerrorstack, CODEERRORTYPE, __func__, err, "IDA Signalname Length is too Long");
        return err;
    }

    //----------------------------------------------------------------------
    // Is the IDA File Already open for Reading? If Not then Open

    IDAM_LOGF(UDA_LOG_DEBUG, "IDA file: (%s)\n", ida_path);

    errno = 0;
    char ida_errmsg[256] = "";

    ida_file_ptr* ida_file_id = ida_open(ida_path, IDA_READ, NULL);
    int serrno = errno;
    if (ida_file_id == NULL || errno != 0) {
        err = IDA_ERROR_OPENING_FILE;
        if (serrno != 0) {
            addIdamError(&idamerrorstack, SYSTEMERRORTYPE, __func__, serrno, "");
        }
        ida_error_mess(ida_error(ida_file_id), ida_errmsg);
        addIdamError(&idamerrorstack, CODEERRORTYPE, __func__, err, ida_errmsg);
        return err;
    }

    //----------------------------------------------------------------------
    // Fetch the Data

    IDAM_LOG(UDA_LOG_DEBUG, "Calling readIdaItem\n");

    short context = (short)0;

    if ((err = readIdaItem(ida_signal, ida_file_id, &context, data_block)) != 0) {
        err = IDA_ERROR_READING_DATA;
        addIdamError(&idamerrorstack, CODEERRORTYPE, __func__, err, "Unable to Read IDA Data Item");
        return err;
    }

    IDAM_LOG(UDA_LOG_DEBUG, "Returned from readIdaItem\n");

    printDataBlock(*data_block);

    //----------------------------------------------------------------------
    // Housekeeping

    // Close IDA File

    int rc = (int)ida_close(ida_file_id);

    if (rc != 0) {
        ida_error_mess(ida_error(ida_file_id), ida_errmsg);
        addIdamError(&idamerrorstack, CODEERRORTYPE, __func__, 1, "Problem Closing IDA File");
        addIdamError(&idamerrorstack, CODEERRORTYPE, __func__, 1, ida_errmsg);
    }

    return err;
}

