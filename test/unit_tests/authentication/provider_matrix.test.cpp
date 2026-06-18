// Provider matrix tests: generic OIDC, Keycloak, SciTokens.
//
// Acceptance tests verify that auth_payload carries the expected claim shapes.
// Rejection tests verify the correct AuthErrorCode for each failure mode.
// ClaimPolicy::check() is exercised in each accepting test as the plugin-layer
// claim reader (the same API a UDA plugin would call against the payload).
//
// Note on wlcg.groups / wlcg.ver (SciTokens):
//   These claim keys contain a literal dot. ClaimPolicy uses dot-separated paths
//   to navigate nested JSON objects, so "wlcg.groups" would be interpreted as
//   payload["wlcg"]["groups"], which is a navigation path — not a top-level key.
//   Direct payload inspection is used for these claims instead.

#include "auth_test_helpers.h"
#include <authentication/claim_policy.h>

// One RSA key pair per binary (generation is slow; sharing is fine).
static const RsaTestKey& test_key()
{
    static const RsaTestKey k = generate_rsa_test_key();
    return k;
}

// ============================================================================
// Provider 1 — Generic OIDC (Auth0 / Okta style)
//
// Claim shape: flat standard OIDC claims, string scope, string aud.
// Fixture corresponds to test/fixtures/auth/generic_oidc_payload.json.
// ============================================================================

TEST_CASE("generic OIDC: accepted, claims preserved as raw strings", "[provider_matrix][generic_oidc]")
{
    const auto& k = test_key();

    TokenBuilder tb;
    tb.issuer   = "https://oidc.example.com";
    tb.audience = "uda";
    tb.azp      = "uda-client";
    tb.scope    = "openid profile email uda.read";
    tb.extra_claims = {
        {"sub",                "1234567890"},
        {"preferred_username", "alice"},
        {"email",              "alice@example.com"},
    };

    const std::string token = tb.build(k);
    OidcConfig cfg = make_cfg("https://oidc.example.com", "uda",
                              "https://oidc.example.com/jwks");
    JwksCache cache(300);
    PayloadType payload;
    REQUIRE_NOTHROW(payload = authenticate(token, cfg, make_jwks_fetcher(k.jwks_json), cache));

    // String claims stored as raw values, not JSON-quoted strings.
    CHECK(payload_str_eq(payload, "azp",                "uda-client"));
    CHECK(payload_str_eq(payload, "sub",                "1234567890"));
    CHECK(payload_str_eq(payload, "preferred_username", "alice"));
    CHECK(payload_str_eq(payload, "email",              "alice@example.com"));
    CHECK(payload_str_eq(payload, "scope",              "openid profile email uda.read"));

    // Plugin layer: ClaimPolicy reads scope as whitespace-delimited words.
    std::string err;
    CHECK(ClaimPolicy::parse("scope:contains_word:uda.read").check(payload, err));
    CHECK(ClaimPolicy::parse("azp:equals:uda-client").check(payload, err));
    CHECK(ClaimPolicy::parse("preferred_username:exists").check(payload, err));

    // Word not present → policy fails.
    CHECK_FALSE(ClaimPolicy::parse("scope:contains_word:write").check(payload, err));
    CHECK_FALSE(err.empty());
}

// ============================================================================
// Provider 2 — Keycloak
//
// Claim shape: realm_access.roles[] (nested JSON object), resource_access
// (doubly nested), string audience.
// Fixture corresponds to test/fixtures/auth/keycloak_payload.json.
// ============================================================================

TEST_CASE("Keycloak: realm_access.roles preserved as JSON, ClaimPolicy readable", "[provider_matrix][keycloak]")
{
    const auto& k = test_key();

    TokenBuilder tb;
    tb.issuer   = "https://keycloak.example/realms/test";
    tb.audience = "uda";
    tb.azp      = "uda-client";
    tb.scope    = "openid profile email";
    tb.extra_claims = {
        {"sub",                "f47ac10b-58cc-4372-a567-0e02b2c3d479"},
        {"preferred_username", "alice"},
        {"realm_access",  {{"roles", {"uda-user", "offline_access", "uma_authorization"}}}},
        {"resource_access", {
            {"uda-client", {{"roles", {"data-read", "data-write"}}}},
            {"account",    {{"roles", {"manage-account", "view-profile"}}}}
        }},
        {"typ", "Bearer"},
    };

    const std::string token = tb.build(k);
    OidcConfig cfg = make_cfg("https://keycloak.example/realms/test", "uda",
                              "https://keycloak.example/realms/test/jwks");
    JwksCache cache(300);
    PayloadType payload;
    REQUIRE_NOTHROW(payload = authenticate(token, cfg, make_jwks_fetcher(k.jwks_json), cache));

    // Standard flat claims.
    CHECK(payload_str_eq(payload, "sub",                "f47ac10b-58cc-4372-a567-0e02b2c3d479"));
    CHECK(payload_str_eq(payload, "preferred_username", "alice"));
    CHECK(payload_str_eq(payload, "typ",                "Bearer"));

    // realm_access stored as a JSON-encoded object string — NOT the literal "object".
    REQUIRE(payload.count("realm_access") == 1);
    const json ra = json::parse(payload.at("realm_access"));
    REQUIRE(ra.is_object());
    REQUIRE(ra.contains("roles"));
    REQUIRE(ra["roles"].is_array());
    // Verify specific role membership.
    bool has_uda_user = false;
    for (const auto& r : ra["roles"]) {
        if (r.is_string() && r.get<std::string>() == "uda-user") has_uda_user = true;
    }
    REQUIRE(has_uda_user);

    // resource_access: doubly-nested object preserved.
    REQUIRE(payload.count("resource_access") == 1);
    const json rac = json::parse(payload.at("resource_access"));
    REQUIRE(rac.is_object());
    REQUIRE(rac.contains("uda-client"));
    REQUIRE(rac["uda-client"]["roles"].is_array());

    // Plugin layer: dot-path navigation into realm_access.roles.
    std::string err;
    CHECK(ClaimPolicy::parse("realm_access.roles:contains:uda-user").check(payload, err));
    CHECK(ClaimPolicy::parse("realm_access.roles:contains:offline_access").check(payload, err));
    CHECK(ClaimPolicy::parse("realm_access.roles:contains_any:uda-user,admin").check(payload, err));

    // Role not in list → policy fails.
    CHECK_FALSE(ClaimPolicy::parse("realm_access.roles:contains:superuser").check(payload, err));
}

TEST_CASE("Keycloak: legacy_keycloak_config azp check applied", "[provider_matrix][keycloak]")
{
    const auto& k = test_key();

    TokenBuilder tb;
    tb.issuer   = "https://keycloak.example/realms/test";
    tb.audience = "uda";
    tb.azp      = "uda-client";
    tb.scope    = "openid";

    OidcConfig cfg;
    cfg.issuer                  = tb.issuer;
    cfg.audience                = "uda";
    cfg.client_id               = "uda-client";
    cfg.jwks_uri                = "https://keycloak.example/realms/test/jwks";
    cfg.allowed_algs            = {"RS256"};
    cfg.verify_issuer           = true;
    cfg.verify_audience         = true;
    cfg.clock_skew_seconds      = 5;
    cfg.legacy_keycloak_config  = true;

    JwksCache cache(300);
    const HttpFetcher fetch = make_jwks_fetcher(k.jwks_json);

    SECTION("matching azp — accepted") {
        REQUIRE_NOTHROW(authenticate(tb.build(k), cfg, fetch, cache));
    }
    SECTION("mismatched azp — rejected") {
        tb.azp = "evil-client";
        require_auth_error(
            [&]{ authenticate(tb.build(k), cfg, fetch, cache); },
            AuthErrorCode::ClaimPolicyFailed);
    }
}

// ============================================================================
// Provider 3 — SciTokens 2.0 / WLCG Token Profile
//
// Claim shape: wlcg.groups (JSON array, key name contains a literal dot),
// wlcg.ver (string), space-delimited scope with path prefixes.
// Fixture corresponds to test/fixtures/auth/scitokens_payload.json.
//
// Note: wlcg.groups / wlcg.ver cannot be navigated via ClaimPolicy dot-paths
// because ClaimPolicy interprets dots as object-path separators. They are
// read by direct payload inspection instead.
// ============================================================================

TEST_CASE("SciTokens: wlcg.groups array preserved, scope words accessible", "[provider_matrix][scitokens]")
{
    const auto& k = test_key();

    TokenBuilder tb;
    tb.issuer   = "https://scitokens.example";
    tb.audience = "uda";
    tb.azp      = "uda-client";
    // Space-delimited scope per SciTokens profile; simple words avoid colon-in-value
    // ambiguity with ClaimPolicy's colon delimiter.
    tb.scope    = "openid compute.read storage.read";
    tb.extra_claims = {
        {"sub",        "alice"},
        {"wlcg.ver",   "1.0"},
        {"wlcg.groups", {"/uda", "/uda/ops", "/atlas"}},
        {"jti",        "a1b2c3d4-e5f6-7890-abcd-ef1234567890"},
    };

    const std::string token = tb.build(k);
    OidcConfig cfg = make_cfg("https://scitokens.example", "uda",
                              "https://scitokens.example/jwks");
    JwksCache cache(300);
    PayloadType payload;
    REQUIRE_NOTHROW(payload = authenticate(token, cfg, make_jwks_fetcher(k.jwks_json), cache));

    // Flat string claims.
    CHECK(payload_str_eq(payload, "sub", "alice"));
    CHECK(payload_str_eq(payload, "jti", "a1b2c3d4-e5f6-7890-abcd-ef1234567890"));
    CHECK(payload_str_eq(payload, "scope", "openid compute.read storage.read"));

    // wlcg.ver: stored under the literal key "wlcg.ver" as a raw string.
    // (ClaimPolicy would misinterpret this path — read directly.)
    REQUIRE(payload.count("wlcg.ver") == 1);
    CHECK(payload.at("wlcg.ver") == "1.0");

    // wlcg.groups: stored under the literal key "wlcg.groups" as a JSON array string.
    // Not the literal string "array" — the full JSON-encoded array.
    REQUIRE(payload.count("wlcg.groups") == 1);
    const std::string groups_str = payload.at("wlcg.groups");
    CHECK(groups_str != "array");
    const json groups = json::parse(groups_str);
    REQUIRE(groups.is_array());
    REQUIRE(groups.size() == 3);

    // Membership check via direct inspection.
    bool has_uda = false;
    for (const auto& g : groups) {
        if (g.is_string() && g.get<std::string>() == "/uda") has_uda = true;
    }
    REQUIRE(has_uda);

    // Plugin layer: scope words that don't contain colons work fine with ClaimPolicy.
    std::string err;
    CHECK(ClaimPolicy::parse("scope:contains_word:compute.read").check(payload, err));
    CHECK(ClaimPolicy::parse("scope:contains_word:storage.read").check(payload, err));
    CHECK_FALSE(ClaimPolicy::parse("scope:contains_word:write").check(payload, err));
}

// ============================================================================
// Rejection matrix
// ============================================================================

TEST_CASE("reject: token with wrong issuer", "[provider_matrix][reject]")
{
    const auto& k = test_key();
    TokenBuilder tb;
    tb.issuer   = "https://evil.example.com";
    tb.audience = "uda";

    const OidcConfig cfg = make_cfg("https://oidc.example.com", "uda",
                                    "https://oidc.example.com/jwks");
    JwksCache cache(300);
    require_auth_error(
        [&]{ authenticate(tb.build(k), cfg, make_jwks_fetcher(k.jwks_json), cache); },
        AuthErrorCode::InvalidToken);
}

TEST_CASE("reject: token with wrong string audience", "[provider_matrix][reject]")
{
    const auto& k = test_key();
    TokenBuilder tb;
    tb.issuer   = "https://oidc.example.com";
    tb.audience = "wrong-audience";

    const OidcConfig cfg = make_cfg("https://oidc.example.com", "uda",
                                    "https://oidc.example.com/wrong-aud-jwks");
    JwksCache cache(300);
    require_auth_error(
        [&]{ authenticate(tb.build(k), cfg, make_jwks_fetcher(k.jwks_json), cache); },
        AuthErrorCode::InvalidToken);
}

TEST_CASE("accept: array audience with matching element", "[provider_matrix]")
{
    const auto& k = test_key();
    TokenBuilder tb;
    tb.issuer        = "https://oidc.example.com";
    tb.audience_arr  = {"uda", "other-service"};

    const OidcConfig cfg = make_cfg("https://oidc.example.com", "uda",
                                    "https://oidc.example.com/array-aud-jwks");
    JwksCache cache(300);
    PayloadType payload;
    REQUIRE_NOTHROW(payload = authenticate(tb.build(k), cfg, make_jwks_fetcher(k.jwks_json), cache));

    // aud stored as a JSON array string (not "array" literal, not the single value).
    REQUIRE(payload.count("aud") == 1);
    const json aud = json::parse(payload.at("aud"));
    REQUIRE(aud.is_array());
    REQUIRE(payload_array_contains(payload, "aud", "uda"));
    REQUIRE(payload_array_contains(payload, "aud", "other-service"));
}

TEST_CASE("reject: array audience with no matching element", "[provider_matrix][reject]")
{
    const auto& k = test_key();
    TokenBuilder tb;
    tb.issuer        = "https://oidc.example.com";
    tb.audience_arr  = {"other-service", "yet-another"};

    const OidcConfig cfg = make_cfg("https://oidc.example.com", "uda",
                                    "https://oidc.example.com/array-aud-no-match-jwks");
    JwksCache cache(300);
    require_auth_error(
        [&]{ authenticate(tb.build(k), cfg, make_jwks_fetcher(k.jwks_json), cache); },
        AuthErrorCode::InvalidToken);
}

TEST_CASE("reject: expired token", "[provider_matrix][reject]")
{
    const auto& k = test_key();
    TokenBuilder tb;
    tb.issuer   = "https://oidc.example.com";
    tb.audience = "uda";
    tb.exp_secs = -120; // expired two minutes ago; clock_skew_seconds=5 cannot cover it

    const OidcConfig cfg = make_cfg("https://oidc.example.com", "uda",
                                    "https://oidc.example.com/expired-jwks");
    JwksCache cache(300);
    require_auth_error(
        [&]{ authenticate(tb.build(k), cfg, make_jwks_fetcher(k.jwks_json), cache); },
        AuthErrorCode::InvalidToken);
}

TEST_CASE("reject: not-yet-valid token (future nbf)", "[provider_matrix][reject]")
{
    const auto& k = test_key();
    TokenBuilder tb;
    tb.issuer          = "https://oidc.example.com";
    tb.audience        = "uda";
    tb.nbf_offset_secs = 300; // valid only 5 minutes from now

    const OidcConfig cfg = make_cfg("https://oidc.example.com", "uda",
                                    "https://oidc.example.com/future-nbf-jwks");
    JwksCache cache(300);
    require_auth_error(
        [&]{ authenticate(tb.build(k), cfg, make_jwks_fetcher(k.jwks_json), cache); },
        AuthErrorCode::InvalidToken);
}

TEST_CASE("reject: RS256 token when only RS512 is permitted", "[provider_matrix][reject]")
{
    const auto& k = test_key();
    TokenBuilder tb;
    tb.issuer    = "https://oidc.example.com";
    tb.audience  = "uda";
    tb.sign_alg  = "RS256"; // JWKS advertises RS256; config requires RS512

    OidcConfig cfg = make_cfg("https://oidc.example.com", "uda",
                              "https://oidc.example.com/rs512-only-jwks");
    cfg.allowed_algs = {"RS512"};

    JwksCache cache(300);
    require_auth_error(
        [&]{ authenticate(tb.build(k), cfg, make_jwks_fetcher(k.jwks_json), cache); },
        AuthErrorCode::InvalidToken);
}

// ============================================================================
// JWKS key rotation — stale kid triggers refresh
//
// This exercises the Keycloak provider shape with a key rotation event: the
// initial JWKS has the wrong kid, so the first verification attempt fails;
// authenticate() retries with a fresh fetch and succeeds.
// ============================================================================

TEST_CASE("Keycloak: stale kid triggers exactly one JWKS refresh", "[provider_matrix][keycloak]")
{
    const auto& k = test_key();

    TokenBuilder tb;
    tb.issuer   = "https://keycloak.example/realms/test";
    tb.audience = "uda";
    tb.azp      = "uda-client";
    tb.scope    = "openid profile";
    tb.extra_claims = {
        {"realm_access", {{"roles", {"uda-user"}}}},
    };

    const std::string token = tb.build(k);
    const OidcConfig cfg = make_cfg("https://keycloak.example/realms/test", "uda",
                                    "https://keycloak.example/realms/test/rotation-test-jwks");

    // Stale JWKS: correct key material, wrong kid — simulates a key rotation event.
    json stale_jwks_json = json::parse(k.jwks_json);
    stale_jwks_json["keys"][0]["kid"] = "old-kid";
    const std::string stale_jwks = stale_jwks_json.dump();

    int fetch_count = 0;
    const HttpFetcher rotating_fetch = [&](const std::string&) -> std::string {
        ++fetch_count;
        return fetch_count == 1 ? stale_jwks : k.jwks_json;
    };

    JwksCache cache(300);
    PayloadType payload;
    // First fetch has wrong kid → triggers force-refresh → correct kid found.
    REQUIRE_NOTHROW(payload = authenticate(token, cfg, rotating_fetch, cache));
    REQUIRE(fetch_count == 2);

    // Claims available in the successfully verified token.
    std::string err;
    CHECK(ClaimPolicy::parse("realm_access.roles:contains:uda-user").check(payload, err));
}
