#if defined(SSLAUTHENTICATION)

#  include "server_ssl.h"

#  include <fcntl.h>
#  include <string>
#  include <openssl/asn1.h>
#  include <openssl/crypto.h>
#  include <openssl/err.h>
#  include <openssl/pem.h>
#  include <openssl/ssl.h>
#  include <openssl/x509.h>

#  include "clientserver/error_log.h"
#  include "logging/logging.h"
#  include "protocol/connection.hpp"

#  include "server/writer.h"

#  define VERIFY_DEPTH 4
#  define X509STRINGSIZE 256

using namespace uda::client_server;
using namespace uda::server;
using namespace uda::logging;

/*
Note on initialisation:
UDA Servers using plugins that connect to other UDA servers through the standard client API library
need to block initialisation of the SSL library by the client. Initialisation must be done once only.
As all information passed between plugins and servers is through the interface structure, the state
of initialisation by the server must be passed within the interface. This state information is local
and should not be passed to subsequent servers. An alternative and simpler mechanism is for the server
to assign a value to an environment variable, and for the client to test this environment variable.
*/

static bool g_sslDisabled = true; // Default state is not SSL authentication
static int g_sslSocket = -1;
static bool g_sslOK = false;   // SSL Authentication has been passed successfully: default is NOT Passed
static bool g_sslInit = false; // Global initialisation of SSL completed
static SSL* g_ssl = nullptr;
static SSL_CTX* g_ctx = nullptr;

static void init_server_ssl();
static SSL_CTX* create_server_ssl_context(std::vector<UdaError>& error_stack);
static int configure_server_ssl_context(const uda::config::Config& config, std::vector<UdaError>& error_stack);
static X509_CRL* load_server_ssl_crl(std::vector<UdaError>& error_stack, const char* crlist);
static int add_server_ssl_crls_store(X509_STORE* st, STACK_OF(X509_CRL) * crls);

void uda::authentication::put_server_ssl_socket(const int socket)
{
    g_sslSocket = socket;
}

bool uda::authentication::get_server_ssl_disabled()
{
    return g_sslDisabled;
}

void report_server_ssl_error_code(std::vector<UdaError>& error_stack, const int rc)
{
    int err = SSL_get_error(g_ssl, rc);
    char msg[256];
    switch (err) {
        case SSL_ERROR_NONE:
            strcpy(msg, "SSL_ERROR_NONE");
            break;
        case SSL_ERROR_ZERO_RETURN:
            strcpy(msg, "SSL_ERROR_ZERO_RETURN");
            break;
        case SSL_ERROR_WANT_READ:
            strcpy(msg, "SSL_ERROR_WANT_READ");
            break;
        case SSL_ERROR_WANT_WRITE:
            strcpy(msg, "SSL_ERROR_WANT_WRITE");
            break;
        case SSL_ERROR_WANT_CONNECT:
            strcpy(msg, "SSL_ERROR_WANT_CONNECT");
            break;
        case SSL_ERROR_WANT_ACCEPT:
            strcpy(msg, "SSL_ERROR_WANT_ACCEPT");
            break;
        case SSL_ERROR_WANT_X509_LOOKUP:
            strcpy(msg, "SSL_ERROR_WANT_X509_LOOKUP");
            break;
        case SSL_ERROR_SYSCALL:
            strcpy(msg, "SSL_ERROR_SYSCALL");
            break;
        case SSL_ERROR_SSL:
            strcpy(msg, "SSL_ERROR_SSL");
            break;
    }
    err = 999;
    add_error(error_stack, ErrorType::Code, "udaSSL", err, msg);
    UDA_LOG(UDA_LOG_DEBUG, "Error - {}", msg);
    UDA_LOG(UDA_LOG_DEBUG, "Error - {}", ERR_error_string(ERR_get_error(), nullptr));
    UDA_LOG(UDA_LOG_DEBUG, "State - {}", SSL_state_string(g_ssl));
}

void init_server_ssl()
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
    _putenv_s("UDA_SSL_INITIALISED", "1");
#  else
    // Ensure the library is not re-initialised by the UDA client library
    setenv("UDA_SSL_INITIALISED", "1", 0);
#  endif
    g_sslInit = true;
    UDA_LOG(UDA_LOG_DEBUG, "SSL initialised");
}

void uda::authentication::close_server_ssl()
{
    // Requires re-initialisation (should only be called once at closedown!)
    if (g_sslDisabled) {
        return;
    }
    g_sslOK = false;
    g_sslSocket = -1;
    g_sslDisabled = true;
    if (g_ssl != nullptr) {
        SSL_shutdown(g_ssl);
        SSL_free(g_ssl);
    }
    if (g_ctx != nullptr) {
        SSL_CTX_free(g_ctx);
    }
    EVP_cleanup();
    g_ssl = nullptr;
    g_ctx = nullptr;
#  ifdef _WIN32
    _putenv_s("UDA_SSL_INITIALISED", NULL);
#  else
    unsetenv("UDA_SSL_INITIALISED");
#  endif
    g_sslInit = false;
    UDA_LOG(UDA_LOG_DEBUG, "SSL closed");
}

SSL_CTX* create_server_ssl_context(std::vector<UdaError>& error_stack)
{
    const SSL_METHOD* method = SSLv23_server_method(); // standard TCP

    // method = DTLSv1_server_method()        // reliable UDP

    g_ctx = SSL_CTX_new(method);

    if (g_ctx == nullptr) {
        UDA_LOG(UDA_LOG_DEBUG, "Unable to create SSL context!");
        UDA_ADD_ERROR(error_stack, 999, "Unable to create SSL context");
        return nullptr;
    }

    // Disable SSLv2 for v3 and TSLv1  negotiation
    SSL_CTX_set_options(g_ctx, SSL_OP_NO_SSLv2);

    /*
    // Set the Cipher List
       if (SSL_CTX_set_cipher_list(g_ctx, "AES128-SHA") <= 0) {
          printf("Error setting the cipher list.\n");
          exit(0);
       }
    */

    UDA_LOG(UDA_LOG_DEBUG, "SSL Context created");

    return g_ctx;
}

int configure_server_ssl_context(const uda::config::Config& config, std::vector<UdaError>& error_stack)
{
    const auto maybe_cert = config.get("server.ssl_cert");
    const auto maybe_key = config.get("server.ssl_key");
    const auto maybe_ca = config.get("server.ssl_ca");
    const auto maybe_crl = config.get("server.ssl_crl");

    if (!maybe_cert || !maybe_key || !maybe_ca) {
        if (!maybe_cert) {
            UDA_ADD_ERROR(error_stack, 999, "No server SSL certificate!");
        }
        if (!maybe_key) {
            UDA_ADD_ERROR(error_stack, 999, "No server SSL key!");
        }
        if (!maybe_ca) {
            UDA_ADD_ERROR(error_stack, 999, "No Certificate Authority certificate!");
        }
        UDA_LOG(UDA_LOG_DEBUG, "Certificate/Key/CRL environment variable problem!");
        return 999;
    }

    const std::string cert = maybe_cert.as<std::string>();
    const std::string key = maybe_key.as<std::string>();
    const std::string ca = maybe_ca.as<std::string>();

    if (SSL_CTX_use_certificate_file(g_ctx, cert.c_str(), SSL_FILETYPE_PEM) <= 0) {
        UDA_THROW_ERROR(error_stack, 999, "Failed to set the server certificate!");
    }

    if (SSL_CTX_use_PrivateKey_file(g_ctx, key.c_str(), SSL_FILETYPE_PEM) <= 0) {
        UDA_THROW_ERROR(error_stack, 999, "Failed to set the server key!");
    }

    // Check key and certificate match
    if (SSL_CTX_check_private_key(g_ctx) == 0) {
        UDA_THROW_ERROR(error_stack, 999, "Private key does not match the certificate public key!");
    }

    // Load certificates of trusted CAs
    if (SSL_CTX_load_verify_locations(g_ctx, ca.c_str(), nullptr) < 1) {
        UDA_THROW_ERROR(error_stack, 999, "Error setting the Cetificate Authority verify locations!");
    }

    // Peer certificate verification
    SSL_CTX_set_verify(g_ctx, SSL_VERIFY_PEER | SSL_VERIFY_FAIL_IF_NO_PEER_CERT, nullptr);
    SSL_CTX_set_verify_depth(g_ctx, VERIFY_DEPTH);

    // Add verification against the Certificate Revocation List
    X509_VERIFY_PARAM* params = X509_VERIFY_PARAM_new();
    X509_VERIFY_PARAM_set_flags(params, X509_V_FLAG_CRL_CHECK);
    SSL_CTX_set1_param(g_ctx, params);

    if (maybe_crl) {
        const std::string crl = maybe_crl.as<std::string>();

        X509_CRL* ssl_crl = load_server_ssl_crl(error_stack, crl.c_str());
        if (!ssl_crl) {
            return 999; // CRL not loaded
        }

        STACK_OF(X509_CRL)* crls = sk_X509_CRL_new_null();
        if (!crls || !sk_X509_CRL_push(crls, ssl_crl)) {
            X509_CRL_free(ssl_crl);
            UDA_THROW_ERROR(error_stack, 999, "Error loading the CRL for client certificate verification!");
        }

        X509_STORE* st = SSL_CTX_get_cert_store(g_ctx);
        add_server_ssl_crls_store(st, crls);
        SSL_CTX_set1_verify_cert_store(g_ctx, st);
    }

    // Set CA list used for client authentication

    /*
      if(SSL_CTX_use_certificate_chain_file(g_ctx, getenv("UDA_SERVER_CA_SSL_CERT")) < 1){
         //printf("Error setting the CA chain file\n");
         exit(0);
      }
    */
    /*
      SSL_CTX_set_client_CA_list(g_ctx, SSL_load_client_CA_file(getenv("UDA_SERVER_CA_SSL_CERT")));

       rc = load_CA(g_ssl, g_ctx, getenv("UDA_SERVER_CA_SSL_CERT"));    // calls SSL_CTX_add_client_CA(g_ctx, X509
      *cacert) and         SSL_add_client_CA(g_ssl, X509 *cacert) if(rc == 0)fprintf(logout, "Unable to load Client
      CA!\n");
    */

    UDA_LOG(UDA_LOG_DEBUG, "SSL Context configured");

    return 0;
}

X509_CRL* load_server_ssl_crl(std::vector<UdaError>& error_stack, const char* crlist)
{
    // Load the Certificate Revocation Lists for certificate verification

    BIO* in = BIO_new(BIO_s_file());
    if (in == nullptr) {
        UDA_ADD_ERROR(error_stack, 999, "Error creating a Certificate Revocation List object!");
        return nullptr;
    }

    if (BIO_read_filename(in, crlist) <= 0) {
        BIO_free(in);
        UDA_ADD_ERROR(error_stack, 999, "Error opening the Certificate Revocation List file!");
        return nullptr;
    }

    X509_CRL* crl = PEM_read_bio_X509_CRL(in, nullptr, nullptr, nullptr);

    if (crl == nullptr) {
        BIO_free(in);
        UDA_ADD_ERROR(error_stack, 999, "Error reading the Certificate Revocation List file!");
        return nullptr;
    }

    BIO_free(in);

    UDA_LOG(UDA_LOG_DEBUG, "CRL loaded");

    return crl;
}

int add_server_ssl_crls_store(X509_STORE* st, STACK_OF(X509_CRL) * crls)
{
    X509_CRL* crl;
    for (int i = 0; i < sk_X509_CRL_num(crls); i++) {
        crl = sk_X509_CRL_value(crls, i);
        X509_STORE_add_crl(st, crl);
    }
    return 1;
}

std::string to_string(ASN1_TIME* time)
{
    std::string time_string;

    BIO* bio = BIO_new(BIO_s_mem());

    if (ASN1_TIME_print(bio, time)) {
        BUF_MEM* ptr;
        BIO_get_mem_ptr(bio, &ptr);
        time_string = { ptr->data, ptr->length };
    }
    BIO_free_all(bio);

    return time_string;
}

int uda::authentication::start_server_ssl(const config::Config& config, std::vector<UdaError>& error_stack)
{
    int rc;

    // Has SSL/TLS authentication already been passed?
    if (g_sslOK) {
        return 0;
    }

    const auto maybe_authenticate = config.get("server.ssl_authenticate");
    g_sslDisabled = !maybe_authenticate || !maybe_authenticate.as<bool>();

    // Has the server disabled SSL/TLS authentication?
    if (g_sslDisabled) {
        return 0;
    }

    UDA_LOG(UDA_LOG_DEBUG, "SSL Authentication is Enabled!");

    // Initialise
    init_server_ssl();

    if (!(g_ctx = create_server_ssl_context(error_stack))) {
        UDA_THROW_ERROR(error_stack, 999, "Unable to create the SSL context!");
    }
    if (configure_server_ssl_context(config, error_stack) != 0) {
        UDA_THROW_ERROR(error_stack, 999, "Unable to configure the SSL context!");
    }

    // Bind an SSL object with the socket
    g_ssl = SSL_new(g_ctx);
    if ((rc = SSL_set_fd(g_ssl, g_sslSocket)) < 1) {
        UDA_LOG(UDA_LOG_DEBUG, "Error - Unable to bind the socket to SSL!");
        UDA_THROW_ERROR(error_stack, 999, "Unable to bind the socket to SSL!");
    }

    // SSL Handshake with Client Authentication
    if ((rc = SSL_accept(g_ssl)) < 1) {
        if (errno != 0) {
            UDA_ADD_SYS_ERROR(error_stack, "SSL Handshake failed!");
        }
        UDA_LOG(UDA_LOG_DEBUG, "Error - SSL Handshake Failed!");
        int err = SSL_get_error(g_ssl, rc);
        if (err == 5) {
            UDA_LOG(UDA_LOG_DEBUG, "Error - Client application terminated?!");
            UDA_ADD_ERROR(error_stack, err, "SSL error in SSL_accept, application terminated!");
        }
        report_server_ssl_error_code(error_stack, rc);
        return err;
    }

    // Get the Client's certificate and verify
    X509* peer = SSL_get_peer_certificate(g_ssl);

    if (peer != nullptr) {
        if ((rc = SSL_get_verify_result(g_ssl)) != X509_V_OK) {
            // returns X509_V_OK if the certificate was not obtained as no error occured!
            X509_free(peer);
            UDA_LOG(UDA_LOG_DEBUG, "SSL Client certificate presented but verification error!");
            UDA_ADD_ERROR(error_stack, 999, "SSL Client certificate  presented but verification error!");
            UDA_THROW_ERROR(error_stack, 999, X509_verify_cert_error_string(rc));
        }

        // Client's details

        char work[X509STRINGSIZE];
        UDA_LOG(UDA_LOG_DEBUG, "Client certificate verified");
        UDA_LOG(UDA_LOG_DEBUG, "X509 subject: {}",
                X509_NAME_oneline(X509_get_subject_name(peer), work, sizeof(work)));
        UDA_LOG(UDA_LOG_DEBUG, "X509 issuer: {}", X509_NAME_oneline(X509_get_issuer_name(peer), work, sizeof(work)));

        ASN1_TIME* before = X509_get_notBefore(peer);
        ASN1_TIME* after = X509_get_notAfter(peer);

        std::string before_string = to_string(before);
        std::string after_string = to_string(after);

        UDA_LOG(UDA_LOG_DEBUG, "X509 not before: {}", before_string.c_str());
        UDA_LOG(UDA_LOG_DEBUG, "X509 not after: {}", after_string.c_str());
        X509_free(peer);
    } else {
        X509_free(peer);
        UDA_LOG(UDA_LOG_DEBUG, "Client certificate not presented for verification!");
        UDA_THROW_ERROR(error_stack, 999, "Client certificate not presented for verification!");
    }

    // Print out connection details

    UDA_LOG(UDA_LOG_DEBUG, "SSL version: {}s", SSL_get_version(g_ssl));
    UDA_LOG(UDA_LOG_DEBUG, "SSL cipher: {}", SSL_get_cipher(g_ssl));

    // SSL/TLS authentication has been passed - do not repeat

    g_sslOK = true;

    return 0;
}

#  ifdef UNUSED
#  elif defined(__GNUC__)
#    define UNUSED __attribute__((unused))
#  elif defined(__LCLINT__)
#    define UNUSED /*@unused@*/
#  else
#    define UNUSED
#  endif

int uda::authentication::write_server_ssl(void* iohandle, const char* buf, int count)
{
    // This routine is only called when there is something to write back to the Client
    // SSL uses an all or nothing approach when the socket is blocking - an SSL error or incomplete write
    // means the write has failed

    int rc;

    fd_set wfds; // File Descriptor Set for Writing to the Socket
    struct timeval tv = {};

    auto* io_data = reinterpret_cast<protocol::IoData*>(iohandle);

    // Block till it's possible to write to the socket or timeout

    protocol::set_select_params(g_sslSocket, &wfds, &tv, io_data->server_tot_block_time);

    while ((rc = select(g_sslSocket + 1, nullptr, &wfds, nullptr, &tv)) <= 0) {

        if (rc < 0) { // Error
            if (errno == EBADF) {
                UDA_LOG(UDA_LOG_DEBUG, "Client Socket is closed! Closing server down.");
            } else {
                UDA_LOG(UDA_LOG_DEBUG, "Read error - {}", strerror(errno));
                UDA_LOG(UDA_LOG_DEBUG, "Closing server down.");
            }
            return -1;
        }

#  ifndef _WIN32
        int fopts = 0;
        if (fcntl(g_sslSocket, F_GETFL, &fopts) < 0 || errno == EBADF) {
            // Is the socket closed? Check status flags
            UDA_LOG(UDA_LOG_DEBUG, "Client Socket is closed! Closing server down.");
            return -1;
        }
#  endif

        *io_data->server_tot_block_time += tv.tv_usec / 1000;

        if (*io_data->server_tot_block_time / 1000 > *io_data->server_timeout) {
            UDA_LOG(UDA_LOG_DEBUG, "Total Blocking Time: {} (ms). Closing server down.",
                    *io_data->server_tot_block_time);
            return -1; // Timeout
        }

        protocol::update_select_params(g_sslSocket, &wfds, &tv, *io_data->server_tot_block_time);
    }

    // set SSL_MODE_AUTO_RETRY flag of the SSL_CTX_set_mode to disable automatic renegotiation?

    rc = SSL_write(g_ssl, buf, count);

    switch (SSL_get_error(g_ssl, rc)) {
        case SSL_ERROR_NONE:
            if (rc != count) {
                // Check the write is complete
                UDA_LOG(UDA_LOG_DEBUG, "Incomplete write to socket!");
                UDA_ADD_ERROR(*io_data->error_stack, 999, "Incomplete write to socket!");
                return -1;
            }
            break;

        default:
            report_server_ssl_error_code(*io_data->error_stack, rc);
            UDA_LOG(UDA_LOG_DEBUG, "Write to socket failed!");
            UDA_ADD_ERROR(*io_data->error_stack, 999, "Write to socket failed!");
#  ifndef _WIN32
            int fopts = 0;
            if (fcntl(g_sslSocket, F_GETFL, &fopts) < 0 || errno == EBADF) {
                // Is the socket closed? Check status flags
                UDA_LOG(UDA_LOG_DEBUG, "Client Socket is closed! Closing server down.");
            }
#  endif
            return -1;
    }

    return rc;
}

int uda::authentication::read_server_ssl(void* iohandle, char* buf, int count)
{
    int rc;
    fd_set rfds; // File Descriptor Set for Reading from the Socket
    timeval tv, tvc;

    // Wait till it's possible to read from the socket
    // Set the blocking period before a timeout
    auto* io_data = reinterpret_cast<protocol::IoData*>(iohandle);

    protocol::set_select_params(g_sslSocket, &rfds, &tv, io_data->server_tot_block_time);
    tvc = tv;

    // TODO: Use pselect to include a signal mask to force a timeout

    while ((rc = select(g_sslSocket + 1, &rfds, nullptr, nullptr, &tvc)) <= 0) {

        if (rc < 0) { // Error
            if (errno == EBADF) {
                UDA_LOG(UDA_LOG_DEBUG, "Client Socket is closed! Closing server down.");
            } else {
                UDA_LOG(UDA_LOG_DEBUG, "Read error - {}", strerror(errno));
                UDA_LOG(UDA_LOG_DEBUG, "Closing server down.");
            }
            return -1;
        }

        *io_data->server_tot_block_time += (int)tv.tv_usec / 1000; // ms

        if (*io_data->server_tot_block_time > 1000 * *io_data->server_timeout) {
            UDA_LOG(UDA_LOG_DEBUG, "Total Wait Time Exceeds Lifetime Limit = {} (ms). Closing server down.",
                    *io_data->server_timeout * 1000);
            return -1;
        }

#  ifndef _WIN32
        int fopts = 0;
        if (fcntl(g_sslSocket, F_GETFL, &fopts) < 0 || errno == EBADF) {
            // Is the socket closed? Check status flags
            UDA_LOG(UDA_LOG_DEBUG, "Client Socket is closed! Closing server down.");
            return -1;
        }
#  endif

        protocol::update_select_params(g_sslSocket, &rfds, &tv,
                                       *io_data->server_tot_block_time); // Keep blocking and wait for data
        tvc = tv;
    }

    // First byte of encrypted data received but need the full record in buffer before SSL can decrypt

    int blocked;
    do {
        blocked = 0;
        rc = SSL_read(g_ssl, buf, count);

        switch (SSL_get_error(g_ssl, rc)) { // check for SSL errors
            case SSL_ERROR_NONE:            // clean read
                break;

            case SSL_ERROR_ZERO_RETURN: // connection closed by client     (not caught by select?)
                report_server_ssl_error_code(*io_data->error_stack, rc);
                UDA_LOG(UDA_LOG_DEBUG, "Client socket connection closed!");
                UDA_ADD_ERROR(*io_data->error_stack, 999, "Client socket connection closed!");
                return -1;

            case SSL_ERROR_WANT_READ: // the operation did not complete, try again
                blocked = 1;
                break;

            case SSL_ERROR_WANT_WRITE: // the operation did not complete, error
                report_server_ssl_error_code(*io_data->error_stack, rc);
                UDA_LOG(UDA_LOG_DEBUG, "A read operation failed!");
                UDA_ADD_ERROR(*io_data->error_stack, 999, "A read operation failed!");
                return -1;

            case SSL_ERROR_SYSCALL: // some I/O error occured - disconnect?
                report_server_ssl_error_code(*io_data->error_stack, rc);
                UDA_LOG(UDA_LOG_DEBUG, "Client socket read I/O error!");
                UDA_ADD_ERROR(*io_data->error_stack, 999, "Client socket read I/O error!");
                return -1;

            default: // some other error
                report_server_ssl_error_code(*io_data->error_stack, rc);
                UDA_LOG(UDA_LOG_DEBUG, "Read from socket failed!");
                UDA_ADD_ERROR(*io_data->error_stack, 999, "Read from socket failed!");
#  ifndef _WIN32
                int fopts = 0;
                if ((rc = fcntl(g_sslSocket, F_GETFL, &fopts)) < 0 ||
                    errno == EBADF) { // Is the socket closed? Check status flags
                    UDA_LOG(UDA_LOG_DEBUG, "writeUdaServerSSL: Client Socket is closed! Closing server down.");
                }
#  endif
                return -1;
        }

    } while (SSL_pending(g_ssl) && !blocked); // data remaining in buffer or re-read attempt

    return rc;
}

#endif // SERVERBUILD
