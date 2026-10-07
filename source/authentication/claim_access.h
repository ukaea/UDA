#pragma once

// Claim access — the one implementation of "read a value out of a verified JWT payload".
//
// Both the server-side claim policy (claim_policy.cpp, driven by
// UDA_SERVER_OIDC_REQUIRED_CLAIMS) and the plugin-facing helpers
// (authPayloadPath / authPayloadContains in plugins/udaPlugin.cpp) resolve claim paths
// against the same flat PayloadType map. They used to do it twice, with two different
// path languages; this is the shared engine so there is only one to learn.
//
// Path syntax
// -----------
//   preferred_username              top-level claim
//   realm_access.roles              navigate into a JSON-encoded nested claim
//   realm_access.roles[0]           index into a JSON array
//   resource_access.uda-client.roles[1]
//   wlcg\.groups                    a literal dot in a claim name, backslash-escaped
//
// The first component is looked up directly in the payload map. Any further components
// navigate into the JSON parsed from that first value — which is how non-string claims
// are stored (see PayloadType in oauth_authentication.h). A leaf string is returned
// unquoted; any other leaf is returned as its JSON serialisation.

#include <optional>
#include <string>
#include <unordered_map>
#include <vector>

namespace uda {
namespace authentication {

using ClaimMap = std::unordered_map<std::string, std::string>;

// Resolve a claim path. Returns nullopt if any component is missing, an index is out of
// range, or a component is applied to a value that cannot be navigated.
std::optional<std::string> resolve_claim(const ClaimMap& payload, const std::string& path);

// Element membership: if value is a JSON array, is needle one of its elements?
// Otherwise, does value contain needle as a substring?
bool claim_contains_element(const std::string& value, const std::string& needle);

// Word membership: is word one of value's whitespace-delimited tokens?
// This is the right test for space-delimited claims such as `scope`.
bool claim_contains_word(const std::string& value, const std::string& word);

// Convenience: resolve `path`, then test membership. Tries element membership first and
// falls back to word membership, so it covers both JSON-array claims (realm_access.roles)
// and space-delimited string claims (scope) without the caller choosing.
bool claim_contains(const ClaimMap& payload, const std::string& path, const std::string& needle);

// Split a claim path into its components, honouring backslash-escaped dots.
// Exposed for testing.
std::vector<std::string> split_claim_path(const std::string& path);

} // namespace authentication
} // namespace uda
