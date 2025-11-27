#pragma once

#include <cstdlib>
#include <string>
#include <algorithm>
#include <cctype>
#include <vector>
#include <optional>

namespace uda::common::env_config {

const std::vector<std::string> truthy_values = {"1", "true", "yes", "on"};
const std::vector<std::string> falsey_values = {"0", "false", "no", "off"};

inline bool strings_match(std::string_view val, const std::vector<std::string>& accepted_values) {
    std::string value(val);
    std::transform(value.begin(), value.end(), value.begin(),
                   [] (unsigned char c) {return std::tolower(c); });

    // case insensitive comparison
    return std::any_of(accepted_values.begin(), accepted_values.end(),
                       [&] (std::string v){ 
                       std::transform(v.begin(), v.end(), v.begin(),
                               [] (unsigned char c) {return std::tolower(c); });
                       return value == v; });
}

inline bool match_custom_values(std::string_view var_name, const std::vector<std::string>& accepted_values,
                                bool default_value=false) {
    const char* value = std::getenv(var_name.data());
    if (value == nullptr) {
        return default_value;
    }
    return strings_match(value, accepted_values);
}

inline std::optional<std::string> 
get_custom_param(std::string_view var_name, const std::vector<std::string>& accepted_values) {
    const char* value = std::getenv(var_name.data());
    if (value == nullptr) {
        return {};
    }
    if (strings_match(value, accepted_values)) {
        return value;
    }
    return {};
}

inline bool evaluate_bool_param(std::string_view var_name, bool default_value=false) {

    const char* val = std::getenv(var_name.data());
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
