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

struct FCloseDeleter {
    void operator()(FILE* file) const {fclose(file);}
};

int uda::protocol::send_xdr_file(XDR* xdrs, const char* xdrfile)
{

    int err = 0;
    int rc = 1;

    //----------------------------------------------------------------------
    // Open the File as a Binary Stream

    errno = 0;
    std::unique_ptr<FILE, FCloseDeleter> fh{fopen(xdrfile, "rb")};

    if (fh == nullptr || errno != 0 || ferror(fh.get())) {
        if (errno != 0) {
            UDA_SYS_THROW("");
        }
        UDA_THROW(err, "Unable to Open the XDR File for Read Access");
        return err;
    }

    UDA_LOG(UDA_LOG_DEBUG, "reading temporary XDR file {}", xdrfile);

    //----------------------------------------------------------------------
    // Error Trap Loop

    int nchar = 0;

    do {

        //----------------------------------------------------------------------
        // Read File and write to xdr data stream

        int buf_size = 100 * 1024;
        int count = 0;

        auto bp = std::make_unique<char[]>(buf_size);

        rc = xdr_int(xdrs, &buf_size); // Send Server buffer size, e.g., 100k bytes

        UDA_LOG(UDA_LOG_DEBUG, "Buffer size {}", buf_size);

        while (!feof(fh.get())) {
            nchar = static_cast<int>(fread(bp.get(), sizeof(char), buf_size, fh.get()));
            rc = rc && xdr_int(xdrs, &nchar); // Number of Bytes to send

            UDA_LOG(UDA_LOG_DEBUG, "File block size {}", nchar);

            if (nchar > 0) { // Send the bytes
                rc = rc && xdr_vector(xdrs, bp.get(), nchar, sizeof(char), reinterpret_cast<xdrproc_t>(xdr_char));
            }

            rc = rc && xdrrec_endofrecord(xdrs, 1);

            count = count + nchar;
        }

        UDA_LOG(UDA_LOG_DEBUG, "Total File size {}", count);

    } while (0);

    nchar = 0; // No more bytes to send
    rc = rc && xdr_int(xdrs, &nchar);
    rc = rc && xdrrec_endofrecord(xdrs, 1);

    return err;
}

int uda::protocol::receive_xdr_file(XDR* xdrs, const char* xdrfile)
{
    int err = 0, rc = 1, nchar, buf_size, count;
    size_t do_loop_limit = 0;

    //----------------------------------------------------------------------
    // Open the File as a Binary Stream

    errno = 0;
    std::unique_ptr<FILE, FCloseDeleter> fh{fopen(xdrfile, "wb")};

    if (fh == nullptr || errno != 0 || ferror(fh.get())) {
        if (errno != 0) {
            UDA_SYS_THROW("");
        }
        UDA_THROW(999, "Unable to Open the XDR File for Write Access");
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
            UDA_THROW(999, "Zero buffer size: Server failure");
        }

        auto bp = std::make_unique<char[]>(buf_size);

        count = 0;

        do {
            errno = 0;

            if (do_loop_limit > 0) {
                rc = rc && xdrrec_skiprecord(xdrs);
            }

            rc = rc && xdr_int(xdrs, &nchar); // How many bytes to receive?

            UDA_LOG(UDA_LOG_DEBUG, "receiveXDRFile: [{}] File block size {}", do_loop_limit, nchar);

            if (nchar > buf_size) {
                UDA_THROW(999, "File block size inconsistent with buffer size");
            }

            if (nchar > 0) {
                rc = rc && xdr_vector(xdrs, bp.get(), nchar, sizeof(char), (xdrproc_t)xdr_char); // Bytes
                count = count + (int)fwrite(bp.get(), sizeof(char), nchar, fh.get());
            }
        } while (nchar > 0 && errno == 0 && do_loop_limit++ < MaxDoLoopLimit);

        if (do_loop_limit >= MaxDoLoopLimit) {
            UDA_THROW(999, "Maximum XDR file size reached: ~50MBytes");
        }

        // *** Read count from server to check all data received
        // *** Read hash sum from server to test data is accurate - another reason to use files and cache rather than
        // a data stream

        UDA_LOG(UDA_LOG_DEBUG, "receiveXDRFile: Total File size {}", count);

        if (errno != 0) {
            UDA_THROW(999, "Problem receiving XDR File");
        }

    } while (0);

    return err;
}
