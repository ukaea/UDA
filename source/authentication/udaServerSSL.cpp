#if defined(SSLAUTHENTICATION) && defined(SERVERBUILD)

#include "udaSSL.h"

#include <clientserver/errorLog.h>
#include <logging/logging.h>
#include <server/writer.h>

/*
Note on initialisation:
UDA Servers using plugins that connect to other UDA servers through the standard client API library
need to block initialisation of the SSL library by the client. Initialisation must be done once only.
As all information passed between plugins and servers is through the interface structure, the state 
of initialisation by the server must be passed within the interface. This state information is local 
and should not be passed to subsequent servers. An alternative and simpler mechanism is for the server
to assign a value to an environment variable, and for the client to test this environment variable. 
*/

static int sslDisabled = 1;        // Default state is not SSL authentication
static int sslSocket = -1;
static int sslOK = 0;        // SSL Authentication has been passed sucessfully: default is NOT Passed
static int sslGlobalInit = 0;        // Global initialisation of SSL completed
static SSL* ssl = nullptr;
static SSL_CTX* ctx = nullptr;

void putUdaServerSSLOK(int ok)
{
    sslOK = ok;
}

int getUdaServerSSLOK()
{
    return sslOK;
}

void putUdaServerSSLDisabled(int disabled)
{
    sslDisabled = disabled;
}

int getUdaServerSSLDisabled()
{
    return sslDisabled;
}

void putUdaServerSSLGlobalInit(int init)
{
    sslGlobalInit = init;
}

int getUdaServerSSLGlobalInit()
{
    return sslGlobalInit;
}

void putUdaServerSSL(SSL* s)
{
    ssl = s;
}

SSL* getUdaServerSSL()
{
    return ssl;
}

void putUdaServerSSLCTX(SSL_CTX* c)
{
    ctx = c;
}

SSL_CTX* getUdaServerSSLCTX()
{
    return ctx;
}

void putUdaServerSSLSocket(int s)
{
    sslSocket = s;
}

int getUdaServerSSLSocket()
{
    return sslSocket;
}

void getUdaServerSSLErrorCode(int rc)
{
    int err = SSL_get_error(getUdaServerSSL(), rc);
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
    addIdamError(CODEERRORTYPE, "udaSSL", err, msg);
    UDA_LOG(UDA_LOG_DEBUG, "udaSSL: Error - %s\n", msg);
    UDA_LOG(UDA_LOG_DEBUG, "udaSSL: Error - %s\n", ERR_error_string(ERR_get_error(), nullptr));
    UDA_LOG(UDA_LOG_DEBUG, "udaSSL: State - %s\n", SSL_state_string(getUdaServerSSL()));
}

void initUdaServerSSL()
{
    if (getUdaServerSSLGlobalInit()) return;    // Already initialised
    if (getenv("UDA_SSL_INITIALISED")) {
        putUdaServerSSLGlobalInit(1);
        UDA_LOG(UDA_LOG_DEBUG, "Prior SSL initialisation\n");
        return;
    }
    SSL_library_init();
    SSL_load_error_strings();
    OpenSSL_add_ssl_algorithms();
#ifdef _WIN32
    _putenv_s("UDA_SSL_INITIALISED", "1");
#else
    setenv("UDA_SSL_INITIALISED", "1", 0);    // Ensure the library is not re-initialised by the UDA client library
#endif
    putUdaServerSSLGlobalInit(1);
    UDA_LOG(UDA_LOG_DEBUG, "SSL initialised\n");
}

void closeUdaServerSSL()
{            // Requires re-initialisation (should only be called once at closedown!)
    if (getUdaServerSSLDisabled()) return;
    putUdaServerSSLOK(0);
    putUdaServerSSLSocket(-1);
    putUdaServerSSLDisabled(1);
    SSL* ssl = getUdaServerSSL();
    SSL_CTX* ctx = getUdaServerSSLCTX();
    if (ssl != nullptr) {
        SSL_shutdown(ssl);
        SSL_free(ssl);
    }
    if (ctx != nullptr) SSL_CTX_free(ctx);
    EVP_cleanup();
    putUdaServerSSL(nullptr);
    putUdaServerSSLCTX(nullptr);
#ifdef _WIN32
    _putenv_s("UDA_SSL_INITIALISED", NULL);
#else
    unsetenv("UDA_SSL_INITIALISED");
#endif
    putUdaServerSSLGlobalInit(0);
    UDA_LOG(UDA_LOG_DEBUG, "closeUdaServerSSL: SSL closed\n");
}


SSL_CTX* createUdaServerSSLContext()
{
    int err = 0;

    const SSL_METHOD* method;
    SSL_CTX* ctx;

    method = SSLv23_server_method();        // standard TCP

    // method = DTLSv1_server_method()		// reliable UDP

    ctx = SSL_CTX_new(method);
    putUdaServerSSLCTX(ctx);

    if (!ctx) {
        err = 999;
        addIdamError(CODEERRORTYPE, "udaSSL", err, "Unable to create SSL context");
        UDA_LOG(UDA_LOG_DEBUG, "Unable to create SSL context!\n");
        return nullptr;
    }

// Disable SSLv2 for v3 and TSLv1  negotiation 

    SSL_CTX_set_options(ctx, SSL_OP_NO_SSLv2);

/*
// Set the Cipher List 
   if (SSL_CTX_set_cipher_list(ctx, "AES128-SHA") <= 0) {
      printf("Error setting the cipher list.\n");
      exit(0);
   }    
*/

    UDA_LOG(UDA_LOG_DEBUG, "SSL Context created\n");

    return ctx;
}

int configureUdaServerSSLContext()
{
    int err = 0;

    SSL_CTX* ctx = getUdaServerSSLCTX();

    //SSL_CTX_set_ecdh_auto(ctx, 1);

// Set the key and cert 

    char* cert = getenv("UDA_SERVER_SSL_CERT");
    char* key = getenv("UDA_SERVER_SSL_KEY");
    char* ca = getenv("UDA_SERVER_CA_SSL_CERT");
    char* crlist = getenv("UDA_SERVER_CA_SSL_CRL");

    if (!cert || !key || !ca || !crlist) {
        err = 999;
        if (!cert) addIdamError(CODEERRORTYPE, "udaServerSSL", err, "No server SSL certificate!");
        if (!key) addIdamError(CODEERRORTYPE, "udaServerSSL", err, "No server SSL key!");
        if (!ca) {
            addIdamError(CODEERRORTYPE, "udaServerSSL", err, "No Certificate Authority certificate!");
        }
        if (!crlist) {
            addIdamError(CODEERRORTYPE, "udaServerSSL", err, "No Certificate Revocation List!");
        }
        UDA_LOG(UDA_LOG_DEBUG, "Certificate/Key/CRL environment variable problem!\n");
        return err;
    }

    if (SSL_CTX_use_certificate_file(ctx, cert, SSL_FILETYPE_PEM) <= 0) {
        err = 999;
        addIdamError(CODEERRORTYPE, "udaServerSSL", err, "Failed to set the server certificate!");
        return err;
    }

    if (SSL_CTX_use_PrivateKey_file(ctx, key, SSL_FILETYPE_PEM) <= 0) {
        err = 999;
        addIdamError(CODEERRORTYPE, "udaServerSSL", err, "Failed to set the server key!");
        return err;
    }

// Check key and certificate match 

    if (SSL_CTX_check_private_key(ctx) == 0) {
        err = 999;
        addIdamError(CODEERRORTYPE, "udaServerSSL", err,
                     "Private key does not match the certificate public key!");
        return err;
    }

// Load certificates of trusted CAs   

    if (SSL_CTX_load_verify_locations(ctx, ca, nullptr) < 1) {
        err = 999;
        addIdamError(CODEERRORTYPE, "udaServerSSL", err,
                     "Error setting the Cetificate Authority verify locations!");
        return err;
    }

// Peer certificate verification

    SSL_CTX_set_verify(ctx, SSL_VERIFY_PEER | SSL_VERIFY_FAIL_IF_NO_PEER_CERT, nullptr);
    SSL_CTX_set_verify_depth(ctx, VERIFY_DEPTH);

// Add verification against the Certificate Revocation List

    X509_VERIFY_PARAM* params = X509_VERIFY_PARAM_new();
    X509_VERIFY_PARAM_set_flags(params, X509_V_FLAG_CRL_CHECK);
    SSL_CTX_set1_param(ctx, params);

    X509_CRL* crl = loadUdaServerSSLCrl(crlist);
    if (!crl) return 999;    // CRL not loaded

    STACK_OF(X509_CRL)* crls = sk_X509_CRL_new_null();
    if (!crls || !sk_X509_CRL_push(crls, crl)) {
        err = 999;
        addIdamError(CODEERRORTYPE, "udaServerSSL", err,
                     "Error loading the CRL for client certificate verification!");
        X509_CRL_free(crl);
        return err;
    }

    X509_STORE* st = SSL_CTX_get_cert_store(ctx);
    addUdaServerSSLCrlsStore(st, crls);
    SSL_CTX_set1_verify_cert_store(ctx, st);

// Set CA list used for client authentication 

/*
  if(SSL_CTX_use_certificate_chain_file(ctx, getenv("UDA_SERVER_CA_SSL_CERT")) < 1){
     //printf("Error setting the CA chain file\n");
     exit(0);
  }
*/
/*   
  SSL_CTX_set_client_CA_list(ctx, SSL_load_client_CA_file(getenv("UDA_SERVER_CA_SSL_CERT"))); 

   rc = load_CA(ssl, ctx, getenv("UDA_SERVER_CA_SSL_CERT"));	// calls SSL_CTX_add_client_CA(ctx, X509 *cacert) and         SSL_add_client_CA(ssl, X509 *cacert)
   if(rc == 0)fprintf(logout, "Unable to load Client CA!\n");
*/

    UDA_LOG(UDA_LOG_DEBUG, "SSL Context configured\n");

    return err;
}

X509_CRL* loadUdaServerSSLCrl(char* crlist)
{

    // Load the Certificate Revocation Lists for certificate verification

    int err = 0;

    BIO* in = BIO_new(BIO_s_file());
    if (in == nullptr) {
        err = 999;
        addIdamError(CODEERRORTYPE, "udaSSL", err,
                     "Error creating a Certificate Revocation List object!");
        return nullptr;
    }

    if (BIO_read_filename(in, crlist) <= 0) {
        BIO_free(in);
        err = 999;
        addIdamError(CODEERRORTYPE, "udaSSL", err,
                     "Error opening the Certificate Revocation List file!");
        return nullptr;
    }

    X509_CRL* x = PEM_read_bio_X509_CRL(in, nullptr, nullptr, nullptr);

    if (x == nullptr) {
        BIO_free(in);
        err = 999;
        addIdamError(CODEERRORTYPE, "udaSSL", err,
                     "Error reading the Certificate Revocation List file!");
        return nullptr;
    }

    BIO_free(in);

    UDA_LOG(UDA_LOG_DEBUG, "CRL loaded\n");

    return (x);
}

int addUdaServerSSLCrlsStore(X509_STORE* st, STACK_OF(X509_CRL)* crls)
{
    X509_CRL* crl;
    int i;
    for (i = 0; i < sk_X509_CRL_num(crls); i++) {
        crl = sk_X509_CRL_value(crls, i);
        X509_STORE_add_crl(st, crl);
    }
    return 1;
}

/*
static int load_CA(SSL *ssl, SSL_CTX *ctx, char *file){
    FILE *in;
    X509 *x = nullptr;

    if ((in = fopen(file, "r")) == nullptr){
        fprintf(logout, "Unable to open the CA certificate file\n");
        return (0);
    }
    for (;;) {
        if (PEM_read_X509(in, &x, 0, nullptr) == nullptr){
           fprintf(logout, "Unable to read the CA certificate file\n");
           break;
        }
        SSL_CTX_add_client_CA(ctx, x);
	SSL_add_client_CA(ssl, x);
    }

    if (x != nullptr) X509_free(x);
    fclose(in);
    
    if(x == nullptr) return 0;
    return (1);
}
*/


int startUdaServerSSL()
{
    int rc, err = 0;
    SSL_CTX* ctx = nullptr;

// Has SSL/TLS authentication already been passed?

    if (getUdaServerSSLOK()) return 0;

// Has the server disabled SSL/TLS authentication?

    if (!getenv("UDA_SERVER_SSL_AUTHENTICATE")) {
        putUdaServerSSLDisabled(1);
        return 0;
    } else {
        putUdaServerSSLDisabled(0);
    }

    UDA_LOG(UDA_LOG_DEBUG, "SSL Authentication is Enabled!\n");

// Initialise  

    initUdaServerSSL();

    if (!(ctx = createUdaServerSSLContext())) {
        err = 999;
        addIdamError(CODEERRORTYPE, "udaSSL", err, "Unable to create the SSL context!");
        return err;
    }
    if (configureUdaServerSSLContext() != 0) {
        err = 999;
        addIdamError(CODEERRORTYPE, "udaSSL", err, "Unable to configure the SSL context!");
        return err;
    }

// Bind an SSL object with the socket

    SSL* ssl = SSL_new(ctx);
    if ((rc = SSL_set_fd(ssl, getUdaServerSSLSocket())) < 1) {
        err = 999;
        addIdamError(CODEERRORTYPE, "udaSSL", err, "Unable to bind the socket to SSL!");
        UDA_LOG(UDA_LOG_DEBUG, "udaSSL: Error - Unable to bind the socket to SSL!\n");
        return err;
    }

    putUdaServerSSL(ssl);

// SSL Handshake with Client Authentication

    if ((rc = SSL_accept(ssl)) < 1) {
        if (errno != 0) addIdamError(SYSTEMERRORTYPE, "udaSSL", errno, "SSL Handshake failed!");
        UDA_LOG(UDA_LOG_DEBUG, "Error - SSL Handshake Failed!\n");
        err = SSL_get_error(ssl, rc);
        if (err == 5) {
            UDA_LOG(UDA_LOG_DEBUG, "Error - Client application terminated?!\n");
            addIdamError(CODEERRORTYPE, "udaSSL", err,
                         "SSL error in SSL_accept, application terminated!");
        }
        getUdaServerSSLErrorCode(rc);
        return err;
    }

// Get the Client's certificate and verify

    X509* peer = SSL_get_peer_certificate(ssl);

    if (peer != nullptr) {

        if ((rc = SSL_get_verify_result(ssl)) !=
            X509_V_OK) {    // returns X509_V_OK if the certificate was not obtained as no error occured!
            err = 999;
            addIdamError(CODEERRORTYPE, "udaSSL", err,
                         "SSL Client certificate  presented but verification error!");
            addIdamError(CODEERRORTYPE, "udaSSL", err, X509_verify_cert_error_string(rc));
            X509_free(peer);
            UDA_LOG(UDA_LOG_DEBUG, "SSL Client certificate presented but verification error!\n");
            return err;
        }

// Client's details

        char work[X509STRINGSIZE];
        UDA_LOG(UDA_LOG_DEBUG, "Client certificate verified\n");
        UDA_LOG(UDA_LOG_DEBUG, "X509 subject: %s\n",
                  X509_NAME_oneline(X509_get_subject_name(peer), work, sizeof(work)));
        UDA_LOG(UDA_LOG_DEBUG, "X509 issuer: %s\n",
                  X509_NAME_oneline(X509_get_issuer_name(peer), work, sizeof(work)));
        UDA_LOG(UDA_LOG_DEBUG, "X509 not before: %d\n", X509_get_notBefore(peer));
        UDA_LOG(UDA_LOG_DEBUG, "X509 not after: %d\n", X509_get_notAfter(peer));
        X509_free(peer);
    } else {
        err = 999;
        addIdamError(CODEERRORTYPE, "udaSSL", err,
                     "Client certificate not presented for verification!");
        X509_free(peer);
        UDA_LOG(UDA_LOG_DEBUG, "Client certificate not presented for verification!\n");
        return err;
    }

// Print out connection details 

    UDA_LOG(UDA_LOG_DEBUG, "SSL version: %s\n", SSL_get_version(ssl));
    UDA_LOG(UDA_LOG_DEBUG, "SSL cipher: %s\n", SSL_get_cipher(ssl));

// SSL/TLS authentication has been passed - do not repeat

    putUdaServerSSLOK(1);

    return 0;
}

int writeUdaServerSSL(void* iohandle, char* buf, int count)
{

//return SSL_write(getUdaServerSSL(), buf, count);

// This routine is only called when there is something to write back to the Client
// SSL uses an all or nothing approach when the socket is blocking - an SSL error or incomplete write means the write has failed

    int rc, err = 0;

    fd_set wfds;        // File Descriptor Set for Writing to the Socket
    struct timeval tv;

    // Block till it's possible to write to the socket or timeout

    setSelectParms(getUdaServerSSLSocket(), &wfds, &tv);

    while ((rc = select(getUdaServerSSLSocket() + 1, nullptr, &wfds, nullptr, &tv)) <= 0) {

        if (rc < 0) {    // Error
            if (errno == EBADF) {
                UDA_LOG(UDA_LOG_DEBUG, "Client Socket is closed! Closing server down.\n");
            } else {
                UDA_LOG(UDA_LOG_DEBUG, "Read error - %s\n", strerror(errno));
                UDA_LOG(UDA_LOG_DEBUG, "Closing server down.\n");
            }
            return -1;
        }

        int fopts = 0;
        if ((rc = fcntl(getUdaServerSSLSocket(), F_GETFL, &fopts)) < 0 ||
            errno == EBADF) {    // Is the socket closed? Check status flags
            err = 999;
            UDA_LOG(UDA_LOG_DEBUG, "Client Socket is closed! Closing server down.\n");
            return -1;
        }

        server_tot_block_time += tv.tv_usec / 1000;

        if (server_tot_block_time / 1000 > server_timeout) {
            UDA_LOG(UDA_LOG_DEBUG, "Total Blocking Time: %d (ms). Closing server down.\n", server_tot_block_time);
            return -1;        // Timeout
        }
        updateSelectParms(getUdaServerSSLSocket(), &wfds, &tv);
    }

// set SSL_MODE_AUTO_RETRY flag of the SSL_CTX_set_mode to disable automatic renegotiation?

    rc = SSL_write(getUdaServerSSL(), buf, count);

    switch (SSL_get_error(getUdaServerSSL(), rc)) {
        case SSL_ERROR_NONE:
            if (rc != count) {    // Check the write is complete
                err = 999;
                UDA_LOG(UDA_LOG_DEBUG, "Incomplete write to socket!\n");
                addIdamError(CODEERRORTYPE, "writeUdaServerSSL", err, "Incomplete write to socket!");
                return -1;
            }
            break;

        default:
            getUdaServerSSLErrorCode(rc);
            err = 999;
            UDA_LOG(UDA_LOG_DEBUG, "Write to socket failed!\n");
            addIdamError(CODEERRORTYPE, "writeUdaServerSSL", err, "Write to socket failed!");
            int fopts = 0;
            if ((rc = fcntl(getUdaServerSSLSocket(), F_GETFL, &fopts)) < 0 ||
                errno == EBADF) {    // Is the socket closed? Check status flags
                    UDA_LOG(UDA_LOG_DEBUG, "Client Socket is closed! Closing server down.\n");
            }
            return -1;
    }

    return rc;

}

int readUdaServerSSL(void* iohandle, char* buf, int count)
{

//return SSL_read(getUdaServerSSL(), buf, count);


    int rc, err = 0;
    fd_set rfds;        // File Descriptor Set for Reading from the Socket
    struct timeval tv, tvc;

    // Wait till it's possible to read from the socket
    // Set the blocking period before a timeout

    setSelectParms(getUdaServerSSLSocket(), &rfds, &tv);
    tvc = tv;

// TODO: Use pselect to include a signal mask to force a timeout

    while ((rc = select(getUdaServerSSLSocket() + 1, &rfds, nullptr, nullptr, &tvc)) <= 0) {

        if (rc < 0) {    // Error
            if (errno == EBADF) {
                UDA_LOG(UDA_LOG_DEBUG, "Client Socket is closed! Closing server down.\n");
            } else {
                UDA_LOG(UDA_LOG_DEBUG, "Read error - %s\n", strerror(errno));
                UDA_LOG(UDA_LOG_DEBUG, "Closing server down.\n");
            }
            return -1;
        }

        server_tot_block_time += (int)tv.tv_usec / 1000;    // ms

        if (server_tot_block_time > 1000 * server_timeout) {
            UDA_LOG(UDA_LOG_DEBUG,
                      "Total Wait Time Exceeds Lifetime Limit = %d (ms). Closing server down.\n",
                      server_timeout * 1000);
            return -1;
        }

        int fopts = 0;
        if ((rc = fcntl(getUdaServerSSLSocket(), F_GETFL, &fopts)) < 0 ||
            errno == EBADF) {    // Is the socket closed? Check status flags
            err = 999;
            UDA_LOG(UDA_LOG_DEBUG, "Client Socket is closed! Closing server down.\n");
            return -1;
        }

        updateSelectParms(getUdaServerSSLSocket(), &rfds, &tv);        // Keep blocking and wait for data
        tvc = tv;
    }

// First byte of encrypted data received but need the full record in buffer before SSL can decrypt

    int blocked;
    do {
        blocked = 0;
        rc = SSL_read(getUdaServerSSL(), buf, count);

        switch (SSL_get_error(getUdaServerSSL(), rc)) {    // check for SSL errors
            case SSL_ERROR_NONE:                    // clean read
                break;

            case SSL_ERROR_ZERO_RETURN:    // connection closed by client 	(not caught by select?)
                getUdaServerSSLErrorCode(rc);
                err = 999;
                UDA_LOG(UDA_LOG_DEBUG, "Client socket connection closed!\n");
                addIdamError(CODEERRORTYPE, "readUdaServerSSL", err,
                             "Client socket connection closed!");
                return -1;

            case SSL_ERROR_WANT_READ:    // the operation did not complete, try again
                blocked = 1;
                break;

            case SSL_ERROR_WANT_WRITE:    //the operation did not complete, error
                getUdaServerSSLErrorCode(rc);
                err = 999;
                UDA_LOG(UDA_LOG_DEBUG, "A read operation failed!\n");
                addIdamError(CODEERRORTYPE, "readUdaServerSSL", err, "A read operation failed!");
                return -1;

            case SSL_ERROR_SYSCALL:    //some I/O error occured - disconnect?
                getUdaServerSSLErrorCode(rc);
                err = 999;
                UDA_LOG(UDA_LOG_DEBUG, "Client socket read I/O error!\n");
                addIdamError(CODEERRORTYPE, "readUdaServerSSL", err, "Client socket read I/O error!");
                return -1;

            default:            //some other error
                getUdaServerSSLErrorCode(rc);
                err = 999;
                UDA_LOG(UDA_LOG_DEBUG, "Read from socket failed!\n");
                addIdamError(CODEERRORTYPE, "readUdaServerSSL", err, "Read from socket failed!");
                int fopts = 0;
                if ((rc = fcntl(getUdaServerSSLSocket(), F_GETFL, &fopts)) < 0 ||
                    errno == EBADF) {    // Is the socket closed? Check status flags
                        UDA_LOG(UDA_LOG_DEBUG, "writeUdaServerSSL: Client Socket is closed! Closing server down.\n");
                }
                return -1;
        }

    } while (SSL_pending(getUdaServerSSL()) && !blocked);    // data remaining in buffer or re-read attempt

    return rc;
}

#endif   // SERVERBUILD && SSLAUTHENTICATION 
