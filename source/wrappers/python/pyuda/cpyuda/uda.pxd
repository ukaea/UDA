from libcpp cimport bool as cbool

cdef extern from "uda/client.h":
    ctypedef struct NTREE
    ctypedef struct LOGMALLOCLIST
    ctypedef struct PUTDATA_BLOCK
    ctypedef enum UdaType:
        pass

    const char* getIdamServerHost();
    int getIdamServerPort();
    const char* udaGetBuildVersion();
    const char* udaGetBuildDate();
    void udaFree(int handle);
    void closeAllConnections();

    int idamGetAPI(const char* data_object, const char* data_source);
    int idamGetBatchAPI(const char** signals, const char** sources, int count, int* handles);

    char* getIdamData(int handle);
    char* getIdamError(int handle);
    int getIdamDataNum(int handle);
    int getIdamDataType(int handle);
    int getIdamErrorType(int handle);
    void putIdamServerHost(const char* host);
    void putIdamServerPort(int port);
    int getIdamProperty(const char* property)
    void setIdamProperty(const char* property);
    void resetIdamProperty(const char* property);
    const char* getIdamDataLabel(int handle);
    const char* getIdamDataUnits(int handle);
    const char* getIdamDataDesc(int handle);
    int getIdamRank(int handle);
    int getIdamDimNum(int handle, int ndim);
    int getIdamDimType(int handle, int ndim);
    int getIdamDimErrorType(int handle, int ndim);
    const char* getIdamDimLabel(int handle, int ndim);
    const char* getIdamDimUnits(int handle, int ndim);
    char* getIdamDimData(int handle, int ndim);
    char* getIdamDimError(int handle, int ndim);
    int setIdamDataTree(int handle);
    const char* getIdamErrorMsg(int handle);
    int getIdamErrorCode(int handle);
    int getIdamOrder(int handle);
    NTREE* getIdamDataTree(int handle);
    LOGMALLOCLIST* getIdamLogMallocList(int handle);

    PUTDATA_BLOCK* udaNewPutDataBlock(UdaType data_type, int count, int rank, int* shape, const char* data);
    void udaFreePutDataBlock(PUTDATA_BLOCK* putdata_block);
    int idamPutAPI(const char* putInstruction, PUTDATA_BLOCK* inPutData);

cdef extern from "uda/structured.h":
    ctypedef struct NTREE
    ctypedef struct LOGMALLOCLIST

    int getNodeChildrenCount(NTREE* ntree);
    NTREE* getNodeChild(NTREE* ntree, int child);
    char* getNodeStructureName(NTREE* ntree);
    int getNodeAtomicCount(NTREE* ntree);
    char** getNodeAtomicNames(LOGMALLOCLIST* logmalloclist, NTREE* ntree);
    char** getNodeStructureTypes(LOGMALLOCLIST* logmalloclist, NTREE* ntree);
    char** getNodeAtomicTypes(LOGMALLOCLIST* logmalloclist, NTREE* ntree);
    int* getNodeAtomicPointers(LOGMALLOCLIST* logmalloclist, NTREE* ntree);
    int* getNodeAtomicRank(LOGMALLOCLIST* logmalloclist, NTREE* ntree);
    int** getNodeAtomicShape(LOGMALLOCLIST* logmalloclist, NTREE* ntree);
    void* getNodeStructureComponentData(LOGMALLOCLIST* logmalloclist, NTREE* ntree, const char* target);
