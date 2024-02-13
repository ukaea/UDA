#pragma once

#include "udaStructs.h"

namespace uda::client_server
{

int compress_dim(DIMS* ddim);

/*---------------------------------------------------------------
 * UDA Dimensional Data Uncompressor
 *
 * Input Arguments:    DIMS *        Dimensional Data
 *
 * Returns:        uncompressDim    0 if no Problems Found
 *                    Error Code if a Problem Occured
 *
 *            DIMS* ->dim    Un-Compressed Dimensional Data
 *            DIMS* ->compressed    Unchanged (necessary)
 *
 * Note: XML based data correction also uses the compression models: New models
 * must also have corrections applied.
 *
 *--------------------------------------------------------------*/
int uncompress_dim(DIMS* ddim);

} // namespace uda::client_server