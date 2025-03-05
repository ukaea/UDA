#pragma once

#include <vector>

#include "clientserver/uda_structs.h"

#if defined(SSLAUTHENTICATION) && !defined(FATCLIENT)

// Create the SSL context and binding to the socket
// 3 UDA protocol modes: TCP without SSL/TLS, TCP and UDP both with SSL/TLS
// This set of functions is concerned only with the SSL/TLS protocol (authentication and encryption) - not with
// establishing socket connections or non SSL TCP transport

// Server host addressed beginng with SSL:// are assumed to be using SSL authentication. The SSL:// prefix is removed to
// make the connection.

namespace uda::authentication
{

int start_server_ssl(std::vector<client_server::UdaError>& error_stack);

void close_server_ssl();

int read_server_ssl(void* iohandle, char* buf, int count);

int write_server_ssl(void* iohandle, const char* buf, int count);

void put_server_ssl_socket(int socket);

bool get_server_ssl_disabled();

} // namespace uda::authentication

#endif // SSLAUTHENTICATION
