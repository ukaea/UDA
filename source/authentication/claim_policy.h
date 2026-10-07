#pragma once

#include <string>
#include <unordered_map>
#include <vector>

namespace uda {
namespace authentication {

// Claim check operations supported by the claim policy engine.
enum class ClaimOp {
    Exists,       // claim path must be present (any value)
    Equals,       // claim string value must exactly equal operand
    Contains,     // claim value (as JSON array or plain string) must contain operand as an element
    ContainsWord, // claim value must contain operand as a whitespace-delimited token (e.g. scope)
    ContainsAny,  // claim value must contain any element from a comma-separated operand list
};

struct ClaimRule {
    std::string path;  // dot-separated claim path, e.g. "azp" or "realm_access.roles"
    ClaimOp     op;
    std::string value; // operand; empty for Exists; comma-separated list for ContainsAny
};

// Configurable claim policy built from a spec string.
//
// Spec format — semicolons separate rules; within each rule colons separate fields:
//
//   <path>:<op>              (for Exists)
//   <path>:<op>:<value>      (for all other ops)
//
// Supported ops (case-insensitive):
//   exists        claim must be present
//   equals        exact string match
//   contains      element membership in a JSON array or substring in a string
//   contains_word whitespace-delimited word match (for claims like scope)
//   contains_any  any of a comma-separated list of values must be present
//
// Path resolution:
//   "azp"                  top-level claim
//   "realm_access.roles"   parse map["realm_access"] as JSON, navigate to "roles"
//
// Example spec:
//   "azp:equals:uda-client;scope:contains_word:uda.read;realm_access.roles:contains:uda-user"
//
class ClaimPolicy {
public:
    ClaimPolicy() = default;

    // Parse a semicolon-separated claim spec.
    // Throws std::invalid_argument on malformed input so callers can fail early.
    static ClaimPolicy parse(const std::string& spec);

    bool empty() const { return rules_.empty(); }

    // Evaluate all rules against the decoded JWT payload (unordered_map<string,string>).
    // Nested JSON values are parsed on demand.
    // Returns true if all rules pass; sets error to a human-readable description on failure.
    bool check(const std::unordered_map<std::string, std::string>& payload, std::string& error) const;

private:
    std::vector<ClaimRule> rules_;
};

} // namespace authentication
} // namespace uda
