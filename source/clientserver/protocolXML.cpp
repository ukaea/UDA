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
 * The DATA_BLOCK structure has the following fields used to pass and receive generalised data structures
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
 *                receiving (client side): set to the SARRAY Data Structure Definition
 *
 * The SARRAY structure has the following:
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

#include "struct.h"
#include <fmt/format.h>
#include <include/logging.h>

#include "errorLog.h"
#include "protocol.h"
#include "readXDRFile.h"
#include "stringUtils.h"
#include "udaErrors.h"
#include "xdrlib.h"

#ifdef HIERARCHICAL_DATA
#  include "allocXMLData.h"
#  include "xdrHData.h"
#  include "xmlStructs.h"
#endif

int protocolXML(XDR* xdrs, int protocol_id, int direction, int* token, LOGMALLOCLIST* logmalloclist,
                USERDEFINEDTYPELIST* userdefinedtypelist, void* str, int protocolVersion,
                LOGSTRUCTLIST* log_struct_list, IoData* io_data, unsigned int private_flags, int malloc_source,
                CreateXDRStreams create_xdr_streams)
{
    DATA_BLOCK* data_block;

#ifdef HIERARCHICAL_DATA
    PFCOILS* pfcoils;
    PFPASSIVE* pfpassive;
    PFSUPPLIES* pfsupplies;
    FLUXLOOP* fluxloop;
    MAGPROBE* magprobe;
    PFCIRCUIT* pfcircuit;
    PLASMACURRENT* plasmacurrent;
    DIAMAGNETIC* diamagnetic;
    TOROIDALFIELD* toroidalfield;
    LIMITER* limiter;
    EFIT* efit;
#endif

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

        if (protocol_id == UDA_PROTOCOL_STRUCTURES) {

            void* data = nullptr;
            data_block = (DATA_BLOCK*)str;

            if (data_block->opaque_type == UDA_OPAQUE_TYPE_STRUCTURES) {

                UDA_LOG(UDA_LOG_DEBUG, "protocolXML: Compound Data Structure\n");
                UDA_LOG(UDA_LOG_DEBUG, "direction  : %d [%d][%d]\n", (int)xdrs->x_op, XDR_ENCODE, XDR_DECODE);

                if (xdrs->x_op == XDR_ENCODE) { // Send Data

                    SARRAY sarray; // Structure array carrier structure
                    SARRAY* psarray = &sarray;
                    int shape = data_block->data_n;                        // rank 1 array of dimension lengths
                    auto udt = (USERDEFINEDTYPE*)data_block->opaque_block; // The data's structure definition
                    USERDEFINEDTYPE* u = findUserDefinedType(userdefinedtypelist, "SARRAY",
                                                             0); // Locate the carrier structure definition

                    UDA_LOG(UDA_LOG_DEBUG, "protocolXML: Sending to Client\n");

                    if (udt == nullptr || u == nullptr) {
                        err = 999;
                        UDA_LOG(UDA_LOG_DEBUG, "protocolXML: nullptr SARRAY User defined data Structure Definition\n");
                        udaPrintUserDefinedTypeListTable(*userdefinedtypelist);
                        udaAddError(UDA_CODE_ERROR_TYPE, "protocolXML", err,
                                     "nullptr User defined data Structure Definition");
                        break;
                    }

                    UDA_LOG(UDA_LOG_DEBUG, "protocolXML: Creating SARRAY carrier structure\n");

                    udaInitSArray(&sarray);
                    sarray.count = data_block->data_n;     // Number of this structure
                    sarray.rank = 1;                       // Array Data Rank?
                    sarray.shape = &shape;                 // Only if rank > 1?
                    sarray.data = (void*)data_block->data; // Pointer to the data to be passed
                    strcpy(sarray.type, udt->name);        // The name of the type
                    data = (void*)&psarray;                // Pointer to the SARRAY array pointer
                    udaAddNonMalloc(logmalloclist, (void*)&shape, 1, sizeof(int), "int");

                    UDA_LOG(UDA_LOG_DEBUG, "protocolXML: sending Structure Definitions\n");

                    rc = 1;

#ifndef FATCLIENT
                    std::string temp_file = "/tmp/idamXDRXXXXXX";

                    // If access is server to server then avoid multiple write/reads of structure components over xdr
                    // by creating a temporary xdr file and passing the file. Structures need only be created in the
                    // originating client, not the intermediate server clients. Control using a global properties flag:
                    // private_flags - passed from the originating client to all servers along the chain

                    UDA_LOG(UDA_LOG_DEBUG, "protocolXML: private_flags   : %d \n", private_flags);
                    UDA_LOG(UDA_LOG_DEBUG, "protocolXML: protocolVersion: %d \n", protocolVersion);

                    if ((private_flags & PRIVATEFLAG_XDRFILE) && protocolVersion >= 5) {
                        char* env;
                        if ((env = getenv("UDA_WORK_DIR")) != nullptr) {
                            // File to record XDR encoded data
                            temp_file = fmt::format("{}/idamXDRXXXXXX", env);
                        }

                        // Server calling another server

                        // Create a temporary or cached XDR file

                        UDA_LOG(UDA_LOG_DEBUG, "protocolXML: creating temporary/cache XDR file\n");

                        errno = 0;
                        if (mkstemp(temp_file.data()) < 0 || errno != 0) {
                            err = 999;
                            if (errno != 0) {
                                err = errno;
                            }
                            udaAddError(UDA_SYSTEM_ERROR_TYPE, "protocolXML", err,
                                         "Unable to Obtain a Temporary/Cache File Name");
                            break;
                        }
                        if ((xdrfile = fopen(temp_file.c_str(), "wb")) == nullptr) {
                            err = 999;
                            udaAddError(UDA_SYSTEM_ERROR_TYPE, "protocolXML", err,
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
                        UDA_LOG(UDA_LOG_DEBUG, "protocolXML: stdio XDR file: %s\n", temp_file.c_str());

                    } else {
                        int packageType = UDA_PACKAGE_STRUCTDATA; // The package is regular XDR

                        UDA_LOG(UDA_LOG_DEBUG, "protocolXML: Sending Package Type: %d\n", packageType);

                        rc = xdr_int(xdrs, &packageType); // Send data package type
                        rc = rc && xdrrec_endofrecord(xdrs, 1);
                    }
#endif
                    // send the full set of known named structures
                    rc = rc && xdr_userdefinedtypelist(xdrs, userdefinedtypelist, xdr_stdio_flag);

                    UDA_LOG(UDA_LOG_DEBUG, "protocolXML: Structure Definitions sent: rc = %d\n", rc);

                    // send the Data
                    rc = rc && udaXdrUserDefinedTypeData(xdrs, logmalloclist, userdefinedtypelist, u, (void**)data,
                                                      protocolVersion, xdr_stdio_flag, log_struct_list, malloc_source);

                    UDA_LOG(UDA_LOG_DEBUG, "protocolXML: Data sent: rc = %d\n", rc);

                    if (!rc) {
                        err = 999;
                        udaAddError(UDA_CODE_ERROR_TYPE, "protocolXML", err,
                                     "Bad Return Code passing data structures");
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

                        UDA_LOG(UDA_LOG_DEBUG, "protocolXML: sending temporary XDR file\n");

                        err = sendXDRFile(xdrs, temp_file.c_str()); // Read and send
                        remove(temp_file.c_str());

                        if (err != 0) {
                            break;
                        }
                    }
#endif // !FATCLIENT

                    //======================================================================================================

                } else { // Receive Data

                    UDA_LOG(UDA_LOG_DEBUG, "protocolXML: Receiving from Server\n");

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
                    UDA_LOG(UDA_LOG_DEBUG, "protocolXML: Receiving Package Type\n");

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

                    UDA_LOG(UDA_LOG_DEBUG, "protocolXML: %d  %d\n", private_flags & PRIVATEFLAG_XDRFILE,
                            packageType == UDA_PACKAGE_STRUCTDATA);
                    UDA_LOG(UDA_LOG_DEBUG, "protocolXML: Receive data option : %d\n", option);
                    UDA_LOG(UDA_LOG_DEBUG, "protocolXML: Receive package Type: %d\n", packageType);

                    if (option == 4) {
                        err = 999;
                        udaAddError(UDA_SYSTEM_ERROR_TYPE, "protocolXML", err, "Unknown package Type control option");
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
                            udaAddError(UDA_SYSTEM_ERROR_TYPE, "protocolXML", err,
                                         "Unable to Obtain a Temporary File Name [3]");
                            err = 998;
                            udaAddError(UDA_CODE_ERROR_TYPE, "protocolXML", err, tempFile);
                            UDA_LOG(UDA_LOG_DEBUG, "Unable to Obtain a Temporary File Name [3], tempFile=[%s]\n",
                                    tempFile);
                            break;
                        }

                        err = receiveXDRFile(xdrs, tempFile); // Receive and write the file

                        char* fname = (char*)malloc(sizeof(char) * (strlen(tempFile) + 1));
                        strcpy(fname, tempFile);
                        data_block->data = nullptr;              // No Data - not unpacked
                        data_block->opaque_block = (void*)fname; // File name
                        data_block->opaque_type =
                            UDA_OPAQUE_TYPE_XDRFILE; // The data block is carrying the filename only
                    }

                    // Unpack data structures

                    if (option == 1 || option == 2) {
                        logmalloclist = (LOGMALLOCLIST*)malloc(sizeof(LOGMALLOCLIST));
                        udaInitLogMallocList(logmalloclist);

                        userdefinedtypelist = (USERDEFINEDTYPELIST*)malloc(sizeof(USERDEFINEDTYPELIST));
                        auto udt_received = (USERDEFINEDTYPE*)malloc(sizeof(USERDEFINEDTYPE));

                        udaInitUserDefinedTypeList(userdefinedtypelist);

                        UDA_LOG(UDA_LOG_DEBUG, "protocolXML: xdr_userdefinedtypelist #A\n");

#ifndef FATCLIENT
                        UDA_LOG(UDA_LOG_DEBUG, "protocolXML: private_flags   : %d \n", private_flags);
                        UDA_LOG(UDA_LOG_DEBUG, "protocolXML: protocolVersion: %d \n", protocolVersion);

                        if (option == 2) {

                            // Create a temporary XDR file and receive data

                            UDA_LOG(UDA_LOG_DEBUG, "protocolXML: creating temporary/cached XDR file\n");

                            errno = 0;
                            if (mkstemp(tempFile) < 0 || errno != 0) {
                                err = 997;
                                if (errno != 0) {
                                    err = errno;
                                }
                                udaAddError(UDA_SYSTEM_ERROR_TYPE, "protocolXML", err,
                                             " Unable to Obtain a Temporary File Name [2]");
                                err = 997;
                                udaAddError(UDA_CODE_ERROR_TYPE, "protocolXML", err, tempFile);
                                UDA_LOG(UDA_LOG_DEBUG, "Unable to Obtain a Temporary File Name [2], tempFile=[%s]\n",
                                        tempFile);
                                break;
                            }

                            err = receiveXDRFile(xdrs, tempFile); // Receive and write

                            // Create input xdr file stream

                            if ((xdrfile = fopen(tempFile, "rb")) == nullptr) { // Read temporary file
                                err = 999;
                                udaAddError(UDA_SYSTEM_ERROR_TYPE, "protocolXML", err,
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
                        rc = rc && xdr_userdefinedtypelist(xdrs, userdefinedtypelist, xdr_stdio_flag);

                        UDA_LOG(UDA_LOG_DEBUG, "protocolXML: xdr_userdefinedtypelist #B\n");

                        if (!rc) {
                            err = 999;
                            udaAddError(UDA_CODE_ERROR_TYPE, "protocolXML", err,
                                         "Failure receiving Structure Definitions");
                            break;
                        }
                        UDA_LOG(UDA_LOG_DEBUG, "protocolXML: udaXdrUserDefinedTypeData #A\n");
                        udaInitUserDefinedType(udt_received);

                        rc = rc && udaXdrUserDefinedTypeData(xdrs, logmalloclist, userdefinedtypelist, udt_received, &data,
                                                          protocolVersion, xdr_stdio_flag, log_struct_list,
                                                          malloc_source); // receive the Data

                        UDA_LOG(UDA_LOG_DEBUG, "protocolXML: udaXdrUserDefinedTypeData #B\n");
                        if (!rc) {
                            err = 999;
                            udaAddError(UDA_CODE_ERROR_TYPE, "protocolXML", err,
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

                        if (STR_EQUALS(udt_received->name, "SARRAY")) { // expecting this carrier structure

                            auto general_block = (GENERAL_BLOCK*)malloc(sizeof(GENERAL_BLOCK));

                            auto s = (SARRAY*)data;
                            if (s->count != data_block->data_n) { // check for consistency
                                err = 999;
                                udaAddError(UDA_CODE_ERROR_TYPE, "protocolXML", err, "Inconsistent S Array Counts");
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
                            udaAddError(UDA_CODE_ERROR_TYPE, "protocolXML", err,
                                         "Name of Received Data Structure Incorrect");
                            break;
                        }
                    }
                }

#ifdef FATCLIENT

            } else {
                err = 999;
                udaAddError(UDA_CODE_ERROR_TYPE, "protocolXML", err, "Unknown Opaque type");
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
                        UDA_LOG(UDA_LOG_DEBUG, "protocolXML: Forwarding XDR File %s\n",
                                (char*)data_block->opaque_block);
                        err = sendXDRFile(xdrs, (char*)data_block->opaque_block); // Forward the xdr file
                    } else {
                        UDA_LOG(UDA_LOG_DEBUG, "protocolXML: Receiving forwarded XDR File\n");

                        // Create a temporary XDR file, receive and write data to the file

                        char tempFile[MAXPATH] = "/tmp/idamXDRXXXXXX";

                        errno = 0;
                        if (mkstemp(tempFile) < 0 || errno != 0) {
                            err = 996;
                            if (errno != 0) {
                                err = errno;
                            }
                            udaAddError(UDA_SYSTEM_ERROR_TYPE, "protocolXML", err,
                                         " Unable to Obtain a Temporary File Name");
                            err = 996;
                            udaAddError(UDA_CODE_ERROR_TYPE, "protocolXML", err, tempFile);
                            UDA_LOG(UDA_LOG_DEBUG, "Unable to Obtain a Temporary File Name, tempFile=[%s]\n", tempFile);
                            break;
                        }

                        err = receiveXDRFile(xdrs, tempFile); // Receive and write the file

                        if (private_flags & PRIVATEFLAG_XDRFILE) { // Forward the file (option 3) again

                            // If this is an intermediate client then read the file without unpacking the structures

                            char* fname = (char*)malloc(sizeof(char) * (strlen(tempFile) + 1));
                            strcpy(fname, tempFile);
                            data_block->data = nullptr;              // No Data - not unpacked
                            data_block->opaque_block = (void*)fname; // File name
                            data_block->opaque_type =
                                UDA_OPAQUE_TYPE_XDRFILE; // The data block is carrying the filename only

                            UDA_LOG(UDA_LOG_DEBUG, "protocolXML: Forwarding Received forwarded XDR File\n");
                        } else {
                            UDA_LOG(UDA_LOG_DEBUG, "protocolXML: Unpacking forwarded XDR File\n");

                            // Unpack the data Structures

                            logmalloclist = (LOGMALLOCLIST*)malloc(sizeof(LOGMALLOCLIST));
                            udaInitLogMallocList(logmalloclist);

                            userdefinedtypelist = (USERDEFINEDTYPELIST*)malloc(sizeof(USERDEFINEDTYPELIST));
                            auto udt_received = (USERDEFINEDTYPE*)malloc(sizeof(USERDEFINEDTYPE));

                            udaInitUserDefinedTypeList(userdefinedtypelist);

                            // Create input xdr file stream

                            if ((xdrfile = fopen(tempFile, "rb")) == nullptr) { // Read temporary file
                                err = 999;
                                udaAddError(UDA_SYSTEM_ERROR_TYPE, "protocolXML", err,
                                             " Unable to Open a Temporary XDR File for Writing");
                                break;
                            }

                            xdr_destroy(xdrs);
                            xdr_stdio_flag = true;
                            xdrstdio_create(&XDRInput, xdrfile, XDR_DECODE);
                            xdrs = &XDRInput;

                            // receive the full set of known named structures
                            rc = xdr_userdefinedtypelist(xdrs, userdefinedtypelist, xdr_stdio_flag);

                            if (!rc) {
                                err = 999;
                                udaAddError(UDA_CODE_ERROR_TYPE, "protocolXML", err,
                                             "Failure receiving Structure Definitions");
                                break;
                            }

                            udaInitUserDefinedType(udt_received);

                            rc = rc && udaXdrUserDefinedTypeData(xdrs, logmalloclist, userdefinedtypelist, udt_received,
                                                              &data, protocolVersion, xdr_stdio_flag, log_struct_list,
                                                              malloc_source); // receive the Data

                            if (!rc) {
                                err = 999;
                                udaAddError(UDA_CODE_ERROR_TYPE, "protocolXML", err,
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
                                           "SARRAY")) { // expecting this carrier structure

                                auto general_block = (GENERAL_BLOCK*)malloc(sizeof(GENERAL_BLOCK));

                                auto s = (SARRAY*)data;
                                if (s->count != data_block->data_n) { // check for consistency
                                    err = 999;
                                    udaAddError(UDA_CODE_ERROR_TYPE, "protocolXML", err,
                                                 "Inconsistent S Array Counts");
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
                                udaAddError(UDA_CODE_ERROR_TYPE, "protocolXML", err,
                                             "Name of Received Data Structure Incorrect");
                                break;
                            }
                        }
                    }
                } else {
                    err = 999;
                    udaAddError(UDA_CODE_ERROR_TYPE, "protocolXML", err, "Unknown Opaque type");
                    break;
                }
            }
        }
#endif // !FATCLIENT

        //----------------------------------------------------------------------------
        // Meta Data XML

#ifndef FATCLIENT

        if (protocol_id == UDA_PROTOCOL_META) {
            data_block = (DATA_BLOCK*)str;
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
                            UDA_LOG(UDA_LOG_DEBUG, "Error sending Metadata XML Document: \n%s\n\n",
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

#  ifdef HIERARCHICAL_DATA
        //----------------------------------------------------------------------------
        // EFIT (Uses recursion)

        if (protocol_id == UDA_PROTOCOL_EFIT) {

            data_block = (DATA_BLOCK*)str;

            switch (direction) {

                case XDR_RECEIVE:

                    if ((efit = (EFIT*)malloc(sizeof(EFIT))) == nullptr) {
                        err = UDA_PROTOCOL_ERROR_1001;
                        break;
                    }

                    if (!(rc = xdrrec_skiprecord(xdrs))) {
                        err = UDA_PROTOCOL_ERROR_5;
                        break;
                    }
                    if (!(rc = xdr_efit(xdrs, efit))) {
                        err = UDA_PROTOCOL_ERROR_1001;
                        break;
                    }

                    if ((err = alloc_efit(efit)) != 0) {
                        break; // Allocate Heap Memory
                    }

                    for (int i = 0; i < efit->npfcoils; i++) {
                        pfcoils = efit->pfcoils + i;
                        if ((err = protocolXML(xdrs, UDA_PROTOCOL_PFCOILS, direction, token, logmalloclist,
                                               userdefinedtypelist, (void*)pfcoils)) != 0) {
                            break;
                        }
                    }
                    for (int i = 0; i < efit->npfpassive; i++) {
                        pfpassive = efit->pfpassive + i;
                        if ((err = protocolXML(xdrs, UDA_PROTOCOL_PFPASSIVE, direction, token, logmalloclist,
                                               userdefinedtypelist, (void*)pfpassive)) != 0) {
                            break;
                        }
                    }
                    for (int i = 0; i < efit->npfsupplies; i++) {
                        pfsupplies = efit->pfsupplies + i;
                        if ((err = protocolXML(xdrs, UDA_PROTOCOL_PFSUPPLIES, direction, token, logmalloclist,
                                               userdefinedtypelist, (void*)pfsupplies)) != 0) {
                            break;
                        }
                    }
                    for (int i = 0; i < efit->nfluxloops; i++) {
                        fluxloop = efit->fluxloop + i;
                        if ((err = protocolXML(xdrs, UDA_PROTOCOL_FLUXLOOP, direction, token, logmalloclist,
                                               userdefinedtypelist, (void*)fluxloop)) != 0) {
                            break;
                        }
                    }
                    for (int i = 0; i < efit->nmagprobes; i++) {
                        magprobe = efit->magprobe + i;
                        if ((err = protocolXML(xdrs, UDA_PROTOCOL_MAGPROBE, direction, token, logmalloclist,
                                               userdefinedtypelist, (void*)magprobe)) != 0) {
                            break;
                        }
                    }
                    for (int i = 0; i < efit->npfcircuits; i++) {
                        pfcircuit = efit->pfcircuit + i;
                        if ((err = protocolXML(xdrs, UDA_PROTOCOL_PFCIRCUIT, direction, token, logmalloclist,
                                               userdefinedtypelist, (void*)pfcircuit)) != 0) {
                            break;
                        }
                    }
                    for (int i = 0; i < efit->nplasmacurrent; i++) {
                        plasmacurrent = efit->plasmacurrent + i;
                        if ((err = protocolXML(xdrs, UDA_PROTOCOL_PLASMACURRENT, direction, token, logmalloclist,
                                               userdefinedtypelist, (void*)plasmacurrent)) != 0) {
                            break;
                        }
                    }
                    for (int i = 0; i < efit->ndiamagnetic; i++) {
                        diamagnetic = efit->diamagnetic + i;
                        if ((err = protocolXML(xdrs, UDA_PROTOCOL_DIAMAGNETIC, direction, token, logmalloclist,
                                               userdefinedtypelist, (void*)diamagnetic)) != 0) {
                            break;
                        }
                    }
                    for (int i = 0; i < efit->ntoroidalfield; i++) {
                        toroidalfield = efit->toroidalfield + i;
                        if ((err = protocolXML(xdrs, UDA_PROTOCOL_TOROIDALFIELD, direction, token, logmalloclist,
                                               userdefinedtypelist, (void*)toroidalfield)) != 0) {
                            break;
                        }
                    }
                    for (int i = 0; i < efit->nlimiter; i++) {
                        limiter = efit->limiter + i;
                        if ((err = protocolXML(xdrs, UDA_PROTOCOL_LIMITER, direction, token, logmalloclist,
                                               userdefinedtypelist, (void*)limiter)) != 0) {
                            break;
                        }
                    }

                    data_block->opaque_block = (void*)efit;

                    break;

                case XDR_SEND:

                    efit = (EFIT*)data_block->opaque_block;

                    if (!(rc = xdr_efit(xdrs, efit))) {
                        err = UDA_PROTOCOL_ERROR_1001;
                        break;
                    }

                    if (!(rc = xdrrec_endofrecord(xdrs, 1))) {
                        err = UDA_PROTOCOL_ERROR_7;
                    }

                    for (int i = 0; i < efit->npfcoils; i++) {
                        pfcoils = efit->pfcoils + i;
                        if ((err = protocolXML(xdrs, UDA_PROTOCOL_PFCOILS, direction, token, logmalloclist,
                                               userdefinedtypelist, (void*)pfcoils)) != 0) {
                            break;
                        }
                    }
                    for (int i = 0; i < efit->npfpassive; i++) {
                        pfpassive = efit->pfpassive + i;
                        if ((err = protocolXML(xdrs, UDA_PROTOCOL_PFPASSIVE, direction, token, logmalloclist,
                                               userdefinedtypelist, (void*)pfpassive)) != 0) {
                            break;
                        }
                    }
                    for (int i = 0; i < efit->npfsupplies; i++) {
                        pfsupplies = efit->pfsupplies + i;
                        if ((err = protocolXML(xdrs, UDA_PROTOCOL_PFSUPPLIES, direction, token, logmalloclist,
                                               userdefinedtypelist, (void*)pfsupplies)) != 0) {
                            break;
                        }
                    }
                    for (int i = 0; i < efit->nfluxloops; i++) {
                        fluxloop = efit->fluxloop + i;
                        if ((err = protocolXML(xdrs, UDA_PROTOCOL_FLUXLOOP, direction, token, logmalloclist,
                                               userdefinedtypelist, (void*)fluxloop)) != 0) {
                            break;
                        }
                    }
                    for (int i = 0; i < efit->nmagprobes; i++) {
                        magprobe = efit->magprobe + i;
                        if ((err = protocolXML(xdrs, UDA_PROTOCOL_MAGPROBE, direction, token, logmalloclist,
                                               userdefinedtypelist, (void*)magprobe)) != 0) {
                            break;
                        }
                    }
                    for (int i = 0; i < efit->npfcircuits; i++) {
                        pfcircuit = efit->pfcircuit + i;
                        if ((err = protocolXML(xdrs, UDA_PROTOCOL_PFCIRCUIT, direction, token, logmalloclist,
                                               userdefinedtypelist, (void*)pfcircuit)) != 0) {
                            break;
                        }
                    }
                    for (int i = 0; i < efit->nplasmacurrent; i++) {
                        plasmacurrent = efit->plasmacurrent + i;
                        if ((err = protocolXML(xdrs, UDA_PROTOCOL_PLASMACURRENT, direction, token, logmalloclist,
                                               userdefinedtypelist, (void*)plasmacurrent)) != 0) {
                            break;
                        }
                    }
                    for (int i = 0; i < efit->ndiamagnetic; i++) {
                        diamagnetic = efit->diamagnetic + i;
                        if ((err = protocolXML(xdrs, UDA_PROTOCOL_DIAMAGNETIC, direction, token, logmalloclist,
                                               userdefinedtypelist, (void*)diamagnetic)) != 0) {
                            break;
                        }
                    }
                    for (int i = 0; i < efit->ntoroidalfield; i++) {
                        toroidalfield = efit->toroidalfield + i;
                        if ((err = protocolXML(xdrs, UDA_PROTOCOL_TOROIDALFIELD, direction, token, logmalloclist,
                                               userdefinedtypelist, (void*)toroidalfield)) != 0) {
                            break;
                        }
                    }
                    for (int i = 0; i < efit->nlimiter; i++) {
                        limiter = efit->limiter + i;
                        if ((err = protocolXML(xdrs, UDA_PROTOCOL_LIMITER, direction, token, logmalloclist,
                                               userdefinedtypelist, (void*)limiter)) != 0) {
                            break;
                        }
                    }

                    break;

                case XDR_FREE_HEAP:
                    break;

                default:
                    err = UDA_PROTOCOL_ERROR_4;
                    break;
            }

            break;
        }

        //----------------------------------------------------------------------------
        // PF Coils

        if (protocol_id == UDA_PROTOCOL_PFCOILS) {

            pfcoils = (PFCOILS*)str;

            switch (direction) {

                case XDR_RECEIVE:

                    if (!(rc = xdrrec_skiprecord(xdrs))) {
                        err = UDA_PROTOCOL_ERROR_5;
                        break;
                    }
                    if (!(rc = xdr_pfcoils1(xdrs, pfcoils))) {
                        err = UDA_PROTOCOL_ERROR_1011;
                        break;
                    }

                    if (pfcoils->nco == 0) {
                        break; // No Data to Receive!
                    }

                    if ((err = alloc_pfcoils(pfcoils)) != 0) {
                        break; // Allocate Heap Memory
                    }

                    if (!(rc = xdr_pfcoils2(xdrs, pfcoils))) {
                        err = UDA_PROTOCOL_ERROR_1012;
                        break;
                    }

                    break;

                case XDR_SEND:

                    if (!(rc = xdr_pfcoils1(xdrs, pfcoils))) {
                        err = UDA_PROTOCOL_ERROR_1011;
                        break;
                    }

                    if (pfcoils->nco == 0) {
                        break; // No Data to Send!
                    }

                    if (!(rc = xdr_pfcoils2(xdrs, pfcoils))) {
                        err = UDA_PROTOCOL_ERROR_1012;
                        break;
                    }

                    if (!(rc = xdrrec_endofrecord(xdrs, 1))) {
                        err = UDA_PROTOCOL_ERROR_7;
                    }

                    break;

                case XDR_FREE_HEAP:
                    break;

                default:
                    err = UDA_PROTOCOL_ERROR_4;
                    break;
            }

            break;
        }

        //----------------------------------------------------------------------------
        // PF Passive

        if (protocol_id == UDA_PROTOCOL_PFPASSIVE) {

            pfpassive = (PFPASSIVE*)str;

            switch (direction) {

                case XDR_RECEIVE:

                    if (!(rc = xdrrec_skiprecord(xdrs))) {
                        err = UDA_PROTOCOL_ERROR_5;
                        break;
                    }
                    if (!(rc = xdr_pfpassive1(xdrs, pfpassive))) {
                        err = UDA_PROTOCOL_ERROR_1021;
                        break;
                    }

                    if (pfpassive->nco == 0) {
                        break; // No Data to Receive!
                    }

                    if ((err = alloc_pfpassive(pfpassive)) != 0) {
                        break; // Allocate Heap Memory
                    }

                    if (!(rc = xdr_pfpassive2(xdrs, pfpassive))) {
                        err = UDA_PROTOCOL_ERROR_1022;
                        break;
                    }

                    break;

                case XDR_SEND:

                    if (!(rc = xdr_pfpassive1(xdrs, pfpassive))) {
                        err = UDA_PROTOCOL_ERROR_1021;
                        break;
                    }

                    if (pfpassive->nco == 0) {
                        break; // No Data to Send!
                    }

                    if (!(rc = xdr_pfpassive2(xdrs, pfpassive))) {
                        err = UDA_PROTOCOL_ERROR_1022;
                        break;
                    }

                    if (!(rc = xdrrec_endofrecord(xdrs, 1))) {
                        err = UDA_PROTOCOL_ERROR_7;
                    }

                    break;

                case XDR_FREE_HEAP:
                    break;

                default:
                    err = UDA_PROTOCOL_ERROR_4;
                    break;
            }

            break;
        }

        //----------------------------------------------------------------------------
        // PF Supplies

        if (protocol_id == UDA_PROTOCOL_PFSUPPLIES) {

            pfsupplies = (PFSUPPLIES*)str;

            switch (direction) {

                case XDR_RECEIVE:

                    if (!(rc = xdrrec_skiprecord(xdrs))) {
                        err = UDA_PROTOCOL_ERROR_5;
                        break;
                    }
                    if (!(rc = xdr_pfsupplies(xdrs, pfsupplies))) {
                        err = UDA_PROTOCOL_ERROR_1031;
                        break;
                    }

                    break;

                case XDR_SEND:

                    if (!(rc = xdr_pfsupplies(xdrs, pfsupplies))) {
                        err = UDA_PROTOCOL_ERROR_1031;
                        break;
                    }

                    if (!(rc = xdrrec_endofrecord(xdrs, 1))) {
                        err = UDA_PROTOCOL_ERROR_7;
                    }

                    break;

                case XDR_FREE_HEAP:
                    break;

                default:
                    err = UDA_PROTOCOL_ERROR_4;
                    break;
            }

            break;
        }

        //----------------------------------------------------------------------------
        // Flux Loops

        if (protocol_id == UDA_PROTOCOL_FLUXLOOP) {

            fluxloop = (FLUXLOOP*)str;

            switch (direction) {

                case XDR_RECEIVE:

                    if (!(rc = xdrrec_skiprecord(xdrs))) {
                        err = UDA_PROTOCOL_ERROR_5;
                        break;
                    }
                    if (!(rc = xdr_fluxloop1(xdrs, fluxloop))) {
                        err = UDA_PROTOCOL_ERROR_1041;
                        break;
                    }

                    if (fluxloop->nco == 0) {
                        break; // No Data to Receive!
                    }

                    if ((err = alloc_fluxloop(fluxloop)) != 0) {
                        break; // Allocate Heap Memory
                    }

                    if (!(rc = xdr_fluxloop2(xdrs, fluxloop))) {
                        err = UDA_PROTOCOL_ERROR_1042;
                        break;
                    }

                    break;

                case XDR_SEND:

                    if (!(rc = xdr_fluxloop1(xdrs, fluxloop))) {
                        err = UDA_PROTOCOL_ERROR_1041;
                        break;
                    }

                    if (fluxloop->nco == 0) {
                        break;
                    }

                    if (!(rc = xdr_fluxloop2(xdrs, fluxloop))) {
                        err = UDA_PROTOCOL_ERROR_1042;
                        break;
                    }

                    if (!(rc = xdrrec_endofrecord(xdrs, 1))) {
                        err = UDA_PROTOCOL_ERROR_7;
                    }

                    break;

                case XDR_FREE_HEAP:
                    break;

                default:
                    err = UDA_PROTOCOL_ERROR_4;
                    break;
            }

            break;
        }

        //----------------------------------------------------------------------------
        // Magnetic Probe

        if (protocol_id == UDA_PROTOCOL_MAGPROBE) {

            magprobe = (MAGPROBE*)str;

            switch (direction) {

                case XDR_RECEIVE:

                    if (!(rc = xdrrec_skiprecord(xdrs))) {
                        err = UDA_PROTOCOL_ERROR_5;
                        break;
                    }
                    if (!(rc = xdr_magprobe(xdrs, magprobe))) {
                        err = UDA_PROTOCOL_ERROR_1051;
                        break;
                    }

                    break;

                case XDR_SEND:

                    if (!(rc = xdr_magprobe(xdrs, magprobe))) {
                        err = UDA_PROTOCOL_ERROR_1051;
                        break;
                    }

                    if (!(rc = xdrrec_endofrecord(xdrs, 1))) {
                        err = UDA_PROTOCOL_ERROR_7;
                    }

                    break;

                case XDR_FREE_HEAP:
                    break;

                default:
                    err = UDA_PROTOCOL_ERROR_4;
                    break;
            }

            break;
        }

        //----------------------------------------------------------------------------
        // PF Circuit

        if (protocol_id == UDA_PROTOCOL_PFCIRCUIT) {

            pfcircuit = (PFCIRCUIT*)str;

            switch (direction) {

                case XDR_RECEIVE:

                    if (!(rc = xdrrec_skiprecord(xdrs))) {
                        err = UDA_PROTOCOL_ERROR_5;
                        break;
                    }
                    if (!(rc = xdr_pfcircuit1(xdrs, pfcircuit))) {
                        err = UDA_PROTOCOL_ERROR_1061;
                        break;
                    }

                    if (pfcircuit->nco == 0) {
                        break; // No Data to Receive!
                    }

                    if ((err = alloc_pfcircuit(pfcircuit)) != 0) {
                        break; // Allocate Heap Memory
                    }

                    if (!(rc = xdr_pfcircuit2(xdrs, pfcircuit))) {
                        err = UDA_PROTOCOL_ERROR_1062;
                        break;
                    }

                    break;

                case XDR_SEND:

                    if (!(rc = xdr_pfcircuit1(xdrs, pfcircuit))) {
                        err = UDA_PROTOCOL_ERROR_1061;
                        break;
                    }

                    if (pfcircuit->nco == 0) {
                        break; // No Data to Send!
                    }

                    if (!(rc = xdr_pfcircuit2(xdrs, pfcircuit))) {
                        err = UDA_PROTOCOL_ERROR_1062;
                        break;
                    }

                    if (!(rc = xdrrec_endofrecord(xdrs, 1))) {
                        err = UDA_PROTOCOL_ERROR_7;
                    }

                    break;

                case XDR_FREE_HEAP:
                    break;

                default:
                    err = UDA_PROTOCOL_ERROR_4;
                    break;
            }

            break;
        }

        //----------------------------------------------------------------------------
        // Plasma Current

        if (protocol_id == UDA_PROTOCOL_PLASMACURRENT) {

            plasmacurrent = (PLASMACURRENT*)str;

            switch (direction) {

                case XDR_RECEIVE:

                    if (!(rc = xdrrec_skiprecord(xdrs))) {
                        err = UDA_PROTOCOL_ERROR_5;
                        break;
                    }
                    if (!(rc = xdr_plasmacurrent(xdrs, plasmacurrent))) {
                        err = UDA_PROTOCOL_ERROR_1071;
                        break;
                    }

                    break;

                case XDR_SEND:

                    if (!(rc = xdr_plasmacurrent(xdrs, plasmacurrent))) {
                        err = UDA_PROTOCOL_ERROR_1071;
                        break;
                    }

                    if (!(rc = xdrrec_endofrecord(xdrs, 1))) {
                        err = UDA_PROTOCOL_ERROR_7;
                    }

                    break;

                case XDR_FREE_HEAP:
                    break;

                default:
                    err = UDA_PROTOCOL_ERROR_4;
                    break;
            }

            break;
        }

        //----------------------------------------------------------------------------
        // Diamagnetic Flux

        if (protocol_id == UDA_PROTOCOL_DIAMAGNETIC) {

            diamagnetic = (DIAMAGNETIC*)str;

            switch (direction) {

                case XDR_RECEIVE:

                    if (!(rc = xdrrec_skiprecord(xdrs))) {
                        err = UDA_PROTOCOL_ERROR_5;
                        break;
                    }
                    if (!(rc = xdr_diamagnetic(xdrs, diamagnetic))) {
                        err = UDA_PROTOCOL_ERROR_1071;
                        break;
                    }

                    break;

                case XDR_SEND:

                    if (!(rc = xdr_diamagnetic(xdrs, diamagnetic))) {
                        err = UDA_PROTOCOL_ERROR_1071;
                        break;
                    }

                    if (!(rc = xdrrec_endofrecord(xdrs, 1))) {
                        err = UDA_PROTOCOL_ERROR_7;
                    }

                    break;

                case XDR_FREE_HEAP:
                    break;

                default:
                    err = UDA_PROTOCOL_ERROR_4;
                    break;
            }

            break;
        }

        //----------------------------------------------------------------------------
        // Toroidal Magnetic Field

        if (protocol_id == UDA_PROTOCOL_TOROIDALFIELD) {

            toroidalfield = (TOROIDALFIELD*)str;

            switch (direction) {

                case XDR_RECEIVE:

                    if (!(rc = xdrrec_skiprecord(xdrs))) {
                        err = UDA_PROTOCOL_ERROR_5;
                        break;
                    }
                    if (!(rc = xdr_toroidalfield(xdrs, toroidalfield))) {
                        err = UDA_PROTOCOL_ERROR_1081;
                        break;
                    }

                    break;

                case XDR_SEND:

                    if (!(rc = xdr_toroidalfield(xdrs, toroidalfield))) {
                        err = UDA_PROTOCOL_ERROR_1081;
                        break;
                    }

                    if (!(rc = xdrrec_endofrecord(xdrs, 1))) {
                        err = UDA_PROTOCOL_ERROR_7;
                    }

                    break;

                case XDR_FREE_HEAP:
                    break;

                default:
                    err = UDA_PROTOCOL_ERROR_4;
                    break;
            }

            break;
        }

        //----------------------------------------------------------------------------
        // Limiters

        if (protocol_id == UDA_PROTOCOL_LIMITER) {

            limiter = (LIMITER*)str;

            switch (direction) {

                case XDR_RECEIVE:

                    if (!(rc = xdrrec_skiprecord(xdrs))) {
                        err = UDA_PROTOCOL_ERROR_5;
                        break;
                    }
                    if (!(rc = xdr_limiter1(xdrs, limiter))) {
                        err = UDA_PROTOCOL_ERROR_1091;
                        break;
                    }

                    if (limiter->nco == 0) {
                        break; // No Data to Receive!
                    }

                    if ((err = alloc_limiter(limiter)) != 0) {
                        break; // Allocate Heap Memory
                    }

                    if (!(rc = xdr_limiter2(xdrs, limiter))) {
                        err = UDA_PROTOCOL_ERROR_1092;
                        break;
                    }

                    break;

                case XDR_SEND:

                    if (!(rc = xdr_limiter1(xdrs, limiter))) {
                        err = UDA_PROTOCOL_ERROR_1091;
                        break;
                    }

                    if (limiter->nco == 0) {
                        break; // No Data to Send!
                    }

                    if (!(rc = xdr_limiter2(xdrs, limiter))) {
                        err = UDA_PROTOCOL_ERROR_1092;
                        break;
                    }

                    if (!(rc = xdrrec_endofrecord(xdrs, 1))) {
                        err = UDA_PROTOCOL_ERROR_7;
                    }

                    break;

                case XDR_FREE_HEAP:
                    break;

                default:
                    err = UDA_PROTOCOL_ERROR_4;
                    break;
            }

            break;
        }
#  endif
#endif // !FATCLIENT
       //----------------------------------------------------------------------------
       // End of Error Trap Loop

    } while (0);

    return err;
}
