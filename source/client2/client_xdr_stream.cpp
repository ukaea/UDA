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

std::pair<XDR*, XDR*> uda::client::create_xdr_stream(IoData* io_data)
{
    static XDR client_input = {};
    static XDR client_output = {};

    client_output.x_ops = nullptr;
    client_input.x_ops = nullptr;

    UDA_LOG(UDA_LOG_DEBUG, "Creating XDR Streams");

#if defined(SSLAUTHENTICATION) && !defined(FATCLIENT)
    if (getUdaClientSSLDisabled()) {
#  if defined(__APPLE__) || defined(__TIRPC__)
        xdrrec_create(&client_output, DBReadBlockSize, DBWriteBlockSize, io_data,
                      reinterpret_cast<int (*)(void*, void*, int)>(uda::client::readin),
                      reinterpret_cast<int (*)(void*, void*, int)>(uda::client::writeout));

        xdrrec_create(&client_input, DBReadBlockSize, DBWriteBlockSize, io_data,
                      reinterpret_cast<int (*)(void*, void*, int)>(uda::client::readin),
                      reinterpret_cast<int (*)(void*, void*, int)>(uda::client::writeout));
#  else
        xdrrec_create(&client_output, DBReadBlockSize, DBWriteBlockSize, (char*)io_data,
                      reinterpret_cast<int (*)(char*, char*, int)>(uda::client::readin),
                      reinterpret_cast<int (*)(char*, char*, int)>(uda::client::writeout));

        xdrrec_create(&client_input, DBReadBlockSize, DBWriteBlockSize, (char*)io_data,
                      reinterpret_cast<int (*)(char*, char*, int)>(uda::client::readin),
                      reinterpret_cast<int (*)(char*, char*, int)>(uda::client::writeout));
#  endif
    } else {
#  if defined(__APPLE__) || defined(__TIRPC__)
        xdrrec_create(&client_output, DBReadBlockSize, DBWriteBlockSize, io_data,
                      reinterpret_cast<int (*)(void*, void*, int)>(readUdaClientSSL),
                      reinterpret_cast<int (*)(void*, void*, int)>(writeUdaClientSSL));

        xdrrec_create(&client_input, DBReadBlockSize, DBWriteBlockSize, io_data,
                      reinterpret_cast<int (*)(void*, void*, int)>(readUdaClientSSL),
                      reinterpret_cast<int (*)(void*, void*, int)>(writeUdaClientSSL));
#  else
        xdrrec_create(&client_output, DBReadBlockSize, DBWriteBlockSize, (char*)io_data,
                      reinterpret_cast<int (*)(char*, char*, int)>(readUdaClientSSL),
                      reinterpret_cast<int (*)(char*, char*, int)>(writeUdaClientSSL));

        xdrrec_create(&client_input, DBReadBlockSize, DBWriteBlockSize, (char*)io_data,
                      reinterpret_cast<int (*)(char*, char*, int)>(readUdaClientSSL),
                      reinterpret_cast<int (*)(char*, char*, int)>(writeUdaClientSSL));
#  endif
    }
#else

#  if defined(__APPLE__) || defined(__TIRPC__)
    xdrrec_create(&client_output, DBReadBlockSize, DBWriteBlockSize, io_data,
                  reinterpret_cast<int (*)(void*, void*, int)>(uda::client::readin),
                  reinterpret_cast<int (*)(void*, void*, int)>(uda::client::writeout));

    xdrrec_create(&client_input, DBReadBlockSize, DBWriteBlockSize, io_data,
                  reinterpret_cast<int (*)(void*, void*, int)>(uda::client::readin),
                  reinterpret_cast<int (*)(void*, void*, int)>(uda::client::writeout));
#  else
    xdrrec_create(&client_output, DBReadBlockSize, DBWriteBlockSize, (char*)io_data,
                  reinterpret_cast<int (*)(char*, char*, int)>(uda::client::readin),
                  reinterpret_cast<int (*)(char*, char*, int)>(uda::client::writeout));

    xdrrec_create(&client_input, DBReadBlockSize, DBWriteBlockSize, (char*)io_data,
                  reinterpret_cast<int (*)(char*, char*, int)>(uda::client::readin),
                  reinterpret_cast<int (*)(char*, char*, int)>(uda::client::writeout));
#  endif

#endif // SSLAUTHENTICATION

    client_input.x_op = XDR_DECODE;
    client_output.x_op = XDR_ENCODE;

    return std::make_pair(&client_input, &client_output);
}
