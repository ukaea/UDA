#include "clientAuthentication.h"

#include <errno.h>
#include <sys/stat.h>
#include <gcrypt.h>
#include <ksba.h>

#include <clientserver/errorLog.h>
#include <clientserver/protocol.h>
#include <client/udaClient.h>
#include <clientserver/xdrlib.h>
#include <clientserver/udaErrors.h>
#include <clientserver/printStructs.h>
#include <logging/logging.h>

#include "security.h"

static int testPermissions(const char* object)
{
    struct stat buffer;
    int rc, err = 0;

    rc = stat(object, &buffer);
    if (rc != 0 || errno != 0) {
        err = 999;
        if (errno != 0) addIdamError(&idamerrorstack, SYSTEMERRORTYPE, "idamClientAuthentication", errno, "");
        addIdamError(&idamerrorstack, CODEERRORTYPE, "idamClientAuthentication", err,
                     "Could not verify the user's private key directory's access permissions!");
        return err;
    }
    if (buffer.st_mode & S_IRGRP ||
        buffer.st_mode & S_IWGRP ||
        buffer.st_mode & S_IXGRP ||
        buffer.st_mode & S_IROTH ||
        buffer.st_mode & S_IWOTH ||
        buffer.st_mode & S_IXOTH) {
        err = 999;
        addIdamError(&idamerrorstack, CODEERRORTYPE, "idamClientAuthentication", err,
                     "The user's private key directory is public: Your keys and certificate may be compromised!!!");
        return err;
    }
    return 0;
}

int clientAuthentication(CLIENT_BLOCK* client_block, SERVER_BLOCK* server_block, unsigned short authenticationStep)
{

    static short initAuthenticationKeys = 1;    // Input keys and certificates at startup

    static gcry_sexp_t privatekey = NULL;    // Client's private key - maintain state for future en/decryption
    static gcry_sexp_t publickey = NULL;    // Server's public key

    SECURITY_BLOCK* securityBlock = NULL;    // passed between Client and Server with authentication challenges

    ksba_cert_t clientCert = NULL, client2Cert = NULL, serverCert = NULL;

    int err = 0;

//---------------------------------------------------------------------------------------------------------------
// Read the User's Private Key (from a PEM format file) and the Server's Public Key (from a DER format x509 cert)
// Convert keys from PEM to S-Expressions representation

// Key locations are identified from an environment variable
// Server public key are from a x509 certificate (to check date validity - a key file isn't sufficient)

    if (initAuthenticationKeys) {

        char* env = NULL;
        size_t len = 0;

        char* clientPrivateKeyFile = NULL;
        char* serverPublicKeyFile = NULL;
        char* clientX509File = NULL;      // Authentication with the first server in a server chain
        char* client2X509File = NULL;     // Delivered to the final host in a server chain where the data resides
        char* serverX509File = NULL;      // Certificate of the first server host


        if ((env = getenv("UDA_CLIENT_CERTIFICATE")) != NULL) {    // Directory with certificates and key files
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

        if ((env = getenv("UDA_CLIENT2_CERTIFICATE")) != NULL) {
            // X509 certificate to authenticate with the final server in a chain
            len = strlen(env) + 56;
            client2X509File = (char*)malloc(len * sizeof(unsigned char));
            sprintf(client2X509File, "%s/client/client2X509.der", env);
        }

        do {    // Error Trap

// Read the Client's certificates and check date validity 

            securityBlock = &client_block->securityBlock;
            initSecurityBlock(securityBlock);

            if (clientX509File != NULL) {
                if ((err = importSecurityDoc(clientX509File, &securityBlock->client_X509,
                                             &securityBlock->client_X509Length)) != 0) {
                    break;
                }
                if ((err = makeX509CertObject(securityBlock->client_X509, securityBlock->client_X509Length,
                                              &clientCert)) != 0) {
                    break;
                }
                if ((err = testX509Dates(clientCert)) != 0) break;                // Check the Certificate Validity
                ksba_cert_release(clientCert);
                clientCert = NULL;
            }

            if (client2X509File != NULL) {
                if ((err = importSecurityDoc(client2X509File, &securityBlock->client2_X509,
                                             &securityBlock->client2_X509Length)) != 0) {
                    break;
                }
                if ((err = makeX509CertObject(securityBlock->client2_X509, securityBlock->client2_X509Length,
                                              &client2Cert)) != 0) {
                    break;
                }
                if ((err = testX509Dates(client2Cert)) != 0) break;
                ksba_cert_release(client2Cert);
                client2Cert = NULL;
            }

// Test the private key file and its directory directory have permissions set to owner read only     

            if ((err = testPermissions(clientPrivateKeyFile)) != 0) break;
            if (client2X509File != NULL && (err = testPermissions((char*)client2X509File)) != 0) break;
            if ((env = getenv("UDA_CLIENT_CERTIFICATE")) != NULL) {
                if ((err = testPermissions(env)) != 0) break;
            } else {
                const char* home = getenv("HOME");
                char work[1024];
                sprintf(work, "%s/.UDA/client", home);
                if ((err = testPermissions(work)) != 0) break;
            }

            // get the user's Private key from a PEM file (for decryption) and convert to S-Expression

            if ((err = importPEMPrivateKey(clientPrivateKeyFile, &privatekey)) != 0) break;

            // get the server's Public key (for encryption of exchanged tokens) from a certificate or a file
            // If from a PEM file, convert to S-Expression

            if (serverX509File != NULL) {
                unsigned char* serverCertificate;        // Server's X509 authentication certificate
                unsigned short serverCertificateLength;

                if ((err = importSecurityDoc(serverX509File, &serverCertificate, &serverCertificateLength)) != 0) {
                    break;
                }
                if ((err = makeX509CertObject(serverCertificate, serverCertificateLength, &serverCert)) != 0) break;
                if ((err = testX509Dates(serverCert)) != 0) break;                // Check the Certificate Validity
                if ((err = extractX509SExpKey(serverCert, &publickey)) != 0) {
                    break;
                }        // get the server's Public key from an X509 certificate
                ksba_cert_release(serverCert);
                serverCert = NULL;
                if (serverCertificate != NULL) free(serverCertificate);
                serverCertificate = NULL;
            } else if ((err = importPEMPublicKey(serverPublicKeyFile, &publickey)) != 0) {
                break;
            }        // get the server's Public key from a file

            // Test the user's private key for consistency
            // User keys also have a lifetime - automatically checked if there is a x509 certificate
            // Stale keys must be renewed by a utility (separate system) requiring strong authentication - proof of identity
            // Server may also renew it's public key at that time (may be different for each user!)

            if (gcry_pk_testkey(privatekey) != 0) {
                err = 999;
                addIdamError(&idamerrorstack, CODEERRORTYPE, "idamClientAuthentication", err,
                             "The User's Private Authentication Key is Invalid!");
                break;
            }

        } while (0);

        free(clientPrivateKeyFile);
        free(serverPublicKeyFile);
        free(clientX509File);
        free(serverX509File);
        free(client2X509File);

        if (err != 0) {
            free((void*)securityBlock->client_X509);
            free((void*)securityBlock->client2_X509);

            securityBlock->client_X509 = NULL;
            securityBlock->client2_X509 = NULL;
            securityBlock->client_X509Length = 0;
            securityBlock->client2_X509Length = 0;

            if (clientCert != NULL) ksba_cert_release(clientCert);
            if (client2Cert != NULL) ksba_cert_release(client2Cert);
            if (serverCert != NULL) ksba_cert_release(serverCert);
            clientCert = NULL;
            client2Cert = NULL;
            serverCert = NULL;

            if (privatekey != NULL) gcry_sexp_release(privatekey);
            if (publickey != NULL) gcry_sexp_release(publickey);
            privatekey = NULL;    // These are declared as static so ensure they are reset when an error occurs
            publickey = NULL;
            return err;
        }

        initAuthenticationKeys = 0;
    }

//---------------------------------------------------------------------------------------------------------------
// Authenticate both Client and Server

    unsigned short encryptionMethod = ASYMMETRICKEY;
    unsigned short tokenByteLength = NONCEBYTELENGTH;        // System problem when >~ 110 !
    unsigned short tokenType = NONCESTRONGRANDOM;    //NONCESTRINGRANDOM; // NONCEWEAKRANDOM; // NONCETEST; //

    static gcry_mpi_t client_mpiToken = NULL;
    static gcry_mpi_t server_mpiToken = NULL;

    unsigned char* client_ciphertext = NULL, * server_ciphertext = NULL;
    unsigned short client_ciphertextLength = 0;
    unsigned short server_ciphertextLength = 0;

    int protocol_id;

    do {        // Error Trap

//---------------------------------------------------------------------------------------------------------------
// Step 1: Client issues a token (A), encrypts with the server's public key (->EASP), passes to server  

        if (authenticationStep == 1) {

// Prepare the encrypted token (A)

            err = udaAuthentication(authenticationStep, encryptionMethod,
                                    tokenType, tokenByteLength,
                                    publickey, privatekey,
                                    &client_mpiToken, &server_mpiToken,
                                    &client_ciphertext, &client_ciphertextLength,
                                    &server_ciphertext, &server_ciphertextLength);

            if (err != 0) {
                addIdamError(&idamerrorstack, CODEERRORTYPE, "idamClientAuthentication", err,
                             "Failed Preparing Authentication Step #1!");
                break;
            }

// Send the encrypted token to the server together with Client's claim of identity  

            securityBlock = &client_block->securityBlock;

            securityBlock->authenticationStep = authenticationStep;
            securityBlock->client_ciphertext = client_ciphertext;
            securityBlock->server_ciphertext = NULL;

            securityBlock->client_ciphertextLength = client_ciphertextLength;
            securityBlock->server_ciphertextLength = 0;

            //securityBlock->client_X509             = x509Block.clientCertificate;		// Only sent in step #1
            //securityBlock->client_X509Length       = x509Block.clientCertificateLength;

            //securityBlock->client2_X509            = x509Block.client2Certificate;		// Only sent in step #2
            //securityBlock->client2_X509Length      = x509Block.client2CertificateLength;

            protocol_id = PROTOCOL_CLIENT_BLOCK;

            if ((err = protocol2(clientOutput, protocol_id, XDR_SEND, NULL, &client_block)) != 0) {
                addIdamError(&idamerrorstack, CODEERRORTYPE, "idamClientAuthentication", err,
                             "Protocol 10 Error (securityBlock #1)");
                break;
            }

// Send to server

            if (!xdrrec_endofrecord(clientOutput, 1)) {
                err = PROTOCOL_ERROR_7;
                addIdamError(&idamerrorstack, CODEERRORTYPE, "idamClient", err, "Protocol 7 Error (Client Block #1)");
                break;
            }

// No need to resend the client's certificates or encrypted token A

            if (securityBlock->client_ciphertext != NULL) free(securityBlock->client_ciphertext);
            securityBlock->client_ciphertext = NULL;
            securityBlock->client_ciphertextLength = 0;
            client_ciphertext = NULL;
            client_ciphertextLength = 0;

            if (securityBlock->client_X509 != NULL) free((void*)securityBlock->client_X509);
            if (securityBlock->client2_X509 != NULL) free((void*)securityBlock->client2_X509);
            securityBlock->client_X509 = NULL;
            securityBlock->client2_X509 = NULL;
            securityBlock->client_X509Length = 0;
            securityBlock->client2_X509Length = 0;
        } else

//---------------------------------------------------------------------------------------------------------------
// Step 5: Client decrypts the passed ciphers (EACP, EBCP) with the client's private key (->A, ->B) and 
//	   checks token (A) => server authenticated

        if (authenticationStep == 5) {

// Receive the encrypted tokens (A,B) from the server

            if (!xdrrec_endofrecord(clientInput, 1)) {
                err = PROTOCOL_ERROR_7;
                addIdamError(&idamerrorstack, CODEERRORTYPE, "idamClient", err, "Protocol 7 Error (Server Block #5)");
                break;
            }

            protocol_id = PROTOCOL_SERVER_BLOCK;

            if ((err = protocol2(clientInput, protocol_id, XDR_RECEIVE, NULL, &server_block)) != 0) {
                addIdamError(&idamerrorstack, CODEERRORTYPE, "idamClientAuthentication", err,
                             "Protocol 11 Error (securityBlock #5)");
// Assuming the server_block is corrupted, replace with a clean copy to avoid concatonation problems
                server_block->idamerrorstack.nerrors = 0;
                break;
            }

// Flush (mark as at EOF) the input socket buffer (not all server state data may have been read - version dependent)

            xdrrec_eof(clientInput);

            IDAM_LOG(LOG_DEBUG, "idamClient: Server Block Received\n");
            printServerBlock(*server_block);

// Protocol Version: Lower of the client and server version numbers
// This defines the set of elements within data structures passed between client and server
// Must be the same on both sides of the socket

            if (client_block->version < server_block->version) protocolVersion = client_block->version;

// Check for FATAL Server Errors

            if (server_block->idamerrorstack.nerrors != 0) {
                err = 999;
                addIdamError(&idamerrorstack, CODEERRORTYPE, "idamClientAuthentication", err,
                             "Server Side Authentication Failed!");
                break;
            }

// Extract Ciphers

            securityBlock = &server_block->securityBlock;

            if (securityBlock->authenticationStep != authenticationStep - 1) {
                err = 999;
                addIdamError(&idamerrorstack, CODEERRORTYPE, "idamClientAuthentication", err,
                             "Authentication Step Inconsistency!");
                break;
            }

            client_ciphertext = securityBlock->client_ciphertext;
            server_ciphertext = securityBlock->server_ciphertext;
            client_ciphertextLength = securityBlock->client_ciphertextLength;
            server_ciphertextLength = securityBlock->server_ciphertextLength;

// Decrypt tokens (A, B) and Authenticate the Server

            err = udaAuthentication(authenticationStep, encryptionMethod,
                                    tokenType, tokenByteLength,
                                    publickey, privatekey,
                                    &client_mpiToken, &server_mpiToken,
                                    &client_ciphertext, &client_ciphertextLength,
                                    &server_ciphertext, &server_ciphertextLength);

            if (err != 0) {
                addIdamError(&idamerrorstack, CODEERRORTYPE, "idamClientAuthentication", err,
                             "Failed Authentication Step #5!");
                break;
            }

            free((void*)client_ciphertext);
            client_ciphertext = NULL;
            client_ciphertextLength = 0;
            free((void*)server_ciphertext);
            server_ciphertext = NULL;
            server_ciphertextLength = 0;

        } else

//---------------------------------------------------------------------------------------------------------------
// Step 6: Client encrypts passed token (B) with the server's public key (->EBSP), passes back to server    

        if (authenticationStep == 6) {

// Encrypt token (B)

            err = udaAuthentication(authenticationStep, encryptionMethod,
                                    tokenType, tokenByteLength,
                                    publickey, privatekey,
                                    &client_mpiToken, &server_mpiToken,
                                    &client_ciphertext, &client_ciphertextLength,
                                    &server_ciphertext, &server_ciphertextLength);

            if (err != 0) {
                addIdamError(&idamerrorstack, CODEERRORTYPE, "idamClientAuthentication", err,
                             "Failed Preparing Authentication Step #6!");
                break;
            }

// Send the encrypted token to the server via the CLIENT_BLOCK data structure

            securityBlock = &client_block->securityBlock;

            securityBlock->authenticationStep = authenticationStep;
            securityBlock->server_ciphertext = server_ciphertext;
            securityBlock->client_ciphertext = NULL;
            securityBlock->server_ciphertextLength = server_ciphertextLength;
            securityBlock->client_ciphertextLength = 0;

            protocol_id = PROTOCOL_CLIENT_BLOCK;

            if ((err = protocol2(clientOutput, protocol_id, XDR_SEND, NULL, &client_block)) != 0) {
                addIdamError(&idamerrorstack, CODEERRORTYPE, "idamClientAuthentication", err,
                             "Protocol 10 Error (securityBlock #6)");
                break;
            }

// Send to server

            if (!xdrrec_endofrecord(clientOutput, 1)) {
                err = PROTOCOL_ERROR_7;
                addIdamError(&idamerrorstack, CODEERRORTYPE, "idamClient", err, "Protocol 7 Error (Client Block #6)");
                break;
            }

            if (server_ciphertext != NULL) free((void*)server_ciphertext);
            server_ciphertext = NULL;
            server_ciphertextLength = 0;

            if (client_ciphertext != NULL) free((void*)client_ciphertext);
            client_ciphertext = NULL;
            client_ciphertextLength = 0;

        } else

//---------------------------------------------------------------------------------------------------------------
// Step 8: Server issues a new token (B) encrypted with the client's public key (->EBCP), passes to client
//         The next step is a repeat of steps 6,7,8 to ensure continuation of authorised data access.  

        if (authenticationStep == 8) {

// Receive the encrypted new token (B) from the server via the SERVER_BLOCK data structure      

#ifndef TESTIDAMSECURITY

            if (!xdrrec_skiprecord(clientInput)) {
                err = PROTOCOL_ERROR_5;
                addIdamError(&idamerrorstack, CODEERRORTYPE, "idamClient", err, "Protocol 5 Error (Server Block #8)");
                break;
            }

            protocol_id = PROTOCOL_SERVER_BLOCK;

            if ((err = protocol2(clientInput, protocol_id, XDR_RECEIVE, NULL, &server_block)) != 0) {
                addIdamError(&idamerrorstack, CODEERRORTYPE, "idamClientAuthentication", err,
                             "Protocol 11 Error (securityBlock #8)");
                break;
            }

// Flush (mark as at EOF) the input socket buffer (not all server state data may have been read - version dependent)

            xdrrec_eof(clientInput);

#endif

            IDAM_LOG(LOG_DEBUG, "idamClient: Server Block Received\n");
            printServerBlock(*server_block);

            if (server_block->idamerrorstack.nerrors > 0) {
                err = server_block->idamerrorstack.idamerror[0].code;        // Problem on the Server Side!
                break;
            }

            securityBlock = &server_block->securityBlock;

            if (securityBlock->authenticationStep != authenticationStep - 1) {
                err = 999;
                addIdamError(&idamerrorstack, CODEERRORTYPE, "idamClientAuthentication", err,
                             "Authentication Step Inconsistency!");
                break;
            }

            client_ciphertext = securityBlock->client_ciphertext;
            server_ciphertext = securityBlock->server_ciphertext;
            client_ciphertextLength = securityBlock->client_ciphertextLength;
            server_ciphertextLength = securityBlock->server_ciphertextLength;

// Decrypt token (B) using the User's private key

            err = udaAuthentication(authenticationStep, encryptionMethod,
                                    tokenType, tokenByteLength,
                                    publickey, privatekey,
                                    &client_mpiToken, &server_mpiToken,
                                    &client_ciphertext, &client_ciphertextLength,
                                    &server_ciphertext, &server_ciphertextLength);

            if (err != 0) {
                addIdamError(&idamerrorstack, CODEERRORTYPE, "idamClientAuthentication", err,
                             "Failed Authentication Step #8!");
                break;
            }

            securityBlock = &client_block->securityBlock;

            securityBlock->authenticationStep = authenticationStep;
            securityBlock->server_ciphertext = server_ciphertext;
            securityBlock->client_ciphertext = NULL;
            securityBlock->server_ciphertextLength = server_ciphertextLength;
            securityBlock->client_ciphertextLength = 0;

            client_ciphertext = NULL;
            client_ciphertextLength = 0;
            server_ciphertext = NULL;
            server_ciphertextLength = 0;

        } else

//---------------------------------------------------------------------------------------------------------------
// Step 9: Housekeeping 

        if (authenticationStep == 9) {
            if (privatekey != NULL) free((void*)privatekey);
            if (privatekey != NULL) free((void*)publickey);
            privatekey = NULL;
            publickey = NULL;
            if (client_mpiToken != NULL) gcry_mpi_release(client_mpiToken);
            if (server_mpiToken != NULL) gcry_mpi_release(server_mpiToken);
            if (client_ciphertext != NULL) free((void*)client_ciphertext);
            if (server_ciphertext != NULL) free((void*)server_ciphertext);
        }

//---------------------------------------------------------------------------------------------------------------

    } while (0);        // End of Error Trap

    if (err != 0) {
        if (client_mpiToken != NULL) gcry_mpi_release(client_mpiToken);
        if (server_mpiToken != NULL) gcry_mpi_release(server_mpiToken);
        if (client_ciphertext != NULL) free((void*)client_ciphertext);
        if (server_ciphertext != NULL) free((void*)server_ciphertext);
    }

    return err;
}
