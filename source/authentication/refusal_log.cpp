#if defined(SSLAUTHENTICATION) || defined(OIDCAUTHENTICATION)

#include "refusal_log.h"

#include <cerrno>
#include <cstdio>
#include <cstring>
#include <ctime>
#include <mutex>
#include <string>

#ifndef _WIN32
#  include <arpa/inet.h>
#  include <netinet/in.h>
#  include <sys/socket.h>
#  include <sys/time.h>
#  include <unistd.h>
#else
#  include <process.h>
#  include <winsock2.h>
#  include <ws2tcpip.h>
#endif

namespace uda { namespace authentication {

// ---------------------------------------------------------------------------
// Log file state

static FILE*      g_log = nullptr;
static std::mutex g_mutex;

void open_refusal_log(const char* logdir, const char* logmode)
{
    if (logdir == nullptr || logdir[0] == '\0') return;
    std::string path = std::string(logdir) + "refused_requests.log";
    const char* mode = (logmode != nullptr && logmode[0] != '\0') ? logmode : "w";
    std::lock_guard<std::mutex> lk(g_mutex);
    if (g_log != nullptr) fclose(g_log);
    g_log = fopen(path.c_str(), mode);
}

void close_refusal_log()
{
    std::lock_guard<std::mutex> lk(g_mutex);
    if (g_log != nullptr) {
        fclose(g_log);
        g_log = nullptr;
    }
}

// ---------------------------------------------------------------------------
// Peer info extraction

PeerInfo get_peer_info(int fd) noexcept
{
    PeerInfo pi;
#ifndef _WIN32
    struct sockaddr_storage addr = {};
    socklen_t addrlen = sizeof(addr);
    if (getpeername(fd, reinterpret_cast<struct sockaddr*>(&addr), &addrlen) != 0) {
        return pi;
    }
    char buf[INET6_ADDRSTRLEN] = {};
    if (addr.ss_family == AF_INET) {
        auto* a4 = reinterpret_cast<struct sockaddr_in*>(&addr);
        inet_ntop(AF_INET, &a4->sin_addr, buf, sizeof(buf));
        pi.port = static_cast<int>(ntohs(a4->sin_port));
    } else if (addr.ss_family == AF_INET6) {
        auto* a6 = reinterpret_cast<struct sockaddr_in6*>(&addr);
        inet_ntop(AF_INET6, &a6->sin6_addr, buf, sizeof(buf));
        pi.port = static_cast<int>(ntohs(a6->sin6_port));
    }
    pi.ip = buf;
#else
    (void)fd;
#endif
    return pi;
}

// ---------------------------------------------------------------------------
// Stable string conversions (keep in sync with reason_code values in the header)

static const char* stage_str(RefusalStage s) noexcept
{
    switch (s) {
        case RefusalStage::TlsConfig:    return "tls_config";
        case RefusalStage::TlsHandshake: return "tls_handshake";
        case RefusalStage::ClientBlock:  return "client_block";
        case RefusalStage::OidcAuth:     return "oidc_auth";
        case RefusalStage::Protocol:     return "protocol";
    }
    return "unknown";
}

static const char* reason_code_str(RefusalReason r) noexcept
{
    switch (r) {
        case RefusalReason::TlsServerCertConfigError: return "TLS_SERVER_CERT_CONFIG_ERROR";
        case RefusalReason::TlsCrlLoadFailed:         return "TLS_CRL_LOAD_FAILED";
        case RefusalReason::TlsClientCertExpired:     return "TLS_CLIENT_CERT_EXPIRED";
        case RefusalReason::TlsClientCertNotYetValid: return "TLS_CLIENT_CERT_NOT_YET_VALID";
        case RefusalReason::TlsClientCertUntrusted:   return "TLS_CLIENT_CERT_UNTRUSTED";
        case RefusalReason::TlsClientCertRevoked:     return "TLS_CLIENT_CERT_REVOKED";
        case RefusalReason::TlsClientCertMissing:     return "TLS_CLIENT_CERT_MISSING";
        case RefusalReason::TlsHandshakeFailed:       return "TLS_HANDSHAKE_FAILED";
        case RefusalReason::OidcTokenMissing:         return "OIDC_TOKEN_MISSING";
        case RefusalReason::OidcTokenExpired:         return "OIDC_TOKEN_EXPIRED";
        case RefusalReason::OidcTokenInvalidSignature:return "OIDC_TOKEN_INVALID_SIGNATURE";
        case RefusalReason::OidcTokenBadIssuer:       return "OIDC_TOKEN_BAD_ISSUER";
        case RefusalReason::OidcTokenBadAudience:     return "OIDC_TOKEN_BAD_AUDIENCE";
        case RefusalReason::OidcClaimPolicyFailed:    return "OIDC_CLAIM_POLICY_FAILED";
        case RefusalReason::OidcConfigInvalid:        return "OIDC_CONFIG_INVALID";
        case RefusalReason::ClientProtocolTooOld:     return "CLIENT_PROTOCOL_TOO_OLD";
        case RefusalReason::ClientBlockMalformed:     return "CLIENT_BLOCK_MALFORMED";
    }
    return "UNKNOWN";
}

// ---------------------------------------------------------------------------
// Minimal JSON builder — no external dependencies, no allocator surprises.

namespace {

std::string json_escape(const std::string& s)
{
    std::string out;
    out.reserve(s.size() + 4);
    for (unsigned char c : s) {
        if      (c == '"')  { out += "\\\""; }
        else if (c == '\\') { out += "\\\\"; }
        else if (c == '\n') { out += "\\n";  }
        else if (c == '\r') { out += "\\r";  }
        else if (c == '\t') { out += "\\t";  }
        else if (c < 0x20)  { /* strip other control chars */ }
        else                { out += static_cast<char>(c); }
    }
    return out;
}

struct JsonObj {
    std::string buf;
    bool first = true;

    JsonObj() { buf.reserve(512); buf += '{'; }

    void comma() { if (!first) buf += ','; first = false; }

    JsonObj& s(const char* key, const std::string& val) {
        if (val.empty()) return *this;
        comma();
        buf += '"'; buf += key; buf += "\":\"";
        buf += json_escape(val);
        buf += '"';
        return *this;
    }

    JsonObj& s(const char* key, const char* val) {
        if (val == nullptr || val[0] == '\0') return *this;
        return s(key, std::string(val));
    }

    // Always emit this integer field.
    JsonObj& n(const char* key, long val) {
        comma();
        buf += '"'; buf += key; buf += "\":";
        buf += std::to_string(val);
        return *this;
    }

    // Emit only when val differs from sentinel.
    JsonObj& n_if(const char* key, long val, long sentinel = 0) {
        if (val == sentinel) return *this;
        return n(key, val);
    }

    std::string finish() { buf += '}'; return buf; }
};

std::string iso8601_utc_ms()
{
#ifndef _WIN32
    struct timeval tv = {};
    gettimeofday(&tv, nullptr);
    struct tm* tm_info = gmtime(&tv.tv_sec);
    char tbuf[28] = {};
    strftime(tbuf, sizeof(tbuf), "%Y-%m-%dT%H:%M:%S", tm_info);
    char full[36];
    snprintf(full, sizeof(full), "%s.%03dZ", tbuf, static_cast<int>(tv.tv_usec / 1000));
    return full;
#else
    return "";
#endif
}

} // anonymous namespace

// ---------------------------------------------------------------------------
// Record writer

void record_refused_request(const RefusalRecord& rec)
{
#ifndef _WIN32
    const long pid = static_cast<long>(getpid());
#else
    const long pid = static_cast<long>(_getpid());
#endif

    std::string line =
        JsonObj{}
            .s("ts",           iso8601_utc_ms())
            .s("event",        "refused_request")
            .s("stage",        stage_str(rec.stage))
            .s("outcome",      "refused")
            .s("reason_code",  reason_code_str(rec.reason))
            .n_if("uda_error_code", rec.uda_error_code)
            .s("message",      rec.message)
            .s("peer_ip",      rec.peer.ip)
            .n_if("peer_port", rec.peer.port)
            .n("server_pid",   pid)
            // Common client fields
            .s("client_username",  rec.client_username)
            .n_if("client_version", static_cast<long>(rec.client_version))
            // TLS fields — only present on TLS stages
            .s("tls_mode",     rec.tls.tls_mode)
            .s("tls_version",  rec.tls.tls_version)
            .s("tls_cipher",   rec.tls.tls_cipher)
            .n_if("client_cert_verify_result",
                  rec.tls.client_cert_verify_result, /* sentinel= */ -1)
            .s("client_cert_subject",            rec.tls.client_cert_subject)
            .s("client_cert_issuer",             rec.tls.client_cert_issuer)
            .s("client_cert_serial",             rec.tls.client_cert_serial)
            .s("client_cert_not_before",         rec.tls.client_cert_not_before)
            .s("client_cert_not_after",          rec.tls.client_cert_not_after)
            .s("client_cert_fingerprint_sha256", rec.tls.client_cert_fingerprint_sha256)
            // OIDC fields — only present on OidcAuth stage
            .s("token_error",    rec.token_error)
            .s("oidc_issuer",    rec.oidc_issuer)
            .s("oidc_sub_sha256",rec.oidc_sub_sha256)
            // Protocol / client block detail
            .s("decode_error",   rec.decode_error)
            .finish();

    std::lock_guard<std::mutex> lk(g_mutex);
    if (g_log == nullptr) return;
    fputs(line.c_str(), g_log);
    fputc('\n', g_log);
    fflush(g_log);
}

}} // namespace uda::authentication

#endif // SSLAUTHENTICATION || OIDCAUTHENTICATION
