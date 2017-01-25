#ifndef IDAM_SERVER_MANAGEPLUGINFILES_H
#define IDAM_SERVER_MANAGEPLUGINFILES_H

#include <include/idampluginfiles.h>

void initIdamPluginFileList(IDAMPLUGINFILELIST * idamfiles);

void registerIdamPluginFileClose(IDAMPLUGINFILELIST * idamfiles, void * fptr);

int addIdamPluginFilePtr(IDAMPLUGINFILELIST * idamfiles, char * filename, void * handle);

int addIdamPluginFileInt(IDAMPLUGINFILELIST * idamfiles, char * filename, int handle);

void * getOpenIdamPluginFilePtr(IDAMPLUGINFILELIST * idamfiles, char * filename);

int getOpenIdamPluginFileInt(IDAMPLUGINFILELIST * idamfiles, char * filename);

int getClosedIdamPluginFile(IDAMPLUGINFILELIST * idamfiles, char * filename);

void closeIdamPluginFile(IDAMPLUGINFILELIST * idamfiles, char * filename);

void closeIdamPluginFiles(IDAMPLUGINFILELIST * idamfiles);

void purgeStalestIdamPluginFile(IDAMPLUGINFILELIST * idamfiles);

int findIdamPluginFileByName(IDAMPLUGINFILELIST * idamfiles, char * filename);

int findIdamPluginFileByInt(IDAMPLUGINFILELIST * idamfiles, int handleInt);

void setIdamPluginFileClosed(IDAMPLUGINFILELIST * idamfiles, int record);

#endif //IDAM_SERVER_MANAGEPLUGINFILES_H
