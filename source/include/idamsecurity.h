
// IDAM Client Server User Authentication

// Change History

// 25June2013   dgm Original Version
//--------------------------------------------------------------------------------------------------------------------

#ifndef IdamSecurityInclude
#define IdamSecurityInclude

#ifdef __cplusplus
extern "C" {
#endif

#include <gcrypt.h>     // GNU libgcrypt library
#include <ksba.h>       // KSBA X509 certificate library
#include <cert.h>       // KSBA X509 certificate library

#define HASH_FNC ((void(*)(void *, const void *, size_t))gcry_md_write)
#define DIM(v)       (sizeof(v)/sizeof((v)[0]))
#define xtrymalloc(a)    gcry_malloc ((a))

// Authentication Methods

#define SHAREDPRIVATEKEY    1
#define ASYMMETRICKEY       2
#define DIGITALSIGNATURE    3
#define DIFFIEHELLMANKEY    4
#define ASYMMETRICKEY2      5

// Nonce Generation Methods

#define NONCEBYTELENGTH     30  //512   //800

#define NONCETEST       0
#define NONCEWEAKRANDOM     1
#define NONCESTRONGRANDOM   2
#define NONCESTRINGRANDOM   3

// Limits

#define IDAM_MAXKEY 4096

// Security standardised in this version onwards

#define IDAMSECURITYVERSION 7

#ifdef __cplusplus
}
#endif

#endif
