/* Use funopen(3) to provide open_memstream(3) like functionality. */

#include "memstream.h"

#include <windows.h>
#include <cstdlib>
#include <string.h>
#include <cerrno>
#include <io.h>

#define MAX_CACHE_SIZE 500000000

FILE *open_memstream(char **cp, size_t *lenp) {
    int fd;
    FILE *stream;

    stream = tmpfile();
    fd = _fileno(stream);
    
    HANDLE fm;
    HANDLE h = (HANDLE) _get_osfhandle (fd);
    *lenp=MAX_CACHE_SIZE;

    fm = CreateFileMapping(h, NULL, PAGE_READWRITE|SEC_RESERVE, 0, MAX_CACHE_SIZE, NULL);
    
    if (fm == NULL) { 
        fprintf (stderr, "memstream_win: Couldn't access memory space! %s\n",  strerror (GetLastError()));
        exit(GetLastError());
    }

    *cp = (char*)MapViewOfFile(fm, FILE_MAP_ALL_ACCESS, 0, 0, 0);
            
    if (*cp == NULL) { 
        fprintf (stderr, "memstream_win: Couldn't fill memory space! %s\n",  strerror (GetLastError()));
        exit(GetLastError());
    }

    return stream;
}
