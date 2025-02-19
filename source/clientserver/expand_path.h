#pragma once

#include "uda_structs.h"

namespace uda::config
{
class Config;
}

namespace uda::client_server
{

/*! The workstation (client host) name is obtained using the operating system command 'hostname'.

@param host The name of the client host workstation. The string is pre-allocated with length StringLength
@returns A pointer to the host string (Identical to the argument).
*/
char* host_id(char* host);

/*! Test a path is legitimate

@param path The file path to be tested.
@returns A pointer to the path argument. If a problem occurs, the path string is empty.
*/
char* path_id(char* path);

/*! Fully expand file directory paths to remove relative path or environment variable components.

 Examples:    filename            use getpwd
        ./filename            use cd; $PWD
        ../../filename            use cd; $PWD
        /abc/filename            do nothing - fully resolved
        ~user/abc/filename        use cd; $PWD
        /scratch/abc/filename        use hostname
        /tmp/abc/def/filename        use hostname
        /fuslwx/scratch/abc/filename    do nothing - fully resolved
        /fuslwx/tmp/abc/filename    do nothing - fully resolved
        /fuslwx/abc/filename        do nothing - fully resolved
        /99999              do nothing - Resolved by the Server Data Plugin
        /99999/999             do nothing - Resolved by the Server Data Plugin
        $ENVAR/abc/filename        expand with the specified environment variable
        $ENVAR/abc/$ENVAR/filename    expand with the specified environment variable

@param path The file path to be resolved and expanded.
@returns An integer Error Code: If non zero, a problem occured.
*/
int expand_file_path(const uda::config::Config& config, char* path);

#ifndef NO_EXPAND_PATH

#  define MAXPATHSUBS 10
#  define MAXPATHSUBSLENGTH 256

/*! Free heap memory allocated to generate lists of target path elements and substitute path elements.

@param tokenList A Pointer to an array of token strings
@param tokenCount The number of tokens in the list.
*/
void free_token_list(char*** tokenListArray, int* tokenCount);

/*! Generate a lists of path elements tokens.

@param delims An array of character delimiters used to separate path elements
@param input The list of path elements to be parsed into individual token strings.
@param tokenList A pointer to an array of token strings. This must be freed using free_token_list when no longer needed.
returns A count of the tokens parsed from input.
*/
int token_list(const char* delims, char* input, char*** tokenListArray);

/*! Substitute/Replace file path name elements for server side name resolution.

Client and server may see different network file paths. A path seen by the client is different to the path seen by the
server. Generally, the path might contain additional hierarchical components that need removing. This is
easily done by substitution.

Examples:    /.automount/funsrv1/root/home/xyz -> /net/funsrv1/home/xyz
        /.automount/funsrv1/root/home/xyz -> /home/xyz
        /.automount/fuslsd/root/data/MAST_Data/013/13500/Pass0/amc0135.00 ->
/net/fuslsd/data/MAST_Data/013/13500/Pass0/amc0135.00 /scratch/mydata -> /net/hostname/scratch/mydata

A list of Target path components is read from the environment variable IDAM_PRIVATE_PATH_TARGET. The delimiter between
components is , or : or ;.
A matching list of substitutes path components is read from the environment variable IDAM_PRIVATE_PATH_SUBSTITUTE.

A maximum number of 10 substitutes is allowed.

Use multiple wild card * for any target path element name
Target and replacement must begin /

If a * appears in both the target and substitute paths, the path element is retained in the replacement.
The number of wildcards must match

If there are more wildcards in the substitute string than in the target string, an error will occur.

@param path The path to be tested for targeted name element replacement.
@returns An integer Error Code: If non zero, a problem occured.
*/
int path_replacement(const config::Config& config, char* path);

int link_replacement(char* path);

#endif // NO_EXPAND_PATH

} // namespace uda::client_server