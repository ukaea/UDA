/*!
  IDAM Plugin to Dump the Signal Details from a File using proprietory tools, e.g. ncdump
*/
/*---------------------------------------------------------------
*
* Input Arguments:	REQUEST_BLOCK request_block
*
* Returns:		dumpFile		0 if read was successful
*						otherwise a Error Code is returned
*			DATA_BLOCK		Structure with Data from the Data File
*
* Calls		freeDataBlock	to free Heap memory if an Error Occurs
*
* Notes: 	All memory required to hold data is allocated dynamically
*		in heap storage. Pointers to these areas of memory are held
*		by the passed DATA_BLOCK structure. Local memory allocations
*		are freed on exit. However, the blocks reserved for data are
*		not and MUST BE FREED by the calling routine.
*
* ToDo:
*
*-----------------------------------------------------------------------------*/
#include "dumpFile.h"

#include <stdlib.h>
#include <errno.h>
#include <strings.h>

#include <logging/logging.h>
#include <clientserver/udaTypes.h>
#include <clientserver/stringUtils.h>
#include <modules/ida/nameIda.h>
#include <clientserver/errorLog.h>
#include <clientserver/printStructs.h>
#include <clientserver/freeDataBlock.h>
#include <clientserver/protocol.h>
#include <clientserver/udaErrors.h>

#include "mastArchiveFilePath.h"

int dumpFile(REQUEST_BLOCK request_block, DATA_BLOCK* data_block)
{

    int err = 0, serrno;

    char cmd[MAXRECLENGTH];
    int offset, bufsize, nread, nchar;
    char* bp = NULL;
    char alias[4] = "";
    char file[STRING_LENGTH] = "";
    char path[STRING_LENGTH] = "";
    char exp_number_str[STRING_LENGTH];
    char* env = NULL;

//----------------------------------------------------------------------
// File Location

    IDAM_LOGF(UDA_LOG_DEBUG, "Exp. Number  : %d \n", request_block.exp_number);
    IDAM_LOGF(UDA_LOG_DEBUG, "Pass Number  : %d \n", request_block.pass);
    IDAM_LOGF(UDA_LOG_DEBUG, "Signal       : %s \n", request_block.signal);
    IDAM_LOGF(UDA_LOG_DEBUG, "File Name    : %s \n", request_block.file);
    IDAM_LOGF(UDA_LOG_DEBUG, "File Path    : %s \n", request_block.path);

    err = 0;

    if (request_block.exp_number > 0 && request_block.request == REQUEST_READ_IDA) {

        if (strlen(request_block.signal) > 0) {
            strncpy(alias, request_block.signal, 3);
            alias[3] = '\0';
            strlwr(alias);
        }

        strlwr(request_block.file);

// Check whether or not the filename is the alias name
// If is it then form the correct filename

        if (STR_IEQUALS(request_block.file, alias)) {
            nameIDA(alias, request_block.exp_number, file);
        } else {
            strcpy(file, request_block.file);
        }

// Check whether or not a Path has been specified

        if (strlen(request_block.path) == 0) {
            mastArchiveFilePath(request_block.exp_number, request_block.pass, file, path);    // Always Latest
        } else {                        // User Specified
            strcpy(path, request_block.path);
            strcat(path, "/");
            strcat(path, file);                // Form Full File Name
        }

    } else {
        strcpy(file, request_block.file);
        strcpy(path, request_block.path);        //Fully Specified
    }

    IDAM_LOGF(UDA_LOG_DEBUG, "File Alias   : %s \n", alias);
    IDAM_LOGF(UDA_LOG_DEBUG, "File Name    : %s \n", file);
    IDAM_LOGF(UDA_LOG_DEBUG, "File Path    : %s \n", path);

//----------------------------------------------------------------------
// Test for embedded semi-colons => embedded linux commands

    if (!IsLegalFilePath(path)) {
        err = 999;
        addIdamError(CODEERRORTYPE, "dumpFile", err, "The directory path has incorrect syntax");
    }

//----------------------------------------------------------------------
// Error Trap Loop

    FILE* ph = NULL;

    do {

        if (err != 0) break;

//----------------------------------------------------------------------
// Create the output file using the appropriate dump utility program

        switch (request_block.request) {
            case REQUEST_READ_IDA:
                if ((env = getenv("UDA_DUMP_IDA")) != NULL) {
                    strcpy(cmd, env);
                    strcat(cmd, " ");
                } else {
                    strcpy(cmd, "idadump ");
                }
                break;
            case REQUEST_READ_CDF:
                if ((env = getenv("UDA_DUMP_NETCDF")) != NULL) {
                    strcpy(cmd, env);
                    strcat(cmd, " -h ");
                } else {
                    strcpy(cmd, "ncdump -h ");
                }
                break;
            case REQUEST_READ_HDF5:
                if ((env = getenv("UDA_DUMP_HDF5")) != NULL) {
                    strcpy(cmd, env);
                    strcat(cmd, " -n ");
                } else {
                    strcpy(cmd, "h5dump -n ");
                }
                break;
            case REQUEST_READ_MDS: {

// Java example: http://www.mdsplus.org/mdsplus/cvsweb.cgi/mdsplus/javatraverser/DecompileTree.java

                char server[MAXSERVER];
                char* token = NULL;
                int i, lpath;

                strcpy(server, request_block.server);
                if (!strncasecmp(server, "localhost.", 10)) server[9] = '/';        // For Parsing

                if ((token = strstr(server, "/")) != NULL) {                // The Server contains the path to the data
                    strcpy(path, token);                        // Extract the Path
                    server[token - server] = '\0';                    // Extract the Server Name
                    if (STR_IEQUALS(server, "localhost")) {
                        lpath = (int) strlen(path);
                        if (!IsLegalFilePath(path)) {                    // Check the file path is regular
                            err = 999;
                            addIdamError(CODEERRORTYPE, "dumpFile", err,
                                         "Unacceptable Path to MDS+ Data Tree");
                            IDAM_LOGF(UDA_LOG_DEBUG,
                                    "Syntax error in the directory path to the MDS+ Data Tree %s\n", path);
                            break;
                        }
                        for (i = 0; i < lpath; i++)
                            if (path[i] == '.')
                                path[i] = '/';        // Change from URL Notation to Path Tree Notation
                    } else {
                        err = 999;
                        addIdamError(CODEERRORTYPE, "dumpFile", err,
                                     "Unable to Set Trees Paths for Remote MDSPlus Servers");
                        IDAM_LOGF(UDA_LOG_DEBUG, "Unable to Set Trees Paths for Remote MDSPlus Servers - %s\n",
                                path);
                        break;
                    }
                }

                if (strlen(server) == 0) strcpy(server, LOCAL_MDSPLUS_SERVER);    // Need a Server!!! - Use the Default

                sprintf(exp_number_str, "%d", request_block.exp_number);
                setenv("IDAM_SERVER_TREENAME", request_block.file, 1);
                setenv("IDAM_SERVER_TREENUM", exp_number_str, 1);

                setenv("IDAM_SERVER_TREESERVER", server, 1);
                setenv("IDAM_SERVER_TREEPATH", path, 1);

                IDAM_LOGF(UDA_LOG_DEBUG, "IDAM_SERVER_TREENAME:   %s\n", request_block.file);
                IDAM_LOGF(UDA_LOG_DEBUG, "IDAM_SERVER_TREENUM:    %s\n", exp_number_str);
                IDAM_LOGF(UDA_LOG_DEBUG, "IDAM_SERVER_TREESERVER: %s\n", server);
                IDAM_LOGF(UDA_LOG_DEBUG, "IDAM_SERVER_TREEPATH:   %s\n", path);

                if ((env = getenv("UDA_DUMP_MDSPLUS")) != NULL) {
                    strcpy(cmd, env);
                    strcat(cmd, " mdsdump ");
                } else {
                    strcpy(cmd, "idl mdsdump ");        // Must be on the Server's path
                }
                path[0] = '\0';                // Details are passed via Environment variables
                break;
            }
            default:
                err = 999;
                addIdamError(CODEERRORTYPE, "dumpFile", err,
                             "No DUMP Utility Program for this File Format");
                break;
        }
        if (err != 0) break;


        strcat(cmd, path);
        strcat(cmd, " 2>&1");

        IDAM_LOGF(UDA_LOG_DEBUG, "DUMP: %s\n", cmd);

// Execute the Command and Open a Pipe to the Output for Reading

        errno = 0;
        ph = popen(cmd, "r");
        serrno = errno;

        if (ph == NULL || serrno != 0) {
            err = 999;
            if (serrno != 0) addIdamError(SYSTEMERRORTYPE, "dumpFile", serrno, "");
            addIdamError(CODEERRORTYPE, "dumpFile", err, "Problem Running the DUMP utility program");
            break;
        }

        nchar = 0;
        offset = 0;
        bufsize = 1024;
        data_block->data_n = bufsize;

        while (!feof(ph)) {
            if ((bp = (char*) realloc(bp, data_block->data_n)) == NULL) {
                err = 9998;
                addIdamError(CODEERRORTYPE, "dumpFile", err,
                             "Unable to Allocate Heap Memory for the File DUMP");
                break;
            }
            nread = (int) fread(bp + offset, sizeof(char), bufsize, ph);
            nchar = nchar + nread;
            offset = nchar;
            data_block->data_n = nchar + bufsize + 1;
        }

        if (err != 0) break;

        IDAM_LOGF(UDA_LOG_DEBUG, "nchar %d\n", nchar);

        data_block->data_n = nchar;
        data_block->data = (char*) bp;

        data_block->rank = 0;        // Scalar Array of Bytes
        data_block->order = -1;        // No Time Dimension

        data_block->data_type = UDA_TYPE_STRING;

//----------------------------------------------------------------------
// End of Error Trap Loop

    } while (0);

//----------------------------------------------------------------------
// Housekeeping

    IDAM_LOGF(UDA_LOG_DEBUG, "DUMP: err %d\n", err);
    IDAM_LOGF(UDA_LOG_DEBUG, "errno     %d\n", errno);
    printDataBlock(*data_block);

    if (err != 0) freeDataBlock(data_block);
    if (ph != NULL) fclose(ph);

    return err;
}
