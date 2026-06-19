#pragma once
// Shared test infrastructure for OIDC/JWT unit tests.
// Include in each test translation unit; helpers are in an anonymous namespace
// so each TU gets its own private copy (no ODR conflict, no shared statics).
//
// Usage in each test file:
//   #include "auth_test_helpers.h"
//   // Optional local singleton — define this in each file that needs one:
//   static const RsaTestKey& test_key() {
//       static const RsaTestKey k = generate_rsa_test_key();
//       return k;
//   }

#include <catch2/catch_test_macros.hpp>

#include <chrono>
#include <cstdlib>
#include <memory>
#include <set>
#include <stdexcept>
#include <string>
#include <unordered_map>
#include <vector>

#include <openssl/bn.h>
#include <openssl/bio.h>
#include <openssl/err.h>
#include <openssl/evp.h>
#include <openssl/pem.h>
#include <openssl/rsa.h>
#include <openssl/ssl.h>

#include <nlohmann/json.hpp>
#include <jwt-cpp/jwt.h>

#include <authentication/oauth_authentication.h>
#include <authentication/oidc_config.h>
#include <authentication/auth_error.h>
#include <authentication/claim_policy.h>
#include <authentication/jwks_cache.h>

using namespace uda::authentication;
using json = nlohmann::json;

namespace {

// ---------------------------------------------------------------------------
// RAII env var helpers

struct EnvGuard {
    std::string name;
    EnvGuard(const char* n, const char* v) : name(n) { setenv(n, v, 1); }
    ~EnvGuard() { unsetenv(name.c_str()); }
};

struct EnvCleaner {
    std::vector<std::string> names;
    EnvCleaner(std::initializer_list<const char*> ns) {
        for (const char* n : ns) { names.emplace_back(n); unsetenv(n); }
    }
    ~EnvCleaner() { for (const auto& n : names) unsetenv(n.c_str()); }
};

// ---------------------------------------------------------------------------
// RSA test key

struct RsaTestKey {
    std::string private_pem;
    std::string public_pem;
    std::string jwks_json;
    std::string kid = "test-key-1";
};

static std::string bio_to_string(BIO* bio)
{
    char* data = nullptr;
    long len = BIO_get_mem_data(bio, &data);
    return std::string(data, static_cast<size_t>(len));
}

static std::string bn_to_base64url(const BIGNUM* bn)
{
    const int nbytes = BN_num_bytes(bn);
    std::vector<unsigned char> buf(static_cast<size_t>(nbytes));
    BN_bn2bin(bn, buf.data());

    BIO* chain = BIO_push(BIO_new(BIO_f_base64()), BIO_new(BIO_s_mem()));
    BIO_set_flags(chain, BIO_FLAGS_BASE64_NO_NL);
    BIO_write(chain, buf.data(), nbytes);
    BIO_flush(chain);
    BUF_MEM* bptr = nullptr;
    BIO_get_mem_ptr(chain, &bptr);
    std::string result(bptr->data, bptr->length);
    BIO_free_all(chain);

    for (char& c : result) {
        if      (c == '+') c = '-';
        else if (c == '/') c = '_';
    }
    while (!result.empty() && result.back() == '=') result.pop_back();
    return result;
}

static RsaTestKey generate_rsa_test_key(const std::string& kid = "test-key-1")
{
    EVP_PKEY_CTX* pctx = EVP_PKEY_CTX_new_id(EVP_PKEY_RSA, nullptr);
    EVP_PKEY_keygen_init(pctx);
    EVP_PKEY_CTX_set_rsa_keygen_bits(pctx, 2048);
    EVP_PKEY* pkey = nullptr;
    EVP_PKEY_keygen(pctx, &pkey);
    EVP_PKEY_CTX_free(pctx);

    BIO* privbio = BIO_new(BIO_s_mem());
    PEM_write_bio_PrivateKey(privbio, pkey, nullptr, nullptr, 0, nullptr, nullptr);
    std::string private_pem = bio_to_string(privbio);
    BIO_free(privbio);

    BIO* pubbio = BIO_new(BIO_s_mem());
    PEM_write_bio_PUBKEY(pubbio, pkey);
    std::string public_pem = bio_to_string(pubbio);
    BIO_free(pubbio);

    const BIGNUM* n_bn = nullptr;
    const BIGNUM* e_bn = nullptr;

#if OPENSSL_VERSION_NUMBER >= 0x30000000L
    BIGNUM* n_tmp = nullptr;
    BIGNUM* e_tmp = nullptr;
    EVP_PKEY_get_bn_param(pkey, "n", &n_tmp);
    EVP_PKEY_get_bn_param(pkey, "e", &e_tmp);
    n_bn = n_tmp;
    e_bn = e_tmp;
#else
    const RSA* rsa = EVP_PKEY_get0_RSA(pkey);
    RSA_get0_key(rsa, &n_bn, &e_bn, nullptr);
#endif

    const std::string n_b64 = bn_to_base64url(n_bn);
    const std::string e_b64 = bn_to_base64url(e_bn);

#if OPENSSL_VERSION_NUMBER >= 0x30000000L
    BN_free(const_cast<BIGNUM*>(n_bn));
    BN_free(const_cast<BIGNUM*>(e_bn));
#endif

    EVP_PKEY_free(pkey);

    const json jwks = {
        {"keys", json::array({
            {
                {"kty", "RSA"},
                {"use", "sig"},
                {"alg", "RS256"},
                {"kid", kid},
                {"n",   n_b64},
                {"e",   e_b64},
            }
        })}
    };

    RsaTestKey k;
    k.private_pem = private_pem;
    k.public_pem  = public_pem;
    k.jwks_json   = jwks.dump();
    k.kid         = kid;
    return k;
}

// ---------------------------------------------------------------------------
// Token builder
//
// Builds a signed JWT with provider-configurable claims.
//
// extra_claims: arbitrary JSON object merged into the payload, useful for
//   provider-specific claims like realm_access, wlcg.groups, resource_access.
//   Nested objects and arrays are passed through picojson so jwt-cpp can encode them.
//   Keys that duplicate standard fields (iss, aud, azp, scope) take no effect — the
//   standard fields are set first and jwt-cpp ignores duplicate claim names.
//
// audience_arr: when non-empty, overrides audience and sets an array aud claim.
//
// nbf_offset_secs: when > 0, sets nbf to (now + offset), producing a not-yet-valid token.

struct TokenBuilder {
    std::string              issuer          = "https://test-issuer.example.com";
    std::string              audience        = "test-audience";
    std::string              azp             = "test-client";
    std::string              scope           = "openid uda.read";
    std::string              kid             = "test-key-1";
    int                      exp_secs        = 300;  // negative → already expired
    std::string              sign_alg        = "RS256";
    int                      nbf_offset_secs = 0;    // > 0 → not yet valid
    std::vector<std::string> audience_arr;            // when set, serialises as JSON array
    json                     extra_claims;            // merged into payload via picojson

    std::string build(const RsaTestKey& k) const
    {
        auto now = std::chrono::system_clock::now();
        auto exp = now + std::chrono::seconds(exp_secs);

        auto creator = jwt::create()
            .set_type("JWT")
            .set_issuer(issuer)
            .set_issued_at(now)
            .set_expires_at(exp)
            .set_payload_claim("azp",   jwt::claim(std::string(azp)))
            .set_payload_claim("scope", jwt::claim(std::string(scope)));

        if (!audience_arr.empty()) {
            std::set<std::string> aud_set(audience_arr.begin(), audience_arr.end());
            creator.set_audience(aud_set);
        } else {
            creator.set_audience(audience);
        }

        if (!kid.empty()) creator.set_key_id(kid);

        if (nbf_offset_secs > 0) {
            creator.set_not_before(now + std::chrono::seconds(nbf_offset_secs));
        }

        // Merge extra_claims by round-tripping through picojson so jwt-cpp
        // can encode nested objects and arrays.
        for (const auto& [cname, cval] : extra_claims.items()) {
            picojson::value pv;
            const std::string err = picojson::parse(pv, cval.dump());
            if (err.empty()) {
                creator.set_payload_claim(cname, jwt::claim(pv));
            }
        }

        if (sign_alg == "RS384") {
            return creator.sign(jwt::algorithm::rs384(k.public_pem, k.private_pem, "", ""));
        }
        if (sign_alg == "RS512") {
            return creator.sign(jwt::algorithm::rs512(k.public_pem, k.private_pem, "", ""));
        }
        return creator.sign(jwt::algorithm::rs256(k.public_pem, k.private_pem, "", ""));
    }
};

// ---------------------------------------------------------------------------
// Mock HTTP fetcher — returns fixed JWKS for any URL

static HttpFetcher make_jwks_fetcher(const std::string& jwks_json)
{
    return [jwks_json](const std::string& /*url*/) { return jwks_json; };
}

// ---------------------------------------------------------------------------
// OidcConfig factory

static OidcConfig make_cfg(
    const std::string& issuer  = "https://test-issuer.example.com",
    const std::string& audience = "test-audience",
    const std::string& jwks_uri = "https://test-issuer.example.com/jwks")
{
    OidcConfig cfg;
    cfg.issuer           = issuer;
    cfg.audience         = audience;
    cfg.jwks_uri         = jwks_uri;
    cfg.allowed_algs     = {"RS256"};
    cfg.verify_issuer    = true;
    cfg.verify_audience  = true;
    cfg.clock_skew_seconds = 5;
    return cfg;
}

// ---------------------------------------------------------------------------
// Catch2 helper: assert an AuthError with a specific code is thrown

template<typename F>
static void require_auth_error(F&& fn, AuthErrorCode expected_code)
{
    try {
        fn();
        FAIL("Expected AuthError to be thrown but nothing was thrown");
    } catch (const AuthError& e) {
        REQUIRE(e.code == expected_code);
    }
}

// ---------------------------------------------------------------------------
// Payload assertion helpers

// Return true if the JSON array in payload[key] contains the string element.
static bool payload_array_contains(const PayloadType& payload,
                                   const std::string& key,
                                   const std::string& element)
{
    const auto it = payload.find(key);
    if (it == payload.end()) return false;
    try {
        const auto arr = json::parse(it->second);
        if (!arr.is_array()) return false;
        for (const auto& v : arr) {
            if (v.is_string() && v.get<std::string>() == element) return true;
        }
    } catch (...) {}
    return false;
}

// Return true if payload[key] exists and its string value equals expected.
static bool payload_str_eq(const PayloadType& payload,
                           const std::string& key,
                           const std::string& expected)
{
    const auto it = payload.find(key);
    return it != payload.end() && it->second == expected;
}

} // anonymous namespace
