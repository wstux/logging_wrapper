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

} // <anonymous> namespace

TEST(logging_cpp, logging)
{
    using root_logger_t = ::wstux::logging::logger<test_logger>;

    ::wstux::logging::manager::clear();
    root_logger_t root_logger = ::wstux::logging::manager::get_logger<test_logger>("Root");
    LOG_ERROR(root_logger, "error log, logging");

    const std::string ethalon_example = "1970-01-01 00:00:00.000 [ERROR] error log, logging\n";
    const std::string ethalon = "[ERROR] error log, logging\n";
    std::string str = root_logger.get_logger().str_logger.str();
    ASSERT_TRUE(str.size() == ethalon_example.size()) << str.size() << " != " << ethalon_example.size();
    EXPECT_TRUE(str.substr(24) == ethalon) << "'" << str.substr(24) << "' != '" << ethalon << "'";
}

TEST(logging_cpp, severity_level)
{
    using root_logger_t = ::wstux::logging::logger<test_logger>;

    ::wstux::logging::manager::clear();
    ::wstux::logging::manager::set_global_level(::wstux::logging::severity_level::crit);
    root_logger_t root_logger = ::wstux::logging::manager::get_logger<test_logger>("Root");
    LOG_ERROR(root_logger, "error log");
    LOG_CRIT(root_logger, "crit log");

    std::string ethalon_example = "1970-01-01 00:00:00.000 [CRIT ] crit log\n";
    std::string ethalon = "[CRIT ] crit log\n";
    std::string str = root_logger.get_logger().str_logger.str();
    ASSERT_TRUE(str.size() == ethalon_example.size()) << str.size() << " != " << ethalon_example.size();
    EXPECT_TRUE(str.substr(24) == ethalon) << "'" << str.substr(24) << "' != '" << ethalon << "'";

    ::wstux::logging::manager::set_global_level(::wstux::logging::severity_level::info);
    LOG_ERROR(root_logger, "error log");
    ethalon_example = "1970-01-01 00:00:00.000 [CRIT ] crit log\n"
                      "1970-01-01 00:00:00.000 [ERROR] error log\n";
    ethalon = "[CRIT ] crit log\n";
    str = root_logger.get_logger().str_logger.str();
    ASSERT_TRUE(str.size() == ethalon_example.size()) << str.size() << " != " << ethalon_example.size();
    EXPECT_TRUE(str.substr(24, 17) == ethalon) << "'" << str.substr(24, 17) << "' != '" << ethalon << "'";
    ethalon = "[ERROR] error log\n";
    EXPECT_TRUE(str.substr(65, 18) == ethalon) << "'" << str.substr(65, 18) << "' != '" << ethalon << "'";
}

int main(int /*argc*/, char** /*argv*/)
{
    return RUN_ALL_TESTS();
}
