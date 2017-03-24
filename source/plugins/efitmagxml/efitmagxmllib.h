#ifndef UDA_PLUGINS_EFITMAGXML_EFITMAGXMLLIB_H
#define UDA_PLUGINS_EFITMAGXML_EFITMAGXMLLIB_H

#include <clientserver/xmlStructs.h>

char* getdevice(EFIT* efit);

void getinstance(INSTANCE* str, int* seq, int* status, float* factor, char** archive, char** file, char** signal,
                        char** owner, char** format);
int getmagprobe(EFIT* efit, const int n, float* r, float* z, float* angle, float* aerr, float* rerr);
int getpfsupplies(EFIT* efit, const int n, float* aerr, float* rerr);
int getfluxloop(EFIT* efit, const int n, float** r, float** z, float** dphi, float* aerr, float* rerr);
int getpfpassive(EFIT* efit, const int n, float** r, float** z, float** dr, float** dz, float** ang1,
                        float** ang2, float** res, float* aerr, float* rerr);
int getpfcoil(EFIT* efit, const int n, int* turns, float** r, float** z, float** dr, float** dz, float* aerr,
                     float* rerr);
int getpfcircuit(EFIT* efit, const int n, int* supply, int** coil);
int getplasmacurrent(EFIT* efit, float* aerr, float* rerr);
int getdiamagnetic(EFIT* efit, float* aerr, float* rerr);
int gettoroidalfield(EFIT* efit, float* aerr, float* rerr);
int getlimitercoords(EFIT* efit, float** r, float** z);
int getpfsupply(EFIT* efit, const int n, float* aerr, float* rerr);

#endif // UDA_PLUGINS_EFITMAGXML_EFITMAGXMLLIB_H