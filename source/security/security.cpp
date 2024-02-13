
// IDAM Client Server Identity Authentication
//
// Architecture models: (single/multiple tier network; mesh or bridge network)
//
// a)    client connects to a server
// b)    client connects to a proxy that connects to a server
// c)    client connects to a server that connects to a server etc. (multi-tier connection)
// d)    client connects to a proxy that connects to a server that connects to a server etc. (multi-tier connection)
//
// IDAM servers without a public key server component must use PKI (Public Key Infrastructure) X.509 based certificates.
// X.509 certificates establish the authenticity of the binding between a public key and its owner through a
// digital signature created using the private key of a trusted third party - a Certificate Authority.
// *** libksba *** to create and parse X.509 certificates (can self sign!)
// Use linux CLI openssl to create certificates [http://www.openssl.org/]
// Authenticate the client and/or server involves checking that their certificate was signed by an acceptable authority
// (Signing means the public key was generated by an authenticated person. They need the private key to complete
// the authentication process.)
// The certificate contains the user's public key. Use the CA public key to test the
// signature => proof the certificate is valid. Servers only need the CA public keys to assist in authentication.
//
// Web based access requires HTTPS - based on Transport Layer Security (TLS)
// [http://en.wikipedia.org/wiki/Transport_Layer_Security] and X.509 (SSL) certificates for public keys.
// *** http://www.gnutls.org/ ***
//
// Options:
//
// a) Mutual athentication for 2 claims of identity (the server may choose not to authenticate)
// b) Proxy does not authenticate and passes through the claim of identity to the next server as a).
// c) Option:
//    1) The User has one identity. Intermediate servers don't authenticate with each other or the client and
//         pass through the claim of identity to the final server. Authentication occurs between the final server
//         and the user.
//    2) The User has one identity. Intermediate servers authenticate with each other but don't authenticate
//         with the client. They pass through the claim of identity to the final server. Authentication occurs
//         between the final server and the user.
//    3) The User has one identity. All servers authenticate with the client.  **** Not adopted ****
//    4) The User has two identities. The first server authenticates with the first user identity.
//         Intermediate servers don't authenticate with each other or the client and
//         pass through the claim of identity to the final server. Authentication occurs between the final server
//         and the user (second identity).
//    5) The User has two identities. The first server authenticates with the first user identity.
//         Intermediate servers authenticate with each other but don't authenticate with the client. They
//         pass through the second claim of identity to the final server. Authentication occurs between the final server
//         and the user (second identity).
//    6) The User has n identities. Each of n servers authenticates with the n user identities. **** Not adopted ****
// d) Proxy does not authenticate and passes through the claims of identity to the multiple servers as c).
//
// Steps:
//
// 1>     Client issues a token (A), encrypts with the server's public key (->EASP), passes to server (with X.509)
//    Server's public key could be obtained from a X.509 certificate (authenticated using signature and CA public key)
// 2>    Server decrypts the passed cipher (EASP) with the server's private key (->A)
// 3>    Server encrypts the client token (A) with the client's public key (->EACP)
//    Public key could be obtained from a X.509 certificate (authenticated using signature and CA public key)
//    Public key could alternatively be obtained a user database.
// 4>    Server issues a new token (B) also encrypted with the client's public key (->EBCP), passes
//    both to client.
// 5>    Client decrypts the passed ciphers (EACP, EBCP) with the client's private key (->A, ->B) and
//    checks token (A) => server authenticated (in addition to the X.509 certificate signature check)
// 6>    Client encrypts passed token (B) with the server's public key (->EBSP), passes to server
// 7>    Server decrypts the passed cipher (EBSP) with the server's private key (->B) and checks
//    token (B) => client authenticated (in addition to the X.509 certificate signature check).
//
// 8>   Server issues a new token (B) encrypted with the client's public key (->EBCP), passes to client.
//
// All further communication between client and server are based on steps 7, 8 to ensure continuation of
// authorised data access. (X.509 certificate signature check now unneccesary)
//
// 1> Client data access request includes the encrypted token (B)
// 2> Server data (unencrypted) returned with new encrypted token + encrypted data checksum
//
// Authentication cost is 3 RTT, incurred before the first data request is processed by the server.
//
// Authentication is completed during the first data request. No additional security based TCP/IP exchanges
// are required thereafter.
//
// Server closedown should be avoided to maintain state.
//
// **** The server could issue a session ID in case of lost connections to speed up access. This ID could
// have a short lifetime ~ 1 mins, sufficient to re-establish the connection at no extra cost.
// The last issued ID could serve as both the session ID and the authentication token.
//
//--------------------------------------------------------------------------------------------------------------------
#include "security.h"

#include <zconf.h>

#include "clientserver/errorLog.h"
#include "logging/logging.h"
#include <stdbool.h>
#include <uda/types.h>

static void logToken(const char* msg, const gcry_mpi_t mpi_token)
{
    unsigned char* token = nullptr;
    size_t tokenLength = 0;

    gcry_mpi_aprint(GCRYMPI_FMT_HEX, &token, &tokenLength, mpi_token);
    UDA_LOG(UDA_LOG_DEBUG, "%s MPI [%d] %s\n", msg, tokenLength, token);
    free(token);
}

/**
 * Create a Multi-Precision Integer (MPI) token message (session ID)
 * @param tokenType
 * @param tokenByteLength
 * @param mpiToken
 * @return
 */
static int createMPIToken(unsigned short tokenType, unsigned short tokenByteLength, gcry_mpi_t* mpiToken)
{
    int err = 0;

    switch (tokenType) {
        // Standard Test Message
        case NONCETEST: {
            const char* txt = "QWERTYqwerty0123456789";
            if (gcry_mpi_scan(mpiToken, GCRYMPI_FMT_USG, txt, strlen(txt), nullptr) != 0) {
                UDA_THROW_ERROR(999, "Unable to generate MPI Token");
            }
        } break;

            // Random bits
        case NONCEWEAKRANDOM: {
            *mpiToken = gcry_mpi_new((unsigned int)tokenByteLength * 8);
            gcry_mpi_randomize(*mpiToken, (unsigned int)tokenByteLength * 8, GCRY_WEAK_RANDOM);
            if (*mpiToken == nullptr) {
                UDA_THROW_ERROR(999, "Unable to generate MPI Token");
            }

        } break;

            // Random bits
        case NONCESTRONGRANDOM: {
            *mpiToken = gcry_mpi_new((unsigned int)tokenByteLength * 8);
            gcry_mpi_randomize(*mpiToken, (unsigned int)tokenByteLength * 8, GCRY_STRONG_RANDOM);
            if (*mpiToken == nullptr) {
                UDA_THROW_ERROR(999, "Unable to generate MPI Token");
            }
        } break;

            // Random String
        case NONCESTRINGRANDOM: {

            // Get the current Process ID
            unsigned int pid = (unsigned int)getpid();

            // Get the current time and convert to a string
            unsigned long long t = (unsigned long long)time(nullptr);
            char timeList[256]; // Overestimate of the maximum possible size of a long long integer
            sprintf(timeList, "%llu", t);
            size_t timeLength = strlen(timeList);

            // Seed the (poor) system random number generator with the process ID and create a random string
            unsigned char* randList = (unsigned char*)malloc(tokenByteLength * sizeof(unsigned char));
            srand(pid); // Seed the random number generator
            for (int i = 0; i < tokenByteLength; i++) {
                // quasi-random integers in the range 1-255
                randList[i] = (unsigned char)(1 + (int)(255.0 * (rand() / (RAND_MAX + 1.0))));
            }

            // Create an MPI from both the time and the quasi-random list
            gcry_mpi_t timeData;
            if (gcry_mpi_scan(&timeData, GCRYMPI_FMT_USG, timeList, timeLength, nullptr) != 0) {
                free(randList);
                UDA_THROW_ERROR(999, "Unable to generate MPI Token");
            }

            gcry_mpi_t randData;
            if (gcry_mpi_scan(&randData, GCRYMPI_FMT_USG, randList, tokenByteLength, nullptr) != 0) {
                gcry_mpi_release(timeData);
                free(randList);
                UDA_THROW_ERROR(999, "Unable to generate MPI Token");
            }

            free(randList);

            // Multiply to generate a token
            *mpiToken = gcry_mpi_new(0);
            gcry_mpi_mul(*mpiToken, timeData, randData);

            if (*mpiToken == nullptr) {
                gcry_mpi_release(timeData);
                gcry_mpi_release(randData);
                UDA_THROW_ERROR(999, "Unable to generate MPI Token");
            }

            gcry_mpi_release(timeData);
            gcry_mpi_release(randData);

            UDA_LOG(UDA_LOG_DEBUG, "pid  = %u\n", pid);
            UDA_LOG(UDA_LOG_DEBUG, "time = %u\n", t);

            return 0;
        }

        default:
            UDA_THROW_ERROR(999, "Unknown token type");
    }

    return err;
}

/**
 * Given an S-expression ENCR_DATA of the form:
 *      (enc-val
 *       (rsa
 *        (a a-value)))
 * as returned by gcry_pk_decrypt, return the the A-VALUE. On error, return nullptr.
 * @param encr_data
 * @return
 */
static gcry_mpi_t extract_a_from_sexp(gcry_sexp_t encr_data)
{

    gcry_sexp_t l1, l2, l3;
    gcry_mpi_t a_value;

    l1 = gcry_sexp_find_token(encr_data, "enc-val", 0);
    if (!l1) {
        return nullptr;
    }
    l2 = gcry_sexp_find_token(l1, "rsa", 0);
    gcry_sexp_release(l1);
    if (!l2) {
        return nullptr;
    }
    l3 = gcry_sexp_find_token(l2, "a", 0);
    gcry_sexp_release(l2);
    if (!l3) {
        return nullptr;
    }
    a_value = gcry_sexp_nth_mpi(l3, 1, 0);
    gcry_sexp_release(l3);

    return a_value;
}

static int generateToken(gcry_mpi_t* mpi_token, unsigned short tokenType, unsigned short tokenByteLength)
{
    int err = 0;

    if (*mpi_token != nullptr) {
        gcry_mpi_release(*mpi_token);
        *mpi_token = nullptr;
    }

    err = createMPIToken(tokenType, tokenByteLength, mpi_token);

    if (err != 0 || *mpi_token == nullptr) {
        if (*mpi_token != nullptr) {
            gcry_mpi_release(*mpi_token);
            *mpi_token = nullptr;
        }
        UDA_THROW_ERROR(err, "Error Generating Token");
    }

    logToken("Generated", *mpi_token);

    return err;
}

static int encryptToken(gcry_mpi_t* mpi_token, unsigned short encryptionMethod, gcry_sexp_t key,
                        unsigned char** ciphertext, size_t* ciphertext_len)
{
    int err = 0;

    gcry_sexp_t mpiTokenSexp = nullptr; // Token as a S-Expression

    gpg_error_t gerr;

    if ((gerr = gcry_sexp_build(&mpiTokenSexp, nullptr, "(data (flags raw) (value %m))", *mpi_token)) != 0) {
        gcry_mpi_release(*mpi_token);
        *mpi_token = nullptr;
        if (mpiTokenSexp != nullptr) {
            gcry_sexp_release(mpiTokenSexp);
            mpiTokenSexp = nullptr;
        }
        add_error(UDA_CODE_ERROR_TYPE, __func__, 999, "Error Generating Token S-Exp");
        UDA_THROW_ERROR(999, gpg_strerror(gerr));
    }

    switch (encryptionMethod) {
        case (ASYMMETRICKEY): {
            gcry_sexp_t encr = nullptr; // Encrypted token

            // Encrypt
            if ((gerr = gcry_pk_encrypt(&encr, mpiTokenSexp, key)) != 0) {
                if (mpiTokenSexp != nullptr) {
                    gcry_sexp_release(mpiTokenSexp);
                }
                add_error(UDA_CODE_ERROR_TYPE, __func__, 999, "Encryption Error");
                UDA_THROW_ERROR(999, gpg_strerror(gerr));
            }

            if (mpiTokenSexp != nullptr) {
                gcry_sexp_release(mpiTokenSexp);
            }

            gcry_mpi_t encrypted_token = nullptr; // MPI in encrypted form

            // Extract the ciphertext from the S-expression
            if ((encrypted_token = extract_a_from_sexp(encr)) == nullptr) {
                if (encr != nullptr) {
                    gcry_sexp_release(encr);
                }
                UDA_THROW_ERROR(999, "Poor Encryption");
            }

            // Check the ciphertext does not match the plaintext
            if (!gcry_mpi_cmp(*mpi_token, encrypted_token)) {
                if (encrypted_token != nullptr) {
                    gcry_mpi_release(encrypted_token);
                }
                UDA_THROW_ERROR(999, "Poor Encryption");
            }

            // Return the ciphertext
            *ciphertext_len = gcry_sexp_sprint(encr, GCRYSEXP_FMT_DEFAULT, nullptr, 0);

            if (*ciphertext_len == 0) {
                if (encr != nullptr) {
                    gcry_sexp_release(encr);
                }
                if (encrypted_token != nullptr) {
                    gcry_mpi_release(encrypted_token);
                }
                *ciphertext = nullptr;
                UDA_THROW_ERROR(999, "Ciphertext extraction error");
            }

            *ciphertext = (unsigned char*)malloc(*ciphertext_len * sizeof(unsigned char));
            gcry_sexp_sprint(encr, GCRYSEXP_FMT_DEFAULT, *ciphertext, *ciphertext_len);

            if (*ciphertext == nullptr) {
                if (encr != nullptr) {
                    gcry_sexp_release(encr);
                }
                if (encrypted_token != nullptr) {
                    gcry_mpi_release(encrypted_token);
                }
                UDA_THROW_ERROR(999, "Ciphertext extraction error");
            }

            logToken("Encrypted", encrypted_token);

            if (encrypted_token != nullptr) {
                gcry_mpi_release(encrypted_token);
            }

            break;
        }

        default:
            UDA_THROW_ERROR(999, "Unknown encryption method");
    }

    return err;
}

static int decryptToken(gcry_mpi_t* mpi_token, gcry_sexp_t key, unsigned char** ciphertext, size_t* ciphertext_len)
{
    int err = 0;

    // Create S-Expression from the passed ciphertext and decrypt

    gpg_error_t gerr = 0;
    gcry_sexp_t encr = nullptr; // Encrypted token
    gcry_sexp_t decr = nullptr; // Decrypted token

    if ((gerr = gcry_sexp_create(&encr, (void*)*ciphertext, *ciphertext_len, 1, nullptr)) != 0) {
        add_error(UDA_CODE_ERROR_TYPE, __func__, 999, "Error Generating Token S-Exp");
        UDA_THROW_ERROR(999, (char*)gpg_strerror(gerr));
    }

    if ((gerr = gcry_pk_decrypt(&decr, encr, key)) != 0) {
        gcry_sexp_release(encr);
        add_error(UDA_CODE_ERROR_TYPE, __func__, 999, "Decryption Error");
        UDA_THROW_ERROR(999, (char*)gpg_strerror(gerr));
    }

    gcry_sexp_release(encr);

    // Extract the decrypted data from the S-expression.
    gcry_sexp_t tmplist = gcry_sexp_find_token(decr, "value", 0);

    gcry_mpi_t plaintext = nullptr;

    if (tmplist) {
        plaintext = gcry_sexp_nth_mpi(tmplist, 1, GCRYMPI_FMT_USG);
        gcry_sexp_release(tmplist);
    } else {
        plaintext = gcry_sexp_nth_mpi(decr, 0, GCRYMPI_FMT_USG);
    }

    gcry_sexp_release(decr);

    if (!plaintext) {
        UDA_THROW_ERROR(999, "S-Exp contains no plaintext!");
    }

    logToken("Decrypted", plaintext);

    // Return the Client MPI token
    *mpi_token = plaintext;

    return err;
}

int udaAuthentication(AUTHENTICATION_STEP authenticationStep, ENCRYPTION_METHOD encryptionMethod, TOKEN_TYPE tokenType,
                      unsigned short tokenByteLength, gcry_sexp_t publickey, gcry_sexp_t privatekey,
                      gcry_mpi_t* client_mpiToken, gcry_mpi_t* server_mpiToken, unsigned char** client_ciphertext,
                      size_t* client_ciphertextLength, unsigned char** server_ciphertext,
                      size_t* server_ciphertextLength)
{

    int err = 0;

    // Initialise the library

    static bool initialised = false;

    if (!initialised) {
        // Check version of runtime gcrypt library.
        if (!gcry_check_version(GCRYPT_VERSION)) {
            UDA_THROW_ERROR(999, "Library version incorrect!");
        }

        // Disable secure memory.
        gcry_control(GCRYCTL_DISABLE_SECMEM, 0);

        // Tell Libgcrypt that initialization has completed.
        gcry_control(GCRYCTL_INITIALIZATION_FINISHED, 0);

        initialised = true;
    }

    //--------------------------------------------------------------------------------------------------------------------
    // Authentication Steps

    static gcry_mpi_t mpiTokenA = nullptr; // Token passed from the client to the server (preserve for comparison)
    static gcry_mpi_t mpiTokenB = nullptr; // Token passed from the server to the client (preserve for comparison)

    UDA_LOG(UDA_LOG_DEBUG, "Step %d\n", authenticationStep);

    switch (authenticationStep) {
        case CLIENT_ISSUE_TOKEN: { // Client issues a token (A), encrypts with the server's public key (EASP), passes to
                                   // server
            err = generateToken(&mpiTokenA, tokenType, tokenByteLength);
            if (err != 0) {
                break;
            }
            err = encryptToken(&mpiTokenA, encryptionMethod, publickey, client_ciphertext, client_ciphertextLength);
            break;
        }

        case SERVER_DECRYPT_CLIENT_TOKEN: { // Server decrypts the passed cipher (EASP) with the server's private key
                                            // (A)
            err = decryptToken(client_mpiToken, privatekey, client_ciphertext, client_ciphertextLength);
            break;
        }

        case SERVER_ENCRYPT_CLIENT_TOKEN: { // Server encrypts the client token (A) with the client's public key (EACP)
            err =
                encryptToken(client_mpiToken, encryptionMethod, publickey, client_ciphertext, client_ciphertextLength);
            break;
        }

        case SERVER_ISSUE_TOKEN: { // Server issues a new token (B) encrypted with the client's public key (EBCP),
                                   // passes both to client
            err = generateToken(&mpiTokenB, tokenType, tokenByteLength);
            if (err != 0) {
                break;
            }
            err = encryptToken(&mpiTokenB, encryptionMethod, publickey, server_ciphertext, server_ciphertextLength);
            break;
        }

        case CLIENT_DECRYPT_SERVER_TOKEN: { // Client decrypts the passed ciphers (EACP, EBCP) with the client's private
                                            // key (A, B)
            gcry_mpi_t received_token = nullptr;
            err = decryptToken(&received_token, privatekey, client_ciphertext, client_ciphertextLength);
            if (err != 0) {
                break;
            }

            // Check that the decrypted token matches the original token.
            if (gcry_mpi_cmp(mpiTokenA, received_token)) {
                gcry_mpi_release(received_token);
                UDA_THROW_ERROR(999, "Server Authentication Failed!");
            }

            gcry_mpi_release(received_token);

            // Server has been authenticated so delete the original token A - no longer required.
            gcry_mpi_release(mpiTokenA);
            mpiTokenA = nullptr;

            err = decryptToken(server_mpiToken, privatekey, server_ciphertext, server_ciphertextLength);
            break;
        }

        case CLIENT_ENCRYPT_SERVER_TOKEN: { // Client encrypts passed token (B) with the server's public key (EBSP),
                                            // passes to server
            err =
                encryptToken(server_mpiToken, encryptionMethod, publickey, server_ciphertext, server_ciphertextLength);
            break;
        }

        case SERVER_VERIFY_TOKEN: { // Server decrypts the passed cipher (EBSP) with the server's private key (B)
            gcry_mpi_t received_token = nullptr;
            err = decryptToken(&received_token, privatekey, server_ciphertext, server_ciphertextLength);

            // Check that the decrypted token matches the original token.
            if (gcry_mpi_cmp(mpiTokenB, received_token)) {
                gcry_mpi_release(received_token);
                UDA_THROW_ERROR(999, "Client Authentication Failed!");
            }

            gcry_mpi_release(received_token);

            // Client has been authenticated so delete the original token B - no longer required.
            gcry_mpi_release(mpiTokenB);
            mpiTokenB = nullptr;
            break;
        }

        default: {
            UDA_THROW_ERROR(999, "Uknown User Authentication Step");
        }
    }

    return err;
}
