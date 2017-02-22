//------------------------------------------------------------------------------------------------------------------
/*! Expand/Resolve the Path/File Name to its Full Form, including link resolution.

 Environment Variables:
				IDAM_SCRATCHNAME
				IDAM_NETWORKNAME
				HOSTNAME
 Macros
				SCRATCHDIR
				NETPREFIX
				HOSTNAME
				NOHOSTPREFIX
*/
//------------------------------------------------------------------------------------------------------------------

#include "expand_path.h"

#include <errno.h>
#include <unistd.h>
#include <stdlib.h>

#include <logging/logging.h>
#include <clientserver/udaErrors.h>

#include "stringUtils.h"
#include "errorLog.h"

#ifdef SERVERBUILD
#  include <server/serverStartup.h>
#endif

// Identify the current working Host

#ifdef NOEXPANDPATH

//! Dummy functions used when path expansion is disabled using the NOEXPANDPATH compiler option

char *hostid(char *host) {
    return host;
}
char *pathid(char *path) {
    return path;
}
void expandFilePath(char *path) {
    return;
}

#else

//------------------------------------------------------------------------------------------------------------------
/*! The workstation (client host) name is obtained using the operating system command 'hostname'.

@param host The name of the client host workstation. The string is pre-allocated with length STRING_LENGTH
@returns A pointer to the host string (Identical to the argument).
*/

char* hostid(char* host)
{

#ifdef _WIN32
    int l = STRING_LENGTH-1;
    GetComputerName(host,&l);
    return host;
#endif

    int rc;
    host[0] = '\0';

#ifndef USEHOSTDOMAINNAME
    if ((rc = gethostname(host, STRING_LENGTH - 1)) != 0) {
        char* env = getenv("HOSTNAME");
        if (env != NULL) copyString(env, host, STRING_LENGTH);
    }
#else
    if((rc = gethostname(host, STRING_LENGTH-1)) == 0) {
        char domain[STRING_LENGTH];
        if((rc = getdomainname(domain, STRING_LENGTH-1)) == 0) {
            int l1 = (int)strlen(host);
            int l2 = (int)strlen(domain);
            if(l1+l2+1 < STRING_LENGTH-1) {
                strcat(host, ".");
                strcat(host, domain);
            }
        }
    } else {
        char *env = getenv("HOSTNAME");
        if(env != NULL) copyString(env, host, STRING_LENGTH);
    }
#endif

    if (host[0] == '\0')
        addIdamError(&idamerrorstack, CODEERRORTYPE, "hostid", 999, "Unable to Identify the Host Name");
    return host;

}

//------------------------------------------------------------------------------------------------------------------
/*! Free heap memory allocated to generate lists of target path elements and substitute path elements.

@param tokenList A Pointer to an array of token strings
@param tokenCount The number of tokens in the list.
*/

void freeTokenList(char*** tokenListArray, int* tokenCount)
{
    int i;
    char** list = *tokenListArray;
    if (*tokenCount == 0 || *tokenListArray == NULL) return;
    for (i = 0; i < *tokenCount; i++) free((void*) list[i]);
    free((void*) list);
    *tokenListArray = NULL;        // Reset to avoid double free.
    *tokenCount = 0;
    return;
}

/*! Generate a lists of path elements tokens.

@param delims An array of character delimiters used to separate path elements
@param input The list of path elements to be parsed into individual token strings.
@param tokenList A pointer to an array of token strings. This must be freed using freeTokenList when no longer needed.
returns A count of the tokens parsed from input.
*/

int tokenList(char* delims, char* input, char*** tokenListArray)
{

    int listCount = 0;
    int listSize = 10;        // Initial estimate of the size of the list. Expanded when required.
    char* item, * work;
    char** list;

    *tokenListArray = NULL;
    if (strlen(delims) == 0 || strlen(input) == 0) return 0;

    list = (char**) malloc(listSize * sizeof(char*));        // Array of strings (tokens)

    work = (char*) malloc((strlen(input) + 1) * sizeof(char));    // Copy the input string into a local work buffer
    strcpy(work, input);

    item = strtok(work, delims);    // First token
    list[listCount] = (char*) malloc((strlen(item) + 1) * sizeof(char));
    strcpy(list[listCount++], item);

    while ((item = strtok(NULL, delims)) != NULL) {
        if (listCount == listSize) {                // Expand List when required
            listSize = listSize + 10;
            list = (char**) realloc((void*) (list), listSize * sizeof(char*));
        }
        list[listCount] = (char*) malloc((strlen(item) + 1) * sizeof(char));
        strcpy(list[listCount++], item);
    }

    free((void*) work);
    *tokenListArray = list;

    return listCount;
}

//----------------------------------------------------------------------------------------------
/*! Substitute/Replace file path name elements for server side name resolution.

Client and server may see different network file paths. A path seen by the client is different to the path seen by the
server. Generally, the path might contain additional hierarchical components that need removing. This is
easily done by substitution.

Examples:	/.automount/funsrv1/root/home/xyz -> /net/funsrv1/home/xyz
		/.automount/funsrv1/root/home/xyz -> /home/xyz
		/.automount/fuslsd/root/data/MAST_Data/013/13500/Pass0/amc0135.00 -> /net/fuslsd/data/MAST_Data/013/13500/Pass0/amc0135.00
		/scratch/mydata -> /net/hostname/scratch/mydata

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

#define MAXPATHSUBS        10
#define MAXPATHSUBSLENGTH    256

int pathReplacement(char* path)
{

//----------------------------------------------------------------------------------------------
// Does the Path contain hierarchical components not seen by the server? If so make a substitution.
//
// This replacement also occurs on the IDAM server
//
// pattern:	/A/B/C;/D/E/F -> /A/C;/E
// use multiple wild card characters '*' for 'any' target path element name
// target and replacement strings must begin with '/'
//
// If a '*' appears in both the target and substitute paths, the path element is retained in the replacement.
// The number of wildcards must match
//
// If a wildcard appears in the substitute string and none in the target string, an error is reported.
//
//----------------------------------------------------------------------------------------------

    char work[STRING_LENGTH];

    char* token, * delimiters = ",:;";
    char targets[MAXPATHSUBS][MAXPATHSUBSLENGTH];
    char substitutes[MAXPATHSUBS][MAXPATHSUBSLENGTH];
    int i, tcount = 0, scount = 0;

    int j, match, err = 0, lpath, k, kstart, subWildCount, targetWildCount;
    char** targetList, ** pathList, ** subList;
    int targetCount = 0, pathCount = 0, subCount = 0;

    if (path[0] == '\0') return 0;                // No replacement
    if (environment.private_path_target[0] == '\0') return 0;    // No replacement

    idamLog(LOG_DEBUG, "pathReplacement: Testing for File Path Replacement\n");
    idamLog(LOG_DEBUG, "%s\n", path);

// Parse targets

    strcpy(work, environment.private_path_target);
    token = strtok(work, delimiters);

    if (strlen(token) < MAXPATHSUBSLENGTH) {
        strcpy(targets[tcount++], token);
    } else {
        err = 999;
        addIdamError(&idamerrorstack, CODEERRORTYPE, "pathReplacement", err,
                     "The length of the path element targeted for replacement is too long. The internal limit is exceeded.");
        return err;
    }

    while ((token = strtok(NULL, delimiters)) != NULL && tcount < MAXPATHSUBS) {
        if (strlen(token) < MAXPATHSUBSLENGTH) {
            strcpy(targets[tcount++], token);
        } else {
            err = 999;
            addIdamError(&idamerrorstack, CODEERRORTYPE, "pathReplacement", err,
                         "The length of the path element targeted for replacement is too long. The internal limit is exceeded.");
            return err;
        }
    }

// Parse substitutes

    strcpy(work, environment.private_path_substitute);
    token = strtok(work, delimiters);

    if (strlen(token) < MAXPATHSUBSLENGTH) {
        strcpy(substitutes[scount++], token);
    } else {
        err = 999;
        addIdamError(&idamerrorstack, CODEERRORTYPE, "pathReplacement", err,
                     "The length of the targeted path element replacement is too long. The internal limit is exceeded.");
        return err;
    }

    while ((token = strtok(NULL, delimiters)) != NULL && scount < MAXPATHSUBS) {
        if (strlen(token) < MAXPATHSUBSLENGTH) {
            strcpy(substitutes[scount++], token);
        } else {
            err = 999;
            addIdamError(&idamerrorstack, CODEERRORTYPE, "pathReplacement", err,
                         "The length of the targeted path element replacement is too long. The internal limit is exceeded.");
            return err;
        }
    }


    if (tcount == scount) {

        for (i = 0; i < tcount; i++) {
            if (strchr(targets[i], '*') != NULL) {            // Wildcard found

// list of target tokens

                targetCount = tokenList(PATH_SEPARATOR, targets[i], &targetList);
                pathCount = tokenList(PATH_SEPARATOR, path, &pathList);

                if (pathCount < targetCount) {
                    freeTokenList(&targetList, &targetCount);
                    freeTokenList(&pathList, &pathCount);
                    continue;                    // Impossible substitution, so ignore this target
                }

                if (strchr(substitutes[i], '*') != NULL) {        // Wildcard found
                    subCount = tokenList(PATH_SEPARATOR, substitutes[i], &subList);
                    subWildCount = 0;
                    targetWildCount = 0;
                    for (j = 0; j < subCount; j++) if (subList[j][0] == '*') subWildCount++;
                    for (j = 0; j < targetCount; j++) if (targetList[j][0] == '*') targetWildCount++;
                    if (subWildCount != targetWildCount) {        // Un-matched Wildcards found
                        err = 999;
                        addIdamError(&idamerrorstack, CODEERRORTYPE, "pathReplacement", err,
                                     "Un-matched wildcards found in the target and substitute paths");
                        freeTokenList(&targetList, &targetCount);
                        freeTokenList(&pathList, &pathCount);
                        freeTokenList(&subList, &subCount);
                        return err;
                    }
                } else {
                    subCount = 0;
                }

                lpath = 0;
                match = 1;                    // Test path tokens against target tokens
                for (j = 0; j < targetCount; j++) {
                    match = match && (STR_EQUALS(targetList[j], pathList[j]) || targetList[j][0] == '*');
                    lpath = lpath + (int) strlen(pathList[j]) + 1;    // Find the split point
                    if (!match) break;
                }

                if (match) {                    // Make the substitution, ignoring wildcards
                    strcpy(work, &path[lpath]);
                    if (subCount == 0) {
                        strcpy(path, substitutes[i]);
                    } else {
                        kstart = 0;
                        path[0] = '\0';
                        for (j = 0; j < subCount; j++) {
                            strcat(path, PATH_SEPARATOR);
                            if (subList[j][0] == '*') {        // Substitute actual path element
                                for (k = kstart; k < targetCount; k++) {
                                    if (targetList[k][0] == '*') {
                                        kstart = k + 1;
                                        strcat(path, pathList[k]);
                                        break;
                                    }
                                }
                            } else {
                                strcat(path, subList[j]);
                            }
                        }
                    }
                    strcat(path, work);
                    break;
                }

            } else {

                if (strchr(substitutes[i], '*') != NULL) {        // Wildcard found in substitute string!
                    err = 999;
                    addIdamError(&idamerrorstack, CODEERRORTYPE, "pathReplacement", err,
                                 "No wildcards are permitted in the substitute path unless matched by one in the target path.");
                    freeTokenList(&targetList, &targetCount);
                    freeTokenList(&pathList, &pathCount);
                    return err;
                }

                lpath = (int) strlen(targets[i]);
                if (!strncmp(path, targets[i], lpath)) {        // Test for straight replacement
                    strcpy(work, &path[lpath]);
                    strcpy(path, substitutes[i]);
                    strcat(path, work);
                    break;
                }
            }

            freeTokenList(&targetList, &targetCount);
            freeTokenList(&pathList, &pathCount);
            freeTokenList(&subList, &subCount);

        }

    } else {
        err = 999;
        addIdamError(&idamerrorstack, CODEERRORTYPE, "pathReplacement", err,
                     "Number of Path Targets and Substitutes is inconsistent. Correct the Environment Variables.");
        return err;
    }

    idamLog(LOG_DEBUG, "%s\n", path);
    idamLog(LOG_DEBUG, "pathReplacement: End\n");

    return err;
}

// Client side only

#ifndef SERVERBUILD

int linkReplacement(char* path)
{

//----------------------------------------------------------------------------------------------
// Is the path a symbolic link not seen by the server? If so make a substitution.
//----------------------------------------------------------------------------------------------

    int err;
    FILE* ph = NULL;
    char* p;
    char cmd[STRING_LENGTH];

#ifdef _WIN32
    return path;		// No check for windows
#endif

//------------------------------------------------------------------------------------
//! Dereference path links using a command pipe: Ignore any errors
//
// If the user has embedded linux commands within the source string, they will be exposed here
// within the client's environment - not the server's.

    sprintf(cmd, "ls -l %s 2>&1;", path);

    errno = 0;
    if ((ph = popen(cmd, "r")) == NULL) {
        if (errno != 0) addIdamError(&idamerrorstack, SYSTEMERRORTYPE, "linkReplacement", errno, "");
        err = 1;
        addIdamError(&idamerrorstack, CODEERRORTYPE, "linkReplacement", err, "Unable to Dereference Symbolic links");
        path[0] = '\0';
        return err;
    }

    if (!feof(ph)) fgets(cmd, STRING_LENGTH - 1, ph);
    fclose(ph);

//------------------------------------------------------------------------------------
//! Extract the Dereferenced path. Accept only if it is Not a Relative path

    if ((p = strstr(cmd, " -> ")) != NULL) {
        if (p[4] == '/') {
            strcpy(path, p + 4);
            convertNonPrintable2(path);
            TrimString(path);
            //expandFilePath(path);
        }
    }

    return 0;
}

#else
int linkReplacement(char *path) { // Links are resolved client side only
    return 0;
}
#endif

//------------------------------------------------------------------------------------------------------------------
//------------------------------------------------------------------------------------------------------------------
/*! Fully expand file directory paths to remove relative path or environment variable components.

 Examples:	filename			use getpwd
		./filename			use cd; $PWD
		../../filename			use cd; $PWD
		/abc/filename			do nothing - fully resolved
		~user/abc/filename		use cd; $PWD
		/scratch/abc/filename		use hostname
		/tmp/abc/def/filename		use hostname
		/fuslwx/scratch/abc/filename	do nothing - fully resolved
		/fuslwx/tmp/abc/filename	do nothing - fully resolved
		/fuslwx/abc/filename		do nothing - fully resolved
		/99999  			do nothing - Resolved by the Server Data Plugin
		/99999/999 			do nothing - Resolved by the Server Data Plugin
		$ENVAR/abc/filename		expand with the specified environment variable
		$ENVAR/abc/$ENVAR/filename	expand with the specified environment variable

@param path The file path to be resolved and expanded.
@returns An integer Error Code: If non zero, a problem occured.
*/

int expandFilePath(char* path)
{

//
//----------------------------------------------------------------------------------------------

    char* fp = NULL, * fp1 = NULL, * env = NULL;
    char file[STRING_LENGTH];
#ifndef NOHOSTPREFIX
    char host[STRING_LENGTH];
#endif
    char cwd[STRING_LENGTH];
    char ocwd[STRING_LENGTH];        // Current Working Directory
    char opath[STRING_LENGTH];        // Original Path string
    char work[STRING_LENGTH];
    char work1[STRING_LENGTH];
    char scratch[STRING_LENGTH];
    char netname[STRING_LENGTH];
    char* pcwd = cwd;
    char* token = NULL;

    int lcwd = STRING_LENGTH - 1;
    int lpath, err = 0;
    size_t lscratch;
    int t1, t2, t3, t4, t5, t6;

#ifdef _WIN32
    return 0;			// No expansion for windows
#endif

//------------------------------------------------------------------------------------------------------------------
// Test for possible imbedded linux command

#ifndef _WIN32
    if (!IsLegalFilePath(path)) {
        err = 999;
        addIdamError(&idamerrorstack, CODEERRORTYPE, "expandFilePath", err, "The Source contains a Syntax Error!");
        return err;
    }
#endif

//------------------------------------------------------------------------------------------------------------------
    /*! Workstations have local hard drives that may also be mounted on the user's network. If this local disk is accessible
    by the IDAM server it's name needs to be expanded using additional information. As all similar workstations on the network might use
    the same name for the local disk area, the host name is used to uniquely identify where the disk is on the network.

    The name model adopted to expand directory paths to local disk drives is: /local -> /NETNAME/HOSTNAME/local

    NETNAME is defined by the environment variable IDAM_NETWORKNAME

    The hostname of the local workstation is obtained directly from the operating system. If this fails, it can be included
    directly in the IDAM_NETWORKNAME environment variable.

    The local disk directory, e.g. /scratch or /tmp, is targeted using the name defined by the environment variable IDAM_SCRATCHNAME

    Only ONE local drive can be targeted using the environment variable (current set up could be expanded to use a list). The path
    replacement function could be used to target other local directories.
    */

#ifndef _WIN32
#ifdef SCRATCHDIR
    sprintf(scratch, "/%s/", SCRATCHDIR);
    lscratch = (int)strlen(scratch);
#else
    strcpy(scratch, "/scratch/");
    lscratch = 9;
#endif
#else
#ifdef SCRATCHDIR
    strcpy(scratch, SCRATCHDIR);
    lscratch = (int)strlen(scratch);
#else
    scratch = '\0';
    lscratch = 0;
#endif
#endif

#ifdef NETPREFIX
    strcpy(netname, NETPREFIX);
#else
    netname[0] = '\0';
#endif

// Override compiler options

    if ((env = getenv("UDA_SCRATCHNAME")) != NULL) {    // Check for Environment Variable
#ifndef _WIN32
        sprintf(scratch, "/%s/", env);
#else
        strcpy(scratch, env);
#endif
        lscratch = (int) strlen(scratch);
    }

    if ((env = getenv("UDA_NETWORKNAME")) != NULL) {
        strcpy(netname, env);
    }


//------------------------------------------------------------------------------------------------------------------
    /*! File Path names are only resolved if they contain relative path elements, environment variables, or
     references to a local scratch disk, e.g., ./ ../ ~ $ /scratch
    */

    if ((lpath = (int) strlen(path)) == 0) return 0;        // Nothing to resolve

// Test for necessary expansion

#ifndef _WIN32
    t1 = strstr(path, "./") == NULL;            // relative path?
    t2 = strstr(path, "../") == NULL;            // relative path?
    t3 = strchr(path, '~') == NULL;            // home path?
    t4 = strchr(path, '$') == NULL;            // No imbedded Environment variable
    t5 = path[0] == '/';                    // No Relative Directory path
    t6 = strncmp(path, scratch, lscratch) != 0;        // Not the Scratch directory

    if (t1 && t2 && t3 && t4 && t5 && t6) return 0;    // No Relative path name elements found
#else
    return 0;
#endif

//------------------------------------------------------------------------------------------------------------------
    /*! The path argument has been stripped of any format or protocol prefix. If another prefix is within the path, this
    indicates the source is server to server. Therefore, the main source element is a URL and is not expanded.

    If the ':' character is detected in the source prefix, it is assumed the request is is via a server (the : indicates a port
    number) and the main source component is a URL. URLs are not path expanded.

    Server side functions are identified via the pair of parnethesis enclosing arguments. These are not expanded.
    */

#ifdef SERVERELEMENTCHECK
    int t7,t8,t9;
    t7 = strstr(path,environment.api_delim) != NULL;		// Pass request forward to another server
    t8 = strchr(path,':') != NULL;				// Port number => Server
    t9 = strchr(path,'(') != NULL && strchr(path,')') != NULL;	// Server Side Function

    if(t7 || t8 || t9) return 0;					// Server host, protocol, and server side functions
#endif

//------------------------------------------------------------------------------------------------------------------
// Test if the Path begins with an Integer or /Integer => Resolved by the Server Data Plugin

    if (path[0] == '/') {
        strcpy(work, path + 1);                // the leading character is a / so ignore
    } else {
        strcpy(work, path);
    }

    token = strtok(work, "/");

    if (token != NULL) if (IsNumber(token)) return 0;    // Is the First token an integer number?

//------------------------------------------------------------------------------------------------------------------
//! Identify the Current Working Directory

    if (!t1 || !t2 || !t3 || !t4 || !t5) {

        work[0] = '\0';
        errno = 0;
        strcpy(opath, path);

        pcwd = getcwd(cwd, lcwd);

        if (errno != 0) {
            err = 999;
            addIdamError(&idamerrorstack, SYSTEMERRORTYPE, "expand_path", errno,
                         "Cannot resolve the Current Working Directory!");
            addIdamError(&idamerrorstack, CODEERRORTYPE, "expand_path", err,
                         "Unable to resolve full file names.");
            return err;
        }

        if (pcwd == NULL) {
            err = 999;
            addIdamError(&idamerrorstack, CODEERRORTYPE, "expand_path", err,
                         "Cannot resolve the Current Working Directory! Unable to resolve full file names.");
            return err;
        }

        strcpy(ocwd, cwd);

//! Does the path NOT contain a path directory separator character => filename only so prepend the CWD and return

        if ((fp = strrchr(path, '/')) == NULL) {        // Search backwards - extract filename
            strcpy(work1, path);
            sprintf(path, "%s/%s", cwd, work1);        // prepend the CWD and return
            if ((err = linkReplacement(path)) != 0) return err;
            if ((err = pathReplacement(path)) != 0) return err;

            if ((t6 = (strncmp(path, scratch, lscratch) != 0))) return 0;    // Not the Scratch directory ?
            fp = strrchr(path, '/');                    // extract filename
        }

        strcpy(file, &fp[1]);                // Filename
        fp[1] = '\0';                    // Split the path string: path now contains directory only

//! Does the Path contain with an Environment variable (Not resolved by the function chdir!)

        fp = NULL;
        lpath = (int) strlen(path);
        if (lpath > 0) fp = strchr(path + 1, '$');

        if (path[0] == '$' || fp != NULL) {            // Search for a $ character

            if (fp != NULL) {
                strncpy(work, path, fp - path);
                work[fp - path] = '\0';

                if ((fp1 = strchr(fp, '/')) != NULL) {
                    strncpy(work1, fp + 1, fp1 - fp - 1);
                    work1[fp1 - fp - 1] = '\0';
                } else strcpy(work1, fp + 1);

                if ((env = getenv(work1)) !=
                    NULL) {    // Check for Environment Variable: If not found then assume it's server side
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
                if ((fp = strchr(path, '/')) != NULL) {
                    strncpy(work, path + 1, fp - path - 1);
                    work[fp - path - 1] = '\0';
                    strcpy(work1, fp);
                } else strcpy(work, path + 1);

                if ((env = getenv(work)) !=
                    NULL) {    // Check for Environment Variable: If not found then assume it's server side
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


        /*! Change to the File's Directory (will resolve relative path elements ~/./../$envVariable/ as well as links)

        If an error occurs, either the directory doesn't exist or the path is not a file path!
        If the path contains an environment variable, then resolved server side
        */

        if (chdir(path) != 0) {
            chdir(ocwd);            // Ensure the Original WD
            strcpy(path, opath);        // Return to the Original path name
            idamLog(LOG_DEBUG, "Unable to identify the Directory of the file: %s\n"
                    "The server will know if a true error exists: Plugin & Environment dependent", path);
            return 0;
        }

//! The Current Working Directory is the resolved directory name

        errno = 0;

        pcwd = getcwd(cwd, lcwd);

        if (errno != 0) {
            err = 998;
            addIdamError(&idamerrorstack, SYSTEMERRORTYPE, "expand_path", errno,
                         "Cannot resolve the Current Working Directory!");
            addIdamError(&idamerrorstack, CODEERRORTYPE, "expand_path", err,
                         "Unable to resolve full file names.");
            return err;
        }

        if (pcwd == NULL) {
            err = 998;
            addIdamError(&idamerrorstack, CODEERRORTYPE, "expand_path", err,
                         "Cannot resolve the Current Working Directory! Unable to resolve full file names.");
            return err;
        }

        strcpy(work1, cwd);

//! Return to the Original Working Directory

        if ((err = chdir(ocwd)) != 0) {
            err = 999;
            addIdamError(&idamerrorstack, SYSTEMERRORTYPE, "expand_path", errno,
                         "Unable to Return to the Working Directory!");
            addIdamError(&idamerrorstack, CODEERRORTYPE, "expand_path", err,
                         "Unable to resolve full file names.");
            return err;
        }

//! Prepend the expanded/resolved directory name to the File Name

        sprintf(path, "%s/%s", work1, file);        // Prepend the path to the filename

    }    // End of t1 - t5 tests


//----------------------------------------------------------------------------------------------
    /*! Symbolic Links might not be visible by the server: Pass the true location
    */

    if ((err = linkReplacement(path)) != 0) return err;

//----------------------------------------------------------------------------------------------
    /*! Does the path contain the client workstation's local Scratch directory (Must be visible by the server)

     Model:	/scratch/a/b/c -> /hostname/scratch/a/b/c			(default)
    	/scratch/a/b/c -> /netname/hostname/scratch/a/b/c

    */

#ifndef NOHOSTPREFIX

    t6 = strncmp(path, scratch, lscratch) != 0;        // Retest for the Scratch directory

    if (!t6) {                        // Scratch directory used without hostname prefix
        strcpy(work, path);
        if (!strncmp(work, scratch, lscratch)) {        // case sensistive

            if ((env = getenv("HOSTNAME")) != NULL) {    // Check for a system Environment Variable
                strcpy(host, env);
            } else {
                hostid(host);                // Identify the Name of the Current Workstation or Host
            }

            if (strlen(netname) > 0 && strlen(host) > 0) {
                sprintf(path, "/%s/%s%s", netname, host, work);    // prepend /netname/hostname to /scratch/...
            } else {
                if (strlen(netname) > 0) {
                    sprintf(path, "/%s%s", netname, work);
                } else {
                    if (strlen(host) > 0) {
                        sprintf(path, "/%s%s", host, work);
                    }
                }
            }
        }
    }

#endif

//----------------------------------------------------------------------------------------------
    /*! Does the Path to a user's Private Files contain network components not seen by the server?
    If so, target these and make a suitable substitution to resolve path problems.
    */
    err = pathReplacement(path);

    return err;
}

//----------------------------------------------------------------------------------------------
/*! Test a path is legitimate

@param path The file path to be tested.
@returns A pointer to the path argument. If a problem occurs, the path string is empty.
*/

char* pathid(char* path)
{

#ifdef _WIN32
    return path;		// No check for windows
#endif

    char* p;
    char work[STRING_LENGTH];        // Are these consistent with the system MAX_PATH?
    char pwd[STRING_LENGTH];
    strcpy(work, path);

// the path string may contain malign embedded linux commands: is chdir secure?
// basic check

    if (!IsLegalFilePath(path)) {
        addIdamError(&idamerrorstack, CODEERRORTYPE, "pathid", 999, "The directory path has incorrect syntax");
        path[0] = '\0';
        return path;
    }

    if ((p = getcwd(pwd, STRING_LENGTH - 1)) != NULL) {
        errno = 0;
        if (chdir(path) == 0) {
            if ((p = getcwd(pwd, STRING_LENGTH - 1)) != NULL) {
                strcpy(path, p);
                chdir(pwd);
                TrimString(path);
                LeftTrimString(path);
                return path;
            }
        } else {
            if (errno == EACCES) {
                addIdamError(&idamerrorstack, SYSTEMERRORTYPE, "pathid", errno, "");
                addIdamError(&idamerrorstack, CODEERRORTYPE, "pathid", 999, "The directory path is not available");
            } else if (errno == ENOENT || errno == ENOTDIR) {
                addIdamError(&idamerrorstack, SYSTEMERRORTYPE, "pathid", errno, "");
                addIdamError(&idamerrorstack, CODEERRORTYPE, "pathid", 999, "The directory path does not exist");
            }
        }
    }
    path[0] = '\0';
    return path;
}

#endif
