#include "west_pf_passive.h"

#include <string.h>
#include <stdio.h>
#include <assert.h>

#include <logging/logging.h>
#include <clientserver/initStructs.h>
#include <clientserver/errorLog.h>
#include <clientserver/stringUtils.h>
#include <clientserver/udaTypes.h>

#include "west_utilities.h"
#include "west_dyn_data_utilities.h"
#include "ts_rqparam.h"
#include "west_static_data_utilities.h"

float Rsup[10] = { 2.3475, 2.3476, 2.3273, 2.3272, 2.3273, 2.3284, 2.3671, 2.3669, 2.3676, 2.3683 };
float Zsup[10] = { 0.8923, 0.8503, 0.9055, 0.8806, 0.8620, 0.8360, 0.9038, 0.8838, 0.8618, 0.8372 };

float Rbaf[8] = { 2.6003, 2.6491, 2.5278, 2.5735, 2.6235, 2.6723, 2.6235, 2.6241 };
float Zbaf[8] = { -0.8503, -0.8503, -0.7216, -0.7222, -0.7222, -0.7222, -0.8094, -0.7618 };

float Rsup1[3] = { 1.9578, 1.9300, 1.9003 };
float Zsup1[3] = { 0.7558, 0.7300, 0.6807 };

float Rsup2[3] = { 2.0678, 2.1521, 2.2167 };
float Zsup2[3] = { 0.7273, 0.7596, 0.7905 };

float Rsup3[2] = { 2.2874, 2.2865 };
float Zsup3[2] = { 0.8451, 0.8883 };

float Rinf1[3] = { 1.9578, 1.9300, 1.9003 };
float Zinf1[3] = { -0.7558, -0.7300, -0.6807 };

float Rinf2[3] = { 2.0678, 2.1521, 2.2167 };
float Zinf2[3] = { -0.7273, -0.7596, -0.7905 };

float Rinf3[2] = { 2.2874, 2.2865 };
float Zinf3[2] = { -0.8451, -0.8883 };

int getIFREEB(int shotNumber, int extractionIndex, float** time, float** data, int* len, float normalizationFactor);
int getIDCOEF(int shotNumber, int extractionIndex, float** time, float** data, int* len, float normalizationFactor);

int I_case_upper(int shotNumber, float** time, float** data, int* len, float normalizationFactor);
int I_case_lower(int shotNumber, float** time, float** data, int* len, float normalizationFactor);
int I_baffle(int shotNumber, float** time, float** data, int* len, float normalizationFactor);
int I_uper_stab(int shotNumber, float** time, float** data, int* len, float normalizationFactor);
int Ip_fw(int shotNumber, float** time, float** data, int* len, float normalizationFactor);
int I_case_upper_hfs(int shotNumber, float** time, float** data, int* len, float normalizationFactor);
int I_case_upper_c(int shotNumber, float** time, float** data, int* len, float normalizationFactor);
int I_case_upper_lfs(int shotNumber, float** time, float** data, int* len, float normalizationFactor);
int I_case_lower_hfs(int shotNumber, float** time, float** data, int* len, float normalizationFactor);
int I_case_lower_c(int shotNumber, float** time, float** data, int* len, float normalizationFactor);
int I_case_lower_lfs(int shotNumber, float** time, float** data, int* len, float normalizationFactor);
int getCurrent(int shotNumber, DATA_BLOCK* data_block, int* nodeIndices, float** data, float** time, int* len);
float somme(float* data, int len);
void somme2(float** s, float* data1, float* data2, float* data3, int len);
float factUpper(int shotNumber);
float factLower(int shotNumber);


int getIFREEB(int shotNumber, int extractionIndex, float** time, float** data, int* len, float normalizationFactor)
{
    char* nomsigp = "GMAG_IFREEB";
    char nomsigp_to_extract[50];
    addExtractionChars(nomsigp_to_extract, nomsigp,
                       extractionIndex); //Concatenate nomsigp_to_extract avec !extractionIndex, example: !1, !2, ...
    int rang[2] = { 0, 0 };
    int status = readSignal(nomsigp_to_extract, shotNumber, 0, rang, time, data, len);
    multiplyFloat(*data, normalizationFactor, *len);
    return status;
}

int getIDCOEF(int shotNumber, int extractionIndex, float** time, float** data, int* len, float normalizationFactor)
{
    char* nomsigp = "GMAG_IDCOEF";
    char nomsigp_to_extract[50];
    addExtractionChars(nomsigp_to_extract, nomsigp,
                       extractionIndex); //Concatenate nomsigp_to_extract avec !extractionIndex, example: !1, !2, ...
    int rang[2] = { 0, 0 };
    int status = readSignal(nomsigp_to_extract, shotNumber, 0, rang, time, data, len);
    multiplyFloat(*data, normalizationFactor, *len);
    return status;
}

int I_case_upper(int shotNumber, float** time, float** data, int* len, float normalizationFactor)
{
    return getIFREEB(shotNumber, 4, time, data, len, normalizationFactor);
}

int I_case_lower(int shotNumber, float** time, float** data, int* len, float normalizationFactor)
{
    return getIFREEB(shotNumber, 5, time, data, len, normalizationFactor);
}

int I_baffle(int shotNumber, float** time, float** data, int* len, float normalizationFactor)
{
    return getIFREEB(shotNumber, 3, time, data, len, normalizationFactor);
}

int I_uper_stab(int shotNumber, float** time, float** data, int* len, float normalizationFactor)
{
    return getIFREEB(shotNumber, 2, time, data, len, normalizationFactor);
}

int Ip_fw(int shotNumber, float** time, float** data, int* len, float normalizationFactor)
{
    return getIFREEB(shotNumber, 1, time, data, len, normalizationFactor);
}

float factUpper(int shotNumber)
{

    int I_case_upper_len;
    float* I_case_upper_data = NULL;
    float* I_case_upper_time = NULL;
    int status = I_case_upper(shotNumber, &I_case_upper_time, &I_case_upper_data, &I_case_upper_len, 1.);
    if (status != 0) return status;

    int coeff_len;
    float* coeff1_data = NULL;
    float* coeff1_time = NULL;
    status = getIDCOEF(shotNumber, 19, &coeff1_time, &coeff1_data, &coeff_len, 1.);
    if (status != 0) return status;

    float* coeff2_data = NULL;
    float* coeff2_time = NULL;
    status = getIDCOEF(shotNumber, 20, &coeff2_time, &coeff2_data, &coeff_len, 1.);
    if (status != 0) return status;

    float* coeff3_data = NULL;
    float* coeff3_time = NULL;
    status = getIDCOEF(shotNumber, 21, &coeff3_time, &coeff3_data, &coeff_len, 1.);
    if (status != 0) return status;

    float* s2 = NULL;
    somme2(&s2, coeff1_data, coeff2_data, coeff3_data, coeff_len);

    float factUpper = somme(I_case_upper_data, I_case_upper_len) / somme(s2, coeff_len);
    return factUpper;
}

float factLower(int shotNumber)
{

    int I_case_lower_len;
    float* I_case_lower_data = NULL;
    float* I_case_lower_time = NULL;
    int status = I_case_lower(shotNumber, &I_case_lower_time, &I_case_lower_data, &I_case_lower_len, 1.);
    if (status != 0) return status;

    int coeff_len;
    float* coeff1_data = NULL;
    float* coeff1_time = NULL;
    status = getIDCOEF(shotNumber, 22, &coeff1_time, &coeff1_data, &coeff_len, 1.);
    if (status != 0) return status;

    float* coeff2_data = NULL;
    float* coeff2_time = NULL;
    status = getIDCOEF(shotNumber, 23, &coeff2_time, &coeff2_data, &coeff_len, 1.);
    if (status != 0) return status;

    float* coeff3_data = NULL;
    float* coeff3_time = NULL;
    status = getIDCOEF(shotNumber, 24, &coeff3_time, &coeff3_data, &coeff_len, 1.);
    if (status != 0) return status;

    float* s2 = NULL;
    somme2(&s2, coeff1_data, coeff2_data, coeff3_data, coeff_len);

    float factLower = somme(I_case_lower_data, I_case_lower_len) / somme(s2, coeff_len);
    return factLower;
}

int I_case_upper_hfs(int shotNumber, float** time, float** data, int* len, float normalizationFactor)
{
    int status = getIDCOEF(shotNumber, 19, time, data, len, factUpper(shotNumber));
    if (status != 0) return status;
    multiplyFloat(*data, normalizationFactor, *len);
    return status;
}

int I_case_upper_c(int shotNumber, float** time, float** data, int* len, float normalizationFactor)
{
    int status = getIDCOEF(shotNumber, 20, time, data, len, factUpper(shotNumber));
    if (status != 0) return status;
    multiplyFloat(*data, normalizationFactor, *len);
    return status;
}

int I_case_upper_lfs(int shotNumber, float** time, float** data, int* len, float normalizationFactor)
{
    int status = getIDCOEF(shotNumber, 21, time, data, len, factUpper(shotNumber));
    if (status != 0) return status;
    multiplyFloat(*data, normalizationFactor, *len);
    return status;
}

int I_case_lower_hfs(int shotNumber, float** time, float** data, int* len, float normalizationFactor)
{
    int status = getIDCOEF(shotNumber, 22, time, data, len, factLower(shotNumber));
    if (status != 0) return status;
    multiplyFloat(*data, normalizationFactor, *len);
    return status;
}

int I_case_lower_c(int shotNumber, float** time, float** data, int* len, float normalizationFactor)
{
    int status = getIDCOEF(shotNumber, 23, time, data, len, factLower(shotNumber));
    if (status != 0) return status;
    multiplyFloat(*data, normalizationFactor, *len);
    return status;
}

int I_case_lower_lfs(int shotNumber, float** time, float** data, int* len, float normalizationFactor)
{
    int status = getIDCOEF(shotNumber, 24, time, data, len, factLower(shotNumber));
    if (status != 0) return status;
    multiplyFloat(*data, normalizationFactor, *len);
    return status;
}

void passive_name(int shotNumber, DATA_BLOCK* data_block, int* nodeIndices)
{
    int k = nodeIndices[0]; //starts from 1
    char* value = NULL;

    if (k <= 10) {
        char s[100];
        sprintf(s, "Upper stabilizing plate (loop %d)", k);
        value = strdup(s);
    } else if (k > 10 && k <= 18) {
        char s[100];
        sprintf(s, "Baffle support (loop %d)", k - 10);
        value = strdup(s);
    } else if (k > 18 && k <= 21) {
        char s[100];
        sprintf(s, "Upper divertor casing, high field side part (loop %d)", k - 18);
        value = strdup(s);
    } else if (k > 21 && k <= 24) {
        char s[100];
        sprintf(s, "Upper divertor casing, centre part (loop %d)", k - 21);
        value = strdup(s);
    } else if (k > 24 && k <= 26) {
        char s[100];
        sprintf(s, "Upper divertor casing, low field side part (loop %d)", k - 24);
        value = strdup(s);
    } else if (k > 26 && k <= 29) {
        char s[100];
        sprintf(s, "Upper divertor casing, high field side part (loop %d)", k - 26);
        value = strdup(s);
    } else if (k > 29 && k <= 32) {
        char s[100];
        sprintf(s, "Lower divertor casing, centre part (loop %d)", k - 29);
        value = strdup(s);
    }

    data_block->data_type = UDA_TYPE_STRING;
    data_block->data = strdup(value);
}

void passive_r(int shotNumber, DATA_BLOCK* data_block, int* nodeIndices)
{
    int k = nodeIndices[0]; //starts from 1
    float* r = NULL;
    r = (float*)malloc(sizeof(float));

    if (k <= 10) {
        r[0] = Rsup[k - 1];
    } else if (k > 10 && k <= 18) {
        r[0] = Rbaf[k - 11];
    } else if (k > 18 && k <= 21) {
        r[0] = Rsup1[k - 19];
    } else if (k > 21 && k <= 24) {
        r[0] = Rsup2[k - 22];
    } else if (k > 24 && k <= 26) {
        r[0] = Rsup3[k - 25];
    } else if (k > 26 && k <= 29) {
        r[0] = Rinf1[k - 27];
    } else if (k > 29 && k <= 32) {
        r[0] = Rinf2[k - 30];
    }
    SetStatic1DData(data_block, 1, r);
}

void passive_z(int shotNumber, DATA_BLOCK* data_block, int* nodeIndices)
{
    int k = nodeIndices[0]; //starts from 1
    float* z = NULL;
    z = (float*)malloc(sizeof(float));

    if (k <= 10) {
        z[0] = Zsup[k - 1];
    } else if (k > 10 && k <= 18) {
        z[0] = Zbaf[k - 11];
    } else if (k > 18 && k <= 21) {
        z[0] = Zsup1[k - 19];
    } else if (k > 21 && k <= 24) {
        z[0] = Zsup2[k - 22];
    } else if (k > 24 && k <= 26) {
        z[0] = Zsup3[k - 25];
    } else if (k > 26 && k <= 29) {
        z[0] = Zinf1[k - 27];
    } else if (k > 29 && k <= 32) {
        z[0] = Zinf2[k - 30];
    }
    SetStatic1DData(data_block, 1, z);
}

void passive_current_shapeOf(int shotNumber, DATA_BLOCK* data_block, int* nodeIndices)
{

    int len = 34;
    data_block->data_type = UDA_TYPE_INT;
    data_block->data = malloc(sizeof(int));
    *((int*)data_block->data) = len;
}

int passive_current(int shotNumber, DATA_BLOCK* data_block, int* nodeIndices)
{

    float* data = NULL;
    float* time = NULL;
    int len;
    int status = getCurrent(shotNumber, data_block, nodeIndices, &data, &time, &len);
    if (status != 0) return status;
    SetDynamicData(data_block, len, time, data);
    return 0;
}

int getCurrent(int shotNumber, DATA_BLOCK* data_block, int* nodeIndices, float** data, float** time, int* len)
{

    UDA_LOG(UDA_LOG_DEBUG, "in getCurrent...\n");

    int k = nodeIndices[0]; //starts from 1
    int status = -1;
    const int Rsup_length = (int)(sizeof(Rsup) / sizeof(Rsup[0]));
    const int Rbaf_length = (int)(sizeof(Rbaf) / sizeof(Rbaf[0]));
    const int Rsup1_length = (int)(sizeof(Rsup1) / sizeof(Rsup1[0]));
    const int Rsup2_length = (int)(sizeof(Rsup2) / sizeof(Rsup2[0]));
    const int Rsup3_length = (int)(sizeof(Rsup3) / sizeof(Rsup3[0]));
    const int Rinf1_length = (int)(sizeof(Rinf1) / sizeof(Rinf1[0]));
    const int Rinf2_length = (int)(sizeof(Rinf2) / sizeof(Rinf2[0]));

    if (k <= 10) {
        status = I_uper_stab(shotNumber, time, data, len, 1. / Rsup_length);
    } else if (k > 10 && k <= 18) {
        status = I_baffle(shotNumber, time, data, len, 1. / Rbaf_length);
    } else if (k > 18 && k <= 21) {
        status = I_case_upper_hfs(shotNumber, time, data, len, 1. / Rsup1_length);
    } else if (k > 21 && k <= 24) {
        status = I_case_upper_c(shotNumber, time, data, len, 1. / Rsup2_length);
    } else if (k > 24 && k <= 26) {
        status = I_case_upper_lfs(shotNumber, time, data, len, 1. / Rsup3_length);
    } else if (k > 26 && k <= 29) {
        status = I_case_lower_hfs(shotNumber, time, data, len, 1. / Rinf1_length);
    } else if (k > 29 && k <= 32) {
        status = I_case_lower_c(shotNumber, time, data, len, 1. / Rinf2_length);
    }

    return status;
}


int passive_time(int shotNumber, DATA_BLOCK* data_block, int* nodeIndices)
{

    int k = nodeIndices[0]; //starts from 1
    int len;
    float* data = NULL;
    float* time = NULL;
    int status = -1;

    if (k <= 10) {
        status = getIFREEB(shotNumber, 2, &time, &data, &len, 1.);
    } else if (k > 10 && k <= 18) {
        status = getIFREEB(shotNumber, 3, &time, &data, &len, 1.);
    } else if (k > 18 && k <= 21) {
        status = getIDCOEF(shotNumber, 19, &time, &data, &len, 1.);
    } else if (k > 21 && k <= 24) {
        status = getIDCOEF(shotNumber, 20, &time, &data, &len, 1.);
    } else if (k > 24 && k <= 26) {
        status = getIDCOEF(shotNumber, 21, &time, &data, &len, 1.);
    } else if (k > 26 && k <= 29) {
        status = getIDCOEF(shotNumber, 22, &time, &data, &len, 1.);
    } else if (k > 29 && k <= 32) {
        status = getIDCOEF(shotNumber, 23, &time, &data, &len, 1.);
    }

    if (status != 0) return status;
    SetDynamicDataTime(data_block, len, time, data);

    //SetStatic1DData(data_block, len, time);

    return 0;
}

float somme(float* data, int len)
{
    int i;
    float s = 0;
    for (i = 0; i < len; i++) {
        s += data[i];
    }
    return s;
}

void somme2(float** s, float* data1, float* data2, float* data3, int len)
{
    *s = (float*)calloc(len, sizeof(float));
    int i;
    for (i = 0; i < len; i++) {
        *s[i] = data1[i] + data2[i] + data3[i];
    }
}

