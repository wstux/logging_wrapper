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

#include <cstdarg>
#include <cstdio>
#include <iostream>
#include <limits>
#include <sstream>

#include <testing/testdefs.h>

#include "logging_wrapper/logging.h"

namespace {

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

struct test_loggerf final
{
    test_loggerf(const std::string&) {}

    void log(const char* p_fmt, ...)
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

TEST(logging_cpp, logging)
{
    using logger_t = ::wstux::logging::logger<test_logger>;

    ::wstux::logging::manager::clear();
    logger_t root_logger = ::wstux::logging::manager::get_logger<test_logger>("Root");
    LOG_ERROR(root_logger, "error log " << 42);

    const std::string ethalon = "****-**-** **:**:**.*** [ERROR] error log 42\n";
    const std::string log = root_logger.get_logger().str_logger.str();
    EXPECT_TRUE(is_equal_logs(ethalon, log)) << "'" << ethalon << "' != '" << log << "'";
}

TEST(logging_cpp, severity_level)
{
    using logger_t = ::wstux::logging::logger<test_logger>;

    ::wstux::logging::manager::clear();
    ::wstux::logging::manager::set_global_level(::wstux::logging::severity_level::crit);
    logger_t root_logger = ::wstux::logging::manager::get_logger<test_logger>("Root");
    LOG_ERROR(root_logger, "error log " << 42);
    LOG_CRIT(root_logger, "crit log " << 42);

    std::string ethalon = "****-**-** **:**:**.*** [CRIT ] crit log 42\n";
    std::string log = root_logger.get_logger().str_logger.str();
    EXPECT_TRUE(is_equal_logs(ethalon, log)) << "'" << ethalon << "' != '" << log << "'";

    ::wstux::logging::manager::set_global_level(::wstux::logging::severity_level::info);
    LOG_ERROR(root_logger, "error log " << 42);
    ethalon = "****-**-** **:**:**.*** [CRIT ] crit log 42\n"
              "****-**-** **:**:**.*** [ERROR] error log 42\n";
    log = root_logger.get_logger().str_logger.str();
    EXPECT_TRUE(is_equal_logs(ethalon, log)) << "'" << ethalon << "' != '" << log << "'";
}

TEST(logging_cpp, channels)
{
    using logger_t = ::wstux::logging::logger<test_logger>;

    ::wstux::logging::manager::clear();
    logger_t root_logger = ::wstux::logging::manager::get_logger<test_logger>("Root");
    logger_t chan_logger = ::wstux::logging::manager::get_logger<test_logger>("Channel");
    ::wstux::logging::manager::set_global_level(::wstux::logging::severity_level::debug);

    ::wstux::logging::manager::set_logger_level("Root", ::wstux::logging::severity_level::info);
    ::wstux::logging::manager::set_logger_level("Channel", ::wstux::logging::severity_level::error);
    LOG_INFO(root_logger, "Root: info log " << 42);
    LOG_INFO(chan_logger, "Channel: info log " << 42);
    LOG_ERROR(root_logger, "Root: error log " << 42);
    LOG_ERROR(chan_logger, "Channel: error log " << 42);

    ::wstux::logging::manager::set_logger_level("Root", ::wstux::logging::severity_level::crit);
    ::wstux::logging::manager::set_logger_level("Channel", ::wstux::logging::severity_level::crit);
    LOG_ERROR(root_logger, "Root: error log " << 42);
    LOG_ERROR(chan_logger, "Channel: error log " << 42);
    LOG_CRIT(root_logger, "Root: crit log " << 42);
    LOG_CRIT(chan_logger, "Channel: crit log " << 42);

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

TEST(loggingf, logging)
{
    using logger_t = ::wstux::logging::logger<test_loggerf>;

    ::wstux::logging::manager::clear();
    logger_t root_logger = ::wstux::logging::manager::get_logger<test_loggerf>("Root");
    LOGF_ERROR(root_logger, "error log, %d", 42);

    const std::string ethalon = "****-**-** **:**:**.*** [ERROR] error log, 42\n";
    const std::string log = root_logger.get_logger().str();
    EXPECT_TRUE(is_equal_logs(ethalon, log)) << "'" << ethalon << "' != '" << log << "'";
}

TEST(loggingf, severity_level)
{
    using logger_t = ::wstux::logging::logger<test_loggerf>;

    ::wstux::logging::manager::clear();
    ::wstux::logging::manager::set_global_level(::wstux::logging::severity_level::crit);
    logger_t root_logger = ::wstux::logging::manager::get_logger<test_loggerf>("Root");
    LOGF_ERROR(root_logger, "error log %d", 42);
    LOGF_CRIT(root_logger, "crit log %d", 42);

    std::string ethalon = "****-**-** **:**:**.*** [CRIT ] crit log 42\n";
    std::string log = root_logger.get_logger().str();
    EXPECT_TRUE(is_equal_logs(ethalon, log)) << "'" << ethalon << "' != '" << log << "'";

    ::wstux::logging::manager::set_global_level(::wstux::logging::severity_level::info);
    LOGF_ERROR(root_logger, "error log %d", 42);
    ethalon = "****-**-** **:**:**.*** [CRIT ] crit log 42\n"
              "****-**-** **:**:**.*** [ERROR] error log 42\n";
    log = root_logger.get_logger().str();
    EXPECT_TRUE(is_equal_logs(ethalon, log)) << "'" << ethalon << "' != '" << log << "'";
}

TEST(loggingf, channels)
{
    using logger_t = ::wstux::logging::logger<test_loggerf>;

    ::wstux::logging::manager::clear();
    logger_t root_logger = ::wstux::logging::manager::get_logger<test_loggerf>("Root");
    logger_t chan_logger = ::wstux::logging::manager::get_logger<test_loggerf>("Channel");
    ::wstux::logging::manager::set_global_level(::wstux::logging::severity_level::debug);

    ::wstux::logging::manager::set_logger_level("Root", ::wstux::logging::severity_level::info);
    ::wstux::logging::manager::set_logger_level("Channel", ::wstux::logging::severity_level::error);
    LOGF_INFO(root_logger, "Root: info log %d", 42);
    LOGF_INFO(chan_logger, "Channel: info log %d", 42);
    LOGF_ERROR(root_logger, "Root: error log %d", 42);
    LOGF_ERROR(chan_logger, "Channel: error log %d", 42);

    ::wstux::logging::manager::set_logger_level("Root", ::wstux::logging::severity_level::crit);
    ::wstux::logging::manager::set_logger_level("Channel", ::wstux::logging::severity_level::crit);
    LOGF_ERROR(root_logger, "Root: error log %d", 42);
    LOGF_ERROR(chan_logger, "Channel: error log %d", 42);
    LOGF_CRIT(root_logger, "Root: crit log %d", 42);
    LOGF_CRIT(chan_logger, "Channel: crit log %d", 42);

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

TEST(logging, combo_loggers)
{
    using logger_t = ::wstux::logging::logger<test_logger>;
    using loggerf_t = ::wstux::logging::logger<test_loggerf>;

    ::wstux::logging::manager::clear();
    logger_t root_logger = ::wstux::logging::manager::get_logger<test_logger>("Root");
    loggerf_t chan_logger = ::wstux::logging::manager::get_logger<test_loggerf>("Channel");
    ::wstux::logging::manager::set_global_level(::wstux::logging::severity_level::debug);

    ::wstux::logging::manager::set_logger_level("Root", ::wstux::logging::severity_level::info);
    ::wstux::logging::manager::set_logger_level("Channel", ::wstux::logging::severity_level::error);
    LOG_INFO(root_logger, "Root: info log " << 42);
    LOGF_INFO(chan_logger, "Channel: info log %d", 42);
    LOG_ERROR(root_logger, "Root: error log " << 42);
    LOGF_ERROR(chan_logger, "Channel: error log %d", 42);

    ::wstux::logging::manager::set_logger_level("Root", ::wstux::logging::severity_level::crit);
    ::wstux::logging::manager::set_logger_level("Channel", ::wstux::logging::severity_level::crit);
    LOG_ERROR(root_logger, "Root: error log " << 42);
    LOGF_ERROR(chan_logger, "Channel: error log %d", 42);
    LOG_CRIT(root_logger, "Root: crit log " << 42);
    LOGF_CRIT(chan_logger, "Channel: crit log %d", 42);

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

int main(int /*argc*/, char** /*argv*/)
{
    return RUN_ALL_TESTS();
}
