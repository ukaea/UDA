#include "stdio.h"
#include "string.h"
#include "idammatlab.h"
#include "mex.h"

#define NFIELDS 100
#define OUT plhs[0]

int listIndex;

void addItem(mxArray* list, const mxArray* item)
{
    if (listIndex >= NFIELDS) {
        mexErrMsgTxt("Too many items");
    } else {
        mxSetCell(list, listIndex, (mxArray*)item);
        listIndex++;
    }
}

void mexFunction(int nlhs, mxArray* plhs[], int nrhs, const mxArray* prhs[])
{
    /* Matlab related data
     */
    mxArray* item;
    double* ptr;
    char* name, * source;
    int ndata, erc;

    /* IDAM related data
     */
    int handle, rank, order;
    int* ivec = NULL;
    float* fvec = NULL;
    double* dvec = NULL;

    if ((nlhs != 1) || (nrhs != 2)) {
        mexErrMsgTxt("One output and two inputs needed");
        return;
    }

    if (!(mxIsChar(prhs[0]))) {
        mexErrMsgTxt("First input must be of type string.\n.");
    }

    if (!(mxIsChar(prhs[1]))) {
        mexErrMsgTxt("Second input must be of type string.\n.");
    }

    /* Allocate memory for the cell array */
    OUT = mxCreateCellMatrix(1, NFIELDS);
    listIndex = 0;

    /* First get the IDAM data
        udaSetProperty("verbose");
        udaSetProperty("debug");
    */

    name = mxArrayToString(prhs[0]);
    source = mxArrayToString(prhs[1]);

    addItem(OUT, mxCreateString("Name"));
    addItem(OUT, mxCreateString(name));
    addItem(OUT, mxCreateString("Source"));
    addItem(OUT, mxCreateString(source));

    handle = udaGetAPI(name, source);

    mxFree(name);
    mxFree(source);

    erc = udaGetErrorCode(handle);
    addItem(OUT, mxCreateString("ErrorCode"));
    addItem(OUT, mxCreateDoubleScalar(erc));
    addItem(OUT, mxCreateString("ErrorMessage"));
    addItem(OUT, mxCreateString(udaGetErrorMsg(handle)));

    /* check status and exit now if appropriate
    */
    if (erc != 0) {
        return;
    }

    /* get and store the data and associated information
    */
    ndata = udaGetDataNum(handle);
    addItem(OUT, mxCreateString("Data"));
    addItem(OUT, mxCreateString(udaGetDataLabel(handle)));
    addItem(OUT, mxCreateString(udaGetDataUnits(handle)));

    item = mxCreateDoubleMatrix(1, ndata, mxREAL);
    ptr = mxGetPr(item);

    // Cast atomic data types to double
    udaGetDoubleData(handle, ptr);
    addItem(OUT, item);

    /* Error Data
    */
    char* error = udaGetError(handle);

    if (error != NULL) {
        addItem(OUT, mxCreateString("Error"));
        addItem(OUT, mxCreateString("Error"));
        addItem(OUT, mxCreateString(udaGetDataUnits(handle)));

        item = mxCreateDoubleMatrix(1, ndata, mxREAL);
        ptr = mxGetPr(item);

        // Cast atomic data types to double
        // Fudge as required accessor not in library - small loss of precision!
        float* fp = (float*)malloc(ndata * sizeof(float));
        udaGetFloatError(handle, fp);

        for (int i = 0; i < ndata; i++) {
            ptr[i] = (double)fp[i];
        }

        addItem(OUT, item);
        free(fp);
    }

    /* do the same for the dimensions
    */
    order = udaGetOrder(handle);
    addItem(OUT, mxCreateString("Order"));
    addItem(OUT, mxCreateDoubleScalar(order));
    rank = udaGetRank(handle);
    addItem(OUT, mxCreateString("Rank"));
    addItem(OUT, mxCreateDoubleScalar(rank));

    if (rank > 0) {
        for (int i = 0; i < rank; i++) {
            ndata = udaGetDimNum(handle, i);
            addItem(OUT, mxCreateString("Dimension"));
            addItem(OUT, mxCreateString(udaGetDimLabel(handle, i)));
            addItem(OUT, mxCreateString(udaGetDimUnits(handle, i)));

            item = mxCreateDoubleMatrix(1, ndata, mxREAL);
            ptr = mxGetPr(item);

            // Cast atomic data types to double
            udaGetDoubleDimData(handle, i, ptr);
            addItem(OUT, item);
        }
    }

    udaFree(handle);           /* Free Heap for this signal */
}
