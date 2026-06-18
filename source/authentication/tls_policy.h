#pragma once

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

// Select the first non-empty string from a list of candidates (env-var results or host values).
// Returns an empty string if all candidates are null or empty.
inline std::string select_first_path(std::initializer_list<const char*> candidates,
                                      const std::string& fallback = "") noexcept
{
    for (const char* v : candidates) {
        if (v != nullptr && v[0] != '\0') return v;
    }
    return fallback;
}

} } // namespace uda::authentication
