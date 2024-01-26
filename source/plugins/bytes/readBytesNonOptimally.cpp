#include "readBytesNonOptimally.h"

#include <cerrno>
#include <cstdlib>
#include <sstream>
#include <iomanip>
#include <openssl/evp.h>

#define BYTEFILEDOESNOTEXIST 100001
#define BYTEFILEATTRIBUTEERROR 100002
#define BYTEFILEISNOTREGULAR 100003
#define BYTEFILEOPENERROR 100004
#define BYTEFILEHEAPERROR 100005
#define BYTEFILEMD5ERROR 100006
#define BYTEFILEMD5DIFF 100007

namespace {
int is_legal_file_path(const char *str) {
    // Basic check that the filename complies with good naming practice - some protection against malign embedded code!
    // Test against the Portable Filename Character Set A-Z, a-z, 0-9, <period>, <underscore> and <hyphen> and <plus>
    // Include <space> and back-slash for windows filenames only, forward-slash for the path seperator and $ for
    // environment variables

    // The API source argument can also be a server based source containing a ':' character
    // The delimiter characters separating the device or format name from the source should have been split off of the
    // path
    //

    const char *tst = str;
    while (*tst != '\0') {
        if ((*tst >= '0' && *tst <= '9') || (*tst >= 'A' && *tst <= 'Z') || (*tst >= 'a' && *tst <= 'z')) {
            tst++;
            continue;
        }

        if (strchr("_-+./$:", *tst) != nullptr) {
            tst++;
            continue;
        }

#ifdef _WIN32
        if (*tst == ' ' || *tst == '\\') {
            tst++;
            continue;
        }
#endif
        return 0; // Error - not compliant!
    }
    return 1;
}
}

std::string hash_sum(char* bp, int size)
{
    EVP_MD_CTX* context = EVP_MD_CTX_new();
    std::string hash_string;

    if (context != nullptr) {
        if (EVP_DigestInit_ex(context, EVP_sha256(), nullptr)) {
            if (EVP_DigestUpdate(context, bp, size)) {
                unsigned char hash[EVP_MAX_MD_SIZE];
                unsigned int length = 0;

                if (EVP_DigestFinal_ex(context, hash, &length)) {
                    std::stringstream ss;
                    ss << std::hex << std::setw(2) << std::setfill('0');
                    for (unsigned int i = 0; i < length; ++i) {
                        ss << (int) hash[i];
                    }
                    hash_string = ss.str();
                }
            }
        }

        EVP_MD_CTX_free(context);
    }

    return hash_string;
}

int readBytes(const std::string& path, UDA_PLUGIN_INTERFACE* plugin_interface)
{
    int err;

    char md5file[2 * MD5_SIZE + 1] = "";
    char md5check[2 * MD5_SIZE + 1] = "";

    //----------------------------------------------------------------------
    // Block Access to External Users

    if (udaPluginIsExternal(plugin_interface)) {
        err = 999;
        udaAddPluginError(plugin_interface, __func__, err, "This Service is Disabled");
        udaPluginLog(plugin_interface, "Disabled Service - Requested File: %s \n", path.c_str());
        return err;
    }

    //----------------------------------------------------------------------
    // Test the filepath

    if (!is_legal_file_path(path.c_str())) {
        err = 999;
        udaAddPluginError(plugin_interface, __func__, err, "The directory path has incorrect syntax");
        udaPluginLog(plugin_interface, "The directory path has incorrect syntax [%s] \n", path.c_str());
        return err;
    }

    //----------------------------------------------------------------------
    // Data Source Details

    err = 0;

    udaPluginLog(plugin_interface, "File Name  : %s \n", path.c_str());

    //----------------------------------------------------------------------
    // File Attributes

    errno = 0;

    //----------------------------------------------------------------------
    // Open the File as a Binary Stream

    errno = 0;
    FILE* fh = fopen(path.c_str(), "rb");

    int serrno = errno;

    if (fh == nullptr || ferror(fh) || serrno != 0) {
        err = BYTEFILEOPENERROR;
        if (serrno != 0) {
            udaAddPluginError(plugin_interface, __func__, serrno, "");
        }
        udaAddPluginError(plugin_interface, __func__, err, "Unable to Open the File for Read Access");
        if (fh != nullptr) {
            fclose(fh);
        }
        return err;
    }

    // Read File (Consider using memory mapped I/O & new type to avoid heap free at end if this is too slow!)

    size_t nchar = 0;
    int offset = 0;
    size_t bufsize = 100 * 1024;

    char* bp = nullptr;
    while (!feof(fh)) {
        if ((bp = (char*)realloc(bp, bufsize)) == nullptr) {
            err = BYTEFILEHEAPERROR;
            udaAddPluginError(plugin_interface, __func__, err, "Unable to Allocate Heap Memory for the File");
            break;
        }
        int nread = (int)fread(bp + offset, sizeof(char), bufsize, fh);
        nchar = nchar + nread;
        offset = nchar;
    }

    fclose(fh);

    if (err != 0) {
        return err;
    }

    auto sum = hash_sum(bp, nchar);

    int shape[] = { (int)nchar };
    setReturnData(plugin_interface, bp, nchar, UDA_TYPE_CHAR, 1, shape, sum.c_str());

    udaPluginLog(plugin_interface, "File Size          : %d \n", nchar);
    udaPluginLog(plugin_interface, "File Checksum      : %s \n", md5file);
    udaPluginLog(plugin_interface, "Read Checksum      : %s \n", md5check);

    return err;
}
