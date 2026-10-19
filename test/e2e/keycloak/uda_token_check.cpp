// uda_token_check — run UDA's real token verification against a real identity provider.
//
// Everything else in the test suite is offline: generated keys, injected JWKS, mock
// fetchers. This is the one place the production path — discovery, JWKS fetch, signature
// verification, issuer/audience checks, claim policy — runs end to end against a live IdP
// and a token that IdP actually issued.
//
// It deliberately calls the same authenticate() the server calls, so the accept/refuse
// decision here is the decision the server would make.
//
//   uda_token_check --token <jwt> --issuer <url> [--audience <aud>]
//                   [--required-claims <spec>] [--allow-http] [--quiet]
//
// Exit status mirrors the operator-facing outcome:
//   0  accepted
//   2  refused  (the UDA error code and reason are printed)
//   3  usage or internal error

#include <authentication/auth_error.h>
#include <authentication/oauth_authentication.h>
#include <authentication/oidc_config.h>

#include <cstdlib>
#include <cstring>
#include <iostream>
#include <string>

namespace {

[[noreturn]] void usage(const char* argv0, int code)
{
    std::cerr << "usage: " << argv0 << " --token <jwt> --issuer <url>\n"
              << "          [--audience <aud>] [--required-claims <spec>]\n"
              << "          [--jwks-uri <url>] [--allow-http] [--quiet]\n";
    std::exit(code);
}

const char* code_name(uda::authentication::AuthErrorCode c)
{
    using uda::authentication::AuthErrorCode;
    switch (c) {
        case AuthErrorCode::MissingToken:        return "MISSING_TOKEN";
        case AuthErrorCode::InvalidConfig:       return "INVALID_CONFIG";
        case AuthErrorCode::DiscoveryFailed:     return "DISCOVERY_FAILED";
        case AuthErrorCode::JwksFetchFailed:     return "JWKS_FETCH";
        case AuthErrorCode::InvalidToken:        return "INVALID_TOKEN";
        case AuthErrorCode::TokenExpired:        return "TOKEN_EXPIRED";
        case AuthErrorCode::TokenBadIssuer:      return "TOKEN_BAD_ISSUER";
        case AuthErrorCode::TokenBadAudience:    return "TOKEN_BAD_AUDIENCE";
        case AuthErrorCode::ClaimPolicyFailed:   return "CLAIM_POLICY";
        case AuthErrorCode::TlsConfigError:      return "TLS_CONFIG";
        case AuthErrorCode::TlsHandshakeFailed:  return "TLS_HANDSHAKE";
        case AuthErrorCode::TlsHostnameMismatch: return "TLS_HOSTNAME_MISMATCH";
    }
    return "UNKNOWN";
}

} // namespace

int main(int argc, char** argv)
{
    using namespace uda::authentication;

    std::string token, issuer, audience, required_claims, jwks_uri;
    bool quiet = false;

    for (int i = 1; i < argc; ++i) {
        const std::string a = argv[i];
        auto next = [&](const char* what) -> std::string {
            if (i + 1 >= argc) {
                std::cerr << "missing value for " << what << "\n";
                usage(argv[0], 3);
            }
            return argv[++i];
        };
        if      (a == "--token")           token           = next("--token");
        else if (a == "--issuer")          issuer          = next("--issuer");
        else if (a == "--audience")        audience        = next("--audience");
        else if (a == "--required-claims") required_claims = next("--required-claims");
        else if (a == "--jwks-uri")        jwks_uri        = next("--jwks-uri");
        else if (a == "--allow-http")      setenv("UDA_SERVER_OIDC_ALLOW_HTTP", "1", 1);
        else if (a == "--quiet")           quiet           = true;
        else if (a == "-h" || a == "--help") usage(argv[0], 0);
        else {
            std::cerr << "unknown argument: " << a << "\n";
            usage(argv[0], 3);
        }
    }

    if (token.empty() || (issuer.empty() && jwks_uri.empty())) {
        usage(argv[0], 3);
    }

    OidcConfig cfg;
    cfg.issuer          = issuer;
    cfg.jwks_uri        = jwks_uri;
    cfg.audience        = audience;
    cfg.required_claims = required_claims;
    cfg.verify_issuer   = !issuer.empty();
    cfg.verify_audience = !audience.empty();

    try {
        const PayloadType payload = authenticate(token, cfg, curl_http_fetch);
        if (!quiet) {
            const auto sub  = payload.find("sub");
            const auto user = payload.find("preferred_username");
            std::cout << "ACCEPTED"
                      << " user=" << (user != payload.end() ? user->second : "-")
                      << " sub="  << (sub  != payload.end() ? sub->second  : "-")
                      << "\n";
        }
        return 0;
    } catch (const AuthError& e) {
        if (!quiet) {
            std::cout << "REFUSED code=" << authErrorToUdaCode(e.code)
                      << " reason=" << code_name(e.code)
                      << " detail=" << e.what() << "\n";
        }
        return 2;
    } catch (const std::exception& e) {
        std::cerr << "internal error: " << e.what() << "\n";
        return 3;
    }
}
