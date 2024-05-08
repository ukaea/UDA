//------------------------------------------------------------------------------------------------------------------
/*! Expand/Resolve the Path/File Name to its Full Form, including link resolution.

 Environment Variables:
                UDA_SCRATCHNAME
                UDA_NETWORKNAME
                HOSTNAME
 Macros
                SCRATCHDIR
                NETPREFIX
                HOSTNAME
                NOHOSTPREFIX
*/
//------------------------------------------------------------------------------------------------------------------

#include "expand_path.h"

#include <cerrno>

#ifndef _WIN32
#  include <unistd.h>
#else
#  include <Windows.h>
#endif

#include <cstdlib>
#include <vector>

#include "logging/logging.h"
#include "config/config.h"
#include "udaErrors.h"

#include "errorLog.h"
#include "stringUtils.h"
#ifdef SERVERBUILD
#  include "server/serverStartup.h"
#endif
// Identify the current working Host
#ifdef NOEXPANDPATH
//! Dummy functions used when path expansion is disabled using the NOEXPANDPATH compiler option

char* hostid(char* host)
{
    return host;
}
char* pathid(char* path)
{
    return path;
}
void expandFilePath(char* path)
{
    return;
}
#else

#  include <boost/algorithm/string.hpp>
#  include <fmt/format.h>

#  define MAXPATHSUBS 10
#  define MAXPATHSUBSLENGTH 256

using namespace uda::client_server;
using namespace uda::logging;

/**
 * The workstation (client host) name is obtained using the operating system command 'hostname'.
 *
 * @param host The name of the client host workstation. The string is pre-allocated with length STRING_LENGTH
 * @return A pointer to the host string (Identical to the argument).
 */
char* uda::client_server::host_id(char* host)
{

#  ifdef _WIN32
    DWORD size = STRING_LENGTH - 1;
    GetComputerName(host, &size);
    return host;
#  else

    host[0] = '\0';

#    ifndef USEHOSTDOMAINNAME
    if ((gethostname(host, STRING_LENGTH - 1)) != 0) {
        char* env = getenv("HOSTNAME");
        if (env != nullptr) {
            copy_string(env, host, STRING_LENGTH);
        }
    }
#    else
    if ((gethostname(host, STRING_LENGTH - 1)) == 0) {
        char domain[STRING_LENGTH];
        if ((getdomainname(domain, STRING_LENGTH - 1)) == 0) {
            int l1 = (int)strlen(host);
            int l2 = (int)strlen(domain);
            if (l1 + l2 + 1 < STRING_LENGTH - 1) {
                strcat(host, ".");
                strcat(host, domain);
            }
        }
    } else {
        char* env = getenv("HOSTNAME");
        if (env != nullptr) {
            copy_string(env, host, STRING_LENGTH);
        }
    }
#    endif

    if (host[0] == '\0') {
        add_error(UDA_CODE_ERROR_TYPE, "host_id", 999, "Unable to Identify the Host Name");
    }
    return host;

#  endif // _WIN32
}

//----------------------------------------------------------------------------------------------
/*! Substitute/Replace file path name elements for server side name resolution.

Client and server may see different network file paths. A path seen by the client is different to the path seen by the
server. Generally, the path might contain additional hierarchical components that need removing. This is
easily done by substitution.

Examples:
    /.automount/funsrv1/root/home/xyz -> /net/funsrv1/home/xyz
    /.automount/funsrv1/root/home/xyz -> /home/xyz
    /.automount/fuslsd/root/data/MAST_Data/013/13500/Pass0/amc0135.00 ->
/net/fuslsd/data/MAST_Data/013/13500/Pass0/amc0135.00 /scratch/mydata -> /net/hostname/scratch/mydata

A list of Target path components is read from the environment variable UDA_PRIVATE_PATH_TARGET. The delimiter between
components is , or : or ;.
A matching list of substitutes path components is read from the environment variable UDA_PRIVATE_PATH_SUBSTITUTE.

A maximum number of 10 substitutes is allowed.

Use multiple wild card * for any target path element name
Target and replacement must begin /

If a * appears in both the target and substitute paths, the path element is retained in the replacement.
The number of wildcards must match

If there are more wildcards in the substitute string than in the target string, an error will occur.

@param path The path to be tested for targeted name element replacement.
@returns An integer Error Code: If non zero, a problem occurred.
*/
int uda::client_server::path_replacement(const uda::config::Config& config, char* path)
{
    //----------------------------------------------------------------------------------------------
    // Does the Path contain hierarchical components not seen by the server? If so make a substitution.
    //
    // This replacement also occurs on the UDA server
    //
    // pattern:    /A/B/C;/D/E/F -> /A/C;/E
    // use multiple wild card characters '*' for 'any' target path element name
    // target and replacement strings must begin with '/'
    //
    // If a '*' appears in both the target and substitute paths, the path element is retained in the replacement.
    // The number of wildcards must match
    //
    // If a wildcard appears in the substitute string and none in the target string, an error is reported.
    //
    //----------------------------------------------------------------------------------------------

    const char* delimiters = ",:;";
    std::vector<std::string> targets;
    std::vector<std::string> substitutes;

    if (path[0] == '\0') {
        return 0; // No replacement
    }

    auto private_path_target = config.get("server.private_path_target").as_or_default<std::string>({});
    auto private_path_substitute = config.get("server.private_path_substitute").as_or_default<std::string>({});

    if (private_path_target.empty()) {
        return 0; // No replacement
    }

    std::string work;
    work.resize(strlen(path));

    UDA_LOG(UDA_LOG_DEBUG, "path_replacement: Testing for File Path Replacement");
    UDA_LOG(UDA_LOG_DEBUG, "{}", path);

    // Parse targets
    boost::split(targets, private_path_target, boost::is_any_of(delimiters), boost::token_compress_on);

    // Parse substitutes
    boost::split(substitutes, private_path_substitute, boost::is_any_of(delimiters), boost::token_compress_on);

    if (targets.size() == substitutes.size()) {
        for (size_t i = 0; i < targets.size(); i++) {
            if (targets[i] == "*") {
                // Wildcard found

                // list of target tokens

                std::vector<std::string> target_tokens;
                std::vector<std::string> path_tokens;
                std::vector<std::string> sub_tokens;

                boost::split(target_tokens, targets[i], boost::is_any_of(PATH_SEPARATOR), boost::token_compress_on);
                boost::split(path_tokens, path, boost::is_any_of(PATH_SEPARATOR), boost::token_compress_on);

                if (path_tokens.size() < target_tokens.size()) {
                    // Impossible substitution, so ignore this target
                    continue;
                }

                if (substitutes[i] == "*") {
                    // Wildcard found
                    boost::split(sub_tokens, substitutes[i], boost::is_any_of(PATH_SEPARATOR),
                                 boost::token_compress_on);

                    auto is_wild = [](const std::string& token) { return token[0] == '0'; };
                    size_t sub_wild_count = std::count_if(sub_tokens.begin(), sub_tokens.end(), is_wild);
                    size_t target_wild_count = std::count_if(target_tokens.begin(), target_tokens.end(), is_wild);

                    if (sub_wild_count != target_wild_count) {
                        // Un-matched Wildcards found
                        UDA_THROW_ERROR(999, "Un-matched wildcards found in the target and substitute paths");
                    }
                }

                size_t lpath = 0;
                bool match = true; // Test path tokens against target tokens
                for (size_t j = 0; j < target_tokens.size(); j++) {
                    match = match && (target_tokens[j] == path_tokens[j] || target_tokens[j][0] == '*');
                    lpath = lpath + path_tokens[j].size() + 1; // Find the split point
                    if (!match) {
                        break;
                    }
                }

                if (match) {
                    // Make the substitution, ignoring wildcards
                    strcpy(work.data(), &path[lpath]);
                    if (sub_tokens.empty()) {
                        strcpy(path, substitutes[i].c_str());
                    } else {
                        size_t kstart = 0;
                        path[0] = '\0';
                        for (size_t j = 0; j < substitutes.size(); j++) {
                            strcat(path, PATH_SEPARATOR);
                            if (substitutes[j][0] == '*') {
                                // Substitute actual path element
                                for (size_t k = kstart; k < target_tokens.size(); k++) {
                                    if (target_tokens[k][0] == '*') {
                                        kstart = k + 1;
                                        strcat(path, path_tokens[k].c_str());
                                        break;
                                    }
                                }
                            } else {
                                strcat(path, sub_tokens[j].c_str());
                            }
                        }
                    }
                    strcat(path, work.c_str());
                    break;
                }

            } else {
                if (substitutes[i] == "*") { // Wildcard found in substitute string!
                    UDA_THROW_ERROR(
                        999,
                        "No wildcards are permitted in the substitute path unless matched by one in the target path.");
                }

                size_t lpath = targets[i].size();
                if (targets[i] == path) {
                    // Test for straight replacement
                    strcpy(work.data(), &path[lpath]);
                    strcpy(path, substitutes[i].c_str());
                    strcat(path, work.c_str());
                    break;
                }
            }
        }

    } else {
        UDA_THROW_ERROR(999,
                        "Number of Path Targets and Substitutes is inconsistent. Correct the Environment Variables.");
    }

    UDA_LOG(UDA_LOG_DEBUG, "{}", path);
    UDA_LOG(UDA_LOG_DEBUG, "path_replacement: End");

    return 0;
}

// Client side only

#  ifndef SERVERBUILD

int uda::client_server::link_replacement(char* path)
{

    //----------------------------------------------------------------------------------------------
    // Is the path a symbolic link not seen by the server? If so make a substitution.
    //----------------------------------------------------------------------------------------------

#    ifdef _WIN32
    return path != nullptr; // No check for windows
#    else

    int err;
    FILE* ph = nullptr;
    char* p;

    //------------------------------------------------------------------------------------
    //! Dereference path links using a command pipe: Ignore any errors
    //
    // If the user has embedded linux commands within the source string, they will be exposed here
    // within the client's environment - not the server's.

    std::string cmd = fmt::format("ls -l {} 2>&1;", path);

    errno = 0;
    if ((ph = popen(cmd.c_str(), "r")) == nullptr) {
        if (errno != 0) {
            add_error(UDA_SYSTEM_ERROR_TYPE, "link_replacement", errno, "");
        }
        err = 1;
        add_error(UDA_CODE_ERROR_TYPE, "link_replacement", err, "Unable to Dereference Symbolic links");
        path[0] = '\0';
        return err;
    }

    char buffer[STRING_LENGTH];
    if (!feof(ph)) {
        if (fgets(buffer, STRING_LENGTH - 1, ph) == nullptr) {
            UDA_THROW_ERROR(999, "failed to read line from command");
        }
    }
    pclose(ph);

    //------------------------------------------------------------------------------------
    //! Extract the Dereferenced path. Accept only if it is Not a Relative path

    if ((p = strstr(buffer, " -> ")) != nullptr) {
        if (p[4] == '/') {
            strcpy(path, p + 4);
            convert_non_printable2(path);
            trim_string(path);
            // expandFilePath(path);
        }
    }

    return 0;

#    endif // _WIN32
}

#  else
int uda::client_server::link_replacement(char* path)
{
    // Links are resolved client side only
    return 0;
}
#  endif

//------------------------------------------------------------------------------------------------------------------
//------------------------------------------------------------------------------------------------------------------
/*! Fully expand file directory paths to remove relative path or environment variable components.

 Examples:    filename                    use getpwd
        ./filename                        use cd; $PWD
        ../../filename                    use cd; $PWD
        /abc/filename                    do nothing - fully resolved
        ~user/abc/filename                use cd; $PWD
        /scratch/abc/filename            use hostname
        /tmp/abc/def/filename            use hostname
        /fuslwx/scratch/abc/filename    do nothing - fully resolved
        /fuslwx/tmp/abc/filename        do nothing - fully resolved
        /fuslwx/abc/filename            do nothing - fully resolved
        /99999                          do nothing - Resolved by the Server Data Plugin
        /99999/999                         do nothing - Resolved by the Server Data Plugin
        $ENVAR/abc/filename                expand with the specified environment variable
        $ENVAR/abc/$ENVAR/filename        expand with the specified environment variable

@param path The file path to be resolved and expanded.
@returns An integer Error Code: If non zero, a problem occured.
*/

int uda::client_server::expand_file_path(const uda::config::Config& config, char* path)
{

#  ifdef _WIN32
    return 0; // No expansion for windows
#  else

    //
    //----------------------------------------------------------------------------------------------

    char *fp = nullptr, *fp1 = nullptr, *env = nullptr;
    char file[STRING_LENGTH];
#    ifndef NOHOSTPREFIX
    char host[STRING_LENGTH];
#    endif
    char cwd[STRING_LENGTH];
    char ocwd[STRING_LENGTH];  // Current Working Directory
    char opath[STRING_LENGTH]; // Original Path string
    char work[STRING_LENGTH];
    char work1[STRING_LENGTH];
    char scratch[STRING_LENGTH];
    char netname[STRING_LENGTH];
    char* pcwd = cwd;
    char* token = nullptr;

    int lcwd = STRING_LENGTH - 1;
    int lpath, err = 0;
    size_t lscratch;
    int t1, t2, t3, t4, t5, t6;

    //------------------------------------------------------------------------------------------------------------------
    // Test for possible imbedded linux command

    if (!is_legal_file_path(path)) {
        err = 999;
        add_error(UDA_CODE_ERROR_TYPE, "expand_file_path", err, "The Source contains a Syntax Error!");
        return err;
    }

    //------------------------------------------------------------------------------------------------------------------
    /*! Workstations have local hard drives that may also be mounted on the user's network. If this local disk is
    accessible by the UDA server it's name needs to be expanded using additional information. As all similar
    workstations on the network might use the same name for the local disk area, the host name is used to uniquely
    identify where the disk is on the network.

    The name model adopted to expand directory paths to local disk drives is: /local -> /NETNAME/HOSTNAME/local

    NETNAME is defined by the environment variable UDA_NETWORKNAME

    The hostname of the local workstation is obtained directly from the operating system. If this fails, it can be
    included directly in the UDA_NETWORKNAME environment variable.

    The local disk directory, e.g. /scratch or /tmp, is targeted using the name defined by the environment variable
    UDA_SCRATCHNAME

    Only ONE local drive can be targeted using the environment variable (current set up could be expanded to use a
    list). The path replacement function could be used to target other local directories.
    */

#    ifdef SCRATCHDIR
    sprintf(scratch, "/%s/", SCRATCHDIR);
    lscratch = (int)strlen(scratch);
#    else
    strcpy(scratch, "/scratch/");
    lscratch = 9;
#    endif

#    ifdef NETPREFIX
    strcpy(netname, NETPREFIX);
#    else
    netname[0] = '\0';
#    endif

    // Override compiler options

    if ((env = getenv("UDA_SCRATCHNAME")) != nullptr) { // Check for Environment Variable
        snprintf(scratch, STRING_LENGTH, "/%s/", env);
        lscratch = (int)strlen(scratch);
    }

    if ((env = getenv("UDA_NETWORKNAME")) != nullptr) {
        strcpy(netname, env);
    }

    //------------------------------------------------------------------------------------------------------------------
    /*! File Path names are only resolved if they contain relative path elements, environment variables, or
     references to a local scratch disk, e.g., ./ ../ ~ $ /scratch
    */

    if ((lpath = (int)strlen(path)) == 0) {
        // Nothing to resolve
        return 0;
    }

    // Test for necessary expansion

    t1 = strstr(path, "./") == nullptr;         // relative path?
    t2 = strstr(path, "../") == nullptr;        // relative path?
    t3 = strchr(path, '~') == nullptr;          // home path?
    t4 = strchr(path, '$') == nullptr;          // No imbedded Environment variable
    t5 = path[0] == '/';                        // No Relative Directory path
    t6 = strncmp(path, scratch, lscratch) != 0; // Not the Scratch directory

    if (t1 && t2 && t3 && t4 && t5 && t6) {
        // No Relative path name elements found
        return 0;
    }

    //------------------------------------------------------------------------------------------------------------------
    /*! The path argument has been stripped of any format or protocol prefix. If another prefix is within the path, this
    indicates the source is server to server. Therefore, the main source element is a URL and is not expanded.

    If the ':' character is detected in the source prefix, it is assumed the request is is via a server (the : indicates
    a port number) and the main source component is a URL. URLs are not path expanded.

    Server side functions are identified via the pair of parnethesis enclosing arguments. These are not expanded.
    */

#    ifdef SERVERELEMENTCHECK
    int t7, t8, t9;
    t7 = strstr(path, environment->api_delim) != nullptr;              // Pass request forward to another server
    t8 = strchr(path, ':') != nullptr;                                 // Port number => Server
    t9 = strchr(path, '(') != nullptr && strchr(path, ')') != nullptr; // Server Side Function

    if (t7 || t8 || t9) {
        return 0; // Server host, protocol, and server side functions
    }
#    endif

    //------------------------------------------------------------------------------------------------------------------
    // Test if the Path begins with an Integer or /Integer => Resolved by the Server Data Plugin

    if (path[0] == '/') {
        strcpy(work, path + 1); // the leading character is a / so ignore
    } else {
        strcpy(work, path);
    }

    token = strtok(work, "/");

    if (token != nullptr) {
        if (is_number(token)) {
            return 0; // Is the First token an integer number?
        }
    }

    //------------------------------------------------------------------------------------------------------------------
    //! Identify the Current Working Directory

    if (!t1 || !t2 || !t3 || !t4 || !t5) {

        work[0] = '\0';
        errno = 0;
        strcpy(opath, path);

        pcwd = getcwd(cwd, lcwd);

        if (errno != 0) {
            err = 999;
            add_error(UDA_SYSTEM_ERROR_TYPE, "expand_path", errno, "Cannot resolve the Current Working Directory!");
            add_error(UDA_CODE_ERROR_TYPE, "expand_path", err, "Unable to resolve full file names.");
            return err;
        }

        if (pcwd == nullptr) {
            UDA_THROW_ERROR(999, "Cannot resolve the Current Working Directory! Unable to resolve full file names.");
        }

        strcpy(ocwd, cwd);

        //! Does the path NOT contain a path directory separator character => filename only so prepend the CWD and
        //! return

        if ((fp = strrchr(path, '/')) == nullptr) { // Search backwards - extract filename
            strcpy(work1, path);
            snprintf(path, STRING_LENGTH, "%s/%s", cwd, work1); // prepend the CWD and return
            if ((err = link_replacement(path)) != 0) {
                return err;
            }
            if ((err = path_replacement(config, path)) != 0) {
                return err;
            }

            if (strncmp(path, scratch, lscratch) != 0) {
                // Not the Scratch directory ?
                return 0;
            }
            fp = strrchr(path, '/'); // extract filename
        }

        strcpy(file, &fp[1]); // Filename
        fp[1] = '\0';         // Split the path string: path now contains directory only

        //! Does the Path contain with an Environment variable (Not resolved by the function chdir!)

        fp = nullptr;
        lpath = (int)strlen(path);
        if (lpath > 0) {
            fp = strchr(path + 1, '$');
        }

        if (path[0] == '$' || fp != nullptr) { // Search for a $ character

            if (fp != nullptr) {
                strlcpy(work, path, fp - path);

                if ((fp1 = strchr(fp, '/')) != nullptr) {
                    strlcpy(work1, fp + 1, fp1 - fp - 1);
                } else {
                    strcpy(work1, fp + 1);
                }

                if ((env = getenv(work1)) !=
                    nullptr) { // Check for Environment Variable: If not found then assume it's server side
                    if (env[0] == '/') {
                        strcpy(work1, env + 1);
                    } else {
                        strcat(work1, env);
                    }
                    strcat(work, work1);
                    strcat(work, fp1);
                    strcpy(path, work);
                }

            } else {
                work1[0] = '\0';
                if ((fp = strchr(path, '/')) != nullptr) {
                    strlcpy(work, path + 1, fp - path - 1);
                    strcpy(work1, fp);
                } else {
                    strcpy(work, path + 1);
                }

                if ((env = getenv(work)) != nullptr) {
                    // Check for Environment Variable: If not found then assume it's server side
                    if (env[0] == '/') {
                        strcpy(work, env);
                    } else {
                        strcpy(work, "/");
                        strcat(work, env);
                    }
                    strcat(work, work1);
                    strcpy(path, work);
                }
            }
        }

        /* Change to the File's Directory (will resolve relative path elements ~/./../$envVariable/ as well as links)
         *
         * If an error occurs, either the directory doesn't exist or the path is not a file path!
         * If the path contains an environment variable, then resolved server side
         */

        if (chdir(path) != 0) {
            // Ensure the Original WD
            if (chdir(ocwd) != 0) {
                UDA_THROW_ERROR(999, "Failed to chdir back to original working directory");
            };
            strcpy(path, opath); // Return to the Original path name
            UDA_LOG(UDA_LOG_DEBUG,
                    "Unable to identify the Directory of the file: {}\n"
                    "The server will know if a true error exists: Plugin & Environment dependent",
                    path);
            return 0;
        }

        //! The Current Working Directory is the resolved directory name

        errno = 0;

        pcwd = getcwd(cwd, lcwd);

        if (errno != 0) {
            err = 998;
            add_error(UDA_SYSTEM_ERROR_TYPE, "expand_path", errno, "Cannot resolve the Current Working Directory!");
            add_error(UDA_CODE_ERROR_TYPE, "expand_path", err, "Unable to resolve full file names.");
            return err;
        }

        if (pcwd == nullptr) {
            UDA_THROW_ERROR(998, "Cannot resolve the Current Working Directory! Unable to resolve full file names.");
        }

        strcpy(work1, cwd);

        //! Return to the Original Working Directory

        if (chdir(ocwd) != 0) {
            err = 999;
            add_error(UDA_SYSTEM_ERROR_TYPE, "expand_path", errno, "Unable to Return to the Working Directory!");
            add_error(UDA_CODE_ERROR_TYPE, "expand_path", err, "Unable to resolve full file names.");
            return err;
        }

        //! Prepend the expanded/resolved directory name to the File Name

        snprintf(path, STRING_LENGTH, "%s/%s", work1, file); // Prepend the path to the filename

    } // End of t1 - t5 tests

    //----------------------------------------------------------------------------------------------
    /*! Symbolic Links might not be visible by the server: Pass the true location
     */

    if ((err = link_replacement(path)) != 0) {
        return err;
    }

    //----------------------------------------------------------------------------------------------
    /*! Does the path contain the client workstation's local Scratch directory (Must be visible by the server)

     Model:    /scratch/a/b/c -> /hostname/scratch/a/b/c            (default)
        /scratch/a/b/c -> /netname/hostname/scratch/a/b/c

    */

#    ifndef NOHOSTPREFIX

    t6 = strncmp(path, scratch, lscratch) != 0; // Retest for the Scratch directory

    if (!t6) { // Scratch directory used without hostname prefix
        strcpy(work, path);
        if (!strncmp(work, scratch, lscratch)) { // case sensistive

            if ((env = getenv("HOSTNAME")) != nullptr) { // Check for a system Environment Variable
                strcpy(host, env);
            } else {
                host_id(host); // Identify the Name of the Current Workstation or Host
            }

            // TODO: refactor this function so that we do not have to guess the path size
            if (strlen(netname) > 0 && strlen(host) > 0) {
                snprintf(path, STRING_LENGTH, "/%s/%s%s", netname, host,
                         work); // prepend /netname/hostname to /scratch/...
            } else {
                if (strlen(netname) > 0) {
                    snprintf(path, STRING_LENGTH, "/%s%s", netname, work);
                } else {
                    if (strlen(host) > 0) {
                        snprintf(path, STRING_LENGTH, "/%s%s", host, work);
                    }
                }
            }
        }
    }

#    endif

    //----------------------------------------------------------------------------------------------
    /*! Does the Path to a user's Private Files contain network components not seen by the server?
    If so, target these and make a suitable substitution to resolve path problems.
    */
    err = path_replacement(config, path);

    return err;

#  endif // _WIN32
}

//----------------------------------------------------------------------------------------------
/*! Test a path is legitimate

@param path The file path to be tested.
@returns A pointer to the path argument. If a problem occurs, the path string is empty.
*/

char* uda::client_server::path_id(char* path)
{

#  ifdef _WIN32
    return path; // No check for windows
#  else

    char* p;
    char work[STRING_LENGTH]; // Are these consistent with the system MAX_PATH?
    char pwd[STRING_LENGTH];
    strcpy(work, path);

    // the path string may contain malign embedded linux commands: is chdir secure?
    // basic check

    if (!is_legal_file_path(path)) {
        add_error(UDA_CODE_ERROR_TYPE, "path_id", 999, "The directory path has incorrect syntax");
        path[0] = '\0';
        return path;
    }

    if (getcwd(pwd, STRING_LENGTH - 1) != nullptr) {
        errno = 0;
        if (chdir(path) == 0) {
            if ((p = getcwd(pwd, STRING_LENGTH - 1)) != nullptr) {
                strcpy(path, p);
                if (chdir(pwd) != 0) {
                    add_error(UDA_SYSTEM_ERROR_TYPE, __func__, errno, "");
                    add_error(UDA_CODE_ERROR_TYPE, __func__, 999, "The directory path is not available");
                }
                trim_string(path);
                left_trim_string(path);
                return path;
            }
        } else {
            if (errno == EACCES) {
                add_error(UDA_SYSTEM_ERROR_TYPE, "path_id", errno, "");
                add_error(UDA_CODE_ERROR_TYPE, "path_id", 999, "The directory path is not available");
            } else if (errno == ENOENT || errno == ENOTDIR) {
                add_error(UDA_SYSTEM_ERROR_TYPE, "path_id", errno, "");
                add_error(UDA_CODE_ERROR_TYPE, "path_id", 999, "The directory path does not exist");
            }
        }
    }
    path[0] = '\0';
    return path;
#  endif // _WIN32
}

#endif
