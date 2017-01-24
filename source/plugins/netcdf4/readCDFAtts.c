//! $LastChangedRevision: 70 $
//! $LastChangedDate: 2008-06-26 10:44:44 +0100 (Thu, 26 Jun 2008) $
//! $LastChangedBy: dgm $
//! $HeadURL: https://fussvn.fusion.culham.ukaea.org.uk/svnroot/IDAM/development/source/plugins/netcdf/readCDFAtts.c $

/*---------------------------------------------------------------
* IDAM Plugin data Reader to Access DATA from netCDF4 Files
*
* Input Arguments:  DATA_SOURCE data_source
*           SIGNAL_DESC signal_desc
*
* Returns:      readCDF     0 if read was successful
*                   otherwise a Error Code is returned
*           DATA_BLOCK  Structure with Data from the IDA File
*
* Calls     freeDataBlock   to free Heap memory if an Error Occurs
*
* Notes:    All memory required to hold data is allocated dynamically
*       in heap storage. Pointers to these areas of memory are held
*       by the passed DATA_BLOCK structure. Local memory allocations
*       are freed on exit. However, the blocks reserved for data are
*       not and MUST BE FREED by the calling routine.
*
* ToDo:
*
*       TRANSP data has coordinate dimensions that are of rank > 1: They are time dependent!
*       Ensure the netCDF3 plugin functionality is enabled.
*
* Change History
*
* 1.0   13Jun2006   D.G.Muir    Original Version
* 1.1   05Jul2006   D.G.Muir    Return meaingful time depenedent dimension values
* 1.2   27Mar2007   D.G.Muir    File Handle Management added
* 1.3   19Apr2007   D.G.Muir    nc_inq_attlen(,,,&attlength) changed to nc_inq_attlen(,,,(size_t *)&attlength)
* 23Oct2007 dgm ERRORSTACK Components added
* 20Apr2009 dgm Modified to use the netCDF4 API
* 07May2010 dgm Set return strings to null before reading attributes
* 16Dec2011 dgm Corrected bug that occurs in 64 bit server: change int to size_t for variable attlength
* 19Mar2012 dgm Generalised string attributes by checking the type: NC_CHAR or NC_STRING
*       removed count check on attributes copied to the output arguments
*-----------------------------------------------------------------------------*/

#include "readCDFAtts.h"

#include <netcdf.h>
#include <string.h>

#include "readCDF4.h"
#include "idamErrorLog.h"
#include "TrimString.h"

// Read Standard Variable Attributes

int readCDF4Atts(int grpid, int varid, char *units, char *title, char *class, char *comment)
{
    int err, i, rc, numatts;
    size_t attlength;
    char attname[MAX_NC_NAME];      // attribute name
    nc_type atttype;                    // attribute type
    char *txt = NULL;
    units[0]   = '\0';
    title[0]   = '\0';
    class[0]   = '\0';
    comment[0] = '\0';

    //---------------------------------------------------------------------------------------------
    // Number of Attributes associated with this variable

    if ((rc = nc_inq_varnatts(grpid, varid, &numatts)) != NC_NOERR) {
        err = NETCDF_ERROR_INQUIRING_ATT_2;
        addIdamError(&idamerrorstack, CODEERRORTYPE, "readCDFAtts", err, (char *)nc_strerror(rc));
        return err;
    }

    for (i=0; i<numatts; i++) {
        if ((rc = nc_inq_attname(grpid, varid, i, attname)) != NC_NOERR) {
            err = NETCDF_ERROR_INQUIRING_ATT_7;
            addIdamError(&idamerrorstack, CODEERRORTYPE, "readCDFAtts", err, (char *)nc_strerror(rc));
            return err;
        }

        if ((rc = nc_inq_atttype(grpid, varid, attname, &atttype)) != NC_NOERR) {
            err = NETCDF_ERROR_INQUIRING_ATT_8;
            addIdamError(&idamerrorstack, CODEERRORTYPE, "readCDFAtts", err, (char *)nc_strerror(rc));
            return err;
        }

        if ((rc = nc_inq_attlen(grpid, varid, attname, (size_t *)&attlength)) != NC_NOERR) {
            err = NETCDF_ERROR_INQUIRING_ATT_9;
            addIdamError(&idamerrorstack, CODEERRORTYPE, "readCDFAtts", err, (char *)nc_strerror(rc));
            return err;
        }

        if (atttype == NC_CHAR) {
            if ((txt=(char *)malloc((size_t)(attlength*sizeof(char)+1))) == NULL) {
                err = NETCDF_ERROR_ALLOCATING_HEAP_9;
                addIdamError(&idamerrorstack, CODEERRORTYPE, "readCDFAtts", err, "Unable to Allocate Heap for Attribute Data");
                return err;
            }

            if ((rc = nc_get_att_text (grpid, varid, attname, txt)) != NC_NOERR) {
                err = NETCDF_ERROR_INQUIRING_ATT_10;
                addIdamError(&idamerrorstack, CODEERRORTYPE, "readCDFAtts", err, (char *)nc_strerror(rc));
                free((void *)txt);
                return err;
            }

            txt[attlength*sizeof(char)] = '\0';     // Add the string Null terminator
            TrimString(txt);
            LeftTrimString(txt);

            if (!strcmp(attname,"units")) {
                copyString(txt, units, STRING_LENGTH);
            } else if (!strcmp(attname,"title") || !strcmp(attname,"label") || !strcmp(attname,"long_name")) {
                copyString(txt, title, STRING_LENGTH);
            } else if (!strcmp(attname,"comment")) {
                copyString(txt, comment, STRING_LENGTH);
            } else if (!strcmp(attname,"class")) {
                copyString(txt, class, STRING_LENGTH);
            }

            free((void *)txt);
            txt = NULL;
        } else if (atttype == NC_STRING) {
            char **sarr = (char **)malloc(attlength*sizeof(char *));

            if ((rc = nc_get_att_string(grpid, varid, attname, sarr)) != NC_NOERR) {
                err = 999;
                addIdamError(&idamerrorstack, CODEERRORTYPE, "readCDF", err, (char *)nc_strerror(rc));
                free((void *) sarr);
                return err;
            }

            if (!strcmp(attname,"units")) {
                copyString(sarr[0], units, STRING_LENGTH);
            } else if (!strcmp(attname,"title") || !strcmp(attname,"label") || !strcmp(attname,"long_name")) {
                copyString(sarr[0], title, STRING_LENGTH);
            } else if (!strcmp(attname,"comment")) {
                copyString(sarr[0], comment, STRING_LENGTH);
            } else if (!strcmp(attname,"class")) {
                copyString(sarr[0], class, STRING_LENGTH);
            }

            nc_free_string(attlength, sarr);
            free((void *) sarr);
        }
    }

    return (0);
}
