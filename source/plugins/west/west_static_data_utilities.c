#include "west_static_data_utilities.h"

#include <string.h>
#include <stdio.h>
#include <assert.h>

#include <logging/logging.h>
#include <clientserver/initStructs.h>
#include <clientserver/errorLog.h>
#include <clientserver/stringUtils.h>
#include <clientserver/udaTypes.h>

#include "west_utilities.h"

void SetStatic1DData(DATA_BLOCK* data_block, int len, float* data)
{

    //IDAM data block initialization
    initDataBlock(data_block);
    data_block->rank = 1; //we return always a 1D array
    data_block->dims = (DIMS*)malloc(data_block->rank * sizeof(DIMS));
    int i;
    for (i = 0; i < data_block->rank; i++) {
        initDimBlock(&data_block->dims[i]);
    }

    free(data_block->dims);

    data_block->rank = 1;
    data_block->data_type = UDA_TYPE_FLOAT;
    data_block->data_n = len;

    data_block->data = (char*)data;

    data_block->dims =
            (DIMS*)malloc(data_block->rank * sizeof(DIMS));

    for (i = 0; i < data_block->rank; i++) {
        initDimBlock(&data_block->dims[i]);
    }

    data_block->dims[0].data_type = UDA_TYPE_FLOAT;
    data_block->dims[0].dim_n = len;
    data_block->dims[0].compressed = 0;
    data_block->dims[0].dim = (char*)data;

    strcpy(data_block->data_label, "");
    strcpy(data_block->data_units, "");
    strcpy(data_block->data_desc, "");
}

void SetStatic1DINTData(DATA_BLOCK* data_block, int len, int* data)
{

    //IDAM data block initialization
    initDataBlock(data_block);
    data_block->rank = 1; //we return always a 1D array
    data_block->dims = (DIMS*)malloc(data_block->rank * sizeof(DIMS));
    int i;
    for (i = 0; i < data_block->rank; i++) {
        initDimBlock(&data_block->dims[i]);
    }

    free(data_block->dims);

    data_block->rank = 1;
    data_block->data_type = UDA_TYPE_INT;
    data_block->data_n = len;

    data_block->data = (char*)data;

    data_block->dims =
            (DIMS*)malloc(data_block->rank * sizeof(DIMS));

    for (i = 0; i < data_block->rank; i++) {
        initDimBlock(&data_block->dims[i]);
    }

    data_block->dims[0].data_type = UDA_TYPE_INT;
    data_block->dims[0].dim_n = len;
    data_block->dims[0].compressed = 0;
    data_block->dims[0].dim = (char*)data;

    strcpy(data_block->data_label, "");
    strcpy(data_block->data_units, "");
    strcpy(data_block->data_desc, "");
}

