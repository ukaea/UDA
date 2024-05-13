/*---------------------------------------------------------------
 * v1 IDAM Plugin viewPort: re-bin data to visualise with a rectangular viewport defined by horizonal
 * and vertical pixel ranges
 *
 * Input Arguments:    UDA_PLUGIN_INTERFACE *plugin_interface
 *
 * Returns:        0 if the plugin functionality was successful
 *            otherwise a Error Code is returned
 *
 * Standard functionality:
 *
 *---------------------------------------------------------------------------------------------------------------*/
#include "viewport.h"

#include <cfloat>
#include <cstdlib>
#ifdef __GNUC__
#  include <strings.h>
#endif

#include "common/stringUtils.h"
#include <uda/uda_plugin_base.hpp>
#include <uda/client.h>

#include <boost/filesystem.hpp>
#include <vector>

UDA_PLUGIN_INFO UDA_PLUGIN_INFO_FUNCTION_NAME()
{
    UDA_PLUGIN_INFO info;
    info.name = "VIEWPORT";
    info.version = "1.0";
    info.entry_function = "viewport";
    info.type = UDA_PLUGIN_CLASS_FUNCTION;
    info.extension = "";
    info.default_method = "help";
    info.description = "Reduce data to viewport pixel size";
    info.cache_mode = UDA_PLUGIN_CACHE_MODE_OK;
    info.is_private = false;
    info.interface_version = 1;
    return info;
}

struct CacheEntry {
    int handle;
    std::string signal;
    std::string source;
};

class ViewportPlugin : public UDAPluginBase
{
  public:
    ViewportPlugin();
    int get(UDA_PLUGIN_INTERFACE* plugin_interface);
    void init(UDA_PLUGIN_INTERFACE* plugin_interface) override {}
    void reset() override {}

  private:
    std::vector<CacheEntry> cache_;
    int find_handle(const std::string& signal, const std::string& source);
};

ViewportPlugin::ViewportPlugin()
    : UDAPluginBase("VIEWPORT", 1, "function",
                    boost::filesystem::path(__FILE__).parent_path().append("help.txt").string())
{
    register_method("get", static_cast<UDAPluginBase::plugin_member_type>(&ViewportPlugin::get));
}

std::vector<float> getBins(float* coords, int count, int pixel_width, float minValue, float maxValue, int** column);
void reduceOrderedData(float* values, int* count, float* startValue, float* endValue, float* coords, float* min,
                       float* max);
void getVerticalPixelValues(float* values, int count, int pixel_height, float* startValue, float* endValue,
                            float** verticalPixelValues, float* delta);

int ViewportPlugin::find_handle(const std::string& signal, const std::string& source)
{
    for (const auto& entry : cache_) {
        if (entry.signal == signal && entry.source == source) {
            return entry.handle;
        }
    }
    return -1;
}

extern "C" int viewport(UDA_PLUGIN_INTERFACE* plugin_interface)
{
    static ViewportPlugin plugin = {};
    return plugin.call(plugin_interface);
}

int ViewportPlugin::get(UDA_PLUGIN_INTERFACE* plugin_interface)
{
    // Context based Tests: required - pixel_width, pixel_height, Signal, Source

    // Access data if the signal/source data are not cached

    std::string signal = find_required_arg<std::string>(plugin_interface, "signal");
    std::string source = find_required_arg<std::string>(plugin_interface, "source");
    auto test = find_arg<int>(plugin_interface, "test");
    auto pixel_width = find_arg<int>(plugin_interface, "pixel_width");
    auto pixel_height = find_arg<int>(plugin_interface, "pixel_height");
    auto start_x = find_arg<float>(plugin_interface, "start_x");
    auto end_x = find_arg<float>(plugin_interface, "end_x");
    auto start_y = find_arg<float>(plugin_interface, "start_y");
    auto end_y = find_arg<float>(plugin_interface, "end_y");
    auto range = find_arg<int>(plugin_interface, "range");
    auto mean = has_arg(plugin_interface, "mean");
    auto median = has_arg(plugin_interface, "mean");
    auto mode = has_arg(plugin_interface, "mean");

    int handle = find_handle(signal, source);

    if (handle < 0) {

        if ((handle = udaGetAPI(signal.c_str(), source.c_str())) < 0 || udaGetErrorCode(handle) != 0) {
            error(plugin_interface, udaGetErrorMsg(handle));
        }

        cache_.emplace_back(CacheEntry{handle, signal, source});
    }

    // Get the data rank and data shape, the data and the coordinates

    int rank = udaGetRank(handle);
    int order = udaGetOrder(handle);

    if (rank == 1) {
        int count = udaGetDataNum(handle);

        std::vector<float> values(static_cast<size_t>(count));
        std::vector<float> coords(static_cast<size_t>(count));

        udaGetFloatData(handle, values.data());
        udaGetFloatDimData(handle, 0, coords.data());

        if (test) {
            debug(plugin_interface, "Running Viewport Test {}\n", *test);

            switch (*test) {
                case 1: { // Do nothing
                    break;
                }
                case 2: { // 100 values mapped to 100 pixels: each pixel column has a single data value
                    pixel_width = 100;
                    break;
                }
                case 3: { // 100 values mapped to 100 pixels: each pixel column has a single data value
                    pixel_height = 100;
                    break;
                }
                case 4: { // 100 values mapped to 100 pixels: each pixel column has a single data value
                    pixel_height = 100;
                    pixel_width = 100;
                    break;
                }

                case 5: { // 100 values mapped to 100 pixels: each pixel column has a single data value
                    count = 101;
                    for (int i = 0; i < count; i++) {
                        values[i] = (float)i + 1;
                        coords[i] = (float)i + 1;
                    }
                    pixel_height = 100;
                    pixel_width = 100;
                    break;
                }

                case 6: { // 50 values mapped to 100 pixels: alternate pixels have no data!
                    count = 201;
                    for (int i = 0; i < count; i++) {
                        values[i] = (float)i + 1;
                        coords[i] = (float)i + 1;
                    }
                    start_x = 151.0;
                    pixel_height = 100;
                    pixel_width = 100;
                    break;
                }
                case 7: { // 50 values mapped to 100 pixels: alternate pixels have no data!
                    count = 201;
                    for (int i = 0; i < count; i++) {
                        values[i] = (float)i + 1;
                        coords[i] = (float)i + 1;
                    }
                    start_y = 151.0;
                    pixel_height = 100;
                    pixel_width = 100;
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

        float max_y = values[0];
        float min_y = values[0];
        float max_x = coords[count - 1];
        float min_x = values[0];

        // Reduce the ordered X data's width

        if (!start_x && !end_x) {
            reduceOrderedData(values.data(), &count, nullptr, nullptr, coords.data(), &min_x, &max_x);
        }
        if (!start_x && end_x) {
            reduceOrderedData(values.data(), &count, nullptr, &*end_x, coords.data(), &min_x, &max_x);
        }
        if (start_x && !end_x) {
            reduceOrderedData(values.data(), &count, &*start_x, nullptr, coords.data(), &min_x, &max_x);
        }
        if (start_x && end_x) {
            reduceOrderedData(values.data(), &count, &*start_x, &*end_x, coords.data(), &min_x, &max_x);
        }

        // Range of Y data

        if (!start_y) {
            min_y = FLT_MAX;
            for (int j = 0; j < count; j++) {
                if (values[j] < min_y) {
                    min_y = values[j];
                }
            }
        } else {
            min_y = *start_y;
        }

        if (!end_y) {
            max_y = -FLT_MAX;
            for (int j = 0; j < count; j++) {
                if (values[j] > max_y) {
                    max_y = values[j];
                }
            }
        } else {
            max_y = *end_y;
        }

        // Reduce unordered Y data: remove points outside the range

        if (start_y || end_y) {
            int new_count = 0;
            int* remove = (int*)malloc(count * sizeof(int));
            for (int j = 0; j < count; j++) {
                if (values[j] < min_y || values[j] > max_y) {
                    remove[j] = 1;
                } else {
                    remove[j] = 0;
                    new_count++;
                }
            }

            if (new_count < count) {
                int id = 0;

                std::vector<float> new_values(static_cast<size_t>(new_count));
                std::vector<float> new_coords(static_cast<size_t>(new_count));

                for (int j = 0; j < count; j++) {
                    if (!remove[j]) {
                        new_values[id] = values[j];
                        new_coords[id++] = coords[j];
                    }
                }

                count = new_count;

                values = new_values;
                coords = new_coords;
            }

            free(remove);
        }

        // Reduce data to fix the device coordinates (relative pixels)

        // Coordinate index range of each horizontal pixel column
        // Each pixel has data points

        std::vector<float> data;
        std::vector<float> err_hi;
        std::vector<float> err_lo;

        int pixelWidth2 = 0;
        std::vector<float> horizontal_pixel_values;

        if (pixel_width && pixel_height) {
            // Map to pixels if the device coordinate viewport is defined

            debug(plugin_interface,
                  "Viewport: Mapping data to device pixel coordinate range (width, height) = {}, {}\n", *pixel_width,
                  *pixel_height);

            int* column = nullptr;

            // Assign coordinates to pixel columns

            horizontal_pixel_values =
                getBins(coords.data(), count, *pixel_width, (double)min_x, (double)max_x, &column);

            // Frequency distribution of pixel hits along each vertical pixel column

            data.resize(*pixel_width);
            err_hi.resize(*pixel_width);
            err_lo.resize(*pixel_width);

            int* good = (int*)malloc(*pixel_width * sizeof(int));

            float* verticalPixelValues = nullptr; // Value at the pixel center
            float delta;

            getVerticalPixelValues(values.data(), count, *pixel_height, &min_y, &max_y, &verticalPixelValues, &delta);

            auto verticalPixelBoundaries = (float*)malloc((*pixel_height + 2) * sizeof(float));
            verticalPixelBoundaries[0] = min_y;

            for (int i = 1; i < *pixel_height + 2; i++) {
                // Lower boundary of pixel cell
                verticalPixelBoundaries[i] = verticalPixelBoundaries[i - 1] + delta;
            }

            // frequency distribution of values within each pixel column

            int* row = (int*)malloc(count * sizeof(int));
            int* fctot = (int*)malloc(*pixel_width * sizeof(int));
            int* frtot = (int*)malloc(*pixel_height * sizeof(int));
            int** freq = (int**)malloc(*pixel_width * sizeof(int*));

            for (int i = 0; i < *pixel_width; i++) {
                fctot[i] = 0;
                freq[i] = (int*)malloc(*pixel_height * sizeof(int));
                for (int j = 0; j < pixel_height; j++) {
                    freq[i][j] = 0;
                }
            }

            for (int i = 0; i < count; i++) {
                fctot[column[i]]++; // total counts
            }

            int colCount = 0;
            for (int i = 0; i < *pixel_width; i++) {
                colCount = colCount + fctot[i];
            }

            debug(plugin_interface, "Column Totals: {}\n", colCount);
            for (int i = 0; i < *pixel_width; i++) {
                debug(plugin_interface, "[{}] {}\n", i, fctot[i]);
            }

            // Which pixel row bin do each un-ordered data point fall into?

            for (int j = 0; j < count; j++) {
                row[j] = -1;
                for (int i = 0; i < *pixel_height + 1; i++) { // Search within pixel boundaries
                    if (values[j] >= verticalPixelBoundaries[i] && values[j] < verticalPixelBoundaries[i + 1]) {
                        row[j] = i;
                        break;
                    }
                }
                if (row[j] > *pixel_height - 1) {
                    row[j] = *pixel_height - 1;
                }

                if (column[j] >= 0 && row[j] >= 0) {
                    freq[column[j]][row[j]]++; // build frequency distribution
                }
            }

            for (int i = 0; i < *pixel_height; i++) {
                frtot[i] = 0;
            }

            for (int i = 0; i < count; i++) {
                frtot[row[i]]++; // total counts
            }

            int rowCount = 0;
            for (int i = 0; i < *pixel_height; i++) {
                rowCount = rowCount + frtot[i];
            }

            debug(plugin_interface, "Row Totals: {}\n", rowCount);
            for (int i = 0; i < *pixel_height; i++) {
                debug(plugin_interface, "[{}] {}\n", i, frtot[i]);
            }

            free(column);
            free(row);
            free(fctot);
            free(frtot);

            // Build return values

            int good_count = 0;
            pixelWidth2 = *pixel_width;
            int* integral = (int*)malloc(*pixel_height * sizeof(int));

            for (int i = 0; i < pixel_width; i++) {

                // which value to return?
                // mean, median or mode?
                // with/without errors - standard deviation, asymmetric?
                // 2D pixel map with colour for frequency - limited pallet? 1 byte == 256 colours

                good[i] = 0;
                data[i] = 0.0;
                err_lo[i] = 0.0;
                err_hi[i] = 0.0;
                int meanCount = 0;

                if (range && mean) {
                    if (i == 0) {
                        debug(plugin_interface, "Mean returned\n");
                    }
                    for (int j = 0; j < pixel_height; j++) {
                        if (freq[i][j] > 0) {
                            data[i] = data[i] + (float)freq[i][j] * verticalPixelValues[j];
                            meanCount = meanCount + freq[i][j];
                        }
                    }
                    if (meanCount > 0) {
                        data[i] = data[i] / (float)meanCount;
                        good[i] = 1;
                        good_count++;
                    }

                } else if (!range && mode) {
                    debug(plugin_interface, "Mode returned\n");
                    int fmax = 0;
                    int fmaxID = -1;
                    for (int j = 0; j < pixel_height; j++) {
                        if (freq[i][j] > fmax) { // First mode found if multi-modal
                            fmaxID = j;
                            fmax = freq[i][j];
                        }
                    }
                    if (fmaxID >= 0) {
                        data[i] = verticalPixelValues[fmaxID];
                        good[i] = 1;
                        good_count++;
                    }
                } else if (!range && median) {
                    if (i == 0) {
                        debug(plugin_interface, "Median returned\n");
                    }
                    integral[0] = freq[i][0];

                    for (int j = 1; j < *pixel_height; j++) {
                        integral[j] = integral[j - 1] + freq[i][j];
                    }

                    int target = integral[*pixel_height - 1] / 2;

                    for (int j = 0; j < *pixel_height; j++) {
                        if (integral[j] >= target) {
                            if (1 && (j > 0 && integral[j - 1] > 0 && integral[j] != target)) {
                                float dx = (float)(integral[j] - integral[j - 1]);
                                if (dx != 0.0) {
                                    float m = (verticalPixelValues[j] - verticalPixelValues[j - 1]) / dx;
                                    data[i] = m * (target - integral[j]) + verticalPixelValues[j]; // Linear Interpolate
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
                            good_count++;

                            break;
                        }
                    }
                }

                // Range of data values - Above and below the mean/mode/median value

                err_hi[i] = 0.0;
                err_lo[i] = 0.0;

                for (int j = 0; j < *pixel_height; j++) {
                    if (freq[i][j] > 0) {
                        err_lo[i] = verticalPixelValues[j]; // lowest value
                        break;
                    }
                }

                for (int j = *pixel_height - 1; j >= 0; j--) {
                    if (freq[i][j] > 0) {
                        err_hi[i] = verticalPixelValues[j]; // highest value
                        break;
                    }
                }

                if (range) {
                    if (i == 0) {
                        debug(plugin_interface, "Range returned\n");
                    }
                    data[i] = 0.5 * (err_lo[i] + err_hi[i]);
                    good_count++;
                }

                free(freq[i]);

                debug(plugin_interface, "[{}]   {}   {}   {}   {}\n", i, data[i], err_lo[i], err_hi[i],
                      horizontal_pixel_values[i]);

            } // end of loop over pixel_width

            debug(plugin_interface, "good_count  = {}\n", good_count);
            debug(plugin_interface, "pixel_width = {}\n", *pixel_width);
            for (int i = 0; i < pixelWidth2; i++) {
                debug(plugin_interface, "[{}]   {}   {}   {}   {}\n", i, data[i], err_lo[i], err_hi[i],
                      horizontal_pixel_values[i]);
            }
            // Free allocated heap

            free(freq);
            free(verticalPixelValues);
            free(verticalPixelBoundaries);
            free(integral);

            // data, err_hi, err_lo, horizontal_pixel_values heap freed once the server transmits the data
            // heap within the idam layer is freed on reset or clearCache or if the cache fills

            // Remove pixel columns without data

            if (good_count < pixel_width) {
                debug(plugin_interface, "Removing pixel columns without data [{}, {}]\n", good_count, *pixel_width);

                std::vector<float> new_data(static_cast<size_t>(good_count));
                std::vector<float> new_err_hi(static_cast<size_t>(good_count));
                std::vector<float> new_err_lo(static_cast<size_t>(good_count));
                std::vector<float> pixel_values(static_cast<size_t>(good_count));

                int ID = 0;
                for (int i = 0; i < pixel_width; i++) {
                    if (good[i]) {
                        new_data[ID] = data[i];
                        new_err_hi[ID] = err_lo[i];
                        new_err_lo[ID] = err_hi[i];
                        pixel_values[ID] = horizontal_pixel_values[i];
                        ID++;
                    }
                }
                free(good);

                data = new_data;
                err_lo = new_err_lo;
                err_hi = new_err_hi;
                horizontal_pixel_values = pixel_values;
                pixelWidth2 = good_count;
            }
        } else { // Device pixel coordinate range not set
            data = values;
            horizontal_pixel_values = coords;
            pixelWidth2 = count;
        }

        // Return viewport data

        size_t shape[] = {(size_t)pixelWidth2};
        udaPluginReturnDataFloatArray(plugin_interface, data.data(), 1, shape, udaGetDataDesc(handle));
        udaPluginReturnDataLabel(plugin_interface, udaGetDataLabel(handle));
        udaPluginReturnDataUnits(plugin_interface, udaGetDataUnits(handle));

        udaPluginReturnDimensionFloatArray(plugin_interface, 0, horizontal_pixel_values.data(), pixelWidth2,
                                           udaGetDimLabel(handle, 0), udaGetDimUnits(handle, 0));

        if (!range) {
            udaPluginReturnErrorAsymmetry(plugin_interface, true);
            udaPluginReturnErrorLow(plugin_interface, err_lo.data(), err_lo.size());
        }
        udaPluginReturnErrorHigh(plugin_interface, err_hi.data(), err_hi.size());
        udaPluginReturnDataOrder(plugin_interface, order);
    } else {
        error(plugin_interface, "A viewport for rank > 1 data has not been implemented!");
    }

    return 0;
}

std::vector<float> getBins(float* coords, int count, int pixel_width, float minValue, float maxValue, int** column)
{
    int start;
    int* bin = (int*)malloc(count * sizeof(int));

    std::vector<float> pix_values(static_cast<size_t>(pixel_width + 2));

    // Value width of each pixel

    float delta = (maxValue - minValue) / (float)pixel_width;

    // lower pixel boundary value (pixel_width + 1 boundaries + extra point)

    pix_values[0] = minValue;
    for (int i = 1; i < pixel_width + 2; i++) {
        pix_values[i] = pix_values[i - 1] + delta;
    }

    // Which pixel column bin do each data point fall into?

    start = 0;
    for (int j = 0; j < count; j++) {
        bin[j] = -1;
        for (int i = start; i < pixel_width + 1; i++) { // Search within pixel boundaries
            if (coords[j] >= pix_values[i] && coords[j] < pix_values[i + 1]) {
                bin[j] = i;
                start = i;
                break;
            }
        }
        if (bin[j] > pixel_width - 1) {
            bin[j] = pixel_width - 1;
        }
    }

    // mid pixel value: the range of data is mapped to these pixels

    pix_values[0] = minValue + 0.5 * delta;
    for (int i = 1; i < pixel_width; i++) {
        pix_values[i] = pix_values[i - 1] + delta;
    }

    *column = bin;
    return pix_values;
}

// Mid-Pixel value

void getVerticalPixelValues(float* values, int count, int pixel_height, float* startValue, float* endValue,
                            float** verticalPixelValues, float* delta)
{
    auto v = (float*)malloc(pixel_height * sizeof(float));
    float maxValue = values[0], minValue = values[0];

    if (startValue == nullptr) {
        for (int i = 0; i < count; i++) {
            if (values[i] < minValue) {
                minValue = values[i];
            }
        }
    } else {
        minValue = *startValue;
    }

    if (endValue == nullptr) {
        for (int i = 0; i < count; i++) {
            if (values[i] > maxValue) {
                maxValue = values[i];
            }
        }
    } else {
        maxValue = *endValue;
    }

    *delta = (maxValue - minValue) / (float)pixel_height;
    v[0] = minValue + 0.5 * (*delta);
    for (int i = 1; i < pixel_height; i++) {
        v[i] = v[i - 1] + *delta;
    }
    *verticalPixelValues = v;
}

// Assume data are arranged in no particular value order

void getBinIds(float* values, int count, int pixel_height, float* pixel_values, int** freq)
{
    auto f = (int*)malloc(pixel_height * sizeof(int));
    for (int i = 0; i < pixel_height; i++) {
        f[i] = 0;
    }
    for (int i = 0; i < count; i++) {
        for (int j = 0; j < pixel_height; j++) {
            if (values[i] >= pixel_values[j] && values[i] <= pixel_values[j]) {
                f[j]++;
            }
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
