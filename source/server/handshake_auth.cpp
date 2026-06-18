#include "handshake_auth.h"

#include <authentication/auth_error.h>
#include <authentication/authLog.h>
#include <authentication/oidc_config.h>
#include <clientserver/udaDefines.h>
#include <clientserver/udaErrors.h>

namespace uda::server {

// Protocol version at which CLIENTFLAG_AUTHENTICATE and the AUTHENTICATION_BLOCK were
// introduced. Clients below this version cannot carry a bearer token at all.
static constexpr int OIDC_MIN_CLIENT_PROTOCOL = 11;

AuthGateResult check_oidc_client_auth(
    const CLIENT_BLOCK*              client_block,
    const std::string&               auth_mode,
    uda::authentication::HttpFetcher fetcher)
{
    using namespace uda::authentication;

    AuthGateResult result;

    const bool is_oidc = (auth_mode == "OAUTH" || auth_mode == "OIDC");
    if (!is_oidc) {
        AUTH_LOG(UDA_LOG_ERROR,
            "Auth: invalid UDA_SERVER_AUTHENTICATION value '%s'; expected OIDC or OAUTH\n",
            auth_mode.c_str());
        result.failed     = true;
        result.error_code = UDA_AUTH_ERR_INVALID_CONFIG;
        result.message    = std::string("Invalid UDA_SERVER_AUTHENTICATION value '")
                            + auth_mode + "' (expected OIDC or OAUTH)";
        return result;
    }

    // The auth block was introduced in protocol 11; older clients cannot carry a token.
    if (client_block->version < OIDC_MIN_CLIENT_PROTOCOL) {
        AUTH_LOG(UDA_LOG_ERROR,
            "Auth: client protocol version %d < %d — cannot carry OIDC bearer token; "
            "client must be upgraded\n",
            client_block->version, OIDC_MIN_CLIENT_PROTOCOL);
        result.failed     = true;
        result.error_code = UDA_AUTH_ERR_MISSING_TOKEN;
        result.message    = "OIDC authentication requires UDA client protocol version 11 "
                            "or later; upgrade the UDA client library";
        return result;
    }

    AUTH_LOG(UDA_LOG_DEBUG, "Auth: authentication block type: %u, payload length: %u\n",
             client_block->authenticationBlock.authentication_type,
             client_block->authenticationBlock.payload_length);

    if (client_block->authenticationBlock.authentication_type != UDA_AUTHENTICATION_OAUTH) {
        AUTH_LOG(UDA_LOG_ERROR,
            "Auth: no bearer token from client "
            "(authentication_type=%u, expected %u=OAUTH)\n",
            client_block->authenticationBlock.authentication_type,
            UDA_AUTHENTICATION_OAUTH);
        result.failed     = true;
        result.error_code = UDA_AUTH_ERR_MISSING_TOKEN;
        result.message    = "No bearer token provided; set UDA_AUTH_TOKEN on the client";
        return result;
    }

    const auto* raw_payload = client_block->authenticationBlock.payload;
    const auto  raw_len     = client_block->authenticationBlock.payload_length;

    if (raw_payload == nullptr) {
        AUTH_LOG(UDA_LOG_ERROR,
            "Auth: OAUTH authentication_type set but payload pointer is null "
            "(payload_length=%u) — possible XDR decode failure or internal caller bug\n",
            raw_len);
        result.failed     = true;
        result.error_code = UDA_AUTH_ERR_MISSING_TOKEN;
        result.message    = "Bearer token is missing (null payload)";
        return result;
    }

    // Bearer tokens are ASCII; a null byte indicates a malformed or hostile payload.
    for (unsigned int i = 0; i < raw_len; ++i) {
        if (raw_payload[i] == '\0') {
            AUTH_LOG(UDA_LOG_ERROR, "Auth: bearer token contains embedded null byte — rejected\n");
            result.failed     = true;
            result.error_code = UDA_AUTH_ERR_INVALID_TOKEN;
            result.message    = "Bearer token contains embedded null byte";
            return result;
        }
    }

    const std::string token{reinterpret_cast<const char*>(raw_payload), raw_len};
    AUTH_LOG(UDA_LOG_DEBUG, "Auth: token validation started (payload_length=%u)\n", raw_len);

    try {
        result.auth_payload = authenticate(token, OidcConfig::from_env(), fetcher);
        AUTH_LOG(UDA_LOG_INFO, "Auth: token validation succeeded\n");
    } catch (const AuthError& e) {
        const int code = authErrorToUdaCode(e.code);
        AUTH_LOG(UDA_LOG_ERROR, "Auth: token validation failed [%d]: %s\n", code, e.what());
        result.failed     = true;
        result.error_code = code;
        result.message    = e.what();
    } catch (const std::exception& e) {
        AUTH_LOG(UDA_LOG_ERROR, "Auth: unexpected exception: %s\n", e.what());
        result.failed     = true;
        result.error_code = 999;
        result.message    = e.what();
    }

    return result;
}

} // namespace uda::server
