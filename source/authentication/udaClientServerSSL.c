
#if defined(SSLAUTHENTICATION)

#include <authentication/udaSSL.h>
#include <clientserver/errorLog.h>
#include <logging/logging.h>

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
   IDAM_LOGF(UDA_LOG_DEBUG, "udaSSL: Error - %s\n", msg);
   IDAM_LOGF(UDA_LOG_DEBUG, "udaSSL: Error - %s\n", ERR_error_string(ERR_get_error(), NULL));
   IDAM_LOGF(UDA_LOG_DEBUG, "udaSSL: State - %s\n", SSL_state_string(getUdaSSL()));
}  

void initUdaSSL(){ 
    SSL_library_init();
    SSL_load_error_strings();	
    OpenSSL_add_ssl_algorithms();
}

void closeUdaSSL(){
   SSL_shutdown(getUdaSSL());           
   SSL_free(getUdaSSL());
   SSL_CTX_free(getUdaSSLCTX());
   EVP_cleanup();    
   putUdaSSL(NULL);
   putUdaSSLCTX(NULL); 
} 

#endif   // SSLAUTHENTICATION
