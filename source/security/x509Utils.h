#ifndef UDA_X509UTILS_H
#define UDA_X509UTILS_H

#include <gcrypt.h>
#include <ksba.h>

#include "export.h"

#ifdef __cplusplus
extern "C" {
#endif

LIBRARY_API int makeX509CertObject(unsigned char* doc, unsigned short docLength, ksba_cert_t* cert);
LIBRARY_API int testX509Dates(ksba_cert_t certificate);
LIBRARY_API int extractX509SExpKey(ksba_cert_t cert, gcry_sexp_t* key_sexp);
LIBRARY_API int importPEMPrivateKey(const char* keyFile, gcry_sexp_t* key_sexp);
LIBRARY_API int importX509Reader(const char* fileName, ksba_cert_t* cert);
LIBRARY_API int checkX509Signature(ksba_cert_t issuer_cert, ksba_cert_t cert);
LIBRARY_API int importSecurityDoc(const char* file, unsigned char** contents, unsigned short* length);
LIBRARY_API int importPEMPublicKey(char* keyFile, gcry_sexp_t* key_sexp);

#ifdef __cplusplus
}
#endif

#endif // UDA_X509UTILS_H
