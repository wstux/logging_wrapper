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

#include <cstdarg>
#include <cstdio>
#include <iostream>
#include <limits>
#include <sstream>

#include <testing/testdefs.h>

#include "loggingf_wrapper/logging.h"

namespace {

struct test_loggerf final
{
    void clear() { cur_size = 0; }
    std::string str() const { return std::string(buffer, cur_size); }

    char buffer[1024];
    size_t cur_size = 0;
};

test_loggerf g_test_loggerf;

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

} // <anonymous> namespace

TEST(loggingf, logging)
{
    g_test_loggerf.clear();
    EXPECT_TRUE(init_logging(log_fn, logging_policy_t::fixed_size, 1, severity_level::debug, NULL));
    loggerf_t root_logger = get_logger("Root");
    EXPECT_TRUE(root_logger != nullptr);
    LOGF_ERROR(root_logger, "error log, %d", 42);

    const std::string ethalon = "****-**-** **:**:**.*** [ERROR] Root: error log, 42\n";
    const std::string log = g_test_loggerf.str();
    EXPECT_TRUE(is_equal_logs(ethalon, log)) << "'" << ethalon << "' != '" << log << "'";
    deinit_logging();
}

TEST(loggingf, logging_dynamic)
{
    g_test_loggerf.clear();
    EXPECT_TRUE(init_logging(log_fn, logging_policy_t::dynamic_size, 1, severity_level::debug, NULL));
    loggerf_t root_logger = get_logger("Root");
    EXPECT_TRUE(root_logger != nullptr);
    LOGF_ERROR(root_logger, "error log, %d", 42);

    const std::string ethalon = "****-**-** **:**:**.*** [ERROR] Root: error log, 42\n";
    const std::string log = g_test_loggerf.str();
    EXPECT_TRUE(is_equal_logs(ethalon, log)) << "'" << ethalon << "' != '" << log << "'";
    deinit_logging();
}

TEST(loggingf, severity_level)
{
    g_test_loggerf.clear();
    EXPECT_TRUE(init_logging(log_fn, logging_policy_t::fixed_size, 1, severity_level::crit, NULL));
    loggerf_t root_logger = get_logger("Root");
    EXPECT_TRUE(root_logger != nullptr);
    LOGF_ERROR(root_logger, "error log %d", 42);
    LOGF_CRIT(root_logger, "crit log %d", 42);

    std::string ethalon = "****-**-** **:**:**.*** [CRIT ] Root: crit log 42\n";
    std::string log = g_test_loggerf.str();
    EXPECT_TRUE(is_equal_logs(ethalon, log)) << "'" << ethalon << "' != '" << log << "'";

    set_global_level(severity_level::info);
    LOGF_ERROR(root_logger, "error log %d", 42);
    ethalon = "****-**-** **:**:**.*** [CRIT ] Root: crit log 42\n"
              "****-**-** **:**:**.*** [ERROR] Root: error log 42\n";
    log = g_test_loggerf.str();
    EXPECT_TRUE(is_equal_logs(ethalon, log)) << "'" << ethalon << "' != '" << log << "'";
    deinit_logging();
}

TEST(loggingf, channels)
{
    g_test_loggerf.clear();
    EXPECT_TRUE(init_logging(log_fn, logging_policy_t::fixed_size, 2, severity_level::crit, NULL));
    loggerf_t root_logger = get_logger("Root");
    loggerf_t chan_logger = get_logger("Channel");
    set_global_level(severity_level::debug);

    set_logger_level("Root", severity_level::info);
    set_logger_level("Channel", severity_level::error);
    LOGF_INFO(root_logger, "info log %d", 42);
    LOGF_INFO(chan_logger, "info log %d", 42);
    LOGF_ERROR(root_logger, "error log %d", 42);
    LOGF_ERROR(chan_logger, "error log %d", 42);

    set_logger_level("Root", severity_level::crit);
    set_logger_level("Channel", severity_level::crit);
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
    deinit_logging();
}

TEST(loggingf, channels_dynamic)
{
    g_test_loggerf.clear();
    EXPECT_TRUE(init_logging(log_fn, logging_policy_t::dynamic_size, 2, severity_level::crit, NULL));
    loggerf_t root_logger = get_logger("Root");
    loggerf_t chan_logger = get_logger("Channel");
    set_global_level(severity_level::debug);

    set_logger_level("Root", severity_level::info);
    set_logger_level("Channel", severity_level::error);
    LOGF_INFO(root_logger, "info log %d", 42);
    LOGF_INFO(chan_logger, "info log %d", 42);
    LOGF_ERROR(root_logger, "error log %d", 42);
    LOGF_ERROR(chan_logger, "error log %d", 42);

    set_logger_level("Root", severity_level::crit);
    set_logger_level("Channel", severity_level::crit);
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
    deinit_logging();
}

TEST(loggingf, channels_limit)
{
    g_test_loggerf.clear();
    EXPECT_TRUE(init_logging(log_fn, logging_policy_t::fixed_size, 1, severity_level::crit, NULL));
    loggerf_t root_logger = get_logger("Root");
    loggerf_t chan_logger = get_logger("Channel");
    set_global_level(severity_level::debug);

    set_logger_level("Root", severity_level::info);
    set_logger_level("Channel", severity_level::error);
    LOGF_INFO(root_logger, "info log %d", 42);
    LOGF_INFO(chan_logger, "info log %d", 42);
    LOGF_ERROR(root_logger, "error log %d", 42);
    LOGF_ERROR(chan_logger, "error log %d", 42);

    set_logger_level("Root", severity_level::crit);
    set_logger_level("Channel", severity_level::crit);
    LOGF_ERROR(root_logger, "error log %d", 42);
    LOGF_ERROR(chan_logger, "error log %d", 42);
    LOGF_CRIT(root_logger, "crit log %d", 42);
    LOGF_CRIT(chan_logger, "crit log %d", 42);

    const std::string ethalon = "****-**-** **:**:**.*** [INFO ] Root: info log 42\n"
                                "****-**-** **:**:**.*** [ERROR] Root: error log 42\n"
                                "****-**-** **:**:**.*** [CRIT ] Root: crit log 42\n";
    const std::string log = g_test_loggerf.str();
    EXPECT_TRUE(is_equal_logs(ethalon, log)) << "'" << ethalon << "' != '" << log << "'";
    deinit_logging();
}

TEST(loggingf, channels_dynamic_hash)
{
    g_test_loggerf.clear();
    EXPECT_TRUE(init_logging(log_fn, logging_policy_t::dynamic_size, 2, severity_level::crit, NULL));
    for (size_t i = 0; i < 64; ++ i) {
        std::string ch_name = "Channel_" + std::to_string(i);
        get_logger(ch_name.c_str());
    }
    loggerf_t root_logger = get_logger("Root");
    loggerf_t chan_logger = get_logger("Channel");
    set_global_level(severity_level::debug);

    set_logger_level("Root", severity_level::info);
    set_logger_level("Channel", severity_level::error);
    LOGF_INFO(root_logger, "info log %d", 42);
    LOGF_INFO(chan_logger, "info log %d", 42);
    LOGF_ERROR(root_logger, "error log %d", 42);
    LOGF_ERROR(chan_logger, "error log %d", 42);

    set_logger_level("Root", severity_level::crit);
    set_logger_level("Channel", severity_level::crit);
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
    deinit_logging();
}

int main(int /*argc*/, char** /*argv*/)
{
    return RUN_ALL_TESTS();
}
