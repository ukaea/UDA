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

#define VERIFY_DEPTH	4
#define X509STRINGSIZE	256

#ifdef SERVERBUILD

void putUdaServerSSL(SSL *s);
SSL *getUdaServerSSL();
void putUdaServerSSLCTX(SSL_CTX *c);
SSL_CTX *getUdaServerSSLCTX();
void putUdaServerSSLSocket(int s);
int getUdaServerSSLSocket();
void getUdaServerSSLErrorCode(int rc);
void initUdaServerSSL();
void closeUdaServerSSL();
void putUdaServerSSLDisabled(int disabled);
int getUdaServerSSLDisabled();
void putUdaServerSSLGlobalInit(int init);
int getUdaServerSSLGlobalInit();
SSL_CTX *createUdaServerSSLContext();
int configureUdaServerSSLContext();
X509_CRL *loadUdaServerSSLCrl(char *crlist);
int addUdaServerSSLCrlsStore(X509_STORE *st, STACK_OF(X509_CRL) *crls);
int startUdaServerSSL();
int readUdaServerSSL(void* iohandle, char* buf, int count);
int writeUdaServerSSL(void* iohandle, char* buf, int count);

#else

void putUdaClientSSL(SSL *s);
SSL *getUdaClientSSL();
void putUdaClientSSLCTX(SSL_CTX *c);
SSL_CTX *getUdaClientSSLCTX();
void putUdaClientSSLSocket(int s);
int getUdaClientSSLSocket();
void getUdaClientSSLErrorCode(int rc);
void initUdaClientSSL();
void closeUdaClientSSL();
void putUdaClientSSLDisabled(int disabled);
int getUdaClientSSLDisabled();
void putUdaClientSSLGlobalInit(int init);
int getUdaClientSSLGlobalInit();
void putUdaClientSSLOK(int ok);
int getUdaClientSSLOK();
void putUdaClientSSLProtocol(int specified);
int getUdaClientSSLProtocol();
SSL_CTX *createUdaClientSSLContext();
int configureUdaClientSSLContext();
int startUdaClientSSL();
int readUdaClientSSL(void* iohandle, char* buf, int count);
int writeUdaClientSSL(void* iohandle, char* buf, int count);

#endif

#endif // SSLAUTHENTICATION

#endif // UDA_AUTHENTICATION_SSL_H
