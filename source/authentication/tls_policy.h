#pragma once

#include <cstdlib>
#include <initializer_list>
#include <string>
#include "tlsMode.h"

namespace uda { namespace authentication {

// If no explicit TLS mode was set in the environment and there are legacy SSL signals
// (SSL:// protocol prefix or host-list entry with isSSL=true), fall back to Mutual.
// This preserves backward compat with the legacy SSL:// URL scheme.
inline TlsMode resolve_client_tls_mode(TlsMode explicit_mode,
                                        bool    explicit_mode_set,
                                        bool    has_ssl_protocol_flag,
                                        bool    host_is_ssl) noexcept
{
    if (!explicit_mode_set && explicit_mode == TlsMode::Off
        && (has_ssl_protocol_flag || host_is_ssl)) {
        return TlsMode::Mutual;
    }
    return explicit_mode;
}

// Choose the hostname to use for TLS verification.
// The actual connected hostname (from connection.cpp) takes precedence because it
// is the name the OS resolved and connected to; the host-list entry may be an alias.
inline std::string select_verification_hostname(const std::string& connected,
                                                 const std::string& hostlist_name) noexcept
{
    return !connected.empty() ? connected : hostlist_name;
}

// Derive whether a peer certificate is required from the TLS mode.
// Mutual: both sides present certs. ServerOnly: only the server presents.
inline PeerCertPolicy peer_cert_policy(TlsMode mode) noexcept
{
    return mode == TlsMode::Mutual ? PeerCertPolicy::Required : PeerCertPolicy::NotRequired;
}

// Return the value of the first of `names` that is set and non-empty, or nullptr.
// Both the client and server TLS setup read their certificate paths through a chain of
// preferred-then-legacy variable names; this is the one implementation of that chain.
inline const char* first_env(const std::initializer_list<const char*>& names) noexcept
{
    for (const char* name : names) {
        const char* value = std::getenv(name);
        if (value != nullptr && value[0] != '\0') {
            return value;
        }
    }
    return nullptr;
}

} } // namespace uda::authentication
