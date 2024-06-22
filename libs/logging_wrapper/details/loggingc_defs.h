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

#ifndef _LOGGING_WRAPPER_LOGGING_WRAPPER_LOGGINGC_DEFS_H_
#define _LOGGING_WRAPPER_LOGGING_WRAPPER_LOGGINGC_DEFS_H_

#include "logging_wrapper/logging_manager.h"
#include "logging_wrapper/severity_level.h"

#if ! defined(LOGGINGC_WRAPPER_IMPL)
    #define LOGGINGC_WRAPPER_IMPL(logger, level)                            \
        logger << ::wstux::logging::details::timestamp() << " " << _LOGC_LEVEL(level) << " " 
#endif

#define _LOGC(logger, level, VARS)                                          \
    do {                                                                    \
        if (! ::wstux::logging::manager::cal_log(_LOG_LEVEL(level)) ||      \
            ! logger.can_log(_LOG_LEVEL(level))) {                          \
            break;                                                          \
        }                                                                   \
        LOGGINGC_WRAPPER_IMPL(logger.get_logger(), level) << VARS << std::endl; \
    }                                                                       \
    while (0)

#endif /* _LOGGING_WRAPPER_LOGGING_WRAPPER_LOGGINGC_DEFS_H_ */

