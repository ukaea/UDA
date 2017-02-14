/*---------------------------------------------------------------
* IDAM Plugin data Reader to Access DATA from TRANSP UFiles
*
* Input Arguments:	DATA_SOURCE data_source
*			SIGNAL_DESC signal_desc
*
* Returns:		readUFile	0 if read was successful
*					otherwise a Error Code is returned
*			DATA_BLOCK	Structure with Data from the U File
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
*-----------------------------------------------------------------------------*/
#include "readUFile.h"

#include <stdio.h>
#include <errno.h>
#include <memory.h>
#include <stdlib.h>
#include <strings.h>

#include <clientserver/idamErrorLog.h>
#include <clientserver/initStructs.h>
#include <clientserver/stringUtils.h>
#include <clientserver/idamTypes.h>
#include <clientserver/idamErrors.h>

//---------------------------------------------------------------------------------------------------------------
// Stub plugin if disabled

#ifdef NOUFILEPLUGIN

int readUFile(DATA_SOURCE data_source,
              SIGNAL_DESC signal_desc,
              DATA_BLOCK *data_block) {
    int err = 999;
    addIdamError(&idamerrorstack, CODEERRORTYPE, "readCDF", err, "UFILE PLUGIN NOT ENABLED");
    return err;
}

#else

//---------------------------------------------------------------------------------------------------------------

#define UFILE_ERROR_OPENING_FILE            200
#define UFILE_ERROR_ALLOCATING_DIM_HEAP     201
#define UFILE_ERROR_ALLOCATING_DATA_HEAP    202

#define TEST            0
#define UFILE_BUFFER    256

int readUFile(DATA_SOURCE data_source,
              SIGNAL_DESC signal_desc,
              DATA_BLOCK* data_block)
{

    FILE* ufile = NULL;
    char buff[UFILE_BUFFER], tokamak[5], date[10], desc[21];
    char value[14];
    int i, j, k, err, nscalar, serrno = 0;
    int nrec, remain, ncheck, nitems;
    float scalar;
    float* data = NULL;

//----------------------------------------------------------------------
// Error Trap Loop

    do {

//----------------------------------------------------------------------
// Open the U File for READing Only

        errno = 0;
        err = 0;

        ufile = fopen(data_source.path, "r");

        if (errno != 0 || ufile == NULL) {
            err = UFILE_ERROR_OPENING_FILE;
            serrno = errno;
            if (serrno != 0) addIdamError(&idamerrorstack, SYSTEMERRORTYPE, "readUFile", serrno, "");
            addIdamError(&idamerrorstack, CODEERRORTYPE, "readUFile", err, "Error Opening U File");
            if (ufile != NULL) fclose(ufile);
            break;
        }

//----------------------------------------------------------------------
// Read the UFile Header Information: Formated as (1x,I6,A4,I2) nshot, tok, rank

        fgets(buff, UFILE_BUFFER - 1, ufile);

        strcpy(tokamak, strtok(NULL, " "));
        data_block->rank = atoi(strtok(NULL, " "));

//----------------------------------------------------------------------
// Read the Shot Date: Formated as (1x,a9)

        fscanf(ufile, "%10s", date);
        fgets(buff, UFILE_BUFFER - 1, ufile);

//----------------------------------------------------------------------
// Number of Associated Scalars: Formated as (1x,i3)

        nscalar = atoi(fgets(buff, 8, ufile));
        fgets(buff, UFILE_BUFFER - 1, ufile);

//----------------------------------------------------------------------
// Read All Scalar Data  & Ignore!

//  For Development Assume No Scalar Data if 1 or 2d data
//  Ignore ALL Scalar values associated with data profiles

//  Scalars are Formated as 1E13.6
//  Scalar Descriptions as A20

        if (data_block->rank != 0 && nscalar != 0) {
            for (i = 0; i < nscalar; i++) {
                scalar = atof(fgets(buff, 14, ufile));
                fgets(buff, UFILE_BUFFER - 1, ufile);

                fscanf(ufile, "%21s", desc);
                fgets(buff, UFILE_BUFFER - 1, ufile);

                if (TEST)printf("Scalar: %s = %13.6f\n", desc, scalar);

            }
        }

//----------------------------------------------------------------------
// Allocate & Initialise Dimensional Structures

        if (data_block->rank > 0) {
            if ((data_block->dims = (DIMS*) malloc(data_block->rank * sizeof(DIMS))) == NULL) {
                err = UFILE_ERROR_ALLOCATING_DIM_HEAP;
                addIdamError(&idamerrorstack, CODEERRORTYPE, "readUFile", err,
                             "Problem Allocating Dimension Heap Memory");
                break;
            }
        }

        for (i = 0; i < data_block->rank; i++) initDimBlock(&data_block->dims[i]);

//----------------------------------------------------------------------
// Dimension Labels and Units

        for (i = 0; i < data_block->rank; i++) {
            fgets(data_block->dims[i].dim_label, 22, ufile);        // Formated as (1x,a20,a10)
            fgets(data_block->dims[i].dim_units, 11, ufile);
            TrimString(data_block->dims[i].dim_label);
            TrimString(data_block->dims[i].dim_units);
            if (!strcasecmp(data_block->dims[i].dim_label, "TIME")) data_block->order = i;
            fgets(buff, UFILE_BUFFER - 1, ufile);
        }

//----------------------------------------------------------------------
// Data Label and Units

        fgets(data_block->data_label, 22, ufile);            // Formated as (1x,a20,a10)
        fgets(data_block->data_units, 11, ufile);
        TrimString(data_block->data_label);
        TrimString(data_block->data_units);

        fgets(buff, UFILE_BUFFER - 1, ufile);

//----------------------------------------------------------------------
// Status

//        status = atoi(fgets(buff, 3, ufile));            // Formated as (1x,i1)
        fgets(buff, UFILE_BUFFER - 1, ufile);

//----------------------------------------------------------------------
// Dimension Sizes & Types: Allocate Dimensional Heap

        data_block->data_n = 0;
        data_block->data_type = TYPE_FLOAT;

        for (i = 0; i < data_block->rank; i++) {
            data_block->dims[i].data_type = TYPE_FLOAT;
            data_block->dims[i].dim_n = atoi(fgets(buff, 12, ufile));  // Formated as (1x,i10)
            if (i == 0)
                data_block->data_n = data_block->dims[i].dim_n;
            else
                data_block->data_n = data_block->data_n * data_block->dims[i].dim_n;
            if ((data_block->dims[i].dim = (char*) malloc(sizeof(float) * data_block->dims[i].dim_n)) == NULL) {
                err = UFILE_ERROR_ALLOCATING_DIM_HEAP;
                addIdamError(&idamerrorstack, CODEERRORTYPE, "readUFile", err,
                             "Problem Allocating Dimensional Heap Memory");
                break;
            }
            fgets(buff, UFILE_BUFFER - 1, ufile);
        }
        if (err != 0) break;

//--------------------------------------------------------------------------------------------
// Allocate Heap for the Data and Read the Data

        if ((data_block->data = (char*) malloc(sizeof(float) * data_block->data_n)) == NULL) {
            err = UFILE_ERROR_ALLOCATING_DATA_HEAP;
            addIdamError(&idamerrorstack, CODEERRORTYPE, "readUFile", err, "Problem Allocating Data Heap Memory");
            break;
        }

//--------------------------------------------------------------------------------------------
// Read the Dimensional Data

        value[13] = '\0';

        for (i = 0; i < data_block->rank; i++) {
            data = (float*) data_block->dims[i].dim;
            nrec = data_block->dims[i].dim_n / 6;        // No. of Records to read
            remain = data_block->dims[i].dim_n - 6 * nrec;        // No. of Data Items in Last Record
            ncheck = 0;                        // Check Count of Items

            for (j = 0; j < nrec + 1; j++) {
                fgets(buff, UFILE_BUFFER - 1, ufile);
                nitems = ((j == (nrec)) ? remain : 6);        // Each record has 6 Items (except the last!)
                for (k = 0; k < nitems; k++) {
                    strncpy(value, &buff[1 + 13 * k], 13);
                    *(data + ncheck++) = atof(value);
                }
            }
        }

//--------------------------------------------------------------------------------------------
// Read the Data

        data = (float*) data_block->data;
        nrec = data_block->data_n / 6;                // No. of Records to read
        remain = data_block->data_n - 6 * nrec;            // No. of Data Items in Last Record
        ncheck = 0;                        // Check Count of Items

        for (j = 0; j < nrec + 1; j++) {
            fgets(buff, UFILE_BUFFER - 1, ufile);
            nitems = ((j == (nrec)) ? remain : 6);            // Each record has 6 Items (except the last!)
            for (k = 0; k < nitems; k++) {
                strncpy(value, &buff[1 + 13 * k], 13);
                *(data + ncheck++) = atof(value);

                if (TEST)fprintf(stdout, "[%d] %12.4e\n", ncheck - 1, (double) *(data + ncheck - 1));
            }
        }

//----------------------------------------------------------------------
//----------------------------------------------------------------------
// End of Error Trap Loop

    } while (0);

//----------------------------------------------------------------------
// Housekeeping

    if (ufile != NULL) fclose(ufile);

    return err;
}

#endif
