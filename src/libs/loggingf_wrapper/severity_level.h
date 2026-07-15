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
/**
 *  \file   severity_level.h
 *  \brief  Definition of severity levels and formatting macros for the logger.
 *  \details    Contains numerical identifiers, string prefixes, and
 *      enumerations (enums) for managing message severity levels.
 *  \ingroup loggingf_wrapper_module
 */

#ifndef _LIBS_LOGGINGF_WRAPPER_SEVERITY_LEVEL_H_
#define _LIBS_LOGGINGF_WRAPPER_SEVERITY_LEVEL_H_

#if defined(__cplusplus)
extern "C" {
#endif

/**
 *  \name   Logging Level Identifiers
 *  \brief  Numerical constants defining log priority (from highest to lowest).
 *  \{
 */
#define LVL_EMERG       0 /**< System is unusable (Emergency) */
#define LVL_FATAL       1 /**< Critical error causing termination (Fatal) */
#define LVL_CRIT        2 /**< Critical condition (Critical) */
#define LVL_ERROR       3 /**< Runtime error (Error) */
#define LVL_WARN        4 /**< Warning (Warning) */
#define LVL_NOTICE      5 /**< Important notification during normal operation (Notice) */
#define LVL_INFO        6 /**< Informational message (Info) */
#define LVL_DEBUG       7 /**< Debugging information (Debug) */
#define LVL_TRACE       8 /**< Detailed trace of program execution steps (Trace) */
/** \} */

/**
 *  \name   Logging Level String Prefixes
 *  \brief  Internal macros specifying fixed-length text tags (7 characters) for output.
 *  \note   Intended solely for internal use.
 *  \{
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
/** \} */

/**
 *  \def    LOGF_LEVEL(level)
 *  \brief  Generates a string prefix for C-style logging.
 *  \param  level - numerical level suffix (from 0 to 8).
 *  \details    Substitutes the numerical level into the token-concatenation macro.
 */
#define LOGF_LEVEL(level)       _IMPL_LOGF_LVL_ ## level

/**
 *  \enum   lw_severity_level
 *  \brief  Strongly typed representation of log severity levels.
 *  \details    Binds internal numerical macros (LVL_*) to an enumeration for
 *      type safety in C++ code.
 */
enum lw_severity_level
{
    emerg   = LVL_EMERG,  /**< System is unusable */
    fatal   = LVL_FATAL,  /**< Critical error */
    crit    = LVL_CRIT,   /**< Critical condition */
    error   = LVL_ERROR,  /**< Runtime error */
    warning = LVL_WARN,   /**< Warning */
    notice  = LVL_NOTICE, /**< Important notification */
    info    = LVL_INFO,   /**< Informational message */
    debug   = LVL_DEBUG,  /**< Debugging message */
    trace   = LVL_TRACE   /**< Execution trace */
};

/**
 *  \def    SEVERITY_LEVEL(level)
 *  \brief  Type casting to enum severity_level.
 *  \param  level - input value of the logging level.
 *  \details The macro performs an explicit conversion of the passed value (a
 *      number or a macro) to the ::wstux::logging::severity_level type for
 *      subsequent checks.
 */
#define SEVERITY_LEVEL(level)  ((enum lw_severity_level)level)

#if defined(__cplusplus)
}
#endif

#endif /* _LIBS_LOGGINGF_WRAPPER_SEVERITY_LEVEL_H_ */
