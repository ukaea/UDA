#include "west_static_data_utilities.h"

#include <string.h>
#include <stdio.h>
#include <assert.h>
#include <math.h>

#include <logging/logging.h>
#include <clientserver/initStructs.h>
#include <clientserver/errorLog.h>
#include <clientserver/stringUtils.h>
#include <clientserver/udaTypes.h>
#include <plugins/udaPlugin.h>
#include <structures/struct.h>

#include "west_utilities.h"
#include "ts_rqparam.h"


int setUDABlockFromArcadeScalarField(int data_type, char* prod_name, char* object_name, char* param_name, int index,
		int shotNumber, DATA_BLOCK* data_block, int* nodeIndices, float normalizationFactor)
{
	int nb_val;
	char* value = NULL;
	int val_nb = 1;
	int status = readStaticParameters(&value, &nb_val, shotNumber, prod_name, object_name, param_name, val_nb);
	setStaticValue(data_type, data_block, value, index, normalizationFactor);
	return status;
}

//Cast the results returned by tsmat according to the type of the data and set IDAM data
void setStaticValue(int data_type, DATA_BLOCK* data_block, char* value, int requestedIndex, float normalizationFactor)
{
	UDA_LOG(UDA_LOG_DEBUG, "Entering setStaticValue()\n");
	UDA_LOG(UDA_LOG_DEBUG, "requested index: %d", requestedIndex);
	if (data_type == UDA_TYPE_DOUBLE) {
		UDA_LOG(UDA_LOG_DEBUG, "handling double in setStaticValue()\n");
		double* pt_double = (double*)value;
		setReturnDataDoubleScalar(data_block, pt_double[requestedIndex] * (double)normalizationFactor, NULL);

	} else if (data_type == UDA_TYPE_FLOAT) {
		UDA_LOG(UDA_LOG_DEBUG, "handling float in setStaticValue(): %d, %g\n", requestedIndex, normalizationFactor);
		float* pt_float = (float*)value;
		UDA_LOG(UDA_LOG_DEBUG, "in setStaticValue(), requestedIndex:  %d\n", requestedIndex);
		UDA_LOG(UDA_LOG_DEBUG, "in setStaticValue(), normalizationFactor:  %f\n", normalizationFactor);
		UDA_LOG(UDA_LOG_DEBUG, "in setStaticValue(), pt_float[requestedIndex]:  %f\n", pt_float[requestedIndex]);
		UDA_LOG(UDA_LOG_DEBUG, "Floating value set to  %f\n", pt_float[requestedIndex] * normalizationFactor);
		setReturnDataFloatScalar(data_block, pt_float[requestedIndex] * normalizationFactor, NULL);

	} else if (data_type == UDA_TYPE_LONG) {
		UDA_LOG(UDA_LOG_DEBUG, "handling long in setStaticValue()\n");
		long* pt_long = (long*)value;
		setReturnDataLongScalar(data_block, pt_long[requestedIndex] * (long)normalizationFactor, NULL);

	} else if (data_type == UDA_TYPE_INT) {
		UDA_LOG(UDA_LOG_DEBUG, "handling int in setStaticValue()\n");
		int* pt_int = (int*)value;
		UDA_LOG(UDA_LOG_DEBUG, "handling in setStaticValue(): %d\n", *pt_int);
		setReturnDataIntScalar(data_block, pt_int[requestedIndex] * (int)normalizationFactor, NULL);

	} else if (data_type == UDA_TYPE_SHORT) {
		UDA_LOG(UDA_LOG_DEBUG, "handling short in setStaticValue()\n");
		int* pt_short = (int*)value;
		setReturnDataShortScalar(data_block, pt_short[requestedIndex] * (short)normalizationFactor, NULL);

	} else if (data_type == UDA_TYPE_STRING) {
		UDA_LOG(UDA_LOG_DEBUG, "handling string in setStaticValue()\n");
		setReturnDataString(data_block, strdup(value), NULL);

	} else {
		int err = 901;
		UDA_LOG(UDA_LOG_DEBUG, "Unsupported data type from setStaticValue()\n");
		UDA_LOG(UDA_LOG_ERROR, "%s", "WEST:ERROR: unsupported data type");
		addIdamError(CODEERRORTYPE, "WEST:ERROR: unsupported data type", err, "");
	}
}

//TODO, this patch will be removed soon
void set_BTANG_StaticValue(int data_type, DATA_BLOCK* data_block, char* value, int requestedIndex, float normalizationFactor)
{
	UDA_LOG(UDA_LOG_DEBUG, "Entering set_BTANG_StaticValue()\n");
	UDA_LOG(UDA_LOG_DEBUG, "requested index: %d", requestedIndex);
	if (data_type == UDA_TYPE_FLOAT) {
		UDA_LOG(UDA_LOG_DEBUG, "handling float in set_BTANG_StaticValue(): %d, %g\n", requestedIndex, normalizationFactor);
		float* pt_float = (float*)value;

		pt_float[requestedIndex] = pt_float[requestedIndex] + 180.;
		if (pt_float[requestedIndex] >= 360.)
			pt_float[requestedIndex] = pt_float[requestedIndex] - 360.;

		setReturnDataFloatScalar(data_block, pt_float[requestedIndex] * normalizationFactor, NULL);

	} else {
		int err = 901;
		UDA_LOG(UDA_LOG_DEBUG, "Unsupported data type from set_BTANG_StaticValue()\n");
		UDA_LOG(UDA_LOG_ERROR, "%s", "WEST:ERROR: unsupported data type");
		addIdamError(CODEERRORTYPE, "WEST:ERROR: unsupported data type", err, "");
	}
}

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

