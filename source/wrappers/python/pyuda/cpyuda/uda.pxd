cdef extern from "client/udaClient.h":
    const char* getIdamServerHost();
    int getIdamServerPort();
    const char* getUdaBuildVersion();
    const char* getUdaBuildDate();
    void idamFree(int handle);

cdef extern from "client/udaGetAPI.h":
    int idamGetAPI(const char* data_object, const char* data_source);
    int idamGetBatchAPI(const char** signals, const char** sources, int count, int* handles);

cdef extern from "structures/genStructs.h":
    ctypedef struct NTREE:                  # N-ary Tree linking all related user defined data structures: definitions and data
        int branches;                       # Children (Branch) Count from this node
        char name[1024];                    # The Structure's and also the tree node's name
        void* data;                         # the Data
        NTREE* parent;                      # Pointer to the parent node
        NTREE** children;                   # Pointer Array of sibling tree nodes (branches).

    ctypedef struct LOGMALLOCLIST

cdef extern from "clientserver/udaStructs.h":
    ctypedef struct CLIENT_BLOCK:
        int altRank;                    # Specify the rank of the alternative signal/source to be used
        int get_nodimdata;              # Don't send Dimensional Data: Send an index only.
        int get_timedble;               # Return Time Dimension Data in Double Precision if originally compressed
        int get_dimdble;                # Return all Dimensional Data in Double Precision
        int get_datadble;               # Return Data in Double Precision
        int get_bad;                    # Return Only Data with Bad Status value
        int get_meta;                   # Return Meta Data associated with Signal
        int get_asis;                   # Return data as Stored in data Archive
        int get_uncal;                  # Disable Calibration Correction
        int get_notoff;                 # Disable Timing Offset Correction
        int get_scalar;                 # Reduce rank from 1 to 0 (Scalar) if dimensional data are all zero
        int get_bytes;                  # Return Data as Bytes or Integers without applying the signal's ADC Calibration Data

    ctypedef struct DATA_SOURCE:
        int source_id;
        int config_id;
        int reason_id;
        int run_id;
        int meta_id;
        int status_desc_id;
        int exp_number;
        int pass_ "pass";
        int status;
        int status_reason_code;
        int status_impact_code;
        char access;
        char reprocess;
        char type;
        char source_alias[1024];
        char pass_date[12];
        char archive[1024];
        char device_name[1024];
        char format[1024];
        char path[1024];
        char filename[1024];
        char server[1024];
        char userid[1024];
        char reason_desc[1024];
        char run_desc[1024];
        char status_desc[1024];
        char creation[12];
        char modified[12];
        char xml[1024];
        char xml_creation[12];

    ctypedef struct SIGNAL_DESC:
        int signal_desc_id;
        int meta_id;
        int rank;
        int range_start;
        int range_stop;
        char type;
        char source_alias[1024];
        char signal_alias[1024];
        char signal_name[1024];
        char generic_name[1024];
        char description[1024];
        char signal_class[1024];
        char signal_owner[1024];
        char creation[12];
        char modified[12];
        char xml[10240];
        char xml_creation[12];
        int signal_alias_type;
        int signal_map_id;

    ctypedef struct PUTDATA_BLOCK:
        int data_type;
        unsigned int rank;
        unsigned int count;
        int* shape;
        const char* data;

    ctypedef struct PUTDATA_BLOCK_LIST:
        unsigned int blockCount;        # Number of data blocks
        unsigned int blockListSize;     # Number of data blocks allocated
        PUTDATA_BLOCK* putDataBlock;    # Array of data blocks

cdef extern from "client/accAPI.h":
    char* getIdamData(int handle);
    char* getIdamError(int handle);
    int getIdamDataNum(int handle);
    int getIdamDataType(int handle);
    int getIdamErrorType(int handle);
    void putIdamServerHost(const char* host);
    void putIdamServerPort(int port);
    int getIdamProperty(const char* prop);
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
    DATA_SOURCE* getIdamDataSource(int handle);
    SIGNAL_DESC* getIdamSignalDesc(int handle);
    int setIdamDataTree(int handle);
    CLIENT_BLOCK* getIdamProperties(int handle);
    const char* getIdamErrorMsg(int handle);
    int getIdamErrorCode(int handle);
    int getIdamOrder(int handle);
    NTREE* getIdamDataTree(int handle);
    LOGMALLOCLIST* getIdamLogMallocList(int handle);

cdef extern from "clientserver/initStructs.h":
    void initIdamPutDataBlock(PUTDATA_BLOCK* str);
    void initPutDataBlockList(PUTDATA_BLOCK_LIST* putDataBlockList);

cdef extern from "client/udaPutAPI.h":
    int idamPutAPI(const char* putInstruction, PUTDATA_BLOCK* inPutData);

cdef extern from "structures/struct.h":
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


# requires cython v3.0+, we're currently using previous version (maybe also has to be in a .pyx file rather than .pyd?)
#cdef extern from "client/closedown.h":
#    cpdef enum class ClosedownType(int):
#        CLOSE_SOCKETS = 0,
#        CLOSE_ALL = 1,


#cdef extern from "client/getEnvironment.h":
#    ENVIRONMENT* getIdamClientEnvironment();


cdef extern from "client/connection.h":
    int resetClientConnection();
    void closeClientConnection();
