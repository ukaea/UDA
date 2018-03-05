#include <treeshr.h>
#include <xtreeshr.h>
#include <mdsshr.h>
#include <mdsdescrip.h>
#include <mds_stdarg.h>
#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include <stdint.h>
#include <usagedef.h>
#include <ncidef.h>

#include <xtreeshr.h>
#include <cacheshr.h>
#include <pthread.h>
#include <semaphore.h>
#include <fcntl.h>
#include "ual_low_level_mdsplus.h"
#include "ual_low_level_remote.h"
extern int TdiData();



#define INTERPOLATION 3
#define CLOSEST_SAMPLE 1
#define PREVIOUS_SAMPLE 2

#define DEFAULT_SEGMENT_ROWS 100

#ifndef WRITE_BACK
#define WRITE_BACK MDS_WRITE_BACK
#endif
static int getDataLocal(int expIdx, char *cpoPath, char *path, struct descriptor_xd *retXd, int evaluate);
static int mdsgetDataLocal(int expIdx, char *cpoPath, char *path, struct descriptor_xd *retXd, int evaluate);
static int getDataLocalNoDescr(int expIdx, char *cpoPath, char *path, void **retData, int *retDims,  int *retDimsCt, char *retType, char *retClass, int *retLength, int64_t *retArsize, int evaluate);
static int getSlicedDataLocal(int expIdx, char *cpoPath, char *path, double time, struct descriptor_xd *retDataXd, struct descriptor_xd *retTimesXd, int expand);
static int putSegmentLocal(int expIdx, char *cpoPath, char *path, char *timeBasePath, struct descriptor_a *dataD, double *times, int nTimes);
static int putDataLocal(int expIdx, char *cpoPath, char *path, struct descriptor *dataD);
static int mdsputDataLocal(int expIdx, char *cpoPath, char *path, struct descriptor *dataD);
static int putSliceLocal(int expIdx, char *cpoPath, char *path, char *timeBasePath, struct descriptor *dataD, double time);
static int mdsputSliceLocal(int expIdx, char *cpoPath, char *path, char *timeBasePath, struct descriptor *dataD, double time);
static int replaceLastSliceLocal(int expIdx, char *cpoPath, char *path, struct descriptor *dataD);
static int getObjectSliceLocal(int expIdx, char *cpoPath, char *path,  double time, void **obj, int expand);


static int putTimedVect1DInt(int expIdx, char *cpoPath, char *path, char *timeBasePath, int *data, double *times, int nTimes);
static int putTimedVect1DFloat(int expIdx, char *cpoPath, char *path, char *timeBasePath, float *data, double *times, int nTimes);
static int putTimedVect1DDouble(int expIdx, char *cpoPath, char *path, char *timeBasePath, double *data, double *times, int nTimes);
static int putTimedVect1DString(int expIdx, char *cpoPath, char *path, char *timeBasePath, char **data, double *times, int nTimes);
static int putTimedVect2DInt(int expIdx, char *cpoPath, char *path, char *timeBasePath, int *data, double *times, int nTimes, int dim1);
static int putTimedVect2DFloat(int expIdx, char *cpoPath, char *path, char *timeBasePath, float *data, double *times, int nTimes, int dim1);
static int putTimedVect2DDouble(int expIdx, char *cpoPath, char *path, char *timeBasePath, double *data, double *times, int nTimes, int dim1);
static int putTimedVect3DInt(int expIdx, char *cpoPath, char *path, char *timeBasePath, int *data, double *times, int nTimes, int dim1, int dim2);
static int putTimedVect3DFloat(int expIdx, char *cpoPath, char *path, char *timeBasePath, float *data, double *times, int nTimes, int dim1, int dim2);
static int putTimedVect3DDouble(int expIdx, char *cpoPath, char *path, char *timeBasePath, double *data, double *times, int nTimes, int dim1, int dim2);
static int putTimedVect4DInt(int expIdx, char *cpoPath, char *path, char *timeBasePath, int *data, double *times, int nTimes, int dim1, int dim2, int dim3);
static int putTimedVect4DFloat(int expIdx, char *cpoPath, char *path, char *timeBasePath, float *data, double *times, int nTimes, int dim1, int dim2, int dim3);
static int putTimedVect4DDouble(int expIdx, char *cpoPath, char *path, char *timeBasePath, double *data, double *times, int nTimes, int dim1, int dim2, int dim3);
static int putTimedVect5DInt(int expIdx, char *cpoPath, char *path, char *timeBasePath, int *data, double *times, int nTimes, int dim1, int dim2, int dim3, int dim4);
static int putTimedVect5DFloat(int expIdx, char *cpoPath, char *path, char *timeBasePath, float *data, double *times, int nTimes, int dim1, int dim2, int dim3, int dim4);
static int putTimedVect5DDouble(int expIdx, char *cpoPath, char *path, char *timeBasePath, double *data, double *times, int nTimes, int dim1, int dim2, int dim3, int dim4);
static int putTimedVect6DInt(int expIdx, char *cpoPath, char *path, char *timeBasePath, int *data, double *times, int nTimes, int dim1, int dim2, int dim3, int dim4, int dim5);
static int putTimedVect6DFloat(int expIdx, char *cpoPath, char *path, char *timeBasePath, float *data, double *times, int nTimes, int dim1, int dim2, int dim3, int dim4, int dim5);
static int putTimedVect6DDouble(int expIdx, char *cpoPath, char *path, char *timeBasePath, double *data, double *times, int nTimes, int dim1, int dim2, int dim3, int dim4, int dim5);
static int putTimedVect7DInt(int expIdx, char *cpoPath, char *path, char *timeBasePath, int *data, double *times, int nTimes, int dim1, int dim2,
int dim3, int dim4, int dim5, int dim6);
static int putTimedVect7DFloat(int expIdx, char *cpoPath, char *path, char *timeBasePath, float *data, double *times, int nTimes, int dim1,
int dim2, int dim3, int dim4, int dim5, int dim6);
static int putTimedVect7DDouble(int expIdx, char *cpoPath, char *path, char *timeBasePath, double *data, double *times, int nTimes, int dim1,
int dim2, int dim3, int dim4, int dim5, int dim6);
static int getTimedDataNoDescr(int expIdx, char *cpoPath, char *path, double start, double end, void **retData, int *retDims,  int *retDimsCt, char *retType, char *retClass, int *retLength, int64_t *retArsize,
    struct descriptor_xd *retTimesXd, int all);

static int getNid(char *cpoPath, char *path);
static int mdsgetNid(char *cpoPath, char *path);
static int putObjectSegmentLocal(int expIdx, char *cpoPath, char *path, void *objSegment, int segIdx);
static int putTimedObjectLocal(int expIdx, char *cpoPath, char *path, void **objSlices, int numSlices);
static int getObjectLocal(int expIdx, char *cpoPath, char *path,  void **obj, int isTimed, int expand);
static void dumpObjElement(struct descriptor_a *apd, int numSpaces);
static int mdsgetObjectLocal(int expIdx, char *cpoPath, char *path,  void **obj, int isTimed, int expand);



//Status reporting:
//0: success
//!= 0 error index. Routine imas_last_errmsg returns the error message associated with the last error.
extern char *errmsg;
static char currExpName[4096];

//Time array used for resampled get and sliced get ONLY FOR STRINGS
static double *stringTimes;
static int nStringTimes;

//Time array used for put timed
static double *putTimes;
static int nPutTimes;

#ifdef MONITOR
/////Cache Monitor management
extern void startCacheMonitor(char *name);
static int cacheMonitorStarted = 0;
static void checkCacheMonitor()
{
    if(!cacheMonitorStarted)
    {
        cacheMonitorStarted = 1;
	startCacheMonitor("ids");
    }
}
/////////////////////////////
#endif

//////////////////////////////////////////////TESTS

//#define DEBUG_UAL
void reportInfo(char *str1, char *str2)
{
#ifdef DEBUG_UAL
    FILE *f = fopen("ual.txt", "a");
    fprintf(f, str1, str2);
    fclose(f);
#endif
}

////////////Lock stuff///////////////

static pthread_mutex_t mutex = PTHREAD_MUTEX_INITIALIZER;
//static int initialized = 0;
static void lock(char *name)
{
    pthread_mutex_lock(&mutex);
}
static void unlock()
{
    pthread_mutex_unlock(&mutex);
//reportInfo("UNLOCK\n", "");
}
static pthread_mutex_t mutexCpo = PTHREAD_MUTEX_INITIALIZER;
//static int initializedCpo = 0;
static void lockIds()
{
//reportInfo("LOCK IDS\n", "");
    pthread_mutex_lock(&mutexCpo);
//reportInfo("LOCKED IDS\n", "");
}
static void unlockIds()
{
    pthread_mutex_unlock(&mutexCpo);
//reportInfo("IDS UNLOCK\n", "");
}



///////////////////////////////////////////////


//Idx used for sliced get
//static int sliceIdx1 = -1, sliceIdx2 = -1;
//static double sliceTime1, sliceTime2;

static int currMdsShot;

static int currConnectionId;

//Multiple experiment support
#define MAX_EXPERIMENTS 512
static int currExperimentIdx = -1;

static struct {
    char *name;
    int shot;
    void *ctx;
    int isRemote;
    int connectionId;
    int remoteIdx;
    int refCount;
    int cacheLevel;
} openExperimentInfo[MAX_EXPERIMENTS];

static int isExpRemote(int id)
{
    return openExperimentInfo[id].isRemote;
}

static int getExpConnectionId(int id)
{
    return openExperimentInfo[id].connectionId;
}

static int getExpRemoteIdx(int id)
{
    return openExperimentInfo[id].remoteIdx;
}

//Time array to be used when writing timed fields
//static double *times;
//static int numTimes;

//Memory mappede flas
static void getLeftItems(int nid, int *leftItems, int *leftRows);
static int mdsStatus;

///////Local-remote stuff
static int mdsbeginIdsPutSliceLocal(int expIdx, char *path);

static int mdsendIdsPutSliceLocal(int expIdx, char *path);
static int mdsbeginIdsReplaceLastSliceLocal(int expIdx, char *path);
static int mdsendIdsReplaceLastSliceLocal(int expIdx, char *path);
static int mdsbeginIdsPutLocal(int expIdx, char *path);
static int mdsendIdsPutLocal(int expIdx, char *path);
static int mdsbeginIdsPutTimedLocal(int expIdx, char *path, int samples, double *inTimes);
static int mdsendIdsPutTimedLocal(int expIdx, char *path);
static int mdsbeginIdsPutNonTimedLocal(int expIdx, char *path);
static int mdsendIdsPutNonTimedLocal(int expIdx, char *path);
static int mdsbeginIdsGetLocal(int expIdx, char *path, int isTimed, int *retSamples);
static int mdsendIdsGetLocal(int expIdx, char *path);
static int mdsbeginIdsGetSliceLocal(int expIdx, char *path, double time);
static int mdsendIdsGetSliceLocal(int expIdx, char *path);

///////////////Memory Based Dimension/existence information////////////
extern void putInfo(int expIdx, char *cpoPath, char *path, int exists, int nDims, int *dims);
extern int getInfo(int expIdx, char *cpoPath, char *path, int *exists, int *nDims, int *dims);
extern void invalidateCpoInfo(int expIdx, char *cpoPath);
extern void invalidateAllCpoInfos(int expIdx);
extern void invalidateCpoField(int expIdx, char *cpoPath, char *path);
extern void putInfoWithData(int expIdx, char *cpoPath, char *path, int exists, int nDims, int *dims, int itemSize, char type, int dataSize, char *data, int isObject);
extern int getInfoWithData(int expIdx, char *cpoPath, char *path, int *exists, int *nDims, int *dims, int *itemSize, char *type, int *dataSize, char **data);
extern int getInfoWithDataNoLock(int expIdx, char *cpoPath, char *path, int *exists, int *nDims, int *dims, int *itemSize, char *type, int *dataSize, char **data);
extern void appendSlice(int expIdx, char *cpoPath, char *path, char *timeBasePath, int nDims, int *dims, int itemSize, char type, int sliceSize, char *slice);
extern void appendSliceSet(int expIdx, char *cpoPath, char *path, char *timeBasePath, int nDims, int *dims, int itemSize, char type, int sliceSize, char *slice, int numSlices);
extern void flushInfo(int expIdx);
extern void flushCpoInfo(int expIdx, char *cpoPath);
extern int getInfoNumSlices(int expIdx, char *cpoPath, char *path);
extern void removeAllInfoObjectSlices(int expIdx, char *cpoPath, char *path);
extern void appendInfoObjectSlice(int expIdx, char *cpoPath, char *path, char *buf, int size);
extern int getInfoSlice(int expIdx, char *cpoPath, char *path, int sliceIdx, int *exists, char **data);

static char *decompileDsc(void *ptr);
static void setCacheLevel(int expIdx, int level);
static int isEmpty(int nid);
static int hasMembers(int nid);
static int getCacheLevel(int expIdx);

static void updateInfo(int expIdx, char *cpoPath, char *path, struct descriptor *dataD, int isObject)
{
    EMPTYXD(xd);
    ARRAY_COEFF(char, 16) *arrayDPtr;
    int dims[16], nDims, i, status;
    for(i = 0; i < 16; i++)
	dims[i] = 0;

    if(dataD->class == CLASS_XD && ((struct descriptor_xd *)dataD)->l_length == 0)
    {
	putInfo(expIdx, cpoPath, path, 0, 0, dims);
	return;
    }
    if(dataD->class == CLASS_XD && ((struct descriptor_xd *)dataD)->l_length)
    {
	printf("FATAL ERROR IN UpdateInfo: Unexpected nonempty XD\n");
	exit(0);
    }

    if(dataD->class == CLASS_S)
    {
    	if(isObject)
    	{
	    printf("FATAL ERROR IN UpdateInfo: isObject for Non array XD\n");
	    exit(0);
    	}
	putInfoWithData(expIdx, cpoPath, path, 1, 0, dims, dataD->length, dataD->dtype, dataD->length, dataD->pointer, 0); //Cannot be a serialized object
	return;
    }
    arrayDPtr = (void *)dataD;
    nDims = arrayDPtr->dimct;
    if(nDims == 1)
	dims[0] = arrayDPtr->arsize/arrayDPtr->length;
    else
    {
        for(i = 0; i < nDims; i++)
	    dims[i] = arrayDPtr->m[i];
    }
    putInfoWithData(expIdx, cpoPath, path, 1, nDims, dims, arrayDPtr->length, arrayDPtr->dtype, arrayDPtr->arsize, arrayDPtr->pointer, isObject);

}

static void updateInfoObject(int expIdx, char *cpoPath, char *path, int exists, void *obj)
{
    char *fullPath;
    EMPTYXD(serializedXd);
    struct descriptor_a *arrD;
    int status;
    int dims[16];

    fullPath = malloc(strlen(path) + 11);
    sprintf(fullPath, "%s/non_timed", path);
    if(!exists)
    {
    	memset(dims, 0, sizeof(int) * 16);
        putInfo(expIdx, cpoPath, fullPath, 0, 0, dims);
	free(fullPath);
    	return;
    }
    status = MdsSerializeDscOut((struct descriptor *)obj, &serializedXd);
    arrD = (struct descriptor_a *)serializedXd.pointer;
    if(!(status & 1) || !arrD || arrD->class != CLASS_A)
    {
	printf("INTERNAL ERROR IN updateInfoObject:CANNOT SERIALIZE STRUCTURE : %s", MdsGetMsg(status));
        return;
    }
    updateInfo(expIdx, cpoPath, fullPath, (struct descriptor *)arrD, 1);
    MdsFree1Dx(&serializedXd, 0);
    free(fullPath);

}
static void updateInfoObjectSlice(int expIdx, char *cpoPath, char *path, void *obj)
{
    char *fullPath;
    EMPTYXD(serializedXd);
    struct descriptor_a *arrD;
    int status;
    int dims[16];
    struct descriptor_a *apd;
    int numSamples;

    fullPath = malloc(strlen(path) + 11);
    sprintf(fullPath, "%s/timed", path);

     apd = (struct descriptor_a *) obj;
     numSamples = apd->arsize / sizeof(struct descriptor *);
     if(numSamples == 1) //The object has been passed by put/replaceObjectSlice and therefore it is an array with 1 elements
         status = MdsSerializeDscOut(((struct descriptor **)apd->pointer)[0], &serializedXd);
     else //The object has been passed by putObject
          status = MdsSerializeDscOut((struct descriptor *)apd, &serializedXd);

    arrD = (struct descriptor_a *)serializedXd.pointer;
    if(!(status & 1) || !arrD || arrD->class != CLASS_A)
    {
	printf("INTERNAL ERROR IN updateInfoObject:CANNOT SERIALIZE STRUCTURE : %s", MdsGetMsg(status));
        return;
    }
    appendInfoObjectSlice(expIdx, cpoPath, fullPath, arrD->pointer, arrD->arsize);
    MdsFree1Dx(&serializedXd, 0);
    free(fullPath);
}

static void updateInfoTimedObject(int expIdx, char *cpoPath, char *path, int exists, void *obj)
{
    char *fullPath;
    EMPTYXD(serializedXd);
    struct descriptor_a *arrD;
    int status, numSlices;
    int dims[16], sliceIdx;
    struct descriptor_a *apd;
    char *currSlice;
    struct descriptor **currPtr;

    fullPath = malloc(strlen(path) + 11);
    sprintf(fullPath, "%s/timed", path);
    if(!exists)
    {
    	memset(dims, 0, sizeof(int) * 16);
        putInfo(expIdx, cpoPath, fullPath, 0, 0, dims);
	free(fullPath);
    	return;
    }

    apd = (struct descriptor_a *)obj;
    if(apd->class == CLASS_XD)
        apd = (struct descriptor_a *)((struct descriptor_xd *)apd)->pointer;
    if(apd->class != CLASS_APD)
    {
	printf("INTERNAL ERROR in updateInfoTimedObject:Not an APD at path %s, IDS path %s\n", path, cpoPath);
	return;
    }

    numSlices = apd->arsize / apd->length;
    currPtr = (struct descriptor **)apd->pointer;
    removeAllInfoObjectSlices(expIdx, cpoPath, fullPath);
    for(sliceIdx = 0; sliceIdx < numSlices; sliceIdx++)
    {
        status = MdsSerializeDscOut(currPtr[sliceIdx], &serializedXd);
        arrD = (struct descriptor_a *)serializedXd.pointer;
        if(!(status & 1) || !arrD || arrD->class != CLASS_A)
        {
	    printf("INTERNAL ERROR IN updateInfoTimedObject:CANNOT SERIALIZE STRUCTURE : %s", MdsGetMsg(status));
            return;
        }
	appendInfoObjectSlice(expIdx, cpoPath, fullPath, arrD->pointer, arrD->arsize);
	MdsFree1Dx(&serializedXd, 0);
    }
    free(fullPath);
}

static int getInfoData(int expIdx, char *cpoPath, char *path, int *exists, int *nDims, int *dims, struct descriptor_xd *retXd)
{
    int infoExists;
    unsigned int dataSize;
    int status, i;
    char *data;
    char dtype;
    unsigned int itemSize;
    struct descriptor dataD;
    DESCRIPTOR_A_COEFF(arrayD, 0, 0, 0, 0, 0);

    infoExists = getInfoWithData(expIdx, cpoPath, path, exists, nDims, dims, &itemSize, &dtype, &dataSize, &data);

    //printf("GET INFO DATA %s/%s infoExists: %d\n", cpoPath, path, infoExists);

    if(!infoExists) return 0;
    if(*exists) // why is this an input argument of the function ??
    {
        if(dataSize == 0 || data == 0)
	{
	    //printf("INTERNAL ERROR IN  getInfoData: Missing data in existing info\n");
	    return 0;
	}
	if(*nDims == 0) //Scalar
	{
	    dataD.class = CLASS_S;
	    dataD.dtype = dtype;
	    dataD.length = itemSize;
	    dataD.pointer = data;
	    status = MdsCopyDxXd(&dataD, retXd);
	}
	else
	{
	    arrayD.dtype = dtype;
	    arrayD.length = itemSize;
	    arrayD.dimct = *nDims;
	    for(i = 0; i < *nDims; i++)
		arrayD.m[i] = dims[i];
	    arrayD.pointer = data;
	    arrayD.arsize = dataSize;
	    status = MdsCopyDxXd((struct descriptor *)&arrayD, retXd);
	}
	if(!(status & 1))
	{
	    printf("INTERNAL ERROR IN  getInfoData: Cannot get existing info: %s\n", MdsGetMsg(status));
	    return 0;
	}
    }
    return 1;
}
static int getInfoObjectSlice(int expIdx, char *cpoPath, char *path, int sliceIdx, int *exists, void **obj)
//Return 0 if info not in cache, 1 otherwise. The actual presence of data is reflected in argument exists
{
    int infoExists;
    int status;
    char *data;
    struct descriptor_xd *retXd;
    EMPTYXD(emptyXd);
    DESCRIPTOR_APD(retApd, DTYPE_L, 0, 0);
    EMPTYXD(deserializedXd);

    infoExists = getInfoSlice(expIdx, cpoPath, path, sliceIdx, exists, &data);
    if(!infoExists) return 0;
    if(*exists)
    {
   	retXd = (struct descriptor_xd *)malloc(sizeof(struct descriptor_xd));
    	*retXd = emptyXd;
	status = MdsSerializeDscIn(data, &deserializedXd);
        retApd.arsize = retApd.length = sizeof(struct descriptor *);
        retApd.pointer = &deserializedXd.pointer;
        status = MdsCopyDxXd((struct descriptor *)&retApd, retXd);
        MdsFree1Dx(&deserializedXd, 0);
 	*obj = (void *)retXd;
	if(!(status & 1))
	{
	    printf("INTERNAL ERROR IN  getInfoObjectSlice: Cannot deserialize existing info: %s\n", MdsGetMsg(status));
	    return 0;
	}
    }
    return 1;
}

static int getInfoObject(int expIdx, char *cpoPath, char *path, int *exists, void **retObj, int isTimed)
//Return 0 if info not fount, 1 otherwise
{
    int infoExists;
    unsigned int dataSize;
    int status, i;
    char *data;
    char dtype;
    unsigned int itemSize;
    int nDims, dims[16];
    struct descriptor dataD;
    DESCRIPTOR_A_COEFF(arrayD, 0, 0, 0, 0, 0);
    EMPTYXD(emptyXd);
    struct descriptor_xd *retXd;
    char *fullPath;
    int numSlices, sliceIdx;
    struct descriptor_xd *xds;
    struct descriptor **dscPtrs;
    char *currSlice;
    DESCRIPTOR_APD(apd, DTYPE_L, 0, 0);

    fullPath = malloc(strlen(path) + 11);
    if(isTimed)
    	sprintf(fullPath, "%s/timed", path);
    else
    	sprintf(fullPath, "%s/non_timed", path);

    if(!isTimed)
    {
        infoExists = getInfoWithData(expIdx, cpoPath, fullPath, exists, &nDims, dims, &itemSize, &dtype, &dataSize, &data);
        if(!infoExists)
	{
	    free(fullPath);
	    return 0;
	}
        if(*exists)
        {
            if(dataSize == 0 || data == 0)
	    {
	        printf("INTERNAL ERROR IN  getInfoObject: Missing data in existing info\n");
	        return 0;
	    }
	    if(nDims == 0) //Scalar
	    {
	        printf("INTERNAL ERROR IN  getInfoObject: Scalar data returned from cache \n");
	        return 0;
	    }
    	    retXd = (struct descriptor_xd *)malloc(sizeof(struct descriptor_xd));
    	    *retXd = emptyXd;
	    status = MdsSerializeDscIn(data, retXd);
	    *retObj = (void *)retXd;
	    if(!(status & 1))
	    {
	        printf("INTERNAL ERROR IN  getInfoObject: Cannot deserialize existing info: %s\n", MdsGetMsg(status));
	        return 0;
	    }
        }
	free(fullPath);
        return 1;
    }
    else //TIMED
    {
    	numSlices = getInfoNumSlices(expIdx, cpoPath, fullPath);
	if(numSlices == -1) //Info not found
	{
	    free(fullPath);
	    return 0;
	}
//Gabriele September 2016: if the AoS is not present no slices are retrieved
	if(numSlices == 0)
	{
	    *exists = 0;
	    *retObj = 0;
	    return 1; //Info found - does not exists
	}
    	xds = (struct descriptor_xd *)malloc(sizeof(struct descriptor_xd) * numSlices);
        dscPtrs = (struct descriptor **)malloc(sizeof(struct descriptor *) * numSlices);
        for(sliceIdx = 0; sliceIdx < numSlices; sliceIdx++)
    	    xds[sliceIdx] = emptyXd;
        for(sliceIdx = 0; sliceIdx < numSlices; sliceIdx++)
    	{
	    *exists = 0;
	    getInfoSlice(expIdx, cpoPath, fullPath, sliceIdx, exists, &currSlice);
	    if(!*exists)
            {
	        printf("INTERNAL ERROR: Cannot read slice in cache at path %s/%s\n", cpoPath, path);
	        return 0;
	    }
	    status = MdsSerializeDscIn(currSlice, &xds[sliceIdx]);
    	    if(!(status & 1))
    	    {
        	printf("INTERNAL ERROR: Cannot deserialize data returned at path %s/%s: %s\n", cpoPath, path, MdsGetMsg(status));
		return 0;
    	    }
	    dscPtrs[sliceIdx] = xds[sliceIdx].pointer;
    	}
    	apd.arsize = numSlices * sizeof(struct descriptor *);
        apd.pointer = (void *)dscPtrs;
        retXd = (struct descriptor_xd *)malloc(sizeof(struct descriptor_xd));
        *retXd = emptyXd;
        MdsCopyDxXd((struct descriptor *)&apd, retXd);
        for(sliceIdx = 0; sliceIdx < numSlices; sliceIdx++)
	    MdsFree1Dx(&xds[sliceIdx], 0);
        free((char *)xds);
        free((char *)dscPtrs);
        *retObj = retXd;
	free(fullPath);
        return 1;
    }
}

/* internal Flush is called when traversing the cached data structure for a given pulse file */
void internalFlush(int expIdx, char *cpoPath, char *path, char *timeBasePath,  int isSliced, int isObject, int nDims, int *dims, int itemSize, int dtype, int dataSize,
	char *data, int numSlices, int *sliceOffsets)
{
    int status = 0, i;
    struct descriptor dataD;
    DESCRIPTOR_A_COEFF(arrayD, 0, 0, 0, 0, 0);
    double *times;
    unsigned int timeItemSize, timeDataSize;
    int timeExists, timeNDims, timeDims[16], currSlice;
    char timeType;
    EMPTYXD(emptyXd);
    struct descriptor_xd *retXd;
    void *obj;


    //reportInfo("START INTERNAL FLUSH %s\n", path);
    //printf("Internal Flush %s %s\n", cpoPath, path);
    if(dataSize == 0 || data == 0)
    {
	   // printf("INTERNAL ERROR IN  internalFlush: Missing data in existing info\n");
	    return;
    }
    if(isObject)
    {
    	if(isSliced)
	{
// Remove previous data, if any
            if(isExpRemote(expIdx))
        	status = putDataRemote(getExpConnectionId(expIdx), getExpRemoteIdx(expIdx), cpoPath, path, (struct descriptor *)&emptyXd);
    	    else
    		status =  putDataLocal(expIdx, cpoPath, path, (struct descriptor *)&emptyXd);
    	    if(status)
	    {
      		printf("ERROR in internalFlush: Cannot delete data %s  %s\n", cpoPath, path);
		return;
	    }
	    retXd = (struct descriptor_xd *)malloc(sizeof(struct descriptor_xd));
	    for(currSlice = 0; currSlice < numSlices; currSlice++)
	    {
    	    	*retXd = emptyXd;
	    	status = MdsSerializeDscIn(&data[sliceOffsets[currSlice]], retXd);
	    	obj = (void *)(retXd->pointer);
	    	if(!(status & 1))
	    	{
	    	    printf("FATAL ERROR in internalFlush: Cannot deserialize object slice\n");
		    return;
	    	}
		status = putObjectSegment(expIdx, cpoPath, path, obj, -1);
		if(status)
	    	{
	    	    printf("FATAL ERROR in internalFlush: Cannot write object slice\n");
		    return;
	    	}
		MdsFree1Dx(retXd, 0);
	    }
	    free((char *)retXd);
	}
	else
	{
	    retXd = (struct descriptor_xd *)malloc(sizeof(struct descriptor_xd));
    	    *retXd = emptyXd;
	    status = MdsSerializeDscIn(data, retXd);
	    obj = (void *)retXd;
	    if(!(status & 1))
	    {
	    	printf("FATAL ERROR in internalFlush: Cannot deserialize object\n");
		return;
	    }
    	    if(isExpRemote(expIdx))
      		status = putDataRemote(getExpConnectionId(expIdx), getExpRemoteIdx(expIdx), cpoPath, path, obj);
    	    else
    		status = putDataLocal(expIdx, cpoPath, path, obj);
	    MdsFree1Dx(retXd, 0);
	    free((char *)retXd);
	}
    	return;
    }
//Non Object management follows
    //printf("Reached here in InternalFlush \n");
    if(nDims == 0) //Scalar
    {
	dataD.class = CLASS_S;
	dataD.dtype = dtype;
	dataD.length = itemSize;
	dataD.pointer = data;
        if(isExpRemote(expIdx))
            status = putDataRemote(getExpConnectionId(expIdx), getExpRemoteIdx(expIdx), cpoPath, path, &dataD);
        else
    	    status = putDataLocal(expIdx, cpoPath, path, &dataD);
    }
    else //Array
    {
    //printf("Reached Array part in InternalFlush \n");
	arrayD.dtype = dtype;
	arrayD.length = itemSize;
	arrayD.dimct = nDims;
	for(i = 0; i < nDims; i++)
	    arrayD.m[i] = dims[i];
	arrayD.pointer = data;
	arrayD.arsize = dataSize;
	if(isSliced)
	{
            //printf("Reached getInfoWithDataNoLock in InternalFlush \n");
	    getInfoWithDataNoLock(expIdx, cpoPath, timeBasePath, &timeExists, &timeNDims, timeDims, &timeItemSize, &timeType, &timeDataSize, (char **)&times); // This is getting the timebase from the structure in memory, should be fine now
	    if(!timeExists)
	    {
	    	printf("FATAL ERROR in internalFlush: Missing time in slice\n");
		return;
	    }
            //printf("Passed getInfoWithDataNoLock in InternalFlush \n");

// Remove previous data, if any
            if(isExpRemote(expIdx))
        	status = putDataRemote(getExpConnectionId(expIdx), getExpRemoteIdx(expIdx), cpoPath, path, (struct descriptor *)&emptyXd);
    	    else
    		status =  putDataLocal(expIdx, cpoPath, path, (struct descriptor *)&emptyXd);
            //printf("Passed putDataLocal in InternalFlush \n");

    	    if(status)
	    {
      		printf("ERROR in internalFlush: Cannot delete data %s  %s\n", cpoPath, path);
		return;
	    }
//Write ALL slices as a unique segment
            if(isExpRemote(expIdx))
                status = putSegmentRemote(getExpConnectionId(expIdx), getExpRemoteIdx(expIdx), cpoPath, path, timeBasePath, (void *)&arrayD, times, timeDims[0]);
            else
            //printf("Reached putSegmentLocal in InternalFlush \n");
    	        status = putSegmentLocal(expIdx, cpoPath, path, timeBasePath, (void *)&arrayD, times, timeDims[0]);
	}
	else
	{
            if(isExpRemote(expIdx))
                status = putDataRemote(getExpConnectionId(expIdx), getExpRemoteIdx(expIdx), cpoPath, path, (void *)&arrayD);
            else
    	        status = putDataLocal(expIdx, cpoPath, path, (void *)&arrayD);
	}
    }
    if(status)
      printf("ERROR in internalFlush: Cannot write data %s  %s\n", cpoPath, path);
}


static void appendSliceData(int expIdx, char *cpoPath, char *path, char *timeBasePath, struct descriptor *dataD)
{
    ARRAY_COEFF(char, 16) *arrayDPtr;
    int dims[16], nDims, i;

    for(i = 0; i < 16; i++)
	dims[i] = 0;
    if(dataD->class == CLASS_XD)
    {
	printf("FATAL ERROR IN appendSliceData: Unexpected nonempty XD\n");
	exit(0);
    }

    if(dataD->class == CLASS_S)
    {
	appendSlice(expIdx, cpoPath, path, timeBasePath, 0, dims, dataD->length, dataD->dtype, dataD->length, dataD->pointer);
	return;
    }
    arrayDPtr = (void *)dataD;
    nDims = arrayDPtr->dimct;
    if(nDims == 1)
	dims[0] = arrayDPtr->arsize/arrayDPtr->length;
    else
    {
        for(i = 0; i < nDims; i++)
	    dims[i] = arrayDPtr->m[i];
    }
    appendSlice(expIdx, cpoPath, path, timeBasePath, nDims, dims, arrayDPtr->length, arrayDPtr->dtype, arrayDPtr->arsize, arrayDPtr->pointer);
}
static void appendSliceSetData(int expIdx, char *cpoPath, char *path, char *timeBasePath, struct descriptor_a *dataD)
{
    ARRAY_COEFF(char, 16) *arrayDPtr;
    int dims[16], nDims, i;

    for(i = 0; i < 16; i++)
	dims[i] = 0;
    if(dataD->class == CLASS_XD)
    {
	printf("FATAL ERROR IN appendSliceData: Unexpected nonempty XD\n");
	exit(0);
    }

    if(dataD->class == CLASS_S)
    {
	printf("FATAL ERROR IN appendSliceData: Scalar data passed\n");
	exit(0);
    }
    arrayDPtr = (void *)dataD;
    nDims = arrayDPtr->dimct;
    if(nDims == 1)
	dims[0] = arrayDPtr->arsize/arrayDPtr->length;
    else
    {
        for(i = 0; i < nDims; i++)
	    dims[i] = arrayDPtr->m[i];
    }
    appendSliceSet(expIdx, cpoPath, path, timeBasePath, nDims, dims, arrayDPtr->length, arrayDPtr->dtype, arrayDPtr->arsize, arrayDPtr->pointer, dims[nDims - 1]);
}

///////////////////////////////////////////////////////////////////////////

int getTimedData(int expIdx, char *cpoPath, char *path, double start, double end, struct descriptor_xd *retDataXd,
    struct descriptor_xd *retTimesXd, int all);

int mdsgetTimedData(int expIdx, char *cpoPath, char *path, double start, double end, struct descriptor_xd *retDataXd,
    struct descriptor_xd *retTimesXd, int all);

int getData(int expIdx, char *cpoPath, char *path, struct descriptor_xd *retXd, int evaluate)
{
/////////Memory Mapped existence info //////
    int nDims, dims[16], exists;
    int status;
static int counter;
    int cacheLevel = getCacheLevel(expIdx);
    //printf("getData : call of getInfoData for %s %s returns %d \n",cpoPath, path, getInfoData(expIdx, cpoPath, path, &exists, &nDims, dims, retXd));
    if(cacheLevel > 0 && getInfoData(expIdx, cpoPath, path, &exists, &nDims, dims, retXd))
    {
//static int count;
  //printf("DATA HIT %s %s\n", cpoPath, path);
	if(exists)
        { //printf("getData : data exists according to getInfoData %s %s\n",cpoPath, path);// This part is correct but is not traversed by the time dependent quantities in this test
	    return 0;}
	else
        { //printf("getData : data DOES NOT EXIST according to getInfoData %s %s\n",cpoPath, path); //; This part is correct but is not traversed by the time dependent quantities in this test
	    return -1;}
    }
//////////////////////////////////////////////////
// printf("MISS %d %s %s\n", counter++, cpoPath, path);

    if(isExpRemote(expIdx))
        status =  getDataRemote(getExpConnectionId(expIdx), getExpRemoteIdx(expIdx), cpoPath, path, retXd, evaluate);
    else
    	status = getDataLocal(expIdx, cpoPath, path, retXd, evaluate);

//////////Memory info management///////////////
    if(cacheLevel > 0)
    {
    	if(!status)
    	    updateInfo(expIdx, cpoPath, path, retXd->pointer, 0);
        else
	    putInfo(expIdx, cpoPath, path, 0, 0, dims);
    }
////////////////////////////////////////////////
//printf("getData DATA %s/%s %s\n", cpoPath, path, decompileDsc((void *)retXd));

    return status;
}

// similar to getData but with a path argument already converted to MDSplus format
int mdsgetData(int expIdx, char *cpoPath, char *path, struct descriptor_xd *retXd, int evaluate)
{
/////////Memory Mapped existence info //////
    int nDims, dims[16], exists;
    int status;
static int counter;
    int cacheLevel = getCacheLevel(expIdx);
    //printf("mdsgetData : call of getInfoData for %s %s returns %d \n",cpoPath, path, getInfoData(expIdx, cpoPath, path, &exists, &nDims, dims, retXd));
    if(cacheLevel > 0 && getInfoData(expIdx, cpoPath, path, &exists, &nDims, dims, retXd))
    {
//static int count;
 //printf("DATA HIT %s %s\n", cpoPath, path);
	if(exists)
        { //printf("getData : data exists according to getInfoData %s %s\n",cpoPath, path);// This part is correct but is not traversed by the time dependent quantities in this test
	    return 0;}
	else
        { //printf("getData : data DOES NOT EXIST according to getInfoData %s %s\n",cpoPath, path); //; This part is correct but is not traversed by the time dependent quantities in this test
	    return -1;}
    }
//////////////////////////////////////////////////
// printf("MISS %d %s %s\n", counter++, cpoPath, path);

    status = mdsgetDataLocal(expIdx, cpoPath, path, retXd, evaluate);

//////////Memory info management///////////////
    if(cacheLevel > 0)
    {
    	if(!status)
    	    updateInfo(expIdx, cpoPath, path, retXd->pointer, 0);
        else
	    putInfo(expIdx, cpoPath, path, 0, 0, dims);
    }
////////////////////////////////////////////////
//printf("getData DATA %s/%s %s\n", cpoPath, path, decompileDsc((void *)retXd));

    return status;
}

int getDataNoDescr(int expIdx, char *cpoPath, char *path, void **retData, int *retDims,  int *retDimsCt, char *retType, char *retClass, int *retLength,  int64_t *retArsize, int evaluate)
{
    int status, i;
    ARRAY_COEFF(char, 16) *dataDPtr;
    EMPTYXD(xd);
    status = getData(expIdx, cpoPath, path, &xd, evaluate);

    //printf("status of getData = %d\n",status);

    if(!status)
    {
	*retType = xd.pointer->dtype;
	*retClass = xd.pointer->class;
	if(xd.pointer->class != CLASS_A)
	{
	    printf("INTERNAL ERROR: getDataNoDescr called for non aray data!!!\n");
	    return -1;
	}
	dataDPtr = (void *)xd.pointer;
    	*retDimsCt = dataDPtr->dimct;
	if(dataDPtr->dimct == 1)
	{
	    retDims[0] = dataDPtr->arsize/dataDPtr->length;
	}
	else
	{
	    for(i = 0; i < dataDPtr->dimct; i++)
        	retDims[i] = dataDPtr->m[i];
	}
      	*retLength = dataDPtr->length;
    	*retArsize = dataDPtr->arsize;
    	*retData = malloc(dataDPtr->arsize);
    	memcpy(*retData, dataDPtr->pointer, dataDPtr->arsize);
	MdsFree1Dx(&xd, 0);
    }
    return status;
}

//NOTE: getDataNoDescr will be called ONLY for arrays
//OLD VERSION
/*int getDataNoDescr(int expIdx, char *cpoPath, char *path, void **retData, int *retDims,  int *retDimsCt, char *retType, char *retClass, int *retLength,  int64_t *retArsize, int evaluate)
{
    int status, i;
    ARRAY_COEFF(char, 16) *dataDPtr;
    EMPTYXD(xd);
    if(isExpRemote(expIdx))
    {
        status = getDataRemote(getExpConnectionId(expIdx), getExpRemoteIdx(expIdx), cpoPath, path, &xd, evaluate);
	if(!status)
	{
	    *retType = xd.pointer->dtype;
	    *retClass = xd.pointer->class;
	    if(xd.pointer->class != CLASS_A)
	    {
		printf("INTERNAL ERROR: getDataNoDescr called for non aray data!!!\n");
		return -1;
	    }
	    dataDPtr = (void *)xd.pointer;
    	    *retDimsCt = dataDPtr->dimct;
	    if(dataDPtr->dimct == 1)
	    {
		retDims[0] = dataDPtr->arsize/dataDPtr->length;
	    }
	    else
	    {
		for(i = 0; i < dataDPtr->dimct; i++)
        	    retDims[i] = dataDPtr->m[i];
	    }
      	    *retLength = dataDPtr->length;
    	    *retArsize = dataDPtr->arsize;
    	    *retData = malloc(dataDPtr->arsize);
    	    memcpy(*retData, dataDPtr->pointer, dataDPtr->arsize);
	    MdsFree1Dx(&xd, 0);
	}
	return status;
    }
    else
    	return getDataLocalNoDescr(expIdx, cpoPath, path, retData, retDims,  retDimsCt, retType, retClass, retLength,  retArsize,  evaluate);
}

*/




int putData(int expIdx, char *cpoPath, char *path, struct descriptor *dataD)
{
    int cacheLevel = getCacheLevel(expIdx);
    if(cacheLevel > 0)
    	updateInfo(expIdx, cpoPath, path, dataD, 0);
    if(cacheLevel == 2)
    	return 0;
    if(isExpRemote(expIdx))
        return putDataRemote(getExpConnectionId(expIdx), getExpRemoteIdx(expIdx), cpoPath, path, dataD);
    else
    	return putDataLocal(expIdx, cpoPath, path, dataD);
}

int putSegment(int expIdx, char *cpoPath, char *path, char *timeBasePath, struct descriptor_a *dataD, double *times, int nTimes)
{
    int exists, nDims, dims[16], status;
    EMPTYXD(dataXd);
    EMPTYXD(timeXd);
    int cacheLevel = getCacheLevel(expIdx);


//First get Data, if any so that it is correctly posted in the memory cache
    if(cacheLevel > 0)
    {
    	if(!getInfo(expIdx, cpoPath, path, &exists, &nDims, dims))
    	{
//Argument time is no more used in getSlicedData()
 	    getSlicedData(expIdx, cpoPath,path, timeBasePath, 0., &dataXd, &timeXd, 0);
	    MdsFree1Dx(&dataXd, 0);
	    MdsFree1Dx(&timeXd, 0);
        }
    	appendSliceSetData(expIdx, cpoPath, path, timeBasePath, dataD);
    	if(cacheLevel == 2)
    	    return 0;
    }
    if(isExpRemote(expIdx))
        return putSegmentRemote(getExpConnectionId(expIdx), getExpRemoteIdx(expIdx), cpoPath, path, timeBasePath, dataD, times, nTimes);
    else
    	return putSegmentLocal(expIdx, cpoPath, path, timeBasePath, dataD, times, nTimes);
}
int putSlice(int expIdx, char *cpoPath, char *path, char *timeBasePath, struct descriptor *dataD, double time)
{
    int exists, nDims, dims[16], status;
    EMPTYXD(dataXd);
    EMPTYXD(timeXd);
    int cacheLevel = getCacheLevel(expIdx);


    if(cacheLevel > 0)
    {
//First get Data, if any so that it is correctly posted in the memory cache
    	if(!getInfo(expIdx, cpoPath, path, &exists, &nDims, dims))
    	{
 	    getSlicedData(expIdx, cpoPath,path, NULL, time, &dataXd, &timeXd, 0);
	    MdsFree1Dx(&dataXd, 0);
	    MdsFree1Dx(&timeXd, 0);
        }
        appendSliceData(expIdx, cpoPath, path, timeBasePath, dataD);
        if(cacheLevel == 2)
    	    return 0;
    }
    if(isExpRemote(expIdx))
        return putSliceRemote(getExpConnectionId(expIdx), getExpRemoteIdx(expIdx), cpoPath, path, timeBasePath, dataD, time);
    else
    	return putSliceLocal(expIdx, cpoPath, path, timeBasePath, dataD, time);
}
int replaceLastSlice(int expIdx, char *cpoPath, char *path, struct descriptor *dataD)
{
    if(isExpRemote(expIdx))
        return replaceLastSliceRemote(getExpConnectionId(expIdx), getExpRemoteIdx(expIdx), cpoPath, path, dataD);
    else
    	return replaceLastSliceLocal(expIdx, cpoPath, path, dataD);
}

int getSlicedData(int expIdx, char *cpoPath, char *path, char *timeBasePath, double time, struct descriptor_xd *retDataXd,
    struct descriptor_xd *retTimesXd, int expandObject)
{
    int status;

    status = getData(expIdx, cpoPath, path, retDataXd, 0);
    if(!status)
	status = getData(expIdx, cpoPath, timeBasePath, retTimesXd, 0);
    return status;

/*
    if(isExpRemote(expIdx))
        return getSlicedDataRemote(getExpConnectionId(expIdx), getExpRemoteIdx(expIdx), cpoPath, path, time, retDataXd,
		retTimesXd);
    else
    	return getSlicedDataLocal(expIdx, cpoPath, path, time, retDataXd, retTimesXd, expandObject);

*/
}

int mdsgetSlicedData(int expIdx, char *cpoPath, char *path, char *timeBasePath, double time, struct descriptor_xd *retDataXd,
    struct descriptor_xd *retTimesXd, int expandObject)
{
    int status;

    status = mdsgetData(expIdx, cpoPath, path, retDataXd, 0);
    if(!status) {
	status = mdsgetData(expIdx, cpoPath, timeBasePath, retTimesXd, 0); // BEWARE: internal compacted name for timeBasePath
    }
    return status;
}

int mdsbeginIdsPutSlice(int expIdx, char *path)
{
//reportInfo("BEGIN IDS PUT SLICE %s\n", path);
    lockIds();
    if(isExpRemote(expIdx))
        return mdsbeginIdsPutSliceRemote(getExpConnectionId(expIdx), getExpRemoteIdx(expIdx), path);
    else
        return mdsbeginIdsPutSliceLocal(expIdx, path);
}

int mdsendIdsPutSlice(int expIdx, char *path)
{
//reportInfo("END IDS PUT SLICE %s\n", path);
    int status;
    if(isExpRemote(expIdx))
        status = mdsendIdsPutSliceRemote(getExpConnectionId(expIdx), getExpRemoteIdx(expIdx), path);
    else
        status = mdsendIdsPutSliceLocal(expIdx, path);
    unlockIds();
    return status;
}

int mdsbeginIdsReplaceLastSlice(int expIdx, char *path)
{
    lockIds();
    if(isExpRemote(expIdx))
        return mdsbeginIdsReplaceLastSliceRemote(getExpConnectionId(expIdx), getExpRemoteIdx(expIdx), path);
    else
        return mdsbeginIdsReplaceLastSliceLocal(expIdx, path);
}

int mdsendIdsReplaceLastSlice(int expIdx, char *path)
{
    int status;
    if(isExpRemote(expIdx))
        status = mdsendIdsReplaceLastSliceRemote(getExpConnectionId(expIdx), getExpRemoteIdx(expIdx), path);
    else
        status = mdsendIdsReplaceLastSliceLocal(expIdx, path);
    unlockIds();
    return status;
}

int mdsbeginIdsPut(int expIdx, char *path)
{
//reportInfo("BEGIN IDS PUT  %s\n", path);
    lockIds();
    if(isExpRemote(expIdx))
        return mdsbeginIdsPutRemote(getExpConnectionId(expIdx), getExpRemoteIdx(expIdx), path);
    else
        return mdsbeginIdsPutLocal(expIdx, path);
}

int mdsendIdsPut(int expIdx, char *path)
{
//reportInfo("END IDS PUT %s\n", path);
    int status;
    if(isExpRemote(expIdx))
        status = mdsendIdsPutRemote(getExpConnectionId(expIdx), getExpRemoteIdx(expIdx), path);
    else
        status = mdsendIdsPutLocal(expIdx, path);
    unlockIds();
    return status;
}

int mdsbeginIdsPutTimed(int expIdx, char *path, int samples, double *inTimes)
{
//reportInfo("BEGIN IDS PUT TIMED %s\n", path);
//    lockIds();
    putTimes = malloc(sizeof(double)*samples);
    memcpy(putTimes, inTimes, sizeof(double)*samples);
    nPutTimes = samples;

    if(isExpRemote(expIdx))
        return mdsbeginIdsPutTimedRemote(getExpConnectionId(expIdx), getExpRemoteIdx(expIdx), path, samples, inTimes);
    else
        return mdsbeginIdsPutTimedLocal(expIdx, path, samples, inTimes);
}

int mdsendIdsPutTimed(int expIdx, char *path)
{
//reportInfo("END IDS PUT TIMED %s\n", path);
    int status;
    if(nPutTimes > 0) {
        free((char *)putTimes);
	nPutTimes = 0;
    }

    if(isExpRemote(expIdx))
        status = mdsendIdsPutTimedRemote(getExpConnectionId(expIdx), getExpRemoteIdx(expIdx), path);
    else
        status = mdsendIdsPutTimedLocal(expIdx, path);
//    unlockIds();
    return status;
}

int mdsbeginIdsPutNonTimed(int expIdx, char *path)
{
//reportInfo("BEGIN IDS PUT NON TIMED %s\n", path);
    lockIds();
    if(isExpRemote(expIdx))
        return mdsbeginIdsPutNonTimedRemote(getExpConnectionId(expIdx), getExpRemoteIdx(expIdx), path);
    else
        return mdsbeginIdsPutNonTimedLocal(expIdx, path);
}

int mdsendIdsPutNonTimed(int expIdx, char *path)
{
//reportInfo("END IDS PUT NON TIMED %s\n", path);
    int status;
    if(isExpRemote(expIdx))
        status = mdsendIdsPutNonTimedRemote(getExpConnectionId(expIdx), getExpRemoteIdx(expIdx), path);
    else
        status = mdsendIdsPutNonTimedLocal(expIdx, path);
    unlockIds();
    return status;
}



int mdsbeginIdsGet(int expIdx, char *path, int isTimed, int *retSamples)
{
    int status;
//reportInfo("BEGIN IDS GET %s\n", path);
    lockIds();
    if(isExpRemote(expIdx))
        status = mdsbeginIdsGetRemote(getExpConnectionId(expIdx), getExpRemoteIdx(expIdx), path, isTimed, retSamples);
    else
        //status = mdsbeginIdsGetLocal(expIdx, path, isTimed, retSamples); This function is not useful anymore, because there is no time dimension associated to an IDS
        // Therefore we just skip it and return zeros
        status = 0;
        *retSamples = 0;
    if(status)
	*retSamples = 0;
    return status;
}

int mdsendIdsGet(int expIdx, char *path)
{
//reportInfo("END IDS GET %s\n", path);
    int status;
    if(isExpRemote(expIdx))
        status = mdsendIdsGetRemote(getExpConnectionId(expIdx), getExpRemoteIdx(expIdx), path);
    else
        status = mdsendIdsGetLocal(expIdx, path);
    unlockIds();
    return status;
}

int mdsbeginIdsGetSlice(int expIdx, char *path, double time)
{
//reportInfo("BEGIN IDS GET SLICE %s\n", path);
    lockIds();
    if(isExpRemote(expIdx))
        return mdsbeginIdsGetSliceRemote(getExpConnectionId(expIdx), getExpRemoteIdx(expIdx), path, time);
    else
        return mdsbeginIdsGetSliceLocal(expIdx, path, time);
}

int mdsendIdsGetSlice(int expIdx, char *path)
{
//reportInfo("END IDS GET SLICE %s\n", path);
    int status;
    if(isExpRemote(expIdx))
        status = mdsendIdsGetSliceRemote(getExpConnectionId(expIdx), getExpRemoteIdx(expIdx), path);
    else
        status = mdsendIdsGetSliceLocal(expIdx, path);
    unlockIds();
    return status;
}


////////////////////////////////////

//MERGE
// paths before setting a new environment
static char prevTreeBase0[1024] = {0};
static char prevTreeBase1[1024] = {0};
static char prevTreeBase2[1024] = {0};
static char prevTreeBase3[1024] = {0};
static char prevTreeBase4[1024] = {0};
static char prevTreeBase5[1024] = {0};
static char prevTreeBase6[1024] = {0};
static char prevTreeBase7[1024] = {0};
static char prevTreeBase8[1024] = {0};
static char prevTreeBase9[1024] = {0};

static void restoreDataEnv()
{
/*		setenv("MDSPLUS_TREE_BASE_0",prevTreeBase0,1);
		setenv("MDSPLUS_TREE_BASE_1",prevTreeBase1,1);
		setenv("MDSPLUS_TREE_BASE_2",prevTreeBase2,1);
		setenv("MDSPLUS_TREE_BASE_3",prevTreeBase3,1);
		setenv("MDSPLUS_TREE_BASE_4",prevTreeBase4,1);
		setenv("MDSPLUS_TREE_BASE_5",prevTreeBase5,1);
		setenv("MDSPLUS_TREE_BASE_6",prevTreeBase6,1);
		setenv("MDSPLUS_TREE_BASE_7",prevTreeBase7,1);
		setenv("MDSPLUS_TREE_BASE_8",prevTreeBase8,1);
		setenv("MDSPLUS_TREE_BASE_9",prevTreeBase9,1); */
		(prevTreeBase0[0] != 0) ? (setenv("MDSPLUS_TREE_BASE_0",prevTreeBase0,1)) : (0);
		(prevTreeBase1[0] != 0) ? (setenv("MDSPLUS_TREE_BASE_1",prevTreeBase1,1)) : (0);
		(prevTreeBase2[0] != 0) ? (setenv("MDSPLUS_TREE_BASE_2",prevTreeBase2,1)) : (0);
		(prevTreeBase3[0] != 0) ? (setenv("MDSPLUS_TREE_BASE_3",prevTreeBase3,1)) : (0);
		(prevTreeBase4[0] != 0) ? (setenv("MDSPLUS_TREE_BASE_4",prevTreeBase4,1)) : (0);
		(prevTreeBase5[0] != 0) ? (setenv("MDSPLUS_TREE_BASE_5",prevTreeBase5,1)) : (0);
		(prevTreeBase6[0] != 0) ? (setenv("MDSPLUS_TREE_BASE_6",prevTreeBase6,1)) : (0);
		(prevTreeBase7[0] != 0) ? (setenv("MDSPLUS_TREE_BASE_7",prevTreeBase7,1)) : (0);
		(prevTreeBase8[0] != 0) ? (setenv("MDSPLUS_TREE_BASE_8",prevTreeBase8,1)) : (0);
		(prevTreeBase9[0] != 0) ? (setenv("MDSPLUS_TREE_BASE_9",prevTreeBase9,1)) : (0);

}
//MERGE


static void setDataEnv(char *user, char *tokamak, char *version)
{
  //MERGE
    char treeBase0[1024] = {0};
    char treeBase1[1024] = {0};
    char treeBase2[1024] = {0};
    char treeBase3[1024] = {0};
    char treeBase4[1024] = {0};
    char treeBase5[1024] = {0};
    char treeBase6[1024] = {0};
    char treeBase7[1024] = {0};
    char treeBase8[1024] = {0};
    char treeBase9[1024] = {0};
	 char* treeBase_ = 0;

    /*
    char *treeBase0 = malloc(1024);
    char *treeBase1 = malloc(1024);
    char *treeBase2 = malloc(1024);
    char *treeBase3 = malloc(1024);
    char *treeBase4 = malloc(1024);
    char *treeBase5 = malloc(1024);
    char *treeBase6 = malloc(1024);
    char *treeBase7 = malloc(1024);
    char *treeBase8 = malloc(1024);
    char *treeBase9 = malloc(1024);
    */
    //MERGE
    char first[2];

    if(!strcmp(user, "public"))
    {
      //MERGE
    	sprintf(treeBase0, "/pfs/imasdb/imas_trees/public/%s/%s/mdsplus/0;/pfs/imasdb/imas_trees/public/models/%s/mdsplus", tokamak, version, version);
    	sprintf(treeBase1, "/pfs/imasdb/imas_trees/public/%s/%s/mdsplus/1;/pfs/imasdb/imas_trees/public/models/%s/mdsplus", tokamak, version, version);
    	sprintf(treeBase2, "/pfs/imasdb/imas_trees/public/%s/%s/mdsplus/2;/pfs/imasdb/imas_trees/public/models/%s/mdsplus", tokamak, version, version);
    	sprintf(treeBase3, "/pfs/imasdb/imas_trees/public/%s/%s/mdsplus/3;/pfs/imasdb/imas_trees/public/models/%s/mdsplus", tokamak, version, version);
    	sprintf(treeBase4, "/pfs/imasdb/imas_trees/public/%s/%s/mdsplus/4;/pfs/imasdb/imas_trees/public/models/%s/mdsplus", tokamak, version, version);
    	sprintf(treeBase5, "/pfs/imasdb/imas_trees/public/%s/%s/mdsplus/5;/pfs/imasdb/imas_trees/public/models/%s/mdsplus", tokamak, version, version);
    	sprintf(treeBase6, "/pfs/imasdb/imas_trees/public/%s/%s/mdsplus/6;/pfs/imasdb/imas_trees/public/models/%s/mdsplus", tokamak, version, version);
    	sprintf(treeBase7, "/pfs/imasdb/imas_trees/public/%s/%s/mdsplus/7;/pfs/imasdb/imas_trees/public/models/%s/mdsplus", tokamak, version, version);
    	sprintf(treeBase8, "/pfs/imasdb/imas_trees/public/%s/%s/mdsplus/8;/pfs/imasdb/imas_trees/public/models/%s/mdsplus", tokamak, version, version);
    	sprintf(treeBase9, "/pfs/imasdb/imas_trees/public/%s/%s/mdsplus/9;/pfs/imasdb/imas_trees/public/models/%s/mdsplus", tokamak, version, version);
	/*
    	sprintf(treeBase0, "MDSPLUS_TREE_BASE_0=/pfs/imasdb/imas_trees/public/%s/%s/mdsplus/0;/pfs/imasdb/imas_trees/public/models/%s/mdsplus", tokamak, version, version);
    	sprintf(treeBase1, "MDSPLUS_TREE_BASE_1=/pfs/imasdb/imas_trees/public/%s/%s/mdsplus/1;/pfs/imasdb/imas_trees/public/models/%s/mdsplus", tokamak, version, version);
    	sprintf(treeBase2, "MDSPLUS_TREE_BASE_2=/pfs/imasdb/imas_trees/public/%s/%s/mdsplus/2;/pfs/imasdb/imas_trees/public/models/%s/mdsplus", tokamak, version, version);
    	sprintf(treeBase3, "MDSPLUS_TREE_BASE_3=/pfs/imasdb/imas_trees/public/%s/%s/mdsplus/3;/pfs/imasdb/imas_trees/public/models/%s/mdsplus", tokamak, version, version);
    	sprintf(treeBase4, "MDSPLUS_TREE_BASE_4=/pfs/imasdb/imas_trees/public/%s/%s/mdsplus/4;/pfs/imasdb/imas_trees/public/models/%s/mdsplus", tokamak, version, version);
    	sprintf(treeBase5, "MDSPLUS_TREE_BASE_5=/pfs/imasdb/imas_trees/public/%s/%s/mdsplus/5;/pfs/imasdb/imas_trees/public/models/%s/mdsplus", tokamak, version, version);
    	sprintf(treeBase6, "MDSPLUS_TREE_BASE_6=/pfs/imasdb/imas_trees/public/%s/%s/mdsplus/6;/pfs/imasdb/imas_trees/public/models/%s/mdsplus", tokamak, version, version);
    	sprintf(treeBase7, "MDSPLUS_TREE_BASE_7=/pfs/imasdb/imas_trees/public/%s/%s/mdsplus/7;/pfs/imasdb/imas_trees/public/models/%s/mdsplus", tokamak, version, version);
    	sprintf(treeBase8, "MDSPLUS_TREE_BASE_8=/pfs/imasdb/imas_trees/public/%s/%s/mdsplus/8;/pfs/imasdb/imas_trees/public/models/%s/mdsplus", tokamak, version, version);
    	sprintf(treeBase9, "MDSPLUS_TREE_BASE_9=/pfs/imasdb/imas_trees/public/%s/%s/mdsplus/9;/pfs/imasdb/imas_trees/public/models/%s/mdsplus", tokamak, version, version);
	*/
	//MERGE
    }
    else
    {
// #define ASSUME_HOME_IN_AFS 1
        char homedir[1024];
#ifdef ASSUME_HOME_IN_AFS
/*        sprintf(homedir, "/afs/efda-imas.eu/user/%c/%s", user[0], user);*/
        sprintf(homedir, "/home/ITER/%s", user);
#else
        // call "echo ~username" in a shell to get homedir of the user
        char command[1024] = "echo ~";
        strcat(command, (const char*) user);

        FILE* pipe = popen(command, "r");
        fscanf(pipe, "%s", homedir);
        pclose(pipe);
#endif
        char *treeBases[] = { treeBase0, treeBase1, treeBase2, treeBase3,
            treeBase4, treeBase5, treeBase6, treeBase7, treeBase8, treeBase9 };
        int i;
        for (i = 0; i < sizeof(treeBases) / sizeof(treeBases[0]); ++i) {
            sprintf(treeBases[i],
                    "%s/public/imasdb/%s/%s/%d",
                    homedir, tokamak, version, i);
        }
    }

    //MERGE
    // Store old values of the paths
/*    strcpy(prevTreeBase0,getenv("MDSPLUS_TREE_BASE_0"));
    strcpy(prevTreeBase1,getenv("MDSPLUS_TREE_BASE_1"));
    strcpy(prevTreeBase2,getenv("MDSPLUS_TREE_BASE_2"));
    strcpy(prevTreeBase3,getenv("MDSPLUS_TREE_BASE_3"));
    strcpy(prevTreeBase4,getenv("MDSPLUS_TREE_BASE_4"));
    strcpy(prevTreeBase5,getenv("MDSPLUS_TREE_BASE_5"));
    strcpy(prevTreeBase6,getenv("MDSPLUS_TREE_BASE_6"));
    strcpy(prevTreeBase7,getenv("MDSPLUS_TREE_BASE_7"));
    strcpy(prevTreeBase8,getenv("MDSPLUS_TREE_BASE_8"));
    strcpy(prevTreeBase9,getenv("MDSPLUS_TREE_BASE_9")); */

    ((treeBase_ = getenv("MDSPLUS_TREE_BASE_0")) != 0) ? (strcpy(prevTreeBase0,treeBase_)) : (0);
    ((treeBase_ = getenv("MDSPLUS_TREE_BASE_1")) != 0) ? (strcpy(prevTreeBase1,treeBase_)) : (0);
    ((treeBase_ = getenv("MDSPLUS_TREE_BASE_2")) != 0) ? (strcpy(prevTreeBase2,treeBase_)) : (0);
    ((treeBase_ = getenv("MDSPLUS_TREE_BASE_3")) != 0) ? (strcpy(prevTreeBase3,treeBase_)) : (0);
    ((treeBase_ = getenv("MDSPLUS_TREE_BASE_4")) != 0) ? (strcpy(prevTreeBase4,treeBase_)) : (0);
    ((treeBase_ = getenv("MDSPLUS_TREE_BASE_5")) != 0) ? (strcpy(prevTreeBase5,treeBase_)) : (0);
    ((treeBase_ = getenv("MDSPLUS_TREE_BASE_6")) != 0) ? (strcpy(prevTreeBase6,treeBase_)) : (0);
    ((treeBase_ = getenv("MDSPLUS_TREE_BASE_7")) != 0) ? (strcpy(prevTreeBase7,treeBase_)) : (0);
    ((treeBase_ = getenv("MDSPLUS_TREE_BASE_8")) != 0) ? (strcpy(prevTreeBase8,treeBase_)) : (0);
    ((treeBase_ = getenv("MDSPLUS_TREE_BASE_9")) != 0) ? (strcpy(prevTreeBase9,treeBase_)) : (0);

    // Set new values of the paths
    setenv("MDSPLUS_TREE_BASE_0",treeBase0,1);
    setenv("MDSPLUS_TREE_BASE_1",treeBase1,1);
    setenv("MDSPLUS_TREE_BASE_2",treeBase2,1);
    setenv("MDSPLUS_TREE_BASE_3",treeBase3,1);
    setenv("MDSPLUS_TREE_BASE_4",treeBase4,1);
    setenv("MDSPLUS_TREE_BASE_5",treeBase5,1);
    setenv("MDSPLUS_TREE_BASE_6",treeBase6,1);
    setenv("MDSPLUS_TREE_BASE_7",treeBase7,1);
    setenv("MDSPLUS_TREE_BASE_8",treeBase8,1);
    setenv("MDSPLUS_TREE_BASE_9",treeBase9,1);
    /*
    putenv(treeBase0);
    putenv(treeBase1);
    putenv(treeBase2);
    putenv(treeBase3);
    putenv(treeBase4);
    putenv(treeBase5);
    putenv(treeBase6);
    putenv(treeBase7);
    putenv(treeBase8);
    putenv(treeBase9);
    */
    //MERGE
}

//imas_path env valus before calling getMdsShot
static char *prevEnv;
static void restoreEnv(char *name)
{
	char logName[64];
	if(!prevEnv) return;
	sprintf(logName, "%s_path", name);
	setenv(logName,prevEnv,1);
	restoreDataEnv();
}


static int getMdsShot(char *name, int shot, int run, int translate)
{
	static int translated = 0;
	int status, runBunch = run/10000, i;
	//MERGE
	char *command, logName[64], baseName[64], *translatedBase, *currTrans, *currPattern, *tempBuf;
	int retShot, substrOfs, currLen;
	/*
	char *command, logName[64], baseName[64], *translatedBase;
	int retShot;
	*/
	//MERGE

	char *currName = malloc(strlen(name)+1);
	strcpy(currName, name);
	for(i = strlen(name) - 1; i > 0 && currName[i] == ' '; i--)
		currName[i] = 0;
	if(name && translate)
	{
		char *command;
		translated = 1;
		sprintf(logName, "%s_path", currName);
		sprintf(baseName, "MDSPLUS_TREE_BASE_%d", runBunch);

		translatedBase = getenv(baseName);
		if(translatedBase && *translatedBase)	// the path to the ITM database is correctly set?
		{
			char *prevLog = getenv(logName);
			//MERGE
			prevEnv = prevLog;
			//MERGE
			if(prevLog)							// imas_path is already set?
			{
			  //MERGE
				if (currPattern = strstr(prevLog,currTrans = getenv(baseName)))	// imas_path already contains the correct path?
				{
			//Yes, move it to the head of the list
					substrOfs = currPattern - prevLog;
					command=malloc(strlen(prevLog)+4);
					sprintf(command, "%s;", currTrans);
					currLen = strlen(command);
					memcpy(&command[currLen], prevLog, substrOfs);
					command[currLen+substrOfs] = 0;
					strcpy(&command[strlen(command)], &currPattern[strlen(currTrans)]);
					//sprintf(command, "%s", prevLog);	// then it remains the same
				}
				/*
				if (strstr(prevLog,getenv(baseName)))	// imas_path already contains the correct path?
				{
					command=malloc(strlen(prevLog)+4);
					sprintf(command, "%s", prevLog);	// then it remains the same
				}
				*/
				//MERGE
				else									// otherwise add the new path
				{
					command=malloc(strlen(getenv(baseName))+strlen(prevLog)+4);
					sprintf(command, "%s;%s", getenv(baseName), prevLog);
				}
			}
			else							// imas_path is not set yet
			{
				command=malloc(strlen(getenv(baseName))+4);
				sprintf(command, "%s", getenv(baseName));
			}
			//printf("%s\n", command);
			status = setenv(logName,command,1);
			free(command);
		}
	}
	retShot =  (shot * 10000) + (run%10000);
	free(currName);
	return retShot;
}



void imas_set_cache_level(int expIdx, int level)
{
    setCacheLevel(expIdx, level);
}

int imas_get_cache_level(int expIdx)
{
    return getCacheLevel(expIdx);
}

void imas_enable_mem_cache(int idx)
{
    imas_set_cache_level(idx, 2);
    return;
}

void imas_disable_mem_cache(int idx)
{
//reportInfo("START DISABLE MEM CACHE\n", "");
    imas_set_cache_level(idx, 0);
//reportInfo("END DISABLE MEM CACHE\n", "");
}

 void imas_flush_mem_cache(int idx)
{
//reportInfo("START FLUSH MEM CACHE\n", "");
    if(getCacheLevel(idx) == 2)
    	flushInfo(idx);
//reportInfo("END FLUSH MEM CACHE\n", "");
}

 void imas_flush_cpo_mem_cache(int idx, char *cpoPath)
{
    if(getCacheLevel(idx) == 2)
    	flushCpoInfo(idx, cpoPath);
}

 void imas_discard_mem_cache(int idx)
{
//reportInfo("START DISCARD MEM CACHE\n", "");
    if(getCacheLevel(idx) == 2)
    	invalidateAllCpoInfos(idx);
//reportInfo("END DISCARD MEM CACHE\n", "");
}

 void imas_discard_cpo_mem_cache(int idx, char *cpoPath)
{
    if(getCacheLevel(idx) == 2)
    	invalidateCpoInfo(idx, cpoPath);
}

void refreshExpCtx(char *name, int shot, int run)
{
    int i, currShot;

    currShot = getMdsShot(name, shot, run, 0);
    lock("refreshExpCtx");
    for(i = 0; i < MAX_EXPERIMENTS; i++)
    {
        if(openExperimentInfo[i].name && openExperimentInfo[i].shot == currShot && !strcmp(openExperimentInfo[i].name, name))
	{
	    int status = _TreeOpen(&openExperimentInfo[i].ctx, name, currShot, 0);
	    printf("REFRESHED %s Shot: %d Run: %d status: %d\n", name, shot, run, status);
	}
    }
    unlock();
}


static void *getExpCtx(char *name, int shot)
{
    int i;
    for(i = 0; i < MAX_EXPERIMENTS; i++)
        if(openExperimentInfo[i].name && openExperimentInfo[i].shot == shot && !strcmp(openExperimentInfo[i].name, name))
	    return openExperimentInfo[i].ctx;
    return 0;
}

static int setExpIndex(char *name, int shot, void *ctx)
{
    int i;
    for(i = 0; i < MAX_EXPERIMENTS; i++)
        if(!openExperimentInfo[i].name) break;
    if(i < MAX_EXPERIMENTS)
    {
        openExperimentInfo[i].name = malloc(strlen(name) + 1);
        strcpy(openExperimentInfo[i].name, name);
        openExperimentInfo[i].shot = shot;
	openExperimentInfo[i].ctx = ctx;
	openExperimentInfo[i].isRemote = 0;
	openExperimentInfo[i].cacheLevel = 0;
        currExperimentIdx = i;
    }
    return i;
}

static int setExpIndexRemote(char *name, int shot, int connectionId, int remoteIdx)
{
    int i;
    for(i = 0; i < MAX_EXPERIMENTS; i++)
        if(!openExperimentInfo[i].name) break;
    if(i < MAX_EXPERIMENTS)
    {
        openExperimentInfo[i].name = malloc(strlen(name) + 1);
        strcpy(openExperimentInfo[i].name, name);
        openExperimentInfo[i].shot = shot;
	openExperimentInfo[i].isRemote = 1;
	openExperimentInfo[i].connectionId = connectionId;
	openExperimentInfo[i].remoteIdx = remoteIdx;
	openExperimentInfo[i].cacheLevel = 0;
        currExperimentIdx = i;
    }
    return i;
}

static void *getExpIndex(int expIdx)
{
    if(expIdx < 0 || expIdx >= MAX_EXPERIMENTS || openExperimentInfo[expIdx].name == 0)
    {
	printf("INTERNAL ERROR: getExpIndex called for a non open experiment!!\n");
	return 0;
    }
    return openExperimentInfo[expIdx].ctx;
}

static int getCacheLevel(int expIdx)
{
    return 0;
//    if(expIdx < 0 || expIdx >= MAX_EXPERIMENTS || openExperimentInfo[expIdx].name == 0)
//    {
//	printf("INTERNAL ERROR: getExpIndex called for a non open experiment!!\n");
//	return 0;
//    }
//    return openExperimentInfo[expIdx].cacheLevel;
}

static void setCacheLevel(int expIdx, int level)
{
//    if(expIdx < 0 || expIdx >= MAX_EXPERIMENTS || openExperimentInfo[expIdx].name == 0)
//    {
//	printf("INTERNAL ERROR: getExpIndex called for a non open experiment!!\n");
//	return;
//    }
//    openExperimentInfo[expIdx].cacheLevel = level;
}

static int deleteExpIndex(int idx, char *name, int shot, void **ctx)
{
    if(idx >= MAX_EXPERIMENTS || !openExperimentInfo[idx].name)
    {
         printf("INTERNAL ERROR: deteleExpIndex called for a non existing pulse file!!!!\n");
         return 0;
    }
    free(openExperimentInfo[idx].name);
    openExperimentInfo[idx].name = 0;
    *ctx = openExperimentInfo[idx].ctx;
    if(idx == currExperimentIdx)
        currExperimentIdx = -1;
//Check for other open experiments
    for(idx = 0; idx < MAX_EXPERIMENTS; idx++)
        if(openExperimentInfo[idx].name && !strcmp(openExperimentInfo[idx].name, name) &&
		openExperimentInfo[idx].shot == shot)
            break;
    if(idx == MAX_EXPERIMENTS)
    	return 0;
    else
	return 1;
}



static int checkExpIndex(int idx)
{
    if(idx < 0 || idx >= MAX_EXPERIMENTS || !openExperimentInfo[idx].name)
    {
        sprintf(errmsg, "Invalid experiment index");
        return -1;
    }

    TreeSwitchDbid(openExperimentInfo[idx].ctx);
    currMdsShot = openExperimentInfo[idx].shot;
    currExperimentIdx = idx;
    return 0;
}

 int mdsimasOpenEnv(char *name, int shot, int run, int *retIdx, char *user, char *tokamak, char *version)
{
    int status, remoteIdx, currMdsShot;
    char *expName = "ids";

    if(currConnectionId > 0)
    {
    	status = mdsimasOpenEnvRemote(currConnectionId, name, shot, run, &remoteIdx, user, tokamak, version);
	if(status)
	    return status;
	currMdsShot = getMdsShot(name, shot, run, 0);
    	*retIdx = setExpIndexRemote(name, currMdsShot, currConnectionId, remoteIdx);
	return 0;

    }
    lock("mdsimasOpenEnv");
    setDataEnv(user, tokamak, version);
    status = mdsimasOpen(expName, shot, run, retIdx);
    //MERGE
    restoreEnv(expName);
    //MERGE
    unlock();
    return status;
}


int mdsimasConnect(char *ipAddr)
{
    currConnectionId = ConnectToMds(ipAddr);

    if(currConnectionId <= 0)
    {
        sprintf(errmsg, "Cannot connect to %s", ipAddr);
        return -1;
    }
    return 0;
}

int mdsimasDisconnect()
{
    DisconnectFromMds(currConnectionId);
    currConnectionId = 0;
    return 0;
}

 int mdsimasOpen(char *name, int shot, int run, int *retIdx)
{
	int status, i;
	void *ctx = 0;
	void *oldCtx;
	int remoteIdx, currMdsShot;

#ifdef MONITOR
	checkCacheMonitor();
#endif


    	if(currConnectionId > 0)
    	{
    		status = mdsimasOpenRemote(currConnectionId, name, shot, run, &remoteIdx);
		if(status)
	    	    return status;
		currMdsShot = getMdsShot(name, shot, run, 0);
    		*retIdx = setExpIndexRemote(name, currMdsShot, currConnectionId, remoteIdx);
		return 0;

    	}

/*	ctx = getExpCtx(name, getMdsShot(name, shot, run, 0));
	if(ctx)
	{
	    oldCtx = TreeSwitchDbid(ctx);
	    printf("OPEN RETURNED CTX %X\n", oldCtx);
	}
	else REMOVED!! LET MDSPlus create a new context every tome it is open
*/	{
            //oldCtx = TreeSwitchDbid((void *)0);
	    status = _TreeOpen(&ctx, name, getMdsShot(name, shot, run,1), 0);
	    if(!(status & 1))
	    {
		sprintf(errmsg, "Error opening pulse file %s shot: %d, run: %d. %s", name, shot, run, MdsGetMsg(mdsStatus));
		printf("%s\n", errmsg);
		return -1;
	    }
	}
	errmsg[0] = 0;
	strcpy(currExpName, name);
	currMdsShot = getMdsShot(name, shot, run, 0);
        *retIdx = setExpIndex(name, currMdsShot, ctx);
	return 0;
}

int mdsimasCreateEnv(char *name, int shot, int run, int refShot, int refRun, int *retIdx, char *user, char *tokamak, char *version)
{
    int status;
    int remoteIdx;
    int currMdsShot;
    char *expName = "ids";
    if(currConnectionId > 0)
    {
    	status = mdsimasCreateEnvRemote(currConnectionId, name, shot, run, &remoteIdx, user, tokamak, version);
	if(status)
	    return status;
	currMdsShot = getMdsShot(name, shot, run, 0);
    	*retIdx = setExpIndexRemote(name, currMdsShot, currConnectionId, remoteIdx);
	return 0;

    }

    lock("mdsimasCreateEnv");
    setDataEnv(user, tokamak, version);

     status = mdsimasCreate(expName, shot, run, refShot, refRun, retIdx);
    //MERGE
    restoreEnv(expName);
    //MERGE
    unlock();
    return status;
}



 int mdsimasCreate(char *name, int shot, int run, int refShot, int refRun, int *retIdx)
{
	int status, remoteIdx;
	int nid, nid1, refNid, currMdsShot;
	void *ctx = 0, *oldCtx;

	int refs[2];
    	DESCRIPTOR_A(refD, sizeof(int), DTYPE_L, (char *)refs, 2 * sizeof(int));
        struct descriptor refShotD = {4, DTYPE_L, CLASS_S, (char *)&refShot};
        struct descriptor refRunD = {4, DTYPE_L, CLASS_S, (char *)&refRun};

#ifdef MONITOR
	checkCacheMonitor();
#endif

   	 if(currConnectionId > 0)
    	{
    	    status = mdsimasCreateRemote(currConnectionId, name, shot, run, &remoteIdx);
	    if(status)
	        return status;
	    currMdsShot = getMdsShot(name, shot, run, 0);
    	    *retIdx = setExpIndexRemote(name, currMdsShot, currConnectionId, remoteIdx);
	    return 0;
        }

        refs[0] = refShot;
	refs[1] = refRun;
	getMdsShot(name, shot, run, 1); //Initial setting logical names
	oldCtx=TreeSwitchDbid((void *)0);
	status = TreeOpen(name, -1, 0);
	if(!(status & 1))
	{
		sprintf(errmsg, "Error opening model for %s pulse file creation: %s", name, MdsGetMsg(status));
		return -1;
	}
	status = TreeCreatePulseFile(getMdsShot(name, shot, run, 0), 0, NULL);
	if(!(status & 1))
	{
		sprintf(errmsg, "Error creating pulse file: %s", MdsGetMsg(status));
		return -1;
	}
	TreeClose(name, -1);

	oldCtx=TreeSwitchDbid((void *)0);
	status = _TreeOpen(&ctx, name, getMdsShot(name, shot, run, 0), 0);
	if(!(status & 1))
	{
		sprintf(errmsg, "Error opening created pulse file %s: %s", name, MdsGetMsg(status));
		return -1;
	}
/* Obsolete part writing the reference shot (never used) , commented out 01/08/2016 because it hangs on hpc-app1 (IMAS-396)
        mdsStatus = status = _TreeFindNode(ctx, "topinfo:ref_shot", &refNid);
	if(!(status & 1))
	{
            //topinfo:ref_shot not found, try older ref_info
            mdsStatus = status = _TreeFindNode(ctx, "ref_info", &refNid);
            if(!(status & 1))
            {
		sprintf(errmsg, "Internal error: treference information not found: %s", MdsGetMsg(status));
		return -1;
            }
            status = _TreePutRecord(ctx, refNid, (struct descriptor *)&refD, 0);
            if(!(status & 1))
            {
		sprintf(errmsg, "Error writing reference shot: %s", MdsGetMsg(status));
                return -1;
            }
        }
        else
        {
            status = _TreePutRecord(ctx, refNid, (struct descriptor *)&refShotD, 0);
            if(!(status & 1))
            {
		sprintf(errmsg, "Error writing reference shot: %s", MdsGetMsg(status));
		return -1;
            }

            mdsStatus = status = _TreeFindNode(ctx, "topinfo:ref_entry", &refNid);
            if(!(status & 1))
            {
		sprintf(errmsg, "Internal error: topinfo:ref_run not found: %s", MdsGetMsg(status));
		return -1;
            }
            status = _TreePutRecord(ctx, refNid, (struct descriptor *)&refRunD, 0);
            if(!(status & 1))
            {
		sprintf(errmsg, "Error writing reference run: %s", MdsGetMsg(status));
		return -1;
            }
         } */

        currMdsShot = getMdsShot(name, shot, run, 0);
	strcpy(currExpName, name);
        *retIdx = setExpIndex(name, currMdsShot, ctx);
	return 0;
}


int mdsimasClose(int idx, char *name, int shot, int run)
{
    int status;
    void *ctx;

    lock("mdsimasClose");
    if(checkExpIndex(idx) < 0)
    {
    	unlock();
	return -1;
    }
    //MERGE
    invalidateAllCpoInfos(idx);
    //MERGE
    if(isExpRemote(idx) > 0)
    {
    	if(deleteExpIndex(idx, name, getMdsShot(name, shot, run, 0), &ctx)== 0)
    	    status = mdsimasCloseRemote(getExpConnectionId(idx), getExpRemoteIdx(idx), name, shot, run);
	else
	    status = 0;
	unlock();
	return status;
    }
    if(deleteExpIndex(idx, name, getMdsShot(name, shot, run, 0), &ctx) == 0)
    {
        status = TreeClose(name, getMdsShot(name, shot, run, 0));
    }
    else
        status = 1;
    if(!(status & 1)) {
        sprintf(errmsg, "Error closing %s, shot: %d, run: %d: %s", name, shot, run, MdsGetMsg(status));
	unlock();
        return -1;
    }
    unlock();
    return 0;
}


// User for passing path to MDSplus which are limited to 12 char
// Keep the integrity of the first 8 characters
// Then sum the ASCII codes of the other characters and store them in the name
static char *path2MDS(char *path)
{
    static char MDSpath[13];
    char SUM[10];
    int i, sum;
    int len = strlen(path);

//    printf("Begin path2MDSpath = %s\n",path);
    strncpy(MDSpath,path,8);
    MDSpath[8] = '\0';
    sum = 0;
    for(i = 8; i < len; i++) {
	sum += tolower(path[i]);
    }
    sprintf(SUM,"%d",sum);
    strncat(MDSpath,SUM,4);
    for (i=0;i<12;i++) {
	MDSpath[i] = tolower(MDSpath[i]);
    }
//    printf("path = %s, MDSpath = %s\n",path, MDSpath);
    return  MDSpath;
}

#define SEPARATORS "/."  /* separators for cpoPath */
static char *convertPath(char *inputpath)
{
    static char mdsCpoPath[2048];
    char  path[2048];;
    char *ptr;

    strcpy(path,inputpath);
    ptr=strtok(path, SEPARATORS);
    if (ptr == NULL) return "";
    strcpy(mdsCpoPath,"");
    if (!isdigit(*ptr)) {
      strcat(mdsCpoPath,path2MDS(ptr));
    }
    else {
      strcat(mdsCpoPath,ptr);
    }
    while ( (ptr=strtok(NULL, SEPARATORS)) != NULL) {
	strcat(mdsCpoPath,"."); /* . */
	if (!isdigit(*ptr)) {
          strcat(mdsCpoPath,path2MDS(ptr));
	}
	else {
	  strcat(mdsCpoPath,ptr);
	}
    }
    //printf("mds+.convertPath. modified path: %s\n",mdsCpoPath);
    return mdsCpoPath;
}

static char *mdsconvertPath(char *inputpath)
{

    static char mdsCpoPath[2048];
    char  path[2048];
    char *ptr;
    int   i;

    strcpy(path,inputpath);
    ptr=strtok(path, SEPARATORS);
    if (ptr == NULL) return "";
    strcpy(mdsCpoPath,"");
    if (!isdigit(*ptr)) {
      strcat(mdsCpoPath,path2MDS(ptr));
    }
    else {
      strcat(mdsCpoPath,ptr);
    }
    while ( (ptr=strtok(NULL, SEPARATORS)) != NULL) {
	strcat(mdsCpoPath,".");
	if (!isdigit(*ptr)) {
          strcat(mdsCpoPath,path2MDS(ptr));
	}
	else {
	  strcat(mdsCpoPath,ptr);
	}
    }
    for(i = strlen(mdsCpoPath) - 1; i >=0; i--){
	if (mdsCpoPath[i] == '.'){
	  mdsCpoPath[i] = ':';
	  break;
	}
    }
    //printf("mds+.mdsconvertPath. modified path: %s\n",mdsCpoPath);
    return mdsCpoPath;
}

static int getNid(char *cpoPath, char *path)
{
	int i, len, status, nid;
	char *mdsPath;

        //printf("ual_low_level_mdsplus: GetNid : %s, %s\n", cpoPath, path);
	len = strlen(cpoPath) + strlen(path)+ 1;
	mdsPath = malloc(len + 1);
	sprintf(mdsPath, "%s/%s", cpoPath, path);
        //printf("getNid: mdspath : %s\n", mdsPath);
	status = TreeFindNode(mdsconvertPath(mdsPath), &nid);
        //printf("ual_low_level_mdsplus: status in GetNid : %d\n", status);
	if(!(status & 1))
	{
		free(mdsPath);
		sprintf(errmsg, "Node at path %s not found: %s", path, MdsGetMsg(status));
		return -1;
	}

	free(mdsPath);
	return nid;
}

static int mdsgetNid(char *cpoPath, char *path)
{
	int i, len, status, nid;
	char *mdsPath;
        //printf("ual_low_level_mdsplus: mdsGetNid : %s, %s\n", cpoPath, path);

	len = strlen(convertPath(cpoPath)) + strlen(path)+ 1;
	mdsPath = malloc(len + 1);
	sprintf(mdsPath, "%s.%s", convertPath(cpoPath), path);
	for(i = 0; i < len; i++)
	{
		if(mdsPath[i] == '/')
			mdsPath[i] = '.';
		mdsPath[i] = tolower(mdsPath[i]);
	}

	for(i = len - 1; i >=0; i--)
	{
		if(mdsPath[i] == '.')
		{
			mdsPath[i] = ':';
			break;
		}
	}

        //Truncation to 12 chars
        for(i = strlen(mdsPath) -1; i >=0 && mdsPath[i] != ':'; i--);
        //printf("ual_low_level_mdsplus: i & len in mdsGetNid : %d, %d\n", i, len);
        if(strlen(mdsPath) - i - 1 > 12)
            mdsPath[i+13] = 0;
        //printf("mdsgetNid. mdspath : %s\n", mdsPath);

	status = TreeFindNode(mdsPath, &nid);
        //printf("ual_low_level_mdsplus: status in mdsGetNid : %d\n", status);
	if(!(status & 1))
	{
		free(mdsPath);
		sprintf(errmsg, "Node at path %s not found: %s", path, MdsGetMsg(status));
		return -1;
	}
	free(mdsPath);
	return nid;
}


static int getDataLocal(int expIdx, char *cpoPath, char *path, struct descriptor_xd *retXd, int evaluate)
{
	int status, numSegments = 0;
	int nid, refNid, *refData, refShot, isObject;
	EMPTYXD(xd);
	EMPTYXD(startXd);
	EMPTYXD(endXd);
	struct descriptor_a *refD;
	struct descriptor_xd *xdPtr;

       	lock("getDataLocal");
        checkExpIndex(expIdx);

//	printf("getDataLocal for %s %s\n",cpoPath, path);
        nid = getNid(cpoPath, path);
	//printf("In getDataLocal: nid for %s %s = %d\n",cpoPath, path,nid);
	if(nid == -1)
	{
	    unlock();
	    return -1;
	}
        status = TreeGetNumSegments(nid, &numSegments);
        if(!(status & 1))
        {
		sprintf(errmsg, "Missing data for IDS: %s, field: %s - %s", cpoPath, path, MdsGetMsg(status));
                //printf("ERROR %s\n", errmsg);
	    	unlock();
		return -1;
        }
        if(numSegments > 0)
        {
//NOTE Segmented data encountered only when getDataLocal is called by getCpoDataServer in remote IDS access

            EMPTYXD(retTimesXd);
//Check the dtype of start segment: if DTYPE_DOUBLE it is a segment containing normal slices
//if DTYPE_L it is a segment containing an object slice
 	    status = TreeGetSegmentLimits(nid, 0, &startXd, &endXd);
	    if(!(status & 1))
	    {
		sprintf(errmsg, "Cannot get segment limits for IDS: %s, field: %s - %s", cpoPath, path, MdsGetMsg(status));
		unlock();
                printf("ERROR %s\n", errmsg);
		return -1;
	    }
	    isObject = (startXd.pointer->dtype == DTYPE_L);
	    MdsFree1Dx(&startXd, 0);
	    MdsFree1Dx(&endXd, 0);
	    unlock();

	    if(isObject)
	    {
		status = getObjectLocal(expIdx, cpoPath, path,  (void **)&xdPtr, 0, 0);
		*retXd = *xdPtr;
		free((char *)xdPtr);
	    }
	    else
           	status = getTimedData(expIdx, cpoPath, path, 0, 0, retXd, &retTimesXd, 1);
            if(!status)
                MdsFree1Dx(&retTimesXd, 0);
            return status;
        }
        mdsStatus = status = TreeGetRecord(nid, retXd);

        if(!(status & 1) || retXd->l_length == 0) //Empty value has been written into the tree
	{
		sprintf(errmsg, "Missing data for IDS: %s, field: %s - %s", cpoPath, path, MdsGetMsg(status));
              //  printf("ERROR %s\n", errmsg);

	    	unlock();
		return -1;
	}
	if((status & 1)&&evaluate && retXd->pointer->class != CLASS_APD) status = TdiData(retXd, retXd MDS_END_ARG);
	unlock();
	return 0;
}

static int mdsgetDataLocal(int expIdx, char *cpoPath, char *path, struct descriptor_xd *retXd, int evaluate)
{
	int status, numSegments = 0;
	int nid, refNid, *refData, refShot, isObject;
	EMPTYXD(xd);
	EMPTYXD(startXd);
	EMPTYXD(endXd);
	struct descriptor_a *refD;
	struct descriptor_xd *xdPtr;

       	lock("mdsgetDataLocal");
        checkExpIndex(expIdx);

        nid = mdsgetNid(cpoPath, path);
	// printf("mdsgetNid. nid for %s %s = %d\n",cpoPath, path,nid);
	if(nid == -1)
	{
	    unlock();
	    return -1;
	}
        status = TreeGetNumSegments(nid, &numSegments);
        if(!(status & 1))
        {
		sprintf(errmsg, "Missing data for IDS: %s, field: %s - %s", cpoPath, path, MdsGetMsg(status));
	    	unlock();
		return -1;
        }
        if(numSegments > 0)
        {
//NOTE Segmented data encountered only when getDataLocal is called by getCpoDataServer in remote IDS access	

            EMPTYXD(retTimesXd);
//Check the dtype of start segment: if DTYPE_DOUBLE it is a segment containing normal slices
//if DTYPE_L it is a segment containing an object slice
 	    status = TreeGetSegmentLimits(nid, 0, &startXd, &endXd);
	    if(!(status & 1))
	    {
		sprintf(errmsg, "Cannot get segment limits for IDS: %s, field: %s - %s", cpoPath, path, MdsGetMsg(status));
		unlock();
		return -1;
	    }
	    isObject = (startXd.pointer->dtype == DTYPE_L);
	    MdsFree1Dx(&startXd, 0);
	    MdsFree1Dx(&endXd, 0);
	    unlock();

	    if(isObject)
	    {
		//printf("IS OBJECT: %s %s\n", cpoPath, path);

		status = mdsgetObjectLocal(expIdx, cpoPath, path,  (void **)&xdPtr, 0, 0);
		*retXd = *xdPtr;
		free((char *)xdPtr);
	    }
	    else {
		//printf("In mdsetDataLocal. before mdsgetTimedData. cpopath: %s, path: %s\n",cpoPath, path);
           	status = mdsgetTimedData(expIdx, cpoPath, path, 0, 0, retXd, &retTimesXd, 1);
	    }
            if(!status)
                MdsFree1Dx(&retTimesXd, 0);
            return status;
        }
        mdsStatus = status = TreeGetRecord(nid, retXd);

        if(!(status & 1) || retXd->l_length == 0) //Empty value has been written into the tree
	{
		sprintf(errmsg, "Missing data for IDS: %s, field: %s - %s", cpoPath, path, MdsGetMsg(status));
	    	unlock();
		return -1;
	}
	if((status & 1)&&evaluate && retXd->pointer->class != CLASS_APD) status = TdiData(retXd, retXd MDS_END_ARG);
	unlock();
	return 0;
}


//Get Local Data without descriptors NOTE: to be used only for arrays

static int getDataLocalNoDescr(int expIdx, char *cpoPath, char *path, void **retData, int *retDims,  int *retDimsCt, char *retType, char *retClass, int *retLength, int64_t *retArsize, int evaluate)
{
	int status, numSegments = 0;
	int nid, refNid, *refData, refShot, i;
	EMPTYXD(xd);
	struct descriptor_a *refD;
    	ARRAY_COEFF(char, 16) *dataDPtr;

       	lock("getDataLocalNoDescr");
        checkExpIndex(expIdx);


        nid = getNid(cpoPath, path);
/*	printf("nid for %s %s = %d\n",cpoPath, path,nid); */
	if(nid == -1)
	{
	    unlock();
	    return -1;
	}
        status = TreeGetNumSegments(nid, &numSegments);
        if(!(status & 1))
        {
		sprintf(errmsg, "Missing data for IDS: %s, field: %s - %s", cpoPath, path, MdsGetMsg(status));
	    	unlock();
		return -1;
        }
        if(numSegments > 0)
        {
            EMPTYXD(retTimesXd);
	    unlock();
            status = getTimedDataNoDescr(expIdx, cpoPath, path, 0, 0, retData, retDims,  retDimsCt, retType, retClass, retLength, retArsize, &retTimesXd, 1);
            if(!status)
                MdsFree1Dx(&retTimesXd, 0);
            return status;
        }

        mdsStatus = status = TreeGetRecord(nid, &xd);

        if(!(status & 1) || xd.l_length == 0) //If empty value has been written into the tree, do not scal reference list
	{
		sprintf(errmsg, "Missing data for IDS: %s, field: %s - %s", cpoPath, path, MdsGetMsg(status));
	    	unlock();
		return -1;
	}
	if((status & 1)&&evaluate) status = TdiData(&xd, &xd MDS_END_ARG);
	if(!xd.pointer)
	{
	    unlock();
	    return -1;
	}
	dataDPtr = (void *)xd.pointer;
	if(dataDPtr->class != CLASS_A)
	{
	    printf("INTERNAL ERROR: getDataLocalNonDescr called for non array data!!!\n");
	    unlock();
	    return -1;
	}
	*retType = dataDPtr->dtype;
	*retClass = dataDPtr->class;
	*retLength = dataDPtr->length;
	*retDimsCt = dataDPtr->dimct;
	if(dataDPtr->dimct == 1) //Single dimension array
	{
	  //MERGE
		if(dataDPtr->arsize > 0)
			retDims[0] = dataDPtr->arsize/dataDPtr->length;
		else // No data
		{
		    MdsFree1Dx(&xd, 0);
		    return -1;
		}
		/*
		retDims[0] = dataDPtr->arsize/dataDPtr->length;
		*/
		//MERGE
	}
	else
	{
		for(i = 0; i < dataDPtr->dimct; i++)
			retDims[i] = dataDPtr->m[i];
	}
	*retData = malloc(dataDPtr->arsize);
	*retArsize = dataDPtr->arsize;
	memcpy(*retData, dataDPtr->pointer, dataDPtr->arsize);
	MdsFree1Dx(&xd, 0);
	unlock();
	return 0;
}

static int getNonSegmentedTimedData(int expIdx, char *cpoPath, char *path, struct descriptor_xd *retDataXd,
    struct descriptor_xd *retTimesXd)
{
    struct descriptor_signal *signalD;
    struct descriptor_a *longTimesD;
    EMPTYXD(xd);
    int status, nTimes, i;
    float currTime;
    DESCRIPTOR_A(timeD, sizeof(double), DTYPE_DOUBLE, 0, 0);
    double *times;
    uint64_t *longTimes;

    status = getData(expIdx, cpoPath, path, &xd, 0);
    if(status) return status;
    signalD = (struct descriptor_signal *)xd.pointer;
    if(signalD->dtype != DTYPE_SIGNAL)
    {
            sprintf(errmsg, "Non signal returned from timed data at path %s: %s ", path, cpoPath);
            MdsFree1Dx(&xd, 0);
            return -1;
    }
    //Time conversion
    longTimesD = (struct descriptor_a *)signalD->dimensions[0];
    if(longTimesD->dtype != DTYPE_QU)
    {
            sprintf(errmsg, "Unexpected timebase data type at path %s: %s ", path, cpoPath);
            MdsFree1Dx(&xd, 0);
            return -1;
    }

    nTimes = longTimesD->arsize/longTimesD->length;
    times = malloc(sizeof(double) * nTimes);
    longTimes = (uint64_t *)longTimesD->pointer;
    for(i = 0; i < nTimes; i++)
    {
        MdsTimeToDouble(longTimes[i], (void *)&currTime);
        times[i] = currTime;
    }
    timeD.arsize = nTimes * sizeof(double);
    timeD.pointer = (char *)times;
    MdsCopyDxXd((struct descriptor *)&timeD, retTimesXd);
    MdsCopyDxXd((struct descriptor *)signalD->data, retDataXd);
    free((char *)times);
    MdsFree1Dx(&xd, 0);
    return 0;
}

static int mdsgetNonSegmentedTimedData(int expIdx, char *cpoPath, char *path, struct descriptor_xd *retDataXd,
    struct descriptor_xd *retTimesXd)
{
    struct descriptor_signal *signalD;
    struct descriptor_a *longTimesD;
    EMPTYXD(xd);
    int status, nTimes, i;
    double currTime;
    DESCRIPTOR_A(timeD, sizeof(double), DTYPE_DOUBLE, 0, 0);
    double *times;
    uint64_t *longTimes;

    //printf("mdsgetNonSegmentedTimedData1. cpoPath: %s, path: %s\n",cpoPath, path);
    status = mdsgetData(expIdx, cpoPath, path, &xd, 0);
    if(status) return status;
    signalD = (struct descriptor_signal *)xd.pointer;
    if(signalD->dtype != DTYPE_SIGNAL)
    {
            sprintf(errmsg, "Non signal returned from timed data at path %s: %s ", path, cpoPath);
            MdsFree1Dx(&xd, 0);
            return -1;
    }
    //Time conversion
    longTimesD = (struct descriptor_a *)signalD->dimensions[0];
    if(longTimesD->dtype != DTYPE_QU)
    {
            sprintf(errmsg, "Unexpected timebase data type at path %s: %s ", path, cpoPath);
            MdsFree1Dx(&xd, 0);
            return -1;
    }

    nTimes = longTimesD->arsize/longTimesD->length;
    times = malloc(sizeof(double) * nTimes);
    longTimes = (uint64_t *)longTimesD->pointer;
    for(i = 0; i < nTimes; i++)
    {
        MdsTimeToDouble(longTimes[i], &currTime);
        times[i] = currTime;
    }
    timeD.arsize = nTimes * sizeof(double);
    timeD.pointer = (char *)times;
    MdsCopyDxXd((struct descriptor *)&timeD, retTimesXd);
    MdsCopyDxXd((struct descriptor *)signalD->data, retDataXd);
    free((char *)times);
    MdsFree1Dx(&xd, 0);
    return 0;
}

static int getNonSegmentedTimedDataNoDescr(int expIdx, char *cpoPath, char *path, void **retData, int *retDims,  int *retDimsCt, char *retType, char *retClass, int *retLength,  int64_t *retArsize,
    struct descriptor_xd *retTimesXd)
{
    struct descriptor_signal *signalD;
    struct descriptor_a *longTimesD;
    EMPTYXD(xd);
    int status, nTimes, i;
    double currTime;
    DESCRIPTOR_A(timeD, sizeof(double), DTYPE_DOUBLE, 0, 0);
    double *times;
    uint64_t *longTimes;
    ARRAY_COEFF(char, 16) *dataDPtr;

    status = getData(expIdx, cpoPath, path, &xd, 0);
    if(status) return status;
    signalD = (struct descriptor_signal *)xd.pointer;
    if(signalD->dtype != DTYPE_SIGNAL)
    {
            sprintf(errmsg, "Non signal returned from timed data at path %s: %s ", path, cpoPath);
            MdsFree1Dx(&xd, 0);
            return -1;
    }
    //Time conversion
    longTimesD = (struct descriptor_a *)signalD->dimensions[0];
    if(longTimesD->dtype != DTYPE_QU)
    {
            sprintf(errmsg, "Unexpected timebase data type at path %s: %s ", path, cpoPath);
            MdsFree1Dx(&xd, 0);
            return -1;
    }

    nTimes = longTimesD->arsize/longTimesD->length;
    times = malloc(sizeof(double) * nTimes);
    longTimes = (uint64_t *)longTimesD->pointer;
    for(i = 0; i < nTimes; i++)
    {
        MdsTimeToDouble(longTimes[i], &currTime);
        times[i] = currTime;
    }
    timeD.arsize = nTimes * sizeof(double);
    timeD.pointer = (char *)times;
    MdsCopyDxXd((struct descriptor *)&timeD, retTimesXd);

    dataDPtr =  (void *)signalD->data;
    *retDimsCt = dataDPtr->dimct;
    for(i = 0; i < dataDPtr->dimct; i++)
        retDims[i] = dataDPtr->m[i];
    *retLength = dataDPtr->length;
    *retArsize = dataDPtr->arsize;
    *retType = dataDPtr->dtype;
    *retClass = dataDPtr->class;
    *retData = malloc(dataDPtr->arsize);
    memcpy(*retData, dataDPtr->pointer, dataDPtr->arsize);



    //MdsCopyDxXd((struct descriptor *)signalD->data, retDataXd);
    free((char *)times);
    MdsFree1Dx(&xd, 0);
    return 0;
}

static double getDoubleTime(struct descriptor *timeD)
{
    double retTime;

    if(timeD->dtype == DTYPE_Q || timeD->dtype == DTYPE_QU)
        MdsTimeToDouble(*(uint64_t *)timeD->pointer, &retTime);
    else
        retTime = *(double *)timeD->pointer;
    return retTime;
}



int getTimedData(int expIdx, char *cpoPath, char *path, double start, double end, struct descriptor_xd *retDataXd,
    struct descriptor_xd *retTimesXd, int all)
{
    struct descriptor_a *longTimesD;
    EMPTYXD(emptyXd);
    EMPTYXD(startXd);
    EMPTYXD(endXd);
    int status, i; //nTimes, i;
    int lastSegmentOffset, leftItems, leftRows, lastDim;
    double *times, *doubleTimes;
    uint64_t *longTimes;
    double currStart, currEnd;
    int nid, numSegments, currSegment, actSegments;
    struct descriptor_xd *segTimesXds, *segDataXds;
    uint64_t dataLen;
    uint64_t timesLen, dataOfs, timesOfs, currNTimes;
    char *data;
    DESCRIPTOR_A_COEFF(dataD, 0, 0, 0, 100, 0);
    //ARRAY_COEFF(char, 16) dataD;
    ARRAY_COEFF(char, 16) *dataDPtr;
    DESCRIPTOR_A(timesD, sizeof(double), DTYPE_DOUBLE, 0, 0);

    lock("getTimedData");
    checkExpIndex(expIdx);

        nid = getNid(cpoPath, path);
    //printf("In GetTimedData: cpopath, path, nid: %s, %s, %d \n",cpoPath, path, nid);
    status = TreeGetNumSegments(nid, &numSegments);
    if(!(status & 1))
    {
	sprintf(errmsg, "Error getting number of segments for IDS: %s, field: %s - %s", cpoPath, path, MdsGetMsg(status));
	unlock();
	return -1;
    }
    if(numSegments == 0)
    {
	unlock();
        status = getNonSegmentedTimedData(expIdx, cpoPath, path, retDataXd, retTimesXd);
	return status;
    }

    segTimesXds = (struct descriptor_xd *)malloc(sizeof(struct descriptor_xd)*numSegments);
    segDataXds = (struct descriptor_xd *)malloc(sizeof(struct descriptor_xd)*numSegments);
    actSegments = 0;
    lastDim = 0;
//    printf("Get Timed Data 1\n");
    for(currSegment = 0; currSegment < numSegments; currSegment++)
    {
        status = TreeGetSegmentLimits(nid, currSegment, &startXd, &endXd);
        if(!(status & 1))
        {
            printf("Error in GetTimedData, due to TreeGetSegmentLimits\n");
            sprintf(errmsg, "Error getting segment limits for IDS: %s, field: %s", cpoPath, path);
	    unlock();
            return -1;
        }
        currStart = getDoubleTime(startXd.pointer);
        currEnd = getDoubleTime(endXd.pointer);
 //      printf("Get Timed Data 3, currsegment, currstart, currend: %d, %lf, %lf\n", currSegment, currStart, currEnd);
        MdsFree1Dx(&startXd, 0);
        MdsFree1Dx(&endXd, 0);
        if(!all && (currStart >= end))
            break;
        if(all || (currEnd >= start) || currSegment == numSegments - 1)
        {
            segTimesXds[actSegments]= emptyXd;
            segDataXds[actSegments]= emptyXd;
            status = TreeGetSegment(nid, currSegment, &segDataXds[actSegments], &segTimesXds[actSegments]);
            if(!(status & 1))
            {
             printf("Error in GetTimedData, due to TreeGetSegment\n");
                sprintf(errmsg, "Error getting segment for IDS: %s, field: %s", cpoPath, path);
	        unlock();
                return -1;
            }

            dataDPtr = (void *)segDataXds[actSegments].pointer;
            if(dataDPtr->dimct == 1)
                lastDim += dataDPtr->arsize/dataDPtr->length;
            else
                lastDim += dataDPtr->m[dataDPtr->dimct - 1];


            //Times is now an expression
            status = TdiData( &segTimesXds[actSegments],  &segTimesXds[actSegments] MDS_END_ARG);
            if(!(status & 1))
            {
            printf("Error in GetTimedData, due to TdiData evaluation\n");
               sprintf(errmsg, "Error getting segment time for IDS: %s, field: %s", cpoPath, path);
	        unlock();
                return -1;
            }


            actSegments++;
        }
    }
  //  printf("Get Timed Data 2\n");
    //Merge data and times, converted to double
    dataLen = timesLen = 0;
    dataDPtr = (void *)segDataXds[0].pointer;
    dataD.class = CLASS_A;
    dataD.dtype = dataDPtr->dtype;
    dataD.length = segDataXds[0].pointer->length;
    dataD.dimct = dataDPtr->dimct;
    for(i = 0; i < dataD.dimct; i++)
        dataD.m[i] = dataDPtr->m[i];
// The last dimension is multiplied fir the number of segments QUESTO CONTO LO DEVO FARE COME SOMMA DELL'ULTIMA DIMENSIONE!!
//    dataD.m[dataD.dimct-1] *= numSegments;
    dataD.m[dataD.dimct-1] = lastDim;

    for(currSegment = 0; currSegment < actSegments; currSegment++)
    {
        dataLen += ((struct descriptor_a *)segDataXds[currSegment].pointer)->arsize;
        timesLen += ((struct descriptor_a *)segTimesXds[currSegment].pointer)->arsize;
    }
    times = (double *)malloc(sizeof(double) * timesLen/sizeof(uint64_t));
    data = malloc(dataLen);
    dataD.arsize = dataLen;
    dataD.pointer = data;
    timesD.pointer = (char *)times;
    dataOfs = 0;
    timesOfs = 0;
    lastSegmentOffset = 0;
    leftRows = 0;
    for(currSegment = 0; currSegment < actSegments; currSegment++)
    {
        //Check whether last segment is not full
        if(currSegment == numSegments - 1)
        {
            getLeftItems(nid, &leftItems, &leftRows);
            lastSegmentOffset = leftItems * ((struct descriptor_a *)segDataXds[currSegment].pointer)->length;
        }

        memcpy(&data[dataOfs], ((struct descriptor_a *)segDataXds[currSegment].pointer)->pointer,
            ((struct descriptor_a *)segDataXds[currSegment].pointer)->arsize);
        dataOfs += ((struct descriptor_a *)segDataXds[currSegment].pointer)->arsize - lastSegmentOffset;
        if(segTimesXds[currSegment].pointer->dtype == DTYPE_Q || segTimesXds[currSegment].pointer->dtype == DTYPE_QU)
        {
            currNTimes = ((struct descriptor_a *)segTimesXds[currSegment].pointer)->arsize/sizeof(uint64_t);
	    /////////////////ADDED///////////
            if(currNTimes < leftRows)
            {
                //printf("Internal error in getTimedData: inconsistent number of samples in data and times\n");
                sprintf(errmsg, "Internal error in getTimedData: inconsistent number of samples in data and times");
                unlock();
                return -1;
            }
            //////////////////////////
            longTimes = (uint64_t *)((struct descriptor_a *)segTimesXds[currSegment].pointer)->pointer;

            for(i = 0; i < currNTimes - leftRows; i++)
                MdsTimeToDouble(longTimes[i], &times[timesOfs++]);
        }
        else
        {
            currNTimes = ((struct descriptor_a *)segTimesXds[currSegment].pointer)->arsize/sizeof(double);
	    ////////ADDED//////////
            if(currNTimes < leftRows)
            {
                //printf("Internal error in getTimedData: inconsistent number of samples in data and times\n");
                sprintf(errmsg, "Internal error in getTimedData: inconsistent number of samples in data and times");
                unlock();
                return -1;
            }
            //////////////////////////
            doubleTimes = (double *)((struct descriptor_a *)segTimesXds[currSegment].pointer)->pointer;
            for(i = 0; i < currNTimes - leftRows; i++)
                times[timesOfs++] = doubleTimes[i];
        }
    }

    dataD.arsize -= lastSegmentOffset;
    dataD.m[dataD.dimct - 1] -= leftRows;
    MdsCopyDxXd((struct descriptor *)&dataD, retDataXd);
    timesD.arsize = timesOfs * sizeof(double);
    MdsCopyDxXd((struct descriptor *)&timesD, retTimesXd);
    free((char *)times);
    free((char *)data);
    for(currSegment = 0; currSegment < actSegments; currSegment++)
    {
        MdsFree1Dx(&segDataXds[currSegment], 0);
        MdsFree1Dx(&segTimesXds[currSegment], 0);
    }
    free((char *)segDataXds);
    free((char *)segTimesXds);
    unlock();
    return 0;
}

int mdsgetTimedData(int expIdx, char *cpoPath, char *path, double start, double end, struct descriptor_xd *retDataXd,
    struct descriptor_xd *retTimesXd, int all)
{
    struct descriptor_a *longTimesD;
    EMPTYXD(emptyXd);
    EMPTYXD(startXd);
    EMPTYXD(endXd);
    int status, i; //nTimes, i;
    int lastSegmentOffset, leftItems, leftRows, lastDim;
    double *times, *doubleTimes;
    uint64_t *longTimes;
    double currStart, currEnd;
    int nid, numSegments, currSegment, actSegments;
    struct descriptor_xd *segTimesXds, *segDataXds;
    int dataLen, timesLen, dataOfs, timesOfs, currNTimes;
    char *data;
    DESCRIPTOR_A_COEFF(dataD, 0, 0, 0, 100, 0);
    //ARRAY_COEFF(char, 16) dataD;
    ARRAY_COEFF(char, 16) *dataDPtr;
    DESCRIPTOR_A(timesD, sizeof(double), DTYPE_DOUBLE, 0, 0);

    lock("mdsgetTimedData");
    checkExpIndex(expIdx);

    nid = mdsgetNid(cpoPath, path);
    //printf("In mdsGetTimedData: cpopath, path, nid: %s, %s, %d \n",cpoPath, path, nid);

    status = TreeGetNumSegments(nid, &numSegments);
    if(!(status & 1))
    {
	sprintf(errmsg, "Error getting number of segments for IDS: %s, field: %s - %s", cpoPath, path, MdsGetMsg(status));
	unlock();
	return -1;
    }
    if(numSegments == 0)
    {
	unlock();
        status = mdsgetNonSegmentedTimedData(expIdx, cpoPath, path, retDataXd, retTimesXd);
	return status;
    }

    segTimesXds = (struct descriptor_xd *)malloc(sizeof(struct descriptor_xd)*numSegments);
    segDataXds = (struct descriptor_xd *)malloc(sizeof(struct descriptor_xd)*numSegments);
    actSegments = 0;
    lastDim = 0;
//    printf("Get Timed Data 1\n");
    for(currSegment = 0; currSegment < numSegments; currSegment++)
    {
        status = TreeGetSegmentLimits(nid, currSegment, &startXd, &endXd);
        if(!(status & 1))
        {
            printf("Error in mdsGetTimedData, due to TreeGetSegmentLimits\n");
            sprintf(errmsg, "Error getting segment limits for IDS: %s, field: %s", cpoPath, path);
	    unlock();
            return -1;
        }
        currStart = getDoubleTime(startXd.pointer);
        currEnd = getDoubleTime(endXd.pointer);
 //      printf("Get Timed Data 3, currsegment, currstart, currend: %d, %lf, %lf\n", currSegment, currStart, currEnd);
        MdsFree1Dx(&startXd, 0);
        MdsFree1Dx(&endXd, 0);
        if(!all && (currStart >= end))
            break;
        if(all || (currEnd >= start) || currSegment == numSegments - 1)
        {
            segTimesXds[actSegments]= emptyXd;
            segDataXds[actSegments]= emptyXd;
            status = TreeGetSegment(nid, currSegment, &segDataXds[actSegments], &segTimesXds[actSegments]);
            if(!(status & 1))
            {
             printf("Error in mdsGetTimedData, due to TreeGetSegment\n");
                sprintf(errmsg, "Error getting segment for IDS: %s, field: %s", cpoPath, path);
	        unlock();
                return -1;
            }

            dataDPtr = (void *)segDataXds[actSegments].pointer;
            if(dataDPtr->dimct == 1)
                lastDim += dataDPtr->arsize/dataDPtr->length;
            else
                lastDim += dataDPtr->m[dataDPtr->dimct - 1];


            //Times is now an expression
            status = TdiData( &segTimesXds[actSegments],  &segTimesXds[actSegments] MDS_END_ARG);
            if(!(status & 1))
            {
            printf("Error in mdsGetTimedData, due to TdiData evaluation\n");
               sprintf(errmsg, "Error getting segment time for IDS: %s, field: %s", cpoPath, path);
	        unlock();
                return -1;
            }


            actSegments++;
        }
    }
  //  printf("Get Timed Data 2\n");
    //Merge data and times, converted to double
    dataLen = timesLen = 0;
    dataDPtr = (void *)segDataXds[0].pointer;
    dataD.class = CLASS_A;
    dataD.dtype = dataDPtr->dtype;
    dataD.length = segDataXds[0].pointer->length;
    dataD.dimct = dataDPtr->dimct;
    for(i = 0; i < dataD.dimct; i++)
        dataD.m[i] = dataDPtr->m[i];
// The last dimension is multiplied fir the number of segments QUESTO CONTO LO DEVO FARE COME SOMMA DELL'ULTIMA DIMENSIONE!!
//    dataD.m[dataD.dimct-1] *= numSegments;
    dataD.m[dataD.dimct-1] = lastDim;

    for(currSegment = 0; currSegment < actSegments; currSegment++)
    {
        dataLen += ((struct descriptor_a *)segDataXds[currSegment].pointer)->arsize;
        timesLen += ((struct descriptor_a *)segTimesXds[currSegment].pointer)->arsize;
    }
    times = (double *)malloc(sizeof(double) * timesLen/sizeof(uint64_t));
    data = malloc(dataLen);
    dataD.arsize = dataLen;
    dataD.pointer = data;
    timesD.pointer = (char *)times;
    dataOfs = 0;
    timesOfs = 0;
    lastSegmentOffset = 0;
    leftRows = 0;
    for(currSegment = 0; currSegment < actSegments; currSegment++)
    {
        //Check whether last segment is not full
        if(currSegment == numSegments - 1)
        {
            getLeftItems(nid, &leftItems, &leftRows);
            lastSegmentOffset = leftItems * ((struct descriptor_a *)segDataXds[currSegment].pointer)->length;
        }

        memcpy(&data[dataOfs], ((struct descriptor_a *)segDataXds[currSegment].pointer)->pointer,
            ((struct descriptor_a *)segDataXds[currSegment].pointer)->arsize);
        dataOfs += ((struct descriptor_a *)segDataXds[currSegment].pointer)->arsize - lastSegmentOffset;
        if(segTimesXds[currSegment].pointer->dtype == DTYPE_Q || segTimesXds[currSegment].pointer->dtype == DTYPE_QU)
        {
            currNTimes = ((struct descriptor_a *)segTimesXds[currSegment].pointer)->arsize/sizeof(uint64_t);
            longTimes = (uint64_t *)((struct descriptor_a *)segTimesXds[currSegment].pointer)->pointer;
            for(i = 0; i < currNTimes - leftRows; i++)
                MdsTimeToDouble(longTimes[i], &times[timesOfs++]);
        }
        else
        {
            currNTimes = ((struct descriptor_a *)segTimesXds[currSegment].pointer)->arsize/sizeof(double);
            doubleTimes = (double *)((struct descriptor_a *)segTimesXds[currSegment].pointer)->pointer;
            for(i = 0; i < currNTimes - leftRows; i++)
                times[timesOfs++] = doubleTimes[i];
        }
    }


    dataD.arsize -= lastSegmentOffset;
    dataD.m[dataD.dimct - 1] -= leftRows;
    MdsCopyDxXd((struct descriptor *)&dataD, retDataXd);
    timesD.arsize = timesOfs * sizeof(double);
    MdsCopyDxXd((struct descriptor *)&timesD, retTimesXd);
    free((char *)times);
    free((char *)data);
    for(currSegment = 0; currSegment < actSegments; currSegment++)
    {
        MdsFree1Dx(&segDataXds[currSegment], 0);
        MdsFree1Dx(&segTimesXds[currSegment], 0);
    }
    free((char *)segDataXds);
    free((char *)segTimesXds);
    unlock();
    return 0;
}

static int getTimedDataNoDescr(int expIdx, char *cpoPath, char *path, double start, double end, void **retData, int *retDims,  int *retDimsCt, char *retType, char *retClass, int *retLength, int64_t *retArsize,
    struct descriptor_xd *retTimesXd, int all)
{
    struct descriptor_a *longTimesD;
    EMPTYXD(emptyXd);
    EMPTYXD(startXd);
    EMPTYXD(endXd);
    int status, i;
    int lastSegmentOffset, leftItems, leftRows, lastDim;
    double *times, *doubleTimes;
    uint64_t *longTimes;
    double currStart, currEnd;
    int nid, numSegments, currSegment, actSegments;
    struct descriptor_xd *segTimesXds, *segDataXds;
    int64_t dataLen, timesLen, dataOfs, timesOfs, currNTimes;
    char *data;
    ARRAY_COEFF(char, 16) *dataDPtr;
    DESCRIPTOR_A(timesD, sizeof(double), DTYPE_DOUBLE, 0, 0);

    lock("getTimedDataNoDescr");
    checkExpIndex(expIdx);

        nid = getNid(cpoPath, path);
    status = TreeGetNumSegments(nid, &numSegments);
    if(!(status & 1))
    {
	sprintf(errmsg, "Error getting number of segments for IDS: %s, field: %s - %s", cpoPath, path, MdsGetMsg(status));
	unlock();
	return -1;
    }
    if(numSegments == 0)
    {
	unlock();
        status = getNonSegmentedTimedDataNoDescr(expIdx, cpoPath, path, retData, retDims,  retDimsCt, retType, retClass, retLength,  retArsize, retTimesXd);
	return status;
    }

    segTimesXds = (struct descriptor_xd *)malloc(sizeof(struct descriptor_xd)*numSegments);
    segDataXds = (struct descriptor_xd *)malloc(sizeof(struct descriptor_xd)*numSegments);
    actSegments = 0;
    lastDim = 0;
    for(currSegment = 0; currSegment < numSegments; currSegment++)
    {
        status = TreeGetSegmentLimits(nid, currSegment, &startXd, &endXd);
        if(!(status & 1))
        {
            sprintf(errmsg, "Error getting segment limits for IDS: %s, field: %s", cpoPath, path);
	    unlock();
            return -1;
        }
        currStart = getDoubleTime(startXd.pointer);
        currEnd = getDoubleTime(endXd.pointer);
        MdsFree1Dx(&startXd, 0);
        MdsFree1Dx(&endXd, 0);
        if(!all && (currStart >= end))
            break;
        if(all || (currEnd >= start) || currSegment == numSegments - 1)
        {
            segTimesXds[actSegments]= emptyXd;
            segDataXds[actSegments]= emptyXd;
            status = TreeGetSegment(nid, currSegment, &segDataXds[actSegments], &segTimesXds[actSegments]);
            if(!(status & 1))
            {
                sprintf(errmsg, "Error getting segment for IDS: %s, field: %s", cpoPath, path);
	        unlock();
                return -1;
            }

            dataDPtr = (void *)segDataXds[actSegments].pointer;
            if(dataDPtr->dimct == 1)
                lastDim += dataDPtr->arsize/dataDPtr->length;
            else
                lastDim += dataDPtr->m[dataDPtr->dimct - 1];


            //Times is now an expression
            status = TdiData( &segTimesXds[actSegments],  &segTimesXds[actSegments] MDS_END_ARG);
            if(!(status & 1))
            {
                sprintf(errmsg, "Error getting segment time for IDS: %s, field: %s", cpoPath, path);
	        unlock();
                return -1;
            }


            actSegments++;
        }
    }
    //Merge data and times, converted to double
    dataLen = timesLen = 0;
    dataDPtr = (void *)segDataXds[0].pointer;

    *retType = dataDPtr->dtype;
    *retClass = dataDPtr->class;
    *retLength = segDataXds[0].pointer->length;
    *retDimsCt = dataDPtr->dimct;
    for(i = 0; i < dataDPtr->dimct; i++)
        retDims[i] = dataDPtr->m[i];
    retDims[dataDPtr->dimct-1] = lastDim;


/*
    dataD.class = CLASS_A;
    dataD.dtype = dataDPtr->dtype;
    dataD.length = segDataXds[0].pointer->length;
    dataD.dimct = dataDPtr->dimct;
    for(i = 0; i < dataD.dimct; i++)
        dataD.m[i] = dataDPtr->m[i];
// The last dimension is multiplied fir the number of segments QUESTO CONTO LO DEVO FARE COME SOMMA DELL'ULTIMA DIMENSIONE!!
    dataD.m[dataD.dimct-1] = lastDim;

*/



    for(currSegment = 0; currSegment < actSegments; currSegment++)
    {
        dataLen += ((struct descriptor_a *)segDataXds[currSegment].pointer)->arsize;
        timesLen += ((struct descriptor_a *)segTimesXds[currSegment].pointer)->arsize;
    }
    times = (double *)malloc(sizeof(double) * timesLen/sizeof(uint64_t));

    *retData = data = malloc(dataLen);
    *retArsize = dataLen;

/*    dataD.arsize = dataLen;
    dataD.pointer = data;
*/

    timesD.pointer = (char *)times;
    dataOfs = 0;
    timesOfs = 0;
    lastSegmentOffset = 0;
    leftRows = 0;
    for(currSegment = 0; currSegment < actSegments; currSegment++)
    {
        //Check whether last segment is not full
        if(currSegment == numSegments - 1)
        {
            getLeftItems(nid, &leftItems, &leftRows);
            lastSegmentOffset = leftItems * ((struct descriptor_a *)segDataXds[currSegment].pointer)->length;
        }

        memcpy(&data[dataOfs], ((struct descriptor_a *)segDataXds[currSegment].pointer)->pointer,
            ((struct descriptor_a *)segDataXds[currSegment].pointer)->arsize);
        dataOfs += ((struct descriptor_a *)segDataXds[currSegment].pointer)->arsize - lastSegmentOffset;
        if(segTimesXds[currSegment].pointer->dtype == DTYPE_Q || segTimesXds[currSegment].pointer->dtype == DTYPE_QU)
        {
            currNTimes = ((struct descriptor_a *)segTimesXds[currSegment].pointer)->arsize/sizeof(uint64_t);
            longTimes = (uint64_t *)((struct descriptor_a *)segTimesXds[currSegment].pointer)->pointer;
            for(i = 0; i < currNTimes - leftRows; i++)
                MdsTimeToDouble(longTimes[i], &times[timesOfs++]);
        }
        else
        {
            currNTimes = ((struct descriptor_a *)segTimesXds[currSegment].pointer)->arsize/sizeof(double);
            doubleTimes = (double *)((struct descriptor_a *)segTimesXds[currSegment].pointer)->pointer;
            for(i = 0; i < currNTimes - leftRows; i++)
                times[timesOfs++] = doubleTimes[i];
        }
    }

    *retArsize -= lastSegmentOffset;
    retDims[dataDPtr->dimct - 1] -= leftRows;

/*    dataD.arsize -= lastSegmentOffset;
    dataD.m[dataD.dimct - 1] -= leftRows;
    MdsCopyDxXd((struct descriptor *)&dataD, retDataXd);
*/

    timesD.arsize = timesOfs * sizeof(double);
    MdsCopyDxXd((struct descriptor *)&timesD, retTimesXd);
    free((char *)times);
//    free((char *)data);
    for(currSegment = 0; currSegment < actSegments; currSegment++)
    {
        MdsFree1Dx(&segDataXds[currSegment], 0);
        MdsFree1Dx(&segTimesXds[currSegment], 0);
    }
    free((char *)segDataXds);
    free((char *)segTimesXds);
    unlock();
    return 0;
}



static int getSlicedDataLocal(int expIdx, char *cpoPath, char *path, double time, struct descriptor_xd *retDataXd,
    struct descriptor_xd *retTimesXd, int expandObject)
{
    struct descriptor_a *longTimesD, *doubleTimesD;
    EMPTYXD(timesXd);
    EMPTYXD(startXd);
    EMPTYXD(endXd);
    EMPTYXD(dataXd);
    int status, nTimes, i, leftItems, leftRows, oldLen;
    DESCRIPTOR_A(timesD, sizeof(double), DTYPE_DOUBLE, 0, 0);
    struct descriptor_a *dataD;
    double *times, start, end, currStart, currEnd;
    uint64_t *longTimes;

    int nid, numSegments, currSegment, isObject;

    lock("getSlicedDataLocal");
    checkExpIndex(expIdx);


        nid = getNid(cpoPath, path);
    status = TreeGetNumSegments(nid, &numSegments);
    if(!(status & 1))
    {
	sprintf(errmsg, "Error getting number of segments for IDS: %s, field: %s - %s", cpoPath, path, MdsGetMsg(status));
        unlock();
	return -1;
    }
    if(numSegments == 0)
    {
        unlock();
        return getNonSegmentedTimedData(expIdx, cpoPath, path, retDataXd, retTimesXd);
    }

//Check the dtype of stert time for the first segment: if DTYPE_DOUBLE it is a segment containing normal slices
//if DTYPE_L it is a segment containing an object slice
    status = TreeGetSegmentLimits(nid, 0, &startXd, &endXd);
    if(!(status & 1))
    {
	sprintf(errmsg, "Cannot get segment limits for IDS: %s, field: %s - %s", cpoPath, path, MdsGetMsg(status));
	unlock();
	return -1;
    }
    isObject = (startXd.pointer->dtype == DTYPE_L);
    if(isObject)
    {
	printf("FATAL ERROR: Unexpected Object found in getSlicedData()!!!!\n");
	exit(0);
    }

    MdsFree1Dx(&startXd, 0);
    MdsFree1Dx(&endXd, 0);
    if(isObject)
    {
	struct descriptor_xd *xdPtr;
	status = getObjectSliceLocal(expIdx, cpoPath, path,  time, (void **)&xdPtr, expandObject);
	if(!status)
	{
	    memcpy(retDataXd, xdPtr, sizeof(struct descriptor_xd));
	    free((char *)xdPtr);
	}
	unlock();
	return status;
    }
    for(currSegment = 0; currSegment < numSegments; currSegment++)
    {
        status = TreeGetSegmentLimits(nid, currSegment, &startXd, &endXd);
        if(!(status & 1))
        {
            sprintf(errmsg, "Error getting segment limits for IDS: %s, field: %s", cpoPath, path);
            unlock();
            return -1;
        }
        currStart = getDoubleTime(startXd.pointer);
        currEnd = getDoubleTime(endXd.pointer);
        MdsFree1Dx(&startXd, 0);
        MdsFree1Dx(&endXd, 0);
        if(time < currStart ||  (currStart <= time && currEnd >= time) )
            break;
    }
    if(currSegment >= numSegments)
        currSegment = numSegments-1;
    status = TreeGetSegment(nid, currSegment, &dataXd, &timesXd);
    if(!(status & 1))
    {
        sprintf(errmsg, "Error getting segment for IDS: %s, field: %s", cpoPath, path);
        return -1;
    }
    //Times is now an expression
    status = TdiData(&timesXd, &timesXd MDS_END_ARG);
    if(!(status & 1))
    {
        sprintf(errmsg, "Error getting segment time for IDS: %s, field: %s", cpoPath, path);
        unlock();
        return -1;
    }
    leftItems = 0;
    leftRows = 0;
    //Check whether last segment is not full
    if(currSegment == numSegments - 1)
    {
        getLeftItems(nid, &leftItems, &leftRows);
    }
    dataD = (struct descriptor_a *)dataXd.pointer;
    oldLen = dataD->arsize;
    dataD->arsize -= leftItems * dataD->length;
    MdsCopyDxXd((struct descriptor *)dataD, retDataXd);
    dataD->arsize = oldLen;
    MdsFree1Dx(&dataXd, 0);

    if(timesXd.pointer->dtype == DTYPE_Q || timesXd.pointer->dtype == DTYPE_QU)
    {
        longTimesD = (struct descriptor_a *)timesXd.pointer;
        longTimes = (uint64_t *)longTimesD->pointer;
        nTimes = longTimesD->arsize/longTimesD->length;
        nTimes -= leftRows;
        times = (double *)malloc(nTimes * sizeof(double));
        timesD.arsize = nTimes * sizeof(double);
        timesD.pointer = (char *)times;
        for(i = 0; i < nTimes; i++)
            MdsTimeToDouble(longTimes[i], &times[i]);
        MdsCopyDxXd((struct descriptor *)&timesD, retTimesXd);
        free((char *)times);
    }
    else //double times
    {
        doubleTimesD = (struct descriptor_a *)timesXd.pointer;
        oldLen = doubleTimesD->arsize;
        doubleTimesD->arsize -= leftRows * doubleTimesD->length;
        MdsCopyDxXd(timesXd.pointer, retTimesXd);
        doubleTimesD->arsize = oldLen;
    }

//printf("GET SLICED DATA LOCAL %s %s %s\n", cpoPath, path, decompileDsc(retDataXd));




    MdsFree1Dx(&timesXd, 0);
    unlock();
    return 0;

}


int getDataAndSliceIdxs(int expIdx, char *cpoPath, char *path, char *timeBasePath, struct descriptor_xd *xd, double time, int *sliceId1, int *sliceId2,
    double *sliceTim1, double *sliceTim2, int *retNTimes)
{
    EMPTYXD(timesXd);
    struct descriptor_a *timesD;
    ARRAY_COEFF(char, 16) *dataD;
    double *times;
    int nTimes;
    int status, startIdx;
    unsigned int itemSize;
    int nItems, i;

//Read Segment corresponding to time
    status = getSlicedData(expIdx, cpoPath, path, timeBasePath, time, xd, &timesXd, 1);
/*    printf("getDataAndSliceIdxs.1 status: %d, cpoPath: %s, path: %s, timeBasePath: %s\n",status, cpoPath, path, timeBasePath);*/
    if(status)
    {
	 return status;
    }
    timesD = (struct descriptor_a *)timesXd.pointer;
    if(timesD->dtype != DTYPE_DOUBLE && timesD->dtype != DTYPE_FT )
    {
            sprintf(errmsg, "Unexpected timebase data type at path %s: %s ", path, cpoPath);
            MdsFree1Dx(xd, 0);
            MdsFree1Dx(&timesXd, 0);
            return -1;
    }
    *retNTimes = nTimes = timesD->arsize / timesD->length;
//Make sure nTimes does not exceed the number of data samples
    dataD = (void *)xd->pointer;
    if(dataD->dimct == 1)
    	nItems = dataD->arsize/dataD->length;
    else
    {
    	itemSize = dataD->length;
	for(i = 0; i < dataD->dimct-1; i++)
	    itemSize *= dataD->m[i];
	nItems = dataD->arsize/itemSize;
    }
    if(nTimes > nItems)
    	*retNTimes = nTimes = nItems;




    times = (double *)timesD->pointer;
    for(startIdx = 0; startIdx < nTimes && times[startIdx] <= time; startIdx++);
    if(startIdx == nTimes)
    {
	*sliceId1 = *sliceId2 = startIdx - 1;
	*sliceTim1 = *sliceTim2 = times[startIdx - 1];
     }
    else if(startIdx > 0)
    {
        *sliceId1 = startIdx - 1;
	*sliceTim1 = times[startIdx - 1];
	*sliceId2 = startIdx;
	*sliceTim2 = times[startIdx];
    }
    else
    {
	*sliceId1 = *sliceId2 = startIdx;
	*sliceTim1 = *sliceTim2 = times[startIdx];
    }
    MdsFree1Dx(&timesXd, 0);
    return 0;
}

/* mdsputDataLocal is similar to putDataLocal but using a MDS name for cpopath and path */
static int mdsputDataLocal(int expIdx, char *cpoPath, char *path, struct descriptor *dataD)
{
	int nid, status;

	lock("mdsputDataLocal");
	checkExpIndex(expIdx);

        nid = mdsgetNid(cpoPath, path);
  	//printf("mdsputDataLocal 2. cpoPath: %s, path: %s, nid: %d\n",cpoPath, path, nid);
	if(nid == -1)
	{
		unlock();
		//printf("MDS PUT DATA LOCAL error get nid\n");
		return -1;
	}
/* Check if dataD is an empty XD and the nid is already empty: simply return in this case */
	if(dataD->class == CLASS_XD && ((struct descriptor_xd *)dataD)->l_length == 0)
	{
/* Check whether the node has member. If this is the case, this means that the node refers to an array of structures
   so it is necessary to delete both TIMED and NON_TIMED members */
   	    if(hasMembers(nid))
	    {
	    	unlock();
		char *newPath = malloc(strlen(path) + 16);
		sprintf(newPath, "%s:timed", path);
		mdsputDataLocal(expIdx, cpoPath, newPath, dataD);
		sprintf(newPath, "%s:non_timed", path);
		mdsputDataLocal(expIdx, cpoPath, newPath, dataD);
		free(newPath);
		return 0;
	    }
	    if(isEmpty(nid))
	      {
		unlock();
	    	return 0;
	      }
	}	
	status = TreePutRecord(nid, dataD, 0);
	unlock();
 	if(!(status & 1))
	{
		sprintf(errmsg, "Cannot write data at path %s, IDS path %s: %s", path, cpoPath, MdsGetMsg(status));
		//printf("MDS PUT DATA LOCAL %s %s\n", path, decompileDsc(dataD));
		//printf("MDS PUT DATA LOCAL error TreePutRecord\n");
		//printf("Cannot write data at path %s, IDS path %s: %s", path, cpoPath, MdsGetMsg(status));
		return -1;
	}
	return 0;
}


static int putDataLocal(int expIdx, char *cpoPath, char *path, struct descriptor *dataD)
{
	int nid, status;


//printf("PUT DATA LOCAL %s %s\n", path, decompileDsc(dataD));
//reportInfo("PUT DATA LOCAL\n", path);
	lock("putDataLocal");
	checkExpIndex(expIdx);

//reportInfo("PUT DATA LOCAL CHECK EXP INDEX\n", path);
        nid = getNid(cpoPath, path);
//reportInfo("PUT DATA LOCAL GOT NID\n", path);
	if(nid == -1)
	{
		unlock();
		return -1;
	}
/* Check if dataD is an empty XD and the nid is already empty: simply return in this case */
	if(dataD->class == CLASS_XD && ((struct descriptor_xd *)dataD)->l_length == 0)
	{
/* Check whether the node has member. If this is the case, this means that the node refers to an array of structures
   so it is necessary to delete both TIMED and NON_TIMED members */
   	    if(hasMembers(nid))
	    {
	    	unlock();
		char *newPath = malloc(strlen(path) + 16);
		sprintf(newPath, "%s/TIMED", path);
//reportInfo("PUT DATA LOCAL CALL PUT DATA 1\n", path);
		putData(expIdx, cpoPath, newPath, dataD);
		sprintf(newPath, "%s/NON_TIMED", path);
//reportInfo("PUT DATA LOCAL CALL PUT DATA 2\n", path);
		putData(expIdx, cpoPath, newPath, dataD);
		free(newPath);
		return 0;
	    }
	    if(isEmpty(nid))
	      {
		unlock();
	    	return 0;
	      }
	}
//reportInfo("PUT DATA LOCAL CALL PUT RECORD\n", path);

	status = TreePutRecord(nid, dataD, 0);
	unlock();
 	if(!(status & 1))
	{
		sprintf(errmsg, "Cannot write data at path %s, IDS path %s: %s", path, cpoPath, MdsGetMsg(status));
		return -1;
	}
	return 0;
}


static void getLeftItems(int nid, int *leftItems, int *leftRows)
{
    char dtype;
    char dimct;
    int dims[64];
    unsigned int rowSize;
    int idx, nextRow, status, i;
#ifdef OLD_MDS
    status = TreeGetSegmentInfo(nid, &dtype, &dimct, dims, &idx, &nextRow);
#else
    status = TreeGetSegmentInfo(nid, -1, &dtype, &dimct, dims, &nextRow);
#endif
    if(!(status & 1))
    {
        printf("Internal error in getLeftItems: %s!\n", MdsGetMsg(status));
    }
    rowSize = 1;
    for(i = 0; i < dimct - 1; i++)
        rowSize *= dims[i];
    *leftItems = (dims[dimct-1] - nextRow)*rowSize;
    *leftRows = dims[dimct-1] - nextRow;
}

// User for passing path to GetCheckedSegment
static char *oldconvertPath(char *path)
{
    static char mdsPath[2048];
    int i;
    int len = strlen(path)+ 1;

    strcpy(mdsPath, path);
    for(i = 0; i < len; i++)
    {
	if(mdsPath[i] == '/')
            mdsPath[i] = '.';
    }
    return mdsPath;
}

static int putSegmentLocal(int expIdx, char *cpoPath, char *path, char *timeBasePath, struct descriptor_a *dataD, double *times, int nTimes)
{
	int nid, status, i, leftItems, leftRows, leftBytes, prevArsize, currSegment, len;
        char *prevPtr, segmentExpr[512];
         double currStart, currEnd;
	DESCRIPTOR_A(timeD, 8, DTYPE_QU, 0, 0);
        DESCRIPTOR_A_COEFF(segDataD, 0, 0, 0, 5, 0);
        ARRAY_COEFF(char *, 5) *arrPtr;
        struct descriptor startD = {8, DTYPE_DOUBLE, CLASS_S, (char *)&currStart};
        struct descriptor endD = {8, DTYPE_DOUBLE, CLASS_S, (char *)&currEnd};
        struct descriptor exprD = {0, DTYPE_T, CLASS_S, 0};
        EMPTYXD(timeExprXd);
        EMPTYXD(startXd);
        EMPTYXD(endXd);
        uint64_t currEndL;

	lock("putSegmentLocal");
	checkExpIndex(expIdx);
//       printf("ual_low_level_mdsplus : putSegmentLocal user paths : %s , %s , %s\n", cpoPath, path,timeBasePath);

        nid = getNid(cpoPath, path);
//        printf("NID = %d\n", nid);
	if(nid == -1)
	{
                printf("Node not found, nid = -1");
		unlock();
		return -1;
        }
        status = TreeGetNumSegments(nid, &currSegment);
        if(!(status & 1))
	{
		sprintf(errmsg, "Cannot get num segments segment at path %s, IDS path %s: %s", path, cpoPath, MdsGetMsg(status));
		unlock();
		return -1;
	}
//    printf("ual_low_level_mdsplus : putSegmentLocal converted paths before CheckedSegment (cpopath, timebasepath) : %s , %s \n", convertPath(cpoPath),convertAnotherPath(timeBasePath));
        sprintf(segmentExpr, "data(GetCheckedSegment(build_path(\"%s:%s\"),%d))", convertPath(cpoPath),mdsconvertPath(timeBasePath), currSegment);
//        sprintf(segmentExpr, "data(GetCheckedSegment(build_path(\"pf0:coils0.1.current0.timebase0\"),0))");
// segmentExpr = data(GetCheckedSegment(build_path("pf0:coils0/1/current0/timebase0"),0))
       // printf("segmentExpr = %s\n",segmentExpr);
	exprD.length = strlen(segmentExpr);
        exprD.pointer = segmentExpr;
        status = TdiCompile(&exprD, &timeExprXd MDS_END_ARG);
        if(!(status & 1))
	{
		sprintf(errmsg, "Internal error at path %s, IDS path %s: %s: cannot compile %s", path, cpoPath, MdsGetMsg(status), segmentExpr);
		unlock();
		return -1;
	}
        if(currSegment == 0)
            leftItems = leftRows = 0;
        else
            getLeftItems(nid, &leftItems, &leftRows);
        if(leftItems > 0)
        {
            leftBytes = leftItems * dataD->length;
            prevArsize = dataD->arsize;
            if(leftBytes < dataD->arsize)
            {
                dataD->arsize = leftBytes;
                currEnd = times[leftRows - 1];
            }
            else
                currEnd = times[nTimes - 1];
            MdsFloatToTime(currEnd, &currEndL);
            status = TreePutSegment(nid, -1, dataD);
            if(!(status & 1))
            {
		sprintf(errmsg, "Cannot write data segment at path %s, IDS path %s: %s", path, cpoPath, MdsGetMsg(status));
		unlock();
		return -1;
            }
            prevPtr = dataD->pointer;
            //Need also to update end time for that segment.
            status = TreeGetSegmentLimits(nid, currSegment - 1, &startXd, &endXd);
            if(!(status & 1))
            {
		sprintf(errmsg, "Cannot write data segment at path %s, IDS path %s: %s", path, cpoPath, MdsGetMsg(status));
		unlock();
		return -1;
            }
            if(endXd.pointer->dtype == DTYPE_Q || endXd.pointer->dtype == DTYPE_QU)
                *(int64_t *)endXd.pointer->pointer = currEndL;
            else
                *(double *)endXd.pointer->pointer = currEnd;
            status = TreeUpdateSegment(nid, 0, endXd.pointer, 0, currSegment - 1);
            if(!(status & 1))
            {
		sprintf(errmsg, "Cannot write data segment at path %s, IDS path %s: %s", path, cpoPath, MdsGetMsg(status));
		unlock();
		return -1;
            }
            MdsFree1Dx(&startXd, 0);
            MdsFree1Dx(&endXd, 0);


            if(prevArsize > leftBytes) //Write remaining segment
            {
                currStart = times[leftRows];
                currEnd = times[nTimes - 1];
                dataD->pointer += leftBytes;
                dataD->arsize = prevArsize - leftBytes;
                status = TreeBeginSegment(nid, &startD, &endD, timeExprXd.pointer, dataD, -1);
                if(!(status & 1))
                {
                    dataD->arsize = prevArsize;
                    dataD->pointer = prevPtr;
                    sprintf(errmsg, "Cannot write data segment at path %s, IDS path %s: %s", path, cpoPath, MdsGetMsg(status));
		    unlock();
                    return -1;
                }
                status = TreePutSegment(nid, -1, dataD);
                if(!(status & 1))
                {
                    sprintf(errmsg, "Cannot write data segment at path %s, IDS path %s: %s", path, cpoPath, MdsGetMsg(status));
		    unlock();
                    return -1;
                }
                //free(segDataD.pointer);
            }
            dataD->arsize = prevArsize;
            dataD->pointer = prevPtr;
        }
        else //No left bytes in previous segmment
        {
            currStart = times[0];
            currEnd = times[nTimes - 1];
//Ensure that the segment holds DEFAULT_SEGMENT_ROW items
/*            segDataD.dtype = dataD->dtype;
            segDataD.length = dataD->length;
            segDataD.dimct = dataD->dimct;
            len = dataD->length;
            if(dataD->dimct == 1)
                segDataD.m[0] = dataD->arsize / dataD->length;
            else
            {
                arrPtr = dataD;
                for(i = 0; i < dataD->dimct; i++)
                {
                    segDataD.m[i] = arrPtr->m[i];
                    if(i < dataD->dimct - 1)
                        len *= segDataD.m[i];
                }
            }
            segDataD.m[segDataD.dimct - 1] = DEFAULT_SEGMENT_ROWS;
            len *= DEFAULT_SEGMENT_ROWS;
            segDataD.arsize = len;
            segDataD.pointer = malloc(len);
            memset(segDataD.pointer, 0, len);
*/
            status = TreeBeginSegment(nid, &startD, &endD, timeExprXd.pointer, dataD, -1);
            if(!(status & 1))
            {
                sprintf(errmsg, "Cannot write data segment at path %s, IDS path %s: %s", path, cpoPath, MdsGetMsg(status));
		unlock();
                return -1;
            }
            status = TreePutSegment(nid, -1, dataD);
            if(!(status & 1))
            {
                sprintf(errmsg, "Cannot write data segment at path %s, IDS path %s: %s", path, cpoPath, MdsGetMsg(status));
		unlock();
                return -1;
            }
        }

        MdsFree1Dx(&timeExprXd, 0);
	unlock();
	return 0;
}


static int mdsbeginIdsPutSliceLocal(int expIdx, char *path)
{
    int status;
    status = checkExpIndex(expIdx);
    return status;
}

static int mdsendIdsPutSliceLocal(int expIdx, char *path)
{
    return 0;
}

static int mdsbeginIdsReplaceLastSliceLocal(int expIdx, char *path)
{
    int status;
    status = checkExpIndex(expIdx);
    return status;
}

static int mdsendIdsReplaceLastSliceLocal(int expIdx, char *path)
{
    return 0;
}



static int putSliceLocal(int expIdx, char *cpoPath, char *path, char *timeBasePath, struct descriptor *dataD, double time)
{
	int nid, status, i, currSegment, currSize;
        uint64_t timestamp;
        struct descriptor timeD;
        int dataLen, leftItems, leftRows, leftBytes;
        double currEnd;
        struct descriptor endD = {sizeof(double), DTYPE_DOUBLE, CLASS_S, (char *)&currEnd};
        struct descriptor exprD = {0, DTYPE_T, CLASS_S, 0};
        char *dataPtr, segmentExpr[256];
        EMPTYXD(timeExprXd);
        DESCRIPTOR_A_COEFF(segDataD, 0, 0, 0, 5, 0);
        ARRAY_COEFF(char *, 5) *segDataPtr;

        static int counter;

/* printf("PUT SLICE LOCAL %s %s\n", path, decompileDsc(dataD));*/

        if(dataD->class == CLASS_A)
            dataLen = ((struct descriptor_a *)dataD)->arsize;
        else
            dataLen = dataD->length;

	lock("putSliceLocal");
	checkExpIndex(expIdx);

        nid = getNid(cpoPath, path);
	if(nid == -1)
	{
		unlock();
		return -1;
	}

        status = TreeGetNumSegments(nid, &currSegment);
        if(!(status & 1))
	{
		sprintf(errmsg, "Cannot get num segments at path %s, IDS path %s: %s", path, cpoPath, MdsGetMsg(status));
		unlock();
		return -1;
	}
//        sprintf(segmentExpr, "data(GetCheckedSegment(%s:time,%d))", convertPath(cpoPath), currSegment);
        sprintf(segmentExpr, "data(GetCheckedSegment(build_path(\"%s:%s\"),%d))", convertPath(cpoPath),mdsconvertPath(timeBasePath), currSegment);
        exprD.length = strlen(segmentExpr);
        exprD.pointer = segmentExpr;
        status = TdiCompile(&exprD, &timeExprXd MDS_END_ARG);
        if(!(status & 1))
	{
		sprintf(errmsg, "Internal error at path %s, IDS path %s: %s: cannot compile %s", path, cpoPath, MdsGetMsg(status), segmentExpr);
		unlock();
		return -1;
	}
        currEnd = time;
        if(currSegment == 0)
            leftItems = leftRows = 0;
        else
            getLeftItems(nid, &leftItems, &leftRows);
        if(leftItems > 0)
        {
            leftBytes = leftItems * dataD->length;
            if(leftBytes < dataLen)
            {
		sprintf(errmsg, "Internal error in putSlice at path %s, IDS path %s: %s: wrong segment size", path, cpoPath, MdsGetMsg(status));
		unlock();
		return -1;
            }
            status = TreePutSegment(nid, -1, (struct descriptor_a *)dataD);
           if(!(status & 1))
            {
		sprintf(errmsg, "Cannot write segment at path %s, IDS path %s: %s", path, cpoPath, MdsGetMsg(status));
		unlock();
		return -1;
            }
            if(leftBytes == dataLen) //if last data in segment, need to update end time
            {
                status = TreeUpdateSegment(nid, 0, &endD, 0, currSegment -1);
                if(!(status & 1))
                {
                    sprintf(errmsg, "Cannot update segment at path %s, IDS path %s: %s", path, cpoPath, MdsGetMsg(status));
		    unlock();
                    return -1;
                }
            }
        }
        else //no space left in segment, need to create a new segment
        {
            dataPtr = malloc(dataLen * DEFAULT_SEGMENT_ROWS);
            memset(dataPtr, 0, dataLen * DEFAULT_SEGMENT_ROWS);
            if(dataD->class == CLASS_S)
            {
                segDataD.dtype = dataD->dtype;
                segDataD.dimct = 1;
                segDataD.length = dataD->length;
                segDataD.m[0] = DEFAULT_SEGMENT_ROWS;
                segDataD.pointer = dataPtr;
                segDataD.arsize = dataD->length * DEFAULT_SEGMENT_ROWS;
            }
            else //Array
            {
                segDataPtr = (void *)dataD;
                segDataD.dtype = segDataPtr->dtype;
                segDataD.length = segDataPtr->length;
                segDataD.dimct = segDataPtr->dimct+1;
                currSize = segDataPtr->length;
                if(segDataPtr->aflags.coeff)
                {
                    for(i = 0; i < segDataPtr->dimct; i++)
                    {
                        segDataD.m[i] = segDataPtr->m[i];
                        currSize *= segDataPtr->m[i];
                    }
                }
                else
                {
                    segDataD.m[0] = segDataPtr->arsize/segDataPtr->length;
                    currSize *= segDataD.m[0];
                }
                segDataD.m[ segDataPtr->dimct] = DEFAULT_SEGMENT_ROWS;
                segDataD.pointer = dataPtr;
                segDataD.arsize = currSize * DEFAULT_SEGMENT_ROWS;
            }



            status = TreeBeginSegment(nid,
	    	&endD, &endD, (struct descriptor *)timeExprXd.pointer, (struct descriptor_a *)&segDataD, -1);
            if(!(status & 1))
            {
                sprintf(errmsg, "Cannot write data segment at path %s, IDS path %s: %s", path, cpoPath, MdsGetMsg(status));
		unlock();
                return -1;
            }
            status = TreePutSegment(nid, -1, (struct descriptor_a *)dataD);
            if(!(status & 1))
            {
                sprintf(errmsg, "Cannot append data segment at path %s, IDS path %s: %s", path, cpoPath, MdsGetMsg(status));
		unlock();
                return -1;
            }
            free(dataPtr);
        }
        MdsFree1Dx(&timeExprXd, 0);
 	unlock();

        return 0;
}

static int mdsputSliceLocal(int expIdx, char *cpoPath, char *path, char *timeBasePath, struct descriptor *dataD, double time)
{
	int nid, status, i, currSegment, currSize;
        uint64_t timestamp;
        struct descriptor timeD;
        int dataLen, leftItems, leftRows, leftBytes;
        double currEnd;
        struct descriptor endD = {sizeof(double), DTYPE_DOUBLE, CLASS_S, (char *)&currEnd};
        struct descriptor exprD = {0, DTYPE_T, CLASS_S, 0};
        char *dataPtr, segmentExpr[256];
        EMPTYXD(timeExprXd);
        DESCRIPTOR_A_COEFF(segDataD, 0, 0, 0, 5, 0);
        ARRAY_COEFF(char *, 5) *segDataPtr;

        static int counter;

/* printf("PUT SLICE LOCAL %s %s\n", path, decompileDsc(dataD));*/

        if(dataD->class == CLASS_A)
            dataLen = ((struct descriptor_a *)dataD)->arsize;
        else
            dataLen = dataD->length;

	lock("putSliceLocal");
	checkExpIndex(expIdx);

        nid = mdsgetNid(cpoPath, path);
	if(nid == -1)
	{
		unlock();
		return -1;
	}

        status = TreeGetNumSegments(nid, &currSegment);
        if(!(status & 1))
	{
		sprintf(errmsg, "Cannot get num segments at path %s, IDS path %s: %s", path, cpoPath, MdsGetMsg(status));
		unlock();
		return -1;
	}
//        sprintf(segmentExpr, "data(GetCheckedSegment(%s:time,%d))", convertPath(cpoPath), currSegment);
        sprintf(segmentExpr, "data(GetCheckedSegment(build_path(\"%s:%s\"),%d))", convertPath(cpoPath),mdsconvertPath(timeBasePath), currSegment);
        exprD.length = strlen(segmentExpr);
        exprD.pointer = segmentExpr;
        status = TdiCompile(&exprD, &timeExprXd MDS_END_ARG);
        if(!(status & 1))
	{
		sprintf(errmsg, "Internal error at path %s, IDS path %s: %s: cannot compile %s", path, cpoPath, MdsGetMsg(status), segmentExpr);
		unlock();
		return -1;
	}
        currEnd = time;
        if(currSegment == 0)
            leftItems = leftRows = 0;
        else
            getLeftItems(nid, &leftItems, &leftRows);
        if(leftItems > 0)
        {
            leftBytes = leftItems * dataD->length;
            if(leftBytes < dataLen)
            {
		sprintf(errmsg, "Internal error in putSlice at path %s, IDS path %s: %s: wrong segment size", path, cpoPath, MdsGetMsg(status));
		unlock();
		return -1;
            }
            status = TreePutSegment(nid, -1, (struct descriptor_a *)dataD);
           if(!(status & 1))
            {
		sprintf(errmsg, "Cannot write segment at path %s, IDS path %s: %s", path, cpoPath, MdsGetMsg(status));
		unlock();
		return -1;
            }
            if(leftBytes == dataLen) //if last data in segment, need to update end time
            {
                status = TreeUpdateSegment(nid, 0, &endD, 0, currSegment -1);
                if(!(status & 1))
                {
                    sprintf(errmsg, "Cannot update segment at path %s, IDS path %s: %s", path, cpoPath, MdsGetMsg(status));
		    unlock();
                    return -1;
                }
            }
        }
        else //no space left in segment, need to create a new segment
        {
            dataPtr = malloc(dataLen * DEFAULT_SEGMENT_ROWS);
            memset(dataPtr, 0, dataLen * DEFAULT_SEGMENT_ROWS);
            if(dataD->class == CLASS_S)
            {
                segDataD.dtype = dataD->dtype;
                segDataD.dimct = 1;
                segDataD.length = dataD->length;
                segDataD.m[0] = DEFAULT_SEGMENT_ROWS;
                segDataD.pointer = dataPtr;
                segDataD.arsize = dataD->length * DEFAULT_SEGMENT_ROWS;
            }
            else //Array
            {
                segDataPtr = (void *)dataD;
                segDataD.dtype = segDataPtr->dtype;
                segDataD.length = segDataPtr->length;
                segDataD.dimct = segDataPtr->dimct+1;
                currSize = segDataPtr->length;
                if(segDataPtr->aflags.coeff)
                {
                    for(i = 0; i < segDataPtr->dimct; i++)
                    {
                        segDataD.m[i] = segDataPtr->m[i];
                        currSize *= segDataPtr->m[i];
                    }
                }
                else
                {
                    segDataD.m[0] = segDataPtr->arsize/segDataPtr->length;
                    currSize *= segDataD.m[0];
                }
                segDataD.m[ segDataPtr->dimct] = DEFAULT_SEGMENT_ROWS;
                segDataD.pointer = dataPtr;
                segDataD.arsize = currSize * DEFAULT_SEGMENT_ROWS;
            }



            status = TreeBeginSegment(nid,
	    	&endD, &endD, (struct descriptor *)timeExprXd.pointer, (struct descriptor_a *)&segDataD, -1);
            if(!(status & 1))
            {
                sprintf(errmsg, "Cannot write data segment at path %s, IDS path %s: %s", path, cpoPath, MdsGetMsg(status));
		unlock();
                return -1;
            }
            status = TreePutSegment(nid, -1, (struct descriptor_a *)dataD);
            if(!(status & 1))
            {
                sprintf(errmsg, "Cannot append data segment at path %s, IDS path %s: %s", path, cpoPath, MdsGetMsg(status));
		unlock();
                return -1;
            }
            free(dataPtr);
        }
        MdsFree1Dx(&timeExprXd, 0);
 	unlock();

        return 0;
}


int mdsDeleteData(int expIdx, char *cpoPath, char *path)
{
    int status;
    EMPTYXD(emptyXd);
    status = putData(expIdx, cpoPath, path, (struct descriptor *)&emptyXd);
    return status;
}

static int replaceLastSliceLocal(int expIdx, char *cpoPath, char *path, struct descriptor *dataD)
{
	int nid, status, i, currSegment, currSize;
        int dataLen, nextRow;
        char dtype, nDims;
        int idx, dims[64];
        double currEnd;
        char *dataPtr;

        DESCRIPTOR_A_COEFF(segDataD, 0, 0, 0, 5, 0);
        ARRAY_COEFF(char *, 5) *segDataPtr;


        if(dataD->class == CLASS_A)
            dataLen = ((struct descriptor_a *)dataD)->arsize;
        else
            dataLen = dataD->length;

 	lock("replaceLastSliceLocal");
	checkExpIndex(expIdx);

        nid = getNid(cpoPath, path);
	if(nid == -1)
	{
		unlock();
		return -1;
	}

        status = TreeGetNumSegments(nid, &currSegment);
        if(!(status & 1))
	{
		sprintf(errmsg, "Cannot get num segments at path %s, IDS path %s: %s", path, cpoPath, MdsGetMsg(status));
		unlock();
		return -1;
	}
        if(currSegment == 0)
	{
		sprintf(errmsg, "No slice to be replaced %s, IDS path %s: %s", path, cpoPath, MdsGetMsg(status));
		unlock();
		return -1;
	}
#ifdef OLD_MDS
        status = TreeGetSegmentInfo(nid, &dtype, &nDims, dims, &idx, &nextRow);
#else
        status = TreeGetSegmentInfo(nid, -1, &dtype, &nDims, dims, &nextRow);
#endif
        if(!(status & 1))
        {
            printf("Internal error TreeGetSegmentInfo!\n");
	    unlock();
	    return -1;
        }
        status = TreePutSegment(nid, nextRow - 1, (struct descriptor_a *)dataD);
        if(!(status & 1))
        {
            sprintf(errmsg, "Cannot append data segment at path %s, IDS path %s: %s", path, cpoPath, MdsGetMsg(status));
	    unlock();
            return -1;
        }
	unlock();
        return 0;
}

static int mdsbeginIdsPutLocal(int expIdx, char *path)
{
    int status;
    status = checkExpIndex(expIdx);
    return status;
}

static int mdsendIdsPutLocal(int expIdx, char *path)
{
    return 0;
}

static int mdsbeginIdsPutTimedLocal(int expIdx, char *path, int samples, double *inTimes)
{
    int status;
    status = checkExpIndex(expIdx);
    return status;
}

static int mdsendIdsPutTimedLocal(int expIdx, char *path)
{
    return 0;
}

static int mdsbeginIdsPutNonTimedLocal(int expIdx, char *path)
{
    int status;
    status = checkExpIndex(expIdx);
    return status;
}

static int mdsendIdsPutNonTimedLocal(int expIdx, char *path)
{
    return 0;
}

int mdsPutEmpty(int expIdx, char *cpoPath, char *path)
{
	EMPTYXD(emptyXd);
	return putData(expIdx, cpoPath, path, (struct descriptor *)&emptyXd);
}


int mdsPutString(int expIdx, char *cpoPath, char *path, char *inData)
{
    int status, i, data;
    EMPTYXD(emptyXd);
    DESCRIPTOR_A(dataD, 1, DTYPE_BU, 0, 0);

	if(!inData || !*inData)
	{
	    return putData(expIdx, cpoPath, path, (struct descriptor *)&emptyXd);//Missing String
	}
        dataD.arsize = strlen(inData);
	dataD.pointer = inData;
	status =  putData(expIdx, cpoPath, path, (struct descriptor *)&dataD);
        return status;
}


 int mdsPutFloat(int expIdx, char *cpoPath, char *path, float data)
{
	int status;

	struct descriptor dataD = {0, DTYPE_FLOAT, CLASS_S, 0};
	dataD.length = sizeof(float);
	dataD.pointer = (char *)&data;
	status = putData(expIdx, cpoPath, path, (struct descriptor *)&dataD);
	return status;
}

 int mdsPutInt(int expIdx, char *cpoPath, char *path, int data)
{
	struct descriptor dataD = {0, DTYPE_L, CLASS_S, 0};
	int status;
	dataD.length = sizeof(int);
	dataD.pointer = (char *)&data;
	status =  putData(expIdx, cpoPath, path, (struct descriptor *)&dataD);
	return status;
}

 int mdsPutDouble(int expIdx, char *cpoPath, char *path, double data)
{
	struct descriptor dataD = {0, DTYPE_DOUBLE, CLASS_S, 0};
	int status;
	dataD.length = sizeof(double);
	dataD.pointer = (char *)&data;
	status = putData(expIdx, cpoPath, path, (struct descriptor *)&dataD);
	return status;
}

 int mdsPutVect1DInt(int expIdx, char *cpoPath, char *path, char *timeBasePath, int *data, int dim, int isTimed)
{
	EMPTYXD(emptyXd);
	DESCRIPTOR_A(dataD, sizeof(int), DTYPE_L, 0, 0);
	int status;
        if(isTimed)
            return putTimedVect1DInt(expIdx, cpoPath, path, timeBasePath, data, putTimes, nPutTimes);
	dataD.arsize = sizeof(int) * dim;
	dataD.pointer = (char *)data;
	if(dim > 0)
		status = putData(expIdx, cpoPath, path, (struct descriptor *)&dataD);
	else
		status = putData(expIdx, cpoPath, path, (struct descriptor *)&emptyXd);
	return status;
}

static int putTimedVect1DInt(int expIdx, char *cpoPath, char *path, char * timeBasePath, int *data, double *times, int nTimes)
{
	EMPTYXD(emptyXd);
	DESCRIPTOR_A(dataD, sizeof(int), DTYPE_L, 0, 0);
	int status;
	dataD.arsize = sizeof(int) * nTimes;
	dataD.pointer = (char *)data;
	if(nTimes > 0)
 	    status =  putSegment(expIdx, cpoPath, path, timeBasePath, (struct descriptor_a *)&dataD, times, nTimes);
	else
	    status = putData(expIdx, cpoPath, path, (struct descriptor *)&emptyXd);
	return status;
}


 int mdsPutVect1DString(int expIdx, char *cpoPath, char *path, char *timeBasePath, char **data, int dim, int isTimed)
{
	EMPTYXD(emptyXd);
        int i, status;
	DESCRIPTOR_A(dataD, 0, DTYPE_T, 0, 0);
        char *strPtr;
        int maxLen = 0;
	//printf("mdsPutVect1DString. cpopath: %s, path: %s, timebase: %s, data: %s \n",cpoPath, path, timeBasePath, *data);
	//printf("mdsPutVect1DString. isTimed: %d \n",isTimed);
	if(!data || !dim)
	{
	   return 0;
	}

        if(isTimed)
	{
/*	    if(dim != nPutTimes)
	    {
	        sprintf(errmsg, "Internal error: data size and time vector length differ for timed data at IDS %s field %s.  ", cpoPath, path);
		return -1;
	    }
*/	
            return putTimedVect1DString(expIdx, cpoPath, path, timeBasePath,  data, putTimes, nPutTimes);
	}

	//printf("mdsPutVect1DString. is not Timed: dim %d\n",dim);
        for(i = 0; i < dim; i++)
	{ //printf("mdsPutVect1DString. strlen %d, max: %d\n",strlen(data[i]), maxLen);
            if(strlen(data[i]) > maxLen)
                maxLen = strlen(data[i]); }
        strPtr = malloc(dim * maxLen);
        memset(strPtr, ' ', dim * maxLen);
        for(i = 0; i < dim; i++)
            memcpy(&strPtr[i*maxLen], data[i], strlen(data[i]));
        dataD.arsize = dim*maxLen;
        dataD.pointer = strPtr;
        dataD.length = maxLen;
	//printf("mdsPutVect1DString. is not Timed: dim %d\n",dim);
	if(dim > 0)
	    status = putData(expIdx, cpoPath, path, (struct descriptor *)&dataD);
        else
            status = putData(expIdx, cpoPath, path, (struct descriptor *)&emptyXd);
        free(strPtr);
        return status;
}
static int putTimedVect1DString(int expIdx, char *cpoPath, char *path, char *timeBasePath, char **data, double *times, int nTimes)
{
	EMPTYXD(emptyXd);
	DESCRIPTOR_A_COEFF(dataD, sizeof(char), DTYPE_BU, 0, 2, 0);
        char *strPtr;
        int maxLen = 0;
        int status, i;
        for(i = 0; i < nTimes; i++)
            if(strlen(data[i]) > maxLen)
                maxLen = strlen(data[i]);
        strPtr = malloc(nTimes * maxLen);
        memset(strPtr, ' ', nTimes * maxLen);
        for(i = 0; i < nTimes; i++)
            memcpy(&strPtr[i*maxLen], data[i], strlen(data[i]));

	dataD.arsize = nTimes*maxLen;
	dataD.m[0] = maxLen;
	dataD.m[1] = nTimes;
	dataD.pointer = strPtr;

	if(nTimes > 0)
        {
		status =  putSegment(expIdx, cpoPath, path, timeBasePath,  (struct descriptor_a *)&dataD, times, nTimes);
        }
        else
                status = putData(expIdx, cpoPath, path, (struct descriptor *)&emptyXd);
        free(strPtr);
        return status;
}



 int mdsPutVect1DDouble(int expIdx, char *cpoPath, char *path, char *timeBasePath, double *data, int dim, int isTimed)
{
	int status;
	EMPTYXD(emptyXd);
	DESCRIPTOR_A(dataD, sizeof(double), DTYPE_DOUBLE, 0, 0);
        if(isTimed)
            return putTimedVect1DDouble(expIdx, cpoPath, path, timeBasePath, data, putTimes, nPutTimes);
	dataD.arsize = sizeof(double) * dim;
	dataD.pointer = (char *)data;
	if(dim > 0)
		status = putData(expIdx, cpoPath, path, (struct descriptor *)&dataD);
	else
		status = putData(expIdx, cpoPath, path, (struct descriptor *)&emptyXd);
	return status;
}

static int putTimedVect1DDouble(int expIdx, char *cpoPath, char *path, char *timeBasePath, double *data, double *times, int nTimes)
{
	EMPTYXD(emptyXd);
	DESCRIPTOR_A(dataD, sizeof(double), DTYPE_DOUBLE, 0, 0);
	int status;
	dataD.arsize = sizeof(double) * nTimes;
	dataD.pointer = (char *)data;
    //printf("ual_low_level_mdsplus : putTimedVect1DDouble : %s , %s , %s\n", cpoPath, path, timeBasePath);
	if(nTimes > 0)
        { //printf("ual_low_level_mdsplus : putTimedVect1DDouble: just before putSegment\n");
		status = putSegment(expIdx, cpoPath, path, timeBasePath, (struct descriptor_a *)&dataD, times, nTimes);
                //printf("ual_low_level_mdsplus : putTimedVect1DDouble: just after putSegment\n");
        }
	else
		status = putData(expIdx, cpoPath, path, (struct descriptor *)&emptyXd);
	return status;
}

 int mdsPutVect1DFloat(int expIdx, char *cpoPath, char *path, char *timeBasePath, float *data, int dim, int isTimed)
{
	EMPTYXD(emptyXd);
	DESCRIPTOR_A(dataD, sizeof(float), DTYPE_FLOAT, 0, 0);
	int status;
         if(isTimed)
            return putTimedVect1DFloat(expIdx, cpoPath, path, timeBasePath, data, putTimes, nPutTimes);
	dataD.arsize = sizeof(float) * dim;
	dataD.pointer = (char *)data;
	if(dim > 0)
	    status = putData(expIdx, cpoPath, path, (struct descriptor *)&dataD);
	else
	    status = putData(expIdx, cpoPath, path, (struct descriptor *)&emptyXd);
	return status;
}
static int putTimedVect1DFloat(int expIdx, char *cpoPath, char *path, char *timeBasePath, float *data, double *times, int nTimes)
{
	EMPTYXD(emptyXd);
	DESCRIPTOR_A(dataD, sizeof(float), DTYPE_FLOAT, 0, 0);
	int status;
	dataD.arsize = sizeof(float) * nTimes;
	dataD.pointer = (char *)data;
	if(nTimes > 0)
        {
		status =  putSegment(expIdx, cpoPath, path, timeBasePath, (struct descriptor_a *)&dataD, times, nTimes);
        }
	else
		status = putData(expIdx, cpoPath, path, (struct descriptor *)&emptyXd);
	return status;
}


 int mdsPutVect2DInt(int expIdx, char *cpoPath, char *path, char *timeBasePath, int *data, int dim1, int dim2, int isTimed)
{
	EMPTYXD(emptyXd);
	DESCRIPTOR_A_COEFF(dataD, sizeof(int), DTYPE_L, 0, 2, 0);
	int status;
        if(isTimed)
            return putTimedVect2DInt(expIdx, cpoPath, path, timeBasePath, data, putTimes, nPutTimes, dim1);
	dataD.arsize = dim1*dim2*sizeof(int);
	dataD.m[0] = dim1;
	dataD.m[1] = dim2;
	dataD.pointer = (char *)data;
	if(dim1 * dim2 > 0)
		status =  putData(expIdx, cpoPath, path, (struct descriptor *)&dataD);
	else
		status = putData(expIdx, cpoPath, path, (struct descriptor *)&emptyXd);
	return status;
}

static int putTimedVect2DInt(int expIdx, char *cpoPath, char *path, char *timeBasePath, int *data, double *times, int nTimes, int dim1)
{
	EMPTYXD(emptyXd);
	DESCRIPTOR_A_COEFF(dataD, sizeof(int), DTYPE_L, 0, 2, 0);
	int status;
	dataD.arsize = dim1*nTimes*sizeof(int);
	dataD.m[0] = dim1;
	dataD.m[1] = nTimes;
	dataD.pointer = (char *)data;
	if(nTimes * dim1 > 0)
		status = putSegment(expIdx, cpoPath, path, timeBasePath, (struct descriptor_a *)&dataD, times, nTimes);
	else
		status = putData(expIdx, cpoPath, path, (struct descriptor *)&emptyXd);
	return status;
}


 int mdsPutVect2DFloat(int expIdx, char *cpoPath, char *path, char *timeBasePath, float *data, int dim1, int dim2, int isTimed)
{
	EMPTYXD(emptyXd);
	DESCRIPTOR_A_COEFF(dataD, sizeof(float), DTYPE_FLOAT, 0, 2, 0);
	int status;
        if(isTimed)
            return putTimedVect2DFloat(expIdx, cpoPath, path, timeBasePath, data, putTimes, nPutTimes, dim1);
	dataD.arsize = dim1*dim2*sizeof(float);
	dataD.m[0] = dim1;
	dataD.m[1] = dim2;
	dataD.pointer = (char *)data;
	if(dim1 * dim2 > 0)
		status =  putData(expIdx, cpoPath, path, (struct descriptor *)&dataD);
	else
		status = putData(expIdx, cpoPath, path, (struct descriptor *)&emptyXd);
	return status;
}
static int putTimedVect2DFloat(int expIdx, char *cpoPath, char *path, char *timeBasePath, float *data, double *times, int nTimes, int dim1)
{
	EMPTYXD(emptyXd);
	DESCRIPTOR_A_COEFF(dataD, sizeof(float), DTYPE_FLOAT, 0, 2, 0);
	int status;
	dataD.arsize = dim1*nTimes*sizeof(float);
	dataD.m[0] = dim1;
	dataD.m[1] = nTimes;
	dataD.pointer = (char *)data;
	if(nTimes * dim1 > 0)
		status = putSegment(expIdx, cpoPath, path, timeBasePath, (struct descriptor_a *)&dataD, times, nTimes);
	else
		status = putData(expIdx, cpoPath, path, (struct descriptor *)&emptyXd);
	return status;
}





 int mdsPutVect2DDouble(int expIdx, char *cpoPath, char *path, char *timeBasePath, double *data, int dim1, int dim2, int isTimed)
{
	EMPTYXD(emptyXd);
	DESCRIPTOR_A_COEFF(dataD, sizeof(double), DTYPE_DOUBLE, 0, 2, 0);
	int status;
        if(isTimed)
            return putTimedVect2DDouble(expIdx, cpoPath, path, timeBasePath, data, putTimes, nPutTimes, dim1);
	dataD.arsize = dim1*dim2*sizeof(double);
	dataD.m[0] = dim1;
	dataD.m[1] = dim2;
	dataD.pointer = (char *)data;
	if(dim1 * dim2 > 0)
		status = putData(expIdx, cpoPath, path, (struct descriptor *)&dataD);
	else
		status = putData(expIdx, cpoPath, path, (struct descriptor *)&emptyXd);
	return status;
}
static int putTimedVect2DDouble(int expIdx, char *cpoPath, char *path, char *timeBasePath, double *data, double *times, int nTimes, int dim1)
{
	EMPTYXD(emptyXd);
	DESCRIPTOR_A_COEFF(dataD, sizeof(double), DTYPE_DOUBLE, 0, 2, 0);
	int status;
	dataD.arsize = dim1*nTimes*sizeof(double);
	dataD.m[0] = dim1;
	dataD.m[1] = nTimes;
	dataD.pointer = (char *)data;
	if(nTimes * dim1 > 0)
		status = putSegment(expIdx, cpoPath, path, timeBasePath, (struct descriptor_a *)&dataD, times, nTimes);
	else
		status = putData(expIdx, cpoPath, path, (struct descriptor *)&emptyXd);
	return status;
}





 int mdsPutVect3DInt(int expIdx, char *cpoPath, char *path, char *timeBasePath, int *data, int dim1, int dim2, int dim3, int isTimed)
{
	EMPTYXD(emptyXd);
	DESCRIPTOR_A_COEFF(dataD, sizeof(int), DTYPE_L, 0, 3, 0);
	int status;
        if(isTimed)
            return putTimedVect3DInt(expIdx, cpoPath, path, timeBasePath, data, putTimes, nPutTimes, dim1, dim2);
	dataD.arsize = dim1*dim2*dim3*sizeof(int);
	dataD.m[0] = dim1;
	dataD.m[1] = dim2;
	dataD.m[2] = dim3;
	dataD.pointer = (char *)data;
	if(dim1 * dim2 * dim3> 0)
		status = putData(expIdx, cpoPath, path, (struct descriptor *)&dataD);
	else
		status = putData(expIdx, cpoPath, path, (struct descriptor *)&emptyXd);
	return status;
}
static int putTimedVect3DInt(int expIdx, char *cpoPath, char *path, char *timeBasePath, int *data, double *times, int nTimes, int dim1,
    int dim2)
{
	EMPTYXD(emptyXd);
	DESCRIPTOR_A_COEFF(dataD, sizeof(int), DTYPE_L, 0, 3, 0);
	int status;
	dataD.arsize = dim1*dim2*nTimes*sizeof(int);
	dataD.m[0] = dim1;
	dataD.m[1] = dim2;
	dataD.m[2] = nTimes;
	dataD.pointer = (char *)data;
	if(dim1 * dim2 * nTimes> 0)
		status = putSegment(expIdx, cpoPath, path, timeBasePath, (struct descriptor_a *)&dataD, times, nTimes);
	else
		status = putData(expIdx, cpoPath, path, (struct descriptor *)&emptyXd);
	return status;
}


 int mdsPutVect3DFloat(int expIdx, char *cpoPath, char *path, char *timeBasePath, float *data, int dim1, int dim2, int dim3, int isTimed)
{
	EMPTYXD(emptyXd);
	DESCRIPTOR_A_COEFF(dataD, sizeof(float), DTYPE_FLOAT, 0, 3, 0);
	int status;
        if(isTimed)
            return putTimedVect3DFloat(expIdx, cpoPath, path, timeBasePath, data, putTimes, nPutTimes, dim1, dim2);
	dataD.arsize = dim1*dim2*dim3*sizeof(float);
	dataD.m[0] = dim1;
	dataD.m[1] = dim2;
	dataD.m[2] = dim3;

	dataD.pointer = (char *)data;
	if(dim1 * dim2 * dim3> 0)
		status = putData(expIdx, cpoPath, path, (struct descriptor *)&dataD);
	else
		status = putData(expIdx, cpoPath, path, (struct descriptor *)&emptyXd);
	return status;
}

static int putTimedVect3DFloat(int expIdx, char *cpoPath, char *path, char *timeBasePath, float *data, double *times, int nTimes, int dim1,
    int dim2)
{
	EMPTYXD(emptyXd);
	DESCRIPTOR_A_COEFF(dataD, sizeof(float), DTYPE_FLOAT, 0, 3, 0);
	int status;
	dataD.arsize = dim1*dim2*nTimes*sizeof(float);
	dataD.m[0] = dim1;
	dataD.m[1] = dim2;
	dataD.m[2] = nTimes;
	dataD.pointer = (char *)data;
	if(dim1 * dim2 * nTimes> 0)
		status = putSegment(expIdx, cpoPath, path, timeBasePath, (struct descriptor_a *)&dataD, times, nTimes);
	else
		status = putData(expIdx, cpoPath, path, (struct descriptor *)&emptyXd);
	return status;
}




 int mdsPutVect3DDouble(int expIdx, char *cpoPath, char *path, char *timeBasePath, double *data, int dim1, int dim2, int dim3, int isTimed)
{
	EMPTYXD(emptyXd);
	DESCRIPTOR_A_COEFF(dataD, sizeof(double), DTYPE_DOUBLE, 0, 3, 0);
	int status;
        if(isTimed)
            return putTimedVect3DDouble(expIdx, cpoPath, path, timeBasePath, data, putTimes, nPutTimes, dim1, dim2);
	dataD.arsize = dim1*dim2*dim3*sizeof(double);
	dataD.m[0] = dim1;
	dataD.m[1] = dim2;
	dataD.m[2] = dim3;
	dataD.pointer = (char *)data;
	if(dim1 * dim2 * dim3> 0)
		status = putData(expIdx, cpoPath, path, (struct descriptor *)&dataD);
	else
		status = putData(expIdx, cpoPath, path, (struct descriptor *)&emptyXd);
	return status;
}
static int putTimedVect3DDouble(int expIdx, char *cpoPath, char *path, char *timeBasePath, double *data, double *times, int nTimes, int dim1,
    int dim2)
{
	EMPTYXD(emptyXd);
	DESCRIPTOR_A_COEFF(dataD, sizeof(double), DTYPE_DOUBLE, 0, 3, 0);
	int status;
	dataD.arsize = dim1*dim2*nTimes*sizeof(double);
	dataD.m[0] = dim1;
	dataD.m[1] = dim2;
	dataD.m[2] = nTimes;
	dataD.pointer = (char *)data;
	if(dim1 * dim2 * nTimes> 0)
		status = putSegment(expIdx, cpoPath, path, timeBasePath, (struct descriptor_a *)&dataD, times, nTimes);
	else
		status = putData(expIdx, cpoPath, path, (struct descriptor *)&emptyXd);
	return status;
}




 int mdsPutVect4DFloat(int expIdx, char *cpoPath, char *path, char *timeBasePath, float *data, int dim1, int dim2, int dim3, int dim4, int isTimed)
{
	EMPTYXD(emptyXd);
	DESCRIPTOR_A_COEFF(dataD, sizeof(float), DTYPE_FLOAT, 0, 4, 0);
	int status;
        if(isTimed)
            return putTimedVect4DFloat(expIdx, cpoPath, path, timeBasePath, data, putTimes, nPutTimes, dim1, dim2, dim3);
	dataD.arsize = dim1*dim2*dim3*dim4*sizeof(float);
	dataD.m[0] = dim1;
	dataD.m[1] = dim2;
	dataD.m[2] = dim3;
	dataD.m[3] = dim4;
	dataD.pointer = (char *)data;
	if(dim1 * dim2 * dim3 * dim4> 0)
		status = putData(expIdx, cpoPath, path, (struct descriptor *)&dataD);
	else
		status = putData(expIdx, cpoPath, path, (struct descriptor *)&emptyXd);
	return status;
}

static int putTimedVect4DFloat(int expIdx, char *cpoPath, char *path, char *timeBasePath, float *data, double *times, int nTimes, int dim1, int dim2, int dim3)
{
	EMPTYXD(emptyXd);
	DESCRIPTOR_A_COEFF(dataD, sizeof(float), DTYPE_FLOAT, 0, 4, 0);
	int status;
	dataD.arsize = dim1*dim2*dim3*nTimes*sizeof(float);
	dataD.m[0] = dim1;
	dataD.m[1] = dim2;
	dataD.m[2] = dim3;
	dataD.m[3] = nTimes;
	dataD.pointer = (char *)data;
	if(dim1 * dim2 * dim3 * nTimes> 0)
		status =  putSegment(expIdx, cpoPath, path, timeBasePath, (struct descriptor_a *)&dataD, times, nTimes);
	else
		status = putData(expIdx, cpoPath, path, (struct descriptor *)&emptyXd);
	return status;
}




 int mdsPutVect4DDouble(int expIdx, char *cpoPath, char *path, char *timeBasePath, double *data, int dim1, int dim2, int dim3, int dim4, int isTimed)
{
	EMPTYXD(emptyXd);
	DESCRIPTOR_A_COEFF(dataD, sizeof(double), DTYPE_DOUBLE, 0, 4, 0);
	int status;
         if(isTimed)
            return putTimedVect4DDouble(expIdx, cpoPath, path, timeBasePath, data, putTimes, nPutTimes, dim1, dim2, dim3);
	dataD.arsize = dim1*dim2*dim3*dim4*sizeof(double);
	dataD.m[0] = dim1;
	dataD.m[1] = dim2;
	dataD.m[2] = dim3;
	dataD.m[3] = dim4;
	dataD.pointer = (char *)data;
	if(dim1 * dim2 * dim3 * dim4> 0)
		status =  putData(expIdx, cpoPath, path, (struct descriptor *)&dataD);
	else
		status = putData(expIdx, cpoPath, path, (struct descriptor *)&emptyXd);
	return status;
}
static int putTimedVect4DDouble(int expIdx, char *cpoPath, char *path, char *timeBasePath, double *data, double *times, int nTimes, int dim1, int dim2, int dim3)
{
	EMPTYXD(emptyXd);
	DESCRIPTOR_A_COEFF(dataD, sizeof(double), DTYPE_DOUBLE, 0, 4, 0);
	int status;
	dataD.arsize = dim1*dim2*dim3*nTimes*sizeof(double);
	dataD.m[0] = dim1;
	dataD.m[1] = dim2;
	dataD.m[2] = dim3;
	dataD.m[3] = nTimes;
	dataD.pointer = (char *)data;
	if(dim1 * dim2 * dim3 * nTimes> 0)
		status =  putSegment(expIdx, cpoPath, path, timeBasePath, (struct descriptor_a *)&dataD, times, nTimes);
	else
		status = putData(expIdx, cpoPath, path, (struct descriptor *)&emptyXd);
	return status;
}



 int mdsPutVect4DInt(int expIdx, char *cpoPath, char *path, char *timeBasePath, int *data, int dim1, int dim2, int dim3, int dim4, int isTimed)
{
	EMPTYXD(emptyXd);
	DESCRIPTOR_A_COEFF(dataD, sizeof(double), DTYPE_L, 0, 4, 0);
	int status;
        if(isTimed)
            return putTimedVect4DInt(expIdx, cpoPath, path, timeBasePath, data, putTimes, nPutTimes, dim1, dim2, dim3);
	dataD.arsize = dim1*dim2*dim3*dim4*sizeof(double);
	dataD.m[0] = dim1;
	dataD.m[1] = dim2;
	dataD.m[2] = dim3;
	dataD.m[3] = dim4;
	dataD.pointer = (char *)data;
	if(dim1 * dim2 * dim3 * dim4> 0)
		status =  putData(expIdx, cpoPath, path, (struct descriptor *)&dataD);
	else
		status = putData(expIdx, cpoPath, path, (struct descriptor *)&emptyXd);
	return status;
}

static int putTimedVect4DInt(int expIdx, char *cpoPath, char *path, char *timeBasePath, int *data, double *times, int nTimes, int dim1, int dim2, int dim3)
{
	EMPTYXD(emptyXd);
	DESCRIPTOR_A_COEFF(dataD, sizeof(int), DTYPE_L, 0, 4, 0);
	int status;
	dataD.arsize = dim1*dim2*dim3*nTimes*sizeof(int);
	dataD.m[0] = dim1;
	dataD.m[1] = dim2;
	dataD.m[2] = dim3;
	dataD.m[3] = nTimes;
	dataD.pointer = (char *)data;
	if(dim1 * dim2 * dim3 * nTimes> 0)
		status = putSegment(expIdx, cpoPath, path, timeBasePath, (struct descriptor_a *)&dataD, times, nTimes);
	else
		status = putData(expIdx, cpoPath, path, (struct descriptor *)&emptyXd);
	return status;
}


 int mdsPutVect5DFloat(int expIdx, char *cpoPath, char *path, char *timeBasePath, float *data, int dim1, int dim2, int dim3, int dim4, int dim5, int isTimed)
{
	EMPTYXD(emptyXd);
	DESCRIPTOR_A_COEFF(dataD, sizeof(float), DTYPE_FLOAT, 0, 5, 0);
	int status;
        if(isTimed)
            return putTimedVect5DFloat(expIdx, cpoPath, path, timeBasePath, data, putTimes, nPutTimes, dim1, dim2, dim3, dim4);
	dataD.arsize = dim1*dim2*dim3*dim4*dim5*sizeof(float);
	dataD.m[0] = dim1;
	dataD.m[1] = dim2;
	dataD.m[2] = dim3;
	dataD.m[3] = dim4;
	dataD.m[4] = dim5;
	dataD.pointer = (char *)data;
	if(dim1 * dim2 * dim3 * dim4 * dim5> 0)
		status =  putData(expIdx, cpoPath, path, (struct descriptor *)&dataD);
	else
		status = putData(expIdx, cpoPath, path, (struct descriptor *)&emptyXd);
	return status;
}

static int putTimedVect5DFloat(int expIdx, char *cpoPath, char *path, char *timeBasePath, float *data, double *times, int nTimes, int dim1, int dim2, int dim3, int dim4)
{
	EMPTYXD(emptyXd);
	DESCRIPTOR_A_COEFF(dataD, sizeof(float), DTYPE_FLOAT, 0, 5, 0);
	int status;
	dataD.arsize = dim1*dim2*dim3*dim4*nTimes*sizeof(float);
	dataD.m[0] = dim1;
	dataD.m[1] = dim2;
	dataD.m[2] = dim3;
	dataD.m[3] = dim4;
	dataD.m[4] = nTimes;
	dataD.pointer = (char *)data;
	if(dim1 * dim2 * dim3 * dim4 * nTimes> 0)
		status =  putSegment(expIdx, cpoPath, path, timeBasePath, (struct descriptor_a *)&dataD, times, nTimes);
	else
		status = putData(expIdx, cpoPath, path, (struct descriptor *)&emptyXd);
	return status;
}




 int mdsPutVect5DDouble(int expIdx, char *cpoPath, char *path, char *timeBasePath, double *data, int dim1, int dim2, int dim3, int dim4, int dim5, int isTimed)
{
	EMPTYXD(emptyXd);
	DESCRIPTOR_A_COEFF(dataD, sizeof(double), DTYPE_DOUBLE, 0, 5, 0);
	int status;
         if(isTimed)
            return putTimedVect5DDouble(expIdx, cpoPath, path, timeBasePath, data, putTimes, nPutTimes, dim1, dim2, dim3, dim4);
	dataD.arsize = dim1*dim2*dim3*dim4*dim5*sizeof(double);
	dataD.m[0] = dim1;
	dataD.m[1] = dim2;
	dataD.m[2] = dim3;
	dataD.m[3] = dim4;
	dataD.m[4] = dim5;
	dataD.pointer = (char *)data;
	if(dim1 * dim2 * dim3 * dim4*dim5> 0)
		status =  putData(expIdx, cpoPath, path, (struct descriptor *)&dataD);
	else
		status = putData(expIdx, cpoPath, path, (struct descriptor *)&emptyXd);
	return status;
}
static int putTimedVect5DDouble(int expIdx, char *cpoPath, char *path, char *timeBasePath, double *data, double *times, int nTimes, int dim1, int dim2, int dim3, int dim4)
{
	EMPTYXD(emptyXd);
	DESCRIPTOR_A_COEFF(dataD, sizeof(double), DTYPE_DOUBLE, 0, 5, 0);
	int status;
	dataD.arsize = dim1*dim2*dim3*dim4*nTimes*sizeof(double);
	dataD.m[0] = dim1;
	dataD.m[1] = dim2;
	dataD.m[2] = dim3;
	dataD.m[3] = dim4;
	dataD.m[4] = nTimes;
	dataD.pointer = (char *)data;
	if(dim1 * dim2 * dim3 * dim4 * nTimes> 0)
		status =  putSegment(expIdx, cpoPath, path, timeBasePath, (struct descriptor_a *)&dataD, times, nTimes);
	else
		status = putData(expIdx, cpoPath, path, (struct descriptor *)&emptyXd);
	return status;
}



 int mdsPutVect5DInt(int expIdx, char *cpoPath, char *path, char *timeBasePath, int *data, int dim1, int dim2, int dim3, int dim4, int dim5, int isTimed)
{
	EMPTYXD(emptyXd);
	DESCRIPTOR_A_COEFF(dataD, sizeof(double), DTYPE_L, 0, 5, 0);
	int status;
        if(isTimed)
            return putTimedVect5DInt(expIdx, cpoPath, path, timeBasePath, data, putTimes, nPutTimes, dim1, dim2, dim3, dim4);
	dataD.arsize = dim1*dim2*dim3*dim4*dim5*sizeof(double);
	dataD.m[0] = dim1;
	dataD.m[1] = dim2;
	dataD.m[2] = dim3;
	dataD.m[3] = dim4;
	dataD.m[4] = dim5;
	dataD.pointer = (char *)data;
	if(dim1 * dim2 * dim3 * dim4 * dim5> 0)
		status =  putData(expIdx, cpoPath, path, (struct descriptor *)&dataD);
	else
		status = putData(expIdx, cpoPath, path, (struct descriptor *)&emptyXd);
	return status;
}

static int putTimedVect5DInt(int expIdx, char *cpoPath, char *path, char *timeBasePath, int *data, double *times, int nTimes, int dim1,
    int dim2, int dim3, int dim4)
{
	EMPTYXD(emptyXd);
	DESCRIPTOR_A_COEFF(dataD, sizeof(int), DTYPE_L, 0, 5, 0);
	int status;

	dataD.arsize = dim1*dim2*dim3*dim4*nTimes*sizeof(int);
	dataD.m[0] = dim1;
	dataD.m[1] = dim2;
	dataD.m[2] = dim3;
	dataD.m[3] = dim4;
	dataD.m[4] = nTimes;
	dataD.pointer = (char *)data;
	if(dim1 * dim2 * dim3 * dim4 * nTimes> 0)
		status = putSegment(expIdx, cpoPath, path, timeBasePath, (struct descriptor_a *)&dataD, times, nTimes);
	else
		status = putData(expIdx, cpoPath, path, (struct descriptor *)&emptyXd);
	return status;
}

 int mdsPutVect6DFloat(int expIdx, char *cpoPath, char *path, char *timeBasePath, float *data, int dim1, int dim2, int dim3,
int dim4, int dim5, int dim6, int isTimed)
{
	EMPTYXD(emptyXd);
	DESCRIPTOR_A_COEFF(dataD, sizeof(float), DTYPE_FLOAT, 0, 6, 0);
	int status;
        if(isTimed)
            return putTimedVect6DFloat(expIdx, cpoPath, path, timeBasePath, data, putTimes, nPutTimes, dim1, dim2, dim3, dim4, dim5);
	dataD.arsize = dim1*dim2*dim3*dim4*dim5*dim6*sizeof(float);
	dataD.m[0] = dim1;
	dataD.m[1] = dim2;
	dataD.m[2] = dim3;
	dataD.m[3] = dim4;
	dataD.m[4] = dim5;
	dataD.m[5] = dim6;
	dataD.pointer = (char *)data;
	if(dim1 * dim2 * dim3 * dim4 * dim5 * dim6> 0)
		status =  putData(expIdx, cpoPath, path, (struct descriptor *)&dataD);
	else
		status = putData(expIdx, cpoPath, path, (struct descriptor *)&emptyXd);
	return status;
}

static int putTimedVect6DFloat(int expIdx, char *cpoPath, char *path, char *timeBasePath, float *data, double *times, int nTimes, int dim1,
    int dim2, int dim3, int dim4, int dim5)
{
	EMPTYXD(emptyXd);
	DESCRIPTOR_A_COEFF(dataD, sizeof(float), DTYPE_FLOAT, 0, 6, 0);
	int status;
	dataD.arsize = dim1*dim2*dim3*dim4*dim5*nTimes*sizeof(float);
	dataD.m[0] = dim1;
	dataD.m[1] = dim2;
	dataD.m[2] = dim3;
	dataD.m[3] = dim4;
	dataD.m[4] = dim5;
	dataD.m[5] = nTimes;
	dataD.pointer = (char *)data;
	if(dim1 * dim2 * dim3 * dim4 * dim5 * nTimes> 0)
		status =  putSegment(expIdx, cpoPath, path, timeBasePath, (struct descriptor_a *)&dataD, times, nTimes);
	else
		status = putData(expIdx, cpoPath, path, (struct descriptor *)&emptyXd);
	return status;
}




 int mdsPutVect6DDouble(int expIdx, char *cpoPath, char *path, char *timeBasePath, double *data, int dim1, int dim2, int dim3,
    int dim4, int dim5, int dim6, int isTimed)
{
	EMPTYXD(emptyXd);
	DESCRIPTOR_A_COEFF(dataD, sizeof(double), DTYPE_DOUBLE, 0, 6, 0);
	int status;
         if(isTimed)
            return putTimedVect6DDouble(expIdx, cpoPath, path, timeBasePath, data, putTimes, nPutTimes, dim1, dim2, dim3, dim4, dim5);
	dataD.arsize = dim1*dim2*dim3*dim4*dim5*dim6*sizeof(double);
	dataD.m[0] = dim1;
	dataD.m[1] = dim2;
	dataD.m[2] = dim3;
	dataD.m[3] = dim4;
	dataD.m[4] = dim5;
	dataD.m[5] = dim6;
	dataD.pointer = (char *)data;
	if(dim1 * dim2 * dim3 * dim4*dim5*dim6> 0)
		status =  putData(expIdx, cpoPath, path, (struct descriptor *)&dataD);
	else
		status = putData(expIdx, cpoPath, path, (struct descriptor *)&emptyXd);
	return status;
}
static int putTimedVect6DDouble(int expIdx, char *cpoPath, char *path, char *timeBasePath, double *data, double *times, int nTimes,
    int dim1, int dim2, int dim3, int dim4, int dim5)
{
	EMPTYXD(emptyXd);
	DESCRIPTOR_A_COEFF(dataD, sizeof(double), DTYPE_DOUBLE, 0, 6, 0);
	int status;
	dataD.arsize = dim1*dim2*dim3*dim4*dim5*nTimes*sizeof(double);
	dataD.m[0] = dim1;
	dataD.m[1] = dim2;
	dataD.m[2] = dim3;
	dataD.m[3] = dim4;
	dataD.m[4] = dim5;
	dataD.m[5] = nTimes;
	dataD.pointer = (char *)data;
	if(dim1 * dim2 * dim3 * dim4 * dim5 * nTimes> 0)
		status =  putSegment(expIdx, cpoPath, path, timeBasePath, (struct descriptor_a *)&dataD, times, nTimes);
	else
		status = putData(expIdx, cpoPath, path, (struct descriptor *)&emptyXd);
	return status;
}



 int mdsPutVect6DInt(int expIdx, char *cpoPath, char *path, char *timeBasePath, int *data, int dim1, int dim2, int dim3, int dim4,
    int dim5, int dim6, int isTimed)
{
	EMPTYXD(emptyXd);
	DESCRIPTOR_A_COEFF(dataD, sizeof(double), DTYPE_L, 0, 6, 0);
	int status;
        if(isTimed)
            return putTimedVect6DInt(expIdx, cpoPath, path, timeBasePath, data, putTimes, nPutTimes, dim1, dim2, dim3, dim4, dim5);
	dataD.arsize = dim1*dim2*dim3*dim4*dim5*dim6*sizeof(double);
	dataD.m[0] = dim1;
	dataD.m[1] = dim2;
	dataD.m[2] = dim3;
	dataD.m[3] = dim4;
	dataD.m[4] = dim5;
	dataD.m[5] = dim6;
	dataD.pointer = (char *)data;
	if(dim1 * dim2 * dim3 * dim4 * dim5 * dim6> 0)
		status =  putData(expIdx, cpoPath, path, (struct descriptor *)&dataD);
	else
		status = putData(expIdx, cpoPath, path, (struct descriptor *)&emptyXd);
	return status;
}

static int putTimedVect6DInt(int expIdx, char *cpoPath, char *path, char *timeBasePath, int *data, double *times, int nTimes, int dim1,
    int dim2, int dim3, int dim4, int dim5)
{
	EMPTYXD(emptyXd);
	DESCRIPTOR_A_COEFF(dataD, sizeof(int), DTYPE_L, 0, 6, 0);
	int status;
	dataD.arsize = dim1*dim2*dim3*dim4*dim5*nTimes*sizeof(int);
	dataD.m[0] = dim1;
	dataD.m[1] = dim2;
	dataD.m[2] = dim3;
	dataD.m[3] = dim4;
	dataD.m[4] = dim5;
	dataD.m[5] = nTimes;
	dataD.pointer = (char *)data;
	if(dim1 * dim2 * dim3 * dim4 * dim5 * nTimes> 0)
		status =  putSegment(expIdx, cpoPath, path, timeBasePath, (struct descriptor_a *)&dataD, times, nTimes);
	else
		status = putData(expIdx, cpoPath, path, (struct descriptor *)&emptyXd);
	return status;
}

///////////////////////7D

 int mdsPutVect7DFloat(int expIdx, char *cpoPath, char *path, char *timeBasePath, float *data, int dim1, int dim2, int dim3,
int dim4, int dim5, int dim6, int dim7, int isTimed)
{
	EMPTYXD(emptyXd);
	DESCRIPTOR_A_COEFF(dataD, sizeof(float), DTYPE_FLOAT, 0, 7, 0);
	int status;
        if(isTimed)
            return putTimedVect7DFloat(expIdx, cpoPath, path, timeBasePath, data, putTimes, nPutTimes, dim1, dim2, dim3, dim4, dim5, dim6);
	dataD.arsize = dim1*dim2*dim3*dim4*dim5*dim6*dim7*sizeof(float);
	dataD.m[0] = dim1;
	dataD.m[1] = dim2;
	dataD.m[2] = dim3;
	dataD.m[3] = dim4;
	dataD.m[4] = dim5;
	dataD.m[5] = dim6;
	dataD.m[6] = dim7;
	dataD.pointer = (char *)data;
	if(dim1 * dim2 * dim3 * dim4 * dim5 * dim6 * dim7> 0)
		status =  putData(expIdx, cpoPath, path, (struct descriptor *)&dataD);
	else
		status = putData(expIdx, cpoPath, path, (struct descriptor *)&emptyXd);
	return status;
}

static int putTimedVect7DFloat(int expIdx, char *cpoPath, char *path, char *timeBasePath, float *data, double *times, int nTimes, int dim1,
    int dim2, int dim3, int dim4, int dim5, int dim6)
{
	EMPTYXD(emptyXd);
	DESCRIPTOR_A_COEFF(dataD, sizeof(float), DTYPE_FLOAT, 0, 7, 0);
	int status;
	dataD.arsize = dim1*dim2*dim3*dim4*dim5*dim6*nTimes*sizeof(float);
	dataD.m[0] = dim1;
	dataD.m[1] = dim2;
	dataD.m[2] = dim3;
	dataD.m[3] = dim4;
	dataD.m[4] = dim5;
	dataD.m[5] = dim6;
	dataD.m[6] = nTimes;
	dataD.pointer = (char *)data;
	if(dim1 * dim2 * dim3 * dim4 * dim5 * dim6* nTimes> 0)
		status =  putSegment(expIdx, cpoPath, path, timeBasePath, (struct descriptor_a *)&dataD, times, nTimes);
	else
		status = putData(expIdx, cpoPath, path, (struct descriptor *)&emptyXd);
	return status;
}




 int mdsPutVect7DDouble(int expIdx, char *cpoPath, char *path, char *timeBasePath, double *data, int dim1, int dim2, int dim3,
    int dim4, int dim5, int dim6, int dim7, int isTimed)
{
	EMPTYXD(emptyXd);
	DESCRIPTOR_A_COEFF(dataD, sizeof(double), DTYPE_DOUBLE, 0, 7, 0);
	int status;
         if(isTimed)
            return putTimedVect7DDouble(expIdx, cpoPath, path, timeBasePath, data, putTimes, nPutTimes, dim1, dim2, dim3, dim4, dim5, dim6);
	dataD.arsize = dim1*dim2*dim3*dim4*dim5*dim6*dim7*sizeof(double);
	dataD.m[0] = dim1;
	dataD.m[1] = dim2;
	dataD.m[2] = dim3;
	dataD.m[3] = dim4;
	dataD.m[4] = dim5;
	dataD.m[5] = dim6;
	dataD.m[6] = dim7;
	dataD.pointer = (char *)data;
	if(dim1 * dim2 * dim3 * dim4*dim5*dim6*dim7> 0)
		status =  putData(expIdx, cpoPath, path, (struct descriptor *)&dataD);
	else
		status = putData(expIdx, cpoPath, path, (struct descriptor *)&emptyXd);
	return status;
}
static int putTimedVect7DDouble(int expIdx, char *cpoPath, char *path, char *timeBasePath, double *data, double *times, int nTimes,
    int dim1, int dim2, int dim3, int dim4, int dim5, int dim6)
{
	EMPTYXD(emptyXd);
	DESCRIPTOR_A_COEFF(dataD, sizeof(double), DTYPE_DOUBLE, 0, 7, 0);
	int status;
	dataD.arsize = dim1*dim2*dim3*dim4*dim5*dim6*nTimes*sizeof(double);
	dataD.m[0] = dim1;
	dataD.m[1] = dim2;
	dataD.m[2] = dim3;
	dataD.m[3] = dim4;
	dataD.m[4] = dim5;
	dataD.m[5] = dim6;
	dataD.m[6] = nTimes;
	dataD.pointer = (char *)data;
	if(dim1 * dim2 * dim3 * dim4 * dim5 * dim6 * nTimes> 0)
		status =  putSegment(expIdx, cpoPath, path, timeBasePath, (struct descriptor_a *)&dataD, times, nTimes);
	else
		status = putData(expIdx, cpoPath, path, (struct descriptor *)&emptyXd);
	return status;
}



 int mdsPutVect7DInt(int expIdx, char *cpoPath, char *path, char *timeBasePath, int *data, int dim1, int dim2, int dim3, int dim4,
    int dim5, int dim6, int dim7, int isTimed)
{
	EMPTYXD(emptyXd);
	DESCRIPTOR_A_COEFF(dataD, sizeof(double), DTYPE_L, 0, 7, 0);
	int status;
        if(isTimed)
            return putTimedVect7DInt(expIdx, cpoPath, path, timeBasePath, data, putTimes, nPutTimes, dim1, dim2, dim3, dim4, dim5, dim6);
	dataD.arsize = dim1*dim2*dim3*dim4*dim5*dim6*dim7*sizeof(double);
	dataD.m[0] = dim1;
	dataD.m[1] = dim2;
	dataD.m[2] = dim3;
	dataD.m[3] = dim4;
	dataD.m[4] = dim5;
	dataD.m[5] = dim6;
	dataD.m[6] = dim7;
	dataD.pointer = (char *)data;
	if(dim1 * dim2 * dim3 * dim4 * dim5 * dim6 * dim7> 0)
		status =  putData(expIdx, cpoPath, path, (struct descriptor *)&dataD);
	else
		status = putData(expIdx, cpoPath, path, (struct descriptor *)&emptyXd);
	return status;
}

static int putTimedVect7DInt(int expIdx, char *cpoPath, char *path, char *timeBasePath, int *data, double *times, int nTimes, int dim1,
    int dim2, int dim3, int dim4, int dim5, int dim6)
{
	EMPTYXD(emptyXd);
	DESCRIPTOR_A_COEFF(dataD, sizeof(int), DTYPE_L, 0, 7, 0);
	int status;
	dataD.arsize = dim1*dim2*dim3*dim4*dim5*dim6*nTimes*sizeof(int);
	dataD.m[0] = dim1;
	dataD.m[1] = dim2;
	dataD.m[2] = dim3;
	dataD.m[3] = dim4;
	dataD.m[4] = dim5;
	dataD.m[5] = dim6;
	dataD.m[6] = nTimes;
	dataD.pointer = (char *)data;
	if(dim1 * dim2 * dim3 * dim4 * dim5 * dim6 * nTimes> 0)
		status =  putSegment(expIdx, cpoPath, path, timeBasePath, (struct descriptor_a *)&dataD, times, nTimes);
	else
		status = putData(expIdx, cpoPath, path, (struct descriptor *)&emptyXd);
	return status;
}

////////////////PUT SLICE ROUTINES

 int mdsPutStringSlice(int expIdx, char *cpoPath, char *path, char *timeBasePath, char *data, double time)
{
	DESCRIPTOR_A(dataD, 1, DTYPE_BU, 0, 0);
	int status;
        if(strlen(data) == 0)
            data = " ";
 	dataD.arsize = strlen(data);
	dataD.pointer = data;
	status = putSlice(expIdx, cpoPath, path, timeBasePath, (struct descriptor *)&dataD, time);
	return status;
}
 int mdsPutFloatSlice(int expIdx, char *cpoPath, char *path, char *timeBasePath, float data, double time)
{
	struct descriptor dataD = {0, DTYPE_FLOAT, CLASS_S, 0};
	int status;
	dataD.length = sizeof(float);
	dataD.pointer = (char *)&data;
	status = putSlice(expIdx, cpoPath, path, timeBasePath, &dataD, time);
	return status;
}

 int mdsPutIntSlice(int expIdx, char *cpoPath, char *path, char *timeBasePath, int data, double time)
{
	struct descriptor dataD = {0, DTYPE_L, CLASS_S, 0};
	int status;
	dataD.length = sizeof(int);
	dataD.pointer = (char *)&data;
	status =  putSlice(expIdx, cpoPath, path, timeBasePath, &dataD, time);
	return status;
}

 int mdsPutDoubleSlice(int expIdx, char *cpoPath, char *path, char *timeBasePath, double data, double time)
{
	struct descriptor dataD = {0, DTYPE_DOUBLE, CLASS_S, 0};
	int status;
	dataD.length = sizeof(double);
	dataD.pointer = (char *)&data;
	status = putSlice(expIdx, cpoPath, path, timeBasePath, &dataD, time);
	return status;
}

 int mdsPutVect1DIntSlice(int expIdx, char *cpoPath, char *path, char *timeBasePath, int *data, int dim, double time)
{
	EMPTYXD(emptyXd);
	DESCRIPTOR_A(dataD, sizeof(int), DTYPE_L, 0, 0);
	int status;
	dataD.arsize = sizeof(int) * dim;
	dataD.pointer = (char *)data;
	if(dim > 0)
		status = putSlice(expIdx, cpoPath, path, timeBasePath, (struct descriptor *)&dataD, time);
	else
		status = 0;
	return status;
}



 int mdsPutVect1DDoubleSlice(int expIdx, char *cpoPath, char *path, char *timeBasePath, double *data, int dim, double time)
{
	EMPTYXD(emptyXd);
	DESCRIPTOR_A(dataD, sizeof(double), DTYPE_DOUBLE, 0, 0);
	int status;
	dataD.arsize = sizeof(double) * dim;
	dataD.pointer = (char *)data;
	if(dim > 0)
		status = putSlice(expIdx, cpoPath, path, timeBasePath, (struct descriptor *)&dataD, time);
	else
		status = 0;
	return status;
}

 int mdsPutVect1DFloatSlice(int expIdx, char *cpoPath, char *path, char *timeBasePath, float *data, int dim, double time)
{
	EMPTYXD(emptyXd);
	DESCRIPTOR_A(dataD, sizeof(float), DTYPE_FLOAT, 0, 0);
	int status;
	dataD.arsize = sizeof(float) * dim;
	dataD.pointer = (char *)data;
	if(dim > 0)
		status =  putSlice(expIdx, cpoPath, path, timeBasePath, (struct descriptor *)&dataD, time);
	else
	    status = 0;
	return status;
}


 int mdsPutVect2DIntSlice(int expIdx, char *cpoPath, char *path, char *timeBasePath, int *data, int dim1, int dim2, double time)
{
	EMPTYXD(emptyXd);
	DESCRIPTOR_A_COEFF(dataD, sizeof(int), DTYPE_L, 0, 2, 0);
 	int status;
	dataD.arsize = dim1*dim2*sizeof(int);
	dataD.m[0] = dim1;
	dataD.m[1] = dim2;
	dataD.pointer = (char *)data;
	if(dim1 * dim2 > 0)
		status =  putSlice(expIdx, cpoPath, path, timeBasePath, (struct descriptor *)&dataD, time);
	else
	    status = 0;
	return status;
}


 int mdsPutVect2DFloatSlice(int expIdx, char *cpoPath, char *path, char *timeBasePath, float *data, int dim1, int dim2, double time)
{
	EMPTYXD(emptyXd);
	DESCRIPTOR_A_COEFF(dataD, sizeof(float), DTYPE_FLOAT, 0, 2, 0);
	int status;
	dataD.arsize = dim1*dim2*sizeof(float);
	dataD.m[0] = dim1;
	dataD.m[1] = dim2;
	dataD.pointer = (char *)data;
	if(dim1 * dim2 > 0)
		status =  putSlice(expIdx, cpoPath, path, timeBasePath, (struct descriptor *)&dataD, time);
	else
	    status = 0;
	return status;
}

 int mdsPutVect2DDoubleSlice(int expIdx, char *cpoPath, char *path, char *timeBasePath, double *data, int dim1, int dim2, double time)
{
	EMPTYXD(emptyXd);
	DESCRIPTOR_A_COEFF(dataD, sizeof(double), DTYPE_DOUBLE, 0, 2, 0);
	int status;
	dataD.arsize = dim1*dim2*sizeof(double);
	dataD.m[0] = dim1;
	dataD.m[1] = dim2;
	dataD.pointer = (char *)data;
	if(dim1 * dim2 > 0)
		status =  putSlice(expIdx, cpoPath, path, timeBasePath, (struct descriptor *)&dataD, time);
	else
	    status = 0;
	return status;
}

 int mdsPutVect3DIntSlice(int expIdx, char *cpoPath, char *path, char *timeBasePath, int *data, int dim1, int dim2, int dim3, double time)
{
	EMPTYXD(emptyXd);
	DESCRIPTOR_A_COEFF(dataD, sizeof(int), DTYPE_L, 0, 3, 0);
	int status;
	dataD.arsize = dim1*dim2*dim3*sizeof(int);
	dataD.m[0] = dim1;
	dataD.m[1] = dim2;
	dataD.m[2] = dim3;
	dataD.pointer = (char *)data;
	if(dim1 * dim2 * dim3> 0)
		status =  putSlice(expIdx, cpoPath, path, timeBasePath, (struct descriptor *)&dataD, time);
	else
	    status = 0;
	return status;
}


 int mdsPutVect3DFloatSlice(int expIdx, char *cpoPath, char *path, char *timeBasePath, float *data, int dim1, int dim2, int dim3, double time)
{
	EMPTYXD(emptyXd);
	DESCRIPTOR_A_COEFF(dataD, sizeof(float), DTYPE_FLOAT, 0, 3, 0);
	int status;
	dataD.arsize = dim1*dim2*dim3*sizeof(float);
	dataD.m[0] = dim1;
	dataD.m[1] = dim2;
	dataD.m[2] = dim3;
	dataD.pointer = (char *)data;
	if(dim1 * dim2 * dim3> 0)
		status =  putSlice(expIdx, cpoPath, path, timeBasePath, (struct descriptor *)&dataD, time);
	else
	    status = 0;
	return status;
}

 int mdsPutVect3DDoubleSlice(int expIdx, char *cpoPath, char *path, char *timeBasePath, double *data, int dim1, int dim2, int dim3, double time)
{
	EMPTYXD(emptyXd);
	DESCRIPTOR_A_COEFF(dataD, sizeof(double), DTYPE_DOUBLE, 0, 3, 0);
	int status;
	dataD.arsize = dim1*dim2*dim3*sizeof(double);
	dataD.m[0] = dim1;
	dataD.m[1] = dim2;
	dataD.m[2] = dim3;
	dataD.pointer = (char *)data;
	if(dim1 * dim2 * dim3> 0)
		status =  putSlice(expIdx, cpoPath, path, timeBasePath, (struct descriptor *)&dataD, time);
	else
	    status = 0;
	return status;
}

 int mdsPutVect4DFloatSlice(int expIdx, char *cpoPath, char *path, char *timeBasePath, float *data, int dim1, int dim2, int dim3, int dim4, double time)
{
	EMPTYXD(emptyXd);
	DESCRIPTOR_A_COEFF(dataD, sizeof(double), DTYPE_FLOAT, 0, 4, 0);
	int status;
	dataD.arsize = dim1*dim2*dim3*dim4*sizeof(double);
	dataD.m[0] = dim1;
	dataD.m[1] = dim2;
	dataD.m[2] = dim3;
	dataD.m[3] = dim4;
	dataD.pointer = (char *)data;
	if(dim1 * dim2 * dim3 * dim4> 0)
		status =  putSlice(expIdx, cpoPath, path, timeBasePath, (struct descriptor *)&dataD, time);
	else
	    status = 0;
	return status;
}
 int mdsPutVect4DDoubleSlice(int expIdx, char *cpoPath, char *path, char *timeBasePath, double *data, int dim1, int dim2, int dim3, int dim4, double time)
{
	EMPTYXD(emptyXd);
	DESCRIPTOR_A_COEFF(dataD, sizeof(double), DTYPE_DOUBLE, 0, 4, 0);
	int status;
	dataD.arsize = dim1*dim2*dim3*dim4*sizeof(double);
	dataD.m[0] = dim1;
	dataD.m[1] = dim2;
	dataD.m[2] = dim3;
	dataD.m[3] = dim4;
	dataD.pointer = (char *)data;
	if(dim1 * dim2 * dim3 * dim4> 0)
		status =  putSlice(expIdx, cpoPath, path, timeBasePath, (struct descriptor *)&dataD, time);
	else
	    status = 0;
	return status;
}


 int mdsPutVect4DIntSlice(int expIdx, char *cpoPath, char *path, char *timeBasePath, int *data, int dim1, int dim2, int dim3, int dim4, double time)
{
	EMPTYXD(emptyXd);
	DESCRIPTOR_A_COEFF(dataD, sizeof(double), DTYPE_L, 0, 4, 0);
	int status;
	dataD.arsize = dim1*dim2*dim3*dim4*sizeof(double);
	dataD.m[0] = dim1;
	dataD.m[1] = dim2;
	dataD.m[2] = dim3;
	dataD.m[3] = dim4;
	dataD.pointer = (char *)data;
	if(dim1 * dim2 * dim3 * dim4> 0)
		status = putSlice(expIdx, cpoPath, path, timeBasePath, (struct descriptor *)&dataD, time);
	else
	    status = 0;
	return status;
}

 int mdsPutVect5DFloatSlice(int expIdx, char *cpoPath, char *path, char *timeBasePath, float *data, int dim1, int dim2, int dim3,
    int dim4, int dim5, double time)
{
	EMPTYXD(emptyXd);
	DESCRIPTOR_A_COEFF(dataD, sizeof(double), DTYPE_FLOAT, 0, 5, 0);
	int status;
	dataD.arsize = dim1*dim2*dim3*dim4*dim5*sizeof(double);
	dataD.m[0] = dim1;
	dataD.m[1] = dim2;
	dataD.m[2] = dim3;
	dataD.m[3] = dim4;
	dataD.m[4] = dim5;
	dataD.pointer = (char *)data;
	if(dim1 * dim2 * dim3 * dim4 * dim5> 0)
		status = putSlice(expIdx, cpoPath, path, timeBasePath, (struct descriptor *)&dataD, time);
	else
	    status = 0;
	return status;
}
 int mdsPutVect5DDoubleSlice(int expIdx, char *cpoPath, char *path, char *timeBasePath, double *data, int dim1, int dim2, int dim3,
    int dim4, int dim5, double time)
{
	EMPTYXD(emptyXd);
	DESCRIPTOR_A_COEFF(dataD, sizeof(double), DTYPE_DOUBLE, 0, 5, 0);
	int status;
	dataD.arsize = dim1*dim2*dim3*dim4*dim5*sizeof(double);
	dataD.m[0] = dim1;
	dataD.m[1] = dim2;
	dataD.m[2] = dim3;
	dataD.m[3] = dim4;
	dataD.m[4] = dim5;
	dataD.pointer = (char *)data;
	if(dim1 * dim2 * dim3 * dim4 * dim5> 0)
		status =  putSlice(expIdx, cpoPath, path, timeBasePath, (struct descriptor *)&dataD, time);
	else
	    status = 0;
	return status;
}


 int mdsPutVect5DIntSlice(int expIdx, char *cpoPath, char *path, char *timeBasePath, int *data, int dim1, int dim2, int dim3,
    int dim4, int dim5, double time)
{
	EMPTYXD(emptyXd);
	DESCRIPTOR_A_COEFF(dataD, sizeof(double), DTYPE_L, 0, 5, 0);
 	int status;
	dataD.arsize = dim1*dim2*dim3*dim4*dim5*sizeof(double);
	dataD.m[0] = dim1;
	dataD.m[1] = dim2;
	dataD.m[2] = dim3;
	dataD.m[3] = dim4;
	dataD.m[4] = dim5;
	dataD.pointer = (char *)data;
	if(dim1 * dim2 * dim3 * dim4 * dim5> 0)
		status =  putSlice(expIdx, cpoPath, path, timeBasePath, (struct descriptor *)&dataD, time);
	else
	    status = 0;
	return status;
}

/////6D Slice
 int mdsPutVect6DFloatSlice(int expIdx, char *cpoPath, char *path, char *timeBasePath, float *data, int dim1, int dim2, int dim3,
    int dim4, int dim5, int dim6, double time)
{
	EMPTYXD(emptyXd);
	DESCRIPTOR_A_COEFF(dataD, sizeof(double), DTYPE_FLOAT, 0, 6, 0);
	int status;
	dataD.arsize = dim1*dim2*dim3*dim4*dim5*dim6*sizeof(double);
	dataD.m[0] = dim1;
	dataD.m[1] = dim2;
	dataD.m[2] = dim3;
	dataD.m[3] = dim4;
	dataD.m[4] = dim5;
	dataD.m[5] = dim6;
	dataD.pointer = (char *)data;
	if(dim1 * dim2 * dim3 * dim4 * dim5 * dim6> 0)
		status = putSlice(expIdx, cpoPath, path, timeBasePath, (struct descriptor *)&dataD, time);
	else
	    status = 0;
	return status;
}
 int mdsPutVect6DDoubleSlice(int expIdx, char *cpoPath, char *path, char *timeBasePath, double *data, int dim1, int dim2, int dim3,
    int dim4, int dim5, int dim6, double time)
{
	EMPTYXD(emptyXd);
	DESCRIPTOR_A_COEFF(dataD, sizeof(double), DTYPE_DOUBLE, 0, 6, 0);
	int status;
	dataD.arsize = dim1*dim2*dim3*dim4*dim5*dim6*sizeof(double);
	dataD.m[0] = dim1;
	dataD.m[1] = dim2;
	dataD.m[2] = dim3;
	dataD.m[3] = dim4;
	dataD.m[4] = dim5;
	dataD.m[5] = dim6;
	dataD.pointer = (char *)data;
	if(dim1 * dim2 * dim3 * dim4 * dim5 * dim6> 0)
		status =  putSlice(expIdx, cpoPath, path, timeBasePath, (struct descriptor *)&dataD, time);
	else
	    status = 0;
	return status;
}


 int mdsPutVect6DIntSlice(int expIdx, char *cpoPath, char *path, char *timeBasePath, int *data, int dim1, int dim2, int dim3,
    int dim4, int dim5, int dim6, double time)
{
	EMPTYXD(emptyXd);
	DESCRIPTOR_A_COEFF(dataD, sizeof(double), DTYPE_L, 0, 6, 0);
 	int status;
	dataD.arsize = dim1*dim2*dim3*dim4*dim5*dim6*sizeof(double);
	dataD.m[0] = dim1;
	dataD.m[1] = dim2;
	dataD.m[2] = dim3;
	dataD.m[3] = dim4;
	dataD.m[4] = dim5;
	dataD.m[5] = dim6;
	dataD.pointer = (char *)data;
	if(dim1 * dim2 * dim3 * dim4 * dim5 * dim6> 0)
		status =  putSlice(expIdx, cpoPath, path, timeBasePath, (struct descriptor *)&dataD, time);
	else
	    status = 0;
	return status;
}



//////////////////REPLACE LAST SLICE ROUTINES


 int mdsReplaceLastStringSlice(int expIdx, char *cpoPath, char *path, char *data)
{
	DESCRIPTOR_A(dataD, 1, DTYPE_BU, 0, 0);
	int status;
        if(strlen(data) == 0)
            data = " ";
 	dataD.arsize = strlen(data);
	dataD.pointer = data;
	status =  replaceLastSlice(expIdx, cpoPath, path, (struct descriptor *)&dataD);
	return status;
}
 int mdsReplaceLastFloatSlice(int expIdx, char *cpoPath, char *path, float data)
{
	struct descriptor dataD = {0, DTYPE_FLOAT, CLASS_S, 0};
	int status;
	dataD.length = sizeof(float);
	dataD.pointer = (char *)&data;
	status =  replaceLastSlice(expIdx, cpoPath, path, &dataD);
	return status;
}

 int mdsReplaceLastIntSlice(int expIdx, char *cpoPath, char *path, int data)
{
	struct descriptor dataD = {0, DTYPE_L, CLASS_S, 0};
	int status;
	dataD.length = sizeof(int);
	dataD.pointer = (char *)&data;
	status = replaceLastSlice(expIdx, cpoPath, path, &dataD);
	return status;
}

 int mdsReplaceLastDoubleSlice(int expIdx, char *cpoPath, char *path, double data)
{
	struct descriptor dataD = {0, DTYPE_DOUBLE, CLASS_S, 0};
	int status;
	dataD.length = sizeof(double);
	dataD.pointer = (char *)&data;
	status = replaceLastSlice(expIdx, cpoPath, path, &dataD);
	return status;
}

 int mdsReplaceLastVect1DIntSlice(int expIdx, char *cpoPath, char *path, int *data, int dim)
{
	EMPTYXD(emptyXd);
	DESCRIPTOR_A(dataD, sizeof(int), DTYPE_L, 0, 0);
	int status;
	dataD.arsize = sizeof(int) * dim;
	dataD.pointer = (char *)data;
	if(dim > 0)
		status =  replaceLastSlice(expIdx, cpoPath, path, (struct descriptor *)&dataD);
	else
		status = 0;
	return status;
}



 int mdsReplaceLastVect1DDoubleSlice(int expIdx, char *cpoPath, char *path, double *data, int dim)
{
	EMPTYXD(emptyXd);
	DESCRIPTOR_A(dataD, sizeof(double), DTYPE_DOUBLE, 0, 0);
	int status;
	dataD.arsize = sizeof(double) * dim;
	dataD.pointer = (char *)data;
	if(dim > 0)
		status = replaceLastSlice(expIdx, cpoPath, path, (struct descriptor *)&dataD);
	else
		status = 0;
	return status;
}

 int mdsReplaceLastVect1DFloatSlice(int expIdx, char *cpoPath, char *path, float *data, int dim)
{
	EMPTYXD(emptyXd);
	DESCRIPTOR_A(dataD, sizeof(float), DTYPE_FLOAT, 0, 0);
	int status;
	dataD.arsize = sizeof(float) * dim;
	dataD.pointer = (char *)data;
	if(dim > 0)
		status = replaceLastSlice(expIdx, cpoPath, path, (struct descriptor *)&dataD);
	else
		status = 0;
	return status;
}


 int mdsReplaceLastVect2DIntSlice(int expIdx, char *cpoPath, char *path, int *data, int dim1, int dim2)
{
	EMPTYXD(emptyXd);
	DESCRIPTOR_A_COEFF(dataD, sizeof(int), DTYPE_L, 0, 2, 0);
	int status;
	dataD.arsize = dim1*dim2*sizeof(int);
	dataD.m[0] = dim1;
	dataD.m[1] = dim2;
	dataD.pointer = (char *)data;
	if(dim1 * dim2 > 0)
		status = replaceLastSlice(expIdx, cpoPath, path, (struct descriptor *)&dataD);
	else
		status = 0;
	return status;
}


 int mdsReplaceLastVect2DFloatSlice(int expIdx, char *cpoPath, char *path, float *data, int dim1, int dim2)
{
	EMPTYXD(emptyXd);
	DESCRIPTOR_A_COEFF(dataD, sizeof(float), DTYPE_FLOAT, 0, 2, 0);
	int status;
	dataD.arsize = dim1*dim2*sizeof(float);
	dataD.m[0] = dim1;
	dataD.m[1] = dim2;
	dataD.pointer = (char *)data;
	if(dim1 * dim2 > 0)
		status = replaceLastSlice(expIdx, cpoPath, path, (struct descriptor *)&dataD);
	else
		status = 0;
	return status;
}

 int mdsReplaceLastVect2DDoubleSlice(int expIdx, char *cpoPath, char *path, double *data, int dim1, int dim2)
{
	EMPTYXD(emptyXd);
	DESCRIPTOR_A_COEFF(dataD, sizeof(double), DTYPE_DOUBLE, 0, 2, 0);
	int status;
	dataD.arsize = dim1*dim2*sizeof(double);
	dataD.m[0] = dim1;
	dataD.m[1] = dim2;
	dataD.pointer = (char *)data;
	if(dim1 * dim2 > 0)
		status = replaceLastSlice(expIdx, cpoPath, path, (struct descriptor *)&dataD);
	else
		status = 0;
	return status;
}

 int mdsReplaceLastVect3DIntSlice(int expIdx, char *cpoPath, char *path, int *data, int dim1, int dim2, int dim3)
{
	EMPTYXD(emptyXd);
	DESCRIPTOR_A_COEFF(dataD, sizeof(int), DTYPE_L, 0, 3, 0);
	int status;
	dataD.arsize = dim1*dim2*dim3*sizeof(int);
	dataD.m[0] = dim1;
	dataD.m[1] = dim2;
	dataD.m[2] = dim3;
	dataD.pointer = (char *)data;
	if(dim1 * dim2 * dim3> 0)
		status = replaceLastSlice(expIdx, cpoPath, path, (struct descriptor *)&dataD);
	else
		status = 0;
	return status;
}


 int mdsReplaceLastVect3DFloatSlice(int expIdx, char *cpoPath, char *path, float *data, int dim1, int dim2, int dim3)
{
	EMPTYXD(emptyXd);
	DESCRIPTOR_A_COEFF(dataD, sizeof(float), DTYPE_FLOAT, 0, 3, 0);
	int status;
	dataD.arsize = dim1*dim2*dim3*sizeof(float);
	dataD.m[0] = dim1;
	dataD.m[1] = dim2;
	dataD.m[2] = dim3;
	dataD.pointer = (char *)data;
	if(dim1 * dim2 * dim3> 0)
		status = replaceLastSlice(expIdx, cpoPath, path, (struct descriptor *)&dataD);
	else
		status = 0;
	return status;
}

 int mdsReplaceLastVect3DDoubleSlice(int expIdx, char *cpoPath, char *path, double *data, int dim1, int dim2, int dim3)
{
	EMPTYXD(emptyXd);
	DESCRIPTOR_A_COEFF(dataD, sizeof(double), DTYPE_DOUBLE, 0, 3, 0);
	int status;
	dataD.arsize = dim1*dim2*dim3*sizeof(double);
	dataD.m[0] = dim1;
	dataD.m[1] = dim2;
	dataD.m[2] = dim3;
	dataD.pointer = (char *)data;
	if(dim1 * dim2 * dim3> 0)
		status = replaceLastSlice(expIdx, cpoPath, path, (struct descriptor *)&dataD);
	else
		status = 0;
	return status;
}

 int mdsReplaceLastVect4DFloatSlice(int expIdx, char *cpoPath, char *path, float *data, int dim1, int dim2, int dim3, int dim4)
{
	EMPTYXD(emptyXd);
	DESCRIPTOR_A_COEFF(dataD, sizeof(double), DTYPE_FLOAT, 0, 4, 0);
	int status;
	dataD.arsize = dim1*dim2*dim3*dim4*sizeof(double);
	dataD.m[0] = dim1;
	dataD.m[1] = dim2;
	dataD.m[2] = dim3;
	dataD.m[3] = dim4;
	dataD.pointer = (char *)data;
	if(dim1 * dim2 * dim3 * dim4> 0)
		status = replaceLastSlice(expIdx, cpoPath, path, (struct descriptor *)&dataD);
	else
		status = 0;
	return status;
}
 int mdsReplaceLastVect4DDoubleSlice(int expIdx, char *cpoPath, char *path, double *data, int dim1, int dim2, int dim3, int dim4)
{
	EMPTYXD(emptyXd);
	DESCRIPTOR_A_COEFF(dataD, sizeof(double), DTYPE_DOUBLE, 0, 4, 0);
	int status;
 	dataD.arsize = dim1*dim2*dim3*dim4*sizeof(double);
	dataD.m[0] = dim1;
	dataD.m[1] = dim2;
	dataD.m[2] = dim3;
	dataD.m[3] = dim4;
	dataD.pointer = (char *)data;
	if(dim1 * dim2 * dim3 * dim4> 0)
		status = replaceLastSlice(expIdx, cpoPath, path, (struct descriptor *)&dataD);
	else
		status = 0;
	return status;
}


 int mdsReplaceLastVect4DIntSlice(int expIdx, char *cpoPath, char *path, int *data, int dim1, int dim2, int dim3, int dim4)
{
	EMPTYXD(emptyXd);
	DESCRIPTOR_A_COEFF(dataD, sizeof(double), DTYPE_L, 0, 4, 0);
	int status;
	dataD.arsize = dim1*dim2*dim3*dim4*sizeof(double);
	dataD.m[0] = dim1;
	dataD.m[1] = dim2;
	dataD.m[2] = dim3;
	dataD.m[3] = dim4;
	dataD.pointer = (char *)data;
	if(dim1 * dim2 * dim3 * dim4> 0)
		status =  replaceLastSlice(expIdx, cpoPath, path, (struct descriptor *)&dataD);
	else
		status = 0;
	return status;
}


 int mdsReplaceLastVect5DFloatSlice(int expIdx, char *cpoPath, char *path, float *data, int dim1, int dim2,
    int dim3, int dim4, int dim5)
{
	EMPTYXD(emptyXd);
	DESCRIPTOR_A_COEFF(dataD, sizeof(double), DTYPE_FLOAT, 0, 5, 0);
	int status;
	dataD.arsize = dim1*dim2*dim3*dim4*dim5*sizeof(double);
	dataD.m[0] = dim1;
	dataD.m[1] = dim2;
	dataD.m[2] = dim3;
	dataD.m[3] = dim4;
	dataD.m[4] = dim5;
	dataD.pointer = (char *)data;
	if(dim1 * dim2 * dim3 * dim4 * dim5> 0)
		status =  replaceLastSlice(expIdx, cpoPath, path, (struct descriptor *)&dataD);
	else
		status = 0;
	return status;
}
 int mdsReplaceLastVect5DDoubleSlice(int expIdx, char *cpoPath, char *path, double *data, int dim1, int dim2,
    int dim3, int dim4, int dim5)
{
	EMPTYXD(emptyXd);
	DESCRIPTOR_A_COEFF(dataD, sizeof(double), DTYPE_DOUBLE, 0, 5, 0);
	int status;
	dataD.arsize = dim1*dim2*dim3*dim4*dim5*sizeof(double);
	dataD.m[0] = dim1;
	dataD.m[1] = dim2;
	dataD.m[2] = dim3;
	dataD.m[3] = dim4;
	dataD.m[4] = dim5;
	dataD.pointer = (char *)data;
	if(dim1 * dim2 * dim3 * dim4 * dim5> 0)
		status =  replaceLastSlice(expIdx, cpoPath, path, (struct descriptor *)&dataD);
	else
		status = 0;
	return status;
}


 int mdsReplaceLastVect5DIntSlice(int expIdx, char *cpoPath, char *path, int *data, int dim1, int dim2,
    int dim3, int dim4, int dim5)
{
	EMPTYXD(emptyXd);
	DESCRIPTOR_A_COEFF(dataD, sizeof(double), DTYPE_L, 0, 5, 0);
	int status;
	dataD.arsize = dim1*dim2*dim3*dim4*dim5*sizeof(double);
	dataD.m[0] = dim1;
	dataD.m[1] = dim2;
	dataD.m[2] = dim3;
	dataD.m[3] = dim4;
	dataD.m[4] = dim5;
	dataD.pointer = (char *)data;
	if(dim1 * dim2 * dim3 * dim4 * dim5> 0)
		status =  replaceLastSlice(expIdx, cpoPath, path, (struct descriptor *)&dataD);
	else
		status = 0;
	return status;
}
//////////6D Slice
 int mdsReplaceLastVect6DFloatSlice(int expIdx, char *cpoPath, char *path, float *data, int dim1, int dim2,
    int dim3, int dim4, int dim5, int dim6)
{
	EMPTYXD(emptyXd);
	DESCRIPTOR_A_COEFF(dataD, sizeof(double), DTYPE_FLOAT, 0, 6, 0);
	int status;
	dataD.arsize = dim1*dim2*dim3*dim4*dim5*dim6*sizeof(double);
	dataD.m[0] = dim1;
	dataD.m[1] = dim2;
	dataD.m[2] = dim3;
	dataD.m[3] = dim4;
	dataD.m[4] = dim5;
	dataD.m[5] = dim6;
	dataD.pointer = (char *)data;
	if(dim1 * dim2 * dim3 * dim4 * dim5 * dim5> 0)
		status =  replaceLastSlice(expIdx, cpoPath, path, (struct descriptor *)&dataD);
	else
		status = 0;
	return status;
}
 int mdsReplaceLastVect6DDoubleSlice(int expIdx, char *cpoPath, char *path, double *data, int dim1, int dim2,
    int dim3, int dim4, int dim5, int dim6)
{
	EMPTYXD(emptyXd);
	DESCRIPTOR_A_COEFF(dataD, sizeof(double), DTYPE_DOUBLE, 0, 6, 0);
	int status;
	dataD.arsize = dim1*dim2*dim3*dim4*dim5*dim6*sizeof(double);
	dataD.m[0] = dim1;
	dataD.m[1] = dim2;
	dataD.m[2] = dim3;
	dataD.m[3] = dim4;
	dataD.m[4] = dim5;
	dataD.m[5] = dim6;
	dataD.pointer = (char *)data;
	if(dim1 * dim2 * dim3 * dim4 * dim5 * dim6> 0)
		status =  replaceLastSlice(expIdx, cpoPath, path, (struct descriptor *)&dataD);
	else
		status = 0;
	return status;
}


 int mdsReplaceLastVect6DIntSlice(int expIdx, char *cpoPath, char *path, int *data, int dim1, int dim2,
    int dim3, int dim4, int dim5, int dim6)
{
	EMPTYXD(emptyXd);
	DESCRIPTOR_A_COEFF(dataD, sizeof(double), DTYPE_L, 0, 6, 0);
	int status;
	dataD.arsize = dim1*dim2*dim3*dim4*dim5*dim6*sizeof(double);
	dataD.m[0] = dim1;
	dataD.m[1] = dim2;
	dataD.m[2] = dim3;
	dataD.m[3] = dim4;
	dataD.m[4] = dim5;
	dataD.m[5] = dim6;
	dataD.pointer = (char *)data;
	if(dim1 * dim2 * dim3 * dim4 * dim5 * dim6> 0)
		status =  replaceLastSlice(expIdx, cpoPath, path, (struct descriptor *)&dataD);
	else
		status = 0;
	return status;
}


/////////////GET ROUTINES////////////////////////

/*
 int mdsGetDimension(int expIdx, char *cpoPath, char *path, int *numDims, int *dim1, int *dim2, int *dim3, int *dim4, int *dim5, int *dim6, int *dim7)
{
        EMPTYXD(xd);
        int status;
	ARRAY_BOUNDS(char, 7) *dataD;
        int i;

        *dim1 = *dim2 = *dim3 = *dim4 = *dim5 = *dim6 = *dim7 = 0;

	status = getData(expIdx, cpoPath, path, &xd, 1);
        if(status) return status;
        if(xd.pointer->class != CLASS_A)
        {
            *numDims = 0;
            MdsFree1Dx(&xd, 0);
            return 0;
        }
        dataD = (void *)xd.pointer;
        *numDims = dataD->dimct;
	if (dataD->dimct == 1)
	   *dim1 = dataD->arsize/dataD->length;
	else
           *dim1 = dataD->m[0];   // m array not declared if not a multidimensional array
        if(dataD->dimct > 1)
            *dim2 = dataD->m[1];
        if(dataD->dimct > 2)
            *dim3 = dataD->m[2];
	if(dataD->dimct > 3)
            *dim4 = dataD->m[3];
	if(dataD->dimct > 4)
            *dim5 = dataD->m[4];
	if(dataD->dimct > 5)
            *dim6 = dataD->m[5];
	if(dataD->dimct > 6)
            *dim7 = dataD->m[6];

        MdsFree1Dx(&xd, 0);
        return 0;
}
*/

/////////////////More efficient way of getting dimensions for timed data//////////////
static int isEmpty(int nid)
{
    int retLen, len = 0, retRecLen, recLen = 0, status;
    struct nci_itm nciList[] =
	{{sizeof(int), NciLENGTH, (char *)&len, &retLen},
	{sizeof(int), NciRLENGTH, (char *)&recLen, &retRecLen},
	{NciEND_OF_LIST, 0, 0, 0}};

    status = TreeGetNci(nid, nciList);
    return len == 0;

}
static int hasMembers(int nid)
{
    int retLen, nMembers = 0, status;
    struct nci_itm nciList[] =
	{{sizeof(int), NciNUMBER_OF_MEMBERS, (char *)&nMembers, &retLen},
	{NciEND_OF_LIST, 0, 0, 0}};

    status = TreeGetNci(nid, nciList);
    return nMembers > 0;

}




static int mdsGetLocalDimension(int expIdx, char *cpoPath, char *path, int *numDims, int *dim1, int *dim2, int *dim3, int *dim4, int *dim5, int *dim6, int *dim7)
{
    int lastDim, i, nextRow, dims[16], nid, status, numSegments, leftItems;
    char dtype, dimct;
    EMPTYXD(xd);
    ARRAY_COEFF(char, 16) *dataDPtr;
    lock("mdsGetLocalDimension");
    checkExpIndex(expIdx);
        nid = getNid(cpoPath, path);
/*	printf("nid for %s %s = %d\n",cpoPath, path,nid); */
    if(nid == -1)
    {
	unlock();
	return -1;
    }
    if(isEmpty(nid))
    {
	unlock();
 	return -1;
    }
//printf("NOT EMPTY\n");
    status = TreeGetNumSegments(nid, &numSegments);
    if(!(status & 1))
    {
	sprintf(errmsg, "Missing data for IDS: %s, field: %s - %s", cpoPath, path, MdsGetMsg(status));
	unlock();
	return -1;
    }
    if(numSegments > 0)
    {

//printf("GET LOCAL DIM TIMED\n");
	lastDim = 0;
	for(i = 0; i < numSegments; i++)
	{
	    status = TreeGetSegmentInfo(nid, i, &dtype, &dimct, dims, &nextRow);
	    if(i < numSegments - 1 && nextRow != dims[dimct - 1])
	    {
		printf("INTERNAL ERROR: Last Segment dimension is inconsistent!!!\n");
		unlock();
		return -1;
	    }
	    lastDim += nextRow;
	}
	dims[dimct - 1] = lastDim;
    }
    else
    {
	status = TreeGetRecord(nid, &xd);
	if(!xd.pointer)
	{
	    sprintf(errmsg, "Missing data for IDS: %s, field: %s - %s", cpoPath, path, MdsGetMsg(status));
	    unlock();
	    return -1;
	}
	dataDPtr = (void *)xd.pointer;
	if(dataDPtr->class != CLASS_A)
	{
	    printf("INTERNAL ERROR: getDataLocalNonDescr called for non array data!!!\n");
	    unlock();
	    return -1;
	}
	if(dataDPtr->dimct == 1) //Single dimension array
	{
	    dims[0] = dataDPtr->arsize/dataDPtr->length;
	    dimct = 1;
	}
	else
	{
	    for(i = 0; i < dataDPtr->dimct; i++)
		dims[i] = dataDPtr->m[i];
	    dimct = dataDPtr->dimct;
	}
	MdsFree1Dx(&xd, 0);
    }
    unlock();
    *numDims = dimct;
    *dim1 = dims[0];   /* m array not declared if not a multidimensional array */
    if(dimct > 1)
        *dim2 = dims[1];
    if(dimct > 2)
        *dim3 = dims[2];
    if(dimct > 3)
        *dim4 = dims[3];
    if(dimct > 4)
        *dim5 = dims[4];
    if(dimct > 5)
        *dim6 = dims[5];
    if(dimct > 6)
        *dim7 = dims[6];
    return 0;
}


/////////////////////////////////////////////////////////////////////////





 int mdsGetDimension(int expIdx, char *cpoPath, char *path, int *numDims, int *dim1, int *dim2, int *dim3, int *dim4, int *dim5, int *dim6, int *dim7)
{
        EMPTYXD(xd);
        int status;
	ARRAY_BOUNDS(char, 7) *dataD;
        int i;
	char dtype, class;
	int dims[16];
	int dimCt;
	int length;
	int64_t arsize;
	char *data;

/////////////Memory-Based dimension / existence info management///////////////
    int infoPresent;
    int exists, nDims;
    int cacheLevel = getCacheLevel(expIdx);
    if(cacheLevel > 0)
    {
    	infoPresent = getInfo(expIdx, cpoPath, path, &exists, &nDims, dims);
    	if(infoPresent)
    	{
//printf("DIM HIT exists: %d nDims: %d\n", exists, nDims);

	    if(!exists)
	    {
	    	nDims = 0;
            	*dim1 = *dim2 = *dim3 = *dim4 = *dim5 = *dim6 = *dim7 = 0;
	    	return -1;
	    }
	    *numDims = nDims;
	    *dim1 = dims[0];
	    *dim2 = dims[1];
	    *dim3 = dims[2];
	    *dim4 = dims[3];
	    *dim5 = dims[4];
	    *dim6 = dims[5];
	    *dim7 = dims[6];
	    return 0;
    	}
    }
/*printf("GET DIMENSION\n");
        *dim1 = *dim2 = *dim3 = *dim4 = *dim5 = *dim6 = *dim7 = 0;
//    	if(!isExpRemote(expIdx))
//	    return mdsGetLocalDimension(expIdx, cpoPath, path, numDims, dim1, dim2, dim3, dim4, dim5, dim6, dim7);
*/
//We arrive here if either cacleLevel == 0 or data not found in cache
    status = getDataNoDescr(expIdx, cpoPath, path, (void **)&data, dims,  &dimCt, &dtype, &class, &length,  &arsize, 1);

//////////////Memory-Based dimension / existence info management///////////////
    if(cacheLevel > 0)
    {
	if(!status)
	    putInfoWithData(expIdx, cpoPath, path, 1, dimCt, dims, length, dtype, arsize, data, 0);
	else
	    putInfo(expIdx, cpoPath, path, 0, 0, dims);
    }
    if(status) return status;
    if(class != CLASS_A)
    {
        *numDims = 0;
	free(data);
        return 0;
    }
    *numDims = dimCt;
    if (dimCt == 1)    /* modif FI*/
      {
	//MERGE
	    if(arsize > 0)   /* modif FI*/
	   	*dim1 = arsize/length;
	    else
		*dim1 = 0;
	    //MERGE
	    /*
	*dim1 = arsize/length;
	*/
      }
    else
        *dim1 = dims[0];   /* m array not declared if not a multidimensional array */
    if(dimCt > 1)
        *dim2 = dims[1];
    if(dimCt > 2)
        *dim3 = dims[2];
    if(dimCt > 3)
        *dim4 = dims[3];
    if(dimCt > 4)
        *dim5 = dims[4];
    if(dimCt > 5)
        *dim6 = dims[5];
    if(dimCt > 6)
        *dim7 = dims[6];

	/*printf("m0,m1,arsize,length in C : %d,%d,%d,%d\n",dataD->m[0],dataD->m[1],dataD->arsize,dataD->length);
        printf("dim1,dim2,dim3 in C : %d,%d,%d\n",*dim1,*dim2,*dim3);*/
    free(data);
    return 0;
}



 int mdsGetString(int expIdx, char *cpoPath, char *path, char **data)
{
	EMPTYXD(xd);
	int status;

	struct descriptor_a *dataD;
	status = getData(expIdx, cpoPath, path, &xd, 1);
	if(!status)
	{
		dataD = (void *)xd.pointer;
		if(dataD->class != CLASS_A || dataD->dtype != DTYPE_BU)
		{
			sprintf(errmsg, "Internal error: unexpected data type at IDS %s field %s", cpoPath, path);
			status = -1;
		}
		*data = malloc(dataD->arsize + 1);
		memcpy(*data, dataD->pointer, dataD->arsize);
		(*data)[dataD->arsize] = 0;
		MdsFree1Dx(&xd, 0);
	}
	return status;
}


 int mdsGetFloat(int expIdx, char *cpoPath, char *path, float *data)
{
	EMPTYXD(xd);
	int status;

	struct descriptor *dataD;
	status = getData(expIdx, cpoPath, path, &xd, 1);
	if(!status)
	{
		dataD = xd.pointer;
		if(dataD->class != CLASS_S || dataD->dtype != DTYPE_FLOAT)
		{
			sprintf(errmsg, "Internal error: unexpected data type at IDS %s field %s", cpoPath, path);
			status = -1;
		}
		*data = *(float *)dataD->pointer;
		MdsFree1Dx(&xd, 0);
	}
	return status;
}

 int mdsGetInt(int expIdx, char *cpoPath, char *path, int *data)
{
	EMPTYXD(xd);
	int status;

	struct descriptor *dataD;
	status = getData(expIdx, cpoPath, path, &xd, 1);
	if(!status)
	{
		dataD = xd.pointer;
		if(dataD->class != CLASS_S || dataD->dtype != DTYPE_L)
		{
			sprintf(errmsg, "Internal error: unexpected data type at IDS %s field %s", cpoPath, path);
			status = -1;
		}
		*data = *(int *)dataD->pointer;
		MdsFree1Dx(&xd, 0);
	}
	return status;
}

 int mdsGetDouble(int expIdx, char *cpoPath, char *path, double *data)
{
	EMPTYXD(xd);
	int status;

	struct descriptor *dataD;
	status = getData(expIdx, cpoPath, path, &xd, 1);
	if(!status)
	{
		dataD = xd.pointer;
		if(dataD->class != CLASS_S || (dataD->dtype != DTYPE_DOUBLE && dataD->dtype != DTYPE_FT))
		{
			sprintf(errmsg, "Internal error: unexpected data type at IDS %s field %s", cpoPath, path);
			status = -1;
		}
		*data = *(double *)dataD->pointer;
		MdsFree1Dx(&xd, 0);
	}
	return status;
}

 int mdsGetVect1DString(int expIdx, char *cpoPath, char *path, char  ***data, int *dim)
{
	EMPTYXD(xd);
	int status, nItems, i;
    	ARRAY_COEFF(char, 16) *dataDPtr;

        char **retData;
        struct descriptor **descrPtr;
	struct descriptor_a *dataD;
	status = getData(expIdx, cpoPath, path, &xd, 1);
	if(!status)
	{
		dataD = (struct descriptor_a *)xd.pointer;
		if(dataD->class == CLASS_A && dataD->dtype == DTYPE_BU)
		{
		//Convert to String array (reshape and change dtype)
		    dataDPtr = (void *)dataD;
		    dataDPtr->length = dataDPtr->m[0];
		    dataDPtr->m[0] = dataDPtr->arsize;
		    dataDPtr->m[1] = 0;
		    dataDPtr->dtype =DTYPE_T;
		}

		if(dataD->class != CLASS_A || dataD->dtype != DTYPE_T)
		{
			sprintf(errmsg, "Internal error: unexpected data type at IDS %s field %s.  Class= %d Type = %d\n", cpoPath, path,
                            dataD->class, dataD->dtype);
                        return -1;
		}
		if(dataD->length == 0)
		{
		    MdsFree1Dx(&xd, 0);
		    return -1;
		}
                nItems = dataD->arsize/dataD->length;
		retData = (char **)malloc(sizeof(char *) * nItems);
                for(i = 0; i < nItems; i++)
                {
                    retData[i] = malloc(dataD->length + 1);
                    memcpy(retData[i], &dataD->pointer[i * dataD->length], dataD->length);
                    retData[i][dataD->length] = 0;
                }
		*data = retData;
		*dim = nItems;
		MdsFree1Dx(&xd, 0);
	}
	return status;
}


/* int mdsGetVect1DInt(int expIdx, char *cpoPath, char *path, int **data, int *dim)
{
	EMPTYXD(xd);
	int status;

	struct descriptor_a *dataD;
	status = getData(expIdx, cpoPath, path, &xd, 1);
	if(!status)
	{
		dataD = (struct descriptor_a *)xd.pointer;
		if(dataD->class != CLASS_A || dataD->dtype != DTYPE_L)
		{
			sprintf(errmsg, "Internal error: unexpected data type at IDS %s field %s", cpoPath, path);
			status = -1;
		}
                else
                {
                    *data = (int *)malloc(dataD->arsize);
                    memcpy(*data, dataD->pointer, dataD->arsize);
                    *dim = dataD->arsize/dataD->length;
                }
		MdsFree1Dx(&xd, 0);
	}
	return status;
}
*/
 int mdsGetVect1DInt(int expIdx, char *cpoPath, char *path, int **data, int *dim)
{
	int status;
	char dtype, class;
	int dims[16];
	int dimCt;
	int length;
	int64_t arsize;


	struct descriptor_a *dataD;
	status = getDataNoDescr(expIdx, cpoPath, path, (void **)data, dims,  &dimCt, &dtype, &class, &length,  &arsize, 1);

	if(!status)
	{
		if(class != CLASS_A || dtype != DTYPE_L)
		{
			sprintf(errmsg, "Internal error: unexpected data type at IDS %s field %s", cpoPath, path);
			status = -1;
		}
                else
                {
                    *dim = dims[0];
                }
	}
	return status;
}



/*
 int mdsGetVect1DFloat(int expIdx, char *cpoPath, char *path, float **data, int *dim)
{
	EMPTYXD(xd);
	int status;

	struct descriptor_a *dataD;
	status = getData(expIdx, cpoPath, path, &xd, 1);
	if(!status)
	{
		dataD = (struct descriptor_a *)xd.pointer;
		if(dataD->class != CLASS_A || dataD->dtype != DTYPE_FLOAT)
		{
			sprintf(errmsg, "Internal error: unexpected data type at IDS %s field %s", cpoPath, path);
			status = -1;
		}
                else
                {
                    *data = (float *)malloc(dataD->arsize);
                    memcpy(*data, dataD->pointer, dataD->arsize);
                    *dim = dataD->arsize/dataD->length;
                }
		MdsFree1Dx(&xd, 0);
	}
	return status;
}
*/

 int mdsGetVect1DFloat(int expIdx, char *cpoPath, char *path, float **data, int *dim)
 {
	int status;
	char dtype, class;
	int dims[16];
	int dimCt;
	int length;
	int64_t arsize;

	status = getDataNoDescr(expIdx, cpoPath, path, (void **)data, dims,  &dimCt, &dtype, &class, &length,  &arsize, 1);

	if(!status)
	{
		if(class != CLASS_A || dtype != DTYPE_FLOAT)
		{
			sprintf(errmsg, "Internal error: unexpected data type at IDS %s field %s", cpoPath, path);
			status = -1;
		}
                else
                {
                    *dim = dims[0];
                }
	}
	return status;
}

/* int mdsGetVect1DDouble(int expIdx, char *cpoPath, char *path, double **data, int *dim)
{
	EMPTYXD(xd);
	int status;

	struct descriptor_a *dataD;
	status = getData(expIdx, cpoPath, path, &xd, 1);
	if(!status)
	{
		dataD = (struct descriptor_a *)xd.pointer;
		if(dataD->class != CLASS_A || dataD->dtype != DTYPE_DOUBLE)
		{

			sprintf(errmsg, "Internal error: unexpected data type at IDS %s field %s", cpoPath, path);
			status = -1;
		}
                else
                {
                    *data = (double *)malloc(dataD->arsize);
                    memcpy(*data, dataD->pointer, dataD->arsize);
                    *dim = dataD->arsize/dataD->length;
                }
		MdsFree1Dx(&xd, 0);
	}
	return status;
}
*/
 int mdsGetVect1DDouble(int expIdx, char *cpoPath, char *path, double **data, int *dim)
{
	int status;
	int i;
	char dtype, class;
	int dims[16];
	int dimCt;
	int length;
	int64_t arsize;


	struct descriptor_a *dataD;
	status = getDataNoDescr(expIdx, cpoPath, path, (void **)data, dims,  &dimCt, &dtype, &class, &length,  &arsize, 1);

        //printf("status of getDataNoDescr = %d\n",status);

	if(!status)
	{
		if(class != CLASS_A || (dtype != DTYPE_DOUBLE && dtype != DTYPE_FT) )
		{
		printf("Expected class: %d Class: %d Expected type: %d Type:%d\n", CLASS_A, class, DTYPE_DOUBLE,dtype);
			sprintf(errmsg, "Internal error: unexpected data type at IDS %s field %s", cpoPath, path);
			status = -1;
		}
                else
                {
                    *dim = dims[0];
                }
	}
	return status;
}


/* int mdsGetVect2DInt(int expIdx, char *cpoPath, char *path, int **data, int *dim1, int *dim2)
{
	EMPTYXD(xd);
	int status;

	ARRAY_BOUNDS(int, 2) *dataD;
	status = getData(expIdx, cpoPath, path, &xd, 1);
	if(!status)
	{
		dataD = (void *)xd.pointer;
		if(dataD->class != CLASS_A || dataD->dtype != DTYPE_L || dataD->dimct != 2)
		{
			sprintf(errmsg, "Internal error: unexpected data type at IDS %s field %s", cpoPath, path);
			status = -1;
		}
                else
                {
                    *data = (int *)malloc(dataD->arsize);
                    memcpy(*data, dataD->pointer, dataD->arsize);
                    *dim1 = dataD->m[0];
                    *dim2 = dataD->m[1];
                }
		MdsFree1Dx(&xd, 0);
	}
	return status;
}*/
 int mdsGetVect2DInt(int expIdx, char *cpoPath, char *path, int **data, int *dim1, int *dim2)
{
	int status;
	char dtype, class;
	int dims[16];
	int dimCt;
	int length;
	int64_t arsize;


	struct descriptor_a *dataD;
	status = getDataNoDescr(expIdx, cpoPath, path, (void **)data, dims,  &dimCt, &dtype, &class, &length,  &arsize, 1);

	if(!status)
	{
		if(class != CLASS_A || dtype != DTYPE_L)
		{
			sprintf(errmsg, "Internal error: unexpected data type at IDS %s field %s", cpoPath, path);
			status = -1;
		}
                else
                {
                    *dim1 = dims[0];
		    *dim2 = dims[1];
                }
	}
	return status;
}

/* int mdsGetVect2DFloat(int expIdx, char *cpoPath, char *path, float **data, int *dim1, int *dim2)
{
	EMPTYXD(xd);
	int status;

	ARRAY_BOUNDS(float, 2) *dataD;
	status = getData(expIdx, cpoPath, path, &xd, 1);
	if(!status)
	{
		dataD = (void *)xd.pointer;
		if(dataD->class != CLASS_A || dataD->dtype != DTYPE_FLOAT || dataD->dimct != 2)
		{
			sprintf(errmsg, "Internal error: unexpected data type at IDS %s field %s", cpoPath, path);
			status = -1;
		}
                else
                {
                    *data = (float *)malloc(dataD->arsize);
                    memcpy(*data, dataD->pointer, dataD->arsize);
                    *dim1 = dataD->m[0];
                    *dim2 = dataD->m[1];
                }
		MdsFree1Dx(&xd, 0);
	}
	return status;
}
*/
 int mdsGetVect2DFloat(int expIdx, char *cpoPath, char *path, float **data, int *dim1, int *dim2)
{
	int status;
	char dtype, class;
	int dims[16];
	int dimCt;
	int length;
	int64_t arsize;


	struct descriptor_a *dataD;
	status = getDataNoDescr(expIdx, cpoPath, path, (void **)data, dims,  &dimCt, &dtype, &class, &length,  &arsize, 1);

	if(!status)
	{
		if(class != CLASS_A || dtype != DTYPE_FLOAT)
		{
			sprintf(errmsg, "Internal error: unexpected data type at IDS %s field %s", cpoPath, path);
			status = -1;
		}
                else
                {
                    *dim1 = dims[0];
		    *dim2 = dims[1];
                }
	}
	return status;
}

/* int mdsGetVect2DDouble(int expIdx, char *cpoPath, char *path, double  **data, int *dim1, int *dim2)
{
	EMPTYXD(xd);
	int status;

	ARRAY_BOUNDS(double, 2) *dataD;
	status = getData(expIdx, cpoPath, path, &xd, 1);
	if(!status)
	{
		dataD = (void *)xd.pointer;
		if(dataD->class != CLASS_A || dataD->dtype != DTYPE_DOUBLE || dataD->dimct != 2)
		{
			sprintf(errmsg, "Internal error: unexpected data type at IDS %s field %s", cpoPath, path);
			status = -1;
		}
                else
                {
                    *data = (double *)malloc(dataD->arsize);
                    memcpy(*data, dataD->pointer, dataD->arsize);
                    *dim1 = dataD->m[0];
                    *dim2 = dataD->m[1];
                }
		MdsFree1Dx(&xd, 0);
	}
	return status;
}
*/
 int mdsGetVect2DDouble(int expIdx, char *cpoPath, char *path, double  **data, int *dim1, int *dim2)
{
	int status;
	char dtype, class;
	int dims[16];
	int dimCt;
	int length;
	int64_t arsize;


	struct descriptor_a *dataD;
	status = getDataNoDescr(expIdx, cpoPath, path, (void **)data, dims,  &dimCt, &dtype, &class, &length,  &arsize, 1);

	if(!status)
	{
		if(class != CLASS_A || (dtype != DTYPE_DOUBLE && dtype != DTYPE_FT) )
		{
			sprintf(errmsg, "Internal error: unexpected data type at IDS %s field %s", cpoPath, path);
			status = -1;
		}
                else
                {
                    *dim1 = dims[0];
		    *dim2 = dims[1];
                }
	}
	return status;
}

/* int mdsGetVect3DInt(int expIdx, char *cpoPath, char *path, int **data, int *dim1, int *dim2, int *dim3)
{
	EMPTYXD(xd);
	int status;

	ARRAY_BOUNDS(int, 3) *dataD;
	status = getData(expIdx, cpoPath, path, &xd, 1);
	if(!status)
	{
		dataD = (void *)xd.pointer;
		if(dataD->class != CLASS_A || dataD->dtype != DTYPE_L || dataD->dimct != 3)
		{
			sprintf(errmsg, "Internal error: unexpected data type at IDS %s field %s", cpoPath, path);
			status = -1;
		}
                else
                {
                    *data = (int *)malloc(dataD->arsize);
                    memcpy(*data, dataD->pointer, dataD->arsize);
                    *dim1 = dataD->m[0];
                    *dim2 = dataD->m[1];
                    *dim3 = dataD->m[2];
                }
		MdsFree1Dx(&xd, 0);
	}
	return status;
}
*/
 int mdsGetVect3DInt(int expIdx, char *cpoPath, char *path, int **data, int *dim1, int *dim2, int *dim3)
{
	int status;
	char dtype, class;
	int dims[16];
	int dimCt;
	int length;
	int64_t arsize;


	struct descriptor_a *dataD;
	status = getDataNoDescr(expIdx, cpoPath, path, (void **)data, dims,  &dimCt, &dtype, &class, &length,  &arsize, 1);

	if(!status)
	{
		if(class != CLASS_A || dtype != DTYPE_L)
		{
			sprintf(errmsg, "Internal error: unexpected data type at IDS %s field %s", cpoPath, path);
			status = -1;
		}
                else
                {
                    *dim1 = dims[0];
		    *dim2 = dims[1];
		    *dim3 = dims[2];
                }
	}
	return status;
}

/* int mdsGetVect3DFloat(int expIdx, char *cpoPath, char *path, float **data, int *dim1, int *dim2, int *dim3)
{
	EMPTYXD(xd);
	int status;

	ARRAY_BOUNDS(float, 3) *dataD;
	status = getData(expIdx, cpoPath, path, &xd, 1);
	if(!status)
	{
		dataD = (void *)xd.pointer;
		if(dataD->class != CLASS_A || dataD->dtype != DTYPE_FLOAT || dataD->dimct != 3)
		{
			sprintf(errmsg, "Internal error: unexpected data type at IDS %s field %s", cpoPath, path);
			status = -1;
		}
                else
                {
                    *data = (float *)malloc(dataD->arsize);
                    memcpy(*data, dataD->pointer, dataD->arsize);
                    *dim1 = dataD->m[0];
                    *dim2 = dataD->m[1];
                    *dim3 = dataD->m[2];
                }

		MdsFree1Dx(&xd, 0);
	}
	return status;
}
*/
 int mdsGetVect3DFloat(int expIdx, char *cpoPath, char *path, float **data, int *dim1, int *dim2, int *dim3)
{
	int status;
	char dtype, class;
	int dims[16];
	int dimCt;
	int length;
	int64_t arsize;


	struct descriptor_a *dataD;
	status = getDataNoDescr(expIdx, cpoPath, path, (void **)data, dims,  &dimCt, &dtype, &class, &length,  &arsize, 1);

	if(!status)
	{
		if(class != CLASS_A || dtype != DTYPE_FLOAT)
		{
			sprintf(errmsg, "Internal error: unexpected data type at IDS %s field %s", cpoPath, path);
			status = -1;
		}
                else
                {
                    *dim1 = dims[0];
		    *dim2 = dims[1];
		    *dim3 = dims[2];
                }
	}
	return status;
}

/* int mdsGetVect3DDouble(int expIdx, char *cpoPath, char *path, double  **data, int *dim1, int *dim2, int *dim3)
{
	EMPTYXD(xd);
	int status;

	ARRAY_BOUNDS(double, 3) *dataD;
	status = getData(expIdx, cpoPath, path, &xd, 1);
	if(!status)
	{
		dataD = (void *)xd.pointer;
		if(dataD->class != CLASS_A || dataD->dtype != DTYPE_DOUBLE || dataD->dimct != 3)
		{
			sprintf(errmsg, "Internal error: unexpected data type at IDS %s field %s", cpoPath, path);
			status = -1;
		}
                else
                {
                    *data = (double *)malloc(dataD->arsize);
                    memcpy(*data, dataD->pointer, dataD->arsize);
                    *dim1 = dataD->m[0];
                    *dim2 = dataD->m[1];
                    *dim3 = dataD->m[2];
                }
                MdsFree1Dx(&xd, 0);
	}
	return status;
}
*/
 int mdsGetVect3DDouble(int expIdx, char *cpoPath, char *path, double  **data, int *dim1, int *dim2, int *dim3)
{
	int status;
	char dtype, class;
	int dims[16];
	int dimCt;
	int length;
	int64_t arsize;


	struct descriptor_a *dataD;
	status = getDataNoDescr(expIdx, cpoPath, path, (void **)data, dims,  &dimCt, &dtype, &class, &length,  &arsize, 1);

	if(!status)
	{
		if(class != CLASS_A || (dtype != DTYPE_DOUBLE && dtype != DTYPE_FT) )
		{
			sprintf(errmsg, "Internal error: unexpected data type at IDS %s field %s", cpoPath, path);
			status = -1;
		}
                else
                {
                    *dim1 = dims[0];
		    *dim2 = dims[1];
		    *dim3 = dims[2];
                }
	}
	return status;
}

/////////////////
/* int mdsGetVect4DDouble(int expIdx, char *cpoPath, char *path, double  **data, int *dim1, int *dim2, int *dim3,
    int *dim4)
{
	EMPTYXD(xd);
	int status;

	ARRAY_BOUNDS(double, 4) *dataD;
	status = getData(expIdx, cpoPath, path, &xd, 1);
	if(!status)
	{
		dataD = (void *)xd.pointer;
		if(dataD->class != CLASS_A || dataD->dtype != DTYPE_DOUBLE || dataD->dimct != 4)
		{
			sprintf(errmsg, "Internal error: unexpected data type at IDS %s field %s", cpoPath, path);
			status = -1;
		}
                else
                {
                    *data = (double *)malloc(dataD->arsize);
                    memcpy(*data, dataD->pointer, dataD->arsize);
                    *dim1 = dataD->m[0];
                    *dim2 = dataD->m[1];
                    *dim3 = dataD->m[2];
                    *dim4 = dataD->m[3];
                }
		MdsFree1Dx(&xd, 0);
	}
	return status;
}
*/

 int mdsGetVect4DDouble(int expIdx, char *cpoPath, char *path, double  **data, int *dim1, int *dim2, int *dim3,
    int *dim4)
{
	int status;
	char dtype, class;
	int dims[16];
	int dimCt;
	int length;
	int64_t arsize;


	struct descriptor_a *dataD;
	status = getDataNoDescr(expIdx, cpoPath, path, (void **)data, dims,  &dimCt, &dtype, &class, &length,  &arsize, 1);

	if(!status)
	{
		if(class != CLASS_A || (dtype != DTYPE_DOUBLE && dtype != DTYPE_FT) )
		{
			sprintf(errmsg, "Internal error: unexpected data type at IDS %s field %s", cpoPath, path);
			status = -1;
		}
                else
                {
                    *dim1 = dims[0];
		    *dim2 = dims[1];
		    *dim3 = dims[2];
		    *dim4 = dims[3];
                }
	}
	return status;
}


/* int mdsGetVect4DFloat(int expIdx, char *cpoPath, char *path, float  **data, int *dim1, int *dim2, int *dim3,
int *dim4)
{
	EMPTYXD(xd);
	int status;

	ARRAY_BOUNDS(float, 4) *dataD;
	status = getData(expIdx, cpoPath, path, &xd, 1);
	if(!status)
	{
		dataD = (void *)xd.pointer;
		if(dataD->class != CLASS_A || dataD->dtype != DTYPE_FLOAT || dataD->dimct != 4)
		{
			sprintf(errmsg, "Internal error: unexpected data type at IDS %s field %s", cpoPath, path);
			status = -1;
		}
                else
                {
                    *data = (float *)malloc(dataD->arsize);
                    memcpy(*data, dataD->pointer, dataD->arsize);
                    *dim1 = dataD->m[0];
                    *dim2 = dataD->m[1];
                    *dim3 = dataD->m[2];
                    *dim4 = dataD->m[3];
                }
		MdsFree1Dx(&xd, 0);
	}
	return status;
}
*/
 int mdsGetVect4DFloat(int expIdx, char *cpoPath, char *path, float  **data, int *dim1, int *dim2, int *dim3,
int *dim4)
{
	int status;
	char dtype, class;
	int dims[16];
	int dimCt;
	int length;
	int64_t arsize;


	struct descriptor_a *dataD;
	status = getDataNoDescr(expIdx, cpoPath, path, (void **)data, dims,  &dimCt, &dtype, &class, &length,  &arsize, 1);

	if(!status)
	{
		if(class != CLASS_A || dtype != DTYPE_FLOAT)
		{
			sprintf(errmsg, "Internal error: unexpected data type at IDS %s field %s", cpoPath, path);
			status = -1;
		}
                else
                {
                    *dim1 = dims[0];
		    *dim2 = dims[1];
		    *dim3 = dims[2];
		    *dim4 = dims[3];
                }
	}
	return status;
}

/* int mdsGetVect4DInt(int expIdx, char *cpoPath, char *path, int  **data, int *dim1, int *dim2, int *dim3,
    int *dim4)
{
	EMPTYXD(xd);
	int status;

	ARRAY_BOUNDS(int, 4) *dataD;
	status = getData(expIdx, cpoPath, path, &xd, 1);
	if(!status)
	{
		dataD = (void *)xd.pointer;
		if(dataD->class != CLASS_A || dataD->dtype != DTYPE_L || dataD->dimct != 4)
		{
			sprintf(errmsg, "Internal error: unexpected data type at IDS %s field %s", cpoPath, path);
			status = -1;
		}
                else
                {
                    *data = (int *)malloc(dataD->arsize);
                    memcpy(*data, dataD->pointer, dataD->arsize);
                    *dim1 = dataD->m[0];
                    *dim2 = dataD->m[1];
                    *dim3 = dataD->m[2];
                    *dim4 = dataD->m[3];
                }
		MdsFree1Dx(&xd, 0);
	}
	return status;
}
*/
 int mdsGetVect4DInt(int expIdx, char *cpoPath, char *path, int  **data, int *dim1, int *dim2, int *dim3,
    int *dim4)
{
	int status;
	char dtype, class;
	int dims[16];
	int dimCt;
	int length;
	int64_t arsize;


	struct descriptor_a *dataD;
	status = getDataNoDescr(expIdx, cpoPath, path, (void **)data, dims,  &dimCt, &dtype, &class, &length,  &arsize, 1);

	if(!status)
	{
		if(class != CLASS_A || dtype != DTYPE_L)
		{
			sprintf(errmsg, "Internal error: unexpected data type at IDS %s field %s", cpoPath, path);
			status = -1;
		}
                else
                {
                    *dim1 = dims[0];
		    *dim2 = dims[1];
		    *dim3 = dims[2];
		    *dim4 = dims[3];
                }
	}
	return status;
}

/////////////
/* int mdsGetVect5DDouble(int expIdx, char *cpoPath, char *path, double  **data, int *dim1, int *dim2, int *dim3,
    int *dim4, int *dim5)
{
	EMPTYXD(xd);
	int status;

	ARRAY_BOUNDS(double, 5) *dataD;
	status = getData(expIdx, cpoPath, path, &xd, 1);
	if(!status)
	{
		dataD = (void *)xd.pointer;
		if(dataD->class != CLASS_A || dataD->dtype != DTYPE_DOUBLE || dataD->dimct != 5)
		{
			sprintf(errmsg, "Internal error: unexpected data type at IDS %s field %s", cpoPath, path);
			status = -1;
		}
                else
                {
                    *data = (double *)malloc(dataD->arsize);
                    memcpy(*data, dataD->pointer, dataD->arsize);
                    *dim1 = dataD->m[0];
                    *dim2 = dataD->m[1];
                    *dim3 = dataD->m[2];
                    *dim4 = dataD->m[3];
                    *dim5 = dataD->m[4];
                }
		MdsFree1Dx(&xd, 0);
	}
	return status;
}
*/

 int mdsGetVect5DDouble(int expIdx, char *cpoPath, char *path, double  **data, int *dim1, int *dim2, int *dim3,
    int *dim4, int *dim5)
{
	int status;
	char dtype, class;
	int dims[16];
	int dimCt;
	int length;
	int64_t arsize;


	struct descriptor_a *dataD;
	status = getDataNoDescr(expIdx, cpoPath, path, (void **)data, dims,  &dimCt, &dtype, &class, &length,  &arsize, 1);

	if(!status)
	{
		if(class != CLASS_A || (dtype != DTYPE_DOUBLE && dtype != DTYPE_FT) )
		{
			sprintf(errmsg, "Internal error: unexpected data type at IDS %s field %s", cpoPath, path);
			status = -1;
		}
                else
                {
                    *dim1 = dims[0];
		    *dim2 = dims[1];
		    *dim3 = dims[2];
		    *dim4 = dims[3];
		    *dim5 = dims[4];
                }
	}
	return status;
}



/* int mdsGetVect5DFloat(int expIdx, char *cpoPath, char *path, float  **data, int *dim1, int *dim2, int *dim3,
int *dim4, int *dim5)
{
	EMPTYXD(xd);
	int status;

	ARRAY_BOUNDS(float, 5) *dataD;
	status = getData(expIdx, cpoPath, path, &xd, 1);
	if(!status)
	{
		dataD = (void *)xd.pointer;
		if(dataD->class != CLASS_A || dataD->dtype != DTYPE_FLOAT || dataD->dimct != 5)
		{
			sprintf(errmsg, "Internal error: unexpected data type at IDS %s field %s", cpoPath, path);
			status = -1;
		}
                else
                {
                    *data = (float *)malloc(dataD->arsize);
                    memcpy(*data, dataD->pointer, dataD->arsize);
                    *dim1 = dataD->m[0];
                    *dim2 = dataD->m[1];
                    *dim3 = dataD->m[2];
                    *dim4 = dataD->m[3];
                    *dim5 = dataD->m[4];
                }
		MdsFree1Dx(&xd, 0);
	}
	return status;
}

*/

 int mdsGetVect5DFloat(int expIdx, char *cpoPath, char *path, float  **data, int *dim1, int *dim2, int *dim3,
int *dim4, int *dim5)
{
	int status;
	char dtype, class;
	int dims[16];
	int dimCt;
	int length;
	int64_t arsize;


	struct descriptor_a *dataD;
	status = getDataNoDescr(expIdx, cpoPath, path, (void **)data, dims,  &dimCt, &dtype, &class, &length,  &arsize, 1);

	if(!status)
	{
		if(class != CLASS_A || dtype != DTYPE_FLOAT)
		{
			sprintf(errmsg, "Internal error: unexpected data type at IDS %s field %s", cpoPath, path);
			status = -1;
		}
                else
                {
                    *dim1 = dims[0];
		    *dim2 = dims[1];
		    *dim3 = dims[2];
		    *dim4 = dims[3];
		    *dim5 = dims[4];
                }
	}
	return status;
}


/* int mdsGetVect5DInt(int expIdx, char *cpoPath, char *path, int  **data, int *dim1, int *dim2, int *dim3,
    int *dim4, int *dim5)
{
	EMPTYXD(xd);
	int status;

	ARRAY_BOUNDS(int, 5) *dataD;
	status = getData(expIdx, cpoPath, path, &xd, 1);
	if(!status)
	{
		dataD = (void *)xd.pointer;
		if(dataD->class != CLASS_A || dataD->dtype != DTYPE_L || dataD->dimct != 5)
		{
			sprintf(errmsg, "Internal error: unexpected data type at IDS %s field %s", cpoPath, path);
			status = -1;
		}
                else
                {
                    *data = (int *)malloc(dataD->arsize);
                    memcpy(*data, dataD->pointer, dataD->arsize);
                    *dim1 = dataD->m[0];
                    *dim2 = dataD->m[1];
                    *dim3 = dataD->m[2];
                    *dim4 = dataD->m[3];
                    *dim5 = dataD->m[4];
                }
		MdsFree1Dx(&xd, 0);
	}
	return status;
}
*/
 int mdsGetVect5DInt(int expIdx, char *cpoPath, char *path, int  **data, int *dim1, int *dim2, int *dim3,
    int *dim4, int *dim5)
{
	int status;
	char dtype, class;
	int dims[16];
	int dimCt;
	int length;
	int64_t arsize;


	struct descriptor_a *dataD;
	status = getDataNoDescr(expIdx, cpoPath, path, (void **)data, dims,  &dimCt, &dtype, &class, &length,  &arsize, 1);

	if(!status)
	{
		if(class != CLASS_A || dtype != DTYPE_L)
		{
			sprintf(errmsg, "Internal error: unexpected data type at IDS %s field %s", cpoPath, path);
			status = -1;
		}
                else
                {
                    *dim1 = dims[0];
		    *dim2 = dims[1];
		    *dim3 = dims[2];
		    *dim4 = dims[3];
		    *dim5 = dims[4];
                }
	}
	return status;
}

//
/* int mdsGetVect6DDouble(int expIdx, char *cpoPath, char *path, double  **data, int *dim1, int *dim2, int *dim3,
    int *dim4, int *dim5, int *dim6)
{
	EMPTYXD(xd);
	int status;

	ARRAY_BOUNDS(double, 6) *dataD;
	status = getData(expIdx, cpoPath, path, &xd, 1);
	if(!status)
	{
		dataD = (void *)xd.pointer;
		if(dataD->class != CLASS_A || dataD->dtype != DTYPE_DOUBLE || dataD->dimct != 6)
		{
			sprintf(errmsg, "Internal error: unexpected data type at IDS %s field %s", cpoPath, path);
			status = -1;
		}
                else
                {
                    *data = (double *)malloc(dataD->arsize);
                    memcpy(*data, dataD->pointer, dataD->arsize);
                    *dim1 = dataD->m[0];
                    *dim2 = dataD->m[1];
                    *dim3 = dataD->m[2];
                    *dim4 = dataD->m[3];
                    *dim5 = dataD->m[4];
                    *dim6 = dataD->m[5];
                }
		MdsFree1Dx(&xd, 0);
	}
	return status;
}
*/
 int mdsGetVect6DDouble(int expIdx, char *cpoPath, char *path, double  **data, int *dim1, int *dim2, int *dim3,
    int *dim4, int *dim5, int *dim6)
{
	int status;
	char dtype, class;
	int dims[16];
	int dimCt;
	int length;
	int64_t arsize;


	struct descriptor_a *dataD;
	status = getDataNoDescr(expIdx, cpoPath, path, (void **)data, dims,  &dimCt, &dtype, &class, &length,  &arsize, 1);

	if(!status)
	{
		if(class != CLASS_A || (dtype != DTYPE_DOUBLE && dtype != DTYPE_FT) )
		{
			sprintf(errmsg, "Internal error: unexpected data type at IDS %s field %s", cpoPath, path);
			status = -1;
		}
                else
                {
                    *dim1 = dims[0];
		    *dim2 = dims[1];
		    *dim3 = dims[2];
		    *dim4 = dims[3];
		    *dim5 = dims[4];
		    *dim6 = dims[5];
                }
	}
	return status;
}

/* int mdsGetVect6DFloat(int expIdx, char *cpoPath, char *path, float  **data, int *dim1, int *dim2, int *dim3,
int *dim4, int *dim5, int *dim6)
{
	EMPTYXD(xd);
	int status;

	ARRAY_BOUNDS(float, 6) *dataD;
	status = getData(expIdx, cpoPath, path, &xd, 1);
	if(!status)
	{
		dataD = (void *)xd.pointer;
		if(dataD->class != CLASS_A || dataD->dtype != DTYPE_FLOAT || dataD->dimct != 6)
		{
			sprintf(errmsg, "Internal error: unexpected data type at IDS %s field %s", cpoPath, path);
			status = -1;
		}
                else
                {
                    *data = (float *)malloc(dataD->arsize);
                    memcpy(*data, dataD->pointer, dataD->arsize);
                    *dim1 = dataD->m[0];
                    *dim2 = dataD->m[1];
                    *dim3 = dataD->m[2];
                    *dim4 = dataD->m[3];
                    *dim5 = dataD->m[4];
                    *dim6 = dataD->m[5];
                }
		MdsFree1Dx(&xd, 0);
	}
	return status;
}
*/
 int mdsGetVect6DFloat(int expIdx, char *cpoPath, char *path, float  **data, int *dim1, int *dim2, int *dim3,
int *dim4, int *dim5, int *dim6)
{
	int status;
	char dtype, class;
	int dims[16];
	int dimCt;
	int length;
	int64_t arsize;


	struct descriptor_a *dataD;
	status = getDataNoDescr(expIdx, cpoPath, path, (void **)data, dims,  &dimCt, &dtype, &class, &length,  &arsize, 1);

	if(!status)
	{
		if(class != CLASS_A || dtype != DTYPE_FLOAT)
		{
			sprintf(errmsg, "Internal error: unexpected data type at IDS %s field %s", cpoPath, path);
			status = -1;
		}
                else
                {
                    *dim1 = dims[0];
		    *dim2 = dims[1];
		    *dim3 = dims[2];
		    *dim4 = dims[3];
		    *dim5 = dims[4];
		    *dim6 = dims[5];
                }
	}
	return status;
}

/* int mdsGetVect6DInt(int expIdx, char *cpoPath, char *path, int  **data, int *dim1, int *dim2, int *dim3,
    int *dim4, int *dim5, int *dim6)
{
	EMPTYXD(xd);
	int status;

	ARRAY_BOUNDS(int, 6) *dataD;
	status = getData(expIdx, cpoPath, path, &xd, 1);
	if(!status)
	{
		dataD = (void *)xd.pointer;
		if(dataD->class != CLASS_A || dataD->dtype != DTYPE_L || dataD->dimct != 6)
		{
			sprintf(errmsg, "Internal error: unexpected data type at IDS %s field %s", cpoPath, path);
			status = -1;
		}
                else
                {
                    *data = (int *)malloc(dataD->arsize);
                    memcpy(*data, dataD->pointer, dataD->arsize);
                    *dim1 = dataD->m[0];
                    *dim2 = dataD->m[1];
                    *dim3 = dataD->m[2];
                    *dim4 = dataD->m[3];
                    *dim5 = dataD->m[4];
                    *dim6 = dataD->m[5];
                }
		MdsFree1Dx(&xd, 0);
	}
	return status;
}
*/
 int mdsGetVect6DInt(int expIdx, char *cpoPath, char *path, int  **data, int *dim1, int *dim2, int *dim3,
    int *dim4, int *dim5, int *dim6)
{
	int status;
	char dtype, class;
	int dims[16];
	int dimCt;
	int length;
	int64_t arsize;


	struct descriptor_a *dataD;
	status = getDataNoDescr(expIdx, cpoPath, path, (void **)data, dims,  &dimCt, &dtype, &class, &length,  &arsize, 1);

	if(!status)
	{
		if(class != CLASS_A || dtype != DTYPE_L)
		{
			sprintf(errmsg, "Internal error: unexpected data type at IDS %s field %s", cpoPath, path);
			status = -1;
		}
                else
                {
                    *dim1 = dims[0];
		    *dim2 = dims[1];
		    *dim3 = dims[2];
		    *dim4 = dims[3];
		    *dim5 = dims[4];
		    *dim6 = dims[5];
                }
	}
	return status;
}

////////////////////7D

/* int mdsGetVect7DDouble(int expIdx, char *cpoPath, char *path, double  **data, int *dim1, int *dim2, int *dim3,
    int *dim4, int *dim5, int *dim6, int *dim7)
{
	EMPTYXD(xd);
	int status;

	ARRAY_BOUNDS(double, 7) *dataD;
	status = getData(expIdx, cpoPath, path, &xd, 1);
	if(!status)
	{
		dataD = (void *)xd.pointer;
		if(dataD->class != CLASS_A || dataD->dtype != DTYPE_DOUBLE || dataD->dimct != 7)
		{
			sprintf(errmsg, "Internal error: unexpected data type at IDS %s field %s", cpoPath, path);
			status = -1;
		}
                else
                {
                    *data = (double *)malloc(dataD->arsize);
                    memcpy(*data, dataD->pointer, dataD->arsize);
                    *dim1 = dataD->m[0];
                    *dim2 = dataD->m[1];
                    *dim3 = dataD->m[2];
                    *dim4 = dataD->m[3];
                    *dim5 = dataD->m[4];
                    *dim6 = dataD->m[5];
                    *dim7 = dataD->m[6];
                }
		MdsFree1Dx(&xd, 0);
	}
	return status;
}
*/

 int mdsGetVect7DDouble(int expIdx, char *cpoPath, char *path, double  **data, int *dim1, int *dim2, int *dim3,
    int *dim4, int *dim5, int *dim6, int *dim7)
{
	int status;
	char dtype, class;
	int dims[16];
	int dimCt;
	int length;
	int64_t arsize;


	struct descriptor_a *dataD;
	status = getDataNoDescr(expIdx, cpoPath, path, (void **)data, dims,  &dimCt, &dtype, &class, &length,  &arsize, 1);

	if(!status)
	{
		if(class != CLASS_A || (dtype != DTYPE_DOUBLE && dtype != DTYPE_FT) )
		{
			sprintf(errmsg, "Internal error: unexpected data type at IDS %s field %s", cpoPath, path);
			status = -1;
		}
                else
                {
                    *dim1 = dims[0];
		    *dim2 = dims[1];
		    *dim3 = dims[2];
		    *dim4 = dims[3];
		    *dim5 = dims[4];
		    *dim6 = dims[5];
		    *dim7 = dims[6];
                }
	}
	return status;
}


/*

 int mdsGetVect7DFloat(int expIdx, char *cpoPath, char *path, float  **data, int *dim1, int *dim2, int *dim3,
int *dim4, int *dim5, int *dim6, int *dim7)
{
	EMPTYXD(xd);
	int status;

	ARRAY_BOUNDS(float, 7) *dataD;
	status = getData(expIdx, cpoPath, path, &xd, 1);
	if(!status)
	{
		dataD = (void *)xd.pointer;
		if(dataD->class != CLASS_A || dataD->dtype != DTYPE_FLOAT || dataD->dimct != 7)
		{
			sprintf(errmsg, "Internal error: unexpected data type at IDS %s field %s", cpoPath, path);
			status = -1;
		}
                else
                {
                    *data = (float *)malloc(dataD->arsize);
                    memcpy(*data, dataD->pointer, dataD->arsize);
                    *dim1 = dataD->m[0];
                    *dim2 = dataD->m[1];
                    *dim3 = dataD->m[2];
                    *dim4 = dataD->m[3];
                    *dim5 = dataD->m[4];
                    *dim6 = dataD->m[5];
                    *dim7 = dataD->m[6];
               }
		MdsFree1Dx(&xd, 0);
	}
	return status;
}
*/

 int mdsGetVect7DFloat(int expIdx, char *cpoPath, char *path, float  **data, int *dim1, int *dim2, int *dim3,
int *dim4, int *dim5, int *dim6, int *dim7)
{
	int status;
	char dtype, class;
	int dims[16];
	int dimCt;
	int length;
	int64_t arsize;


	struct descriptor_a *dataD;
	status = getDataNoDescr(expIdx, cpoPath, path, (void **)data, dims,  &dimCt, &dtype, &class, &length,  &arsize, 1);

	if(!status)
	{
		if(class != CLASS_A || dtype != DTYPE_FLOAT)
		{
			sprintf(errmsg, "Internal error: unexpected data type at IDS %s field %s", cpoPath, path);
			status = -1;
		}
                else
                {
                    *dim1 = dims[0];
		    *dim2 = dims[1];
		    *dim3 = dims[2];
		    *dim4 = dims[3];
		    *dim5 = dims[4];
		    *dim6 = dims[5];
		    *dim7 = dims[6];
                }
	}
	return status;
}

/*
 int mdsGetVect7DInt(int expIdx, char *cpoPath, char *path, int  **data, int *dim1, int *dim2, int *dim3,
    int *dim4, int *dim5, int *dim6, int *dim7)
{
	EMPTYXD(xd);
	int status;

	ARRAY_BOUNDS(int, 7) *dataD;
	status = getData(expIdx, cpoPath, path, &xd, 1);
	if(!status)
	{
		dataD = (void *)xd.pointer;
		if(dataD->class != CLASS_A || dataD->dtype != DTYPE_L || dataD->dimct != 7)
		{
			sprintf(errmsg, "Internal error: unexpected data type at IDS %s field %s", cpoPath, path);
			status = -1;
		}
                else
                {
                    *data = (int *)malloc(dataD->arsize);
                    memcpy(*data, dataD->pointer, dataD->arsize);
                    *dim1 = dataD->m[0];
                    *dim2 = dataD->m[1];
                    *dim3 = dataD->m[2];
                    *dim4 = dataD->m[3];
                    *dim5 = dataD->m[4];
                    *dim6 = dataD->m[5];
                    *dim7 = dataD->m[6];
                }
		MdsFree1Dx(&xd, 0);
	}
	return status;
}

*/
 int mdsGetVect7DInt(int expIdx, char *cpoPath, char *path, int  **data, int *dim1, int *dim2, int *dim3,
    int *dim4, int *dim5, int *dim6, int *dim7)
{
	int status;
	char dtype, class;
	int dims[16];
	int dimCt;
	int length;
	int64_t arsize;


	struct descriptor_a *dataD;
	status = getDataNoDescr(expIdx, cpoPath, path, (void **)data, dims,  &dimCt, &dtype, &class, &length,  &arsize, 1);

	if(!status)
	{
		if(class != CLASS_A || dtype != DTYPE_L)
		{
			sprintf(errmsg, "Internal error: unexpected data type at IDS %s field %s", cpoPath, path);
			status = -1;
		}
                else
                {
                    *dim1 = dims[0];
		    *dim2 = dims[1];
		    *dim3 = dims[2];
		    *dim4 = dims[3];
		    *dim5 = dims[4];
		    *dim6 = dims[5];
		    *dim7 = dims[6];
                }
	}
	return status;
}


////////////////

static int mdsendIdsGetLocal(int expIdx, char *path)
{
    return 0;
}


static int mdsbeginIdsGetLocal(int expIdx, char *path, int isTimed, int *retSamples)
{
	int status = 0;
	struct descriptor_a *arrayD;
	EMPTYXD(xd);
	EMPTYXD(timesXd);

        status = checkExpIndex(expIdx);
        if(status)
        {
            mdsendIdsGet(expIdx, path);
            return status;
        }
	if(!isTimed)
	{
	    *retSamples = 1;
	}
	else
	{
         //   printf("********* Inside beginIdsGetLocal: path=%s \n",path);
            status = getTimedData(expIdx, path, "time", 0., 0., &xd, &timesXd, 1); //  PROBLEM cpo:time is hardcoded here !
	    if(status || !xd.l_length || xd.pointer->class != CLASS_A)
	    {
                printf("Error in beginIdsGetLocal\n");
		sprintf(errmsg, "Internal error: Time not found for beginIdsGet at path %s: %s", path, MdsGetMsg(mdsStatus));
                mdsendIdsGet(expIdx, path);
                return -1;
	    }
	    arrayD = (struct descriptor_a *)xd.pointer;
	    *retSamples = arrayD->arsize/arrayD->length;
         //   printf("retSamples in BeingIDSGetLocal : %d \n",*retSamples);
	    MdsFree1Dx(&xd, 0);
	}


	return 0;
}





////////////////////GET SLICE ROUTINES ////////////////////////////////////
static int mdsendIdsGetSliceLocal(int expIdx, char *path)
{
    return 0;
}

static int mdsbeginIdsGetSliceLocal(int expIdx, char *path, double time)
{
	int status = 0;
	struct descriptor_a *arrayD;
	float *floatTime;
	double *doubleTime;
	int startIdx, totSamples;

	EMPTYXD(xd);
	EMPTYXD(timeXd);

        status = checkExpIndex(expIdx);
        if(status)
        {
            mdsendIdsGetSlice(expIdx, path);
            return status;
        }

	return status;

/*
	status = getTimedData(expIdx, path, "time", 0, 0, &xd, &timeXd, 1);
        if(status) //If failed
        {
            mdsendIdsGetSlice(expIdx, path);
            return status;
        }
	if(!xd.l_length || xd.pointer->class != CLASS_A || (xd.pointer->dtype != DTYPE_FLOAT && (xd.pointer->dtype != DTYPE_DOUBLE && xd.pointer->dtype != DTYPE_FT)))
	{
		sprintf(errmsg, "Internal error: Time not found for beginIdsGetSlice at path %s: %s", path, MdsGetMsg(mdsStatus));
                mdsendIdsGetSlice(expIdx, path);
		return -1;
	}
	arrayD = (struct descriptor_a *)xd.pointer;
	totSamples = arrayD->arsize/arrayD->length;
	if(arrayD->dtype == DTYPE_FLOAT)
	{
		floatTime = (float*)arrayD->pointer;
		for(startIdx = 0; startIdx < totSamples && floatTime[startIdx] <= time; startIdx++);
		if(startIdx == totSamples)
		{
			sliceIdx1 = sliceIdx2 = startIdx - 1;
			sliceTime1 = sliceTime2 = floatTime[startIdx - 1];
			//sprintf(errmsg, "Specified time range outside recorded times for IDS %s", path);
			//MdsFree1Dx(&xd, 0);
                        //mdsendIdsGetSlice(expIdx, path);
			//return -1;
		}
                else if(startIdx > 0)
		{
			sliceIdx1 = startIdx - 1;
			sliceTime1 = floatTime[sliceIdx1];
			sliceIdx2 = startIdx;
			sliceTime2 = floatTime[sliceIdx2];
		}
		else
		{
			sliceIdx1 = sliceIdx2 = startIdx;
			sliceTime1 = sliceTime2 = floatTime[sliceIdx2];
		}
	}
	else //double
	{
		doubleTime = (double*)arrayD->pointer;
		for(startIdx = 0; startIdx < totSamples && doubleTime[startIdx] <= time; startIdx++);
		if(startIdx == totSamples)
		{
			sliceIdx1 = sliceIdx2 = startIdx - 1;
			sliceTime1 = sliceTime2 = doubleTime[startIdx - 1];
			//sprintf(errmsg, "Specified time range outside recorded times for IDS %s", path);
			//MdsFree1Dx(&xd, 0);
                        //mdsendIdsGetSlice(expIdx, path);
			//return -1;
		}
                else if(startIdx > 0)
		{
			sliceIdx1 = startIdx - 1;
			sliceTime1 = doubleTime[sliceIdx1];
			sliceIdx2 = startIdx;
			sliceTime2 = doubleTime[sliceIdx2];
		}
		else
		{
			sliceIdx1 = sliceIdx2 = startIdx;
			sliceTime1 = sliceTime2 = doubleTime[sliceIdx2];
		}
	}
	MdsFree1Dx(&xd, 0);
	MdsFree1Dx(&timeXd, 0);
	return 0;

*/
}




 int mdsGetIntSlice(int expIdx, char *cpoPath, char *path, char *timeBasePath, int *data, double time, double *retTime, int interpolMode)
{
	struct descriptor_a *arrayD;
	int y1, y2;
	int status;
        double sliceTime1, sliceTime2;
	int sliceIdx1, sliceIdx2;
        int nTimes;


	EMPTYXD(xd);
	status = getDataAndSliceIdxs(expIdx, cpoPath, path, timeBasePath, &xd, time, &sliceIdx1, &sliceIdx2, &sliceTime1, &sliceTime2, &nTimes);
	if(status)
	{
		sprintf(errmsg, "Error reading IDS: %s, field: %s. %s", cpoPath, path, MdsGetMsg(mdsStatus));
		return -1;
	}
	if(status || !xd.l_length || xd.pointer->class != CLASS_A || xd.pointer->dtype != DTYPE_L)
	{
		sprintf(errmsg, "Error: not an int array at IDS:  %s, field %s", cpoPath, path);
		return -1;
	}
	arrayD = (struct descriptor_a *)xd.pointer;
	y1 = ((int *)arrayD->pointer)[sliceIdx1];
	y2 = ((int *)arrayD->pointer)[sliceIdx2];
	interpolMode = PREVIOUS_SAMPLE; //Force to previous sample for integers
	switch(interpolMode) {
		case INTERPOLATION:
			*data = (sliceTime1 == sliceTime2)?y1:y1 + ((double)y2 - (double)y1)*(time - sliceTime1)/(sliceTime2 - sliceTime1);
			*retTime = (sliceTime1 == sliceTime2)?sliceTime1:time;
			break;
		case CLOSEST_SAMPLE:
			if(time - sliceTime1 < sliceTime2 - time)
			{
				*data = y1;
				*retTime = sliceTime1;
			}
			else
			{
				*data = y2;
				*retTime = sliceTime2;
			}
			break;
		case PREVIOUS_SAMPLE:
			*data = y1;
			*retTime = sliceTime1;
			break;
	}
	MdsFree1Dx(&xd, 0);
	return 0;
}


 int mdsGetFloatSlice(int expIdx, char *cpoPath, char *path, char *timeBasePath, float *data, double time, double *retTime, int interpolMode)
{
	struct descriptor_a *arrayD;
	float y1, y2;
	int status;
        double sliceTime1, sliceTime2;
	int sliceIdx1, sliceIdx2;
        int nTimes;


	EMPTYXD(xd);
	status = getDataAndSliceIdxs(expIdx, cpoPath, path, timeBasePath, &xd, time, &sliceIdx1, &sliceIdx2, &sliceTime1, &sliceTime2, &nTimes);
	if(status)
	{
		sprintf(errmsg, "Error reading IDS: %s, field: %s. %s", cpoPath, path, MdsGetMsg(mdsStatus));
		return -1;
	}
	if(!xd.l_length || xd.pointer->class != CLASS_A || xd.pointer->dtype != DTYPE_FLOAT)
	{
		sprintf(errmsg, "Internal error: not a float array at IDS path %s, path %s", cpoPath, path);
		return -1;
	}
	arrayD = (struct descriptor_a *)xd.pointer;
	y1 = ((float *)arrayD->pointer)[sliceIdx1];
	y2 = ((float *)arrayD->pointer)[sliceIdx2];
        switch(interpolMode) {
                case INTERPOLATION:
                        *data = (sliceTime1 == sliceTime2)?y1:y1 + ((double)y2 - (double)y1)*(time - sliceTime1)/(sliceTime2 - sliceTime1);
                        *retTime = (sliceTime1 == sliceTime2)?sliceTime1:time;
                        break;
                case CLOSEST_SAMPLE:
                        if(time - sliceTime1 < sliceTime2 - time)
                        {
                                *data = y1;
                                *retTime = sliceTime1;
                        }
                        else
                        {
                                *data = y2;
                                *retTime = sliceTime2;
                        }
                        break;
                case PREVIOUS_SAMPLE:
                        *data = y1;
                        *retTime = sliceTime1;
                        break;
        }
	MdsFree1Dx(&xd, 0);
	return 0;
}


 int mdsGetDoubleSlice(int expIdx, char *cpoPath, char *path, char *timeBasePath, double *data, double time, double *retTime, int interpolMode)
{
	struct descriptor_a *arrayD;
	double y1, y2;
	int status;
        double sliceTime1, sliceTime2;
	int sliceIdx1, sliceIdx2;
        int nTimes;


	EMPTYXD(xd);
	status = getDataAndSliceIdxs(expIdx, cpoPath, path, timeBasePath, &xd, time, &sliceIdx1, &sliceIdx2, &sliceTime1, &sliceTime2, &nTimes);
	if(status)
	{
		sprintf(errmsg, "Error reading IDS: %s, field: %s. %s", cpoPath, path, MdsGetMsg(mdsStatus));
		return -1;
	}
	if(status || !xd.l_length || xd.pointer->class != CLASS_A || (xd.pointer->dtype != DTYPE_DOUBLE && xd.pointer->dtype != DTYPE_FT))
	{
		sprintf(errmsg, "Internal error: not a double array at IDS path %s, path %s", cpoPath, path);
		return -1;
	}
	arrayD = (struct descriptor_a *)xd.pointer;
	y1 = ((double *)arrayD->pointer)[sliceIdx1];
	y2 = ((double *)arrayD->pointer)[sliceIdx2];
        switch(interpolMode) {
                case INTERPOLATION:
                        *data = (sliceTime1 == sliceTime2)?y1:y1 + (y2 - y1)*(time - sliceTime1)/(sliceTime2 - sliceTime1);
                        *retTime = (sliceTime1 == sliceTime2)?sliceTime1:time;
                        break;
                case CLOSEST_SAMPLE:
                        if(time - sliceTime1 < sliceTime2 - time)
                        {
                                *data = y1;
                                *retTime = sliceTime1;
                        }
                        else
                        {
                                *data = y2;
                                *retTime = sliceTime2;
                        }
                        break;
                case PREVIOUS_SAMPLE:
                        *data = y1;
                        *retTime = sliceTime1;
                        break;
            }
	MdsFree1Dx(&xd, 0);
	return 0;
}

 int mdsGetVect1DIntSlice(int expIdx, char *cpoPath, char *path, char *timeBasePath, int **data, int *dim, double time, double *retTime, int interpolMode)
{
	ARRAY_BOUNDS(int, 2) *arrayD;
	int *y1, *y2;
	int status, i;
	int *retData;
	int nItems;
        double sliceTime1, sliceTime2;
	int sliceIdx1, sliceIdx2;
        int nTimes;


	EMPTYXD(xd);
	status = getDataAndSliceIdxs(expIdx, cpoPath, path, timeBasePath, &xd, time, &sliceIdx1, &sliceIdx2, &sliceTime1, &sliceTime2, &nTimes);
	if(status)
	{
		sprintf(errmsg, "Error reading IDS: %s, field: %s. %s", cpoPath, path, MdsGetMsg(mdsStatus));
		return -1;
	}
	if(status || !xd.l_length || xd.pointer->class != CLASS_A || xd.pointer->dtype != DTYPE_L)
	{
		sprintf(errmsg, "Internal error: not a int array at IDS path %s, path %s", cpoPath, path);
		return -1;
	}

	arrayD = (void *)xd.pointer;
	*dim = nItems = arrayD->m[0];

	y1 = &((int *)arrayD->pointer)[sliceIdx1 * nItems];
	y2 = &((int *)arrayD->pointer)[sliceIdx2 * nItems];
	retData = (int *)malloc(sizeof(int) * nItems);

	interpolMode = PREVIOUS_SAMPLE; //Force to previous sample for integers

	for(i = 0; i < nItems; i++)
	{
		switch(interpolMode) {
			case INTERPOLATION:
				retData[i] = (sliceTime1 == sliceTime2)?y1[i]:y1[i] + ((double)y2[i] - (double)y1[i])*(time - sliceTime1)/(sliceTime2 - sliceTime1);
				*retTime = (sliceTime1 == sliceTime2)?sliceTime1:time;
				break;
			case CLOSEST_SAMPLE:
				if(time - sliceTime1 < sliceTime2 - time)
				{
					retData[i] = y1[i];
					*retTime = sliceTime1;
				}
				else
				{
					retData[i] = y2[i];
					*retTime = sliceTime2;
				}
				break;
			case PREVIOUS_SAMPLE:
				retData[i] = y1[i];
				*retTime = sliceTime1;
				break;
		}
	}
	MdsFree1Dx(&xd, 0);
	*data = retData;
	return 0;
}



 int mdsGetVect1DFloatSlice(int expIdx, char *cpoPath, char *path, char *timeBasePath, float **data, int *dim, double time, double *retTime, int interpolMode)
{
	ARRAY_BOUNDS(int, 2) *arrayD;
	float *y1, *y2;
	int status, i;
	float *retData;
	int nItems;

	EMPTYXD(xd);
        double sliceTime1, sliceTime2;
	int sliceIdx1, sliceIdx2;
        int nTimes;

	status = getDataAndSliceIdxs(expIdx, cpoPath, path, timeBasePath, &xd, time, &sliceIdx1, &sliceIdx2, &sliceTime1, &sliceTime2, &nTimes);
	if(status)
	{
		sprintf(errmsg, "Error reading IDS: %s, field: %s. %s", cpoPath, path, MdsGetMsg(mdsStatus));
		return -1;
	}
	if(status || !xd.l_length || xd.pointer->class != CLASS_A || xd.pointer->dtype != DTYPE_FLOAT)
	{
		sprintf(errmsg, "Internal error: not a int array at IDS path %s, path %s", cpoPath, path);
		return -1;
	}

	arrayD = (void *)xd.pointer;
	*dim = nItems = arrayD->m[0];

	y1 = &((float *)arrayD->pointer)[sliceIdx1 * nItems];
	y2 = &((float *)arrayD->pointer)[sliceIdx2 * nItems];
	retData = (float *)malloc(sizeof(float) * nItems);

	for(i = 0; i < nItems; i++)
	{
		switch(interpolMode) {
			case INTERPOLATION:
				retData[i] = (sliceTime1 == sliceTime2)?y1[i]:y1[i] + ((double)y2[i] - (double)y1[i])*(time - sliceTime1)/(sliceTime2 - sliceTime1);
				*retTime = (sliceTime1 == sliceTime2)?sliceTime1:time;
				break;
			case CLOSEST_SAMPLE:
				if(time - sliceTime1 < sliceTime2 - time)
				{
					retData[i] = y1[i];
					*retTime = sliceTime1;
				}
				else
				{
					retData[i] = y2[i];
					*retTime = sliceTime2;
				}
				break;
			case PREVIOUS_SAMPLE:
				retData[i] = y1[i];
				*retTime = sliceTime1;
				break;
		}
	}
	MdsFree1Dx(&xd, 0);
	*data = retData;
	return 0;
}
 int mdsGetStringSlice(int expIdx, char *cpoPath, char *path, char *timeBasePath, char **data, double time, double *retTime, int interpolMode)
{
	ARRAY_BOUNDS(int, 2) *arrayD;
	char *y1, *y2;
	int status, i;
	char *retData;
	int nItems;

	EMPTYXD(xd);
        double sliceTime1, sliceTime2;
	int sliceIdx1, sliceIdx2;
        int nTimes;

	status = getDataAndSliceIdxs(expIdx, cpoPath, path, timeBasePath, &xd, time, &sliceIdx1, &sliceIdx2, &sliceTime1, &sliceTime2, &nTimes);

        if(status)
	{
		sprintf(errmsg, "Error reading IDS: %s, field: %s. %s", cpoPath, path, MdsGetMsg(mdsStatus));
		return -1;
	}
	if(status || !xd.l_length || xd.pointer->class != CLASS_A || xd.pointer->dtype != DTYPE_BU)
	{
		sprintf(errmsg, "Internal error: not a int array at IDS path %s, path %s", cpoPath, path);
		return -1;
	}

	arrayD = (void *)xd.pointer;
        nItems = arrayD->arsize/nTimes;

	y1 = &((char *)arrayD->pointer)[sliceIdx1 * nItems];
	y2 = &((char *)arrayD->pointer)[sliceIdx2 * nItems];
	retData = malloc(nItems+1);

	for(i = 0; i < nItems; i++)
	{
		switch(interpolMode) {
			case INTERPOLATION:
			case CLOSEST_SAMPLE:
				if(time - sliceTime1 < sliceTime2 - time)
				{
					retData[i] = y1[i];
					*retTime = sliceTime1;
				}
				else
				{
					retData[i] = y2[i];
					*retTime = sliceTime2;
				}
				break;
			case PREVIOUS_SAMPLE:
				retData[i] = y1[i];
				*retTime = sliceTime1;
				break;
		}
	}
        retData[nItems] = 0;
	MdsFree1Dx(&xd, 0);
	*data = retData;
	return 0;
}


 int mdsGetVect1DDoubleSlice(int expIdx, char *cpoPath, char *path, char *timeBasePath, double **data, int *dim, double time, double *retTime, int interpolMode)
{
	ARRAY_BOUNDS(int, 2) *arrayD;
	double *y1, *y2;
	int status, i;
	double *retData;
	int nItems;
        double sliceTime1, sliceTime2;
	int sliceIdx1, sliceIdx2;
        int nTimes;


	EMPTYXD(xd);
 	status = getDataAndSliceIdxs(expIdx, cpoPath, path, timeBasePath, &xd, time, &sliceIdx1, &sliceIdx2, &sliceTime1, &sliceTime2, &nTimes);

	//printf("sliceIdx1, sliceIdx2, sliceTime1, sliceTime2 = %d, %d, %lf, %lf\n", sliceIdx1, sliceIdx2, sliceTime1, sliceTime2);
	if(status)
	{
		sprintf(errmsg, "Error reading IDS: %s, field: %s. %s", cpoPath, path, MdsGetMsg(mdsStatus));
		return -1;
	}
	if(!xd.l_length || xd.pointer->class != CLASS_A || (xd.pointer->dtype != DTYPE_DOUBLE && xd.pointer->dtype != DTYPE_FT))
	{
		sprintf(errmsg, "Internal error: not a int array at IDS path %s, path %s", cpoPath, path);
		return -1;
	}

	arrayD = (void *)xd.pointer;
	*dim = nItems = arrayD->m[0];
	//printf("nitems: %d\n",nItems);

	y1 = &((double *)arrayD->pointer)[sliceIdx1 * nItems];
	y2 = &((double *)arrayD->pointer)[sliceIdx2 * nItems];
	retData = (double *)malloc(sizeof(double) * nItems);

	for(i = 0; i < nItems; i++)
	{
		switch(interpolMode) {
			case INTERPOLATION:
				//printf("Interpolation \n");
				retData[i] = (sliceTime1 == sliceTime2)?y1[i]:y1[i] + (y2[i] - y1[i])*(time - sliceTime1)/(sliceTime2 - sliceTime1);
				*retTime = (sliceTime1 == sliceTime2)?sliceTime1:time;
				break;
			case CLOSEST_SAMPLE:
				//printf("Closest_sample %lf, %lf\n", time - sliceTime1, sliceTime2 - time);
				if(time - sliceTime1 < sliceTime2 - time)
				{
					retData[i] = y1[i];
					*retTime = sliceTime1;
				}
				else
				{
					retData[i] = y2[i];
					*retTime = sliceTime2;
				}
				break;
			case PREVIOUS_SAMPLE:
				/* printf("Previous_sample \n");*/
				retData[i] = y1[i];
				*retTime = sliceTime1;
				break;
		}
	}
	MdsFree1Dx(&xd, 0);
	*data = retData;
	return 0;
}



 int mdsGetVect2DIntSlice(int expIdx, char *cpoPath, char *path, char *timeBasePath, int **data, int *dim1, int *dim2, double time, double *retTime, int interpolMode)
{
	ARRAY_BOUNDS(int, 2) *arrayD;
	int *y1, *y2;
	int status, i;
	int *retData;
	int nItems;
        double sliceTime1, sliceTime2;
	int sliceIdx1, sliceIdx2;
        int nTimes;


	EMPTYXD(xd);
	status = getDataAndSliceIdxs(expIdx, cpoPath, path, timeBasePath, &xd, time, &sliceIdx1, &sliceIdx2, &sliceTime1, &sliceTime2, &nTimes);
	if(status)
	{
		sprintf(errmsg, "Error reading IDS: %s, field: %s. %s", cpoPath, path, MdsGetMsg(mdsStatus));
		return -1;
	}
	if(!xd.l_length || xd.pointer->class != CLASS_A || xd.pointer->dtype != DTYPE_L)
	{
		sprintf(errmsg, "Internal error: not a int array at IDS path %s, path %s", cpoPath, path);
		return -1;
	}

	arrayD = (void *)xd.pointer;
	*dim1 = arrayD->m[0];
	*dim2 = arrayD->m[1];
	nItems = *dim1 * *dim2;


	y1 = &((int *)arrayD->pointer)[sliceIdx1 * nItems];
	y2 = &((int *)arrayD->pointer)[sliceIdx2 * nItems];
	retData = (int *)malloc(sizeof(int) * nItems);

	interpolMode = PREVIOUS_SAMPLE; //Force to previous sample for integers
	for(i = 0; i < nItems; i++)
	{
		switch(interpolMode) {
			case INTERPOLATION:
				retData[i] = (sliceTime1 == sliceTime2)?y1[i]:y1[i] + ((double)y2[i] - (double)y1[i])*(time - sliceTime1)/(sliceTime2 - sliceTime1);
				*retTime = (sliceTime1 == sliceTime2)?sliceTime1:time;
				break;
			case CLOSEST_SAMPLE:
				if(time - sliceTime1 < sliceTime2 - time)
				{
					retData[i] = y1[i];
					*retTime = sliceTime1;
				}
				else
				{
					retData[i] = y2[i];
					*retTime = sliceTime2;
				}
				break;
			case PREVIOUS_SAMPLE:
				retData[i] = y1[i];
				*retTime = sliceTime1;
				break;
		}
	}
	MdsFree1Dx(&xd, 0);
	*data = retData;
	return 0;
}

 int mdsGetVect2DFloatSlice(int expIdx, char *cpoPath, char *path, char *timeBasePath, float **data, int *dim1, int *dim2, double time, double *retTime, int interpolMode)
{
	ARRAY_BOUNDS(int, 2) *arrayD;
	float *y1, *y2;
	int status, i;
	float *retData;
	int nItems;
        double sliceTime1, sliceTime2;
	int sliceIdx1, sliceIdx2;
        int nTimes;


	EMPTYXD(xd);
	status = getDataAndSliceIdxs(expIdx, cpoPath, path, timeBasePath, &xd, time, &sliceIdx1, &sliceIdx2, &sliceTime1, &sliceTime2, &nTimes);
	if(status)
	{
		sprintf(errmsg, "Error reading IDS: %s, field: %s. %s", cpoPath, path, MdsGetMsg(status));
		return -1;
	}
	if(!xd.l_length || xd.pointer->class != CLASS_A || xd.pointer->dtype != DTYPE_FLOAT)
	{
		sprintf(errmsg, "Internal error: not a int array at IDS path %s, path %s", cpoPath, path);
		return -1;
	}

	arrayD = (void *)xd.pointer;
	*dim1 = arrayD->m[0];
	*dim2 = arrayD->m[1];
	nItems = *dim1 * *dim2;


	y1 = &((float *)arrayD->pointer)[sliceIdx1 * nItems];
	y2 = &((float *)arrayD->pointer)[sliceIdx2 * nItems];
	retData = (float *)malloc(sizeof(float) * nItems);

	for(i = 0; i < nItems; i++)
	{
		switch(interpolMode) {
			case INTERPOLATION:
				retData[i] = (sliceTime1 == sliceTime2)?y1[i]:y1[i] + ((double)y2[i] - (double)y1[i])*(time - sliceTime1)/(sliceTime2 - sliceTime1);
				*retTime = (sliceTime1 == sliceTime2)?sliceTime1:time;
				break;
			case CLOSEST_SAMPLE:
				if(time - sliceTime1 < sliceTime2 - time)
				{
					retData[i] = y1[i];
					*retTime = sliceTime1;
				}
				else
				{
					retData[i] = y2[i];
					*retTime = sliceTime2;
				}
				break;
			case PREVIOUS_SAMPLE:
				retData[i] = y1[i];
				*retTime = sliceTime1;
				break;
		}
	}
	MdsFree1Dx(&xd, 0);
	*data = retData;
	return 0;
}


 int mdsGetVect2DDoubleSlice(int expIdx, char *cpoPath, char *path, char *timeBasePath, double **data, int *dim1, int *dim2, double time, double *retTime, int interpolMode)
{
	ARRAY_BOUNDS(int, 2) *arrayD;
	double *y1, *y2;
	int status, i;
	double *retData;
        double sliceTime1, sliceTime2;
	int sliceIdx1, sliceIdx2;
	int nItems;
        int nTimes;


	EMPTYXD(xd);
	status = getDataAndSliceIdxs(expIdx, cpoPath, path, timeBasePath, &xd, time, &sliceIdx1, &sliceIdx2, &sliceTime1, &sliceTime2, &nTimes);
	if(status)
	{
		sprintf(errmsg, "Error reading IDS: %s, field: %s. %s", cpoPath, path, MdsGetMsg(mdsStatus));
		return -1;
	}
	if(!xd.l_length || xd.pointer->class != CLASS_A || (xd.pointer->dtype != DTYPE_DOUBLE && xd.pointer->dtype != DTYPE_FT))
	{
		sprintf(errmsg, "Internal error: not a int array at IDS path %s, path %s", cpoPath, path);
		return -1;
	}

	arrayD = (void *)xd.pointer;
	*dim1 = arrayD->m[0];
	*dim2 = arrayD->m[1];
	nItems = *dim1 * *dim2;


	y1 = &((double *)arrayD->pointer)[sliceIdx1 * nItems];
	y2 = &((double *)arrayD->pointer)[sliceIdx2 * nItems];
	retData = (double *)malloc(sizeof(double) * nItems);

	for(i = 0; i < nItems; i++)
	{
		switch(interpolMode) {
			case INTERPOLATION:
				retData[i] = (sliceTime1 == sliceTime2)?y1[i]:y1[i] + (y2[i] - y1[i])*(time - sliceTime1)/(sliceTime2 - sliceTime1);
				*retTime = (sliceTime1 == sliceTime2)?sliceTime1:time;
				break;
			case CLOSEST_SAMPLE:
				if(time - sliceTime1 < sliceTime2 - time)
				{
					retData[i] = y1[i];
					*retTime = sliceTime1;
				}
				else
				{
					retData[i] = y2[i];
					*retTime = sliceTime2;
				}
				break;
			case PREVIOUS_SAMPLE:
				retData[i] = y1[i];
				*retTime = sliceTime1;
				break;
		}
	}
	MdsFree1Dx(&xd, 0);
	*data = retData;
	return 0;
}

 int mdsGetVect3DIntSlice(int expIdx, char *cpoPath, char *path, char *timeBasePath, int **data, int *dim1, int *dim2, int *dim3, double time, double *retTime, int interpolMode)
{
	ARRAY_BOUNDS(int, 3) *arrayD;
	int *y1, *y2;
	int status, i;
	int *retData;
	int nItems;
        double sliceTime1, sliceTime2;
	int sliceIdx1, sliceIdx2;
        int nTimes;


	EMPTYXD(xd);
	status = getDataAndSliceIdxs(expIdx, cpoPath, path, timeBasePath, &xd, time, &sliceIdx1, &sliceIdx2, &sliceTime1, &sliceTime2, &nTimes);
	if(status)
	{
		sprintf(errmsg, "Error reading IDS: %s, field: %s. %s", cpoPath, path, MdsGetMsg(mdsStatus));
		return -1;
	}
	if(!xd.l_length || xd.pointer->class != CLASS_A || xd.pointer->dtype != DTYPE_L)
	{
		sprintf(errmsg, "Internal error: not a int array at IDS path %s, path %s", cpoPath, path);
		return -1;
	}

       	arrayD = (void *)xd.pointer;
	*dim1 = arrayD->m[0];
	*dim2 = arrayD->m[1];
	*dim3 = arrayD->m[2];
	nItems = *dim1 * *dim2 * *dim3;


	y1 = &((int *)arrayD->pointer)[sliceIdx1 * nItems];
	y2 = &((int *)arrayD->pointer)[sliceIdx2 * nItems];
	retData = (int *)malloc(sizeof(int) * nItems);

	interpolMode = PREVIOUS_SAMPLE; //Force to previous sample for integers
	for(i = 0; i < nItems; i++)
	{
		switch(interpolMode) {
			case INTERPOLATION:
				retData[i] = (sliceTime1 == sliceTime2)?y1[i]:y1[i] + ((double)y2[i] - (double)y1[i])*(time - sliceTime1)/(sliceTime2 - sliceTime1);
				*retTime = (sliceTime1 == sliceTime2)?sliceTime1:time;
				break;
			case CLOSEST_SAMPLE:
				if(time - sliceTime1 < sliceTime2 - time)
				{
					retData[i] = y1[i];
					*retTime = sliceTime1;
				}
				else
				{
					retData[i] = y2[i];
					*retTime = sliceTime2;
				}
				break;
			case PREVIOUS_SAMPLE:
				retData[i] = y1[i];
				*retTime = sliceTime1;
				break;
		}
	}
	MdsFree1Dx(&xd, 0);
	*data = retData;
	return 0;
}

 int mdsGetVect3DFloatSlice(int expIdx, char *cpoPath, char *path, char *timeBasePath, float **data, int *dim1, int *dim2, int *dim3, double time, double *retTime, int interpolMode)
{
	ARRAY_BOUNDS(int, 3) *arrayD;
	float *y1, *y2;
	int status, i;
	float *retData;
	int nItems;
        double sliceTime1, sliceTime2;
	int sliceIdx1, sliceIdx2;
        int nTimes;


	EMPTYXD(xd);
	status = getDataAndSliceIdxs(expIdx, cpoPath, path, timeBasePath, &xd, time, &sliceIdx1, &sliceIdx2, &sliceTime1, &sliceTime2, &nTimes);
	if(status)
	{
		sprintf(errmsg, "Error reading IDS: %s, field: %s. %s", cpoPath, path, MdsGetMsg(status));
		return -1;
	}
	if(!xd.l_length || xd.pointer->class != CLASS_A || xd.pointer->dtype != DTYPE_FLOAT)
	{
		sprintf(errmsg, "Internal error: not a int array at IDS path %s, path %s", cpoPath, path);
		return -1;
	}

	arrayD = (void *)xd.pointer;
	*dim1 = arrayD->m[0];
	*dim2 = arrayD->m[1];
	*dim3 = arrayD->m[2];
	nItems = *dim1 * *dim2 * *dim3;


	y1 = &((float *)arrayD->pointer)[sliceIdx1 * nItems];
	y2 = &((float *)arrayD->pointer)[sliceIdx2 * nItems];
	retData = (float *)malloc(sizeof(float) * nItems);

	for(i = 0; i < nItems; i++)
	{
		switch(interpolMode) {
			case INTERPOLATION:
				retData[i] = (sliceTime1 == sliceTime2)?y1[i]:y1[i] + ((double)y2[i] - (double)y1[i])*(time - sliceTime1)/(sliceTime2 - sliceTime1);
				*retTime = (sliceTime1 == sliceTime2)?sliceTime1:time;
				break;
			case CLOSEST_SAMPLE:
				if(time - sliceTime1 < sliceTime2 - time)
				{
					retData[i] = y1[i];
					*retTime = sliceTime1;
				}
				else
				{
					retData[i] = y2[i];
					*retTime = sliceTime2;
				}
				break;
			case PREVIOUS_SAMPLE:
				retData[i] = y1[i];
				*retTime = sliceTime1;
				break;
		}
	}
	MdsFree1Dx(&xd, 0);
	*data = retData;
	return 0;
}



 int mdsGetVect3DDoubleSlice(int expIdx, char *cpoPath, char *path, char *timeBasePath, double **data, int *dim1, int *dim2, int *dim3, double time, double *retTime, int interpolMode)
{
	ARRAY_BOUNDS(int, 3) *arrayD;
	double *y1, *y2;
	int status, i;
	double *retData;
	int nItems;
        double sliceTime1, sliceTime2;
	int sliceIdx1, sliceIdx2;
        int nTimes;


	EMPTYXD(xd);
	status = getDataAndSliceIdxs(expIdx, cpoPath, path, timeBasePath, &xd, time, &sliceIdx1, &sliceIdx2, &sliceTime1, &sliceTime2, &nTimes);
	if(status)
	{
		sprintf(errmsg, "Error reading IDS: %s, field: %s. %s", cpoPath, path, MdsGetMsg(mdsStatus));
		return -1;
	}
	if(!xd.l_length || xd.pointer->class != CLASS_A || (xd.pointer->dtype != DTYPE_DOUBLE && xd.pointer->dtype != DTYPE_FT))
	{
		sprintf(errmsg, "Internal error: not a int array at IDS path %s, path %s", cpoPath, path);
		return -1;
	}

	arrayD = (void *)xd.pointer;
	*dim1 = arrayD->m[0];
	*dim2 = arrayD->m[1];
	*dim3 = arrayD->m[2];
	nItems = *dim1 * *dim2 * *dim3;


	y1 = &((double *)arrayD->pointer)[sliceIdx1 * nItems];
	y2 = &((double *)arrayD->pointer)[sliceIdx2 * nItems];
	retData = (double *)malloc(sizeof(double) * nItems);

	for(i = 0; i < nItems; i++)
	{
		switch(interpolMode) {
			case INTERPOLATION:
				retData[i] = (sliceTime1 == sliceTime2)?y1[i]:y1[i] + (y2[i] - y1[i])*(time - sliceTime1)/(sliceTime2 - sliceTime1);
				*retTime = (sliceTime1 == sliceTime2)?sliceTime1:time;
				break;
			case CLOSEST_SAMPLE:
				if(time - sliceTime1 < sliceTime2 - time)
				{
					retData[i] = y1[i];
					*retTime = sliceTime1;
				}
				else
				{
					retData[i] = y2[i];
					*retTime = sliceTime2;
				}
				break;
			case PREVIOUS_SAMPLE:
				retData[i] = y1[i];
				*retTime = sliceTime1;
				break;
		}
	}
	MdsFree1Dx(&xd, 0);
	*data = retData;
	return 0;
}
//////////
 int mdsGetVect4DIntSlice(int expIdx, char *cpoPath, char *path, char *timeBasePath, int **data, int *dim1, int *dim2, int *dim3, int *dim4, double time, double *retTime, int interpolMode)
{
	ARRAY_BOUNDS(int, 4) *arrayD;
	int *y1, *y2;
	int status, i;
	int *retData;
	int nItems;
        double sliceTime1, sliceTime2;
	int sliceIdx1, sliceIdx2;
        int nTimes;


	EMPTYXD(xd);
	status = getDataAndSliceIdxs(expIdx, cpoPath, path, timeBasePath, &xd, time, &sliceIdx1, &sliceIdx2, &sliceTime1, &sliceTime2, &nTimes);
	if(status)
	{
		sprintf(errmsg, "Error reading IDS: %s, field: %s. %s", cpoPath, path, MdsGetMsg(mdsStatus));
		return -1;
	}
	if(!xd.l_length || xd.pointer->class != CLASS_A || xd.pointer->dtype != DTYPE_L)
	{
		sprintf(errmsg, "Internal error: not a int array at IDS path %s, path %s", cpoPath, path);
		return -1;
	}

       	arrayD = (void *)xd.pointer;
	*dim1 = arrayD->m[0];
	*dim2 = arrayD->m[1];
	*dim3 = arrayD->m[2];
	*dim4 = arrayD->m[3];
	nItems = *dim1 * *dim2 * *dim3 * *dim4;


	y1 = &((int *)arrayD->pointer)[sliceIdx1 * nItems];
	y2 = &((int *)arrayD->pointer)[sliceIdx2 * nItems];
	retData = (int *)malloc(sizeof(int) * nItems);

	interpolMode = PREVIOUS_SAMPLE; //Force to previous sample for integers
	for(i = 0; i < nItems; i++)
	{
		switch(interpolMode) {
			case INTERPOLATION:
				retData[i] = (sliceTime1 == sliceTime2)?y1[i]:y1[i] + ((double)y2[i] -(double)y1[i])*(time - sliceTime1)/(sliceTime2 - sliceTime1);
				*retTime = (sliceTime1 == sliceTime2)?sliceTime1:time;
				break;
			case CLOSEST_SAMPLE:
				if(time - sliceTime1 < sliceTime2 - time)
				{
					retData[i] = y1[i];
					*retTime = sliceTime1;
				}
				else
				{
					retData[i] = y2[i];
					*retTime = sliceTime2;
				}
				break;
			case PREVIOUS_SAMPLE:
				retData[i] = y1[i];
				*retTime = sliceTime1;
				break;
		}
	}
	MdsFree1Dx(&xd, 0);
	*data = retData;
	return 0;
}

 int mdsGetVect4DFloatSlice(int expIdx, char *cpoPath, char *path, char *timeBasePath, float **data, int *dim1, int *dim2,
    int *dim3, int *dim4, double time, double *retTime, int interpolMode)
{
	ARRAY_BOUNDS(int, 4) *arrayD;
	float *y1, *y2;
	int status, i;
	float *retData;
	int nItems;
        double sliceTime1, sliceTime2;
	int sliceIdx1, sliceIdx2;
        int nTimes;


	EMPTYXD(xd);
	status = getDataAndSliceIdxs(expIdx, cpoPath, path, timeBasePath, &xd, time, &sliceIdx1, &sliceIdx2, &sliceTime1, &sliceTime2, &nTimes);
	if(status)
	{
		sprintf(errmsg, "Error reading IDS: %s, field: %s. %s", cpoPath, path, MdsGetMsg(status));
		return -1;
	}
	if(!xd.l_length || xd.pointer->class != CLASS_A || xd.pointer->dtype != DTYPE_FLOAT)
	{
		sprintf(errmsg, "Internal error: not a int array at IDS path %s, path %s", cpoPath, path);
		return -1;
	}

	arrayD = (void *)xd.pointer;
	*dim1 = arrayD->m[0];
	*dim2 = arrayD->m[1];
	*dim3 = arrayD->m[2];
	*dim4 = arrayD->m[3];
	nItems = *dim1 * *dim2 * *dim3 * *dim4;


	y1 = &((float *)arrayD->pointer)[sliceIdx1 * nItems];
	y2 = &((float *)arrayD->pointer)[sliceIdx2 * nItems];
	retData = (float *)malloc(sizeof(float) * nItems);

	for(i = 0; i < nItems; i++)
	{
		switch(interpolMode) {
			case INTERPOLATION:
				retData[i] = (sliceTime1 == sliceTime2)?y1[i]:y1[i] + ((double)y2[i] - (double)y1[i])*(time - sliceTime1)/(sliceTime2 - sliceTime1);
				*retTime = (sliceTime1 == sliceTime2)?sliceTime1:time;
				break;
			case CLOSEST_SAMPLE:
				if(time - sliceTime1 < sliceTime2 - time)
				{
					retData[i] = y1[i];
					*retTime = sliceTime1;
				}
				else
				{
					retData[i] = y2[i];
					*retTime = sliceTime2;
				}
				break;
			case PREVIOUS_SAMPLE:
				retData[i] = y1[i];
				*retTime = sliceTime1;
				break;
		}
	}
	MdsFree1Dx(&xd, 0);
	*data = retData;
	return 0;
}



 int mdsGetVect4DDoubleSlice(int expIdx, char *cpoPath, char *path, char *timeBasePath, double **data, int *dim1, int *dim2,
int *dim3, int *dim4, double time, double *retTime, int interpolMode)
{
	ARRAY_BOUNDS(int, 4) *arrayD;
	double *y1, *y2;
	int status, i;
	double *retData;
	int nItems;
        double sliceTime1, sliceTime2;
	int sliceIdx1, sliceIdx2;
        int nTimes;


	EMPTYXD(xd);
	status = getDataAndSliceIdxs(expIdx, cpoPath, path, timeBasePath, &xd, time, &sliceIdx1, &sliceIdx2, &sliceTime1, &sliceTime2, &nTimes);
	if(status)
	{
		sprintf(errmsg, "Error reading IDS: %s, field: %s. %s", cpoPath, path, MdsGetMsg(mdsStatus));
		return -1;
	}
	if(!xd.l_length || xd.pointer->class != CLASS_A || (xd.pointer->dtype != DTYPE_DOUBLE && xd.pointer->dtype != DTYPE_FT))
	{
		sprintf(errmsg, "Internal error: not a int array at IDS path %s, path %s", cpoPath, path);
		return -1;
	}

	arrayD = (void *)xd.pointer;
	*dim1 = arrayD->m[0];
	*dim2 = arrayD->m[1];
	*dim3 = arrayD->m[2];
	*dim4 = arrayD->m[3];
	nItems = *dim1 * *dim2 * *dim3 * *dim4;


	y1 = &((double *)arrayD->pointer)[sliceIdx1 * nItems];
	y2 = &((double *)arrayD->pointer)[sliceIdx2 * nItems];
	retData = (double *)malloc(sizeof(double) * nItems);

	for(i = 0; i < nItems; i++)
	{
		switch(interpolMode) {
			case INTERPOLATION:
				retData[i] = (sliceTime1 == sliceTime2)?y1[i]:y1[i] + (y2[i] - y1[i])*(time - sliceTime1)/(sliceTime2 - sliceTime1);
				*retTime = (sliceTime1 == sliceTime2)?sliceTime1:time;
				break;
			case CLOSEST_SAMPLE:
				if(time - sliceTime1 < sliceTime2 - time)
				{
					retData[i] = y1[i];
					*retTime = sliceTime1;
				}
				else
				{
					retData[i] = y2[i];
					*retTime = sliceTime2;
				}
				break;
			case PREVIOUS_SAMPLE:
				retData[i] = y1[i];
				*retTime = sliceTime1;
				break;
		}
	}
	MdsFree1Dx(&xd, 0);
	*data = retData;
	return 0;
}
///////////
 int mdsGetVect5DIntSlice(int expIdx, char *cpoPath, char *path, char *timeBasePath, int **data, int *dim1, int *dim2, int *dim3,
    int *dim4, int *dim5, double time, double *retTime, int interpolMode)
{
	ARRAY_BOUNDS(int, 5) *arrayD;
	int *y1, *y2;
	int status, i;
	int *retData;
	int nItems;
        double sliceTime1, sliceTime2;
	int sliceIdx1, sliceIdx2;
        int nTimes;


	EMPTYXD(xd);
	status = getDataAndSliceIdxs(expIdx, cpoPath, path, timeBasePath, &xd, time, &sliceIdx1, &sliceIdx2, &sliceTime1, &sliceTime2, &nTimes);
	if(status)
	{
		sprintf(errmsg, "Error reading IDS: %s, field: %s. %s", cpoPath, path, MdsGetMsg(mdsStatus));
		return -1;
	}
	if(!xd.l_length || xd.pointer->class != CLASS_A || xd.pointer->dtype != DTYPE_L)
	{
		sprintf(errmsg, "Internal error: not a int array at IDS path %s, path %s", cpoPath, path);
		return -1;
	}

       	arrayD = (void *)xd.pointer;
	*dim1 = arrayD->m[0];
	*dim2 = arrayD->m[1];
	*dim3 = arrayD->m[2];
	*dim4 = arrayD->m[3];
	*dim5 = arrayD->m[4];
	nItems = *dim1 * *dim2 * *dim3 * *dim4 * *dim5;


	y1 = &((int *)arrayD->pointer)[sliceIdx1 * nItems];
	y2 = &((int *)arrayD->pointer)[sliceIdx2 * nItems];
	retData = (int *)malloc(sizeof(int) * nItems);

	interpolMode = PREVIOUS_SAMPLE; //Force to previous sample for integers
	for(i = 0; i < nItems; i++)
	{
		switch(interpolMode) {
			case INTERPOLATION:
				retData[i] = (sliceTime1 == sliceTime2)?y1[i]:y1[i] + ((double)y2[i] - (double)y1[i])*(time - sliceTime1)/(sliceTime2 - sliceTime1);
				*retTime = (sliceTime1 == sliceTime2)?sliceTime1:time;
				break;
			case CLOSEST_SAMPLE:
				if(time - sliceTime1 < sliceTime2 - time)
				{
					retData[i] = y1[i];
					*retTime = sliceTime1;
				}
				else
				{
					retData[i] = y2[i];
					*retTime = sliceTime2;
				}
				break;
			case PREVIOUS_SAMPLE:
				retData[i] = y1[i];
				*retTime = sliceTime1;
				break;
		}
	}
	MdsFree1Dx(&xd, 0);
	*data = retData;
	return 0;
}

 int mdsGetVect5DFloatSlice(int expIdx, char *cpoPath, char *path, char *timeBasePath, float **data, int *dim1, int *dim2,
    int *dim3, int *dim4, int *dim5, double time, double *retTime, int interpolMode)
{
	ARRAY_BOUNDS(int, 5) *arrayD;
	float *y1, *y2;
	int status, i;
	float *retData;
	int nItems;
        double sliceTime1, sliceTime2;
	int sliceIdx1, sliceIdx2;
        int nTimes;


	EMPTYXD(xd);
	status = getDataAndSliceIdxs(expIdx, cpoPath, path, timeBasePath, &xd, time, &sliceIdx1, &sliceIdx2, &sliceTime1, &sliceTime2, &nTimes);
	if(status)
	{
		sprintf(errmsg, "Error reading IDS: %s, field: %s. %s", cpoPath, path, MdsGetMsg(status));
		return -1;
	}
	if(!xd.l_length || xd.pointer->class != CLASS_A || xd.pointer->dtype != DTYPE_FLOAT)
	{
		sprintf(errmsg, "Internal error: not a int array at IDS path %s, path %s", cpoPath, path);
		return -1;
	}

	arrayD = (void *)xd.pointer;
	*dim1 = arrayD->m[0];
	*dim2 = arrayD->m[1];
	*dim3 = arrayD->m[2];
	*dim4 = arrayD->m[3];
	*dim5 = arrayD->m[4];
	nItems = *dim1 * *dim2 * *dim3 * *dim4 * *dim5;


	y1 = &((float *)arrayD->pointer)[sliceIdx1 * nItems];
	y2 = &((float *)arrayD->pointer)[sliceIdx2 * nItems];
	retData = (float *)malloc(sizeof(float) * nItems);

	for(i = 0; i < nItems; i++)
	{
		switch(interpolMode) {
			case INTERPOLATION:
				retData[i] = (sliceTime1 == sliceTime2)?y1[i]:y1[i] + ((double)y2[i] - (double)y1[i])*(time - sliceTime1)/(sliceTime2 - sliceTime1);
				*retTime = (sliceTime1 == sliceTime2)?sliceTime1:time;
				break;
			case CLOSEST_SAMPLE:
				if(time - sliceTime1 < sliceTime2 - time)
				{
					retData[i] = y1[i];
					*retTime = sliceTime1;
				}
				else
				{
					retData[i] = y2[i];
					*retTime = sliceTime2;
				}
				break;
			case PREVIOUS_SAMPLE:
				retData[i] = y1[i];
				*retTime = sliceTime1;
				break;
		}
	}
	MdsFree1Dx(&xd, 0);
	*data = retData;
	return 0;
}



 int mdsGetVect5DDoubleSlice(int expIdx, char *cpoPath, char *path, char *timeBasePath, double **data, int *dim1, int *dim2,
int *dim3, int *dim4, int *dim5, double time, double *retTime, int interpolMode)
{
	ARRAY_BOUNDS(int, 5) *arrayD;
	double *y1, *y2;
	int status, i;
	double *retData;
	int nItems;
        double sliceTime1, sliceTime2;
	int sliceIdx1, sliceIdx2;
        int nTimes;


	EMPTYXD(xd);
	status = getDataAndSliceIdxs(expIdx, cpoPath, path, timeBasePath, &xd, time, &sliceIdx1, &sliceIdx2, &sliceTime1, &sliceTime2, &nTimes);
	if(status)
	{
		sprintf(errmsg, "Error reading IDS: %s, field: %s. %s", cpoPath, path, MdsGetMsg(mdsStatus));
		return -1;
	}
	if(!xd.l_length || xd.pointer->class != CLASS_A || (xd.pointer->dtype != DTYPE_DOUBLE && xd.pointer->dtype != DTYPE_FT))
	{
		sprintf(errmsg, "Internal error: not a int array at IDS path %s, path %s", cpoPath, path);
		return -1;
	}

	arrayD = (void *)xd.pointer;
	*dim1 = arrayD->m[0];
	*dim2 = arrayD->m[1];
	*dim3 = arrayD->m[2];
	*dim4 = arrayD->m[3];
	*dim5 = arrayD->m[4];
	nItems = *dim1 * *dim2 * *dim3 * *dim4 * *dim5;


	y1 = &((double *)arrayD->pointer)[sliceIdx1 * nItems];
	y2 = &((double *)arrayD->pointer)[sliceIdx2 * nItems];
	retData = (double *)malloc(sizeof(double) * nItems);

	for(i = 0; i < nItems; i++)
	{
		switch(interpolMode) {
			case INTERPOLATION:
				retData[i] = (sliceTime1 == sliceTime2)?y1[i]:y1[i] + (y2[i] - y1[i])*(time - sliceTime1)/(sliceTime2 - sliceTime1);
				*retTime = (sliceTime1 == sliceTime2)?sliceTime1:time;
				break;
			case CLOSEST_SAMPLE:
				if(time - sliceTime1 < sliceTime2 - time)
				{
					retData[i] = y1[i];
					*retTime = sliceTime1;
				}
				else
				{
					retData[i] = y2[i];
					*retTime = sliceTime2;
				}
				break;
			case PREVIOUS_SAMPLE:
				retData[i] = y1[i];
				*retTime = sliceTime1;
				break;
		}
	}
	MdsFree1Dx(&xd, 0);
	*data = retData;
	return 0;
}
//////////6D Slice
 int mdsGetVect6DIntSlice(int expIdx, char *cpoPath, char *path, char *timeBasePath, int **data, int *dim1, int *dim2, int *dim3,
    int *dim4, int *dim5, int *dim6, double time, double *retTime, int interpolMode)
{
	ARRAY_BOUNDS(int, 6) *arrayD;
	int *y1, *y2;
	int status, i;
	int *retData;
	int nItems;
        double sliceTime1, sliceTime2;
	int sliceIdx1, sliceIdx2;
        int nTimes;


	EMPTYXD(xd);
	status = getDataAndSliceIdxs(expIdx, cpoPath, path, timeBasePath, &xd, time, &sliceIdx1, &sliceIdx2, &sliceTime1, &sliceTime2, &nTimes);
	if(status)
	{
		sprintf(errmsg, "Error reading IDS: %s, field: %s. %s", cpoPath, path, MdsGetMsg(mdsStatus));
		return -1;
	}
	if(!xd.l_length || xd.pointer->class != CLASS_A || xd.pointer->dtype != DTYPE_L)
	{
		sprintf(errmsg, "Internal error: not a int array at IDS path %s, path %s", cpoPath, path);
		return -1;
	}

       	arrayD = (void *)xd.pointer;
	*dim1 = arrayD->m[0];
	*dim2 = arrayD->m[1];
	*dim3 = arrayD->m[2];
	*dim4 = arrayD->m[3];
	*dim5 = arrayD->m[4];
	*dim6 = arrayD->m[5];
	nItems = *dim1 * *dim2 * *dim3 * *dim4 * *dim5 * *dim6;


	y1 = &((int *)arrayD->pointer)[sliceIdx1 * nItems];
	y2 = &((int *)arrayD->pointer)[sliceIdx2 * nItems];
	retData = (int *)malloc(sizeof(int) * nItems);

	interpolMode = PREVIOUS_SAMPLE; //Force to previous sample for integers
	for(i = 0; i < nItems; i++)
	{
		switch(interpolMode) {
			case INTERPOLATION:
				retData[i] = (sliceTime1 == sliceTime2)?y1[i]:y1[i] + ((double)y2[i] - (double)y1[i])*(time - sliceTime1)/(sliceTime2 - sliceTime1);
				*retTime = (sliceTime1 == sliceTime2)?sliceTime1:time;
				break;
			case CLOSEST_SAMPLE:
				if(time - sliceTime1 < sliceTime2 - time)
				{
					retData[i] = y1[i];
					*retTime = sliceTime1;
				}
				else
				{
					retData[i] = y2[i];
					*retTime = sliceTime2;
				}
				break;
			case PREVIOUS_SAMPLE:
				retData[i] = y1[i];
				*retTime = sliceTime1;
				break;
		}
	}
	MdsFree1Dx(&xd, 0);
	*data = retData;
	return 0;
}

 int mdsGetVect6DFloatSlice(int expIdx, char *cpoPath, char *path, char *timeBasePath, float **data, int *dim1, int *dim2,
    int *dim3, int *dim4, int *dim5, int *dim6, double time, double *retTime, int interpolMode)
{
	ARRAY_BOUNDS(int, 6) *arrayD;
	float *y1, *y2;
	int status, i;
	float *retData;
	int nItems;
        double sliceTime1, sliceTime2;
	int sliceIdx1, sliceIdx2;
        int nTimes;


	EMPTYXD(xd);
	status = getDataAndSliceIdxs(expIdx, cpoPath, path, timeBasePath, &xd, time, &sliceIdx1, &sliceIdx2, &sliceTime1, &sliceTime2, &nTimes);
	if(status)
	{
		sprintf(errmsg, "Error reading IDS: %s, field: %s. %s", cpoPath, path, MdsGetMsg(status));
		return -1;
	}
	if(!xd.l_length || xd.pointer->class != CLASS_A || xd.pointer->dtype != DTYPE_FLOAT)
	{
		sprintf(errmsg, "Internal error: not a int array at IDS path %s, path %s", cpoPath, path);
		return -1;
	}

	arrayD = (void *)xd.pointer;
	*dim1 = arrayD->m[0];
	*dim2 = arrayD->m[1];
	*dim3 = arrayD->m[2];
	*dim4 = arrayD->m[3];
	*dim5 = arrayD->m[4];
	*dim6 = arrayD->m[5];
	nItems = *dim1 * *dim2 * *dim3 * *dim4 * *dim5 * *dim6;


	y1 = &((float *)arrayD->pointer)[sliceIdx1 * nItems];
	y2 = &((float *)arrayD->pointer)[sliceIdx2 * nItems];
	retData = (float *)malloc(sizeof(float) * nItems);

	for(i = 0; i < nItems; i++)
	{
		switch(interpolMode) {
			case INTERPOLATION:
				retData[i] = (sliceTime1 == sliceTime2)?y1[i]:y1[i] + ((double)y2[i] - (double)y1[i])*(time - sliceTime1)/(sliceTime2 - sliceTime1);
				*retTime = (sliceTime1 == sliceTime2)?sliceTime1:time;
				break;
			case CLOSEST_SAMPLE:
				if(time - sliceTime1 < sliceTime2 - time)
				{
					retData[i] = y1[i];
					*retTime = sliceTime1;
				}
				else
				{
					retData[i] = y2[i];
					*retTime = sliceTime2;
				}
				break;
			case PREVIOUS_SAMPLE:
				retData[i] = y1[i];
				*retTime = sliceTime1;
				break;
		}
	}
	MdsFree1Dx(&xd, 0);
	*data = retData;
	return 0;
}



 int mdsGetVect6DDoubleSlice(int expIdx, char *cpoPath, char *path, char *timeBasePath, double **data, int *dim1, int *dim2,
int *dim3, int *dim4, int *dim5, int *dim6, double time, double *retTime, int interpolMode)
{
	ARRAY_BOUNDS(int, 6) *arrayD;
	double *y1, *y2;
	int status, i;
	double *retData;
	int nItems;
        double sliceTime1, sliceTime2;
	int sliceIdx1, sliceIdx2;
        int nTimes;


	EMPTYXD(xd);
	status = getDataAndSliceIdxs(expIdx, cpoPath, path, timeBasePath, &xd, time, &sliceIdx1, &sliceIdx2, &sliceTime1, &sliceTime2, &nTimes);
	if(status)
	{
		sprintf(errmsg, "Error reading IDS: %s, field: %s. %s", cpoPath, path, MdsGetMsg(mdsStatus));
		return -1;
	}
	if(!xd.l_length || xd.pointer->class != CLASS_A || (xd.pointer->dtype != DTYPE_DOUBLE && xd.pointer->dtype != DTYPE_FT))
	{
		sprintf(errmsg, "Internal error: not a int array at IDS path %s, path %s", cpoPath, path);
		return -1;
	}

	arrayD = (void *)xd.pointer;
	*dim1 = arrayD->m[0];
	*dim2 = arrayD->m[1];
	*dim3 = arrayD->m[2];
	*dim4 = arrayD->m[3];
	*dim5 = arrayD->m[4];
	*dim6 = arrayD->m[5];
	nItems = *dim1 * *dim2 * *dim3 * *dim4 * *dim5 * *dim6;


	y1 = &((double *)arrayD->pointer)[sliceIdx1 * nItems];
	y2 = &((double *)arrayD->pointer)[sliceIdx2 * nItems];
	retData = (double *)malloc(sizeof(double) * nItems);

	for(i = 0; i < nItems; i++)
	{
		switch(interpolMode) {
			case INTERPOLATION:
				retData[i] = (sliceTime1 == sliceTime2)?y1[i]:y1[i] + ((double)y2[i] - (double)y1[i])*(time - sliceTime1)/(sliceTime2 - sliceTime1);
				*retTime = (sliceTime1 == sliceTime2)?sliceTime1:time;
				break;
			case CLOSEST_SAMPLE:
				if(time - sliceTime1 < sliceTime2 - time)
				{
					retData[i] = y1[i];
					*retTime = sliceTime1;
				}
				else
				{
					retData[i] = y2[i];
					*retTime = sliceTime2;
				}
				break;
			case PREVIOUS_SAMPLE:
				retData[i] = y1[i];
				*retTime = sliceTime1;
				break;
		}
	}
	MdsFree1Dx(&xd, 0);
	*data = retData;
	return 0;
}



//Get Unique run number based on experiment model run_numbers
 int getUniqueRun(int shot)
{
    int status, i;
    int shotNid, runNid, numItems;
    struct descriptor_a *shotD, *runD;
    int *runs, *shots, *newRuns, *newShots;
    DESCRIPTOR_A(newRunD, sizeof(int), DTYPE_L, 0, 0);
    DESCRIPTOR_A(newShotD, sizeof(int), DTYPE_L, 0, 0);
    EMPTYXD(shotXd);
    EMPTYXD(runXd);
    int retRun;

    status = TreeOpenEdit("runs", -1);
    if(!(status & 1))
    {
        sprintf(errmsg, "Error opening support experiment model runs");
        return 0;
    }
    status = TreeFindNode("\\shots", &shotNid);
    if(!(status & 1))
    {
        sprintf(errmsg, "Error Getting shots nid in getUniqueRun");
        return 0;
    }
    status = TreeFindNode("\\runs", &runNid);
    if(!(status & 1))
    {
        sprintf(errmsg, "Error Getting shots nid in getUniqueRun");
        return 0;
    }
    status = TreeGetRecord(shotNid, &shotXd);
    if((status & 1)&& shotXd.l_length > 0)
    {
        shotD = (struct descriptor_a *)shotXd.pointer;
        if(shotD->class != CLASS_A || shotD->dtype != DTYPE_L)
        {
            sprintf(errmsg, "Internal error in getUniqueRun: shots not an integer array");
            return 0;
        }
        shots = (int *)shotD->pointer;
        numItems = shotD->arsize/shotD->length;
    }
    else
        numItems = 0;

    status = TreeGetRecord(runNid, &runXd);
    if((status & 1)&& runXd.l_length > 0)
    {
        runD = (struct descriptor_a *)runXd.pointer;
        if(runD->class != CLASS_A || runD->dtype != DTYPE_L)
        {
            sprintf(errmsg, "Internal error in getUniqueRun: runs not an integer array");
            return 0;
        }
        runs = (int *)runD->pointer;
        if(runD->arsize/runD->length < numItems)
            numItems = runD->arsize/runD->length;
    }
    else
        numItems = 0;


    for(i = 0; i < numItems; i++)
    {
        if(shots[i] == shot)
        {
            runs[i]++;
            if(runs[i] == 0) runs[i]++;
            retRun = runs[i];
            break;
        }
    }
    if(i < numItems)//shot found, no need to add a new shot
    {
        TreePutRecord(shotNid, (struct descriptor *)shotD, 0);
        TreePutRecord(runNid, (struct descriptor *)runD, 0);
        MdsFree1Dx(&shotXd, 0);
        MdsFree1Dx(&runXd, 0);
    }
    else//Need to append the new shot
    {
        newRuns = (int *)malloc((numItems + 1)*sizeof(int));
        memcpy(newRuns, runs, numItems * sizeof(int));
        newShots = (int *)malloc((numItems + 1)*sizeof(int));
        memcpy(newShots, shots, numItems * sizeof(int));
        newShots[numItems] = shot;
        newRuns[numItems] = 1;
        newRunD.arsize = (numItems + 1)*sizeof(int);
        newRunD.pointer = (char *)newRuns;
        newShotD.arsize = (numItems + 1)*sizeof(int);
        newShotD.pointer = (char *)newShots;
        TreePutRecord(shotNid, (struct descriptor *)&newShotD, 0);
        TreePutRecord(runNid, (struct descriptor *)&newRunD, 0);
        MdsFree1Dx(&shotXd, 0);
        MdsFree1Dx(&runXd, 0);
        free((char *)newShots);
        free((char *)newRuns);
        retRun = 1;
    }
    TreeWriteTree("runs", -1);
    TreeClose("runs", -1);
    return retRun;
}


///////////Remote Data Access Support
static int isAnotherInstance(char *path)
{
//Check whether the first path component is a number. In this case it referes to another IDS instance
    char *currPtr;
    int isNumeric;
    char *firstPart = malloc(strlen(path) + 1);
    if(path[0] == '.' || path[0] == ':')
        strcpy(firstPart, &path[1]);
    else
        strcpy(firstPart, path);
    currPtr = firstPart;
    while(*currPtr && *currPtr != '.' && *currPtr != ':')
    	currPtr++;
    *currPtr = 0;
    isNumeric = 1;
    for(currPtr = firstPart; *currPtr; currPtr++)
    {
    	if(*currPtr < '0' || *currPtr > '9')
	    isNumeric = 0;
    }
    free(firstPart);
    return isNumeric;
}

/*
static sem_t *wildSem;
static void lockWild()
{
    char name[512];
    if((wildSem = sem_open("UAL_REMOTE_SEM", O_CREAT, 0666, 1)) == SEM_FAILED)
    {
        printf("getMdsCpoFields: CANNOT CREATE SEMAPHORE\n");
	return;
    }
    if(sem_wait(wildSem) == -1)
    {
        printf("getMdsCpoFields: CANNOT WAIT SEMAPHORE\n");
	return;
    }
}
static void unlockWild()
{
    if(sem_post(wildSem) == -1)
        printf("getMdsCpoFields: CANNOT POST SEMAPHORE\n");
    sem_close(wildSem);

}
*/

char **getMdsCpoFields(int expIdx, char *cpoPath, int *numFields, int checkSegments)
{
	int i, len, status, nid, usage, numNids, nidIdx, totNumNids;
	char *mdsPath;
	void *ctx;
	char **retNames	;
	int defNid, currNid;
	int recLen = 0;
	int retLen, numSegments;
	char *currPath;
	struct nci_itm nciList[] =
		{{sizeof(int), NciLENGTH, (char *)&recLen, &retLen},
		{NciEND_OF_LIST, 0, 0, 0}};

	int *wildNids;
        lock("getMdsCpoFields");
        if(checkExpIndex(expIdx) < 0)
        {
    	    unlock();
	    return 0;
        }

//printf("IDSPATH: %s\n", cpoPath);

	len = strlen(cpoPath) + 1;
	mdsPath = malloc(len+20);
//	sprintf(mdsPath, "\\TOP.%s", cpoPath);
	sprintf(mdsPath, "\\TOP.%s", convertPath(cpoPath));

	len = strlen(mdsPath);

	for(i = 3; i < len; i++)
	{
		if(mdsPath[i] == '/')
			mdsPath[i] = '.';
	}
//   	lockWild();


//printf("MDSPATH: %s\n", mdsPath);
	status = TreeFindNode(mdsPath, &nid);
	if(!(status & 1))
	{
	    free(mdsPath);
	    sprintf(errmsg, "getMdsCpoFields: Node at path %s not found: %s", cpoPath, MdsGetMsg(status));
	    printf(errmsg, "getMdsCpoFields: Node at path %s not found: %s", cpoPath, MdsGetMsg(status));
	    unlock();
	    return 0;
	}
	free(mdsPath);


	status = TreeGetDefaultNid(&defNid);
	if(!(status & 1))
	{
	    sprintf(errmsg, "Internal error in getDefaultNid: %s\n", MdsGetMsg(status));
	    printf("Internal error in getDefaultNid: %s\n", MdsGetMsg(status));
	    unlock();
	    return 0;
	}

	status = TreeSetDefaultNid(nid);
	if(!(status & 1))
	{
	    sprintf(errmsg, "Internal error in setDefaultNid: %s\n", MdsGetMsg(status));
	    printf("Internal error in setDefaultNid: %s\n", MdsGetMsg(status));
	    unlock();
	    return 0;
	}



	usage = 1 << TreeUSAGE_NUMERIC;
	usage |= 1 << TreeUSAGE_SIGNAL;
	usage |= 1 << TreeUSAGE_TEXT;

	numNids = 0;
	ctx = 0;



  	while ((status = TreeFindNodeWild("***",&currNid,&ctx,usage)) & 1)
	    numNids++;
	TreeFindNodeEnd(&ctx);

	retNames = (char **)malloc(numNids * sizeof(char *));
	wildNids = (int *)malloc(sizeof(int) * numNids);
	numNids = 0;
	ctx = 0;
  	while ((status = TreeFindNodeWild("***",&currNid,&ctx,usage)) & 1)
	{
	    wildNids[numNids] = currNid;
	    numNids++;
	}
	TreeFindNodeEnd(&ctx);

	//unlockWild();


	totNumNids = numNids;
	numNids = 0;
	for(nidIdx = 0; nidIdx < totNumNids; nidIdx++)
	{
	    currNid = wildNids[nidIdx];

	    recLen = 0;
	    status = TreeGetNci(currNid, nciList);
	    if(!(status & 1))
	    {
	    	sprintf(errmsg, "Internal error in GetNci: %s\n", MdsGetMsg(status));
	        printf("Internal error in GetNci: %s\n", MdsGetMsg(status));
		unlock();
	        return 0;
	    }
	    if(recLen > 0)
	    {
	        currPath = TreeGetMinimumPath(&nid, currNid);

		if(!isAnotherInstance(currPath))
		{
		    status = TreeGetNumSegments(wildNids[nidIdx], &numSegments);
	    	    if(!(status & 1))
	    	    {
	    		sprintf(errmsg, "Internal error in GetNumSegments: %s\n", MdsGetMsg(status));
	        	printf("Internal error in GetNumSegments: %s\n", MdsGetMsg(status));
			unlock();
	        	return 0;
	    	    }
		    if(!checkSegments || numSegments > 0)
		    {
		      	retNames[numNids] = malloc(strlen(currPath) + 1);
		    	if(currPath[0] == '.' || currPath[0] == ':')
		            strcpy(retNames[numNids], &currPath[1]);
		    	else
		            strcpy(retNames[numNids], currPath);
		    	for(i = 0; i < strlen(retNames[numNids]); i++)
		    	{
		           if(retNames[numNids][i] == ':' || retNames[numNids][i] == '.')
		           	retNames[numNids][i]  = '/';
		    	}
	    	    	numNids++;
		    }
		}
		TreeFree(currPath);
	    }
	}
	TreeSetDefaultNid(defNid);
	/*DEBUG
	for(i = 0; i < numNids; i++)
	    printf("%s\n", retNames[i]);
*/
  	*numFields = numNids;
	free((char *)wildNids);
	unlock();
	return retNames;

}

void mdsDeleteAllFields(int expIdx, char *cpoPath)
{
    int numFields = 0, i, nid, status;

    char **paths = getMdsCpoFields(expIdx, cpoPath, &numFields, 0);
    EMPTYXD(emptyXd);

 //printf("DELETE ALL FIELDS %d %s %d\n", expIdx, cpoPath, numFields);

    for(i = 0; i < numFields; i++)
    {
 //printf("mdsDeletAllFields. cpoPath: %s, paths[i]: %s \n",cpoPath, paths[i]);
    	status = mdsputDataLocal(expIdx, cpoPath, paths[i], (struct descriptor *)&emptyXd);
	printf("%s\n", paths[i]);
	if(status)
	    printf("INTERNAL ERROR: Cannot delete data\n");
    }
    for(i = 0; i < numFields; i++)
        free(paths[i]);
    free((char *)paths);


//printf("DELETED\n");
}

int mdsIsSliced(int expIdx, char *cpoPath, char *path)
{
    int status, numSegments = 0, nid;
    lock("mdsIsSliced");
    checkExpIndex(expIdx);
        nid = getNid(cpoPath, path);
    status = TreeGetNumSegments(nid, &numSegments);
    if(!(status & 1))
    {
	sprintf(errmsg, "Internal error: cannot get num segments for %s, field: %s - %s", cpoPath, path, MdsGetMsg(status));
	printf(errmsg, "Internal error: cannot get num segments for %s, field: %s - %s\n", cpoPath, path, MdsGetMsg(status));
    }
    unlock();
    return numSegments > 0;
}

char *mdsLastErrmsg()
{
    return errmsg;
}

//Array of structures stuff
//Initialize room for a generic array of structures
#define OBJECT_CHUNK 256
static void addApdSlot(struct descriptor_a *apd, struct descriptor *item)
{
    struct descriptor *slots;
    int nSamples = apd->arsize/apd->length;
    if(nSamples > 0 && (nSamples % OBJECT_CHUNK == 0))
    {
	slots = (struct descriptor *)apd->pointer;
	apd->pointer = malloc((nSamples + OBJECT_CHUNK) * sizeof(struct descriptor *));
	memcpy(apd->pointer, slots, apd->arsize);
	free((char *)slots);
    }
    ((void **)apd->pointer)[nSamples] = item;
    apd->arsize += sizeof(struct descriptor *);
}

void *mdsBeginObject()
{
    DESCRIPTOR_APD(emptyApd, DTYPE_L, 0, 0);
    struct descriptor_a *apdPtr = (struct descriptor_a *)malloc(sizeof(emptyApd));
    memcpy(apdPtr, &emptyApd, sizeof(emptyApd));
//    apdPtr->class = CLASS_APD;
//    apdPtr->dtype = DTYPE_DSC;
//    apdPtr->dtype = DTYPE_L;
    apdPtr->length = sizeof(struct descriptor *);
    apdPtr->arsize = 0;
    apdPtr->pointer = malloc(OBJECT_CHUNK * sizeof(struct descriptor *));
    return apdPtr;
}


//Releases memory for array of structures
static void releaseObject(void *obj)
{
    struct descriptor_a *apdPtr;
    struct descriptor *dPtr;
    struct descriptor_a *aPtr;
    int i, nItems;

    apdPtr = (struct descriptor_a *)obj;
    if(apdPtr->class == CLASS_APD)
    {
	nItems = apdPtr->arsize/apdPtr->length;
	for(i = 0; i < nItems; i++)
	  //OH: bug fix = test for un-allocated elements of a struct_array
	  if (((void **)apdPtr->pointer)[i] != NULL)
	    releaseObject(((void **)apdPtr->pointer)[i]);
	free((char *)apdPtr->pointer);
	free((char *)apdPtr);
    }
    else if(apdPtr->class == CLASS_A)
    {
	aPtr = (struct descriptor_a *)apdPtr;
	if(aPtr->arsize > 0)
	    free((char *)aPtr->pointer);
	free((char *)aPtr);
    }
    else if(apdPtr->class == CLASS_S)
    {
	dPtr = (struct descriptor *)apdPtr;
	if(dPtr->length > 0)
	    free((char *)dPtr->pointer);
	free((char *)dPtr);
    }
    else
    {
	printf("FATAL: unexpected descriptor in mdsReleaseObject\n");
	exit(0);
    }
}

int putObjectSegment(int expIdx, char *cpoPath, char *path, void *objSegment, int segIdx)
{
    if(isExpRemote(expIdx))
        return putObjectSegmentRemote(getExpConnectionId(expIdx), getExpRemoteIdx(expIdx), cpoPath, path, objSegment, segIdx);
    else
        return putObjectSegmentLocal(expIdx, cpoPath, path, objSegment, segIdx);
}

static int putObjectSegmentLocalOLD(int expIdx, char *cpoPath, char *path, void *objSegment, int segIdx)
{
    int idx;
    struct descriptor idxD = {4, DTYPE_L, CLASS_S, (char *)&idx};
    EMPTYXD(serializedXd);
    int status;
    struct descriptor_a *arrD;
    struct descriptor_a *apd;
    int numSamples, nid;

    lock("putObjectSegmentLocal");
    checkExpIndex(expIdx);
        nid = getNid(cpoPath, path);
    if(nid == -1)
    {
	unlock();
	return -1;
    }

    reportInfo("PUT OBJECT SEGMENT 1\n", "");
    reportInfo(cpoPath, path);

    status = TreeGetNumSegments(nid, &idx);
    if(!(status & 1))
    {
        sprintf(errmsg, "Error reading number of segments at path %s/%s: %s ", path, cpoPath, MdsGetMsg(status));
	unlock();
	return -1;
    }

    reportInfo("PUT OBJECT SEGMENT 1 \n", "");
    apd = (struct descriptor_a *) objSegment;
    numSamples = apd->arsize / sizeof(struct descriptor *);
    if(numSamples == 1) //The object has been passed by put/replaceObjectSlice and therefore it is an array with 1 elements
        status = MdsSerializeDscOut(((struct descriptor **)apd->pointer)[0], &serializedXd);
    else //The object has been passed by putObject
         status = MdsSerializeDscOut((struct descriptor *)apd, &serializedXd);
    arrD = (struct descriptor_a *)serializedXd.pointer;
    if(!(status & 1) || !arrD || arrD->class != CLASS_A)
    {
	sprintf(errmsg, "INTERNAL ERROR:CANNOT SERIALIZE STRUCTURE : %s", MdsGetMsg(status));
	unlock();
	return -1;
    }
    reportInfo("PUT OBJECT SEGMENT 3\n", "");
    if(segIdx == -1)
    {
            reportInfo("PUT OBJECT SEGMENT MAKE SEGMENT %s\n", decompileDsc(arrD));
	    status = TreeMakeSegment(nid, &idxD, &idxD, &idxD, arrD, segIdx, arrD->arsize);
    }
    else
    {
        reportInfo("PUT OBJECT SEGMENT PUT SEGMENT %s\n", decompileDsc(arrD));
    	status = TreePutSegment(nid, segIdx, arrD);
    }
    if(status & 1)
    {
        reportInfo("PUT OBJECT SEGMENT 4 SUCCESS %s\n", TreeGetPath(nid));
	//exit(0);
    }
    else
        reportInfo("PUT OBJECT SEGMENT 4 FAILURE %s\n", TreeGetPath(nid));

    MdsFree1Dx(&serializedXd, 0);
    if(!(status & 1))
    {
	sprintf(errmsg, "Error Writing Object Segment: %s", MdsGetMsg(status));
	unlock();
	return -1;
    }
    unlock();
    return 0;
}

//If a serialized object is larger than the treshold, a single segment is created for it
#define SEGMENT_OBJECT_TRESHOLD 10000
//Defaut dimension of a created segment when multiple objects may be put in the same segment
#define SEGMENT_OBJECT_SIZE 30000


static int putTimedObjectLocal(int expIdx, char *cpoPath, char *path, void **objSlices, int numSlices)
{
    EMPTYXD(emptyXd);
    struct descriptor_xd *serializedXds;
    unsigned int segmentSize, totSize;
    int nid, status, i, sliceIdx, numSegmentSlices;
    struct descriptor_a **apds, *arrD;
    int *sliceSizes, *segmentTimes;
    int startSegIdx, endSegIdx, currIdx;
    struct descriptor startSegIdxD = {4, DTYPE_L, CLASS_S, (char *)&startSegIdx};
    struct descriptor endSegIdxD = {4, DTYPE_L, CLASS_S, (char *)&endSegIdx};
    char *segmentData, *segmentTemplate;
    DESCRIPTOR_A(segmentDataDsc, 1, DTYPE_B, 0, 0);
    DESCRIPTOR_A(segmentTemplateDsc, 1, DTYPE_B, 0, 0);
    DESCRIPTOR_A(segmentTimesDsc, sizeof(int), DTYPE_L, 0, 0);


    lock("putObjectSegmentLocal");
    checkExpIndex(expIdx);
        nid = getNid(cpoPath, path);
    if(nid == -1)
    {
	unlock();
	return -1;
    }
    if(!isEmpty(nid))
    {
	status = TreePutRecord(nid, (struct descriptor *)&emptyXd, 0);
	if(!(status & 1))
    	{
	    unlock();
	    sprintf(errmsg, "INTERNAL ERROR:CANNOT DELETE DATA : %s", MdsGetMsg(status));
	    return -1;
    	}
    }
    apds = (struct descriptor_a **)objSlices;
    serializedXds = (struct descriptor_xd *)malloc(sizeof(struct descriptor_xd) * numSlices);
    sliceSizes = (int *)malloc(sizeof(int) * numSlices);
    for(sliceIdx = 0; sliceIdx < numSlices; sliceIdx++)
    {
	serializedXds[sliceIdx] = emptyXd;
	status = MdsSerializeDscOut((struct descriptor *)apds[sliceIdx], &serializedXds[sliceIdx]);
    	arrD = (struct descriptor_a *)(serializedXds[sliceIdx].pointer);
    	if(!(status & 1) || !arrD || arrD->class != CLASS_A)
    	{
	    sprintf(errmsg, "INTERNAL ERROR:CANNOT SERIALIZE STRUCTURE : %s", MdsGetMsg(status));
	    unlock();
	    return -1;
    	}
	sliceSizes[sliceIdx] = arrD->arsize;
    }

    sliceIdx = 0;
    while(sliceIdx < numSlices)
    {
	if(sliceSizes[sliceIdx] >= SEGMENT_OBJECT_TRESHOLD)
	{
   	    arrD = (struct descriptor_a *)(serializedXds[sliceIdx].pointer);
	    startSegIdx = endSegIdx = sliceIdx;
	    status = TreeMakeSegment(nid, &startSegIdxD, &endSegIdxD, &endSegIdxD, arrD, -1, arrD->arsize);
	    sliceIdx++;
	}
	else
	{
	    totSize = 0;
	    numSegmentSlices = 0;
	    startSegIdx = sliceIdx;
	    for(i = sliceIdx; i < numSlices && totSize < SEGMENT_OBJECT_SIZE ; i++)
	    {
		totSize += (sizeof(int)+sliceSizes[i]);
		numSegmentSlices++;
	    }
	    endSegIdx = sliceIdx + numSegmentSlices - 1;
	    segmentData = malloc(totSize);
	    segmentDataDsc.pointer = segmentData;
	    segmentDataDsc.arsize = totSize;
	    currIdx = 0;
	    for(i = 0; i < numSegmentSlices; i++)
	    {
   	    	arrD = (struct descriptor_a *)(serializedXds[sliceIdx + i].pointer);
		memcpy(&segmentData[currIdx], &sliceSizes[sliceIdx +i], sizeof(int));
		currIdx += sizeof(int);
		memcpy(&segmentData[currIdx],arrD->pointer, sliceSizes[sliceIdx +i]);
		currIdx += sliceSizes[sliceIdx +i];
	    }
	    if(totSize >= SEGMENT_OBJECT_SIZE) //A whole segment is filled
	      	status = TreeMakeSegment(nid, &startSegIdxD, &endSegIdxD, &endSegIdxD, (struct descriptor_a *)&segmentDataDsc, -1, totSize);
	    else
	    {
		segmentTemplate = malloc(SEGMENT_OBJECT_SIZE);
		segmentTemplateDsc.pointer = segmentTemplate;
		segmentTemplateDsc.arsize = SEGMENT_OBJECT_SIZE;
		segmentTimes = (int *)malloc(sizeof(int) * numSegmentSlices);
		for(i = 0; i < numSegmentSlices; i++)
		    segmentTimes[i] = sliceIdx + i;
		segmentTimesDsc.pointer = (char *)segmentTimes;
		segmentTimesDsc.arsize = sizeof(int) * numSegmentSlices;
                status = TreeBeginSegment(nid, &startSegIdxD, &endSegIdxD, (struct descriptor *)&segmentTimesDsc, (struct descriptor_a *)&segmentTemplateDsc, -1);
               	if(status & 1) status = TreePutSegment(nid, -1, (struct descriptor_a *)&segmentDataDsc);
		free(segmentTemplate);
		free(segmentTimes);
	    }
	    sliceIdx += numSegmentSlices;
	    free((char *)segmentData);
	}
        if(!(status & 1))
        {
            sprintf(errmsg, "Cannot write data segment at path %s, IDS path %s: %s", path, cpoPath, MdsGetMsg(status));
	    //Free stuff
	    for(i = 0; i < numSlices; i++)
		MdsFree1Dx(&serializedXds[i], 0);
	    free((char *)serializedXds);
	    free((char *)sliceSizes);
 	    unlock();
            return -1;
        }
    }//endwhile
    //Free stuff
    for(i = 0; i < numSlices; i++)
	MdsFree1Dx(&serializedXds[i], 0);
    free((char *)serializedXds);
    free((char *)sliceSizes);
    unlock();
    return 0;
}



static int putObjectSegmentLocal(int expIdx, char *cpoPath, char *path, void *objSegment, int segIdx)
{
    int startSegIdx, endSegIdx, numSegments;
    struct descriptor startSegIdxD = {4, DTYPE_L, CLASS_S, (char *)&startSegIdx};
    struct descriptor endSegIdxD = {4, DTYPE_L, CLASS_S, (char *)&endSegIdx};
    EMPTYXD(serializedXd);
    EMPTYXD(startXd);
    EMPTYXD(endXd);
    int status;
    struct descriptor_a *arrD;
    struct descriptor_a *apd;
    int numSamples, nid;
    int leftRows, leftItems, leftBytes;
    char *extSerialized;
    DESCRIPTOR_A(extSerializedDsc, 1, DTYPE_B, 0, 0);
    EMPTYXD(retSegmentXd);
    EMPTYXD(retDimXd);
    char *fillArray;
    DESCRIPTOR_A(fillArrayDsc, 1, DTYPE_B, 0, 0);
    struct descriptor_a *currArrD;

    lock("putObjectSegmentLocal");
    checkExpIndex(expIdx);
        nid = getNid(cpoPath, path);
    if(nid == -1)
    {
	unlock();
	return -1;
    }

//Accessory segment information:
//	startTime: initial index of objects in this segment
//	endTime: final index of objects in this segment

//Get number of segment. If segments are present, get lastIdx of last segment
    status = TreeGetNumSegments(nid, &numSegments);
    if(!(status & 1))
    {
        sprintf(errmsg, "Error reading number of segments at path %s/%s: %s ", path, cpoPath, MdsGetMsg(status));
	unlock();
	return -1;
    }
    if(numSegments == 0)
    {
	startSegIdx = -1;
	endSegIdx = -1;
    }
    else
    {
	status = TreeGetSegmentLimits(nid, numSegments - 1, &startXd, &endXd);
    	if(!(status & 1))
    	{
             sprintf(errmsg, "Error reading nsegment limits at path %s/%s: %s ", path, cpoPath, MdsGetMsg(status));
	    unlock();
	    return -1;
    	}
	startSegIdx = *((int *)startXd.pointer->pointer);
	endSegIdx = *((int *)endXd.pointer->pointer);
	MdsFree1Dx(&startXd, 0);
	MdsFree1Dx(&endXd, 0);
    }

// Serialize object
    apd = (struct descriptor_a *) objSegment;
    numSamples = apd->arsize / sizeof(struct descriptor *);
    if(numSamples == 1) //The object has been passed by put/replaceObjectSlice and therefore it is an array with 1 elements
        status = MdsSerializeDscOut(((struct descriptor **)apd->pointer)[0], &serializedXd);
    else //The object has been passed by putObject
         status = MdsSerializeDscOut((struct descriptor *)apd, &serializedXd);
    arrD = (struct descriptor_a *)serializedXd.pointer;
    if(!(status & 1) || !arrD || arrD->class != CLASS_A)
    {
	sprintf(errmsg, "INTERNAL ERROR:CANNOT SERIALIZE STRUCTURE : %s", MdsGetMsg(status));
	unlock();
	return -1;
    }
    if(segIdx != -1)
//ReplaceLastObject case: decrease number of slices for this segment and write a new straight segment for this slice
    {
	endSegIdx--;
	status = TreeUpdateSegment(nid, &startSegIdxD, &endSegIdxD, &endSegIdxD, numSegments-1);//Dimension field
	if(!(status & 1))
    	{
	    sprintf(errmsg, "INTERNAL ERROR:CANNOT SERIALIZE STRUCTURE : %s", MdsGetMsg(status));
	    unlock();
	    return -1;
    	}
	if(endSegIdx != -1 && endSegIdx == startSegIdx) //If only one slice for this segment is left, change its content in the "traditional" way, i.e. without 4 byte length in front of serialized
	{
	    status = TreeGetSegment(nid, -1, &retSegmentXd, &retDimXd);
	    if(!(status & 1))
    	    {
	    	sprintf(errmsg, "INTERNAL ERROR:CANNOT SERIALIZE STRUCTURE : %s", MdsGetMsg(status));
	    	unlock();
	    	return -1;
    	    }
	    currArrD = (struct descriptor_a *)retSegmentXd.pointer;
	    currArrD->pointer += sizeof(int);
	    currArrD->arsize -= sizeof(int);
	    status = TreePutSegment(nid, 0, (struct descriptor_a *)retSegmentXd.pointer);
	    if(!(status & 1))
    	    {
	    	sprintf(errmsg, "INTERNAL ERROR:CANNOT SERIALIZE STRUCTURE : %s", MdsGetMsg(status));
	    	unlock();
	    	return -1;
    	    }
	    currArrD->pointer -= sizeof(int);
	    currArrD->arsize += sizeof(int);
	    MdsFree1Dx(&retSegmentXd, 0);
	    MdsFree1Dx(&retDimXd, 0);
	}
	startSegIdx = endSegIdx = endSegIdx+1;
	status = TreeMakeSegment(nid, &startSegIdxD, &endSegIdxD, &endSegIdxD, arrD, -1, arrD->arsize);
    	MdsFree1Dx(&serializedXd, 0);
	unlock();
	return 0;
    }

    if(numSegments > 0)
    {
    	getLeftItems(nid, &leftItems, &leftRows);
    	leftBytes = leftItems; //Serialized objects are saves as byte arays
    //4 additional bytes to store serialized object length when multiple
    //serialized objects are stored in the same segment
    }
    else
	leftBytes = 0;

    if(leftBytes > arrD->arsize + 4)  //In this case the (length + serialized object) is put in the current segment
    {

	extSerialized = malloc(arrD->arsize + sizeof(int));
	memcpy(extSerialized, &arrD->arsize, sizeof(int));
	memcpy(&extSerialized[sizeof(int)], arrD->pointer, arrD->arsize);
	extSerializedDsc.pointer = extSerialized;
	extSerializedDsc.arsize = sizeof(int) + arrD->arsize;
    	status = TreePutSegment(nid, -1, (struct descriptor_a *)&extSerializedDsc);
	free(extSerialized);
	if(status & 1)
	{
	    endSegIdx++;
	    status = TreeUpdateSegment(nid, &startSegIdxD, &endSegIdxD, &endSegIdxD, numSegments-1);//Dimension field not useful
	}
    }
    else //a new segment must be created
    {
//Check if the last segment contains only one slice, in which case change it in "traditional" storage
	if(endSegIdx != -1 && leftBytes >0 && startSegIdx == endSegIdx)
	{
	    status = TreeGetSegment(nid, -1, &retSegmentXd, &retDimXd);
	    if(!(status & 1))
    	    {
	    	sprintf(errmsg, "INTERNAL ERROR:CANNOT GET SEGMENT : %s", MdsGetMsg(status));
	    	unlock();
	    	return -1;
    	    }
	    currArrD = (struct descriptor_a *)retSegmentXd.pointer;
	    currArrD->pointer += sizeof(int);
	    currArrD->arsize -= sizeof(int);
	    status = TreePutSegment(nid, 0, (struct descriptor_a *)retSegmentXd.pointer);
	    if(!(status & 1))
    	    {
	    	sprintf(errmsg, "INTERNAL ERROR:CANNOT PUT SEGMENT : %s", MdsGetMsg(status));
	    	unlock();
	    	return -1;
    	    }
	    currArrD->pointer -= sizeof(int);
	    currArrD->arsize += sizeof(int);
	    MdsFree1Dx(&retSegmentXd, 0);
	    MdsFree1Dx(&retDimXd, 0);
	}
	if((arrD->arsize + sizeof(int)) >= SEGMENT_OBJECT_TRESHOLD)
/* Note that in this case the situation in which a single slice fits in a segment in "non traditional"
   way is avoided */
	{
//Large object slice, make a segment only for it
	    startSegIdx = endSegIdx = endSegIdx+1;
	    status = TreeMakeSegment(nid, &startSegIdxD, &endSegIdxD, &endSegIdxD, arrD, -1, arrD->arsize);
	}
	else
	{
//Create a new segment large SEGMENT_OBJECT_SIZE
	    extSerialized = calloc(SEGMENT_OBJECT_SIZE, 1);
	    extSerializedDsc.pointer = extSerialized;
	    extSerializedDsc.arsize = SEGMENT_OBJECT_SIZE;
	    startSegIdx = endSegIdx = endSegIdx+1;
	    status = TreeBeginSegment(nid, &startSegIdxD, &endSegIdxD, &endSegIdxD, (struct descriptor_a *)&extSerializedDsc, -1);
	    free(extSerialized);
	    if(status & 1)
	    {
		extSerialized = malloc(arrD->arsize + sizeof(int));
		memcpy(extSerialized, &arrD->arsize, sizeof(int));
		memcpy(&extSerialized[sizeof(int)], arrD->pointer, arrD->arsize);
		extSerializedDsc.pointer = extSerialized;
		extSerializedDsc.arsize = sizeof(int) + arrD->arsize;
    		status = TreePutSegment(nid, -1, (struct descriptor_a *)&extSerializedDsc);
		free(extSerialized);
	    }
	}
    }
    MdsFree1Dx(&serializedXd, 0);
    if(!(status & 1))
    {
	sprintf(errmsg, "Error Writing Object Segment: %s", MdsGetMsg(status));
	unlock();
	return -1;
    }
    unlock();
    return 0;
}




static int putTimedObject(int expIdx, char *cpoPath, char *path, void *obj)
{
    struct descriptor_a *apd;
    int numSamples;
    int i, nid, status;
    void **currPtr;
    char *fullPath;
    int cacheLevel = getCacheLevel(expIdx);


    reportInfo("PUT TIMED OBJECT\n", "");

    if(cacheLevel > 0)
    	updateInfoTimedObject(expIdx, cpoPath, path, 1, obj);
    if(cacheLevel == 2)
    {
        releaseObject(obj);
   	return 0;
    }
    apd = (struct descriptor_a *)obj;
    if(apd->class != CLASS_APD)
    {
	sprintf(errmsg, "INTERNAL ERROR:Not an APD at path %s, IDS path %s", path, cpoPath);
	return -1;
    }

    fullPath = malloc(strlen(path) + 7);

    sprintf(fullPath, "%s/timed", path);
// For test only    sprintf(fullPath, "%s", path);


    numSamples = apd->arsize / apd->length;
    currPtr = (void **)apd->pointer;

//printf ("PUT TIME OBJECT: %d Samples\n", numSamples);

//Gabriele 2014: a more performing version of putTimedObject is provided only for local access
    if(!isExpRemote(expIdx))
	putTimedObjectLocal(expIdx, cpoPath, fullPath, currPtr, numSamples);
    else //remote access, use the old approach
    {
    	for(i = 0; i < numSamples; i++)
    	{
	    status = putObjectSegment(expIdx, cpoPath, fullPath, currPtr[i], -1);
	    if(status)
	    {
	    	return status;
	    }
    	}
    }
    releaseObject(obj);
    free(fullPath);
    return 0;
}

int mdsPutObject(int expIdx, char *cpoPath, char *path, void *obj, int isTimed)
{
    int nid, status, cacheLevel;
    char *fullPath;
    EMPTYXD(serializedXd);
    struct descriptor_a *arrD;

    if(isTimed)
	return putTimedObject(expIdx, cpoPath, path, obj);
    fullPath = malloc(strlen(path) + 11);
    sprintf(fullPath, "%s/non_timed", path);

    cacheLevel = getCacheLevel(expIdx);
    if(cacheLevel > 0)
    {
	updateInfoObject(expIdx, cpoPath, path, 1, obj);
    }
    if(cacheLevel == 2)
    {
        releaseObject(obj);
    	free(fullPath);
    	return 0;
    }
    if(isExpRemote(expIdx))
      status = putDataRemote(getExpConnectionId(expIdx), getExpRemoteIdx(expIdx), cpoPath, fullPath, obj);
    else
    	status = putDataLocal(expIdx, cpoPath, fullPath, obj);
    free(fullPath);
    releaseObject(obj);
    return status;
}

static char *getApdName(struct descriptor_a *apd)
{
    struct descriptor *strD;
    char *retName;
    if(apd->class != CLASS_APD || apd->arsize/apd->length < 2|| ((struct descriptor **)apd->pointer)[0]->dtype != DTYPE_T)
    {
	printf("FATAL ERROR in getName for APD\n");
	exit(0);
    }
    strD = ((struct descriptor **)apd->pointer)[0];
    retName = malloc(strD->length + 1);
    memcpy(retName, strD->pointer, strD->length);
    retName[strD->length] = 0;
    return retName;
}


static struct descriptor *makeStringDescriptor(char *name)
{
    int len = strlen(name);
    struct descriptor *descr = (struct descriptor *)malloc(sizeof(struct descriptor));
    descr->class = CLASS_S;
    descr->dtype = DTYPE_T;
    descr->length = strlen(name);
    descr->pointer = malloc(len);
    memcpy(descr->pointer, name, len);
    return descr;
}

static void putInApd(struct descriptor_a *apd, char *prevName, struct descriptor *dataD)
{
    struct descriptor_a *currApd;
    char *name, *currName;
    struct descriptor *newNameD;
    int numChildren, i;

    name = strtok(NULL, "/");
    numChildren = apd->arsize / apd->length - 1;
    if(name) //This is not the last name in the path
    {
	//In this case the current apd can contain only other apds (except the first element which is the name)
	for(i = 0; i < numChildren; i++)
	{
	    currApd = ((struct descriptor_a **)apd->pointer)[i+1];
	    if(currApd->class != CLASS_APD)
	    {
		printf("FATAL ERROR: inconsistent APD structure\n");
		exit(0);
	    }
	    currName = getApdName(currApd);
	    if(!strcmp(currName, name))
	    {
		free(currName);
		break;
	    }
	    free(currName);
	}
	if(i == numChildren) //If an APD with the same name has not been found, create a new one
	{
	    currApd = mdsBeginObject();
	    addApdSlot(currApd, makeStringDescriptor(name));
	    addApdSlot(apd, (struct descriptor *)currApd);
	}
	putInApd(currApd, name, dataD);
    }
    else //This is the last name in the path. This APD cannot conain other apds
    {
	if(numChildren != 0)
	{
	    printf("FATAL ERROR: wrong insertion in an APD\n");
	    exit(0);
	}
	addApdSlot(apd, dataD);
    }
}


static void putInObject(void *obj, char *path, int idx, struct descriptor *dataD)
{
    int pathLen = strlen(path);
    char *currPath = malloc(pathLen + 1); //strtok changes the name
    char *currName, *prevName;
    struct descriptor_a *apd, *currApd;
    int i, numElements;

//printf("PUT IN OBJECT %s\n", path);


    strcpy(currPath, path);
    apd = (struct descriptor_a *)obj;
    if(apd->class != CLASS_APD)
    {
	printf("FATAL ERROR: not an APD in putInObject\n");
	exit(0);
    }
    lock("putInObject"); //strtok is not thread safe
    numElements = apd->arsize/apd->length;
    currName = strtok(currPath, "/");
    //make enough room for this idx
    for(i = 0; i < idx - numElements + 1; i++)
	addApdSlot(apd, NULL);
    currApd = ((struct descriptor_a **)apd->pointer)[idx];
    if(!currApd) //not yet existing
    {
	((struct descriptor_a **)apd->pointer)[idx] = currApd = mdsBeginObject();
	addApdSlot(currApd, makeStringDescriptor(currName));
    }
    else
    {
	prevName = getApdName(currApd);
	if(strcmp(currName, prevName))
	{
	    printf("FATAL ERROR: wrong initial name in APD\n");
	    exit(0);
	}
	free(prevName);
    }
    putInApd(currApd, currName, dataD);

    free(currPath);
    unlock();
}

static void *allocateDescr(int type, void *data, int size, int totSize, int nDims, int dim1, int dim2, int dim3, int dim4, int dim5, int dim6, int dim7)
{
    DESCRIPTOR_A_COEFF(arrD, size, type, 0, 7, 0);
    ARRAY_COEFF(char *, 7) *arrDPtr;
    struct descriptor *descrPtr;
    char *dataPtr = 0;
    int i;

    if(totSize > 0)
    {
    	dataPtr = malloc(totSize);
    	memcpy(dataPtr, data, totSize);
    }
    if(nDims == 0)
    {
	descrPtr = (struct descriptor *)malloc(sizeof(struct descriptor));
	descrPtr->dtype = type;
	descrPtr->class = CLASS_S;
	descrPtr->length = size;
	descrPtr->pointer = dataPtr;
	return descrPtr;
    }
     else {
	arrD.dimct = nDims;
	arrD.arsize = totSize;
	arrD.dtype = type;
	arrD.m[0] = dim1;
	arrD.m[1] = dim2;
	arrD.m[2] = dim3;
	arrD.m[3] = dim4;
	arrD.m[4] = dim5;
	arrD.m[5] = dim6;
	arrD.m[6] = dim7;
	arrD.pointer = dataPtr;
	arrDPtr = (void *)malloc(sizeof(arrD));
	memcpy(arrDPtr, &arrD, sizeof(arrD));
	return arrDPtr;
    }
}


//Add elements to the structure array. Note: returns the new pointer to the object, possibly changed (if object reallocated)
//Argument path refers to the path name within the structure
//Argument idx is the index in the array of structues
void *mdsPutIntInObject(void *obj, char *path, int idx, int data)
{
    putInObject(obj, path, idx, allocateDescr(DTYPE_L, &data, sizeof(int), sizeof(int), 0,0,0,0,0,0,0,0));
    return obj;
}

void *mdsPutStringInObject(void *obj, char *path, int idx, char *inData)
{
    DESCRIPTOR_A(dataD, 1, DTYPE_BU, 0, 0);
    EMPTYXD(emptyXd);
    struct descriptor_a *dataDPtr;

    //printf("PUT STRING %d %s %s %s\n", strlen(inData), cpoPath, path, inData);
    if(!inData || !*inData) //Missing String
    {
       	dataD.arsize = 0;
	dataD.pointer = 0;
// 	dataD.pointer = malloc(1);
//	*dataD.pointer = 0;
    }
    else
    {
    	dataD.arsize = strlen(inData);
    	dataD.pointer = malloc(dataD.arsize);
    	memcpy(dataD.pointer, inData, dataD.arsize);
    }
    dataDPtr = malloc(sizeof(dataD));
    memcpy(dataDPtr, &dataD, sizeof(dataD));
    putInObject(obj, path, idx, (struct descriptor *)dataDPtr);
    return obj;
}

void *mdsPutFloatInObject(void *obj, char *path, int idx, float data)
{
    putInObject(obj, path, idx, allocateDescr(DTYPE_FLOAT, &data, sizeof(float), sizeof(float), 0,0,0,0,0,0,0,0));
    return obj;
}

void *mdsPutDoubleInObject(void *obj, char *path, int idx, double data)
{
    putInObject(obj, path, idx, allocateDescr(DTYPE_DOUBLE, &data, sizeof(double), sizeof(double), 0,0,0,0,0,0,0,0));
    return obj;
}

void *mdsPutVect1DStringInObject(void *obj, char *path, int idx, char **data, int dim)
{
    int maxLen = 0, len;
    int i;
    char *names;

    for(i = 0; i < dim; i++)
    {
	if((len = strlen(data[i])) > maxLen)
	    maxLen = len;
    }
    names = malloc(dim*maxLen);
    memset(names, ' ', dim*maxLen);
    for(i = 0; i < dim; i++)
    {
	memcpy(&names[i*maxLen], data[i], strlen(data[i]));
    }
    putInObject(obj, path, idx, allocateDescr(DTYPE_T, names, maxLen, dim*maxLen, 1, dim,0,0,0,0,0,0));
    free(names);
    return obj;
}
void *mdsPutVect1DIntInObject(void *obj, char *path, int idx, int *data, int dim)
{
    putInObject(obj, path, idx, allocateDescr(DTYPE_L, data, sizeof(int), dim*sizeof(int), 1, dim,0,0,0,0,0,0));
    return obj;
}

void *mdsPutVect1DFloatInObject(void *obj, char *path, int idx, float *data, int dim)
{
    putInObject(obj, path, idx, allocateDescr(DTYPE_FLOAT, data, sizeof(float), dim*sizeof(float), 1, dim,0,0,0,0,0,0));
    return obj;
}

void *mdsPutVect1DDoubleInObject(void *obj, char *path, int idx, double *data, int dim)
{
    putInObject(obj, path, idx, allocateDescr(DTYPE_DOUBLE, data, sizeof(double), dim*sizeof(double), 1, dim,0,0,0,0,0,0));
    return obj;
}

void *mdsPutVect2DIntInObject(void *obj, char *path, int idx, int *data, int dim1, int dim2)
{
    putInObject(obj, path, idx, allocateDescr(DTYPE_L, data, sizeof(int), dim1*dim2*sizeof(int), 2, dim1,dim2,0,0,0,0,0));
    return obj;
}

void *mdsPutVect2DFloatInObject(void *obj, char *path, int idx, float *data, int dim1, int dim2)
{
    putInObject(obj, path, idx, allocateDescr(DTYPE_FLOAT, data, sizeof(float), dim1*dim2*sizeof(float), 2, dim1,dim2,0,0,0,0,0));
    return obj;
}

void *mdsPutVect2DDoubleInObject(void *obj, char *path, int idx, double *data, int dim1, int dim2)
{
    putInObject(obj, path, idx, allocateDescr(DTYPE_DOUBLE, data, sizeof(double), dim1*dim2*sizeof(double), 2, dim1,dim2,0,0,0,0,0));
    return obj;
}

void *mdsPutVect3DIntInObject(void *obj, char *path, int idx, int *data, int dim1, int dim2, int dim3)
{
    putInObject(obj, path, idx, allocateDescr(DTYPE_L, data, sizeof(int), dim1*dim2*dim3*sizeof(int), 3, dim1,dim2,dim3,0,0,0,0));
    return obj;
}

void *mdsPutVect3DFloatInObject(void *obj, char *path, int idx, float *data, int dim1, int dim2, int dim3)
{
    putInObject(obj, path, idx, allocateDescr(DTYPE_FLOAT, data, sizeof(float), dim1*dim2*dim3*sizeof(float), 3, dim1,dim2,dim3,0,0,0,0));
    return obj;
}

void *mdsPutVect3DDoubleInObject(void *obj, char *path, int idx, double *data, int dim1, int dim2, int dim3)
{
    putInObject(obj, path, idx, allocateDescr(DTYPE_DOUBLE, data, sizeof(double), dim1*dim2*dim3*sizeof(double), 3, dim1,dim2,dim3,0,0,0,0));
    return obj;
}

void *mdsPutVect4DIntInObject(void *obj, char *path, int idx, int *data, int dim1, int dim2, int dim3, int dim4)
{
    putInObject(obj, path, idx, allocateDescr(DTYPE_L, data, sizeof(int), dim1*dim2*dim3*dim4*sizeof(int), 4, dim1,dim2,dim3,dim4,0,0,0));
    return obj;
}

void *mdsPutVect4DFloatInObject(void *obj, char *path, int idx, float *data, int dim1, int dim2, int dim3, int dim4)
{
    putInObject(obj, path, idx, allocateDescr(DTYPE_FLOAT, data, sizeof(float), dim1*dim2*dim3*dim4*sizeof(float), 4, dim1,dim2,dim3,dim4,0,0,0));
    return obj;
}

void *mdsPutVect4DDoubleInObject(void *obj, char *path, int idx, double *data, int dim1, int dim2, int dim3, int dim4)
{
    putInObject(obj, path, idx, allocateDescr(DTYPE_DOUBLE, data, sizeof(double), dim1*dim2*dim3*dim4*sizeof(double), 4, dim1,dim2,dim3,dim4,0,0,0));
    return obj;
}

void *mdsPutVect5DIntInObject(void *obj, char *path, int idx, int *data, int dim1, int dim2, int dim3, int dim4, int dim5)
{
    putInObject(obj, path, idx, allocateDescr(DTYPE_L, data, sizeof(int), dim1*dim2*dim3*dim4*dim5*sizeof(int), 5, dim1,dim2,dim3,dim4,dim5,0,0));
    return obj;
}

void *mdsPutVect5DFloatInObject(void *obj, char *path, int idx, float *data, int dim1, int dim2, int dim3, int dim4,int dim5)
{
    putInObject(obj, path, idx, allocateDescr(DTYPE_FLOAT, data, sizeof(float), dim1*dim2*dim3*dim4*dim5*sizeof(float), 5, dim1,dim2,dim3,dim4,dim5,0,0));
    return obj;
}

void *mdsPutVect5DDoubleInObject(void *obj, char *path, int idx, double *data, int dim1, int dim2, int dim3, int dim4,int dim5)
{
    putInObject(obj, path, idx, allocateDescr(DTYPE_DOUBLE, data, sizeof(double), dim1*dim2*dim3*dim4*dim5*sizeof(double), 5, dim1,dim2,dim3,dim4,dim5,0,0));
    return obj;
}

void *mdsPutVect6DIntInObject(void *obj, char *path, int idx, int *data, int dim1, int dim2, int dim3, int dim4, int dim5, int dim6)
{
    putInObject(obj, path, idx, allocateDescr(DTYPE_L, data, sizeof(int), dim1*dim2*dim3*dim4*dim5*dim6*sizeof(int), 6, dim1,dim2,dim3,dim4,dim5,dim6,0));
    return obj;
}

void *mdsPutVect6DFloatInObject(void *obj, char *path, int idx, float *data, int dim1, int dim2, int dim3, int dim4,int dim5, int dim6)
{
    putInObject(obj, path, idx, allocateDescr(DTYPE_FLOAT, data, sizeof(float), dim1*dim2*dim3*dim4*dim5*dim6*sizeof(float), 6, dim1,dim2,dim3,dim4,dim5,dim6,0));
    return obj;
}

void *mdsPutVect6DDoubleInObject(void *obj, char *path, int idx, double *data, int dim1, int dim2, int dim3, int dim4,int dim5, int dim6)
{
    putInObject(obj, path, idx, allocateDescr(DTYPE_DOUBLE, data, sizeof(double), dim1*dim2*dim3*dim4*dim5*dim6*sizeof(double), 6, dim1,dim2,dim3,dim4,dim5,dim6,0));
    return obj;
}

void *mdsPutVect7DIntInObject(void *obj, char *path, int idx, int *data, int dim1, int dim2, int dim3, int dim4, int dim5, int dim6, int dim7)
{
    putInObject(obj, path, idx, allocateDescr(DTYPE_L, data, sizeof(int), dim1*dim2*dim3*dim4*dim5*dim6*dim7*sizeof(int), 7, dim1,dim2,dim3,dim4,dim5,dim6,dim7));
    return obj;
}

void *mdsPutVect7DFloatInObject(void *obj, char *path, int idx, float *data, int dim1, int dim2, int dim3, int dim4,int dim5, int dim6, int dim7)
{
    putInObject(obj, path, idx, allocateDescr(DTYPE_FLOAT, data, sizeof(float), dim1*dim2*dim3*dim4*dim5*dim6*dim7*sizeof(float), 7, dim1,dim2,dim3,dim4,dim5,dim6,dim7));
    return obj;
}

void *mdsPutVect7DDoubleInObject(void *obj, char *path, int idx, double *data, int dim1, int dim2, int dim3, int dim4,int dim5, int dim6, int dim7)
{
    putInObject(obj, path, idx, allocateDescr(DTYPE_DOUBLE, data, sizeof(double), dim1*dim2*dim3*dim4*dim5*dim6*dim7*sizeof(double), 7, dim1,dim2,dim3,dim4,dim5,dim6,dim7));
    return obj;
}

void *mdsPutObjectInObject(void *obj, char *path, int idx, void *dataObj)
{
    putInObject(obj, path, idx, dataObj);
    return obj;
}

//Retrieve the number of elements for the array of structures. Returns -1 if an error occurs;
int mdsGetObjectDim(void *obj)
{
    struct descriptor_xd *xdPtr = (struct descriptor_xd *)obj;
    struct descriptor_a *apd;

    if(xdPtr->class == CLASS_XD)
	apd = (struct descriptor_a *)xdPtr->pointer;
    else
	apd = (struct descriptor_a *)xdPtr;

    if(!apd || apd->class != CLASS_APD)
    {
	sprintf(errmsg, "Invalid object in mdsGetObjectDim\n");
	return -1;
    }
    return apd->arsize/apd->length;
}

//releaseOject is called only for read objects, which are represented by a XD pointer pointing to the startAPD descriptor
void mdsReleaseObject(void *obj)
{
    struct descriptor_xd *xd;

    xd = (struct descriptor_xd *)obj;
    if(xd->class != CLASS_XD)
    {
	releaseObject(obj); //In the case an empty object has not been put in the pulse file
	return;
    }
    MdsFree1Dx(xd, 0);
    free((char *)xd);
}

static int getObjectLocal(int expIdx, char *cpoPath, char *path,  void **obj, int isTimed, int expand)
{
    int nid, numSegments, segIdx, status;
    EMPTYXD(xd);
    struct descriptor_xd *retXd;
    EMPTYXD(emptyXd);
    EMPTYXD(dimXd);
    struct descriptor_xd *xds;
    struct descriptor **dscPtrs;
    struct descriptor_a *arrD;
//    DESCRIPTOR_APD(apd, DTYPE_DSC, 0, 0);
    DESCRIPTOR_APD(apd, DTYPE_L, 0, 0);
    char *fullPath;

    int numSlices, sliceIdx, currSliceIdx;
    int startSegId, endSegId;
    int *slicesPerSegment;
    EMPTYXD(startSegIdXd);
    EMPTYXD(endSegIdXd);
    char *currSlicePtr;
    int leftItems, leftRows;


    fullPath = malloc(strlen(path) + 11);
    if(expand)
    {
        if(isTimed)
    	    sprintf(fullPath, "%s/timed", path);
        else
            sprintf(fullPath, "%s/non_timed", path);
    }
    else //Used when retrieving object data from getCpoDataServer
    {
	strcpy(fullPath, path);
    }

    lock("getObjectLocal");
    checkExpIndex(expIdx);

    nid = getNid(cpoPath, fullPath);
    // printf("getObjectLocal. nid= %d, fullpath: %s\n",nid, fullPath);
    free(fullPath);
    if(nid == -1)
    {
	unlock();
	return -1;
    }
    status = TreeGetNumSegments(nid, &numSegments);
    if(!(status & 1))
    {
        sprintf(errmsg, "Error getting number of segments at path %s/%s ", cpoPath, path);
	unlock();
	return -1;
    }
    if(numSegments == 0)
    {
    	retXd = (struct descriptor_xd *)malloc(sizeof(struct descriptor_xd));
    	*retXd = emptyXd;
	status = TreeGetRecord(nid, retXd);
	if(!(status & 1))
	{
            sprintf(errmsg, "Error reading object at path %s/%s: %s ", cpoPath, path, MdsGetMsg(status));
	    unlock();
	    free((char *)retXd);
	    return -1;
    	}
	unlock();
	*obj = retXd;
	return 0;
    }

//Count first actual number of slices
    numSlices = 0;
    slicesPerSegment = (int *)malloc(numSegments * sizeof(int));
    for(segIdx = 0; segIdx < numSegments; segIdx++)
    {
	status = TreeGetSegmentLimits(nid, segIdx, &startSegIdXd, &endSegIdXd);
	if(!(status & 1))
	{
            sprintf(errmsg, "Error reading egment limits for object at path %s/%s: %s ", cpoPath, path, MdsGetMsg(status));
	    unlock();
	    free((char *)retXd);
	    return -1;
    	}
	startSegId = *(int *)(startSegIdXd.pointer->pointer);
	endSegId = *(int *)(endSegIdXd.pointer->pointer);
	MdsFree1Dx(&startSegIdXd, 0);
	MdsFree1Dx(&endSegIdXd, 0);
	slicesPerSegment[segIdx] = endSegId - startSegId + 1;
	numSlices += endSegId - startSegId + 1;
    }

	
    xds = (struct descriptor_xd *)malloc(sizeof(struct descriptor_xd) * numSlices);
    dscPtrs = (struct descriptor **)malloc(sizeof(struct descriptor *) * numSlices);
    for(segIdx = 0; segIdx < numSlices; segIdx++)
    	xds[segIdx] = emptyXd;

    sliceIdx = 0;
    for(segIdx = 0; segIdx < numSegments; segIdx++)
    {
        status = TreeGetSegment(nid, segIdx, &xd, &dimXd);
	MdsFree1Dx(&dimXd, 0);
	if(!(status & 1))
	{
            sprintf(errmsg, "Error reading object segment at path %s/%s: %s ", cpoPath, path, MdsGetMsg(status));
	    unlock();
	    return -1;
    	}
	if(!xd.pointer || xd.pointer->class != CLASS_A)
	{
            sprintf(errmsg, "Wrong segment data returned at path %s/%s", cpoPath, path);
	    unlock();
	    return -1;
    	}
	arrD = (struct descriptor_a *)xd.pointer;
	if(slicesPerSegment[segIdx] == 0) continue; //In case the segment sdoes not contain slices (replaceLastSegment)

	/* If it is the last segment and it contains only one slice, check if the segment is
	   completely filled. If yes, it is stored in the "traditional way" othewise the length
 	   of the serialized slice is stored in front
	*/
	if(segIdx == numSegments - 1 && slicesPerSegment[segIdx] == 1)
    	    getLeftItems(nid, &leftItems, &leftRows);
	else
	    leftRows = 0;

	if(slicesPerSegment[segIdx] == 1 && leftRows == 0)
//One slice in the segment, stored in the "traditional" way
	{
	    status = MdsSerializeDscIn(arrD->pointer, &xds[sliceIdx]);
	    MdsFree1Dx(&xd, 0);
    	    if(!(status & 1))
    	    {
        	printf("INTERNAL ERROR: Cannot deserialize data returned at path %s/%s: %s\n", cpoPath, path, MdsGetMsg(status));
		unlock();
		return -1;
    	    }
	    dscPtrs[sliceIdx] = xds[sliceIdx].pointer;
	    sliceIdx++;
	}
	else //Multiple slices per segment, the dimension (4 bytes) is stored before the serialized slice
	{
	    currSlicePtr = arrD->pointer;
	    for(currSliceIdx = 0; currSliceIdx < slicesPerSegment[segIdx]; currSliceIdx++)
	    {
		status = MdsSerializeDscIn(currSlicePtr + sizeof(int), &xds[sliceIdx]);
   	    	if(!(status & 1))
    	    	{
        	    printf("INTERNAL ERROR: Cannot deserialize data returned at path %s/%s: %s\n", cpoPath, path, MdsGetMsg(status));
		    unlock();
		    return -1;
    	    	}
	    	dscPtrs[sliceIdx] = xds[sliceIdx].pointer;
	    	sliceIdx++;
		currSlicePtr += *(int *)currSlicePtr + sizeof(int); //advance pointer to next serialized slice in segment
	    }
	    MdsFree1Dx(&xd, 0);
	}
    }
    apd.arsize = numSlices * sizeof(struct descriptor *);
    apd.pointer = (void *)dscPtrs;
    retXd = (struct descriptor_xd *)malloc(sizeof(struct descriptor_xd));
    *retXd = emptyXd;
    MdsCopyDxXd((struct descriptor *)&apd, retXd);
    for(segIdx = 0; segIdx < numSlices; segIdx++)
	MdsFree1Dx(&xds[segIdx], 0);
    free((char *)xds);
    free((char *)dscPtrs);
    free((char *)slicesPerSegment);
    unlock();
    *obj = retXd;
    return 0;
}


static int mdsgetObjectLocal(int expIdx, char *cpoPath, char *path,  void **obj, int isTimed, int expand)
{
    int nid, numSegments, segIdx, status;
    EMPTYXD(xd);
    struct descriptor_xd *retXd;
    EMPTYXD(emptyXd);
    EMPTYXD(dimXd);
    struct descriptor_xd *xds;
    struct descriptor **dscPtrs;
    struct descriptor_a *arrD;
//    DESCRIPTOR_APD(apd, DTYPE_DSC, 0, 0);
    DESCRIPTOR_APD(apd, DTYPE_L, 0, 0);
    char *fullPath;

    int numSlices, sliceIdx, currSliceIdx;
    int startSegId, endSegId;
    int *slicesPerSegment;
    EMPTYXD(startSegIdXd);
    EMPTYXD(endSegIdXd);
    char *currSlicePtr;
    int leftItems, leftRows;


    fullPath = malloc(strlen(path) + 11);
    if(expand)
    {
        if(isTimed)
    	    sprintf(fullPath, "%s/timed", path);
        else
            sprintf(fullPath, "%s/non_timed", path);
    }
    else //Used when retrieving object data from getCpoDataServer
    {
	strcpy(fullPath, path);
    }

    lock("mdsgetObjectLocal");
    checkExpIndex(expIdx);

    nid = mdsgetNid(cpoPath, fullPath);
    free(fullPath);
    if(nid == -1)
    {
	unlock();
	return -1;
    }
    status = TreeGetNumSegments(nid, &numSegments);
    if(!(status & 1))
    {
        sprintf(errmsg, "Error getting number of segments at path %s/%s ", cpoPath, path);
	unlock();
	return -1;
    }
    if(numSegments == 0)
    {
    	retXd = (struct descriptor_xd *)malloc(sizeof(struct descriptor_xd));
    	*retXd = emptyXd;
	status = TreeGetRecord(nid, retXd);
	if(!(status & 1))
	{
            sprintf(errmsg, "Error reading object at path %s/%s: %s ", cpoPath, path, MdsGetMsg(status));
	    unlock();
	    free((char *)retXd);
	    return -1;
    	}
	unlock();
	*obj = retXd;
	return 0;
    }

//Count first actual number of slices
    numSlices = 0;
    slicesPerSegment = (int *)malloc(numSegments * sizeof(int));
    for(segIdx = 0; segIdx < numSegments; segIdx++)
    {
	status = TreeGetSegmentLimits(nid, segIdx, &startSegIdXd, &endSegIdXd);
	if(!(status & 1))
	{
            sprintf(errmsg, "Error reading egment limits for object at path %s/%s: %s ", cpoPath, path, MdsGetMsg(status));
	    unlock();
	    free((char *)retXd);
	    return -1;
    	}
	startSegId = *(int *)(startSegIdXd.pointer->pointer);
	endSegId = *(int *)(endSegIdXd.pointer->pointer);
	MdsFree1Dx(&startSegIdXd, 0);
	MdsFree1Dx(&endSegIdXd, 0);
	slicesPerSegment[segIdx] = endSegId - startSegId + 1;
	numSlices += endSegId - startSegId + 1;
    }


    xds = (struct descriptor_xd *)malloc(sizeof(struct descriptor_xd) * numSlices);
    dscPtrs = (struct descriptor **)malloc(sizeof(struct descriptor *) * numSlices);
    for(segIdx = 0; segIdx < numSlices; segIdx++)
    	xds[segIdx] = emptyXd;

    sliceIdx = 0;
    for(segIdx = 0; segIdx < numSegments; segIdx++)
    {
        status = TreeGetSegment(nid, segIdx, &xd, &dimXd);
	MdsFree1Dx(&dimXd, 0);
	if(!(status & 1))
	{
            sprintf(errmsg, "Error reading object segment at path %s/%s: %s ", cpoPath, path, MdsGetMsg(status));
	    unlock();
	    return -1;
    	}
	if(!xd.pointer || xd.pointer->class != CLASS_A)
	{
            sprintf(errmsg, "Wrong segment data returned at path %s/%s", cpoPath, path);
	    unlock();
	    return -1;
    	}
	arrD = (struct descriptor_a *)xd.pointer;
	if(slicesPerSegment[segIdx] == 0) continue; //In case the segment sdoes not contain slices (replaceLastSegment)

	/* If it is the last segment and it contains only one slice, check if the segment is
	   completely filled. If yes, it is stored in the "traditional way" othewise the length
 	   of the serialized slice is stored in front
	*/
	if(segIdx == numSegments - 1 && slicesPerSegment[segIdx] == 1)
    	    getLeftItems(nid, &leftItems, &leftRows);
	else
	    leftRows = 0;

	if(slicesPerSegment[segIdx] == 1 && leftRows == 0)
//One slice in the segment, stored in the "traditional" way
	{
	    status = MdsSerializeDscIn(arrD->pointer, &xds[sliceIdx]);
	    MdsFree1Dx(&xd, 0);
    	    if(!(status & 1))
    	    {
        	printf("INTERNAL ERROR: Cannot deserialize data returned at path %s/%s: %s\n", cpoPath, path, MdsGetMsg(status));
		unlock();
		return -1;
    	    }
	    dscPtrs[sliceIdx] = xds[sliceIdx].pointer;
	    sliceIdx++;
	}
	else //Multiple slices per segment, the dimension (4 bytes) is stored before the serialized slice
	{
	    currSlicePtr = arrD->pointer;
	    for(currSliceIdx = 0; currSliceIdx < slicesPerSegment[segIdx]; currSliceIdx++)
	    {
		status = MdsSerializeDscIn(currSlicePtr + sizeof(int), &xds[sliceIdx]);
   	    	if(!(status & 1))
    	    	{
        	    printf("INTERNAL ERROR: Cannot deserialize data returned at path %s/%s: %s\n", cpoPath, path, MdsGetMsg(status));
		    unlock();
		    return -1;
    	    	}
	    	dscPtrs[sliceIdx] = xds[sliceIdx].pointer;
	    	sliceIdx++;
		currSlicePtr += *(int *)currSlicePtr + sizeof(int); //advance pointer to next serialized slice in segment
	    }
	    MdsFree1Dx(&xd, 0);
	}
    }
    apd.arsize = numSlices * sizeof(struct descriptor *);
    apd.pointer = (void *)dscPtrs;
    retXd = (struct descriptor_xd *)malloc(sizeof(struct descriptor_xd));
    *retXd = emptyXd;
    MdsCopyDxXd((struct descriptor *)&apd, retXd);
    for(segIdx = 0; segIdx < numSlices; segIdx++)
	MdsFree1Dx(&xds[segIdx], 0);
    free((char *)xds);
    free((char *)dscPtrs);
    free((char *)slicesPerSegment);
    unlock();
    *obj = retXd;
    return 0;
}

//Read the array of structures from the pulse file. Status indicates as always success (0) or error (!= 0)
int mdsGetObject(int expIdx, char *cpoPath, char *path, void **obj, int isTimed)
{
    int status;
    int dims[16];
    EMPTYXD(serializedXd);
    int exists;
    int cacheLevel = getCacheLevel(expIdx);
    if(cacheLevel > 0 && getInfoObject(expIdx, cpoPath, path, &exists, obj, isTimed))
    {
	if(exists)
	    return 0;
	else
	    return -1;
    }
    if(isExpRemote(expIdx))
        status = getObjectRemote(getExpConnectionId(expIdx), getExpRemoteIdx(expIdx), cpoPath, path, obj, isTimed);
    else
    	status = getObjectLocal(expIdx, cpoPath, path, obj, isTimed, 1);

//////////Memory info management///////////////
    if(cacheLevel > 0)
    {
    	if(!isTimed)
	    updateInfoObject(expIdx, cpoPath, path, !status, *obj);
        else
	    updateInfoTimedObject(expIdx, cpoPath, path, !status, *obj);
    }
////////////////////////////////////////////////
    return status;
}

static struct descriptor *getDataFromApd(struct descriptor_a *apd, char *prevName)
{
    char *name, *currName;
    int i;
    int numChildren = apd->arsize/apd->length - 1;

    name = strtok(NULL, "/");
    if(!name) //Last name, reached the end of the path
    {
	if(numChildren > 0)
	    return ((struct descriptor **)apd->pointer)[1];
    }
    else //find a child with that name
    {
	for(i = 0; i < numChildren; i++)
	{
	    if(((struct descriptor **)apd->pointer)[i+1])
	    {
		currName = getApdName(((struct descriptor_a **)apd->pointer)[i+1]);
		if(!strcmp(name, currName))
		{
		    free(currName);
		    break;
		}
		free(currName);
	    }
	}
	if(i == numChildren) //not found
	    return NULL;
	return getDataFromApd(((struct descriptor_a **)apd->pointer)[i+1], name);
    }
    return NULL;
}

static struct descriptor *getDataFromObject(void *obj, char *path, int idx)
{
    char *currPath, *name;
    struct descriptor_a *apd;
    struct descriptor_xd *xdPtr = (struct descriptor_xd *)obj;
    int numChildren;
    struct descriptor *retDsc;

    if(xdPtr->class == CLASS_XD)
	apd  = (struct descriptor_a *)xdPtr->pointer;
    else
	apd = (struct descriptor_a *)xdPtr;
    numChildren = apd->arsize/apd->length;
    if(idx >= numChildren) return NULL;
    if(!((struct descriptor **)apd->pointer)[idx]) return NULL;
    currPath = malloc(strlen(path) + 1);
    strcpy(currPath, path);
    //printf("in getDataFromObject, currPath = %s\n",currPath);
    lock("getDataFromObject"); //strtok is not thread safe
    name = strtok(currPath, "/");
   //printf("in getDataFromObject, name = %s, idx = %d\n",name, idx);
    retDsc = getDataFromApd(((struct descriptor_a **)apd->pointer)[idx], name);
    //printf("in getDataFromObject 3\n");
    unlock();
    free(currPath);
    return retDsc;
}




// //Retrieves components from array of structures. Returned status indicates success (0) or error(!= 0)
int mdsGetStringFromObject(void *obj, char *path, int idx, char **data)
{
    int status;
    struct descriptor_a *dataD = (struct descriptor_a *)getDataFromObject(obj, path, idx);

    if(!dataD) return -1;
    if(dataD->class != CLASS_A || dataD->dtype != DTYPE_BU)
    {
	sprintf(errmsg, "Internal error: unexpected data type in object at path %s ", path);
	return -1;
    }
    *data = malloc(dataD->arsize + 1);
    memcpy(*data, dataD->pointer, dataD->arsize);
    (*data)[dataD->arsize] = 0;
    return 0;
}

int mdsGetIntFromObject(void *obj, char *path, int idx, int *data)
{
    struct descriptor *dataD = getDataFromObject(obj, path, idx);

    if(!dataD) return -1;
    if(dataD->class != CLASS_S || dataD->dtype != DTYPE_L)
    {
	sprintf(errmsg, "Internal error: unexpected data type in object at path %s ", path);
	return -1;
    }
    *data = *(int *)dataD->pointer;
    return 0;
}

int mdsGetFloatFromObject(void *obj, char *path, int idx, float *data)
{
    struct descriptor *dataD = getDataFromObject(obj, path, idx);

    if(!dataD) return -1;
    if(dataD->class != CLASS_S || dataD->dtype != DTYPE_FLOAT)
    {
	sprintf(errmsg, "Internal error: unexpected data type in object at path %s ", path);
	return -1;
    }
    *data = *(float *)dataD->pointer;
    return 0;
}

int mdsGetDoubleFromObject(void *obj, char *path, int idx, double *data)
{
    struct descriptor *dataD = getDataFromObject(obj, path, idx);

    if(!dataD) return -1;
    if(dataD->class != CLASS_S || (dataD->dtype != DTYPE_DOUBLE && dataD->dtype != DTYPE_FT))
    {
	sprintf(errmsg, "Internal error: unexpected data type in object at path %s ", path);
	return -1;
    }
    *data = *(double *)dataD->pointer;
    return 0;
}

int mdsGetVect1DStringFromObject(void *obj, char *path, int idx, char  ***data, int *dim)
{
    int nItems, i;
    char **retData;
    struct descriptor_a *dataD = (struct descriptor_a *)getDataFromObject(obj, path, idx);

    if(!dataD) return -1;
    if(dataD->class != CLASS_A || dataD->dtype != DTYPE_T)
    {
	sprintf(errmsg, "Internal error: unexpected data type in object at path %s ", path);
        return -1;
    }
    if(dataD->length == 0)
    {
	return -1;
    }
    nItems = dataD->arsize/dataD->length;
    retData = (char **)malloc(sizeof(char *) * nItems);
    for(i = 0; i < nItems; i++)
    {
    	retData[i] = malloc(dataD->length + 1);
        memcpy(retData[i], &dataD->pointer[i * dataD->length], dataD->length);
        retData[i][dataD->length] = 0;
     }
    *data = retData;
    *dim = nItems;
    return 0;
}


int mdsGetVect1DIntFromObject(void *obj, char *path, int idx, int **data, int *dim)
{
   ARRAY_COEFF(char *, 7) *arrD = (void *)getDataFromObject(obj, path, idx);

    if(!arrD) return -1;
    if(arrD->class != CLASS_A || arrD->dtype != DTYPE_L)
    {
	sprintf(errmsg, "Internal error: unexpected data type in object at path %s ", path);
	return -1;
    }
    else
    {
     	*dim = arrD->m[0];
	*data = (int *)malloc(arrD->arsize);
	memcpy(*data, arrD->pointer, arrD->arsize);
	return 0;
    }
}


int mdsGetVect1DFloatFromObject(void *obj, char *path, int idx, float **data, int *dim)
{
   ARRAY_COEFF(char *, 7) *arrD = (void *)getDataFromObject(obj, path, idx);

    if(!arrD) return -1;
    if(arrD->class != CLASS_A || arrD->dtype != DTYPE_FLOAT)
    {
	sprintf(errmsg, "Internal error: unexpected data type in object at path %s ", path);
	return -1;
    }
    else
    {
     	*dim = arrD->m[0];
	*data = (float *)malloc(arrD->arsize);
	memcpy(*data, arrD->pointer, arrD->arsize);
	return 0;
    }
}

int mdsGetVect1DDoubleFromObject(void *obj, char *path, int idx, double **data, int *dim)
{
   ARRAY_COEFF(char *, 7) *arrD = (void *)getDataFromObject(obj, path, idx);

    if(!arrD) return -1;
    if(arrD->class != CLASS_A || (arrD->dtype != DTYPE_DOUBLE && arrD->dtype != DTYPE_FT))
    {
	sprintf(errmsg, "Internal error: unexpected data type in object at path %s ", path);
	return -1;
    }
    else
    {
     	*dim = arrD->m[0];
	*data = (double *)malloc(arrD->arsize);
	memcpy(*data, arrD->pointer, arrD->arsize);
	return 0;
    }
}

int mdsGetVect2DIntFromObject(void *obj, char *path, int idx, int **data, int *dim1, int *dim2)
{
   ARRAY_COEFF(char *, 7) *arrD = (void *)getDataFromObject(obj, path, idx);

    if(!arrD) return -1;
    if(arrD->class != CLASS_A || arrD->dtype != DTYPE_L)
    {
	sprintf(errmsg, "Internal error: unexpected data type in object at path %s ", path);
	return -1;
    }
    else
    {
     	*dim1 = arrD->m[0];
     	*dim2 = arrD->m[1];
	*data = (int *)malloc(arrD->arsize);
	memcpy(*data, arrD->pointer, arrD->arsize);
	return 0;
    }
}

int mdsGetVect2DFloatFromObject(void *obj, char *path, int idx, float **data, int *dim1, int *dim2)
{
   ARRAY_COEFF(char *, 7) *arrD = (void *)getDataFromObject(obj, path, idx);

    if(!arrD) return -1;
    if(arrD->class != CLASS_A || arrD->dtype != DTYPE_FLOAT)
    {
	sprintf(errmsg, "Internal error: unexpected data type in object at path %s ", path);
	return -1;
    }
    else
    {
     	*dim1 = arrD->m[0];
     	*dim2 = arrD->m[1];
	*data = (float *)malloc(arrD->arsize);
	memcpy(*data, arrD->pointer, arrD->arsize);
	return 0;
    }
}

int mdsGetVect2DDoubleFromObject(void *obj, char *path, int idx, double **data, int *dim1, int *dim2)
{
   ARRAY_COEFF(char *, 7) *arrD = (void *)getDataFromObject(obj, path, idx);

    if(!arrD) return -1;
    if(arrD->class != CLASS_A || (arrD->dtype != DTYPE_DOUBLE && arrD->dtype != DTYPE_FT))
    {
	sprintf(errmsg, "Internal error: unexpected data type in object at path %s ", path);
	return -1;
    }
    else
    {
     	*dim1 = arrD->m[0];
     	*dim2 = arrD->m[1];
	*data = (double *)malloc(arrD->arsize);
	memcpy(*data, arrD->pointer, arrD->arsize);
	return 0;
    }
}

int mdsGetVect3DIntFromObject(void *obj, char *path, int idx, int **data, int *dim1, int *dim2, int *dim3)
{
   ARRAY_COEFF(char *, 7) *arrD = (void *)getDataFromObject(obj, path, idx);

    if(!arrD) return -1;
    if(arrD->class != CLASS_A || arrD->dtype != DTYPE_L)
    {
	sprintf(errmsg, "Internal error: unexpected data type in object at path %s ", path);
	return -1;
    }
    else
    {
     	*dim1 = arrD->m[0];
     	*dim2 = arrD->m[1];
     	*dim3 = arrD->m[2];
	*data = (int *)malloc(arrD->arsize);
	memcpy(*data, arrD->pointer, arrD->arsize);
	return 0;
    }
}

int mdsGetVect3DFloatFromObject(void *obj, char *path, int idx, float **data, int *dim1, int *dim2, int *dim3)
{
   ARRAY_COEFF(char *, 7) *arrD = (void *)getDataFromObject(obj, path, idx);

    if(!arrD) return -1;
    if(arrD->class != CLASS_A || arrD->dtype != DTYPE_FLOAT)
    {
	sprintf(errmsg, "Internal error: unexpected data type in object at path %s ", path);
	return -1;
    }
    else
    {
     	*dim1 = arrD->m[0];
     	*dim2 = arrD->m[1];
     	*dim3 = arrD->m[2];
	*data = (float *)malloc(arrD->arsize);
	memcpy(*data, arrD->pointer, arrD->arsize);
	return 0;
    }
}

int mdsGetVect3DDoubleFromObject(void *obj, char *path, int idx, double **data, int *dim1, int *dim2, int *dim3)
{
   ARRAY_COEFF(char *, 7) *arrD = (void *)getDataFromObject(obj, path, idx);

    if(!arrD) return -1;
    if(arrD->class != CLASS_A || (arrD->dtype != DTYPE_DOUBLE && arrD->dtype != DTYPE_FT))
    {
	sprintf(errmsg, "Internal error: unexpected data type in object at path %s ", path);
	return -1;
    }
    else
    {
     	*dim1 = arrD->m[0];
     	*dim2 = arrD->m[1];
     	*dim3 = arrD->m[2];
	*data = (double *)malloc(arrD->arsize);
	memcpy(*data, arrD->pointer, arrD->arsize);
	return 0;
    }
}

int mdsGetVect4DIntFromObject(void *obj, char *path, int idx, int **data, int *dim1, int *dim2, int *dim3, int *dim4)
{
   ARRAY_COEFF(char *, 7) *arrD = (void *)getDataFromObject(obj, path, idx);

    if(!arrD) return -1;
    if(arrD->class != CLASS_A || arrD->dtype != DTYPE_L)
    {
	sprintf(errmsg, "Internal error: unexpected data type in object at path %s ", path);
	return -1;
    }
    else
    {
     	*dim1 = arrD->m[0];
     	*dim2 = arrD->m[1];
     	*dim3 = arrD->m[2];
     	*dim4 = arrD->m[3];
	*data = (int *)malloc(arrD->arsize);
	memcpy(*data, arrD->pointer, arrD->arsize);
	return 0;
    }
}

int mdsGetVect4DFloatFromObject(void *obj, char *path, int idx, float **data, int *dim1, int *dim2, int *dim3, int *dim4)
{
   ARRAY_COEFF(char *, 7) *arrD = (void *)getDataFromObject(obj, path, idx);

    if(!arrD) return -1;
    if(arrD->class != CLASS_A || arrD->dtype != DTYPE_FLOAT)
    {
	sprintf(errmsg, "Internal error: unexpected data type in object at path %s ", path);
	return -1;
    }
    else
    {
     	*dim1 = arrD->m[0];
     	*dim2 = arrD->m[1];
     	*dim3 = arrD->m[2];
     	*dim4 = arrD->m[3];
	*data = (float *)malloc(arrD->arsize);
	memcpy(*data, arrD->pointer, arrD->arsize);
	return 0;
    }
}

int mdsGetVect4DDoubleFromObject(void *obj, char *path, int idx, double **data, int *dim1, int *dim2, int *dim3, int *dim4)
{
   ARRAY_COEFF(char *, 7) *arrD = (void *)getDataFromObject(obj, path, idx);

    if(!arrD) return -1;
    if(arrD->class != CLASS_A || (arrD->dtype != DTYPE_DOUBLE && arrD->dtype != DTYPE_FT))
    {
	sprintf(errmsg, "Internal error: unexpected data type in object at path %s ", path);
	return -1;
    }
    else
    {
     	*dim1 = arrD->m[0];
     	*dim2 = arrD->m[1];
     	*dim3 = arrD->m[2];
     	*dim4 = arrD->m[3];
	*data = (double *)malloc(arrD->arsize);
	memcpy(*data, arrD->pointer, arrD->arsize);
	return 0;
    }
}

int mdsGetVect5DIntFromObject(void *obj, char *path, int idx, int **data, int *dim1, int *dim2, int *dim3, int *dim4, int *dim5)
{
   ARRAY_COEFF(char *, 7) *arrD = (void *)getDataFromObject(obj, path, idx);

    if(!arrD) return -1;
    if(arrD->class != CLASS_A || arrD->dtype != DTYPE_L)
    {
	sprintf(errmsg, "Internal error: unexpected data type in object at path %s ", path);
	return -1;
    }
    else
    {
     	*dim1 = arrD->m[0];
     	*dim2 = arrD->m[1];
     	*dim3 = arrD->m[2];
     	*dim4 = arrD->m[3];
     	*dim5 = arrD->m[4];
	*data = (int *)malloc(arrD->arsize);
	memcpy(*data, arrD->pointer, arrD->arsize);
	return 0;
    }
}

int mdsGetVect5DFloatFromObject(void *obj, char *path, int idx, float **data, int *dim1, int *dim2, int *dim3, int *dim4, int *dim5)
{
   ARRAY_COEFF(char *, 7) *arrD = (void *)getDataFromObject(obj, path, idx);

    if(!arrD) return -1;
    if(arrD->class != CLASS_A || arrD->dtype != DTYPE_FLOAT)
    {
	sprintf(errmsg, "Internal error: unexpected data type in object at path %s ", path);
	return -1;
    }
    else
    {
     	*dim1 = arrD->m[0];
     	*dim2 = arrD->m[1];
     	*dim3 = arrD->m[2];
     	*dim4 = arrD->m[3];
     	*dim5 = arrD->m[4];
	*data = (float *)malloc(arrD->arsize);
	memcpy(*data, arrD->pointer, arrD->arsize);
	return 0;
    }
}

int mdsGetVect5DDoubleFromObject(void *obj, char *path, int idx, double **data, int *dim1, int *dim2, int *dim3, int *dim4, int *dim5)
{
   ARRAY_COEFF(char *, 7) *arrD = (void *)getDataFromObject(obj, path, idx);

    if(!arrD) return -1;
    if(arrD->class != CLASS_A || (arrD->dtype != DTYPE_DOUBLE && arrD->dtype != DTYPE_FT))
    {
	sprintf(errmsg, "Internal error: unexpected data type in object at path %s ", path);
	return -1;
    }
    else
    {
     	*dim1 = arrD->m[0];
     	*dim2 = arrD->m[1];
     	*dim3 = arrD->m[2];
     	*dim4 = arrD->m[3];
     	*dim5 = arrD->m[4];
	*data = (double *)malloc(arrD->arsize);
	memcpy(*data, arrD->pointer, arrD->arsize);
	return 0;
    }
}

int mdsGetVect6DIntFromObject(void *obj, char *path, int idx, int **data, int *dim1, int *dim2, int *dim3, int *dim4, int *dim5, int *dim6)
{
   ARRAY_COEFF(char *, 7) *arrD = (void *)getDataFromObject(obj, path, idx);

    if(!arrD) return -1;
    if(arrD->class != CLASS_A || arrD->dtype != DTYPE_L)
    {
	sprintf(errmsg, "Internal error: unexpected data type in object at path %s ", path);
	return -1;
    }
    else
    {
     	*dim1 = arrD->m[0];
     	*dim2 = arrD->m[1];
     	*dim3 = arrD->m[2];
     	*dim4 = arrD->m[3];
     	*dim5 = arrD->m[4];
     	*dim6 = arrD->m[5];
	*data = (int *)malloc(arrD->arsize);
	memcpy(*data, arrD->pointer, arrD->arsize);
	return 0;
    }
}

int mdsGetVect6DFloatFromObject(void *obj, char *path, int idx, float **data, int *dim1, int *dim2, int *dim3, int *dim4, int *dim5, int *dim6)
{
   ARRAY_COEFF(char *, 7) *arrD = (void *)getDataFromObject(obj, path, idx);

    if(!arrD) return -1;
    if(arrD->class != CLASS_A || arrD->dtype != DTYPE_FLOAT)
    {
	sprintf(errmsg, "Internal error: unexpected data type in object at path %s ", path);
	return -1;
    }
    else
    {
     	*dim1 = arrD->m[0];
     	*dim2 = arrD->m[1];
     	*dim3 = arrD->m[2];
     	*dim4 = arrD->m[3];
     	*dim5 = arrD->m[4];
     	*dim6 = arrD->m[5];
	*data = (float *)malloc(arrD->arsize);
	memcpy(*data, arrD->pointer, arrD->arsize);
	return 0;
    }
}

int mdsGetVect6DDoubleFromObject(void *obj, char *path, int idx, double **data, int *dim1, int *dim2, int *dim3, int *dim4, int *dim5, int *dim6)
{
   ARRAY_COEFF(char *, 7) *arrD = (void *)getDataFromObject(obj, path, idx);
    if(!arrD) return -1;
    if(arrD->class != CLASS_A || (arrD->dtype != DTYPE_DOUBLE && arrD->dtype != DTYPE_FT))
    {
	sprintf(errmsg, "Internal error: unexpected data type in object at path %s ", path);
	return -1;
    }
    else
    {
     	*dim1 = arrD->m[0];
     	*dim2 = arrD->m[1];
     	*dim3 = arrD->m[2];
     	*dim4 = arrD->m[3];
     	*dim5 = arrD->m[4];
     	*dim6 = arrD->m[5];
	*data = (double *)malloc(arrD->arsize);
	memcpy(*data, arrD->pointer, arrD->arsize);

	return 0;
    }
}

int mdsGetVect7DIntFromObject(void *obj, char *path, int idx, int **data, int *dim1, int *dim2, int *dim3, int *dim4, int *dim5, int *dim6, int *dim7)
{
   ARRAY_COEFF(char *, 7) *arrD = (void *)getDataFromObject(obj, path, idx);

    if(!arrD) return -1;
    if(arrD->class != CLASS_A || arrD->dtype != DTYPE_L)
    {
	sprintf(errmsg, "Internal error: unexpected data type in object at path %s ", path);
	return -1;
    }
    else
    {
     	*dim1 = arrD->m[0];
     	*dim2 = arrD->m[1];
     	*dim3 = arrD->m[2];
     	*dim4 = arrD->m[3];
     	*dim5 = arrD->m[4];
     	*dim6 = arrD->m[5];
     	*dim7 = arrD->m[6];
	*data = (int *)malloc(arrD->arsize);
	memcpy(*data, arrD->pointer, arrD->arsize);
	return 0;
    }
}

int mdsGetVect7DFloatFromObject(void *obj, char *path, int idx, float **data, int *dim1, int *dim2, int *dim3, int *dim4, int *dim5, int *dim6, int *dim7)
{
   ARRAY_COEFF(char *, 7) *arrD = (void *)getDataFromObject(obj, path, idx);

    if(!arrD) return -1;
    if(arrD->class != CLASS_A || arrD->dtype != DTYPE_FLOAT)
    {
	sprintf(errmsg, "Internal error: unexpected data type in object at path %s ", path);
	return -1;
    }
    else
    {
     	*dim1 = arrD->m[0];
     	*dim2 = arrD->m[1];
     	*dim3 = arrD->m[2];
     	*dim4 = arrD->m[3];
     	*dim5 = arrD->m[4];
     	*dim6 = arrD->m[5];
     	*dim7 = arrD->m[6];
	*data = (float *)malloc(arrD->arsize);
	memcpy(*data, arrD->pointer, arrD->arsize);
	return 0;
    }
}

int mdsGetVect7DDoubleFromObject(void *obj, char *path, int idx, double **data, int *dim1, int *dim2, int *dim3, int *dim4, int *dim5, int *dim6, int *dim7)
{
   ARRAY_COEFF(char *, 7) *arrD = (void *)getDataFromObject(obj, path, idx);

    if(!arrD) return -1;
    if(arrD->class != CLASS_A || (arrD->dtype != DTYPE_DOUBLE && arrD->dtype != DTYPE_FT))
    {
	sprintf(errmsg, "Internal error: unexpected data type in object at path %s ", path);
	return -1;
    }
    else
    {
     	*dim1 = arrD->m[0];
     	*dim2 = arrD->m[1];
     	*dim3 = arrD->m[2];
     	*dim4 = arrD->m[3];
     	*dim5 = arrD->m[4];
     	*dim6 = arrD->m[5];
     	*dim7 = arrD->m[6];
	*data = (double *)malloc(arrD->arsize);
	memcpy(*data, arrD->pointer, arrD->arsize);
	return 0;
    }
}

int mdsGetObjectFromObject(void *obj, char *path, int idx, void **dataObj)
{
    *dataObj = (void *)getDataFromObject(obj, path, idx);
    if(!*dataObj) return -1;
    return 0;
}
int mdsGetDimensionFromObject(int expIdx, void *obj, char *path, int idx, int *numDims, int *dim1, int *dim2, int *dim3, int *dim4, int *dim5, int *dim6, int *dim7)
{
    struct descriptor *dscPtr;
    ARRAY_COEFF(char *, 7) *arrDPtr;
    int i;
    *dim1 = *dim2 = *dim3 = *dim4 = *dim5 = *dim6 = *dim7 = 0;


    //printf("in mdsGetDimensionFromObject, path = %s, idx =%d\n",path, idx);
    dscPtr = getDataFromObject(obj, path, idx);
    //printf("in mdsGetDimensionFromObject 2\n");

    if(!dscPtr) return -1;
    if(dscPtr->class == CLASS_S)
    {
        *numDims = 0;
        return 0;
    }
    else //CLASS_A
    {
        arrDPtr = (void *)dscPtr;
        *numDims = arrDPtr->dimct;
        if(arrDPtr->dimct >= 1)
            *dim1 = arrDPtr->m[0];
        if(arrDPtr->dimct >= 2)
            *dim2 = arrDPtr->m[1];
        if(arrDPtr->dimct >= 3)
            *dim3 = arrDPtr->m[2];
        if(arrDPtr->dimct >= 4)
            *dim4 = arrDPtr->m[3];
        if(arrDPtr->dimct >= 5)
            *dim5 = arrDPtr->m[4];
        if(arrDPtr->dimct >= 6)
            *dim6 = arrDPtr->m[5];
        if(arrDPtr->dimct >= 7)
            *dim7 = arrDPtr->m[6];
    }
    return 0;
}

int mdsGetObjectSlice(int expIdx, char *cpoPath, char *path,  double time, void **obj)
{
    if(isExpRemote(expIdx))
        return getObjectSliceRemote(getExpConnectionId(expIdx), getExpRemoteIdx(expIdx), cpoPath, path, time, obj);
    else
        return getObjectSliceLocal(expIdx, cpoPath, path, time, obj, 1);
}


//Array of structures Slice Management
//NOTE For the moment only local data access is supported
static int getObjectSliceLocalOLD(int expIdx, char *cpoPath, char *path,  double time, void **obj, int expand)
{
    double *times;
    int nTimes;
    int status, i, segIdx, nid;
    char *fullPath;
    EMPTYXD(xd);
    EMPTYXD(deserializedXd);
    EMPTYXD(emptyXd);
    EMPTYXD(dimXd);
    struct descriptor_a *arrD;
    DESCRIPTOR_APD(retApd, DTYPE_L, 0, 0);
    struct descriptor_xd *retXd;
    int cacheLevel;
    int exists;
    int nDims, dims[16];
    void *tempObj;

    status = mdsGetVect1DDouble(expIdx, cpoPath, "time", &times, &nTimes);
    if(status) return status;
//Find Idx
    if(time <= times[0])
	segIdx = 0;
    else if (time >= times[nTimes - 1])
	segIdx = nTimes - 1;
    else
    {
    	for(i = segIdx = 0; i < nTimes - 1; i++)
    	{
	    if(times[i] <= time && times[i+1] >= time) //Closest sample
	    {
	    	if(time - times[i] < times[i+1] - time)
	   	    segIdx = i;
	    	else
		    segIdx = i+1;
	    	break;
	    }
	}

    }
    free((char *)times);
    fullPath = malloc(strlen(path) + 7);
    if(expand)
    	sprintf(fullPath, "%s/timed", path);
    else
   	sprintf(fullPath, "%s", path);

/* Check Cache. Only for IN THIS CASE this is performed within "local" routine */

    cacheLevel = getCacheLevel(expIdx);
    if(cacheLevel > 0 && getInfoObjectSlice(expIdx, cpoPath, fullPath, segIdx, &exists, obj))
    {
    	free(fullPath);
	if(exists)
	    return 0;
	else
	    return -1;
    }

    lock("getObjectSliceLocal");
    checkExpIndex(expIdx);
        nid = getNid(cpoPath, path);
    free(fullPath);
    if(nid == -1)
    {
	unlock();
	return -1;
    }

    status = TreeGetSegment(nid, segIdx, &xd, &dimXd);
    MdsFree1Dx(&dimXd, 0);
    if(!(status & 1))
    {
        sprintf(errmsg, "Error reading object segment at path %s/%s: %s ", path, cpoPath, MdsGetMsg(status));
	unlock();
	return -1;
    }
    if(!xd.pointer || xd.pointer->class != CLASS_A)
    {
        sprintf(errmsg, "Wrong segment data returned at path %s/%s", path, cpoPath);
	unlock();
	return -1;
    }
    arrD = (struct descriptor_a *)xd.pointer;
    retXd = (struct descriptor_xd *)malloc(sizeof(struct descriptor_xd));
    *retXd = emptyXd;
    status = MdsSerializeDscIn(arrD->pointer, &deserializedXd);
    MdsFree1Dx(&xd, 0);
    if(!(status & 1))
    {
    	printf("INTERNAL ERROR: Cannot deserialize data eturned at path %s/%s: %s\n", path, cpoPath, MdsGetMsg(status));
	unlock();
	return -1;
    }
    retApd.arsize = retApd.length = sizeof(struct descriptor *);
    retApd.pointer = &deserializedXd.pointer;
    status = MdsCopyDxXd((struct descriptor *)&retApd, retXd);
    MdsFree1Dx(&deserializedXd, 0);
    *obj = retXd;
    unlock();
    return 0;
}

//Array of structures Slice Management
//NOTE For the moment only local data access is supported
static int getObjectSliceLocal(int expIdx, char *cpoPath, char *path,  double time, void **obj, int expand)
{
    double *times;
    int nTimes;
    int status, i, sliceIdx, nid;
    char *fullPath;
    char *fullPathTime;
    EMPTYXD(xd);
    EMPTYXD(deserializedXd);
    EMPTYXD(emptyXd);
    EMPTYXD(dimXd);
    struct descriptor_a *arrD;
    DESCRIPTOR_APD(retApd, DTYPE_L, 0, 0);
    struct descriptor_xd *retXd;
    int cacheLevel;
    int exists;
    int nDims, dims[16];
    void *tempObj;
    int numSegments, actSegmentIdx, segStartIdx, segEndIdx;
    EMPTYXD(segStartXd);
    EMPTYXD(segEndXd);
    char *objectPtr, *multiObjectPtr;
    int currOffset, leftItems, leftRows;

    fullPathTime = malloc(strlen(path) + 7);
    if(expand)
    	sprintf(fullPathTime, "%s/time", path);
    else
   	sprintf(fullPathTime, "%s", path);

   // printf("fullPathTime = %s\n",fullPathTime);

    status = mdsGetVect1DDouble(expIdx, cpoPath, fullPathTime, &times, &nTimes); // Changed w.r.t. to ITM: read the (user hidden) time array of the type 3 AoS
    if(status) return status;
//Find Idx
    if(time <= times[0])
	sliceIdx = 0;
    else if (time >= times[nTimes - 1])
	sliceIdx = nTimes - 1;
    else
    {
    	for(i = sliceIdx = 0; i < nTimes - 1; i++)
    	{
	    if(times[i] <= time && times[i+1] >= time) //Closest sample
	    {
	    	if(time - times[i] < times[i+1] - time)
	   	    sliceIdx = i;
	    	else
		    sliceIdx = i+1;
	    	break;
	    }
	}

    }

   // printf("sliceIdx = %d\n",sliceIdx);

    free((char *)times);
    fullPath = malloc(strlen(path) + 7);
    if(expand)
    	sprintf(fullPath, "%s/timed", path);
    else
   	sprintf(fullPath, "%s", path);

   // printf("fullPath = %s\n",fullPath);

/* Check Cache. Only for IN THIS CASE this is performed within "local" routine */

    cacheLevel = getCacheLevel(expIdx);
    if(cacheLevel > 0 && getInfoObjectSlice(expIdx, cpoPath, fullPath, sliceIdx, &exists, obj))
    {
    	free(fullPath);
	if(exists)
	    return 0;
	else
	    return -1;
    }

    lock("getObjectSliceLocal");
    checkExpIndex(expIdx);
    nid = getNid(cpoPath, fullPath);
    free(fullPath);
    if(nid == -1)
    {
	unlock();
	return -1;
    }
/***********Change single TreeGetSegment with the search of the right segment *************
    status = TreeGetSegment(nid, segIdx, &xd, &dimXd);

    MdsFree1Dx(&dimXd, 0);
    if(!(status & 1))
    {
        sprintf(errmsg, "Error reading object segment at path %s/%s: %s ", path, cpoPath, MdsGetMsg(status));
	unlock();
	return -1;
    }
    if(!xd.pointer || xd.pointer->class != CLASS_A)
    {
        sprintf(errmsg, "Wrong segment data returned at path %s/%s", path, cpoPath);
	unlock();
	return -1;
    }
    arrD = (struct descriptor_a *)xd.pointer;
    retXd = (struct descriptor_xd *)malloc(sizeof(struct descriptor_xd));
    *retXd = emptyXd;
    status = MdsSerializeDscIn(arrD->pointer, &deserializedXd);
*******************************************************************************************/
    status = TreeGetNumSegments(nid, &numSegments);
    if(!(status & 1))
    {
        sprintf(errmsg, "Error reading num object segment at path %s/%s: %s ", path, cpoPath, MdsGetMsg(status));
	unlock();
	return -1;
    }

    //printf("Here in low level 1\n");

    if(numSegments == 0)
    {
        sprintf(errmsg, "Missing data at path %s/%s", cpoPath, path);
        printf("ERROR %s\n",errmsg);

	unlock();
	return -1;
    }

    for(actSegmentIdx = numSegments - 1; actSegmentIdx >= 0; actSegmentIdx--)
    {
	status = TreeGetSegmentLimits(nid, actSegmentIdx, &segStartXd, &segEndXd);
    	if(!(status & 1))
    	{
            sprintf(errmsg, "Error reading num object segment at path %s/%s: %s ", cpoPath, path, MdsGetMsg(status));
	    unlock();
        printf("ERROR %s\n",errmsg);

	    return -1;
    	}
	segStartIdx = *((int *)segStartXd.pointer->pointer);
	segEndIdx = *((int *)segEndXd.pointer->pointer);
	MdsFree1Dx(&segStartXd, 0);
	MdsFree1Dx(&segEndXd, 0);
	if(segEndIdx < segStartIdx) //This happens only for a replace last slice in case a segmment with one struct array is dismissed
	    continue;
	if(sliceIdx >= segStartIdx && sliceIdx <= segEndIdx)
	    break;
    }
    //printf("Here in low level 2\n");

    if(actSegmentIdx < 0)
    {
        sprintf(errmsg, "INTERNAL ERROR in getObjectSliceLocal: segment not found");
        printf("%s\n",errmsg);

	unlock();
	return -1;
    }
    status = TreeGetSegment(nid, actSegmentIdx, &xd, &dimXd);
    if(!(status & 1))
    {
        sprintf(errmsg, "Error reading object segment at path %s/%s: %s ", path, cpoPath, MdsGetMsg(status));
        printf("%s\n",errmsg);
	unlock();
	return -1;
    }
    MdsFree1Dx(&dimXd, 0);
    if(!xd.pointer || xd.pointer->class != CLASS_A)
    {
        sprintf(errmsg, "Wrong segment data returned at path %s/%s", path, cpoPath);
        printf("%s\n",errmsg);
	unlock();
	return -1;
    }
    //printf("Here in low level 3\n");

/*If segStartIdx == segEndIdx  and the segment is not the last one, than it will contain for sure
one slice ine the traditional way. If it is the last segment, segment may either contain one slice
in traditional way or only one of multiple slices (i.e. the last slice) */
    if(actSegmentIdx == numSegments - 1)
    	getLeftItems(nid, &leftItems, &leftRows);
    else
	leftRows = 0;
    if(segStartIdx == segEndIdx && leftRows == 0) //the segment contains only that slice in traditional way
    {
	objectPtr = ((struct descriptor_a *)xd.pointer)->pointer;
    }
    else
    {
	multiObjectPtr = ((struct descriptor_a *)xd.pointer)->pointer;
	currOffset = 0;
	for(i = 0; i < sliceIdx - segStartIdx; i++)
	    currOffset += *((int *)(&multiObjectPtr[currOffset])) + sizeof(int);
	currOffset += sizeof(int);
	objectPtr = &multiObjectPtr[currOffset];
    }

    //printf("Here in low level 4\n");


/*******************************************************************************************/

    retXd = (struct descriptor_xd *)malloc(sizeof(struct descriptor_xd));
    *retXd = emptyXd;
//    status = MdsSerializeDscIn(arrD->pointer, &deserializedXd);
    status = MdsSerializeDscIn(objectPtr, &deserializedXd);
    MdsFree1Dx(&xd, 0);
    if(!(status & 1))
    {
    	printf("INTERNAL ERROR: Cannot deserialize data eturned at path %s/%s: %s\n", path, cpoPath, MdsGetMsg(status));
	unlock();
	return -1;
    }
    retApd.arsize = retApd.length = sizeof(struct descriptor *);
    retApd.pointer = &deserializedXd.pointer;
    status = MdsCopyDxXd((struct descriptor *)&retApd, retXd);
    MdsFree1Dx(&deserializedXd, 0);
    *obj = retXd;
    unlock();
    return 0;
}


int mdsPutObjectSlice(int expIdx, char *cpoPath, char *path, double time, void *obj)
{
    char *fullPath;
    int status;
    int cacheLevel = getCacheLevel(expIdx);
    int nDims, dims[16];
    void *tempObj;
    int exists;

    fullPath = malloc(strlen(path) + 7);
    sprintf(fullPath, "%s/timed", path);
    if(cacheLevel > 0)
    {
//First get Data, if any so that it is correctly posted in the memory cache
    	if(!getInfo(expIdx, cpoPath, fullPath, &exists, &nDims, dims))
    	{
	    status = mdsGetObject(expIdx, cpoPath, path, &tempObj, 1);
	    if(!status)
	        releaseObject(tempObj);
        }
	updateInfoObjectSlice(expIdx, cpoPath, path, obj);
        if(cacheLevel == 2)
	{
	    free(fullPath);
    	    releaseObject(obj);
    	    return 0;
	}
    }
    status = putObjectSegment(expIdx, cpoPath, fullPath, obj, -1);

    releaseObject(obj);
    free(fullPath);
    return status;
}


int mdsReplaceLastObjectSlice(int expIdx, char *cpoPath, char *path, void *obj)
{
   char *fullPath;
    int nid, status, numSegments;

    fullPath = malloc(strlen(path) + 7);
    sprintf(fullPath, "%s/timed", path);

    status = putObjectSegment(expIdx, cpoPath, fullPath, obj, 0);
    releaseObject(obj);
    free(fullPath);
    return status;
}

static char *decompileDsc(void *ptr)
{
    int status;
	EMPTYXD(xd);
	char *buf;
	struct descriptor *dscPtr = (struct descriptor *)ptr;

	status = TdiDecompile(dscPtr, &xd MDS_END_ARG);
	if(!(status & 1))
    {
      printf("Error decompiling expression: %s\n", MdsGetMsg(status));
      return NULL;
    }

	dscPtr = xd.pointer;
	buf = (char *)malloc(dscPtr->length + 1);
	memcpy(buf, dscPtr->pointer, dscPtr->length);
	buf[dscPtr->length] = 0;
	MdsFree1Dx(&xd, NULL);
	return buf;
}

static void dumpApd(struct descriptor_a *apd, int spaces)
{
    char *name, *decompiled;
    int i;
    struct descriptor *currDsc;
    int numChildren = apd->arsize/apd->length;
//Check if this element is in turn an object, i.e. the first descriptior is an APD
    if(numChildren == 0) return;
    if(((struct descriptor **)apd->pointer)[0]->class == CLASS_APD)
    {
	dumpObjElement(apd, spaces + 2);
	return;
    }
    name = getApdName(apd);
    for(i = 0; i < spaces; i++)
	printf(" ");
    printf("%s: ", name);
    free(name);
    if(numChildren == 1) return;
    if(((struct descriptor **)apd->pointer)[1]->class == CLASS_APD) //Non leaf Object node
    {
	printf("\n");
	for(i = 0; i < numChildren - 1; i++)
	    dumpApd(((struct descriptor_a **)apd->pointer)[i+1], spaces + 2);
    }
    else
    {
	currDsc = ((struct descriptor **)apd->pointer)[1];
	if(currDsc->class == CLASS_A && currDsc->dtype == DTYPE_BU) //UAL String representation
	{
	    struct descriptor_a *currArr = (struct descriptor_a *)currDsc;
	    char *currName = malloc(currArr->arsize + 1);
	    memcpy(currName, currArr->pointer, currArr->arsize);
	    currName[currArr->arsize] = 0;
	    printf("\"%s\"\n", currName);
	    free(currName);
	}
	else
	{
	    decompiled = decompileDsc(((struct descriptor **)apd->pointer)[1]);
	    printf("%s\n", decompiled);
	    free(decompiled);
	}
    }
}

static void dumpObjElement(struct descriptor_a *apd, int numSpaces)
{
    int i, j;
    int numChildren = apd->arsize/apd->length;
    for(i = 0; i < numChildren; i++)
    {
	for(j = 0; j < numSpaces; j++)
	    printf(" ");
	printf("%d:\n", i);
	dumpApd(((struct descriptor_a **)apd->pointer)[i], numSpaces + 2);
	printf("\n");
    }
}

void dumpObject(void *obj)
{
    struct descriptor_xd *xd = (struct descriptor_xd *)obj;
    if(xd->class == CLASS_XD)
	dumpObjElement((struct descriptor_a *)xd->pointer, 0);
    else
    	dumpObjElement((struct descriptor_a *)obj, 0);
}


//Low Level IDS Copy. Instance Number -1 means default IDS
//Note: this routine uses _tree routines and des not need locks
//Note: only MDSplus implementation is available. They are therefore only defined here

int mdsCopyCpo(int fromIdx, int toIdx, char *inputcpoName, int fromCpoOccur, int toCpoOccur)
{
    char *cpoName;
    void *fromCtx, *toCtx, *wildCtx = 0;
    char *fromNameBuf, *toNameBuf, *fromNameWild;
    int currNid, fromBaseNid, toBaseNid, toNid;
    int retNameLen, currSegment, numSegments, status;
    int usage = 1<<TreeUSAGE_SIGNAL | 1<<TreeUSAGE_NUMERIC | 1<<TreeUSAGE_TEXT;
    char *currName;
    char dtype, dimct;
    int dims[64], next_row, currDim, numItems, fromDefNid, toDefNid, i, currSize;
    EMPTYXD(xd);
    EMPTYXD(segmentXd);
    EMPTYXD(dimXd);
    EMPTYXD(startXd);
    EMPTYXD(endXd);
    EMPTYXD(emptyXd);

    DESCRIPTOR_A_COEFF(initArrDsc, 0, 0, 0, 0, 0);
    ARRAY_COEFF(char, 16) *currSegmentDPtr;

    fromCtx = getExpIndex(fromIdx);
    if(fromCtx == 0)
    {
        sprintf(errmsg, "Cannot get source pulse file");
        return -1;
    }
    toCtx = getExpIndex(toIdx);
    if(toCtx == 0)
    {
        sprintf(errmsg, "Cannot get target pulse file");
        return -1;
    }
    _TreeGetDefaultNid(fromCtx, &fromDefNid);
    _TreeGetDefaultNid(toCtx, &toDefNid);
    cpoName = path2MDS(inputcpoName);
    fromNameBuf = malloc(strlen(cpoName) + 16);
    fromNameWild = malloc(strlen(cpoName) + 16);
    toNameBuf = malloc(strlen(cpoName) + 16);
    if(fromCpoOccur > 0)
    {
	sprintf(fromNameBuf, "\\TOP.%s.%d", cpoName, fromCpoOccur);
	sprintf(fromNameWild, ".-.%d", fromCpoOccur);
    }
    else
    {
	sprintf(fromNameBuf, "\\TOP.%s", cpoName);
	sprintf(fromNameWild, ".-.%s", cpoName);
    }
    if(toCpoOccur > 0)
	sprintf(toNameBuf, "\\TOP.%s.%d", cpoName, toCpoOccur);
    else
	sprintf(toNameBuf, "\\TOP.%s", cpoName);

    status = _TreeSetDefault(fromCtx, fromNameBuf, &fromBaseNid);
    if(!(status & 1))
    {
        sprintf(errmsg, "Cannot find source IDS");
        return -1;
    }
    status = _TreeSetDefault(toCtx, toNameBuf, &toBaseNid);
    if(!(status & 1))
    {
        sprintf(errmsg, "Cannot find target IDS");
        return -1;
    }

    while(_TreeFindNodeWild(fromCtx, "***", &currNid, &wildCtx, usage) & 1)
    {
 	currName = _TreeGetMinimumPath(fromCtx, &fromBaseNid, currNid);
	if(currName[1] == '-') //It is thr baseNid itself
	    continue;
	if(currName[1] >= '0' && currName[1] <= '9') //It is the field of a different occurrence
	    continue;
	//printf("%s\n", currName);
	status = _TreeFindNode(toCtx, currName, &toNid);
	if(!(status & 1))
	{
	    printf("Warning: cannot find target %s in copy cpo\n", currName);
	    continue;
	}
//Make sure that the node content is deleted if containing segments
	status  = _TreeGetNumSegments(toCtx, toNid, &numSegments);
	if((status & 1) && numSegments > 0)
	{
	    status = _TreePutRecord(toCtx, toNid, (struct descriptor *)&emptyXd, 0);
	    if(!(status & 1))
	    {
		printf("Internal error: Cannot write in cpo field %s\n", currName);
		continue;
	    }
	}
	status = _TreeGetNumSegments(fromCtx, currNid, &numSegments);
	if(!(status & 1)) //No data contained, make sure no data is contained in the target
	{
	    status = _TreePutRecord(toCtx, toNid, (struct descriptor *)&emptyXd, 0);
	    if(!(status & 1))
	    {
		printf("Internal error: Cannot write in cpo field %s: %s\n", currName, MdsGetMsg(status));
		continue;
	    }
	}
	if(numSegments == 0) //Non segmented data
	{
	    status = _TreeGetRecord(fromCtx, currNid, &xd);
	    status = _TreePutRecord(toCtx, toNid, (struct descriptor *)&xd,0);
	    if(!(status & 1))
		printf("Internal error: Cannot write in cpo field %s: %s\n", currName, MdsGetMsg(status));
	    MdsFree1Dx(&xd, 0);
	}
	else
	{
	    for(currSegment = 0; currSegment < numSegments; currSegment++)
	    {
		status = _TreeGetSegment(fromCtx, currNid, currSegment, &segmentXd, &dimXd);
		if(status & 1)
		    status = _TreeGetSegmentLimits(fromCtx, currNid, currSegment, &startXd, &endXd);
		if(status & 1)
		    status = _TreeGetSegmentInfo(fromCtx, currNid, -1, &dtype, &dimct, dims, &next_row);
	    	if(!(status & 1))
		{
		    printf("Internal error: Cannot read segment in cpo field %s\n", currName);
		    continue;
		}
//If it is the last segment, and not containing bytes (serialized objects) make sure the new segment contains DEFAULT_SEGMENT_ROW rows
		if(currSegment == numSegments - 1 && dtype != DTYPE_B && dtype != DTYPE_BU && dims[dimct-1] < DEFAULT_SEGMENT_ROWS)
		    dims[dimct - 1] = DEFAULT_SEGMENT_ROWS;


		initArrDsc.dtype = dtype;
		initArrDsc.length = ((struct descriptor_a *)segmentXd.pointer)->length;
		initArrDsc.dimct = dimct;
		for(currDim = 0, numItems = 1; currDim < dimct; currDim++)
		{
		    initArrDsc.m[currDim] = dims[currDim];
		    numItems *= dims[currDim];
		}
		initArrDsc.arsize = numItems * initArrDsc.length;
		initArrDsc.pointer = calloc(numItems, initArrDsc.length);
		status = _TreeBeginSegment(toCtx, toNid, (struct descriptor *)&startXd, (struct descriptor *)&endXd, (struct descriptor *)&dimXd, (void *)&initArrDsc, -1);
		free(initArrDsc.pointer);
		if(status & 1)
		{
		    if(currSegment == numSegments - 1) //If it is the last segment, it is necessary to store only actual data, not the whole segment
		    {
		    	currSegmentDPtr = (void *)segmentXd.pointer;
		    	currSegmentDPtr->m[currSegmentDPtr->dimct - 1] = next_row;
			for(currSize = 1, i = 0; i < currSegmentDPtr->dimct - 1; currSize *= currSegmentDPtr->m[i], i++);
			currSegmentDPtr->arsize = next_row * currSize * currSegmentDPtr->length;

		    }
		    status = _TreePutSegment(toCtx, toNid, -1, (struct descriptor_a *)segmentXd.pointer);
	    	}
		if(!(status & 1))
		    printf("Internal error: Cannot write in cpo field %s: %s %s\n", currName, MdsGetMsg(status), _TreeGetPath(toCtx, toNid));
		MdsFree1Dx(&startXd, 0);
		MdsFree1Dx(&endXd, 0);
		MdsFree1Dx(&dimXd, 0);
		MdsFree1Dx(&segmentXd, 0);
	    }
	}
    }
    _TreeFindNodeEnd(fromCtx, &wildCtx);
    free(fromNameBuf);
    free(fromNameWild);
    free(toNameBuf);
//Make sure the default position has not changed
    _TreeSetDefaultNid(fromCtx, fromDefNid);
    _TreeSetDefaultNid(toCtx, toDefNid);

    return 0;
}


//Fake Routines still used by fortraninterface
void ids_discard_mem(){}
void ids_flush_all(){}
void ids_flush(){}
void ids_discard_old_mem(){}


//MERGE
#define MAX_COMMAND_ANSWER_LEN 10000
struct descriptor_xd *doShellCommand(char *cmd)
{
    static EMPTYXD(retXd);
    char *fullCommand = malloc(strlen(cmd) + 64);
    char *answer = malloc(MAX_COMMAND_ANSWER_LEN);
    int retLen;
    FILE *f;
    struct descriptor answerD = {0, DTYPE_T, CLASS_S, answer};
    char *tmpName;

    tmpName = tempnam("/tmp","ual_");
    sprintf(fullCommand, "%s > %s", cmd, tmpName);
    system(fullCommand);
    free(fullCommand);
    retLen = 0;
    f = fopen(tmpName, "r");
    if(!f) {
        free(tmpName);
        return NULL;
    }
    while(fread(&answer[retLen], 1, 1, f) > 0 && retLen < MAX_COMMAND_ANSWER_LEN)
        retLen++;
    fclose(f);
    remove(tmpName);
    free(tmpName);
    answerD.length = retLen;
    MdsCopyDxXd(&answerD, &retXd);
    free(answer);
    return &retXd;
}
//MERGE


