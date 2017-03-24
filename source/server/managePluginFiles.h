#ifndef IDAM_SERVER_MANAGEPLUGINFILES_H
#define IDAM_SERVER_MANAGEPLUGINFILES_H

#include <plugins/udaPluginFiles.h>

void initIdamPluginFileList(IDAMPLUGINFILELIST * idamfiles);

void registerIdamPluginFileClose(IDAMPLUGINFILELIST * idamfiles, void * fptr);

int addIdamPluginFilePtr(IDAMPLUGINFILELIST * idamfiles, char * filename, void * handle);

int addIdamPluginFileLong(IDAMPLUGINFILELIST * idamfiles, char * filename, long handle);

void * getOpenIdamPluginFilePtr(IDAMPLUGINFILELIST * idamfiles, char * filename);

long getOpenIdamPluginFileLong(IDAMPLUGINFILELIST * idamfiles, char * filename);

int getClosedIdamPluginFile(IDAMPLUGINFILELIST * idamfiles, char * filename);

void closeIdamPluginFile(IDAMPLUGINFILELIST * idamfiles, char * filename);

void closeIdamPluginFiles(IDAMPLUGINFILELIST * idamfiles);

void purgeStalestIdamPluginFile(IDAMPLUGINFILELIST * idamfiles);

int findIdamPluginFileByName(IDAMPLUGINFILELIST * idamfiles, char * filename);

int findIdamPluginFileByLong(IDAMPLUGINFILELIST* idamfiles, long handle);

void setIdamPluginFileClosed(IDAMPLUGINFILELIST * idamfiles, int record);

#endif //IDAM_SERVER_MANAGEPLUGINFILES_H
