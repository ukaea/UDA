#include <mdsdescrip.h>
#define INTERPOLATION 3
#define CLOSEST_SAMPLE 1
#define PREVIOUS_SAMPLE 2

extern int mdsimasConnect(char *ip);
extern char *spawnCommand(char *command, char *ipAddress);
extern int mdsimasCreate(char *name, int shot, int run, int refShot, int refRun, int *retIdx);
extern int mdsimasCreateEnv(char *name, int shot, int run, int refShot, int refRun, int *retIdx, char *user, char *tokamak, char *version);
extern int mdsimasOpen(char *name, int shot, int run, int *retIdx);
extern int mdsimasOpenEnv(char *name, int shot, int run, int *retIdx, char *user, char *tokamak, char *version);
extern int mdsimasClose(int idx, char *name, int shot, int run);
extern void disableMemCache();
extern void discardMem(int expIdx, char *cpoPath, char *path);
extern void discardOldMem(int expIdx, char *cpoPath, char *path, double time);
extern void flush(int expIdx, char *cpoPath, char *path);
extern char *mdsLastErrmsg();

extern int mdsPutString(int expIdx, char *cpoPath, char *path, char *data);
extern int mdsPutInt(int expIdx, char *cpoPath, char *path, int data);
extern int mdsPutFloat(int expIdx, char *cpoPath, char *path, float data);
extern int mdsPutDouble(int expIdx, char *cpoPath, char *path, double data);
extern int mdsPutVect1DString(int expIdx, char *cpoPath, char *path, char *timeBasePath, char **data, int dim, int isTimed);
extern int mdsPutVect1DInt(int expIdx, char *cpoPath, char *path, char *timeBasePath, int *data, int dim, int isTimed);
extern int mdsPutVect1DFloat(int expIdx, char *cpoPath, char *path, char *timeBasePath, float *data, int dim, int isTimed);
extern int mdsPutVect1DDouble(int expIdx, char *cpoPath, char *path, char *timeBasePath, double *data, int dim, int isTimed);
extern int mdsPutVect2DInt(int expIdx, char *cpoPath, char *path, char *timeBasePath, int *data, int dim1, int dim2, int isTimed);
extern int mdsPutVect2DFloat(int expIdx, char *cpoPath, char *path, char *timeBasePath, float *data, int dim1, int dim2, int isTimed);
extern int mdsPutVect2DDouble(int expIdx, char *cpoPath, char *path, char *timeBasePath, double *data, int dim1, int dim2, int isTimed);
extern int mdsPutVect3DInt(int expIdx, char *cpoPath, char *path, char *timeBasePath, int *data, int dim1, int dim2, int dim3, int isTimed);
extern int mdsPutVect3DFloat(int expIdx, char *cpoPath, char *path, char *timeBasePath, float *data, int dim1, int dim2, int dim3, int isTimed);
extern int mdsPutVect3DDouble(int expIdx, char *cpoPath, char *path, char *timeBasePath, double *data, int dim1, int dim2, int dim3, int isTimed);
extern int mdsPutVect4DInt(int expIdx, char *cpoPath, char *path, char *timeBasePath, int *data, int dim1, int dim2, int dim3, int dim4, int isTimed);
extern int mdsPutVect4DFloat(int expIdx, char *cpoPath, char *path, char *timeBasePath, float *data, int dim1, int dim2, int dim3, int dim4, int isTimed);
extern int mdsPutVect4DDouble(int expIdx, char *cpoPath, char *path, char *timeBasePath, double *data, int dim1, int dim2, int dim3, int dim4, int isTimed);
extern int mdsPutVect5DInt(int expIdx, char *cpoPath, char *path, char *timeBasePath, int *data, int dim1, int dim2, int dim3, int dim4, int dim5, int isTimed);
extern int mdsPutVect5DFloat(int expIdx, char *cpoPath, char *path, char *timeBasePath, float *data, int dim1, int dim2, int dim3, int dim4, int dim5, int isTimed);
extern int mdsPutVect5DDouble(int expIdx, char *cpoPath, char *path, char *timeBasePath, double *data, int dim1, int dim2, int dim3, int dim4, int dim5, int isTimed);
extern int mdsPutVect6DInt(int expIdx, char *cpoPath, char *path, char *timeBasePath, int *data, int dim1, int dim2, int dim3, int dim4, int dim5, int dim6, int isTimed);
extern int mdsPutVect6DFloat(int expIdx, char *cpoPath, char *path, char *timeBasePath, float *data, int dim1, int dim2, int dim3, int dim4, int dim5, int dim6, int isTimed);
extern int mdsPutVect6DDouble(int expIdx, char *cpoPath, char *path, char *timeBasePath, double *data, int dim1, int dim2, int dim3, int dim4, int dim5, int dim6, int isTimed);
extern int mdsPutVect7DInt(int expIdx, char *cpoPath, char *path, char *timeBasePath, int *data, int dim1, int dim2, int dim3, int dim4, int dim5, int dim6, int dim7, int isTimed);
extern int mdsPutVect7DFloat(int expIdx, char *cpoPath, char *path, char *timeBasePath, float *data, int dim1, int dim2, int dim3, int dim4, int dim5, int dim6, int dim7, int isTimed);
extern int mdsPutVect7DDouble(int expIdx, char *cpoPath, char *path, char *timeBasePath, double *data, int dim1, int dim2, int dim3, int dim4, int dim5, int dim6, int dim7, int isTimed);
extern int mdsPutIntSlice(int expIdx, char *cpoPath, char *path, char *timeBasePath, int data, double time);
extern int mdsPutFloatSlice(int expIdx, char *cpoPath, char *path, char *timeBasePath, float data, double time);
extern int mdsPutDoubleSlice(int expIdx, char *cpoPath, char *path, char *timeBasePath, double data, double time);
extern int mdsPutStringSlice(int expIdx, char *cpoPath, char *path, char *timeBasePath, char *data, double time);
extern int mdsPutVect1DIntSlice(int expIdx, char *cpoPath, char *path, char *timeBasePath, int *data, int dim, double time);
extern int mdsPutVect1DFloatSlice(int expIdx, char *cpoPath, char *path, char *timeBasePath, float *data, int dim, double time);
extern int mdsPutVect1DDoubleSlice(int expIdx, char *cpoPath, char *path, char *timeBasePath, double *data, int dim, double time);
extern int mdsPutVect2DIntSlice(int expIdx, char *cpoPath, char *path, char *timeBasePath, int *data, int dim1, int dim2, double time);
extern int mdsPutVect2DFloatSlice(int expIdx, char *cpoPath, char *path, char *timeBasePath, float *data, int dim1, int dim2, double time);
extern int mdsPutVect2DDoubleSlice(int expIdx, char *cpoPath, char *path, char *timeBasePath, double *data, int dim1, int dim2, double time);
extern int mdsPutVect3DIntSlice(int expIdx, char *cpoPath, char *path, char *timeBasePath, int *data, int dim1, int dim2, int dim3, double time);
extern int mdsPutVect3DFloatSlice(int expIdx, char *cpoPath, char *path, char *timeBasePath, float *data, int dim1, int dim2, int dim3, double time);
extern int mdsPutVect3DDoubleSlice(int expIdx, char *cpoPath, char *path, char *timeBasePath, double *data, int dim1, int dim2, int dim3, double time);
extern int mdsPutVect4DIntSlice(int expIdx, char *cpoPath, char *path, char *timeBasePath, int *data, int dim1, int dim2, int dim3, int dim4, double time);
extern int mdsPutVect4DFloatSlice(int expIdx, char *cpoPath, char *path, char *timeBasePath, float *data, int dim1, int dim2, int dim3, int dim4, double time);
extern int mdsPutVect4DDoubleSlice(int expIdx, char *cpoPath, char *path, char *timeBasePath, double *data, int dim1, int dim2, int dim3, int dim4, double time);
extern int mdsPutVect5DIntSlice(int expIdx, char *cpoPath, char *path, char *timeBasePath, int *data, int dim1, int dim2, int dim3, int dim4, int dim5, double time);
extern int mdsPutVect5DFloatSlice(int expIdx, char *cpoPath, char *path, char *timeBasePath, float *data, int dim1, int dim2, int dim3, int dim4, int dim5, double time);
extern int mdsPutVect5DDoubleSlice(int expIdx, char *cpoPath, char *path, char *timeBasePath, double *data, int dim1, int dim2, int dim3, int dim4, int dim5, double time);
extern int mdsPutVect6DIntSlice(int expIdx, char *cpoPath, char *path, char *timeBasePath, int *data, int dim1, int dim2, int dim3, int dim4, int dim5, int dim6, double time);
extern int mdsPutVect6DFloatSlice(int expIdx, char *cpoPath, char *path, char *timeBasePath, float *data, int dim1, int dim2, int dim3, int dim4, int dim5, int dim6, double time);
extern int mdsPutVect6DDoubleSlice(int expIdx, char *cpoPath, char *path, char *timeBasePath, double *data, int dim1, int dim2, int dim3, int dim4, int dim5, int dim6, double time);

extern int mdsReplaceLastIntSlice(int expIdx, char *cpoPath, char *path, int data);
extern int mdsReplaceLastFloatSlice(int expIdx, char *cpoPath, char *path, float data);
extern int mdsReplaceLastDoubleSlice(int expIdx, char *cpoPath, char *path, double data);
extern int mdsReplaceLastStringSlice(int expIdx, char *cpoPath, char *path, char *data);
extern int mdsReplaceLastVect1DIntSlice(int expIdx, char *cpoPath, char *path, int *data, int dim);
extern int mdsReplaceLastVect1DFloatSlice(int expIdx, char *cpoPath, char *path, float *data, int dim);
extern int mdsReplaceLastVect1DDoubleSlice(int expIdx, char *cpoPath, char *path, double *data, int dim);
extern int mdsReplaceLastVect2DIntSlice(int expIdx, char *cpoPath, char *path, int *data, int dim1, int dim2);
extern int mdsReplaceLastVect2DFloatSlice(int expIdx, char *cpoPath, char *path, float *data, int dim1, int dim2);
extern int mdsReplaceLastVect2DDoubleSlice(int expIdx, char *cpoPath, char *path, double *data, int dim1, int dim2);
extern int mdsReplaceLastVect3DIntSlice(int expIdx, char *cpoPath, char *path, int *data, int dim1, int dim2, int dim3);
extern int mdsReplaceLastVect3DFloatSlice(int expIdx, char *cpoPath, char *path, float *data, int dim1, int dim2, int dim3);
extern int mdsReplaceLastVect3DDoubleSlice(int expIdx, char *cpoPath, char *path, double *data, int dim1, int dim2, int dim3);
extern int mdsReplaceLastVect4DIntSlice(int expIdx, char *cpoPath, char *path, int *data, int dim1, int dim2, int dim3, int dim4);
extern int mdsReplaceLastVect4DFloatSlice(int expIdx, char *cpoPath, char *path, float *data, int dim1, int dim2, int dim3, int dim4);
extern int mdsReplaceLastVect4DDoubleSlice(int expIdx, char *cpoPath, char *path, double *data, int dim1, int dim2, int dim3, int dim4);
extern int mdsReplaceLastVect5DIntSlice(int expIdx, char *cpoPath, char *path, int *data, int dim1, int dim2, int dim3, int dim4, int dim5);
extern int mdsReplaceLastVect5DFloatSlice(int expIdx, char *cpoPath, char *path, float *data, int dim1, int dim2, int dim3, int dim4, int dim5);
extern int mdsReplaceLastVect5DDoubleSlice(int expIdx, char *cpoPath, char *path, double *data, int dim1, int dim2, int dim3, int dim4, int dim5);
extern int mdsReplaceLastVect6DIntSlice(int expIdx, char *cpoPath, char *path, int *data, int dim1, int dim2, int dim3, int dim4, int dim5, int dim6);
extern int mdsReplaceLastVect6DFloatSlice(int expIdx, char *cpoPath, char *path, float *data, int dim1, int dim2, int dim3, int dim4, int dim5, int dim6);
extern int mdsReplaceLastVect6DDoubleSlice(int expIdx, char *cpoPath, char *path, double *data, int dim1, int dim2, int dim3, int dim4, int dim5, int dim6);

extern int mdsGetString(int expIdx, char *cpoPath, char *path, char **data);
extern int mdsGetFloat(int expIdx, char *cpoPath, char *path, float *data);
extern int mdsGetInt(int expIdx, char *cpoPath, char *path, int *data);
extern int mdsGetDouble(int expIdx, char *cpoPath, char *path, double *data);
extern int mdsGetVect1DString(int expIdx, char *cpoPath, char *path, char  ***data, int *dim);
extern int mdsGetVect1DInt(int expIdx, char *cpoPath, char *path, int **data, int *dim);
extern int mdsGetVect1DFloat(int expIdx, char *cpoPath, char *path, float **data, int *dim);
extern int mdsGetVect1DDouble(int expIdx, char *cpoPath, char *path, double **data, int *dim);
extern int mdsGetVect2DInt(int expIdx, char *cpoPath, char *path, int **data, int *dim1, int *dim2);
extern int mdsGetVect2DFloat(int expIdx, char *cpoPath, char *path, float **data, int *dim1, int *dim2);
extern int mdsGetVect2DDouble(int expIdx, char *cpoPath, char *path, double **data, int *dim1, int *dim2);
extern int mdsGetVect3DInt(int expIdx, char *cpoPath, char *path, int **data, int *dim1, int *dim2, int *dim3);
extern int mdsGetVect3DFloat(int expIdx, char *cpoPath, char *path, float **data, int *dim1, int *dim2, int *dim3);
extern int mdsGetVect3DDouble(int expIdx, char *cpoPath, char *path, double **data, int *dim1, int *dim2, int *dim3);
extern int mdsGetVect4DInt(int expIdx, char *cpoPath, char *path, int **data, int *dim1, int *dim2, int *dim3, int *dim4);
extern int mdsGetVect4DFloat(int expIdx, char *cpoPath, char *path, float **data, int *dim1, int *dim2, int *dim3, int *dim4);
extern int mdsGetVect4DDouble(int expIdx, char *cpoPath, char *path, double **data, int *dim1, int *dim2, int *dim3, int *dim4);
extern int mdsGetVect5DInt(int expIdx, char *cpoPath, char *path, int **data, int *dim1, int *dim2, int *dim3, int *dim4, int *dim5);
extern int mdsGetVect5DFloat(int expIdx, char *cpoPath, char *path, float **data, int *dim1, int *dim2, int *dim3, int *dim4, int *dim5);
extern int mdsGetVect5DDouble(int expIdx, char *cpoPath, char *path, double **data, int *dim1, int *dim2, int *dim3, int *dim4, int *dim5);
extern int mdsGetVect6DInt(int expIdx, char *cpoPath, char *path, int **data, int *dim1, int *dim2, int *dim3, int *dim4, int *dim5, int *dim6);
extern int mdsGetVect6DFloat(int expIdx, char *cpoPath, char *path, float **data, int *dim1, int *dim2, int *dim3, int *dim4, int *dim5, int *dim6);
extern int mdsGetVect6DDouble(int expIdx, char *cpoPath, char *path, double **data, int *dim1, int *dim2, int *dim3, int *dim4, int *dim5, int *dim6);
extern int mdsGetVect7DInt(int expIdx, char *cpoPath, char *path, int **data, int *dim1, int *dim2, int *dim3, int *dim4, int *dim5, int *dim6, int *dim7);
extern int mdsGetVect7DFloat(int expIdx, char *cpoPath, char *path, float **data, int *dim1, int *dim2, int *dim3, int *dim4, int *dim5, int *dim6, int *dim7);
extern int mdsGetVect7DDouble(int expIdx, char *cpoPath, char *path, double **data, int *dim1, int *dim2, int *dim3, int *dim4, int *dim5, int *dim6, int *dim7);


extern int mdsGetStringSlice(int expIdx, char *cpoPath, char *path, char *timeBasePath, char **data, double time, double *retTime, int interpolMode);
extern int mdsGetFloatSlice(int expIdx, char *cpoPath, char *path, char *timeBasePath, float *data, double time, double *retTime, int interpolMode);
extern int mdsGetIntSlice(int expIdx, char *cpoPath, char *path, char *timeBasePath, int *data, double time, double *retTime, int interpolMode);
extern int mdsGetStringSlice(int expIdx, char *cpoPath, char *path, char *timeBasePath, char **data, double time, double *retTime, int interpolMode);
extern int mdsGetDoubleSlice(int expIdx, char *cpoPath, char *path, char *timeBasePath, double *data, double time, double *retTime, int interpolMode);
extern int mdsGetVect1DIntSlice(int expIdx, char *cpoPath, char *path, char *timeBasePath, int **data, int *dim, double time, double *retTime, int interpolMode);
extern int mdsGetVect1DFloatSlice(int expIdx, char *cpoPath, char *path, char *timeBasePath, float **data, int *dim, double time, double *retTime, int interpolMode);
extern int mdsGetVect1DDoubleSlice(int expIdx, char *cpoPath, char *path, char *timeBasePath, double **data, int *dim, double time, double *retTime, int interpolMode);
extern int mdsGetVect2DIntSlice(int expIdx, char *cpoPath, char *path, char *timeBasePath, int **data, int *dim1, int *dim2, double time, double *retTime, int interpolMode);
extern int mdsGetVect2DFloatSlice(int expIdx, char *cpoPath, char *path, char *timeBasePath, float **data, int *dim1, int *dim2, double time, double *retTime, int interpolMode);
extern int mdsGetVect2DDoubleSlice(int expIdx, char *cpoPath, char *path, char *timeBasePath, double **data, int *dim1, int *dim2, double time, double *retTime, int interpolMode);
extern int mdsGetVect3DIntSlice(int expIdx, char *cpoPath, char *path, char *timeBasePath, int **data, int *dim1, int *dim2, int *dim3, double time, double *retTime, int interpolMode);
extern int mdsGetVect3DFloatSlice(int expIdx, char *cpoPath, char *path, char *timeBasePath, float **data, int *dim1, int *dim2, int *dim3, double time, double *retTime, int interpolMode);
extern int mdsGetVect3DDoubleSlice(int expIdx, char *cpoPath, char *path, char *timeBasePath, double **data, int *dim1, int *dim2, int *dim3, double time, double *retTime, int interpolMode);
extern int mdsGetVect4DIntSlice(int expIdx, char *cpoPath, char *path, char *timeBasePath, int **data, int *dim1, int *dim2, int *dim3, int *dim4, double time, double *retTime, int interpolMode);
extern int mdsGetVect4DFloatSlice(int expIdx, char *cpoPath, char *path, char *timeBasePath, float **data, int *dim1, int *dim2, int *dim3, int *dim4, double time, double *retTime, int interpolMode);
extern int mdsGetVect4DDoubleSlice(int expIdx, char *cpoPath, char *path, char *timeBasePath, double **data, int *dim1, int *dim2, int *dim3, int *dim4, double time, double *retTime, int interpolMode);
extern int mdsGetVect5DIntSlice(int expIdx, char *cpoPath, char *path, char *timeBasePath, int **data, int *dim1, int *dim2, int *dim3, int *dim4, int *dim5, double time, double *retTime, int interpolMode);
extern int mdsGetVect5DFloatSlice(int expIdx, char *cpoPath, char *path, char *timeBasePath, float **data, int *dim1, int *dim2, int *dim3, int *dim4, int *dim5, double time, double *retTime, int interpolMode);
extern int mdsGetVect5DDoubleSlice(int expIdx, char *cpoPath, char *path, char *timeBasePath, double **data, int *dim1, int *dim2, int *dim3, int *dim4, int *dim5, double time, double *retTime, int interpolMode);
extern int mdsGetVect6DIntSlice(int expIdx, char *cpoPath, char *path, char *timeBasePath, int **data, int *dim1, int *dim2, int *dim3, int *dim4, int *dim5, int *dim6, double time, double *retTime, int interpolMode);
extern int mdsGetVect6DFloatSlice(int expIdx, char *cpoPath, char *path, char *timeBasePath, float **data, int *dim1, int *dim2, int *dim3, int *dim4, int *dim5, int *dim6, double time, double *retTime, int interpolMode);
extern int mdsGetVect6DDoubleSlice(int expIdx, char *cpoPath, char *path, char *timeBasePath, double **data, int *dim1, int *dim2, int *dim3, int *dim4, int *dim5, int *dim6, double time, double *retTime, int interpolMode);

extern int mdsDeleteData(int expIdx, char *cpoPath, char *path);
extern int getUniqueRun(int shot);

extern int mdsbeginIdsGet(int expIdx, char *path, int isTimed, int *retSamples);
extern int mdsendIdsGet(int expIdx, char *path);
extern int mdsbeginIdsGetResampled(int expIdx, char *path, double start, double end, double delta, int *retSamples);
extern int mdsendIdsGetResampled(int expIdx, char *path);
extern int mdsbeginIdsGetSlice(int expIdx, char *path, double time);
extern int mdsendIdsGetSlice(int expIdx, char *path);
extern int mdsbeginIdsPut(int expIdx, char *path);
extern int mdsendIdsPut(int expIdx, char *path);
extern int mdsbeginIdsPutTimed(int expIdx, char *path, int samples, double *inTimes);
extern int mdsendIdsPutTimed(int expIdx, char *path);
extern int mdsbeginIdsPutNonTimed(int expIdx, char *path);
extern int mdsendIdsPutNonTimed(int expIdx, char *path);
extern int mdsbeginIdsPutSlice(int expIdx, char *path);
extern int mdsendIdsPutSlice(int expIdx, char *path);
extern int mdsbeginIdsReplaceLastSlice(int expIdx, char *path);
extern int mdsendIdsReplaceLastSlice(int expIdx, char *path);

int getData(int expIdx, char *cpoPath, char *path, struct descriptor_xd *retXd, int evaluate);
int getSlicedData(int expIdx, char *cpoPath, char *path, char *timeBasePath, double time, struct descriptor_xd *retDataXd, 
    struct descriptor_xd *retTimesXd, int expandObj);
int putSegment(int expIdx, char *cpoPath, char *path, char *timeBasePath, struct descriptor_a *dataD, double *times, int nTimes);
int putData(int expIdx, char *cpoPath, char *path, struct descriptor *dataD);
int putSlice(int expIdx, char *cpoPath, char *path, char *timeBasePath, struct descriptor *dataD, double time);
int replaceLastSlice(int expIdx, char *cpoPath, char *path, struct descriptor *dataD);
void mdsDeleteAllFields(int expIdx, char *cpoPath);
int mdsIsSliced(int expIdx, char *cpoPath, char *path);


//------------------ Array of structures stuff -----------------

//Initialize room for a generic array of structures
void *mdsBeginObject();

//Releases memory for array of structures
void mdsReleaseObject(void *obj);

//Writes an array of objects array of structures
//Flag isTimed specifies whether the array refers to time-dependent field (in this case the dimension refers to time)
int mdsPutObject(int expIdx, char *cpoPath, char *mdsPath, void *obj, int isTimed);

//Add elements to the structure array. Note: returns the new pointer to the object, possibly chjanged (if object reallocated)
//Argument path refers to the path name within the structure
//Argument idx is the index in the array of structues
void *mdsPutIntInObject(void *obj, char *mdsPath, int idx, int data);
void *mdsPutStringInObject(void *obj, char *mdsPath, int idx, char *data);
void *mdsPutFloatInObject(void *obj, char *mdsPath, int idx, float data);
void *mdsPutDoubleInObject(void *obj, char *mdsPath, int idx, double data);
void *mdsPutVect1DStringInObject(void *obj, char *mdsPath, int idx, char **data, int dim);
void *mdsPutVect1DIntInObject(void *obj, char *mdsPath, int idx, int *data, int dim);
void *mdsPutVect1DFloatInObject(void *obj, char *mdsPath, int idx, float *data, int dim);
void *mdsPutVect1DDoubleInObject(void *obj, char *mdsPath, int idx, double *data, int dim);
void *mdsPutVect2DIntInObject(void *obj, char *mdsPath, int idx, int *data, int dim1, int dim2);
void *mdsPutVect2DFloatInObject(void *obj, char *mdsPath, int idx, float *data, int dim1, int dim2);
void *mdsPutVect2DDoubleInObject(void *obj, char *mdsPath, int idx, double *data, int dim1, int dim2);
void *mdsPutVect3DIntInObject(void *obj, char *mdsPath, int idx, int *data, int dim1, int dim2, int dim3);
void *mdsPutVect3DFloatInObject(void *obj, char *mdsPath, int idx, float *data, int dim1, int dim2, int dim3);
void *mdsPutVect3DDoubleInObject(void *obj, char *mdsPath, int idx, double *data, int dim1, int dim2, int dim3);
void *mdsPutVect4DIntInObject(void *obj, char *mdsPath, int idx, int *data, int dim1, int dim2, int dim3, int dim4);
void *mdsPutVect4DFloatInObject(void *obj, char *mdsPath, int idx, float *data, int dim1, int dim2, int dim3, int dim4);
void *mdsPutVect4DDoubleInObject(void *obj, char *mdsPath, int idx, double *data, int dim1, int dim2, int dim3, int dim4);
void *mdsPutVect5DIntInObject(void *obj, char *mdsPath, int idx, int *data, int dim1, int dim2, int dim3, int dim4, int dim5);
void *mdsPutVect5DFloatInObject(void *obj, char *mdsPath, int idx, float *data, int dim1, int dim2, int dim3, int dim4,int dim5);
void *mdsPutVect5DDoubleInObject(void *obj, char *mdsPath, int idx, double *data, int dim1, int dim2, int dim3, int dim4,int dim5);
void *mdsPutVect6DIntInObject(void *obj, char *mdsPath, int idx, int *data, int dim1, int dim2, int dim3, int dim4, int dim5, int dim6);
void *mdsPutVect6DFloatInObject(void *obj, char *mdsPath, int idx, float *data, int dim1, int dim2, int dim3, int dim4,int dim5, int dim6);
void *mdsPutVect6DDoubleInObject(void *obj, char *mdsPath, int idx, double *data, int dim1, int dim2, int dim3, int dim4,int dim5, int dim6);
void *mdsPutVect7DIntInObject(void *obj, char *mdsPath, int idx, int *data, int dim1, int dim2, int dim3, int dim4, int dim5, int dim6, int dim7);
void *mdsPutVect7DFloatInObject(void *obj, char *mdsPath, int idx, float *data, int dim1, int dim2, int dim3, int dim4,int dim5, int dim6, int dim7);
void *mdsPutVect7DDoubleInObject(void *obj, char *mdsPath, int idx, double *data, int dim1, int dim2, int dim3, int dim4,int dim5, int dim6, int dim7);
void *mdsPutObjectInObject(void *obj, char *mdsPath, int idx, void *dataObj);

//Retrieve the number of elements for the array of structures. Returns -1 if an error occurs;
int mdsGetObjectDim(void *obj);

//Read the array of structures from the pulse file. Status indicates as always success (0) or error (!= 0)
int mdsGetObject(int expIdx, char *mdsPath, char *cpoPath, void **obj, int isTimed);

//Retrieves components from array of structures. Returned status indicates success (0) or error(!= 0)
int mdsGetStringFromObject(void *obj, char *mdsPath, int idx, char **data);
int mdsGetFloatFromObject(void *obj, char *mdsPath, int idx, float *data);
int mdsGetIntFromObject(void *obj, char *mdsPath, int idx, int *data);
int mdsGetDoubleFromObject(void *obj, char *mdsPath, int idx, double *data);
int mdsGetVect1DStringFromObject(void *obj, char *mdsPath, int idx, char  ***data, int *dim);
int mdsGetVect1DIntFromObject(void *obj, char *mdsPath, int idx, int **data, int *dim);
int mdsGetVect1DFloatFromObject(void *obj, char *mdsPath, int idx, float **data, int *dim);
int mdsGetVect1DDoubleFromObject(void *obj, char *mdsPath, int idx, double **data, int *dim);
int mdsGetVect2DIntFromObject(void *obj, char *mdsPath, int idx, int **data, int *dim1, int *dim2);
int mdsGetVect2DFloatFromObject(void *obj, char *mdsPath, int idx, float **data, int *dim1, int *dim2);
int mdsGetVect2DDoubleFromObject(void *obj, char *mdsPath, int idx, double **data, int *dim1, int *dim2);
int mdsGetVect3DIntFromObject(void *obj, char *mdsPath, int idx, int **data, int *dim1, int *dim2, int *dim3);
int mdsGetVect3DFloatFromObject(void *obj, char *mdsPath, int idx, float **data, int *dim1, int *dim2, int *dim3);
int mdsGetVect3DDoubleFromObject(void *obj, char *mdsPath, int idx, double **data, int *dim1, int *dim2, int *dim3);
int mdsGetVect4DIntFromObject(void *obj, char *mdsPath, int idx, int **data, int *dim1, int *dim2, int *dim3, int *dim4);
int mdsGetVect4DFloatFromObject(void *obj, char *mdsPath, int idx, float **data, int *dim1, int *dim2, int *dim3, int *dim4);
int mdsGetVect4DDoubleFromObject(void *obj, char *mdsPath, int idx, double **data, int *dim1, int *dim2, int *dim3, int *dim4);
int mdsGetVect5DIntFromObject(void *obj, char *mdsPath, int idx, int **data, int *dim1, int *dim2, int *dim3, int *dim4, int *dim5);
int mdsGetVect5DFloatFromObject(void *obj, char *mdsPath, int idx, float **data, int *dim1, int *dim2, int *dim3, int *dim4, int *dim5);
int mdsGetVect5DDoubleFromObject(void *obj, char *mdsPath, int idx, double **data, int *dim1, int *dim2, int *dim3, int *dim4, int *dim5);
int mdsGetVect6DIntFromObject(void *obj, char *mdsPath, int idx, int **data, int *dim1, int *dim2, int *dim3, int *dim4, int *dim5, int *dim6);
int mdsGetVect6DFloatFromObject(void *obj, char *mdsPath, int idx, float **data, int *dim1, int *dim2, int *dim3, int *dim4, int *dim5, int *dim6);
int mdsGetVect6DDoubleFromObject(void *obj, char *mdsPath, int idx, double **data, int *dim1, int *dim2, int *dim3, int *dim4, int *dim5, int *dim6);
int mdsGetVect7DIntFromObject(void *obj, char *mdsPath, int idx, int **data, int *dim1, int *dim2, int *dim3, int *dim4, int *dim5, int *dim6, int *dim7);
int mdsGetVect7DFloatFromObject(void *obj, char *mdsPath, int idx, float **data, int *dim1, int *dim2, int *dim3, int *dim4, int *dim5, int *dim6, int *dim7);
int mdsGetVect7DDoubleFromObject(void *obj, char *mdsPath, int idx, double **data, int *dim1, int *dim2, int *dim3, int *dim4, int *dim5, int *dim6, int *dim7);
int mdsGetObjectFromObject(void *obj, char *mdsPath, int idx, void **dataObj);

int mdsGetDimensionFromObject(int expIdx, void *obj, char *path, int idx, int *numDims, int *dim1, int *dim2, int *dim3, int *dim4, int *dim5, int *dim6, int *dim7);


//Array of structures Slice Management
int mdsGetObjectSlice(int expIdx, char *mdsPath, char *cpoPath, double time, void **obj);
int mdsPutObjectSlice(int expIdx, char *mdsPath, char *cpoPath, double time, void *obj);
int mdsReplaceLastObjectSlice(int expIdx, char *cpoPath, char *mdsPath, void *obj);

//cpo copy
int mdsCopyCpo(int fromIdx, int toIdx, char *cpoName, int fromCpoOccur, int toCpoOccur);

