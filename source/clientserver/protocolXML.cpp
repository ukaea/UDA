/*----------------------------------------------------------------------------------------------
 * Client - Server Conversation Protocol for XML based Hierarchical Data Structures
 *
 * Args:    xdrs        XDR Stream
 *
 *    protocol_id    Client/Server Conversation item: Data Exchange context
 *    direction    Send (0) or Receive (1) or Free (2)
 *    token        current error condition or next protocol or .... exchange token
 *
 *    str        Information Structure depending on the protocol id ....
 *
 *    100    efit
 *    101    pfcoils
 *    102    pfpassive
 *    103    pfsupplies
 *    104    fluxloop
 *    105    magprobe
 *    106    pfcircuit
 *    107    plasmacurrent
 *    108    diamagnetic
 *    109    toroidalfield
 *    110    limiter
 *
 * Returns: error code if failure, otherwise 0
 *
 *-----------------------------------------------------return;---------------------------------------------
 * Notes on Generalised Data Structures:
 *
 * The DataBlock structure has the following fields used to pass and receive generalised data structures
 *
 * data_block->data_type        set to UDA_TYPE_COMPOUND (external to this routine)
 * data_block->data_n        set to the count of structure array elements (external to this routine)
 *
 * data_block->data        sending (server side): set to the data array (external to this routine)
 *                receiving (client side): set to the root data tree node within this routine
 *
 * data_block->opaque_type    set to UDA_OPAQUE_TYPE_STRUCTURES (external to this routine)
 * data_block->count        set to 1 (external to this routine). Not Used!
 *
 * data_block->opaque_block    sending (server side): set to the User Defined Data Structure Definition of
 *                the Data (external to this routine).
 *                receiving (client side): set to the SArray Data Structure Definition
 *
 * The SArray structure has the following:
 *
 * sarray.count            set to the count of structure array elements. Identical to data_block->data_n.
 * sarray.rank            set to 1 (Higher ranked arrays possible ?)
 * sarray.shape            set to [sarray.count] for consistency.
 * sarray.data            set to data_block->data
 * sarray.type            set to the name of the User Defined Structure type of data
 *                (data_block->opaque_block->name). This is registered within the Structure
 *                Type List.
 **--------------------------------------------------------------------------------------------------*/
#include "protocolXML.h"

#include <cerrno>
#include <cstdlib>
#include <memory.h>
#include <tuple>
#include <uda/structured.h>

#include "logging/logging.h"
#include <fmt/format.h>

#include "errorLog.h"
#include "protocol.h"
#include "protocolXML2.h"
#include "readXDRFile.h"
#include "common/stringUtils.h"
#include "structures/struct.h"
#include "udaErrors.h"
#include "xdrlib.h"

using namespace uda::client_server;
using namespace uda::logging;
using namespace uda::structures;

int uda::client_server::protocol_xml(XDR* xdrs, ProtocolId protocol_id, int direction, ProtocolId* token,
                                     LogMallocList* logmalloclist, UserDefinedTypeList* userdefinedtypelist, void* str,
                                     int protocolVersion, LogStructList* log_struct_list, IoData* io_data,
                                     unsigned int private_flags, int malloc_source, CreateXDRStreams create_xdr_streams)
{
    DataBlock* data_block;

    int rc;
    int err = 0;
#ifndef FATCLIENT
    XDR XDRInput;
    XDR XDROutput;
    FILE* xdrfile = nullptr;
#endif
    bool xdr_stdio_flag = false;

    //----------------------------------------------------------------------------
    // Check Versions

    // output (ENCODE) means written by the server
    // input (DECODE) means read by the client
    // xdrs->x_op == XDR_DECODE && protocolVersion == 2 Means Client receiving data from a Version 2 Server
    // xdrs->x_op == XDR_ENCODE && protocolVersion == 3 Means Server sending data to a Version 3 Client

#ifndef FATCLIENT
    if ((xdrs->x_op == XDR_DECODE && protocolVersion < 3) || (xdrs->x_op == XDR_ENCODE && protocolVersion < 3)) {
        return 0;
    }
#endif

    //----------------------------------------------------------------------------
    // Error Management Loop

    do {

        //----------------------------------------------------------------------------
        // Generalised User Defined Data Structures

        if (protocol_id == ProtocolId::Structures) {

            void* data = nullptr;
            data_block = (DataBlock*)str;

            if (data_block->opaque_type == UDA_OPAQUE_TYPE_STRUCTURES) {

                UDA_LOG(UDA_LOG_DEBUG, "protocolXML: Compound Data Structure");
                UDA_LOG(UDA_LOG_DEBUG, "direction  : {} [{}][{}]", (int)xdrs->x_op, (int)XDR_ENCODE, (int)XDR_DECODE);

                if (xdrs->x_op == XDR_ENCODE) { // Send Data

                    SArray sarray; // Structure array carrier structure
                    SArray* psarray = &sarray;
                    int shape = data_block->data_n;                        // rank 1 array of dimension lengths
                    auto udt = (UserDefinedType*)data_block->opaque_block; // The data's structure definition
                    auto u = static_cast<UserDefinedType*>(
                        udaFindUserDefinedType(userdefinedtypelist, "SArray",
                                               0)); // Locate the carrier structure definition

                    UDA_LOG(UDA_LOG_DEBUG, "protocolXML: Sending to Client");

                    if (udt == nullptr || u == nullptr) {
                        err = 999;
                        UDA_LOG(UDA_LOG_DEBUG, "protocolXML: nullptr SArray User defined data Structure Definition");
                        print_user_defined_type_list_table(*userdefinedtypelist);
                        add_error(ErrorType::Code, "protocolXML", err,
                                  "nullptr User defined data Structure Definition");
                        break;
                    }

                    UDA_LOG(UDA_LOG_DEBUG, "protocolXML: Creating SArray carrier structure");

                    initSArray(&sarray);
                    sarray.count = data_block->data_n;     // Number of this structure
                    sarray.rank = 1;                       // Array Data Rank?
                    sarray.shape = &shape;                 // Only if rank > 1?
                    sarray.data = (void*)data_block->data; // Pointer to the data to be passed
                    strcpy(sarray.type, udt->name);        // The name of the type
                    data = (void*)&psarray;                // Pointer to the SArray array pointer
                    udaAddNonMalloc(logmalloclist, (void*)&shape, 1, sizeof(int), "int");

                    UDA_LOG(UDA_LOG_DEBUG, "protocolXML: sending Structure Definitions");

                    rc = 1;

#ifndef FATCLIENT
                    std::string temp_file = "/tmp/idamXDRXXXXXX";

                    // If access is server to server then avoid multiple write/reads of structure components over xdr
                    // by creating a temporary xdr file and passing the file. Structures need only be created in the
                    // originating client, not the intermediate server clients. Control using a global properties flag:
                    // private_flags - passed from the originating client to all servers along the chain

                    UDA_LOG(UDA_LOG_DEBUG, "protocolXML: private_flags   : {} ", private_flags);
                    UDA_LOG(UDA_LOG_DEBUG, "protocolXML: protocolVersion: {} ", protocolVersion);

                    if ((private_flags & PRIVATEFLAG_XDRFILE) && protocolVersion >= 5) {
                        char* env;
                        if ((env = getenv("UDA_WORK_DIR")) != nullptr) {
                            // File to record XDR encoded data
                            temp_file = fmt::format("{}/idamXDRXXXXXX", env);
                        }

                        // Server calling another server

                        // Create a temporary or cached XDR file

                        UDA_LOG(UDA_LOG_DEBUG, "protocolXML: creating temporary/cache XDR file");

                        errno = 0;
                        if (mkstemp(temp_file.data()) < 0 || errno != 0) {
                            err = 999;
                            if (errno != 0) {
                                err = errno;
                            }
                            add_error(ErrorType::System, "protocolXML", err,
                                      "Unable to Obtain a Temporary/Cache File Name");
                            break;
                        }
                        if ((xdrfile = fopen(temp_file.c_str(), "wb")) == nullptr) {
                            err = 999;
                            add_error(ErrorType::System, "protocolXML", err,
                                      "Unable to Open a Temporary/Cache XDR File for Writing");
                            break;
                        }

                        int packageType = UDA_PACKAGE_XDRFILE; // The package is a file
                        rc = xdr_int(xdrs, &packageType);      // Send data package type
                        rc = rc && xdrrec_endofrecord(xdrs, 1);

                        // Close current output xdr stream and create stdio file stream

                        xdr_destroy(xdrs);
                        xdr_stdio_flag = true;
                        xdrstdio_create(&XDROutput, xdrfile, XDR_ENCODE);
                        xdrs = &XDROutput; // Switch from stream to file
                        UDA_LOG(UDA_LOG_DEBUG, "protocolXML: stdio XDR file: {}", temp_file.c_str());

                    } else {
                        int packageType = UDA_PACKAGE_STRUCTDATA; // The package is regular XDR

                        UDA_LOG(UDA_LOG_DEBUG, "protocolXML: Sending Package Type: {}", packageType);

                        rc = xdr_int(xdrs, &packageType); // Send data package type
                        rc = rc && xdrrec_endofrecord(xdrs, 1);
                    }
#endif
                    // send the full set of known named structures
                    rc = rc && xdr_user_defined_type_list(xdrs, userdefinedtypelist, xdr_stdio_flag);

                    UDA_LOG(UDA_LOG_DEBUG, "protocolXML: Structure Definitions sent: rc = {}", rc);

                    // send the Data
                    rc = rc &&
                         xdr_user_defined_type_data(xdrs, logmalloclist, userdefinedtypelist, u, (void**)data,
                                                    protocolVersion, xdr_stdio_flag, log_struct_list, malloc_source);

                    UDA_LOG(UDA_LOG_DEBUG, "protocolXML: Data sent: rc = {}", rc);

                    if (!rc) {
                        err = 999;
                        add_error(ErrorType::Code, "protocolXML", err, "Bad Return Code passing data structures");
                        break;
                    }

#ifndef FATCLIENT

                    if ((private_flags & PRIVATEFLAG_XDRFILE) &&
                        protocolVersion >= 5) { // Server calling another server

                        // Close the stream and file

                        fflush(xdrfile);
                        fclose(xdrfile);

                        // Create the XDR Record Streams

                        XDR* xdr_input;
                        XDR* xdr_output;
                        std::tie(xdr_input, xdr_output) = create_xdr_streams(io_data);
                        xdrs = xdr_output;
                        xdr_stdio_flag = false;

                        // Send the Temporary File

                        UDA_LOG(UDA_LOG_DEBUG, "protocolXML: sending temporary XDR file");

                        err = send_xdr_file(xdrs, temp_file.c_str()); // Read and send
                        remove(temp_file.c_str());

                        if (err != 0) {
                            break;
                        }
                    }
#endif // !FATCLIENT

                    //======================================================================================================

                } else { // Receive Data

                    UDA_LOG(UDA_LOG_DEBUG, "protocolXML: Receiving from Server");

                    // 3 valid options:
                    //    1> unpack structures, no xdr file involved    => private_flags & PRIVATEFLAG_XDRFILE == 0 &&
                    //    packageType == PACKAGE_STRUCTDATA 2> unpack structures, from an xdr file        =>
                    //    private_flags & PRIVATEFLAG_XDRFILE == 0 && packageType == PACKAGE_XDRFILE 3> xdr file only,
                    //    no unpacking, passforward    => private_flags & PRIVATEFLAG_XDRFILE == 1 && packageType ==
                    //    PACKAGE_XDRFILE 4> Error                    => private_flags & PRIVATEFLAG_XDRFILE == 1 &&
                    //    packageType == PACKAGE_STRUCTDATA
                    //
                    // Option 3 does not include intermediate file caching - option 2 only

                    int option = 4;
                    int packageType = 0;

#ifndef FATCLIENT
                    UDA_LOG(UDA_LOG_DEBUG, "protocolXML: Receiving Package Type");

                    rc = xdrrec_skiprecord(xdrs);
                    rc = rc && xdr_int(xdrs, &packageType); // Receive data package type
#else
                    rc = 1;

                    if (private_flags & PRIVATEFLAG_XDRFILE) {
                        packageType = UDA_PACKAGE_XDRFILE;
                    } else {
                        packageType = UDA_PACKAGE_STRUCTDATA;
                    }
#endif

                    if ((private_flags & PRIVATEFLAG_XDRFILE) == 0 && packageType == UDA_PACKAGE_STRUCTDATA) {
                        option = 1;
                    } else if ((private_flags & PRIVATEFLAG_XDRFILE) == 0 && packageType == UDA_PACKAGE_XDRFILE &&
                               protocolVersion >= 5) {
                        option = 2;
                    }
                    if ((private_flags & PRIVATEFLAG_XDRFILE) == 1 && packageType == UDA_PACKAGE_XDRFILE &&
                        protocolVersion >= 5) {
                        option = 3;
                    }

                    UDA_LOG(UDA_LOG_DEBUG, "protocolXML: {}  {}", private_flags & PRIVATEFLAG_XDRFILE,
                            packageType == UDA_PACKAGE_STRUCTDATA);
                    UDA_LOG(UDA_LOG_DEBUG, "protocolXML: Receive data option : {}", option);
                    UDA_LOG(UDA_LOG_DEBUG, "protocolXML: Receive package Type: {}", packageType);

                    if (option == 4) {
                        err = 999;
                        add_error(ErrorType::System, "protocolXML", err, "Unknown package Type control option");
                        break;
                    }

                    // Read xdr file without unpacking the structures

                    char tempFile[MAXPATH] = "/tmp/idamXDRXXXXXX";

                    if (option == 3) {

                        // Create a temporary XDR file, receive and write data to the file - do not unpack data
                        // structures, pass the file onward

                        errno = 0;
                        if (mkstemp(tempFile) < 0 || errno != 0) {
                            err = 998;
                            if (errno != 0) {
                                err = errno;
                            }
                            add_error(ErrorType::System, "protocolXML", err,
                                      "Unable to Obtain a Temporary File Name [3]");
                            err = 998;
                            add_error(ErrorType::Code, "protocolXML", err, tempFile);
                            UDA_LOG(UDA_LOG_DEBUG, "Unable to Obtain a Temporary File Name [3], tempFile=[{}]",
                                    tempFile);
                            break;
                        }

                        err = receive_xdr_file(xdrs, tempFile); // Receive and write the file

                        char* fname = (char*)malloc(sizeof(char) * (strlen(tempFile) + 1));
                        strcpy(fname, tempFile);
                        data_block->data = nullptr;              // No Data - not unpacked
                        data_block->opaque_block = (void*)fname; // File name
                        data_block->opaque_type =
                            UDA_OPAQUE_TYPE_XDRFILE; // The data block is carrying the filename only
                    }

                    // Unpack data structures

                    if (option == 1 || option == 2) {
                        logmalloclist = (LogMallocList*)malloc(sizeof(LogMallocList));
                        init_log_malloc_list(logmalloclist);

                        userdefinedtypelist = (UserDefinedTypeList*)malloc(sizeof(UserDefinedTypeList));
                        auto udt_received = (UserDefinedType*)malloc(sizeof(UserDefinedType));

                        init_user_defined_type_list(userdefinedtypelist);

                        UDA_LOG(UDA_LOG_DEBUG, "protocolXML: xdr_userdefinedtypelist #A");

#ifndef FATCLIENT
                        UDA_LOG(UDA_LOG_DEBUG, "protocolXML: private_flags   : {} ", private_flags);
                        UDA_LOG(UDA_LOG_DEBUG, "protocolXML: protocolVersion: {} ", protocolVersion);

                        if (option == 2) {

                            // Create a temporary XDR file and receive data

                            UDA_LOG(UDA_LOG_DEBUG, "protocolXML: creating temporary/cached XDR file");

                            errno = 0;
                            if (mkstemp(tempFile) < 0 || errno != 0) {
                                err = 997;
                                if (errno != 0) {
                                    err = errno;
                                }
                                add_error(ErrorType::System, "protocolXML", err,
                                          " Unable to Obtain a Temporary File Name [2]");
                                err = 997;
                                add_error(ErrorType::Code, "protocolXML", err, tempFile);
                                UDA_LOG(UDA_LOG_DEBUG, "Unable to Obtain a Temporary File Name [2], tempFile=[{}]",
                                        tempFile);
                                break;
                            }

                            err = receive_xdr_file(xdrs, tempFile); // Receive and write

                            // Create input xdr file stream

                            if ((xdrfile = fopen(tempFile, "rb")) == nullptr) { // Read temporary file
                                err = 999;
                                add_error(ErrorType::System, "protocolXML", err,
                                          " Unable to Open a Temporary XDR File for Writing");
                                break;
                            }

                            xdr_destroy(xdrs);
                            xdr_stdio_flag = true;
                            xdrstdio_create(&XDRInput, xdrfile, XDR_DECODE);
                            xdrs = &XDRInput; // Switch from stream to file
                        }
#endif // !FATCLIENT
       // receive the full set of known named structures
                        rc = rc && xdr_user_defined_type_list(xdrs, userdefinedtypelist, xdr_stdio_flag);

                        UDA_LOG(UDA_LOG_DEBUG, "protocolXML: xdr_userdefinedtypelist #B");

                        if (!rc) {
                            err = 999;
                            add_error(ErrorType::Code, "protocolXML", err,
                                      "Failure receiving Structure Definitions");
                            break;
                        }
                        UDA_LOG(UDA_LOG_DEBUG, "protocolXML: udaXDRUserDefinedTypeData #A");
                        init_user_defined_type(udt_received);

                        rc = rc && xdr_user_defined_type_data(xdrs, logmalloclist, userdefinedtypelist, udt_received,
                                                              &data, protocolVersion, xdr_stdio_flag, log_struct_list,
                                                              malloc_source); // receive the Data

                        UDA_LOG(UDA_LOG_DEBUG, "protocolXML: udaXDRUserDefinedTypeData #B");
                        if (!rc) {
                            err = 999;
                            add_error(ErrorType::Code, "protocolXML", err,
                                      "Failure receiving Data and Structure Definition");
                            break;
                        }

#ifndef FATCLIENT

                        if (option == 2) {

                            // Close the stream and file

                            fflush(xdrfile);
                            fclose(xdrfile);

                            // Switch back to the normal xdr record stream

                            // xdrs = priorxdrs;

                            XDR* xdr_input;
                            XDR* xdr_output;
                            std::tie(xdr_input, xdr_output) = create_xdr_streams(io_data);
                            xdrs = xdr_input;
                            xdr_stdio_flag = false;
                            remove(tempFile);
                        }
#endif // !FATCLIENT

                        if (STR_EQUALS(udt_received->name, "SArray")) { // expecting this carrier structure

                            auto general_block = (GeneralBlock*)malloc(sizeof(GeneralBlock));

                            auto s = (SArray*)data;
                            if (s->count != data_block->data_n) { // check for consistency
                                err = 999;
                                add_error(ErrorType::Code, "protocolXML", err, "Inconsistent S Array Counts");
                                break;
                            }
                            data_block->data =
                                (char*)udaGetFullNTree(); // Global Root Node with the Carrier Structure containing data
                            data_block->opaque_block = (void*)general_block;
                            general_block->userdefinedtype = udt_received;
                            general_block->userdefinedtypelist = userdefinedtypelist;
                            general_block->logmalloclist = logmalloclist;
                            general_block->lastMallocIndex = 0;

                        } else {
                            err = 999;
                            add_error(ErrorType::Code, "protocolXML", err,
                                      "Name of Received Data Structure Incorrect");
                            break;
                        }
                    }
                }

#ifdef FATCLIENT

            } else {
                err = 999;
                add_error(ErrorType::Code, "protocolXML", err, "Unknown Opaque type");
                break;
            }
        }

#else

                //====================================================================================================================
                // Passing temporary XDR files: server to server (protocolVersion >= 5 is TRUE && packageType ==
                // PACKAGE_XDRFILE)

            } else {

                if (data_block->opaque_type == UDA_OPAQUE_TYPE_XDRFILE) {

                    if (xdrs->x_op == XDR_ENCODE) {
                        UDA_LOG(UDA_LOG_DEBUG, "protocolXML: Forwarding XDR File {}",
                                (char*)data_block->opaque_block);
                        err = send_xdr_file(xdrs, (char*)data_block->opaque_block); // Forward the xdr file
                    } else {
                        UDA_LOG(UDA_LOG_DEBUG, "protocolXML: Receiving forwarded XDR File");

                        // Create a temporary XDR file, receive and write data to the file

                        char tempFile[MAXPATH] = "/tmp/idamXDRXXXXXX";

                        errno = 0;
                        if (mkstemp(tempFile) < 0 || errno != 0) {
                            err = 996;
                            if (errno != 0) {
                                err = errno;
                            }
                            add_error(ErrorType::System, "protocolXML", err,
                                      " Unable to Obtain a Temporary File Name");
                            err = 996;
                            add_error(ErrorType::Code, "protocolXML", err, tempFile);
                            UDA_LOG(UDA_LOG_DEBUG, "Unable to Obtain a Temporary File Name, tempFile=[{}]", tempFile);
                            break;
                        }

                        err = receive_xdr_file(xdrs, tempFile); // Receive and write the file

                        if (private_flags & PRIVATEFLAG_XDRFILE) { // Forward the file (option 3) again

                            // If this is an intermediate client then read the file without unpacking the structures

                            char* fname = (char*)malloc(sizeof(char) * (strlen(tempFile) + 1));
                            strcpy(fname, tempFile);
                            data_block->data = nullptr;              // No Data - not unpacked
                            data_block->opaque_block = (void*)fname; // File name
                            data_block->opaque_type =
                                UDA_OPAQUE_TYPE_XDRFILE; // The data block is carrying the filename only

                            UDA_LOG(UDA_LOG_DEBUG, "protocolXML: Forwarding Received forwarded XDR File");
                        } else {
                            UDA_LOG(UDA_LOG_DEBUG, "protocolXML: Unpacking forwarded XDR File");

                            // Unpack the data Structures

                            logmalloclist = (LogMallocList*)malloc(sizeof(LogMallocList));
                            init_log_malloc_list(logmalloclist);

                            userdefinedtypelist = (UserDefinedTypeList*)malloc(sizeof(UserDefinedTypeList));
                            auto udt_received = (UserDefinedType*)malloc(sizeof(UserDefinedType));

                            init_user_defined_type_list(userdefinedtypelist);

                            // Create input xdr file stream

                            if ((xdrfile = fopen(tempFile, "rb")) == nullptr) { // Read temporary file
                                err = 999;
                                add_error(ErrorType::System, "protocolXML", err,
                                          " Unable to Open a Temporary XDR File for Writing");
                                break;
                            }

                            xdr_destroy(xdrs);
                            xdr_stdio_flag = true;
                            xdrstdio_create(&XDRInput, xdrfile, XDR_DECODE);
                            xdrs = &XDRInput;

                            // receive the full set of known named structures
                            rc = xdr_user_defined_type_list(xdrs, userdefinedtypelist, xdr_stdio_flag);

                            if (!rc) {
                                err = 999;
                                add_error(ErrorType::Code, "protocolXML", err,
                                          "Failure receiving Structure Definitions");
                                break;
                            }

                            init_user_defined_type(udt_received);

                            rc = rc &&
                                 xdr_user_defined_type_data(xdrs, logmalloclist, userdefinedtypelist, udt_received,
                                                            &data, protocolVersion, xdr_stdio_flag, log_struct_list,
                                                            malloc_source); // receive the Data

                            if (!rc) {
                                err = 999;
                                add_error(ErrorType::Code, "protocolXML", err,
                                          "Failure receiving Data and Structure Definition");
                                break;
                            }

                            // Close the stream and file

                            fflush(xdrfile);
                            fclose(xdrfile);

                            XDR* xdr_input;
                            XDR* xdr_output;
                            std::tie(xdr_input, xdr_output) = create_xdr_streams(io_data);
                            xdrs = xdr_input;

                            xdr_stdio_flag = false;
                            remove(tempFile);

                            // Regular client or server

                            if (STR_EQUALS(udt_received->name,
                                           "SArray")) { // expecting this carrier structure

                                auto general_block = (GeneralBlock*)malloc(sizeof(GeneralBlock));

                                auto s = (SArray*)data;
                                if (s->count != data_block->data_n) { // check for consistency
                                    err = 999;
                                    add_error(ErrorType::Code, "protocolXML", err, "Inconsistent S Array Counts");
                                    break;
                                }
                                data_block->data = (char*)
                                    udaGetFullNTree(); // Global Root Node with the Carrier Structure containing data
                                data_block->opaque_block = (void*)general_block;
                                data_block->opaque_type = UDA_OPAQUE_TYPE_STRUCTURES;
                                general_block->userdefinedtype = udt_received;
                                general_block->userdefinedtypelist = userdefinedtypelist;
                                general_block->logmalloclist = logmalloclist;
                                general_block->lastMallocIndex = 0;

                            } else {
                                err = 999;
                                add_error(ErrorType::Code, "protocolXML", err,
                                          "Name of Received Data Structure Incorrect");
                                break;
                            }
                        }
                    }
                } else {
                    err = 999;
                    add_error(ErrorType::Code, "protocolXML", err, "Unknown Opaque type");
                    break;
                }
            }
        }
#endif // !FATCLIENT

        //----------------------------------------------------------------------------
        // Meta Data XML

#ifndef FATCLIENT

        if (protocol_id == ProtocolId::Meta) {
            data_block = (DataBlock*)str;
            if (data_block->opaque_type == UDA_OPAQUE_TYPE_XML_DOCUMENT && data_block->opaque_count > 0) {
                switch (direction) {
                    case XDR_RECEIVE:
                        if (!xdrrec_skiprecord(xdrs)) {
                            err = UDA_PROTOCOL_ERROR_5;
                            break;
                        }
                        if ((data_block->opaque_block = (char*)malloc((data_block->opaque_count + 1) * sizeof(char))) ==
                            nullptr) {
                            err = 991;
                            break;
                        }
                        if (!xdr_meta(xdrs, data_block)) {
                            err = 992;
                            break;
                        }

                        break;

                    case XDR_SEND:
                        if (!xdr_meta(xdrs, data_block)) {
                            UDA_LOG(UDA_LOG_DEBUG, "Error sending Metadata XML Document: \n{}\n",
                                    (char*)data_block->opaque_block);
                            err = 990;
                            break;
                        }
                        if (!xdrrec_endofrecord(xdrs, 1)) {
                            err = UDA_PROTOCOL_ERROR_7;
                            break;
                        }
                        break;

                    case XDR_FREE_HEAP:
                        break;

                    default:
                        err = UDA_PROTOCOL_ERROR_4;
                        break;
                }
            }
        }

#endif // !FATCLIENT
       //----------------------------------------------------------------------------
       // End of Error Trap Loop

    } while (0);

    return err;
}
