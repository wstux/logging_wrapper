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

class logging_fixture : public ::testing::Test
{
public:
    virtual void SetUp() override {  }
    virtual void TearDown() override
    {
        g_test_loggerf.clear();
        lw_deinit_logging();
    }
};

using loggingf = logging_fixture;

} // <anonymous> namespace

TEST_F(loggingf, logging)
{
    EXPECT_TRUE(lw_init_logging(log_fn, lw_logging_policy_t::fixed_size, 1, lw_severity_level_t::debug, NULL));
    lw_loggerf_t root_logger = lw_get_logger("Root");
    EXPECT_TRUE(root_logger != nullptr);
    LOGF_ERROR(root_logger, "error log, %d", 42);

    const std::string ethalon = "****-**-** **:**:**.*** [ERROR] Root: error log, 42\n";
    const std::string log = g_test_loggerf.str();
    EXPECT_TRUE(is_equal_logs(ethalon, log)) << "'" << ethalon << "' != '" << log << "'";
}

TEST_F(loggingf, logging_dynamic)
{
    EXPECT_TRUE(lw_init_logging(log_fn, lw_logging_policy_t::dynamic_size, 1, lw_severity_level_t::debug, NULL));
    lw_loggerf_t root_logger = lw_get_logger("Root");
    EXPECT_TRUE(root_logger != nullptr);
    LOGF_ERROR(root_logger, "error log, %d", 42);

    const std::string ethalon = "****-**-** **:**:**.*** [ERROR] Root: error log, 42\n";
    const std::string log = g_test_loggerf.str();
    EXPECT_TRUE(is_equal_logs(ethalon, log)) << "'" << ethalon << "' != '" << log << "'";
    lw_deinit_logging();
}

TEST_F(loggingf, severity_level)
{
    g_test_loggerf.clear();
    EXPECT_TRUE(lw_init_logging(log_fn, lw_logging_policy_t::fixed_size, 1, lw_severity_level_t::crit, NULL));
    lw_loggerf_t root_logger = lw_get_logger("Root");
    EXPECT_TRUE(root_logger != nullptr);
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

TEST_F(loggingf, channels_dynamic_hash)
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

int main(int /*argc*/, char** /*argv*/)
{
    return RUN_ALL_TESTS();
}
