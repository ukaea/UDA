#ifndef UDA_AUTHENTICATION_SSL_H
#define UDA_AUTHENTICATION_SSL_H

#if defined(SSLAUTHENTICATION) && !defined(FATCLIENT)

#include <openssl/ssl.h>
#include <openssl/err.h>
#include <openssl/crypto.h>
#include <openssl/x509.h>
#include <openssl/pem.h>

void putUdaSSL(SSL *s);
SSL *getUdaSSL();
void putUdaSSLCTX(SSL_CTX *c);
SSL_CTX *getUdaSSLCTX();
void putUdaSSLSocket(int s);
int getUdaSSLSocket();
void getUdaSSLErrorCode(int rc);
void initUdaSSL();
void cleanupUdaSSL();
SSL_CTX *createUdaSSLContext();
int configureUdaSSLContext();
int startUdaSSL();
void closeUdaSSL();
int readUdaClientSSL(void* iohandle, char* buf, int count);
int writeUdaClientSSL(void* iohandle, char* buf, int count);
int readUdaServerSSL(void* iohandle, char* buf, int count);
int writeUdaServerSSL(void* iohandle, char* buf, int count);

#endif // UDA_AUTHENTICATION_SSL_H
