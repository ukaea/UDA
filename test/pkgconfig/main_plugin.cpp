#include <plugins/managePluginFiles.h>

int main()
{
    UDA_PLUGIN_FILE_LIST files;
    initIdamPluginFileList(&files);
    return files.count;
}
