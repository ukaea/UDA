#include "http_get.h"
#include "auth_error.h"

#include <cstdlib>
#include <mutex>
#include <string>

#include <curl/curl.h>
#include <nlohmann/json.hpp>

namespace uda {
namespace authentication {

namespace {

// cURL global init — once per process, never cleaned up (safe for a library).
void ensure_curl_initialized()
{
    static std::once_flag flag;
    std::call_once(flag, [] { curl_global_init(CURL_GLOBAL_DEFAULT); });
}

size_t write_callback(void* contents, size_t size, size_t count, std::string* output)
{
    const size_t total = size * count;
    output->append(static_cast<char*>(contents), total);
    return total;
}

} // anonymous namespace

std::string http_get(const std::string& url, const HttpGetOptions& opts)
{
    if (opts.require_https) {
        const bool allow_http = (std::getenv("UDA_SERVER_OIDC_ALLOW_HTTP") != nullptr);
        const bool is_https   = url.size() >= 8 && url.compare(0, 8, "https://") == 0;
        if (!allow_http && !is_https) {
            throw AuthError(AuthErrorCode::InvalidConfig,
                "OIDC fetch rejected non-HTTPS URL '" + url + "' — "
                "set UDA_SERVER_OIDC_ALLOW_HTTP=1 to override (testing only)");
        }
    }

    ensure_curl_initialized();
    CURL* handle = curl_easy_init();
    if (!handle) {
        throw AuthError(AuthErrorCode::JwksFetchFailed, "curl_easy_init() failed");
    }

    std::string response;
    char errbuf[CURL_ERROR_SIZE] = {};

    curl_easy_setopt(handle, CURLOPT_URL,               url.c_str());
    curl_easy_setopt(handle, CURLOPT_WRITEFUNCTION,     write_callback);
    curl_easy_setopt(handle, CURLOPT_WRITEDATA,         &response);
    curl_easy_setopt(handle, CURLOPT_ERRORBUFFER,       errbuf);
    curl_easy_setopt(handle, CURLOPT_CONNECTTIMEOUT_MS, opts.connect_timeout_ms);
    curl_easy_setopt(handle, CURLOPT_TIMEOUT_MS,        opts.total_timeout_ms);
    curl_easy_setopt(handle, CURLOPT_FAILONERROR,       1L);
    curl_easy_setopt(handle, CURLOPT_FOLLOWLOCATION,    opts.max_redirects > 0 ? 1L : 0L);
    curl_easy_setopt(handle, CURLOPT_MAXREDIRS,         opts.max_redirects);

    const CURLcode rc = curl_easy_perform(handle);

    if (rc != CURLE_OK) {
        long http_code = 0;
        curl_easy_getinfo(handle, CURLINFO_RESPONSE_CODE, &http_code);
        curl_easy_cleanup(handle);
        throw AuthError(AuthErrorCode::JwksFetchFailed,
            "HTTP GET '" + url + "' failed: " +
            (errbuf[0] ? errbuf : curl_easy_strerror(rc)) +
            (http_code > 0 ? " (HTTP " + std::to_string(http_code) + ")" : ""));
    }

    curl_easy_cleanup(handle);
    return response;
}

std::string http_put(
    const std::string& url,
    const nlohmann::json& payload,
    const HttpGetOptions& opts)
{
    if (opts.require_https) {
        const bool allow_http =
            std::getenv("UDA_SERVER_OIDC_ALLOW_HTTP") != nullptr;

        const bool is_https =
            url.size() >= 8 && url.compare(0, 8, "https://") == 0;

        if (!allow_http && !is_https) {
            throw AuthError(
                AuthErrorCode::InvalidConfig,
                "OIDC fetch rejected non-HTTPS URL '" + url + "' - "
                "set UDA_SERVER_OIDC_ALLOW_HTTP=1 to override (testing only)");
        }
    }

    ensure_curl_initialized();

    CURL* handle = curl_easy_init();
    if (!handle) {
        throw AuthError(
            AuthErrorCode::JwksFetchFailed,
            "curl_easy_init() failed");
    }

    std::string response;
    char err_buf[CURL_ERROR_SIZE] = {};

    // Tell the server that the request body contains JSON.
    curl_slist* headers = nullptr;
    headers = curl_slist_append(
        headers,
        "Content-Type: application/json");

    curl_easy_setopt(handle, CURLOPT_URL, url.c_str());

    // Perform a PUT request.
    curl_easy_setopt(handle, CURLOPT_CUSTOMREQUEST, "PUT");

    std::string json_payload = payload.dump();

    // Send JSON as the request body.
    curl_easy_setopt(handle, CURLOPT_POSTFIELDS, json_payload.c_str());
    curl_easy_setopt(
        handle,
        CURLOPT_POSTFIELDSIZE,
        static_cast<long>(json_payload.size()));

    curl_easy_setopt(handle, CURLOPT_HTTPHEADER, headers);

    curl_easy_setopt(handle, CURLOPT_WRITEFUNCTION, write_callback);
    curl_easy_setopt(handle, CURLOPT_WRITEDATA, &response);
    curl_easy_setopt(handle, CURLOPT_ERRORBUFFER, err_buf);

    curl_easy_setopt(
        handle,
        CURLOPT_CONNECTTIMEOUT_MS,
        opts.connect_timeout_ms);

    curl_easy_setopt(
        handle,
        CURLOPT_TIMEOUT_MS,
        opts.total_timeout_ms);

    curl_easy_setopt(handle, CURLOPT_FAILONERROR, 1L);

    curl_easy_setopt(
        handle,
        CURLOPT_FOLLOWLOCATION,
        opts.max_redirects > 0 ? 1L : 0L);

    curl_easy_setopt(
        handle,
        CURLOPT_MAXREDIRS,
        opts.max_redirects);

    const CURLcode rc = curl_easy_perform(handle);

    if (rc != CURLE_OK) {
        long http_code = 0;

        curl_easy_getinfo(
            handle,
            CURLINFO_RESPONSE_CODE,
            &http_code);

        const std::string error_message =
            "HTTP PUT '" + url + "' failed: " +
            (err_buf[0] ? err_buf : curl_easy_strerror(rc)) +
            (http_code > 0
                ? " (HTTP " + std::to_string(http_code) + ")"
                : "");

        curl_slist_free_all(headers);
        curl_easy_cleanup(handle);

        throw AuthError(
            AuthErrorCode::JwksFetchFailed,
            error_message);
    }

    curl_slist_free_all(headers);
    curl_easy_cleanup(handle);

    return response;
}

} // namespace authentication
} // namespace uda
