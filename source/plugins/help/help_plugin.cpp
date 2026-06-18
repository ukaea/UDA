#include "help_plugin.h"

#include <cstdlib>
#include <string>

#ifdef __GNUC__

#  include <strings.h>

#else
#  include <winsock2.h>
#endif

#ifdef OIDCAUTHENTICATION
#  include <curl/curl.h>
#endif

#include <clientserver/initStructs.h>
#include <structures/struct.h>
#include <structures/accessors.h>
#include <clientserver/errorLog.h>
#include <logging/logging.h>
#include <plugins/udaPlugin.h>
#include <clientserver/stringUtils.h>
#include <authentication/oauth_authentication.h>
#include <common/uda_env_options.hpp>
#include <version.h>
#include <fmt/format.h>

static int do_ping(IDAM_PLUGIN_INTERFACE* plugin_interface);

static int do_services(IDAM_PLUGIN_INTERFACE* plugin_interface);

static int do_server_metadata(IDAM_PLUGIN_INTERFACE* plugin_interface);

#ifdef OIDCAUTHENTICATION
static int do_authorisation_test(IDAM_PLUGIN_INTERFACE* plugin_interface);
#endif

int helpPlugin(IDAM_PLUGIN_INTERFACE* plugin_interface)
{
    static short init = 0;

    //----------------------------------------------------------------------------------------
    // Standard v1 Plugin Interface

    DATA_BLOCK* data_block;
    REQUEST_DATA* request;

    if (plugin_interface->interfaceVersion > THISPLUGIN_MAX_INTERFACE_VERSION) {
        RAISE_PLUGIN_ERROR("Plugin Interface Version Unknown to this plugin: Unable to execute the request!");
    }

    plugin_interface->pluginVersion = THISPLUGIN_VERSION;

    data_block = plugin_interface->data_block;
    request = plugin_interface->request_data;

    unsigned short housekeeping = plugin_interface->housekeeping;

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
    int err {0};
    if (STR_IEQUALS(request->function, "help")) {
        const char* help = "\nHelp\tList of HELP plugin functions:\n\n"
                           "services()\t\tReturns a list of available services with descriptions\n"
                           "ping()\t\t\tReturn the Local Server Time in seconds and microseconds\n"
                           "servertime()\t\tReturn the Local Server Time in seconds and microseconds\n"
                           "servermetadata()\tReturn server compilation flags and runtime configuration\n"
#ifdef OIDCAUTHENTICATION
                           "authorise()\t\tTest token claim against an external authorisation service\n"
                           "\t\t\t  Env: UDA_HELP_AUTHZ_URL, UDA_HELP_AUTHZ_CLAIM, UDA_HELP_AUTHZ_EXPECT\n"
#endif
                           "\n";
        err = setReturnDataString(data_block, help, "Help help = description of this plugin");
#ifdef OIDCAUTHENTICATION
    } else if (STR_IEQUALS(request->function, "authorise")
            || STR_IEQUALS(request->function, "authorize")) {
        err = do_authorisation_test(plugin_interface);
#endif
    } else if (STR_IEQUALS(request->function, "version")) {
        err = setReturnDataString(data_block, UDA_BUILD_VERSION, "Plugin version number");
    } else if (STR_IEQUALS(request->function, "builddate")) {
        err = setReturnDataString(data_block, __DATE__, "Plugin build date");
    } else if (STR_IEQUALS(request->function, "defaultmethod")) {
        err = setReturnDataString(data_block, THISPLUGIN_DEFAULT_METHOD, "Plugin default method");
    } else if (STR_IEQUALS(request->function, "maxinterfaceversion")) {
        err = setReturnDataIntScalar(data_block, THISPLUGIN_MAX_INTERFACE_VERSION, "Maximum Interface Version");
    } else if (STR_IEQUALS(request->function, "ping") || STR_IEQUALS(request->function, "servertime")) {
        err = do_ping(plugin_interface);
    } else if (STR_IEQUALS(request->function, "services")) {
        err = do_services(plugin_interface);
    } else if (STR_IEQUALS(request->function, "servermetadata")) {
        err = do_server_metadata(plugin_interface);
    } else {
        RAISE_PLUGIN_ERROR_AND_EXIT("Unknown function requested!", plugin_interface);
    }

    if (err != 0) {
        concatUdaError(&plugin_interface->error_stack);
    }
    return err;
}

static int do_ping(IDAM_PLUGIN_INTERFACE* plugin_interface)
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

    USERDEFINEDTYPELIST* userdefinedtypelist = plugin_interface->userdefinedtypelist;
    addUserDefinedType(userdefinedtypelist, usertype);

    // assign the returned data structure

    auto data = (HELP_PING*)malloc(sizeof(HELP_PING));
    addMalloc(plugin_interface->logmalloclist, (void*)data, 1, sizeof(HELP_PING), "HELP_PING");        // Register

    data->seconds = (unsigned int)serverTime.tv_sec;
    data->microseconds = (unsigned int)serverTime.tv_usec;

    // return to the client

    DATA_BLOCK* data_block = plugin_interface->data_block;
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

static int do_services(IDAM_PLUGIN_INTERFACE* plugin_interface)
{
    //======================================================================================
    // Plugin functionality
    int count;
    unsigned short target;

    const char* line = "\n------------------------------------------------------\n";

    // Document is a single block of chars

    std::string doc;

    // Total Number of registered plugins available

    const ENVIRONMENT* environment = plugin_interface->environment;

    const PLUGINLIST* pluginList = plugin_interface->pluginList;

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

    return setReturnDataString(plugin_interface->data_block, doc.c_str(), "Description of UDA data access services");
}

#ifdef OIDCAUTHENTICATION

// Test whether the token bearer is authorised by an external HTTP service.
//
// Configuration (all via environment variables):
//   UDA_HELP_AUTHZ_URL   — base URL of the authorisation service, required.
//   UDA_HELP_AUTHZ_CLAIM — JWT claim whose value is sent as the ?value= parameter
//                          (dot-path notation supported, e.g. "realm_access.roles[0]").
//                          Defaults to "preferred_username".
//   UDA_HELP_AUTHZ_EXPECT — expected response body string for an authorised result.
//                           Defaults to "True".
//
// The service is called as:
//   GET {UDA_HELP_AUTHZ_URL}?claim={claim_name}&value={url-encoded claim value}
//
// Returns "authorised" or "unauthorised" as a string data block.
static int do_authorisation_test(IDAM_PLUGIN_INTERFACE* plugin_interface)
{
    DATA_BLOCK* data_block = plugin_interface->data_block;

    const char* base_url = getenv("UDA_HELP_AUTHZ_URL");
    if (!base_url || !base_url[0]) {
        return setReturnDataString(data_block, "unauthorised: UDA_HELP_AUTHZ_URL not set",
                                   "HELP authorisation check");
    }

    const char* claim_name = getenv("UDA_HELP_AUTHZ_CLAIM");
    if (!claim_name || !claim_name[0]) {
        claim_name = "preferred_username";
    }

    const char* expected = getenv("UDA_HELP_AUTHZ_EXPECT");
    if (!expected || !expected[0]) {
        expected = "True";
    }

    // authPayloadPath supports dot-path notation (e.g. "realm_access.roles[0]")
    // and falls back to a flat key lookup for simple claim names.
    const char* claim_value = authPayloadPath(claim_name, plugin_interface);
    if (!claim_value || !claim_value[0]) {
        return setReturnDataString(data_block, "unauthorised: claim not found in token",
                                   "HELP authorisation check");
    }

    // URL-encode the claim name and value so they are safe as query parameters.
    // curl_easy_escape requires a handle but does not perform any network operation.
    CURL* tmp = curl_easy_init();
    if (!tmp) {
        return setReturnDataString(data_block, "unauthorised: curl init failed",
                                   "HELP authorisation check");
    }
    char* escaped_name  = curl_easy_escape(tmp, claim_name,  0);
    char* escaped_value = curl_easy_escape(tmp, claim_value, 0);
    const std::string auth_url = std::string{base_url}
                                + "?claim=" + (escaped_name  ? escaped_name  : claim_name)
                                + "&value=" + (escaped_value ? escaped_value : claim_value);
    curl_free(escaped_name);
    curl_free(escaped_value);
    curl_easy_cleanup(tmp);

    try {
        const uda::authentication::CurlWrapper curl;
        const std::string response = curl.perform_get_request(auth_url);
        const bool authorised = (response == expected);
        return setReturnDataString(data_block,
                                   authorised ? "authorised" : "unauthorised",
                                   "HELP authorisation check");
    } catch (...) {
        return setReturnDataString(data_block, "unauthorised: authorisation service error",
                                   "HELP authorisation check");
    }
}

#endif // OIDCAUTHENTICATION

static int do_server_metadata(IDAM_PLUGIN_INTERFACE* plugin_interface)
{
    using namespace uda::common::env_config;

    std::string meta;

    // --- Compile-time capabilities ---

#ifdef SSLAUTHENTICATION
    meta += "ssl_authentication=1\n";
#else
    meta += "ssl_authentication=0\n";
#endif

#ifdef CAPNP_ENABLED
    meta += "capnp_enabled=1\n";
#else
    meta += "capnp_enabled=0\n";
#endif

#ifdef FATCLIENT
    meta += "fat_client=1\n";
#else
    meta += "fat_client=0\n";
#endif

#ifdef SECURITYENABLED
    meta += "security_enabled=1\n";
#else
    meta += "security_enabled=0\n";
#endif

#ifdef EXTERNAL_USER
    meta += "external_user_forced=1\n";
#else
    meta += "external_user_forced=0\n";
#endif

    // --- Build info ---

    meta += fmt::format("server_version={}\n", UDA_BUILD_VERSION);
    meta += fmt::format("build_date={}\n", __DATE__);

    // --- Runtime: TLS ---
    // Report effective TLS mode. UDA_SERVER_SSL_AUTHENTICATE is the legacy boolean fallback
    // that implies mutual TLS when no explicit mode is set.

    const char* tls_mode_env = getenv("UDA_SERVER_TLS_MODE");
    if (tls_mode_env != nullptr && tls_mode_env[0] != '\0') {
        meta += fmt::format("tls_mode={}\n", tls_mode_env);
    } else if (evaluate_bool_param("UDA_SERVER_SSL_AUTHENTICATE", false)) {
        meta += "tls_mode=mutual\n";
    } else {
        meta += "tls_mode=off\n";
    }

    // --- Runtime: authentication ---

    const char* auth_env = getenv("UDA_SERVER_AUTHENTICATION");
    meta += fmt::format("authentication={}\n", auth_env != nullptr ? auth_env : "none");

    // Report whether Keycloak is configured without exposing the realm URL or client ID.
    const bool keycloak_configured = getenv("UDA_SERVER_KEYCLOAK_REALM") != nullptr
                                  && getenv("UDA_SERVER_KEYCLOAK_CLIENT_ID") != nullptr;
    meta += fmt::format("keycloak_configured={}\n", keycloak_configured ? 1 : 0);

    return setReturnDataString(plugin_interface->data_block, meta.c_str(),
                               "Server compilation flags and runtime configuration");
}
