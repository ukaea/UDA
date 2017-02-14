/*---------------------------------------------------------------
  IDAM Plugin data Reader to Access DATA from JPF Files

  Input Arguments:	DATA_SOURCE data_source
 			SIGNAL_DESC signal_desc

  Returns:		readJPF		0 if read was successful
 					otherwise a Error Code is returned
 			DATA_BLOCK	Structure with Data from the IDA File

  Calls		freeDataBlock	to free Heap memory if an Error Occurs

  Notes: 	All memory required to hold data is allocated dynamically
 		in heap storage. Pointers to these areas of memory are held
 		by the passed DATA_BLOCK structure. Local memory allocations
 		are freed on exit. However, the blocks reserved for data are
 		not and MUST BE FREED by the calling routine.

  ToDo:

*/

#include "readjpf.h"

#include <stdio.h>

#include <clientserver/idamErrorLog.h>
#include <clientserver/idamErrors.h>

/*
  Disabled, stub plugin.
*/
#ifdef	NOJPFPLUGIN

int readJPF(DATA_SOURCE data_source, SIGNAL_DESC signal_desc, DATA_BLOCK *data_block) {
    int err = 999;
    addIdamError(&idamerrorstack, CODEERRORTYPE, "readCDF", err, "JPF PLUGIN NOT ENABLED");
    return err;
}

#else

#include <stdarg.h>

#include <clientserver/initStructs.h>
#include <clientserver/idamTypes.h>
#include <clientserver/stringUtils.h>

/*
  JPF plugin.
*/
int mydprintf(FILE* fp, const char *  fmt, ...)
{
    va_list list;
    int i;
    fprintf( fp, ":dsand:");

    va_start(list, fmt);
    i = vfprintf( fp, fmt, list);
    va_end(list);
    return i ;
}

int readJPF( DATA_SOURCE data_source
             , SIGNAL_DESC signal_desc
             , DATA_BLOCK *data_block)
{
    int nd, ndw, err, isScalar=0;
    long lstr;

    char desc[129]="", units[11]="";

    float  *dvec=NULL;	//Data and T vector arrays
    double *tvec=NULL;

    int pulse = 0;

    if(data_source.exp_number == 0) {
        err = 999;
        addIdamError(&idamerrorstack, CODEERRORTYPE, "readJPF"
                     , err, "No Shot Number!");
        return err;
    }

    if( ! (lstr = strlen(signal_desc.signal_name)))
    {
        err = 999;
        addIdamError(&idamerrorstack, CODEERRORTYPE, "readJPF"
                     , err, "No Node Name!");
        return err;
    }

    //--------------------------------------------------------------------
    // Get the number of samples to use for the GETDAT|SCA|TIM call and to
    // allocate the data buffer.
    pulse = data_source.exp_number;

    if( pulse < 0)
    {
        gxlcon_();                 /* local on, offline */
        pulse = -pulse;
    }
    else
        gxlcof_();                 /* local off, online */

    getnwds_(signal_desc.signal_name, &pulse, &ndw, &err, lstr );

    if(err != 0) {
        err = 998;
        addIdamError(&idamerrorstack, CODEERRORTYPE, "readJPF"
                     , err, "Failed enquiry for JPF data array size");
        return err;
    }

    if(ndw <= 0) {
        err = 998;
        addIdamError(&idamerrorstack, CODEERRORTYPE, "readJPF"
                     , err, "JPF data array size is Zero!");
        return err;
    }

    //--------------------------------------------------------------------
    // Call GETDAT to retrieve JPF Data

    // Allocate Heap Memory

    if( ! (dvec = malloc(ndw*sizeof(float)))) {
        err = 998;
        addIdamError(&idamerrorstack, CODEERRORTYPE, "readJPF"
                     , err, "Unable to Allocate Heap for Data");
        return err;
    }

    if( ! (tvec = malloc(ndw*sizeof(double)))) {
        err = 997;
        addIdamError(&idamerrorstack, CODEERRORTYPE, "readJPF"
                     , err, "Unable to Allocate Heap for the Time Vector");
        free(dvec);
        return err;
    }

    // Call GETDAT8 to retrieve JPF Data and double tvec.

    nd = ndw;

    getdat8_( signal_desc.signal_name, &pulse, dvec, tvec, &nd, desc, units, &err
              , lstr, sizeof( desc) - 1, sizeof( units) - 1);

#if	(0)
    if(TEST) {
        fprintf(stdout,"getdat calls : %d\n", ntry);
        fprintf(stdout,"nd:            %d\n", nd);
        fprintf(stdout,"err:           %d\n", err);
    }
#endif

    /*
      For present allow 33 (truncation).
    */

    if( err == 33) err = 0;

    if (err) {

        nd = ndw;

        getsca_(signal_desc.signal_name, &pulse, dvec, &nd, desc, units, &err,
                lstr, sizeof( desc) - 1, sizeof( units) - 1);

        free(tvec);
        if(err) {
            addIdamError(&idamerrorstack, CODEERRORTYPE, "readJPF"
                         , err, "Unable to Obtain Scalar JPF Data");
            free(dvec);
            return err;
        }
        isScalar = 1;
    }

    if (err) {
        addIdamError(&idamerrorstack, CODEERRORTYPE, "readJPF"
                     , err, "Unable to Obtain JPF Data");
        free(dvec);
        free(tvec);
        return err;
    }

    //--------------------------------------------------------------------
    // Copy Data into Structures for Returning

#define	_s0(s)		(s)[sizeof(s)-1]=(0)

    _s0( desc);
    _s0( units);

    TrimString(desc);
    TrimString(units);

    data_block->data_type = TYPE_FLOAT;

    data_block->data = (char *)dvec;

    data_block->data_n = nd;
    data_block->rank   = 1;
    data_block->order  = 0;
    data_block->source_status = 0;
    data_block->signal_status = 0;

#define	_strcpy(s,t)	(snprintf((s),sizeof(s),"%s",t))

    _strcpy(data_block->data_units, units);
    _strcpy(data_block->data_label, "");
    _strcpy(data_block->data_desc, desc);

    // Dimension Data

    if(!isScalar) {

        data_block->dims = (DIMS *)malloc(data_block->rank * sizeof(DIMS));

        initDimBlock(data_block->dims);

        data_block->dims[0].dim_n = nd;
        data_block->dims[0].dim   = (char *)tvec;
        _strcpy(data_block->dims[0].dim_units, "s");		/* SI */

        _strcpy(data_block->dims[0].dim_label, "Time");

        data_block->dims[0].data_type = TYPE_DOUBLE;

    }
    else {
        data_block->rank   = 0;
        data_block->order  = -1;
    }

    //--------------------------------------------------------------------
    // Housekeeping

    return 0;
}
#endif
