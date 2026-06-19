#pragma once

#include <string>
#include <authentication/oauth_authentication.h>
#include <clientserver/udaStructs.h>

namespace uda::server {

struct AuthGateResult {
    bool        failed    = false;
    int         error_code = 0;
    std::string message;
    uda::authentication::PayloadType auth_payload;
};

// Validates the authentication block in client_block for the given auth_mode.
// Returns a plain result struct and does NOT manipulate the UDA error stack —
// callers (handshakeClient) translate the result into UDA_ADD_ERROR calls.
//
// The fetcher defaults to curl_http_fetch; inject a mock for unit tests.
AuthGateResult check_oidc_client_auth(
    const CLIENT_BLOCK*              client_block,
    const std::string&               auth_mode,
    uda::authentication::HttpFetcher fetcher = uda::authentication::curl_http_fetch);

} // namespace uda::server
