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
/**
 *  \file
 *  \brief  Logging wrapper API.
 *  \ingroup logging_wrapper_module
 */

#ifndef _LIBS_LOGGING_WRAPPER_LOGGING_H_
#define _LIBS_LOGGING_WRAPPER_LOGGING_H_

#include "logging_wrapper/manager.h"
#include "logging_wrapper/severity_level.h"

/*******************************************************************************
 *  Logging for loggers in C-style
 ******************************************************************************/

#if defined(LOGGINGF_WRAPPER_IMPL)
    /**
     *  \def    _LOGGINGF_WRAPPER_IMPL(logger, level, fmt, ...)
     *  \brief  Internal macro to invoke the custom C-style logging implementation.
     *  \param  logger - logger object providing the channel and write method.
     *  \param  level - severity level for the log entry.
     *  \param  fmt - format string (printf-style).
     *  \param  ... - variadic arguments for the format string.
     *
     *  \details    Calling a custom logging implementation. The logger and the
     *      required logging level are passed to the custom logging implementation.
     *
     *  \code
     *  #define LOGGINGF_WRAPPER_IMPL(logger, level, fmt, ...)                  \
     *      char cur_ts[24];                                                    \
     *      ::wstux::logging::manager::timestamp(cur_ts, 24);                   \
     *      logger.get_logger()("%s " LOGF_LEVEL(level) " %s: " fmt "\n",       \
     *                          cur_ts, logger.channel().c_str() __VA_OPT__(,) __VA_ARGS__)
     *
     *  #include <logging_wrapper/logging.h>
     *  \endcode
     */
    #define _LOGGINGF_WRAPPER_IMPL(logger, level, fmt, ...)                 \
        LOGGINGF_WRAPPER_IMPL(logger, level, fmt, __VA_ARGS__)
#else
    /**
     *  \def    _LOGGINGF_WRAPPER_IMPL(logger, level, fmt, ...)
     *  \brief  Default implementation of formatted C-style logging.
     *  \param  logger - logger object providing the channel and write method.
     *  \param  level - severity level for the log entry.
     *  \param  fmt - format string (printf-style).
     *  \param  ... - variadic arguments for the format string.
     *
     *  \details    Generates a log string in the standard format:
     *          `YYYY-MM-DD HH:MM:SS.mmm [S_LVL] Channel: message`
     *
     *  Where,
     *  YYYY-MM-DD HH:MM:SS.mmm - time stamp in format yyyy-mm-dd HH:MM:SS.mmm;
     *  S_LVL - severity level;
     *  Channel - channel name;
     *  message - user's message.
     */
    #define _LOGGINGF_WRAPPER_IMPL(logger, level, fmt, ...)                 \
        char cur_ts[24];                                                    \
        ::wstux::logging::manager::timestamp(cur_ts, 24);                   \
        logger.get_logger()("%s " LOGF_LEVEL(level) " %s: " fmt "\n",       \
                            cur_ts, logger.channel().c_str() __VA_OPT__(,) __VA_ARGS__)
#endif

/**
 *  \def    _LOGF(logger, level, fmt, ...)
 *  \brief  Base filtering and recording macro for the C-style logger.
 *  \param  logger - logger object for recording.
 *  \param  level - required logging level.
 *  \param  fmt - format string.
 *  \param  ... - formatting arguments.
 *
 *  \attention  Log argument expressions are evaluated **only** if the current
 *      logging level permits recording. If logging is disabled, arguments are
 *      not evaluated (lazy evaluation).
 *
 *  \details    First checks the global logging level, then the level of the
 *      specific channel. If the checks pass, evaluates the arguments and forwards
 *      them to the logger implementation.
 */
#define _LOGF(logger, level, fmt, ...)                                      \
    do {                                                                    \
        if (! ::wstux::logging::manager::cal_log(SEVERITY_LEVEL(level)) ||  \
            ! logger.can_log(SEVERITY_LEVEL(level))) {                      \
            break;                                                          \
        }                                                                   \
        _LOGGINGF_WRAPPER_IMPL(logger, level, fmt, __VA_ARGS__);            \
    }                                                                       \
    while (0)


/*******************************************************************************
 *  Logging for loggers in CPP-style
 ******************************************************************************/

#if defined(LOGGING_WRAPPER_IMPL)
    /**
     *  \def    _LOGGING_WRAPPER_IMPL(logger, level)
     *  \brief  Internal macro to invoke the custom CPP-style logging implementation.
     *  \param  logger - logger object providing the channel and write stream.
     *  \param  level - severity level for the log entry.
     *
     *  \details    Called automatically if the user has defined their own
     *      LOGGING_WRAPPER_IMPL macro.
     *
     *  \code
     *  #define LOGGING_WRAPPER_IMPL(logger, level)                             \
     *      logger.get_logger() << ::wstux::logging::manager::timestamp() << " "\
     *                          << LOG_LEVEL(level) << " " << logger.channel() << ": "
     *
     *  #include <logging_wrapper/logging.h>
     *  \endcode
     */
    #define _LOGGING_WRAPPER_IMPL(logger, level)                            \
        LOGGING_WRAPPER_IMPL(logger, level)
#else
    /**
     *  \def    _LOGGING_WRAPPER_IMPL(logger, level)
     *  \brief  Default implementation of stream-based CPP-style logging.
     *  \param  logger - logger object providing the channel and write stream.
     *  \param  level - severity level for the log entry.
     *
     *  \details    Formats and returns the beginning of the log string stream
     *      in the format:
     *          `YYYY-MM-DD HH:MM:SS.mmm [S_LVL] Channel: `
     *
     *  Where,
     *  YYYY-MM-DD HH:MM:SS.mmm - time stamp in format yyyy-mm-dd HH:MM:SS.mmm;
     *  S_LVL - severity level;
     *  Channel - channel name.
     */
    #define _LOGGING_WRAPPER_IMPL(logger, level)                            \
        logger.get_logger() << ::wstux::logging::manager::timestamp() << " "\
                            << LOG_LEVEL(level) << " " << logger.channel()  \
                            << ": "
#endif

/**
 *  \def    _LOG(logger, level, VARS)
 *  \brief  Base filtering and recording macro for the CPP-style (stream-based) logger.
 *  \param  logger - logger object for recording.
 *  \param  level - required logging level.
 *  \param  VARS - expression or data chain to output to the stream (e.g., `x << " text " << y`).
 *
 *  \attention  The VARS expression is evaluated **only** if the current logging
 *      level permits recording (lazy evaluation).
 *
 *  \details    details First checks the global logging level, then the level of
 *      the specific channel. Upon success, outputs the VARS expression into the
 *      logger stream and terminates the line with std::endl.
 */
#define _LOG(logger, level, VARS)                                           \
    do {                                                                    \
        if (! ::wstux::logging::manager::cal_log(SEVERITY_LEVEL(level)) ||  \
            ! logger.can_log(SEVERITY_LEVEL(level))) {                      \
            break;                                                          \
        }                                                                   \
        _LOGGING_WRAPPER_IMPL(logger, level) << VARS << std::endl;          \
    }                                                                       \
    while (0)

/**
 *  \defgroup   FormattedCppLogging Formatted Cpp Logging API (printf-style)
 *  \brief  Macros for recording log messages of various severity levels. Macros
 *      that accept a format string and a variable number of arguments.
 *
 * **List of logging macros:**
 * - \ref LOGF_EMERG()  - critical system error (Emergency)
 * - \ref LOGF_FATAL()  - fatal error (Fatal)
 * - \ref LOGF_CRIT()   - critical condition (Critical)
 * - \ref LOGF_ERROR()  - standard error (Error)
 * - \ref LOGF_WARN()   - warning (Warning)
 * - \ref LOGF_NOTICE() - important notification (Notice)
 * - \ref LOGF_INFO()   - informational message (Info)
 * - \ref LOGF_DEBUG()  - debugging information (Debug)
 * - \ref LOGF_TRACE()  - trace logging (Trace)
 *
 *  \todo   Add the ability to completely exclude calls to the \ref LOGF_DEBUG
 *      and \ref LOGF_TRACE levels from the binary file when compiling in the
 *      Release configuration.
 *
 *  \{
 */

/**
 *  \brief  Logs a critical system error (Emergency).
 *  \param  logger - logger object for recording.
 *  \param  fmt - format string.
 *  \param  ... - formatting arguments.
 *  \details    Used when the system is completely unusable.
 *
 *  \code
 *  LOGF_EMERG(logger, "Kernel panic: out of memory. Free bytes: %lu", free_mem);
 *  \endcode
 */
#define LOGF_EMERG(logger, fmt, ...)        _LOGF(logger, LVL_EMERG,  fmt, __VA_ARGS__)

/**
 *  \brief  Logs a fatal error (Fatal).
 *  \param  logger - logger object for recording.
 *  \param  fmt - format string.
 *  \param  ... - formatting arguments.
 *  \details    Leads to abnormal termination of the thread or application.
 *
 *  \code
 *  LOGF_FATAL(logger, "Database connection failed! Code: %d", errno);
 *  \endcode
 */
#define LOGF_FATAL(logger, fmt, ...)        _LOGF(logger, LVL_FATAL,  fmt, __VA_ARGS__)

/**
 *  \brief  Logs a critical condition (Critical).
 *  \param  logger - logger object for recording.
 *  \param  fmt - format string.
 *  \param  ... - formatting arguments.
 *  \details    Signals severe failures in key components.
 *
 *  \code
 *  LOGF_CRIT(logger, "Hardware failure on interface %s", iface_name);
 *  \endcode
 */
#define LOGF_CRIT(logger, fmt, ...)         _LOGF(logger, LVL_CRIT,   fmt, __VA_ARGS__)

/**
 *  \brief  Logs a standard error (Error).
 *  \param  logger - logger object for recording.
 *  \param  fmt - format string.
 *  \param  ... - formatting arguments.
 *  \details    The operation failed, but the system continues to operate.
 *
 *  \code
 *  LOGF_ERROR(logger, "Failed to open configuration file: %s", path);
 *  \endcode
 */
#define LOGF_ERROR(logger, fmt, ...)        _LOGF(logger, LVL_ERROR,  fmt, __VA_ARGS__)

/**
 *  \brief  Logs a warning (Warning).
 *  \param  logger - logger object for recording.
 *  \param  fmt - format string.
 *  \param  ... - formatting arguments.
 *  \details    Suspicious situations or non-critical deviations.
 *
 *  \code
 *  LOGF_WARN(logger, "Unauthorized login attempt from IP: %s", ip_addr);
 *  \endcode
 */
#define LOGF_WARN(logger, fmt, ...)         _LOGF(logger, LVL_WARN,   fmt, __VA_ARGS__)

/**
 *  \brief  Logs an important notification (Notice).
 *  \param  logger - logger object for recording.
 *  \param  fmt - format string.
 *  \param  ... - formatting arguments.
 *  \details    Special events during the normal operation of the system.
 *
 *  \code
 *  LOGF_NOTICE(logger, "Configuration reloaded successfully. Workers: %d", num_workers);
 *  \endcode
 */
#define LOGF_NOTICE(logger, fmt, ...)       _LOGF(logger, LVL_NOTICE, fmt, __VA_ARGS__)

/**
 *  \brief  Logs an informational message (Info).
 *  \param  logger - logger object for recording.
 *  \param  fmt - format string.
 *  \param  ... - formatting arguments.
 *  \details    Routine messages about key stages of the program execution.
 *
 *  \code
 *  LOGF_INFO(logger, "Application started on port %d", port);
 *  \endcode
 */
#define LOGF_INFO(logger, fmt, ...)         _LOGF(logger, LVL_INFO,   fmt, __VA_ARGS__)

/**
 *  \brief  Logs debugging information (Debug).
 *  \param  logger - logger object for recording.
 *  \param  fmt - format string.
 *  \param  ... - formatting arguments.
 *  \details    Detailed data for developers during testing.
 *
 *  \code
 *  LOGF_DEBUG(logger, "Parsed token '%s', length: %zu", token, len);
 *  \endcode
 */
#define LOGF_DEBUG(logger, fmt, ...)        _LOGF(logger, LVL_DEBUG,  fmt, __VA_ARGS__)

/**
 *  \brief  Trace logging (Trace).
 *  \param  logger - logger object for recording.
 *  \param  fmt - format string.
 *  \param  ... - formatting arguments.
 *  \details    Maximum detailed output, including loop steps and function calls.
 *
 *  \code
 *  LOGF_TRACE(logger, "Entering function read_bytes(fd=%d, buf=%p)", fd, buf);
 *  \endcode
 */
#define LOGF_TRACE(logger, fmt, ...)        _LOGF(logger, LVL_TRACE,  fmt, __VA_ARGS__)

/** \}*/

/**
 *  \defgroup   StreamedLogging Streamed Logging API (Key-Value)
 *  \brief  Macros for recording log messages of various severity levels. Macros
 *      that accept a list of variables or streamed arguments.
 *
 * **List of logging macros:**
 * - \ref LOG_EMERG()   - critical system error (Emergency)
 * - \ref LOG_FATAL()   - fatal error (Fatal)
 * - \ref LOG_CRIT()    - critical condition (Critical)
 * - \ref LOG_ERROR()   - standard error (Error)
 * - \ref LOG_WARN()    - warning (Warning)
 * - \ref LOG_NOTICE()  - important notification (Notice)
 * - \ref LOG_INFO()    - informational message (Info)
 * - \ref LOG_DEBUG()   - debugging information (Debug)
 * - \ref LOG_TRACE()   - trace logging (Trace)
 *
 *  \todo   Add the ability to completely exclude calls to the \ref LOG_DEBUG
 *      and \ref LOG_TRACE levels from the binary file when compiling in the
 *      Release configuration.
 *
 *  \{
 */

/**
 *  \brief  Logs a critical system error (Emergency).
 *  \param  logger - logger object for recording.
 *  \param  VARS - expression or data chain to output to the stream.
 *  \details    Used when the system is completely unusable.
 *
 *  \code
 *  LOG_EMERG(logger, "Kernel panic: out of memory. Free bytes: " << free_mem);
 *  \endcode
 */
#define LOG_EMERG(logger, VARS)             _LOG(logger, LVL_EMERG,  VARS)

/**
 *  \brief Logs a fatal error (Fatal).
 *  \param  logger - logger object for recording.
 *  \param  VARS - expression or data chain to output to the stream.
 *  \details    Leads to abnormal termination of the thread or application.
 *
 *  \code
 *  LOG_FATAL(logger, "Database connection failed! Code: " << errno);
 *  \endcode
 */
#define LOG_FATAL(logger, VARS)             _LOG(logger, LVL_FATAL,  VARS)

/**
 *  \brief  Logs a critical condition (Critical).
 *  \param  logger - logger object for recording.
 *  \param  VARS - expression or data chain to output to the stream.
 *  \details    Signals severe failures in key components.
 *
 *  \code
 *  LOG_CRIT(logger, "Hardware failure on interface " << iface_name);
 *  \endcode
 */
#define LOG_CRIT(logger, VARS)              _LOG(logger, LVL_CRIT,   VARS)

/**
 *  \brief  Logs a standard error (Error).
 *  \param  logger - logger object for recording.
 *  \param  VARS - expression or data chain to output to the stream.
 *  \details    The operation failed, but the system continues to operate.
 *
 *  \code
 *  LOG_ERROR(logger, "Failed to open configuration file: " << path);
 *  \endcode
 */
#define LOG_ERROR(logger, VARS)             _LOG(logger, LVL_ERROR,  VARS)

/**
 *  \brief  Logs a warning (Warning).
 *  \param  logger - logger object for recording.
 *  \param  VARS - expression or data chain to output to the stream.
 *  \details    Suspicious situations or non-critical deviations.
 *
 *  \code
 *  LOG_WARN(logger, "Unauthorized login attempt from IP: " << ip_addr);
 *  \endcode
 */
#define LOG_WARN(logger, VARS)              _LOG(logger, LVL_WARN,   VARS)

/**
 *  \brief  Logs an important notification (Notice).
 *  \param  logger - logger object for recording.
 *  \param  VARS - expression or data chain to output to the stream.
 *  \details    Special events during the normal operation of the system.
 *
 *  \code
 *  LOG_NOTICE(logger, "Configuration reloaded successfully. Workers: " << num_workers);
 *  \endcode
 */
#define LOG_NOTICE(logger, VARS)            _LOG(logger, LVL_NOTICE, VARS)

/**
 *  \brief  Logs an informational message (Info).
 *  \param  logger - logger object for recording.
 *  \param  VARS - expression or data chain to output to the stream.
 *  \details    Routine messages about key stages of the program execution.
 *
 *  \code
 *  LOG_INFO(logger, "Application started on port " << port);
 *  \endcode
 */
#define LOG_INFO(logger, VARS)              _LOG(logger, LVL_INFO,   VARS)

/**
 *  \brief  Logs debugging information (Debug).
 *  \param  logger - logger object for recording.
 *  \param  VARS - expression or data chain to output to the stream.
 *  \details    Detailed data for developers during testing.
 *
 *  \code
 *  LOG_DEBUG(logger, "Parsed token '" << token << "', length: " << len);
 *  \endcode
 */
#define LOG_DEBUG(logger, VARS)             _LOG(logger, LVL_DEBUG,  VARS)

/**
 *  \brief  Trace logging (Trace).
 *  \param  logger - logger object for recording.
 *  \param  VARS - expression or data chain to output to the stream.
 *  \details    Maximum detailed output, including loop steps and function calls.
 *
 *  \code
 *  LOG_TRACE(logger, "Entering function read_bytes(fd=" << fd << ", buf=" << buf << ")");
 *  \endcode
 */
#define LOG_TRACE(logger, VARS)             _LOG(logger, LVL_TRACE,  VARS)

/** \}*/

#endif /* _LIBS_LOGGING_WRAPPER_LOGGING_H_ */
