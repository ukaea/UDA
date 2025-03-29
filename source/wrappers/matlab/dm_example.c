/* #include "idamclientserver.h"
#include "idamclient.h"
*/
#include "stdio.h"
#include "idammatlab.h"

int main()
{
    int handle, nmax, ndata, rank, i, irank;
    float* fvec = NULL;
    double* dvec = NULL;

    int type;

    //udaSetProperty("verbose");   // Print Errors
    //udaSetProperty("debug"); // Print Debug Trail
    //udaPutServerHost("fuslwn");  // Identify the Server?s Host
    //udaPutServerPort(56565); // Identify the Server?s Port

    handle = udaGetAPI("ip", "13500"); // Execute the API

    fprintf(stdout, "Test: Handle %d, Error Code %d [%s]\n", handle, udaGetErrorCode(handle),
            udaGetErrorMsg(handle));

    if (handle >= 0 && udaGetErrorCode(handle) == 0) { // Test Data Access was OK
        fprintf(stdout, "Returned Source Status:     %d\n", udaGetSourceStatus(handle));
        fprintf(stdout, "Returned Signal Status:     %d\n", udaGetSignalStatus(handle));
        fprintf(stdout, "Returned Data Status  :     %d\n", udaGetDataStatus(handle));
        fprintf(stdout, "No. Data Elements     :     %d\n", udaGetDataNum(handle));
        fprintf(stdout, "Rank                  :     %d\n", udaGetRank(handle));
        fprintf(stdout, "Order of Time Vector  :     %d\n", udaGetOrder(handle));
        fprintf(stdout, "Data Description      :     %s\n", udaGetDataDesc(handle));
        fprintf(stdout, "Data Units            :     %s\n", udaGetDataUnits(handle));
        fprintf(stdout, "Data Label            :     %s\n", udaGetDataLabel(handle));
        fprintf(stdout, "Data Type             :     %d\n", udaGetDataType(handle));
        fprintf(stdout, "Error Type            :     %d\n", udaGetDataErrorType(handle));
        fprintf(stdout, "Error Asymmetry?      :     %d\n", udaGetDataErrorAsymmetry(handle));

        nmax = 10;
        ndata = udaGetDataNum(handle);

        if (ndata < 10) {
            nmax = ndata;
        }

        type = udaGetDataType(handle);

        switch (type) {
            case (UDA_TYPE_FLOAT):
                fvec = (float*)udaGetData(handle);       // pointer to the Data

                for (int i = 0; i < nmax; i++) {
                    fprintf(stdout, "%d %f\n", i, fvec[i]);    // print some data
                }

                break;

            case (UDA_TYPE_DOUBLE):
                dvec = (double*)udaGetData(handle);

                for (int i = 0; i < nmax; i++) {
                    fprintf(stdout, "%d %f\n", i, dvec[i]);
                }

                break;
        }

        rank = udaGetRank(handle);

        if (rank > 0) {         // Are there dimensions?
            for (irank = 0; irank < rank; irank++) {
                fprintf(stdout, "Dimension Id                  :     %d\n", irank);

                if (irank == udaGetOrder(handle)) {
                    fprintf(stdout, "This is the Time Dimension\n");
                }

                fprintf(stdout, "No. Dimension Elements        :     %d\n",
                        udaGetDimNum(handle, irank));
                fprintf(stdout, "Dimension units               :     %s\n",
                        udaGetDimUnits(handle, irank));
                fprintf(stdout, "Dimension Label               :     %s\n",
                        udaGetDimLabel(handle, irank));
                fprintf(stdout, "Dimension Data Type           :     %d\n",
                        udaGetDimType(handle, irank));
                fprintf(stdout, "Dimension Error Type          :     %d\n",
                        udaGetDimErrorType(handle, irank));
                fprintf(stdout, "Dimension Error Asymmetry     :      %d\n",
                        udaGetDimErrorAsymmetry(handle, irank));

                nmax = 10;
                ndata = udaGetDimNum(handle, irank);

                if (ndata < 10) {
                    nmax = ndata;
                }

                type = udaGetDimType(handle, irank);

                switch (type) {
                    case (UDA_TYPE_FLOAT):
                        fvec = (float*)udaGetDimData(handle, irank); // pointer to the Dimension?s Data

                        for (int i = 0; i < nmax; i++) {
                            fprintf(stdout, "%d %f\n", i, fvec[i]);    // print some data
                        }

                        break;

                    case (UDA_TYPE_DOUBLE):
                        dvec = (double*)udaGetDimData(handle, irank);

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
