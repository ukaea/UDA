#include "ipx_plugin.h"

#include <exception>

#include <plugins/udaPlugin.h>
#include <clientserver/stringUtils.h>
#include <clientserver/initStructs.h>
#include <structures/struct.h>
#include <structures/accessors.h>
#include <algorithm>

int ipxPlugin(IDAM_PLUGIN_INTERFACE *idam_plugin_interface)
{
    try {
        static uda::plugins::ipx::IPXPlugin plugin;

        if (idam_plugin_interface->interfaceVersion > THISPLUGIN_MAX_INTERFACE_VERSION) {
            RAISE_PLUGIN_ERROR("Plugin Interface Version Unknown to this plugin: Unable to execute the request!");
        }

        idam_plugin_interface->pluginVersion = THISPLUGIN_VERSION;

        REQUEST_BLOCK* request_block = idam_plugin_interface->request_block;

        if (idam_plugin_interface->housekeeping || STR_IEQUALS(request_block->function, "reset")) {
            plugin.reset();
            return 0;
        }

        plugin.init();

        if (STR_IEQUALS(request_block->function, "init") || STR_IEQUALS(request_block->function, "initialise")) {
            return 0;
        }

        if (STR_IEQUALS(request_block->function, "help")) {
            return plugin.help(idam_plugin_interface);
        } else if (STR_IEQUALS(request_block->function, "version")) {
            return plugin.version(idam_plugin_interface);
        } else if (STR_IEQUALS(request_block->function, "builddate")) {
            return plugin.build_date(idam_plugin_interface);
        } else if (STR_IEQUALS(request_block->function, "defaultmethod")) {
            return plugin.default_method(idam_plugin_interface);
        } else if (STR_IEQUALS(request_block->function, "maxinterfaceversion")) {
            return plugin.max_interface_version(idam_plugin_interface);
        } else if (STR_IEQUALS(request_block->function, "read")) {
            return plugin.read(idam_plugin_interface);
        } else {
            RAISE_PLUGIN_ERROR("Unknown function requested!");
        }
    } catch (std::exception& ex) {
        RAISE_PLUGIN_ERROR(ex.what());
    }
}

/**
 * Help: A Description of library functionality
 * @param idam_plugin_interface
 * @return
 */
int uda::plugins::ipx::IPXPlugin::help(IDAM_PLUGIN_INTERFACE* plugin_interface)
{
    const char* help = "\nIPX: IPX reaader\n\n";
    const char* desc = "IPX: help = description of this plugin";

    return setReturnDataString(plugin_interface->data_block, help, desc);
}

/**
 * Plugin version
 * @param idam_plugin_interface
 * @return
 */
int uda::plugins::ipx::IPXPlugin::version(IDAM_PLUGIN_INTERFACE* plugin_interface)
{
    return setReturnDataIntScalar(plugin_interface->data_block, THISPLUGIN_VERSION, "Plugin version number");
}

/**
 * Plugin Build Date
 * @param idam_plugin_interface
 * @return
 */
int uda::plugins::ipx::IPXPlugin::build_date(IDAM_PLUGIN_INTERFACE* plugin_interface)
{
    return setReturnDataString(plugin_interface->data_block, __DATE__, "Plugin build date");
}

/**
 * Plugin Default Method
 * @param idam_plugin_interface
 * @return
 */
int uda::plugins::ipx::IPXPlugin::default_method(IDAM_PLUGIN_INTERFACE* plugin_interface)
{
    return setReturnDataString(plugin_interface->data_block, THISPLUGIN_DEFAULT_METHOD, "Plugin default method");
}

/**
 * Plugin Maximum Interface Version
 * @param idam_plugin_interface
 * @return
 */
int uda::plugins::ipx::IPXPlugin::max_interface_version(IDAM_PLUGIN_INTERFACE* plugin_interface)
{
    return setReturnDataIntScalar(plugin_interface->data_block, THISPLUGIN_MAX_INTERFACE_VERSION, "Maximum Interface Version");
}

struct BGR {
    unsigned char b;
    unsigned char g;
    unsigned char r;
//    unsigned char _; // padding byte
};

struct Frame {
    ~Frame() = delete;

    int number;
    double time;
    void* r;
    void* g;
    void* b;
    void* k;
};

struct Video {
    static constexpr int string_length = 256;

    ~Video() = delete;

    char datetime[string_length];
    int shot;
    char lens[string_length];
    char filter[string_length];
    char view[string_length];
    char camera[string_length];
    int is_color;
    int width;
    int height;
    int depth;
    int taps;
    int left;
    int top;
    int hbin;
    int vbin;
    double offset[2];
    double gain[2];
    double preexp;
    double exposure;
    double board_temp;
    double ccd_temp;
    int n_frames;

    Frame* frames;
};

namespace {

struct PluginArgs {
    bool is_frame;
    int frame;
    bool is_range;
    int first;
    int last;
    int stride;
};

void read_frame(LOGMALLOCLIST* logmalloclist, Cipx& ipx, Frame& frame, int idx, int width, int height)
{
    frame.number = idx;
    frame.time = ipx.frameTime(idx);

    size_t sz = (size_t)width * (size_t)height;
    int byte_depth = ipx.depth();

    if (byte_depth <= 8) {
        frame.r = calloc(sz, sizeof(unsigned char));
        frame.g = calloc(sz, sizeof(unsigned char));
        frame.b = calloc(sz, sizeof(unsigned char));
        frame.k = calloc(sz, sizeof(unsigned char));

        addMalloc(logmalloclist, frame.r, (int)sz, sizeof(unsigned char), "unsigned char *");
        addMalloc(logmalloclist, frame.g, (int)sz, sizeof(unsigned char), "unsigned char *");
        addMalloc(logmalloclist, frame.b, (int)sz, sizeof(unsigned char), "unsigned char *");
        addMalloc(logmalloclist, frame.k, (int)sz, sizeof(unsigned char), "unsigned char *");
    } else if (byte_depth <= 16) {
        frame.r = calloc(sz, sizeof(unsigned short));
        frame.g = calloc(sz, sizeof(unsigned short));
        frame.b = calloc(sz, sizeof(unsigned short));
        frame.k = calloc(sz, sizeof(unsigned short));

        addMalloc(logmalloclist, frame.r, (int)sz, sizeof(unsigned short), "unsigned short *");
        addMalloc(logmalloclist, frame.g, (int)sz, sizeof(unsigned short), "unsigned short *");
        addMalloc(logmalloclist, frame.b, (int)sz, sizeof(unsigned short), "unsigned short *");
        addMalloc(logmalloclist, frame.k, (int)sz, sizeof(unsigned short), "unsigned short *");
    } else if (byte_depth <= 32) {
        frame.r = calloc(sz, sizeof(unsigned int));
        frame.g = calloc(sz, sizeof(unsigned int));
        frame.b = calloc(sz, sizeof(unsigned int));
        frame.k = calloc(sz, sizeof(unsigned int));

        addMalloc(logmalloclist, frame.r, (int)sz, sizeof(unsigned int), "unsigned int *");
        addMalloc(logmalloclist, frame.g, (int)sz, sizeof(unsigned int), "unsigned int *");
        addMalloc(logmalloclist, frame.b, (int)sz, sizeof(unsigned int), "unsigned int *");
        addMalloc(logmalloclist, frame.k, (int)sz, sizeof(unsigned int), "unsigned int *");
    }

    if (ipx.color()) {
        unsigned char* data;
        ipx.getBMPbits(idx, 0, 0, &data); // do not free data, it is freed by ~Cipx()

        auto bgr_data = (BGR*)data;
        auto r = (unsigned char*)frame.r;
        auto g = (unsigned char*)frame.g;
        auto b = (unsigned char*)frame.b;
        for (size_t i = 0; i < sz; ++i) {
            r[i] = bgr_data[i].r;
            g[i] = bgr_data[i].g;
            b[i] = bgr_data[i].b;
        }
    }

    unsigned char* data;
    ipx.getPixelPointer(idx, &data); // do not free data, it is freed by ~Cipx()

    if (byte_depth <= 8) {
        auto k = (unsigned char*)frame.k;
        auto d = data;
        for (size_t i = 0; i < sz; ++i) {
            k[i] = d[i];
        }
    } else if (byte_depth <= 16) {
        auto k = (unsigned short*)frame.k;
        auto d = (unsigned short*)data;
        for (size_t i = 0; i < sz; ++i) {
            k[i] = d[i];
        }
    } else if (byte_depth <= 32) {
        auto k = (unsigned int*)frame.k;
        auto d = (unsigned int*)data;
        for (size_t i = 0; i < sz; ++i) {
            k[i] = d[i];
        }
    }
}

Video* read_video(LOGMALLOCLIST* logmalloclist, Cipx& ipx, const PluginArgs& args)
{
    auto video = (Video*)malloc(1 * sizeof(Video));    // Structured Data Must be a heap variable
    addMalloc(logmalloclist, (void*)video, 1, sizeof(Video), "VIDEO");
    memset(video, '\0', sizeof(Video));

    strftime(video->datetime, Video::string_length, "%FT%TZ", ipx.shotTime());
    video->shot = ipx.shot();
    StringCopy(video->lens, ipx.lens(), Video::string_length);
    StringCopy(video->filter, ipx.filter(), Video::string_length);
    StringCopy(video->view, ipx.view(), Video::string_length);
    StringCopy(video->camera, ipx.camera(), Video::string_length);
    video->is_color = ipx.color();
    video->width = ipx.width();
    video->height = ipx.height();
    video->depth = ipx.depth();
    video->taps = ipx.taps();
    video->left = ipx.left();
    video->top = ipx.top();
    video->hbin = ipx.hbin();
    video->vbin = ipx.vbin();
    video->offset[0] = ipx.offset(1);
    video->offset[1] = ipx.offset(2);
    video->gain[0] = ipx.gain(1);
    video->gain[1] = ipx.gain(2);
    video->preexp = ipx.preexp();
    video->exposure = ipx.exposure();
    video->board_temp = ipx.boardtemp();
    video->ccd_temp = ipx.ccdtemp();

    if (args.is_frame) {
        video->n_frames = 1;
    } else if (args.is_range) {
        video->n_frames = (args.last - args.first) / args.stride;
    } else {
        video->n_frames = (unsigned int)ipx.frames();
    }
    video->frames = (Frame*)malloc(video->n_frames * sizeof(Frame));
    memset(video->frames, '\0', video->n_frames * sizeof(Frame));

    int rank = 1;

    addMalloc2(logmalloclist, (void*)video->frames, video->n_frames, sizeof(Frame), "FRAME", rank, nullptr);

    if (args.is_frame) {
        read_frame(logmalloclist, ipx, video->frames[0], args.frame, video->width, video->height);
    } else if (args.is_range) {
        int frame = args.first;
        for (int i = 0; i < video->n_frames; i++) {
            read_frame(logmalloclist, ipx, video->frames[i], frame, video->width, video->height);
            frame += args.stride;
        }
    } else {
        for (int i = 0; i < video->n_frames; i++) {
            read_frame(logmalloclist, ipx, video->frames[i], i, video->width, video->height);
        }
    }

    return video;
}

void setup_usertypes(USERDEFINEDTYPELIST* userdefinedtypelist, bool is_color, int byte_depth)
{
    USERDEFINEDTYPE frame_type;
    initUserDefinedType(&frame_type);                                 // New structure definition

    strcpy(frame_type.name, "FRAME");
    strcpy(frame_type.source, "IPX plugin FRAME");
    frame_type.ref_id = 0;
    frame_type.imagecount = 0;                                        // No Structure Image data
    frame_type.image = nullptr;
    frame_type.size = sizeof(Frame);                                // Structure size
    frame_type.idamclass = UDA_TYPE_COMPOUND;

    addStructureField(&frame_type, "number", "", UDA_TYPE_INT, false, 0, nullptr, offsetof(Frame, number));
    addStructureField(&frame_type, "time", "", UDA_TYPE_DOUBLE, false, 0, nullptr, offsetof(Frame, time));

    if (is_color) {
        if (byte_depth <= 8) {
            addStructureField(&frame_type, "r", "red channel", UDA_TYPE_UNSIGNED_CHAR, true, 0, nullptr, offsetof(Frame, r));
            addStructureField(&frame_type, "g", "green channel", UDA_TYPE_UNSIGNED_CHAR, true, 0, nullptr, offsetof(Frame, g));
            addStructureField(&frame_type, "b", "blue channel", UDA_TYPE_UNSIGNED_CHAR, true, 0, nullptr, offsetof(Frame, b));
        } else if (byte_depth <= 16) {
            addStructureField(&frame_type, "r", "red channel", UDA_TYPE_UNSIGNED_SHORT, true, 0, nullptr, offsetof(Frame, r));
            addStructureField(&frame_type, "g", "green channel", UDA_TYPE_UNSIGNED_SHORT, true, 0, nullptr, offsetof(Frame, g));
            addStructureField(&frame_type, "b", "blue channel", UDA_TYPE_UNSIGNED_SHORT, true, 0, nullptr, offsetof(Frame, b));
        } else if (byte_depth <= 32) {
            addStructureField(&frame_type, "r", "red channel", UDA_TYPE_UNSIGNED_INT, true, 0, nullptr, offsetof(Frame, r));
            addStructureField(&frame_type, "g", "green channel", UDA_TYPE_UNSIGNED_INT, true, 0, nullptr, offsetof(Frame, g));
            addStructureField(&frame_type, "b", "blue channel", UDA_TYPE_UNSIGNED_INT, true, 0, nullptr, offsetof(Frame, b));
        }
    }

    const char* name = is_color ? "raw" : "k";

    if (byte_depth <= 8) {
        addStructureField(&frame_type, name, "black channel", UDA_TYPE_UNSIGNED_CHAR, true, 0, nullptr, offsetof(Frame, k));
    } else if (byte_depth <= 16) {
        addStructureField(&frame_type, name, "black channel", UDA_TYPE_UNSIGNED_SHORT, true, 0, nullptr, offsetof(Frame, k));
    } else if (byte_depth <= 32) {
        addStructureField(&frame_type, name, "black channel", UDA_TYPE_UNSIGNED_INT, true, 0, nullptr, offsetof(Frame, k));
    }

    addUserDefinedType(userdefinedtypelist, frame_type);

    USERDEFINEDTYPE video_type;
    initUserDefinedType(&video_type);                                 // New structure definition

    strcpy(video_type.name, "VIDEO");
    strcpy(video_type.source, "IPX plugin VIDEO");
    video_type.ref_id = 0;
    video_type.imagecount = 0;                                        // No Structure Image data
    video_type.image = nullptr;
    video_type.size = sizeof(Video);                                 // Structure size
    video_type.idamclass = UDA_TYPE_COMPOUND;

    int shape[] = { Video::string_length };
    addStructureField(&video_type, "datetime", "", UDA_TYPE_STRING, false, 1, shape, offsetof(Video, datetime));
    addStructureField(&video_type, "shot", "", UDA_TYPE_INT, false, 0, nullptr, offsetof(Video, shot));
    addStructureField(&video_type, "lens", "", UDA_TYPE_STRING, false, 1, shape, offsetof(Video, lens));
    addStructureField(&video_type, "filter", "", UDA_TYPE_STRING, false, 1, shape, offsetof(Video, filter));
    addStructureField(&video_type, "view", "", UDA_TYPE_STRING, false, 1, shape, offsetof(Video, view));
    addStructureField(&video_type, "camera", "", UDA_TYPE_STRING, false, 1, shape, offsetof(Video, camera));
    addStructureField(&video_type, "is_color", "", UDA_TYPE_INT, false, 0, nullptr, offsetof(Video, is_color));
    addStructureField(&video_type, "width", "", UDA_TYPE_INT, false, 0, nullptr, offsetof(Video, width));
    addStructureField(&video_type, "height", "", UDA_TYPE_INT, false, 0, nullptr, offsetof(Video, height));
    addStructureField(&video_type, "depth", "", UDA_TYPE_INT, false, 0, nullptr, offsetof(Video, depth));
    addStructureField(&video_type, "taps", "", UDA_TYPE_INT, false, 0, nullptr, offsetof(Video, taps));
    addStructureField(&video_type, "left", "", UDA_TYPE_INT, false, 0, nullptr, offsetof(Video, left));
    addStructureField(&video_type, "top", "", UDA_TYPE_INT, false, 0, nullptr, offsetof(Video, top));
    addStructureField(&video_type, "hbin", "", UDA_TYPE_INT, false, 0, nullptr, offsetof(Video, hbin));
    addStructureField(&video_type, "vbin", "", UDA_TYPE_INT, false, 0, nullptr, offsetof(Video, vbin));
    shape[0] = 2;
    addStructureField(&video_type, "offset", "", UDA_TYPE_DOUBLE, false, 1, shape, offsetof(Video, offset));
    addStructureField(&video_type, "gain", "", UDA_TYPE_DOUBLE, false, 1, shape, offsetof(Video, gain));
    addStructureField(&video_type, "preexp", "", UDA_TYPE_DOUBLE, false, 0, nullptr, offsetof(Video, preexp));
    addStructureField(&video_type, "exposure", "", UDA_TYPE_DOUBLE, false, 0, nullptr, offsetof(Video, exposure));
    addStructureField(&video_type, "board_temp", "", UDA_TYPE_DOUBLE, false, 0, nullptr, offsetof(Video, board_temp));
    addStructureField(&video_type, "ccd_temp", "", UDA_TYPE_DOUBLE, false, 0, nullptr, offsetof(Video, ccd_temp));
    addStructureField(&video_type, "n_frames", "", UDA_TYPE_INT, false, 0, nullptr, offsetof(Video, n_frames));

    COMPOUNDFIELD field;
    initCompoundField(&field);

    strcpy(field.name, "frames");
    field.atomictype = UDA_TYPE_UNKNOWN;
    strcpy(field.type, "FRAME");
    strcpy(field.desc, "structure FRAME");

    field.pointer = 1;
    field.count = 1;
    field.rank = 0;
    field.shape = nullptr;

    field.size = field.count * sizeof(Frame*);
    field.offset = (int)offsetof(Video, frames);
    field.offpad = (int)padding(offsetof(Video, frames), field.type);
    field.alignment = getalignmentof(field.type);

    addCompoundField(&video_type, field);                             // Single Structure element

    addUserDefinedType(userdefinedtypelist, video_type);
}

void set_return_video(USERDEFINEDTYPELIST* userdefinedtypelist, DATA_BLOCK* data_block, const Video* video)
{
    initDataBlock(data_block);

    data_block->data_type = UDA_TYPE_COMPOUND;
    data_block->rank = 0;
    data_block->data_n = 1;
    data_block->data = (char*)video;

    strcpy(data_block->data_desc, "Structure Data Test #33");
    strcpy(data_block->data_label, "Double Values [5, 20] : (1*, 10*)");
    strcpy(data_block->data_units, "");

    data_block->opaque_type = UDA_OPAQUE_TYPE_STRUCTURES;
    data_block->opaque_count = 1;
    data_block->opaque_block = (void*)findUserDefinedType(userdefinedtypelist, "VIDEO", 0);
}

} // anon namespace

int uda::plugins::ipx::IPXPlugin::read(IDAM_PLUGIN_INTERFACE* idam_plugin_interface)
{
    const char* filename;
    FIND_REQUIRED_STRING_VALUE(idam_plugin_interface->request_block->nameValueList, filename);

    int frame = 0;
    bool is_frame = FIND_INT_VALUE(idam_plugin_interface->request_block->nameValueList, frame);

    int first = 0;
    bool is_first = FIND_INT_VALUE(idam_plugin_interface->request_block->nameValueList, first);

    int last = 0;
    bool is_last = FIND_INT_VALUE(idam_plugin_interface->request_block->nameValueList, last);

    int stride = 0;
    bool is_stride = FIND_INT_VALUE(idam_plugin_interface->request_block->nameValueList, stride);

    if (is_frame && (is_first || is_last || is_stride)) {
        RAISE_PLUGIN_ERROR("cannot specify both frame and first|last|stride frames");
    }

    if (is_first != is_last) {
        RAISE_PLUGIN_ERROR("both first and last must be specified");
    }

    if ((is_first && is_last) && (last - first) <= 0) {
        RAISE_PLUGIN_ERROR("positive range must be specified");
    }

    Cipx ipx;
    if (!ipx.loadFile(filename)) {
        RAISE_PLUGIN_ERROR(ipx.lasterr());
    }

    setup_usertypes(idam_plugin_interface->userdefinedtypelist, bool(ipx.color()), ipx.depth());

    PluginArgs args{
        .is_frame=is_frame,
        .frame=frame,
        .is_range=is_first,
        .first=first,
        .last=last,
        .stride=(is_stride ? stride : 1)
    };

    Video* video = read_video(idam_plugin_interface->logmalloclist, ipx, args);

    set_return_video(idam_plugin_interface->userdefinedtypelist, idam_plugin_interface->data_block, video);

    return 0;
}