#ifndef IDAM_PLUGINS_EQUIMAP_XDATA_H
#define IDAM_PLUGINS_EQUIMAP_XDATA_H

#include "equimap.h"

int xdatamapx(int rGridCount, float * rGrid, int ndata, float * rdata, float * data, float * mapped);

int xdatamapw(int volumePoints, EQUIMAPDATA * equimapdata, EFITDATA * efitdata, float * data, float minvalue,
              float * mapped);

int xdatamap(int rGridCount, float * rGrid, int ndata, float * rdata, float * data, float minvalue, float * mapped);

int xdataintegrate(int ndata, float * rdata, float * data, float minvalue, int * narea, float * area, float * xarea);

int xdatainterval(int rank, int order, int ndata, int * shape, float * dim, float tslice, float twindow, int * target1,
                  int * target2);

int xdatand(char * signal, char * source, int * hand, int * rank, int * order, int * ndata, int ** shape, float ** data,
            float ** dim);

int xdata1d(char * signal, char * source, int * hand, int * ndata, float ** data, float ** dim);

#endif // IDAM_PLUGINS_EQUIMAP_XDATA_H
