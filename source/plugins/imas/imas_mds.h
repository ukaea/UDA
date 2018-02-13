#ifndef UDA_PLUGINS_IMAS_PLUGIN_IMAS_MDS_H
#define UDA_PLUGINS_IMAS_PLUGIN_IMAS_MDS_H

#include <plugins/pluginStructs.h>
#include <stdbool.h>

#ifdef __cplusplus
extern "C" {
#endif

#define THISPLUGIN_VERSION                  1
#define THISPLUGIN_MAX_INTERFACE_VERSION    1        // Interface versions higher than this will not be understood!
#define THISPLUGIN_DEFAULT_METHOD           "help"

#define FILEISNEW       1
#define FILEISOPEN      2
#define FILEISCLOSED    3

#define CLOSEST_SAMPLE  1
#define PREVIOUS_SAMPLE 2
#define INTERPOLATION   3

#define TIMEBASEPATHLENGTH      256

#define ERROR_RETURN_VALUE      1
#define OK_RETURN_VALUE         0

extern int imas_mds(IDAM_PLUGIN_INTERFACE* idam_plugin_interface);

char* imas_reset_errmsg();

int mdsGetDimension(int expIdx, char *cpoPath, char *path, int *numDims, int *dim1, int *dim2, int *dim3, int *dim4, int *dim5, int *dim6, int *dim7);

typedef struct PluginArgs {
    bool isCPOPath;
    const char* CPOPath;
    bool isPath;
    const char* path;
    bool isTypeName;
    const char* typeName;
    bool isClientIdx;
    int clientIdx;
    bool isClientObjectId;
    int clientObjectId;
    bool isRank;
    int rank;
    bool isIndex;
    int index;
    bool isCount;
    int count;
    bool isShapeString;
    const char* shapeString;
    bool isDataString;
    const char* dataString;
    bool isFileName;
    const char* filename;
    bool isShotNumber;
    int shotNumber;
    bool isRunNumber;
    int runNumber;
    bool isRefShotNumber;
    int refShotNumber;
    bool isRefRunNumber;
    int refRunNumber;
    bool isTimedArg;
    int isTimed;
    bool isInterpolMode;
    int interpolMode;
    bool isSignal;
    const char* signal;
    bool isSource;
    const char* source;
    bool isFormat;
    const char* format;
    bool isOwner;
    const char* owner;
    bool isServer;
    const char* server;
    bool isSetLevel;
    int setLevel;
    bool isCommand;
    const char* command;
    bool isIPAddress;
    const char* IPAddress;
    bool isTimes;
    const char* timesString;
    bool isPutDataSlice;
    bool isReplaceLastDataSlice;
    bool isGetDataSlice;
    bool isGetDimension;
    bool isCreateFromModel;
    bool isFlush;
    bool isDiscard;
    bool isGetLevel;
    bool isFlushCPO;
    bool isDisable;
    bool isEnable;
    bool isBeginIDSSlice;
    bool isEndIDSSlice;
    bool isReplaceIDSSlice;
    bool isBeginIDS;
    bool isEndIDS;
    bool isBeginIDSTimed;
    bool isEndIDSTimed;
    bool isBeginIDSNonTimed;
    bool isEndIDSNonTimed;
    char quote;
    char delimiter;
    bool isPutData;
    bool isImasIdsVersion;
    const char* imasIdsVersion;
    bool isImasIdsDevice;
    const char* imasIdsDevice;

} PLUGIN_ARGS;

int imas_mds_putDataSlice(int idx, char* cpoPath, char* path, char* timeBasePath, int type, int nDims,
                          int* dims, void* data, double time);

int imas_mds_replaceLastDataSlice(int idx, char* cpoPath, char* path, int type, int nDims, int* dims,
                                  void* data);

int imas_mds_putData(int idx, char* cpoPath, char* path, int type, int nDims, int* dims, int isTimed,
                     void* data, double time);

int imas_mds_putDataX(int idx, char* cpoPath, char* path, int type, int nDims, int* dims, int operation,
                      void* data, double time);

int imas_mds_getData(int idx, char* cpoPath, char* path, int type, int nDims, int* dims, void** dataOut);

int imas_mds_getDataSlices(int idx, char* cpoPath, char* path, int type, int rank, int* shape, void** data,
                           double time, double* retTime, int interpolMode);

void* imas_mds_putDataSliceInObject(void* obj, char* path, int index, int type, int nDims, int* dims,
                                    void* data);

int imas_mds_getDataSliceInObject(void* obj, char* path, int index, int type, int nDims, int* dims,
                                  void** data);

void putTimeBasePath(char* timeBasePath);

char* getTimeBasePath();

#ifdef __cplusplus
}
#endif

#endif // UDA_PLUGINS_IMAS_PLUGIN_IMAS_MDS_H
