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

#include <boost/date_time/posix_time/ptime.hpp>
#include <boost/log/attributes.hpp>
#include <boost/log/attributes/attribute_name.hpp>
#include <boost/log/attributes/clock.hpp>
#include <boost/log/attributes/current_thread_id.hpp>
#include <boost/log/expressions.hpp>
#include <boost/log/sinks.hpp>
#include <boost/log/sources/record_ostream.hpp>
#include <boost/log/sources/severity_channel_logger.hpp>
#include <boost/log/support/date_time.hpp>
#include <boost/log/utility/setup/common_attributes.hpp>
#include <boost/log/utility/setup/console.hpp>
#include <boost/log/utility/setup/filter_parser.hpp>
#include <boost/log/utility/setup/formatter_parser.hpp>
#include <boost/log/utility/setup/from_settings.hpp>

#define LOGGING_WRAPPER_IMPL(logger, level)                                    \
    BOOST_LOG_SEV(logger.get_logger(), SEVERITY_LEVEL(level))

#include "logging_wrapper/logging.h"

namespace wstux {
namespace logging {

using boost_logger_t = boost::log::sources::severity_channel_logger_mt<::wstux::logging::severity_level>;

template<> boost_logger_t make_logger<boost_logger_t>(const std::string& ch)
{
    return boost_logger_t(boost::log::keywords::channel = ch);
}

template<typename TChar, typename TTraits>
std::basic_ostream<TChar, TTraits>& operator<<(std::basic_ostream<TChar, TTraits>& stream, severity_level lvl)
{
    static const std::array<const char*, 9> severities = {
        "EMERG",
        "FATAL",
        "CRIT",
        "ERROR",
        "WARN",
        "NOTIC",
        "INFO",
        "DEBUG",
        "TRACE"
    };

    assert(severities.size() > static_cast<std::size_t>(lvl) && "severity level to stream conversion failed - out of bounds");
    stream << std::setw(5) << std::left << severities[static_cast<std::size_t>(lvl)];
    return stream;
}

BOOST_LOG_ATTRIBUTE_KEYWORD(severity, "Severity", severity_level)
BOOST_LOG_ATTRIBUTE_KEYWORD(channel, "Channel", std::string)

} // namespace logging
} // namespace wstux

void init_logging()
{
    namespace attrs = boost::log::attributes;
    namespace exprs = boost::log::expressions;
    namespace keywords = boost::log::keywords;

    boost::log::register_simple_formatter_factory<::wstux::logging::severity_level, char>("Severity");
    boost::log::add_console_log
    (
        std::clog,
        keywords::format =
        (
            exprs::stream
                << exprs::attr<unsigned>("LineID")
                << " " << exprs::format_date_time<boost::posix_time::ptime>("TimeStamp", "%Y.%m.%d %H:%M:%S.%f")
                << " (" << exprs::attr<attrs::current_thread_id::value_type>("ThreadID") << ")"
                << " [" << ::wstux::logging::severity << "]"
                << ": <" << ::wstux::logging::channel << ">"
                << " " << exprs::message
        )
    );

    boost::log::add_common_attributes();
}


using logger_t = ::wstux::logging::logger<::wstux::logging::boost_logger_t>;

int main()
{
    ::wstux::logging::manager::init(::wstux::logging::severity_level::debug, init_logging);

    logger_t root_logger = ::wstux::logging::manager::get_logger<logger_t>("Root");
    LOG_NOTICE(root_logger, "Hello, world!");
    LOG_INFO(root_logger, "Hello, world!");

    ::wstux::logging::manager::deinit();
    return 0;
}
