/*
 * logging_wrapper
 * Copyright (C) 2026  Chistyakov Alexander
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
#include <memory>
#include <vector>

#include <quill/Backend.h>
#include <quill/Frontend.h>
#include <quill/Logger.h>
#include <quill/LogFunctions.h>
#include <quill/sinks/ConsoleSink.h>

/*#define LOGGINGF_WRAPPER_IMPL(logger, level, fmt, ...)                         \
    QUILL_LOG_WITH_LEVEL(logger.get_logger(),                                  \
                         ::wstux::logging::to_qlevel(level),              \
                         fmt, ##__VA_ARGS__) */
#define LOGGINGF_WRAPPER_IMPL(logger, level, fmt, ...)                         \
    quill::log((logger).get_logger(),                                          \
               "",                                                             \
               ::wstux::logging::to_qlevel(SEVERITY_LEVEL(level)),             \
               fmt,                                                            \
               quill::SourceLocation::current()                                \
               __VA_OPT__(,) __VA_ARGS__)

#include "logging_wrapper/logging.h"

namespace wstux {
namespace logging {

using quill_logger_t = quill::Logger*;

constexpr quill::LogLevel to_qlevel(severity_level lvl)
{
    switch (lvl)
    {
        case severity_level::emerg:     return quill::LogLevel::Critical;
        case severity_level::fatal:     return quill::LogLevel::Critical;
        case severity_level::crit:      return quill::LogLevel::Critical;
        case severity_level::error:     return quill::LogLevel::Error;
        case severity_level::warning:   return quill::LogLevel::Warning;
        case severity_level::notice:    return quill::LogLevel::Notice;
        case severity_level::info:      return quill::LogLevel::Info;
        case severity_level::debug:     return quill::LogLevel::Debug;
        case severity_level::trace:     return quill::LogLevel::TraceL1;
        default:                        return quill::LogLevel::Info;
    }
}

struct logger_config final
{
    logger_config()
        : options("%(time) (%(thread_id)) [%(log_level:<5)]: <%(logger)> %(message)", "%Y.%m.%d %H:%M:%S.%Qus", quill::Timezone::LocalTime)
    {}

    quill::PatternFormatterOptions options;

    std::vector<std::shared_ptr<quill::Sink>> sinks;
};

logger_config g_log_config;

template<> quill_logger_t make_logger<quill_logger_t>(const std::string& ch)
{
    std::shared_ptr<quill::Sink> sink =
        quill::Frontend::create_or_get_sink<quill::ConsoleSink>("stdout_sink");

    return quill::Frontend::create_or_get_logger(ch, g_log_config.sinks, g_log_config.options);
}

} // namespace logging
} // namespace wstux

void init_logging()
{
    quill::BackendOptions backend_options;
    quill::Backend::start(backend_options);

    quill::ConsoleSinkConfig console_config;
    console_config.set_colour_mode(quill::ConsoleSinkConfig::ColourMode::Never);
    std::shared_ptr<quill::Sink> console_sink =
        quill::Frontend::create_or_get_sink<quill::ConsoleSink>("stdout_sink", console_config);

    ::wstux::logging::g_log_config.sinks = {console_sink};
}

void deinit_logging()
{
    quill::Backend::stop();
}

using logger_t = ::wstux::logging::logger<::wstux::logging::quill_logger_t>;

int main()
{
    ::wstux::logging::manager::init(::wstux::logging::severity_level::debug, init_logging);

    logger_t root_logger = ::wstux::logging::manager::get_logger<logger_t>("Root");
    LOGF_NOTICE(root_logger, "Hello, world!");
    LOGF_INFO(root_logger, "Hello, world!");
    LOGF_ERROR(root_logger, "Failed with code: {} and status: {}", 404, "Not Found");

    logger_t ch_logger = ::wstux::logging::manager::get_logger<logger_t>("Channel");
    LOGF_NOTICE(ch_logger, "Hello, world!");
    LOGF_INFO(ch_logger, "Hello, world!");
    LOGF_ERROR(ch_logger, "Failed with code: {} and status: {}", 404, "Not Found");

    ::wstux::logging::manager::deinit();
    deinit_logging();
    return 0;
}
