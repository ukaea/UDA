#include "authenticationUtils.h"

#include <cerrno>
#include <sys/stat.h>

#include "clientserver/errorLog.h"

int testFilePermissions(const char* object)
{
    struct stat buffer;
    int rc, err = 0;

    rc = stat(object, &buffer);
    if (rc != 0 || errno != 0) {
        err = 999;
        if (errno != 0) {
            add_error(UDA_SYSTEM_ERROR_TYPE, "testSecurityFilePermissions", errno, "");
        }
        add_error(UDA_CODE_ERROR_TYPE, "clientAuthentication", err,
                  "Could not verify the user's private key directory's access permissions!");
        return err;
    }
    if (buffer.st_mode & S_IRGRP || buffer.st_mode & S_IWGRP || buffer.st_mode & S_IXGRP || buffer.st_mode & S_IROTH ||
        buffer.st_mode & S_IWOTH || buffer.st_mode & S_IXOTH) {
        err = 999;
        add_error(UDA_CODE_ERROR_TYPE, "testSecurityFilePermissions", err,
                  "The user's private key directory is public: Your keys and certificate may be compromised!!!");
        return err;
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
    str->client2_ciphertext = nullptr;
    str->client_ciphertext = nullptr;
    str->server_ciphertext = nullptr;
    str->client_X509 = nullptr;
    str->client2_X509 = nullptr;
}
