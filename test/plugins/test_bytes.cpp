#define CATCH_CONFIG_MAIN
#include "catch.hpp"

#include <c++/UDA.hpp>
#include <client/accAPI.h>

#include <cstdlib>
#include <filesystem>
#include <fstream>
#include <string>

namespace {

// Writes the integers 1..5 to a fresh file in a temporary directory and returns its canonical path
std::string make_test_file(const std::string& name = "bytes.dat")
{
    auto dir = std::filesystem::temp_directory_path() / "uda_test_bytes";
    std::filesystem::create_directories(dir);
    auto path = dir / name;
    std::ofstream out(path, std::ios::binary);
    for (int i = 1; i <= 5; ++i) {
        out.write(reinterpret_cast<const char*>(&i), sizeof(int));
    }
    out.close();
    return std::filesystem::canonical(path).string();
}

std::string test_dir()
{
    return std::filesystem::canonical(std::filesystem::temp_directory_path() / "uda_test_bytes").string();
}

std::string missing_file()
{
    return (std::filesystem::temp_directory_path() / "uda_test_bytes" / "does_not_exist.dat").string();
}

// Checks that a request failed with the given code, a first error message containing msg, and that the message
// appears exactly once in the returned error stack
void require_server_error(uda::Client& client, const std::string& request, int code, const std::string& msg)
{
    REQUIRE_THROWS_AS( client.get(request, ""), uda::ServerException );

    UDA_ERROR_STACK* errors = getUdaServerErrorStack();
    REQUIRE( errors != nullptr );
    REQUIRE( errors->nerrors > 0 );
    REQUIRE( errors->idamerror[0].code == code );
    REQUIRE_THAT( std::string(errors->idamerror[0].msg), Catch::Contains(msg) );

    int count = 0;
    for (unsigned int i = 0; i < errors->nerrors; ++i) {
        if (std::string(errors->idamerror[i].msg).find(msg) != std::string::npos) {
            ++count;
        }
    }
    REQUIRE( count == 1 );
}

#ifdef FATCLIENT
// The allow-list is read from the server's environment, which only the fat client can control from a test
class AllowedPaths
{
public:
    explicit AllowedPaths(const char* value)
    {
        const char* old = std::getenv("UDA_ALLOWED_PATHS");
        had_old_ = old != nullptr;
        if (had_old_) {
            old_ = old;
        }
        if (value == nullptr) {
            unsetenv("UDA_ALLOWED_PATHS");
        } else {
            setenv("UDA_ALLOWED_PATHS", value, 1);
        }
    }
    ~AllowedPaths()
    {
        if (had_old_) {
            setenv("UDA_ALLOWED_PATHS", old_.c_str(), 1);
        } else {
            unsetenv("UDA_ALLOWED_PATHS");
        }
    }
private:
    bool had_old_ = false;
    std::string old_;
};
#endif

} // namespace

TEST_CASE( "Test BYTES::help() function", "[BYTES][plugins]" )
{
#include "setup.inc"

    uda::Client client;

    const uda::Result& result = client.get("BYTES::help()", "");

    REQUIRE( result.errorCode() == 0 );
    REQUIRE( result.errorMessage().empty() );

    uda::Data* data = result.data();

    REQUIRE( data != nullptr );
    REQUIRE( !data->isNull() );
    REQUIRE( data->type().name() == typeid(char*).name() );

    auto str = dynamic_cast<uda::String*>(data);

    REQUIRE( str != nullptr );

    std::string expected = "\nbytes: data reader to access files as a block of bytes without interpretation\n\n";

    REQUIRE( str->str() == expected );
}

TEST_CASE( "Test BYTES::read() of a missing file reports a distinct error", "[BYTES][plugins]" )
{
#include "setup.inc"

    make_test_file();
    uda::Client client;

    std::string request = std::string("BYTES::read(path=") + missing_file() + ")";
    require_server_error(client, request, 999, "Provided File Path Not Found!");

    request = std::string("BYTES::size(path=") + missing_file() + ")";
    require_server_error(client, request, 999, "Provided File Path Not Found!");
}

#ifdef FATCLIENT

TEST_CASE( "Test BYTES::read() function", "[BYTES][plugins]" )
{
#include "setup.inc"

    std::string path = make_test_file();
    AllowedPaths allowed(test_dir().c_str());

    uda::Client client;

    std::string request = std::string("BYTES::read(path=") + path + ")";

    const uda::Result& result = client.get(request, "");

    REQUIRE( result.errorCode() == 0 );
    REQUIRE( result.errorMessage().empty() );

    uda::Data* data = result.data();

    REQUIRE( data != nullptr );
    REQUIRE( !data->isNull() );
    REQUIRE( data->type().name() == typeid(char).name() );

    auto arr = dynamic_cast<uda::Array*>(data);

    REQUIRE( arr != nullptr );

    REQUIRE( arr->type().name() == typeid(char).name() );
    REQUIRE( arr->size() == 5 * sizeof(int) );

    std::vector<char> bytes = arr->as<char>();

    auto numbers = reinterpret_cast<int*>(bytes.data());

    REQUIRE( numbers[0] == 1 );
    REQUIRE( numbers[1] == 2 );
    REQUIRE( numbers[2] == 3 );
    REQUIRE( numbers[3] == 4 );
    REQUIRE( numbers[4] == 5 );
}

TEST_CASE( "Test BYTES::size() function", "[BYTES][plugins]" )
{
#include "setup.inc"

    std::string path = make_test_file();
    AllowedPaths allowed(test_dir().c_str());

    uda::Client client;

    const uda::Result& result = client.get(std::string("BYTES::size(path=") + path + ")", "");

    REQUIRE( result.errorCode() == 0 );

    uda::Data* data = result.data();
    REQUIRE( data != nullptr );
    REQUIRE( !data->isNull() );

    auto scalar = dynamic_cast<uda::Scalar*>(data);
    REQUIRE( scalar != nullptr );
    REQUIRE( scalar->as<long>() == 5 * sizeof(int) );
}

TEST_CASE( "Test BYTES rejects existing files when UDA_ALLOWED_PATHS is empty", "[BYTES][plugins]" )
{
#include "setup.inc"

    std::string path = make_test_file();
    AllowedPaths allowed("");

    uda::Client client;

    require_server_error(client, std::string("BYTES::size(path=") + path + ")", 999, "Bad Path Provided " + path);
    require_server_error(client, std::string("BYTES::read(path=") + path + ")", 999, "Bad Path Provided " + path);
}

TEST_CASE( "Test BYTES rejects existing files when UDA_ALLOWED_PATHS is unset", "[BYTES][plugins]" )
{
#include "setup.inc"

    std::string path = make_test_file();
    AllowedPaths allowed(nullptr);

    uda::Client client;

    require_server_error(client, std::string("BYTES::size(path=") + path + ")", 999, "Bad Path Provided " + path);
    require_server_error(client, std::string("BYTES::read(path=") + path + ")", 999, "Bad Path Provided " + path);
}

TEST_CASE( "Test BYTES rejects files outside UDA_ALLOWED_PATHS", "[BYTES][plugins]" )
{
#include "setup.inc"

    std::string path = make_test_file();
    AllowedPaths allowed("/nonexistent/allowed/dir");

    uda::Client client;

    require_server_error(client, std::string("BYTES::read(path=") + path + ")", 999, "Bad Path Provided " + path);
}

#endif // FATCLIENT
