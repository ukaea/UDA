/* V3 output in a cell array

Change History

20May2010   dgm Changed to IDAM accessors getIdamDoubleData, getIdamDoubleDimData to cast atomic types to double.
------------------------------------------------------------------------------------------------------------------------
*/

#include "stdio.h"
#include "string.h"
#include "idammatlab.h"
#include "mex.h"

#define NFIELDS 100
#define OUT plhs[0]

int listIndex;

void addItem(mxArray * list, const mxArray * item)
{
    if (listIndex >= NFIELDS) {
        mexErrMsgTxt("Too many items");
    } else {
        mxSetCell(list, listIndex, item);
        listIndex++;
    }
}

void mexFunction(int nlhs, mxArray * plhs[], int nrhs, const mxArray * prhs[])
{
    /* Matlab related data
     */
    mxArray * item;
    double * ptr;
    char * name, *source;
    int ndata, erc;

    /* IDAM related data
     */
    int handle, rank, order;
    int * ivec = NULL;
    float * fvec = NULL;
    double * dvec = NULL;

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
        setIdamProperty("verbose");
        setIdamProperty("debug");
    */

    name = mxArrayToString(prhs[0]);
    source = mxArrayToString(prhs[1]);

    addItem(OUT, mxCreateString("Name"));
    addItem(OUT, mxCreateString(name));
    addItem(OUT, mxCreateString("Source"));
    addItem(OUT, mxCreateString(source));

    handle = idamGetAPI(name, source);

    mxFree(name);
    mxFree(source);

    erc = getIdamErrorCode(handle);
    addItem(OUT, mxCreateString("ErrorCode"));
    addItem(OUT, mxCreateDoubleScalar(erc));
    addItem(OUT, mxCreateString("ErrorMessage"));
    addItem(OUT, mxCreateString(getIdamErrorMsg(handle)));

    /* check status and exit now if appropriate
    */
    if (erc != 0) {
        return;
    }

    /* get and store the data and associated information
    */
    ndata = getIdamDataNum(handle);
    addItem(OUT, mxCreateString("Data"));
    addItem(OUT, mxCreateString(getIdamDataLabel(handle)));
    addItem(OUT, mxCreateString(getIdamDataUnits(handle)));

    item = mxCreateDoubleMatrix(1, ndata, mxREAL);
    ptr = mxGetPr(item);

    // Cast atomic data types to double

    getIdamDoubleData(handle, ptr);

    addItem(OUT, item);


    /* do the same for the dimensions
    */
    order = getIdamOrder(handle);
    addItem(OUT, mxCreateString("Order"));
    addItem(OUT, mxCreateDoubleScalar(order));
    rank = getIdamRank(handle);
    addItem(OUT, mxCreateString("Rank"));
    addItem(OUT, mxCreateDoubleScalar(rank));

    if (rank > 0) {
        for (int i = 0; i < rank; i++) {
            ndata = getIdamDimNum(handle, i);
            addItem(OUT, mxCreateString("Dimension"));
            addItem(OUT, mxCreateString(getIdamDimLabel(handle, i)));
            addItem(OUT, mxCreateString(getIdamDimUnits(handle, i)));

            item = mxCreateDoubleMatrix(1, ndata, mxREAL);
            ptr = mxGetPr(item);

            // Cast atomic data types to double

            getIdamDoubleDimData(handle, i, ptr);

            addItem(OUT, item);
        }
    }

    udaFree(handle);           /* Free Heap for this signal */
}
