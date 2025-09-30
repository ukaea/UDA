#pragma once

#include <cstdlib>
#include <string>
#include <algorithm>
#include <cctype>
#include <vector>

namespace uda::common::env_options {

const std::vector<std::string> truthy_values = {"1", "true", "yes", "on"};
const std::vector<std::string> falsey_values = {"0", "false", "no", "off"};


inline bool strings_match(std::string_view val, const std::vector<std::string>& accepted_values) {
    std::string value(val);
    std::transform(value.begin(), value.end(), value.begin(),
                   [] (unsigned char c) {return std::tolower(c); });

    return std::any_of(accepted_values.begin(), accepted_values.end(),
                       [&] (std::string v){ 
                       std::transform(v.begin(), v.end(), v.begin(),
                               [] (unsigned char c) {return std::tolower(c); });
                       return value == v; });
}

inline bool match_env_option(const char* val, const std::vector<std::string>& accepted_values) {
    const char* value = std::getenv(val);
    if (value == nullptr) {
        return false;
    }
    return strings_match(value, accepted_values);
}

inline bool evaluate_env_option(const char* var_name, bool default_value=false) {

    const char* val = std::getenv(var_name);
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
