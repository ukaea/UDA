#include <catch2/catch_test_macros.hpp>
#include <sstream>
// #include <catch2/catch_approx.hpp>
#include "logging/logging.h"
#include <thread>
#include <chrono>

using namespace uda::logging;

//TODO: 
// - adding file sink removes stdout sink
// - file sinks can only be appended to, not replaced?

//NOTE: need to call close_logging() to teardown each test due to global state

void reset_logging_if_required()
{
    if(logging_initialised())
    {
        close_logging();
    }
}

TEST_CASE( "logging initialisaiton status can be checked", "[init-logging]")
{
    reset_logging_if_required();
    REQUIRE_FALSE( logging_initialised() );
    init_logging();
    REQUIRE( logging_initialised() );
    close_logging();
    REQUIRE_FALSE( logging_initialised() );
}

TEST_CASE( "init can be called multiple times without error", "[init-logging]")
{
    reset_logging_if_required();
    REQUIRE_FALSE( logging_initialised() );
    init_logging();
    REQUIRE( logging_initialised() );
    init_logging();
    REQUIRE( logging_initialised() );
    init_logging();
    REQUIRE( logging_initialised() );
    close_logging();
    REQUIRE_FALSE( logging_initialised() );
}

TEST_CASE( "logging can be re-initialised after spdlog::shutdown", "[init-logging]")
{
    reset_logging_if_required();
    REQUIRE_FALSE( logging_initialised() );
    init_logging();
    REQUIRE( logging_initialised() );
    close_logging();
    REQUIRE_FALSE( logging_initialised() );

    init_logging();
    REQUIRE( logging_initialised() );
    std::this_thread::sleep_for(std::chrono::milliseconds(50));

    // some of these calls used to segfault after shutdown/reinit
    auto expected_result = LogLevel::UDA_LOG_DEBUG;
    set_log_level(expected_result);
    auto log_level = get_log_level();
    REQUIRE( log_level == expected_result);

    close_logging();
    REQUIRE_FALSE( logging_initialised() );
}

TEST_CASE( "log level can be set and queried", "[init-logging]")
{
    reset_logging_if_required();
    REQUIRE_FALSE( logging_initialised() );
    init_logging();
    REQUIRE( logging_initialised() );

    auto expected_result = LogLevel::UDA_LOG_DEBUG;
    set_log_level(expected_result);
    auto log_level = get_log_level();
    auto expected_level = spdlog::default_logger()->level();
    REQUIRE( spdlog::default_logger()->level() == spdlog::level::debug );
    REQUIRE( log_level == expected_result);

    expected_result = LogLevel::UDA_LOG_INFO;
    set_log_level(expected_result);
    log_level = get_log_level();
    expected_level = spdlog::default_logger()->level();
    REQUIRE( spdlog::default_logger()->level() == spdlog::level::info );
    REQUIRE( log_level == expected_result);

    expected_result = LogLevel::UDA_LOG_WARN;
    set_log_level(expected_result);
    log_level = get_log_level();
    expected_level = spdlog::default_logger()->level();
    REQUIRE( expected_level == spdlog::level::warn );
    REQUIRE( log_level == expected_result);

    expected_result = LogLevel::UDA_LOG_ERROR;
    set_log_level(expected_result);
    log_level = get_log_level();
    expected_level = spdlog::default_logger()->level();
    REQUIRE( expected_level == spdlog::level::err );
    REQUIRE( log_level == expected_result);

    expected_result = LogLevel::UDA_LOG_ACCESS;
    set_log_level(expected_result);
    log_level = get_log_level();
    expected_level = spdlog::default_logger()->level();
    REQUIRE( expected_level == spdlog::level::critical );
    REQUIRE( log_level == expected_result);

    expected_result = LogLevel::UDA_LOG_NONE;
    set_log_level(expected_result);
    log_level = get_log_level();
    expected_level = spdlog::default_logger()->level();
    REQUIRE( expected_level == spdlog::level::off );
    REQUIRE( log_level == expected_result);

    close_logging();
    REQUIRE_FALSE( logging_initialised() );
}

TEST_CASE( "log function doesn't segfault when logs are uninitialised", "[uda-log]")
{
    reset_logging_if_required();
    REQUIRE_FALSE( logging_initialised() );
    // REQUIRE_NOTHROW( UDA_LOG(LogLevel::UDA_LOG_ERROR, "A test message") );
    REQUIRE_NOTHROW( log(LogLevel::UDA_LOG_ERROR, __FILE__, __LINE__, "A test message") );
}

//TODO: this function needs the get_logger function which is private to logging.capture_log_output
// so has been moved there too. check this is the best way to do this
// void capture_log_output(LogLevel mode, std::ostream& oss)
// {
//     std::shared_ptr<spdlog::logger> logger = get_logger(mode);
//     if (!logger) {
//         return;
//     }
//
//     auto& sinks = logger->sinks();
//     sinks.clear();
//     sinks.push_back(std::make_shared<spdlog::sinks::ostream_sink_st>(oss));
// }

TEST_CASE( "No logging output generated when log level is UDA_LOG_NONE", "[uda-log]")
{
    reset_logging_if_required();
    REQUIRE_FALSE( logging_initialised() );
    init_logging();
    REQUIRE( logging_initialised() );

    set_log_level(LogLevel::UDA_LOG_NONE);
    std::ostringstream oss;
    capture_log_output(LogLevel::UDA_LOG_ERROR, oss);
    UDA_LOG(LogLevel::UDA_LOG_ERROR, "A test message");
    REQUIRE( oss.str().empty() );

    close_logging();
    REQUIRE_FALSE( logging_initialised() );
}

TEST_CASE( "Error logging output generated when log level is not UDA_LOG_NONE", "[uda-log]")
{
    reset_logging_if_required();
    REQUIRE_FALSE( logging_initialised() );
    init_logging();
    REQUIRE( logging_initialised() );

    set_log_level(LogLevel::UDA_LOG_DEBUG);
    std::ostringstream oss_err;
    capture_log_output(LogLevel::UDA_LOG_ERROR, oss_err);
    std::ostringstream oss_dbg;
    capture_log_output(LogLevel::UDA_LOG_DEBUG, oss_dbg);
    std::ostringstream oss_access;
    capture_log_output(LogLevel::UDA_LOG_ACCESS, oss_access);

    std::string test_string = "a test message";

    UDA_LOG(LogLevel::UDA_LOG_ERROR, test_string);
    std::string err_log = oss_err.str();
    REQUIRE_FALSE( err_log.empty() );
    REQUIRE(err_log.find(test_string) != std::string::npos);

    UDA_LOG(LogLevel::UDA_LOG_DEBUG, test_string);
    std::string dbg_log = oss_dbg.str();
    REQUIRE_FALSE( dbg_log.empty() );
    REQUIRE(dbg_log.find(test_string) != std::string::npos);

    close_logging();
    REQUIRE_FALSE( logging_initialised() );
}
