#include "readBytesNonOptimally.h"

#include <array>
#include <cerrno>
#include <cstdlib>
#include <cstring>
#include <iomanip>
#include <openssl/evp.h>
#include <sstream>

#define BYTEFILEDOESNOTEXIST 100001
#define BYTEFILEATTRIBUTEERROR 100002
#define BYTEFILEISNOTREGULAR 100003
#define BYTEFILEOPENERROR 100004
#define BYTEFILEHEAPERROR 100005
#define BYTEFILEMD5ERROR 100006
#define BYTEFILEMD5DIFF 100007

namespace
{
int is_legal_file_path(const char* str)
{
    // Basic check that the filename complies with good naming practice - some protection against malign embedded code!
    // Test against the Portable Filename Character Set A-Z, a-z, 0-9, <period>, <underscore> and <hyphen> and <plus>
    // Include <space> and back-slash for windows filenames only, forward-slash for the path seperator and $ for
    // environment variables

    // The API source argument can also be a server based source containing a ':' character
    // The delimiter characters separating the device or format name from the source should have been split off of the
    // path
    //

    const char* tst = str;
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
} // namespace

EVP_MD_CTX* new_hash_context()
{
    EVP_MD_CTX* context = EVP_MD_CTX_new();
    std::string hash_string;

    if (context != nullptr && EVP_DigestInit_ex(context, EVP_sha256(), nullptr)) {
        return context;
    }

    return nullptr;
}

void update_hash(EVP_MD_CTX* context, const char* data, size_t size)
{
    EVP_DigestUpdate(context, data, size);
}

std::string get_hash_sum(EVP_MD_CTX* context)
{
    std::string hash_string;

    unsigned char hash[EVP_MAX_MD_SIZE];
    unsigned int length = 0;

    if (EVP_DigestFinal_ex(context, hash, &length)) {
        std::stringstream ss;
        ss << std::hex << std::setw(2) << std::setfill('0');
        for (unsigned int i = 0; i < length; ++i) {
            ss << (int)hash[i];
        }
        hash_string = ss.str();
    }

    EVP_MD_CTX_free(context);
    return hash_string;
}

std::string exec(const char* cmd)
{
    std::array<char, 128> buffer;
    std::string result;
    std::unique_ptr<FILE, decltype(&pclose)> pipe(popen(cmd, "r"), pclose);
    if (!pipe) {
        throw std::runtime_error("popen() failed!");
    }
    while (fgets(buffer.data(), buffer.size(), pipe.get()) != nullptr) {
        result.append(buffer.data());
    }
    return result;
}

int readBytes(const std::string& path, UDA_PLUGIN_INTERFACE* plugin_interface)
{
    int err;

    //----------------------------------------------------------------------
    // Block Access to External Users

    if (udaPluginIsExternal(plugin_interface)) {
        err = 999;
        udaAddPluginError(plugin_interface, __func__, err, "This Service is Disabled");
        UDA_PLUGIN_LOG_S(plugin_interface, "Disabled Service - Requested File: %s \n", path.c_str());
        return err;
    }

    //----------------------------------------------------------------------
    // Test the filepath

    if (!is_legal_file_path(path.c_str())) {
        err = 999;
        udaAddPluginError(plugin_interface, __func__, err, "The directory path has incorrect syntax");
        UDA_PLUGIN_LOG_S(plugin_interface, "The directory path has incorrect syntax [%s] \n", path.c_str());
        return err;
    }

    //----------------------------------------------------------------------
    // Data Source Details

    err = 0;

    UDA_PLUGIN_LOG_S(plugin_interface, "File Name  : %s \n", path.c_str());

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

    auto ctx = new_hash_context();
    update_hash(ctx, bp, nchar);
    auto read_hash = get_hash_sum(ctx);

    int shape[] = {(int)nchar};
    udaPluginReturnData(plugin_interface, bp, nchar, UDA_TYPE_CHAR, 1, shape, read_hash.c_str());

    // TODO: read sha256 sum command from config and try and run
    //    std::string cmd = "sha3sum -a 256 -b " + path;
    //    auto file_hash = exec(cmd.c_str());

    UDA_PLUGIN_LOG_I(plugin_interface, "File Size          : {} \n", (int)nchar);
    UDA_PLUGIN_LOG_S(plugin_interface, "Read Checksum      : {} \n", read_hash.c_str());

    return err;
}
