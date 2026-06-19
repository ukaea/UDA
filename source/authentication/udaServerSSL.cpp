#if defined(SSLAUTHENTICATION)

#include "udaServerSSL.h"
#include "auth_error.h"
#include "server/createXDRStream.h"
#include "tlsMode.h"
#include "tls_policy.h"

#include <fcntl.h>
#include <initializer_list>
#include <memory>
#include <openssl/asn1.h>
#include <openssl/crypto.h>
#include <openssl/err.h>
#include <openssl/pem.h>
#include <openssl/ssl.h>
#include <openssl/x509.h>
#include <string>

#include <clientserver/errorLog.h>
#include <logging/logging.h>
#include <server/writer.h>

#include "authLog.h"
#include "refusal_log.h"
#include "utils.h"

#define VERIFY_DEPTH 4

/*
Note on initialisation:
UDA Servers using plugins that connect to other UDA servers through the standard client API library
need to block initialisation of the SSL library by the client. Initialisation must be done once only.
As all information passed between plugins and servers is through the interface structure, the state
of initialisation by the server must be passed within the interface. This state information is local
and should not be passed to subsequent servers. An alternative and simpler mechanism is for the server
to assign a value to an environment variable, and for the client to test this environment variable.
*/

struct SslDeleter    { void operator()(SSL* p)     const noexcept { if (p) SSL_free(p); } };
struct SslCtxDeleter { void operator()(SSL_CTX* p) const noexcept { if (p) SSL_CTX_free(p); } };
using SslPtr    = std::unique_ptr<SSL,     SslDeleter>;
using SslCtxPtr = std::unique_ptr<SSL_CTX, SslCtxDeleter>;

struct ServerSslState {
    bool      ssl_disabled = true;
    int       ssl_socket   = -1;
    bool      ssl_ok       = false;
    bool      ssl_init     = false;
    SslPtr    ssl;
    SslCtxPtr ctx;
};
static ServerSslState g_state;

using uda::authentication::AuthError;
using uda::authentication::AuthErrorCode;
using uda::authentication::authErrorToUdaCode;

static void initUdaServerSSL();
static SslCtxPtr create_server_context();
static void load_server_certificate(SSL_CTX* ctx);
static void load_ca_certificates(SSL_CTX* ctx, const std::string& path);
static void configure_server_tls_server_only(SSL_CTX* ctx);
static void configure_server_tls_mutual(SSL_CTX* ctx);
static void accept_tls_connection(SSL_CTX* ctx, PeerCertPolicy policy);
static X509_CRL* loadUdaServerSSLCrl(const char* crlist);
static int addUdaServerSSLCrlsStore(X509_STORE* st, STACK_OF(X509_CRL) * crls);

void putUdaServerSSLSocket(int socket) { g_state.ssl_socket = socket; }

bool getUdaServerSSLDisabled() { return g_state.ssl_disabled; }

static const char* server_ssl_error_name(int err)
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

void reportServerSSLErrorCode(int rc)
{
    const int err = SSL_get_error(g_state.ssl.get(), rc);
    const char* msg = server_ssl_error_name(err);
    addIdamError(UDA_CODE_ERROR_TYPE, "udaSSL", 999, msg);
    AUTH_LOG(UDA_LOG_ERROR, "Auth: SSL error: %s\n", msg);

    char detail[256];
    unsigned long ossl_err;
    while ((ossl_err = ERR_get_error()) != 0) {
        ERR_error_string_n(ossl_err, detail, sizeof(detail));
        AUTH_LOG(UDA_LOG_ERROR, "Auth: SSL error detail: %s\n", detail);
    }

    if (g_state.ssl != nullptr) {
        AUTH_LOG(UDA_LOG_DEBUG, "Auth: SSL state: %s\n", SSL_state_string(g_state.ssl.get()));
        const long vr = SSL_get_verify_result(g_state.ssl.get());
        if (vr != X509_V_OK) {
            AUTH_LOG(UDA_LOG_ERROR, "Auth: TLS verify result: %s\n",
                     X509_verify_cert_error_string(vr));
        }
    }
}

void initUdaServerSSL()
{
    if (g_state.ssl_init) return;
    if (getenv("UDA_SSL_INITIALISED")) {
        g_state.ssl_init = true;
        UDA_LOG(UDA_LOG_DEBUG, "Prior SSL initialisation\n");
        return;
    }
    SSL_library_init();
    SSL_load_error_strings();
    OpenSSL_add_ssl_algorithms();
#ifdef _WIN32
    _putenv_s("UDA_SSL_INITIALISED", "1");
#else
    setenv("UDA_SSL_INITIALISED", "1", 0);
#endif
    g_state.ssl_init = true;
    UDA_LOG(UDA_LOG_DEBUG, "SSL initialised\n");
}

void closeUdaServerSSL()
{
    if (g_state.ssl_disabled) {
        return;
    }
    if (g_state.ssl) SSL_shutdown(g_state.ssl.get());
#ifdef _WIN32
    _putenv_s("UDA_SSL_INITIALISED", NULL);
#else
    unsetenv("UDA_SSL_INITIALISED");
#endif
    g_state = ServerSslState{};
    UDA_LOG(UDA_LOG_DEBUG, "SSL closed\n");
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

// -------------------------------------------------------------------------
// Internal TLS setup — throw AuthError; no UDA error stack manipulation.
// Translation to UDA error codes happens only in the boundary (startUdaServerSSL).

static SslCtxPtr create_server_context()
{
    const SSL_METHOD* method = SSLv23_server_method();
    SslCtxPtr ctx(SSL_CTX_new(method));
    if (!ctx) {
        throw AuthError(AuthErrorCode::TlsConfigError, "Unable to create SSL context");
    }
    SSL_CTX_set_options(ctx.get(), SSL_OP_NO_SSLv2);
    UDA_LOG(UDA_LOG_DEBUG, "SSL Context created\n");
    return ctx;
}

static void load_server_certificate(SSL_CTX* ctx)
{
    const char* cert = first_env({"UDA_SERVER_TLS_CERT", "UDA_SERVER_SSL_CERT"});
    const char* key  = first_env({"UDA_SERVER_TLS_KEY",  "UDA_SERVER_SSL_KEY"});

    if (!cert) {
        throw AuthError(AuthErrorCode::TlsConfigError,
            "No server TLS certificate; set UDA_SERVER_TLS_CERT or UDA_SERVER_SSL_CERT");
    }
    if (!key) {
        throw AuthError(AuthErrorCode::TlsConfigError,
            "No server TLS key; set UDA_SERVER_TLS_KEY or UDA_SERVER_SSL_KEY");
    }

    AUTH_LOG(UDA_LOG_DEBUG, "Server cert: %s\n", cert);
    AUTH_LOG(UDA_LOG_DEBUG, "Server key:  %s\n", key);

    if (SSL_CTX_use_certificate_file(ctx, cert, SSL_FILETYPE_PEM) <= 0) {
        throw AuthError(AuthErrorCode::TlsConfigError,
            std::string("Failed to load server certificate: ") + cert);
    }
    if (SSL_CTX_use_PrivateKey_file(ctx, key, SSL_FILETYPE_PEM) <= 0) {
        throw AuthError(AuthErrorCode::TlsConfigError,
            std::string("Failed to load server private key: ") + key);
    }
    if (SSL_CTX_check_private_key(ctx) == 0) {
        throw AuthError(AuthErrorCode::TlsConfigError,
            "Server private key does not match the certificate public key");
    }
    AUTH_LOG(UDA_LOG_DEBUG, "Server cert and key loaded and verified\n");
}

static void load_ca_certificates(SSL_CTX* ctx, const std::string& path)
{
    if (path.empty()) {
        throw AuthError(AuthErrorCode::TlsConfigError,
            "No CA certificate path; set UDA_SERVER_CA_TLS_CERT or UDA_SERVER_CA_SSL_CERT");
    }
    if (SSL_CTX_load_verify_locations(ctx, path.c_str(), nullptr) < 1) {
        throw AuthError(AuthErrorCode::TlsConfigError,
            "Failed to load CA certificates from: " + path);
    }
}

static void configure_server_tls_server_only(SSL_CTX* ctx)
{
    AUTH_LOG(UDA_LOG_INFO, "Configuring server TLS: server-only (client cert not required)\n");
    load_server_certificate(ctx);
    SSL_CTX_set_verify(ctx, SSL_VERIFY_NONE, nullptr);
    SSL_CTX_set_verify_depth(ctx, VERIFY_DEPTH);
    AUTH_LOG(UDA_LOG_INFO, "Server-only TLS context configured\n");
}

static void configure_server_tls_mutual(SSL_CTX* ctx)
{
    const char* ca     = first_env({"UDA_SERVER_CA_TLS_CERT", "UDA_SERVER_TLS_CA_CERT", "UDA_SERVER_CA_SSL_CERT"});
    const char* crlist = first_env({"UDA_SERVER_CA_TLS_CRL",  "UDA_SERVER_TLS_CA_CRL",  "UDA_SERVER_CA_SSL_CRL"});

    load_server_certificate(ctx);
    load_ca_certificates(ctx, ca == nullptr ? "" : ca);

    SSL_CTX_set_verify(ctx, SSL_VERIFY_PEER | SSL_VERIFY_FAIL_IF_NO_PEER_CERT, nullptr);
    SSL_CTX_set_verify_depth(ctx, VERIFY_DEPTH);

    if (crlist != nullptr) {
        X509_VERIFY_PARAM* params = X509_VERIFY_PARAM_new();
        X509_VERIFY_PARAM_set_flags(params, X509_V_FLAG_CRL_CHECK);
        SSL_CTX_set1_param(ctx, params);

        X509_CRL* crl = loadUdaServerSSLCrl(crlist);
        if (!crl) {
            throw AuthError(AuthErrorCode::TlsConfigError, "Failed to load CRL");
        }

        STACK_OF(X509_CRL)* crls = sk_X509_CRL_new_null();
        if (!crls || !sk_X509_CRL_push(crls, crl)) {
            X509_CRL_free(crl);
            throw AuthError(AuthErrorCode::TlsConfigError,
                "Failed to add CRL to certificate store");
        }

        X509_STORE* st = SSL_CTX_get_cert_store(ctx);
        addUdaServerSSLCrlsStore(st, crls);
        SSL_CTX_set1_verify_cert_store(ctx, st);
    }

    AUTH_LOG(UDA_LOG_INFO, "Mutual TLS context configured\n");
}

static X509_CRL* loadUdaServerSSLCrl(const char* crlist)
{
    BIO* in = BIO_new(BIO_s_file());
    if (in == nullptr) {
        return nullptr;
    }
    if (BIO_read_filename(in, crlist) <= 0) {
        BIO_free(in);
        return nullptr;
    }
    X509_CRL* crl = PEM_read_bio_X509_CRL(in, nullptr, nullptr, nullptr);
    BIO_free(in);
    UDA_LOG(UDA_LOG_DEBUG, "CRL loaded\n");
    return crl;
}

static int addUdaServerSSLCrlsStore(X509_STORE* st, STACK_OF(X509_CRL) * crls)
{
    for (int i = 0; i < sk_X509_CRL_num(crls); i++) {
        X509_STORE_add_crl(st, sk_X509_CRL_value(crls, i));
    }
    return 1;
}

// ---------------------------------------------------------------------------
// Helpers for refused_requests.log

static RefusalReason classify_verify_result(long vr) noexcept
{
    if (vr == X509_V_ERR_CERT_HAS_EXPIRED)                      return RefusalReason::TlsClientCertExpired;
    if (vr == X509_V_ERR_CERT_NOT_YET_VALID)                    return RefusalReason::TlsClientCertNotYetValid;
    if (vr == X509_V_ERR_CERT_REVOKED)                          return RefusalReason::TlsClientCertRevoked;
    if (vr == X509_V_ERR_CERT_UNTRUSTED                    ||
        vr == X509_V_ERR_DEPTH_ZERO_SELF_SIGNED_CERT       ||
        vr == X509_V_ERR_SELF_SIGNED_CERT_IN_CHAIN         ||
        vr == X509_V_ERR_UNABLE_TO_GET_ISSUER_CERT         ||
        vr == X509_V_ERR_UNABLE_TO_GET_ISSUER_CERT_LOCALLY ||
        vr == X509_V_ERR_UNABLE_TO_VERIFY_LEAF_SIGNATURE)       return RefusalReason::TlsClientCertUntrusted;
    return RefusalReason::TlsHandshakeFailed;
}

static TlsRefusalFields extract_cert_fields(X509* peer, SSL* ssl) noexcept
{
    TlsRefusalFields f;

    if (ssl != nullptr) {
        const char* ver = SSL_get_version(ssl);
        if (ver) f.tls_version = ver;
        const char* cip = SSL_get_cipher(ssl);
        if (cip) f.tls_cipher = cip;
        f.client_cert_verify_result = SSL_get_verify_result(ssl);
    }

    if (peer == nullptr) return f;

    char work[X509_STRING_SIZE];
    const char* subj = X509_NAME_oneline(X509_get_subject_name(peer), work, sizeof(work));
    if (subj) f.client_cert_subject = subj;
    const char* iss  = X509_NAME_oneline(X509_get_issuer_name(peer),  work, sizeof(work));
    if (iss)  f.client_cert_issuer  = iss;

    // Serial number as hex string
    ASN1_INTEGER* sn = X509_get_serialNumber(peer);
    if (sn) {
        BIGNUM* bn = ASN1_INTEGER_to_BN(sn, nullptr);
        if (bn) {
            char* hex = BN_bn2hex(bn);
            if (hex) { f.client_cert_serial = hex; OPENSSL_free(hex); }
            BN_free(bn);
        }
    }

    f.client_cert_not_before = to_string(X509_getm_notBefore(peer));
    f.client_cert_not_after  = to_string(X509_getm_notAfter(peer));

    // SHA-256 fingerprint as lowercase hex
    unsigned char digest[EVP_MAX_MD_SIZE];
    unsigned int  dlen = 0;
    if (X509_digest(peer, EVP_sha256(), digest, &dlen)) {
        std::string hex;
        hex.reserve(dlen * 2);
        for (unsigned int i = 0; i < dlen; ++i) {
            char tmp[3];
            snprintf(tmp, sizeof(tmp), "%02x", digest[i]);
            hex += tmp;
        }
        f.client_cert_fingerprint_sha256 = hex;
    }

    return f;
}

static void accept_tls_connection(SSL_CTX* ctx, PeerCertPolicy policy)
{
    int rc;

    AUTH_LOG(UDA_LOG_DEBUG, "Initiating TLS accept (peer cert policy: %s)\n",
             policy == PeerCertPolicy::Required ? "required" : "not-required");

    g_state.ssl.reset(SSL_new(ctx));
    if (g_state.ssl == nullptr) {
        const std::string msg = "SSL_new failed (server-side resource error)";
        RefusalRecord rec;
        rec.stage          = RefusalStage::TlsHandshake;
        rec.reason         = RefusalReason::TlsHandshakeFailed;
        rec.uda_error_code = authErrorToUdaCode(AuthErrorCode::TlsHandshakeFailed);
        rec.message        = msg;
        rec.peer           = get_peer_info(g_state.ssl_socket);
        rec.tls.tls_mode   = tlsModeStr(getServerTlsMode());
        record_refused_request(rec);
        throw AuthError(AuthErrorCode::TlsHandshakeFailed, msg);
    }

    if ((rc = SSL_set_fd(g_state.ssl.get(), g_state.ssl_socket)) < 1) {
        const std::string msg = "Unable to bind socket to SSL";
        RefusalRecord rec;
        rec.stage          = RefusalStage::TlsHandshake;
        rec.reason         = RefusalReason::TlsHandshakeFailed;
        rec.uda_error_code = authErrorToUdaCode(AuthErrorCode::TlsHandshakeFailed);
        rec.message        = msg;
        rec.peer           = get_peer_info(g_state.ssl_socket);
        rec.tls.tls_mode   = tlsModeStr(getServerTlsMode());
        record_refused_request(rec);
        throw AuthError(AuthErrorCode::TlsHandshakeFailed, msg);
    }

    if ((rc = SSL_accept(g_state.ssl.get())) < 1) {
        const int ssl_err = SSL_get_error(g_state.ssl.get(), rc);

        // Capture first OpenSSL error string before draining the queue.
        char ssl_err_buf[256] = {};
        unsigned long ossl_err = ERR_get_error();
        if (ossl_err != 0) {
            ERR_error_string_n(ossl_err, ssl_err_buf, sizeof(ssl_err_buf));
            AUTH_LOG(UDA_LOG_ERROR, "Auth: TLS error detail: %s\n", ssl_err_buf);
            unsigned long next_err;
            while ((next_err = ERR_get_error()) != 0) {
                char tmp[256];
                ERR_error_string_n(next_err, tmp, sizeof(tmp));
                AUTH_LOG(UDA_LOG_ERROR, "Auth: TLS error detail: %s\n", tmp);
            }
        }

        if (g_state.ssl != nullptr) {
            AUTH_LOG(UDA_LOG_DEBUG, "Auth: SSL state: %s\n", SSL_state_string(g_state.ssl.get()));
        }

        const long vr = SSL_get_verify_result(g_state.ssl.get());
        if (vr != X509_V_OK) {
            AUTH_LOG(UDA_LOG_ERROR,
                "Auth: TLS client cert verification failed: %s (mode=%s)\n",
                X509_verify_cert_error_string(vr), tlsModeStr(getServerTlsMode()));
        } else if (ssl_err == SSL_ERROR_SYSCALL) {
            AUTH_LOG(UDA_LOG_ERROR,
                "Auth: TLS handshake failed (SSL_ERROR_SYSCALL): connection closed by client "
                "before handshake (mode=%s) — check UDA_SERVER_TLS_MODE / UDA_CLIENT_TLS_MODE\n",
                tlsModeStr(getServerTlsMode()));
        } else {
            AUTH_LOG(UDA_LOG_ERROR,
                "Auth: TLS handshake failed (%s mode=%s)\n",
                server_ssl_error_name(ssl_err), tlsModeStr(getServerTlsMode()));
        }

        if (errno != 0) {
            AUTH_LOG(UDA_LOG_ERROR, "Auth: system error: %s\n", strerror(errno));
        }

        const std::string err_msg =
            std::string("TLS handshake failed (") + server_ssl_error_name(ssl_err) + ")";

        {
            RefusalRecord rec;
            rec.stage          = RefusalStage::TlsHandshake;
            rec.reason         = (vr != X509_V_OK)
                                 ? classify_verify_result(vr)
                                 : RefusalReason::TlsHandshakeFailed;
            rec.uda_error_code = authErrorToUdaCode(AuthErrorCode::TlsHandshakeFailed);
            rec.message        = err_msg;
            rec.peer           = get_peer_info(g_state.ssl_socket);
            rec.tls            = extract_cert_fields(nullptr, g_state.ssl.get());
            rec.tls.tls_mode   = tlsModeStr(getServerTlsMode());
            record_refused_request(rec);
        }

        throw AuthError(AuthErrorCode::TlsHandshakeFailed, err_msg);
    }

    AUTH_LOG(UDA_LOG_INFO, "Auth: TLS handshake succeeded (version=%s cipher=%s)\n",
             SSL_get_version(g_state.ssl.get()), SSL_get_cipher(g_state.ssl.get()));

    X509* peer = SSL_get_peer_certificate(g_state.ssl.get());

    if (peer != nullptr) {
        if (policy == PeerCertPolicy::Required &&
                (rc = (int)SSL_get_verify_result(g_state.ssl.get())) != X509_V_OK) {
            const char* reason = X509_verify_cert_error_string(rc);
            AUTH_LOG(UDA_LOG_ERROR, "Auth: client cert verification failed: %s\n", reason);
            const std::string err_msg =
                std::string("Client certificate verification failed: ") + reason;
            {
                RefusalRecord rec;
                rec.stage          = RefusalStage::TlsHandshake;
                rec.reason         = classify_verify_result(static_cast<long>(rc));
                rec.uda_error_code = authErrorToUdaCode(AuthErrorCode::TlsHandshakeFailed);
                rec.message        = err_msg;
                rec.peer           = get_peer_info(g_state.ssl_socket);
                rec.tls            = extract_cert_fields(peer, g_state.ssl.get());
                rec.tls.tls_mode   = tlsModeStr(getServerTlsMode());
                record_refused_request(rec);
            }
            X509_free(peer);
            throw AuthError(AuthErrorCode::TlsHandshakeFailed, err_msg);
        }

        char work[X509_STRING_SIZE];
        AUTH_LOG(UDA_LOG_INFO, "Auth: client cert verified — subject: %s\n",
                 X509_NAME_oneline(X509_get_subject_name(peer), work, sizeof(work)));
        AUTH_LOG(UDA_LOG_DEBUG, "Auth: client cert issuer: %s\n",
                 X509_NAME_oneline(X509_get_issuer_name(peer), work, sizeof(work)));

        ASN1_TIME* before = X509_getm_notBefore(peer);
        ASN1_TIME* after  = X509_getm_notAfter(peer);
        AUTH_LOG(UDA_LOG_DEBUG, "Auth: client cert validity: %s to %s\n",
                 to_string(before).c_str(), to_string(after).c_str());
        X509_free(peer);
    } else {
        if (policy == PeerCertPolicy::Required) {
            const std::string err_msg = "Client certificate not presented for verification";
            {
                RefusalRecord rec;
                rec.stage          = RefusalStage::TlsHandshake;
                rec.reason         = RefusalReason::TlsClientCertMissing;
                rec.uda_error_code = authErrorToUdaCode(AuthErrorCode::TlsHandshakeFailed);
                rec.message        = err_msg;
                rec.peer           = get_peer_info(g_state.ssl_socket);
                rec.tls            = extract_cert_fields(nullptr, g_state.ssl.get());
                rec.tls.tls_mode   = tlsModeStr(getServerTlsMode());
                record_refused_request(rec);
            }
            throw AuthError(AuthErrorCode::TlsHandshakeFailed, err_msg);
        }
        AUTH_LOG(UDA_LOG_DEBUG, "Auth: no client cert presented; not required in server-only mode\n");
    }

    g_state.ssl_ok = true;
}

// -------------------------------------------------------------------------
// Boundary function — catch AuthError, translate to UDA error stack, return stable code

int startUdaServerSSL()
{
    if (g_state.ssl_ok) {
        return 0;
    }

    try {
        if (!isValidTlsModeEnv("UDA_SERVER_TLS_MODE")) {
            getServerTlsMode();
            throw AuthError(AuthErrorCode::TlsConfigError, "Invalid server TLS mode");
        }

        const TlsMode mode = getServerTlsMode();
        AUTH_LOG(UDA_LOG_INFO, "Server TLS mode: %s\n", tlsModeStr(mode));
        g_state.ssl_disabled = mode == TlsMode::Off;
        if (g_state.ssl_disabled) {
            return 0;
        }

        AUTH_LOG(UDA_LOG_INFO, "Server TLS enabled\n");
        initUdaServerSSL();

        g_state.ctx = create_server_context();

        const PeerCertPolicy policy = uda::authentication::peer_cert_policy(mode);

        switch (mode) {
            case TlsMode::Off:
                return 0;
            case TlsMode::ServerOnly:
                configure_server_tls_server_only(g_state.ctx.get());
                accept_tls_connection(g_state.ctx.get(), policy);
                return 0;
            case TlsMode::Mutual:
                configure_server_tls_mutual(g_state.ctx.get());
                accept_tls_connection(g_state.ctx.get(), policy);
                return 0;
        }

        throw AuthError(AuthErrorCode::TlsConfigError, "Unsupported server TLS mode");

    } catch (const AuthError& e) {
        AUTH_LOG(UDA_LOG_ERROR, "Auth: TLS start failed [%d]: %s\n",
                 authErrorToUdaCode(e.code), e.what());
        UDA_ADD_ERROR(authErrorToUdaCode(e.code), e.what());
        // Config errors are logged here; handshake errors were already logged inside
        // accept_tls_connection before they were thrown.
        if (e.code == AuthErrorCode::TlsConfigError) {
            RefusalRecord rec;
            rec.stage          = RefusalStage::TlsConfig;
            rec.reason         = RefusalReason::TlsServerCertConfigError;
            rec.uda_error_code = authErrorToUdaCode(e.code);
            rec.message        = e.what();
            rec.peer           = get_peer_info(g_state.ssl_socket);
            rec.tls.tls_mode   = tlsModeStr(getServerTlsMode());
            record_refused_request(rec);
        }
        return authErrorToUdaCode(e.code);
    }
}

#ifdef UNUSED
#elif defined(__GNUC__)
#define UNUSED __attribute__((unused))
#elif defined(__LCLINT__)
#define UNUSED /*@unused@*/
#else
#define UNUSED
#endif

int writeUdaServerSSL(void* iohandle, const char* buf, int count)
{
    int rc;

    fd_set wfds;
    struct timeval tv = {};

    auto io_data = reinterpret_cast<IoData*>(iohandle);

    setSelectParms(g_state.ssl_socket, &wfds, &tv, io_data->server_tot_block_time);

    while ((rc = select(g_state.ssl_socket + 1, nullptr, &wfds, nullptr, &tv)) <= 0) {

        if (rc < 0) {
            if (errno == EBADF) {
                UDA_LOG(UDA_LOG_DEBUG, "Client Socket is closed! Closing server down.\n");
            } else {
                UDA_LOG(UDA_LOG_DEBUG, "Read error - %s\n", strerror(errno));
                UDA_LOG(UDA_LOG_DEBUG, "Closing server down.\n");
            }
            return -1;
        }

#ifndef _WIN32
        int fopts = 0;
        if (fcntl(g_state.ssl_socket, F_GETFL, &fopts) < 0 || errno == EBADF) {
            UDA_LOG(UDA_LOG_DEBUG, "Client Socket is closed! Closing server down.\n");
            return -1;
        }
#endif

        *io_data->server_tot_block_time += tv.tv_usec / 1000;

        if (*io_data->server_tot_block_time / 1000 > *io_data->server_timeout) {
            UDA_LOG(UDA_LOG_DEBUG, "Total Blocking Time: %d (ms). Closing server down.\n",
                    *io_data->server_tot_block_time);
            return -1;
        }

        updateSelectParms(g_state.ssl_socket, &wfds, &tv, *io_data->server_tot_block_time);
    }

    rc = SSL_write(g_state.ssl.get(), buf, count);

    switch (SSL_get_error(g_state.ssl.get(), rc)) {
        case SSL_ERROR_NONE:
            if (rc != count) {
                UDA_LOG(UDA_LOG_DEBUG, "Incomplete write to socket!\n");
                UDA_ADD_ERROR(999, "Incomplete write to socket!");
                return -1;
            }
            break;

        default:
            reportServerSSLErrorCode(rc);
            UDA_LOG(UDA_LOG_DEBUG, "Write to socket failed!\n");
            UDA_ADD_ERROR(999, "Write to socket failed!");
#ifndef _WIN32
            int fopts = 0;
            if (fcntl(g_state.ssl_socket, F_GETFL, &fopts) < 0 || errno == EBADF) {
                UDA_LOG(UDA_LOG_DEBUG, "Client Socket is closed! Closing server down.\n");
            }
#endif
            return -1;
    }

    return rc;
}

int readUdaServerSSL(void* iohandle, char* buf, int count)
{
    int rc;
    fd_set rfds;
    struct timeval tv, tvc;

    auto io_data = reinterpret_cast<IoData*>(iohandle);

    setSelectParms(g_state.ssl_socket, &rfds, &tv, io_data->server_tot_block_time);
    tvc = tv;

    while ((rc = select(g_state.ssl_socket + 1, &rfds, nullptr, nullptr, &tvc)) <= 0) {

        if (rc < 0) {
            if (errno == EBADF) {
                UDA_LOG(UDA_LOG_DEBUG, "Client Socket is closed! Closing server down.\n");
            } else {
                UDA_LOG(UDA_LOG_DEBUG, "Read error - %s\n", strerror(errno));
                UDA_LOG(UDA_LOG_DEBUG, "Closing server down.\n");
            }
            return -1;
        }

        *io_data->server_tot_block_time += (int)tv.tv_usec / 1000;

        if (*io_data->server_tot_block_time > 1000 * *io_data->server_timeout) {
            UDA_LOG(UDA_LOG_DEBUG, "Total Wait Time Exceeds Lifetime Limit = %d (ms). Closing server down.\n",
                    *io_data->server_timeout * 1000);
            return -1;
        }

#ifndef _WIN32
        int fopts = 0;
        if (fcntl(g_state.ssl_socket, F_GETFL, &fopts) < 0 || errno == EBADF) {
            UDA_LOG(UDA_LOG_DEBUG, "Client Socket is closed! Closing server down.\n");
            return -1;
        }
#endif

        updateSelectParms(g_state.ssl_socket, &rfds, &tv, *io_data->server_tot_block_time);
        tvc = tv;
    }

    int blocked;
    do {
        blocked = 0;
        rc = SSL_read(g_state.ssl.get(), buf, count);

        switch (SSL_get_error(g_state.ssl.get(), rc)) {
            case SSL_ERROR_NONE:
                break;

            case SSL_ERROR_ZERO_RETURN:
                reportServerSSLErrorCode(rc);
                UDA_LOG(UDA_LOG_DEBUG, "Client socket connection closed!\n");
                UDA_ADD_ERROR(999, "Client socket connection closed!");
                return -1;

            case SSL_ERROR_WANT_READ:
                blocked = 1;
                break;

            case SSL_ERROR_WANT_WRITE:
                reportServerSSLErrorCode(rc);
                UDA_LOG(UDA_LOG_DEBUG, "A read operation failed!\n");
                UDA_ADD_ERROR(999, "A read operation failed!");
                return -1;

            case SSL_ERROR_SYSCALL:
                reportServerSSLErrorCode(rc);
                UDA_LOG(UDA_LOG_DEBUG, "Client socket read I/O error!\n");
                UDA_ADD_ERROR(999, "Client socket read I/O error!");
                return -1;

            default:
                reportServerSSLErrorCode(rc);
                UDA_LOG(UDA_LOG_DEBUG, "Read from socket failed!\n");
                UDA_ADD_ERROR(999, "Read from socket failed!");
#ifndef _WIN32
                int fopts = 0;
                if ((rc = fcntl(g_state.ssl_socket, F_GETFL, &fopts)) < 0 ||
                    errno == EBADF) {
                    UDA_LOG(UDA_LOG_DEBUG, "writeUdaServerSSL: Client Socket is closed! Closing server down.\n");
                }
#endif
                return -1;
        }

    } while (SSL_pending(g_state.ssl.get()) && !blocked);

    return rc;
}

#endif // SERVERBUILD
