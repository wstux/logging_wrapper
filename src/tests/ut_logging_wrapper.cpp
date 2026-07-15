/*
 * logging_wrapper
 * Copyright (C) 2024  Chistyakov Alexander
 *
 * This program is free software: you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation, either version 3 of the License, or
 * (at your option) any later version.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program.  If not, see <http://www.gnu.org/licenses/>.
 */
/**
 *  \file
 *  \brief  Logging wrapper unit tests.
 *  \ingroup    logging_wrapper_tests
 */

#include <cstdarg>
#include <cstdio>
#include <iostream>
#include <limits>
#include <sstream>

#include <gtest/gtest.h>

#include "logging_wrapper/logging.h"

namespace {

/**
 *  \internal
 *  \brief  Mock logger for testing stream syntax (std::ostream style).
 *
 *  Accumulates all data passed via the output operator into an internal
 *  `std::stringstream` buffer.
 */
struct test_logger final
{
    test_logger(const std::string&) {}

    template <typename T>
    inline std::stringstream& operator<<(const T& val)
    {
        str_logger << val;
        return str_logger;
    }

    std::stringstream str_logger;
};

/**
 *  \internal
 *  \brief  Specialized mock logger that accepts additional arguments during
 *      construction.
 *
 *  Used to verify the correctness of argument passing into custom backends.
 */
struct test_specific_logger final
{
    test_specific_logger(const std::string&, const std::string&) {}

    template <typename T>
    inline std::stringstream& operator<<(const T& val)
    {
        str_logger << val;
        return str_logger;
    }

    std::stringstream str_logger;
};

/**
 *  \internal
 *  \brief  Mock logger for testing formatted syntax (printf style).
 *
 *  Writes logs to a fixed-size static buffer using `vsprintf`.
 */
struct test_loggerf final
{
    test_loggerf(const std::string&) {}

    void operator()(const char* p_fmt, ...)
    {
        va_list args;
        va_start(args, p_fmt);
        const int rc = vsprintf(buffer + cur_size, p_fmt, args);
        va_end(args);
        if (rc >= 0) {
            cur_size += rc;
        }
    }

    std::string str() const { return std::string(buffer, cur_size); }

    char buffer[1024];
    size_t cur_size = 0;
};

/**
 *  \internal
 *  \brief  Helper function to compare a captured log message against a
 *      reference template.
 *  \param  ethalon - reference string where the '*' character replaces dynamic
 *      data (e.g., a timestamp).
 *  \param  log - actual log string captured from the backend.
 *  \return true if the strings match (taking the '*' wildcard into account),
 *      otherwise false.
 */
bool is_equal_logs(const std::string& ethalon, const std::string& log)
{
    if (ethalon.size() != log.size()) {
        return false;
    }
    for (size_t i = 0; i < ethalon.size(); ++i) {
        if (ethalon[i] != '*' && ethalon[i] != log[i]) {
            return false;
        }
    }
    return true;
}

/**
 *  \internal
 *  \brief  Base test fixture for isolating logging tests.
 *
 *  Guarantees that before each test, the logging subsystem is initialized
 *  to its initial state, and after each test, resources are properly released.
 */
class logging_fixture : public ::testing::Test
{
public:
    /// \brief  Environment setup (Invoked before each test case).
    virtual void SetUp() override { ::wstux::logging::manager::init(); }
    /// \brief  Environment cleanup (Invoked after each test case).
    virtual void TearDown() override { ::wstux::logging::manager::deinit(); }
};

using logging_cpp = logging_fixture;
using loggingf = logging_fixture;
using logging = logging_fixture;

} // <anonymous> namespace

namespace wstux {
namespace logging {

template<> test_logger make_logger<test_logger>(const std::string& ch) { return test_logger(ch); }
template<> test_loggerf make_logger<test_loggerf>(const std::string& ch) { return test_loggerf(ch); }
template<> test_specific_logger make_logger<test_specific_logger>(const std::string& ch) { return test_specific_logger(ch, ch); }

} // namespace logging
} // namespace wstux

/**
 *  \test   Verification of the basic stream-style (ostream-style) logging mechanism.
 *  \see    LOG_ERROR, wstux::logging::manager::get_logger
 *
 *  **Test logic description:**
 *  The test verifies that when the `LOG_ERROR` stream macro is invoked, the
 *  message is successfully formatted, augmented with metadata (level, channel
 *  name, date), and reaches the target logger backend.
 *
 *  **Steps to reproduce:**
 *  -# Request a logger for the `"Root"` channel with the `test_logger` backend
 *      from the manager.
 *  -# Record an error using the macro: `LOG_ERROR(root_logger, "error log " << 42)`.
 *  -# Read the accumulated data from the mock backend and compare it against
 *      the reference template wildcard.
 *
 *  \expected_result    The log string must match the pattern
 *      `"****-**-** **:**:**.*** [ERROR] Root: error log 42\n"`, where the
 *      asterisks are replaced by the actual time. The `is_equal_logs` function
 *      must return `true`.
 */
TEST_F(logging_cpp, logging)
{
    using logger_t = ::wstux::logging::logger<test_logger>;

    logger_t root_logger = ::wstux::logging::manager::get_logger<logger_t>("Root");
    LOG_ERROR(root_logger, "error log " << 42);

    const std::string ethalon = "****-**-** **:**:**.*** [ERROR] Root: error log 42\n";
    const std::string log = root_logger.get_logger().str_logger.str();
    EXPECT_TRUE(is_equal_logs(ethalon, log)) << "'" << ethalon << "' != '" << log << "'";
}

/**
 *  \test   Verification of retrieving an existing logger from the manager again.
 *  \see    wstux::logging::manager::get_logger
 *
 *  **Test logic description:**
 *  The test verifies that the logging manager correctly caches loggers by their
 *  channel names. Invoking `get_logger` again for the same channel must return
 *  an instance that points to the exact same internal backend object.
 *
 *  **Steps to reproduce:**
 *  -# Create a logger named `"Root"` and write a single entry to it.
 *  -# Request the `"Root"` logger once more into a new variable and write a
 *      second entry.
 *  -# Extract the log buffer from the first instance and verify that it
 *      contains both entries.
 *
 *  \expected_result    Since both loggers share the same underlying backend,
 *      the first variable `root_logger` must contain two consecutive log
 *      entries in its buffer.
 */
TEST_F(logging_cpp, same_loggers)
{
    using logger_t = ::wstux::logging::logger<test_logger>;

    logger_t root_logger = ::wstux::logging::manager::get_logger<logger_t>("Root");
    LOG_ERROR(root_logger, "error log " << 42);

    logger_t logger = ::wstux::logging::manager::get_logger<logger_t>("Root");
    LOG_ERROR(logger, "error log " << 42);

    const std::string ethalon = "****-**-** **:**:**.*** [ERROR] Root: error log 42\n"
                                "****-**-** **:**:**.*** [ERROR] Root: error log 42\n";
    const std::string log = root_logger.get_logger().str_logger.str();
    EXPECT_TRUE(is_equal_logs(ethalon, log)) << "'" << ethalon << "' != '" << log << "'";
}

/**
 *  \test   Verification of macro behavior with custom loggers requiring
 *      extended initialization.
 *  \see    wstux::logging::make_logger
 *
 *  **Test logic description:**
 *  The test guarantees that the factory method `make_logger` and the logger
 *  wrapper function correctly with backends whose constructors require multiple
 *  arguments (in this case, `test_specific_logger`).
 *
 *  **Steps to reproduce:**
 *  -# Request a logger for the `"Root"` channel configured to compile under the
 *      `test_specific_logger` type.
 *  -# Execute a log entry using the `LOG_ERROR` macro.
 *  -# Verify that the log is successfully formatted and written to the buffer.
 *
 *  \expected_result    The logger is successfully created via the factory
 *      specialization, the message is formatted without errors, and it strictly
 *      matches the reference template.
 */
TEST_F(logging_cpp, logging_specific)
{
    using logger_t = ::wstux::logging::logger<test_specific_logger>;

    logger_t root_logger = ::wstux::logging::manager::get_logger<logger_t>("Root");
    LOG_ERROR(root_logger, "error log " << 42);

    const std::string ethalon = "****-**-** **:**:**.*** [ERROR] Root: error log 42\n";
    const std::string log = root_logger.get_logger().str_logger.str();
    EXPECT_TRUE(is_equal_logs(ethalon, log)) << "'" << ethalon << "' != '" << log << "'";
}

/**
 *  \test   Verification of dynamic global logging level modification (ostream-style).
 *  \see    wstux::logging::manager::set_global_level, LOG_ERROR, LOG_CRIT
 *
 *  **Test logic description:**
 *  The test verifies that changing the global filtering threshold at runtime
 *  correctly suppresses lower-priority messages and begins to allow them through
 *  once the threshold is lowered.
 *
 *  **Steps to reproduce:**
 *  -# Set the global level to `severity_level::crit`.
 *  -# Attempt to write logs with `ERROR` and `CRIT` levels. Verify the buffer.
 *  -# Lower the global level to `severity_level::info`.
 *  -# Write an `ERROR` level log entry again. Verify the buffer once more.
 *
 *  \expected_result    When the level is set to `CRIT`, the `ERROR` message is
 *      filtered out (leaving only one entry in the buffer). After shifting the
 *      level to `INFO`, the new `ERROR` message successfully passes the filter
 *      and is appended to the buffer.
 */
TEST_F(logging_cpp, severity_level)
{
    using logger_t = ::wstux::logging::logger<test_logger>;

    ::wstux::logging::manager::set_global_level(::wstux::logging::severity_level::crit);
    logger_t root_logger = ::wstux::logging::manager::get_logger<logger_t>("Root");
    LOG_ERROR(root_logger, "error log " << 42);
    LOG_CRIT(root_logger, "crit log " << 42);

    std::string ethalon = "****-**-** **:**:**.*** [CRIT ] Root: crit log 42\n";
    std::string log = root_logger.get_logger().str_logger.str();
    EXPECT_TRUE(is_equal_logs(ethalon, log)) << "'" << ethalon << "' != '" << log << "'";

    ::wstux::logging::manager::set_global_level(::wstux::logging::severity_level::info);
    LOG_ERROR(root_logger, "error log " << 42);
    ethalon = "****-**-** **:**:**.*** [CRIT ] Root: crit log 42\n"
              "****-**-** **:**:**.*** [ERROR] Root: error log 42\n";
    log = root_logger.get_logger().str_logger.str();
    EXPECT_TRUE(is_equal_logs(ethalon, log)) << "'" << ethalon << "' != '" << log << "'";
}

/**
 *  \test   Resilience to invalid global logging level values.
 *  \see    wstux::logging::manager::set_global_level, wstux::logging::manager::global_level
 *
 *  **Test logic description:**
 *  The test verifies the system's behavior when an incorrect or invalid integer
 *  value is passed instead of the strongly-typed `severity_level` enum. The
 *  system must protect its state and ignore the invalid level change request.
 *
 *  **Steps to reproduce:**
 *  -# Set a valid level (`severity_level::debug`) and verify its application.
 *  -# Invoke the overloaded method `set_global_level(int lvl)` with an
 *      intentionally invalid number `42`.
 *  -# Read the current global level.
 *
 *  \expected_result    The logging manager ignores the invalid `int` parameter,
 *      preserving the previous correct state (`severity_level::debug`).
 */
TEST_F(logging_cpp, invalid_severity_level)
{
    ::wstux::logging::manager::set_global_level(::wstux::logging::severity_level::debug);
    EXPECT_TRUE(::wstux::logging::manager::global_level() == ::wstux::logging::severity_level::debug)
        << "Current global level: " << ::wstux::logging::manager::global_level();

    ::wstux::logging::manager::set_global_level(42);
    EXPECT_TRUE(::wstux::logging::manager::global_level() == ::wstux::logging::severity_level::debug)
        << "Current global level: " << ::wstux::logging::manager::global_level();
}

/**
 *  \test   Resilience to invalid severity levels for specific channels.
 *  \see    wstux::logging::manager::set_logger_level, wstux::logging::logger::can_log
 *
 *  **Test logic description:**
 *  The test verifies the behavior of the `set_logger_level` function when
 *  attempting to pass invalid level boundaries (e.g., `42` or `-42`) for a
 *  specific log channel.
 *
 *  **Steps to reproduce:**
 *  -# Set the global level to `debug` and the level for the `"Root"` channel
 *      to `info`.
 *  -# Verify that the logger permits writing an `INFO` level entry.
 *  -# Attempt to set the level of the `"Root"` channel to `42` and verify the
 *      `can_log` method.
 *  -# Attempt to set the level of the `"Root"` channel to `-42` and verify the
 *      `can_log` method.
 *
 *  \expected_result    Attempts to pass invalid level values to a channel must
 *      not corrupt or reset the filtering configurations. The logger preserves
 *      its capability to correctly process the `INFO` level.
 */
TEST_F(logging_cpp, invalid_logger_severity_level)
{
    using logger_t = ::wstux::logging::logger<test_logger>;

    ::wstux::logging::manager::set_global_level(::wstux::logging::severity_level::debug);
    ::wstux::logging::manager::set_logger_level("Root", ::wstux::logging::severity_level::info);

    logger_t root_logger = ::wstux::logging::manager::get_logger<logger_t>("Root");
    EXPECT_TRUE(root_logger.can_log(::wstux::logging::severity_level::info));

    ::wstux::logging::manager::set_logger_level("Root", ::wstux::logging::severity_level(42));
    EXPECT_TRUE(root_logger.can_log(::wstux::logging::severity_level::info));

    ::wstux::logging::manager::set_logger_level("Root", ::wstux::logging::severity_level(-42));
    EXPECT_TRUE(root_logger.can_log(::wstux::logging::severity_level::info));
}

/**
 *  \test   Verification of independent message filtering across multiple channels.
 *  \see    wstux::logging::manager::set_logger_level, LOG_INFO, LOG_ERROR, LOG_CRIT
 *
 *  **Test logic description:**
 *  Verifies channel isolation. The filtering configurations of one string-based
 *  channel (e.g., `"Root"`) must not affect the behavior or filtering thresholds
 *  of another channel (`"Channel"`).
 *
 *  **Steps to reproduce:**
 *  -# Create two independent loggers: `"Root"` and `"Channel"`.
 *  -# Set the level for `"Root"` to `INFO` and the level for `"Channel"` to `ERROR`.
 *  -# Send a batch of `INFO` and `ERROR` messages to both loggers.
 *  -# Change the levels of both channels to `CRIT` and send `ERROR` and `CRIT` messages.
 *  -# Verify the buffers of both loggers against their individual reference templates.
 *
 *  \expected_result    Each logger filters out only those messages that fail
 *      its personal severity threshold. The `"Root"` buffer contains an `INFO`
 *      entry, whereas the `"Channel"` buffer does not.
 */
TEST_F(logging_cpp, channels)
{
    using logger_t = ::wstux::logging::logger<test_logger>;

    logger_t root_logger = ::wstux::logging::manager::get_logger<logger_t>("Root");
    logger_t chan_logger = ::wstux::logging::manager::get_logger<logger_t>("Channel");
    ::wstux::logging::manager::set_global_level(::wstux::logging::severity_level::debug);

    ::wstux::logging::manager::set_logger_level("Root", ::wstux::logging::severity_level::info);
    ::wstux::logging::manager::set_logger_level("Channel", ::wstux::logging::severity_level::error);
    LOG_INFO(root_logger, "info log " << 42);
    LOG_INFO(chan_logger, "info log " << 42);
    LOG_ERROR(root_logger, "error log " << 42);
    LOG_ERROR(chan_logger, "error log " << 42);

    ::wstux::logging::manager::set_logger_level("Root", ::wstux::logging::severity_level::crit);
    ::wstux::logging::manager::set_logger_level("Channel", ::wstux::logging::severity_level::crit);
    LOG_ERROR(root_logger, "error log " << 42);
    LOG_ERROR(chan_logger, "error log " << 42);
    LOG_CRIT(root_logger, "crit log " << 42);
    LOG_CRIT(chan_logger, "crit log " << 42);

    const std::string ethalon_root = "****-**-** **:**:**.*** [INFO ] Root: info log 42\n"
                                     "****-**-** **:**:**.*** [ERROR] Root: error log 42\n"
                                     "****-**-** **:**:**.*** [CRIT ] Root: crit log 42\n";
    const std::string ethalon_chan = "****-**-** **:**:**.*** [ERROR] Channel: error log 42\n"
                                     "****-**-** **:**:**.*** [CRIT ] Channel: crit log 42\n";
    const std::string log_root = root_logger.get_logger().str_logger.str();
    const std::string log_chan = chan_logger.get_logger().str_logger.str();
    EXPECT_TRUE(is_equal_logs(ethalon_root, log_root)) << "'" << ethalon_root << "' != '" << log_root << "'";
    EXPECT_TRUE(is_equal_logs(ethalon_chan, log_chan)) << "'" << ethalon_chan << "' != '" << log_chan << "'";
}

/**
 *  \test   Verification of the basic formatted (printf-style) logging mechanism.
 *  \see    LOGF_ERROR, wstux::logging::manager::get_logger
 *
 *  **Test logic description:**
 *  The test verifies that when the `LOG_ERROR` stream macro is invoked, the
 *  message is successfully formatted, augmented with metadata (level, channel
 *  name, date), and reaches the target logger backend.
 *
 *  **Steps to reproduce:**
 *  -# Request a logger for the `"Root"` channel with the `test_loggerf` backend
 *      from the manager.
 *  -# Record an error using the macro: `LOGF_ERROR(root_logger, "error log, %d", 42)`.
 *  -# Read the accumulated data from the mock backend and compare it against
 *      the reference template wildcard.
 *
 *  \expected_result    The log string must match the pattern
 *      `"****-**-** **:**:**.*** [ERROR] Root: error log 42\n"`, where the
 *      asterisks are replaced by the actual time. The `is_equal_logs` function
 *      must return `true`.
 */
TEST_F(loggingf, logging)
{
    using logger_t = ::wstux::logging::logger<test_loggerf>;

    logger_t root_logger = ::wstux::logging::manager::get_logger<logger_t>("Root");
    LOGF_ERROR(root_logger, "error log, %d", 42);

    const std::string ethalon = "****-**-** **:**:**.*** [ERROR] Root: error log, 42\n";
    const std::string log = root_logger.get_logger().str();
    EXPECT_TRUE(is_equal_logs(ethalon, log)) << "'" << ethalon << "' != '" << log << "'";
}

/**
 *  \test   Verification of dynamic global logging level modification (printf-style).
 *  \see    wstux::logging::manager::set_global_level, LOGF_ERROR, LOGF_CRIT
 *
 *  **Test logic description:**
 *  The test verifies that changing the global filtering threshold at runtime
 *  correctly suppresses lower-priority messages and begins to allow them through
 *  once the threshold is lowered.
 *
 *  **Steps to reproduce:**
 *  -# Set the global level to `severity_level::crit`.
 *  -# Attempt to write logs with `ERROR` and `CRIT` levels. Verify the buffer.
 *  -# Lower the global level to `severity_level::info`.
 *  -# Write an `ERROR` level log entry again. Verify the buffer once more.
 *
 *  \expected_result    When the level is set to `CRIT`, the `ERROR` message is
 *      filtered out (leaving only one entry in the buffer). After shifting the
 *      level to `INFO`, the new `ERROR` message successfully passes the filter
 *      and is appended to the buffer.
 */
TEST_F(loggingf, severity_level)
{
    using logger_t = ::wstux::logging::logger<test_loggerf>;

    ::wstux::logging::manager::set_global_level(::wstux::logging::severity_level::crit);
    logger_t root_logger = ::wstux::logging::manager::get_logger<logger_t>("Root");
    LOGF_ERROR(root_logger, "error log %d", 42);
    LOGF_CRIT(root_logger, "crit log %d", 42);

    std::string ethalon = "****-**-** **:**:**.*** [CRIT ] Root: crit log 42\n";
    std::string log = root_logger.get_logger().str();
    EXPECT_TRUE(is_equal_logs(ethalon, log)) << "'" << ethalon << "' != '" << log << "'";

    ::wstux::logging::manager::set_global_level(::wstux::logging::severity_level::info);
    LOGF_ERROR(root_logger, "error log %d", 42);
    ethalon = "****-**-** **:**:**.*** [CRIT ] Root: crit log 42\n"
              "****-**-** **:**:**.*** [ERROR] Root: error log 42\n";
    log = root_logger.get_logger().str();
    EXPECT_TRUE(is_equal_logs(ethalon, log)) << "'" << ethalon << "' != '" << log << "'";
}

/**
 *  \test   Verification of independent message filtering across multiple channels.
 *  \see    wstux::logging::manager::set_logger_level, LOG_INFO, LOG_ERROR, LOG_CRIT
 *
 *  **Test logic description:**
 *  Verifies channel isolation. The filtering configurations of one string-based
 *  channel (e.g., `"Root"`) must not affect the behavior or filtering thresholds
 *  of another channel (`"Channel"`).
 *
 *  **Steps to reproduce:**
 *  -# Create two independent loggers: `"Root"` and `"Channel"`.
 *  -# Set the level for `"Root"` to `INFO` and the level for `"Channel"` to `ERROR`.
 *  -# Send a batch of `INFO` and `ERROR` messages to both loggers.
 *  -# Change the levels of both channels to `CRIT` and send `ERROR` and `CRIT` messages.
 *  -# Verify the buffers of both loggers against their individual reference templates.
 *
 *  \expected_result    Each logger filters out only those messages that fail
 *      its personal severity threshold. The `"Root"` buffer contains an `INFO`
 *      entry, whereas the `"Channel"` buffer does not.
 */
TEST_F(loggingf, channels)
{
    using logger_t = ::wstux::logging::logger<test_loggerf>;

    logger_t root_logger = ::wstux::logging::manager::get_logger<logger_t>("Root");
    logger_t chan_logger = ::wstux::logging::manager::get_logger<logger_t>("Channel");
    ::wstux::logging::manager::set_global_level(::wstux::logging::severity_level::debug);

    ::wstux::logging::manager::set_logger_level("Root", ::wstux::logging::severity_level::info);
    ::wstux::logging::manager::set_logger_level("Channel", ::wstux::logging::severity_level::error);
    LOGF_INFO(root_logger, "info log %d", 42);
    LOGF_INFO(chan_logger, "info log %d", 42);
    LOGF_ERROR(root_logger, "error log %d", 42);
    LOGF_ERROR(chan_logger, "error log %d", 42);

    ::wstux::logging::manager::set_logger_level("Root", ::wstux::logging::severity_level::crit);
    ::wstux::logging::manager::set_logger_level("Channel", ::wstux::logging::severity_level::crit);
    LOGF_ERROR(root_logger, "error log %d", 42);
    LOGF_ERROR(chan_logger, "error log %d", 42);
    LOGF_CRIT(root_logger, "crit log %d", 42);
    LOGF_CRIT(chan_logger, "crit log %d", 42);

    const std::string ethalon_root = "****-**-** **:**:**.*** [INFO ] Root: info log 42\n"
                                     "****-**-** **:**:**.*** [ERROR] Root: error log 42\n"
                                     "****-**-** **:**:**.*** [CRIT ] Root: crit log 42\n";
    const std::string ethalon_chan = "****-**-** **:**:**.*** [ERROR] Channel: error log 42\n"
                                     "****-**-** **:**:**.*** [CRIT ] Channel: crit log 42\n";
    const std::string log_root = root_logger.get_logger().str();
    const std::string log_chan = chan_logger.get_logger().str();
    EXPECT_TRUE(is_equal_logs(ethalon_root, log_root)) << "'" << ethalon_root << "' != '" << log_root << "'";
    EXPECT_TRUE(is_equal_logs(ethalon_chan, log_chan)) << "'" << ethalon_chan << "' != '" << log_chan << "'";
}

/**
 *  \test   Mixed simultaneous operation of different logger types (combo test).
 *  \see    LOG_INFO, LOGF_INFO, wstux::logging::manager::set_logger_level
 *
 *  **Test logic description:**
 *  The test verifies a heterogeneous logging environment. The manager must
 *  simultaneously and correctly manage channels that utilize different backend
 *  types: `test_logger` (stream-style) and `test_loggerf` (formatted printf-style),
 *  preserving independent filtering for each.
 *
 *  **Steps to reproduce:**
 *  -# Create a logger named `"Root"` (stream-style) and a logger named
 *      `"Channel"` (formatted printf-style).
 *  -# Configure individual filtering thresholds: `INFO` for `"Root"` and
 *      `ERROR` for `"Channel"`.
 *  -# Send several messages of both styles (`LOG_*` and `LOGF_*`) to their
 *      respective loggers.
 *  -# Change the levels of both channels to `CRIT` and send several messages
 *      (`ERROR` and `CRIT`).
 *  -# Verify the buffers of both loggers against their individual reference templates.
 *
 *  \expected_result    Each logger, regardless of its internal implementation
 *      (streams or printf), correctly filters messages according to its own
 *      rules. The macro invocations do not conflict with each other. The
 *      `log_root` and `log_chan` buffers are fully equivalent to their
 *      reference wildcard templates.
 */
TEST_F(logging, combo_loggers)
{
    using logger_t = ::wstux::logging::logger<test_logger>;
    using loggerf_t = ::wstux::logging::logger<test_loggerf>;

    logger_t root_logger = ::wstux::logging::manager::get_logger<logger_t>("Root");
    loggerf_t chan_logger = ::wstux::logging::manager::get_logger<loggerf_t>("Channel");
    ::wstux::logging::manager::set_global_level(::wstux::logging::severity_level::debug);

    ::wstux::logging::manager::set_logger_level("Root", ::wstux::logging::severity_level::info);
    ::wstux::logging::manager::set_logger_level("Channel", ::wstux::logging::severity_level::error);
    LOG_INFO(root_logger, "info log " << 42);
    LOGF_INFO(chan_logger, "info log %d", 42);
    LOG_ERROR(root_logger, "error log " << 42);
    LOGF_ERROR(chan_logger, "error log %d", 42);

    ::wstux::logging::manager::set_logger_level("Root", ::wstux::logging::severity_level::crit);
    ::wstux::logging::manager::set_logger_level("Channel", ::wstux::logging::severity_level::crit);
    LOG_ERROR(root_logger, "error log " << 42);
    LOGF_ERROR(chan_logger, "error log %d", 42);
    LOG_CRIT(root_logger, "crit log " << 42);
    LOGF_CRIT(chan_logger, "crit log %d", 42);

    const std::string ethalon_root = "****-**-** **:**:**.*** [INFO ] Root: info log 42\n"
                                     "****-**-** **:**:**.*** [ERROR] Root: error log 42\n"
                                     "****-**-** **:**:**.*** [CRIT ] Root: crit log 42\n";
    const std::string ethalon_chan = "****-**-** **:**:**.*** [ERROR] Channel: error log 42\n"
                                     "****-**-** **:**:**.*** [CRIT ] Channel: crit log 42\n";
    const std::string log_root = root_logger.get_logger().str_logger.str();
    const std::string log_chan = chan_logger.get_logger().str();
    EXPECT_TRUE(is_equal_logs(ethalon_root, log_root)) << "'" << ethalon_root << "' != '" << log_root << "'";
    EXPECT_TRUE(is_equal_logs(ethalon_chan, log_chan)) << "'" << ethalon_chan << "' != '" << log_chan << "'";
}

/**
 *  \test   Verification of the global logging level immutability (lock) mechanism.
 *  \see    wstux::logging::manager::set_immutable_global_level, wstux::logging::manager::set_global_level
 *
 *  **Test logic description:**
 *  Verifies the state "freezing" logic. The `set_immutable_global_level` method
 *  must permanently lock the global logging level once and block any subsequent
 *  runtime changes (both via `set_global_level` and via repeated invocations of
 *  `set_immutable_global_level` itself).
 *
 *  **Steps to reproduce:**
 *  -# Set the global level to `DEBUG` and verify that the `"Root"` channel
 *      permits `INFO` entries.
 *  -# Freeze the global level to `WARNING` using `set_immutable_global_level`.
 *  -# Attempt to modify the level to `CRIT` via regular `set_global_level()`.
 *      Verify that the level remains `WARNING`.
 *  -# Attempt to repeatedly overwrite the immutable level to `DEBUG` via
 *      `set_immutable_global_level`.
 *
 *  \expected_result    After the first invocation of `set_immutable_global_level`,
 *      the state is frozen at the `WARNING` value. No subsequent calls to level
 *      modification methods (including repeated locking attempts) can alter this
 *      value. The protection mechanism functions correctly.
 */
TEST_F(logging, immutable)
{
    using logger_t = ::wstux::logging::logger<test_logger>;

    logger_t root_logger = ::wstux::logging::manager::get_logger<logger_t>("Root");

    ::wstux::logging::manager::set_global_level(::wstux::logging::severity_level::debug);
    ::wstux::logging::manager::set_logger_level("Root", ::wstux::logging::severity_level::info);
    EXPECT_TRUE(::wstux::logging::manager::global_level() == ::wstux::logging::severity_level::debug);
    EXPECT_TRUE(root_logger.can_log(::wstux::logging::severity_level::info));

    ::wstux::logging::manager::set_immutable_global_level(::wstux::logging::severity_level::warning);

    ::wstux::logging::manager::set_global_level(::wstux::logging::severity_level::crit);
    ::wstux::logging::manager::set_logger_level("Root", ::wstux::logging::severity_level::crit);
    EXPECT_TRUE(::wstux::logging::manager::global_level() == ::wstux::logging::severity_level::warning);
    EXPECT_TRUE(root_logger.can_log(::wstux::logging::severity_level::crit));

    ::wstux::logging::manager::set_immutable_global_level(::wstux::logging::severity_level::debug);
    EXPECT_TRUE(::wstux::logging::manager::global_level() == ::wstux::logging::severity_level::warning);
}

/**
 *  \test   Verification of logger creation with an explicit default level (printf-style).
 *  \see    wstux::logging::manager::get_logger_dfl, LOGF_ERROR, LOGF_CRIT
 *
 *  **Test logic description:**
 *  The test verifies the functionality of the `get_logger_dfl` method, which
 *  allows setting an individual filtering threshold for a newly created channel
 *  exactly at the moment of its initialization, overriding the current global
 *  level.
 *
 *  **Steps to reproduce:**
 *  -# Set the general global level to `DEBUG`.
 *  -# Initialize a new channel named `"Root"` via the `get_logger_dfl` method,
 *      explicitly passing the `CRIT` level to it.
 *  -# Execute log entries for `ERROR` and `CRIT` levels using printf-style macros.
 *  -# Verify the buffer against the expected wildcard template.
 *
 *  \expected_result    The `"Root"` channel is successfully created with a local
 *      restriction of `CRIT`. The `ERROR` level message is filtered out locally
 *      by the logger, even though the global `DEBUG` level would otherwise
 *      permit it. Only a single line (`CRIT`) is retained in the buffer.
 */
TEST_F(loggingf, default_severity_level)
{
    using logger_t = ::wstux::logging::logger<test_loggerf>;

    ::wstux::logging::manager::set_global_level(::wstux::logging::severity_level::debug);
    logger_t root_logger = ::wstux::logging::manager::get_logger_dfl<logger_t>("Root", ::wstux::logging::severity_level::crit);
    LOGF_ERROR(root_logger, "error log %d", 42);
    LOGF_CRIT(root_logger, "crit log %d", 42);

    std::string ethalon = "****-**-** **:**:**.*** [CRIT ] Root: crit log 42\n";
    std::string log = root_logger.get_logger().str();
    EXPECT_TRUE(is_equal_logs(ethalon, log)) << "'" << ethalon << "' != '" << log << "'";
}

/**
 *  \internal
 *  \brief  Main function.
 */
int main(int argc, char** argv)
{
    testing::InitGoogleTest(&argc, argv);
    return RUN_ALL_TESTS();
}
