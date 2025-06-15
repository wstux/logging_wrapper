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

#include <iostream>

#define LOGGING_WRAPPER_IMPL(logger, level)                                    \
    logger.get_logger() << ::wstux::logging::manager::timestamp() << " "       \
                        << LOG_LEVEL(level) << " " << logger.channel() << ": "

#include "logging_wrapper/logging.h"

struct clog_logger final
{
    template <typename T>
    inline std::ostream& operator<<(const T& val) { return std::clog << val; }
};

using clog_backend_t = clog_logger;
using logger_t = ::wstux::logging::logger<clog_backend_t>;

namespace wstux {
namespace logging {

template<> clog_backend_t make_logger<clog_backend_t>(const std::string&) { return clog_backend_t(); }

} // namespace logging
} // namespace wstux

int main()
{
    ::wstux::logging::manager::init(::wstux::logging::severity_level::debug);

    logger_t root_logger = ::wstux::logging::manager::get_logger<logger_t>("Root");
    LOG_INFO(root_logger, "Hello, world!");

    ::wstux::logging::manager::deinit();
    return 0;
}
