#include "readXDRFile.h"

#include <cerrno>
#include <cstdlib>

#include "clientserver/error_log.h"
#include "logging/logging.h"

#ifdef SERVERBUILD
#  include "server/serverStartup.h"
#endif

constexpr size_t MaxDoLoopLimit = 500; // ~50MB file

using namespace uda::client_server;
using namespace uda::logging;

int uda::client_server::send_xdr_file(XDR* xdrs, const char* xdrfile)
{

    int err = 0, rc = 1, nchar, buf_size, count;
    FILE* fh;
    char* bp = nullptr;

    //----------------------------------------------------------------------
    // Open the File as a Binary Stream

    errno = 0;
    fh = fopen(xdrfile, "rb");

    if (fh == nullptr || errno != 0 || ferror(fh)) {
        err = 999;
        if (errno != 0) {
            add_error(ErrorType::System, "sendXDRFile", errno, "");
        }
        add_error(ErrorType::Code, "sendXDRFile", err, "Unable to Open the XDR File for Read Access");
        if (fh != nullptr) {
            fclose(fh);
        }
        return err;
    }

    UDA_LOG(UDA_LOG_DEBUG, "reading temporary XDR file {}", xdrfile);

    //----------------------------------------------------------------------
    // Error Trap Loop

    do {

        //----------------------------------------------------------------------
        // Read File and write to xdr data stream

        nchar = 0;
        buf_size = 100 * 1024;
        rc = 1;
        count = 0;

        if ((bp = (char*)malloc(buf_size * sizeof(char))) == nullptr) {
            err = 999;
            add_error(ErrorType::Code, "sendXDRFile", err, "Unable to Allocate Heap Memory for the XDR File");
            buf_size = 0;
            rc = xdr_int(xdrs, &buf_size);
            break;
        }

        rc = xdr_int(xdrs, &buf_size); // Send Server buffer size, e.g., 100k bytes

        UDA_LOG(UDA_LOG_DEBUG, "Buffer size {}", buf_size);

        while (!feof(fh)) {
            nchar = (int)fread(bp, sizeof(char), buf_size, fh);
            rc = rc && xdr_int(xdrs, &nchar); // Number of Bytes to send

            UDA_LOG(UDA_LOG_DEBUG, "File block size {}", nchar);

            if (nchar > 0) { // Send the bytes
                rc = rc && xdr_vector(xdrs, (char*)bp, nchar, sizeof(char), (xdrproc_t)xdr_char);
            }

            rc = rc && xdrrec_endofrecord(xdrs, 1);

            count = count + nchar;
        }

        UDA_LOG(UDA_LOG_DEBUG, "Total File size {}", count);

    } while (0);

    nchar = 0; // No more bytes to send
    rc = rc && xdr_int(xdrs, &nchar);
    rc = rc && xdrrec_endofrecord(xdrs, 1);

    // *** Send count to client as a check all data received
    // *** Send hash sum to client as a test data is accurate - another reason to use files and cache rather than a data
    // stream

    //----------------------------------------------------------------------
    // Housekeeping

    fclose(fh); // Close the File
    free(bp);

    return err;
}

int uda::client_server::receive_xdr_file(XDR* xdrs, const char* xdrfile)
{
    int err = 0, rc = 1, nchar, buf_size, count;
    size_t do_loop_limit = 0;
    FILE* fh;
    char* bp = nullptr;

    //----------------------------------------------------------------------
    // Open the File as a Binary Stream

    errno = 0;
    fh = fopen(xdrfile, "wb");

    if (fh == nullptr || errno != 0 || ferror(fh)) {
        err = 999;
        if (errno != 0) {
            add_error(ErrorType::System, "receiveXDRFile", errno, "");
        }
        add_error(ErrorType::Code, "receiveXDRFile", err, "Unable to Open the XDR File for Write Access");
        if (fh != nullptr) {
            fclose(fh);
        }
        return err;
    }

    UDA_LOG(UDA_LOG_DEBUG, "receiveXDRFile: writing temporary XDR file {}", xdrfile);

    //----------------------------------------------------------------------
    // Error Trap Loop

    do {

        //----------------------------------------------------------------------
        // Read xdr data stream and write file

        nchar = 0;

        rc = xdrrec_skiprecord(xdrs);
        rc = xdr_int(xdrs, &buf_size); // Server buffer size, e.g., 100k bytes

        UDA_LOG(UDA_LOG_DEBUG, "receiveXDRFile: Buffer size {}", buf_size);

        if (buf_size <= 0 || buf_size > 100 * 1024) {
            err = 999;
            add_error(ErrorType::Code, "receiveXDRFile", err, "Zero buffer size: Server failure");
            break;
        }

        if ((bp = (char*)malloc(buf_size * sizeof(char))) == nullptr) {
            err = 999;
            add_error(ErrorType::Code, "receiveXDRFile", err, "Unable to Allocate Heap Memory for the XDR File");
            break;
        }

        count = 0;

        do {
            errno = 0;

            if (do_loop_limit > 0) {
                rc = rc && xdrrec_skiprecord(xdrs);
            }

            rc = rc && xdr_int(xdrs, &nchar); // How many bytes to receive?

            UDA_LOG(UDA_LOG_DEBUG, "receiveXDRFile: [{}] File block size {}", do_loop_limit, nchar);

            if (nchar > buf_size) {
                err = 999;
                add_error(ErrorType::Code, "receiveXDRFile", err, "File block size inconsistent with buffer size");
                break;
            }

            if (nchar > 0) {
                rc = rc && xdr_vector(xdrs, (char*)bp, nchar, sizeof(char), (xdrproc_t)xdr_char); // Bytes
                count = count + (int)fwrite(bp, sizeof(char), nchar, fh);
            }
        } while (nchar > 0 && errno == 0 && do_loop_limit++ < MaxDoLoopLimit);

        if (do_loop_limit >= MaxDoLoopLimit) {
            err = 999;
            add_error(ErrorType::Code, "receiveXDRFile", err, "Maximum XDR file size reached: ~50MBytes");
            break;
        }

        // *** Read count from server to check all data received
        // *** Read hash sum from server to test data is accurate - another reason to use files and cache rather than
        // a data stream

        UDA_LOG(UDA_LOG_DEBUG, "receiveXDRFile: Total File size {}", count);

        if (errno != 0) {
            err = 999;
            add_error(ErrorType::System, "receiveXDRFile", errno, "Problem receiving XDR File");
            break;
        }

    } while (0);

    //----------------------------------------------------------------------
    // Housekeeping

    fclose(fh); // Close the File
    free(bp);

    return err;
}
