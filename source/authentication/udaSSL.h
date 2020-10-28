#ifndef UDA_AUTHENTICATION_SSL_H
#define UDA_AUTHENTICATION_SSL_H

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

#define VERIFY_DEPTH	4
#define X509STRINGSIZE	256

#include <clientserver/export.h>

#ifdef __cplusplus
extern "C" {
#endif

#ifdef SERVERBUILD

LIBRARY_API void putUdaServerSSL(SSL *s);
LIBRARY_API SSL *getUdaServerSSL();
LIBRARY_API void putUdaServerSSLCTX(SSL_CTX *c);
LIBRARY_API SSL_CTX *getUdaServerSSLCTX();
LIBRARY_API void putUdaServerSSLSocket(int s);
LIBRARY_API int getUdaServerSSLSocket();
LIBRARY_API void getUdaServerSSLErrorCode(int rc);
LIBRARY_API void initUdaServerSSL();
LIBRARY_API void closeUdaServerSSL();
LIBRARY_API void putUdaServerSSLDisabled(int disabled);
LIBRARY_API int getUdaServerSSLDisabled();
LIBRARY_API void putUdaServerSSLGlobalInit(int init);
LIBRARY_API int getUdaServerSSLGlobalInit();
LIBRARY_API SSL_CTX *createUdaServerSSLContext();
LIBRARY_API int configureUdaServerSSLContext();
LIBRARY_API X509_CRL *loadUdaServerSSLCrl(char *crlist);
LIBRARY_API int addUdaServerSSLCrlsStore(X509_STORE *st, STACK_OF(X509_CRL) *crls);
LIBRARY_API int startUdaServerSSL();
LIBRARY_API int readUdaServerSSL(void* iohandle, char* buf, int count);
LIBRARY_API int writeUdaServerSSL(void* iohandle, char* buf, int count);

#else

LIBRARY_API void putUdaClientSSL(SSL *s);
LIBRARY_API SSL *getUdaClientSSL();
LIBRARY_API void putUdaClientSSLCTX(SSL_CTX *c);
LIBRARY_API SSL_CTX *getUdaClientSSLCTX();
LIBRARY_API void putUdaClientSSLSocket(int s);
LIBRARY_API int getUdaClientSSLSocket();
LIBRARY_API void getUdaClientSSLErrorCode(int rc);
LIBRARY_API void initUdaClientSSL();
LIBRARY_API void closeUdaClientSSL();
LIBRARY_API void putUdaClientSSLDisabled(int disabled);
LIBRARY_API int getUdaClientSSLDisabled();
LIBRARY_API void putUdaClientSSLGlobalInit(int init);
LIBRARY_API int getUdaClientSSLGlobalInit();
LIBRARY_API void putUdaClientSSLOK(int ok);
LIBRARY_API int getUdaClientSSLOK();
LIBRARY_API void putUdaClientSSLProtocol(int specified);
LIBRARY_API int getUdaClientSSLProtocol();
LIBRARY_API SSL_CTX *createUdaClientSSLContext();
LIBRARY_API int configureUdaClientSSLContext();
LIBRARY_API int startUdaClientSSL();
LIBRARY_API int readUdaClientSSL(void* iohandle, char* buf, int count);
LIBRARY_API int writeUdaClientSSL(void* iohandle, char* buf, int count);

#endif

#ifdef __cplusplus
}
#endif

#endif // SSLAUTHENTICATION

#endif // UDA_AUTHENTICATION_SSL_H
