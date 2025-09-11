#include "help_plugin.h"

#include <cstdlib>

#ifdef __GNUC__

#  include <strings.h>

#else
#  include <winsock2.h>
#endif

#include <clientserver/initStructs.h>
#include <structures/struct.h>
#include <structures/accessors.h>
#include <clientserver/errorLog.h>
#include <logging/logging.h>
#include <plugins/udaPlugin.h>
#include <clientserver/stringUtils.h>
#include <authentication/oauth_authentication.h>
#include <version.h>
#include <fmt/format.h>

static int do_ping(IDAM_PLUGIN_INTERFACE* idam_plugin_interface);

static int do_services(IDAM_PLUGIN_INTERFACE* idam_plugin_interface);

int helpPlugin(IDAM_PLUGIN_INTERFACE* idam_plugin_interface)
{
    int err;
    static short init = 0;

    //----------------------------------------------------------------------------------------
    // Standard v1 Plugin Interface

    DATA_BLOCK* data_block;
    REQUEST_DATA* request;

    if (idam_plugin_interface->interfaceVersion > THISPLUGIN_MAX_INTERFACE_VERSION) {
        RAISE_PLUGIN_ERROR("Plugin Interface Version Unknown to this plugin: Unable to execute the request!");
    }

    idam_plugin_interface->pluginVersion = THISPLUGIN_VERSION;

    data_block = idam_plugin_interface->data_block;
    request = idam_plugin_interface->request_data;

    unsigned short housekeeping = idam_plugin_interface->housekeeping;

    //----------------------------------------------------------------------------------------
    // Heap Housekeeping

    // Plugin must maintain a list of open file handles and sockets: loop over and close all files and sockets
    // Plugin must maintain a list of plugin functions called: loop over and reset state and free heap.
    // Plugin must maintain a list of calls to other plugins: loop over and call each plugin with the housekeeping request
    // Plugin must destroy lists at end of housekeeping

    if (housekeeping || STR_IEQUALS(request->function, "reset")) {

        if (!init) { return 0; }        // Not previously initialised: Nothing to do!
        init = 0;
        return 0;
    }

    //----------------------------------------------------------------------------------------
    // Initialise

    if (!init || STR_IEQUALS(request->function, "init")
        || STR_IEQUALS(request->function, "initialise")) {

        init = 1;
        if (STR_IEQUALS(request->function, "init") || STR_IEQUALS(request->function, "initialise")) {
            return 0;
        }
    }

    if (STR_IEQUALS(request->function, "help") || request->function[0] == '\0') {
        if (const char* url = getenv("UDA_AUTHORISATION_URL"); url != nullptr) {
            const auto* email = authPayloadValue("", idam_plugin_interface); // get email
            bool authorised = false;
            if (email != nullptr) {
                const std::string auth_url = std::string{url} + "/" + email;
                try {
                    const uda::authentication::CurlWrapper curl_wrapper;
                    if (const auto response = curl_wrapper.perform_get_request(auth_url); response == "True") {
                        authorised = true;
                    }
                } catch (...) {
                    authorised = false;
                }
            }
            return setReturnDataString(data_block, authorised ? "authorised" : "unauthorised",
                "Help help = description of this plugin");
        }

        const char* help = "\nHelp\tList of HELP plugin functions:\n\n"
                           "services()\tReturns a list of available services with descriptions\n"
                           "ping()\t\tReturn the Local Server Time in seconds and microseonds\n"
                           "servertime()\tReturn the Local Server Time in seconds and microseonds\n\n";
        return setReturnDataString(data_block, help, "Help help = description of this plugin");
    } else if (STR_IEQUALS(request->function, "version")) {
        return setReturnDataString(data_block, UDA_BUILD_VERSION, "Plugin version number");
    } else if (STR_IEQUALS(request->function, "builddate")) {
        return setReturnDataString(data_block, __DATE__, "Plugin build date");
    } else if (STR_IEQUALS(request->function, "defaultmethod")) {
        return setReturnDataString(data_block, THISPLUGIN_DEFAULT_METHOD, "Plugin default method");
    } else if (STR_IEQUALS(request->function, "maxinterfaceversion")) {
        return setReturnDataIntScalar(data_block, THISPLUGIN_MAX_INTERFACE_VERSION, "Maximum Interface Version");
    } else if (STR_IEQUALS(request->function, "ping") || STR_IEQUALS(request->function, "servertime")) {
        return do_ping(idam_plugin_interface);
    } else if (STR_IEQUALS(request->function, "services")) {
        return do_services(idam_plugin_interface);
    } else {
        RAISE_PLUGIN_ERROR("Unknown function requested!");
    }

    return err;
}

static int do_ping(IDAM_PLUGIN_INTERFACE* idam_plugin_interface)
{
    //----------------------------------------------------------------------------------------

    // Ping: Timing
    struct timeval serverTime;        // Local time in microseconds
    gettimeofday(&serverTime, nullptr);

    // define the returned data structure

    struct HELP_PING
    {
        unsigned int seconds;    // Server time in seconds
        unsigned int microseconds;    // Server time in microseconds
    };
    typedef struct HELP_PING HELP_PING;

    USERDEFINEDTYPE usertype;
    COMPOUNDFIELD field;

    initUserDefinedType(&usertype);            // New structure definition
    initCompoundField(&field);

    strcpy(usertype.name, "HELP_PING");
    strcpy(usertype.source, "idamServerHelp");
    usertype.ref_id = 0;
    usertype.imagecount = 0;                // No Structure Image data
    usertype.image = nullptr;
    usertype.size = sizeof(HELP_PING);        // Structure size
    usertype.idamclass = UDA_TYPE_COMPOUND;

    int offset = 0;
    defineField(&field, "seconds", "Server time in seconds from the epoch start", &offset, SCALARUINT);
    addCompoundField(&usertype, field);
    defineField(&field, "microseconds", "Server inter-second time in microseconds", &offset, SCALARUINT);
    addCompoundField(&usertype, field);

    USERDEFINEDTYPELIST* userdefinedtypelist = idam_plugin_interface->userdefinedtypelist;
    addUserDefinedType(userdefinedtypelist, usertype);

    // assign the returned data structure

    auto data = (HELP_PING*)malloc(sizeof(HELP_PING));
    addMalloc(idam_plugin_interface->logmalloclist, (void*)data, 1, sizeof(HELP_PING), "HELP_PING");        // Register

    data->seconds = (unsigned int)serverTime.tv_sec;
    data->microseconds = (unsigned int)serverTime.tv_usec;

    // return to the client

    DATA_BLOCK* data_block = idam_plugin_interface->data_block;
    initDataBlock(data_block);

    data_block->data_type = UDA_TYPE_COMPOUND;
    data_block->rank = 0;
    data_block->data_n = 1;
    data_block->data = (char*)data;

    strcpy(data_block->data_desc, "Local UDA server time");
    strcpy(data_block->data_label, "servertime");
    strcpy(data_block->data_units, "");

    data_block->opaque_type = UDA_OPAQUE_TYPE_STRUCTURES;
    data_block->opaque_count = 1;
    data_block->opaque_block = (void*)findUserDefinedType(userdefinedtypelist, "HELP_PING", 0);

    return 0;
}

static int do_services(IDAM_PLUGIN_INTERFACE* idam_plugin_interface)
{
    //======================================================================================
    // Plugin functionality
    int count;
    unsigned short target;

    const char* line = "\n------------------------------------------------------\n";

    // Document is a single block of chars

    std::string doc;

    // Total Number of registered plugins available

    const ENVIRONMENT* environment = idam_plugin_interface->environment;

    const PLUGINLIST* pluginList = idam_plugin_interface->pluginList;

    count = 0;
    for (int i = 0; i < pluginList->count; i++) {
        if (pluginList->plugin[i].status == UDA_PLUGIN_OPERATIONAL &&
            (pluginList->plugin[i].is_private == UDA_PLUGIN_PUBLIC ||
             (pluginList->plugin[i].is_private == UDA_PLUGIN_PRIVATE && !environment->external_user))) {
            count++;
        }
    }

    doc = fmt::format("\nTotal number of registered plugins available: {}\n", count);

    for (int j = 0; j < 5; j++) {
        count = 0;
        doc += line;
        switch (j) {
            case 0: {
                target = UDA_PLUGIN_CLASS_FILE;

                for (int i = 0; i < pluginList->count; i++) {
                    if (pluginList->plugin[i].plugin_class == target &&
                        pluginList->plugin[i].status == UDA_PLUGIN_OPERATIONAL &&
                        (pluginList->plugin[i].is_private == UDA_PLUGIN_PUBLIC ||
                         (pluginList->plugin[i].is_private == UDA_PLUGIN_PRIVATE && !environment->external_user)) &&
                        pluginList->plugin[i].format[0] != '\0' && pluginList->plugin[i].extension[0] != '\0') {
                        count++;
                    }
                }

                doc += fmt::format("\nNumber of plugins for data file formats: {}\n\n", count);

                for (int i = 0; i < pluginList->count; i++) {
                    if (pluginList->plugin[i].plugin_class == target &&
                        pluginList->plugin[i].status == UDA_PLUGIN_OPERATIONAL &&
                        (pluginList->plugin[i].is_private == UDA_PLUGIN_PUBLIC ||
                         (pluginList->plugin[i].is_private == UDA_PLUGIN_PRIVATE && !environment->external_user)) &&
                        pluginList->plugin[i].format[0] != '\0' && pluginList->plugin[i].extension[0] != '\0') {
                        doc += fmt::format("File format:\t\t{}\n", pluginList->plugin[i].format);
                        doc += fmt::format("Filename extension:\t{}\n", pluginList->plugin[i].extension);
                        doc += fmt::format("Description:\t\t{}\n", pluginList->plugin[i].desc);
                        doc += fmt::format("Example API call:\t{}\n\n", pluginList->plugin[i].example);
                    }
                }
                break;
            }
            case 1: {
                target = UDA_PLUGIN_CLASS_FUNCTION;
                for (int i = 0; i < pluginList->count; i++) {
                    if (pluginList->plugin[i].plugin_class == target &&
                        pluginList->plugin[i].status == UDA_PLUGIN_OPERATIONAL &&
                        (pluginList->plugin[i].is_private == UDA_PLUGIN_PUBLIC ||
                         (pluginList->plugin[i].is_private == UDA_PLUGIN_PRIVATE && !environment->external_user))) {
                        count++;
                    }
                }
                doc += fmt::format("\nNumber of plugins for Libraries: {}\n\n", count);
                for (int i = 0; i < pluginList->count; i++) {
                    if (pluginList->plugin[i].plugin_class == target &&
                        pluginList->plugin[i].status == UDA_PLUGIN_OPERATIONAL &&
                        (pluginList->plugin[i].is_private == UDA_PLUGIN_PUBLIC ||
                         (pluginList->plugin[i].is_private == UDA_PLUGIN_PRIVATE && !environment->external_user))) {
                        doc += fmt::format("Library name:\t\t{}\n", pluginList->plugin[i].format);
                        doc += fmt::format("Description:\t\t{}\n", pluginList->plugin[i].desc);
                        doc += fmt::format("Example API call:\t{}\n\n", pluginList->plugin[i].example);
                    }
                }
                break;
            }
            case 2: {
                target = UDA_PLUGIN_CLASS_SERVER;
                for (int i = 0; i < pluginList->count; i++) {
                    if (pluginList->plugin[i].plugin_class == target &&
                        pluginList->plugin[i].status == UDA_PLUGIN_OPERATIONAL &&
                        (pluginList->plugin[i].is_private == UDA_PLUGIN_PUBLIC ||
                         (pluginList->plugin[i].is_private == UDA_PLUGIN_PRIVATE && !environment->external_user))) {
                        count++;
                    }
                }
                doc += fmt::format("\nNumber of plugins for Data Servers: {}\n\n", count);
                for (int i = 0; i < pluginList->count; i++) {
                    if (pluginList->plugin[i].plugin_class == target &&
                        pluginList->plugin[i].status == UDA_PLUGIN_OPERATIONAL &&
                        (pluginList->plugin[i].is_private == UDA_PLUGIN_PUBLIC ||
                         (pluginList->plugin[i].is_private == UDA_PLUGIN_PRIVATE && !environment->external_user))) {
                        doc += fmt::format("Server name:\t\t{}\n", pluginList->plugin[i].format);
                        doc += fmt::format("Description:\t\t{}\n", pluginList->plugin[i].desc);
                        doc += fmt::format("Example API call:\t{}\n\n", pluginList->plugin[i].example);
                    }
                }
                break;
            }
            case 3: {
                target = UDA_PLUGIN_CLASS_DEVICE;
                for (int i = 0; i < pluginList->count; i++) {
                    if (pluginList->plugin[i].plugin_class == target &&
                        pluginList->plugin[i].status == UDA_PLUGIN_OPERATIONAL &&
                        (pluginList->plugin[i].is_private == UDA_PLUGIN_PUBLIC ||
                         (pluginList->plugin[i].is_private == UDA_PLUGIN_PRIVATE && !environment->external_user))) {
                        count++;
                    }
                }
                doc += fmt::format("\nNumber of plugins for External Devices: {}\n\n", count);
                for (int i = 0; i < pluginList->count; i++) {
                    if (pluginList->plugin[i].plugin_class == target &&
                        pluginList->plugin[i].status == UDA_PLUGIN_OPERATIONAL &&
                        (pluginList->plugin[i].is_private == UDA_PLUGIN_PUBLIC ||
                         (pluginList->plugin[i].is_private == UDA_PLUGIN_PRIVATE && !environment->external_user))) {
                        doc += fmt::format("External device name:\t{}\n", pluginList->plugin[i].format);
                        doc += fmt::format("Description:\t\t{}\n", pluginList->plugin[i].desc);
                        doc += fmt::format("Example API call:\t{}\n\n", pluginList->plugin[i].example);
                    }
                }
                break;
            }
            case 4: {
                target = UDA_PLUGIN_CLASS_OTHER;
                for (int i = 0; i < pluginList->count; i++) {
                    if (pluginList->plugin[i].plugin_class == target &&
                        pluginList->plugin[i].status == UDA_PLUGIN_OPERATIONAL &&
                        (pluginList->plugin[i].is_private == UDA_PLUGIN_PUBLIC ||
                         (pluginList->plugin[i].is_private == UDA_PLUGIN_PRIVATE && !environment->external_user))) {
                        count++;
                    }
                }
                doc += fmt::format("\nNumber of plugins for Other data services: {}\n\n", count);
                for (int i = 0; i < pluginList->count; i++) {
                    if (pluginList->plugin[i].plugin_class == target &&
                        pluginList->plugin[i].status == UDA_PLUGIN_OPERATIONAL &&
                        (pluginList->plugin[i].is_private == UDA_PLUGIN_PUBLIC ||
                         (pluginList->plugin[i].is_private == UDA_PLUGIN_PRIVATE && !environment->external_user))) {
                        doc += fmt::format("Data service name:\t{}\n", pluginList->plugin[i].format);
                        doc += fmt::format("Description:\t\t{}\n", pluginList->plugin[i].desc);
                        doc += fmt::format("Example API call:\t{}\n\n", pluginList->plugin[i].example);
                    }
                }
                break;
            }
        }
    }

    doc += "\n\n";

    return setReturnDataString(idam_plugin_interface->data_block, doc.c_str(), "Description of UDA data access services");
}
