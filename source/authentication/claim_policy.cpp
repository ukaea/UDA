#include "claim_policy.h"

#include <algorithm>
#include <optional>
#include <sstream>
#include <stdexcept>
#include <nlohmann/json.hpp>

namespace uda {
namespace authentication {

using json = nlohmann::json;
using PayloadMap = std::unordered_map<std::string, std::string>;

namespace {

ClaimOp parse_op(const std::string& s)
{
    std::string lower = s;
    std::transform(lower.begin(), lower.end(), lower.begin(), ::tolower);
    if (lower == "exists")        return ClaimOp::Exists;
    if (lower == "equals")        return ClaimOp::Equals;
    if (lower == "contains")      return ClaimOp::Contains;
    if (lower == "contains_word") return ClaimOp::ContainsWord;
    if (lower == "contains_any")  return ClaimOp::ContainsAny;
    throw std::invalid_argument("Unknown claim op: " + s);
}

std::vector<std::string> split(const std::string& s, char delim)
{
    std::vector<std::string> result;
    std::istringstream ss(s);
    std::string token;
    while (std::getline(ss, token, delim)) {
        result.push_back(token);
    }
    return result;
}

// Resolve a dot-separated claim path against the flat payload map.
// The first component is looked up directly; remaining components navigate
// into a JSON-encoded nested value (e.g. realm_access.roles → parse
// map["realm_access"] as JSON, then get ["roles"]).
std::optional<std::string> resolve_path(const PayloadMap& payload, const std::string& path)
{
    const auto dot = path.find('.');
    if (dot == std::string::npos) {
        auto it = payload.find(path);
        if (it == payload.end()) return std::nullopt;
        return it->second;
    }

    const std::string root = path.substr(0, dot);
    const std::string rest = path.substr(dot + 1);

    auto it = payload.find(root);
    if (it == payload.end()) return std::nullopt;

    try {
        auto j = json::parse(it->second);
        const auto parts = split(rest, '.');
        json cur = j;
        for (const auto& part : parts) {
            if (!cur.is_object() || !cur.contains(part)) return std::nullopt;
            cur = cur[part];
        }
        if (cur.is_string()) return cur.get<std::string>();
        return cur.dump();
    } catch (...) {
        return std::nullopt;
    }
}

// Check whether val (a plain string or JSON array string) contains needle as an element.
bool value_contains_element(const std::string& val, const std::string& needle)
{
    if (!val.empty() && val.front() == '[') {
        try {
            const auto arr = json::parse(val);
            if (arr.is_array()) {
                for (const auto& elem : arr) {
                    if (elem.is_string() && elem.get<std::string>() == needle) return true;
                    if (!elem.is_string() && elem.dump() == needle) return true;
                }
                return false;
            }
        } catch (...) {}
    }
    return val.find(needle) != std::string::npos;
}

// Check whether val contains word as a whitespace-delimited token.
bool value_contains_word(const std::string& val, const std::string& word)
{
    std::istringstream ss(val);
    std::string token;
    while (ss >> token) {
        if (token == word) return true;
    }
    return false;
}

bool check_rule(const ClaimRule& rule, const PayloadMap& payload, std::string& error)
{
    const auto maybe_val = resolve_path(payload, rule.path);

    switch (rule.op) {
        case ClaimOp::Exists:
            if (!maybe_val) {
                error = "Required claim '" + rule.path + "' is missing";
                return false;
            }
            return true;

        case ClaimOp::Equals:
            if (!maybe_val) {
                error = "Required claim '" + rule.path + "' is missing";
                return false;
            }
            if (*maybe_val != rule.value) {
                error = "Claim '" + rule.path + "' has unexpected value (expected '" + rule.value + "')";
                return false;
            }
            return true;

        case ClaimOp::Contains:
            if (!maybe_val) {
                error = "Required claim '" + rule.path + "' is missing";
                return false;
            }
            if (!value_contains_element(*maybe_val, rule.value)) {
                error = "Claim '" + rule.path + "' does not contain '" + rule.value + "'";
                return false;
            }
            return true;

        case ClaimOp::ContainsWord:
            if (!maybe_val) {
                error = "Required claim '" + rule.path + "' is missing";
                return false;
            }
            if (!value_contains_word(*maybe_val, rule.value)) {
                error = "Claim '" + rule.path + "' does not contain word '" + rule.value + "'";
                return false;
            }
            return true;

        case ClaimOp::ContainsAny: {
            if (!maybe_val) {
                error = "Required claim '" + rule.path + "' is missing";
                return false;
            }
            const auto options = split(rule.value, ',');
            for (const auto& opt : options) {
                if (value_contains_element(*maybe_val, opt)) return true;
            }
            error = "Claim '" + rule.path + "' does not contain any of '" + rule.value + "'";
            return false;
        }
    }
    return false;
}

} // namespace

ClaimPolicy ClaimPolicy::parse(const std::string& spec)
{
    ClaimPolicy policy;
    if (spec.empty()) return policy;

    const auto rule_strs = split(spec, ';');
    for (const auto& rule_str : rule_strs) {
        if (rule_str.empty()) continue;

        const auto parts = split(rule_str, ':');
        if (parts.size() < 2) {
            throw std::invalid_argument(
                "Malformed claim rule (expected path:op[:value]): " + rule_str);
        }

        ClaimRule rule;
        rule.path = parts[0];
        rule.op   = parse_op(parts[1]);

        if (parts.size() >= 3) {
            rule.value = parts[2];
            // Rejoin if the value itself contains colons (e.g. resource_access.client:contains:role)
            for (size_t i = 3; i < parts.size(); ++i) {
                rule.value += ':' + parts[i];
            }
        } else if (rule.op != ClaimOp::Exists) {
            throw std::invalid_argument(
                "Claim rule op '" + parts[1] + "' requires a value: " + rule_str);
        }

        policy.rules_.push_back(std::move(rule));
    }
    return policy;
}

bool ClaimPolicy::check(const PayloadMap& payload, std::string& error) const
{
    for (const auto& rule : rules_) {
        if (!check_rule(rule, payload, error)) {
            return false;
        }
    }
    return true;
}

} // namespace authentication
} // namespace uda
