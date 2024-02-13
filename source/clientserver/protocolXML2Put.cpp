#include "protocolXML2Put.h"

#include <cstdlib>
#include <uda/structured.h>

#include "logging/logging.h"
#include <structures/xdrUserDefinedData.h>

#include "errorLog.h"
#include "protocol.h"
#include "stringUtils.h"
#include "structures/struct.h"
#include "xdrlib.h"

#ifdef SERVERBUILD
#  include "server/serverStartup.h"
#endif

using namespace uda::client_server;
using namespace uda::logging;

static int recursiveDepthPut = 0; // Keep count of recursive calls

int uda::client_server::xdrUserDefinedDataPut(XDR* xdrs, LOGMALLOCLIST* logmalloclist, LOGSTRUCTLIST* log_struct_list,
                                              USERDEFINEDTYPELIST* userdefinedtypelist,
                                              USERDEFINEDTYPE* userdefinedtype, void** data, int datacount,
                                              int structRank, int* structShape, int index, NTREE** NTree,
                                              int protocolVersion, int malloc_source)
{
    // Grow the data tree recursively through pointer elements within individual structures
    // Build a linked list tree structure when receiving data.

    // Sending: data points to the memory location of the structure, defined by userdefinedtype, to be sent
    // Receiving: userdefinedtype contains the definition of the structure to be received.

    int rc = 1, id, loopcount, rank, count, size, passdata = 0, isSOAP;
    int* shape;
    char *p0, *d;
    const char* type;

    char rudtype[MAXELEMENTNAME]; // Received name of the user defined type
    const char* chartype = "char";

    VOIDTYPE* p; // Type Needs to be the same size as a local pointer, e.g.,  int* on 32 and long long* on 64 bit
    VOIDTYPE* prev = nullptr; // Pointer to previous structure element (need to manage SOAP data bindings)

    USERDEFINEDTYPE* utype = nullptr;

    NTREE* newNTree = nullptr;
    NTREE* subNTree = nullptr;

    // Flag whether there is Data to Send or Receive

    if (xdrs->x_op == XDR_DECODE) {
        rc = xdr_int(xdrs, &passdata);
    } else {
        passdata = 0;
        if (data != nullptr) {
            passdata = *(int**)data != nullptr;
        }
        rc = xdr_int(xdrs, &passdata);
    }

    if (!passdata) {
        return rc;
    }

    // If the recursive depth is too large then perhaps an infinite loop is in play!

    if (recursiveDepthPut++ > MAXRECURSIVEDEPTH) {
        add_error(UDA_CODE_ERROR_TYPE, "xdrUserDefinedData", 999, "Maximum Recursive Depth reached!");
        return 0;
    }

    UDA_LOG(UDA_LOG_DEBUG, "xdrUserDefinedData Depth: %d\n", recursiveDepthPut);

    // Allocate HEAP if receiving Data:
    // Size of Structure or atomic type array or scalar is provided by the structure definition (corrected for alignment
    // and architecture) datacount, rank and shape are passed via the argument - it's the number of structure array
    // elements to allocate in a single block Index is the element of the structure array to be received Allocation
    // occurs only when index == 0, i.e. the first time the function is entered. Rank and shape of structured data are
    // captured. Allocation occurs only when datacount > 0, i.e. Must be a Pointer to a User Defined Type Structure:
    // Otherwise it's already allocated - but must add a tree node to the structure element.
    //
    // Child nodes of the same type are branched consequentially

    if (xdrs->x_op == XDR_DECODE) {

        UDA_LOG(UDA_LOG_DEBUG, "index: %d   datacount: %d\n", index, datacount);

        if (index == 0 && datacount > 0) {
            *data = malloc(datacount * userdefinedtype->size);
            if (structRank > 1 && structShape != nullptr) {
                udaAddMalloc2(logmalloclist, *data, datacount, userdefinedtype->size, userdefinedtype->name, structRank,
                              structShape);
            } else {
                udaAddMalloc(logmalloclist, *data, datacount, userdefinedtype->size, userdefinedtype->name);
            }
            structRank = 0;
        }

        newNTree = (NTREE*)malloc(sizeof(NTREE)); // this is the parent node for the received structure
        // dgm 15Nov2011
        udaAddMalloc(logmalloclist, (void*)newNTree, 1, sizeof(NTREE), "NTREE");

        *NTree = newNTree; // Return the new tree node address

        initNTree(newNTree);
        newNTree->data = nullptr;
        newNTree->userdefinedtype = userdefinedtype; // preserve Pairing of data and data type
    }

    // Start of the Structure Array Element

    p0 = *((char**)data) + index * userdefinedtype->size;

    if (xdrs->x_op == XDR_DECODE) {
        newNTree->data = (void*)p0;
    } // Each tree node points to a structure or an atomic array

    // Loop over all structure elements: Send or Receive

    for (int j = 0; j < userdefinedtype->fieldcount; j++) {

        if (j >= userdefinedtype->fieldcount) {
            add_error(UDA_CODE_ERROR_TYPE, "xdrUserDefinedData", 999, "Fieldcount Exceeded!");
            break;
        }

        p = (VOIDTYPE*)&p0[userdefinedtype->compoundfield[j].offset]; // the Element's location

        if (xdrs->x_op == XDR_DECODE && userdefinedtype->compoundfield[j].pointer) { // Initialise
            *p = 0; // nullptr pointer: to be allocated on heap
        }

        type = nullptr; // Reset pointer attributes
        d = nullptr;
        count = 0;
        size = 0;
        isSOAP = 0;

        // Element Data by Atomic Type (Use recursion to manage user defined types)
        // Data can be a scalar, an array of fixed and known length or a pointer to an array also of known length

        switch (userdefinedtype->compoundfield[j].atomictype) {

            case UDA_TYPE_FLOAT: {

                UDA_LOG(UDA_LOG_DEBUG, "Type: FLOAT\n");

                if (userdefinedtype->compoundfield[j].pointer) { // Pointer to Float Data array
                    if (xdrs->x_op == XDR_DECODE) {              // Allocate Heap for Data Received
                        rc = rc &&
                             xdr_int(xdrs,
                                     &count); // Count is known from the client's malloc log and passed by the sender
                        if (count > 0) {
                            rc = rc && xdr_int(xdrs,
                                               &rank); // Receive Shape of pointer arrays (not included in definition)
                            if (rank > 1) {
                                shape = (int*)malloc(rank * sizeof(int)); // freed via the malloc log registration
                                rc = rc && xdr_vector(xdrs, (char*)shape, rank, sizeof(int), (xdrproc_t)xdr_int);
                            } else {
                                shape = nullptr;
                            }
                            d = (char*)malloc(count * sizeof(float));
                            udaAddMalloc2(logmalloclist, (void*)d, count, sizeof(float), "float", rank, shape);
                            *p = (VOIDTYPE)d;
                        } else {
                            break;
                        }
                        if (!rc) {
                            break;
                        }
                    } else {
                        d = (char*)*p; // data read from here
                        if (d == nullptr) {
                            count = 0;
                            rc = rc && xdr_int(xdrs, &count); // No data to send
                            break;
                        }

                        udaFindMalloc2(logmalloclist, (void*)p, &count, &size, &type, &rank,
                                       &shape); // Assume 0 means No Pointer data to send!

                        // Allocation of pointer data within SOAP is problematic.
                        // Data malloc'd within SOAP are typed "unknown".
                        // Generally, pointer class data are preceded within the data binding structures with integer
                        // elements named __size followed by the name of the element. The value stored by this element
                        // is the equivalent of the count parameter required from udaFindMalloc.
                        //
                        // Other data creators, e.g., XML DOM, also have types "unknown"
                        // In these cases, a best guess is made to the type and count based on expectations and the heap
                        // allocated - very unsatisfactory!

                        if (type != nullptr && STR_EQUALS(type, "unknown")) {
                            if (malloc_source == UDA_MALLOC_SOURCE_SOAP && j > 0 &&
                                STR_IEQUALS(userdefinedtype->compoundfield[j - 1].name, "__size") &&
                                STR_EQUALS(userdefinedtype->compoundfield[j].name,
                                           &userdefinedtype->compoundfield[j - 1].name[6])) {

                                count = (int)*prev; // the value of __size...
                                size = sizeof(float);
                                type = userdefinedtype->compoundfield[j].type;
                            } else {
                                if (count > 0) {
                                    int totalsize = count * size;           // from the malloc log
                                    int rcount = totalsize % sizeof(float); // array element count remainder
                                    size = sizeof(float);                   // element size
                                    count = totalsize / size;               // array element count

                                    if (rcount != 0) { // there should be no remainder
                                        add_error(UDA_CODE_ERROR_TYPE, "xdrUserDefinedData", 999,
                                                  "Specified malloc total size not integer multiple!");
                                        count = 0;
                                    }
                                }
                            }
                        }

                        rc = rc && xdr_int(xdrs, &count);

                        if ((count == 0 || size == 0) && *p != 0) {
                            add_error(UDA_CODE_ERROR_TYPE, "xdrUserDefinedData", 999,
                                      "Type Float Data Heap Allocation not found in log!");
                            break;
                        }

                        rc = rc && xdr_int(xdrs, &rank); // Send Shape of arrays
                        if (rank > 1) {
                            rc = rc && xdr_vector(xdrs, (char*)shape, rank, sizeof(int), (xdrproc_t)xdr_int);
                        }
                    }
                    rc = rc && xdr_vector(xdrs, d, count, sizeof(float), (xdrproc_t)xdr_float);
                } else {
                    if (userdefinedtype->compoundfield[j].rank == 0) { // Element is a Scalar of fixed size
                        rc = rc && xdr_float(xdrs, (float*)p);
                    } else { // Element is an Array of fixed size
                        rc = rc && xdr_vector(xdrs, (char*)p, userdefinedtype->compoundfield[j].count, sizeof(float),
                                              (xdrproc_t)xdr_float);
                    }
                }
                break;
            }

            case UDA_TYPE_DOUBLE: {
                UDA_LOG(UDA_LOG_DEBUG, "Type: DOUBLE\n");

                if (userdefinedtype->compoundfield[j].pointer) { // Pointer to Double Data array
                    if (xdrs->x_op == XDR_DECODE) {              // Allocate Heap for Data Received
                        rc = rc &&
                             xdr_int(xdrs,
                                     &count); // Count is known from the client's malloc log and passed by the sender
                        if (count > 0) {
                            rc = rc && xdr_int(xdrs,
                                               &rank); // Receive Shape of pointer arrays (not included in definition)
                            if (rank > 1) {
                                shape = (int*)malloc(rank * sizeof(int)); // freed via the malloc log registration
                                rc = rc && xdr_vector(xdrs, (char*)shape, rank, sizeof(int), (xdrproc_t)xdr_int);
                            } else {
                                shape = nullptr;
                            }
                            d = (char*)malloc(count * sizeof(double));
                            udaAddMalloc2(logmalloclist, (void*)d, count, sizeof(double), "double", rank, shape);
                            *p = (VOIDTYPE)d; // Save pointer: data will be written here
                        } else {
                            break;
                        }
                    } else {
                        d = (char*)*p; // data read from here
                        if (d == nullptr) {
                            count = 0;
                            rc = rc && xdr_int(xdrs, &count); // No data to send
                            break;
                        }

                        udaFindMalloc2(logmalloclist, (void*)p, &count, &size, &type, &rank,
                                       &shape); // Assume count of 0 means No Pointer data to send!

                        if (type != nullptr && STR_EQUALS(type, "unknown")) {
                            if (malloc_source == UDA_MALLOC_SOURCE_SOAP && j > 0 &&
                                STR_IEQUALS(userdefinedtype->compoundfield[j - 1].name, "__size") &&
                                STR_EQUALS(userdefinedtype->compoundfield[j].name,
                                           &userdefinedtype->compoundfield[j - 1].name[6])) {

                                count = (int)*prev; // the value of __size...
                                size = sizeof(double);
                                type = userdefinedtype->compoundfield[j].type;

                            } else {
                                if (count > 0) {
                                    int totalsize = count * size;
                                    int rcount = totalsize % sizeof(double);
                                    size = sizeof(double);
                                    count = totalsize / size;

                                    if (rcount != 0) {
                                        add_error(UDA_CODE_ERROR_TYPE, "xdrUserDefinedData", 999,
                                                  "Specified malloc total size not integer multiple!");
                                        count = 0;
                                    }
                                }
                            }
                        }

                        rc = rc && xdr_int(xdrs, &count);

                        if ((count == 0 || size == 0) && *p != 0) {
                            add_error(UDA_CODE_ERROR_TYPE, "xdrUserDefinedData", 999,
                                      "Type Double Data Heap Allocation not found in log!");
                            break;
                        }

                        rc = rc && xdr_int(xdrs, &rank); // Send Shape of arrays
                        if (rank > 1) {
                            rc = rc && xdr_vector(xdrs, (char*)shape, rank, sizeof(int), (xdrproc_t)xdr_int);
                        }
                    }
                    rc = rc && xdr_vector(xdrs, d, count, sizeof(double),
                                          (xdrproc_t)xdr_double); // Send or Receive data
                } else {
                    if (userdefinedtype->compoundfield[j].rank == 0) { // Element is a Scalar of fixed size
                        rc = rc && xdr_double(xdrs, (double*)p);
                    } else { // Element is an Array of fixed size
                        rc = rc && xdr_vector(xdrs, (char*)p, userdefinedtype->compoundfield[j].count, sizeof(double),
                                              (xdrproc_t)xdr_double);
                    }
                }
                break;
            }

            case UDA_TYPE_SHORT: {
                UDA_LOG(UDA_LOG_DEBUG, "Type: SHORT\n");

                if (userdefinedtype->compoundfield[j].pointer) { // Pointer to Short Data array
                    if (xdrs->x_op == XDR_DECODE) {              // Allocate Heap for Data
                        rc = rc &&
                             xdr_int(xdrs,
                                     &count); // Count is known from the client's malloc log and passed by the sender
                        if (count > 0) {
                            rc = rc && xdr_int(xdrs,
                                               &rank); // Receive Shape of pointer arrays (not included in definition)
                            if (rank > 1) {
                                shape = (int*)malloc(rank * sizeof(int)); // freed via the malloc log registration
                                rc = rc && xdr_vector(xdrs, (char*)shape, rank, sizeof(int), (xdrproc_t)xdr_int);
                            } else {
                                shape = nullptr;
                            }
                            d = (char*)malloc(count * sizeof(short));
                            udaAddMalloc2(logmalloclist, (void*)d, count, sizeof(short), "short", rank, shape);
                            *p = (VOIDTYPE)d; // Save pointer: data will be written here
                        } else {
                            break;
                        }
                    } else {
                        d = (char*)*p; // data read from here
                        if (d == nullptr) {
                            count = 0;
                            rc = rc && xdr_int(xdrs, &count); // No data to send
                            break;
                        }
                        udaFindMalloc2(logmalloclist, (void*)p, &count, &size, &type, &rank,
                                       &shape); // Assume count of 0 means No Pointer data to send!

                        if (type != nullptr && STR_EQUALS(type, "unknown")) {
                            if (malloc_source == UDA_MALLOC_SOURCE_SOAP && j > 0 &&
                                STR_IEQUALS(userdefinedtype->compoundfield[j - 1].name, "__size") &&
                                STR_EQUALS(userdefinedtype->compoundfield[j].name,
                                           &userdefinedtype->compoundfield[j - 1].name[6])) {

                                count = (int)*prev; // the value of __size...
                                size = sizeof(short);
                                type = userdefinedtype->compoundfield[j].type;
                            } else {
                                if (count > 0) {
                                    int totalsize = count * size;
                                    int rcount = totalsize % sizeof(short);
                                    size = sizeof(short);
                                    count = totalsize / size;

                                    if (rcount != 0) {
                                        add_error(UDA_CODE_ERROR_TYPE, "xdrUserDefinedData", 999,
                                                  "Specified short malloc total size not integer multiple!");
                                        count = 0;
                                    }
                                }
                            }
                        }

                        rc = rc && xdr_int(xdrs, &count);

                        if ((count == 0 || size == 0) && *p != 0) {
                            add_error(UDA_CODE_ERROR_TYPE, "xdrUserDefinedData", 999,
                                      "Short Data Heap Allocation not found in log!");
                            break;
                        }

                        rc = rc && xdr_int(xdrs, &rank); // Send Shape of arrays
                        if (rank > 1) {
                            rc = rc && xdr_vector(xdrs, (char*)shape, rank, sizeof(int), (xdrproc_t)xdr_int);
                        }
                    }
                    rc = rc && xdr_vector(xdrs, d, count, sizeof(short), (xdrproc_t)xdr_short);
                } else {
                    if (userdefinedtype->compoundfield[j].rank == 0) { // Element is a Scalar
                        rc = rc && xdr_short(xdrs, (short*)p);
                    } else { // Element is an Array of fixed size
                        rc = rc && xdr_vector(xdrs, (char*)p, userdefinedtype->compoundfield[j].count, sizeof(short),
                                              (xdrproc_t)xdr_short);
                    }
                }
                break;
            }

            case UDA_TYPE_UNSIGNED_SHORT: {
                UDA_LOG(UDA_LOG_DEBUG, "Type: UNSIGNED_SHORT\n");

                if (userdefinedtype->compoundfield[j].pointer) { // Pointer to Data array
                    if (xdrs->x_op == XDR_DECODE) {              // Allocate Heap for Data
                        rc = rc &&
                             xdr_int(xdrs,
                                     &count); // Count is known from the client's malloc log and passed by the sender
                        if (count > 0) {
                            rc = rc && xdr_int(xdrs,
                                               &rank); // Receive Shape of pointer arrays (not included in definition)
                            if (rank > 1) {
                                shape = (int*)malloc(rank * sizeof(int)); // freed via the malloc log registration
                                rc = rc && xdr_vector(xdrs, (char*)shape, rank, sizeof(int), (xdrproc_t)xdr_int);
                            } else {
                                shape = nullptr;
                            }
                            d = (char*)malloc(count * sizeof(unsigned short));
                            udaAddMalloc2(logmalloclist, (void*)d, count, sizeof(unsigned short), "unsigned short",
                                          rank, shape);
                            *p = (VOIDTYPE)d; // Save pointer: data will be written here
                        } else {
                            break;
                        }
                    } else {
                        d = (char*)*p; // data read from here
                        if (d == nullptr) {
                            count = 0;
                            rc = rc && xdr_int(xdrs, &count); // No data to send
                            break;
                        }
                        udaFindMalloc2(logmalloclist, (void*)p, &count, &size, &type, &rank,
                                       &shape); // Assume count of 0 means No Pointer data to send!

                        if (type != nullptr && STR_EQUALS(type, "unknown")) {
                            if (malloc_source == UDA_MALLOC_SOURCE_SOAP && j > 0 &&
                                STR_IEQUALS(userdefinedtype->compoundfield[j - 1].name, "__size") &&
                                STR_EQUALS(userdefinedtype->compoundfield[j].name,
                                           &userdefinedtype->compoundfield[j - 1].name[6])) {

                                count = (int)*prev; // the value of __size...
                                size = sizeof(unsigned short);
                                type = userdefinedtype->compoundfield[j].type;

                            } else {
                                if (count > 0) {
                                    int totalsize = count * size;
                                    int rcount = totalsize % sizeof(unsigned short);
                                    size = sizeof(unsigned short);
                                    count = totalsize / size;

                                    if (rcount != 0) {
                                        add_error(UDA_CODE_ERROR_TYPE, "xdrUserDefinedData", 999,
                                                  "Specified unsigned short malloc total size not integer multiple!");
                                        count = 0;
                                    }
                                }
                            }
                        }

                        rc = rc && xdr_int(xdrs, &count);

                        if ((count == 0 || size == 0) && *p != 0) {
                            add_error(UDA_CODE_ERROR_TYPE, "xdrUserDefinedData", 999,
                                      "Unsigned Short Data Heap Allocation not found in log!");
                            break;
                        }

                        rc = rc && xdr_int(xdrs, &rank); // Send Shape of arrays
                        if (rank > 1) {
                            rc = rc && xdr_vector(xdrs, (char*)shape, rank, sizeof(int), (xdrproc_t)xdr_int);
                        }
                    }
                    rc = rc && xdr_vector(xdrs, d, count, sizeof(unsigned short), (xdrproc_t)xdr_u_short);
                } else {
                    if (userdefinedtype->compoundfield[j].rank == 0) { // Element is a Scalar
                        rc = rc && xdr_u_short(xdrs, (unsigned short*)p);
                    } else { // Element is an Array of fixed size
                        rc = rc && xdr_vector(xdrs, (char*)p, userdefinedtype->compoundfield[j].count,
                                              sizeof(unsigned short), (xdrproc_t)xdr_u_short);
                    }
                }
                break;
            }

            case UDA_TYPE_INT: {
                UDA_LOG(UDA_LOG_DEBUG, "Type: INT\n");

                if (userdefinedtype->compoundfield[j].pointer) { // Pointer to Integer Data array
                    if (xdrs->x_op == XDR_DECODE) {              // Allocate Heap for Data
                        rc = rc &&
                             xdr_int(xdrs,
                                     &count); // Count is known from the client's malloc log and passed by the sender
                        if (count > 0) {
                            rc = rc && xdr_int(xdrs,
                                               &rank); // Receive Shape of pointer arrays (not included in definition)
                            if (rank > 1) {
                                shape = (int*)malloc(rank * sizeof(int)); // freed via the malloc log registration
                                rc = rc && xdr_vector(xdrs, (char*)shape, rank, sizeof(int), (xdrproc_t)xdr_int);
                            } else {
                                shape = nullptr;
                            }
                            d = (char*)malloc(count * sizeof(int));
                            udaAddMalloc2(logmalloclist, (void*)d, count, sizeof(int), "int", rank, shape);
                            *p = (VOIDTYPE)d; // Save pointer: data will be written here
                        } else {
                            break;
                        }
                    } else {
                        d = (char*)*p; // data read from here
                        if (d == nullptr) {
                            count = 0;
                            rc = rc && xdr_int(xdrs, &count); // No data to send
                            break;
                        }
                        udaFindMalloc2(logmalloclist, (void*)p, &count, &size, &type, &rank,
                                       &shape); // Assume count of 0 means No Pointer data to send!

                        if (type != nullptr && STR_EQUALS(type, "unknown")) {
                            if (malloc_source == UDA_MALLOC_SOURCE_SOAP && j > 0 &&
                                STR_IEQUALS(userdefinedtype->compoundfield[j - 1].name, "__size") &&
                                STR_EQUALS(userdefinedtype->compoundfield[j].name,
                                           &userdefinedtype->compoundfield[j - 1].name[6])) {

                                count = (int)*prev; // the value of __size...
                                size = sizeof(int);
                                type = userdefinedtype->compoundfield[j].type;

                            } else {
                                if (count > 0) {
                                    int totalsize = count * size;
                                    int rcount = totalsize % sizeof(int);
                                    size = sizeof(int);
                                    count = totalsize / size;

                                    if (rcount != 0) {
                                        add_error(UDA_CODE_ERROR_TYPE, "xdrUserDefinedData", 999,
                                                  "Specified malloc total size not integer multiple!");
                                        count = 0;
                                    }
                                }
                            }
                        }

                        rc = rc && xdr_int(xdrs, &count);

                        if ((count == 0 || size == 0) && *p != 0) {
                            add_error(UDA_CODE_ERROR_TYPE, "xdrUserDefinedData", 999,
                                      "Integer Data Heap Allocation not found in log!");
                            break;
                        }

                        rc = rc && xdr_int(xdrs, &rank); // Send Shape of arrays
                        if (rank > 1) {
                            rc = rc && xdr_vector(xdrs, (char*)shape, rank, sizeof(int), (xdrproc_t)xdr_int);
                        }
                    }
                    rc = rc && xdr_vector(xdrs, d, count, sizeof(int), (xdrproc_t)xdr_int);
                } else {
                    if (userdefinedtype->compoundfield[j].rank == 0) { // Element is a Scalar
                        rc = rc && xdr_int(xdrs, (int*)p);
                    } else { // Element is an Array of fixed size
                        rc = rc && xdr_vector(xdrs, (char*)p, userdefinedtype->compoundfield[j].count, sizeof(int),
                                              (xdrproc_t)xdr_int);
                    }
                }
                break;
            }

            case UDA_TYPE_UNSIGNED_INT: {
                UDA_LOG(UDA_LOG_DEBUG, "Type: UNSIGNED INT\n");

                if (userdefinedtype->compoundfield[j].pointer) { // Pointer to Data array
                    if (xdrs->x_op == XDR_DECODE) {              // Allocate Heap for Data
                        rc = rc &&
                             xdr_int(xdrs,
                                     &count); // Count is known from the client's malloc log and passed by the sender
                        if (count > 0) {
                            rc = rc && xdr_int(xdrs,
                                               &rank); // Receive Shape of pointer arrays (not included in definition)
                            if (rank > 1) {
                                shape = (int*)malloc(rank * sizeof(int)); // freed via the malloc log registration
                                rc = rc && xdr_vector(xdrs, (char*)shape, rank, sizeof(int), (xdrproc_t)xdr_int);
                            } else {
                                shape = nullptr;
                            }
                            d = (char*)malloc(count * sizeof(unsigned int));
                            udaAddMalloc2(logmalloclist, (void*)d, count, sizeof(unsigned int), "unsigned int", rank,
                                          shape);
                            *p = (VOIDTYPE)d; // Save pointer: data will be written here
                        } else {
                            break;
                        }
                    } else {
                        d = (char*)*p; // data read from here
                        if (d == nullptr) {
                            count = 0;
                            rc = rc && xdr_int(xdrs, &count); // No data to send
                            break;
                        }
                        udaFindMalloc2(logmalloclist, (void*)p, &count, &size, &type, &rank,
                                       &shape); // Assume count of 0 means No Pointer data to send!

                        if (type != nullptr && STR_EQUALS(type, "unknown")) {
                            if (malloc_source == UDA_MALLOC_SOURCE_SOAP && j > 0 &&
                                STR_IEQUALS(userdefinedtype->compoundfield[j - 1].name, "__size") &&
                                STR_EQUALS(userdefinedtype->compoundfield[j].name,
                                           &userdefinedtype->compoundfield[j - 1].name[6])) {

                                count = (int)*prev; // the value of __size...
                                size = sizeof(unsigned int);
                                type = userdefinedtype->compoundfield[j].type;
                            } else {
                                if (count > 0) {
                                    int totalsize = count * size;
                                    int rcount = totalsize % sizeof(unsigned int);
                                    size = sizeof(unsigned int);
                                    count = totalsize / size;

                                    if (rcount != 0) {
                                        add_error(UDA_CODE_ERROR_TYPE, "xdrUserDefinedData", 999,
                                                  "Specified malloc total size not integer multiple!");
                                        count = 0;
                                    }
                                }
                            }
                        }

                        rc = rc && xdr_int(xdrs, &count);
                        if ((count == 0 || size == 0) && *p != 0) {
                            add_error(UDA_CODE_ERROR_TYPE, "xdrUserDefinedData", 999,
                                      "Unsigned Integer Data Heap Allocation not found in log!");
                            break;
                        }
                        rc = rc && xdr_int(xdrs, &rank); // Send Shape of arrays
                        if (rank > 1) {
                            rc = rc && xdr_vector(xdrs, (char*)shape, rank, sizeof(int), (xdrproc_t)xdr_int);
                        }
                    }
                    rc = rc && xdr_vector(xdrs, d, count, sizeof(unsigned int), (xdrproc_t)xdr_u_int);
                } else {
                    if (userdefinedtype->compoundfield[j].rank == 0) { // Element is a Scalar
                        rc = rc && xdr_u_int(xdrs, (unsigned int*)p);
                    } else { // Element is an Array of fixed size
                        rc = rc && xdr_vector(xdrs, (char*)p, userdefinedtype->compoundfield[j].count,
                                              sizeof(unsigned int), (xdrproc_t)xdr_u_int);
                    }
                }
                break;
            }

            case UDA_TYPE_LONG64: {
                UDA_LOG(UDA_LOG_DEBUG, "Type: LONG LONG\n");

                if (userdefinedtype->compoundfield[j].pointer) { // Pointer to long long Data array
                    if (xdrs->x_op == XDR_DECODE) {              // Allocate Heap for Data
                        rc = rc &&
                             xdr_int(xdrs,
                                     &count); // Count is known from the client's malloc log and passed by the sender
                        if (count > 0) {
                            rc = rc && xdr_int(xdrs,
                                               &rank); // Receive Shape of pointer arrays (not included in definition)
                            if (rank > 1) {
                                shape = (int*)malloc(rank * sizeof(int)); // freed via the malloc log registration
                                rc = rc && xdr_vector(xdrs, (char*)shape, rank, sizeof(int), (xdrproc_t)xdr_int);
                            } else {
                                shape = nullptr;
                            }
                            d = (char*)malloc(count * sizeof(long long));
                            udaAddMalloc2(logmalloclist, (void*)d, count, sizeof(long long), "long long", rank, shape);
                            *p = (VOIDTYPE)d; // Save pointer: data will be written here
                        } else {
                            break;
                        }
                    } else {
                        d = (char*)*p; // data read from here
                        if (d == nullptr) {
                            count = 0;
                            rc = rc && xdr_int(xdrs, &count); // No data to send
                            break;
                        }
                        udaFindMalloc2(logmalloclist, (void*)p, &count, &size, &type, &rank,
                                       &shape); // Assume count of 0 means No Pointer data to send!

                        if (type != nullptr && STR_EQUALS(type, "unknown")) {
                            if (malloc_source == UDA_MALLOC_SOURCE_SOAP && j > 0 &&
                                STR_EQUALS(userdefinedtype->compoundfield[j - 1].name, "__size") &&
                                STR_EQUALS(userdefinedtype->compoundfield[j].name,
                                           &userdefinedtype->compoundfield[j - 1].name[6])) {

                                count = (int)*prev; // the value of __size...
                                size = sizeof(long long);
                                type = userdefinedtype->compoundfield[j].type;

                            } else {
                                if (count > 0) {
                                    int totalsize = count * size;
                                    int rcount = totalsize % sizeof(long long);
                                    size = sizeof(long long);
                                    count = totalsize / size;

                                    if (rcount != 0) {
                                        add_error(UDA_CODE_ERROR_TYPE, "xdrUserDefinedData", 999,
                                                  "Specified malloc total size not integer multiple!");
                                        count = 0;
                                    }
                                }
                            }
                        }

                        rc = rc && xdr_int(xdrs, &count);
                        if ((count == 0 || size == 0) && *p != 0) {
                            add_error(UDA_CODE_ERROR_TYPE, "xdrUserDefinedData", 999,
                                      "Long Long Data Heap Allocation not found in log!");
                            break;
                        }
                        rc = rc && xdr_int(xdrs, &rank); // Send Shape of arrays
                        if (rank > 1) {
                            rc = rc && xdr_vector(xdrs, (char*)shape, rank, sizeof(int), (xdrproc_t)xdr_int);
                        }
                    }
                    rc = rc && xdr_vector(xdrs, d, count, sizeof(long long), (xdrproc_t)xdr_int64_t);
                } else {
                    if (userdefinedtype->compoundfield[j].rank == 0) { // Element is a Scalar
                        rc = rc && xdr_int64_t(xdrs, (int64_t*)p);
                    } else { // Element is an Array of fixed size
                        rc = rc && xdr_vector(xdrs, (char*)p, userdefinedtype->compoundfield[j].count,
                                              sizeof(long long), (xdrproc_t)xdr_int64_t);
                    }
                }
                break;
            }

            case UDA_TYPE_UNSIGNED_LONG64: {
                UDA_LOG(UDA_LOG_DEBUG, "Type: UNSIGNED LONG LONG\n");

                if (userdefinedtype->compoundfield[j].pointer) { // Pointer to Data array
                    if (xdrs->x_op == XDR_DECODE) {              // Allocate Heap for Data
                        rc = rc &&
                             xdr_int(xdrs,
                                     &count); // Count is known from the client's malloc log and passed by the sender
                        if (count > 0) {
                            rc = rc && xdr_int(xdrs,
                                               &rank); // Receive Shape of pointer arrays (not included in definition)
                            if (rank > 1) {
                                shape = (int*)malloc(rank * sizeof(int)); // freed via the malloc log registration
                                rc = rc && xdr_vector(xdrs, (char*)shape, rank, sizeof(int), (xdrproc_t)xdr_int);
                            } else {
                                shape = nullptr;
                            }
                            d = (char*)malloc(count * sizeof(unsigned long long));
                            udaAddMalloc2(logmalloclist, (void*)d, count, sizeof(unsigned long long),
                                          "unsigned long long", rank, shape);
                            *p = (VOIDTYPE)d; // Save pointer: data will be written here
                        } else {
                            break;
                        }
                    } else {
                        d = (char*)*p; // data read from here
                        if (d == nullptr) {
                            count = 0;
                            rc = rc && xdr_int(xdrs, &count); // No data to send
                            break;
                        }
                        udaFindMalloc2(logmalloclist, (void*)p, &count, &size, &type, &rank,
                                       &shape); // Assume count of 0 means No Pointer data to send!

                        if (type != nullptr && STR_EQUALS(type, "unknown")) {
                            if (malloc_source == UDA_MALLOC_SOURCE_SOAP && j > 0 &&
                                STR_EQUALS(userdefinedtype->compoundfield[j - 1].name, "__size") &&
                                STR_EQUALS(userdefinedtype->compoundfield[j].name,
                                           &userdefinedtype->compoundfield[j - 1].name[6])) {

                                count = (int)*prev; // the value of __size...
                                size = sizeof(unsigned long long);
                                type = userdefinedtype->compoundfield[j].type;

                            } else {
                                if (count > 0) {
                                    int totalsize = count * size;
                                    int rcount = totalsize % sizeof(unsigned long long);
                                    size = sizeof(unsigned long long);
                                    count = totalsize / size;

                                    if (rcount != 0) {
                                        add_error(UDA_CODE_ERROR_TYPE, "xdrUserDefinedData", 999,
                                                  "Specified malloc total size not integer multiple!");
                                        count = 0;
                                    }
                                }
                            }
                        }

                        rc = rc && xdr_int(xdrs, &count);
                        if ((count == 0 || size == 0) && *p != 0) {
                            add_error(UDA_CODE_ERROR_TYPE, "xdrUserDefinedData", 999,
                                      "Unsigned Long Long Data Heap Allocation not found in log!");
                            break;
                        }
                        rc = rc && xdr_int(xdrs, &rank); // Send Shape of arrays
                        if (rank > 1) {
                            rc = rc && xdr_vector(xdrs, (char*)shape, rank, sizeof(int), (xdrproc_t)xdr_int);
                        }
                    }
                    rc = rc && xdr_vector(xdrs, d, count, sizeof(unsigned long long), (xdrproc_t)xdr_uint64_t);
                } else {
                    if (userdefinedtype->compoundfield[j].rank == 0) { // Element is a Scalar
                        rc = rc && xdr_uint64_t(xdrs, (uint64_t*)p);
                    } else {
                        // Element is an Array of fixed size
                        rc = rc && xdr_vector(xdrs, (char*)p, userdefinedtype->compoundfield[j].count,
                                              sizeof(unsigned long long), (xdrproc_t)xdr_uint64_t);
                    }
                }
                break;
            }

            case UDA_TYPE_CHAR: {
                UDA_LOG(UDA_LOG_DEBUG, "Type: CHAR\n");

                if (userdefinedtype->compoundfield[j].pointer) { // Pointer to Float Data array
                    if (xdrs->x_op == XDR_DECODE) {              // Allocate Heap for Data
                        rc = rc &&
                             xdr_int(xdrs,
                                     &count); // Count is known from the client's malloc log and passed by the sender
                        if (!rc) {
                            break;
                        }
                        if (count > 0) {
                            rc = rc && xdr_int(xdrs,
                                               &rank); // Receive Shape of pointer arrays (not included in definition)
                            if (rank > 1) {
                                shape = (int*)malloc(rank * sizeof(int)); // freed via the malloc log registration
                                rc = rc && xdr_vector(xdrs, (char*)shape, rank, sizeof(int), (xdrproc_t)xdr_int);
                            } else {
                                shape = nullptr;
                            }
                            d = (char*)malloc(count * sizeof(char));
                            udaAddMalloc2(logmalloclist, (void*)d, count, sizeof(char), "char", rank, shape);
                            *p = (VOIDTYPE)d; // Save pointer: data will be written here
                        } else {
                            break;
                        }
                    } else {
                        d = (char*)*p;
                        if (d == nullptr) {
                            count = 0;
                            rc = rc && xdr_int(xdrs, &count); // No data to send
                            break;
                        }
                        udaFindMalloc2(logmalloclist, (void*)p, &count, &size, &type, &rank,
                                       &shape); // Assume count of 0 means No Pointer data to send!

                        if (type != nullptr && STR_EQUALS(type, "unknown")) {
                            if (malloc_source == UDA_MALLOC_SOURCE_SOAP && j > 0 &&
                                STR_EQUALS(userdefinedtype->compoundfield[j - 1].name, "__size") &&
                                STR_EQUALS(userdefinedtype->compoundfield[j].name,
                                           &userdefinedtype->compoundfield[j - 1].name[6])) {

                                isSOAP = 1;
                                count = (int)*prev; // the value of __size...
                                size = sizeof(char);
                                type = userdefinedtype->compoundfield[j].type;
                            } else {
                                if (count > 0) {
                                    int totalsize = count * size;
                                    int rcount = totalsize % sizeof(char);
                                    size = sizeof(char);
                                    count = totalsize / size;

                                    if (rcount != 0) {
                                        add_error(UDA_CODE_ERROR_TYPE, "xdrUserDefinedData", 999,
                                                  "Specified malloc total size not integer multiple!");
                                        count = 0;
                                    }
                                }
                            }
                        } else {
                            if (malloc_source == UDA_MALLOC_SOURCE_SOAP && (count == 0 || size == 0) &&
                                *p != 0) { // Assume SOAP string
                                isSOAP = 1;
                                if (d != nullptr) {
                                    int lstr = (int)strlen(d);
                                    if (lstr < MAXSOAPSTACKSTRING) {
                                        count = lstr + 1;
                                        size = sizeof(char);
                                        type = userdefinedtype->compoundfield[j].type;
                                    }
                                    convertNonPrintable2(d); // Remove obvious garbage (bug - non initialised...?)
                                }
                            }
                        }

                        rc = rc && xdr_int(xdrs, &count);
                        if (!rc) {
                            break;
                        }

                        if ((count == 0 || size == 0) && *p != 0) {
                            add_error(UDA_CODE_ERROR_TYPE, "xdrUserDefinedData", 999,
                                      "Char Data Heap Allocation not found in log!");
                            break;
                        }
                        rc = rc && xdr_int(xdrs, &rank); // Send Shape of arrays
                        if (rank > 1) {
                            rc = rc && xdr_vector(xdrs, (char*)shape, rank, sizeof(int), (xdrproc_t)xdr_int);
                        }
                    }

                    rc = rc && xdr_int(xdrs, &isSOAP); // synchronise XDR function calls
                    if (!rc) {
                        break;
                    }

                    if (isSOAP) { // char* is a C String in gSOAP
                        if (xdrs->x_op == XDR_ENCODE) {
                            int sl = (int)strlen(d);
                            if (sl > count) {
                                d[count - 1] = '\0'; // Terminate
                            }
                        }
                        rc = rc && WrapXDRString(xdrs, d, count);
                        isSOAP = 0;
                    } else {
                        rc = rc && xdr_vector(xdrs, d, count, sizeof(char), (xdrproc_t)xdr_char);
                    }
                    if (!rc) {
                        break;
                    }

                } else {
                    if (userdefinedtype->compoundfield[j].rank == 0) { // Element is a Scalar
                        rc = rc && xdr_char(xdrs, (char*)p);
                    } else { // Element is an Array of fixed size
                        rc = rc && xdr_vector(xdrs, (char*)p, userdefinedtype->compoundfield[j].count, sizeof(char),
                                              (xdrproc_t)xdr_char);
                    }
                    if (!rc) {
                        break;
                    }
                }
                break;
            }

            case UDA_TYPE_STRING2: {                             // Array of char terminated by \0
                if (userdefinedtype->compoundfield[j].pointer) { // Pointer to string array
                    if (xdrs->x_op == XDR_DECODE) {              // Allocate Heap for Data
                        rc = rc &&
                             xdr_int(xdrs,
                                     &count); // Count is known from the client's malloc log and passed by the sender
                        if (count > 0) {
                            d = (char*)malloc(count * sizeof(char));
                            udaAddMalloc(logmalloclist, (void*)d, count, sizeof(char), "STRING");
                            *p = (VOIDTYPE)d; // Save pointer: data will be written here
                        } else {
                            break;
                        }
                    } else {
                        d = (char*)*p;
                        if (d == nullptr) {
                            count = 0;
                            rc = rc && xdr_int(xdrs, &count); // No data to send
                            break;
                        }

                        if (malloc_source == UDA_MALLOC_SOURCE_DOM) { // Bad address range?
                            count = 0;
                            rc = rc && xdr_int(xdrs, &count); // No data to send
                            break;
                        }

                        udaFindMalloc(logmalloclist, (void*)p, &count, &size,
                                      &type); // Assume 0 means No Pointer data to send!

                        if (malloc_source == UDA_MALLOC_SOURCE_DOM && (count == 0 || size == 0) && d != nullptr) {
                            int lstr = (int)strlen(d);
                            if (lstr < MAXSOAPSTACKSTRING) {
                                count = lstr + 1;
                                size = sizeof(char);
                                type = userdefinedtype->compoundfield[j].type;
                            }
                        }

                        if (count == 1 && STR_EQUALS(type, "unknown")) {
                            int lstr = (int)strlen(d);
                            count = size;
                            size = sizeof(char);
                            if (malloc_source == UDA_MALLOC_SOURCE_DOM && lstr > count - 1 &&
                                lstr < MAXSOAPSTACKSTRING) {
                                count = lstr + 1; // A bug in xml DOM?
                            }
                        }

                        rc = rc && xdr_int(xdrs, &count);

                        if ((count == 0 || size == 0) && *p != 0) {
                            add_error(UDA_CODE_ERROR_TYPE, "xdrUserDefinedData", 999,
                                      "String Data Heap Allocation not found in log!");
                            break;
                        }
                    }

                    if (count > 0) {
                        rc = rc && WrapXDRString(xdrs, d, count);
                    }

                } else {
                    if (userdefinedtype->compoundfield[j].rank == 1) { // Element is a Regular String
                        rc = rc && WrapXDRString(xdrs, (char*)p, userdefinedtype->compoundfield[j].count);
                    }
                }
                break;
            }

                // String handling is complicated ...

                // Strings with a Fixed Length:        Rank >= 1, Pointer = FALSE    (Convention)
                // Strings with a Non-Fixed Length: Rank  = 0, Pointer = TRUE
                // If the number of strings is arbitrary then the type is STRING* rather than STRING

                // Strings:
                //    char *p                 single scalar string of arbitrary length                 => rank = 0,
                //    pointer = 1, type STRING char **p;            arbitrary number array of strings of arbitrary
                //    length     => rank = 0, pointer = 1, type STRING* char p[int]            single scalar string of
                //    fixed length                     => rank = 1, pointer = 0, type STRING char *p[int]        fixed
                //    number array of strings of arbitrary length         => rank = 1, pointer = 0, type STRING* char
                //    p[int][int]    fixed number array of strings of fixed length             => rank = 2, pointer = 0,
                //    type STRING

            case UDA_TYPE_STRING: { // Array of char terminated by \0
                UDA_LOG(UDA_LOG_DEBUG, "Type: STRING\n");

                char** strarr;
                int nstr = 0, istr;
                if (userdefinedtype->compoundfield[j].pointer) { // Pointer to string array
                    if (xdrs->x_op == XDR_DECODE) {              // Allocate Heap for Data

                        if (STR_EQUALS(userdefinedtype->compoundfield[j].type, "STRING *")) {
                            rc = rc && xdr_int(xdrs, &nstr); // Number of strings
                            if (nstr > 0) {
                                char** str = (char**)malloc(nstr * sizeof(char*));
                                udaAddMalloc(logmalloclist, (void*)str, nstr, sizeof(char*), "STRING *");
                                for (istr = 0; istr < nstr; istr++) {
                                    rc = rc && xdr_int(xdrs, &count);
                                    if (count > 0) {
                                        d = (char*)malloc(count * sizeof(char));
                                        udaAddMalloc(logmalloclist, (void*)d, count, sizeof(char), "char");
                                        rc = rc && WrapXDRString(xdrs, d, count);
                                        str[istr] = d;
                                    }
                                }
                                *p = (VOIDTYPE)str;
                            }
                            break;
                        }

                        rc = rc &&
                             xdr_int(
                                 xdrs,
                                 &nstr); // nstr (count) is known from the client's malloc log and passed by the sender
                        if (nstr > 0) {
                            rc = rc && xdr_int(xdrs,
                                               &rank); // Receive Shape of pointer arrays (not included in definition)
                            if (rank > 1) {
                                shape = (int*)malloc(rank * sizeof(int)); // freed via the malloc log registration
                                rc = rc && xdr_vector(xdrs, (char*)shape, rank, sizeof(int), (xdrproc_t)xdr_int);
                            } else {
                                shape = nullptr;
                            }
                            strarr = (char**)malloc(nstr *
                                                    sizeof(char*)); // nstr is the length of the array, not the strings
                            udaAddMalloc2(logmalloclist, (void*)strarr, nstr, sizeof(char*), "STRING", rank, shape);
                            *p = (VOIDTYPE)strarr;                // Save pointer: First String will be written here
                            for (istr = 0; istr < nstr; istr++) { // Receive individual String lengths, then the string
                                rc = rc && xdr_int(xdrs, &count);
                                strarr[istr] = (char*)malloc(count * sizeof(char));
                                udaAddMalloc(logmalloclist, (void*)strarr[istr], count, sizeof(char), "STRING");
                                rc = rc && WrapXDRString(xdrs, strarr[istr], count);
                                if (rank == 0 && nstr == 1) {
                                    *p = (VOIDTYPE)strarr[0];
                                }
                            }
                        } else {
                            break;
                        }
                    } else {

                        if (STR_EQUALS(userdefinedtype->compoundfield[j].type, "STRING *")) {
                            char** str = (char**)*p;
                            udaFindMalloc(logmalloclist, (void*)&str, &nstr, &size, &type);
                            rc = rc && xdr_int(xdrs, &nstr); // Number of strings
                            if (nstr > 0) {
                                for (istr = 0; istr < nstr; istr++) {
                                    udaFindMalloc(logmalloclist, (void*)&str[istr], &count, &size, &type);
                                    rc = rc && xdr_int(xdrs, &count);
                                    if (count > 0) {
                                        d = str[istr];
                                        rc = rc && WrapXDRString(xdrs, d, count);
                                    }
                                }
                            }
                            break;
                        }

                        d = (char*)*p; // First string in the Array
                        if (d == nullptr) {
                            count = 0;
                            rc = rc && xdr_int(xdrs, &count); // No data to send
                            break;
                        }

                        udaFindMalloc2(logmalloclist, (void*)p, &nstr, &size, &type, &rank,
                                       &shape); // Assume 0 means No Pointer data to send!
                        // or heap allocated in external library!

                        // Is this a fixed length array of strings ?

                        if (nstr == 0 && userdefinedtype->compoundfield[j].rank == 1 &&
                            userdefinedtype->compoundfield[j].shape[0] > 0) {
                            rank = 1;
                            nstr = userdefinedtype->compoundfield[j].shape[0];
                            shape = userdefinedtype->compoundfield[j].shape;
                            size = userdefinedtype->compoundfield[j].size;
                        }

                        rc = rc && xdr_int(xdrs, &nstr); // This many strings to send

                        if ((nstr == 0 || size == 0) && *p != 0) {
                            add_error(UDA_CODE_ERROR_TYPE, "xdrUserDefinedData", 999,
                                      "String Array Data Heap Allocation not found in log!");
                            break;
                        }

                        rc = rc && xdr_int(xdrs, &rank); // Send Shape of arrays
                        if (rank > 1) {
                            rc = rc && xdr_vector(xdrs, (char*)shape, rank, sizeof(int), (xdrproc_t)xdr_int);
                        }

                        if (rank == 0) {
                            rc = rc && xdr_int(xdrs, &size); // This length of string
                            rc = rc && WrapXDRString(xdrs, d, size);
                        } else {
                            strarr = (char**)d;
                            for (istr = 0; istr < nstr;
                                 istr++) { // Send individual String lengths, then the string itself
                                count = (int)strlen(strarr[istr]) + 1;
                                rc = rc && xdr_int(xdrs, &count);
                                rc = rc && WrapXDRString(xdrs, strarr[istr], count);
                            }
                        }
                    }
                } else {
                    if (userdefinedtype->compoundfield[j].rank >= 1) { // fixed length Regular Strings
                        if (userdefinedtype->compoundfield[j].rank == 1 &&
                            strcmp(userdefinedtype->compoundfield[j].type, "STRING *") !=
                                0) { // Element is a Single String
                            rc = rc && WrapXDRString(xdrs, (char*)p, userdefinedtype->compoundfield[j].count);
                        } else { // Element is a String Array: Treat as Rank 1 array
                            if (userdefinedtype->compoundfield[j].rank == 1 &&
                                STR_EQUALS(userdefinedtype->compoundfield[j].type, "STRING *")) {
                                char** str = (char**)p;
                                nstr = userdefinedtype->compoundfield[j].count; // Number of strings
                                for (istr = 0; istr < nstr; istr++) {
                                    if (xdrs->x_op == XDR_DECODE) {
                                        rc = rc && xdr_int(xdrs, &count); // Arbitrary String length
                                        if (count > 0) {
                                            d = (char*)malloc(count * sizeof(char));
                                            udaAddMalloc(logmalloclist, (void*)d, count, sizeof(char), "STRING");
                                            rc = rc && WrapXDRString(xdrs, d, count);
                                            str[istr] = d;
                                        } // Save pointer: data will be written here
                                    } else {
                                        udaFindMalloc(logmalloclist, (void*)&str[istr], &count, &size, &type);
                                        rc = rc && xdr_int(xdrs, &count);
                                        if (count > 0) {
                                            d = (char*)str[istr];
                                            rc = rc && WrapXDRString(xdrs, d, count);
                                        }
                                    }
                                }
                            } else {
                                char* str = (char*)p;
                                int stride = userdefinedtype->compoundfield[j].shape[0]; // String length
                                nstr = 1;
                                for (istr = 1; istr < userdefinedtype->compoundfield[j].rank; istr++) {
                                    nstr = nstr * userdefinedtype->compoundfield[j]
                                                      .shape[istr]; // Number of strings to send/receive
                                }
                                for (istr = 0; istr < nstr; istr++) { // send/receive individual strings
                                    rc = rc && WrapXDRString(xdrs, &str[istr * stride],
                                                             userdefinedtype->compoundfield[j].count);
                                }
                            }
                        }
                    } else {                            // Element is a Single String of any size
                        if (xdrs->x_op == XDR_DECODE) { // Allocate Heap for Data
                            rc =
                                rc &&
                                xdr_int(xdrs,
                                        &count); // Count is known from the client's malloc log and passed by the sender
                            if (count > 0) {
                                d = (char*)malloc(count * sizeof(char));
                                udaAddMalloc(logmalloclist, (void*)d, count, sizeof(char), "STRING");
                                *p = (VOIDTYPE)d; // Save pointer: data will be written here
                            } else {
                                break;
                            }
                        } else {
                            d = (char*)*p;
                            if (d == nullptr) {
                                count = 0;
                                rc = rc && xdr_int(xdrs, &count); // No data to send
                                break;
                            }
                            udaFindMalloc(logmalloclist, (void*)p, &count, &size,
                                          &type);                            // Assume 0 means No string to send!
                            if (count == 1 && STR_EQUALS(type, "unknown")) { // ***** Fix for SOAP sources incomplete!
                                count = size;
                                size = sizeof(char);
                            }
                            rc = rc && xdr_int(xdrs, &count);
                            if ((count == 0 || size == 0) && *p != 0) {
                                add_error(UDA_CODE_ERROR_TYPE, "xdrUserDefinedData", 999,
                                          "String Data Heap Allocation not found in log!");
                                break;
                            }
                        }
                        rc = rc && WrapXDRString(xdrs, d, count);
                    }
                }
                break;
            }

                // Atomic Type or User Defined Type => Recursive Send/Receive
                // Scalar or Array of fixed count within a structure
                // Pointer to Scalar or Array
                // For fixed count types, the count is given by the structure definition
                // For pointer types, the count is given by the malloc log

            default: {

                // Send or Receive the Count, Size and Type of the sub-structure (All atomic types except void are
                // trapped before this point)

                UDA_LOG(UDA_LOG_DEBUG, "Type: OTHER - Void Type or Structure\n");

                if (userdefinedtype->compoundfield[j].pointer) {
                    if (xdrs->x_op != XDR_DECODE) {
                        udaFindMalloc2(logmalloclist, (void*)p, &count, &size, &type, &structRank, &structShape);

                        // Interpret an 'unknown' void data type using knowledge of the gSOAP or DOM systems

                        if (type != nullptr && STR_EQUALS(type, "unknown")) { // arises from a malloc redirection
                            if (malloc_source == UDA_MALLOC_SOURCE_SOAP && j > 0 &&
                                STR_EQUALS(userdefinedtype->compoundfield[j - 1].name, "__size") &&
                                STR_EQUALS(userdefinedtype->compoundfield[j].name,
                                           &userdefinedtype->compoundfield[j - 1].name[6])) {

                                count = (int)*prev; // the value of __size...
                                size = (int)udaGetsizeof(userdefinedtypelist, userdefinedtype->compoundfield[j].type);
                                type = userdefinedtype->compoundfield[j].type;
                            } else {
                                if (count > 0) {
                                    int totalsize, ssize;
                                    totalsize = count * size;
                                    if (malloc_source == UDA_MALLOC_SOURCE_DOM &&
                                        STR_EQUALS(userdefinedtype->compoundfield[j].type, "void")) {
                                        ssize = sizeof(char); // Assume xml void pointer type is to char
                                        type = chartype;
                                    } else {
                                        ssize = (int)udaGetsizeof(userdefinedtypelist,
                                                                  userdefinedtype->compoundfield[j].type);
                                        type = userdefinedtype->compoundfield[j].type;
                                    }

                                    if (ssize > 0) {
                                        size = ssize;
                                        count = totalsize / size;
                                    } else {
                                        add_error(UDA_CODE_ERROR_TYPE, "xdrUserDefinedData", 999,
                                                  "Specified malloc total size not integer multiple!");
                                        count = 0;
                                    }
                                }
                            }
                        }

                        if ((count == 0 || size == 0) && *p != 0) {
                            add_error(UDA_CODE_ERROR_TYPE, "xdrUserDefinedData", 999,
                                      "Data Heap Allocation not found in log!");
                            break;
                        }
                    }

                    rc = rc && xdr_int(xdrs, &count); // Not passed with the Structure definition

                    if (count == 0) {
                        break; // Nothing to Send or Receive
                    }

                    loopcount = count; // Array Count

                    rc = rc && xdr_int(xdrs, &size); // Structure Size

                    if (xdrs->x_op == XDR_DECODE) {
                        rc = rc && WrapXDRString(xdrs, (char*)rudtype, MAXELEMENTNAME - 1);
                        type = rudtype;
                    } else {
                        rc = rc && WrapXDRString(xdrs, (char*)type, MAXELEMENTNAME - 1);
                    }

                    if (protocolVersion >= 7) {
                        rc = rc && xdr_int(xdrs, &structRank); // Rank of the Structure Array
                        if (structRank > 1) {
                            if (xdrs->x_op == XDR_DECODE) {
                                structShape = (int*)malloc(structRank * sizeof(int));
                            }
                            rc =
                                rc && xdr_vector(xdrs, (char*)structShape, structRank, sizeof(int), (xdrproc_t)xdr_int);
                        } else {
                            structShape = nullptr;
                        }
                    }

                    UDA_LOG(UDA_LOG_DEBUG, "Pointer: Send or Receive Count: %d, Size: %d, Type: %s\n", count, size,
                            type);

                } else {

                    // Non Pointer types: Heap already allocated (pass 0 count to xdrUserDefinedData)
                    // Size and Type also known. Type cannot be 'void'.

                    udaAddNonMalloc(logmalloclist, (void*)p, userdefinedtype->compoundfield[j].count,
                                    userdefinedtype->compoundfield[j].size, userdefinedtype->compoundfield[j].type);

                    loopcount = userdefinedtype->compoundfield[j].count;
                    count = 0;
                    type = userdefinedtype->compoundfield[j].type;

                    UDA_LOG(UDA_LOG_DEBUG, "Pointer: Send or Receive Count: %d, Size: %d, Type: %s\n",
                            userdefinedtype->compoundfield[j].count, userdefinedtype->compoundfield[j].size,
                            userdefinedtype->compoundfield[j].type);
                }

                // Pointer to structure definition (void type ignored)

                if ((utype = udaFindUserDefinedType(userdefinedtypelist, type, 0)) == nullptr &&
                    strcmp(userdefinedtype->compoundfield[j].type, "void") != 0) {

                    UDA_LOG(UDA_LOG_DEBUG, "**** Error #1: User Defined Type %s not known!\n",
                            userdefinedtype->compoundfield[j].type);
                    UDA_LOG(UDA_LOG_DEBUG, "structure Name: %s\n", userdefinedtype->name);
                    UDA_LOG(UDA_LOG_DEBUG, "Element Type  : %s\n", userdefinedtype->compoundfield[j].type);
                    UDA_LOG(UDA_LOG_DEBUG, "        Offset: %d\n", userdefinedtype->compoundfield[j].offset);
                    UDA_LOG(UDA_LOG_DEBUG, "        Count : %d\n", userdefinedtype->compoundfield[j].count);
                    UDA_LOG(UDA_LOG_DEBUG, "        Size  : %d\n", userdefinedtype->compoundfield[j].size);

                    break;
                }

                // Must be a known User Defined Type
                // Execute once for each structure array element: count comes either from the malloc log or from the
                // structure definition for non-pointer types A new tree node is used for each array element when
                // receiving

                // When passing linked lists, the parent->child->parent link needs to be detected and blocked when
                // sending. On receiving data, the pointer references can be added back.

                if (utype != nullptr) {

                    for (int i = 0; i < loopcount; i++) {

                        // Has this structure already been sent/received (e.g. in a linked list)

                        id = 0;

                        if (0 && malloc_source == UDA_MALLOC_SOURCE_DOM && xdrs->x_op == XDR_ENCODE &&
                            userdefinedtype->compoundfield[j].pointer) {
                            char* stype;
                            void* heap;
                            heap = p;
                            id = udaFindStructId(heap, &stype, log_struct_list);
                        }

                        if (id == 0) { // Only send/receive new structures
                            if (userdefinedtype->compoundfield[j].pointer) {
                                rc = rc &&
                                     xdrUserDefinedData(
                                         xdrs, logmalloclist, log_struct_list, userdefinedtypelist, utype, (void**)p,
                                         count, structRank, structShape, i, &subNTree, protocolVersion,
                                         malloc_source); // User Defined type // rc set to 0 somewhere => stops call
                            } else {
                                rc = rc &&
                                     xdrUserDefinedData(xdrs, logmalloclist, log_struct_list, userdefinedtypelist,
                                                        utype, (void**)&p, count, structRank, structShape, i, &subNTree,
                                                        protocolVersion,
                                                        malloc_source); // if rc is set to 0 somewhere => stops call
                            }

                            // Add the new data branch to the tree
                            // If this is the first pass, allocate all loopcount child nodes
                            // dgm 15Nov2011: pre-allocate to avoid performance degradation when tree becomes large

                            if (xdrs->x_op == XDR_DECODE && subNTree != nullptr) {
                                strcpy(subNTree->name, userdefinedtype->compoundfield[j].name);
                                if (i == 0 && loopcount > 0) {
                                    if (newNTree->children == nullptr && newNTree->branches == 0) {
                                        newNTree->children =
                                            (NTREE**)malloc(loopcount * sizeof(NTREE*)); // Allocate the node array
                                        udaAddMalloc(logmalloclist, (void*)newNTree->children, loopcount,
                                                     sizeof(NTREE*), "NTREE *");
                                    } else { // Multiple branches (user types) originating in the same node
                                        auto old = (VOIDTYPE)newNTree->children;
                                        newNTree->children = (NTREE**)realloc(
                                            (void*)old,
                                            (newNTree->branches + loopcount) *
                                                sizeof(NTREE*)); // Individual node addresses remain valid
                                        udaChangeMalloc(logmalloclist, old, (void*)newNTree->children,
                                                        newNTree->branches + loopcount, sizeof(NTREE*), "NTREE *");
                                    }
                                }
                                udaAddNTree(newNTree, subNTree); // Only first call creates new tree node
                            }
                        }
                    }
                    break;

                } else {

                    // Must be a voided atomic type

                    if (udaGettypeof(type) != UDA_TYPE_UNKNOWN) {
                        char* z = (char*)*p;
                        rc = rc && xdrAtomicData(logmalloclist, xdrs, type, count, size, &z); // Must be an Atomic Type
                        *p = (VOIDTYPE)z;
                        break;
                    } else {
                        add_error(UDA_CODE_ERROR_TYPE, "xdrUserDefinedData", 999, "User Defined Type not known!");
                        break;
                    }
                }

                add_error(UDA_CODE_ERROR_TYPE, "xdrUserDefinedData", 999, "Type not known!");
                break;
            }
        }

        if (!rc) {
            add_error(UDA_CODE_ERROR_TYPE, "xdrUserDefinedData", 999, "XDR Return Code False => Bad send/receive!");
            recursiveDepthPut--;
            return rc;
        }

        prev = p; // Preserve the previous data pointer
    }

    recursiveDepthPut--;
    return rc;
}

// Send/Receive Array of Structures

int udaXDRUserDefinedTypeDataPut(XDR* xdrs, LOGMALLOCLIST* logmalloclist, USERDEFINEDTYPELIST* userdefinedtypelist,
                                 USERDEFINEDTYPE* userdefinedtype, void** data, int protocolVersion,
                                 LOGSTRUCTLIST* log_struct_list, int malloc_source)
{
    int rc = 1;

    initLogStructList(log_struct_list); // Initialise Linked List Structure Log

    if (xdrs->x_op == XDR_DECODE) {

        NTREE* dataNTree = nullptr;

        rc = rc && xdr_userdefinedtype(xdrs, userdefinedtypelist,
                                       userdefinedtype); // User Defined Type Definitions

        rc = rc &&
             xdrUserDefinedDataPut(xdrs, logmalloclist, log_struct_list, userdefinedtypelist, userdefinedtype, data, 1,
                                   0, nullptr, 0, &dataNTree, protocolVersion, malloc_source); // Data within Structures

        udaSetFullNTree(dataNTree); // Copy to Global
    } else {

        if (userdefinedtype == nullptr) {
            add_error(UDA_CODE_ERROR_TYPE, "udaXDRUserDefinedTypeData", 999,
                      "No User Defined Type passed - cannot send!");
            return 0;
        }

        rc = xdr_userdefinedtype(xdrs, userdefinedtypelist,
                                 userdefinedtype); // User Defined Type Definitions

        rc = rc &&
             xdrUserDefinedDataPut(xdrs, logmalloclist, log_struct_list, userdefinedtypelist, userdefinedtype, data, 1,
                                   0, nullptr, 0, nullptr, protocolVersion, malloc_source); // Data within Structures
        /*
              if(!XDRstdioFlag) rc = rc && xdrrec_endofrecord(xdrs, 1);
        */
    }

    udaFreeLogStructList(log_struct_list); // Free Linked List Structure Log heap

    return rc;
}

bool_t uda::client_server::xdr_userdefinedtypelistPut(XDR* xdrs, USERDEFINEDTYPELIST* str)
{

    // Send/Receive the list of userdefined types

    int rc = 1;

    rc = rc && xdr_int(xdrs, &str->listCount);

    UDA_LOG(UDA_LOG_DEBUG, "xdr_userdefinedtypelist: rc = %d, listCount = %d\n", rc, str->listCount);

    if (!rc || str->listCount == 0) {
        return rc;
    }

    if (xdrs->x_op == XDR_DECODE) { // Receiving array so allocate Heap for it then initialise
        str->userdefinedtype = (USERDEFINEDTYPE*)malloc(str->listCount * sizeof(USERDEFINEDTYPE));
        for (int i = 0; i < str->listCount; i++) {
            initUserDefinedType(&str->userdefinedtype[i]);
        }
    }

    for (int i = 0; i < str->listCount; i++) {
        rc = rc && xdr_userdefinedtype(xdrs, str, &str->userdefinedtype[i]);
    }

    return rc;
}

int uda::client_server::protocolXML2Put(XDR* xdrs, int protocol_id, int direction, int* token,
                                        LOGMALLOCLIST* logmalloclist, USERDEFINEDTYPELIST* userdefinedtypelist,
                                        void* str, int protocolVersion, LOGSTRUCTLIST* log_struct_list,
                                        unsigned int private_flags, int malloc_source)
{
    DATA_BLOCK* data_block;

    int rc = 1;
    int err = 0;

    //----------------------------------------------------------------------------
    // Error Management Loop

    do {

        //----------------------------------------------------------------------------
        // Generalised User Defined Data Structures

        if (protocol_id == UDA_PROTOCOL_STRUCTURES) {

            void* data = nullptr;
            data_block = (DATA_BLOCK*)str;

            if (data_block->opaque_type == UDA_OPAQUE_TYPE_STRUCTURES) {
                int packageType = 0;

                if (xdrs->x_op == XDR_ENCODE) { // Send Data

                    SARRAY sarray; // Structure array carrier structure
                    SARRAY* psarray = &sarray;
                    int shape = data_block->data_n;                        // rank 1 array of dimension lengths
                    auto udt = (USERDEFINEDTYPE*)data_block->opaque_block; // The data's structure definition
                    USERDEFINEDTYPE* u = udaFindUserDefinedType(userdefinedtypelist, "SARRAY",
                                                                0); // Locate the carrier structure definition

                    if (udt == nullptr || u == nullptr) {
                        err = 999;
                        UDA_LOG(UDA_LOG_DEBUG, "nullptr SARRAY User defined data Structure Definition\n");
                        printUserDefinedTypeListTable(*userdefinedtypelist);
                        add_error(UDA_CODE_ERROR_TYPE, "protocolXML2Put", err,
                                  "nullptr User defined data Structure Definition");
                        break;
                    }

                    UDA_LOG(UDA_LOG_DEBUG, "Creating SARRAY carrier structure to Send\n");

                    initSArray(&sarray);
                    sarray.count = data_block->data_n;     // Number of this structure
                    sarray.rank = 1;                       // Array Data Rank?
                    sarray.shape = &shape;                 // Only if rank > 1?
                    sarray.data = (void*)data_block->data; // Pointer to the data to be passed
                    strcpy(sarray.type, udt->name);        // The name of the type
                    data = (void*)&psarray;                // Pointer to the SARRAY array pointer
                    udaAddNonMalloc(logmalloclist, (void*)&shape, 1, sizeof(int), "int");

                    rc = 1;

                    packageType = UDA_PACKAGE_STRUCTDATA; // The package is regular XDR

                    UDA_LOG(UDA_LOG_DEBUG, "Sending Package Type: %d\n", packageType);

                    rc = xdr_int(xdrs, &packageType); // Send data package type
                    // **** the original protocolXML2 marks this as the end of a record and dispatches. This causes an
                    // error - unknown root cause

                    rc = rc &&
                         xdr_userdefinedtypelistPut(xdrs,
                                                    userdefinedtypelist); // send the full set of known named structures

                    UDA_LOG(UDA_LOG_DEBUG, "Structure Definitions sent: rc = %d\n", rc);

                    rc = rc && udaXDRUserDefinedTypeDataPut(xdrs, logmalloclist, userdefinedtypelist, u, (void**)data,
                                                            protocolVersion, log_struct_list,
                                                            malloc_source); // send the Data

                    UDA_LOG(UDA_LOG_DEBUG, "Structured Data sent: rc = %d\n", rc);

                    if (!rc) {
                        err = 999;
                        add_error(UDA_CODE_ERROR_TYPE, "protocolXML2Put", err,
                                  "Bad Return Code passing data structures");
                        break;
                    }

                    //======================================================================================================================

                } else { // Receive Data

                    int option = 4;

                    UDA_LOG(UDA_LOG_DEBUG, "Receiving Package Type\n");

                    // **** the original protocolXML2 reads the next record. This causes an error - unknown root cause

                    rc = rc && xdr_int(xdrs, &packageType); // Receive data package type

                    if ((private_flags & PRIVATEFLAG_XDRFILE) == 0 && packageType == UDA_PACKAGE_STRUCTDATA) {
                        option = 1;
                    }

                    UDA_LOG(UDA_LOG_DEBUG, "Receive data option : %d\n", option);
                    UDA_LOG(UDA_LOG_DEBUG, "Receive package Type: %d\n", packageType);

                    if (option == 4) {
                        err = 999;
                        add_error(UDA_SYSTEM_ERROR_TYPE, "protocolXML2Put", err, "Unknown package Type control option");
                        break;
                    }

                    // Unpack data structures

                    if (option == 1) {

                        logmalloclist = (LOGMALLOCLIST*)malloc(sizeof(LOGMALLOCLIST));
                        initLogMallocList(logmalloclist);

                        userdefinedtypelist = (USERDEFINEDTYPELIST*)malloc(sizeof(USERDEFINEDTYPELIST));
                        auto udt_received = (USERDEFINEDTYPE*)malloc(sizeof(USERDEFINEDTYPE));

                        initUserDefinedTypeList(userdefinedtypelist);

                        rc = rc && xdr_userdefinedtypelistPut(
                                       xdrs,
                                       userdefinedtypelist); // receive the full set of known named structures

                        UDA_LOG(UDA_LOG_DEBUG, "userdefinedtypelist received\n");

                        if (!rc) {
                            err = 999;
                            add_error(UDA_CODE_ERROR_TYPE, "protocolXML2Put", err,
                                      "Failure receiving Structure Definitions");
                            break;
                        }

                        initUserDefinedType(udt_received);

                        rc = rc && udaXDRUserDefinedTypeDataPut(xdrs, logmalloclist, userdefinedtypelist, udt_received,
                                                                &data, protocolVersion, log_struct_list,
                                                                malloc_source); // receive the Data
                        // rc = rc && xdrUserDefinedTypeData(xdrs, udt_received, &data);        // receive the Data

                        UDA_LOG(UDA_LOG_DEBUG, "udaXDRUserDefinedTypeData received\n");

                        if (!rc) {
                            err = 999;
                            add_error(UDA_CODE_ERROR_TYPE, "protocolXML2Put", err,
                                      "Failure receiving Data and Structure Definition");
                            break;
                        }

                        if (STR_EQUALS(udt_received->name, "SARRAY")) { // expecting this carrier structure

                            auto general_block = (GENERAL_BLOCK*)malloc(sizeof(GENERAL_BLOCK));

                            auto s = (SARRAY*)data;
                            if (s->count != data_block->data_n) { // check for consistency
                                err = 999;
                                add_error(UDA_CODE_ERROR_TYPE, "protocolXML2Put", err, "Inconsistent S Array Counts");
                                break;
                            }

                            general_block->userdefinedtype = udt_received;
                            general_block->userdefinedtypelist = userdefinedtypelist;
                            general_block->logmalloclist = logmalloclist;
                            general_block->lastMallocIndex = 0;

                            data_block->data =
                                (char*)udaGetFullNTree(); // Global Root Node with the Carrier Structure containing data

                            data_block->opaque_block =
                                (void*)general_block; // Contains all the other information needed

                        } else {
                            err = 999;
                            add_error(UDA_CODE_ERROR_TYPE, "protocolXML2Put", err,
                                      "Name of Received Data Structure Incorrect");
                            break;
                        }
                    }
                }
            }
        }

        //----------------------------------------------------------------------------
        // End of Error Trap Loop

    } while (0);

    return err;
}
