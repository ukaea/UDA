// Open File State Management
//
//----------------------------------------------------------------------------------

#include "managePluginFiles.h"

#include <cstdlib>
#if defined(__GNUC__)
#  include <strings.h>
#endif

#include <clientserver/stringUtils.h>
#include <logging/logging.h>

#define UDA_PLUGIN_FILE_ALLOC 10
#define MAX_OPEN_PLUGIN_FILE_DESC 50

// Initialise the File List and allocate heap for the list

void initIdamPluginFileList(UDA_PLUGIN_FILE_LIST* uda_files)
{
    uda_files->count = 0;
    uda_files->mcount = 0;
    uda_files->files = nullptr;
    uda_files->close = nullptr;
}

// Add the file closing function pointer (same for all entries)

void registerIdamPluginFileClose(UDA_PLUGIN_FILE_LIST* uda_files, void* fptr)
{
    uda_files->close = fptr;
}

// Add a New Open File to the List or re-Open an existing record
// returns 1 if the handle already exists, 0 if not

int addIdamPluginFilePtr(UDA_PLUGIN_FILE_LIST* uda_files, const char* filename, void* handle)
{
    void* old_handle = nullptr;
    int closed;

    if ((old_handle = getOpenIdamPluginFilePtr(uda_files, filename)) != nullptr) {
        // Is an Open File Handle already listed?
        if (old_handle == handle) {
            return 1;
        }
    }

    if ((closed = getClosedIdamPluginFile(uda_files, filename)) >= 0) { // Does a Closed File Handle entry exist?
        uda_files->files[closed].status = 1;                            // If so then reopen and update the record
        uda_files->files[closed].handlePtr = handle;
        uda_files->files[closed].handleInt = -1;
        gettimeofday(&uda_files->files[closed].file_open, nullptr);
        return 0;
    }

    // New File (Close Stalest File if at maximum open handle limit)

    if (uda_files->count >= MAX_OPEN_PLUGIN_FILE_DESC) {
        purgeStalestIdamPluginFile(uda_files);
    }

    if (uda_files->count == uda_files->mcount || uda_files->mcount == 0) {
        uda_files->files = (UDA_PLUGIN_FILE*)realloc(
            (void*)uda_files->files, (uda_files->count + UDA_PLUGIN_FILE_ALLOC) * sizeof(UDA_PLUGIN_FILE));
        uda_files->mcount += UDA_PLUGIN_FILE_ALLOC;
    }
    uda_files->files[uda_files->count].status = 1;
    uda_files->files[uda_files->count].handlePtr = handle;
    uda_files->files[uda_files->count].handleInt = -1;

    strcpy(uda_files->files[uda_files->count].filename, filename);
    gettimeofday(&uda_files->files[uda_files->count].file_open, nullptr);

    (uda_files->count)++; // Count of Open File handles
    return 0;
}

int addIdamPluginFileLong(UDA_PLUGIN_FILE_LIST* uda_files, const char* filename, long handle)
{
    long old_handle;
    int closed;

    if ((old_handle = getOpenIdamPluginFileLong(uda_files, filename)) >= 0) {
        // Is an Open File Handle already listed?
        if (old_handle == handle) {
            return 1;
        }
    }

    if ((closed = getClosedIdamPluginFile(uda_files, filename)) >= 0) { // Does a Closed File Handle entry exist?
        uda_files->files[closed].status = 1;                            // If so then reopen and update the record
        uda_files->files[closed].handleInt = handle;
        uda_files->files[closed].handlePtr = nullptr;
        gettimeofday(&uda_files->files[closed].file_open, nullptr);
        return 0;
    }

    // New File (Close Stalest File if at maximum open handle limit)

    if (uda_files->count >= MAX_OPEN_PLUGIN_FILE_DESC) {
        purgeStalestIdamPluginFile(uda_files);
    }

    if (uda_files->count == uda_files->mcount || uda_files->mcount == 0) {
        uda_files->files = (UDA_PLUGIN_FILE*)realloc(
            (void*)uda_files->files, (uda_files->count + UDA_PLUGIN_FILE_ALLOC) * sizeof(UDA_PLUGIN_FILE));
        uda_files->mcount += UDA_PLUGIN_FILE_ALLOC;
    }
    uda_files->files[uda_files->count].status = 1;
    uda_files->files[uda_files->count].handleInt = handle;
    uda_files->files[uda_files->count].handlePtr = nullptr;

    strcpy(uda_files->files[uda_files->count].filename, filename);
    gettimeofday(&uda_files->files[uda_files->count].file_open, nullptr);

    (uda_files->count)++; // Count of Open File handles
    return 0;
}

// Search for an Open File in the List
// Returns an opaque pointer to the handle if found, nullptr othewise.

void* getOpenIdamPluginFilePtr(UDA_PLUGIN_FILE_LIST* uda_files, const char* filename)
{
    UDA_LOG(UDA_LOG_DEBUG, "Open File Count %d\n", uda_files->count);
    for (int i = 0; i < uda_files->count; i++) {
        UDA_LOG(UDA_LOG_DEBUG, "Status %d, Name %s [%s]\n", uda_files->files[i].status, uda_files->files[i].filename,
                filename);

        if (uda_files->files[i].status == 1) {
            if (STR_IEQUALS(filename, uda_files->files[i].filename)) {
                gettimeofday(&uda_files->files[i].file_open, nullptr); // Refresh Time of Last use
                return uda_files->files[i].handlePtr;
            }
        }
    }
    return nullptr; // Not Found => Not open
}

long getOpenIdamPluginFileLong(UDA_PLUGIN_FILE_LIST* uda_files, const char* filename)
{
    UDA_LOG(UDA_LOG_DEBUG, "Open File Count %d\n", uda_files->count);
    for (int i = 0; i < uda_files->count; i++) {
        UDA_LOG(UDA_LOG_DEBUG, "Status %d, Name %s [%s]\n", uda_files->files[i].status, uda_files->files[i].filename,
                filename);

        if (uda_files->files[i].status == 1) {
            if (STR_IEQUALS(filename, uda_files->files[i].filename)) {
                gettimeofday(&uda_files->files[i].file_open, nullptr); // Refresh Time of Last use
                return uda_files->files[i].handleInt;
            }
        }
    }
    return -1; // Not Found => Not open
}

// Search for a Closed File in the List

int getClosedIdamPluginFile(UDA_PLUGIN_FILE_LIST* uda_files, const char* filename)
{
    for (int i = 0; i < uda_files->count; i++) {
        if (uda_files->files[i].status == 0) { // Only check Close handle records
            if (STR_IEQUALS(filename, uda_files->files[i].filename)) {
                return i;
            }
        }
    }
    return -1; // No Closed File Found
}

// Close a specific file (Ignoring returned values)

void closeIdamPluginFile(UDA_PLUGIN_FILE_LIST* uda_files, const char* filename)
{
    for (int i = 0; i < uda_files->count; i++) {
        if (uda_files->files[i].status == 1) { // Only check Open handle records
            if (STR_IEQUALS(filename, uda_files->files[i].filename)) {
                if (uda_files->files[i].handlePtr != nullptr) {
                    typedef void (*close_t)(void*);
                    auto close = (close_t)uda_files->close;
                    if (close != nullptr) {
                        close(uda_files->files[i].handlePtr);
                    }
                } else {
                    typedef void (*close_t)(long);
                    auto close = (close_t)uda_files->close;
                    if (close != nullptr) {
                        close(uda_files->files[i].handleInt);
                    }
                }
                uda_files->files[i].status = 0;
                break;
            }
        }
    }
}

// Close all files and re-initialise the list

void closeIdamPluginFiles(UDA_PLUGIN_FILE_LIST* uda_files)
{
    for (int i = 0; i < uda_files->count; i++) {
        closeIdamPluginFile(uda_files, uda_files->files[i].filename);
    }
    free(uda_files->files);
    initIdamPluginFileList(uda_files);
}

void purgeStalestIdamPluginFile(UDA_PLUGIN_FILE_LIST* uda_files)
{
    int stalest = -1;
    struct timeval oldest = {};
    gettimeofday(&oldest, nullptr); // Start comparison with Present time

    for (int i = 0; i < uda_files->count; i++) {
        if (uda_files->files[i].status == 0) { // Drop Closed Files First
            if (uda_files->files[i].file_open.tv_sec < oldest.tv_sec ||
                (uda_files->files[i].file_open.tv_sec == oldest.tv_sec &&
                 uda_files->files[i].file_open.tv_usec <= oldest.tv_usec)) {
                oldest = uda_files->files[i].file_open;
                stalest = i;
            }
        }
    }

    if (stalest < 0) { // No Closed files so choose an Open File
        stalest = 0;
        for (int i = 0; i < uda_files->count; i++) {
            if (uda_files->files[i].file_open.tv_sec < oldest.tv_sec ||
                (uda_files->files[i].file_open.tv_sec == oldest.tv_sec &&
                 uda_files->files[i].file_open.tv_usec <= oldest.tv_usec)) {
                oldest = uda_files->files[i].file_open;
                stalest = i;
            }
        }
    }

    if (stalest >= 0) {
        if (uda_files->files[stalest].status == 1) { // Close the Stale File
            if (uda_files->files[stalest].handlePtr != nullptr) {
                typedef void (*close_t)(void*);
                auto close = (close_t)uda_files->close;
                close(uda_files->files[stalest].handlePtr);
            } else {
                typedef void (*close_t)(long);
                auto close = (close_t)uda_files->close;
                close(uda_files->files[stalest].handleInt);
            }
        }
        for (int i = stalest; i < uda_files->count - 1; i++) {
            uda_files->files[i] = uda_files->files[i + 1]; // Overwrite Stale Position
        }
        uda_files->files[uda_files->count - 1].status = 0;
        uda_files->files[uda_files->count - 1].filename[0] = '\0';
        uda_files->count = uda_files->count - 1;
    }
}

int findIdamPluginFileByName(UDA_PLUGIN_FILE_LIST* uda_files, const char* filename)
{
    if (!filename || filename[0] == '\0' || uda_files->count == 0) {
        return -1;
    }
    for (int i = 0; i < uda_files->count; i++) {
        if (STR_EQUALS(uda_files->files[i].filename, filename)) {
            return i;
        }
    }
    return -1;
}

int findIdamPluginFileByLong(UDA_PLUGIN_FILE_LIST* uda_files, long handle)
{
    if (uda_files->count == 0) {
        return -1;
    }
    for (int i = 0; i < uda_files->count; i++) {
        if (uda_files->files[i].handleInt == handle) {
            return i;
        }
    }
    return -1;
}

void setIdamPluginFileClosed(UDA_PLUGIN_FILE_LIST* uda_files, int record)
{
    uda_files->files[record].status = 0;
    uda_files->files[record].filename[0] = '\0';
    uda_files->files[record].handleInt = 0;
    uda_files->files[record].handlePtr = nullptr;
}
