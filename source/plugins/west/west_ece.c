#include "west_ece.h"

#include <string.h>
#include <stdio.h>
#include <assert.h>

#include <logging/logging.h>
#include <clientserver/initStructs.h>
#include <clientserver/errorLog.h>
#include <clientserver/stringUtils.h>
#include <clientserver/udaTypes.h>

#include "west_ece_mode.h"

int modeO1 = 1;
int modeX2 = 2;

int SHOT_30814 = 30814;
int SHOT_31957 = 31957;
int SHOT_28452 = 28452;

int getECEMode();

int ece_frequencies(int shotNumber, DATA_BLOCK* data_block, int* nodeIndices) {


	//Get the ECE acquisition mode
	int mode = getECEMode(shotNumber);

	//Mode 01
	float GSH1_01[8]  = {92.5,90.5,88.5,86.5,84.5,82.5,80.5,78.5};
	float GSH2_01[8]  = {109.5,107.5,105.5,103.5,101.5,99.5,97.5,95.5};
	float GVSH1_01[8] = {93.5,92.5,91.5,90.5,89.5,88.5,87.5,86.5};
	float GVSH2_01[8] = {85.5,84.5,83.5,82.5,81.5,80.5,79.5,78.5};
	float GVSH3_01[8] = {109.5,108.5,107.5,106.5,105.5,104.5,103.5,102.5};
	float GVSH4_01[8] = {101.5,100.5,99.5,98.5,97.5,96.5,95.5,94.5};

	//Mode X2
	float GSH1_X2_before28452[8] = {92.5,90.5,88.5,86.5,84.5,82.5,80.5,78.5};
	float GSH2_X2_before28452[8] = {109.5,107.5,105.5,103.5,101.5,99.5,97.5,95.5};
	float GSH1_X2_after28452[8]  = {125,123,121,119,117,115,113,111};
	float GSH2_X2_after28452[8]  = {109.5,107.5,105.5,103.5,101.5,99.5,97.5,95.5};
	float GVSH1_X2[8]            = {126,125,124,123,122,121,120,119};
	float GVSH2_X2[8]            = {118,117,116,115,114,113,112,111};
	float GVSH3_X2[8]            = {109.5,108.5,107.5,106.5,105.5,104.5,103.5,102.5};
	float GVSH4_X2[8]            = {101.5,100.5,99.5,98.5,97.5,96.5,95.5,94.5};

	int i;
	float *GSH;
	int CHANNELS_COUNT;

	if (mode == modeO1) {

		if (shotNumber <= SHOT_30814) {

			CHANNELS_COUNT = 16;

			GSH = malloc(CHANNELS_COUNT * sizeof(int));

			for (i = 0; i < CHANNELS_COUNT; i++ ) {
				if (i <= 7){
					GSH[i] = GSH1_01[i];
				}
				else {
					GSH[i] = GSH2_01[i-8];
				}
			}
		}
		else {

			if (shotNumber <= SHOT_31957) {
				CHANNELS_COUNT = 16;
			}
			else {
				CHANNELS_COUNT = 32;
			}

			GSH = malloc(CHANNELS_COUNT * sizeof(int));

			for (i = 0; i < CHANNELS_COUNT; i++ ) {
				if (i <= 7){
					GSH[i] = GVSH1_01[i];
				}
				else if (i > 7 && i < 16) {
					GSH[i] = GVSH2_01[i-8];
				}
				else if (i >= 16 && i < 24) {
					GSH[i] = GVSH3_01[i-16];
				}
				else if (i >= 24 && i < 32) {
					GSH[i] = GVSH4_01[i-24];
				}
			}

		}
	}
	else if (mode == modeX2) {
		if (shotNumber <= SHOT_30814) {

			CHANNELS_COUNT = 16;

			GSH = malloc(CHANNELS_COUNT * sizeof(int));

			for (i = 0; i < CHANNELS_COUNT; i++ ) {
				if (i <= 7){
					if (shotNumber <= SHOT_28452) {
						GSH[i] = GSH1_X2_before28452[i];
					}
					else {
						GSH[i] = GSH1_X2_after28452[i];
					}
				}
				else {
					if (shotNumber <= SHOT_28452) {
						GSH[i] = GSH2_X2_before28452[i-8];
					}
					else {
						GSH[i] = GSH2_X2_after28452[i-8];
					}
				}
			}
		}
		else {

			if (shotNumber <= SHOT_31957) {
				CHANNELS_COUNT = 16;
			}
			else {
				CHANNELS_COUNT = 32;
			}

			GSH = malloc(CHANNELS_COUNT * sizeof(int));

			for (i = 0; i < CHANNELS_COUNT; i++ ) {
				if (i <= 7){
					GSH[i] = GVSH1_X2[i];
				}
				else if (i > 7 && i < 16) {
					GSH[i] = GVSH2_X2[i-8];
				}
				else if (i >= 16 && i < 24) {
					GSH[i] = GVSH3_X2[i-16];
				}
				else if (i >= 24 && i < 32) {
					GSH[i] = GVSH4_X2[i-24];
				}
			}
		}
	}
	else {
		return -1; //ERROR
	}

	int idamIndex = nodeIndices[0] - 1;
	data_block->data_type = TYPE_FLOAT;
	data_block->data = malloc(1 * sizeof(float));
	((float*)data_block->data)[0] = GSH[idamIndex]*1e9; //result converted in Hertz

	return 0;
}

int ece_names(int shotNumber, DATA_BLOCK* data_block, int* nodeIndices) {

	IDAM_LOG(LOG_DEBUG, "Calling ece_names\n");


	//Get the ECE acquisition mode
	int mode = getECEMode(shotNumber);
	int channelNumber = nodeIndices[0];

	char* name = malloc(5*sizeof(char));

	char channels_count[5];

	if (channelNumber <= 9) {
		sprintf(channels_count, "0%d", channelNumber);
	}
	else {
		sprintf(channels_count, "%d", channelNumber);
	}

	if (mode == modeO1) {
		sprintf(name, "%s", "O1_");
		name = strcat(name, channels_count);
	}
	else if (mode == modeX2) {
		sprintf(name, "%s", "X2_");
		name = strcat(name, channels_count);
	}
	else {
		return -1; //ERROR
	}
	data_block->data_type = TYPE_STRING;
	data_block->data = strdup(name);

	return 0;
}

int ece_identifiers(int shotNumber, DATA_BLOCK* data_block, int* nodeIndices) {

	IDAM_LOG(LOG_DEBUG, "Calling ece_identifiers\n");
	int channelNumber = nodeIndices[0];
	char* identifier = malloc(sizeof(char));
	sprintf(identifier, "%d", channelNumber);
	data_block->data_type = TYPE_STRING;
	data_block->data = strdup(identifier);
	return 0;
}

void ece_t_e_data_shape_of(int shotNumber, char** mapfun)
{
	if (shotNumber <= SHOT_30814) {
		*mapfun = strdup("shape_of_tsmat_collect;DECE:GSH1:CALIB,DECE:GSH2:CALIB;0:float:#0");
	}
	else {
		if (shotNumber <= SHOT_31957) {
			*mapfun = strdup("shape_of_tsmat_collect;DVECE:GVSH1:CALIB,DVECE:GVSH2:CALIB;0:float:#0");
		}
		else {
			*mapfun = strdup("shape_of_tsmat_collect;DVECE:GVSH1:CALIB,DVECE:GVSH2:CALIB,DVECE:GVSH3:CALIB,DVECE:GVSH4:CALIB;0:float:#0");
		}
	}
}

void ece_t_e_data(int shotNumber, char** mapfun) {

	if (shotNumber <= SHOT_30814) {
		*mapfun = strdup("tsbase_collect;DECE:GSH1,DECE:GSH2;1:float:#0");
	}
	else {
		if (shotNumber <= SHOT_31957) {
			*mapfun = strdup("tsbase_collect;DVECE:GVSH1,DVECE:GVSH2;1:float:#0");
		}
		else {
			*mapfun = strdup("tsbase_collect;DVECE:GVSH1,DVECE:GVSH2,DVECE:GVSH3,DVECE:GVSH4;1:float:#0");
		}
	}
}

void ece_t_e_time(int shotNumber, char** mapfun) {

	if (shotNumber <= SHOT_30814) {
		*mapfun = strdup("tsbase_time;DECE:GSH1,DECE:GSH2;1:float:#0");
	}
	else {
		if (shotNumber <= SHOT_31957) {
			*mapfun = strdup("tsbase_time;DVECE:GVSH1,DVECE:GVSH2;1:float:#0");
		}
		else {
			*mapfun = strdup("tsbase_time;DVECE:GVSH1,DVECE:GVSH2,DVECE:GVSH3,DVECE:GVSH4;1:float:#0");
		}
	}
}

int getECEMode(int shotNumber) {

	struct Node* head = NULL;

	int ECE_mode;

	int shotNumberInFile;

	FILE * pFile;
	char content [15];

	char* ece_modes_file = getenv("WEST_ECE_MODES_FILE");

	pFile = fopen (ece_modes_file , "r");
	if (pFile == NULL) {
		int err = 901;
		addIdamError(&idamerrorstack, CODEERRORTYPE, "Unable to read ECE mode file from WEST", err, "");
		IDAM_LOG(LOG_DEBUG, "Error opening ECE mode file\n");
	}
	else {

		while(!feof(pFile))
		{
			if ( fgets (content , sizeof(content) , pFile) != NULL ) {
				const char delim[] = ":";
				shotNumberInFile = atoi(strtok(content, delim)); //the shot number
				ECE_mode = atoi(strtok(NULL, delim)); //the ECE mode
				push(&head, shotNumberInFile, ECE_mode);
			}

		}
	}

	struct Node* s = search(head, shotNumber);

	if (s == NULL) {
		int err = 901;
		addIdamError(&idamerrorstack, CODEERRORTYPE, "Unable to found ECE mode from WEST", err, "");
		IDAM_LOGF(LOG_DEBUG, "ECE mode not found for shot: %d\n", shotNumber);
	}
	else {
		return s->ECE_mode;
	}

	return -1;
}
