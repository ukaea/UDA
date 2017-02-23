/*--------------------------------------------------------------------------------------------------------------------
* Legacy Request Data structure from old Client versions
*
* No alteration to request_block are necessary - only interpretation.
*---------------------------------------------------------------------------------------------------------------------*/
#include "serverLegacyPlugin.h"

#include <stdlib.h>
#include <strings.h>

#include <logging/logging.h>
#include <clientserver/errorLog.h>
#include <modules/ida/parseIdaPath.h>
#include <clientserver/stringUtils.h>
#include <clientserver/protocol.h>
#include <clientserver/udaErrors.h>

#ifndef FATCLIENT
#  include <server/getServerEnvironment.h>
#endif

int idamServerLegacyPlugin(REQUEST_BLOCK* request_block, DATA_SOURCE* data_source, SIGNAL_DESC* signal_desc)
{

    int err = 0;
#ifndef NOTGENERICENABLED
    int exp_number, pass;

    char file[STRING_LENGTH] = "";
    char signal[STRING_LENGTH] = "";
    char archive[STRING_LENGTH] = "";
    char device_name[STRING_LENGTH] = "";
#endif
    char* token = NULL;
    char work[STRING_LENGTH];

    IDAM_LOG(LOG_DEBUG, "IdamServerLegacyPlugin: Start\n");

//----------------------------------------------------------------------------
// Start of Error Trap

    do {

//----------------------------------------------------------------------------
// Does the Path to Private Files contain hierarchical components not seen by the server? If so make a substitution.

#ifndef FATCLIENT

        ENVIRONMENT* environment = getIdamServerEnvironment();

        if (request_block->request == REQUEST_READ_FORMAT) {
            if (environment->private_path_target[0] != '\0') {

                char* delimiters = ",:";
                char targets[10][256];
                char substitutes[10][256];
                int lpath, i, tcount = 0, scount = 0;

                strcpy(work, environment->private_path_target);
                token = strtok(work, delimiters);
                strcpy(targets[tcount++], token);
                while ((token = strtok(NULL, delimiters)) != NULL) strcpy(targets[tcount++], token);

                strcpy(work, environment->private_path_substitute);
                token = strtok(work, delimiters);
                strcpy(substitutes[scount++], token);
                while ((token = strtok(NULL, delimiters)) != NULL) strcpy(substitutes[scount++], token);

                if (tcount == scount) {
                    for (i = 0; i < tcount; i++) {
                        lpath = (int) strlen(targets[i]);
                        if (!strncmp(request_block->path, targets[i], lpath)) {
                            strcpy(work, &request_block->path[lpath]);
                            strcpy(request_block->path, substitutes[i]);
                            strcat(request_block->path, work);
                        }
                    }
                } else {
                    err = 999;
                    addIdamError(&idamerrorstack, CODEERRORTYPE, "idamServer", err,
                                 "Unmatched count of Target and Substitute File Paths.");
                    break;

                }
            }
        }

#endif


//----------------------------------------------------------------------
// Client Requests the Server to Choose Data Access plugin


        if (request_block->request == REQUEST_READ_FORMAT) {
            IDAM_LOG(LOG_DEBUG, "Request: REQUEST_READ_FORMAT \n");
            IDAM_LOGF(LOG_DEBUG, "Format : %s \n", request_block->format);

            if (STR_IEQUALS(request_block->format, "IDA") || STR_IEQUALS(request_block->format, "IDA3")) {
                request_block->request = REQUEST_READ_IDA;
                parseIDAPath(request_block);        // Check Path for file details
            } else if (STR_IEQUALS(request_block->format, "NETCDF")) request_block->request = REQUEST_READ_CDF;
            else if (STR_IEQUALS(request_block->format, "HDF5")) request_block->request = REQUEST_READ_HDF5;
            else if (STR_IEQUALS(request_block->format, "XML")) {
                request_block->request = REQUEST_READ_XML;
                parseXMLPath(request_block);        // Check Path for details
            } else if (STR_IEQUALS(request_block->format, "UFILE")) request_block->request = REQUEST_READ_UFILE;
            else if (STR_IEQUALS(request_block->format, "BIN") || STR_IEQUALS(request_block->format, "BINARY"))
                request_block->request = REQUEST_READ_FILE;
            else if (STR_IEQUALS(request_block->format, "PPF")) request_block->request = REQUEST_READ_PPF;
            else if (STR_IEQUALS(request_block->format, "JPF")) request_block->request = REQUEST_READ_JPF;
            else if (STR_IEQUALS(request_block->format, "TEST")) request_block->request = REQUEST_READ_NEW_PLUGIN;
            else if (STR_IEQUALS(request_block->format, "NOTHING")) request_block->request = REQUEST_READ_NOTHING;
            else if (STR_IEQUALS(request_block->format, "HDATA")) request_block->request = REQUEST_READ_HDATA;
            else if (STR_IEQUALS(request_block->format, "SQL")) request_block->request = REQUEST_READ_SQL;

            IDAM_LOGF(LOG_DEBUG, "Request Selected: %d\n", request_block->request);
            IDAM_LOGF(LOG_DEBUG, "File: %s\n", request_block->file);
            IDAM_LOGF(LOG_DEBUG, "Path: %s\n", request_block->path);
#ifdef IDA_ENABLE
            IDAM_LOGF(LOG_DEBUG, "IDA is Enabled!\n");
#endif
        }

//----------------------------------------------------------------------
// Client Identifies the File or Signal via the State Block

        switch (request_block->request) {

//#ifdef IDA_ENABLE
            case REQUEST_READ_IDA :

                strcpy(data_source->source_alias, TrimString(request_block->file));
                strcpy(data_source->filename, TrimString(request_block->file));
                strcpy(data_source->path, TrimString(request_block->path));

                copyString(TrimString(request_block->signal), signal_desc->signal_name, MAXNAME);

                data_source->exp_number = request_block->exp_number;
                data_source->pass = request_block->pass;
                data_source->type = ' ';

                IDAM_LOG(LOG_DEBUG, "Request: Read IDA \n");
                IDAM_LOGF(LOG_DEBUG, "IdamServer: File Alias   : %s \n", request_block->file);
                IDAM_LOGF(LOG_DEBUG, "IdamServer: File Path    : %s \n", request_block->path);
                IDAM_LOGF(LOG_DEBUG, "IdamServer: Signal       : %s \n", request_block->signal);
                IDAM_LOGF(LOG_DEBUG, "IdamServer: Pulse Number : %d \n", request_block->exp_number);
                IDAM_LOGF(LOG_DEBUG, "IdamServer: Pass Number  : %d \n", request_block->pass);
                break;
//#endif
            case REQUEST_READ_NEW_PLUGIN :

                strcpy(data_source->source_alias, TrimString(request_block->file));
                strcpy(data_source->filename, TrimString(request_block->file));
                strcpy(data_source->path, TrimString(request_block->path));

                copyString(TrimString(request_block->signal), signal_desc->signal_name, MAXNAME);

                data_source->exp_number = request_block->exp_number;
                data_source->pass = request_block->pass;
                data_source->type = ' ';

                IDAM_LOG(LOG_DEBUG, "Request: Read IDA \n");
                IDAM_LOGF(LOG_DEBUG, "IdamServer: File Alias   : %s \n", request_block->file);
                IDAM_LOGF(LOG_DEBUG, "IdamServer: File Path    : %s \n", request_block->path);
                IDAM_LOGF(LOG_DEBUG, "IdamServer: Signal       : %s \n", request_block->signal);
                IDAM_LOGF(LOG_DEBUG, "IdamServer: Pulse Number : %d \n", request_block->exp_number);
                IDAM_LOGF(LOG_DEBUG, "IdamServer: Pass Number  : %d \n", request_block->pass);
                break;

            case REQUEST_READ_MDS :

                strcpy(data_source->filename, TrimString(request_block->file));        // MDS+ Tree
                strcpy(data_source->server, TrimString(request_block->server));        // MDS+ Server Name

                copyString(TrimString(request_block->signal), signal_desc->signal_name, MAXNAME);

                if (strlen(signal_desc->signal_name) == MAXNAME - 1) {
                    copyString(TrimString(request_block->signal), signal_desc->xml, MAXMETA);    // Pass via XML member
                    signal_desc->signal_name[0] = '\0';
                }

                data_source->exp_number = request_block->exp_number;                // MDS+ Tree Number

                IDAM_LOG(LOG_DEBUG, "Request: Read MDS+ \n");
                IDAM_LOGF(LOG_DEBUG, "IdamServer: Server       : %s \n", request_block->server);
                IDAM_LOGF(LOG_DEBUG, "IdamServer: Tree         : %s \n", request_block->file);
                IDAM_LOGF(LOG_DEBUG, "IdamServer: Data Node    : %s \n", request_block->signal);
                IDAM_LOGF(LOG_DEBUG, "IdamServer: Tree Number  : %d \n", request_block->exp_number);
                break;

            case REQUEST_READ_IDAM :
                IDAM_LOG(LOG_DEBUG, "Request: Read Remote IDAM Source \n");
                IDAM_LOGF(LOG_DEBUG, "IdamServer: Server       : %s \n", request_block->server);
                IDAM_LOGF(LOG_DEBUG, "IdamServer: Source       : %s \n", request_block->file);
                IDAM_LOGF(LOG_DEBUG, "IdamServer: Signal       : %s \n", request_block->signal);
                break;

            case REQUEST_READ_CDF :

                strcpy(data_source->path, TrimString(request_block->path));        // netCDF File Location
                copyString(TrimString(request_block->signal), signal_desc->signal_name, MAXNAME);

                IDAM_LOG(LOG_DEBUG, "Request: readnetCDF \n");
                IDAM_LOGF(LOG_DEBUG, "IdamServer: netCDF File  : %s \n", request_block->path);
                IDAM_LOGF(LOG_DEBUG, "IdamServer: Signal       : %s \n", request_block->signal);
                break;

            case REQUEST_READ_HDF5 :

                strcpy(data_source->path, TrimString(request_block->path));        // HDF5 File Location
                copyString(TrimString(request_block->signal), signal_desc->signal_name, MAXNAME);

                IDAM_LOG(LOG_DEBUG, "Request: ReadHDF5 \n");
                IDAM_LOGF(LOG_DEBUG, "IdamServer: HDF5 File    : %s \n", request_block->path);
                IDAM_LOGF(LOG_DEBUG, "IdamServer: Signal       : %s \n", request_block->signal);
                break;

            case REQUEST_READ_XML :

                data_source->exp_number = request_block->exp_number;
                data_source->pass = request_block->pass;

                IDAM_LOG(LOG_DEBUG, "Request: ReadXML \n");
                IDAM_LOGF(LOG_DEBUG, "IdamServer: XML File     : %s \n", request_block->path);
                IDAM_LOGF(LOG_DEBUG, "IdamServer: XML Document : %s \n", request_block->signal);
                break;

            case REQUEST_READ_UFILE :

                strcpy(data_source->path, TrimString(request_block->path));    // UFile File Location

                IDAM_LOG(LOG_DEBUG, "Request: ReadUFile \n");
                IDAM_LOGF(LOG_DEBUG, "IdamServer: UFile File   : %s \n", request_block->path);
                break;

            case REQUEST_READ_FILE :

                strcpy(data_source->path, TrimString(request_block->path));    // File Location

                IDAM_LOG(LOG_DEBUG, "Request: ReadBytes \n");
                IDAM_LOGF(LOG_DEBUG, "IdamServer: File  : %s \n", request_block->path);
                break;


            case REQUEST_READ_HDATA :

                strcpy(data_source->path, TrimString(request_block->path));    // File Location

                IDAM_LOG(LOG_DEBUG, "Request: ReadHData \n");
                IDAM_LOGF(LOG_DEBUG, "IdamServer: File  : %s \n", request_block->path);
                break;

            case REQUEST_READ_SQL :

                strcpy(data_source->path, TrimString(request_block->path));        // SQL database etc.
                strcpy(data_source->server, TrimString(request_block->server));        // SQL server host
                strcpy(data_source->format, TrimString(request_block->format));
                strcpy(data_source->archive, TrimString(request_block->archive));
                strcpy(data_source->device_name, TrimString(request_block->device_name));

                IDAM_LOG(LOG_DEBUG, "Request: ReadSQL \n");
                IDAM_LOGF(LOG_DEBUG, "IdamServer: SQL   : %s \n", request_block->signal);
                break;

            case REQUEST_READ_NOTHING :

                data_source->exp_number = request_block->exp_number;        // Size of Data Block
                data_source->pass = request_block->pass;        // Compressible or Not

                if (data_source->exp_number == 0 && data_source->pass == -1) {    // May be passed in Path String
                    strcpy(work, request_block->path);
                    if (work[0] == '/' && (token = strtok(work, "/")) != NULL) {    // Tokenise the remaining string
                        if (IsNumber(token)) {                    // Is the First token an integer number?
                            request_block->exp_number = atoi(token);
                            if ((token = strtok(NULL, "/")) != NULL) {        // Next Token
                                if (IsNumber(token)) {
                                    request_block->pass = atoi(token);        // Must be the Pass number
                                } else {
                                    strcpy(request_block->tpass, token);        // anything else
                                }
                            }
                        }
                        data_source->exp_number = request_block->exp_number;        // Size of Data Block
                        data_source->pass = request_block->pass;        // Compressible or Not
                    }
                }

                IDAM_LOG(LOG_DEBUG, "Request: Read Nothing! (Returns Test Data)\n");
                break;

#ifndef NOTGENERICENABLED
            case REQUEST_READ_GENERIC :

                strcpy(file, TrimString(request_block->file));
                strcpy(signal, TrimString(request_block->signal));
                strcpy(archive, TrimString(request_block->archive));
                strcpy(device_name, TrimString(request_block->device_name));
                exp_number = request_block->exp_number;
                pass = request_block->pass;

                strupr(signal);
                strlwr(archive);
                strlwr(device_name);

                IDAM_LOG(LOG_DEBUG, "Request: Read Generic \n");
                IDAM_LOGF(LOG_DEBUG, "IdamServer: Archive      : %s \n", archive);
                IDAM_LOGF(LOG_DEBUG, "IdamServer: Device Name  : %s \n", device_name);
                IDAM_LOGF(LOG_DEBUG, "IdamServer: Signal Name  : %s \n", signal);
                IDAM_LOGF(LOG_DEBUG, "IdamServer: Pulse Number : %d \n", exp_number);
                IDAM_LOGF(LOG_DEBUG, "IdamServer: Pass Number  : %d \n", pass);
                break;
#endif

            case REQUEST_READ_PPF :

                strcpy(data_source->source_alias, TrimString(request_block->file));
                strcpy(data_source->filename, TrimString(request_block->file));
                strcpy(data_source->path, TrimString(request_block->path));
                copyString(TrimString(request_block->signal), signal_desc->signal_name, MAXNAME);
                data_source->exp_number = request_block->exp_number;
                data_source->pass = request_block->pass;
                data_source->type = ' ';

                IDAM_LOG(LOG_DEBUG, "Request: Read PPF \n");
                IDAM_LOGF(LOG_DEBUG, "IdamServer: File Alias   : %s \n", request_block->file);
                IDAM_LOGF(LOG_DEBUG, "IdamServer: File Path    : %s \n", request_block->path);
                IDAM_LOGF(LOG_DEBUG, "IdamServer: Signal       : %s \n", request_block->signal);
                IDAM_LOGF(LOG_DEBUG, "IdamServer: Pulse Number : %d \n", request_block->exp_number);
                IDAM_LOGF(LOG_DEBUG, "IdamServer: Pass Number  : %d \n", request_block->pass);
                break;

            case REQUEST_READ_JPF :

                copyString(TrimString(request_block->signal), signal_desc->signal_name, MAXNAME);
                data_source->exp_number = request_block->exp_number;

                IDAM_LOG(LOG_DEBUG, "Request: Read JPF \n");
                IDAM_LOGF(LOG_DEBUG, "IdamServer: Signal       : %s \n", request_block->signal);
                IDAM_LOGF(LOG_DEBUG, "IdamServer: Pulse Number : %d \n", request_block->exp_number);
                break;


            default:
                IDAM_LOGF(LOG_DEBUG, "Unknown Requested Data Access Routine (%d) \n", request_block->request);
                err = 9999;
                addIdamError(&idamerrorstack, CODEERRORTYPE, "idamServer", err,
                             "Unknown Requested Data Access Routine");
                break;
        }

        if (err != 0) break;

//------------------------------------------------------------------------------------------------
// End of Error Trap

    } while (0);

    IDAM_LOG(LOG_DEBUG, "IdamServerLegacyPlugin: End\n");

    return err;
}
