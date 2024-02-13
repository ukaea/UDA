#pragma once

#include "udaStructs.h"

namespace uda::client_server
{

int compress_dim(Dims* ddim);

/*---------------------------------------------------------------
 * UDA Dimensional Data Uncompressor
 *
 * Input Arguments:    Dims *        Dimensional Data
 *
 * Returns:        uncompressDim    0 if no Problems Found
 *                    Error Code if a Problem Occured
 *
 *            Dims* ->dim    Un-Compressed Dimensional Data
 *            Dims* ->compressed    Unchanged (necessary)
 *
 * Note: XML based data correction also uses the compression models: New models
 * must also have corrections applied.
 *
 *--------------------------------------------------------------*/
int uncompress_dim(Dims* ddim);

} // namespace uda::client_server