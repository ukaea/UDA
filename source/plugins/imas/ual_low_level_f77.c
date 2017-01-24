#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include <mdslib.h>
#include "ual_low_level.h"

static void trim(char *str)
{
    int i;
    for(i = strlen(str) - 1; i > 0 && str[i] == ' '; i--)
        str[i] = 0;
}

static char *allocateC(char *in, int len)
{
    char *outStr = malloc(len + 1);
    memcpy(outStr, in, len);
    outStr[len] = 0;
    trim(outStr);
    return outStr;
}

int get_last_errmsg_(char *errmsg, int errmsgLen)
{
    int i, len;
    char *msg = imas_last_errmsg();
    len = strlen(msg);
    for(i = 0; i < len && i < errmsgLen; i++)
        errmsg[i] = msg[i];
    for(;i < errmsgLen; i++)
        errmsg[i] = ' ';
    return 0;
}

extern void reportInfo(char *str1, char *str2);

int delete_data_(int *expIdx, char *cpoPath, char *path, int cpoPathLen, int pathLen)
{
    int status;
    char *intCpoPath = allocateC(cpoPath, cpoPathLen);
    char *intPath = allocateC(path, pathLen);

    status = deleteData(*expIdx, intCpoPath, intPath);
    free(intCpoPath);
    free(intPath);
    return status;
}

int put_string_(int *expIdx, char *cpoPath, char *path, char *data, int *stat, int cpoPathLen, int pathLen, int dataLen)
{
    int status;
    char *intCpoPath = allocateC(cpoPath, cpoPathLen);
    char *intPath = allocateC(path, pathLen);
    char *intData = allocateC(data, dataLen);

    status = putString(*expIdx, intCpoPath, intPath, intData,dataLen);
   *stat = status;
    free(intCpoPath);
    free(intPath);

    free(intData);
    return status;
}

int put_int_(int *expIdx, char *cpoPath, char *path, int *data, int *stat, int cpoPathLen, int pathLen)
{
    int status;
    char *intCpoPath = allocateC(cpoPath, cpoPathLen);
    char *intPath = allocateC(path, pathLen);

    status = putInt(*expIdx, intCpoPath, intPath, *data);
    *stat = status;
    free(intCpoPath);
    free(intPath);

    return status;
}

int put_float_(int *expIdx, char *cpoPath, char *path, float *data, int *stat, int cpoPathLen, int pathLen)
{
    int status;
    char *intCpoPath = allocateC(cpoPath, cpoPathLen);
    char *intPath = allocateC(path, pathLen);

    status = putFloat(*expIdx, intCpoPath, intPath, *data);
    *stat = status;
    free(intCpoPath);
    free(intPath);

   return status;
}

int put_double_(int *expIdx, char *cpoPath, char *path, double *data, int *stat, int cpoPathLen, int pathLen)
{
    int status;
    char *intCpoPath = allocateC(cpoPath, cpoPathLen);
    char *intPath = allocateC(path, pathLen);

    status = putDouble(*expIdx, intCpoPath, intPath, *data);
    *stat = status;

    free(intCpoPath);
    free(intPath);

   return status;
}

int put_vect1d_int_(int *expIdx, char *cpoPath, char *path, char *timeBasePath, int *data, int *dim, int *isTimed, int *stat, int cpoPathLen, int pathLen, int timeBasePathLen)
{
     int status;
    char *intCpoPath = allocateC(cpoPath, cpoPathLen);
    char *intPath = allocateC(path, pathLen);
    char *intTimeBasePath = allocateC(timeBasePath, timeBasePathLen);
    status =putVect1DInt(*expIdx, intCpoPath, intPath, intTimeBasePath, data, *dim, *isTimed);
    *stat = status;

    free(intCpoPath);
    free(intPath);
    free(intTimeBasePath);
    return status;
}
int put_vect1d_float_(int *expIdx, char *cpoPath, char *path, char *timeBasePath, float *data, int *dim, int *isTimed, int *stat, int cpoPathLen, int pathLen, int timeBasePathLen)
{
     int status;
    char *intCpoPath = allocateC(cpoPath, cpoPathLen);
    char *intPath = allocateC(path, pathLen);
    char *intTimeBasePath = allocateC(timeBasePath, timeBasePathLen);
    status =putVect1DFloat(*expIdx, intCpoPath, intPath, intTimeBasePath, data, *dim, *isTimed);
    *stat = status;

    free(intCpoPath);
    free(intPath);
    free(intTimeBasePath);
    return status;
}

int put_vect1d_double_(int *expIdx, char *cpoPath, char *path, char *timeBasePath, double *data, int *dim, int *isTimed, int *stat, int cpoPathLen, int pathLen, int timeBasePathLen)
{
    int status;

    char *intCpoPath = allocateC(cpoPath, cpoPathLen);
    char *intPath = allocateC(path, pathLen);
    char *intTimeBasePath = allocateC(timeBasePath, timeBasePathLen);

    status =putVect1DDouble(*expIdx, intCpoPath, intPath, intTimeBasePath, data, *dim, *isTimed);
    *stat = status;

    free(intCpoPath);
    free(intPath);
    free(intTimeBasePath);
    return status;
}

int put_vect1d_string_(int *expIdx, char *cpoPath, char *path, char *timeBasePath, char *data, int *dim, int *dimtab, int *isTimed, int *stat, int cpoPathLen, int pathLen, int timeBasePathLen)
{
    int status;
    int i;
    char *intCpoPath = allocateC(cpoPath, cpoPathLen);
    char *intPath = allocateC(path, pathLen);
    char *intTimeBasePath = allocateC(timeBasePath, timeBasePathLen);

    char** tabchar;
    tabchar=malloc(*dim*sizeof(char *));

    for(i = 0; i < *dim; i++)
    {
       tabchar[i] = allocateC(data + i*132, dimtab[i]);
       /*printf("Line %d : %s\n",i,tabchar[i]); */
    }
    /* Old version without dimtab The drawback of this is that we store 132 characters for each line, whatever the real length of the line */
    /*for(i = 0; i < *dim; i++)
    {
       tabchar[i] = allocateC(data + i*132, 132);
    } */

    status =putVect1DString(*expIdx, intCpoPath, intPath, intTimeBasePath, tabchar, *dim, *isTimed);
    *stat = status;

    free(intCpoPath);
    free(intPath);
    free(intTimeBasePath);
    for(i = 0; i < *dim; i++) free(tabchar[i]);
    free(tabchar);
    return status;
}

int put_vect2d_int_(int *expIdx, char *cpoPath, char *path, char *timeBasePath, int *data, int *dim1, int *dim2, int *isTimed, int *stat, int cpoPathLen, int pathLen, int timeBasePathLen)
{
    int status;
    char *intCpoPath = allocateC(cpoPath, cpoPathLen);
    char *intPath = allocateC(path, pathLen);
    char *intTimeBasePath = allocateC(timeBasePath, timeBasePathLen);
    status =putVect2DInt(*expIdx, intCpoPath, intPath, intTimeBasePath,  data, *dim1, *dim2, *isTimed);
    *stat = status;

    free(intCpoPath);
    free(intPath);
    free(intTimeBasePath);
    return status;
}
int put_vect2d_float_(int *expIdx, char *cpoPath, char *path, char *timeBasePath, float *data, int *dim1, int *dim2, int *isTimed, int *stat, int cpoPathLen, int pathLen, int timeBasePathLen)
{
     int status;
    char *intCpoPath = allocateC(cpoPath, cpoPathLen);
    char *intPath = allocateC(path, pathLen);
    char *intTimeBasePath = allocateC(timeBasePath, timeBasePathLen);
    status = putVect2DFloat(*expIdx, intCpoPath, intPath, intTimeBasePath, data, *dim1, *dim2, *isTimed);
    *stat = status;

    free(intCpoPath);
    free(intPath);
    free(intTimeBasePath);
    return status;
}
int put_vect2d_double_(int *expIdx, char *cpoPath, char *path, char *timeBasePath, double *data, int *dim1, int *dim2, int *isTimed, int *stat, int cpoPathLen, int pathLen, int timeBasePathLen)
{
     int status;
    char *intCpoPath = allocateC(cpoPath, cpoPathLen);
    char *intPath = allocateC(path, pathLen);
    char *intTimeBasePath = allocateC(timeBasePath, timeBasePathLen);
    status =putVect2DDouble(*expIdx, intCpoPath, intPath, intTimeBasePath, data, *dim1, *dim2, *isTimed);
    *stat = status;

    free(intCpoPath);
    free(intPath);
    free(intTimeBasePath);
    return status;
}


int put_vect3d_int_(int *expIdx, char *cpoPath, char *path, char *timeBasePath, int *data, int *dim1, int *dim2, int *dim3, int *isTimed, int *stat, int cpoPathLen, int pathLen, int timeBasePathLen)
{
    int status;
    char *intCpoPath = allocateC(cpoPath, cpoPathLen);
    char *intPath = allocateC(path, pathLen);
    char *intTimeBasePath = allocateC(timeBasePath, timeBasePathLen);
    status =putVect3DInt(*expIdx, intCpoPath, intPath, intTimeBasePath, data, *dim1, *dim2, *dim3, *isTimed);
    *stat = status;

    free(intCpoPath);
    free(intPath);
    free(intTimeBasePath);
    return status;
}
int put_vect3d_float_(int *expIdx, char *cpoPath, char *path, char *timeBasePath, float *data, int *dim1, int *dim2, int *dim3, int *isTimed, int *stat, int cpoPathLen, int pathLen, int timeBasePathLen)
{
    int status;
    char *intCpoPath = allocateC(cpoPath, cpoPathLen);
    char *intPath = allocateC(path, pathLen);
    char *intTimeBasePath = allocateC(timeBasePath, timeBasePathLen);
    status =putVect3DFloat(*expIdx, intCpoPath, intPath, intTimeBasePath, data, *dim1, *dim2, *dim3, *isTimed);
    *stat = status;

    free(intCpoPath);
    free(intPath);
    free(intTimeBasePath);
    return status;
}
int put_vect3d_double_(int *expIdx, char *cpoPath, char *path, char *timeBasePath, double *data, int *dim1, int *dim2, int *dim3, int *isTimed, int *stat, int cpoPathLen, int pathLen, int timeBasePathLen)
{
    int status;
    char *intCpoPath = allocateC(cpoPath, cpoPathLen);
    char *intPath = allocateC(path, pathLen);
    char *intTimeBasePath = allocateC(timeBasePath, timeBasePathLen);
    status =putVect3DDouble(*expIdx, intCpoPath, intPath, intTimeBasePath, data, *dim1, *dim2, *dim3, *isTimed);
    *stat = status;

    free(intCpoPath);
    free(intPath);
    free(intTimeBasePath);
    return status;
}

int put_vect4d_double_(int *expIdx, char *cpoPath, char *path, char *timeBasePath, double *data, int *dim1, int *dim2, int *dim3, int *dim4, int *isTimed, int *stat, int cpoPathLen, int pathLen, int timeBasePathLen)
{
    int status;
    char *intCpoPath = allocateC(cpoPath, cpoPathLen);
    char *intPath = allocateC(path, pathLen);
    char *intTimeBasePath = allocateC(timeBasePath, timeBasePathLen);
    status =putVect4DDouble(*expIdx, intCpoPath, intPath, intTimeBasePath, data, *dim1, *dim2, *dim3, *dim4, *isTimed);
    *stat = status;

    free(intCpoPath);
    free(intPath);
    free(intTimeBasePath);
    return status;
}
int put_vect5d_double_(int *expIdx, char *cpoPath, char *path, char *timeBasePath, double *data, int *dim1, int *dim2, int *dim3,
    int *dim4, int *dim5, int *isTimed, int *stat, int cpoPathLen, int pathLen, int timeBasePathLen)
{
    int status;
    char *intCpoPath = allocateC(cpoPath, cpoPathLen);
    char *intPath = allocateC(path, pathLen);
    char *intTimeBasePath = allocateC(timeBasePath, timeBasePathLen);
    status =putVect5DDouble(*expIdx, intCpoPath, intPath, intTimeBasePath, data, *dim1, *dim2, *dim3, *dim4, *dim5, *isTimed);
    *stat = status;

    free(intCpoPath);
    free(intPath);
    free(intTimeBasePath);
    return status;
}
int put_vect6d_double_(int *expIdx, char *cpoPath, char *path, char *timeBasePath, double *data, int *dim1, int *dim2, int *dim3,
    int *dim4, int *dim5, int *dim6, int *isTimed, int *stat, int cpoPathLen, int pathLen, int timeBasePathLen)
{
    int status;
    char *intCpoPath = allocateC(cpoPath, cpoPathLen);
    char *intPath = allocateC(path, pathLen);
    char *intTimeBasePath = allocateC(timeBasePath, timeBasePathLen);
    status =putVect6DDouble(*expIdx, intCpoPath, intPath, intTimeBasePath, data, *dim1, *dim2, *dim3, *dim4, *dim5, *dim6, *isTimed);
    *stat = status;

    free(intCpoPath);
    free(intPath);
    free(intTimeBasePath);
    return status;
}
int put_vect7d_double_(int *expIdx, char *cpoPath, char *path, char *timeBasePath, double *data, int *dim1, int *dim2, int *dim3,
    int *dim4, int *dim5, int *dim6, int *dim7, int *isTimed, int *stat, int cpoPathLen, int pathLen, int timeBasePathLen)
{
    int status;
    char *intCpoPath = allocateC(cpoPath, cpoPathLen);
    char *intPath = allocateC(path, pathLen);
    char *intTimeBasePath = allocateC(timeBasePath, timeBasePathLen);
    status =putVect7DDouble(*expIdx, intCpoPath, intPath, intTimeBasePath, data, *dim1, *dim2, *dim3, *dim4, *dim5, *dim6, *dim7, *isTimed);
    *stat = status;

    free(intCpoPath);
    free(intPath);
    free(intTimeBasePath);
    return status;
}







int put_string_slice_(int *expIdx, char *cpoPath, char *path, char *timeBasePath, char *data, double *time, int *stat, int cpoPathLen, int pathLen, int timeBasePathLen, int dataLen)
{
    int status;
    char *intCpoPath = allocateC(cpoPath, cpoPathLen);
    char *intPath = allocateC(path, pathLen);
    char *intData = allocateC(data, dataLen);
    char *intTimeBasePath = allocateC(timeBasePath, timeBasePathLen);
    status = putStringSlice(*expIdx, intCpoPath, intPath, intTimeBasePath, intData, *time);
    *stat = status;

    free(intCpoPath);
    free(intPath);
    free(intTimeBasePath);
    free(intData);
    return status;
}

int put_int_slice_(int *expIdx, char *cpoPath, char *path, char *timeBasePath, int *data, double *time, int *stat, int cpoPathLen, int pathLen, int timeBasePathLen)
{
    int status;
    char *intCpoPath = allocateC(cpoPath, cpoPathLen);
    char *intPath = allocateC(path, pathLen);
    char *intTimeBasePath = allocateC(timeBasePath, timeBasePathLen);
    status = putIntSlice(*expIdx, intCpoPath, intPath, intTimeBasePath, *data, *time);
    *stat = status;

    free(intCpoPath);
    free(intPath);
    free(intTimeBasePath);
    return status;
}

int put_float_slice_(int *expIdx, char *cpoPath, char *path, char *timeBasePath, float *data, double *time, int *stat, int cpoPathLen, int pathLen, int timeBasePathLen)
{
    int status;
    char *intCpoPath = allocateC(cpoPath, cpoPathLen);
    char *intPath = allocateC(path, pathLen);
    char *intTimeBasePath = allocateC(timeBasePath, timeBasePathLen);
    status = putFloatSlice(*expIdx, intCpoPath, intPath, intTimeBasePath, *data, *time);
    *stat = status;

    free(intCpoPath);
    free(intPath);
    free(intTimeBasePath);
    return status;
}

int put_double_slice_(int *expIdx, char *cpoPath, char *path, char *timeBasePath, double *data, double *time, int *stat, int cpoPathLen, int pathLen, int timeBasePathLen)
{
    int status;
    char *intCpoPath = allocateC(cpoPath, cpoPathLen);
    char *intPath = allocateC(path, pathLen);
    char *intTimeBasePath = allocateC(timeBasePath, timeBasePathLen);
    status = putDoubleSlice(*expIdx, intCpoPath, intPath, intTimeBasePath, *data, *time);
    *stat = status;

    free(intCpoPath);
    free(intPath);
    free(intTimeBasePath);
    return status;
}

int put_vect1d_int_slice_(int *expIdx, char *cpoPath, char *path, char *timeBasePath, int *data, int *dim, double *time,  int *stat, int cpoPathLen, int pathLen, int timeBasePathLen)
{
     int status;
    char *intCpoPath = allocateC(cpoPath, cpoPathLen);
    char *intPath = allocateC(path, pathLen);
    char *intTimeBasePath = allocateC(timeBasePath, timeBasePathLen);
    status =putVect1DIntSlice(*expIdx, intCpoPath, intPath, intTimeBasePath, data, *dim, *time);
    *stat = status;

    free(intCpoPath);
    free(intPath);
    free(intTimeBasePath);
   return status;
}
int put_vect1d_float_slice_(int *expIdx, char *cpoPath, char *path, char *timeBasePath, float *data, int *dim, double *time, int *stat, int cpoPathLen, int pathLen, int timeBasePathLen)
{
     int status;
    char *intCpoPath = allocateC(cpoPath, cpoPathLen);
    char *intPath = allocateC(path, pathLen);
    char *intTimeBasePath = allocateC(timeBasePath, timeBasePathLen);
    status =putVect1DFloatSlice(*expIdx, intCpoPath, intPath, intTimeBasePath, data, *dim, *time);
    *stat = status;

    free(intCpoPath);
    free(intPath);
    free(intTimeBasePath);
    return status;
}

int put_vect1d_double_slice_(int *expIdx, char *cpoPath, char *path, char *timeBasePath, double *data, int *dim, double *time, int *stat, int cpoPathLen, int pathLen, int timeBasePathLen)
{
     int status;
    char *intCpoPath = allocateC(cpoPath, cpoPathLen);
    char *intPath = allocateC(path, pathLen);
    char *intTimeBasePath = allocateC(timeBasePath, timeBasePathLen);
    status =putVect1DDoubleSlice(*expIdx, intCpoPath, intPath, intTimeBasePath, data, *dim, *time);
    *stat = status;

    free(intCpoPath);
    free(intPath);
    free(intTimeBasePath);
    return status;
}

int put_vect2d_int_slice_(int *expIdx, char *cpoPath, char *path, char *timeBasePath, int *data, int *dim1, int *dim2, double *time, int *stat, int cpoPathLen, int pathLen, int timeBasePathLen)
{
     int status;
    char *intCpoPath = allocateC(cpoPath, cpoPathLen);
    char *intPath = allocateC(path, pathLen);
    char *intTimeBasePath = allocateC(timeBasePath, timeBasePathLen);
    status =putVect2DIntSlice(*expIdx, intCpoPath, intPath, intTimeBasePath, data, *dim1, *dim2, *time);
    *stat = status;

    free(intCpoPath);
    free(intPath);
    free(intTimeBasePath);
    return status;
}
int put_vect2d_float_slice_(int *expIdx, char *cpoPath, char *path, char *timeBasePath, float *data, int *dim1, int *dim2, double *time, int *stat, int cpoPathLen, int pathLen, int timeBasePathLen)
{
     int status;
    char *intCpoPath = allocateC(cpoPath, cpoPathLen);
    char *intPath = allocateC(path, pathLen);
    char *intTimeBasePath = allocateC(timeBasePath, timeBasePathLen);
    status = putVect2DFloatSlice(*expIdx, intCpoPath, intPath, intTimeBasePath, data, *dim1, *dim2, *time);
    *stat = status;

    free(intCpoPath);
    free(intPath);
    free(intTimeBasePath);
    return status;
}
int put_vect2d_double_slice_(int *expIdx, char *cpoPath, char *path, char *timeBasePath, double *data, int *dim1, int *dim2, double *time, int *stat, int cpoPathLen, int pathLen, int timeBasePathLen)
{
     int status;
    char *intCpoPath = allocateC(cpoPath, cpoPathLen);
    char *intPath = allocateC(path, pathLen);
    char *intTimeBasePath = allocateC(timeBasePath, timeBasePathLen);
    status =putVect2DDoubleSlice(*expIdx, intCpoPath, intPath, intTimeBasePath, data, *dim1, *dim2, *time);
    *stat = status;

    free(intCpoPath);
    free(intPath);
    free(intTimeBasePath);
    return status;
}


int put_vect3d_int_slice_(int *expIdx, char *cpoPath, char *path, char *timeBasePath, int *data, int *dim1, int *dim2, int *dim3, double *time, int *stat, int cpoPathLen, int pathLen, int timeBasePathLen)
{
    int status;
    char *intCpoPath = allocateC(cpoPath, cpoPathLen);
    char *intPath = allocateC(path, pathLen);
    char *intTimeBasePath = allocateC(timeBasePath, timeBasePathLen);
    status =putVect3DIntSlice(*expIdx, intCpoPath, intPath, intTimeBasePath, data, *dim1, *dim2, *dim3, *time);
    *stat = status;

    free(intCpoPath);
    free(intPath);
    free(intTimeBasePath);
    return status;
}
int put_vect3d_float_slice_(int *expIdx, char *cpoPath, char *path, char *timeBasePath, float *data, int *dim1, int *dim2, int *dim3, double *time, int *stat, int cpoPathLen, int pathLen, int timeBasePathLen)
{
    int status;
    char *intCpoPath = allocateC(cpoPath, cpoPathLen);
    char *intPath = allocateC(path, pathLen);
    char *intTimeBasePath = allocateC(timeBasePath, timeBasePathLen);
    status = putVect3DFloatSlice(*expIdx, intCpoPath, intPath, intTimeBasePath, data, *dim1, *dim2, *dim3, *time);
    *stat = status;

    free(intCpoPath);
    free(intPath);
    free(intTimeBasePath);
    return status;
}
int put_vect3d_double_slice_(int *expIdx, char *cpoPath, char *path, char *timeBasePath, double *data, int *dim1, int *dim2, int *dim3, double *time,  int *stat, int cpoPathLen, int pathLen, int timeBasePathLen)
{
    int status;
    char *intCpoPath = allocateC(cpoPath, cpoPathLen);
    char *intPath = allocateC(path, pathLen);
    char *intTimeBasePath = allocateC(timeBasePath, timeBasePathLen);
    status =putVect3DDoubleSlice(*expIdx, intCpoPath, intPath, intTimeBasePath, data, *dim1, *dim2, *dim3, *time);
    *stat = status;

    free(intCpoPath);
    free(intPath);
    free(intTimeBasePath);
    return status;
}

int put_vect4d_int_slice_(int *expIdx, char *cpoPath, char *path, char *timeBasePath, int *data, int *dim1, int *dim2, int *dim3,
    int *dim4, double *time, int *stat, int cpoPathLen, int pathLen, int timeBasePathLen)
{
    int status;
    char *intCpoPath = allocateC(cpoPath, cpoPathLen);
    char *intPath = allocateC(path, pathLen);
    char *intTimeBasePath = allocateC(timeBasePath, timeBasePathLen);
    status =putVect4DIntSlice(*expIdx, intCpoPath, intPath, intTimeBasePath, data, *dim1, *dim2, *dim3, *dim4, *time);
    *stat = status;

    free(intCpoPath);
    free(intPath);
    free(intTimeBasePath);
    return status;
}
int put_vect4d_float_slice_(int *expIdx, char *cpoPath, char *path, char *timeBasePath, float *data, int *dim1, int *dim2, int *dim3,
    int *dim4, double *time,  int *stat, int cpoPathLen, int pathLen, int timeBasePathLen)
{
    int status;
    char *intCpoPath = allocateC(cpoPath, cpoPathLen);
    char *intPath = allocateC(path, pathLen);
    char *intTimeBasePath = allocateC(timeBasePath, timeBasePathLen);
    status =putVect4DFloatSlice(*expIdx, intCpoPath, intPath, intTimeBasePath, data, *dim1, *dim2, *dim3, *dim4, *time);
    *stat = status;

    free(intCpoPath);
    free(intPath);
    free(intTimeBasePath);
    return status;
}
int put_vect4d_double_slice_(int *expIdx, char *cpoPath, char *path, char *timeBasePath, double *data, int *dim1, int *dim2, int *dim3,
    int *dim4, double *time, int *stat, int cpoPathLen, int pathLen, int timeBasePathLen)
{
    int status;
    char *intCpoPath = allocateC(cpoPath, cpoPathLen);
    char *intPath = allocateC(path, pathLen);
    char *intTimeBasePath = allocateC(timeBasePath, timeBasePathLen);
    status =putVect4DDoubleSlice(*expIdx, intCpoPath, intPath, intTimeBasePath, data, *dim1, *dim2, *dim3, *dim4, *time);
    *stat = status;

    free(intCpoPath);
    free(intPath);
    free(intTimeBasePath);
    return status;
}

int put_vect5d_int_slice_(int *expIdx, char *cpoPath, char *path, char *timeBasePath, int *data, int *dim1, int *dim2, int *dim3,
    int *dim4, int *dim5, double *time, int *stat, int cpoPathLen, int pathLen, int timeBasePathLen)
{
    int status;
    char *intCpoPath = allocateC(cpoPath, cpoPathLen);
    char *intPath = allocateC(path, pathLen);
    char *intTimeBasePath = allocateC(timeBasePath, timeBasePathLen);
    status =putVect5DIntSlice(*expIdx, intCpoPath, intPath, intTimeBasePath, data, *dim1, *dim2, *dim3, *dim4, *dim5, *time);
    *stat = status;

    free(intCpoPath);
    free(intPath);
    free(intTimeBasePath);
    return status;
}
int put_vect5d_float_slice_(int *expIdx, char *cpoPath, char *path, char *timeBasePath, float *data, int *dim1, int *dim2, int *dim3,
    int *dim4, int *dim5, double *time, int *stat, int cpoPathLen, int pathLen, int timeBasePathLen)
{
    int status;
    char *intCpoPath = allocateC(cpoPath, cpoPathLen);
    char *intPath = allocateC(path, pathLen);
    char *intTimeBasePath = allocateC(timeBasePath, timeBasePathLen);
    status =putVect5DFloatSlice(*expIdx, intCpoPath, intPath, intTimeBasePath, data, *dim1, *dim2, *dim3, *dim4, *dim5, *time);
    *stat = status;

    free(intCpoPath);
    free(intPath);
    free(intTimeBasePath);
    return status;
}
int put_vect5d_double_slice_(int *expIdx, char *cpoPath, char *path, char *timeBasePath, double *data, int *dim1, int *dim2, int *dim3,
    int *dim4, int *dim5, double *time,  int *stat, int cpoPathLen, int pathLen, int timeBasePathLen)
{
    int status;
    char *intCpoPath = allocateC(cpoPath, cpoPathLen);
    char *intPath = allocateC(path, pathLen);
    char *intTimeBasePath = allocateC(timeBasePath, timeBasePathLen);
    status =putVect5DDoubleSlice(*expIdx, intCpoPath, intPath, intTimeBasePath, data, *dim1, *dim2, *dim3, *dim4, *dim5, *time);
    *stat = status;

    free(intCpoPath);
    free(intPath);
    free(intTimeBasePath);
    return status;
}


int put_vect6d_int_slice_(int *expIdx, char *cpoPath, char *path, char *timeBasePath, int *data, int *dim1, int *dim2, int *dim3,
    int *dim4, int *dim5, int *dim6, double *time, int *stat, int cpoPathLen, int pathLen, int timeBasePathLen)
{
    int status;
    char *intCpoPath = allocateC(cpoPath, cpoPathLen);
    char *intPath = allocateC(path, pathLen);
    char *intTimeBasePath = allocateC(timeBasePath, timeBasePathLen);
    status =putVect6DIntSlice(*expIdx, intCpoPath, intPath, intTimeBasePath, data, *dim1, *dim2, *dim3, *dim4, *dim5, *dim6, *time);
    *stat = status;

    free(intCpoPath);
    free(intPath);
    free(intTimeBasePath);
    return status;
}
int put_vect6d_float_slice_(int *expIdx, char *cpoPath, char *path, char *timeBasePath, float *data, int *dim1, int *dim2, int *dim3,
    int *dim4, int *dim5, int *dim6, double *time,  int *stat, int cpoPathLen, int pathLen, int timeBasePathLen)
{
    int status;
    char *intCpoPath = allocateC(cpoPath, cpoPathLen);
    char *intPath = allocateC(path, pathLen);
    char *intTimeBasePath = allocateC(timeBasePath, timeBasePathLen);
    status =putVect6DFloatSlice(*expIdx, intCpoPath, intPath, intTimeBasePath, data, *dim1, *dim2, *dim3, *dim4, *dim5, *dim6, *time);
    *stat = status;

    free(intCpoPath);
    free(intPath);
    free(intTimeBasePath);
    return status;
}
int put_vect6d_double_slice_(int *expIdx, char *cpoPath, char *path, char *timeBasePath, double *data, int *dim1, int *dim2, int *dim3,
    int *dim4, int *dim5, int *dim6, double *time, int *stat, int cpoPathLen, int pathLen, int timeBasePathLen)
{
    int status;
    char *intCpoPath = allocateC(cpoPath, cpoPathLen);
    char *intPath = allocateC(path, pathLen);
    char *intTimeBasePath = allocateC(timeBasePath, timeBasePathLen);
    status =putVect6DDoubleSlice(*expIdx, intCpoPath, intPath, intTimeBasePath, data, *dim1, *dim2, *dim3, *dim4, *dim5, *dim6, *time);
    *stat = status;

    free(intCpoPath);
    free(intPath);
    free(intTimeBasePath);
    return status;
}





int replace_last_string_slice_(int *expIdx, char *cpoPath, char *path, char *data,  int *stat, int cpoPathLen, int pathLen, int dataLen)
{
    int status;
    char *intCpoPath = allocateC(cpoPath, cpoPathLen);
    char *intPath = allocateC(path, pathLen);
    char *intData = allocateC(data, dataLen);
    status = replaceLastStringSlice(*expIdx, intCpoPath, intPath, intData);
    *stat = status;

    free(intCpoPath);
    free(intPath);
    free(intData);
    return status;
}

int replace_last_int_slice_(int *expIdx, char *cpoPath, char *path, int *data, int *stat, int cpoPathLen, int pathLen)
{
    int status;
    char *intCpoPath = allocateC(cpoPath, cpoPathLen);
    char *intPath = allocateC(path, pathLen);
    status = replaceLastIntSlice(*expIdx, intCpoPath, intPath, *data);
    *stat = status;

    free(intCpoPath);
    free(intPath);
    return status;
}

int replace_last_float_slice_(int *expIdx, char *cpoPath, char *path, float *data,  int *stat, int cpoPathLen, int pathLen)
{
    int status;
    char *intCpoPath = allocateC(cpoPath, cpoPathLen);
    char *intPath = allocateC(path, pathLen);
    status = replaceLastFloatSlice(*expIdx, intCpoPath, intPath, *data);
    *stat = status;

    free(intCpoPath);
    free(intPath);
    return status;
}

int replace_last_double_slice_(int *expIdx, char *cpoPath, char *path, double *data, int *stat, int cpoPathLen, int pathLen)
{
    int status;
    char *intCpoPath = allocateC(cpoPath, cpoPathLen);
    char *intPath = allocateC(path, pathLen);
    status = replaceLastDoubleSlice(*expIdx, intCpoPath, intPath, *data);
    *stat = status;

    free(intCpoPath);
    free(intPath);
    return status;
}

int replace_last_vect1d_int_slice_(int *expIdx, char *cpoPath, char *path, int *data, int *dim,  int *stat, int cpoPathLen, int pathLen)
{
     int status;
    char *intCpoPath = allocateC(cpoPath, cpoPathLen);
    char *intPath = allocateC(path, pathLen);
    status = replaceLastVect1DIntSlice(*expIdx, intCpoPath, intPath, data, *dim);
    *stat = status;

    free(intCpoPath);
    free(intPath);
    return status;
}

int replace_last_vect1d_float_slice_(int *expIdx, char *cpoPath, char *path, float *data, int *dim, int *stat, int cpoPathLen, int pathLen)
{
     int status;
    char *intCpoPath = allocateC(cpoPath, cpoPathLen);
    char *intPath = allocateC(path, pathLen);
    status = replaceLastVect1DFloatSlice(*expIdx, intCpoPath, intPath, data, *dim);
    *stat = status;

    free(intCpoPath);
    free(intPath);
    return status;
}

int replace_last_vect1d_double_slice_(int *expIdx, char *cpoPath, char *path, double *data, int *dim,  int *stat, int cpoPathLen, int pathLen)
{
     int status;
    char *intCpoPath = allocateC(cpoPath, cpoPathLen);
    char *intPath = allocateC(path, pathLen);
    status = replaceLastVect1DDoubleSlice(*expIdx, intCpoPath, intPath, data, *dim);
    *stat = status;

    free(intCpoPath);
    free(intPath);
    return status;
}

int replace_last_vect2d_int_slice_(int *expIdx, char *cpoPath, char *path, int *data, int *dim1, int *dim2, int *stat, int cpoPathLen, int pathLen)
{
     int status;
    char *intCpoPath = allocateC(cpoPath, cpoPathLen);
    char *intPath = allocateC(path, pathLen);
    status = replaceLastVect2DIntSlice(*expIdx, intCpoPath, intPath, data, *dim1, *dim2);
    *stat = status;

    free(intCpoPath);
    free(intPath);
    return status;
}
int replace_last_vect2d_float_slice_(int *expIdx, char *cpoPath, char *path, float *data, int *dim1, int *dim2,  int *stat, int cpoPathLen, int pathLen)
{
     int status;
    char *intCpoPath = allocateC(cpoPath, cpoPathLen);
    char *intPath = allocateC(path, pathLen);
    status = replaceLastVect2DFloatSlice(*expIdx, intCpoPath, intPath, data, *dim1, *dim2);
    *stat = status;

    free(intCpoPath);
    free(intPath);
    return status;
}
int replace_last_vect2d_double_slice_(int *expIdx, char *cpoPath, char *path, double *data, int *dim1, int *dim2, int *stat, int cpoPathLen, int pathLen)
{
     int status;
    char *intCpoPath = allocateC(cpoPath, cpoPathLen);
    char *intPath = allocateC(path, pathLen);
    status = replaceLastVect2DDoubleSlice(*expIdx, intCpoPath, intPath, data, *dim1, *dim2);
    *stat = status;

    free(intCpoPath);
    free(intPath);
    return status;
}


int replace_last_vect3d_int_slice_(int *expIdx, char *cpoPath, char *path, int *data, int *dim1, int *dim2, int *dim3,  int *stat, int cpoPathLen, int pathLen)
{
    int status;
    char *intCpoPath = allocateC(cpoPath, cpoPathLen);
    char *intPath = allocateC(path, pathLen);
    status = replaceLastVect3DIntSlice(*expIdx, intCpoPath, intPath, data, *dim1, *dim2, *dim3);
    *stat = status;

    free(intCpoPath);
    free(intPath);
    return status;
}
int replace_last_vect3d_float_slice_(int *expIdx, char *cpoPath, char *path, float *data, int *dim1, int *dim2, int *dim3, int *stat, int cpoPathLen, int pathLen)
{
    int status;
    char *intCpoPath = allocateC(cpoPath, cpoPathLen);
    char *intPath = allocateC(path, pathLen);
    status = replaceLastVect3DFloatSlice(*expIdx, intCpoPath, intPath, data, *dim1, *dim2, *dim3);
    *stat = status;

    free(intCpoPath);
    free(intPath);
    return status;
}
int replace_last_vect3d_double_slice_(int *expIdx, char *cpoPath, char *path, double *data, int *dim1, int *dim2, int *dim3,  int *stat, int cpoPathLen, int pathLen)
{
    int status;
    char *intCpoPath = allocateC(cpoPath, cpoPathLen);
    char *intPath = allocateC(path, pathLen);
    status = replaceLastVect3DDoubleSlice(*expIdx, intCpoPath, intPath, data, *dim1, *dim2, *dim3);
    *stat = status;

    free(intCpoPath);
    free(intPath);
    return status;
}

int replace_last_vect4d_int_slice_(int *expIdx, char *cpoPath, char *path, int *data, int *dim1, int *dim2, int *dim3,
    int *dim4, int *stat, int cpoPathLen, int pathLen)
{
    int status;
    char *intCpoPath = allocateC(cpoPath, cpoPathLen);
    char *intPath = allocateC(path, pathLen);
    status = replaceLastVect4DIntSlice(*expIdx, intCpoPath, intPath, data, *dim1, *dim2, *dim3, *dim4);
    *stat = status;

    free(intCpoPath);
    free(intPath);
    return status;
}
int replace_last_vect4d_float_slice_(int *expIdx, char *cpoPath, char *path, float *data, int *dim1, int *dim2,
    int *dim3, int *dim4,  int *stat, int cpoPathLen, int pathLen)
{
    int status;
    char *intCpoPath = allocateC(cpoPath, cpoPathLen);
    char *intPath = allocateC(path, pathLen);
    status = replaceLastVect4DFloatSlice(*expIdx, intCpoPath, intPath, data, *dim1, *dim2, *dim3, *dim4);
    *stat = status;

    free(intCpoPath);
    free(intPath);
    return status;
}
int replace_last_vect4d_double_slice_(int *expIdx, char *cpoPath, char *path, double *data, int *dim1, int *dim2,
    int *dim3, int *dim4, int *stat, int cpoPathLen, int pathLen)
{
    int status;
    char *intCpoPath = allocateC(cpoPath, cpoPathLen);
    char *intPath = allocateC(path, pathLen);
    status = replaceLastVect4DDoubleSlice(*expIdx, intCpoPath, intPath, data, *dim1, *dim2, *dim3, *dim4);
    *stat = status;

    free(intCpoPath);
    free(intPath);
    return status;
}
int replace_last_vect5d_int_slice_(int *expIdx, char *cpoPath, char *path, int *data, int *dim1, int *dim2, int *dim3,
    int *dim4, int *dim5,  int *stat, int cpoPathLen, int pathLen)
{
    int status;
    char *intCpoPath = allocateC(cpoPath, cpoPathLen);
    char *intPath = allocateC(path, pathLen);
    status = replaceLastVect5DIntSlice(*expIdx, intCpoPath, intPath, data, *dim1, *dim2, *dim3, *dim4, *dim5);
    *stat = status;

    free(intCpoPath);
    free(intPath);
    return status;
}
int replace_last_vect5d_float_slice_(int *expIdx, char *cpoPath, char *path, float *data, int *dim1, int *dim2,
    int *dim3, int *dim4, int *dim5, int *stat, int cpoPathLen, int pathLen)
{
    int status;
    char *intCpoPath = allocateC(cpoPath, cpoPathLen);
    char *intPath = allocateC(path, pathLen);
    status = replaceLastVect5DFloatSlice(*expIdx, intCpoPath, intPath, data, *dim1, *dim2, *dim3, *dim4, *dim5);
    *stat = status;

    free(intCpoPath);
    free(intPath);
    return status;
}
int replace_last_vect5d_double_slice_(int *expIdx, char *cpoPath, char *path, double *data, int *dim1, int *dim2,
    int *dim3, int *dim4, int *dim5,  int *stat, int cpoPathLen, int pathLen)
{
    int status;
    char *intCpoPath = allocateC(cpoPath, cpoPathLen);
    char *intPath = allocateC(path, pathLen);
    status = replaceLastVect5DDoubleSlice(*expIdx, intCpoPath, intPath, data, *dim1, *dim2, *dim3, *dim4, *dim5);
    *stat = status;

    free(intCpoPath);
    free(intPath);
    return status;
}



int replace_last_vect6d_int_slice_(int *expIdx, char *cpoPath, char *path, int *data, int *dim1, int *dim2, int *dim3,
    int *dim4, int *dim5, int *dim6, int *stat, int cpoPathLen, int pathLen)
{
    int status;
    char *intCpoPath = allocateC(cpoPath, cpoPathLen);
    char *intPath = allocateC(path, pathLen);
    status = replaceLastVect6DIntSlice(*expIdx, intCpoPath, intPath, data, *dim1, *dim2, *dim3, *dim4, *dim5, *dim6);
    *stat = status;

    free(intCpoPath);
    free(intPath);
    return status;
}
int replace_last_vect6d_float_slice_(int *expIdx, char *cpoPath, char *path, float *data, int *dim1, int *dim2,
    int *dim3, int *dim4, int *dim5, int *dim6,  int *stat, int cpoPathLen, int pathLen)
{
    int status;
    char *intCpoPath = allocateC(cpoPath, cpoPathLen);
    char *intPath = allocateC(path, pathLen);
    status = replaceLastVect6DFloatSlice(*expIdx, intCpoPath, intPath, data, *dim1, *dim2, *dim3, *dim4, *dim5, *dim6);
    *stat = status;

    free(intCpoPath);
    free(intPath);
    return status;
}
int replace_last_vect6d_double_slice_(int *expIdx, char *cpoPath, char *path, double *data, int *dim1, int *dim2,
    int *dim3, int *dim4, int *dim5, int *dim6, int *stat, int cpoPathLen, int pathLen)
{
    int status;
    char *intCpoPath = allocateC(cpoPath, cpoPathLen);
    char *intPath = allocateC(path, pathLen);
    status = replaceLastVect6DDoubleSlice(*expIdx, intCpoPath, intPath, data, *dim1, *dim2, *dim3, *dim4, *dim5, *dim6);
    *stat = status;

    free(intCpoPath);
    free(intPath);
    return status;
}








int get_dimension_(int *expIdx, char *cpoPath, char *path, int *numDims, int *dim1, int *dim2, int *dim3, int *dim4, int *dim5, int *dim6, int *dim7, int cpoPathLen, int pathLen)
{
    int status;
    char *intCpoPath = allocateC(cpoPath, cpoPathLen);
    char *intPath = allocateC(path, pathLen);
    status = getDimension(*expIdx, intCpoPath, intPath, numDims, dim1, dim2, dim3, dim4, dim5, dim6, dim7);
    free(intCpoPath);
    free(intPath);
    return status;
}

int get_string_(int *expIdx, char *cpoPath, char *path, char *data, int *stat, int cpoPathLen, int pathLen, int dataLen)
{
    int status, i, len;
    char *intCpoPath = allocateC(cpoPath, cpoPathLen);
    char *intPath = allocateC(path, pathLen);
    char *intData = NULL;
    status = getString(*expIdx, intCpoPath, intPath, &intData);
    *stat = status;
    free(intCpoPath);
    free(intPath);
    if(status) return status;

    if (intData) {
        len = strlen(intData);
        for(i = 0; i < len && i < dataLen; i++)
            data[i] = intData[i];
        /* for(;i < dataLen; i++) Not necessary to pad end of the string with blanks (rapidly tested)
           data[i] = ' ';  */
        free(intData);
    }
    return status;

}
int get_int_(int *expIdx, char *cpoPath, char *path, int *data, int *stat, int cpoPathLen, int pathLen)
{
    int status;
    char *intCpoPath = allocateC(cpoPath, cpoPathLen);
    char *intPath = allocateC(path, pathLen);
    status = getInt(*expIdx, intCpoPath, intPath, data);
    *stat = status;
    free(intCpoPath);
    free(intPath);
    return status;
}

int get_double_(int *expIdx, char *cpoPath, char *path, double *data, int *stat, int cpoPathLen, int pathLen)
{
    int status;
    char *intCpoPath = allocateC(cpoPath, cpoPathLen);
    char *intPath = allocateC(path, pathLen);
    status = getDouble(*expIdx, intCpoPath, intPath, data);
    *stat = status;
    free(intCpoPath);
    free(intPath);
    return status;
}
int get_vect1d_int_(int *expIdx, char *cpoPath, char *path, int *data, int *dim, int *retDim, int *stat, int cpoPathLen, int pathLen)
{
    int status;
    char *intCpoPath = allocateC(cpoPath, cpoPathLen);
    char *intPath = allocateC(path, pathLen);
    int *intData = NULL;
    int outDim;
    status = getVect1DInt(*expIdx, intCpoPath, intPath, &intData, &outDim);
    *stat = status;
    free(intCpoPath);
    free(intPath);
    if(status) return status;
    *retDim = outDim;
    if(*retDim > *dim)
        *retDim = *dim;

    if (intData) {
        memcpy(data, intData, *retDim * sizeof(int));
        free((char *)intData);
    }
    return status;
}
int get_vect1d_float_(int *expIdx, char *cpoPath, char *path, float *data, int *dim, int *retDim, int *stat, int cpoPathLen, int pathLen)
{
    int status;
    char *intCpoPath = allocateC(cpoPath, cpoPathLen);
    char *intPath = allocateC(path, pathLen);
    float *intData;
    int outDim;
    status = getVect1DFloat(*expIdx, intCpoPath, intPath, &intData, &outDim);
    *stat = status;
    free(intCpoPath);
    free(intPath);
    if(status) return status;
    *retDim = outDim;
    if(*retDim > *dim)
        *retDim = *dim;

    memcpy(data, intData, *retDim * sizeof(float));
    free((char *)intData);
    return status;
}
int get_vect1d_double_(int *expIdx, char *cpoPath, char *path, double *data, int *dim, int *retDim, int *stat, int cpoPathLen, int pathLen)
{
    int status;
    char *intCpoPath = allocateC(cpoPath, cpoPathLen);
    char *intPath = allocateC(path, pathLen);
    double *intData = NULL;
    int outDim;
    status = getVect1DDouble(*expIdx, intCpoPath, intPath, &intData, &outDim);
    *stat = status;
    free(intCpoPath);
    free(intPath);
    if(status) return status;
    *retDim = outDim;
    if(*retDim > *dim)
        *retDim = *dim;

    if (intData) {
        memcpy(data, intData, *retDim * sizeof(double));
        free((char *)intData);
    }
    return status;
}
int get_vect1d_string_(int *expIdx, char *cpoPath, char *path, char *data, int *dim, int *retDim, int *stat, int cpoPathLen, int pathLen)
{
    int status;
    char *intCpoPath = allocateC(cpoPath, cpoPathLen);
    char *intPath = allocateC(path, pathLen);
    char **intData;
    int outDim;
    int i,j;

    status = getVect1DString(*expIdx, intCpoPath, intPath, &intData, &outDim);
    *stat = status;

    free(intCpoPath);
    free(intPath);
    if(status) return status;
    *retDim = outDim;
    if(*retDim > *dim)
        *retDim = *dim;

    for(i = 0; i < *dim; i++)
    {

        for(j = 0; j < strlen(intData[i]) && j < 132; j++)
            data[i*132+j] = intData[i][j];

        for(;j < 132; j++)   /* necessary to pad the end of the string with blanks for string vectors, tested */
            data[i*132+j] = ' ';

       /*printf("Line %d : %s\n",i,intData[i]);
       printf("Line length %d : %d\n",i,strlen(intData[i])); */
    }

    for(i = 0; i < *dim; i++) free(intData[i]);
    free(intData);
    return status;
}
int get_vect2d_int_(int *expIdx, char *cpoPath, char *path, int *data, int *dim1, int *dim2, int *retDim1, int *retDim2, int *stat, int cpoPathLen, int pathLen)
{
    int status, i, j;
    char *intCpoPath = allocateC(cpoPath, cpoPathLen);
    char *intPath = allocateC(path, pathLen);
    int *intData;
    int outDim1, outDim2;
    status = getVect2DInt(*expIdx, intCpoPath, intPath, &intData, &outDim1, &outDim2);
    *stat = status;
    free(intCpoPath);
    free(intPath);
    if(status) return status;
    *retDim1 = outDim1;
    *retDim2 = outDim2;
    if(*retDim1 > *dim1)
        *retDim1 = *dim1;
    if(*retDim2 > *dim2)
        *retDim2 = *dim2;

    for(i = 0; i < *retDim1; i++)
    {
        for(j = 0; j < *retDim2; j++)
        {
            data[i* outDim2 + j] = intData[i* outDim2 + j];
        }
    }
    free((char *)intData);
    return status;
}
int get_vect2d_float_(int *expIdx, char *cpoPath, char *path, float *data, int *dim1, int *dim2, int *retDim1, int *retDim2, int *stat, int cpoPathLen, int pathLen)
{
    int status, i, j;
    char *intCpoPath = allocateC(cpoPath, cpoPathLen);
    char *intPath = allocateC(path, pathLen);
    float *intData;
    int outDim1, outDim2;
    status = getVect2DFloat(*expIdx, intCpoPath, intPath, &intData, &outDim1, &outDim2);
    *stat = status;
    free(intCpoPath);
    free(intPath);
    if(status) return status;
    *retDim1 = outDim1;
    *retDim2 = outDim2;
    if(*retDim1 > *dim1)
        *retDim1 = *dim1;
    if(*retDim2 > *dim2)
        *retDim2 = *dim2;

    for(i = 0; i < *retDim1; i++)
    {
        for(j = 0; j < *retDim2; j++)
        {
            data[i* outDim2 + j] = intData[i* outDim2 + j];
        }
    }
    free((char *)intData);
    return status;
}
int get_vect2d_double_(int *expIdx, char *cpoPath, char *path, double *data, int *dim1, int *dim2, int *retDim1, int *retDim2, int *stat, int cpoPathLen, int pathLen)
{
    int status, i, j;
    char *intCpoPath = allocateC(cpoPath, cpoPathLen);
    char *intPath = allocateC(path, pathLen);
    double *intData;
    int outDim1, outDim2;
    status = getVect2DDouble(*expIdx, intCpoPath, intPath, &intData, &outDim1, &outDim2);
    *stat = status;
    free(intCpoPath);
    free(intPath);
    if(status) return status;
    *retDim1 = outDim1;
    *retDim2 = outDim2;
    if(*retDim1 > *dim1)
        *retDim1 = *dim1;
    if(*retDim2 > *dim2)
        *retDim2 = *dim2;

    for(i = 0; i < *retDim1; i++)
    {
        for(j = 0; j < *retDim2; j++)
        {
            data[i* outDim2 + j] = intData[i* outDim2 + j];
        }
    }
    free((char *)intData);
    return status;
}


int get_vect3d_int_(int *expIdx, char *cpoPath, char *path, int *data, int *dim1, int *dim2, int *dim3,
    int *retDim1, int *retDim2, int *retDim3, int *stat, int cpoPathLen, int pathLen)
{
    int status, i, j, k;
    char *intCpoPath = allocateC(cpoPath, cpoPathLen);
    char *intPath = allocateC(path, pathLen);
    int *intData;
    int outDim1, outDim2, outDim3;
    status = getVect3DInt(*expIdx, intCpoPath, intPath, &intData, &outDim1, &outDim2, &outDim3);
    *stat = status;
    free(intCpoPath);
    free(intPath);
    if(status) return status;
    *retDim1 = outDim1;
    *retDim2 = outDim2;
    *retDim3 = outDim3;
    if(*retDim1 > *dim1)
        *retDim1 = *dim1;
    if(*retDim2 > *dim2)
        *retDim2 = *dim2;
    if(*retDim3 > *dim3)
        *retDim3 = *dim3;

    for(i = 0; i < *retDim1; i++)
    {
        for(j = 0; j < *retDim2; j++)
        {
            for(k = 0; k < *retDim3; k++)
                data[i* outDim2 * outDim3 + j*outDim3 + k] = intData[i* outDim2 * outDim3 + j*outDim3 + k];
        }
    }
    free((char *)intData);
    return status;
}

int get_vect3d_float_(int *expIdx, char *cpoPath, char *path, float *data, int *dim1, int *dim2, int *dim3,
    int *retDim1, int *retDim2, int *retDim3, int *stat, int cpoPathLen, int pathLen)
{
    int status, i, j, k;
    char *intCpoPath = allocateC(cpoPath, cpoPathLen);
    char *intPath = allocateC(path, pathLen);
    float *intData;
    int outDim1, outDim2, outDim3;
    status = getVect3DFloat(*expIdx, intCpoPath, intPath, &intData, &outDim1, &outDim2, &outDim3);
    *stat = status;
    free(intCpoPath);
    free(intPath);
    if(status) return status;
    *retDim1 = outDim1;
    *retDim2 = outDim2;
    *retDim3 = outDim3;
    if(*retDim1 > *dim1)
        *retDim1 = *dim1;
    if(*retDim2 > *dim2)
        *retDim2 = *dim2;
    if(*retDim3 > *dim3)
        *retDim3 = *dim3;

    for(i = 0; i < *retDim1; i++)
    {
        for(j = 0; j < *retDim2; j++)
        {
            for(k = 0; k < *retDim3; k++)
                data[i* outDim2 * outDim3 + j*outDim3 + k] = intData[i* outDim2 * outDim3 + j*outDim3 + k];
        }
    }
    free((char *)intData);
    return status;
}


int get_vect3d_double_(int *expIdx, char *cpoPath, char *path, double *data, int *dim1, int *dim2, int *dim3,
    int *retDim1, int *retDim2, int *retDim3, int *stat, int cpoPathLen, int pathLen)
{
    int status, i, j, k;
    char *intCpoPath = allocateC(cpoPath, cpoPathLen);
    char *intPath = allocateC(path, pathLen);
    double *intData;
    int outDim1, outDim2, outDim3;
    status = getVect3DDouble(*expIdx, intCpoPath, intPath, &intData, &outDim1, &outDim2, &outDim3);
    *stat = status;
    free(intCpoPath);
    free(intPath);
    if(status) return status;
    *retDim1 = outDim1;
    *retDim2 = outDim2;
    *retDim3 = outDim3;
    if(*retDim1 > *dim1)
        *retDim1 = *dim1;
    if(*retDim2 > *dim2)
        *retDim2 = *dim2;
    if(*retDim3 > *dim3)
        *retDim3 = *dim3;

    for(i = 0; i < *retDim1; i++)
    {
        for(j = 0; j < *retDim2; j++)
        {
            for(k = 0; k < *retDim3; k++)
                data[i* outDim2 * outDim3 + j*outDim3 + k] = intData[i* outDim2 * outDim3 + j*outDim3 + k];
        }
    }
    free((char *)intData);
    return status;
}

int get_vect4d_double_(int *expIdx, char *cpoPath, char *path, double *data, int *dim1, int *dim2, int *dim3,
    int *dim4, int *retDim1, int *retDim2, int *retDim3, int *retDim4, int *stat, int cpoPathLen, int pathLen)
{
    int status, i, j, k, l;
    char *intCpoPath = allocateC(cpoPath, cpoPathLen);
    char *intPath = allocateC(path, pathLen);
    double *intData;
    int outDim1, outDim2, outDim3, outDim4;
    status = getVect4DDouble(*expIdx, intCpoPath, intPath, &intData, &outDim1, &outDim2, &outDim3, &outDim4);
    *stat = status;
    free(intCpoPath);
    free(intPath);
    if(status) return status;
    *retDim1 = outDim1;
    *retDim2 = outDim2;
    *retDim3 = outDim3;
    *retDim4 = outDim4;
    if(*retDim1 > *dim1)
        *retDim1 = *dim1;
    if(*retDim2 > *dim2)
        *retDim2 = *dim2;
    if(*retDim3 > *dim3)
        *retDim3 = *dim3;
    if(*retDim4 > *dim4)
        *retDim4 = *dim4;

    for(i = 0; i < *retDim1; i++)
    {
        for(j = 0; j < *retDim2; j++)
        {
            for(k = 0; k < *retDim3; k++)
	    {
	        for(l = 0; l < *retDim4; l++)
		{
                   data[i* outDim2 * outDim3 * outDim4 + j*outDim3 *outDim4 + k*outDim4 + l] =
		    intData[i* outDim2 * outDim3 * outDim4 + j*outDim3 *outDim4 + k*outDim4 + l];
		}
	    }
        }
    }
    free((char *)intData);
    return status;
}
int get_vect5d_double_(int *expIdx, char *cpoPath, char *path, double *data, int *dim1, int *dim2, int *dim3,
    int *dim4, int *dim5, int *retDim1, int *retDim2, int *retDim3, int *retDim4, int *retDim5, int *stat, int cpoPathLen, int pathLen)
{
    int status, i, j, k, l, m;
    char *intCpoPath = allocateC(cpoPath, cpoPathLen);
    char *intPath = allocateC(path, pathLen);
    double *intData;
    int outDim1, outDim2, outDim3, outDim4, outDim5;
    status = getVect5DDouble(*expIdx, intCpoPath, intPath, &intData, &outDim1, &outDim2, &outDim3, &outDim4, &outDim5);
    *stat = status;
    free(intCpoPath);
    free(intPath);
    if(status) return status;
    *retDim1 = outDim1;
    *retDim2 = outDim2;
    *retDim3 = outDim3;
    *retDim4 = outDim4;
    *retDim5 = outDim5;
    if(*retDim1 > *dim1)
        *retDim1 = *dim1;
    if(*retDim2 > *dim2)
        *retDim2 = *dim2;
    if(*retDim3 > *dim3)
        *retDim3 = *dim3;
    if(*retDim4 > *dim4)
        *retDim4 = *dim4;
    if(*retDim5 > *dim5)
        *retDim5 = *dim5;

    for(i = 0; i < *retDim1; i++)
    {
        for(j = 0; j < *retDim2; j++)
        {
            for(k = 0; k < *retDim3; k++)
	    {
	        for(l = 0; l < *retDim4; l++)
		{
                    for(m = 0; m < *retDim5; m++)
                    {
                        data[i* outDim2 * outDim3 * outDim4 * outDim5 + j*outDim3 *outDim4*outDim5 + k*outDim4 * outDim5 +
                            l * outDim5 + m] =
                            intData[i* outDim2 * outDim3 * outDim4 * outDim5 + j*outDim3 *outDim4*outDim5 + k*outDim4 * outDim5 +
                            l * outDim5 + m];
                    }
		}
	    }
        }
    }
    free((char *)intData);
    return status;
}

int get_vect6d_double_(int *expIdx, char *cpoPath, char *path, double *data, int *dim1, int *dim2, int *dim3,
    int *dim4, int *dim5, int *dim6, int *retDim1, int *retDim2, int *retDim3, int *retDim4, int *retDim5,
    int *retDim6, int *stat, int cpoPathLen, int pathLen)
{
    int status, i, j, k, l, m, n;
    char *intCpoPath = allocateC(cpoPath, cpoPathLen);
    char *intPath = allocateC(path, pathLen);
    double *intData;
    int outDim1, outDim2, outDim3, outDim4, outDim5, outDim6;
    status = getVect6DDouble(*expIdx, intCpoPath, intPath, &intData, &outDim1, &outDim2, &outDim3, &outDim4, &outDim5, &outDim6);
    *stat = status;
    free(intCpoPath);
    free(intPath);
    if(status) return status;
    *retDim1 = outDim1;
    *retDim2 = outDim2;
    *retDim3 = outDim3;
    *retDim4 = outDim4;
    *retDim5 = outDim5;
    *retDim6 = outDim6;
    if(*retDim1 > *dim1)
        *retDim1 = *dim1;
    if(*retDim2 > *dim2)
        *retDim2 = *dim2;
    if(*retDim3 > *dim3)
        *retDim3 = *dim3;
    if(*retDim4 > *dim4)
        *retDim4 = *dim4;
    if(*retDim5 > *dim5)
        *retDim5 = *dim5;
    if(*retDim6 > *dim6)
        *retDim6 = *dim6;

    for(i = 0; i < *retDim1; i++)
    {
        for(j = 0; j < *retDim2; j++)
        {
            for(k = 0; k < *retDim3; k++)
	    {
	        for(l = 0; l < *retDim4; l++)
		{
                    for(m = 0; m < *retDim5; m++)
                    {
                        for(n = 0; n < *retDim6; n++)
                        {
                         data[i* outDim2 * outDim3 * outDim4 * outDim5 * outDim6 + j*outDim3 *outDim4*outDim5 * outDim6
                            + k*outDim4 * outDim5 *outDim6 + l * outDim5 * outDim6 + m*outDim6 + n] =
                            intData[i* outDim2 * outDim3 * outDim4 * outDim5 * outDim6 + j*outDim3 *outDim4*outDim5 * outDim6
                            + k*outDim4 * outDim5 *outDim6 + l * outDim5 * outDim6 + m*outDim6 + n];
                         }
                    }
		}
	    }
        }
    }
    free((char *)intData);
    return status;
}

int get_vect7d_double_(int *expIdx, char *cpoPath, char *path, double *data, int *dim1, int *dim2, int *dim3,
    int *dim4, int *dim5, int *dim6, int *dim7, int *retDim1, int *retDim2, int *retDim3, int *retDim4, int *retDim5,
    int *retDim6, int *retDim7, int *stat, int cpoPathLen, int pathLen)
{
    int status, i, j, k, l, m, n, p;
    char *intCpoPath = allocateC(cpoPath, cpoPathLen);
    char *intPath = allocateC(path, pathLen);
    double *intData;
    int outDim1, outDim2, outDim3, outDim4, outDim5, outDim6, outDim7;
    status = getVect7DDouble(*expIdx, intCpoPath, intPath, &intData, &outDim1, &outDim2, &outDim3, &outDim4, &outDim5, &outDim6,
    	&outDim7);
    *stat = status;
    free(intCpoPath);
    free(intPath);
    if(status) return status;
    *retDim1 = outDim1;
    *retDim2 = outDim2;
    *retDim3 = outDim3;
    *retDim4 = outDim4;
    *retDim5 = outDim5;
    *retDim6 = outDim6;
    *retDim7 = outDim7;
    if(*retDim1 > *dim1)
        *retDim1 = *dim1;
    if(*retDim2 > *dim2)
        *retDim2 = *dim2;
    if(*retDim3 > *dim3)
        *retDim3 = *dim3;
    if(*retDim4 > *dim4)
        *retDim4 = *dim4;
    if(*retDim5 > *dim5)
        *retDim5 = *dim5;
    if(*retDim6 > *dim6)
        *retDim6 = *dim6;
    if(*retDim7 > *dim7)
        *retDim7 = *dim7;

    for(i = 0; i < *retDim1; i++)
    {
        for(j = 0; j < *retDim2; j++)
        {
            for(k = 0; k < *retDim3; k++)
	    {
	        for(l = 0; l < *retDim4; l++)
		{
                    for(m = 0; m < *retDim5; m++)
                    {
                        for(n = 0; n < *retDim6; n++)
                        {
                       	    for(p = 0; p < *retDim7; p++)
                            {
                         	data[i* outDim2 * outDim3 * outDim4 * outDim5 * outDim6 * outDim7 + j*outDim3 *outDim4*outDim5 * outDim6 * outDim7
                            	  + k*outDim4 * outDim5 *outDim6 * outDim7 + l * outDim5 * outDim6 *outDim7 + m*outDim6 * outDim7  + n * outDim7 + p] =
                            	intData[i* outDim2 * outDim3 * outDim4 * outDim5 * outDim6 * outDim7 + j*outDim3 *outDim4*outDim5 * outDim6 * outDim7
                            	  + k*outDim4 * outDim5 *outDim6 * outDim7 + l * outDim5 * outDim6 * outDim7 + m*outDim6  * outDim7+ n* outDim7 + p];
			    }
                        }
                    }
		}
	    }
        }
    }
    free((char *)intData);
    return status;
}

int get_int_slice_ (int *expIdx, char *cpoPath, char *path, char *timeBasePath, int *data, double *time, double *retTime,
    int *interpolMode, int *stat, int cpoPathLen, int pathLen, int timeBasePathLen)
{
    int status;
    char *intCpoPath = allocateC(cpoPath, cpoPathLen);
    char *intPath = allocateC(path, pathLen);
    char *intTimeBasePath = allocateC(timeBasePath, timeBasePathLen);
    status = getIntSlice(*expIdx, intCpoPath, intPath, intTimeBasePath, data, *time, retTime, *interpolMode);
    *stat = status;
    free(intCpoPath);
    free(intPath);
    free(intTimeBasePath);
    return status;
}

int get_string_slice_ (int *expIdx, char *cpoPath, char *path, char *timeBasePath, char *data, double *time, double *retTime,
    int *interpolMode, int *stat, int cpoPathLen, int pathLen, int timeBasePathLen, int dataLen)
{
/* was added for IMAS, not fully tested yet */
    int status, i, len;
    char *intCpoPath = allocateC(cpoPath, cpoPathLen);
    char *intPath = allocateC(path, pathLen);
    char *intTimeBasePath = allocateC(timeBasePath, timeBasePathLen);
    char *intData = NULL;
    status = getStringSlice(*expIdx, intCpoPath, intPath, intTimeBasePath, &intData, *time, retTime, *interpolMode);
    *stat = status;
    free(intCpoPath);
    free(intPath);
    free(intTimeBasePath);
    if(status) return status;
    if (intData) {
        len = strlen(intData);
        for(i = 0; i < len && i < dataLen; i++)
            data[i] = intData[i];
        /* for(;i < dataLen; i++) Not necessary to pad end of the string with blanks (rapidly tested)
           data[i] = ' ';  */
        free(intData);
    }



    return status;
}

int get_float_slice_ (int *expIdx, char *cpoPath, char *path, char *timeBasePath, float *data, double *time, double *retTime,
    int *interpolMode, int *stat, int cpoPathLen, int pathLen, int timeBasePathLen)
{
    int status;
    char *intCpoPath = allocateC(cpoPath, cpoPathLen);
    char *intPath = allocateC(path, pathLen);
    char *intTimeBasePath = allocateC(timeBasePath, timeBasePathLen);
    status = getFloatSlice(*expIdx, intCpoPath, intPath, intTimeBasePath, data, *time, retTime, *interpolMode);
    *stat = status;
    free(intCpoPath);
    free(intPath);
    free(intTimeBasePath);
    return status;
}

int get_double_slice_ (int *expIdx, char *cpoPath, char *path, char *timeBasePath, double *data, double *time, double *retTime,
    int *interpolMode, int *stat, int cpoPathLen, int pathLen, int timeBasePathLen)
{
    int status;
    char *intCpoPath = allocateC(cpoPath, cpoPathLen);
    char *intPath = allocateC(path, pathLen);
    char *intTimeBasePath = allocateC(timeBasePath, timeBasePathLen);
    status = getDoubleSlice(*expIdx, intCpoPath, intPath, intTimeBasePath, data, *time, retTime, *interpolMode);
    *stat = status;
    free(intCpoPath);
    free(intPath);
    free(intTimeBasePath);
    return status;
}
int get_vect1d_int_slice_(int *expIdx, char *cpoPath, char *path, char *timeBasePath, int *data, int *dim, int *retDim,
        double *time, double *retTime, int *interpolMode, int *stat, int cpoPathLen, int pathLen, int timeBasePathLen)
{
    int status;
    char *intCpoPath = allocateC(cpoPath, cpoPathLen);
    char *intPath = allocateC(path, pathLen);
    char *intTimeBasePath = allocateC(timeBasePath, timeBasePathLen);
    int *intData;
    int outDim;
    status = getVect1DIntSlice(*expIdx, intCpoPath, intPath, intTimeBasePath, &intData, &outDim, *time, retTime, *interpolMode);
    *stat = status;
    free(intCpoPath);
    free(intPath);
    free(intTimeBasePath);
    if(status) return status;
    *retDim = outDim;
    if(*retDim > *dim)
        *retDim = *dim;

    memcpy(data, intData, *retDim * sizeof(int));
    free((char *)intData);
    return status;
}
int get_vect1d_float_slice_(int *expIdx, char *cpoPath, char *path, char *timeBasePath, float *data, int *dim, int *retDim,
        double *time, double *retTime, int *interpolMode, int *stat, int cpoPathLen, int pathLen, int timeBasePathLen)
{
    int status;
    char *intCpoPath = allocateC(cpoPath, cpoPathLen);
    char *intPath = allocateC(path, pathLen);
    char *intTimeBasePath = allocateC(timeBasePath, timeBasePathLen);
    float *intData;
    int outDim;
    status = getVect1DFloatSlice(*expIdx, intCpoPath, intPath, intTimeBasePath, &intData, &outDim, *time, retTime, *interpolMode);
    *stat = status;
    free(intCpoPath);
    free(intPath);
    free(intTimeBasePath);
    if(status) return status;
    *retDim = outDim;
    if(*retDim > *dim)
        *retDim = *dim;

    memcpy(data, intData, *retDim * sizeof(float));
    free((char *)intData);
    return status;
}
int get_vect1d_double_slice_(int *expIdx, char *cpoPath, char *path, char *timeBasePath, double *data, int *dim, int *retDim,
        double *time, double *retTime, int *interpolMode, int *stat, int cpoPathLen, int pathLen, int timeBasePathLen)
{
    int status;
    char *intCpoPath = allocateC(cpoPath, cpoPathLen);
    char *intPath = allocateC(path, pathLen);
    char *intTimeBasePath = allocateC(timeBasePath, timeBasePathLen);
    double *intData;
    int outDim;
    status = getVect1DDoubleSlice(*expIdx, intCpoPath, intPath, intTimeBasePath, &intData, &outDim, *time, retTime, *interpolMode);
    *stat = status;
    free(intCpoPath);
    free(intPath);
    free(intTimeBasePath);
    if(status) return status;
    *retDim = outDim;
    if(*retDim > *dim)
        *retDim = *dim;

    memcpy(data, intData, *retDim * sizeof(double));
    free((char *)intData);
    return status;
}

int get_vect2d_int_slice_(int *expIdx, char *cpoPath, char *path, char *timeBasePath, int *data, int *dim1, int *dim2,
    int *retDim1, int *retDim2, double *time, double *retTime, int *interpolMode, int *stat, int cpoPathLen, int pathLen, int timeBasePathLen)
{
    int status, i, j;
    char *intCpoPath = allocateC(cpoPath, cpoPathLen);
    char *intPath = allocateC(path, pathLen);
    char *intTimeBasePath = allocateC(timeBasePath, timeBasePathLen);
    int *intData;
    int outDim1, outDim2;
    status = getVect2DIntSlice(*expIdx, intCpoPath, intPath, intTimeBasePath, &intData, &outDim1, &outDim2, *time, retTime, *interpolMode);
    *stat = status;
    free(intCpoPath);
    free(intPath);
    free(intTimeBasePath);
    if(status) return status;
    *retDim1 = outDim1;
    *retDim2 = outDim2;
    if(*retDim1 > *dim1)
        *retDim1 = *dim1;
    if(*retDim2 > *dim2)
        *retDim2 = *dim2;

    for(i = 0; i < *retDim1; i++)
    {
        for(j = 0; j < *retDim2; j++)
        {
            data[i* outDim2 + j] = intData[i* outDim2 + j];
        }
    }
    free((char *)intData);
    return status;
}
int get_vect2d_float_slice_(int *expIdx, char *cpoPath, char *path, char *timeBasePath, float *data, int *dim1, int *dim2,
    int *retDim1, int *retDim2, double *time, double *retTime, int *interpolMode, int *stat, int cpoPathLen, int pathLen, int timeBasePathLen)
{
    int status, i, j;
    char *intCpoPath = allocateC(cpoPath, cpoPathLen);
    char *intPath = allocateC(path, pathLen);
    char *intTimeBasePath = allocateC(timeBasePath, timeBasePathLen);
    float *intData;
    int outDim1, outDim2;
    status = getVect2DFloatSlice(*expIdx, intCpoPath, intPath, intTimeBasePath, &intData, &outDim1, &outDim2, *time, retTime, *interpolMode);
    *stat = status;
    free(intCpoPath);
    free(intPath);
    free(intTimeBasePath);
    if(status) return status;
    *retDim1 = outDim1;
    *retDim2 = outDim2;
    if(*retDim1 > *dim1)
        *retDim1 = *dim1;
    if(*retDim2 > *dim2)
        *retDim2 = *dim2;

    for(i = 0; i < *retDim1; i++)
    {
        for(j = 0; j < *retDim2; j++)
        {
            data[i* outDim2 + j] = intData[i* outDim2 + j];
        }
    }
    free((char *)intData);
    return status;
}
int get_vect2d_double_slice_(int *expIdx, char *cpoPath, char *path, char *timeBasePath, double *data, int *dim1, int *dim2,
    int *retDim1, int *retDim2, double *time, double *retTime, int *interpolMode, int *stat, int cpoPathLen, int pathLen, int timeBasePathLen)
{
    int status, i, j;
    char *intCpoPath = allocateC(cpoPath, cpoPathLen);
    char *intPath = allocateC(path, pathLen);
    char *intTimeBasePath = allocateC(timeBasePath, timeBasePathLen);
    double *intData;
    int outDim1, outDim2;
    status = getVect2DDoubleSlice(*expIdx, intCpoPath, intPath, intTimeBasePath, &intData, &outDim1, &outDim2, *time, retTime, *interpolMode);
    *stat = status;
    free(intCpoPath);
    free(intPath);
    free(intTimeBasePath);
    if(status) return status;
    *retDim1 = outDim1;
    *retDim2 = outDim2;
    if(*retDim1 > *dim1)
        *retDim1 = *dim1;
    if(*retDim2 > *dim2)
        *retDim2 = *dim2;

    for(i = 0; i < *retDim1; i++)
    {
        for(j = 0; j < *retDim2; j++)
        {
            data[i* outDim2 + j] = intData[i* outDim2 + j];
        }
    }
    free((char *)intData);
    return status;
}

int get_vect3d_double_slice_(int *expIdx, char *cpoPath, char *path, char *timeBasePath, double *data, int *dim1, int *dim2, int *dim3,
    int *retDim1, int *retDim2, int *retDim3, double *time, double *retTime, int *interpolMode, int *stat, int cpoPathLen, int pathLen, int timeBasePathLen)
{
    int status, i, j, k;
    char *intCpoPath = allocateC(cpoPath, cpoPathLen);
    char *intPath = allocateC(path, pathLen);
    char *intTimeBasePath = allocateC(timeBasePath, timeBasePathLen);
    double *intData;
    int outDim1, outDim2, outDim3;
    status = getVect3DDoubleSlice(*expIdx, intCpoPath, intPath, intTimeBasePath, &intData, &outDim1, &outDim2, &outDim3, *time, retTime, *interpolMode);
    *stat = status;
    free(intCpoPath);
    free(intPath);
    free(intTimeBasePath);
    if(status) return status;
    *retDim1 = outDim1;
    *retDim2 = outDim2;
    *retDim3 = outDim3;
    if(*retDim1 > *dim1)
        *retDim1 = *dim1;
    if(*retDim2 > *dim2)
        *retDim2 = *dim2;
    if(*retDim3 > *dim3)
        *retDim3 = *dim3;

    for(i = 0; i < *retDim1; i++)
    {
        for(j = 0; j < *retDim2; j++)
        {
            for(k = 0; k < *retDim3; k++)
            {
	        data[i* outDim2 * outDim3 + j * outDim3 + k] = intData[i* outDim2 * outDim3 + j * outDim3 + k];
	    }
        }
    }
    free((char *)intData);
    return status;
}

int get_vect4d_double_slice_(int *expIdx, char *cpoPath, char *path, char *timeBasePath, double *data, int *dim1, int *dim2, int *dim3,
    int *dim4, int *retDim1, int *retDim2, int *retDim3, int *retDim4, double *time, double *retTime,
    int *interpolMode, int *stat, int cpoPathLen, int pathLen, int timeBasePathLen)
{
    int status, i, j, k, l;
    char *intCpoPath = allocateC(cpoPath, cpoPathLen);
    char *intPath = allocateC(path, pathLen);
    char *intTimeBasePath = allocateC(timeBasePath, timeBasePathLen);
    double *intData;
    int outDim1, outDim2, outDim3, outDim4;
    status = getVect4DDoubleSlice(*expIdx, intCpoPath, intPath, intTimeBasePath, &intData, &outDim1, &outDim2, &outDim3, &outDim4, *time, retTime, *interpolMode);
    *stat = status;
    free(intCpoPath);
    free(intPath);
    free(intTimeBasePath);
    if(status) return status;
    *retDim1 = outDim1;
    *retDim2 = outDim2;
    *retDim3 = outDim3;
    *retDim4 = outDim4;
    if(*retDim1 > *dim1)
        *retDim1 = *dim1;
    if(*retDim2 > *dim2)
        *retDim2 = *dim2;
    if(*retDim3 > *dim3)
        *retDim3 = *dim3;
    if(*retDim4 > *dim4)
        *retDim4 = *dim4;

    for(i = 0; i < *retDim1; i++)
    {
        for(j = 0; j < *retDim2; j++)
        {
            for(k = 0; k < *retDim3; k++)
            {
                for(l = 0; l < *retDim4; l++)
                {
                    data[i* outDim2 * outDim3 * outDim4 + j * outDim3 * outDim4 + k * outDim4 + l] =
                        intData[i* outDim2 * outDim3 * outDim4 + j * outDim3 * outDim4 + k * outDim4 + l];
                }
            }
        }
    }
    free((char *)intData);
    return status;
}
int get_vect5d_double_slice_(int *expIdx, char *cpoPath, char *path, char *timeBasePath, double *data, int *dim1, int *dim2, int *dim3,
    int *dim4, int *dim5, int *retDim1, int *retDim2, int *retDim3, int *retDim4, int *retDim5, double *time, double *retTime,
    int *interpolMode, int *stat, int cpoPathLen, int pathLen, int timeBasePathLen)
{
    int status, i, j, k, l, m;
    char *intCpoPath = allocateC(cpoPath, cpoPathLen);
    char *intPath = allocateC(path, pathLen);
    char *intTimeBasePath = allocateC(timeBasePath, timeBasePathLen);
    double *intData;
    int outDim1, outDim2, outDim3, outDim4, outDim5;
    status = getVect5DDoubleSlice(*expIdx, intCpoPath, intPath, intTimeBasePath, &intData, &outDim1, &outDim2, &outDim3, &outDim4, &outDim5, *time, retTime, *interpolMode);
    *stat = status;
    free(intCpoPath);
    free(intPath);
    free(intTimeBasePath);
    if(status) return status;
    *retDim1 = outDim1;
    *retDim2 = outDim2;
    *retDim3 = outDim3;
    *retDim4 = outDim4;
    *retDim5 = outDim5;
    if(*retDim1 > *dim1)
        *retDim1 = *dim1;
    if(*retDim2 > *dim2)
        *retDim2 = *dim2;
    if(*retDim3 > *dim3)
        *retDim3 = *dim3;
    if(*retDim4 > *dim4)
        *retDim4 = *dim4;
    if(*retDim5 > *dim5)
        *retDim5 = *dim5;

    for(i = 0; i < *retDim1; i++)
    {
        for(j = 0; j < *retDim2; j++)
        {
            for(k = 0; k < *retDim3; k++)
            {
                for(l = 0; l < *retDim4; l++)
                {
                    for(m = 0; m < *retDim5; m++)
                    {
                        data[i* outDim2 * outDim3 * outDim4 * outDim5 + j * outDim3 * outDim4 * outDim5 +
                            k * outDim4 *outDim5 + l * outDim5 + m] =
                            intData[i* outDim2 * outDim3 * outDim4 * outDim5 + j * outDim3 * outDim4 * outDim5 +
                            k * outDim4 *outDim5 + l * outDim5 + m];
                    }
                }
            }
        }
    }
    free((char *)intData);
    return status;
}

int get_vect6d_double_slice_(int *expIdx, char *cpoPath, char *path, char *timeBasePath, double *data, int *dim1, int *dim2, int *dim3,
    int *dim4, int *dim5, int *dim6, int *retDim1, int *retDim2, int *retDim3, int *retDim4, int *retDim5, int *retDim6, double *time, double *retTime,
    int *interpolMode, int *stat, int cpoPathLen, int pathLen, int timeBasePathLen)
{
    int status, i, j, k, l, m, n;
    char *intCpoPath = allocateC(cpoPath, cpoPathLen);
    char *intPath = allocateC(path, pathLen);
    char *intTimeBasePath = allocateC(timeBasePath, timeBasePathLen);
    double *intData;
    int outDim1, outDim2, outDim3, outDim4, outDim5, outDim6;
    status = getVect6DDoubleSlice(*expIdx, intCpoPath, intPath, intTimeBasePath, &intData, &outDim1, &outDim2, &outDim3, &outDim4, &outDim5, &outDim6, *time, retTime, *interpolMode);
    *stat = status;
    free(intCpoPath);
    free(intPath);
    free(intTimeBasePath);
    if(status) return status;
    *retDim1 = outDim1;
    *retDim2 = outDim2;
    *retDim3 = outDim3;
    *retDim4 = outDim4;
    *retDim5 = outDim5;
    *retDim6 = outDim6;
    if(*retDim1 > *dim1)
        *retDim1 = *dim1;
    if(*retDim2 > *dim2)
        *retDim2 = *dim2;
    if(*retDim3 > *dim3)
        *retDim3 = *dim3;
    if(*retDim4 > *dim4)
        *retDim4 = *dim4;
    if(*retDim5 > *dim5)
        *retDim5 = *dim5;
    if(*retDim6 > *dim6)
        *retDim6 = *dim6;

    for(i = 0; i < *retDim1; i++)
    {
        for(j = 0; j < *retDim2; j++)
        {
            for(k = 0; k < *retDim3; k++)
            {
                for(l = 0; l < *retDim4; l++)
                {
                    for(m = 0; m < *retDim5; m++)
                    {
                        for(n = 0; n < *retDim6; m++)
                        {
                          data[i* outDim2 * outDim3 * outDim4 * outDim5 * outDim6 + j * outDim3 * outDim4 * outDim5 * outDim6 +
                            k * outDim4 *outDim5 * outDim6 + l * outDim5 * outDim6 + m * outDim6 + n] =
                            intData[i* outDim2 * outDim3 * outDim4 * outDim5  * outDim6+ j * outDim3 * outDim4 * outDim5 * outDim6 +
                            k * outDim4 *outDim5 * outDim6 + l * outDim5 * outDim6 + m * outDim6 + n];
			}
                    }
                }
            }
        }
    }
    free((char *)intData);
    return status;
}


int begin_ids_put_(int *expIdx, char *path, int pathLen)
{
    int status;
    char *intPath = allocateC(path, pathLen);

reportInfo("BEGIN CPO PUT %s \n", intPath);

    status = beginIdsPut(*expIdx, intPath);
    free(intPath);
    return status;
}
void end_ids_put_(int *expIdx, char *path, int pathLen)
{
    char *intPath = allocateC(path, pathLen);
    endIdsPut(*expIdx, intPath);
reportInfo("END CPO PUT %s \n", intPath);
    free(intPath);
}
int begin_ids_put_non_timed_(int *expIdx, char *path, int pathLen)
{
    int status;
    char *intPath = allocateC(path, pathLen);
reportInfo("BEGIN CPO PUT NON TIMED%s \n", intPath);
    status = beginIdsPutNonTimed(*expIdx, intPath);
    free(intPath);
    return status;
}
void end_ids_put_non_timed_(int *expIdx, char *path, int pathLen)
{
    char *intPath = allocateC(path, pathLen);
    endIdsPutNonTimed(*expIdx, intPath);
reportInfo("END CPO PUT NON TIMED%s \n", intPath);
    free(intPath);
}
int begin_ids_put_timed_(int *expIdx, char *path, int *nSamples, double *time, int pathLen)
{
    int status;
    char *intPath = allocateC(path, pathLen);
reportInfo("BEGIN CPO PUT TIMED%s \n", intPath);
    status = beginIdsPutTimed(*expIdx, intPath, *nSamples, time);
    free(intPath);
    return status;
}
void end_ids_put_timed_(int *expIdx, char *path, int pathLen)
{
    char *intPath = allocateC(path, pathLen);
    endIdsPutTimed(*expIdx, intPath);
reportInfo("END CPO PUT TIMED %s \n", intPath);
    free(intPath);
}
int begin_ids_put_slice_(int *expIdx, char *path, int pathLen)
{
    int status;
    char *intPath = allocateC(path, pathLen);
reportInfo("BEGIN CPO PUT SLICE %s \n", intPath);
    status = beginIdsPutSlice(*expIdx, intPath);
    free(intPath);
    return status;
}
void end_ids_put_slice_(int *expIdx, char *path, int pathLen)
{
    char *intPath = allocateC(path, pathLen);
    endIdsPutSlice(*expIdx, intPath);
reportInfo("END CPO PUT SLICE %s \n", intPath);
    free(intPath);
}


int begin_ids_get_(int *expIdx, char *path, int *isTimed, int *retSamples, int pathLen)
{
    int status;
    char *intPath = allocateC(path, pathLen);
reportInfo("BEGIN CPO GET %s \n", intPath);
    status = beginIdsGet(*expIdx, intPath, *isTimed, retSamples);
    free(intPath);
    return status;
}
void end_ids_get_(int *expIdx, char *path, int pathLen)
{
    char *intPath = allocateC(path, pathLen);
    endIdsGet(*expIdx, intPath);
    free(intPath);
reportInfo("END CPO GET %s \n", intPath);
}
int begin_ids_get_slice_(int *expIdx, char *path, double *time, int *stat, int pathLen)
{
    int status;
    char *intPath = allocateC(path, pathLen);

reportInfo("BEGIN CPO GET SLICE %s \n", intPath);
    status = beginIdsGetSlice(*expIdx, intPath, *time);
    *stat = status;
    free(intPath);
    return status;
}
void end_ids_get_slice_(int *expIdx, char *path, int pathLen)
{
    char *intPath = allocateC(path, pathLen);
    endIdsGetSlice(*expIdx, intPath);
    free(intPath);
reportInfo("END CPO GET SLICE %s \n", intPath);
}

int begin_ids_replace_last_slice_(int *expIdx, char *path, int *stat, int pathLen)
{
    int status;
    char *intPath = allocateC(path, pathLen);
    status = beginIdsReplaceLastSlice(*expIdx, intPath);
    *stat = status;
    free(intPath);
    return status;
}
void end_ids_replace_last_slice_(int *expIdx, char *path, int pathLen)
{
    char *intPath = allocateC(path, pathLen);
    endIdsReplaceLastSlice(*expIdx, intPath);
    free(intPath);
}

void imas_last_errmsg_(char *errMsg, int len)
{
    char *intErr = imas_last_errmsg();
    int minLen, i;
    minLen = strlen(intErr);
    if(minLen > len)
        minLen = len;

    for(i = 0; i < minLen; i++)
        errMsg[i] = intErr[i];
    for(;i < len; i++)
        errMsg[i] = ' ';
}

int imas_create_(char *name, int *shot, int *run, int *refShot, int *refRun, int *retIdx, int nameLen)
{
    int status;
    char *intName = allocateC(name, nameLen);
    status = imas_create(intName, *shot, *run, *refShot, *refRun, retIdx);
    free(intName);
   reportInfo("imas CREATE", "");
    return status;
}

int imas_create_env_(char *name, int *shot, int *run, int *refShot, int *refRun, int *retIdx,
 char *user, char *tokamak, char *version, int nameLen, int userLen, int tokamakLen, int versionLen)
{
    int status;
    char *intName = allocateC(name, nameLen);
    char *intUser = allocateC(user, userLen);
    char *intTokamak = allocateC(tokamak, tokamakLen);
    char *intVersion = allocateC(version, versionLen);
    status = imas_create_env(intName, *shot, *run, *refShot, *refRun, retIdx, intUser, intTokamak, intVersion);
    free(intName);
    free(intUser);
    free(intTokamak);
    free(intVersion);
    return status;
}

int imas_create_hdf5_(char *name, int *shot, int *run, int *refShot, int *refRun, int *retIdx, int nameLen)
{
    int status;
    char *intName = allocateC(name, nameLen);
    status = imas_create_hdf5(intName, *shot, *run, *refShot, *refRun, retIdx);
    free(intName);
    return status;
}

int imas_connect_(char *ip, int ipLen)
{
    char *intIp = allocateC(ip, ipLen);
    int status = imas_connect(intIp);
    free(intIp);
    return status;
}

int imas_disconnect_()
{
    return imas_disconnect();
}

int imas_exec_(char *ip, char *command, char *stdOut, int ipLen, int commandLen, int stdOutLen)
{
    int i;
    char *_ip = allocateC(ip, ipLen);
    char *_command = allocateC(command, commandLen);
    char *_stdOut = imas_exec(_ip,_command);
    free(_ip);
    if (_stdOut==NULL) {
        printf("Error executing command: %s\n",_command);
        free(_command);
        return -1;
    }
    free(_command);
    int _stdOutLen = strlen(_stdOut);
    // copy string to Fortran
    for (i=0; i<stdOutLen  && i<_stdOutLen; i++)
        stdOut[i] = _stdOut[i];
    // pad with blanks
    for(; i<stdOutLen; i++)
        stdOut[i] = ' ';
    free(_stdOut);
    return 0;
}

int imas_open_(char *name, int *shot, int *run, int *retIdx, int nameLen)
{
    int status;
    char *intName = allocateC(name, nameLen);

    status = imas_open(intName, *shot, *run, retIdx);
   reportInfo("imas OPEN ", "");
    free(intName);
    return status;
}

int imas_open_env_(char *name, int *shot, int *run, int *retIdx, char *user, char *tokamak, char
*version, int nameLen, int userLen, int tokamakLen, int versionLen)
{
    int status;
    char *intName = allocateC(name, nameLen);
    char *intUser = allocateC(user, userLen);
    char *intTokamak = allocateC(tokamak, tokamakLen);
    char *intVersion = allocateC(version, versionLen);
   reportInfo("imas OPEN ENV ", "");
    status = imas_open_env(intName, *shot, *run, retIdx, intUser, intTokamak, intVersion);
    free(intName);
    free(intUser);
    free(intTokamak);
    free(intVersion);
    return status;
}

int imas_open_hdf5_(char *name, int *shot, int *run, int *retIdx, int nameLen)
{
    int status;
    char *intName = allocateC(name, nameLen);

    status = imas_open_hdf5(intName, *shot, *run, retIdx);
    free(intName);
    return status;
}

int imas_close_(int *idx)
{
    int status;
    reportInfo("imas CLOSE START", "");

    status = imas_close(*idx);
    /* printf("Closed: status  %d %s\n", status, imas_last_errmsg());*/
    reportInfo("imas CLOSE END", "");
    return status;
}

/* Deprecated
//dummy routines
void imas_discard_cache_(int *expIdx, char *cpoPathm, char *path, int cpoPathLen, int pathLen){}
void imas_flush_cache_(int *expIdx, char *cpoPathm, char *path, int cpoPathLen, int pathLen){}
*/

void imas_enable_mem_cache_(int *expIdx)
{
    imas_enable_mem_cache(*expIdx);
}
void imas_disable_mem_cache_(int *expIdx)
{
    imas_disable_mem_cache(*expIdx);
}
void imas_flush_all_(int *expIdx)
{
    imas_flush_mem_cache(*expIdx);
}

void imas_discard_all_(int *expIdx)
{
    imas_discard_mem_cache(*expIdx);
}
void imas_flush_(int *expIdx, char *cpoPath, int cpoPathLen)
{
    char *intCpoPath = allocateC(cpoPath, cpoPathLen);
    imas_flush_cpo_mem_cache(*expIdx, intCpoPath);
    free(intCpoPath);
}

void imas_discard_(int *expIdx, char *cpoPath, int cpoPathLen)
{
    char *intCpoPath = allocateC(cpoPath, cpoPathLen);
    imas_discard_cpo_mem_cache(*expIdx, intCpoPath);
    free(intCpoPath);
}


/******************** Object management (arrays of structures)********************/

int begin_object_(int *expIdx, int *hParent, int *index, char *relPath, int *isTimed, int *handle, int relPathLen)
{
    void *parent,*obj;
    char *path = allocateC(relPath, relPathLen);
    if (*hParent<0)
        parent = NULL;
    else
        parent = getObjectFromList(*hParent);
    obj = beginObject(*expIdx,parent,*index-1,path,*isTimed);
    free(path);
    *handle = addObjectToList(obj);
    if (*handle < 0) {
      releaseObject(*expIdx,obj);
      printf("No more slots available for arrays of structures\n");
    }
    return *handle;
}

void release_object_(int *expIdx, int *handle)
{
    void *obj = getObjectFromList(*handle);
    removeObjectFromList(*handle);
    releaseObject(*expIdx, obj);
}

int put_object_(int *expIdx, char *cpoPath, char *path, int *handle, int *isTimed, int *stat, int cpoPathLen, int pathLen)
{
    char *intCpoPath = allocateC(cpoPath, cpoPathLen);
    char *intPath = allocateC(path, pathLen);
    void *obj = getObjectFromList(*handle);
    int status = putObject(*expIdx, intCpoPath, intPath, obj, *isTimed);
    *stat = status;

    free(intCpoPath);
    free(intPath);
    //printf("Put_Object status = %d\n",status);
    if(status)
        printf("%s\n",imas_last_errmsg());
    return status;
}

int put_string_in_object_(int *expIdx, int *handle, char *path, int *idx, char *data, int pathLen, int dataLen)
{
    void *newObj;
    void *obj = getObjectFromList(*handle);
    char *intPath = allocateC(path, pathLen);
    char *intData = allocateC(data, dataLen);
    newObj = putStringInObject(*expIdx, obj, intPath, *idx-1, intData);
    replaceObjectInList(*handle, newObj);
    free(intPath);
    free(intData);
    return 0;
}

int put_int_in_object_(int *expIdx, int *handle, char *path, int *idx, int *data, int pathLen)
{
    void *newObj;
    void *obj = getObjectFromList(*handle);
    char *intPath = allocateC(path, pathLen);
    newObj = putIntInObject(*expIdx, obj, intPath, *idx-1, *data);
    replaceObjectInList(*handle, newObj);
    free(intPath);
    return 0;
}

int put_float_in_object_(int *expIdx, int *handle, char *path, int *idx, float *data,  int pathLen)
{
    void *newObj;
    void *obj = getObjectFromList(*handle);
    char *intPath = allocateC(path, pathLen);
    newObj = putFloatInObject(*expIdx, obj, intPath, *idx-1, *data);
    replaceObjectInList(*handle, newObj);
    free(intPath);
    return 0;
}

int put_double_in_object_(int *expIdx, int *handle, char *path, int *idx, double *data, int pathLen)
{
    void *newObj;
    void *obj = getObjectFromList(*handle);
    char *intPath = allocateC(path, pathLen);
    newObj = putDoubleInObject(*expIdx, obj, intPath, *idx-1, *data);
    replaceObjectInList(*handle, newObj);
    free(intPath);
    return 0;
}

int put_vect1d_int_in_object_(int *expIdx, int *handle, char *path, int *idx, int *data, int *dim,  int pathLen)
{
    void *newObj;
    void *obj = getObjectFromList(*handle);
    char *intPath = allocateC(path, pathLen);
    newObj = putVect1DIntInObject(*expIdx, obj, intPath, *idx-1, data, *dim);
    replaceObjectInList(*handle, newObj);
    free(intPath);
    return 0;
}

int put_vect1d_float_in_object_(int *expIdx, int *handle, char *path, int *idx, float *data, int *dim, int pathLen)
{
    void *newObj;
    void *obj = getObjectFromList(*handle);
    char *intPath = allocateC(path, pathLen);
    newObj = putVect1DFloatInObject(*expIdx, obj, intPath, *idx-1, data, *dim);
    replaceObjectInList(*handle, newObj);
    free(intPath);
    return 0;
}

int put_vect1d_double_in_object_(int *expIdx, int *handle, char *path, int *idx, double *data, int *dim, int pathLen)
{
    void *newObj;
    void *obj = getObjectFromList(*handle);
    char *intPath = allocateC(path, pathLen);
    newObj = putVect1DDoubleInObject(*expIdx, obj, intPath, *idx-1, data, *dim);
    replaceObjectInList(*handle, newObj);
    free(intPath);
    return 0;
}

int put_vect1d_string_in_object_(int *expIdx, int *handle, char *path, int *idx, char *data, int *dim, int *dimtab, int pathLen)
{
    int i;
    void *newObj;
    void *obj = getObjectFromList(*handle);
    char *intPath = allocateC(path, pathLen);

    char** tabchar;
    tabchar=malloc(*dim*sizeof(char *));

    for(i = 0; i < *dim; i++)
    {
       tabchar[i] = allocateC(data + i*132, dimtab[i]);
    }

    newObj = putVect1DStringInObject(*expIdx, obj, intPath, *idx-1, tabchar, *dim);
    replaceObjectInList(*handle, newObj);
    free(intPath);
    for(i = 0; i < *dim; i++) free(tabchar[i]);
    free(tabchar);
    return 0;
}

int put_vect2d_int_in_object_(int *expIdx, int *handle, char *path, int *idx, int *data, int *dim1, int *dim2,  int pathLen)
{
    void *newObj;
    void *obj = getObjectFromList(*handle);
    char *intPath = allocateC(path, pathLen);
    newObj = putVect2DIntInObject(*expIdx, obj, intPath, *idx-1, data, *dim1, *dim2);
    replaceObjectInList(*handle, newObj);
    free(intPath);
    return 0;
}
int put_vect2d_float_in_object_(int *expIdx, int *handle, char *path, int *idx, float *data, int *dim1, int *dim2, int pathLen)
{
    void *newObj;
    void *obj = getObjectFromList(*handle);
    char *intPath = allocateC(path, pathLen);
    newObj = putVect2DFloatInObject(*expIdx, obj, intPath, *idx-1, data, *dim1, *dim2);
    replaceObjectInList(*handle, newObj);
    free(intPath);
    return 0;
}
int put_vect2d_double_in_object_(int *expIdx, int *handle, char *path, int *idx, double *data, int *dim1, int *dim2,  int pathLen)
{
    void *newObj;
    void *obj = getObjectFromList(*handle);
    char *intPath = allocateC(path, pathLen);
    newObj = putVect2DDoubleInObject(*expIdx, obj, intPath, *idx-1, data, *dim1, *dim2);
    replaceObjectInList(*handle, newObj);
    free(intPath);
    return 0;
}


int put_vect3d_int_in_object_(int *expIdx, int *handle, char *path, int *idx, int *data, int *dim1, int *dim2, int *dim3, int pathLen)
{
    void *newObj;
    void *obj = getObjectFromList(*handle);
    char *intPath = allocateC(path, pathLen);
    newObj = putVect3DIntInObject(*expIdx, obj, intPath, *idx-1, data, *dim1, *dim2, *dim3);
    replaceObjectInList(*handle, newObj);
    free(intPath);
    return 0;
}
int put_vect3d_float_in_object_(int *expIdx, int *handle, char *path, int *idx, float *data, int *dim1, int *dim2, int *dim3,  int pathLen)
{
    void *newObj;
    void *obj = getObjectFromList(*handle);
    char *intPath = allocateC(path, pathLen);
    newObj = putVect3DFloatInObject(*expIdx, obj, intPath, *idx-1, data, *dim1, *dim2, *dim3);
    replaceObjectInList(*handle, newObj);
    free(intPath);
    return 0;
}
int put_vect3d_double_in_object_(int *expIdx, int *handle, char *path, int *idx, double *data, int *dim1, int *dim2, int *dim3,  int pathLen)
{
    void *newObj;
    void *obj = getObjectFromList(*handle);
    char *intPath = allocateC(path, pathLen);
    newObj = putVect3DDoubleInObject(*expIdx, obj, intPath, *idx-1, data, *dim1, *dim2, *dim3);
    replaceObjectInList(*handle, newObj);
    free(intPath);
    return 0;
}

int put_vect4d_double_in_object_(int *expIdx, int *handle, char *path, int *idx, double *data, int *dim1, int *dim2, int *dim3, int *dim4,  int pathLen)
{
    void *newObj;
    void *obj = getObjectFromList(*handle);
    char *intPath = allocateC(path, pathLen);
    newObj = putVect4DDoubleInObject(*expIdx, obj, intPath, *idx-1, data, *dim1, *dim2, *dim3, *dim4);
    replaceObjectInList(*handle, newObj);
    free(intPath);
    return 0;
}
int put_vect5d_double_in_object_(int *expIdx, int *handle, char *path, int *idx, double *data, int *dim1, int *dim2, int *dim3, int *dim4, int *dim5,  int pathLen)
{
    void *newObj;
    void *obj = getObjectFromList(*handle);
    char *intPath = allocateC(path, pathLen);
    newObj = putVect5DDoubleInObject(*expIdx, obj, intPath, *idx-1, data, *dim1, *dim2, *dim3, *dim4, *dim5);
    replaceObjectInList(*handle, newObj);
    free(intPath);
    return 0;
}
int put_vect6d_double_in_object_(int *expIdx, int *handle, char *path, int *idx, double *data, int *dim1, int *dim2, int *dim3, int *dim4, int *dim5, int *dim6,  int pathLen)
{
    void *newObj;
    void *obj = getObjectFromList(*handle);
    char *intPath = allocateC(path, pathLen);
    newObj = putVect6DDoubleInObject(*expIdx, obj, intPath, *idx-1, data, *dim1, *dim2, *dim3, *dim4, *dim5, *dim6);
    replaceObjectInList(*handle, newObj);
    free(intPath);
    return 0;
}
int put_vect7d_double_in_object_(int *expIdx, int *handle, char *path, int *idx, double *data, int *dim1, int *dim2, int *dim3, int *dim4, int *dim5, int *dim6, int *dim7, int pathLen)
{
    void *newObj;
    void *obj = getObjectFromList(*handle);
    char *intPath = allocateC(path, pathLen);
    newObj = putVect7DDoubleInObject(*expIdx, obj, intPath, *idx-1, data, *dim1, *dim2, *dim3, *dim4, *dim5, *dim6, *dim7);
    replaceObjectInList(*handle, newObj);
    free(intPath);
    return 0;
}

int put_object_in_object_(int *expIdx, int *handle, char *path, int *idx, int *dataHandle,  int pathLen)
{
    char *intPath = allocateC(path, pathLen);
    void *obj = getObjectFromList(*handle);
    void *dataObj = getObjectFromList(*dataHandle);
    void *newObj = putObjectInObject(*expIdx, obj, intPath, *idx-1, dataObj);
    replaceObjectInList(*handle, newObj);
    free(intPath);
    return 0;
}

int get_object_dim_(int *expIdx, int *handle, int *dim)
{
    void *obj = getObjectFromList(*handle);
    *dim = getObjectDim(*expIdx, obj);
    return *dim;
}

int get_object_(int *expIdx, char *cpoPath, char *path, int *handle, int *isTimed, int *stat, int cpoPathLen, int pathLen)
{
    char *intCpoPath = allocateC(cpoPath, cpoPathLen);
    char *intPath = allocateC(path, pathLen);
    void * obj;
    int status = getObject(*expIdx, intCpoPath, intPath, &obj, *isTimed);
    *stat = status;
    free(intPath);
    free(intCpoPath);
    if(status) return status;
    *handle = addObjectToList(obj);
    if (*handle < 0) {
        releaseObject(*expIdx,obj);
        printf("No more slots available for arrays of structures\n");
        return -1;
    }
    return 0;
}

int get_dimension_from_object_(int *expIdx, int *handle, char *path, int *idx, int *numDims, int *dim1, int *dim2, int *dim3, int *dim4, int *dim5, int *dim6, int *dim7,  int pathLen)
{
    int status;
    void *obj = getObjectFromList(*handle);
    char *intPath = allocateC(path, pathLen);
    status = getDimensionFromObject(*expIdx, obj, intPath, *idx-1, numDims, dim1, dim2, dim3, dim4, dim5, dim6, dim7);
    free(intPath);
    return status;
}

int get_string_from_object_(int *expIdx, int *handle, char *path, int *idx, char *data, int *stat, int pathLen, int dataLen)
{
    int status, i, len;
    void *obj = getObjectFromList(*handle);
    char *intPath = allocateC(path, pathLen);
    char *intData;
    status = getStringFromObject(*expIdx, obj, intPath, *idx-1, &intData);
    *stat = status;
    free(intPath);
    if(status) return status;

    len = strlen(intData);
    for(i = 0; i < len && i < dataLen; i++)
        data[i] = intData[i];
    /* for(;i < dataLen; i++) Not necessary to pad end of the string with blanks (rapidly tested)
        data[i] = ' ';  */
    free(intData);
    return status;

}
int get_int_from_object_(int *expIdx, int *handle, char *path, int *idx, int *data, int *stat, int pathLen)
{
    int status;
    void *obj = getObjectFromList(*handle);
    char *intPath = allocateC(path, pathLen);
    status = getIntFromObject(*expIdx, obj, intPath, *idx-1, data);
    *stat = status;
    free(intPath);
    return status;
}

int get_double_from_object_(int *expIdx, int *handle, char *path, int *idx, double *data, int *stat, int pathLen)
{
    int status;
    void *obj = getObjectFromList(*handle);
    char *intPath = allocateC(path, pathLen);
    status = getDoubleFromObject(*expIdx, obj, intPath, *idx-1, data);
    *stat = status;
    free(intPath);
    return status;
}
int get_vect1d_int_from_object_(int *expIdx, int *handle, char *path, int *idx, int *data, int *dim, int *retDim, int *stat, int pathLen)
{
    int status;
    void *obj = getObjectFromList(*handle);
    char *intPath = allocateC(path, pathLen);
    int *intData;
    int outDim;
    status = getVect1DIntFromObject(*expIdx, obj, intPath, *idx-1, &intData, &outDim);
    *stat = status;
    free(intPath);
    if(status) return status;
    *retDim = outDim;
    if(*retDim > *dim)
        *retDim = *dim;

    memcpy(data, intData, *retDim * sizeof(int));
    free((char *)intData);
    return status;
}
int get_vect1d_float_from_object_(int *expIdx, int *handle, char *path, int *idx, float *data, int *dim, int *retDim, int *stat, int pathLen)
{
    int status;
    void *obj = getObjectFromList(*handle);
    char *intPath = allocateC(path, pathLen);
    float *intData;
    int outDim;
    status = getVect1DFloatFromObject(*expIdx, obj, intPath, *idx-1, &intData, &outDim);
    *stat = status;
    free(intPath);
    if(status) return status;
    *retDim = outDim;
    if(*retDim > *dim)
        *retDim = *dim;

    memcpy(data, intData, *retDim * sizeof(float));
    free((char *)intData);
    return status;
}
int get_vect1d_double_from_object_(int *expIdx, int *handle, char *path, int *idx, double *data, int *dim, int *retDim, int *stat, int pathLen)
{
    int status;
    void *obj = getObjectFromList(*handle);
    char *intPath = allocateC(path, pathLen);
    double *intData;
    int outDim;
    status = getVect1DDoubleFromObject(*expIdx, obj, intPath, *idx-1, &intData, &outDim);
    *stat = status;
    free(intPath);
    if(status) return status;
    *retDim = outDim;
    if(*retDim > *dim)
        *retDim = *dim;

    memcpy(data, intData, *retDim * sizeof(double));
    free((char *)intData);
    return status;
}
int get_vect1d_string_from_object_(int *expIdx, int *handle, char *path, int *idx, char *data, int *dim, int *retDim, int *stat, int pathLen)
{
    int status;
    void *obj = getObjectFromList(*handle);
    char *intPath = allocateC(path, pathLen);
    char **intData;
    int outDim;
    int i,j;

    status = getVect1DStringFromObject(*expIdx, obj, intPath, *idx-1, &intData, &outDim);
    *stat = status;

    free(intPath);
    if(status) return status;
    *retDim = outDim;
    if(*retDim > *dim)
    *retDim = *dim;

    for(i = 0; i < *dim; i++)
    {

        for(j = 0; j < strlen(intData[i]) && j < 132; j++)
            data[i*132+j] = intData[i][j];

        for(;j < 132; j++)   /* necessary to pad the end of the string with blanks for string vectors, tested */
            data[i*132+j] = ' ';
    }

    for(i = 0; i < *dim; i++) free(intData[i]);
    free(intData);
    return status;
}

int get_vect2d_int_from_object_(int *expIdx, int *handle, char *path, int *idx, int *data, int *dim1, int *dim2, int *retDim1, int *retDim2, int *stat, int pathLen)
{
    int status, i, j;
    void *obj = getObjectFromList(*handle);
    char *intPath = allocateC(path, pathLen);
    int *intData;
    int outDim1, outDim2;
    status = getVect2DIntFromObject(*expIdx, obj, intPath, *idx-1, &intData, &outDim1, &outDim2);
    *stat = status;
    free(intPath);
    if(status) return status;
    *retDim1 = outDim1;
    *retDim2 = outDim2;
    if(*retDim1 > *dim1)
        *retDim1 = *dim1;
    if(*retDim2 > *dim2)
        *retDim2 = *dim2;

    for(i = 0; i < *retDim1; i++)
    {
        for(j = 0; j < *retDim2; j++)
        {
            data[i* outDim2 + j] = intData[i* outDim2 + j];
        }
    }
    free((char *)intData);
    return status;
}
int get_vect2d_float_from_object_(int *expIdx, int *handle, char *path, int *idx, float *data, int *dim1, int *dim2, int *retDim1, int *retDim2, int *stat, int pathLen)
{
    int status, i, j;
    void *obj = getObjectFromList(*handle);
    char *intPath = allocateC(path, pathLen);
    float *intData;
    int outDim1, outDim2;
    status = getVect2DFloatFromObject(*expIdx, obj, intPath, *idx-1, &intData, &outDim1, &outDim2);
    *stat = status;
    free(intPath);
    if(status) return status;
    *retDim1 = outDim1;
    *retDim2 = outDim2;
    if(*retDim1 > *dim1)
        *retDim1 = *dim1;
    if(*retDim2 > *dim2)
        *retDim2 = *dim2;

    for(i = 0; i < *retDim1; i++)
    {
        for(j = 0; j < *retDim2; j++)
        {
            data[i* outDim2 + j] = intData[i* outDim2 + j];
        }
    }
    free((char *)intData);
    return status;
}
int get_vect2d_double_from_object_(int *expIdx, int *handle, char *path, int *idx, double *data, int *dim1, int *dim2, int *retDim1, int *retDim2, int *stat, int pathLen)
{
    int status, i, j;
    void *obj = getObjectFromList(*handle);
    char *intPath = allocateC(path, pathLen);
    double *intData;
    int outDim1, outDim2;
    status = getVect2DDoubleFromObject(*expIdx, obj, intPath, *idx-1, &intData, &outDim1, &outDim2);
    *stat = status;
    free(intPath);
    if(status) return status;
    *retDim1 = outDim1;
    *retDim2 = outDim2;
    if(*retDim1 > *dim1)
        *retDim1 = *dim1;
    if(*retDim2 > *dim2)
        *retDim2 = *dim2;

    for(i = 0; i < *retDim1; i++)
    {
        for(j = 0; j < *retDim2; j++)
        {
            data[i* outDim2 + j] = intData[i* outDim2 + j];
        }
    }
    free((char *)intData);
    return status;
}


int get_vect3d_int_from_object_(int *expIdx, int *handle, char *path, int *idx, int *data, int *dim1, int *dim2, int *dim3,
    int *retDim1, int *retDim2, int *retDim3, int *stat, int pathLen)
{
    int status, i, j, k;
    void *obj = getObjectFromList(*handle);
    char *intPath = allocateC(path, pathLen);
    int *intData;
    int outDim1, outDim2, outDim3;
    status = getVect3DIntFromObject(*expIdx, obj, intPath, *idx-1, &intData, &outDim1, &outDim2, &outDim3);
    *stat = status;
    free(intPath);
    if(status) return status;
    *retDim1 = outDim1;
    *retDim2 = outDim2;
    *retDim3 = outDim3;
    if(*retDim1 > *dim1)
        *retDim1 = *dim1;
    if(*retDim2 > *dim2)
        *retDim2 = *dim2;
    if(*retDim3 > *dim3)
        *retDim3 = *dim3;

    for(i = 0; i < *retDim1; i++)
    {
        for(j = 0; j < *retDim2; j++)
        {
            for(k = 0; k < *retDim3; k++)
                data[i* outDim2 * outDim3 + j*outDim3 + k] = intData[i* outDim2 * outDim3 + j*outDim3 + k];
        }
    }
    free((char *)intData);
    return status;
}

int get_vect3d_float_from_object_(int *expIdx, int *handle, char *path, int *idx, float *data, int *dim1, int *dim2, int *dim3, int *retDim1, int *retDim2, int *retDim3, int *stat, int pathLen)
{
    int status, i, j, k;
    void *obj = getObjectFromList(*handle);
    char *intPath = allocateC(path, pathLen);
    float *intData;
    int outDim1, outDim2, outDim3;
    status = getVect3DFloatFromObject(*expIdx, obj, intPath, *idx-1, &intData, &outDim1, &outDim2, &outDim3);
    *stat = status;
    free(intPath);
    if(status) return status;
    *retDim1 = outDim1;
    *retDim2 = outDim2;
    *retDim3 = outDim3;
    if(*retDim1 > *dim1)
        *retDim1 = *dim1;
    if(*retDim2 > *dim2)
        *retDim2 = *dim2;
    if(*retDim3 > *dim3)
        *retDim3 = *dim3;

    for(i = 0; i < *retDim1; i++)
    {
        for(j = 0; j < *retDim2; j++)
        {
            for(k = 0; k < *retDim3; k++)
                data[i* outDim2 * outDim3 + j*outDim3 + k] = intData[i* outDim2 * outDim3 + j*outDim3 + k];
        }
    }
    free((char *)intData);
    return status;
}


int get_vect3d_double_from_object_(int *expIdx, int *handle, char *path, int *idx, double *data, int *dim1, int *dim2, int *dim3, int *retDim1, int *retDim2, int *retDim3, int *stat, int pathLen)
{
    int status, i, j, k;
    void *obj = getObjectFromList(*handle);
    char *intPath = allocateC(path, pathLen);
    double *intData;
    int outDim1, outDim2, outDim3;
    status = getVect3DDoubleFromObject(*expIdx, obj, intPath, *idx-1, &intData, &outDim1, &outDim2, &outDim3);
    *stat = status;
    free(intPath);
    if(status) return status;
    *retDim1 = outDim1;
    *retDim2 = outDim2;
    *retDim3 = outDim3;
    if(*retDim1 > *dim1)
        *retDim1 = *dim1;
    if(*retDim2 > *dim2)
        *retDim2 = *dim2;
    if(*retDim3 > *dim3)
        *retDim3 = *dim3;

    for(i = 0; i < *retDim1; i++)
    {
        for(j = 0; j < *retDim2; j++)
        {
            for(k = 0; k < *retDim3; k++)
                data[i* outDim2 * outDim3 + j*outDim3 + k] = intData[i* outDim2 * outDim3 + j*outDim3 + k];
        }
    }
    free((char *)intData);
    return status;
}

int get_vect4d_double_from_object_(int *expIdx, int *handle, char *path, int *idx, double *data, int *dim1, int *dim2, int *dim3, int *dim4, int *retDim1, int *retDim2, int *retDim3, int *retDim4, int *stat, int pathLen)
{
    int status, i, j, k, l;
    void *obj = getObjectFromList(*handle);
    char *intPath = allocateC(path, pathLen);
    double *intData;
    int outDim1, outDim2, outDim3, outDim4;
    status = getVect4DDoubleFromObject(*expIdx, obj, intPath, *idx-1, &intData, &outDim1, &outDim2, &outDim3, &outDim4);
    *stat = status;
    free(intPath);
    if(status) return status;
    *retDim1 = outDim1;
    *retDim2 = outDim2;
    *retDim3 = outDim3;
    *retDim4 = outDim4;
    if(*retDim1 > *dim1)
        *retDim1 = *dim1;
    if(*retDim2 > *dim2)
        *retDim2 = *dim2;
    if(*retDim3 > *dim3)
        *retDim3 = *dim3;
    if(*retDim4 > *dim4)
        *retDim4 = *dim4;

    for(i = 0; i < *retDim1; i++)
    {
        for(j = 0; j < *retDim2; j++)
        {
            for(k = 0; k < *retDim3; k++)
       {
           for(l = 0; l < *retDim4; l++)
      {
                   data[i* outDim2 * outDim3 * outDim4 + j*outDim3 *outDim4 + k*outDim4 + l] =
          intData[i* outDim2 * outDim3 * outDim4 + j*outDim3 *outDim4 + k*outDim4 + l];
      }
       }
        }
    }
    free((char *)intData);
    return status;
}
int get_vect5d_double_from_object_(int *expIdx, int *handle, char *path, int *idx, double *data, int *dim1, int *dim2, int *dim3, int *dim4, int *dim5, int *retDim1, int *retDim2, int *retDim3, int *retDim4, int *retDim5, int *stat, int pathLen)
{
    int status, i, j, k, l, m;
    void *obj = getObjectFromList(*handle);
    char *intPath = allocateC(path, pathLen);
    double *intData;
    int outDim1, outDim2, outDim3, outDim4, outDim5;
    status = getVect5DDoubleFromObject(*expIdx, obj, intPath, *idx-1, &intData, &outDim1, &outDim2, &outDim3, &outDim4, &outDim5);
    *stat = status;
    free(intPath);
    if(status) return status;
    *retDim1 = outDim1;
    *retDim2 = outDim2;
    *retDim3 = outDim3;
    *retDim4 = outDim4;
    *retDim5 = outDim5;
    if(*retDim1 > *dim1)
        *retDim1 = *dim1;
    if(*retDim2 > *dim2)
        *retDim2 = *dim2;
    if(*retDim3 > *dim3)
        *retDim3 = *dim3;
    if(*retDim4 > *dim4)
        *retDim4 = *dim4;
    if(*retDim5 > *dim5)
        *retDim5 = *dim5;

    for(i = 0; i < *retDim1; i++)
    {
        for(j = 0; j < *retDim2; j++)
        {
            for(k = 0; k < *retDim3; k++)
       {
           for(l = 0; l < *retDim4; l++)
      {
                    for(m = 0; m < *retDim5; m++)
                    {
                        data[i* outDim2 * outDim3 * outDim4 * outDim5 + j*outDim3 *outDim4*outDim5 + k*outDim4 * outDim5 + l * outDim5 + m] =
                            intData[i* outDim2 * outDim3 * outDim4 * outDim5 + j*outDim3 *outDim4*outDim5 + k*outDim4 * outDim5 + l * outDim5 + m];
                    }
      }
       }
        }
    }
    free((char *)intData);
    return status;
}

int get_vect6d_double_from_object_(int *expIdx, int *handle, char *path, int *idx, double *data, int *dim1, int *dim2, int *dim3, int *dim4, int *dim5, int *dim6, int *retDim1, int *retDim2, int *retDim3, int *retDim4, int *retDim5, int *retDim6, int *stat, int pathLen)
{
    int status, i, j, k, l, m, n;
    void *obj = getObjectFromList(*handle);
    char *intPath = allocateC(path, pathLen);
    double *intData;
    int outDim1, outDim2, outDim3, outDim4, outDim5, outDim6;
    status = getVect6DDoubleFromObject(*expIdx, obj, intPath, *idx-1, &intData, &outDim1, &outDim2, &outDim3, &outDim4, &outDim5, &outDim6);
    *stat = status;
    free(intPath);
    if(status) return status;
    *retDim1 = outDim1;
    *retDim2 = outDim2;
    *retDim3 = outDim3;
    *retDim4 = outDim4;
    *retDim5 = outDim5;
    *retDim6 = outDim6;
    if(*retDim1 > *dim1)
        *retDim1 = *dim1;
    if(*retDim2 > *dim2)
        *retDim2 = *dim2;
    if(*retDim3 > *dim3)
        *retDim3 = *dim3;
    if(*retDim4 > *dim4)
        *retDim4 = *dim4;
    if(*retDim5 > *dim5)
        *retDim5 = *dim5;
    if(*retDim6 > *dim6)
        *retDim6 = *dim6;

    for(i = 0; i < *retDim1; i++)
    {
        for(j = 0; j < *retDim2; j++)
        {
            for(k = 0; k < *retDim3; k++)
       {
           for(l = 0; l < *retDim4; l++)
      {
                    for(m = 0; m < *retDim5; m++)
                    {
                        for(n = 0; n < *retDim6; n++)
                        {
                         data[i* outDim2 * outDim3 * outDim4 * outDim5 * outDim6 + j*outDim3 *outDim4*outDim5 * outDim6
                            + k*outDim4 * outDim5 *outDim6 + l * outDim5 * outDim6 + m*outDim6 + n] =
                            intData[i* outDim2 * outDim3 * outDim4 * outDim5 * outDim6 + j*outDim3 *outDim4*outDim5 * outDim6
                            + k*outDim4 * outDim5 *outDim6 + l * outDim5 * outDim6 + m*outDim6 + n];
                         }
                    }
      }
       }
        }
    }
    free((char *)intData);
    return status;
}

int get_vect7d_double_from_object_(int *expIdx, int *handle, char *path, int *idx, double *data, int *dim1, int *dim2, int *dim3, int *dim4, int *dim5, int *dim6, int *dim7, int *retDim1, int *retDim2, int *retDim3, int *retDim4, int *retDim5, int *retDim6, int *retDim7, int *stat, int pathLen)
{
    int status, i, j, k, l, m, n, p;
    void *obj = getObjectFromList(*handle);
    char *intPath = allocateC(path, pathLen);
    double *intData;
    int outDim1, outDim2, outDim3, outDim4, outDim5, outDim6, outDim7;
    status = getVect7DDoubleFromObject(*expIdx, obj, intPath, *idx-1, &intData, &outDim1, &outDim2, &outDim3, &outDim4, &outDim5, &outDim6,
      &outDim7);
    *stat = status;
    free(intPath);
    if(status) return status;
    *retDim1 = outDim1;
    *retDim2 = outDim2;
    *retDim3 = outDim3;
    *retDim4 = outDim4;
    *retDim5 = outDim5;
    *retDim6 = outDim6;
    *retDim7 = outDim7;
    if(*retDim1 > *dim1)
        *retDim1 = *dim1;
    if(*retDim2 > *dim2)
        *retDim2 = *dim2;
    if(*retDim3 > *dim3)
        *retDim3 = *dim3;
    if(*retDim4 > *dim4)
        *retDim4 = *dim4;
    if(*retDim5 > *dim5)
        *retDim5 = *dim5;
    if(*retDim6 > *dim6)
        *retDim6 = *dim6;
    if(*retDim7 > *dim7)
        *retDim7 = *dim7;

    for(i = 0; i < *retDim1; i++)
    {
        for(j = 0; j < *retDim2; j++)
        {
            for(k = 0; k < *retDim3; k++)
       {
           for(l = 0; l < *retDim4; l++)
      {
                    for(m = 0; m < *retDim5; m++)
                    {
                        for(n = 0; n < *retDim6; n++)
                        {
                            for(p = 0; p < *retDim7; p++)
                            {
                           data[i* outDim2 * outDim3 * outDim4 * outDim5 * outDim6 * outDim7 + j*outDim3 *outDim4*outDim5 * outDim6 * outDim7
                                + k*outDim4 * outDim5 *outDim6 * outDim7 + l * outDim5 * outDim6 *outDim7 + m*outDim6 * outDim7  + n * outDim7 + p] =
                              intData[i* outDim2 * outDim3 * outDim4 * outDim5 * outDim6 * outDim7 + j*outDim3 *outDim4*outDim5 * outDim6 * outDim7
                                + k*outDim4 * outDim5 *outDim6 * outDim7 + l * outDim5 * outDim6 * outDim7 + m*outDim6  * outDim7+ n* outDim7 + p];
             }
                        }
                    }
      }
       }
        }
    }
    free((char *)intData);
    return status;
}

int get_object_from_object_(int *expIdx, int *handle, char *path, int *idx, int *dataHandle, int *stat, int pathLen)
{
    void *obj = getObjectFromList(*handle);
    char *intPath = allocateC(path, pathLen);
    void * objData;
    int status = getObjectFromObject(*expIdx, obj, intPath, *idx-1, &objData);
    *stat = status;
    free(intPath);
    if(status) return status;
    *dataHandle = addObjectToList(objData);
    if (*dataHandle < 0) {
        releaseObject(*expIdx,objData);
        printf("No more slots available for arrays of structures\n");
        return -1;
    }
    return 0;
}

//Array of structures Slice Management
int get_object_slice_(int *expIdx, char *cpoPath, char *path, double *time, int *handle, int *stat, int cpoPathLen, int pathLen)
{
    char *intCpoPath = allocateC(cpoPath, cpoPathLen);
    char *intPath = allocateC(path, pathLen);
    void * obj;
    int status = getObjectSlice(*expIdx, intCpoPath, intPath, *time, &obj);
    *stat = status;
    free(intPath);
    free(intCpoPath);
    if(status) return status;
    *handle = addObjectToList(obj);
    if (*handle < 0) {
        releaseObject(*expIdx,obj);
        printf("No more slots available for arrays of structures\n");
        return -1;
    }
    return 0;
}

int put_object_slice_(int *expIdx, char *cpoPath, char *path, double *time, int *handle, int *stat, int cpoPathLen, int pathLen)
{
    char *intCpoPath = allocateC(cpoPath, cpoPathLen);
    char *intPath = allocateC(path, pathLen);
    void *obj = getObjectFromList(*handle);
    int status = putObjectSlice(*expIdx, intCpoPath, intPath, *time, obj);
    free(intCpoPath);
    free(intPath);
    if(status)
        printf("%s\n",imas_last_errmsg());
    return status;
}

int replace_last_object_slice_(int *expIdx, char *cpoPath, char *path, int *handle, int cpoPathLen, int pathLen)
{
    int status;
    char *intCpoPath = allocateC(cpoPath, cpoPathLen);
    char *intPath = allocateC(path, pathLen);
    void *obj = getObjectFromList(*handle);
    status = replaceLastObjectSlice(*expIdx, intCpoPath, intPath, obj);
    free(intCpoPath);
    free(intPath);
    if(status)
        printf("%s\n",imas_last_errmsg());
    return status;
}


///////////////TEMPORARY!!!
void imas_discard_cache_(){}
void imas_flush_cache_(){}


///////////////ERROR HANDLING
int is_critical_error_(int *status)
{
    int retValue;
    retValue = isCriticalError(*status);
    return retValue;
}


int get_error_type_(int *status)
{
    int retValue;
    retValue = getErrorType(*status);
    return retValue;
}

