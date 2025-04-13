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

#ifndef _LIBS_LOGGINGF_WRAPPER_SEVERITY_LEVEL_H_
#define _LIBS_LOGGINGF_WRAPPER_SEVERITY_LEVEL_H_

#if defined(__cplusplus)
extern "C" {
#endif

/* 
 * Implementations of log levels.
 * Supported levels:
 * - EMERG  - the system is unusable;
 * - FATAL  - actions that must be taken care of immediately;
 * - CRIT   - critical conditions;
 * - ERROR  - non-critical error conditions;
 * - WARN   - warning conditions that should be taken care of;
 * - NOTICE - normal, but significant events;
 * - INFO   - informational messages that require no action;
 * - DEBUG  - debugging messages, output if the developer enabled debugging at
 *            compile time;
 * - TRACE  - the most detailed level of logging within the spectrum of log
 *            levels that developers can use.
 */
#define LVL_EMERG       0
#define LVL_FATAL       1
#define LVL_CRIT        2
#define LVL_ERROR       3
#define LVL_WARN        4
#define LVL_NOTICE      5
#define LVL_INFO        6
#define LVL_DEBUG       7
#define LVL_TRACE       8

/**
 */
#define _IMPL_LOGF_LVL_0        "[EMERG]"
#define _IMPL_LOGF_LVL_1        "[FATAL]"
#define _IMPL_LOGF_LVL_2        "[CRIT ]"
#define _IMPL_LOGF_LVL_3        "[ERROR]"
#define _IMPL_LOGF_LVL_4        "[WARN ]"
#define _IMPL_LOGF_LVL_5        "[NOTIC]"
#define _IMPL_LOGF_LVL_6        "[INFO ]"
#define _IMPL_LOGF_LVL_7        "[DEBUG]"
#define _IMPL_LOGF_LVL_8        "[TRACE]"

#define _LOGF_LEVEL(level)      _IMPL_LOGF_LVL_ ## level

enum severity_level
{
    emerg   = 0,
    fatal   = 1,
    crit    = 2,
    error   = 3,
    warning = 4,
    notice  = 5,
    info    = 6,
    debug   = 7,
    trace   = 8
};

#if defined(__cplusplus)
}
#endif

#endif /* _LIBS_LOGGINGF_WRAPPER_SEVERITY_LEVEL_H_ */
