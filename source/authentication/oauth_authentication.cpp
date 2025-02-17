    /*
        Things to do:
        - Check for token expiry using `exp` claim
        - Make sure curl is using HTTPS
    */

#include <iostream>
#include <stdexcept>
#include <curl/curl.h>
#include <nlohmann/json.hpp>
#include <jwt-cpp/jwt.h>

#include "logging/logging.h"

using json = nlohmann::json;

// Callback function to handle the response data
size_t write_callback(void* contents, const size_t size, const size_t count, std::string* output) {
    const size_t total_size = size * count;
    output->append(static_cast<char*>(contents), total_size);
    return total_size;
}

// RAII Wrapper for CURL
class CurlWrapper {
public:
    CurlWrapper() {
        curl_global_init(CURL_GLOBAL_DEFAULT);
        handle_ = curl_easy_init();
        if (!handle_) {
            throw std::runtime_error("Failed to initialize cURL.\n");
        }
    }

    ~CurlWrapper() {
        if (handle_) {
            curl_easy_cleanup(handle_);
        }
        curl_global_cleanup();
    }

    // Function to perform a GET request
    [[nodiscard]] std::string perform_get_request(const std::string& url) const {
        std::string response;
        try {
            curl_easy_setopt(handle_, CURLOPT_URL, url.c_str());
            set_common_options(&response);
            const CURLcode curl_response = curl_easy_perform(handle_);
            handle_curl_response(curl_response);
        } catch (const std::exception& e) {
            handle_error(e);
        }
        return response;
    }

private:
    CURL* handle_;

    // Common options for both GET and POST requests
    void set_common_options(std::string* response_data) const {
        curl_easy_setopt(handle_, CURLOPT_WRITEFUNCTION, write_callback);
        curl_easy_setopt(handle_, CURLOPT_WRITEDATA, response_data); // pointer to string is passed to write_callback
    }

    // Handle the cURL response and check for errors
    static void handle_curl_response(const CURLcode response) {
        if (response != CURLE_OK) {
            throw std::runtime_error("curl_easy_perform() failed: " + std::string(curl_easy_strerror(response)) + "\n");
        }
    }

    // Handle exceptions and print error messages
    static void handle_error(const std::exception& e) {
        UDA_LOG(UDA_LOG_ERROR, "CURL error: %s\n", e.what());
        throw; // Re-throw the exception for the caller to handle
    }
};

class OAuthCTX {
public:
    OAuthCTX(const std::string& realm_url, const std::string& client_id) {
        try{
            client_id_ = client_id;
            openid_configuration_endpoint_ = realm_url + "/.well-known/openid-configuration";
            json oauth_openid_conf = get_json_from_endpoint(openid_configuration_endpoint_);
            issuer_ = oauth_openid_conf["issuer"];
            token_endpoint_ = oauth_openid_conf["token_endpoint"];
            jwks_uri_ = oauth_openid_conf["jwks_uri"];
            jwks_ = get_json_from_endpoint(jwks_uri_);

        } catch (const std::exception& e) {
            UDA_LOG(UDA_LOG_ERROR, "Keycloak: Context creation error: %s\n", e.what());
            throw; // Re-throw the exception for the caller to handle
        }
    }

    static json get_json_from_endpoint(const std::string& endpoint) {
        const CurlWrapper curl;
        std::string response = curl.perform_get_request(endpoint);
        return json::parse(response);
    }

    // Verify the token signature using the fetched JWKS
    // https://github.com/Thalhammer/jwt-cpp/blob/master/example/jwks-verify.cpp
    void verify_token_signature(const std::string& token) const {
        try {
            auto decoded_jwt = jwt::decode(token);
            auto jwk = jwt::parse_jwks(jwks_.dump()).get_jwk(decoded_jwt.get_key_id());
            UDA_LOG(UDA_LOG_DEBUG, "Keycloak: JWK Found\n");
            auto x5c = jwk.get_x5c_key_value();
            if (!x5c.empty()) {
                UDA_LOG(UDA_LOG_DEBUG, "Keycloak: Verifying token with x5c component\n");
                auto verifier =
                    jwt::verify()
                        .allow_algorithm(jwt::algorithm::rs256(jwt::helper::convert_base64_der_to_pem(x5c), "", "", ""))
                        .with_issuer(issuer_)
                        .with_claim("azp", jwt::claim(client_id_))
                        .leeway(60UL);
                verifier.verify(decoded_jwt);

            } else {
                UDA_LOG(UDA_LOG_DEBUG, "Keycloak: Verifying token with RSA components\n");
                const auto modulus = jwk.get_jwk_claim("n").as_string();
                const auto exponent = jwk.get_jwk_claim("e").as_string();
                auto verifier = jwt::verify()
                                    .allow_algorithm(jwt::algorithm::rs256(
                                        jwt::helper::create_public_key_from_rsa_components(modulus, exponent)))
                                    .with_issuer(issuer_)
                                    .with_claim("azp", jwt::claim(client_id_))
                                    .leeway(60UL);
                verifier.verify(decoded_jwt);

            }
            UDA_LOG(UDA_LOG_DEBUG, "Keycloak: Token signature verified successfully\n");
        } catch (const std::exception& e) {
            UDA_LOG(UDA_LOG_ERROR, "Keycloak: Token signature verification failed: %s\n", e.what());
            throw; // Re-throw the exception for the caller to handle
        }
    }

    private:
        std::string client_id_;
        std::string issuer_;
        std::string openid_configuration_endpoint_;
        std::string token_endpoint_;
        std::string jwks_uri_;
        json jwks_;
};
