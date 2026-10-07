// Always compiled, in every configuration.
//
// Two jobs:
//   1. It reports, at runtime, whether this build actually has TLS/OIDC compiled in —
//      which is what HELP::servermetadata() answers and what turns "the server ignores
//      my token" into "the server was not built with authentication".
//   2. It gives the authentication object libraries a source file in builds where
//      everything else in them is compiled out. CMake rejects an OBJECT library with no
//      sources, so without this the auth-free configuration fails to configure at all.

namespace uda {
namespace authentication {

bool tls_compiled_in() noexcept
{
#ifdef SSLAUTHENTICATION
    return true;
#else
    return false;
#endif
}

bool oidc_compiled_in() noexcept
{
#ifdef OIDCAUTHENTICATION
    return true;
#else
    return false;
#endif
}

} // namespace authentication
} // namespace uda
