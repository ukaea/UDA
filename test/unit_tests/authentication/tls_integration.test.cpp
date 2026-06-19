// TLS integration tests using in-memory OpenSSL BIO pairs.
// Tests perform real TLS handshakes between paired client and server SSL_CTX objects
// without any network — the BIO pair acts as a synchronous in-memory pipe.
//
// To add scenarios requiring specific server cert properties (IP SAN, wildcard, etc.)
// extend generate_server_cert() with the appropriate X509 extension calls.

#include <catch2/catch_test_macros.hpp>

#include <chrono>
#include <cstring>
#include <ctime>
#include <memory>
#include <stdexcept>
#include <string>
#include <vector>

#include <openssl/asn1.h>
#include <openssl/bio.h>
#include <openssl/err.h>
#include <openssl/evp.h>
#include <openssl/pem.h>
#include <openssl/ssl.h>
#include <openssl/x509.h>
#include <openssl/x509v3.h>

// -------------------------------------------------------------------------
// In-memory cert/key generation helpers

namespace {

struct EvpPkeyDeleter { void operator()(EVP_PKEY* p) const { if (p) EVP_PKEY_free(p); } };
struct X509Deleter    { void operator()(X509* p)     const { if (p) X509_free(p); } };
struct SslCtxDeleter  { void operator()(SSL_CTX* p)  const { if (p) SSL_CTX_free(p); } };
struct SslDeleter     { void operator()(SSL* p)      const { if (p) SSL_free(p); } };
struct BioDeleter     { void operator()(BIO* p)      const { if (p) BIO_free_all(p); } };

using EvpPkeyPtr = std::unique_ptr<EVP_PKEY, EvpPkeyDeleter>;
using X509Ptr    = std::unique_ptr<X509,     X509Deleter>;
using SslCtxPtr  = std::unique_ptr<SSL_CTX,  SslCtxDeleter>;
using SslPtr     = std::unique_ptr<SSL,      SslDeleter>;
using BioPtr     = std::unique_ptr<BIO,      BioDeleter>;

std::string openssl_last_error()
{
    char buf[256];
    ERR_error_string_n(ERR_get_error(), buf, sizeof(buf));
    return buf;
}

EvpPkeyPtr generate_key()
{
    EVP_PKEY_CTX* ctx = EVP_PKEY_CTX_new_id(EVP_PKEY_RSA, nullptr);
    EVP_PKEY_keygen_init(ctx);
    EVP_PKEY_CTX_set_rsa_keygen_bits(ctx, 2048);
    EVP_PKEY* pkey = nullptr;
    if (EVP_PKEY_keygen(ctx, &pkey) != 1) {
        EVP_PKEY_CTX_free(ctx);
        throw std::runtime_error("RSA key generation failed: " + openssl_last_error());
    }
    EVP_PKEY_CTX_free(ctx);
    return EvpPkeyPtr(pkey);
}

// Build and sign an X509 certificate.
// san_dns: set to a hostname to add a DNS SAN (for server certs).
// san_ip:  set to an IP address string to add an IP SAN.
// valid_days: use negative values to produce an already-expired cert.
X509Ptr make_cert(EVP_PKEY* subject_key,
                  EVP_PKEY* signing_key,
                  X509*     issuer_cert,  // nullptr for self-signed
                  const char* cn,
                  int valid_days     = 365,
                  const char* san_dns = nullptr,
                  const char* san_ip  = nullptr)
{
    X509Ptr cert(X509_new());
    X509_set_version(cert.get(), 2); // X509v3

    // Serial number
    ASN1_INTEGER_set(X509_get_serialNumber(cert.get()),
                     static_cast<long>(std::chrono::system_clock::now()
                         .time_since_epoch().count() & 0x7FFFFFFF));

    // Validity
    if (valid_days >= 0) {
        X509_gmtime_adj(X509_get_notBefore(cert.get()), 0);
        X509_gmtime_adj(X509_get_notAfter(cert.get()),
                        static_cast<long>(valid_days) * 24 * 60 * 60);
    } else {
        // Expired: set both start and end in the past
        X509_gmtime_adj(X509_get_notBefore(cert.get()),
                        static_cast<long>(valid_days - 1) * 24 * 60 * 60);
        X509_gmtime_adj(X509_get_notAfter(cert.get()),
                        static_cast<long>(valid_days) * 24 * 60 * 60);
    }

    X509_set_pubkey(cert.get(), subject_key);

    // Subject CN
    X509_NAME* name = X509_get_subject_name(cert.get());
    X509_NAME_add_entry_by_txt(name, "CN", MBSTRING_ASC,
        reinterpret_cast<const unsigned char*>(cn), -1, -1, 0);

    // Issuer
    if (issuer_cert) {
        X509_set_issuer_name(cert.get(), X509_get_subject_name(issuer_cert));
    } else {
        X509_set_issuer_name(cert.get(), name); // self-signed
    }

    // Basic constraints: CA:TRUE for CA certs
    if (!issuer_cert) {
        X509_EXTENSION* ex = X509V3_EXT_conf_nid(nullptr, nullptr,
            NID_basic_constraints, "critical,CA:TRUE");
        if (ex) { X509_add_ext(cert.get(), ex, -1); X509_EXTENSION_free(ex); }
    }

    // Subject Alternative Name
    if (san_dns || san_ip) {
        std::string san_str;
        if (san_dns) san_str += std::string("DNS:") + san_dns;
        if (san_ip)  { if (!san_str.empty()) san_str += ","; san_str += std::string("IP:") + san_ip; }
        X509_EXTENSION* ex = X509V3_EXT_conf_nid(nullptr, nullptr,
            NID_subject_alt_name, san_str.c_str());
        if (ex) { X509_add_ext(cert.get(), ex, -1); X509_EXTENSION_free(ex); }
    }

    if (X509_sign(cert.get(), signing_key, EVP_sha256()) == 0) {
        throw std::runtime_error("X509_sign failed: " + openssl_last_error());
    }
    return cert;
}

// Write X509 to PEM string
std::string cert_to_pem(X509* cert)
{
    BioPtr bio(BIO_new(BIO_s_mem()));
    PEM_write_bio_X509(bio.get(), cert);
    char* data;
    long len = BIO_get_mem_data(bio.get(), &data);
    return std::string(data, static_cast<size_t>(len));
}

std::string key_to_pem(EVP_PKEY* key)
{
    BioPtr bio(BIO_new(BIO_s_mem()));
    PEM_write_bio_PrivateKey(bio.get(), key, nullptr, nullptr, 0, nullptr, nullptr);
    char* data;
    long len = BIO_get_mem_data(bio.get(), &data);
    return std::string(data, static_cast<size_t>(len));
}

// Build client SSL_CTX that trusts the given CA cert PEM
SslCtxPtr make_client_ctx(const std::string& ca_pem,
                           bool verify_peer = true)
{
    SslCtxPtr ctx(SSL_CTX_new(TLS_client_method()));
    if (!ctx) throw std::runtime_error("SSL_CTX_new (client) failed");

    BioPtr bio(BIO_new_mem_buf(ca_pem.data(), static_cast<int>(ca_pem.size())));
    X509Ptr ca_x509(PEM_read_bio_X509(bio.get(), nullptr, nullptr, nullptr));
    X509_STORE* store = SSL_CTX_get_cert_store(ctx.get());
    X509_STORE_add_cert(store, ca_x509.get());

    SSL_CTX_set_verify(ctx.get(), verify_peer ? SSL_VERIFY_PEER : SSL_VERIFY_NONE, nullptr);
    return ctx;
}

// Build server SSL_CTX with the given cert/key PEM; optionally require client cert
SslCtxPtr make_server_ctx(const std::string& cert_pem,
                           const std::string& key_pem,
                           const std::string& ca_pem    = "",
                           bool  require_client_cert    = false)
{
    SslCtxPtr ctx(SSL_CTX_new(TLS_server_method()));
    if (!ctx) throw std::runtime_error("SSL_CTX_new (server) failed");

    // Load cert from memory
    BioPtr cbio(BIO_new_mem_buf(cert_pem.data(), static_cast<int>(cert_pem.size())));
    X509Ptr cert(PEM_read_bio_X509(cbio.get(), nullptr, nullptr, nullptr));
    SSL_CTX_use_certificate(ctx.get(), cert.get());

    // Load key from memory
    BioPtr kbio(BIO_new_mem_buf(key_pem.data(), static_cast<int>(key_pem.size())));
    EVP_PKEY* pkey = PEM_read_bio_PrivateKey(kbio.get(), nullptr, nullptr, nullptr);
    SSL_CTX_use_PrivateKey(ctx.get(), pkey);
    EVP_PKEY_free(pkey);

    if (!ca_pem.empty()) {
        BioPtr cabio(BIO_new_mem_buf(ca_pem.data(), static_cast<int>(ca_pem.size())));
        X509Ptr ca_cert(PEM_read_bio_X509(cabio.get(), nullptr, nullptr, nullptr));
        X509_STORE* store = SSL_CTX_get_cert_store(ctx.get());
        X509_STORE_add_cert(store, ca_cert.get());
    }

    if (require_client_cert) {
        SSL_CTX_set_verify(ctx.get(),
            SSL_VERIFY_PEER | SSL_VERIFY_FAIL_IF_NO_PEER_CERT, nullptr);
    }
    return ctx;
}

// Perform an in-memory TLS handshake between client_ctx and server_ctx.
// Returns {client_result, server_result} — both should be 1 on success.
// Optionally configure client hostname for SNI/verification before SSL_connect.
std::pair<int, int> do_handshake(SSL_CTX* client_ctx,
                                  SSL_CTX* server_ctx,
                                  const char* client_hostname = nullptr,
                                  bool set_verify_hostname    = false)
{
    // Create BIO pairs: client ↔ server
    BIO* client_bio_internal;
    BIO* client_bio_network;
    BIO_new_bio_pair(&client_bio_internal, 0, &client_bio_network, 0);

    BIO* server_bio_internal;
    BIO* server_bio_network;
    BIO_new_bio_pair(&server_bio_internal, 0, &server_bio_network, 0);

    // Cross-connect: client reads/writes through server_bio_network and vice versa
    SslPtr client_ssl(SSL_new(client_ctx));
    SslPtr server_ssl(SSL_new(server_ctx));

    SSL_set_bio(client_ssl.get(), server_bio_network, client_bio_internal);
    SSL_set_bio(server_ssl.get(), client_bio_network, server_bio_internal);

    if (client_hostname) {
        SSL_set_tlsext_host_name(client_ssl.get(), client_hostname); // SNI
        if (set_verify_hostname) {
            SSL_set_hostflags(client_ssl.get(), X509_CHECK_FLAG_NO_PARTIAL_WILDCARDS);
            SSL_set1_host(client_ssl.get(), client_hostname);
        }
    }

    // Drive the handshake by alternating SSL_connect / SSL_accept calls
    // until both complete or one fails fatally.
    int client_result = -1;
    int server_result = -1;
    for (int i = 0; i < 100; ++i) {
        if (client_result <= 0) client_result = SSL_connect(client_ssl.get());
        if (server_result <= 0) server_result = SSL_accept(server_ssl.get());
        if (client_result == 1 && server_result == 1) break;

        const int c_err = SSL_get_error(client_ssl.get(), client_result);
        const int s_err = SSL_get_error(server_ssl.get(), server_result);
        if (c_err != SSL_ERROR_WANT_READ && c_err != SSL_ERROR_WANT_WRITE &&
            s_err != SSL_ERROR_WANT_READ && s_err != SSL_ERROR_WANT_WRITE) {
            break;
        }
    }

    return {client_result, server_result};
}

} // namespace

// -------------------------------------------------------------------------
// Test fixtures — generate CA and server cert once per binary

namespace {

struct TlsFixture {
    EvpPkeyPtr ca_key;
    X509Ptr    ca_cert;
    std::string ca_pem;

    EvpPkeyPtr server_key;
    X509Ptr    server_cert;
    std::string server_cert_pem;
    std::string server_key_pem;

    EvpPkeyPtr client_key;
    X509Ptr    client_cert;
    std::string client_cert_pem;
    std::string client_key_pem;

    TlsFixture()
    {
        SSL_library_init();
        SSL_load_error_strings();

        ca_key  = generate_key();
        ca_cert = make_cert(ca_key.get(), ca_key.get(), nullptr, "Test CA");
        ca_pem  = cert_to_pem(ca_cert.get());

        server_key      = generate_key();
        server_cert     = make_cert(server_key.get(), ca_key.get(), ca_cert.get(),
                                    "test-server.example.com",
                                    365,
                                    "test-server.example.com"); // DNS SAN
        server_cert_pem = cert_to_pem(server_cert.get());
        server_key_pem  = key_to_pem(server_key.get());

        client_key      = generate_key();
        client_cert     = make_cert(client_key.get(), ca_key.get(), ca_cert.get(), "Test Client");
        client_cert_pem = cert_to_pem(client_cert.get());
        client_key_pem  = key_to_pem(client_key.get());
    }
};

const TlsFixture& fixture()
{
    static const TlsFixture f;
    return f;
}

} // namespace

// -------------------------------------------------------------------------
// Tests

TEST_CASE("TLS server-only: valid cert, matching hostname — handshake succeeds", "[tls_integration]")
{
    const auto& f  = fixture();
    SslCtxPtr c_ctx = make_client_ctx(f.ca_pem, true);
    SslCtxPtr s_ctx = make_server_ctx(f.server_cert_pem, f.server_key_pem);

    const auto [c_res, s_res] = do_handshake(c_ctx.get(), s_ctx.get(),
                                              "test-server.example.com", true);
    REQUIRE( c_res == 1 );
    REQUIRE( s_res == 1 );
}

TEST_CASE("TLS server-only: hostname mismatch — client rejects handshake", "[tls_integration]")
{
    const auto& f  = fixture();
    SslCtxPtr c_ctx = make_client_ctx(f.ca_pem, true);
    SslCtxPtr s_ctx = make_server_ctx(f.server_cert_pem, f.server_key_pem);

    // Connect to "wrong-host.example.com" but cert only has SAN for test-server.example.com
    const auto [c_res, s_res] = do_handshake(c_ctx.get(), s_ctx.get(),
                                              "wrong-host.example.com", true);
    REQUIRE( c_res != 1 ); // client must reject
}

TEST_CASE("TLS server-only: no hostname set — cert trust still checked", "[tls_integration]")
{
    const auto& f  = fixture();
    SslCtxPtr c_ctx = make_client_ctx(f.ca_pem, true);
    SslCtxPtr s_ctx = make_server_ctx(f.server_cert_pem, f.server_key_pem);

    // No hostname set at all (no SNI, no hostname check) — should succeed because the
    // CA is trusted and OpenSSL does not enforce hostname matching unless SSL_set1_host is called.
    const auto [c_res, s_res] = do_handshake(c_ctx.get(), s_ctx.get(),
                                              nullptr, false);
    REQUIRE( c_res == 1 );
    REQUIRE( s_res == 1 );
}

TEST_CASE("TLS server-only: untrusted CA — client rejects handshake", "[tls_integration]")
{
    const auto& f = fixture();

    // Client trusts a different CA (freshly generated for this test)
    auto other_ca_key  = generate_key();
    auto other_ca_cert = make_cert(other_ca_key.get(), other_ca_key.get(), nullptr, "Other CA");
    const std::string other_ca_pem = cert_to_pem(other_ca_cert.get());

    SslCtxPtr c_ctx = make_client_ctx(other_ca_pem, true); // trusts OTHER CA
    SslCtxPtr s_ctx = make_server_ctx(f.server_cert_pem, f.server_key_pem);

    const auto [c_res, s_res] = do_handshake(c_ctx.get(), s_ctx.get(),
                                              "test-server.example.com", true);
    REQUIRE( c_res != 1 ); // client must reject (cert signed by test CA, not other CA)
}

TEST_CASE("TLS server-only: expired server cert — client rejects handshake", "[tls_integration]")
{
    const auto& f = fixture();

    // Generate a cert that expired a day ago
    auto exp_key  = generate_key();
    auto exp_cert = make_cert(exp_key.get(), f.ca_key.get(), f.ca_cert.get(),
                               "test-server.example.com",
                               -1, // expired yesterday
                               "test-server.example.com");
    const std::string exp_cert_pem = cert_to_pem(exp_cert.get());
    const std::string exp_key_pem  = key_to_pem(exp_key.get());

    SslCtxPtr c_ctx = make_client_ctx(f.ca_pem, true);
    SslCtxPtr s_ctx = make_server_ctx(exp_cert_pem, exp_key_pem);

    const auto [c_res, s_res] = do_handshake(c_ctx.get(), s_ctx.get(),
                                              "test-server.example.com", true);
    REQUIRE( c_res != 1 ); // client must reject expired cert
}

TEST_CASE("TLS server-only: IP SAN — client matches IP address", "[tls_integration]")
{
    const auto& f = fixture();

    auto ip_key  = generate_key();
    auto ip_cert = make_cert(ip_key.get(), f.ca_key.get(), f.ca_cert.get(),
                              "127.0.0.1",
                              365,
                              nullptr,  // no DNS SAN
                              "127.0.0.1"); // IP SAN
    const std::string ip_cert_pem = cert_to_pem(ip_cert.get());
    const std::string ip_key_pem  = key_to_pem(ip_key.get());

    SslCtxPtr c_ctx = make_client_ctx(f.ca_pem, true);
    SslCtxPtr s_ctx = make_server_ctx(ip_cert_pem, ip_key_pem);

    const auto [c_res, s_res] = do_handshake(c_ctx.get(), s_ctx.get(),
                                              "127.0.0.1", true);
    REQUIRE( c_res == 1 );
    REQUIRE( s_res == 1 );
}

TEST_CASE("Mutual TLS: both certs valid — handshake succeeds", "[tls_integration]")
{
    const auto& f = fixture();

    SslCtxPtr c_ctx = make_client_ctx(f.ca_pem, true);

    // Load client cert/key into client_ctx
    BioPtr ccbio(BIO_new_mem_buf(f.client_cert_pem.data(), static_cast<int>(f.client_cert_pem.size())));
    X509Ptr cc(PEM_read_bio_X509(ccbio.get(), nullptr, nullptr, nullptr));
    SSL_CTX_use_certificate(c_ctx.get(), cc.get());
    BioPtr ckbio(BIO_new_mem_buf(f.client_key_pem.data(), static_cast<int>(f.client_key_pem.size())));
    EVP_PKEY* ck = PEM_read_bio_PrivateKey(ckbio.get(), nullptr, nullptr, nullptr);
    SSL_CTX_use_PrivateKey(c_ctx.get(), ck);
    EVP_PKEY_free(ck);

    // Server requires client cert; trusts our test CA
    SslCtxPtr s_ctx = make_server_ctx(f.server_cert_pem, f.server_key_pem, f.ca_pem, true);

    const auto [c_res, s_res] = do_handshake(c_ctx.get(), s_ctx.get(),
                                              "test-server.example.com", true);
    REQUIRE( c_res == 1 );
    REQUIRE( s_res == 1 );
}

TEST_CASE("Mutual TLS: no client cert — server rejects", "[tls_integration]")
{
    const auto& f = fixture();

    // Client has no client cert loaded
    SslCtxPtr c_ctx = make_client_ctx(f.ca_pem, true);
    // Server requires client cert
    SslCtxPtr s_ctx = make_server_ctx(f.server_cert_pem, f.server_key_pem, f.ca_pem, true);

    const auto [c_res, s_res] = do_handshake(c_ctx.get(), s_ctx.get(),
                                              "test-server.example.com", true);
    REQUIRE( s_res != 1 ); // server must reject (no client cert)
}

TEST_CASE("Mutual TLS: invalid (self-signed) client cert — server rejects", "[tls_integration]")
{
    const auto& f = fixture();

    // Client uses a self-signed cert (not signed by the CA the server trusts)
    auto rogue_key  = generate_key();
    auto rogue_cert = make_cert(rogue_key.get(), rogue_key.get(), nullptr, "Rogue Client");
    const std::string rogue_cert_pem = cert_to_pem(rogue_cert.get());
    const std::string rogue_key_pem  = key_to_pem(rogue_key.get());

    SslCtxPtr c_ctx = make_client_ctx(f.ca_pem, true);
    BioPtr rcbio(BIO_new_mem_buf(rogue_cert_pem.data(), static_cast<int>(rogue_cert_pem.size())));
    X509Ptr rc(PEM_read_bio_X509(rcbio.get(), nullptr, nullptr, nullptr));
    SSL_CTX_use_certificate(c_ctx.get(), rc.get());
    BioPtr rkbio(BIO_new_mem_buf(rogue_key_pem.data(), static_cast<int>(rogue_key_pem.size())));
    EVP_PKEY* rk = PEM_read_bio_PrivateKey(rkbio.get(), nullptr, nullptr, nullptr);
    SSL_CTX_use_PrivateKey(c_ctx.get(), rk);
    EVP_PKEY_free(rk);

    SslCtxPtr s_ctx = make_server_ctx(f.server_cert_pem, f.server_key_pem, f.ca_pem, true);

    const auto [c_res, s_res] = do_handshake(c_ctx.get(), s_ctx.get(),
                                              "test-server.example.com", false);
    REQUIRE( s_res != 1 ); // server must reject rogue cert
}

TEST_CASE("Mutual TLS: expired client cert — server rejects", "[tls_integration]")
{
    const auto& f = fixture();
    // Generate a client cert that expired yesterday
    auto exp_client_key  = generate_key();
    auto exp_client_cert = make_cert(exp_client_key.get(), f.ca_key.get(), f.ca_cert.get(),
                                     "Expired Client", -1); // expired
    const std::string exp_cert_pem = cert_to_pem(exp_client_cert.get());
    const std::string exp_key_pem  = key_to_pem(exp_client_key.get());

    SslCtxPtr c_ctx = make_client_ctx(f.ca_pem, true);
    BioPtr ccbio(BIO_new_mem_buf(exp_cert_pem.data(), static_cast<int>(exp_cert_pem.size())));
    X509Ptr cc(PEM_read_bio_X509(ccbio.get(), nullptr, nullptr, nullptr));
    SSL_CTX_use_certificate(c_ctx.get(), cc.get());
    BioPtr ckbio(BIO_new_mem_buf(exp_key_pem.data(), static_cast<int>(exp_key_pem.size())));
    EVP_PKEY* ck = PEM_read_bio_PrivateKey(ckbio.get(), nullptr, nullptr, nullptr);
    SSL_CTX_use_PrivateKey(c_ctx.get(), ck);
    EVP_PKEY_free(ck);

    SslCtxPtr s_ctx = make_server_ctx(f.server_cert_pem, f.server_key_pem, f.ca_pem, true);
    const auto [c_res, s_res] = do_handshake(c_ctx.get(), s_ctx.get(),
                                              "test-server.example.com", true);
    REQUIRE( s_res != 1 ); // server must reject expired client cert
}

TEST_CASE("TLS server-only: IP SAN mismatch — client rejects wrong IP", "[tls_integration]")
{
    const auto& f = fixture();
    auto ip_key  = generate_key();
    auto ip_cert = make_cert(ip_key.get(), f.ca_key.get(), f.ca_cert.get(),
                              "127.0.0.1", 365, nullptr, "127.0.0.1");
    const std::string ip_cert_pem = cert_to_pem(ip_cert.get());
    const std::string ip_key_pem  = key_to_pem(ip_key.get());

    SslCtxPtr c_ctx = make_client_ctx(f.ca_pem, true);
    SslCtxPtr s_ctx = make_server_ctx(ip_cert_pem, ip_key_pem);

    // Cert has SAN for 127.0.0.1, but client verifies against 192.0.2.1
    const auto [c_res, s_res] = do_handshake(c_ctx.get(), s_ctx.get(), "192.0.2.1", true);
    REQUIRE( c_res != 1 ); // client must reject: cert IP SAN does not match
}

TEST_CASE("TLS server-only: wildcard DNS SAN matches subdomain", "[tls_integration]")
{
    const auto& f = fixture();
    auto wc_key  = generate_key();
    auto wc_cert = make_cert(wc_key.get(), f.ca_key.get(), f.ca_cert.get(),
                              "*.example.com", 365, "*.example.com");
    const std::string wc_cert_pem = cert_to_pem(wc_cert.get());
    const std::string wc_key_pem  = key_to_pem(wc_key.get());

    SslCtxPtr c_ctx = make_client_ctx(f.ca_pem, true);
    SslCtxPtr s_ctx = make_server_ctx(wc_cert_pem, wc_key_pem);

    const auto [c_res, s_res] = do_handshake(c_ctx.get(), s_ctx.get(), "host.example.com", true);
    REQUIRE( c_res == 1 );
    REQUIRE( s_res == 1 );
}

TEST_CASE("TLS server-only: partial wildcard rejected with NO_PARTIAL_WILDCARDS flag", "[tls_integration]")
{
    const auto& f = fixture();
    auto wc_key  = generate_key();
    auto wc_cert = make_cert(wc_key.get(), f.ca_key.get(), f.ca_cert.get(),
                              "*.example.com", 365, "*.example.com");
    const std::string wc_cert_pem = cert_to_pem(wc_cert.get());
    const std::string wc_key_pem  = key_to_pem(wc_key.get());

    SslCtxPtr c_ctx = make_client_ctx(f.ca_pem, true);
    SslCtxPtr s_ctx = make_server_ctx(wc_cert_pem, wc_key_pem);

    // X509_CHECK_FLAG_NO_PARTIAL_WILDCARDS means *.example.com does NOT match
    // deep.sub.example.com (only single-label wildcards are allowed)
    const auto [c_res, s_res] = do_handshake(c_ctx.get(), s_ctx.get(),
                                              "deep.sub.example.com", true);
    REQUIRE( c_res != 1 ); // wildcard *.example.com should not match deep.sub.example.com
}
