#ifndef UDA_PLUGINS_UDAPLUGINFILES_H
#define UDA_PLUGINS_UDAPLUGINFILES_H

#include <time.h>
#if defined(__GNUC__)
#  include <sys/time.h>
#else
#  include <winsock2.h>
#endif

#include "clientserver/uda_defines.h"

#ifdef __cplusplus
extern "C" {
#endif

#define UDA_FILENAME_LENGTH 1024

//------------------------------------------------------------------------------------------------------------------------
// Plugins opening files can maintain state by recording file handles in a list.
// Two usage patterns are assumed: the file opening function returns an integer or a pointer
// Also assumed is the function used to close the file has a single argument - the returned value from the opening
// function. A function pointer to the Close function needs to be registerd the first time the plugin is called.
//------------------------------------------------------------------------------------------------------------------------

typedef struct UdaPluginFile {
    int status;                   // Open (1) or Closed (0)
    char filename[UDA_FILENAME_LENGTH]; // Full Data Source Filename
    long handleInt;               // Integer File Handle
    void* handlePtr;              // Pointer file handle
    struct timeval file_open;     // File Open Clock Time
} UDA_PLUGIN_FILE;

typedef struct UdaPluginFileList {
    int count;              // Number of Files
    int mcount;             // malloc count allocated
    UDA_PLUGIN_FILE* files; // Array of File Handles
    void* close;            // Function pointer to the File Close API function
} UDA_PLUGIN_FILE_LIST;

#ifdef __cplusplus
}
#endif

#endif // UDA_PLUGINS_UDAPLUGINFILES_H
