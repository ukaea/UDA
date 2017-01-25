#ifndef IdamServerFiles
#define IdamServerFiles

#ifdef __cplusplus
extern "C" {
#endif

#include <sys/time.h>

#ifndef NOHDF5PLUGIN
#  include <hdf5.h>
#else
#  define H5_DLL
typedef int hid_t;
void H5Fclose(int fd);
//YM static void H5Fclose(int fd);
#endif

#ifndef NONETCDFPLUGIN
#  include <netcdf.h>
#else
void ncclose(int fd);
//YM static void ncclose(int fd);
#endif

#ifndef NOIDAPLUGIN
#  include <ida3.h>
#else
typedef int ida_file_ptr;
void ida_close(ida_file_ptr*);
#endif

#include <clientserver/idamDefines.h>

typedef struct IdamFile {
    int type;                       // File Type Code
    int status;                     // Open (1) or Closed (0)
    char filename[STRING_LENGTH];   // Full Data Source Filename
    int netcdf;                     // netCDF File Handle
    hid_t hdf5;                     // HDF5 File id
    ida_file_ptr* ida;              // IDA File Structure
    struct timeval file_open;       // File Open Clock Time
} IDAMFILE;

typedef struct IdamFileList {
    int nfiles;                     // Number of Sockets
    IDAMFILE* files;                // Array of Socket Management Data
} IDAMFILELIST;

extern IDAMFILELIST idamfilelist;

#ifdef __cplusplus
}
#endif

#endif
