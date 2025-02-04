#include <catch2/catch_test_macros.hpp>

#include "name_value_list.hpp"

TEST_CASE( "NameValueLists are parsed correctly", "[request]" ) {
    SECTION( "Single value" ) {
        NameValueList name_value_list{ "a=1", true };
        REQUIRE( name_value_list.size() == 1 );
        REQUIRE( name_value_list["a"] == "1" );
    }

    SECTION( "Multiple values" ) {
        SECTION( "Two values" ) {
            NameValueList name_value_list{ "a=1,b=2", true };
            REQUIRE( name_value_list.size() == 2 );
            REQUIRE( name_value_list["a"] == "1" );
            REQUIRE( name_value_list["b"] == "2" );
        }

        SECTION( "Three values" ) {
            NameValueList name_value_list{ "a=1,b=2,c=3", true };
            REQUIRE( name_value_list.size() == 3 );
            REQUIRE( name_value_list["a"] == "1" );
            REQUIRE( name_value_list["b"] == "2" );
            REQUIRE( name_value_list["c"] == "3" );
        }

        SECTION( "Four values" ) {
            NameValueList name_value_list{ "a=1,b=2,c=3,d=4", true };
            REQUIRE( name_value_list.size() == 4 );
            REQUIRE( name_value_list["a"] == "1" );
            REQUIRE( name_value_list["b"] == "2" );
            REQUIRE( name_value_list["c"] == "3" );
            REQUIRE( name_value_list["d"] == "4" );
        }
    }

    SECTION( "Multiple values with spaces" ) {
        NameValueList name_value_list;
        SECTION( "Spaces after comma" ) {
            name_value_list = { "a=1, b=2", true };
        }
        SECTION( "Spaces before comma" ) {
            name_value_list = { "a=1 ,b=2", true };
        }
        SECTION( "Spaces around comma" ) {
            name_value_list = { "a=1 , b=2", true };
        }
        SECTION( "Spaces after equals" ) {
            name_value_list = { "a= 1, b= 2", true };
        }
        SECTION( "Spaces before equals" ) {
            name_value_list = { "a =1, b =2", true };
        }
        SECTION( "Spaces around equals" ) {
            name_value_list = { "a = 1, b = 2", true };
        }
        SECTION( "Spaces everywhere" ) {
            name_value_list = { "a = 1 , b = 2", true };
        }

        REQUIRE( name_value_list.size() == 2 );
        REQUIRE( name_value_list["a"] == "1" );
        REQUIRE( name_value_list["b"] == "2" );
    }

    SECTION( "Value with double quotes" ) {
        NameValueList name_value_list{ "a='1'", true };
        REQUIRE( name_value_list.size() == 1 );
        REQUIRE( name_value_list["a"] == "1" );
    }

    SECTION( "Value with single quotes" ) {
        NameValueList name_value_list{ "a=\"1\"", true };
        REQUIRE( name_value_list.size() == 1 );
        REQUIRE( name_value_list["a"] == "1" );
    }

    SECTION( "Multiple values with quotes" ) {
        NameValueList name_value_list{ "a='1',b=\"2\"", true };
        REQUIRE( name_value_list.size() == 2 );
        REQUIRE( name_value_list["a"] == "1" );
        REQUIRE( name_value_list["b"] == "2" );
    }

    SECTION( "Name with quotes is stripped" ) {
        NameValueList name_value_list{ "'a'=1", true };
        REQUIRE( name_value_list.size() == 1 );
        REQUIRE( name_value_list["a"] == "1" );
    }

    SECTION( "Without strip quotes are preserved" ) {
        SECTION( "Name with single quotes" ) {
            NameValueList name_value_list{ "'a'=1", false };
            REQUIRE( name_value_list.size() == 1 );
            REQUIRE( name_value_list["'a'"] == "1" );
        }
        SECTION( "Name with double quotes" ) {
            NameValueList name_value_list{ "\"a\"=1", false };
            REQUIRE( name_value_list.size() == 1 );
            REQUIRE( name_value_list["\"a\""] == "1" );
        }
        SECTION( "Value with single quotes" ) {
            NameValueList name_value_list{ "a='1'", false };
            REQUIRE( name_value_list.size() == 1 );
            REQUIRE( name_value_list["a"] == "'1'" );
        }
        SECTION( "Value with double quotes" ) {
            NameValueList name_value_list{ "a=\"1\"", false };
            REQUIRE( name_value_list.size() == 1 );
            REQUIRE( name_value_list["a"] == "\"1\"" );
        }
    }

    SECTION( "Value with quotes with spaces" ) {
        SECTION( "Single quotes" ) {
            NameValueList name_value_list{ "a=\"1 2 3\"", true };
            REQUIRE( name_value_list.size() == 1 );
            REQUIRE( name_value_list["a"] == "1 2 3" );
        }
        SECTION( "Double quotes" ) {
            NameValueList name_value_list{ "a='1 2 3'", true };
            REQUIRE( name_value_list.size() == 1 );
            REQUIRE( name_value_list["a"] == "1 2 3" );
        }
    }

    SECTION( "Value with quoted comma" ) {
        SECTION( "Single quotes" ) {
            NameValueList name_value_list{ "a='1,2,3'", true };
            REQUIRE( name_value_list.size() == 1 );
            REQUIRE( name_value_list["a"] == "1,2,3" );
        }
        SECTION( "Double quotes" ) {
            NameValueList name_value_list{ "a=\"1,2,3\"", true };
            REQUIRE( name_value_list.size() == 1 );
            REQUIRE( name_value_list["a"] == "1,2,3" );
        }
    }

    SECTION( "Multiple values some with quoted commas" ) {
        SECTION( "Single quotes" ) {
            NameValueList name_value_list{ "a=1,b='1,2,3',c=3", true };
            REQUIRE( name_value_list.size() == 3 );
            REQUIRE( name_value_list["a"] == "1" );
            REQUIRE( name_value_list["b"] == "1,2,3" );
            REQUIRE( name_value_list["c"] == "3" );
        }
        SECTION( "Double quotes" ) {
            NameValueList name_value_list{ "a=1,b=\"1,2,3\",c=3", true };
            REQUIRE( name_value_list.size() == 3 );
            REQUIRE( name_value_list["a"] == "1" );
            REQUIRE( name_value_list["b"] == "1,2,3" );
            REQUIRE( name_value_list["c"] == "3" );
        }
    }

    SECTION( "Value ending with comma" ) {
        NameValueList name_value_list{ "a=1,b=2,", true };
        REQUIRE( name_value_list.size() == 2 );
        REQUIRE( name_value_list["a"] == "1" );
        REQUIRE( name_value_list["b"] == "2" );
    }

    SECTION( "Value ending with comma and space" ) {
        NameValueList name_value_list{ "a=1,b=2, ", true };
        REQUIRE( name_value_list.size() == 2 );
        REQUIRE( name_value_list["a"] == "1" );
        REQUIRE( name_value_list["b"] == "2" );
    }
}