#pragma once

// refused_requests.log API.
// One JSON Lines record per connection/request that was refused before completing normally.
//
// This is an audit artifact, not a debug log:
//   - it is always opened in append mode, never truncated (the server is fork-per-connection,
//     so a truncating open would let each new connection erase the previous one's records);
//   - it is not gated on the debug log level;
//   - each record is written with a single write call so that concurrent server processes
//     appending to the same file cannot interleave a record with another record's newline.
//
// Call open_refusal_log() at server startup alongside openAuthLog().
// Call close_refusal_log() at shutdown alongside closeAuthLog().
// Call record_refused_request() at each boundary where a connection is refused:
//   - TLS handshake / config failure  (udaServerSSL.cpp)
//   - OIDC gate failure               (handshake_auth.cpp)
//   - Client block decode failure      (udaServer.cpp)
//
// The record types below are always defined so that call-sites need no #ifdef guards.
// The three functions are real only in a server build with TLS and/or OIDC compiled in;
// elsewhere (fat-client builds, auth-free builds) they are inline no-ops.

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

#if (defined(SSLAUTHENTICATION) || defined(OIDCAUTHENTICATION)) && !defined(FATCLIENT)

// ---------------------------------------------------------------------------
// API

// Stable wire spellings for the two vocabularies. Exposed so that tests and the
// documented log schema are generated from the same source as the log itself.
const char* stage_str(RefusalStage s) noexcept;
const char* reason_code_str(RefusalReason r) noexcept;

// Serialise one record to its JSON Lines form (without the trailing newline).
// Exposed for testing; record_refused_request() writes exactly this.
std::string format_refusal_record(const RefusalRecord& rec);

// The UDA server is launched per connection by inetd / systemd socket activation with
// the connected socket already on fd 0. Call sites that have no fd of their own use this
// rather than a bare literal, so the assumption lives in one place.
inline constexpr int INETD_SOCKET_FD = 0;

// Extract peer IP and port from a connected socket fd (POSIX getpeername).
// Returns empty PeerInfo on failure.
PeerInfo get_peer_info(int fd) noexcept;

// Open <logdir>/refused_requests.log. Always append mode — see the note above.
void open_refusal_log(const char* logdir);
void close_refusal_log();
void record_refused_request(const RefusalRecord& rec);

#else

// Fat-client and auth-free builds: no server, so nothing to audit.
inline constexpr int INETD_SOCKET_FD = 0;
inline PeerInfo get_peer_info(int) noexcept { return {}; }
inline void open_refusal_log(const char*) {}
inline void close_refusal_log() {}
inline void record_refused_request(const RefusalRecord&) {}

#endif

}} // namespace uda::authentication
