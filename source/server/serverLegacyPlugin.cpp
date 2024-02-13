#include "serverLegacyPlugin.h"

#include <cstdlib>
#if defined(__GNUC__)
#  include <strings.h>
#endif

#include "clientserver/errorLog.h"
#include "clientserver/stringUtils.h"
#include "logging/logging.h"

#ifndef FATCLIENT
#  include "server/getServerEnvironment.h"
#endif

using namespace uda::client_server;
using namespace uda::server;
using namespace uda::logging;

int uda::server::udaServerLegacyPlugin(RequestData* request, DataSource* data_source, SignalDesc* signal_desc)
{
    int err = 0;
    char* token = nullptr;
    char work[STRING_LENGTH];

    UDA_LOG(UDA_LOG_DEBUG, "Start\n");

    //----------------------------------------------------------------------------
    // Start of Error Trap

    do {

        //----------------------------------------------------------------------------
        // Does the Path to Private Files contain hierarchical components not seen by the server? If so make a
        // substitution.

#ifndef FATCLIENT

        Environment* environment = getServerEnvironment();

        if (request->request == REQUEST_READ_FORMAT) {
            if (environment->private_path_target[0] != '\0') {

                const char* delimiters = ",:";
                char targets[10][256];
                char substitutes[10][256];
                int lpath, tcount = 0, scount = 0;

                strcpy(work, environment->private_path_target);
                token = strtok(work, delimiters);
                strcpy(targets[tcount++], token);
                while ((token = strtok(nullptr, delimiters)) != nullptr) {
                    strcpy(targets[tcount++], token);
                }

                strcpy(work, environment->private_path_substitute);
                token = strtok(work, delimiters);
                strcpy(substitutes[scount++], token);
                while ((token = strtok(nullptr, delimiters)) != nullptr) {
                    strcpy(substitutes[scount++], token);
                }

                if (tcount == scount) {
                    for (int i = 0; i < tcount; i++) {
                        lpath = (int)strlen(targets[i]);
                        if (!strncmp(request->path, targets[i], lpath)) {
                            strcpy(work, &request->path[lpath]);
                            strcpy(request->path, substitutes[i]);
                            strcat(request->path, work);
                        }
                    }
                } else {
                    err = 999;
                    add_error(UDA_CODE_ERROR_TYPE, __func__, err,
                              "Unmatched count of Target and Substitute File Paths.");
                    break;
                }
            }
        }

#endif

        //----------------------------------------------------------------------
        // Client Requests the Server to Choose Data Access plugin

        if (request->request == REQUEST_READ_FORMAT) {
            UDA_LOG(UDA_LOG_DEBUG, "Request: REQUEST_READ_FORMAT \n");
            UDA_LOG(UDA_LOG_DEBUG, "Format : %s \n", request->format);

            if (STR_IEQUALS(request->format, "IDA") || STR_IEQUALS(request->format, "IDA3")) {
                request->request = REQUEST_READ_IDA;
                //                parseIDAPath(request);        // Check Path for file details
            } else if (STR_IEQUALS(request->format, "NETCDF")) {
                request->request = REQUEST_READ_CDF;
            } else if (STR_IEQUALS(request->format, "HDF5")) {
                request->request = REQUEST_READ_HDF5;
            } else if (STR_IEQUALS(request->format, "XML")) {
                request->request = REQUEST_READ_XML;
                //                parseXMLPath(request);        // Check Path for details
            } else if (STR_IEQUALS(request->format, "UFILE")) {
                request->request = REQUEST_READ_UFILE;
            } else if (STR_IEQUALS(request->format, "BIN") || STR_IEQUALS(request->format, "BINARY")) {
                request->request = REQUEST_READ_FILE;
            } else if (STR_IEQUALS(request->format, "PPF")) {
                request->request = REQUEST_READ_PPF;
            } else if (STR_IEQUALS(request->format, "JPF")) {
                request->request = REQUEST_READ_JPF;
            } else if (STR_IEQUALS(request->format, "TEST")) {
                request->request = REQUEST_READ_NEW_PLUGIN;
            } else if (STR_IEQUALS(request->format, "NOTHING")) {
                request->request = REQUEST_READ_NOTHING;
            } else if (STR_IEQUALS(request->format, "HDATA")) {
                request->request = REQUEST_READ_HDATA;
            } else if (STR_IEQUALS(request->format, "SQL")) {
                request->request = REQUEST_READ_SQL;
            }

            UDA_LOG(UDA_LOG_DEBUG, "Request Selected: %d\n", request->request);
            UDA_LOG(UDA_LOG_DEBUG, "File: %s\n", request->file);
            UDA_LOG(UDA_LOG_DEBUG, "Path: %s\n", request->path);
#ifdef IDA_ENABLE
            UDA_LOG(UDA_LOG_DEBUG, "IDA is Enabled!\n");
#endif
        }

        //----------------------------------------------------------------------
        // Client Identifies the File or Signal via the State Block

        switch (request->request) {
            case REQUEST_READ_IDA:

                strcpy(data_source->source_alias, trim_string(request->file));
                strcpy(data_source->filename, trim_string(request->file));
                strcpy(data_source->path, trim_string(request->path));

                copy_string(trim_string(request->signal), signal_desc->signal_name, MAXNAME);

                data_source->exp_number = request->exp_number;
                data_source->pass = request->pass;
                data_source->type = ' ';

                UDA_LOG(UDA_LOG_DEBUG, "Request: Read IDA \n");
                UDA_LOG(UDA_LOG_DEBUG, "File Alias   : %s \n", request->file);
                UDA_LOG(UDA_LOG_DEBUG, "File Path    : %s \n", request->path);
                UDA_LOG(UDA_LOG_DEBUG, "Signal       : %s \n", request->signal);
                UDA_LOG(UDA_LOG_DEBUG, "Pulse Number : %d \n", request->exp_number);
                UDA_LOG(UDA_LOG_DEBUG, "Pass Number  : %d \n", request->pass);
                break;

            case REQUEST_READ_NEW_PLUGIN:
                strcpy(data_source->source_alias, trim_string(request->file));
                strcpy(data_source->filename, trim_string(request->file));
                strcpy(data_source->path, trim_string(request->path));

                copy_string(trim_string(request->signal), signal_desc->signal_name, MAXNAME);

                data_source->exp_number = request->exp_number;
                data_source->pass = request->pass;
                data_source->type = ' ';

                UDA_LOG(UDA_LOG_DEBUG, "Request: Read IDA \n");
                UDA_LOG(UDA_LOG_DEBUG, "File Alias   : %s \n", request->file);
                UDA_LOG(UDA_LOG_DEBUG, "File Path    : %s \n", request->path);
                UDA_LOG(UDA_LOG_DEBUG, "Signal       : %s \n", request->signal);
                UDA_LOG(UDA_LOG_DEBUG, "Pulse Number : %d \n", request->exp_number);
                UDA_LOG(UDA_LOG_DEBUG, "Pass Number  : %d \n", request->pass);
                break;

            case REQUEST_READ_MDS:
                strcpy(data_source->filename, trim_string(request->file)); // MDS+ Tree
                strcpy(data_source->server, trim_string(request->server)); // MDS+ Server Name

                copy_string(trim_string(request->signal), signal_desc->signal_name, MAXNAME);

                if (strlen(signal_desc->signal_name) == MAXNAME - 1) {
                    copy_string(trim_string(request->signal), signal_desc->xml, MAXMETA); // Pass via XML member
                    signal_desc->signal_name[0] = '\0';
                }

                data_source->exp_number = request->exp_number; // MDS+ Tree Number

                UDA_LOG(UDA_LOG_DEBUG, "Request: Read MDS+ \n");
                UDA_LOG(UDA_LOG_DEBUG, "Server       : %s \n", request->server);
                UDA_LOG(UDA_LOG_DEBUG, "Tree         : %s \n", request->file);
                UDA_LOG(UDA_LOG_DEBUG, "Data Node    : %s \n", request->signal);
                UDA_LOG(UDA_LOG_DEBUG, "Tree Number  : %d \n", request->exp_number);
                break;

            case REQUEST_READ_IDAM:
                UDA_LOG(UDA_LOG_DEBUG, "Request: Read Remote IDAM Source \n");
                UDA_LOG(UDA_LOG_DEBUG, "Server       : %s \n", request->server);
                UDA_LOG(UDA_LOG_DEBUG, "Source       : %s \n", request->file);
                UDA_LOG(UDA_LOG_DEBUG, "Signal       : %s \n", request->signal);
                break;

            case REQUEST_READ_CDF:
                strcpy(data_source->path, trim_string(request->path)); // netCDF File Location
                copy_string(trim_string(request->signal), signal_desc->signal_name, MAXNAME);

                UDA_LOG(UDA_LOG_DEBUG, "Request: readnetCDF \n");
                UDA_LOG(UDA_LOG_DEBUG, "netCDF File  : %s \n", request->path);
                UDA_LOG(UDA_LOG_DEBUG, "Signal       : %s \n", request->signal);
                break;

            case REQUEST_READ_HDF5:
                strcpy(data_source->path, trim_string(request->path)); // HDF5 File Location
                copy_string(trim_string(request->signal), signal_desc->signal_name, MAXNAME);

                UDA_LOG(UDA_LOG_DEBUG, "Request: ReadHDF5 \n");
                UDA_LOG(UDA_LOG_DEBUG, "HDF5 File    : %s \n", request->path);
                UDA_LOG(UDA_LOG_DEBUG, "Signal       : %s \n", request->signal);
                break;

            case REQUEST_READ_XML:
                data_source->exp_number = request->exp_number;
                data_source->pass = request->pass;

                UDA_LOG(UDA_LOG_DEBUG, "Request: ReadXML \n");
                UDA_LOG(UDA_LOG_DEBUG, "XML File     : %s \n", request->path);
                UDA_LOG(UDA_LOG_DEBUG, "XML Document : %s \n", request->signal);
                break;

            case REQUEST_READ_UFILE:
                strcpy(data_source->path, trim_string(request->path)); // UFile File Location

                UDA_LOG(UDA_LOG_DEBUG, "Request: ReadUFile \n");
                UDA_LOG(UDA_LOG_DEBUG, "UFile File   : %s \n", request->path);
                break;

            case REQUEST_READ_FILE:
                strcpy(data_source->path, trim_string(request->path)); // File Location

                UDA_LOG(UDA_LOG_DEBUG, "Request: ReadBytes \n");
                UDA_LOG(UDA_LOG_DEBUG, "File  : %s \n", request->path);
                break;

            case REQUEST_READ_HDATA:
                strcpy(data_source->path, trim_string(request->path)); // File Location

                UDA_LOG(UDA_LOG_DEBUG, "Request: ReadHData \n");
                UDA_LOG(UDA_LOG_DEBUG, "File  : %s \n", request->path);
                break;

            case REQUEST_READ_SQL:
                strcpy(data_source->path, trim_string(request->path));     // SQL database etc.
                strcpy(data_source->server, trim_string(request->server)); // SQL server host
                strcpy(data_source->format, trim_string(request->format));
                strcpy(data_source->archive, trim_string(request->archive));
                strcpy(data_source->device_name, trim_string(request->device_name));

                UDA_LOG(UDA_LOG_DEBUG, "Request: ReadSQL \n");
                UDA_LOG(UDA_LOG_DEBUG, "SQL   : %s \n", request->signal);
                break;

            case REQUEST_READ_NOTHING:
                data_source->exp_number = request->exp_number; // Size of Data Block
                data_source->pass = request->pass;             // Compressible or Not

                if (data_source->exp_number == 0 && data_source->pass == -1) { // May be passed in Path String
                    strcpy(work, request->path);
                    if (work[0] == '/' && (token = strtok(work, "/")) != nullptr) { // Tokenise the remaining string
                        if (is_number(token)) { // Is the First token an integer number?
                            request->exp_number = atoi(token);
                            if ((token = strtok(nullptr, "/")) != nullptr) { // Next Token
                                if (is_number(token)) {
                                    request->pass = atoi(token); // Must be the Pass number
                                } else {
                                    strcpy(request->tpass, token); // anything else
                                }
                            }
                        }
                        data_source->exp_number = request->exp_number; // Size of Data Block
                        data_source->pass = request->pass;             // Compressible or Not
                    }
                }

                UDA_LOG(UDA_LOG_DEBUG, "Request: Read Nothing! (Returns Test Data)\n");
                break;

            case REQUEST_READ_PPF:
                strcpy(data_source->source_alias, trim_string(request->file));
                strcpy(data_source->filename, trim_string(request->file));
                strcpy(data_source->path, trim_string(request->path));
                copy_string(trim_string(request->signal), signal_desc->signal_name, MAXNAME);
                data_source->exp_number = request->exp_number;
                data_source->pass = request->pass;
                data_source->type = ' ';

                UDA_LOG(UDA_LOG_DEBUG, "Request: Read PPF \n");
                UDA_LOG(UDA_LOG_DEBUG, "File Alias   : %s \n", request->file);
                UDA_LOG(UDA_LOG_DEBUG, "File Path    : %s \n", request->path);
                UDA_LOG(UDA_LOG_DEBUG, "Signal       : %s \n", request->signal);
                UDA_LOG(UDA_LOG_DEBUG, "Pulse Number : %d \n", request->exp_number);
                UDA_LOG(UDA_LOG_DEBUG, "Pass Number  : %d \n", request->pass);
                break;

            case REQUEST_READ_JPF:
                copy_string(trim_string(request->signal), signal_desc->signal_name, MAXNAME);
                data_source->exp_number = request->exp_number;

                UDA_LOG(UDA_LOG_DEBUG, "Request: Read JPF \n");
                UDA_LOG(UDA_LOG_DEBUG, "Signal       : %s \n", request->signal);
                UDA_LOG(UDA_LOG_DEBUG, "Pulse Number : %d \n", request->exp_number);
                break;

            default:
                UDA_LOG(UDA_LOG_DEBUG, "Unknown Requested Data Access Routine (%d) \n", request->request);
                err = 9999;
                add_error(UDA_CODE_ERROR_TYPE, __func__, err, "Unknown Requested Data Access Routine");
                break;
        }

        if (err != 0) {
            break;
        }

        //------------------------------------------------------------------------------------------------
        // End of Error Trap

    } while (0);

    UDA_LOG(UDA_LOG_DEBUG, "End\n");

    return err;
}
