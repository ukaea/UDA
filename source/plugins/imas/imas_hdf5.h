#ifndef IDAM_PLUGINS_IMAS_HDF5_PLUGIN_H
#define IDAM_PLUGINS_IMAS_HDF5_PLUGIN_H

#include <plugins/udaPluginFiles.h>
#include <hdf5.h>

#ifdef __cplusplus
extern "C" {
#endif

IDAMPLUGINFILELIST* getImasPluginFileList();

void setSliceIdx(int idx1, int idx2);

void setSliceTime(double time1, double time2);

char* getImasErrorMsg();

//extern void setSliceIdx(int idx1, int idx2);
//extern void setSliceTime(double time1, double time2);

void initHdf5File();

int findHdf5Idx(int idam_id);

int findFirstHdf5Idx();

int checkHdf5Idx(int idx);

hid_t createGroup(hid_t rootId, char* pathName);

void splitVarGrp(char* cpoPath, char* path, char** groupName, char** dataName);

char* getHdf5FileName(char* filename, int shot, int run);

char* getHdf5ModelName(char* filename);

void releaseHdf5File(int idx);

int imas_hdf5IdsModelCreate(char* filename, int version);

int imas_hdf5EuitmCreate(char* name, int shot, int run, int refShot, int refRun, int* retIdx);

int imas_hdf5IMASCreate(char* name, int shot, int run, int refShot, int refRun, int* retIdx);

int imas_hdf5EuitmOpen(char* name, int shot, int run, int* retIdx);

int imas_hdf5EuitmClose(int idx, char* name, int shot, int run);

int imas_putData(int idx, char* cpoPath, char* path, int type, int nDims, int* dims, int isTimed,
                 void* data);

int imas_putDataX(int idx, char* cpoPath, char* path, int type, int nDims, int* dims, int dataOperation,
                  void* data, double time);

int imas_putDataSlice(int idx, char* cpoPath, char* path, int type, int nDims, int* dims, void* data,
                      double time);

int imas_replaceLastDataSlice(int idx, char* cpoPath, char* path, int type, int nDims, int* dims,
                              void* data);

int imas_getData(int idx, char* cpoPath, char* path, int type, int nDims, int* dims, char** data);

int imas_getDataSlices(int idx, char* cpoPath, char* path, int type, int nDims, int* dims, int dataIdx,
                       int numSlices, char** data);

int imas_hdf5GetDimension(int expIdx, char* cpoPath, char* path, int* numDims, int* dim1, int* dim2,
                          int* dim3, int* dim4, int* dim5, int* dim6, int* dim7);

int imas_hdf5DeleteData(int expIdx, char* cpoPath, char* path);

// *  Arrays of structures  *

#define NON_TIMED   0
#define TIMED       1
#define TIMED_CLEAR 2

#define INT         2
#define FLOAT       3
#define DOUBLE      4
#define DIMENSION   -1    // if we need to read the dimensions only

hid_t openGroup(hid_t root, const char* relPath, int create_flag, int clear_flag);

int imas_putDataSliceInObject(void* obj, char* path, int index, int type, int nDims, int* dims, void* data);

int imas_getDataSliceFromObject(void* obj, char* path, int index, int type, int nDims, int* dims,
                                void** data);

void* imas_hdf5BeginObject(int expIdx, void* obj, int index, const char* relPath, int isTimed);

int imas_hdf5GetObject(int expIdx, char* hdf5Path, char* cpoPath, void** obj, int isTimed);

int imas_hdf5GetObjectSlice(int expIdx, char* hdf5Path, char* cpoPath, double time, void** obj);

int imas_hdf5GetObjectFromObject(void* obj, char* hdf5Path, int idx, void** dataObj);

void imas_hdf5ReleaseObject(void* obj);

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

#endif // IDAM_PLUGINS_IMAS_HDF5_PLUGIN_H
