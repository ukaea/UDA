#include "west_soft_x_rays.h"

#include <string.h>
#include <stdio.h>
#include <assert.h>

#include <logging/logging.h>
#include <clientserver/initStructs.h>
#include <clientserver/errorLog.h>
#include <clientserver/stringUtils.h>
#include <clientserver/udaTypes.h>
#include <plugins/udaPlugin.h>
#include <structures/struct.h>

#include "west_utilities.h"
#include "west_dyn_data_utilities.h"
#include "ts_rqparam.h"
#include "west_static_data_utilities.h"

const int CHANNELS_COUNT = 45;
const int GTXMH1_CHANNELS_COUNT = 28;

float second_point_z[45] =  {-654.80,-623.10,-591.70,-560.50,-529.60,-498.90,-468.50,
		-438.30,-408.20,-378.30,-348.60,-319.10,-289.60,-260.30,-231.10,-202.00,-173.00,
		-144.10,-115.20,-86.30,-57.50,-28.80, 0.00,28.80,57.50,86.30,115.20,144.10,173.00,
		202.00,231.10,260.30,289.60,319.10,348.60,378.30,408.20,438.30,468.50,498.90,529.60,
		560.50,591.70,623.10,654.80};

int channels_power_density(int shotNumber, char* nomsigp, int extractionIndex, float** time, float** data, int* len);

void soft_x_rays_throwsIdamError(int status, char* methodName, char* object_name, int index, int shotNumber) {
	int err = 901;
	char msg[1000];
	sprintf(msg, "%s(%s),object:%s,index:%d,shot:%d,err:%d\n", "WEST:ERROR", methodName, object_name, index, shotNumber, status);
	//UDA_LOG(UDA_LOG_ERROR, "%s", msg);
	addIdamError(CODEERRORTYPE, msg, err, "");
}

int soft_x_rays_idsproperties_comment(int shotNumber, DATA_BLOCK* data_block, int* nodeIndices)
{
	const char* comment = "WEST soft-X-rays diagnostic (September 2017), experimental data. Horizontal camera only, with old (Tore Supra) detectors.";
	setReturnDataString(data_block, comment, NULL);
	return 0;
}

int soft_x_rays_channels_shapeof(int shotNumber, DATA_BLOCK* data_block, int* nodeIndices)
{
	setReturnDataIntScalar(data_block, CHANNELS_COUNT, NULL);
	return 0;
}

int channel_line_of_sight_first_point_r(int shotNumber, DATA_BLOCK* data_block, int* nodeIndices)
{
	setReturnDataFloatScalar(data_block, 4.3458, NULL);
	return 0;
}

int channel_line_of_sight_first_point_z(int shotNumber, DATA_BLOCK* data_block, int* nodeIndices)
{
	setReturnDataFloatScalar(data_block, 0., NULL);
	return 0;
}

int channel_line_of_sight_first_point_phi(int shotNumber, DATA_BLOCK* data_block, int* nodeIndices)
{
	setReturnDataFloatScalar(data_block, 1.396, NULL);
	return 0;
}

int channel_line_of_sight_second_point_r(int shotNumber, DATA_BLOCK* data_block, int* nodeIndices)
{
	setReturnDataFloatScalar(data_block, 2.4, NULL);
	return 0;
}

int channel_line_of_sight_second_point_z(int shotNumber, DATA_BLOCK* data_block, int* nodeIndices)
{
	int index = nodeIndices[0] - 1; //starts from 0
	setReturnDataFloatScalar(data_block, second_point_z[index]*1e-3, NULL);
	return 0;
}

int channel_line_of_sight_second_point_phi(int shotNumber, DATA_BLOCK* data_block, int* nodeIndices)
{
	setReturnDataFloatScalar(data_block, 1.396, NULL);
	return 0;
}

int soft_x_rays_channels_energy_band_shapeof(int shotNumber, DATA_BLOCK* data_block, int* nodeIndices)
{
	setReturnDataIntScalar(data_block, 1, NULL);
	return 0;
}

int soft_x_rays_channels_energy_band_lower_bound(int shotNumber, DATA_BLOCK* data_block, int* nodeIndices)
{
	setReturnDataFloatScalar(data_block, 1500, NULL);
	return 0;
}

int soft_x_rays_channels_energy_band_upper_bound(int shotNumber, DATA_BLOCK* data_block, int* nodeIndices)
{
	setReturnDataFloatScalar(data_block, 15000, NULL);
	return 0;
}

int soft_x_rays_channels_power_density_data(int shotNumber, DATA_BLOCK* data_block, int* nodeIndices)
{
	int index = nodeIndices[0]; //starts from 1

	int len;
	float *time = NULL;
	float *data = NULL;
	char* nomsigp = NULL;

	int extractionIndex;

	if (index <= GTXMH1_CHANNELS_COUNT) {
		nomsigp = strdup("GTXMH1");
		extractionIndex = index;
	}
	else {
		nomsigp = strdup("GTXMH2");
		extractionIndex = CHANNELS_COUNT + 1 - index;
	}
	UDA_LOG(UDA_LOG_DEBUG, "reading channels_power_density...\n");
	int status = channels_power_density(shotNumber, nomsigp, extractionIndex, &time, &data, &len);

	if (status != 0) {
		UDA_LOG(UDA_LOG_DEBUG, "reading channels_power_density, error status...\n");
		soft_x_rays_throwsIdamError(status, "soft_x_rays_channels_power_density_data", nomsigp, extractionIndex, shotNumber);
		free(time);
		free(data);
		return status;
	}
	else {
		UDA_LOG(UDA_LOG_DEBUG, "setting channels_power_density...\n");
		setReturnData2DFloat(data_block, 1, len, data);
		return 0;
	}
}

int soft_x_rays_channels_power_density_time(int shotNumber, DATA_BLOCK* data_block, int* nodeIndices)
{
	int index = nodeIndices[0]; //starts from 1

	int len;
	float *time = NULL;
	float *data = NULL;
	char* nomsigp = NULL;

	int extractionIndex;

	if (index <= GTXMH1_CHANNELS_COUNT) {
		nomsigp = strdup("GTXMH1");
		extractionIndex = index;
	}
	else {
		nomsigp = strdup("GTXMH2");
		extractionIndex = CHANNELS_COUNT + 1 - index;
	}

	int status = channels_power_density(shotNumber, nomsigp, extractionIndex, &time, &data, &len);

	if (status != 0) {
		soft_x_rays_throwsIdamError(status, "soft_x_rays_channels_power_density_time", nomsigp, extractionIndex, shotNumber);
		free(time);
		free(data);
		return status;
	}
	else {
		setReturnData2DFloat(data_block, 1, len, time);
		return status;
	}
}

int channels_power_density(int shotNumber, char* nomsigp, int extractionIndex, float** time, float** data, int* len)
{
	char nomsigp_to_extract[50];
	addExtractionChars(nomsigp_to_extract, nomsigp, extractionIndex); //Concatenate nomsigp_to_extract avec !extractionIndex, example: !1, !2, ...
	int rang[2] = { 0, 0 };
	int status = readSignal(nomsigp_to_extract, shotNumber, 0, rang, time, data, len);
	UDA_LOG(UDA_LOG_DEBUG, "end of reading channels_power_density signal...\n");
	return status;
}
