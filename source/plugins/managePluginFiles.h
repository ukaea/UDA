#ifndef IDAM_SERVER_MANAGEPLUGINFILES_H
#define IDAM_SERVER_MANAGEPLUGINFILES_H

#include <plugins/udaPluginFiles.h>

#ifdef __cplusplus
extern "C" {
#endif

void initIdamPluginFileList(IDAMPLUGINFILELIST* idamfiles);

void registerIdamPluginFileClose(IDAMPLUGINFILELIST* idamfiles, void* fptr);

int addIdamPluginFilePtr(IDAMPLUGINFILELIST* idamfiles, const char* filename, void* handle);

int addIdamPluginFileLong(IDAMPLUGINFILELIST* idamfiles, const char* filename, long handle);

void* getOpenIdamPluginFilePtr(IDAMPLUGINFILELIST* idamfiles, const char* filename);

long getOpenIdamPluginFileLong(IDAMPLUGINFILELIST* idamfiles, const char* filename);

int getClosedIdamPluginFile(IDAMPLUGINFILELIST* idamfiles, const char* filename);

void closeIdamPluginFile(IDAMPLUGINFILELIST* idamfiles, const char* filename);

void closeIdamPluginFiles(IDAMPLUGINFILELIST* idamfiles);

void purgeStalestIdamPluginFile(IDAMPLUGINFILELIST* idamfiles);

int findIdamPluginFileByName(IDAMPLUGINFILELIST* idamfiles, const char* filename);

int findIdamPluginFileByLong(IDAMPLUGINFILELIST* idamfiles, long handle);

void setIdamPluginFileClosed(IDAMPLUGINFILELIST* idamfiles, int record);

#ifdef __cplusplus
}
#endif

#endif //IDAM_SERVER_MANAGEPLUGINFILES_H
