#ifndef _UAL_LOW_LEVEL_H
#define _UAL_LOW_LEVEL_H

#define INTERPOLATION 3
#define CLOSEST_SAMPLE 1
#define PREVIOUS_SAMPLE 2
#define EMPTY_INT  -999999999
#define EMPTY_FLOAT -9.0E40
#define EMPTY_DOUBLE -9.0E40

typedef struct {
    double re;
    double im;
} UalComplex;

const char* getExpName(int expIdx);

//Low level function prototypes
int putString(int expIdx, char *cpoPath, char *path, char *data, int strlen);
int putInt(int expIdx, char *cpoPath, char *path, int data);
int putFloat(int expIdx, char *cpoPath, char *path, float data);
int putDouble(int expIdx, char *cpoPath, char *path, double data);
int putComplex(int expIdx, char *cpoPath, char *path, char *timeBasePath, UalComplex data);
int putVect1DString(int expIdx, char *cpoPath, char *path, char *timeBasePath, char **data, int dim, int isTimed);
int putVect1DInt(int expIdx, char *cpoPath, char *path, char *timeBasePath, int *data, int dim, int isTimed);
int putVect1DFloat(int expIdx, char *cpoPath, char *path, char *timeBasePath, float *data, int dim, int isTimed);
int putVect1DDouble(int expIdx, char *cpoPath, char *path, char *timeBasePath, double *data, int dim, int isTimed);
int putVect1DComplex(int expIdx, char *cpoPath, char *path, char *timeBasePath, UalComplex *data, int dim, int isTimed);
int putVect2DInt(int expIdx, char *cpoPath, char *path, char *timeBasePath, int *data, int dim1, int dim2, int isTimed);
int putVect2DFloat(int expIdx, char *cpoPath, char *path, char *timeBasePath, float *data, int dim1, int dim2, int isTimed);
int putVect2DDouble(int expIdx, char *cpoPath, char *path, char *timeBasePath, double *data, int dim1, int dim2, int isTimed);
int putVect3DInt(int expIdx, char *cpoPath, char *path, char *timeBasePath, int *data, int dim1, int dim2, int dim3, int isTimed);
int putVect3DFloat(int expIdx, char *cpoPath, char *path, char *timeBasePath, float *data, int dim1, int dim2, int dim3, int isTimed);
int putVect3DDouble(int expIdx, char *cpoPath, char *path, char *timeBasePath, double *data, int dim1, int dim2, int dim3, int isTimed);
int putVect4DInt(int expIdx, char *cpoPath, char *path, char *timeBasePath, int *data, int dim1, int dim2, int dim3, int dim4, int isTimed);
int putVect4DFloat(int expIdx, char *cpoPath, char *path, char *timeBasePath, float *data, int dim1, int dim2, int dim3, int dim4, int isTimed);
int putVect4DDouble(int expIdx, char *cpoPath, char *path, char *timeBasePath, double *data, int dim1, int dim2, int dim3, int dim4, int isTimed);
int putVect5DInt(int expIdx, char *cpoPath, char *path, char *timeBasePath, int *data, int dim1, int dim2, int dim3, int dim4, int dim5, int isTimed);
int putVect5DFloat(int expIdx, char *cpoPath, char *path, char *timeBasePath, float *data, int dim1, int dim2, int dim3, int dim4,int dim5,  int isTimed);
int putVect5DDouble(int expIdx, char *cpoPath, char *path, char *timeBasePath, double *data, int dim1, int dim2, int dim3, int dim4,int dim5,  int isTimed);
int putVect6DInt(int expIdx, char *cpoPath, char *path, char *timeBasePath, int *data, int dim1, int dim2, int dim3, int dim4, int dim5, int dim6, int isTimed);
int putVect6DFloat(int expIdx, char *cpoPath, char *path, char *timeBasePath, float *data, int dim1, int dim2, int dim3, int dim4,int dim5, int dim6, int isTimed);
int putVect6DDouble(int expIdx, char *cpoPath, char *path, char *timeBasePath, double *data, int dim1, int dim2, int dim3, int dim4,int dim5, int dim6, int isTimed);
int putVect7DInt(int expIdx, char *cpoPath, char *path, char *timeBasePath, int *data, int dim1, int dim2, int dim3, int dim4, int dim5, int dim6, int dim7, int isTimed);
int putVect7DFloat(int expIdx, char *cpoPath, char *path, char *timeBasePath, float *data, int dim1, int dim2, int dim3, int dim4,int dim5, int dim6, int dim7, int isTimed);
int putVect7DDouble(int expIdx, char *cpoPath, char *path, char *timeBasePath, double *data, int dim1, int dim2, int dim3, int dim4,int dim5, int dim6, int dim7, int isTimed);


int putIntSlice(int expIdx, char *cpoPath, char *path, char *timeBasePath, int data, double time);
int putFloatSlice(int expIdx, char *cpoPath, char *path, char *timeBasePath, float data, double time);
int putDoubleSlice(int expIdx, char *cpoPath, char *path, char *timeBasePath, double data, double time);
int putStringSlice(int expIdx, char *cpoPath, char *path, char *timeBasePath, char *data, double time);
int putVect1DIntSlice(int expIdx, char *cpoPath, char *path, char *timeBasePath, int *data, int dim, double time);
int putVect1DFloatSlice(int expIdx, char *cpoPath, char *path, char *timeBasePath, float *data, int dim, double time);
int putVect1DDoubleSlice(int expIdx, char *cpoPath, char *path, char *timeBasePath, double *data, int dim, double time);
int putVect2DIntSlice(int expIdx, char *cpoPath, char *path, char *timeBasePath, int *data, int dim1, int dim2, double time);
int putVect2DFloatSlice(int expIdx, char *cpoPath, char *path, char *timeBasePath, float *data, int dim1, int dim2, double time);
int putVect2DDoubleSlice(int expIdx, char *cpoPath, char *path, char *timeBasePath, double *data, int dim1, int dim2, double time);
int putVect3DIntSlice(int expIdx, char *cpoPath, char *path, char *timeBasePath, int *data, int dim1, int dim2, int dim3, double time);
int putVect3DFloatSlice(int expIdx, char *cpoPath, char *path, char *timeBasePath, float *data, int dim1, int dim2, int dim3, double time);
int putVect3DDoubleSlice(int expIdx, char *cpoPath, char *path, char *timeBasePath, double *data, int dim1, int dim2, int dim3, double time);
int putVect4DIntSlice(int expIdx, char *cpoPath, char *path, char *timeBasePath, int *data, int dim1, int dim2, int dim3, int dim4, double time);
int putVect4DFloatSlice(int expIdx, char *cpoPath, char *path, char *timeBasePath, float *data, int dim1, int dim2, int dim3, int dim4, double time);
int putVect4DDoubleSlice(int expIdx, char *cpoPath, char *path, char *timeBasePath, double *data, int dim1, int dim2, int dim3, int dim4, double time);
int putVect5DIntSlice(int expIdx, char *cpoPath, char *path, char *timeBasePath, int *data, int dim1, int dim2, int dim3, int dim4, int dim5, double time);
int putVect5DFloatSlice(int expIdx, char *cpoPath, char *path, char *timeBasePath, float *data, int dim1, int dim2, int dim3, int dim4, int dim5, double time);
int putVect5DDoubleSlice(int expIdx, char *cpoPath, char *path, char *timeBasePath, double *data, int dim1, int dim2, int dim3, int dim4, int dim5, double time);
int putVect6DIntSlice(int expIdx, char *cpoPath, char *path, char *timeBasePath, int *data, int dim1, int dim2, int dim3, int dim4, int dim5, int dim6, double time);
int putVect6DFloatSlice(int expIdx, char *cpoPath, char *path, char *timeBasePath, float *data, int dim1, int dim2, int dim3, int dim4, int dim5, int dim6, double time);
int putVect6DDoubleSlice(int expIdx, char *cpoPath, char *path, char *timeBasePath, double *data, int dim1, int dim2, int dim3, int dim4, int dim5, int dim6, double time);

int replaceLastIntSlice(int expIdx, char *cpoPath, char *path, int data);
int replaceLastFloatSlice(int expIdx, char *cpoPath, char *path, float data);
int replaceLastDoubleSlice(int expIdx, char *cpoPath, char *path, double data);
int replaceLastStringSlice(int expIdx, char *cpoPath, char *path, char *data);
int replaceLastVect1DIntSlice(int expIdx, char *cpoPath, char *path, int *data, int dim);
int replaceLastVect1DFloatSlice(int expIdx, char *cpoPath, char *path, float *data, int dim);
int replaceLastVect1DDoubleSlice(int expIdx, char *cpoPath, char *path, double *data, int dim);
int replaceLastVect2DIntSlice(int expIdx, char *cpoPath, char *path, int *data, int dim1, int dim2);
int replaceLastVect2DFloatSlice(int expIdx, char *cpoPath, char *path, float *data, int dim1, int dim2);
int replaceLastVect2DDoubleSlice(int expIdx, char *cpoPath, char *path, double *data, int dim1, int dim2);
int replaceLastVect3DIntSlice(int expIdx, char *cpoPath, char *path, int *data, int dim1, int dim2, int dim3);
int replaceLastVect3DFloatSlice(int expIdx, char *cpoPath, char *path, float *data, int dim1, int dim2, int dim3);
int replaceLastVect3DDoubleSlice(int expIdx, char *cpoPath, char *path, double *data, int dim1, int dim2, int dim3);
int replaceLastVect4DIntSlice(int expIdx, char *cpoPath, char *path, int *data, int dim1, int dim2, int dim3, int dim4);
int replaceLastVect4DFloatSlice(int expIdx, char *cpoPath, char *path, float *data, int dim1, int dim2, int dim3, int dim4);
int replaceLastVect4DDoubleSlice(int expIdx, char *cpoPath, char *path, double *data, int dim1, int dim2, int dim3, int dim4);
int replaceLastVect5DIntSlice(int expIdx, char *cpoPath, char *path, int *data, int dim1, int dim2, int dim3, int dim4, int dim5);
int replaceLastVect5DFloatSlice(int expIdx, char *cpoPath, char *path, float *data, int dim1, int dim2, int dim3, int dim4, int dim5);
int replaceLastVect5DDoubleSlice(int expIdx, char *cpoPath, char *path, double *data, int dim1, int dim2, int dim3, int dim4, int dim5);
int replaceLastVect6DIntSlice(int expIdx, char *cpoPath, char *path, int *data, int dim1, int dim2, int dim3, int dim4, int dim5, int dim6);
int replaceLastVect6DFloatSlice(int expIdx, char *cpoPath, char *path, float *data, int dim1, int dim2, int dim3, int dim4, int dim5, int dim6);
int replaceLastVect6DDoubleSlice(int expIdx, char *cpoPath, char *path, double *data, int dim1, int dim2, int dim3, int dim4, int dim5, int dim6);

int getString(int expIdx, char *cpoPath, char *path, char **data);
int getFloat(int expIdx, char *cpoPath, char *path, float *data);
int getInt(int expIdx, char *cpoPath, char *path, int *data);
int getDouble(int expIdx, char *cpoPath, char *path, double *data);
int getVect1DString(int expIdx, char *cpoPath, char *path, char  ***data, int *dim);
int getVect1DInt(int expIdx, char *cpoPath, char *path, int **data, int *dim);
int getVect1DFloat(int expIdx, char *cpoPath, char *path, float **data, int *dim);
int getVect1DDouble(int expIdx, char *cpoPath, char *path, double **data, int *dim);
int getVect2DInt(int expIdx, char *cpoPath, char *path, int **data, int *dim1, int *dim2);
int getVect2DFloat(int expIdx, char *cpoPath, char *path, float **data, int *dim1, int *dim2);
int getVect2DDouble(int expIdx, char *cpoPath, char *path, double **data, int *dim1, int *dim2);
int getVect3DInt(int expIdx, char *cpoPath, char *path, int **data, int *dim1, int *dim2, int *dim3);
int getVect3DFloat(int expIdx, char *cpoPath, char *path, float **data, int *dim1, int *dim2, int *dim3);
int getVect3DDouble(int expIdx, char *cpoPath, char *path, double **data, int *dim1, int *dim2, int *dim3);
int getVect4DInt(int expIdx, char *cpoPath, char *path, int **data, int *dim1, int *dim2, int *dim3, int *dim4);
int getVect4DFloat(int expIdx, char *cpoPath, char *path, float **data, int *dim1, int *dim2, int *dim3, int *dim4);
int getVect4DDouble(int expIdx, char *cpoPath, char *path, double **data, int *dim1, int *dim2, int *dim3, int *dim4);
int getVect5DInt(int expIdx, char *cpoPath, char *path, int **data, int *dim1, int *dim2, int *dim3, int *dim4, int *dim5);
int getVect5DFloat(int expIdx, char *cpoPath, char *path, float **data, int *dim1, int *dim2, int *dim3, int *dim4, int *dim5);
int getVect5DDouble(int expIdx, char *cpoPath, char *path, double **data, int *dim1, int *dim2, int *dim3, int *dim4, int *dim5);
int getVect6DInt(int expIdx, char *cpoPath, char *path, int **data, int *dim1, int *dim2, int *dim3, int *dim4, int *dim5, int *dim6);
int getVect6DFloat(int expIdx, char *cpoPath, char *path, float **data, int *dim1, int *dim2, int *dim3, int *dim4, int *dim5, int *dim6);
int getVect6DDouble(int expIdx, char *cpoPath, char *path, double **data, int *dim1, int *dim2, int *dim3, int *dim4, int *dim5, int *dim6);
int getVect7DInt(int expIdx, char *cpoPath, char *path, int **data, int *dim1, int *dim2, int *dim3, int *dim4, int *dim5, int *dim6, int *dim7);
int getVect7DFloat(int expIdx, char *cpoPath, char *path, float **data, int *dim1, int *dim2, int *dim3, int *dim4, int *dim5, int *dim6, int *dim7);
int getVect7DDouble(int expIdx, char *cpoPath, char *path, double **data, int *dim1, int *dim2, int *dim3, int *dim4, int *dim5, int *dim6, int *dim7);


int getStringSlice(int expIdx, char *cpoPath, char *path, char *timeBasePath, char **data, double time, double *retTime, int interpolMode);
int getFloatSlice(int expIdx, char *cpoPath, char *path, char *timeBasePath, float *data, double time, double *retTime, int interpolMode);
int getIntSlice(int expIdx, char *cpoPath, char *path, char *timeBasePath, int *data, double time, double *retTime, int interpolMode);
int getDoubleSlice(int expIdx, char *cpoPath, char *path, char *timeBasePath, double *data, double time, double *retTime, int interpolMode);
int getVect1DIntSlice(int expIdx, char *cpoPath, char *path, char *timeBasePath, int **data, int *dim, double time, double *retTime, int interpolMode);
int getVect1DFloatSlice(int expIdx, char *cpoPath, char *path, char *timeBasePath, float **data, int *dim, double time, double *retTime, int interpolMode);
int getVect1DDoubleSlice(int expIdx, char *cpoPath, char *path, char *timeBasePath, double **data, int *dim, double time, double *retTime, int interpolMode);
int getVect2DIntSlice(int expIdx, char *cpoPath, char *path, char *timeBasePath, int **data, int *dim1, int *dim2, double time, double *retTime, int interpolMode);
int getVect2DFloatSlice(int expIdx, char *cpoPath, char *path, char *timeBasePath, float **data, int *dim1, int *dim2, double time, double *retTime, int interpolMode);
int getVect2DDoubleSlice(int expIdx, char *cpoPath, char *path, char *timeBasePath, double **data, int *dim1, int *dim2, double time, double *retTime, int interpolMode);
int getVect3DIntSlice(int expIdx, char *cpoPath, char *path, char *timeBasePath, int **data, int *dim1, int *dim2, int *dim3, double time, double *retTime, int interpolMode);
int getVect3DFloatSlice(int expIdx, char *cpoPath, char *path, char *timeBasePath, float **data, int *dim1, int *dim2, int *dim3, double time, double *retTime, int interpolMode);
int getVect3DDoubleSlice(int expIdx, char *cpoPath, char *path, char *timeBasePath, double **data, int *dim1, int *dim2, int *dim3, double time, double *retTime, int interpolMode);
int getVect4DIntSlice(int expIdx, char *cpoPath, char *path, char *timeBasePath, int **data, int *dim1, int *dim2, int *dim3, int *dim4, double time, double *retTime, int interpolMode);
int getVect4DFloatSlice(int expIdx, char *cpoPath, char *path, char *timeBasePath, float **data, int *dim1, int *dim2, int *dim3, int *dim4, double time, double *retTime, int interpolMode);
int getVect4DDoubleSlice(int expIdx, char *cpoPath, char *path, char *timeBasePath, double **data, int *dim1, int *dim2, int *dim3, int *dim4, double time, double *retTime, int interpolMode);
int getVect5DIntSlice(int expIdx, char *cpoPath, char *path, char *timeBasePath, int **data, int *dim1, int *dim2, int *dim3, int *dim4, int *dim5, double time, double *retTime, int interpolMode);
int getVect5DFloatSlice(int expIdx, char *cpoPath, char *path, char *timeBasePath, float **data, int *dim1, int *dim2, int *dim3, int *dim4, int *dim5, double time, double *retTime, int interpolMode);
int getVect5DDoubleSlice(int expIdx, char *cpoPath, char *path, char *timeBasePath, double **data, int *dim1, int *dim2, int *dim3, int *dim4, int *dim5, double time, double *retTime, int interpolMode);
int getVect6DIntSlice(int expIdx, char *cpoPath, char *path, char *timeBasePath, int **data, int *dim1, int *dim2, int *dim3, int *dim4, int *dim5, int *dim6, double time, double *retTime, int interpolMode);
int getVect6DFloatSlice(int expIdx, char *cpoPath, char *path, char *timeBasePath, float **data, int *dim1, int *dim2, int *dim3, int *dim4, int *dim5, int *dim6, double time, double *retTime, int interpolMode);
int getVect6DDoubleSlice(int expIdx, char *cpoPath, char *path, char *timeBasePath, double **data, int *dim1, int *dim2, int *dim3, int *dim4, int *dim5, int *dim6, double time, double *retTime, int interpolMode);

int deleteData(int expIdx, char *cpoPath, char *path);
int getUniqueRun(int shot);
int getDimension(int expIdx, char *cpoPath, char *path, int *numDims, int *dim1, int *dim2, int *dim3, int *dim4, int *dim5, int *dim6, int *dim7);
int ual_get_shot(int idx);
int ual_get_run(int idx);

int beginIdsGet(int expIdx, char *path, int isTimed, int *retSamples);
int endIdsGet(int expIdx, char *path);
int beginIdsGetSlice(int expIdx, char *path, double time);
int endIdsGetSlice(int expIdx, char *path);
int beginIdsPut(int expIdx, char *path);
int endIdsPut(int expIdx, char *path);
int beginIdsPutTimed(int expIdx, char *path, int samples, double *inTimes);
int endIdsPutTimed(int expIdx, char *path);
int beginIdsPutNonTimed(int expIdx, char *path);
int endIdsPutNonTimed(int expIdx, char *path);
int beginIdsPutSlice(int expIdx, char *path);
int endIdsPutSlice(int expIdx, char *path);
int beginIdsReplaceLastSlice(int expIdx, char *path);
int endIdsReplaceLastSlice(int expIdx, char *path);

char *imas_last_errmsg();

int imas_create(char *name, int shot, int run, int refShot, int refRun, int *retIdx);
int imas_open(char *name, int shot, int run, int *retIdx);
int imas_create_env(char *name, int shot, int run, int refShot, int refRun, int *retIdx, char *user, char *tokamak, char *version);
int imas_open_env(char *name, int shot, int run, int *retIdx, char *user, char *tokamak, char *version);
int imas_create_hdf5(char *name, int shot, int run, int refShot, int refRun, int *retIdx);
int imas_open_hdf5(char *name, int shot, int run, int *retIdx);
int imas_create_public(char *name, int shot, int run, int refShot, int refRun, int *retIdx, const char* expName);
int imas_open_public(char *name, int shot, int run, int *retIdx, const char* expName);
int imas_close(int idx);

void imas_enable_mem_cache(int expIdx);
void imas_disable_mem_cache(int expIdx);
void imas_discard_mem_cache(int expIdx);
void imas_flush_mem_cache(int expIdx);
void imas_discard_cpo_mem_cache(int expIdx, char *cpoPath);
void imas_flush_cpo_mem_cache(int expIdx, char *cpoPath);

int imas_connect(char *ip);
int imas_disconnect();
char *imas_exec(char *ip, char *command);

//---------------- Array of structures support ----------------

// management of object list
void removeObjectFromList(int idx);
int addObjectToList(void *obj);
void replaceObjectInList(int idx, void *obj);
void *getObjectFromList(int idx);

//Initialize room for a generic array of structures
void *beginObject(int expIdx, void *obj, int index, const char *relPath, int isTimed);

//Releases memory for array of structures
void releaseObject(int expIdx, void *obj);


//Writes an array of objects array of structures
//Flag isTImes specifies whether the array refers to time-dependent field (in this case the dimension refers to time)
int putObject(int expIdx, char *cpoPath, char *path, void *obj, int isTimed);

//Add elements to the structure array. Note: returns the new pointer to the object, possibly changed (if object reallocated)
//Argument path refers to the path name within the structure
//Argument idx is the index in the array of structures
void *putIntInObject(int expIdx, void *obj, char *path, int idx, int data);
void *putStringInObject(int expIdx, void *obj, char *path, int idx, char *data);
void *putFloatInObject(int expIdx, void *obj, char *path, int idx, float data);
void *putDoubleInObject(int expIdx, void *obj, char *path, int idx, double data);
void *putVect1DStringInObject(int expIdx, void *obj, char *path, int idx, char **data, int dim);
void *putVect1DIntInObject(int expIdx, void *obj, char *path, int idx, int *data, int dim);
void *putVect1DFloatInObject(int expIdx, void *obj, char *path, int idx, float *data, int dim);
void *putVect1DDoubleInObject(int expIdx, void *obj, char *path, int idx, double *data, int dim);
void *putVect2DIntInObject(int expIdx, void *obj, char *path, int idx, int *data, int dim1, int dim2);
void *putVect2DFloatInObject(int expIdx, void *obj, char *path, int idx, float *data, int dim1, int dim2);
void *putVect2DDoubleInObject(int expIdx, void *obj, char *path, int idx, double *data, int dim1, int dim2);
void *putVect3DIntInObject(int expIdx, void *obj, char *path, int idx, int *data, int dim1, int dim2, int dim3);
void *putVect3DFloatInObject(int expIdx, void *obj, char *path, int idx, float *data, int dim1, int dim2, int dim3);
void *putVect3DDoubleInObject(int expIdx, void *obj, char *path, int idx, double *data, int dim1, int dim2, int dim3);
void *putVect4DIntInObject(int expIdx, void *obj, char *path, int idx, int *data, int dim1, int dim2, int dim3, int dim4);
void *putVect4DFloatInObject(int expIdx, void *obj, char *path, int idx, float *data, int dim1, int dim2, int dim3, int dim4);
void *putVect4DDoubleInObject(int expIdx, void *obj, char *path, int idx, double *data, int dim1, int dim2, int dim3, int dim4);
void *putVect5DIntInObject(int expIdx,void *obj, char *path, int idx, int *data, int dim1, int dim2, int dim3, int dim4, int dim5);
void *putVect5DFloatInObject(int expIdx, void *obj, char *path, int idx, float *data, int dim1, int dim2, int dim3, int dim4,int dim5);
void *putVect5DDoubleInObject(int expIdx, void *obj, char *path, int idx, double *data, int dim1, int dim2, int dim3, int dim4,int dim5);
void *putVect6DIntInObject(int expIdx, void *obj, char *path, int idx, int *data, int dim1, int dim2, int dim3, int dim4, int dim5, int dim6);
void *putVect6DFloatInObject(int expIdx, void *obj, char *path, int idx, float *data, int dim1, int dim2, int dim3, int dim4,int dim5, int dim6);
void *putVect6DDoubleInObject(int expIdx, void *obj, char *path, int idx, double *data, int dim1, int dim2, int dim3, int dim4,int dim5, int dim6);
void *putVect7DIntInObject(int expIdx, void *obj, char *path, int idx, int *data, int dim1, int dim2, int dim3, int dim4, int dim5, int dim6, int dim7);
void *putVect7DFloatInObject(int expIdx, void *obj, char *path, int idx, float *data, int dim1, int dim2, int dim3, int dim4,int dim5, int dim6, int dim7);
void *putVect7DDoubleInObject(int expIdx, void *obj, char *path, int idx, double *data, int dim1, int dim2, int dim3, int dim4,int dim5, int dim6, int dim7);
void *putObjectInObject(int expIdx, void *obj, char *path, int idx, void *dataObj);

//Retrieve the number of elements for the array of structures. Returns -1 if an error occurs;
int getObjectDim(int expIdx, void *obj);

//Read the array of structures from the pulse file. Status indicates as always success (0) or error (!= 0)
int getObject(int expIdx, char *cpoPath, char *path, void **obj, int isTimed);

//Retrieves components from array of strictures. Returned status indicates success (0) or error(!= 0)
int getStringFromObject(int expIdx, void *obj, char *path, int idx, char **data);
int getFloatFromObject(int expIdx, void *obj, char *path, int idx, float *data);
int getIntFromObject(int expIdx, void *obj, char *path, int idx, int *data);
int getDoubleFromObject(int expIdx, void *obj, char *path, int idx, double *data);
int getVect1DStringFromObject(int expIdx, void *obj, char *path, int idx, char  ***data, int *dim);
int getVect1DIntFromObject(int expIdx, void *obj, char *path, int idx, int **data, int *dim);
int getVect1DFloatFromObject(int expIdx, void *obj, char *path, int idx, float **data, int *dim);
int getVect1DDoubleFromObject(int expIdx, void *obj, char *path, int idx, double **data, int *dim);
int getVect2DIntFromObject(int expIdx, void *obj, char *path, int idx, int **data, int *dim1, int *dim2);
int getVect2DFloatFromObject(int expIdx, void *obj, char *path, int idx, float **data, int *dim1, int *dim2);
int getVect2DDoubleFromObject(int expIdx, void *obj, char *path, int idx, double **data, int *dim1, int *dim2);
int getVect3DIntFromObject(int expIdx, void *obj, char *path, int idx, int **data, int *dim1, int *dim2, int *dim3);
int getVect3DFloatFromObject(int expIdx, void *obj, char *path, int idx, float **data, int *dim1, int *dim2, int *dim3);
int getVect3DDoubleFromObject(int expIdx, void *obj, char *path, int idx, double **data, int *dim1, int *dim2, int *dim3);
int getVect4DIntFromObject(int expIdx, void *obj, char *path, int idx, int **data, int *dim1, int *dim2, int *dim3, int *dim4);
int getVect4DFloatFromObject(int expIdx, void *obj, char *path, int idx, float **data, int *dim1, int *dim2, int *dim3, int *dim4);
int getVect4DDoubleFromObject(int expIdx, void *obj, char *path, int idx, double **data, int *dim1, int *dim2, int *dim3, int *dim4);
int getVect5DIntFromObject(int expIdx, void *obj, char *path, int idx, int **data, int *dim1, int *dim2, int *dim3, int *dim4, int *dim5);
int getVect5DFloatFromObject(int expIdx, void *obj, char *path, int idx, float **data, int *dim1, int *dim2, int *dim3, int *dim4, int *dim5);
int getVect5DDoubleFromObject(int expIdx, void *obj, char *path, int idx, double **data, int *dim1, int *dim2, int *dim3, int *dim4, int *dim5);
int getVect6DIntFromObject(int expIdx, void *obj, char *path, int idx, int **data, int *dim1, int *dim2, int *dim3, int *dim4, int *dim5, int *dim6);
int getVect6DFloatFromObject(int expIdx, void *obj, char *path, int idx, float **data, int *dim1, int *dim2, int *dim3, int *dim4, int *dim5, int *dim6);
int getVect6DDoubleFromObject(int expIdx, void *obj, char *path, int idx, double **data, int *dim1, int *dim2, int *dim3, int *dim4, int *dim5, int *dim6);
int getVect7DIntFromObject(int expIdx, void *obj, char *path, int idx, int **data, int *dim1, int *dim2, int *dim3, int *dim4, int *dim5, int *dim6, int *dim7);
int getVect7DFloatFromObject(int expIdx, void *obj, char *path, int idx, float **data, int *dim1, int *dim2, int *dim3, int *dim4, int *dim5, int *dim6, int *dim7);
int getVect7DDoubleFromObject(int expIdx, void *obj, char *path, int idx, double **data, int *dim1, int *dim2, int *dim3, int *dim4, int *dim5, int *dim6, int *dim7);
int getObjectFromObject(int expIdx, void *obj, char *path, int idx, void **dataObj);

int getDimensionFromObject(int expIdx, void *obj, char *path, int idx, int *numDims, int *dim1, int *dim2, int *dim3, int *dim4, int *dim5, int *dim6, int *dim7);

//Array of structures Slice Management
int getObjectSlice(int expIdx, char *cpoPath, char *path, double time, void **obj);
int putObjectSlice(int expIdx, char *cpoPath, char *path, double time, void *obj);
int replaceLastObjectSlice(int expIdx, char *cpoPath, char *path, void *obj);


//CPO copies
int ual_copy_cpo(int fromIdx, int toIdx, char *cpoName, int fromCpoOccur, int toCpoOccur);
int ual_copy_cpo_env(char *tokamakFrom, char *versionFrom, char *userFrom, int shotFrom, int runFrom, int occurrenceFrom,  
	char *tokamakTo, char *versionTo, char *userTo, int shotTo, int runTo, int occurrenceTo, char *cpoName);


//New memory cache settings
void imas_set_cache_level(int expIdx, int level);
int imas_get_cache_level(int expIdx);

//Error Management support routine
#define WRITE_ERROR 1
#define READ_ERROR 2
#define OPEN_ERROR 3
#define CLOSE_ERROR 4
int  makeErrorStatus(int isCritical, int type, int intStatus);
int isCriticalError(int status);
int getErrorType(int status);

#endif
