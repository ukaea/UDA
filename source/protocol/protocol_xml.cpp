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
 *---------------------------------------------------------------------------------------------------------
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
#include "protocol_xml.h"

#include <cerrno>
#include <cstdlib>

#include "clientserver/error_log.h"
#include "common/string_utils.h"
#include "logging/logging.h"
#include "structures/struct.h"

#include "protocol.h"
#include "read_xdr_file.h"
#include "xdr_lib.h"

#ifdef SERVERBUILD
#  include "server/createXDRStream.h"
#  include "server/serverStartup.h"
#  include "server/udaServer.h"
#endif

#ifndef FATCLIENT
#  include "clientserver/uda_errors.h"
#endif

#include <fmt/format.h>
#include <openssl/sha.h>

using namespace uda::client_server;
using namespace uda::structures;
using namespace uda::logging;
using namespace uda::protocol;

extern "C" {

void sha1Block(unsigned char* block, size_t blockSize, unsigned char* md);

int sha1File(char* name, FILE* fh, unsigned char* md);
}

constexpr int MaxElementSha1 = 20;

struct FCloseDeleter {
    void operator()(FILE* file) const {fclose(file);}
};

int uda::protocol::protocol_xml2(std::vector<UdaError>& error_stack, XDR* xdrs, ProtocolId protocol_id, XDRStreamDirection direction, ProtocolId* token,
                                      LogMallocList* logmalloclist, UserDefinedTypeList* userdefinedtypelist, void* str,
                                      int protocolVersion, LogStructList* log_struct_list, unsigned int private_flags,
                                      int malloc_source)
{
    DataBlock* data_block;

    int rc, err = 0, count = 0;

#ifndef FATCLIENT
    XDR xdr_input; // stdio xdr files
    XDR xdr_output;
#endif
    bool xdr_stdio_flag = false;
    std::unique_ptr<FILE, FCloseDeleter> xdrfile = {};

    XDR* priorxdrs = xdrs; // Preserve the current stream object

    std::string temp_file = "/tmp/idamXDRXXXXXX";
    char* env = nullptr;

    unsigned char md[MaxElementSha1 + 1]; // SHA1 Hash
    md[MaxElementSha1] = '\0';
    strcpy((char*)md, "12345678901234567890");
    int hashSize = MaxElementSha1;
    unsigned char mdr[MaxElementSha1]; // SHA1 Hash of data received

    if ((private_flags & private_flags::XdrFile) && protocolVersion >= 5) { // Intermediate XDR File, not stream
        if ((env = getenv("UDA_WORK_DIR")) != nullptr) {
            // File to record XDR encoded data
            temp_file = fmt::format("{}/idamXDRXXXXXX", env);
        }
    }

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

            unsigned char* object = nullptr;
            size_t objectSize = 0;

            if (data_block->opaque_type == UDA_OPAQUE_TYPE_STRUCTURES) {
                // These can be transformed into different opaque types to simplify sending
                int packageType = 0;

                UDA_LOG(UDA_LOG_DEBUG, "Compound Data Structure");
                UDA_LOG(UDA_LOG_DEBUG, "direction  : {} [{}][{}]", (int)xdrs->x_op, (int)XDR_ENCODE, (int)XDR_DECODE);

                if (xdrs->x_op == XDR_ENCODE) { // Send Data

                    SArray sarray; // Structure array carrier structure
                    SArray* psarray = &sarray;
                    int shape = data_block->data_n;                        // rank 1 array of dimension lengths
                    auto udt = (UserDefinedType*)data_block->opaque_block; // The data's structure definition
                    // Locate the carrier structure definition
                    UserDefinedType* u =
                        static_cast<UserDefinedType*>(find_user_defined_type(userdefinedtypelist, "SArray", 0));

                    UDA_LOG(UDA_LOG_DEBUG, "Sending to Client");

                    if (udt == nullptr || u == nullptr) {
                        err = 999;
                        UDA_LOG(UDA_LOG_DEBUG, "nullptr SArray User defined data Structure Definition");
                        print_user_defined_type_list_table(*userdefinedtypelist);
                        add_error(error_stack, ErrorType::Code, "protocolXML", err,
                                  "nullptr User defined data Structure Definition");
                        break;
                    }

                    UDA_LOG(UDA_LOG_DEBUG, "Creating SArray carrier structure");

                    initSArray(&sarray);
                    sarray.count = data_block->data_n;     // Number of this structure
                    sarray.rank = 1;                       // Array Data Rank?
                    sarray.shape = &shape;                 // Only if rank > 1?
                    sarray.data = (void*)data_block->data; // Pointer to the data to be passed
                    strcpy(sarray.type, udt->name);        // The name of the type
                    data = (void*)&psarray;                // Pointer to the SArray array pointer
                    add_non_malloc(logmalloclist, (void*)&shape, 1, sizeof(int), "int");

                    UDA_LOG(UDA_LOG_DEBUG, "sending Structure Definitions");

                    rc = 1;

#ifndef FATCLIENT

                    // If access is server to server then avoid multiple write/reads of structure components over xdr by
                    // creating a temporary xdr file or xdr object and passing the file or object. Structures need only
                    // be created in the originating client, not the intermediate server clients. Control using a global
                    // properties flag: private_flags - passed from the originating client to all servers along the
                    // chain

                    UDA_LOG(UDA_LOG_DEBUG, "private_flags   : {} ", private_flags);
                    UDA_LOG(UDA_LOG_DEBUG, "protocolVersion: {} ", protocolVersion);

                    if ((private_flags & private_flags::XdrFile) &&
                        protocolVersion >= 5) { // Server calling another server

                        // Create a temporary or cached XDR file
                        // Record name and location in MEMCACHE

                        UDA_LOG(UDA_LOG_DEBUG, "creating temporary/cache XDR file");

                        errno = 0;
                        if (mkstemp(temp_file.data()) < 0 || errno != 0) {
                            err = 999;
                            if (errno != 0) {
                                err = errno;
                            }
                            add_error(error_stack, ErrorType::System, "protocolXML", err,
                                      " Unable to Obtain a Temporary/Cache File Name");
                            break;
                        }
                        xdrfile = std::unique_ptr<FILE, FCloseDeleter>{fopen(temp_file.c_str(), "wb")};
                        if (!xdrfile) {
                            err = 999;
                            add_error(error_stack, ErrorType::System, "protocolXML", err,
                                      " Unable to Open a Temporary/Cache XDR File for Writing");
                            break;
                        }

                        UDA_LOG(UDA_LOG_DEBUG, "stdio XDR file: {}", temp_file.c_str());

                        packageType = UDA_PACKAGE_XDRFILE; // The package is a file with XDR serialised data
                        rc = xdr_int(xdrs, &packageType);  // Send data package type
                        rc = rc && xdrrec_endofrecord(xdrs, 1);

                        // Create a stdio file stream

                        xdr_stdio_flag = true;
                        xdrstdio_create(&xdr_output, xdrfile.get(), XDR_ENCODE);
                        xdrs = &xdr_output; // Switch from TCP stream to file based object

                    } else if ((private_flags & private_flags::XdrObject) && protocolVersion >= 7) {

                        // Create a memory stream file
                        // Write the serialised data into a data object using a stdio xdr stream

                        errno = 0;
#  ifndef _WIN32
                        object = nullptr; // the data object
                        objectSize = 0;   // the size of the data object

                        xdrfile = std::unique_ptr<FILE, FCloseDeleter>{open_memstream((char**)&object, &objectSize)};
#  else
                        xdrfile = tmpfile();
#  endif

                        if (xdrfile == nullptr || errno != 0) {
                            err = 999;
                            if (errno != 0) {
                                add_error(error_stack, ErrorType::System, "protocolXML", errno, "");
                            }
                            add_error(error_stack, ErrorType::Code, "protocolXML2", err,
                                      "Unable to Open a XDR Memory Stream for Writing data objects");
                            break;
                        }

                        packageType = UDA_PACKAGE_XDROBJECT; // The package is an XDR serialised object

                        rc = xdr_int(xdrs, &packageType); // Send data package type
                        rc = rc && xdrrec_endofrecord(xdrs, 1);

                        UDA_LOG(UDA_LOG_DEBUG, "Sending Package Type: {}", packageType);

                        // Create a stdio file stream

                        xdr_stdio_flag = true;
                        xdrstdio_create(&xdr_output, xdrfile.get(), XDR_ENCODE);
                        xdrs = &xdr_output; // Switch from TCP stream to memory based object

#  ifdef _WIN32
                        fflush(xdrfile);
                        fseek(xdrfile, 0, SEEK_END);
                        long fsize = ftell(xdrfile);
                        rewind(xdrfile);

                        objectSize = (size_t)fsize;
                        object = (unsigned char*)malloc(objectSize);
                        fread(object, objectSize, 1, xdrfile);
#  endif
                    } else {

                        packageType = UDA_PACKAGE_STRUCTDATA; // The package is regular XDR

                        UDA_LOG(UDA_LOG_DEBUG, "Sending Package Type: {}", packageType);

                        rc = xdr_int(xdrs, &packageType); // Send data package type
                        rc = rc && xdrrec_endofrecord(xdrs, 1);
                    }
#endif

                    // Send the data

                    // send the full set of known named structures
                    rc = rc && xdr_user_defined_type_list(xdrs, userdefinedtypelist, xdr_stdio_flag);

                    UDA_LOG(UDA_LOG_DEBUG, "Structure Definitions sent: rc = {}", rc);

                    if (!rc) {
                        err = 999;
                        add_error(error_stack, ErrorType::Code, "protocolXML", err,
                                  "Bad Return Code passing Structure Definitions");
                        if (xdr_stdio_flag) {
                            xdr_destroy(xdrs); // Close the stdio stream and reuse the TCP socket stream
                            xdrs = priorxdrs;
                            xdr_stdio_flag = false;
                        }
                        break;
                    }

                    rc = rc && xdr_user_defined_type_data(error_stack, xdrs, logmalloclist, userdefinedtypelist, u, (void**)data,
                                                          protocolVersion, xdr_stdio_flag, log_struct_list,
                                                          malloc_source); // send the Data

                    UDA_LOG(UDA_LOG_DEBUG, "Data sent: rc = {}", rc);

                    if (!rc) {
                        err = 999;
                        add_error(error_stack, ErrorType::Code, "protocolXML", err, "Bad Return Code passing data structures");
                        if (xdr_stdio_flag) {
                            xdr_destroy(xdrs); // Close the stdio stream and reuse the TCP socket stream
                            xdrs = priorxdrs;
                            xdr_stdio_flag = false;
                        }
                        break;
                    }

#ifndef FATCLIENT

                    if ((private_flags & private_flags::XdrFile) &&
                        protocolVersion >= 5) { // Server calling another server

                        // Close the stream and file

                        fflush(xdrfile.get());

                        // Switch back to the normal TCP socket xdr stream

                        xdr_destroy(xdrs); // Close the stdio stream
                        xdrs = priorxdrs;
                        xdr_stdio_flag = false;

                        xdrfile = {};

                        // Send the Temporary File

                        UDA_LOG(UDA_LOG_DEBUG, "sending temporary XDR file");

                        err = send_xdr_file(xdrs, temp_file.c_str()); // Read and send

                        remove(temp_file.c_str());

                        if (err != 0) {
                            break;
                        }

                    } else if ((private_flags & private_flags::XdrObject) &&
                               protocolVersion >= 7) { // Server calling another server

                        // Close the stream and file

                        fflush(xdrfile.get());

                        // Write object to a semi-persistent cache with metadata
                        //
                        // client request details    : signal+source arguments
                        // MEMCACHE key            : Created for each new request not available from cache
                        // MEMCACHE value
                        //    type            : XDR object
                        //    SHA1 hash            : md
                        //    Log Entry            : provenance
                        //    Size            : the amount of data
                        //    Data              : the XDR serialised data object
                        // Date & Time            : internal to MEMCACHE - needed to purge old records
                        // MEMCACHE connection object    : Created at server/client startup

                        // Switch back to the normal TCP socket xdr stream

                        xdr_destroy(xdrs); // Close the stdio stream
                        xdrs = priorxdrs;
                        xdr_stdio_flag = false;

                        xdrfile = {};

                        // hash the object
                        sha1Block(object, objectSize, md);

                        // Send the Object
                        UDA_LOG(UDA_LOG_DEBUG, "sending XDR object");

                        // Send size, bytes, hash
                        count = (int)objectSize;
                        rc = xdr_int(xdrs, &count) && xdr_opaque(xdrs, (char*)object, (unsigned int)objectSize) &&
                             xdr_vector(xdrs, (char*)md, hashSize, sizeof(char), (xdrproc_t)xdr_char);

                        rc = rc && xdrrec_endofrecord(xdrs, 1);

                        // Free data object
                        if (object != nullptr) {
                            free(object);
                        }
                        object = nullptr;
                        objectSize = 0;

                        if (err != 0) {
                            break;
                        }

                        // dgm BUG 12/3/2015
                    } // else

#endif // !FATCLIENT

                    //======================================================================================================================

                } else { // Receive Data

                    UDA_LOG(UDA_LOG_DEBUG, "Receiving from Server");

                    // 5 valid options:
                    //    1> unpack structures, no xdr file involved    => private_flags & private_flags::XdrFile   == 0 &&
                    //    packageType == PACKAGE_STRUCTDATA 2> unpack structures, from an xdr file        =>
                    //    private_flags & private_flags::XdrFile   == 0 && packageType == PACKAGE_XDRFILE 3> xdr file only,
                    //    no unpacking, passforward    => private_flags & private_flags::XdrFile        && packageType ==
                    //    PACKAGE_XDRFILE 4> Error                    => private_flags & private_flags::XdrFile        &&
                    //    (packageType == PACKAGE_STRUCTDATA || packageType == PACKAGE_XDROBJECT) 5> unpack structures,
                    //    from an xdr object    => private_flags & private_flags::XdrObject == 0 && packageType ==
                    //    PACKAGE_XDROBJECT 6> xdr object only, no unpacking, passforward    => private_flags &
                    //    private_flags::XdrObject      && packageType == PACKAGE_XDROBJECT 4> Error                    =>
                    //    private_flags & private_flags::XdrObject      && (packageType == PACKAGE_STRUCTDATA ||
                    //    packageType == PACKAGE_XDRFILE)

                    // Data Object Caching rules:
                    // a) on send if the server is the origin of the data [what state variable flags this? Always when
                    // data are Not from an IDAM client plugin!] b) on receipt if a client that is the origin of the
                    // request [what state variable flags this? Not an IDAM client plugin!] c) no caching on
                    // intermediate servers: 2 cache updates only - 1x client and 1x server

                    // Server plugins have a cachePermission property. All IDAM client plugins that chain servers
                    // together should have this property set False. This should be passed into the IDAM client
                    // environment to disable caching. The original client will have this set 'True' by default. The
                    // server will also act on this plugin property.
                    //
                    // setIdamCachePermission([T|F]);

                    // Caching should be made on return from a plugin. All services are assumed to be plugin based.
                    // All enquiries against a cache should be made as early as possible.

                    // A complete data object in serialised form should be cached.

                    int option = 4;

#ifndef FATCLIENT
                    UDA_LOG(UDA_LOG_DEBUG, "Receiving Package Type");

                    rc = xdrrec_skiprecord(xdrs);
                    rc = rc && xdr_int(xdrs, &packageType); // Receive data package type

#else
                    rc = 1;

                    if (private_flags & private_flags::XdrFile) {
                        packageType = UDA_PACKAGE_XDRFILE;
                    } else if (private_flags & private_flags::XdrObject) {
                        packageType = UDA_PACKAGE_XDROBJECT;
                    } else if (private_flags & private_flags::XdrObject) {
                        packageType = UDA_PACKAGE_XDROBJECT;
                    } else {
                        packageType = UDA_PACKAGE_STRUCTDATA;
                    }
#endif

                    if ((private_flags & private_flags::XdrFile) == 0 && packageType == UDA_PACKAGE_STRUCTDATA) {
                        option = 1;
                    }
                    if ((private_flags & private_flags::XdrFile) == 0 && packageType == UDA_PACKAGE_XDRFILE &&
                        protocolVersion >= 5) {
                        option = 2;
                    }
                    if ((private_flags & private_flags::XdrFile) == private_flags::XdrFile &&
                        packageType == UDA_PACKAGE_XDRFILE && protocolVersion >= 5) {
                        option = 3;
                    }
                    if ((private_flags & private_flags::XdrObject) == 0 && packageType == UDA_PACKAGE_XDROBJECT &&
                        protocolVersion >= 7) {
                        option = 5;
                    }
                    if ((private_flags & private_flags::XdrObject) == private_flags::XdrObject &&
                        packageType == UDA_PACKAGE_XDROBJECT && protocolVersion >= 7) {
                        option = 6;
                    }

                    UDA_LOG(UDA_LOG_DEBUG, "{}  {}   {}", private_flags & private_flags::XdrFile,
                            packageType == UDA_PACKAGE_STRUCTDATA, private_flags & private_flags::XdrObject);
                    UDA_LOG(UDA_LOG_DEBUG, "Receive data option : {}", option);
                    UDA_LOG(UDA_LOG_DEBUG, "Receive package Type: {}", packageType);

                    if (option == 4) {
                        err = 999;
                        add_error(error_stack, ErrorType::System, "protocolXML", err, "Unknown package Type control option");
                        break;
                    }

                    // Read xdr file without unpacking the structures

                    if (option == 3) {

                        // Create a temporary XDR file, receive and write data to the file - do not unpack data
                        // structures, pass the file onward

                        errno = 0;
                        if (mkstemp(temp_file.data()) < 0 || errno != 0) {
                            err = 998;
                            if (errno != 0) {
                                err = errno;
                            }
                            add_error(error_stack, ErrorType::System, "protocolXML", err,
                                      "Unable to Obtain a Temporary File Name [3]");
                            err = 998;
                            add_error(error_stack, ErrorType::Code, "protocolXML", err, temp_file.c_str());
                            UDA_LOG(UDA_LOG_DEBUG, "Unable to Obtain a Temporary File Name [3], tempFile=[{}]",
                                    temp_file.c_str());
                            break;
                        }

                        err = receive_xdr_file(xdrs, temp_file.c_str()); // Receive and write the file

                        if (err != 0) {
                            break;
                        }

                        data_block->data = nullptr;                           // No Data - not unpacked
                        data_block->opaque_block = strdup(temp_file.c_str()); // File name
                        data_block->opaque_type =
                            UDA_OPAQUE_TYPE_XDRFILE; // The data block is carrying the filename only

                        // The temporary file is essentially cached
                        // The opaque type has been changed so the future receiving server or client knows what it is
                        // and how to de-serialise it.

                    } else if (option == 6) {

                        // Receive size, bytes, hash

                        rc = xdrrec_skiprecord(xdrs);

                        count = 0;
                        rc = rc && xdr_int(xdrs, &count);
                        objectSize = (size_t)count;

                        if (rc) {
                            object = (unsigned char*)malloc(objectSize * sizeof(unsigned char));
                        }

                        rc = rc && xdr_opaque(xdrs, (char*)object, (unsigned int)objectSize) &&
                             xdr_vector(xdrs, (char*)md, hashSize, sizeof(char), (xdrproc_t)xdr_char);

                        data_block->data = nullptr; // No Data - not unpacked
                        data_block->opaque_block =
                            (void*)object; // data object (needs to be freed when the Data_Block is sent)
                        data_block->opaque_count = (int)objectSize;
                        data_block->opaque_type = UDA_OPAQUE_TYPE_XDROBJECT;

                        // data_block->hash = md;

                        // Check the hash
                        sha1Block(object, objectSize, mdr);
                        rc = 1;
                        for (int i = 0; i < MaxElementSha1; i++) {
                            rc = rc && (md[i] == mdr[i]);
                        }
                        if (!rc) {
                            // ERROR
                        }

                        // The opaque type has been changed so the future receiving server or client knows what it is
                        // and how to de-serialise it.
                    }

                    // Unpack data structures
                    if (option == 1 || option == 2 || option == 5) {
                        logmalloclist = (LogMallocList*)malloc(sizeof(LogMallocList));
                        init_log_malloc_list(logmalloclist);

                        userdefinedtypelist = (UserDefinedTypeList*)malloc(sizeof(UserDefinedTypeList));
                        auto udt_received = (UserDefinedType*)malloc(sizeof(UserDefinedType));

                        init_user_defined_type_list(userdefinedtypelist);

                        UDA_LOG(UDA_LOG_DEBUG, "xdr_userdefinedtypelist #A");

#ifndef FATCLIENT

                        UDA_LOG(UDA_LOG_DEBUG, "private_flags   : {} ", private_flags);
                        UDA_LOG(UDA_LOG_DEBUG, "protocolVersion: {} ", protocolVersion);

                        if (option == 2) {

                            // Create a temporary XDR file and receive data

                            UDA_LOG(UDA_LOG_DEBUG, "creating temporary/cached XDR file");

                            errno = 0;
                            if (mkstemp(temp_file.data()) < 0 || errno != 0) {
                                err = 997;
                                if (errno != 0) {
                                    err = errno;
                                }
                                add_error(error_stack, ErrorType::System, "protocolXML", err,
                                          " Unable to Obtain a Temporary File Name [2]");
                                err = 997;
                                add_error(error_stack, ErrorType::Code, "protocolXML", err, temp_file.c_str());
                                UDA_LOG(UDA_LOG_DEBUG, "Unable to Obtain a Temporary File Name [2], tempFile=[{}]",
                                        temp_file.c_str());
                                break;
                            }

                            err = receive_xdr_file(xdrs, temp_file.c_str()); // Receive and write the file

                            // Create input xdr file stream

                            xdrfile = std::unique_ptr<FILE, FCloseDeleter>{fopen(temp_file.c_str(), "rb")};
                            if (!xdrfile) { // Read temporary file
                                err = 999;
                                add_error(error_stack, ErrorType::System, "protocolXML", err,
                                          " Unable to Open a Temporary XDR File for Writing");
                                break;
                            }

                            xdr_stdio_flag = true;
                            xdrstdio_create(&xdr_input, xdrfile.get(), XDR_DECODE);
                            xdrs = &xdr_input; // Switch from stream to file

                        } else if (option == 5) {

                            // Receive size, bytes, hash

                            rc = xdrrec_skiprecord(xdrs);

                            count = 0;
                            rc = rc && xdr_int(xdrs, &count);
                            objectSize = (size_t)count;

                            if (rc) {
                                object = (unsigned char*)malloc(objectSize * sizeof(unsigned char));
                            }

                            rc = rc && xdr_opaque(xdrs, (char*)object, (unsigned int)objectSize) &&
                                 xdr_vector(xdrs, (char*)md, hashSize, sizeof(char), (xdrproc_t)xdr_char);

                            // Check the hash
                            sha1Block(object, objectSize, mdr);
                            rc = 1;
                            for (int i = 0; i < MaxElementSha1; i++) {
                                rc = rc && (md[i] == mdr[i]);
                            }
                            if (!rc) {
                                // ERROR
                            }

                            // Close current input xdr stream and create a memory stream

                            xdr_stdio_flag = true;
                            xdrmem_create(&xdr_input, (char*)object, (unsigned int)objectSize, XDR_DECODE);
                            xdrs = &xdr_input; // Switch from TCP stream to memory based object
                        }
#endif

                        // receive the full set of known named structures
                        rc = rc && xdr_user_defined_type_list(xdrs, userdefinedtypelist, xdr_stdio_flag);

                        UDA_LOG(UDA_LOG_DEBUG, "xdr_userdefinedtypelist #B");

                        if (!rc) {
                            err = 999;
                            add_error(error_stack, ErrorType::Code, "protocolXML", err,
                                      "Failure receiving Structure Definitions");
                            if (xdr_stdio_flag) {
                                xdr_destroy(xdrs); // Close the stdio stream and reuse the TCP socket stream
                                xdrs = priorxdrs;
                                xdr_stdio_flag = false;
                            }
                            break;
                        }

                        UDA_LOG(UDA_LOG_DEBUG, "udaXDRUserDefinedTypeData #A");
                        init_user_defined_type(udt_received);

                        rc = rc && xdr_user_defined_type_data(error_stack, xdrs, logmalloclist, userdefinedtypelist, udt_received,
                                                              &data, protocolVersion, xdr_stdio_flag, log_struct_list,
                                                              malloc_source); // receive the Data

                        UDA_LOG(UDA_LOG_DEBUG, "udaXDRUserDefinedTypeData #B");
                        if (!rc) {
                            err = 999;
                            add_error(error_stack, ErrorType::Code, "protocolXML", err,
                                      "Failure receiving Data and Structure Definition");
                            if (xdr_stdio_flag) {
                                xdr_destroy(xdrs); // Close the stdio stream and reuse the TCP socket stream
                                xdrs = priorxdrs;
                                xdr_stdio_flag = false;
                            }
                            break;
                        }

#ifndef FATCLIENT

                        if (option == 2) {

                            // Close the stream and file

                            fflush(xdrfile.get());

                            // Switch back to the normal xdr record stream

                            xdr_destroy(xdrs); // Close the stdio stream and reuse the TCP socket stream
                            xdrs = priorxdrs;
                            xdr_stdio_flag = false;

                            xdrfile = {};
                            remove(temp_file.c_str());

                        } else if (option == 5) {

                            // Switch back to the normal xdr record stream

                            xdr_destroy(xdrs); // Close the stdio stream and reuse the TCP socket stream
                            xdrs = priorxdrs;
                            xdr_stdio_flag = false;

                            // Free the object

                            if (object != nullptr) {
                                free(object);
                            }
                            object = nullptr;
                            objectSize = 0;
                        }

#endif // !FATCLIENT

                        if (STR_IEQUALS(udt_received->name, "SArray")) { // expecting this carrier structure

                            auto general_block = (GeneralBlock*)malloc(sizeof(GeneralBlock));

                            auto s = (SArray*)data;
                            if (s->count != data_block->data_n) { // check for consistency
                                err = 999;
                                add_error(error_stack, ErrorType::Code, "protocolXML", err, "Inconsistent S Array Counts");
                                break;
                            }
                            data_block->data =
                                (char*)get_full_ntree(); // Global Root Node with the Carrier Structure containing data
                            data_block->opaque_block = (void*)general_block;
                            general_block->userdefinedtype = udt_received;
                            general_block->userdefinedtypelist = userdefinedtypelist;
                            general_block->logmalloclist = logmalloclist;
                            general_block->lastMallocIndex = 0;

                        } else {
                            err = 999;
                            add_error(error_stack, ErrorType::Code, "protocolXML", err,
                                      "Name of Received Data Structure Incorrect");
                            break;
                        }
                    }
                }

#ifdef FATCLIENT

            } else {
                err = 999;
                add_error(error_stack, ErrorType::Code, "protocolXML", err, "Unknown Opaque type");
                break;
            }
        }

#else

                //====================================================================================================================
                // Passing temporary XDR files or objects: server to server (protocolVersion >= 5 && packageType ==
                // PACKAGE_XDRFILE)

            } else {

                if (data_block->opaque_type == UDA_OPAQUE_TYPE_XDROBJECT) {

                    if (xdrs->x_op == XDR_ENCODE) {
                        UDA_LOG(UDA_LOG_DEBUG, "Forwarding XDR Objects");

                        // Send size, bytes, hash

                        rc = xdr_int(xdrs, &(data_block->opaque_count)) &&
                             xdr_opaque(xdrs, (char*)data_block->opaque_block, data_block->opaque_count) &&
                             xdr_vector(xdrs, (char*)md, hashSize, sizeof(char), (xdrproc_t)xdr_char) &&
                             xdrrec_endofrecord(xdrs, 1);

                        // Check the hash
                        sha1Block(object, objectSize, mdr);
                        rc = 1;
                        for (int i = 0; i < MaxElementSha1; i++) {
                            rc = rc && (md[i] == mdr[i]);
                        }
                        if (!rc) {
                            // ERROR
                        }
                    } else {
                        UDA_LOG(UDA_LOG_DEBUG, "Receiving forwarded XDR File");

                        // Receive size, bytes, hash

                        rc = xdrrec_skiprecord(xdrs);

                        count = 0;
                        rc = rc && xdr_int(xdrs, &count);
                        objectSize = (size_t)count;

                        if (rc) {
                            object = (unsigned char*)malloc(objectSize * sizeof(unsigned char));
                        }

                        rc = rc && xdr_opaque(xdrs, (char*)object, (unsigned int)objectSize);
                        rc = rc && xdr_vector(xdrs, (char*)md, hashSize, sizeof(char), (xdrproc_t)xdr_char);

                        // Check the hash
                        sha1Block(object, objectSize, mdr);
                        rc = 1;
                        for (int i = 0; i < MaxElementSha1; i++) {
                            rc = rc && (md[i] == mdr[i]);
                        }
                        if (!rc) {
                            // ERROR
                        }

                        if (private_flags & private_flags::XdrObject) { // Forward the object again

                            data_block->data = nullptr; // No Data - not unpacked
                            data_block->opaque_block =
                                (void*)object; // data object (needs to be freed when the Data_Block is sent)
                            data_block->opaque_count = (int)objectSize;
                            data_block->opaque_type = UDA_OPAQUE_TYPE_XDROBJECT;

                            UDA_LOG(UDA_LOG_DEBUG, "Forwarding Received forwarded XDR Object");
                        } else {
                            UDA_LOG(UDA_LOG_DEBUG, "Unpacking forwarded XDR Object");

                            // Unpack the data Structures

                            logmalloclist = (LogMallocList*)malloc(sizeof(LogMallocList));
                            init_log_malloc_list(logmalloclist);

                            userdefinedtypelist = (UserDefinedTypeList*)malloc(sizeof(UserDefinedTypeList));
                            UserDefinedType* udt_received = (UserDefinedType*)malloc(sizeof(UserDefinedType));

                            init_user_defined_type_list(userdefinedtypelist);

                            // Close current input xdr stream and create a memory stream

                            xdr_stdio_flag = true;
                            xdrmem_create(&xdr_input, (char*)object, (unsigned int)objectSize, XDR_DECODE);
                            xdrs = &xdr_input; // Switch from TCP stream to memory based object

                            // receive the full set of known named structures
                            rc = xdr_user_defined_type_list(xdrs, userdefinedtypelist, xdr_stdio_flag);

                            if (!rc) {
                                err = 999;
                                add_error(error_stack, ErrorType::Code, "protocolXML", err,
                                          "Failure receiving Structure Definitions");
                                if (xdr_stdio_flag) {
                                    xdr_destroy(xdrs); // Close the stdio stream and reuse the TCP socket stream
                                    xdrs = priorxdrs;
                                    xdr_stdio_flag = false;
                                }
                                break;
                            }

                            init_user_defined_type(udt_received);

                            rc = rc &&
                                 xdr_user_defined_type_data(error_stack, xdrs, logmalloclist, userdefinedtypelist, udt_received,
                                                            &data, protocolVersion, xdr_stdio_flag, log_struct_list,
                                                            malloc_source); // receive the Data

                            if (!rc) {
                                err = 999;
                                add_error(error_stack, ErrorType::Code, "protocolXML", err,
                                          "Failure receiving Data and Structure Definition");
                                if (xdr_stdio_flag) {
                                    xdr_destroy(xdrs); // Close the stdio stream and reuse the TCP socket stream
                                    xdrs = priorxdrs;
                                    xdr_stdio_flag = false;
                                }
                                break;
                            }

                            // Close the stream

                            xdr_destroy(xdrs); // Close the stdio stream and reuse the TCP socket stream
                            xdrs = priorxdrs;
                            xdr_stdio_flag = false;

                            // Write data object to cache

                            // Regular client or server

                            if (STR_IEQUALS(udt_received->name,
                                           "SArray")) { // expecting this carrier structure

                                GeneralBlock* general_block = (GeneralBlock*)malloc(sizeof(GeneralBlock));

                                SArray* s = (SArray*)data;
                                if (s->count != data_block->data_n) { // check for consistency
                                    err = 999;
                                    add_error(error_stack, ErrorType::Code, "protocolXML", err, "Inconsistent S Array Counts");
                                    break;
                                }
                                data_block->data = (char*)
                                    get_full_ntree(); // Global Root Node with the Carrier Structure containing data
                                data_block->opaque_block = (void*)general_block;
                                data_block->opaque_type = UDA_OPAQUE_TYPE_STRUCTURES;
                                general_block->userdefinedtype = udt_received;
                                general_block->userdefinedtypelist = userdefinedtypelist;
                                general_block->logmalloclist = logmalloclist;
                                general_block->lastMallocIndex = 0;

                            } else {
                                err = 999;
                                add_error(error_stack, ErrorType::Code, "protocolXML", err,
                                          "Name of Received Data Structure Incorrect");
                                break;
                            }
                        }
                    }

                } else if (data_block->opaque_type == UDA_OPAQUE_TYPE_XDRFILE) {
                    if (xdrs->x_op == XDR_ENCODE) {
                        UDA_LOG(UDA_LOG_DEBUG, "Forwarding XDR File {}", (char*)data_block->opaque_block);
                        err = send_xdr_file(xdrs, (char*)data_block->opaque_block); // Forward the xdr file
                    } else {
                        UDA_LOG(UDA_LOG_DEBUG, "Receiving forwarded XDR File");

                        // Create a temporary XDR file, receive and write data to the file

                        errno = 0;
                        if (mkstemp(temp_file.data()) < 0 || errno != 0) {
                            err = 996;
                            if (errno != 0) {
                                err = errno;
                            }
                            add_error(error_stack, ErrorType::System, "protocolXML", err,
                                      " Unable to Obtain a Temporary File Name");
                            err = 996;
                            add_error(error_stack, ErrorType::Code, "protocolXML", err, temp_file.c_str());
                            UDA_LOG(UDA_LOG_DEBUG, "Unable to Obtain a Temporary File Name, tempFile=[{}]",
                                    temp_file.c_str());
                            break;
                        }

                        err = receive_xdr_file(xdrs, temp_file.c_str()); // Receive and write the file

                        if (private_flags & private_flags::XdrFile) { // Forward the file (option 3) again

                            // If this is an intermediate client then read the file without unpacking the structures

                            data_block->data = nullptr;                           // No Data - not unpacked
                            data_block->opaque_block = strdup(temp_file.c_str()); // File name
                            data_block->opaque_type =
                                UDA_OPAQUE_TYPE_XDRFILE; // The data block is carrying the filename only

                            UDA_LOG(UDA_LOG_DEBUG, "Forwarding Received forwarded XDR File");
                        } else {
                            UDA_LOG(UDA_LOG_DEBUG, "Unpacking forwarded XDR File");

                            // Unpack the data Structures

                            logmalloclist = (LogMallocList*)malloc(sizeof(LogMallocList));
                            init_log_malloc_list(logmalloclist);

                            userdefinedtypelist = (UserDefinedTypeList*)malloc(sizeof(UserDefinedTypeList));
                            UserDefinedType* udt_received = (UserDefinedType*)malloc(sizeof(UserDefinedType));

                            init_user_defined_type_list(userdefinedtypelist);

                            // Create input xdr file stream

                            xdrfile = std::unique_ptr<FILE, FCloseDeleter>{fopen(temp_file.c_str(), "rb")};
                            if (!xdrfile) { // Read temporary file
                                err = 999;
                                add_error(error_stack, ErrorType::System, "protocolXML", err,
                                          " Unable to Open a Temporary XDR File for Writing");
                                break;
                            }

                            xdr_stdio_flag = true;
                            xdrstdio_create(&xdr_input, xdrfile.get(), XDR_DECODE);
                            xdrs = &xdr_input;

                            rc = xdr_user_defined_type_list(xdrs, userdefinedtypelist, xdr_stdio_flag);
                            // receive the full set of known named structures

                            if (!rc) {
                                err = 999;
                                add_error(error_stack, ErrorType::Code, "protocolXML", err,
                                          "Failure receiving Structure Definitions");
                                if (xdr_stdio_flag) {
                                    xdr_destroy(xdrs); // Close the stdio stream and reuse the TCP socket stream
                                    xdrs = priorxdrs;
                                    xdr_stdio_flag = false;
                                }
                                break;
                            }

                            init_user_defined_type(udt_received);

                            rc = rc &&
                                 xdr_user_defined_type_data(error_stack, xdrs, logmalloclist, userdefinedtypelist, udt_received,
                                                            &data, protocolVersion, xdr_stdio_flag, log_struct_list,
                                                            malloc_source); // receive the Data

                            if (!rc) {
                                err = 999;
                                add_error(error_stack, ErrorType::Code, "protocolXML", err,
                                          "Failure receiving Data and Structure Definition");
                                if (xdr_stdio_flag) {
                                    xdr_destroy(xdrs); // Close the stdio stream and reuse the TCP socket stream
                                    xdrs = priorxdrs;
                                    xdr_stdio_flag = false;
                                }
                                break;
                            }

                            // Close the stream and file

                            xdr_destroy(xdrs); // Close the stdio stream and reuse the TCP socket stream
                            xdrs = priorxdrs;
                            xdr_stdio_flag = false;

                            xdrfile = {};

                            // Remove the Temporary File
                            remove(temp_file.c_str());

                            // Regular client or server

                            if (STR_IEQUALS(udt_received->name,
                                           "SArray")) { // expecting this carrier structure

                                auto general_block = (GeneralBlock*)malloc(sizeof(GeneralBlock));
                                auto s = (SArray*)data;
                                if (s->count != data_block->data_n) { // check for consistency
                                    err = 999;
                                    add_error(error_stack, ErrorType::Code, "protocolXML", err, "Inconsistent S Array Counts");
                                    break;
                                }
                                data_block->data = (char*)
                                    get_full_ntree(); // Global Root Node with the Carrier Structure containing data
                                data_block->opaque_block = (void*)general_block;
                                data_block->opaque_type = UDA_OPAQUE_TYPE_STRUCTURES;
                                general_block->userdefinedtype = udt_received;
                                general_block->userdefinedtypelist = userdefinedtypelist;
                                general_block->logmalloclist = logmalloclist;
                                general_block->lastMallocIndex = 0;

                            } else {
                                err = 999;
                                add_error(error_stack, ErrorType::Code, "protocolXML", err,
                                          "Name of Received Data Structure Incorrect");
                                break;
                            }
                        }
                    }
                } else {
                    err = 999;
                    add_error(error_stack, ErrorType::Code, "protocolXML", err, "Unknown Opaque type");
                    break;
                }
            }
        }
#endif

        //----------------------------------------------------------------------------
        // Meta Data XML

#ifndef FATCLIENT

        if (protocol_id == ProtocolId::Meta) {
            data_block = (DataBlock*)str;
            if (data_block->opaque_type == UDA_OPAQUE_TYPE_XML_DOCUMENT && data_block->opaque_count > 0) {
                switch (direction) {
                    case XDRStreamDirection::Receive:
                        if (!(rc = xdrrec_skiprecord(xdrs))) {
                            err = (int)ProtocolError::Error5;
                            break;
                        }
                        if ((data_block->opaque_block = (char*)malloc((data_block->opaque_count + 1) * sizeof(char))) ==
                            nullptr) {
                            err = 991;
                            break;
                        }
                        if (!(rc = xdr_meta(xdrs, data_block))) {
                            err = 992;
                            break;
                        }
                        break;

                    case XDRStreamDirection::Send:

                        if (!(rc = xdr_meta(xdrs, data_block))) {
                            UDA_LOG(UDA_LOG_DEBUG, "Error sending Metadata XML Document: \n{}\n",
                                    (char*)data_block->opaque_block);
                            err = 990;
                            break;
                        }
                        if (!(rc = xdrrec_endofrecord(xdrs, 1))) {
                            err = (int)ProtocolError::Error7;
                            break;
                        }
                        break;

                    case XDRStreamDirection::FreeHeap:
                        break;

                    default:
                        err = (int)ProtocolError::Error4;
                        break;
                }
            }
        }

#endif
        //----------------------------------------------------------------------------
        // End of Error Trap Loop

    } while (0);

    return err;
}

// include libcrypto.so in link step
// The hash output is 20 bytes
// http://linux.die.net/man/3/sha1_update

// Hash a complete data block
void sha1Block(unsigned char* block, size_t blockSize, unsigned char* md)
{
    SHA1(block, blockSize, md);
}

#ifndef FATCLIENT

int unpackXDRFile(std::vector<UdaError>& error_stack, LogMallocList* logmalloclist, XDR* xdrs, unsigned char* filename, DataBlock* data_block,
                  int protocolVersion, bool xdr_stdio_flag, LogStructList* log_struct_list, int malloc_source)
{
    int rc = 1, err = 0;
    void* data = nullptr;
    XDR xdr_input;
    FILE* xdrfile = nullptr;
    XDR* priorxdrs = xdrs; // Preserve the current stream object

    UDA_LOG(UDA_LOG_DEBUG, "unpackXDRFile: Unpacking XDR Data Files");

    // Unpack the data Structures

    logmalloclist = (LogMallocList*)malloc(sizeof(LogMallocList));
    init_log_malloc_list(logmalloclist);

    auto userdefinedtypelist = (UserDefinedTypeList*)malloc(sizeof(UserDefinedTypeList));
    auto udt_received = (UserDefinedType*)malloc(sizeof(UserDefinedType));

    init_user_defined_type_list(userdefinedtypelist);

    // Close current input xdr stream and create a file stream

    if ((xdrfile = fopen((char*)filename, "rb")) == nullptr) { // Read temporary file
        err = 999;
        add_error(error_stack, ErrorType::System, "unpackXDRFile", err, " Unable to Open a XDR File for Reading");
        return err;
    }

    xdr_stdio_flag = true;
    xdrstdio_create(&xdr_input, xdrfile, XDR_DECODE);
    xdrs = &xdr_input; // Switch from stream to file

    do { // Error trap

        // Unpack the associated set of named structures

        rc = xdr_user_defined_type_list(xdrs, userdefinedtypelist, xdr_stdio_flag);

        if (!rc) {
            err = 999;
            add_error(error_stack, ErrorType::Code, "unpackXDRFile", err, "Failure receiving Structure Definitions");
            if (xdr_stdio_flag) {
                xdr_destroy(xdrs); // Close the stdio stream and reuse the TCP socket stream
                xdrs = priorxdrs;
                xdr_stdio_flag = false;
            }
            break;
        }

        // Unpack the Data

        init_user_defined_type(udt_received);

        rc = rc && xdr_user_defined_type_data(error_stack, xdrs, logmalloclist, userdefinedtypelist, udt_received, &data,
                                              protocolVersion, xdr_stdio_flag, log_struct_list, malloc_source);

        if (!rc) {
            err = 999;
            add_error(error_stack, ErrorType::Code, "unpackXDRFile", err, "Failure receiving Data and Structure Definition");
            if (xdr_stdio_flag) {
                xdr_destroy(xdrs); // Close the stdio stream and reuse the TCP socket stream
                xdrs = priorxdrs;
                xdr_stdio_flag = false;
            }
            break;
        }

    } while (0);

    // Close the stream

    fflush(xdrfile);
    fclose(xdrfile);

    // Switch back to the normal xdr record stream

    xdr_destroy(xdrs);
    xdrs = priorxdrs;
    xdr_stdio_flag = false;

    if (err != 0) {
        return err;
    }

    // Return data tree

    if (STR_IEQUALS(udt_received->name, "SArray")) { // expecting this carrier structure

        auto general_block = (GeneralBlock*)malloc(sizeof(GeneralBlock));

        auto s = (SArray*)data;
        if (s->count != data_block->data_n) { // check for consistency
            err = 999;
            add_error(error_stack, ErrorType::Code, "unpackXDRFile", err, "Inconsistent SArray Counts");
            return err;
        }

        data_block->data = (char*)get_full_ntree(); // Global Root Node with the Carrier Structure containing data
        data_block->opaque_block = (void*)general_block;
        data_block->opaque_type = UDA_OPAQUE_TYPE_STRUCTURES;
        general_block->userdefinedtype = udt_received;
        general_block->userdefinedtypelist = userdefinedtypelist;
        general_block->logmalloclist = logmalloclist;
        general_block->lastMallocIndex = 0;

    } else {
        err = 999;
        add_error(error_stack, ErrorType::Code, "unpackXDRFile", err, "Name of Received Data Structure Incorrect");
        return err;
    }

    return err;
}

int unpackXDRObject(std::vector<UdaError>& error_stack, LogMallocList* logmalloclist, XDR* xdrs, unsigned char* object, size_t objectSize,
                    DataBlock* data_block, int protocolVersion, bool xdr_stdio_flag, LogStructList* log_struct_list,
                    int malloc_source)
{

    int rc = 1, err = 0;
    void* data = nullptr;
    XDR xdr_input;
    XDR* priorxdrs = xdrs; // Preserve the current stream object

    UDA_LOG(UDA_LOG_DEBUG, "unpackXDRObject: Unpacking XDR Data Object");

    // Unpack the data Structures

    logmalloclist = (LogMallocList*)malloc(sizeof(LogMallocList));
    init_log_malloc_list(logmalloclist);

    auto userdefinedtypelist = (UserDefinedTypeList*)malloc(sizeof(UserDefinedTypeList));
    auto udt_received = (UserDefinedType*)malloc(sizeof(UserDefinedType));

    init_user_defined_type_list(userdefinedtypelist);

    // Create a memory stream

    xdr_stdio_flag = true;
    xdrmem_create(&xdr_input, (char*)object, (unsigned int)objectSize, XDR_DECODE);
    xdrs = &xdr_input;

    do { // Error trap

        // Unpack the associated set of named structures

        rc = xdr_user_defined_type_list(xdrs, userdefinedtypelist, xdr_stdio_flag);

        if (!rc) {
            err = 999;
            add_error(error_stack, ErrorType::Code, "unpackXDRObject", err, "Failure receiving Structure Definitions");
            if (xdr_stdio_flag) {
                xdr_destroy(xdrs); // Close the stdio stream and reuse the TCP socket stream
                xdrs = priorxdrs;
                xdr_stdio_flag = false;
            }
            break;
        }

        // Unpack the Data

        init_user_defined_type(udt_received);

        rc = rc && xdr_user_defined_type_data(error_stack, xdrs, logmalloclist, userdefinedtypelist, udt_received, &data,
                                              protocolVersion, xdr_stdio_flag, log_struct_list, malloc_source);

        if (!rc) {
            err = 999;
            add_error(error_stack, ErrorType::Code, "unpackXDRObject", err, "Failure receiving Data and Structure Definition");
            if (xdr_stdio_flag) {
                xdr_destroy(xdrs); // Close the stdio stream and reuse the TCP socket stream
                xdrs = priorxdrs;
                xdr_stdio_flag = false;
            }
            break;
        }

    } while (0);

    // Switch back to the normal xdr record stream

    xdr_destroy(xdrs);
    xdrs = priorxdrs;
    xdr_stdio_flag = false;

    if (err != 0) {
        return err;
    }

    // Return data tree

    if (STR_IEQUALS(udt_received->name, "SArray")) { // expecting this carrier structure

        auto general_block = (GeneralBlock*)malloc(sizeof(GeneralBlock));

        auto s = (SArray*)data;
        if (s->count != data_block->data_n) { // check for consistency
            err = 999;
            add_error(error_stack, ErrorType::Code, "unpackXDRObject", err, "Inconsistent SArray Counts");
            return err;
        }

        data_block->data = (char*)get_full_ntree(); // Global Root Node with the Carrier Structure containing data
        data_block->opaque_block = (void*)general_block;
        data_block->opaque_type = UDA_OPAQUE_TYPE_STRUCTURES;
        general_block->userdefinedtype = udt_received;
        general_block->userdefinedtypelist = userdefinedtypelist;
        general_block->logmalloclist = logmalloclist;
        general_block->lastMallocIndex = 0;

    } else {
        err = 999;
        add_error(error_stack, ErrorType::Code, "unpackXDRObject", err, "Name of Received Data Structure Incorrect");
        return err;
    }

    return err;
}

//==========================================================================================================================
// Serialise a regular Data_Block structure
// Write to a memory block - the data object - using a memory stream

int packXDRDataBlockObject(std::vector<UdaError>& error_stack, unsigned char* object, size_t objectSize, DataBlock* data_block,
                           LogMallocList* logmalloclist, UserDefinedTypeList* userdefinedtypelist, int protocolVersion,
                           LogStructList* log_struct_list, unsigned int private_flags, int malloc_source)
{
    int err = 0;
    XDR xdrObject;
    FILE* xdrfile = nullptr;

#  ifdef CACHEDEV
    if (data_block->totalDataBlockSize > 1000000 || data_block->data_type == UDA_TYPE_UNKNOWN) {
        return 0; // Size is too large and the type is not Atomic
    }
#  endif
    do {

        errno = 0;

#  ifndef _WIN32
        xdrfile = open_memstream((char**)&object, &objectSize);
#  else
        xdrfile = tmpfile();
        fwrite(object, objectSize, 1, xdrfile);
        fflush(xdrfile);
        rewind(xdrfile);
#  endif

        if (xdrfile == nullptr || errno != 0) {
            err = 999;
            if (errno != 0) {
                add_error(error_stack, ErrorType::System, "packXDRDataBlockObject", errno, "");
            }
            add_error(error_stack, ErrorType::Code, "packXDRDataBlockObject", err,
                      "Unable to Open a XDR Memory Stream for Writing data objects");
            break;
        }

        // Create stdio file stream

        xdrstdio_create(&xdrObject, xdrfile, XDR_ENCODE);

        // Data object meta data

        // Serialise the structure and create the data object

        std::vector<DataBlock> data_block_list;
        data_block_list.push_back(*data_block);
        err = protocol2(error_stack, &xdrObject, ProtocolId::DataBlockList, XDRStreamDirection::Send, nullptr, logmalloclist, userdefinedtypelist,
                        &data_block_list, protocolVersion, log_struct_list, private_flags, malloc_source);

        // Close the stream and file

        fflush(xdrfile);
        fclose(xdrfile);

        xdr_destroy(&xdrObject);

    } while (0);

    return err;
}

// Deserialise a regular Data_Block structure
// Read from a memory block - the data object - using a memory stream

int unpackXDRDataBlockObject(std::vector<UdaError>& error_stack, unsigned char* object, size_t objectSize, DataBlock* data_block,
                             LogMallocList* logmalloclist, UserDefinedTypeList* userdefinedtypelist,
                             int protocolVersion, LogStructList* log_struct_list, unsigned int private_flags,
                             int malloc_source)
{
    int err = 0;
    XDR xdrObject;

    if (objectSize > 1000000) {
        return 0; // Check type is Atomic
    }

    do {

        // Create a memory stream

        xdrmem_create(&xdrObject, (char*)object, (unsigned int)objectSize, XDR_DECODE);

        // Data object meta data

        // Deserialise the object and create the data_block structure

        std::vector<DataBlock> data_block_list;
        data_block_list.push_back(*data_block);
        err = protocol2(error_stack, &xdrObject, ProtocolId::DataBlockList, XDRStreamDirection::Receive, nullptr, logmalloclist,
                        userdefinedtypelist, &data_block_list, protocolVersion, log_struct_list, private_flags,
                        malloc_source);

        // Close the stream

        xdr_destroy(&xdrObject);

    } while (0);

    return err;
}

#endif // !FATCLIENT
