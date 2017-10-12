#ifndef UDA_X509UTILS_H
#define UDA_X509UTILS_H

#include <ksba.h>
#include <gcrypt.h>

int makeX509CertObject(unsigned char* doc, unsigned short docLength, ksba_cert_t* cert);
int testX509Dates(ksba_cert_t certificate);
int extractX509SExpKey(ksba_cert_t cert, gcry_sexp_t* key_sexp);
int importPEMPrivateKey(const char* keyFile, gcry_sexp_t* key_sexp);
int importX509Reader(const char* fileName, ksba_cert_t* cert);
int checkX509Signature(ksba_cert_t issuer_cert, ksba_cert_t cert);
int importSecurityDoc(const char* file, unsigned char** contents, unsigned short* length);
int importPEMPublicKey(char* keyFile, gcry_sexp_t* key_sexp);

#endif //UDA_X509UTILS_H
