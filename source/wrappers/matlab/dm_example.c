/* #include "idamclientserver.h"
#include "idamclient.h"
*/
#include "stdio.h"
#include "idammatlab.h"

int main()
{

    int handle, nmax, ndata, rank, i, irank;
    float * fvec = NULL;
    double * dvec = NULL;

    int type;

    //setIdamProperty("verbose");   // Print Errors
    //setIdamProperty("debug"); // Print Debug Trail
    //putIdamServerHost("fuslwn");  // Identify the Server?s Host
    //putIdamServerPort(56565); // Identify the Server?s Port

    handle = idamGetAPI("ip", "13500"); // Execute the API

    fprintf(stdout, "Test: Handle %d, Error Code %d [%s]\n", handle, getIdamErrorCode(handle),
            getIdamErrorMsg(handle));

    if (handle >= 0 && getIdamErrorCode(handle) == 0) { // Test Data Access was OK
        fprintf(stdout, "Returned Source Status:     %d\n", getIdamSourceStatus(handle));
        fprintf(stdout, "Returned Signal Status:     %d\n", getIdamSignalStatus(handle));
        fprintf(stdout, "Returned Data Status  :     %d\n", getIdamDataStatus(handle));
        fprintf(stdout, "No. Data Elements     :     %d\n", getIdamDataNum(handle));
        fprintf(stdout, "Rank                  :     %d\n", getIdamRank(handle));
        fprintf(stdout, "Order of Time Vector  :     %d\n", getIdamOrder(handle));
        fprintf(stdout, "Data Description      :     %s\n", getIdamDataDesc(handle));
        fprintf(stdout, "Data Units            :     %s\n", getIdamDataUnits(handle));
        fprintf(stdout, "Data Label            :     %s\n", getIdamDataLabel(handle));
        fprintf(stdout, "Data Type             :     %d\n", getIdamDataType(handle));
        fprintf(stdout, "Error Type            :     %d\n", getIdamErrorType(handle));
        fprintf(stdout, "Error Asymmetry?      :     %d\n", getIdamErrorAsymmetry(handle));

        nmax = 10;
        ndata = getIdamDataNum(handle);

        if (ndata < 10) {
            nmax = ndata;
        }

        type = getIdamDataType(handle);

        switch (type) {
        case (UDA_TYPE_FLOAT):
            fvec = (float *) getIdamData(handle);       // pointer to the Data

            for (int i = 0; i < nmax; i++) {
                fprintf(stdout, "%d %f\n", i, fvec[i]);    // print some data
            }

            break;

        case (UDA_TYPE_DOUBLE):
            dvec = (double *) getIdamData(handle);

            for (int i = 0; i < nmax; i++) {
                fprintf(stdout, "%d %f\n", i, dvec[i]);
            }

            break;
        }

        rank = getIdamRank(handle);

        if (rank > 0) {         // Are there dimensions?
            for (irank = 0; irank < rank; irank++) {
                fprintf(stdout, "Dimension Id                  :     %d\n", irank);

                if (irank == getIdamOrder(handle)) {
                    fprintf(stdout, "This is the Time Dimension\n");
                }

                fprintf(stdout, "No. Dimension Elements        :     %d\n",
                        getIdamDimNum(handle, irank));
                fprintf(stdout, "Dimension units               :     %s\n",
                        getIdamDimUnits(handle, irank));
                fprintf(stdout, "Dimension Label               :     %s\n",
                        getIdamDimLabel(handle, irank));
                fprintf(stdout, "Dimension Data Type           :     %d\n",
                        getIdamDimType(handle, irank));
                fprintf(stdout, "Dimension Error Type          :     %d\n",
                        getIdamDimErrorType(handle, irank));
                fprintf(stdout, "Dimension Error Asymmetry     :      %d\n",
                        getIdamDimErrorAsymmetry(handle, irank));

                nmax = 10;
                ndata = getIdamDimNum(handle, irank);

                if (ndata < 10) {
                    nmax = ndata;
                }

                type = getIdamDimType(handle, irank);

                switch (type) {
                case (UDA_TYPE_FLOAT):
                    fvec = (float *) getIdamDimData(handle, irank); // pointer to the Dimension?s Data

                    for (int i = 0; i < nmax; i++) {
                        fprintf(stdout, "%d %f\n", i, fvec[i]);    // print some data
                    }

                    break;

                case (UDA_TYPE_DOUBLE):
                    dvec = (double *) getIdamDimData(handle, irank);

                    for (int i = 0; i < nmax; i++) {
                        fprintf(stdout, "%d %f\n", i, dvec[i]);
                    }

                    break;
                }

            }
        }

        udaFree(handle);       // Free Heap for this signal
    }

    udaFreeAll();              // Free all Heap and signal the server to close
    return 0;
}
