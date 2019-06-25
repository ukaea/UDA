#ifndef UDA_PLUGINS_IMAS_PLUGIN_IMAS_HDF5_H
#define UDA_PLUGINS_IMAS_PLUGIN_IMAS_HDF5_H

#include <plugins/udaPluginFiles.h>
#include <hdf5.h>

#ifdef __cplusplus
extern "C" {
#endif

UDA_PLUGIN_FILE_LIST* getImasPluginFileList();

char* getImasErrorMsg();

//extern void setSliceIdx(int idx1, int idx2);
//extern void setSliceTime(double time1, double time2);

void initHdf5File();

int findHdf5Idx(int idam_id);

int findFirstHdf5Idx();

int checkHdf5Idx(int idx);

hid_t createGroup(hid_t rootId, const char* pathName);

void splitVarGrp(const char* cpoPath, const char* path, char** groupName, char** dataName);

char* getHdf5FileName(const char* filename, int shot, int run);

char* getHdf5ModelName(const char* filename);

void releaseHdf5File(int idx);

int imas_hdf5_IdsModelCreate(const char* filename, int version);

int imas_hdf5_EuitmCreate(const char* name, int shot, int run, int refShot, int refRun, int* retIdx);

int imas_hdf5_IMASCreate(const char* name, int shot, int run, int refShot, int refRun, int* retIdx);

int imas_hdf5_EuitmOpen(const char* name, int shot, int run, int* retIdx);

int imas_hdf5_EuitmClose(int idx, const char* name, int shot, int run);

int imas_hdf5_putData(int idx, const char* cpoPath, const char* path, int type, int nDims, int* dims, int isTimed,
                 void* data);

int imas_hdf5_putDataX(int idx, const char* cpoPath, const char* path, int type, int nDims, int* dims, int dataOperation,
                  void* data, double time);

int imas_hdf5_putDataSlice(int idx, const char* cpoPath, const char* path, int type, int nDims, int* dims, void* data,
                      double time);

int imas_hdf5_replaceLastDataSlice(int idx, const char* cpoPath, const char* path, int type, int nDims, int* dims,
                              void* data);

int imas_hdf5_getData(int idx, const char* cpoPath, const char* path, int type, int nDims, int* dims, char** data);

int imas_hdf5_getDataSlices(int idx, const char* cpoPath, const char* path, int type, int nDims, int* dims, int dataIdx,
                       int numSlices, char** data);

int imas_hdf5_GetDimension(int expIdx, const char* cpoPath, const char* path, int* numDims, int* dim1, int* dim2,
                          int* dim3, int* dim4, int* dim5, int* dim6, int* dim7);

int imas_hdf5_DeleteData(int expIdx, const char* cpoPath, const char* path);

// *  Arrays of structures  *

#define NON_TIMED   0
#define TIMED       1
#define TIMED_CLEAR 2

#define INT         2
#define FLOAT       3
#define DOUBLE      4
#define DIMENSION   -1    // if we need to read the dimensions only

hid_t openGroup(hid_t root, const char* relPath, int create_flag, int clear_flag);

int imas_hdf5_putDataSliceInObject(void* obj, const char* path, int index, int type, int nDims, int* dims, void* data);

int imas_hdf5_getDataSliceFromObject(void* obj, const char* path, int index, int type, int nDims, int* dims,
                                void** data);

void* imas_hdf5_BeginObject(int expIdx, void* obj, int index, const char* relPath, int isTimed);

int imas_hdf5_GetObject(int expIdx, const char* hdf5Path, const char* cpoPath, void** obj, int isTimed);

int imas_hdf5_GetObjectSlice(int expIdx, const char* hdf5Path, const char* cpoPath, double time, void** obj);

int imas_hdf5_GetObjectFromObject(void* obj, const char* hdf5Path, int idx, void** dataObj);

void imas_hdf5_ReleaseObject(void* obj);

//void sha1Block(unsigned char *block, size_t blockSize, unsigned char *md)

typedef struct obj_t {
    hid_t handle;   // HDF5 handle of the group corresponding to the object
    int dim;        // number of elements contained in the object (not used for put)
    int timeIdx;    // for timed objects, index of the time to be read (not used for put)
    struct obj_t* nextObj; // a list of other objects contained inside this object (for cleaning purpose)
} obj_t;

#ifdef __cplusplus
}
#endif

#endif // UDA_PLUGINS_IMAS_PLUGIN_IMAS_HDF5_H
