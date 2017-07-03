/*---------------------------------------------------------------
* IDAM Plugin data Reader to Return Test Data
*
* Input Arguments:	DATA_SOURCE data_source
*			SIGNAL_DESC signal_desc
*
* Returns:		readNothing	0 (always OK)
*			DATA_BLOCK	Structure with Test Data
*
* Calls
*
* Notes:
* ToDo:
*
*-----------------------------------------------------------------------------*/
#include "readNothing.h"

#include <string.h>
#include <stdlib.h>

#include <clientserver/udaStructs.h>
#include <clientserver/errorLog.h>
#include <clientserver/udaTypes.h>
#include <logging/logging.h>
#include <clientserver/freeDataBlock.h>
#include <clientserver/initStructs.h>
#include <clientserver/udaErrors.h>

//---------------------------------------------------------------------------------------------------------------
// Stub plugin if disabled

#ifdef NONOTHINGPLUGIN

int readNothing(DATA_SOURCE data_source,
                SIGNAL_DESC signal_desc,
                DATA_BLOCK *data_block) {
    int err = 999;
    addIdamError(&idamerrorstack, CODEERRORTYPE, "readCDF", err, "PLUGIN NOT ENABLED");
    return err;
}

#else

//---------------------------------------------------------------------------------------------------------------

int readNothing(DATA_SOURCE data_source,
                SIGNAL_DESC signal_desc,
                DATA_BLOCK* data_block)
{

    int i, err = 0;
    float* fp = NULL, * fdp = NULL;

//----------------------------------------------------------------------
// Allocate Heap

    if (data_source.exp_number == 0) return 0;    // return Nothing!

    data_block->data_n = data_source.exp_number;

    if ((fp = (float*)malloc(data_block->data_n * sizeof(float))) == NULL) {
        err = HEAPERROR;
        addIdamError(&idamerrorstack, CODEERRORTYPE, "readNothing", err, "Unable to Allocate Heap Memory for the Test");
        return err;
    }

    IDAM_LOGF(UDA_LOG_DEBUG, "Generating %d test Data\n", data_block->data_n);

    data_block->rank = 1;
    data_block->order = 0;
    data_block->data_type = TYPE_FLOAT;
    strcpy(data_block->data_label, "Data Label");
    strcpy(data_block->data_units, "Data Units");
    strcpy(data_block->data_desc, "Data Description");

    if ((data_block->dims = (DIMS*)malloc(data_block->rank * sizeof(DIMS))) == NULL) {
        err = HEAPERROR;
        addIdamError(&idamerrorstack, CODEERRORTYPE, "readNothing", err,
                     "Problem Allocating Dimension Heap Memory #1 for Test");
        freeDataBlock(data_block);
        return err;
    }

    if ((fdp = (float*)malloc(data_block->data_n * sizeof(float))) == NULL) {
        err = HEAPERROR;
        addIdamError(&idamerrorstack, CODEERRORTYPE, "readNothing", err,
                     "Problem Allocating Dimension Heap Memory #2 for Test");
        freeDataBlock(data_block);
        return err;
    }

    initDimBlock(&data_block->dims[0]);

    data_block->dims[0].dim_n = data_block->data_n;
    data_block->dims[0].data_type = TYPE_FLOAT;
    data_block->dims[0].compressed = 0;

    strcpy(data_block->dims[0].dim_label, "Dim Label");
    strcpy(data_block->dims[0].dim_units, "Dim Units");

    for (i = 0; i < data_block->data_n; i++) {
        fp[i] = (float)i;
        fdp[i] = (float)i;    // Should be Compresssible!
    }

    if (data_source.pass != -1 && data_block->data_n >= 2) fdp[1] = fdp[0];    // Not Compressible

    data_block->data = (char*)fp;
    data_block->dims[0].dim = (char*)fdp;

    IDAM_LOG(UDA_LOG_DEBUG, "Data Generated\n");

    return 0;
}

#endif
