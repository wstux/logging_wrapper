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
    LOG_ERROR(root_logger, "error log, logging");

    const std::string ethalon = "****-**-** **:**:**.*** [ERROR] error log, logging\n";
    const std::string log = root_logger.get_logger().str_logger.str();
    EXPECT_TRUE(is_equal_logs(ethalon, log)) << "'" << ethalon << "' != '" << log << "'";
}

TEST(logging_cpp, severity_level)
{
    using logger_t = ::wstux::logging::logger<test_logger>;

    ::wstux::logging::manager::clear();
    ::wstux::logging::manager::set_global_level(::wstux::logging::severity_level::crit);
    logger_t root_logger = ::wstux::logging::manager::get_logger<test_logger>("Root");
    LOG_ERROR(root_logger, "error log");
    LOG_CRIT(root_logger, "crit log");

    std::string ethalon = "****-**-** **:**:**.*** [CRIT ] crit log\n";
    std::string log = root_logger.get_logger().str_logger.str();
    EXPECT_TRUE(is_equal_logs(ethalon, log)) << "'" << ethalon << "' != '" << log << "'";

    ::wstux::logging::manager::set_global_level(::wstux::logging::severity_level::info);
    LOG_ERROR(root_logger, "error log");
    ethalon = "****-**-** **:**:**.*** [CRIT ] crit log\n"
              "****-**-** **:**:**.*** [ERROR] error log\n";
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
    LOG_INFO(root_logger, "Root: info log");
    LOG_INFO(chan_logger, "Channel: info log");
    LOG_ERROR(root_logger, "Root: error log");
    LOG_ERROR(chan_logger, "Channel: error log");

    ::wstux::logging::manager::set_logger_level("Root", ::wstux::logging::severity_level::crit);
    ::wstux::logging::manager::set_logger_level("Channel", ::wstux::logging::severity_level::crit);
    LOG_ERROR(root_logger, "Root: error log");
    LOG_ERROR(chan_logger, "Channel: error log");
    LOG_CRIT(root_logger, "Root: crit log");
    LOG_CRIT(chan_logger, "Channel: crit log");

    const std::string ethalon_root = "****-**-** **:**:**.*** [INFO ] Root: info log\n"
                                     "****-**-** **:**:**.*** [ERROR] Root: error log\n"
                                     "****-**-** **:**:**.*** [CRIT ] Root: crit log\n";
    const std::string ethalon_chan = "****-**-** **:**:**.*** [ERROR] Channel: error log\n"
                                     "****-**-** **:**:**.*** [CRIT ] Channel: crit log\n";
    const std::string log_root = root_logger.get_logger().str_logger.str();
    const std::string log_chan = chan_logger.get_logger().str_logger.str();
    EXPECT_TRUE(is_equal_logs(ethalon_root, log_root)) << "'" << ethalon_root << "' != '" << log_root << "'";
    EXPECT_TRUE(is_equal_logs(ethalon_chan, log_chan)) << "'" << ethalon_chan << "' != '" << log_chan << "'";
}

int main(int /*argc*/, char** /*argv*/)
{
    return RUN_ALL_TESTS();
}
