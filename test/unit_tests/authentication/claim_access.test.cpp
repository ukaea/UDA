// Tests for the shared claim-path engine used by both the server-side claim policy
// (UDA_SERVER_OIDC_REQUIRED_CLAIMS) and the plugin-facing authPayloadPath /
// authPayloadContains helpers.

#include <catch2/catch_test_macros.hpp>

#include <authentication/claim_access.h>

using uda::authentication::ClaimMap;
using uda::authentication::claim_contains;
using uda::authentication::claim_contains_element;
using uda::authentication::claim_contains_word;
using uda::authentication::resolve_claim;
using uda::authentication::split_claim_path;

namespace {

// A payload shaped the way oauth_authentication.cpp stores one: string claims raw,
// everything else JSON-serialised.
ClaimMap sample_payload()
{
    return {
        {"sub", "1234"},
        {"preferred_username", "alice"},
        {"scope", "openid uda.read profile"},
        {"realm_access", R"({"roles":["uda-user","offline_access"]})"},
        {"resource_access", R"({"uda-client":{"roles":["read","write"]}})"},
        {"aud", R"(["uda","account"])"},
        {"wlcg.groups", R"(["/uda/users"])"},
        {"wlcg.ver", "1.0"},
        {"email_verified", "true"},
    };
}

} // namespace

TEST_CASE("split_claim_path splits on unescaped dots", "[claim_access]")
{
    REQUIRE(split_claim_path("a") == std::vector<std::string>{"a"});
    REQUIRE(split_claim_path("a.b") == std::vector<std::string>{"a", "b"});
    REQUIRE(split_claim_path("a.b.c") == std::vector<std::string>{"a", "b", "c"});
}

TEST_CASE("split_claim_path honours backslash-escaped dots", "[claim_access]")
{
    REQUIRE(split_claim_path(R"(wlcg\.groups)") == std::vector<std::string>{"wlcg.groups"});
    REQUIRE(split_claim_path(R"(a\.b.c)") == std::vector<std::string>{"a.b", "c"});
}

TEST_CASE("resolve_claim reads a flat string claim", "[claim_access]")
{
    const auto payload = sample_payload();
    REQUIRE(resolve_claim(payload, "preferred_username") == "alice");
    REQUIRE(resolve_claim(payload, "sub") == "1234");
}

TEST_CASE("resolve_claim returns nullopt for a missing claim", "[claim_access]")
{
    const auto payload = sample_payload();
    REQUIRE_FALSE(resolve_claim(payload, "no_such_claim").has_value());
    REQUIRE_FALSE(resolve_claim(payload, "").has_value());
}

TEST_CASE("resolve_claim navigates nested objects", "[claim_access]")
{
    const auto payload = sample_payload();
    REQUIRE(resolve_claim(payload, "realm_access.roles") == R"(["uda-user","offline_access"])");
    REQUIRE(resolve_claim(payload, "resource_access.uda-client.roles") == R"(["read","write"])");
}

TEST_CASE("resolve_claim indexes into arrays", "[claim_access]")
{
    const auto payload = sample_payload();
    REQUIRE(resolve_claim(payload, "realm_access.roles[0]") == "uda-user");
    REQUIRE(resolve_claim(payload, "realm_access.roles[1]") == "offline_access");
    REQUIRE(resolve_claim(payload, "resource_access.uda-client.roles[1]") == "write");
    REQUIRE(resolve_claim(payload, "aud[0]") == "uda");
}

TEST_CASE("resolve_claim rejects an out-of-range index", "[claim_access]")
{
    const auto payload = sample_payload();
    REQUIRE_FALSE(resolve_claim(payload, "realm_access.roles[2]").has_value());
    REQUIRE_FALSE(resolve_claim(payload, "aud[99]").has_value());
}

TEST_CASE("resolve_claim rejects navigating into a plain string", "[claim_access]")
{
    const auto payload = sample_payload();
    // preferred_username is not JSON, so there is nothing below it.
    REQUIRE_FALSE(resolve_claim(payload, "preferred_username.anything").has_value());
}

TEST_CASE("resolve_claim rejects a missing nested segment", "[claim_access]")
{
    const auto payload = sample_payload();
    REQUIRE_FALSE(resolve_claim(payload, "realm_access.groups").has_value());
    REQUIRE_FALSE(resolve_claim(payload, "resource_access.other-client.roles").has_value());
}

TEST_CASE("resolve_claim reaches claim names containing literal dots", "[claim_access]")
{
    // This is the SciTokens case: `wlcg.groups` is one claim name, not a path.
    const auto payload = sample_payload();
    REQUIRE(resolve_claim(payload, R"(wlcg\.groups)") == R"(["/uda/users"])");
    REQUIRE(resolve_claim(payload, R"(wlcg\.ver)") == "1.0");
    REQUIRE(resolve_claim(payload, R"(wlcg\.groups[0])") == "/uda/users");

    // Unescaped, the same text is a path into a claim named "wlcg", which does not exist.
    REQUIRE_FALSE(resolve_claim(payload, "wlcg.groups").has_value());
}

TEST_CASE("resolve_claim serialises a non-string leaf", "[claim_access]")
{
    const ClaimMap payload{{"nested", R"({"count":3,"flag":true})"}};
    REQUIRE(resolve_claim(payload, "nested.count") == "3");
    REQUIRE(resolve_claim(payload, "nested.flag") == "true");
}

TEST_CASE("claim_contains_element matches JSON array members", "[claim_access]")
{
    REQUIRE(claim_contains_element(R"(["a","b"])", "a"));
    REQUIRE(claim_contains_element(R"(["a","b"])", "b"));
    REQUIRE_FALSE(claim_contains_element(R"(["a","b"])", "c"));
}

TEST_CASE("claim_contains_word matches whole tokens only", "[claim_access]")
{
    REQUIRE(claim_contains_word("openid uda.read profile", "uda.read"));
    REQUIRE(claim_contains_word("openid uda.read profile", "openid"));
    REQUIRE_FALSE(claim_contains_word("openid uda.read profile", "uda"));
    REQUIRE_FALSE(claim_contains_word("openid uda.read profile", "read"));
}

TEST_CASE("claim_contains covers both array and space-delimited claims", "[claim_access]")
{
    const auto payload = sample_payload();

    // Array claim → element membership
    REQUIRE(claim_contains(payload, "realm_access.roles", "uda-user"));
    REQUIRE_FALSE(claim_contains(payload, "realm_access.roles", "admin"));

    // Space-delimited string claim → word membership, not substring
    REQUIRE(claim_contains(payload, "scope", "uda.read"));
    REQUIRE_FALSE(claim_contains(payload, "scope", "uda"));

    // Missing claim is not a match, and is not an error
    REQUIRE_FALSE(claim_contains(payload, "no_such_claim", "anything"));
}
