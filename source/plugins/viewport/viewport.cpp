/*---------------------------------------------------------------
* v1 IDAM Plugin viewPort: re-bin data to visualise with a rectangular viewport defined by horizonal 
* and vertical pixel ranges 
*
* Input Arguments:    IDAM_PLUGIN_INTERFACE *idam_plugin_interface
*
* Returns:        0 if the plugin functionality was successful
*            otherwise a Error Code is returned
*
* Standard functionality:
*
*---------------------------------------------------------------------------------------------------------------*/
#include "viewport.h"

#include <cstdlib>
#include <cfloat>
#ifdef __GNUC__
#  include <strings.h>
#endif

#include <client/udaGetAPI.h>
#include <client/udaClient.h>
#include <clientserver/initStructs.h>
#include <client/accAPI.h>
#include <clientserver/udaTypes.h>
#include <clientserver/stringUtils.h>

static int handleCount = 0;
static int handles[MAXHANDLES];
static char signals[MAXHANDLES][MAXMETA];
static char sources[MAXHANDLES][STRING_LENGTH];

int whichHandle(char* signal, char* source)
{
    for (int i = 0; i < handleCount; i++)
        if (STR_IEQUALS(signals[i], signal) && STR_IEQUALS(sources[i], source)) {
            return handles[i];
        }
    return -1;        // Not found
}


extern int viewport(IDAM_PLUGIN_INTERFACE* idam_plugin_interface)
{
    int err = 0;
    char* p;

    static short init = 0;

    //----------------------------------------------------------------------------------------
    // Standard v1 Plugin Interface

    unsigned short housekeeping;

    if (idam_plugin_interface->interfaceVersion > THISPLUGIN_MAX_INTERFACE_VERSION) {
        RAISE_PLUGIN_ERROR("Plugin Interface Version Unknown to this plugin: Unable to execute the request!");
    }

    idam_plugin_interface->pluginVersion = THISPLUGIN_VERSION;

    DATA_BLOCK* data_block = idam_plugin_interface->data_block;
    REQUEST_DATA* request = idam_plugin_interface->request_data;

    housekeeping = idam_plugin_interface->housekeeping;

    //----------------------------------------------------------------------------------------
    // Context relevent Name-Value pairs and keywords

    bool isStartValueX = false;
    bool isEndValueX = false;
    bool isStartValueY = false;
    bool isEndValueY = false;
    bool isPixelHeight = false;
    bool isPixelWidth = false;
    int pixelHeight = 0;
    int pixelWidth = 0;
    bool isClearCache = false;
    bool isMean = false;
    bool isMode = false;
    bool isMedian = false;
    bool isTest = false;
    bool isRange = true;

    int test = 0;
    char* signal = nullptr;
    char* source = nullptr;
    float startValueX = 0.0;
    float endValueX = 0.0;
    float startValueY = 0.0;
    float endValueY = 0.0;

    {
        for (int i = 0; i < request->nameValueList.pairCount; i++) {
            if (STR_IEQUALS(request->nameValueList.nameValue[i].name, "signal")) {
                signal = request->nameValueList.nameValue[i].value;
                continue;
            }
            if (STR_IEQUALS(request->nameValueList.nameValue[i].name, "source")) {
                source = request->nameValueList.nameValue[i].value;
                continue;
            }
            if (STR_IEQUALS(request->nameValueList.nameValue[i].name, "startValueX")) {
                isStartValueX = true;
                startValueX = (float)atof(request->nameValueList.nameValue[i].value);
            }
            if (STR_IEQUALS(request->nameValueList.nameValue[i].name, "endValueX")) {
                isEndValueX = true;
                endValueX = (float)atof(request->nameValueList.nameValue[i].value);
            }
            if (STR_IEQUALS(request->nameValueList.nameValue[i].name, "startValueY")) {
                isStartValueY = true;
                startValueY = (float)atof(request->nameValueList.nameValue[i].value);
            }
            if (STR_IEQUALS(request->nameValueList.nameValue[i].name, "endValueY")) {
                isEndValueY = true;
                endValueY = (float)atof(request->nameValueList.nameValue[i].value);
            }
            if (STR_IEQUALS(request->nameValueList.nameValue[i].name, "pixelHeight")) {
                isPixelHeight = true;
                pixelHeight = atoi(request->nameValueList.nameValue[i].value);
                continue;
            }
            if (STR_IEQUALS(request->nameValueList.nameValue[i].name, "pixelWidth")) {
                isPixelWidth = true;
                pixelWidth = atoi(request->nameValueList.nameValue[i].value);
                continue;
            }
            if (STR_IEQUALS(request->nameValueList.nameValue[i].name, "clearCache")) {
                isClearCache = true;
                continue;
            }
            if (STR_IEQUALS(request->nameValueList.nameValue[i].name, "test")) {
                isTest = true;
                test = atoi(request->nameValueList.nameValue[i].value);
                continue;
            }
            if (STR_IEQUALS(request->nameValueList.nameValue[i].name, "mean")) {
                isMean = true;
                continue;
            }
            if (STR_IEQUALS(request->nameValueList.nameValue[i].name, "mode")) {
                isMode = true;
                continue;
            }
            if (STR_IEQUALS(request->nameValueList.nameValue[i].name, "median")) {
                isMedian = true;
                continue;
            }
            if (STR_IEQUALS(request->nameValueList.nameValue[i].name, "range")) {
                isRange = true;
                continue;
            }
        }
    }

    //----------------------------------------------------------------------------------------
    // Heap Housekeeping

    // Plugin must maintain a list of open file handles and sockets: loop over and close all files and sockets
    // Plugin must maintain a list of plugin functions called: loop over and reset state and free heap.
    // Plugin must maintain a list of calls to other plugins: loop over and call each plugin with the housekeeping request
    // Plugin must destroy lists at end of housekeeping

    // A plugin only has a single instance on a server. For multiple instances, multiple servers are needed.
    // Plugins can maintain state so recursive calls (on the same server) must respect this.
    // If the housekeeping action is requested, this must be also applied to all plugins called.
    // A list must be maintained to register these plugin calls to manage housekeeping.
    // Calls to plugins must also respect access policy and user authentication policy

    if (housekeeping || STR_IEQUALS(request->function, "reset")) {

        if (!init) return 0;        // Not previously initialised: Nothing to do!

        // Free Heap & reset counters
        
        for (int i = 0; i < handleCount; i++) {
            idamFree(handles[i]);
            handles[i] = -1;
        }

        init = 0;
        handleCount = 0;

        return 0;
    }

    if (STR_IEQUALS(request->function, "clearCache") || isClearCache) {

        // Free Cached data if requested or filled
        
        for (int i = 0; i < handleCount; i++) {
            idamFree(handles[i]);
            handles[i] = -1;
        }

        handleCount = 0;

        return 0;
    }

    if (handleCount == MAXHANDLES) {
        // Free some Cached data if cache is full
        for (int i = 0; i < FREEHANDLEBLOCK; i++) {
            idamFree(handles[i]);
            handles[i] = handles[i + FREEHANDLEBLOCK];
            strcpy(signals[i], signals[i + FREEHANDLEBLOCK]);
            strcpy(sources[i], sources[i + FREEHANDLEBLOCK]);
        }

        handleCount = handleCount - FREEHANDLEBLOCK;

        return 0;
    }

    //----------------------------------------------------------------------------------------
    // Initialise

    if (!init || STR_IEQUALS(request->function, "init")
        || STR_IEQUALS(request->function, "initialise")) {

        // Free Heap Cache & reset counters
        for (int i = 0; i < handleCount; i++) {
            idamFree(handles[i]);
            handles[i] = -1;
        }

        handleCount = 0;

        init = 1;
        if (STR_IEQUALS(request->function, "init") || STR_IEQUALS(request->function, "initialise")) {
            return 0;
        }
    }

    //----------------------------------------------------------------------------------------
    // Plugin Functions
    //----------------------------------------------------------------------------------------

    do {

        // Help: A Description of library functionality

        if (STR_IEQUALS(request->function, "help")) {

            p = (char*)malloc(sizeof(char) * 2 * 1024);

            strcpy(p, "\nviewport: Add Functions Names, Syntax, and Descriptions\n\n");

            initDataBlock(data_block);

            data_block->rank = 1;
            data_block->dims = (DIMS*)malloc(data_block->rank * sizeof(DIMS));
            for (unsigned int i = 0; i < data_block->rank; i++) {
                initDimBlock(&data_block->dims[i]);
            }

            data_block->data_type = UDA_TYPE_STRING;
            strcpy(data_block->data_desc, "viewport: help = description of this plugin");

            data_block->data = (char*)p;

            data_block->dims[0].data_type = UDA_TYPE_UNSIGNED_INT;
            data_block->dims[0].dim_n = strlen(p) + 1;
            data_block->dims[0].compressed = 1;
            data_block->dims[0].dim0 = 0.0;
            data_block->dims[0].diff = 1.0;
            data_block->dims[0].method = 0;

            data_block->data_n = data_block->dims[0].dim_n;

            strcpy(data_block->data_label, "");
            strcpy(data_block->data_units, "");

            break;
        } else

            //----------------------------------------------------------------------------------------
            // Standard methods: version, builddate, defaultmethod, maxinterfaceversion

        if (STR_IEQUALS(request->function, "version")) {
            initDataBlock(data_block);
            data_block->data_type = UDA_TYPE_INT;
            data_block->rank = 0;
            data_block->data_n = 1;
            int* data = (int*)malloc(sizeof(int));
            data[0] = THISPLUGIN_VERSION;
            data_block->data = (char*)data;
            strcpy(data_block->data_desc, "Plugin version number");
            strcpy(data_block->data_label, "version");
            strcpy(data_block->data_units, "");
            break;
        } else

            // Plugin Build Date

        if (STR_IEQUALS(request->function, "builddate")) {
            initDataBlock(data_block);
            data_block->data_type = UDA_TYPE_STRING;
            data_block->rank = 0;
            data_block->data_n = strlen(__DATE__) + 1;
            char* data = (char*)malloc(data_block->data_n * sizeof(char));
            strcpy(data, __DATE__);
            data_block->data = (char*)data;
            strcpy(data_block->data_desc, "Plugin build date");
            strcpy(data_block->data_label, "date");
            strcpy(data_block->data_units, "");
            break;
        } else

            // Plugin Default Method

        if (STR_IEQUALS(request->function, "defaultmethod")) {
            initDataBlock(data_block);
            data_block->data_type = UDA_TYPE_STRING;
            data_block->rank = 0;
            data_block->data_n = strlen(THISPLUGIN_DEFAULT_METHOD) + 1;
            char* data = (char*)malloc(data_block->data_n * sizeof(char));
            strcpy(data, THISPLUGIN_DEFAULT_METHOD);
            data_block->data = (char*)data;
            strcpy(data_block->data_desc, "Plugin default method");
            strcpy(data_block->data_label, "method");
            strcpy(data_block->data_units, "");
            break;
        } else

            // Plugin Maximum Interface Version

        if (STR_IEQUALS(request->function, "maxinterfaceversion")) {
            initDataBlock(data_block);
            data_block->data_type = UDA_TYPE_INT;
            data_block->rank = 0;
            data_block->data_n = 1;
            int* data = (int*)malloc(sizeof(int));
            data[0] = THISPLUGIN_MAX_INTERFACE_VERSION;
            data_block->data = (char*)data;
            strcpy(data_block->data_desc, "Maximum Interface Version");
            strcpy(data_block->data_label, "version");
            strcpy(data_block->data_units, "");
            break;
        } else


            //----------------------------------------------------------------------------------------
            // Get data and map to the specified viewport
            // Viewport Resolution Aggregation

            // Real world coordinates: startValueX, endValueX, startValueY, endValueY
            // Device coordinates (relative) pixelWidth, pixelHeight

        if (STR_IEQUALS(request->function, "get")) {

            // Context based Tests: required - pixelWidth, pixelHeight, Signal, Source

            // Access data if the signal/source data are not cached

            int handle = whichHandle(signal, source);

            if (handle < 0) {

                if ((handle = idamGetAPI(signal, source)) < 0 || getIdamErrorCode(handle) != 0) {
                    err = 999;
                    addIdamError(UDA_CODE_ERROR_TYPE, "viewPort", err, (char*)getIdamErrorMsg(handle));
                    break;
                }

                handles[handleCount] = handle;
                strcpy(signals[handleCount], signal);
                strcpy(sources[handleCount], source);
                handleCount++;
            }

            // Get the data rank and data shape, the data and the coordinates

            int rank = getIdamRank(handle);
            int order = getIdamOrder(handle);

            if (rank == 1) {
                int count = getIdamDataNum(handle);

                auto values = (float*)malloc(count * sizeof(float));
                auto coords = (float*)malloc(count * sizeof(float));

                getIdamFloatData(handle, values);
                getIdamFloatDimData(handle, 0, coords);

                if (isTest) {
                    UDA_LOG(UDA_LOG_DEBUG, "Running Viewport Test %d\n", test);

                    switch (test) {
                        case 1: {                // Do nothing
                            break;
                        }
                        case 2: {                // 100 values mapped to 100 pixels: each pixel column has a single data value
                            isPixelWidth = true;
                            pixelWidth = 100;
                            break;
                        }
                        case 3: {                // 100 values mapped to 100 pixels: each pixel column has a single data value
                            isPixelHeight = true;
                            pixelHeight = 100;
                            break;
                        }
                        case 4: {                // 100 values mapped to 100 pixels: each pixel column has a single data value
                            isPixelHeight = true;
                            isPixelWidth = true;
                            pixelHeight = 100;
                            pixelWidth = 100;
                            break;
                        }

                        case 5: {                // 100 values mapped to 100 pixels: each pixel column has a single data value
                            count = 101;
                            for (int i = 0; i < count; i++) {
                                values[i] = (float)i + 1;
                                coords[i] = (float)i + 1;
                            }
                            isPixelHeight = true;
                            isPixelWidth = true;
                            pixelHeight = 100;
                            pixelWidth = 100;
                            break;
                        }

                        case 6: {                // 50 values mapped to 100 pixels: alternate pixels have no data!
                            count = 201;
                            for (int i = 0; i < count; i++) {
                                values[i] = (float)i + 1;
                                coords[i] = (float)i + 1;
                            }
                            isStartValueX = true;
                            startValueX = 151.0;
                            isPixelHeight = true;
                            isPixelWidth = true;
                            pixelHeight = 100;
                            pixelWidth = 100;
                            break;
                        }
                        case 7: {                // 50 values mapped to 100 pixels: alternate pixels have no data!
                            count = 201;
                            for (int i = 0; i < count; i++) {
                                values[i] = (float)i + 1;
                                coords[i] = (float)i + 1;
                            }
                            isStartValueX = true;
                            startValueY = 151.0;
                            isPixelHeight = true;
                            isPixelWidth = true;
                            pixelHeight = 100;
                            pixelWidth = 100;
                            break;
                        }
                    }
                }

                // Map data to the viewport

                // Real-world coordinate Width Scenarios
                // 1> no start or end value set
                // 2> start value set
                // 3> end value set
                // 4> start and end value set

                // Issues
                // 1> Insufficient data to fill some pixels
                // 2> requested range lies outside the data's range


                // Reduce data to fit the value ranges specified
                // Assume X data (coordinates) are ordered in increasing value

                float maxValueY = values[0];
                float minValueY = values[0];
                float maxValueX = coords[count - 1];
                float minValueX = values[0];

                // Reduce the ordered X data's width

                if (!isStartValueX && !isEndValueX) {
                    reduceOrderedData(coords, &count, nullptr, nullptr, values, &minValueX, &maxValueX);
                }
                if (!isStartValueX && isEndValueX) {
                    reduceOrderedData(coords, &count, nullptr, &endValueX, values, &minValueX, &maxValueX);
                }
                if (isStartValueX && !isEndValueX) {
                    reduceOrderedData(coords, &count, &startValueX, nullptr, values, &minValueX, &maxValueX);
                }
                if (isStartValueX && isEndValueX) {
                    reduceOrderedData(coords, &count, &startValueX, &endValueX, values, &minValueX, &maxValueX);
                }

                // Range of Y data

                if (!isStartValueY) {
                    minValueY = FLT_MAX;
                    for (int j = 0; j < count; j++) {
                        if (values[j] < minValueY) minValueY = values[j];
                    }
                } else {
                    minValueY = startValueY;
                }

                if (!isEndValueY) {
                    maxValueY = -FLT_MAX;
                    for (int j = 0; j < count; j++) {
                        if (values[j] > maxValueY) maxValueY = values[j];
                    }
                } else {
                    maxValueY = endValueY;
                }

                // Reduce unordered Y data: remove points outside the range

                if (isStartValueY || isEndValueY) {
                    int newCount = 0;
                    int* remove = (int*)malloc(count * sizeof(int));
                    for (int j = 0; j < count; j++) {
                        if (values[j] < minValueY || values[j] > maxValueY) {
                            remove[j] = 1;
                        } else {
                            remove[j] = 0;
                            newCount++;
                        }
                    }

                    if (newCount < count) {
                        int id = 0;
                        auto newValues = (float*)malloc(newCount * sizeof(float));
                        auto newCoords = (float*)malloc(newCount * sizeof(float));

                        for (int j = 0; j < count; j++) {
                            if (!remove[j]) {
                                newValues[id] = values[j];
                                newCoords[id++] = coords[j];
                            }
                        }

                        count = newCount;
                        free(values);
                        free(coords);

                        values = newValues;
                        coords = newCoords;

                    }

                    free(remove);
                }

                // Reduce data to fix the device coordinates (relative pixels)

                // Coordinate index range of each horizontal pixel column
                // Each pixel has data points

                float* data = nullptr;
                float* errhi = nullptr;
                float* errlo = nullptr;
                int pixelWidth2 = 0;
                float* horizontalPixelValues = nullptr;        // Value at the pixel center

                if (isPixelWidth && isPixelHeight) {
                    // Map to pixels if the device coordinate viewport is defined

                    UDA_LOG(UDA_LOG_DEBUG,
                            "Viewport: Mapping data to device pixel coordinate range (width, height) = %d, %d\n",
                            pixelWidth, pixelHeight);

                    int* column = nullptr;

                    // Assign coordinates to pixel columns

                    getBins(coords, count, pixelWidth, (double)minValueX, (double)maxValueX, &column,
                            &horizontalPixelValues);

                    // Frequency distribution of pixel hits along each vertical pixel column

                    data = (float*)malloc(pixelWidth * sizeof(float));
                    errhi = (float*)malloc(pixelWidth * sizeof(float));
                    errlo = (float*)malloc(pixelWidth * sizeof(float));

                    int* good = (int*)malloc(pixelWidth * sizeof(int));

                    float* verticalPixelValues = nullptr;            // Value at the pixel center
                    float delta;

                    getVerticalPixelValues(values, count, pixelHeight, &minValueY, &maxValueY, &verticalPixelValues,
                                           &delta);

                    auto verticalPixelBoundaries = (float*)malloc((pixelHeight + 2) * sizeof(float));
                    verticalPixelBoundaries[0] = minValueY;
                    
                    for (int i = 1; i < pixelHeight + 2; i++) {
                        // Lower boundary of pixel cell
                        verticalPixelBoundaries[i] = verticalPixelBoundaries[i - 1] + delta;
                    }

                    // frequency distribution of values within each pixel column

                    int* row = (int*)malloc(count * sizeof(int));
                    int* fctot = (int*)malloc(pixelWidth * sizeof(int));
                    int* frtot = (int*)malloc(pixelHeight * sizeof(int));
                    int** freq = (int**)malloc(pixelWidth * sizeof(int*));

                    for (int i = 0; i < pixelWidth; i++) {
                        fctot[i] = 0;
                        freq[i] = (int*)malloc(pixelHeight * sizeof(int));
                        for (int j = 0; j < pixelHeight; j++) {
                            freq[i][j] = 0;
                        }
                    }

                    for (int i = 0; i < count; i++) {
                        fctot[column[i]]++;        // total counts
                    }

                    int colCount = 0;
                    for (int i = 0; i < pixelWidth; i++) {
                        colCount = colCount + fctot[i];
                    }

                    UDA_LOG(UDA_LOG_DEBUG, "Column Totals: %d\n", colCount);
                    for (int i = 0; i < pixelWidth; i++) {
                        UDA_LOG(UDA_LOG_DEBUG, "[%d] %d\n", i, fctot[i]);
                    }

                    // Which pixel row bin do each un-ordered data point fall into?

                    for (int j = 0; j < count; j++) {
                        row[j] = -1;
                        for (int i = 0; i < pixelHeight + 1; i++) {        // Search within pixel boundaries
                            if (values[j] >= verticalPixelBoundaries[i] && values[j] < verticalPixelBoundaries[i + 1]) {
                                row[j] = i;
                                break;
                            }
                        }
                        if (row[j] > pixelHeight - 1) row[j] = pixelHeight - 1;

                        if (column[j] >= 0 && row[j] >= 0) freq[column[j]][row[j]]++;    // build frequency distribution
                    }

                    for (int i = 0; i < pixelHeight; i++) {
                        frtot[i] = 0;
                    }

                    for (int i = 0; i < count; i++) {
                        frtot[row[i]]++;        // total counts
                    }

                    int rowCount = 0;
                    for (int i = 0; i < pixelHeight; i++) {
                        rowCount = rowCount + frtot[i];
                    }

                    UDA_LOG(UDA_LOG_DEBUG, "Row Totals: %d\n", rowCount);
                    for (int i = 0; i < pixelHeight; i++) {
                        UDA_LOG(UDA_LOG_DEBUG, "[%d] %d\n", i, frtot[i]);
                    }

                    free(column);
                    free(row);
                    free(fctot);
                    free(frtot);

                    // Build return values

                    int goodCount = 0;
                    pixelWidth2 = pixelWidth;
                    int* integral = (int*)malloc(pixelHeight * sizeof(int));

                    for (int i = 0; i < pixelWidth; i++) {

                        // which value to return?
                        // mean, median or mode?
                        // with/without errors - standard deviation, asymmetric?
                        // 2D pixel map with colour for frequency - limited pallet? 1 byte == 256 colours

                        good[i] = 0;
                        data[i] = 0.0;
                        errlo[i] = 0.0;
                        errhi[i] = 0.0;
                        int meanCount = 0;

                        if (!isRange && isMean) {
                            if (i == 0) {
                                UDA_LOG(UDA_LOG_DEBUG, "Mean returned\n");
                            }
                            for (int j = 0; j < pixelHeight; j++) {
                                if (freq[i][j] > 0) {
                                    data[i] = data[i] + (float)freq[i][j] * verticalPixelValues[j];
                                    meanCount = meanCount + freq[i][j];
                                }
                            }
                            if (meanCount > 0) {
                                data[i] = data[i] / (float)meanCount;
                                good[i] = 1;
                                goodCount++;
                            }

                        } else if (!isRange && isMode) {
                            UDA_LOG(UDA_LOG_DEBUG, "Mode returned\n");
                            int fmax = 0;
                            int fmaxID = -1;
                            for (int j = 0; j < pixelHeight; j++) {
                                if (freq[i][j] > fmax) {        // First mode found if multi-modal
                                    fmaxID = j;
                                    fmax = freq[i][j];
                                }
                            }
                            if (fmaxID >= 0) {
                                data[i] = verticalPixelValues[fmaxID];
                                good[i] = 1;
                                goodCount++;
                            }
                        } else if (!isRange && isMedian) {
                            if (i == 0) {
                                UDA_LOG(UDA_LOG_DEBUG, "Median returned\n");
                            }
                            integral[0] = freq[i][0];

                            for (int j = 1; j < pixelHeight; j++) {
                                integral[j] = integral[j - 1] + freq[i][j];
                            }

                            int target = integral[pixelHeight - 1] / 2;

                            for (int j = 0; j < pixelHeight; j++) {
                                if (integral[j] >= target) {
                                    if (1 && (j > 0 && integral[j - 1] > 0 && integral[j] != target)) {
                                        float dx = (float)(integral[j] - integral[j - 1]);
                                        if (dx != 0.0) {
                                            float m = (verticalPixelValues[j] - verticalPixelValues[j - 1]) / dx;
                                            data[i] = m * (target - integral[j]) +
                                                      verticalPixelValues[j];            // Linear Interpolate
                                        } else {
                                            data[i] = verticalPixelValues[j];
                                        }

                                    } else {
                                        if (j > 0 && integral[j - 1] > 0 &&
                                            (target - integral[j - 1]) <= (integral[j] - target)) {
                                            data[i] = verticalPixelValues[j - 1];
                                        } else {
                                            data[i] = verticalPixelValues[j];
                                        }
                                    }
                                    good[i] = 1;
                                    goodCount++;

                                    break;
                                }
                            }
                        }

                        // Range of data values - Above and below the mean/mode/median value

                        errhi[i] = 0.0;
                        errlo[i] = 0.0;

                        for (int j = 0; j < pixelHeight; j++) {
                            if (freq[i][j] > 0) {
                                errlo[i] = verticalPixelValues[j];    // lowest value
                                break;
                            }
                        }

                        for (int j = pixelHeight - 1; j >= 0; j--) {
                            if (freq[i][j] > 0) {
                                errhi[i] = verticalPixelValues[j];    // highest value
                                break;
                            }
                        }

                        if (isRange) {
                            if (i == 0) {
                                UDA_LOG(UDA_LOG_DEBUG, "Range returned\n");
                            }
                            data[i] = 0.5 * (errlo[i] + errhi[i]);
                            goodCount++;
                        }

                        free(freq[i]);

                        if (i == 0) {
                            UDA_LOG(UDA_LOG_DEBUG, "&data = %p\n", data);
                        }
                        UDA_LOG(UDA_LOG_DEBUG, "[%d]   %f   %f   %f   %f\n", i, data[i], errlo[i], errhi[i],
                                horizontalPixelValues[i]);

                    }   // end of loop over pixelWidth

                    UDA_LOG(UDA_LOG_DEBUG, "goodCount  = %d\n", goodCount);
                    UDA_LOG(UDA_LOG_DEBUG, "pixelWidth = %d\n", pixelWidth);
                    UDA_LOG(UDA_LOG_DEBUG, "&data = %p\n", data);
                    for (int i = 0; i < pixelWidth2; i++) {
                        UDA_LOG(UDA_LOG_DEBUG, "[%d]   %f   %f   %f   %f\n", i, data[i], errlo[i], errhi[i],
                                horizontalPixelValues[i]);
                    }
                    // Free allocated heap

                    free(values);
                    free(coords);
                    free(freq);
                    free(verticalPixelValues);
                    free(verticalPixelBoundaries);
                    free(integral);

                    // data, errhi, errlo, horizontalPixelValues heap freed once the server transmits the data
                    // heap within the idam layer is freed on reset or clearCache or if the cache fills

                    // Remove pixel columns without data

                    if (goodCount < pixelWidth) {
                        UDA_LOG(UDA_LOG_DEBUG, "Removing pixel columns without data [%d, %d]\n", goodCount, pixelWidth);
                        auto newData = (float*)malloc(goodCount * sizeof(float));
                        auto newErrhi = (float*)malloc(goodCount * sizeof(float));
                        auto newErrlo = (float*)malloc(goodCount * sizeof(float));
                        auto pixelValues = (float*)malloc(goodCount * sizeof(float));
                        int ID = 0;
                        for (int i = 0; i < pixelWidth; i++) {
                            if (good[i]) {
                                newData[ID] = data[i];
                                newErrhi[ID] = errlo[i];
                                newErrlo[ID] = errhi[i];
                                pixelValues[ID] = horizontalPixelValues[i];
                                ID++;
                            }
                        }
                        free(good);

                        free(data);
                        free(errlo);
                        free(errhi);
                        free(horizontalPixelValues);

                        data = newData;
                        errlo = newErrlo;
                        errhi = newErrhi;
                        horizontalPixelValues = pixelValues;
                        pixelWidth2 = goodCount;
                    }
                } else {                    // Device pixel coordinate range not set
                    data = values;
                    horizontalPixelValues = coords;
                    pixelWidth2 = count;
                }

                // Return viewport data

                initDataBlock(data_block);

                data_block->rank = 1;
                data_block->dims = (DIMS*)malloc(data_block->rank * sizeof(DIMS));
                for (unsigned int i = 0; i < data_block->rank; i++) {
                    initDimBlock(&data_block->dims[i]);
                }

                data_block->data_n = pixelWidth2;
                data_block->data_type = UDA_TYPE_FLOAT;
                strcpy(data_block->data_desc, getIdamDataDesc(handle));
                strcpy(data_block->data_label, getIdamDataLabel(handle));
                strcpy(data_block->data_units, getIdamDataUnits(handle));

                data_block->dims[0].dim = (char*)horizontalPixelValues;

                data_block->dims[0].data_type = UDA_TYPE_FLOAT;
                data_block->dims[0].dim_n = pixelWidth2;
                data_block->dims[0].compressed = 0;
                strcpy(data_block->dims[0].dim_label, getIdamDimLabel(handle, 0));
                strcpy(data_block->dims[0].dim_units, getIdamDimUnits(handle, 0));

                data_block->data = (char*)data;

                if (errhi != nullptr && errlo != nullptr) {
                    if (!isRange) {
                        data_block->errasymmetry = 1;
                        data_block->errlo = (char*)errlo;
                    } else {
                        free(errlo);
                    }
                    data_block->errhi = (char*)errhi;
                    data_block->error_type = UDA_TYPE_FLOAT;
                }

                data_block->order = order;


            } else {
                err = 999;
                addIdamError(UDA_CODE_ERROR_TYPE, "viewPort", err, "A viewport for rank > 1 data has not been implemented!");
                break;
            }

            break;
        } else {

            //======================================================================================
            // Error ...

            err = 999;
            addIdamError(UDA_CODE_ERROR_TYPE, "viewport", err, "Unknown function requested!");
            break;
        }

    } while (0);

    return err;
}


void getBins(float* coords, int count, int pixelWidth, float minValue, float maxValue, int** column,
             float** pixelValues)
{
    int start;
    int* bin = (int*)malloc(count * sizeof(int));
    float* pixValues = (float*)malloc((pixelWidth + 2) * sizeof(float));        // Include an extra point

    // Value width of each pixel

    float delta = (maxValue - minValue) / (float)pixelWidth;

    // lower pixel boundary value (pixelWidth + 1 boundaries + extra point)

    pixValues[0] = minValue;
    for (int i = 1; i < pixelWidth + 2; i++) {
        pixValues[i] = pixValues[i - 1] + delta;
    }

    // Which pixel column bin do each data point fall into?

    start = 0;
    for (int j = 0; j < count; j++) {
        bin[j] = -1;
        for (int i = start; i < pixelWidth + 1; i++) {        // Search within pixel boundaries
            if (coords[j] >= pixValues[i] && coords[j] < pixValues[i + 1]) {
                bin[j] = i;
                start = i;
                break;
            }
        }
        if (bin[j] > pixelWidth - 1) bin[j] = pixelWidth - 1;
    }

    // mid pixel value: the range of data is mapped to these pixels

    pixValues[0] = minValue + 0.5 * delta;
    for (int i = 1; i < pixelWidth; i++) pixValues[i] = pixValues[i - 1] + delta;

    *column = bin;
    *pixelValues = pixValues;
}

// Mid-Pixel value

void getVerticalPixelValues(float* values, int count, int pixelHeight, float* startValue, float* endValue,
                            float** verticalPixelValues, float* delta)
{
    auto v = (float*)malloc(pixelHeight * sizeof(float));
    float maxValue = values[0], minValue = values[0];

    if (startValue == nullptr) {
        for (int i = 0; i < count; i++) {
            if (values[i] < minValue) minValue = values[i];
        }
    } else {
        minValue = *startValue;
    }

    if (endValue == nullptr) {
        for (int i = 0; i < count; i++) {
            if (values[i] > maxValue) maxValue = values[i];
        }
    } else {
        maxValue = *endValue;
    }

    *delta = (maxValue - minValue) / (float)pixelHeight;
    v[0] = minValue + 0.5 * (*delta);
    for (int i = 1; i < pixelHeight; i++) {
        v[i] = v[i - 1] + *delta;
    }
    *verticalPixelValues = v;
}

// Assume data are arranged in no particular value order

void getBinIds(float* values, int count, int pixelHeight, float* pixelValues, int** freq)
{
    auto f = (int*)malloc(pixelHeight * sizeof(int));
    for (int i = 0; i < pixelHeight; i++) {
        f[i] = 0;
    }
    for (int i = 0; i < count; i++) {
        for (int j = 0; j < pixelHeight; j++) {
            if (values[i] >= pixelValues[j] && values[i] <= pixelValues[j]) f[j]++;
        }
    }
    *freq = f;
}

void reduceOrderedData(float* values, int* count, float* startValue, float* endValue, float* coords, float* min,
                       float* max)
{
    // if the data are ordered, use this knowledge to simplify the search
    float maxValue = values[0], minValue = values[0];
    int startID = 0, endID = (*count) - 1;
    int oldCount = *count;

    // Identify the data range by index value

    startID = 0;
    minValue = values[startID];

    if (startValue != nullptr) {
        for (int j = 1; j < oldCount; j++) {
            if (values[j] > *startValue) {
                minValue = values[j - 1];
                startID = j - 1;
                break;
            }
        }
    }

    endID = oldCount - 1;
    maxValue = values[endID];

    if (endValue != nullptr) {
        endID = 0;
        maxValue = values[endID];
        for (int j = 0; j < oldCount; j++) {
            if (values[j] > *endValue) {
                maxValue = values[j - 1];
                endID = j - 1;
                break;
            }
        }
    }

    // Reduce the data

    if (startID > 0 || endID < oldCount - 1) {
        *count = endID - startID + 1;
        if (startID > 0) {
            for (int j = 0; j < *count; j++) {
                coords[j] = coords[startID + j];
                values[j] = values[startID + j];
            }
        }
    }

    *min = minValue;
    *max = maxValue;
}


