#include <catch2/catch_test_macros.hpp>

#include <cstdlib>
#include <cstring>
#include <filesystem>
#include <fstream>
#include <iterator>
#include <string>
#include <system_error>

#include <clientserver/errorLog.h>
#include <clientserver/initStructs.h>
#include <clientserver/makeRequestBlock.h>
#include <server/pluginStreamRedirect.hpp>
#include <server/serverPlugin.h>

namespace {

namespace fs = std::filesystem;

struct TestDirectory {
    TestDirectory()
    {
        char name[] = "/tmp/uda-resources-XXXXXX";
        const char* dir = mkdtemp(name);
        REQUIRE(dir != nullptr);
        path = dir;
    }

    ~TestDirectory()
    {
        std::error_code error;
        fs::remove_all(path, error);
    }

    fs::path path;
};

int decode_source(const fs::path& path)
{
    ENVIRONMENT environment{};
    std::strcpy(environment.api_delim, "::");

    PLUGIN_DATA plugins[3]{};
    std::strcpy(plugins[0].format, "netcdf");
    std::strcpy(plugins[1].format, "hdf5");
    std::strcpy(plugins[2].format, "IDA3");
    for (int i = 0; i < 3; ++i) {
        plugins[i].request = REQUEST_READ_START + i;
        plugins[i].plugin_class = UDA_PLUGIN_CLASS_FILE;
    }
    PLUGINLIST list{};
    list.count = 3;
    list.plugin = plugins;

    REQUEST_DATA request;
    initRequestData(&request);
    std::strcpy(request.signal, "test");
    std::strcpy(request.source, path.c_str());
    const int error = makeRequestData(&request, list, &environment);
    freeNameValueList(&request.nameValueList);
    closeUdaError();
    return error == 0 ? request.request : -1;
}

#ifdef __linux__
size_t descriptor_count()
{
    return std::distance(fs::directory_iterator("/proc/self/fd"), fs::directory_iterator{});
}
#endif

} // namespace

TEST_CASE("Extensionless source probes release their command pipes")
{
    TestDirectory directory;
    const auto netcdf = directory.path / "classic";
    const auto hdf = directory.path / "container";
    const auto unknown = directory.path / "unknown";
    const auto missing = directory.path / "missing";
    std::ofstream(netcdf, std::ios::binary).write("CDF\x01\0", 5);
    std::ofstream(hdf, std::ios::binary).write("\x89HDF\r\n\x1a\n", 8);
    std::ofstream(unknown) << "unknown\n";

    REQUIRE(setenv("UDA_DUMP_IDA", "", 1) == 0);
    REQUIRE(unsetenv("UDA_DUMP_NETCDF") == 0);
    REQUIRE(decode_source(netcdf) == REQUEST_READ_START);
    REQUIRE(decode_source(hdf) == -1);
    REQUIRE(setenv("UDA_DUMP_NETCDF", "", 1) == 0);
    REQUIRE(decode_source(hdf) == -1);
    REQUIRE(setenv("UDA_DUMP_NETCDF", "printf 'netcdf example'", 1) == 0);
    REQUIRE(decode_source(hdf) == REQUEST_READ_START);
    REQUIRE(setenv("UDA_DUMP_NETCDF", "true", 1) == 0);
    REQUIRE(decode_source(hdf) == REQUEST_READ_START + 1);

    REQUIRE(setenv("UDA_DUMP_IDA", "printf 'one\\ntwo\\nthree\\nfour\\nopened\\n'", 1) == 0);
    REQUIRE(decode_source(unknown) == REQUEST_READ_START + 2);
    REQUIRE(setenv("UDA_DUMP_IDA", "printf 'one\\ntwo\\n'", 1) == 0);
    REQUIRE(decode_source(unknown) == -1);
    REQUIRE(setenv("UDA_DUMP_IDA", "true", 1) == 0);

#ifdef __linux__
    const auto before = descriptor_count();
#endif
    for (int i = 0; i < 10; ++i) {
        REQUIRE(decode_source(missing) == -1);
        REQUIRE(decode_source(unknown) == -1);
    }
#ifdef __linux__
    REQUIRE(descriptor_count() == before);
#endif
}

TEST_CASE("Plugin redirection restores streams and only removes its own temporary file")
{
    TestDirectory directory;
    const auto fixed = directory.path / "fixed";

    // Exercise the exported fallback as well as the session owner.
    REQUIRE(udaServerRedirectStdStreams(false, true) == 0);
    REQUIRE(setenv("UDA_PLUGIN_REDIRECT_FILE", fixed.c_str(), 1) == 0);
    REQUIRE(udaServerRedirectStdStreams(false) == 0);
    std::fputs("standalone stdout\n", stdout);
    REQUIRE(udaServerRedirectStdStreams(true) == 0);
    REQUIRE(udaServerRedirectStdStreams(false, true) == 0);
    REQUIRE(fs::is_regular_file(fixed));

    uda::PluginStreamRedirect redirect;
    redirect.activate();
    REQUIRE(redirect.close() == 0);
    REQUIRE(setenv("UDA_PLUGIN_REDIRECT_FILE", (directory.path / "missing/file").c_str(), 1) == 0);
#ifdef __linux__
    const auto before = descriptor_count();
#endif
    {
        uda::PluginOutputCapture capture;
        REQUIRE(capture.status() != 0);
    }
    REQUIRE_FALSE(redirect.is_capturing());
    closeUdaError();
#ifdef __linux__
    REQUIRE(descriptor_count() == before);
#endif

    REQUIRE(setenv("UDA_PLUGIN_REDIRECT_FILE", fixed.c_str(), 1) == 0);
    try {
        uda::PluginOutputCapture capture;
        REQUIRE(capture.status() == 0);
        {
            uda::PluginOutputCapture nested;
            REQUIRE(nested.status() == 0);
            std::fputs("nested stderr\n", stderr);
        }
        std::fputs("plugin stdout\n", stdout);
        throw -1;
    } catch (int plugin_error) {
        REQUIRE(plugin_error == -1);
    }
    REQUIRE_FALSE(redirect.is_capturing());
    REQUIRE(redirect.close() == 0);
    {
        std::ifstream file(fixed);
        const std::string contents(std::istreambuf_iterator<char>{file}, {});
        REQUIRE(contents.find("nested stderr\n") != std::string::npos);
        REQUIRE(contents.find("plugin stdout\n") != std::string::npos);
    }

    REQUIRE(unsetenv("UDA_PLUGIN_REDIRECT_FILE") == 0);
    REQUIRE(setenv("UDA_PLUGIN_REDIRECT_DIR", directory.path.c_str(), 1) == 0);
    {
        uda::PluginOutputCapture capture;
        REQUIRE(capture.status() == 0);
        REQUIRE(std::distance(fs::directory_iterator(directory.path), fs::directory_iterator{}) == 2);
    }
    REQUIRE(redirect.close() == 0);
    REQUIRE(redirect.close() == 0);
    REQUIRE(std::distance(fs::directory_iterator(directory.path), fs::directory_iterator{}) == 1);
    REQUIRE(fs::is_regular_file(fixed));
#ifdef __linux__
    REQUIRE(descriptor_count() == before);
#endif
}
