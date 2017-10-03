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
int
getECEModeHarmonic(int shotNumber, int channel, float** time, float** data, int* len, char* TOP_collections_parameters);
int getECEModeObject(char* TOP_collections_parameters, char** objectName, int GVSHArray, int mode);
int getECEModeHarmonicTime(int shotNumber, int channel, float** time, int* len, char* TOP_collections_parameters);
char* getTopParameters(int shotNumber);


int test_fun(int shotNumber, DATA_BLOCK* data_block, int* nodeIndices)
{
    float* data = NULL;
    float* time = NULL;
    //SetDynamicData(data_block, 0, time, data);
    return 0;
}

int ece_harmonic_data(int shotNumber, DATA_BLOCK* data_block, int* nodeIndices)
{

    int channel = nodeIndices[0]; //starts from 1

    float* data = NULL;
    float* time = NULL;
    char* TOP_collections_parameters = "DVECE:GVSH1,DVECE:GVSH2,DVECE:GVSH3,DVECE:GVSH4";

    if (shotNumber >= ARCADE_GECEMODE_EXISTS_FROM_SHOT) {

        int len;
        int status = getECEModeHarmonic(shotNumber, channel, &time, &data, &len, TOP_collections_parameters);
        if (status != 0) {
            free(time);
            free(data);
            return status;
        }
        SetDynamicData(data_block, len, time, data);
    } else {
        int len;
        int status = getECEModeHarmonicTime(shotNumber, channel, &time, &len, TOP_collections_parameters);
        UDA_LOG(UDA_LOG_DEBUG, "getECEModeHarmonicTime status: %d\n", status);
        if (status != 0) return status;
        data = malloc(sizeof(float) * len);
        int i;
        int mode = getECEModeFromNPZFile(shotNumber); //Get the ECE acquisition mode from NPZ file
        UDA_LOG(UDA_LOG_DEBUG, "mode: %d\n", mode);
        if (mode == -1) { //ERROR
            free(time);
            free(data);
            return -1;
        }
        for (i = 0; i < len; i++)
            data[i] = mode;

        SetDynamicData(data_block, len, time, data);
    }
    return 0;
}

int ece_harmonic_time(int shotNumber, DATA_BLOCK* data_block, int* nodeIndices)
{

    int channel = nodeIndices[0]; //UDA request index, starts from 1

    float* data = NULL;
    float* time = NULL;
    int len;
    char* TOP_collections_parameters = getTopParameters(shotNumber);
    UDA_LOG(UDA_LOG_DEBUG, "Calling getECEModeHarmonicTime() \n");
    getECEModeHarmonicTime(shotNumber, channel, &time, &len, TOP_collections_parameters);
    UDA_LOG(UDA_LOG_DEBUG, "harmonic time array length: %d\n", len);

    //	UDA_LOG(UDA_LOG_DEBUG, "%s\n", "First time values...");
    //	int j;
    //	for (j=0; j <10; j++) {
    //		UDA_LOG(UDA_LOG_DEBUG, "time : %f\n", time[j]);
    //	}
    SetDynamicDataTime(data_block, len, time, data);
    UDA_LOG(UDA_LOG_DEBUG, "reaching end of function of ece_harmonic_time()\n");
    return 0;
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
    char* TOP_collections_parameters = NULL;

    int channel = nodeIndices[0]; //starts from 1

    if (shotNumber >= ARCADE_GECEMODE_EXISTS_FROM_SHOT) {

        float* data = NULL;
        int len;
        TOP_collections_parameters = "DVECE:GVSH1,DVECE:GVSH2,DVECE:GVSH3,DVECE:GVSH4";
        int status = getECEModeHarmonic(shotNumber, channel, &frequencies_time, &data, &len,
                                        TOP_collections_parameters);

        if (status != 0) return status;

        frequencies_data = malloc(sizeof(float) * len);

        int i;

        for (i = 0; i < len; i++) {

            int index = nodeIndices[0];

            if (data[i] == 1) {

                if (index <= 7) {
                    frequencies_data[i] = GVSH1_01[i];
                } else if (index > 7 && index <= 15) {
                    frequencies_data[i] = GVSH2_01[i];
                } else if (index > 15 && index <= 23) {
                    frequencies_data[i] = GVSH3_01[i];
                } else if (index > 23 && index <= 31) {
                    frequencies_data[i] = GVSH4_01[i];
                }
            } else if (data[i] == 2) {

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
                addIdamError(CODEERRORTYPE, "Unexpected ECE mode from WEST", err, "");
                free(data);
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
        float* GSH;
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
                TOP_collections_parameters = "DECE:GSH1,DECE:GSH2";
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
                TOP_collections_parameters = "DVECE:GVSH1,DVECE:GVSH2,DVECE:GVSH3,DVECE:GVSH4";
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
                TOP_collections_parameters = "DECE:GSH1,DECE:GSH2";
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
                TOP_collections_parameters = "DVECE:GVSH1,DVECE:GVSH2,DVECE:GVSH3,DVECE:GVSH4";
            }
        } else {
            return -1; //ERROR
        }

        int len;
        UDA_LOG(UDA_LOG_DEBUG, "Calling getECEModeHarmonicTime\n");
        getECEModeHarmonicTime(shotNumber, channel, &frequencies_time, &len, TOP_collections_parameters);
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

int ece_names(int shotNumber, DATA_BLOCK* data_block, int* nodeIndices)
{

    UDA_LOG(UDA_LOG_DEBUG, "Calling ece_names\n");
    char* name = "";
    data_block->data_type = UDA_TYPE_STRING;
    data_block->data = strdup(name);

    return 0;
}

int ece_identifiers(int shotNumber, DATA_BLOCK* data_block, int* nodeIndices)
{

    UDA_LOG(UDA_LOG_DEBUG, "Calling ece_identifiers\n");
    int channelNumber = nodeIndices[0]; //UDA request index
    char* identifier = malloc(sizeof(char));
    sprintf(identifier, "%d", channelNumber + 1); //starts the identifier to 1
    data_block->data_type = UDA_TYPE_STRING;
    data_block->data = identifier;
    return 0;
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

void ece_t_e_time(int shotNumber, char** mapfun)
{

    if (shotNumber < SHOT_30814) {
        *mapfun = strdup("tsbase_time;DECE:GSH1,DECE:GSH2;1:float:#0");
    } else {
        if (shotNumber < SHOT_31957) {
            *mapfun = strdup("tsbase_time;DVECE:GVSH1,DVECE:GVSH2;1:float:#0");
        } else {
            *mapfun = strdup("tsbase_time;DVECE:GVSH1,DVECE:GVSH2,DVECE:GVSH3,DVECE:GVSH4;1:float:#0");
        }
    }
}

int getECEModeFromNPZFile(int shotNumber)
{

    if (shotNumber >= ARCADE_GECEMODE_EXISTS_FROM_SHOT) {
        int err = 901;
        addIdamError(CODEERRORTYPE, "getECEModeFromNPZFile() should not be called for this shot number (from WEST)",
                     err, "");
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
        addIdamError(CODEERRORTYPE, "Unable to read ECE mode file from WEST", err, "");
        UDA_LOG(UDA_LOG_DEBUG, "Error opening ECE mode file\n");
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
        addIdamError(CODEERRORTYPE, "Unable to found ECE mode from WEST", err, "");
        UDA_LOG(UDA_LOG_DEBUG, "ECE mode not found for shot: %d\n", shotNumber);
    } else {
        searchedMode = s->ECE_mode;
    }

    free(head);
    free(s);

    return searchedMode;
}

int getECEModeHarmonicTime(int shotNumber, int channel, float** time, int* len,
                           char* TOP_collections_parameters)
{
    //channel is the UDA index starting from 1
    int GVSHArray; //0, 1, 2 or 3 (refers to the GVSH1, GVSH2, GVSH3 or GVSH4 Arcade object)
    int index = channel - 1;
    if (index <= 7) {
        GVSHArray = 0;
    } else if (index > 7 && index <= 15) {
        GVSHArray = 1;
    } else if (index > 15 && index <= 23) {
        GVSHArray = 2;
    } else if (index > 23 && index <= 31) {
        GVSHArray = 3;
    }

    UDA_LOG(UDA_LOG_DEBUG, "TOP_collections_parameters: %s\n", TOP_collections_parameters);

    float* O_ModeData = NULL;

    int rang[2] = { 0, 0 };
    char* O_ObjectName = NULL;

    int mode = 1;

    int status = getECEModeObject(TOP_collections_parameters, &O_ObjectName, GVSHArray, mode);

    if (status != 0) return status;

    UDA_LOG(UDA_LOG_DEBUG, "O_ObjectName: %s\n", O_ObjectName);
    status = readSignal(O_ObjectName, shotNumber, 0, rang, time, &O_ModeData, len);

    if (status != 0) {
        int err = 901;
        addIdamError(CODEERRORTYPE, "Unable to get time for O1 mode", err, "");
    }

    free(O_ModeData);
    free(O_ObjectName);

    return status;

}


int getECEModeHarmonic(int shotNumber, int channel, float** time, float** data, int* len,
                       char* TOP_collections_parameters)
{

    //channel is the UDA index starting from 1
    int GVSHArray; //0, 1, 2 or 3 (refers to the GVSH1, GVSH2, GVSH3 or GVSH4 Arcade object)
    int index = channel - 1;
    if (index <= 7) {
        GVSHArray = 0;
    } else if (index > 7 && index <= 15) {
        GVSHArray = 1;
    } else if (index > 15 && index <= 23) {
        GVSHArray = 2;
    } else if (index > 23 && index <= 31) {
        GVSHArray = 3;
    }

    UDA_LOG(UDA_LOG_DEBUG, "TOP_collections_parameters: %s\n", TOP_collections_parameters);

    float* O_ModeTime = NULL;
    float* O_ModeData = NULL;
    int O_Mode_Signal_len;

    int rang[2] = { 0, 0 };
    char* O_ObjectName = NULL;

    int mode = 1;

    int status = getECEModeObject(TOP_collections_parameters, &O_ObjectName, GVSHArray, mode);

    if (status != 0) return status;

    status = readSignal(O_ObjectName, shotNumber, 0, rang, &O_ModeTime, &O_ModeData, &O_Mode_Signal_len);

    if (status != 0) {
        int err = 901;
        UDA_LOG(UDA_LOG_DEBUG, "Unable to get signal\n");
        addIdamError(CODEERRORTYPE, "Unable to get signal", err, "");
        return status;
    }

    float* X_ModeTime = NULL;
    float* X_ModeData = NULL;
    int X_Mode_Signal_len;
    char* X_ObjectName = NULL;

    mode = 2;

    status = getECEModeObject(TOP_collections_parameters, &X_ObjectName, GVSHArray, mode);

    if (status != 0) return status;

    status = readSignal(X_ObjectName, shotNumber, 0, rang, &X_ModeTime, &X_ModeData, &X_Mode_Signal_len);

    if (status != 0) {
        int err = 901;
        UDA_LOG(UDA_LOG_DEBUG, "Unable to get signal\n");
        addIdamError(CODEERRORTYPE, "Unable to get signal", err, "");
        return status;
    }

    *time = O_ModeTime; //we set the time to the O mode time
    *data = malloc(O_Mode_Signal_len *
                   sizeof(float)); //we allocate the data array with the same length that the O1 mode signal
    *len = O_Mode_Signal_len;

    int i;

    for (i = 0; i < O_Mode_Signal_len; i++) {
        if (O_ModeData[i] == 1) {
            *data[i] = O_ModeData[i];
        } else {
            *data[i] = X_ModeData[i];
        }
    }
    return 0;

}


int getECEModeObject(char* TOP_collections_parameters, char** objectName, int GVSHArray, int mode)
{
    char* command = NULL;
    UDA_LOG(UDA_LOG_DEBUG, "getting the command\n");
    int status = getCommand(GVSHArray, &command,
                            TOP_collections_parameters); //command is for instance: DVECE:GVSH1 or DVECE:GVSH2

    if (status != 0) {
        int err = 901;
        UDA_LOG(UDA_LOG_DEBUG, "Unable to get command\n");
        addIdamError(CODEERRORTYPE, "Unable to get command", err, "");
        free(command);
        return status;
    }

    UDA_LOG(UDA_LOG_DEBUG, "command: %s\n", command);
    UDA_LOG(UDA_LOG_DEBUG, "Getting object name\n");
    getObjectName(objectName, command);

    char result[50];
    addExtractionChars(result, *objectName, mode); //Concatenate objectName with %1 or %2 depending on the ECE mode
    *objectName = strdup(result);
    UDA_LOG(UDA_LOG_DEBUG, "Object name for mode %d: %s\n", mode, *objectName);
    free(command);
    return 0;
}

char* getTopParameters(int shotNumber)
{

    char* TOP_collections_parameters = malloc(sizeof(char) * 100);

    if (shotNumber < SHOT_30814) {
        TOP_collections_parameters = "DECE:GSH1,DECE:GSH2";
    } else {
        TOP_collections_parameters = "DVECE:GVSH1,DVECE:GVSH2,DVECE:GVSH3,DVECE:GVSH4";
    }

    return TOP_collections_parameters;
}


