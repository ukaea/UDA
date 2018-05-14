#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <mdsdescrip.h>
#include <usagedef.h>
#include <ipdesc.h>
#include <treeshr.h>
#include <ncidef.h>
#include <mdsshr.h>
#include <mds_stdarg.h>
#ifdef __APPLE__
#  include <pthread.h>
#endif

#include "ual_low_level_mdsplus.h"
#include "ual_low_level_remote.h"

extern int getConnectionId(int expIdx);
extern int getRemoteConnectionIdx(int expIdx);
extern char *errmsg;

#define MAX_DATA_INFO  64


static struct {
    int expIdx;
    int refCount;
    char path[16];
    struct descriptor_xd xd;
}getDataInfo[MAX_DATA_INFO];


#define MAX_CPO_FIELDS 100000
static struct {
    int expIdx;
    char path[16];
    char timeBasePath[132];
    int numDataFields;
    int numSegmentFields;
    int numObjSegmentFields;
    struct descriptor_xd dataPathXds[MAX_CPO_FIELDS];
    struct descriptor_xd segmentPathXds[MAX_CPO_FIELDS];
    struct descriptor_xd objSegmentPathXds[MAX_CPO_FIELDS];
    struct descriptor_xd dataXds[MAX_CPO_FIELDS];
    struct descriptor_xd segmentXds[MAX_CPO_FIELDS];
    struct descriptor_xd objSegmentXds[MAX_CPO_FIELDS];
}putDataInfo[MAX_DATA_INFO];

static pthread_mutex_t mutex;
static int initialized = 0;
static void lock()
{
    LockMdsShrMutex(&mutex, &initialized);
}
static void unlock()
{
    UnlockMdsShrMutex(&mutex);
}
static void printDecompiled(struct descriptor *dsc)
{
	EMPTYXD(out_xd);
	static char decompiled[1000000000];

	int status = TdiDecompile(dsc, &out_xd MDS_END_ARG);
	if(!out_xd.pointer) printf("NULL\n");
	memcpy(decompiled, out_xd.pointer->pointer, out_xd.pointer->length);
	decompiled[out_xd.pointer->length] = 0;
	printf("%s\n", decompiled);
	MdsFree1Dx(&out_xd, 0);
}

static char *toUpperString(char *inStr)
{
    int i;
    int len = strlen(inStr);
    char *outStr = malloc(len + 1);
    for(i = 0; i < len; i++)
    	outStr[i] = toupper(inStr[i]);
    outStr[len] = 0;
    return outStr;
}
static int beginPutDataInfo(int expIdx, char *path)
{
    int i;
    lock();
    for(i = 0; i < MAX_DATA_INFO; i++)
    {
        if(!putDataInfo[i].path[0])
	{
	    putDataInfo[i].expIdx = expIdx;
	    strcpy(putDataInfo[i].path, path);
	    putDataInfo[i].numSegmentFields = putDataInfo[i].numDataFields = putDataInfo[i].numObjSegmentFields = 0;
	    unlock();
	    return 0;
	}
    }
    printf("INTERNAL ERROR: no putDataInfo slots left!!!\n");
    unlock();
    return -1;
}

static char *mdsconvertPath(char *);

static int insertPutDataInfo(int expIdx, char *cpoPath, char *path, char *timeBasePath, struct descriptor *dataDsc)
{
    int i;
    static EMPTYXD(emptyXd);
    struct descriptor pathDsc = {0, DTYPE_T, CLASS_S, 0};
    if(dataDsc->class == CLASS_XD && !dataDsc->pointer)
        return 0;
    lock();
//  printf("insertPutDataInfo. cpoPath: %s, path: %s, timeBasePath: %s \n",cpoPath, path, timeBasePath);
    for(i = 0; i < MAX_DATA_INFO; i++)
    {
	if(putDataInfo[i].expIdx == expIdx && !strcmp(putDataInfo[i].path, cpoPath))
	{
	    if(putDataInfo[i].numDataFields == MAX_CPO_FIELDS)
	    {
	        printf("INTERNAL ERROR: naximum number of CPO fields in putDataInfo reached!!!\n");
		unlock();
		return -1;
	    }
	    if (timeBasePath != NULL) strncpy(putDataInfo[i].timeBasePath,timeBasePath,132);
	    putDataInfo[i].dataXds[putDataInfo[i].numDataFields] = emptyXd;
	    MdsCopyDxXd(dataDsc, &putDataInfo[i].dataXds[putDataInfo[i].numDataFields]);
	    pathDsc.length = strlen(path);
	    pathDsc.pointer = path;
	    putDataInfo[i].dataPathXds[putDataInfo[i].numDataFields] = emptyXd;
	    MdsCopyDxXd((struct descriptor *)&pathDsc, &putDataInfo[i].dataPathXds[putDataInfo[i].numDataFields]);
	    putDataInfo[i].numDataFields++;
	    unlock();
	    return 0;
	}
    }
    printf("INTERNAL ERROR: no putDataInfo slots found!!!\n");
    unlock();
    return -1;
}

static int insertPutSegmentInfo(int expIdx, char *cpoPath, char *path, char *timeBasePath, struct descriptor *dataDsc)
//static int insertPutSegmentInfo(int expIdx, char *cpoPath, char *path, struct descriptor *dataDsc)
{
    int i;
    static EMPTYXD(emptyXd);
    struct descriptor pathDsc = {0, DTYPE_T, CLASS_S, 0};

    if(dataDsc->class == CLASS_XD && !dataDsc->pointer)
        return 0;
    lock();
    for(i = 0; i < MAX_DATA_INFO; i++)
    {
	if(putDataInfo[i].expIdx == expIdx && !strcmp(putDataInfo[i].path, cpoPath))
	{
	    if(putDataInfo[i].numSegmentFields == MAX_CPO_FIELDS)
	    {
	        printf("INTERNAL ERROR: naximum number of CPO fields in putSegmentInfo reached!!!\n");
		unlock();
		return -1;
	    }
	    strncpy(putDataInfo[i].timeBasePath,timeBasePath,132);
	    putDataInfo[i].segmentXds[putDataInfo[i].numSegmentFields] = emptyXd;
	    MdsCopyDxXd(dataDsc, &putDataInfo[i].segmentXds[putDataInfo[i].numSegmentFields]);
	    pathDsc.length = strlen(path);
	    pathDsc.pointer = path;
	    putDataInfo[i].segmentPathXds[putDataInfo[i].numSegmentFields] = emptyXd;
	    MdsCopyDxXd((struct descriptor *)&pathDsc, &putDataInfo[i].segmentPathXds[putDataInfo[i].numSegmentFields]);
	    putDataInfo[i].numSegmentFields++;
	    unlock();
	    return 0;
	}
    }
    printf("INTERNAL ERROR: no putSegmentInfo slots found!!!\n");
    unlock();
    return -1;
}
static int insertPutObjectSegmentInfo(int expIdx, char *cpoPath, char *path, struct descriptor *dataDsc, int segIdx)
{
    int i;
    static EMPTYXD(emptyXd);
    struct descriptor pathDsc = {0, DTYPE_T, CLASS_S, 0};
    if(dataDsc->class == CLASS_XD && !dataDsc->pointer)
        return 0;
    lock();
    for(i = 0; i < MAX_DATA_INFO; i++)
    {
	if(putDataInfo[i].expIdx == expIdx && !strcmp(putDataInfo[i].path, cpoPath))
	{
	    if(putDataInfo[i].numObjSegmentFields == MAX_CPO_FIELDS)
	    {
	        printf("INTERNAL ERROR: maximum number of CPO fields in putObjSegmentInfo reached!!!\n");
		unlock();
		return -1;
	    }
	    putDataInfo[i].objSegmentXds[putDataInfo[i].numObjSegmentFields] = emptyXd;
	    MdsCopyDxXd(dataDsc, &putDataInfo[i].objSegmentXds[putDataInfo[i].numObjSegmentFields]);
	    pathDsc.length = strlen(path);
	    pathDsc.pointer = path;
	    putDataInfo[i].objSegmentPathXds[putDataInfo[i].numObjSegmentFields] = emptyXd;
	    MdsCopyDxXd((struct descriptor *)&pathDsc, &putDataInfo[i].objSegmentPathXds[putDataInfo[i].numObjSegmentFields]);
	    putDataInfo[i].numObjSegmentFields++;
	    unlock();
	    return 0;
	}
    }
    printf("INTERNAL ERROR: no putObjSegmentInfo slots found!!!\n");
    unlock();
    return -1;
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
//    printf("convertPath. initial path: %s\n",path);
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
	strcat(mdsCpoPath,"/"); /* . */
	if (!isdigit(*ptr)) {
          strcat(mdsCpoPath,path2MDS(ptr));
	}
	else {
	  strcat(mdsCpoPath,ptr);
	}
    }
//    printf("convertPath. modified path: %s\n",mdsCpoPath);
    return mdsCpoPath;
}

static char *mdsconvertPath(char *inputpath)
{
    static char mdsCpoPath[2048];
    char  path[2048];
    char *ptr;
    int   i;

    strcpy(path,inputpath);
//  printf("mdsconvertPath. initial path: %s\n",path);
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
//  printf("mdsconvertPath. modified path: %s\n",mdsCpoPath);
    return mdsCpoPath;
}

//Build APD Composed of pairs (name, data) and free structures
static void terminatePutDataInfo(int expIdx, char *cpoPath, char *timeBasePath, struct descriptor_xd *retDataXd, struct descriptor_xd *retSegmentXd, struct descriptor_xd *retObjSegmentXd)
{
    struct descriptor **dscs;
    DESCRIPTOR_APD(retApd, DTYPE_DSC, 0, 0);
    int i, j;
    lock();
    for(i = 0; i < MAX_DATA_INFO; i++)
    {
	if(putDataInfo[i].expIdx == expIdx && !strcmp(putDataInfo[i].path, cpoPath))
	{
	//Non segmented stuff
	    dscs = (struct descriptor **)malloc(2 * putDataInfo[i].numDataFields * sizeof(struct descriptor *));
	    for(j = 0; j < putDataInfo[i].numDataFields; j++)
	    {
	    	dscs[2*j] = putDataInfo[i].dataPathXds[j].pointer;
		if(putDataInfo[i].dataXds[j].pointer)
	    	    dscs[2*j+1] = putDataInfo[i].dataXds[j].pointer;
		else
	    	    dscs[2*j+1] = (struct descriptor *)&putDataInfo[i].dataXds[j];

		//printDecompiled(dscs[2*j]);
		//printDecompiled(dscs[2*j+1]);
	    }
	    retApd.arsize =  2 * putDataInfo[i].numDataFields * sizeof(struct descriptor *);
//          printf("terminatePutDataInfo1. j: %d, retApd.arsize: %d\n",j,retApd.arsize);
	    retApd.pointer = (void *)dscs;
	    MdsCopyDxXd((struct descriptor *)&retApd, retDataXd);
	    //Free structures
	    for(j = 0; j < putDataInfo[i].numDataFields; j++)
	    {
	    	MdsFree1Dx(&putDataInfo[i].dataPathXds[j], 0);
	    	MdsFree1Dx(&putDataInfo[i].dataXds[j], 0);
	    }
	    free((char *)dscs);

	//Segmented stuff
            if (putDataInfo[i].numSegmentFields > 0) {
                strcpy(timeBasePath,putDataInfo[i].timeBasePath);
             }
	     else {
               strcpy(timeBasePath,putDataInfo[i].timeBasePath);
//                strcpy(timeBasePath, "time");
             }
	    dscs = (struct descriptor **)malloc(2 * putDataInfo[i].numSegmentFields * sizeof(struct descriptor *));
	    for(j = 0; j < putDataInfo[i].numSegmentFields; j++)
	    {
	    	dscs[2*j] = putDataInfo[i].segmentPathXds[j].pointer;

		if(putDataInfo[i].segmentXds[j].pointer)
	    	    dscs[2*j+1] = putDataInfo[i].segmentXds[j].pointer;
		else
	    	    dscs[2*j+1] = (struct descriptor *)&putDataInfo[i].segmentXds[j];

	    }
	    retApd.arsize =  2 * putDataInfo[i].numSegmentFields * sizeof(struct descriptor *);
//          printf("terminatePutDataInfo2. j: %d, retApd.arsize: %d\n",j,retApd.arsize);
	    retApd.pointer = (void *)dscs;
	    MdsCopyDxXd((struct descriptor *)&retApd, retSegmentXd);
	    //Free structures
	    for(j = 0; j < putDataInfo[i].numSegmentFields; j++)
	    {
	    	MdsFree1Dx(&putDataInfo[i].segmentPathXds[j], 0);
	    	MdsFree1Dx(&putDataInfo[i].segmentXds[j], 0);
	    }
	    free((char *)dscs);
	//Object Segmented stuff
	    dscs = (struct descriptor **)malloc(2 * putDataInfo[i].numObjSegmentFields * sizeof(struct descriptor *));
	    for(j = 0; j < putDataInfo[i].numObjSegmentFields; j++)
	    {
	    	dscs[2*j] = putDataInfo[i].objSegmentPathXds[j].pointer;

		if(putDataInfo[i].objSegmentXds[j].pointer)
	    	    dscs[2*j+1] = putDataInfo[i].objSegmentXds[j].pointer;
		else
	    	    dscs[2*j+1] = (struct descriptor *)&putDataInfo[i].objSegmentXds[j];

	    }
	    retApd.arsize =  2 * putDataInfo[i].numObjSegmentFields * sizeof(struct descriptor *);
//          printf("terminatePutDataInfo3. j: %d, retApd.arsize: %d\n",j,retApd.arsize);
	    retApd.pointer = (void *)dscs;
	    MdsCopyDxXd((struct descriptor *)&retApd, retObjSegmentXd);
	    //Free structures
	    for(j = 0; j < putDataInfo[i].numObjSegmentFields; j++)
	    {
	    	MdsFree1Dx(&putDataInfo[i].objSegmentPathXds[j], 0);
	    	MdsFree1Dx(&putDataInfo[i].objSegmentXds[j], 0);
	    }
	    free((char *)dscs);


	    putDataInfo[i].path[0] = 0;
	    unlock();
	    return;
	}
    }
    printf("INTERNAL ERROR: no putDataInfo slot found!!!\n");
    unlock();
 }




static void setDataInfo(int expIdx, char *path, struct descriptor_a *serialized)
{
    int i, status;
    static EMPTYXD(emptyXd);


//printf("SET DATA INFO %d %s\n", expIdx, path);

    for(i = 0; i < MAX_DATA_INFO; i++)
    {
        if(getDataInfo[i].expIdx == expIdx && !strcmp(path, getDataInfo[i].path))
	{
	    printf("IDS Data already cached!!!!\n");
	    getDataInfo[i].refCount++;
	    return;
	}
    }
    for(i = 0; i < MAX_DATA_INFO; i++)
    {
        if(getDataInfo[i].path[0] == 0)
	{
	    getDataInfo[i].expIdx = expIdx;
	    strcpy(getDataInfo[i].path, path);
	    getDataInfo[i].refCount = 1;
	    getDataInfo[i].xd = emptyXd;
	    status = MdsSerializeDscIn(serialized->pointer, &getDataInfo[i].xd);
	    if(!(status & 1)) printf("INTERNAL ERROR: Cannot deserialize remote data: %s\n", MdsGetMsg(status));
	    return;
	}
    }
    printf("INTERNAL ERROR: No DataInfo slots left!!!\n");
}

static void releaseDataInfo(int expIdx, char *path)
{
    int i;

    for(i = 0; i < MAX_DATA_INFO; i++)
    {
        if(getDataInfo[i].expIdx == expIdx && !strcmp(path, getDataInfo[i].path))
	{
	    getDataInfo[i].refCount--;
	    if(getDataInfo[i].refCount == 0)
	    {
	        MdsFree1Dx(&getDataInfo[i].xd, 0);
		getDataInfo[i].path[0] = 0;
	    }
	    return;
	}
    }
    printf("INTERNAL ERROR: Released non existing data info!!!\n");
}


static int collectDataInfo(int expIdx, char *path, char *cpoPath, struct descriptor_xd *retXd)
{
    int i, j, numItems;
    struct descriptor *currDsc;
    char *currPath;

    currPath = toUpperString(convertPath(cpoPath));
    for(i = 0; i < MAX_DATA_INFO; i++)
    {
/*printf("collectDataInfo. i: %d path: %s, cpoPath: %s, getDataInfo[i].path: %s\n",i, path, cpoPath, getDataInfo[i].path);*/
        if(getDataInfo[i].expIdx == expIdx && !strcmp(path, getDataInfo[i].path))
	{
    	    struct descriptor_a *arrDsc = (struct descriptor_a *)getDataInfo[i].xd.pointer;
	    if(!arrDsc || arrDsc->class != CLASS_APD)
	    {
	        printf("INTERNAL ERROR: cannot get cached remote data!!!\n");
		return -1 ;
	    }
	    numItems = arrDsc->arsize/arrDsc->length;
	    numItems/=2;
	    for(j = 0; j < numItems; j++)
	    {
	        currDsc = ((struct descriptor **)arrDsc->pointer)[2*j];
		if(currDsc->dtype != DTYPE_T)
		{
		    printf("INTERNAL ERROR: Wrong type in cached remote data!!!\n");
		    return -1;
		}
//		if(currDsc->length == strlen(cpoPath) && !strncmp(currDsc->pointer, cpoPath, currDsc->length)) //Found item
		if(currDsc->length == strlen(currPath) && !strncmp(currDsc->pointer, currPath, currDsc->length)) //Found item
		{
		    MdsCopyDxXd(((struct descriptor **)arrDsc->pointer)[2*j+1], retXd);
		    return 0;
		}
	    }
	    //Missing data
	    return -1;
	}
    }
    printf("INTERNAL ERROR: DataInfo not found for expIdx %d, path %s!!!\n", expIdx, path);
    return -1;
}


struct descriptor_xd* getCpoDataServer(int *expIdx, char *path)
{
    static EMPTYXD(retXd);
    static EMPTYXD(emptyXd);
    EMPTYXD(xd);
    struct descriptor messageD = {0, DTYPE_T, CLASS_S, 0};
    int status;
    char *msg;
    char **cpoFields;
    int numCpoFields, i;
    struct descriptor_xd *xds;
    struct descriptor *currNameDsc;
    struct descriptor *nameDscs;
    struct descriptor **dscs;
    DESCRIPTOR_APD(retApd, DTYPE_DSC, 0, 0);

    //printf("GetDataServer(%d, %s)\n", *expIdx, path);

    cpoFields = getMdsCpoFields(*expIdx, path, &numCpoFields, 0);
    if(!cpoFields)
    {
    	msg = mdsLastErrmsg();
	messageD.length = strlen(msg);
	messageD.pointer = msg;
	MdsCopyDxXd((struct descriptor *)&messageD, &retXd);
	return &retXd;
    }

    //printf("LETTI %d CPO fields\n", numCpoFields);

    xds = (struct descriptor_xd *)malloc(numCpoFields * sizeof(struct descriptor_xd));
    nameDscs = (struct descriptor *)malloc(numCpoFields * sizeof(struct descriptor));
    for(i=  0; i < numCpoFields; i++)
    {
        xds[i] =  emptyXd;
	nameDscs[i].dtype = DTYPE_T;
	nameDscs[i].class = CLASS_S;
	nameDscs[i].length = strlen(cpoFields[i]);
	nameDscs[i].pointer = cpoFields[i];
    }

    dscs = (struct descriptor **)malloc(numCpoFields * 2 * sizeof(struct descriptor *));
    for(i = 0; i < numCpoFields; i++)
    {
	//printf("getCpoDataServer. cpoFields[%d]= %s\n",i,cpoFields[i]);
//        status = getData(*expIdx, path, cpoFields[i], &xds[i], 1);
        status = mdsgetData(*expIdx, path, cpoFields[i], &xds[i], 1); // cpoFields[i] is already converted
        if(status || !xds[i].pointer)
        {
    	    msg = mdsLastErrmsg();
	    messageD.length = strlen(msg);
	    messageD.pointer = msg;
	    MdsCopyDxXd((struct descriptor *)&messageD, &retXd);
	    return &retXd;
	}
	dscs[2*i] = &nameDscs[i];
	dscs[2*i+1] = xds[i].pointer;
    }

    retApd.arsize = 2*numCpoFields * sizeof(struct descriptor *);
    retApd.pointer = (void *)dscs;

    status = MdsSerializeDscOut((struct descriptor *)&retApd, &retXd);
    if(!(status & 1) || !retXd.pointer)
	printf("INTERNAL ERROR: Cannot serialize result\n");
    for(i = 0; i < numCpoFields; i++)
    	MdsFree1Dx(&xds[i], 0);
    free((char *)dscs);
    free((char *)xds);
    free((char *)nameDscs);

    for(i = 0; i < numCpoFields; i++)
        free(cpoFields[i]);
    free((char *)cpoFields);


    return &retXd;
}


int getDataRemote(int connId, int expIdx, char *cpoPath, char *path, struct descriptor_xd *retXd, int evaluate)
{
    char *upPath = toUpperString(path);
    int status =  collectDataInfo(expIdx, cpoPath, upPath, retXd);
    free(upPath);
    return status;
}
int getObjectRemote(int connId, int expIdx, char *cpoPath, char *path, void **obj, int isTimed)
{
    char *upPath = toUpperString(path);
    EMPTYXD(retXd);
    char *fullPath = malloc(strlen(upPath) + 16);

    if(isTimed)
	sprintf(fullPath, "%s/TIMED", upPath);
    else
	sprintf(fullPath, "%s/NON_TIMED", upPath);

    int status =  collectDataInfo(expIdx, cpoPath, fullPath, &retXd);
    if(!status)
    {
	struct descriptor_xd *xdPtr = (struct descriptor_xd *)malloc(sizeof(struct descriptor_xd));
	*xdPtr = retXd;
	*obj = (void *)xdPtr;
    }
    free(upPath);
    free(fullPath);
    return status;
}

int mdsbeginIdsGetRemote(int connId, int remoteExpIdx, char *path, int isTimed, int *retSamples)
{
    EMPTYXD(timeXd);
    char expr[512];
    char *currErr;
    int errLen;
    int status;
    struct descrip pathDsc;
    struct descrip remoteExpIdxDsc;
    struct descrip ansarg;
    DESCRIPTOR_A(serializedDsc, 1, DTYPE_BU, 0, 0);
    struct descriptor_a *arrDsc;

    sprintf(expr, "imas->getCpoDataServer:DSC($1, $2)");
    memset(&ansarg, 0, sizeof(ansarg));
    MakeDescrip((struct descrip *)&pathDsc,DTYPE_CSTRING,0,0,path);
    MakeDescrip((struct descrip *)&remoteExpIdxDsc,DTYPE_LONG,0,0,&remoteExpIdx);

    status = MdsValue(connId, expr, &remoteExpIdxDsc, &pathDsc, &ansarg, NULL);
    if(!(status & 1))
    {
        sprintf(errmsg, "Remote Access Failed: %s", MdsGetMsg(status));
	return -1;
    }
    if(ansarg.dtype == DTYPE_CSTRING)
    {
    	errLen = ansarg.length;
	if(errLen > 256) errLen = 256;
    	strncpy(errmsg, ansarg.ptr, errLen);
	errmsg[errLen] = 0;
	printf("Error Getting serialized remote CPO data: %s\n", errmsg);
	return -1;
    }
    if(ansarg.length == 0)
    {
 	printf("No bytes for serialized remote CPO data\n");
	return -1;
    }
     if(ansarg.dtype != DTYPE_CHAR && ansarg.dtype != DTYPE_UCHAR)
    {
 	printf("Wrong returned type for serialized remote CPO data\n");
	return -1;
    }
    serializedDsc.arsize = ansarg.length;
    serializedDsc.pointer = ansarg.ptr;

    setDataInfo(remoteExpIdx, path, (struct descriptor_a *)&serializedDsc);
    free(ansarg.ptr);
    if(!isTimed)
        *retSamples = 1;
    else
    {
        status = collectDataInfo(remoteExpIdx, path, "time", &timeXd);
        if(status || !timeXd.pointer || timeXd.pointer->class != CLASS_A)
        {
           printf("INTERNAL ERROR: unknown times in serialized remote data!!\n");
	   return -1;
        }
	arrDsc = (struct descriptor_a *)timeXd.pointer;
	*retSamples = arrDsc->arsize/arrDsc->length;
	MdsFree1Dx(&timeXd, 0);
    }
    return 0;
}

int mdsendIdsGetRemote(int connId, int expIdx, char *path)
{
    releaseDataInfo(expIdx, path);
    return 0;
}


struct descriptor_xd* getCpoSlicedDataServer(int *expIdx, char *path, double *time)
{
    static EMPTYXD(retXd);
    static EMPTYXD(emptyXd);
    EMPTYXD(xd);
    EMPTYXD(timeXd);
    struct descriptor messageD = {0, DTYPE_T, CLASS_S, 0};
    int status;
    char *msg;
    char **cpoFields;
    int numCpoFields, i, numSlicedFields, numFields;
    struct descriptor_xd *xds;
    struct descriptor *currNameDsc;
    struct descriptor *nameDscs;
    struct descriptor **dscs;
    DESCRIPTOR_APD(retApd, DTYPE_DSC, 0, 0);

    //printf("GetCpoSlicedDataServer(%d, %s, %f)\n", *expIdx, path,*time);

    cpoFields = getMdsCpoFields(*expIdx, path, &numFields, 0);
    if(!cpoFields)
    {
    	msg = mdsLastErrmsg();
	messageD.length = strlen(msg);
	messageD.pointer = msg;
	MdsCopyDxXd((struct descriptor *)&messageD, &retXd);
	return &retXd;
    }

    xds = (struct descriptor_xd *)malloc(numFields * sizeof(struct descriptor_xd));
    nameDscs = (struct descriptor *)malloc(numFields * sizeof(struct descriptor));
    for(i=  0; i < numFields; i++)
    {
        xds[i] = emptyXd;
	nameDscs[i].dtype = DTYPE_T;
	nameDscs[i].class = CLASS_S;
	nameDscs[i].length = strlen(cpoFields[i]);
	nameDscs[i].pointer = cpoFields[i];
    }

    dscs = (struct descriptor **)malloc(numFields * 2 * sizeof(struct descriptor *));
    numSlicedFields = 0;
    for(i = 0; i < numFields; i++)
    {
    	//printf("GetCpoSlicedDataServer2. path: %s, %d, %s)\n", path, i, cpoFields[i]);
        if(mdsIsSliced(*expIdx, path, cpoFields[i]))
	{
            status = mdsgetSlicedData(*expIdx, path, cpoFields[i], *time, &xds[i], &timeXd, 0);
	    MdsFree1Dx(&timeXd, 0);
	}
	else {
            status = mdsgetData(*expIdx, path, cpoFields[i], &xds[i], 1);
	}
        if(!status)
        {
	    dscs[2*i] = &nameDscs[i];
	    dscs[2*i+1] = xds[i].pointer;
	    numSlicedFields++;
	}
    }

    retApd.arsize = 2*numSlicedFields * sizeof(struct descriptor *);
    retApd.pointer = (void *)dscs;

    status = MdsSerializeDscOut((struct descriptor *)&retApd, &retXd);
    if(!(status & 1) || !retXd.pointer)
	printf("INTERNAL ERROR: Cannot serialize result\n");
    for(i = 0; i < numSlicedFields; i++)
    	MdsFree1Dx(&xds[i], 0);
    free((char *)dscs);
    free((char *)xds);
    free((char *)nameDscs);

    for(i = 0; i < numFields; i++)
        free(cpoFields[i]);
    free((char *)cpoFields);

    return &retXd;
}


int getSlicedDataRemote(int connId, int expIdx, char *cpoPath, char *path, double time, struct descriptor_xd *retDataXd,
    struct descriptor_xd *retTimesXd)
{
    char *upPath = toUpperString(path);
    int status = collectDataInfo(expIdx, cpoPath, upPath, retDataXd);
    if(!status) status = collectDataInfo(expIdx, cpoPath, "time", retTimesXd);
    free(upPath);
    return status;
}
int getObjectSliceRemote(int connId, int expIdx, char *cpoPath, char *path, double time, void **obj)
{
    char *upPath = toUpperString(path);
    EMPTYXD(retXd);
    char *fullPath = malloc(strlen(upPath) + 7);
    sprintf(fullPath, "%s/TIMED", upPath);
    int status =  collectDataInfo(expIdx, cpoPath, fullPath, &retXd);
    if(!status)
    {
	struct descriptor_xd *xdPtr = (struct descriptor_xd *)malloc(sizeof(struct descriptor_xd));
	*xdPtr = retXd;
	*obj = (void *)xdPtr;
    }
    free(upPath);
    free(fullPath);
    return status;
}

int mdsbeginIdsGetSliceRemote(int connId, int remoteExpIdx, char *path, double time)
{
    EMPTYXD(timeXd);
    char expr[512];
    char *currErr;
    int errLen;
    int status;
    struct descrip pathDsc;
    struct descrip timeDsc;
    struct descrip remoteExpIdxDsc;
    struct descrip ansarg;
    DESCRIPTOR_A(serializedDsc, 1, DTYPE_BU, 0, 0);
    struct descriptor_a *arrDsc;

    MakeDescrip((struct descrip *)&pathDsc,DTYPE_CSTRING,0,0,path);
    MakeDescrip((struct descrip *)&remoteExpIdxDsc,DTYPE_LONG,0,0,&remoteExpIdx);
    MakeDescrip((struct descrip *)&timeDsc,DTYPE_DOUBLE,0,0,&time);

/*    sprintf(expr, "imas->getCpoSlicedDataServer:DSC($1, $2, $3)");*/
    sprintf(expr, "imas->getCpoDataServer:DSC($1, $2)");
    memset(&ansarg, 0, sizeof(ansarg));
    status = MdsValue(connId, expr, &remoteExpIdxDsc, &pathDsc, &timeDsc, &ansarg, NULL);

    //getCpoSlicedDataServer(remoteExpIdxDsc.ptr, pathDsc.ptr, timeDsc.ptr);


    if(!(status & 1))
    {
        sprintf(errmsg, "Remote Access Failed: %s", MdsGetMsg(status));
	return -1;
    }
    if(ansarg.dtype == DTYPE_CSTRING)
    {
    	errLen = ansarg.length;
	if(errLen > 256) errLen = 256;
    	strncpy(errmsg, ansarg.ptr, errLen);
	errmsg[errLen] = 0;
	printf("Error Getting serialized remote CPO Slice: %s\n", errmsg);
	return -1;
    }
    if(ansarg.length == 0)
    {
 	printf("No bytes for serialized remote CPO Slice\n");
	return -1;
    }
     if(ansarg.dtype != DTYPE_CHAR && ansarg.dtype != DTYPE_UCHAR)
    {
 	printf("Wrong returned type for serialized remote CPO Slice\n");
	return -1;
    }
    serializedDsc.arsize = ansarg.length;
    serializedDsc.pointer = ansarg.ptr;

    setDataInfo(remoteExpIdx, path, (struct descriptor_a *)&serializedDsc);
    free(ansarg.ptr);
    return 0;
}

int mdsendIdsGetSliceRemote(int connId, int expIdx, char *path)
{
    releaseDataInfo(expIdx, path);
    return 0;
}

/////////////////////Put Stuff



int putDataServer(int *expIdx, char *cpoPath, char *timeBasePath, char *serData, char *serSegment, char *serObjSegment, double *times, int *nTimesPtr)
// int putDataServer(int *expIdx, char *cpoPath, char *serData, char *serSegment, char *serObjSegment, double *times, int *nTimesPtr)
{
    int i, numFields, status;
    char *path;
    struct descriptor *currPathDsc, *currDataDsc;
    int nTimes;
    EMPTYXD(dataXd);
    EMPTYXD(segmentXd);
    EMPTYXD(objSegmentXd);
    struct descriptor_a *dataApd, *segmentApd, *objSegmentApd;

    // printf("putDataServer 1. cpoPath: %s\n",cpoPath);
    mdsDeleteAllFields(*expIdx, cpoPath);

    status = MdsSerializeDscIn(serData, &dataXd);
    if(!(status & 1)||!dataXd.pointer)
    {
        printf("INTERNAL ERROR: Cannot deserialize data: %s\n", MdsGetMsg(status));
	return -1;
    }

    status = MdsSerializeDscIn(serSegment, &segmentXd);
    if(!(status & 1) || !segmentXd.pointer)
    {
        printf("INTERNAL ERROR: Cannot deserialize data!!\n");
	return -1;
    }
    status = MdsSerializeDscIn(serObjSegment, &objSegmentXd);
    if(!(status & 1) || !objSegmentXd.pointer)
    {
        printf("INTERNAL ERROR: Cannot deserialize data!!\n");
	return -1;
    }
    dataApd = (struct descriptor_a *)dataXd.pointer;
    segmentApd = (struct descriptor_a *)segmentXd.pointer;
    objSegmentApd = (struct descriptor_a *)objSegmentXd.pointer;


    if(dataApd->class != CLASS_APD)
    {
        printf("INTERNAL ERROR: Expected APD, got class %d\n", dataApd->class);
	return -1;
    }
    if(segmentApd->class != CLASS_APD)
    {
        printf("INTERNAL ERROR: Expected APD, got class %d\n", dataApd->class);
	return -1;
    }
    if(objSegmentApd->class != CLASS_APD)
    {
        printf("INTERNAL ERROR: Expected APD, got class %d\n", dataApd->class);
	return -1;
    }

    nTimes = *nTimesPtr;

//Segmented stuff
    numFields = segmentApd->arsize/segmentApd->length;
    numFields /= 2;
    //printf("putDataServer 4. cpoPath: %s, numFields: %d\n",cpoPath, numFields);
    for(i = 0; i < numFields; i++)
    {
        currPathDsc = ((struct descriptor **)segmentApd->pointer)[2 * i];
        currDataDsc = ((struct descriptor **)segmentApd->pointer)[2 * i+1];
	path = malloc(currPathDsc->length + 1);
	memcpy(path, currPathDsc->pointer, currPathDsc->length);
	path[currPathDsc->length] = 0;
        //printf("putDataServer 4a. cpoPath: %s, path: %s\n",cpoPath, path);
        status = putSegment(*expIdx, cpoPath, path, timeBasePath, (struct descriptor_a *)currDataDsc, times, nTimes);
	if(status)
	    printf("Internal error in putDataServer %s. %s\n", path, mdsLastErrmsg());
	free(path);
    }

//Object Segmented stuff
    numFields = objSegmentApd->arsize/objSegmentApd->length;
    numFields /= 2;
    //printf("putDataServer 5. cpoPath: %s, numFields: %d\n",cpoPath, numFields);
    for(i = 0; i < numFields; i++)
    {
        currPathDsc = ((struct descriptor **)objSegmentApd->pointer)[2 * i];
        currDataDsc = ((struct descriptor **)objSegmentApd->pointer)[2 * i+1];
	path = malloc(currPathDsc->length + 1);
	memcpy(path, currPathDsc->pointer, currPathDsc->length);
	path[currPathDsc->length] = 0;
	status = putObjectSegment(*expIdx, cpoPath, path, (void *)currDataDsc, -1);
	if(status)
	    printf("Internal error in putDataServer %s. %s\n", path, mdsLastErrmsg());
	free(path);
    }

 //Non Segmented stuff
    numFields = dataApd->arsize/dataApd->length;
    numFields /= 2;
    //printf("putDataServer 6. cpoPath: %s, numFields: %d\n",cpoPath, numFields);
    for(i = 0; i < numFields; i++)
    {
        currPathDsc = ((struct descriptor **)dataApd->pointer)[2 * i];
        currDataDsc = ((struct descriptor **)dataApd->pointer)[2 * i+1];
	path = malloc(currPathDsc->length + 1);
	memcpy(path, currPathDsc->pointer, currPathDsc->length);
	path[currPathDsc->length] = 0;
	status = putData(*expIdx, cpoPath, path, currDataDsc);
	if(status)
	    printf("Internal error in putDataServer %s. %s\n", path, mdsLastErrmsg());
	free(path);
    }
    MdsFree1Dx(&dataXd, 0);
    MdsFree1Dx(&segmentXd, 0);
    MdsFree1Dx(&objSegmentXd, 0);
    return 0;
}

/*
int putSliceServer(int *expIdx, char *cpoPath, char *serData, char *serObjSegment, double *time)
*/
int putSliceServer(int *expIdx, char *cpoPath, char *timeBasePath, char *serData, char *serObjSegment, double *time)
{
    int i, numFields, status;
    char *path;
    struct descriptor *currPathDsc, *currDataDsc, *currObjSegmentDsc;

    EMPTYXD(dataXd);
    struct descriptor_a *dataApd;
    EMPTYXD(objSegmentXd);
    struct descriptor_a *objSegmentApd;

    //printf("*******putSliceServer. cpoPath: %s, timeBasePath:%s\n",cpoPath ,timeBasePath);
    status = MdsSerializeDscIn(serData, &dataXd);
    if(!(status & 1)||!dataXd.pointer)
    {
        printf("INTERNAL ERROR: Cannot deserialize data!!\n");
	return -1;
    }
    status = MdsSerializeDscIn(serObjSegment, &objSegmentXd);
    if(!(status & 1)||!objSegmentXd.pointer)
    {
        printf("INTERNAL ERROR: Cannot deserialize data!!\n");
	return -1;
    }

    dataApd = (struct descriptor_a *)dataXd.pointer;
    objSegmentApd = (struct descriptor_a *)objSegmentXd.pointer;


    if(dataApd->class != CLASS_APD)
    {
        printf("INTERNAL ERROR: Expected APD, got class %d\n", dataApd->class);
	return -1;
    }
    if(objSegmentApd->class != CLASS_APD)
    {
        printf("INTERNAL ERROR: Expected APD, got class %d\n", objSegmentApd->class);
	return -1;
    }

//Data stuff
    numFields = dataApd->arsize/dataApd->length;
    numFields /= 2;
    //printf("*******putSliceServer. data, numFields: %d\n",numFields);
    for(i = 0; i < numFields; i++)
    {
        currPathDsc = ((struct descriptor **)dataApd->pointer)[2 * i];
        currDataDsc = ((struct descriptor **)dataApd->pointer)[2 * i+1];
	path = malloc(currPathDsc->length + 1);
	memcpy(path, currPathDsc->pointer, currPathDsc->length);
	path[currPathDsc->length] = 0;
	//printf("%s\t%d\n", path, i);
//	status = putSlice(*expIdx, cpoPath, path, "time", currDataDsc, *time);
        if (strstr(path, "Shape_of") != NULL) {
          //printf("*******putDataServer. path %s => call putData\n",path);
          status = putData(*expIdx, cpoPath, path, currDataDsc);
        }
        else {
          //printf("*******putSliceServer. timeBasePath: %s\n",timeBasePath);
          status = putSlice(*expIdx, cpoPath, path, timeBasePath, currDataDsc, *time);
        }
	if(status)
	    printf("Internal error in putSliceServer %s. %s\n", path, mdsLastErrmsg());
	free(path);
    }
//Obj Segment stuff
    numFields = objSegmentApd->arsize/objSegmentApd->length;
    numFields /= 2;
    //printf("*******putSliceServer. objsegment, numFields: %d\n",numFields);
    for(i = 0; i < numFields; i++)
    {
        currPathDsc = ((struct descriptor **)objSegmentApd->pointer)[2 * i];
        currDataDsc = ((struct descriptor **)objSegmentApd->pointer)[2 * i+1];
	path = malloc(currPathDsc->length + 1);
	memcpy(path, currPathDsc->pointer, currPathDsc->length);
	path[currPathDsc->length] = 0;
	//printf("%s\t%d\n", path, i);
	status = putObjectSegment(*expIdx, cpoPath, path, (void *)currDataDsc, -1);
//ATTENZIONE!!!!!!! NON VIENE COMUNICATO L'INDICE QUI SEMPRE MESSO A -1 MA PER IL REPLACE DOVREBBE ESSERE 0!!!!!
	//status = putSlice(*expIdx, cpoPath, path, currDataDsc, *time);
	if(status)
	    printf("Internal error in putSliceServer %s. %s\n", path, mdsLastErrmsg());
	free(path);
    }
    MdsFree1Dx(&dataXd, 0);
    MdsFree1Dx(&objSegmentXd, 0);
    return 0;
}

int replaceLastSliceServer(int *expIdx, char *cpoPath, char *serData, char *serObjSegment)
{
    int i, numFields, status;
    char *path;
    struct descriptor *currPathDsc, *currDataDsc;


    EMPTYXD(dataXd);
    struct descriptor_a *dataApd;
    EMPTYXD(objSegmentXd);
    struct descriptor_a *objSegmentApd;

//printf("REPLACE LAST SLICE SERVER\n");

    status = MdsSerializeDscIn(serData, &dataXd);
    if(!(status & 1)||!dataXd.pointer)
    {
        printf("INTERNAL ERROR: Cannot deserialize data!!\n");
	return -1;
    }
    status = MdsSerializeDscIn(serObjSegment, &objSegmentXd);
    if(!(status & 1)||!objSegmentXd.pointer)
    {
        printf("INTERNAL ERROR: Cannot deserialize data!!\n");
	return -1;
    }

    dataApd = (struct descriptor_a *)dataXd.pointer;
    objSegmentApd = (struct descriptor_a *)objSegmentXd.pointer;

    if(dataApd->class != CLASS_APD)
    {
        printf("INTERNAL ERROR: Expected APD, got class %d\n", dataApd->class);
	return -1;
    }
    numFields = dataApd->arsize/dataApd->length;
    numFields /= 2;
    for(i = 0; i < numFields; i++)
    {
        currPathDsc = ((struct descriptor **)dataApd->pointer)[2 * i];
        currDataDsc = ((struct descriptor **)dataApd->pointer)[2 * i+1];
	path = malloc(currPathDsc->length + 1);
	memcpy(path, currPathDsc->pointer, currPathDsc->length);
	path[currPathDsc->length] = 0;
	status = replaceLastSlice(*expIdx, cpoPath, path, currDataDsc);
	if(status)
	    printf("Internal error in putDataServer %s. %s\n", path, mdsLastErrmsg());
	free(path);
    }
    MdsFree1Dx(&dataXd, 0);
    numFields = objSegmentApd->arsize/objSegmentApd->length;
    numFields /= 2;
    for(i = 0; i < numFields; i++)
    {
        currPathDsc = ((struct descriptor **)objSegmentApd->pointer)[2 * i];
        currDataDsc = ((struct descriptor **)objSegmentApd->pointer)[2 * i+1];
	path = malloc(currPathDsc->length + 1);
	memcpy(path, currPathDsc->pointer, currPathDsc->length);
	path[currPathDsc->length] = 0;
	status = putObjectSegment(*expIdx, cpoPath, path, (void *)currDataDsc, 0);
	if(status)
	    printf("Internal error in putDataServer %s. %s\n", path, mdsLastErrmsg());
	free(path);
    }
    MdsFree1Dx(&objSegmentXd, 0);
    return 0;
}

int mdsbeginIdsPutRemote(int connId, int expIdx, char *path)
{
     return beginPutDataInfo(expIdx, path);
}

static double *segmentTimes;
static int nSegmentTimes;

int mdsendIdsPutRemote(int connId, int remoteExpIdx, char *path)
{
    char expr[512];
    struct descrip remoteExpIdxDsc;
    struct descrip ansarg;
    struct descrip dataDsc;
    struct descrip segmentDsc;
    struct descrip objSegmentDsc;
    struct descrip timesDsc;
    struct descrip nTimesDsc;
    struct descrip pathDsc;
    struct descrip timeBasePathDsc;
    char timeBasePath[132];
    int currLen;

    EMPTYXD(dataXd);
    EMPTYXD(segmentXd);
    EMPTYXD(objSegmentXd);
    EMPTYXD(serDataXd);
    EMPTYXD(serSegmentXd);
    EMPTYXD(serObjSegmentXd);
    int status;

    sprintf(expr, "imas->putDataServer($1, $2, $3, $4, $5, $6, $7, $8)"); //
    //printf("mdsendIdsPutRemote 1. path: %s, remoteExpIdx: %d\n",path, remoteExpIdx);
    memset(&ansarg, 0, sizeof(ansarg));

    terminatePutDataInfo(remoteExpIdx, path, timeBasePath, &dataXd, &segmentXd, &objSegmentXd);
    //printf("mdsendIdsPutRemote 2. timeBasePath: %s\n",timeBasePath);

    MakeDescrip((struct descrip *)&pathDsc,DTYPE_CSTRING,0,0, path);
    MakeDescrip((struct descrip *)&remoteExpIdxDsc,DTYPE_LONG,0,0,&remoteExpIdx);
    MakeDescrip((struct descrip *)&timesDsc,DTYPE_DOUBLE,1,&nSegmentTimes,segmentTimes);
    MakeDescrip((struct descrip *)&nTimesDsc,DTYPE_LONG,0, 0,&nSegmentTimes);
    MakeDescrip((struct descrip *)&timeBasePathDsc,DTYPE_CSTRING,0,0, timeBasePath);

    status = MdsSerializeDscOut(dataXd.pointer, &serDataXd);
    if(!(status & 1) || !serDataXd.pointer)
    {
	printf("INTERNAL ERROR: Cannot serialize Data\n");
	return -1;
    }

    status = MdsSerializeDscOut(segmentXd.pointer, &serSegmentXd);
    if(!(status & 1) || !serSegmentXd.pointer)
    {
	printf("INTERNAL ERROR: Cannot serialize Segment\n");
	return -1;
    }
    status = MdsSerializeDscOut(objSegmentXd.pointer, &serObjSegmentXd);
    if(!(status & 1) || !serObjSegmentXd.pointer)
    {
	printf("INTERNAL ERROR: Cannot serialize Object Segment\n");
	return -1;
    }
    currLen = ((struct descriptor_a *)serDataXd.pointer)->arsize;
    MakeDescrip((struct descrip *)&dataDsc, DTYPE_UCHAR, 1, &currLen,
    	((struct descriptor_a *)serDataXd.pointer)->pointer);

    currLen = ((struct descriptor_a *)serSegmentXd.pointer)->arsize;
    MakeDescrip((struct descrip *)&segmentDsc, DTYPE_UCHAR, 1, &currLen,
    	((struct descriptor_a *)serSegmentXd.pointer)->pointer);

    currLen = ((struct descriptor_a *)serObjSegmentXd.pointer)->arsize;
    MakeDescrip((struct descrip *)&objSegmentDsc, DTYPE_UCHAR, 1, &currLen,
    	((struct descriptor_a *)serObjSegmentXd.pointer)->pointer);

//   status = MdsValue(connId, expr, &remoteExpIdxDsc, &pathDsc, &dataDsc, &segmentDsc, &objSegmentDsc, &timesDsc, &nTimesDsc, &ansarg, NULL);
   status = MdsValue(connId, expr, &remoteExpIdxDsc, &pathDsc, &timeBasePathDsc, &dataDsc, &segmentDsc, &objSegmentDsc, &timesDsc, &nTimesDsc, &ansarg, NULL);

   //status = putDataServer(remoteExpIdxDsc.ptr, pathDsc.ptr, dataDsc.ptr, segmentDsc.ptr, objSegmentDsc.ptr, timesDsc.ptr, nTimesDsc.ptr);




    MdsFree1Dx(&dataXd, 0);
    MdsFree1Dx(&segmentXd, 0);
    MdsFree1Dx(&objSegmentXd, 0);
    MdsFree1Dx(&serDataXd, 0);
    MdsFree1Dx(&serSegmentXd, 0);
    MdsFree1Dx(&serObjSegmentXd, 0);
    if(!(status & 1))
    {
        sprintf(errmsg, "Remote Access Failed: %s", MdsGetMsg(status));
	return -1;
    }
    if(ansarg.length > 0)
    	free(ansarg.ptr);
    return 0;
}

int mdsbeginIdsPutTimedRemote(int connId, int expIdx, char *path, int samples, double *inTimes)
{
    segmentTimes = malloc(sizeof(double)*samples);
    memcpy(segmentTimes, inTimes, sizeof(double) *samples);
    nSegmentTimes = samples;
    return beginPutDataInfo(expIdx, path);
}

int mdsendIdsPutTimedRemote(int connId, int expIdx, char *path) // , char *timeBasePath
{
    int status;

//    status = mdsendIdsPutRemote(connId, expIdx, path); // , timeBasePath
    if(nSegmentTimes > 0)
    {
        free((char *)segmentTimes);
	nSegmentTimes = 0;
    }
    return status;
}

int mdsbeginIdsPutNonTimedRemote(int connId, int expIdx, char *path)
{
    return beginPutDataInfo(expIdx, path);
}

int mdsendIdsPutNonTimedRemote(int connId, int expIdx, char *path)
{
    return mdsendIdsPutRemote(connId, expIdx, path);
}

int putDataRemote(int connId, int expIdx, char *cpoPath, char *path, struct descriptor *dataDsc)
{
    return insertPutDataInfo(expIdx, cpoPath, path, " ", dataDsc);
}

int putSegmentRemote(int connId, int expIdx, char *cpoPath, char *path, char *timeBasePath, struct descriptor_a *dataDsc, double *times, int nTimes)
//int putSegmentRemote(int connId, int expIdx, char *cpoPath, char *path, struct descriptor_a *dataDsc, double *times, int nTimes)
{
    return insertPutSegmentInfo(expIdx, cpoPath, path, timeBasePath, (struct descriptor *)dataDsc);
//    return insertPutSegmentInfo(expIdx, cpoPath, path, (struct descriptor *)dataDsc);
}

//ATTENZIONE!!! si perde segIdx!!!
int putObjectSegmentRemote(int connId, int expIdx, char *cpoPath, char *path, void *objSegment, int segIdx)
{
    return insertPutObjectSegmentInfo(expIdx, cpoPath, path, objSegment, segIdx);
}

int mdsbeginIdsPutSliceRemote(int connId, int expIdx, char *path)
{
    return beginPutDataInfo(expIdx, path);
}

static double sliceTime;
int mdsendIdsPutSliceRemote(int connId, int remoteExpIdx, char *path)
{
    char expr[512];
    struct descrip remoteExpIdxDsc;
    struct descrip ansarg;
    struct descrip timeDsc;
    struct descrip pathDsc;
    struct descrip dataDsc;
    struct descrip objSegmentDsc;
    struct descrip timeBasePathDsc;
    char timeBasePath[132];
    EMPTYXD(dataXd);
    EMPTYXD(segmentXd);
    EMPTYXD(objSegmentXd);
    EMPTYXD(serDataXd);
    EMPTYXD(serObjSegmentXd);
    int status, currLen;

    sprintf(expr, "imas->putSliceServer($1, $2, $3, $4, $5, $6)");
    memset(&ansarg, 0, sizeof(ansarg));

    terminatePutDataInfo(remoteExpIdx, path, timeBasePath, &dataXd, &segmentXd, &objSegmentXd);
    //printf("mdsendIdsPutSliceRemote 2. timeBasePath: %s\n",timeBasePath);
    MakeDescrip((struct descrip *)&pathDsc,DTYPE_CSTRING,0,0,path);
    MakeDescrip((struct descrip *)&remoteExpIdxDsc,DTYPE_LONG,0,0,&remoteExpIdx);
    MakeDescrip((struct descrip *)&timeDsc,DTYPE_DOUBLE,0, 0,&sliceTime);
    MakeDescrip((struct descrip *)&timeBasePathDsc,DTYPE_CSTRING,0,0, timeBasePath);

    status = MdsSerializeDscOut(dataXd.pointer, &serDataXd);
    if(!(status & 1) || !serDataXd.pointer)
    {
	printf("INTERNAL ERROR: Cannot serialize Data\n");
	return -1;
    }
    currLen = ((struct descriptor_a *)serDataXd.pointer)->arsize;
    MakeDescrip((struct descrip *)&dataDsc, DTYPE_UCHAR, 1, &currLen,
    	((struct descriptor_a *)serDataXd.pointer)->pointer);

    status = MdsSerializeDscOut(objSegmentXd.pointer, &serObjSegmentXd);
    if(!(status & 1) || !serObjSegmentXd.pointer)
    {
	printf("INTERNAL ERROR: Cannot serialize Object segment\n");
	return -1;
    }
    currLen = ((struct descriptor_a *)serObjSegmentXd.pointer)->arsize;
    MakeDescrip((struct descrip *)&objSegmentDsc, DTYPE_UCHAR, 1, &currLen,
    	((struct descriptor_a *)serObjSegmentXd.pointer)->pointer);

//    status = MdsValue(connId, expr, &remoteExpIdxDsc, &pathDsc, &dataDsc, &objSegmentDsc, &timeDsc, &ansarg, NULL);
    status = MdsValue(connId, expr, &remoteExpIdxDsc, &pathDsc, &timeBasePathDsc, &dataDsc, &objSegmentDsc, &timeDsc, &ansarg, NULL);
    //status = putSliceServer(remoteExpIdxDsc.ptr, pathDsc.ptr, dataDsc.ptr, serObjtimeDsc.ptr);


    MdsFree1Dx(&dataXd, 0);
    MdsFree1Dx(&serDataXd, 0);
    MdsFree1Dx(&segmentXd, 0);
    MdsFree1Dx(&objSegmentXd, 0);
    MdsFree1Dx(&serObjSegmentXd, 0);
    if(!(status & 1))
    {
        sprintf(errmsg, "Remote Access Failed: %s", MdsGetMsg(status));
	return -1;
    }
    if(ansarg.length > 0)
    	free(ansarg.ptr);
    return 0;
}

int putSliceRemote(int connId, int expIdx, char *cpoPath, char *path, char *timeBasePath, struct descriptor *dataDsc, double time)
{
    sliceTime = time;
    return insertPutDataInfo(expIdx, cpoPath, path, timeBasePath, dataDsc);
}


int mdsbeginIdsReplaceLastSliceRemote(int connId, int expIdx, char *path)
{
    return beginPutDataInfo(expIdx, path);
}

int mdsendIdsReplaceLastSliceRemote(int connId, int remoteExpIdx, char *path)
{
    char expr[512];
    struct descrip remoteExpIdxDsc;
    struct descrip ansarg;
    struct descrip pathDsc;
    struct descrip dataDsc;
    struct descrip objSegmentDsc;
    char timeBasePath[132];
    EMPTYXD(dataXd);
    EMPTYXD(serDataXd);
    EMPTYXD(serObjSegmentXd);
    EMPTYXD(segmentXd);
    EMPTYXD(objSegmentXd);
    int status, currLen;

    MakeDescrip((struct descrip *)&pathDsc,DTYPE_CSTRING,0,0,path);
    MakeDescrip((struct descrip *)&remoteExpIdxDsc,DTYPE_LONG,0,0,&remoteExpIdx);
    sprintf(expr, "imas->replaceLastSliceServer($1, $2, $3, $4)");
    memset(&ansarg, 0, sizeof(ansarg));

    terminatePutDataInfo(remoteExpIdx, path, timeBasePath, &dataXd, &segmentXd, &objSegmentXd);
    status = MdsSerializeDscOut(dataXd.pointer, &serDataXd);
    if(!(status & 1) || !serDataXd.pointer)
    {
	printf("INTERNAL ERROR: Cannot serialize Data\n");
	return -1;
    }
    currLen = ((struct descriptor_a *)serDataXd.pointer)->arsize;
    MakeDescrip((struct descrip *)&dataDsc, DTYPE_UCHAR, 1, &currLen,
    	((struct descriptor_a *)serDataXd.pointer)->pointer);

    status = MdsSerializeDscOut(objSegmentXd.pointer, &serObjSegmentXd);
    if(!(status & 1) || !serObjSegmentXd.pointer)
    {
	printf("INTERNAL ERROR: Cannot serialize Object segment\n");
	return -1;
    }
    currLen = ((struct descriptor_a *)serObjSegmentXd.pointer)->arsize;
    MakeDescrip((struct descrip *)&objSegmentDsc, DTYPE_UCHAR, 1, &currLen,
    	((struct descriptor_a *)serObjSegmentXd.pointer)->pointer);

    status = MdsValue(connId, expr, &remoteExpIdxDsc, &pathDsc, &dataDsc, &objSegmentDsc, &ansarg, NULL);


    //status = MdsValue(connId, expr, &remoteExpIdxDsc, &pathDsc, dataXd.pointer, &ansarg, NULL);
    MdsFree1Dx(&dataXd, 0);
    MdsFree1Dx(&serDataXd, 0);
    MdsFree1Dx(&segmentXd, 0);
    MdsFree1Dx(&objSegmentXd, 0);
    MdsFree1Dx(&serObjSegmentXd, 0);

    if(!(status & 1))
    {
        sprintf(errmsg, "Remote Access Failed: %s", MdsGetMsg(status));
	return -1;
    }
    if(ansarg.length > 0)
    	free(ansarg.ptr);
    return 0;
}
int replaceLastSliceRemote(int connId, int expIdx, char *cpoPath, char *path, struct descriptor *dataDsc)
{
    return insertPutDataInfo(expIdx, cpoPath, path, " ", dataDsc);
}


int mdsimasOpenEnvServer(char *name, int *shot, int *run, char *user, char *tokamak, char *version)
{
    int expIdx, status;

    status = mdsimasOpenEnv(name, *shot, *run, &expIdx, user, tokamak, version);
    if(!status)
        return expIdx;
    else
        return -1;
}

int mdsimasOpenServer(char *name, int *shot, int *run)
{
    int expIdx, status;


    refreshExpCtx(name, *shot, *run);
    status = mdsimasOpen(name, *shot, *run, &expIdx);
    if(!status)
        return expIdx;
    else
        return -1;
}

int mdsimasCreateEnvServer(char *name, int *shot, int *run, char *user, char *tokamak, char *version)
{
    int expIdx, status;

    status = mdsimasCreateEnv(name, *shot, *run, 0, 0, &expIdx, user, tokamak, version);
    if(!status)
        return expIdx;
    else
        return -1;
}

int mdsimasCreateServer(char *name, int *shot, int *run)
{
    int expIdx, status;

    status = mdsimasCreate(name, *shot, *run, 0, 0, &expIdx);
    if(!status)
        return expIdx;
    else
        return -1;
}


int mdsimasOpenEnvRemote(int connId, char *name, int shot, int run, int *retIdx, char *user, char *tokamak, char *version)
{
    char expr[512];
    int status, errLen;
    struct descrip ansarg;
    struct descrip nameDsc;
    struct descrip shotDsc;
    struct descrip runDsc;
    struct descrip userDsc;
    struct descrip tokamakDsc;
    struct descrip versionDsc;

    sprintf(expr, "imas->mdsimasOpenEnvServer($1, $2, $3, $4, $5, $6)");
    MakeDescrip((struct descrip *)&nameDsc,DTYPE_CSTRING,0,0,name);
    MakeDescrip((struct descrip *)&shotDsc,DTYPE_LONG,0,0,&shot);
    MakeDescrip((struct descrip *)&runDsc,DTYPE_LONG,0,0,&run);
    MakeDescrip((struct descrip *)&userDsc,DTYPE_CSTRING,0,0,user);
    MakeDescrip((struct descrip *)&tokamakDsc,DTYPE_CSTRING,0,0,tokamak);
    MakeDescrip((struct descrip *)&versionDsc,DTYPE_CSTRING,0,0,version);



    status = MdsValue(connId, expr, &nameDsc, &shotDsc, &runDsc, &userDsc, &tokamakDsc, &versionDsc, &ansarg, NULL);


    if(ansarg.dtype == DTYPE_CSTRING)
    {
    	errLen = ansarg.length;
	if(errLen > 256) errLen = 256;
    	strncpy(errmsg, ansarg.ptr, errLen);
	errmsg[errLen] = 0;
	printf("Error in remote OpenEnv: %s\n", errmsg);
	return -1;
    }
    if(ansarg.dtype != DTYPE_LONG)
    {
	sprintf(errmsg, "Internal Error in remote OpenEnv\n");
	printf("Internal Error in remote OpenEnv\n");
	return -1;
    }
    *retIdx =  *((int *)ansarg.ptr);
    if(ansarg.length > 0)
    	free(ansarg.ptr);
    if(*retIdx == -1)
        return -1;
    else
        return 0;
}

int mdsimasOpenRemote(int connId, char *name, int shot, int run, int *retIdx)
{
    char expr[512];
    int status, errLen;
    struct descrip ansarg;
    struct descrip nameDsc;
    struct descrip shotDsc;
    struct descrip runDsc;

    sprintf(expr, "imas->mdsimasOpenServer($1, $2, $3)");
    MakeDescrip((struct descrip *)&nameDsc,DTYPE_CSTRING,0,0,name);
    MakeDescrip((struct descrip *)&shotDsc,DTYPE_LONG,0,0,&shot);
    MakeDescrip((struct descrip *)&runDsc,DTYPE_LONG,0,0,&run);
    status = MdsValue(connId, expr, &nameDsc, &shotDsc, &runDsc, &ansarg, NULL);


    if(ansarg.dtype == DTYPE_CSTRING)
    {
    	errLen = ansarg.length;
	if(errLen > 256) errLen = 256;
    	strncpy(errmsg, ansarg.ptr, errLen);
	errmsg[errLen] = 0;
	printf("Error in remote OpenEnv: %s\n", errmsg);
	return -1;
    }
    if(ansarg.dtype != DTYPE_LONG)
    {
	sprintf(errmsg, "Internal Error in remote OpenEnv\n");
	printf("Internal Error in remote OpenEnv\n");
	return -1;
    }
    *retIdx =  *((int *)ansarg.ptr);
    if(ansarg.length > 0)
    	free(ansarg.ptr);
    if(*retIdx == -1)
    {
	sprintf(errmsg, "Remote OpenEnv failed");
        return -1;
    }
    else
        return 0;
}

int mdsimasCreateEnvRemote(int connId, char *name, int shot, int run, int *retIdx, char *user, char *tokamak, char *version)
{
    char expr[512];
    int status, errLen;
    struct descrip ansarg;
    struct descrip nameDsc;
    struct descrip shotDsc;
    struct descrip runDsc;
    struct descrip userDsc;
    struct descrip tokamakDsc;
    struct descrip versionDsc;

    sprintf(expr, "imas->mdsimasCreateEnvServer($1, $2, $3, $4, $5, $6)");
    MakeDescrip((struct descrip *)&nameDsc,DTYPE_CSTRING,0,0,name);
    MakeDescrip((struct descrip *)&shotDsc,DTYPE_LONG,0,0,&shot);
    MakeDescrip((struct descrip *)&runDsc,DTYPE_LONG,0,0,&run);
    MakeDescrip((struct descrip *)&userDsc,DTYPE_CSTRING,0,0,user);
    MakeDescrip((struct descrip *)&tokamakDsc,DTYPE_CSTRING,0,0,tokamak);
    MakeDescrip((struct descrip *)&versionDsc,DTYPE_CSTRING,0,0,version);
    status = MdsValue(connId, expr, &nameDsc, &shotDsc, &runDsc, &userDsc, &tokamakDsc, &versionDsc, &ansarg, NULL);


    if(ansarg.dtype == DTYPE_CSTRING)
    {
    	errLen = ansarg.length;
	if(errLen > 256) errLen = 256;
    	strncpy(errmsg, ansarg.ptr, errLen);
	errmsg[errLen] = 0;
	printf("Error in remote OpenEnv: %s\n", errmsg);
	return -1;
    }
    if(ansarg.dtype != DTYPE_LONG)
    {
	sprintf(errmsg, "Internal Error in remote OpenEnv\n");
	printf("Internal Error in remote OpenEnv\n");
	return -1;
    }
    *retIdx =  *((int *)ansarg.ptr);
    if(ansarg.length > 0)
    	free(ansarg.ptr);
    if(*retIdx == -1)
        return -1;
    else
        return 0;
}

int mdsimasCreateRemote(int connId, char *name, int shot, int run, int *retIdx)
{
    char expr[512];
    int status, errLen;
    struct descrip ansarg;
    struct descrip nameDsc;
    struct descrip shotDsc;
    struct descrip runDsc;

    sprintf(expr, "imas->mdsimasCreateServer($1, $2, $3)");
    MakeDescrip((struct descrip *)&nameDsc,DTYPE_CSTRING,0,0,name);
    MakeDescrip((struct descrip *)&shotDsc,DTYPE_LONG,0,0,&shot);
    MakeDescrip((struct descrip *)&runDsc,DTYPE_LONG,0,0,&run);
    status = MdsValue(connId, expr, &nameDsc, &shotDsc, &runDsc, &ansarg, NULL);


    if(ansarg.dtype == DTYPE_CSTRING)
    {
    	errLen = ansarg.length;
	if(errLen > 256) errLen = 256;
    	strncpy(errmsg, ansarg.ptr, errLen);
	errmsg[errLen] = 0;
	printf("Error in remote OpenEnv: %s\n", errmsg);
	return -1;
    }
    if(ansarg.dtype != DTYPE_LONG)
    {
	sprintf(errmsg, "Internal Error in remote OpenEnv\n");
	printf("Internal Error in remote OpenEnv\n");
	return -1;
    }
    *retIdx =  *((int *)ansarg.ptr);
    if(ansarg.length > 0)
    	free(ansarg.ptr);
    if(*retIdx == -1)
        return -1;
    else
        return 0;
}

int mdsimasCloseRemote(int connId, int idx, char *name, int shot, int run)
{
    char expr[512];
    int status, errLen;
    struct descrip ansarg;
    struct descrip nameDsc;
    struct descrip shotDsc;
    struct descrip runDsc;
    struct descrip idxDsc;

    sprintf(expr, "imas->mdsimasClose(val($1), $2, $3, $4)");
    MakeDescrip((struct descrip *)&nameDsc,DTYPE_CSTRING,0,0,name);
    MakeDescrip((struct descrip *)&shotDsc,DTYPE_LONG,0,0,&shot);
    MakeDescrip((struct descrip *)&runDsc,DTYPE_LONG,0,0,&run);
    MakeDescrip((struct descrip *)&idxDsc,DTYPE_LONG,0,0,&idx);
    status = MdsValue(connId, expr, &idxDsc, &nameDsc, &shotDsc, &runDsc, &ansarg, NULL);
    if(ansarg.dtype == DTYPE_CSTRING)
    {
    	errLen = ansarg.length;
	if(errLen > 256) errLen = 256;
    	strncpy(errmsg, ansarg.ptr, errLen);
	errmsg[errLen] = 0;
	printf("Error in remote OpenEnv: %s\n", errmsg);
	return -1;
    }
    if(ansarg.dtype != DTYPE_LONG)
    {
	sprintf(errmsg, "Internal Error in remote OpenEnv\n");
	printf("Internal Error in remote OpenEnv\n");
	return -1;
    }
    if(ansarg.length > 0)
    	free(ansarg.ptr);
    return 0;
}




