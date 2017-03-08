#ifndef UDA_SECURITY_SECURITY_H
#define UDA_SECURITY_SECURITY_H

#include <ksba.h>
#include <gcrypt.h>
#include <clientserver/udaStructs.h>

#define NOTKNOWNKEY         0   // Initialisation value
#define SHAREDPRIVATEKEY    1
#define ASYMMETRICKEY       2   // Default
#define DIGITALSIGNATUREKEY 3
#define DIFFIEHELLMANKEY    4
#define CERTIFICATEKEY      5
#define NOTREQUIREDKEY      6   // Trusted client so byepass authentication (server initiated only)

// Nonce Generation Methods

#define NONCEBYTELENGTH     50  // System problem when >~ 110 !

#define NONCETEST           0
#define NONCEWEAKRANDOM     1
#define NONCESTRONGRANDOM   2
#define NONCESTRINGRANDOM   3

#define HASH_FNC ((void(*)(void *, const void *, size_t))gcry_md_write)
#define DIM(v) (sizeof(v)/sizeof((v)[0]))
#define xtrymalloc(a) gcry_malloc ((a))

// Limits

#define UDA_MAXKEY 4096

enum AuthenticationStep {
    CLIENT_ISSUE_TOKEN = 1,
    SERVER_DECRYPT_CLIENT_TOKEN = 2,
    SERVER_ENCRYPT_CLIENT_TOKEN = 3,
    SERVER_ISSUE_TOKEN = 4,
    CLIENT_DECRYPT_SERVER_TOKEN = 5,
    CLIENT_ENCRYPT_SERVER_TOKEN = 6,
    SERVER_VERIFY_TOKEN = 7,
    HOUSEKEEPING = 9,
};

int makeX509CertObject(unsigned char* doc, unsigned short docLength, ksba_cert_t* cert);
int testX509Dates(ksba_cert_t certificate);
int extractX509SExpKey(ksba_cert_t cert, gcry_sexp_t* key_sexp);
int importPEMPrivateKey(const char* keyFile, gcry_sexp_t* key_sexp);
int importX509Reader(const char* fileName, ksba_cert_t* cert);
int checkX509Signature(ksba_cert_t issuer_cert, ksba_cert_t cert);

int udaAuthentication(unsigned short authenticationStep, unsigned short encryptionMethod,
                      unsigned short tokenType, unsigned short tokenByteLength,
                      gcry_sexp_t publickey, gcry_sexp_t privatekey,
                      gcry_mpi_t* client_mpiToken, gcry_mpi_t* server_mpiToken,
                      unsigned char** client_ciphertext, size_t* client_ciphertextLength,
                      unsigned char** server_ciphertext, size_t* server_ciphertextLength);

int importSecurityDoc(const char* file, unsigned char** contents, unsigned short* length);
int importPEMPublicKey(char* keyFile, gcry_sexp_t* key_sexp);

#endif // UDA_SECURITY_SECURITY_H
