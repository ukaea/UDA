#pragma once

#include <cstdlib>
#include <string>
#include <string_view>
#include <algorithm>
#include <cctype>
#include <vector>
#include <optional>

namespace uda::common::env_config {

inline std::string to_lower(std::string_view text) {
    std::string lower(text);
    std::transform(lower.begin(), lower.end(), lower.begin(), [] (unsigned char c) { return std::tolower(c); });
    return lower;
}

// std::getenv needs a null-terminated name, which a string_view does not guarantee.
inline const char* get_env(std::string_view var_name) {
    return std::getenv(std::string(var_name).c_str());
}

const std::vector<std::string> truthy_values = {"1", "true", "yes", "on"};
const std::vector<std::string> falsey_values = {"0", "false", "no", "off"};

// Case-insensitive: true if val is one of accepted_values.
inline bool strings_match(std::string_view val, const std::vector<std::string>& accepted_values) {
    const std::string value = to_lower(val);
    return std::any_of(accepted_values.begin(), accepted_values.end(),
                       [&] (const std::string& accepted) { return value == to_lower(accepted); });
}

inline bool match_custom_values(std::string_view var_name, const std::vector<std::string>& accepted_values,
                                bool default_value=false) {
    const char* value = get_env(var_name);
    if (value == nullptr) {
        return default_value;
    }
    return strings_match(value, accepted_values);
}

inline std::optional<std::string> 
get_custom_param(std::string_view var_name, const std::vector<std::string>& accepted_values) {
    const char* value = get_env(var_name);
    if (value == nullptr) {
        return {};
    }
    if (strings_match(value, accepted_values)) {
        return value;
    }
    return {};
}

inline bool evaluate_bool_param(std::string_view var_name, bool default_value=false) {

    const char* val = get_env(var_name);
    if (val == nullptr) {
        return default_value;
    }
    if (strings_match(val, truthy_values)) {
        return true;
    }
    if (strings_match(val, falsey_values)) {
        return false;
    }
    return default_value;
}

} // namespace
