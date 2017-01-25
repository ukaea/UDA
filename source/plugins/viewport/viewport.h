#ifndef IDAM_PLUGINS_VIEWPORT_VIEWPORT_H
#define IDAM_PLUGINS_VIEWPORT_VIEWPORT_H

#include <include/idamplugin.h>

#ifdef __cplusplus
extern "C" {
#endif

#define THISPLUGIN_VERSION                  1
#define THISPLUGIN_MAX_INTERFACE_VERSION    1        // Interface versions higher than this will not be understood!
#define THISPLUGIN_DEFAULT_METHOD           "get"

#define MAXHANDLES          8
#define MAXSIGNALNAME       256
#define FREEHANDLEBLOCK     4

extern int viewport(IDAM_PLUGIN_INTERFACE * idam_plugin_interface);

void getVerticalPixelValues(float * values, int count, int pixelHeight, float * startValue, float * endValue,
                            float ** verticalPixelValues, float * delta);

void getBins(float * coords, int count, int pixelWidth, float minValue, float maxValue, int ** column,
             float ** pixelValues);

void reduceOrderedData(float * values, int * count, float * startValue, float * endValue, float * coords, float * min,
                       float * max);

void getBinIds(float * values, int count, int pixelHeight, float * pixelValues, int ** freq);

int whichHandle(char * signal, char * source);

#ifdef __cplusplus
}
#endif

#endif // IDAM_PLUGINS_VIEWPORT_VIEWPORT_H
