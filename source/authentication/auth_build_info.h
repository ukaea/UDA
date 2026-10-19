#pragma once

// Runtime answers to "what was this build compiled with?". Available in every
// configuration, including builds with no authentication support at all.

namespace uda {
namespace authentication {

// True when this binary was compiled with TLS transport support (SSLAUTHENTICATION).
bool tls_compiled_in() noexcept;

// True when this binary was compiled with OIDC bearer-token support (OIDCAUTHENTICATION).
bool oidc_compiled_in() noexcept;

} // namespace authentication
} // namespace uda
