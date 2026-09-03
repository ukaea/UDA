#pragma once

#include <algorithm>
#include <cctype>
#include <cstdlib>
#include <string>
#include <string_view>

#include <clientserver/errorLog.h>

enum class TlsMode {
    Off,
    ServerOnly,
    Mutual,
};

enum class PeerCertPolicy {
    NotRequired,
    Required,
};

inline std::string normalise_tls_mode(std::string_view mode)
{
    std::string value(mode);
    std::transform(value.begin(), value.end(), value.begin(),
                   [](unsigned char c) { return static_cast<char>(std::tolower(c)); });
    return value;
}

inline TlsMode parseTlsMode(const char* var_name, const char* value)
{
    const std::string mode = normalise_tls_mode(value);
    if (mode == "off") {
        return TlsMode::Off;
    }
    if (mode == "server") {
        return TlsMode::ServerOnly;
    }
    if (mode == "mutual") {
        return TlsMode::Mutual;
    }

    std::string msg = std::string("Invalid ") + var_name + " value '" + value + "'; expected off, server, or mutual";
    UDA_ADD_ERROR(999, msg.c_str());
    return TlsMode::Off;
}

inline bool isTlsModeSet(const char* var_name)
{
    return std::getenv(var_name) != nullptr;
}

inline TlsMode getTlsMode(const char* mode_var, const char* legacy_auth_var)
{
    if (const char* mode = std::getenv(mode_var)) {
        return parseTlsMode(mode_var, mode);
    }

    const char* legacy = std::getenv(legacy_auth_var);
    const std::string legacy_value = legacy == nullptr ? "" : normalise_tls_mode(legacy);
    if (legacy_value == "1" || legacy_value == "true" || legacy_value == "yes" || legacy_value == "on") {
        return TlsMode::Mutual;
    }

    return TlsMode::Off;
}

inline TlsMode getServerTlsMode()
{
    return getTlsMode("UDA_SERVER_TLS_MODE", "UDA_SERVER_SSL_AUTHENTICATE");
}

inline TlsMode getClientTlsMode()
{
    return getTlsMode("UDA_CLIENT_TLS_MODE", "UDA_CLIENT_SSL_AUTHENTICATE");
}

inline bool isValidTlsModeEnv(const char* var_name)
{
    const char* value = std::getenv(var_name);
    if (value == nullptr) {
        return true;
    }

    const std::string mode = normalise_tls_mode(value);
    return mode == "off" || mode == "server" || mode == "mutual";
}
