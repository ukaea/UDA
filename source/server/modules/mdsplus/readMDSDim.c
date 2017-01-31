/*---------------------------------------------------------------------------
/ Read MDS Data Dimensions
/
/ Input Arguments:	char *node	name of the MDS+ Signal with path
/			int   ndim	signal dimension (0 usualy time)
/
/ Returns:		readMDSDim	0 if read was successful
/					otherwise a Error Code is returned
/			DIMS *ddim	Data Dimension Structure containing the
/					dimensional data.
/
/ Calls			MDS+ Library routines
/---------------------------------------------------------------------------*/
#include "readMDSDim.h"

#include <mdslib.h>

#include <clientserver/idamErrors.h>
#include <clientserver/idamErrorLog.h>
#include <clientserver/idamTypes.h>
#include <clientserver/TrimString.h>
#include <include/idamclientserverprivate.h>
#include <logging/idamLog.h>

#define status_ok(status) (((status) & 1) == 1)

int readMDSType(int type);

int readMDSDim(char *node, int ndim, DIMS *ddim) {

    int dtype_float  = DTYPE_FLOAT;
    int dtype_double = DTYPE_DOUBLE;

    int dtype_char   = DTYPE_CHAR;
    int dtype_uchar  = DTYPE_UCHAR;
    int dtype_short  = DTYPE_SHORT;
    int dtype_ushort = DTYPE_USHORT;
    int dtype_int    = DTYPE_LONG;
    int dtype_uint   = DTYPE_ULONG;
    int dtype_long   = DTYPE_LONGLONG;
    int dtype_ulong  = DTYPE_ULONGLONG;

    int dtype_str    = DTYPE_CSTRING;
    int desc ;
    //int lenstr = STRING_LENGTH;
    int null   = 0;

    char  *sdim = NULL;
    char  *units= NULL;
    void  *data = NULL;

    int status, err, size, lunits, type;

    idamLog(LOG_DEBUG,"readMDSDim: Node      =  %s \n", node);
    idamLog(LOG_DEBUG,"readMDSDim: Dimension =  %d \n", ndim);

// Check Constraint on Maximum Number of Dimensions

    if (ndim > 99) {
        err = MDS_ERROR_DIM_DIMENSION_NUMBER_EXCEEDED ;
        addIdamError(&idamerrorstack, CODEERRORTYPE, "readMDSDim", err, "Maximum Number of Node Dimensions Exceeded");
        return(err);
    }

//----------------------------------------------------------------------
// Error Management Loop

    err = 0;
    do {

//----------------------------------------------------------------------
// Length of Dimension Data

        null = 0;
        sdim=(char *)malloc((18+strlen(node))*sizeof(char));

        if ( !sdim ) {
            err = MDS_ERROR_DIM_ALLOCATING_HEAP_TDI_SIZE ;
            addIdamError(&idamerrorstack, CODEERRORTYPE, "readMDSDim", err, "Unable to Allocate Heap for MdsValue Dim Size Enquiry");
            break;
        }

        sprintf(sdim,"SIZE(DIM_OF(%s,%d))",node,ndim);

        idamLog(LOG_DEBUG,"readMDSDim: TDI =  %s \n", sdim);

        desc = descr(&dtype_int, &size, &null);
        status = MdsValue(sdim, &desc, &null, 0);

        if ( !status_ok(status) || size < 1) {
            err = MDS_ERROR_DIM_MDSVALUE_SIZE;
            addIdamError(&idamerrorstack, CODEERRORTYPE, "readMDSDim", err, "Unable to Retrieve the Dim Size");
            break;
        }

        idamLog(LOG_DEBUG,"readMDSDim: Length of Dimension (%d) = %d\n", ndim, size);

//----------------------------------------------------------------------
// Data Type

        null = 0;
        sdim = (char *) realloc((void *)sdim, (size_t)(24+strlen(node))*sizeof(char) );

        sprintf(sdim,"KIND(DATA(DIM_OF(%s,%d)))",node,ndim);

        desc   = descr(&dtype_int, &type, &null);
        status = MdsValue(sdim, &desc, &null, 0);

        if ( !status_ok(status) || size < 1) {
            err = MDS_ERROR_MDSVALUE_TYPE ;
            addIdamError(&idamerrorstack, CODEERRORTYPE, "readMDSDim", err, "Unable to Retrieve the Dim Type");
            break;
        }

//----------------------------------------------------------------------
// Identify IDAM type

        if ((ddim->data_type = readMDSType(type)) == TYPE_UNKNOWN) {
            err = 999;
            addIdamError(&idamerrorstack, CODEERRORTYPE, "readMDSDim", err, "Unknown Data Type");
            break;
        }

//----------------------------------------------------------------------
// Allocate Heap for Data, and define descriptor

        null = 0;

        switch(ddim->data_type) {
        case(TYPE_FLOAT): {
            data = malloc(size * sizeof(float));				// allocate memory for the signal
            desc = descr(&dtype_float, (float *)data, &size, &null);	// descriptor for this signal
            break;
        }
        case(TYPE_DOUBLE): {
            data = malloc(size * sizeof(double));
            desc = descr(&dtype_double, (double *)data, &size, &null);
            break;
        }
        case(TYPE_UNSIGNED_CHAR): {
            data = malloc(size * sizeof(unsigned char));
            desc = descr(&dtype_uchar, (unsigned char *)data, &size, &null);
            break;
        }
        case(TYPE_CHAR): {
            data = malloc(size * sizeof(char));
            desc = descr(&dtype_char, (char *)data, &size, &null);
            break;
        }
        case(TYPE_UNSIGNED_SHORT): {
            data = malloc(size * sizeof(unsigned short));
            desc = descr(&dtype_ushort, (unsigned short *)data, &size, &null);
            break;
        }
        case(TYPE_SHORT): {
            data = malloc(size * sizeof(short));
            desc = descr(&dtype_short, (short *)data, &size, &null);
            break;
        }
        case(TYPE_UNSIGNED): {
            data = malloc(size * sizeof(unsigned int));
            desc = descr(&dtype_uint, (unsigned int *)data, &size, &null);
            break;
        }
        case(TYPE_INT): {
            data = malloc(size * sizeof(int));
            desc = descr(&dtype_int, (int *)data, &size, &null);
            break;
        }
        case(TYPE_UNSIGNED_LONG): {
            data = malloc(size * sizeof(unsigned long));
            desc = descr(&dtype_ulong, (unsigned long *)data, &size, &null);
            break;
        }
        case(TYPE_LONG): {
            data = malloc(size * sizeof(long));
            desc = descr(&dtype_long, (long *)data, &size, &null);
            break;
        }
        }

        if (data == NULL) {
            err = MDS_ERROR_DIM_ALLOCATING_HEAP_DATA_BLOCK ;
            addIdamError(&idamerrorstack, CODEERRORTYPE, "readMDSDim", err, "Unable to Allocate Heap for the Dimensional Data");
            break;
        }


//----------------------------------------------------------------------
// Dimension Vector data

        sdim = (char *) realloc((void *)sdim, (size_t)(12+strlen(node))*sizeof(char) );

        if (sdim == NULL) {
            err = MDS_ERROR_DIM_ALLOCATING_HEAP_TDI_DIM_OF;
            addIdamError(&idamerrorstack, CODEERRORTYPE, "readMDSDim", err, "Unable to Allocate Heap for the Dim Data");
            break;
        }

        sprintf(sdim,"DIM_OF(%s,%d)",node, ndim);

        idamLog(LOG_DEBUG,"readMDSDim: TDI = %s \n", sdim);

        status = MdsValue(sdim, &desc, &null, 0);

        if ( !status_ok(status) ) {
            err = MDS_ERROR_DIM_MDSVALUE_DATA;
            addIdamError(&idamerrorstack, CODEERRORTYPE, "readMDSDim", err, "Unable to Retrieve the Dim Data");
            break;
        }

        {
            int i;
            float tot = 0.0;
            float *fp = (float *)data;
            for(i=0; i<size; i++) tot=tot+fp[i];
            idamLog(LOG_DEBUG," Dim Data Sum = %f\n",tot);
        }

//----------------------------------------------------------------------
// length of Data Units String

        null = 0;
        sdim = (char *) realloc((void *)sdim, (size_t)(27+strlen(node))*sizeof(char) );

        if ( !sdim ) {
            err = MDS_ERROR_DIM_ALLOCATING_HEAP_TDI_LEN_UNITS;
            addIdamError(&idamerrorstack, CODEERRORTYPE, "readMDSDim", err, "Unable to Allocate Heap for MdsValue Dim Units Length Enquiry");
            break;
        }

        sprintf(sdim,"LEN(UNITS_OF(DIM_OF(%s,%d)))",node, ndim);

        desc = descr(&dtype_int, &lunits, &null);
        status = MdsValue(sdim, &desc, &null, 0);

        if ( !status_ok(status) || lunits < 1) {
            //err = MDS_ERROR_DIM_MDSVALUE_LEN_UNITS;
            //addIdamError(&idamerrorstack, CODEERRORTYPE, "readMDSDim", err, "Unable to Retrieve the Dim Units Length");
            //break;
            lunits = 0;
        }

        lunits++;	// 1 Byte For Null Terminator

        idamLog(LOG_DEBUG,"readMDSDim: Length of Units String %d\n", lunits);

//----------------------------------------------------------------------
// Dimension Units

        if(lunits > 1) {
            null = 0;
            sdim = (char *) realloc((void *)sdim, (size_t)(22+strlen(node))*sizeof(char) );

            if ( !sdim ) {
                err = MDS_ERROR_DIM_ALLOCATING_HEAP_TDI_UNITS;
                addIdamError(&idamerrorstack, CODEERRORTYPE, "readMDSDim", err, "Unable to Allocate Heap for MdsValue Dim Units Enquiry");
                break;
            }

            sprintf(sdim,"UNITS_OF(DIM_OF(%s,%d))",node, ndim);

            idamLog(LOG_DEBUG,"ReadMDSDim: TDI =  %s \n", sdim);

            units = (char *)malloc(lunits*sizeof(char));

            if ( !units ) {
                err = MDS_ERROR_DIM_ALLOCATING_HEAP_DIM_UNITS;
                addIdamError(&idamerrorstack, CODEERRORTYPE, "readMDSDim", err, "Unable to Allocate Heap for the Dim Units");
                break;
            }

            desc = descr(&dtype_str, units, &null, &lunits);
            status = MdsValue(sdim, &desc, &null, 0);

            if ( !status_ok(status) ) {
                err = MDS_ERROR_DIM_MDSVALUE_UNITS;
                addIdamError(&idamerrorstack, CODEERRORTYPE, "readMDSDim", err, "Unable to Retrieve the Dim Units");
                break;
            }

            units[lunits-1] = '\0';
            TrimString(units);
        } else {
            units = (char *)malloc(sizeof(char));
            units[0] = '\0';
        }

        idamLog(LOG_DEBUG,"ReadMDSDim: Dimension Units =[%s] %d\n", units,
                                strlen(units));

    } while(0);	// Always exit the Error Management Loop

//----------------------------------------------------------------------
// Error Status

    idamLog(LOG_DEBUG,"readMDSDim: Final Error Status = %d\n", err);

//----------------------------------------------------------------------
// Free Local Heap Memory

    if(err != 0) {
        if(data  != NULL) free( (void *)data );
        if(units != NULL) free( (void *)units );
    } else {
        ddim->dim_n     = size;
        ddim->dim       = (char *) data;
        strcpy(ddim->dim_units,units);
        LeftTrimString(ddim->dim_units);
        if(units != NULL) free( (void *)units );
    }

    free( (void *)sdim );

    return(err);
}

