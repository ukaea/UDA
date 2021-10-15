#include "createXDRStream.h"

#include <server/udaServer.h>

#include "writer.h"

#if !defined(FATCLIENT) && defined(SSLAUTHENTICATION)
#include <authentication/udaServerSSL.h>
#endif

std::pair<XDR*, XDR*> CreateXDRStream()
{
    static XDR server_input = {};
    static XDR server_output = {};

    server_output.x_ops = nullptr;
    server_input.x_ops = nullptr;

#if !defined(FATCLIENT) && defined(SSLAUTHENTICATION)

    if (getUdaServerSSLDisabled()) {

#if defined (__APPLE__) || defined(__TIRPC__)
       xdrrec_create( &server_output, DB_READ_BLOCK_SIZE, DB_WRITE_BLOCK_SIZE, nullptr,
                      reinterpret_cast<int (*)(void *, void *, int)>(Readin),
                      reinterpret_cast<int (*)(void *, void *, int)>(Writeout));

       xdrrec_create( &server_input, DB_READ_BLOCK_SIZE, DB_WRITE_BLOCK_SIZE, nullptr,
                      reinterpret_cast<int (*)(void *, void *, int)>(Readin),
                      reinterpret_cast<int (*)(void *, void *, int)>(Writeout));
#else
       xdrrec_create( &server_output, DB_READ_BLOCK_SIZE, DB_WRITE_BLOCK_SIZE, nullptr,
                      reinterpret_cast<int (*)(char *, char *, int)>(Readin),
                      reinterpret_cast<int (*)(char *, char *, int)>(Writeout));

       xdrrec_create( &server_input, DB_READ_BLOCK_SIZE, DB_WRITE_BLOCK_SIZE, nullptr,
                      reinterpret_cast<int (*)(char *, char *, int)>(Readin),
                      reinterpret_cast<int (*)(char *, char *, int)>(Writeout));
#endif     
    } else { 
#if defined (__APPLE__) || defined(__TIRPC__)
       xdrrec_create( &server_output, DB_READ_BLOCK_SIZE, DB_WRITE_BLOCK_SIZE, nullptr,
                      reinterpret_cast<int (*)(void *, void *, int)>(readUdaServerSSL),
                      reinterpret_cast<int (*)(void *, void *, int)>(writeUdaServerSSL));

       xdrrec_create( &server_input, DB_READ_BLOCK_SIZE, DB_WRITE_BLOCK_SIZE, nullptr,
                      reinterpret_cast<int (*)(void *, void *, int)>(readUdaServerSSL),
                      reinterpret_cast<int (*)(void *, void *, int)>(writeUdaServerSSL));
#else
       xdrrec_create( &server_output, DB_READ_BLOCK_SIZE, DB_WRITE_BLOCK_SIZE, nullptr,
                      reinterpret_cast<int (*)(char *, char *, int)>(readUdaServerSSL),
                      reinterpret_cast<int (*)(char *, char *, int)>(writeUdaServerSSL));

       xdrrec_create( &server_input, DB_READ_BLOCK_SIZE, DB_WRITE_BLOCK_SIZE, nullptr,
                      reinterpret_cast<int (*)(char *, char *, int)>(readUdaServerSSL),
                      reinterpret_cast<int (*)(char *, char *, int)>(writeUdaServerSSL));
#endif
    }

#else    // SSLAUTHENTICATION

#if defined (__APPLE__) || defined(__TIRPC__)
    xdrrec_create(&server_output, DB_READ_BLOCK_SIZE, DB_WRITE_BLOCK_SIZE, nullptr,
                  reinterpret_cast<int (*)(void*, void*, int)>(Readin),
                  reinterpret_cast<int (*)(void*, void*, int)>(Writeout));

    xdrrec_create(&server_input, DB_READ_BLOCK_SIZE, DB_WRITE_BLOCK_SIZE, nullptr,
                  reinterpret_cast<int (*)(void*, void*, int)>(Readin),
                  reinterpret_cast<int (*)(void*, void*, int)>(Writeout));
#else
    xdrrec_create( &server_output, DB_READ_BLOCK_SIZE, DB_WRITE_BLOCK_SIZE, nullptr,
                   reinterpret_cast<int (*)(char *, char *, int)>(Readin),
                   reinterpret_cast<int (*)(char *, char *, int)>(Writeout));

    xdrrec_create( &server_input, DB_READ_BLOCK_SIZE, DB_WRITE_BLOCK_SIZE, nullptr,
                   reinterpret_cast<int (*)(char *, char *, int)>(Readin),
                   reinterpret_cast<int (*)(char *, char *, int)>(Writeout));
#endif

#endif   // SSLAUTHENTICATION

    server_input.x_op = XDR_DECODE;
    server_output.x_op = XDR_ENCODE;

    return std::make_pair(&server_input, &server_output);
}
