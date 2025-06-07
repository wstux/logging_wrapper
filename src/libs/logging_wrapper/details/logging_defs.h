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

#ifndef _LIBS_LOGGING_WRAPPER_LOGGING_DEFS_H_
#define _LIBS_LOGGING_WRAPPER_LOGGING_DEFS_H_

#include "logging_wrapper/manager.h"
#include "logging_wrapper/severity_level.h"

#if defined(LOGGING_WRAPPER_IMPL)
    #define _LOGGING_WRAPPER_IMPL(logger, level)                            \
        LOGGING_WRAPPER_IMPL(logger.get_logger(), level)
#else
    #define _LOGGING_WRAPPER_IMPL(logger, level)                            \
        logger.get_logger() << ::wstux::logging::details::timestamp() << " "\
                            << _LOG_LEVEL(level) << " " << logger.channel() \
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

#endif /* _LIBS_LOGGING_WRAPPER_LOGGING_DEFS_H_ */

