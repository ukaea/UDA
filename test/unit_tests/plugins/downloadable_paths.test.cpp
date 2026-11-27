// catch includes
#include <catch2/catch_test_macros.hpp>

// std includes
#include <cstdlib>
#include <filesystem>
#include <fstream>
#include <cstdio>
#include <string>
#include <sys/stat.h>

// uda includes
#include <plugins/utils.h>

namespace fs = std::filesystem;
using uda::plugins::path_in_list;

namespace {

// A temporary tree: data/file, data2/file, and link -> data.
class Tree
{
  public:
    Tree()
    {
        root = fs::canonical(fs::temp_directory_path()) /
               ("uda-downloadable-" + std::to_string(std::rand()) + std::to_string(std::rand()));
        fs::create_directories(root / "data");
        fs::create_directories(root / "data2");
        std::ofstream(root / "data" / "file") << "x";
        std::ofstream(root / "data2" / "file") << "x";
        fs::create_directory_symlink(root / "data", root / "link");
    }
    ~Tree() { fs::remove_all(root); }

    std::string operator()(const std::string& relative) const { return (root / relative).string(); }

    fs::path root;
};

class EnvVar
{
  public:
    EnvVar(const char* name, const std::string& value) : name_(name) { setenv(name, value.c_str(), 1); }
    ~EnvVar() { unsetenv(name_); }

  private:
    const char* name_;
};

} // namespace

TEST_CASE("downloadable path entries match whole path components", "[downloadable-paths]")
{
    Tree tree;
    const auto file = tree("data/file");
    REQUIRE(path_in_list(file, tree("data")));
    REQUIRE(path_in_list(file, tree("data") + "/"));
    REQUIRE(path_in_list(file, file));
    REQUIRE(path_in_list(tree("data"), tree("data")));
    REQUIRE_FALSE(path_in_list(tree("data2/file"), tree("data")));
    REQUIRE(path_in_list(file, "/"));
}

TEST_CASE("empty and malformed downloadable path entries never match", "[downloadable-paths]")
{
    Tree tree;
    const auto other = tree("data2/file");
    REQUIRE_FALSE(path_in_list(other, ""));
    REQUIRE_FALSE(path_in_list(other, ":"));
    REQUIRE_FALSE(path_in_list(other, tree("data") + ":"));          // a trailing separator used to allow everything
    REQUIRE_FALSE(path_in_list(other, "::" + tree("data")));
    REQUIRE_FALSE(path_in_list(other, tree("data") + ";", ';'));
    REQUIRE_FALSE(path_in_list(other, "relative/data2"));
    REQUIRE_FALSE(path_in_list(other, tree("missing")));
    REQUIRE(path_in_list(other, tree("missing") + ":" + tree("data2")));
}

TEST_CASE("downloadable path entries are trimmed and resolved", "[downloadable-paths]")
{
    Tree tree;
    const auto file = tree("data/file");
    REQUIRE(path_in_list(file, "  " + tree("data") + " : " + tree("data2")));
    REQUIRE(path_in_list(file, tree("data2") + ";" + tree("data"), ';'));
    REQUIRE_FALSE(path_in_list(file, tree("data2") + ";" + tree("data")));
    REQUIRE(path_in_list(file, tree("link")));                         // a listed symlink means its target
    REQUIRE(path_in_list(file, tree("data2/../data")));
    REQUIRE_FALSE(path_in_list(tree("data2/file"), tree("link")));
}

TEST_CASE("UDA_DOWNLOADABLE_PATHS takes precedence over UDA_ALLOWED_PATHS", "[downloadable-paths]")
{
    Tree tree;
    const auto file = tree("data/file");
    unsetenv("UDA_DOWNLOADABLE_PATHS");
    unsetenv("UDA_ALLOWED_PATHS");

    SECTION("neither set rejects everything")
    {
        REQUIRE(check_allowed_path(file.c_str()) != 0);
    }
    SECTION("the new name is used")
    {
        EnvVar value{"UDA_DOWNLOADABLE_PATHS", tree("data")};
        REQUIRE(check_allowed_path(file.c_str()) == 0);
        REQUIRE(check_allowed_path(tree("data2/file").c_str()) != 0);
    }
    SECTION("the legacy name still works")
    {
        EnvVar legacy{"UDA_ALLOWED_PATHS", tree("data")};
        REQUIRE(check_allowed_path(file.c_str()) == 0);
    }
    SECTION("an empty new name falls back to the legacy name")
    {
        EnvVar value{"UDA_DOWNLOADABLE_PATHS", ""};
        EnvVar legacy{"UDA_ALLOWED_PATHS", tree("data")};
        REQUIRE(check_allowed_path(file.c_str()) == 0);
    }
    SECTION("when both are set, the legacy name is ignored")
    {
        EnvVar value{"UDA_DOWNLOADABLE_PATHS", tree("data2")};
        EnvVar legacy{"UDA_ALLOWED_PATHS", tree("data")};
        REQUIRE(check_allowed_path(file.c_str()) != 0);
        REQUIRE(check_allowed_path(tree("data2/file").c_str()) == 0);
    }
    SECTION("the new name is ':'-separated and the legacy name keeps ';'")
    {
        {
            EnvVar value{"UDA_DOWNLOADABLE_PATHS", tree("data2") + ":" + tree("data")};
            REQUIRE(check_allowed_path(file.c_str()) == 0);
        }
        {
            EnvVar value{"UDA_DOWNLOADABLE_PATHS", tree("data2") + ";" + tree("data")};
            REQUIRE(check_allowed_path(file.c_str()) != 0);
        }
        EnvVar legacy{"UDA_ALLOWED_PATHS", tree("data2") + ";" + tree("data")};
        REQUIRE(check_allowed_path(file.c_str()) == 0);
    }
    SECTION("a missing requested path is rejected")
    {
        EnvVar value{"UDA_DOWNLOADABLE_PATHS", "/"};
        REQUIRE(check_allowed_path(tree("data/missing").c_str()) != 0);
    }
}

TEST_CASE("open_downloadable_file checks the file it opened", "[downloadable-paths]")
{
    Tree tree;
    EnvVar value{"UDA_DOWNLOADABLE_PATHS", tree("data")};
    FILE* file = reinterpret_cast<FILE*>(1);

    auto opens = [&](const std::string& path) {
        const int rc = open_downloadable_file(path.c_str(), &file);
        REQUIRE((rc == 0) == (file != nullptr));
        if (file != nullptr) {
            REQUIRE(std::fgetc(file) == 'x');
            std::fclose(file);
        }
        return rc == 0;
    };

    REQUIRE(opens(tree("data/file")));
    REQUIRE(opens(tree("link/file")));
    REQUIRE_FALSE(opens(tree("data2/file")));
    REQUIRE_FALSE(opens(tree("data/missing")));

    SECTION("a symlink in a downloadable directory is judged by its target")
    {
        // What a symlink swapped in after a name check would look like at open time.
        fs::create_symlink(tree("data2/file"), tree("data/escape"));
        REQUIRE_FALSE(opens(tree("data/escape")));
        fs::create_symlink(tree("data/file"), tree("data2/inward"));
        REQUIRE(opens(tree("data2/inward")));
    }
    SECTION("only regular files are opened")
    {
        REQUIRE_FALSE(opens(tree("data")));
        REQUIRE(mkfifo(tree("data/fifo").c_str(), 0600) == 0);
        REQUIRE_FALSE(opens(tree("data/fifo"))); // and does not block waiting for a writer
    }
}
