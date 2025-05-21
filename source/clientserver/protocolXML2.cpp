/*----------------------------------------------------------------------------------------------
* Client - Server Conversation Protocol for XML based Hierarchical Data Structures
*
* Args:	xdrs		XDR Stream
*
*	protocol_id	Client/Server Conversation item: Data Exchange context
*	direction	Send (0) or Receive (1) or Free (2)
*	token		current error condition or next protocol or .... exchange token
*
*	str		Information Structure depending on the protocol id ....
*
*	100	efit
*	101	pfcoils
*	102	pfpassive
*	103	pfsupplies
*	104	fluxloop
*	105	magprobe
*	106	pfcircuit
*	107	plasmacurrent
*	108	diamagnetic
*	109	toroidalfield
*	110	limiter
*
* Returns: error code if failure, otherwise 0
*
*---------------------------------------------------------------------------------------------------------
* Notes on Generalised Data Structures:
*
* The DATA_BLOCK structure has the following fields used to pass and receive generalised data structures
*
* data_block->data_type		set to UDA_TYPE_COMPOUND (external to this routine)
* data_block->data_n		set to the count of structure array elements (external to this routine)
*
* data_block->data		sending (server side): set to the data array (external to this routine)
*				receiving (client side): set to the root data tree node within this routine
*
* data_block->opaque_type	set to UDA_OPAQUE_TYPE_STRUCTURES (external to this routine)
* data_block->count		set to 1 (external to this routine). Not Used!
*
* data_block->opaque_block	sending (server side): set to the User Defined Data Structure Definition of
*				the Data (external to this routine).
*				receiving (client side): set to the SARRAY Data Structure Definition
*
* The SARRAY structure has the following:
*
* sarray.count			set to the count of structure array elements. Identical to data_block->data_n.
* sarray.rank			set to 1 (Higher ranked arrays possible ?)
* sarray.shape			set to [sarray.count] for consistency.
* sarray.data			set to data_block->data
* sarray.type			set to the name of the User Defined Structure type of data
*				(data_block->opaque_block->name). This is registered within the Structure
*				Type List.
**--------------------------------------------------------------------------------------------------*/
#include "protocolXML2.h"

#include <stdlib.h>
#include <errno.h>

#include <structures/struct.h>
#include <logging/logging.h>
#include <clientserver/memstream.h>
#include <clientserver/mkstemp.h>

#include "readXDRFile.h"
#include "errorLog.h"
#include "protocol.h"
#include "xdrlib.h"
#include "stringUtils.h"

#ifdef SERVERBUILD
#  include <server/udaServer.h>
#  include <server/createXDRStream.h>
#  include <server/serverStartup.h>
#endif

#ifndef FATCLIENT
#  include "udaErrors.h"
#endif

#define HASHXDR 1
#ifdef HASHXDR

#  include <openssl/sha.h>

#  define PARTBLOCKINIT     1
#  define PARTBLOCKUPDATE   2
#  define PARTBLOCKOUTPUT   3

extern "C" {

void sha1Block(unsigned char* block, size_t blockSize, unsigned char* md);

int sha1File(char* name, FILE* fh, unsigned char* md);

}

#endif // HASHXDR

#define MAXELEMENTSHA1        20

int
protocolXML2(XDR* xdrs, int protocol_id, int direction, int* token, LOGMALLOCLIST* logmalloclist,
             USERDEFINEDTYPELIST* userdefinedtypelist, void* str, int protocolVersion)
{
    DATA_BLOCK* data_block;

    int rc, err = 0, count = 0;

#ifndef FATCLIENT
    XDR XDRInput;                    // stdio xdr files
    XDR XDROutput;
#endif
    FILE* xdrfile = nullptr;

    XDR* priorxdrs = xdrs;        // Preserve the current stream object

    char tempFile[MAXPATH] = "/tmp/idamXDRXXXXXX";
    char* env = nullptr;

    unsigned char md[MAXELEMENTSHA1 + 1];        // SHA1 Hash
    md[MAXELEMENTSHA1] = '\0';
    strcpy((char*)md, "12345678901234567890");
    int hashSize = MAXELEMENTSHA1;
    unsigned char mdr[MAXELEMENTSHA1];        // SHA1 Hash of data received

    if ((privateFlags & PRIVATEFLAG_XDRFILE) && protocolVersion >= 5) {        // Intermediate XDR File, not stream
        if ((env = getenv("UDA_WORK_DIR")) != nullptr) {
            // File to record XDR encoded data
            snprintf(tempFile, MAXPATH, "%s/idamXDRXXXXXX", env);
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

        if (protocol_id == PROTOCOL_STRUCTURES) {

            void* data = nullptr;
            data_block = (DATA_BLOCK*)str;

            unsigned char* object = nullptr;
            size_t objectSize = 0;

            if (data_block->opaque_type == UDA_OPAQUE_TYPE_STRUCTURES) {
                // These can be transformed into different opaque types to simplify sending
                int packageType = 0;

                UDA_LOG(UDA_LOG_DEBUG, "Compound Data Structure\n");
                UDA_LOG(UDA_LOG_DEBUG, "direction  : %d [%d][%d]\n", (int)xdrs->x_op, XDR_ENCODE, XDR_DECODE);

                if (xdrs->x_op == XDR_ENCODE) {        // Send Data

                    SARRAY sarray;                                // Structure array carrier structure
                    SARRAY* psarray = &sarray;
                    int shape = data_block->data_n;                                         // rank 1 array of dimension lengths
                    auto udt = (USERDEFINEDTYPE*)data_block->opaque_block;     // The data's structure definition
                    // Locate the carrier structure definition
                    USERDEFINEDTYPE* u = findUserDefinedType(userdefinedtypelist, "SARRAY", 0);

                    UDA_LOG(UDA_LOG_DEBUG, "Sending to Client\n");

                    if (udt == nullptr || u == nullptr) {
                        err = 999;
                        UDA_LOG(UDA_LOG_DEBUG, "nullptr SARRAY User defined data Structure Definition\n");
                        printUserDefinedTypeListTable(*userdefinedtypelist);
                        addIdamError(CODEERRORTYPE, "protocolXML", err,
                                     "nullptr User defined data Structure Definition");
                        break;
                    }

                    UDA_LOG(UDA_LOG_DEBUG, "Creating SARRAY carrier structure\n");

                    initSArray(&sarray);
                    sarray.count = data_block->data_n;          // Number of this structure
                    sarray.rank = 1;                            // Array Data Rank?
                    sarray.shape = &shape;                      // Only if rank > 1?
                    sarray.data = (void*)data_block->data;     // Pointer to the data to be passed
                    strcpy(sarray.type, udt->name);             // The name of the type
                    data = (void*)&psarray;                    // Pointer to the SARRAY array pointer
                    addNonMalloc(logmalloclist, (void*)&shape, 1, sizeof(int), "int");

                    UDA_LOG(UDA_LOG_DEBUG, "sending Structure Definitions\n");

                    rc = 1;

#ifndef FATCLIENT

                    // If access is server to server then avoid multiple write/reads of structure components over xdr by creating a
                    // temporary xdr file or xdr object and passing the file or object. Structures need only be created in the originating client, not the
                    // intermediate server clients. Control using a global properties flag: privateFlags - passed from the originating client
                    // to all servers along the chain

                    UDA_LOG(UDA_LOG_DEBUG, "privateFlags   : %d \n", privateFlags);
                    UDA_LOG(UDA_LOG_DEBUG, "protocolVersion: %d \n", protocolVersion);

                    if ((privateFlags & PRIVATEFLAG_XDRFILE) &&
                        protocolVersion >= 5) {        // Server calling another server

                        // Create a temporary or cached XDR file
                        // Record name and location in MEMCACHE

                        UDA_LOG(UDA_LOG_DEBUG, "creating temporary/cache XDR file\n");

                        errno = 0;
                        if (mkstemp(tempFile) < 0 || errno != 0) {
                            err = 999;
                            if (errno != 0) err = errno;
                            addIdamError(SYSTEMERRORTYPE, "protocolXML", err,
                                         " Unable to Obtain a Temporary/Cache File Name");
                            break;
                        }
                        if ((xdrfile = fopen(tempFile, "wb")) == nullptr) {
                            err = 999;
                            addIdamError(SYSTEMERRORTYPE, "protocolXML", err,
                                         " Unable to Open a Temporary/Cache XDR File for Writing");
                            break;
                        }

                        UDA_LOG(UDA_LOG_DEBUG, "stdio XDR file: %s\n", tempFile);

                        packageType = PACKAGE_XDRFILE;              // The package is a file with XDR serialised data
                        rc = xdr_int(xdrs, &packageType);           // Send data package type
                        rc = rc && xdrrec_endofrecord(xdrs, 1);

                        // Create a stdio file stream

                        XDRstdioFlag = 1;
                        xdrstdio_create(&XDROutput, xdrfile, XDR_ENCODE);
                        xdrs = &XDROutput;                        // Switch from TCP stream to file based object

                    } else if ((privateFlags & PRIVATEFLAG_XDROBJECT) && protocolVersion >= 7) {

                        // Create a memory stream file
                        // Write the serialised data into a data object using a stdio xdr stream

                        errno = 0;
#ifndef _WIN32
                        object = nullptr;    // the data object
                        objectSize = 0;    // the size of the data object

                        xdrfile = open_memstream((char**)&object, &objectSize);
#else
                        xdrfile = tmpfile();
#endif

                        if (xdrfile == nullptr || errno != 0) {
                            err = 999;
                            if (errno != 0) addIdamError(SYSTEMERRORTYPE, "protocolXML", errno, "");
                            addIdamError(CODEERRORTYPE, "protocolXML2", err,
                                         "Unable to Open a XDR Memory Stream for Writing data objects");
                            break;
                        }

                        packageType = PACKAGE_XDROBJECT;        // The package is an XDR serialised object

                        rc = xdr_int(xdrs, &packageType);        // Send data package type
                        rc = rc && xdrrec_endofrecord(xdrs, 1);

                        UDA_LOG(UDA_LOG_DEBUG, "Sending Package Type: %d\n", packageType);

                        // Create a stdio file stream

                        XDRstdioFlag = 1;
                        xdrstdio_create(&XDROutput, xdrfile, XDR_ENCODE);
                        xdrs = &XDROutput;                      // Switch from TCP stream to memory based object

#ifdef _WIN32
                        fflush(xdrfile);
                        fseek(xdrfile, 0, SEEK_END);
                        long fsize = ftell(xdrfile);
                        rewind(xdrfile);

                        objectSize = (size_t)fsize;
                        object = (unsigned char*)malloc(objectSize);
                        fread(object, objectSize, 1, xdrfile);
#endif
                    } else {

                        packageType = PACKAGE_STRUCTDATA;        // The package is regular XDR

                        UDA_LOG(UDA_LOG_DEBUG, "Sending Package Type: %d\n", packageType);

                        rc = xdr_int(xdrs, &packageType);        // Send data package type
                        rc = rc && xdrrec_endofrecord(xdrs, 1);

                    }
#endif

// Send the data

                    // send the full set of known named structures
                    rc = rc && xdr_userdefinedtypelist(xdrs, userdefinedtypelist);

                    UDA_LOG(UDA_LOG_DEBUG, "Structure Definitions sent: rc = %d\n", rc);

                    if (!rc) {
                        err = 999;
                        addIdamError(CODEERRORTYPE, "protocolXML", err,
                                     "Bad Return Code passing Structure Definitions");
                        if (XDRstdioFlag) {
                            xdr_destroy(xdrs);        // Close the stdio stream and reuse the TCP socket stream
                            xdrs = priorxdrs;
                            XDRstdioFlag = 0;
                            if (xdrfile != nullptr) fclose(xdrfile);
                        }
                        break;
                    }

                    rc = rc && xdrUserDefinedTypeData(xdrs, logmalloclist, userdefinedtypelist, u,
                                                      (void**)data, protocolVersion);        // send the Data

                    UDA_LOG(UDA_LOG_DEBUG, "Data sent: rc = %d\n", rc);

                    if (!rc) {
                        err = 999;
                        addIdamError(CODEERRORTYPE, "protocolXML", err,
                                     "Bad Return Code passing data structures");
                        if (XDRstdioFlag) {
                            xdr_destroy(xdrs);        // Close the stdio stream and reuse the TCP socket stream
                            xdrs = priorxdrs;
                            XDRstdioFlag = 0;
                            if (xdrfile != nullptr) fclose(xdrfile);
                        }
                        break;
                    }

#ifndef FATCLIENT

                    if ((privateFlags & PRIVATEFLAG_XDRFILE) &&
                        protocolVersion >= 5) {        // Server calling another server

// Close the stream and file

                        fflush(xdrfile);

// Switch back to the normal TCP socket xdr stream

                        xdr_destroy(xdrs);        // Close the stdio stream
                        xdrs = priorxdrs;
                        XDRstdioFlag = 0;

                        fclose(xdrfile);

// Send the Temporary File

                        UDA_LOG(UDA_LOG_DEBUG, "sending temporary XDR file\n");

#ifdef HASHXDRFILE
                        err = sendXDRFile(xdrs, tempFile, md);	// Read and send with SHA1 hash
#else
                        err = sendXDRFile(xdrs, tempFile);        // Read and send
#endif

                        remove(tempFile);

                        if (err != 0) break;

                    } else if ((privateFlags & PRIVATEFLAG_XDROBJECT) &&
                               protocolVersion >= 7) {        // Server calling another server

// Close the stream and file

                        fflush(xdrfile);

// Write object to a semi-persistent cache with metadata
//
// client request details	: signal+source arguments
// MEMCACHE key			: Created for each new request not available from cache
// MEMCACHE value
//    type			: XDR object
//    SHA1 hash			: md
//    Log Entry			: provenance
//    Size			: the amount of data
//    Data  			: the XDR serialised data object
// Date & Time			: internal to MEMCACHE - needed to purge old records
// MEMCACHE connection object	: Created at server/client startup

// Switch back to the normal TCP socket xdr stream

                        xdr_destroy(xdrs);        // Close the stdio stream
                        xdrs = priorxdrs;
                        XDRstdioFlag = 0;

                        fclose(xdrfile);

// hash the object

#ifdef HASHXDR
                        sha1Block(object, objectSize, md);
#endif

// Send the Object

                        UDA_LOG(UDA_LOG_DEBUG, "sending XDR object\n");

// Send size, bytes, hash

                        count = (int)objectSize;
                        rc = xdr_int(xdrs, &count)
                             && xdr_opaque(xdrs, (char*)object, (unsigned int)objectSize)
                             && xdr_vector(xdrs, (char*)md, hashSize, sizeof(char), (xdrproc_t)xdr_char);

                        rc = rc && xdrrec_endofrecord(xdrs, 1);

// Free data object

                        if (object != nullptr) free(object);
                        object = nullptr;
                        objectSize = 0;

                        if (err != 0) break;

// dgm BUG 12/3/2015
                    } //else

#endif    // !FATCLIENT

//======================================================================================================================

                } else {            // Receive Data

                    UDA_LOG(UDA_LOG_DEBUG, "Receiving from Server\n");

// 5 valid options:
//	1> unpack structures, no xdr file involved	=> privateFlags & PRIVATEFLAG_XDRFILE   == 0 && packageType == PACKAGE_STRUCTDATA
//	2> unpack structures, from an xdr file		=> privateFlags & PRIVATEFLAG_XDRFILE   == 0 && packageType == PACKAGE_XDRFILE
//	3> xdr file only, no unpacking, passforward	=> privateFlags & PRIVATEFLAG_XDRFILE        && packageType == PACKAGE_XDRFILE
//	4> Error					=> privateFlags & PRIVATEFLAG_XDRFILE        && (packageType == PACKAGE_STRUCTDATA || packageType == PACKAGE_XDROBJECT)
//	5> unpack structures, from an xdr object	=> privateFlags & PRIVATEFLAG_XDROBJECT == 0 && packageType == PACKAGE_XDROBJECT
//	6> xdr object only, no unpacking, passforward	=> privateFlags & PRIVATEFLAG_XDROBJECT      && packageType == PACKAGE_XDROBJECT
//	4> Error					=> privateFlags & PRIVATEFLAG_XDROBJECT      && (packageType == PACKAGE_STRUCTDATA || packageType == PACKAGE_XDRFILE)

// Data Object Caching rules:
// a) on send if the server is the origin of the data [what state variable flags this? Always when data are Not from an IDAM client plugin!]
// b) on receipt if a client that is the origin of the request [what state variable flags this? Not an IDAM client plugin!]
// c) no caching on intermediate servers: 2 cache updates only - 1x client and 1x server

// Server plugins have a cachePermission property. All IDAM client plugins that chain servers together should have this property set False. This should be
// passed into the IDAM client environment to disable caching. The original client will have this set 'True' by default. The server will also act on
// this plugin property.
//
// setIdamCachePermission([T|F]);

// Caching should be made on return from a plugin. All services are assumed to be plugin based.
// All enquiries against a cache should be made as early as possible.

// A complete data object in serialised form should be cached.



                    int option = 4;

#ifndef FATCLIENT
                    UDA_LOG(UDA_LOG_DEBUG, "Receiving Package Type\n");

                    rc = xdrrec_skiprecord(xdrs);
                    rc = rc && xdr_int(xdrs, &packageType);        // Receive data package type

#else
                    rc = 1;

                    if (privateFlags & PRIVATEFLAG_XDRFILE)
                        packageType = PACKAGE_XDRFILE;
                    else if (privateFlags & PRIVATEFLAG_XDROBJECT)
                        packageType = PACKAGE_XDROBJECT;
                    else if (privateFlags & PRIVATEFLAG_XDROBJECT)
                        packageType = PACKAGE_XDROBJECT;
                    else
                        packageType = PACKAGE_STRUCTDATA;
#endif

                    if ((privateFlags & PRIVATEFLAG_XDRFILE) == 0 && packageType == PACKAGE_STRUCTDATA) option = 1;
                    if ((privateFlags & PRIVATEFLAG_XDRFILE) == 0 && packageType == PACKAGE_XDRFILE &&
                        protocolVersion >= 5) {
                        option = 2;
                    }
                    if ((privateFlags & PRIVATEFLAG_XDRFILE) == PRIVATEFLAG_XDRFILE && packageType == PACKAGE_XDRFILE &&
                        protocolVersion >= 5) {
                        option = 3;
                    }
                    if ((privateFlags & PRIVATEFLAG_XDROBJECT) == 0 && packageType == PACKAGE_XDROBJECT &&
                        protocolVersion >= 7) {
                        option = 5;
                    }
                    if ((privateFlags & PRIVATEFLAG_XDROBJECT) == PRIVATEFLAG_XDROBJECT &&
                        packageType == PACKAGE_XDROBJECT && protocolVersion >= 7) {
                        option = 6;
                    }

                    UDA_LOG(UDA_LOG_DEBUG, "%d  %d   %d\n", privateFlags & PRIVATEFLAG_XDRFILE,
                            packageType == PACKAGE_STRUCTDATA, privateFlags & PRIVATEFLAG_XDROBJECT);
                    UDA_LOG(UDA_LOG_DEBUG, "Receive data option : %d\n", option);
                    UDA_LOG(UDA_LOG_DEBUG, "Receive package Type: %d\n", packageType);

                    if (option == 4) {
                        err = 999;
                        addIdamError(SYSTEMERRORTYPE, "protocolXML", err,
                                     "Unknown package Type control option");
                        break;
                    }

// Read xdr file without unpacking the structures

                    if (option == 3) {

// Create a temporary XDR file, receive and write data to the file - do not unpack data structures, pass the file onward

                        errno = 0;
                        if (mkstemp(tempFile) < 0 || errno != 0) {
                            err = 998;
                            if (errno != 0) err = errno;
                            addIdamError(SYSTEMERRORTYPE, "protocolXML", err,
                                         "Unable to Obtain a Temporary File Name [3]");
                            err = 998;
                            addIdamError(CODEERRORTYPE, "protocolXML", err, tempFile);
                            UDA_LOG(UDA_LOG_DEBUG, "Unable to Obtain a Temporary File Name [3], tempFile=[%s]\n",
                                    tempFile);
                            break;
                        }
#ifdef HASHXDRFILE
                        err = receiveXDRFile(xdrs, tempFile, md);	// Receive and write the file with SHA1 hash
#else
                        err = receiveXDRFile(xdrs, tempFile);        // Receive and write the file
#endif

                        if (err != 0) break;

                        char* fname = (char*)malloc(sizeof(char) * (strlen(tempFile) + 1));
                        strcpy(fname, tempFile);
                        data_block->data = nullptr;                // No Data - not unpacked
                        data_block->opaque_block = (void*)fname;            // File name
                        data_block->opaque_type = UDA_OPAQUE_TYPE_XDRFILE;        // The data block is carrying the filename only

// The temporary file is essentially cached
// The opaque type has been changed so the future receiving server or client knows what it is and how to de-serialise it.

                    } else if (option == 6) {

// Receive size, bytes, hash

                        rc = xdrrec_skiprecord(xdrs);

                        count = 0;
                        rc = rc && xdr_int(xdrs, &count);
                        objectSize = (size_t)count;

                        if (rc) object = (unsigned char*)malloc(objectSize * sizeof(unsigned char));

                        rc = rc && xdr_opaque(xdrs, (char*)object, (unsigned int)objectSize)
                             && xdr_vector(xdrs, (char*)md, hashSize, sizeof(char), (xdrproc_t)xdr_char);

                        data_block->data = nullptr;                // No Data - not unpacked
                        data_block->opaque_block = (void*)object;            // data object (needs to be freed when the Data_Block is sent)
                        data_block->opaque_count = (int)objectSize;
                        data_block->opaque_type = UDA_OPAQUE_TYPE_XDROBJECT;

                        //data_block->hash = md;
// Check the hash

#ifdef HASHXDR
                        sha1Block(object, objectSize, mdr);
                        rc = 1;
                        for (int i = 0; i < MAXELEMENTSHA1; i++) rc = rc && (md[i] == mdr[i]);
                        if (!rc) {
                            // ERROR
                        }
#endif

// The opaque type has been changed so the future receiving server or client knows what it is and how to de-serialise it.

                    }

// Unpack data structures

                    if (option == 1 || option == 2 || option == 5) {
                        logmalloclist = (LOGMALLOCLIST*)malloc(sizeof(LOGMALLOCLIST));
                        initLogMallocList(logmalloclist);

                        userdefinedtypelist = (USERDEFINEDTYPELIST*)malloc(sizeof(USERDEFINEDTYPELIST));
                        USERDEFINEDTYPE* udt_received = (USERDEFINEDTYPE*)malloc(sizeof(USERDEFINEDTYPE));

                        initUserDefinedTypeList(userdefinedtypelist);

                        UDA_LOG(UDA_LOG_DEBUG, "xdr_userdefinedtypelist #A\n");

#ifndef FATCLIENT

                        UDA_LOG(UDA_LOG_DEBUG, "privateFlags   : %d \n", privateFlags);
                        UDA_LOG(UDA_LOG_DEBUG, "protocolVersion: %d \n", protocolVersion);

                        if (option == 2) {

// Create a temporary XDR file and receive data

                            UDA_LOG(UDA_LOG_DEBUG, "creating temporary/cached XDR file\n");

                            errno = 0;
                            if (mkstemp(tempFile) < 0 || errno != 0) {
                                err = 997;
                                if (errno != 0) err = errno;
                                addIdamError(SYSTEMERRORTYPE, "protocolXML", err,
                                             " Unable to Obtain a Temporary File Name [2]");
                                err = 997;
                                addIdamError(CODEERRORTYPE, "protocolXML", err, tempFile);
                                UDA_LOG(UDA_LOG_DEBUG, "Unable to Obtain a Temporary File Name [2], tempFile=[%s]\n",
                                        tempFile);
                                break;
                            }

#ifdef HASHXDRFILE
                            err = receiveXDRFile(xdrs, tempFile, md);		// Receive and write the file with SHA1 hash
#else
                            err = receiveXDRFile(xdrs, tempFile);        // Receive and write the file
#endif

// Create input xdr file stream

                            if ((xdrfile = fopen(tempFile, "rb")) == nullptr) {    // Read temporary file
                                err = 999;
                                addIdamError(SYSTEMERRORTYPE, "protocolXML", err,
                                             " Unable to Open a Temporary XDR File for Writing");
                                break;
                            }

                            XDRstdioFlag = 1;
                            xdrstdio_create(&XDRInput, xdrfile, XDR_DECODE);
                            xdrs = &XDRInput;                        // Switch from stream to file

                        } else if (option == 5) {

// Receive size, bytes, hash

                            rc = xdrrec_skiprecord(xdrs);

                            count = 0;
                            rc = rc && xdr_int(xdrs, &count);
                            objectSize = (size_t)count;

                            if (rc) object = (unsigned char*)malloc(objectSize * sizeof(unsigned char));

                            rc = rc && xdr_opaque(xdrs, (char*)object, (unsigned int)objectSize)
                                 && xdr_vector(xdrs, (char*)md, hashSize, sizeof(char), (xdrproc_t)xdr_char);

// Check the hash

#ifdef HASHXDR
                            sha1Block(object, objectSize, mdr);
                            rc = 1;
                            for (int i = 0; i < MAXELEMENTSHA1; i++) rc = rc && (md[i] == mdr[i]);
                            if (!rc) {
                                // ERROR
                            }
#endif

// Close current input xdr stream and create a memory stream

                            XDRstdioFlag = 1;
                            xdrmem_create(&XDRInput, (char*)object, (unsigned int)objectSize, XDR_DECODE);
                            xdrs = &XDRInput;                // Switch from TCP stream to memory based object

                        }

#endif

                        // receive the full set of known named structures
                        rc = rc && xdr_userdefinedtypelist(xdrs, userdefinedtypelist);

                        UDA_LOG(UDA_LOG_DEBUG, "xdr_userdefinedtypelist #B\n");

                        if (!rc) {
                            err = 999;
                            addIdamError(CODEERRORTYPE, "protocolXML", err,
                                         "Failure receiving Structure Definitions");
                            if (XDRstdioFlag) {
                                xdr_destroy(xdrs);        // Close the stdio stream and reuse the TCP socket stream
                                xdrs = priorxdrs;
                                XDRstdioFlag = 0;
                                if (xdrfile != nullptr) fclose(xdrfile);
                            }
                            break;
                        }

                        UDA_LOG(UDA_LOG_DEBUG, "xdrUserDefinedTypeData #A\n");
                        initUserDefinedType(udt_received);

                        rc = rc && xdrUserDefinedTypeData(xdrs, logmalloclist, userdefinedtypelist, udt_received,
                                                          &data, protocolVersion);        // receive the Data

                        UDA_LOG(UDA_LOG_DEBUG, "xdrUserDefinedTypeData #B\n");
                        if (!rc) {
                            err = 999;
                            addIdamError(CODEERRORTYPE, "protocolXML", err,
                                         "Failure receiving Data and Structure Definition");
                            if (XDRstdioFlag) {
                                xdr_destroy(xdrs);        // Close the stdio stream and reuse the TCP socket stream
                                xdrs = priorxdrs;
                                XDRstdioFlag = 0;
                                if (xdrfile != nullptr) fclose(xdrfile);
                            }
                            break;
                        }

#ifndef FATCLIENT

                        if (option == 2) {

                            // Close the stream and file

                            fflush(xdrfile);

                            // Switch back to the normal xdr record stream

                            xdr_destroy(xdrs);        // Close the stdio stream and reuse the TCP socket stream
                            xdrs = priorxdrs;
                            XDRstdioFlag = 0;

                            fclose(xdrfile);
                            remove(tempFile);

                        } else if (option == 5) {

                            // Switch back to the normal xdr record stream

                            xdr_destroy(xdrs);        // Close the stdio stream and reuse the TCP socket stream
                            xdrs = priorxdrs;
                            XDRstdioFlag = 0;

                            // Free the object

                            if (object != nullptr) free(object);
                            object = nullptr;
                            objectSize = 0;
                        }

#endif    // !FATCLIENT

                        if (STR_EQUALS(udt_received->name, "SARRAY")) {            // expecting this carrier structure

                            GENERAL_BLOCK* general_block = (GENERAL_BLOCK*)malloc(sizeof(GENERAL_BLOCK));

                            SARRAY* s = (SARRAY*)data;
                            if (s->count != data_block->data_n) {                // check for consistency
                                err = 999;
                                addIdamError(CODEERRORTYPE, "protocolXML", err,
                                             "Inconsistent S Array Counts");
                                break;
                            }
                            data_block->data = (char*)fullNTree;        // Global Root Node with the Carrier Structure containing data
                            data_block->opaque_block = (void*)general_block;
                            general_block->userdefinedtype = udt_received;
                            general_block->userdefinedtypelist = userdefinedtypelist;
                            general_block->logmalloclist = logmalloclist;
                            general_block->lastMallocIndex = 0;

                        } else {
                            err = 999;
                            addIdamError(CODEERRORTYPE, "protocolXML", err,
                                         "Name of Received Data Structure Incorrect");
                            break;
                        }
                    }
                }

#ifdef FATCLIENT

                } else {
                    err = 999;
                    addIdamError(CODEERRORTYPE, "protocolXML", err, "Unknown Opaque type");
                    break;
                }
            }

#else

                //====================================================================================================================
                // Passing temporary XDR files or objects: server to server (protocolVersion >= 5 && packageType == PACKAGE_XDRFILE)

            } else {

                if (data_block->opaque_type == UDA_OPAQUE_TYPE_XDROBJECT) {

                    if (xdrs->x_op == XDR_ENCODE) {
                        UDA_LOG(UDA_LOG_DEBUG, "Forwarding XDR Objects\n");

                        // Send size, bytes, hash

                        rc = xdr_int(xdrs, &(data_block->opaque_count))
                             && xdr_opaque(xdrs, (char*)data_block->opaque_block, data_block->opaque_count)
                             && xdr_vector(xdrs, (char*)md, hashSize, sizeof(char), (xdrproc_t)xdr_char)
                             && xdrrec_endofrecord(xdrs, 1);
                        // Check the hash

#  ifdef HASHXDR
                        sha1Block(object, objectSize, mdr);
                        rc = 1;
                        for (int i = 0; i < MAXELEMENTSHA1; i++) rc = rc && (md[i] == mdr[i]);
                        if (!rc) {
                            // ERROR
                        }
#  endif


                    } else {
                        UDA_LOG(UDA_LOG_DEBUG, "Receiving forwarded XDR File\n");

                        // Receive size, bytes, hash

                        rc = xdrrec_skiprecord(xdrs);

                        count = 0;
                        rc = rc && xdr_int(xdrs, &count);
                        objectSize = (size_t)count;

                        if (rc) object = (unsigned char*)malloc(objectSize * sizeof(unsigned char));

                        rc = rc && xdr_opaque(xdrs, (char*)object, (unsigned int)objectSize);
                        rc = rc && xdr_vector(xdrs, (char*)md, hashSize, sizeof(char), (xdrproc_t)xdr_char);

                        // Check the hash

#  ifdef HASHXDR
                        sha1Block(object, objectSize, mdr);
                        rc = 1;
                        for (int i = 0; i < MAXELEMENTSHA1; i++) rc = rc && (md[i] == mdr[i]);
                        if (!rc) {
                            // ERROR
                        }
#  endif

                        if (privateFlags & PRIVATEFLAG_XDROBJECT) {        // Forward the object again

                            data_block->data = nullptr;            // No Data - not unpacked
                            data_block->opaque_block = (void*)object;    // data object (needs to be freed when the Data_Block is sent)
                            data_block->opaque_count = (int)objectSize;
                            data_block->opaque_type = UDA_OPAQUE_TYPE_XDROBJECT;

                            UDA_LOG(UDA_LOG_DEBUG, "Forwarding Received forwarded XDR Object\n");
                        } else {
                            UDA_LOG(UDA_LOG_DEBUG, "Unpacking forwarded XDR Object\n");

                            // Unpack the data Structures

                            logmalloclist = (LOGMALLOCLIST*)malloc(sizeof(LOGMALLOCLIST));
                            initLogMallocList(logmalloclist);

                            userdefinedtypelist = (USERDEFINEDTYPELIST*)malloc(sizeof(USERDEFINEDTYPELIST));
                            USERDEFINEDTYPE* udt_received = (USERDEFINEDTYPE*)malloc(sizeof(USERDEFINEDTYPE));

                            initUserDefinedTypeList(userdefinedtypelist);

                            // Close current input xdr stream and create a memory stream

                            XDRstdioFlag = 1;
                            xdrmem_create(&XDRInput, (char*)object, (unsigned int)objectSize, XDR_DECODE);
                            xdrs = &XDRInput;                // Switch from TCP stream to memory based object

                            rc = xdr_userdefinedtypelist(xdrs,
                                                         userdefinedtypelist);        // receive the full set of known named structures

                            if (!rc) {
                                err = 999;
                                addIdamError(CODEERRORTYPE, "protocolXML", err,
                                             "Failure receiving Structure Definitions");
                                if (XDRstdioFlag) {
                                    xdr_destroy(xdrs);        // Close the stdio stream and reuse the TCP socket stream
                                    xdrs = priorxdrs;
                                    XDRstdioFlag = 0;
                                }
                                break;
                            }

                            initUserDefinedType(udt_received);

                            rc = rc && xdrUserDefinedTypeData(xdrs, logmalloclist, userdefinedtypelist, udt_received,
                                                              &data, protocolVersion);        // receive the Data

                            if (!rc) {
                                err = 999;
                                addIdamError(CODEERRORTYPE, "protocolXML", err,
                                             "Failure receiving Data and Structure Definition");
                                if (XDRstdioFlag) {
                                    xdr_destroy(xdrs);        // Close the stdio stream and reuse the TCP socket stream
                                    xdrs = priorxdrs;
                                    XDRstdioFlag = 0;
                                }
                                break;
                            }

                            // Close the stream

                            xdr_destroy(xdrs);        // Close the stdio stream and reuse the TCP socket stream
                            xdrs = priorxdrs;
                            XDRstdioFlag = 0;

                            // Write data object to cache

                            // Regular client or server

                            if (STR_EQUALS(udt_received->name,
                                           "SARRAY")) {            // expecting this carrier structure

                                GENERAL_BLOCK* general_block = (GENERAL_BLOCK*)malloc(sizeof(GENERAL_BLOCK));

                                SARRAY* s = (SARRAY*)data;
                                if (s->count != data_block->data_n) {                // check for consistency
                                    err = 999;
                                    addIdamError(CODEERRORTYPE, "protocolXML", err,
                                                 "Inconsistent S Array Counts");
                                    break;
                                }
                                data_block->data = (char*)fullNTree;        // Global Root Node with the Carrier Structure containing data
                                data_block->opaque_block = (void*)general_block;
                                data_block->opaque_type = UDA_OPAQUE_TYPE_STRUCTURES;
                                general_block->userdefinedtype = udt_received;
                                general_block->userdefinedtypelist = userdefinedtypelist;
                                general_block->logmalloclist = logmalloclist;
                                general_block->lastMallocIndex = 0;

                            } else {
                                err = 999;
                                addIdamError(CODEERRORTYPE, "protocolXML", err,
                                             "Name of Received Data Structure Incorrect");
                                break;
                            }
                        }
                    }

                } else if (data_block->opaque_type == UDA_OPAQUE_TYPE_XDRFILE) {
                    if (xdrs->x_op == XDR_ENCODE) {
                        UDA_LOG(UDA_LOG_DEBUG, "Forwarding XDR File %s\n",
                                (char*)data_block->opaque_block);

#  ifdef HASHXDRFILE
                        err = sendXDRFile(xdrs, (char *)data_block->opaque_block, md);	// Forward the xdr file
#  else
                        err = sendXDRFile(xdrs, (char*)data_block->opaque_block);        // Forward the xdr file
#  endif
                    } else {
                        UDA_LOG(UDA_LOG_DEBUG, "Receiving forwarded XDR File\n");

                        // Create a temporary XDR file, receive and write data to the file

                        errno = 0;
                        if (mkstemp(tempFile) < 0 || errno != 0) {
                            err = 996;
                            if (errno != 0) err = errno;
                            addIdamError(SYSTEMERRORTYPE, "protocolXML", err,
                                         " Unable to Obtain a Temporary File Name");
                            err = 996;
                            addIdamError(CODEERRORTYPE, "protocolXML", err, tempFile);
                            UDA_LOG(UDA_LOG_DEBUG, "Unable to Obtain a Temporary File Name, tempFile=[%s]\n",
                                    tempFile);
                            break;
                        }
#  ifdef HASHXDRFILE
                        err = receiveXDRFile(xdrs, tempFile, md);	// Receive and write the file
#  else
                        err = receiveXDRFile(xdrs, tempFile);        // Receive and write the file
#  endif
                        if (privateFlags & PRIVATEFLAG_XDRFILE) {    // Forward the file (option 3) again

                            // If this is an intermediate client then read the file without unpacking the structures

                            char* fname = (char*)malloc(sizeof(char) * (strlen(tempFile) + 1));
                            strcpy(fname, tempFile);
                            data_block->data = nullptr;                // No Data - not unpacked
                            data_block->opaque_block = (void*)fname;            // File name
                            data_block->opaque_type = UDA_OPAQUE_TYPE_XDRFILE;        // The data block is carrying the filename only

                            UDA_LOG(UDA_LOG_DEBUG, "Forwarding Received forwarded XDR File\n");
                        } else {
                            UDA_LOG(UDA_LOG_DEBUG, "Unpacking forwarded XDR File\n");

                            // Unpack the data Structures

                            logmalloclist = (LOGMALLOCLIST*)malloc(sizeof(LOGMALLOCLIST));
                            initLogMallocList(logmalloclist);

                            userdefinedtypelist = (USERDEFINEDTYPELIST*)malloc(sizeof(USERDEFINEDTYPELIST));
                            USERDEFINEDTYPE* udt_received = (USERDEFINEDTYPE*)malloc(sizeof(USERDEFINEDTYPE));

                            initUserDefinedTypeList(userdefinedtypelist);

                            // Create input xdr file stream

                            if ((xdrfile = fopen(tempFile, "rb")) == nullptr) {    // Read temporary file
                                err = 999;
                                addIdamError(SYSTEMERRORTYPE, "protocolXML", err,
                                             " Unable to Open a Temporary XDR File for Writing");
                                break;
                            }

                            XDRstdioFlag = 1;
                            xdrstdio_create(&XDRInput, xdrfile, XDR_DECODE);
                            xdrs = &XDRInput;

                            rc = xdr_userdefinedtypelist(xdrs, userdefinedtypelist);
                            // receive the full set of known named structures

                            if (!rc) {
                                err = 999;
                                addIdamError(CODEERRORTYPE, "protocolXML", err,
                                             "Failure receiving Structure Definitions");
                                if (XDRstdioFlag) {
                                    xdr_destroy(xdrs);        // Close the stdio stream and reuse the TCP socket stream
                                    xdrs = priorxdrs;
                                    XDRstdioFlag = 0;
                                }
                                break;
                            }

                            initUserDefinedType(udt_received);

                            rc = rc && xdrUserDefinedTypeData(xdrs, logmalloclist, userdefinedtypelist, udt_received,
                                                              &data, protocolVersion);        // receive the Data

                            if (!rc) {
                                err = 999;
                                addIdamError(CODEERRORTYPE, "protocolXML", err,
                                             "Failure receiving Data and Structure Definition");
                                if (XDRstdioFlag) {
                                    xdr_destroy(xdrs);        // Close the stdio stream and reuse the TCP socket stream
                                    xdrs = priorxdrs;
                                    XDRstdioFlag = 0;
                                }
                                break;
                            }

                            // Close the stream and file

                            xdr_destroy(xdrs);        // Close the stdio stream and reuse the TCP socket stream
                            xdrs = priorxdrs;
                            XDRstdioFlag = 0;

                            fclose(xdrfile);

                            // Remove the Temporary File
                            remove(tempFile);

                            // Regular client or server

                            if (STR_EQUALS(udt_received->name,
                                           "SARRAY")) {            // expecting this carrier structure

                                GENERAL_BLOCK* general_block = (GENERAL_BLOCK*)malloc(sizeof(GENERAL_BLOCK));

                                SARRAY* s = (SARRAY*)data;
                                if (s->count != data_block->data_n) {                // check for consistency
                                    err = 999;
                                    addIdamError(CODEERRORTYPE, "protocolXML", err,
                                                 "Inconsistent S Array Counts");
                                    break;
                                }
                                data_block->data = (char*)fullNTree;        // Global Root Node with the Carrier Structure containing data
                                data_block->opaque_block = (void*)general_block;
                                data_block->opaque_type = UDA_OPAQUE_TYPE_STRUCTURES;
                                general_block->userdefinedtype = udt_received;
                                general_block->userdefinedtypelist = userdefinedtypelist;
                                general_block->logmalloclist = logmalloclist;
                                general_block->lastMallocIndex = 0;

                            } else {
                                err = 999;
                                addIdamError(CODEERRORTYPE, "protocolXML", err,
                                             "Name of Received Data Structure Incorrect");
                                break;
                            }
                        }
                    }
                } else {
                    err = 999;
                    addIdamError(CODEERRORTYPE, "protocolXML", err, "Unknown Opaque type");
                    break;
                }
            }
        }
#endif

        //----------------------------------------------------------------------------
        // Meta Data XML

#ifndef FATCLIENT

        if (protocol_id == PROTOCOL_META) {
            data_block = (DATA_BLOCK*)str;
            if (data_block->opaque_type == UDA_OPAQUE_TYPE_XML_DOCUMENT && data_block->opaque_count > 0) {
                switch (direction) {
                    case XDR_RECEIVE:
                        if (!(rc = xdrrec_skiprecord(xdrs))) {
                            err = PROTOCOL_ERROR_5;
                            break;
                        }
                        if ((data_block->opaque_block = (char*)malloc(
                                (data_block->opaque_count + 1) * sizeof(char))) == nullptr) {
                            err = 991;
                            break;
                        }
                        if (!(rc = xdr_meta(xdrs, data_block))) {
                            err = 992;
                            break;
                        }
                        break;

                    case XDR_SEND:

                        if (!(rc = xdr_meta(xdrs, data_block))) {
                            UDA_LOG(UDA_LOG_DEBUG, "Error sending Metadata XML Document: \n%s\n\n",
                                    (char*)data_block->opaque_block);
                            err = 990;
                            break;
                        }
                        if (!(rc = xdrrec_endofrecord(xdrs, 1))) {
                            err = PROTOCOL_ERROR_7;
                            break;
                        }
                        break;

                    case XDR_FREE_HEAP:
                        break;

                    default:
                        err = PROTOCOL_ERROR_4;
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

#ifdef HASHXDR

// include libcrypto.so in link step
// The hash output is 20 bytes
// http://linux.die.net/man/3/sha1_update

// Hash a complete data block
void sha1Block(unsigned char* block, size_t blockSize, unsigned char* md)
{
    SHA1(block, blockSize, md);
}

#endif

#ifndef FATCLIENT

int unpackXDRFile(LOGMALLOCLIST* logmalloclist, XDR* xdrs, unsigned char* filename, DATA_BLOCK* data_block, int protocolVersion)
{
    int rc = 1, err = 0;
    void* data = nullptr;
    XDR XDRInput;
    FILE* xdrfile = nullptr;
    XDR* priorxdrs = xdrs;        // Preserve the current stream object

    UDA_LOG(UDA_LOG_DEBUG, "unpackXDRFile: Unpacking XDR Data Files\n");

    // Unpack the data Structures

    logmalloclist = (LOGMALLOCLIST*)malloc(sizeof(LOGMALLOCLIST));
    initLogMallocList(logmalloclist);

    USERDEFINEDTYPELIST* userdefinedtypelist = (USERDEFINEDTYPELIST*)malloc(sizeof(USERDEFINEDTYPELIST));
    USERDEFINEDTYPE* udt_received = (USERDEFINEDTYPE*)malloc(sizeof(USERDEFINEDTYPE));

    initUserDefinedTypeList(userdefinedtypelist);

    // Close current input xdr stream and create a file stream

    if ((xdrfile = fopen((char*)filename, "rb")) == nullptr) {    // Read temporary file
        err = 999;
        addIdamError(SYSTEMERRORTYPE, "unpackXDRFile", err, " Unable to Open a XDR File for Reading");
        return err;
    }

    XDRstdioFlag = 1;
    xdrstdio_create(&XDRInput, xdrfile, XDR_DECODE);
    xdrs = &XDRInput;                        // Switch from stream to file

    do {    // Error trap

        // Unpack the associated set of named structures

        rc = xdr_userdefinedtypelist(xdrs, userdefinedtypelist);

        if (!rc) {
            err = 999;
            addIdamError(CODEERRORTYPE, "unpackXDRFile", err,
                         "Failure receiving Structure Definitions");
            if (XDRstdioFlag) {
                xdr_destroy(xdrs);        // Close the stdio stream and reuse the TCP socket stream
                xdrs = priorxdrs;
                XDRstdioFlag = 0;
            }
            break;
        }

        // Unpack the Data

        initUserDefinedType(udt_received);

        rc = rc && xdrUserDefinedTypeData(xdrs, logmalloclist, userdefinedtypelist, udt_received, &data, protocolVersion);

        if (!rc) {
            err = 999;
            addIdamError(CODEERRORTYPE, "unpackXDRFile", err,
                         "Failure receiving Data and Structure Definition");
            if (XDRstdioFlag) {
                xdr_destroy(xdrs);        // Close the stdio stream and reuse the TCP socket stream
                xdrs = priorxdrs;
                XDRstdioFlag = 0;
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
    XDRstdioFlag = 0;

    if (err != 0) return err;

    // Return data tree

    if (STR_EQUALS(udt_received->name, "SARRAY")) {            // expecting this carrier structure

        GENERAL_BLOCK* general_block = (GENERAL_BLOCK*)malloc(sizeof(GENERAL_BLOCK));

        SARRAY* s = (SARRAY*)data;
        if (s->count != data_block->data_n) {                // check for consistency
            err = 999;
            addIdamError(CODEERRORTYPE, "unpackXDRFile", err, "Inconsistent SARRAY Counts");
            return err;
        }

        data_block->data = (char*)fullNTree;        // Global Root Node with the Carrier Structure containing data
        data_block->opaque_block = (void*)general_block;
        data_block->opaque_type = UDA_OPAQUE_TYPE_STRUCTURES;
        general_block->userdefinedtype = udt_received;
        general_block->userdefinedtypelist = userdefinedtypelist;
        general_block->logmalloclist = logmalloclist;
        general_block->lastMallocIndex = 0;

    } else {
        err = 999;
        addIdamError(CODEERRORTYPE, "unpackXDRFile", err, "Name of Received Data Structure Incorrect");
        return err;
    }

    return err;
}

int unpackXDRObject(LOGMALLOCLIST* logmalloclist, XDR* xdrs, unsigned char* object, size_t objectSize,
                    DATA_BLOCK* data_block, int protocolVersion)
{

    int rc = 1, err = 0;
    void* data = nullptr;
    XDR XDRInput;
    XDR* priorxdrs = xdrs;        // Preserve the current stream object

    UDA_LOG(UDA_LOG_DEBUG, "unpackXDRObject: Unpacking XDR Data Object\n");

    // Unpack the data Structures

    logmalloclist = (LOGMALLOCLIST*)malloc(sizeof(LOGMALLOCLIST));
    initLogMallocList(logmalloclist);

    USERDEFINEDTYPELIST* userdefinedtypelist = (USERDEFINEDTYPELIST*)malloc(sizeof(USERDEFINEDTYPELIST));
    USERDEFINEDTYPE* udt_received = (USERDEFINEDTYPE*)malloc(sizeof(USERDEFINEDTYPE));

    initUserDefinedTypeList(userdefinedtypelist);

    // Create a memory stream

    XDRstdioFlag = 1;
    xdrmem_create(&XDRInput, (char*)object, (unsigned int)objectSize, XDR_DECODE);
    xdrs = &XDRInput;

    do {    // Error trap

        // Unpack the associated set of named structures

        rc = xdr_userdefinedtypelist(xdrs, userdefinedtypelist);

        if (!rc) {
            err = 999;
            addIdamError(CODEERRORTYPE, "unpackXDRObject", err,
                         "Failure receiving Structure Definitions");
            if (XDRstdioFlag) {
                xdr_destroy(xdrs);        // Close the stdio stream and reuse the TCP socket stream
                xdrs = priorxdrs;
                XDRstdioFlag = 0;
            }
            break;
        }

        // Unpack the Data

        initUserDefinedType(udt_received);

        rc = rc && xdrUserDefinedTypeData(xdrs, logmalloclist, userdefinedtypelist, udt_received, &data, protocolVersion);

        if (!rc) {
            err = 999;
            addIdamError(CODEERRORTYPE, "unpackXDRObject", err,
                         "Failure receiving Data and Structure Definition");
            if (XDRstdioFlag) {
                xdr_destroy(xdrs);        // Close the stdio stream and reuse the TCP socket stream
                xdrs = priorxdrs;
                XDRstdioFlag = 0;
            }
            break;
        }

    } while (0);

// Switch back to the normal xdr record stream

    xdr_destroy(xdrs);
    xdrs = priorxdrs;
    XDRstdioFlag = 0;

    if (err != 0) return err;

    // Return data tree

    if (STR_EQUALS(udt_received->name, "SARRAY")) {            // expecting this carrier structure

        GENERAL_BLOCK* general_block = (GENERAL_BLOCK*)malloc(sizeof(GENERAL_BLOCK));

        SARRAY* s = (SARRAY*)data;
        if (s->count != data_block->data_n) {                // check for consistency
            err = 999;
            addIdamError(CODEERRORTYPE, "unpackXDRObject", err, "Inconsistent SARRAY Counts");
            return err;
        }

        data_block->data = (char*)fullNTree;        // Global Root Node with the Carrier Structure containing data
        data_block->opaque_block = (void*)general_block;
        data_block->opaque_type = UDA_OPAQUE_TYPE_STRUCTURES;
        general_block->userdefinedtype = udt_received;
        general_block->userdefinedtypelist = userdefinedtypelist;
        general_block->logmalloclist = logmalloclist;
        general_block->lastMallocIndex = 0;

    } else {
        err = 999;
        addIdamError(CODEERRORTYPE, "unpackXDRObject", err,
                     "Name of Received Data Structure Incorrect");
        return err;
    }

    return err;
}

//==========================================================================================================================
// Serialise a regular Data_Block structure
// Write to a memory block - the data object - using a memory stream

int packXDRDataBlockObject(unsigned char* object, size_t objectSize, DATA_BLOCK* data_block,
                           LOGMALLOCLIST* logmalloclist, USERDEFINEDTYPELIST* userdefinedtypelist, int protocolVersion)
{
    int err = 0;
    XDR xdrObject;
    FILE* xdrfile = nullptr;

#ifdef CACHEDEV
    if(data_block->totalDataBlockSize > 1000000 || data_block->data_type == UDA_TYPE_UNKNOWN) return 0;		// Size is too large and the type is not Atomic
#endif
    do {

        errno = 0;

#ifndef _WIN32
        xdrfile = open_memstream((char**)&object, &objectSize);
#else
        xdrfile = tmpfile();
        fwrite(object, objectSize, 1, xdrfile);
        fflush(xdrfile);
        rewind(xdrfile);
#endif

        if (xdrfile == nullptr || errno != 0) {
            err = 999;
            if (errno != 0) {
                addIdamError(SYSTEMERRORTYPE, "packXDRDataBlockObject", errno, "");
            }
            addIdamError(CODEERRORTYPE, "packXDRDataBlockObject", err,
                         "Unable to Open a XDR Memory Stream for Writing data objects");
            break;
        }

        // Create stdio file stream

        XDRstdioFlag = 1;
        xdrstdio_create(&xdrObject, xdrfile, XDR_ENCODE);

        // Data object meta data

        // Serialise the structure and create the data object

        DATA_BLOCK_LIST data_block_list;
        data_block_list.count = 1;
        data_block_list.data = data_block;
        err = protocol2(&xdrObject, PROTOCOL_DATA_BLOCK_LIST, XDR_SEND, nullptr, logmalloclist, userdefinedtypelist,
                        &data_block_list, protocolVersion);

        // Close the stream and file

        fflush(xdrfile);
        fclose(xdrfile);

        xdr_destroy(&xdrObject);
        XDRstdioFlag = 0;

    } while (0);

    return err;
}

// Deserialise a regular Data_Block structure
// Read from a memory block - the data object - using a memory stream

int unpackXDRDataBlockObject(unsigned char* object, size_t objectSize, DATA_BLOCK* data_block,
                             LOGMALLOCLIST* logmalloclist, USERDEFINEDTYPELIST* userdefinedtypelist, int protocolVersion)
{
    int err = 0;
    XDR xdrObject;

    if (objectSize > 1000000) return 0;        // Check type is Atomic

    do {

        // Create a memory stream

        xdrmem_create(&xdrObject, (char*)object, (unsigned int)objectSize, XDR_DECODE);

        XDRstdioFlag = 1;

        // Data object meta data

        // Deserialise the object and create the data_block structure

        DATA_BLOCK_LIST data_block_list;
        data_block_list.count = 1;
        data_block_list.data = data_block;
        err = protocol2(&xdrObject, PROTOCOL_DATA_BLOCK_LIST, XDR_RECEIVE, nullptr, logmalloclist, userdefinedtypelist,
                        &data_block_list, protocolVersion);

        // Close the stream

        xdr_destroy(&xdrObject);
        XDRstdioFlag = 0;

    } while (0);

    return err;
}

#endif    // !FATCLIENT
