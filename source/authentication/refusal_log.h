#pragma once

// refused_requests.log API.
// One JSON Lines record per connection/request that was refused before completing normally.
// Compiled and active when SSLAUTHENTICATION or OIDCAUTHENTICATION is enabled (server only).
//
// Call open_refusal_log() at server startup alongside openAuthLog().
// Call close_refusal_log() at shutdown alongside closeAuthLog().
// Call record_refused_request() at each boundary where a connection is refused:
//   - TLS handshake / config failure  (udaServerSSL.cpp)
//   - OIDC gate failure               (handshake_auth.cpp)
//   - Client block decode failure      (udaServer.cpp)

#if defined(SSLAUTHENTICATION) || defined(OIDCAUTHENTICATION)

#include <string>

namespace uda { namespace authentication {

// ---------------------------------------------------------------------------
// Stage and reason codes — kept stable across releases for SIEM / grep use.

enum class RefusalStage {
    TlsConfig,    // server-side TLS configuration error (bad cert/key path, CRL load)
    TlsHandshake, // TLS handshake failure (client cert validation, protocol error)
    ClientBlock,  // CLIENT_BLOCK XDR decode failure
    OidcAuth,     // OIDC / bearer-token gate failure
    Protocol,     // client / server protocol version incompatibility
};

enum class RefusalReason {
    // TLS configuration
    TlsServerCertConfigError,
    TlsCrlLoadFailed,
    // TLS handshake — client certificate
    TlsClientCertExpired,
    TlsClientCertNotYetValid,
    TlsClientCertUntrusted,
    TlsClientCertRevoked,
    TlsClientCertMissing,
    // TLS handshake — other
    TlsHandshakeFailed,
    // OIDC
    OidcTokenMissing,
    OidcTokenExpired,
    OidcTokenInvalidSignature,
    OidcTokenBadIssuer,
    OidcTokenBadAudience,
    OidcClaimPolicyFailed,
    OidcConfigInvalid,
    // Protocol / client block
    ClientProtocolTooOld,
    ClientBlockMalformed,
};

// ---------------------------------------------------------------------------
// Data containers

struct PeerInfo {
    std::string ip;
    int         port = 0;
};

struct TlsRefusalFields {
    std::string tls_mode;                       // "off" | "server" | "mutual"
    std::string tls_version;                    // "TLSv1.3" etc., if partially negotiated
    std::string tls_cipher;
    long        client_cert_verify_result = -1; // X509_V_* code; -1 = not applicable
    std::string client_cert_subject;
    std::string client_cert_issuer;
    std::string client_cert_serial;
    std::string client_cert_not_before;
    std::string client_cert_not_after;
    std::string client_cert_fingerprint_sha256;
};

struct RefusalRecord {
    RefusalStage  stage          = RefusalStage::Protocol;
    RefusalReason reason         = RefusalReason::ClientBlockMalformed;
    int           uda_error_code = 0;
    std::string   message;
    PeerInfo      peer;
    // Common client fields (from CLIENT_BLOCK when decoded)
    std::string   client_username;
    int           client_version = 0;
    // TLS-specific — only populate for TLS stages
    TlsRefusalFields tls;
    // OIDC-specific — only populate for OidcAuth stage
    std::string   token_error;   // "missing" | "invalid" | "expired" | "bad_issuer" | ...
    std::string   oidc_issuer;   // from server env config, if available
    std::string   oidc_sub_sha256;
    // Protocol / client block error detail
    std::string   decode_error;
};

// ---------------------------------------------------------------------------
// API

// Extract peer IP and port from a connected socket fd (POSIX getpeername).
// Returns empty PeerInfo on failure.
PeerInfo get_peer_info(int fd) noexcept;

void open_refusal_log(const char* logdir, const char* logmode);
void close_refusal_log();
void record_refused_request(const RefusalRecord& rec);

}} // namespace uda::authentication

#endif // SSLAUTHENTICATION || OIDCAUTHENTICATION
