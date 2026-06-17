#pragma once

#include <string>
#include <unordered_map>
#include <curl/curl.h>

namespace uda {
namespace authentication {

class CurlWrapper {
public:
    CurlWrapper();
    ~CurlWrapper();

    CurlWrapper(const CurlWrapper&) = delete;
    CurlWrapper(CurlWrapper&&) = delete;
    CurlWrapper& operator=(const CurlWrapper&) = delete;
    CurlWrapper& operator=(CurlWrapper&&) = delete;

    [[nodiscard]] std::string perform_get_request(const std::string& url) const;

private:
    CURL* handle_;

    void set_common_options(std::string* response_data) const;
    static void handle_curl_response(CURLcode response);
    static void handle_error(const std::exception& e);
};

// Decoded JWT payload as a flat string map.
// Nested JSON claims (e.g. realm_access) are preserved as JSON-encoded strings
// and can be traversed by ClaimPolicy using dot-separated paths.
using PayloadType = std::unordered_map<std::string, std::string>;

// Validate a bearer token using the OIDC/JWT configuration from the environment.
// Reads UDA_SERVER_OIDC_* (preferred) or UDA_SERVER_KEYCLOAK_* (legacy aliases).
// Throws std::runtime_error on any validation failure.
// Never logs the raw token value.
PayloadType authenticate(const std::string& token);

} // namespace authentication
} // namespace uda

struct AuthPayload {
    const uda::authentication::PayloadType* auth_payload;
};
