
#ifndef IDAM_COMPRESSDIM_H
#define IDAM_COMPRESSDIM_H

#include "idamStructs.h"

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
* Revision 1.0  06Jul2005	D.G.Muir
* 13May2011 dgm		return if zero length dimension
*--------------------------------------------------------------*/
int uncompressDim(DIMS * ddim);

#endif // IDAM_COMPRESSDIM_H
