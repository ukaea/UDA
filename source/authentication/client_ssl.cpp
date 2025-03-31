#if defined(SSLAUTHENTICATION) && !defined(SERVERBUILD) && !defined(FATCLIENT)

#  include "client_ssl.h"

#  include <cstdio>
#  include <fcntl.h>
#  include <openssl/ssl.h>
#  include <ctime>

#  include "update_select_params.h"
#  include "clientserver/socket_structs.h"
#  include "clientserver/error_log.h"
#  include "logging/logging.h"
#  include "client2/connection.hpp"

using namespace uda::client_server;
using namespace uda::client;
using namespace uda::authentication;
using namespace uda::logging;

static bool g_sslDisabled = true; // Default state is not SSL authentication
static int g_sslProtocol = 0;     // The default server host name has the SSL protocol name prefix or
static int g_sslSocket = -1;
static bool g_sslOK = false;   // SSL Authentication has been passed sucessfully: default is NOT Passed
static bool g_sslInit = false; // Global initialisation of SSL completed
static SSL* g_ssl = nullptr;
static SSL_CTX* g_ctx = nullptr;
static const HostData* g_host = nullptr;

void uda::authentication::put_client_host(const HostData* host)
{
    g_host = host;
}

bool uda::authentication::get_client_ssl_disabled()
{
    return g_sslDisabled;
}

void uda::authentication::put_client_ssl_protocol(int specified)
{
    g_sslProtocol = specified;
}

void uda::authentication::put_client_ssl_socket(int s)
{
    g_sslSocket = s;
}

static void init_ssl_library()
{
    if (g_sslInit) {
        return; // Already initialised
    }
    if (getenv("UDA_SSL_INITIALISED")) {
        g_sslInit = true;
        UDA_LOG(UDA_LOG_DEBUG, "Prior SSL initialisation");
        return;
    }
    SSL_library_init();
    SSL_load_error_strings();
    OpenSSL_add_ssl_algorithms();
#  ifdef _WIN32
    if (getenv("UDA_SSL_INITIALISED") == nullptr) {
        _putenv_s("UDA_SSL_INITIALISED", "1");
    }
#  else
    setenv("UDA_SSL_INITIALISED", "1", 0);
#  endif
    g_sslInit = true;
    UDA_LOG(UDA_LOG_DEBUG, "SSL initialised");
}

void uda::authentication::close_client_ssl()
{
    // Requires re-initialisation
    if (g_sslDisabled) {
        return;
    }
    g_sslOK = false;
    g_sslSocket = -1;
    g_sslProtocol = 0;
    g_sslDisabled = true;
    SSL* ssl = g_ssl;
    if (ssl != nullptr) {
        SSL_shutdown(ssl);
        SSL_free(ssl);
    }
    SSL_CTX* ctx = g_ctx;
    if (ctx != nullptr) {
        SSL_CTX_free(ctx);
    }
    EVP_cleanup();
    g_ssl = nullptr;
    g_ctx = nullptr;
#  ifdef _WIN32
    _putenv_s("UDA_SSL_INITIALISED", nullptr);
#  else
    unsetenv("UDA_SSL_INITIALISED");
#  endif
    g_sslInit = false;
    UDA_LOG(UDA_LOG_DEBUG, "SSL closed");
}

SSL* uda::authentication::get_client_ssl()
{
    return g_ssl;
}

void put_client_ssl_ctx(SSL_CTX* c)
{
    g_ctx = c;
}

void report_ssl_error_code(std::vector<UdaError>& error_stack, int rc)
{
    const int err = SSL_get_error(get_client_ssl(), rc);
    std::string message;
    switch (err) {
        case SSL_ERROR_NONE:
            message = "SSL_ERROR_NONE";
            break;
        case SSL_ERROR_ZERO_RETURN:
            message = "SSL_ERROR_ZERO_RETURN";
            break;
        case SSL_ERROR_WANT_READ:
            message = "SSL_ERROR_WANT_READ";
            break;
        case SSL_ERROR_WANT_WRITE:
            message = "SSL_ERROR_WANT_WRITE";
            break;
        case SSL_ERROR_WANT_CONNECT:
            message = "SSL_ERROR_WANT_CONNECT";
            break;
        case SSL_ERROR_WANT_ACCEPT:
            message = "SSL_ERROR_WANT_ACCEPT";
            break;
        case SSL_ERROR_WANT_X509_LOOKUP:
            message = "SSL_ERROR_WANT_X509_LOOKUP";
            break;
        case SSL_ERROR_SYSCALL:
            message = "SSL_ERROR_SYSCALL";
            break;
        case SSL_ERROR_SSL:
            message = "SSL_ERROR_SSL";
            break;
        default:
            message = "Unknown SSL error";
            break;
    }
    UDA_ADD_ERROR(error_stack, 999, message.c_str());
    UDA_LOG(UDA_LOG_DEBUG, "Error - {}", message.c_str());
    UDA_LOG(UDA_LOG_DEBUG, "Error - {}", ERR_error_string(ERR_get_error(), nullptr));
    UDA_LOG(UDA_LOG_DEBUG, "State - {}", SSL_state_string(get_client_ssl()));
}

SSL_CTX* create_client_ssl_context(std::vector<UdaError>& error_stack)
{
    const SSL_METHOD* method = SSLv23_client_method(); // standard TCP

    // method = DTLSv1_client_method()// reliable UDP

    SSL_CTX* ctx = SSL_CTX_new(method);
    put_client_ssl_ctx(ctx);

    if (!ctx) {
        UDA_ADD_ERROR(error_stack, 999, "Unable to create SSL context");
        return nullptr;
    }

    // Disable SSLv2 for v3 and TSLv1  negotiation

    SSL_CTX_set_options(ctx, SSL_OP_NO_SSLv2);

    /*
    // Set the Cipher List
       if (SSL_CTX_set_cipher_list(g_ctx, "AES128-SHA") <= 0) {
          printf("Error setting the cipher list.\n");
          exit(0);
       }
    */

    UDA_LOG(UDA_LOG_DEBUG, "SSL Context created");

    return ctx;
}

int configure_client_ssl_context(const uda::config::Config& config, std::vector<UdaError>& error_stack, const HostData* host)
{
    // SSL_CTX_set_ecdh_auto(g_ctx, 1);

    // Set the key and cert - these take priority over entries in the host configuration file

    auto maybe_cert = config.get("ssl.cert");
    auto maybe_key = config.get("ssl.key");
    auto maybe_ca = config.get("ssl.ca");

    auto cert = maybe_cert.as_or_default<std::string>("");
    auto key = maybe_key.as_or_default<std::string>("");
    auto ca_cert = maybe_ca.as_or_default<std::string>("");

    if (!maybe_cert || !maybe_key || !maybe_ca) {
        // Check the client hosts configuration file
        if (host != nullptr) {
            // Socket connection was opened with a host entry in the configuration file
            if (!maybe_cert) {
                cert = host->certificate;
            }
            if (!maybe_key) {
                key = host->key;
            }
            if (!maybe_ca) {
                ca_cert = host->ca_certificate;
            }
            UDA_LOG(UDA_LOG_DEBUG,
                    "SSL certificates and private key obtained from the hosts configuration file\n");
        }
        if (cert.empty() || key.empty() || ca_cert.empty()) {
            if (cert.empty()) {
                UDA_LOG(UDA_LOG_DEBUG, "No Client SSL certificate");
                UDA_ADD_ERROR(error_stack, 999, "No client SSL certificate!");
            }
            if (key.empty()) {
                UDA_LOG(UDA_LOG_DEBUG, "No Client Private Key");
                UDA_ADD_ERROR(error_stack, 999, "No client SSL key!");
            }
            if (ca_cert.empty()) {
                UDA_LOG(UDA_LOG_DEBUG, "No CA SSL certificate");
                UDA_ADD_ERROR(error_stack, 999, "No Certificate Authority certificate!");
            }
            UDA_LOG(UDA_LOG_DEBUG, "Error: No SSL certificates and/or private key!");
            return 999;
        }
    }

    UDA_LOG(UDA_LOG_DEBUG, "Client SSL certificates: {}", cert);
    UDA_LOG(UDA_LOG_DEBUG, "Client SSL key: {}", key);
    UDA_LOG(UDA_LOG_DEBUG, "CA SSL certificates: {}", ca_cert);

    if (SSL_CTX_use_certificate_file(g_ctx, cert.c_str(), SSL_FILETYPE_PEM) <= 0) {
        UDA_LOG(UDA_LOG_DEBUG, "Error: Failed to set the client certificate!");
        UDA_THROW_ERROR(error_stack, 999, "Failed to set the client certificate!");
    }

    if (SSL_CTX_use_PrivateKey_file(g_ctx, key.c_str(), SSL_FILETYPE_PEM) <= 0) {
        UDA_LOG(UDA_LOG_DEBUG, "Error: Failed to set the client key!");
        UDA_THROW_ERROR(error_stack, 999, "Failed to set the client key!");
    }

    // Check key and certificate match
    if (SSL_CTX_check_private_key(g_ctx) == 0) {
        UDA_LOG(UDA_LOG_DEBUG, "Error: Private key does not match the certificate public key!");
        UDA_THROW_ERROR(error_stack, 999, "Private key does not match the certificate public key!");
    }

    // Load certificates of trusted CAs based on file provided
    if (SSL_CTX_load_verify_locations(g_ctx, ca_cert.c_str(), nullptr) < 1) {
        UDA_LOG(UDA_LOG_DEBUG, "Error: Error setting the certificate authority verify locations!");
        UDA_THROW_ERROR(error_stack, 999, "Error setting the certificate authority verify locations!");
    }

    // Peer certificate verification
    SSL_CTX_set_verify(g_ctx, SSL_VERIFY_PEER | SSL_VERIFY_FAIL_IF_NO_PEER_CERT, nullptr);
    SSL_CTX_set_verify_depth(g_ctx, VERIFY_DEPTH);

    UDA_LOG(UDA_LOG_DEBUG, "SSL context configured");

    // validate the client's certificate
    FILE* ca_file = fopen(ca_cert.c_str(), "r");

    if (ca_file == nullptr) {
        UDA_LOG(UDA_LOG_DEBUG, "Unable to open client certificate [{}] to verify certificate validity", cert);
        UDA_THROW_ERROR(error_stack, 999, "Unable to open client certificate to verify certificate validity!");
    }

    X509* client_cert = PEM_read_X509(ca_file, nullptr, nullptr, nullptr);

    fclose(ca_file);

    if (!client_cert) {
        X509_free(client_cert);
        UDA_LOG(UDA_LOG_DEBUG, "Unable to parse client certificate [{}] to verify certificate validity", cert);
        UDA_THROW_ERROR(error_stack, 999, "Unable to parse client certificate [{}] to verify certificate validity");
    }

    const ASN1_TIME* before = X509_get_notBefore(client_cert);
    const ASN1_TIME* after = X509_get_notAfter(client_cert);

    char work[X509STRINGSIZE];
    UDA_LOG(UDA_LOG_DEBUG, "Client X509 subject: {}",
            X509_NAME_oneline(X509_get_subject_name(client_cert), work, sizeof(work)));
    UDA_LOG(UDA_LOG_DEBUG, "Client X509 issuer: {}",
            X509_NAME_oneline(X509_get_issuer_name(client_cert), work, sizeof(work)));

    time_t current_time = time(nullptr);
    char* c_time_string = ctime(&current_time);

    int rc = 0, count = 0;
    BIO* b = BIO_new(BIO_s_mem());
    if (b && ASN1_TIME_print(b, before)) {
        count = BIO_read(b, work, X509STRINGSIZE - 1);
        BIO_free(b);
    }
    work[count] = '\0';
    UDA_LOG(UDA_LOG_DEBUG, "Client X509 not before: {}", work);
    if ((rc = X509_cmp_time(before, &current_time)) >= 0) {
        // Not Before is after Now!
        X509_free(client_cert);
        UDA_LOG(UDA_LOG_DEBUG, "Current Time               : {}", c_time_string);
        UDA_LOG(UDA_LOG_DEBUG, "Client X509 not before date is before the current date!");
        UDA_LOG(UDA_LOG_DEBUG, "The client SSL/x509 certificate is Not Valid - the Validity Date is in the future!");
        UDA_THROW_ERROR(error_stack, 999, "The client SSL/x509 certificate is Not Valid - the Validity Date is in the future");
    }

    count = 0;
    b = BIO_new(BIO_s_mem());
    if (b && ASN1_TIME_print(b, after)) {
        count = BIO_read(b, work, X509STRINGSIZE - 1);
        BIO_free(b);
    }
    work[count] = '\0';
    UDA_LOG(UDA_LOG_DEBUG, "Client X509 not after   : {}", work);
    if ((rc = X509_cmp_time(after, &current_time)) <= 0) { // Not After is before Now!
        X509_free(client_cert);
        UDA_LOG(UDA_LOG_DEBUG, "Current Time               : {}", c_time_string);
        UDA_LOG(UDA_LOG_DEBUG, "Client X509 not after date is after the current date!");
        UDA_LOG(UDA_LOG_DEBUG, "The client SSL/x509 certificate is Not Valid - the Date has Expired!");
        UDA_THROW_ERROR(error_stack, 999, "The client SSL/x509 certificate is Not Valid - the Date has Expired!");
    }
    X509_free(client_cert);

    UDA_LOG(UDA_LOG_DEBUG, "Current Time               : {}", c_time_string);
    UDA_LOG(UDA_LOG_DEBUG, "Client certificate date validity checked but not validated");

    return 0;
}

int uda::authentication::init_client_ssl(const config::Config& config, std::vector<UdaError>& error_stack)
{
    // Has SSL/TLS authentication already been passed?
    if (g_sslOK) {
        return 0;
    }

    // Has the user specified the SSL protocol on the host URL?
    // Has the user directly specified SSL/TLS authentication?
    // Does the connection entry in the client host configuration file have the three SSL authentication files

    if (!g_sslProtocol && !getenv("UDA_CLIENT_SSL_AUTHENTICATE")) {
        g_sslDisabled = true;

        if (g_host != nullptr) {
            // Socket connection was opened with a host entry in the configuration file
            if (!g_host->isSSL) {
                // 3 files are Not present or SSL:// not specified
                return 0;
            } else {
                g_sslDisabled = false;
            }
        } else {
            return 0;
        }
    } else {
        g_sslDisabled = false;
    }

    UDA_LOG(UDA_LOG_DEBUG, "SSL Authentication is Enabled!");

    // Initialise

    init_ssl_library();

    if (!(g_ctx = create_client_ssl_context(error_stack))) {
        UDA_THROW_ERROR(error_stack, 999, "Unable to create the SSL context!");
    }

    if (configure_client_ssl_context(config, error_stack, g_host) != 0) {
        UDA_THROW_ERROR(error_stack, 999, "Unable to configure the SSL context!");
    }

    return 0;
}

int uda::authentication::start_client_ssl(std::vector<UdaError>& error_stack)
{
    if (g_sslDisabled) {
        return 0;
    }

    // Bind an SSL object with the socket

    g_ssl = SSL_new(g_ctx);
    SSL_set_fd(g_ssl, g_sslSocket);

    // Connect to the server
    int rc;
    if ((rc = SSL_connect(g_ssl)) < 1) {
        UDA_LOG(UDA_LOG_DEBUG, "Error connecting to the server!");
        if (errno != 0) {
            UDA_ADD_SYS_ERROR(error_stack, "Error connecting to the server!");
        }
        report_ssl_error_code(error_stack, rc);
        return 999;
    }

    // Get the Server certificate and verify
    X509* peer = SSL_get_peer_certificate(g_ssl);

    if (peer != nullptr) {

        if ((rc = SSL_get_verify_result(g_ssl)) != X509_V_OK) {
            // returns X509_V_OK if the certificate was not obtained as no error occurred!
            UDA_ADD_ERROR(error_stack, 999, X509_verify_cert_error_string(rc));
            X509_free(peer);
            UDA_LOG(UDA_LOG_DEBUG, "SSL Server certificate presented but verification error!");
            UDA_THROW_ERROR(error_stack, 999, "SSL Server certificate presented but verification error!");
        }

        // Server's details - not required apart from logging

        char work[X509STRINGSIZE];
        UDA_LOG(UDA_LOG_DEBUG, "Server certificate verified");
        UDA_LOG(UDA_LOG_DEBUG, "X509 subject: {}",
                X509_NAME_oneline(X509_get_subject_name(peer), work, sizeof(work)));
        UDA_LOG(UDA_LOG_DEBUG, "X509 issuer: {}", X509_NAME_oneline(X509_get_issuer_name(peer), work, sizeof(work)));

        // Verify Date validity

        const ASN1_TIME* before = X509_get_notBefore(peer);
        const ASN1_TIME* after = X509_get_notAfter(peer);

        time_t current_time = time(nullptr);
        char* c_time_string = ctime(&current_time);

        int count = 0;
        BIO* b = BIO_new(BIO_s_mem());
        if (b && ASN1_TIME_print(b, before)) {
            count = BIO_read(b, work, X509STRINGSIZE - 1);
            BIO_free(b);
        }
        work[count] = '\0';
        UDA_LOG(UDA_LOG_DEBUG, "Server X509 not before: {}", work);
        if ((rc = X509_cmp_time(before, &current_time)) >= 0) { // Not Before is after Now!
            X509_free(peer);
            UDA_LOG(UDA_LOG_DEBUG, "Current Time               : {}", c_time_string);
            UDA_LOG(UDA_LOG_DEBUG, "Server X509 not before date is before the current date!");
            UDA_LOG(UDA_LOG_DEBUG,
                    "The Server's SSL/x509 certificate is Not Valid - the Validity Date is in the future!\n");
            UDA_THROW_ERROR(error_stack, 999, "The Server's SSL/x509 certificate is Not Valid - the Validity Date is in the future");
        }

        count = 0;
        b = BIO_new(BIO_s_mem());
        if (b && ASN1_TIME_print(b, after)) {
            count = BIO_read(b, work, X509STRINGSIZE - 1);
            BIO_free(b);
        }
        work[count] = '\0';
        UDA_LOG(UDA_LOG_DEBUG, "Server X509 not after   : {}", work);
        if ((rc = X509_cmp_time(after, &current_time)) <= 0) { // Not After is before Now!
            X509_free(peer);
            UDA_LOG(UDA_LOG_DEBUG, "Current Time               : {}", c_time_string);
            UDA_LOG(UDA_LOG_DEBUG, "Server X509 not after date is after the current date!");
            UDA_LOG(UDA_LOG_DEBUG, "The Server's SSL/x509 certificate is Not Valid - the Date has Expired!");
            UDA_THROW_ERROR(error_stack, 999, "The Server's SSL/x509 certificate is Not Valid - the Date has Expired!");
        }

        UDA_LOG(UDA_LOG_DEBUG, "Current Time               : {}", c_time_string);

        X509_free(peer);

    } else {
        X509_free(peer);
        UDA_LOG(UDA_LOG_DEBUG, "Server certificate not presented for verification!");
        UDA_THROW_ERROR(error_stack, 999, "Server certificate not presented for verification!");
    }

    // Print out connection details

    UDA_LOG(UDA_LOG_DEBUG, "SSL version: {}", SSL_get_version(g_ssl));
    UDA_LOG(UDA_LOG_DEBUG, "SSL cipher: {}", SSL_get_cipher(g_ssl));

    // SSL/TLS authentication has been passed - do not repeat

    g_sslOK = true;

    return 0;
}

int uda::authentication::write_client_ssl(void* io_handle, const char* buf, const int count)
{
    // This routine is only called when there is something to write to the Server
    // SSL uses an all or nothing approach when the socket is blocking - an SSL error or incomplete write
    // means the write has failed

    int rc, err = 0;

    fd_set wfds; // File Descriptor Set for Writing to the Socket
    timeval tv;

    // Block till it's possible to write to the socket or timeout

    const auto* io_data = static_cast<client::IoData*>(io_handle);

    update_select_params(g_sslSocket, &wfds, &tv);

    while ((rc = select(g_sslSocket + 1, nullptr, &wfds, nullptr, &tv)) <= 0) {
        if (rc < 0) { // Error
            if (errno == EBADF) {
                UDA_LOG(UDA_LOG_DEBUG, "Socket is closed! Data access failed!.");
            } else {
                UDA_LOG(UDA_LOG_DEBUG, "Read error - {}", strerror(errno));
            }
            return -1;
        }

#  ifndef _WIN32
        int fopts = 0;
        if ((rc = fcntl(g_sslSocket, F_GETFL, &fopts)) < 0 || errno == EBADF) {
            // Is the socket closed? Check status flags
            err = 999;
            UDA_LOG(UDA_LOG_DEBUG, "Socket is closed!");
            return -1;
        }
#  endif

        update_select_params(g_sslSocket, &wfds, &tv);
    }

    // set SSL_MODE_AUTO_RETRY flag of the SSL_CTX_set_mode to disable automatic renegotiation?

    rc = SSL_write(get_client_ssl(), buf, count);

    switch (SSL_get_error(get_client_ssl(), rc)) {
        case SSL_ERROR_NONE:
            if (rc != count) { // Check the write is complete
                err = 999;
                UDA_LOG(UDA_LOG_DEBUG, "Incomplete write to socket!");
                add_error(*io_data->error_stack, ErrorType::Code, "writeUdaClientSSL", err, "Incomplete write to socket!");
                return -1;
            }
            break;

        default:
            report_ssl_error_code(*io_data->error_stack, rc);
            err = 999;
            UDA_LOG(UDA_LOG_DEBUG, "Write to socket failed!");
            add_error(*io_data->error_stack, ErrorType::Code, "writeUdaClientSSL", err, "Write to socket failed!");
#  ifndef _WIN32
            int fopts = 0;
            if ((rc = fcntl(g_sslSocket, F_GETFL, &fopts)) < 0 ||
                errno == EBADF) { // Is the socket closed? Check status flags
                UDA_LOG(UDA_LOG_DEBUG, "Socket is closed!");
            }
#  endif
            return -1;
    }

    return rc;
}

int uda::authentication::read_client_ssl(void* io_handle, char* buf, int count)
{
    int rc, err = 0;
    fd_set rfds;
    timeval tv;

    int maxloop = 0;

    // Wait till it's possible to read from socket

    const auto* io_data = static_cast<client::IoData*>(io_handle);

    update_select_params(g_sslSocket, &rfds, &tv);

    while (((rc = select(g_sslSocket + 1, &rfds, nullptr, nullptr, &tv)) <= 0) && maxloop++ < MaxLoop) {

        if (rc < 0) { // Error
            int serrno = errno;
            add_error(*io_data->error_stack, ErrorType::System, "readUdaClientSSL", errno, "Socket is Closed!");
            if (serrno == EBADF) {
                UDA_LOG(UDA_LOG_DEBUG, "Socket is closed!");
            } else {
                UDA_LOG(UDA_LOG_DEBUG, "Read error - {}", strerror(serrno));
            }
            err = 999;
            add_error(*io_data->error_stack, ErrorType::Code, "readUdaClientSSL", err,
                      "Socket is Closed! Data request failed. Restarting connection.");
            UDA_LOG(UDA_LOG_DEBUG, "Socket is Closed! Data request failed. Restarting connection.");
            return -1;
        }
#  ifndef _WIN32
        int fopts = 0;
        if ((rc = fcntl(g_sslSocket, F_GETFL, &fopts)) < 0 ||
            errno == EBADF) { // Is the socket closed? Check status flags
            err = 999;
            UDA_LOG(UDA_LOG_DEBUG, "Socket is closed!");
            return -1;
        }
#  endif

        update_select_params(g_sslSocket, &rfds, &tv); // Keep blocking and wait for data
    }

    // First byte of encrypted data received but need the full record in buffer before SSL can decrypt

    int blocked;
    do {
        blocked = 0;
        rc = SSL_read(get_client_ssl(), buf, count);

        switch (SSL_get_error(get_client_ssl(), rc)) { // check for SSL errors
            case SSL_ERROR_NONE:                        // clean read
                break;

            case SSL_ERROR_ZERO_RETURN: // connection closed by server (not caught by select?)
                report_ssl_error_code(*io_data->error_stack, rc);
                err = 999;
                UDA_LOG(UDA_LOG_DEBUG, "Server socket connection closed!");
                add_error(*io_data->error_stack, ErrorType::Code, "readUdaClientSSL", err, "Server socket connection closed!");
                return -1;

            case SSL_ERROR_WANT_READ: // the operation did not complete, try again
                blocked = 1;
                break;

            case SSL_ERROR_WANT_WRITE: // the operation did not complete, error
                report_ssl_error_code(*io_data->error_stack, rc);
                err = 999;
                UDA_LOG(UDA_LOG_DEBUG, "A read operation failed!");
                add_error(*io_data->error_stack, ErrorType::Code, "readUdaClientSSL", err, "A read operation failed!");
                return -1;

            case SSL_ERROR_SYSCALL: // some I/O error occured - disconnect?
                report_ssl_error_code(*io_data->error_stack, rc);
                err = 999;
                UDA_LOG(UDA_LOG_DEBUG, "Socket read I/O error!");
                add_error(*io_data->error_stack, ErrorType::Code, "readUdaClientSSL", err, "Socket read I/O error!");
                return -1;

            default: // some other error
                report_ssl_error_code(*io_data->error_stack, rc);
                err = 999;
                UDA_LOG(UDA_LOG_DEBUG, "Read from socket failed!");
                add_error(*io_data->error_stack, ErrorType::Code, "readUdaClientSSL", err, "Read from socket failed!");
#  ifndef _WIN32
                int fopts = 0;
                if ((rc = fcntl(g_sslSocket, F_GETFL, &fopts)) < 0 ||
                    errno == EBADF) { // Is the socket closed? Check status flags
                    UDA_LOG(UDA_LOG_DEBUG, "Socket is closed!");
                }
#  endif
                return -1;
        }

    } while (SSL_pending(get_client_ssl()) && !blocked); // data remaining in buffer or re-read attempt

    return rc;
}

#endif // !SERVERBUILD && SSLAUTHENTICATION
