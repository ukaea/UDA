#ifndef UDA_CLIENTSERVER_COMPRESSDIM_H
#define UDA_CLIENTSERVER_COMPRESSDIM_H

#include "udaStructs.h"

#if defined(_WIN32)
#  define LIBRARY_API __declspec(dllexport)
#else
#  define LIBRARY_API
#endif

#ifdef __cplusplus
extern "C" {
#endif

LIBRARY_API int compressDim(DIMS * ddim);

/*---------------------------------------------------------------
* UDA Dimensional Data Uncompressor
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
LIBRARY_API int uncompressDim(DIMS * ddim);

#ifdef __cplusplus
}
#endif

#endif // UDA_CLIENTSERVER_COMPRESSDIM_H
