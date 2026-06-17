#if defined(SSLAUTHENTICATION) && !defined(SERVERBUILD) && !defined(FATCLIENT)

#include "udaClientSSL.h"
#include "tlsMode.h"

#include <cstdio>
#include <fcntl.h>
#include <initializer_list>
#include <ctime>
#include <openssl/err.h>
#include <openssl/ssl.h>
#include <openssl/x509v3.h>
#include <string>

#include <client/updateSelectParms.h>
#include <clientserver/errorLog.h>
#include <logging/logging.h>
#include <client/udaClientHostList.h>

#include "authLog.h"
#include "utils.h"

static bool g_sslDisabled = true;   // Default state is not SSL authentication
static int g_sslProtocol = 0;       // The default server host name has the SSL protocol name prefix or
static int g_sslSocket = -1;
static bool g_sslOK = false;        // SSL Authentication has been passed sucessfully: default is NOT Passed
static bool g_sslInit = false;      // Global initialisation of SSL completed
static SSL* g_ssl = nullptr;
static SSL_CTX* g_ctx = nullptr;
static const HostData* g_host = nullptr;
static TlsMode g_tlsMode = TlsMode::Off;

static SSL_CTX* create_client_context();
static int load_client_certificate(SSL_CTX* ctx, const HostData* host);
static int load_ca_certificates(SSL_CTX* ctx, const std::string& path);
static int configure_client_tls_server_only(SSL_CTX* ctx, const HostData* host);
static int configure_client_tls_mutual(SSL_CTX* ctx, const HostData* host);
static int connect_tls_connection(SSL_CTX* ctx, PeerCertPolicy policy);

void putClientHost(const HostData* host)
{
    g_host = host;
}

bool getUdaClientSSLDisabled()
{
    return g_sslDisabled;
}

void putUdaClientSSLProtocol(int specified)
{
    g_sslProtocol = specified;
}

void putUdaClientSSLSocket(int s)
{
    g_sslSocket = s;
}

static void init_ssl_library()
{
    if (g_sslInit) {
        return;    // Already initialised
    }
    if (getenv("UDA_SSL_INITIALISED")) {
        g_sslInit = true;
        UDA_LOG(UDA_LOG_DEBUG, "Prior SSL initialisation\n");
        return;
    }
    SSL_library_init();
    SSL_load_error_strings();
    OpenSSL_add_ssl_algorithms();
#ifdef _WIN32
    if (getenv("UDA_SSL_INITIALISED") == nullptr) {
        _putenv_s("UDA_SSL_INITIALISED", "1");
    }
#else
    setenv("UDA_SSL_INITIALISED", "1", 0);
#endif
    g_sslInit = true;
    UDA_LOG(UDA_LOG_DEBUG, "SSL initialised\n");
}

void closeUdaClientSSL()
{
    // Requires re-initialisation
    if (g_sslDisabled) {
        return;
    }
    g_sslOK = false;
    g_sslSocket = -1;
    g_sslProtocol = 0;
    g_sslDisabled = true;
    g_tlsMode = TlsMode::Off;
    SSL* ssl = g_ssl;
    if (ssl != nullptr) {
        SSL_shutdown(ssl);
        SSL_free(ssl);
    }
    SSL_CTX* ctx = g_ctx;
    if (ctx != nullptr) {
        SSL_CTX_free(ctx);
    }
    g_ssl = nullptr;
    g_ctx = nullptr;
#ifdef _WIN32
    _putenv_s("UDA_SSL_INITIALISED", nullptr);
#else
    unsetenv("UDA_SSL_INITIALISED");
#endif
    g_sslInit = false;
    UDA_LOG(UDA_LOG_DEBUG, "SSL closed\n");
}

SSL* getUdaClientSSL()
{
    return g_ssl;
}

void putUdaClientSSLCTX(SSL_CTX* c)
{
    g_ctx = c;
}

static const char* ssl_error_name(int err)
{
    switch (err) {
        case SSL_ERROR_NONE:           return "SSL_ERROR_NONE";
        case SSL_ERROR_ZERO_RETURN:    return "SSL_ERROR_ZERO_RETURN";
        case SSL_ERROR_WANT_READ:      return "SSL_ERROR_WANT_READ";
        case SSL_ERROR_WANT_WRITE:     return "SSL_ERROR_WANT_WRITE";
        case SSL_ERROR_WANT_CONNECT:   return "SSL_ERROR_WANT_CONNECT";
        case SSL_ERROR_WANT_ACCEPT:    return "SSL_ERROR_WANT_ACCEPT";
        case SSL_ERROR_WANT_X509_LOOKUP: return "SSL_ERROR_WANT_X509_LOOKUP";
        case SSL_ERROR_SYSCALL:        return "SSL_ERROR_SYSCALL";
        case SSL_ERROR_SSL:            return "SSL_ERROR_SSL";
        default:                       return "SSL_ERROR_UNKNOWN";
    }
}

void reportSSLErrorCode(int rc)
{
    const int err = SSL_get_error(getUdaClientSSL(), rc);
    const char* msg = ssl_error_name(err);
    UDA_ADD_ERROR(999, msg);
    AUTH_LOG(UDA_LOG_ERROR, "Auth: SSL error: %s\n", msg);

    // Drain the full OpenSSL error queue
    char detail[256];
    unsigned long ossl_err;
    while ((ossl_err = ERR_get_error()) != 0) {
        ERR_error_string_n(ossl_err, detail, sizeof(detail));
        AUTH_LOG(UDA_LOG_ERROR, "Auth: SSL error detail: %s\n", detail);
    }

    if (getUdaClientSSL() != nullptr) {
        AUTH_LOG(UDA_LOG_DEBUG, "Auth: SSL state: %s\n", SSL_state_string(getUdaClientSSL()));
        const long vr = SSL_get_verify_result(getUdaClientSSL());
        if (vr != X509_V_OK) {
            AUTH_LOG(UDA_LOG_ERROR, "Auth: TLS verify result: %s\n",
                     X509_verify_cert_error_string(vr));
        }
    }
}

static const char* first_env(const std::initializer_list<const char*>& names)
{
    for (const char* name : names) {
        const char* value = getenv(name);
        if (value != nullptr && value[0] != '\0') {
            return value;
        }
    }
    return nullptr;
}

static const char* first_env_or_host(const std::initializer_list<const char*>& names, const std::string& host_value)
{
    if (const char* value = first_env(names)) {
        return value;
    }
    return host_value.empty() ? nullptr : host_value.c_str();
}

SSL_CTX* create_client_context()
{
    const SSL_METHOD* method = SSLv23_client_method(); // standard TCP

    // method = DTLSv1_client_method()// reliable UDP

    SSL_CTX* ctx = SSL_CTX_new(method);
    putUdaClientSSLCTX(ctx);

    if (!ctx) {
        UDA_ADD_ERROR(999, "Unable to create SSL context");
        return nullptr;
    }

    // Disable SSLv2 for v3 and TSLv1  negotiation 

    SSL_CTX_set_options(ctx, SSL_OP_NO_SSLv2);

/*
// Set the Cipher List 
   if (SSL_CTX_set_cipher_list(g_ctx, "AES128-SHA") <= 0) {
      printf("Error setting the cipher list.\n");
      exit(0);
   }    
*/

    UDA_LOG(UDA_LOG_DEBUG, "SSL Context created\n");

    return ctx;
}

int load_client_certificate(SSL_CTX* ctx, const HostData* host)
{
    const std::string empty;
    const std::string& host_cert = host == nullptr ? empty : host->certificate;
    const std::string& host_key = host == nullptr ? empty : host->key;

    const char* cert = first_env_or_host({"UDA_CLIENT_TLS_CERT", "UDA_CLIENT_SSL_CERT"}, host_cert);
    const char* key = first_env_or_host({"UDA_CLIENT_TLS_KEY", "UDA_CLIENT_SSL_KEY"}, host_key);

    if (!cert || !key) {
        if (!cert) {
            UDA_LOG(UDA_LOG_DEBUG, "No Client TLS certificate\n");
            UDA_ADD_ERROR(999, "No client TLS certificate; set UDA_CLIENT_TLS_CERT or UDA_CLIENT_SSL_CERT");
        }
        if (!key) {
            UDA_LOG(UDA_LOG_DEBUG, "No Client Private Key\n");
            UDA_ADD_ERROR(999, "No client TLS key; set UDA_CLIENT_TLS_KEY or UDA_CLIENT_SSL_KEY");
        }
        UDA_LOG(UDA_LOG_DEBUG, "Error: No client TLS certificate and/or private key!\n");
        return 999;
    }

    UDA_LOG(UDA_LOG_DEBUG, "Client SSL certificates: %s\n", cert);
    UDA_LOG(UDA_LOG_DEBUG, "Client SSL key: %s\n", key);

    if (SSL_CTX_use_certificate_file(ctx, cert, SSL_FILETYPE_PEM) <= 0) {
        UDA_LOG(UDA_LOG_DEBUG, "Error: Failed to set the client certificate!\n");
        UDA_THROW_ERROR(999, "Failed to set the client certificate!");
    }

    if (SSL_CTX_use_PrivateKey_file(ctx, key, SSL_FILETYPE_PEM) <= 0) {
        UDA_LOG(UDA_LOG_DEBUG, "Error: Failed to set the client key!\n");
        UDA_THROW_ERROR(999, "Failed to set the client key!");
    }

    // Check key and certificate match
    if (SSL_CTX_check_private_key(ctx) == 0) {
        UDA_LOG(UDA_LOG_DEBUG, "Error: Private key does not match the certificate public key!\n");
        UDA_THROW_ERROR(999, "Private key does not match the certificate public key!");
    }

    // validate the client's certificate
    FILE* fd = fopen(cert, "r");

    if (!fd) {
        UDA_LOG(UDA_LOG_DEBUG, "Unable to open client certificate [%s] to verify certificate validity\n", cert);
        UDA_THROW_ERROR(999, "Unable to open client certificate to verify certificate validity!");
    }

    X509* clientCert = PEM_read_X509(fd, nullptr, nullptr, nullptr);

    fclose(fd);

    if (!clientCert) {
        X509_free(clientCert);
        UDA_LOG(UDA_LOG_DEBUG, "Unable to parse client certificate [%s] to verify certificate validity\n", cert);
        UDA_THROW_ERROR(999, "Unable to parse client certificate [%s] to verify certificate validity");
    }

    char work[X509_STRING_SIZE];
    AUTH_LOG(UDA_LOG_DEBUG, "Auth: client cert subject: %s\n",
             X509_NAME_oneline(X509_get_subject_name(clientCert), work, sizeof(work)));
    AUTH_LOG(UDA_LOG_DEBUG, "Auth: client cert issuer: %s\n",
             X509_NAME_oneline(X509_get_issuer_name(clientCert), work, sizeof(work)));

    // Client cert date validation (UDA_CLIENT_TLS_VERIFY_CERT_DATES, default: enabled)
    if (clientTlsVerifyCertDates()) {
        const ASN1_TIME* before = X509_get_notBefore(clientCert);
        const ASN1_TIME* after  = X509_get_notAfter(clientCert);
        time_t current_time = time(nullptr);

        const std::string before_string = to_string(before);
        const std::string after_string  = to_string(after);
        AUTH_LOG(UDA_LOG_DEBUG, "Auth: client cert validity: %s to %s\n",
                 before_string.c_str(), after_string.c_str());

        int rc = 0;
        if ((rc = X509_cmp_time(before, &current_time)) >= 0) {
            X509_free(clientCert);
            AUTH_LOG(UDA_LOG_ERROR,
                "Auth: client cert is not yet valid (notBefore=%s) — "
                "check system clock or set UDA_CLIENT_TLS_VERIFY_CERT_DATES=0\n",
                before_string.c_str());
            UDA_THROW_ERROR(999, "Client TLS certificate validity date is in the future");
        }
        if ((rc = X509_cmp_time(after, &current_time)) <= 0) {
            X509_free(clientCert);
            AUTH_LOG(UDA_LOG_ERROR,
                "Auth: client cert has expired (notAfter=%s) — "
                "renew the client certificate or set UDA_CLIENT_TLS_VERIFY_CERT_DATES=0\n",
                after_string.c_str());
            UDA_THROW_ERROR(999, "Client TLS certificate has expired");
        }
    } else {
        AUTH_LOG(UDA_LOG_WARN,
            "Auth: client cert date validation is DISABLED "
            "(UDA_CLIENT_TLS_VERIFY_CERT_DATES=0)\n");
    }
    X509_free(clientCert);

    return 0;
}

int load_ca_certificates(SSL_CTX* ctx, const std::string& path)
{
    if (path.empty()) {
        UDA_LOG(UDA_LOG_DEBUG, "No CA TLS certificate\n");
        UDA_THROW_ERROR(999, "No Certificate Authority certificate; set UDA_CLIENT_CA_TLS_CERT or UDA_CLIENT_CA_SSL_CERT");
    }

    UDA_LOG(UDA_LOG_DEBUG, "CA SSL certificates: %s\n", path.c_str());

    // Load certificates of trusted CAs based on file provided
    if (SSL_CTX_load_verify_locations(ctx, path.c_str(), nullptr) < 1) {
        UDA_LOG(UDA_LOG_DEBUG, "Error: Error setting the certificate authority verify locations!\n");
        UDA_THROW_ERROR(999, "Error setting the certificate authority verify locations!");
    }

    return 0;
}

int configure_client_tls_server_only(SSL_CTX* ctx, const HostData* host)
{
    const std::string empty;
    const std::string& host_ca = host == nullptr ? empty : host->ca_certificate;
    const char* ca = first_env_or_host({"UDA_CLIENT_CA_TLS_CERT", "UDA_CLIENT_TLS_CA_CERT", "UDA_CLIENT_CA_SSL_CERT"},
                                       host_ca);

    if (load_ca_certificates(ctx, ca == nullptr ? "" : ca) != 0) {
        return 999;
    }
    SSL_CTX_set_verify(ctx, SSL_VERIFY_PEER, nullptr);
    SSL_CTX_set_verify_depth(ctx, VERIFY_DEPTH);

    UDA_LOG(UDA_LOG_DEBUG, "Server-only TLS context configured\n");
    return 0;
}

int configure_client_tls_mutual(SSL_CTX* ctx, const HostData* host)
{
    const std::string empty;
    const std::string& host_ca = host == nullptr ? empty : host->ca_certificate;
    const char* ca = first_env_or_host({"UDA_CLIENT_CA_TLS_CERT", "UDA_CLIENT_TLS_CA_CERT", "UDA_CLIENT_CA_SSL_CERT"},
                                       host_ca);

    if (load_ca_certificates(ctx, ca == nullptr ? "" : ca) != 0 || load_client_certificate(ctx, host) != 0) {
        return 999;
    }
    SSL_CTX_set_verify(ctx, SSL_VERIFY_PEER, nullptr);
    SSL_CTX_set_verify_depth(ctx, VERIFY_DEPTH);

    UDA_LOG(UDA_LOG_DEBUG, "Mutual TLS context configured\n");
    return 0;
}

int initUdaClientSSL()
{
    // Has SSL/TLS authentication already been passed?
    if (g_sslOK) {
        return 0;
    }

    if (!isValidTlsModeEnv("UDA_CLIENT_TLS_MODE")) {
        getClientTlsMode();
        UDA_THROW_ERROR(999, "Invalid client TLS mode!");
    }

    TlsMode mode = getClientTlsMode();

    // Preserve legacy behaviour: SSL:// and SSL host config imply mutual TLS unless the new mode var overrides them.
    if (!isTlsModeSet("UDA_CLIENT_TLS_MODE") && mode == TlsMode::Off &&
        (g_sslProtocol || (g_host != nullptr && g_host->isSSL))) {
        mode = TlsMode::Mutual;
    }

    g_tlsMode = mode;
    AUTH_LOG(UDA_LOG_INFO, "Client TLS mode: %s\n", tlsModeStr(mode));
    g_sslDisabled = mode == TlsMode::Off;
    if (g_sslDisabled) {
        return 0;
    }

    AUTH_LOG(UDA_LOG_INFO, "Client TLS enabled\n");

    // Initialise

    init_ssl_library();

    if (!(g_ctx = create_client_context())) {
        UDA_THROW_ERROR(999, "Unable to create the SSL context!");
    }

    switch (mode) {
        case TlsMode::Off:
            return 0;
        case TlsMode::ServerOnly:
            if (configure_client_tls_server_only(g_ctx, g_host) != 0) {
                UDA_THROW_ERROR(999, "Unable to configure the server-only TLS context!");
            }
            return 0;
        case TlsMode::Mutual:
            if (configure_client_tls_mutual(g_ctx, g_host) != 0) {
                UDA_THROW_ERROR(999, "Unable to configure the mutual TLS context!");
            }
            return 0;
    }

    UDA_THROW_ERROR(999, "Unsupported client TLS mode!");
}

int connect_tls_connection(SSL_CTX* ctx, PeerCertPolicy policy)
{
    AUTH_LOG(UDA_LOG_DEBUG, "Initiating TLS connect (peer cert policy: %s)\n",
             policy == PeerCertPolicy::Required ? "required" : "not-required");

    g_ssl = SSL_new(ctx);
    if (g_ssl == nullptr) {
        AUTH_LOG(UDA_LOG_ERROR, "SSL_new failed — cannot create SSL object\n");
        UDA_THROW_ERROR(999, "SSL_new failed");
    }

    SSL_set_fd(g_ssl, g_sslSocket);

    // Hostname verification (UDA_CLIENT_TLS_VERIFY_HOSTNAME, default: enabled).
    // Applies to server-only and mutual TLS when the peer is identified by hostname.
    const std::string server_hostname = (g_host != nullptr) ? g_host->host_name : "";
    if (!server_hostname.empty()) {
        // Set SNI extension so the server can select the right certificate
        SSL_set_tlsext_host_name(g_ssl, server_hostname.c_str());

        if (clientTlsVerifyHostname()) {
            AUTH_LOG(UDA_LOG_DEBUG, "Auth: TLS hostname verification enabled for '%s'\n",
                     server_hostname.c_str());
            SSL_set_hostflags(g_ssl, X509_CHECK_FLAG_NO_PARTIAL_WILDCARDS);
            if (SSL_set1_host(g_ssl, server_hostname.c_str()) != 1) {
                AUTH_LOG(UDA_LOG_ERROR,
                         "Auth: failed to set hostname '%s' for TLS verification\n",
                         server_hostname.c_str());
                UDA_THROW_ERROR(999, "Failed to configure TLS hostname verification");
            }
        } else {
            AUTH_LOG(UDA_LOG_WARN,
                "Auth: TLS hostname verification is DISABLED "
                "(UDA_CLIENT_TLS_VERIFY_HOSTNAME=0) for host '%s' — "
                "this weakens security; enable for production use\n",
                server_hostname.c_str());
        }
    } else {
        // No hostname available (g_host is null or host_name is empty).
        // This can happen for connections made without a host config entry.
        AUTH_LOG(UDA_LOG_DEBUG,
            "Auth: TLS hostname verification skipped — server hostname is not known at connect time\n");
    }

    // Connect to the server
    int rc;
    if ((rc = SSL_connect(g_ssl)) < 1) {
        const int ssl_err = SSL_get_error(g_ssl, rc);

        // Drain the OpenSSL error queue for detailed diagnostics
        char ssl_err_buf[256];
        unsigned long ossl_err;
        while ((ossl_err = ERR_get_error()) != 0) {
            ERR_error_string_n(ossl_err, ssl_err_buf, sizeof(ssl_err_buf));
            AUTH_LOG(UDA_LOG_ERROR, "Auth: TLS error detail: %s\n", ssl_err_buf);
        }

        // Classify common failure modes
        const long verify_result = SSL_get_verify_result(g_ssl);
        if (verify_result != X509_V_OK) {
            const char* reason = X509_verify_cert_error_string(verify_result);
            AUTH_LOG(UDA_LOG_ERROR,
                "Auth: TLS certificate verification failed: %s\n"
                "  mode=%s host='%s' — check UDA_CLIENT_CA_TLS_CERT and UDA_CLIENT_TLS_MODE\n",
                reason, tlsModeStr(g_tlsMode), server_hostname.c_str());

            // Special-case: hostname mismatch gets its own message
            if (verify_result == X509_V_ERR_HOSTNAME_MISMATCH) {
                AUTH_LOG(UDA_LOG_ERROR,
                    "Auth: TLS hostname verification failed: certificate does not match "
                    "requested host '%s' — "
                    "set UDA_CLIENT_TLS_VERIFY_HOSTNAME=0 only if you understand the risk\n",
                    server_hostname.c_str());
            }
        } else if (ssl_err == SSL_ERROR_SYSCALL) {
            AUTH_LOG(UDA_LOG_ERROR,
                "Auth: TLS connect failed (SSL_ERROR_SYSCALL): connection was closed by the "
                "server before the handshake completed\n"
                "  mode=%s — possible causes:\n"
                "    * server was not compiled with SSLAUTHENTICATION\n"
                "    * UDA_SERVER_TLS_MODE is not set or is 'off' on the server\n"
                "    * client is connecting to a plaintext server with TLS\n"
                "  check UDA_CLIENT_TLS_MODE and UDA_SERVER_TLS_MODE\n",
                tlsModeStr(g_tlsMode));
        } else {
            AUTH_LOG(UDA_LOG_ERROR,
                "Auth: TLS connect failed (ssl_err=%d mode=%s host='%s')\n",
                ssl_err, tlsModeStr(g_tlsMode), server_hostname.c_str());
        }

        if (errno != 0) {
            UDA_ADD_SYS_ERROR("Error connecting to the server!");
        }
        reportSSLErrorCode(rc);
        return 999;
    }

    AUTH_LOG(UDA_LOG_INFO, "Auth: TLS connect succeeded (version=%s cipher=%s host='%s')\n",
             SSL_get_version(g_ssl), SSL_get_cipher(g_ssl), server_hostname.c_str());

    // Get the Server certificate and verify/log according to mode.
    X509* peer = SSL_get_peer_certificate(g_ssl);

    if (peer != nullptr) {

        if ((rc = SSL_get_verify_result(g_ssl)) != X509_V_OK) {
            // returns X509_V_OK if the certificate was not obtained as no error occurred!
            AUTH_LOG(UDA_LOG_ERROR, "Auth: server cert verification failed: %s\n",
                     X509_verify_cert_error_string(rc));
            UDA_ADD_ERROR(999, X509_verify_cert_error_string(rc));
            X509_free(peer);
            UDA_THROW_ERROR(999, "SSL Server certificate presented but verification error!");
        }

        char work[X509_STRING_SIZE];
        AUTH_LOG(UDA_LOG_INFO, "Auth: server cert verified — subject: %s\n",
                 X509_NAME_oneline(X509_get_subject_name(peer), work, sizeof(work)));
        AUTH_LOG(UDA_LOG_DEBUG, "Auth: server cert issuer: %s\n",
                 X509_NAME_oneline(X509_get_issuer_name(peer), work, sizeof(work)));

        // Cert date validation (UDA_CLIENT_TLS_VERIFY_CERT_DATES, default: enabled)
        if (clientTlsVerifyCertDates()) {
            const ASN1_TIME* before = X509_get_notBefore(peer);
            const ASN1_TIME* after  = X509_get_notAfter(peer);
            time_t current_time = time(nullptr);

            const std::string before_string = to_string(before);
            const std::string after_string  = to_string(after);
            AUTH_LOG(UDA_LOG_DEBUG, "Auth: server cert validity: %s to %s\n",
                     before_string.c_str(), after_string.c_str());

            if ((rc = X509_cmp_time(before, &current_time)) >= 0) {
                X509_free(peer);
                AUTH_LOG(UDA_LOG_ERROR,
                    "Auth: server cert is not yet valid (notBefore=%s) — "
                    "check system clock or set UDA_CLIENT_TLS_VERIFY_CERT_DATES=0\n",
                    before_string.c_str());
                UDA_THROW_ERROR(999, "Server TLS certificate validity date is in the future");
            }
            if ((rc = X509_cmp_time(after, &current_time)) <= 0) {
                X509_free(peer);
                AUTH_LOG(UDA_LOG_ERROR,
                    "Auth: server cert has expired (notAfter=%s) — "
                    "renew the server certificate or set UDA_CLIENT_TLS_VERIFY_CERT_DATES=0\n",
                    after_string.c_str());
                UDA_THROW_ERROR(999, "Server TLS certificate has expired");
            }
        } else {
            AUTH_LOG(UDA_LOG_WARN,
                "Auth: server cert date validation is DISABLED "
                "(UDA_CLIENT_TLS_VERIFY_CERT_DATES=0) — "
                "this weakens security; enable for production use\n");
        }

        X509_free(peer);

    } else {
        if (policy == PeerCertPolicy::Required) {
            AUTH_LOG(UDA_LOG_ERROR, "Auth: server cert required but not presented\n");
            UDA_THROW_ERROR(999, "Server certificate not presented for verification!");
        }
        AUTH_LOG(UDA_LOG_DEBUG, "Auth: no server cert presented; not required in this TLS mode\n");
    }

    // SSL/TLS authentication has been passed - do not repeat

    g_sslOK = true;

    return 0;
}

int startUdaClientSSL()
{
    if (g_sslDisabled) {
        return 0;
    }

    const PeerCertPolicy policy = g_tlsMode == TlsMode::Mutual ? PeerCertPolicy::Required : PeerCertPolicy::NotRequired;
    return connect_tls_connection(g_ctx, policy);
}

int writeUdaClientSSL(void* iohandle, char* buf, int count)
{
    // This routine is only called when there is something to write to the Server
    // SSL uses an all or nothing approach when the socket is blocking - an SSL error or incomplete write
    // means the write has failed

    int rc, err = 0;

    fd_set wfds;        // File Descriptor Set for Writing to the Socket
    struct timeval tv;

    // Block till it's possible to write to the socket or timeout

    udaUpdateSelectParms(g_sslSocket, &wfds, &tv);

    while ((rc = select(g_sslSocket + 1, nullptr, &wfds, nullptr, &tv)) <= 0) {
        if (rc < 0) {    // Error
            if (errno == EBADF) {
                UDA_LOG(UDA_LOG_DEBUG, "Socket is closed! Data access failed!.\n");
            } else {
                UDA_LOG(UDA_LOG_DEBUG, "Read error - %s\n", strerror(errno));
            }
            return -1;
        }

#ifndef _WIN32
        int fopts = 0;
        if ((rc = fcntl(g_sslSocket, F_GETFL, &fopts)) < 0 || errno == EBADF) {
            // Is the socket closed? Check status flags
            err = 999;
            UDA_LOG(UDA_LOG_DEBUG, "Socket is closed!\n");
            return -1;
        }
#endif

        udaUpdateSelectParms(g_sslSocket, &wfds, &tv);
    }

    // set SSL_MODE_AUTO_RETRY flag of the SSL_CTX_set_mode to disable automatic renegotiation?

    rc = SSL_write(getUdaClientSSL(), buf, count);

    switch (SSL_get_error(getUdaClientSSL(), rc)) {
        case SSL_ERROR_NONE:
            if (rc != count) {    // Check the write is complete
                err = 999;
                UDA_LOG(UDA_LOG_DEBUG, "Incomplete write to socket!\n");
                addIdamError(UDA_CODE_ERROR_TYPE, "writeUdaClientSSL", err, "Incomplete write to socket!");
                return -1;
            }
            break;

        default:
            reportSSLErrorCode(rc);
            err = 999;
            UDA_LOG(UDA_LOG_DEBUG, "Write to socket failed!\n");
            addIdamError(UDA_CODE_ERROR_TYPE, "writeUdaClientSSL", err, "Write to socket failed!");
#ifndef _WIN32
            int fopts = 0;
            if ((rc = fcntl(g_sslSocket, F_GETFL, &fopts)) < 0 ||
                errno == EBADF) {    // Is the socket closed? Check status flags
                UDA_LOG(UDA_LOG_DEBUG, "Socket is closed!\n");
            }
#endif
            return -1;
    }

    return rc;
}

int readUdaClientSSL(void* iohandle, char* buf, int count)
{
    int rc, err = 0;
    fd_set rfds;
    struct timeval tv;

    int maxloop = 0;

    // Wait till it's possible to read from socket 

    udaUpdateSelectParms(g_sslSocket, &rfds, &tv);

    while (((rc = select(g_sslSocket + 1, &rfds, nullptr, nullptr, &tv)) <= 0)
            && maxloop++ < MAXLOOP) {

        if (rc < 0) {    // Error
            int serrno = errno;
            addIdamError(UDA_SYSTEM_ERROR_TYPE, "readUdaClientSSL", errno, "Socket is Closed!");
            if (serrno == EBADF) {
                UDA_LOG(UDA_LOG_DEBUG, "Socket is closed!\n");
            } else {
                UDA_LOG(UDA_LOG_DEBUG, "Read error - %s\n", strerror(serrno));
            }
            err = 999;
            addIdamError(UDA_CODE_ERROR_TYPE, "readUdaClientSSL", err,
                         "Socket is Closed! Data request failed. Restarting connection.");
            UDA_LOG(UDA_LOG_DEBUG,
                    "Socket is Closed! Data request failed. Restarting connection.\n");
            return -1;
        }
#ifndef _WIN32
        int fopts = 0;
        if ((rc = fcntl(g_sslSocket, F_GETFL, &fopts)) < 0 ||
            errno == EBADF) {    // Is the socket closed? Check status flags
            err = 999;
            UDA_LOG(UDA_LOG_DEBUG, "Socket is closed!\n");
            return -1;
        }
#endif

        udaUpdateSelectParms(g_sslSocket, &rfds, &tv);        // Keep blocking and wait for data
    }

// First byte of encrypted data received but need the full record in buffer before SSL can decrypt

    int blocked;
    do {
        blocked = 0;
        rc = SSL_read(getUdaClientSSL(), buf, count);

        switch (SSL_get_error(getUdaClientSSL(), rc)) {    // check for SSL errors
            case SSL_ERROR_NONE:                // clean read
                break;

            case SSL_ERROR_ZERO_RETURN:    // connection closed by server (not caught by select?)
                reportSSLErrorCode(rc);
                err = 999;
                UDA_LOG(UDA_LOG_DEBUG, "Server socket connection closed!\n");
                addIdamError(UDA_CODE_ERROR_TYPE, "readUdaClientSSL", err,
                             "Server socket connection closed!");
                return -1;

            case SSL_ERROR_WANT_READ:    // the operation did not complete, try again
                blocked = 1;
                break;

            case SSL_ERROR_WANT_WRITE:    //the operation did not complete, error
                reportSSLErrorCode(rc);
                err = 999;
                UDA_LOG(UDA_LOG_DEBUG, "A read operation failed!\n");
                addIdamError(UDA_CODE_ERROR_TYPE, "readUdaClientSSL", err, "A read operation failed!");
                return -1;

            case SSL_ERROR_SYSCALL:    //some I/O error occured - disconnect?
                reportSSLErrorCode(rc);
                err = 999;
                UDA_LOG(UDA_LOG_DEBUG, "Socket read I/O error!\n");
                addIdamError(UDA_CODE_ERROR_TYPE, "readUdaClientSSL", err, "Socket read I/O error!");
                return -1;

            default:            //some other error
                reportSSLErrorCode(rc);
                err = 999;
                UDA_LOG(UDA_LOG_DEBUG, "Read from socket failed!\n");
                addIdamError(UDA_CODE_ERROR_TYPE, "readUdaClientSSL", err, "Read from socket failed!");
#ifndef _WIN32
                int fopts = 0;
                if ((rc = fcntl(g_sslSocket, F_GETFL, &fopts)) < 0 ||
                    errno == EBADF) {    // Is the socket closed? Check status flags
                    UDA_LOG(UDA_LOG_DEBUG, "Socket is closed!\n");
                }
#endif
                return -1;
        }

    } while (SSL_pending(getUdaClientSSL()) && !blocked);    // data remaining in buffer or re-read attempt

    return rc;
}

#endif   // !SERVERBUILD && SSLAUTHENTICATION 
