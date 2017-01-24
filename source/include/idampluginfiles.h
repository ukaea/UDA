// Plugin Open File List

#ifndef IDAM_SERVER_IDAMPLUGINFILES_H
#define IDAM_SERVER_IDAMPLUGINFILES_H

// Change History:
//
// 10Jul2014    dgm Original Version

#include "idamclientserverpublic.h"
#include <time.h>
#include <clientserver/idamDefines.h>

#ifdef __cplusplus
extern "C" {
#endif

#define IDAMPLUGINFILEALLOC     10
#define MAXOPENPLUGINFILEDESC   50

//------------------------------------------------------------------------------------------------------------------------
// Plugins opening files can maintain state by recording file handles in a list.
// Two usage patterns are assumed: the file opening function returns an integer or a pointer
// Also assumed is the function used to close the file has a single argument - the returned value from the opening function.
// A function pointer to the Close function needs to be registerd the first time the plugin is called.
//------------------------------------------------------------------------------------------------------------------------

struct IDAMPLUGINFILE {
    int status;             // Open (1) or Closed (0)
    char filename[STRING_LENGTH];   // Full Data Source Filename
    int handleInt;          // Integer File Handle
    void* handlePtr;           // Pointer file handle
    struct timeval file_open;       // File Open Clock Time
};
typedef struct IDAMPLUGINFILE IDAMPLUGINFILE;

struct IDAMPLUGINFILELIST {
    int count;              // Number of Files
    int mcount;             // malloc count allocated
    IDAMPLUGINFILE* files;     // Array of File Handles
    void* close;               // Function pointer to the File Close API function
};
typedef struct IDAMPLUGINFILELIST IDAMPLUGINFILELIST;

#ifdef __cplusplus
}
#endif

#endif // IDAM_SERVER_IDAMPLUGINFILES_H
