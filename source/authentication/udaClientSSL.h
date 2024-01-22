#pragma once

#ifndef UDA_AUTHENTICATION_CLIENT_SSL_H
#  define UDA_AUTHENTICATION_CLIENT_SSL_H

#  if defined(SSLAUTHENTICATION) && !defined(FATCLIENT)

// Create the SSL context and binding to the socket
// 3 UDA protocol modes: TCP without SSL/TLS, TCP and UDP both with SSL/TLS
// This set of functions is concerned only with the SSL/TLS protocol (authentication and encryption) - not with
// establishing socket connections or non SSL TCP transport

// Server host addressed beginng with SSL:// are assumed to be using SSL authentication. The SSL:// prefix is removed to
// make the connection.

#    include <openssl/asn1.h>
#    include <openssl/crypto.h>
#    include <openssl/err.h>
#    include <openssl/pem.h>
#    include <openssl/ssl.h>
#    include <openssl/x509.h>

#    define VERIFY_DEPTH 4
#    define X509STRINGSIZE 256

#    include <export.h>

#    include <client/udaClientHostList.h>
#    include <clientserver/socketStructs.h>

bool getUdaClientSSLDisabled();
SSL* getUdaClientSSL();
void putUdaClientSSLSocket(int s);
void closeUdaClientSSL();
void putUdaClientSSLProtocol(int specified);
int initUdaClientSSL();
int startUdaClientSSL();
int readUdaClientSSL(void* iohandle, char* buf, int count);
int writeUdaClientSSL(void* iohandle, char* buf, int count);
void putClientHost(const HostData* host);

#  endif // SSLAUTHENTICATION

#endif // UDA_AUTHENTICATION_CLIENT_SSL_H
