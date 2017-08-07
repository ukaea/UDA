#ifndef UDA_PLUGINS_EXP2IMAS_MDS_H
#define UDA_PLUGINS_EXP2IMAS_MDS_H

int exp2imas_mds_get(const char *signalName, int shot, float **time, float **data, int *len);

#endif // UDA_PLUGINS_EXP2IMAS_MDS_H