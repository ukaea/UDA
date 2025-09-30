// catch includes
#include <catch2/catch_test_macros.hpp>
#include <catch2/generators/catch_generators.hpp>

// std includes
#include <cstdlib>
#include <string>
#include <vector>

// uda includes
#include <common/uda_env_options.hpp>

namespace {
class EnvFixture
{
    public:
    EnvFixture(std::string_view name, std::string_view val)
    :name(name), value(val)
    {
       setenv(this->name.c_str(), value.c_str(), 1);
    }

    ~EnvFixture()
    {
        unsetenv(name.c_str());
    }

    std::string name;
    std::string value;
};
} // anon namespace

namespace uda_env = uda::common::env_options;

SCENARIO( "uda options stored in env vars can be interpreted", "[uda-env]" )
{
    GIVEN( "an existing environment variable" )
    {
        AND_GIVEN( "The variable is truthy" )
        {
            auto value = GENERATE("1", "true", "yes", "on", "ON", "TRUE", "YeS");
            std::string name = std::string("UDA_ENV_TEST_VAR_EXISTS_") + value;
            EnvFixture env(name, value);

            THEN( "check evaluates to true" )
            {
                REQUIRE( uda_env::evaluate_env_option(name.c_str()) );
            }
        }
        AND_GIVEN( "The variable is falsey" )
        {
            auto value = GENERATE("0", "false", "no", "off", "OFF","FALSE", "FaLSe");
            std::string name = std::string("UDA_ENV_TEST_VAR_EXISTS_") + value;
            EnvFixture env(name, value);

            THEN( "string match fails for list of truthy values" )
            {
                REQUIRE( !  uda_env::match_env_option(name.c_str(), uda_env::truthy_values) );
            }
            THEN( "string match passes for list of falsey values" )
            {
                REQUIRE(  uda_env::match_env_option(name.c_str(), uda_env::falsey_values) );
            }
            AND_THEN( "check evaluates to false" )
            {
                REQUIRE( ! uda_env::evaluate_env_option(name.c_str()) );
            }
        }
        AND_GIVEN( "The variable has an unexpected value" )
        {
            auto value = GENERATE("fake", "not_in_list", "TYPO");
            std::string name = std::string("UDA_ENV_TEST_VAR_EXISTS_") + value;
            EnvFixture env(name, value);

            THEN( "string match fails for lists of both truthy and falsey values" )
            {
                REQUIRE( ! uda_env::strings_match(name, uda_env::falsey_values) );
                REQUIRE( ! uda_env::strings_match(name, uda_env::truthy_values) );
                AND_THEN( "check evaluates false" )
                {
                    REQUIRE( ! uda_env::evaluate_env_option(name.c_str()) );
                }
            }
            AND_THEN( "check evaluates false" )
            {
                REQUIRE( ! uda_env::evaluate_env_option(name.c_str()) );
            }
        }
        AND_GIVEN( "A custom list of accepted values" )
        {
            std::vector<std::string> accepted_values {"ENUM1", "ENUM2", "ENUM3"};
            // auto value = GENERATE(values(accepted_values));
            // std::string name = std::string("UDA_ENV_TEST_VAR_EXISTS_") + value;
            // EnvFixture env(name, value);

            WHEN( "the value is in the accepted list")
            {
                auto value = GENERATE("ENUM1", "ENUM2", "ENUM3");
                std::string name = std::string("UDA_ENV_TEST_VAR2_EXISTS_") + value;
                EnvFixture env(name, value);
                THEN( "string match passes for list of accepted values" )
                {
                    REQUIRE( uda_env::match_env_option(name.c_str(), accepted_values) );
                }
            }
            WHEN( "the value is not in the accepted list")
            {
                auto value = GENERATE("fake", "not_in_list", "TYPO");
                std::string name = std::string("UDA_ENV_TEST_VAR2_EXISTS_") + value;
                EnvFixture env(name, value);
                THEN( "string match fails for list of accepted values" )
                {
                    REQUIRE( ! uda_env::match_env_option(name.c_str(), accepted_values) );
                }
            }
        }
    }

    GIVEN( "a non-existent environment variable" )
    {
        const char* name = "UDA_ENV_TEST_VAR_UNSET";
        unsetenv(name);
        WHEN( "A default value of true is used" )
        {
            bool default_value = true;
            THEN( "check evaluates true" )
            {
                REQUIRE( uda_env::evaluate_env_option(name, default_value) );
            }
        }
        WHEN( "A default value of false is used" )
        {
            bool default_value = false;
            THEN( "check evaluates false" )
            {
                REQUIRE( ! uda_env::evaluate_env_option(name, default_value) );
            }
        }

    }
}
