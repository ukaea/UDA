// Tests for refused_requests.log — the record schema is an integration contract with
// whatever consumes the audit trail, so it is tested as one.

#include <catch2/catch_test_macros.hpp>

#include <authentication/refusal_log.h>

#include <nlohmann/json.hpp>

#include <cstdio>
#include <fstream>
#include <string>

using namespace uda::authentication;
using json = nlohmann::json;

namespace {

RefusalRecord minimal_record()
{
    RefusalRecord rec;
    rec.stage          = RefusalStage::OidcAuth;
    rec.reason         = RefusalReason::OidcTokenExpired;
    rec.uda_error_code = 706;
    rec.message        = "token expired";
    return rec;
}

json parse_record(const RefusalRecord& rec)
{
    return json::parse(format_refusal_record(rec));
}

} // namespace

TEST_CASE("a record is valid JSON with the always-present fields", "[refusal_log]")
{
    const auto j = parse_record(minimal_record());

    REQUIRE(j["event"]   == "refused_request");
    REQUIRE(j["outcome"] == "refused");
    REQUIRE(j["stage"]   == "oidc_auth");
    REQUIRE(j["reason_code"] == "OIDC_TOKEN_EXPIRED");
    REQUIRE(j["uda_error_code"] == 706);
    REQUIRE(j["message"] == "token expired");
    REQUIRE(j.contains("ts"));
    REQUIRE(j.contains("server_pid"));
}

TEST_CASE("the timestamp is ISO 8601 UTC with milliseconds", "[refusal_log]")
{
    const auto j  = parse_record(minimal_record());
    const auto ts = j["ts"].get<std::string>();

    REQUIRE(ts.size() == 24);       // YYYY-MM-DDTHH:MM:SS.mmmZ
    REQUIRE(ts[4]  == '-');
    REQUIRE(ts[10] == 'T');
    REQUIRE(ts[19] == '.');
    REQUIRE(ts.back() == 'Z');
}

TEST_CASE("fields that do not apply to a stage are omitted", "[refusal_log]")
{
    const auto j = parse_record(minimal_record());

    // An OIDC refusal carries no TLS certificate fields at all — not empty strings.
    REQUIRE_FALSE(j.contains("client_cert_subject"));
    REQUIRE_FALSE(j.contains("tls_version"));
    REQUIRE_FALSE(j.contains("peer_ip"));
    REQUIRE_FALSE(j.contains("decode_error"));
    REQUIRE_FALSE(j.contains("oidc_issuer"));
}

TEST_CASE("client_cert_verify_result uses -1 as its omit sentinel", "[refusal_log]")
{
    RefusalRecord rec = minimal_record();
    REQUIRE_FALSE(parse_record(rec).contains("client_cert_verify_result"));

    // X509_V_OK is 0, which must still be emitted — 0 is a real value for this field.
    rec.tls.client_cert_verify_result = 0;
    REQUIRE(parse_record(rec)["client_cert_verify_result"] == 0);

    rec.tls.client_cert_verify_result = 10;
    REQUIRE(parse_record(rec)["client_cert_verify_result"] == 10);
}

TEST_CASE("a TLS record carries its certificate fields", "[refusal_log]")
{
    RefusalRecord rec;
    rec.stage                     = RefusalStage::TlsHandshake;
    rec.reason                    = RefusalReason::TlsClientCertExpired;
    rec.uda_error_code            = 711;
    rec.message                   = "client certificate verification failed";
    rec.peer.ip                   = "192.0.2.10";
    rec.peer.port                 = 41234;
    rec.tls.tls_mode              = "mutual";
    rec.tls.tls_version           = "TLSv1.3";
    rec.tls.client_cert_subject   = "/CN=alice";
    rec.tls.client_cert_not_after = "2020-01-01T00:00:00Z";

    const auto j = parse_record(rec);
    REQUIRE(j["stage"]       == "tls_handshake");
    REQUIRE(j["reason_code"] == "TLS_CLIENT_CERT_EXPIRED");
    REQUIRE(j["peer_ip"]     == "192.0.2.10");
    REQUIRE(j["peer_port"]   == 41234);
    REQUIRE(j["tls_mode"]    == "mutual");
    REQUIRE(j["client_cert_subject"]   == "/CN=alice");
    REQUIRE(j["client_cert_not_after"] == "2020-01-01T00:00:00Z");
}

TEST_CASE("hostile certificate content cannot break the record", "[refusal_log]")
{
    // Certificate subjects come off the wire, so they are attacker-controlled. A quote,
    // a backslash, a newline or a control byte must not be able to forge a second record
    // or produce a line that fails to parse.
    RefusalRecord rec = minimal_record();
    rec.stage  = RefusalStage::TlsHandshake;
    rec.tls.client_cert_subject =
        "/CN=evil\",\"reason_code\":\"TLS_CLIENT_CERT_UNTRUSTED\nfake\t\\end\x01";

    const std::string line = format_refusal_record(rec);

    // One line: the embedded newline must have been escaped, not emitted raw.
    REQUIRE(line.find('\n') == std::string::npos);

    const auto j = json::parse(line); // must still parse
    REQUIRE(j["reason_code"] == "OIDC_TOKEN_EXPIRED"); // not the injected value
    REQUIRE(j["client_cert_subject"].get<std::string>().find("evil") != std::string::npos);
}

TEST_CASE("every stage and reason has a stable wire spelling", "[refusal_log]")
{
    // A reason code with no spelling would silently log as "UNKNOWN" and break any
    // downstream rule keyed on it.
    const RefusalStage stages[] = {
        RefusalStage::TlsConfig, RefusalStage::TlsHandshake, RefusalStage::ClientBlock,
        RefusalStage::OidcAuth,  RefusalStage::Protocol,
    };
    for (const auto s : stages) {
        REQUIRE(std::string(stage_str(s)) != "unknown");
    }

    const RefusalReason reasons[] = {
        RefusalReason::TlsServerCertConfigError, RefusalReason::TlsCrlLoadFailed,
        RefusalReason::TlsClientCertExpired,     RefusalReason::TlsClientCertNotYetValid,
        RefusalReason::TlsClientCertUntrusted,   RefusalReason::TlsClientCertRevoked,
        RefusalReason::TlsClientCertMissing,     RefusalReason::TlsHandshakeFailed,
        RefusalReason::OidcTokenMissing,         RefusalReason::OidcTokenExpired,
        RefusalReason::OidcTokenInvalidSignature, RefusalReason::OidcTokenBadIssuer,
        RefusalReason::OidcTokenBadAudience,     RefusalReason::OidcClaimPolicyFailed,
        RefusalReason::OidcConfigInvalid,        RefusalReason::ClientProtocolTooOld,
        RefusalReason::ClientBlockMalformed,
    };
    for (const auto r : reasons) {
        REQUIRE(std::string(reason_code_str(r)) != "UNKNOWN");
    }
}

TEST_CASE("recording without an open log is a no-op, not a crash", "[refusal_log]")
{
    close_refusal_log();
    REQUIRE_NOTHROW(record_refused_request(minimal_record()));
}

TEST_CASE("the log appends rather than truncating", "[refusal_log]")
{
    // The server forks per connection, so a truncating open would let each new connection
    // erase the previous one's audit records. Re-opening must preserve what is there.
    const std::string dir  = "./";
    const std::string path = dir + "refused_requests.log";
    std::remove(path.c_str());

    open_refusal_log(dir.c_str());
    record_refused_request(minimal_record());
    close_refusal_log();

    // A second server process opening the same log.
    open_refusal_log(dir.c_str());
    record_refused_request(minimal_record());
    close_refusal_log();

    std::ifstream in(path);
    int lines = 0;
    std::string line;
    while (std::getline(in, line)) {
        if (!line.empty()) {
            REQUIRE_NOTHROW(json::parse(line));
            ++lines;
        }
    }
    in.close();
    std::remove(path.c_str());

    REQUIRE(lines == 2);
}
