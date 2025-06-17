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

#ifndef _LIBS_LOGGINGF_WRAPPER_LOGGING_H_
#define _LIBS_LOGGINGF_WRAPPER_LOGGING_H_

#include "loggingf_wrapper/manager.h"
#include "loggingf_wrapper/severity_level.h"

#if defined(LOGGINGF_WRAPPER_IMPL)
    /**
     *  \brief  User's logging implementation.
     *
     *  Calling a custom logging implementation. The logger and the required
     *  logging level are passed to the custom logging implementation.
     *
     *  \code
     *  #define LOGGINGF_WRAPPER_IMPL(logger, level, fmt, ...)                 \
     *      char cur_ts[24];                                                   \
     *      lw_timestamp(cur_ts, 24);                                          \
     *      logger->p_logger("%s " LOGF_LEVEL(level) " %s: " fmt "\n",         \
     *                       cur_ts, logger->channel __VA_OPT__(,) __VA_ARGS__)
     *
     *  #include "loggingf_wrapper/logging.h"
     *  \endcode
     */
    #define _LOGGINGF_WRAPPER_IMPL(logger, level, fmt, ...)                 \
        LOGGINGF_WRAPPER_IMPL(logger, level, fmt, __VA_ARGS__)
#else
    /**
     *  \brief  Default logging implementation.
     *
     *  By default, a log line is generated in the format:
     *
     *      ****-**-** **:**:**.*** [S_LVL] Channel: message
     *
     *  Where,
     *  ****-**-** **:**:**.*** - time stamp in format yyyy-mm-dd HH:MM:SS.mmm;
     *  S_LVL - severity level;
     *  Channel - channel name;
     *  message - user's message.
     */
    #define _LOGGINGF_WRAPPER_IMPL(logger, level, fmt, ...)                 \
        char cur_ts[24];                                                    \
        lw_timestamp(cur_ts, 24);                                           \
        logger->p_logger("%s " LOGF_LEVEL(level) " %s: " fmt "\n",          \
                         cur_ts, logger->channel __VA_OPT__(,) __VA_ARGS__)
#endif

/**
 *  \brief  The macro writes a record to the log.
 *  \param  logger - C-style logger for recording.
 *  \param  level - the required logging level.
 *  \param  fmt - message format.
 *
 *  \attention  The message is evaluated only if the current application logging
 *      level satisfies the required logging level.
 *
 *  \details    The macro first checks the global logging level. If the check is
 *      positive, the channel logging level is checked. After the channel logging
 *      level is successfully checked, the message is transferred to the logger.
 */
#define _LOGF(logger, level, fmt, ...)                                      \
    do {                                                                    \
        if (! lw_can_log(level) || ! lw_can_channel_log(logger, level)) {   \
            break;                                                          \
        }                                                                   \
        _LOGGINGF_WRAPPER_IMPL(logger, level, fmt, __VA_ARGS__);            \
    }                                                                       \
    while (0)

/**
 *  \brief  The macro writes a record to the log.
 *  \param  logger - C-style logger for recording.
 *  \param  fmt - message format.
 */
#define LOGF_EMERG(logger, fmt, ...)        _LOGF(logger, LVL_EMERG,  fmt, __VA_ARGS__)
#define LOGF_FATAL(logger, fmt, ...)        _LOGF(logger, LVL_FATAL,  fmt, __VA_ARGS__)
#define LOGF_CRIT(logger, fmt, ...)         _LOGF(logger, LVL_CRIT,   fmt, __VA_ARGS__)
#define LOGF_ERROR(logger, fmt, ...)        _LOGF(logger, LVL_ERROR,  fmt, __VA_ARGS__)
#define LOGF_WARN(logger, fmt, ...)         _LOGF(logger, LVL_WARN,   fmt, __VA_ARGS__)
#define LOGF_NOTICE(logger, fmt, ...)       _LOGF(logger, LVL_NOTICE, fmt, __VA_ARGS__)
#define LOGF_INFO(logger, fmt, ...)         _LOGF(logger, LVL_INFO,   fmt, __VA_ARGS__)
#define LOGF_DEBUG(logger, fmt, ...)        _LOGF(logger, LVL_DEBUG,  fmt, __VA_ARGS__)
#define LOGF_TRACE(logger, fmt, ...)        _LOGF(logger, LVL_TRACE,  fmt, __VA_ARGS__)

#endif /* _LIBS_LOGGINGF_WRAPPER_LOGGING_H_ */

