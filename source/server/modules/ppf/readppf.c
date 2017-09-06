/*---------------------------------------------------------------
* IDAM Plugin data Reader to Access DATA from PPF Files
*
* Input Arguments:	DATA_SOURCE data_source
*			SIGNAL_DESC signal_desc
*
* Returns:		readPPF		0 if read was successful
*					otherwise a Error Code is returned
*			DATA_BLOCK	Structure with Data from the IDA File
*
* Calls		freeDataBlock	to free Heap memory if an Error Occurs
*
* Notes: 	All memory required to hold data is allocated dynamically
*		in heap storage. Pointers to these areas of memory are held
*		by the passed DATA_BLOCK structure. Local memory allocations
*		are freed on exit. However, the blocks reserved for data are
*		not and MUST BE FREED by the calling routine.
*
* ToDo:
*
*-----------------------------------------------------------------------------*/
#include "readppf.h"

#include <clientserver/errorLog.h>
#include <clientserver/udaErrors.h>
#include <logging/logging.h>

//---------------------------------------------------------------------------------------------------------------
// Stub plugin if disabled

#ifdef NOPPFPLUGIN

int readPPF(DATA_SOURCE data_source,
            SIGNAL_DESC signal_desc,
            DATA_BLOCK *data_block) {
    int err = 999;
    addIdamError(&idamerrorstack, CODEERRORTYPE, "readCDF", err, "PPF PLUGIN NOT ENABLED");
    return err;
}

#else

//---------------------------------------------------------------------------------------------------------------

#include <ppf.h>

#include <stdio.h>

#include <clientserver/stringUtils.h>
#include <clientserver/udaTypes.h>
#include <clientserver/initStructs.h>

#define NWCOM     20        // 80 Byte DDA Comment
#define NDTNAMS   500        // Overkill?

#define TEST    0        // Output Debug Data

int readPPF(DATA_SOURCE data_source,
            SIGNAL_DESC signal_desc,
            DATA_BLOCK* data_block) {

    int nwcom = NWCOM, ndt = NDTNAMS, lxtv[2 * NDTNAMS], err = 0, err2 = 0, lowner = 0, xsubset = 0;
    int pulno, pass, rank, order, nx, nt, lun = 0, ndmax, dtid, i;
    int irdat[13], iwdat[13];
    int swap[] = {0, 1, 0};            // Finding the Correct Dimension Index

    char ddacom[4 * NWCOM + 1], dtnams[4 * NDTNAMS + 1];
    char dda[5], dtype[5], ihdat[60], msg[81];
    char test[5];

    float* dvec = NULL, * xvec = NULL, * tvec = NULL, * sdvec = NULL;    //Data, x vector and T vector arrays

#ifndef FATCLIENT
    static int pwdstatus = 1;
#endif

//--------------------------------------------------------------------
// Extract Signal from SubSet Instruction if present.
// Model: "dtyp(n)" where dtyp is the Datatype Name and n is the Dimension Slice to Subset >= 1
// Default is X Slice: No syntax specifying which dimension
// Reduce Rank by 1 and subset the X-Dimension as requested

    TrimString(signal_desc.signal_name);

    char* p2 = strrchr(signal_desc.signal_name, ')');
    if (p2 != NULL) {
        if (TEST)fprintf(stdout, "Non-Standard PPF Signal: %s\n", signal_desc.signal_name);

        p2[0] = '\0';
        strcpy(dtype, "    ");

        char* p1 = strrchr(signal_desc.signal_name, '(');
        if (p1 != NULL && p2 - p1 > 1) {
            p1[0] = '\0';
            if ((xsubset = atoi(&p1[1])) == 0) {
                err = 999;
                addIdamError(&idamerrorstack, CODEERRORTYPE, "readPPF", err,
                             "Subsetting the X-Dimension begins at slice 1 not 0!");
                return err;
            }
        } else {
            err = 999;
            addIdamError(&idamerrorstack, CODEERRORTYPE, "readPPF", err, "Non-Standard PPF Signal Name Syntax Error!");
            return err;
        }
        strcpy(dtype, signal_desc.signal_name);
        if (TEST) {
            fprintf(stdout, "Data Type         : %s\n", dtype);
            fprintf(stdout, "X Dimension Subset: %d\n", xsubset);
        }
    } else {
        if (TEST) fprintf(stdout, "Standard PPF Signal: %s\n", signal_desc.signal_name);
        strncpy(dtype, signal_desc.signal_name, 4);
        dtype[4] = '\0';
    }

// Bug Fix: D.G.Muir 20Dec2016
/*

    if(signal_desc.signal_name[ldtype-1]==')') {
        if(TEST)fprintf(stdout,"Non-Standard PPF Signal: %s\n", signal_desc.signal_name);

        signal_desc.signal_name[ldtype-1]='\0';
        strcpy(dtype,"    ");
        for(i=0; i<5; i++) {
            if(signal_desc.signal_name[i]!='(') {
                if(i <= 3) dtype[i] = signal_desc.signal_name[i];
            } else {
                xsubset = atoi(signal_desc.signal_name+i+1);
                if(TEST) {
                    fprintf(stdout,"Data Type         : %s\n", dtype);
                    fprintf(stdout,"X Dimension Subset: %d\n", xsubset);
                }
            }
        }
    } else {
        if(TEST) fprintf(stdout,"Standard PPF Signal: %s\n", signal_desc.signal_name);
        strncpy(dtype, signal_desc.signal_name, 4);
        dtype[4] = '\0';
    }
*/

//--------------------------------------------------------------------
// Signal etc.

    strncpy(dda, data_source.filename, 4);    // Only 4 Characters Allowed!
    dda[4] = '\0';

    strupr(dda);                    // Change Case to Upper
    strupr(dtype);
    TrimString(dtype);

    pulno = (int) data_source.exp_number;
    pass = (int) data_source.pass;        // PPF Sequence Number

    if (TEST) {
        fprintf(stdout, "Pulse : %d\n", pulno);
        fprintf(stdout, "Seq.  : %d\n", pass);
        fprintf(stdout, "DDA   : %s\n", dda);
        fprintf(stdout, "Signal: %s\n", dtype);
        fprintf(stdout, "Owner : %s\n", data_source.path);
    }

   IDAM_LOGF(UDA_LOG_DEBUG,"Pulse : %d\n", pulno);
   IDAM_LOGF(UDA_LOG_DEBUG,"Seq.  : %d\n", pass);
   IDAM_LOGF(UDA_LOG_DEBUG,"DDA   : %s\n", dda);
   IDAM_LOGF(UDA_LOG_DEBUG,"Signal: %s\n", dtype);
   IDAM_LOGF(UDA_LOG_DEBUG,"Owner : %s\n", data_source.path);
   IDAM_LOGF(UDA_LOG_DEBUG,"X Dimension Subset: %d\n", xsubset);

//--------------------------------------------------------------------
// Setup PPFUID call - specify the user name used for reading data

#ifndef FATCLIENT
    if (pwdstatus) {
        PPFPWD("ppfread", "ppfread", 8, 8);            // Read Only Permission (No .netrc needed)
        pwdstatus = 0;
    }
#endif

    if ((lowner = strlen(data_source.path)) > 0)
        PPFUID(data_source.path, "R", lowner, 1);        // Private PPF's Only
    else
        PPFUID("JETPPF", "R", 7, 1);            // Public PPF's Only

//--------------------------------------------------------------------
// Call PPFGO with the specified shot and sequence number

    if (pass >= 0)
        PPFGO(&pulno, &pass, &lun, &err);
    else
        PPFGO(&lun, &lun, &lun, &err);

    if (err != 0) {
        PPFERR("PPFGO", &err, msg, &err2, 6, 81);
        msg[80] = '\0';
        TrimString(msg);
        IDAM_LOGF(UDA_LOG_DEBUG,"PPFGO Error : %s\n", msg);
        if (err2 != 0)
            addIdamError(&idamerrorstack, CODEERRORTYPE, "readPPF", err, "PPFGO Error");
        else
            addIdamError(&idamerrorstack, CODEERRORTYPE, "readPPF", err, msg);
        PPFUID("JETPPF", "R", 7, 1);            // Reset to reading Public PPF's Only
        return err;
    }

//--------------------------------------------------------------------
// Get DDA and Datatype Information

    ddacom[4 * NWCOM] = '\0';
    dtnams[4 * NDTNAMS] = '\0';

    DDAINF(&pulno, dda, &nwcom, ddacom, &ndt, dtnams, lxtv,
           &err, 5, 4 * NWCOM + 1, 4 * NDTNAMS + 1);

    if (err != 0) {
        PPFERR("DDAINF", &err, msg, &err2, 7, 81);
        msg[80] = '\0';
        TrimString(msg);
        IDAM_LOGF(UDA_LOG_DEBUG,"DDAINF Error : %s\n", msg);
        if (err2 != 0)
            addIdamError(&idamerrorstack, CODEERRORTYPE, "readPPF", err, "DDAINF Error");
        else
            addIdamError(&idamerrorstack, CODEERRORTYPE, "readPPF", err, msg);
        PPFUID("JETPPF", "R", 7, 1);            // Reset to reading Public PPF's Only
        return err;
    }

    if (TEST) {
        fprintf(stdout, "DDA Comment   : %s\n", ddacom);
        fprintf(stdout, "No. Data Types: %d\n", ndt);
        fprintf(stdout, "Signal List   : %s\n", dtnams);
    }

    IDAM_LOGF(UDA_LOG_DEBUG,"DDA Comment   : %s\n", ddacom);
    IDAM_LOGF(UDA_LOG_DEBUG,"No. Data Types: %d\n", ndt);
    IDAM_LOGF(UDA_LOG_DEBUG,"Signal List   : %s\n", dtnams);


//--------------------------------------------------------------------
// Extract Requested DataType Info

    dtid = -1;

    for (i = 0; i < ndt; i++) {
        test[4] = '\0';
        strncpy(test, dtnams + i * 4, 4);    // Choose a data type name for testing
        TrimString(test);
        if (STR_EQUALS(dtype, test)) {
            dtid = i;            // Found a Match
            if (TEST) {
                fprintf(stdout, "Signal Located: %d\n", dtid);
                fprintf(stdout, "Signal Name   : %s\n", dtnams + dtid * 4);
            }
            break;
        }
    }

    if (TEST) for (i = 0; i < 2 * ndt; i++)fprintf(stdout, "lxtv+%d = %d\n", i, lxtv[i]);

    if (dtid >= 0) {
        nx = lxtv[2 * dtid];    // Length of the X Dimension
        nt = lxtv[2 * dtid + 1];        // Length of the T Dimension
        if (TEST) {
            fprintf(stdout, "nx: %d\n", nx);
            fprintf(stdout, "nt: %d\n", nt);
        }
    } else {
        err = 999;
        addIdamError(&idamerrorstack, CODEERRORTYPE, "readPPF", err,
                     "Unable to Identify the PPF DDA Data-Type Requested");
        PPFUID("JETPPF", "R", 7, 1);
        return err;
    }


// Subsetting indicies begin with value 1
    if (xsubset > 0 && xsubset > nx) {
        err = 995;
        addIdamError(&idamerrorstack, CODEERRORTYPE, "readPPF", err,
                     "The Requested X-Dimension Subset Exceeds the Valid X-Dimensions");
        PPFUID("JETPPF", "R", 7, 1);
        return err;
    }

    ndmax = 1;
    if (nx > 0) ndmax = ndmax * nx;
    if (nt > 0) ndmax = ndmax * nt;

//--------------------------------------------------------------------
// Allocate Heap Memory for Data

    if ((dvec = (float*) malloc(ndmax * sizeof(float))) == NULL) {
        err = 998;
        addIdamError(&idamerrorstack, CODEERRORTYPE, "readPPF", err, "Data Heap Memory Allocation Failed");
        PPFUID("JETPPF", "R", 7, 1);
        return err;
    }

    if (nt > 0) {
        if ((tvec = (float*) malloc(nt * sizeof(float))) == NULL) {
            err = 997;
            addIdamError(&idamerrorstack, CODEERRORTYPE, "readPPF", err, "Tvec Heap Memory Allocation Failed");
            PPFUID("JETPPF", "R", 7, 1);
            return err;
        }
    }

    if (nx > 0) {
        if ((xvec = (float*) malloc(nx * sizeof(float))) == NULL) {
            err = 996;
            addIdamError(&idamerrorstack, CODEERRORTYPE, "readPPF", err, "Xvec Heap Memory Allocation Failed");
            PPFUID("JETPPF", "R", 7, 1);
            return err;
        }
    }

//--------------------------------------------------------------------
// Setup PPFGET call
// array sizes to hold retrieved X, T and DATA vectors

    irdat[0] = 0;
    irdat[1] = nx;
    irdat[2] = 0;
    irdat[3] = nt;
    irdat[4] = ndmax;        // Length of the Data Block
    irdat[5] = nx;        // X DImension
    irdat[6] = nt;        // T Dimendion
    irdat[7] = 0;        // Set flags to return x- and t-vectors
    irdat[8] = 0;

    if (nx <= 0)irdat[7] = -1;
    if (nt <= 0)irdat[8] = -1;

// Call PPFGET to retrieve DataType.

    PPFGET(&pulno, dda, dtype, irdat, ihdat, iwdat, dvec, xvec, tvec, &err,
           5, 5, 60);

    if (err != 0) {
        PPFERR("PPFGET", &err, msg, &err2, 7, 81);
        msg[80] = '\0';
        TrimString(msg);
        if (err2 != 0)
            addIdamError(&idamerrorstack, CODEERRORTYPE, "readPPF", err, "PPFGET Error");
        else
            addIdamError(&idamerrorstack, CODEERRORTYPE, "readPPF", err, msg);
        if (dvec != NULL)free((void*) dvec);
        if (tvec != NULL)free((void*) tvec);
        if (xvec != NULL)free((void*) xvec);
        PPFUID("JETPPF", "R", 7, 1);            // Reset to reading Public PPF's Only
        return err;
    }

//--------------------------------------------------------------------
// Copy Data into Structures for Returning

// Observational Data

    if (ihdat[24] == 'F') {
        data_block->data_type = TYPE_FLOAT;
    } else {
        if (ihdat[24] == 'I') {
            data_block->data_type = TYPE_INT;
        } else {
            data_block->data_type = TYPE_UNKNOWN;
        }
    }

    //fprintf(stdout,"Data is of Type: %c\n",ihdat[24]);

    data_block->error_type = TYPE_UNKNOWN;

    data_block->data = (char*) dvec;
    data_block->data_n = iwdat[4];

    rank = 0;
    if (nt > 0)rank++;

    if (nx > 1 && xsubset == 0)rank++;    // X Dimension must be > 1 in length

    data_block->rank = rank;

    data_block->source_status = iwdat[10];        // PPF/DDA Status
    data_block->signal_status = iwdat[11];        // Signal Status

// Based on ECM1/PRFL data from PPFGET are arranged: data[t][x] => t dimension is order 1 not 0

    order = 0;
    if (rank == 2) order = 1;
    data_block->order = order;

    strncpy(data_block->data_units, ihdat, 8);
    data_block->data_units[8] = '\0';

    strcpy(data_block->data_label, "");
    strncpy(data_block->data_desc, ihdat + 36, 24);
    data_block->data_desc[24] = '\0';
    TrimString(data_block->data_desc);

// Dimension Data

    if (rank > 0) {
        data_block->dims = (DIMS*) malloc(data_block->rank * sizeof(DIMS));
    } else {
        data_block->dims = NULL;
    }

    for (i = 0; i < rank; i++) {
        initDimBlock(&data_block->dims[i]);
    }

// T axis

    if (nt > 0 && order >= 0) {
        data_block->dims[order].dim_n = iwdat[6];
        data_block->dims[order].dim = (char*) tvec;
        strncpy(data_block->dims[order].dim_units, ihdat + 16, 8);
        data_block->dims[order].dim_units[8] = '\0';
        strcpy(data_block->dims[order].dim_label, "Time");

        if (ihdat[32] == 'F') {
            data_block->dims[order].data_type = TYPE_FLOAT;
        } else {
            if (ihdat[32] == 'I') {
                data_block->dims[order].data_type = TYPE_INT;
            } else {
                data_block->dims[order].data_type = TYPE_UNKNOWN;
            }
        }
    }

// X axis (Only filled if no subsetting occurs)

    if (nx > 1 && xsubset == 0) {
        data_block->dims[swap[order + 1]].dim_n = iwdat[5];
        data_block->dims[swap[order + 1]].dim = (char*) xvec;
        strncpy(data_block->dims[swap[order + 1]].dim_units, ihdat + 8, 8);
        data_block->dims[swap[order + 1]].dim_units[8] = '\0';
        strcpy(data_block->dims[swap[order + 1]].dim_label, "X");
        if (ihdat[28] == 'F') {
            data_block->dims[swap[order + 1]].data_type = TYPE_FLOAT;
        } else {
            if (ihdat[28] == 'I') {
                data_block->dims[swap[order + 1]].data_type = TYPE_INT;
            } else {
                data_block->dims[swap[order + 1]].data_type = TYPE_UNKNOWN;
            }
        }
    }

//--------------------------------------------------------------------
// Sub-Set the X Dimension: create New Heap, Copy and Free the Old Data Block

    if (xsubset > 0) {

        if ((sdvec = (float*) malloc(nt * sizeof(float))) == NULL) {
            err = 998;
            addIdamError(&idamerrorstack, CODEERRORTYPE, "readPPF", err, "Xvec Subset Heap Memory Allocation Failed");
            PPFUID("JETPPF", "R", 7, 1);
            return err;
        }

// Arranged as: data[t][x]

        for (i = 0; i < nt; i++) sdvec[i] = dvec[xsubset - 1 + nx * i];

        free((void*) dvec);
        free((void*) xvec);

        dvec = NULL;
        xvec = NULL;

        data_block->data = (char*) sdvec;
        data_block->data_n = nt;

    }

    if (nx == 1 && xsubset == 0 && xvec != NULL) free((void*) xvec);

//--------------------------------------------------------------------
// Housekeeping

    PPFUID("JETPPF", "R", 7, 1);            // Reset to reading Public PPF's Only

    return 0;
}

#endif
