#pragma once

#include <string>
#include <unordered_map>
#include <curl/curl.h>

namespace uda::authentication {

class CurlWrapper {
public:
    CurlWrapper();
    ~CurlWrapper();

    CurlWrapper(const CurlWrapper&) = delete;
    CurlWrapper(CurlWrapper&&) = delete;
    CurlWrapper& operator=(const CurlWrapper&) = delete;
    CurlWrapper& operator=(CurlWrapper&&) = delete;

    // Function to perform a GET request
    [[nodiscard]] std::string perform_get_request(const std::string& url) const;

private:
    CURL* handle_;

    // Common options for both GET and POST requests
    void set_common_options(std::string* response_data) const;

    // Handle the cURL response and check for errors
    static void handle_curl_response(CURLcode response);

    // Handle exceptions and print error messages
    static void handle_error(const std::exception& e);
};

using PayloadType = std::unordered_map<std::string, std::string>;

PayloadType authenticate(const std::string& token);

}

struct AuthPayload {
    const uda::authentication::PayloadType* auth_payload;
};