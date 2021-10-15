#ifndef UDA_AUTHENTICATION_CLIENT_SSL_H
#define UDA_AUTHENTICATION_CLIENT_SSL_H

#if defined(SSLAUTHENTICATION) && !defined(FATCLIENT)

// Create the SSL context and binding to the socket
// 3 UDA protocol modes: TCP without SSL/TLS, TCP and UDP both with SSL/TLS
// This set of functions is concerned only with the SSL/TLS protocol (authentication and encryption) - not with establishing socket connections or non SSL TCP transport

// Server host addressed beginng with SSL:// are assumed to be using SSL authentication. The SSL:// prefix is removed to make the connection.

#include <openssl/ssl.h>
#include <openssl/err.h>
#include <openssl/crypto.h>
#include <openssl/x509.h>
#include <openssl/pem.h>
#include <openssl/asn1.h>

#define VERIFY_DEPTH    4
#define X509STRINGSIZE    256

#include <clientserver/export.h>

#ifdef __cplusplus
extern "C" {
#endif

LIBRARY_API bool getUdaClientSSLDisabled();
LIBRARY_API SSL *getUdaClientSSL();
LIBRARY_API void putUdaClientSSLSocket(int s);
LIBRARY_API void closeUdaClientSSL();
LIBRARY_API void putUdaClientSSLProtocol(int specified);
LIBRARY_API int startUdaClientSSL();
LIBRARY_API int readUdaClientSSL(void* iohandle, char* buf, int count);
LIBRARY_API int writeUdaClientSSL(void* iohandle, char* buf, int count);

#ifdef __cplusplus
}
#endif

#endif // SSLAUTHENTICATION

#endif // UDA_AUTHENTICATION_CLIENT_SSL_H
