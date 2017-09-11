#include "west_pf_active.h"

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

//Coils data reported by Philippe Moreau for DPOLO diagnostics - 25/08/2017
// R0, Z0, dR, dZ, Nturn
float A[5] = { 0.7380, 0.0000, 0.1350, 1.7500, 195 }; //coil 'A'
float Bh[5] = { 1.1180, 1.7500, 0.0750, 1.1000, 176 };
float Dh[5] = { 2.8850, 1.9200, 0.2700, 0.3500, 95 };
float Eh[5] = { 3.7700, 1.5230, 0.2900, 0.3750, 96 };
float Fh[5] = { 4.3750, 0.6370, 0.2900, 0.3750, 96 };
float Fb[5] = { 4.3750, -0.6370, 0.2900, 0.3750, 96 };
float Eb[5] = { 3.7700, -1.5230, 0.2900, 0.3750, 96 };
float Db[5] = { 2.8850, -1.9200, 0.2700, 0.3500, 95 };
float Bb[5] = { 1.1180, -1.7500, 0.0750, 1.1000, 176 };
float Divh1HFS[5] = { 2.0115, 0.7432, 0.0634, 0.0674, 4 };
float Divh2HFS[5] = { 2.0755, 0.7772, 0.0634, 0.0674, 4 };
float Divh1LFS[5] = { 2.1665, 0.8058, 0.0634, 0.0674, 4 };
float Divh2LFS[5] = { 2.2305, 0.8398, 0.0634, 0.0674, 4 };
float Divb1HFS[5] = { 2.0115, -0.7403, 0.0634, 0.0674, 4 };
float Divb2HFS[5] = { 2.0755, -0.7743, 0.0634, 0.0674, 4 };
float Divb1LFS[5] = { 2.1665, -0.8029, 0.0634, 0.0674, 4 };
float Divb2LFS[5] = { 2.2305, -0.8369, 0.0634, 0.0674, 4 }; //coil 'Divertor_bottom2_LFS'

const char* COILS_NAMES[] = { "A", "Bh", "Dh", "Eh", "Fh", "Fb", "Eb", "Db", "Bb",
                              "Divertor_top1_HFS", "Divertor_top2_HFS", "Divertor_top1_LFS", "Divertor_top2_LFS",
                              "Divertor_bottom1_HFS", "Divertor_bottom2_HFS", "Divertor_bottom1_LFS",
                              "Divertor_bottom2_LFS" };

void pf_active(int shotNumber, DATA_BLOCK* data_block, int* nodeIndices, int index);


void pf_active_coil_name(int shotNumber, DATA_BLOCK* data_block, int* nodeIndices)
{
    int index = nodeIndices[0] - 1; //starts from 0
    const char* value = COILS_NAMES[index];
    data_block->data_type = UDA_TYPE_STRING;
    data_block->data = strdup(value);
}

void pf_active_coil_identifier(int shotNumber, DATA_BLOCK* data_block, int* nodeIndices)
{
    pf_active_coil_name(shotNumber, data_block, nodeIndices);
}

void pf_active_element_name(int shotNumber, DATA_BLOCK* data_block, int* nodeIndices)
{
    int element_number = nodeIndices[1]; //starts from 1
    char* value = NULL;
    char s[100];
    sprintf(s, "%d", element_number);
    value = strdup(s);
    data_block->data_type = UDA_TYPE_STRING;
    data_block->data = strdup(value);
}

void pf_active_element_identifier(int shotNumber, DATA_BLOCK* data_block, int* nodeIndices)
{
    pf_active_element_name(shotNumber, data_block, nodeIndices);
}

void pf_active_elements_shapeOf(int shotNumber, DATA_BLOCK* data_block, int* nodeIndices)
{
    int len = 1;
    data_block->data_type = UDA_TYPE_INT;
    data_block->data = malloc(sizeof(int));
    *((int*)data_block->data) = len;
}

void pf_active_coils_shapeOf(int shotNumber, DATA_BLOCK* data_block, int* nodeIndices)
{
    int len = 17;
    data_block->data_type = UDA_TYPE_INT;
    data_block->data = malloc(sizeof(int));
    *((int*)data_block->data) = len;
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

    int* r = NULL;
    r = (int*)malloc(sizeof(int));

    int index = 4;

    if (coil_number == 1) {
        r[0] = (int)A[index];
    } else if (coil_number == 2) {
        r[0] = (int)Bh[index];
    } else if (coil_number == 3) {
        r[0] = (int)Dh[index];
    } else if (coil_number == 4) {
        r[0] = (int)Eh[index];
    } else if (coil_number == 5) {
        r[0] = (int)Fh[index];
    } else if (coil_number == 6) {
        r[0] = (int)Fb[index];
    } else if (coil_number == 7) {
        r[0] = (int)Eb[index];
    } else if (coil_number == 8) {
        r[0] = (int)Db[index];
    } else if (coil_number == 9) {
        r[0] = (int)Bb[index];
    } else if (coil_number == 10) {
        r[0] = (int)Divh1HFS[index];
    } else if (coil_number == 11) {
        r[0] = (int)Divh2HFS[index];
    } else if (coil_number == 12) {
        r[0] = (int)Divh1LFS[index];
    } else if (coil_number == 13) {
        r[0] = (int)Divh2LFS[index];
    } else if (coil_number == 14) {
        r[0] = (int)Divb1HFS[index];
    } else if (coil_number == 15) {
        r[0] = (int)Divb2HFS[index];
    } else if (coil_number == 16) {
        r[0] = (int)Divb1LFS[index];
    } else if (coil_number == 17) {
        r[0] = (int)Divb2LFS[index];
    }

    SetStatic1DINTData(data_block, 1, r);
}

void pf_active(int shotNumber, DATA_BLOCK* data_block, int* nodeIndices, int index)
{
    int coil_number = nodeIndices[0]; //starts from 1

    float* r = NULL;
    r = (float*)malloc(sizeof(float));

    if (coil_number == 1) {
        r[0] = A[index];
    } else if (coil_number == 2) {
        r[0] = Bh[index];
    } else if (coil_number == 3) {
        r[0] = Dh[index];
    } else if (coil_number == 4) {
        r[0] = Eh[index];
    } else if (coil_number == 5) {
        r[0] = Fh[index];
    } else if (coil_number == 6) {
        r[0] = Fb[index];
    } else if (coil_number == 7) {
        r[0] = Eb[index];
    } else if (coil_number == 8) {
        r[0] = Db[index];
    } else if (coil_number == 9) {
        r[0] = Bb[index];
    } else if (coil_number == 10) {
        r[0] = Divh1HFS[index];
    } else if (coil_number == 11) {
        r[0] = Divh2HFS[index];
    } else if (coil_number == 12) {
        r[0] = Divh1LFS[index];
    } else if (coil_number == 13) {
        r[0] = Divh2LFS[index];
    } else if (coil_number == 14) {
        r[0] = Divb1HFS[index];
    } else if (coil_number == 15) {
        r[0] = Divb2HFS[index];
    } else if (coil_number == 16) {
        r[0] = Divb1LFS[index];
    } else if (coil_number == 17) {
        r[0] = Divb2LFS[index];
    }
    SetStatic1DData(data_block, 1, r);
}

