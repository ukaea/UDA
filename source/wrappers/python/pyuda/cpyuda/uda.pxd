from libcpp cimport bool as cbool

cdef extern from "uda/client.h":
    ctypedef struct NTREE
    ctypedef struct LOGMALLOCLIST
    ctypedef struct PUTDATA_BLOCK
    ctypedef enum UdaType:
        pass

    const char* udaGetServerHost();
    int udaGetServerPort();
    const char* udaGetBuildVersion();
    const char* udaGetBuildDate();
    void udaFree(int handle);
    void udaCloseAllConnections();

    int udaGetAPI(const char* data_object, const char* data_source);
    int udaGetBatchAPI(const char** signals, const char** sources, int count, int* handles);

    char* udaGetData(int handle);
    char* udaGetError(int handle);
    int udaGetDataNum(int handle);
    int udaGetDataType(int handle);
    int udaGetErrorType(int handle);
    void udaPutServerHost(const char* host);
    void udaPutServerPort(int port);
    int udaGetProperty(const char* property)
    void udaSetProperty(const char* property);
    void udaResetProperty(const char* property);
    const char* udaGetDataLabel(int handle);
    const char* udaGetDataUnits(int handle);
    const char* udaGetDataDesc(int handle);
    int udaGetRank(int handle);
    int udaGetDimNum(int handle, int ndim);
    int udaGetDimType(int handle, int ndim);
    int udaGetDimErrorType(int handle, int ndim);
    const char* udaGetDimLabel(int handle, int ndim);
    const char* udaGetDimUnits(int handle, int ndim);
    char* udaGetDimData(int handle, int ndim);
    char* udaGetDimError(int handle, int ndim);
    int udaSetDataTree(int handle);
    const char* udaGetErrorMsg(int handle);
    int udaGetErrorCode(int handle);
    int udaGetOrder(int handle);
    NTREE* udaGetDataTree(int handle);
    LOGMALLOCLIST* udaGetLogMallocList(int handle);

    PUTDATA_BLOCK* udaNewPutDataBlock(UdaType data_type, int count, int rank, int* shape, const char* data);
    void udaFreePutDataBlock(PUTDATA_BLOCK* putdata_block);
    int udaPutAPI(const char* putInstruction, PUTDATA_BLOCK* inPutData);

cdef extern from "uda/structured.h":
    ctypedef struct NTREE
    ctypedef struct LOGMALLOCLIST

    int udaGetNodeChildrenCount(NTREE* ntree);
    NTREE* udaGetNodeChild(NTREE* ntree, int child);
    char* udaGetNodeStructureName(NTREE* ntree);
    int udaGetNodeAtomicCount(NTREE* ntree);
    char** udaGetNodeAtomicNames(LOGMALLOCLIST* logmalloclist, NTREE* ntree);
    char** udaGetNodeStructureTypes(LOGMALLOCLIST* logmalloclist, NTREE* ntree);
    char** udaGetNodeAtomicTypes(LOGMALLOCLIST* logmalloclist, NTREE* ntree);
    int* udaGetNodeAtomicPointers(LOGMALLOCLIST* logmalloclist, NTREE* ntree);
    int* udaGetNodeAtomicRank(LOGMALLOCLIST* logmalloclist, NTREE* ntree);
    int** udaGetNodeAtomicShape(LOGMALLOCLIST* logmalloclist, NTREE* ntree);
    void* udaGetNodeStructureComponentData(LOGMALLOCLIST* logmalloclist, NTREE* ntree, const char* target);
