#include "clientAuthentication.h"

#include <clientserver/errorLog.h>
#include <clientserver/protocol.h>
#include <clientserver/printStructs.h>
#include <logging/logging.h>
#ifndef TESTIDAMSECURITY
#  include <clientserver/xdrlib.h>
#  include <clientserver/udaErrors.h>
#  include <client/udaClient.h>
#endif

#include "authenticationUtils.h"
#include "x509Utils.h"

static ENCRYPTION_METHOD encryptionMethod = ASYMMETRICKEY;
static unsigned short tokenByteLength = NONCEBYTELENGTH;        // System problem when >~ 110 !
static TOKEN_TYPE tokenType = NONCESTRONGRANDOM; // NONCESTRONGRANDOM NONCESTRINGRANDOM NONCEWEAKRANDOM NONCETEST; //

/**
 * Read the User's Private Key (from a PEM format file) and the Server's Public Key (from a DER format x509 cert)
 * Convert keys from PEM to S-Expressions representation.
 *
 * Key locations are identified from an environment variable
 * Server public key are from a x509 certificate (to check date validity - a key file isn't sufficient)
 */
static int initialiseKeys(CLIENT_BLOCK* client_block, gcry_sexp_t* publickey_out, gcry_sexp_t* privatekey_out)
{
    SECURITY_BLOCK* securityBlock = nullptr;

    static gcry_sexp_t privatekey = nullptr;    // Client's private key - maintain state for future en/decryption
    static gcry_sexp_t publickey = nullptr;    // Server's public key

    static short initialised = FALSE;    // Input keys and certificates at startup

    if (initialised) {
        *privatekey_out = privatekey;
        *publickey_out = publickey;
        return 0;
    }

    char* env = nullptr;
    size_t len = 0;

    char* clientPrivateKeyFile = nullptr;
    char* serverPublicKeyFile = nullptr;
    char* clientX509File = nullptr;      // Authentication with the first server in a server chain
    char* client2X509File = nullptr;     // Delivered to the final host in a server chain where the data resides
    char* serverX509File = nullptr;      // Certificate of the first server host

    if ((env = getenv("UDA_CLIENT_CERTIFICATE")) != nullptr) {    // Directory with certificates and key files
        len = strlen(env) + 56;
        clientPrivateKeyFile = (char*)malloc(len * sizeof(unsigned char));
        serverPublicKeyFile = (char*)malloc(len * sizeof(unsigned char));
        clientX509File = (char*)malloc(len * sizeof(unsigned char));
        serverX509File = (char*)malloc(len * sizeof(unsigned char));

        sprintf(clientPrivateKeyFile, "%s/clientskey.pem", env);    // Client's
        sprintf(serverPublicKeyFile, "%s/serverpkey.pem", env);    // Server's
        sprintf(clientX509File, "%s/clientX509.der", env);
        sprintf(serverX509File, "%s/serverX509.der", env);
    } else {
        char* home = getenv("HOME");
        len = 256 + strlen(home);

        clientPrivateKeyFile = (char*)malloc(len * sizeof(unsigned char));
        serverPublicKeyFile = (char*)malloc(len * sizeof(unsigned char));
        clientX509File = (char*)malloc(len * sizeof(unsigned char));
        serverX509File = (char*)malloc(len * sizeof(unsigned char));

        sprintf(clientPrivateKeyFile, "%s/.UDA/client/clientskey.pem", home);
        sprintf(serverPublicKeyFile, "%s/.UDA/client/serverpkey.pem", home);
        sprintf(clientX509File, "%s/.UDA/client/clientX509.der", home);
        sprintf(serverX509File, "%s/.UDA/client/serverX509.der", home);
    }

    if ((env = getenv("UDA_CLIENT2_CERTIFICATE")) != nullptr) {
        // X509 certificate to authenticate with the final server in a chain
        len = strlen(env) + 56;
        client2X509File = (char*)malloc(len * sizeof(unsigned char));
        sprintf(client2X509File, "%s/client/client2X509.der", env);
    }

    ksba_cert_t clientCert = nullptr;
    ksba_cert_t client2Cert = nullptr;
    ksba_cert_t serverCert = nullptr;
    int err = 0;

// Read the Client's certificates and check date validity

    securityBlock = &client_block->securityBlock;
    initSecurityBlock(securityBlock);

    if (clientX509File != nullptr) {
        if ((err = importSecurityDoc(clientX509File, &securityBlock->client_X509,
                                     &securityBlock->client_X509Length)) != 0) {
            return err;
        }
        if ((err = makeX509CertObject(securityBlock->client_X509, securityBlock->client_X509Length,
                                      &clientCert)) != 0) {
            return err;
        }
        if ((err = testX509Dates(clientCert)) != 0) {
            return err;
        }
        ksba_cert_release(clientCert);
        clientCert = nullptr;
    }

    if (client2X509File != nullptr) {
        if ((err = importSecurityDoc(client2X509File, &securityBlock->client2_X509,
                                     &securityBlock->client2_X509Length)) != 0) {
            return err;
        }
        if ((err = makeX509CertObject(securityBlock->client2_X509, securityBlock->client2_X509Length,
                                      &client2Cert)) != 0) {
            return err;
        }
        if ((err = testX509Dates(client2Cert)) != 0) {
            return err;
        }
        ksba_cert_release(client2Cert);
        client2Cert = nullptr;
    }

// Test the private key file and its directory directory have permissions set to owner read only

    if ((err = testFilePermissions(clientPrivateKeyFile)) != 0) {
        return err;
    }

    if (client2X509File != nullptr && (err = testFilePermissions(client2X509File)) != 0) {
        return err;
    }

    if ((env = getenv("UDA_CLIENT_CERTIFICATE")) != nullptr) {
        if ((err = testFilePermissions(env)) != 0) {
            return err;
        }
    } else {
        const char* home = getenv("HOME");
        char work[1024];
        sprintf(work, "%s/.UDA/client", home);
        if ((err = testFilePermissions(work)) != 0) {
            return err;
        }
    }

    // get the user's Private key from a PEM file (for decryption) and convert to S-Expression

    if ((err = importPEMPrivateKey(clientPrivateKeyFile, &privatekey)) != 0) {
        return err;
    }

    // get the server's Public key (for encryption of exchanged tokens) from a certificate or a file
    // If from a PEM file, convert to S-Expression

    if (serverX509File != nullptr) {
        unsigned char* serverCertificate;        // Server's X509 authentication certificate
        unsigned short serverCertificateLength;

        if ((err = importSecurityDoc(serverX509File, &serverCertificate, &serverCertificateLength)) != 0) {
            return err;
        }
        if ((err = makeX509CertObject(serverCertificate, serverCertificateLength, &serverCert)) != 0) {
            return err;
        }
        if ((err = testX509Dates(serverCert)) != 0) {
            return err;
        }
        if ((err = extractX509SExpKey(serverCert, &publickey)) != 0) {
            return err;
        }        // get the server's Public key from an X509 certificate
        ksba_cert_release(serverCert);
        serverCert = nullptr;
        free(serverCertificate);
        serverCertificate = nullptr;
    } else if ((err = importPEMPublicKey(serverPublicKeyFile, &publickey)) != 0) {
        return err;
    }        // get the server's Public key from a file

    // Test the user's private key for consistency
    // User keys also have a lifetime - automatically checked if there is a x509 certificate
    // Stale keys must be renewed by a utility (separate system) requiring strong authentication - proof of identity
    // Server may also renew it's public key at that time (may be different for each user!)

    if (gcry_pk_testkey(privatekey) != 0) {
        err = 999;
        addIdamError(UDA_CODE_ERROR_TYPE, "clientAuthentication", err,
                     "The User's Private Authentication Key is Invalid!");
        return err;
    }

    free(clientPrivateKeyFile);
    free(serverPublicKeyFile);
    free(clientX509File);
    free(serverX509File);
    free(client2X509File);

    if (err != 0) {
        free(securityBlock->client_X509);
        free(securityBlock->client2_X509);

        securityBlock->client_X509 = nullptr;
        securityBlock->client2_X509 = nullptr;
        securityBlock->client_X509Length = 0;
        securityBlock->client2_X509Length = 0;

        if (clientCert != nullptr) ksba_cert_release(clientCert);
        if (client2Cert != nullptr) ksba_cert_release(client2Cert);
        if (serverCert != nullptr) ksba_cert_release(serverCert);
        clientCert = nullptr;
        client2Cert = nullptr;
        serverCert = nullptr;

        if (privatekey != nullptr) gcry_sexp_release(privatekey);
        if (publickey != nullptr) gcry_sexp_release(publickey);
        privatekey = nullptr;    // These are declared as static so ensure they are reset when an error occurs
        publickey = nullptr;
        return err;
    }

    initialised = TRUE;
    *publickey_out = publickey;
    *privatekey_out = privatekey;

    return 0;
}

static int issueToken(CLIENT_BLOCK* client_block, LOGMALLOCLIST* logmalloclist, USERDEFINEDTYPELIST* userdefinedtypelist,
                      gcry_sexp_t publickey, gcry_sexp_t privatekey, gcry_mpi_t* client_mpiToken, gcry_mpi_t* server_mpiToken)
{
    int err = 0;

    // Prepare the encrypted token (A)

    unsigned char* client_ciphertext = nullptr;
    unsigned char* server_ciphertext = nullptr;
    size_t client_ciphertextLength = 0;
    size_t server_ciphertextLength = 0;

    err = udaAuthentication(CLIENT_ISSUE_TOKEN, encryptionMethod,
                            tokenType, tokenByteLength,
                            publickey, privatekey,
                            client_mpiToken, server_mpiToken,
                            &client_ciphertext, &client_ciphertextLength,
                            &server_ciphertext, &server_ciphertextLength);

    if (err != 0) {
        THROW_ERROR(err, "Failed Preparing Authentication Step #1!");
    }

// Send the encrypted token to the server together with Client's claim of identity

    SECURITY_BLOCK* securityBlock = &client_block->securityBlock;

    securityBlock->authenticationStep = CLIENT_ISSUE_TOKEN;
    securityBlock->client_ciphertext = client_ciphertext;
    securityBlock->server_ciphertext = nullptr;

    securityBlock->client_ciphertextLength = (unsigned short)client_ciphertextLength;
    securityBlock->server_ciphertextLength = 0;

#ifndef TESTIDAMSECURITY
    int protocol_id = UDA_PROTOCOL_CLIENT_BLOCK;

    if ((err = protocol2(clientOutput, protocol_id, XDR_SEND, nullptr, logmalloclist, userdefinedtypelist, &client_block)) != 0) {
        THROW_ERROR(err, "Protocol 10 Error (securityBlock #1)");
    }

    // Send to server
    if (!xdrrec_endofrecord(clientOutput, 1)) {
        THROW_ERROR(UDA_PROTOCOL_ERROR_7, "Protocol 7 Error (Client Block #1)");
    }

    // No need to resend the client's certificates or encrypted token A
    free(securityBlock->client_ciphertext);

    securityBlock->client_ciphertext = nullptr;
    securityBlock->client_ciphertextLength = 0;

    free(securityBlock->client_X509);
    free(securityBlock->client2_X509);

    securityBlock->client_X509 = nullptr;
    securityBlock->client2_X509 = nullptr;
    securityBlock->client_X509Length = 0;
    securityBlock->client2_X509Length = 0;
#endif

    return err;
}

static int decryptServerToken(SERVER_BLOCK* server_block, CLIENT_BLOCK* client_block, LOGMALLOCLIST* logmalloclist,
                              USERDEFINEDTYPELIST* userdefinedtypelist, gcry_sexp_t publickey, gcry_sexp_t privatekey,
                              gcry_mpi_t* client_mpiToken, gcry_mpi_t* server_mpiToken)
{
    int err = 0;

    //---------------------------------------------------------------------------------------------------------------
    // Step 5: Client decrypts the passed ciphers (EACP, EBCP) with the client's private key (->A, ->B) and
    //       checks token (A) => server authenticated

    // Receive the encrypted tokens (A,B) from the server

#ifndef TESTIDAMSECURITY
    if (!xdrrec_endofrecord(clientInput, 1)) {
        THROW_ERROR(UDA_PROTOCOL_ERROR_7, "Protocol 7 Error (Server Block #5)");
    }

    int protocol_id = UDA_PROTOCOL_SERVER_BLOCK;

    if ((err = protocol2(clientInput, protocol_id, XDR_RECEIVE, nullptr, logmalloclist, userdefinedtypelist, &server_block)) != 0) {
        THROW_ERROR(err, "Protocol 11 Error (securityBlock #5)");
    }

    // Flush (mark as at EOF) the input socket buffer (not all server state data may have been read - version dependent)

    xdrrec_eof(clientInput);
#endif

    UDA_LOG(UDA_LOG_DEBUG, "Server Block Received\n");
    printServerBlock(*server_block);

    // Protocol Version: Lower of the client and server version numbers
    // This defines the set of elements within data structures passed between client and server
    // Must be the same on both sides of the socket

    if (client_block->version < server_block->version) {
        protocolVersion = client_block->version;
    }

    // Check for FATAL Server Errors

    if (server_block->idamerrorstack.nerrors != 0) {
        THROW_ERROR(999, "Server Side Authentication Failed!");
    }

    // Extract Ciphers

    SECURITY_BLOCK* securityBlock = &server_block->securityBlock;

    if (securityBlock->authenticationStep != CLIENT_DECRYPT_SERVER_TOKEN - 1) {
        THROW_ERROR(999, "Authentication Step Inconsistency!");
    }

    unsigned char* client_ciphertext = securityBlock->client_ciphertext;
    unsigned char* server_ciphertext = securityBlock->server_ciphertext;
    size_t client_ciphertextLength = securityBlock->client_ciphertextLength;
    size_t server_ciphertextLength = securityBlock->server_ciphertextLength;

    // Decrypt tokens (A, B) and Authenticate the Server

    err = udaAuthentication(CLIENT_DECRYPT_SERVER_TOKEN, encryptionMethod,
                            tokenType, tokenByteLength,
                            publickey, privatekey,
                            client_mpiToken, server_mpiToken,
                            &client_ciphertext, &client_ciphertextLength,
                            &server_ciphertext, &server_ciphertextLength);

    if (err != 0) {
        THROW_ERROR(err, "Failed Authentication Step #5!");
    }

    free(client_ciphertext);
    free(server_ciphertext);

    return err;
}

static int encryptServerToken(CLIENT_BLOCK* client_block, LOGMALLOCLIST* logmalloclist, USERDEFINEDTYPELIST* userdefinedtypelist,
                              gcry_sexp_t publickey, gcry_sexp_t privatekey, gcry_mpi_t* client_mpiToken, gcry_mpi_t* server_mpiToken)
{
    int err = 0;

    //---------------------------------------------------------------------------------------------------------------
    // Step 6: Client encrypts passed token (B) with the server's public key (->EBSP), passes back to server

    // Encrypt token (B)

    unsigned char* client_ciphertext = nullptr;
    unsigned char* server_ciphertext = nullptr;
    size_t client_ciphertextLength = 0;
    size_t server_ciphertextLength = 0;

    err = udaAuthentication(CLIENT_ENCRYPT_SERVER_TOKEN, encryptionMethod,
                            tokenType, tokenByteLength,
                            publickey, privatekey,
                            client_mpiToken, server_mpiToken,
                            &client_ciphertext, &client_ciphertextLength,
                            &server_ciphertext, &server_ciphertextLength);

    if (err != 0) {
        THROW_ERROR(err, "Failed Preparing Authentication Step #6!");
    }

    // Send the encrypted token to the server via the CLIENT_BLOCK data structure

    SECURITY_BLOCK* securityBlock = &client_block->securityBlock;

    securityBlock->authenticationStep = CLIENT_ENCRYPT_SERVER_TOKEN;
    securityBlock->server_ciphertext = server_ciphertext;
    securityBlock->client_ciphertext = nullptr;
    securityBlock->server_ciphertextLength = (unsigned short)server_ciphertextLength;
    securityBlock->client_ciphertextLength = 0;

#ifndef TESTIDAMSECURITY
    int protocol_id = UDA_PROTOCOL_CLIENT_BLOCK;

    if ((err = protocol2(clientOutput, protocol_id, XDR_SEND, nullptr, logmalloclist, userdefinedtypelist, &client_block)) != 0) {
        THROW_ERROR(err, "Protocol 10 Error (securityBlock #6)");
    }

    // Send to server
    if (!xdrrec_endofrecord(clientOutput, 1)) {
        THROW_ERROR(UDA_PROTOCOL_ERROR_7, "Protocol 7 Error (Client Block #6)");
    }

    free(server_ciphertext);
    free(client_ciphertext);
#endif

    return err;
}

int clientAuthentication(CLIENT_BLOCK* client_block, SERVER_BLOCK* server_block, LOGMALLOCLIST* logmalloclist,
                         USERDEFINEDTYPELIST* userdefinedtypelist, AUTHENTICATION_STEP authenticationStep)
{
    gcry_sexp_t privatekey = nullptr;
    gcry_sexp_t publickey = nullptr;

    int err = initialiseKeys(client_block, &publickey, &privatekey);
    if (err != 0) {
        return err;
    }

    static gcry_mpi_t client_mpiToken = nullptr;
    static gcry_mpi_t server_mpiToken = nullptr;

    switch (authenticationStep) {
        case CLIENT_ISSUE_TOKEN:
            err = issueToken(client_block, logmalloclist, userdefinedtypelist, publickey, privatekey, &client_mpiToken, &server_mpiToken);
            break;

        case CLIENT_DECRYPT_SERVER_TOKEN:
            err = decryptServerToken(server_block, client_block, logmalloclist, userdefinedtypelist, publickey, privatekey, &client_mpiToken, &server_mpiToken);
            break;

        case CLIENT_ENCRYPT_SERVER_TOKEN:
            err = encryptServerToken(client_block, logmalloclist, userdefinedtypelist, publickey, privatekey, &client_mpiToken, &server_mpiToken);
            break;

        case HOUSEKEEPING:
            free(publickey);
            free(privatekey);
            if (client_mpiToken != nullptr) gcry_mpi_release(client_mpiToken);
            if (server_mpiToken != nullptr) gcry_mpi_release(server_mpiToken);
            break;

        default:
            THROW_ERROR(999, "Unknown authentication step");
    }

    if (err != 0) {
        if (client_mpiToken != nullptr) gcry_mpi_release(client_mpiToken);
        if (server_mpiToken != nullptr) gcry_mpi_release(server_mpiToken);
    }

    return err;
}
