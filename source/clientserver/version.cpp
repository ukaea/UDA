#include <version.h>
#include <regex>
#include <string>

/**
 * @brief Encode the semantic version string MAJOR.MINOR.PATCH[.DELTA] in an integer value
 * using 2-byte chunks for each component value.
 * ie value = (MAJOR << 24) | (MINOR << 16) | (PATCH << 8) | (DELTA)
 * if the input string is malformed this function will return 0
 *
 * @param version_string string containing the semantic version number in the format MAJOR.MINOR.PATCH[.DELTA]
 * @return The encoded integer value
 */
unsigned int bitEncodeSemanticVersionString(const char* version)
{
    unsigned int encoded_version = 0;

    std::regex r("([0-9]+)\\.([0-9]+)\\.([0-9]+)(\\.([0-9]+))?");

    std::smatch r_matches;
    std::string version_string(version);
    std::regex_match(version_string, r_matches, r);

    int bitshift = 24;
    for (std::size_t i = 1; i < r_matches.size(); ++i)
    {
        std::ssub_match sub_match = r_matches[i];
        std::string token = sub_match.str();
        if (i !=4 and !token.empty() and bitshift >=0)
        {
            encoded_version |= static_cast<unsigned int>(std::stoi(token)) << bitshift;   
            bitshift -= 8;
        }
    }
    return encoded_version;
}

/**
 * @brief Encode semantic verison informetion into a single integer variable using 2-byte 
 * chunks for each component value 
 * ie value = (MAJOR << 24) | (MINOR << 16) | (PATCH << 8) | (DELTA)
 *
 * @param[in] major Major version number
 * @param[in] minor Minor version number
 * @param[in] patch Patch version number
 * @param[in] delta delta version number
 * @return The encoded integer value
 */
unsigned int bitEncodeVersionNumber(unsigned int major, unsigned int minor, 
                                    unsigned int patch, unsigned int delta)
{
    return (major << 24) | (minor << 16) | (patch << 8) | (delta);
}

/**
 * @brief Decode semantic version number information from a bit-encoded integer variable
 *
 * @param[in] encoded_version The integer value containing bit-encoded version information
 * @return VersionData struct containing version data for each of the major, minor, patch, and delta components. 
 */
VERSION_DATA decodeVersionNumber(unsigned int encoded_version)
{
    VERSION_DATA result;
    result.major = (encoded_version >> 24) & 0x000F;
    result.minor = (encoded_version >> 16) & 0x000F;
    result.patch = (encoded_version >> 8) & 0x000F;
    result.delta = encoded_version & 0x000F;
    
    snprintf(result.version_string, 256, "%d.%d.%d.%d", result.major, result.minor, result.patch, result.delta);

    return result;
}


