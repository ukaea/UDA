//! $LastChangedRevision: 107 $
//! $LastChangedDate: 2009-10-07 15:18:57 +0100 (Wed, 07 Oct 2009) $
//! $LastChangedBy: dgm $
//! $HeadURL: https://fussvn.fusion.culham.ukaea.org.uk/svnroot/IDAM/development/source/client/ClientMDS.c $

/*---------------------------------------------------------------
* Reads the Requested Data
*
* Input Arguments:	1) MDS+ Server Name
*			3) MDS+ Tree Name   or Generic)
*			4) MDS+ Node Name
*			5) MDS+ Tree Number
*
* Returns:
*
* Revision 0.0  05-Aug-2004	D.G.Muir
*
// 09Jul2009	dgm	Legacy APIs moved to accAPI_CL.c
*--------------------------------------------------------------*/

#include <idamLog.h>
#include "ClientMDS.h"

#include "initStructs.h"
#include "startup.h"

int idamClientMDS(const char* server, const char* tree, const char* node, int treenum)
{
    REQUEST_BLOCK request_block;

//-------------------------------------------------------------------------
// Open the Logs

    if (idamStartup(0) != 0) return PROBLEM_OPENING_LOGS;

//-------------------------------------------------------------------------
// Passed Args

    initRequestBlock(&request_block);

    request_block.request = REQUEST_READ_MDS;
    request_block.exp_number = treenum;

    strcpy(request_block.file, tree);
    strcpy(request_block.signal, node);
    strcpy(request_block.server, server);

    idamLog(LOG_DEBUG, "Routine: ClientMDS\n");
    idamLog(LOG_DEBUG, "Server 		 %s\n", request_block.server);
    idamLog(LOG_DEBUG, "Tree  		 %s\n", request_block.file);
    idamLog(LOG_DEBUG, "Node  		 %s\n", request_block.signal);
    idamLog(LOG_DEBUG, "Tree Number       %d\n", request_block.exp_number);

//-------------------------------------------------------------------------
// Fetch Data

    return (idamClient(&request_block));
}
