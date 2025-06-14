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

#ifndef _LIBS_LOGGINGF_WRAPPER_MANAGER_H_
#define _LIBS_LOGGINGF_WRAPPER_MANAGER_H_

#include <signal.h>
#include <stdbool.h>
#include <stdint.h>

#include "loggingf_wrapper/severity_level.h"

#if ! defined(LOG_CHANNEL_LEN)
    #define LOG_CHANNEL_LEN     16
#endif

#if defined(__cplusplus)
extern "C" {
#endif

enum lw_logging_policy
{
    dynamic_size,
    fixed_size
};

typedef int	(*lw_loggerf_fn_t)(const char*, ...);
typedef enum lw_logging_policy      lw_logging_policy_t;
typedef enum lw_severity_level      lw_severity_level_t;

struct lw_loggerf
{
    lw_loggerf_fn_t p_logger;
    volatile sig_atomic_t level;
    char channel[LOG_CHANNEL_LEN];
    int _length;
};

typedef const struct lw_loggerf*    lw_loggerf_t;

bool lw_can_log(int lvl);

bool lw_can_channel_log(lw_loggerf_t p_logger, int lvl);

lw_loggerf_t lw_get_logger(const char* channel);

lw_severity_level_t lw_global_level(void);

bool lw_init_logging(lw_loggerf_fn_t p_logger_fn, lw_logging_policy_t policy, size_t channel_count,
                     lw_severity_level_t dfl_lvl, const char* p_root_ch);

bool lw_deinit_logging(void);

lw_loggerf_t lw_root_logger(void);

void lw_set_global_level(lw_severity_level_t lvl);

void lw_set_logger_level(const char* channel, lw_severity_level_t lvl);

int lw_timestamp(char* buf, size_t size);

#if defined(__cplusplus)
}
#endif

#endif /* _LIBS_LOGGINGF_WRAPPER_MANAGER_H_ */

