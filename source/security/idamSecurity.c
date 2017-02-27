
// IDAM Client Server Identity Authentication
//
// Architecture models: (single/multiple tier network; mesh or bridge network)
//
// a)	client connects to a server
// b)	client connects to a proxy that connects to a server
// c)	client connects to a server that connects to a server etc. (multi-tier connection)
// d)	client connects to a proxy that connects to a server that connects to a server etc. (multi-tier connection)
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
//	1) The User has one identity. Intermediate servers don't authenticate with each other or the client and 
//         pass through the claim of identity to the final server. Authentication occurs between the final server 
//         and the user. 
//	2) The User has one identity. Intermediate servers authenticate with each other but don't authenticate 
//         with the client. They pass through the claim of identity to the final server. Authentication occurs 
//         between the final server and the user.
//	3) The User has one identity. All servers authenticate with the client.  **** Not adopted ****
//	4) The User has two identities. The first server authenticates with the first user identity.
//         Intermediate servers don't authenticate with each other or the client and 
//         pass through the claim of identity to the final server. Authentication occurs between the final server 
//         and the user (second identity).  
//	5) The User has two identities. The first server authenticates with the first user identity. 
//         Intermediate servers authenticate with each other but don't authenticate with the client. They
//         pass through the second claim of identity to the final server. Authentication occurs between the final server 
//         and the user (second identity).
//	6) The User has n identities. Each of n servers authenticates with the n user identities. **** Not adopted ****
// d) Proxy does not authenticate and passes through the claims of identity to the multiple servers as c).
// 
// Steps:
//
// 1> 	Client issues a token (A), encrypts with the server's public key (->EASP), passes to server (with X.509)
//	Server's public key could be obtained from a X.509 certificate (authenticated using signature and CA public key)
// 2>	Server decrypts the passed cipher (EASP) with the server's private key (->A) 
// 3>	Server encrypts the client token (A) with the client's public key (->EACP)
//	Public key could be obtained from a X.509 certificate (authenticated using signature and CA public key)
//	Public key could alternatively be obtained a user database.
// 4>	Server issues a new token (B) also encrypted with the client's public key (->EBCP), passes 
//	both to client. 
// 5>	Client decrypts the passed ciphers (EACP, EBCP) with the client's private key (->A, ->B) and 
//	checks token (A) => server authenticated (in addition to the X.509 certificate signature check)
// 6>	Client encrypts passed token (B) with the server's public key (->EBSP), passes to server
// 7>	Server decrypts the passed cipher (EBSP) with the server's private key (->B) and checks 
//	token (B) => client authenticated (in addition to the X.509 certificate signature check).
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
// Change History

// 25June2013	dgm	Original Version
//--------------------------------------------------------------------------------------------------------------------

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <errno.h>

#include "idamclientserver.h"
#include "idamclientserverpublic.h"
#include "idamsecurity.h"

#ifdef TESTIDAMSECURITY
static IDAMERRORSTACK idamerrorstack;		// Local Error Stack
static FILE *dbgout = NULL;
static int debugon = 1;

#include "/home/dgm/IDAM/source/clientserver/idamErrorLog.c"
#include "/home/dgm/IDAM/source/clientserver/TrimString.c"
#include "/home/dgm/IDAM/source/clientserver/initStructs.c"
#include "/home/dgm/IDAM/source/clientserver/printStructs.c"

#include "./idamClientAuthentication.c"
#include "./idamServerAuthentication.c"

#endif

/* A sample 1024 bit RSA key used for the selftests.  */
static const char sample_secret_key[] =
        "(private-key"
                " (rsa"
                "  (n #00e0ce96f90b6c9e02f3922beada93fe50a875eac6bcc18bb9a9cf2e84965caa"
                "      2d1ff95a7f542465c6c0c19d276e4526ce048868a7a914fd343cc3a87dd74291"
                "      ffc565506d5bbb25cbac6a0e2dd1f8bcaab0d4a29c2f37c950f363484bf269f7"
                "      891440464baf79827e03a36e70b814938eebdc63e964247be75dc58b014b7ea251#)"
                "  (e #010001#)"
                "  (d #046129f2489d71579be0a75fe029bd6cdb574ebf57ea8a5b0fda942cab943b11"
                "      7d7bb95e5d28875e0f9fc5fcc06a72f6d502464dabded78ef6b716177b83d5bd"
                "      c543dc5d3fed932e59f5897e92e6f58a0f33424106a3b6fa2cbf877510e4ac21"
                "      c3ee47851e97d12996222ac3566d4ccb0b83d164074abf7de655fc2446da1781#)"
                "  (p #00e861b700e17e8afe6837e7512e35b6ca11d0ae47d8b85161c67baf64377213"
                "      fe52d772f2035b3ca830af41d8a4120e1c1c70d12cc22f00d28d31dd48a8d424f1#)"
                "  (q #00f7a7ca5367c661f8e62df34f0d05c10c88e5492348dd7bddc942c9a8f369f9"
                "      35a07785d2db805215ed786e4285df1658eed3ce84f469b81b50d358407b4ad361#)"
                "  (u #304559a9ead56d2309d203811a641bb1a09626bc8eb36fffa23c968ec5bd891e"
                "      ebbafc73ae666e01ba7c8990bae06cc2bbe10b75e69fcacb353a6473079d8e9b#)))";

/* A sample 1024 bit RSA key used for the selftests (public only).  */
static const char sample_public_key[] =
        "(public-key"
                " (rsa"
                "  (n #00e0ce96f90b6c9e02f3922beada93fe50a875eac6bcc18bb9a9cf2e84965caa"
                "      2d1ff95a7f542465c6c0c19d276e4526ce048868a7a914fd343cc3a87dd74291"
                "      ffc565506d5bbb25cbac6a0e2dd1f8bcaab0d4a29c2f37c950f363484bf269f7"
                "      891440464baf79827e03a36e70b814938eebdc63e964247be75dc58b014b7ea251#)"
                "  (e #010001#)))";

#define digitp(p)   (*(p) >= '0' && *(p) <= '9')

#ifdef TESTIDAMSECURITY

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
#endif

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

static void
print_sexp(ksba_const_sexp_t p)
{
    int level = 0;

    if (!p) {
        fputs("[none]", stdout);
    } else {
        for (;;) {
            if (*p == '(') {
                putchar(*p);
                p++;
                level++;
            } else if (*p == ')') {
                putchar(*p);
                p++;
                if (--level <= 0) {
                    return;
                }
            } else if (!digitp (p)) {
                fputs("[invalid s-exp]", stdout);
                return;
            } else {
                char* endp;
                unsigned long n, i;
                int need_hex;

                n = strtoul(p, &endp, 10);
                p = endp;
                if (*p != ':') {
                    fputs("[invalid s-exp]", stdout);
                    return;
                }
                p++;
                for (i = 0; i < n; i++)
                    if (!((p[i] >= 'A' && p[i] <= 'Z')
                          || (p[i] >= 'a' && p[i] <= 'z')
                          || (p[i] >= '0' && p[i] <= '9')
                          || p[i] == '-'
                          || p[i] == '.')) {
                              break;
                    }
                need_hex = (i < n);
                if (!n /* n==0 is not allowed, but anyway.  */
                    || (!need_hex
                        && !((*p >= 'A' && *p <= 'Z') || (*p >= 'a' && *p <= 'z')))) {
                            printf("%lu:", n);
                }

                if (need_hex) {
                    putchar('#');
                    for (; n; n--, p++)
                        printf("%02X", *p);
                    putchar('#');
                } else {
                    for (; n; n--, p++)
                        putchar(*p);
                }
                putchar(' ');
            }
        }
    }
}

/* Return the description for OID; if no description is available
   NULL is returned. */
/*
static const char *
get_oid_desc (const char *oid)
{
  int i;

  if (oid)
    for (i=0; oidtranstbl[i].oid; i++)
      if (STR_EQUALS (oidtranstbl[i].oid, oid))
        return oidtranstbl[i].desc;
  return NULL;
}
*/

static void
print_time(ksba_isotime_t t)
{
    if (!t || !*t) {
        fputs("none", stdout);
    } else {
        printf("%.4s-%.2s-%.2s %.2s:%.2s:%s", t, t + 4, t + 6, t + 9, t + 11, t + 13);
    }
}

static void
print_dn(char* p)
{

    if (!p) {
        fputs("error", stdout);
    } else {
        printf("`%s'", p);
    }
}


static void
print_names(int indent, ksba_name_t name)
{
    int idx;
    const char* s;
    int indent_all;

    if ((indent_all = (indent < 0))) {
        indent = -indent;
    }

    if (!name) {
        fputs("none\n", stdout);
        return;
    }

    for (idx = 0; (s = ksba_name_enum(name, idx)); idx++) {
        char* p = ksba_name_get_uri(name, idx);
        printf("%*s%s\n", idx || indent_all ? indent : 0, "", p ? p : s);
        xfree (p);
    }
}

#include "testCert.c"

int main()
{

    int err = 0;
    dbgout = stdout;
    errout = stderr;

    CLIENT_BLOCK client_block;
    SERVER_BLOCK server_block;

    int clientVersion = 7;
    char* clientUsername = "dgm";

    initClientBlock(&client_block, clientVersion, clientUsername);
    initServerBlock(&server_block, 0);

    initSecurityBlock(&client_block.securityBlock);
    initSecurityBlock(&server_block.securityBlock);

    do {        // Start of Error Trap

//-----------------------------------------------------------------------------------------------------------------------
// Client step:
// generate token A and encrypt with the server public key
// Send: client certificate, encrypted token A      

        unsigned short authenticationStep = 1;   // Client Authentication

        if ((err = idamClientAuthentication(&client_block, &server_block, authenticationStep)) != 0) {
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

        authenticationStep = 2;

        if ((err = idamServerAuthentication(&client_block, &server_block, authenticationStep)) != 0) {
            addIdamError(&idamerrorstack, CODEERRORTYPE, "idamServer", err,
                         "Client or Server Authentication Failed #2");
            break;
        }

        authenticationStep = 3;

        if ((err = idamServerAuthentication(&client_block, &server_block, authenticationStep)) != 0) {
            addIdamError(&idamerrorstack, CODEERRORTYPE, "idamServer", err,
                         "Client or Server Authentication Failed #3");
            break;
        }

        authenticationStep = 4;

        if ((err = idamServerAuthentication(&client_block, &server_block, authenticationStep)) != 0) {
            addIdamError(&idamerrorstack, CODEERRORTYPE, "idamServer", err,
                         "Client or Server Authentication Failed #4");
            break;
        }

//-----------------------------------------------------------------------------------------------------------------------
// Client step:
// Decrypt tokens A, B
// Test token A is identical to that sent in step 1		=> proof server has a valid private key

        authenticationStep = 5;    // Server Authentication Completed

        if ((err = idamClientAuthentication(&client_block, &server_block, authenticationStep)) != 0) {
            addIdamError(&idamerrorstack, CODEERRORTYPE, "idamClient", err,
                         "Client or Server Authentication Failed #5");
            break;
        }

//-----------------------------------------------------------------------------------------------------------------------
// All subsequent steps exchange the server generated nonce token B
//-----------------------------------------------------------------------------------------------------------------------
// Client step:
// Encrypt token B with the server public key

        authenticationStep = 6;    // Client Authentication Completed

        if ((err = idamClientAuthentication(&client_block, &server_block, authenticationStep)) != 0) {
            addIdamError(&idamerrorstack, CODEERRORTYPE, "idamClient", err,
                         "Client or Server Authentication Failed #6");
            break;
        }

//-----------------------------------------------------------------------------------------------------------------------
// Server step:
// Decrypt token B with the server's private key
// Test token B is identical to that sent in step 4		=> maintain mutual authentication
// Generate a new nonce token B and encrypt with the client public key

        authenticationStep = 7;

        if ((err = idamServerAuthentication(&client_block, &server_block, authenticationStep)) != 0) {
            addIdamError(&idamerrorstack, CODEERRORTYPE, "idamServer", err,
                         "Client or Server Authentication Failed #7");
            break;
        }

//-----------------------------------------------------------------------------------------------------------------------
// Client step: Maintain proof of identity
// Decrypt token B with the clients's private key
// Encrypt with the server's public key

        authenticationStep = 8;            // Paired with Server step #7

        if ((err = idamClientAuthentication(&client_block, &server_block, authenticationStep)) != 0) {
            addIdamError(&idamerrorstack, CODEERRORTYPE, "idamClient", err,
                         "Client or Server Authentication Failed #8");
            break;
        }

        fprintf(stdout, "\n\n****  Mutual Authentication completed  ****\n\n");

        if (client_block.securityBlock.server_ciphertext != NULL)free(client_block.securityBlock.server_ciphertext);
        client_block.securityBlock.server_ciphertextLength = 0;
        client_block.securityBlock.server_ciphertext = NULL;

    } while (0);        // End of Error trap

    return 0;
}
  