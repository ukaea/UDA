#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <errno.h>
#include <ksba.h>
#include <gcrypt.h>

#include <clientserver/errorLog.h>
#include <security/clientAuthentication.h>
#include <security/serverAuthentication.h>
#include <security/security.h>
#include <clientserver/initStructs.h>
#include <logging/logging.h>
#include <security/authenticationUtils.h>

IDAMERRORSTACK idamerrorstack;		// Local Error Stack

#define digitp(p)   (*(p) >= '0' && *(p) <= '9')

#define fail_if_err(a) do { if(a) {                                       \
                              fprintf (stderr, "%s:%d: KSBA error: %s\n", \
                              __FILE__, __LINE__, gpg_strerror(a));   \
                              exit (1); }                              \
                           } while(0)


#define fail_if_err2(f, a) do { if(a) {\
            fprintf (stderr, "%s:%d: KSBA error on file `%s': %s\n", \
                       __FILE__, __LINE__, (f), gpg_strerror(a));   \
                            exit (1); }                              \
                           } while(0)

#define xfree(a)  ksba_free (a)

// Extract a Public Key from a X509 certificate file and return an S-Expression 

int extractIdamX509SExpKey(ksba_cert_t cert, gcry_sexp_t* key_sexp)
{

    gcry_error_t errCode;
    int err = 0;

    ksba_sexp_t p;
    size_t n;

    if ((p = ksba_cert_get_public_key(cert)) == NULL) {
        err = 999;
        addIdamError(&idamerrorstack, CODEERRORTYPE, "extractIdamX509SExpKey", err, "Failure to get the Public key!");
        return err;
    }

    // Get the length of the canonical S-Expression (public key)

    if ((n = gcry_sexp_canon_len(p, 0, NULL, NULL)) == 0) {
        err = 999;
        addIdamError(&idamerrorstack, CODEERRORTYPE, "extractIdamX509SExpKey", err, "did not return a proper S-Exp!");
        ksba_free(p);
        return err;
    }

    // Create an internal S-Expression from the external representation

    if ((errCode = gcry_sexp_sscan(key_sexp, NULL, (char*)p, n)) != 0) {
        err = 999;
        addIdamError(&idamerrorstack, CODEERRORTYPE, "extractIdamX509SExpKey", err, "S-Exp creation failed!");
        ksba_free(p);
        return err;
    }

    ksba_free(p);

    return err;
}

void
print_hex(const unsigned char* p, size_t n)
{
    if (!p) {
        fputs("none", stdout);
    } else {
        for (; n; n--, p++)
            printf("%02X", *p);
    }
}

int main()
{
    int err = 0;

    CLIENT_BLOCK client_block;
    SERVER_BLOCK server_block;

    int clientVersion = 7;
    char* clientUsername = "jholloc";

    idamSetLogFile(UDA_LOG_DEBUG, stdout);
    idamSetLogFile(UDA_LOG_INFO, stdout);
    idamSetLogFile(UDA_LOG_WARN, stdout);
    idamSetLogFile(UDA_LOG_ERROR, stderr);
    idamSetLogLevel(UDA_LOG_DEBUG);

    initClientBlock(&client_block, clientVersion, clientUsername);
    initServerBlock(&server_block, 0);

    initSecurityBlock(&client_block.securityBlock);
    initSecurityBlock(&server_block.securityBlock);

    initIdamErrorStack(&idamerrorstack);

    do {        // Start of Error Trap

        //-----------------------------------------------------------------------------------------------------------------------
        // Client step:
        // generate token A and encrypt with the server public key
        // Send: client certificate, encrypted token A

        USERDEFINEDTYPELIST userdefinedtypelist = { 0 };
        LOGMALLOCLIST logmalloclist = { 0 };
        AUTHENTICATION_STEP authenticationStep = CLIENT_ISSUE_TOKEN;   // Client Authentication

        if ((err = clientAuthentication(&client_block, &server_block, &logmalloclist, &userdefinedtypelist, authenticationStep)) != 0) {
            addIdamError(&idamerrorstack, CODEERRORTYPE, "idamClient", err,
                         "Client or Server Authentication Failed #1");
            break;
        }

        //-----------------------------------------------------------------------------------------------------------------------
        // Server steps:
        //
        // Test date validity of X509 certificate				=> not expired
        // Check database to verify not revoked	(not implemented)		=> not revoked
        //
        // Test X509 certificate signature using the CA's public key		=> has a valid certificate signed by UKAEA
        // Decrypt token A with the server's private key
        // Encrypt token A with the client's public key from their X509 certificate
        //
        // Generate new token B (fixed or nonce) and encrypt with the client's public key
        // Send encrypted tokens A, B					=> mutual proof each has valid private keys to match public keys

        authenticationStep = SERVER_DECRYPT_CLIENT_TOKEN;

        if ((err = serverAuthentication(&client_block, &server_block, &logmalloclist, &userdefinedtypelist, authenticationStep)) != 0) {
            addIdamError(&idamerrorstack, CODEERRORTYPE, "idamServer", err,
                         "Client or Server Authentication Failed #2");
            break;
        }

        authenticationStep = SERVER_ENCRYPT_CLIENT_TOKEN;

        if ((err = serverAuthentication(&client_block, &server_block, &logmalloclist, &userdefinedtypelist, authenticationStep)) != 0) {
            addIdamError(&idamerrorstack, CODEERRORTYPE, "idamServer", err,
                         "Client or Server Authentication Failed #3");
            break;
        }

        authenticationStep = SERVER_ISSUE_TOKEN;

        if ((err = serverAuthentication(&client_block, &server_block, &logmalloclist, &userdefinedtypelist, authenticationStep)) != 0) {
            addIdamError(&idamerrorstack, CODEERRORTYPE, "idamServer", err,
                         "Client or Server Authentication Failed #4");
            break;
        }

        //-----------------------------------------------------------------------------------------------------------------------
        // Client step:
        // Decrypt tokens A, B
        // Test token A is identical to that sent in step 1		=> proof server has a valid private key

        authenticationStep = CLIENT_DECRYPT_SERVER_TOKEN;    // Server Authentication Completed

        if ((err = clientAuthentication(&client_block, &server_block, &logmalloclist, &userdefinedtypelist, authenticationStep)) != 0) {
            addIdamError(&idamerrorstack, CODEERRORTYPE, "idamClient", err,
                         "Client or Server Authentication Failed #5");
            break;
        }

        //-----------------------------------------------------------------------------------------------------------------------
        // All subsequent steps exchange the server generated nonce token B
        //-----------------------------------------------------------------------------------------------------------------------
        // Client step:
        // Encrypt token B with the server public key

        authenticationStep = CLIENT_ENCRYPT_SERVER_TOKEN;    // Client Authentication Completed

        if ((err = clientAuthentication(&client_block, &server_block, &logmalloclist, &userdefinedtypelist, authenticationStep)) != 0) {
            addIdamError(&idamerrorstack, CODEERRORTYPE, "idamClient", err,
                         "Client or Server Authentication Failed #6");
            break;
        }

        //-----------------------------------------------------------------------------------------------------------------------
        // Server step:
        // Decrypt token B with the server's private key
        // Test token B is identical to that sent in step 4		=> maintain mutual authentication
        // Generate a new nonce token B and encrypt with the client public key

        authenticationStep = SERVER_VERIFY_TOKEN;

        if ((err = serverAuthentication(&client_block, &server_block, &logmalloclist, &userdefinedtypelist, authenticationStep)) != 0) {
            addIdamError(&idamerrorstack, CODEERRORTYPE, "idamServer", err,
                         "Client or Server Authentication Failed #7");
            break;
        }

        //-----------------------------------------------------------------------------------------------------------------------
        // Client step: Maintain proof of identity
        // Decrypt token B with the clients's private key
        // Encrypt with the server's public key

        //        authenticationStep = 8;            // Paired with Server step #7
        //
        //        if ((err = clientAuthentication(&client_block, &server_block, authenticationStep)) != 0) {
        //            addIdamError(&idamerrorstack, CODEERRORTYPE, "idamClient", err,
        //                         "Client or Server Authentication Failed #8");
        //            break;
        //        }

        fprintf(stdout, "\n\n****  Mutual Authentication completed  ****\n\n");

        //        if (client_block.securityBlock.server_ciphertext != NULL)free(client_block.securityBlock.server_ciphertext);
        //        client_block.securityBlock.server_ciphertextLength = 0;
        //        client_block.securityBlock.server_ciphertext = NULL;

    } while (0);        // End of Error trap

    printIdamErrorStack(idamerrorstack);

    return 0;
}
  
