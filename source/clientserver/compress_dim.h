#pragma once

#include "uda_structs.h"

namespace uda::client_server
{

int compress_dim(Dimension* ddim);

/*---------------------------------------------------------------
 * UDA Dimensional Data Uncompressor
 *
 * Input Arguments:    Dimension *        Dimensional Data
 *
 * Returns:        uncompressDim    0 if no Problems Found
 *                    Error Code if a Problem Occured
 *
 *            Dimension* ->dim    Un-Compressed Dimensional Data
 *            Dimension* ->compressed    Unchanged (necessary)
 *
 * Note: XML based data correction also uses the compression models: New models
 * must also have corrections applied.
 *
 *--------------------------------------------------------------*/
int uncompress_dim(Dimension* ddim);

} // namespace uda::client_server