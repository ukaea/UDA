#pragma once

#include <stdexcept>
#include <string>

#include "clientserver/udaErrors.h"

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

// Map an AuthErrorCode to its stable UDA error code integer.
// Numeric values are defined in clientserver/udaErrors.h.
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
