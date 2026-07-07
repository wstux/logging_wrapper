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
 *  \file
 *  \brief  Logging manager for C with support for channels and severity levels.
 *  \ingroup loggingf_wrapper_module
 */

#ifndef _LIBS_LOGGINGF_WRAPPER_MANAGER_H_
#define _LIBS_LOGGINGF_WRAPPER_MANAGER_H_

#include <signal.h>
#include <stdbool.h>
#include <stdint.h>

#include "loggingf_wrapper/severity_level.h"

#if ! defined(LOG_CHANNEL_LEN)
    /** Maximum length of a log channel name (including the null terminator) */
    #define LOG_CHANNEL_LEN     16
#endif

#if defined(__cplusplus)
extern "C" {
#endif

/**
 *  \enum   lw_logging_policy
 *  \brief  Enumeration of memory allocation policies for channels.
 */
enum lw_logging_policy
{
    dynamic_size, /**< Dynamic memory allocation when creating new channels. */
    fixed_size    /**< Fixed pool of channels specified during initialization. */
};

/**
 *  \brief  Signature of the logging function (similar to printf).
 *  \param  format - format string.
 *  \param  ... - format arguments.
 *  \return Number of characters written or a negative value on error.
 */
typedef int (*lw_loggerf_fn_t)(const char*, ...);

typedef enum lw_logging_policy      lw_logging_policy_t;
typedef enum lw_severity_level      lw_severity_level_t;

/**
 *  \brief  Structure of a specific logger (channel).
 */
struct lw_loggerf
{
    lw_loggerf_fn_t p_logger;      /**< Pointer to the log output function. */
    volatile sig_atomic_t level;   /**< Current channel severity level (thread-safe/atomic). */
    char channel[LOG_CHANNEL_LEN]; /**< Channel name. */
};

/** Pointer to a constant logger structure. */
typedef const struct lw_loggerf*    lw_loggerf_t;

/**
 *  \brief  Checks if logging is allowed for the global level.
 *  \param  lvl - the severity level to check.
 *  \return true if a log of this level should be written, false otherwise.
 */
bool lw_can_log(int lvl);

/**
 *  \brief  Checks if logging is allowed for a specific channel.
 *  \param  p_logger - pointer to the channel logger.
 *  \param  lvl - the severity level to check.
 *  \return true if the channel is active for this level, false otherwise.
 */
bool lw_can_channel_log(lw_loggerf_t p_logger, int lvl);

/**
 *  \brief  Returns a logger by its channel name.
 *  \param  channel - channel name.
 *  \return Pointer to the logger, or NULL if the channel is not found or cannot
 *      be created.
 *
 *  \details If the channel does not exist and the policy allows, it will be created.
 */
lw_loggerf_t lw_get_logger(const char* channel);

/**
 *  \brief  Returns a logger by its channel name or creates it with a default level.
 *  \param  channel - channel name.
 *  \param  dfl_lvl - default severity level for the new channel.
 *  \return Pointer to the logger, or NULL.
 */
lw_loggerf_t lw_get_logger_dfl(const char* channel, lw_severity_level_t dfl_lvl);

/**
 *  \brief  Returns the current global severity level.
 *  \return Current level of type \ref lw_severity_level_t.
 */
lw_severity_level_t lw_global_level(void);

/**
 *  \brief  Initializes the logging manager.
 *  \param  p_logger_fn - base output function (e.g., printf or a custom function).
 *  \param  policy - memory management policy (dynamic or fixed).
 *  \param  channel_count - maximum number of channels (relevant for the \ref fixed_size policy).
 *  \param  dfl_lvl - default global severity level.
 *  \param  p_root_ch - name of the root (main) logging channel.
 *  \return true if initialization is successful, false otherwise.
 *
 *  \code
 *  // Example of initialization and usage:
 *  #include <stdio.h>
 *  #include "logging.h"
 *
 *  int main(void)
 *  {
 *      // Initialize the manager with the vprintf/printf function
 *      if (! lw_init_logging(printf, dynamic_size, 0, LW_INFO, NULL)) {
 *          return 1;
 *      }
 *
 *      // Get a channel for the network module
 *      lw_loggerf_t root_logger = lw_get_logger("Root");
 *      LOGF_INFO(root_logger, "Hello, world!");
 *
 *      lw_deinit_logging();
 *      return 0;
 *  }
 *  \endcode
 */
bool lw_init_logging(lw_loggerf_fn_t p_logger_fn, lw_logging_policy_t policy,
                     size_t channel_count, lw_severity_level_t dfl_lvl,
                     const char* p_root_ch);

/**
 *  \brief  Releases the logging manager resources and deinitializes it.
 *  \return true upon successful closure, false otherwise.
 */
bool lw_deinit_logging(void);

/**
 *  \brief  Returns the root (main) logger created during initialization.
 *  \return Pointer to the root logger.
 */
lw_loggerf_t lw_root_logger(void);

/**
 *  \brief  Sets a new global severity level.
 *  \param  lvl - the new severity level.
 */
void lw_set_global_level(lw_severity_level_t lvl);

/**
 *  \brief  Sets an immutable global severity level.
 *  \param  lvl - the protected severity level.
 *
 *  \details    After calling this function, subsequent calls to
 *      \ref lw_set_global_level will be ignored.
 */
void lw_set_immutable_global_level(lw_severity_level_t lvl);

/**
 *  \brief  Sets the severity level for a specific channel.
 *  \param  channel - channel name.
 *  \param  lvl - new severity level for this channel.
 */
void lw_set_logger_level(const char* channel, lw_severity_level_t lvl);

/**
 *  \brief  Writes the current high-resolution time into a raw C-string buffer.
 *  \param  buf - pointer to the character array where the date/time will be written.
 *  \param  size - size limit of the buffer (at least 24 bytes recommended).
 *  \return Number of characters successfully written.
 */
int lw_timestamp(char* buf, size_t size);

#if defined(__cplusplus)
}
#endif

#endif /* _LIBS_LOGGINGF_WRAPPER_MANAGER_H_ */
