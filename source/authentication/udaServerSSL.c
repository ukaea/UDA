
#if defined(SSLAUTHENTICATION) && defined(SERVERBUILD)
 
#include <signal.h>

#include <authentication/udaSSL.h>
#include <clientserver/errorLog.h>
#include <logging/logging.h>
#include <server/writer.h>

static int sslDisabled = 0;		// Default state is SSL enabled
static int sslSocket   = -1;
static int sslOK       = 0;		// SSL Authentication has been passed sucessfully: default is NOT Passed
static SSL *ssl        = NULL;
static SSL_CTX *ctx    = NULL;

void putUdaServerSSLOK(int ok){
   sslOK = ok;
   return;
} 
int getUdaServerSSLOK(){
   return sslOK;
}
void putUdaServerSSLDisabled(int disabled){
   sslDisabled = disabled;
   return;
} 
int getUdaServerSSLDisabled(){
   return sslDisabled;
}
void putUdaClientSSLSocket(int s){
   sslSocket = s;
   return;
} 
void putUdaServerSSL(SSL *s){
   ssl = s;
   return;
} 
SSL *getUdaServerSSL(){
   return ssl;
} 

void putUdaServerSSLCTX(SSL_CTX *c){
   ctx = c;
   return;
} 
SSL_CTX *getUdaServerSSLCTX(){
   return ctx;
}

void putUdaServerSSLSocket(int s){
   sslSocket = s;
   return;
} 
int getUdaServerSSLSocket(){
   return sslSocket;
}   

void getUdaServerSSLErrorCode(int rc){
   int err = SSL_get_error(getUdaServerSSL(), rc);            
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
   IDAM_LOGF(UDA_LOG_DEBUG, "udaSSL: Error - %s\n", msg);
   IDAM_LOGF(UDA_LOG_DEBUG, "udaSSL: Error - %s\n", ERR_error_string(ERR_get_error(), NULL));
   IDAM_LOGF(UDA_LOG_DEBUG, "udaSSL: State - %s\n", SSL_state_string(getUdaServerSSL()));
}  

void initUdaServerSSL(){ 
    SSL_library_init();
    SSL_load_error_strings();	
    OpenSSL_add_ssl_algorithms();
}

void closeUdaServerSSL(){
   if(getUdaServerSSLDisabled()) return;
   putUdaServerSSLOK(0);
   putUdaServerSSLSocket(-1);
   putUdaServerSSLDisabled(0);
   SSL_shutdown(getUdaServerSSL());           
   SSL_free(getUdaServerSSL());
   SSL_CTX_free(getUdaServerSSLCTX());
   EVP_cleanup();    
   putUdaServerSSL(NULL);
   putUdaServerSSLCTX(NULL); 
} 

    
SSL_CTX *createUdaServerSSLContext(){
    int err = 0;
    
    const SSL_METHOD *method;
    SSL_CTX *ctx;

    method = SSLv23_server_method();		// standard TCP 
    
    // method = DTLSv1_server_method()		// reliable UDP

    ctx = SSL_CTX_new(method);
    putUdaServerSSLCTX(ctx);
    
    if (!ctx) {
      err = 999;
      addIdamError(&idamerrorstack, CODEERRORTYPE, "udaSSL", err, "Unable to create SSL context");
      IDAM_LOG(UDA_LOG_DEBUG, "createUdaServerSSLContext: Unable to create SSL context!\n");
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

    IDAM_LOG(UDA_LOG_DEBUG, "createUdaServerSSLContext: SSL Context created\n");

    return ctx;
}
        
int configureUdaServerSSLContext(){
   int err = 0;
   
   SSL_CTX *ctx = getUdaServerSSLCTX();    
    
   //SSL_CTX_set_ecdh_auto(ctx, 1);

// Set the key and cert 

   char *cert   = getenv("UDA_SERVER_SSL_CERT");
   char *key    = getenv("UDA_SERVER_SSL_KEY");
   char *ca     = getenv("UDA_CA_SSL_CERT");
   char *crlist = getenv("UDA_CA_SSL_CRL");
   
   if(!cert || !key || !ca || !crlist){
      err = 999;
      if(!cert)   addIdamError(&idamerrorstack, CODEERRORTYPE, "udaSSL", err, "No server SSL certificate!");
      if(!key)    addIdamError(&idamerrorstack, CODEERRORTYPE, "udaSSL", err, "No server SSL key!");
      if(!ca)     addIdamError(&idamerrorstack, CODEERRORTYPE, "udaSSL", err, "No Certificate Authority certificate!");
      if(!crlist) addIdamError(&idamerrorstack, CODEERRORTYPE, "udaSSL", err, "No Certificate Revocation List!");
      IDAM_LOG(UDA_LOG_DEBUG, "configureUdaServerSSLContext: Certificate/Key/CRL environment variable problem!\n");   
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

// Load certificates of trusted CAs   

    if (SSL_CTX_load_verify_locations(ctx, ca, NULL) < 1){
        err = 999;
	addIdamError(&idamerrorstack, CODEERRORTYPE, "udaSSL", err, "Error setting the Cetificate Authority verify locations!");
	return err;
    } 
    
// Peer certificate verification
    
    SSL_CTX_set_verify(ctx, SSL_VERIFY_PEER|SSL_VERIFY_FAIL_IF_NO_PEER_CERT, NULL);
    SSL_CTX_set_verify_depth(ctx, VERIFY_DEPTH); 
    
// Add verification against the Certificate Revocation List

   X509_VERIFY_PARAM *params = X509_VERIFY_PARAM_new();
   X509_VERIFY_PARAM_set_flags(params, X509_V_FLAG_CRL_CHECK); 
   SSL_CTX_set1_param(ctx, params);

   X509_CRL *crl = loadUdaServerSSLCrl(crlist);
   if (!crl) return 999;	// CRL not loaded
   
   STACK_OF(X509_CRL) *crls = sk_X509_CRL_new_null();
   if (!crls || !sk_X509_CRL_push(crls, crl)) {
       err = 999;
       addIdamError(&idamerrorstack, CODEERRORTYPE, "udaSSL", err, "Error loading the CRL for client certificate verification!");
       X509_CRL_free(crl);
       return err;
   }   
    
   X509_STORE *st = SSL_CTX_get_cert_store(ctx);
   addUdaServerSSLCrlsStore(st, crls); 
   SSL_CTX_set1_verify_cert_store(ctx, st);   
   
// Set CA list used for client authentication 

/*
  if(SSL_CTX_use_certificate_chain_file(ctx, getenv("UDA_CA_SSL_CERT")) < 1){
     //printf("Error setting the CA chain file\n");
     exit(0);
  }
*/
/*   
  SSL_CTX_set_client_CA_list(ctx, SSL_load_client_CA_file(getenv("UDA_CA_SSL_CERT"))); 

   rc = load_CA(ssl, ctx, getenv("UDA_CA_SSL_CERT"));	// calls SSL_CTX_add_client_CA(ctx, X509 *cacert) and         SSL_add_client_CA(ssl, X509 *cacert)
   if(rc == 0)fprintf(logout, "Unable to load Client CA!\n");
*/ 

   IDAM_LOG(UDA_LOG_DEBUG, "configureUdaServerSSLContext: SSL Context configured\n");   
             
   return err; 
}

X509_CRL *loadUdaServerSSLCrl(char *crlist){

   // Load the Certificate Revocation Lists for certificate verification

    int err = 0;
    
    BIO *in =  BIO_new(BIO_s_file());
    if (in == NULL) {
        err = 999;
	addIdamError(&idamerrorstack, CODEERRORTYPE, "udaSSL", err, "Error creating a Certificate Revocation List object!");
        return NULL;
    }

    if (BIO_read_filename(in, crlist) <= 0) {
        BIO_free(in);
	err = 999;
	addIdamError(&idamerrorstack, CODEERRORTYPE, "udaSSL", err, "Error opening the Certificate Revocation List file!");
        return NULL;
    }

    X509_CRL *x = PEM_read_bio_X509_CRL(in, NULL, NULL, NULL);
    
    if (x == NULL) {
        BIO_free(in);
	err = 999;
	addIdamError(&idamerrorstack, CODEERRORTYPE, "udaSSL", err, "Error reading the Certificate Revocation List file!");
        return NULL;
    }
 
    BIO_free(in);
    
    IDAM_LOG(UDA_LOG_DEBUG, "loadUdaServerSSLCrl: CRL loaded\n");   

    return (x);
}

int addUdaServerSSLCrlsStore(X509_STORE *st, STACK_OF(X509_CRL) *crls){
   X509_CRL *crl;
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
    X509 *x = NULL;

    if ((in = fopen(file, "r")) == NULL){
        fprintf(logout, "Unable to open the CA certificate file\n");
        return (0);
    }
    for (;;) {
        if (PEM_read_X509(in, &x, 0, NULL) == NULL){
           fprintf(logout, "Unable to read the CA certificate file\n");
           break;
        }
        SSL_CTX_add_client_CA(ctx, x);
	SSL_add_client_CA(ssl, x);
    }

    if (x != NULL) X509_free(x);
    fclose(in);
    
    if(x == NULL) return 0;
    return (1);
}
*/


int startUdaServerSSL(){
   int rc, err = 0;
   SSL_CTX *ctx = NULL;

// Has SSL/TLS authentication already been passed?

   if(getUdaServerSSLOK()) return 0;
   
// Has the server disabled SSL/TLS authentication?

   if(getenv("UDA_SERVER_SSL_DISABLE") != NULL){ 
      putUdaServerSSLDisabled(1);
      return 0;
   } else 
      putUdaServerSSLDisabled(0);      

   
   initUdaServerSSL();
   
   if(!(ctx = createUdaServerSSLContext())){
      err = 999;
      addIdamError(&idamerrorstack, CODEERRORTYPE, "udaSSL", err, "Unable to create the SSL context!");
      return err;
   }   
   if(configureUdaServerSSLContext() != 0){
      err = 999;
      addIdamError(&idamerrorstack, CODEERRORTYPE, "udaSSL", err, "Unable to configure the SSL context!");
      return err;
   }
    
// Bind an SSL object with the socket
     
    SSL *ssl = SSL_new(ctx);
    if((rc = SSL_set_fd(ssl, getUdaServerSSLSocket())) < 1){
       err = 999;
       addIdamError(&idamerrorstack, CODEERRORTYPE, "udaSSL", err, "Unable to bind the socket to SSL!");
       IDAM_LOG(UDA_LOG_DEBUG, "udaSSL: Error - Unable to bind the socket to SSL!\n");
       return err;
    }
    
    putUdaServerSSL(ssl);
        
// SSL Handshake with Client Authentication

   if ((rc = SSL_accept(ssl)) < 1) {      
      if(errno != 0) addIdamError(&idamerrorstack, SYSTEMERRORTYPE, "udaSSL", errno, "SSL Handshake failed!");      
      IDAM_LOG(UDA_LOG_DEBUG, "udaSSL: Error - SSL Handshake Failed!\n");
      err=SSL_get_error(ssl, rc);
      if(err == 5){
         IDAM_LOG(UDA_LOG_DEBUG, "udaSSL: Error - Client application terminated?!\n");
	 addIdamError(&idamerrorstack, CODEERRORTYPE, "udaSSL", err, "SSL error in SSL_accept, application terminated!"); 
      }
      getUdaServerSSLErrorCode(rc);
      return err; 
   }
              
// Get the Client's certificate and verify

   X509 *peer = SSL_get_peer_certificate(ssl); 
   
   if(peer != NULL){

      if ((rc=SSL_get_verify_result(ssl)) != X509_V_OK) {	// returns X509_V_OK if the certificate was not obtained as no error occured!
         err = 999;
	 addIdamError(&idamerrorstack, CODEERRORTYPE, "udaSSL", err, "SSL Client certificate verification error");
	 addIdamError(&idamerrorstack, CODEERRORTYPE, "udaSSL", err, X509_verify_cert_error_string(rc));
	 X509_free(peer);
         return err;
      }

// Client's details

      char work[X509STRINGSIZE];
      IDAM_LOG(UDA_LOG_DEBUG, "udaSSL: Client certificate verified\n");
      IDAM_LOGF(UDA_LOG_DEBUG, "X509 subject: %s\n", X509_NAME_oneline(X509_get_subject_name(peer), work, sizeof(work)));
      IDAM_LOGF(UDA_LOG_DEBUG, "X509 issuer: %s\n", X509_NAME_oneline(X509_get_issuer_name(peer), work, sizeof(work)));
      IDAM_LOGF(UDA_LOG_DEBUG, "X509 not before: %d\n", X509_get_notBefore(peer));
      IDAM_LOGF(UDA_LOG_DEBUG, "X509 not after: %d\n", X509_get_notAfter(peer));
      X509_free(peer);
   } else {
      err = 999;
      addIdamError(&idamerrorstack, CODEERRORTYPE, "udaSSL", err, "Client certificate not presented for verification!");
      X509_free(peer);
      return err;
   }

// Print out connection details 

    IDAM_LOGF(UDA_LOG_DEBUG, "SSL version: %s\n", SSL_get_version(ssl));
    IDAM_LOGF(UDA_LOG_DEBUG, "SSL cipher: %s\n", SSL_get_cipher(ssl));

// SSL/TLS authentication has been passed - do not repeat

   putUdaServerSSLOK(1);

   return 0;
} 

int writeUdaServerSSL(void* iohandle, char* buf, int count)
{

return SSL_write(getUdaServerSSL(), buf, count);


// This routine is only called when there is something to write back to the Client

    int rc, err=0;

    fd_set wfds;        // File Descriptor Set for Writing to the Socket
    struct timeval tv;

    // Block till it's possible to write to the socket

    setSelectParms(getUdaServerSSLSocket(), &wfds, &tv);

    while (select(getUdaServerSSLSocket() + 1, NULL, &wfds, NULL, &tv) <= 0) {
        server_tot_block_time += tv.tv_usec / 1000;
        if (server_tot_block_time / 1000 > server_timeout) {
            IDAM_LOGF(UDA_LOG_DEBUG, "writeUdaServerSSL: Total Blocking Time: %d (ms)\n", server_tot_block_time);
        }
        if (server_tot_block_time / 1000 > server_timeout) return -1;
        updateSelectParms(getUdaServerSSLSocket(), &wfds, &tv);
    }

// Write to socket, checking for EINTR, as happens if called from IDL
// if the return code from SSL_write == -1 and WANT_READ or WANT_WRITE then repeat the write with the same arguments

    while (((rc = (int) SSL_write(getUdaServerSSL(), buf, count)) == -1) && 
           (((err = SSL_get_error(getUdaServerSSL(), rc)) == SSL_ERROR_WANT_READ || err == SSL_ERROR_WANT_WRITE) || 
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
       getUdaServerSSLErrorCode(rc);
       return -1;
    } else 
    if(rc == -1){	// Failed write 
       if (errno != 0) addIdamError(&idamerrorstack, SYSTEMERRORTYPE, "writeUdaServerSSL", errno, "");
       err = 998;
       IDAM_LOG(UDA_LOG_DEBUG, "writeUdaServerSSL: Unable to write to socket! Unknown!\n");
       addIdamError(&idamerrorstack, CODEERRORTYPE, "writeUdaServerSSL", err, "Unable to write to socket!, Unknown!");
       getUdaServerSSLErrorCode(rc);
       return -1;
    }

    return rc;
}

int readUdaServerSSL(void* iohandle, char* buf, int count)
{

return SSL_read(getUdaServerSSL(), buf, count);


    int rc, err=0;
    fd_set rfds;        // File Descriptor Set for Reading from the Socket
    struct timeval tv, tvc;

    // Wait till it's possible to read from socket

    setSelectParms(getUdaServerSSLSocket(), &rfds, &tv);
    tvc = tv;

    while (select(getUdaServerSSLSocket() + 1, &rfds, NULL, NULL, &tvc) <= 0) {
        server_tot_block_time = server_tot_block_time + (int) tv.tv_usec / 1000;
        if (server_tot_block_time > 1000 * server_timeout) {
            IDAM_LOGF(UDA_LOG_DEBUG, "readUdaServerSSL: Total Wait Time Exceeds Lifetime Limit = %d (ms)\n", server_timeout * 1000);
        }

        if (server_tot_block_time > 1000 * server_timeout) return -1;

        updateSelectParms(getUdaServerSSLSocket(), &rfds, &tv);        // Keep trying ...
        tvc = tv;
    }

// Read from it, checking for EINTR, as happens if called from IDL

    while (((rc = (int) SSL_read(getUdaServerSSL(), buf, count)) == -1) && 
           (((err = SSL_get_error(getUdaServerSSL(), rc)) == SSL_ERROR_WANT_READ || err == SSL_ERROR_WANT_WRITE) || 
	   (errno == EINTR))) { }

// As we have waited to be told that there is data to be read, if nothing arrives, then there must be an error

    if(rc == 0){	// Failed read - connection closed? 
       if (errno != 0) addIdamError(&idamerrorstack, SYSTEMERRORTYPE, "readUdaServerSSL", errno, "");
       err = 999;
       IDAM_LOG(UDA_LOG_DEBUG, "readUdaServerSSL: Unable to read from the socket! Closed?\n");
       addIdamError(&idamerrorstack, CODEERRORTYPE, "readUdaServerSSL", err, "No Data waiting at Socket when Data Expected!");
       addIdamError(&idamerrorstack, CODEERRORTYPE, "readUdaServerSSL", err, "Unable to read from the socket! Closed?");
       getUdaServerSSLErrorCode(rc);
       return -1;
    }else 
    if(rc == -1){	// Failed read 
       if (errno != 0) addIdamError(&idamerrorstack, SYSTEMERRORTYPE, "readUdaServerSSL", errno, "");
       err = 998;
       IDAM_LOG(UDA_LOG_DEBUG, "readUdaServerSSL: Unable to read from the socket! Unknown!\n");
       addIdamError(&idamerrorstack, CODEERRORTYPE, "readUdaServerSSL", err, "No Data waiting at Socket when Data Expected!");
       addIdamError(&idamerrorstack, CODEERRORTYPE, "readUdaServerSSL", err, "Unable to read from the socket!, Unknown!");
       getUdaServerSSLErrorCode(rc);
       return -1;
    }  

    return rc;
}

#endif   // SERVERBUILD && SSLAUTHENTICATION 
