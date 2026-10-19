#pragma once

// The lifetime rule for an authenticated connection.
//
// A bearer token is verified exactly once, during the handshake. The claims established
// there are what the rest of the connection runs on: the client re-sends the token with
// every request, but the server neither re-parses nor re-verifies it.
//
// That leaves one question — how long does the session last? The answer UDA implements:
//
//     an authenticated session lasts for the lifetime of the connection or the lifetime
//     of the token, whichever is shorter.
//
// The token's own `exp` is therefore checked before each request is served, against the
// value captured at the handshake. When it passes, the server closes the connection. That
// keeps a token's expiry meaningful without re-verifying a signature on the data path, and
// without the server ever trusting a token it has not itself checked.
//
// It also means a client cannot change identity mid-connection. A client that obtains a
// new token must open a new connection to use it; a replacement sent on an existing
// connection is ignored, not honoured and not rejected. See docs/authentication.md.

#include <ctime>
#include <string>
#include <unordered_map>

namespace uda {
namespace authentication {

using PayloadType = std::unordered_map<std::string, std::string>;

struct AuthSession {
    // True when this connection was established against a verified bearer token. False for
    // an unauthenticated server, where nothing here applies.
    bool token_backed = false;

    // Unix time at which the token expires, from its `exp` claim. Zero means the token
    // carried no expiry, in which case the session is bounded only by the connection.
    std::time_t expires_at = 0;

    // The same clock skew allowance used when the token was verified, so the session does
    // not end a moment before the verifier would still have accepted the token.
    int clock_skew_seconds = 0;

    // Has the token backing this session expired as of `now`?
    [[nodiscard]] bool expired(std::time_t now) const noexcept
    {
        if (!token_backed || expires_at == 0) {
            return false;
        }
        return now > expires_at + static_cast<std::time_t>(clock_skew_seconds);
    }

    // Seconds remaining, clamped at zero. For logging and diagnostics.
    [[nodiscard]] long seconds_remaining(std::time_t now) const noexcept
    {
        if (!token_backed || expires_at == 0) {
            return -1; // no expiry
        }
        const long remaining =
            static_cast<long>(expires_at + static_cast<std::time_t>(clock_skew_seconds) - now);
        return remaining > 0 ? remaining : 0;
    }
};

// Build the session description from a verified payload. `exp` is a numeric claim, so it
// arrives in the payload map as its JSON serialisation; a missing or unparsable value
// yields no expiry rather than an immediately-expired session.
AuthSession make_auth_session(const PayloadType& payload, int clock_skew_seconds);

} // namespace authentication
} // namespace uda
