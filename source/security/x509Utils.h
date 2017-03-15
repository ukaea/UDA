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

typedef struct DistinguishedName {
    char* emailAddress;
    char* commonName;
    char* organisationalUnitName;
    char* organisationName;
    char* localityName;
    char* countryName;
} DISTINGUISHED_NAME;

/**
 * Unpack the distinguished name string and return a structure containing the elements contained within.
 * @param dn_string the distinguished name as a string in accordance with RFC-2253.
 * @return
 */
DISTINGUISHED_NAME unpackDistinguishedName(const char* dn_string);

void printDistinguishedName(const DISTINGUISHED_NAME* dn);

void destroyDistinguishedName(DISTINGUISHED_NAME* dn);

#endif //UDA_X509UTILS_H
