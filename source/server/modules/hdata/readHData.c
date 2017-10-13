/*---------------------------------------------------------------
* IDAM Plugin data Reader to Access Hierarchical DATA
*
* Input Arguments:	DATA_SOURCE data_source
*			SIGNAL_DESC signal_desc
*
* Returns:		readHData	0 if read was successful
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
*
*-----------------------------------------------------------------------------*/
#include "readHData.h"

#include <clientserver/errorLog.h>
#include <server/udaServer.h>
#include <clientserver/udaErrors.h>

#ifdef HIERARCHICAL_DATA
#include "idamclientserverxml.h"

int readHData(PGconn *DBConnect, REQUEST_BLOCK request_block, DATA_SOURCE data_source,
              DATA_BLOCK *data_block) {

    int rc, err;

    EFIT *efit = NULL;
    char *xml  = NULL;

    //----------------------------------------------------------------------
    // Identify the Data Structure Requested

    if(strcasecmp(request_block.signal,"EFIT") != 0) {
        err = 998;
        addIdamError(CODEERRORTYPE, "readHData", err, "Requested Hierarchical Data Structure is Not Recognised");
        return err;
    }

    //----------------------------------------------------------------------
    // User specified file or SQL Database record?

    if(data_source.exp_number != 0 && strlen(data_source.path) == 0) {
        if((rc = sqlHData(DBConnect, request_block.device_name, xml)) != 0) {
            err = 998;
            addIdamError(CODEERRORTYPE, "readHData", err, "No Hierarchical EFIT Data Structure XML Record Found");
            return err;
        }
        if(strlen(xml) == 0) {
            err = 998;
            addIdamError(CODEERRORTYPE, "readHData", err, "Hierarchical EFIT Data Structure XML Record is Empty");
            return err;
        }
    }

    //----------------------------------------------------------------------
    // Allocate the Hierarchical Data Structure

    if((efit = (EFIT *)malloc(sizeof(EFIT))) == NULL) {
        err = 998;
        addIdamError(CODEERRORTYPE, "readHData", err, "Problem allocating Heap Memory for Hierarchical EFIT Data Structure");
        free((void *) xml);
        return err;
    }

    initEfit(efit);
    data_block->opaque_type = UDA_OPAQUE_TYPE_UNKNOWN;

    //----------------------------------------------------------------------
    // parse XML

    if((rc = parseHData(data_source.path, xml, efit)) != 0) {
        err = 998;
        addIdamError(CODEERRORTYPE, "readHData", err, "Hierarchical EFIT Data Structure Record could Not be Parsed");
        free((void *) xml);
        free((void *) efit);
        return err;
    }

    //----------------------------------------------------------------------
    // Assign meta data to the Data Block return structure

    data_block->opaque_type  = UDA_OPAQUE_TYPE_EFIT;
    data_block->opaque_block = (void *) efit;

    free((void *) xml);

    return 0;
}

#else

int readHData(PGconn *DBConnect, REQUEST_BLOCK request_block, DATA_SOURCE data_source,
              DATA_BLOCK *data_block) {
    int err = 999;
    addIdamError(CODEERRORTYPE, "readHData", err, "Not Configured to Read Hierarchical Data");
    return err;
}

#endif
