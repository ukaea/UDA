#include "claim_access.h"

#include <sstream>
#include <nlohmann/json.hpp>

namespace uda {
namespace authentication {

using json = nlohmann::json;

namespace {

// One path component, with its optional array index: "roles[0]" → ("roles", 0).
struct Component {
    std::string key;
    int         index = -1; // -1 = no index
};

Component parse_component(const std::string& s)
{
    const auto open = s.rfind('[');
    if (open == std::string::npos || s.empty() || s.back() != ']') {
        return {s, -1};
    }
    const std::string idx_str = s.substr(open + 1, s.size() - open - 2);
    if (idx_str.empty() || idx_str.find_first_not_of("0123456789") != std::string::npos) {
        return {s, -1}; // malformed index — treat the whole token as a key name
    }
    try {
        return {s.substr(0, open), std::stoi(idx_str)};
    } catch (...) {
        return {s, -1};
    }
}

// Apply an index to a JSON value, if one was given. Returns false if it cannot be applied.
bool apply_index(json& cur, int index)
{
    if (index < 0) {
        return true;
    }
    if (!cur.is_array() || index >= static_cast<int>(cur.size())) {
        return false;
    }
    cur = cur[static_cast<std::size_t>(index)];
    return true;
}

std::string leaf_to_string(const json& j)
{
    return j.is_string() ? j.get<std::string>() : j.dump();
}

} // anonymous namespace

std::vector<std::string> split_claim_path(const std::string& path)
{
    std::vector<std::string> parts;
    std::string current;
    for (std::size_t i = 0; i < path.size(); ++i) {
        const char c = path[i];
        if (c == '\\' && i + 1 < path.size() && path[i + 1] == '.') {
            current += '.'; // escaped dot — part of the claim name, not a separator
            ++i;
        } else if (c == '.') {
            parts.push_back(current);
            current.clear();
        } else {
            current += c;
        }
    }
    parts.push_back(current);
    return parts;
}

std::optional<std::string> resolve_claim(const ClaimMap& payload, const std::string& path)
{
    if (path.empty()) {
        return std::nullopt;
    }

    const auto parts = split_claim_path(path);
    const Component first = parse_component(parts[0]);

    const auto it = payload.find(first.key);
    if (it == payload.end()) {
        return std::nullopt;
    }

    // Fast path: a single flat key with no index needs no JSON parsing at all.
    if (parts.size() == 1 && first.index < 0) {
        return it->second;
    }

    json cur;
    try {
        cur = json::parse(it->second);
    } catch (...) {
        return std::nullopt; // a plain string claim cannot be navigated further
    }

    if (!apply_index(cur, first.index)) {
        return std::nullopt;
    }

    for (std::size_t i = 1; i < parts.size(); ++i) {
        const Component comp = parse_component(parts[i]);
        if (!cur.is_object()) {
            return std::nullopt;
        }
        const auto found = cur.find(comp.key);
        if (found == cur.end()) {
            return std::nullopt;
        }
        cur = *found;
        if (!apply_index(cur, comp.index)) {
            return std::nullopt;
        }
    }

    return leaf_to_string(cur);
}

bool claim_contains_element(const std::string& value, const std::string& needle)
{
    if (!value.empty() && value.front() == '[') {
        try {
            const auto arr = json::parse(value);
            if (arr.is_array()) {
                for (const auto& elem : arr) {
                    if (elem.is_string()) {
                        if (elem.get<std::string>() == needle) return true;
                    } else if (elem.dump() == needle) {
                        return true;
                    }
                }
                return false;
            }
        } catch (...) {
        }
    }
    return value.find(needle) != std::string::npos;
}

bool claim_contains_word(const std::string& value, const std::string& word)
{
    std::istringstream ss(value);
    std::string token;
    while (ss >> token) {
        if (token == word) {
            return true;
        }
    }
    return false;
}

bool claim_contains(const ClaimMap& payload, const std::string& path, const std::string& needle)
{
    const auto value = resolve_claim(payload, path);
    if (!value) {
        return false;
    }
    // A JSON array claim answers on element membership; anything else is treated as a
    // space-delimited token list, which is what `scope` and friends actually are.
    if (!value->empty() && value->front() == '[') {
        return claim_contains_element(*value, needle);
    }
    return claim_contains_word(*value, needle);
}

} // namespace authentication
} // namespace uda
