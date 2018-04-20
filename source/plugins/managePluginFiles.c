// Open File State Management
//
//----------------------------------------------------------------------------------

#include "managePluginFiles.h"

#include <stdlib.h>
#include <strings.h>

#include <logging/logging.h>
#include <clientserver/stringUtils.h>

// Initialise the File List and allocate heap for the list

void initIdamPluginFileList(IDAMPLUGINFILELIST* idamfiles)
{
    idamfiles->count = 0;
    idamfiles->mcount = 0;
    idamfiles->files = NULL;
    idamfiles->close = NULL;
}

// Add the file closing function pointer (same for all entries)

void registerIdamPluginFileClose(IDAMPLUGINFILELIST* idamfiles, void* fptr)
{
    idamfiles->close = fptr;
}

// Add a New Open File to the List or re-Open an existing record
// returns 1 if the handle already exists, 0 if not

int addIdamPluginFilePtr(IDAMPLUGINFILELIST* idamfiles, const char* filename, void* handle)
{
    void* old_handle = NULL;
    int closed;

    if ((old_handle = getOpenIdamPluginFilePtr(idamfiles, filename)) !=
        NULL) {    // Is an Open File Handle already listed?
        if (old_handle == handle) return 1;
    }

    if ((closed = getClosedIdamPluginFile(idamfiles, filename)) >= 0) {        // Does a Closed File Handle entry exist?
        idamfiles->files[closed].status = 1;                    // If so then reopen and update the record
        idamfiles->files[closed].handlePtr = handle;
        idamfiles->files[closed].handleInt = -1;
        gettimeofday(&idamfiles->files[closed].file_open, NULL);
        return 0;
    }

// New File (Close Stalest File if at maximum open handle limit)

    if (idamfiles->count >= MAXOPENPLUGINFILEDESC) purgeStalestIdamPluginFile(idamfiles);

    if (idamfiles->count == idamfiles->mcount || idamfiles->mcount == 0) {
        idamfiles->files = (IDAMPLUGINFILE*)realloc((void*)idamfiles->files,
                                                    (idamfiles->count + IDAMPLUGINFILEALLOC) *
                                                    sizeof(IDAMPLUGINFILE));
        idamfiles->mcount += IDAMPLUGINFILEALLOC;
    }
    idamfiles->files[idamfiles->count].status = 1;
    idamfiles->files[idamfiles->count].handlePtr = handle;
    idamfiles->files[idamfiles->count].handleInt = -1;

    strcpy(idamfiles->files[idamfiles->count].filename, filename);
    gettimeofday(&idamfiles->files[idamfiles->count].file_open, NULL);

    (idamfiles->count)++;    // Count of Open File handles
    return 0;
}

int addIdamPluginFileLong(IDAMPLUGINFILELIST* idamfiles, const char* filename, long handle)
{
    long old_handle;
    int closed;

    if ((old_handle = getOpenIdamPluginFileLong(idamfiles, filename)) >= 0) {
        // Is an Open File Handle already listed?
        if (old_handle == handle) return 1;
    }

    if ((closed = getClosedIdamPluginFile(idamfiles, filename)) >= 0) {        // Does a Closed File Handle entry exist?
        idamfiles->files[closed].status = 1;                    // If so then reopen and update the record
        idamfiles->files[closed].handleInt = handle;
        idamfiles->files[closed].handlePtr = NULL;
        gettimeofday(&idamfiles->files[closed].file_open, NULL);
        return 0;
    }

// New File (Close Stalest File if at maximum open handle limit)

    if (idamfiles->count >= MAXOPENPLUGINFILEDESC) purgeStalestIdamPluginFile(idamfiles);

    if (idamfiles->count == idamfiles->mcount || idamfiles->mcount == 0) {
        idamfiles->files = (IDAMPLUGINFILE*)realloc((void*)idamfiles->files,
                                                    (idamfiles->count + IDAMPLUGINFILEALLOC) *
                                                    sizeof(IDAMPLUGINFILE));
        idamfiles->mcount += IDAMPLUGINFILEALLOC;
    }
    idamfiles->files[idamfiles->count].status = 1;
    idamfiles->files[idamfiles->count].handleInt = handle;
    idamfiles->files[idamfiles->count].handlePtr = NULL;

    strcpy(idamfiles->files[idamfiles->count].filename, filename);
    gettimeofday(&idamfiles->files[idamfiles->count].file_open, NULL);

    (idamfiles->count)++;    // Count of Open File handles
    return 0;
}

// Search for an Open File in the List
// Returns an opaque pointer to the handle if found, NULL othewise.

void* getOpenIdamPluginFilePtr(IDAMPLUGINFILELIST* idamfiles, const char* filename)
{
    int i;
    UDA_LOG(UDA_LOG_DEBUG, "Open File Count %d\n", idamfiles->count);
    for (i = 0; i < idamfiles->count; i++) {
        UDA_LOG(UDA_LOG_DEBUG, "Status %d, Name %s [%s]\n",
                  idamfiles->files[i].status, idamfiles->files[i].filename, filename);

        if (idamfiles->files[i].status == 1) {
            if (STR_IEQUALS(filename, idamfiles->files[i].filename)) {
                gettimeofday(&idamfiles->files[i].file_open, NULL);        // Refresh Time of Last use
                return (idamfiles->files[i].handlePtr);
            }
        }
    }
    return NULL;    // Not Found => Not open
}

long getOpenIdamPluginFileLong(IDAMPLUGINFILELIST* idamfiles, const char* filename)
{
    int i;
    UDA_LOG(UDA_LOG_DEBUG, "Open File Count %d\n", idamfiles->count);
    for (i = 0; i < idamfiles->count; i++) {
        UDA_LOG(UDA_LOG_DEBUG, "Status %d, Name %s [%s]\n",
                  idamfiles->files[i].status, idamfiles->files[i].filename, filename);

        if (idamfiles->files[i].status == 1) {
            if (STR_IEQUALS(filename, idamfiles->files[i].filename)) {
                gettimeofday(&idamfiles->files[i].file_open, NULL);        // Refresh Time of Last use
                return idamfiles->files[i].handleInt;
            }
        }
    }
    return -1;    // Not Found => Not open
}

// Search for a Closed File in the List

int getClosedIdamPluginFile(IDAMPLUGINFILELIST* idamfiles, const char* filename)
{
    int i;
    for (i = 0; i < idamfiles->count; i++) {
        if (idamfiles->files[i].status == 0) {    // Only check Close handle records
            if (STR_IEQUALS(filename, idamfiles->files[i].filename)) return (i);
        }
    }
    return -1;    // No Closed File Found
}


// Close a specific file (Ignoring returned values)

void closeIdamPluginFile(IDAMPLUGINFILELIST* idamfiles, const char* filename)
{
    int i;
    for (i = 0; i < idamfiles->count; i++) {
        if (idamfiles->files[i].status == 1) {    // Only check Open handle records
            if (STR_IEQUALS(filename, idamfiles->files[i].filename)) {
                if (idamfiles->files[i].handlePtr != NULL) {
                    void (* close)(void*);
                    close = idamfiles->close;
                    if (close != NULL) {
                        close(idamfiles->files[i].handlePtr);
                    }
                } else {
                    void (* close)(long);
                    close = idamfiles->close;
                    if (close != NULL) {
                        close(idamfiles->files[i].handleInt);
                    }
                }
                idamfiles->files[i].status = 0;
                break;
            }
        }
    }
}

// Close all files and re-initialise the list

void closeIdamPluginFiles(IDAMPLUGINFILELIST* idamfiles)
{
    int i;
    for (i = 0; i < idamfiles->count; i++) {
        closeIdamPluginFile(idamfiles, idamfiles->files[i].filename);
    }
    free((void*)idamfiles->files);
    initIdamPluginFileList(idamfiles);
    return;
}

void purgeStalestIdamPluginFile(IDAMPLUGINFILELIST* idamfiles)
{
    int i, stalest = -1;
    struct timeval oldest;
    gettimeofday(&oldest, NULL);                // Start comparison with Present time

    for (i = 0; i < idamfiles->count; i++) {
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
        for (i = 0; i < idamfiles->count; i++) {
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
            if (idamfiles->files[stalest].handlePtr != NULL) {
                void (* close)(void*);
                close = idamfiles->close;
                close(idamfiles->files[stalest].handlePtr);
            } else {
                void (* close)(long);
                close = idamfiles->close;
                close(idamfiles->files[stalest].handleInt);
            }
        }
        for (i = stalest; i < idamfiles->count - 1; i++)
            idamfiles->files[i] = idamfiles->files[i + 1];    // Overwrite Stale Position
        idamfiles->files[idamfiles->count - 1].status = 0;
        idamfiles->files[idamfiles->count - 1].filename[0] = '\0';
        idamfiles->count = idamfiles->count - 1;
    }

    return;
}

int findIdamPluginFileByName(IDAMPLUGINFILELIST* idamfiles, const char* filename)
{
    int i;
    if (!filename || filename[0] == '\0' || idamfiles->count == 0) {
        return -1;
    }
    for (i = 0; i < idamfiles->count; i++) {
        if (STR_EQUALS(idamfiles->files[i].filename, filename)) {
            return i;
        }
    }
    return -1;
}

int findIdamPluginFileByLong(IDAMPLUGINFILELIST* idamfiles, long handle)
{
    int i;
    if (idamfiles->count == 0) {
        return -1;
    }
    for (i = 0; i < idamfiles->count; i++) {
        if (idamfiles->files[i].handleInt == handle) {
            return i;
        }
    }
    return -1;
}

void setIdamPluginFileClosed(IDAMPLUGINFILELIST* idamfiles, int record)
{
    idamfiles->files[record].status = 0;
    idamfiles->files[record].filename[0] = '\0';
    idamfiles->files[record].handleInt = 0;
    idamfiles->files[record].handlePtr = NULL;
}
