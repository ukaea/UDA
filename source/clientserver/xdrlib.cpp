//
// Library of XDR (de)serialiser routines for passing structures between the
// client and the data server.
//
// xdr_longlong is not available - use xdr_int64_t and xdr_uint64_t
//

#include "xdrlib.h"

#include <cstdlib>
#include <memory.h>
#include <uda/structured.h>

#include "clientserver/protocol.h"
#include "clientserver/protocolXML2.h"
#include "logging/logging.h"

#include "errorLog.h"
#include "printStructs.h"
#include "stringUtils.h"
#include "structures/struct.h"

using namespace uda::client_server;
using namespace uda::logging;

//-----------------------------------------------------------------------
// Test version's type passing capability

int uda::client_server::protocolVersionTypeTest(int protocol_version, int type)
{
    // If this client/server version cannot pass/receive a specific type, then return TRUE

    UDA_LOG(UDA_LOG_DEBUG, "protocolVersionTypeTest Version: %d, Type: %d\n", protocol_version, type);

    if (protocol_version < 3) {
        switch (type) {
            case UDA_TYPE_UNSIGNED_CHAR:
            case UDA_TYPE_UNSIGNED_SHORT:
            case UDA_TYPE_UNSIGNED_LONG:
            case UDA_TYPE_UNSIGNED_LONG64:
            case UDA_TYPE_COMPLEX:
            case UDA_TYPE_DCOMPLEX:
                return 1;
            default:
                return 0;
        }
    } else {
        if (protocol_version < 4) {
            if (type == UDA_TYPE_COMPOUND) {
                return 1;
            }
        }
        if (protocol_version < 6) {
            if (type == UDA_TYPE_STRING) {
                return 1;
            }
        }
    }
    return 0; // Return Test False: This type is OK
}

//-----------------------------------------------------------------------
// Strings

int uda::client_server::wrap_string(XDR* xdrs, char* sp)
{
    return xdr_string(xdrs, &sp, MAX_STRING_LENGTH);
}

int uda::client_server::WrapXDRString(XDR* xdrs, const char* sp, int maxlen)
{
    return xdr_string(xdrs, (char**)&sp, (unsigned int)maxlen);
}

//-----------------------------------------------------------------------
// Meta Data

bool_t uda::client_server::xdr_meta(XDR* xdrs, DATA_BLOCK* str)
{
    int rc = WrapXDRString(xdrs, (char*)str->opaque_block, str->opaque_count + 1);
    return rc;
}

//-----------------------------------------------------------------------
// Security block

bool_t uda::client_server::xdr_securityBlock1(XDR* xdrs, SECURITY_BLOCK* str)
{
    int rc = xdr_u_short(xdrs, &str->structVersion) && xdr_u_short(xdrs, &str->encryptionMethod) &&
             xdr_u_short(xdrs, &str->authenticationStep) && xdr_u_short(xdrs, &str->client_ciphertextLength) &&
             xdr_u_short(xdrs, &str->client2_ciphertextLength) && xdr_u_short(xdrs, &str->server_ciphertextLength) &&
             xdr_u_short(xdrs, &str->client_X509Length) && xdr_u_short(xdrs, &str->client2_X509Length);
    return rc;
}

bool_t uda::client_server::xdr_securityBlock2(XDR* xdrs, SECURITY_BLOCK* str)
{
    int rc = 1;

    if (str->client_ciphertextLength > 0) {
        rc = rc && xdr_vector(xdrs, (char*)str->client_ciphertext, (int)str->client_ciphertextLength,
                              sizeof(unsigned char), (xdrproc_t)xdr_u_char);
    }

    if (str->client2_ciphertextLength > 0) {
        rc = rc && xdr_vector(xdrs, (char*)str->client2_ciphertext, (int)str->client2_ciphertextLength,
                              sizeof(unsigned char), (xdrproc_t)xdr_u_char);
    }

    if (str->server_ciphertextLength > 0) {
        rc = rc && xdr_vector(xdrs, (char*)str->server_ciphertext, (int)str->server_ciphertextLength,
                              sizeof(unsigned char), (xdrproc_t)xdr_u_char);
    }

    if (str->client_X509Length > 0) {
        rc = rc && xdr_vector(xdrs, (char*)str->client_X509, (int)str->client_X509Length, sizeof(unsigned char),
                              (xdrproc_t)xdr_u_char);
    }

    if (str->client2_X509Length > 0) {
        rc = rc && xdr_vector(xdrs, (char*)str->client2_X509, (int)str->client2_X509Length, sizeof(unsigned char),
                              (xdrproc_t)xdr_u_char);
    }

    return rc;
}

//-----------------------------------------------------------------------
// Client State Block

// On first connection, client sends client_block to server
// Server knows the appropriate protocolVersion to adapt the conversation
// Client does not know the server version so send all
// Server ignores what it doesn't need
// Server must flush the tcp buffer before the next

// Notes:
//        debug_level     ** Not Used
//        get_datadble    ** Client Side Only
//        get_timedble
//        get_dimdble
//        get_scalar
//        get_bytes
//        get_bad        ** Client Side Only
//        get_meta
//        get_asis
//        get_uncal
//        get_notoff

bool_t uda::client_server::xdr_client(XDR* xdrs, CLIENT_BLOCK* str, int protocolVersion)
{
    int rc = xdr_int(xdrs, &str->version) && xdr_int(xdrs, &str->pid) && xdr_int(xdrs, &str->timeout) &&
             WrapXDRString(xdrs, (char*)str->uid, STRING_LENGTH);

    if (str->version < protocolVersion) {
        protocolVersion = str->version;
    }

    // clientFlags and altRank do not exist in the CLIENT_BLOCK structure prior to version 6

    if (protocolVersion >= 6) {
        rc = rc && xdr_u_int(xdrs, &str->clientFlags) && xdr_int(xdrs, &str->altRank);
    } else {
        int temp = 0;                         // retain Legacy!
        rc = rc && xdr_int(xdrs, &temp)       // Changed type        (was verbose & not used)
             && xdr_int(xdrs, &str->altRank); //                  (was debug)
        str->clientFlags = (unsigned int)temp;
    }

    rc = rc && xdr_int(xdrs, &str->get_nodimdata) && xdr_int(xdrs, &str->get_datadble) &&
         xdr_int(xdrs, &str->get_timedble) && xdr_int(xdrs, &str->get_dimdble) && xdr_int(xdrs, &str->get_scalar) &&
         xdr_int(xdrs, &str->get_bytes) && xdr_int(xdrs, &str->get_bad) && xdr_int(xdrs, &str->get_meta) &&
         xdr_int(xdrs, &str->get_asis) && xdr_int(xdrs, &str->get_uncal) && xdr_int(xdrs, &str->get_notoff);

    // output (ENCODE) means written by the client
    // input (DECODE) means read by the server

    // xdrs->x_op == XDR_DECODE && protocolVersion == 2 Means Client receiving data from a Version 2 Server
    // xdrs->x_op == XDR_ENCODE && protocolVersion == 3 Means Server sending data to a Version 3 Client

    // privateFlags does not exist in the CLIENT_BLOCK structure prior to version 5

    if (protocolVersion >= 5) {
        rc = rc && xdr_u_int(xdrs, &str->privateFlags);
    }

    if (xdrs->x_op == XDR_DECODE && protocolVersion < 6) {
        str->clientFlags = 0; // The original properties have no effect on the server whatever the version
        str->altRank = 0;
    }

    if (protocolVersion >= 7) {
        rc = rc && WrapXDRString(xdrs, (char*)str->OSName, STRING_LENGTH) &&
             WrapXDRString(xdrs, (char*)str->DOI, STRING_LENGTH);

#ifdef SECURITYENABLED
        rc = rc && WrapXDRString(xdrs, (char*)str->uid2, STRING_LENGTH);
#endif
    }

    UDA_LOG(UDA_LOG_DEBUG, "protocolVersion %d\n", protocolVersion);
    printClientBlock(*str);

    return rc;
}

//-----------------------------------------------------------------------
// Server State Block

bool_t uda::client_server::xdr_server1(XDR* xdrs, SERVER_BLOCK* str, int protocolVersion)
{

    int rc = 0;
    static int serverVersion = 0;

    rc = xdr_int(xdrs, &str->version);

    if (xdrs->x_op == XDR_DECODE && rc) { // Test for a server crash!
        if (serverVersion == 0) {
            serverVersion = str->version;           // Assume OK on first exchange
        } else if (serverVersion != str->version) { // Usually different if the server has crashed
            rc = 0;                                 // Force an error
            str->version = serverVersion;           // Replace the erroneous version number
            UDA_LOG(UDA_LOG_DEBUG, "Server #1 rc = %d\n", rc);
            return rc;
        }
    }

    UDA_LOG(UDA_LOG_DEBUG, "Server #1 rc[1] = %d, version = %d\n", rc, str->version);

    rc = rc && xdr_int(xdrs, &str->error);
    UDA_LOG(UDA_LOG_DEBUG, "Server #1 rc[2] = %d, error = %d\n", rc, str->error);
    rc = rc && xdr_u_int(xdrs, &str->idamerrorstack.nerrors);
    UDA_LOG(UDA_LOG_DEBUG, "Server #1 rc[3] = %d, error = %d\n", rc, str->idamerrorstack.nerrors);

    rc = rc && WrapXDRString(xdrs, (char*)str->msg, STRING_LENGTH);

    UDA_LOG(UDA_LOG_DEBUG, "Server #1 rc[4] = %d, strlen = %zd\n", rc, strlen(str->msg));
    UDA_LOG(UDA_LOG_DEBUG, "str->msg = %p\n", str->msg);
    UDA_LOG(UDA_LOG_DEBUG, "str->msg[0] = %d\n", str->msg[0]);
    UDA_LOG(UDA_LOG_DEBUG, "maxsize = %d\n", STRING_LENGTH);
    UDA_LOG(UDA_LOG_DEBUG, "Server #1 protocolVersion %d [rc = %d]\n", protocolVersion, rc);

    if ((xdrs->x_op == XDR_DECODE && protocolVersion >= 7) || (xdrs->x_op == XDR_ENCODE && protocolVersion >= 7)) {
        rc = rc && WrapXDRString(xdrs, (char*)str->OSName, STRING_LENGTH) &&
             WrapXDRString(xdrs, (char*)str->DOI, STRING_LENGTH);
#ifdef SECURITYENABLED
        // rc = rc && xdr_securityBlock(xdrs, &str->securityBlock);
#endif
    }

    UDA_LOG(UDA_LOG_DEBUG, "Server #1 rc = %d\n", rc);

    return rc;
}

bool_t uda::client_server::xdr_server2(XDR* xdrs, SERVER_BLOCK* str)
{
    int rc = 1;
    for (unsigned int i = 0; i < str->idamerrorstack.nerrors; i++) {
        rc = rc && xdr_int(xdrs, &str->idamerrorstack.idamerror[i].type) &&
             xdr_int(xdrs, &str->idamerrorstack.idamerror[i].code) &&
             WrapXDRString(xdrs, (char*)str->idamerrorstack.idamerror[i].location, STRING_LENGTH) &&
             WrapXDRString(xdrs, (char*)str->idamerrorstack.idamerror[i].msg, STRING_LENGTH);

        UDA_LOG(UDA_LOG_DEBUG, "xdr_server2 [%d] %s\n", i, str->idamerrorstack.idamerror[i].msg);
    }

    UDA_LOG(UDA_LOG_DEBUG, "Server #2 rc = %d\n", rc);

    return rc;
}

bool_t uda::client_server::xdr_server(XDR* xdrs, SERVER_BLOCK* str)
{
    return xdr_int(xdrs, &str->version) && xdr_int(xdrs, &str->error) &&
           WrapXDRString(xdrs, (char*)str->msg, STRING_LENGTH);
}

//-----------------------------------------------------------------------
// Client Data Request Block

bool_t uda::client_server::xdr_request_data(XDR* xdrs, REQUEST_DATA* str, int protocolVersion)
{
    int request = static_cast<int>(str->request);
    int rc = xdr_int(xdrs, &request);
    str->request = static_cast<REQUEST>(request);
    rc = rc && xdr_int(xdrs, &str->exp_number);
    rc = rc && xdr_int(xdrs, &str->pass);
    rc = rc && WrapXDRString(xdrs, (char*)str->tpass, STRING_LENGTH);
    rc = rc && WrapXDRString(xdrs, (char*)str->archive, STRING_LENGTH);
    rc = rc && WrapXDRString(xdrs, (char*)str->device_name, STRING_LENGTH);
    rc = rc && WrapXDRString(xdrs, (char*)str->server, STRING_LENGTH);
    rc = rc && WrapXDRString(xdrs, (char*)str->path, STRING_LENGTH);
    rc = rc && WrapXDRString(xdrs, (char*)str->file, STRING_LENGTH);
    rc = rc && WrapXDRString(xdrs, (char*)str->format, STRING_LENGTH);

    rc = rc && WrapXDRString(xdrs, (char*)str->signal, MAXMETA);

    if ((xdrs->x_op == XDR_DECODE && protocolVersion >= 6) || (xdrs->x_op == XDR_ENCODE && protocolVersion >= 6)) {
        rc = rc && WrapXDRString(xdrs, (char*)str->source, STRING_LENGTH) &&
             WrapXDRString(xdrs, (char*)str->api_delim, MAXNAME);
    }

    if (protocolVersion >= 7) {
        rc = rc && xdr_int(xdrs, &str->put);
    }

    return rc;
}

bool_t uda::client_server::xdr_request(XDR* xdrs, REQUEST_BLOCK* str, int protocolVersion)
{
    int rc = 1;

    if (protocolVersion <= 7) {
        str->num_requests = 1;
    } else {
        rc = rc && xdr_int(xdrs, &str->num_requests);
    }

    UDA_LOG(UDA_LOG_DEBUG, "number of requests: %d\n", str->num_requests);
    return rc;
}

bool_t uda::client_server::xdr_data_block_list(XDR* xdrs, DATA_BLOCK_LIST* str, int protocolVersion)
{
    int rc = 1;

    if (protocolVersion <= 7) {
        str->count = 1;
    } else {
        rc = rc && xdr_int(xdrs, &str->count);
    }

    UDA_LOG(UDA_LOG_DEBUG, "number of data blocks: %d\n", str->count);
    return rc;
}

//-----------------------------------------------------------------------
// Put Data

bool_t uda::client_server::xdr_putdatablocklist_block(XDR* xdrs, PutDataBlockList* str)
{
    int rc = xdr_u_int(xdrs, &str->blockCount);
    return rc;
}

bool_t uda::client_server::xdr_putdata_block1(XDR* xdrs, PutDataBlock* str)
{
    int rc = 1;
    rc = rc && xdr_u_int(xdrs, &str->rank) && xdr_u_int(xdrs, &str->count) && xdr_int(xdrs, &str->data_type) &&
         xdr_int(xdrs, &str->opaque_type) && xdr_int(xdrs, &str->opaque_count) &&
         xdr_u_int(xdrs, &str->blockNameLength);
    return rc;
}

bool_t uda::client_server::xdr_putdata_block2(XDR* xdrs, PutDataBlock* str)
{
    int rc = 1;
    if (str->rank > 0) {
        rc = rc && xdr_vector(xdrs, (char*)str->shape, (int)str->rank, sizeof(int), (xdrproc_t)xdr_int);
    }

    if (str->blockNameLength > 0) {
        rc = rc && WrapXDRString(xdrs, (char*)str->blockName, str->blockNameLength + 1);
    }

    switch (str->data_type) {
        case UDA_TYPE_FLOAT:
            return rc && xdr_vector(xdrs, (char*)str->data, (int)str->count, sizeof(float), (xdrproc_t)xdr_float);
        case UDA_TYPE_DOUBLE:
            return rc && xdr_vector(xdrs, (char*)str->data, (int)str->count, sizeof(double), (xdrproc_t)xdr_double);
        case UDA_TYPE_CHAR:
            return rc && xdr_vector(xdrs, (char*)str->data, (int)str->count, sizeof(char), (xdrproc_t)xdr_char);
        case UDA_TYPE_SHORT:
            return rc && xdr_vector(xdrs, (char*)str->data, (int)str->count, sizeof(short), (xdrproc_t)xdr_short);
        case UDA_TYPE_INT:
            return rc && xdr_vector(xdrs, (char*)str->data, (int)str->count, sizeof(int), (xdrproc_t)xdr_int);
        case UDA_TYPE_LONG:
            return rc && xdr_vector(xdrs, (char*)str->data, (int)str->count, sizeof(long), (xdrproc_t)xdr_long);
        case UDA_TYPE_LONG64:
            return rc &&
                   xdr_vector(xdrs, (char*)str->data, (int)str->count, sizeof(long long int), (xdrproc_t)xdr_int64_t);
        case UDA_TYPE_UNSIGNED_CHAR:
            return rc &&
                   xdr_vector(xdrs, (char*)str->data, (int)str->count, sizeof(unsigned char), (xdrproc_t)xdr_u_char);
        case UDA_TYPE_UNSIGNED_SHORT:
            return rc &&
                   xdr_vector(xdrs, (char*)str->data, (int)str->count, sizeof(unsigned short), (xdrproc_t)xdr_u_short);
        case UDA_TYPE_UNSIGNED_INT:
            return rc &&
                   xdr_vector(xdrs, (char*)str->data, (int)str->count, sizeof(unsigned int), (xdrproc_t)xdr_u_int);
        case UDA_TYPE_UNSIGNED_LONG:
            return rc &&
                   xdr_vector(xdrs, (char*)str->data, (int)str->count, sizeof(unsigned long), (xdrproc_t)xdr_u_long);
        case UDA_TYPE_UNSIGNED_LONG64:
            return rc && xdr_vector(xdrs, (char*)str->data, (int)str->count, sizeof(unsigned long long int),
                                    (xdrproc_t)xdr_uint64_t);
            // Strings are passed as a regular array of CHARs

        case UDA_TYPE_STRING:
            return rc && xdr_vector(xdrs, (char*)str->data, (int)str->count, sizeof(char), (xdrproc_t)xdr_char);

            // Complex structure is a simple two float combination: => twice the number of element transmitted

        case UDA_TYPE_DCOMPLEX:
            return rc && xdr_vector(xdrs, (char*)str->data, 2 * str->count, sizeof(double), (xdrproc_t)xdr_double);
        case UDA_TYPE_COMPLEX:
            return rc && xdr_vector(xdrs, (char*)str->data, 2 * str->count, sizeof(float), (xdrproc_t)xdr_float);

            // General Data structures are passed using a specialised set of xdr components

        case UDA_TYPE_COMPOUND:
            return rc; // Nothing to send so retain good return code

        default:
            return 0;
    }
}

//-----------------------------------------------------------------------
// Data Objects

bool_t uda::client_server::xdr_data_object1(XDR* xdrs, DATA_OBJECT* str)
{
    int rc =
        xdr_u_short(xdrs, &str->objectType) && xdr_u_int(xdrs, &str->objectSize) && xdr_u_short(xdrs, &str->hashLength);
    return rc;
}

bool_t uda::client_server::xdr_data_object2(XDR* xdrs, DATA_OBJECT* str)
{
    int rc = xdr_opaque(xdrs, str->object, (unsigned int)str->objectSize) &&
             xdr_vector(xdrs, str->md, str->hashLength, sizeof(char), (xdrproc_t)xdr_char);
    return rc;
}

bool_t xdr_serialise_object(XDR* xdrs, LOGMALLOCLIST* logmalloclist, USERDEFINEDTYPELIST* userdefinedtypelist,
                            DATA_BLOCK* str, int protocolVersion, bool xdr_stdio_flag, LOGSTRUCTLIST* log_struct_list,
                            int malloc_source)
{
    int err = 0, rc = 1;
    int packageType = 0;
    void* data = nullptr;

    if (xdrs->x_op == XDR_ENCODE) { // Send Data

        SARRAY sarray; // Structure array carrier structure
        SARRAY* psarray = &sarray;
        int shape = str->data_n;                        // rank 1 array of dimension lengths
        auto udt = (USERDEFINEDTYPE*)str->opaque_block; // The data's structure definition
        auto u = udaFindUserDefinedType(userdefinedtypelist, "SARRAY",
                                        0); // Locate the carrier structure definition

        if (udt == nullptr || u == nullptr) {
            err = 999;
            add_error(UDA_CODE_ERROR_TYPE, "protocolDataObject", err, "nullptr User defined data Structure Definition");
            return 0;
        }

        initSArray(&sarray);
        sarray.count = str->data_n;     // Number of this structure
        sarray.rank = 1;                // Array Data Rank?
        sarray.shape = &shape;          // Only if rank > 1?
        sarray.data = (void*)str->data; // Pointer to the data to be passed
        strcpy(sarray.type, udt->name); // The name of the type
        data = (void*)&psarray;         // Pointer to the SARRAY array pointer
        udaAddNonMalloc(logmalloclist, (void*)&shape, 1, sizeof(int), "int");

        packageType = UDA_PACKAGE_XDROBJECT; // The package is an XDR serialised object

        rc = xdr_int(xdrs, &packageType); // Send data package type

        // Send the data

        rc = rc && xdr_userdefinedtypelist(xdrs, userdefinedtypelist,
                                           xdr_stdio_flag); // send the full set of known named structures
        rc = rc && xdrUserDefinedTypeData(xdrs, logmalloclist, userdefinedtypelist, u, (void**)data, protocolVersion,
                                          xdr_stdio_flag, log_struct_list, malloc_source); // send the Data

        if (!rc) {
            err = 999;
            add_error(UDA_CODE_ERROR_TYPE, "protocolDataObject", err, "Bad Return Code passing data structures");
            return 0;
        }

    } else { // Receive Data

        rc = rc && xdr_int(xdrs, &packageType); // Receive data package type

        if (packageType != UDA_PACKAGE_XDROBJECT) {
            err = 999;
            add_error(UDA_SYSTEM_ERROR_TYPE, "protocolDataObject", err, "Incorrect package Type option");
            return 0;
        }

        // Heap allocations log

        logmalloclist = (LOGMALLOCLIST*)malloc(sizeof(LOGMALLOCLIST));
        initLogMallocList(logmalloclist);

        // Data structure definitions

        userdefinedtypelist = (USERDEFINEDTYPELIST*)malloc(sizeof(USERDEFINEDTYPELIST));
        initUserDefinedTypeList(userdefinedtypelist);

        // receive the full set of known named structures
        rc = rc && xdr_userdefinedtypelist(xdrs, userdefinedtypelist, xdr_stdio_flag);

        if (!rc) {
            err = 999;
            add_error(UDA_CODE_ERROR_TYPE, "protocolDataObject", err, "Failure receiving Structure Definitions");
            return 0;
        }

        // Receive data

        auto udt_received = (USERDEFINEDTYPE*)malloc(sizeof(USERDEFINEDTYPE));
        initUserDefinedType(udt_received);

        rc = rc && xdrUserDefinedTypeData(xdrs, logmalloclist, userdefinedtypelist, udt_received, &data,
                                          protocolVersion, xdr_stdio_flag, log_struct_list,
                                          malloc_source); // receive the Data

        if (!rc) {
            err = 999;
            add_error(UDA_CODE_ERROR_TYPE, "protocolDataObject", err,
                      "Failure receiving Data and it's Structure Definition");
            return 0;
        }

        // Prepare returned data structure containing the data

        if (STR_EQUALS(udt_received->name, "SARRAY")) { // expecting this carrier structure

            auto general_block = (GENERAL_BLOCK*)malloc(sizeof(GENERAL_BLOCK));

            auto s = (SARRAY*)data;
            if (s->count != str->data_n) { // check for consistency
                err = 999;
                add_error(UDA_CODE_ERROR_TYPE, "protocolDataObject", err, "Inconsistent S Array Counts");
                return 0;
            }
            str->data = (char*)udaGetFullNTree(); // Global Root Node with the Carrier Structure containing data
            str->opaque_block = (void*)general_block;
            general_block->userdefinedtype = udt_received;
            general_block->userdefinedtypelist = userdefinedtypelist;
            general_block->logmalloclist = logmalloclist;
            general_block->lastMallocIndex = 0;

        } else {
            err = 999;
            add_error(UDA_CODE_ERROR_TYPE, "protocolDataObject", err, "Name of Received Data Structure Incorrect");
            return 0;
        }
    }
    return rc;
}

//-----------------------------------------------------------------------
// Data from File Source

bool_t uda::client_server::xdr_data_block1(XDR* xdrs, DATA_BLOCK* str, int protocolVersion)
{
    int rc = xdr_int(xdrs, &str->data_n);
    rc = rc && xdr_u_int(xdrs, &str->rank);
    rc = rc && xdr_int(xdrs, &str->order);
    rc = rc && xdr_int(xdrs, &str->data_type);

    rc = rc && xdr_int(xdrs, &str->error_type);
    rc = rc && xdr_int(xdrs, &str->error_model);
    rc = rc && xdr_int(xdrs, &str->errasymmetry);
    rc = rc && xdr_int(xdrs, &str->error_param_n);
    rc = rc && xdr_int(xdrs, &str->errcode);
    rc = rc && xdr_int(xdrs, &str->source_status);
    rc = rc && xdr_int(xdrs, &str->signal_status);
    rc = rc && WrapXDRString(xdrs, (char*)str->data_units, STRING_LENGTH);
    rc = rc && WrapXDRString(xdrs, (char*)str->data_label, STRING_LENGTH);
    rc = rc && WrapXDRString(xdrs, (char*)str->data_desc, STRING_LENGTH);
    rc = rc && WrapXDRString(xdrs, (char*)str->error_msg, STRING_LENGTH);

    // output (ENCODE) means written by the server
    // input (DECODE) means read by the client

    // xdrs->x_op == XDR_DECODE && protocolVersion == 2 Means Client receiving data from a Version 2 Server
    // xdrs->x_op == XDR_ENCODE && protocolVersion == 3 Means Server sending data to a Version 3 Client

    // opaque_count does not exist in the DATA_BLOCK structure prior to version 3
    // general structure passing not available prior to version 4

    if ((xdrs->x_op == XDR_DECODE && protocolVersion >= 3) || (xdrs->x_op == XDR_ENCODE && protocolVersion >= 3)) {
        rc = rc && xdr_int(xdrs, &str->opaque_type) && xdr_int(xdrs, &str->opaque_count);
    }

    return rc;
}

bool_t uda::client_server::xdr_data_block2(XDR* xdrs, DATA_BLOCK* str)
{
    switch (str->data_type) {
        case UDA_TYPE_FLOAT:
            return xdr_vector(xdrs, str->data, (u_int)str->data_n, sizeof(float), (xdrproc_t)xdr_float);
        case UDA_TYPE_DOUBLE:
            return xdr_vector(xdrs, str->data, (u_int)str->data_n, sizeof(double), (xdrproc_t)xdr_double);
        case UDA_TYPE_CHAR:
            return xdr_vector(xdrs, str->data, (u_int)str->data_n, sizeof(char), (xdrproc_t)xdr_char);
        case UDA_TYPE_SHORT:
            return xdr_vector(xdrs, str->data, (u_int)str->data_n, sizeof(short), (xdrproc_t)xdr_short);
        case UDA_TYPE_INT:
            return xdr_vector(xdrs, str->data, (u_int)str->data_n, sizeof(int), (xdrproc_t)xdr_int);
        case UDA_TYPE_LONG:
            return xdr_vector(xdrs, str->data, (u_int)str->data_n, sizeof(long), (xdrproc_t)xdr_long);
        case UDA_TYPE_LONG64:
            return xdr_vector(xdrs, str->data, (u_int)str->data_n, sizeof(long long int), (xdrproc_t)xdr_int64_t);
        case UDA_TYPE_UNSIGNED_CHAR:
            return xdr_vector(xdrs, str->data, (u_int)str->data_n, sizeof(unsigned char), (xdrproc_t)xdr_u_char);
        case UDA_TYPE_UNSIGNED_SHORT:
            return xdr_vector(xdrs, str->data, (u_int)str->data_n, sizeof(unsigned short), (xdrproc_t)xdr_u_short);
        case UDA_TYPE_UNSIGNED_INT:
            return xdr_vector(xdrs, str->data, (u_int)str->data_n, sizeof(unsigned int), (xdrproc_t)xdr_u_int);
        case UDA_TYPE_UNSIGNED_LONG:
            return xdr_vector(xdrs, str->data, (u_int)str->data_n, sizeof(unsigned long), (xdrproc_t)xdr_u_long);
        case UDA_TYPE_UNSIGNED_LONG64:
            return xdr_vector(xdrs, str->data, (u_int)str->data_n, sizeof(unsigned long long int),
                              (xdrproc_t)xdr_uint64_t);
            // Strings are passed as a regular array of CHARs

        case UDA_TYPE_STRING:
            return xdr_vector(xdrs, str->data, (u_int)str->data_n, sizeof(char), (xdrproc_t)xdr_char);

            // Complex structure is a simple two float combination: => twice the number of element transmitted

        case UDA_TYPE_DCOMPLEX:
            return xdr_vector(xdrs, str->data, 2 * (u_int)str->data_n, sizeof(double), (xdrproc_t)xdr_double);
        case UDA_TYPE_COMPLEX:
            return xdr_vector(xdrs, str->data, 2 * (u_int)str->data_n, sizeof(float), (xdrproc_t)xdr_float);

            // General Data structures are passed using a specialised set of xdr components

        case UDA_TYPE_COMPOUND:
            return 1; // Nothing to send so retain good return code

        case UDA_TYPE_CAPNP:
            return xdr_vector(xdrs, str->data, (u_int)str->data_n, sizeof(char), (xdrproc_t)xdr_char);

        default:
            return 0;
    }
}

bool_t uda::client_server::xdr_data_block3(XDR* xdrs, DATA_BLOCK* str)
{

    if (str->error_param_n > 0) {
        xdr_vector(xdrs, (char*)str->errparams, (u_int)str->error_param_n, sizeof(float), (xdrproc_t)xdr_float);
    }

    // Data Errors

    switch (str->error_type) {
        case UDA_TYPE_FLOAT:
            return xdr_vector(xdrs, str->errhi, (u_int)str->data_n, sizeof(float), (xdrproc_t)xdr_float);
        case UDA_TYPE_DOUBLE:
            return xdr_vector(xdrs, str->errhi, (u_int)str->data_n, sizeof(double), (xdrproc_t)xdr_double);
        case UDA_TYPE_CHAR:
            return xdr_vector(xdrs, str->errhi, (u_int)str->data_n, sizeof(char), (xdrproc_t)xdr_char);
        case UDA_TYPE_SHORT:
            return xdr_vector(xdrs, str->errhi, (u_int)str->data_n, sizeof(short), (xdrproc_t)xdr_short);
        case UDA_TYPE_INT:
            return xdr_vector(xdrs, str->errhi, (u_int)str->data_n, sizeof(int), (xdrproc_t)xdr_int);
        case UDA_TYPE_LONG:
            return xdr_vector(xdrs, str->errhi, (u_int)str->data_n, sizeof(long), (xdrproc_t)xdr_long);
        case UDA_TYPE_LONG64:
            return xdr_vector(xdrs, str->errhi, (u_int)str->data_n, sizeof(long long int), (xdrproc_t)xdr_int64_t);
        case UDA_TYPE_UNSIGNED_CHAR:
            return xdr_vector(xdrs, str->errhi, (u_int)str->data_n, sizeof(unsigned char), (xdrproc_t)xdr_u_char);
        case UDA_TYPE_UNSIGNED_SHORT:
            return xdr_vector(xdrs, str->errhi, (u_int)str->data_n, sizeof(unsigned short), (xdrproc_t)xdr_u_short);
        case UDA_TYPE_UNSIGNED_INT:
            return xdr_vector(xdrs, str->errhi, (u_int)str->data_n, sizeof(unsigned int), (xdrproc_t)xdr_u_int);
        case UDA_TYPE_UNSIGNED_LONG:
            return xdr_vector(xdrs, str->errhi, (u_int)str->data_n, sizeof(unsigned long), (xdrproc_t)xdr_u_long);
        case UDA_TYPE_UNSIGNED_LONG64:
            return xdr_vector(xdrs, str->errhi, (u_int)str->data_n, sizeof(unsigned long long int),
                              (xdrproc_t)xdr_uint64_t);
            // Complex structure is a simple two float combination: => twice the number of element transmitted

        case UDA_TYPE_DCOMPLEX:
            return xdr_vector(xdrs, str->errhi, 2 * (u_int)str->data_n, sizeof(double), (xdrproc_t)xdr_double);
        case UDA_TYPE_COMPLEX:
            return xdr_vector(xdrs, str->errhi, 2 * (u_int)str->data_n, sizeof(float), (xdrproc_t)xdr_float);

        default:
            return 1;
    }
}

bool_t uda::client_server::xdr_data_block4(XDR* xdrs, DATA_BLOCK* str)
{
    if (!str->errasymmetry) {
        return 1; // Nothing New to Pass or Receive (same as errhi!)
    }

    // Asymmetric Data Errors

    switch (str->error_type) {
        case UDA_TYPE_FLOAT:
            return xdr_vector(xdrs, str->errlo, (u_int)str->data_n, sizeof(float), (xdrproc_t)xdr_float);
        case UDA_TYPE_DOUBLE:
            return xdr_vector(xdrs, str->errlo, (u_int)str->data_n, sizeof(double), (xdrproc_t)xdr_double);
        case UDA_TYPE_CHAR:
            return xdr_vector(xdrs, str->errlo, (u_int)str->data_n, sizeof(char), (xdrproc_t)xdr_char);
        case UDA_TYPE_SHORT:
            return xdr_vector(xdrs, str->errlo, (u_int)str->data_n, sizeof(short), (xdrproc_t)xdr_short);
        case UDA_TYPE_INT:
            return xdr_vector(xdrs, str->errlo, (u_int)str->data_n, sizeof(int), (xdrproc_t)xdr_int);
        case UDA_TYPE_LONG:
            return xdr_vector(xdrs, str->errlo, (u_int)str->data_n, sizeof(long), (xdrproc_t)xdr_long);
        case UDA_TYPE_LONG64:
            return xdr_vector(xdrs, str->errlo, (u_int)str->data_n, sizeof(long long int), (xdrproc_t)xdr_int64_t);
        case UDA_TYPE_UNSIGNED_CHAR:
            return xdr_vector(xdrs, str->errlo, (u_int)str->data_n, sizeof(unsigned char), (xdrproc_t)xdr_u_char);
        case UDA_TYPE_UNSIGNED_SHORT:
            return xdr_vector(xdrs, str->errlo, (u_int)str->data_n, sizeof(unsigned short), (xdrproc_t)xdr_u_short);
        case UDA_TYPE_UNSIGNED_INT:
            return xdr_vector(xdrs, str->errlo, (u_int)str->data_n, sizeof(unsigned int), (xdrproc_t)xdr_u_int);
        case UDA_TYPE_UNSIGNED_LONG:
            return xdr_vector(xdrs, str->errlo, (u_int)str->data_n, sizeof(unsigned long), (xdrproc_t)xdr_u_long);
        case UDA_TYPE_UNSIGNED_LONG64:
            return xdr_vector(xdrs, str->errlo, (u_int)str->data_n, sizeof(unsigned long long int),
                              (xdrproc_t)xdr_uint64_t);
            // Complex structure is a simple two float combination: => twice the number of element transmitted

        case UDA_TYPE_DCOMPLEX:
            return xdr_vector(xdrs, str->errlo, 2 * (u_int)str->data_n, sizeof(double), (xdrproc_t)xdr_double);
        case UDA_TYPE_COMPLEX:
            return xdr_vector(xdrs, str->errlo, 2 * (u_int)str->data_n, sizeof(float), (xdrproc_t)xdr_float);

        default:
            return 1;
    }
}

bool_t uda::client_server::xdr_data_dim1(XDR* xdrs, DATA_BLOCK* str)
{
    int rc = 1;
    for (unsigned int i = 0; i < str->rank; i++) {
        rc = rc && xdr_int(xdrs, &str->dims[i].data_type) && xdr_int(xdrs, &str->dims[i].error_type) &&
             xdr_int(xdrs, &str->dims[i].error_model) && xdr_int(xdrs, &str->dims[i].errasymmetry) &&
             xdr_int(xdrs, &str->dims[i].error_param_n) && xdr_int(xdrs, &str->dims[i].dim_n) &&
             xdr_int(xdrs, &str->dims[i].compressed) && xdr_double(xdrs, &str->dims[i].dim0) &&
             xdr_double(xdrs, &str->dims[i].diff) && xdr_int(xdrs, &str->dims[i].method) &&
             xdr_u_int(xdrs, &str->dims[i].udoms) &&
             WrapXDRString(xdrs, (char*)str->dims[i].dim_units, STRING_LENGTH) &&
             WrapXDRString(xdrs, (char*)str->dims[i].dim_label, STRING_LENGTH);
    }

    return rc;
}

bool_t uda::client_server::xdr_data_dim2(XDR* xdrs, DATA_BLOCK* str)
{
    for (unsigned int i = 0; i < str->rank; i++) {
        if (str->dims[i].compressed == 0) {
            switch (str->dims[i].data_type) {

                case UDA_TYPE_FLOAT:
                    if (!xdr_vector(xdrs, str->dims[i].dim, (u_int)str->dims[i].dim_n, sizeof(float),
                                    (xdrproc_t)xdr_float)) {
                        return 0;
                    }
                    break;

                case UDA_TYPE_DOUBLE:
                    if (!xdr_vector(xdrs, str->dims[i].dim, (u_int)str->dims[i].dim_n, sizeof(double),
                                    (xdrproc_t)xdr_double)) {
                        return 0;
                    }
                    break;

                case UDA_TYPE_CHAR:
                    if (!xdr_vector(xdrs, str->dims[i].dim, (u_int)str->dims[i].dim_n, sizeof(char),
                                    (xdrproc_t)xdr_char)) {
                        return 0;
                    }
                    break;

                case UDA_TYPE_SHORT:
                    if (!xdr_vector(xdrs, str->dims[i].dim, (u_int)str->dims[i].dim_n, sizeof(short),
                                    (xdrproc_t)xdr_short)) {
                        return 0;
                    }
                    break;

                case UDA_TYPE_INT:
                    if (!xdr_vector(xdrs, str->dims[i].dim, (u_int)str->dims[i].dim_n, sizeof(int),
                                    (xdrproc_t)xdr_int)) {
                        return 0;
                    }
                    break;

                case UDA_TYPE_LONG:
                    if (!xdr_vector(xdrs, str->dims[i].dim, (u_int)str->dims[i].dim_n, sizeof(long),
                                    (xdrproc_t)xdr_long)) {
                        return 0;
                    }
                    break;

                case UDA_TYPE_LONG64:
                    if (!xdr_vector(xdrs, str->dims[i].dim, (u_int)str->dims[i].dim_n, sizeof(long long int),
                                    (xdrproc_t)xdr_int64_t)) {
                        return 0;
                    }
                    break;

                case UDA_TYPE_UNSIGNED_CHAR:
                    if (!xdr_vector(xdrs, str->dims[i].dim, (u_int)str->dims[i].dim_n, sizeof(unsigned char),
                                    (xdrproc_t)xdr_u_char)) {
                        return 0;
                    }
                    break;

                case UDA_TYPE_UNSIGNED_SHORT:
                    if (!xdr_vector(xdrs, str->dims[i].dim, (u_int)str->dims[i].dim_n, sizeof(unsigned short),
                                    (xdrproc_t)xdr_u_short)) {
                        return 0;
                    }
                    break;

                case UDA_TYPE_UNSIGNED_INT:
                    if (!xdr_vector(xdrs, str->dims[i].dim, (u_int)str->dims[i].dim_n, sizeof(unsigned int),
                                    (xdrproc_t)xdr_u_int)) {
                        return 0;
                    }
                    break;

                case UDA_TYPE_UNSIGNED_LONG:
                    if (!xdr_vector(xdrs, str->dims[i].dim, (u_int)str->dims[i].dim_n, sizeof(unsigned long),
                                    (xdrproc_t)xdr_u_long)) {
                        return 0;
                    }
                    break;
                case UDA_TYPE_UNSIGNED_LONG64:
                    if (!xdr_vector(xdrs, str->dims[i].dim, (u_int)str->dims[i].dim_n, sizeof(unsigned long long int),
                                    (xdrproc_t)xdr_uint64_t)) {
                        return 0;
                    }
                    break;

                    // Complex structure is a simple two float combination: => twice the number of element transmitted

                case UDA_TYPE_DCOMPLEX:
                    if (!xdr_vector(xdrs, str->dims[i].dim, 2 * (u_int)str->dims[i].dim_n, sizeof(double),
                                    (xdrproc_t)xdr_double)) {
                        return 0;
                    }
                    break;

                case UDA_TYPE_COMPLEX:
                    if (!xdr_vector(xdrs, str->dims[i].dim, 2 * (u_int)str->dims[i].dim_n, sizeof(float),
                                    (xdrproc_t)xdr_float)) {
                        return 0;
                    }
                    break;

                default:
                    return 0;
            }
        } else {
            if (str->dims[i].method > 0) {
                switch (str->dims[i].data_type) {
                    case UDA_TYPE_FLOAT:
                        switch (str->dims[i].method) {
                            case 1:
                                if (!(xdr_vector(xdrs, (char*)str->dims[i].sams, str->dims[i].udoms, sizeof(int),
                                                 (xdrproc_t)xdr_int) &&
                                      xdr_vector(xdrs, str->dims[i].offs, (int)str->dims[i].udoms, sizeof(float),
                                                 (xdrproc_t)xdr_float) &&
                                      xdr_vector(xdrs, str->dims[i].ints, (int)str->dims[i].udoms, sizeof(float),
                                                 (xdrproc_t)xdr_float))) {
                                    return 0;
                                }
                                break;
                            case 2:
                                if (!xdr_vector(xdrs, str->dims[i].offs, (int)str->dims[i].udoms, sizeof(float),
                                                (xdrproc_t)xdr_float)) {
                                    return 0;
                                }
                                break;
                            case 3:
                                if (!(xdr_vector(xdrs, str->dims[i].offs, 1, sizeof(float), (xdrproc_t)xdr_float) &&
                                      xdr_vector(xdrs, str->dims[i].ints, 1, sizeof(float), (xdrproc_t)xdr_float))) {
                                    return 0;
                                }
                                break;
                        }
                        break;

                    case UDA_TYPE_DOUBLE:
                        switch (str->dims[i].method) {
                            case 1:
                                if (!(xdr_vector(xdrs, (char*)str->dims[i].sams, str->dims[i].udoms, sizeof(int),
                                                 (xdrproc_t)xdr_int) &&
                                      xdr_vector(xdrs, str->dims[i].offs, str->dims[i].udoms, sizeof(double),
                                                 (xdrproc_t)xdr_double) &&
                                      xdr_vector(xdrs, str->dims[i].ints, str->dims[i].udoms, sizeof(double),
                                                 (xdrproc_t)xdr_double))) {
                                    return 0;
                                }
                                break;
                            case 2:
                                if (!xdr_vector(xdrs, str->dims[i].offs, str->dims[i].udoms, sizeof(double),
                                                (xdrproc_t)xdr_double)) {
                                    return 0;
                                }
                                break;
                            case 3:
                                if (!(xdr_vector(xdrs, str->dims[i].offs, 1, sizeof(double), (xdrproc_t)xdr_double) &&
                                      xdr_vector(xdrs, str->dims[i].ints, 1, sizeof(double), (xdrproc_t)xdr_double))) {
                                    return 0;
                                }
                                break;
                        }
                        break;

                    case UDA_TYPE_CHAR:
                        switch (str->dims[i].method) {
                            case 1:
                                if (!(xdr_vector(xdrs, (char*)str->dims[i].sams, (int)str->dims[i].udoms, sizeof(int),
                                                 (xdrproc_t)xdr_int) &&
                                      xdr_vector(xdrs, str->dims[i].offs, (int)str->dims[i].udoms, sizeof(char),
                                                 (xdrproc_t)xdr_char) &&
                                      xdr_vector(xdrs, str->dims[i].ints, (int)str->dims[i].udoms, sizeof(char),
                                                 (xdrproc_t)xdr_char))) {
                                    return 0;
                                }
                                break;
                            case 2:
                                if (!xdr_vector(xdrs, str->dims[i].offs, (int)str->dims[i].udoms, sizeof(char),
                                                (xdrproc_t)xdr_char)) {
                                    return 0;
                                }
                                break;
                            case 3:
                                if (!(xdr_vector(xdrs, str->dims[i].offs, 1, sizeof(char), (xdrproc_t)xdr_char) &&
                                      xdr_vector(xdrs, str->dims[i].ints, 1, sizeof(char), (xdrproc_t)xdr_char))) {
                                    return 0;
                                }
                                break;
                        }
                        break;

                    case UDA_TYPE_SHORT:
                        switch (str->dims[i].method) {
                            case 1:
                                if (!(xdr_vector(xdrs, (char*)str->dims[i].sams, (int)str->dims[i].udoms, sizeof(int),
                                                 (xdrproc_t)xdr_int) &&
                                      xdr_vector(xdrs, str->dims[i].offs, (int)str->dims[i].udoms, sizeof(short),
                                                 (xdrproc_t)xdr_short) &&
                                      xdr_vector(xdrs, str->dims[i].ints, (int)str->dims[i].udoms, sizeof(short),
                                                 (xdrproc_t)xdr_short))) {
                                    return 0;
                                }
                                break;
                            case 2:
                                if (!xdr_vector(xdrs, str->dims[i].offs, (int)str->dims[i].udoms, sizeof(short),
                                                (xdrproc_t)xdr_short)) {
                                    return 0;
                                }
                                break;
                            case 3:
                                if (!(xdr_vector(xdrs, str->dims[i].offs, 1, sizeof(short), (xdrproc_t)xdr_short) &&
                                      xdr_vector(xdrs, str->dims[i].ints, 1, sizeof(short), (xdrproc_t)xdr_short))) {
                                    return 0;
                                }
                                break;
                        }
                        break;

                    case UDA_TYPE_INT:
                        switch (str->dims[i].method) {
                            case 1:
                                if (!(xdr_vector(xdrs, (char*)str->dims[i].sams, (int)str->dims[i].udoms, sizeof(int),
                                                 (xdrproc_t)xdr_int) &&
                                      xdr_vector(xdrs, str->dims[i].offs, (int)str->dims[i].udoms, sizeof(int),
                                                 (xdrproc_t)xdr_int) &&
                                      xdr_vector(xdrs, str->dims[i].ints, (int)str->dims[i].udoms, sizeof(int),
                                                 (xdrproc_t)xdr_int))) {
                                    return 0;
                                }
                                break;
                            case 2:
                                if (!xdr_vector(xdrs, str->dims[i].offs, (int)str->dims[i].udoms, sizeof(int),
                                                (xdrproc_t)xdr_int)) {
                                    return 0;
                                }
                                break;
                            case 3:
                                if (!(xdr_vector(xdrs, str->dims[i].offs, 1, sizeof(int), (xdrproc_t)xdr_int) &&
                                      xdr_vector(xdrs, str->dims[i].ints, 1, sizeof(int), (xdrproc_t)xdr_int))) {
                                    return 0;
                                }
                                break;
                        }
                        break;

                    case UDA_TYPE_LONG:
                        switch (str->dims[i].method) {
                            case 1:
                                if (!(xdr_vector(xdrs, (char*)str->dims[i].sams, (int)str->dims[i].udoms, sizeof(int),
                                                 (xdrproc_t)xdr_int) &&
                                      xdr_vector(xdrs, str->dims[i].offs, (int)str->dims[i].udoms, sizeof(long),
                                                 (xdrproc_t)xdr_long) &&
                                      xdr_vector(xdrs, str->dims[i].ints, (int)str->dims[i].udoms, sizeof(long),
                                                 (xdrproc_t)xdr_long))) {
                                    return 0;
                                }
                                break;
                            case 2:
                                if (!xdr_vector(xdrs, str->dims[i].offs, (int)str->dims[i].udoms, sizeof(long),
                                                (xdrproc_t)xdr_long)) {
                                    return 0;
                                }
                                break;
                            case 3:
                                if (!(xdr_vector(xdrs, str->dims[i].offs, 1, sizeof(long), (xdrproc_t)xdr_long) &&
                                      xdr_vector(xdrs, str->dims[i].ints, 1, sizeof(long), (xdrproc_t)xdr_long))) {
                                    return 0;
                                }
                                break;
                        }
                        break;

                    case UDA_TYPE_LONG64:
                        switch (str->dims[i].method) {
                            case 1:
                                if (!(xdr_vector(xdrs, (char*)str->dims[i].sams, (int)str->dims[i].udoms, sizeof(int),
                                                 (xdrproc_t)xdr_int) &&
                                      xdr_vector(xdrs, str->dims[i].offs, (int)str->dims[i].udoms,
                                                 sizeof(long long int), (xdrproc_t)xdr_int64_t) &&
                                      xdr_vector(xdrs, str->dims[i].ints, (int)str->dims[i].udoms,
                                                 sizeof(long long int), (xdrproc_t)xdr_int64_t))) {
                                    return 0;
                                }
                                break;
                            case 2:
                                if (!xdr_vector(xdrs, str->dims[i].offs, (int)str->dims[i].udoms, sizeof(long long int),
                                                (xdrproc_t)xdr_int64_t)) {
                                    return 0;
                                }
                                break;
                            case 3:
                                if (!(xdr_vector(xdrs, str->dims[i].offs, 1, sizeof(long long int),
                                                 (xdrproc_t)xdr_int64_t) &&
                                      xdr_vector(xdrs, str->dims[i].ints, 1, sizeof(long long int),
                                                 (xdrproc_t)xdr_int64_t))) {
                                    return 0;
                                }
                                break;
                        }
                        break;

                    case UDA_TYPE_UNSIGNED_CHAR:
                        switch (str->dims[i].method) {
                            case 1:
                                if (!(xdr_vector(xdrs, (char*)str->dims[i].sams, (int)str->dims[i].udoms, sizeof(int),
                                                 (xdrproc_t)xdr_int) &&
                                      xdr_vector(xdrs, str->dims[i].offs, (int)str->dims[i].udoms,
                                                 sizeof(unsigned char), (xdrproc_t)xdr_u_char) &&
                                      xdr_vector(xdrs, str->dims[i].ints, (int)str->dims[i].udoms,
                                                 sizeof(unsigned char), (xdrproc_t)xdr_u_char))) {
                                    return 0;
                                }
                                break;
                            case 2:
                                if (!xdr_vector(xdrs, str->dims[i].offs, (int)str->dims[i].udoms, sizeof(unsigned char),
                                                (xdrproc_t)xdr_u_char)) {
                                    return 0;
                                }
                                break;
                            case 3:
                                if (!(xdr_vector(xdrs, str->dims[i].offs, 1, sizeof(unsigned char),
                                                 (xdrproc_t)xdr_u_char) &&
                                      xdr_vector(xdrs, str->dims[i].ints, 1, sizeof(unsigned char),
                                                 (xdrproc_t)xdr_u_char))) {
                                    return 0;
                                }
                                break;
                        }
                        break;

                    case UDA_TYPE_UNSIGNED_SHORT:
                        switch (str->dims[i].method) {
                            case 1:
                                if (!(xdr_vector(xdrs, (char*)str->dims[i].sams, (int)str->dims[i].udoms, sizeof(int),
                                                 (xdrproc_t)xdr_int) &&
                                      xdr_vector(xdrs, str->dims[i].offs, (int)str->dims[i].udoms,
                                                 sizeof(unsigned short), (xdrproc_t)xdr_u_short) &&
                                      xdr_vector(xdrs, str->dims[i].ints, (int)str->dims[i].udoms,
                                                 sizeof(unsigned short), (xdrproc_t)xdr_u_short))) {
                                    return 0;
                                }
                                break;
                            case 2:
                                if (!xdr_vector(xdrs, str->dims[i].offs, (int)str->dims[i].udoms,
                                                sizeof(unsigned short), (xdrproc_t)xdr_u_short)) {
                                    return 0;
                                }
                                break;
                            case 3:
                                if (!(xdr_vector(xdrs, str->dims[i].offs, 1, sizeof(unsigned short),
                                                 (xdrproc_t)xdr_u_short) &&
                                      xdr_vector(xdrs, str->dims[i].ints, 1, sizeof(unsigned short),
                                                 (xdrproc_t)xdr_u_short))) {
                                    return 0;
                                }
                                break;
                        }
                        break;

                    case UDA_TYPE_UNSIGNED_INT:
                        switch (str->dims[i].method) {
                            case 1:
                                if (!(xdr_vector(xdrs, (char*)str->dims[i].sams, (int)str->dims[i].udoms, sizeof(int),
                                                 (xdrproc_t)xdr_int) &&
                                      xdr_vector(xdrs, str->dims[i].offs, (int)str->dims[i].udoms, sizeof(unsigned int),
                                                 (xdrproc_t)xdr_u_int) &&
                                      xdr_vector(xdrs, str->dims[i].ints, (int)str->dims[i].udoms, sizeof(unsigned int),
                                                 (xdrproc_t)xdr_u_int))) {
                                    return 0;
                                }
                                break;
                            case 2:
                                if (!xdr_vector(xdrs, str->dims[i].offs, (int)str->dims[i].udoms, sizeof(unsigned int),
                                                (xdrproc_t)xdr_u_int)) {
                                    return 0;
                                }
                                break;
                            case 3:
                                if (!(xdr_vector(xdrs, str->dims[i].offs, 1, sizeof(unsigned int),
                                                 (xdrproc_t)xdr_u_int) &&
                                      xdr_vector(xdrs, str->dims[i].ints, 1, sizeof(unsigned int),
                                                 (xdrproc_t)xdr_u_int))) {
                                    return 0;
                                }
                                break;
                        }
                        break;

                    case UDA_TYPE_UNSIGNED_LONG:
                        switch (str->dims[i].method) {
                            case 1:
                                if (!(xdr_vector(xdrs, (char*)str->dims[i].sams, (int)str->dims[i].udoms, sizeof(int),
                                                 (xdrproc_t)xdr_int) &&
                                      xdr_vector(xdrs, str->dims[i].offs, (int)str->dims[i].udoms,
                                                 sizeof(unsigned long), (xdrproc_t)xdr_u_long) &&
                                      xdr_vector(xdrs, str->dims[i].ints, (int)str->dims[i].udoms,
                                                 sizeof(unsigned long), (xdrproc_t)xdr_u_long))) {
                                    return 0;
                                }
                                break;
                            case 2:
                                if (!xdr_vector(xdrs, str->dims[i].offs, (int)str->dims[i].udoms, sizeof(unsigned long),
                                                (xdrproc_t)xdr_u_long)) {
                                    return 0;
                                }
                                break;
                            case 3:
                                if (!(xdr_vector(xdrs, str->dims[i].offs, 1, sizeof(unsigned long),
                                                 (xdrproc_t)xdr_u_long) &&
                                      xdr_vector(xdrs, str->dims[i].ints, 1, sizeof(unsigned long),
                                                 (xdrproc_t)xdr_u_long))) {
                                    return 0;
                                }
                                break;
                        }
                        break;
                    case UDA_TYPE_UNSIGNED_LONG64:
                        switch (str->dims[i].method) {
                            case 1:
                                if (!(xdr_vector(xdrs, (char*)str->dims[i].sams, (int)str->dims[i].udoms, sizeof(int),
                                                 (xdrproc_t)xdr_int) &&
                                      xdr_vector(xdrs, str->dims[i].offs, (int)str->dims[i].udoms,
                                                 sizeof(unsigned long long int), (xdrproc_t)xdr_uint64_t) &&
                                      xdr_vector(xdrs, str->dims[i].ints, (int)str->dims[i].udoms,
                                                 sizeof(unsigned long long int), (xdrproc_t)xdr_uint64_t))) {
                                    return 0;
                                }
                                break;
                            case 2:
                                if (!xdr_vector(xdrs, str->dims[i].offs, (int)str->dims[i].udoms, sizeof(unsigned long),
                                                (xdrproc_t)xdr_uint64_t)) {
                                    return 0;
                                }
                                break;
                            case 3:
                                if (!(xdr_vector(xdrs, str->dims[i].offs, (int)1, sizeof(unsigned long long int),
                                                 (xdrproc_t)xdr_uint64_t) &&
                                      xdr_vector(xdrs, str->dims[i].ints, (int)1, sizeof(unsigned long long int),
                                                 (xdrproc_t)xdr_uint64_t))) {
                                    return 0;
                                }
                                break;
                        }
                        break;

                        // COMPLEX Types are Not Compressed

                    default:
                        return 0;
                }
            }
        }
    }

    return 1;
}

bool_t uda::client_server::xdr_data_dim3(XDR* xdrs, DATA_BLOCK* str)
{
    int rc, arc = 1;
    for (unsigned int i = 0; i < str->rank; i++) {

        if (str->dims[i].error_param_n > 0) {
            xdr_vector(xdrs, (char*)str->dims[i].errparams, (unsigned int)str->dims[i].error_param_n, sizeof(float),
                       (xdrproc_t)xdr_float);
        }

        switch (str->dims[i].error_type) {
            case UDA_TYPE_FLOAT:
                rc = xdr_vector(xdrs, str->dims[i].errhi, (u_int)str->dims[i].dim_n, sizeof(float),
                                (xdrproc_t)xdr_float);
                break;
            case UDA_TYPE_DOUBLE:
                rc = xdr_vector(xdrs, str->dims[i].errhi, (u_int)str->dims[i].dim_n, sizeof(double),
                                (xdrproc_t)xdr_double);
                break;
            case UDA_TYPE_CHAR:
                rc = xdr_vector(xdrs, str->dims[i].errhi, (u_int)str->dims[i].dim_n, sizeof(char), (xdrproc_t)xdr_char);
                break;
            case UDA_TYPE_SHORT:
                rc = xdr_vector(xdrs, str->dims[i].errhi, (u_int)str->dims[i].dim_n, sizeof(short),
                                (xdrproc_t)xdr_short);
                break;
            case UDA_TYPE_INT:
                rc = xdr_vector(xdrs, str->dims[i].errhi, (u_int)str->dims[i].dim_n, sizeof(int), (xdrproc_t)xdr_int);
                break;
            case UDA_TYPE_LONG:
                rc = xdr_vector(xdrs, str->dims[i].errhi, (u_int)str->dims[i].dim_n, sizeof(long), (xdrproc_t)xdr_long);
                break;
            case UDA_TYPE_LONG64:
                rc = xdr_vector(xdrs, str->dims[i].errhi, (u_int)str->dims[i].dim_n, sizeof(long long int),
                                (xdrproc_t)xdr_int64_t);
                break;
            case UDA_TYPE_UNSIGNED_CHAR:
                rc = xdr_vector(xdrs, str->dims[i].errhi, (u_int)str->dims[i].dim_n, sizeof(unsigned char),
                                (xdrproc_t)xdr_u_char);
                break;
            case UDA_TYPE_UNSIGNED_SHORT:
                rc = xdr_vector(xdrs, str->dims[i].errhi, (u_int)str->dims[i].dim_n, sizeof(unsigned short),
                                (xdrproc_t)xdr_u_short);
                break;
            case UDA_TYPE_UNSIGNED_INT:
                rc = xdr_vector(xdrs, str->dims[i].errhi, (u_int)str->dims[i].dim_n, sizeof(unsigned int),
                                (xdrproc_t)xdr_u_int);
                break;
            case UDA_TYPE_UNSIGNED_LONG:
                rc = xdr_vector(xdrs, str->dims[i].errhi, (u_int)str->dims[i].dim_n, sizeof(unsigned long),
                                (xdrproc_t)xdr_u_long);
                break;
            case UDA_TYPE_UNSIGNED_LONG64:
                rc = xdr_vector(xdrs, str->dims[i].errhi, (u_int)str->dims[i].dim_n, sizeof(unsigned long long int),
                                (xdrproc_t)xdr_uint64_t);
                break;

                // Complex structure is a simple two float combination: => twice the number of element transmitted

            case UDA_TYPE_DCOMPLEX:
                rc = xdr_vector(xdrs, str->dims[i].errhi, 2 * (u_int)str->dims[i].dim_n, sizeof(double),
                                (xdrproc_t)xdr_double);
                break;
            case UDA_TYPE_COMPLEX:
                rc = xdr_vector(xdrs, str->dims[i].errhi, 2 * (u_int)str->dims[i].dim_n, sizeof(float),
                                (xdrproc_t)xdr_float);
                break;

            default:
                rc = 1;
                break;
        }
        if (!(arc = arc && rc)) {
            return 0;
        }
    }

    return 1;
}

bool_t uda::client_server::xdr_data_dim4(XDR* xdrs, DATA_BLOCK* str)
{
    int arc = 1, rc;
    for (unsigned int i = 0; i < str->rank; i++) {
        if (str->dims[i].errasymmetry) {
            switch (str->dims[i].error_type) {
                case UDA_TYPE_FLOAT:
                    rc = xdr_vector(xdrs, str->dims[i].errlo, (u_int)str->dims[i].dim_n, sizeof(float),
                                    (xdrproc_t)xdr_float);
                    break;
                case UDA_TYPE_DOUBLE:
                    rc = xdr_vector(xdrs, str->dims[i].errlo, (u_int)str->dims[i].dim_n, sizeof(double),
                                    (xdrproc_t)xdr_double);
                    break;
                case UDA_TYPE_CHAR:
                    rc = xdr_vector(xdrs, str->dims[i].errlo, (u_int)str->dims[i].dim_n, sizeof(char),
                                    (xdrproc_t)xdr_char);
                    break;
                case UDA_TYPE_SHORT:
                    rc = xdr_vector(xdrs, str->dims[i].errlo, (u_int)str->dims[i].dim_n, sizeof(short),
                                    (xdrproc_t)xdr_short);
                    break;
                case UDA_TYPE_INT:
                    rc = xdr_vector(xdrs, str->dims[i].errlo, (u_int)str->dims[i].dim_n, sizeof(int),
                                    (xdrproc_t)xdr_int);
                    break;
                case UDA_TYPE_LONG:
                    rc = xdr_vector(xdrs, str->dims[i].errlo, (u_int)str->dims[i].dim_n, sizeof(long),
                                    (xdrproc_t)xdr_long);
                    break;
                case UDA_TYPE_LONG64:
                    rc = xdr_vector(xdrs, str->dims[i].errlo, (u_int)str->dims[i].dim_n, sizeof(long long int),
                                    (xdrproc_t)xdr_int64_t);
                    break;
                case UDA_TYPE_UNSIGNED_CHAR:
                    rc = xdr_vector(xdrs, str->dims[i].errlo, (u_int)str->dims[i].dim_n, sizeof(unsigned char),
                                    (xdrproc_t)xdr_u_char);
                    break;
                case UDA_TYPE_UNSIGNED_SHORT:
                    rc = xdr_vector(xdrs, str->dims[i].errlo, (u_int)str->dims[i].dim_n, sizeof(unsigned short),
                                    (xdrproc_t)xdr_u_short);
                    break;
                case UDA_TYPE_UNSIGNED_INT:
                    rc = xdr_vector(xdrs, str->dims[i].errlo, (u_int)str->dims[i].dim_n, sizeof(unsigned int),
                                    (xdrproc_t)xdr_u_int);
                    break;
                case UDA_TYPE_UNSIGNED_LONG:
                    rc = xdr_vector(xdrs, str->dims[i].errlo, (u_int)str->dims[i].dim_n, sizeof(unsigned long),
                                    (xdrproc_t)xdr_u_long);
                    break;
                case UDA_TYPE_UNSIGNED_LONG64:
                    rc = xdr_vector(xdrs, str->dims[i].errlo, (u_int)str->dims[i].dim_n, sizeof(unsigned long long int),
                                    (xdrproc_t)xdr_uint64_t);
                    break;

                    // Complex structure is a simple two float combination: => twice the number of element transmitted

                case UDA_TYPE_DCOMPLEX:
                    rc = xdr_vector(xdrs, str->dims[i].errlo, 2 * (u_int)str->dims[i].dim_n, sizeof(double),
                                    (xdrproc_t)xdr_double);
                    break;
                case UDA_TYPE_COMPLEX:
                    rc = xdr_vector(xdrs, str->dims[i].errlo, 2 * (u_int)str->dims[i].dim_n, sizeof(float),
                                    (xdrproc_t)xdr_float);
                    break;

                default:
                    rc = 1;
                    break;
            }
            if (!(arc = arc && rc)) {
                return 0;
            }
        }
    }

    return 1;
}

//-----------------------------------------------------------------------
// From DATA_SYSTEM Table

bool_t uda::client_server::xdr_data_system(XDR* xdrs, DATA_SYSTEM* str)
{
    return xdr_int(xdrs, &str->system_id) && xdr_int(xdrs, &str->version) && xdr_int(xdrs, &str->meta_id) &&
           xdr_char(xdrs, &str->type) && WrapXDRString(xdrs, (char*)str->device_name, STRING_LENGTH) &&
           WrapXDRString(xdrs, (char*)str->system_name, STRING_LENGTH) &&
           WrapXDRString(xdrs, (char*)str->system_desc, MAX_STRING_LENGTH) &&
           WrapXDRString(xdrs, (char*)str->creation, DATE_LENGTH) && WrapXDRString(xdrs, (char*)str->xml, MAXMETA) &&
           WrapXDRString(xdrs, (char*)str->xml_creation, DATE_LENGTH);
}

//-----------------------------------------------------------------------
// From SYSTEM_CONFIG Table

bool_t uda::client_server::xdr_system_config(XDR* xdrs, SYSTEM_CONFIG* str)
{
    return xdr_int(xdrs, &str->config_id) && xdr_int(xdrs, &str->system_id) && xdr_int(xdrs, &str->meta_id) &&
           WrapXDRString(xdrs, (char*)str->config_name, STRING_LENGTH) &&
           WrapXDRString(xdrs, (char*)str->config_desc, MAX_STRING_LENGTH) &&
           WrapXDRString(xdrs, (char*)str->creation, DATE_LENGTH) && WrapXDRString(xdrs, (char*)str->xml, MAXMETA) &&
           WrapXDRString(xdrs, (char*)str->xml_creation, DATE_LENGTH);
}

//-----------------------------------------------------------------------
// From DATA_SOURCE Table

bool_t uda::client_server::xdr_data_source(XDR* xdrs, DATA_SOURCE* str)
{
    return xdr_int(xdrs, &str->source_id) && xdr_int(xdrs, &str->config_id) && xdr_int(xdrs, &str->reason_id) &&
           xdr_int(xdrs, &str->run_id) && xdr_int(xdrs, &str->meta_id) && xdr_int(xdrs, &str->status_desc_id) &&
           xdr_int(xdrs, &str->exp_number) && xdr_int(xdrs, &str->pass) && xdr_int(xdrs, &str->status) &&
           xdr_int(xdrs, &str->status_reason_code) && xdr_int(xdrs, &str->status_impact_code) &&
           xdr_char(xdrs, &str->access) && xdr_char(xdrs, &str->reprocess) && xdr_char(xdrs, &str->type) &&
           WrapXDRString(xdrs, (char*)str->source_alias, STRING_LENGTH) &&
           WrapXDRString(xdrs, (char*)str->pass_date, DATE_LENGTH) &&
           WrapXDRString(xdrs, (char*)str->archive, STRING_LENGTH) &&
           WrapXDRString(xdrs, (char*)str->device_name, STRING_LENGTH) &&
           WrapXDRString(xdrs, (char*)str->format, STRING_LENGTH) &&
           WrapXDRString(xdrs, (char*)str->path, MAX_STRING_LENGTH) &&
           WrapXDRString(xdrs, (char*)str->filename, STRING_LENGTH) &&
           WrapXDRString(xdrs, (char*)str->server, STRING_LENGTH) &&
           WrapXDRString(xdrs, (char*)str->userid, STRING_LENGTH) &&
           WrapXDRString(xdrs, (char*)str->reason_desc, MAX_STRING_LENGTH) &&
           WrapXDRString(xdrs, (char*)str->status_desc, MAXMETA) &&
           WrapXDRString(xdrs, (char*)str->run_desc, MAXMETA) &&
           WrapXDRString(xdrs, (char*)str->creation, DATE_LENGTH) &&
           WrapXDRString(xdrs, (char*)str->modified, DATE_LENGTH) && WrapXDRString(xdrs, (char*)str->xml, MAXMETA) &&
           WrapXDRString(xdrs, (char*)str->xml_creation, DATE_LENGTH);
}

//-----------------------------------------------------------------------
// From SIGNAL Table

bool_t uda::client_server::xdr_signal(XDR* xdrs, SIGNAL* str)
{
    return xdr_int(xdrs, &str->source_id) && xdr_int(xdrs, &str->signal_desc_id) && xdr_int(xdrs, &str->meta_id) &&
           xdr_int(xdrs, &str->status_desc_id) && xdr_int(xdrs, &str->status) &&
           xdr_int(xdrs, &str->status_reason_code) && xdr_int(xdrs, &str->status_impact_code) &&
           xdr_char(xdrs, &str->access) && xdr_char(xdrs, &str->reprocess) &&
           WrapXDRString(xdrs, (char*)str->status_desc, MAXMETA) &&
           WrapXDRString(xdrs, (char*)str->creation, DATE_LENGTH) &&
           WrapXDRString(xdrs, (char*)str->modified, DATE_LENGTH) && WrapXDRString(xdrs, (char*)str->xml, MAXMETA) &&
           WrapXDRString(xdrs, (char*)str->xml_creation, DATE_LENGTH);
}

//-----------------------------------------------------------------------
// From SIGNAL_DESC Table

bool_t uda::client_server::xdr_signal_desc(XDR* xdrs, SIGNAL_DESC* str)
{
    return xdr_int(xdrs, &str->signal_desc_id) && xdr_int(xdrs, &str->meta_id) && xdr_int(xdrs, &str->rank) &&
           xdr_int(xdrs, &str->range_start) && xdr_int(xdrs, &str->range_stop) && xdr_char(xdrs, &str->type) &&
           WrapXDRString(xdrs, (char*)str->source_alias, STRING_LENGTH) &&
           WrapXDRString(xdrs, (char*)str->signal_alias, STRING_LENGTH) &&
           WrapXDRString(xdrs, (char*)str->signal_name, STRING_LENGTH) &&
           WrapXDRString(xdrs, (char*)str->generic_name, STRING_LENGTH) &&
           WrapXDRString(xdrs, (char*)str->description, MAX_STRING_LENGTH) &&
           WrapXDRString(xdrs, (char*)str->signal_class, MAX_STRING_LENGTH) &&
           WrapXDRString(xdrs, (char*)str->signal_owner, MAX_STRING_LENGTH) &&
           WrapXDRString(xdrs, (char*)str->creation, DATE_LENGTH) &&
           WrapXDRString(xdrs, (char*)str->modified, DATE_LENGTH) && WrapXDRString(xdrs, (char*)str->xml, MAXMETA) &&
           WrapXDRString(xdrs, (char*)str->xml_creation, DATE_LENGTH);
}
