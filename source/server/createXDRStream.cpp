#include "createXDRStream.h"

#include "server/udaServer.h"

#include "writer.h"

#if !defined(FATCLIENT) && defined(SSLAUTHENTICATION)
#  include <authentication/server_ssl.h>

using namespace uda::authentication;
#endif

using namespace uda::client_server;

std::pair<XDR*, XDR*> uda::server::serverCreateXDRStream(IoData* io_data)
{
    static XDR server_input = {};
    static XDR server_output = {};

    server_output.x_ops = nullptr;
    server_input.x_ops = nullptr;

#if !defined(FATCLIENT) && defined(SSLAUTHENTICATION)
    if (getUdaServerSSLDisabled()) {
#  if defined(__APPLE__) || defined(__TIRPC__)
        xdrrec_create(&server_output, DBReadBlockSize, DBWriteBlockSize, io_data,
                      reinterpret_cast<int (*)(void*, void*, int)>(server_read),
                      reinterpret_cast<int (*)(void*, void*, int)>(server_write));

        xdrrec_create(&server_input, DBReadBlockSize, DBWriteBlockSize, io_data,
                      reinterpret_cast<int (*)(void*, void*, int)>(server_read),
                      reinterpret_cast<int (*)(void*, void*, int)>(server_write));
#  else
        xdrrec_create(&server_output, DBReadBlockSize, DBWriteBlockSize, (char*)io_data,
                      reinterpret_cast<int (*)(char*, char*, int)>(server_read),
                      reinterpret_cast<int (*)(char*, char*, int)>(server_write));

        xdrrec_create(&server_input, DBReadBlockSize, DBWriteBlockSize, (char*)io_data,
                      reinterpret_cast<int (*)(char*, char*, int)>(server_read),
                      reinterpret_cast<int (*)(char*, char*, int)>(server_write));
#  endif
    } else {
#  if defined(__APPLE__) || defined(__TIRPC__)
        xdrrec_create(&server_output, DBReadBlockSize, DBWriteBlockSize, io_data,
                      reinterpret_cast<int (*)(void*, void*, int)>(readUdaServerSSL),
                      reinterpret_cast<int (*)(void*, void*, int)>(writeUdaServerSSL));

        xdrrec_create(&server_input, DBReadBlockSize, DBWriteBlockSize, io_data,
                      reinterpret_cast<int (*)(void*, void*, int)>(readUdaServerSSL),
                      reinterpret_cast<int (*)(void*, void*, int)>(writeUdaServerSSL));
#  else
        xdrrec_create(&server_output, DBReadBlockSize, DBWriteBlockSize, (char*)io_data,
                      reinterpret_cast<int (*)(char*, char*, int)>(readUdaServerSSL),
                      reinterpret_cast<int (*)(char*, char*, int)>(writeUdaServerSSL));

        xdrrec_create(&server_input, DBReadBlockSize, DBWriteBlockSize, (char*)io_data,
                      reinterpret_cast<int (*)(char*, char*, int)>(readUdaServerSSL),
                      reinterpret_cast<int (*)(char*, char*, int)>(writeUdaServerSSL));
#  endif
    }
#else // SSLAUTHENTICATION

#  if defined(__APPLE__) || defined(__TIRPC__)
    xdrrec_create(&server_output, DBReadBlockSize, DBWriteBlockSize, io_data,
                  reinterpret_cast<int (*)(void*, void*, int)>(server_read),
                  reinterpret_cast<int (*)(void*, void*, int)>(server_write));

    xdrrec_create(&server_input, DBReadBlockSize, DBWriteBlockSize, io_data,
                  reinterpret_cast<int (*)(void*, void*, int)>(server_read),
                  reinterpret_cast<int (*)(void*, void*, int)>(server_write));
#  else
    xdrrec_create(&server_output, DBReadBlockSize, DBWriteBlockSize, (char*)io_data,
                  reinterpret_cast<int (*)(char*, char*, int)>(server_read),
                  reinterpret_cast<int (*)(char*, char*, int)>(server_write));

    xdrrec_create(&server_input, DBReadBlockSize, DBWriteBlockSize, (char*)io_data,
                  reinterpret_cast<int (*)(char*, char*, int)>(server_read),
                  reinterpret_cast<int (*)(char*, char*, int)>(server_write));
#  endif

#endif // SSLAUTHENTICATION

    server_input.x_op = XDR_DECODE;
    server_output.x_op = XDR_ENCODE;

    return std::make_pair(&server_input, &server_output);
}
