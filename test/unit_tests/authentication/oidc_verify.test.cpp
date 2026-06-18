// JWT verification tests using injected mock HTTP fetcher.
// RSA key pairs are generated at test startup (no hardcoded keys).
// Tests call authenticate(token, cfg, fetcher) directly — no network, no cURL.

#include <catch2/catch_test_macros.hpp>

#include <chrono>
#include <cstdlib>
#include <memory>
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
#include <authentication/jwks_cache.h>

using namespace uda::authentication;
using json = nlohmann::json;

// ---------------------------------------------------------------------------
// Test RSA key helpers

namespace {

// RAII env var guard
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

struct RsaTestKey {
    std::string private_pem;
    std::string public_pem;
    std::string jwks_json;
    std::string kid = "test-key-1";
};

std::string bio_to_string(BIO* bio)
{
    char* data;
    long len = BIO_get_mem_data(bio, &data);
    return std::string(data, static_cast<size_t>(len));
}

std::string bn_to_base64url(const BIGNUM* bn)
{
    const int nbytes = BN_num_bytes(bn);
    std::vector<unsigned char> buf(static_cast<size_t>(nbytes));
    BN_bn2bin(bn, buf.data());

    BIO* chain = BIO_push(BIO_new(BIO_f_base64()), BIO_new(BIO_s_mem()));
    BIO_set_flags(chain, BIO_FLAGS_BASE64_NO_NL);
    BIO_write(chain, buf.data(), nbytes);
    BIO_flush(chain);
    BUF_MEM* bptr;
    BIO_get_mem_ptr(chain, &bptr);
    std::string result(bptr->data, bptr->length);
    BIO_free_all(chain);

    for (char& c : result) {
        if (c == '+') c = '-';
        else if (c == '/') c = '_';
    }
    while (!result.empty() && result.back() == '=') result.pop_back();
    return result;
}

RsaTestKey generate_rsa_test_key(const std::string& kid = "test-key-1")
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

    // Extract RSA n and e for JWKS construction
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

    json jwks = {
        {"keys", json::array({
            {
                {"kty", "RSA"},
                {"use", "sig"},
                {"alg", "RS256"},
                {"kid", kid},
                {"n", n_b64},
                {"e", e_b64},
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

// Generate one key pair per test binary (expensive but necessary)
const RsaTestKey& test_key()
{
    static const RsaTestKey key = generate_rsa_test_key();
    return key;
}

// Build a signed JWT with sensible defaults; callers can override fields.
struct TokenBuilder {
    std::string issuer    = "https://test-issuer.example.com";
    std::string audience  = "test-audience";
    std::string azp       = "test-client";
    std::string scope     = "openid uda.read";
    std::string kid       = "test-key-1";
    int         exp_secs  = 300; // seconds from now; negative for already-expired
    std::string sign_alg  = "RS256"; // "RS256", "RS384", or "RS512"

    std::string build(const RsaTestKey& k) const
    {
        auto now = std::chrono::system_clock::now();
        auto exp = now + std::chrono::seconds(exp_secs);

        auto creator = jwt::create()
            .set_type("JWT")
            .set_issuer(issuer)
            .set_audience(audience)
            .set_issued_at(now)
            .set_expires_at(exp)
            .set_payload_claim("azp",   jwt::claim(azp))
            .set_payload_claim("scope", jwt::claim(scope));

        if (!kid.empty()) {
            creator.set_key_id(kid);
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

// Mock fetcher: returns jwks_json for any URL.
HttpFetcher make_jwks_fetcher(const std::string& jwks_json)
{
    return [jwks_json](const std::string& /* url */) { return jwks_json; };
}

// Build a minimal OidcConfig for tests.
OidcConfig make_cfg(const std::string& issuer   = "https://test-issuer.example.com",
                    const std::string& audience  = "test-audience",
                    const std::string& jwks_uri  = "https://test-issuer.example.com/jwks")
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

// Catch2 helper: require an AuthError with a specific code.
template<typename F>
void require_auth_error(F&& fn, AuthErrorCode expected_code)
{
    try {
        fn();
        FAIL("Expected AuthError to be thrown");
    } catch (const AuthError& e) {
        REQUIRE( e.code == expected_code );
    }
}

} // namespace

// ---------------------------------------------------------------------------
// Happy-path: valid token passes all checks

TEST_CASE("authenticate succeeds with valid RS256 token", "[oidc_verify]")
{
    const auto& k = test_key();
    const std::string token = TokenBuilder{}.build(k);
    const OidcConfig cfg    = make_cfg();
    const HttpFetcher fetch = make_jwks_fetcher(k.jwks_json);

    PayloadType payload;
    REQUIRE_NOTHROW( payload = authenticate(token, cfg, fetch) );
    REQUIRE( payload.count("azp") == 1 );
    REQUIRE( payload.at("azp") == "test-client" ); // string claims stored as raw strings (no JSON quotes)
}

// ---------------------------------------------------------------------------
// Empty / missing token

TEST_CASE("authenticate throws on empty token", "[oidc_verify]")
{
    const OidcConfig cfg    = make_cfg();
    const HttpFetcher fetch = make_jwks_fetcher(test_key().jwks_json);

    REQUIRE_THROWS_AS( authenticate("", cfg, fetch), AuthError );
    require_auth_error([&]{ authenticate("", cfg, fetch); }, AuthErrorCode::MissingToken);
}

// ---------------------------------------------------------------------------
// Missing OIDC config

TEST_CASE("authenticate throws when no issuer and no jwks_uri configured", "[oidc_verify]")
{
    OidcConfig cfg;
    cfg.audience         = "aud";
    cfg.allowed_algs     = {"RS256"};
    // issuer and jwks_uri both empty
    const HttpFetcher fetch = make_jwks_fetcher(test_key().jwks_json);

    REQUIRE_THROWS_AS( authenticate("some-token", cfg, fetch), AuthError );
    require_auth_error(
        [&]{ authenticate("some-token", cfg, fetch); },
        AuthErrorCode::InvalidConfig);
}

// ---------------------------------------------------------------------------
// Expired token

TEST_CASE("authenticate rejects expired token", "[oidc_verify]")
{
    const auto& k = test_key();
    TokenBuilder tb;
    tb.exp_secs = -60; // expired a minute ago
    const std::string token = tb.build(k);
    const OidcConfig cfg    = make_cfg(); // clock_skew_seconds=5, not enough to cover 60 s expiry
    const HttpFetcher fetch = make_jwks_fetcher(k.jwks_json);

    REQUIRE_THROWS_AS( authenticate(token, cfg, fetch), AuthError );
    require_auth_error([&]{ authenticate(token, cfg, fetch); }, AuthErrorCode::InvalidToken);
}

// ---------------------------------------------------------------------------
// Issuer mismatch

TEST_CASE("authenticate rejects token with wrong issuer", "[oidc_verify]")
{
    const auto& k = test_key();
    TokenBuilder tb;
    tb.issuer = "https://evil.example.com";
    const std::string token = tb.build(k);
    // make_cfg() sets verify_issuer=true and issuer="https://test-issuer.example.com"
    const OidcConfig cfg    = make_cfg();
    const HttpFetcher fetch = make_jwks_fetcher(k.jwks_json);

    REQUIRE_THROWS_AS( authenticate(token, cfg, fetch), AuthError );
    require_auth_error([&]{ authenticate(token, cfg, fetch); }, AuthErrorCode::InvalidToken);
}

// ---------------------------------------------------------------------------
// Audience mismatch

TEST_CASE("authenticate rejects token with wrong audience", "[oidc_verify]")
{
    const auto& k = test_key();
    TokenBuilder tb;
    tb.audience = "wrong-audience";
    const std::string token = tb.build(k);
    // make_cfg() sets verify_audience=true and audience="test-audience"
    const OidcConfig cfg    = make_cfg();
    const HttpFetcher fetch = make_jwks_fetcher(k.jwks_json);

    REQUIRE_THROWS_AS( authenticate(token, cfg, fetch), AuthError );
    require_auth_error([&]{ authenticate(token, cfg, fetch); }, AuthErrorCode::InvalidToken);
}

// ---------------------------------------------------------------------------
// Legacy Keycloak azp check

TEST_CASE("authenticate applies legacy azp check when using Keycloak config", "[oidc_verify]")
{
    const auto& k = test_key();

    SECTION("matching azp succeeds") {
        TokenBuilder tb;
        tb.azp = "uda-client";
        const std::string token = tb.build(k);

        OidcConfig cfg;
        cfg.issuer                = tb.issuer;
        cfg.audience              = tb.audience;
        cfg.client_id             = "uda-client";
        cfg.allowed_algs          = {"RS256"};
        cfg.verify_issuer         = true;
        cfg.verify_audience       = true;
        cfg.clock_skew_seconds    = 5;
        cfg.jwks_uri              = "https://test.example/jwks";
        cfg.legacy_keycloak_config = true;

        const HttpFetcher fetch = make_jwks_fetcher(k.jwks_json);
        REQUIRE_NOTHROW( authenticate(token, cfg, fetch) );
    }

    SECTION("mismatched azp fails") {
        TokenBuilder tb;
        tb.azp = "other-client";
        const std::string token = tb.build(k);

        OidcConfig cfg;
        cfg.issuer                = tb.issuer;
        cfg.audience              = tb.audience;
        cfg.client_id             = "uda-client"; // doesn't match
        cfg.allowed_algs          = {"RS256"};
        cfg.verify_issuer         = true;
        cfg.verify_audience       = true;
        cfg.clock_skew_seconds    = 5;
        cfg.jwks_uri              = "https://test.example/jwks";
        cfg.legacy_keycloak_config = true;

        const HttpFetcher fetch = make_jwks_fetcher(k.jwks_json);
        REQUIRE_THROWS_AS( authenticate(token, cfg, fetch), AuthError );
        require_auth_error(
            [&]{ authenticate(token, cfg, fetch); },
            AuthErrorCode::ClaimPolicyFailed);
    }
}

// ---------------------------------------------------------------------------
// required_claims policy

TEST_CASE("authenticate applies UDA_SERVER_OIDC_REQUIRED_CLAIMS policy", "[oidc_verify]")
{
    const auto& k = test_key();

    SECTION("scope word present — passes") {
        TokenBuilder tb;
        tb.scope = "openid uda.read";
        const std::string token = tb.build(k);
        OidcConfig cfg           = make_cfg();
        cfg.required_claims      = "scope:contains_word:uda.read";
        const HttpFetcher fetch  = make_jwks_fetcher(k.jwks_json);
        REQUIRE_NOTHROW( authenticate(token, cfg, fetch) );
    }

    SECTION("scope word absent — fails") {
        TokenBuilder tb;
        tb.scope = "openid";
        const std::string token = tb.build(k);
        OidcConfig cfg           = make_cfg();
        cfg.required_claims      = "scope:contains_word:uda.read";
        const HttpFetcher fetch  = make_jwks_fetcher(k.jwks_json);
        REQUIRE_THROWS_AS( authenticate(token, cfg, fetch), AuthError );
        require_auth_error(
            [&]{ authenticate(token, cfg, fetch); },
            AuthErrorCode::ClaimPolicyFailed);
    }
}

// ---------------------------------------------------------------------------
// Malformed OIDC required_claims spec

TEST_CASE("authenticate throws on malformed UDA_SERVER_OIDC_REQUIRED_CLAIMS", "[oidc_verify]")
{
    const auto& k = test_key();
    const std::string token = TokenBuilder{}.build(k);
    OidcConfig cfg           = make_cfg();
    cfg.required_claims      = "bad_rule_no_op";
    const HttpFetcher fetch  = make_jwks_fetcher(k.jwks_json);

    REQUIRE_THROWS_AS( authenticate(token, cfg, fetch), AuthError );
    require_auth_error(
        [&]{ authenticate(token, cfg, fetch); },
        AuthErrorCode::InvalidConfig);
}

// ---------------------------------------------------------------------------
// No claim policy — must fail unless UDA_SERVER_OIDC_POLICY=none

TEST_CASE("authenticate requires claim policy or explicit opt-out", "[oidc_verify]")
{
    const auto& k = test_key();
    const std::string token = TokenBuilder{}.build(k);
    const HttpFetcher fetch = make_jwks_fetcher(k.jwks_json);

    EnvCleaner clean{"UDA_SERVER_OIDC_POLICY"};

    SECTION("no audience, no required_claims, no client_id — fails") {
        OidcConfig cfg;
        cfg.issuer          = "https://test-issuer.example.com";
        cfg.jwks_uri        = "https://test.example/jwks";
        cfg.allowed_algs    = {"RS256"};
        cfg.verify_issuer   = true;
        cfg.verify_audience = false;
        cfg.clock_skew_seconds = 5;
        // no audience, no required_claims, no legacy client_id
        REQUIRE_THROWS_AS( authenticate(token, cfg, fetch), AuthError );
        require_auth_error(
            [&]{ authenticate(token, cfg, fetch); },
            AuthErrorCode::InvalidConfig);
    }

    SECTION("UDA_SERVER_OIDC_POLICY=none bypasses the check") {
        EnvGuard policy("UDA_SERVER_OIDC_POLICY", "none");
        OidcConfig cfg;
        cfg.issuer          = "https://test-issuer.example.com";
        cfg.jwks_uri        = "https://test.example/jwks";
        cfg.allowed_algs    = {"RS256"};
        cfg.verify_issuer   = true;
        cfg.verify_audience = false;
        cfg.clock_skew_seconds = 5;
        // With POLICY=none, the check is bypassed — token passes (assuming valid sig/exp/iss)
        REQUIRE_NOTHROW( authenticate(token, cfg, fetch) );
    }
}

// ---------------------------------------------------------------------------
// Unsupported algorithm

TEST_CASE("authenticate throws when token uses unsupported algorithm", "[oidc_verify]")
{
    // Build an HS256 token (symmetric) — we only support RS256/384/512
    const std::string secret = "test-secret-key-long-enough-for-hmac";
    const std::string token = jwt::create()
        .set_type("JWT")
        .set_issuer("https://test-issuer.example.com")
        .set_audience("test-audience")
        .set_expires_at(std::chrono::system_clock::now() + std::chrono::minutes{5})
        .set_payload_claim("azp", jwt::claim(std::string("test-client")))
        .sign(jwt::algorithm::hs256{secret});

    OidcConfig cfg           = make_cfg();
    cfg.required_claims      = ""; // use audience as the only policy
    const HttpFetcher fetch  = make_jwks_fetcher(test_key().jwks_json);

    // RS256 verifier will reject HS256 token (kid lookup fails → InvalidToken)
    REQUIRE_THROWS_AS( authenticate(token, cfg, fetch), AuthError );
    require_auth_error(
        [&]{ authenticate(token, cfg, fetch); },
        AuthErrorCode::InvalidToken);
}

// ---------------------------------------------------------------------------
// OIDC discovery path: mock fetcher returns discovery doc then JWKS

TEST_CASE("authenticate follows OIDC discovery when jwks_uri not set directly", "[oidc_verify]")
{
    const auto& k = test_key();
    const std::string token = TokenBuilder{}.build(k);

    OidcConfig cfg;
    cfg.issuer          = "https://test-issuer.example.com";
    cfg.audience        = "test-audience";
    cfg.allowed_algs    = {"RS256"};
    cfg.verify_issuer   = true;
    cfg.verify_audience = true;
    cfg.clock_skew_seconds = 5;
    // jwks_uri NOT set — should discover via issuer/.well-known/openid-configuration

    const std::string discovery_url = cfg.issuer + "/.well-known/openid-configuration";
    const std::string jwks_url      = cfg.issuer + "/jwks";
    const json discovery_doc = {
        {"issuer",   cfg.issuer},
        {"jwks_uri", jwks_url},
    };

    HttpFetcher fetch = [&](const std::string& url) -> std::string {
        if (url == discovery_url) return discovery_doc.dump();
        if (url == jwks_url)      return k.jwks_json;
        throw std::runtime_error("Unexpected URL in test: " + url);
    };

    REQUIRE_NOTHROW( authenticate(token, cfg, fetch) );
}

TEST_CASE("authenticate rejects discovery when issuer in doc mismatches config", "[oidc_verify]")
{
    const auto& k = test_key();
    const std::string token = TokenBuilder{}.build(k);

    OidcConfig cfg;
    // Use a unique issuer URL so this test's discovery URL is not already in the
    // process-global discovery cache from the "follows OIDC discovery" test above.
    cfg.issuer          = "https://bad-discovery.example.com";
    cfg.audience        = "test-audience";
    cfg.allowed_algs    = {"RS256"};
    cfg.verify_issuer   = true;
    cfg.verify_audience = true;
    cfg.clock_skew_seconds = 5;

    // Discovery doc returns an issuer that doesn't match cfg.issuer
    const json discovery_doc = {
        {"issuer",   "https://evil.example.com"},
        {"jwks_uri", cfg.issuer + "/jwks"},
    };

    HttpFetcher fetch = [&](const std::string& url) -> std::string {
        if (url.find("openid-configuration") != std::string::npos) return discovery_doc.dump();
        return k.jwks_json;
    };

    REQUIRE_THROWS_AS( authenticate(token, cfg, fetch), AuthError );
    require_auth_error(
        [&]{ authenticate(token, cfg, fetch); },
        AuthErrorCode::InvalidConfig);
}

// ---------------------------------------------------------------------------
// Array-claim regression: nlohmann/json must produce JSON array string, not "array"
//
// Prior picojson bug: picojson::value::to_str() returned the literal string "array" for
// array-typed claims instead of the JSON-serialised array. Fixed by using
// decoded.get_payload() + nlohmann/json for payload map construction.

TEST_CASE("authenticate exposes array claim as JSON string, not literal 'array'", "[oidc_verify]")
{
    const auto& k = test_key();

    // Build a token with an array payload claim "roles"
    picojson::array roles_arr;
    roles_arr.emplace_back(picojson::value(std::string("admin")));
    roles_arr.emplace_back(picojson::value(std::string("user")));

    auto now = std::chrono::system_clock::now();
    const std::string token = jwt::create()
        .set_type("JWT")
        .set_issuer("https://test-issuer.example.com")
        .set_audience("test-audience")
        .set_issued_at(now)
        .set_expires_at(now + std::chrono::minutes{5})
        .set_key_id("test-key-1")
        .set_payload_claim("azp",   jwt::claim(std::string("test-client")))
        .set_payload_claim("roles", jwt::claim(picojson::value(roles_arr)))
        .sign(jwt::algorithm::rs256(k.public_pem, k.private_pem, "", ""));

    const OidcConfig cfg    = make_cfg();
    const HttpFetcher fetch = make_jwks_fetcher(k.jwks_json);

    PayloadType payload;
    REQUIRE_NOTHROW( payload = authenticate(token, cfg, fetch) );

    REQUIRE( payload.count("roles") == 1 );
    const std::string roles_val = payload.at("roles");
    // Prior picojson bug produced the literal string "array"; nlohmann produces JSON
    REQUIRE( roles_val != "array" );
    const auto roles_json = json::parse(roles_val);
    REQUIRE( roles_json.is_array() );
    REQUIRE( roles_json.size() == 2 );
}

// ---------------------------------------------------------------------------
// RS384 and RS512 algorithm support

TEST_CASE("authenticate accepts RS384 token when configured for RS384", "[oidc_verify]")
{
    const auto& k = test_key();
    TokenBuilder tb;
    tb.sign_alg = "RS384";
    const std::string token = tb.build(k);

    OidcConfig cfg = make_cfg("https://test-issuer.example.com", "test-audience",
                              "https://rs384-test.example.com/jwks");
    cfg.allowed_algs = {"RS384"};
    const HttpFetcher fetch = make_jwks_fetcher(k.jwks_json);

    REQUIRE_NOTHROW( authenticate(token, cfg, fetch) );
}

TEST_CASE("authenticate accepts RS512 token when configured for RS512", "[oidc_verify]")
{
    const auto& k = test_key();
    TokenBuilder tb;
    tb.sign_alg = "RS512";
    const std::string token = tb.build(k);

    OidcConfig cfg = make_cfg("https://test-issuer.example.com", "test-audience",
                              "https://rs512-test.example.com/jwks");
    cfg.allowed_algs = {"RS512"};
    const HttpFetcher fetch = make_jwks_fetcher(k.jwks_json);

    REQUIRE_NOTHROW( authenticate(token, cfg, fetch) );
}

TEST_CASE("authenticate rejects RS256 token when only RS512 is allowed", "[oidc_verify]")
{
    const auto& k = test_key();
    const std::string token = TokenBuilder{}.build(k); // signed with RS256

    OidcConfig cfg = make_cfg("https://test-issuer.example.com", "test-audience",
                              "https://rs512-reject.example.com/jwks");
    cfg.allowed_algs = {"RS512"};
    const HttpFetcher fetch = make_jwks_fetcher(k.jwks_json);

    REQUIRE_THROWS_AS( authenticate(token, cfg, fetch), AuthError );
    require_auth_error(
        [&]{ authenticate(token, cfg, fetch); },
        AuthErrorCode::InvalidToken);
}

// ---------------------------------------------------------------------------
// JWKS cache behaviour

TEST_CASE("JWKS cache: second call with same URI uses cached JWKS", "[oidc_verify][jwks_cache]")
{
    const auto& k = test_key();
    // Use a unique URI so this test doesn't collide with other tests' cache entries.
    const OidcConfig cfg = make_cfg("https://test-issuer.example.com", "test-audience",
                                    "https://cache-hit-unique-1.example.com/jwks");

    int fetch_count = 0;
    const HttpFetcher counting_fetch = [&](const std::string&) -> std::string {
        ++fetch_count;
        return k.jwks_json;
    };

    const std::string token1 = TokenBuilder{}.build(k);
    const std::string token2 = TokenBuilder{}.build(k);

    REQUIRE_NOTHROW( authenticate(token1, cfg, counting_fetch) );
    REQUIRE_NOTHROW( authenticate(token2, cfg, counting_fetch) );
    REQUIRE( fetch_count == 1 ); // second call reused the cached JWKS
}

TEST_CASE("JWKS cache: kid-not-found triggers one force-refresh retry", "[oidc_verify][jwks_cache]")
{
    const auto& k = test_key();
    // Use a unique URI to start with an empty cache for this test.
    const OidcConfig cfg = make_cfg("https://test-issuer.example.com", "test-audience",
                                    "https://kid-retry-unique-1.example.com/jwks");

    // Stale JWKS: same key material but wrong kid — simulates a stale cache entry
    // where the token's kid is absent. On retry, the correct JWKS is returned.
    json stale_jwks = json::parse(k.jwks_json);
    stale_jwks["keys"][0]["kid"] = "stale-wrong-kid";
    const std::string stale_str = stale_jwks.dump();

    int fetch_count = 0;
    const HttpFetcher rotating_fetch = [&](const std::string&) -> std::string {
        ++fetch_count;
        return fetch_count == 1 ? stale_str : k.jwks_json;
    };

    const std::string token = TokenBuilder{}.build(k);

    // Should succeed: first fetch has wrong kid → force-refresh → correct kid found
    REQUIRE_NOTHROW( authenticate(token, cfg, rotating_fetch) );
    REQUIRE( fetch_count == 2 ); // initial fetch + one retry
}

// ---------------------------------------------------------------------------
// OidcConfig::from_env() round-trip integration test

TEST_CASE("authenticate succeeds when OidcConfig is loaded from environment", "[oidc_verify][integration]")
{
    const auto& k = test_key();

    // Use unique values to avoid JWKS/discovery cache interference with other tests.
    const std::string issuer   = "https://from-env-unique-1.example.com";
    const std::string jwks_uri = "https://from-env-unique-1.example.com/jwks";

    EnvGuard g_issuer("UDA_SERVER_OIDC_ISSUER",       issuer.c_str());
    EnvGuard g_jwks  ("UDA_SERVER_OIDC_JWKS_URI",      jwks_uri.c_str());
    EnvGuard g_algs  ("UDA_SERVER_OIDC_ALLOWED_ALGS",  "RS256");
    // Ensure no audience or claim-policy env vars leak in from the environment.
    EnvCleaner cleanup({"UDA_SERVER_OIDC_AUDIENCE", "UDA_SERVER_OIDC_REQUIRED_CLAIMS",
                        "UDA_SERVER_OIDC_CLIENT_ID", "UDA_SERVER_KEYCLOAK_REALM",
                        "UDA_SERVER_KEYCLOAK_CLIENT_ID"});

    TokenBuilder tb;
    tb.issuer = issuer;
    const std::string token = tb.build(k);

    const OidcConfig cfg = OidcConfig::from_env();
    REQUIRE( cfg.issuer   == issuer );
    REQUIRE( cfg.jwks_uri == jwks_uri );
    // Without UDA_SERVER_OIDC_AUDIENCE, verify_audience defaults false (empty audience)
    REQUIRE( cfg.verify_audience == false );

    const HttpFetcher fetch = make_jwks_fetcher(k.jwks_json);
    PayloadType payload;
    REQUIRE_NOTHROW( payload = authenticate(token, cfg, fetch) );
    REQUIRE( payload.count("azp") == 1 );
    REQUIRE( payload.at("azp") == "test-client" );
}

// ---------------------------------------------------------------------------
// JwksCache injection — test isolation via explicit cache

TEST_CASE("authenticate with explicit cache: second call reuses cached JWKS", "[oidc_verify]")
{
    const auto& k = test_key();
    const OidcConfig cfg = make_cfg("https://test-issuer.example.com", "test-audience",
                                    "https://explicit-cache-test-1.example.com/jwks");
    const std::string token = TokenBuilder{}.build(k);

    int fetch_count = 0;
    const HttpFetcher counting_fetch = [&](const std::string&) -> std::string {
        ++fetch_count;
        return k.jwks_json;
    };

    JwksCache fresh_cache(300);
    REQUIRE_NOTHROW( authenticate(token, cfg, counting_fetch, fresh_cache) );
    REQUIRE_NOTHROW( authenticate(token, cfg, counting_fetch, fresh_cache) );
    REQUIRE( fetch_count == 1 ); // second call hits the injected cache
}

TEST_CASE("authenticate with explicit cache: fresh cache does not share entries with global", "[oidc_verify]")
{
    const auto& k = test_key();
    // Use a URI that's definitely been fetched by other tests (via global cache)
    const OidcConfig cfg = make_cfg("https://test-issuer.example.com", "test-audience",
                                    "https://test-issuer.example.com/jwks");
    const std::string token = TokenBuilder{}.build(k);

    int fetch_count = 0;
    const HttpFetcher counting_fetch = [&](const std::string&) -> std::string {
        ++fetch_count;
        return k.jwks_json;
    };

    // Fresh cache starts empty — must fetch even if global cache has this URI
    JwksCache fresh_cache(300);
    REQUIRE_NOTHROW( authenticate(token, cfg, counting_fetch, fresh_cache) );
    REQUIRE( fetch_count == 1 );
}
