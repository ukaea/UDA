// udaSSL - Create the SSL context and binding to the socket
// 3 protocol modes: TCP without SSL/TLS, TCP and UDP both with SSL/TLS
// This set of functions is concerned only with the SSL/TLS protocol - not with establishing socket connections
 
#include <signal.h>

#include <authentication/udaSSL.h>
#include <clientserver/errorLog.h>
#include <logging/logging.h>

#include <client/updateSelectParms.h>
#include <server/writer.h>

static SSL *ssl = NULL;
static SSL_CTX *ctx = NULL;
static int sslSocket = 0;

void putUdaSSL(SSL *s){
   ssl = s;
   return;
} 
SSL *getUdaSSL(){
   return ssl;
} 

void putUdaSSLCTX(SSL_CTX *c){
   ctx = c;
   return;
} 
SSL_CTX *getUdaSSLCTX(){
   return ctx;
}

void putUdaSSLSocket(int s){
   sslSocket = s;
   return;
} 
int getUdaSSLSocket(){
   return sslSocket;
}   

void getUdaSSLErrorCode(int rc){
   int err = SSL_get_error(getUdaSSL(), rc);            
   char msg[256];           
   switch (err){
      case SSL_ERROR_NONE: strcpy(msg,"SSL_ERROR_NONE");
      break;
      case SSL_ERROR_ZERO_RETURN: strcpy(msg,"SSL_ERROR_ZERO_RETURN");
      break;
      case SSL_ERROR_WANT_READ: strcpy(msg,"SSL_ERROR_WANT_READ");
      break;
      case SSL_ERROR_WANT_WRITE: strcpy(msg,"SSL_ERROR_WANT_WRITE");
      break;
      case SSL_ERROR_WANT_CONNECT: strcpy(msg,"SSL_ERROR_WANT_CONNECT");
      break;
      case SSL_ERROR_WANT_ACCEPT: strcpy(msg,"SSL_ERROR_WANT_ACCEPT");
      break;
      case SSL_ERROR_WANT_X509_LOOKUP: strcpy(msg,"SSL_ERROR_WANT_X509_LOOKUP");
      break;
      case SSL_ERROR_SYSCALL: strcpy(msg,"SSL_ERROR_SYSCALL");
      break;
      case SSL_ERROR_SSL: strcpy(msg,"SSL_ERROR_SSL");
      break;
   }
   err = 999;
   addIdamError(&idamerrorstack, CODEERRORTYPE, "udaSSL", err, msg);
   IDAM_LOGF(UDA_LOG_DEBUG, "udaSSL: Error - %s\n", ERR_error_string(ERR_get_error(), NULL));
   IDAM_LOGF(UDA_LOG_DEBUG, "udaSSL: State - %s\n", SSL_state_string(getUdaSSL()));
}  

void initUdaSSL(){ 
    SSL_library_init();
    SSL_load_error_strings();	
    OpenSSL_add_ssl_algorithms();
}

void cleanupUdaSSL(){
    EVP_cleanup();
}

SSL_CTX *createUdaSSLContext(){
    int err = 0;
    
    const SSL_METHOD *method;
    SSL_CTX *ctx;

    method = SSLv23_client_method();		// standard TCP 
    
    // method = DTLSv1_client_method()		// reliable UDP

    ctx = SSL_CTX_new(method);
    putUdaSSLCTX(ctx);
    
    if (!ctx) {
      err = 999;
      addIdamError(&idamerrorstack, CODEERRORTYPE, "udaSSL", err, "Unable to create SSL context");
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

// Peer certificate verification
    
    SSL_CTX_set_verify(ctx, SSL_VERIFY_PEER|SSL_VERIFY_FAIL_IF_NO_PEER_CERT, NULL);
    SSL_CTX_set_verify_depth(ctx, VERIFY_DEPTH); 

    return ctx;
}

int configureUdaSSLContext(){
   int err = 0;
   
   SSL_CTX *ctx = getUdaSSLCTX();    
    
   //SSL_CTX_set_ecdh_auto(ctx, 1);

// Set the key and cert 

   char *cert = getenv("UDA_CLIENT_SSL_CERT");
   char *key  = getenv("UDA_CLIENT_SSL_KEY");
   char *ca   = getenv("UDA_CA_SSL_CERT");
   
   if(!cert || !key || !ca){
      err = 999;
      if(!cert) addIdamError(&idamerrorstack, CODEERRORTYPE, "udaSSL", err, "No client SSL certificate!");
      if(!key) addIdamError(&idamerrorstack, CODEERRORTYPE, "udaSSL", err, "No client SSL key!");
      if(!ca) addIdamError(&idamerrorstack, CODEERRORTYPE, "udaSSL", err, "No Certificate Authority certificate!");
      return err;      
   }

   if (SSL_CTX_use_certificate_file(ctx, cert, SSL_FILETYPE_PEM) <= 0) {
      err = 999;
      addIdamError(&idamerrorstack, CODEERRORTYPE, "udaSSL", err, "Failed to set the client certificate!");
      return err;
   }

    if (SSL_CTX_use_PrivateKey_file(ctx, key, SSL_FILETYPE_PEM) <= 0 ) {
      err = 999;
      addIdamError(&idamerrorstack, CODEERRORTYPE, "udaSSL", err, "Failed to set the client key!");
      return err;
    }
        
// Check key and certificate match 

    if (SSL_CTX_check_private_key(ctx) == 0) {
        err = 999;
	addIdamError(&idamerrorstack, CODEERRORTYPE, "udaSSL", err, "Private key does not match the certificate public key!");
	return err;
    }   

// Load certificates of trusted CAs based on file provided 

    if (SSL_CTX_load_verify_locations(ctx, ca, NULL) < 1){
        err = 999;
	addIdamError(&idamerrorstack, CODEERRORTYPE, "udaSSL", err, "Error setting the Cetificate Authority verify locations!");
	return err;
    } 
    return err;      
}

int startUdaSSL(){
   int err = 0;
   initUdaSSL();
   SSL_CTX *ctx = NULL;
   if(!(ctx = createUdaSSLContext())){
      err = 999;
      addIdamError(&idamerrorstack, CODEERRORTYPE, "udaSSL", err, "Unable to create the SSL context!");
      return err;
   }   
   if(configureUdaSSLContext() != 0){
      err = 999;
      addIdamError(&idamerrorstack, CODEERRORTYPE, "udaSSL", err, "Unable to configure the SSL context!");
      return err;
   }
    
// Bind an SSL object with the socket
     
    ssl = SSL_new(ctx);
    SSL_set_fd(ssl, getUdaSSLSocket());
    
    putUdaSSL(ssl);
        
// Connect to the server

    int rc = SSL_connect(ssl);

// Check for error in connect 

    if (rc < 1) {       
       err = 999;
       if(errno != 0) addIdamError(&idamerrorstack, SYSTEMERRORTYPE, "udaSSL", errno, "Error connecting to the server!");       
       getUdaSSLErrorCode(rc);
       SSL_free(ssl);
       SSL_CTX_free(ctx);
       ssl = NULL;
       ctx = NULL;
       putUdaSSL(ssl);
       putUdaSSLCTX(ctx);
       return err;
    }    
       
// Get the Server certificate and verify

   X509 *peer = SSL_get_peer_certificate(ssl); 
   
   if(peer != NULL){

      if ((rc=SSL_get_verify_result(ssl)) != X509_V_OK) {	// returns X509_V_OK if the certificate was not obtained as no error occured!
         err = 999;
	 addIdamError(&idamerrorstack, CODEERRORTYPE, "udaSSL", err, "SSL Server certificate verification error");
	 addIdamError(&idamerrorstack, CODEERRORTYPE, "udaSSL", err, X509_verify_cert_error_string(rc));
         SSL_free(ssl);
         SSL_CTX_free(ctx);
	 X509_free(peer);
         ssl = NULL;
         ctx = NULL;
	 putUdaSSL(ssl);
	 putUdaSSLCTX(ctx);
         return err;
      }

// Server's details - not required apart from logging

      char work[X509STRINGSIZE];
      IDAM_LOG(UDA_LOG_DEBUG, "udaSSL: Server certificate verified");
      IDAM_LOGF(UDA_LOG_DEBUG, "X509 subject: %s\n", X509_NAME_oneline(X509_get_subject_name(peer), work, sizeof(work)));
      IDAM_LOGF(UDA_LOG_DEBUG, "X509 issuer: %s\n", X509_NAME_oneline(X509_get_issuer_name(peer), work, sizeof(work)));
      IDAM_LOGF(UDA_LOG_DEBUG, "X509 not before: %d\n", X509_get_notBefore(peer));
      IDAM_LOGF(UDA_LOG_DEBUG, "X509 not after: %d\n", X509_get_notAfter(peer));
      X509_free(peer);
   } else {
      err = 999;
      addIdamError(&idamerrorstack, CODEERRORTYPE, "udaSSL", err, "Server certificate not presented for verification!");
      SSL_free(ssl);
      SSL_CTX_free(ctx);
      ssl = NULL;
      ctx = NULL;
      putUdaSSL(ssl);
      putUdaSSLCTX(ctx);
      return err;
   }

// Print out connection details 

    IDAM_LOGF(UDA_LOG_DEBUG, "SSL version: %s\n", SSL_get_version(ssl));
    IDAM_LOGF(UDA_LOG_DEBUG, "SSL cipher: %s\n", SSL_get_cipher(ssl));

   return 0;
} 

void closeUdaSSL(){
   SSL_shutdown(getUdaSSL());           
   SSL_free(getUdaSSL());
   SSL_CTX_free(getUdaSSLCTX());
   cleanupUdaSSL();
} 

#ifndef SERVERBUILD
int writeUdaClientSSL(void* iohandle, char* buf, int count)
{
    void (* OldSIGPIPEHandler)();
    int rc = 0, err = 0;

    fd_set wfds;
    struct timeval tv;

    // Block till it's possible to write to the socket

    idamUpdateSelectParms(getUdaSSLSocket(), &wfds, &tv);

    while (select(getUdaSSLSocket() + 1, NULL, &wfds, NULL, &tv) <= 0) {

        if (errno == ECONNRESET || errno == ENETUNREACH || errno == ECONNREFUSED) {
            if (errno == ECONNRESET) {
                IDAM_LOG(UDA_LOG_DEBUG, "writeUdaClientSSL: ECONNRESET error!\n");
                addIdamError(&idamerrorstack, CODEERRORTYPE, "writeUdaClientSSL", -2,
                             "ECONNRESET: The server program has crashed or closed the socket unexpectedly");
                return -2;
            } else {
                if (errno == ENETUNREACH) {
                    IDAM_LOG(UDA_LOG_DEBUG, "writeUdaClientSSL: ENETUNREACH error!\n");
                    addIdamError(&idamerrorstack, CODEERRORTYPE, "writeUdaClientSSL", -3,
                                 "Server Unavailable: ENETUNREACH");
                    return -3;
                } else {
                    IDAM_LOG(UDA_LOG_DEBUG, "writeUdaClientSSL: ECONNREFUSED error!\n");
                    addIdamError(&idamerrorstack, CODEERRORTYPE, "writeUdaClientSSL", -4,
                                 "Server Unavailable: ECONNREFUSED");
                    return -4;
                }
            }
        }

        idamUpdateSelectParms(getUdaSSLSocket(), &wfds, &tv);
    }

    /* UNIX version

     Ignore the SIG_PIPE signal.  Standard behaviour terminates
     the application with an error code of 141.  When the signal
     is ignored, write calls (when there is no server process to
     communicate with) will return with errno set to EPIPE
    */

    if ((OldSIGPIPEHandler = signal(SIGPIPE, SIG_IGN)) == SIG_ERR) {
        addIdamError(&idamerrorstack, CODEERRORTYPE, "writeUdaClientSSL", -1, "Error attempting to ignore SIG_PIPE");
        return -1;
    }

// Write to socket, checking for EINTR, as happens if called from IDL
// if the return code from SSL_write == -1 and WANT_READ or WANT_WRITE then repeat the write with the same arguments

    while (((rc = (int) SSL_write(getUdaSSL(), buf, count)) == -1) && 
           (((err = SSL_get_error(getUdaSSL(), rc)) == SSL_ERROR_WANT_READ || err == SSL_ERROR_WANT_WRITE) || 
	   (errno == EINTR))) { }
    
    if(rc > 0 && rc != count){		// Successful write but incorrect count!!!
       err = 999;
       IDAM_LOG(UDA_LOG_DEBUG, "writeUdaClientSSL: Inconsistent number of bytes written to socket!\n");
       addIdamError(&idamerrorstack, CODEERRORTYPE, "writeUdaClientSSL", err, "Inconsistent number of bytes written to socket!");
       return -1;
    } else
    if(rc == 0){	// Failed write - connection closed? 
       if (errno != 0) addIdamError(&idamerrorstack, SYSTEMERRORTYPE, "writeUdaClientSSL", errno, "");
       err = 999;
       IDAM_LOG(UDA_LOG_DEBUG, "writeUdaClientSSL: Unable to write to socket! Closed?\n");
       addIdamError(&idamerrorstack, CODEERRORTYPE, "writeUdaClientSSL", err, "Unable to write to socket! Closed?");
       getUdaSSLErrorCode(rc);
       return -1;
    } else 
    if(rc == -1){	// Failed write 
       if (errno != 0) addIdamError(&idamerrorstack, SYSTEMERRORTYPE, "writeUdaSSL", errno, "");
       err = 998;
       IDAM_LOG(UDA_LOG_DEBUG, "writeUdaClientSSL: Unable to write to socket! Unknown!\n");
       addIdamError(&idamerrorstack, CODEERRORTYPE, "writeUdaClientSSL", err, "Unable to write to socket!, Unknown!");
       getUdaSSLErrorCode(rc);
       return -1;
    } 
    
// Restore the original SIGPIPE handler set by the application

    if (signal(SIGPIPE, OldSIGPIPEHandler) == SIG_ERR) {
        addIdamError(&idamerrorstack, CODEERRORTYPE, "writeUdaClientSSL", -1,
                     "Error attempting to restore SIG_PIPE handler");
        return -1;
    }

    return rc;
}
 
int readUdaClientSSL(void* iohandle, char* buf, int count)
{
    int rc, err=0;
    fd_set rfds;
    struct timeval tv;

    int maxloop = 0;

    // Wait till it's possible to read from socket 

    idamUpdateSelectParms(getUdaSSLSocket(), &rfds, &tv);

    while ((select(getUdaSSLSocket() + 1, &rfds, NULL, NULL, &tv) <= 0) && maxloop++ < MAXLOOP) {
        idamUpdateSelectParms(getUdaSSLSocket(), &rfds, &tv);        // Keep trying ...
    }

    // Read from it, checking for EINTR, as happens if called from IDL

    while (((rc = (int) SSL_read(getUdaSSL(), buf, count)) == -1) && 
           (((err = SSL_get_error(getUdaSSL(), rc)) == SSL_ERROR_WANT_READ || err == SSL_ERROR_WANT_WRITE) || 
	   (errno == EINTR))) { }

    // As we have waited to be told that there is data to be read, if nothing arrives, then there must be an error

    if(rc == 0){	// Failed read - connection closed? 
       if (errno != 0) addIdamError(&idamerrorstack, SYSTEMERRORTYPE, "readUdaClientSSL", errno, "");
       err = 999;
       IDAM_LOG(UDA_LOG_DEBUG, "readUdaClientSSL: Unable to read from the socket! Closed?\n");
       addIdamError(&idamerrorstack, CODEERRORTYPE, "readUdaClientSSL", err, "No Data waiting at Socket when Data Expected!");
       addIdamError(&idamerrorstack, CODEERRORTYPE, "readUdaClientSSL", err, "Unable to read from the socket! Closed?");
       getUdaSSLErrorCode(rc);
       return -1;
    }else 
    if(rc == -1){	// Failed read 
       if (errno != 0) addIdamError(&idamerrorstack, SYSTEMERRORTYPE, "readUdaClientSSL", errno, "");
       err = 998;
       IDAM_LOG(UDA_LOG_DEBUG, "readUdaClientSSL: Unable to read from the socket! Unknown!\n");
       addIdamError(&idamerrorstack, CODEERRORTYPE, "readUdaClientSSL", err, "No Data waiting at Socket when Data Expected!");
       addIdamError(&idamerrorstack, CODEERRORTYPE, "readUdaClientSSL", err, "Unable to read from the socket!, Unknown!");
       getUdaSSLErrorCode(rc);
       return -1;
    }  

    return rc;
}

#endif   // #ifndef SERVERBUILD

//============================================================================================================================

#ifdef SERVERBUILD
int writeUdaServerSSL(void* iohandle, char* buf, int count)
{

// This routine is only called when there is something to write back to the Client

    int rc, err=0;

    fd_set wfds;        // File Descriptor Set for Writing to the Socket
    struct timeval tv;

    // Block till it's possible to write to the socket

    setSelectParms(getUdaSSLSocket(), &wfds, &tv);

    while (select(getUdaSSLSocket() + 1, NULL, &wfds, NULL, &tv) <= 0) {
        server_tot_block_time += tv.tv_usec / 1000;
        if (server_tot_block_time / 1000 > server_timeout) {
            IDAM_LOGF(UDA_LOG_DEBUG, "writeUdaServerSSL: Total Blocking Time: %d (ms)\n", server_tot_block_time);
        }
        if (server_tot_block_time / 1000 > server_timeout) return -1;
        updateSelectParms(getUdaSSLSocket(), &wfds, &tv);
    }

// Write to socket, checking for EINTR, as happens if called from IDL
// if the return code from SSL_write == -1 and WANT_READ or WANT_WRITE then repeat the write with the same arguments

    while (((rc = (int) SSL_write(getUdaSSL(), buf, count)) == -1) && 
           (((err = SSL_get_error(getUdaSSL(), rc)) == SSL_ERROR_WANT_READ || err == SSL_ERROR_WANT_WRITE) || 
	   (errno == EINTR))) { }

    if(rc > 0 && rc != count){		// Successful write but incorrect count!!!
       err = 999;
       IDAM_LOG(UDA_LOG_DEBUG, "writeUdaServerSSL: Inconsistent number of bytes written to socket!\n");
       addIdamError(&idamerrorstack, CODEERRORTYPE, "writeUdaServerSSL", err, "Inconsistent number of bytes written to socket!");
       return -1;
    } else
    if(rc == 0){	// Failed write - connection closed? 
       if (errno != 0) addIdamError(&idamerrorstack, SYSTEMERRORTYPE, "writeUdaServerSSL", errno, "");
       err = 999;
       IDAM_LOG(UDA_LOG_DEBUG, "writeUdaServerSSL: Unable to write to socket! Closed?\n");
       addIdamError(&idamerrorstack, CODEERRORTYPE, "writeUdaServerSSL", err, "Unable to write to socket! Closed?");
       getUdaSSLErrorCode(rc);
       return -1;
    } else 
    if(rc == -1){	// Failed write 
       if (errno != 0) addIdamError(&idamerrorstack, SYSTEMERRORTYPE, "writeUdaServerSSL", errno, "");
       err = 998;
       IDAM_LOG(UDA_LOG_DEBUG, "writeUdaServerSSL: Unable to write to socket! Unknown!\n");
       addIdamError(&idamerrorstack, CODEERRORTYPE, "writeUdaServerSSL", err, "Unable to write to socket!, Unknown!");
       getUdaSSLErrorCode(rc);
       return -1;
    }

    return rc;
}

int readUdaServerSSL(void* iohandle, char* buf, int count)
{

    int rc, err=0;
    fd_set rfds;        // File Descriptor Set for Reading from the Socket
    struct timeval tv, tvc;

    // Wait till it's possible to read from socket

    setSelectParms(getUdaSSLSocket(), &rfds, &tv);
    tvc = tv;

    while (select(getUdaSSLSocket() + 1, &rfds, NULL, NULL, &tvc) <= 0) {
        server_tot_block_time = server_tot_block_time + (int) tv.tv_usec / 1000;
        if (server_tot_block_time > 1000 * server_timeout) {
            IDAM_LOGF(UDA_LOG_DEBUG, "readUdaServerSSL: Total Wait Time Exceeds Lifetime Limit = %d (ms)\n", server_timeout * 1000);
        }

        if (server_tot_block_time > 1000 * server_timeout) return -1;

        updateSelectParms(getUdaSSLSocket(), &rfds, &tv);        // Keep trying ...
        tvc = tv;
    }

// Read from it, checking for EINTR, as happens if called from IDL

    while (((rc = (int) SSL_read(getUdaSSL(), buf, count)) == -1) && 
           (((err = SSL_get_error(getUdaSSL(), rc)) == SSL_ERROR_WANT_READ || err == SSL_ERROR_WANT_WRITE) || 
	   (errno == EINTR))) { }

// As we have waited to be told that there is data to be read, if nothing arrives, then there must be an error

    if(rc == 0){	// Failed read - connection closed? 
       if (errno != 0) addIdamError(&idamerrorstack, SYSTEMERRORTYPE, "readUdaServerSSL", errno, "");
       err = 999;
       IDAM_LOG(UDA_LOG_DEBUG, "readUdaServerSSL: Unable to read from the socket! Closed?\n");
       addIdamError(&idamerrorstack, CODEERRORTYPE, "readUdaServerSSL", err, "No Data waiting at Socket when Data Expected!");
       addIdamError(&idamerrorstack, CODEERRORTYPE, "readUdaServerSSL", err, "Unable to read from the socket! Closed?");
       getUdaSSLErrorCode(rc);
       return -1;
    }else 
    if(rc == -1){	// Failed read 
       if (errno != 0) addIdamError(&idamerrorstack, SYSTEMERRORTYPE, "readUdaServerSSL", errno, "");
       err = 998;
       IDAM_LOG(UDA_LOG_DEBUG, "readUdaServerSSL: Unable to read from the socket! Unknown!\n");
       addIdamError(&idamerrorstack, CODEERRORTYPE, "readUdaServerSSL", err, "No Data waiting at Socket when Data Expected!");
       addIdamError(&idamerrorstack, CODEERRORTYPE, "readUdaServerSSL", err, "Unable to read from the socket!, Unknown!");
       getUdaSSLErrorCode(rc);
       return -1;
    }  

    return rc;
}
#endif   // #ifdef SERVERBUILD
