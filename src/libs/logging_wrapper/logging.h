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

#ifndef _LIBS_LOGGING_WRAPPER_LOGGING_H_
#define _LIBS_LOGGING_WRAPPER_LOGGING_H_

#include "logging_wrapper/manager.h"
#include "logging_wrapper/severity_level.h"

/*
 * Logging for loggers in C-style
 */
#if defined(LOGGINGF_WRAPPER_IMPL)
    #define _LOGGINGF_WRAPPER_IMPL(logger, level, fmt, ...)                 \
        LOGGINGF_WRAPPER_IMPL(logger, level, fmt, __VA_ARGS__)
#else
    #define _LOGGINGF_WRAPPER_IMPL(logger, level, fmt, ...)                 \
        char cur_ts[24];                                                    \
        ::wstux::logging::manager::timestamp(cur_ts, 24);                   \
        logger.get_logger()("%s " LOGF_LEVEL(level) " %s: " fmt "\n",       \
                            cur_ts, logger.channel().c_str() __VA_OPT__(,) __VA_ARGS__)
#endif

#define _LOGF(logger, level, fmt, ...)                                      \
    do {                                                                    \
        if (! ::wstux::logging::manager::cal_log(_SEVERITY_LEVEL(level)) || \
            ! logger.can_log(_SEVERITY_LEVEL(level))) {                     \
            break;                                                          \
        }                                                                   \
        _LOGGINGF_WRAPPER_IMPL(logger, level, fmt, __VA_ARGS__);            \
    }                                                                       \
    while (0)

/*
 * Logging for loggers in CPP-style
 */
#if defined(LOGGING_WRAPPER_IMPL)
    #define _LOGGING_WRAPPER_IMPL(logger, level)                            \
        LOGGING_WRAPPER_IMPL(logger, level)
#else
    #define _LOGGING_WRAPPER_IMPL(logger, level)                            \
        logger.get_logger() << ::wstux::logging::manager::timestamp() << " "\
                            << LOG_LEVEL(level) << " " << logger.channel()  \
                            << ": "
#endif

#define _LOG(logger, level, VARS)                                           \
    do {                                                                    \
        if (! ::wstux::logging::manager::cal_log(_SEVERITY_LEVEL(level)) || \
            ! logger.can_log(_SEVERITY_LEVEL(level))) {                     \
            break;                                                          \
        }                                                                   \
        _LOGGING_WRAPPER_IMPL(logger, level) << VARS << std::endl;          \
    }                                                                       \
    while (0)

#define LOGF_EMERG(logger, fmt, ...)        _LOGF(logger, LVL_EMERG,  fmt, __VA_ARGS__)
#define LOGF_FATAL(logger, fmt, ...)        _LOGF(logger, LVL_FATAL,  fmt, __VA_ARGS__)
#define LOGF_CRIT(logger, fmt, ...)         _LOGF(logger, LVL_CRIT,   fmt, __VA_ARGS__)
#define LOGF_ERROR(logger, fmt, ...)        _LOGF(logger, LVL_ERROR,  fmt, __VA_ARGS__)
#define LOGF_WARN(logger, fmt, ...)         _LOGF(logger, LVL_WARN,   fmt, __VA_ARGS__)
#define LOGF_NOTICE(logger, fmt, ...)       _LOGF(logger, LVL_NOTICE, fmt, __VA_ARGS__)
#define LOGF_INFO(logger, fmt, ...)         _LOGF(logger, LVL_INFO,   fmt, __VA_ARGS__)
#define LOGF_DEBUG(logger, fmt, ...)        _LOGF(logger, LVL_DEBUG,  fmt, __VA_ARGS__)
#define LOGF_TRACE(logger, fmt, ...)        _LOGF(logger, LVL_TRACE,  fmt, __VA_ARGS__)

#define LOG_EMERG(logger, VARS)             _LOG(logger, LVL_EMERG,  VARS)
#define LOG_FATAL(logger, VARS)             _LOG(logger, LVL_FATAL,  VARS)
#define LOG_CRIT(logger, VARS)              _LOG(logger, LVL_CRIT,   VARS)
#define LOG_ERROR(logger, VARS)             _LOG(logger, LVL_ERROR,  VARS)
#define LOG_WARN(logger, VARS)              _LOG(logger, LVL_WARN,   VARS)
#define LOG_NOTICE(logger, VARS)            _LOG(logger, LVL_NOTICE, VARS)
#define LOG_INFO(logger, VARS)              _LOG(logger, LVL_INFO,   VARS)
#define LOG_DEBUG(logger, VARS)             _LOG(logger, LVL_DEBUG,  VARS)
#define LOG_TRACE(logger, VARS)             _LOG(logger, LVL_TRACE,  VARS)

#endif /* _LIBS_LOGGING_WRAPPER_LOGGING_H_ */

