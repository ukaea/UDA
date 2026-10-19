#pragma once

#ifndef UDA_AUTHENTICATION_CLIENT_SSL_H
#define UDA_AUTHENTICATION_CLIENT_SSL_H

#if defined(SSLAUTHENTICATION) && !defined(FATCLIENT)

// Create the SSL context and binding to the socket
// 3 UDA protocol modes: TCP without SSL/TLS, TCP and UDP both with SSL/TLS
// This set of functions is concerned only with the SSL/TLS protocol (authentication and encryption) - not with
// establishing socket connections or non SSL TCP transport

// Server host addressed beginng with SSL:// are assumed to be using SSL authentication. The SSL:// prefix is removed to
// make the connection.

#include <openssl/asn1.h>
#include <openssl/crypto.h>
#include <openssl/err.h>
#include <openssl/pem.h>
#include <openssl/ssl.h>
#include <openssl/x509.h>

#define VERIFY_DEPTH 4

#include <memory>
#include <string>
#include "tlsMode.h"

#include <client/udaClientHostList.h>
#include <clientserver/export.h>
#include <clientserver/socketStructs.h>

struct EvpPkeyDeleter { void operator()(EVP_PKEY* p) const noexcept { if (p) EVP_PKEY_free(p); } };
struct SslDeleter     { void operator()(SSL* p)      const noexcept { if (p) SSL_free(p); } };
struct SslCtxDeleter  { void operator()(SSL_CTX* p)  const noexcept { if (p) SSL_CTX_free(p); } };

using SslPtr    = std::unique_ptr<SSL,     SslDeleter>;
using SslCtxPtr = std::unique_ptr<SSL_CTX, SslCtxDeleter>;

struct HostData;  // forward declaration if not already visible

struct ClientSslState {
    bool        ssl_disabled        = true;
    int         ssl_protocol        = 0;
    int         ssl_socket          = -1;
    bool        ssl_ok              = false;
    bool        ssl_init            = false;
    SslPtr      ssl;
    SslCtxPtr   ctx;
    const HostData* host            = nullptr;
    TlsMode     tls_mode            = TlsMode::Off;
    std::string connected_hostname;
};

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

// Set the resolved hostname of the server we are about to connect to.
// Called by connection.cpp with the final hostname before SSL handshake.
// This ensures hostname verification works even when no host-list entry exists.
void putClientHostname(const std::string& hostname);

#endif // SSLAUTHENTICATION

#endif // UDA_AUTHENTICATION_CLIENT_SSL_H
