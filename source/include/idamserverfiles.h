//! $LastChangedRevision: 264 $
//! $LastChangedDate: 2011-06-02 11:25:56 +0100 (Thu, 02 Jun 2011) $
//! $LastChangedBy: dgm $
//! $HeadURL: https://fussvn.fusion.culham.ukaea.org.uk/svnroot/IDAM/development/source/include/idamserverfiles.h $

#ifndef IdamServerFiles
#define IdamServerFiles

// Server Side Header
//
// Change History
//
// 26Mar2007 dgm    Data Source File Handle Management
// 02Apr2008 dgm    C++ test added for inclusion of extern "C"
//----------------------------------------------------------------/

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

struct IDAMFILE {
    int type;               // File Type Code
    int status;             // Open (1) or Closed (0)
    char filename[STRING_LENGTH];   // Full Data Source Filename
    int netcdf;             // netCDF File Handle
    hid_t hdf5;             // HDF5 File id
    ida_file_ptr* ida;         // IDA File Structure
    struct timeval file_open;       // File Open Clock Time
};
typedef struct IDAMFILE IDAMFILE;

struct IDAMFILELIST {
    int nfiles;         // Number of Sockets
    IDAMFILE* files;       // Array of Socket Management Data
};
typedef struct IDAMFILELIST IDAMFILELIST;

extern IDAMFILELIST idamfilelist;

#ifdef __cplusplus
}
#endif

#endif
