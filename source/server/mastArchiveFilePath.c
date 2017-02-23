/*---------------------------------------------------------------
* Forms Fully Qualified Path to a Specified File in the Standard MAST Archive
*
* Input Arguments:	int pulno:	Pulse Number
*			int pass:	Pass Number. If -1 then LATEST directory accessed
*			char *file	Name of the File
*
* Returns:		char *path	Directory Path to the File
*
* Calls
* Notes:
* ToDo:
*
*-----------------------------------------------------------------------------*/
#include "mastArchiveFilePath.h"
#include "getServerEnvironment.h"

#include <stdlib.h>
#include <string.h>
#include <stdio.h>

#include <clientserver/udaStructs.h>

void mastArchiveFilePath(int pulno, int pass, char* file, char* path)
{
    char strint[56];
    char* env = NULL;

// Path Root

    strcpy(path, DEFAULT_ARCHIVE_DIR);        // Default location

    if ((env = getenv("MAST_DATA")) != NULL) {    // MAST Archive Root Directory Environment Variable?
        strcpy(path, env);
        if (path[strlen(path)] != '/') strcat(path, "/");
    }

// Alternative Paths

    if (getIdamServerEnvironment()->data_path_id == 0) {
        sprintf(strint, "%d", pulno);
        strcat(path, strint);            // Original naming convention
    } else {
        if (pulno <= 99999)
            sprintf(strint, "/0%d/%d", pulno / 1000, pulno);
        else
            sprintf(strint, "/%d/%d", pulno / 1000, pulno);
        strcat(path, strint);
    }

// Add the Pass element to the Path

    if (pass == -1) {
        strcat(path, "/LATEST/");
    } else {
        strcat(path, "/Pass");
        sprintf(strint, "%d", pass);
        strcat(path, strint);
        strcat(path, "/");
    }

    strcat(path, file);        // Full filename path

    return;
}
