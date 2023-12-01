#ifndef UDA_PLUGINS_MANAGEPLUGINFILES_H
#define UDA_PLUGINS_MANAGEPLUGINFILES_H

#include <plugins/udaPluginFiles.h>
#include "export.h"

#ifdef __cplusplus
extern "C" {
#endif

LIBRARY_API void initIdamPluginFileList(UDA_PLUGIN_FILE_LIST* idamfiles);
LIBRARY_API void registerIdamPluginFileClose(UDA_PLUGIN_FILE_LIST* idamfiles, void* fptr);
LIBRARY_API int addIdamPluginFilePtr(UDA_PLUGIN_FILE_LIST* uda_files, const char* filename, void* handle);
LIBRARY_API int addIdamPluginFileLong(UDA_PLUGIN_FILE_LIST* uda_files, const char* filename, long handle);
LIBRARY_API void* getOpenIdamPluginFilePtr(UDA_PLUGIN_FILE_LIST* uda_files, const char* filename);
LIBRARY_API long getOpenIdamPluginFileLong(UDA_PLUGIN_FILE_LIST* uda_files, const char* filename);
LIBRARY_API int getClosedIdamPluginFile(UDA_PLUGIN_FILE_LIST* uda_files, const char* filename);
LIBRARY_API void closeIdamPluginFile(UDA_PLUGIN_FILE_LIST* idamfiles, const char* filename);
LIBRARY_API void closeIdamPluginFiles(UDA_PLUGIN_FILE_LIST* uda_files);
LIBRARY_API void purgeStalestIdamPluginFile(UDA_PLUGIN_FILE_LIST* uda_files);
LIBRARY_API int findIdamPluginFileByName(UDA_PLUGIN_FILE_LIST* uda_files, const char* filename);
LIBRARY_API int findIdamPluginFileByLong(UDA_PLUGIN_FILE_LIST* uda_files, long handle);
LIBRARY_API void setIdamPluginFileClosed(UDA_PLUGIN_FILE_LIST* uda_files, int record);

#ifdef __cplusplus
}
#endif

#endif // UDA_PLUGINS_MANAGEPLUGINFILES_H
