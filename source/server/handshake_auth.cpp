#include "handshake_auth.h"

#include <authentication/auth_error.h>
#include <authentication/authLog.h>
#include <authentication/oidc_config.h>
#include <authentication/refusal_log.h>
#include <authentication/tlsMode.h>
#include <clientserver/udaDefines.h>
#include <clientserver/udaErrors.h>

#include <cstdlib>

namespace uda::server {

using namespace uda::authentication;

// Protocol version at which CLIENTFLAG_AUTHENTICATE and the AUTHENTICATION_BLOCK were
// introduced. Clients below this version cannot carry a bearer token at all.
static constexpr int OIDC_MIN_CLIENT_PROTOCOL = 11;

namespace {

// Map AuthErrorCode to the RefusalReason used in the audit log. The token rejections are
// kept distinct here because they are what an operator acts on: an expired token means
// "get a new one", a bad signature means something else entirely.
RefusalReason oidc_refusal_reason(AuthErrorCode code) noexcept
{
    switch (code) {
        case AuthErrorCode::MissingToken:      return RefusalReason::OidcTokenMissing;
        case AuthErrorCode::TokenExpired:      return RefusalReason::OidcTokenExpired;
        case AuthErrorCode::TokenBadIssuer:    return RefusalReason::OidcTokenBadIssuer;
        case AuthErrorCode::TokenBadAudience:  return RefusalReason::OidcTokenBadAudience;
        case AuthErrorCode::InvalidToken:      return RefusalReason::OidcTokenInvalidSignature;
        case AuthErrorCode::ClaimPolicyFailed: return RefusalReason::OidcClaimPolicyFailed;
        case AuthErrorCode::DiscoveryFailed:
        case AuthErrorCode::JwksFetchFailed:
        case AuthErrorCode::InvalidConfig:     return RefusalReason::OidcConfigInvalid;
        case AuthErrorCode::TlsConfigError:
        case AuthErrorCode::TlsHandshakeFailed:
        case AuthErrorCode::TlsHostnameMismatch:
            break; // not reachable from the OIDC gate
    }
    return RefusalReason::OidcTokenInvalidSignature;
}

// The short token_error tag that accompanies a refusal record.
const char* token_error_tag(AuthErrorCode code) noexcept
{
    switch (code) {
        case AuthErrorCode::MissingToken:      return "missing";
        case AuthErrorCode::TokenExpired:      return "expired";
        case AuthErrorCode::TokenBadIssuer:    return "bad_issuer";
        case AuthErrorCode::TokenBadAudience:  return "bad_audience";
        case AuthErrorCode::ClaimPolicyFailed: return "claim_policy";
        case AuthErrorCode::InvalidConfig:
        case AuthErrorCode::DiscoveryFailed:
        case AuthErrorCode::JwksFetchFailed:   return "config";
        default:                               return "invalid";
    }
}

// Everything a refusal at this gate needs. `cfg` is the config already read for this
// request — the issuer is taken from it rather than re-reading the environment, so the
// record names the issuer that was actually used.
struct Gate {
    const CLIENT_BLOCK* client_block;
    const OidcConfig*   cfg; // may be null when the failure happened before config was read

    // Fill in the result and write the audit record. Returns the result so call sites can
    // `return gate.refuse(...)` and read as a list of policy checks.
    AuthGateResult refuse(RefusalReason reason,
                          int           error_code,
                          std::string   message,
                          const char*   token_error = "",
                          RefusalStage  stage = RefusalStage::OidcAuth) const
    {
        AuthGateResult result;
        result.failed     = true;
        result.error_code = error_code;
        result.message    = std::move(message);

        RefusalRecord rec;
        rec.stage           = stage;
        rec.reason          = reason;
        rec.uda_error_code  = error_code;
        rec.message         = result.message;
        rec.peer            = get_peer_info(INETD_SOCKET_FD);
        rec.client_version  = client_block->version;
        rec.client_username = client_block->uid;
        rec.token_error     = token_error;
        if (cfg != nullptr) {
            rec.oidc_issuer = cfg->issuer;
        }
        record_refused_request(rec);

        return result;
    }
};

} // anonymous namespace

AuthGateResult check_oidc_client_auth(
    const CLIENT_BLOCK*              client_block,
    const std::string&               auth_mode,
    uda::authentication::HttpFetcher fetcher)
{
    Gate gate{client_block, nullptr};

    const bool is_oidc = (auth_mode == "OAUTH" || auth_mode == "OIDC");
    if (!is_oidc) {
        AUTH_LOG(UDA_LOG_ERROR,
            "Auth: invalid UDA_SERVER_AUTHENTICATION value '%s'; expected OIDC or OAUTH\n",
            auth_mode.c_str());
        return gate.refuse(RefusalReason::OidcConfigInvalid, UDA_AUTH_ERR_INVALID_CONFIG,
            "Invalid UDA_SERVER_AUTHENTICATION value '" + auth_mode + "' (expected OIDC or OAUTH)",
            "config");
    }

    // The auth block was introduced in protocol 11; older clients cannot carry a token.
    if (client_block->version < OIDC_MIN_CLIENT_PROTOCOL) {
        AUTH_LOG(UDA_LOG_ERROR,
            "Auth: client protocol version %d < %d — cannot carry OIDC bearer token; "
            "client must be upgraded\n",
            client_block->version, OIDC_MIN_CLIENT_PROTOCOL);
        return gate.refuse(RefusalReason::ClientProtocolTooOld, UDA_AUTH_ERR_MISSING_TOKEN,
            "OIDC authentication requires UDA client protocol version 11 or later; "
            "upgrade the UDA client library",
            "missing", RefusalStage::Protocol);
    }

    AUTH_LOG(UDA_LOG_DEBUG, "Auth: authentication block type: %u, payload length: %u\n",
             client_block->authenticationBlock.authentication_type,
             client_block->authenticationBlock.payload_length);

    if (client_block->authenticationBlock.authentication_type != UDA_AUTHENTICATION_OAUTH) {
        AUTH_LOG(UDA_LOG_ERROR,
            "Auth: no bearer token from client (authentication_type=%u, expected %u=OAUTH)\n",
            client_block->authenticationBlock.authentication_type, UDA_AUTHENTICATION_OAUTH);
        return gate.refuse(RefusalReason::OidcTokenMissing, UDA_AUTH_ERR_MISSING_TOKEN,
            "No bearer token provided; set UDA_AUTH_TOKEN on the client", "missing");
    }

    const auto* raw_payload = client_block->authenticationBlock.payload;
    const auto  raw_len     = client_block->authenticationBlock.payload_length;

    if (raw_payload == nullptr) {
        AUTH_LOG(UDA_LOG_ERROR,
            "Auth: OAUTH authentication_type set but payload pointer is null "
            "(payload_length=%u) — possible XDR decode failure or internal caller bug\n",
            raw_len);
        return gate.refuse(RefusalReason::OidcTokenMissing, UDA_AUTH_ERR_MISSING_TOKEN,
            "Bearer token is missing (null payload)", "missing");
    }

    // Bearer tokens are ASCII; a null byte indicates a malformed or hostile payload.
    for (unsigned int i = 0; i < raw_len; ++i) {
        if (raw_payload[i] == '\0') {
            AUTH_LOG(UDA_LOG_ERROR, "Auth: bearer token contains embedded null byte — rejected\n");
            return gate.refuse(RefusalReason::OidcTokenInvalidSignature,
                UDA_AUTH_ERR_INVALID_TOKEN, "Bearer token contains embedded null byte", "invalid");
        }
    }

    const std::string token{reinterpret_cast<const char*>(raw_payload), raw_len};
    AUTH_LOG(UDA_LOG_DEBUG, "Auth: token validation started (payload_length=%u)\n", raw_len);

    // The token just arrived in clear text if this connection is not TLS-protected.
    // Accepted (local development against a plain-HTTP IdP is a real workflow) but
    // recorded, because in production it means the deployment is misconfigured.
    if (getServerTlsMode() == TlsMode::Off && std::getenv("UDA_ALLOW_TOKEN_WITHOUT_TLS") == nullptr) {
        AUTH_LOG(UDA_LOG_WARN,
            "Auth: bearer token received over an unencrypted connection "
            "(UDA_SERVER_TLS_MODE=off) — tokens are exposed on the network path. "
            "Enable TLS, or set UDA_ALLOW_TOKEN_WITHOUT_TLS=1 to acknowledge this.\n");
    }

    // Read the OIDC configuration once, and use the same object for validation and for
    // the audit record.
    OidcConfig cfg;
    try {
        cfg = OidcConfig::from_env();
    } catch (const std::exception& e) {
        AUTH_LOG(UDA_LOG_ERROR, "Auth: OIDC configuration is invalid: %s\n", e.what());
        return gate.refuse(RefusalReason::OidcConfigInvalid, UDA_AUTH_ERR_INVALID_CONFIG,
            std::string("OIDC configuration is invalid: ") + e.what(), "config");
    }
    gate.cfg = &cfg;

    AuthGateResult result;
    try {
        result.auth_payload = authenticate(token, cfg, fetcher);
        AUTH_LOG(UDA_LOG_INFO, "Auth: token validation succeeded\n");
    } catch (const AuthError& e) {
        const int code = authErrorToUdaCode(e.code);
        AUTH_LOG(UDA_LOG_ERROR, "Auth: token validation failed [%d]: %s\n", code, e.what());
        return gate.refuse(oidc_refusal_reason(e.code), code, e.what(), token_error_tag(e.code));
    } catch (const std::exception& e) {
        AUTH_LOG(UDA_LOG_ERROR, "Auth: unexpected exception: %s\n", e.what());
        return gate.refuse(RefusalReason::OidcTokenInvalidSignature, 999, e.what(), "invalid");
    }

    return result;
}

} // namespace uda::server
