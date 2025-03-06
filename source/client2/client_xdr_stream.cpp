#include "client_xdr_stream.hpp"

#include <cstdio>
#include <rpc/rpc.h>

#include "clientserver/uda_defines.h"
#include "logging/logging.h"

#include "connection.hpp"

#if defined(SSLAUTHENTICATION) && !defined(FATCLIENT)
#  include <authentication/udaClientSSL.h>

using namespace uda::authentication;
#endif

using namespace uda::logging;
using namespace uda::client_server;

void uda::client::xdr_deleter(XDR* xdr_ptr)
{
    if (xdr_ptr) {
        if (xdr_ptr->x_ops != nullptr) {
            xdr_destroy(xdr_ptr);
        }
        delete xdr_ptr;
    }
}

// this wraps calls to the lower-level xdrrec_create method. This (i) creates the stream pointers which
// hold state for the XDR input and output operations, it (ii) takes in buffer sizes for the send and receive buffers
// (iii) takes function pointers for the callbacks which perform the actual i/o (this are the readin and writeout
// client functions), and (iv) takes a handle pointer (io_data here) which is an argument passed to the callback functions
//
// The IoData struct can therefore have any definition we choose, but must contian all data required by the readin
// and writeout methods
std::pair<std::unique_ptr<XDR, void(*)(XDR*)>, std::unique_ptr<XDR, void(*)(XDR*)>> uda::client::create_xdr_stream(IoData* io_data)
{
    // static XDR client_input = {};
    // static XDR client_output = {};
    std::unique_ptr<XDR, void(*)(XDR*)> client_input(new XDR(), xdr_deleter);
    std::unique_ptr<XDR, void(*)(XDR*)> client_output(new XDR(), xdr_deleter);

    client_output->x_ops = nullptr;
    client_input->x_ops = nullptr;

    UDA_LOG(UDA_LOG_DEBUG, "Creating XDR Streams");

#if defined(SSLAUTHENTICATION) && !defined(FATCLIENT)
    if (get_client_ssl_disabled()) {
#  if defined(__APPLE__) || defined(__TIRPC__)
        xdrrec_create(client_output.get(), DBReadBlockSize, DBWriteBlockSize, io_data,
                      reinterpret_cast<int (*)(void*, void*, int)>(uda::client::readin),
                      reinterpret_cast<int (*)(void*, void*, int)>(uda::client::writeout));

        xdrrec_create(client_input.get(), DBReadBlockSize, DBWriteBlockSize, io_data,
                      reinterpret_cast<int (*)(void*, void*, int)>(uda::client::readin),
                      reinterpret_cast<int (*)(void*, void*, int)>(uda::client::writeout));
#  else
        xdrrec_create(client_output.get(), DBReadBlockSize, DBWriteBlockSize, (char*)io_data,
                      reinterpret_cast<int (*)(char*, char*, int)>(uda::client::readin),
                      reinterpret_cast<int (*)(char*, char*, int)>(uda::client::writeout));

        xdrrec_create(client_input.get(), DBReadBlockSize, DBWriteBlockSize, (char*)io_data,
                      reinterpret_cast<int (*)(char*, char*, int)>(uda::client::readin),
                      reinterpret_cast<int (*)(char*, char*, int)>(uda::client::writeout));
#  endif
    } else {
#  if defined(__APPLE__) || defined(__TIRPC__)
        xdrrec_create(client_output.get(), DBReadBlockSize, DBWriteBlockSize, io_data,
                      reinterpret_cast<int (*)(void*, void*, int)>(read_client_ssl),
                      reinterpret_cast<int (*)(void*, void*, int)>(write_client_ssl));

        xdrrec_create(client_input.get(), DBReadBlockSize, DBWriteBlockSize, io_data,
                      reinterpret_cast<int (*)(void*, void*, int)>(read_client_ssl),
                      reinterpret_cast<int (*)(void*, void*, int)>(write_client_ssl));
#  else
        xdrrec_create(client_output.get(), DBReadBlockSize, DBWriteBlockSize, (char*)io_data,
                      reinterpret_cast<int (*)(char*, char*, int)>(readUdaClientSSL),
                      reinterpret_cast<int (*)(char*, char*, int)>(writeUdaClientSSL));

        xdrrec_create(client_input.get(), DBReadBlockSize, DBWriteBlockSize, (char*)io_data,
                      reinterpret_cast<int (*)(char*, char*, int)>(readUdaClientSSL),
                      reinterpret_cast<int (*)(char*, char*, int)>(writeUdaClientSSL));
#  endif
    }
#else

#  if defined(__APPLE__) || defined(__TIRPC__)
    xdrrec_create(client_output.get(), DBReadBlockSize, DBWriteBlockSize, io_data,
                  reinterpret_cast<int (*)(void*, void*, int)>(uda::client::readin),
                  reinterpret_cast<int (*)(void*, void*, int)>(uda::client::writeout));

    xdrrec_create(client_input.get(), DBReadBlockSize, DBWriteBlockSize, io_data,
                  reinterpret_cast<int (*)(void*, void*, int)>(uda::client::readin),
                  reinterpret_cast<int (*)(void*, void*, int)>(uda::client::writeout));
#  else
    xdrrec_create(client_output.get(), DBReadBlockSize, DBWriteBlockSize, (char*)io_data,
                  reinterpret_cast<int (*)(char*, char*, int)>(uda::client::readin),
                  reinterpret_cast<int (*)(char*, char*, int)>(uda::client::writeout));

    xdrrec_create(client_input.get(), DBReadBlockSize, DBWriteBlockSize, (char*)io_data,
                  reinterpret_cast<int (*)(char*, char*, int)>(uda::client::readin),
                  reinterpret_cast<int (*)(char*, char*, int)>(uda::client::writeout));
#  endif

#endif // SSLAUTHENTICATION

    client_input->x_op = XDR_DECODE;
    client_output->x_op = XDR_ENCODE;

    return std::make_pair(std::move(client_input), std::move(client_output));
}
