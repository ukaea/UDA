#include <catch2/catch_test_macros.hpp>
#include "clientserver/name_value_list.hpp"

TEST_CASE( "Name-value list can be default constructed", "[nvl-instantiation]" )
{
    REQUIRE_NOTHROW( NameValueList() );
}

TEST_CASE( "Name-value list can be constructed from a string", "[nvl-instantiation]" )
{
    std::string request_string = "key1=val1, key2=val2";
    REQUIRE_NOTHROW( NameValueList(request_string, true) );
}

TEST_CASE( "Name-value list can be copy-constructed and copy-assigned", "[nvl-instantiation]" )
{
    NameValueList nvl1 = {};

    std::string request_string = "key1=val1, key2=val2";
    auto nvl2 = NameValueList(request_string, true);

    request_string = "";
    auto nvl3 = NameValueList(request_string, true);

    NameValueList nvl = nvl1;
    nvl = nvl2;
    nvl = nvl3;
}

TEST_CASE( "Name-value list can be move-constructed and move-assigned", "[nvl-instantiation]" )
{
    std::string request_string = "key1=val1, key2=val2";
    auto nvl1 = NameValueList(request_string, true);
    
    NameValueList nvl2 = {};

    request_string = "";
    auto nvl3 = NameValueList(request_string, true);

    NameValueList nvl = std::move(nvl1);
    nvl = std::move(nvl2);
    nvl = std::move(nvl3);
}


struct Blah
{
    int i;
    NameValueList nvl;
};

void init_blah(Blah& blah)
{
    blah.i = 0;
    blah.nvl = {};
}

TEST_CASE( "namevalue list in struct doesn't randomly crash...", "[nvl-instantiation]" )
{
    {
        Blah blah {};
        init_blah(blah);
        REQUIRE( blah.nvl.empty() );

        std::string request_string {};
        blah.nvl.parse(request_string, true);
    }
    {
        Blah blah {};
        init_blah(blah);
        REQUIRE( blah.nvl.empty() );

        std::string request_string {};
        blah.nvl.parse(request_string, false);
    }

    {
        Blah blah {};
        init_blah(blah);
        REQUIRE( blah.nvl.empty() );
        std::string request_string = "key1=val1, key2=val2";
        blah.nvl.parse(request_string, true);
    }
    {
        Blah blah {};
        init_blah(blah);
        REQUIRE( blah.nvl.empty() );
        std::string request_string = "key1=val1, key2=val2";
        blah.nvl.parse(request_string, false);
    }

    {
        Blah blah {};
        init_blah(blah);
        REQUIRE( blah.nvl.empty() );
        std::string request_string = "help::help()";
        blah.nvl.parse(request_string, false);
    }
    {
        Blah blah {};
        init_blah(blah);
        REQUIRE( blah.nvl.empty() );
        std::string request_string = "help::help()";
        blah.nvl.parse(request_string, true);
    }
    {
        Blah blah {};
        init_blah(blah);
        REQUIRE( blah.nvl.empty() );
        std::string request_string = "help::help()";
        blah.nvl.parse(request_string, false);
    }
    {
        Blah blah {};
        init_blah(blah);
        REQUIRE( blah.nvl.empty() );
        std::string request_string = "help::help()";
        blah.nvl.parse(request_string, true);
    }
    {
        Blah blah {};
        init_blah(blah);
        std::string request_string = "key1=val1, key2=val2";
        auto nvl1 = NameValueList(request_string, true);

        request_string = "help::help()";
        auto nvl2 = NameValueList(request_string, true);

        request_string = "";
        auto nvl3 = NameValueList(request_string, true);

        blah.nvl = nvl1;
        blah.nvl = nvl2;
        blah.nvl = nvl3;
        blah.nvl = {};

        blah.nvl = std::move(nvl1);
        blah.nvl = std::move(nvl2);
        blah.nvl = std::move(nvl3);
    }

}
