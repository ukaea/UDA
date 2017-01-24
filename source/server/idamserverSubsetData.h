//! $LastChangedRevision: 353 $
//! $LastChangedDate: 2013-11-18 15:32:28 +0000 (Mon, 18 Nov 2013) $
//! $LastChangedBy: dgm $
//! $HeadURL: https://fussvn.fusion.culham.ukaea.org.uk/svnroot/IDAM/development/source/server/idamserverSubsetData.c $

//--------------------------------------------------------------------------------------------------------------------
// Serverside Data Subsetting Operations Data
//
// Return Codes:	0 => OK, otherwise Error
//
// Change History:
// 28Mar2008 dgm	Original Version
// 22Sep2008 dgm	Unsigned Types added
// 07Jul2010 dgm	Added test for error data type. If unknown then don't subset - data allocated for error models only.
// 03Nov2010 dgm	Corrected bug in idamserverParseServerSide relating to #
//			Corrected bug relating to reversing data
//--------------------------------------------------------------------------------------------------------------------

#ifndef IDAM_IDAMSERVERSUBSETDATA_H
#define IDAM_IDAMSERVERSUBSETDATA_H

#include <clientserver/parseXML.h>
#include <clientserver/idamStructs.h>

int idamserverSubsetData(DATA_BLOCK *data_block, ACTION action);
int idamserversubsetindices(char *operation, DIMS *dim, double value, unsigned int *subsetindices);
int idamserverParseServerSide(REQUEST_BLOCK *request_block, ACTIONS *actions_serverside);
int idamserverNewDataArray2(DIMS *dims, int rank, int dimid,
                            char *data, int ndata, int data_type, int notoperation, int reverse,
                            int start, int end, int start1, int end1, int *n, void **newdata);

#endif // IDAM_IDAMSERVERSUBSETDATA_H
