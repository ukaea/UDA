#include "west_ece.h"

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

#include "west_ece_mode.h"
#include "west_utilities.h"
#include "west_dyn_data_utilities.h"
#include "ts_rqparam.h"

int modeO1 = 1;
int modeX2 = 2;

int SHOT_30814 = 30814;
int SHOT_31957 = 31957;
int SHOT_28452 = 28452;

int ARCADE_GECEMODE_EXISTS_FROM_SHOT = 50820; //TODO

int getECEModeFromNPZFile(int shotNumber);
int getECEModeHarmonic(int shotNumber, float** time, float** data, int* len);
int get_signal(char* nomsigp, int shotNumber, int extractionIndex, float** time, float** data, int* len);
int isTimeHomogeneous1(int shotNumber);
int isTimeHomogeneous2(int shotNumber);
int isTimeHomogeneous3(int shotNumber);
void ece_t_e_time1(int shotNumber, DATA_BLOCK* data_block, int* nodeIndices);
void ece_t_e_time2(int shotNumber, DATA_BLOCK* data_block, int* nodeIndices);
void ece_t_e_time3(int shotNumber, DATA_BLOCK* data_block, int* nodeIndices);

void homogeneous_time(int shotNumber, DATA_BLOCK* data_block, int* nodeIndices){
	int homogeneous_time;
	if (shotNumber < SHOT_30814) {
		homogeneous_time = isTimeHomogeneous1(shotNumber);
	} else {
		if (shotNumber < SHOT_31957) {
			homogeneous_time = isTimeHomogeneous2(shotNumber);
		} else {
			homogeneous_time = isTimeHomogeneous3(shotNumber);
		}
	}
	if (homogeneous_time == -1) //an error has occurred, we do not set the field
		return;
	setReturnDataIntScalar(data_block, homogeneous_time, NULL);
}

void ece_t_e_data_shape_of(int shotNumber, char** mapfun)
{
	if (shotNumber < SHOT_30814) {
		*mapfun = strdup("shape_of_tsmat_collect;DECE:GSH1:VOIE,DECE:GSH2:VOIE;0:float:#0");
	} else {
		if (shotNumber < SHOT_31957) {
			*mapfun = strdup("shape_of_tsmat_collect;DVECE:GVSH1:VOIE,DVECE:GVSH2:VOIE;0:float:#0");
		} else {
			*mapfun = strdup(
					"shape_of_tsmat_collect;DVECE:GVSH1:VOIE,DVECE:GVSH2:VOIE,DVECE:GVSH3:VOIE,DVECE:GVSH4:VOIE;0:float:#0");
		}
	}
}

void ece_t_e_data(int shotNumber, char** mapfun)
{
	if (shotNumber < SHOT_30814) {
		*mapfun = strdup("tsbase_collect;DECE:GSH1,DECE:GSH2;1:float:#0");
	} else {
		if (shotNumber < SHOT_31957) {
			*mapfun = strdup("tsbase_collect;DVECE:GVSH1,DVECE:GVSH2;1:float:#0");
		} else {
			*mapfun = strdup("tsbase_collect;DVECE:GVSH1,DVECE:GVSH2,DVECE:GVSH3,DVECE:GVSH4;1:float:#0");
		}
	}
}

void ece_t_e_time(int shotNumber, DATA_BLOCK* data_block, int* nodeIndices)
{
	int homogeneous_time;

	if (shotNumber < SHOT_30814) {
		//*mapfun = strdup("tsbase_time;DECE:GSH1,DECE:GSH2;1:float:#0");
		homogeneous_time = isTimeHomogeneous1(shotNumber);
		if (homogeneous_time == -1 || homogeneous_time == 1) {
			return;
		}
		ece_t_e_time1(shotNumber, data_block, nodeIndices);
	} else {
		if (shotNumber < SHOT_31957) {
			//*mapfun = strdup("tsbase_time;DVECE:GVSH1,DVECE:GVSH2;1:float:#0");
			homogeneous_time = isTimeHomogeneous2(shotNumber);
			if (homogeneous_time == -1 || homogeneous_time == 1) {
				return;
			}
			ece_t_e_time2(shotNumber, data_block, nodeIndices);
		} else {
			//*mapfun = strdup("tsbase_time;DVECE:GVSH1,DVECE:GVSH2,DVECE:GVSH3,DVECE:GVSH4;1:float:#0");
			homogeneous_time = isTimeHomogeneous3(shotNumber);
			if (homogeneous_time == -1 || homogeneous_time == 1) {
				return;
			}
			ece_t_e_time3(shotNumber, data_block, nodeIndices);
		}
	}
}

void ece_t_e_time1(int shotNumber, DATA_BLOCK* data_block, int* nodeIndices) {

	int channelID = nodeIndices[0];
	int extractionIndex = channelID;

	int GVSH_extractions_count[2];
	getExtractionsCount("GSH1", shotNumber, 0, &GVSH_extractions_count[0]);
	getExtractionsCount("GSH2", shotNumber, 0, &GVSH_extractions_count[1]);

	char tableName[4];

	if (extractionIndex <= GVSH_extractions_count[0]) {
		strcpy (tableName,"GSH1");
	} else if ((extractionIndex > GVSH_extractions_count[0]) && (extractionIndex <= GVSH_extractions_count[1])) {
		strcpy (tableName,"GSH2");
	}

	int status = setUDABlockTimeFromArcade(tableName, shotNumber, extractionIndex, data_block, nodeIndices);
	if (status != 0) {
		int err = 901;
		char* errorMsg = "WEST:ERROR (west_ece) : unable to get object : ";
		strcat(errorMsg, tableName);
		strcat(errorMsg, " for shot : ");
		char shotStr[6];
		sprintf(shotStr, "%d", shotNumber);
		strcat(errorMsg, shotStr);
		UDA_LOG(UDA_LOG_ERROR, "%s", errorMsg);
		addIdamError(CODEERRORTYPE, errorMsg, err, "");
	}
}

void ece_t_e_time2(int shotNumber, DATA_BLOCK* data_block, int* nodeIndices) {
	int channelID = nodeIndices[0];
	int extractionIndex = channelID;

	int GVSH_extractions_count[2];
	getExtractionsCount("GVSH1", shotNumber, 0, &GVSH_extractions_count[0]);
	getExtractionsCount("GVSH2", shotNumber, 0, &GVSH_extractions_count[1]);

	char tableName[5];

	if (extractionIndex <= GVSH_extractions_count[0]) {
		strcpy (tableName,"GVSH1");
	} else if ((extractionIndex > GVSH_extractions_count[0]) && (extractionIndex <= GVSH_extractions_count[1])) {
		strcpy (tableName,"GVSH2");
	}

	int status = setUDABlockTimeFromArcade(tableName, shotNumber, extractionIndex, data_block, nodeIndices);
	if (status != 0) {
		int err = 901;
		char* errorMsg = "WEST:ERROR (west_ece) : unable to get object : ";
		strcat(errorMsg, tableName);
		strcat(errorMsg, " for shot : ");
		char shotStr[6];
		sprintf(shotStr, "%d", shotNumber);
		strcat(errorMsg, shotStr);
		UDA_LOG(UDA_LOG_ERROR, "%s", errorMsg);
		addIdamError(CODEERRORTYPE, errorMsg, err, "");
	}
}

void ece_t_e_time3(int shotNumber, DATA_BLOCK* data_block, int* nodeIndices) {
	int channelID = nodeIndices[0];
	int extractionIndex = channelID;

	int GVSH_extractions_count[4];
	getExtractionsCount("GVSH1", shotNumber, 0, &GVSH_extractions_count[0]);
	getExtractionsCount("GVSH2", shotNumber, 0, &GVSH_extractions_count[1]);
	getExtractionsCount("GVSH3", shotNumber, 0, &GVSH_extractions_count[2]);
	getExtractionsCount("GVSH4", shotNumber, 0, &GVSH_extractions_count[3]);

	char tableName[5];

	if (extractionIndex <= GVSH_extractions_count[0]) {
		strcpy (tableName,"GVSH1");
	} else if ((extractionIndex > GVSH_extractions_count[0]) && (extractionIndex <= GVSH_extractions_count[1])) {
		strcpy (tableName,"GVSH2");
	} else if ((extractionIndex > GVSH_extractions_count[1]) && (extractionIndex <= GVSH_extractions_count[2])) {
		strcpy (tableName,"GVSH3");
	} else if ((extractionIndex > GVSH_extractions_count[3]) && (extractionIndex <= GVSH_extractions_count[4])) {
		strcpy (tableName,"GVSH4");
	}

	int status = setUDABlockTimeFromArcade(tableName, shotNumber, extractionIndex, data_block, nodeIndices);
	if (status != 0) {
		int err = 901;
		char* errorMsg = "WEST:ERROR (west_ece) : unable to get object : ";
		strcat(errorMsg, tableName);
		strcat(errorMsg, " for shot : ");
		char shotStr[6];
		sprintf(shotStr, "%d", shotNumber);
		strcat(errorMsg, shotStr);
		UDA_LOG(UDA_LOG_ERROR, "%s", errorMsg);
		addIdamError(CODEERRORTYPE, errorMsg, err, "");
	}
}


int isTimeHomogeneous1(int shotNumber) {
	int GSH1_extractions_count = 0;
	int size1 = 0;
	getExtractionsCount("GSH1", shotNumber, 0, &GSH1_extractions_count);
	float *time1 = NULL;
	float *data1 = NULL;
	int status = get_signal("GSH1", shotNumber, 1, &time1, &data1, &size1);

	if (status != 0) {
		return -1; //return if status is different of 0
	}

	int GSH2_extractions_count = 0;
	int size2 = 0;
	getExtractionsCount("GSH2", shotNumber, 0, &GSH2_extractions_count);
	float *time2 = NULL;
	float *data2 = NULL;
	status = get_signal("GSH2", shotNumber, 1, &time2, &data2, &size2);

	if (status != 0 || size1 != size2) {
		return -1; //return if status is different of 0
	}

	if (equals(time1, time2, size1) == 1) {
		return 1; //time is homogeneous
	}
	else {
		return 0; //time is not homogeneous
	}
}

int isTimeHomogeneous2(int shotNumber) {
	int GVSH1_extractions_count = 0;
	int size1 = 0;
	getExtractionsCount("GVSH1", shotNumber, 0, &GVSH1_extractions_count);
	float *time1 = NULL;
	float *data1 = NULL;
	int status = get_signal("GVSH1", shotNumber, 1, &time1, &data1, &size1);

	if (status != 0) {
		return -1; //return if status is different of 0
	}

	int GVSH2_extractions_count = 0;
	int size2 = 0;
	getExtractionsCount("GVSH2", shotNumber, 0, &GVSH2_extractions_count);
	float *time2 = NULL;
	float *data2 = NULL;
	status = get_signal("GVSH2", shotNumber, 1, &time2, &data2, &size2);

	if (status != 0 || size1 != size2) {
		return -1; //return if status is different of 0
	}

	if (equals(time1, time2, size1) == 1) {
		return 1; //time is homogeneous
	}
	else {
		return 0; //time is not homogeneous
	}
}

int isTimeHomogeneous3(int shotNumber) {
	int GVSH1_extractions_count = 0;
	int size1 = 0;
	getExtractionsCount("GVSH1", shotNumber, 0, &GVSH1_extractions_count);
	float *time1 = NULL;
	float *data1 = NULL;
	int status = get_signal("GVSH1", shotNumber, 1, &time1, &data1, &size1);

	if (status != 0) {
		return -1; //return if status is different of 0
	}

	int GVSH2_extractions_count = 0;
	int size2 = 0;
	getExtractionsCount("GVSH2", shotNumber, 0, &GVSH2_extractions_count);
	float *time2 = NULL;
	float *data2 = NULL;
	status = get_signal("GVSH2", shotNumber, 1, &time2, &data2, &size2);

	if (status != 0 || size1 != size2) {
		return -1; //return if status is different of 0
	}

	int GVSH3_extractions_count = 0;
	int size3 = 0;
	getExtractionsCount("GVSH3", shotNumber, 0, &GVSH3_extractions_count);
	float *time3 = NULL;
	float *data3 = NULL;
	status = get_signal("GVSH3", shotNumber, 1, &time3, &data3, &size3);

	if (status != 0 || size2 != size3) {
		return -1; //return if status is different of 0
	}

	int GVSH4_extractions_count = 0;
	int size4 = 0;
	getExtractionsCount("GVSH4", shotNumber, 0, &GVSH4_extractions_count);
	float *time4 = NULL;
	float *data4 = NULL;
	status = get_signal("GVSH4", shotNumber, 1, &time4, &data4, &size4);

	if (status != 0 || size3 != size4) {
		return -1; //return if status is different of 0
	}

	if (equals(time1, time2, size1) && equals(time2, time3, size1) && equals(time3, time4, size1) == 1) {
		return 1; //time is homogeneous
	}
	else {
		return 0; //time is not homogeneous
	}
}

int get_signal(char* nomsigp, int shotNumber, int extractionIndex, float** time, float** data, int* len)
{
	char nomsigp_to_extract[50];
	addExtractionChars(nomsigp_to_extract, nomsigp, extractionIndex); //Concatenate nomsigp_to_extract avec !extractionIndex, example: !1, !2, ...
	int rang[2] = { 0, 0 };
	int status = readSignal(nomsigp_to_extract, shotNumber, 0, rang, time, data, len);
	return status;
}

void ece_time(int shotNumber, DATA_BLOCK* data_block, int* nodeIndices)
{
	int homogeneous_time;
	if (shotNumber < SHOT_30814) {
		homogeneous_time = isTimeHomogeneous1(shotNumber);
		if (homogeneous_time == -1 || homogeneous_time == 0) {
			return;
		}
		//*mapfun = strdup("tsbase_time;DECE:GSH1,DECE:GSH2;1:float:#0");
		//Time is homogeneous, we take for example GSH1%1
		int status = setUDABlockTimeFromArcade("GSH1", shotNumber, 1, data_block, nodeIndices);
		if (status != 0) {
			int err = 901;
			char* errorMsg = "WEST:ERROR (west_ece) : unable to get object : ";
			strcat(errorMsg, "GSH1");
			strcat(errorMsg, " for shot : ");
			char shotStr[6];
			sprintf(shotStr, "%d", shotNumber);
			strcat(errorMsg, shotStr);
			UDA_LOG(UDA_LOG_ERROR, "%s", errorMsg);
			addIdamError(CODEERRORTYPE, errorMsg, err, "");
		}
	} else {
		if (shotNumber < SHOT_31957) {
			homogeneous_time = isTimeHomogeneous2(shotNumber);
			//*mapfun = strdup("tsbase_time;DVECE:GVSH1,DVECE:GVSH2;1:float:#0");
		} else {
			homogeneous_time = isTimeHomogeneous3(shotNumber);
			//*mapfun = strdup("tsbase_time;DVECE:GVSH1,DVECE:GVSH2,DVECE:GVSH3,DVECE:GVSH4;1:float:#0");
		}
		if (homogeneous_time == -1 || homogeneous_time == 0) {
			return;
		}
		//Time is homogeneous, we take for example GVSH1%1
		int status = setUDABlockTimeFromArcade("GVSH1", shotNumber, 1, data_block, nodeIndices);
		if (status != 0) {
			int err = 901;
			char* errorMsg = "WEST:ERROR (west_ece) : unable to get object : ";
			strcat(errorMsg, "GVSH1");
			strcat(errorMsg, " for shot : ");
			char shotStr[6];
			sprintf(shotStr, "%d", shotNumber);
			strcat(errorMsg, shotStr);
			UDA_LOG(UDA_LOG_ERROR, "%s", errorMsg);
			addIdamError(CODEERRORTYPE, errorMsg, err, "");
		}

	}

}

int ece_frequencies(int shotNumber, DATA_BLOCK* data_block, int* nodeIndices)
{
	//Mode 01
	float GSH1_01[8]    = { 92.5, 90.5, 88.5, 86.5, 84.5, 82.5, 80.5, 78.5 };
	float GSH2_01[8]    = { 109.5, 107.5, 105.5, 103.5, 101.5, 99.5, 97.5, 95.5 };
	float GVSH1_01[8]   = { 93.5, 92.5, 91.5, 90.5, 89.5, 88.5, 87.5, 86.5 };
	float GVSH2_01[8]   = { 85.5, 84.5, 83.5, 82.5, 81.5, 80.5, 79.5, 78.5 };
	float GVSH3_01[8]   = { 109.5, 108.5, 107.5, 106.5, 105.5, 104.5, 103.5, 102.5 };
	float GVSH4_01[8]   = { 101.5, 100.5, 99.5, 98.5, 97.5, 96.5, 95.5, 94.5 };

	//Mode X2
	float GSH1_X2_before28452[8]    = { 92.5, 90.5, 88.5, 86.5, 84.5, 82.5, 80.5, 78.5 };
	float GSH2_X2_before28452[8]    = { 109.5, 107.5, 105.5, 103.5, 101.5, 99.5, 97.5, 95.5 };
	float GSH1_X2_after28452[8]     = { 125, 123, 121, 119, 117, 115, 113, 111 };
	float GSH2_X2_after28452[8]     = { 109.5, 107.5, 105.5, 103.5, 101.5, 99.5, 97.5, 95.5 };
	float GVSH1_X2[8]               = { 126, 125, 124, 123, 122, 121, 120, 119 };
	float GVSH2_X2[8]               = { 118, 117, 116, 115, 114, 113, 112, 111 };
	float GVSH3_X2[8]               = { 109.5, 108.5, 107.5, 106.5, 105.5, 104.5, 103.5, 102.5 };
	float GVSH4_X2[8]               = { 101.5, 100.5, 99.5, 98.5, 97.5, 96.5, 95.5, 94.5 };

	float* frequencies_data = NULL;
	float* frequencies_time = NULL;
	char* TOP_collections_parameters = malloc(255);

	int channel = nodeIndices[0]; //starts from 1

	if (shotNumber >= ARCADE_GECEMODE_EXISTS_FROM_SHOT) {

		float* data = NULL;
		int len;
		//strcpy(TOP_collections_parameters, "DVECE:GVSH1,DVECE:GVSH2,DVECE:GVSH3,DVECE:GVSH4");
		UDA_LOG(UDA_LOG_DEBUG, "calling getECEModeHarmonic\n");
		int status = getECEModeHarmonic(shotNumber, &frequencies_time, &data, &len);

		UDA_LOG(UDA_LOG_DEBUG, "after calling getECEModeHarmonic2\n");

		if (status != 0) {
			int err = 901;
			char* errorMsg = "WEST:ERROR (ece_frequencies): error calling getECEModeHarmonic() for shot : ";
			char shotStr[6];
			sprintf(shotStr, "%d", shotNumber);
			strcat(errorMsg, shotStr);
			UDA_LOG(UDA_LOG_ERROR, "%s", errorMsg);
			addIdamError(CODEERRORTYPE, errorMsg, err, "");
			free(data);
			free(frequencies_time);
			return status;
		}

		frequencies_data = malloc(sizeof(float) * len);

		int i;

		for (i = 0; i < len; i++) {

			int index = nodeIndices[0];

			if (data[i] == 0) {

				if (index <= 7) {
					frequencies_data[i] = GVSH1_01[i];
				} else if (index > 7 && index <= 15) {
					frequencies_data[i] = GVSH2_01[i];
				} else if (index > 15 && index <= 23) {
					frequencies_data[i] = GVSH3_01[i];
				} else if (index > 23 && index <= 31) {
					frequencies_data[i] = GVSH4_01[i];
				}
			} else if (data[i] == 1) {

				if (index <= 7) {
					frequencies_data[i] = GVSH1_X2[i];
				} else if (index > 7 && index <= 15) {
					frequencies_data[i] = GVSH2_X2[i];
				} else if (index > 15 && index <= 23) {
					frequencies_data[i] = GVSH3_X2[i];
				} else if (index > 23 && index <= 31) {
					frequencies_data[i] = GVSH4_X2[i];
				}
			} else {
				int err = 901;
				char* errorMsg = "WEST:ERROR (ece_frequencies): unexpected ECE mode for shot : ";
				char shotStr[6];
				sprintf(shotStr, "%d", shotNumber);
				strcat(errorMsg, shotStr);
				UDA_LOG(UDA_LOG_ERROR, "%s", errorMsg);
				addIdamError(CODEERRORTYPE, errorMsg, err, "");
				free(data);
				free(TOP_collections_parameters);
				return -1;
			}

		}

		SetDynamicData(data_block, len, frequencies_time, frequencies_data);
		free(data);
		return 0;
	} else {

		//Get the ECE acquisition mode from NPZ file
		int mode = getECEModeFromNPZFile(shotNumber);

		int i;
		float* GSH = NULL;
		int CHANNELS_COUNT;

		if (mode == modeO1) {

			if (shotNumber < SHOT_30814) {

				CHANNELS_COUNT = 16;

				GSH = malloc(CHANNELS_COUNT * sizeof(int));

				for (i = 0; i < CHANNELS_COUNT; i++) {
					if (i <= 7) {
						GSH[i] = GSH1_01[i];
					} else {
						GSH[i] = GSH2_01[i - 8];
					}
				}
				strcpy(TOP_collections_parameters, "DECE:GSH1,DECE:GSH2");
			} else {

				if (shotNumber < SHOT_31957) {
					CHANNELS_COUNT = 16;
				} else {
					CHANNELS_COUNT = 32;
				}

				GSH = malloc(CHANNELS_COUNT * sizeof(int));

				for (i = 0; i < CHANNELS_COUNT; i++) {
					if (i <= 7) {
						GSH[i] = GVSH1_01[i];
					} else if (i > 7 && i < 16) {
						GSH[i] = GVSH2_01[i - 8];
					} else if (i >= 16 && i < 24) {
						GSH[i] = GVSH3_01[i - 16];
					} else if (i >= 24 && i < 32) {
						GSH[i] = GVSH4_01[i - 24];
					}
				}
				strcpy(TOP_collections_parameters, "DVECE:GVSH1,DVECE:GVSH2,DVECE:GVSH3,DVECE:GVSH4");
			}
		} else if (mode == modeX2) {
			if (shotNumber < SHOT_30814) {

				CHANNELS_COUNT = 16;

				GSH = malloc(CHANNELS_COUNT * sizeof(int));

				for (i = 0; i < CHANNELS_COUNT; i++) {
					if (i <= 7) {
						if (shotNumber < SHOT_28452) {
							GSH[i] = GSH1_X2_before28452[i];
						} else {
							GSH[i] = GSH1_X2_after28452[i];
						}
					} else {
						if (shotNumber < SHOT_28452) {
							GSH[i] = GSH2_X2_before28452[i - 8];
						} else {
							GSH[i] = GSH2_X2_after28452[i - 8];
						}
					}
				}
				strcpy(TOP_collections_parameters,"DECE:GSH1,DECE:GSH2");
			} else {

				if (shotNumber < SHOT_31957) {
					CHANNELS_COUNT = 16;
				} else {
					CHANNELS_COUNT = 32;
				}

				GSH = malloc(CHANNELS_COUNT * sizeof(int));

				for (i = 0; i < CHANNELS_COUNT; i++) {
					if (i <= 7) {
						GSH[i] = GVSH1_X2[i];
					} else if (i > 7 && i < 16) {
						GSH[i] = GVSH2_X2[i - 8];
					} else if (i >= 16 && i < 24) {
						GSH[i] = GVSH3_X2[i - 16];
					} else if (i >= 24 && i < 32) {
						GSH[i] = GVSH4_X2[i - 24];
					}
				}
				strcpy(TOP_collections_parameters, "DVECE:GVSH1,DVECE:GVSH2,DVECE:GVSH3,DVECE:GVSH4");
			}
		} else {
			free(TOP_collections_parameters);
			return -1; //ERROR
		}

		int len;
		UDA_LOG(UDA_LOG_DEBUG, "Calling getECEModeHarmonic\n");
		float* data = NULL;
		//get time only
		int status = getECEModeHarmonic(shotNumber, &frequencies_time, &data, &len);

		if (status != 0) {
			int err = 901;
			char* errorMsg = "WEST:ERROR (ece_frequencies): error calling getECEModeHarmonic() for shot : ";
			char shotStr[6];
			sprintf(shotStr, "%d", shotNumber);
			strcat(errorMsg, shotStr);
			UDA_LOG(UDA_LOG_ERROR, "%s", errorMsg);
			addIdamError(CODEERRORTYPE, errorMsg, err, "");
			free(frequencies_time);
			free(data);
			return status;
		}
		UDA_LOG(UDA_LOG_DEBUG, "After calling getECEModeHarmonicTime\n");
		frequencies_data = malloc(sizeof(float) * len);
		for (i = 0; i < len; i++)
			frequencies_data[i] = GSH[channel - 1] * 1e9; //result converted in Hertz
		UDA_LOG(UDA_LOG_DEBUG, "setting dynamic data\n");
		SetDynamicData(data_block, len, frequencies_time, frequencies_data);
		UDA_LOG(UDA_LOG_DEBUG, "freeing GSH\n");
		free(GSH);
		UDA_LOG(UDA_LOG_DEBUG, "end of function\n");
		return 0;
	}
	return -1;
}

int ece_harmonic_time(int shotNumber, DATA_BLOCK* data_block, int* nodeIndices)
{
	float* data = NULL;
	float* time = NULL;
	int len;
	UDA_LOG(UDA_LOG_DEBUG, "Calling getECEModeHarmonic() \n");
	int status = getECEModeHarmonic(shotNumber, &time, &data, &len);
	if (status != 0) {
		free(time);
		free(data);
		return status;
	}
	UDA_LOG(UDA_LOG_DEBUG, "ECE harmonic mode time array length: %d\n", len);
	SetDynamicDataTime(data_block, len, time, data);
	UDA_LOG(UDA_LOG_DEBUG, "reaching end of function of ece_harmonic_time()\n");
	return 0;
}





int getECEModeFromNPZFile(int shotNumber)
{

	if (shotNumber >= ARCADE_GECEMODE_EXISTS_FROM_SHOT) {
		int err = 901;
		char* errorMsg = "WEST:ERROR (getECEModeFromNPZFile): getECEModeFromNPZFile() should not be called for shot : ";
		char shotStr[6];
		sprintf(shotStr, "%d", shotNumber);
		strcat(errorMsg, shotStr);
		UDA_LOG(UDA_LOG_ERROR, "%s", errorMsg);
		addIdamError(CODEERRORTYPE, errorMsg, err, "");
		return -1;
	}

	struct Node* head = NULL;

	int ECE_mode;

	int shotNumberInFile;

	FILE* pFile;
	char content[15];

	char* ece_modes_file = getenv("WEST_ECE_MODES_FILE");

	pFile = fopen(ece_modes_file, "r");

	if (pFile == NULL) {
		int err = 901;
		char* errorMsg = "WEST:ERROR (getECEModeFromNPZFile): unable to read ECE mode file for shot : ";
		char shotStr[6];
		sprintf(shotStr, "%d", shotNumber);
		strcat(errorMsg, shotStr);
		UDA_LOG(UDA_LOG_ERROR, "%s", errorMsg);
		addIdamError(CODEERRORTYPE, errorMsg, err, "");
		return -1;
	} else {

		while (!feof(pFile)) {
			if (fgets(content, sizeof(content), pFile) != NULL) {
				const char delim[] = ":";
				shotNumberInFile = atoi(strtok(content, delim)); //the shot number
				ECE_mode = atoi(strtok(NULL, delim)); //the ECE mode
				push(&head, shotNumberInFile, ECE_mode);
			}

		}
		fclose(pFile);
	}

	struct Node* s = search(head, shotNumber);
	int searchedMode = -1;

	if (s == NULL) {
		int err = 901;
		char* errorMsg = "WEST:ERROR (getECEModeFromNPZFile): unable to found ECE mode for shot : ";
		char shotStr[6];
		sprintf(shotStr, "%d", shotNumber);
		strcat(errorMsg, shotStr);
		UDA_LOG(UDA_LOG_ERROR, "%s", errorMsg);
		addIdamError(CODEERRORTYPE, errorMsg, err, "");
	} else {
		searchedMode = s->ECE_mode;
	}

	free(head);
	free(s);

	return searchedMode;
}

int getECEModeHarmonic(int shotNumber, float** time, float** data, int* len)
{
	int rang[2] = { 0, 0 };
	char* objectName = "GECEMODE";

	int status = readSignal(objectName, shotNumber, 0, rang, time, data, len);

	if (status != 0) {
		int err = 901;
		UDA_LOG(UDA_LOG_ERROR, "WEST:ERROR (getECEModeHarmonic): unable to get ECE mode\n");
		char* errorMsg = "WEST:ERROR (getECEModeHarmonic): unable to get ECE mode for shot : ";
		char shotStr[6];
		sprintf(shotStr, "%d", shotNumber);
		strcat(errorMsg, shotStr);
		UDA_LOG(UDA_LOG_ERROR, "%s", errorMsg);
		addIdamError(CODEERRORTYPE, errorMsg, err, "");
		return status;
	}

	UDA_LOG(UDA_LOG_DEBUG, "returning from getECEModeHarmonic()...\n");
	return 0;
}
