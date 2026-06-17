#pragma once

#include <stdexcept>
#include <string>

namespace uda {
namespace authentication {

// Typed error codes for all authentication and TLS failures.
// Used to translate C++ exceptions into UDA error-stack entries at the layer boundaries
// (initUdaClientSSL, startUdaClientSSL, startUdaServerSSL, handshakeClient).
enum class AuthErrorCode {
    MissingToken,
    InvalidConfig,
    DiscoveryFailed,
    JwksFetchFailed,
    InvalidToken,
    ClaimPolicyFailed,
    TlsConfigError,
    TlsHandshakeFailed,
    TlsHostnameMismatch,
};

class AuthError : public std::runtime_error {
public:
    const AuthErrorCode code;
    AuthError(AuthErrorCode c, const std::string& msg)
        : std::runtime_error(msg), code(c) {}
};

// Stable UDA error codes for reporting auth failures on the UDA error stack.
// Clients receive these codes in the server block's idamerrorstack.
static constexpr int UDA_AUTH_ERR_MISSING_TOKEN    = 700;
static constexpr int UDA_AUTH_ERR_INVALID_CONFIG   = 701;
static constexpr int UDA_AUTH_ERR_DISCOVERY_FAILED = 702;
static constexpr int UDA_AUTH_ERR_JWKS_FETCH       = 703;
static constexpr int UDA_AUTH_ERR_INVALID_TOKEN    = 704;
static constexpr int UDA_AUTH_ERR_CLAIM_POLICY     = 705;
static constexpr int UDA_TLS_ERR_CONFIG            = 710;
static constexpr int UDA_TLS_ERR_HANDSHAKE         = 711;
static constexpr int UDA_TLS_ERR_HOSTNAME_MISMATCH = 712;

// Map an AuthErrorCode to its stable UDA error code integer.
inline int authErrorToUdaCode(AuthErrorCode code) noexcept
{
    switch (code) {
        case AuthErrorCode::MissingToken:        return UDA_AUTH_ERR_MISSING_TOKEN;
        case AuthErrorCode::InvalidConfig:       return UDA_AUTH_ERR_INVALID_CONFIG;
        case AuthErrorCode::DiscoveryFailed:     return UDA_AUTH_ERR_DISCOVERY_FAILED;
        case AuthErrorCode::JwksFetchFailed:     return UDA_AUTH_ERR_JWKS_FETCH;
        case AuthErrorCode::InvalidToken:        return UDA_AUTH_ERR_INVALID_TOKEN;
        case AuthErrorCode::ClaimPolicyFailed:   return UDA_AUTH_ERR_CLAIM_POLICY;
        case AuthErrorCode::TlsConfigError:      return UDA_TLS_ERR_CONFIG;
        case AuthErrorCode::TlsHandshakeFailed:  return UDA_TLS_ERR_HANDSHAKE;
        case AuthErrorCode::TlsHostnameMismatch: return UDA_TLS_ERR_HOSTNAME_MISMATCH;
    }
    return 999;
}

} // namespace authentication
} // namespace uda
