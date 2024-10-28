#include "serverLegacyPlugin.h"

#include <cstdlib>
#if defined(__GNUC__)
#  include <strings.h>
#endif

#include "clientserver/errorLog.h"
#include "common/stringUtils.h"
#include "logging/logging.h"

using namespace uda::client_server;
using namespace uda::server;
using namespace uda::logging;

int uda::server::udaServerLegacyPlugin(RequestData* request, DataSource* data_source, SignalDesc* signal_desc)
{
    int err = 0;
    char* token = nullptr;
    char work[STRING_LENGTH];

    UDA_LOG(UDA_LOG_DEBUG, "Start");

    //----------------------------------------------------------------------------
    // Start of Error Trap

    do {

        //----------------------------------------------------------------------
        // Client Requests the Server to Choose Data Access plugin

        if (request->request == (int)Request::ReadUDA) {
            UDA_LOG(UDA_LOG_DEBUG, "Request: Request::ReadFormat");
            UDA_LOG(UDA_LOG_DEBUG, "Format : {} ", request->format);

            if (STR_IEQUALS(request->format, "IDA") || STR_IEQUALS(request->format, "IDA3")) {
                request->request = (int)Request::ReadIDA;
                //                parseIDAPath(request);        // Check Path for file details
            } else if (STR_IEQUALS(request->format, "NETCDF")) {
                request->request = (int)Request::ReadCPF;
            } else if (STR_IEQUALS(request->format, "HDF5")) {
                request->request = (int)Request::ReadHDF5;
            } else if (STR_IEQUALS(request->format, "XML")) {
                request->request = (int)Request::ReadXML;
                //                parseXMLPath(request);        // Check Path for details
            } else if (STR_IEQUALS(request->format, "UFILE")) {
                request->request = (int)Request::ReadUFile;
            } else if (STR_IEQUALS(request->format, "BIN") || STR_IEQUALS(request->format, "BINARY")) {
                request->request = (int)Request::ReadFile;
            } else if (STR_IEQUALS(request->format, "PPF")) {
                request->request = (int)Request::ReadPPF;
            } else if (STR_IEQUALS(request->format, "JPF")) {
                request->request = (int)Request::ReadJPF;
            } else if (STR_IEQUALS(request->format, "TEST")) {
                request->request = (int)Request::ReadNewPlugin;
            } else if (STR_IEQUALS(request->format, "NOTHING")) {
                request->request = (int)Request::ReadNothing;
            } else if (STR_IEQUALS(request->format, "HDATA")) {
                request->request = (int)Request::ReadHData;
            } else if (STR_IEQUALS(request->format, "SQL")) {
                request->request = (int)Request::ReadSQL;
            }

            UDA_LOG(UDA_LOG_DEBUG, "Request Selected: {}", request->request);
            UDA_LOG(UDA_LOG_DEBUG, "File: {}", request->file);
            UDA_LOG(UDA_LOG_DEBUG, "Path: {}", request->path);
#ifdef IDA_ENABLE
            UDA_LOG(UDA_LOG_DEBUG, "IDA is Enabled!");
#endif
        }

        //----------------------------------------------------------------------
        // Client Identifies the File or Signal via the State Block

        switch (request->request) {
            case (int)Request::ReadIDA:

                strcpy(data_source->source_alias, trim_string(request->file));
                strcpy(data_source->filename, trim_string(request->file));
                strcpy(data_source->path, trim_string(request->path));

                copy_string(trim_string(request->signal), signal_desc->signal_name, MAXNAME);

                data_source->exp_number = request->exp_number;
                data_source->pass = request->pass;
                data_source->type = ' ';

                UDA_LOG(UDA_LOG_DEBUG, "Request: Read IDA");
                UDA_LOG(UDA_LOG_DEBUG, "File Alias   : {} ", request->file);
                UDA_LOG(UDA_LOG_DEBUG, "File Path    : {} ", request->path);
                UDA_LOG(UDA_LOG_DEBUG, "Signal       : {} ", request->signal);
                UDA_LOG(UDA_LOG_DEBUG, "Pulse Number : {} ", request->exp_number);
                UDA_LOG(UDA_LOG_DEBUG, "Pass Number  : {} ", request->pass);
                break;

            case (int)Request::ReadNewPlugin:
                strcpy(data_source->source_alias, trim_string(request->file));
                strcpy(data_source->filename, trim_string(request->file));
                strcpy(data_source->path, trim_string(request->path));

                copy_string(trim_string(request->signal), signal_desc->signal_name, MAXNAME);

                data_source->exp_number = request->exp_number;
                data_source->pass = request->pass;
                data_source->type = ' ';

                UDA_LOG(UDA_LOG_DEBUG, "Request: Read IDA");
                UDA_LOG(UDA_LOG_DEBUG, "File Alias   : {} ", request->file);
                UDA_LOG(UDA_LOG_DEBUG, "File Path    : {} ", request->path);
                UDA_LOG(UDA_LOG_DEBUG, "Signal       : {} ", request->signal);
                UDA_LOG(UDA_LOG_DEBUG, "Pulse Number : {} ", request->exp_number);
                UDA_LOG(UDA_LOG_DEBUG, "Pass Number  : {} ", request->pass);
                break;

            case (int)Request::ReadMDS:
                strcpy(data_source->filename, trim_string(request->file)); // MDS+ Tree
                strcpy(data_source->server, trim_string(request->server)); // MDS+ Server Name

                copy_string(trim_string(request->signal), signal_desc->signal_name, MAXNAME);

                if (strlen(signal_desc->signal_name) == MAXNAME - 1) {
                    copy_string(trim_string(request->signal), signal_desc->xml, MAXMETA); // Pass via XML member
                    signal_desc->signal_name[0] = '\0';
                }

                data_source->exp_number = request->exp_number; // MDS+ Tree Number

                UDA_LOG(UDA_LOG_DEBUG, "Request: Read MDS+");
                UDA_LOG(UDA_LOG_DEBUG, "Server       : {} ", request->server);
                UDA_LOG(UDA_LOG_DEBUG, "Tree         : {} ", request->file);
                UDA_LOG(UDA_LOG_DEBUG, "Data Node    : {} ", request->signal);
                UDA_LOG(UDA_LOG_DEBUG, "Tree Number  : {} ", request->exp_number);
                break;

            case (int)Request::ReadUDA:
                UDA_LOG(UDA_LOG_DEBUG, "Request: Read Remote IDAM Source");
                UDA_LOG(UDA_LOG_DEBUG, "Server       : {} ", request->server);
                UDA_LOG(UDA_LOG_DEBUG, "Source       : {} ", request->file);
                UDA_LOG(UDA_LOG_DEBUG, "Signal       : {} ", request->signal);
                break;

            case (int)Request::ReadCPF:
                strcpy(data_source->path, trim_string(request->path)); // netCDF File Location
                copy_string(trim_string(request->signal), signal_desc->signal_name, MAXNAME);

                UDA_LOG(UDA_LOG_DEBUG, "Request: readnetCDF");
                UDA_LOG(UDA_LOG_DEBUG, "netCDF File  : {} ", request->path);
                UDA_LOG(UDA_LOG_DEBUG, "Signal       : {} ", request->signal);
                break;

            case (int)Request::ReadHDF5:
                strcpy(data_source->path, trim_string(request->path)); // HDF5 File Location
                copy_string(trim_string(request->signal), signal_desc->signal_name, MAXNAME);

                UDA_LOG(UDA_LOG_DEBUG, "Request: ReadHDF5");
                UDA_LOG(UDA_LOG_DEBUG, "HDF5 File    : {} ", request->path);
                UDA_LOG(UDA_LOG_DEBUG, "Signal       : {} ", request->signal);
                break;

            case (int)Request::ReadXML:
                data_source->exp_number = request->exp_number;
                data_source->pass = request->pass;

                UDA_LOG(UDA_LOG_DEBUG, "Request: ReadXML");
                UDA_LOG(UDA_LOG_DEBUG, "XML File     : {} ", request->path);
                UDA_LOG(UDA_LOG_DEBUG, "XML Document : {} ", request->signal);
                break;

            case (int)Request::ReadUFile:
                strcpy(data_source->path, trim_string(request->path)); // UFile File Location

                UDA_LOG(UDA_LOG_DEBUG, "Request: ReadUFile");
                UDA_LOG(UDA_LOG_DEBUG, "UFile File   : {} ", request->path);
                break;

            case (int)Request::ReadFile:
                strcpy(data_source->path, trim_string(request->path)); // File Location

                UDA_LOG(UDA_LOG_DEBUG, "Request: ReadBytes");
                UDA_LOG(UDA_LOG_DEBUG, "File  : {} ", request->path);
                break;

            case (int)Request::ReadHData:
                strcpy(data_source->path, trim_string(request->path)); // File Location

                UDA_LOG(UDA_LOG_DEBUG, "Request: ReadHData");
                UDA_LOG(UDA_LOG_DEBUG, "File  : {} ", request->path);
                break;

            case (int)Request::ReadSQL:
                strcpy(data_source->path, trim_string(request->path));     // SQL database etc.
                strcpy(data_source->server, trim_string(request->server)); // SQL server host
                strcpy(data_source->format, trim_string(request->format));
                strcpy(data_source->archive, trim_string(request->archive));
                strcpy(data_source->device_name, trim_string(request->device_name));

                UDA_LOG(UDA_LOG_DEBUG, "Request: ReadSQL");
                UDA_LOG(UDA_LOG_DEBUG, "SQL   : {} ", request->signal);
                break;

            case (int)Request::ReadNothing:
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

                UDA_LOG(UDA_LOG_DEBUG, "Request: Read Nothing! (Returns Test Data)");
                break;

            case (int)Request::ReadPPF:
                strcpy(data_source->source_alias, trim_string(request->file));
                strcpy(data_source->filename, trim_string(request->file));
                strcpy(data_source->path, trim_string(request->path));
                copy_string(trim_string(request->signal), signal_desc->signal_name, MAXNAME);
                data_source->exp_number = request->exp_number;
                data_source->pass = request->pass;
                data_source->type = ' ';

                UDA_LOG(UDA_LOG_DEBUG, "Request: Read PPF");
                UDA_LOG(UDA_LOG_DEBUG, "File Alias   : {} ", request->file);
                UDA_LOG(UDA_LOG_DEBUG, "File Path    : {} ", request->path);
                UDA_LOG(UDA_LOG_DEBUG, "Signal       : {} ", request->signal);
                UDA_LOG(UDA_LOG_DEBUG, "Pulse Number : {} ", request->exp_number);
                UDA_LOG(UDA_LOG_DEBUG, "Pass Number  : {} ", request->pass);
                break;

            case (int)Request::ReadJPF:
                copy_string(trim_string(request->signal), signal_desc->signal_name, MAXNAME);
                data_source->exp_number = request->exp_number;

                UDA_LOG(UDA_LOG_DEBUG, "Request: Read JPF");
                UDA_LOG(UDA_LOG_DEBUG, "Signal       : {} ", request->signal);
                UDA_LOG(UDA_LOG_DEBUG, "Pulse Number : {} ", request->exp_number);
                break;

            default:
                UDA_LOG(UDA_LOG_DEBUG, "Unknown Requested Data Access Routine ({}) ", request->request);
                err = 9999;
                add_error(ErrorType::Code, __func__, err, "Unknown Requested Data Access Routine");
                break;
        }

        if (err != 0) {
            break;
        }

        //------------------------------------------------------------------------------------------------
        // End of Error Trap

    } while (0);

    UDA_LOG(UDA_LOG_DEBUG, "End");

    return err;
}
