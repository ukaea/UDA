#if defined(SERVERBUILD) || defined(TESTIDAMSECURITY)

#  include "serverAuthentication.h"

#  include "clientserver/errorLog.h"
#  include "logging/logging.h"
#  include "server/udaServer.h"
#  ifndef TESTIDAMSECURITY
#    include "clientserver/printStructs.h"
#    include "clientserver/protocol.h"
#    include "clientserver/udaErrors.h"
#    include "clientserver/xdrlib.h"
#  endif

#  include "authenticationUtils.h"
#  include "security.h"
#  include "x509Utils.h"

static const ENCRYPTION_METHOD encryptionMethod = ASYMMETRICKEY;
static const unsigned short tokenByteLength = NONCEBYTELENGTH; // System problem when >~ 110 !
static const TOKEN_TYPE tokenType =
    NONCESTRONGRANDOM; // NONCESTRONGRANDOM NONCESTRINGRANDOM NONCEWEAKRANDOM NONCETEST //

/**
 * Read the Server's Private Key (from a PEM file) and the User's Public Key (from the passed x509 cert)
 * Read the Certificate Authority's Public Key to check signature of the client's certificate
 * Keys have S-Expressions format
 *
 * Key locations are identified from an environment variable
 * The client's public key is from a x509 certificate (check date validity + CA signature)
 *
 * @param client_block
 * @param publickey_out
 * @param privatekey_out
 * @return
 */
static int initialiseKeys(ClientBlock* client_block, gcry_sexp_t* publickey_out, gcry_sexp_t* privatekey_out)
{
    int err = 0;
    static int initialised = FALSE;

    static gcry_sexp_t privatekey = nullptr; // Server's
    static gcry_sexp_t publickey = nullptr;  // Client's

    if (initialised) {
        *publickey_out = publickey;
        *privatekey_out = privatekey;
        return 0;
    }

    char* env = nullptr;
    size_t len = 0;

    char* serverPrivateKeyFile = nullptr;
    char* CACertFile = nullptr;

    if ((env = getenv("UDA_SERVER_CERTIFICATE")) != nullptr) { // Directory with certificates and key files
        len = strlen(env) + 56;
        serverPrivateKeyFile = (char*)malloc(len * sizeof(unsigned char));
        CACertFile = (char*)malloc(len * sizeof(unsigned char));

        sprintf(serverPrivateKeyFile, "%s/serverskey.pem", env); // Server's
        sprintf(CACertFile, "%s/carootX509.der", env);           // CA Certificate for signature verification
    } else {
        char* home = getenv("HOME");
        len = 256 + strlen(home);

        serverPrivateKeyFile = (char*)malloc(len * sizeof(unsigned char));
        CACertFile = (char*)malloc(len * sizeof(unsigned char));

        sprintf(serverPrivateKeyFile, "%s/.UDA/server/serverskey.pem", home);
        sprintf(CACertFile, "%s/.UDA/server/carootX509.der", home);
    }

    ksba_cert_t clientCert = nullptr;
    ksba_cert_t CACert = nullptr;

    do { // Error Trap

        SECURITY_BLOCK* securityBlock = &client_block->securityBlock;

        // Read the Client's certificate, check validity, extract the public key

        if (securityBlock->client_X509 != nullptr) {
            if ((err = makeX509CertObject(securityBlock->client_X509, securityBlock->client_X509Length, &clientCert)) !=
                0) {
                break;
            }
            if ((err = testX509Dates(clientCert)) != 0) {
                break; // Check the Certificate Validity
            }
            if ((err = extractX509SExpKey(clientCert, &publickey)) != 0) {
                break;
            } // get the Public key from an X509 certificate
        }

        // get the server's Private key from a PEM file (for decryption) and convert to S-Expression

        if ((err = importPEMPrivateKey(serverPrivateKeyFile, &privatekey)) != 0) {
            add_error(UDA_CODE_ERROR_TYPE, __func__, err, "Failed to load Server's Private Key File");
            break;
        }

        // Read the CA's certificate, check date validity

        if (CACertFile != nullptr) {
            if ((err = importX509Reader(CACertFile, &CACert)) != 0) {
                break;
            }
            if ((err = testX509Dates(CACert)) != 0) {
                break;
            }
        }

        // Test the server's private key for consistency

        if (gcry_pk_testkey(privatekey) != 0) {
            err = 999;
            add_error(UDA_CODE_ERROR_TYPE, "idamServerAuthentication", err,
                      "The Server's Private Authentication Key is Invalid!");
            break;
        }

        // Verify the client certificate's signature using the CA's public key

        if ((err = checkX509Signature(CACert, clientCert)) != 0) {
            break;
        }

        ksba_cert_release(clientCert);
        ksba_cert_release(CACert);
        clientCert = nullptr;
        CACert = nullptr;

    } while (0);

    free(serverPrivateKeyFile);
    free(CACertFile);

    if (err != 0) {
        if (privatekey != nullptr) {
            gcry_sexp_release(privatekey);
        }
        if (publickey != nullptr) {
            gcry_sexp_release(publickey);
        }
        if (clientCert != nullptr) {
            ksba_cert_release(clientCert);
        }
        if (CACert != nullptr) {
            ksba_cert_release(CACert); // BUG in ksba library# ksba_free !
        }
        privatekey = nullptr; // These are declared as static so ensure reset when an error occurs
        publickey = nullptr;
        return err;
    }

    *publickey_out = publickey;
    *privatekey_out = privatekey;
    initialised = TRUE;

    return err;
}

/**
 * Receive the client block, respecting earlier protocol versions
 * @param client_block
 * @return
 */
static SECURITY_BLOCK* receiveSecurityBlock(ClientBlock* client_block, LogMallocList* logmalloclist,
                                            UserDefinedTypeList* userdefinedtypelist)
{
    UDA_LOG(UDA_LOG_DEBUG, "Waiting for Initial Client Block\n");

#  ifndef TESTIDAMSECURITY
    if (!xdrrec_skiprecord(serverInput)) {
        UDA_LOG(UDA_LOG_DEBUG, "xdrrec_skiprecord error!\n");
        add_error(UDA_CODE_ERROR_TYPE, __func__, UDA_PROTOCOL_ERROR_5, "Protocol 5 Error (Client Block #2)");
    } else {
        int protocol_id = UDA_PROTOCOL_CLIENT_BLOCK; // Recieve Client Block

        int err = 0;
        if ((err = protocol2(serverInput, protocol_id, XDR_RECEIVE, nullptr, logmalloclist, userdefinedtypelist,
                             client_block)) != 0) {
            add_error(UDA_CODE_ERROR_TYPE, __func__, err, "Protocol 10 Error (Client Block #2)");
            UDA_LOG(UDA_LOG_DEBUG, "protocol error! Client Block not received!\n");
        }

        if (err == 0) {
            UDA_LOG(UDA_LOG_DEBUG, "Initial Client Block received\n");
            print_client_block(*client_block);
        }
    }

    // Flush (mark as at EOF) the input socket buffer (not all client state data may have been read - version dependent)

    xdrrec_eof(serverInput);
#  endif

    return &client_block->securityBlock;
}

static int decryptClientToken(ClientBlock* client_block, LogMallocList* logmalloclist,
                              UserDefinedTypeList* userdefinedtypelist, gcry_sexp_t publickey, gcry_sexp_t privatekey,
                              gcry_mpi_t* client_mpiToken, gcry_mpi_t* server_mpiToken)
{
    int err = 0;

    //---------------------------------------------------------------------------------------------------------------
    // Read the ClientBlock and client x509 certificate
    SECURITY_BLOCK* securityBlock = receiveSecurityBlock(client_block, logmalloclist, userdefinedtypelist);

    //---------------------------------------------------------------------------------------------------------------
    // Step 2: Receive the Client's token cipher (EASP) and decrypt with the server's private key (->A)

    // Already received the encrypted token (A) from the client

    if (securityBlock->authenticationStep != SERVER_DECRYPT_CLIENT_TOKEN - 1) {
        UDA_THROW_ERROR(999, "Authentication Step #2 Inconsistency!");
    }

    unsigned char* client_ciphertext = securityBlock->client_ciphertext;
    unsigned char* server_ciphertext = securityBlock->server_ciphertext;

    size_t client_ciphertextLength = securityBlock->client_ciphertextLength;
    size_t server_ciphertextLength = securityBlock->server_ciphertextLength;

    // Decrypt token (A)

    err = udaAuthentication(SERVER_DECRYPT_CLIENT_TOKEN, encryptionMethod, tokenType, tokenByteLength, publickey,
                            privatekey, client_mpiToken, server_mpiToken, &client_ciphertext, &client_ciphertextLength,
                            &server_ciphertext, &server_ciphertextLength);

    if (err != 0) {
        UDA_THROW_ERROR(err, "Failed Decryption Step #2!");
    }

    free(client_ciphertext);
    client_ciphertext = nullptr;
    client_ciphertextLength = 0;
    free(server_ciphertext);
    server_ciphertext = nullptr;
    server_ciphertextLength = 0;

    return err;
}

static int encryptClientToken(ServerBlock* server_block, gcry_sexp_t publickey, gcry_sexp_t privatekey,
                              gcry_mpi_t* client_mpiToken, gcry_mpi_t* server_mpiToken)
{
    int err = 0;

    //---------------------------------------------------------------------------------------------------------------
    // Step 3: Server encrypts the client token (A) with the client's public key (->EACP)

    // Encrypt token (A)

    unsigned char* client_ciphertext = nullptr;
    unsigned char* server_ciphertext = nullptr;
    size_t client_ciphertextLength = 0;
    size_t server_ciphertextLength = 0;

    err = udaAuthentication(SERVER_ENCRYPT_CLIENT_TOKEN, encryptionMethod, tokenType, tokenByteLength, publickey,
                            privatekey, client_mpiToken, server_mpiToken, &client_ciphertext, &client_ciphertextLength,
                            &server_ciphertext, &server_ciphertextLength);

    if (err != 0) {
        UDA_THROW_ERROR(err, "Failed Encryption Step #3!");
    }

    SECURITY_BLOCK* securityBlock = &server_block->securityBlock;
    initSecurityBlock(securityBlock);

    securityBlock->client_ciphertext = client_ciphertext;
    securityBlock->client_ciphertextLength = (unsigned short)client_ciphertextLength;

    gcry_mpi_release(*client_mpiToken);

    return err;
}

static int issueToken(ServerBlock* server_block, LogMallocList* logmalloclist, UserDefinedTypeList* userdefinedtypelist,
                      gcry_sexp_t publickey, gcry_sexp_t privatekey, gcry_mpi_t* client_mpiToken,
                      gcry_mpi_t* server_mpiToken)
{
    int err = 0;
    //---------------------------------------------------------------------------------------------------------------
    // Step 4: Server issues a new token (B) also encrypted with the client's public key (->EBCP), passes both to
    // client.

    // Generate new Token and Encrypt (B)

    unsigned char* client_ciphertext = nullptr;
    unsigned char* server_ciphertext = nullptr;
    size_t client_ciphertextLength = 0;
    size_t server_ciphertextLength = 0;

    err = udaAuthentication(SERVER_ISSUE_TOKEN, encryptionMethod, tokenType, tokenByteLength, publickey, privatekey,
                            client_mpiToken, server_mpiToken, &client_ciphertext, &client_ciphertextLength,
                            &server_ciphertext, &server_ciphertextLength);

    if (err != 0) {
        UDA_THROW_ERROR(err, "Failed Encryption Step #4!");
    }

    // Send the encrypted token to the server

    SECURITY_BLOCK* securityBlock = &server_block->securityBlock;

    securityBlock->authenticationStep = SERVER_ISSUE_TOKEN;
    securityBlock->server_ciphertext = server_ciphertext;
    securityBlock->server_ciphertextLength = (unsigned short)server_ciphertextLength;

#  ifndef TESTIDAMSECURITY
    int protocol_id = UDA_PROTOCOL_SERVER_BLOCK;

    if ((err = protocol2(serverOutput, protocol_id, XDR_SEND, nullptr, logmalloclist, userdefinedtypelist,
                         server_block)) != 0) {
        add_error(UDA_CODE_ERROR_TYPE, __func__, err, "Protocol 10 Error (securityBlock #4)");
    }

    if (!xdrrec_endofrecord(serverOutput, 1)) {
        add_error(UDA_CODE_ERROR_TYPE, __func__, UDA_PROTOCOL_ERROR_7, "Protocol 7 Error (Server Block)");
    }
#  endif

    return err;
}

static int verifyToken(ServerBlock* server_block, ClientBlock* client_block, LogMallocList* logmalloclist,
                       UserDefinedTypeList* userdefinedtypelist, gcry_sexp_t publickey, gcry_sexp_t privatekey,
                       gcry_mpi_t* client_mpiToken, gcry_mpi_t* server_mpiToken)
{
    int err = 0;

    //---------------------------------------------------------------------------------------------------------------
    // Step 7: Server decrypts the passed cipher (EBSP) with the server's private key (->B) and checks
    //       token (B) => client authenticated

    // Receive the encrypted token (B) from the client

#  ifndef TESTIDAMSECURITY
    int protocol_id = UDA_PROTOCOL_CLIENT_BLOCK;

    if (!xdrrec_skiprecord(serverInput)) {
        UDA_LOG(UDA_LOG_DEBUG, "xdrrec_skiprecord error!\n");
        UDA_THROW_ERROR(UDA_PROTOCOL_ERROR_5, "Protocol 5 Error (Client Block #7)");
    }

    if ((err = protocol2(serverInput, protocol_id, XDR_RECEIVE, nullptr, logmalloclist, userdefinedtypelist,
                         client_block)) != 0) {
        UDA_THROW_ERROR(err, "Protocol 11 Error (securityBlock #7)");
    }

    // Flush (mark as at EOF) the input socket buffer (not all client state data may have been read - version dependent)
    xdrrec_eof(serverInput);
#  endif

    SECURITY_BLOCK* securityBlock = &client_block->securityBlock;

    if (securityBlock->authenticationStep != SERVER_VERIFY_TOKEN - 1) {
        UDA_THROW_ERROR(999, "Authentication Step Inconsistency!");
    }

    unsigned char* client_ciphertext = securityBlock->client_ciphertext;
    unsigned char* server_ciphertext = securityBlock->server_ciphertext;
    size_t client_ciphertextLength = securityBlock->client_ciphertextLength;
    size_t server_ciphertextLength = securityBlock->server_ciphertextLength;

    // Decrypt token (B) and Authenticate the Client

    err = udaAuthentication(SERVER_VERIFY_TOKEN, encryptionMethod, tokenType, tokenByteLength, publickey, privatekey,
                            client_mpiToken, server_mpiToken, &client_ciphertext, &client_ciphertextLength,
                            &server_ciphertext, &server_ciphertextLength);

    if (err != 0) {
        UDA_THROW_ERROR(err, "Failed Authentication Step #7!");
    }

    // Send the encrypted token B to the client

    securityBlock = &server_block->securityBlock;

    securityBlock->authenticationStep = SERVER_VERIFY_TOKEN;
    securityBlock->server_ciphertext = server_ciphertext;
    securityBlock->server_ciphertextLength = (unsigned short)server_ciphertextLength;
    securityBlock->client_ciphertext = client_ciphertext;
    securityBlock->client_ciphertextLength = (unsigned short)client_ciphertextLength;

#  ifndef TESTIDAMSECURITY
    protocol_id = UDA_PROTOCOL_SERVER_BLOCK;

    if ((err = protocol2(serverOutput, protocol_id, XDR_SEND, nullptr, logmalloclist, userdefinedtypelist,
                         server_block)) != 0) {
        UDA_THROW_ERROR(err, "Protocol 10 Error (securityBlock #7)");
    }

    if (!xdrrec_endofrecord(serverOutput, 1)) {
        UDA_THROW_ERROR(UDA_PROTOCOL_ERROR_7, "Protocol 7 Error (Server Block #7)");
    }
#  endif

    return err;
}

int serverAuthentication(ClientBlock* client_block, ServerBlock* server_block, LogMallocList* logmalloclist,
                         UserDefinedTypeList* userdefinedtypelist, AUTHENTICATION_STEP authenticationStep)
{
    gcry_sexp_t privatekey = nullptr;
    gcry_sexp_t publickey = nullptr;

    int err = initialiseKeys(client_block, &publickey, &privatekey);
    if (err != 0) {
        return err;
    }

    //---------------------------------------------------------------------------------------------------------------
    // Authenticate both Client and Server

    static gcry_mpi_t client_mpiToken = nullptr;
    static gcry_mpi_t server_mpiToken = nullptr;

    switch (authenticationStep) {
        case SERVER_DECRYPT_CLIENT_TOKEN:
            err = decryptClientToken(client_block, logmalloclist, userdefinedtypelist, publickey, privatekey,
                                     &client_mpiToken, &server_mpiToken);
            break;

        case SERVER_ENCRYPT_CLIENT_TOKEN:
            err = encryptClientToken(server_block, publickey, privatekey, &client_mpiToken, &server_mpiToken);
            break;

        case SERVER_ISSUE_TOKEN:
            err = issueToken(server_block, logmalloclist, userdefinedtypelist, publickey, privatekey, &client_mpiToken,
                             &server_mpiToken);
            break;

        case SERVER_VERIFY_TOKEN:
            err = verifyToken(server_block, client_block, logmalloclist, userdefinedtypelist, publickey, privatekey,
                              &client_mpiToken, &server_mpiToken);
            break;

        case HOUSEKEEPING:
            free(privatekey);
            free(publickey);
            if (client_mpiToken != nullptr) {
                gcry_mpi_release(client_mpiToken);
            }
            if (server_mpiToken != nullptr) {
                gcry_mpi_release(server_mpiToken);
            }
            break;

        default:
            UDA_THROW_ERROR(999, "Unknown authentication step");
    }

    if (err != 0) {
        if (client_mpiToken != nullptr) {
            gcry_mpi_release(client_mpiToken);
        }
        if (server_mpiToken != nullptr) {
            gcry_mpi_release(server_mpiToken);
        }
    }

    return err;
}

#endif // SERVERBUILD || TESTIDAMSECURITY
