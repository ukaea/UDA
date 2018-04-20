#ifndef IDAM_CLIENTSERVER_COMPRESSDIM_H
#define IDAM_CLIENTSERVER_COMPRESSDIM_H

#include "udaStructs.h"

#ifdef __cplusplus
extern "C" {
#endif

int compressDim(DIMS * ddim);

/*---------------------------------------------------------------
* IDAM Dimensional Data Uncompressor
*
* Input Arguments:	DIMS *		Dimensional Data
*
* Returns:		uncompressDim	0 if no Problems Found
*					Error Code if a Problem Occured
*
*			DIMS* ->dim	Un-Compressed Dimensional Data
*			DIMS* ->compressed	Unchanged (necessary)
*
* Note: XML based data correction also uses the compression models: New models
* must also have corrections applied.
*
*--------------------------------------------------------------*/
int uncompressDim(DIMS * ddim);

#ifdef __cplusplus
}
#endif

#endif // IDAM_CLIENTSERVER_COMPRESSDIM_H
