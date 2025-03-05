#include <client2/connection.hpp>
#if defined(SSLAUTHENTICATION) && !defined(SERVERBUILD) && !defined(FATCLIENT)

#  include "udaClientSSL.h"

#  include <cstdio>
#  include <fcntl.h>
#  include <openssl/ssl.h>
#  include <time.h>

#  include "client/udaClientHostList.h"
#  include "clientserver/socket_structs.h"
#  include "clientserver/error_log.h"
#  include "logging/logging.h"
#  include "updateSelectParms.h"

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

void putUdaClientSSLCTX(SSL_CTX* c)
{
    g_ctx = c;
}

void reportSSLErrorCode(std::vector<UdaError>& error_stack, int rc)
{
    int err = SSL_get_error(get_client_ssl(), rc);
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
    UDA_ADD_ERROR(error_stack, 999, msg);
    UDA_LOG(UDA_LOG_DEBUG, "Error - {}", msg);
    UDA_LOG(UDA_LOG_DEBUG, "Error - {}", ERR_error_string(ERR_get_error(), nullptr));
    UDA_LOG(UDA_LOG_DEBUG, "State - {}", SSL_state_string(get_client_ssl()));
}

SSL_CTX* createUdaClientSSLContext(std::vector<UdaError>& error_stack)
{
    const SSL_METHOD* method = SSLv23_client_method(); // standard TCP

    // method = DTLSv1_client_method()// reliable UDP

    SSL_CTX* ctx = SSL_CTX_new(method);
    putUdaClientSSLCTX(ctx);

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

int configureUdaClientSSLContext(std::vector<UdaError>& error_stack, const HostData* host)
{
    // SSL_CTX_set_ecdh_auto(g_ctx, 1);

    // Set the key and cert - these take priority over entries in the host configuration file

    const char* cert = getenv("UDA_CLIENT_SSL_CERT");
    const char* key = getenv("UDA_CLIENT_SSL_KEY");
    const char* ca = getenv("UDA_CLIENT_CA_SSL_CERT");

    if (!cert || !key || !ca) {
        // Check the client hosts configuration file
        int hostId = -1;
        if (host != nullptr) {
            // Socket connection was opened with a host entry in the configuration file
            if (!cert) {
                cert = host->certificate.c_str();
            }
            if (!key) {
                key = host->key.c_str();
            }
            if (!ca) {
                ca = host->ca_certificate.c_str();
            }
            UDA_LOG(UDA_LOG_DEBUG,
                    "SSL certificates and private key obtained from the hosts configuration file. Host id = %d\n",
                    hostId);
        }
        if (!cert || !key || !ca || cert[0] == '\0' || key[0] == '\0' || ca[0] == '\0') {
            if (!cert || cert[0] == '\0') {
                UDA_LOG(UDA_LOG_DEBUG, "No Client SSL certificate");
                UDA_ADD_ERROR(error_stack, 999, "No client SSL certificate!");
            }
            if (!key || key[0] == '\0') {
                UDA_LOG(UDA_LOG_DEBUG, "No Client Private Key");
                UDA_ADD_ERROR(error_stack, 999, "No client SSL key!");
            }
            if (!ca || ca[0] == '\0') {
                UDA_LOG(UDA_LOG_DEBUG, "No CA SSL certificate");
                UDA_ADD_ERROR(error_stack, 999, "No Certificate Authority certificate!");
            }
            UDA_LOG(UDA_LOG_DEBUG, "Error: No SSL certificates and/or private key!");
            return 999;
        }
    }

    UDA_LOG(UDA_LOG_DEBUG, "Client SSL certificates: {}", cert);
    UDA_LOG(UDA_LOG_DEBUG, "Client SSL key: {}", key);
    UDA_LOG(UDA_LOG_DEBUG, "CA SSL certificates: {}", ca);

    if (SSL_CTX_use_certificate_file(g_ctx, cert, SSL_FILETYPE_PEM) <= 0) {
        UDA_LOG(UDA_LOG_DEBUG, "Error: Failed to set the client certificate!");
        UDA_THROW_ERROR(error_stack, 999, "Failed to set the client certificate!");
    }

    if (SSL_CTX_use_PrivateKey_file(g_ctx, key, SSL_FILETYPE_PEM) <= 0) {
        UDA_LOG(UDA_LOG_DEBUG, "Error: Failed to set the client key!");
        UDA_THROW_ERROR(error_stack, 999, "Failed to set the client key!");
    }

    // Check key and certificate match
    if (SSL_CTX_check_private_key(g_ctx) == 0) {
        UDA_LOG(UDA_LOG_DEBUG, "Error: Private key does not match the certificate public key!");
        UDA_THROW_ERROR(error_stack, 999, "Private key does not match the certificate public key!");
    }

    // Load certificates of trusted CAs based on file provided
    if (SSL_CTX_load_verify_locations(g_ctx, ca, nullptr) < 1) {
        UDA_LOG(UDA_LOG_DEBUG, "Error: Error setting the certificate authority verify locations!");
        UDA_THROW_ERROR(error_stack, 999, "Error setting the certificate authority verify locations!");
    }

    // Peer certificate verification
    SSL_CTX_set_verify(g_ctx, SSL_VERIFY_PEER | SSL_VERIFY_FAIL_IF_NO_PEER_CERT, nullptr);
    SSL_CTX_set_verify_depth(g_ctx, VERIFY_DEPTH);

    UDA_LOG(UDA_LOG_DEBUG, "SSL context configured");

    // validate the client's certificate
    FILE* fd = fopen(cert, "r");

    if (!fd) {
        UDA_LOG(UDA_LOG_DEBUG, "Unable to open client certificate [{}] to verify certificate validity", cert);
        UDA_THROW_ERROR(error_stack, 999, "Unable to open client certificate to verify certificate validity!");
    }

    X509* clientCert = PEM_read_X509(fd, nullptr, nullptr, nullptr);

    fclose(fd);

    if (!clientCert) {
        X509_free(clientCert);
        UDA_LOG(UDA_LOG_DEBUG, "Unable to parse client certificate [{}] to verify certificate validity", cert);
        UDA_THROW_ERROR(error_stack, 999, "Unable to parse client certificate [{}] to verify certificate validity");
    }

    const ASN1_TIME* before = X509_get_notBefore(clientCert);
    const ASN1_TIME* after = X509_get_notAfter(clientCert);

    char work[X509STRINGSIZE];
    UDA_LOG(UDA_LOG_DEBUG, "Client X509 subject: {}",
            X509_NAME_oneline(X509_get_subject_name(clientCert), work, sizeof(work)));
    UDA_LOG(UDA_LOG_DEBUG, "Client X509 issuer: {}",
            X509_NAME_oneline(X509_get_issuer_name(clientCert), work, sizeof(work)));

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
        X509_free(clientCert);
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
        X509_free(clientCert);
        UDA_LOG(UDA_LOG_DEBUG, "Current Time               : {}", c_time_string);
        UDA_LOG(UDA_LOG_DEBUG, "Client X509 not after date is after the current date!");
        UDA_LOG(UDA_LOG_DEBUG, "The client SSL/x509 certificate is Not Valid - the Date has Expired!");
        UDA_THROW_ERROR(error_stack, 999, "The client SSL/x509 certificate is Not Valid - the Date has Expired!");
    }
    X509_free(clientCert);

    UDA_LOG(UDA_LOG_DEBUG, "Current Time               : {}", c_time_string);
    UDA_LOG(UDA_LOG_DEBUG, "Client certificate date validity checked but not validated");

    return 0;
}

int uda::authentication::init_client_ssl(std::vector<UdaError>& error_stack)
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

    if (!(g_ctx = createUdaClientSSLContext(error_stack))) {
        UDA_THROW_ERROR(error_stack, 999, "Unable to create the SSL context!");
    }

    if (configureUdaClientSSLContext(error_stack, g_host) != 0) {
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
        reportSSLErrorCode(error_stack, rc);
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

int uda::authentication::write_client_ssl(void* iohandle, char* buf, int count)
{
    // This routine is only called when there is something to write to the Server
    // SSL uses an all or nothing approach when the socket is blocking - an SSL error or incomplete write
    // means the write has failed

    int rc, err = 0;

    fd_set wfds; // File Descriptor Set for Writing to the Socket
    struct timeval tv;

    // Block till it's possible to write to the socket or timeout

    auto io_data = reinterpret_cast<client::IoData*>(iohandle);

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
            reportSSLErrorCode(*io_data->error_stack, rc);
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

int uda::authentication::read_client_ssl(void* iohandle, char* buf, int count)
{
    int rc, err = 0;
    fd_set rfds;
    struct timeval tv;

    int maxloop = 0;

    // Wait till it's possible to read from socket

    auto io_data = reinterpret_cast<uda::client::IoData*>(iohandle);

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
                reportSSLErrorCode(*io_data->error_stack, rc);
                err = 999;
                UDA_LOG(UDA_LOG_DEBUG, "Server socket connection closed!");
                add_error(*io_data->error_stack, ErrorType::Code, "readUdaClientSSL", err, "Server socket connection closed!");
                return -1;

            case SSL_ERROR_WANT_READ: // the operation did not complete, try again
                blocked = 1;
                break;

            case SSL_ERROR_WANT_WRITE: // the operation did not complete, error
                reportSSLErrorCode(*io_data->error_stack, rc);
                err = 999;
                UDA_LOG(UDA_LOG_DEBUG, "A read operation failed!");
                add_error(*io_data->error_stack, ErrorType::Code, "readUdaClientSSL", err, "A read operation failed!");
                return -1;

            case SSL_ERROR_SYSCALL: // some I/O error occured - disconnect?
                reportSSLErrorCode(*io_data->error_stack, rc);
                err = 999;
                UDA_LOG(UDA_LOG_DEBUG, "Socket read I/O error!");
                add_error(*io_data->error_stack, ErrorType::Code, "readUdaClientSSL", err, "Socket read I/O error!");
                return -1;

            default: // some other error
                reportSSLErrorCode(*io_data->error_stack, rc);
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
