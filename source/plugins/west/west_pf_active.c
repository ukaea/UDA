#include "west_pf_active.h"

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

//Coils data reported by Philippe Moreau for DPOLO diagnostics - 25/08/2017
// R0, Z0, dR, dZ, Nturn
float A[5] =  { 0.7380,  0.0000, 0.1350, 1.7500, 195}; //coil 'A'
float Bh[5] = { 1.1180,  1.7500, 0.0750, 1.1000, 176};
float Dh[5] = { 2.8850,  1.9200, 0.2700, 0.3500, 95 };
float Eh[5] = { 3.7700,  1.5230, 0.2900, 0.3750, 96 };
float Fh[5] = { 4.3750,  0.6370, 0.2900, 0.3750, 96 };
float Fb[5] = { 4.3750, -0.6370, 0.2900, 0.3750, 96 };
float Eb[5] = { 3.7700, -1.5230, 0.2900, 0.3750, 96 };
float Db[5] = { 2.8850, -1.9200, 0.2700, 0.3500, 95 };
float Bb[5] = { 1.1180, -1.7500, 0.0750, 1.1000, 176};

//Divertor coils data reported by Philippe Moreau for DPOLO diagnostics - 26/09/2017
// R0, Z0, dR, dZ, Nturn
float Xu1[5] = { 2.0115,  0.7432, 0.0320, 0.0340, 4 }; //Xu1
float Xu2[5] = { 2.0755,  0.7772, 0.0320, 0.0340, 4 }; //Xu2
float Xu3[5] = { 2.1665,  0.8058, 0.0320, 0.0340, 4 }; //Xu3
float Xu4[5] = { 2.2305,  0.8398, 0.0320, 0.0340, 4 }; //Xu4 --> coil 'Divertor_up2_LFS'
float Xl1[5] = { 2.0115, -0.7403, 0.0320, 0.0340, 4 }; //Xl1
float Xl2[5] = { 2.0755, -0.7743, 0.0320, 0.0340, 4 }; //Xl2
float Xl3[5] = { 2.1665, -0.8029, 0.0320, 0.0340, 4 }; //Xl3
float Xl4[5] = { 2.2305, -0.8369, 0.0320, 0.0340, 4 }; //Xl4 --> coil 'Divertor_bottom2_LFS'

const char* COILS_NAMES[] = { "A", "Bh", "Dh", "Eh", "Fh", "Fb", "Eb", "Db", "Bb",
		"Divertor_top1_HFS", "Divertor_top2_HFS", "Divertor_top1_LFS", "Divertor_top2_LFS",
		"Divertor_bottom1_HFS", "Divertor_bottom2_HFS", "Divertor_bottom1_LFS",
		"Divertor_bottom2_LFS" };

int get_pf_current(int shotNumber, int extractionIndex, float** time, float** data, int* len, float normalizationFactor);
void pf_active(int shotNumber, DATA_BLOCK* data_block, int* nodeIndices, int index);


int pf_active_current_data(int shotNumber, DATA_BLOCK* data_block, int* nodeIndices)
{
	int index = nodeIndices[0]; //starts from 1
	float *time = NULL;
	float *data = NULL;
	int len;
	int status = -1;
	if (index <= 9) {
		status = get_pf_current(shotNumber, index, &time, &data, &len, 1000.);
	}
	else if (index == 10 || index == 11) {
		status = get_pf_current(shotNumber, 10, &time, &data, &len, 1000.);
	}
	else if (index == 12 || index == 13) {
		status = get_pf_current(shotNumber, 11, &time, &data, &len, 1000.);
	}
	else if (index == 14 || index == 15) {
		status = get_pf_current(shotNumber, 12, &time, &data, &len, 1000.);
	}
	else if (index == 16 || index == 17) {
		status = get_pf_current(shotNumber, 13, &time, &data, &len, 1000.);
	}
	if (status != 0) {
		int err = 901;
		char* errorMsg = "WEST:ERROR (pf_active_current_data): unable to get pf_active current for shot : ";
		char shotStr[6];
		sprintf(shotStr, "%d", shotNumber);
		strcat(errorMsg, shotStr);
		UDA_LOG(UDA_LOG_ERROR, "%s", errorMsg);
		addIdamError(CODEERRORTYPE, errorMsg, err, "");
		free(time);
		free(data);
		return status;
	}
	SetDynamicData(data_block, len, time, data);
	return 0;
}

int pf_active_current_time(int shotNumber, DATA_BLOCK* data_block, int* nodeIndices)
{
	int index = nodeIndices[0]; //starts from 1
	float *time = NULL;
	float *data = NULL;
	int len;
	int status = -1;
	if (index <= 9) {
		status = get_pf_current(shotNumber, index, &time, &data, &len, 1000.);
	}
	else if (index == 10 || index == 11) {
		status = get_pf_current(shotNumber, 10, &time, &data, &len, 1000.);
	}
	else if (index == 12 || index == 13) {
		status = get_pf_current(shotNumber, 11, &time, &data, &len, 1000.);
	}
	else if (index == 14 || index == 15) {
		status = get_pf_current(shotNumber, 12, &time, &data, &len, 1000.);
	}
	else if (index == 16 || index == 17) {
		status = get_pf_current(shotNumber, 13, &time, &data, &len, 1000.);
	}
	if (status != 0) {
		int err = 901;
		char* errorMsg = "WEST:ERROR (pf_active_current_time): unable to get pf_active_current_time for shot : ";
		char shotStr[6];
		sprintf(shotStr, "%d", shotNumber);
		strcat(errorMsg, shotStr);
		UDA_LOG(UDA_LOG_ERROR, "%s", errorMsg);
		addIdamError(CODEERRORTYPE, errorMsg, err, "");
		free(time);
		free(data);
		return status;
	}
	SetDynamicDataTime(data_block, len, time, data);
	return 0;
}


int get_pf_current(int shotNumber, int extractionIndex, float** time, float** data, int* len, float normalizationFactor)
{
	char* nomsigp = "GPOLO_IMES";
	char nomsigp_to_extract[50];
	addExtractionChars(nomsigp_to_extract, nomsigp, extractionIndex); //Concatenate nomsigp_to_extract avec !extractionIndex, example: !1, !2, ...
	int rang[2] = { 0, 0 };
	int status = readSignal(nomsigp_to_extract, shotNumber, 0, rang, time, data, len);
	multiplyFloat(*data, normalizationFactor, *len);
	return status;
}


int pf_active_coil_name(int shotNumber, DATA_BLOCK* data_block, int* nodeIndices)
{
	int index = nodeIndices[0] - 1; //starts from 0
	setReturnDataString(data_block, COILS_NAMES[index], NULL);
	return 0;
}

void pf_active_coil_identifier(int shotNumber, DATA_BLOCK* data_block, int* nodeIndices)
{
	pf_active_coil_name(shotNumber, data_block, nodeIndices);
}

void pf_active_element_name(int shotNumber, DATA_BLOCK* data_block, int* nodeIndices)
{
	int element_number = nodeIndices[1]; //starts from 1
	char s[100];
	sprintf(s, "%d", element_number);
	setReturnDataString(data_block, s, NULL);
}

void pf_active_element_identifier(int shotNumber, DATA_BLOCK* data_block, int* nodeIndices)
{
	pf_active_element_name(shotNumber, data_block, nodeIndices);
}

void pf_active_elements_shapeOf(int shotNumber, DATA_BLOCK* data_block, int* nodeIndices)
{
	int len = 1;
	setReturnDataIntScalar(data_block, len, NULL);
}

void pf_active_coils_shapeOf(int shotNumber, DATA_BLOCK* data_block, int* nodeIndices)
{
	int len = 0;
	getExtractionsCount("GPOLO_IMES", shotNumber, 0, &len);
	setReturnDataIntScalar(data_block, len, NULL);
}

void pf_active_R(int shotNumber, DATA_BLOCK* data_block, int* nodeIndices)
{
	pf_active(shotNumber, data_block, nodeIndices, 0);
}

void pf_active_Z(int shotNumber, DATA_BLOCK* data_block, int* nodeIndices)
{
	pf_active(shotNumber, data_block, nodeIndices, 1);
}

void pf_active_W(int shotNumber, DATA_BLOCK* data_block, int* nodeIndices)
{
	pf_active(shotNumber, data_block, nodeIndices, 2);
}

void pf_active_H(int shotNumber, DATA_BLOCK* data_block, int* nodeIndices)
{
	pf_active(shotNumber, data_block, nodeIndices, 3);
}

void pf_active_turns(int shotNumber, DATA_BLOCK* data_block, int* nodeIndices)
{
	int coil_number = nodeIndices[0]; //starts from 1
	float r;
	int index = 4;

	if (coil_number == 1) {
		r = (float)A[index];
	} else if (coil_number == 2) {
		r = (float)Bh[index];
	} else if (coil_number == 3) {
		r = (float)Dh[index];
	} else if (coil_number == 4) {
		r = (float)Eh[index];
	} else if (coil_number == 5) {
		r = (float)Fh[index];
	} else if (coil_number == 6) {
		r = (float)Fb[index];
	} else if (coil_number == 7) {
		r = (float)Eb[index];
	} else if (coil_number == 8) {
		r = (float)Db[index];
	} else if (coil_number == 9) {
		r = (float)Bb[index];
	} else if (coil_number == 10) {
		r = (float)Xu1[index];
	} else if (coil_number == 11) {
		r = (float)Xu2[index];
	} else if (coil_number == 12) {
		r = (float)Xu3[index];
	} else if (coil_number == 13) {
		r = (float)Xu4[index];
	} else if (coil_number == 14) {
		r = (float)Xl1[index];
	} else if (coil_number == 15) {
		r = (float)Xl2[index];
	} else if (coil_number == 16) {
		r = (float)Xl3[index];
	} else if (coil_number == 17) {
		r = (float)Xl4[index];
	}
	setReturnDataFloatScalar(data_block, r, NULL);
}

void pf_active(int shotNumber, DATA_BLOCK* data_block, int* nodeIndices, int index)
{
	int coil_number = nodeIndices[0]; //starts from 1
	float r;

	if (coil_number == 1) {
		r = A[index];
	} else if (coil_number == 2) {
		r = Bh[index];
	} else if (coil_number == 3) {
		r = Dh[index];
	} else if (coil_number == 4) {
		r = Eh[index];
	} else if (coil_number == 5) {
		r = Fh[index];
	} else if (coil_number == 6) {
		r = Fb[index];
	} else if (coil_number == 7) {
		r = Eb[index];
	} else if (coil_number == 8) {
		r = Db[index];
	} else if (coil_number == 9) {
		r = Bb[index];
	} else if (coil_number == 10) {
		r = Xu1[index];
	} else if (coil_number == 11) {
		r = Xu2[index];
	} else if (coil_number == 12) {
		r = Xu3[index];
	} else if (coil_number == 13) {
		r = Xu4[index];
	} else if (coil_number == 14) {
		r = Xl1[index];
	} else if (coil_number == 15) {
		r = Xl2[index];
	} else if (coil_number == 16) {
		r = Xl3[index];
	} else if (coil_number == 17) {
		r = Xl4[index];
	}
	setReturnDataFloatScalar(data_block, r, NULL);
}

