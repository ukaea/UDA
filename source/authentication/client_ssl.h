#pragma once
#include <clientserver/uda_structs.h>
#include <config/config.h>

#if defined(SSLAUTHENTICATION) && !defined(FATCLIENT)

// Create the SSL context and binding to the socket
// 3 UDA protocol modes: TCP without SSL/TLS, TCP and UDP both with SSL/TLS
// This set of functions is concerned only with the SSL/TLS protocol (authentication and encryption) - not with
// establishing socket connections or non SSL TCP transport

// Server host addressed beginng with SSL:// are assumed to be using SSL authentication. The SSL:// prefix is removed to
// make the connection.

#  include <openssl/asn1.h>
#  include <openssl/crypto.h>
#  include <openssl/err.h>
#  include <openssl/pem.h>
#  include <openssl/ssl.h>
#  include <openssl/x509.h>

#  define VERIFY_DEPTH 4
#  define X509STRINGSIZE 256

#  include "clientserver/socket_structs.h"

namespace uda::authentication
{

bool get_client_ssl_disabled();

SSL* get_client_ssl();

void put_client_ssl_socket(int s);

void close_client_ssl();

void put_client_ssl_protocol(int specified);

int init_client_ssl(const config::Config& config, std::vector<client_server::UdaError>& error_stack);

int start_client_ssl(std::vector<client_server::UdaError>& error_stack);

int read_client_ssl(void* io_handle, char* buf, int count);

int write_client_ssl(void* io_handle, const char* buf, int count);

void put_client_host(const client_server::HostData* host);

} // namespace uda::authentication

#endif // _sslAUTHENTICATION
