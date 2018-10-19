
#ifndef IDAM_PLUGIN_WEST_SOFT_X_RAYS_H
#define IDAM_PLUGIN_WEST_SOFT_X_RAYS_H

#include <clientserver/udaStructs.h>

int soft_x_rays_idsproperties_comment(int shotNumber, DATA_BLOCK* data_block, int* nodeIndices);

int soft_x_rays_channels_shapeof(int shotNumber, DATA_BLOCK* data_block, int* nodeIndices);

int channel_line_of_sight_first_point_r(int shotNumber, DATA_BLOCK* data_block, int* nodeIndices);

int channel_line_of_sight_first_point_z(int shotNumber, DATA_BLOCK* data_block, int* nodeIndices);

int channel_line_of_sight_first_point_phi(int shotNumber, DATA_BLOCK* data_block, int* nodeIndices);

int channel_line_of_sight_second_point_r(int shotNumber, DATA_BLOCK* data_block, int* nodeIndices);

int channel_line_of_sight_second_point_z(int shotNumber, DATA_BLOCK* data_block, int* nodeIndices);

int channel_line_of_sight_second_point_phi(int shotNumber, DATA_BLOCK* data_block, int* nodeIndices);

int soft_x_rays_channels_energy_band_shapeof(int shotNumber, DATA_BLOCK* data_block, int* nodeIndices);

int soft_x_rays_channels_energy_band_lower_bound(int shotNumber, DATA_BLOCK* data_block, int* nodeIndices);

int soft_x_rays_channels_energy_band_upper_bound(int shotNumber, DATA_BLOCK* data_block, int* nodeIndices);

int soft_x_rays_channels_power_density_data(int shotNumber, DATA_BLOCK* data_block, int* nodeIndices);

int soft_x_rays_channels_power_density_time(int shotNumber, DATA_BLOCK* data_block, int* nodeIndices);

void soft_x_rays_throwsIdamError(int status, char* methodName, char* object_name, int index, int shotNumber);

#endif // IDAM_PLUGIN_WEST_SOFT_X_RAYS_H
