#pragma once

#include <string>
#include <unordered_map>

namespace uda::authentication {

using PayloadType = std::unordered_map<std::string, std::string>;

PayloadType authenticate(const std::string& token);

}

struct AuthPayload {
    const uda::authentication::PayloadType* auth_payload;
};