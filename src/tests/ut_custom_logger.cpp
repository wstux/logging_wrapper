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

#define LOGGINGF_WRAPPER_IMPL(logger, level, fmt, ...)                      \
    logger.get_logger()(LOGF_LEVEL(level) " %s: " fmt "\n",                 \
           logger.channel().c_str() __VA_OPT__(,) __VA_ARGS__)

#define LOGGING_WRAPPER_IMPL(logger, level)                                 \
    logger.get_logger() << LOG_LEVEL(level) << " " << logger.channel() << ": "

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

class logging_fixture : public ::testing::Test
{
public:
    virtual void SetUp() override { ::wstux::logging::manager::init(); }
    virtual void TearDown() override { ::wstux::logging::manager::clear(); }
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

TEST_F(logging_cpp, logging)
{
    using logger_t = ::wstux::logging::logger<test_logger>;

    logger_t root_logger = ::wstux::logging::manager::get_logger<test_logger>("Root");
    LOG_ERROR(root_logger, "error log " << 42);

    const std::string ethalon = "[ERROR] Root: error log 42\n";
    const std::string log = root_logger.get_logger().str_logger.str();
    EXPECT_TRUE(ethalon == log) << "'" << ethalon << "' != '" << log << "'";
}

TEST_F(logging_cpp, logging_specific)
{
    using logger_t = ::wstux::logging::logger<test_specific_logger>;

    logger_t root_logger = ::wstux::logging::manager::get_logger<test_specific_logger>("Root");
    LOG_ERROR(root_logger, "error log " << 42);

    const std::string ethalon = "[ERROR] Root: error log 42\n";
    const std::string log = root_logger.get_logger().str_logger.str();
    EXPECT_TRUE(ethalon == log) << "'" << ethalon << "' != '" << log << "'";
}

TEST_F(logging_cpp, severity_level)
{
    using logger_t = ::wstux::logging::logger<test_logger>;

    ::wstux::logging::manager::set_global_level(::wstux::logging::severity_level::crit);
    logger_t root_logger = ::wstux::logging::manager::get_logger<test_logger>("Root");
    LOG_ERROR(root_logger, "error log " << 42);
    LOG_CRIT(root_logger, "crit log " << 42);

    std::string ethalon = "[CRIT ] Root: crit log 42\n";
    std::string log = root_logger.get_logger().str_logger.str();
    EXPECT_TRUE(ethalon == log) << "'" << ethalon << "' != '" << log << "'";

    ::wstux::logging::manager::set_global_level(::wstux::logging::severity_level::info);
    LOG_ERROR(root_logger, "error log " << 42);
    ethalon = "[CRIT ] Root: crit log 42\n"
              "[ERROR] Root: error log 42\n";
    log = root_logger.get_logger().str_logger.str();
    EXPECT_TRUE(ethalon == log) << "'" << ethalon << "' != '" << log << "'";
}

TEST_F(logging_cpp, channels)
{
    using logger_t = ::wstux::logging::logger<test_logger>;

    logger_t root_logger = ::wstux::logging::manager::get_logger<test_logger>("Root");
    logger_t chan_logger = ::wstux::logging::manager::get_logger<test_logger>("Channel");
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

    const std::string ethalon_root = "[INFO ] Root: info log 42\n"
                                     "[ERROR] Root: error log 42\n"
                                     "[CRIT ] Root: crit log 42\n";
    const std::string ethalon_chan = "[ERROR] Channel: error log 42\n"
                                     "[CRIT ] Channel: crit log 42\n";
    const std::string log_root = root_logger.get_logger().str_logger.str();
    const std::string log_chan = chan_logger.get_logger().str_logger.str();
    EXPECT_TRUE(ethalon_root == log_root) << "'" << ethalon_root << "' != '" << log_root << "'";
    EXPECT_TRUE(ethalon_chan == log_chan) << "'" << ethalon_chan << "' != '" << log_chan << "'";
}

TEST_F(loggingf, logging)
{
    using logger_t = ::wstux::logging::logger<test_loggerf>;

    logger_t root_logger = ::wstux::logging::manager::get_logger<test_loggerf>("Root");
    LOGF_ERROR(root_logger, "error log, %d", 42);

    const std::string ethalon = "[ERROR] Root: error log, 42\n";
    const std::string log = root_logger.get_logger().str();
    EXPECT_TRUE(ethalon == log) << "'" << ethalon << "' != '" << log << "'";
}

TEST_F(loggingf, severity_level)
{
    using logger_t = ::wstux::logging::logger<test_loggerf>;

    ::wstux::logging::manager::set_global_level(::wstux::logging::severity_level::crit);
    logger_t root_logger = ::wstux::logging::manager::get_logger<test_loggerf>("Root");
    LOGF_ERROR(root_logger, "error log %d", 42);
    LOGF_CRIT(root_logger, "crit log %d", 42);

    std::string ethalon = "[CRIT ] Root: crit log 42\n";
    std::string log = root_logger.get_logger().str();
    EXPECT_TRUE(ethalon == log) << "'" << ethalon << "' != '" << log << "'";

    ::wstux::logging::manager::set_global_level(::wstux::logging::severity_level::info);
    LOGF_ERROR(root_logger, "error log %d", 42);
    ethalon = "[CRIT ] Root: crit log 42\n"
              "[ERROR] Root: error log 42\n";
    log = root_logger.get_logger().str();
    EXPECT_TRUE(ethalon == log) << "'" << ethalon << "' != '" << log << "'";
}

TEST_F(loggingf, channels)
{
    using logger_t = ::wstux::logging::logger<test_loggerf>;

    logger_t root_logger = ::wstux::logging::manager::get_logger<test_loggerf>("Root");
    logger_t chan_logger = ::wstux::logging::manager::get_logger<test_loggerf>("Channel");
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

    const std::string ethalon_root = "[INFO ] Root: info log 42\n"
                                     "[ERROR] Root: error log 42\n"
                                     "[CRIT ] Root: crit log 42\n";
    const std::string ethalon_chan = "[ERROR] Channel: error log 42\n"
                                     "[CRIT ] Channel: crit log 42\n";
    const std::string log_root = root_logger.get_logger().str();
    const std::string log_chan = chan_logger.get_logger().str();
    EXPECT_TRUE(ethalon_root == log_root) << "'" << ethalon_root << "' != '" << log_root << "'";
    EXPECT_TRUE(ethalon_chan == log_chan) << "'" << ethalon_chan << "' != '" << log_chan << "'";
}

TEST_F(logging, combo_loggers)
{
    using logger_t = ::wstux::logging::logger<test_logger>;
    using loggerf_t = ::wstux::logging::logger<test_loggerf>;

    logger_t root_logger = ::wstux::logging::manager::get_logger<test_logger>("Root");
    loggerf_t chan_logger = ::wstux::logging::manager::get_logger<test_loggerf>("Channel");
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

    const std::string ethalon_root = "[INFO ] Root: info log 42\n"
                                     "[ERROR] Root: error log 42\n"
                                     "[CRIT ] Root: crit log 42\n";
    const std::string ethalon_chan = "[ERROR] Channel: error log 42\n"
                                     "[CRIT ] Channel: crit log 42\n";
    const std::string log_root = root_logger.get_logger().str_logger.str();
    const std::string log_chan = chan_logger.get_logger().str();
    EXPECT_TRUE(ethalon_root == log_root) << "'" << ethalon_root << "' != '" << log_root << "'";
    EXPECT_TRUE(ethalon_chan == log_chan) << "'" << ethalon_chan << "' != '" << log_chan << "'";
}

int main(int /*argc*/, char** /*argv*/)
{
    return RUN_ALL_TESTS();
}
