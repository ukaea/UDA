#include <catch2/catch_test_macros.hpp>
#include "client2/host_list.hpp"
#include "config/config.h"

#include <iostream>

// #include "config/config.h"
// #include "clientserver/socket_structs.h"
// #include "clientserver/uda_structs.h"
// #include <string>
// #include <utility>

#include <sstream>

//TODO: test SSL syntax
//TODO: test extract port from hostname ie a.b.c:999 or localhost:9999
//TODO: test port extraction not legal for IP6 host names

TEST_CASE( "host list object can be default constructed", "[host_list-instantiation]" )
{
    REQUIRE_NOTHROW(uda::client::HostList());
}

TEST_CASE( "host list object can be constructed with the path of a valid config file", "[host_list-instantiation]" )
{
    const std::string input =
    "[[host_list]]\n"
    "host_alias=\"localhost\"\n"
    "host_name=\"localhost\"\n"
    "port=56565\n"
    "\n"
    "[[host_list]]\n"
    "host_alias=\"uda2\"\n"
    "host_name=\"uda2.mast.l\"\n"
    "port=56565\n"
    "\n"
    "[[host_list]]\n"
    "host_alias=\"uda3-ssl\"\n"
    "host_name=\"uda3.mast.l\"\n"
    "port=56560\n"
    "\n"
    "[[host_list]]\n"
    "host_alias=\"uda3\"\n"
    "host_name=\"uda3.mast.l\"\n"
    "port=56563\n"
    "\n"
    "[[host_list]]\n"
    "host_alias=\"uda3-ext\"\n"
    "host_name=\"data.mastu.ukaea.uk\"\n"
    "port=56565\n"
    ;
    std::istringstream stream{input};
    REQUIRE_NOTHROW(uda::client::HostList(stream, __FILE__));
}

TEST_CASE( "host list parses config file correctly into array of HostData structs", "[host_list-instantiation]" )
{
    const std::string input =
    "# ordering is important here as parser pushes new struct to the list on each \"host_name\" entry\n"
    "# host_name must always be the first field defined for each entry\n"
    "host_name localhost\n"
    "host_alias localhost\n"
    "port 56565\n"
    "\n"
    "host_name uda2.mast.l\n"
    "host_alias uda2\n"
    "port 56565\n"
    "\n"
    "host_name uda3.mast.l\n"
    "host_alias uda3-ssl\n"
    "port 56560\n"
    "\n"
    "host_name uda3.mast.l\n"
    "host_alias uda3\n"
    "port 56563\n"
    "\n"
    "host_name data.mastu.ukaea.uk\n"
    "host_alias uda3-ext\n"
    "port 56565\n"
    ;
    std::istringstream stream{input};
    uda::client::HostList host_list{stream, __FILE__, false};

    auto hosts = host_list.get_host_list();
    REQUIRE_FALSE(hosts.empty());
    REQUIRE(hosts.size() == 5);

    for (const auto& host: hosts)
    {
        std::cout << std::endl << "---- ENTRY ----" << std::endl;
        std::cout << "alias: " << host.host_alias << std::endl;
        std::cout << "name: " << host.host_name << std::endl;
        std::cout << "port: " << host.port << std::endl;
    }

    auto entry = host_list.find_by_alias("localhost");
    REQUIRE(entry != nullptr);
    REQUIRE(entry->host_name == "localhost");
    REQUIRE(entry->port == 56565);

    entry = host_list.find_by_alias("uda2");
    REQUIRE(entry != nullptr);
    REQUIRE(entry->host_name == "uda2.mast.l");
    REQUIRE(entry->port == 56565);

    entry = host_list.find_by_alias("uda3-ssl");
    REQUIRE(entry != nullptr);
    REQUIRE(entry->host_name == "uda3.mast.l");
    REQUIRE(entry->port == 56560);

    entry = host_list.find_by_alias("uda3");
    REQUIRE(entry != nullptr);
    REQUIRE(entry->host_name == "uda3.mast.l");
    REQUIRE(entry->port == 56563);

    entry = host_list.find_by_alias("uda3-ext");
    REQUIRE(entry != nullptr);
    REQUIRE(entry->host_name == "data.mastu.ukaea.uk");
    REQUIRE(entry->port == 56565);
}

TEST_CASE( "host-list can be parsed from a toml config file", "[host_list-toml]" )
{
    uda::config::Config config {};
    config.load("test_files/host_list.toml");
    REQUIRE_NOTHROW(uda::client::HostList(config));
    uda::client::HostList host_list(config);

    auto hosts = host_list.get_host_list();
    REQUIRE_FALSE(hosts.empty());
    REQUIRE(hosts.size() == 5);

    for (const auto& host: hosts)
    {
        std::cout << std::endl << "---- ENTRY ----" << std::endl;
        std::cout << "alias: " << host.host_alias << std::endl;
        std::cout << "name: " << host.host_name << std::endl;
        std::cout << "port: " << host.port << std::endl;
    }

    auto entry = host_list.find_by_alias("localhost");
    REQUIRE(entry != nullptr);
    REQUIRE(entry->host_name == "localhost");
    REQUIRE(entry->port == 56565);

    entry = host_list.find_by_alias("uda2");
    REQUIRE(entry != nullptr);
    REQUIRE(entry->host_name == "uda2.mast.l");
    REQUIRE(entry->port == 56565);

    entry = host_list.find_by_alias("uda3-ssl");
    REQUIRE(entry != nullptr);
    REQUIRE(entry->host_name == "uda3.mast.l");
    REQUIRE(entry->port == 56560);

    entry = host_list.find_by_alias("uda3");
    REQUIRE(entry != nullptr);
    REQUIRE(entry->host_name == "uda3.mast.l");
    REQUIRE(entry->port == 56563);

    entry = host_list.find_by_alias("uda3-ext");
    REQUIRE(entry != nullptr);
    REQUIRE(entry->host_name == "data.mastu.ukaea.uk");
    REQUIRE(entry->port == 56565);

}
