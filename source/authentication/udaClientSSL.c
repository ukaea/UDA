#if defined(SSLAUTHENTICATION) && !defined(SERVERBUILD) && !defined(FATCLIENT)

#include "udaSSL.h"

#include <fcntl.h>

#include <client/updateSelectParms.h>
#include <clientserver/errorLog.h>
#include <logging/logging.h>
#include <client/udaClientHostList.h>

static int sslDisabled = 1;         // Default state is not SSL authentication
static int sslProtocol = 0;         // The default server host name has the SSL protocol name prefix or 
static int sslSocket = -1;
static int sslOK = 0;               // SSL Authentication has been passed sucessfully: default is NOT Passed
static int sslGlobalInit = 0;       // Global initialisation of SSL completed
static SSL* ssl = NULL;
static SSL_CTX* ctx = NULL;

void putUdaClientSSLProtocol(int specified)
{
    sslProtocol = specified;
}

int getUdaClientSSLProtocol()
{
    return sslProtocol;
}

void putUdaClientSSLOK(int ok)
{
    sslOK = ok;
}

int getUdaClientSSLOK()
{
    return sslOK;
}

void putUdaClientSSLGlobalInit(int init)
{
    sslGlobalInit = init;
}

int getUdaClientSSLGlobalInit()
{
    return sslGlobalInit;
}

void putUdaClientSSLDisabled(int disabled)
{
    sslDisabled = disabled;
}

int getUdaClientSSLDisabled()
{
    return sslDisabled;
}

void putUdaClientSSLSocket(int s)
{
    sslSocket = s;
}

int getUdaClientSSLSocket()
{
    return sslSocket;
}

void initUdaClientSSL()
{
    if (getUdaClientSSLGlobalInit()) return;    // Already initialised
    if (getenv("UDA_SSL_INITIALISED")) {
        putUdaClientSSLGlobalInit(1);
        UDA_LOG(UDA_LOG_DEBUG, "Prior SSL initialisation\n");
        return;
    }
    SSL_library_init();
    SSL_load_error_strings();
    OpenSSL_add_ssl_algorithms();
    setenv("UDA_SSL_INITIALISED", "1", 0);    // Ensure the library is not re-initialised by the UDA server
    putUdaClientSSLGlobalInit(1);
    UDA_LOG(UDA_LOG_DEBUG, "SSL initialised\n");
}

void closeUdaClientSSL()
{            // Requires re-initialisation
    if (getUdaClientSSLDisabled()) return;
    putUdaClientSSLOK(0);
    putUdaClientSSLSocket(-1);
    putUdaClientSSLProtocol(0);
    putUdaClientSSLDisabled(1);
    SSL* ssl = getUdaClientSSL();
    SSL_CTX* ctx = getUdaClientSSLCTX();
    if (ssl != NULL) {
        SSL_shutdown(ssl);
        SSL_free(ssl);
    }
    if (ctx != NULL) SSL_CTX_free(ctx);
    EVP_cleanup();
    putUdaClientSSL(NULL);
    putUdaClientSSLCTX(NULL);
    unsetenv("UDA_SSL_INITIALISED");
    putUdaClientSSLGlobalInit(0);
    UDA_LOG(UDA_LOG_DEBUG, "SSL closed\n");
}

void putUdaClientSSL(SSL* s)
{
    ssl = s;
    return;
}

SSL* getUdaClientSSL()
{
    return ssl;
}

void putUdaClientSSLCTX(SSL_CTX* c)
{
    ctx = c;
    return;
}

SSL_CTX* getUdaClientSSLCTX()
{
    return ctx;
}

void getUdaClientSSLErrorCode(int rc)
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
    err = 999;
    addIdamError(CODEERRORTYPE, "udaSSL", err, msg);
    UDA_LOG(UDA_LOG_DEBUG, "udaSSL: Error - %s\n", msg);
    UDA_LOG(UDA_LOG_DEBUG, "udaSSL: Error - %s\n", ERR_error_string(ERR_get_error(), NULL));
    UDA_LOG(UDA_LOG_DEBUG, "udaSSL: State - %s\n", SSL_state_string(getUdaClientSSL()));
}

SSL_CTX* createUdaClientSSLContext()
{
    int err = 0;

    const SSL_METHOD* method;
    SSL_CTX* ctx;

    method = SSLv23_client_method();        // standard TCP

    // method = DTLSv1_client_method()		// reliable UDP

    ctx = SSL_CTX_new(method);
    putUdaClientSSLCTX(ctx);

    if (!ctx) {
        err = 999;
        addIdamError(CODEERRORTYPE, "udaSSL", err, "Unable to create SSL context");
        return NULL;
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

int configureUdaClientSSLContext()
{
    int err = 0;

    SSL_CTX* ctx = getUdaClientSSLCTX();

    //SSL_CTX_set_ecdh_auto(ctx, 1);

    // Set the key and cert - these take priority over entries in the host configuration file

    char* cert = getenv("UDA_CLIENT_SSL_CERT");
    char* key = getenv("UDA_CLIENT_SSL_KEY");
    char* ca = getenv("UDA_CLIENT_CA_SSL_CERT");

    if (!cert || !key || !ca) {        // Check the client hosts configuration file
        int hostId = -1;
        if ((hostId = udaClientGetHostNameId()) >=
            0) {    // Socket connection was opened with a host entry in the configuration file
            if (!cert) cert = udaClientGetHostCertificatePath(hostId);
            if (!key) key = udaClientGetHostKeyPath(hostId);
            if (!ca) ca = udaClientGetHostCAPath(hostId);
        }
        if (!cert || !key || !ca || cert[0] == '\0' || key[0] == '\0' || ca[0] == '\0') {
            err = 999;
            if (!cert || cert[0] == '\0') {
                addIdamError(CODEERRORTYPE, "udaClientSSL", err, "No client SSL certificate!");
            }
            if (!key || key[0] == '\0') addIdamError(CODEERRORTYPE, "udaClientSSL", err, "No client SSL key!");
            if (!ca || ca[0] == '\0') {
                addIdamError(CODEERRORTYPE, "udaClientSSL", err, "No Certificate Authority certificate!");
            }
            return err;
        }
    }

    if (SSL_CTX_use_certificate_file(ctx, cert, SSL_FILETYPE_PEM) <= 0) {
        err = 999;
        addIdamError(CODEERRORTYPE, "udaClientSSL", err, "Failed to set the client certificate!");
        return err;
    }

    if (SSL_CTX_use_PrivateKey_file(ctx, key, SSL_FILETYPE_PEM) <= 0) {
        err = 999;
        addIdamError(CODEERRORTYPE, "udaClientSSL", err, "Failed to set the client key!");
        return err;
    }

    // Check key and certificate match 

    if (SSL_CTX_check_private_key(ctx) == 0) {
        err = 999;
        addIdamError(CODEERRORTYPE, "udaClientSSL", err,
                     "Private key does not match the certificate public key!");
        return err;
    }

    // Load certificates of trusted CAs based on file provided 

    if (SSL_CTX_load_verify_locations(ctx, ca, NULL) < 1) {
        err = 999;
        addIdamError(CODEERRORTYPE, "udaClientSSL", err,
                     "Error setting the Cetificate Authority verify locations!");
        return err;
    }

    // Peer certificate verification

    SSL_CTX_set_verify(ctx, SSL_VERIFY_PEER | SSL_VERIFY_FAIL_IF_NO_PEER_CERT, NULL);
    SSL_CTX_set_verify_depth(ctx, VERIFY_DEPTH);

    UDA_LOG(UDA_LOG_DEBUG, "configureUdaClientSSLContext: SSL Context configured\n");

    return err;
}

int startUdaClientSSL()
{
    int err = 0;
    SSL_CTX* ctx = NULL;

// Has SSL/TLS authentication already been passed?

    if (getUdaClientSSLOK()) return 0;

// Has the user specified the SSL protocol on the host URL?           
// Has the user directly specified SSL/TLS authentication?
// Does the connection entry in the client host configuration file have the three SSL authentication files

    if (!getUdaClientSSLProtocol() && !getenv("UDA_CLIENT_SSL_AUTHENTICATE")) {
        putUdaClientSSLDisabled(1);

        int hostId = -1;
        if ((hostId = udaClientGetHostNameId()) >=
            0) {    // Socket connection was opened with a host entry in the configuration file
            char* cert = udaClientGetHostCertificatePath(hostId);    // Check for 3 authentication files
            char* key = udaClientGetHostKeyPath(hostId);
            char* ca = udaClientGetHostCAPath(hostId);
            if (cert[0] == '\0' || key[0] == '\0' || ca[0] == '\0') {    // 3 files are Not present
                return 0;
            } else {
                putUdaClientSSLDisabled(0);
            }
        } else {
            return 0;
        }
    } else {
        putUdaClientSSLDisabled(0);
    }

    UDA_LOG(UDA_LOG_DEBUG, "SSL Authentication is Enabled!\n");

// Initialise   

    initUdaClientSSL();

    if (!(ctx = createUdaClientSSLContext())) {
        err = 999;
        addIdamError(CODEERRORTYPE, "udaSSL", err, "Unable to create the SSL context!");
        return err;
    }
    if (configureUdaClientSSLContext() != 0) {
        err = 999;
        addIdamError(CODEERRORTYPE, "udaSSL", err, "Unable to configure the SSL context!");
        return err;
    }

// Bind an SSL object with the socket

    SSL* ssl = SSL_new(ctx);
    SSL_set_fd(ssl, getUdaClientSSLSocket());

    putUdaClientSSL(ssl);

// Connect to the server

    int rc = SSL_connect(ssl);

// Check for error in connect 

    if (rc < 1) {
        UDA_LOG(UDA_LOG_DEBUG, "Error connecting to the server!\n");
        err = 999;
        if (errno != 0) {
            addIdamError(SYSTEMERRORTYPE, "udaSSL", errno, "Error connecting to the server!");
        }
        getUdaClientSSLErrorCode(rc);
        return err;
    }

// Get the Server certificate and verify

    X509* peer = SSL_get_peer_certificate(ssl);

    if (peer != NULL) {

        if ((rc = SSL_get_verify_result(ssl)) !=
            X509_V_OK) {    // returns X509_V_OK if the certificate was not obtained as no error occured!
            err = 999;
            addIdamError(CODEERRORTYPE, "udaSSL", err,
                         "SSL Server certificate presented but verification error!");
            addIdamError(CODEERRORTYPE, "udaSSL", err, X509_verify_cert_error_string(rc));
            X509_free(peer);
            UDA_LOG(UDA_LOG_DEBUG, "SSL Server certificate presented but verification error!\n");
            return err;
        }

// Server's details - not required apart from logging

        char work[X509STRINGSIZE];
        UDA_LOG(UDA_LOG_DEBUG, "Server certificate verified");
        UDA_LOG(UDA_LOG_DEBUG, "X509 subject: %s\n",
                X509_NAME_oneline(X509_get_subject_name(peer), work, sizeof(work)));
        UDA_LOG(UDA_LOG_DEBUG, "X509 issuer: %s\n",
                X509_NAME_oneline(X509_get_issuer_name(peer), work, sizeof(work)));
        UDA_LOG(UDA_LOG_DEBUG, "X509 not before: %d\n", X509_get_notBefore(peer));
        UDA_LOG(UDA_LOG_DEBUG, "X509 not after: %d\n", X509_get_notAfter(peer));
/*      
      // Write the certificate to a tmp file
      char template[] = "/tmp/UDAServer-X509-XXXXXX";
      char *xname = mktemp(&template);
      FILE *tmp = fopen(xname, "wb+");
      if(tmp){
         PEM_write_X509(tmp, peer);
	 fclose(tmp);
      }
*/
        X509_free(peer);
    } else {
        err = 999;
        addIdamError(CODEERRORTYPE, "udaSSL", err,
                     "Server certificate not presented for verification!");
        X509_free(peer);
        UDA_LOG(UDA_LOG_DEBUG, "Server certificate not presented for verification!\n");
        return err;
    }

// Print out connection details 

    UDA_LOG(UDA_LOG_DEBUG, "SSL version: %s\n", SSL_get_version(ssl));
    UDA_LOG(UDA_LOG_DEBUG, "SSL cipher: %s\n", SSL_get_cipher(ssl));

// SSL/TLS authentication has been passed - do not repeat

    putUdaClientSSLOK(1);

    return 0;
}

int writeUdaClientSSL(void* iohandle, char* buf, int count)
{

//return SSL_write(getUdaClientSSL(), buf, count);

// This routine is only called when there is something to write to the Server
// SSL uses an all or nothing approach when the socket is blocking - an SSL error or incomplete write means the write has failed

    int rc, err = 0;

    fd_set wfds;        // File Descriptor Set for Writing to the Socket
    struct timeval tv;

    // Block till it's possible to write to the socket or timeout

    idamUpdateSelectParms(getUdaClientSSLSocket(), &wfds, &tv);

    while ((rc = select(getUdaClientSSLSocket() + 1, NULL, &wfds, NULL, &tv)) <= 0) {

        if (rc < 0) {    // Error
            if (errno == EBADF) {
                UDA_LOG(UDA_LOG_DEBUG, "Socket is closed! Data access failed!.\n");
            } else {
                UDA_LOG(UDA_LOG_DEBUG, "Read error - %s\n", strerror(errno));
            }
            return -1;
        }

        int fopts = 0;
        if ((rc = fcntl(getUdaClientSSLSocket(), F_GETFL, &fopts)) < 0 ||
            errno == EBADF) {    // Is the socket closed? Check status flags
            err = 999;
            UDA_LOG(UDA_LOG_DEBUG, "Socket is closed!\n");
            return -1;
        }

        idamUpdateSelectParms(getUdaClientSSLSocket(), &wfds, &tv);
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
            getUdaClientSSLErrorCode(rc);
            err = 999;
            UDA_LOG(UDA_LOG_DEBUG, "Write to socket failed!\n");
            addIdamError(CODEERRORTYPE, "writeUdaClientSSL", err, "Write to socket failed!");
            int fopts = 0;
            if ((rc = fcntl(getUdaClientSSLSocket(), F_GETFL, &fopts)) < 0 ||
                errno == EBADF) {    // Is the socket closed? Check status flags
                UDA_LOG(UDA_LOG_DEBUG, "Socket is closed!\n");
            }
            return -1;
    }

    return rc;

}

int readUdaClientSSL(void* iohandle, char* buf, int count)
{

//return SSL_read(getUdaClientSSL(), buf, count);


    int rc, err = 0;
    fd_set rfds;
    struct timeval tv;

    int maxloop = 0;

    // Wait till it's possible to read from socket 

    idamUpdateSelectParms(getUdaClientSSLSocket(), &rfds, &tv);

    while (((rc = select(getUdaClientSSLSocket() + 1, &rfds, NULL, NULL, &tv)) <= 0) && maxloop++ < MAXLOOP) {

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
        int fopts = 0;
        if ((rc = fcntl(getUdaClientSSLSocket(), F_GETFL, &fopts)) < 0 ||
            errno == EBADF) {    // Is the socket closed? Check status flags
            err = 999;
            UDA_LOG(UDA_LOG_DEBUG, "Socket is closed!\n");
            return -1;
        }

        idamUpdateSelectParms(getUdaClientSSLSocket(), &rfds, &tv);        // Keep blocking and wait for data
    }

// First byte of encrypted data received but need the full record in buffer before SSL can decrypt

    int blocked;
    do {
        blocked = 0;
        rc = SSL_read(getUdaClientSSL(), buf, count);

        switch (SSL_get_error(getUdaClientSSL(), rc)) {    // check for SSL errors
            case SSL_ERROR_NONE:                // clean read
                break;

            case SSL_ERROR_ZERO_RETURN:    // connection closed by server 	(not caught by select?)
                getUdaClientSSLErrorCode(rc);
                err = 999;
                UDA_LOG(UDA_LOG_DEBUG, "Server socket connection closed!\n");
                addIdamError(CODEERRORTYPE, "readUdaClientSSL", err,
                             "Server socket connection closed!");
                return -1;

            case SSL_ERROR_WANT_READ:    // the operation did not complete, try again
                blocked = 1;
                break;

            case SSL_ERROR_WANT_WRITE:    //the operation did not complete, error
                getUdaClientSSLErrorCode(rc);
                err = 999;
                UDA_LOG(UDA_LOG_DEBUG, "A read operation failed!\n");
                addIdamError(CODEERRORTYPE, "readUdaClientSSL", err, "A read operation failed!");
                return -1;

            case SSL_ERROR_SYSCALL:    //some I/O error occured - disconnect?
                getUdaClientSSLErrorCode(rc);
                err = 999;
                UDA_LOG(UDA_LOG_DEBUG, "Socket read I/O error!\n");
                addIdamError(CODEERRORTYPE, "readUdaClientSSL", err, "Socket read I/O error!");
                return -1;

            default:            //some other error
                getUdaClientSSLErrorCode(rc);
                err = 999;
                UDA_LOG(UDA_LOG_DEBUG, "Read from socket failed!\n");
                addIdamError(CODEERRORTYPE, "readUdaClientSSL", err, "Read from socket failed!");
                int fopts = 0;
                if ((rc = fcntl(getUdaClientSSLSocket(), F_GETFL, &fopts)) < 0 ||
                    errno == EBADF) {    // Is the socket closed? Check status flags
                    UDA_LOG(UDA_LOG_DEBUG, "Socket is closed!\n");
                }
                return -1;
        }

    } while (SSL_pending(getUdaClientSSL()) && !blocked);    // data remaining in buffer or re-read attempt

    return rc;
}

#endif   // !SERVERBUILD && SSLAUTHENTICATION 
