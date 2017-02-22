#include "serverAuthentication.h"

#include <stdlib.h>
#include <string.h>
#include <gcrypt.h>
#include <ksba.h>

#include <server/udaServer.h>
#include <clientserver/printStructs.h>
#include <clientserver/errorLog.h>
#include <clientserver/xdrlib.h>
#include <clientserver/protocol.h>
#include <clientserver/udaErrors.h>
#include <logging/logging.h>

#include "security.h"

int serverAuthentication(CLIENT_BLOCK* client_block, SERVER_BLOCK* server_block, unsigned short authenticationStep)
{
    static short initAuthenticationKeys = 1;

    static gcry_sexp_t privatekey = NULL;       // Server's
    static gcry_sexp_t publickey = NULL;        // Client's

    SECURITY_BLOCK* securityBlock = NULL;       // passed between Client and Server with authentication challenges

    ksba_cert_t clientCert = NULL, CACert = NULL;

    gcry_error_t errCode;
    int err = 0, rc;

    int protocol_id;

//---------------------------------------------------------------------------------------------------------------
// Read the CLIENT_BLOCK and client x509 certificate

    if (authenticationStep == 2) {

        // Receive the client block, respecting earlier protocol versions

            IDAM_LOG(LOG_DEBUG, "Waiting for Initial Client Block\n");

        if (!(rc = xdrrec_skiprecord(serverInput))) {
            err = PROTOCOL_ERROR_5;
                IDAM_LOG(LOG_DEBUG, "xdrrec_skiprecord error!\n");
            addIdamError(&idamerrorstack, CODEERRORTYPE, "idamServer", err, "Protocol 5 Error (Client Block #2)");
        } else {

            protocol_id = PROTOCOL_CLIENT_BLOCK;        // Recieve Client Block

            if ((err = protocol2(serverInput, protocol_id, XDR_RECEIVE, NULL, client_block)) != 0) {
                addIdamError(&idamerrorstack, CODEERRORTYPE, "idamServer", err, "Protocol 10 Error (Client Block #2)");
                    IDAM_LOG(LOG_DEBUG, "protocol error! Client Block not received!\n");
            }

            if (err == 0) {
                IDAM_LOG(LOG_DEBUG, "Initial Client Block received\n");
                printClientBlock(*client_block);
            }
        }

        if (err != 0) return err;

// Flush (mark as at EOF) the input socket buffer (not all client state data may have been read - version dependent)

        xdrrec_eof(serverInput);

// Protocol Version: Lower of the client and server version numbers
// This defines the set of elements within data structures passed between client and server
// Must be the same on both sides of the socket
// set in xdr_client

        securityBlock = &client_block->securityBlock;

        //x509Block.clientCertificate  = securityBlock->client_X509;
        //x509Block.client2Certificate = securityBlock->client2_X509;			// Passed onwards down the server chain

        //x509Block.clientCertificateLength  = securityBlock->client_X509Length;
        //x509Block.client2CertificateLength = securityBlock->client2_X509Length;
    }

//---------------------------------------------------------------------------------------------------------------
// Read the Server's Private Key (from a PEM file) and the User's Public Key (from the passed x509 cert)
// Read the Certificate Authority's Public Key to check signature of the client's certificate
// Keys have S-Expressions format

// Key locations are identified from an environment variable
// The client's public key is from a x509 certificate (check date validity + CA signature)

    if (initAuthenticationKeys) {

        char* env = NULL;
        size_t len = 0;

        char* serverPrivateKeyFile = NULL;
        char* CACertFile = NULL;

        if ((env = getenv("UDA_SERVER_CERTIFICATE")) != NULL) {    // Directory with certificates and key files
            len = strlen(env) + 56;
            serverPrivateKeyFile = (char*)malloc(len * sizeof(unsigned char));
            CACertFile = (char*)malloc(len * sizeof(unsigned char));

            sprintf((char*)serverPrivateKeyFile, "%s/serverskey.pem", env);  // Server's
            sprintf((char*)CACertFile, "%s/carootX509.der", env); // CA Certificate for signature verification
        } else {
            err = 999;
            addIdamError(&idamerrorstack, CODEERRORTYPE, "idamServerAuthentication", err,
                         "The Server's User Authentication Environment is Incomplete - set the Certificate Directory path!");
            return err;
        }

        do {    // Error Trap

// Read the Client's certificate, check validity, extract the public key 

            if (securityBlock->client_X509 != NULL) {
                if ((err = makeX509CertObject(securityBlock->client_X509, securityBlock->client_X509Length,
                                              &clientCert)) != 0) {
                                                      break;
                }
                if ((err = testX509Dates(clientCert)) != 0) break;                // Check the Certificate Validity
                if ((err = extractX509SExpKey(clientCert, &publickey)) != 0) {
                    break;
                }        // get the Public key from an X509 certificate
            }

// get the server's Private key from a PEM file (for decryption) and convert to S-Expression

            if ((err = importPEMPrivateKey(serverPrivateKeyFile, &privatekey)) != 0) break;

// Read the CA's certificate, check date validity 

            if (CACertFile != NULL) {
                if ((err = importX509Reader(CACertFile, &CACert)) != 0) break;
                if ((err = testX509Dates(CACert)) != 0) break;
            }

// Test the server's private key for consistency

            if ((errCode = gcry_pk_testkey(privatekey)) != 0) {
                err = 999;
                addIdamError(&idamerrorstack, CODEERRORTYPE, "idamServerAuthentication", err,
                             "The Server's Private Authentication Key is Invalid!");
                break;
            }

// Verify the client certificate's signature using the CA's public key

            if ((err = checkX509Signature(CACert, clientCert)) != 0) break;

            ksba_cert_release(clientCert);
            ksba_cert_release(CACert);
            clientCert = NULL;
            CACert = NULL;

        } while (0);

        if (serverPrivateKeyFile != NULL) free(serverPrivateKeyFile);
        if (CACertFile != NULL) free(CACertFile);

        if (err != 0) {
            if (privatekey != NULL) gcry_sexp_release(privatekey);
            if (publickey != NULL) gcry_sexp_release(publickey);
            if (clientCert != NULL) ksba_cert_release(clientCert);
            if (CACert != NULL) ksba_cert_release(CACert);        // BUG in ksba library# ksba_free !
            privatekey = NULL;    // These are declared as static so ensure reset when an error occurs
            publickey = NULL;
            return err;
        }

        initAuthenticationKeys = 0;
    }

//---------------------------------------------------------------------------------------------------------------
// Authenticate both Client and Server

    unsigned short encryptionMethod = ASYMMETRICKEY;
    unsigned short tokenByteLength = NONCEBYTELENGTH;        // System problem when >~ 110 !
    unsigned short tokenType = NONCESTRONGRANDOM;    // NONCESTRINGRANDOM; // NONCEWEAKRANDOM; // NONCETEST; //

    static gcry_mpi_t client_mpiToken = NULL;
    static gcry_mpi_t server_mpiToken = NULL;

    unsigned char* client_ciphertext = NULL;
    unsigned char* server_ciphertext = NULL;
    unsigned short client_ciphertextLength = 0;
    unsigned short server_ciphertextLength = 0;

    do {        // Error Trap

        if (authenticationStep == 2) {

            //---------------------------------------------------------------------------------------------------------------
            // Step 2: Receive the Client's token cipher (EASP) and decrypt with the server's private key (->A)

            // Already received the encrypted token (A) from the client

            securityBlock = &client_block->securityBlock;

            if (securityBlock->authenticationStep != authenticationStep - 1) {
                err = 999;
                addIdamError(&idamerrorstack, CODEERRORTYPE, "idamServerAuthentication", err,
                             "Authentication Step #2 Inconsistency!");
                break;
            }

            client_ciphertext = securityBlock->client_ciphertext;
            server_ciphertext = securityBlock->server_ciphertext;

            client_ciphertextLength = securityBlock->client_ciphertextLength;
            server_ciphertextLength = securityBlock->server_ciphertextLength;

            // Decrypt token (A)

            err = udaAuthentication(authenticationStep, encryptionMethod,
                                    tokenType, tokenByteLength,
                                    publickey, privatekey,
                                    &client_mpiToken, &server_mpiToken,
                                    &client_ciphertext, &client_ciphertextLength,
                                    &server_ciphertext, &server_ciphertextLength);

            if (err != 0) {
                addIdamError(&idamerrorstack, CODEERRORTYPE, "idamServerAuthentication", err,
                             "Failed Decryption Step #2!");
                break;
            }

            free((void*)client_ciphertext);
            client_ciphertext = NULL;
            client_ciphertextLength = 0;
            free((void*)server_ciphertext);
            server_ciphertext = NULL;
            server_ciphertextLength = 0;

        } else if (authenticationStep == 3) {

            //---------------------------------------------------------------------------------------------------------------
            // Step 3: Server encrypts the client token (A) with the client's public key (->EACP)

            // Encrypt token (A)

            err = udaAuthentication(authenticationStep, encryptionMethod,
                                    tokenType, tokenByteLength,
                                    publickey, privatekey,
                                    &client_mpiToken, &server_mpiToken,
                                    &client_ciphertext, &client_ciphertextLength,
                                    &server_ciphertext, &server_ciphertextLength);

            if (err != 0) {
                addIdamError(&idamerrorstack, CODEERRORTYPE, "idamServerAuthentication", err,
                             "Failed Encryption Step #3!");
                break;
            }

            securityBlock = &server_block->securityBlock;
            initSecurityBlock(securityBlock);

            securityBlock->client_ciphertext = client_ciphertext;
            securityBlock->client_ciphertextLength = client_ciphertextLength;

            gcry_mpi_release(client_mpiToken);

        } else if (authenticationStep == 4) {

            //---------------------------------------------------------------------------------------------------------------
            // Step 4: Server issues a new token (B) also encrypted with the client's public key (->EBCP), passes both to client.

            // Generate new Token and Encrypt (B)

            err = udaAuthentication(authenticationStep, encryptionMethod,
                                    tokenType, tokenByteLength,
                                    publickey, privatekey,
                                    &client_mpiToken, &server_mpiToken,
                                    &client_ciphertext, &client_ciphertextLength,
                                    &server_ciphertext, &server_ciphertextLength);

            if (err != 0) {
                addIdamError(&idamerrorstack, CODEERRORTYPE, "idamServerAuthentication", err,
                             "Failed Encryption Step #4!");
                break;
            }

            // Send the encrypted token to the server

            securityBlock = &server_block->securityBlock;

            securityBlock->authenticationStep = authenticationStep;
            securityBlock->server_ciphertext = server_ciphertext;
            securityBlock->server_ciphertextLength = server_ciphertextLength;

            protocol_id = PROTOCOL_SERVER_BLOCK;

            if ((err = protocol2(serverOutput, protocol_id, XDR_SEND, NULL, server_block)) != 0) {
                addIdamError(&idamerrorstack, CODEERRORTYPE, "idamServerAuthentication", err,
                             "Protocol 10 Error (securityBlock #4)");
                break;
            }

            if (!(rc = xdrrec_endofrecord(serverOutput, 1))) {
                err = PROTOCOL_ERROR_7;
                addIdamError(&idamerrorstack, CODEERRORTYPE, "idamServerAuthentication", err,
                             "Protocol 7 Error (Server Block)");
                break;
            }

            client_ciphertext = NULL;
            client_ciphertextLength = 0;
            server_ciphertext = NULL;
            server_ciphertextLength = 0;

        } else if (authenticationStep == 7) {

            //---------------------------------------------------------------------------------------------------------------
            // Step 7: Server decrypts the passed cipher (EBSP) with the server's private key (->B) and checks
            //	   token (B) => client authenticated

            // Receive the encrypted token (B) from the client

            protocol_id = PROTOCOL_CLIENT_BLOCK;

            if (!(rc = xdrrec_skiprecord(serverInput))) {
                err = PROTOCOL_ERROR_5;
                    IDAM_LOG(LOG_DEBUG, "xdrrec_skiprecord error!\n");
                addIdamError(&idamerrorstack, CODEERRORTYPE, "idamServer", err, "Protocol 5 Error (Client Block #7)");
                break;
            }

            if ((err = protocol2(serverInput, protocol_id, XDR_RECEIVE, NULL, client_block)) != 0) {
                addIdamError(&idamerrorstack, CODEERRORTYPE, "idamServerAuthentication", err,
                             "Protocol 11 Error (securityBlock #7)");
                break;
            }

            // Flush (mark as at EOF) the input socket buffer (not all client state data may have been read - version dependent)

            xdrrec_eof(serverInput);

            securityBlock = &client_block->securityBlock;

            if (securityBlock->authenticationStep != authenticationStep - 1) {
                err = 999;
                addIdamError(&idamerrorstack, CODEERRORTYPE, "idamServerAuthentication", err,
                             "Authentication Step Inconsistency!");
                break;
            }

            client_ciphertext = securityBlock->client_ciphertext;
            server_ciphertext = securityBlock->server_ciphertext;
            client_ciphertextLength = securityBlock->client_ciphertextLength;
            server_ciphertextLength = securityBlock->server_ciphertextLength;

            // Decrypt token (B) and Authenticate the Client

            err = udaAuthentication(authenticationStep, encryptionMethod,
                                    tokenType, tokenByteLength,
                                    publickey, privatekey,
                                    &client_mpiToken, &server_mpiToken,
                                    &client_ciphertext, &client_ciphertextLength,
                                    &server_ciphertext, &server_ciphertextLength);

            if (err != 0) {
                addIdamError(&idamerrorstack, CODEERRORTYPE, "idamServerAuthentication", err,
                             "Failed Authentication Step #7!");
                break;
            }

            // Send the encrypted token B to the client

            securityBlock = &server_block->securityBlock;

            securityBlock->authenticationStep = authenticationStep;
            securityBlock->server_ciphertext = server_ciphertext;
            securityBlock->server_ciphertextLength = server_ciphertextLength;
            securityBlock->client_ciphertext = client_ciphertext;
            securityBlock->client_ciphertextLength = client_ciphertextLength;

            protocol_id = PROTOCOL_SERVER_BLOCK;

            if ((err = protocol2(serverOutput, protocol_id, XDR_SEND, NULL, server_block)) != 0) {
                addIdamError(&idamerrorstack, CODEERRORTYPE, "idamServerAuthentication", err,
                             "Protocol 10 Error (securityBlock #7)");
                break;
            }

            if (!(rc = xdrrec_endofrecord(serverOutput, 1))) {
                err = PROTOCOL_ERROR_7;
                addIdamError(&idamerrorstack, CODEERRORTYPE, "idamServerAuthentication", err,
                             "Protocol 7 Error (Server Block #7)");
                break;
            }

            client_ciphertext = NULL;
            client_ciphertextLength = 0;
            server_ciphertext = NULL;
            server_ciphertextLength = 0;
        } else if (authenticationStep == 9) {

            //---------------------------------------------------------------------------------------------------------------
            // Step 9: Housekeeping
            if (privatekey != NULL) free((void*)privatekey);
            if (privatekey != NULL) free((void*)publickey);
            privatekey = NULL;
            publickey = NULL;
            if (client_mpiToken != NULL) gcry_mpi_release(client_mpiToken);
            if (server_mpiToken != NULL) gcry_mpi_release(server_mpiToken);
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
