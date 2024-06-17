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

#ifndef _LOGGING_WRAPPER_LOGGING_WRAPPER_LOGGING_H_
#define _LOGGING_WRAPPER_LOGGING_WRAPPER_LOGGING_H_

#include "logging_wrapper/severity_level.h"
#include "logging_wrapper/details/loggingc_defs.h"

#define LOG_EMERG(logger, VARS)             _LOGC(logger, LVL_EMERG,  VARS)
#define LOG_FATAL(logger, VARS)             _LOGC(logger, LVL_FATAL,  VARS)
#define LOG_CRIT(logger, VARS)              _LOGC(logger, LVL_CRIT,   VARS)
#define LOG_ERROR(logger, VARS)             _LOGC(logger, LVL_ERROR,  VARS)
#define LOG_WARN(logger, VARS)              _LOGC(logger, LVL_WARN,   VARS)
#define LOG_NOTICE(logger, VARS)            _LOGC(logger, LVL_NOTICE, VARS)
#define LOG_INFO(logger, VARS)              _LOGC(logger, LVL_INFO,   VARS)
#define LOG_DEBUG(logger, VARS)             _LOGC(logger, LVL_DEBUG,  VARS)
#define LOG_TRACE(logger, VARS)             _LOGC(logger, LVL_TRACE,  VARS)

#endif /* _LOGGING_WRAPPER_LOGGING_WRAPPER_LOGGING_H_ */

