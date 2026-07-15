/*
 * logging_wrapper
 * Copyright (C) 2025  Chistyakov Alexander
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
 *  \brief  Loggingf wrapper unit tests.
 *  \ingroup    loggingf_wrapper_tests
 */

#include <cstdarg>
#include <cstdio>
#include <iostream>
#include <limits>
#include <sstream>

#include <gtest/gtest.h>

#include "loggingf_wrapper/logging.h"

namespace {

/**
 *  \internal
 *  \brief  Global mock logger for intercepting C-style logging calls.
 *
 *  Accumulates the formatting results of the `log_fn` function into a fixed
 *  character buffer for subsequent verification in tests.
 */
struct test_loggerf final
{
    /// \brief  Reset the buffer state.
    /// \details    Invoked in the fixture's `TearDown` to clear log messages between tests.
    void clear() { cur_size = 0; }

    /// \brief  Get the accumulated log messages.
    /// \return The current contents of the buffer as a `std::string`.
    std::string str() const { return std::string(buffer, cur_size); }

    char buffer[1024];   ///< Static buffer for raw text data
    size_t cur_size = 0; ///< Current number of written bytes
};

test_loggerf g_test_loggerf; ///< Global mock logger instance for integration with the C callback

/**
 *  \internal
 *  \brief  Custom logging function (C callback).
 *
 *  Matches the signature expected by the `lw_init_logging` initialization function.
 *  Redirects the formatted output to the internal `g_test_loggerf` buffer.
 */
int log_fn(const char* p_fmt, ...)
{
    va_list args;
    va_start(args, p_fmt);
    const int rc = vsprintf(g_test_loggerf.buffer + g_test_loggerf.cur_size, p_fmt, args);
    va_end(args);
    if (rc >= 0) {
        g_test_loggerf.cur_size += rc;
    }
    return rc;
}

/**
 *  \internal
 *  \brief  Helper function to compare a captured log message against a
 *      reference template.
 *  \param  ethalon - reference string where the '*' character replaces dynamic
 *      data (e.g., a timestamp).
 *  \param  log - actual log string captured from the buffer.
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
 *  \brief  Test fixture for verifying the lightweight C-style logging API.
 *
 *  Isolates logging backend tests. Clears the global interception buffer
 *  and forces the deinitialization of the C subsystem after each test case.
 */
class logging_fixture : public ::testing::Test
{
public:
    virtual void SetUp() override {}

    /// \brief  Environment cleanup (Invoked after each test case).
    virtual void TearDown() override { g_test_loggerf.clear(); lw_deinit_logging(); }
};

using loggingf = logging_fixture;

} // <anonymous> namespace

/**
 *  \test   Verification of basic logging via the C-style API using a fixed policy.
 *  \see    lw_init_logging, lw_get_logger, LOGF_ERROR
 *
 *  **Test logic description:**
 *  The test verifies that when the subsystem is initialized with the fixed-size
 *  (`fixed_size`) policy, the logger is successfully created and the `LOGF_ERROR`
 *  formatting macro correctly routes the call through the C interface into the
 *  registered callback.
 *
 *  **Steps to reproduce:**
 *  -# Initialize the subsystem via `lw_init_logging` with the `fixed_size`
 *      policy and `DEBUG` level.
 *  -# Request a pointer to the logger for the `"Root"` channel. Verify that it
 *      is valid.
 *  -# Send an error entry using the formatted macro `LOGF_ERROR`.
 *  -# Compare the result intercepted in `g_test_loggerf` against the expected
 *      wildcard template string.
 *
 *  \expected_result    Initialization and logger retrieval are completed
 *      successfully. The log line is correctly formatted by the callback,
 *      properly containing the channel metadata, the `ERROR` level, and the
 *      evaluated argument value `42`.
 */
TEST_F(loggingf, logging)
{
    EXPECT_TRUE(lw_init_logging(log_fn, lw_logging_policy_t::fixed_size, 1, lw_severity_level_t::debug, NULL));
    lw_loggerf_t root_logger = lw_get_logger("Root");
    ASSERT_TRUE(root_logger != nullptr);
    LOGF_ERROR(root_logger, "error log, %d", 42);

    const std::string ethalon = "****-**-** **:**:**.*** [ERROR] Root: error log, 42\n";
    const std::string log = g_test_loggerf.str();
    EXPECT_TRUE(is_equal_logs(ethalon, log)) << "'" << ethalon << "' != '" << log << "'";
}

/**
 *  \test   Validation of boundary and invalid initialization parameters for the
 *      C-interface.
 *  \see    lw_init_logging
 *
 *  **Test logic description:**
 *  The test verifies the defensive behavior of the initialization function when
 *  an intentionally invalid numerical size parameter (a value of `0`) is provided.
 *  The system must reject this configuration at startup.
 *
 *  **Steps to reproduce:**
 *  -# Invoke the `lw_init_logging` function, passing a size argument equal to `0`.
 *  -# Verify the boolean value returned by the function.
 *
 *  \expected_result    The `lw_init_logging` function recognizes the incorrect
 *      configuration parameter, aborts execution, and returns a failure
 *      indicator (`false`), safeguarding the system from an invalid state.
 */
TEST_F(loggingf, invalid_initialization_params)
{
    EXPECT_FALSE(lw_init_logging(log_fn, lw_logging_policy_t::fixed_size, 0, lw_severity_level_t::debug, NULL));
}

/**
 *  \test   Verification of basic logging via the C-style API using a dynamic policy.
 *  \see    lw_init_logging, lw_get_logger, LOGF_ERROR
 *
 *  **Test logic description:**
 *  The test verifies the correct behavior of the C-interface under an alternative
 *  memory allocation policy — dynamic resizing (`dynamic_size`). Formatting and
 *  message delivery must function identically to the standard mode.
 *
 *  **Steps to reproduce:**
 *  -# Initialize the logging subsystem, specifying the `dynamic_size` policy.
 *  -# Request the `"Root"` logger and send an `ERROR` level entry with an
 *      argument to it.
 *  -# Read the global mock buffer and compare it against the wildcard mask.
 *
 *  \expected_result    Shifting the internal memory allocation policy to
 *      `dynamic_size` does not impact the final syntax or the formatting success
 *      of the macros. The log is successfully delivered to the backend.
 */
TEST_F(loggingf, logging_dynamic)
{
    EXPECT_TRUE(lw_init_logging(log_fn, lw_logging_policy_t::dynamic_size, 1, lw_severity_level_t::debug, NULL));
    lw_loggerf_t root_logger = lw_get_logger("Root");
    ASSERT_TRUE(root_logger != nullptr);
    LOGF_ERROR(root_logger, "error log, %d", 42);

    const std::string ethalon = "****-**-** **:**:**.*** [ERROR] Root: error log, 42\n";
    const std::string log = g_test_loggerf.str();
    EXPECT_TRUE(is_equal_logs(ethalon, log)) << "'" << ethalon << "' != '" << log << "'";
    lw_deinit_logging();
}

/**
 *  \test   Verification of dynamic global level modification within the C-style API.
 *  \see    lw_init_logging, lw_set_global_level, LOGF_ERROR, LOGF_CRIT
 *
 *  **Test logic description:**
 *  Verifies the runtime severity level filtering functionality for the C-interface.
 *  The `lw_set_global_level` function must dynamically reconfigure the `LOGF_*`
 *  macro filter, discarding messages below the specified threshold.
 *
 *  **Steps to reproduce:**
 *  -# Start the subsystem with a strict restriction set to `CRIT`.
 *  -# Execute consecutive log entries for `ERROR` and `CRIT` messages. Verify
 *      that `ERROR` is discarded.
 *  -# Modify the global threshold to `INFO` via the `lw_set_global_level` function.
 *  -# Send an `ERROR` level log entry again and verify the final state of the
 *      mock buffer.
 *
 *  \expected_result    Initially, only the `CRIT` log is recorded. Following the
 *      invocation of `lw_set_global_level(info)`, the filtering threshold is
 *      successfully lowered, and the subsequent `ERROR` level message is written
 *      to the buffer without restriction.
 */
TEST_F(loggingf, severity_level)
{
    EXPECT_TRUE(lw_init_logging(log_fn, lw_logging_policy_t::fixed_size, 1, lw_severity_level_t::crit, NULL));
    lw_loggerf_t root_logger = lw_get_logger("Root");
    ASSERT_TRUE(root_logger != nullptr);
    LOGF_ERROR(root_logger, "error log %d", 42);
    LOGF_CRIT(root_logger, "crit log %d", 42);

    std::string ethalon = "****-**-** **:**:**.*** [CRIT ] Root: crit log 42\n";
    std::string log = g_test_loggerf.str();
    EXPECT_TRUE(is_equal_logs(ethalon, log)) << "'" << ethalon << "' != '" << log << "'";

    lw_set_global_level(lw_severity_level_t::info);
    LOGF_ERROR(root_logger, "error log %d", 42);
    ethalon = "****-**-** **:**:**.*** [CRIT ] Root: crit log 42\n"
              "****-**-** **:**:**.*** [ERROR] Root: error log 42\n";
    log = g_test_loggerf.str();
    EXPECT_TRUE(is_equal_logs(ethalon, log)) << "'" << ethalon << "' != '" << log << "'";
}

/**
 *  \test   Resilience of the C-style API to invalid global level values.
 *  \see    lw_set_global_level, lw_global_level, lw_can_log
 *
 *  **Test logic description:**
 *  The test verifies the defensive behavior of the C-interface when attempting
 *  to pass invalid severity level boundaries (values `42` and `-42`) as a global
 *  filter. The system must ignore the incorrect configuration and preserve its
 *  last valid state.
 *
 *  **Steps to reproduce:**
 *  -# Initialize the subsystem and switch the global level to `DEBUG`.
 *  -# Attempt to set the invalid level `42`. Verify the current level and the
 *      `lw_can_log(42)` method.
 *  -# Attempt to set the invalid level `-42`. Verify the level and the
 *      `lw_can_log(-42)` method.
 *
 *  \expected_result    The logging subsystem ignores invalid filtering parameters.
 *      The global level remains equal to `DEBUG`, and the helper verification
 *      predicates return `false`.
 */
TEST_F(loggingf, invalid_severity_level)
{
    EXPECT_TRUE(lw_init_logging(log_fn, lw_logging_policy_t::fixed_size, 1, lw_severity_level_t::crit, NULL));

    lw_set_global_level(lw_severity_level_t::debug);
    EXPECT_TRUE(lw_global_level() == lw_severity_level_t::debug)
        << "Current global level: " << lw_global_level();

    lw_set_global_level(lw_severity_level_t(42));
    EXPECT_TRUE(lw_global_level() == lw_severity_level_t::debug) << "Current global level: " << lw_global_level();
    EXPECT_FALSE(lw_can_log(42));

    lw_set_global_level(lw_severity_level_t(-42));
    EXPECT_TRUE(lw_global_level() == lw_severity_level_t::debug) << "Current global level: " << lw_global_level();
    EXPECT_FALSE(lw_can_log(-42));
}

/**
 *  \test   Resilience of the C-style API to invalid channel severity levels.
 *  \see    lw_set_logger_level, lw_can_channel_log
 *
 *  **Test logic description:**
 *  Parameter validation at the specific channel level is verified. Passing
 *  invalid values (`42`, `-42`) to the `lw_set_logger_level` function must not
 *  cause a reset of the log channel's current valid settings.
 *
 *  **Steps to reproduce:**
 *  -# Request the `"Root"` channel and change its local filter to the `INFO` state.
 *  -# Invoke `lw_set_logger_level` with the invalid number `42`. Verify the
 *      channel predicates for `INFO` and `42`.
 *  -# Invoke `lw_set_logger_level` with the invalid number `-42`. Verify the
 *      predicates similarly.
 *
 *  \expected_result    The `"Root"` channel preserves its capability to correctly
 *      filter logs by the `INFO` level. Invalid runtime modifications are
 *      rejected by the internal backend logic.
 */
TEST_F(loggingf, invalid_logger_severity_level)
{
    EXPECT_TRUE(lw_init_logging(log_fn, lw_logging_policy_t::fixed_size, 1, lw_severity_level_t::crit, NULL));
    lw_loggerf_t root_logger = lw_get_logger("Root");

    lw_set_logger_level("Root", lw_severity_level_t::info);
    EXPECT_TRUE(lw_can_channel_log(root_logger, lw_severity_level_t::info));

    lw_set_logger_level("Root", lw_severity_level_t(42));
    EXPECT_TRUE(lw_can_channel_log(root_logger, lw_severity_level_t::info));
    EXPECT_FALSE(lw_can_channel_log(root_logger, 42));

    lw_set_logger_level("Root", lw_severity_level_t(-42));
    EXPECT_TRUE(lw_can_channel_log(root_logger, lw_severity_level_t::info));
    EXPECT_FALSE(lw_can_channel_log(root_logger, -42));
}

/**
 *  \test   Verification of channel isolation and independent filtering in
 *      `fixed_size` mode.
 *  \see lw_set_logger_level, LOGF_INFO, LOGF_ERROR, LOGF_CRIT
 *
 *  **Test logic description:**
 *  The test verifies that when using the C-interface and the fixed policy,
 *  the filter settings of one channel have no effect on the throughput of
 *  another channel.
 *
 *  **Steps to reproduce:**
 *  -# Create the `"Root"` and `"Channel"` loggers. Configure them to `INFO` and
 *      `ERROR` levels respectively.
 *  -# Send a batch of `INFO` and `ERROR` messages to both loggers.
 *  -# Change both channels to the `CRIT` level and send `ERROR` and `CRIT` messages.
 *  -# Compare the single global interception buffer against the common reference
 *      template.
 *
 *  \expected_result    The resulting buffer completely lacks an `INFO` entry
 *      for the `"Channel"` logger, as well as `ERROR` entries for both loggers
 *      after switching to the `CRIT` level. Filtering operated in isolation.
 */
TEST_F(loggingf, channels)
{
    EXPECT_TRUE(lw_init_logging(log_fn, lw_logging_policy_t::fixed_size, 2, lw_severity_level_t::crit, NULL));
    lw_loggerf_t root_logger = lw_get_logger("Root");
    lw_loggerf_t chan_logger = lw_get_logger("Channel");
    lw_set_global_level(lw_severity_level_t::debug);

    lw_set_logger_level("Root", lw_severity_level_t::info);
    lw_set_logger_level("Channel", lw_severity_level_t::error);
    LOGF_INFO(root_logger, "info log %d", 42);
    LOGF_INFO(chan_logger, "info log %d", 42);
    LOGF_ERROR(root_logger, "error log %d", 42);
    LOGF_ERROR(chan_logger, "error log %d", 42);

    lw_set_logger_level("Root", lw_severity_level_t::crit);
    lw_set_logger_level("Channel", lw_severity_level_t::crit);
    LOGF_ERROR(root_logger, "error log %d", 42);
    LOGF_ERROR(chan_logger, "error log %d", 42);
    LOGF_CRIT(root_logger, "crit log %d", 42);
    LOGF_CRIT(chan_logger, "crit log %d", 42);

    const std::string ethalon = "****-**-** **:**:**.*** [INFO ] Root: info log 42\n"
                                "****-**-** **:**:**.*** [ERROR] Root: error log 42\n"
                                "****-**-** **:**:**.*** [ERROR] Channel: error log 42\n"
                                "****-**-** **:**:**.*** [CRIT ] Root: crit log 42\n"
                                "****-**-** **:**:**.*** [CRIT ] Channel: crit log 42\n";
    const std::string log = g_test_loggerf.str();
    EXPECT_TRUE(is_equal_logs(ethalon, log)) << "'" << ethalon << "' != '" << log << "'";
}

/**
 *  \test   System behavior when handling long and invalid channel names.
 *  \see    lw_get_logger, lw_set_logger_level
 *
 *  **Test logic description:**
 *  Verifies operations with string length constraints on channel names. The test
 *  checks the library's behavior when creating loggers with an unacceptably long
 *  channel name.
 *
 *  **Steps to reproduce:**
 *  -# Create the `"123456789012345"` and `"12345678901234567890"` loggers.
 *  -# Set the `INFO` level for a non-existent/truncated channel named `"1234567890123456"`.
 *  -# Send a batch of `INFO` and `ERROR` logs to both original loggers.
 *
 *  \expected_result    An excessively long channel name is truncated to the
 *      maximum allowed limit. As a result, two loggers with different channel
 *      names end up with an identical name and become a single logger. The
 *      filter configurations applied via `lw_set_logger_level` affected both
 *      loggers, causing them to receive the last set logging level of `ERROR`.
 *      The `INFO` level messages were filtered out, and only two `ERROR` level
 *      lines reached the buffer.
 */
TEST_F(loggingf, long_channel_name)
{
    EXPECT_TRUE(lw_init_logging(log_fn, lw_logging_policy_t::fixed_size, 2, lw_severity_level_t::crit, NULL));
    lw_loggerf_t root_logger = lw_get_logger("123456789012345");
    lw_loggerf_t chan_logger = lw_get_logger("12345678901234567890");
    lw_set_global_level(lw_severity_level_t::debug);

    lw_set_logger_level("1234567890123456", lw_severity_level_t::info);
    lw_set_logger_level("12345678901234567890", lw_severity_level_t::error);
    LOGF_INFO(root_logger, "info log %d", 42);
    LOGF_INFO(chan_logger, "info log %d", 42);
    LOGF_ERROR(root_logger, "error log %d", 42);
    LOGF_ERROR(chan_logger, "error log %d", 42);

    const std::string ethalon = "****-**-** **:**:**.*** [ERROR] 123456789012345: error log 42\n"
                                "****-**-** **:**:**.*** [ERROR] 123456789012345: error log 42\n";
    const std::string log = g_test_loggerf.str();
    EXPECT_TRUE(is_equal_logs(ethalon, log)) << "'" << ethalon << "' != '" << log << "'";
}

/**
 *  \test   Verification of channel isolation and independent filtering in
 *      `dynamic_size` mode.
 *  \see    lw_set_logger_level, LOGF_INFO, LOGF_ERROR, LOGF_CRIT
 *
 *  **Test logic description:**
 *  Duplicates the scenario for verifying the independent operation of channel
 *  filters, but switches the memory allocation policy of the logging subsystem
 *  to the dynamic resizing (`dynamic_size`) mode.
 *
 *  **Steps to reproduce:**
 *  -# Initialize the subsystem with the `dynamic_size` flag. Create the `"Root"`
 *      and `"Channel"` loggers.
 *  -# Configure the `INFO` and `ERROR` levels for the channels respectively.
 *      Send test logs.
 *  -# Raise the levels to `CRIT`, and send a final batch of messages. Verify
 *      the buffer.
 *
 *  \expected_result    The dynamic mode operates identically to the fixed mode
 *      from a logical perspective. Messages are successfully filtered based on
 *      the individual severity thresholds of each channel.
 */
TEST_F(loggingf, channels_dynamic)
{
    EXPECT_TRUE(lw_init_logging(log_fn, lw_logging_policy_t::dynamic_size, 2, lw_severity_level_t::crit, NULL));
    lw_loggerf_t root_logger = lw_get_logger("Root");
    lw_loggerf_t chan_logger = lw_get_logger("Channel");
    lw_set_global_level(lw_severity_level_t::debug);

    lw_set_logger_level("Root", lw_severity_level_t::info);
    lw_set_logger_level("Channel", lw_severity_level_t::error);
    LOGF_INFO(root_logger, "info log %d", 42);
    LOGF_INFO(chan_logger, "info log %d", 42);
    LOGF_ERROR(root_logger, "error log %d", 42);
    LOGF_ERROR(chan_logger, "error log %d", 42);

    lw_set_logger_level("Root", lw_severity_level_t::crit);
    lw_set_logger_level("Channel", lw_severity_level_t::crit);
    LOGF_ERROR(root_logger, "error log %d", 42);
    LOGF_ERROR(chan_logger, "error log %d", 42);
    LOGF_CRIT(root_logger, "crit log %d", 42);
    LOGF_CRIT(chan_logger, "crit log %d", 42);

    const std::string ethalon = "****-**-** **:**:**.*** [INFO ] Root: info log 42\n"
                                "****-**-** **:**:**.*** [ERROR] Root: error log 42\n"
                                "****-**-** **:**:**.*** [ERROR] Channel: error log 42\n"
                                "****-**-** **:**:**.*** [CRIT ] Root: crit log 42\n"
                                "****-**-** **:**:**.*** [CRIT ] Channel: crit log 42\n";
    const std::string log = g_test_loggerf.str();
    EXPECT_TRUE(is_equal_logs(ethalon, log)) << "'" << ethalon << "' != '" << log << "'";
}

/**
 *  \test   System behavior when handling long and invalid channel names.
 *  \see    lw_get_logger, lw_set_logger_level
 *
 *  **Test logic description:**
 *  Verifies operations with string length constraints on channel names. The test
 *  checks the library's behavior when creating loggers with an unacceptably long
 *  channel name.
 *
 *  **Steps to reproduce:**
 *  -# Create the `"123456789012345"` and `"12345678901234567890"` loggers.
 *  -# Set the `INFO` level for a non-existent/truncated channel named `"1234567890123456"`.
 *  -# Send a batch of `INFO` and `ERROR` logs to both original loggers.
 *
 *  \expected_result    An excessively long channel name is truncated to the
 *      maximum allowed limit. As a result, two loggers with different channel
 *      names end up with an identical name and become a single logger. The
 *      filter configurations applied via `lw_set_logger_level` affected both
 *      loggers, causing them to receive the last set logging level of `ERROR`.
 *      The `INFO` level messages were filtered out, and only two `ERROR` level
 *      lines reached the buffer.
 */
TEST_F(loggingf, long_channel_name_dynamic)
{
    EXPECT_TRUE(lw_init_logging(log_fn, lw_logging_policy_t::dynamic_size, 2, lw_severity_level_t::crit, NULL));
    lw_loggerf_t root_logger = lw_get_logger("123456789012345");
    lw_loggerf_t chan_logger = lw_get_logger("12345678901234567890");
    lw_set_global_level(lw_severity_level_t::debug);

    lw_set_logger_level("1234567890123456", lw_severity_level_t::info);
    lw_set_logger_level("12345678901234567890", lw_severity_level_t::error);
    LOGF_INFO(root_logger, "info log %d", 42);
    LOGF_INFO(chan_logger, "info log %d", 42);
    LOGF_ERROR(root_logger, "error log %d", 42);
    LOGF_ERROR(chan_logger, "error log %d", 42);

    const std::string ethalon = "****-**-** **:**:**.*** [ERROR] 123456789012345: error log 42\n"
                                "****-**-** **:**:**.*** [ERROR] 123456789012345: error log 42\n";
    const std::string log = g_test_loggerf.str();
    EXPECT_TRUE(is_equal_logs(ethalon, log)) << "'" << ethalon << "' != '" << log << "'";
}

/**
 *  \test   Verification of the hard channel count limit in `fixed_size` mode.
 *  \see    lw_init_logging, lw_get_logger, LOGF_INFO, LOGF_ERROR
 *
 *  **Test logic description:**
 *  Verifies the behavior of the subsystem when exceeding the maximum number of
 *  simultaneously allowed channels in a fixed mode (`fixed_size`). A limit of 1
 *  channel is initialized. When requesting a second channel, the system must
 *  prevent its creation.
 *
 *  **Steps to reproduce:**
 *  -# Initialize the subsystem with the `fixed_size` policy and a limit of
 *      exactly `1` channel.
 *  -# Successfully request the first logger, `"Root"`.
 *  -# Attempt to request a second logger, `"Channel"`, which exceeds the
 *      specified limit.
 *  -# Send a batch of test messages (`INFO`, `ERROR`, `CRIT`) to both loggers.
 *  -# Verify the resulting buffer to ensure the absence of logs from the
 *      disallowed channel.
 *
 *  \expected_result    The creation of the `"Channel"` logger is rejected by
 *      the internal logic due to the limit (returning `nullptr`). The logging
 *      macros safely ignore calls with a `nullptr` logger. The final buffer
 *      strictly contains entries from the valid `"Root"` channel alone.
 */
TEST_F(loggingf, channels_limit)
{
    EXPECT_TRUE(lw_init_logging(log_fn, lw_logging_policy_t::fixed_size, 1, lw_severity_level_t::crit, NULL));
    lw_loggerf_t root_logger = lw_get_logger("Root");
    lw_loggerf_t chan_logger = lw_get_logger("Channel");
    lw_set_global_level(lw_severity_level_t::debug);

    lw_set_logger_level("Root", lw_severity_level_t::info);
    lw_set_logger_level("Channel", lw_severity_level_t::error);
    LOGF_INFO(root_logger, "info log %d", 42);
    LOGF_INFO(chan_logger, "info log %d", 42);
    LOGF_ERROR(root_logger, "error log %d", 42);
    LOGF_ERROR(chan_logger, "error log %d", 42);

    lw_set_logger_level("Root", lw_severity_level_t::crit);
    lw_set_logger_level("Channel", lw_severity_level_t::crit);
    LOGF_ERROR(root_logger, "error log %d", 42);
    LOGF_ERROR(chan_logger, "error log %d", 42);
    LOGF_CRIT(root_logger, "crit log %d", 42);
    LOGF_CRIT(chan_logger, "crit log %d", 42);

    const std::string ethalon = "****-**-** **:**:**.*** [INFO ] Root: info log 42\n"
                                "****-**-** **:**:**.*** [ERROR] Root: error log 42\n"
                                "****-**-** **:**:**.*** [CRIT ] Root: crit log 42\n";
    const std::string log = g_test_loggerf.str();
    EXPECT_TRUE(is_equal_logs(ethalon, log)) << "'" << ethalon << "' != '" << log << "'";
}

/**
 *  \test   Testing of the channel hash table in dynamic mode (`dynamic_size`).
 *  \see    lw_init_logging, lw_get_logger, lw_set_logger_level
 *
 *  **Test logic description:**
 *  Verifies the correct behavior of the internal storage structure of the dynamic
 *  manager during mass channel registration. The test simulates a heavy workload
 *  by generating 64 unique channels, forcing collisions and memory reallocations,
 *  and then verifies the availability of the target loggers.
 *
 *  **Steps to reproduce:**
 *  -# Initialize the system in `dynamic_size` mode with a base size of 2.
 *  -# Generate and register 64 channels named `"Channel_0"` through
 *      `"Channel_63"` in a loop.
 *  -# Request the control loggers `"Root"` and `"Channel"`.
 *  -# Verify standard runtime filtering level modifications and execute log entries.
 *
 *  \expected_result    The dynamic structure scales successfully. Mass registration
 *      does not lead to memory corruption or lost pointers. The control loggers
 *      are correctly retrieved, configured, and write messages in strict accordance
 *      with the reference template.
 */
TEST_F(loggingf, channels_dynamic_rehash)
{
    EXPECT_TRUE(lw_init_logging(log_fn, lw_logging_policy_t::dynamic_size, 2, lw_severity_level_t::crit, NULL));
    for (size_t i = 0; i < 64; ++ i) {
        std::string ch_name = "Channel_" + std::to_string(i);
        lw_get_logger(ch_name.c_str());
    }
    lw_loggerf_t root_logger = lw_get_logger("Root");
    lw_loggerf_t chan_logger = lw_get_logger("Channel");
    lw_set_global_level(lw_severity_level_t::debug);

    lw_set_logger_level("Root", lw_severity_level_t::info);
    lw_set_logger_level("Channel", lw_severity_level_t::error);
    LOGF_INFO(root_logger, "info log %d", 42);
    LOGF_INFO(chan_logger, "info log %d", 42);
    LOGF_ERROR(root_logger, "error log %d", 42);
    LOGF_ERROR(chan_logger, "error log %d", 42);

    lw_set_logger_level("Root", lw_severity_level_t::crit);
    lw_set_logger_level("Channel", lw_severity_level_t::crit);
    LOGF_ERROR(root_logger, "error log %d", 42);
    LOGF_ERROR(chan_logger, "error log %d", 42);
    LOGF_CRIT(root_logger, "crit log %d", 42);
    LOGF_CRIT(chan_logger, "crit log %d", 42);

    const std::string ethalon = "****-**-** **:**:**.*** [INFO ] Root: info log 42\n"
                                "****-**-** **:**:**.*** [ERROR] Root: error log 42\n"
                                "****-**-** **:**:**.*** [ERROR] Channel: error log 42\n"
                                "****-**-** **:**:**.*** [CRIT ] Root: crit log 42\n"
                                "****-**-** **:**:**.*** [CRIT ] Channel: crit log 42\n";
    const std::string log = g_test_loggerf.str();
    EXPECT_TRUE(is_equal_logs(ethalon, log)) << "'" << ethalon << "' != '" << log << "'";
}

/**
 *  \test   Verification of the global logging level immutability (lock) mechanism.
 *  \see    lw_set_immutable_global_level, lw_set_global_level, lw_can_channel_log
 *
 *  **Test logic description:**
 *  Verifies the state "freezing" logic. The `lw_set_immutable_global_level` method
 *  must permanently lock the global logging level once and block any subsequent
 *  runtime changes (both via `lw_set_global_level` and via repeated invocations of
 *  `lw_set_immutable_global_level` itself).
 *
 *  **Steps to reproduce:**
 *  -# Set the global level to `DEBUG` and verify that the `"Root"` channel
 *      permits `INFO` entries.
 *  -# Freeze the global level to `WARNING` using `lw_set_immutable_global_level`.
 *  -# Attempt to modify the level to `CRIT` via regular `lw_set_global_level()`.
 *      Verify that the level remains `WARNING`.
 *  -# Attempt to repeatedly overwrite the immutable level to `DEBUG` via
 *      `lw_set_immutable_global_level`.
 *
 *  \expected_result    After the first invocation of `lw_set_immutable_global_level`,
 *      the state is frozen at the `WARNING` value. No subsequent calls to level
 *      modification methods (including repeated locking attempts) can alter this
 *      value. The protection mechanism functions correctly.
 */
TEST_F(loggingf, immutable)
{
    EXPECT_TRUE(lw_init_logging(log_fn, lw_logging_policy_t::fixed_size, 1, lw_severity_level_t::crit, NULL));
    lw_loggerf_t root_logger = lw_get_logger("Root");

    lw_set_global_level(lw_severity_level_t::debug);
    lw_set_logger_level("Root", lw_severity_level_t::info);
    EXPECT_TRUE(lw_global_level() == lw_severity_level_t::debug);
    EXPECT_TRUE(lw_can_channel_log(root_logger, lw_severity_level_t::info));

    lw_set_immutable_global_level(lw_severity_level_t::warning);

    lw_set_global_level(lw_severity_level_t::crit);
    lw_set_logger_level("Root", lw_severity_level_t::crit);
    EXPECT_TRUE(lw_global_level() == lw_severity_level_t::warning);
    EXPECT_TRUE(lw_can_channel_log(root_logger, lw_severity_level_t::crit));

    lw_set_global_level(lw_severity_level_t::debug);
    EXPECT_TRUE(lw_global_level() == lw_severity_level_t::warning);
}

/**
 *  \test   Verification of logger creation with an explicit default level.
 *  \see    lw_get_logger_dfl, LOGF_ERROR, LOGF_CRIT
 *
 *  **Test logic description:**
 *  The test verifies the functionality of the `lw_get_logger_dfl` method, which
 *  allows setting an individual filtering threshold for a newly created channel
 *  exactly at the moment of its initialization, overriding the current global
 *  level.
 *
 *  **Steps to reproduce:**
 *  -# Set the general global level to `DEBUG`.
 *  -# Initialize a new channel named `"Root"` via the `lw_get_logger_dfl` method,
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
    g_test_loggerf.clear();
    EXPECT_TRUE(lw_init_logging(log_fn, lw_logging_policy_t::fixed_size, 1, lw_severity_level_t::trace, NULL));
    lw_loggerf_t root_logger = lw_get_logger_dfl("Root", lw_severity_level_t::crit);
    ASSERT_TRUE(root_logger != nullptr);
    LOGF_ERROR(root_logger, "error log %d", 42);
    LOGF_CRIT(root_logger, "crit log %d", 42);

    std::string ethalon = "****-**-** **:**:**.*** [CRIT ] Root: crit log 42\n";
    std::string log = g_test_loggerf.str();
    EXPECT_TRUE(is_equal_logs(ethalon, log)) << "'" << ethalon << "' != '" << log << "'";

    ethalon = "****-**-** **:**:**.*** [CRIT ] Root: crit log 42\n";
    log = g_test_loggerf.str();
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
