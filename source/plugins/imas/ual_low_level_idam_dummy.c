#define INTERPOLATION   3
#define CLOSEST_SAMPLE  1
#define PREVIOUS_SAMPLE 2

#include <stdlib.h>

extern int idamGetDimension(int expIdx, char *cpoPath, char *path, int *numDims, int *dim1, int *dim2, int *dim3, int *dim4, int *dim5, int *dim6, int *dim7) { return -1; }

extern int idamimasConnect(char *ip) { return -1; }
extern int idamimasCreate(char *name, int shot, int run, int refShot, int refRun, int *retIdx) { return -1; }
extern int idamimasCreateEnv(char *name, int shot, int run, int refShot, int refRun, int *retIdx, char *user, char *tokamak, char *version) { return -1; }
extern int idamimasOpen(char *name, int shot, int run, int *retIdx) { return -1; }
extern int idamimasOpenEnv(char *name, int shot, int run, int *retIdx, char *user, char *tokamak, char *version) { return -1; }
extern int idamimasClose(int idx, char *name, int shot, int run) { return -1; }
extern void disableMemCache() { return; }
extern void discardMem(int expIdx, char *cpoPath, char *path) { return; }
extern void discardOldMem(int expIdx, char *cpoPath, char *path, double time) { return; }
extern void flush(int expIdx, char *cpoPath, char *path) { return; }
extern char *idamLastErrmsg() { return NULL; }

extern int idamPutString(int expIdx, char *cpoPath, char *path, char *data, int strlen) { return -1; }
extern int idamPutInt(int expIdx, char *cpoPath, char *path, int data) { return -1; }
extern int idamPutFloat(int expIdx, char *cpoPath, char *path, float data) { return -1; }
extern int idamPutDouble(int expIdx, char *cpoPath, char *path, double data) { return -1; }
extern int idamPutVect1DString(int expIdx, char *cpoPath, char *path, char **data, int dim, int isTimed) { return -1; }
extern int idamPutVect1DInt(int expIdx, char *cpoPath, char *path, int *data, int dim, int isTimed) { return -1; }
extern int idamPutVect1DFloat(int expIdx, char *cpoPath, char *path, float *data, int dim, int isTimed) { return -1; }
extern int idamPutVect1DDouble(int expIdx, char *cpoPath, char *path, double *data, int dim, int isTimed) { return -1; }
extern int idamPutVect2DInt(int expIdx, char *cpoPath, char *path, int *data, int dim1, int dim2, int isTimed) { return -1; }
extern int idamPutVect2DFloat(int expIdx, char *cpoPath, char *path, float *data, int dim1, int dim2, int isTimed) { return -1; }
extern int idamPutVect2DDouble(int expIdx, char *cpoPath, char *path, double *data, int dim1, int dim2, int isTimed) { return -1; }
extern int idamPutVect3DInt(int expIdx, char *cpoPath, char *path, int *data, int dim1, int dim2, int dim3, int isTimed) { return -1; }
extern int idamPutVect3DFloat(int expIdx, char *cpoPath, char *path, float *data, int dim1, int dim2, int dim3, int isTimed) { return -1; }
extern int idamPutVect3DDouble(int expIdx, char *cpoPath, char *path, double *data, int dim1, int dim2, int dim3, int isTimed) { return -1; }
extern int idamPutVect4DInt(int expIdx, char *cpoPath, char *path, int *data, int dim1, int dim2, int dim3, int dim4, int isTimed) { return -1; }
extern int idamPutVect4DFloat(int expIdx, char *cpoPath, char *path, float *data, int dim1, int dim2, int dim3, int dim4, int isTimed) { return -1; }
extern int idamPutVect4DDouble(int expIdx, char *cpoPath, char *path, double *data, int dim1, int dim2, int dim3, int dim4, int isTimed) { return -1; }
extern int idamPutVect5DInt(int expIdx, char *cpoPath, char *path, int *data, int dim1, int dim2, int dim3, int dim4, int dim5, int isTimed) { return -1; }
extern int idamPutVect5DFloat(int expIdx, char *cpoPath, char *path, float *data, int dim1, int dim2, int dim3, int dim4, int dim5, int isTimed) { return -1; }
extern int idamPutVect5DDouble(int expIdx, char *cpoPath, char *path, double *data, int dim1, int dim2, int dim3, int dim4, int dim5, int isTimed) { return -1; }
extern int idamPutVect6DInt(int expIdx, char *cpoPath, char *path, int *data, int dim1, int dim2, int dim3, int dim4, int dim5, int dim6, int isTimed) { return -1; }
extern int idamPutVect6DFloat(int expIdx, char *cpoPath, char *path, float *data, int dim1, int dim2, int dim3, int dim4, int dim5, int dim6, int isTimed) { return -1; }
extern int idamPutVect6DDouble(int expIdx, char *cpoPath, char *path, double *data, int dim1, int dim2, int dim3, int dim4, int dim5, int dim6, int isTimed) { return -1; }
extern int idamPutVect7DInt(int expIdx, char *cpoPath, char *path, int *data, int dim1, int dim2, int dim3, int dim4, int dim5, int dim6, int dim7, int isTimed) { return -1; }
extern int idamPutVect7DFloat(int expIdx, char *cpoPath, char *path, float *data, int dim1, int dim2, int dim3, int dim4, int dim5, int dim6, int dim7, int isTimed) { return -1; }
extern int idamPutVect7DDouble(int expIdx, char *cpoPath, char *path, double *data, int dim1, int dim2, int dim3, int dim4, int dim5, int dim6, int dim7, int isTimed) { return -1; }
extern int idamPutIntSlice(int expIdx, char *cpoPath, char *path, int data, double time) { return -1; }
extern int idamPutFloatSlice(int expIdx, char *cpoPath, char *path, float data, double time) { return -1; }
extern int idamPutDoubleSlice(int expIdx, char *cpoPath, char *path, double data, double time) { return -1; }
extern int idamPutStringSlice(int expIdx, char *cpoPath, char *path, char *data, double time) { return -1; }
extern int idamPutVect1DIntSlice(int expIdx, char *cpoPath, char *path, int *data, int dim, double time) { return -1; }
extern int idamPutVect1DFloatSlice(int expIdx, char *cpoPath, char *path, float *data, int dim, double time) { return -1; }
extern int idamPutVect1DDoubleSlice(int expIdx, char *cpoPath, char *path, double *data, int dim, double time) { return -1; }
extern int idamPutVect2DIntSlice(int expIdx, char *cpoPath, char *path, int *data, int dim1, int dim2, double time) { return -1; }
extern int idamPutVect2DFloatSlice(int expIdx, char *cpoPath, char *path, float *data, int dim1, int dim2, double time) { return -1; }
extern int idamPutVect2DDoubleSlice(int expIdx, char *cpoPath, char *path, double *data, int dim1, int dim2, double time) { return -1; }
extern int idamPutVect3DIntSlice(int expIdx, char *cpoPath, char *path, int *data, int dim1, int dim2, int dim3, double time) { return -1; }
extern int idamPutVect3DFloatSlice(int expIdx, char *cpoPath, char *path, float *data, int dim1, int dim2, int dim3, double time) { return -1; }
extern int idamPutVect3DDoubleSlice(int expIdx, char *cpoPath, char *path, double *data, int dim1, int dim2, int dim3, double time) { return -1; }
extern int idamPutVect4DIntSlice(int expIdx, char *cpoPath, char *path, int *data, int dim1, int dim2, int dim3, int dim4, double time) { return -1; }
extern int idamPutVect4DFloatSlice(int expIdx, char *cpoPath, char *path, float *data, int dim1, int dim2, int dim3, int dim4, double time) { return -1; }
extern int idamPutVect4DDoubleSlice(int expIdx, char *cpoPath, char *path, double *data, int dim1, int dim2, int dim3, int dim4, double time) { return -1; }
extern int idamPutVect5DIntSlice(int expIdx, char *cpoPath, char *path, int *data, int dim1, int dim2, int dim3, int dim4, int dim5, double time) { return -1; }
extern int idamPutVect5DFloatSlice(int expIdx, char *cpoPath, char *path, float *data, int dim1, int dim2, int dim3, int dim4, int dim5, double time) { return -1; }
extern int idamPutVect5DDoubleSlice(int expIdx, char *cpoPath, char *path, double *data, int dim1, int dim2, int dim3, int dim4, int dim5, double time) { return -1; }
extern int idamPutVect6DIntSlice(int expIdx, char *cpoPath, char *path, int *data, int dim1, int dim2, int dim3, int dim4, int dim5, int dim6, double time) { return -1; }
extern int idamPutVect6DFloatSlice(int expIdx, char *cpoPath, char *path, float *data, int dim1, int dim2, int dim3, int dim4, int dim5, int dim6, double time) { return -1; }
extern int idamPutVect6DDoubleSlice(int expIdx, char *cpoPath, char *path, double *data, int dim1, int dim2, int dim3, int dim4, int dim5, int dim6, double time) { return -1; }

extern int idamReplaceLastIntSlice(int expIdx, char *cpoPath, char *path, int data) { return -1; }
extern int idamReplaceLastFloatSlice(int expIdx, char *cpoPath, char *path, float data) { return -1; }
extern int idamReplaceLastDoubleSlice(int expIdx, char *cpoPath, char *path, double data) { return -1; }
extern int idamReplaceLastStringSlice(int expIdx, char *cpoPath, char *path, char *data) { return -1; }
extern int idamReplaceLastVect1DIntSlice(int expIdx, char *cpoPath, char *path, int *data, int dim) { return -1; }
extern int idamReplaceLastVect1DFloatSlice(int expIdx, char *cpoPath, char *path, float *data, int dim) { return -1; }
extern int idamReplaceLastVect1DDoubleSlice(int expIdx, char *cpoPath, char *path, double *data, int dim) { return -1; }
extern int idamReplaceLastVect2DIntSlice(int expIdx, char *cpoPath, char *path, int *data, int dim1, int dim2) { return -1; }
extern int idamReplaceLastVect2DFloatSlice(int expIdx, char *cpoPath, char *path, float *data, int dim1, int dim2) { return -1; }
extern int idamReplaceLastVect2DDoubleSlice(int expIdx, char *cpoPath, char *path, double *data, int dim1, int dim2) { return -1; }
extern int idamReplaceLastVect3DIntSlice(int expIdx, char *cpoPath, char *path, int *data, int dim1, int dim2, int dim3) { return -1; }
extern int idamReplaceLastVect3DFloatSlice(int expIdx, char *cpoPath, char *path, float *data, int dim1, int dim2, int dim3) { return -1; }
extern int idamReplaceLastVect3DDoubleSlice(int expIdx, char *cpoPath, char *path, double *data, int dim1, int dim2, int dim3) { return -1; }
extern int idamReplaceLastVect4DIntSlice(int expIdx, char *cpoPath, char *path, int *data, int dim1, int dim2, int dim3, int dim4) { return -1; }
extern int idamReplaceLastVect4DFloatSlice(int expIdx, char *cpoPath, char *path, float *data, int dim1, int dim2, int dim3, int dim4) { return -1; }
extern int idamReplaceLastVect4DDoubleSlice(int expIdx, char *cpoPath, char *path, double *data, int dim1, int dim2, int dim3, int dim4) { return -1; }
extern int idamReplaceLastVect5DIntSlice(int expIdx, char *cpoPath, char *path, int *data, int dim1, int dim2, int dim3, int dim4, int dim5) { return -1; }
extern int idamReplaceLastVect5DFloatSlice(int expIdx, char *cpoPath, char *path, float *data, int dim1, int dim2, int dim3, int dim4, int dim5) { return -1; }
extern int idamReplaceLastVect5DDoubleSlice(int expIdx, char *cpoPath, char *path, double *data, int dim1, int dim2, int dim3, int dim4, int dim5) { return -1; }
extern int idamReplaceLastVect6DIntSlice(int expIdx, char *cpoPath, char *path, int *data, int dim1, int dim2, int dim3, int dim4, int dim5, int dim6) { return -1; }
extern int idamReplaceLastVect6DFloatSlice(int expIdx, char *cpoPath, char *path, float *data, int dim1, int dim2, int dim3, int dim4, int dim5, int dim6) { return -1; }
extern int idamReplaceLastVect6DDoubleSlice(int expIdx, char *cpoPath, char *path, double *data, int dim1, int dim2, int dim3, int dim4, int dim5, int dim6) { return -1; }

extern int idamGetString(int expIdx, char *cpoPath, char *path, char **data) { return -1; }
extern int idamGetFloat(int expIdx, char *cpoPath, char *path, float *data) { return -1; }
extern int idamGetInt(int expIdx, char *cpoPath, char *path, int *data) { return -1; }
extern int idamGetDouble(int expIdx, char *cpoPath, char *path, double *data) { return -1; }
extern int idamGetVect1DString(int expIdx, char *cpoPath, char *path, char  ***data, int *dim) { return -1; }
extern int idamGetVect1DInt(int expIdx, char *cpoPath, char *path, int **data, int *dim) { return -1; }
extern int idamGetVect1DFloat(int expIdx, char *cpoPath, char *path, float **data, int *dim) { return -1; }
extern int idamGetVect1DDouble(int expIdx, char *cpoPath, char *path, double **data, int *dim) { return -1; }
extern int idamGetVect2DInt(int expIdx, char *cpoPath, char *path, int **data, int *dim1, int *dim2) { return -1; }
extern int idamGetVect2DFloat(int expIdx, char *cpoPath, char *path, float **data, int *dim1, int *dim2) { return -1; }
extern int idamGetVect2DDouble(int expIdx, char *cpoPath, char *path, double **data, int *dim1, int *dim2) { return -1; }
extern int idamGetVect3DInt(int expIdx, char *cpoPath, char *path, int **data, int *dim1, int *dim2, int *dim3) { return -1; }
extern int idamGetVect3DFloat(int expIdx, char *cpoPath, char *path, float **data, int *dim1, int *dim2, int *dim3) { return -1; }
extern int idamGetVect3DDouble(int expIdx, char *cpoPath, char *path, double **data, int *dim1, int *dim2, int *dim3) { return -1; }
extern int idamGetVect4DInt(int expIdx, char *cpoPath, char *path, int **data, int *dim1, int *dim2, int *dim3, int *dim4) { return -1; }
extern int idamGetVect4DFloat(int expIdx, char *cpoPath, char *path, float **data, int *dim1, int *dim2, int *dim3, int *dim4) { return -1; }
extern int idamGetVect4DDouble(int expIdx, char *cpoPath, char *path, double **data, int *dim1, int *dim2, int *dim3, int *dim4) { return -1; }
extern int idamGetVect5DInt(int expIdx, char *cpoPath, char *path, int **data, int *dim1, int *dim2, int *dim3, int *dim4, int *dim5) { return -1; }
extern int idamGetVect5DFloat(int expIdx, char *cpoPath, char *path, float **data, int *dim1, int *dim2, int *dim3, int *dim4, int *dim5) { return -1; }
extern int idamGetVect5DDouble(int expIdx, char *cpoPath, char *path, double **data, int *dim1, int *dim2, int *dim3, int *dim4, int *dim5) { return -1; }
extern int idamGetVect6DInt(int expIdx, char *cpoPath, char *path, int **data, int *dim1, int *dim2, int *dim3, int *dim4, int *dim5, int *dim6) { return -1; }
extern int idamGetVect6DFloat(int expIdx, char *cpoPath, char *path, float **data, int *dim1, int *dim2, int *dim3, int *dim4, int *dim5, int *dim6) { return -1; }
extern int idamGetVect6DDouble(int expIdx, char *cpoPath, char *path, double **data, int *dim1, int *dim2, int *dim3, int *dim4, int *dim5, int *dim6) { return -1; }
extern int idamGetVect7DInt(int expIdx, char *cpoPath, char *path, int **data, int *dim1, int *dim2, int *dim3, int *dim4, int *dim5, int *dim6, int *dim7) { return -1; }
extern int idamGetVect7DFloat(int expIdx, char *cpoPath, char *path, float **data, int *dim1, int *dim2, int *dim3, int *dim4, int *dim5, int *dim6, int *dim7) { return -1; }
extern int idamGetVect7DDouble(int expIdx, char *cpoPath, char *path, double **data, int *dim1, int *dim2, int *dim3, int *dim4, int *dim5, int *dim6, int *dim7) { return -1; }


extern int idamGetStringSlice(int expIdx, char *cpoPath, char *path, char **data, double time, double *retTime, int interpolMode) { return -1; }
extern int idamGetFloatSlice(int expIdx, char *cpoPath, char *path, float *data, double time, double *retTime, int interpolMode) { return -1; }
extern int idamGetIntSlice(int expIdx, char *cpoPath, char *path, int *data, double time, double *retTime, int interpolMode) { return -1; }
extern int idamGetDoubleSlice(int expIdx, char *cpoPath, char *path, double *data, double time, double *retTime, int interpolMode) { return -1; }
extern int idamGetVect1DIntSlice(int expIdx, char *cpoPath, char *path, int **data, int *dim, double time, double *retTime, int interpolMode) { return -1; }
extern int idamGetVect1DFloatSlice(int expIdx, char *cpoPath, char *path, float **data, int *dim, double time, double *retTime, int interpolMode) { return -1; }
extern int idamGetVect1DDoubleSlice(int expIdx, char *cpoPath, char *path, double **data, int *dim, double time, double *retTime, int interpolMode) { return -1; }
extern int idamGetVect2DIntSlice(int expIdx, char *cpoPath, char *path, int **data, int *dim1, int *dim2, double time, double *retTime, int interpolMode) { return -1; }
extern int idamGetVect2DFloatSlice(int expIdx, char *cpoPath, char *path, float **data, int *dim1, int *dim2, double time, double *retTime, int interpolMode) { return -1; }
extern int idamGetVect2DDoubleSlice(int expIdx, char *cpoPath, char *path, double **data, int *dim1, int *dim2, double time, double *retTime, int interpolMode) { return -1; }
extern int idamGetVect3DIntSlice(int expIdx, char *cpoPath, char *path, int **data, int *dim1, int *dim2, int *dim3, double time, double *retTime, int interpolMode) { return -1; }
extern int idamGetVect3DFloatSlice(int expIdx, char *cpoPath, char *path, float **data, int *dim1, int *dim2, int *dim3, double time, double *retTime, int interpolMode) { return -1; }
extern int idamGetVect3DDoubleSlice(int expIdx, char *cpoPath, char *path, double **data, int *dim1, int *dim2, int *dim3, double time, double *retTime, int interpolMode) { return -1; }
extern int idamGetVect4DIntSlice(int expIdx, char *cpoPath, char *path, int **data, int *dim1, int *dim2, int *dim3, int *dim4, double time, double *retTime, int interpolMode) { return -1; }
extern int idamGetVect4DFloatSlice(int expIdx, char *cpoPath, char *path, float **data, int *dim1, int *dim2, int *dim3, int *dim4, double time, double *retTime, int interpolMode) { return -1; }
extern int idamGetVect4DDoubleSlice(int expIdx, char *cpoPath, char *path, double **data, int *dim1, int *dim2, int *dim3, int *dim4, double time, double *retTime, int interpolMode) { return -1; }
extern int idamGetVect5DIntSlice(int expIdx, char *cpoPath, char *path, int **data, int *dim1, int *dim2, int *dim3, int *dim4, int *dim5, double time, double *retTime, int interpolMode) { return -1; }
extern int idamGetVect5DFloatSlice(int expIdx, char *cpoPath, char *path, float **data, int *dim1, int *dim2, int *dim3, int *dim4, int *dim5, double time, double *retTime, int interpolMode) { return -1; }
extern int idamGetVect5DDoubleSlice(int expIdx, char *cpoPath, char *path, double **data, int *dim1, int *dim2, int *dim3, int *dim4, int *dim5, double time, double *retTime, int interpolMode) { return -1; }
extern int idamGetVect6DIntSlice(int expIdx, char *cpoPath, char *path, int **data, int *dim1, int *dim2, int *dim3, int *dim4, int *dim5, int *dim6, double time, double *retTime, int interpolMode) { return -1; }
extern int idamGetVect6DFloatSlice(int expIdx, char *cpoPath, char *path, float **data, int *dim1, int *dim2, int *dim3, int *dim4, int *dim5, int *dim6, double time, double *retTime, int interpolMode) { return -1; }
extern int idamGetVect6DDoubleSlice(int expIdx, char *cpoPath, char *path, double **data, int *dim1, int *dim2, int *dim3, int *dim4, int *dim5, int *dim6, double time, double *retTime, int interpolMode) { return -1; }

extern int idamDeleteData(int expIdx, char *cpoPath, char *path) { return -1; }

extern int idambeginIdsGet(int expIdx, char *path, int isTimed, int *retSamples) { return -1; }
extern int idamendIdsGet(int expIdx, char *path) { return -1; }
extern int idambeginIdsGetResampled(int expIdx, char *path, double start, double end, double delta, int *retSamples) { return -1; }
extern int idamendIdsGetResampled(int expIdx, char *path) { return -1; }
extern int idambeginIdsGetSlice(int expIdx, char *path, double time) { return -1; }
extern int idamendIdsGetSlice(int expIdx, char *path) { return -1; }
extern int idambeginIdsPut(int expIdx, char *path) { return -1; }
extern int idamendIdsPut(int expIdx, char *path) { return -1; }
extern int idambeginIdsPutTimed(int expIdx, char *path, int samples, double *inTimes) { return -1; }
extern int idamendIdsPutTimed(int expIdx, char *path) { return -1; }
extern int idambeginIdsPutNonTimed(int expIdx, char *path) { return -1; }
extern int idamendIdsPutNonTimed(int expIdx, char *path) { return -1; }
extern int idambeginIdsPutSlice(int expIdx, char *path) { return -1; }
extern int idamendIdsPutSlice(int expIdx, char *path) { return -1; }
extern int idambeginIdsReplaceLastSlice(int expIdx, char *path) { return -1; }
extern int idamendIdsReplaceLastSlice(int expIdx, char *path) { return -1; }

void idamDeleteAllFields(int expIdx, char *cpoPath) { return; }
int idamIsSliced(int expIdx, char *cpoPath, char *path) { return -1; }

//------------------ Array of structures stuff -----------------

//Initialize room for a generic array of structures
void *idamBeginObject() { return NULL; }

//Releases memory for array of structures
void idamReleaseObject(void *obj) { return; }

//Writes an array of objects array of structures
//Flag isTimed specifies whether the array refers to time-dependent field (in this case the dimension refers to time)
int idamPutObject(int expIdx, char *cpoPath, char *idamPath, void *obj, int isTimed) { return -1; }

//Add elements to the structure array. Note: returns the new pointer to the object, possibly chjanged (if object reallocated)
//Argument path refers to the path name within the structure
//Argument idx is the index in the array of structues
void *idamPutIntInObject(void *obj, char *idamPath, int idx, int data) { return NULL; }
void *idamPutStringInObject(void *obj, char *idamPath, int idx, char *data) { return NULL; }
void *idamPutFloatInObject(void *obj, char *idamPath, int idx, float data) { return NULL; }
void *idamPutDoubleInObject(void *obj, char *idamPath, int idx, double data) { return NULL; }
void *idamPutVect1DStringInObject(void *obj, char *idamPath, int idx, char **data, int dim) { return NULL; }
void *idamPutVect1DIntInObject(void *obj, char *idamPath, int idx, int *data, int dim) { return NULL; }
void *idamPutVect1DFloatInObject(void *obj, char *idamPath, int idx, float *data, int dim) { return NULL; }
void *idamPutVect1DDoubleInObject(void *obj, char *idamPath, int idx, double *data, int dim) { return NULL; }
void *idamPutVect2DIntInObject(void *obj, char *idamPath, int idx, int *data, int dim1, int dim2) { return NULL; }
void *idamPutVect2DFloatInObject(void *obj, char *idamPath, int idx, float *data, int dim1, int dim2) { return NULL; }
void *idamPutVect2DDoubleInObject(void *obj, char *idamPath, int idx, double *data, int dim1, int dim2) { return NULL; }
void *idamPutVect3DIntInObject(void *obj, char *idamPath, int idx, int *data, int dim1, int dim2, int dim3) { return NULL; }
void *idamPutVect3DFloatInObject(void *obj, char *idamPath, int idx, float *data, int dim1, int dim2, int dim3) { return NULL; }
void *idamPutVect3DDoubleInObject(void *obj, char *idamPath, int idx, double *data, int dim1, int dim2, int dim3) { return NULL; }
void *idamPutVect4DIntInObject(void *obj, char *idamPath, int idx, int *data, int dim1, int dim2, int dim3, int dim4) { return NULL; }
void *idamPutVect4DFloatInObject(void *obj, char *idamPath, int idx, float *data, int dim1, int dim2, int dim3, int dim4) { return NULL; }
void *idamPutVect4DDoubleInObject(void *obj, char *idamPath, int idx, double *data, int dim1, int dim2, int dim3, int dim4) { return NULL; }
void *idamPutVect5DIntInObject(void *obj, char *idamPath, int idx, int *data, int dim1, int dim2, int dim3, int dim4, int dim5) { return NULL; }
void *idamPutVect5DFloatInObject(void *obj, char *idamPath, int idx, float *data, int dim1, int dim2, int dim3, int dim4,int dim5) { return NULL; }
void *idamPutVect5DDoubleInObject(void *obj, char *idamPath, int idx, double *data, int dim1, int dim2, int dim3, int dim4,int dim5) { return NULL; }
void *idamPutVect6DIntInObject(void *obj, char *idamPath, int idx, int *data, int dim1, int dim2, int dim3, int dim4, int dim5, int dim6) { return NULL; }
void *idamPutVect6DFloatInObject(void *obj, char *idamPath, int idx, float *data, int dim1, int dim2, int dim3, int dim4,int dim5, int dim6) { return NULL; }
void *idamPutVect6DDoubleInObject(void *obj, char *idamPath, int idx, double *data, int dim1, int dim2, int dim3, int dim4,int dim5, int dim6) { return NULL; }
void *idamPutVect7DIntInObject(void *obj, char *idamPath, int idx, int *data, int dim1, int dim2, int dim3, int dim4, int dim5, int dim6, int dim7) { return NULL; }
void *idamPutVect7DFloatInObject(void *obj, char *idamPath, int idx, float *data, int dim1, int dim2, int dim3, int dim4,int dim5, int dim6, int dim7) { return NULL; }
void *idamPutVect7DDoubleInObject(void *obj, char *idamPath, int idx, double *data, int dim1, int dim2, int dim3, int dim4,int dim5, int dim6, int dim7) { return NULL; }
void *idamPutObjectInObject(void *obj, char *idamPath, int idx, void *dataObj) { return NULL; }

//Retrieve the number of elements for the array of structures. Returns -1 if an error occurs;
int idamGetObjectDim(void *obj) { return -1; }

//Read the array of structures from the pulse file. Status indicates as always success (0) or error (!= 0)
int idamGetObject(int expIdx, char *idamPath, char *cpoPath, void **obj, int isTimed) { return -1; }

//Retrieves components from array of structures. Returned status indicates success (0) or error(!= 0)
int idamGetStringFromObject(void *obj, char *idamPath, int idx, char **data) { return -1; }
int idamGetFloatFromObject(void *obj, char *idamPath, int idx, float *data) { return -1; }
int idamGetIntFromObject(void *obj, char *idamPath, int idx, int *data) { return -1; }
int idamGetDoubleFromObject(void *obj, char *idamPath, int idx, double *data) { return -1; }
int idamGetVect1DStringFromObject(void *obj, char *idamPath, int idx, char  ***data, int *dim) { return -1; }
int idamGetVect1DIntFromObject(void *obj, char *idamPath, int idx, int **data, int *dim) { return -1; }
int idamGetVect1DFloatFromObject(void *obj, char *idamPath, int idx, float **data, int *dim) { return -1; }
int idamGetVect1DDoubleFromObject(void *obj, char *idamPath, int idx, double **data, int *dim) { return -1; }
int idamGetVect2DIntFromObject(void *obj, char *idamPath, int idx, int **data, int *dim1, int *dim2) { return -1; }
int idamGetVect2DFloatFromObject(void *obj, char *idamPath, int idx, float **data, int *dim1, int *dim2) { return -1; }
int idamGetVect2DDoubleFromObject(void *obj, char *idamPath, int idx, double **data, int *dim1, int *dim2) { return -1; }
int idamGetVect3DIntFromObject(void *obj, char *idamPath, int idx, int **data, int *dim1, int *dim2, int *dim3) { return -1; }
int idamGetVect3DFloatFromObject(void *obj, char *idamPath, int idx, float **data, int *dim1, int *dim2, int *dim3) { return -1; }
int idamGetVect3DDoubleFromObject(void *obj, char *idamPath, int idx, double **data, int *dim1, int *dim2, int *dim3) { return -1; }
int idamGetVect4DIntFromObject(void *obj, char *idamPath, int idx, int **data, int *dim1, int *dim2, int *dim3, int *dim4) { return -1; }
int idamGetVect4DFloatFromObject(void *obj, char *idamPath, int idx, float **data, int *dim1, int *dim2, int *dim3, int *dim4) { return -1; }
int idamGetVect4DDoubleFromObject(void *obj, char *idamPath, int idx, double **data, int *dim1, int *dim2, int *dim3, int *dim4) { return -1; }
int idamGetVect5DIntFromObject(void *obj, char *idamPath, int idx, int **data, int *dim1, int *dim2, int *dim3, int *dim4, int *dim5) { return -1; }
int idamGetVect5DFloatFromObject(void *obj, char *idamPath, int idx, float **data, int *dim1, int *dim2, int *dim3, int *dim4, int *dim5) { return -1; }
int idamGetVect5DDoubleFromObject(void *obj, char *idamPath, int idx, double **data, int *dim1, int *dim2, int *dim3, int *dim4, int *dim5) { return -1; }
int idamGetVect6DIntFromObject(void *obj, char *idamPath, int idx, int **data, int *dim1, int *dim2, int *dim3, int *dim4, int *dim5, int *dim6) { return -1; }
int idamGetVect6DFloatFromObject(void *obj, char *idamPath, int idx, float **data, int *dim1, int *dim2, int *dim3, int *dim4, int *dim5, int *dim6) { return -1; }
int idamGetVect6DDoubleFromObject(void *obj, char *idamPath, int idx, double **data, int *dim1, int *dim2, int *dim3, int *dim4, int *dim5, int *dim6) { return -1; }
int idamGetVect7DIntFromObject(void *obj, char *idamPath, int idx, int **data, int *dim1, int *dim2, int *dim3, int *dim4, int *dim5, int *dim6, int *dim7) { return -1; }
int idamGetVect7DFloatFromObject(void *obj, char *idamPath, int idx, float **data, int *dim1, int *dim2, int *dim3, int *dim4, int *dim5, int *dim6, int *dim7) { return -1; }
int idamGetVect7DDoubleFromObject(void *obj, char *idamPath, int idx, double **data, int *dim1, int *dim2, int *dim3, int *dim4, int *dim5, int *dim6, int *dim7) { return -1; }
int idamGetObjectFromObject(void *obj, char *idamPath, int idx, void **dataObj) { return -1; }

int idamGetDimensionFromObject(int expIdx, void *obj, char *path, int idx, int *numDims, int *dim1, int *dim2, int *dim3, int *dim4, int *dim5, int *dim6, int *dim7) { return -1; }


//Array of structures Slice Management
int idamGetObjectSlice(int expIdx, char *idamPath, char *cpoPath, double time, void **obj) { return -1; }
int idamPutObjectSlice(int expIdx, char *idamPath, char *cpoPath, double time, void *obj) { return -1; }
int idamReplaceLastObjectSlice(int expIdx, char *cpoPath, char *idamPath, void *obj) { return -1; }

//cpo copy
int idamCopyCpo(int fromIdx, int toIdx, char *cpoName, int fromCpoOccur, int toCpoOccur) { return -1; }

