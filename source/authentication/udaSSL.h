#ifndef UDA_AUTHENTICATION_SSL_H
#define UDA_AUTHENTICATION_SSL_H

#if defined(SSLAUTHENTICATION) && !defined(FATCLIENT)

#include <openssl/ssl.h>
#include <openssl/err.h>
#include <openssl/crypto.h>
#include <openssl/x509.h>
#include <openssl/pem.h>

#define VERIFY_DEPTH	4
#define X509STRINGSIZE	256

void putUdaSSL(SSL *s);
SSL *getUdaSSL();
void putUdaSSLCTX(SSL_CTX *c);
SSL_CTX *getUdaSSLCTX();
void putUdaSSLSocket(int s);
int getUdaSSLSocket();
void getUdaSSLErrorCode(int rc);
void initUdaSSL();
void cleanupUdaSSL();
void closeUdaSSL();

#ifdef SERVERBUILD
SSL_CTX *createUdaServerSSLContext();
int configureUdaServerSSLContext();
X509_CRL *loadUdaServerSSLCrl(char *crlist);
int addUdaSSLCrlsStore(X509_STORE *st, STACK_OF(X509_CRL) *crls);
int startUdaServerSSL();
int readUdaServerSSL(void* iohandle, char* buf, int count);
int writeUdaServerSSL(void* iohandle, char* buf, int count);
#else
SSL_CTX *createUdaClientSSLContext();
int configureUdaClientSSLContext();
int startUdaClientSSL();
int readUdaClientSSL(void* iohandle, char* buf, int count);
int writeUdaClientSSL(void* iohandle, char* buf, int count);
#endif

#endif // SSLAUTHENTICATION

#endif // UDA_AUTHENTICATION_SSL_H
