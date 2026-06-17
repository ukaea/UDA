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

inline const char* tlsModeStr(TlsMode mode)
{
    switch (mode) {
        case TlsMode::Off:        return "off";
        case TlsMode::ServerOnly: return "server-only";
        case TlsMode::Mutual:     return "mutual";
    }
    return "unknown";
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

// Returns true if a TLS env var requests any TLS mode (server or mutual).
// Used for compile/runtime mismatch detection.
inline bool tlsEnvRequestsTls(const char* mode_var, const char* legacy_auth_var)
{
    if (const char* mode = std::getenv(mode_var)) {
        const std::string s = normalise_tls_mode(mode);
        if (s == "server" || s == "mutual") return true;
    }
    const char* legacy = std::getenv(legacy_auth_var);
    if (!legacy) return false;
    const std::string v = normalise_tls_mode(legacy);
    return v == "1" || v == "true" || v == "yes" || v == "on";
}

// TLS verification policy helpers.
// These read env vars at call time and return a bool.

// UDA_CLIENT_TLS_VERIFY_HOSTNAME: default enabled (1).
// Disable with =0 to skip hostname verification (logs a warning).
inline bool clientTlsVerifyHostname()
{
    const char* v = std::getenv("UDA_CLIENT_TLS_VERIFY_HOSTNAME");
    if (!v) return true; // secure default
    const std::string s = normalise_tls_mode(v);
    return s != "0" && s != "false" && s != "no" && s != "off";
}

// UDA_CLIENT_TLS_VERIFY_CERT_DATES: default enabled (1).
inline bool clientTlsVerifyCertDates()
{
    const char* v = std::getenv("UDA_CLIENT_TLS_VERIFY_CERT_DATES");
    if (!v) return true;
    const std::string s = normalise_tls_mode(v);
    return s != "0" && s != "false" && s != "no" && s != "off";
}

// UDA_SERVER_TLS_VERIFY_CLIENT_CERT_DATES: default enabled (1).
inline bool serverTlsVerifyClientCertDates()
{
    const char* v = std::getenv("UDA_SERVER_TLS_VERIFY_CLIENT_CERT_DATES");
    if (!v) return true;
    const std::string s = normalise_tls_mode(v);
    return s != "0" && s != "false" && s != "no" && s != "off";
}
