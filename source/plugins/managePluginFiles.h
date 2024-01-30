#ifndef UDA_PLUGINS_MANAGEPLUGINFILES_H
#define UDA_PLUGINS_MANAGEPLUGINFILES_H

#include <plugins/udaPluginFiles.h>

#ifdef __cplusplus
extern "C" {
#endif

void initIdamPluginFileList(UDA_PLUGIN_FILE_LIST* idamfiles);
void registerIdamPluginFileClose(UDA_PLUGIN_FILE_LIST* idamfiles, void* fptr);
int addIdamPluginFilePtr(UDA_PLUGIN_FILE_LIST* uda_files, const char* filename, void* handle);
int addIdamPluginFileLong(UDA_PLUGIN_FILE_LIST* uda_files, const char* filename, long handle);
void* getOpenIdamPluginFilePtr(UDA_PLUGIN_FILE_LIST* uda_files, const char* filename);
long getOpenIdamPluginFileLong(UDA_PLUGIN_FILE_LIST* uda_files, const char* filename);
int getClosedIdamPluginFile(UDA_PLUGIN_FILE_LIST* uda_files, const char* filename);
void closeIdamPluginFile(UDA_PLUGIN_FILE_LIST* idamfiles, const char* filename);
void closeIdamPluginFiles(UDA_PLUGIN_FILE_LIST* uda_files);
void purgeStalestIdamPluginFile(UDA_PLUGIN_FILE_LIST* uda_files);
int findIdamPluginFileByName(UDA_PLUGIN_FILE_LIST* uda_files, const char* filename);
int findIdamPluginFileByLong(UDA_PLUGIN_FILE_LIST* uda_files, long handle);
void setIdamPluginFileClosed(UDA_PLUGIN_FILE_LIST* uda_files, int record);

#ifdef __cplusplus
}
#endif

#endif // UDA_PLUGINS_MANAGEPLUGINFILES_H
