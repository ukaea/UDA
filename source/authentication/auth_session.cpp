#include "auth_session.h"

#include <cstdlib>
#include <string>

namespace uda {
namespace authentication {

AuthSession make_auth_session(const PayloadType& payload, int clock_skew_seconds)
{
    AuthSession session;
    session.token_backed      = true;
    session.clock_skew_seconds = clock_skew_seconds > 0 ? clock_skew_seconds : 0;

    const auto it = payload.find("exp");
    if (it == payload.end()) {
        return session; // no expiry claim; bounded by the connection only
    }

    // `exp` is a NumericDate — seconds since the epoch. It reaches the payload map as text
    // because the map is flat; anything unparsable is treated as absent, because refusing
    // every request is the wrong response to a claim we merely failed to read.
    try {
        const long long value = std::stoll(it->second);
        if (value > 0) {
            session.expires_at = static_cast<std::time_t>(value);
        }
    } catch (const std::exception&) {
        // leave expires_at at 0
    }

    return session;
}

} // namespace authentication
} // namespace uda
