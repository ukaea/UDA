#if defined(SSLAUTHENTICATION) && !defined(SERVERBUILD) && !defined(FATCLIENT)

#include "udaClientSSL.h"
#include "auth_error.h"
#include "tlsMode.h"
#include "tls_policy.h"

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

static ClientSslState g_state;

using uda::authentication::AuthError;
using uda::authentication::AuthErrorCode;
using uda::authentication::authErrorToUdaCode;

static SslCtxPtr create_client_context();
static void load_client_certificate(SSL_CTX* ctx, const HostData* host);
static void load_ca_certificates(SSL_CTX* ctx, const std::string& path);
static void configure_client_tls_server_only(SSL_CTX* ctx, const HostData* host);
static void configure_client_tls_mutual(SSL_CTX* ctx, const HostData* host);
static void connect_tls_connection(SSL_CTX* ctx, PeerCertPolicy policy);

void putClientHost(const HostData* host)
{
    g_state.host = host;
}

void putClientHostname(const std::string& hostname)
{
    g_state.connected_hostname = hostname;
}

bool getUdaClientSSLDisabled()
{
    return g_state.ssl_disabled;
}

void putUdaClientSSLProtocol(int specified)
{
    g_state.ssl_protocol = specified;
}

void putUdaClientSSLSocket(int s)
{
    g_state.ssl_socket = s;
}

static void init_ssl_library()
{
    if (g_state.ssl_init) {
        return;
    }
    if (getenv("UDA_SSL_INITIALISED")) {
        g_state.ssl_init = true;
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
    g_state.ssl_init = true;
    UDA_LOG(UDA_LOG_DEBUG, "SSL initialised\n");
}

void closeUdaClientSSL()
{
    if (g_state.ssl_disabled) {
        return;
    }
    if (g_state.ssl) SSL_shutdown(g_state.ssl.get());
#ifdef _WIN32
    _putenv_s("UDA_SSL_INITIALISED", nullptr);
#else
    unsetenv("UDA_SSL_INITIALISED");
#endif
    g_state = ClientSslState{};
    UDA_LOG(UDA_LOG_DEBUG, "SSL closed\n");
}

SSL* getUdaClientSSL()
{
    return g_state.ssl.get();
}

static const char* ssl_error_name(int err)
{
    switch (err) {
        case SSL_ERROR_NONE:             return "SSL_ERROR_NONE";
        case SSL_ERROR_ZERO_RETURN:      return "SSL_ERROR_ZERO_RETURN";
        case SSL_ERROR_WANT_READ:        return "SSL_ERROR_WANT_READ";
        case SSL_ERROR_WANT_WRITE:       return "SSL_ERROR_WANT_WRITE";
        case SSL_ERROR_WANT_CONNECT:     return "SSL_ERROR_WANT_CONNECT";
        case SSL_ERROR_WANT_ACCEPT:      return "SSL_ERROR_WANT_ACCEPT";
        case SSL_ERROR_WANT_X509_LOOKUP: return "SSL_ERROR_WANT_X509_LOOKUP";
        case SSL_ERROR_SYSCALL:          return "SSL_ERROR_SYSCALL";
        case SSL_ERROR_SSL:              return "SSL_ERROR_SSL";
        default:                         return "SSL_ERROR_UNKNOWN";
    }
}

void reportSSLErrorCode(int rc)
{
    const int err = SSL_get_error(getUdaClientSSL(), rc);
    const char* msg = ssl_error_name(err);
    UDA_ADD_ERROR(999, msg);
    AUTH_LOG(UDA_LOG_ERROR, "Auth: SSL error: %s\n", msg);

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

static const char* first_env_or_host(const std::initializer_list<const char*>& names,
                                     const std::string& host_value)
{
    if (const char* value = first_env(names)) {
        return value;
    }
    return host_value.empty() ? nullptr : host_value.c_str();
}

// -------------------------------------------------------------------------
// Internal TLS setup — throw AuthError; no UDA error stack manipulation.
// Translation to UDA error codes happens only in the boundary functions
// (initUdaClientSSL, startUdaClientSSL).

static SslCtxPtr create_client_context()
{
    const SSL_METHOD* method = SSLv23_client_method();
    SslCtxPtr ctx(SSL_CTX_new(method));
    if (!ctx) {
        throw AuthError(AuthErrorCode::TlsConfigError, "Unable to create SSL context");
    }
    SSL_CTX_set_options(ctx.get(), SSL_OP_NO_SSLv2);
    UDA_LOG(UDA_LOG_DEBUG, "SSL Context created\n");
    return ctx;
}

static void load_client_certificate(SSL_CTX* ctx, const HostData* host)
{
    const std::string empty;
    const std::string& host_cert = host == nullptr ? empty : host->certificate;
    const std::string& host_key  = host == nullptr ? empty : host->key;

    const char* cert = first_env_or_host({"UDA_CLIENT_TLS_CERT", "UDA_CLIENT_SSL_CERT"}, host_cert);
    const char* key  = first_env_or_host({"UDA_CLIENT_TLS_KEY",  "UDA_CLIENT_SSL_KEY"},  host_key);

    if (!cert) {
        throw AuthError(AuthErrorCode::TlsConfigError,
            "No client TLS certificate; set UDA_CLIENT_TLS_CERT or UDA_CLIENT_SSL_CERT");
    }
    if (!key) {
        throw AuthError(AuthErrorCode::TlsConfigError,
            "No client TLS key; set UDA_CLIENT_TLS_KEY or UDA_CLIENT_SSL_KEY");
    }

    UDA_LOG(UDA_LOG_DEBUG, "Client SSL certificates: %s\n", cert);
    UDA_LOG(UDA_LOG_DEBUG, "Client SSL key: %s\n", key);

    if (SSL_CTX_use_certificate_file(ctx, cert, SSL_FILETYPE_PEM) <= 0) {
        throw AuthError(AuthErrorCode::TlsConfigError, "Failed to load client certificate");
    }
    if (SSL_CTX_use_PrivateKey_file(ctx, key, SSL_FILETYPE_PEM) <= 0) {
        throw AuthError(AuthErrorCode::TlsConfigError, "Failed to load client private key");
    }
    if (SSL_CTX_check_private_key(ctx) == 0) {
        throw AuthError(AuthErrorCode::TlsConfigError,
            "Client private key does not match the certificate public key");
    }

    // Read the cert to check validity dates and log subject/issuer.
    // This provides a clear early failure before the server rejects the cert at handshake time.
    FILE* fd = fopen(cert, "r");
    if (!fd) {
        throw AuthError(AuthErrorCode::TlsConfigError,
            std::string("Unable to open client certificate to check validity: ") + cert);
    }
    X509* client_cert = PEM_read_X509(fd, nullptr, nullptr, nullptr);
    fclose(fd);
    if (!client_cert) {
        throw AuthError(AuthErrorCode::TlsConfigError,
            std::string("Unable to parse client certificate: ") + cert);
    }

    char work[X509_STRING_SIZE];
    AUTH_LOG(UDA_LOG_DEBUG, "Auth: client cert subject: %s\n",
             X509_NAME_oneline(X509_get_subject_name(client_cert), work, sizeof(work)));
    AUTH_LOG(UDA_LOG_DEBUG, "Auth: client cert issuer: %s\n",
             X509_NAME_oneline(X509_get_issuer_name(client_cert), work, sizeof(work)));

    const ASN1_TIME* before = X509_get_notBefore(client_cert);
    const ASN1_TIME* after  = X509_get_notAfter(client_cert);
    time_t now = time(nullptr);

    const std::string before_str = to_string(before);
    const std::string after_str  = to_string(after);
    AUTH_LOG(UDA_LOG_DEBUG, "Auth: client cert validity: %s to %s\n",
             before_str.c_str(), after_str.c_str());

    if (X509_cmp_time(before, &now) >= 0) {
        X509_free(client_cert);
        AUTH_LOG(UDA_LOG_ERROR,
            "Auth: client cert is not yet valid (notBefore=%s)\n", before_str.c_str());
        throw AuthError(AuthErrorCode::TlsConfigError,
            "Client TLS certificate validity date is in the future (notBefore=" + before_str + ")");
    }
    if (X509_cmp_time(after, &now) <= 0) {
        X509_free(client_cert);
        AUTH_LOG(UDA_LOG_ERROR,
            "Auth: client cert has expired (notAfter=%s)\n", after_str.c_str());
        throw AuthError(AuthErrorCode::TlsConfigError,
            "Client TLS certificate has expired (notAfter=" + after_str + ")");
    }
    X509_free(client_cert);
}

static void load_ca_certificates(SSL_CTX* ctx, const std::string& path)
{
    if (path.empty()) {
        throw AuthError(AuthErrorCode::TlsConfigError,
            "No CA certificate path; set UDA_CLIENT_CA_TLS_CERT or UDA_CLIENT_CA_SSL_CERT");
    }
    UDA_LOG(UDA_LOG_DEBUG, "CA SSL certificates: %s\n", path.c_str());
    if (SSL_CTX_load_verify_locations(ctx, path.c_str(), nullptr) < 1) {
        throw AuthError(AuthErrorCode::TlsConfigError,
            "Failed to load CA certificates from: " + path);
    }
}

static void configure_client_tls_server_only(SSL_CTX* ctx, const HostData* host)
{
    const std::string empty;
    const std::string& host_ca = host == nullptr ? empty : host->ca_certificate;
    const char* ca = first_env_or_host(
        {"UDA_CLIENT_CA_TLS_CERT", "UDA_CLIENT_TLS_CA_CERT", "UDA_CLIENT_CA_SSL_CERT"}, host_ca);

    load_ca_certificates(ctx, ca == nullptr ? "" : ca);
    SSL_CTX_set_verify(ctx, SSL_VERIFY_PEER, nullptr);
    SSL_CTX_set_verify_depth(ctx, VERIFY_DEPTH);
    UDA_LOG(UDA_LOG_DEBUG, "Server-only TLS context configured\n");
}

static void configure_client_tls_mutual(SSL_CTX* ctx, const HostData* host)
{
    const std::string empty;
    const std::string& host_ca = host == nullptr ? empty : host->ca_certificate;
    const char* ca = first_env_or_host(
        {"UDA_CLIENT_CA_TLS_CERT", "UDA_CLIENT_TLS_CA_CERT", "UDA_CLIENT_CA_SSL_CERT"}, host_ca);

    load_ca_certificates(ctx, ca == nullptr ? "" : ca);
    load_client_certificate(ctx, host);
    SSL_CTX_set_verify(ctx, SSL_VERIFY_PEER, nullptr);
    SSL_CTX_set_verify_depth(ctx, VERIFY_DEPTH);
    UDA_LOG(UDA_LOG_DEBUG, "Mutual TLS context configured\n");
}

static void connect_tls_connection(SSL_CTX* ctx, PeerCertPolicy policy)
{
    AUTH_LOG(UDA_LOG_DEBUG, "Initiating TLS connect (peer cert policy: %s)\n",
             policy == PeerCertPolicy::Required ? "required" : "not-required");

    g_state.ssl.reset(SSL_new(ctx));
    if (g_state.ssl == nullptr) {
        throw AuthError(AuthErrorCode::TlsHandshakeFailed, "SSL_new failed");
    }

    SSL_set_fd(g_state.ssl.get(), g_state.ssl_socket);

    const std::string server_hostname = uda::authentication::select_verification_hostname(
        g_state.connected_hostname,
        g_state.host != nullptr ? g_state.host->host_name : "");

    if (!server_hostname.empty()) {
        SSL_set_tlsext_host_name(g_state.ssl.get(), server_hostname.c_str());

        if (clientTlsVerifyHostname()) {
            AUTH_LOG(UDA_LOG_DEBUG, "Auth: TLS hostname verification enabled for '%s'\n",
                     server_hostname.c_str());
            SSL_set_hostflags(g_state.ssl.get(), X509_CHECK_FLAG_NO_PARTIAL_WILDCARDS);
            if (SSL_set1_host(g_state.ssl.get(), server_hostname.c_str()) != 1) {
                throw AuthError(AuthErrorCode::TlsConfigError,
                    "Failed to configure TLS hostname verification for '" + server_hostname + "'");
            }
        } else {
            AUTH_LOG(UDA_LOG_WARN,
                "Auth: TLS hostname verification is DISABLED "
                "(UDA_CLIENT_TLS_VERIFY_HOSTNAME=0) for host '%s'\n",
                server_hostname.c_str());
        }
    } else {
        AUTH_LOG(UDA_LOG_DEBUG,
            "Auth: TLS hostname verification skipped — server hostname unknown at connect time\n");
    }

    int rc;
    if ((rc = SSL_connect(g_state.ssl.get())) < 1) {
        const int ssl_err = SSL_get_error(g_state.ssl.get(), rc);

        // Drain OpenSSL error queue for diagnostics
        char ssl_err_buf[256];
        unsigned long ossl_err;
        while ((ossl_err = ERR_get_error()) != 0) {
            ERR_error_string_n(ossl_err, ssl_err_buf, sizeof(ssl_err_buf));
            AUTH_LOG(UDA_LOG_ERROR, "Auth: TLS error detail: %s\n", ssl_err_buf);
        }

        if (g_state.ssl != nullptr) {
            AUTH_LOG(UDA_LOG_DEBUG, "Auth: SSL state: %s\n", SSL_state_string(g_state.ssl.get()));
        }

        const long vr = SSL_get_verify_result(g_state.ssl.get());
        if (vr != X509_V_OK) {
            const char* reason = X509_verify_cert_error_string(vr);
            AUTH_LOG(UDA_LOG_ERROR,
                "Auth: TLS certificate verification failed: %s (mode=%s host='%s')\n",
                reason, tlsModeStr(g_state.tls_mode), server_hostname.c_str());

            if (vr == X509_V_ERR_HOSTNAME_MISMATCH) {
                throw AuthError(AuthErrorCode::TlsHostnameMismatch,
                    std::string("TLS hostname verification failed — certificate does not match '")
                    + server_hostname + "': " + reason);
            }
            throw AuthError(AuthErrorCode::TlsHandshakeFailed,
                std::string("TLS certificate verification failed: ") + reason);
        }

        if (ssl_err == SSL_ERROR_SYSCALL) {
            AUTH_LOG(UDA_LOG_ERROR,
                "Auth: TLS connect failed (SSL_ERROR_SYSCALL): connection closed by server "
                "before handshake (mode=%s) — check UDA_CLIENT_TLS_MODE / UDA_SERVER_TLS_MODE\n",
                tlsModeStr(g_state.tls_mode));
        } else {
            AUTH_LOG(UDA_LOG_ERROR,
                "Auth: TLS connect failed (ssl_err=%s mode=%s host='%s')\n",
                ssl_error_name(ssl_err), tlsModeStr(g_state.tls_mode), server_hostname.c_str());
        }

        if (errno != 0) {
            AUTH_LOG(UDA_LOG_ERROR, "Auth: system error: %s\n", strerror(errno));
        }

        throw AuthError(AuthErrorCode::TlsHandshakeFailed,
            std::string("TLS connect failed (") + ssl_error_name(ssl_err) + ")");
    }

    AUTH_LOG(UDA_LOG_INFO, "Auth: TLS connect succeeded (version=%s cipher=%s host='%s')\n",
             SSL_get_version(g_state.ssl.get()), SSL_get_cipher(g_state.ssl.get()), server_hostname.c_str());

    X509* peer = SSL_get_peer_certificate(g_state.ssl.get());

    if (peer != nullptr) {
        if ((rc = SSL_get_verify_result(g_state.ssl.get())) != X509_V_OK) {
            const char* reason = X509_verify_cert_error_string(rc);
            AUTH_LOG(UDA_LOG_ERROR, "Auth: server cert verification failed: %s\n", reason);
            X509_free(peer);
            throw AuthError(AuthErrorCode::TlsHandshakeFailed,
                std::string("Server certificate verification failed: ") + reason);
        }

        char work[X509_STRING_SIZE];
        AUTH_LOG(UDA_LOG_INFO, "Auth: server cert verified — subject: %s\n",
                 X509_NAME_oneline(X509_get_subject_name(peer), work, sizeof(work)));
        AUTH_LOG(UDA_LOG_DEBUG, "Auth: server cert issuer: %s\n",
                 X509_NAME_oneline(X509_get_issuer_name(peer), work, sizeof(work)));

        // Date verification is performed by OpenSSL during SSL_connect() above.
        // Log dates for diagnostics only.
        const ASN1_TIME* before = X509_get_notBefore(peer);
        const ASN1_TIME* after  = X509_get_notAfter(peer);
        AUTH_LOG(UDA_LOG_DEBUG, "Auth: server cert validity: %s to %s\n",
                 to_string(before).c_str(), to_string(after).c_str());

        X509_free(peer);
    } else {
        if (policy == PeerCertPolicy::Required) {
            throw AuthError(AuthErrorCode::TlsHandshakeFailed,
                "Server certificate not presented for verification");
        }
        AUTH_LOG(UDA_LOG_DEBUG, "Auth: no server cert presented; not required in this TLS mode\n");
    }

    g_state.ssl_ok = true;
}

// -------------------------------------------------------------------------
// Boundary functions — catch AuthError, translate to UDA error stack, return stable code

int initUdaClientSSL()
{
    if (g_state.ssl_ok) {
        return 0;
    }

    try {
        if (!isValidTlsModeEnv("UDA_CLIENT_TLS_MODE")) {
            getClientTlsMode();
            throw AuthError(AuthErrorCode::TlsConfigError, "Invalid client TLS mode");
        }

        TlsMode mode = getClientTlsMode();

        mode = uda::authentication::resolve_client_tls_mode(
            mode,
            isTlsModeSet("UDA_CLIENT_TLS_MODE"),
            g_state.ssl_protocol != 0,
            g_state.host != nullptr && g_state.host->isSSL);

        g_state.tls_mode = mode;
        AUTH_LOG(UDA_LOG_INFO, "Client TLS mode: %s\n", tlsModeStr(mode));
        g_state.ssl_disabled = mode == TlsMode::Off;
        if (g_state.ssl_disabled) {
            return 0;
        }

        AUTH_LOG(UDA_LOG_INFO, "Client TLS enabled\n");
        init_ssl_library();

        g_state.ctx = create_client_context();

        switch (mode) {
            case TlsMode::Off:
                return 0;
            case TlsMode::ServerOnly:
                configure_client_tls_server_only(g_state.ctx.get(), g_state.host);
                return 0;
            case TlsMode::Mutual:
                configure_client_tls_mutual(g_state.ctx.get(), g_state.host);
                return 0;
        }

        throw AuthError(AuthErrorCode::TlsConfigError, "Unsupported client TLS mode");

    } catch (const AuthError& e) {
        AUTH_LOG(UDA_LOG_ERROR, "Auth: TLS init failed [%d]: %s\n",
                 authErrorToUdaCode(e.code), e.what());
        UDA_ADD_ERROR(authErrorToUdaCode(e.code), e.what());
        return authErrorToUdaCode(e.code);
    }
}

int startUdaClientSSL()
{
    if (g_state.ssl_disabled) {
        return 0;
    }

    const PeerCertPolicy policy = uda::authentication::peer_cert_policy(g_state.tls_mode);

    try {
        connect_tls_connection(g_state.ctx.get(), policy);
        return 0;
    } catch (const AuthError& e) {
        AUTH_LOG(UDA_LOG_ERROR, "Auth: TLS connect failed [%d]: %s\n",
                 authErrorToUdaCode(e.code), e.what());
        UDA_ADD_ERROR(authErrorToUdaCode(e.code), e.what());
        return authErrorToUdaCode(e.code);
    }
}

int writeUdaClientSSL(void* iohandle, char* buf, int count)
{
    int rc, err = 0;

    fd_set wfds;
    struct timeval tv;

    udaUpdateSelectParms(g_state.ssl_socket, &wfds, &tv);

    while ((rc = select(g_state.ssl_socket + 1, nullptr, &wfds, nullptr, &tv)) <= 0) {
        if (rc < 0) {
            if (errno == EBADF) {
                UDA_LOG(UDA_LOG_DEBUG, "Socket is closed! Data access failed!.\n");
            } else {
                UDA_LOG(UDA_LOG_DEBUG, "Read error - %s\n", strerror(errno));
            }
            return -1;
        }

#ifndef _WIN32
        int fopts = 0;
        if ((rc = fcntl(g_state.ssl_socket, F_GETFL, &fopts)) < 0 || errno == EBADF) {
            err = 999;
            UDA_LOG(UDA_LOG_DEBUG, "Socket is closed!\n");
            return -1;
        }
#endif

        udaUpdateSelectParms(g_state.ssl_socket, &wfds, &tv);
    }

    rc = SSL_write(getUdaClientSSL(), buf, count);

    switch (SSL_get_error(getUdaClientSSL(), rc)) {
        case SSL_ERROR_NONE:
            if (rc != count) {
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
            if ((rc = fcntl(g_state.ssl_socket, F_GETFL, &fopts)) < 0 ||
                errno == EBADF) {
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

    udaUpdateSelectParms(g_state.ssl_socket, &rfds, &tv);

    while (((rc = select(g_state.ssl_socket + 1, &rfds, nullptr, nullptr, &tv)) <= 0)
            && maxloop++ < MAXLOOP) {

        if (rc < 0) {
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
        if ((rc = fcntl(g_state.ssl_socket, F_GETFL, &fopts)) < 0 ||
            errno == EBADF) {
            err = 999;
            UDA_LOG(UDA_LOG_DEBUG, "Socket is closed!\n");
            return -1;
        }
#endif

        udaUpdateSelectParms(g_state.ssl_socket, &rfds, &tv);
    }

    int blocked;
    do {
        blocked = 0;
        rc = SSL_read(getUdaClientSSL(), buf, count);

        switch (SSL_get_error(getUdaClientSSL(), rc)) {
            case SSL_ERROR_NONE:
                break;

            case SSL_ERROR_ZERO_RETURN:
                reportSSLErrorCode(rc);
                err = 999;
                UDA_LOG(UDA_LOG_DEBUG, "Server socket connection closed!\n");
                addIdamError(UDA_CODE_ERROR_TYPE, "readUdaClientSSL", err,
                             "Server socket connection closed!");
                return -1;

            case SSL_ERROR_WANT_READ:
                blocked = 1;
                break;

            case SSL_ERROR_WANT_WRITE:
                reportSSLErrorCode(rc);
                err = 999;
                UDA_LOG(UDA_LOG_DEBUG, "A read operation failed!\n");
                addIdamError(UDA_CODE_ERROR_TYPE, "readUdaClientSSL", err, "A read operation failed!");
                return -1;

            case SSL_ERROR_SYSCALL:
                reportSSLErrorCode(rc);
                err = 999;
                UDA_LOG(UDA_LOG_DEBUG, "Socket read I/O error!\n");
                addIdamError(UDA_CODE_ERROR_TYPE, "readUdaClientSSL", err, "Socket read I/O error!");
                return -1;

            default:
                reportSSLErrorCode(rc);
                err = 999;
                UDA_LOG(UDA_LOG_DEBUG, "Read from socket failed!\n");
                addIdamError(UDA_CODE_ERROR_TYPE, "readUdaClientSSL", err, "Read from socket failed!");
#ifndef _WIN32
                int fopts = 0;
                if ((rc = fcntl(g_state.ssl_socket, F_GETFL, &fopts)) < 0 ||
                    errno == EBADF) {
                    UDA_LOG(UDA_LOG_DEBUG, "Socket is closed!\n");
                }
#endif
                return -1;
        }

    } while (SSL_pending(getUdaClientSSL()) && !blocked);

    return rc;
}

#endif   // !SERVERBUILD && SSLAUTHENTICATION
