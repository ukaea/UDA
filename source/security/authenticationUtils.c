#include "authenticationUtils.h"

#include <sys/stat.h>
#include <errno.h>

#include <clientserver/errorLog.h>

int testFilePermissions(const char* object)
{
    struct stat buffer;
    errno = 0;

    int rc = stat(object, &buffer);
    if (rc != 0 || errno != 0) {
        if (errno != 0) addIdamError(&idamerrorstack, SYSTEMERRORTYPE, "idamClientAuthentication", errno, "");
        THROW_ERROR(999, "Could not verify the user's private key directory's access permissions!");
    }
    if (buffer.st_mode & S_IRGRP ||
        buffer.st_mode & S_IWGRP ||
        buffer.st_mode & S_IXGRP ||
        buffer.st_mode & S_IROTH ||
        buffer.st_mode & S_IWOTH ||
        buffer.st_mode & S_IXOTH) {
        THROW_ERROR(999, "The user's private key directory is public: Your keys and certificate may be compromised!!!");
    }

    return 0;
}

void initSecurityBlock(SECURITY_BLOCK* str)
{
    str->structVersion = 1;
    str->encryptionMethod = 2;
    str->authenticationStep = 0;
    str->client_ciphertextLength = 0;
    str->client2_ciphertextLength = 0;
    str->server_ciphertextLength = 0;
    str->client_X509Length = 0;
    str->client2_X509Length = 0;
    str->client2_ciphertext = NULL;
    str->client_ciphertext = NULL;
    str->server_ciphertext = NULL;
    str->client_X509 = NULL;
    str->client2_X509 = NULL;
}

