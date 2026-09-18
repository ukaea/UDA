// Tests for the session lifetime rule: an authenticated session lasts for the lifetime of
// the connection or the lifetime of the token, whichever is shorter.

#include <catch2/catch_test_macros.hpp>

#include <authentication/auth_session.h>

using uda::authentication::AuthSession;
using uda::authentication::PayloadType;
using uda::authentication::make_auth_session;

namespace {
constexpr std::time_t NOW = 1'800'000'000; // a fixed point, so nothing here depends on the clock
}

TEST_CASE("a session takes its expiry from the token's exp claim", "[auth_session]")
{
    const PayloadType payload{{"sub", "alice"}, {"exp", std::to_string(NOW + 300)}};
    const auto session = make_auth_session(payload, 0);

    REQUIRE(session.token_backed);
    REQUIRE(session.expires_at == NOW + 300);
    REQUIRE_FALSE(session.expired(NOW));
}

TEST_CASE("a session ends once the token has expired", "[auth_session]")
{
    const PayloadType payload{{"exp", std::to_string(NOW - 1)}};
    const auto session = make_auth_session(payload, 0);

    REQUIRE(session.expired(NOW));
}

TEST_CASE("expiry honours the same clock skew the verifier used", "[auth_session]")
{
    // A session must not end a moment before the verifier would still have accepted the
    // token, or the two disagree about the same instant.
    const PayloadType payload{{"exp", std::to_string(NOW - 30)}};
    const auto session = make_auth_session(payload, 60);

    REQUIRE_FALSE(session.expired(NOW));      // 30s past exp, within 60s skew
    REQUIRE(session.expired(NOW + 31));       // 61s past exp, beyond it
}

TEST_CASE("the boundary instant is not yet expired", "[auth_session]")
{
    const PayloadType payload{{"exp", std::to_string(NOW)}};
    const auto session = make_auth_session(payload, 0);

    REQUIRE_FALSE(session.expired(NOW));  // expiry is "past exp", not "at exp"
    REQUIRE(session.expired(NOW + 1));
}

TEST_CASE("a token with no exp bounds the session only by the connection", "[auth_session]")
{
    const PayloadType payload{{"sub", "alice"}};
    const auto session = make_auth_session(payload, 0);

    REQUIRE(session.token_backed);
    REQUIRE(session.expires_at == 0);
    REQUIRE_FALSE(session.expired(NOW));
    REQUIRE_FALSE(session.expired(NOW + 10'000'000));
    REQUIRE(session.seconds_remaining(NOW) == -1); // -1 means "no expiry"
}

TEST_CASE("an unreadable exp is treated as absent, not as expired", "[auth_session]")
{
    // Refusing every request is the wrong response to a claim we merely failed to parse:
    // it converts a cosmetic problem into a total outage.
    for (const char* bad : {"not-a-number", "", "-1", "0"}) {
        const PayloadType payload{{"exp", bad}};
        const auto session = make_auth_session(payload, 0);
        REQUIRE(session.expires_at == 0);
        REQUIRE_FALSE(session.expired(NOW));
    }
}

TEST_CASE("an unauthenticated session never expires", "[auth_session]")
{
    // The default-constructed session is what an auth-free server runs with. Nothing about
    // token lifetime applies to it.
    const AuthSession session;

    REQUIRE_FALSE(session.token_backed);
    REQUIRE_FALSE(session.expired(NOW));
    REQUIRE_FALSE(session.expired(NOW + 10'000'000));
}

TEST_CASE("seconds_remaining reports the time left and clamps at zero", "[auth_session]")
{
    const PayloadType payload{{"exp", std::to_string(NOW + 120)}};
    const auto session = make_auth_session(payload, 0);

    REQUIRE(session.seconds_remaining(NOW) == 120);
    REQUIRE(session.seconds_remaining(NOW + 119) == 1);
    REQUIRE(session.seconds_remaining(NOW + 120) == 0);
    REQUIRE(session.seconds_remaining(NOW + 500) == 0); // never negative
}

TEST_CASE("a fractional or oversized exp does not wrap", "[auth_session]")
{
    // exp is a NumericDate; a provider emitting something odd must not produce a session
    // that has already expired or that lasts forever by accident.
    const PayloadType fractional{{"exp", "1800000000.75"}};
    REQUIRE(make_auth_session(fractional, 0).expires_at == 1'800'000'000);

    const PayloadType huge{{"exp", "999999999999999999999"}};
    REQUIRE(make_auth_session(huge, 0).expires_at == 0); // unparsable, treated as absent
}
