// Create and Manage a List of Data Source File Handles
//
// Routines addIdamFile and getIdamFile return 0 if successfull otherwise return 1
//
//	IDA: 	ida_file_ptr *ida_file_id;
//	netCDF: int           fd
//	HDF5:	hid_t         file_id
//
//----------------------------------------------------------------------------------
#include "manageFiles.h"

#include <stdlib.h>
#include <strings.h>

#include <logging/idamLog.h>
#include <clientserver/protocol.h>
#include <server/idamServer.h>

// Initialise the File List

void initIdamFileList(IDAMFILELIST* idamfiles)
{
    idamfiles->nfiles = 0;
    idamfiles->files = NULL;
}

// Add a New Open File to the List or re-Open an existing record
// returns 1 if the handle already exists, 0 if not

int addIdamFile(IDAMFILELIST* idamfiles, int type, char* filename, void* handle)
{
    void* old_handle = NULL;
    int closed = -1;

    if ((old_handle = getOpenIdamFile(idamfiles, type, filename)) !=
        NULL) {    // Is an Open File Handle already listed?
        if (old_handle == handle) return 1;
    }

    if ((closed = getClosedIdamFile(idamfiles, type, filename)) >= 0) {    // Does a Closed File Handle entry exist?
        idamfiles->files[closed].status = 1;                // If so then reopen and update the record
        if (type == REQUEST_READ_IDA) idamfiles->files[closed].ida = (ida_file_ptr*) handle;
        if (type == REQUEST_READ_CDF) idamfiles->files[closed].netcdf = *((int*) handle);
        if (type == REQUEST_READ_HDF5) idamfiles->files[closed].hdf5 = *((hid_t*) handle);
        gettimeofday(&idamfiles->files[closed].file_open, NULL);
        return 0;
    }

// New File (Close Stalest File if at maximum open handle limit)

    if (idamfiles->nfiles >= MAXOPENFILEDESC) purgeStalestIdamFile(idamfiles);

    idamfiles->files = (IDAMFILE*) realloc((void*) idamfiles->files, (idamfiles->nfiles + 1) * sizeof(IDAMFILE));
    idamfiles->files[idamfiles->nfiles].type = type;
    idamfiles->files[idamfiles->nfiles].status = 1;
    if (type == REQUEST_READ_IDA) idamfiles->files[idamfiles->nfiles].ida = (ida_file_ptr*) handle;
    if (type == REQUEST_READ_CDF) idamfiles->files[idamfiles->nfiles].netcdf = *((int*) handle);
    if (type == REQUEST_READ_HDF5) idamfiles->files[idamfiles->nfiles].hdf5 = *((hid_t*) handle);

    strcpy(idamfiles->files[idamfiles->nfiles].filename, filename);
    gettimeofday(&idamfiles->files[idamfiles->nfiles].file_open, NULL);

    (idamfiles->nfiles)++;    // Count of Open File handles
    return 0;
}

// Search for an Open File in the List
// Returns an opaque pointer to the appropriate handle if found, NULL othewise.

void* getOpenIdamFile(IDAMFILELIST* idamfiles, int type, char* filename)
{
    int i;
    IDAM_LOGF(LOG_DEBUG, "Open File Count %d\n", idamfiles->nfiles);
    for (i = 0; i < idamfiles->nfiles; i++) {

        IDAM_LOGF(LOG_DEBUG, "Status %d, Type %d, Name %s [%s]\n",
                idamfiles->files[i].status, idamfiles->files[i].type, idamfiles->files[i].filename, filename);

        if (idamfiles->files[i].status == 1 && idamfiles->files[i].type == type) {
            if (!strcasecmp(filename, idamfiles->files[i].filename)) {
                gettimeofday(&idamfiles->files[i].file_open, NULL);        // Refresh Time of Last use
                if (idamfiles->files[i].type == REQUEST_READ_IDA) return ((void*) idamfiles->files[i].ida);
                if (idamfiles->files[i].type == REQUEST_READ_CDF) return ((void*) &idamfiles->files[i].netcdf);
                if (idamfiles->files[i].type == REQUEST_READ_HDF5) return ((void*) &idamfiles->files[i].hdf5);
            }
        }
    }
    return NULL;    // Not Found => Not open
}

// Search for a Closed File in the List

int getClosedIdamFile(IDAMFILELIST* idamfiles, int type, char* filename)
{
    int i;
    for (i = 0; i < idamfiles->nfiles; i++) {
        if (idamfiles->files[i].status == 0) {    // Only check Close handle records
            if (!strcasecmp(filename, idamfiles->files[i].filename) && idamfiles->files[i].type == type) return (i);
        }
    }
    return -1;    // No Closed File Found
}


void closeIdamFile(IDAMFILELIST* idamfiles, char* filename)
{
    int i;
    for (i = 0; i < idamfiles->nfiles; i++) {
        if (idamfiles->files[i].status == 1) {    // Only check Open handle records
            if (!strcasecmp(filename, idamfiles->files[i].filename)) {
                if (idamfiles->files[i].type == REQUEST_READ_IDA) ida_close((ida_file_ptr*) idamfiles->files[i].ida);
                if (idamfiles->files[i].type == REQUEST_READ_CDF) ncclose((int) idamfiles->files[i].netcdf);
                if (idamfiles->files[i].type == REQUEST_READ_HDF5) H5Fclose((hid_t) idamfiles->files[i].hdf5);
                idamfiles->files[i].status = 0;
                break;
            }
        }
    }
}

void closeIdamFiles(IDAMFILELIST* idamfiles)
{
    int i;
    for (i = 0; i < idamfiles->nfiles; i++) closeIdamFile(idamfiles, idamfiles->files[i].filename);
    if (idamfiles->files != NULL) free((void*) idamfiles->files);
    initIdamFileList(idamfiles);
    return;
}

void purgeStalestIdamFile(IDAMFILELIST* idamfiles)
{
    int i, stalest = -1;
    struct timeval oldest;
    gettimeofday(&oldest, NULL);                // Start comparison with Present time

    for (i = 0; i < idamfiles->nfiles; i++) {
        if (idamfiles->files[i].status == 0) {        // Drop Closed Files First
            if (idamfiles->files[i].file_open.tv_sec < oldest.tv_sec ||
                (idamfiles->files[i].file_open.tv_sec == oldest.tv_sec &&
                 idamfiles->files[i].file_open.tv_usec <= oldest.tv_usec)) {
                oldest = idamfiles->files[i].file_open;
                stalest = i;
            }
        }
    }

    if (stalest < 0) {                    // No Closed files so choose an Open File
        stalest = 0;
        for (i = 0; i < idamfiles->nfiles; i++) {
            if (idamfiles->files[i].file_open.tv_sec < oldest.tv_sec ||
                (idamfiles->files[i].file_open.tv_sec == oldest.tv_sec &&
                 idamfiles->files[i].file_open.tv_usec <= oldest.tv_usec)) {
                oldest = idamfiles->files[i].file_open;
                stalest = i;
            }
        }
    }

    if (stalest >= 0) {
        if (idamfiles->files[stalest].status == 1) {    // Close the Stale File
            if (idamfiles->files[stalest].type == REQUEST_READ_IDA)
                ida_close((ida_file_ptr*) idamfiles->files[stalest].ida);
            if (idamfiles->files[stalest].type == REQUEST_READ_CDF) ncclose((int) idamfiles->files[stalest].netcdf);
            if (idamfiles->files[stalest].type == REQUEST_READ_HDF5) H5Fclose((hid_t) idamfiles->files[stalest].hdf5);
        }
        for (i = stalest; i < idamfiles->nfiles - 1; i++)
            idamfiles->files[i] = idamfiles->files[i + 1];    // Overwrite Stale Position
        idamfiles->files[idamfiles->nfiles - 1].status = 0;
        idamfiles->files[idamfiles->nfiles - 1].type = 0;
        idamfiles->files[idamfiles->nfiles - 1].filename[0] = '\0';
        idamfiles->nfiles = idamfiles->nfiles - 1;
    }

    return;
}


