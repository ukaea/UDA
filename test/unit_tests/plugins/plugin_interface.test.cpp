// Unit tests for plugin-interface error delivery, nested calls and version negotiation

#include <catch2/catch_test_macros.hpp>

#include <cstring>
#include <memory>
#include <string>

#include <clientserver/errorLog.h>
#include <clientserver/initStructs.h>
#include <clientserver/makeRequestBlock.h>
#include <plugins/udaPlugin.h>
#include <server/pluginInterface.h>

namespace {

int g_calls = 0;
int g_seen_version = 0;

int ok_plugin(IDAM_PLUGIN_INTERFACE* plugin_interface)
{
    ++g_calls;
    g_seen_version = plugin_interface->interfaceVersion;
    REQUIRE( plugin_interface->error_stack.nerrors == 0 );     // child starts with an empty stack
    return 0;
}

int failing_plugin(IDAM_PLUGIN_INTERFACE* plugin_interface)
{
    ++g_calls;
    UDA_PLUGIN_RAISE_ERROR_CODE(plugin_interface, 4242, "child detail");
}

int silent_plugin(IDAM_PLUGIN_INTERFACE*)
{
    ++g_calls;
    return 4343;
}

struct Fixture {
    ENVIRONMENT environment = {};
    PLUGINLIST plugin_list = {};
    PLUGIN_DATA plugins[4] = {};
    REQUEST_DATA request = {};
    std::unique_ptr<IDAM_PLUGIN_INTERFACE> parent = std::make_unique<IDAM_PLUGIN_INTERFACE>();

    Fixture()
    {
        strcpy(environment.api_delim, "::");
        add(0, "OKPLUGIN", ok_plugin, 1);
        add(1, "FAILPLUGIN", failing_plugin, 2);
        add(2, "SILENTPLUGIN", silent_plugin, 2);
        add(3, "FUTUREPLUGIN", ok_plugin, 99);
        plugin_list.count = 4;
        plugin_list.plugin = plugins;

        initRequestData(&request);
        udaInitHostPluginInterface(parent.get(), &plugins[1]);   // parent negotiated at version 2
        parent->request_data = &request;
        parent->environment = &environment;
        parent->pluginList = &plugin_list;
        g_calls = 0;
        g_seen_version = 0;
    }

    void add(int i, const char* format, PLUGINFUNP entry, unsigned short version)
    {
        strcpy(plugins[i].format, format);
        plugins[i].request = 1000 + i;
        plugins[i].plugin_class = UDA_PLUGIN_CLASS_FUNCTION;
        plugins[i].idamPlugin = entry;
        plugins[i].interfaceVersion = version;
    }
};

} // namespace

TEST_CASE( "udaAddErrorToStack keeps the earliest records and terminates strings", "[plugin_interface]" )
{
    auto stack = std::make_unique<UDA_ERROR_STACK>();
    stack->nerrors = 0;

    std::string long_msg(STRING_LENGTH * 2, 'x');
    REQUIRE( udaAddErrorToStack(stack.get(), UDA_CODE_ERROR_TYPE, "here", 1, long_msg.c_str()) == 0 );
    REQUIRE( strlen(stack->idamerror[0].msg) == STRING_LENGTH - 1 );

    for (int i = 1; i < UDA_MAX_ERRORS; ++i) {
        REQUIRE( udaAddErrorToStack(stack.get(), UDA_CODE_ERROR_TYPE, "here", i + 1, "msg") == 0 );
    }
    REQUIRE( udaAddErrorToStack(stack.get(), UDA_CODE_ERROR_TYPE, "here", 999, "dropped") == 1 );
    REQUIRE( stack->nerrors == UDA_MAX_ERRORS );
    REQUIRE( stack->idamerror[0].code == 1 );

    REQUIRE( udaAddErrorToStack(nullptr, UDA_CODE_ERROR_TYPE, "here", 1, "msg") == -1 );

    auto fresh = std::make_unique<UDA_ERROR_STACK>();
    fresh->nerrors = 0;
    REQUIRE( udaAddErrorToStack(fresh.get(), UDA_CODE_ERROR_TYPE, nullptr, 1, nullptr) == 0 );
    REQUIRE( fresh->idamerror[0].location[0] == '\0' );
    REQUIRE( fresh->idamerror[0].msg[0] == '\0' );
}

TEST_CASE( "udaEnsureErrorRecorded adds a fallback only for an empty failed stack", "[plugin_interface]" )
{
    auto stack = std::make_unique<UDA_ERROR_STACK>();
    stack->nerrors = 0;

    REQUIRE( udaEnsureErrorRecorded(stack.get(), 0, "loc") == 0 );
    REQUIRE( stack->nerrors == 0 );

    REQUIRE( udaEnsureErrorRecorded(stack.get(), 123, "loc") == 1 );
    REQUIRE( stack->nerrors == 1 );
    REQUIRE( stack->idamerror[0].code == 123 );
    REQUIRE( std::string(stack->idamerror[0].msg).find("error code 123") != std::string::npos );

    REQUIRE( udaEnsureErrorRecorded(stack.get(), 456, "loc") == 0 );
    REQUIRE( stack->nerrors == 1 );
}

TEST_CASE( "Host negotiates min(registered, host) interface version", "[plugin_interface]" )
{
    auto plugin_interface = std::make_unique<IDAM_PLUGIN_INTERFACE>();
    PLUGIN_DATA plugin = {};

    plugin.interfaceVersion = 1;
    udaInitHostPluginInterface(plugin_interface.get(), &plugin);
    REQUIRE( plugin_interface->interfaceVersion == 1 );
    REQUIRE( plugin_interface->log_function != nullptr );
    REQUIRE( plugin_interface->error_stack.nerrors == 0 );

    plugin.interfaceVersion = 0;        // missing registration column => 1
    udaSetPluginInterfaceTarget(plugin_interface.get(), &plugin);
    REQUIRE( plugin_interface->interfaceVersion == 1 );

    plugin.interfaceVersion = 2;
    udaSetPluginInterfaceTarget(plugin_interface.get(), &plugin);
    REQUIRE( plugin_interface->interfaceVersion == 2 );

    plugin.interfaceVersion = 99;
    udaSetPluginInterfaceTarget(plugin_interface.get(), &plugin);
    REQUIRE( plugin_interface->interfaceVersion == UDA_PLUGIN_INTERFACE_VERSION );
    REQUIRE( udaCheckPluginInterfaceVersion(&plugin) == 999 );

    plugin.interfaceVersion = UDA_PLUGIN_INTERFACE_VERSION;
    REQUIRE( udaCheckPluginInterfaceVersion(&plugin) == 0 );
}

TEST_CASE( "Nested call: child gets its registered version and an empty stack", "[plugin_interface]" )
{
    Fixture f;
    REQUIRE( udaCallPlugin(f.parent.get(), "OKPLUGIN::fn()") == 0 );
    REQUIRE( g_calls == 1 );
    REQUIRE( g_seen_version == 1 );
    REQUIRE( f.parent->error_stack.nerrors == 0 );
}

TEST_CASE( "Nested call: child error is merged into the parent exactly once", "[plugin_interface]" )
{
    Fixture f;
    REQUIRE( udaCallPlugin(f.parent.get(), "FAILPLUGIN::fn()") == 4242 );
    REQUIRE( f.parent->error_stack.nerrors == 1 );
    REQUIRE( f.parent->error_stack.idamerror[0].code == 4242 );
    REQUIRE( std::string(f.parent->error_stack.idamerror[0].msg) == "child detail" );
}

TEST_CASE( "Nested call: silent child failure still carries its code", "[plugin_interface]" )
{
    Fixture f;
    // An unrelated earlier record on the parent must not suppress the child's fallback record
    udaPluginAddError(f.parent.get(), UDA_CODE_ERROR_TYPE, "parent", 1, "earlier parent record");
    REQUIRE( udaCallPlugin(f.parent.get(), "SILENTPLUGIN::fn()") == 4343 );
    REQUIRE( f.parent->error_stack.nerrors == 2 );
    REQUIRE( f.parent->error_stack.idamerror[1].code == 4343 );
}

TEST_CASE( "Nested call: registration above the host version is rejected before the entry point", "[plugin_interface]" )
{
    Fixture f;
    REQUIRE( udaCallPlugin(f.parent.get(), "FUTUREPLUGIN::fn()") == 999 );
    REQUIRE( g_calls == 0 );
    REQUIRE( f.parent->error_stack.nerrors == 1 );
    REQUIRE( std::string(f.parent->error_stack.idamerror[0].msg).find("interface version 99") != std::string::npos );
}

TEST_CASE( "Nested call: unknown plugin is reported on the parent", "[plugin_interface]" )
{
    Fixture f;
    REQUIRE( udaCallPlugin(f.parent.get(), "NOSUCHPLUGIN::fn()") != 0 );
    REQUIRE( g_calls == 0 );
    REQUIRE( f.parent->error_stack.nerrors >= 1 );
}

namespace {

struct CapturedLog {
    int count = 0;
    int level = 0;
    std::string message;
};

void capture_log(void* context, int level, const char*, int, const char* message)
{
    auto captured = static_cast<CapturedLog*>(context);
    ++captured->count;
    captured->level = level;
    captured->message = message;
}

} // namespace

TEST_CASE( "udaPluginDiagnostics records on the interface and logs through the host", "[plugin_interface][diagnostics]" )
{
    Fixture f;
    CapturedLog captured;
    f.parent->log_function = capture_log;
    f.parent->log_context = &captured;
    f.parent->log_level = UDA_LOG_INFO;

    UDA_DIAGNOSTICS diag = udaPluginDiagnostics(f.parent.get());
    UDA_DIAG_ADD_ERROR(&diag, 77, "helper failed");
    UDA_DIAG_LOG(&diag, UDA_LOG_DEBUG, "below the host level %d\n", 1);
    UDA_DIAG_LOG(&diag, UDA_LOG_WARN, "trace %d\n", 5);

    REQUIRE( f.parent->error_stack.nerrors == 1 );
    REQUIRE( f.parent->error_stack.idamerror[0].code == 77 );
    REQUIRE( std::string(f.parent->error_stack.idamerror[0].msg) == "helper failed" );
    REQUIRE( captured.count == 1 );
    REQUIRE( captured.level == UDA_LOG_WARN );
    REQUIRE( captured.message == "trace 5\n" );

    SECTION( "a version 1 interface has no host log sink" ) {
        f.parent->interfaceVersion = 1;
        UDA_DIAGNOSTICS v1 = udaPluginDiagnostics(f.parent.get());
        UDA_DIAG_LOG(&v1, UDA_LOG_ERROR, "not delivered\n");
        REQUIRE( captured.count == 1 );
        UDA_DIAG_ADD_ERROR(&v1, 78, "still recorded");
        REQUIRE( f.parent->error_stack.nerrors == 2 );
    }
}

TEST_CASE( "A null diagnostics context discards output", "[plugin_interface][diagnostics]" )
{
    REQUIRE( udaDiagAddError(nullptr, UDA_CODE_ERROR_TYPE, "here", 1, "dropped") != 0 );
    REQUIRE( udaDiagLogEnabled(nullptr, UDA_LOG_ERROR) == 0 );
    UDA_DIAG_LOG((const UDA_DIAGNOSTICS*)nullptr, UDA_LOG_ERROR, "dropped\n");
}

TEST_CASE( "udaGlobalDiagnostics forwards to the library error stack", "[plugin_interface][diagnostics]" )
{
    int before = udaNumErrors();
    UDA_DIAGNOSTICS diag = udaGlobalDiagnostics();
    UDA_DIAG_ADD_ERROR(&diag, 79, "global record");
    REQUIRE( udaNumErrors() == before + 1 );
    closeUdaError();
}

TEST_CASE( "Nested call: request parse errors reach the parent, not global state", "[plugin_interface][diagnostics]" )
{
    Fixture f;
    int global_before = udaNumErrors();

    // An empty request is rejected by udaMakeRequestData()
    REQUIRE( udaCallPlugin(f.parent.get(), "") != 0 );
    REQUIRE( g_calls == 0 );
    REQUIRE( udaNumErrors() == global_before );
    REQUIRE( f.parent->error_stack.nerrors >= 2 );
    REQUIRE( std::string(f.parent->error_stack.idamerror[0].msg) == "Neither Data Object nor Source specified!" );
    unsigned int last = f.parent->error_stack.nerrors - 1;
    REQUIRE( std::string(f.parent->error_stack.idamerror[last].msg).find("Unable to parse plugin call request")
             != std::string::npos );
}

TEST_CASE( "Nested call: a successful parse leaves no records behind", "[plugin_interface][diagnostics]" )
{
    Fixture f;
    REQUIRE( udaCallPlugin(f.parent.get(), "OKPLUGIN::fn(a=1, b=two)") == 0 );
    REQUIRE( g_calls == 1 );
    REQUIRE( f.parent->error_stack.nerrors == 0 );
}

namespace {

bool has_error(const UDA_ERROR_STACK& stack, const std::string& text)
{
    for (unsigned int i = 0; i < stack.nerrors; ++i) {
        if (std::string(stack.idamerror[i].msg).find(text) != std::string::npos) {
            return true;
        }
    }
    return false;
}

} // namespace

TEST_CASE( "Subset parse errors fail the request", "[plugin_interface][subset]" )
{
    Fixture f;

    SECTION( "too many elements" ) {
        REQUIRE( udaCallPlugin(f.parent.get(), "OKPLUGIN::fn()[1:2:3:4]") != 0 );
        REQUIRE( has_error(f.parent->error_stack, "Invalid number of elements in subset operation") );
    }

    SECTION( "a non-integer index is reported, not thrown" ) {
        REQUIRE( udaCallPlugin(f.parent.get(), "OKPLUGIN::fn()[abc]") != 0 );
        REQUIRE( has_error(f.parent->error_stack, "Invalid integer in subset operation: abc") );
    }

    SECTION( "an out of range index is reported, not thrown" ) {
        REQUIRE( udaCallPlugin(f.parent.get(), "OKPLUGIN::fn()[99999999999999999999]") != 0 );
        REQUIRE( has_error(f.parent->error_stack, "Integer out of range in subset operation") );
    }

    REQUIRE( g_calls == 0 );
    REQUIRE( has_error(f.parent->error_stack, "Subset operation is incorrect!") );
}

TEST_CASE( "Subset wildcards and padded indices parse", "[plugin_interface][subset]" )
{
    // Parsed directly: a successful nested call would go on to subset the (absent) fixture data block
    Fixture f;
    UDA_DIAGNOSTICS diag = udaPluginDiagnostics(f.parent.get());

    REQUEST_DATA request = {};
    initRequestData(&request);
    strcpy(request.signal, "OKPLUGIN::fn()[*]");
    REQUIRE( udaMakeRequestData(&diag, &request, f.plugin_list, &f.environment) == 0 );
    REQUIRE( request.datasubset.nbound == 1 );
    REQUIRE( !request.datasubset.lbindex[0].init );

    initRequestData(&request);
    strcpy(request.signal, "OKPLUGIN::fn()[2:*, 0:10:2 ]");
    REQUIRE( udaMakeRequestData(&diag, &request, f.plugin_list, &f.environment) == 0 );
    REQUIRE( request.datasubset.nbound == 2 );
    REQUIRE( request.datasubset.lbindex[0].value == 2 );
    REQUIRE( !request.datasubset.ubindex[0].init );
    REQUIRE( request.datasubset.ubindex[1].value == 10 );
    REQUIRE( request.datasubset.stride[1].value == 2 );

    REQUIRE( f.parent->error_stack.nerrors == 0 );
}
