#include "refusal_log.h"

#if (defined(SSLAUTHENTICATION) || defined(OIDCAUTHENTICATION)) && !defined(FATCLIENT)

#include <cerrno>
#include <cstdio>
#include <cstring>
#include <ctime>
#include <mutex>
#include <string>

#include <nlohmann/json.hpp>

#ifndef _WIN32
#  include <arpa/inet.h>
#  include <fcntl.h>
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
// Log file state.
//
// A file descriptor rather than a FILE*, so that each record goes out as one
// write(2) on an O_APPEND descriptor. That is what makes concurrent appends from
// independent server processes safe: the kernel serialises the seek-to-end and the
// write, and a single sub-PIPE_BUF write cannot be split by another process's write.

static int        g_fd = -1;
static std::mutex g_mutex;

void open_refusal_log(const char* logdir)
{
    if (logdir == nullptr || logdir[0] == '\0') {
        return;
    }
    const std::string path = std::string(logdir) + "refused_requests.log";

    std::lock_guard<std::mutex> lk(g_mutex);
    if (g_fd >= 0) {
        close(g_fd);
    }
    // Always O_APPEND: this is an audit trail, and the server forks per connection.
    // Opening it for truncation would let each new connection erase its predecessors.
    g_fd = open(path.c_str(), O_WRONLY | O_CREAT | O_APPEND, 0640);
}

void close_refusal_log()
{
    std::lock_guard<std::mutex> lk(g_mutex);
    if (g_fd >= 0) {
        close(g_fd);
        g_fd = -1;
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
// Stable string conversions — these are the wire vocabulary. Changing a spelling
// here breaks downstream SIEM rules; add new values rather than renaming old ones.

const char* stage_str(RefusalStage s) noexcept
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

const char* reason_code_str(RefusalReason r) noexcept
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
// Record serialisation

namespace {

// ordered_json, not json: the field order below is part of the log's readability
// contract, and nlohmann's default json object sorts keys alphabetically.
using ordered_json = nlohmann::ordered_json;

// Optional-field helpers. Empty strings and sentinel numbers are omitted entirely
// rather than emitted as "" or 0, so a record carries only the fields that apply
// to its stage.
void put(ordered_json& j, const char* key, const std::string& val)
{
    if (!val.empty()) {
        j[key] = val;
    }
}

void put_if(ordered_json& j, const char* key, long val, long sentinel = 0)
{
    if (val != sentinel) {
        j[key] = val;
    }
}

// Note: this server does not run on Windows, so the POSIX-only timestamp path is
// the only one that matters. gmtime() rather than gmtime_r() is safe here because
// the server is fork-per-connection and single-threaded.
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
    return "unknown";
#endif
}

long current_pid()
{
#ifndef _WIN32
    return static_cast<long>(getpid());
#else
    return static_cast<long>(_getpid());
#endif
}

} // anonymous namespace

std::string format_refusal_record(const RefusalRecord& rec)
{
    ordered_json j;

    j["ts"]          = iso8601_utc_ms();
    j["event"]       = "refused_request";
    j["stage"]       = stage_str(rec.stage);
    j["outcome"]     = "refused";
    j["reason_code"] = reason_code_str(rec.reason);
    put_if(j, "uda_error_code", rec.uda_error_code);
    put(j, "message", rec.message);
    put(j, "peer_ip", rec.peer.ip);
    put_if(j, "peer_port", rec.peer.port);
    j["server_pid"] = current_pid();

    // Common client fields
    put(j, "client_username", rec.client_username);
    put_if(j, "client_version", rec.client_version);

    // TLS fields — only present on TLS stages
    put(j, "tls_mode",    rec.tls.tls_mode);
    put(j, "tls_version", rec.tls.tls_version);
    put(j, "tls_cipher",  rec.tls.tls_cipher);
    put_if(j, "client_cert_verify_result", rec.tls.client_cert_verify_result, /* sentinel= */ -1);
    put(j, "client_cert_subject",            rec.tls.client_cert_subject);
    put(j, "client_cert_issuer",             rec.tls.client_cert_issuer);
    put(j, "client_cert_serial",             rec.tls.client_cert_serial);
    put(j, "client_cert_not_before",         rec.tls.client_cert_not_before);
    put(j, "client_cert_not_after",          rec.tls.client_cert_not_after);
    put(j, "client_cert_fingerprint_sha256", rec.tls.client_cert_fingerprint_sha256);

    // OIDC fields — only present on OidcAuth stage
    put(j, "token_error",     rec.token_error);
    put(j, "oidc_issuer",     rec.oidc_issuer);
    put(j, "oidc_sub_sha256", rec.oidc_sub_sha256);

    // Protocol / client block detail
    put(j, "decode_error", rec.decode_error);

    return j.dump();
}

// ---------------------------------------------------------------------------
// Record writer

void record_refused_request(const RefusalRecord& rec)
{
    std::lock_guard<std::mutex> lk(g_mutex);
    if (g_fd < 0) {
        return;
    }

    std::string line = format_refusal_record(rec);
    line += '\n';

    // One write, newline included — see the note on g_fd.
    const char* p    = line.c_str();
    size_t      left = line.size();
    while (left > 0) {
        const ssize_t n = write(g_fd, p, left);
        if (n <= 0) {
            if (errno == EINTR) {
                continue;
            }
            return; // the audit log must never take the server down
        }
        p    += n;
        left -= static_cast<size_t>(n);
    }
}

}} // namespace uda::authentication

#endif // (SSLAUTHENTICATION || OIDCAUTHENTICATION) && !FATCLIENT
