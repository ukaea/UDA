#pragma once

#if defined(SSLAUTHENTICATION) && !defined(FATCLIENT)

// Create the SSL context and binding to the socket
// 3 UDA protocol modes: TCP without SSL/TLS, TCP and UDP both with SSL/TLS
// This set of functions is concerned only with the SSL/TLS protocol (authentication and encryption) - not with
// establishing socket connections or non SSL TCP transport

// Server host addressed beginng with SSL:// are assumed to be using SSL authentication. The SSL:// prefix is removed to
// make the connection.

namespace uda::authentication
{

int startUdaServerSSL();

void closeUdaServerSSL();

int readUdaServerSSL(void* iohandle, char* buf, int count);

int writeUdaServerSSL(void* iohandle, const char* buf, int count);

void putUdaServerSSLSocket(int socket);

bool getUdaServerSSLDisabled();

} // namespace uda::authentication

#endif // SSLAUTHENTICATION
