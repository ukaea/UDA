#ifndef UDA_SECURITY_SECURITY_H
#define UDA_SECURITY_SECURITY_H

#include <gcrypt.h>

// Limits
#define NONCEBYTELENGTH     50  // System problem when >~ 110 !
#define UDA_MAXKEY 4096

typedef enum TokenType {
    NONCETEST           = 0,
    NONCEWEAKRANDOM     = 1,
    NONCESTRONGRANDOM   = 2,
    NONCESTRINGRANDOM   = 3,
} TOKEN_TYPE;

typedef enum EncryptionMethod {
    NOTKNOWNKEY         = 0,   // Initialisation value
    SHAREDPRIVATEKEY    = 1,
    ASYMMETRICKEY       = 2,   // Default
    DIGITALSIGNATUREKEY = 3,
    DIFFIEHELLMANKEY    = 4,
    CERTIFICATEKEY      = 5,
    NOTREQUIREDKEY      = 6,   // Trusted client so byepass authentication (server initiated only)
} ENCRYPTION_METHOD;

typedef enum AuthenticationStep {
    CLIENT_ISSUE_TOKEN          = 1,
    SERVER_DECRYPT_CLIENT_TOKEN = 2,
    SERVER_ENCRYPT_CLIENT_TOKEN = 3,
    SERVER_ISSUE_TOKEN          = 4,
    CLIENT_DECRYPT_SERVER_TOKEN = 5,
    CLIENT_ENCRYPT_SERVER_TOKEN = 6,
    SERVER_VERIFY_TOKEN         = 7,
    HOUSEKEEPING                = 9,
} AUTHENTICATION_STEP;

int initAuthentication();

int udaAuthentication(AUTHENTICATION_STEP authenticationStep, ENCRYPTION_METHOD encryptionMethod,
                      TOKEN_TYPE tokenType, unsigned short tokenByteLength,
                      gcry_sexp_t publickey, gcry_sexp_t privatekey,
                      gcry_mpi_t* client_mpiToken, gcry_mpi_t* server_mpiToken,
                      unsigned char** client_ciphertext, size_t* client_ciphertextLength,
                      unsigned char** server_ciphertext, size_t* server_ciphertextLength);

#endif // UDA_SECURITY_SECURITY_H
