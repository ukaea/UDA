#include <cstring>
#include <pthread.h>
#include <cstdio>
#include <cstdlib>

//Memory storage of metadata information: presence of data and dimensions

#define INITIAL_CHILDREN_SIZE 100

#define INITIAL_SLICES 10
#define STEP_SLICES 10

extern "C" {
    void putInfo(int expIdx, char *cpoPath, char *path, int exists, int nDims, int *dims);
    void putInfoWithData(int expIdx, char *cpoPath, char *path, int exists, int nDims, int *dims, int itemSize, char type, int dataSize, char *data, int isObject);
    void appendSlice(int expIdx, char *cpoPath, char *path, char *timeBasePath, int nDims, int *dims, int itemSize, char type, int sliceSize, char *slice);
    void appendSliceSet(int expIdx, char *cpoPath, char *path, char *timeBasePath, int nDims, int *dims, int itemSize, char type, int sliceSize, char *slice, int
    numSlices);
    int getInfo(int expIdx, char *cpoPath, char *path, int *exists, int *nDims, int *dims);
    int getInfoWithData(int expIdx, char *cpoPath, char *path, int *exists, int *nDims, int *dims, int *itemSize, char *type, int *dataSize, char **data);
    int getInfoWithDataNoLock(int expIdx, char *cpoPath, char *path, int *exists, int *nDims, int *dims, int *itemSize, char *type, int *dataSize, char **data);
    void invalidateCpoOccurrenceInfo(int expIdx, char *cpoPath, int occurrence);
    void invalidateAllCpoInfos(int expIdx);
    void invalidateCpoInfo(int expIdx, char *cpoPath);
    void invalidateCpoField(int expIdx, char *cpoPath, char *path);
    void internalFlush(int expIdx, char *cpoPath, char *path, char *timeBasePath,int isSliced, int isObject, int nDims, int *dims, int itemSize, int dtype, int dataSize, char *data, int numSlices, int *sliceOffsets);
    void flushInfo(int expIdx);
    void flushCpoInfo(int expIdx, char *cpoName);

    int getInfoNumSlices(int expIdx, char *cpoPath, char *path);
    int getInfoSlice(int expIdx, char *cpoPath, char *path, int sliceIdx, int *exists, char **data);
    void removeAllInfoObjectSlices(int expIdx, char *cpoPath, char *path);
    void appendInfoObjectSlice(int expIdx, char *cpoPath, char *path, char *buf, int size);

#ifdef MONITOR
    void startCacheMonitor(char *name);
#endif

}

static int getFirstSlashOccurrence(char *path)
{
    int i;
    int len = strlen(path);
    for(i = 0; i < len; i++)
	if(path[i] == '/')
	    return i;
    return -1;
}

#ifdef MONITOR
class CacheMonitor;
#endif

class PathLeaf;
class PathNode;
static PathLeaf *createPathLeaf(PathNode *parent, char *name, bool exists, int nDims, int *dims, int itemSize, char type, int dataSize, char *data);

#ifdef MONITOR
static bool report(CacheMonitor *cm, char *cpoPath, char *path, bool exists, bool isSliced, bool isObject, int dataSize, int numSlices);
#endif

class PathNode
{
protected:
    int numChildren;
    int childrenSize;
    PathNode **children;
    PathNode *parent;
public:
    char *name;
    PathNode(PathNode *parent, char *name)
    {
	this->name = new char[strlen(name)+1];
	strcpy(this->name, name);
	children = new PathNode *[INITIAL_CHILDREN_SIZE];
	childrenSize = INITIAL_CHILDREN_SIZE;
	numChildren = 0;
	this->parent = parent;
    }
    ~PathNode()
    {
	delete[] name;
	for(int i = 0; i < numChildren; i++)
	    delete children[i];
	delete[]children;
    }

#ifdef MONITOR
    virtual bool reportStatus(CacheMonitor *cm)
    {
	for(int i = 0; i < numChildren; i++)
	    if(!children[i]->reportStatus(cm))
	    	return false;
	return true;
    }
#endif

    PathNode *getLeaf(char *path)
    {
	int idx, childIdx;
	PathNode *retLeaf;
	bool exists;
	int dims[16];
	if(numChildren == childrenSize - 1)
	{
	    PathNode** newC = new PathNode*[2 * numChildren];
	    memcpy(newC, children, numChildren * sizeof(PathNode *));
	    delete[] children;
	    children = newC;
	    childrenSize = 2 * childrenSize;
	}
        idx = getFirstSlashOccurrence(path);

	if(idx == -1) //this is the last part of the name and refers to a  leaf
	{
	    for(childIdx= 0; childIdx < numChildren; childIdx++)
	    {
		if(!strcmp(children[childIdx]->name, path))
		{
		   return children[childIdx];
		   break;
		}
	    }
	    if(childIdx == numChildren)
	    {
	    	children[numChildren++] = retLeaf = (PathNode *)createPathLeaf(this, path, &exists, 0, dims, 0, 0, 0, 0);
		return retLeaf;
	    }
	}
	else
	{
	    char *name1 = new char[idx + 1];
	    memcpy(name1, path, idx);
	    name1[idx] = 0;
	    for(childIdx = 0; childIdx < numChildren; childIdx++)
	    {
		if(!strcmp(children[childIdx]->name, name1))
		{
		   retLeaf = children[childIdx]->getLeaf(&path[idx+1]);
		   break;
		}
	    }
	    if(childIdx == numChildren)
	    {
	    	children[numChildren] = new PathNode(this, name1);
		retLeaf = children[numChildren]->getLeaf(&path[idx+1]);
		numChildren++;
	    }
	    delete[] name1;
	    return retLeaf;
	}
    }



    virtual void putInfo(char *path, bool exists, int nDims, int *dims, int itemSize, char type, int dataSize, char *data, bool isObject)
    {
//When deleting data (exists false) do not create new nodes
       	PathNode *currLeaf = getLeaf(path);
//Gabriele June 2012Handle deletion of object node containing timed and non timed children
	if(currLeaf->numChildren > 0)
	{
	    for(int i = 0; i < currLeaf->numChildren; i++)
	    {
	    	currLeaf->children[i]->putInfo("", exists, nDims, dims, itemSize, type, dataSize, data, isObject);
	    }
	}
	else
	    currLeaf->putInfo("", exists, nDims, dims, itemSize, type, dataSize, data, isObject);
    }

    virtual bool getInfo(char *path, bool *exists, int *nDims, int *dims, int *itemSize, char *type, int *dataSize, char **data)
    {
   	int idx;
	char *name1;
        idx = getFirstSlashOccurrence(path);
	if(idx == -1)
	{
	    name1 = new char[strlen(path)+1];
	    strcpy(name1, path);
	}
	else
	{
	    name1 = new char[idx + 1];
	    memcpy(name1, path, idx);
	    name1[idx] = 0;
	}
	for(int i = 0; i < numChildren; i++)
	{
	    if(!strcmp(name1, children[i]->name))
	    {
		delete[]name1;
		return children[i]->getInfo(&path[idx+1], exists, nDims, dims, itemSize, type, dataSize, data);
	    }
	}
	delete[]name1;
	return false;
    }

    virtual void appendSliceSet(char *path, char *timeBasePath, int nDims, int *dims, int itemSize, char type, int sliceSize, char *slice, int numSlices)
    {
        PathNode *leaf = getLeaf(path);
	leaf->appendSliceSet("", timeBasePath, nDims, dims, itemSize, type, sliceSize, slice, numSlices);
    }

    virtual void appendSlice(char *path, char *timeBasePath, int nDims, int *dims, int itemSize, char type, int sliceSize, char *slice)
    {
        PathNode *leaf = getLeaf(path);
	leaf->appendSlice("", timeBasePath, nDims, dims, itemSize, type, sliceSize, slice);
    }

    virtual void removeAllInfoObjectSlices(char *path)
    {
        PathNode *leaf = getLeaf(path);
	leaf->removeAllInfoObjectSlices("");
    }


    virtual void appendInfoObjectSlice(char *path, char *buf, int size)
    {
        PathNode *leaf = getLeaf(path);
	leaf->appendInfoObjectSlice("", buf, size);
    }


    virtual void invalidateField(char *path)
    {
   	int idx;
	char *name1;
        idx = getFirstSlashOccurrence(path);
	if(idx == -1)
	{
	    name1 = new char[strlen(path)+1];
	    strcpy(name1, path);
	}
	else
	{
	    name1 = new char[idx + 1];
	    memcpy(name1, path, idx);
	    name1[idx] = 0;
	}
	for(int i = 0; i < numChildren; i++)
	{
	    if(!strcmp(name1, children[i]->name))
	    {
		delete[]name1;
		if(idx == -1) //Leaf child
		{
		    delete children[i];
		    for(int j = i; j < numChildren - 1; j++)
			children[j] = children[j+1];
		    numChildren--;
		    return;
		}
		else
		{
		    children[i]->invalidateField(&path[idx+1]);
		    return;
		}
	    }
	}
	delete[]name1; //Not found, who cares
    }

    virtual int getNumSlices(char *path)
    {
   	int idx;
	char *name1;
        idx = getFirstSlashOccurrence(path);
	if(idx == -1)
	{
	    name1 = new char[strlen(path)+1];
	    strcpy(name1, path);
	}
	else
	{
	    name1 = new char[idx + 1];
	    memcpy(name1, path, idx);
	    name1[idx] = 0;
	}
	for(int i = 0; i < numChildren; i++)
	{
	    if(!strcmp(name1, children[i]->name))
	    {
		delete[]name1;
		return children[i]->getNumSlices(&path[idx+1]);
	    }
	}
	delete[]name1;
	return -1;
    }

    virtual bool getInfoSlice(char *path, int sliceIdx, bool *exists, char **data)
    {
   	int idx;
	char *name1;
        idx = getFirstSlashOccurrence(path);
	if(idx == -1)
	{
	    name1 = new char[strlen(path)+1];
	    strcpy(name1, path);
	}
	else
	{
	    name1 = new char[idx + 1];
	    memcpy(name1, path, idx);
	    name1[idx] = 0;
	}
	for(int i = 0; i < numChildren; i++)
	{
	    if(!strcmp(name1, children[i]->name))
	    {
		delete[]name1;
		return children[i]->getInfoSlice(&path[idx+1], sliceIdx, exists, data);
	    }
	}
	delete[]name1;
	return false;
    }

#ifdef MONITOR
    virtual char *getCpoPath()
    {
    	if(!parent)
	{
	    char *cpoPath = new char[strlen(name)+1];
	    strcpy(cpoPath, name);
	    return cpoPath;
	}
	else
	    return parent->getCpoPath();
    }
#endif

    virtual char *getPath()
    {
        char *retPath;
        if(!parent)
	{
	    printf("FATAL ERROR in metadata: getPath called with parent null\n");
	    exit(0);
	}
	//If it is the child of a TopCpo instance
	if(!parent->parent)
	{
	    retPath = new char[strlen(name) + 1];
	    strcpy(retPath, name);
	}
	else
	{
	    char *parentPath = parent->getPath();
	    retPath = new char[2 + strlen(name) + strlen(parentPath)];
	    sprintf(retPath, "%s/%s", parentPath, name);
	    delete [] parentPath;
	}
	return retPath;
    }

    virtual void flush(int expIdx, char *cpoPath)
    {
	for(int i = 0; i < numChildren; i++)
	{
	    children[i]->flush(expIdx, cpoPath);
	}
    }
};

class PathLeaf:public PathNode
{
public:
    bool exists;
    int nDims;
    int dims[16];
    unsigned int dataSize;
    unsigned int bufferSize;
    char type;
    int itemSize;
    char *data;
    bool isSliced;
//Arrays of objects are stored in a different way: no control on dimension and variable slice dimension
    bool isObject;
    int numSlices;
    int *sliceOffsets;
    int sliceOffsetSize;
//Added to keep track of the original path name for time information
    char *timeBasePath;

    PathLeaf(PathNode *parent, char *name, bool exists, int nDims, int *dims, int itemSize = 0, char type = 0, int dataSize = 0, char *data = 0):PathNode(parent, name)
    {
	if(getFirstSlashOccurrence(name) != -1)
	{
	    printf("FATAL INTERNAL ERROR IN METADATA MANAGEMENT: PathLeaf::Create!!!!\n");
	    exit(0);
	}
	this->exists = exists;
	this->nDims = nDims;
	this->type = type;
	this->itemSize = itemSize;
	for(int i = 0; i < nDims; i++)
	    this->dims[i] = dims[i];
	for(int i = nDims; i < 16; i++)
	    this->dims[i] = 0;
	this->dataSize = dataSize;
	bufferSize = 0;
	if(data && dataSize > 0)
	{
	    this->data = new char[dataSize];
	    memcpy(this->data, data, dataSize);
	    bufferSize = dataSize;
	}
	isSliced = false;
	isObject = false;
	numSlices = 0;
	sliceOffsetSize = 0;
	timeBasePath = 0;
    }
    ~PathLeaf()
    {
	if(dataSize > 0)
	    delete[] data;
	if(isObject && sliceOffsetSize > 0)
	    delete [] sliceOffsets;
	if(timeBasePath)
	    delete[]timeBasePath;
    }

#ifdef MONITOR
    virtual bool reportStatus(CacheMonitor *cm)
    {
        if(numChildren > 0) //For timed and non timed object fields
	{
	    for(int i = 0; i < numChildren; i++)
	    	if(!children[i]->reportStatus(cm))
		    return false;
	    return true;
	}
	else
	{
	    char *path = getPath();
	    char *cpoPath = getCpoPath();
	    bool res =  report(cm, cpoPath, path, exists, isSliced, isObject, dataSize, numSlices);
	    delete [] path;
	    delete [] cpoPath;
	    return res;
	}
    }
#endif

    void putInfo(char *name, bool exists, int nDims, int *dims, int itemSize, char type, int dataSize, char *data, bool isObject)
    {
/*	if(strcmp(this->name, name)|| getFirstSlashOccurrence(name) != -1)
	{
	    printf("FATAL INTERNAL ERROR IN METADATA MANAGEMENT: PathLeaf::putInfo\n");
	    exit(0);
	}
*/

	if(!exists && this->isObject && numSlices > 0)
	{
            this->dataSize = 0;
	    this->numSlices = 0;
	    this->exists = exists;
	    return;
	}

	this->exists = exists;
	this->nDims = nDims;
	this->type = type;
	this->itemSize = itemSize;
	for(int i = 0; i < nDims; i++)
	    this->dims[i] = dims[i];
	if(this->dataSize > 0)
	    delete[] this->data;
	this->dataSize = dataSize;
	if(dataSize > 0)
	{
	    this->data = new char[dataSize];
	    memcpy(this->data, data, dataSize);
	    bufferSize = dataSize;
	}
	this->isObject = isObject;

    }
    virtual bool getInfo(char *name, bool *exists, int *nDims, int *dims, int *itemSize, char *type, int *dataSize, char **data)
    {

    //printf("virtual GET INFO FOR %s EXISTS: %d dataSIze: %d NUM CHILDREN: %d\n", this->name, this->exists, this->dataSize, numChildren);
    	if(numChildren > 0) //Handle object components which may be created as leaf even when having timed and nontimed children
	{
	    return PathNode::getInfo(name, exists, nDims, dims, itemSize, type, dataSize, data);
      	}
	if(strcmp(this->name, name)) return false;
	*exists = this->exists;
	*nDims = this->nDims;
	for(int i = 0; i < this->nDims; i++)
	    dims[i] = this->dims[i];
	if(dataSize)
	{
	    *dataSize = this->dataSize;
	    if(data) *data = this->data;
	    *type = this->type;
	    *itemSize = this->itemSize;
	}
	return true;
    }
    virtual void appendSlice(char *name, char *timeBasePath, int nDims, int *dims, int itemSize, char type, int sliceSize, char *slice)
    {
/*	if(strcmp(this->name, name)|| getFirstSlashOccurrence(name) != -1)
	{
	    printf("FATAL INTERNAL ERROR IN METADATA MANAGEMENT: PathLeaf::appendSlice\n");
	    exit(0);
	}
*/
	if(!exists)
	{
//printf("APPEND %s SLICE FROM SCRATCH!!!!!!!\n", this->name);
	    this->exists = true;
	    this->nDims = nDims+1;
	    this->type = type;
	    this->itemSize = itemSize;
	    for(int i = 0; i < nDims; i++)
	        this->dims[i] = dims[i];
	    this->dims[nDims] = 1;
	    if(this->dataSize > 0)
	        delete[] this->data;
	    this->dataSize = sliceSize;
	    if(dataSize > 0)
	    {
	    	this->bufferSize = sliceSize  * INITIAL_SLICES;
	        this->data = new char[this->bufferSize];
	        memcpy(this->data, slice, sliceSize);
	    }
#ifdef MONITOR
	    this->numSlices = 1;
#endif

	}
	else
	{
	//Test consistency
	    int currSliceSize = itemSize;
	    for(int i = 0; i < this->nDims - 1; i++)
	        currSliceSize *= this->dims[i];
	    if(currSliceSize != sliceSize)
	    {
	        printf("FATAL ERROR IN METADATA MANAGEMENT: Wrong Slice size. Expected: %d, Actual: %d\n", currSliceSize, sliceSize);
	        exit(0);
	    }
	    if(bufferSize < dataSize + sliceSize)
	    {//Make room for more STEP_SLICES slices
	        char *newBuf = new char[dataSize + STEP_SLICES * sliceSize];
	        memcpy(newBuf, data, dataSize);
	        delete[] data;
	        data = newBuf;
	        bufferSize = dataSize + STEP_SLICES * sliceSize;
	    }
	    memcpy(&data[dataSize], slice, sliceSize);
	    dataSize += sliceSize;
	    this->dims[this->nDims - 1]++;

#ifdef MONITOR
	    this->numSlices++;
#endif

	}
	isSliced = true;
//Gabriele: added to record timeBasePath
	if(this->timeBasePath)
	    delete[] this->timeBasePath;
	this->timeBasePath = new char[strlen(timeBasePath) + 1];
	strcpy(this->timeBasePath, timeBasePath);
//////////////////
    }
    virtual void appendSliceSet(char *name, char *timeBasePath, int nDims, int *dims, int itemSize, char type, int sliceSize, char *slice, int numSlices)
    {
/*	if(strcmp(this->name, name)|| getFirstSlashOccurrence(name) != -1)
	{
	    printf("FATAL INTERNAL ERROR IN METADATA MANAGEMENT: PathLeaf::appendSlice\n");
	    exit(0);
	}
*/

//Gabriele Oct 2016: appendSliceSet is called ONLY by putTimedXXX and therefore the content is NOT appended to cache content, but replaces it


//	if(!exists) Gabriele Oct 2016 force always replacement of existing data
	{
//printf("APPEND SLICE SET FROM SCRATCH!!!!!!!\n");
	    this->exists = true;
	    this->nDims = nDims;
	    this->type = type;
	    this->itemSize = itemSize;
	    for(int i = 0; i < nDims; i++)
	        this->dims[i] = dims[i];
	    if(this->dataSize > 0)
	        delete[] this->data;
	    this->dataSize = sliceSize;
	    if(dataSize > 0)
	    {
	    	this->bufferSize = sliceSize  * INITIAL_SLICES;
	        this->data = new char[this->bufferSize];
	        memcpy(this->data, slice, sliceSize);
	    }

#ifdef MONITOR
	    this->numSlices = numSlices;
#endif

	}
/* Gabriele Oct 2016 - removed

	else
	{
	//Test consistency
	    int currSliceSize = itemSize;
	    for(int i = 0; i < this->nDims - 1; i++)
	        currSliceSize *= dims[i];
	    if(currSliceSize != sliceSize/numSlices)
	    {
	        printf("FATAL ERROR IN METADATA MANAGEMENT: Wrong Slice size. Expected: %d, Actual: %d\n", currSliceSize, sliceSize);
	        exit(0);
	    }
	    if(bufferSize < dataSize + sliceSize)
	    {//Make room for more STEP_SLICES slices
	        char *newBuf = new char[dataSize + STEP_SLICES * sliceSize];
	        memcpy(newBuf, data, dataSize);
	        delete[] data;
	        data = newBuf;
	        bufferSize = dataSize + STEP_SLICES * sliceSize;
	    }
	    memcpy(&data[dataSize], slice, sliceSize);
	    dataSize += sliceSize;
	    dims[this->nDims - 1] += numSlices;

#ifdef MONITOR
	    this->numSlices += numSlices;
#endif
	}
*/

	isSliced = true;
//Gabriele: added to record timeBasePath
	if(this->timeBasePath)
	    delete[] this->timeBasePath;
	this->timeBasePath = new char[strlen(timeBasePath) + 1];
	strcpy(this->timeBasePath, timeBasePath);
//////////////////
    }
    virtual void removeAllInfoObjectSlices(char *path)
    {
        dataSize = 0;
	numSlices = 0;
	isObject = true;
    }
    virtual void appendInfoObjectSlice(char *path, char *buf, int size)
    {
    	//Check offsets array dimension
	exists = true;
	isObject = true;
	isSliced = true;

	if(sliceOffsetSize < numSlices + 1)
	{
	    int *newSliceOffsets = new int[sliceOffsetSize + STEP_SLICES];
	    if(numSlices > 0)
	    {
	    	memcpy(newSliceOffsets, sliceOffsets, numSlices * sizeof(int));
	    	delete[] sliceOffsets;
	    }
	    sliceOffsets = newSliceOffsets;
	    sliceOffsetSize += STEP_SLICES;
	}
	//Check Buffer array dimension
	if(bufferSize < dataSize + size)
	{//Make room for more STEP_SLICES slices assuming the passed size
	    char *newBuf = new char[dataSize + STEP_SLICES * size];
	    if(dataSize > 0)
	    {
	    	memcpy(newBuf, data, dataSize);
	    	delete[] data;
	    }
	    data = newBuf;
	    bufferSize = dataSize + STEP_SLICES * size;
	}
	memcpy(&data[dataSize], buf, size);
	sliceOffsets[numSlices] = dataSize;
	dataSize += size;
	numSlices++;
    }



    virtual void invalidate(char *name)
    {
	printf("FATAL ERROR IN METADATA MANAGEMENT: Invalidate called for leaf\n");
	exit(0);
    }
    virtual void flush(int expIdx, char *cpoPath)
    {
    	if(numChildren > 0) //Handle object components which may be created as leaf even when having timed and nontimed children
	{
	    for(int i = 0; i < numChildren; i++)
	    {
	    	children[i]->flush(expIdx, cpoPath);
	    }
      	}
	else
	{
    	    char *path = getPath();
//Gabriele: Use saved information about timebase path
//    	    internalFlush(expIdx, cpoPath, path, "dummyTimeBasePath", isSliced, isObject, nDims, dims, itemSize, type, dataSize, data, numSlices, sliceOffsets);
    	    internalFlush(expIdx, cpoPath, path, timeBasePath, isSliced, isObject, nDims, dims, itemSize, type, dataSize, data, numSlices, sliceOffsets);
    	    delete [] path;
	}
    }

    virtual int getNumSlices(char *name)
    {
    	if(numChildren > 0) //Handle object components which may be created as leaf even when having timed and nontimed children
	{
	    return PathNode::getNumSlices(name);
      	}
	else
	{
	    if(strcmp(this->name, name)|| getFirstSlashOccurrence(name) != -1)
	    {
	        printf("FATAL INTERNAL ERROR IN METADATA MANAGEMENT: PathLeaf::getNumSlices\n");
	        exit(0);
	    }
	    return numSlices;
	}
    }
    virtual bool getInfoSlice(char *path, int sliceIdx, bool *exists, char **retData)
    {
    	if(numChildren > 0) //Handle object components which may be created as leaf even when having timed and nontimed children
	{
	    return PathNode::getInfoSlice(path, sliceIdx, exists, retData);
      	}
	if(strcmp(this->name, name)|| getFirstSlashOccurrence(name) != -1)
	{
	    printf("FATAL INTERNAL ERROR IN METADATA MANAGEMENT: PathLeaf::getInfoSlice\n");
	    exit(0);
	}
	*exists = this->exists;
	if(sliceIdx >= numSlices)
	    *exists = false;
	else
	    *retData = &data[sliceOffsets[sliceIdx]];
	return true;
    }




};
static PathLeaf *createPathLeaf(PathNode *parent, char *name, bool exists, int nDims, int *dims, int itemSize, char type, int dataSize, char *data)
{
    return new PathLeaf(parent, name, exists, nDims, dims, itemSize, type, dataSize, data);
}

class TopCpo:public PathNode
{
public:
    int treeIdx;
    bool enabled;
    TopCpo(char *name, int treeIdx):PathNode(0, name)
    {
	this->treeIdx = treeIdx;
    }
    bool corresponds(int treeIdx, char *name)
    {
	if(name)
	    return !strcmp(this->name, name) && this->treeIdx == treeIdx;
	else
	    return this->treeIdx == treeIdx;
    }
    virtual void flush(int expIdx, char *cpoPath)
    {
        printf("FATAL ERROR in MetaData: flush() called for TopCpo\n");
    }
    void flush()
    {
	for(int i = 0; i < numChildren; i++)
	{
	    children[i]->flush(treeIdx, name);
	}
    }

};

////////////////////////////End Class Definitions///////////////////////////
static TopCpo **cpos;
int cpoSize = 0;
int numCpos = 0;
static pthread_mutex_t mutex = PTHREAD_MUTEX_INITIALIZER;

static void putInfoLocal(int expIdx, char *cpoPath, char *path, int exists, int nDims, int *dims, bool isObject = false, int itemSize = 0, char type = 0, int dataSize = 0, char *data = 0)
{
    int idx;
    pthread_mutex_lock(&mutex);
    TopCpo *cpo;
    for(idx = 0; idx < numCpos; idx++)
    {
	if(cpos[idx]->corresponds(expIdx, cpoPath))
	{
	    cpo = cpos[idx];
	    break;
	}
    }
    if(idx == numCpos) //First allocation
    {
	if(numCpos == cpoSize)
	{
	    TopCpo **newCpos;
	    if(cpoSize == 0)
	    {
		cpos = new TopCpo*[INITIAL_CHILDREN_SIZE];
		cpoSize = INITIAL_CHILDREN_SIZE;
	    }
	    else
	    {
		newCpos = new TopCpo*[2 * cpoSize];
	    	memcpy(newCpos, cpos, cpoSize * sizeof(TopCpo *));
	    	cpoSize = 2 * cpoSize;
		cpos = newCpos;
	    }
	}
	cpo = cpos[numCpos] = new TopCpo(cpoPath, expIdx);
	numCpos++;
    }
    cpo->putInfo(path, exists, nDims, dims, itemSize, type, dataSize, data, isObject);
    pthread_mutex_unlock(&mutex);
}
void putInfo(int expIdx, char *cpoPath, char *path, int exists, int nDims, int *dims)
{
    putInfoLocal(expIdx, cpoPath, path, exists, nDims, dims);
}
void putInfoWithData(int expIdx, char *cpoPath, char *path, int exists, int nDims, int *dims, int itemSize, char type, int dataSize, char *data, int isObject)
{
    putInfoLocal(expIdx, cpoPath, path, exists, nDims, dims, (isObject)?true:false, itemSize, type, dataSize, data);
}
void removeAllInfoObjectSlices(int expIdx, char *cpoPath, char *path)
{
    int idx;
    pthread_mutex_lock(&mutex);

//Gabriele May 2016: woraround for wrongly passed pat starting with /
    if(path && path[0] == '/')
	path++;


    TopCpo *cpo;
    for(idx = 0; idx < numCpos; idx++)
    {
	if(cpos[idx]->corresponds(expIdx, cpoPath))
	{
	    cpo = cpos[idx];
	    break;
	}
    }
    if(idx == numCpos) //First allocation
    {
	if(numCpos == cpoSize)
	{
	    TopCpo **newCpos;
	    if(cpoSize == 0)
	    {
		cpos = new TopCpo*[INITIAL_CHILDREN_SIZE];
		cpoSize = INITIAL_CHILDREN_SIZE;
	    }
	    else
	    {
		newCpos = new TopCpo*[2 * cpoSize];
	    	memcpy(newCpos, cpos, cpoSize * sizeof(TopCpo *));
	    	cpoSize = 2 * cpoSize;
		cpos = newCpos;
	    }
	}
	cpo = cpos[numCpos] = new TopCpo(cpoPath, expIdx);
	numCpos++;
    }
    cpo->removeAllInfoObjectSlices(path);
    pthread_mutex_unlock(&mutex);
}

void appendInfoObjectSlice(int expIdx, char *cpoPath, char *path, char *buf, int size)
{
    int idx;
    pthread_mutex_lock(&mutex);
    TopCpo *cpo;
    for(idx = 0; idx < numCpos; idx++)
    {
	if(cpos[idx]->corresponds(expIdx, cpoPath))
	{
	    cpo = cpos[idx];
	    break;
	}
    }
    if(idx == numCpos) //First allocation
    {
	if(numCpos == cpoSize)
	{
	    TopCpo **newCpos;
	    if(cpoSize == 0)
	    {
		cpos = new TopCpo*[INITIAL_CHILDREN_SIZE];
		cpoSize = INITIAL_CHILDREN_SIZE;
	    }
	    else
	    {
		newCpos = new TopCpo*[2 * cpoSize];
	    	memcpy(newCpos, cpos, cpoSize * sizeof(TopCpo *));
	    	cpoSize = 2 * cpoSize;
		cpos = newCpos;
	    }
	}
	cpo = cpos[numCpos] = new TopCpo(cpoPath, expIdx);
	numCpos++;
    }
    cpo->appendInfoObjectSlice(path, buf, size);
    pthread_mutex_unlock(&mutex);
}





static int getInfoLocal(int expIdx, char *cpoPath, char *path, int *exists, int *nDims, int *dims, int *itemSize = 0, char *type = 0, int *dataSize = 0, char **data = 0, bool lock = true)
{
    //printf("getInfoLocal, looking for %s\n", cpoPath);
    if(lock) pthread_mutex_lock(&mutex);
    for(int idx = 0; idx < numCpos; idx++)
    {
	bool existsB;
	if(cpos[idx]->corresponds(expIdx, cpoPath))
	{
	    int res = cpos[idx]->getInfo(path, &existsB, nDims, dims, itemSize, type, dataSize, data);
	    if(res)
	        *exists = (existsB)?1:0;
	    pthread_mutex_unlock(&mutex);
	    return (res)?1:0;
	}
    }
    if(lock) pthread_mutex_unlock(&mutex);
    return 0;
}

int getInfoNumSlices(int expIdx, char *cpoPath, char *path)
{
    pthread_mutex_lock(&mutex);
    for(int idx = 0; idx < numCpos; idx++)
    {
	if(cpos[idx]->corresponds(expIdx, cpoPath))
	{
	    int numSlices =  cpos[idx]->getNumSlices(path);
            pthread_mutex_unlock(&mutex);
	    return numSlices;
	}
    }
    pthread_mutex_unlock(&mutex);
    return -1; //Info not found
}

int getInfoSlice(int expIdx, char *cpoPath, char *path, int sliceIdx, int *exists, char **data)
{
    char *slice;
    pthread_mutex_lock(&mutex);
    for(int idx = 0; idx < numCpos; idx++)
    {
	bool existsB;
	if(cpos[idx]->corresponds(expIdx, cpoPath))
	{
	    int res = cpos[idx]->getInfoSlice(path, sliceIdx, &existsB, data);
	    if(res)
	        *exists = (existsB)?1:0;
	    pthread_mutex_unlock(&mutex);
	    return (res)?1:0;
	}
    }
    pthread_mutex_unlock(&mutex);
    return 0;
}



int getInfo(int expIdx, char *cpoPath, char *path, int *exists, int *nDims, int *dims)
{
    return getInfoLocal(expIdx, cpoPath, path, exists, nDims, dims);
}
int getInfoWithDataNoLock(int expIdx, char *cpoPath, char *path, int *exists, int *nDims, int *dims, int *itemSize, char *type, int *dataSize, char **data)
{
    return getInfoLocal(expIdx, cpoPath, path, exists, nDims, dims, itemSize, type, dataSize, data, false);
}
int getInfoWithData(int expIdx, char *cpoPath, char *path, int *exists, int *nDims, int *dims, int *itemSize, char *type, int *dataSize, char **data)
{
    return getInfoLocal(expIdx, cpoPath, path, exists, nDims, dims, itemSize, type, dataSize, data);
}

void appendSlice(int expIdx, char *cpoPath, char *path, char *timeBasePath, int nDims, int *dims, int itemSize, char type, int sliceSize, char *slice)
{

    pthread_mutex_lock(&mutex);
    for(int idx = 0; idx < numCpos; idx++)
    {
	if(cpos[idx]->corresponds(expIdx, cpoPath))
	{
	    cpos[idx]->appendSlice(path, timeBasePath, nDims, dims, itemSize, type, sliceSize, slice);
	    pthread_mutex_unlock(&mutex);
	    return;
	}
    }
    pthread_mutex_unlock(&mutex);
}

void appendSliceSet(int expIdx, char *cpoPath, char *path, char *timeBasePath, int nDims, int *dims, int itemSize, char type, int sliceSize, char *slice, int numSlices)
{
    pthread_mutex_lock(&mutex);
    for(int idx = 0; idx < numCpos; idx++)
    {
	if(cpos[idx]->corresponds(expIdx, cpoPath))
	{
	    cpos[idx]->appendSliceSet(path, timeBasePath, nDims, dims, itemSize, type, sliceSize, slice, numSlices);
	    pthread_mutex_unlock(&mutex);
	    return;
	}
    }
    pthread_mutex_unlock(&mutex);
}

void invalidate(int expIdx, char *name = 0)
{
    pthread_mutex_lock(&mutex);
    int idx, idx1;
    idx = 0;
    while(idx < numCpos)
    {
	if(cpos[idx]->corresponds(expIdx, name))
	{
	    delete cpos[idx];
	    for(idx1 = idx; idx1 < numCpos - 1; idx1++)
		cpos[idx1] = cpos[idx1+1];
	    numCpos--;
	}
	else
	    idx++;
    }
    pthread_mutex_unlock(&mutex);
}

void flushInfo(int expIdx)
{
    pthread_mutex_lock(&mutex);
    int idx, idx1;
    for(idx = 0; idx < numCpos; idx++)
    {
	if(cpos[idx]->treeIdx == expIdx)
	{
	    cpos[idx]->flush();
	}
    }
    pthread_mutex_unlock(&mutex);
}

void flushCpoInfo(int expIdx, char *cpoPath)
{
    pthread_mutex_lock(&mutex);
    int idx, idx1;
    for(idx = 0; idx < numCpos; idx++)
    {
	if(cpos[idx]->corresponds(expIdx, cpoPath))
	{
	    cpos[idx]->flush();
	}
    }
    pthread_mutex_unlock(&mutex);
}




void invalidateCpoField(int expIdx, char *cpoPath, char *path)
{
    pthread_mutex_lock(&mutex);
    int idx, idx1;
    for(idx = 0; idx < numCpos; idx++)
    {
	if(cpos[idx]->corresponds(expIdx, cpoPath))
	{
	    cpos[idx]->invalidateField(path);
	    break;
	}
    }
    pthread_mutex_unlock(&mutex);
}

void invalidateCpoInfo(int expIdx, char *cpoPath)
{
    invalidate(expIdx, cpoPath);
}

void invalidateAllCpoInfos(int expIdx)
{
    invalidate(expIdx);
}


#ifdef MONITOR
//////////////////////////////////Cache Monitoring support///////////////////////////////////////////
#include <sys/types.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <netdb.h>
#include <pthread.h>


class CacheMonitor
{
    int port;
    int sock;
    char *name;

    public:
    	CacheMonitor(char *name)
	{
	    this->name = new char[strlen(name) + 1];
	    strcpy(this->name, name);
	    port = 4444;
	}
    	bool report(char *cpoPath, char *path, bool exists, bool isSliced, bool isObject, int dataSize, int numSlices)
	{
	//Used protocol: name length(4 bytes), name, exists(1 byte), isSliced(1 byte), dataSize(4 bytes) numSlices(4 bytes)
	    unsigned int pathLenH, cpoPathLenH, dataSizeH, numSlicesH;
	    cpoPathLenH = htonl(strlen(cpoPath));
	    pathLenH = htonl(strlen(path));
	    dataSizeH = htonl(dataSize);
	    numSlicesH = htonl(numSlices);
	    if(send(sock, &cpoPathLenH, sizeof(int), 0)==-1) return false;
	    if(send(sock, cpoPath, strlen(cpoPath), 0)==-1) return false;
	    if(send(sock, &pathLenH, sizeof(int), 0)==-1) return false;
	    if(send(sock, path, strlen(path), 0)==-1) return false;
	    if(send(sock, &exists, 1, 0)==-1) return false;
	    if(send(sock, &isSliced, 1, 0)==-1) return false;
	    if(send(sock, &dataSizeH, sizeof(int), 0)==-1) return false;
	    if(send(sock, &numSlicesH, sizeof(int), 0)==-1) return false;
	    return true;
	}
	void startServer()
	{
	    struct sockaddr_in sin ;
            struct hostent *hp;
            if (( hp = gethostbyname ("localhost")) == 0)
	    {
                perror (" gethostbyname ");
		return;
            }
            memset (& sin , 0, sizeof ( sin ));
 	    sin.sin_family = AF_INET ;
	    sin.sin_addr.s_addr = (( struct in_addr *)( hp->h_addr ))->s_addr;
	    sin.sin_port = htons ( port );
	    if (( sock = socket ( AF_INET , SOCK_STREAM , 0)) == -1)
	    {
		perror (" socket ");
		return;
	    }
	    if ( connect (sock ,( struct sockaddr *)& sin , sizeof ( sin )) == -1)
            {
  	        perror (" connect ");
		return;
	    }
	    char nameLen = strlen(name);
	    if(send(sock, &nameLen, 1, 0) == -1) return;
	    if(send(sock, name, nameLen, 0) == -1) return;
	    struct timespec time;
	    time.tv_sec = 0;
	    time.tv_nsec = 500000000;
	    while (true)
	    {
		nanosleep(&time, 0);
    		pthread_mutex_lock(&mutex);
    		int idx, idx1;
    		for(idx = 0; idx < numCpos; idx++)
		{
		    if(!cpos[idx]->reportStatus(this))
		    {
    		        pthread_mutex_unlock(&mutex);
		    	return;
		    }
		}
    		pthread_mutex_unlock(&mutex);
		int minusone = -1;
		if(send(sock, &minusone, sizeof(int), 0) == -1) return;
	    }
	}
};

static bool report(CacheMonitor *cm, char *cpoPath, char *path, bool exists, bool isSliced, bool isObject, int dataSize, int numSlices)
{
    if(!path || strlen(path) == 0) return true;
    return cm->report(cpoPath, path, exists, isSliced, isObject, dataSize, numSlices);
}

static void *reportStatus(void *cm)
{
    ((CacheMonitor *)cm)->startServer();
}
static CacheMonitor *cacheMonitor;
void startCacheMonitor(char *name)
{
    if(cacheMonitor) return;
    cacheMonitor = new CacheMonitor(name);
    pthread_t thread;
    pthread_create(&thread, NULL , reportStatus , cacheMonitor);
}

#endif
