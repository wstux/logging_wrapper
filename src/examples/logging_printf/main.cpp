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

#define LOGGINGF_WRAPPER_IMPL(logger, level, fmt, ...)                         \
    char cur_ts[24];                                                           \
    ::wstux::logging::manager::timestamp(cur_ts, 24);                          \
    logger.get_logger()("%s " LOGF_LEVEL(level) " %s: " fmt "\n",              \
                        cur_ts, logger.channel().c_str() __VA_OPT__(,) __VA_ARGS__)

#include "logging_wrapper/logging.h"

struct printf_logger final
{
    int operator()(const char* p_fmt, ...)
    {
        va_list args;
        va_start(args, p_fmt);
        const int rc = vprintf(p_fmt, args);
        va_end(args);
        return rc;
    }
};

using clog_backend_t = printf_logger;
using logger_t = ::wstux::logging::logger<clog_backend_t>;

namespace wstux {
namespace logging {

template<> clog_backend_t make_logger<clog_backend_t>(const std::string&) { return clog_backend_t(); }

} // namespace logging
} // namespace wstux

int main()
{
    ::wstux::logging::manager::init();

    logger_t root_logger = ::wstux::logging::manager::get_logger<clog_backend_t>("Root");
    LOGF_INFO(root_logger, "Hello, world!");

    ::wstux::logging::manager::clear();
    return 0;
}
