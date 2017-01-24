#ifndef IdamProtocolXML
#define IdamProtocolXML        // Cheap fix to Prevent double inclusion in build for FAT clients

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
* Change History:

* 1.0  31May2007 D.G.Muir
* 25Nov2009	DGMuir	Added generalised stuctures
* 26Apr2010	DGMuir	Server to Server communication via SENDFILE temporary (cached) xdr files
* 18May2010	DGMuir	SENDFILE abandoned: use xdr instead. Temporary file deleted after send.
* 08Jul2010	DGMuir	rc initialised if fatclient before receiving structure definition data
* 28May2012	DGMuir	Added caching of XDR files to minimise network trafic
* 06Noc2012	DGMuir	Added lastMallocIndex to GENERAL_BLOCK
*-----------------------------------------------------return;---------------------------------------------
* Notes on Generalised Data Structures:
*
* The DATA_BLOCK structure has the following fields used to pass and receive generalised data structures
*
* data_block->data_type		set to TYPE_COMPOUND (external to this routine)
* data_block->data_n		set to the count of structure array elements (external to this routine)
*
* data_block->data		sending (server side): set to the data array (external to this routine)
*				receiving (client side): set to the root data tree node within this routine
*
* data_block->opaque_type	set to OPAQUE_TYPE_STRUCTURES (external to this routine)
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

#include "protocolXML.h"

#include <stdlib.h>
#include <errno.h>
#include <memory.h>
#include <logging/idamLog.h>
#include <include/idamclientserverprivate.h>
#include <include/idamclientserver.h>
#include "readXDRFile.h"
#include "idamErrorLog.h"
#include <structures/struct.h>
#include "xdrlib.h"
#include "idamErrors.h"

#ifdef SERVERBUILD
#  include "idamserver.h"
#  include "CreateXDRStream.h"
#  include "idamServerStartup.h"
#elif !defined(FATCLIENT)
#  include <client/createClientXDRStream.h>
#  include <cache/idamFileCache.h>
#  include <include/idamclientprivate.h>
#endif

int protocolXML(XDR* xdrs, int protocol_id, int direction, int* token, void* str)
{

    DATA_BLOCK* data_block;

#ifdef HIERARCHICAL_DATA
    PFCOILS *pfcoils;
    PFPASSIVE *pfpassive;
    PFSUPPLIES *pfsupplies;
    FLUXLOOP  *fluxloop;
    MAGPROBE  *magprobe;
    PFCIRCUIT *pfcircuit;
    PLASMACURRENT *plasmacurrent;
    DIAMAGNETIC *diamagnetic;
    TOROIDALFIELD *toroidalfield;
    LIMITER *limiter;
    EFIT *efit;
#endif

    int rc;
    int err = 0;
#ifndef FATCLIENT
    XDR XDRInput;
    XDR XDROutput;
    FILE* xdrfile = NULL;
#endif

    char tempFile[MAXPATH] = "/tmp/idamXDRXXXXXX";
    char* env = NULL;

    if ((privateFlags & PRIVATEFLAG_XDRFILE) && protocolVersion >= 5) {
#ifdef FILECACHE
        if (privateFlags & PRIVATEFLAG_CACHE)
            env = getenv("IDAM_CACHE_DIR");    // Use a specific directory to cache XDR files
        else
            env = getenv("IDAM_WORK_DIR");        // Use a general work area directory for temporary XDR files
#else
        if((env = getenv("IDAM_WORK_DIR")) != NULL)
#endif

        if (env != NULL) sprintf(tempFile, "%s/idamXDRXXXXXX", env);    // File to record XDR encoded data
    }

//----------------------------------------------------------------------------
// Check Versions

// output (ENCODE) means written by the server
// input (DECODE) means read by the client
// xdrs->x_op == XDR_DECODE && protocolVersion == 2 Means Client receiving data from a Version 2 Server
// xdrs->x_op == XDR_ENCODE && protocolVersion == 3 Means Server sending data to a Version 3 Client

#ifndef FATCLIENT

    if ((xdrs->x_op == XDR_DECODE && protocolVersion < 3) || (xdrs->x_op == XDR_ENCODE && protocolVersion < 3))
        return 0;

#endif
//----------------------------------------------------------------------------
// Error Management Loop

    do {

//----------------------------------------------------------------------------
// Generalised User Defined Data Structures

#ifdef GENERALSTRUCTS
        if (protocol_id == PROTOCOL_STRUCTURES) {

            void* data = NULL;
            data_block = (DATA_BLOCK*) str;

            if (data_block->opaque_type == OPAQUE_TYPE_STRUCTURES) {
                int packageType = 0;

                idamLog(LOG_DEBUG, "protocolXML: Compound Data Structure\n");
                idamLog(LOG_DEBUG, "direction  : %d [%d][%d]\n", (int) xdrs->x_op, XDR_ENCODE, XDR_DECODE);

                if (xdrs->x_op == XDR_ENCODE) {        // Send Data

                    SARRAY sarray;                                // Structure array carrier structure
                    SARRAY* psarray = &sarray;
                    int shape = data_block->data_n;                                     // rank 1 array of dimension lengths
                    USERDEFINEDTYPE* udt = (USERDEFINEDTYPE*) data_block->opaque_block; // The data's structure definition
                    USERDEFINEDTYPE* u = findUserDefinedType("SARRAY", 0);              // Locate the carrier structure definition

                    idamLog(LOG_DEBUG, "protocolXML: Sending to Client\n");

                    if (udt == NULL || u == NULL) {
                        err = 999;
                        idamLog(LOG_DEBUG, "protocolXML: NULL SARRAY User defined data Structure Definition\n");
                        printUserDefinedTypeListTable(*userdefinedtypelist);
                        addIdamError(&idamerrorstack, CODEERRORTYPE, "protocolXML", err,
                                     "NULL User defined data Structure Definition");
                        break;
                    }

                    idamLog(LOG_DEBUG, "protocolXML: Creating SARRAY carrier structure\n");

                    initSArray(&sarray);
                    sarray.count = data_block->data_n;              // Number of this structure
                    sarray.rank = 1;                                // Array Data Rank?
                    sarray.shape = &shape;                          // Only if rank > 1?
                    sarray.data = (void*) data_block->data;         // Pointer to the data to be passed
                    strcpy(sarray.type, udt->name);                 // The name of the type
                    data = (void*) &psarray;                        // Pointer to the SARRAY array pointer
                    addNonMalloc((void*) &shape, 1, sizeof(int), "int");

                    idamLog(LOG_DEBUG, "protocolXML: sending Structure Definitions\n");

                    rc = 1;

#  ifndef FATCLIENT

                    // If access is server to server then avoid multiple write/reads of structure components over xdr by creating a
                    // temporary xdr file and passing the file. Structures need only be created in the originating client, not the
                    // intermediate server clients. Control using a global properties flag: privateFlags - passed from the originating client
                    // to all servers along the chain

                    idamLog(LOG_DEBUG, "protocolXML: privateFlags   : %d \n", privateFlags);
                    idamLog(LOG_DEBUG, "protocolXML: protocolVersion: %d \n", protocolVersion);

                    if ((privateFlags & PRIVATEFLAG_XDRFILE) &&
                        protocolVersion >= 5) {        // Server calling another server

                        // Create a temporary or cached XDR file

                        idamLog(LOG_DEBUG, "protocolXML: creating temporary/cache XDR file\n");

                        errno = 0;
                        if (mkstemp(tempFile) < 0 || errno != 0) {
                            err = 999;
                            if (errno != 0) err = errno;
                            addIdamError(&idamerrorstack, SYSTEMERRORTYPE, "protocolXML", err,
                                         " Unable to Obtain a Temporary/Cache File Name");
                            break;
                        }
                        if ((xdrfile = fopen(tempFile, "wb")) == NULL) {
                            err = 999;
                            addIdamError(&idamerrorstack, SYSTEMERRORTYPE, "protocolXML", err,
                                         " Unable to Open a Temporary/Cache XDR File for Writing");
                            break;
                        }

                        packageType = PACKAGE_XDRFILE;          // The package is a file
                        rc = xdr_int(xdrs, &packageType);       // Send data package type
                        rc = rc && xdrrec_endofrecord(xdrs, 1);

                        // Close current output xdr stream and create stdio file stream

                        xdr_destroy(xdrs);
                        XDRstdioFlag = 1;
                        xdrstdio_create(&XDROutput, xdrfile, XDR_ENCODE);
                        xdrs = &XDROutput;                        // Switch from stream to file
                        idamLog(LOG_DEBUG, "protocolXML: stdio XDR file: %s\n", tempFile);

                    } else {
                        packageType = PACKAGE_STRUCTDATA;        // The package is regular XDR

                        idamLog(LOG_DEBUG, "protocolXML: Sending Package Type: %d\n", packageType);

                        rc = xdr_int(xdrs, &packageType);        // Send data package type
                        rc = rc && xdrrec_endofrecord(xdrs, 1);

                    }
#  endif

                    rc = rc && xdr_userdefinedtypelist(xdrs,
                                                       userdefinedtypelist);        // send the full set of known named structures

                    idamLog(LOG_DEBUG, "protocolXML: Structure Definitions sent: rc = %d\n", rc);

                    rc = rc && xdrUserDefinedTypeData(xdrs, u, data);        // send the Data

                    idamLog(LOG_DEBUG, "protocolXML: Data sent: rc = %d\n", rc);

                    if (!rc) {
                        err = 999;
                        addIdamError(&idamerrorstack, CODEERRORTYPE, "protocolXML", err,
                                     "Bad Return Code passing data structures");
                        break;
                    }

#  ifndef FATCLIENT

                    if ((privateFlags & PRIVATEFLAG_XDRFILE) &&
                        protocolVersion >= 5) {        // Server calling another server

// Close the stream and file

                        fflush(xdrfile);
                        fclose(xdrfile);

// Create the XDR Record Streams


#    ifdef SERVERBUILD
                        CreateXDRStream();
                        xdrs = serverOutput;
#    else
                        idamCreateXDRStream();
                        xdrs = clientOutput;
#    endif
                        XDRstdioFlag = 0;

// Send the Temporary File

                        idamLog(LOG_DEBUG, "protocolXML: sending temporary XDR file\n");

                        err = sendXDRFile(xdrs, tempFile);        // Read and send

#    ifdef FILECACHE

// Write cache file metadata

                        char* p = strrchr(tempFile, '/');
                        if (err == 0) rc = idamClientWriteCache(&p[1]);

                        if (err != 0 || rc != 0) remove(tempFile);    // bad file!
#    else

                        // Remove the Temporary File (regardless of success or otherwise of the send)
                        
                                                remove(tempFile);
#    endif

                        if (err != 0) break;
                    }
#  endif // !FATCLIENT

//======================================================================================================================

                } else {            // Receive Data

                    idamLog(LOG_DEBUG, "protocolXML: Receiving from Server\n");


// 3 valid options:
//	1> unpack structures, no xdr file involved	=> privateFlags & PRIVATEFLAG_XDRFILE == 0 && packageType == PACKAGE_STRUCTDATA
//	2> unpack structures, from an xdr file		=> privateFlags & PRIVATEFLAG_XDRFILE == 0 && packageType == PACKAGE_XDRFILE
//	3> xdr file only, no unpacking, passforward	=> privateFlags & PRIVATEFLAG_XDRFILE == 1 && packageType == PACKAGE_XDRFILE
//	4> Error					=> privateFlags & PRIVATEFLAG_XDRFILE == 1 && packageType == PACKAGE_STRUCTDATA
//
// Option 3 does not include intermediate file caching - option 2 only

                    int option = 4;

#  ifndef FATCLIENT
                    idamLog(LOG_DEBUG, "protocolXML: Receiving Package Type\n");

                    rc = xdrrec_skiprecord(xdrs);
                    rc = rc && xdr_int(xdrs, &packageType);        // Receive data package type
#  else
                    rc = 1;

                    if (privateFlags & PRIVATEFLAG_XDRFILE)
                        packageType = PACKAGE_XDRFILE;
                    else
                        packageType = PACKAGE_STRUCTDATA;
#  endif

                    if ((privateFlags & PRIVATEFLAG_XDRFILE) == 0 && packageType == PACKAGE_STRUCTDATA) option = 1;
                    if ((privateFlags & PRIVATEFLAG_XDRFILE) == 0 && packageType == PACKAGE_XDRFILE &&
                        protocolVersion >= 5)
                        option = 2;
                    if ((privateFlags & PRIVATEFLAG_XDRFILE) == 1 && packageType == PACKAGE_XDRFILE &&
                        protocolVersion >= 5)
                        option = 3;

                    idamLog(LOG_DEBUG, "protocolXML: %d  %d\n", privateFlags & PRIVATEFLAG_XDRFILE,
                            packageType == PACKAGE_STRUCTDATA);
                    idamLog(LOG_DEBUG, "protocolXML: Receive data option : %d\n", option);
                    idamLog(LOG_DEBUG, "protocolXML: Receive package Type: %d\n", packageType);

                    if (option == 4) {
                        err = 999;
                        addIdamError(&idamerrorstack, SYSTEMERRORTYPE, "protocolXML", err,
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
                            addIdamError(&idamerrorstack, SYSTEMERRORTYPE, "protocolXML", err,
                                         "Unable to Obtain a Temporary File Name [3]");
                            err = 998;
                            addIdamError(&idamerrorstack, CODEERRORTYPE, "protocolXML", err, tempFile);
                            idamLog(LOG_DEBUG, "Unable to Obtain a Temporary File Name [3], tempFile=[%s]\n",
                                    tempFile);
                            break;
                        }

                        err = receiveXDRFile(xdrs, tempFile);        // Receive and write the file

                        char* fname = (char*) malloc(sizeof(char) * (strlen(tempFile) + 1));
                        strcpy(fname, tempFile);
                        data_block->data = NULL;                // No Data - not unpacked
                        data_block->opaque_block = (void*) fname;            // File name
                        data_block->opaque_type = OPAQUE_TYPE_XDRFILE;        // The data block is carrying the filename only

                    }

// Unpack data structures

                    if (option == 1 || option == 2) {

                        logmalloclist = (LOGMALLOCLIST*) malloc(sizeof(LOGMALLOCLIST));
                        initLogMallocList(logmalloclist);

                        userdefinedtypelist = (USERDEFINEDTYPELIST*) malloc(sizeof(USERDEFINEDTYPELIST));
                        USERDEFINEDTYPE* udt_received = (USERDEFINEDTYPE*) malloc(sizeof(USERDEFINEDTYPE));

                        initUserDefinedTypeList(userdefinedtypelist);

                        idamLog(LOG_DEBUG, "protocolXML: xdr_userdefinedtypelist #A\n");

#  ifndef FATCLIENT
                        idamLog(LOG_DEBUG, "protocolXML: privateFlags   : %d \n", privateFlags);
                        idamLog(LOG_DEBUG, "protocolXML: protocolVersion: %d \n", protocolVersion);

                        if (option == 2) {

// Create a temporary XDR file and receive data

                            idamLog(LOG_DEBUG, "protocolXML: creating temporary/cached XDR file\n");

                            errno = 0;
                            if (mkstemp(tempFile) < 0 || errno != 0) {
                                err = 997;
                                if (errno != 0) err = errno;
                                addIdamError(&idamerrorstack, SYSTEMERRORTYPE, "protocolXML", err,
                                             " Unable to Obtain a Temporary File Name [2]");
                                err = 997;
                                addIdamError(&idamerrorstack, CODEERRORTYPE, "protocolXML", err, tempFile);
                                idamLog(LOG_DEBUG, "Unable to Obtain a Temporary File Name [2], tempFile=[%s]\n",
                                        tempFile);
                                break;
                            }

                            err = receiveXDRFile(xdrs, tempFile);        // Receive and write

#    ifdef FILECACHE

// Write cache file metadata

                            char * p = strrchr(tempFile, '/');
                            if (err == 0) rc = idamClientWriteCache(&p[1]);
#    endif

// Create input xdr file stream

                            if ((xdrfile = fopen(tempFile, "rb")) == NULL) {    // Read temporary file
                                err = 999;
                                addIdamError(&idamerrorstack, SYSTEMERRORTYPE, "protocolXML", err,
                                             " Unable to Open a Temporary XDR File for Writing");
                                break;
                            }

                            xdr_destroy(xdrs);
                            XDRstdioFlag = 1;
                            xdrstdio_create(&XDRInput, xdrfile, XDR_DECODE);
                            xdrs = &XDRInput;                        // Switch from stream to file

                        }
#  endif // !FATCLIENT

                        rc = rc && xdr_userdefinedtypelist(xdrs,
                                                           userdefinedtypelist);        // receive the full set of known named structures

                        idamLog(LOG_DEBUG, "protocolXML: xdr_userdefinedtypelist #B\n");

                        if (!rc) {
                            err = 999;
                            addIdamError(&idamerrorstack, CODEERRORTYPE, "protocolXML", err,
                                         "Failure receiving Structure Definitions");
                            break;
                        }
                        idamLog(LOG_DEBUG, "protocolXML: xdrUserDefinedTypeData #A\n");
                        initUserDefinedType(udt_received);

                        rc = rc && xdrUserDefinedTypeData(xdrs, udt_received, &data);        // receive the Data

                        idamLog(LOG_DEBUG, "protocolXML: xdrUserDefinedTypeData #B\n");
                        if (!rc) {
                            err = 999;
                            addIdamError(&idamerrorstack, CODEERRORTYPE, "protocolXML", err,
                                         "Failure receiving Data and Structure Definition");
                            break;
                        }

#  ifndef FATCLIENT

                        if (option == 2) {

// Close the stream and file

                            fflush(xdrfile);
                            fclose(xdrfile);

// Switch back to the normal xdr record stream

                            //xdrs = priorxdrs;

#    ifdef SERVERBUILD
                            CreateXDRStream();
                            xdrs = serverInput;
#    else
                            idamCreateXDRStream();
                            xdrs = clientInput;
#    endif
                            XDRstdioFlag = 0;

#    ifndef FILECACHE

                            // Remove the Temporary File
                            
                                                        remove(tempFile);
#    endif

                        }
#  endif // !FATCLIENT

                        if (!strcmp(udt_received->name, "SARRAY")) {            // expecting this carrier structure

                            GENERAL_BLOCK* general_block = (GENERAL_BLOCK*) malloc(sizeof(GENERAL_BLOCK));

                            SARRAY* s = (SARRAY*) data;
                            if (s->count != data_block->data_n) {                // check for consistency
                                err = 999;
                                addIdamError(&idamerrorstack, CODEERRORTYPE, "protocolXML", err,
                                             "Inconsistent S Array Counts");
                                break;
                            }
                            data_block->data = (char*) fullNTree;        // Global Root Node with the Carrier Structure containing data
                            data_block->opaque_block = (void*) general_block;
                            general_block->userdefinedtype = udt_received;
                            general_block->userdefinedtypelist = userdefinedtypelist;
                            general_block->logmalloclist = logmalloclist;
                            general_block->lastMallocIndex = 0;

                        } else {
                            err = 999;
                            addIdamError(&idamerrorstack, CODEERRORTYPE, "protocolXML", err,
                                         "Name of Received Data Structure Incorrect");
                            break;
                        }
                    }
                }

#  ifdef FATCLIENT

                } else {
                    err = 999;
                    addIdamError(&idamerrorstack, CODEERRORTYPE, "protocolXML", err, "Unknown Opaque type");
                    break;
                }
            }

#  else

                //====================================================================================================================
                // Passing temporary XDR files: server to server (protocolVersion >= 5 is TRUE && packageType == PACKAGE_XDRFILE)

            } else {

                if (data_block->opaque_type == OPAQUE_TYPE_XDRFILE) {

                    if (xdrs->x_op == XDR_ENCODE) {
                        idamLog(LOG_DEBUG, "protocolXML: Forwarding XDR File %s\n", (char*) data_block->opaque_block);
                        err = sendXDRFile(xdrs, (char*) data_block->opaque_block);    // Forward the xdr file
                    } else {
                        idamLog(LOG_DEBUG, "protocolXML: Receiving forwarded XDR File\n");

                        // Create a temporary XDR file, receive and write data to the file

                        errno = 0;
                        if (mkstemp(tempFile) < 0 || errno != 0) {
                            err = 996;
                            if (errno != 0) err = errno;
                            addIdamError(&idamerrorstack, SYSTEMERRORTYPE, "protocolXML", err,
                                         " Unable to Obtain a Temporary File Name");
                            err = 996;
                            addIdamError(&idamerrorstack, CODEERRORTYPE, "protocolXML", err, tempFile);
                            idamLog(LOG_DEBUG, "Unable to Obtain a Temporary File Name, tempFile=[%s]\n", tempFile);
                            break;
                        }

                        err = receiveXDRFile(xdrs, tempFile);        // Receive and write the file

                        if (privateFlags & PRIVATEFLAG_XDRFILE) {    // Forward the file (option 3) again

                            // If this is an intermediate client then read the file without unpacking the structures

                            char* fname = (char*) malloc(sizeof(char) * (strlen(tempFile) + 1));
                            strcpy(fname, tempFile);
                            data_block->data = NULL;                // No Data - not unpacked
                            data_block->opaque_block = (void*) fname;            // File name
                            data_block->opaque_type = OPAQUE_TYPE_XDRFILE;        // The data block is carrying the filename only

                            idamLog(LOG_DEBUG, "protocolXML: Forwarding Received forwarded XDR File\n");
                        } else {
                            idamLog(LOG_DEBUG, "protocolXML: Unpacking forwarded XDR File\n");

                            // Unpack the data Structures

                            logmalloclist = (LOGMALLOCLIST*) malloc(sizeof(LOGMALLOCLIST));
                            initLogMallocList(logmalloclist);

                            userdefinedtypelist = (USERDEFINEDTYPELIST*) malloc(sizeof(USERDEFINEDTYPELIST));
                            USERDEFINEDTYPE* udt_received = (USERDEFINEDTYPE*) malloc(sizeof(USERDEFINEDTYPE));

                            initUserDefinedTypeList(userdefinedtypelist);

                            // Create input xdr file stream

                            if ((xdrfile = fopen(tempFile, "rb")) == NULL) {    // Read temporary file
                                err = 999;
                                addIdamError(&idamerrorstack, SYSTEMERRORTYPE, "protocolXML", err,
                                             " Unable to Open a Temporary XDR File for Writing");
                                break;
                            }

                            xdr_destroy(xdrs);
                            XDRstdioFlag = 1;
                            xdrstdio_create(&XDRInput, xdrfile, XDR_DECODE);
                            xdrs = &XDRInput;

                            rc = xdr_userdefinedtypelist(xdrs,
                                                         userdefinedtypelist);        // receive the full set of known named structures

                            if (!rc) {
                                err = 999;
                                addIdamError(&idamerrorstack, CODEERRORTYPE, "protocolXML", err,
                                             "Failure receiving Structure Definitions");
                                break;
                            }

                            initUserDefinedType(udt_received);

                            rc = rc && xdrUserDefinedTypeData(xdrs, udt_received, &data);        // receive the Data

                            if (!rc) {
                                err = 999;
                                addIdamError(&idamerrorstack, CODEERRORTYPE, "protocolXML", err,
                                             "Failure receiving Data and Structure Definition");
                                break;
                            }

                            // Close the stream and file

                            fflush(xdrfile);
                            fclose(xdrfile);

#    ifdef SERVERBUILD
                            CreateXDRStream();
                            xdrs = serverInput;
#    else
                            idamCreateXDRStream();
                            xdrs = clientInput;
#    endif
                            XDRstdioFlag = 0;

#    ifdef FILECACHE

                            // Write cache file metadata
                            char * p = strrchr(tempFile, '/');
                            rc = idamClientWriteCache(&p[1]);
                            if (rc != 0) remove(tempFile);    // bad file!
#    else

                            // Remove the Temporary File
                    
                                                        remove(tempFile);
#    endif

                            // Regular client or server

                            if (!strcmp(udt_received->name, "SARRAY")) {            // expecting this carrier structure

                                GENERAL_BLOCK* general_block = (GENERAL_BLOCK*) malloc(sizeof(GENERAL_BLOCK));

                                SARRAY* s = (SARRAY*) data;
                                if (s->count != data_block->data_n) {                // check for consistency
                                    err = 999;
                                    addIdamError(&idamerrorstack, CODEERRORTYPE, "protocolXML", err,
                                                 "Inconsistent S Array Counts");
                                    break;
                                }
                                data_block->data = (char*) fullNTree;        // Global Root Node with the Carrier Structure containing data
                                data_block->opaque_block = (void*) general_block;
                                data_block->opaque_type = OPAQUE_TYPE_STRUCTURES;
                                general_block->userdefinedtype = udt_received;
                                general_block->userdefinedtypelist = userdefinedtypelist;
                                general_block->logmalloclist = logmalloclist;
                                general_block->lastMallocIndex = 0;

                            } else {
                                err = 999;
                                addIdamError(&idamerrorstack, CODEERRORTYPE, "protocolXML", err,
                                             "Name of Received Data Structure Incorrect");
                                break;
                            }
                        }
                    }
                } else {
                    err = 999;
                    addIdamError(&idamerrorstack, CODEERRORTYPE, "protocolXML", err, "Unknown Opaque type");
                    break;
                }
            }
        }
#  endif // !FATCLIENT
#endif // GENERALSTRUCTS

//----------------------------------------------------------------------------
// Meta Data XML

#ifndef FATCLIENT

        if (protocol_id == PROTOCOL_META) {
            data_block = (DATA_BLOCK*) str;
            if (data_block->opaque_type == OPAQUE_TYPE_XML_DOCUMENT && data_block->opaque_count > 0) {
                switch (direction) {
                    case XDR_RECEIVE :
                        if (!xdrrec_skiprecord(xdrs)) {
                            err = PROTOCOL_ERROR_5;
                            break;
                        }
                        if ((data_block->opaque_block = (char*) malloc(
                                (data_block->opaque_count + 1) * sizeof(char))) == NULL) {
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
                            idamLog(LOG_DEBUG, "Error sending Metadata XML Document: \n%s\n\n",
                                    (char*) data_block->opaque_block);
                            err = 990;
                            break;
                        }
                        if (!xdrrec_endofrecord(xdrs, 1)) {
                            err = PROTOCOL_ERROR_7;
                            break;
                        }
                        break;

                    case XDR_FREE_HEAP :
                        break;

                    default:
                        err = PROTOCOL_ERROR_4;
                        break;
                }
            }
        }

#  ifdef HIERARCHICAL_DATA
        //----------------------------------------------------------------------------
        // EFIT (Uses recursion)
        
                if(protocol_id == PROTOCOL_EFIT) {
        
                    data_block = (DATA_BLOCK *)str;
        
                    switch (direction) {
        
                    case XDR_RECEIVE :
        
                        if((efit = (EFIT *)malloc(sizeof(EFIT))) == NULL) {
                            err = PROTOCOL_ERROR_1001;
                            break;
                        }
        
                        if (!(rc = xdrrec_skiprecord(xdrs))) {
                            err = PROTOCOL_ERROR_5;
                            break;
                        }
                        if(!(rc = xdr_efit(xdrs, efit))) {
                            err = PROTOCOL_ERROR_1001;
                            break;
                        }
        
                        if((err = alloc_efit(efit)) != 0) break;		// Allocate Heap Memory
        
                        for(i=0; i<efit->npfcoils; i++) {
                            pfcoils = efit->pfcoils+i;
                            if((err = protocolXML(xdrs, PROTOCOL_PFCOILS, direction, token, (void *)pfcoils)) != 0) break;
                        }
                        for(i=0; i<efit->npfpassive; i++) {
                            pfpassive = efit->pfpassive+i;
                            if((err = protocolXML(xdrs, PROTOCOL_PFPASSIVE, direction, token, (void *)pfpassive)) != 0) break;
                        }
                        for(i=0; i<efit->npfsupplies; i++) {
                            pfsupplies = efit->pfsupplies+i;
                            if((err = protocolXML(xdrs, PROTOCOL_PFSUPPLIES, direction, token, (void *)pfsupplies)) != 0) break;
                        }
                        for(i=0; i<efit->nfluxloops; i++) {
                            fluxloop = efit->fluxloop+i;
                            if((err = protocolXML(xdrs, PROTOCOL_FLUXLOOP, direction, token, (void *)fluxloop)) != 0) break;
                        }
                        for(i=0; i<efit->nmagprobes; i++) {
                            magprobe = efit->magprobe+i;
                            if((err = protocolXML(xdrs, PROTOCOL_MAGPROBE, direction, token, (void *)magprobe)) != 0) break;
                        }
                        for(i=0; i<efit->npfcircuits; i++) {
                            pfcircuit = efit->pfcircuit+i;
                            if((err = protocolXML(xdrs, PROTOCOL_PFCIRCUIT, direction, token, (void *)pfcircuit)) != 0) break;
                        }
                        for(i=0; i<efit->nplasmacurrent; i++) {
                            plasmacurrent = efit->plasmacurrent+i;
                            if((err = protocolXML(xdrs, PROTOCOL_PLASMACURRENT, direction, token, (void *)plasmacurrent)) != 0) break;
                        }
                        for(i=0; i<efit->ndiamagnetic; i++) {
                            diamagnetic = efit->diamagnetic+i;
                            if((err = protocolXML(xdrs, PROTOCOL_DIAMAGNETIC, direction, token, (void *)diamagnetic)) != 0) break;
                        }
                        for(i=0; i<efit->ntoroidalfield; i++) {
                            toroidalfield = efit->toroidalfield+i;
                            if((err = protocolXML(xdrs, PROTOCOL_TOROIDALFIELD, direction, token, (void *)toroidalfield)) != 0) break;
                        }
                        for(i=0; i<efit->nlimiter; i++) {
                            limiter = efit->limiter+i;
                            if((err = protocolXML(xdrs, PROTOCOL_LIMITER, direction, token, (void *)limiter)) != 0) break;
                        }
        
                        data_block->opaque_block = (void *) efit;
        
                        break;
        
                    case XDR_SEND:
        
                        efit = (EFIT *) data_block->opaque_block;
        
                        if(!(rc = xdr_efit(xdrs, efit))) {
                            err = PROTOCOL_ERROR_1001;
                            break;
                        }
        
                        if(!(rc = xdrrec_endofrecord(xdrs, 1))) err = PROTOCOL_ERROR_7;
        
                        for(i=0; i<efit->npfcoils; i++) {
                            pfcoils = efit->pfcoils+i;
                            if((err = protocolXML(xdrs, PROTOCOL_PFCOILS, direction, token, (void *)pfcoils)) != 0) break;
                        }
                        for(i=0; i<efit->npfpassive; i++) {
                            pfpassive = efit->pfpassive+i;
                            if((err = protocolXML(xdrs, PROTOCOL_PFPASSIVE, direction, token, (void *)pfpassive)) != 0) break;
                        }
                        for(i=0; i<efit->npfsupplies; i++) {
                            pfsupplies = efit->pfsupplies+i;
                            if((err = protocolXML(xdrs, PROTOCOL_PFSUPPLIES, direction, token, (void *)pfsupplies)) != 0) break;
                        }
                        for(i=0; i<efit->nfluxloops; i++) {
                            fluxloop = efit->fluxloop+i;
                            if((err = protocolXML(xdrs, PROTOCOL_FLUXLOOP, direction, token, (void *)fluxloop)) != 0) break;
                        }
                        for(i=0; i<efit->nmagprobes; i++) {
                            magprobe = efit->magprobe+i;
                            if((err = protocolXML(xdrs, PROTOCOL_MAGPROBE, direction, token, (void *)magprobe)) != 0) break;
                        }
                        for(i=0; i<efit->npfcircuits; i++) {
                            pfcircuit = efit->pfcircuit+i;
                            if((err = protocolXML(xdrs, PROTOCOL_PFCIRCUIT, direction, token, (void *)pfcircuit)) != 0) break;
                        }
                        for(i=0; i<efit->nplasmacurrent; i++) {
                            plasmacurrent = efit->plasmacurrent+i;
                            if((err = protocolXML(xdrs, PROTOCOL_PLASMACURRENT, direction, token, (void *)plasmacurrent)) != 0) break;
                        }
                        for(i=0; i<efit->ndiamagnetic; i++) {
                            diamagnetic = efit->diamagnetic+i;
                            if((err = protocolXML(xdrs, PROTOCOL_DIAMAGNETIC, direction, token, (void *)diamagnetic)) != 0) break;
                        }
                        for(i=0; i<efit->ntoroidalfield; i++) {
                            toroidalfield = efit->toroidalfield+i;
                            if((err = protocolXML(xdrs, PROTOCOL_TOROIDALFIELD, direction, token, (void *)toroidalfield)) != 0) break;
                        }
                        for(i=0; i<efit->nlimiter; i++) {
                            limiter = efit->limiter+i;
                            if((err = protocolXML(xdrs, PROTOCOL_LIMITER, direction, token, (void *)limiter)) != 0) break;
                        }
        
                        break;
        
                    case XDR_FREE_HEAP :
                        break;
        
                    default:
                        err = PROTOCOL_ERROR_4;
                        break;
                    }
        
                    break;
                }
        
        //----------------------------------------------------------------------------
        // PF Coils
        
                if(protocol_id == PROTOCOL_PFCOILS) {
        
                    pfcoils = (PFCOILS *)str;
        
                    switch (direction) {
        
                    case XDR_RECEIVE :
        
                        if (!(rc = xdrrec_skiprecord(xdrs))) {
                            err = PROTOCOL_ERROR_5;
                            break;
                        }
                        if(!(rc = xdr_pfcoils1(xdrs, pfcoils))) {
                            err = PROTOCOL_ERROR_1011;
                            break;
                        }
        
                        if(pfcoils->nco == 0) break;				// No Data to Receive!
        
                        if((err = alloc_pfcoils(pfcoils)) != 0) break;		// Allocate Heap Memory
        
                        if(!(rc = xdr_pfcoils2(xdrs, pfcoils))) {
                            err = PROTOCOL_ERROR_1012;
                            break;
                        }
        
                        break;
        
                    case XDR_SEND:
        
                        if(!(rc = xdr_pfcoils1(xdrs, pfcoils))) {
                            err = PROTOCOL_ERROR_1011;
                            break;
                        }
        
                        if(pfcoils->nco == 0) break;				// No Data to Send!
        
                        if(!(rc = xdr_pfcoils2(xdrs, pfcoils))) {
                            err = PROTOCOL_ERROR_1012;
                            break;
                        }
        
                        if(!(rc = xdrrec_endofrecord(xdrs, 1))) err = PROTOCOL_ERROR_7;
        
                        break;
        
                    case XDR_FREE_HEAP :
                        break;
        
                    default:
                        err = PROTOCOL_ERROR_4;
                        break;
                    }
        
                    break;
                }
        
        //----------------------------------------------------------------------------
        // PF Passive
        
                if(protocol_id == PROTOCOL_PFPASSIVE) {
        
                    pfpassive = (PFPASSIVE *)str;
        
                    switch (direction) {
        
                    case XDR_RECEIVE :
        
                        if (!(rc = xdrrec_skiprecord(xdrs))) {
                            err = PROTOCOL_ERROR_5;
                            break;
                        }
                        if(!(rc = xdr_pfpassive1(xdrs, pfpassive))) {
                            err = PROTOCOL_ERROR_1021;
                            break;
                        }
        
                        if(pfpassive->nco == 0) break;				// No Data to Receive!
        
                        if((err = alloc_pfpassive(pfpassive)) != 0) break;	// Allocate Heap Memory
        
                        if(!(rc = xdr_pfpassive2(xdrs, pfpassive))) {
                            err = PROTOCOL_ERROR_1022;
                            break;
                        }
        
                        break;
        
                    case XDR_SEND:
        
                        if(!(rc = xdr_pfpassive1(xdrs, pfpassive))) {
                            err = PROTOCOL_ERROR_1021;
                            break;
                        }
        
                        if(pfpassive->nco == 0) break;				// No Data to Send!
        
                        if(!(rc = xdr_pfpassive2(xdrs, pfpassive))) {
                            err = PROTOCOL_ERROR_1022;
                            break;
                        }
        
                        if(!(rc = xdrrec_endofrecord(xdrs, 1))) err = PROTOCOL_ERROR_7;
        
                        break;
        
                    case XDR_FREE_HEAP :
                        break;
        
                    default:
                        err = PROTOCOL_ERROR_4;
                        break;
                    }
        
                    break;
                }
        
        //----------------------------------------------------------------------------
        // PF Supplies
        
                if(protocol_id == PROTOCOL_PFSUPPLIES) {
        
                    pfsupplies = (PFSUPPLIES *)str;
        
                    switch (direction) {
        
                    case XDR_RECEIVE :
        
                        if (!(rc = xdrrec_skiprecord(xdrs))) {
                            err = PROTOCOL_ERROR_5;
                            break;
                        }
                        if(!(rc = xdr_pfsupplies(xdrs, pfsupplies))) {
                            err = PROTOCOL_ERROR_1031;
                            break;
                        }
        
                        break;
        
                    case XDR_SEND:
        
                        if(!(rc = xdr_pfsupplies(xdrs, pfsupplies))) {
                            err = PROTOCOL_ERROR_1031;
                            break;
                        }
        
                        if(!(rc = xdrrec_endofrecord(xdrs, 1))) err = PROTOCOL_ERROR_7;
        
                        break;
        
                    case XDR_FREE_HEAP :
                        break;
        
                    default:
                        err = PROTOCOL_ERROR_4;
                        break;
                    }
        
                    break;
                }
        
        //----------------------------------------------------------------------------
        // Flux Loops
        
                if(protocol_id == PROTOCOL_FLUXLOOP) {
        
                    fluxloop = (FLUXLOOP *)str;
        
                    switch (direction) {
        
                    case XDR_RECEIVE :
        
                        if (!(rc = xdrrec_skiprecord(xdrs))) {
                            err = PROTOCOL_ERROR_5;
                            break;
                        }
                        if(!(rc = xdr_fluxloop1(xdrs, fluxloop))) {
                            err = PROTOCOL_ERROR_1041;
                            break;
                        }
        
                        if(fluxloop->nco == 0) break;				// No Data to Receive!
        
                        if((err = alloc_fluxloop(fluxloop)) != 0) break;	// Allocate Heap Memory
        
                        if(!(rc = xdr_fluxloop2(xdrs, fluxloop))) {
                            err = PROTOCOL_ERROR_1042;
                            break;
                        }
        
                        break;
        
                    case XDR_SEND:
        
                        if(!(rc = xdr_fluxloop1(xdrs, fluxloop))) {
                            err = PROTOCOL_ERROR_1041;
                            break;
                        }
        
                        if(fluxloop->nco == 0) break;
        
                        if(!(rc = xdr_fluxloop2(xdrs, fluxloop))) {
                            err = PROTOCOL_ERROR_1042;
                            break;
                        }
        
                        if(!(rc = xdrrec_endofrecord(xdrs, 1))) err = PROTOCOL_ERROR_7;
        
                        break;
        
                    case XDR_FREE_HEAP :
                        break;
        
                    default:
                        err = PROTOCOL_ERROR_4;
                        break;
                    }
        
                    break;
                }
        
        //----------------------------------------------------------------------------
        // Magnetic Probe
        
                if(protocol_id == PROTOCOL_MAGPROBE) {
        
                    magprobe = (MAGPROBE *)str;
        
                    switch (direction) {
        
                    case XDR_RECEIVE :
        
                        if (!(rc = xdrrec_skiprecord(xdrs))) {
                            err = PROTOCOL_ERROR_5;
                            break;
                        }
                        if(!(rc = xdr_magprobe(xdrs, magprobe))) {
                            err = PROTOCOL_ERROR_1051;
                            break;
                        }
        
                        break;
        
                    case XDR_SEND:
        
                        if(!(rc = xdr_magprobe(xdrs, magprobe))) {
                            err = PROTOCOL_ERROR_1051;
                            break;
                        }
        
                        if(!(rc = xdrrec_endofrecord(xdrs, 1))) err = PROTOCOL_ERROR_7;
        
                        break;
        
                    case XDR_FREE_HEAP :
                        break;
        
                    default:
                        err = PROTOCOL_ERROR_4;
                        break;
                    }
        
                    break;
                }
        
        //----------------------------------------------------------------------------
        // PF Circuit
        
                if(protocol_id == PROTOCOL_PFCIRCUIT) {
        
                    pfcircuit = (PFCIRCUIT *)str;
        
                    switch (direction) {
        
                    case XDR_RECEIVE :
        
                        if (!(rc = xdrrec_skiprecord(xdrs))) {
                            err = PROTOCOL_ERROR_5;
                            break;
                        }
                        if(!(rc = xdr_pfcircuit1(xdrs, pfcircuit))) {
                            err = PROTOCOL_ERROR_1061;
                            break;
                        }
        
                        if(pfcircuit->nco == 0) break;				// No Data to Receive!
        
                        if((err = alloc_pfcircuit(pfcircuit)) != 0) break;	// Allocate Heap Memory
        
                        if(!(rc = xdr_pfcircuit2(xdrs, pfcircuit))) {
                            err = PROTOCOL_ERROR_1062;
                            break;
                        }
        
                        break;
        
                    case XDR_SEND:
        
                        if(!(rc = xdr_pfcircuit1(xdrs, pfcircuit))) {
                            err = PROTOCOL_ERROR_1061;
                            break;
                        }
        
                        if(pfcircuit->nco == 0) break;				// No Data to Send!
        
                        if(!(rc = xdr_pfcircuit2(xdrs, pfcircuit))) {
                            err = PROTOCOL_ERROR_1062;
                            break;
                        }
        
                        if(!(rc = xdrrec_endofrecord(xdrs, 1))) err = PROTOCOL_ERROR_7;
        
                        break;
        
                    case XDR_FREE_HEAP :
                        break;
        
                    default:
                        err = PROTOCOL_ERROR_4;
                        break;
                    }
        
                    break;
                }
        
        //----------------------------------------------------------------------------
        // Plasma Current
        
                if(protocol_id == PROTOCOL_PLASMACURRENT) {
        
                    plasmacurrent = (PLASMACURRENT *)str;
        
                    switch (direction) {
        
                    case XDR_RECEIVE :
        
                        if (!(rc = xdrrec_skiprecord(xdrs))) {
                            err = PROTOCOL_ERROR_5;
                            break;
                        }
                        if(!(rc = xdr_plasmacurrent(xdrs, plasmacurrent))) {
                            err = PROTOCOL_ERROR_1071;
                            break;
                        }
        
                        break;
        
                    case XDR_SEND:
        
                        if(!(rc = xdr_plasmacurrent(xdrs, plasmacurrent))) {
                            err = PROTOCOL_ERROR_1071;
                            break;
                        }
        
                        if(!(rc = xdrrec_endofrecord(xdrs, 1))) err = PROTOCOL_ERROR_7;
        
                        break;
        
                    case XDR_FREE_HEAP :
                        break;
        
                    default:
                        err = PROTOCOL_ERROR_4;
                        break;
                    }
        
                    break;
                }
        
        //----------------------------------------------------------------------------
        // Diamagnetic Flux
        
                if(protocol_id == PROTOCOL_DIAMAGNETIC) {
        
                    diamagnetic = (DIAMAGNETIC *)str;
        
                    switch (direction) {
        
                    case XDR_RECEIVE :
        
                        if (!(rc = xdrrec_skiprecord(xdrs))) {
                            err = PROTOCOL_ERROR_5;
                            break;
                        }
                        if(!(rc = xdr_diamagnetic(xdrs, diamagnetic))) {
                            err = PROTOCOL_ERROR_1071;
                            break;
                        }
        
                        break;
        
                    case XDR_SEND:
        
                        if(!(rc = xdr_diamagnetic(xdrs, diamagnetic))) {
                            err = PROTOCOL_ERROR_1071;
                            break;
                        }
        
                        if(!(rc = xdrrec_endofrecord(xdrs, 1))) err = PROTOCOL_ERROR_7;
        
                        break;
        
                    case XDR_FREE_HEAP :
                        break;
        
                    default:
                        err = PROTOCOL_ERROR_4;
                        break;
                    }
        
                    break;
                }
        
        //----------------------------------------------------------------------------
        // Toroidal Magnetic Field
        
                if(protocol_id == PROTOCOL_TOROIDALFIELD) {
        
                    toroidalfield = (TOROIDALFIELD *)str;
        
                    switch (direction) {
        
                    case XDR_RECEIVE :
        
                        if (!(rc = xdrrec_skiprecord(xdrs))) {
                            err = PROTOCOL_ERROR_5;
                            break;
                        }
                        if(!(rc = xdr_toroidalfield(xdrs, toroidalfield))) {
                            err = PROTOCOL_ERROR_1081;
                            break;
                        }
        
                        break;
        
                    case XDR_SEND:
        
                        if(!(rc = xdr_toroidalfield(xdrs, toroidalfield))) {
                            err = PROTOCOL_ERROR_1081;
                            break;
                        }
        
                        if(!(rc = xdrrec_endofrecord(xdrs, 1))) err = PROTOCOL_ERROR_7;
        
                        break;
        
                    case XDR_FREE_HEAP :
                        break;
        
                    default:
                        err = PROTOCOL_ERROR_4;
                        break;
                    }
        
                    break;
                }
        
        //----------------------------------------------------------------------------
        // Limiters
        
                if(protocol_id == PROTOCOL_LIMITER) {
        
                    limiter = (LIMITER *)str;
        
                    switch (direction) {
        
                    case XDR_RECEIVE :
        
                        if (!(rc = xdrrec_skiprecord(xdrs))) {
                            err = PROTOCOL_ERROR_5;
                            break;
                        }
                        if(!(rc = xdr_limiter1(xdrs, limiter))) {
                            err = PROTOCOL_ERROR_1091;
                            break;
                        }
        
                        if(limiter->nco == 0) break;				// No Data to Receive!
        
                        if((err = alloc_limiter(limiter)) != 0) break;		// Allocate Heap Memory
        
                        if(!(rc = xdr_limiter2(xdrs, limiter))) {
                            err = PROTOCOL_ERROR_1092;
                            break;
                        }
        
                        break;
        
                    case XDR_SEND:
        
                        if(!(rc = xdr_limiter1(xdrs, limiter))) {
                            err = PROTOCOL_ERROR_1091;
                            break;
                        }
        
                        if(limiter->nco == 0) break;				// No Data to Send!
        
                        if(!(rc = xdr_limiter2(xdrs, limiter))) {
                            err = PROTOCOL_ERROR_1092;
                            break;
                        }
        
                        if(!(rc = xdrrec_endofrecord(xdrs, 1))) err = PROTOCOL_ERROR_7;
        
                        break;
        
                    case XDR_FREE_HEAP :
                        break;
        
                    default:
                        err = PROTOCOL_ERROR_4;
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

#endif
