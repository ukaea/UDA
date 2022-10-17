#if defined(SSLAUTHENTICATION) && !defined(SERVERBUILD) && !defined(FATCLIENT)

#include "udaClientSSL.h"

#include <stdio.h>
#include <fcntl.h>
#include <time.h>

#include <client/updateSelectParms.h>
#include <clientserver/errorLog.h>
#include <logging/logging.h>
#include <client/udaClientHostList.h>

static bool g_sslDisabled = true;   // Default state is not SSL authentication
static int g_sslProtocol = 0;       // The default server host name has the SSL protocol name prefix or
static int g_sslSocket = -1;
static bool g_sslOK = false;        // SSL Authentication has been passed sucessfully: default is NOT Passed
static bool g_sslInit = false;      // Global initialisation of SSL completed
static SSL* g_ssl = nullptr;
static SSL_CTX* g_ctx = nullptr;

bool getUdaClientSSLDisabled()
{
    return g_sslDisabled;
}

void putUdaClientSSLProtocol(int specified)
{
    g_sslProtocol = specified;
}

void putUdaClientSSLSocket(int s)
{
    g_sslSocket = s;
}

void initUdaClientSSL()
{
    if (g_sslInit) {
        return;    // Already initialised
    }
    if (getenv("UDA_SSL_INITIALISED")) {
        g_sslInit = true;
        UDA_LOG(UDA_LOG_DEBUG, "Prior SSL initialisation\n");
        return;
    }
    SSL_library_init();
    SSL_load_error_strings();
    OpenSSL_add_ssl_algorithms();
#ifdef _WIN32
    if (getenv("UDA_SSL_INITIALISED") == nullptr) {
        _putenv_s("UDA_SSL_INITIALISED", "1");
    }
#else
    setenv("UDA_SSL_INITIALISED", "1", 0);
#endif
    g_sslInit = true;
    UDA_LOG(UDA_LOG_DEBUG, "SSL initialised\n");
}

void closeUdaClientSSL()
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
#ifdef _WIN32
    _putenv_s("UDA_SSL_INITIALISED", nullptr);
#else
    unsetenv("UDA_SSL_INITIALISED");
#endif
    g_sslInit = false;
    UDA_LOG(UDA_LOG_DEBUG, "SSL closed\n");
}

SSL* getUdaClientSSL()
{
    return g_ssl;
}

void putUdaClientSSLCTX(SSL_CTX* c)
{
    g_ctx = c;
}

void reportSSLErrorCode(int rc)
{
    int err = SSL_get_error(getUdaClientSSL(), rc);
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
    ADD_ERROR(999, msg);
    UDA_LOG(UDA_LOG_DEBUG, "Error - %s\n", msg);
    UDA_LOG(UDA_LOG_DEBUG, "Error - %s\n", ERR_error_string(ERR_get_error(), nullptr));
    UDA_LOG(UDA_LOG_DEBUG, "State - %s\n", SSL_state_string(getUdaClientSSL()));
}

SSL_CTX* createUdaClientSSLContext()
{
    const SSL_METHOD* method = SSLv23_client_method(); // standard TCP

    // method = DTLSv1_client_method()// reliable UDP

    SSL_CTX* ctx = SSL_CTX_new(method);
    putUdaClientSSLCTX(ctx);

    if (!ctx) {
        ADD_ERROR(999, "Unable to create SSL context");
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

    UDA_LOG(UDA_LOG_DEBUG, "SSL Context created\n");

    return ctx;
}

int configureUdaClientSSLContext()
{
    //SSL_CTX_set_ecdh_auto(g_ctx, 1);

    // Set the key and cert - these take priority over entries in the host configuration file

    const char* cert = getenv("UDA_CLIENT_SSL_CERT");
    const char* key = getenv("UDA_CLIENT_SSL_KEY");
    const char* ca = getenv("UDA_CLIENT_CA_SSL_CERT");

    if (!cert || !key || !ca) {
        // Check the client hosts configuration file
        int hostId = -1;
        if ((hostId = udaClientGetHostNameId()) >= 0) {
            // Socket connection was opened with a host entry in the configuration file
            if (!cert) {
                cert = udaClientGetHostCertificatePath(hostId);
            }
            if (!key) {
                key = udaClientGetHostKeyPath(hostId);
            }
            if (!ca) {
                ca = udaClientGetHostCAPath(hostId);
            }
            UDA_LOG(UDA_LOG_DEBUG,
                    "SSL certificates and private key obtained from the hosts configuration file. Host id = %d\n",
                    hostId);
        }
        if (!cert || !key || !ca || cert[0] == '\0' || key[0] == '\0' || ca[0] == '\0') {
            if (!cert || cert[0] == '\0') {
                UDA_LOG(UDA_LOG_DEBUG, "No Client SSL certificate\n");
                ADD_ERROR(999, "No client SSL certificate!");
            }
            if (!key || key[0] == '\0') {
                UDA_LOG(UDA_LOG_DEBUG, "No Client Private Key\n");
                ADD_ERROR(999, "No client SSL key!");
            }
            if (!ca || ca[0] == '\0') {
                UDA_LOG(UDA_LOG_DEBUG, "No CA SSL certificate\n");
                ADD_ERROR(999, "No Certificate Authority certificate!");
            }
            UDA_LOG(UDA_LOG_DEBUG, "Error: No SSL certificates and/or private key!\n");
            return 999;
        }
    }

    UDA_LOG(UDA_LOG_DEBUG, "Client SSL certificates: %s\n", cert);
    UDA_LOG(UDA_LOG_DEBUG, "Client SSL key: %s\n", key);
    UDA_LOG(UDA_LOG_DEBUG, "CA SSL certificates: %s\n", ca);

    if (SSL_CTX_use_certificate_file(g_ctx, cert, SSL_FILETYPE_PEM) <= 0) {
        UDA_LOG(UDA_LOG_DEBUG, "Error: Failed to set the client certificate!\n");
        THROW_ERROR(999, "Failed to set the client certificate!");
    }

    if (SSL_CTX_use_PrivateKey_file(g_ctx, key, SSL_FILETYPE_PEM) <= 0) {
        UDA_LOG(UDA_LOG_DEBUG, "Error: Failed to set the client key!\n");
        THROW_ERROR(999, "Failed to set the client key!");
    }

    // Check key and certificate match
    if (SSL_CTX_check_private_key(g_ctx) == 0) {
        UDA_LOG(UDA_LOG_DEBUG, "Error: Private key does not match the certificate public key!\n");
        THROW_ERROR(999, "Private key does not match the certificate public key!");
    }

    // Load certificates of trusted CAs based on file provided
    if (SSL_CTX_load_verify_locations(g_ctx, ca, nullptr) < 1) {
        UDA_LOG(UDA_LOG_DEBUG, "Error: Error setting the certificate authority verify locations!\n");
        THROW_ERROR(999, "Error setting the certificate authority verify locations!");
    }

    // Peer certificate verification
    SSL_CTX_set_verify(g_ctx, SSL_VERIFY_PEER | SSL_VERIFY_FAIL_IF_NO_PEER_CERT, nullptr);
    SSL_CTX_set_verify_depth(g_ctx, VERIFY_DEPTH);

    UDA_LOG(UDA_LOG_DEBUG, "SSL context configured\n");

    // validate the client's certificate
    FILE* fd = fopen(cert, "r");

    if (!fd) {
        UDA_LOG(UDA_LOG_DEBUG, "Unable to open client certificate [%s] to verify certificate validity\n", cert);
        THROW_ERROR(999, "Unable to open client certificate to verify certificate validity!");
    }

    X509* clientCert = PEM_read_X509(fd, nullptr, nullptr, nullptr);

    fclose(fd);

    if (!clientCert) {
        X509_free(clientCert);
        UDA_LOG(UDA_LOG_DEBUG, "Unable to parse client certificate [%s] to verify certificate validity\n", cert);
        THROW_ERROR(999, "Unable to parse client certificate [%s] to verify certificate validity");
    }

    const ASN1_TIME* before = X509_get_notBefore(clientCert);
    const ASN1_TIME* after = X509_get_notAfter(clientCert);

    char work[X509STRINGSIZE];
    UDA_LOG(UDA_LOG_DEBUG, "Client X509 subject: %s\n",
            X509_NAME_oneline(X509_get_subject_name(clientCert), work, sizeof(work)));
    UDA_LOG(UDA_LOG_DEBUG, "Client X509 issuer: %s\n",
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
    UDA_LOG(UDA_LOG_DEBUG, "Client X509 not before: %s\n", work);
    if ((rc = X509_cmp_time(before, &current_time)) >= 0) {
        // Not Before is after Now!
        X509_free(clientCert);
        UDA_LOG(UDA_LOG_DEBUG, "Current Time               : %s\n", c_time_string);
        UDA_LOG(UDA_LOG_DEBUG, "Client X509 not before date is before the current date!\n");
        UDA_LOG(UDA_LOG_DEBUG, "The client SSL/x509 certificate is Not Valid - the Vaidity Date is in the future!\n");
        THROW_ERROR(999, "The client SSL/x509 certificate is Not Valid - the Vaidity Date is in the future");
    }

    count = 0;
    b = BIO_new(BIO_s_mem());
    if (b && ASN1_TIME_print(b, after)) {
        count = BIO_read(b, work, X509STRINGSIZE - 1);
        BIO_free(b);
    }
    work[count] = '\0';
    UDA_LOG(UDA_LOG_DEBUG, "Client X509 not after   : %s\n", work);
    if ((rc = X509_cmp_time(after, &current_time)) <= 0) {// Not After is before Now!
        X509_free(clientCert);
        UDA_LOG(UDA_LOG_DEBUG, "Current Time               : %s\n", c_time_string);
        UDA_LOG(UDA_LOG_DEBUG, "Client X509 not after date is after the current date!\n");
        UDA_LOG(UDA_LOG_DEBUG, "The client SSL/x509 certificate is Not Valid - the Date has Expired!\n");
        THROW_ERROR(999, "The client SSL/x509 certificate is Not Valid - the Date has Expired!");
    }
    X509_free(clientCert);

    UDA_LOG(UDA_LOG_DEBUG, "Current Time               : %s\n", c_time_string);
    UDA_LOG(UDA_LOG_DEBUG, "Cient certificate date validity checked but not validated \n");

    return 0;
}

int startUdaClientSSL()
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

        int hostId = -1;
        if ((hostId = udaClientGetHostNameId()) >= 0) {
            // Socket connection was opened with a host entry in the configuration file
            if (!udaClientGetHostSSL(hostId)) {
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

    UDA_LOG(UDA_LOG_DEBUG, "SSL Authentication is Enabled!\n");

    // Initialise

    initUdaClientSSL();

    if (!(g_ctx = createUdaClientSSLContext())) {
        THROW_ERROR(999, "Unable to create the SSL context!");
    }
    if (configureUdaClientSSLContext() != 0) {
        THROW_ERROR(999, "Unable to configure the SSL context!");
    }

    // Bind an SSL object with the socket

    g_ssl = SSL_new(g_ctx);
    SSL_set_fd(g_ssl, g_sslSocket);

    // Connect to the server
    int rc;
    if ((rc = SSL_connect(g_ssl)) < 1) {
        UDA_LOG(UDA_LOG_DEBUG, "Error connecting to the server!\n");
        if (errno != 0) {
            ADD_SYS_ERROR("Error connecting to the server!");
        }
        reportSSLErrorCode(rc);
        return 999;
    }

    // Get the Server certificate and verify

    X509* peer = SSL_get_peer_certificate(g_ssl);

    if (peer != nullptr) {

        if ((rc = SSL_get_verify_result(g_ssl)) != X509_V_OK) {
            // returns X509_V_OK if the certificate was not obtained as no error occurred!
            ADD_ERROR(999, X509_verify_cert_error_string(rc));
            X509_free(peer);
            UDA_LOG(UDA_LOG_DEBUG, "SSL Server certificate presented but verification error!\n");
            THROW_ERROR(999, "SSL Server certificate presented but verification error!");
        }

        // Server's details - not required apart from logging

        char work[X509STRINGSIZE];
        UDA_LOG(UDA_LOG_DEBUG, "Server certificate verified\n");
        UDA_LOG(UDA_LOG_DEBUG, "X509 subject: %s\n",
                X509_NAME_oneline(X509_get_subject_name(peer), work, sizeof(work)));
        UDA_LOG(UDA_LOG_DEBUG, "X509 issuer: %s\n",
                X509_NAME_oneline(X509_get_issuer_name(peer), work, sizeof(work)));

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
        UDA_LOG(UDA_LOG_DEBUG, "Server X509 not before: %s\n", work);
        if ((rc = X509_cmp_time(before, &current_time)) >= 0) {// Not Before is after Now!
            X509_free(peer);
            UDA_LOG(UDA_LOG_DEBUG, "Current Time               : %s\n", c_time_string);
            UDA_LOG(UDA_LOG_DEBUG, "Server X509 not before date is before the current date!\n");
            UDA_LOG(UDA_LOG_DEBUG,
<<<<<<< HEAD
                    "The Server's SSL/x509 certificate is Not Valid - the Vaidity Date is in the future!\n");
            THROW_ERROR(999, "The Server's SSL/x509 certificate is Not Valid - the Vaidity Date is in the future");
=======
                    "The Server's SSL/x509 certificate is Not Valid - the Validity Date is in the future!\n");
            UDA_THROW_ERROR(999, "The Server's SSL/x509 certificate is Not Valid - the Validity Date is in the future");
>>>>>>> Adding Jekyll docs.
        }

        count = 0;
        b = BIO_new(BIO_s_mem());
        if (b && ASN1_TIME_print(b, after)) {
            count = BIO_read(b, work, X509STRINGSIZE - 1);
            BIO_free(b);
        }
        work[count] = '\0';
        UDA_LOG(UDA_LOG_DEBUG, "Server X509 not after   : %s\n", work);
        if ((rc = X509_cmp_time(after, &current_time)) <= 0) {// Not After is before Now!
            X509_free(peer);
            UDA_LOG(UDA_LOG_DEBUG, "Current Time               : %s\n", c_time_string);
            UDA_LOG(UDA_LOG_DEBUG, "Server X509 not after date is after the current date!\n");
            UDA_LOG(UDA_LOG_DEBUG, "The Server's SSL/x509 certificate is Not Valid - the Date has Expired!\n");
            THROW_ERROR(999, "The Server's SSL/x509 certificate is Not Valid - the Date has Expired!");
        }

        UDA_LOG(UDA_LOG_DEBUG, "Current Time               : %s\n", c_time_string);

        X509_free(peer);

    } else {
        X509_free(peer);
        UDA_LOG(UDA_LOG_DEBUG, "Server certificate not presented for verification!\n");
        THROW_ERROR(999, "Server certificate not presented for verification!");
    }

    // Print out connection details

    UDA_LOG(UDA_LOG_DEBUG, "SSL version: %s\n", SSL_get_version(g_ssl));
    UDA_LOG(UDA_LOG_DEBUG, "SSL cipher: %s\n", SSL_get_cipher(g_ssl));

    // SSL/TLS authentication has been passed - do not repeat

    g_sslOK = true;

    return 0;
}

int writeUdaClientSSL(void* iohandle, char* buf, int count)
{
    // This routine is only called when there is something to write to the Server
    // SSL uses an all or nothing approach when the socket is blocking - an SSL error or incomplete write
    // means the write has failed

    int rc, err = 0;

    fd_set wfds;        // File Descriptor Set for Writing to the Socket
    struct timeval tv;

    // Block till it's possible to write to the socket or timeout

    udaUpdateSelectParms(g_sslSocket, &wfds, &tv);

    while ((rc = select(g_sslSocket + 1, nullptr, &wfds, nullptr, &tv)) <= 0) {
        if (rc < 0) {    // Error
            if (errno == EBADF) {
                UDA_LOG(UDA_LOG_DEBUG, "Socket is closed! Data access failed!.\n");
            } else {
                UDA_LOG(UDA_LOG_DEBUG, "Read error - %s\n", strerror(errno));
            }
            return -1;
        }

#ifndef _WIN32
        int fopts = 0;
        if ((rc = fcntl(g_sslSocket, F_GETFL, &fopts)) < 0 || errno == EBADF) {
            // Is the socket closed? Check status flags
            err = 999;
            UDA_LOG(UDA_LOG_DEBUG, "Socket is closed!\n");
            return -1;
        }
#endif

        udaUpdateSelectParms(g_sslSocket, &wfds, &tv);
    }

    // set SSL_MODE_AUTO_RETRY flag of the SSL_CTX_set_mode to disable automatic renegotiation?

    rc = SSL_write(getUdaClientSSL(), buf, count);

    switch (SSL_get_error(getUdaClientSSL(), rc)) {
        case SSL_ERROR_NONE:
            if (rc != count) {    // Check the write is complete
                err = 999;
                UDA_LOG(UDA_LOG_DEBUG, "Incomplete write to socket!\n");
                addIdamError(CODEERRORTYPE, "writeUdaClientSSL", err, "Incomplete write to socket!");
                return -1;
            }
            break;

        default:
            reportSSLErrorCode(rc);
            err = 999;
            UDA_LOG(UDA_LOG_DEBUG, "Write to socket failed!\n");
            addIdamError(CODEERRORTYPE, "writeUdaClientSSL", err, "Write to socket failed!");
#ifndef _WIN32
            int fopts = 0;
            if ((rc = fcntl(g_sslSocket, F_GETFL, &fopts)) < 0 ||
                errno == EBADF) {    // Is the socket closed? Check status flags
                UDA_LOG(UDA_LOG_DEBUG, "Socket is closed!\n");
            }
#endif
            return -1;
    }

    return rc;
}

int readUdaClientSSL(void* iohandle, char* buf, int count)
{
    int rc, err = 0;
    fd_set rfds;
    struct timeval tv;

    int maxloop = 0;

    // Wait till it's possible to read from socket 

    udaUpdateSelectParms(g_sslSocket, &rfds, &tv);

    while (((rc = select(g_sslSocket + 1, &rfds, nullptr, nullptr, &tv)) <= 0)
            && maxloop++ < MAXLOOP) {

        if (rc < 0) {    // Error
            int serrno = errno;
            addIdamError(SYSTEMERRORTYPE, "readUdaClientSSL", errno, "Socket is Closed!");
            if (serrno == EBADF) {
                UDA_LOG(UDA_LOG_DEBUG, "Socket is closed!\n");
            } else {
                UDA_LOG(UDA_LOG_DEBUG, "Read error - %s\n", strerror(serrno));
            }
            err = 999;
            addIdamError(CODEERRORTYPE, "readUdaClientSSL", err,
                         "Socket is Closed! Data request failed. Restarting connection.");
            UDA_LOG(UDA_LOG_DEBUG,
                    "Socket is Closed! Data request failed. Restarting connection.\n");
            return -1;
        }
#ifndef _WIN32
        int fopts = 0;
        if ((rc = fcntl(g_sslSocket, F_GETFL, &fopts)) < 0 ||
            errno == EBADF) {    // Is the socket closed? Check status flags
            err = 999;
            UDA_LOG(UDA_LOG_DEBUG, "Socket is closed!\n");
            return -1;
        }
#endif

        udaUpdateSelectParms(g_sslSocket, &rfds, &tv);        // Keep blocking and wait for data
    }

// First byte of encrypted data received but need the full record in buffer before SSL can decrypt

    int blocked;
    do {
        blocked = 0;
        rc = SSL_read(getUdaClientSSL(), buf, count);

        switch (SSL_get_error(getUdaClientSSL(), rc)) {    // check for SSL errors
            case SSL_ERROR_NONE:                // clean read
                break;

            case SSL_ERROR_ZERO_RETURN:    // connection closed by server (not caught by select?)
                reportSSLErrorCode(rc);
                err = 999;
                UDA_LOG(UDA_LOG_DEBUG, "Server socket connection closed!\n");
                addIdamError(CODEERRORTYPE, "readUdaClientSSL", err,
                             "Server socket connection closed!");
                return -1;

            case SSL_ERROR_WANT_READ:    // the operation did not complete, try again
                blocked = 1;
                break;

            case SSL_ERROR_WANT_WRITE:    //the operation did not complete, error
                reportSSLErrorCode(rc);
                err = 999;
                UDA_LOG(UDA_LOG_DEBUG, "A read operation failed!\n");
                addIdamError(CODEERRORTYPE, "readUdaClientSSL", err, "A read operation failed!");
                return -1;

            case SSL_ERROR_SYSCALL:    //some I/O error occured - disconnect?
                reportSSLErrorCode(rc);
                err = 999;
                UDA_LOG(UDA_LOG_DEBUG, "Socket read I/O error!\n");
                addIdamError(CODEERRORTYPE, "readUdaClientSSL", err, "Socket read I/O error!");
                return -1;

            default:            //some other error
                reportSSLErrorCode(rc);
                err = 999;
                UDA_LOG(UDA_LOG_DEBUG, "Read from socket failed!\n");
                addIdamError(CODEERRORTYPE, "readUdaClientSSL", err, "Read from socket failed!");
#ifndef _WIN32
                int fopts = 0;
                if ((rc = fcntl(g_sslSocket, F_GETFL, &fopts)) < 0 ||
                    errno == EBADF) {    // Is the socket closed? Check status flags
                    UDA_LOG(UDA_LOG_DEBUG, "Socket is closed!\n");
                }
#endif
                return -1;
        }

    } while (SSL_pending(getUdaClientSSL()) && !blocked);    // data remaining in buffer or re-read attempt

    return rc;
}

#endif   // !SERVERBUILD && SSLAUTHENTICATION 
