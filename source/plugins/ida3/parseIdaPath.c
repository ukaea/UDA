/*---------------------------------------------------------------
* Identify the IDA File and it's Location
*
* Input Arguments:	1) Request_Block
*
* Assumes: path has the form /exp_number/pass
*--------------------------------------------------------------*/

#include <TrimString.h>
#include <idamclientserverpublic.h>

void parseIDAPath(REQUEST_BLOCK* request_block)
{
    char* token = NULL;
    char work[STRING_LENGTH] = "";

    if (request_block->path[0] == '\0') return;    // Nothing to work with!

//------------------------------------------------------------------------------
// Extract Exp_Number or Source Name

    if (request_block->path[0] == '/')
        strcpy(work, request_block->path + 1);    // the leading character is a / so ignore
    else
        strcpy(work, request_block->path);

    token = strtok(work, "/");

    if (token != NULL) {                    // Tokenise the remaining path string
        if (IsNumber(token)) {                // Is the First token an integer number?
            request_block->exp_number = atoi(token);    // It must be the Exp_number
            if ((token = strtok(NULL, "/")) != NULL) {    // Next Token
                if (IsNumber(token)) {
                    request_block->pass = atoi(token);    // Followed by the Pass number
                } else {
                    strcpy(request_block->tpass, token);    // or something else known to the plugin
                }
            }
            strcpy(request_block->path, "");
            strncpy(request_block->file, request_block->signal, 3);    // IDA Source alias
            request_block->file[3] = '\0';
        }
    } else {
        if (IsNumber(work)) {                // Is the Only token an integer number?
            request_block->exp_number = atoi(work);    // It must be the Exp_number
            strcpy(request_block->path, "");
            strncpy(request_block->file, request_block->signal, 3);
            request_block->file[3] = '\0';
        }
    }

    return;
}

void parseXMLPath(REQUEST_BLOCK* request_block)
{
    char* token = NULL;
    char work[STRING_LENGTH] = "";

    if (request_block->path[0] == '\0') return;    // Nothing to work with!

//------------------------------------------------------------------------------
// Extract Exp_Number and Pass Number

    if (request_block->path[0] == '/')
        strcpy(work, request_block->path + 1);    // the leading character is a / so ignore
    else
        strcpy(work, request_block->path);

    token = strtok(work, "/");

    if (token != NULL) {                    // Tokenise the remaining string
        if (IsNumber(token)) {                // Is the First token an integer number?
            request_block->exp_number = atoi(token);    // It must be the Exp_number
            if ((token = strtok(NULL, "/")) != NULL) {    // Next Token
                if (IsNumber(token)) {
                    request_block->pass = atoi(token);    // Followed by the Pass number
                } else {
                    strcpy(request_block->tpass, token);    // or something else known to the plugin
                }
            }
            strcpy(request_block->path, "");
        }
    } else {
        if (IsNumber(work)) {                // Is the Only token an integer number?
            request_block->exp_number = atoi(work);    // It must be the Exp_number
            strcpy(request_block->path, "");
        }
    }

    return;
}

