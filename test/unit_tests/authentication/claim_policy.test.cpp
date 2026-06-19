#include <catch2/catch_test_macros.hpp>

#include <stdexcept>
#include <string>
#include <unordered_map>

#include <authentication/claim_policy.h>

using namespace uda::authentication;
using Payload = std::unordered_map<std::string, std::string>;

// ---------------------------------------------------------------------------
// parse() — structural tests

TEST_CASE("ClaimPolicy::parse accepts empty spec", "[claim_policy]")
{
    const ClaimPolicy p = ClaimPolicy::parse("");
    REQUIRE( p.empty() );
}

TEST_CASE("ClaimPolicy::parse accepts single exists rule", "[claim_policy]")
{
    REQUIRE_NOTHROW( ClaimPolicy::parse("sub:exists") );
}

TEST_CASE("ClaimPolicy::parse accepts multiple rules separated by semicolons", "[claim_policy]")
{
    REQUIRE_NOTHROW( ClaimPolicy::parse("sub:exists;azp:equals:my-client;scope:contains_word:read") );
}

TEST_CASE("ClaimPolicy::parse rejects malformed rule (no op)", "[claim_policy]")
{
    REQUIRE_THROWS_AS( ClaimPolicy::parse("sub"), std::invalid_argument );
}

TEST_CASE("ClaimPolicy::parse rejects unknown op", "[claim_policy]")
{
    REQUIRE_THROWS_AS( ClaimPolicy::parse("sub:unknown_op:value"), std::invalid_argument );
}

TEST_CASE("ClaimPolicy::parse rejects op that requires value but has none", "[claim_policy]")
{
    REQUIRE_THROWS_AS( ClaimPolicy::parse("sub:equals"), std::invalid_argument );
    REQUIRE_THROWS_AS( ClaimPolicy::parse("sub:contains"), std::invalid_argument );
    REQUIRE_THROWS_AS( ClaimPolicy::parse("sub:contains_word"), std::invalid_argument );
    REQUIRE_THROWS_AS( ClaimPolicy::parse("sub:contains_any"), std::invalid_argument );
}

TEST_CASE("ClaimPolicy::parse accepts exists with no value", "[claim_policy]")
{
    REQUIRE_NOTHROW( ClaimPolicy::parse("sub:exists") );
}

TEST_CASE("ClaimPolicy::parse is case-insensitive for op names", "[claim_policy]")
{
    REQUIRE_NOTHROW( ClaimPolicy::parse("sub:EXISTS") );
    REQUIRE_NOTHROW( ClaimPolicy::parse("sub:Equals:val") );
    REQUIRE_NOTHROW( ClaimPolicy::parse("sub:CONTAINS_WORD:word") );
}

// ---------------------------------------------------------------------------
// check() — exists

TEST_CASE("ClaimPolicy::check exists — present", "[claim_policy]")
{
    const ClaimPolicy p = ClaimPolicy::parse("sub:exists");
    Payload payload{{"sub", "user123"}};
    std::string err;
    REQUIRE( p.check(payload, err) );
}

TEST_CASE("ClaimPolicy::check exists — missing", "[claim_policy]")
{
    const ClaimPolicy p = ClaimPolicy::parse("sub:exists");
    Payload payload{{"iss", "https://example.com"}};
    std::string err;
    REQUIRE_FALSE( p.check(payload, err) );
    REQUIRE_FALSE( err.empty() );
}

// ---------------------------------------------------------------------------
// check() — equals

TEST_CASE("ClaimPolicy::check equals — match", "[claim_policy]")
{
    const ClaimPolicy p = ClaimPolicy::parse("azp:equals:my-client");
    Payload payload{{"azp", "my-client"}};
    std::string err;
    REQUIRE( p.check(payload, err) );
}

TEST_CASE("ClaimPolicy::check equals — mismatch", "[claim_policy]")
{
    const ClaimPolicy p = ClaimPolicy::parse("azp:equals:my-client");
    Payload payload{{"azp", "other-client"}};
    std::string err;
    REQUIRE_FALSE( p.check(payload, err) );
    REQUIRE( err.find("azp") != std::string::npos );
}

TEST_CASE("ClaimPolicy::check equals — claim missing", "[claim_policy]")
{
    const ClaimPolicy p = ClaimPolicy::parse("azp:equals:my-client");
    Payload payload{};
    std::string err;
    REQUIRE_FALSE( p.check(payload, err) );
}

// ---------------------------------------------------------------------------
// check() — contains (array element or substring)

TEST_CASE("ClaimPolicy::check contains — JSON array with matching element", "[claim_policy]")
{
    const ClaimPolicy p = ClaimPolicy::parse("roles:contains:uda-user");
    // picojson-style JSON array string as stored by jwt-cpp payload serialisation
    Payload payload{{"roles", R"(["uda-user","uda-admin"])"}};
    std::string err;
    REQUIRE( p.check(payload, err) );
}

TEST_CASE("ClaimPolicy::check contains — JSON array without element", "[claim_policy]")
{
    const ClaimPolicy p = ClaimPolicy::parse("roles:contains:uda-user");
    Payload payload{{"roles", R"(["uda-admin","uda-reader"])"}};
    std::string err;
    REQUIRE_FALSE( p.check(payload, err) );
}

TEST_CASE("ClaimPolicy::check contains — plain string substring match", "[claim_policy]")
{
    const ClaimPolicy p = ClaimPolicy::parse("scope:contains:read");
    Payload payload{{"scope", "profile read openid"}};
    std::string err;
    REQUIRE( p.check(payload, err) );
}

// ---------------------------------------------------------------------------
// check() — contains_word (whitespace-delimited token in scope-style claim)

TEST_CASE("ClaimPolicy::check contains_word — word present", "[claim_policy]")
{
    const ClaimPolicy p = ClaimPolicy::parse("scope:contains_word:uda.read");
    Payload payload{{"scope", "profile uda.read openid"}};
    std::string err;
    REQUIRE( p.check(payload, err) );
}

TEST_CASE("ClaimPolicy::check contains_word — substring not accepted as word", "[claim_policy]")
{
    const ClaimPolicy p = ClaimPolicy::parse("scope:contains_word:read");
    Payload payload{{"scope", "profile uda.read openid"}};
    std::string err;
    REQUIRE_FALSE( p.check(payload, err) );
}

TEST_CASE("ClaimPolicy::check contains_word — exact word match", "[claim_policy]")
{
    const ClaimPolicy p = ClaimPolicy::parse("scope:contains_word:read");
    Payload payload{{"scope", "profile read openid"}};
    std::string err;
    REQUIRE( p.check(payload, err) );
}

// ---------------------------------------------------------------------------
// check() — contains_any

TEST_CASE("ClaimPolicy::check contains_any — at least one element matches", "[claim_policy]")
{
    const ClaimPolicy p = ClaimPolicy::parse("roles:contains_any:uda-user,uda-admin");
    Payload payload{{"roles", R"(["uda-user"])"}};
    std::string err;
    REQUIRE( p.check(payload, err) );
}

TEST_CASE("ClaimPolicy::check contains_any — none match", "[claim_policy]")
{
    const ClaimPolicy p = ClaimPolicy::parse("roles:contains_any:uda-user,uda-admin");
    Payload payload{{"roles", R"(["other-role"])"}};
    std::string err;
    REQUIRE_FALSE( p.check(payload, err) );
}

// ---------------------------------------------------------------------------
// check() — nested dot-path resolution

TEST_CASE("ClaimPolicy::check resolves nested path realm_access.roles", "[claim_policy]")
{
    const ClaimPolicy p = ClaimPolicy::parse("realm_access.roles:contains:uda-user");
    // Keycloak-style nested claim
    Payload payload{{"realm_access", R"({"roles":["uda-user","offline_access"]})"}};
    std::string err;
    REQUIRE( p.check(payload, err) );
}

TEST_CASE("ClaimPolicy::check rejects when nested path missing", "[claim_policy]")
{
    const ClaimPolicy p = ClaimPolicy::parse("realm_access.roles:contains:uda-user");
    Payload payload{{"realm_access", R"({"other_field":["something"]})"}};
    std::string err;
    REQUIRE_FALSE( p.check(payload, err) );
}

TEST_CASE("ClaimPolicy::check rejects when root key missing for nested path", "[claim_policy]")
{
    const ClaimPolicy p = ClaimPolicy::parse("realm_access.roles:contains:uda-user");
    Payload payload{};
    std::string err;
    REQUIRE_FALSE( p.check(payload, err) );
}

// ---------------------------------------------------------------------------
// check() — multiple rules, short-circuit on first failure

TEST_CASE("ClaimPolicy::check short-circuits on first failing rule", "[claim_policy]")
{
    const ClaimPolicy p = ClaimPolicy::parse("azp:equals:good-client;scope:contains_word:read");
    Payload payload{{"azp", "bad-client"}, {"scope", "read"}};
    std::string err;
    REQUIRE_FALSE( p.check(payload, err) );
    REQUIRE( err.find("azp") != std::string::npos );
}

TEST_CASE("ClaimPolicy::check all rules must pass", "[claim_policy]")
{
    const ClaimPolicy p = ClaimPolicy::parse("azp:equals:good-client;scope:contains_word:read");
    Payload payload{{"azp", "good-client"}, {"scope", "profile read openid"}};
    std::string err;
    REQUIRE( p.check(payload, err) );
}
