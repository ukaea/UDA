#include "tsarcad.h"

int readStaticParameters(char** pt_char, int *nb_val, int num_choc, char* nom_prod, char* nom_objet, char* nom_param, int val_nb);
int readSignal(char *nomsigp, int numchoc, int occ, int *rang, float **X, float **Y, int* len);
void getSignalType(char* nomsig, int numchoc, int *signalType);
void getExtractionsCount(char* nomsigp, int numchoc, int occ, int* extractionCount);



